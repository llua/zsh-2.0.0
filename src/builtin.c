/*

	builtin.c - builtin commands

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
#include <sys/times.h>

/* builtin flags */

#define BINF_PLUSOPTS		1		/* +xyz legal */
#define BINF_R					2		/* this is r (fc -e -) */
#define BINF_PRINTOPTS		4
#define BINF_SETOPTS			8
#define BINF_FCOPTS		  16
#define BINF_TYPEOPT      32
#define BINF_TYPEOPTS	  (BINF_TYPEOPT|BINF_PLUSOPTS)

/* builtin funcs */

enum xbins {
	BIN_TYPESET,BIN_BG,BIN_FG,BIN_JOBS,
	BIN_WAIT,BIN_DISOWN,BIN_BREAK,BIN_CONTINUE,BIN_EXIT,
	BIN_RETURN,BIN_SHIFT,BIN_CD,BIN_POPD,BIN_PUSHD,BIN_PRINT,
	BIN_EVAL,BIN_SCHED,
	BIN_FC,BIN_PUSHLINE,BIN_LOGOUT,BIN_BUILTIN
	};

struct bincmd {
	char *name;
	int (*handlerfunc) DCLPROTO((char *,char **,char *,int));
	int minargs;		/* min # of args */
	int maxargs;		/* max # of args, or -1 for no limit */
	int flags;			/* BINF_flags (see above) */
	int funcid;			/* xbins (see above) for overloaded handlerfuncs */
	char *optstr;		/* string of legal options */
	char *defopts;		/* options set by default for overloaded handlerfuncs */
	};

/* structure for foo=bar assignments */

struct asgment {
	struct asgment *next;
	char *name,*value;
	};

static char *auxdata;
static int auxlen;
static int showflag = 0,showflag2 = 0;

struct bincmd builtins[] = {
	".",bin_dot,1,-1,0,0,NULL,NULL,
	":",bin_colon,0,-1,0,0,NULL,NULL,
	"alias",bin_alias,0,-1,0,0,"ga",NULL,
	"autoload",bin_typeset,0,-1,BINF_TYPEOPTS,0,"tx","fu",
	"bg",bin_fg,0,-1,0,BIN_BG,NULL,NULL,
	"bindkey",bin_bindkey,0,-1,0,0,"asvedrl",NULL,
	"break",bin_break,0,1,0,BIN_BREAK,NULL,NULL,
	"builtin",NULL,0,0,0,BIN_BUILTIN,NULL,NULL,
	"bye",bin_break,0,1,0,BIN_EXIT,NULL,NULL,
	"cd",bin_cd,0,2,0,BIN_CD,NULL,NULL,
	"chdir",bin_cd,0,2,0,BIN_CD,NULL,NULL,
	"continue",bin_break,0,1,0,BIN_CONTINUE,NULL,NULL,
	"declare",bin_typeset,0,-1,BINF_TYPEOPTS,0,"LRZfilrtux",NULL,
	"dirs",bin_dirs,0,-1,0,0,"v",NULL,
	"disable",bin_unhash,1,-1,0,0,NULL,NULL,
	"disown",bin_fg,1,-1,0,BIN_DISOWN,NULL,NULL,
	"echo",bin_print,0,-1,BINF_PRINTOPTS,BIN_PRINT,"n","-",
	"echotc",bin_echotc,1,-1,0,0,NULL,NULL,
	"enable",bin_enable,1,-1,0,0,NULL,NULL,
	"eval",bin_eval,0,-1,0,BIN_EVAL,NULL,NULL,
	"exit",bin_break,0,1,0,BIN_EXIT,NULL,NULL,
	"export",bin_typeset,0,-1,BINF_TYPEOPTS,0,"LRZfilrtu","x",
	"false",bin_let,0,0,0,0,NULL,NULL,
	"fc",bin_fc,0,-1,BINF_FCOPTS,BIN_FC,"nlre",NULL,
	"fg",bin_fg,0,-1,0,BIN_FG,NULL,NULL,
	"functions",bin_typeset,0,-1,BINF_TYPEOPTS,0,"tu","f",
	"getln",bin_read,0,-1,0,0,NULL,"zr",
	"getopts",bin_getopts,2,-1,0,0,NULL,NULL,
	"hash",bin_hash,2,2,0,0,"r",NULL,
	"history",bin_fc,0,-1,0,BIN_FC,"nr","l",
	"integer",bin_typeset,0,-1,BINF_TYPEOPTS,0,"LRZlrtux","i",
	"jobs",bin_fg,0,-1,0,BIN_JOBS,"lp",NULL,
	"kill",bin_kill,0,-1,0,0,NULL,NULL,
	"let",bin_let,1,-1,0,0,NULL,NULL,
	"limit",bin_limit,0,-1,0,0,"sh",NULL,
	"local",bin_typeset,0,-1,BINF_TYPEOPTS,0,"LRZfilrtux",NULL,
	"log",bin_log,0,0,0,0,NULL,NULL,
	"logout",bin_break,0,1,0,BIN_LOGOUT,NULL,NULL,
	"popd",bin_cd,0,2,0,BIN_POPD,NULL,NULL,
	"print",bin_print,0,-1,BINF_PRINTOPTS,BIN_PRINT,"Rnrslz0-",NULL,
	"pushd",bin_cd,0,2,0,BIN_PUSHD,NULL,NULL,
	"pushln",bin_print,0,-1,BINF_PRINTOPTS,BIN_PRINT,NULL,"-nz",
	"pwd",bin_pwd,0,0,0,0,NULL,NULL,
	"r",bin_fc,0,-1,BINF_R,BIN_FC,"nrl",NULL,
	"read",bin_read,0,-1,0,0,"rz",NULL,
	"readonly",bin_typeset,0,-1,BINF_TYPEOPTS,0,"LRZfiltux","r",
	"rehash",bin_rehash,0,0,0,0,NULL,NULL,
	"return",bin_break,0,1,0,BIN_RETURN,NULL,NULL,
	"sched",bin_sched,0,-1,0,0,NULL,NULL,
	"set",bin_set,0,-1,BINF_SETOPTS|BINF_PLUSOPTS,0,"0123456789ABCDEFGHIJKLMNOPQRSTUWXYZaefghijklmnsuvxy",NULL,
	"setopt",bin_setopt,0,-1,0,0,"0123456789BCDEFGHIJKLMNOPQRSTUWXYZaefghijklmnsuvxy",NULL,
	"shift",bin_break,0,1,0,BIN_SHIFT,NULL,NULL,
	"source",bin_dot,1,-1,0,0,NULL,NULL,
	"suspend",bin_suspend,0,0,0,0,"f",NULL,
	"times",bin_times,0,0,0,0,NULL,NULL,
	"trap",bin_trap,0,-1,0,0,NULL,NULL,
	"true",bin_colon,0,0,0,0,NULL,NULL,
	"type",bin_whence,0,-1,0,0,"p","v",
	"typeset",bin_typeset,0,-1,BINF_TYPEOPTS,0,"LRZfilrtux",NULL,
	"ulimit",bin_ulimit,0,1,0,0,"HSacdfmnt",NULL,
	"umask",bin_umask,0,1,0,0,NULL,NULL,
	"unalias",bin_unalias,1,-1,0,0,NULL,NULL,
	"unfunction",bin_unhash,1,-1,0,0,NULL,NULL,
	"unhash",bin_unhash,1,-1,0,0,NULL,NULL,
	"unlimit",bin_unlimit,0,-1,0,0,"h",NULL,
	"unset",bin_unset,1,-1,0,0,NULL,NULL,
	"unsetopt",bin_setopt,0,-1,0,1,"0123456789BCDEFGHIJKLMNOPQRSTUWXYZaefghijklmnsuvxy",NULL,
	"vared",bin_vared,1,1,0,0,NULL,NULL,
	"wait",bin_fg,0,-1,0,BIN_WAIT,NULL,NULL,
	"whence",bin_whence,0,-1,0,0,"pv",NULL,
	"which",bin_whence,0,-1,0,0,"pv",NULL,
	NULL,NULL,0,0,0,0,NULL,NULL
	};

/* add builtins to the command hash table */

void addbuiltins() /**/
{
struct cmdnam *c;
struct bincmd *b;
int t0;

	for (t0 = 0, b = builtins; b->name; b++,t0++)
		{
		c = zcalloc(sizeof *c);
		c->type = BUILTIN;
		c->u.binnum = t0;
		addhperm(b->name,c,cmdnamtab,freecmdnam);
		}
}

/* enable */

int bin_enable(name,argv,ops,whocares) /**/
char *name;char **argv;char *ops;int whocares;
{
struct cmdnam *c;
struct bincmd *b;
int t0,ret = 0;

	for (; *argv; argv++)
		{
		for (t0 = 0, b = builtins; b->name; b++,t0++)
			if (!strcmp(*argv,b->name))
				break;
		if (!b->name)
			{
			zerrnam(name,"no such builtin: %s",*argv,0);
			ret = 1;
			}
		else
			{
			c = zcalloc(sizeof *c);
			c->type = BUILTIN;
			c->u.binnum = t0;
			addhperm(b->name,c,cmdnamtab,freecmdnam);
			}
		}
	return ret;
}

/* :, true */

int bin_colon(name,argv,ops,whocares) /**/
char *name;char **argv;char *ops;int whocares;
{
	return 0;
}

/* break, bye, continue, exit, logout, return, shift */

int bin_break(name,argv,ops,func) /**/
char *name;char **argv;char *ops;int func;
{
int num = -1;

	if (*argv)
		num = matheval(*argv);
	if ((func == BIN_BREAK || func == BIN_CONTINUE) && !loops)
		{
		zerrnam(name,"not in loop",NULL,0);
		return 1;
		}
	switch (func)
		{
		case BIN_BREAK:
			breaks = (num == -1) ? 1 : num;
			break;
		case BIN_CONTINUE:
			contflag = 1;
			break;
		case BIN_LOGOUT:
			if (!islogin)
				{
				zerrnam(name,"not login shell",NULL,0);
				return 1;
				}
		case BIN_EXIT:
			zexit((num == -1) ? lastval : num);
			break;
		case BIN_RETURN:
			retflag = 1;
			return lastval = (num == -1) ? lastval : num;
		case BIN_SHIFT:
			{
			char **s;

			if (num == -1)
				num = 1;
			if (num > arrlen(pparams))
				num = arrlen(pparams);
			permalloc();
			s = arrdup(pparams+num);
			heapalloc();
			freearray(pparams);
			pparams = s;
			break;
			}
		}
	return 0;
}

/* bg, disown, fg, jobs, wait */

int bin_fg(name,argv,ops,func) /**/
char *name;char **argv;char *ops;int func;
{
int job,lng,firstjob = -1,retval = 0;

	lng = (ops['l']) ? 1 : (ops['p']) ? 2 : 0;
	if ((func == BIN_FG || func == BIN_BG) && !jobbing)
		{
		zerrnam(name,"no job control in this shell.",NULL,0);
		return 1;
		}
	if (func == BIN_JOBS)
		stopmsg = 2;
	if (!*argv)
		if (func == BIN_FG || func == BIN_BG)
			{
			if (curjob == -1 || curjob == thisjob)
				{
				zerrnam(name,"no current job",NULL,0);
				return 1;
				}
			firstjob = curjob;
			}
		else if (func == BIN_JOBS)
			{
			for (job = 0; job != MAXJOB; job++)
				if (job != thisjob && jobtab[job].stat)
					printjob(job+jobtab,lng);
			return 0;
			}
		else
			{
			for (job = 0; job != MAXJOB; job++)
				if (job != thisjob && jobtab[job].stat)
					waitjob(job);
			return lastval;
			}
	for (; (firstjob != -1) || *argv; ( void ) (*argv && argv++))
		{
		int stopped,ocj = thisjob;

		job = (*argv) ? getjob(*argv,name) : firstjob;
		firstjob = -1;
		if (job == -1)
			break;
		if (!(jobtab[job].stat & STAT_INUSE))
			{
			zerrnam(name,"no such job: %d",0,job);
			return 1;
			}
		switch (func)
			{
			case BIN_FG:
			case BIN_BG:
				if (stopped = (jobtab[job].stat & STAT_STOPPED))
					makerunning(jobtab+job);
				else if (func == BIN_BG)
					{
					zerrnam(name,"job already in background",NULL,0);
					thisjob = ocj;
					return 1;
					}
				if (curjob == job)
					{
					curjob = prevjob;
					prevjob = (func == BIN_BG) ? -1 : job;
					}
				if (prevjob == job)
					prevjob = -1;
				if (prevjob == -1)
					setprevjob();
				if (curjob == -1)
					{
					curjob = prevjob;
					setprevjob();
					}
				printjob(jobtab+job,(stopped) ? -1 : 0);
				if (func == BIN_FG)
					{
					thisjob = job;
					if (strcmp(jobtab[job].cwd,cwd))
						{
						printf("(pwd : ");
						printdir(jobtab[job].cwd);
						printf(")\n");
						}
					settyinfo(&jobtab[job].ttyinfo);
					attachtty(jobtab[job].gleader);
					}
				if (stopped)
					killpg(jobtab[job].gleader,SIGCONT);
				if (func == BIN_FG)
					waitjobs();
				break;
			case BIN_JOBS:
				printjob(job+jobtab,lng);
				break;
			case BIN_WAIT:
				waitjob(job);
				retval = lastval;
				break;
			case BIN_DISOWN:
				{
				static struct job zero;
				jobtab[job] = zero;
				break;
				}
			}
		thisjob = ocj;
		}
	return retval;
}

/* false, let */

int bin_let(name,argv,ops,func) /**/
char *name;char **argv;char *ops;int func;
{
long val = 0;

	while (*argv)
		val = matheval(*argv++);
	return !val;
}

/* print the directory stack */

static void pdstack()
{
Lknode node;

	printdir(cwd);
	for (node = firstnode(dirstack); node; incnode(node))
		{
		putchar(' ');
		printdir(getdata(node));
		}
	putchar('\n');
}

/* exit the shell */

int zexit(val) /**/
int val;
{
	if (isset(MONITOR))
		if (!stopmsg)
			{
			checkjobs();
			if (stopmsg)
				{
				stopmsg = 2;
				return 1;
				}
			}
		else
			killrunjobs();
	savehistfile();
	if (islogin && unset(NORCS))
		sourcehome(".zlogout");
	if (sigtrapped[SIGEXIT])
		dotrap(SIGEXIT);
	exit(val); return 0;
}

/* identify an option name */

int optlookup(s) /**/
char *s;
{
char *t;
struct option *o;

	t = s = strdup(s);
	while (*t)
		if (*t == '_')
			chuck(t);
		else
			{
			*t = tolower(*t);
			t++;
			}
	for (o = optns; o->name; o++)
		if (!strcmp(o->name,s))
			return o->id;
	return -1;
}

/* setopt, unsetopt */

int bin_setopt(nam,args,ops,isun) /**/
char *nam;char **args;char *ops;int isun;
{
struct option *opp;
int c;

	if (!ops['@'] && !*args)
		{
		if (isun)
			return 0;
		for (opp = optns; opp->name; opp++)
			if (opts[opp->id] == OPT_SET)
				puts(opp->name);
		return 0;
		}
	for (opp = optns; opp->name; opp++)
		if (ops[opp->id] == 1+isun)
			opts[opp->id] = OPT_SET;
		else if (ops[opp->id] == 2-isun)
			opts[opp->id] = OPT_UNSET;
	while (*args)
		{
		c = optlookup(*args++);
		if (c != -1)
			{
			if (c == INTERACTIVE || c == MONITOR)
				zerrnam(nam,"can't change that option",NULL,0);
			else
				opts[c] = (isun) ? OPT_UNSET : OPT_SET;
			}
		else
			{
			zerrnam(nam,"no such option: %s",args[-1],0);
			return 1;
			}
		}
	return 0;
}

/* execute func on each member of the hash table ht */

void listhtable(ht,func) /**/
Hashtab ht;HFunc func;
{
int t0;
struct hashnode *hn;

	for (t0 = ht->hsize-1; t0 >= 0; t0--)
		for (hn = ht->nodes[t0]; hn; hn = hn->next)
			func(hn->nam,hn->dat);
}

/* print a shell function (used with listhtable) */

void pshfunc(s,cc) /**/
char *s;Cmdnam cc;
{
char *t;

	if (cc->type != SHFUNC)
		return;
	if (showflag && (cc->flags & showflag2) != showflag2)
		return;
	if (cc->flags & PMFLAG_u)
		printf("undefined ");
	if (cc->flags & PMFLAG_t)
		printf("traced ");
	if (!cc->u.list || !showflag)
		{
		printf("%s ()\n",s);
		return;
		}
	t = gettext((void *) (cc->u.list),1);
	printf("%s () {\n\t%s\n}\n",s,t);
	free(t);
}

void niceprint(s) /**/
char *s;
{
	niceprintf(s,stdout);
}

void niceprintf(s,f) /**/
char *s;FILE *f;
{
	for (; *s; s++)
		{
		if (*s >= 32 && *s <= 126)
			fputc(*s,f);
		else if (*s == '\n')
			{
			putc('\\',f);
			putc('n',f);
			}
		else
			{
			putc('^',f);
			fputc(*s | 0x40,f);
			}
		}
}

int bin_umask(nam,args,ops,func) /**/
char *nam;char **args;char *ops;int func;
{
int um;
char *s = *args;

	um = umask(0);
	umask(um);
	if (!s)
		{
		printf("%03o\n",um);
		return 0;
		}
	if (idigit(*s))
		{
		um = strtol(s,&s,8);
		if (*s)
			{
			zerrnam(nam,"bad umask",NULL,0);
			return 1;
			}
		}
	else
		{
		int whomask,op,mask;

		for (;;)
			{
			if (*s == 'u')
				s++, whomask = 0100;
			else if (*s == 'g')
				s++, whomask = 0010;
			else if (*s == 'o')
				s++, whomask = 0001;
			else
				whomask = 0111;
			op = *s++;
			if (!(op == '+' || op == '-' || op == '='))
				{
				zerrnam(nam,"bad symbolic mode operator: %c",NULL,op);
				return 1;
				}
			mask = whomask;
			if (*s == 'r')
				mask *= 04;
			else if (*s == 'w')
				mask *= 02;
			else if (*s != 'x')
				{
				zerrnam(nam,"bad symbolic mode permission: %c",NULL,*s);
				return 1;
				}
			if (op == '+')
				um |= mask;
			else if (op == '-')
				um &= ~mask;
			else /* op == '=' */
				um = (um & ~(whomask*07)) | mask;
			if (*++s == ',')
				s++;
			else
				break;
			}
		if (*s)
			{
			zerrnam(nam,"bad character in symbolic mode: %c",NULL,*s);
			return 1;
			}
		}
	umask(um);
	return 0;
}

/* type, whence, which */

int bin_whence(nam,argv,ops,func) /**/
char *nam;char **argv;char *ops;int func;
{
struct cmdnam *chn;
struct alias *a;
int retval = 0,v = ops['v'];
char *cnam;

	for (; *argv; argv++)
		{
		if (!ops['p'] && (a = gethnode(*argv,aliastab)) && a->cmd)
			{
			if (a->cmd < 0)
				printf((v) ? "%s is a reserved word\n" : "%s\n",*argv);
			else if (!v)
				puts(a->text);
			else if (a->cmd)
				printf("%s is an alias for %s\n",*argv,a->text);
			else
				printf("%s is a global alias for %s\n",*argv,a->text);
			retval = 0;
			}
		else if (!ops['p'] && (chn = gethnode(*argv,cmdnamtab)))
			{
			if (chn->type == SHFUNC)
				printf((v) ? "%s is a function\n" : "%s\n",*argv);
			else if (chn->type != BUILTIN)
				if (v)
					printf("%s is hashed to %s\n",*argv,chn->u.nam);
				else
					puts(chn->u.nam);
			else
				printf((v) ? "%s is a shell builtin\n" : "%s\n",*argv);
			retval = 0;
			}
		else if (!(cnam = findcmd(*argv)))
			{
			if (v)
				printf("%s not found\n",*argv);
			retval = 1;
			}
		else
			{
			if (v)
				printf("%s is %s\n",*argv,cnam);
			else
				puts(cnam);
			retval = 0;
			}
		}
	return retval;
}

/* cd, chdir, pushd, popd */

int bin_cd(nam,argv,ops,func) /**/
char *nam;char **argv;char *ops;int func;
{
char *dest,*new;
Lknode n;
int dd,esav = 0,t0;
char *s,buf[MAXPATHLEN];
int val,pnew = 0;

	if (func == BIN_CD && isset(AUTOPUSHD))
		func = BIN_PUSHD;
	if (!argv[0])
		if (func == BIN_CD || (func == BIN_PUSHD && isset(PUSHDTOHOME)
				|| !full(dirstack)))
			dest = ztrdup(home);
		else
			dest = getnode(dirstack);
	else if (!argv[1])
		if (argv[0][0] == '+')
			{
			dd = atoi(argv[0]+1)-1;
			if (dd < 0)
				{
				zerrnam(nam,"bad directory specification",NULL,0);
				return 1;
				}
			for (n = firstnode(dirstack); n && dd; dd--, incnode(n));
			if (!n)
				{
				zerrnam(nam,"no such entry in dir stack",NULL,0);
				return 1;
				}
			dest = remnode(dirstack,n);
			}
		else if (argv[0][0] == '-' && argv[0][1])
			{
			dd = atoi(argv[0]+1);
			for (n = lastnode(dirstack); n != (Lknode) dirstack && dd;
					dd--, n = prevnode(n));
			if (n == (Lknode) dirstack)
				{
				zerrnam(nam,"no such entry in dir stack",NULL,0);
				return 1;
				}
			dest = remnode(dirstack,n);
			}
		else
			dest = ztrdup(strcmp(argv[0],"-") ? argv[0] : oldpwd);
	else
		{
		char *u;
		int len1,len2,len3;

		if (!(u = strstr(cwd,argv[0])))
			{
			zerrnam(nam,"string not in pwd: %s",argv[1],0);
			return 1;
			}
		len1 = strlen(argv[0]);
		len2 = strlen(argv[1]);
		len3 = u-cwd;
		dest = zalloc(len3+len2+strlen(u+len1)+1);
		strncpy(dest,cwd,len3);
		strcpy(dest+len3,argv[1]);
		strcat(dest,u+len1);
		}
	if (*dest == '/')
		{
		val = chdir(new = dest);
		esav = errno;
		}
	else if (val = (chdir(dest) != -1))
		new = dest;
	else
		{
		esav = errno;
		for (t0 = 0; cdpath[t0]; t0++)
			{
			sprintf(buf,"%s/%s",cdpath[t0],dest);
			if ((val = chdir(new = buf)) != -1)
				{
				pnew = 1;
				break;
				}
			if (errno != ENOENT && errno != ENOTDIR)
				zerrnam(nam,"warning: %e: %s",buf,errno);
			}
		}
	if (val == -1 && errno == ENOENT)
		{
		if (isset(CDABLEVARS) &&
				(s = getsparamval(dest,strlen(dest))) && *s == '/')
			if (chdir(new = s) != -1)
				{
				val = 0;
				pnew = 1;
				goto goneto;
				}
		zerrnam(nam,"%e: %s",dest,esav);
		free(dest);
		return 1;
		}
goneto:
	if (val == -1)
		{
		zerrnam(nam,"%e: %s",dest,esav);
		free(dest);
		return 1;
		}
	else
		{
		List l;

		oldpwd = cwd;
		cwd = findcwd(new);
		free(dest);
		if (pnew)
			{
			printdir(cwd);
			putchar('\n');
			}
		if (func == BIN_PUSHD)
			{
			permalloc();
			pushnode(dirstack,oldpwd);
			heapalloc();
			}
		if (unset(PUSHDSILENT) && func != BIN_CD && isset(INTERACTIVE))
			pdstack();
		if (l = getshfunc("chpwd"))
			newrunlist(l);
		}
	if (dirstacksize != -1 && countnodes(dirstack) >= dirstacksize)
		{
		if (dirstacksize < 2)
			dirstacksize = 2;
		else
			free(remnode(dirstack,lastnode(dirstack)));
		}
	return 0;
}

int bin_rehash(name,argv,ops,func) /**/
char *name;char **argv;char *ops;int func;
{
	newcmdnamtab();
	return 0;
}

int bin_hash(name,argv,ops,func) /**/
char *name;char **argv;char *ops;int func;
{
struct cmdnam *chn;

	chn = zcalloc(sizeof *chn);
	chn->type = EXCMD_PREDOT;
	chn->u.nam = ztrdup(argv[1]);
	addhnode(ztrdup(argv[0]),chn,cmdnamtab,freecmdnam);
	return 0;
}

/* != 0 if s is a prefix of t */

int prefix(s,t) /**/
char *s;char *t;
{
	while (*s && *t && *s == *t) s++,t++;
	return (!*s);
}

/* convert %%, %1, %foo, %?bar? to a job number */

int getjob(s,prog) /**/
char *s;char *prog;
{
int t0,retval;

	if (*s != '%')
		goto jump;
	s++;
	if (*s == '%' || *s == '+' || !*s)
		{
		if (curjob == -1)
			{
			zerrnam(prog,"no current job",NULL,0);
			retval = -1; goto done;
			}
		retval = curjob; goto done;
		}
	if (*s == '-')
		{
		if (prevjob == -1)
			{
			zerrnam(prog,"no previous job",NULL,0);
			retval = -1; goto done;
			}
		retval = prevjob; goto done;
		}
	if (idigit(*s))
		{
		t0 = atoi(s);
		if (t0 && t0 < MAXJOB && jobtab[t0].stat && t0 != thisjob)
			{ retval = t0; goto done; }
		zerrnam(prog,"no such job",NULL,0);
		retval = -1; goto done;
		}
	if (*s == '?')
		{
		struct process *pn;

		for (t0 = MAXJOB-1; t0 >= 0; t0--)
			if (jobtab[t0].stat && t0 != thisjob)
				for (pn = jobtab[t0].procs; pn; pn = pn->next)
					if (strstr(pn->text,s+1))
						{ retval = t0; goto done; }
		zerrnam(prog,"job not found: %s",s,0);
		retval = -1; goto done;
		}
jump:
	if ((t0 = findjobnam(s)) != -1)
		{ retval = t0; goto done; }
	zerrnam(prog,"job not found: %s",s,0);
	retval = -1;
done:
	return retval;
}

/* find a job named s */

int findjobnam(s) /**/
char *s;
{
int t0;

	for (t0 = MAXJOB-1; t0 >= 0; t0--)
		if (jobtab[t0].stat && jobtab[t0].procs && t0 != thisjob && 
				prefix(s,jobtab[t0].procs->text))
			return t0;
	return -1;
}

int bin_kill(nam,argv,ops,func) /**/
char *nam;char **argv;char *ops;int func;
{
int sig = SIGTERM;

	if (*argv && **argv == '-')
		{
		if (idigit((*argv)[1]))
			sig = atoi(*argv+1);
		else
			{
			if ((*argv)[1] == 'l' && (*argv)[2] == '\0')
				{
				printf("%s",sigs[1]);
				for (sig = 2; sig != SIGCOUNT; sig++)
					printf(" %s",sigs[sig]);
				putchar('\n');
				return 0;
				}
			for (sig = 0; sig != SIGCOUNT; sig++)
				if (!strcmp(sigs[sig],*argv+1))
					break;
			if (sig == SIGCOUNT)
				{
				zerrnam(nam,"unknown signal: SIG%s",*argv+1,0);
				zerrnam(nam,"type kill -l for a List of signals",NULL,0);
				return 1;
				}
			}
		argv++;
		}
	while (*argv)
		{
		if (**argv == '%')
			{
			int p = getjob(*argv,"kill");

			if (killjb(jobtab+p,sig) == -1)
				{
				zerrnam("kill","kill failed: %e",NULL,errno);
				return 1;
				}
			if (jobtab[p].stat & STAT_STOPPED && sig == SIGCONT)
				jobtab[p].stat &= ~STAT_STOPPED;
			if (sig != SIGKILL && sig != SIGCONT)
				killpg(jobtab[p].gleader,SIGCONT);
			}
		else
			if (kill(atoi(*argv),sig) == -1)
				{
				zerrnam("kill","kill failed: %e",NULL,errno);
				return 1;
				}
		argv++;
		}
	return 0;
}

static char *recs[] = {
	"cputime","filesize","datasize","stacksize","coredumpsize",
	"resident","descriptors"
	};

int bin_limit(nam,argv,ops,func) /**/
char *nam;char **argv;char *ops;int func;
{
char *s;
int hard = ops['h'],t0,lim;
long val;

	if (ops['s'])
		{
		if (*argv)
			zerrnam(nam,"arguments after -s ignored",NULL,0);
		for (t0 = 0; t0 != RLIM_NLIMITS; t0++)
			if (setrlimit(t0,limits+t0) < 0)
				zerrnam(nam,"setrlimit failed: %e",NULL,errno);
		return 0;
		}
	if (!*argv)
		{
		showlimits(hard,-1);
		return 0;
		}
	while (s = *argv++)
		{
		for (lim = -1, t0 = 0; t0 != RLIM_NLIMITS; t0++)
			if (!strncmp(recs[t0],s,strlen(s)))
				{
				if (lim != -1)
					lim = -2;
				else
					lim = t0;
				}
		if (lim < 0)
			{
			zerrnam("limit",
				(lim == -2) ? "ambiguous resource specification: %s"
								: "no such resource: %s",s,0);
			return 1;
			}
		if (!(s = *argv++))
			{
			showlimits(hard,lim);
			return 0;
			}
		if (!lim)
			{
			val = strtol(s,&s,10);
			if (*s)
				if ((*s == 'h' || *s == 'H') && !s[1])
					val *= 3600L;
				else if ((*s == 'm' || *s == 'M') && !s[1])
					val *= 60L;
				else if (*s == ':')
					val = val*60+strtol(s+1,&s,10);
				else
					{
					zerrnam("limit","unknown scaling factor: %s",s,0);
					return 1;
					}
			}
#ifdef RLIMIT_NOFILE
		else if (lim == RLIMIT_NOFILE)
			val = strtol(s,&s,10);
#endif
		else
			{
			val = strtol(s,&s,10);
			if (!*s || ((*s == 'k' || *s == 'K') && !s[1]))
				val *= 1024L;
			else if ((*s == 'M' || *s == 'm') && !s[1])
				val *= 1024L*1024;
			else
				{
				zerrnam("limit","unknown scaling factor: %s",s,0);
				return 1;
				}
			}
		if (hard)
			if (val > limits[lim].rlim_max && geteuid())
				{
				zerrnam("limit","can't raise hard limits",NULL,0);
				return 1;
				}
			else
				{
				limits[lim].rlim_max = val;
				if (limits[lim].rlim_max < limits[lim].rlim_cur)
					limits[lim].rlim_cur = limits[lim].rlim_max;
				}
		else
			if (val > limits[lim].rlim_max)
				{
				zerrnam("limit","limit exceeds hard limit",NULL,0);
				return 1;
				}
			else
				limits[lim].rlim_cur = val;
		}
	return 0;
}

int bin_unlimit(nam,argv,ops,func) /**/
char *nam;char **argv;char *ops;int func;
{
int hard = ops['h'],t0,lim;

	if (hard && geteuid())
		{
		zerrnam("unlimit","can't remove hard limits",NULL,0);
		return 1;
		}
	if (!*argv)
		{
		for (t0 = 0; t0 != RLIM_NLIMITS; t0++)
			{
			if (hard)
				limits[t0].rlim_max = RLIM_INFINITY;
			else
				limits[t0].rlim_cur = limits[t0].rlim_max;
			}
		return 0;
		}
	for (; *argv; argv++)
		{
		for (lim = -1, t0 = 0; t0 != RLIM_NLIMITS; t0++)
			if (!strncmp(recs[t0],*argv,strlen(*argv)))
				{
				if (lim != -1)
					lim = -2;
				else
					lim = t0;
				}
		if (lim < 0)
			{
			zerrnam("unlimit",
				(lim == -2) ? "ambiguous resource specification: %s"
								: "no such resource: %s",*argv,0);
			return 1;
			}
		if (hard)
			limits[lim].rlim_max = RLIM_INFINITY;
		else
			limits[lim].rlim_cur = limits[lim].rlim_max;
		}
	return 0;
}

void showlimits(hard,lim) /**/
int hard;int lim;
{
int t0;
long val;

	for (t0 = 0; t0 != RLIM_NLIMITS; t0++)
		if (t0 == lim || lim == -1)
			{
			printf("%-16s",recs[t0]);
			val = (hard) ? limits[t0].rlim_max : limits[t0].rlim_cur;
			if (val == RLIM_INFINITY)
				printf("unlimited\n");
			else if (!t0)
				printf("%d:%02d:%02d\n",(int) (val/3600),
					(int) (val/60) % 60,(int) (val % 60));
#ifdef RLIMIT_NOFILE
			else if (t0 == RLIMIT_NOFILE)
				printf("%d\n",(int) val);
#endif
			else if (val >= 1024L*1024L)
				printf("%ldMb\n",val/(1024L*1024L));
			else
				printf("%ldKb\n",val/1024L);
			}
}

int bin_sched(nam,argv,ops,func) /**/
char *nam;char **argv;char *ops;int func;
{
char *s = *argv++;
time_t t;
long h,m;
struct tm *tm;
struct schedcmd *sch,*sch2,*schl;
int t0;

	if (s && *s == '-')
		{
		t0 = atoi(s+1);

		if (!t0)
			{
			zerrnam("sched","usage for delete: sched -<item#>.",NULL,0);
			return 1;
			}
		for (schl = (struct schedcmd *) &schedcmds, sch = schedcmds, t0--;
				sch && t0; sch = (schl = sch)->next, t0--);
		if (!sch)
			{
			zerrnam("sched","not that many entries",NULL,0);
			return 1;
			}
		schl->next = sch->next;
		free(sch->cmd);
		free(sch);
		return 0;
		}
	if (!s)
		{
		char tbuf[40];

		for (t0 = 1, sch = schedcmds; sch; sch = sch->next,t0++)
			{
			t = sch->time;
			tm = localtime(&t);
			ztrftime(tbuf,20,"%a %b %e %k:%M:%S",tm);
			printf("%3d %s %s\n",t0,tbuf,sch->cmd);
			}
		return 0;
		}
	else if (!*argv)
		{
		zerrnam("sched","not enough arguments",NULL,0);
		return 1;
		}
	if (*s == '+')
		{
		h = strtol(s+1,&s,10);
		if (*s != ':')
			{
			zerrnam("sched","bad time specifier",NULL,0);
			return 1;
			}
		m = strtol(s+1,&s,10);
		if (*s)
			{
			zerrnam("sched","bad time specifier",NULL,0);
			return 1;
			}
		t = time(NULL)+h*3600+m*60;
		}
	else
		{
		h = strtol(s,&s,10);
		if (*s != ':')
			{
			zerrnam("sched","bad time specifier",NULL,0);
			return 1;
			}
		m = strtol(s+1,&s,10);
		if (*s && *s != 'a' && *s != 'p')
			{
			zerrnam("sched","bad time specifier",NULL,0);
			return 1;
			}
		t = time(NULL);
		tm = localtime(&t);
		t -= tm->tm_sec+tm->tm_min*60+tm->tm_hour*3600;
		if (*s == 'p')
			h += 12;
		t += h*3600+m*60;
		if (t < time(NULL))
			t += 3600*24;
		}
	sch = zcalloc(sizeof *sch);
	sch->time = t;
	sch->cmd = spacejoin(argv);
	sch->next = NULL;
	for (sch2 = (struct schedcmd *) &schedcmds; sch2->next; sch2 = sch2->next);
	sch2->next = sch;
	return 0;
}

int bin_eval(nam,argv,ops,func) /**/
char *nam;char **argv;char *ops;int func;
{
char *s = spacejoin(argv);
List list;

	hungets(s);
	free(s);
	strinbeg();
	if (!(list = parlist()))
		{
		hflush();
		strinend();
		return 1;
		}
	strinend();
	runlist(list);
	return lastval;
}

/* get the history event associated with s */

int fcgetcomm(s) /**/
char *s;
{
int cmd;

	if (cmd = atoi(s))
		{
		if (cmd < 0)
			cmd = curhist+cmd+1;
		return cmd;
		}
	cmd = hcomsearch(s);
	if (cmd == -1)
		zerrnam("fc","event not found: %s",s,0);
	return cmd;
}

/* perform old=new substituion */

int fcsubs(sp,sub) /**/
char **sp;struct asgment *sub;
{
char *s1,*s2,*s3,*s4,*s = *sp,*s5;
int subbed = 0;

	while (sub)
		{
		s1 = sub->name;
		s2 = sub->value;
		sub = sub->next;
		s5 = s;
		while (s3 = (char *) strstr(s5,s1))
			{
			s4 = alloc(1+(s3-s)+strlen(s2)+strlen(s3+strlen(s1)));
			strncpy(s4,s,s3-s);
			s4[s3-s] = '\0';
			strcat(s4,s2);
			s5 = s4+strlen(s4);
			strcat(s4,s3+strlen(s1));
			s = s4;
			subbed = 1;
			}
		}
	*sp = s;
	return subbed;
}

/* print a series of history events to a file */

int fclist(f,n,r,first,last,subs) /**/
FILE *f;int n;int r;int first;int last;struct asgment *subs;
{
int done = 0,ct;
Lknode node;
char *s;

	if (!subs)
		done = 1;
	last -= first;
	first -= firsthist;
	if (r)
		first += last;
	for (node = firstnode(histlist),ct = first; ct && node;
		incnode(node), ct--);
	first += firsthist;
	while (last-- >= 0)
		{
		if (!node)
			{
			zerrnam("fc","no such event: %d",NULL,first);
			return 1;
			}
		s = makehstr(getdata(node));
		done |= fcsubs(&s,subs);
		if (n)
			fprintf(f,"%5d  ",first);
		if (f == stdout)
			{
			niceprintf(s,f);
			putc('\n',f);
			}
		else
			fprintf(f,"%s\n",s);
		node = (r) ? prevnode(node) : nextnode(node);
		(r) ? first-- : first++;
		}
	if (f != stdout)
		fclose(f);
	if (!done)
		{
		zerrnam("fc","no substitutions performed",NULL,0);
		return 1;
		}
	return 0;
}

int fcedit(ename,fn) /**/
char *ename;char *fn;
{
	if (!strcmp(ename,"-"))
		return 1;
	return !zyztem(ename,fn);
}

/* fc, history, r */

int bin_fc(nam,argv,ops,func) /**/
char *nam;char **argv;char *ops;int func;
{
int first = -1,last = -1,retval,minflag = 0;
char *s;
struct asgment *asgf = NULL,*asgl = NULL;

	if (!interact)
		{
		zerrnam("fc","not interactive shell",NULL,0);
		return 1;
		}
	remhist();
	while (*argv && equalsplit(*argv,&s))
		{
		struct asgment *a = alloc(sizeof *a);

		if (!asgf)
			asgf = asgl = a;
		else
			{
			asgl->next = a;
			asgl = a;
			}
		a->name = *argv;
		a->value = s;
		argv++;
		}
	if (*argv)
		{
		minflag = **argv == '-';
		first = fcgetcomm(*argv);
		if (first == -1)
			return 1;
		argv++;
		}
	if (*argv)
		{
		last = fcgetcomm(*argv);
		if (last == -1)
			return 1;
		argv++;
		}
	if (*argv)
		{
		zerrnam("fc","too many arguments",NULL,0);
		return 1;
		}
	if (first == -1)
		{
		first = (ops['l']) ? curhist-16 : curhist;
		if (last == -1)
			last = (ops['l']) ? curhist : first;
		}
	if (first < firsthist)
		first = firsthist;
	if (last == -1)
		last = (minflag) ? curhist : first;
	if (ops['l'])
		retval = fclist(stdout,!ops['n'],ops['r'],first,last,asgf);
	else
		{
		FILE *out;
		char *fil = gettemp();

		out = fopen(fil,"w");
		if (!out)
			zerrnam("fc","can't open temp file: %e",NULL,errno);
		else
			{
			retval = 1;
			if (!fclist(out,0,ops['r'],first,last,asgf))
				if (fcedit(auxdata ? auxdata : DEFFCEDIT,fil))
					if (stuff(fil))
						zerrnam("fc","%e: %s",s,errno);
					else
						retval = 0;
			}
		unlink(fil);
		}
	return retval;
}

int bin_suspend(name,argv,ops,func) /**/
char *name;char **argv;char *ops;int func;
{
	if (islogin && !ops['f'])
		{
		zerrnam(name,"can't suspend login shell",NULL,0);
		return 1;
		}
	if (jobbing)
		signal(SIGTSTP,SIG_DFL);
	kill(0,SIGTSTP);
	if (jobbing)
		signal(SIGTSTP,SIG_IGN);
	return 0;
}

int bin_alias(name,argv,ops,func) /**/
char *name;char **argv;char *ops;int func;
{
struct alias *an;
struct asgment *asg;
int incm = !(ops['a'] || ops['g']),ret = 0;

	showflag = !incm;
	if (!*argv)
		listhtable(aliastab,(HFunc) printalias);
	else while (asg = getasg(*argv++))
		{
		if (asg->value)
			addhnode(ztrdup(asg->name),mkanode(ztrdup(asg->value),incm),
				aliastab,freeanode);
		else if (an = gethnode(asg->name,aliastab))
			printalias(asg->name,an);
		else
			ret = 1;
		}
	return ret;
}

/* print an alias; used with listhtable */

void printalias(s,a) /**/
char *s;struct alias *a;
{
	if (a->cmd >= 0 && !(showflag && a->cmd))
		printf("%s=%s\n",s,a->text);
}

/* print a param; used with listhtable */

void printparam(s,p) /**/
char *s;Param p;
{
	if (showflag)
		if (showflag == PMFLAG_SPECIAL)
			{
			if (p->flags & showflag)
				return;
			}
		else if (!(p->flags & showflag))
			return;
	if (!showflag)
		{
		int fgs = p->flags;

		if (fgs & PMFLAG_i) printf("integer ");
		if (fgs & PMFLAG_A) printf("array ");
		if (fgs & PMFLAG_L) printf("left justified %d ",p->ct);
		if (fgs & PMFLAG_R) printf("right justified %d ",p->ct);
		if (fgs & PMFLAG_Z) printf("zero filled %d ",p->ct);
		if (fgs & PMFLAG_l) printf("lowercase ");
		if (fgs & PMFLAG_u) printf("uppercase ");
		if (fgs & PMFLAG_r) printf("readonly ");
		if (fgs & PMFLAG_t) printf("tagged ");
		if (fgs & PMFLAG_x) printf("exported ");
		}
	if (showflag2)
		printf("%s\n",s);
	else
		{
		char *t,**u;

		printf("%s=",s);
		switch (p->flags & PMTYPE)
			{
			case PMFLAG_s:
				if (p->gets.cfn && (t = p->gets.cfn(p)))
					puts(t);
				else
					putchar('\n');
				break;
			case PMFLAG_i: printf("%ld\n",p->gets.ifn(p)); break;
			case PMFLAG_A:
				putchar('(');
				u = p->gets.afn(p);
				if (!*u)
					printf(")\n");
				else
					{
					while (u[1])
						printf("%s ",*u++);
					printf("%s)\n",*u);
					}
				break;
			}
		}
}

/* autoload, declare, export, functions, integer, local, readonly, typeset */

int bin_typeset(name,argv,ops,func) /**/
char *name;char **argv;char *ops;int func;
{
int on = 0,off = 0,roff,bit = 1,retcode = 0;
char *optstr = "LRZilurtx";
struct param *pm;
struct asgment *asg;

	for (; *optstr; optstr++,bit <<= 1)
		if (ops[*optstr] == 1)
			on |= bit;
		else if (ops[*optstr] == -1)
			off |= bit;
	roff = off;
	if (ops['f'])
		{
		on &= PMFLAG_t|PMFLAG_u;
		off &= PMFLAG_t|PMFLAG_u;
		showflag = (ops['f'] == 1);
		if (ops['@'] && (off || (on & ~(PMFLAG_u|PMFLAG_t))))
			{
			zerrnam(name,"invalid option(s)",NULL,0);
			return 1;
			}
		showflag2 = 0;
		if (!*argv)
			{
			showflag2 = off|on;
			listhtable(cmdnamtab,(HFunc) pshfunc);
			}
		else for (; *argv; argv++)
			{
			Cmdnam cc;

			if ((cc = gethnode(*argv,cmdnamtab)) && cc->type == SHFUNC)
				if (on)
					cc->flags |= on;
				else
					pshfunc(*argv,cc);
			else if (on & PMFLAG_u)
				{
				cc = zcalloc(sizeof *cc);
				cc->type = SHFUNC;
				cc->flags = on;
				addhnode(ztrdup(*argv),cc,cmdnamtab,freecmdnam);
				}
			else
				retcode = 1;
			}
		return retcode;
		}
	if (on & PMFLAG_L)
		off |= PMFLAG_R;
	if (on & PMFLAG_R)
		off |= PMFLAG_L;
	if (on & PMFLAG_u)
		off |= PMFLAG_l;
	if (on & PMFLAG_l)
		off |= PMFLAG_u;
	on &= ~off;
	if (!*argv)
		{
		showflag = on|off;
		showflag2 = roff;
		listhtable(paramtab,(HFunc) printparam);
		}
	else while (asg = getasg(*argv++))
		{
		pm = gethnode(asg->name,paramtab);
		if (pm)
			{
			if (!(pm->flags & PMFLAG_r))
				{
				if (!on && !roff && !asg->value)
					{
					printparam(asg->name,pm);
					continue;
					}
				pm->flags = (pm->flags | on) & ~off;
				if ((on & (PMFLAG_L | PMFLAG_R | PMFLAG_Z | PMFLAG_i)) 
						&& (pmtype(pm) != PMFLAG_A))
					pm->ct = auxlen;
				if (pmtype(pm) != PMFLAG_A)
					{
					if (pm->flags & PMFLAG_x)
						{
						if (!pm->env)
							pm->env = addenv(asg->name,
								(asg->value) ? asg->value : getsparam(asg->name));
						}
					else if (pm->env)
						delenv(pm->env);
					if (asg->value)
						setsparam(asg->name,ztrdup(asg->value));
					}
				}
			}
		else
			{
			if (locallist && !(on & PMFLAG_x))
				{
				permalloc();
				addnode(locallist,ztrdup(asg->name));
				heapalloc();
				}
			createparam(ztrdup(asg->name),
				ztrdup((asg->value) ? asg->value : ""),on);
			pm = gethnode(asg->name,paramtab);
			pm->ct = auxlen;
			}
		}
	return 0;
}

/* print s with escape sequences */

int escputs(s) /**/
char *s;
{
int nnl = 0;

	for (; *s; s++)
		if (*s == '\\' && s[1])
			switch (*++s)
				{
				case 'b': putchar('\b'); break;
				case 'c': nnl = 1; break;
				case 'f': putchar('\f'); break;
				case 'n': putchar('\n'); break;
				case 'r': putchar('\r'); break;
				case 't': putchar('\t'); break;
				case 'v': putchar('\v'); break;
				case '\\': putchar('\\'); break;
				case '0': putchar(strtol(s,&s,8)); s--; break;
				default: putchar('\\'); putchar(*s); break;
				}
		else
			putchar(*s);
	return nnl;
}

/* echo, print, pushln */

int bin_print(name,args,ops,func) /**/
char *name;char **args;char *ops;int func;
{
int nnl = 0;

	if (ops['z'])
		{
		permalloc();
		pushnode(bufstack,spacejoin(args));
		heapalloc();
		return 0;
		}
	if (ops['s'])
		{
		addnode(histlist,join(args,HISTSPACE));
		curhist++;
		return 0;
		}
	if (ops['R'])
		ops['r'] = 1;
	for (; *args; args++)
		{
		if (ops['r'])
			fputs(*args,stdout);
		else
			nnl |= escputs(*args);
		if (args[1])
			putchar(ops['l'] ? '\n' : ops['0'] ? '\0' : ' ');
		}
	if (!(ops['n'] || nnl))
		putchar(ops['0'] ? '\0' : '\n');
	return 0;
}

int bin_dirs(name,argv,ops,func) /**/
char *name;char **argv;char *ops;int func;
{
Lklist l;

	if (ops['v'])
		{
		Lknode node;
		int t0 = 1;

		printf("0\t");
		printdir(cwd);
		for (node = firstnode(dirstack); node; incnode(node))
			{
			printf("\n%d\t",t0++);
			printdir(getdata(node));
			}
		putchar('\n');
		return 0;
		}
	if (!*argv)
		{
		pdstack();
		return 0;
		}
	permalloc();
	l = newlist();
	if (!*argv)
		{
		heapalloc();
		return 0;
		}
	while (*argv)
		addnode(l,ztrdup(*argv++));
	freetable(dirstack,freestr);
	dirstack = l;
	heapalloc();
	return 0;
}

int bin_unalias(name,argv,ops,func) /**/
char *name;char **argv;char *ops;int func;
{
int ret = 0;
void *dat;

	while (*argv)
		{
		if (dat = remhnode(*argv++,aliastab))
			freeanode(dat);
		else
			ret = 1;
		}
	return ret;
}

/* disable, unfunction, unhash */

int bin_unhash(name,argv,ops,func) /**/
char *name;char **argv;char *ops;int func;
{
int retval = 0;
void *dat;

	while (*argv)
		{
		if (!strncmp(*argv,"TRAP",4))
			unsettrap(getsignum(*argv+4));
		if (dat = remhnode(*argv++,cmdnamtab))
			freecmdnam(dat);
		else
			retval = 1;
		}
	return retval;
}

int bin_unset(name,argv,ops,func) /**/
char *name;char **argv;char *ops;int func;
{
int retval = 0;
char *s;

	while (s = *argv++)
		if (gethnode(s,paramtab))
			unsetparam(s);
		else
			retval = 1;
	return retval;
}

static char *zbuf;

int zread() /**/
{
char cc;

	if (zbuf)
		return *zbuf++;
	if (read(0,&cc,1) != 1)
		return EOF;
	return cc;
}

int bin_read(name,args,ops,func) /**/
char *name;char **args;char *ops;int func;
{
char *reply,*pmpt;
int bsiz,c,gotnl = 0;
char *buf,*bptr;
char cc;

	if (*args)
		reply = *args++;
	else
		reply = "REPLY";
	if (ops['z'])
		zbuf = (full(bufstack)) ? (char *) getnode(bufstack) : ztrdup("");
	else
		zbuf = NULL;
	if (isatty(0))
		{
		for (pmpt = reply; *pmpt && *pmpt != '?'; pmpt++);
		if (*pmpt++)
			{
			write(2,pmpt,strlen(pmpt));
			pmpt[-1] = '\0';
			}
		}
	while (*args)
		{
		buf = bptr = zalloc(bsiz = 64);
redo:
		for(;;)
			{
			if (gotnl)
				break;
			c = zread();
			if (!ops['r'] && c == '\n' && bptr != buf && bptr[-1] == '\\')
				{
				bptr--;
				continue;
				}
			if (c == EOF || iblank(c))
				break;
			*bptr++ = c;
			if (bptr == buf+bsiz)
				{
				buf = realloc(buf,bsiz *= 2);
				bptr = buf+(bsiz/2);
				}
			}
		if (c == EOF)
			return 1;
		if (c == '\n')
			gotnl = 1;
		if (bptr == buf)
			goto redo;
		*bptr = '\0';
		setsparam(reply,buf);
		reply = *args++;
		}
	buf = bptr = zalloc(bsiz = 64);
	if (!gotnl)
		for (;;)
			{
			c = zread();
			if (!ops['r'] && c == '\n' && bptr != buf && bptr[-1] == '\\')
				{
				bptr--;
				continue;
				}
			if (c == EOF || c == '\n')
				break;
			*bptr++ = c;
			if (bptr == buf+bsiz)
				{
				buf = realloc(buf,bsiz *= 2);
				bptr = buf+(bsiz/2);
				}
			}
	*bptr = '\0';
	if (c == EOF)
		return 1;
	setsparam(reply,buf);
	return 0;
}

int bin_vared(name,args,ops,func) /**/
char *name;char **args;char *ops;int func;
{
char *s,*t;

	if (!(s = getsparam(args[0])))
		{
		zerrnam(name,"no such variable: %s",args[0],0);
		return 1;
		}
	permalloc();
	pushnode(bufstack,ztrdup(s));
	heapalloc();
	t = zleread("> ",NULL,2);
	if (!t || errflag)
		return 1;
	if (t[strlen(t)-1] == '\n')
		t[strlen(t)-1] = '\0';
	setsparam(args[0],t);
	return 0;
}

#define fset(X) (flags & X)

/* execute a builtin handler function after parsing the arguments */

int execbin(args,cnode) /**/
Lklist args;Cmdnam cnode;
{
struct bincmd *b;
char ops[128],*arg,*pp,*name,**argv,**oargv;
int t0,flags,sense,argc = 0,retval,op;
void *data;
Lknode n;

	auxdata = NULL;
	auxlen = 0;
	for (t0 = 0; t0 != 128; t0++)
		ops[t0] = 0;
	name = ugetnode(args);
	b = builtins+cnode->u.binnum;

/* the 'builtin' builtin is handled specially */

	if (b->funcid == BIN_BUILTIN)
		{
		if (!(name = ugetnode(args)))
			{
			zerrnam("builtin","command name expected",NULL,0);
			return 1;
			}
		for (t0 = 0, b = builtins; b->name; b++,t0++)
			if (!strcmp(name,b->name))
				break;
		if (!b->name)
			{
			zerrnam("builtin","no such builtin: %s",name,0);
			return 1;
			}
		}
	flags = b->flags;
	arg = ugetnode(args);
	if (b->optstr)
		while (arg &&
				((sense = *arg == '-') ||  fset(BINF_PLUSOPTS) && *arg == '+') &&
				!atoi(arg))
			{
			pp = arg;
			if (!arg[1])
				{
				ops['-'] = 1;
				if (!sense)
					ops['+'] = 1;
				}
			else
				ops['@'] = 1;
			while (*++arg)
				if (strchr(b->optstr,op = *arg))
					ops[*arg] = (sense) ? 1 : -1;
				else
					break;
			if (*arg)
				{
				zerr("bad option: %c",NULL,*arg);
				return 1;
				}
			arg = ugetnode(args);
			if (fset(BINF_SETOPTS) && op == 'o')
				{
				int c = optlookup(arg);

				if (c == -1)
					{
					zerr("bad option: %s",arg,0);
					return 1;
					}
				else
					{
					ops[c] = ops['o'];
					arg = ugetnode(args);
					}
				}
			if ((fset(BINF_PRINTOPTS) && ops['R']) || ops['-'])
				break;
			if (fset(BINF_SETOPTS) && ops['A'])
				{
				auxdata = arg;
				arg = ugetnode(args);
				break;
				}
			if (fset(BINF_FCOPTS) && op == 'e')
				{
				auxdata = arg;
				arg = ugetnode(args);
				}
			if (fset(BINF_TYPEOPT) && (op == 'L' || op == 'R' ||
					op == 'Z' || op == 'i') && arg && idigit(*arg))
				{
				auxlen = atoi(arg);
				arg = ugetnode(args);
				}
			}
	if (fset(BINF_R))
		auxdata = "-";
	if (pp = b->defopts)
		while (*pp)
			ops[*pp++] = 1;
	if (arg)
		{
		argc = 1;
		n = firstnode(args);
		while (n)
			argc++,incnode(n);
		}
	oargv = argv = (char **) ncalloc(sizeof(char **) * (argc+1));
	if (*argv++ = arg)
		while (*argv++ = ugetnode(args));
	argv = oargv;
	data = argv;
	if (errflag)
		return 1;
	if (argc < b->minargs || (argc > b->maxargs && b->maxargs != -1))
		{
		zerrnam(name,(argc < b->minargs)
			? "not enough arguments" : "too many arguments",NULL,0);
		retval = 1;
		}
	else
		retval = (*(b->handlerfunc))(name,data,ops,b->funcid);
	return retval;
}

struct asgment *getasg(s) /**/
char *s;
{
static struct asgment asg;

	if (!s)
		return NULL;
	asg.name = s;
	for (; *s && *s != '='; s++);
	if (*s)
		{
		*s = '\0';
		asg.value = s+1;
		}
	else
		asg.value = NULL;
	return &asg;
}

/* ., source */

int bin_dot(name,argv,ops,func) /**/
char *name;char **argv;char *ops;int func;
{
char **old,*old0;
int ret;
char buf[MAXPATHLEN];
char *s,**t,*enam;

	if (!*argv)
		return 0;
	old = pparams;
	old0 = argzero;
	permalloc();
	pparams = arrdup(argv+1);
	heapalloc();
	enam = argzero = ztrdup(*argv);
	errno = ENOENT;
	ret = 1;
	for (s = argzero; *s; s++)
		if (*s == '/')
			{
			ret = source(argzero);
			break;
			}
	if (!*s)
		{
		for (t = path; *t; t++)
			if ((*t)[0] == '.' && !(*t)[1])
				{
				ret = source(argzero);
				break;
				}
			else
				{
				sprintf(buf,"%s/%s",*t,argzero);
				if (access(buf,F_OK) == 0)
					{
					ret = source(enam = buf);
					break;
					}
				}
		if (!*t && access(argzero,F_OK) == 0)
			ret = source(enam = argzero);
		}
	freearray(pparams);
	pparams = old;
	if (ret)
		zerrnam(name,"%e: %s",enam,errno);
	free(argzero);
	argzero = old0;
	return ret;
}

int bin_set(name,argv,ops,func) /**/
char *name;char **argv;char *ops;int func;
{
struct option *opp;
char **x;

	if (!ops['@'] && !*argv)
		{
		showflag = PMFLAG_SPECIAL;
		showflag2 = ops['+'];
		listhtable(paramtab,(HFunc) printparam);
		}
   for (opp = optns; opp->name; opp++)
      if (ops[opp->id] == 1)
         opts[opp->id] = OPT_SET;
      else if (ops[opp->id] == 2)
         opts[opp->id] = OPT_UNSET;
	if (!*argv && !ops['-'])
		return 0;
	permalloc();
	x = arrdup(argv);
	heapalloc();
	if (ops['A'])
		setaparam(auxdata,x);
	else
		{
		freearray(pparams);
		permalloc();
		pparams = x;
		heapalloc();
		}
	return 0;
}

#define pttime(X) printf("%dm%ds",(X)/3600,(X)/60%60)

int bin_times(name,argv,ops,func) /**/
char *name;char **argv;char *ops;int func;
{
struct tms buf;

	if (times(&buf))
		return 1;
	pttime(buf.tms_utime);
	putchar(' ');
	pttime(buf.tms_stime);
	putchar('\n');
	pttime(buf.tms_cutime);
	putchar(' ');
	pttime(buf.tms_cstime);
	putchar('\n');
	return 0;
}

int bin_getopts(name,argv,ops,func) /**/
char *name;char **argv;char *ops;int func;
{
char *optstr = *argv++,*var = *argv++;
char **args = (*argv) ? argv : pparams;
static int optcind = 1,quiet;
char *str,optbuf[3],*opch = optbuf+1;

	optbuf[0] = '+'; optbuf[1] = optbuf[2] = '\0';
	if (optarg)
		free(optarg);
	optarg = ztrdup("");
	setsparam(var,ztrdup(""));
	if (*optstr == ':')
		{
		quiet = 1;
		optstr++;
		}
	if (optind >= arrlen(args))
		return 1;
	str = args[optind-1];
	if (*str != '+' && *str != '-' || optcind >= strlen(str) ||
			!strcmp("--",str))
		{
		if (*str == '+' || *str == '-')
			optind++;
		optcind = 0;
		return 1;
		}
	if (!optcind)
		optcind = 1;
	*opch = str[optcind++];
	if (!args[optcind])
		{
		optind++;
		optcind = 0;
		}
	for (; *optstr; optstr++)
		if (*opch == *optstr)
			break;
	if (!*optstr)
		{
		if (quiet)
			{
			optarg = ztrdup(opch);
			setsparam(var,ztrdup("?"));
			return 0;
			}
		zerr("bad option: %c",NULL,*opch);
		return 1;
		}
	setsparam(var,ztrdup(opch-(*str == '+')));
	if (optstr[1] == ':')
		{
		if (!args[optind-1])
			{
			if (quiet)
				{
				optarg = ztrdup(opch);
				setsparam(var,ztrdup(":"));
				return 0;
				}
			zerr("argument expected after %c option",NULL,*opch);
			return 1;
			}
		free(optarg);
		optarg = ztrdup(args[optind-1]+optcind);
		optind++;
		optcind = 0;
		}
	return 0;
}

/* get a signal number from a string */

int getsignum(s) /**/
char *s;
{
int x = atoi(s),t0;

	if (idigit(*s) && x >= 0 && x < VSIGCOUNT)
		return x;
	for (t0 = 0; t0 != VSIGCOUNT; t0++)
		if (!strcmp(s,sigs[t0]))
			return t0;
	return -1;
}

int bin_trap(name,argv,ops,func) /**/
char *name;char **argv;char *ops;int func;
{
List l;
char *arg;

	if (!*argv)
		{
		int t0;

		for (t0 = 0; t0 != VSIGCOUNT; t0++)
			if (sigtrapped[t0])
				if (!sigfuncs[t0])
					printf("TRAP%s () {}\n",sigs[t0]);
				else
					{
					char *s = gettext((void *) sigfuncs[t0],1);
					printf("TRAP%s () {\n\t%s\n}\n",sigs[t0],s);
					free(s);
					}
		return 0;
		}
	if (!strcmp(*argv,"-"))
		{
		int t0;

		argv++;
		if (*argv)
			for (t0 = 0; t0 != VSIGCOUNT; t0++)
				unsettrap(t0);
		else
			while (*argv)
				unsettrap(getsignum(*argv++));
		return 0;
		}
	arg = *argv++;
	if (!*arg)
		l = NULL;
	else if (!(l = parselstring(arg)))
		{
		zerrnam(name,"couldn't parse trap command",NULL,0);
		popheap();
		return 1;
		}
	for (; *argv; argv++)
		{
		int sg = getsignum(*argv);
		if (sg == -1)
			{
			zerrnam(name,"undefined signal: %s",*argv,0);
			break;
			}
		settrap(sg,l);
		}
	if (l)
		popheap();
	return errflag;
}

void printulimit(lim,hard) /**/
int lim;int hard;
{
long t0;

	t0 = (hard) ? limits[lim].rlim_max : limits[lim].rlim_cur;
	switch (lim)
		{
		case RLIMIT_CPU: printf("cpu time (seconds)         "); break;
		case RLIMIT_FSIZE: printf("file size (blocks)         "); t0 /= 512; break;
		case RLIMIT_DATA: printf("data seg size (kbytes)     "); t0 /= 1024; break;
		case RLIMIT_STACK: printf("stack size (kbytes)        "); t0 /= 1024; break;
		case RLIMIT_CORE: printf("core file size (blocks)    "); t0 /= 512; break;
		case RLIMIT_RSS: printf("resident set size (kbytes) "); t0 /= 1024; break;
#ifdef RLIMIT_NOFILE
		case RLIMIT_NOFILE: printf("file descriptors           "); break;
#endif
		}
	printf("%ld\n",t0);
}

int bin_ulimit(name,argv,ops,func) /**/
char *name;char **argv;char *ops;int func;
{
int res,hard;

	hard = ops['H'];
	if (ops['a'] || !ops['@'])
		res = -1;
	else if (ops['t'])
		res = RLIMIT_CPU;
	else if (ops['f'])
		res = RLIMIT_FSIZE;
	else if (ops['d'])
		res = RLIMIT_DATA;
	else if (ops['s'])
		res = RLIMIT_STACK;
	else if (ops['c'])
		res = RLIMIT_CORE;
	else if (ops['m'])
		res = RLIMIT_RSS;
#ifdef RLIMIT_NOFILE
	else if (ops['n'])
		res = RLIMIT_NOFILE;
#endif
	else
		{
		zerrnam(name,"no such limit",NULL,0);
		return 1;
		}
	if (res == -1)
		if (*argv)
			{
			zerrnam(name,"no arguments required after -a",NULL,0);
			return 1;
			}
		else
			{
			int t0;

			for (t0 = 0; t0 != RLIM_NLIMITS; t0++)
				printulimit(t0,hard);
			return 0;
			}
	if (!*argv)
		printulimit(res,hard);
	else if (strcmp(*argv,"unlimited"))
		{
		long t0;
		
		t0 = atol(*argv);
		switch(res)
			{
			case RLIMIT_FSIZE: case RLIMIT_CORE: t0 *= 512; break;
			case RLIMIT_DATA: case RLIMIT_STACK: case RLIMIT_RSS:
				t0 *= 1024; break;
			}
		if (hard)
			{
			if (t0 > limits[res].rlim_max && geteuid())
				{
				zerrnam(name,"can't raise hard limits",NULL,0);
				return 1;
				}
			limits[res].rlim_max = t0;
			}
		else
			{
			if (t0 > limits[res].rlim_max)
				{
				if (geteuid())
					{
					zerrnam(name,"value exceeds hard limit",NULL,0);
					return 1;
					}
				limits[res].rlim_max = limits[res].rlim_cur = t0;
				}
			else
				limits[res].rlim_cur = t0;
			}
		}
	else
		{
		if (hard)
			{
			if (geteuid())
				{
				zerrnam(name,"can't remove hard limits",NULL,0);
				return 1;
				}
			limits[res].rlim_max = RLIM_INFINITY;
			}
		else
			limits[res].rlim_cur = limits[res].rlim_max;
		}
	return 0;
}

int putraw(c) /**/
int c;
{
	putchar(c);
	return 0;
}

int bin_echotc(name,argv,ops,func) /**/
char *name;char **argv;char *ops;int func;
{
char *s,buf[2048],*t,*u;
int num,argct,t0;

	s = *argv++;
	if (!termok)
		return 1;
	if ((num = tgetnum(s)) != -1)
		{
		printf("%d\n",num);
		return 0;
		}
	u = buf;
	t = tgetstr(s,&u);
	if (!t || !*t)
		{
		zerrnam(name,"no such capability: %s",s,0);
		return 1;
		}
	for (argct = 0, u = t; *u; u++)
		if (*u == '%')
			{
			if (u++, (*u == 'd' || *u == '2' || *u == '3' || *u == '.' ||
					*u == '+'))
				argct++;
			}
	if (arrlen(argv) != argct)
		{
		zerrnam(name,(arrlen(argv) < argct) ? "not enough arguments" :
			"too many arguments",NULL,0);
		return 1;
		}
	if (!argct)
		tputs(t,1,putraw);
	else
		{
		t0 = (argv[1]) ? atoi(argv[1]) : atoi(*argv);
		tputs(tgoto(t,atoi(*argv),t0),t0,putraw);
		}
	return 0;
}

int bin_pwd(name,argv,ops,func) /**/
char *name;char **argv;char *ops;int func;
{
	printf("%s\n",cwd);
	return 0;
}

