/*

	exec.c - command execution

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
#include <sys/dir.h>

#define execerr() { if (forked) exit(1); \
	closemnodes(mfds); errflag = 1; return; }

/* parse list in a string */

List parselstring(s) /**/
char *s;
{
List l;

	hungets(s);
	strinbeg();
	pushheap();
	if (!(l = parlist()))
		{
		strinend();
		hflush();
		popheap();
		return NULL;
		}
	strinend();
	return l;
}

/* execute a string */

void execstring(s) /**/
char *s;
{
List l;

	if (l = parselstring(s))
		{
		execlist(l);
		popheap();
		}
}

/* duplicate a list and run it */

void newrunlist(l) /**/
List l;
{
	List a = dupstruct(l); runlist(a);
}

/* fork and set limits */

int phork() /**/
{
int pid = fork(),t0;

	if (pid == -1)
		{
		zerr("fork failed: %e",NULL,errno);
		return -1;
		}
	if (!pid)
		for (t0 = 0; t0 != RLIM_NLIMITS; t0++)
			setrlimit(t0,limits+t0);
	return pid;
}

/* execute a current shell command */

int execcursh(cmd) /**/
Cmd cmd;
{
	runlist(cmd->u.list);
	cmd->u.list = NULL;
	return lastval;
}

/* execve after handling $_ and #! */

void zexecve(pth,argv,ee,b1,b2) /**/
char *pth;char **argv;int *ee;char *b1;char *b2;
{
int eno;
char buf[MAXPATHLEN*2];
char **eep;

	for (eep = environ; *eep; eep++)
		if (**eep == '_' && (*eep)[1] == '=')
			break;
	buf[0] = '_';
	buf[1] = '=';
	if (*pth == '/')
		strcpy(buf+2,pth);
	else
		sprintf(buf+2,"%s/%s",cwd,pth);
	if (!*eep)
		eep[1] = NULL;
	*eep = buf;
	execve(pth,argv,environ);
	if ((eno = errno) == ENOEXEC)
		{
		char buf[33],*ptr,*ptr2,*argv0;
		int fd,ct,t0;

		if ((fd = open(pth,O_RDONLY)) >= 0)
			{
			argv0 = *argv;
			*argv = pth;
			ct = read(fd,buf,32);
			close(fd);
			if (ct > 0)
				{
				if (buf[0] == '#')
					if (buf[1] == '!')
						{
						for (t0 = 0; t0 != ct; t0++)
							if (buf[t0] == '\n')
								buf[t0] = '\0';
						buf[32] = '\0';
						for (ptr = buf+2; *ptr && *ptr == ' '; ptr++);
						for (ptr2 = ptr; *ptr && *ptr != ' '; ptr++);
						if (*ptr)
							{
							*ptr = '\0';
							argv[-2] = ptr2;
							argv[-1] = ptr+1;
							execve(ptr2,argv-2,environ);
							}
						else
							{
							argv[-1] = ptr2;
							execve(ptr2,argv-1,environ);
							}
						}
					else
						{
						argv[-1] = MYSELF;
						execve(MYSELF,argv-1,environ);
						}
				else
					{
					for (t0 = 0; t0 != ct; t0++)
						if (!buf[t0])
							break;
					if (t0 == ct)
						{
						argv[-1] = "/bin/sh";
						execve("/bin/sh",argv-1,environ);
						}
					}
				}
			else
				eno = errno;
			*argv = argv0;
			}
		else
			eno = errno;
		}
	if (ee && eno != ENOENT)
		{
		*ee = eno;
		strcpy(b1,b2);
		}
}

#define MAXCMDLEN (MAXPATHLEN*4)

/* execute an external command */

void execute(args,dash) /**/
Lklist args;int dash;
{
char **argv,*arg0;
char *z,*s,buf[MAXCMDLEN],buf2[MAXCMDLEN];
Cmdnam cn;
int tl,ee = 0;

	cn = gethnode(peekfirst(args),cmdnamtab);
	if (s = zgetenv("STTY"))
		zyztem("stty",s);
	arg0 = peekfirst(args);
	if (z = zgetenv("ARGV0"))
		{
		setdata(firstnode(args),ztrdup(z));
		delenv(z-6);
		}
	else if (dash)
		{
		sprintf(buf2,"-%s",arg0);
		setdata(firstnode(args),ztrdup(buf2));
		}
	argv = makecline(args);
	fixsigs();
	if (cn && (cn->type != BUILTIN && cn->type != SHFUNC))
		{
		if (cn->type == EXCMD_POSTDOT)
			zexecve(arg0,argv,&ee,buf2,buf);
		zexecve(cn->u.nam,argv,&ee,buf2,buf);
		}
	for (s = arg0; *s; s++)
		if (*s == '/')
			{
			zexecve(arg0,argv,NULL,NULL,NULL);
			if (arg0 == s || unset(PATHDIRS))
				{
				zerr("%e: %s",arg0,errno);
				_exit(1);
				}
			break;
			}
	for (; *path; path++)
		if ((*path)[0] == '.' && !(*path)[1])
			zexecve(arg0,argv,&ee,buf2,buf);
		else
			{
			tl = strlen(*path);
			strcpy(buf,*path);
			buf[tl] = '/';
			if (strlen(arg0)+strlen(buf)+1 >= MAXCMDLEN)
				{
				zerr("command too long: %s",arg0,0);
				_exit(1);
				}
			strcpy(buf+tl+1,arg0);
			zexecve(buf,argv,&ee,buf2,buf);
			}
	if (ee)
		{
		zerr("%e: %s",arg0,ee);
		_exit(1);
		}
	zerr("command not found: %s",arg0,0);
	_exit(1);
}

#define try(X) { if (!access(X,X_OK)) return ztrdup(X); }

/* get the pathname of a command */

char *findcmd(arg0) /**/
char *arg0;
{
char *s,buf[MAXPATHLEN];
int tl;
struct cmdnam *cn = gethnode(arg0,cmdnamtab);
char **pp = path;

	if (cn && (cn->type == EXCMD_POSTDOT || cn->type == EXCMD_PREDOT))
		{
		if (cn->type == EXCMD_POSTDOT)
			{
			strcpy(buf,"./");
			strcat(buf,arg0);
			try(buf);
			}
		try(cn->u.nam);
		}
	for (s = arg0; *s; s++)
		if (*s == '/')
			{
			try(arg0);
			if (s == arg0 || unset(PATHDIRS))
				goto failed;
			break;
			}
	for (; *pp; pp++)
		if (**pp == '.')
			{
			strcpy(buf,"./");
			strcat(buf,arg0);
			try(buf);
			}
		else
			{
			tl = strlen(*pp);
			strcpy(buf,*pp);
			buf[tl] = '/';
			strcpy(buf+tl+1,arg0);
			try(buf);
			}
failed:
	return NULL;
}

void execlist(list) /**/
List list;
{
	if (breaks)
		return;
	simplifyright(list);
	switch(list->type)
		{
		case SYNC:
		case ASYNC:
			execlist2(list->left,list->type,!list->right);
			if (sigtrapped[SIGDEBUG])
				dotrap(SIGDEBUG);
			if (sigtrapped[SIGERR] && lastval)
				dotrap(SIGERR);
			if (list->right && !retflag)
				execlist(list->right);
			break;
		}
}

void execlist2(list,type,last1) /**/
Sublist list;int type;int last1;
{
	switch(list->type)
		{
		case END:
			execpline(list,type,last1);
			break;
		case ORNEXT:
			if (!execpline(list,SYNC,0))
				execlist2(list->right,type,last1);
			break;
		case ANDNEXT:
			if (execpline(list,SYNC,0))
				execlist2(list->right,type,last1);
			break;
		}
}

int execpline(l,how,last1) /**/
Sublist l;int how;int last1;
{
int ipipe[2],opipe[2];

	ipipe[0] = ipipe[1] = opipe[0] = opipe[1] = 0;
	sigblock(sigmask(SIGCHLD));
	if ((thisjob = getfreejob()) == -1)
		return 1;
	initjob();
	if (how == TIMED)
		{
		jobtab[thisjob].stat |= STAT_TIMED;
		how = SYNC;
		}
	if (l->flags & PFLAG_COPROC)
		{
		how = ASYNC;
		mpipe(ipipe);
		mpipe(opipe);
		if (coprocin)
			{
			close(coprocin);
			close(coprocout);
			}
		coprocin = ipipe[0];
		coprocout = opipe[1];
		}
	execpline2(l->left,how,opipe[0],ipipe[1],last1);
	if (how == ASYNC)
		{
		spawnjob();
		sigsetmask(0);
		return 1;
		}
	else
		{
		waitjobs();
		sigsetmask(0);
		if (l->flags & PFLAG_NOT)
			lastval = !lastval;
		return !lastval;
		}
}

void execpline2(pline,how,input,output,last1) /**/
Pline pline;int how;int input;int output;int last1;
{
int pid;
int pipes[2];

	if (breaks)
		return;
	if (!pline)
		return;
	if (pline->type == END)
		{
		execcmd(pline->left,input,output,how==ASYNC,last1);
		pline->left = NULL;
		}
	else
		{
		mpipe(pipes);

		/* if we are doing "foo | bar" where foo is a current
			shell command, do foo in the current shell and do
			the rest of the pipeline in a subshell. */

		if (pline->left->type >= CURSH && how == SYNC)
			{
			if (!(pid = fork()))
				{
				close(pipes[1]);
				entersubsh(1);
				exiting = 1;
				execpline2(pline->right,ASYNC,pipes[0],output,1);
				_exit(lastval);
				}
			else if (pid == -1)
				zerr("fork failed: %e",NULL,errno);
			else
				{
				char *s,*text;

				close(pipes[0]);
				text = s = gettext((void *) pline->right,0);
				addproc(pid,text)->lastfg = 1;
				pline->right = NULL;
				}
			}

		/* otherwise just do the pipeline normally. */

		execcmd(pline->left,input,pipes[1],how==ASYNC,0);
		pline->left = NULL;
		close(pipes[1]);
		if (pline->right)
			{
			execpline2(pline->right,how,pipes[0],output,last1);
			close(pipes[0]);
			}
		}
}

/* make the argv array */

char **makecline(list) /**/
struct lklist *list;
{
int ct = 0;
Lknode node;
char **argv,**ptr;

	if (isset(XTRACE))
		{
		fprintf(stderr,"%s",(prompt4) ? prompt4 : "");
		for (node = firstnode(list); node; incnode(node),ct++);
		ptr = argv = 2+(char **) ncalloc((ct+4)*sizeof(char *));
		for (node = firstnode(list); node; incnode(node))
			if (*(char *) getdata(node))
				{
				*ptr++ = getdata(node);
				untokenize(getdata(node));
				fputs(getdata(node),stderr);
				if (nextnode(node))
					fputc(' ',stderr);
				}
		*ptr = NULL;
		fputc('\n',stderr);
		fflush(stderr);
		return(argv);
		}
	else
		{
		for (node = firstnode(list); node; incnode(node),ct++);
		ptr = argv = 2+(char **) ncalloc((ct+4)*sizeof(char *));
		for (node = firstnode(list); node; incnode(node))
			if (*(char *) getdata(node))
				{
				*ptr++ = getdata(node);
				untokenize(getdata(node));
				}
		*ptr = NULL;
		return(argv);
		}
}

/* untokenize the command line and remove null arguments */

void fixcline(l) /**/
Lklist l;
{
Lknode node,next;

	for (node = firstnode(l); node; node = next)
		{
		next = nextnode(node);
		if (*(char *) getdata(node))
			untokenize(getdata(node));
		else
			uremnode(l,node);
		}
}

void untokenize(s) /**/
char *s;
{
	for (; *s; s++)
		if (itok(*s))
			if (*s == Nularg)
				chuck(s--);
			else
				*s = ztokens[*s-Pound];
}

/* nonzero if we shouldn't clobber a file */

int dontclob(f) /**/
struct redir *f;
{
struct stat buf;

	if (unset(NOCLOBBER) || f->type & 1)
		return 0;
	if (stat(f->name,&buf) == -1)
		return 1;
	return S_ISREG(buf.st_mode);
}

/* close an multio (success) */

void closemn(mfds,fd) /**/
struct multio **mfds;int fd;
{
	if (mfds[fd])
		{
		if (mfds[fd]->ct > 1)
			if (mfds[fd]->rflag == 0)
				catproc(mfds[fd]);
			else
				teeproc(mfds[fd]);
		mfds[fd] = NULL;
		}
}

/* close all the mnodes (failure) */

void closemnodes(mfds) /**/
struct multio **mfds;
{
int t0,t1;

	for (t0 = 0; t0 != 10; t0++)
		if (mfds[t0])
			{
			for (t1 = 0; t1 != mfds[t0]->ct; t1++)
				close(mfds[t0]->fds[t1]);
			mfds[t0] = NULL;
			}
}

/* add a fd to an multio */
/* an multio is a list of fds associated with a certain fd.
	thus if you do "foo >bar >ble", the multio for fd 1 will have
	two fds, the result of open("bar",...), and the result of
	open("ble",....). */

void addfd(forked,save,mfds,fd1,fd2,rflag) /**/
int forked;int *save;struct multio **mfds;int fd1;int fd2;int rflag;
{
int pipes[2];

	if (!mfds[fd1])	/* starting a new multio */
		{
		mfds[fd1] = alloc(sizeof(struct multio));
		if (!forked && fd1 != fd2 && fd1 < 10)
			save[fd1] = movefd(fd1);
		redup(fd2,fd1);
		mfds[fd1]->ct = 1;
		mfds[fd1]->fds[0] = fd1;
		mfds[fd1]->rflag = rflag;
		}
	else
		{
		if (mfds[fd1]->rflag != rflag)
			{
			zerr("file mode mismatch on fd %d",NULL,fd1);
			return;
			}
		if (mfds[fd1]->ct == 1)		/* split the stream */
			{
			mfds[fd1]->fds[0] = movefd(fd1);
			mfds[fd1]->fds[1] = movefd(fd2);
			mpipe(pipes);
			mfds[fd1]->pipe = pipes[1-rflag];
			redup(pipes[rflag],fd1);
			mfds[fd1]->ct = 2;
			}
		else		/* add another fd to an already split stream */
			mfds[fd1]->fds[mfds[fd1]->ct++] = movefd(fd2);
		}
}

void addvars(l,export) /**/
Lklist l;int export;
{
struct varasg *v;
Lklist vl;

	while (full(l))
		{
		char **arr,**ptr;

		v = ugetnode(l);
		singsub(&v->name);
		if (errflag)
			return;
		untokenize(v->name);
		if (v->type == PMFLAG_s)
			{
			vl = newlist();
			addnode(vl,v->str);
			}
		else
			vl = v->arr;
		prefork(vl);
		if (errflag)
			return;
		postfork(vl,1);
		if (errflag)
			return;
		if (v->type == PMFLAG_s)
			{
			Param pm;
			char *val;

			if (!full(vl))
				pm = setsparam(v->name,val = ztrdup(""));
			else
				{
				untokenize(peekfirst(vl));
				pm = setsparam(v->name,val = ztrdup(ugetnode(vl)));
				}
			if (export && !(pm->flags & PMFLAG_x))
				addenv(v->name,val);
			continue;
			}
		ptr = arr = (char **) zalloc(sizeof(char **)*(countnodes(v->arr)+1));
		while (full(v->arr))
			{
			*ptr = ztrdup(ugetnode(v->arr));
			untokenize(*ptr++);
			}
		*ptr = NULL;
		setaparam(v->name,arr);
		}
}

void execcmd(cmd,input,output,bkg,last1) /**/
Cmd cmd;int input;int output;int bkg;int last1;
{
int type;
long pid = 0;
Lklist args = cmd->args;
int save[10],t0;
struct redir *fn;
struct multio *mfds[10];
int fil,forked = 0,iscursh = 0,nullexec = 0;
Cmdnam chn = NULL;
char *text;

	for (t0 = 0; t0 != 10; t0++)
		{
		save[t0] = 0;
		mfds[t0] = NULL;
		}
	if ((type = cmd->type) == SIMPLE && !full(args))
		if (full(cmd->redir))
			if (cmd->flags & CFLAG_EXEC)
				nullexec = 1;
			else
				addnode(args,strdup("cat"));
		else
			{
			addvars(cmd->vars,0);
			return;
			}
	if (full(args) && *(char *) peekfirst(args) == '%')
		{
		insnode(args,(Lknode) args,strdup((bkg) ? "bg" : "fg"));
		bkg = 0;
		}
	if (isset(AUTORESUME) && !full(cmd->redir) && full(args) &&
			type == SIMPLE && !nextnode(firstnode(args)) &&
			findjobnam(peekfirst(args)) != -1)
		pushnode(args,strdup("fg"));
	if (unset(RMSTARSILENT) && interact && isset(SHINSTDIN) &&
				type == SIMPLE && full(args) && nextnode(firstnode(args)) &&
				!strcmp(peekfirst(args),"rm"))
			{
			char *s = getdata(nextnode(firstnode(args)));

			if (s[0] == Star && !s[1])
				checkrmall();
			}
	if (jobbing)	/* get the text associated with this command */
		{
		char *s;
		s = text = gettext((void *) cmd,0);
		}
	else
		text = NULL;
	prefork(args);	/* do prefork substitutions */
	if (errflag)
		{
		lastval = 1;
		return;
		}
	if (full(args) && !(cmd->flags & CFLAG_COMMAND))
		chn = gethnode(peekfirst(args),cmdnamtab);
	if (!pathsuppress && !chn && isset(AUTOCD) && full(args) &&
			!full(cmd->redir) &&
			!nextnode(firstnode(args)) && cancd(peekfirst(args)))
		{
		pushnode(args,strdup("cd"));
		chn = gethnode("cd",cmdnamtab);
		}

	/* this is nonzero if cmd is a current shell procedure */

	iscursh = (type >= CURSH) || (type == SIMPLE && chn &&
		(chn->type == BUILTIN || chn->type == SHFUNC));

	/* if this command is backgrounded or (this is an external
		command and we are not exec'ing it) or this is a builtin
		with output piped somewhere, then fork.  If this is the
		last stage in a subshell pipeline, don't fork, but make
		the rest of the function think we forked. */

	if (bkg || !(iscursh || (cmd->flags & CFLAG_EXEC)) ||
			(chn && (chn->type == BUILTIN || chn->type == SHFUNC) && output))
		{
		pid = (last1 && execok()) ? 0 : phork();
		if (pid == -1)
			return;
		if (pid)
			{
			if (pid == -1)
				zerr("%e",NULL,errno);
			else
				( void ) addproc(pid,text);
			return;
			}
		entersubsh(bkg);
		forked = 1;
		}
	if (bkg && isset(BGNICE))	/* stupid */
		nice(5);
	if (input)		/* add pipeline input/output to mnodes */
		addfd(forked,save,mfds,0,input,0);
	if (output)
		addfd(forked,save,mfds,1,output,1);
	spawnpipes(cmd->redir);		/* do process substitutions */
	while (full(cmd->redir))
		if ((fn = ugetnode(cmd->redir))->type == INPIPE)
			{
			if (fn->fd2 == -1)
				execerr();
			addfd(forked,save,mfds,fn->fd1,fn->fd2,0);
			}
		else if (fn->type == OUTPIPE)
			{
			if (fn->fd2 == -1)
				execerr();
			addfd(forked,save,mfds,fn->fd1,fn->fd2,1);
			}
		else
			{
			if (!(fn->type == HERESTR || fn->type == CLOSE || fn->type ==
					MERGE || fn->type == MERGEOUT))
				if (xpandredir(fn,cmd->redir))
					continue;
			if (fn->type == HERESTR)
				{
				fil = getherestr(fn);
				if (fil == -1)
					{
					if (errno != EINTR)
						zerr("%e",NULL,errno);
					execerr();
					}
				addfd(forked,save,mfds,fn->fd1,fil,0);
				}
			else if (fn->type == READ)
				{
				fil = open(fn->name,O_RDONLY);
				if (fil == -1)
					{
					if (errno != EINTR)
						zerr("%e: %s",fn->name,errno);
					execerr();
					}
				addfd(forked,save,mfds,fn->fd1,fil,0);
				}
			else if (fn->type == CLOSE)
				{
				if (!forked && fn->fd1 < 3)
					{
					zerr("can't close fd %d without forking",NULL,fn->fd1);
					execerr();
					}
				closemn(mfds,fn->fd1);
				close(fn->fd1);
				}
			else if (fn->type == MERGE || fn->type == MERGEOUT)
				{
				fil = dup(fn->fd2);
				if (mfds[fn->fd1])
					redup(fil,fn->fd1);
				else
					addfd(forked,save,mfds,fn->fd1,fil,fn->type == MERGEOUT);
				}
			else
				{
				if (fn->type >= APP)
					fil = open(fn->name,isset(NOCLOBBER) ?
						O_WRONLY|O_APPEND : O_WRONLY|O_APPEND|O_CREAT,0666);
				else
					fil = open(fn->name,dontclob(fn) ? 
						O_WRONLY|O_CREAT|O_EXCL : O_WRONLY|O_CREAT|O_TRUNC,0666);
				if (fil == -1)
					{
					if (errno != EINTR)
						zerr("%e: %s",fn->name,errno);
					execerr();
					}
				addfd(forked,save,mfds,fn->fd1,fil,1);
				}
			}
	postfork(args,!(cmd->flags & CFLAG_NOGLOB));
		/* perform postfork substitutions */
	if (errflag)
		{
		lastval = 1;
		goto err;
		}
	
	/* we are done with redirection.  close the mnodes, spawning
		tee/cat processes as necessary. */
	for (t0 = 0; t0 != 10; t0++)
		closemn(mfds,t0);

	if (nullexec)
		return;
	if (unset(NOEXEC))
		if (type >= CTIME)
			{
			static int (*func[]) DCLPROTO((Cmd)) = {
				exectime,execcursh,execfuncdef,execfor,execwhile,
				execrepeat,execif,execcase,execselect,execcond };
	
			fixcline(args);
			lastval = (func[type-CTIME])(cmd);
			}
		else if (iscursh)		/* builtin or shell function */
			{
			if (cmd->vars)
				addvars(cmd->vars,0);
			fixcline(args);
			if (chn && chn->type == SHFUNC)
				execshfunc(cmd,chn);
			else
				{
				lastval = execbin(args,chn);
				if (isset(PRINTEXITVALUE) && lastval)
					zerr("exit %d",NULL,lastval);
				fflush(stdout);
				if (ferror(stdout))
					{
					zerr("write error: %e",NULL,errno);
					clearerr(stdout);
					}
				}
			}
		else
			{
			if (cmd->vars)
				addvars(cmd->vars,1);
			if (type == SIMPLE)
				{
				closem();
				execute(args,cmd->flags & CFLAG_DASH);
				}
			else	/* ( ... ) */
				execlist(cmd->u.list);
			}
err:
	if (forked)
		_exit(lastval);
	fixfds(save);
}

/* restore fds after redirecting a builtin */

void fixfds(save) /**/
int *save;
{
int t0;

	for (t0 = 0; t0 != 10; t0++)
		if (save[t0])
			redup(save[t0],t0);
}

void entersubsh(bkg) /**/
int bkg;
{
	if (!jobbing)
		{
		if (bkg && isatty(0))
			{
			close(0);
			if (open("/dev/null",O_RDWR))
				{
				zerr("can't open /dev/null: %e",NULL,errno);
				_exit(1);
				}
			}
		}
	else if (!jobtab[thisjob].gleader)
		{
		setpgrp(0L,jobtab[thisjob].gleader = getpid());
		if (!bkg)
			attachtty(jobtab[thisjob].gleader);
		}
	else
		setpgrp(0L,jobtab[thisjob].gleader);
	subsh = 1;
	if (SHTTY != -1)
		{
		close(SHTTY);
		SHTTY = -1;
		}
	if (jobbing)
		{
		signal(SIGTTOU,SIG_DFL);
		signal(SIGTTIN,SIG_DFL);
		signal(SIGTSTP,SIG_DFL);
		signal(SIGPIPE,SIG_DFL);
		}
	if (interact)
		{
		signal(SIGTERM,SIG_DFL);
		if (sigtrapped[SIGINT])
			signal(SIGINT,SIG_IGN);
		}
	if (!sigtrapped[SIGQUIT])
		signal(SIGQUIT,SIG_DFL);
	opts[MONITOR] = OPT_UNSET;
	clearjobtab();
}

/* close all internal shell fds */

void closem() /**/
{
int t0;

	for (t0 = 10; t0 != NOFILE; t0++)
		close(t0);
}

/* convert here document into a here string */

char *gethere(str) /**/
char *str;
{
char pbuf[256];
int qt = 0,siz = 0,l,strip = 0;
char *s,*t,*bptr;

	for (s = str; *s; s++)
		if (*s == Nularg)
			qt = 1;
	untokenize(str);
	if (*str == '-')
		{
		str++;
		strip = 1;
		while (*str == '\t')
			str++;
		}
	t = ztrdup("");
	for(;;)
		{
		char *u,*v;

		if (!fgets(pbuf,256,bshin))
			break;
		bptr = pbuf;
		if (strip)
			while (*bptr == '\t')
				bptr++;
		for (u = bptr, v = str; *u != '\n' && *v; u++,v++)
			if (*u != *v)
				break;
		if (!(*u == '\n' && !*v))
			{
			l = strlen(bptr);
			if (!qt && l > 1 && bptr[l-1] == '\n' && bptr[l-2] == '\\')
				bptr[l -= 2] = '\0';
			t = realloc(t,siz+l+1);
			strncpy(t+siz,bptr,l);
			siz += l;
			}
		else
			break;
		}
	t[siz] = '\0';
	if (siz && t[siz-1] == '\n')
		t[siz-1] = '\0';
	if (!qt)
		for (s = t; *s; s++)
			if (*s == '$')
				*s = Qstring;
			else if (*s == '`')
				*s = Qtick;
			else if (*s == '\\')
				{
				s++;
				if (!*s)
					break;
				}
	s = strdup(t);
	free(t);
	return s;
}

/* open here string fd */

int getherestr(fn) /**/
struct redir *fn;
{
Lklist fake;
char *s = gettemp(),*t;
int fd;

	fake = newlist();
	addnode(fake,fn->name);
	prefork(fake);
	if (!errflag)
		postfork(fake,1);
	if (errflag)
		return -1;
	if ((fd = open(s,O_CREAT|O_WRONLY,0600)) == -1)
		return -1;
	while (t = ugetnode(fake))
		{
		untokenize(t);
		write(fd,t,strlen(t));
		if (full(fake))
			write(fd," ",1);
		}
	write(fd,"\n",1);
	close(fd);
	fd = open(s,O_RDONLY);
	unlink(s);
	return fd;
}

void catproc(mn) /**/
struct multio *mn;
{
int len,t0;
char *buf;

	if (phork())
		{
		for (t0 = 0; t0 != mn->ct; t0++)
			close(mn->fds[t0]);
		close(mn->pipe);
		return;
		}
	closeallelse(mn);
	buf = zalloc(4096);
	for (t0 = 0; t0 != mn->ct; t0++)
		while (len = read(mn->fds[t0],buf,4096))
			write(mn->pipe,buf,len);
	_exit(0);
}
 
void teeproc(mn) /**/
struct multio *mn;
{
int len,t0;
char *buf;

	if (phork())
		{
		for (t0 = 0; t0 != mn->ct; t0++)
			close(mn->fds[t0]);
		close(mn->pipe);
		return;
		}
	buf = zalloc(4096);
	closeallelse(mn);
	while ((len = read(mn->pipe,buf,4096)) > 0)
		for (t0 = 0; t0 != mn->ct; t0++)
			write(mn->fds[t0],buf,len);
	_exit(0);
}

void closeallelse(mn) /**/
struct multio *mn;
{
int t0,t1;

	for (t0 = 0; t0 != NOFILE; t0++)
		if (mn->pipe != t0)
			{
			for (t1 = 0; t1 != mn->ct; t1++)
				if (mn->fds[t1] == t0)
					break;
			if (t1 == mn->ct)
				close(t0);
			}
}

long int zstrtol(s,t,base) /**/
char *s;char **t;int base;
{
int ret = 0;

	for (; *s >= '0' && *s < ('0'+base); s++)
		ret = ret*base+*s-'0';
	if (t)
		*t = (char *) s;
	return ret;
}

/* $(...) */

Lklist getoutput(cmd,qt) /**/
char *cmd;int qt;
{
List list;
int pipes[2];

	if (*cmd == '<')
		{
		int stream;
		char *fi;

		fi = strdup(cmd+1);
		if (*fi == '~')
			*fi = Tilde;
		else if (*fi == '=')
			*fi = Equals;
		singsub(&fi);
		if (errflag)
			return NULL;
		stream = open(fi,O_RDONLY);
		if (stream == -1)
			{
			zerr("%e: %s",cmd+1,errno);
			return NULL;
			}
		return readoutput(stream,qt);
		}
	if (!(list = parselstring(cmd)))
		return NULL;
	mpipe(pipes);
	if (phork())
		{
		popheap();
		close(pipes[1]);
		return readoutput(pipes[0],qt);
		}
	subsh = 1;
	close(pipes[0]);
	redup(pipes[1],1);
	entersubsh(0);
	signal(SIGTSTP,SIG_IGN);
	exiting = 1;
	execlist(list);
	close(1);
	exit(0);  return NULL;
}

/* read output of command substitution */

Lklist readoutput(in,qt) /**/
int in;int qt;
{
Lklist ret;
char *buf,*ptr;
int bsiz,c,cnt = 0;
FILE *fin;

	fin = fdopen(in,"r");
	ret = newlist();
	ptr = buf = zalloc(bsiz = 256);
	while ((c = fgetc(fin)) != EOF)
		if (!qt && isep(c))
			{
			if (cnt)
				{
				*ptr = '\0';
				addnode(ret,ztrdup(buf));
				cnt = 0;
				ptr = buf;
				}
			}
		else
			{
			*ptr++ = c;
			if (++cnt == bsiz)
				{
				char *pp = zalloc(bsiz *= 2);
				
				memcpy(pp,buf,cnt);
				free(buf);
				ptr = (buf = pp)+cnt;
				}
			}
	if (qt && ptr != buf && ptr[-1] == '\n')
		ptr[-1] = '\0';
	if (cnt)
		addnode(ret,ztrdup(buf));
	free(buf);
	fclose(fin);
	return ret;
}

/* =(...) */

char *getoutputfile(cmd) /**/
char *cmd;
{
#ifdef WAITPID
int pid;
#endif
char *nam = gettemp(),*str;
int tfil;
List list;

	if (thisjob == -1)
		return NULL;
	for (str = cmd; *str && *str != Outpar; str++);
	if (!*str)
		zerr("oops.",NULL,0);
	*str = '\0';
	if (!(list = parselstring(cmd)))
		return NULL;
	permalloc();
	if (!jobtab[thisjob].filelist)
		jobtab[thisjob].filelist = newlist();
	addnode(jobtab[thisjob].filelist,ztrdup(nam));
	heapalloc();
#ifdef WAITPID
	if (pid = phork())
		{
		popheap();
		waitpid(pid,NULL,WUNTRACED);
		return nam;
		}
#else
	if (waitfork())
		return nam;
#endif
	subsh = 1;
	close(1);
	entersubsh(0);
	tfil = creat(nam,0666);
	exiting = 1;
	execlist(list);
	close(1);
	exit(0); return NULL;
}

/* get a temporary named pipe */

char *namedpipe() /**/
{
char *tnam = gettemp();

	mknod(tnam,0010666,0);
	return tnam;
}

/* <(...) */

char *getoutproc(cmd) /**/
char *cmd;
{
List list;
int fd;
char *pnam,*str;

	if (thisjob == -1)
		return NULL;
	for (str = cmd; *str && *str != Outpar; str++);
	if (!*str)
		zerr("oops.",NULL,0);
	*str = '\0';
	pnam = namedpipe();
	permalloc();
	if (!jobtab[thisjob].filelist)
		jobtab[thisjob].filelist = newlist();
	addnode(jobtab[thisjob].filelist,ztrdup(pnam));
	heapalloc();
	if (!(list = parselstring(cmd)))
		return NULL;
	if (phork())
		{
		popheap();
		return pnam;
		}
	entersubsh(1);
	fd = open(pnam,O_WRONLY);
	if (fd == -1)
		{
		zerr("can't open %s: %e",pnam,errno);
		_exit(0);
		}
	redup(fd,1);
	fd = open("/dev/null",O_RDONLY);
	redup(fd,0);
	exiting = 1;
	execlist(list);
	close(1);
	_exit(0);  return NULL;
}

/* >(...) */

char *getinproc(cmd) /**/
char *cmd;
{
List list;
int pid,fd;
char *pnam,*str;

	if (thisjob == -1)
		return NULL;
	for (str = cmd; *str && *str != Outpar; str++);
	if (!*str)
		zerr("oops.",NULL,0);
	*str = '\0';
	pnam = namedpipe();
	permalloc();
	if (!jobtab[thisjob].filelist)
		jobtab[thisjob].filelist = newlist();
	addnode(jobtab[thisjob].filelist,ztrdup(pnam));
	heapalloc();
	if (!(list = parselstring(cmd)))
		return NULL;
	if (pid = phork())
		{
		popheap();
		return pnam;
		}
	entersubsh(1);
	fd = open(pnam,O_RDONLY);
	redup(fd,0);
	exiting = 1;
	execlist(list);
	_exit(0);  return NULL;
}

/* > >(...) (does not use named pipes) */

int getinpipe(cmd) /**/
char *cmd;
{
List list;
int pipes[2];
char *str = cmd;

	for (str = cmd; *str && *str != Outpar; str++);
	if (!*str)
		zerr("oops.",NULL,0);
	*str = '\0';
	if (!(list = parselstring(cmd+2)))
		return -1;
	mpipe(pipes);
	if (phork())
		{
		popheap();
		close(pipes[1]);
		return pipes[0];
		}
	close(pipes[0]);
	entersubsh(1);
	redup(pipes[1],1);
	exiting = 1;
	execlist(list);
	_exit(0);  return 0;
}

/* < <(...) */

int getoutpipe(cmd) /**/
char *cmd;
{
List list;
int pipes[2];
char *str;

	for (str = cmd; *str && *str != Outpar; str++);
	if (!*str)
		zerr("oops.",NULL,0);
	*str = '\0';
	if (!(list = parselstring(cmd+2)))
		return -1;
	strinend();
	mpipe(pipes);
	if (phork())
		{
		popheap();
		close(pipes[0]);
		return pipes[1];
		}
	close(pipes[1]);
	entersubsh(1);
	redup(pipes[0],0);
	exiting = 1;
	execlist(list);
	_exit(0);  return 0;
}

/* run a list, saving the current job num */

void runlist(l) /**/
List l;
{
int cj = thisjob;

	execlist(l);
	thisjob = cj;
}

char *gettemp() /**/
{
	return mktemp(strdup("/tmp/zshXXXXXX"));
}

/* my getwd; all the other ones I tried confused the SIGCHLD handler */

char *zgetwd() /**/
{
static char buf0[MAXPATHLEN];
char buf3[MAXPATHLEN],*buf2 = buf0+1;
struct stat sbuf;
struct direct *de;
DIR *dir;
ino_t ino = -1;
dev_t dev = -1;

	holdintr();
	buf2[0] = '\0';
	buf0[0] = '/';
	for(;;)
		{
		if (stat(".",&sbuf) < 0)
			{
			chdir(buf0);
			noholdintr();
			return ztrdup(".");
			}
		ino = sbuf.st_ino;
		dev = sbuf.st_dev;
		if (stat("..",&sbuf) < 0)
			{
			chdir(buf0);
			noholdintr();
			return ztrdup(".");
			}
		if (sbuf.st_ino == ino && sbuf.st_dev == dev)
			{
			chdir(buf0);
			noholdintr();
			return ztrdup(buf0);
			}
		dir = opendir("..");
		if (!dir)
			{
			chdir(buf0);
			noholdintr();
			return ztrdup(".");
			}
		chdir("..");
		readdir(dir); readdir(dir);
		while (de = readdir(dir))
			if (de->d_ino == ino)
				{
				lstat(de->d_name,&sbuf);
				if (sbuf.st_dev == dev)
					goto match;
				}
		rewinddir(dir);
		readdir(dir); readdir(dir);
		while (de = readdir(dir))
			{
			lstat(de->d_name,&sbuf);
			if (sbuf.st_dev == dev)
				goto match;
			}
		noholdintr();
		closedir(dir);
		return ztrdup(".");
match:
		strcpy(buf3,de->d_name);
		if (*buf2)
			strcat(buf3,"/");
		strcat(buf3,buf2);
		strcpy(buf2,buf3);
		closedir(dir);
		}
}

/* open pipes with fds >= 10 */

void mpipe(pp) /**/
int *pp;
{
	pipe(pp);
	pp[0] = movefd(pp[0]);
	pp[1] = movefd(pp[1]);
}

/* do process substitution with redirection */

void spawnpipes(l) /**/
Lklist l;
{
Lknode n = firstnode(l);
struct redir *f;

	for (; n; incnode(n))
		{
		f = getdata(n);
		if (f->type == OUTPIPE)
			{
			char *str = f->name;
			f->fd2 = getoutpipe(str);
			}
		if (f->type == INPIPE)
			{
			char *str = f->name;
			f->fd2 = getinpipe(str);
			}
		}
}

/* perform time ... command */

int exectime(cmd) /**/
Cmd cmd;
{
int jb = thisjob;

	execpline(cmd->u.pline,TIMED,0);
	thisjob = jb;
	return lastval;
}

/* define a function */

int execfuncdef(cmd) /**/
Cmd cmd;
{
Cmdnam cc;
char *s;

	permalloc();
	while (s = ugetnode(cmd->args))
		{
		cc = zalloc(sizeof *cc);
		cc->type = SHFUNC;
		cc->flags = 0;
		cc->u.list = dupstruct(cmd->u.list);
		addhnode(ztrdup(s),cc,cmdnamtab,freecmdnam);
		if (!strncmp(s,"TRAP",4))
			{
			int t0 = getsignum(s+4);

			if (t0 != -1)
				settrap(t0,cmd->u.list);
			}
		}
	heapalloc();
	return 0;
}

/* evaluate a [[ ... ]] */

int execcond(cmd) /**/
Cmd cmd;
{
	return !evalcond(cmd->u.cond);
}

void execshfunc(cmd,cn) /**/
Cmd cmd;Cmdnam cn;
{
char **tab,**x,*oargv0;
int oxtr = opts[XTRACE],flags,xexittr;
List l;
Lklist olist;
char *s;
List xexitfn;

	if (errflag)
		return;
	l = cn->u.list;
	if (!l)
		{
		char *nam;

		if (!(l = getfpfunc(nam = peekfirst(cmd->args))))
			{
			zerr("function not found: %s",nam,0);
			lastval = 1;
			return;
			}
		cn->flags &= ~PMFLAG_u;
		permalloc();
		cn->u.list = dupstruct(l);
		heapalloc();
		}
	flags = cn->flags;
	xexittr = sigtrapped[SIGEXIT];
	xexitfn = sigfuncs[SIGEXIT];
	tab = pparams;
	oargv0 = argzero;
	if (flags & PMFLAG_t)
		opts[XTRACE] = OPT_SET;
	pparams = x = zcalloc(((sizeof *x)*(1+countnodes(cmd->args))));
	argzero = ztrdup(ugetnode(cmd->args));
	while (*x = ugetnode(cmd->args))
		*x = ztrdup(*x), x++;
	permalloc();
	olist = locallist;
	locallist = newlist();
	heapalloc();
	newrunlist(l);
	while (s = getnode(locallist))
		unsetparam(s);
	free(locallist);
	locallist = olist;
	retflag = 0;
	cmd->u.list = NULL;
	freearray(pparams);
	free(argzero);
	argzero = oargv0;
	pparams = tab;
	if (sigtrapped[SIGEXIT])
		dotrap(SIGEXIT);
	if (sigfuncs[SIGEXIT])
		freestruct(sigfuncs[SIGEXIT]);
	sigtrapped[SIGEXIT] = xexittr;
	sigfuncs[SIGEXIT] = xexitfn;
	opts[XTRACE] = oxtr;
}

/* search fpath for an undefined function */

List getfpfunc(s) /**/
char *s;
{
char **pp = fpath,buf[MAXPATHLEN];
int fd;

	for (; *pp; pp++)
		{
		sprintf(buf,"%s/%s",*pp,s);
		if (!access(buf,X_OK) && (fd = open(buf,O_RDONLY)) != -1)
			{
			int len = lseek(fd,0,2);

			if (len == -1)
				close(fd);
			else
				{
				char *d;

				lseek(fd,0,0);
				d = zcalloc(len+1);
				if (read(fd,d,len) != len)
					{
					free(d);
					close(fd);
					}
				else
					{
					close(fd);
					return parselstring(d);
					}
				}
			}
		}
	return NULL;
}

/* check to see if AUTOCD applies here */

int cancd(s)
char *s;
{
char *t;

	for (t = s; *t; t++)
		if (*t == '/')
			break;
	if (!*t)
		{
		char sbuf[MAXPATHLEN],**cp;

		if (cancd2(s))
			return 1;
		for (cp = cdpath; *cp; cp++)
			{
			sprintf(sbuf,"%s/%s",*cp,s);
			if (cancd2(sbuf))
				return 1;
			}
		return 0;
		}
	return cancd2(s);
}

int cancd2(s)
char *s;
{
struct stat buf;

	return !(access(s,X_OK) || lstat(s,&buf) || !S_ISDIR(buf.st_mode));
}

