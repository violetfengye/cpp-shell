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
static char *commandtext(void *node);
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
    } while (1);
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
        
        pgrp = getpid();
        setpgid(0, pgrp);
        tcsetpgrp(ttyfd, pgrp);
    } else {
        /* 关闭作业控制 */
        fd = ttyfd;
        pgrp = initialpgrp;
        tcsetpgrp(fd, pgrp);
        setpgid(0, pgrp);
        
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
close:
        if (ttyfd >= 0)
            close(ttyfd);
        ttyfd = -1;
    }
    
    jobctl = on;
}

/**
 * 前台/后台命令实现
 */
int
fgcmd_bg(int argc, char **argv)
{
    struct job *jp;
    struct output out;
    int mode;
    int retval;
    
    out.buf = NULL;
    out.bufsize = 0;
    out.bufpos = 0;
    out.fd = STDOUT_FILENO;

    mode = (**argv == 'f') ? FORK_FG : FORK_BG;
    
    /* 跳过命令名 */
    argv++;
    
    if (*argv == NULL) {
        /* 没有指定作业，使用当前作业 */
        if (!curjob) {
            fprintf(stderr, "当前没有作业\n");
            return 1;
        }
        jp = curjob;
    } else {
        /* 获取指定的作业 */
        jp = getjob(*argv, 1);
        if (!jp) {
            return 1;
        }
    }
    
    if (mode == FORK_BG) {
        set_curjob(jp, CUR_RUNNING);
        printf("[%d] ", jobno(jp));
    }
    
    printf("%s", jp->ps->cmd);
    showpipe(jp, &out);
    printf("\n");
    
#if JOBS
    retval = restartjob(jp, mode);
#else
    retval = 0;
#endif
    
    return retval;
}

/**
 * bg命令的别名
 */
int bgcmd_bg(int argc, char **argv)
{
    return fgcmd_bg(argc, argv);
}
#endif /* JOBS */

/**
 * 显示一个作业的状态
 */
static void
showjob(struct output *out, struct job *jp, int mode)
{
    char status[100];
    struct procstat *ps;
    int i;
    int st;
    
    /* 首先，显示作业编号和状态 */
    if (mode & SHOW_PGID) {
        /* 仅显示进程组ID */
        printf("%d\n", jp->ps->pid);
        return;
    }
    
    printf("[%d] ", jobno(jp));
    
    status[0] = '\0';
    st = getstatus(jp);
    if (jp->state == JOBSTOPPED) {
        sprintf(status, "已停止(%s)", strsignal(WSTOPSIG(st)));
    } else if (WIFEXITED(st)) {
        if (WEXITSTATUS(st))
            sprintf(status, "完成(%d) PID:%d", WEXITSTATUS(st), jp->ps->pid);
        else
            sprintf(status, "完成 PID:%d", jp->ps->pid);
    } else {
        /* 被信号终止 */
        sprintf(status, "%s", strsignal(WTERMSIG(st)));
        if (WCOREDUMP(st))
            strcat(status, " (核心已转储)");
    }
    
    if (status[0])
        printf("%-20s", status);
    
    /* 显示进程ID */
    if (mode & SHOW_PID) {
        ps = jp->ps;
        i = jp->nprocs;
        do {
            printf("%d ", ps->pid);
            ps++;
        } while (--i > 0);
    }
    
    /* 显示命令 */
    printf("%s", jp->ps->cmd);
    showpipe(jp, out);
    printf("\n");
    
    jp->changed = 0;
    jp->waited = 1;
    jp->notified = 1;
}

/**
 * 显示作业
 */
void
showjobs(struct output *out, int mode)
{
    struct job *jp;
    
    /* 首先，更新所有作业的状态 */
    dowait(DOWAIT_NONBLOCK, NULL);
    
    for (jp = curjob; jp; jp = jp->prev_job) {
        if (!(mode & SHOW_CHANGED) || jp->changed)
            showjob(out, jp, mode);
    }
}

/**
 * 标记作业结构为未使用
 */
void
freejob(struct job *jp)
{
    struct procstat *ps;
    int i;
    
    for (i = jp->nprocs, ps = jp->ps; --i >= 0; ps++) {
        if (ps->cmd != NULL)
            free(ps->cmd);
    }
    
    if (jp->ps != &jp->ps0)
        free(jp->ps);
    
    jp->used = 0;
    set_curjob(jp, CUR_DELETE);
}

/**
 * wait命令实现
 */
int
waitcmd_bg(int argc, char **argv)
{
    struct job *job;
    int retval;
    struct job *jp;
    
    /* 跳过命令名 */
    argv++;
    
    retval = 0;
    
    if (!*argv) {
        /* 等待所有作业 */
        for (;;) {
            jp = curjob;
            while (1) {
                if (!jp) {
                    /* 没有运行中的进程 */
                    goto out;
                }
                if (jp->state == JOBRUNNING)
                    break;
                jp->waited = 1;
                jp = jp->prev_job;
            }
            if (!dowait(DOWAIT_WAITCMD_ALL, 0))
                goto sigout;
        }
    }
    
    retval = 127;
    do {
        if (**argv != '%') {
            pid_t pid = atoi(*argv);
            job = curjob;
            goto start;
            do {
                if (job->ps[job->nprocs - 1].pid == pid)
                    break;
                job = job->prev_job;
start:
                if (!job)
                    goto repeat;
            } while (1);
        } else
            job = getjob(*argv, 0);
        
        /* 循环直到进程终止或停止 */
        if (!dowait(DOWAIT_WAITCMD, job))
            goto sigout;
        
        job->waited = 1;
        retval = getstatus(job);
repeat:
        ;
    } while (*++argv);
    
out:
    return retval;
    
sigout:
    return 128;  /* 信号 */
}

/**
 * 将作业名称转换为作业结构
 */
static struct job *
getjob(const char *arg, int getctl)
{
    struct job *jp;
    int i;
    
    if (arg[0] == '%') {
        if (arg[1] == '%' || arg[1] == '+') {
            /* 当前作业 */
            jp = curjob;
            if (!jp) {
                fprintf(stderr, "没有当前作业\n");
                return NULL;
            }
        } else if (arg[1] == '-') {
            /* 先前的作业 */
            jp = curjob;
            if (!jp)
                goto err;
            jp = jp->prev_job;
            if (!jp)
                goto err;
        } else if (isdigit((unsigned char)arg[1])) {
            /* 作业编号 */
            int jobno = atoi(arg + 1);
            if (jobno <= 0 || jobno > njobs) {
                goto err;
            }
            jp = jobtab + jobno - 1;
            if (!jp->used)
                goto err;
        } else {
            /* 通过命令前缀查找 */
            jp = curjob;
            for (i = strlen(arg + 1); jp; jp = jp->prev_job) {
                if (strncmp(jp->ps->cmd, arg + 1, i) == 0)
                    break;
            }
            if (!jp)
                goto err;
        }
    } else {
        /* 按PID查找 */
        pid_t pid = atoi(arg);
        jp = curjob;
        while (jp) {
            struct procstat *ps;
            int j;
            
            ps = jp->ps;
            for (j = jp->nprocs; --j >= 0; ps++) {
                if (ps->pid == pid)
                    goto found;
            }
            jp = jp->prev_job;
        }
        goto err;
    found:
        ;
    }
    
    if (getctl && jp->jobctl == 0)
        goto err;
    
    return jp;
    
err:
    fprintf(stderr, "没有此类作业: %s\n", arg);
    return NULL;
}

/**
 * 根据作业编号获取作业
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
 * 根据PID获取作业
 */
struct job *
getjob_by_pid(pid_t pid)
{
    struct job *jp = curjob;
    while (jp) {
        struct procstat *ps;
        int j;
        
        ps = jp->ps;
        for (j = jp->nprocs; --j >= 0; ps++) {
            if (ps->pid == pid)
                return jp;
        }
        jp = jp->prev_job;
    }
    
    return NULL;
}

/**
 * 获取作业的状态
 */
int
getstatus(struct job *jp)
{
    if (jp->state == JOBRUNNING)
        return -1;
    
    return jp->ps[jp->nprocs - 1].status;
}

/**
 * 增加作业表大小
 */
static struct job *
growjobtab(void)
{
    size_t len;
    ptrdiff_t offset;
    struct job *jp, *jq;
    
    len = njobs * sizeof(*jp);
    jq = jobtab;
    jp = realloc(jq, len + 4 * sizeof(*jp));
    
    if (jp == NULL) {
        fprintf(stderr, "内存分配失败\n");
        exit(1);
    }
    
    offset = (char *)jp - (char *)jq;
    if (offset) {
        /* 重新定位指针 */
        size_t l = len;
        
        jq = (struct job *)((char *)jq + l);
        while (l) {
            l -= sizeof(*jp);
            jq--;
#define joff(p) ((struct job *)((char *)(p) + l))
#define jmove(p) (p) = (void *)((char *)(p) + offset)
            if (joff(jp)->ps == &jq->ps0)
                jmove(joff(jp)->ps);
            if (joff(jp)->prev_job)
                jmove(joff(jp)->prev_job);
#undef joff
#undef jmove
        }
        if (curjob)
            curjob = (void *)((char *)curjob + offset);
    }
    
    njobs += 4;
    jobtab = jp;
    jp = (struct job *)((char *)jp + len);
    jq = jp + 3;
    do {
        jq->used = 0;
    } while (--jq >= jp);
    
    return jp;
}

/**
 * 创建作业
 */
struct job *
makejob(void *node, int nprocs)
{
    int i;
    struct job *jp;
    
    /* 查找可用的作业槽位 */
    for (i = 0; i < njobs; i++) {
        if (jobtab[i].used == 0)
            break;
    }
    
    if (i >= njobs) {
        jp = growjobtab();
    } else {
        jp = jobtab + i;
    }
    
    memset(jp, 0, sizeof(*jp));
    jp->ps = &jp->ps0;
    if (nprocs > 1) {
        jp->ps = calloc(nprocs, sizeof(struct procstat));
        if (!jp->ps) {
            fprintf(stderr, "内存分配失败\n");
            exit(1);
        }
    }
    
    jp->nprocs = 0;
    jp->used = 1;
    jp->changed = 1;
    jp->waited = 0;
    
    if (jobctl)
        jp->jobctl = 1;
    
    return jp;
}

/**
 * 为子进程设置环境
 */
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

/**
 * 在fork之后处理父进程
 */
pid_t
forkparent_bg(struct job *jp, int mode)
{
    /* 确保我们有控制终端 */
    if (ttyfd < 0) {
        ttyfd = open("/dev/tty", O_RDWR);
    }
    
    /* 在fork之前，保存当前的进程组ID */
    pid_t parent_pgid = getpgrp();
    
    /* 保存当前前台进程组 */
    pid_t fg_pgid = tcgetpgrp(ttyfd);
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        if (jp)
            freejob(jp);
        return -1;
    }
    
    if (pid == 0) {
        /* 子进程 */
        return 0;
    }
    
    /* 父进程 */
    
#if JOBS
    if (mode != FORK_NOJOB && jp->jobctl) {
        int pgrp;
        
        if (jp->nprocs == 0)
            pgrp = pid;
        else
            pgrp = jp->ps[0].pid;
        
        /* 这可能会失败，因为我们在子进程中也这样做 */
        if (setpgid(pid, pgrp) < 0 && errno != EACCES) {
            // 设置进程组失败，但继续尝试
        }
        
        /* 如果是前台命令，将进程组设置为前台 */
        if (mode == FORK_FG && fg_pgid >= 0) {
            tcsetpgrp(ttyfd, pgrp);
        }
    }
    else
#endif
    if (mode == FORK_BG) {
        /* 确保后台进程在独立的进程组中 */
        if (setpgid(pid, pid) < 0 && errno != EACCES) {
            // 设置进程组失败，但继续尝试
        }
        
        /* 确保父进程保留终端控制权 */
        if (fg_pgid >= 0 && fg_pgid == parent_pgid) {
            tcsetpgrp(ttyfd, parent_pgid);
        }
        
        backgndpid = pid;       /* 设置 $! */
        set_curjob(jp, CUR_RUNNING);
    }
    
    /* 更新作业状态 */
    if (jp) {
        struct procstat *ps = &jp->ps[jp->nprocs++];
        ps->pid = pid;
        ps->status = -1;
        ps->cmd = NULL;
    }
    
    return pid;
}

/**
 * 等待作业完成
 */
int
waitforjob(struct job *jp)
{
    int st;
    
    /* 更新作业状态 */
    dowait(jp ? DOWAIT_BLOCK : DOWAIT_NONBLOCK, jp);
    
    if (!jp)
        return 0;  /* 退出状态 */
    
    st = getstatus(jp);
    
#if JOBS
    if (jp->jobctl) {
        tcsetpgrp(ttyfd, getpid());
        
        /* 
         * 这确实很恶心。
         * 如果我们在做作业控制，那么我们做了一个TIOCSPGRP，使我们（shell）
         * 不再处于控制会话中——所以我们不会看到任何^C/SIGINT。
         * 因此，我们从子进程的退出状态中推测是否发生了SIGINT，
         * 如果发生了，就中断我们自己。呕！
         */
        if (jp->sigint)
            raise(SIGINT);
    }
#endif
    
    if (!jobctl || jp->state == JOBDONE)
        freejob(jp);
    
    return st;
}

/**
 * 等待进程终止
 */
static int
waitproc(int block, int *status)
{
    int flags = block == DOWAIT_BLOCK ? 0 : WNOHANG;
    int err;
    
#if JOBS
    if (jobctl)
        flags |= WUNTRACED;
#endif
    
    do {
        err = waitpid(-1, status, flags);
    } while (err < 0 && errno == EINTR);
    
    return err;
}

/**
 * 等待作业
 */
static int
dowait(int block, struct job *jp)
{
    int pid;
    int status;
    struct job *job;
    struct procstat *ps;
    int done;
    
    do {
        pid = waitproc(block, &status);
        if (pid <= 0)
            break;
        
        /* 更新进程状态 */
        job = NULL;
        for (job = curjob; job; job = job->prev_job) {
            done = 0;
            ps = job->ps;
            int nproc = job->nprocs;
            do {
                if (ps->pid == pid) {
                    ps->status = status;
                    done = 1;
                    break;
                }
                ps++;
            } while (--nproc);
            if (done)
                break;
        }
        
        if (!job)
            continue;  /* 不是我们的子进程 */
        
        /* 更新作业状态 */
#if JOBS
        if (WIFSTOPPED(status)) {
            job->stopstatus = status;
            job->state = JOBSTOPPED;
            job->sigint = 0;
        } else
#endif
        {
            /* 如果有信号，记录下来 */
            if (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT)
                job->sigint = 1;
            
            /* 检查该作业的所有进程是否已完成 */
            ps = job->ps;
            int nproc = job->nprocs;
            do {
                if (ps->pid == pid) {
                    ps->status = status;
                }
                if (ps->pid != -1 && !WIFEXITED(ps->status) && !WIFSIGNALED(ps->status))
                    break;
                ps++;
            } while (--nproc);
            
            if (!nproc) {
                /* 所有进程都已完成 */
                job->state = JOBDONE;
            }
        }
        
        job->changed = 1;
        
        if (jp && jp == job)
            return pid;  /* 找到了我们等待的作业 */
        
    } while (pid > 0 || (pid < 0 && errno == EINTR));
    
    return 0;  /* 没有找到匹配的作业 */
}

#if JOBS
/**
 * 重启作业
 */
static int
restartjob(struct job *jp, int mode)
{
    struct procstat *ps;
    int i;
    int status;
    pid_t pgid;
    
    if (jp->state == JOBDONE)
        return 0;
    
    jp->state = JOBRUNNING;
    pgid = jp->ps->pid;
    
    if (mode == FORK_FG)
        tcsetpgrp(ttyfd, pgid);
    
    /* 发送继续信号给进程组 */
    kill(-pgid, SIGCONT);
    
    ps = jp->ps;
    i = jp->nprocs;
    do {
        if (WIFSTOPPED(ps->status)) {
            ps->status = -1;
        }
    } while (ps++, --i);
    
    status = (mode == FORK_FG) ? waitforjob(jp) : 0;
    
    return status;
}
#endif

/**
 * 显示管道命令
 */
static void
showpipe(struct job *jp, struct output *out)
{
    struct procstat *ps;
    int i;
    
    ps = jp->ps;
    i = jp->nprocs;
    if (i > 1) {
        printf(" | ");
        ps++;
        i--;
        while (--i >= 0) {
            printf("%s%s", ps->cmd, i > 0 ? " | " : "");
            ps++;
        }
    }
}

/**
 * 检查是否有停止的作业，如果有则返回1，否则返回0
 */
int
stoppedjobs(void)
{
    struct job *jp;
    int retval;
    
    retval = 0;
    if (job_warning)
        goto out;
    
    jp = curjob;
    if (jp && jp->state == JOBSTOPPED) {
        printf("您有已停止的作业。\n");
        job_warning = 2;
        retval++;
    }
    
out:
    return retval;
} 