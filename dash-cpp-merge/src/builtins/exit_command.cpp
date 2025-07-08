/**
 * @file exit_command.cpp
 * @brief 退出命令实现
 */

#include "builtins/exit_command.h"
#include "core/shell.h"
#include "job/job_control.h"
#include <iostream>
#include <cstdlib>

namespace dash
{

    ExitCommand::ExitCommand(Shell *shell)
        : BuiltinCommand(shell, "exit")
    {
    }

    ExitCommand::~ExitCommand()
    {
    }

    std::string ExitCommand::getHelp() const
    {
        return "退出Shell，状态码为N；如果N省略，则状态码为最后一条执行的命令的退出状态";
    }

    int ExitCommand::execute(const std::vector<std::string> &args)
    {
        int exit_code = 0;

        // 检查参数数量
        if (args.size() > 1) {
            // 尝试将参数转换为退出码
            try {
                exit_code = std::stoi(args[1]);
            } catch (const std::exception &) {
                std::cerr << "exit: " << args[1] << ": 需要数字参数" << std::endl;
                return 1;
            }
        }

        // 检查是否有停止的作业
        JobControl *job_control = shell_->getJobControl();
        if (job_control && job_control->hasStoppedJobs()) {
            std::cout << "你有停止的作业。" << std::endl;
            // 在非交互式模式下，退出操作不会被阻止
            if (shell_->isInteractive()) {
                return 1;
            }
        }

        // 请求退出Shell
        shell_->exit(exit_code);
        return 0;
    }

} // namespace dash 