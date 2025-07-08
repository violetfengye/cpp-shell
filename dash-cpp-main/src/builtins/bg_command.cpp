/**
 * @file bg_command.cpp
 * @brief Bg命令类实现
 */

#include <iostream>
#include "builtins/bg_command.h"
#include "core/shell.h"
#include "job/job_control.h"
#include "utils/error.h"

namespace dash
{

    BgCommand::BgCommand(Shell *shell)
        : BuiltinCommand(shell)
    {
    }

    int BgCommand::execute(const std::vector<std::string> &args)
    {
        // 检查作业控制是否启用
        if (!shell_->getJobControl()->isEnabled())
        {
            std::cerr << "bg: 作业控制未启用" << std::endl;
            return 1;
        }

        int job_id;

        // 如果没有指定作业ID，使用最近的已停止作业
        if (args.size() < 2)
        {
            // TODO: 获取最近的已停止作业ID
            // 暂时简单实现，使用ID为1的作业
            job_id = 1;
        }
        else
        {
            // 解析作业ID
            std::string job_arg = args[1];

            // 检查是否是作业ID格式（以%开头）
            if (job_arg[0] == '%')
            {
                job_arg = job_arg.substr(1);
            }

            try
            {
                job_id = std::stoi(job_arg);
            }
            catch (const std::exception &e)
            {
                std::cerr << "bg: " << job_arg << ": 无效的作业规格" << std::endl;
                return 1;
            }
        }

        // 将作业放入后台并继续运行
        try
        {
            shell_->getJobControl()->putJobInBackground(job_id, true);
        }
        catch (const ShellException &e)
        {
            std::cerr << "bg: " << e.what() << std::endl;
            return 1;
        }

        return 0;
    }

    std::string BgCommand::getName() const
    {
        return "bg";
    }

    std::string BgCommand::getHelp() const
    {
        return "bg [job_id] - 在后台继续运行已停止的作业";
    }

} // namespace dash