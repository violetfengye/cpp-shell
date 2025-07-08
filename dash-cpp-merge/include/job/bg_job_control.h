/**
 * @file bg_job_control.h
 * @brief C风格的后台任务控制API
 */

#ifndef DASH_BG_JOB_CONTROL_H
#define DASH_BG_JOB_CONTROL_H

#include <sys/types.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 作业状态 */
#define JOBSTOPPED 1
#define JOBRUNNING 2
#define JOBDONE    3
#define JOBDEAD    4

/* fork模式 */
#define FORK_FG    0
#define FORK_BG    1
#define FORK_NOJOB 2

/* 输出模式标志 */
#define SHOW_PGID    0x01
#define SHOW_PID     0x04
#define SHOW_CHANGED 0x08

/* 进程状态结构 */
struct procstat {
    pid_t pid;         /* 进程ID */
    int status;        /* 退出状态 */
    char *cmd;         /* 命令文本 */
};

/* 简单的输出缓冲区结构 */
struct output {
    int fd;            /* 文件描述符 */
    char *buf;         /* 缓冲区 */
    size_t bufsize;    /* 缓冲区大小 */
    size_t bufpos;     /* 当前位置 */
};

/* 作业结构 */
struct job {
    int used;          /* 是否使用中 */
    int changed;       /* 作业状态是否变化 */
    int notified;      /* 是否已通知用户 */
    int jobctl;        /* 作业控制启用标志 */
    int state;         /* 作业状态 */
    int stopstatus;    /* 停止状态 */
    struct procstat ps0; /* 主进程状态 */
    int nprocs;        /* 其他进程数 */
    struct procstat *ps; /* 其他进程状态数组 */
    struct job *prev_job; /* 上一个作业 */
};

/**
 * 初始化任务控制系统
 */
void bg_job_init(void);

/**
 * 启用或禁用作业控制
 */
void setjobctl(int on);

/**
 * 执行fg命令
 */
int fgcmd_bg(int argc, char **argv);

/**
 * 执行bg命令
 */
int bgcmd_bg(int argc, char **argv);

/**
 * 获取作业的工作号
 */
int jobno(const struct job *jp);

/**
 * 显示作业列表
 */
void showjobs(struct output *out, int mode);

/**
 * 获取作业的退出状态
 */
int getstatus(struct job *jp);

/**
 * 创建一个新作业
 */
struct job *makejob(void *node, int nprocs);

/**
 * 释放作业资源
 */
void freejob(struct job *jp);

/**
 * 父进程中fork前的处理
 */
pid_t forkparent_bg(struct job *jp, int mode);

/**
 * 子进程中fork后的处理
 */
void forkchild_bg(struct job *jp, int mode);

/**
 * 等待作业完成
 */
int waitforjob(struct job *jp);

/**
 * 等待命令
 */
int waitcmd_bg(int argc, char **argv);

/**
 * 检查是否有停止的作业
 */
int stoppedjobs(void);

/**
 * 根据作业号获取作业
 */
struct job *getjob_by_jobno(int jobno);

/**
 * 根据进程ID获取作业
 */
struct job *getjob_by_pid(pid_t pid);

/**
 * 向输出结构中添加格式化文本
 */
static void outfmt(struct output *out, const char *format, ...);

/**
 * 错误处理函数
 */
static void error(const char *msg, ...);

#ifdef __cplusplus
}
#endif

#endif /* DASH_BG_JOB_CONTROL_H */ 