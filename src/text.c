/*

	text.c - textual representations of syntax trees

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

static char *tptr,*tbuf,*tlim;
static int tsiz,tindent,tnewlins;

/* add a character to the text buffer */

void taddchr(c) /**/
int c;
{
	*tptr++ = c;
	if (tptr == tlim)
		{
		tbuf = realloc(tbuf,tsiz *= 2);
		tlim = tbuf+tsiz;
		tptr = tbuf+tsiz/2;
		}
}

/* add a string to the text buffer */

void taddstr(s) /**/
char *s;
{
int sl = strlen(s);

	while (tptr+sl >= tlim)
		{
		int x = tptr-tbuf;

		tbuf = realloc(tbuf,tsiz *= 2);
		tlim = tbuf+tsiz;
		tptr = tbuf+x;
		}
	strcpy(tptr,s);
	tptr += sl;
}

/* add an integer to the text buffer */

void taddint(x) /**/
int x;
{
char buf[10];

	sprintf(buf,"%d",x);
	taddstr(buf);
}

/* add a newline, or something equivalent, to the text buffer */

void taddnl() /**/
{
int t0;

	if (tnewlins)
		{
		taddchr('\n');
		for (t0 = 0; t0 != tindent; t0++)
			taddchr('\t');
		}
	else
		taddstr("; ");
}

/* get a textual representation of n */

char *gettext(n,nls) /**/
struct node *n;int nls;
{
	tnewlins = nls;
	tbuf = zalloc(tsiz = 32);
	tptr = tbuf;
	tlim = tbuf+tsiz;
	tindent = 1;
	gettext2(n);
	*tptr = '\0';
	untokenize(tbuf);
	return tbuf;
}

#define gt2(X) gettext2((struct node *) (X))

/*
	"gettext2" or "type checking and how to avoid it"
	an epic function by Paul Falstad
*/

#define _Cond(X) ((Cond) (X))
#define _Cmd(X) ((Cmd) (X))
#define _Pline(X) ((Pline) (X))
#define _Sublist(X) ((Sublist) (X))
#define _List(X) ((List) (X))
#define _casecmd(X) ((struct casecmd *) (X))
#define _ifcmd(X) ((struct ifcmd *) (X))
#define _whilecmd(X) ((struct whilecmd *) (X))

void gettext2(n) /**/
struct node *n;
{
Cmd nn;
Cond nm;

	if (!n)
		return;
	switch (n->type)
		{
		case N_LIST:
			gt2(_List(n)->left);
			if (_List(n)->type == ASYNC)
				taddstr(" &");
			simplifyright(_List(n));
			if (_List(n)->right)
				{
				if (tnewlins)
					taddnl();
				else
					taddstr((_List(n)->type == ASYNC) ? " " : "; ");
				gt2(_List(n)->right);
				}
			break;
		case N_SUBLIST:
			if (_Sublist(n)->flags & PFLAG_NOT)
				taddstr("! ");
			if (_Sublist(n)->flags & PFLAG_COPROC)
				taddstr("coproc ");
			gt2(_Sublist(n)->left);
			if (_Sublist(n)->right)
				{
				taddstr((_Sublist(n)->type == ORNEXT) ? " || " : " && ");
				gt2(_Sublist(n)->right);
				}
			break;
		case N_PLINE:
			gt2(_Pline(n)->left);
			if (_Pline(n)->type == PIPE)
				{
				taddstr(" | ");
				gt2(_Pline(n)->right);
				}
			break;
		case N_CMD:
			nn = _Cmd(n);
			if (nn->flags & CFLAG_EXEC)
				taddstr("exec ");
			if (nn->flags & CFLAG_COMMAND)
				taddstr("command ");
			switch (nn->type)
				{
				case SIMPLE:
					getsimptext(nn);
					break;
				case SUBSH:
					taddstr("( ");
					tindent++;
					gt2(nn->u.list);
					tindent--;
					taddstr(" )");
					break;
				case CTIME:
					taddstr("time ");
					tindent++;
					gt2(nn->u.pline);
					tindent--;
					break;
				case FUNCDEF:
					taddlist(nn->args);
					taddstr(" () {");
					tindent++;
					taddnl();
					gt2(nn->u.list);
					tindent--;
					taddnl();
					taddstr("}");
					break;
				case CURSH:
					taddstr("{ ");
					tindent++;
					gt2(nn->u.list);
					tindent--;
					taddstr(" }");
					break;
				case CFOR:
				case CSELECT:
					taddstr((nn->type == CFOR) ? "for " : "select ");
					taddstr(nn->u.forcmd->name);
					if (nn->u.forcmd->inflag)
						{
						taddstr(" in ");
						taddlist(nn->args);
						}
					taddnl();
					taddstr("do");
					tindent++;
					taddnl();
					gt2(nn->u.forcmd->list);
					taddnl();
					tindent--;
					taddstr("done");
					break;
				case CIF:
					gt2(nn->u.ifcmd);
					taddstr("fi");
					break;
				case CCASE:
					taddstr("case ");
					taddlist(nn->args);
					taddstr(" in");
					tindent++;
					taddnl();
					gt2(nn->u.casecmd);
					tindent--;
					if (tnewlins)
						taddnl();
					else
						taddchr(' ');
					taddstr("esac");
					break;
				case COND:
					taddstr("[[ ");
					gt2(nn->u.cond);
					taddstr(" ]]");
					break;
				case CREPEAT:
					taddstr("repeat ");
					taddlist(nn->args);
					taddnl();
					taddstr("do");
					tindent++;
					taddnl();
					gt2(nn->u.list);
					tindent--;
					taddnl();
					taddstr("done");
					break;
				case CWHILE:
					gt2(nn->u.whilecmd);
					break;
				}
			getredirs(nn);
			break;
		case N_COND:
			nm = _Cond(n);
			switch (nm->type)
				{
				case COND_NOT:
					taddstr("! ");
					gt2(nm->left);
					break;
				case COND_AND:
					taddstr("( ");
					gt2(nm->left);
					taddstr(" && ");
					gt2(nm->right);
					taddstr(" )");
					break;
				case COND_OR:
					taddstr("( ");
					gt2(nm->left);
					taddstr(" || ");
					gt2(nm->right);
					taddstr(" )");
					break;
				default:
					{
					static char *c1[] = {
						" = "," != "," < "," > "," -nt "," -ot "," -ef "," -eq ",
						" -ne "," -lt "," -gt "," -le "," -ge "
						};
					if (nm->right)
						taddstr(nm->left);
					if (nm->type <= COND_GE)
						taddstr(c1[nm->type]);
					else
						{
						char c2[5];
						c2[0] = ' '; c2[1] = '-';
						c2[2] = nm->type;
						c2[3] = ' '; c2[4] = '\0';
						taddstr(c2);
						}
					taddstr((nm->right) ? nm->right : nm->left);
					}
					break;
				}
			break;
		case N_CASE:
			taddstr(_casecmd(n)->pat);
			taddstr(") ");
			tindent++;
			gt2(_casecmd(n)->list);
			tindent--;
			gt2(_casecmd(n)->next);
			break;
		case N_IF:
			if (_ifcmd(n)->ifl)
				{
				taddstr("if ");
				tindent++;
				gt2(_ifcmd(n)->ifl);
				tindent--;
				taddnl();
				taddstr("then");
				}
			else
				taddchr('e');
			tindent++;
			taddnl();
			gt2(_ifcmd(n)->thenl);
			tindent--;
			taddnl();
			if (_ifcmd(n)->next)
				{
				taddstr("els");
				gt2(_ifcmd(n)->next);
				}
			break;
		case N_WHILE:
			taddstr((_whilecmd(n)->cond) ? "until " : "while ");
			tindent++;
			gt2(_whilecmd(n)->cont);
			tindent--;
			taddnl();
			taddstr("do");
			tindent++;
			taddnl();
			gt2(_whilecmd(n)->loop);
			tindent--;
			taddnl();
			taddstr("done");
			break;
		}
}

void getsimptext(cmd) /**/
Cmd cmd;
{
Lknode n;

	for (n = firstnode(cmd->vars); n; incnode(n))
		{
		struct varasg *v = getdata(n);

		taddstr(v->name);
		taddchr('=');
		if ((v->type & PMTYPE) == PMFLAG_A)
			{
			taddchr('(');
			taddlist(v->arr);
			taddstr(") ");
			}
		else
			{
			taddstr(v->str);
			taddchr(' ');
			}
		}
	taddlist(cmd->args);
}

void getredirs(cmd) /**/
Cmd cmd;
{
Lknode n;
static char *fstr[] = {
	">",">!",">>",">>!",">&",">&!",">>&",">>&!","<","<<","<&",">&",">&-",
	">&-","..","..","<<<"
	};

	taddchr(' ');
	for (n = firstnode(cmd->redir); n; incnode(n))
		{
		struct redir *f = getdata(n);

		switch(f->type)
			{
			case WRITE: case WRITENOW: case APP: case APPNOW: case READ:
			case HERESTR:
				if (f->fd1 != ((f->type == READ) ? 0 : 1))
					taddchr('0'+f->fd1);
				taddstr(fstr[f->type]);
				taddchr(' ');
				taddstr(f->name);
				taddchr(' ');
				break;
			case MERGE: case MERGEOUT:
				if (f->fd1 != ((f->type == MERGEOUT) ? 1 : 0))
					taddchr('0'+f->fd1);
				taddstr(fstr[f->type]);
				taddchr(' ');
				taddint(f->fd2);
				taddchr(' ');
				break;
			case CLOSE:
				taddchr(f->fd1+'0');
				taddstr(">&- ");
				break;
			case INPIPE:
			case OUTPIPE:
				if (f->fd1 != ((f->type == INPIPE) ? 0 : 1))
					taddchr('0'+f->fd1);
				taddstr((f->type == INPIPE) ? "< " : "> ");
				taddstr(f->name);
				taddchr(' ');
				break;
			}
		}
	tptr--;
}

void taddlist(l) /**/
Lklist l;
{
Lknode n;

	for (n = firstnode(l); n; incnode(n))
		{
		taddstr(getdata(n));
		taddchr(' ');
		}
	tptr--;
}
