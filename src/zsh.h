/*

	zsh.h - standard header file

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

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>		/* this is the key to the whole thing */
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/file.h>
#include <signal.h>
#ifdef TERMIO
#include <sys/termio.h>
#else
#ifdef TERMIOS
#include <sys/termios.h>
#else
#include <sgtty.h>
#endif
#endif
#include <sys/param.h>
#include <sys/stat.h>

#define VERSIONSTR "zsh v2.00.00"

#define DEFWORDCHARS "*?_-.[]~=/"
#define DEFTIMEFMT "%E real  %U user  %S system  %P"
#ifdef UTMP_HOST
#define DEFWATCHFMT "%n has %a %l from %m."
#else
#define DEFWATCHFMT "%n has %a %l."
#endif

#define DCLPROTO(X) ()

#ifdef GLOBALS
#define EXTERN
#else
#define EXTERN extern
#endif

#include "zle.h"

/* size of job list - set small on purpose */

#define MAXJOB 16

/* memory allocation routines - changed with permalloc()/heapalloc() */

void *(*alloc)DCLPROTO((int));
void *(*ncalloc)DCLPROTO((int));

#define addhnode(A,B,C,D) Addhnode(A,B,C,D,1)
#define addhperm(A,B,C,D) Addhnode(A,B,C,D,0)

/* character tokens */

#define ALPOP			((char) 0x81)
#define HERR			((char) 0x82)
#define HISTSPACE		((char) 0x83)
#define Pound			((char) 0x84)
#define String			((char) 0x85)
#define Hat				((char) 0x86)
#define Star			((char) 0x87)
#define Inpar			((char) 0x88)
#define Outpar			((char) 0x89)
#define Qstring		((char) 0x8a)
#define Equals			((char) 0x8b)
#define Bar				((char) 0x8c)
#define Inbrace		((char) 0x8d)
#define Outbrace		((char) 0x8e)
#define Inbrack		((char) 0x8f)
#define Outbrack		((char) 0x90)
#define Tick			((char) 0x91)
#define Inang			((char) 0x92)
#define Outang			((char) 0x93)
#define Quest			((char) 0x94)
#define Tilde			((char) 0x95)
#define Qtick			((char) 0x96)
#define Comma			((char) 0x97)
#define Nularg			((char) 0x98)

/* chars that need to be quoted if meant literally */

#define SPECCHARS "#$^*()$=|{}[]`<>?~;&!\n\t \\\'\""

/* ALPOP in the form of a string */

#define ALPOPS " \201"
#define HISTMARK "\201"

EXTERN char **environ;

typedef struct hashtab *Hashtab;
typedef struct hashnode *Hashnode;
typedef struct schedcmd *Schedcmd;
typedef struct alias *Alias;
typedef struct process *Process;
typedef struct job *Job;
typedef struct value *Value;
typedef struct arrind *Arrind;
typedef struct varasg *Varasg;
typedef struct param *Param;
typedef struct cmdnam *Cmdnam;
typedef struct cond *Cond;
typedef struct cmd *Cmd;
typedef struct pline *Pline;
typedef struct sublist *Sublist;
typedef struct list *List;
typedef struct lklist *Lklist;
typedef struct lknode *Lknode;
typedef struct comp *Comp;
typedef struct complist *Complist;
typedef struct heap *Heap;
typedef void (*FFunc)DCLPROTO((void *));
typedef void *(*VFunc)DCLPROTO((void *));
typedef void (*HFunc)DCLPROTO((char *,char *));

/* linked list abstract data type */

struct lknode;
struct lklist;

struct lknode {
   Lknode next,last;
   void *dat;
   };
struct lklist {
   Lknode first,last;
   };

#define addnode(X,Y) insnode(X,(X)->last,Y)
#define full(X) ((X)->first != NULL)
#define firstnode(X) ((X)->first)
#define getaddrdata(X) (&((X)->dat))
#define getdata(X) ((X)->dat)
#define setdata(X,Y) ((X)->dat = (Y))
#define lastnode(X) ((X)->last)
#define nextnode(X) ((X)->next)
#define prevnode(X) ((X)->last)
#define peekfirst(X) ((X)->first->dat)
#define pushnode(X,Y) insnode(X,(Lknode) X,Y)
#define incnode(X) (X = nextnode(X))

/* node structure for syntax trees */

/* struct list, struct sublist, struct pline, etc.  all fit the form
	of this structure and are used interchangably
*/

struct node {
	int data[4];			/* arbitrary integer data */
	void *ptrs[4];			/* arbitrary pointer data */
	int types[4];			/* what ptrs[] are pointing to */
	int type;				/* node type */
	};

enum node_type {
	N_LIST,N_SUBLIST,N_PLINE,N_CMD,
	N_REDIR,N_COND,N_FOR,N_CASE,
	N_IF,N_WHILE,N_VARASG,
	N_COUNT
	};

/* values for types[4] */

#define NT_EMPTY 0
#define NT_NODE  1
#define NT_STR   2
#define NT_LIST  4
#define NT_MALLOC 8

/* tree element for lists */

struct list {
	int type;
	int ifil[3];		/* to fit struct node */
   Sublist left;
   List right;
   };

enum list_type {
   SYNC,		/* ; */
	ASYNC,	/* & */
	TIMED
   };

/* tree element for sublists */

struct sublist {
	int type;
	int flags;			/* see PFLAGs below */
	int ifil[2];
	Pline left;
	Sublist right;
	};

enum sublist_type {
	ORNEXT = 10,	/* || */
	ANDNEXT			/* && */
	};

#define PFLAG_NOT 1			/* ! ... */
#define PFLAG_COPROC 32		/* coproc ... */

/* tree element for pipes */

struct pline {
	int type;
	int ifil[3];
   Cmd left;
   Pline right;
   };

enum pline_type {
   END,		/* pnode *right is null */
	PIPE		/* pnode *right is the rest of the pipeline */
   };

/* tree element for commands */

struct cmd {
	int type;
	int flags;				/* see CFLAGs below */
	int ifil[2];
   Lklist args;			/* command & argmument List (char *'s) */
	union {
   	List list;			/* for SUBSH/CURSH/SHFUNC */
		struct forcmd *forcmd;
		struct casecmd *casecmd;
		struct ifcmd *ifcmd;
		struct whilecmd *whilecmd;
		Sublist pline;
		Cond cond;
		} u;
   Lklist redir;			/* i/o redirections (struct redir *'s) */
	Lklist vars;			/* param assignments (struct varasg *'s) */
   };

enum cmd_type {
	SIMPLE,		/* simple command */
	SUBSH,		/* ( list ) */
	CTIME,		/* time pline */
	CURSH,		/* { list } */
	FUNCDEF,		/* foo ... () { list } */
	CFOR,
	CWHILE,
	CREPEAT,
	CIF,
	CCASE,
	CSELECT,
	COND,			/* [[ ... ]] */
	};

#define CFLAG_EXEC 1			/* exec ... */
#define CFLAG_COMMAND 2		/* command ... */
#define CFLAG_NOGLOB 4     /* noglob ... */
#define CFLAG_DASH 8			/* - ... */

/* tree element for redirection lists */

struct redir {
	int type,fd1,fd2,ifil;
	char *name;
	struct redir *pair;		/* used by the parser */
   };

enum ftype {
	WRITE,			/* fd1 > name */
	WRITENOW,		/* fd1 >! name */
	APP,				/* fd1 >> name */
	APPNOW,			/* fd1 >>! name */
	MERGEOUT,		/* fd1 >& fd2 */
	MERGEOUTNOW,	/* fd1 >&! fd2 */
	ERRAPP,        /* fd1 >>& */
	ERRAPPNOW,     /* fd1 >>&! */
	READ,				/* fd1 < name */
	HEREDOC,			/* fd1 << foo */
	MERGE,			/* fd1 <& fd2 */

	CLOSE,			/* fd1 >&-, #<&- */
	INPIPE,			/* fd1 < name, where name is <(...) */
	OUTPIPE,			/* fd1 > name, where name is >(...)  */
	HERESTR,			/* fd1 <<< foo */
	NONE
	};

/* tree element for conditionals */

struct cond {
	int type;		/* can be cond_type, or a single letter (-a, -b, ...) */
	int ifil[3];
	void *left,*right,*vfil[2];
	int types[4],typ;	/* from struct node.  DO NOT REMOVE */
	};

enum cond_type {
	COND_NOT,COND_AND,COND_OR,
	COND_STREQ,COND_STRNEQ,COND_STRLT,COND_STRGTR,
	COND_NT,COND_OT,COND_EF,
	COND_EQ,COND_NE,COND_LT,COND_GT,COND_LE,COND_GE
	};

struct forcmd {		/* for/select */
							/* Cmd->args contains list of words to loop thru */
	int inflag;			/* if there is an in ... clause */
	int ifil[3];
	char *name;			/* parameter to assign values to */
	List list;			/* list to look through for each name */
	};
struct casecmd {
							/* Cmd->args contains word to test */
	int ifil[4];
	struct casecmd *next;
	char *pat;
	List list;					/* list to execute */
	};

/*

	a command like "if foo then bar elif baz then fubar else fooble"
	generates a tree like:

	struct ifcmd a = { next =  &b,  ifl = "foo", thenl = "bar" }
	struct ifcmd b = { next =  &c,  ifl = "baz", thenl = "fubar" }
	struct ifcmd c = { next = NULL, ifl = NULL, thenl = "fooble" }

*/

struct ifcmd {
	int ifil[4];
	struct ifcmd *next;
	List ifl;
	List thenl;
	};

struct whilecmd {
	int cond;		/* 0 for while, 1 for until */
	int ifil[3];
	List cont;		/* condition */
	List loop;		/* list to execute until condition met */
	};

/* structure used for multiple i/o redirection */
/* one for each fd open */

struct multio {
	int ct;				/* # of redirections on this fd */
	int rflag;			/* 0 if open for reading, 1 if open for writing */
	int pipe;			/* fd of pipe if ct > 1 */
	int fds[NOFILE];	/* list of src/dests redirected to/from this fd */
   }; 

/* node used in command path hash table (cmdnamtab) */

struct cmdnam 
{
	int type,flags;
	int ifil[2];
	union {
		char *nam;		/* full pathname if type != BUILTIN */
		int binnum;		/* func to exec if type == BUILTIN */
		List list;		/* list to exec if type == SHFUNC */
		} u;
	};

enum cmdnam_type {
	EXCMD_PREDOT,		/* external command in path before . */
	EXCMD_POSTDOT,		/* external command in path after . */
	BUILTIN,
	SHFUNC
	};

/* node used in parameter hash table (paramtab) */

struct param {
	union {
		char **arr;		/* value if declared array */
		char *str;		/* value if declared string (scalar) */
		long val;		/* value if declared integer */
		} u;
	union {				/* functions to call to set value */
		void (*cfn)DCLPROTO((Param,char *));
		void (*ifn)DCLPROTO((Param,long));
		void (*afn)DCLPROTO((Param,char **));
		} sets;
	union {				/* functions to call to get value */
		char *(*cfn)DCLPROTO((Param));
		long (*ifn)DCLPROTO((Param));
		char **(*afn)DCLPROTO((Param));
		} gets;
	int ct;				/* output base or field width */
	int flags;
	void *data;			/* used by getfns */
	char *env;			/* location in environment, if exported */
	char *ename;		/* name of corresponding environment var */
	};

#define PMFLAG_s 0		/* scalar */
#define PMFLAG_L 1		/* left justify and remove leading blanks */
#define PMFLAG_R 2		/* right justify and fill with leading blanks */
#define PMFLAG_Z 4		/* right justify and fill with leading zeros */
#define PMFLAG_i 8		/* integer */
#define PMFLAG_l 16		/* all lower case */
#define PMFLAG_u 32		/* all upper case */
#define PMFLAG_r 64		/* readonly */
#define PMFLAG_t 128		/* tagged */
#define PMFLAG_x 256		/* exported */
#define PMFLAG_A 512		/* array */
#define PMFLAG_SPECIAL	1024
#define PMTYPE (PMFLAG_i|PMFLAG_A)
#define pmtype(X) ((X)->flags & PMTYPE)

/* variable assignment tree element */

struct varasg {
	int type;			/* nonzero means array */
	int ifil[3];
	char *name;
	char *str;			/* should've been a union here.  oh well */
	Lklist arr;
	};

/* lvalue for variable assignment/expansion */

struct value {
	int isarr;
	struct param *pm;		/* parameter node */
	int a;					/* first element of array slice, or -1 */
	int b;					/* last element of array slice, or -1 */
	};

struct fdpair {
	int fd1,fd2;
	};

/* tty state structure */

struct ttyinfo {
#ifdef TERMIOS
	struct termios termios;
#else
#ifdef TERMIO
	struct termio termio;
#else
	struct sgttyb sgttyb;
	struct tchars tchars;
	struct ltchars ltchars;
#endif
#endif
	struct winsize winsize;
	};

EXTERN struct ttyinfo shttyinfo;

/* entry in job table */

struct job {
	long gleader;					/* process group leader of this job */
	int stat;
	char *cwd;						/* current working dir of shell when
											this job was spawned */
	struct process *procs;		/* list of processes */
	Lklist filelist;				/* list of files to delete when done */
	struct ttyinfo ttyinfo;		/* saved tty state */
	};

#define STAT_CHANGED 1		/* status changed and not reported */
#define STAT_STOPPED 2		/* all procs stopped or exited */
#define STAT_TIMED 4			/* job is being timed */
#define STAT_DONE 8
#define STAT_LOCKED 16		/* shell is finished creating this job,
										may be deleted from job table */
#define STAT_INUSE 64		/* this job entry is in use */

#define SP_RUNNING -1		/* fake statusp for running jobs */

/* node in job process lists */

struct process {
	struct process *next;
	long pid;
	char *text;						/* text to print when 'jobs' is run */
	int statusp;					/* return code from wait3() */
	int lastfg;						/* set if this process represents a
											fragment of a pipeline run in a subshell
											for commands like:

											foo | bar | ble

											where foo is a current shell function
											or control structure.  The command
											actually executed is:

											foo | (bar | ble)

											That's two procnodes in the parent
											shell, the latter having this flag set. */
	struct rusage ru;
	time_t bgtime;					/* time job was spawned */
	time_t endtime;				/* time job exited */
	};

/* node in alias hash table */

struct alias {
	char *text;			/* expansion of alias */
	int cmd;				/* one for regular aliases,
								zero for global aliases,
								negative for reserved words */
	int inuse;			/* alias is being expanded */
	};

/* node in sched list */

struct schedcmd {
	struct schedcmd *next;
	char *cmd;		/* command to run */
	time_t time;	/* when to run it */
	};

#define MAXAL 20	/* maximum number of aliases expanded at once */

/* hash table node */

struct hashnode {
	struct hashnode *next;
	char *nam;
	void *dat;
	int canfree;		/* nam is free()able */
	};

/* hash table */

struct hashtab {
	int hsize;							/* size of nodes[] */
	int ct;								/* # of elements */
	struct hashnode **nodes;		/* array of size hsize */
	};

extern char *sys_errlist[];
extern int errno;

/* values in opts[] array */

#define OPT_INVALID 1	/* opt is invalid, like -$ */
#define OPT_UNSET 0
#define OPT_SET 2

/* the options */

struct option {
	char *name;
	char id;			/* corresponding letter */
	};

#define CORRECT '0'
#define NOCLOBBER '1'
#define NOBADPATTERN '2'
#define NONOMATCH '3'
#define GLOBDOTS '4'
#define NOTIFY '5'
#define BGNICE '6'
#define IGNOREEOF '7'
#define MARKDIRS '8'
#define AUTOLIST '9'
#define NOBEEP 'B'
#define PRINTEXITVALUE 'C'
#define PUSHDTOHOME 'D'
#define PUSHDSILENT 'E'
#define NOGLOBOPT 'F'
#define NULLGLOB 'G'
#define RMSTARSILENT 'H'
#define IGNOREBRACES 'I'
#define AUTOCD 'J'
#define NOBANGHIST 'K'
#define SUNKEYBOARDHACK 'L'
#define SINGLELINEZLE 'M'
#define AUTOPUSHD 'N'
#define CORRECTALL 'O'
#define RCEXPANDPARAM 'P'
#define PATHDIRS 'Q'
#define LONGLISTJOBS 'R'
#define RECEXACT 'S'
#define CDABLEVARS 'T'
#define MAILWARNING 'U'
#define AUTORESUME 'W'
#define NICEAPPENDAGES 'X'			/* historical */
#define MENUCOMPLETE 'Y'
#define USEZLE 'Z'
#define ALLEXPORT 'a'
#define ERREXIT 'e'
#define NORCS 'f'
#define HISTIGNORESPACE 'g'
#define HISTIGNOREDUPS 'h'
#define INTERACTIVE 'i'
#define HISTLIT 'j'
#define INTERACTIVECOMMENTS 'k'
#define LOGINSHELL 'l'
#define MONITOR 'm'
#define NOEXEC 'n'
#define SHINSTDIN 's'
#define NOUNSET 'u'
#define VERBOSE 'v'
#define XTRACE 'x'
#define SHWORDSPLIT 'y'

#ifndef GLOBALS
extern struct option optns[];
#else
struct option optns[] = {
	"correct",'0',
	"noclobber",'1',
	"nobadpattern",'2',
	"nonomatch",'3',
	"globdots",'4',
	"notify",'5',
	"bgnice",'6',
	"ignoreeof",'7',
	"markdirs",'8',
	"autolist",'9',
	"nobeep",'B',
	"printexitvalue",'C',
	"pushdtohome",'D',
	"pushdsilent",'E',
	"noglob",'F',
	"nullglob",'G',
	"rmstarsilent",'H',
	"ignorebraces",'I',
	"autocd",'J',
	"nobanghist",'K',
	"sunkeyboardhack",'L',
	"singlelinezle",'M',
	"autopushd",'N',
	"correctall",'O',
	"rcexpandparam",'P',
	"pathdirs",'Q',
	"longlistjobs",'R',
	"recexact",'S',
	"cdablevars",'T',
	"mailwarning",'U',
	"autoresume",'W',
	"listtypes",'X',
	"menucomplete",'Y',
	"zle",'Z',
	"allexport",'a',
	"errexit",'e',
	"norcs",'f',
	"histignorespace",'g',
	"histignoredups",'h',
	"interactive",'i',
	"histlit",'j',
	"interactivecomments",'k',
	"login",'l',
	"monitor",'m',
	"noexec",'n',
	"shinstdin",'s',
	"nounset",'u',
	"verbose",'v',
	"xtrace",'x',
	"shwordsplit",'y',
	NULL,0
};
#endif

#define ALSTAT_MORE 1	/* last alias ended with ' ' */
#define ALSTAT_JUNK 2	/* don't put word in history List */

#undef isset
#define isset(X) (opts[X])
#define unset(X) (!opts[X])
#define interact (isset(INTERACTIVE))
#define jobbing (isset(MONITOR))
#define nointr() signal(SIGINT,SIG_IGN)
#define islogin (isset(LOGINSHELL))


#define SP(x) (*((union wait *) &(x)))

#ifndef WEXITSTATUS
#define	WEXITSTATUS(x)	(((union wait*)&(x))->w_retcode)
#define	WTERMSIG(x)	(((union wait*)&(x))->w_termsig)
#define	WSTOPSIG(x)	(((union wait*)&(x))->w_stopsig)
#endif

#ifndef S_ISBLK
#define	_IFMT		0170000
#define	_IFDIR	0040000
#define	_IFCHR	0020000
#define	_IFBLK	0060000
#define	_IFREG	0100000
#define	_IFLNK	0120000
#define	_IFSOCK	0140000
#define	_IFIFO	0010000
#define	S_ISBLK(m)	(((m)&_IFMT) == _IFBLK)
#define	S_ISCHR(m)	(((m)&_IFMT) == _IFCHR)
#define	S_ISDIR(m)	(((m)&_IFMT) == _IFDIR)
#define	S_ISFIFO(m)	(((m)&_IFMT) == _IFIFO)
#define	S_ISREG(m)	(((m)&_IFMT) == _IFREG)
#define	S_ISLNK(m)	(((m)&_IFMT) == _IFLNK)
#define	S_ISSOCK(m)	(((m)&_IFMT) == _IFSOCK)
#endif

/* buffered shell input for non-interactive shells */

EXTERN FILE *bshin;

/* NULL-terminated arrays containing path, cdpath, etc. */

EXTERN char **path,**cdpath,**fpath,**watch,**mailpath,**tildedirs,**fignore;

/* named directories */

EXTERN char **userdirs,**usernames;

/* size of userdirs[], # of userdirs */

EXTERN int userdirsz,userdirct;

EXTERN char *mailfile;

EXTERN char *yytext;

/* error/break flag */

EXTERN int errflag;

/* suppress error messages */

EXTERN int noerrs;

/* current history event number */

EXTERN int curhist;

/* if != 0, this is the first line of the command */

EXTERN int isfirstln;

/* if != 0, this is the first char of the command (not including
	white space */

EXTERN int isfirstch;

/* first event number in the history lists */

EXTERN int firsthist,firstlithist;

/* capacity of history lists */

EXTERN int histsiz,lithistsiz;

/* if = 1, we have performed history substitution on the current line
 	if = 2, we have used the 'p' modifier */

EXTERN int histdone;

/* default event (usually curhist-1, that is, "!!") */

EXTERN int defev;

/* != 0 if we are in the middle of parsing a command (that is,
	we have already read the command word */

EXTERN int incmd;

/* != 0 if we just read a redirection */

EXTERN int inredir;

/* != 0 if we are in the middle of a [[ ... ]] */

EXTERN int incond;

/* != 0 if we are about to read a case pattern */

EXTERN int incase;

/* != 0 if we just read a separator (; or newline) */

EXTERN int lsep;

/* != 0 if we just read FUNCTION */

EXTERN int infunc;

/* the lists of history events */

EXTERN Lklist histlist,lithistlist;

/* the directory stack */

EXTERN Lklist dirstack;

/* the zle buffer stack */

EXTERN Lklist bufstack;

/* the input queue (stack?)

	inbuf    = start of buffer
	inbufptr = location in buffer (= inbuf for a FULL buffer)
											(= inbuf+inbufsz for an EMPTY buffer)
	inbufct  = # of chars in buffer (inbufptr+inbufct == inbuf+inbufsz)
	inbufsz  = max size of buffer
*/

EXTERN char *inbuf,*inbufptr;
EXTERN int inbufct,inbufsz;

EXTERN char *ifs;		/* $IFS */

EXTERN char *oldpwd;	/* $OLDPWD */

EXTERN char *underscore; /* $_ */

/* != 0 if this is a subshell */

EXTERN int subsh;

/* # of break levels */

EXTERN int breaks;

/* != 0 if we have a return pending */

EXTERN int retflag;

/* != 0 means don't hash the PATH */

EXTERN int pathsuppress;

/* # of nested loops we are in */

EXTERN int loops;

/* # of continue levels */

EXTERN int contflag;

/* the job we are working on */

EXTERN int thisjob;

/* the current job (+) */

EXTERN int curjob;

/* the previous job (-) */

EXTERN int prevjob;

/* hash table containing the aliases and reserved words */

EXTERN Hashtab aliastab;

/* hash table containing the parameters */

EXTERN Hashtab paramtab;

/* hash table containing the builtins/shfuncs/hashed commands */

EXTERN Hashtab cmdnamtab;

/* hash table containing the zle multi-character bindings */

EXTERN Hashtab xbindtab;

/* the job table */

EXTERN struct job jobtab[MAXJOB];

/* the list of sched jobs pending */

EXTERN struct schedcmd *schedcmds;

/* the last l for s/l/r/ history substitution */

EXTERN char *hsubl;

/* the last r for s/l/r/ history substitution */

EXTERN char *hsubr;

EXTERN char *username;	/* $USERNAME */
EXTERN long lastval;		/* $? */
EXTERN long baud;			/* $BAUD */
EXTERN long columns;		/* $COLUMNS */
EXTERN long lines;		/* $LINES */

/* input fd from the coprocess */

EXTERN int coprocin;

/* output fd from the coprocess */

EXTERN int coprocout;

EXTERN long mailcheck;	/* $MAILCHECK */
EXTERN long logcheck;	/* $LOGCHECK */

/* the last time we checked mail */

EXTERN time_t lastmailcheck;

/* the last time we checked the people in the WATCH variable */

EXTERN time_t lastwatch;

/* the last time we did the periodic() shell function */

EXTERN time_t lastperiod;

/* $SECONDS = time(NULL) - shtimer */

EXTERN time_t shtimer;

EXTERN long mypid;		/* $$ */
EXTERN long lastpid;		/* $! */
EXTERN long ppid;			/* $PPID */

/* the process group of the shell */

EXTERN long mypgrp;

EXTERN char *cwd;			/* $PWD */
EXTERN char *optarg;		/* $OPTARG */
EXTERN long optind;		/* $OPTIND */
EXTERN char *prompt;		/* $PROMPT */
EXTERN char *rprompt;	/* $RPROMPT */
EXTERN char *prompt2;	/* etc. */
EXTERN char *prompt3;
EXTERN char *prompt4;
EXTERN char *timefmt;
EXTERN char *watchfmt;
EXTERN char *wordchars;

EXTERN char *argzero;	/* $0 */

/* the hostname */

EXTERN char *hostm;

/* the hostname, truncated after the '.' */

EXTERN char *hostM;

EXTERN char *home;		/* $HOME */
EXTERN char **pparams;	/* $argv */

/* the List of local variables we have to destroy */

EXTERN Lklist locallist;

/* the shell input fd */

EXTERN int SHIN;

/* the shell tty fd */

EXTERN int SHTTY;

/* the stack of aliases we are expanding */

EXTERN struct alias *alstack[MAXAL];

/* the alias stack pointer; also, the number of aliases currently
 	being expanded */

EXTERN int alstackind;

/* != 0 means we are reading input from a string */

EXTERN int strin;

/* period between periodic() commands, in seconds */

EXTERN long period;

/* != 0 means history substitution is turned off */

EXTERN int stophist;

EXTERN int lithist;

/* this line began with a space, so junk it if HISTIGNORESPACE is on */

EXTERN int spaceflag;

/* != 0 means we have removed the current event from the history List */

EXTERN int histremmed;

/* the options; e.g. if opts['a'] == OPT_SET, -a is turned on */

EXTERN int opts[128];

EXTERN long lineno;		/* LINENO */
EXTERN long listmax;		/* LISTMAX */
EXTERN long savehist;	/* SAVEHIST */
EXTERN long shlvl;		/* SHLVL */
EXTERN long tmout;		/* TMOUT */
EXTERN long dirstacksize;	/* DIRSTACKSIZE */

/* != 0 means we have called execlist() and then intend to exit(),
 	so don't fork if not necessary */

EXTERN int exiting;

EXTERN int lastbase;		/* last input base we used */

/* the limits for child processes */

EXTERN struct rlimit limits[RLIM_NLIMITS];

/* the current word in the history List */

EXTERN char *hlastw;

/* pointer into the history line */

EXTERN char *hptr;

/* the current history line */

EXTERN char *hline;

/* the termcap buffer */

EXTERN char termbuf[1024];

/* $TERM */

EXTERN char *term;

/* != 0 if this $TERM setup is usable */

EXTERN int termok;

/* max size of hline */

EXTERN int hlinesz;

/* the alias expansion status - if == ALSTAT_MORE, we just finished
	expanding an alias ending with a space */

EXTERN int alstat;

/* we have printed a 'you have stopped (running) jobs.' message */

EXTERN int stopmsg;

/* the default tty state */

EXTERN struct ttyinfo shttyinfo;

/* $TTY */

EXTERN char *ttystrname;

/* list of memory heaps */

EXTERN Lklist heaplist;

/* != 0 if we are allocating in the heaplist */

EXTERN int useheap;

#include "signals.h"

#ifdef GLOBALS

/* signal names */
char **sigptr = sigs;

/* tokens */
char *ztokens = "#$^*()$=|{}[]`<>?~`,";

#else
extern char *ztokens,**sigptr;
#endif

#define SIGERR (SIGCOUNT+1)
#define SIGDEBUG (SIGCOUNT+2)
#define VSIGCOUNT (SIGCOUNT+3)
#define SIGEXIT 0

/* signals that are trapped = 1, signals ignored =2 */

EXTERN int sigtrapped[VSIGCOUNT];

/* trap functions for each signal */

EXTERN List sigfuncs[VSIGCOUNT];

/* $HISTCHARS */

EXTERN char bangchar,hatchar,hashchar;

EXTERN int eofseen;

/* we are parsing a line sent to use by the editor */

EXTERN int zleparse;

EXTERN int wordbeg;

/* interesting termcap strings */

enum xtcaps {
	TCCLEARSCREEN,TCLEFT,TCMULTLEFT,TCRIGHT,
	TCMULTRIGHT,TCUP,TCMULTUP,TCDOWN,
	TCMULTDOWN,TCDEL,TCMULTDEL,TCINS,
	TCMULTINS,TCCLEAREOD,TCCLEAREOL,TCINSLINE,
	TCDELLINE,TC_COUNT
	};

/* lengths of each string */

EXTERN int tclen[TC_COUNT];

EXTERN char *tcstr[TC_COUNT];

#ifdef GLOBALS

/* names of the strings we want */

char *tccapnams[TC_COUNT] = {
	"cl","le","LE","nd","RI","up","UP","do",
	"DO","dc","DC","ic","IC","cd","ce","al","dl"
	};

#else
extern char *tccapnams[TC_COUNT];
#endif

#define tccan(X) (!!tclen[X])

#include "ztype.h"
