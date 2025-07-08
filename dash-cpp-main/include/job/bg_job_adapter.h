/**
 * @file bg_job_adapter.h
 * @brief 后台任务控制适配器类
 */

#ifndef DASH_BG_JOB_ADAPTER_H
#define DASH_BG_JOB_ADAPTER_H

#include <string>
#include <vector>
#include <memory>
#include "job/bg_job_control.h"

namespace dash
{

    class Shell;
    class JobControl;

    /**
     * @brief 后台任务控制适配器类
     * 
     * 该类为C风格的后台任务控制API提供C++封装，以便与Shell类集成
     */
    class BGJobAdapter
    {
    private:
        Shell *shell_;
        JobControl *job_control_;
        bool is_initialized_;

    public:
        /**
         * @brief 构造函数
         * 
         * @param shell Shell对象指针
         * @param job_control JobControl对象指针
         */
        BGJobAdapter(Shell *shell, JobControl *job_control);

        /**
         * @brief 析构函数
         */
        ~BGJobAdapter();

        /**
         * @brief 初始化后台任务控制
         * 
         * @return true 初始化成功
         * @return false 初始化失败
         */
        bool initialize();

        /**
         * @brief 创建后台任务
         * 
         * @param command 命令字符串
         * @param nprocs 预期进程数量
         * @return 作业结构指针，如果失败则为NULL
         */
        struct job *createJob(const std::string &command, int nprocs);

        /**
         * @brief 在后台运行命令
         * 
         * @param jp 作业结构指针
         * @param cmd 命令字符串
         * @param argv 参数数组
         * @return int 成功返回0，失败则返回非零值
         */
        int runInBackground(struct job *jp, const std::string &cmd, char *const argv[]);

        /**
         * @brief 在前台运行命令
         * 
         * @param jp 作业结构指针
         * @param cmd 命令字符串
         * @param argv 参数数组
         * @return int 命令的退出状态
         */
        int runInForeground(struct job *jp, const std::string &cmd, char *const argv[]);

        /**
         * @brief 等待作业完成
         * 
         * @param jp 作业结构指针
         * @return int 退出状态
         */
        int waitForJob(struct job *jp);

        /**
         * @brief 实现fg命令
         * 
         * @param argc 参数数量
         * @param argv 参数数组
         * @return int 命令退出状态
         */
        int fgCommand(int argc, char **argv);

        /**
         * @brief 实现bg命令
         * 
         * @param argc 参数数量
         * @param argv 参数数组
         * @return int 命令退出状态
         */
        int bgCommand(int argc, char **argv);

        /**
         * @brief 实现jobs命令
         * 
         * @param show_running 是否显示运行中的作业
         * @param show_stopped 是否显示已停止的作业
         * @param show_pids 是否显示进程ID
         * @param show_changed 是否只显示状态已改变的作业
         */
        void showJobs(bool show_running = true, bool show_stopped = true, 
                     bool show_pids = false, bool show_changed = false);

        /**
         * @brief 获取作业状态
         * 
         * @param jp 作业结构指针
         * @return int 状态码
         */
        int getJobStatus(struct job *jp);

        /**
         * @brief 清理已完成的作业
         */
        void cleanupJobs();

        /**
         * @brief 检查是否有已停止的作业
         * 
         * @return true 有已停止的作业
         * @return false 没有已停止的作业
         */
        bool hasStoppedJobs();

        /**
         * @brief 使用作业号查找作业
         * 
         * @param jobno 作业号
         * @return struct job* 作业结构指针，如果未找到则为NULL
         */
        struct job *getJobByJobno(int jobno);

        /**
         * @brief 使用进程ID查找作业
         * 
         * @param pid 进程ID
         * @return struct job* 作业结构指针，如果未找到则为NULL
         */
        struct job *getJobByPid(pid_t pid);
    };

} // namespace dash

#endif // DASH_BG_JOB_ADAPTER_H 