/*

	lex.c - lexical analysis

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
#include "y.tab.h"
#include "funcs.h"

/* lexical state */

static int ignl;

static int xignl,xlsep,xincmd,xincond,xinfunc,xinredir,xincase;
static int dbparens,xdbparens,xalstat;
static char *xhlastw;

static int xisfirstln, xisfirstch, xhistremmed, xhistdone,
	xspaceflag, xstophist, xlithist, xalstackind,xhlinesz;
static char *xhline, *xhptr;

static char *tokstr;

/* initialize lexical state */

void lexinit() /**/
{
	ignl = lsep = incmd = incond = infunc = inredir = incase =
		dbparens = alstat = 0;
}

/* save the lexical state */

/* is this a hack or what? */

void lexsave() /**/
{
	xignl = ignl;
	xlsep = lsep;
	xincmd = incmd;
	xincond = incond;
	xinredir = inredir;
	xinfunc = infunc;
	xincase = incase;
	xdbparens = dbparens;
	xalstat = alstat;
	xalstackind = alstackind;
	xisfirstln = isfirstln;
	xisfirstch = isfirstch;
	xhistremmed = histremmed;
	xhistdone = histdone;
	xspaceflag = spaceflag;
	xstophist = stophist;
	xlithist = lithist;
	xhline = hline;
	xhptr = hptr;
	xhlastw = hlastw;
	xhlinesz = hlinesz;
}

/* restore lexical state */

void lexrestore() /**/
{
	ignl = xignl;
	lsep = xlsep;
	incmd = xincmd;
	incond = xincond;
	inredir = xinredir;
	infunc = xinfunc;
	incase = xincase;
	dbparens = xdbparens;
	alstat = xalstat;
	isfirstln = xisfirstln;
	isfirstch = xisfirstch;
	histremmed = xhistremmed;
	histdone = xhistdone;
	spaceflag = xspaceflag;
	stophist = xstophist;
	lithist = xlithist;
	hline = xhline;
	hptr = xhptr;
	hlastw = xhlastw;
	alstackind = xalstackind;
	hlinesz = xhlinesz;
	eofseen = errflag = 0;
}

int yylex() /**/
{
int x;

	for (;;)
		{
		do
			x = gettok();
		while (x != ENDINPUT && exalias(&x));
		if (x == NEWLIN && ignl)
			continue;
		if (x == SEMI || x == NEWLIN)
			{
			if (lsep)
				continue;
			x = SEPER;
			lsep = 1;
			}
		else
			lsep = (x == AMPER);
		break;
		}
	ignl = 0;
	switch (x)
		{
		case OUTPAR: infunc = incmd = incase = 0; break;
		case INPAR:case INBRACE:case DBAR:case DAMPER:case DO:
		case THEN:case ELIF:case BAR:case BARAMP:case IF:case WHILE:
		case ELSE:ignl = 1;infunc = 0;
			case INOUTPAR: case SEPER:
			case AMPER:incmd = 0; break;
		case ESAC: incase = 0;
		case STRING: case ENVARRAY:
			if (!inredir && !infunc) incmd = 1; inredir = 0; break;
		case OUTANG:case OUTANGBANG:case DOUTANG:case INANG:
		case DINANG:case TRINANG:case INANGAMP:case OUTANGAMP:case OUTANGAMPBANG:
		case DOUTANGAMP:case DOUTANGAMPBANG: inredir = 1; break;
		case FUNC:infunc = 1;break;
		case DINBRACK: incond = 1; break;
		case DOUTBRACK: incond = 0; break;
		case DSEMI: ignl = 1; incmd = 0; case CASE: incase = 1; break;
		}
	return x;
}

int len = 0,bsiz = 256;
char *bptr;

/* add a char to the string buffer */

void add(c) /**/
int c;
{
	*bptr++ = c;
	if (bsiz == ++len)
		{
		int newbsiz;

		newbsiz = bsiz * 8;
		while (newbsiz < inbufct)
			newbsiz *= 2;
		bptr = len+(tokstr = hrealloc(tokstr,bsiz,newbsiz));
		bsiz = newbsiz;
		}
}

int gettok() /**/
{
int bct = 0,pct = 0,brct = 0;
int c,d,intpos = 1;
int peekfd = -1,peek,incm;

beginning:
	hlastw = NULL;
	tokstr = NULL;
	incm = incmd || incond || inredir || incase;
	while (iblank(c = hgetc()));
	isfirstln = 0;
	wordbeg = inbufct;
	hwbegin();
	hwaddc(c);
	if (dbparens)	/* handle ((...)) */
		{
		pct = 2;
		peek = STRING;
		len = dbparens = 0;
		bptr = tokstr = ncalloc(bsiz = 256);
		for (;;)
			{
			if (c == '(')
				pct++;
			else if (c == ')')
				pct--;
			else if (c == '\n')
				{
				zerr("parse error: )) expected",NULL,0);
				peek = LEXERR;
				return peek;
				}
			else if (c == '$')
				c = Qstring;
			if (pct >= 2)
				add(c);
			if (pct)
				c = hgetc();
			else
				break;
			}
		*bptr = '\0';
		yylval.str = tokstr;
		return peek;
		}
	if (idigit(c))	/* handle 1< foo */
		{
		d = hgetc();
		hungetc(d);
		if (d == '>' || d == '<')
			{
			peekfd = c-'0';
			c = hgetc();
			}
		}

	/* chars in initial position in word */

	if ((!interact || unset(SHINSTDIN) || strin ||
			isset(INTERACTIVECOMMENTS)) && c == hashchar)
		{
		while ((c = hgetch()) != '\n' && !itok(c) && c != EOF);
		if (c == '\n')
			peek = NEWLIN;
		else
			errflag = 1;
		return peek;
		}
	switch (c)
		{
		case '\\':
			d = hgetc();
			if (d == '\n')
				goto beginning;
			hungetc(d);
			break;
		case EOF:
			peek = ENDINPUT;
			return peek;
		case HERR:
			peek = LEXERR;
			return peek;
		case '\n':
			peek = NEWLIN;
			return peek;
		case ';':
			d = hgetc();
			if (d != ';')
				{
				hungetc(d);
				peek = SEMI;
				}
			else
				peek = DSEMI;
			return peek;
		case '!':
			if (!incm || incond)
				{
				peek = BANG;
				return peek;
				}
			break;
		case '&':
			d = hgetc();
			if (d != '&')
				{
				hungetc(d);
				peek = AMPER;
				}
			else
				peek = DAMPER;
			return peek;
		case '|':
			d = hgetc();
			if (d == '|')
				peek = DBAR;
			else if (d == '&')
				peek = BARAMP;
			else
				{
				hungetc(d);
				peek = BAR;
				}
			return peek;
		case '(':
			d = hgetc();
			if (d == '(' && !incm)
				{
				yylval.str = tokstr = strdup("let");
				dbparens = 1;
				return STRING;
				}
			else if (d == ')')
				return INOUTPAR;
			hungetc(d);
			if (incm && !incond)
				break;
			return INPAR;
		case ')':
			return OUTPAR;
		case '{':
			if (incm)
				break;
			return INBRACE;
		case '}':
			return OUTBRACE;
		case '[':
			if (incm)
				break;
			d = hgetc();
			if (d == '[')
				return DINBRACK;
			hungetc(d);
			break;
		case ']':
			if (!incond)
				break;
			d = hgetc();
			if (d == ']')
				return DOUTBRACK;
			hungetc(d);
			break;
		case '<':
			d = hgetc();
			if ((incmd && d == '(') || incase)
				{
				hungetc(d);
				break;
				}
			else if (d == '<')
				{
				int e = hgetc();

				if (e == '(')
					{
					hungetc(e);
					hungetc(d);
					peek = INANG;
					}
				else if (e == '<')
					peek = TRINANG;
				else
					{
					hungetc(e);
					peek = DINANG;
					}
				}
			else if (d == '&')
				peek = INANGAMP;
			else
				{
				peek = INANG;
				hungetc(d);
				}
			yylval.fds.fd1 = peekfd;
			return peek;
		case '>':
			d = hgetc();
			if (d == '(')
				{
				hungetc(d);
				break;
				}
			else if (d == '&')
				{
				d = hgetc();
				if (d == '!')
					peek = OUTANGAMPBANG;
				else
					{
					hungetc(d);
					peek = OUTANGAMP;
					}
				}
			else if (d == '!')
				peek = OUTANGBANG;
			else if (d == '>')
				{
				d = hgetc();
				if (d == '&')
					{
					d = hgetc();
					if (d == '!')
						peek = DOUTANGAMPBANG;
					else
						{
						hungetc(d);
						peek = DOUTANGAMP;
						}
					}
				else if (d == '!')
					peek = DOUTANGBANG;
				else if (d == '(')
					{
					hungetc(d);
					hungetc('>');
					peek = OUTANG;
					}
				else
					{
					hungetc(d);
					peek = DOUTANG;
					}
				}
			else
				{
				hungetc(d);
				peek = OUTANG;
				}
			yylval.fds.fd1 = peekfd;
			return peek;
		}

	/* we've started a string, now get the rest of it, performing
		tokenization */

	peek = STRING;
	len = 0;
	bptr = tokstr = ncalloc(bsiz = 256);
	for(;;)
		{
		if (c == ';' || c == '&' || c == EOF ||
				c == HERR || inblank(c))
			break;
		if (c == '#')
			c = Pound;
		else if (c == ')')
			{
			if (!pct)
				break;
			pct--;
			c = Outpar;
			}
		else if (c == ',')
			c = Comma;
		else if (c == '|')
			{
			if (!pct && !incase)
				break;
			c = Bar;
			}
		else if (c == '$')
			{
			d = hgetc();

			c = String;
			if (d == '[')
				{
				add(String);
				add(Inbrack);
				while ((c = hgetc()) != ']' && !itok(c) && c != EOF)
					add(c);
				c = Outbrack;
				}
			else if (d == '(')
				{
				add(String);
				skipcomm();
				c = Outpar;
				}
			else
				hungetc(d);
			}
		else if (c == '^')
			c = Hat;
		else if (c == '[')
			{
			brct++;
			c = Inbrack;
			}
		else if (c == ']')
			{
			if (incond && !brct)
				break;
			brct--;
			c = Outbrack;
			}
		else if (c == '*')
			c = Star;
		else if (intpos && c == '~')
			c = Tilde;
		else if (c == '?')
			c = Quest;
		else if (c == '(')
			{
			d = hgetc();
			hungetc(d);
			if (d == ')' || !incm)
				break;
			pct++;
			c = Inpar;
			}
		else if (c == '{')
			{
			c = Inbrace;
			bct++;
			}
		else if (c == '}')
			{
			if (!bct)
				break;
			c = Outbrace;
			bct--;
			}
		else if (c == '>')
			{
			d = hgetc();
			if (d != '(')
				{
				hungetc(d);
				break;
				}
			add(Outang);
			skipcomm();
			c = Outpar;
			}
		else if (c == '<')
			{
			d = hgetc();
			if (!(idigit(d) || d == '-' || d == '>' || d == '(' || d == ')'))
				{
				hungetc(d);
				break;
				}
			c = Inang;
			if (d == '(')
				{
				add(c);
				skipcomm();
				c = Outpar;
				}
			else if (d == ')')
				hungetc(d);
			else
				{
				add(c);
				c = d;
				while (c != '>' && !itok(c) && c != EOF)
					add(c),c = hgetc();
				c = Outang;
				}
			}
		else if (c == '=')
			{
			if (intpos)
				{
				d = hgetc();
				if (d != '(')
					{
					hungetc(d);
					c = Equals;
					}
				else
					{
					add(Equals);
					skipcomm();
					c = Outpar;
					}
				}
			else if (peek != ENVSTRING && !incm)
				{
				d = hgetc();
				if (d == '(' && !incm)
					{
					*bptr = '\0';
					yylval.str = tokstr;
					return ENVARRAY;
					}
				hungetc(d);
				peek = ENVSTRING;
				intpos = 2;
				}
			}
		else if (c == '\\')
			{
			c = hgetc();

			if (c == '\n')
				{
				c = hgetc();
				continue;
				}
			add(c);
			c = hgetc();
			continue;
			}
		else if (c == '\'')
			{
			add(Nularg);

			/* we add the Nularg to prevent this:

			echo $PA'TH'

			from printing the path. */

			while ((c = hgetc()) != '\'' && !itok(c) && c != EOF)
				add(c);
			c = Nularg;
			}
		else if (c == '\"')
			{
			add(Nularg);
			while ((c = hgetc()) != '\"' && !itok(c) && c != EOF)
				if (c == '\\')
					{
					c = hgetc();
					if (c != '\n')
						{
						if (c != '$' && c != '\\' && c != '\"' && c != '`')
							add('\\');
						add(c);
						}
					}
				else
					{
					if (c == '$')
						{
						d = hgetc();
						if (d == '(')
							{
							add(Qstring);
							skipcomm();
							c = Outpar;
							}
						else if (d == '[')
							{
							add(String);
							add(Inbrack);
							while ((c = hgetc()) != ']' && c != EOF)
								add(c);
							c = Outbrack;
							}
						else
							{
							c = Qstring;
							hungetc(d);
							}
						}
					else if (c == '`')
						c = Qtick;
					add(c);
					}
			c = Nularg;
			}
		else if (c == '`')
			{
			add(Tick);
			while ((c = hgetc()) != '`' && !itok(c) && c != EOF)
				if (c == '\\')
					{
					c = hgetc();
					if (c != '\n')
						{
						if (c != '`' && c != '\\' && c != '$')
							add('\\');
						add(c);
						}
					}
				else
					{
					if (c == '$')
						c = String;
					add(c);
					}
			c = Tick;
			}
		add(c);
		c = hgetc();
		if (intpos)
			intpos--;
		}
	if (c == LEXERR)
		{
		peek = LEXERR;
		return peek;
		}
	hungetc(c);
	*bptr = '\0';
	yylval.str = tokstr;
	return peek;
}

/* expand aliases, perhaps */

int exalias(pk) /**/
int *pk;
{
struct alias *an;
char *s,*t;

	if (interact && isset(SHINSTDIN) && !strin && !incase && *pk == STRING)
		{
		int ic = incmd || incond || inredir;

		if (isset(CORRECTALL) || (isset(CORRECT) && !ic))
			spckword(&yylval.str,ic);
		}
	s = yytext = hwadd();
	for (t = s; *t && *t != HISTSPACE; t++);
	if (!*t)
		t = NULL;
	else
		*t = '\0';
	if (zleparse && !alstackind)
		gotword(s);
	an = gethnode(s,aliastab);
	if (t)
		*t = HISTSPACE;
	if (alstackind != MAXAL && an && !an->inuse &&
			!(an->cmd && incmd && alstat != ALSTAT_MORE))
		{
		if (an->cmd < 0)
			{
			*pk = DO-an->cmd-1;
			return 0;
			}
		an->inuse = 1;
		hungets(ALPOPS);
		hungets((alstack[alstackind++] = an)->text);
		alstat = 0;
		return 1;
		}
	return 0;
}

/* skip (...) */

void skipcomm() /**/
{
int pct = 1,c;

	c = Inpar;
	do
		{
		add(c);
		c = hgetc();
		if (itok(c) || c == EOF)
			break;
		else if (c == '(') pct++;
		else if (c == ')') pct--;
		else if (c == '\\')
			{
			add(c);
			c = hgetc();
			}
		else if (c == '\'')
			{
			add(c);
			while ((c = hgetc()) != '\'' && !itok(c) && c != EOF)
				add(c);
			}
		else if (c == '\"')
			{
			add(c);
			while ((c = hgetc()) != '\"' && !itok(c) && c != EOF)
				if (c == '\\')
					{
					add(c);
					add(hgetc());
					}
				else add(c);
			}
		else if (c == '`')
			{
			add(c);
			while ((c = hgetc()) != '`' && c != HERR && c != EOF)
				if (c == '\\') add(c), add(hgetc());
				else add(c);
			}
		}
	while(pct);
}

