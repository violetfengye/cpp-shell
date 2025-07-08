/**
 * @file executor.cpp
 * @brief 执行器类实现
 */

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <algorithm>
#include "core/executor.h"
#include "core/shell.h"
#include "core/node.h"
#include "job/job_control.h"
#include "utils/error.h"
#include "variable/variable_manager.h"
#include "builtins/cd_command.h"
#include "builtins/echo_command.h"
#include "builtins/exit_command.h"
#include "builtins/pwd_command.h"
#include "builtins/jobs_command.h"
#include "builtins/fg_command.h"
#include "builtins/bg_command.h"
#include "builtins/sprf_command.h"
#include "builtins/su_command.h"
#include "builtins/tsl_command.h"

namespace dash
{

    Executor::Executor(Shell *shell)
        : shell_(shell), last_status_(0)
    {
        registerBuiltins();
    }

    Executor::~Executor()
    {
    }

    int Executor::execute(const Node *node)
    {
        if (!node)
        {
            return 0;
        }

        try
        {
            int status = 0;

            switch (node->getType())
            {
            case NodeType::COMMAND:
                status = executeCommand(static_cast<const CommandNode *>(node));
                break;

            case NodeType::PIPE:
                status = executePipe(static_cast<const PipeNode *>(node));
                break;

            case NodeType::LIST:
                status = executeList(static_cast<const ListNode *>(node));
                break;

            case NodeType::IF:
                status = executeIf(static_cast<const IfNode *>(node));
                break;

            case NodeType::FOR:
                status = executeFor(static_cast<const ForNode *>(node));
                break;

            case NodeType::WHILE:
                status = executeWhile(static_cast<const WhileNode *>(node));
                break;

            case NodeType::CASE:
                status = executeCase(static_cast<const CaseNode *>(node));
                break;

            case NodeType::SUBSHELL:
                status = executeSubshell(static_cast<const SubshellNode *>(node));
                break;

            default:
                throw ShellException(ExceptionType::INTERNAL, "Unknown node type");
            }

            last_status_ = status;
            return status;
        }
        catch (const ShellException &e)
        {
            std::cerr << e.getTypeString() << ": " << e.what() << std::endl;
            last_status_ = 1;
            return 1;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            last_status_ = 1;
            return 1;
        }
    }

    int Executor::executeCommand(const CommandNode *command)
    {
        if (command->getArgs().empty())
        {
            // 如果只有变量赋值，则设置变量
            for (const auto &assignment : command->getAssignments())
            {
                size_t pos = assignment.find('=');
                if (pos != std::string::npos)
                {
                    std::string name = assignment.substr(0, pos);
                    std::string value = assignment.substr(pos + 1);
                    // 对赋值的值进行变量展开
                    value = shell_->getVariableManager()->expand(value);
                    shell_->getVariableManager()->set(name, value);
                }
            }
            return 0;
        }

        // 获取命令名和参数
        std::vector<std::string> args = command->getArgs();
        
        // 对所有参数进行变量展开
        for (auto &arg : args)
        {
            arg = shell_->getVariableManager()->expand(arg);
        }
        
        std::string cmd_name = args[0];

        // 处理变量赋值
        for (const auto &assignment : command->getAssignments())
        {
            size_t pos = assignment.find('=');
            if (pos != std::string::npos)
            {
                std::string name = assignment.substr(0, pos);
                std::string value = assignment.substr(pos + 1);
                // 对赋值的值进行变量展开
                value = shell_->getVariableManager()->expand(value);
                shell_->getVariableManager()->set(name, value, Variable::VAR_NONE); // 临时变量
            }
        }
        // 处理sudo命令，待完善
        /*
        if (cmd_name == "sudo"){
            std::cout<<"sudo -> "<<cmd_name<<" args[0]: "<<args[0]<<std::endl;
            cmd_name = args[1];
            args.erase(args.begin());
            std::cout<<"sudo args"<<args[0]<<std::endl;
        }
        */
        
        // 检查是否是内置命令
        if (isBuiltin(cmd_name))
        {
            // 设置重定向
            std::unordered_map<int, int> saved_fds;
            bool redirect_success = applyRedirections(command->getRedirections(), saved_fds);

            if (!redirect_success)
            {
                return 1;
            }

            // 执行内置命令
            int status = executeBuiltin(cmd_name, args);

            // 恢复重定向
            restoreRedirections(saved_fds);

            return status;
        }

        // 执行外部命令
        return executeExternalCommand(cmd_name, args, command->getRedirections(), false);
    }

    int Executor::executePipe(const PipeNode *pipe_node)
    {
        // 如果是后台运行，创建作业
        if (pipe_node->isBackground())
        {
            // 创建作业
            // 这里需要实现作业控制
            // 暂时简单实现，后续完善
            pid_t pid = fork();

            if (pid == -1)
            {
                throw ShellException(ExceptionType::SYSTEM, "Failed to fork process");
            }
            else if (pid == 0)
            {
                // 子进程
                // 执行管道
                if (pipe_node->getRight())
                {
                    // 创建管道
                    int pipefd[2];
                    if (::pipe(pipefd) == -1)
                    {
                        std::cerr << "Failed to create pipe" << std::endl;
                        exit(1);
                    }

                    // 创建左侧命令的子进程
                    pid_t left_pid = fork();

                    if (left_pid == -1)
                    {
                        std::cerr << "Failed to fork process" << std::endl;
                        exit(1);
                    }
                    else if (left_pid == 0)
                    {
                        // 左侧命令的子进程
                        // 关闭读取端
                        close(pipefd[0]);

                        // 将标准输出重定向到管道写入端
                        dup2(pipefd[1], STDOUT_FILENO);
                        close(pipefd[1]);

                        // 执行左侧命令
                        exit(execute(pipe_node->getLeft()));
                    }

                    // 创建右侧命令的子进程
                    pid_t right_pid = fork();

                    if (right_pid == -1)
                    {
                        std::cerr << "Failed to fork process" << std::endl;
                        exit(1);
                    }
                    else if (right_pid == 0)
                    {
                        // 右侧命令的子进程
                        // 关闭写入端
                        close(pipefd[1]);

                        // 将标准输入重定向到管道读取端
                        dup2(pipefd[0], STDIN_FILENO);
                        close(pipefd[0]);

                        // 执行右侧命令
                        exit(execute(pipe_node->getRight()));
                    }

                    // 父进程关闭管道的两端
                    close(pipefd[0]);
                    close(pipefd[1]);

                    // 等待左侧命令完成
                    int status;
                    waitpid(left_pid, &status, 0);

                    // 等待右侧命令完成
                    waitpid(right_pid, &status, 0);

                    exit(WEXITSTATUS(status));
                }
                else
                {
                    // 只有左侧命令
                    exit(execute(pipe_node->getLeft()));
                }
            }

            // 父进程
            std::cout << "[" << pid << "] " << "Background job started" << std::endl;
            return 0;
        }

        // 前台运行
        if (pipe_node->getRight())
        {
            // 创建管道
            int pipefd[2];
            if (::pipe(pipefd) == -1)
            {
                throw ShellException(ExceptionType::SYSTEM, "Failed to create pipe");
            }

            // 创建左侧命令的子进程
            pid_t left_pid = fork();

            if (left_pid == -1)
            {
                throw ShellException(ExceptionType::SYSTEM, "Failed to fork process");
            }
            else if (left_pid == 0)
            {
                // 子进程
                // 关闭读取端
                close(pipefd[0]);

                // 将标准输出重定向到管道写入端
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);

                // 执行左侧命令
                exit(execute(pipe_node->getLeft()));
            }

            // 创建右侧命令的子进程
            pid_t right_pid = fork();

            if (right_pid == -1)
            {
                throw ShellException(ExceptionType::SYSTEM, "Failed to fork process");
            }
            else if (right_pid == 0)
            {
                // 子进程
                // 关闭写入端
                close(pipefd[1]);

                // 将标准输入重定向到管道读取端
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]);

                // 执行右侧命令
                exit(execute(pipe_node->getRight()));
            }

            // 父进程关闭管道的两端
            close(pipefd[0]);
            close(pipefd[1]);

            // 等待左侧命令完成
            int left_status;
            waitpid(left_pid, &left_status, 0);

            // 等待右侧命令完成
            int right_status;
            waitpid(right_pid, &right_status, 0);

            // 返回右侧命令的状态码
            return WEXITSTATUS(right_status);
        }
        else
        {
            // 只有左侧命令
            return execute(pipe_node->getLeft());
        }
    }

    int Executor::executeList(const ListNode *list)
    {
        int status = 0;

        const auto &commands = list->getCommands();
        const auto &operators = list->getOperators();

        for (size_t i = 0; i < commands.size(); ++i)
        {
            // 执行当前命令
            status = execute(commands[i].get());

            // 根据操作符决定是否继续执行
            if (i < operators.size())
            {
                if (operators[i] == "&&" && status != 0)
                {
                    // && 操作符，如果前一个命令失败，则跳过后续命令
                    break;
                }
                else if (operators[i] == "||" && status == 0)
                {
                    // || 操作符，如果前一个命令成功，则跳过后续命令
                    break;
                }
            }
        }

        return status;
    }

    int Executor::executeIf(const IfNode *if_node)
    {
        // 执行条件
        int condition_status = execute(if_node->getCondition());

        // 如果条件为真（状态码为0），执行 then 部分
        if (condition_status == 0)
        {
            return execute(if_node->getThenPart());
        }
        else if (if_node->getElsePart())
        {
            // 否则，如果有 else 部分，执行 else 部分
            return execute(if_node->getElsePart());
        }

        return condition_status;
    }

    int Executor::executeFor(const ForNode *for_node)
    {
        int status = 0;

        // 获取循环变量和单词列表
        const std::string &var = for_node->getVar();
        const auto &words = for_node->getWords();

        // 遍历单词列表
        for (const auto &word : words)
        {
            // 设置循环变量
            shell_->getVariableManager()->set(var, word);

            // 执行循环体
            status = execute(for_node->getBody());

            // 如果循环体中有 break 或 continue 命令，需要处理
            // 暂时简单实现，后续完善
        }

        return status;
    }

    int Executor::executeWhile(const WhileNode *while_node)
    {
        int status = 0;

        while (true)
        {
            // 执行条件
            int condition_status = execute(while_node->getCondition());

            // 根据条件和循环类型决定是否执行循环体
            bool execute_body = false;

            if (while_node->isUntil())
            {
                // until 循环，条件为假（状态码非0）时执行循环体
                execute_body = (condition_status != 0);
            }
            else
            {
                // while 循环，条件为真（状态码为0）时执行循环体
                execute_body = (condition_status == 0);
            }

            if (!execute_body)
            {
                break;
            }

            // 执行循环体
            status = execute(while_node->getBody());

            // 如果循环体中有 break 或 continue 命令，需要处理
            // 暂时简单实现，后续完善
        }

        return status;
    }

    int Executor::executeCase(const CaseNode *case_node)
    {
        int status = 0;

        // 获取匹配词
        std::string word = case_node->getWord();

        // 替换变量
        // 暂时简单实现，后续完善

        // 遍历 case 项
        for (const auto &item : case_node->getItems())
        {
            // 检查是否匹配
            bool matched = false;

            for (const auto &pattern : item->patterns)
            {
                // 简单实现，后续完善为正则匹配
                if (pattern == word || pattern == "*")
                {
                    matched = true;
                    break;
                }
            }

            if (matched)
            {
                // 执行匹配项的命令
                status = execute(item->commands.get());
                break;
            }
        }

        return status;
    }

    int Executor::executeSubshell(const SubshellNode *subshell)
    {
        // 创建子进程
        pid_t pid = fork();

        if (pid == -1)
        {
            throw ShellException(ExceptionType::SYSTEM, "Failed to fork process");
        }
        else if (pid == 0)
        {
            // 子进程

            // 设置重定向
            std::unordered_map<int, int> saved_fds;
            bool redirect_success = applyRedirections(subshell->getRedirections(), saved_fds);

            if (!redirect_success)
            {
                exit(1);
            }

            // 执行命令
            int status = execute(subshell->getCommands());

            // 恢复重定向
            restoreRedirections(saved_fds);

            exit(status);
        }

        // 父进程等待子进程完成
        int status;
        waitpid(pid, &status, 0);

        return WEXITSTATUS(status);
    }

    bool Executor::applyRedirections(const std::vector<Redirection> &redirections, std::unordered_map<int, int> &saved_fds)
    {
        for (const auto &redir : redirections)
        {
            int fd = redir.fd;
            std::string filename = redir.filename;
            
            // 对文件名进行变量展开
            filename = shell_->getVariableManager()->expand(filename);

            // 保存原始文件描述符
            int saved_fd = dup(fd);
            if (saved_fd == -1)
            {
                // 恢复已保存的文件描述符
                restoreRedirections(saved_fds);
                return false;
            }
            saved_fds[fd] = saved_fd;

            // 应用重定向
            switch (redir.type)
            {
            case RedirType::REDIR_INPUT:
                // 输入重定向
                {
                    int new_fd = open(filename.c_str(), O_RDONLY);
                    if (new_fd == -1)
                    {
                        std::cerr << "dash: " << filename << ": " << strerror(errno) << std::endl;
                        restoreRedirections(saved_fds);
                        return false;
                    }
                    dup2(new_fd, fd);
                    close(new_fd);
                }
                break;

            case RedirType::REDIR_OUTPUT:
                // 输出重定向
                {
                    int new_fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    if (new_fd == -1)
                    {
                        std::cerr << "dash: " << filename << ": " << strerror(errno) << std::endl;
                        restoreRedirections(saved_fds);
                        return false;
                    }
                    dup2(new_fd, fd);
                    close(new_fd);
                }
                break;

            case RedirType::REDIR_APPEND:
                // 追加重定向
                {
                    int new_fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
                    if (new_fd == -1)
                    {
                        std::cerr << "dash: " << filename << ": " << strerror(errno) << std::endl;
                        restoreRedirections(saved_fds);
                        return false;
                    }
                    dup2(new_fd, fd);
                    close(new_fd);
                }
                break;

            case RedirType::REDIR_INPUT_DUP:
            {
                // 输入复制重定向 <&
                int target_fd;
                if (filename == "-")
                {
                    // 关闭文件描述符
                    close(fd);
                }
                else
                {
                    // 复制文件描述符
                    target_fd = std::stoi(filename);
                    dup2(target_fd, fd);
                }
                break;
            }

            case RedirType::REDIR_OUTPUT_DUP:
            {
                // 输出复制重定向 >&
                int target_fd;
                if (filename == "-")
                {
                    // 关闭文件描述符
                    close(fd);
                }
                else
                {
                    // 复制文件描述符
                    target_fd = std::stoi(filename);
                    dup2(target_fd, fd);
                }
                break;
            }

            case RedirType::REDIR_HEREDOC:
            {
                // Here 文档 <<
                // 暂时简单实现，后续完善
                break;
            }
            }
        }

        return true;
    }

    void Executor::restoreRedirections(std::unordered_map<int, int> &saved_fds)
    {
        for (const auto &pair : saved_fds)
        {
            dup2(pair.second, pair.first);
            close(pair.second);
        }

        saved_fds.clear();
    }

    void Executor::exec_in_child(const std::string &command, const std::vector<std::string> &args) {
        std::vector<char *> c_args;
        c_args.reserve(args.size() + 1);
        for (const auto &arg : args) {
            c_args.push_back(const_cast<char *>(arg.c_str()));
        }
        c_args.push_back(nullptr);

        execvp(command.c_str(), c_args.data());
        // 如果 execvp 返回，则表示执行失败
        std::cerr << "Failed to execute command: " << command << std::endl;
        exit(1);
    }

    int Executor::executeExternalCommand(const std::string &command, const std::vector<std::string> &args,
                                         const std::vector<Redirection> &redirections, bool background)
    {
        // 创建子进程
        pid_t pid = fork();

        if (pid == -1)
        {
            throw ShellException(ExceptionType::SYSTEM, "Failed to fork process");
        }
        else if (pid == 0)
        {
            // 子进程

            // 设置重定向
            std::unordered_map<int, int> saved_fds;
            bool redirect_success = applyRedirections(redirections, saved_fds);

            if (!redirect_success)
            {
                exit(1);
            }

            exec_in_child(command, args);
        }

        // 父进程
        if (background)
        {
            // 后台运行，不等待子进程完成
            std::cout << "[" << pid << "] " << command << std::endl;
            return 0;
        }
        else
        {
            // 前台运行，等待子进程完成
            int status;
            waitpid(pid, &status, 0);

            return WEXITSTATUS(status);
        }
    }

    bool Executor::isBuiltin(const std::string &command) const
    {
        return builtins_.find(command) != builtins_.end();
    }

    int Executor::executeBuiltin(const std::string &command, const std::vector<std::string> &args)
    {
        auto it = builtins_.find(command);
        if (it != builtins_.end())
        {
            return it->second(args);
        }

        return 1;
    }

    void Executor::registerBuiltins()
    {
        // 创建内置命令对象
        auto cd_cmd = std::make_shared<CdCommand>(shell_);
        auto echo_cmd = std::make_shared<EchoCommand>(shell_);
        auto exit_cmd = std::make_shared<ExitCommand>(shell_);
        auto pwd_cmd = std::make_shared<PwdCommand>(shell_);
        auto jobs_cmd = std::make_shared<JobsCommand>(shell_);
        auto fg_cmd = std::make_shared<FgCommand>(shell_);
        auto bg_cmd = std::make_shared<BgCommand>(shell_);
        auto sprf_cmd = std::make_shared<SprfCommand>(shell_);
        auto su_cmd = std::make_shared<SuCommand>(shell_);
        auto tsl_cmd = std::make_shared<TslCommand>(shell_);

        // 保存内置命令对象
        builtin_commands_.push_back(cd_cmd);
        builtin_commands_.push_back(echo_cmd);
        builtin_commands_.push_back(exit_cmd);
        builtin_commands_.push_back(pwd_cmd);
        builtin_commands_.push_back(jobs_cmd);
        builtin_commands_.push_back(fg_cmd);
        builtin_commands_.push_back(bg_cmd);
        builtin_commands_.push_back(sprf_cmd);
        builtin_commands_.push_back(su_cmd);
        builtin_commands_.push_back(tsl_cmd);

        // 注册内置命令
        builtins_[cd_cmd->getName()] = [cd_cmd](const std::vector<std::string> &args) -> int
        {
            return cd_cmd->execute(args);
        };

        builtins_[echo_cmd->getName()] = [echo_cmd](const std::vector<std::string> &args) -> int
        {
            return echo_cmd->execute(args);
        };

        builtins_[exit_cmd->getName()] = [exit_cmd](const std::vector<std::string> &args) -> int
        {
            return exit_cmd->execute(args);
        };

        builtins_[pwd_cmd->getName()] = [pwd_cmd](const std::vector<std::string> &args) -> int
        {
            return pwd_cmd->execute(args);
        };

        builtins_[jobs_cmd->getName()] = [jobs_cmd](const std::vector<std::string> &args) -> int
        {
            return jobs_cmd->execute(args);
        };

        builtins_[fg_cmd->getName()] = [fg_cmd](const std::vector<std::string> &args) -> int
        {
            return fg_cmd->execute(args);
        };

        builtins_[bg_cmd->getName()] = [bg_cmd](const std::vector<std::string> &args) -> int
        {
            return bg_cmd->execute(args);
        };

        builtins_[sprf_cmd->getName()] = [sprf_cmd](const std::vector<std::string> &args) -> int
        {
            return sprf_cmd->execute(args);
        };
        builtins_[su_cmd->getName()] = [su_cmd](const std::vector<std::string> &args) -> int
        {
            return su_cmd->execute(args);
        };
        builtins_[tsl_cmd->getName()] = [tsl_cmd](const std::vector<std::string> &args) -> int
        {
            return tsl_cmd->execute(args);
        };

        // TODO: 添加更多内置命令
    }

} // namespace dash