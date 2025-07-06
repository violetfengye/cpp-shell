/**
 * @file fg_command.cpp
 * @brief Fg命令类实现
 */

#include <iostream>
#include "builtins/fg_command.h"
#include "core/shell.h"
#include "job/job_control.h"

namespace dash
{

    FgCommand::FgCommand(Shell *shell)
        : BuiltinCommand(shell)
    {
    }

    int FgCommand::execute(const std::vector<std::string> &args)
    {
        // 检查作业控制是否启用
        if (!shell_->getJobControl()->isEnabled())
        {
            std::cerr << "fg: 作业控制未启用" << std::endl;
            return 1;
        }

        int job_id;

        // 如果没有指定作业ID，使用最近的作业
        if (args.size() < 2)
        {
            // TODO: 获取最近的作业ID
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
                std::cerr << "fg: " << job_arg << ": 无效的作业规格" << std::endl;
                return 1;
            }
        }

        // 将作业放入前台
        int status = shell_->getJobControl()->putJobInForeground(job_id, true);

        if (status < 0)
        {
            std::cerr << "fg: 作业 " << job_id << " 不存在" << std::endl;
            return 1;
        }

        return status;
    }

    std::string FgCommand::getName() const
    {
        return "fg";
    }

    std::string FgCommand::getHelp() const
    {
        return "fg [job_id] - 将作业放入前台";
    }

} // namespace dash