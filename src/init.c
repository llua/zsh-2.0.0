/*

	init.c - main loop and initialization routines

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

#define GLOBALS
#include "zsh.h"
#include "funcs.h"
#include <pwd.h>

extern int yydebug;

void main(argc,argv,envp) /**/
int argc; char **argv; char **envp;
{
int notect = 0;

	environ = envp;
	pathsuppress = 1;
	meminit();
	setflags();
	parseargs(argv);
	setmoreflags();
	setupvals();
	initialize();
	heapalloc();
	runscripts();
	if (interact)
		{
		pathsuppress = 0;
		newcmdnamtab();
		}
	for(;;)
		{
		do
			loop();
		while (!eofseen);
		if (!(isset(IGNOREEOF) && interact))
			{
#if 0
			if (interact)
				fputs(islogin ? "logout\n" : "exit\n",stderr);
#endif
			zexit(NULL);
			continue;
			}
		zerrnam("\nzsh",(!islogin) ? "use 'exit' to exit."
			: "use 'logout' to logout.",NULL,0);
		notect++;
		if (notect == 10)
			zexit(NULL);
		}
}

/* keep executing lists until EOF found */

void loop() /**/
{
List list;

	pushheap();
	for(;;)
		{
		freeheap();
		if (interact && isset(SHINSTDIN))
			preprompt();
		hbegin();		/* init history mech */
		intr();			/* interrupts on */
		ainit();			/* init alias mech */
		lexinit();
		errflag = 0;
		if (!(list = parlist()))
			{				/* if we couldn't parse a list */
			hend();
			if (eofseen && !errflag)
				break;
			continue;
			}
		if (hend())
			{
			if (stopmsg)		/* unset 'you have stopped jobs' flag */
				stopmsg--;
			execlist(list);
			}
		if (ferror(stderr))
			{
			zerr("write error",NULL,0);
			clearerr(stderr);
			}
		if (subsh)				/* how'd we get this far in a subshell? */
			exit(lastval);
		if ((!interact && errflag) || retflag)
			break;
		if ((opts['t'] == OPT_SET) || (lastval && opts[ERREXIT] == OPT_SET))
			{
			if (sigtrapped[SIGEXIT])
				dotrap(SIGEXIT);
			exit(lastval);
			}
		}
	popheap();
}

void setflags() /**/
{
int c;

	for (c = 0; c != 128; c++)
		opts[c] = OPT_INVALID;
	for (c = 'a'; c <= 'z'; c++)
		opts[c] = opts[c-'a'+'A'] = OPT_UNSET;
	for (c = '0'; c <= '9'; c++)
		opts[c] = OPT_UNSET;
	opts['A'] = opts['V'] = OPT_INVALID;
	opts['i'] = (isatty(0)) ? OPT_SET : OPT_UNSET;
	opts[BGNICE] = opts[NOTIFY] = OPT_SET;
	opts[USEZLE] = (interact && SHTTY != -1) ? OPT_SET : OPT_UNSET;
}

static char *cmd;

void parseargs(argv) /**/
char **argv;
{
char **x;
int bk = 0,action;
Lklist paramlist;

	argzero = *argv;
	opts[LOGINSHELL] = (**(argv++) == '-') ? OPT_SET : OPT_UNSET;
	SHIN = 0;
	while (!bk && *argv && (**argv == '-' || **argv == '+'))
		{
		action = (**argv == '-') ? OPT_SET : OPT_UNSET;
		while (*++*argv)
			{
			if (opts[**argv] == OPT_INVALID)
				{
				zerr("bad option: -%c",NULL,**argv);
				exit(1);
				}
			opts[**argv] = action;
			if (bk = **argv == 'b')
				break;
			if (**argv == 'c') /* -c command */
				{
				argv++;
				if (!*argv)
					{
					zerr("string expected after -c",NULL,0);
					exit(1);
					}
				cmd = *argv;
				opts[INTERACTIVE] = OPT_UNSET;
				break;
				}
			else if (**argv == 'o')
				{
				int c;

				argv++;
				c = optlookup(*argv);
				if (c == -1)
					zerr("no such option: %s",argv[-1],0);
				else
					opts[c] = action;
				break;
				}
			}
		argv++;
		}
	paramlist = newlist();
	if (*argv)
		{
		if (opts[SHINSTDIN] == OPT_UNSET)
			{
			SHIN = movefd(open(argzero = *argv,O_RDONLY));
			if (SHIN == -1)
				{
				zerr("can't open input file: %s",*argv,0);
				exit(1);
				}
			opts[INTERACTIVE] = OPT_UNSET;
			argv++;
			}
		while (*argv)
			addnode(paramlist,ztrdup(*argv++));
		}
	else
		opts[SHINSTDIN] = OPT_SET;
	pparams = x = zcalloc((countnodes(paramlist)+1)*sizeof(char *));
	while (*x++ = getnode(paramlist));
	free(paramlist);
	argzero = ztrdup(argzero);
}

void setmoreflags() /**/
{
	setvbuf(stdout,NULL,_IOFBF,BUFSIZ);	/* stdout,stderr fully buffered */
	setvbuf(stderr,NULL,_IOFBF,BUFSIZ);
	subsh = 0;
	opts[MONITOR] = (interact) ? OPT_SET : OPT_UNSET;
	if (jobbing)
		{
		SHTTY = movefd((isatty(0)) ? dup(0) : open("/dev/tty",O_RDWR));
		if (SHTTY == -1)
			opts[MONITOR] = OPT_UNSET;
		else
			gettyinfo(&shttyinfo);	/* get tty state */
		if ((mypgrp = getpgrp(0)) <= 0)
			opts[MONITOR] = OPT_UNSET;
		}
	else
		SHTTY = -1;
}

void setupvals() /**/
{
struct passwd *pwd;
char *ptr;
static long bauds[] = {
	0,50,75,110,134,150,200,300,600,1200,1800,2400,4800,9600,19200,38400
	};

	curhist = 0;
	histsiz = 20;
	lithistsiz = 5;
	logcheck = 60;
	dirstacksize = -1;
	listmax = 100;
	bangchar = '!';
	hashchar = '#';
	hatchar = '^';
	termok = 0;
	curjob = prevjob = coprocin = coprocout = -1;
	shtimer = time(NULL);	/* init $SECONDS */
	srand((unsigned int) shtimer);
	/* build various hash tables; argument to newhtable is table size */
	aliastab = newhtable(37);
	addreswords();
	paramtab = newhtable(151);
	cmdnamtab = newhtable(13);
	initxbindtab();
	if (interact)
		{
		prompt = ztrdup("%m%# ");
		prompt2 = ztrdup("> ");
		prompt3 = ztrdup("?# ");
		prompt4 = ztrdup("+ ");
		}
	ppid = getppid();
#ifdef TERMIOS
	baud = bauds[cfgetospeed(&shttyinfo.termios)];
#else
#ifdef TERMIO
	baud = bauds[shttyinfo.termio.c_cflag & CBAUD];
#else
	baud = bauds[shttyinfo.sgttyb.sg_ospeed];
#endif
#endif
	if (!(columns = shttyinfo.winsize.ws_col))
		columns = 80;
	if (!(lines = shttyinfo.winsize.ws_row))
		lines = 24;
	home = ztrdup("/");
	ifs = ztrdup(" \t\n");
	if (pwd = getpwuid(getuid()))
		{
		username = ztrdup(pwd->pw_name);
		home = xsymlink(pwd->pw_dir);
		}
	else
		{
		username = ztrdup("");
		home = ztrdup("/");
		}
	timefmt = ztrdup(DEFTIMEFMT);
	watchfmt = ztrdup(DEFWATCHFMT);
	ttystrname = ztrdup(ttyname(SHTTY));
	wordchars = ztrdup(DEFWORDCHARS);
	cwd = zgetwd();
	oldpwd = ztrdup(cwd);
	hostM = zalloc(512);	/* get hostname, with and without .podunk.edu */
	hostm = hostM+256;
	underscore = ztrdup("");
	gethostname(hostm,256);
	gethostname(hostM,256);
	mypid = getpid();
	cdpath = mkarray(ztrdup("."));
	fignore = mkarray(NULL);
	fpath = mkarray(NULL);
	mailpath = mkarray(NULL);
	watch = mkarray(NULL);
	userdirs = (char **) zcalloc(sizeof(char **)*2);
	usernames = (char **) zcalloc(sizeof(char **)*2);
	userdirsz = 2;
	userdirct = 0;
	optarg = ztrdup("");
	optind = 1;
	path = (char **) zalloc(4*sizeof *path);
	path[0] = ztrdup("/bin"); path[1] = ztrdup("/usr/bin");
	path[2] = ztrdup("/usr/ucb"); path[3] = NULL;
	for (ptr = hostM; *ptr && *ptr != '.'; ptr++);
	*ptr = '\0';
	inittyptab();
	setupparams();
	setparams();
	inittyptab();
}

void initialize() /**/
{
int t0;

	breaks = loops = incmd = 0;
	lastmailcheck = time(NULL);
	firsthist = firstlithist = 1;
	histsiz = DEFAULT_HISTSIZE;
	histlist = newlist();
	lithistlist = newlist();
	locallist = NULL;
	dirstack = newlist();
	bufstack = newlist();
	newcmdnamtab();
	inbuf = zalloc(inbufsz = 256);
	inbufptr = inbuf+inbufsz;
	inbufct = 0;
	/*signal(SIGQUIT,SIG_IGN);*/
	for (t0 = 0; t0 != RLIM_NLIMITS; t0++)
		getrlimit(t0,limits+t0);
	hsubl = hsubr = NULL;
	lastpid = 0;
	bshin = fdopen(SHIN,"r");
	signal(SIGCHLD,handler);
	if (jobbing)
		{
		signal(SIGTTOU,SIG_IGN);
		signal(SIGTSTP,SIG_IGN);
		signal(SIGTTIN,SIG_IGN);
		signal(SIGPIPE,SIG_IGN);
		attachtty(mypgrp);
		}
	if (interact)
		{
		signal(SIGTERM,SIG_IGN);
		signal(SIGWINCH,handler);
		signal(SIGALRM,handler);
		intr();
		}
}

void addreswords() /**/
{
static char *reswds[] = {
	"do", "done", "esac", "then", "elif", "else", "fi", "for", "case",
	"if", "while", "function", "repeat", "time", "until", "exec", "command",
	"select", "coproc", "noglob", "-", NULL
	};
int t0;

	for (t0 = 0; reswds[t0]; t0++)
		addhperm(reswds[t0],mkanode(NULL,-1-t0),aliastab,NULL);
}

void runscripts() /**/
{
/*	if (interact)
		checkfirstmail();*/
	if (opts[NORCS] == OPT_UNSET)
		{
#ifdef GLOBALZSHRC
		source(GLOBALZSHRC);
#endif
		sourcehome(".zshrc");
		if (islogin)
			{
#ifdef GLOBALZLOGIN
			source(GLOBALZLOGIN);
#endif
			sourcehome(".zlogin");
			}
		}
	if (interact)
		readhistfile();
	if (opts['c'] == OPT_SET)
		{
		if (SHIN >= 10)
			close(SHIN);
		SHIN = movefd(open("/dev/null",O_RDONLY));
		hungets(cmd);
		strinbeg();
		}
	if (!(columns = shttyinfo.winsize.ws_col))
		columns = 80;
	if (!(lines = shttyinfo.winsize.ws_row))
		lines = 24;
}

void ainit() /**/
{
	alstackind = 0;		/* reset alias stack */
	alstat = 0;
	isfirstln = 1;
}

void readhistfile() /**/
{
char *s,buf[1024];
FILE *in;

	if (!(s = getsparam("HISTFILE")))
		return;
	if (in = fopen(s,"r"))
		{
		permalloc();
		while (fgets(buf,1024,in))
			{
			int l = strlen(buf);

			if (l && buf[l-1] == '\n')
				buf[l-1] = '\0';
			addnode(histlist,ztrdup(buf));
			addnode(lithistlist,ztrdup(buf));
			curhist++;
			}
		fclose(in);
		lastalloc();
		}
}

void savehistfile() /**/
{
char *s,*t;
Lknode n;
Lklist l;
FILE *out;

	if (!(s = getsparam("HISTFILE")) || !interact)
		return;
	if (out = fdopen(open(s,O_CREAT|O_WRONLY|O_TRUNC,0600),"w"))
		{
		n = lastnode(l = (isset(HISTLIT) ? lithistlist : histlist));
		if (n == (Lknode) l)
			{
			fclose(out);
			return;
			}
		while (--savehist && prevnode(n) != (Lknode) l)
			n = prevnode(n);
		for (; n; incnode(n))
			{
			for (s = t = getdata(n); *s; s++)
				if (*s == HISTSPACE)
					*s = ' ';
			fputs(t,out);
			fputc('\n',out);
			}
		fclose(out);
		}
}

