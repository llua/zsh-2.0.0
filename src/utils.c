/*

	utils.c - miscellaneous utilities

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
#include <pwd.h>
#include <errno.h>
#include <sys/dir.h>
#include <fcntl.h>

/* source a file */

int source(s) /**/
char *s;
{
int fd,cj = thisjob;
int oldlineno = lineno,oldshst;
FILE *obshin = bshin;

	fd = SHIN;
	lineno = 0;
	oldshst = opts[SHINSTDIN];
	opts[SHINSTDIN] = OPT_UNSET;
	if ((SHIN = movefd(open(s,O_RDONLY))) == -1)
		{
		SHIN = fd;
		thisjob = cj;
		opts[SHINSTDIN] = oldshst;
		return 1;
		}
	bshin = fdopen(SHIN,"r");
	loop();
	fclose(bshin);
	bshin = obshin;
	opts[SHINSTDIN] = oldshst;
	SHIN = fd;
	thisjob = cj;
	errflag = 0;
	retflag = 0;
	lineno = oldlineno;
	return 0;
}

/* try to source a file in the home directory */

void sourcehome(s) /**/
char *s;
{
char buf[MAXPATHLEN];

	sprintf(buf,"%s/%s",home,s);
	(void) source(buf);
}

/* print an error */

void zerrnam(cmd,fmt,str,num) /**/
char *cmd; char *fmt; char *str;int num;
{
	if (cmd)
		{
		if (errflag || noerrs)
			return;
		errflag = 1;
		trashzle();
		if (isset(SHINSTDIN))
			fprintf(stderr,"%s: ",cmd);
		else
			fprintf(stderr,"%s: %s: ",argzero,cmd);
		}
	while (*fmt)
		if (*fmt == '%')
			{
			fmt++;
			switch(*fmt++)
				{
				case 's':
					while (*str)
						niceputc(*str++,stderr);
					break;
				case 'l':
					while (num--)
						niceputc(*str++,stderr);
					break;
				case 'd':
					fprintf(stderr,"%d",num);
					break;
				case '%':
					putc('%',stderr);
					break;
				case 'c':
					niceputc(num,stderr);
					break;
				case 'e':
					if (num == EINTR)
						{
						fputs("interrupt\n",stderr);
						errflag = 1;
						return;
						}
					fputc(tolower(sys_errlist[num][0]),stderr);
					fputs(sys_errlist[num]+1,stderr);
					break;
				}
			}
		else
			putc(*fmt++,stderr);
	if (unset(SHINSTDIN))
		fprintf(stderr," [%ld]\n",lineno);
	else
		putc('\n',stderr);
	fflush(stderr);
}

void zerr(fmt,str,num) /**/
char *fmt; char *str;int num;
{
	if (errflag || noerrs)
		return;
	errflag = 1;
	trashzle();
	fprintf(stderr,"%s: ",(isset(SHINSTDIN)) ? "zsh" : argzero);
	zerrnam(NULL,fmt,str,num);
}

void niceputc(c,f) /**/
int c;FILE *f;
{
	if (itok(c))
		{
		if (c >= Pound && c <= Qtick)
			putc(ztokens[c-Pound],f);
		return;
		}
	c &= 0x7f;
	if (c >= ' ' && c < '\x7f')
		putc(c,f);
	else if (c == '\n')
		{
		putc('\\',f);
		putc('n',f);
		}
	else
		{
		putc('^',f);
		putc(c|'A',f);
		}
}

/* enable ^C interrupts */

void intr() /**/
{
#ifdef SIGVEC
static struct sigvec vec = { handler,sigmask(SIGINT),SV_INTERRUPT };

	if (interact)
		sigvec(SIGINT,&vec,NULL);
	sigsetmask(0);
#else
	if (interact)
		signal(SIGINT,handler);
#endif
}

void noholdintr() /**/
{
	intr();
}

void holdintr() /**/
{
#ifdef SIGVEC
static struct sigvec vec = { handler,sigmask(SIGINT),0 };

	if (interact)
		{
		sigvec(SIGINT,&vec,NULL);
		sigsetmask(0);
		}
#else
	if (interact)
		signal(SIGINT,SIG_IGN);
#endif
}

char *fgetline(buf,len,in) /**/
char *buf;int len;FILE *in;
{
	if (!fgets(buf,len,in))
		return NULL;
	buf[len] = '\0';
	buf[strlen(buf)-1] = '\0';
	return buf;
}

/* get a symlink-free pathname for s relative to PWD */

char *findcwd(s) /**/
char *s;
{
char *t;

	if (*s == '/')
		return xsymlink(s);
	s = tricat((cwd[1]) ? cwd : "","/",s);
	t = xsymlink(s);
	free(s);
	return t;
}

static char xbuf[MAXPATHLEN];

/* expand symlinks in s, and remove other weird things */

char *xsymlink(s) /**/
char *s;
{
	if (*s != '/')
		return NULL;
	strcpy(xbuf,"");
	if (xsymlinks(s+1))
		return ztrdup(s);
	if (!*xbuf)
		return ztrdup("/");
	return ztrdup(xbuf);
}

char **slashsplit(s) /**/
char *s;
{
char *t,**r,**q;
int t0;

	if (!*s)
		return (char **) zcalloc(sizeof(char **));
	for (t = s, t0 = 0; *t; t++)
		if (*t == '/')
			t0++;
	q  = r = (char **) zalloc(sizeof(char **)*(t0+2));
	while (t = strchr(s,'/'))
		{
		*t = '\0';
		*q++ = ztrdup(s);
		*t = '/';
		while (*t == '/')
			t++;
		if (!*t)
			{
			*q = NULL;
			return r;
			}
		s = t;
		}
	*q++ = ztrdup(s);
	*q = NULL;
	return r;
}

int islink(s) /**/
char *s;
{
	return readlink(s,NULL,0) == 0;
}

int xsymlinks(s) /**/
char *s;
{
char **pp,**opp;
char xbuf2[MAXPATHLEN],xbuf3[MAXPATHLEN];
int t0;

	opp = pp = slashsplit(s);
	for (; *pp; pp++)
		{
		if (!strcmp(*pp,"."))
			{
			free(*pp);
			continue;
			}
		if (!strcmp(*pp,".."))
			{
			char *p;

			free(*pp);
			if (!strcmp(xbuf,"/"))
				continue;
			p = xbuf+strlen(xbuf);
			while (*--p != '/');
			*p = '\0';
			continue;
			}
		sprintf(xbuf2,"%s/%s",xbuf,*pp);
		t0 = readlink(xbuf2,xbuf3,MAXPATHLEN);
		if (t0 == -1)
			{
			strcat(xbuf,"/");
			strcat(xbuf,*pp);
			free(*pp);
			}
		else
			{
			xbuf3[t0] = '\0'; /* STUPID */
			if (*xbuf3 == '/')
				{
				strcpy(xbuf,"");
				if (xsymlinks(xbuf3+1))
					return 1;
				}
			else
				if (xsymlinks(xbuf3))
					return 1;
			free(*pp);
			}
		}
	free(opp);
	return 0;
}

/* print a directory */

void printdir(s) /**/
char *s;
{
int t0;

	t0 = finddir(s);
	if (t0 == -1)
		{
		if (!strncmp(s,home,t0 = strlen(home)))
			{
			putchar('~');
			fputs(s+t0,stdout);
			}
		else
			fputs(s,stdout);
		}
	else
		{
		putchar('~');
		fputs(usernames[t0],stdout);
		fputs(s+strlen(userdirs[t0]),stdout);
		}
}

/* see if a path has a named directory as its prefix */

int finddir(s) /**/
char *s;
{
int t0,t1,step;

	if (userdirsz)
		{
		step = t0 = userdirsz/2;
		for(;;)
			{
			t1 = (userdirs[t0]) ? dircmp(userdirs[t0],s) : 1;
			if (!t1)
				{
				while (t0 != userdirsz-1 && userdirs[t0+1] && 
						!dircmp(userdirs[t0+1],s)) 
					t0++;
				return t0;
				}
			if (!step)
				break;
			if (t1 > 0)
				t0 = t0-step+step/2;
			else
				t0 += step/2;
			step /= 2;
			}
		}
	return -1;
}

/* add a named directory */

void adduserdir(s,t) /**/
char *s;char *t;
{
int t0,t1;

	if (!interact || ((t0 = finddir(t)) != -1 && !strcmp(s,usernames[t0])))
		return;
	if ((t0 = finddir(t)) != -1 && !strcmp(s,usernames[t0]))
		return;
	if (userdirsz == userdirct)
		{
		userdirsz *= 2;
		userdirs = (char **) realloc((char *) userdirs,
			sizeof(char **)*userdirsz);
		usernames = (char **) realloc((char *) usernames,
			sizeof(char **)*userdirsz);
		for (t0 = userdirct; t0 != userdirsz; t0++)
			userdirs[t0] = usernames[t0] = NULL;
		}
	for (t0 = 0; t0 != userdirct; t0++)
		if (strcmp(userdirs[t0],t) > 0)
			break;
	for (t1 = userdirct-1; t1 >= t0; t1--)
		{
		userdirs[t1+1] = userdirs[t1];
		usernames[t1+1] = usernames[t1];
		}
	userdirs[t0] = ztrdup(t);
	usernames[t0] = ztrdup(s);
	userdirct++;
}

int dircmp(s,t) /**/
char *s;char *t;
{
	for (; *s && *t; s++,t++)
		if (*s != *t)
			return *s-*t;
	if (!*s && (!*t || *t == '/'))
		return 0;
	return *s-*t;
}

int ddifftime(t1,t2) /**/
time_t t1;time_t t2;
{
	return ((long) t2-(long) t1);
}

/* see if jobs need printing */

void scanjobs() /**/
{
int t0;

	for (t0 = 1; t0 != MAXJOB; t0++)
		if (jobtab[t0].stat & STAT_CHANGED)
			printjob(jobtab+t0,0);
}

/* do pre-prompt stuff */

void preprompt() /**/
{
int diff;
List list;
struct schedcmd *sch,*schl;

	if (unset(NOTIFY))
		scanjobs();
	if (errflag)
		return;
	if (list = getshfunc("precmd"))
		newrunlist(list);
	if (errflag)
		return;
	if (period && (time(NULL) > lastperiod+period) &&
			(list = getshfunc("periodic")))
		{
		newrunlist(list);
		lastperiod = time(NULL);
		}
	if (errflag)
		return;
	if (watch)
		{
		diff = (int) ddifftime(lastwatch,time(NULL));
		if (diff > logcheck)
			{
			dowatch();
			lastwatch = time(NULL);
			}
		}
	if (errflag)
		return;
	diff = (int) ddifftime(lastmailcheck,time(NULL));
	if (diff > mailcheck)
		{
		if (mailpath && *mailpath)
			checkmailpath(mailpath);
		else if (mailfile)
			{
			char *x[2];

			x[0] = mailfile;
			x[1] = NULL;
			checkmailpath(x);
			}
		lastmailcheck = time(NULL);
		}
	for (schl = (struct schedcmd *) &schedcmds, sch = schedcmds; sch;
			sch = (schl = sch)->next)
		{
		if (sch->time < time(NULL))
			{
			execstring(sch->cmd);
			schl->next = sch->next;
			free(sch->cmd);
			free(sch);
			}
		if (errflag)
			return;
		}
}

int arrlen(s) /**/
char **s;
{
int t0;

	for (t0 = 0; *s; s++,t0++);
	return t0;
}

void checkmailpath(s) /**/
char **s;
{
struct stat st;
char *v,*u,c;

	while (*s)
		{
		for (v = *s; *v && *v != '?'; v++);
		c = *v;
		*v = '\0';
		if (c != '?')
			u = NULL;
		else
			u = v+1;
		if (stat(*s,&st) == -1)
			{
			if (errno != ENOENT)
				zerr("%e: %s",*s,errno);
			}
		else if (S_ISDIR(st.st_mode))
			{
			Lklist l;
			DIR *lock = opendir(s);
			char buf[MAXPATHLEN*2],**arr,**ap;
			struct direct *de;
			int ct = 1;

			if (lock)
				{
				pushheap();
				heapalloc();
				l = newlist();
				readdir(lock); readdir(lock);
				while (de = readdir(lock))
					{
					if (errflag)
						break;
					if (u)
						sprintf(buf,"%s/%s?%s",*s,de->d_name,u);
					else
						sprintf(buf,"%s/%s",*s,de->d_name);
					addnode(l,strdup(buf));
					ct++;
					}
				closedir(lock);
				ap = arr = alloc(ct*sizeof(char *));
				while (*ap++ = ugetnode(l));
				checkmailpath(arr);
				popheap();
				}
			}
		else
			{
			if (st.st_size && st.st_atime < st.st_mtime &&
					st.st_mtime > lastmailcheck)
				if (!u)
					{
					fprintf(stderr,"You have new mail.\n",*s);
					fflush(stderr);
					}
				else
					{
					char *z = u;

					while (*z)
						if (*z == '$' && z[1] == '_')
							{
							fprintf(stderr,"%s",*s);
							z += 2;
							}
						else
							fputc(*z++,stderr);
					fputc('\n',stderr);
					fflush(stderr);
					}
			if (isset(MAILWARNING) && st.st_atime > st.st_mtime &&
					st.st_atime > lastmailcheck && st.st_size)
				{
				fprintf(stderr,"The mail in %s has been read.\n",*s);
				fflush(stderr);
				}
			}
		*v = c;
		s++;
		}
}

void saveoldfuncs(x,y) /**/
char *x;Cmdnam y;
{
Cmdnam cc;

	if (y->type == SHFUNC)
		{
		cc = zcalloc(sizeof *cc);
		*cc = *y;
		y->u.list = NULL;
		addhnode(ztrdup(x),cc,cmdnamtab,freecmdnam);
		}
}

/* create command hashtable */

void newcmdnamtab() /**/
{
int t0,dot = 0,pathct;
struct direct *de;
DIR *dir;
Cmdnam cc;
Hashtab oldcnt;

	oldcnt = cmdnamtab;
	permalloc();
	cmdnamtab = newhtable(101);
	if (pathsuppress)
		{
		addbuiltins();
		if (oldcnt)
			{
			listhtable(oldcnt,(HFunc) saveoldfuncs);
			freehtab(oldcnt,freecmdnam);
			}
		lastalloc();
		return;
		}
	holdintr();
	for (t0 = 0; path[t0]; t0++)
		if (!strcmp(".",path[t0]))
			{
			dot = 1;
			break;
			}
	for (pathct = t0,t0 = pathct-1; t0 >= 0; t0--)
		if (!strcmp(".",path[t0]))
			dot = 0;
		else if (strncmp("/./",path[t0],3))
			{
			dir = opendir(path[t0]);
			if (!dir)
				continue;
			readdir(dir); readdir(dir);
			while (de = readdir(dir))
				{
				cc = zcalloc(sizeof *cc);
				cc->type = (dot) ? EXCMD_POSTDOT : EXCMD_PREDOT;
				cc->u.nam = tricat(path[t0],"/",de->d_name);
				addhnode(ztrdup(de->d_name),cc,cmdnamtab,freecmdnam);
				}
			closedir(dir);
			}
	addbuiltins();
	if (oldcnt)
		{
		listhtable(oldcnt,(HFunc) saveoldfuncs);
		freehtab(oldcnt,freecmdnam);
		}
	noholdintr();
	lastalloc();
}

void freecmdnam(a) /**/
void *a;
{
struct cmdnam *c = (struct cmdnam *) a;

	if (c->type == SHFUNC)
		{
		if (c->u.list)
			freestruct(c->u.list);
		}
	else if (c->type != BUILTIN)
		free(c->u.nam);
	free(c);
}

void freestr(a) /**/
void *a;
{
	free(a);
}

void freeanode(a) /**/
void *a;
{
struct alias *c = (struct alias *) a;

	free(c->text);
	free(c);
}

void freepm(a) /**/
void *a;
{
struct param *pm = a;

	free(pm);
}

void restoretty() /**/
{
	settyinfo(&shttyinfo);
}

void gettyinfo(ti) /**/
struct ttyinfo *ti;
{
	if (jobbing)
		{
#ifdef TERMIOS
		ioctl(SHTTY,TCGETS,&ti->termios);
#else
#ifdef TERMIO
		ioctl(SHTTY,TCGETA,&ti->termio);
#else
		ioctl(SHTTY,TIOCGETP,&ti->sgttyb);
		ioctl(SHTTY,TIOCGETC,&ti->tchars);
		ioctl(SHTTY,TIOCGLTC,&ti->ltchars);
#endif
#endif
		ioctl(SHTTY,TIOCGWINSZ,&ti->winsize);
		}
}

void settyinfo(ti) /**/
struct ttyinfo *ti;
{
	if (jobbing)
		{
#ifdef TERMIOS
		ioctl(SHTTY,TCSETS,&ti->termios);
#else
#ifdef TERMIO
		ioctl(SHTTY,TCSETA,&ti->termio);
#else
		ioctl(SHTTY,TIOCSETP,&ti->sgttyb);
		ioctl(SHTTY,TIOCSETC,&ti->tchars);
		ioctl(SHTTY,TIOCSLTC,&ti->ltchars);
#endif
#endif
		ioctl(SHTTY,TIOCSWINSZ,&ti->winsize);
		}
}

void adjustwinsize() /**/
{
	ioctl(SHTTY,TIOCGWINSZ,&shttyinfo.winsize);
	columns = shttyinfo.winsize.ws_col;
	lines = shttyinfo.winsize.ws_row;
	if (zleactive)
		refresh();
}

int zyztem(s,t) /**/
char *s;char *t;
{
#ifdef WAITPID
int pid,statusp;

	if (!(pid = fork()))
		{
		s = tricat(s," ",t);
		execl("/bin/sh","sh","-c",s,(char *) 0);
		_exit(1);
		}
	waitpid(pid,&statusp,WUNTRACED);
	if (WIFEXITED(SP(statusp)))
		return WEXITSTATUS(SP(statusp));
	return 1;
#else
	if (!waitfork())
		{
		s = tricat(s," ",t);
		execl("/bin/sh","sh","-c",s,(char *) 0);
		_exit(1);
		}
	return 0;
#endif
}

#ifndef WAITPID

/* fork a process and wait for it to complete without confusing
	the SIGCHLD handler */

int waitfork() /**/
{
int pipes[2];
char x;

	pipe(pipes);
	if (!fork())
		{
		close(pipes[0]);
		signal(SIGCHLD,SIG_DFL);
		if (!fork())
			return 0;
		wait(NULL);
		_exit(0);
		}
	close(pipes[1]);
	read(pipes[0],&x,1);
	close(pipes[0]);
	return 1;
}

#endif

/* move a fd to a place >= 10 */

int movefd(fd) /**/
int fd;
{
int fe;

	if (fd == -1)
		return fd;
#ifdef F_DUPFD
	fe = fcntl(fd,F_DUPFD,10);
#else
	if ((fe = dup(fd)) < 10)
		fe = movefd(fe);
#endif
	close(fd);
	return fe;
}

/* move fd x to y */

void redup(x,y) /**/
int x;int y;
{
	if (x != y)
		{
		dup2(x,y);
		close(x);
		}
}

void settrap(t0,l) /**/
int t0;List l;
{
Cmd c;

	if (l)
		{
		c = l->left->left->left;
		if (c->type == SIMPLE && !full(c->args) && !full(c->redir)
				&& !full(c->vars) && !c->flags)
			l = NULL;
		}
	if (t0 == -1)
		return;
	if (jobbing && (t0 == SIGTTOU || t0 == SIGTSTP || t0 == SIGTTIN
			|| t0 == SIGPIPE))
		{
		zerr("can't trap SIG%s in interactive shells",sigs[t0-1],0);
		return;
		}
	if (!l)
		{
		sigtrapped[t0] = 2;
		if (t0 && t0 < SIGCOUNT && t0 != SIGCHLD)
			{
			signal(t0,SIG_IGN);
			sigtrapped[t0] = 2;
			}
		}
	else
		{
		if (t0 && t0 < SIGCOUNT && t0 != SIGCHLD)
			signal(t0,handler);
		sigtrapped[t0] = 1;
		permalloc();
		sigfuncs[t0] = dupstruct(l);
		heapalloc();
		}
}

void unsettrap(t0) /**/
int t0;
{
	if (t0 == -1)
		return;
	if (jobbing && (t0 == SIGTTOU || t0 == SIGTSTP || t0 == SIGTTIN
			|| t0 == SIGPIPE))
		{
		zerr("can't trap SIG%s in interactive shells",sigs[t0],0);
		return;
		}
	sigtrapped[t0] = 0;
	if (t0 == SIGINT)
		intr();
	else if (t0 && t0 < SIGCOUNT && t0 != SIGCHLD)
		signal(t0,SIG_DFL);
	if (sigfuncs[t0])
		freestruct(sigfuncs[t0]);
}

void dotrap(sig) /**/
int sig;
{
int sav;

	sav = sigtrapped[sig];
	if (sav == 2)
		return;
	sigtrapped[sig] = 2;
	if (sigfuncs[sig])
		newrunlist(sigfuncs[sig]);
	sigtrapped[sig] = sav;
}

/* copy t into *s and update s */

void strucpy(s,t) /**/
char **s;char *t;
{
char *u = *s;

	while (*u++ = *t++);
	*s = u-1;
}

void struncpy(s,t,n) /**/
char **s;char *t;int n;
{
char *u = *s;

	while (n--)
		*u++ = *t++;
	*s = u;
	*u = '\0';
}

void checkrmall() /**/
{
	fflush(stdin);
	fprintf(stderr,"zsh: are you sure you want to delete all the files? ");
	fflush(stderr);
	feep();
	errflag |= (getquery() != 'y');
}

int getquery() /**/
{
char c;
int yes = 'q';

	setcbreak();
	if (read(SHTTY,&c,1) == 1)
		if (c == 'y' || c == 'Y' || c == '\t')
			yes = 'y';
		else if (c == 'n')
			yes = 'n';
	unsetcbreak();
	if (c != '\n')
		write(2,"\n",1);
	return yes;
}

static int d;
static char *guess,*best;

void spscan(s,junk) /**/
char *s;char *junk;
{
int nd;

	nd = spdist(s,guess,strlen(guess)/4+1);
	if (nd <= d)
		{
		best = s;
		d = nd;
		}
}

/* spellcheck a word */

void spckword(s,cmd) /**/
char **s;int cmd;
{
char *t,*u;
int x;

	if (**s == '-' || **s == '%')
		return;
	if (!strcmp(*s,"in"))
		return;
	if (gethnode(*s,cmdnamtab) || gethnode(*s,aliastab) || strlen(*s) == 1)
		return;
	for (t = *s; *t; t++)
		if (itok(*t))
			return;
	if (access(*s,F_OK) == 0)
		return;
	best = NULL;
	for (t = *s; *t; t++)
		if (*t == '/')
			break;
	if ((u = spname(*s)) != *s)
		best = u;
	else if (!*t && !cmd)
		{
		guess = *s;
		d = 100;
		listhtable(aliastab,spscan);
		listhtable(cmdnamtab,spscan);
		}
	if (best && strlen(best) > 1)
		{
		fprintf(stderr,"zsh: correct to `%s' (y/n)? ",best);
		fflush(stderr);
		feep();
		x = getquery();
		if (x == 'y')
			*s = strdup(best);
		}
}

int ztrftime(buf,bufsize,fmt,tm) /**/
char *buf;int bufsize;char *fmt;struct tm *tm;
{
static char *astr[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
static char *estr[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul",
	"Aug","Sep","Oct","Nov","Dec"};
static char *lstr[] = {"12"," 1"," 2"," 3"," 4"," 5"," 6"," 7"," 8"," 9",
	"10","11"};

	while (*fmt)
		if (*fmt == '%')
			{
			fmt++;
			switch(*fmt++)
				{
				case 'a':
					strucpy(&buf,astr[tm->tm_wday]);
					break;
				case 'b':
					strucpy(&buf,estr[tm->tm_mon]);
					break;
				case 'd':
					*buf++ = '0'+tm->tm_mday/10;
					*buf++ = '0'+tm->tm_mday%10;
					break;
				case 'e':
					if (tm->tm_mday > 9)
						*buf++ = '0'+tm->tm_mday/10;
					*buf++ = '0'+tm->tm_mday%10;
					break;
				case 'k':
					if (tm->tm_hour > 9)
						*buf++ = '0'+tm->tm_hour/10;
					*buf++ = '0'+tm->tm_hour%10;
					break;
				case 'l':
					strucpy(&buf,lstr[tm->tm_hour%12]);
					break;
				case 'm':
					*buf++ = '0'+tm->tm_mon/10;
					*buf++ = '0'+tm->tm_mon%10;
					break;
				case 'M':
					*buf++ = '0'+tm->tm_min/10;
					*buf++ = '0'+tm->tm_min%10;
					break;
				case 'p':
					*buf++ = (tm->tm_hour > 11) ? 'p' : 'a';
					*buf++ = 'm';
					break;
				case 'S':
					*buf++ = '0'+tm->tm_sec/10;
					*buf++ = '0'+tm->tm_sec%10;
					break;
				case 'y':
					*buf++ = '0'+tm->tm_year/10;
					*buf++ = '0'+tm->tm_year%10;
					break;
				default:
					exit(20);
				}
			}
		else
			*buf++ = *fmt++;
	*buf = '\0';
	return 0;
}

char *join(arr,delim) /**/
char **arr;int delim;
{
int len = 0;
char **s,*ret,*ptr;

	for (s = arr; *s; s++)
		len += strlen(*s)+1;
	if (!len)
		return ztrdup("");
	ptr = ret = zalloc(len);
	for (s = arr; *s; s++)
		{
		strucpy(&ptr,*s);
		*ptr++ = delim;
		}
	ptr[-1] = '\0';
	return ret;
}

char *spacejoin(s) /**/
char **s;
{
	return join(s,*ifs);
}

char *colonjoin(s) /**/
char **s;
{
	return join(s,':');
}

char **colonsplit(s) /**/
char *s;
{
int ct;
char *t,**ret,**ptr;

	for (t = s, ct = 0; *t; t++)
		if (*t == ':')
			ct++;
	ptr = ret = (char **) zalloc(sizeof(char **)*(ct+2));
	t = s;
	do
		{
		for (s = t; *t && *t != ':'; t++);
		*ptr = zalloc((t-s)+1);
		strncpy(*ptr,s,(t-s)+1);
		(*ptr++)[t-s] = '\0';
		}
	while (*t++);
	*ptr = NULL;
	return ret;
}

char **spacesplit(s) /**/
char *s;
{
int ct;
char *t,**ret,**ptr;

	for (t = s, ct = 0; *t; t++)
		if (iblank(*t))
			ct++;
	ptr = ret = (char **) zalloc(sizeof(char **)*(ct+2));
	t = s;
	do
		{
		for (s = t; *t && !iblank(*t); t++);
		*ptr = zalloc((t-s)+1);
		strncpy(*ptr,s,(t-s)+1);
		(*ptr++)[t-s] = '\0';
		}
	while (*t++);
	*ptr = NULL;
	return ret;
}

List getshfunc(nam) /**/
char *nam;
{
Cmdnam x = gethnode(nam,cmdnamtab);

	return (x && x->type == SHFUNC) ? x->u.list : NULL;
}

/* allocate a tree element */

void *allocnode(type) /**/
int type;
{
int t0;
struct node *n = alloc(sizeof *n);
static int typetab[N_COUNT][4] = {
	NT_NODE,NT_NODE,0,0,
	NT_NODE,NT_NODE,0,0,
	NT_NODE,NT_NODE,0,0,
	NT_STR|NT_LIST,NT_NODE,NT_NODE|NT_LIST,NT_NODE|NT_LIST,
	NT_STR,0,0,0,
	NT_NODE,NT_NODE,0,0,
	NT_STR,NT_NODE,0,0,
	NT_NODE,NT_STR,NT_NODE,0,
	NT_NODE,NT_NODE,NT_NODE,0,
	NT_NODE,NT_NODE,0,0,
	NT_STR,NT_STR,NT_STR|NT_LIST,0
	};

	n->type = type;
	for (t0 = 0; t0 != 4; t0++)
		n->types[t0] = typetab[type][t0];
	return n;
}

/* duplicate a syntax tree */

void *dupstruct(a) /**/
void *a;
{
struct node *n = a,*m;
int t0;

	m = alloc(sizeof *m);
	*m = *n;
	for (t0 = 0; t0 != 4; t0++)
		if (m->ptrs[t0])
			switch(m->types[t0])
				{
				case NT_NODE: m->ptrs[t0] = dupstruct(m->ptrs[t0]); break;
				case NT_STR: m->ptrs[t0] =
					(useheap) ? strdup(m->ptrs[t0]) : ztrdup(m->ptrs[t0]); break;
				case NT_LIST|NT_NODE:
					m->ptrs[t0] = duplist(m->ptrs[t0],dupstruct); break;
				case NT_LIST|NT_STR:
					m->ptrs[t0] = duplist(m->ptrs[t0],(VFunc)
						((useheap) ? strdup : ztrdup));
					break;
				}
	return (void *) m;
}

/* free a syntax tree */

void freestruct(a) /**/
void *a;
{
struct node *n = a;
int t0;

	for (t0 = 0; t0 != 4; t0++)
		if (n->ptrs[t0])
			switch(n->types[t0])
				{
				case NT_NODE: freestruct(n->ptrs[t0]); break;
				case NT_STR: free(n->ptrs[t0]); break;
				case NT_LIST|NT_STR: freetable(n->ptrs[t0],freestr); break;
				case NT_LIST|NT_NODE: freetable(n->ptrs[t0],freestruct); break;
				}
	free(n);
}

Lklist duplist(l,func) /**/
Lklist l;VFunc func;
{
Lklist ret;
Lknode node;

	ret = newlist();
	for (node = firstnode(l); node; incnode(node))
		addnode(ret,func(getdata(node)));
	return ret;
}

char **mkarray(s) /**/
char *s;
{
char **t = (char **) zalloc((s) ? (2*sizeof s) : (sizeof s));

	if (*t = s)
		t[1] = NULL;
	return t;
}

void feep() /**/
{
	if (unset(NOBEEP))
		write(2,"\07",1);
}

void freearray(s) /**/
char **s;
{
char **t = s;

	while (*s)
		free(*s++);
	free(t);
}

int equalsplit(s,t) /**/
char *s;char **t;
{
	for (; *s && *s != '='; s++);
	if (*s == '=')
		{
		*s++ = '\0';
		*t = s;
		return 1;
		}
	return 0;
}

/* see if the right side of a list is trivial */

void simplifyright(l) /**/
List l;
{
Cmd c;

	if (!l->right)
		return;
	if (l->right->right || l->right->left->right ||
			l->right->left->left->right)
		return;
	c = l->left->left->left;
	if (c->type != SIMPLE || full(c->args) || full(c->redir)
			|| full(c->vars))
		return;
	l->right = NULL;
	return;
}

/* initialize the ztypes table */

void inittyptab() /**/
{
int t0;
char *s;

	for (t0 = 0; t0 != 256; t0++)
		typtab[t0] = 0;
	for (t0 = 0; t0 != 32; t0++)
		typtab[t0] = typtab[t0+128] = ICNTRL;
	typtab[127] = ICNTRL;
	for (t0 = '0'; t0 <= '9'; t0++)
		typtab[t0] = IDIGIT|IALNUM|IWORD|IIDENT|IUSER;
	for (t0 = 'a'; t0 <= 'z'; t0++)
		typtab[t0] = typtab[t0-'a'+'A'] = IALPHA|IALNUM|IIDENT|IUSER|IWORD;
	typtab['_'] = IIDENT;
	typtab['-'] = IUSER;
	typtab[' '] |= IBLANK|INBLANK;
	typtab['\t'] |= IBLANK|INBLANK;
	typtab['\n'] |= INBLANK;
	for (t0 = (int) (unsigned char) ALPOP; t0 <= (int) (unsigned char) Nularg;
			t0++)
		typtab[t0] |= ITOK;
	for (s = ifs; *s; s++)
		typtab[(int) (unsigned char) *s] |=
			(*s == '\n') ? ISEP|INBLANK : ISEP|IBLANK|INBLANK;
	for (s = wordchars; *s; s++)
		typtab[(int) (unsigned char) *s] |= IWORD;
	for (s = SPECCHARS; *s; s++)
		typtab[(int) (unsigned char) *s] |= ISPECIAL;
}

char **arrdup(s) /**/
char **s;
{
char **x,**y;

	y = x = ncalloc(sizeof(char *)*(arrlen(s)+1));
	while (*x++ = strdup(*s++));
	return y;
}

/* next few functions stolen (with changes) from Kernighan & Pike */
/* "The UNIX Programming Environment" (w/o permission) */

char *spname (oldname) /**/
char *oldname;
{
	char *p,guess[MAXPATHLEN+1],best[MAXPATHLEN+1];
	char newname[MAXPATHLEN+1];
	char *new = newname, *old = oldname;

	for (;;)
	{
		while (*old == '/')
			*new++ = *old++;
		*new = '\0';
		if (*old == '\0')
			return newname;
		p = guess;
		for (; *old != '/' && *old != '\0'; old++)
			if (p < guess+MAXPATHLEN)
				*p++ = *old;
		*p = '\0';
		if (mindist(newname,guess,best) >= 3)
			return oldname;
		for (p = best; *new = *p++; )
			new++;
	}
}

int mindist(dir,guess,best) /**/
char *dir;char *guess;char *best;
{
	int d,nd;
	DIR *dd;
	struct direct *de;

	if (dir[0] == '\0')
		dir = ".";
	d = 100;
	if (!(dd = opendir(dir)))
		return d;
	while (de = readdir(dd))
	{
		nd = spdist(de->d_name,guess,strlen(guess)/4+1);
		if (nd <= d)
			{
			strcpy(best,de->d_name);
			d = nd;
			if (d == 0)
				break;
			}
	}
	closedir(dd);
	return d;
}

int spdist(s,t,thresh) /**/
char *s;char *t;int thresh;
{
char *p,*q;
char *keymap =
"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\
\t1234567890-=\t\
\tqwertyuiop[]\t\
\tasdfghjkl;'\n\t\
\tzxcvbnm,./\t\t\t\
\n\n\n\n\n\n\n\n\n\n\n\n\n\n\
\t!@#$%^&*()_+\t\
\tQWERTYUIOP{}\t\
\tASDFGHJKL:\"\n\t\
\tZXCVBNM<>?\n\n\t\
\n\n\n\n\n\n\n\n\n\n\n\n\n\n";

	if (!strcmp(s,t))
		return 0;
	/* any number of upper/lower mistakes allowed (dist = 1) */
	for (p = s, q = t; *p && tolower(*p) == tolower(*q); p++,q++);
	if (!*p && !*q)
		return 1;
	if (!thresh)
		return 200;
	for (p = s, q = t; *p && *q; p++,q++)
		if (p[1] == q[0] && q[1] == p[0])  /* transpositions */
			return spdist(p+2,q+2,thresh-1)+1;
		else if (p[1] == q[0])	/* missing letter */
			return spdist(p+1,q+0,thresh-1)+2;
		else if (p[0] == q[1])	/* missing letter */
			return spdist(p+0,q+1,thresh-1)+2;
		else if (*p != *q)
			break;
	if ((!*p && strlen(q) == 1) || (!*q && strlen(p) == 1))
		return 2;
	for (p = s, q = t; *p && *q; p++,q++)
		if (p[0] != q[0] && p[1] == q[1])
			{
			int t0;
			char *z;

			/* mistyped letter */

			if (!(z = strchr(keymap,p[0])) || *z == '\n' || *z == '\t')
				return spdist(p+1,q+1,thresh-1)+1;
			t0 = z-keymap;
			if (*q == keymap[t0-15] || *q == keymap[t0-14] ||
					*q == keymap[t0-13] ||
					*q == keymap[t0-1] || *q == keymap[t0+1] ||
					*q == keymap[t0+13] || *q == keymap[t0+14] ||
					*q == keymap[t0+15])
				return spdist(p+1,q+1,thresh-1)+2;
			return 200;
			}
		else if (*p != *q)
			break;
	return 200;
}

char *zgetenv(s) /**/
char *s;
{
char **av,*p,*q;

	for (av = environ; *av; av++)
		{
		for (p = *av, q = s; *p && *p != '=' && *q && *p == *q; p++,q++);
		if (*p == '=' && !*q)
			return p+1;
		}
	return NULL;
}

