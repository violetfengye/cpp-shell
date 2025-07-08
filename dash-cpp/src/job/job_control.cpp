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
                // waitpid错误，但进程可能仍在运行（如果是守护进程）
                // 使用kill(pid, 0)检查进程是否存在
                if (errno == ECHILD && kill(process->getPid(), 0) == 0)
                {
                    // 进程存在但不是子进程，可能是守护进程
                    continue;  // 保持运行状态
                }
                else
                {
                    // 进程确实不存在
                    process->setCompleted(true);
                    status_changed = true;
                }
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
        for (const auto &process : processes_)
        {
            if (!process->isCompleted() && !process->isStopped())
            {
                return false;
            }
        }

        return true;
    }

    // JobControl 实现

    JobControl::JobControl(Shell *shell)
        : shell_(shell), next_job_id_(1), enabled_(false), terminal_fd_(-1), shell_pgid_(-1), current_job_id_(-1)
    {
    }

    JobControl::~JobControl()
    {
        // 清理作业
        jobs_.clear();
    }

    void JobControl::initialize()
    {
        // 打开终端设备
        terminal_fd_ = open("/dev/tty", O_RDWR);
        if (terminal_fd_ < 0)
        {
            // 无法打开终端，禁用作业控制
            enabled_ = false;
            return;
        }

        // 获取 shell 的进程组 ID
        shell_pgid_ = getpgrp();

        // 将 shell 进程组放入前台
        if (tcsetpgrp(terminal_fd_, shell_pgid_) < 0)
        {
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
    }

    void JobControl::enableJobControl()
    {
        if (!enabled_)
        {
            initialize();
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
        jobs_[job_id] = std::make_unique<Job>(job_id, command, pgid, terminal_fd_);
        return job_id;
    }

    bool JobControl::addProcess(int job_id, pid_t pid, const std::string &command)
    {
        Job *job = findJob(job_id);
        if (!job)
        {
            return false;
        }

        job->addProcess(pid, command);
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

            if (pid > 0) {
                // 找到了一个状态改变的子进程
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
                        // 更新作业状态
                        job->updateStatus();
                        // 只有状态确实发生了变化，才设置为未通知
                        if (job->getStatus() == JobStatus::DONE || job->getStatus() == JobStatus::STOPPED) {
                            job->setNotified(false);
                        }
                        break;
                    }
                }
                
                // 如果找不到对应的进程，可能是被孤立的进程或其他程序的子进程
                if (!process_found && pid > 0) {
                    // 这里可以记录日志，但不需要特殊处理
                }
            } else {
                // waitpid返回0或负数，没有子进程状态变化或出错
                break;
            }
        } while (pid > 0);
        
        // 无论waitpid结果如何，都要检查所有作业中的进程状态
        // 这对于处理守护进程（双重fork后被init接管的进程）特别重要
        for (auto &pair : jobs_) {
            Job *job = pair.second.get();
            
            // 对于每个RUNNING状态的作业，检查其进程是否仍然存在
            if (job->getStatus() == JobStatus::RUNNING) {
                bool all_completed = true;
                
                for (const auto &process : job->getProcesses()) {
                    if (!process->isCompleted()) {
                        // 使用kill(pid, 0)检查进程是否存在
                        if (kill(process->getPid(), 0) == 0) {
                            // 进程存在
                            all_completed = false;
                        } else if (errno == ESRCH) {
                            // 进程不存在
                            process->setCompleted(true);
                        }
                    }
                }
                
                // 如果所有进程都已完成，更新作业状态
                if (all_completed && job->getProcesses().size() > 0) {
                    job->updateStatus();
                }
            }
        }

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
        // 更新所有作业的状态
        for (auto &pair : jobs_)
        {
            Job *job = pair.second.get();
            job->updateStatus();
        }

        // 显示作业状态
        for (auto &pair : jobs_)
        {
            Job *job = pair.second.get();

            // 如果只显示状态已更改的作业，则跳过已通知的作业
            if (changed_only && job->isNotified())
            {
                continue;
            }

            // 根据作业状态和显示选项决定是否显示
            if ((job->getStatus() == JobStatus::RUNNING && !show_running) ||
                (job->getStatus() == JobStatus::STOPPED && !show_stopped))
            {
                continue;
            }

            // 显示作业状态
            std::cout << "[" << job->getId() << "] ";

            // 如果是当前作业，添加+号
            if (job == findCurrentJob())
            {
                std::cout << "+ ";
            }
            else
            {
                std::cout << "  ";
            }

            // 显示进程ID(s)
            if (show_pids)
            {
                std::cout << "(";
                bool first = true;
                for (const auto &process : job->getProcesses())
                {
                    if (!first) std::cout << " ";
                    std::cout << process->getPid();
                    first = false;
                }
                std::cout << ") ";
            }

            // 显示作业状态
            switch (job->getStatus())
            {
            case JobStatus::RUNNING:
                std::cout << "运行中";
                break;
            case JobStatus::STOPPED:
                std::cout << "已停止";
                break;
            case JobStatus::DONE:
                // 显示"已完成"并添加PID信息
                if (!job->getProcesses().empty()) {
                    std::cout << "已完成 PID:" << job->getProcesses()[0]->getPid();
                } else {
                    std::cout << "已完成";
                }
                break;
            }

            // 显示命令
            std::cout << "\t" << job->getCommand() << std::endl;

            // 标记作业为已通知
            job->setNotified(true);
        }

        // 清理已完成的作业
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
        // 删除已完成的作业
        for (auto it = jobs_.begin(); it != jobs_.end();)
        {
            if (it->second->getStatus() == JobStatus::DONE && it->second->isNotified())
            {
                it = jobs_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    bool JobControl::hasActiveJobs() const
    {
        for (const auto &pair : jobs_)
        {
            const auto &job = pair.second;
            if (job->getStatus() == JobStatus::RUNNING || job->getStatus() == JobStatus::STOPPED)
            {
                return true;
            }
        }
        return false;
    }

    Job *JobControl::findCurrentJob() const
    {
        if (current_job_id_ == -1) {
            // 没有当前作业，尝试找到最新的作业
            int newest_job_id = -1;
            for (const auto &pair : jobs_) {
                if (newest_job_id == -1 || pair.first > newest_job_id) {
                    newest_job_id = pair.first;
                }
            }
            
            if (newest_job_id != -1) {
                // 找到最新作业
                return findJob(newest_job_id);
            }
            return nullptr;
        }
        
        return findJob(current_job_id_);
    }

} // namespace dash