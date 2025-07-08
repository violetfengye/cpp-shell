/**
 * @file bg_job_adapter.cpp
 * @brief 后台任务控制适配器类实现
 */

#include "job/bg_job_adapter.h"
#include "core/shell.h"
#include "job/job_control.h"
#include <iostream>
#include <cstring>

namespace dash
{

    BGJobAdapter::BGJobAdapter(Shell *shell, JobControl *job_control)
        : shell_(shell), job_control_(job_control), is_initialized_(false)
    {
    }

    BGJobAdapter::~BGJobAdapter()
    {
        // 在这里不需要额外清理，底层API会自行处理资源
    }

    bool BGJobAdapter::initialize()
    {
        if (is_initialized_)
        {
            return true;
        }

        // 初始化底层后台任务控制API
        bg_job_init();
        is_initialized_ = true;

        // 如果Shell处于交互式模式，启用作业控制
        if (shell_->isInteractive())
        {
            setjobctl(1);
        }

        return is_initialized_;
    }

    struct job *BGJobAdapter::createJob(const std::string &command, int nprocs)
    {
        if (!is_initialized_)
        {
            if (!initialize())
            {
                std::cerr << "后台任务控制系统初始化失败" << std::endl;
                return nullptr;
            }
        }

        // 为了传递命令文本，我们需要一个能表示Node的结构
        // 实际上我们只需要commandtext函数能够获取命令字符串
        // 这里使用一个临时指针，实际上bg_job_control不会使用它的内容
        void *dummy_node = nullptr;

        // 创建作业
        struct job *jp = makejob(dummy_node, nprocs);
        if (jp == nullptr)
        {
            std::cerr << "创建作业失败" << std::endl;
            return nullptr;
        }

        // 设置命令
        if (jp->ps0.cmd != nullptr)
        {
            free(jp->ps0.cmd);
        }
        jp->ps0.cmd = strdup(command.c_str());

        return jp;
    }

    int BGJobAdapter::runInBackground(struct job *jp, const std::string &cmd, char *const argv[])
    {
        if (!is_initialized_)
        {
            if (!initialize())
            {
                std::cerr << "后台任务控制系统初始化失败" << std::endl;
                return -1;
            }
        }

        // 复制命令到作业结构
        if (jp->ps0.cmd != nullptr)
        {
            free(jp->ps0.cmd);
        }
        jp->ps0.cmd = strdup(cmd.c_str());

        // 在后台模式下派生子进程
        pid_t pid = forkparent_bg(jp, FORK_BG);
        if (pid < 0)
        {
            std::cerr << "创建子进程失败" << std::endl;
            return -1;
        }
        else if (pid == 0)
        {
            // 子进程，准备执行命令
            forkchild_bg(jp, FORK_BG);

            // 执行命令
            execvp(argv[0], argv);

            // 如果execvp失败
            std::cerr << "执行命令失败: " << strerror(errno) << std::endl;
            _exit(127);
        }

        return 0; // 成功
    }

    int BGJobAdapter::runInForeground(struct job *jp, const std::string &cmd, char *const argv[])
    {
        if (!is_initialized_)
        {
            if (!initialize())
            {
                std::cerr << "后台任务控制系统初始化失败" << std::endl;
                return -1;
            }
        }

        // 复制命令到作业结构
        if (jp->ps0.cmd != nullptr)
        {
            free(jp->ps0.cmd);
        }
        jp->ps0.cmd = strdup(cmd.c_str());

        // 在前台模式下派生子进程
        pid_t pid = forkparent_bg(jp, FORK_FG);
        if (pid < 0)
        {
            std::cerr << "创建子进程失败" << std::endl;
            return -1;
        }
        else if (pid == 0)
        {
            // 子进程，准备执行命令
            forkchild_bg(jp, FORK_FG);

            // 执行命令
            execvp(argv[0], argv);

            // 如果execvp失败
            std::cerr << "执行命令失败: " << strerror(errno) << std::endl;
            _exit(127);
        }

        // 等待前台作业完成
        return waitforjob(jp);
    }

    int BGJobAdapter::waitForJob(struct job *jp)
    {
        if (!is_initialized_)
        {
            std::cerr << "后台任务控制系统未初始化" << std::endl;
            return -1;
        }

        return waitforjob(jp);
    }

    int BGJobAdapter::fgCommand(int argc, char **argv)
    {
        if (!is_initialized_)
        {
            std::cerr << "后台任务控制系统未初始化" << std::endl;
            return -1;
        }

        return fgcmd_bg(argc, argv);
    }

    int BGJobAdapter::bgCommand(int argc, char **argv)
    {
        if (!is_initialized_)
        {
            std::cerr << "后台任务控制系统未初始化" << std::endl;
            return -1;
        }

        return bgcmd_bg(argc, argv);
    }

    void BGJobAdapter::showJobs(bool show_running, bool show_stopped, bool show_pids, bool show_changed)
    {
        if (!is_initialized_)
        {
            std::cerr << "后台任务控制系统未初始化" << std::endl;
            return;
        }

        // 准备输出结构
        struct output out;
        memset(&out, 0, sizeof(struct output));

        // 设置模式标志
        int mode = 0;
        if (show_pids)
        {
            mode |= SHOW_PID;
        }
        if (show_changed)
        {
            mode |= SHOW_CHANGED;
        }

        // 显示作业
        showjobs(&out, mode);

        // 处理输出
        if (out.buf != nullptr)
        {
            std::cout << out.buf;
            free(out.buf);
        }
    }

    int BGJobAdapter::getJobStatus(struct job *jp)
    {
        if (!is_initialized_ || jp == nullptr)
        {
            return -1;
        }

        return getstatus(jp);
    }

    void BGJobAdapter::cleanupJobs()
    {
        if (!is_initialized_)
        {
            return;
        }

        // 清理已完成的作业
        // 底层API没有提供直接的清理函数，作业状态会在waitforjob中更新
    }

    bool BGJobAdapter::hasStoppedJobs()
    {
        if (!is_initialized_)
        {
            return false;
        }

        return stoppedjobs() > 0;
    }

    struct job *BGJobAdapter::getJobByJobno(int jobno)
    {
        if (!is_initialized_)
        {
            return nullptr;
        }

        return getjob_by_jobno(jobno);
    }

    struct job *BGJobAdapter::getJobByPid(pid_t pid)
    {
        if (!is_initialized_)
        {
            return nullptr;
        }

        return getjob_by_pid(pid);
    }

} // namespace dash 