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
 *
 *	@(#)eval.h	8.2 (Berkeley) 5/4/95
 */

extern char *commandname;	/* currently executing command */
extern int exitstatus;		/* exit status of last command */
extern int back_exitstatus;	/* exit status of backquoted command */
extern int savestatus;		/* exit status of last command outside traps */


struct backcmd {		/* result of evalbackcmd */
	int fd;			/* file descriptor to read from */
	char *buf;		/* buffer */
	int nleft;		/* number of chars in buffer */
	struct job *jp;		/* job structure for command */
};

/* flags in argument to evaltree */
#define EV_EXIT 01		/* exit after evaluating tree */
#define EV_TESTED 02		/* exit status is checked; ignore -e flag */

int evalstring(char *, int);
union node;	/* BLETCH for ansi C */
int evaltree(union node *, int);
void evalbackcmd(union node *, struct backcmd *);

extern int evalskip;

/* reasons for skipping commands (see comment on breakcmd routine) */
#define SKIPBREAK	(1 << 0)
#define SKIPCONT	(1 << 1)
#define SKIPFUNC	(1 << 2)
#define SKIPFUNCDEF	(1 << 3)
