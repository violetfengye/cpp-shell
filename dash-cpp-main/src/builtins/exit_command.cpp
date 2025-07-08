/**
 * @file exit_command.cpp
 * @brief Exit命令类实现
 */

#include <iostream>
#include <cstdlib>
#include "builtins/exit_command.h"
#include "core/shell.h"
#include "utils/error.h"
#include "core/executor.h"
#include "job/job_control.h"

namespace dash
{

    ExitCommand::ExitCommand(Shell *shell)
        : BuiltinCommand(shell)
    {
    }

    int ExitCommand::execute(const std::vector<std::string> &args)
    {
        int status = 0;

        // 如果提供了退出状态码参数
        if (args.size() > 1)
        {
            try
            {
                status = std::stoi(args[1]);
            }
            catch (const std::exception &e)
            {
                std::cerr << "exit: " << args[1] << ": 数字参数无效" << std::endl;
                status = 2; // 按照POSIX标准，无效的数字参数应该返回2
            }
        }
        else
        {
            // 使用上一个命令的退出状态
            status = shell_->getExecutor()->getLastStatus();
        }

        // 如果有作业在后台运行，并且是交互式shell，则提示用户
        if (shell_->isInteractive() && shell_->getJobControl()->hasActiveJobs())
        {
            std::cerr << "exit: 有后台作业在运行" << std::endl;
            return 1;
        }

        // 请求退出shell
        shell_->exit(status);

        // 这里抛出一个异常，使得shell可以立即退出当前命令执行循环
        throw ShellException(ExceptionType::EXIT, "Exit requested");
    }

    std::string ExitCommand::getName() const
    {
        return "exit";
    }

    std::string ExitCommand::getHelp() const
    {
        return "exit [n] - 退出shell，状态码为n（默认为最后执行的命令的退出状态）";
    }

} // namespace dash