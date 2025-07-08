/**
 * @file exit_command.cpp
 * @brief Exit命令类实现
 */

#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include "builtins/exit_command.h"
#include "core/shell.h"
#include "utils/error.h"
#include "core/executor.h"
#include "job/job_control.h"
#include "../core/debug.h"

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

        // 获取作业控制对象
        JobControl* jc = shell_->getJobControl();
        if (!jc) {
            std::cerr << "错误: 作业控制对象为空!" << std::endl;
            return 1;
        }

        // 多次更新所有作业状态，确保捕获到所有状态变化
        // 特别是对于刚刚完成的后台作业
        for (int i = 0; i < 3; i++) {
            jc->updateStatus(0);
            // 短暂延时，确保作业状态有时间更新
            if (i < 2) usleep(100000); // 100ms
        }
        
        // 强制更新所有作业状态
        jc->updateStatus(0);
        
        // 立即清理所有已完成的作业
        jc->cleanupJobs();
        
        // 再次强制更新
        jc->updateStatus(0);
        
        // 再次清理
        jc->cleanupJobs();
        
        // 检查是否有活动的作业（运行中或已停止）
        bool has_active_jobs = false;
        const auto &jobs = jc->getJobs();
        
        // 手动检查每个作业
        for (const auto &pair : jobs) {
            const auto &job = pair.second;
            JobStatus job_status = job->getStatus();
            
            if (job_status == JobStatus::RUNNING || job_status == JobStatus::STOPPED) {
                has_active_jobs = true;
                break;
            }
        }
        
        // 在继续前再次检查作业的状态
        if (has_active_jobs) {
            // 输出作业状态详细信息
            DebugLog::logCommand("当前有活动的作业:");
            for (const auto &pair : jobs) {
                const auto &job = pair.second;
                JobStatus job_status = job->getStatus();
                // 只显示活动的作业
                if (job_status == JobStatus::RUNNING || job_status == JobStatus::STOPPED) {
                    DebugLog::logCommand("  [" + std::to_string(pair.first) + "] " + 
                        (job_status == JobStatus::RUNNING ? "Running" : "Stopped") + "\t" + job->getCommand());
                }
            }
            
            // 如果有作业在后台运行，并且是交互式shell，则提示用户
            if (shell_->isInteractive()) {
                DebugLog::logCommand("exit: 有后台作业在运行");
                return 1; // 返回错误状态，阻止退出
            }
        }

        // 检查是否有任何已完成但未清理的作业
        // 我们允许shell退出，但首先显示已完成的作业信息
        bool has_completed_jobs = false;
        for (const auto &pair : jobs) {
            const auto &job = pair.second;
            if (job->getStatus() == JobStatus::DONE) {
                has_completed_jobs = true;
                break;
            }
        }
        
        if (has_completed_jobs) {
            DebugLog::logCommand("以下作业已完成:");
            jc->showJobs(false, false, false, false);  // 显示所有已完成作业
        }

        // 如果没有活动作业，则可以安全退出
        DebugLog::logCommand("准备退出shell，状态码: " + std::to_string(status));
        
        // 请求退出shell
        shell_->exit(status);

        // 抛出一个异常，使得shell可以立即退出当前命令执行循环
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