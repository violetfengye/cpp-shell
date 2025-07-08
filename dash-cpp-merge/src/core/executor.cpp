/**
 * @file executor.cpp
 * @brief 命令执行器实现
 */

#include "core/executor.h"
#include "core/shell.h"
#include "core/node.h"
#include "core/expand.h"
#include "core/output.h"
#include "builtins/builtin_command.h"
#include "job/job_control.h"
#include "variable/variable_manager.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

namespace dash
{

    Executor::Executor(Shell *shell)
        : shell_(shell)
    {
    }

    Executor::~Executor()
    {
    }

    bool Executor::initialize()
    {
        return true;
    }

    int Executor::execute(Node *node)
    {
        if (!node) {
            return 0;
        }

        // 根据节点类型执行不同操作
        switch (node->getType()) {
        case NodeType::COMMAND:
            return executeCommand(node);
        case NodeType::PIPE:
            return executePipe(node);
        case NodeType::SEQUENCE:
            return executeSequence(node);
        case NodeType::BACKGROUND:
            return executeBackground(node);
        case NodeType::IF:
            return executeIf(node);
        case NodeType::WHILE:
            return executeWhile(node);
        case NodeType::FOR:
            return executeFor(node);
        case NodeType::CASE:
            return executeCase(node);
        case NodeType::FUNCTION:
            return executeFunction(node);
        case NodeType::SUBSHELL:
            return executeSubshell(node);
        case NodeType::GROUP:
            return executeGroup(node);
        default:
            std::cerr << "未知的节点类型" << std::endl;
            return 1;
        }
    }

    int Executor::executeCommand(Node *node)
    {
        if (!node) {
            return 0;
        }

        // 获取命令名和参数
        std::string command_name = node->getCommandName();
        if (command_name.empty()) {
            return 0;
        }

        // 展开命令名（变量和通配符等）
        Expand *expand = shell_->getExpand();
        std::vector<std::string> expanded_args;
        
        if (!expand->expandCommand(node, expanded_args)) {
            std::cerr << "展开命令失败" << std::endl;
            return 1;
        }

        if (expanded_args.empty()) {
            return 0;
        }

        command_name = expanded_args[0];

        // 检查是否为内置命令
        if (shell_->isBuiltinCommand(command_name)) {
            // 转换参数格式
            std::vector<char *> argv;
            for (const auto &arg : expanded_args) {
                argv.push_back(const_cast<char *>(arg.c_str()));
            }
            argv.push_back(nullptr);

            // 执行内置命令
            return shell_->executeBuiltinCommand(command_name, argv.size() - 1, argv.data());
        }

        // 准备执行外部命令
        return executeExternalCommand(expanded_args, node->getRedirections());
    }

    int Executor::executeExternalCommand(const std::vector<std::string> &args, const std::vector<Redirection> &redirections)
    {
        if (args.empty()) {
            return 0;
        }

        // 创建作业
        JobControl *job_control = shell_->getJobControl();
        std::string cmd_str;
        for (const auto &arg : args) {
            cmd_str += arg + " ";
        }
        
        Job *job = job_control->createJob(cmd_str);
        if (!job) {
            std::cerr << "创建作业失败" << std::endl;
            return 1;
        }

        // 转换参数格式
        std::vector<char *> argv;
        for (const auto &arg : args) {
            argv.push_back(const_cast<char *>(arg.c_str()));
        }
        argv.push_back(nullptr);

        // 设置重定向
        std::vector<int> saved_fds;
        if (!setupRedirections(redirections, saved_fds)) {
            std::cerr << "设置重定向失败" << std::endl;
            return 1;
        }

        // 在前台执行命令
        int status = job_control->runInForeground(job, cmd_str, argv.data());

        // 恢复重定向
        restoreRedirections(redirections, saved_fds);

        return status;
    }

    bool Executor::setupRedirections(const std::vector<Redirection> &redirections, std::vector<int> &saved_fds)
    {
        for (const auto &redir : redirections) {
            int fd = -1;
            int saved_fd = -1;

            // 保存原始文件描述符
            saved_fd = dup(redir.fd);
            if (saved_fd == -1) {
                std::cerr << "无法复制文件描述符: " << strerror(errno) << std::endl;
                restoreRedirections(redirections, saved_fds);
                return false;
            }
            saved_fds.push_back(saved_fd);

            // 根据重定向类型执行不同操作
            switch (redir.type) {
            case RedirectionType::INPUT:
                fd = open(redir.filename.c_str(), O_RDONLY);
                break;
            case RedirectionType::OUTPUT:
                fd = open(redir.filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                break;
            case RedirectionType::APPEND:
                fd = open(redir.filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
                break;
            case RedirectionType::DUPLICATE:
                fd = atoi(redir.filename.c_str());
                break;
            case RedirectionType::HERE_DOC:
                // 创建临时文件，写入here document内容
                char temp_filename[] = "/tmp/dash_heredoc_XXXXXX";
                fd = mkstemp(temp_filename);
                if (fd != -1) {
                    write(fd, redir.here_doc.c_str(), redir.here_doc.size());
                    lseek(fd, 0, SEEK_SET);
                    unlink(temp_filename);  // 文件将在关闭时被删除
                }
                break;
            }

            if (fd == -1) {
                std::cerr << "无法打开文件 '" << redir.filename << "': " << strerror(errno) << std::endl;
                restoreRedirections(redirections, saved_fds);
                return false;
            }

            // 重定向文件描述符
            if (dup2(fd, redir.fd) == -1) {
                std::cerr << "重定向失败: " << strerror(errno) << std::endl;
                close(fd);
                restoreRedirections(redirections, saved_fds);
                return false;
            }

            // 如果不是复制，关闭原始文件描述符
            if (redir.type != RedirectionType::DUPLICATE) {
                close(fd);
            }
        }

        return true;
    }

    void Executor::restoreRedirections(const std::vector<Redirection> &redirections, std::vector<int> &saved_fds)
    {
        for (size_t i = 0; i < saved_fds.size() && i < redirections.size(); ++i) {
            if (saved_fds[i] != -1) {
                dup2(saved_fds[i], redirections[i].fd);
                close(saved_fds[i]);
            }
        }
    }

    int Executor::executePipe(Node *node)
    {
        if (!node) {
            return 0;
        }

        Node *left_node = node->getLeftChild();
        Node *right_node = node->getRightChild();

        if (!left_node || !right_node) {
            std::cerr << "无效的管道命令" << std::endl;
            return 1;
        }

        // 创建管道
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            std::cerr << "创建管道失败: " << strerror(errno) << std::endl;
            return 1;
        }

        // 保存标准输入输出
        int saved_stdout = dup(STDOUT_FILENO);
        int saved_stdin = dup(STDIN_FILENO);

        // 执行左边命令，输出到管道
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        
        int left_status = execute(left_node);

        // 恢复标准输出并准备执行右边命令
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);

        // 执行右边命令，从管道读取输入
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        
        int right_status = execute(right_node);

        // 恢复标准输入
        dup2(saved_stdin, STDIN_FILENO);
        close(saved_stdin);

        // 返回右边命令的状态
        return right_status;
    }

    int Executor::executeSequence(Node *node)
    {
        if (!node) {
            return 0;
        }

        Node *left_node = node->getLeftChild();
        Node *right_node = node->getRightChild();

        // 执行左边命令
        int left_status = 0;
        if (left_node) {
            left_status = execute(left_node);
        }

        // 如果shell请求退出，不执行右边命令
        if (shell_->isExitRequested()) {
            return left_status;
        }

        // 执行右边命令
        int right_status = 0;
        if (right_node) {
            right_status = execute(right_node);
        }

        // 返回最后执行的命令的状态
        return right_status;
    }

    int Executor::executeBackground(Node *node)
    {
        if (!node) {
            return 0;
        }

        Node *command_node = node->getChild();
        if (!command_node) {
            return 0;
        }

        // 获取命令名和参数
        std::string command_name = command_node->getCommandName();
        if (command_name.empty()) {
            return 0;
        }

        // 展开命令名和参数
        Expand *expand = shell_->getExpand();
        std::vector<std::string> expanded_args;
        
        if (!expand->expandCommand(command_node, expanded_args)) {
            std::cerr << "展开命令失败" << std::endl;
            return 1;
        }

        if (expanded_args.empty()) {
            return 0;
        }

        command_name = expanded_args[0];

        // 创建作业
        JobControl *job_control = shell_->getJobControl();
        std::string cmd_str;
        for (const auto &arg : expanded_args) {
            cmd_str += arg + " ";
        }
        
        Job *job = job_control->createJob(cmd_str);
        if (!job) {
            std::cerr << "创建作业失败" << std::endl;
            return 1;
        }

        // 转换参数格式
        std::vector<char *> argv;
        for (const auto &arg : expanded_args) {
            argv.push_back(const_cast<char *>(arg.c_str()));
        }
        argv.push_back(nullptr);

        // 设置重定向
        std::vector<Redirection> redirections = command_node->getRedirections();
        std::vector<int> saved_fds;
        if (!setupRedirections(redirections, saved_fds)) {
            std::cerr << "设置重定向失败" << std::endl;
            return 1;
        }

        // 在后台执行命令
        int status = job_control->runInBackground(job, cmd_str, argv.data());

        // 恢复重定向
        restoreRedirections(redirections, saved_fds);

        return 0;  // 后台命令总是返回0
    }

    int Executor::executeIf(Node *node)
    {
        if (!node) {
            return 0;
        }

        // 执行条件
        Node *condition = node->getCondition();
        if (!condition) {
            return 0;
        }

        int condition_status = execute(condition);

        // 根据条件选择执行哪个分支
        if (condition_status == 0) {
            // 条件为真，执行then分支
            Node *then_branch = node->getThenBranch();
            if (then_branch) {
                return execute(then_branch);
            }
        } else {
            // 条件为假，执行else分支
            Node *else_branch = node->getElseBranch();
            if (else_branch) {
                return execute(else_branch);
            }
        }

        return condition_status;
    }

    int Executor::executeWhile(Node *node)
    {
        if (!node) {
            return 0;
        }

        Node *condition = node->getCondition();
        Node *body = node->getBody();

        if (!condition || !body) {
            return 0;
        }

        int status = 0;
        
        // 重复执行，直到条件为假
        while (!shell_->isExitRequested()) {
            status = execute(condition);
            
            if (status != 0) {
                break;  // 条件为假，退出循环
            }
            
            status = execute(body);
            
            // 如果是break或continue，需要特殊处理
            // 这里简化处理，实际应该检查控制流类型
        }

        return status;
    }

    int Executor::executeFor(Node *node)
    {
        if (!node) {
            return 0;
        }

        std::string var_name = node->getVarName();
        std::vector<std::string> values = node->getValues();
        Node *body = node->getBody();

        if (var_name.empty() || !body) {
            return 0;
        }

        // 如果没有指定值，使用位置参数
        if (values.empty()) {
            // TODO: 从变量管理器获取位置参数
        }

        int status = 0;

        // 遍历每个值
        VariableManager *var_manager = shell_->getVariableManager();
        for (const auto &value : values) {
            if (shell_->isExitRequested()) {
                break;
            }

            // 设置循环变量
            var_manager->setVariable(var_name, value);
            
            // 执行循环体
            status = execute(body);
            
            // 如果是break或continue，需要特殊处理
            // 这里简化处理，实际应该检查控制流类型
        }

        return status;
    }

    int Executor::executeCase(Node *node)
    {
        if (!node) {
            return 0;
        }

        std::string word = node->getWord();
        std::vector<std::pair<std::vector<std::string>, Node *>> cases = node->getCases();

        if (word.empty()) {
            return 0;
        }

        // 展开word
        Expand *expand = shell_->getExpand();
        std::vector<std::string> expanded_word;
        expand->expandWord(word, expanded_word);

        if (expanded_word.empty()) {
            return 0;
        }

        word = expanded_word[0];

        // 遍历每个case
        for (const auto &case_item : cases) {
            const auto &patterns = case_item.first;
            Node *actions = case_item.second;

            // 检查每个模式
            for (const auto &pattern : patterns) {
                // 展开模式
                std::vector<std::string> expanded_pattern;
                expand->expandWord(pattern, expanded_pattern);

                if (expanded_pattern.empty()) {
                    continue;
                }

                std::string expanded_pat = expanded_pattern[0];

                // 检查模式是否匹配
                if (patternMatch(word, expanded_pat)) {
                    // 执行匹配的动作
                    if (actions) {
                        return execute(actions);
                    }
                    return 0;
                }
            }
        }

        return 0;
    }

    bool Executor::patternMatch(const std::string &str, const std::string &pattern)
    {
        // 简单的通配符匹配实现
        // TODO: 实现完整的shell通配符匹配
        return str == pattern;
    }

    int Executor::executeFunction(Node *node)
    {
        // 函数定义
        // TODO: 实现函数定义和执行
        return 0;
    }

    int Executor::executeSubshell(Node *node)
    {
        if (!node) {
            return 0;
        }

        Node *command = node->getChild();
        if (!command) {
            return 0;
        }

        // 创建子进程
        pid_t pid = fork();
        
        if (pid == -1) {
            std::cerr << "创建子进程失败: " << strerror(errno) << std::endl;
            return 1;
        } else if (pid == 0) {
            // 子进程
            int status = execute(command);
            _exit(status);
        } else {
            // 父进程
            int status;
            waitpid(pid, &status, 0);
            
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                return 128 + WTERMSIG(status);
            }
            
            return 1;
        }
    }

    int Executor::executeGroup(Node *node)
    {
        if (!node) {
            return 0;
        }

        Node *command = node->getChild();
        if (!command) {
            return 0;
        }

        // 在当前shell中执行命令
        return execute(command);
    }

} // namespace dash 