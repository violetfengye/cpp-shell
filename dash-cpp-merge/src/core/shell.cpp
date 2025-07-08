/**
 * @file shell.cpp
 * @brief Shell核心实现
 */

#include "core/shell.h"
#include "builtins/builtin_command.h"
#include "builtins/cd_command.h"
#include "builtins/debug_command.h"
#include "builtins/echo_command.h"
#include "builtins/exit_command.h"
#include "builtins/fg_command.h"
#include "builtins/bg_command.h"
#include "builtins/help_command.h"
#include "builtins/history_command.h"
#include "builtins/jobs_command.h"
#include "builtins/pwd_command.h"
#include "builtins/source_command.h"
#include "builtins/kill_command.h"
#include "builtins/wait_command.h"
#include "variable/variable_manager.h"
#include "core/alias.h"
#include "core/arithmetic.h"
#include "core/executor.h"
#include "core/expand.h"
#include "core/input.h"
#include "core/lexer.h"
#include "core/output.h"
#include "core/parser.h"
#include "core/signal_handler.h"
#include "utils/history.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace dash
{
    // 全局Shell实例，用于信号处理
    static Shell* g_shell_instance = nullptr;

    Shell::Shell()
        : lexer_(nullptr), parser_(nullptr), executor_(nullptr),
          job_control_(nullptr), variable_manager_(nullptr),
          alias_manager_(nullptr), arithmetic_(nullptr),
          history_(nullptr), input_(nullptr), output_(nullptr),
          expand_(nullptr), signal_handler_(nullptr),
          interactive_(false), exit_requested_(false), exit_code_(0),
          debug_mode_(false)
    {
        g_shell_instance = this;
    }

    Shell::~Shell()
    {
        cleanup();
        g_shell_instance = nullptr;
    }

    bool Shell::initialize(int argc, char** argv)
    {
        // 检查是否为交互式模式
        interactive_ = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);

        // 创建各个组件
        variable_manager_ = new VariableManager(this);
        if (!variable_manager_) {
            std::cerr << "无法初始化变量管理器" << std::endl;
            return false;
        }

        alias_manager_ = new AliasManager(this);
        if (!alias_manager_) {
            std::cerr << "无法初始化别名管理器" << std::endl;
            return false;
        }

        arithmetic_ = new Arithmetic(this);
        if (!arithmetic_) {
            std::cerr << "无法初始化算术表达式计算器" << std::endl;
            return false;
        }

        history_ = new History(this);
        if (!history_) {
            std::cerr << "无法初始化历史记录管理器" << std::endl;
            return false;
        }

        input_ = new Input(this);
        if (!input_) {
            std::cerr << "无法初始化输入管理器" << std::endl;
            return false;
        }

        output_ = new Output(this);
        if (!output_) {
            std::cerr << "无法初始化输出管理器" << std::endl;
            return false;
        }

        expand_ = new Expand(this);
        if (!expand_) {
            std::cerr << "无法初始化扩展器" << std::endl;
            return false;
        }

        signal_handler_ = new SignalHandler(this);
        if (!signal_handler_) {
            std::cerr << "无法初始化信号处理器" << std::endl;
            return false;
        }

        job_control_ = new JobControl(this);
        if (!job_control_) {
            std::cerr << "无法初始化任务控制" << std::endl;
            return false;
        }

        lexer_ = new Lexer(this);
        if (!lexer_) {
            std::cerr << "无法初始化词法分析器" << std::endl;
            return false;
        }

        parser_ = new Parser(this);
        if (!parser_) {
            std::cerr << "无法初始化语法分析器" << std::endl;
            return false;
        }

        executor_ = new Executor(this);
        if (!executor_) {
            std::cerr << "无法初始化执行器" << std::endl;
            return false;
        }

        // 初始化组件
        if (!variable_manager_->initialize()) {
            std::cerr << "初始化变量管理器失败" << std::endl;
            return false;
        }

        if (!alias_manager_->initialize()) {
            std::cerr << "初始化别名管理器失败" << std::endl;
            return false;
        }

        if (!arithmetic_->initialize()) {
            std::cerr << "初始化算术表达式计算器失败" << std::endl;
            return false;
        }

        if (!history_->initialize()) {
            std::cerr << "初始化历史记录管理器失败" << std::endl;
            return false;
        }

        if (!input_->initialize()) {
            std::cerr << "初始化输入管理器失败" << std::endl;
            return false;
        }

        if (!output_->initialize()) {
            std::cerr << "初始化输出管理器失败" << std::endl;
            return false;
        }

        if (!expand_->initialize()) {
            std::cerr << "初始化扩展器失败" << std::endl;
            return false;
        }

        if (!signal_handler_->initialize()) {
            std::cerr << "初始化信号处理器失败" << std::endl;
            return false;
        }

        if (!job_control_->initialize()) {
            std::cerr << "初始化任务控制失败" << std::endl;
            return false;
        }

        if (!lexer_->initialize()) {
            std::cerr << "初始化词法分析器失败" << std::endl;
            return false;
        }

        if (!parser_->initialize()) {
            std::cerr << "初始化语法分析器失败" << std::endl;
            return false;
        }

        if (!executor_->initialize()) {
            std::cerr << "初始化执行器失败" << std::endl;
            return false;
        }

        // 注册内置命令
        registerBuiltinCommands();

        // 处理命令行参数
        if (!processCommandLineArgs(argc, argv)) {
            return false;
        }

        return true;
    }

    void Shell::cleanup()
    {
        // 释放所有内置命令
        for (auto& command : builtin_commands_) {
            delete command.second;
        }
        builtin_commands_.clear();

        // 释放各个组件
        if (executor_) {
            delete executor_;
            executor_ = nullptr;
        }

        if (parser_) {
            delete parser_;
            parser_ = nullptr;
        }

        if (lexer_) {
            delete lexer_;
            lexer_ = nullptr;
        }

        if (job_control_) {
            delete job_control_;
            job_control_ = nullptr;
        }

        if (signal_handler_) {
            delete signal_handler_;
            signal_handler_ = nullptr;
        }

        if (expand_) {
            delete expand_;
            expand_ = nullptr;
        }

        if (output_) {
            delete output_;
            output_ = nullptr;
        }

        if (input_) {
            delete input_;
            input_ = nullptr;
        }

        if (history_) {
            delete history_;
            history_ = nullptr;
        }

        if (arithmetic_) {
            delete arithmetic_;
            arithmetic_ = nullptr;
        }

        if (alias_manager_) {
            delete alias_manager_;
            alias_manager_ = nullptr;
        }

        if (variable_manager_) {
            delete variable_manager_;
            variable_manager_ = nullptr;
        }
    }

    void Shell::run()
    {
        if (interactive_) {
            runInteractive();
        } else {
            runScript();
        }
    }

    void Shell::runInteractive()
    {
        // 显示欢迎信息
        std::cout << "欢迎使用Dash-CPP合并版 Shell" << std::endl;
        std::cout << "输入'help'获取帮助，输入'exit'退出" << std::endl;

        // 主循环
        while (!exit_requested_) {
            // 显示提示符
            showPrompt();

            // 获取输入
            std::string line = input_->readLine();
            if (input_->isEOF()) {
                exit_requested_ = true;
                break;
            }

            // 忽略空行
            if (line.empty()) {
                continue;
            }

            // 添加到历史记录
            history_->addToHistory(line);

            // 执行命令
            executeCommand(line);

            // 清理完成的作业
            job_control_->cleanupJobs();
        }

        // 检查是否有停止的作业
        if (job_control_->hasStoppedJobs()) {
            std::cout << "你有停止的作业。" << std::endl;
        }
    }

    void Shell::runScript()
    {
        std::string line;
        
        while (!exit_requested_ && !input_->isEOF()) {
            // 获取输入
            line = input_->readLine();
            if (input_->isEOF()) {
                break;
            }

            // 忽略空行
            if (line.empty()) {
                continue;
            }

            // 执行命令
            executeCommand(line);
        }
    }

    int Shell::executeCommand(const std::string& command)
    {
        if (command.empty()) {
            return 0;
        }

        // 解析命令
        lexer_->setInput(command);
        Node* root = parser_->parse();
        if (!root) {
            // 解析错误
            if (debug_mode_) {
                std::cerr << "解析错误：无法解析命令" << std::endl;
            }
            return 1;
        }

        // 执行命令
        int result = executor_->execute(root);

        // 释放节点
        delete root;

        return result;
    }

    void Shell::showPrompt()
    {
        std::string cwd = getCurrentWorkingDirectory();
        std::string username = getenv("USER") ? getenv("USER") : "user";
        std::string hostname = getHostname();

        std::cout << "\033[1;32m" << username << "@" << hostname << "\033[0m:"
                  << "\033[1;34m" << cwd << "\033[0m$ ";
        std::cout.flush();
    }

    std::string Shell::getCurrentWorkingDirectory()
    {
        char buf[PATH_MAX];
        if (getcwd(buf, sizeof(buf)) != nullptr) {
            return std::string(buf);
        }
        return std::string(".");
    }

    std::string Shell::getHostname()
    {
        char buf[256];
        if (gethostname(buf, sizeof(buf)) == 0) {
            return std::string(buf);
        }
        return std::string("localhost");
    }

    bool Shell::processCommandLineArgs(int argc, char** argv)
    {
        // 处理命令行选项
        for (int i = 1; i < argc; i++) {
            std::string arg(argv[i]);
            
            if (arg == "-c" && i + 1 < argc) {
                // 执行命令后退出
                executeCommand(argv[i + 1]);
                exit_requested_ = true;
                return true;
            } else if (arg == "-d" || arg == "--debug") {
                // 开启调试模式
                debug_mode_ = true;
                std::cout << "调试模式已启用" << std::endl;
            } else if (arg[0] == '-') {
                // 未知选项
                std::cerr << "未知选项: " << arg << std::endl;
                return false;
            } else {
                // 脚本文件
                if (!executeScript(arg)) {
                    return false;
                }
                exit_requested_ = true;
                return true;
            }
        }

        return true;
    }

    bool Shell::executeScript(const std::string& filename)
    {
        // 打开脚本文件
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "无法打开脚本文件: " << filename << std::endl;
            return false;
        }

        // 保存当前输入状态
        Input* old_input = input_;
        
        // 创建新的文件输入
        Input* file_input = new Input(this, &file);
        input_ = file_input;

        // 读取并执行脚本文件的每一行
        std::string line;
        while (!exit_requested_ && !input_->isEOF()) {
            line = input_->readLine();
            if (input_->isEOF()) {
                break;
            }

            // 忽略空行
            if (line.empty()) {
                continue;
            }

            // 执行命令
            executeCommand(line);
        }

        // 恢复输入状态
        delete file_input;
        input_ = old_input;

        return true;
    }

    void Shell::registerBuiltinCommands()
    {
        // 注册内置命令
        registerBuiltinCommand(new CdCommand(this));
        registerBuiltinCommand(new EchoCommand(this));
        registerBuiltinCommand(new ExitCommand(this));
        registerBuiltinCommand(new PwdCommand(this));
        registerBuiltinCommand(new JobsCommand(this));
        registerBuiltinCommand(new FgCommand(this));
        registerBuiltinCommand(new BgCommand(this));
        registerBuiltinCommand(new KillCommand(this));
        registerBuiltinCommand(new WaitCommand(this));
        
        // 附加命令，如果需要
        registerBuiltinCommand(new DebugCommand(this));
        registerBuiltinCommand(new HelpCommand(this));
        registerBuiltinCommand(new HistoryCommand(this));
        registerBuiltinCommand(new SourceCommand(this));
    }

    void Shell::registerBuiltinCommand(BuiltinCommand* command)
    {
        if (command) {
            builtin_commands_[command->getName()] = command;
        }
    }

    BuiltinCommand* Shell::getBuiltinCommand(const std::string& name)
    {
        auto it = builtin_commands_.find(name);
        if (it != builtin_commands_.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool Shell::isBuiltinCommand(const std::string& name)
    {
        return builtin_commands_.find(name) != builtin_commands_.end();
    }

    int Shell::executeBuiltinCommand(const std::string& name, int argc, char** argv)
    {
        auto it = builtin_commands_.find(name);
        if (it != builtin_commands_.end()) {
            return it->second->execute(argc, argv);
        }
        return -1;
    }

    void Shell::requestExit(int exit_code)
    {
        exit_requested_ = true;
        exit_code_ = exit_code;
    }

    bool Shell::isExitRequested() const
    {
        return exit_requested_;
    }

    int Shell::getExitCode() const
    {
        return exit_code_;
    }

    bool Shell::isInteractive() const
    {
        return interactive_;
    }

    void Shell::setDebugMode(bool enabled)
    {
        debug_mode_ = enabled;
    }

    bool Shell::isDebugMode() const
    {
        return debug_mode_;
    }

    // 获取各种组件
    Lexer* Shell::getLexer()
    {
        return lexer_;
    }

    Parser* Shell::getParser()
    {
        return parser_;
    }

    Executor* Shell::getExecutor()
    {
        return executor_;
    }

    JobControl* Shell::getJobControl()
    {
        return job_control_;
    }

    VariableManager* Shell::getVariableManager()
    {
        return variable_manager_;
    }

    AliasManager* Shell::getAliasManager()
    {
        return alias_manager_;
    }

    Arithmetic* Shell::getArithmetic()
    {
        return arithmetic_;
    }

    History* Shell::getHistory()
    {
        return history_;
    }

    Input* Shell::getInput()
    {
        return input_;
    }

    Output* Shell::getOutput()
    {
        return output_;
    }

    Expand* Shell::getExpand()
    {
        return expand_;
    }

    SignalHandler* Shell::getSignalHandler()
    {
        return signal_handler_;
    }

    // 静态信号处理函数
    void Shell::handleSignal(int signum)
    {
        if (g_shell_instance) {
            g_shell_instance->onSignal(signum);
        }
    }

    void Shell::onSignal(int signum)
    {
        if (signal_handler_) {
            signal_handler_->handleSignal(signum);
        }
    }

    void Shell::handleInterrupt()
    {
        // 处理Ctrl+C中断
        if (interactive_) {
            std::cout << std::endl;
            displayPrompt();
        }
        
        // 如果有前台作业，发送SIGINT信号给前台进程组
        if (job_control_ && job_control_->isEnabled()) {
            Job* foreground_job = job_control_->getForegroundJob();
            if (foreground_job) {
                kill(-foreground_job->getPgid(), SIGINT);
            }
        }
    }

} // namespace dash 