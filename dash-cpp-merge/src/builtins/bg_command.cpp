/**
 * @file bg_command.cpp
 * @brief 后台命令实现
 */

#include "builtins/bg_command.h"
#include "core/shell.h"
#include "job/job_control.h"
#include <iostream>
#include <string>
#include <vector>
#include <cctype>

namespace dash
{

    BgCommand::BgCommand(Shell *shell)
        : BuiltinCommand(shell, "bg")
    {
    }

    BgCommand::~BgCommand()
    {
    }

    int BgCommand::execute(const std::vector<std::string> &args)
    {
        // 获取作业控制器
        JobControl *job_control = shell_->getJobControl();
        if (!job_control) {
            std::cerr << "bg: 无法获取作业控制器" << std::endl;
            return 1;
        }

        // 如果没有参数，默认使用当前停止的作业
        int job_id = 0;
        if (args.size() <= 1) {
            // 检查是否有停止的作业
            if (!job_control->hasStoppedJobs()) {
                std::cerr << "bg: 当前没有停止的作业" << std::endl;
                return 1;
            }
            
            // 查找第一个停止的作业
            // 我们需要遍历所有可能的作业ID来找到一个停止的作业
            for (int i = 1; i <= 100; i++) { // 假设作业ID不超过100
                Job* job = job_control->getJobByJobId(i);
                if (job && job->getState() == JobState::STOPPED) {
                    job_id = i;
                    break;
                }
            }
            
            if (job_id <= 0) {
                std::cerr << "bg: 无法找到停止的作业" << std::endl;
                return 1;
            }
        } else {
            // 解析作业ID
            std::string job_spec = args[1];
            if (job_spec[0] == '%') {
                // %N 形式
                if (job_spec.size() == 1) {
                    std::cerr << "bg: 缺少作业说明符" << std::endl;
                    return 1;
                }
                
                if (job_spec[1] == '+' || job_spec[1] == '%') {
                    // %+ 或 %% 表示当前作业 - 找第一个停止的作业
                    for (int i = 1; i <= 100; i++) {
                        Job* job = job_control->getJobByJobId(i);
                        if (job && job->getState() == JobState::STOPPED) {
                            job_id = i;
                            break;
                        }
                    }
                } else if (job_spec[1] == '-') {
                    // %- 表示最近的另一个停止的作业
                    bool found_first = false;
                    for (int i = 1; i <= 100; i++) {
                        Job* job = job_control->getJobByJobId(i);
                        if (job && job->getState() == JobState::STOPPED) {
                            if (!found_first) {
                                found_first = true; // 跳过第一个停止的作业
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
                        std::cerr << "bg: 无效的作业号: " << job_spec.substr(1) << std::endl;
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
                    std::cerr << "bg: 无效的作业号: " << job_spec << std::endl;
                    return 1;
                }
            }
        }

        // 检查作业是否存在
        Job* job = job_control->getJobByJobId(job_id);
        if (!job) {
            std::cerr << "bg: 作业 " << job_id << " 不存在" << std::endl;
            return 1;
        }

        // 检查作业是否已停止
        if (job->getState() != JobState::STOPPED) {
            std::cerr << "bg: 作业 " << job_id << " 已经在后台运行" << std::endl;
            return 1;
        }

        // 输出作业信息
        std::cout << "[" << job_id << "] " << job->getCommand() << " &" << std::endl;

        // 后台继续运行作业
        return job_control->continueJob(job, false);
    }

    std::string BgCommand::getHelp() const
    {
        return "bg [作业ID]\n"
               "  将已停止的作业在后台继续运行。\n"
               "  作业ID可以是以下形式:\n"
               "    %N        作业号为N的作业\n"
               "    %+, %%    当前作业\n"
               "    %-        上一个作业\n"
               "    %string   命令以string开头的作业\n"
               "    N         作业号为N的作业\n"
               "  如果不指定作业ID，则默认使用当前作业。";
    }

} // namespace dash 