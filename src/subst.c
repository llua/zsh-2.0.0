/*

	subst.c - various substitutions

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

/* do substitutions before fork */

void prefork(list) /**/
Lklist list;
{
Lknode node = firstnode(list);
int qt;

	while (node)
		{
		char *str,*str3;
		
		str = str3 = getdata(node);
		if (str[1] == Inpar && (*str == Inang ||
				*str == Outang || *str == Equals))
			{
			if (*str == Inang)
				setdata(node,getoutproc(str+2));		/* <(...) */
			else if (*str == Equals)
				setdata(node,getoutputfile(str+2));	/* =(...) */
			else
				setdata(node,getinproc(str+2));		/* >(...) */
			if (!getdata(node))
				{
				zerr("parse error in process substitution",NULL,0);
				return;
				}
			}
		else while (*str)
			{
			if ((qt = *str == Qstring) || *str == String)
				if (str[1] != Inpar)
					if (str[1] == Inbrack)
						{
						arithsubst((void **) &str,&str3);	/* $[...] */
						setdata(node,str3);
						}
					else
						{
						paramsubst(list,node,str,str3,qt);
						if (errflag)
							return;
						str3 = str = getdata(node);
						continue;
						}
			str++;
			if (errflag)
				return;
			}
		if (*(char *) getdata(node))
			remnulargs(getdata(node));
		if (unset(IGNOREBRACES))
			while (hasbraces(getdata(node)))
				xpandbraces(list,&node);
		filesub((char **) getaddrdata(node));
		if (errflag)
			return;
		incnode(node);
		}
}

void postfork(list,doglob) /**/
Lklist list;int doglob;
{
Lknode node = firstnode(list);
int glb = 1;

	if (isset(NOGLOBOPT) || !doglob)
		glb = 0;
	while (node)
		{
		char *str3,*str;
		
		str = str3 = getdata(node);
		while (*str)
			{
			if (((*str == String || *str == Qstring) && str[1] == Inpar) ||
					*str == Tick || *str == Qtick)
				{
				Lknode n = prevnode(node);

				commsubst(list,node,str,str3,
					(*str == Qstring || *str == Qtick));	/* `...`,$(...) */
				if (errflag)
					return;
				str = str3 = getdata(node = nextnode(n));
				}
			str++;
			}
		if (glb)
			{
			if (haswilds(getdata(node)))
				glob(list,&node);
			if (errflag)
				return;
			}
		incnode(node);
		}
}

/* perform substitution on a single word */

void singsub(s) /**/
char **s;
{
Lklist foo;
char *t;

	for (t = *s; *t; t++)
		if (*t == String)
			*t = Qstring;
		else if (*t == Tick)
			*t = Qtick;
	foo = newlist();
	addnode(foo,*s);
	prefork(foo);
	if (errflag)
		return;
	postfork(foo,0);
	if (errflag)
		return;
	*s = ugetnode(foo);
	if (firstnode(foo))
		zerr("ambiguous: %s",*s,0);
}

/* strdup, but returns "Nularg" if this is a null string */

void *nstrdup(s) /**/
void *s;
{
char *t = s;
char u[2];

	u[0] = Nularg; u[1] = '\0';
	if (!*t)
		return strdup(u);
	return strdup(t);
}

char *dynread(stop) /**/
int stop;
{
int bsiz = 256,ct = 0,c;
char *buf = zalloc(bsiz),*ptr;
 
	ptr = buf;
	while ((c = hgetc()) != stop)
		{
		*ptr++ = c;
		if (++ct == bsiz)
			{
			buf = realloc(buf,bsiz *= 2);
			ptr = buf+ct;
			}
		}
	*ptr = 0;
	return buf;
}

int filesub(namptr) /**/
char **namptr;
{
char *str = *namptr,*cnam;

	if (*str == Tilde && str[1] != '=')
		{
		if (str[1] == '+')
			{
			char *foo = strdup(cwd);	/* ~+ */

			str+=2;
			modify(&foo,&str);
			*namptr = dyncat(cwd,str);
			return 1;
			}
		else if (str[1] == '-')			/* ~- */
			{
			char *foo;

			if (cnam = oldpwd)
				foo = cnam;
			else
				foo = cwd;
			str += 2;
			foo = strdup(foo);
			modify(&foo,&str);
			*namptr = dyncat(foo,str);
			return 1;
			}
		if (ialpha(str[1]))		/* ~foo */
			{
			char *ptr,*hom;
 
			for (ptr = ++str; *ptr && iuser(*ptr); ptr++)
				if (*ptr == '-')
					*ptr = '-';
			if (!(hom = gethome(str,ptr-str)))
				{
				zerr("user not found: %l",str,ptr-str);
				errflag = 1;
				return 0;
				}
			modify(&hom,&ptr);
			*namptr = dyncat(hom,ptr);
			return 1;
			}
		else if (str[1] == '/')	/* ~/foo */
			{
			*namptr = dyncat(home,str+1);
			return 1;
			}
		else if (!str[1])		/* ~ by itself */
			{
			*namptr = strdup(home);
			return 1;
			}
		}
	if (*str == Equals && iuser(str[1]))
		{
		char *ptr,*s,*ds;
		int val;
		
		untokenize(str);
		if (ialpha(str[1]))		/* =foo */
			{
			struct cmdnam *chn;
			struct alias *t;
			char sav,*pp;
 
			for (pp = str+1; *pp && *pp != ':'; pp++);
			sav = *pp;
			*pp = '\0';
			if ((t = gethnode(str+1,aliastab)) && t->cmd)
				if (t->cmd >= 0)
					cnam = strdup(t->text);
				else
					{
					zerr("%s: shell reserved word",str+1,0);
					errflag = 1;
					return 0;
					}
		 	else if (chn = gethnode(str+1,cmdnamtab))
				if (chn->type != BUILTIN && chn->type != SHFUNC)
					cnam = strdup(chn->u.nam);
				else
					{
					zerr((chn->type == BUILTIN) ?
						"shell built-in command: %s" :
						"shell function: %s",str+1,0);
					errflag = 1;
					return 0;
					}
			else if (!(cnam = findcmd(str+1)))
				{
				zerr("%s not found",str+1,0);
				errflag = 1;
				return 0;
				}
			*namptr = cnam;
			if ((*pp = sav) == ':')
				{
				modify(namptr,&pp);
				s = *namptr;
				*namptr = dyncat(*namptr,pp);
				}
			return 1;
			}
		if (str[1] == '-') 	/* =- */
			{
			val = -1;
			ptr = str+2;
			}
		else
			val = strtol(str+1,&ptr,10);	/* =# */
		ds = dstackent(val);
		if (!ds)
			return 1;
		s = strdup(ds);
		modify(&s,&ptr);
		*namptr = dyncat(s,ptr);
		return 1;
		}
	return 0;
}

/* get a named directory */

char *gethome(user,len) /**/
char *user;int len;
{
char sav,*str;
struct passwd *pw;
 
	sav = user[len];
	user[len] = '\0';
	if (str = getsparamval(user,len))
		{
		str = strdup(str);
		adduserdir(user,str);
		user[len] = sav;
		return str;
		}
	if (!(pw = getpwnam(user)))
		{
		user[len] = sav;
		return NULL;
		}
	str = xsymlink(pw->pw_dir);
	adduserdir(user,str);
	user[len] = sav;
	return str;
}

/* `...`, $(...) */

void commsubst(l,n,str3,str,qt) /**/
Lklist l;Lknode n;char *str3;char *str;int qt;
{
char *str2;
Lknode where = prevnode(n);
Lklist pl;

	if (*str3 == Tick || *str3 == Qtick)
		{
		*str3 = '\0';
		for (str2 = ++str3; *str3 != Tick && *str3 != Qtick; str3++);
		*str3++ = '\0';
		}
	else
		{
		*str3++ = '\0';
		for (str2 = ++str3; *str3 != Outpar; str3++);
		*str3++ = '\0';
		}
	uremnode(l,n);
	if (!(pl = getoutput(str2,qt)))
		{
		zerr("parse error in command substitution",NULL,0);
		errflag = 1;
		return;
		}
	if (full(pl))
		{
		setdata(firstnode(pl),dyncat(str,peekfirst(pl)));
		setdata(lastnode(pl),dyncat(getdata(lastnode(pl)),str3));
		inslist(pl,where,l);
		}
	else
		insnode(l,where,dyncat(str,str3));
}

/* parameter substitution */

void paramsubst(l,n,aptr,bptr,qt) /**/
Lklist l;Lknode n;char *aptr;char *bptr;int qt;
{
char *s = aptr,*u,*idbeg,*idend,*ostr = bptr;
int brs;			/* != 0 means ${...}, otherwise $... */
int colf;		/* != 0 means we found a colon after the name */
int doub = 0;	/* != 0 means we have %%, not %, or ##, not # */
int isarr = 0;
int plan9 = isset(RCEXPANDPARAM);
int getlen = 0;
int vunset = 0;
int spbreak = isset(SHWORDSPLIT) && !qt;
char *val = NULL,**aval = NULL;
int fwidth = 0;
Value v;

	*s++ = '\0';
	if (!ialnum(*s) && *s != '#' && *s != Pound && *s != '-' &&
			*s != '!' && *s != '$' && *s != String && *s != Qstring &&
			*s != '?' && *s != Quest && *s != '_' &&
			*s != '*' && *s != Star && *s != '@' && *s != '{' &&
			*s != Inbrace && *s != '=' && *s != Hat && *s != '^')
		{
		s[-1] = '$';
		return;
		}
	if (brs = (*s == '{' || *s == Inbrace))
		s++;
	for (;;)
		if (*s == '^' || *s == Hat)
			plan9 ^= 1,s++;
		else if (*s == '=')
			spbreak ^= 1,s++;
		else if ((*s == '#' || *s == Pound) && iident(s[1]))
			getlen = 1,s++;
		else
			break;

	idbeg = s;
	if (!(v = getvalue(&s,1)))
		{
		vunset = 1;
		idend = s;
		}
	else
		if (isarr = v->isarr)
			aval = getarrvalue(v);
		else
			{
			val = getstrvalue(v);
			fwidth = v->pm->ct;
			switch (v->pm->flags & (PMFLAG_L | PMFLAG_R | PMFLAG_Z))
				{
				char *t;
				int t0;

				case PMFLAG_L:
				case PMFLAG_L|PMFLAG_Z:
					t = val;
					if (v->pm->flags & PMFLAG_Z)
						while (*t == '0') t++;
					else
						while (isep(*t)) t++;
					val = ncalloc(fwidth+1);
					val[fwidth] = '\0';
					strcpy(val,t);
					if ((t0 = strlen(t)) > fwidth)
						t0 = fwidth;
					memset(val,' ',fwidth);
					strncpy(val,t,t0);
					break;
				case PMFLAG_R:
				case PMFLAG_Z:
				case PMFLAG_Z|PMFLAG_R:
					if (strlen(val) < fwidth)
						{
						t = ncalloc(fwidth+1);
						memset(t,(v->pm->flags & PMFLAG_R) ? ' ' : '0',fwidth);
						if ((t0 = strlen(val)) > fwidth)
							t0 = fwidth;
						strcpy(t+(fwidth-t0),val);
						val = t;
						}
					break;
				}
			switch (v->pm->flags & (PMFLAG_l | PMFLAG_u))
				{
				char *t;

				case PMFLAG_l:
					t = val;
					for (;*t;t++)
						*t = tolower(*t);
					break;
				case PMFLAG_u:
					t = val;
					for (;*t;t++)
						*t = toupper(*t);
					break;
				}
			}
	if (colf = *s == ':')
		s++;
	
	/* check for ${..?...} or ${..=..} or one of those.  Only works
		if the name is in braces. */

	if (brs && (*s == '-' || *s == '=' || *s == '?' || *s == '+' || *s == '#' ||
			*s == '%' || *s == Quest || *s == Pound))
		{
		if (v && v->isarr)
			{
			zerr("operator requires a scalar",NULL,0);
			return;
			}
		if (*s == s[1])
			{
			s++;
			doub = 1;
			}
		u = ++s;
		if (brs)
			while (*s != '}' && *s != Outbrace)
				s++;
		else
			{
			while (*s++);
			s--;
			}
		*s = '\0';
		if (colf && !vunset)
			vunset = !*val;
		switch (u[-1])
			{
			case '-':
				if (vunset)
					val = strdup(u);
				break;
			case '=':
				if (vunset)
					{
					char sav = *idend;

					*idend = '\0';
					setsparam(idbeg,ztrdup(val = strdup(u)));
					*idend = sav;
					}
				break;
			case '?':
			case Quest:
				if (vunset)
					{
					zerr("%s",(*u) ? u : "parameter not set",0);
					if (!interact)
						exit(1);
					return;
					}
				break;
			case '+':
				if (vunset)
					val = strdup("");
				else
					val = strdup(u);
				break;
			case '#':
			case Pound:
				if (vunset)
					val = strdup("");
				getmatch(&val,u,doub);
				break;
			case '%':
				if (vunset)
					val = strdup("");
				getmatch(&val,u,doub+2);
				break;
			}
		}
	else		/* no ${...=...} or anything, but possible modifiers. */
		{
		if (vunset)
			{
			if (isset(NOUNSET))
				{
				zerr("parameter not set",NULL,0);
				return;
				}
			val = strdup("");
			}
		if (colf)
			{
			s--;
			if (!isarr)
				modify(&val,&s);
			/*else
				modifyarr(&aval,&s);*/
			}
		if (brs)
			{
			if (*s != '}' && *s != Outbrace)
				{
				zerr("closing brace expected",NULL,0);
				errflag = 1;
				return;
				}
			s++;
			}
		}
	if (errflag)
		return;
	if (getlen)
		{
		long len = 0;
		char buf[14];

		if (isarr)
			{
			char **ctr;
			for (ctr = aval; *ctr; ctr++,len++);
			}
		else
			len = strlen(val);
		sprintf(buf,"%ld",len);
		val = strdup(buf);
		isarr = 0;
		}
	if (isarr && !plan9)
		if (!aval || !aval[0])
			{
			val = strdup("");
			isarr = 0;
			}
		else if (!aval[1])
			{
			val = aval[0];
			isarr = 0;
			}
	if (qt)
		if (isarr)
			{
			val = spacejoin(aval);
			isarr = 0;
			}
		else if (spbreak)
			{
			if (isarr)
				val = spacejoin(aval);
			isarr = 1;
			aval = spacesplit(s);
			}
	if (isarr)
		if (plan9)
			{
			int dlen;
			char *y;

			y = ncalloc((dlen = (char *) aptr-bptr+strlen(s)+1)+strlen(aval[0]));
			setdata(n,y);
			strcpy(y,ostr);
			strcat(y,aval[0]);
			strcat(y,s);
			while (*++aval)
				{
				char *x = ncalloc(dlen+strlen(*aval));

				strcpy(x,ostr);
				strcat(x,*aval);
				strcat(x,s);
				insnode(l,n,x), incnode(n);
				}
			}
		else
			{
			char *zz;

			zz = ncalloc((char *) aptr-(bptr)+strlen(aval[0])+1);
			setdata(n,zz);
			strcpy(zz,ostr);
			strcat(zz,*aval++);
			while (aval[1])
				insnode(l,n,*aval++), incnode(n);
			zz = ncalloc(strlen(*aval)+strlen(s)+1);
			strcpy(zz,*aval);
			strcat(zz,s);
			insnode(l,n,zz);
			}
	else
		{
		bptr = ncalloc((char *) aptr-bptr+strlen(val)+strlen(s)+1);
		setdata(n,bptr);
		strcpy(bptr,ostr);
		strcat(bptr,val);
		strcat(bptr,s);
		}
}

/* arithmetic substitution */

void arithsubst(aptr,bptr) /**/
void **aptr;char **bptr;
{
char *s = *aptr,*t,buf[16];
long v;

	*s = '\0';
	for (; *s != Outbrack; s++);
	*s++ = '\0';
	v = matheval((char *) *aptr+2);
	sprintf(buf,"%ld",v);
	t = ncalloc(strlen(*bptr)+strlen(buf)+strlen(s)+1);
	strcpy(t,*bptr);
	strcat(t,buf);
	strcat(t,s);
	*bptr = t;
}

void modify(str,ptr) /**/
char **str;char **ptr;
{
char *ptr1,*ptr2,*ptr3,del,*lptr;
int gbal;

	if (**ptr == ':')
		*str = strdup(*str);
	while (**ptr == ':')
		{
		lptr = *ptr;
		(*ptr)++;
		gbal = 0;
here:
		switch(*(*ptr)++)
			{
			case 'h': remtpath(str); break;
			case 'r': remtext(str); break;
			case 'e': rembutext(str); break;
			case 't': remlpaths(str); break;
			case 'l': downcase(str); break;
			case 'u': upcase(str); break;
			case 's':
				if (hsubl)
					free(hsubl);
				if (hsubr)
					free(hsubr);
				ptr1 = *ptr;
				del = *ptr1++;
				for (ptr2 = ptr1; *ptr2 != del && *ptr2; ptr2++);
				if (!*ptr2)
					{
					zerr("bad subtitution",NULL,0);
					errflag = 1;
					return;
					}
				*ptr2++ = '\0';
				for (ptr3 = ptr2; *ptr3 != del && *ptr3; ptr3++);
				if (*ptr3)
					*ptr3++ = '\0';
				hsubl = ztrdup(ptr1);
				hsubr = ztrdup(ptr2);
				*ptr = ptr3;
			case '&':
				if (hsubl && hsubr)
					subst(str,hsubl,hsubr,gbal);
				break;
			case 'g': gbal = 1; goto here;
			default: *ptr = lptr; return;
			}
		}
}

/* get a directory stack entry */

char *dstackent(val) /**/
int val;
{
Lknode node;
 
	if ((val < 0 && !firstnode(dirstack)) || !val--)
		return cwd;
	if (val < 0)
		node = lastnode(dirstack);
	else
		for (node = firstnode(dirstack); node && val; val--,incnode(node));
	if (!node)
		{
		zerr("not enough dir stack entries.",NULL,0);
		errflag = 1;
		return NULL;
		}
	return getdata(node);
}

/* make an alias hash table node */

struct alias *mkanode(txt,cmflag) /**/
char *txt;int cmflag;
{
struct alias *ptr = (void *) zcalloc(sizeof *ptr);

	ptr->text  = txt;
	ptr->cmd = cmflag;
	ptr->inuse = 0;
	return ptr;
}
