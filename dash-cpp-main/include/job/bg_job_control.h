/**
 * @file bg_job_control.h
 * @brief 后台任务控制API (C风格实现)
 */

#ifndef DASH_BG_JOB_CONTROL_H
#define DASH_BG_JOB_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <stdbool.h>

/* 后台任务控制标志 */
#define FORK_FG 0
#define FORK_BG 1
#define FORK_NOJOB 2

/* 作业显示模式标志 */
#define SHOW_PGID     0x01    /* 只显示进程组ID - 用于 jobs -p */
#define SHOW_PID      0x04    /* 包含进程ID */
#define SHOW_CHANGED  0x08    /* 只显示状态已改变的作业 */

/* 作业状态定义 */
#define JOBRUNNING   0       /* 至少一个进程正在运行 */
#define JOBSTOPPED   1       /* 所有进程已停止 */
#define JOBDONE      2       /* 所有进程已完成 */

/* 最大进程数量 */
#define MAX_PROCS_PER_JOB 16

/* 进程状态结构 */
struct procstat {
    pid_t   pid;            /* 进程ID */
    int     status;         /* 上一次等待状态 */
    char    *cmd;           /* 命令文本 */
};

/* 作业结构 */
struct job {
    struct procstat ps0;    /* 第一个进程的状态 */
    struct procstat *ps;    /* 当有多个进程时的状态数组 */
    int stopstatus;         /* 已停止作业的状态 */
    unsigned short nprocs;  /* 进程数量 */
    unsigned char state;    /* 作业状态 (JOBRUNNING, JOBSTOPPED, JOBDONE) */
    unsigned char sigint;   /* 作业是否被SIGINT终止 */
    unsigned char jobctl;   /* 作业是否使用作业控制 */
    unsigned char waited;   /* 是否已等待此作业 */
    unsigned char used;     /* 此条目是否已使用 */
    unsigned char changed;  /* 状态是否已改变 */
    unsigned char notified; /* 是否已通知用户状态变化 */
    struct job *prev_job;   /* 前一个作业 */
};

/* 用于输出的结构 */
struct output {
    char *buf;              /* 输出缓冲区 */
    int bufsize;            /* 缓冲区大小 */
    int bufpos;             /* 当前位置 */
    int fd;                 /* 文件描述符 */
};

/* 全局变量 */
extern pid_t backgndpid;    /* 最后一个后台进程的pid */
extern int job_warning;     /* 用户是否已被警告有停止的作业 */
extern int jobctl;          /* 是否启用作业控制 */

/* API 函数 */

/* 初始化作业控制子系统 */
void bg_job_init(void);

/* 设置作业控制 */
void setjobctl(int on);

/* 创建作业 */
struct job *makejob(void *node, int nprocs);

/* fork子进程，支持作业控制 */
pid_t forkparent_bg(struct job *jp, int mode);
void forkchild_bg(struct job *jp, int mode);

/* 等待作业完成 */
int waitforjob(struct job *jp);
int waitcmd_bg(int argc, char **argv);

/* 前台后台命令 */
int fgcmd_bg(int argc, char **argv);
int bgcmd_bg(int argc, char **argv);

/* 显示作业 */
void showjobs(struct output *out, int mode);

/* 查找作业 */
struct job *getjob_by_jobno(int jobno);
struct job *getjob_by_pid(pid_t pid);

/* 获取作业编号 */
int jobno(const struct job *jp);

/* 其他辅助函数 */
int getstatus(struct job *jp);
void freejob(struct job *jp);
int stoppedjobs(void);

#ifdef __cplusplus
}
#endif

#endif /* DASH_BG_JOB_CONTROL_H */ 