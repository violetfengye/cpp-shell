/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1997-2005
 *	Herbert Xu <herbert@gondor.apana.org.au>.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "shell.h"
#include "main.h"
#include "mail.h"
#include "options.h"
#include "output.h"
#include "parser.h"
#include "nodes.h"
#include "expand.h"
#include "eval.h"
#include "jobs.h"
#include "input.h"
#include "trap.h"
#include "var.h"
#include "show.h"
#include "memalloc.h"
#include "error.h"
#include "init.h"
#include "mystring.h"
#include "exec.h"
#include "cd.h"

#define PROFILE 0

int rootpid;
int shlvl;
#ifdef __GLIBC__
int *dash_errno;
#endif
#if PROFILE
short profile_buf[16384];
extern int etext();
#endif
MKINIT struct jmploc main_handler;

STATIC void read_profile(const char *);
STATIC char *find_dot_file(char *);
static int cmdloop(int);
int main(int, char **);

/*
 * Main routine.  We initialize things, parse the arguments, execute
 * profiles if we're a login shell, and then call cmdloop to execute
 * commands.  The setjmp call sets up the location to jump to when an
 * exception occurs.  When an exception occurs the variable "state"
 * is used to figure out how far we had gotten.
 */

int main(int argc, char **argv)
{
	char *shinit;			// 用于存储 ENV 环境变量的值，如果存在的话
	volatile int state;		// 用于 setjmp/longjmp 机制中的状态变量，标记程序执行到哪个阶段
	struct stackmark smark; // 栈标记，用于内存管理，方便回溯和释放临时分配的内存
	int login;				// 标志，指示当前 shell 是否是登录 shell

#ifdef __GLIBC__
	// 如果是 GNU C Library (GLIBC) 环境，则获取 errno 的位置
	// dash_errno 是一个指向 errno 的指针，这样 dash 可以直接访问和修改 errno
	dash_errno = __errno_location();
#endif

#if PROFILE
	// 如果启用了性能分析 (PROFILE 宏为 1)，则调用 monitor 函数进行初始化
	// monitor 是一个用于程序执行时间分析的工具函数
	monitor(4, etext, profile_buf, sizeof profile_buf, 50);
#endif
	state = 0; // 初始化状态变量为 0
	// setjmp 设置一个跳转点 (longjmp 的目标)。
	// 当发生异常时，可以通过 longjmp 跳转回这里。
	// 如果是正常执行，setjmp 返回 0；如果是从 longjmp 返回，则返回 longjmp 的第二个参数。
	if (unlikely(setjmp(main_handler.loc)))
	{
		int e; // 存储异常类型
		int s; // 存储发生异常时的状态

		exitreset(); // 重置与退出相关的状态，例如关闭文件描述符等

		e = exception; // 获取当前的异常类型

		s = state; // 保存发生异常时的状态
		// 判断是否需要直接退出 shell：
		// EXEND: 正常结束
		// EXEXIT: 明确的退出请求 (例如执行了 exit 命令)
		// s == 0: 异常发生在初始化阶段之前
		// iflag == 0: 非交互式 shell
		// shlvl: shell 嵌套级别，如果不是顶层 shell，可能直接退出
		if (e == EXEND || e == EXEXIT || s == 0 || iflag == 0 || shlvl)
			exitshell(); // 退出 shell

		reset(); // 重置 shell 的内部状态，例如清除临时变量、恢复信号处理等

		// 如果是中断异常 (EXINT) 并且满足特定条件 (例如终端是 atty 且不是 emacs 模式)
		// 则输出一个换行符，并刷新错误输出流
		if (e == EXINT
#if ATTY
			&& (!attyset() || equal(termval(), "emacs"))
#endif
		)
		{
			out2c('\n'); // 输出换行符到标准错误
#ifdef FLUSHERR
			flushout(out2); // 刷新标准错误输出缓冲区
#endif
		}
		popstackmark(&smark); // 弹出栈标记，释放异常发生前分配的临时内存
		FORCEINTON;			  // 强制启用中断，确保可以响应信号
		// 根据保存的状态 s，跳转到 main 函数中对应的代码段，继续执行或重新尝试
		if (s == 1)
			goto state1;
		else if (s == 2)
			goto state2;
		else if (s == 3)
			goto state3;
		else
			goto state4;
	}
	handler = &main_handler; // 将全局异常处理器设置为 main_handler
#ifdef DEBUG
	opentrace(); // 如果定义了 DEBUG 宏，则打开跟踪功能
	trputs("Shell args:  ");
	trargs(argv); // 打印 shell 启动参数用于调试
#endif
	rootpid = getpid();			  // 获取当前 shell 进程的 PID，并存储在全局变量 rootpid 中
	init();						  // 调用初始化函数，设置 shell 的基本环境，如信号处理、作业控制等
	setstackmark(&smark);		  // 设置一个新的栈标记，用于后续的内存管理
	login = procargs(argc, argv); // 处理命令行参数，并返回是否是登录 shell 的标志
	if (login)
	{								  // 如果是登录 shell
		state = 1;					  // 更新状态为 1
		read_profile("/etc/profile"); // 读取系统级的 profile 文件
	state1:
		state = 2;						// 更新状态为 2
		read_profile("$HOME/.profile"); // 读取用户主目录下的 .profile 文件
	}
state2:
	state = 3; // 更新状态为 3
	// 如果不是 Linux 系统 (即 __linux__ 未定义) 并且实际用户ID和有效用户ID相同，
	// 实际组ID和有效组ID相同，并且是交互式 shell (iflag 为真)
	if (
#ifndef linux
		getuid() == geteuid() && getgid() == getegid() &&
#endif
		iflag)
	{
		// 查找环境变量 "ENV" 的值
		if ((shinit = lookupvar("ENV")) != NULL && *shinit != '\0')
		{
			read_profile(shinit); // 如果 ENV 变量存在且非空，则读取其指定的文件
		}
	}
	popstackmark(&smark); // 弹出栈标记，释放为处理参数和配置文件而分配的临时内存
state3:
	state = 4; // 更新状态为 4
	// 如果命令行中指定了 -c 选项 (minusc 非空)，则执行 -c 后面的命令字符串
	if (minusc)
		evalstring(minusc, sflag ? 0 : EV_EXIT); // 执行字符串命令，sflag 决定是否在执行后退出

	// 如果是脚本模式 (sflag 为真) 或者没有指定 -c 选项 (minusc 为空)
	// 则进入主命令循环，从标准输入或文件中读取并执行命令
	if (sflag || minusc == NULL)
	{
	state4:			/* XXX ??? - why isn't this before the "if" statement */
		cmdloop(1); // 进入主命令循环，参数 1 表示是顶层命令循环 (通常会显示提示符)
	}
#if PROFILE
	monitor(0); // 如果启用了性能分析，则停止性能监控
#endif
#if GPROF
	{
		extern void _mcleanup(void);
		_mcleanup(); // 如果启用了 gprof 性能分析，则执行清理工作
	}
#endif
	exitshell();	 // 退出 shell，执行清理并退出进程
	/* NOTREACHED */ // 代码不会执行到这里，因为 exitshell() 会终止进程
}

/*
 * Read and execute commands.  "Top" is nonzero for the top level command
 * loop; it turns on prompting if the shell is interactive.
 */

static int
cmdloop(int top)
{
	union node *n;			// 指向解析后的命令树的根节点
	struct stackmark smark; // 栈标记，用于内存管理
	int inter;				// 标志，指示当前是否处于交互模式
	int status = 0;			// 存储命令的退出状态码
	int numeof = 0;			// 连续遇到 EOF (文件结束符) 的次数

	TRACE(("cmdloop(%d) called\n", top)); // 调试信息：打印 cmdloop 被调用的信息
	for (;;)
	{			  // 无限循环，这是 shell 的主循环，不断读取、解析和执行命令
		int skip; // 用于控制循环跳出的标志

		setstackmark(&smark);			  // 设置一个新的栈标记，用于当前命令的内存管理
		if (jobctl)						  // 如果启用了作业控制
			showjobs(out2, SHOW_CHANGED); // 显示已改变状态的作业信息
		inter = 0;						  // 默认设置为非交互模式
		if (iflag && top)
		{			   // 如果是交互式 shell (iflag 为真) 并且是顶层命令循环 (top 为真)
			inter++;   // 设置为交互模式
			chkmail(); // 检查是否有新邮件 (在交互式 shell 中通常会检查)
		}
		n = parsecmd(inter);	 // 解析命令：从输入中读取一行命令并将其解析成一个命令树 (AST)
		/* showtree(n); DEBUG */ // 调试代码，用于显示命令树结构
		if (n == NEOF)
		{							  // 如果解析结果是文件结束符 (EOF)
			if (!top || numeof >= 50) // 如果不是顶层循环，或者连续遇到 EOF 超过 50 次
				break;				  // 退出循环
			if (!stoppedjobs())
			{ // 如果没有停止的作业
				if (!Iflag)
				{ // 如果没有设置忽略 EOF 的标志 (Iflag)
					if (iflag)
					{				 // 如果是交互式 shell
						out2c('\n'); // 输出一个换行符
#ifdef FLUSHERR
						flushout(out2); // 刷新输出缓冲区
#endif
					}
					break; // 退出循环
				}
				out2str("\nUse \"exit\" to leave shell.\n"); // 提示用户使用 "exit" 退出
			}
			numeof++; // 连续 EOF 计数器加一
		}
		else
		{		   // 如果解析到了有效的命令
			int i; // 存储 eval 结果

			job_warning = (job_warning == 2) ? 1 : 0; // 更新作业警告状态
			numeof = 0;								  // 重置连续 EOF 计数器
			i = evaltree(n, 0);						  // 执行命令树，返回命令的退出状态码
			if (n)									  // 如果命令树不为空
				status = i;							  // 更新当前命令的退出状态码
		}
		popstackmark(&smark); // 弹出栈标记，释放当前命令执行过程中分配的临时内存

		skip = evalskip; // 获取 eval 过程中设置的跳过标志 (例如 break, continue)
		if (skip)
		{										   // 如果有跳过标志
			evalskip &= ~(SKIPFUNC | SKIPFUNCDEF); // 清除函数相关的跳过标志
			break;								   // 退出 cmdloop 循环
		}
	}

	return status; // 返回最后一个命令的退出状态码
}

/*
 * Read /etc/profile or .profile.  Return on error.
 */

STATIC void
read_profile(const char *name)
{
	name = expandstr(name); // 展开文件名中的变量，例如将 "$HOME/.profile" 展开为 "/home/user/.profile"
	// 设置输入文件：将 shell 的输入源切换到指定的 profile 文件。
	// INPUT_PUSH_FILE: 将当前输入源压入栈，以便在 profile 文件读取完毕后恢复。
	// INPUT_NOFILE_OK: 如果文件不存在，不报错，直接返回。
	if (setinputfile(name, INPUT_PUSH_FILE | INPUT_NOFILE_OK) < 0)
		return; // 如果设置输入文件失败 (例如文件不存在且没有 INPUT_NOFILE_OK 标志)，则直接返回

	cmdloop(0); // 执行 profile 文件中的命令。参数 0 表示这不是顶层命令循环，因此不会显示提示符。
	popfile();	// 从输入源栈中弹出 profile 文件，恢复到之前的输入源 (例如标准输入或脚本文件)
}

/*
 * Read a file containing shell functions.
 */

void readcmdfile(char *name)
{
	// 设置输入文件：将 shell 的输入源切换到指定的命令文件。
	// INPUT_PUSH_FILE: 将当前输入源压入栈，以便在命令文件读取完毕后恢复。
	setinputfile(name, INPUT_PUSH_FILE);
	cmdloop(0); // 执行命令文件中的命令。参数 0 表示这不是顶层命令循环，因此不会显示提示符。
	popfile();	// 从输入源栈中弹出命令文件，恢复到之前的输入源。
}

/*
 * Take commands from a file.  To be compatible we should do a path
 * search for the file, which is necessary to find sub-commands.
 */

STATIC char *
find_dot_file(char *basename)
{
	char *fullname;				  // 存储找到的完整文件路径
	const char *path = pathval(); // 获取 PATH 环境变量的值
	struct stat64 statb;		  // 用于存储文件状态信息的结构体
	int len;					  // 存储路径长度

	/* don't try this for absolute or relative paths */
	// 如果 basename 包含斜杠 (/)，说明它是一个绝对路径或相对路径，
	// 此时不需要在 PATH 中查找，直接返回原始名称。
	if (strchr(basename, '/'))
		return basename;

	// 遍历 PATH 环境变量中的每个目录
	// padvance 函数会从 path 中提取下一个目录，并将其与 basename 组合成完整路径
	// len 返回组合后的路径长度，如果找到下一个目录则 >= 0
	while ((len = padvance(&path, basename)) >= 0)
	{
		fullname = stackblock(); // 从栈上分配内存来存储完整路径
		// 检查文件是否存在且是普通文件：
		// (!pathopt || *pathopt == 'f')：可能与路径选项有关，确保是文件查找模式
		// !stat64(fullname, &statb)：获取文件状态成功 (文件存在)
		// S_ISREG(statb.st_mode)：检查文件是否是普通文件
		if ((!pathopt || *pathopt == 'f') &&
			!stat64(fullname, &statb) && S_ISREG(statb.st_mode))
		{
			/* This will be freed by the caller. */
			return stalloc(len); // 如果找到，则在堆上分配内存并返回完整路径
		}
	}

	/* not found in the PATH */
	sh_error("%s: not found", basename); // 如果在 PATH 中没有找到文件，则报告错误
	/* NOTREACHED */					 // 代码不会执行到这里，因为 sh_error 会抛出异常或退出
}

int dotcmd(int argc, char **argv)
{
	int status = 0; // 初始化返回状态为 0

	nextopt(nullstr); // 处理命令行选项，这里传入 nullstr 表示没有额外的选项需要处理
	argv = argptr;	  // 将 argv 指向处理选项后的参数列表

	if (*argv)
	{					// 如果有文件参数 (即 . 命令后面跟着文件名)
		char *fullname; // 存储找到的完整文件路径

		fullname = find_dot_file(*argv);		 // 在 PATH 中查找文件，获取完整路径
		setinputfile(fullname, INPUT_PUSH_FILE); // 设置输入文件为找到的文件，并将其压入输入栈
		commandname = fullname;					 // 设置当前命令的名称为完整文件路径
		status = cmdloop(0);					 // 执行文件中的命令。参数 0 表示非顶层循环。
		popfile();								 // 从输入栈中弹出文件，恢复到之前的输入源
	}

	return status; // 返回执行结果状态
}

int exitcmd(int argc, char **argv)
{
	// 检查是否有停止的作业 (stopped jobs)。
	// 如果有，通常 shell 不会立即退出，而是返回 0 (表示成功，但没有退出)。
	if (stoppedjobs())
		return 0;

	// 如果 exit 命令带有参数 (例如 `exit 10`)，则将该参数作为 shell 的退出状态码。
	// number() 函数将字符串转换为整数。
	if (argc > 1)
		savestatus = number(argv[1]);

	// 抛出 EXEXIT 异常。这将导致程序跳转回 main 函数中的 setjmp 块，
	// 从而触发 shell 的退出流程。
	exraise(EXEXIT);
	/* NOTREACHED */ // 这行代码永远不会被执行到，因为 exraise 会导致非局部跳转并最终退出进程。
}

#ifdef mkinit
INCLUDE "error.h"

	FORKRESET
{
	handler = &main_handler;
}
#endif
