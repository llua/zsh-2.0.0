/*

	params.c - parameters

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

#define new(X) (X=(void*)alloc(sizeof(*(X))))

static Param argvparam;

/* put predefined params in hash table */

void setupparams() /**/
{
/* special integer params */
static struct { char *name;
			long (*get) DCLPROTO((Param));
			void (*set) DCLPROTO((Param,long)); } x1[] = {
	"#",poundgetfn,NULL,
	"ARGC",poundgetfn,NULL,
	"ERRNO",errnogetfn,NULL,
	"GID",gidgetfn,NULL,
	"HISTSIZE",histsizegetfn,histsizesetfn,
	"LITHISTSIZE",lithistsizegetfn,lithistsizesetfn,
	"RANDOM",randomgetfn,randomsetfn,
	"SECONDS",secondsgetfn,secondssetfn,
	"UID",uidgetfn,uidsetfn,
	NULL,NULL,NULL
	}, *p1 = x1;
/* special string params */
static struct { char *name;
			char *(*get) DCLPROTO((Param));
			void (*set) DCLPROTO((Param,char *)); } x2[] = {
	"-",dashgetfn,NULL,
	"HISTCHARS",histcharsgetfn,histcharssetfn,
	"HOME",homegetfn,homesetfn,
	"TERM",termgetfn,termsetfn,
	"WORDCHARS",wordcharsgetfn,wordcharssetfn,
	"IFS",ifsgetfn,ifssetfn,
	"_",underscoregetfn,NULL,
	NULL,NULL,NULL
	}, *p2 = x2;
/* constant string params */
static struct { char *name,*data; } x3[] = {
	"HOSTTYPE",HOSTTYPE,
	"VERSION",VERSIONSTR,
	NULL,NULL
	}, *p3 = x3;
/* variable integer params */
static struct { char *name; long *data; } x4[] = {
	"!",&lastpid,				/* read only */
	"$",&mypid,
	"?",&lastval,
	"LINENO",&lineno,
	"PPID",&ppid,
	NULL,NULL,

	"BAUD",&baud,				/* read/write */
	"COLUMNS",&columns,
	"DIRSTACKSIZE",&dirstacksize,
	"LINES",&lines,
	"LISTMAX",&listmax,
	"LOGCHECK",&logcheck,
	"MAILCHECK",&mailcheck,
	"OPTIND",&optind,
	"PERIOD",&period,
	"SAVEHIST",&savehist,
	"SHLVL",&shlvl,
	"TMOUT",&tmout,
	NULL,NULL
	}, *p4 = x4;
/* variable string params */
static struct { char *name; char **data; } x5[] = {
	"HOST",&hostm,				/* read only */
	"LOGNAME",&username,
	"OLDPWD",&oldpwd,
	"PWD",&cwd,
	"TTY",&ttystrname,
	"USERNAME",&username,
	NULL,NULL,

	"OPTARG",&optarg,		/* read/write */
	"MAIL",&mailfile,
	"PROMPT",&prompt,
	"PROMPT2",&prompt2,
	"PROMPT3",&prompt3,
	"PROMPT4",&prompt4,
	"RPROMPT",&rprompt,
	"PS1",&prompt,
	"PS2",&prompt2,
	"PS3",&prompt3,
	"PS4",&prompt4,
	"RPS1",&rprompt,
	"TIMEFMT",&timefmt,
	"WATCHFMT",&watchfmt,
	"0",&argzero,
	NULL,NULL
	}, *p5 = x5;
/* colonsplit string params */
static struct { char *name; } x6[] = {
	"CDPATH","FIGNORE","FPATH","MAILPATH","WATCH",
	NULL
	}, *p6 = x6;
/* variable array params */
static struct { char *name; char ***data; } x7[] = {
	"cdpath",&cdpath,
	"fignore",&fignore,
	"fpath",&fpath,
	"mailpath",&mailpath,
	"watch",&watch,
	"signals",(char ***) &sigptr,
	"argv",&pparams,
	"*",&pparams,
	"@",&pparams,
	NULL,NULL
	}, *p7 = x7;
/* special array params */
static struct { char *name;
					char **(*get) DCLPROTO((Param));
					void (*set) DCLPROTO((Param,char **)); } x8[] = {
	"path",pathgetfn,pathsetfn,
	NULL,NULL,NULL
	}, *p8 = x8;
Param pm,pm2;

	for (;p1->name;p1++)
		{
		new(pm);
		pm->gets.ifn = p1->get;
		pm->sets.ifn = p1->set;
		pm->flags = (p1->set) ? PMFLAG_i|PMFLAG_SPECIAL :
			PMFLAG_r|PMFLAG_i|PMFLAG_SPECIAL;
		pm->ct = 10;
		addhperm(p1->name,pm,paramtab,NULL);
		}
	for (;p2->name;p2++)
		{
		new(pm);
		pm->gets.cfn = p2->get;
		pm->sets.cfn = p2->set;
		pm->flags = (p2->set) ? PMFLAG_SPECIAL : PMFLAG_r|PMFLAG_SPECIAL;
		addhperm(p2->name,pm,paramtab,NULL);
		}
	for (;p3->name;p3++)
		{
		new(pm);
		pm->gets.cfn = strconstgetfn;
		pm->flags = PMFLAG_r|PMFLAG_SPECIAL;
		pm->data = p3->data;
		addhperm(p3->name,pm,paramtab,NULL);
		}
	for (;p4->name;p4++)
		{
		new(pm);
		pm->gets.ifn = intvargetfn;
		pm->sets.ifn = NULL;
		pm->data = p4->data;
		pm->flags = PMFLAG_r|PMFLAG_i|PMFLAG_SPECIAL;
		pm->ct = 10;
		addhperm(p4->name,pm,paramtab,NULL);
		}
	for (p4++;p4->name;p4++)
		{
		new(pm);
		pm->gets.ifn = intvargetfn;
		pm->sets.ifn = intvarsetfn;
		pm->data = p4->data;
		pm->flags = PMFLAG_i|PMFLAG_SPECIAL;
		pm->ct = 10;
		addhperm(p4->name,pm,paramtab,NULL);
		}
	for (;p5->name;p5++)
		{
		new(pm);
		pm->gets.cfn = strvargetfn;
		pm->sets.cfn = NULL;
		pm->data = p5->data;
		pm->flags = PMFLAG_r|PMFLAG_SPECIAL;
		addhperm(p5->name,pm,paramtab,NULL);
		}
	for (p5++;p5->name;p5++)
		{
		new(pm);
		pm->gets.cfn = strvargetfn;
		pm->sets.cfn = strvarsetfn;
		pm->data = p5->data;
		pm->flags = PMFLAG_SPECIAL;
		addhperm(p5->name,pm,paramtab,NULL);
		}
	for (;p6->name;p6++,p7++)
		{
		new(pm);
		new(pm2);
		pm->gets.cfn = colonarrgetfn;
		pm->sets.cfn = colonarrsetfn;
		pm2->gets.afn = arrvargetfn;
		pm2->sets.afn = arrvarsetfn;
		pm->data = p7->data;
		pm2->data = p7->data;
		pm->flags = PMFLAG_SPECIAL;
		pm2->flags = PMFLAG_A|PMFLAG_SPECIAL;
		pm2->ename = p6->name;
		addhperm(p6->name,pm,paramtab,NULL);
		addhperm(p7->name,pm2,paramtab,NULL);
		}
	new(pm);
	pm->gets.cfn = colonarrgetfn;
	pm->sets.cfn = colonarrsetfn;
	pm->data = NULL;
	pm->flags = PMFLAG_SPECIAL;
	addhperm("PATH",pm,paramtab,NULL);
	for (;p7->name;p7++)
		{
		new(pm);
		pm->gets.afn = arrvargetfn;
		pm->sets.afn = arrvarsetfn;
		pm->data = p7->data;
		pm->flags = PMFLAG_A|PMFLAG_SPECIAL;
		if (pm->data == &sigptr)
			pm->flags |= PMFLAG_r;
		addhperm(p7->name,pm,paramtab,NULL);
		}
	for (;p8->name;p8++)
		{
		new(pm);
		pm->gets.afn = p8->get;
		pm->sets.afn = p8->set;
		pm->flags = PMFLAG_A|PMFLAG_SPECIAL;
		addhperm(p8->name,pm,paramtab,NULL);
		}
	argvparam = gethnode("argv",paramtab);
}

static int unsetflag;

struct param *createparam(name,value,flags) /**/
char *name;void *value;int flags;
{
struct param *pm;

	pm = zcalloc(sizeof *pm);
	if (isset(ALLEXPORT))
		flags |= PMFLAG_x;
	pm->flags = flags;
	if ((flags & PMTYPE) == PMFLAG_s)
		{
		pm->u.str = ztrdup(value);
		pm->sets.cfn = strsetfn;
		pm->gets.cfn = strgetfn;
		}
	else if ((flags & PMTYPE) == PMFLAG_A)
		{
		pm->u.arr = value;
		pm->sets.afn = arrsetfn;
		pm->gets.afn = arrgetfn;
		}
	else
		{
		pm->u.val = (value) ? matheval(value) : 0;
		pm->sets.ifn = intsetfn;
		pm->gets.ifn = intgetfn;
		}
	if (flags & PMFLAG_x)
		pm->env = addenv(name,value);
	addhnode(ztrdup(name),pm,paramtab,NULL);
	return pm;
}

int isident(s) /**/
char *s;
{
	if (idigit(*s))
		while (idigit(*s)) s++;
	else if (iident(*s))
		while (iident(*s)) s++;
	else if (*s == Quest)
		*s++ = '?';
	else if (*s == Pound)
		*s++ = '#';
	else if (*s == String)
		*s++ = '$';
	else if (*s == Qstring)
		*s++ = '$';
	else if (*s == Star)
		*s++ = '*';
	else if (*s == '#' || *s == '-' || *s == '?' || *s == '$' ||
			*s == '_' || *s == '!' || *s == '@' || *s == '*')
		s++;
	else
		return 0;
	return 1;
}

Value getvalue(pptr,bracks) /**/
char **pptr;int bracks;
{
char *s = *pptr,*t = *pptr;
char sav;
Value v;

	if (idigit(*s))
		while (idigit(*s)) s++;
	else if (iident(*s))
		while (iident(*s)) s++;
	else if (*s == Quest)
		*s++ = '?';
	else if (*s == Pound)
		*s++ = '#';
	else if (*s == String)
		*s++ = '$';
	else if (*s == Qstring)
		*s++ = '$';
	else if (*s == Star)
		*s++ = '*';
	else if (*s == '#' || *s == '-' || *s == '?' || *s == '$' ||
			*s == '_' || *s == '!' || *s == '@' || *s == '*')
		s++;
	else
		return NULL;
	if (sav = *s)
		*s = '\0';
	if (idigit(*t) && *t != '0')
		{
		v = (Value) alloc(sizeof *v);
		v->pm = argvparam;
		v->a = v->b = atoi(t)-1;
		if (sav)
			*s = sav;
		}
	else
		{
		struct param *pm;

		pm = gethnode(t,paramtab);
		if (sav)
			*s = sav;
		*pptr = s;
		if (!pm)
			return NULL;
		v = alloc(sizeof *v);
		v->isarr = pmtype(pm) == PMFLAG_A;
		v->pm = pm;
		v->a = v->b = -1;
		if (bracks && (*s == '[' || *s == Inbrack))
			{
			int a,b;
			char *olds = s,*t;

			*s++ = '[';
			for (t = s; *t && *t != ']' && *t != Outbrack; t++)
				if (itok(*t))
					*t = ztokens[*t-Pound];
			if (*t == Outbrack)
				*t = ']';
			if (s[0] == '*' && s[1] == ']')
				{
				s += 2;
				v->isarr = 1;
				v->a = 0;
				v->b = -1;
				}
			else
				{
				a = mathevalarg(s,&s)-1;
				if (a < 0)
					a = -1;
				else if (*s == ',' || *s == Comma)
					{
					s++;
					b = mathevalarg(s,&s)-1;
					if (b < 0 || b < a)
						b = -1;
					}
				else
					b = a;
				if (*s == ']')
					{
					s++;
					v->isarr = v->isarr && ((a == -1) || !(a == b));
					v->a = a;
					v->b = b;
					}
				else
					s = olds;
				}
			}
		}
	if (!bracks && *s)
		return NULL;
	*pptr = s;
	return v;
}

char *getstrvalue(v) /**/
Value v;
{
char *s;
static char buf[20];

	if (!v)
		return "";
	if (pmtype(v->pm) != PMFLAG_A)
		{
		if ((pmtype(v->pm) == PMFLAG_i))
			convbase(s = buf,v->pm->gets.ifn(v->pm),v->pm->ct);
		else
			s = v->pm->gets.cfn(v->pm);
		if (v->a != -1)
			{
			s = (v->a > strlen(s)) ? strdup("") : strdup(s+v->a);
			if (v->b != -1 && v->b-v->a < strlen(s))
				s[v->b-v->a+1] = '\0';
			}
		}
	else if (v->isarr)
		s = spacejoin(v->pm->gets.afn(v->pm));
	else
		{
		char **ss = v->pm->gets.afn(v->pm);

		s = (v->a >= arrlen(ss)) ? "" : ss[v->a];
		}
	return s;
}

char **getarrvalue(v) /**/
Value v;
{
char **s;
static char *nular[] = { "", NULL };

	if (!v)
		return arrdup(nular);
	s = v->pm->gets.afn(v->pm);
	if (v->a != -1)
		{
		if (v->a > arrlen(s))
			s = arrdup(nular);
		else
			s = arrdup(s)+v->a;
		if (v->b != -1 && v->b-v->a < arrlen(s))
			s[v->b-v->a+1] = NULL;
		}
	return s;
}

long getintvalue(v) /**/
Value v;
{
	if (!v || v->isarr)
		return 0;
	if (pmtype(v->pm) != PMFLAG_A)
		{
		if (pmtype(v->pm) == PMFLAG_i)
			return v->pm->gets.ifn(v->pm);
		return atol(v->pm->gets.cfn(v->pm));
		}
	return atol(v->pm->gets.afn(v->pm)[v->a]);
}

void setstrvalue(v,val) /**/
Value v;char *val;
{
char *s;

	if (v->pm->flags & PMFLAG_r)
		return;
	if ((s = v->pm->env) && val)
		v->pm->env = replenv(v->pm->env,val);
	switch (pmtype(v->pm))
		{
		case PMFLAG_s:
			if (v->a == -1)
				(v->pm->sets.cfn)(v->pm,val);
			else
				{
				char *z,*y,*x;

				z = strdup((v->pm->gets.cfn)(v->pm));
				z[v->a] = '\0';
				y = (v->b == -1) ? "" : z+v->b+1;
				x = zalloc(strlen(z)+strlen(y)+strlen(val)+1);
				strcpy(x,z);
				strcat(x,val);
				strcat(x,y);
				(v->pm->sets.cfn)(v->pm,z);
				}
			if (v->pm->flags & (PMFLAG_L|PMFLAG_R|PMFLAG_Z) && !v->pm->ct)
				v->pm->ct = strlen(val);
			break;
		case PMFLAG_i:
			(v->pm->sets.ifn)(v->pm,matheval(val));
			if (!v->pm->ct && lastbase != 1)
				v->pm->ct = lastbase;
			free(val);
			break;
		case PMFLAG_A:
			if (v->a == -1 || v->a != v->b)
				zerr("illegal array assignment",NULL,0);
			else
				{
				char **ss = (v->pm->gets.afn)(v->pm);
				int act;

				act = arrlen(ss);
				if (v->a < act)
					{
					free(ss[v->a]);
					ss[v->a] = val;
					(v->pm->sets.afn)(v->pm,ss);
					}
				else
					zerr("illegal array assignment",NULL,0);
				}
			break;
		}
}

void setintvalue(v,val) /**/
Value v;long val;
{
char buf[20];

	if (v->pm->flags & PMFLAG_r)
		return;
	switch (pmtype(v->pm))
		{
		case PMFLAG_s:
			sprintf(buf,"%ld",val);
			(v->pm->sets.cfn)(v->pm,ztrdup(buf));
			break;
		case PMFLAG_i:
			(v->pm->sets.ifn)(v->pm,val);
			if (!v->pm->ct && lastbase != -1)
				v->pm->ct = lastbase;
			break;
		case PMFLAG_A:
			zerr("attempt to assign integer to array",NULL,0);
			break;
		}
}

void setarrvalue(v,val) /**/
Value v;char **val;
{
	if (v->pm->flags & PMFLAG_r)
		return;
	if (pmtype(v->pm) != PMFLAG_A)
		{
		zerr("attempt to assign non-array to array",NULL,0);
		return;
		}
	(v->pm->sets.afn)(v->pm,val);
}

char *getsparamval(s,l) /**/
char *s;int l;
{
char sav,*t = s;
Value v;

	if (sav = t[l])
		t[l] = '\0';
	if (!(v = getvalue(&s,0)))
		return NULL;
	t[l] = sav;
	t = getstrvalue(v);
	return t;
}

long getiparam(s) /**/
char *s;
{
Value v;

	if (!(v = getvalue(&s,0)))
		return 0;
	return getintvalue(v);
}

char *getsparam(s) /**/
char *s;
{
Value v;

	if (!(v = getvalue(&s,0)))
		return NULL;
	return getstrvalue(v);
}

Param setsparam(s,val) /**/
char *s;char *val;
{
Value v;
char *t = s;

	if (!isident(s))
		{
		zerr("not an identifier: %s",s,0);
		return NULL;
		}
	if (!(v = getvalue(&s,1)) || *s)
		return createparam(t,val,PMFLAG_s);
	setstrvalue(v,val);
	return v->pm;
}

Param setaparam(s,val) /**/
char *s;char **val;
{
Value v;
char *t = s;

	if (!isident(s))
		{
		zerr("not an identifier: %s",s,0);
		return NULL;
		}
	if (!(v = getvalue(&s,1)) || *s)
		return createparam(t,val,PMFLAG_A);
	setarrvalue(v,val);
	return v->pm;
}

Param setiparam(s,val) /**/
char *s;long val;
{
Value v;
char *t = s;
Param pm;

	if (!isident(s))
		{
		zerr("not an identifier: %s",s,0);
		return NULL;
		}
	if (!(v = getvalue(&s,0)))
		{
		pm = createparam(t,NULL,PMFLAG_i);
		pm->u.val = val;
		return pm;
		}
	setintvalue(v,val);
	return v->pm;
}

void unsetparam(s) /**/
char *s;
{
Param pm;

	if (!(pm = gethnode(s,paramtab)))
		return;
	if (pm->flags & PMFLAG_r)
		return;
	unsetflag = 1;
	switch (pmtype(pm))
		{
		case 0:
			(pm->sets.cfn)(pm,NULL);
			break;
		case PMFLAG_i:
			(pm->sets.ifn)(pm,0);
			break;
		case PMFLAG_A:
			(pm->sets.afn)(pm,NULL);
			break;
		}
	if (pmtype(pm) == PMFLAG_s && (pm->flags & PMFLAG_x))
		delenv(pm->env);
	if (!(pm->flags & PMFLAG_SPECIAL))
		remhnode(s,paramtab); /* storage leak - no freepm */
	unsetflag = 0;
}

void intsetfn(pm,x) /**/
Param pm;long x;
{
	pm->u.val = x;
}

long intgetfn(pm) /**/
Param pm;
{
	return pm->u.val;
}

void strsetfn(pm,x) /**/
Param pm;char *x;
{
	if (x) 
		{
		if (pm->u.str)
			free(pm->u.str);
		pm->u.str = x;
		}
}

char *strgetfn(pm) /**/
Param pm;
{
	return pm->u.str;
}

void arrsetfn(pm,x) /**/
Param pm;char **x;
{
int ct;

	if (x)
		{
		if (pm->u.arr && pm->u.arr != x)
			freearray(pm->u.arr);
		pm->u.arr = x;
		for (ct = 0; *x; x++,ct++);
		pm->ct = ct;
		}
}

char **arrgetfn(pm) /**/
Param pm;
{
	return pm->u.arr;
}

void intvarsetfn(pm,x) /**/
Param pm;long x;
{
	*((long *) pm->data) = x;
}

long intvargetfn(pm) /**/
Param pm;
{
	return *((long *) pm->data);
}

void strvarsetfn(pm,x) /**/
Param pm;char *x;
{
	*((char **) pm->data) = x;
}

void strvarnonullsetfn(pm,x) /**/
Param pm;char *x;
{
	*((char **) pm->data) = (x) ? x : ztrdup("");
}

char *strvargetfn(pm) /**/
Param pm;
{
	return *((char **) pm->data);
}

char *strconstgetfn(pm) /**/
Param pm;
{
	return (char *) pm->data;
}

void colonarrsetfn(pm,x) /**/
Param pm;char *x;
{
char **s,**t,*u;

	s = colonsplit(x);
	for (t = s; *t; t++)
		{
		u = *t;
		if (*u == '~')
			*u = Tilde;
		if (*u == '=')
			*u = Equals;
		u = strdup(u);
		filesub(&u);
		if (!*u)
			u = ".";
		*t = ztrdup(u);
		}
	if (pm->data)
		{
		*((char ***) pm->data) = s;
		if (pm->ename)
			arrfixenv(pm->ename,s);
		}
	else
		{
		path = s;
		newcmdnamtab();
		arrfixenv("PATH",s);
		}
}

char *colonarrgetfn(pm) /**/
Param pm;
{
	if ((char **) pm->data)
		return colonjoin(*(char ***) pm->data);
	else
		return colonjoin(path);
}

char **arrvargetfn(pm) /**/
Param pm;
{
	return *((char ***) pm->data);
}

void arrvarsetfn(pm,x) /**/
Param pm;char **x;
{
	if ((*(char ***) pm->data) != x)
		freearray(*(char ***) pm->data);
	*((char ***) pm->data) = x;
	if (pm->ename)
		arrfixenv(pm->ename,x);
}

char **pathgetfn(pm) /**/
Param pm;
{
	return path;
}

void pathsetfn(pm,x) /**/
Param pm;char **x;
{
	if (path != x)
		freearray(path);
	path = x;
	newcmdnamtab();
	arrfixenv("PATH",x);
}

void unsettablesetfn(pm,x) /**/
Param pm;char *x;
{ ; }

long poundgetfn(pm) /**/
Param pm;
{
	return arrlen(pparams);
}

long euidgetfn(pm) /**/
Param pm;
{
	return geteuid();
}

void euidsetfn(pm,x) /**/
Param pm;long x;
{
	if (!unsetflag)
		seteuid(x);
}

long randomgetfn(pm) /**/
Param pm;
{
	return rand() & 0x7fff;
}

void randomsetfn(pm,v) /**/
Param pm;long v;
{
	srand((unsigned int) v);
}

long secondsgetfn(pm) /**/
Param pm;
{
	return time(NULL)-shtimer;
}

void secondssetfn(pm,x) /**/
Param pm;long x;
{
	shtimer = x+time(NULL);
}

long uidgetfn(pm) /**/
Param pm;
{
	return getuid();
}

void uidsetfn(pm,x) /**/
Param pm;long x;
{
	if (!unsetflag)
		setuid(x);
}

long gidgetfn(pm) /**/
Param pm;
{
	return getegid();
}

void gidsetfn(pm,x) /**/
Param pm;long x;
{
	if (!unsetflag)
		setegid(x);
}

char *usernamegetfn(pm) /**/
Param pm;
{
struct passwd *pwd;

	pwd = getpwuid(getuid());
	return pwd->pw_name;
}

char *hostgetfn(pm) /**/
Param pm;
{
static char hostnam[65];
static int got = 0;

	if (!got)
		{
		gethostname(hostnam,64);
		hostnam[64] = '\0';
		got = 1;
		}
	return hostnam;
}

char *ifsgetfn(pm) /**/
Param pm;
{
	return ifs;
}

void ifssetfn(pm,x) /**/
Param pm;char *x;
{
	if (x) ifs = x;
	inittyptab();
}

void histsizesetfn(pm,v) /**/
Param pm;long v;
{
	if ((histsiz = v) <= 2)
		histsiz = 2;
}

long histsizegetfn(pm) /**/
Param pm;
{
	return histsiz;
}

void lithistsizesetfn(pm,v) /**/
Param pm;long v;
{
	if ((lithistsiz = v) <= 2)
		lithistsiz = 2;
}

long lithistsizegetfn(pm) /**/
Param pm;
{
	return lithistsiz;
}

void mailchecksetfn(pm,x) /**/
Param pm;long x;
{
	mailcheck = (unsetflag) ? 600 : x;
}

void pathasetfn(pm,x) /**/
Param pm;char **x;
{
	freearray(path);
	path = x;
	newcmdnamtab();
}

char **pathagetfn(pm) /**/
Param pm;
{
	return path;
}

long errnogetfn(pm) /**/
Param pm;
{
	return errno;
}

char *dashgetfn(pm) /**/
Param pm;
{
static char buf[100];
char *val = buf;
int t0;

	for (val = buf, t0 = ' ';t0 <= 'z'; t0++)
		if (opts[t0] == OPT_SET)
			*val++ = t0;
	*val = '\0';
	return buf;
}

char *ttygetfn(pm) /**/
Param pm;
{
	return ttyname(SHTTY);
}

void histcharssetfn(pm,x) /**/
Param pm;char *x;
{
	if (x)
		{
		bangchar = x[0];
		hatchar = (bangchar) ? x[1] : '\0';
		hashchar = (hatchar) ? x[2] : '\0';
		free(x);
		}
}

char *histcharsgetfn(pm) /**/
Param pm;
{
static char buf[4];

	buf[0] = bangchar;
	buf[1] = hatchar;
	buf[2] = hashchar;
	buf[3] = '\0';
	return buf;
}

char *homegetfn(pm) /**/
Param pm;
{
	return home;
}

void homesetfn(pm,x) /**/
Param pm;char *x;
{
	if (home = xsymlink(x))
		free(x);
	else
		home = x;
}

char *wordcharsgetfn(pm) /**/
Param pm;
{
	return wordchars;
}

void wordcharssetfn(pm,x) /**/
Param pm;char *x;
{
	if (x)
		wordchars = x;
	else
		wordchars = ztrdup(DEFWORDCHARS);
	inittyptab();
}

char *underscoregetfn(pm) /**/
Param pm;
{
char *s,*t;

	if (!(s = qgetevent(curhist-1)))
		return "";
	for (t = s+strlen(s); t > s; t--)
		if (*t == HISTSPACE)
			break;
	if (t != s)
		t++;
	return t;
}

char *termgetfn(pm) /**/
Param pm;
{
	return term;
}

void termsetfn(pm,x) /**/
Param pm;char *x;
{
	if (term)
		free(term);
	term = x;
	if (!interact || unset(USEZLE))
		return;
	if (tgetent(termbuf,term) != 1)
		{
		zerr("can't find termcap info for %s",term,0);
		errflag = 0;
		termok = 0;
		}
	else
		{
		char tbuf[1024],*pp;
		int t0;

		termok = 1;
		for (t0 = 0; t0 != TC_COUNT; t0++)
			{
			pp = tbuf;
			if (tcstr[t0])
				free(tcstr[t0]);
			if (!tgetstr(tccapnams[t0],&pp))
				tcstr[t0] = NULL, tclen[t0] = 0;
			else
				{
				tcstr[t0] = zalloc(tclen[t0] = pp-tbuf);
				memcpy(tcstr[t0],tbuf,tclen[t0]);
				}
			}

/* if there's no termcap entry for cursor left, use \b. */

		if (!tccan(TCLEFT))
			{
			tcstr[TCLEFT] = ztrdup("\b");
			tclen[TCLEFT] = 1;
			}

/* if there's no termcap entry for clear, use ^L. */

		if (!tccan(TCCLEARSCREEN))
			{
			tcstr[TCCLEARSCREEN] = ztrdup("\14");
			tclen[TCCLEARSCREEN] = 1;
			}

/* if the termcap entry for down is \n, don't use it. */

		if (tccan(TCDOWN) && tcstr[TCDOWN][0] == '\n')
			{
			tclen[TCDOWN] = 0;
			tcstr[TCDOWN] = NULL;
			}

/* if there's no termcap entry for cursor up, forget it.
	Use single line mode. */

		if (!tccan(TCUP))
			termok = 0;
		}
}

void setparams() /**/
{
char **envp,**envp2,**envp3,*str;
char buf[50];
struct param *pm;
int ct;

	noerrs = 1;
	for (envp = environ, ct = 2; *envp; envp++,ct++);
	envp = environ;
	envp2 = envp3 = (char **) zalloc(sizeof(char *)*ct);
	for (; *envp; envp++)
		*envp2++ = ztrdup(*envp);
	*envp2 = NULL;
	envp = environ;
	environ = envp2 = envp3;
	for (; *envp; envp++,envp2++)
		{
		for (str = *envp; *str && *str != '='; str++);
		if (*str == '=')
			{
			*str = '\0';
			if (isident(*envp))
				pm = setsparam(ztrdup(*envp),ztrdup(str+1));
			*str = '=';
			if (pm)
				{
				pm->flags |= PMFLAG_x;
				pm->env = *envp2;
				}
			}
		}
	pm = gethnode("HOME",paramtab);
	if (!(pm->flags & PMFLAG_x))
		{
		pm->flags |= PMFLAG_x;
		pm->env = addenv("HOME",home);
		}
	pm = gethnode("SHLVL",paramtab);
	if (!(pm->flags & PMFLAG_x))
		pm->flags |= PMFLAG_x;
	sprintf(buf,"%d",++shlvl);
	pm->env = addenv("SHLVL",buf);
	noerrs = 0;
}

char *mkenvstr(x,y) /**/
char *x;char *y;
{
char *z;
int xl = strlen(x),yl = strlen(y);

	z = zalloc(xl+yl+2);
	strcpy(z,x);
	z[xl] = '=';
	strcpy(z+xl+1,y);
	z[xl+yl+1] = '\0';
	return z;
}

void arrfixenv(s,t) /**/
char *s;char **t;
{
char **ep;
int sl = strlen(s);

	for (ep = environ; *ep; ep++)
		if (!strncmp(*ep,s,sl) && (*ep)[sl] == '=')
			{
			char *u = colonjoin(t);

			replenv(*ep,u);
			free(u);
			break;
			}
}

char *replenv(e,value) /**/
char *e;char *value;
{
char **ep;

	for (ep = environ; *ep; ep++)
		if (*ep == e)
			{
			char *s = e;

			while (*s++ != '=');
			*s = '\0';
			*ep = zalloc(strlen(e)+strlen(value)+2);
			strcpy(*ep,e);
			strcat(*ep,value);
			free(e);
			return *ep;
			}
	return NULL;
}

char *addenv(name,value) /**/
char *name;char *value;
{
char **ep,**ep2,**ep3;
int envct;

	for (ep = environ; *ep; ep++)
		{
		char *s = *ep,*t = name;

		while (*s && *s == *t) s++,t++;
		if (*s == '=' && !*t)
			{
			free(*ep);
			return *ep = mkenvstr(name,value);
			}
		}
	envct = arrlen(environ);
	ep = ep2 = (char **) zalloc((sizeof (char *))*(envct+3));
	for (ep3 = environ; *ep2 = *ep3; ep3++,ep2++);
	*ep2 = mkenvstr(name,value);
	ep2[1] = NULL;
	free(environ);
	environ = ep;
	return *ep2;
}

void delenv(x) /**/
char *x;
{
char **ep;

	ep = environ;
	for (; *ep; ep++)
		if (*ep == x)
			break;
	if (*ep)
		for (; ep[0] = ep[1]; ep++);
}

void convbase(s,v,base) /**/
char *s;long v;int base;
{
int digs = 0;
long x;

	if (base <= 1)
		base = 10;
	x = v;
	if (x < 0)
		{
		x = -x;
		digs++;
		}
	for (; x; digs++)
		x /= base;
	if (!digs)
		digs = 1;
	s[digs--] = '\0';
	x = (v < 0) ? -v : v;
	while (digs >= 0)
		{
		s[digs--] = '0'+(x % base);
		x /= base;
		}
	if (v < 0)
		s[0] = '-';
}


