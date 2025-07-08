/**
 * @file bg_job_adapter.cpp
 * @brief 后台任务控制适配器类实现
 */

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include "job/bg_job_adapter.h"
#include "job/job_control.h"
#include "core/shell.h"

namespace dash
{

    BGJobAdapter::BGJobAdapter(Shell *shell, JobControl *job_control)
        : shell_(shell), job_control_(job_control), is_initialized_(false)
    {
    }

    BGJobAdapter::~BGJobAdapter()
    {
        // 析构函数不需要特别清理，因为C风格API的jobtab是静态的
    }

    bool BGJobAdapter::initialize()
    {
        if (is_initialized_)
            return true;

        // 初始化C风格后台任务控制系统
        bg_job_init();

        // 启用作业控制
        if (shell_->isInteractive()) {
            setjobctl(1); // 启用
        }

        is_initialized_ = true;
        return true;
    }

    struct job *BGJobAdapter::createJob(const std::string &command, int nprocs)
    {
        if (!is_initialized_) {
            if (!initialize()) {
                return NULL;
            }
        }

        // 创建一个新作业
        struct job *jp = makejob(NULL, nprocs);
        if (jp) {
            // 设置第一个进程的命令
            if (jp->ps) {
                jp->ps->cmd = strdup(command.c_str());
            }
        }
        
        return jp;
    }

    int BGJobAdapter::runInBackground(struct job *jp, const std::string &cmd, char *const argv[])
    {
        if (!jp) {
            return -1;
        }

        // 确保作业控制已初始化
        if (!is_initialized_ && !initialize()) {
            return -1;
        }

        // 获取终端信息
        int shell_terminal = STDIN_FILENO;
        pid_t shell_pgid = getpgid(0);
        
        // 确保Shell在前台
        if (tcgetpgrp(shell_terminal) != shell_pgid) {
            if (tcsetpgrp(shell_terminal, shell_pgid) < 0) {
                // 无法设置前台进程组，但继续尝试
            }
        }
        
        // 保存原始信号处理器
        struct sigaction old_sigint, old_sigquit, old_sigtstp, old_sigchld;
        sigaction(SIGINT, NULL, &old_sigint);
        sigaction(SIGQUIT, NULL, &old_sigquit);
        sigaction(SIGTSTP, NULL, &old_sigtstp);
        sigaction(SIGCHLD, NULL, &old_sigchld);
        
        // 临时忽略SIGCHLD信号，防止子进程退出影响主进程
        struct sigaction sa_ignore;
        memset(&sa_ignore, 0, sizeof(sa_ignore));
        sa_ignore.sa_handler = SIG_IGN;
        sigaction(SIGCHLD, &sa_ignore, NULL);
        
        // 创建用于获取第二个子进程PID的管道
        int pid_pipe[2];
        if (pipe(pid_pipe) < 0) {
            sigaction(SIGCHLD, &old_sigchld, NULL);
            return -1;
        }
        
        // 使用双重fork技术完全分离子进程，避免僵尸进程
        pid_t pid = fork();
        
        if (pid < 0) {
            // fork失败
            sigaction(SIGCHLD, &old_sigchld, NULL);
            close(pid_pipe[0]);
            close(pid_pipe[1]);
            return -1;
        } else if (pid == 0) {
            // 第一个子进程
            close(pid_pipe[0]); // 关闭读端
            
            // 设置新的进程组
            if (setpgid(0, 0) < 0) {
                // 设置进程组失败，但继续尝试
            }
            
            // 执行第二次fork
            pid_t pid2 = fork();
            
            if (pid2 < 0) {
                // 第二次fork失败
                close(pid_pipe[1]);
                _exit(1);
            } else if (pid2 == 0) {
                // 第二个子进程 - 这将是真正执行命令的进程
                
                // 关闭管道，因为我们不需要它
                close(pid_pipe[1]);
                
                // 调用forkchild_bg设置进程属性
                forkchild_bg(jp, FORK_BG);
                
                // 创建新会话，使进程成为新会话的领导者
                // 这会将进程与原来的控制终端完全分离
                pid_t new_sid = setsid();
                if (new_sid < 0) {
                    // setsid失败，但继续尝试
                }
                
                // 重定向标准输入到/dev/null
                int dev_null_fd = open("/dev/null", O_RDONLY);
                if (dev_null_fd >= 0) {
                    dup2(dev_null_fd, STDIN_FILENO);
                    close(dev_null_fd);
                }
                
                // 创建输出文件名（使用自己的PID）
                pid_t my_pid = getpid();
                std::string output_file = "output_" + std::to_string(my_pid) + ".txt";
                
                // 重定向标准输出和标准错误到文件
                int output_fd = open(output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (output_fd >= 0) {
                    dup2(output_fd, STDOUT_FILENO);
                    // 在实际应用中可能也需要重定向stderr
                    // dup2(output_fd, STDERR_FILENO);
                    close(output_fd);
                }
                
                // 执行命令
                execvp(argv[0], argv);
                
                // 如果执行到这里，说明execvp失败
                _exit(127);
            }
            
            // 第一个子进程将第二个子进程的PID通过管道发送给父进程
            if (write(pid_pipe[1], &pid2, sizeof(pid2)) != sizeof(pid2)) {
                // 写入失败，但继续执行
            }
            close(pid_pipe[1]);
            
            // 第一个子进程退出，让第二个子进程被init收养
            _exit(0);
        }
        
        // 父进程
        
        // 关闭写入端
        close(pid_pipe[1]);
        
        // 从管道读取第二子进程的PID
        pid_t child_pid = -1;
        ssize_t bytes_read = read(pid_pipe[0], &child_pid, sizeof(child_pid));
        close(pid_pipe[0]);
        
        int job_id = 1; // 默认作业ID
        
        if (bytes_read == sizeof(child_pid)) {
            // 更新作业信息，设置正确的PID
            if (jp->ps) {
                jp->ps->pid = child_pid;
            }
            
            // 如果使用C++作业控制系统，也更新它
            if (job_control_) {
                // 对于守护进程，PID就是它自己的进程组ID
                job_id = job_control_->addJob(cmd, child_pid);
                job_control_->addProcess(job_id, child_pid, cmd);
                job_control_->setCurrentJobId(job_id); // 设置为当前作业
            }
        }
        
        // 等待第一个子进程退出
        int status;
        waitpid(pid, &status, 0);
        
        // 恢复SIGCHLD的处理
        sigaction(SIGCHLD, &old_sigchld, NULL);
        
        // 打印后台作业信息
        if (child_pid > 0) {
            std::cout << "[" << job_id << "] " << child_pid << std::endl;
        } else {
            std::cout << "[" << job_id << "] ？" << std::endl;
        }
        
        // 确保所有输出都被刷新到终端
        std::cout.flush();
        
        return 0; // 返回0表示成功启动后台进程
    }

    int BGJobAdapter::runInForeground(struct job *jp, const std::string &cmd, char *const argv[])
    {
        if (!jp)
            return -1;

        pid_t pid = forkparent_bg(jp, FORK_FG);
        if (pid < 0) {
            return -1;
        } else if (pid == 0) {
            // 子进程
            forkchild_bg(jp, FORK_FG);
            
            // 执行命令
            execvp(argv[0], argv);
            
            // 执行失败
            perror(argv[0]);
            _exit(127);
        }

        // 父进程 - 等待前台作业完成
        return waitForJob(jp);
    }

    int BGJobAdapter::waitForJob(struct job *jp)
    {
        return waitforjob(jp);
    }

    int BGJobAdapter::fgCommand(int argc, char **argv)
    {
        return fgcmd_bg(argc, argv);
    }

    int BGJobAdapter::bgCommand(int argc, char **argv)
    {
        return bgcmd_bg(argc, argv);
    }

    void BGJobAdapter::showJobs(bool show_running, bool show_stopped, bool show_pids, bool show_changed)
    {
        struct output out;
        out.buf = NULL;
        out.bufsize = 0;
        out.bufpos = 0;
        out.fd = STDOUT_FILENO;
        
        int mode = 0;
        if (show_pids) mode |= SHOW_PID;
        if (show_changed) mode |= SHOW_CHANGED;
        
        ::showjobs(&out, mode);
    }

    int BGJobAdapter::getJobStatus(struct job *jp)
    {
        if (!jp)
            return -1;
            
        return getstatus(jp);
    }

    void BGJobAdapter::cleanupJobs()
    {
        // 简化版本，这里不依赖全局变量
        // 在真实情况下，我们应该遍历所有作业并清理已完成的作业
    }

    bool BGJobAdapter::hasStoppedJobs()
    {
        return stoppedjobs() > 0;
    }

    struct job *BGJobAdapter::getJobByJobno(int jobno)
    {
        return getjob_by_jobno(jobno);
    }

    struct job *BGJobAdapter::getJobByPid(pid_t pid)
    {
        return getjob_by_pid(pid);
    }

    void
    forkchild_bg(struct job *jp, int mode)
    {
    #if JOBS
        /* 仅在根shell中做作业控制 */
        if (mode != FORK_NOJOB && jp->jobctl) {
            pid_t pgrp;
            
            if (jp->nprocs == 0)
                pgrp = getpid();
            else
                pgrp = jp->ps[0].pid;
            
            /* 这可能会失败，因为我们在父进程中也这样做 */
            setpgid(0, pgrp);
            
            if (mode == FORK_FG) {
                tcsetpgrp(ttyfd, pgrp);
            }
            
            /* 设置信号处理程序 */
            signal(SIGTSTP, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);
        }
        else
    #endif
        if (mode == FORK_BG) {
            /* 确保后台进程在自己的进程组中 */
            setpgid(0, 0);
            
            /* 忽略各种终端信号 */
            signal(SIGINT, SIG_IGN);
            signal(SIGQUIT, SIG_IGN);
            signal(SIGTSTP, SIG_IGN);
            signal(SIGTTIN, SIG_IGN);
            signal(SIGTTOU, SIG_IGN);
            
            /* 将标准输入重定向到/dev/null以防止终端输入 */
            if (jp->nprocs == 0) {
                close(0);
                int fd = open("/dev/null", O_RDONLY, 0);
                if (fd != 0 && fd > 0) {
                    dup2(fd, 0);
                    close(fd);
                }
            }
        }
        
        /* 还原可能被覆盖的信号处理程序 */
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
    }

} // namespace dash 