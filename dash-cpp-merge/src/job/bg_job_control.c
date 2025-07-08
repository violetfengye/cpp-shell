/**
 * @file bg_job_control.c
 * @brief 后台任务控制实现 (C风格)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stddef.h>
#include "job/bg_job_control.h"

#ifndef JOBS
#define JOBS 1
#endif

#ifndef WCOREDUMP
#define WCOREDUMP(x) ((x) & 0x80)
#endif

/* 全局变量 */
pid_t backgndpid = 0;           /* 最后一个后台进程的pid */
int job_warning = 0;            /* 用户是否已被警告有停止的作业 */
int jobctl = 0;                 /* 是否启用作业控制 */

/* 设置为CUR_DELETE, CUR_RUNNING, CUR_STOPPED的模式标志 */
#define CUR_DELETE  2
#define CUR_RUNNING 1
#define CUR_STOPPED 0

/* dowait的模式标志 */
#define DOWAIT_NONBLOCK 0
#define DOWAIT_BLOCK 1
#define DOWAIT_WAITCMD 2
#define DOWAIT_WAITCMD_ALL 4

/* 作业数组 */
static struct job *jobtab = NULL;
/* 数组大小 */
static unsigned njobs = 0;

/* shell在调用时的进程组 */
static int initialpgrp;
/* 控制终端 */
static int ttyfd = -1;

/* 当前作业 */
static struct job *curjob;

/* 静态函数声明 */
static struct job *getjob(const char *arg, int getctl);
static struct job *growjobtab(void);
static int dowait(int block, struct job *jp);
static int waitproc(int block, int *status);
static void showpipe(struct job *jp, struct output *out);
static void showjob(struct output *out, struct job *jp, int mode);

#if JOBS
static int restartjob(struct job *jp, int mode);
static void xtcsetpgrp(int fd, pid_t pgrp);
#endif

/**
 * 初始化作业控制子系统
 */
void bg_job_init(void) {
    /* 初始化作业控制数组 */
    njobs = 4;
    jobtab = calloc(njobs, sizeof(struct job));
    if (jobtab == NULL) {
        fprintf(stderr, "内存分配失败\n");
        exit(1);
    }
    
    /* 标记所有作业为未使用 */
    for (unsigned i = 0; i < njobs; i++) {
        jobtab[i].used = 0;
    }
    
    /* 初始化全局变量 */
    backgndpid = 0;
    job_warning = 0;
    jobctl = 0;
    
    /* 获取控制终端 */
    if (ttyfd < 0) {
        ttyfd = open("/dev/tty", O_RDWR);
        /* 保存当前进程组 */
        if (ttyfd >= 0) {
            initialpgrp = getpgrp();
        }
    }
}

/**
 * 设置作业的当前状态
 */
static void
set_curjob(struct job *jp, unsigned mode)
{
    struct job *jp1;
    struct job **jpp, **curp;

    /* 首先从列表中删除 */
    jpp = curp = &curjob;
    do {
        jp1 = *jpp;
        if (jp1 == jp)
            break;
        jpp = &jp1->prev_job;
    } while (jp1);
    
    if (jp1)
        *jpp = jp1->prev_job;

    /* 然后重新以正确位置插入 */
    jpp = curp;
    switch (mode) {
    default:
        abort();
    case CUR_DELETE:
        /* 作业被删除 */
        break;
    case CUR_RUNNING:
        /* 新创建的作业或后台作业，放在所有停止的作业之后 */
        do {
            jp1 = *jpp;
            if (!jobctl || !jp1 || jp1->state != JOBSTOPPED)
                break;
            jpp = &jp1->prev_job;
        } while (1);
        /* FALLTHROUGH */
    case CUR_STOPPED:
        /* 新停止的作业 - 成为当前作业 */
        jp->prev_job = *jpp;
        *jpp = jp;
        break;
    }
}

/**
 * 获取作业编号
 */
int
jobno(const struct job *jp)
{
    return jp - jobtab + 1;
}

#if JOBS
/**
 * 开启和关闭作业控制
 */
void
setjobctl(int on)
{
    int fd;
    int pgrp;

    if (on == jobctl || !on)
        return;
    
    if (on) {
        int ofd;
        ofd = fd = open("/dev/tty", O_RDWR);
        if (fd < 0) {
            fd += 3;
            while (!isatty(fd))
                if (--fd < 0)
                    goto out;
        }
        /* 保存文件描述符 */
        if (fd > 2) {
            ttyfd = fd;
        } else {
            ttyfd = dup(fd);
            close(fd);
        }
        
        do { /* 当我们在后台时 */
            if ((pgrp = tcgetpgrp(ttyfd)) < 0) {
out:
                fprintf(stderr, "无法访问tty; 作业控制已关闭\n");
                on = 0;
                goto close;
            }
            if (pgrp == getpgrp())
                break;
            kill(0, SIGTTIN);
        } while (1);
        initialpgrp = pgrp;

        /* 设置信号处理程序 */
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        
        /* 设置进程组ID */
        if (setpgid(0, 0) < 0)
            perror("setpgid");
        
        if (tcsetpgrp(ttyfd, getpgrp()) < 0)
            perror("tcsetpgrp");
        
        on = 1;
    } else {
        /* 关闭作业控制 */
close:
        if (ttyfd >= 0) {
            close(ttyfd);
            ttyfd = -1;
        }
    }
    jobctl = on;
}

/**
 * 将进程组设为前台进程组
 */
static void
xtcsetpgrp(int fd, pid_t pgrp)
{
    if (tcsetpgrp(fd, pgrp))
        perror("tcsetpgrp");
}
#endif

/**
 * 执行fg命令
 */
int
fgcmd_bg(int argc, char **argv)
{
    struct job *jp;
    struct output out;
    int i;
    int status;
    
    /* 初始化输出结构 */
    out.fd = 2; /* stderr */
    out.buf = NULL;
    out.bufsize = 0;
    out.bufpos = 0;
    
    nextopt("");
    
    if (!jobctl) {
        error("作业控制未启用");
        return 1;
    }
    
    /* fg命令不带参数，默认使用当前作业 */
    if (argc <= 1) {
        if (curjob == NULL) {
            error("没有当前作业");
            return 1;
        }
        jp = curjob;
    } else {
        /* 否则，获取指定的作业 */
        jp = getjob(argv[1], 1);
        if (jp == NULL)
            return 1;
    }
    
    /* 显示将被前台执行的命令 */
    showjob(&out, jp, 0);
    if (out.buf) {
        out.buf[out.bufpos] = '\0';
        puts(out.buf);
        free(out.buf);
    }
    
    /* 重启作业并等待其完成 */
    return restartjob(jp, FORK_FG);
}

/**
 * 执行bg命令
 */
int
bgcmd_bg(int argc, char **argv)
{
    struct job *jp;
    struct output out;
    int i;
    int status;
    
    /* 初始化输出结构 */
    out.fd = 2; /* stderr */
    out.buf = NULL;
    out.bufsize = 0;
    out.bufpos = 0;
    
    nextopt("");
    
    if (!jobctl) {
        error("作业控制未启用");
        return 1;
    }
    
    /* bg命令不带参数，默认使用当前作业 */
    if (argc <= 1) {
        if (curjob == NULL) {
            error("没有当前作业");
            return 1;
        }
        jp = curjob;
    } else {
        /* 否则，获取指定的作业 */
        jp = getjob(argv[1], 1);
        if (jp == NULL)
            return 1;
    }
    
    /* 如果作业不是停止的，报错 */
    if (jp->state != JOBSTOPPED) {
        error("作业未停止");
        return 1;
    }
    
    /* 显示将被后台执行的命令 */
    showjob(&out, jp, 0);
    if (out.buf) {
        out.buf[out.bufpos] = '\0';
        printf("%s &\n", out.buf);
        free(out.buf);
    }
    
    /* 在后台重启作业 */
    return restartjob(jp, FORK_BG);
}

/**
 * 显示单个作业信息
 */
static void
showjob(struct output *out, struct job *jp, int mode)
{
    int jobno;
    pid_t pid;
    int status;
    struct procstat *ps;
    int i;
    
    status = getstatus(jp);
    jobno = jobno(jp);
    
    /* 只显示进程组ID */
    if (mode & SHOW_PGID) {
        outfmt(out, "%d\n", jp->ps0.pid);
        return;
    }
    
    /* 显示作业号和状态 */
    outfmt(out, "[%d]  ", jobno);
    
    /* 标记作业 */
    if (jp == curjob)
        outfmt(out, "+ ");
    else {
        struct job *jp2;
        int j = 0;
        
        for (jp2 = curjob; jp2 != NULL; jp2 = jp2->prev_job)
            j++;
        
        if (j > 1 && jp == curjob->prev_job)
            outfmt(out, "- ");
        else
            outfmt(out, "  ");
    }
    
    /* 显示作业状态 */
    if (jp->state == JOBSTOPPED)
        outfmt(out, "已停止    ");
    else if (jp->state == JOBDONE)
        outfmt(out, "已完成");
    else
        outfmt(out, "运行中    ");
    
    /* 显示作业命令 */
    if (jp->nprocs > 0)
        showpipe(jp, out);
    else
        outfmt(out, "%s", jp->ps0.cmd);
    
    jp->changed = 0;
    
    /* 标记已通知 */
    if (jp->state == JOBDONE)
        jp->notified = 1;
    
    /* 显示退出状态 */
    if ((mode & SHOW_PID) && jp->nprocs > 0) {
        outfmt(out, " ");
        for (i = 0; i < jp->nprocs; i++) {
            ps = jp->ps + i;
            if (i > 0)
                outfmt(out, " | ");
            outfmt(out, "%d", ps->pid);
        }
    }
    
    outc('\n', out);
}

/**
 * 显示作业列表
 */
void
showjobs(struct output *out, int mode)
{
    int jobno;
    struct job *jp;
    
    /* 遍历所有作业并显示符合条件的 */
    for (jobno = 1, jp = jobtab; jobno <= njobs; jobno++, jp++) {
        if (!jp->used)
            continue;
        if ((mode & SHOW_CHANGED) && !jp->changed)
            continue;
        showjob(out, jp, mode);
    }
    
    out->bufpos = 0;
}

/**
 * 释放作业资源
 */
void
freejob(struct job *jp)
{
    struct procstat *ps;
    int i;
    
    /* 从当前作业列表中移除 */
    set_curjob(jp, CUR_DELETE);
    
    /* 释放命令字符串 */
    if (jp->ps0.cmd) {
        free(jp->ps0.cmd);
        jp->ps0.cmd = NULL;
    }
    
    /* 释放其他进程状态 */
    if (jp->nprocs > 0 && jp->ps) {
        for (i = 0; i < jp->nprocs; i++) {
            ps = &jp->ps[i];
            if (ps->cmd) {
                free(ps->cmd);
                ps->cmd = NULL;
            }
        }
        free(jp->ps);
        jp->ps = NULL;
    }
    
    /* 标记为未使用 */
    jp->used = 0;
    jp->changed = 0;
}

/**
 * 执行wait命令
 */
int
waitcmd_bg(int argc, char **argv)
{
    struct job *job;
    int status;
    struct job *jp;
    int retval = 0;
    
    /* 如果没有指定参数，等待所有作业 */
    if (argc <= 1) {
        /* 等待所有后台作业 */
        while ((jp = curjob) != NULL) {
            if (jp->state == JOBDONE)
                continue;
            status = dowait(DOWAIT_BLOCK, NULL);
            if (status < 0 && errno == ECHILD)
                break;
        }
    } else {
        /* 等待指定的作业 */
        for (argc--, argv++; argc > 0; argc--, argv++) {
            job = getjob(*argv, 1);
            if (!job) {
                retval = 127;
                continue;
            }
            /* 如果作业已完成，继续 */
            if (job->state == JOBDONE)
                continue;
            
            /* 等待作业完成 */
            status = dowait(DOWAIT_BLOCK, job);
            if (status < 0) {
                retval = 127;
                break;
            }
            
            /* 获取退出状态 */
            status = getstatus(job);
            if (WIFEXITED(status))
                retval = WEXITSTATUS(status);
            else if (WIFSIGNALED(status)) {
                retval = 128 + WTERMSIG(status);
            } else
                retval = 127;
        }
    }
    
    return retval;
}

/**
 * 根据参数获取作业
 */
static struct job *
getjob(const char *arg, int getctl)
{
    struct job *jp;
    int jobno;
    pid_t pid;
    
    if (!arg)
        return NULL;
    
    /* 如果参数以%开头，按作业号解析 */
    if (*arg == '%') {
        arg++;
        /* 解析作业号 */
        if (!*arg || *arg == '+' || *arg == '%') {
            /* 当前作业 */
            jp = curjob;
            if (!jp) {
                if (getctl)
                    error("没有当前作业");
                return NULL;
            }
            return jp;
        } else if (*arg == '-') {
            /* 前一个作业 */
            jp = curjob;
            if (!jp || !jp->prev_job) {
                if (getctl)
                    error("没有前一个作业");
                return NULL;
            }
            return jp->prev_job;
        }
        
        /* 按编号获取作业 */
        jobno = strtol(arg, NULL, 10);
        if (jobno <= 0 || jobno > njobs) {
            if (getctl)
                error("作业号超出范围");
            return NULL;
        }
        
        jp = &jobtab[jobno - 1];
        if (!jp->used) {
            if (getctl)
                error("作业 %d 不存在", jobno);
            return NULL;
        }
        return jp;
    }
    
    /* 否则，按PID解析 */
    pid = (pid_t)strtol(arg, NULL, 10);
    if (pid <= 0) {
        if (getctl)
            error("无效的进程ID");
        return NULL;
    }
    
    /* 遍历所有作业，查找匹配的PID */
    for (jp = jobtab, jobno = 1; jobno <= njobs; jobno++, jp++) {
        struct procstat *ps;
        int i;
        
        if (!jp->used)
            continue;
        
        /* 检查主进程 */
        if (jp->ps0.pid == pid)
            return jp;
        
        /* 检查管道中的其他进程 */
        for (i = 0; i < jp->nprocs; i++) {
            ps = &jp->ps[i];
            if (ps->pid == pid)
                return jp;
        }
    }
    
    if (getctl)
        error("进程ID %d 不属于任何作业", (int)pid);
    return NULL;
}

/**
 * 按作业号获取作业
 */
struct job *
getjob_by_jobno(int jobno)
{
    if (jobno <= 0 || jobno > njobs)
        return NULL;
    
    struct job *jp = jobtab + jobno - 1;
    if (!jp->used)
        return NULL;
    
    return jp;
}

/**
 * 按PID获取作业
 */
struct job *
getjob_by_pid(pid_t pid)
{
    struct job *jp = curjob;
    int i;
    
    while (jp) {
        struct procstat *ps;
        
        /* 检查主进程 */
        if (jp->ps0.pid == pid)
            return jp;
        
        /* 检查管道中的其他进程 */
        for (i = 0; i < jp->nprocs; i++) {
            ps = &jp->ps[i];
            if (ps->pid == pid)
                return jp;
        }
        
        jp = jp->prev_job;
    }
    
    return NULL;
}

/**
 * 获取作业状态
 */
int
getstatus(struct job *jp)
{
    return jp->ps0.status;
}

/**
 * 增长作业表大小
 */
static struct job *
growjobtab(void)
{
    size_t len;
    struct job *jp, *jq;
    
    /* 如果作业表已满，扩展它 */
    if (njobs >= INT_MAX / 2)
        return NULL;
    
    len = sizeof(struct job) * njobs;
    jq = realloc(jobtab, len * 2);
    if (jq == NULL)
        return NULL;
    
    /* 更新作业表指针 */
    jp = (struct job *)((char *)jq + len);
    memset(jp, '\0', len);
    
    /* 调整前一个作业指针 */
    if (curjob)
        curjob = (struct job *)((char *)curjob + ((char *)jq - (char *)jobtab));
    
    jobtab = jq;
    njobs *= 2;
    return jobtab + njobs / 2;
}

/**
 * 创建新作业
 */
struct job *
makejob(void *node, int nprocs)
{
    int i;
    struct job *jp;
    
    /* 查找一个空闲的作业槽位 */
    for (i = njobs, jp = jobtab; --i >= 0; jp++) {
        if (jp->used == 0)
            break;
    }
    
    /* 如果没有空闲槽位，扩展作业表 */
    if (i < 0) {
        jp = growjobtab();
        if (jp == NULL) {
            error("作业表已满");
            return NULL;
        }
    }
    
    memset(jp, 0, sizeof(*jp));
    jp->used = 1;
    jp->changed = 1;
    
    /* 如果需要，分配进程状态数组 */
    if (nprocs > 1) {
        jp->ps = calloc(nprocs, sizeof(struct procstat));
        if (!jp->ps) {
            error("内存分配失败");
            return NULL;
        }
    }
    
    /* 将作业添加到当前作业列表 */
    set_curjob(jp, CUR_RUNNING);
    
    return jp;
}

/**
 * 在子进程中执行的操作
 */
void
forkchild_bg(struct job *jp, int mode)
{
    /* 重置信号处理程序 */
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
    
    /* 如果不使用作业控制，直接返回 */
    if (mode == FORK_NOJOB)
        return;
    
    /* 设置进程组ID */
    if (jp->ps0.pid != 0 && setpgid(0, jp->ps0.pid) == 0)
        mode = FORK_NOJOB;
    
#if JOBS
    if (mode == FORK_FG) {
        /* 前台模式，设置为前台进程组 */
        if (jobctl && ttyfd >= 0)
            xtcsetpgrp(ttyfd, getpgrp());
    }
#endif
}

/**
 * 在父进程中执行的fork操作
 */
pid_t
forkparent_bg(struct job *jp, int mode)
{
    pid_t pid;
    int status;
    
    /* 设置作业控制标志 */
    jp->jobctl = jobctl;
    
    /* 创建子进程 */
    switch ((pid = fork())) {
    case -1:
        /* fork失败 */
        error("fork失败: %s", strerror(errno));
        return -1;
        
    case 0:
        /* 子进程，永远不会返回 */
        forkchild_bg(jp, mode);
        return 0;
        
    default:
        /* 父进程 */
        /* 保存进程ID */
        jp->ps0.pid = pid;
        
        /* 设置进程组ID */
        if (mode != FORK_NOJOB && setpgid(pid, pid) < 0) {
            if (errno == EACCES) {
                /* 子进程已经执行，无法设置其进程组 */
                /* 这是正常的，可以忽略 */
            } else {
                error("setpgid 失败: %s", strerror(errno));
            }
        }
        
        /* 记录后台进程ID */
        if (mode == FORK_BG)
            backgndpid = pid;
        
        /* 更新作业状态 */
        struct procstat *ps = &jp->ps0;
        ps->pid = pid;
        ps->status = -1;
        ps->cmd = NULL; /* 会在调用者中设置 */
        
        /* 返回子进程ID */
        return pid;
    }
}

/**
 * 等待作业完成
 */
int
waitforjob(struct job *jp)
{
    int status;
    int st;
    
    /* 只有在作业控制启用时才会等待 */
    if (!jp->jobctl || jp->state == JOBDONE)
        return getstatus(jp);
    
    /* 等待作业结束 */
    status = dowait(DOWAIT_BLOCK, jp);
    if (status < 0)
        return status;
    
    /* 获取退出状态 */
    st = getstatus(jp);
    if (WIFSIGNALED(st)) {
        if (WTERMSIG(st) != SIGINT && WTERMSIG(st) != SIGPIPE) {
            outfmt(stderr, "%s\n",
                   strsignal(WTERMSIG(st)));
        }
    }
    
    return st;
}

/**
 * 等待进程状态变化
 */
static int
waitproc(int block, int *status)
{
    int flags = 0;
    int err;
    int pid;
    
    /* 设置等待标志 */
    if (!block)
        flags |= WNOHANG;
    if (jobctl)
        flags |= WUNTRACED;
    
    /* 等待任何子进程 */
    do {
        err = errno;
        pid = waitpid(-1, status, flags);
        if (pid <= 0) {
            if (pid < 0 && errno == EINTR)
                continue;
            return pid;
        }
    } while (0);
    
    return pid;
}

/**
 * 等待作业状态变化
 */
static int
dowait(int block, struct job *jp)
{
    int pid;
    int status;
    struct job *job;
    struct procstat *ps;
    int done;
    int stopped;
    
    do {
        /* 等待任何子进程状态变化 */
        pid = waitproc(block, &status);
        if (pid <= 0)
            return pid;
        
        /* 查找此PID对应的作业 */
        for (job = jobtab; job < jobtab + njobs; job++) {
            if (!job->used)
                continue;
            
            /* 检查主进程 */
            if (job->ps0.pid == pid) {
                job->ps0.status = status;
                break;
            }
            
            /* 检查管道中的其他进程 */
            for (ps = job->ps; ps < job->ps + job->nprocs; ps++) {
                if (ps->pid == pid) {
                    ps->status = status;
                    break;
                }
            }
            if (ps < job->ps + job->nprocs)
                break;
        }
        
        if (job >= jobtab + njobs)
            continue; /* 未知进程，继续等待 */
        
        /* 标记作业状态变化 */
        job->changed = 1;
        
        /* 检查作业是否完成或停止 */
        done = 1;
        stopped = 1;
        
        /* 检查主进程 */
        ps = &job->ps0;
        if (ps->pid) {
            if (WIFSTOPPED(ps->status)) {
                done = 0;
            } else {
                stopped = 0;
                if (WIFCONTINUED(ps->status))
                    done = 0;
            }
        }
        
        /* 检查管道中的其他进程 */
        for (ps = job->ps; ps < job->ps + job->nprocs; ps++) {
            if (ps->pid) {
                if (WIFSTOPPED(ps->status)) {
                    done = 0;
                } else {
                    stopped = 0;
                    if (WIFCONTINUED(ps->status))
                        done = 0;
                }
            }
        }
        
        /* 更新作业状态 */
        if (done) {
            job->state = JOBDONE;
            if (job->nprocs > 0)
                job->ps0.status = job->ps[job->nprocs - 1].status;
        } else if (stopped) {
            job->state = JOBSTOPPED;
            job->stopstatus = status;
        }
        
        /* 如果是我们正在等待的作业，返回 */
        if (jp && jp == job)
            return pid;
        
    } while (jp);
    
    /* 没有找到指定的作业 */
    return pid;
}

/**
 * 重新启动停止的作业
 */
static int
restartjob(struct job *jp, int mode)
{
    struct procstat *ps;
    int i;
    int status;
    
    /* 只处理停止的作业 */
    if (jp->state != JOBSTOPPED)
        return JOBDONE;
    
    /* 发送SIGCONT信号 */
    if (killpg(jp->ps0.pid, SIGCONT) < 0) {
        if (errno == ESRCH) {
            /* 进程组不存在，标记为完成 */
            jp->state = JOBDONE;
            return JOBDONE;
        }
        error("killpg 失败: %s", strerror(errno));
    }
    
    /* 更新作业状态 */
    jp->state = JOBRUNNING;
    
    /* 前台作业需要设置为前台进程组 */
    if (mode == FORK_FG) {
        if (jobctl && ttyfd >= 0)
            xtcsetpgrp(ttyfd, jp->ps0.pid);
        
        /* 等待作业完成 */
        return waitforjob(jp);
    }
    
    return 0;
}

/**
 * 显示管道命令
 */
static void
showpipe(struct job *jp, struct output *out)
{
    int i;
    struct procstat *ps;
    
    /* 显示主进程 */
    outfmt(out, "%s", jp->ps0.cmd);
    
    /* 显示管道中的其他进程 */
    for (i = 0; i < jp->nprocs; i++) {
        ps = &jp->ps[i];
        outfmt(out, " | %s", ps->cmd);
    }
    
    /* 在后台作业后添加 & 符号 */
    if (jp->state == JOBRUNNING)
        outfmt(out, " &");
}

/**
 * 检查是否有停止的作业
 */
int
stoppedjobs(void)
{
    int jobno;
    struct job *jp;
    int count = 0;
    
    /* 遍历所有作业，计算已停止的作业数 */
    for (jobno = 1, jp = jobtab; jobno <= njobs; jobno++, jp++) {
        if (jp->used && jp->state == JOBSTOPPED)
            count++;
    }
    
    return count;
}

/**
 * 将字符添加到输出缓冲区
 */
static void
outc(char c, struct output *out)
{
    if (out->buf == NULL) {
        out->buf = malloc(BUFSIZ);
        if (out->buf == NULL)
            return;
        out->bufsize = BUFSIZ;
        out->bufpos = 0;
    } else if (out->bufpos >= out->bufsize) {
        char *newbuf;
        
        newbuf = realloc(out->buf, out->bufsize * 2);
        if (newbuf == NULL)
            return;
        out->buf = newbuf;
        out->bufsize *= 2;
    }
    
    out->buf[out->bufpos++] = c;
}

/**
 * 格式化输出到缓冲区
 */
static void
outfmt(struct output *out, const char *format, ...)
{
    va_list ap;
    char *p;
    char c;
    
    va_start(ap, format);
    
    while ((c = *format++) != '\0') {
        if (c != '%') {
            outc(c, out);
            continue;
        }
        
        c = *format++;
        switch (c) {
        case 'd':
            {
                int num = va_arg(ap, int);
                char buf[32];
                sprintf(buf, "%d", num);
                for (p = buf; *p; p++)
                    outc(*p, out);
            }
            break;
        case 's':
            p = va_arg(ap, char *);
            if (p == NULL)
                p = "(null)";
            while (*p)
                outc(*p++, out);
            break;
        default:
            outc(c, out);
            break;
        }
    }
    
    va_end(ap);
}

/**
 * 打印错误信息
 */
static void
error(const char *msg, ...)
{
    va_list ap;
    fprintf(stderr, "Error: ");
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

/**
 * 处理命令行选项
 */
static int
nextopt(const char *optstring)
{
    /* 这只是一个占位符，实际上我们不使用这个函数 */
    return 0;
} 