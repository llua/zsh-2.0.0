/*

	zle_tricky.c - expansion and completion

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

#define ZLE
#include "zsh.h"
#include "funcs.h"
#include "y.tab.h"
#include <sys/dir.h>

static int we,wb;

static int menub,menue;
static Lklist menulist;
static Lknode menunode;

#define INCMD (incmd||incond||inredir||incase)

#define inststr(X) inststrlen((X),-1)

static int usetab()
{
char *s = line+cs-1;

	for (; s >= line && *s != '\n'; s--)
		if (*s != '\t' && *s != ' ')
			return 0;
	return 1;
}

enum xcompcmd {
	COMP_COMPLETE = 0,COMP_LIST_COMPLETE,
	COMP_SPELL, COMP_EXPAND, COMP_EXPAND_COMPLETE,
	COMP_LIST_EXPAND
	};

void completeword() /**/
{
	if (c == '\t' && usetab())
		selfinsert();
	else
		docomplete(COMP_COMPLETE);
}

void listchoices() /**/
{
	docomplete(COMP_LIST_COMPLETE);
}

void spellword() /**/
{
	docomplete(COMP_SPELL);
}

void deletecharorlist() /**/
{
	if (cs != ll)
		deletechar();
	else
		docomplete(COMP_LIST_COMPLETE);
}

void expandword() /**/
{
	if (c == '\t' && usetab())
		selfinsert();
	else
		docomplete(COMP_EXPAND);
}

void expandorcomplete() /**/
{
	if (c == '\t' && usetab())
		selfinsert();
	else
		docomplete(COMP_EXPAND_COMPLETE);
}

void listexpand() /**/
{
	docomplete(COMP_LIST_EXPAND);
}

void reversemenucomplete() /**/
{
char *s;

	if (!menucmp)
		feep();
	cs = menub;
	foredel(menue-menub);
	if (menunode == firstnode(menulist))
		menunode = lastnode(menulist);
	else
		menunode = prevnode(menunode);
	inststr(s = menunode->dat);
	menue = cs;
}

void docomplete(lst) /**/
int lst;
{
int t0,lincmd = INCMD;
char *s;

	if (menucmp)
		{
		cs = menub;
		foredel(menue-menub);
		incnode(menunode);
		if (!menunode)
			menunode = firstnode(menulist);
		inststr(s = menunode->dat);
		menue = cs;
		return;
		}
	if (doexpandhist())
		return;
	zleparse = 1;
	eofseen = 0;
	lexsave();
	hungets(" "); /* KLUDGE! */
	hungets(line);
	strinbeg();
	pushheap();
	while (!eofseen && zleparse)
		{
		lincmd = INCMD;
		if ((t0 = yylex()) == ENDINPUT)
			break;
		}
	if (t0 == ENDINPUT)
		{
		s = ztrdup("");
		we = wb = cs;
		t0 = STRING;
		}
	else if (t0 == STRING)
		s = ztrdup(yylval.str);
	hflush();
	strinend();
	errflag = zleparse = 0;
	if (we > ll)
		we = ll;
	if (t0 != STRING)
		feep();
	else
		{
		if (lst == COMP_SPELL)
			{
			untokenize(s);
			cs = wb;
			foredel(we-wb);
			inststr(spname(s));
			}
		else if (lst >= COMP_EXPAND)
			doexpansion(s,lst,lincmd);
		else
			{
			untokenize(s);
			docompletion(s,lst,lincmd);
			}
		free(s);
		}
	popheap();
	lexrestore();
}

void doexpansion(s,lst,lincmd) /**/
char *s;int lst;int lincmd;
{
Lklist vl = newlist();
char *ss;

	pushheap();
	addnode(vl,s);
	prefork(vl);
	if (errflag)
		goto end;
	postfork(vl,1);
	if (errflag)
		goto end;
	if (lst == COMP_LIST_EXPAND)
		{
		listmatches(vl,NULL);
		goto end;
		}
	else if (peekfirst(vl) == s) 
		{
		if (lst == COMP_EXPAND_COMPLETE)
			{
			untokenize(s);
			docompletion(s,COMP_COMPLETE,lincmd);
			}
		else
			feep();
		goto end;
		}
	cs = wb;
	foredel(we-wb);
	while (ss = ugetnode(vl))
		{
		inststr(ss);
		if (full(vl))
			{
			spaceinline(1);
			line[cs++] = ' ';
			}
		}
end:
	popheap();
	setterm();
}

void gotword(s) /**/
char *s;
{
	we = ll+1-inbufct;
	if (cs <= we)
		{
		wb = ll-wordbeg;
		zleparse = 0;
		/* major hack ahead */
		if (wb && line[wb] == '!' && line[wb-1] == '\\')
			wb--;
		}
}

void inststrlen(s,l) /**/
char *s;int l;
{
char *t,*u,*v;

	t = zalloc(strlen(s)*2+2);
	u = s;
	v = t;
	for (; *u; u++)
		{
		if (l != -1 && !l--)
			break;
		if (ispecial(*u))
			if (*u == '\n')
				{
				*v++ = '\'';
				*v++ = '\n';
				*v++ = '\'';
				continue;
				}
			else
				*v++ = '\\';
		*v++ = *u;
		}
	*v = '\0';
	spaceinline(strlen(t));
	strncpy(line+cs,t,strlen(t));
	cs += strlen(t);
	free(t);
}

static int ambig,haspath,exact;
static Lklist matches;
static char *pat;

void addmatch(s) /**/
char *s;
{
	if (full(matches))
		{
		int y = pfxlen(peekfirst(matches),s);

		if (y < ambig)
			ambig = y;
		}
	else
		ambig = strlen(s);
	if (!strcmp(pat,s))
		exact = 1;
	addnode(matches,strdup(s));
}

void addcmdmatch(s,t) /**/
char *s;char *t;
{
	if (strpfx(pat,s))
		addmatch(s);
}

void docompletion(s,lst,incmd) /**/
char *s;int lst;int incmd;
{
DIR *d;
struct direct *de;
char *t = NULL,*u,*ppfx;
int commcomp;

	heapalloc();
	pushheap();
	matches = newlist();
	haspath = exact = 0;
	for (u = s+strlen(s); u >= s && *u != '/'; u--);
	if (u >= s)
		{
		*u++ = '\0';
		haspath = 1;
		}
	else
		u = s;
	pat = u;
	if (commcomp = !incmd && !haspath)
		{
		listhtable(aliastab ,addcmdmatch);
		listhtable(cmdnamtab,addcmdmatch);
		}
	if (d = opendir(ppfx = ((haspath) ? ((*s) ? s : "/") : ".")))
		{
		char *q;

		readdir(d); readdir(d);
		while (de = readdir(d))
			if (strpfx(pat,q = de->d_name))
				{
				int namlen = strlen(q);
				char **pt = fignore;
		
				if (!(*q != '.' || *u == '.' || isset(GLOBDOTS)))
					continue;
				for (; *pt; pt++)
					if (strlen(*pt) < namlen && !strcmp(q+namlen-strlen(*pt),*pt))
						break;
				if (!*pt)
					addmatch(q);
				}
		closedir(d);
		}
	if (!full(matches))
		feep();
	else if (lst == COMP_LIST_COMPLETE)
		listmatches(matches,
					unset(NICEAPPENDAGES) ? NULL : (haspath) ? s : "./");
	else if (nextnode(firstnode(matches)))
		{
		if (isset(MENUCOMPLETE))
			{
			feep();
			menucmp = 1;
			cs = wb;
			foredel(we-wb);
			if (haspath)
				{
				inststr(s);
				spaceinline(1);
				line[cs++] = '/';
				}
			menub = cs;
			inststr(peekfirst(matches));
			menue = cs;
			menulist = duplist(matches,ztrdup);
			menunode = firstnode(menulist);
			popheap();
			permalloc();
			return;
			}
		cs = wb;
		foredel(we-wb);
		if (haspath)
			{
			inststr(s);
			spaceinline(1);
			line[cs++] = '/';
			}
		if (isset(RECEXACT) && exact)
			{
			inststr(u);
			spaceinline(1);
			line[cs++] = (isdir(u,ppfx) ? '/' : ' ');
			}
		else
			{
			inststrlen(peekfirst(matches),ambig);
			refresh();
			feep();
			if (isset(AUTOLIST))
				listmatches(matches,
					unset(NICEAPPENDAGES) ? NULL : (haspath) ? s : "./");
			}
		}
	else
		{
		cs = wb;
		foredel(we-wb);
		if (haspath)
			{
			inststr(s);
			spaceinline(1);
			line[cs++] = '/';
			}
		inststr(peekfirst(matches));
		spaceinline(1);
		line[cs++] = (!commcomp && isdir(peekfirst(matches),ppfx) ? '/' : ' ');
		}
	popheap();
	permalloc();
}

int strpfx(s,t) /**/
char *s;char *t;
{
	while (*s && *s == *t) s++,t++;
	return !*s;
}

int pfxlen(s,t) /**/
char *s;char *t;
{
int i = 0;

	while (*s && *s == *t) s++,t++,i++;
	return i;
}

int isdir(s,pfx) /**/
char *s;char *pfx;
{
struct stat sbuf;
char buf[MAXPATHLEN];

	sprintf(buf,"%s/%s",pfx,s);
	if (stat(buf,&sbuf) == -1)
		return 0;
	return S_ISDIR(sbuf.st_mode);
}

void listmatches(l,apps) /**/
Lklist l;char *apps;
{
int longest = 1,fct,fw = 0,colsz,t0,t1,ct;
Lknode n;
char **arr,**ap;

	trashzle();
	ct = countnodes(l);
	if (listmax && ct > listmax)
		{
		fprintf(stdout,"zsh: do you wish to see all %d possibilities? ",ct);
		fflush(stdout);
		feep();
		if (getquery() != 'y')
			return;
		}
	ap = arr = alloc((countnodes(l)+1)*sizeof(char **));
	for (n = firstnode(l); n; incnode(n))
		*ap++ = getdata(n);
	*ap = NULL;
	for (ap = arr; *ap; ap++)
		if (strlen(*ap) > longest)
			longest = strlen(*ap);
	if (apps)
		longest++;
	qsort(arr,ct,sizeof(char *),forstrcmp);
	fct = (columns-1)/(longest+2);
	if (fct == 0)
		fct = 1;
	else
		fw = (columns-1)/fct;
	colsz = (ct+fct-1)/fct;
	for (t1 = 0; t1 != colsz; t1++)
		{
		ap = arr+t1;
		if (apps)
			do
				{
				int t2 = strlen(*ap)+1;
				char pbuf[MAXPATHLEN];
				struct stat buf;

				printf("%s",*ap);
				sprintf(pbuf,"%s/%s",apps,*ap);
				if (lstat(pbuf,&buf))
					putchar(' ');
				else switch (buf.st_mode & S_IFMT) /* screw POSIX */
					{
					case S_IFDIR: putchar('/'); break;
					case S_IFIFO: putchar('|'); break;
					case S_IFCHR: putchar('%'); break;
					case S_IFBLK: putchar('#'); break;
					case S_IFLNK: putchar('@'); break;
					case S_IFSOCK: putchar('='); break;
					default:
						if (buf.st_mode & 0111)
							putchar('*');
						else
							putchar(' ');
						break;
					}
				for (; t2 < fw; t2++) putchar(' ');
				for (t0 = colsz; t0 && *ap; t0--,ap++);
				}
			while (*ap);
		else
			do
				{
				int t2 = strlen(*ap);

				printf("%s",*ap);
				for (; t2 < fw; t2++) putchar(' ');
				for (t0 = colsz; t0 && *ap; t0--,ap++);
				}
			while (*ap);
		putchar('\n');
		}
	resetneeded = 1;
	fflush(stdout);
}

void selectlist(l) /**/
Lklist l;
{
int longest = 1,fct,fw = 0,colsz,t0,t1,ct;
Lknode n;
char **arr,**ap;

	trashzle();
	ct = countnodes(l);
	ap = arr = alloc((countnodes(l)+1)*sizeof(char **));
	for (n = firstnode(l); n; incnode(n))
		*ap++ = getdata(n);
	*ap = NULL;
	for (ap = arr; *ap; ap++)
		if (strlen(*ap) > longest)
			longest = strlen(*ap);
	t0 = ct;
	longest++;
	while (t0)
		t0 /= 10, longest++;
	qsort(arr,ct,sizeof(char *),forstrcmp);
	fct = (columns-1)/(longest+2);
	if (fct == 0)
		fct = 1;
	else
		fw = (columns-1)/fct;
	colsz = (ct+fct-1)/fct;
	for (t1 = 0; t1 != colsz; t1++)
		{
		ap = arr+t1;
		do
			{
			int t2 = strlen(*ap)+1,t3;

			fprintf(stderr,"%d %s",t3 = ap-arr+1,*ap);
			while (t3)
				t2++,t3 /= 10;
			for (; t2 < fw; t2++) fputc(' ',stderr);
			for (t0 = colsz; t0 && *ap; t0--,ap++);
			}
		while (*ap);
		fputc('\n',stderr);
		}
	resetneeded = 1;
	fflush(stderr);
}

int doexpandhist() /**/
{
char *cc,*ce;
int t0;

	for (cc = line, ce = line+ll; cc < ce; cc++)
		if (*cc == bangchar || *cc == hatchar)
			break;
	if (cc == ce)
		return 0;
	zleparse = 1;
	eofseen = 0;
	lexsave();
	hungets(line);
	strinbeg();
	pushheap();
	ll = cs = 0;
	for(;;)
		{
		t0 = hgetc();
		if (t0 == EOF || t0 == HERR)
			break;
		spaceinline(1);
		line[cs++] = t0;
		}
	hflush();
	popheap();
	strinend();
	errflag = zleparse = 0;
	t0 = histdone;
	lexrestore();
	line[ll = cs] = '\0';
	return t0;
}

void magicspace() /**/
{
	doexpandhist();
	c = ' ';
	selfinsert();
}

void expandhistory() /**/
{
	if (!doexpandhist())
		feep();
}

char *getcurcmd() /**/
{
int t0,lincmd = INCMD;
char *s = NULL;

	zleparse = 1;
	eofseen = 0;
	lexsave();
	hungets(" "); /* KLUDGE! */
	hungets(line);
	strinbeg();
	pushheap();
	while (!eofseen && zleparse)
		{
		if ((t0 = yylex()) == ENDINPUT)
			break;
		else if (t0 == STRING && !lincmd)
			{
			if (s)
				free(s);
			s = ztrdup(yylval.str);
			}
		lincmd = INCMD;
		}
	hflush();
	popheap();
	strinend();
	errflag = zleparse = 0;
	lexrestore();
	return s;
}

void processcmd() /**/
{
char *s;

	s = getcurcmd();
	if (s)
		{
		char *t;

		t = zlecmds[bindk].name;
		mult = 1;
		pushline();
		sizeline(strlen(s)+strlen(t)+1);
		strcpy(line,t);
		strcat(line," ");
		cs = ll = strlen(line);
		inststr(s);
		free(s);
		done = 1;
		}
	else
		feep();
}

void freemenu() /**/
{
	menucmp = 0;
	freetable(menulist,freestr);
}
