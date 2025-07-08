/**
 * @file jobs_command.cpp
 * @brief 作业列表命令实现
 */

#include "builtins/jobs_command.h"
#include "core/shell.h"
#include "job/job_control.h"
#include <iostream>
#include <cstring>
#include <unistd.h>

namespace dash
{

    JobsCommand::JobsCommand(Shell *shell)
        : BuiltinCommand(shell, "jobs")
    {
    }

    JobsCommand::~JobsCommand()
    {
    }

    std::string JobsCommand::getHelp() const
    {
        return "列出活动作业。\n"
               "选项:\n"
               "  -l  显示进程ID和更多信息\n"
               "  -p  仅显示进程ID\n"
               "  -r  仅显示运行中的作业\n"
               "  -s  仅显示停止的作业";
    }

    int JobsCommand::execute(const std::vector<std::string> &args)
    {
        // 获取作业控制器
        JobControl *job_control = shell_->getJobControl();
        if (!job_control) {
            std::cerr << "作业控制未初始化" << std::endl;
            return 1;
        }

        // 解析选项
        bool show_pids = false;
        bool only_running = false;
        bool only_stopped = false;

        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i][0] == '-') {
                // 处理选项
                const char *opt = args[i].c_str() + 1;
                while (*opt) {
                    switch (*opt) {
                    case 'l':
                        show_pids = true;
                        break;
                    case 'p':
                        show_pids = true;
                        break;
                    case 'r':
                        only_running = true;
                        break;
                    case 's':
                        only_stopped = true;
                        break;
                    default:
                        std::cerr << "jobs: 未知选项: -" << *opt << std::endl;
                        return 1;
                    }
                    opt++;
                }
            }
        }

        // 如果both only_running和only_stopped都被设置，显示所有作业
        bool show_running = !only_stopped || only_running;
        bool show_stopped = !only_running || only_stopped;

        // 显示作业
        job_control->showJobs(show_running, show_stopped, show_pids);

        return 0;
    }

} // namespace dash 