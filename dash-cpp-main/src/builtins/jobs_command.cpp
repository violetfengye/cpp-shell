/**
 * @file jobs_command.cpp
 * @brief Jobs命令类实现
 */

#include <iostream>
#include <getopt.h>
#include <vector>
#include "builtins/jobs_command.h"
#include "core/shell.h"
#include "job/job_control.h"

namespace dash
{

    JobsCommand::JobsCommand(Shell *shell)
        : BuiltinCommand(shell)
    {
    }

    int JobsCommand::execute(const std::vector<std::string> &args)
    {
        bool list_pids = false;
        bool list_running = false;
        bool list_stopped = false;
        bool changed_only = false;

        // 手动解析选项，避免使用 getopt
        for (size_t i = 1; i < args.size(); ++i) {
            const std::string &arg = args[i];
            if (arg[0] == '-') {
                for (size_t j = 1; j < arg.size(); ++j) {
                    switch (arg[j]) {
                    case 'l':
                        list_pids = true;
                        break;
                    case 'p':
                        list_pids = true;
                        break;
                    case 'r':
                        list_running = true;
                        break;
                    case 's':
                        list_stopped = true;
                        break;
                    default:
                        std::cerr << "jobs: 无效选项: -" << arg[j] << std::endl;
                        std::cerr << "jobs: 用法: jobs [-lprs]" << std::endl;
                        return 1;
                    }
                }
            } else {
                std::cerr << "jobs: 无效参数: " << arg << std::endl;
                std::cerr << "jobs: 用法: jobs [-lprs]" << std::endl;
                return 1;
            }
        }

        // 如果没有指定-r或-s，则显示所有作业
        if (!list_running && !list_stopped)
        {
            list_running = list_stopped = true;
        }

        // 在显示作业前强制更新所有作业的状态
        shell_->getJobControl()->updateStatus(0);

        // 显示作业
        shell_->getJobControl()->showJobs(changed_only, list_running, list_stopped, list_pids);

        return 0;
    }

    std::string JobsCommand::getName() const
    {
        return "jobs";
    }

    std::string JobsCommand::getHelp() const
    {
        return "jobs [-lprs] - 列出活动作业";
    }

} // namespace dash