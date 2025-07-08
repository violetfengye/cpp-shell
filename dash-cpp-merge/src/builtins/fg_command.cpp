/**
 * @file fg_command.cpp
 * @brief 前台命令实现
 */

#include "builtins/fg_command.h"
#include "core/shell.h"
#include "job/job_control.h"
#include <iostream>
#include <string>
#include <vector>
#include <cctype>

namespace dash
{

    FgCommand::FgCommand(Shell *shell)
        : BuiltinCommand(shell, "fg")
    {
    }

    FgCommand::~FgCommand()
    {
    }

    int FgCommand::execute(const std::vector<std::string> &args)
    {
        // 获取作业控制器
        JobControl *job_control = shell_->getJobControl();
        if (!job_control) {
            std::cerr << "fg: 无法获取作业控制器" << std::endl;
            return 1;
        }

        // 如果没有参数，默认使用第一个作业
        int job_id = 0;
        if (args.size() <= 1) {
            // 查找第一个作业
            for (int i = 1; i <= 100; i++) {
                Job* job = job_control->getJobByJobId(i);
                if (job) {
                    job_id = i;
                    break;
                }
            }
            
            if (job_id <= 0) {
                std::cerr << "fg: 当前没有作业" << std::endl;
                return 1;
            }
        } else {
            // 解析作业ID
            std::string job_spec = args[1];
            if (job_spec[0] == '%') {
                // %N 形式
                if (job_spec.size() == 1) {
                    std::cerr << "fg: 缺少作业说明符" << std::endl;
                    return 1;
                }
                
                if (job_spec[1] == '+' || job_spec[1] == '%') {
                    // %+ 或 %% 表示当前作业 - 找第一个作业
                    for (int i = 1; i <= 100; i++) {
                        Job* job = job_control->getJobByJobId(i);
                        if (job) {
                            job_id = i;
                            break;
                        }
                    }
                } else if (job_spec[1] == '-') {
                    // %- 表示最近的另一个作业
                    bool found_first = false;
                    for (int i = 1; i <= 100; i++) {
                        Job* job = job_control->getJobByJobId(i);
                        if (job) {
                            if (!found_first) {
                                found_first = true; // 跳过第一个作业
                            } else {
                                job_id = i;
                                break;
                            }
                        }
                    }
                } else if (std::isdigit(job_spec[1])) {
                    // %N 表示作业号为N的作业
                    try {
                        job_id = std::stoi(job_spec.substr(1));
                    } catch (const std::exception &) {
                        std::cerr << "fg: 无效的作业号: " << job_spec.substr(1) << std::endl;
                        return 1;
                    }
                } else {
                    // %string 表示命令以string开头的作业
                    std::string prefix = job_spec.substr(1);
                    for (int i = 1; i <= 100; i++) {
                        Job* job = job_control->getJobByJobId(i);
                        if (job && job->getCommand().substr(0, prefix.size()) == prefix) {
                            job_id = i;
                            break;
                        }
                    }
                }
            } else {
                // 直接是数字，表示作业号
                try {
                    job_id = std::stoi(job_spec);
                } catch (const std::exception &) {
                    std::cerr << "fg: 无效的作业号: " << job_spec << std::endl;
                    return 1;
                }
            }
        }

        // 检查作业是否存在
        Job* job = job_control->getJobByJobId(job_id);
        if (!job) {
            std::cerr << "fg: 作业 " << job_id << " 不存在" << std::endl;
            return 1;
        }

        // 输出作业命令
        std::cout << job->getCommand() << std::endl;

        // 前台运行作业
        return job_control->continueJob(job, true);
    }

    std::string FgCommand::getHelp() const
    {
        return "fg [作业ID]\n"
               "  将作业移至前台运行。\n"
               "  作业ID可以是以下形式:\n"
               "    %N        作业号为N的作业\n"
               "    %+, %%    当前作业\n"
               "    %-        上一个作业\n"
               "    %string   命令以string开头的作业\n"
               "    N         作业号为N的作业\n"
               "  如果不指定作业ID，则默认使用当前作业。";
    }

} // namespace dash 