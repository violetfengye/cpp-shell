/**
 * @file job_control.h
 * @brief 作业控制类定义
 */

#ifndef DASH_JOB_CONTROL_H
#define DASH_JOB_CONTROL_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <sys/types.h>

namespace dash
{

    // 前向声明
    class Shell;

    /**
     * @brief 作业状态
     */
    enum class JobStatus
    {
        RUNNING, // 运行中
        STOPPED, // 已停止
        DONE     // 已完成
    };

    /**
     * @brief 进程类
     */
    class Process
    {
    private:
        pid_t pid_;
        int status_;
        bool completed_;
        bool stopped_;
        std::string command_;

    public:
        /**
         * @brief 构造函数
         *
         * @param pid 进程 ID
         * @param command 命令字符串
         */
        Process(pid_t pid, const std::string &command);

        /**
         * @brief 获取进程 ID
         *
         * @return pid_t 进程 ID
         */
        pid_t getPid() const { return pid_; }

        /**
         * @brief 获取退出状态
         *
         * @return int 退出状态
         */
        int getStatus() const { return status_; }

        /**
         * @brief 设置退出状态
         *
         * @param status 退出状态
         */
        void setStatus(int status) { status_ = status; }

        /**
         * @brief 检查进程是否已完成
         *
         * @return true 已完成
         * @return false 未完成
         */
        bool isCompleted() const { return completed_; }

        /**
         * @brief 设置完成状态
         *
         * @param completed 是否已完成
         */
        void setCompleted(bool completed) { completed_ = completed; }

        /**
         * @brief 检查进程是否已停止
         *
         * @return true 已停止
         * @return false 未停止
         */
        bool isStopped() const { return stopped_; }

        /**
         * @brief 设置停止状态
         *
         * @param stopped 是否已停止
         */
        void setStopped(bool stopped) { stopped_ = stopped; }

        /**
         * @brief 获取命令字符串
         *
         * @return const std::string& 命令字符串
         */
        const std::string &getCommand() const { return command_; }
    };

    /**
     * @brief 作业类
     */
    class Job
    {
    private:
        int id_;
        std::vector<std::unique_ptr<Process>> processes_;
        pid_t pgid_;
        bool notified_;
        int terminal_fd_;
        JobStatus status_;
        std::string command_;

    public:
        /**
         * @brief 构造函数
         *
         * @param id 作业 ID
         * @param command 命令字符串
         * @param pgid 进程组 ID
         * @param terminal_fd 终端文件描述符
         */
        Job(int id, const std::string &command, pid_t pgid, int terminal_fd);

        /**
         * @brief 析构函数
         */
        ~Job();

        /**
         * @brief 获取作业 ID
         *
         * @return int 作业 ID
         */
        int getId() const { return id_; }

        /**
         * @brief 获取进程组 ID
         *
         * @return pid_t 进程组 ID
         */
        pid_t getPgid() const { return pgid_; }

        /**
         * @brief 获取终端文件描述符
         *
         * @return int 终端文件描述符
         */
        int getTerminalFd() const { return terminal_fd_; }

        /**
         * @brief 添加进程
         *
         * @param pid 进程 ID
         * @param command 命令字符串
         */
        void addProcess(pid_t pid, const std::string &command);

        /**
         * @brief 获取所有进程
         *
         * @return const std::vector<std::unique_ptr<Process>>& 进程列表
         */
        const std::vector<std::unique_ptr<Process>> &getProcesses() const { return processes_; }

        /**
         * @brief 获取命令字符串
         *
         * @return const std::string& 命令字符串
         */
        const std::string &getCommand() const { return command_; }

        /**
         * @brief 检查是否已通知状态变化
         *
         * @return true 已通知
         * @return false 未通知
         */
        bool isNotified() const { return notified_; }

        /**
         * @brief 设置通知状态
         *
         * @param notified 是否已通知
         */
        void setNotified(bool notified) { notified_ = notified; }

        /**
         * @brief 获取作业状态
         *
         * @return JobStatus 作业状态
         */
        JobStatus getStatus() const { return status_; }

        /**
         * @brief 更新作业状态
         *
         * @return true 状态已更改
         * @return false 状态未更改
         */
        bool updateStatus();

        /**
         * @brief 将作业放入前台
         *
         * @param cont 是否继续已停止的作业
         * @return int 最后一个进程的退出状态
         */
        int putInForeground(bool cont);

        /**
         * @brief 将作业放入后台
         *
         * @param cont 是否继续已停止的作业
         */
        void putInBackground(bool cont);

        /**
         * @brief 检查作业是否已完成
         *
         * @return true 已完成
         * @return false 未完成
         */
        bool isCompleted() const;

        /**
         * @brief 检查作业是否已停止
         *
         * @return true 已停止
         * @return false 未停止
         */
        bool isStopped() const;
    };

    /**
     * @brief 作业控制类
     */
    class JobControl
    {
    private:
        Shell *shell_;
        std::unordered_map<int, std::unique_ptr<Job>> jobs_;
        int next_job_id_;
        bool enabled_;
        int terminal_fd_;
        pid_t shell_pgid_;
        int current_job_id_; // 当前作业ID

        /**
         * @brief 初始化作业控制
         */
        void initialize();

        /**
         * @brief 查找作业
         *
         * @param id 作业 ID
         * @return Job* 作业指针，如果找不到则返回 nullptr
         */
        Job *findJob(int id) const;

        /**
         * @brief 查找当前作业
         * 
         * @return Job* 当前作业指针，如果没有则返回 nullptr
         */
        Job *findCurrentJob() const;

    public:
        /**
         * @brief 构造函数
         *
         * @param shell Shell 对象指针
         */
        explicit JobControl(Shell *shell);

        /**
         * @brief 析构函数
         */
        ~JobControl();

        /**
         * @brief 启用作业控制
         */
        void enableJobControl();

        /**
         * @brief 检查作业控制是否已启用
         *
         * @return true 已启用
         * @return false 未启用
         */
        bool isEnabled() const { return enabled_; }

        /**
         * @brief 添加作业
         *
         * @param command 命令字符串
         * @param pgid 进程组 ID
         * @return int 作业 ID
         */
        int addJob(const std::string &command, pid_t pgid);

        /**
         * @brief 添加进程到作业
         *
         * @param job_id 作业 ID
         * @param pid 进程 ID
         * @param command 命令字符串
         * @return true 添加成功
         * @return false 添加失败
         */
        bool addProcess(int job_id, pid_t pid, const std::string &command);

        /**
         * @brief 更新作业状态
         *
         * @param wait_for_pid 是否等待特定进程（0 表示等待任何子进程）
         */
        void updateStatus(pid_t wait_for_pid);

        /**
         * @brief 等待作业
         *
         * @param job_id 作业 ID
         * @return int 作业的退出状态
         */
        int waitForJob(int job_id);

        /**
         * @brief 将作业放入前台
         *
         * @param job_id 作业 ID
         * @param cont 是否继续已停止的作业
         * @return int 作业的退出状态
         */
        int putJobInForeground(int job_id, bool cont);

        /**
         * @brief 将作业放入后台
         *
         * @param job_id 作业 ID
         * @param cont 是否继续已停止的作业
         */
        void putJobInBackground(int job_id, bool cont);

        /**
         * @brief 显示作业状态
         *
         * @param changed_only 是否只显示状态已更改的作业
         * @param show_running 是否显示运行中的作业
         * @param show_stopped 是否显示已停止的作业
         * @param show_pids 是否显示进程ID
         */
        void showJobs(bool changed_only, bool show_running = true, bool show_stopped = true, bool show_pids = false);

        /**
         * @brief 检查是否有已停止的作业
         *
         * @return true 有已停止的作业
         * @return false 没有已停止的作业
         */
        bool hasStoppedJobs() const;

        /**
         * @brief 清理已完成的作业
         */
        void cleanupJobs();

        /**
         * @brief 获取所有作业
         * 
         * @return const std::unordered_map<int, std::unique_ptr<Job>>& 所有作业的引用
         */
        const std::unordered_map<int, std::unique_ptr<Job>>& getJobs() const { return jobs_; }

        /**
         * @brief 获取终端文件描述符
         *
         * @return int 终端文件描述符
         */
        int getTerminalFd() const { return terminal_fd_; }

        /**
         * @brief 获取 shell 进程组 ID
         *
         * @return pid_t shell 进程组 ID
         */
        pid_t getShellPgid() const { return shell_pgid_; }

        /**
         * @brief 检查是否有活动作业
         *
         * @return true 有活动作业
         * @return false 没有活动作业
         */
        bool hasActiveJobs() const;

        /**
         * @brief 获取当前作业ID
         * 
         * @return int 当前作业ID，如果没有则返回-1
         */
        int getCurrentJobId() const { return current_job_id_; }
        
        /**
         * @brief 设置当前作业ID
         * 
         * @param job_id 要设置为当前作业的ID
         */
        void setCurrentJobId(int job_id) { current_job_id_ = job_id; }
    };

} // namespace dash

#endif // DASH_JOB_CONTROL_H