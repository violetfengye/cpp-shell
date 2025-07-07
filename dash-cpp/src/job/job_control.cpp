/**
 * @file job_control.cpp
 * @brief 作业控制类实现
 */

#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <fcntl.h>
#include <mutex>
#include <cstring>
#include <map>
#include <ctime>
#include <unordered_set>
#include "job/job_control.h"
#include "core/shell.h"
#include "utils/error.h"

namespace dash
{

    // Process 实现

    Process::Process(pid_t pid, const std::string &command)
        : pid_(pid), status_(0), completed_(false), stopped_(false), command_(command)
    {
    }

    // Job 实现

    Job::Job(int id, const std::string &command, pid_t pgid, int terminal_fd)
        : id_(id), pgid_(pgid), notified_(false), terminal_fd_(terminal_fd),
          status_(JobStatus::RUNNING), command_(command)
    {
    }

    Job::~Job()
    {
        // 清理进程列表
        processes_.clear();
    }

    void Job::addProcess(pid_t pid, const std::string &command)
    {
        processes_.push_back(std::make_unique<Process>(pid, command));
    }

    bool Job::updateStatus()
    {
        bool status_changed = false;

        // 检查每个进程的状态
        for (auto &process : processes_)
        {
            if (process->isCompleted())
            {
                continue;
            }

            int status;
            pid_t result = waitpid(process->getPid(), &status, WUNTRACED | WNOHANG);

            if (result == 0)
            {
                // 进程仍在运行
                continue;
            }
            else if (result < 0)
            {
                // 错误
                perror("waitpid");
                process->setCompleted(true);
                status_changed = true;
            }
            else if (result == process->getPid())
            {
                // 进程状态已更改
                process->setStatus(status);

                if (WIFSTOPPED(status))
                {
                    process->setStopped(true);
                    status_changed = true;
                }
                else
                {
                    process->setCompleted(true);
                    status_changed = true;
                }
            }
        }

        // 更新作业状态
        if (isCompleted())
        {
            status_ = JobStatus::DONE;
            status_changed = true;
        }
        else if (isStopped())
        {
            status_ = JobStatus::STOPPED;
            status_changed = true;
        }
        else
        {
            status_ = JobStatus::RUNNING;
        }

        return status_changed;
    }

    int Job::putInForeground(bool cont)
    {
        // 将作业放入前台
        if (terminal_fd_ >= 0)
        {
            // 将作业的进程组设置为前台进程组
            if (tcsetpgrp(terminal_fd_, pgid_) < 0)
            {
                perror("tcsetpgrp");
            }
        }

        // 如果需要，继续已停止的作业
        if (cont && status_ == JobStatus::STOPPED)
        {
            if (kill(-pgid_, SIGCONT) < 0)
            {
                perror("kill (SIGCONT)");
            }

            // 更新进程状态
            for (auto &process : processes_)
            {
                process->setStopped(false);
            }

            status_ = JobStatus::RUNNING;
        }

        // 等待作业完成或停止
        int status = 0;
        bool wait_for_job = true;

        while (wait_for_job)
        {
            // 等待任何子进程状态变化
            pid_t child = waitpid(-pgid_, &status, WUNTRACED);

            if (child < 0)
            {
                // 错误
                perror("waitpid");
                break;
            }
            else if (child == 0)
            {
                // 没有子进程状态变化
                break;
            }
            else
            {
                // 更新进程状态
                for (auto &process : processes_)
                {
                    if (process->getPid() == child)
                    {
                        process->setStatus(status);

                        if (WIFSTOPPED(status))
                        {
                            process->setStopped(true);
                            wait_for_job = false;
                        }
                        else
                        {
                            process->setCompleted(true);
                        }

                        break;
                    }
                }

                // 检查作业是否完成或停止
                if (isCompleted() || isStopped())
                {
                    wait_for_job = false;
                }
            }
        }

        // 更新作业状态
        if (isCompleted())
        {
            status_ = JobStatus::DONE;
        }
        else if (isStopped())
        {
            status_ = JobStatus::STOPPED;
        }

        // 将 shell 放回前台
        if (terminal_fd_ >= 0)
        {
            if (tcsetpgrp(terminal_fd_, getpgrp()) < 0)
            {
                perror("tcsetpgrp");
            }
        }

        // 返回最后一个进程的状态
        return status;
    }

    void Job::putInBackground(bool cont)
    {
        // 如果需要，继续已停止的作业
        if (cont && status_ == JobStatus::STOPPED)
        {
            if (kill(-pgid_, SIGCONT) < 0)
            {
                perror("kill (SIGCONT)");
            }

            // 更新进程状态
            for (auto &process : processes_)
            {
                process->setStopped(false);
            }

            status_ = JobStatus::RUNNING;
        }
    }

    bool Job::isCompleted() const
    {
        for (const auto &process : processes_)
        {
            if (!process->isCompleted())
            {
                return false;
            }
        }

        return true;
    }

    bool Job::isStopped() const
    {
        // 首先检查是否所有进程都已完成
        bool all_completed = true;
        
        for (const auto &process : processes_)
        {
            if (!process->isCompleted())
            {
                all_completed = false;
                break;
            }
        }
        
        // 如果所有进程已完成，那么作业不是stopped状态
        if (all_completed)
        {
            return false;
        }
        
        // 如果有任何未完成且未停止的进程，则作业不是stopped状态
        for (const auto &process : processes_)
        {
            if (!process->isCompleted() && !process->isStopped())
            {
                return false;
            }
        }
        
        // 所有未完成的进程都处于停止状态
        return true;
    }

    // JobControl 实现

    JobControl::JobControl(Shell *shell)
        : shell_(shell), next_job_id_(1), enabled_(false), terminal_fd_(-1), shell_pgid_(-1)
    {
    }

    JobControl::~JobControl()
    {
        // 清理作业
        jobs_.clear();
    }

    void JobControl::initialize()
    {
        std::cout << "JobControl初始化开始..." << std::endl;
        
        // 确保作业列表为空
        if (!jobs_.empty()) {
            std::cout << "警告: 作业列表非空，清除旧作业" << std::endl;
            jobs_.clear();
        }
        
        next_job_id_ = 1; // 确保作业ID从1开始
        
        // 打开终端设备
        terminal_fd_ = open("/dev/tty", O_RDWR);
        if (terminal_fd_ < 0)
        {
            std::cerr << "无法打开终端设备: " << strerror(errno) << std::endl;
            // 无法打开终端，禁用作业控制
            enabled_ = false;
            return;
        }

        // 获取 shell 的进程组 ID
        shell_pgid_ = getpgrp();
        std::cout << "Shell进程组ID: " << shell_pgid_ << std::endl;

        // 将 shell 进程组放入前台
        if (tcsetpgrp(terminal_fd_, shell_pgid_) < 0)
        {
            std::cerr << "tcsetpgrp失败: " << strerror(errno) << std::endl;
            perror("tcsetpgrp");
            close(terminal_fd_);
            enabled_ = false;
            return;
        }

        // 忽略作业控制信号
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);

        enabled_ = true;
        std::cout << "JobControl初始化完成，enabled_=" << enabled_ << std::endl;
    }

    void JobControl::enableJobControl()
    {
        // 添加调试输出
        std::cout << "启用作业控制，当前状态: " << (enabled_ ? "已启用" : "未启用") << std::endl;
        
        if (!enabled_)
        {
            std::cout << "初始化作业控制系统" << std::endl;
            initialize();
            std::cout << "作业控制初始化完成，状态: " << (enabled_ ? "成功" : "失败") << std::endl;
        }
    }

    Job *JobControl::findJob(int id) const
    {
        auto it = jobs_.find(id);
        if (it != jobs_.end())
        {
            return it->second.get();
        }

        return nullptr;
    }

    int JobControl::addJob(const std::string &command, pid_t pgid)
    {
        int job_id = next_job_id_++;
        
        // 检查作业控制是否启用
        if (!enabled_) {
            std::cout << "警告: 作业控制未启用，尝试启用" << std::endl;
            enableJobControl();
            if (!enabled_) {
                std::cerr << "错误: 无法启用作业控制，无法添加作业!" << std::endl;
                return -1;
            }
        }
        
        // 使用安全的方式创建作业
        try {
            auto new_job = std::make_unique<Job>(job_id, command, pgid, terminal_fd_);
            jobs_.emplace(job_id, std::move(new_job));
            
            std::cout << "添加作业 #" << job_id << ", 进程组ID: " << pgid 
                     << ", 命令: " << command << std::endl;
            std::cout << "当前作业数量: " << jobs_.size() << std::endl;
            
            // 验证作业是否被正确添加
            if (jobs_.find(job_id) == jobs_.end()) {
                std::cerr << "错误: 作业添加失败，无法在容器中找到!" << std::endl;
            } else {
                std::cout << "成功: 作业已添加到容器中" << std::endl;
            }
            
            return job_id;
        }
        catch (const std::exception& e) {
            std::cerr << "错误: 添加作业时发生异常: " << e.what() << std::endl;
            return -1;
        }
    }

    bool JobControl::addProcess(int job_id, pid_t pid, const std::string &command)
    {
        Job *job = findJob(job_id);
        if (!job)
        {
            std::cout << "错误: 无法找到作业 #" << job_id << " 来添加进程 " << pid << std::endl;
            return false;
        }

        job->addProcess(pid, command);
        std::cout << "向作业 #" << job_id << " 添加进程, PID: " << pid << ", 命令: " << command << std::endl;
        return true;
    }

    // 添加一个静态变量来防止递归调用
    static bool is_updating = false;
    
    // 静态互斥锁，保护作业状态更新
    static std::mutex update_mutex;

    void JobControl::updateStatus(pid_t wait_for_pid)
    {
        // 使用互斥锁防止多线程或信号处理器并发访问
        std::lock_guard<std::mutex> lock(update_mutex);
        
        // 防止递归调用
        if (is_updating)
            return;

        is_updating = true;
        
        // 记录哪些作业的状态发生了变化
        std::unordered_set<int> updated_jobs;

        // 使用非阻塞等待检查子进程状态
        int status;
        pid_t pid;

        // 连续尝试等待任何已完成的子进程
        do {
            if (wait_for_pid > 0) {
                pid = waitpid(wait_for_pid, &status, WUNTRACED | WNOHANG);
            } else {
                pid = waitpid(-1, &status, WUNTRACED | WNOHANG);
            }

            if (pid <= 0) {
                break;  // 没有子进程需要处理
            }

            // 更新对应进程的状态
            bool process_found = false;
            for (auto &pair : jobs_) {
                Job *job = pair.second.get();
                bool job_updated = false;

                for (const auto &process : job->getProcesses()) {
                    if (process->getPid() == pid) {
                        process_found = true;
                        // 更新进程状态
                        if (WIFSTOPPED(status)) {
                            process->setStopped(true);
                            process->setStatus(status);
                        } else {
                            process->setCompleted(true);
                            process->setStatus(status);
                        }
                        job_updated = true;
                        break;
                    }
                }

                if (job_updated) {
                    // 更新作业状态，并记录状态是否改变
                    JobStatus old_status = job->getStatus();
                    bool status_changed = job->updateStatus();
                    JobStatus new_status = job->getStatus();
                    
                    // 只有状态确实发生了变化，才设置为未通知
                    if (status_changed) {
                        if (new_status == JobStatus::DONE || new_status == JobStatus::STOPPED) {
                            job->setNotified(false);
                        }
                        
                        // 如果作业从运行状态变为完成状态，特别处理
                        if (old_status == JobStatus::RUNNING && new_status == JobStatus::DONE) {
                            // 作业已完成，但我们不马上清理它
                            // 让cleanupJobs方法处理清理逻辑
                        }
                    }
                    break;
                }
            }
            
            // 如果找不到对应的进程，可能是被孤立的进程或其他程序的子进程
            if (!process_found && pid > 0) {
                // 这里可以记录日志，但不需要特殊处理
            }
        } while (pid > 0);

        is_updating = false;
    }

    int JobControl::waitForJob(int job_id)
    {
        Job *job = findJob(job_id);
        if (!job)
        {
            return -1;
        }

        // 等待作业完成或停止
        int status = 0;

        while (!job->isCompleted() && !job->isStopped())
        {
            // 更新作业状态
            job->updateStatus();
        }

        // 获取最后一个进程的状态
        if (!job->getProcesses().empty())
        {
            status = job->getProcesses().back()->getStatus();
        }

        return status;
    }

    int JobControl::putJobInForeground(int job_id, bool cont)
    {
        Job *job = findJob(job_id);
        if (!job)
        {
            return -1;
        }

        // 将作业放入前台
        return job->putInForeground(cont);
    }

    void JobControl::putJobInBackground(int job_id, bool cont)
    {
        Job *job = findJob(job_id);
        if (!job)
        {
            return;
        }

        // 将作业放入后台
        job->putInBackground(cont);
    }

    void JobControl::showJobs(bool changed_only, bool show_running, bool show_stopped, bool show_pids)
    {
        // 确保状态最新 - 强制更新每个作业
        for (auto &pair : jobs_)
        {
            Job *job = pair.second.get();
            job->updateStatus();
            
            // 强制已完成作业设置为已通知
            if (job->getStatus() == JobStatus::DONE) {
                job->setNotified(true);
            }
        }

        // 立即尝试清理已完成的作业
        cleanupJobs();
        
        // 重新计数有效作业数量
        int running_count = 0, stopped_count = 0, done_count = 0;
        for (const auto &pair : jobs_)
        {
            switch (pair.second->getStatus()) {
                case JobStatus::RUNNING: running_count++; break;
                case JobStatus::STOPPED: stopped_count++; break;
                case JobStatus::DONE: done_count++; break;
            }
        }
        
        // 只有在有作业时才显示状态
        if (!jobs_.empty()) {
            std::cout << "作业状态: 运行中=" << running_count 
                    << ", 已停止=" << stopped_count 
                    << ", 已完成=" << done_count 
                    << ", 总数=" << jobs_.size() << std::endl;
        }

        // 显示活动作业状态
        bool jobs_found = false;
        for (auto &pair : jobs_)
        {
            Job *job = pair.second.get();
            
            // 直接跳过已完成的作业 - 强制立即清理策略
            if (job->getStatus() == JobStatus::DONE)
            {
                continue;
            }
            
            // 根据作业状态和显示选项决定是否显示
            if ((job->getStatus() == JobStatus::RUNNING && !show_running) ||
                (job->getStatus() == JobStatus::STOPPED && !show_stopped))
            {
                continue;
            }
            
            jobs_found = true;

            // 显示作业状态
            std::cout << "[" << job->getId() << "] ";

            // 显示作业状态
            switch (job->getStatus())
            {
            case JobStatus::RUNNING:
                std::cout << "Running";
                break;
            case JobStatus::STOPPED:
                std::cout << "Stopped";
                break;
            case JobStatus::DONE:
                std::cout << "Done";
                break;
            }

            // 如果需要显示进程ID
            if (show_pids)
            {
                std::cout << " (";
                bool first = true;
                for (const auto &process : job->getProcesses())
                {
                    if (!first)
                    {
                        std::cout << " ";
                    }
                    std::cout << process->getPid();
                    first = false;
                }
                std::cout << ")";
            }

            std::cout << "\t" << job->getCommand() << std::endl;
        }
        
        // 再次尝试清理已完成的作业
        cleanupJobs();
    }

    bool JobControl::hasStoppedJobs() const
    {
        for (const auto &pair : jobs_)
        {
            if (pair.second->getStatus() == JobStatus::STOPPED)
            {
                return true;
            }
        }

        return false;
    }

    void JobControl::cleanupJobs()
    {
        // 删除已完成的作业 - 强制清理机制
        int cleaned = 0;
        
        // 先强制更新所有作业状态
        for (auto& pair : jobs_) {
            pair.second->updateStatus();
        }
        
        // 强制设置所有已完成作业的通知标志
        for (auto& pair : jobs_) {
            if (pair.second->getStatus() == JobStatus::DONE) {
                pair.second->setNotified(true);
            }
        }
        
        // 清理所有已完成的作业，无论是否已通知
        for (auto it = jobs_.begin(); it != jobs_.end();) {
            int job_id = it->first;
            const auto& job = it->second;
            
            // 立即清理所有已完成的作业
            if (job->getStatus() == JobStatus::DONE) {
                std::cout << "强制清理作业 #" << job_id << " (" << job->getCommand() << ")" << std::endl;
                it = jobs_.erase(it);
                cleaned++;
                continue;
            }
            
            ++it;
        }
        
        // 简化输出
        if (cleaned > 0) {
            std::cout << "已清理 " << cleaned << " 个已完成的作业" << std::endl;
        }
        
        // 只在仍有作业时才打印详细信息
        if (jobs_.size() > 0) {
            std::cout << "剩余 " << jobs_.size() << " 个作业:" << std::endl;
            for (const auto& pair : jobs_) {
                const auto& job = pair.second;
                std::string status_str;
                switch (job->getStatus()) {
                    case JobStatus::RUNNING: status_str = "运行中"; break;
                    case JobStatus::STOPPED: status_str = "已停止"; break; 
                    case JobStatus::DONE: status_str = "已完成 (错误!)"; break;
                }
                std::cout << "  #" << pair.first << " " << status_str 
                          << " : " << job->getCommand() << std::endl;
            }
        }
    }

    bool JobControl::hasActiveJobs() const
    {
        // 首先检查是否有任何作业
        if (jobs_.empty()) {
            return false;
        }
        
        // 检查每个作业的状态
        for (const auto &pair : jobs_)
        {
            const auto &job = pair.second;
            JobStatus status = job->getStatus();
            
            if (status == JobStatus::RUNNING || status == JobStatus::STOPPED)
            {
                return true;
            }
        }
        
        return false;
    }

} // namespace dash