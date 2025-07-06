/**
 * @file shell.cpp
 * @brief Shell 类实现 (已修复)
 */

#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <cstring>
#include <cerrno> // 需要包含 errno
#include <sys/wait.h>
#include <vector>
#include "core/shell.h"
#include "core/input.h"
#include "core/parser.h"
#include "core/executor.h"
#include "variable/variable_manager.h"
#include "job/job_control.h"
#include "utils/error.h"

// ================================= 重要提示 =================================
// 请确保您已经在 "shell.h" 的 Shell 类定义中添加了以下这行代码：
//
// class Shell {
// public:
//     // ... 其他成员 ...
//     static volatile sig_atomic_t received_sigchld;
//     static volatile sig_atomic_t received_sigint; // <-- 请添加这一行
// };
//
// ==========================================================================

namespace dash
{

    // 初始化静态成员变量
    volatile sig_atomic_t Shell::received_sigchld = 0;
    volatile sig_atomic_t Shell::received_sigint = 0; // 新增：用于SIGINT的标志

    // 全局 Shell 实例指针，用于信号处理
    static Shell *g_shell = nullptr;

    // 信号处理函数 - 现在绝对安全
    static void signalHandler(int signo)
    {
        if (signo == SIGCHLD) {
            Shell::received_sigchld = 1;
        } else if (signo == SIGINT) {
            Shell::received_sigint = 1;
        }
    }

    Shell::Shell()
        : variable_manager_(std::make_unique<VariableManager>(this)),
          parser_(std::make_unique<Parser>(this)),
          executor_(std::make_unique<Executor>(this)),
          job_control_(std::make_unique<JobControl>(this)),
          interactive_(false),
          exit_requested_(false),
          exit_status_(0)
    {
        // 创建输入处理器
        input_ = std::make_unique<InputHandler>(this);
        
        // 设置全局 Shell 实例指针
        g_shell = this;

        // 初始化信号处理
        setupSignalHandlers();
    }

    Shell::~Shell()
    {
        // 清除全局 Shell 实例指针
        if (g_shell == this)
        {
            g_shell = nullptr;
        }
    }

    void Shell::setupSignalHandlers()
    {
        // 设置信号处理函数
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = signalHandler;
        
        // 我们不希望系统调用被信号中断后自动重启
        // 这样可以确保系统调用（如read）在收到信号后正确返回 EINTR
        sa.sa_flags = 0; // 不设置SA_RESTART
        
        // 在信号处理期间阻塞所有信号
        sigfillset(&sa.sa_mask);

        // 设置SIGINT, SIGQUIT, SIGCHLD信号处理
        sigaction(SIGINT, &sa, nullptr);
        sigaction(SIGQUIT, &sa, nullptr);
        sigaction(SIGCHLD, &sa, nullptr);
    }

    // handleSignal 方法已被移除，因为它是不安全的。所有逻辑移至主循环。

    int Shell::run(int argc, char *argv[])
    {
        // 解析命令行参数
        if (!parseArgs(argc, argv))
        {
            return 1;
        }

        // 检查是否是交互式模式
        interactive_ = isatty(STDIN_FILENO) && script_file_.empty() && command_string_.empty();

        // 设置环境变量
        setupEnvironment();

        // 主循环
        if (!script_file_.empty() || !command_string_.empty())
        {
            // 执行脚本文件或命令字符串
            return runScript();
        }
        else if (interactive_)
        {
            // 交互式模式
            return runInteractive();
        }
        else
        {
            // 标准输入重定向但不是交互式
            return runScript();
        }
    }

    bool Shell::parseArgs(int argc, char *argv[])
    {
        // 解析命令行参数 (无变化)
        for (int i = 1; i < argc; ++i)
        {
            std::string arg = argv[i];

            if (arg == "-c")
            {
                if (i + 1 < argc)
                {
                    command_string_ = argv[++i];
                }
                else
                {
                    std::cerr << "dash: -c: option requires an argument" << std::endl;
                    return false;
                }
            }
            else if (arg[0] == '-')
            {
                std::cerr << "dash: " << arg << ": invalid option" << std::endl;
                return false;
            }
            else
            {
                script_file_ = arg;
                for (int j = i; j < argc; ++j)
                {
                    script_args_.push_back(argv[j]);
                }
                break;
            }
        }
        return true;
    }

    void Shell::setupEnvironment()
    {
        // 设置环境变量 (无变化)
        variable_manager_->initialize();
        if (variable_manager_->get("PS1").empty())
        {
            variable_manager_->set("PS1", "$ ");
        }
        if (variable_manager_->get("PS2").empty())
        {
            variable_manager_->set("PS2", "> ");
        }
    }

    // --- 核心修改部分 ---
    // 重写 runInteractive 方法，使其更健壮
    int Shell::runInteractive()
    {
        std::cout << "Dash-CPP Shell Created by Isaleafa." << std::endl;

        sigset_t block_mask, orig_mask;
        sigemptyset(&block_mask);
        sigaddset(&block_mask, SIGCHLD);

        while (!exit_requested_)
        {
            // 在循环开始，先安全地处理所有挂起的信号事件
            sigprocmask(SIG_BLOCK, &block_mask, &orig_mask);

            // 修复：使用 Shell:: 作用域访问静态成员
            if (Shell::received_sigint) {
                Shell::received_sigint = 0;
                std::cout << std::endl; // 响应 Ctrl+C，打印换行
            }
            
            // 修复：使用 Shell:: 作用域访问静态成员
            if (Shell::received_sigchld) {
                Shell::received_sigchld = 0;
                if (job_control_ && job_control_->isEnabled()) {
                    job_control_->updateStatus(0); // 更新所有作业状态
                    
                    // 打印已完成作业的通知
                    bool has_notification = false;
                    for (const auto& pair : job_control_->getJobs()) {
                        const auto& job = pair.second;
                        if (job->getStatus() == JobStatus::DONE && !job->isNotified()) {
                             if (!has_notification) {
                                std::cout << std::endl; // 在打印提示符前换行
                                has_notification = true;
                            }
                            std::cout << "[" << job->getId() << "] 已完成\t" << job->getCommand() << std::endl;
                            const_cast<Job*>(job.get())->setNotified(true);
                        }
                    }
                    job_control_->cleanupJobs(); // 清理已完成且已通知的作业
                }
            }
            
            // 恢复信号掩码，准备读取用户输入
            sigprocmask(SIG_SETMASK, &orig_mask, nullptr);

            // --- 交互逻辑 ---
            try
            {
                // 1. 显示提示符
                displayPrompt();

                // 2. 读取并解析命令 (此步骤可能会被信号中断)
                std::unique_ptr<Node> command = parser_->parseCommand(true);

                // 检查是否是文件结尾 (Ctrl+D)
                if (input_->isEOF()) {
                    std::cout << "exit" << std::endl;
                    break; // 退出循环
                }

                if (!command) {
                    continue;
                }

                // 3. 执行命令
                // 在执行期间阻塞SIGCHLD，防止在操作作业列表时出现竞态条件
                sigprocmask(SIG_BLOCK, &block_mask, &orig_mask);
                if (command->getType() == NodeType::PIPE) {
                    execute_pipeline(static_cast<const PipeNode*>(command.get()));
                } else {
                    executor_->execute(command.get());
                }
                sigprocmask(SIG_SETMASK, &orig_mask, nullptr);
            }
            catch (const ShellException &e)
            {
                std::cerr << e.getTypeString() << ": " << e.what() << std::endl;
                // 确保在异常情况下恢复信号掩码
                sigprocmask(SIG_SETMASK, &orig_mask, nullptr);
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error: " << e.what() << std::endl;
                // 确保在异常情况下恢复信号掩码
                sigprocmask(SIG_SETMASK, &orig_mask, nullptr);
            }
        }

        return exit_status_;
    }

    int Shell::runScript()
    {
        // 修复：恢复到原始的、正确的逐行读取逻辑
        try
        {
            if (!script_file_.empty())
            {
                input_->pushFile(script_file_, InputHandler::IF_NONE);
                for (size_t i = 0; i < script_args_.size(); ++i)
                {
                    variable_manager_->set(std::to_string(i), script_args_[i]);
                }
                variable_manager_->set("#", std::to_string(script_args_.size()));

                while (!exit_requested_ && !input_->isEOF())
                {
                    std::string line = input_->readLine(false);
                    if (!line.empty())
                    {
                        parser_->setInput(line);
                        std::unique_ptr<Node> command = parser_->parseCommand(false);
                        if (command)
                        {
                            if (command->getType() == NodeType::PIPE) {
                                execute_pipeline(static_cast<const PipeNode*>(command.get()));
                            } else {
                                executor_->execute(command.get());
                            }
                        }
                    }
                }
            }
            else if (!command_string_.empty())
            {
                parser_->setInput(command_string_);
                std::unique_ptr<Node> command = parser_->parseCommand(false);
                if (command)
                {
                    if (command->getType() == NodeType::PIPE) {
                        execute_pipeline(static_cast<const PipeNode*>(command.get()));
                    } else {
                        executor_->execute(command.get());
                    }
                }
            }
        }
        catch (const ShellException &e)
        {
            std::cerr << e.getTypeString() << ": " << e.what() << std::endl;
            return 1;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
        return exit_status_;
    }

    void Shell::displayPrompt()
    {
        // 显示提示符 (无变化)
        std::string ps1 = variable_manager_->get("PS1");
        if (ps1.empty())
        {
            ps1 = "$ ";
        }
        std::cout << ps1 << std::flush;
    }

    void Shell::exit(int status)
    {
        exit_requested_ = true;
        exit_status_ = status;
    }

    void collect_pipe_commands(const Node* node, std::vector<const Node*>& commands) {
        if (node->getType() == NodeType::PIPE) {
            const auto* pipe_node = static_cast<const PipeNode*>(node);
            collect_pipe_commands(pipe_node->getLeft(), commands);
            collect_pipe_commands(pipe_node->getRight(), commands);
        } else {
            commands.push_back(node);
        }
    }

    int Shell::execute_pipeline(const PipeNode *node) {
        int status = 0;
        std::vector<const Node*> commands;
        collect_pipe_commands(node, commands);

        int in_fd = 0;
        std::vector<pid_t> pids;

        for (size_t i = 0; i < commands.size(); ++i) {
            int pipe_fds[2];
            if (i < commands.size() - 1) {
                if (pipe(pipe_fds) < 0) {
                    perror("pipe");
                    return -1;
                }
            }

            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                return -1;
            }

            if (pid == 0) { // Child
                if (i > 0) {
                    dup2(in_fd, 0);
                    close(in_fd);
                }
                if (i < commands.size() - 1) {
                    close(pipe_fds[0]);
                    dup2(pipe_fds[1], 1);
                    close(pipe_fds[1]);
                }

                const auto *command_node = dynamic_cast<const CommandNode *>(commands[i]);
                if (command_node) {
                    executor_->exec_in_child(command_node->getArgs()[0], command_node->getArgs());
                } else {
                    exit(EXIT_FAILURE);
                }
            }

            pids.push_back(pid);

            if (i > 0) {
                close(in_fd);
            }
            if (i < commands.size() - 1) {
                in_fd = pipe_fds[0];
                close(pipe_fds[1]);
            }
        }

        for (pid_t pid : pids) {
            int child_status;
            waitpid(pid, &child_status, 0);
            if (WIFEXITED(child_status)) {
                status = WEXITSTATUS(child_status);
            }
        }

        return status;
    }

    // Getters (无变化)
    InputHandler *Shell::getInput() const { return input_.get(); }
    VariableManager *Shell::getVariableManager() const { return variable_manager_.get(); }
    Parser *Shell::getParser() const { return parser_.get(); }
    Executor *Shell::getExecutor() const { return executor_.get(); }
    JobControl *Shell::getJobControl() const { return job_control_.get(); }
    bool Shell::isInteractive() const { return interactive_; }
    int Shell::getExitStatus() const { return exit_status_; }

    // createShell (无变化)
    int createShell(int argc, char *argv[])
    {
        std::unique_ptr<Shell> shell = std::make_unique<Shell>();
        return shell->run(argc, argv);
    }

} // namespace dash