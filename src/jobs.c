/*

	jobs.c - job control

	This file is part of zsh, the Z shell.

	zsh is free software; no one can prevent you from reading the source
   code, or giving it to someone else.

   This file is copyrighted under the GNU General Public License, which
   can be found in the file called COPYING.

   Copyright (C) 1990, 1991 Paul Falstad

   zsh is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY.  No author or distributor accepts
   responsibility to anyone for the consequences of using it or for
   whether it serves any particular purpose or works at all, unless he
   says so in writing.  Refer to the GNU General Public License
   for full details.

   Everyone is granted permission to copy, modify and redistribute
   zsh, but only under the conditions described in the GNU General Public
   License.   A copy of this license is supposed to have been given to you
   along with zsh so you can know your rights and responsibilities.
   It should be in a file named COPYING.

   Among other things, the copyright notice and this notice must be
   preserved on all copies.

*/

#include "zsh.h"
#include "funcs.h"
#include <sys/errno.h>

#define WCOREDUMPED(x) ((x)&0x80)

/* != 0 means the handler is active */

static int handling = 0;

/* != 0 means the shell is waiting for a job to complete */

static int waiting = 0;

#ifdef INTHANDTYPE
#define RETURN return 0
#else
#define RETURN return
#endif

/* the signal handler */

HANDTYPE handler(sig,code) /**/
int sig;int code;
{
long pid;
int statusp;
Job jn;
struct process *pn;
struct rusage ru;

#ifdef RESETHANDNEEDED
	signal(sig,handler);
#endif
	if (sig == SIGINT)
		{
		if (sigtrapped[SIGINT])
			dotrap(SIGINT);
		else
			errflag = 1;
		RETURN;
		}
	if (sig == SIGWINCH)
		adjustwinsize();
	if (sig != SIGCHLD)
		{
		dotrap(sig);
		if (sig == SIGALRM && !sigtrapped[SIGALRM])
			{
			zerr("timeout",NULL,0);
			exit(1);
			}
		RETURN;
		}
	for (;;)
		{
		pid = wait3(&statusp,WNOHANG|WUNTRACED,&ru);
		if (pid == -1)
			{
			if (errno != ECHILD)
				zerr("%e",NULL,errno);
			RETURN;
			}
		if (!pid)
			RETURN;
		findproc(pid,&jn,&pn);	/* find the process of this pid */
		if (jn)
			{
			pn->statusp = statusp;
			handling = 1;
			pn->ru = ru;
			pn->endtime = time(NULL);
			updatestatus(jn);
			handling = 0;
			if (zleactive)
				refresh();
			}
		else if (WIFSTOPPED(SP(statusp)))
			kill(pid,SIGKILL);	/* kill stopped untraced children */
		}
	RETURN;
}

/* change job table entry from stopped to running */

void makerunning(jn) /**/
Job jn;
{
struct process *pn;

	jn->stat &= ~STAT_STOPPED;
	for (pn = jn->procs; pn; pn = pn->next)
		if (WIFSTOPPED(SP(pn->statusp)))
			pn->statusp = SP_RUNNING;
}

/* update status of job, possibly printing it */

void updatestatus(jn) /**/
Job jn;
{
struct process *pn;
int notrunning = 1,alldone = 1,val,job = jn-jobtab,somestopped = 0;

	for (pn = jn->procs; pn; pn = pn->next)
		{
		if (pn->statusp == SP_RUNNING)
			notrunning = 0;
		if (pn->statusp == SP_RUNNING || WIFSTOPPED(SP(pn->statusp)))
			alldone = 0;
		if (WIFSTOPPED(SP(pn->statusp)))
			somestopped = 1;
		if (!pn->next && jn)
			val = (WIFSIGNALED(SP(pn->statusp))) ?
				0200 | WTERMSIG(SP(pn->statusp)) : WEXITSTATUS(SP(pn->statusp));
		}
	if (!notrunning)
		return;
	if (somestopped && (jn->stat & STAT_STOPPED))
		return;
	jn->stat |= (alldone) ? STAT_CHANGED|STAT_DONE :
		STAT_CHANGED|STAT_STOPPED;
	if (!alldone)
		{
		gettyinfo(&jn->ttyinfo);
		settyinfo(&shttyinfo);
		}
	else if (job == thisjob)
		{
		if (!val)
			{
			gettyinfo(&shttyinfo);
			columns = shttyinfo.winsize.ws_col;
			lines = shttyinfo.winsize.ws_row;
			}
		else
			settyinfo(&shttyinfo);
		lastval = val;
		}
	if (jn->stat & STAT_STOPPED)
		{
		prevjob = curjob;
		curjob = job;
		}
	if ((isset(NOTIFY) || job == thisjob) && jn->stat & STAT_LOCKED)
		printjob(jn,!!isset(LONGLISTJOBS));
	if (sigtrapped[SIGCHLD] && job != thisjob)
		dotrap(SIGCHLD);
}

/* find process and job associated with pid */

void findproc(pid,jptr,pptr) /**/
int pid;Job *jptr;struct process **pptr;
{
struct process *pn;
int jn;

	for (jn = 1; jn != MAXJOB; jn++)
		for (pn = jobtab[jn].procs; pn; pn = pn->next)
			if (pn->pid == pid)
				{
				*pptr = pn;
				*jptr = jobtab+jn;
				return;
				}
	*pptr = NULL;
	*jptr = NULL;
}

/*
	lng = 0 means jobs 
	lng = 1 means jobs -l
	lng = 2 means jobs -p
*/

void printjob(jn,lng) /**/
Job jn;int lng;
{
int job = jn-jobtab,len = 9,sig = -1,sflag = 0,llen,printed = 0;
int conted = 0,lineleng = getlineleng(),doputnl = 0,skip = 0;
struct process *pn;

	if (lng < 0)
		{
		conted = 1;
		lng = 0;
		}

	/* find length of longest signame, check to see if we
		really need to print this job */

	for (pn = jn->procs; pn; pn = pn->next)
		{
		if (pn->statusp != SP_RUNNING)
			if (WIFSIGNALED(SP(pn->statusp)))
				{
				sig = WTERMSIG(SP(pn->statusp));
				llen = strlen(sigmsg[sig]);
				if (WCOREDUMPED(pn->statusp))
					llen += 14;
				if (llen > len)
					len = llen;
				if (sig != SIGINT && sig != SIGPIPE)
					sflag = 1;
				if (sig == SIGINT && job == thisjob && interact)
					doputnl = 1;
				}
			else if (WIFSTOPPED(SP(pn->statusp)))
				{
				sig = WSTOPSIG(SP(pn->statusp));
				if (strlen(sigmsg[sig]) > len)
					len = strlen(sigmsg[sig]);
				if (handling && (!waiting || sig == SIGSTOP))
					doputnl = 1;
				}
			else if (isset(PRINTEXITVALUE) && WEXITSTATUS(SP(pn->statusp)))
				sflag = 1;
		}

	/* print if necessary */

	if (interact && jobbing && ((jn->stat & STAT_STOPPED) || sflag ||
			job != thisjob))
		{
		int len2,fline = 1;
		struct process *qn;

		trashzle();
		if (doputnl)
			putc('\n',stderr);
		for (pn = jn->procs; pn;)
			{
			len2 = ((job == thisjob) ? 5 : 10)+len; /* 2 spaces */
			if (lng)
				qn = pn->next;
			else for (qn = pn->next; qn; qn = qn->next)
				{
				if (qn->statusp != pn->statusp)
					break;
				if (strlen(qn->text)+len2+((qn->next) ? 3 : 0) > lineleng)
					break;
				len2 += strlen(qn->text)+2;
				}
			if (job != thisjob)
				if (fline)
					fprintf(stderr,"[%d]  %c ",jn-jobtab,(job == curjob) ? '+' :
						(job == prevjob) ? '-' : ' ');
				else
					fprintf(stderr,(job > 9) ? "        " : "       ");
			else
				fprintf(stderr,"zsh: ");
			if (lng)
				if (lng == 1)
					fprintf(stderr,"%d ",pn->pid);
				else
					{
					int x = jn->gleader;

					fprintf(stderr,"%d ",x);
					do skip++; while (x /= 10);
					skip++;
					lng = 0;
					}
			else
				fprintf(stderr,"%*s",skip,"");
			if (pn->statusp == SP_RUNNING)
				if (!conted)
					fprintf(stderr,"running%*s",len-7+2,"");
				else
					fprintf(stderr,"continued%*s",len-9+2,"");
			else if (WIFEXITED(SP(pn->statusp)))
				if (WEXITSTATUS(SP(pn->statusp)))
					fprintf(stderr,"exit %-4d%*s",WEXITSTATUS(SP(pn->statusp)),
						len-9+2,"");
				else
					fprintf(stderr,"done%*s",len-4+2,"");
			else if (WIFSTOPPED(SP(pn->statusp)))
				fprintf(stderr,"%-*s",len+2,sigmsg[WSTOPSIG(SP(pn->statusp))]);
			else if (WCOREDUMPED(pn->statusp))
				fprintf(stderr,"%s (core dumped)%*s",
					sigmsg[WTERMSIG(SP(pn->statusp))],
					len-14+2-strlen(sigmsg[WTERMSIG(SP(pn->statusp))]),"");
			else
				fprintf(stderr,"%-*s",len+2,sigmsg[WTERMSIG(SP(pn->statusp))]);
			for (; pn != qn; pn = pn->next)
				fprintf(stderr,(pn->next) ? "%s | " : "%s",pn->text);
			putc('\n',stderr);
			fline = 0;
			}
		printed = 1;
		fflush(stderr);
		}

	/* print "(pwd now: foo)" messages */

	if (interact && job==thisjob && strcmp(jn->cwd,cwd))
		{
		printf("(pwd now: ");
		printdir(cwd);
		printf(")\n");
		fflush(stdout);
		}

	/* delete job if done */

	if (jn->stat & STAT_DONE)
		{
		static struct job zero;
		struct process *nx;
		char *s;

		if (jn->stat & STAT_TIMED)
			{
			dumptime(jn);
			printed = 1;
			}
		for (pn = jn->procs; pn; pn = nx)
			{
			nx = pn->next;
			if (pn->text)
				free(pn->text);
			free(pn);
			}
		free(jn->cwd);
		if (jn->filelist)
			{
			while (s = getnode(jn->filelist))
				{
				unlink(s);
				free(s);
				}
			free(jn->filelist);
			}
		*jn = zero;
		if (job == curjob)
			{
			curjob = prevjob;
			prevjob = job;
			}
		if (job == prevjob)
			setprevjob();
		}
	else
		jn->stat &= ~STAT_CHANGED;
}

/* set the previous job to something reasonable */

void setprevjob() /**/
{
int t0;

	for (t0 = MAXJOB-1; t0; t0--)
		if (jobtab[t0].stat && jobtab[t0].stat & STAT_STOPPED &&
				t0 != curjob && t0 != thisjob)
			break;
	if (!t0)
		for (t0 = MAXJOB-1; t0; t0--)
			if (jobtab[t0].stat && t0 != curjob && t0 != thisjob)
				break;
	prevjob = (t0) ? t0 : -1;
}

/* initialize a job table entry */

void initjob() /**/
{
	jobtab[thisjob].cwd = ztrdup(cwd);
	jobtab[thisjob].stat = STAT_INUSE;
	jobtab[thisjob].ttyinfo = shttyinfo;
	jobtab[thisjob].gleader = 0;
}

/* add a process to the current job */

struct process *addproc(pid,text) /**/
long pid;char *text;
{
struct process *process;

	if (!jobtab[thisjob].gleader)
		jobtab[thisjob].gleader = lastpid = pid;
	lastpid = pid;
	process = zcalloc(sizeof *process);
	process->pid = pid;
	process->text = text;
	process->next = NULL;
	process->statusp = SP_RUNNING;
	process->bgtime = time(NULL);
	if (jobtab[thisjob].procs)
		{
		struct process *n;

		for (n = jobtab[thisjob].procs; n->next && !n->next->lastfg; n = n->next);
		process->next = n->next;
		n->next = process;
		}
	else
		jobtab[thisjob].procs = process;
	return process;
}

/* determine if it's all right to exec a command without
	forking in last component of subshells; it's not ok if we have files
	to delete */

int execok() /**/
{
Job jn;

	if (!exiting)
		return 0;
	for (jn = jobtab+1; jn != jobtab+MAXJOB; jn++)
		if (jn->stat && jn->filelist)
			return 0;
	return 1;
}

/* wait for a SIGCHLD, wait for the handler to execute, and return */

void chldsuspend() /**/
{
#ifdef SIGVEC
static struct sigvec vec = { handler,sigmask(SIGCHLD),SV_INTERRUPT };

	sigvec(SIGCHLD,&vec,NULL);
	sigpause(0);
	signal(SIGCHLD,handler);
#else
	pause();
#endif
}

/* wait for a job to finish */

void waitjob(job) /**/
int job;
{
static struct job zero;
Job jn;

	if (jobtab[job].procs)	/* if any forks were done */
		{
		jobtab[job].stat |= STAT_LOCKED;
		waiting = 1;
		if (jobtab[job].stat & STAT_CHANGED)
			printjob(jobtab+job,!!isset(LONGLISTJOBS));
		while (jobtab[job].stat &&
				!(jobtab[job].stat & (STAT_DONE|STAT_STOPPED)))
			chldsuspend();
		waiting = 0;
		}
	else	/* else do what printjob() usually does */
		{
		char *s;

		jn = jobtab+job;
		free(jn->cwd);
		if (jn->filelist)
			{
			while (s = getnode(jn->filelist))
				{
				unlink(s);
				free(s);
				}
			free(jn->filelist);
			}
		*jn = zero;
		}
}

/* wait for running job to finish */

void waitjobs() /**/
{
	waitjob(thisjob);
	thisjob = -1;
}

/* clear job table when entering subshells */

void clearjobtab() /**/
{
static struct job zero;
int t0;

	for (t0 = 1; t0 != MAXJOB; t0++)
		jobtab[thisjob] = zero;
}

/* get a free entry in the job table to use */

int getfreejob() /**/
{
int mask,t0;

	mask = sigblock(sigmask(SIGCHLD));
	for (t0 = 1; t0 != MAXJOB; t0++)
		if (!jobtab[t0].stat)
			{
			sigsetmask(mask);
			jobtab[t0].stat |= STAT_INUSE;
			return t0;
			}
	sigsetmask(mask);
	zerr("job table full",NULL,0);
	return -1;
}

/* print pids for & */

void spawnjob() /**/
{
struct process *pn;

	if (!subsh)
		{
		if (curjob == -1 || !(jobtab[curjob].stat & STAT_STOPPED))
			{
			curjob = thisjob;
			setprevjob();
			}
		else if (prevjob == -1 || !(jobtab[prevjob].stat & STAT_STOPPED))
			prevjob = thisjob;
		if (interact && jobbing && jobtab[thisjob].procs)
			{
			fprintf(stderr,"[%d]",thisjob);
			for (pn = jobtab[thisjob].procs; pn; pn = pn->next)
				fprintf(stderr," %d",pn->pid);
			fprintf(stderr,"\n");
			fflush(stderr);
			}
		}
	if (!jobtab[thisjob].procs)
		{
		char *s;
		static struct job zero;
		struct job *jn;

		jn = jobtab+thisjob;
		free(jn->cwd);
		if (jn->filelist)
			{
			while (s = getnode(jn->filelist))
				{
				unlink(s);
				free(s);
				}
			free(jn->filelist);
			}
		*jn = zero;
		}
	else
		jobtab[thisjob].stat |= STAT_LOCKED;
	thisjob = -1;
}

void fixsigs() /**/
{
	sigsetmask(0);
}

void printtime(real,ru,desc) /**/
time_t real;struct rusage *ru;char *desc;
{
char *s;

	if (!desc)
		desc = "";
	for (s = (timefmt) ? timefmt : DEFTIMEFMT; *s; s++)
		if (*s == '%')
			switch(s++,*s)
				{
				case 'E': fprintf(stderr,"%lds",real); break;
				case 'U': fprintf(stderr,"%ld.%03lds",
					ru->ru_utime.tv_sec,ru->ru_utime.tv_usec/1000); break;
				case 'S': fprintf(stderr,"%ld.%03lds",
					ru->ru_stime.tv_sec,ru->ru_stime.tv_usec/1000); break;
				case 'P':
					if (real)
						fprintf(stderr,"%d%%",
							(int) (ru->ru_utime.tv_sec+ru->ru_stime.tv_sec) / real);
					break;
				case 'W': fprintf(stderr,"%ld",ru->ru_nswap); break;
				case 'X': fprintf(stderr,"%ld",ru->ru_ixrss); break;
				case 'D': fprintf(stderr,"%ld",ru->ru_idrss); break;
				case 'K': fprintf(stderr,"%ld",ru->ru_ixrss+ru->ru_idrss); break;
				case 'M': fprintf(stderr,"%ld",ru->ru_maxrss); break;
				case 'F': fprintf(stderr,"%ld",ru->ru_majflt); break;
				case 'R': fprintf(stderr,"%ld",ru->ru_minflt); break;
				case 'I': fprintf(stderr,"%ld",ru->ru_inblock); break;
				case 'O': fprintf(stderr,"%ld",ru->ru_oublock); break;
				case 'r': fprintf(stderr,"%ld",ru->ru_msgrcv); break;
				case 's': fprintf(stderr,"%ld",ru->ru_msgsnd); break;
				case 'k': fprintf(stderr,"%ld",ru->ru_nsignals); break;
				case 'w': fprintf(stderr,"%ld",ru->ru_nvcsw); break;
				case 'c': fprintf(stderr,"%ld",ru->ru_nivcsw); break;
				default: fprintf(stderr,"%%%c",s[-1]); break;
				}
		else
			putc(*s,stderr);
	putc('\n',stderr);
	fflush(stderr);
}

void dumptime(jn) /**/
Job jn;
{
struct process *pn = jn->procs;

	if (!jn->procs)
		return;
	for (pn = jn->procs; pn; pn = pn->next)
		printtime(pn->endtime-pn->bgtime,&pn->ru,pn->text);
}

/* SIGHUP any jobs left running */

void killrunjobs() /**/
{
int t0,killed = 0;

	for (t0 = 1; t0 != MAXJOB; t0++)
		if (t0 != thisjob && jobtab[t0].stat &&
				!(jobtab[t0].stat & STAT_STOPPED))
			{
			killpg(jobtab[t0].gleader,SIGHUP);
			killed++;
			}
	if (killed)
		zerr("warning: %d jobs SIGHUPed",NULL,killed);
}

/* check to see if user has jobs running/stopped */

void checkjobs() /**/
{
int t0;

	for (t0 = 1; t0 != MAXJOB; t0++)
		if (t0 != thisjob && jobtab[t0].stat)
			break;
	if (t0 != MAXJOB)
		{
		if (jobtab[t0].stat & STAT_STOPPED)
			{
#ifdef USE_SUSPENDED
			zerr("you have suspended jobs.",NULL,0);
#else
			zerr("you have stopped jobs.",NULL,0);
#endif
			}
		else
			zerr("you have running jobs.",NULL,0);
		stopmsg = 1;
		}
}

/* send a signal to a job (simply involves killpg if monitoring is on) */

int killjb(jn,sig) /**/
Job jn;int sig;
{
struct process *pn;
int err;

	if (jobbing)
		return(killpg(jn->gleader,sig));
	for (pn = jn->procs; pn; pn = pn->next)
		if ((err = kill(pn->pid,sig)) == -1 && errno != ESRCH)
			return -1;
	return err;
}

