/*

	math.c - mathematical expression evaluation

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

static char *ptr;

typedef int LV;

static long yyval;
static LV yylval;

/* nonzero means we are not evaluating, just parsing */

static int noeval = 0;

/* != 0 means recognize unary plus, minus, etc. */

static int unary = 1;

void mathparse DCLPROTO((int));

/* LR = left-to-right associativity
	RL = right-to-left associativity
	BOO = short-circuiting boolean */

enum xtyp { LR,RL,BOOL };

enum xtok {
	INPAR, OUTPAR, NOT, COMP, POSTPLUS,
	POSTMINUS, UPLUS, UMINUS, AND, XOR,
	OR, MUL, DIV, MOD, PLUS,
	MINUS, SHLEFT, SHRIGHT, LES, LEQ,
	GRE, GEQ, DEQ, NEQ, DAND,
	DOR, DXOR, QUEST, COLON, EQ,
	PLUSEQ, MINUSEQ, MULEQ, DIVEQ, MODEQ,
	ANDEQ, XOREQ, OREQ, SHLEFTEQ, SHRIGHTEQ,
	DANDEQ, DOREQ, DXOREQ, COMMA, EOI,
	PREPLUS, PREMINUS, NUM, ID, TOKCOUNT
};

/* precedences */

static int prec[TOKCOUNT] = {
	1,137,2,2,2,
	2,2,2,4,5,
	6,7,7,7,8,
	8,3,3,9,9,
	9,9,10,10,11,
	12,12,13,13,14,
	14,14,14,14,14,
	14,14,14,14,14,
	14,14,14,15,200,
	2,2,0,0,
};

#define TOPPREC 15
#define ARGPREC (15-1)

static int type[TOKCOUNT] = {
	LR,LR,RL,RL,RL,
	RL,RL,RL,LR,LR,
	LR,LR,LR,LR,LR,
	LR,LR,LR,LR,LR,
	LR,LR,LR,LR,BOOL,
	BOOL,LR,RL,RL,RL,
	RL,RL,RL,RL,RL,
	RL,RL,RL,RL,RL,
	BOOL,BOOL,RL,RL,RL,
	RL,RL,LR,LR,
};

#define LVCOUNT 32

/* list of lvalues (variables) */

static int lvc;
static char *lvals[LVCOUNT];

int zzlex() /**/
{
	for(;;)
		switch (*ptr++)
			{
			case '+':
				if (*ptr == '+' && (unary || !ialnum(*ptr)))
					{
					ptr++;
					return (unary) ? PREPLUS : POSTPLUS;
					}
				if (*ptr == '=') { unary = 1; ptr++; return PLUSEQ; }
				return (unary) ? UPLUS : PLUS;
			case '-':
				if (*ptr == '-' && (unary || !ialnum(*ptr)))
					{
					ptr++;
					return (unary) ? PREMINUS : POSTMINUS;
					}
				if (*ptr == '=') { unary = 1; ptr++; return MINUSEQ; }
				return (unary) ? UMINUS : MINUS;
			case '(': unary = 1; return INPAR;
			case ')': return OUTPAR;
			case '!': if (*ptr == '=')
						{ unary = 1; ptr++; return NEQ; }
						return NOT;
			case '~': return COMP;
			case '&': unary = 1;
				if (*ptr == '&') { if (*++ptr == '=')
				{ ptr++; return DANDEQ; } return DAND; }
				else if (*ptr == '=') { ptr++; return ANDEQ; } return AND;
			case '|': unary = 1;
				if (*ptr == '|') { if (*++ptr == '=')
				{ ptr++; return DOREQ; } return DOR; }
				else if (*ptr == '=') { ptr++; return OREQ; } return OR;
			case '^': unary = 1;
				if (*ptr == '^') { if (*++ptr == '=')
				{ ptr++; return DXOREQ; } return DXOR; }
				else if (*ptr == '=') { ptr++; return XOREQ; } return XOR;
			case '*': unary = 1;
				if (*ptr == '=') { ptr++; return MULEQ; } return MUL;
			case '/': unary = 1;
				if (*ptr == '=') { ptr++; return DIVEQ; } return DIV;
			case '%': unary = 1;
				if (*ptr == '=') { ptr++; return MODEQ; } return MOD;
			case '<': unary = 1; if (*ptr == '<')
				{ if (*++ptr == '=') { ptr++; return SHLEFTEQ; } return SHLEFT; }
				else if (*ptr == '=') { ptr++; return LEQ; } return LES;
			case '>': unary = 1; if (*ptr == '>')
				{ if (*++ptr == '=') { ptr++; return SHRIGHTEQ; } return SHRIGHT; }
				else if (*ptr == '=') { ptr++; return GEQ; } return GRE;
			case '=': unary = 1; if (*ptr == '=') { ptr++; return DEQ; }
				return EQ;
			case '?': unary = 1; return QUEST;
			case ':': unary = 1; return COLON;
			case ',': unary = 1; return COMMA;
			case '\0': unary = 1; ptr--; return EOI;
			case '[': unary = 0;
				{ int base = zstrtol(ptr,&ptr,10);
					yyval = zstrtol(ptr+1,&ptr,lastbase = base); return NUM; }
			case ' ': case '\t':
				break;
			default:
				if (idigit(*--ptr))
					{ unary = 0; yyval = zstrtol(ptr,&ptr,10); return NUM; }
				if (ialpha(*ptr) || *ptr == '$')
					{
					char *p,q;

					if (*ptr == '$')
						ptr++;
					p = ptr;
					if (lvc == LVCOUNT)
						{
						zerr("too many identifiers in expression",NULL,0);
						return EOI;
						}
					unary = 0;
					while(ialpha(*++ptr));
					q = *ptr;
					*ptr = '\0';
					lvals[yylval = lvc++] = ztrdup(p);
					*ptr = q;
					return ID;
					}
				return EOI;
			}
}

/* the value stack */

#define STACKSZ 100
int tok;			/* last token */
int sp = -1;	/* stack pointer */
struct mathvalue {
	LV lval;
	long val;
	} stack[STACKSZ];

void push(val,lval) /**/
long val;LV lval;
{
	if (sp == STACKSZ-1)
		zerr("stack overflow",NULL,0);
	else
		sp++;
	stack[sp].val = val;
	stack[sp].lval = lval;
}

long getvar(s) /**/
LV s;
{
long t;

	if (!(t = getiparam(lvals[s])))
		return 0;
	return t;
}

long setvar(s,v) /**/
LV s;long v;
{
	if (s == -1)
		{
		zerr("lvalue required",NULL,0);
		return 0;
		}
	if (noeval)
		return v;
	setiparam(lvals[s],v);
	return v;
}

int notzero(a) /**/
int a;
{
	if (a == 0)
		{
		zerr("division by zero",NULL,0);
		return 0;
		}
	return 1;
}

#define pop2() { b = stack[sp--].val; a = stack[sp--].val; }
#define pop3() {c=stack[sp--].val;b=stack[sp--].val;a=stack[sp--].val;}
#define nolval() {stack[sp].lval= -1;}
#define pushv(X) { push(X,-1); }
#define pop2lv() { pop2() lv = stack[sp+1].lval; }
#define set(X) { push(setvar(lv,X),lv); }

void op(what) /**/
int what;
{
long a,b,c;
LV lv;

	if (sp < 0)
		{
		zerr("stack empty",NULL,0);
		return;
		}
	switch(what) {
		case NOT: stack[sp].val = !stack[sp].val; nolval(); break;
		case COMP: stack[sp].val = ~stack[sp].val; nolval(); break;
		case POSTPLUS: ( void ) setvar(stack[sp].lval,stack[sp].val+1); break;
		case POSTMINUS: ( void ) setvar(stack[sp].lval,stack[sp].val-1); break;
		case UPLUS: nolval(); break;
		case UMINUS: stack[sp].val = -stack[sp].val; nolval(); break;
		case AND: pop2(); pushv(a&b); break;
		case XOR: pop2(); pushv(a^b); break;
		case OR: pop2(); pushv(a|b); break;
		case MUL: pop2(); pushv(a*b); break;
		case DIV: pop2(); if (notzero(b)) pushv(a/b); break;
		case MOD: pop2(); if (notzero(b)) pushv(a%b); break;
		case PLUS: pop2(); pushv(a+b); break;
		case MINUS: pop2(); pushv(a-b); break;
		case SHLEFT: pop2(); pushv(a<<b); break;
		case SHRIGHT: pop2(); pushv(a>>b); break;
		case LES: pop2(); pushv(a<b); break;
		case LEQ: pop2(); pushv(a<=b); break;
		case GRE: pop2(); pushv(a>b); break;
		case GEQ: pop2(); pushv(a>=b); break;
		case DEQ: pop2(); pushv(a==b); break;
		case NEQ: pop2(); pushv(a!=b); break;
		case DAND: pop2(); pushv(a&&b); break;
		case DOR: pop2(); pushv(a||b); break;
		case DXOR: pop2(); pushv(a&&!b||!a&&b); break;
		case QUEST: pop3(); pushv((a)?b:c); break;
		case COLON: break;
		case EQ: pop2lv(); set(b); break;
		case PLUSEQ: pop2lv(); set(a+b); break;
		case MINUSEQ: pop2lv(); set(a-b); break;
		case MULEQ: pop2lv(); set(a*b); break;
		case DIVEQ: pop2lv(); if (notzero(b)) set(a/b); break;
		case MODEQ: pop2lv(); if (notzero(b)) set(a%b); break;
		case ANDEQ: pop2lv(); set(a&b); break;
		case XOREQ: pop2lv(); set(a^b); break;
		case OREQ: pop2lv(); set(a|b); break;
		case SHLEFTEQ: pop2lv(); set(a<<b); break;
		case SHRIGHTEQ: pop2lv(); set(a>>b); break;
		case DANDEQ: pop2lv(); set(a&&b); break;
		case DOREQ: pop2lv(); set(a||b); break;
		case DXOREQ: pop2lv(); set(a&&!b||!a&&b); break;
		case COMMA: pop2(); pushv(b); break;
		case PREPLUS: stack[sp].val = setvar(stack[sp].lval,
			stack[sp].val+1); break;
		case PREMINUS: stack[sp].val = setvar(stack[sp].lval,
			stack[sp].val-1); break;
		default: zerr("out of integers",NULL,0); exit(1);
	}
}

void bop(tk) /**/
int tk;
{
	switch (tk) {
		case DAND: case DANDEQ: if (!stack[sp].val) noeval++; break;
		case DOR: case DOREQ: if (stack[sp].val) noeval++; break;
		};
}

long mathevall(s,prek,ep) /**/
char *s;int prek;char **ep;
{
int t0;

	lastbase = -1;
	for (t0 = 0; t0 != LVCOUNT; t0++)
		lvals[t0] = NULL;
	lvc = 0;
	ptr = s;
	sp = -1;
	unary = 1;
	mathparse(prek);
	*ep = ptr;
	if (sp)
		zerr("unbalanced stack",NULL,0);
	for (t0 = 0; t0 != lvc; t0++)
		free(lvals[t0]);
	return stack[0].val;
}

long matheval(s) /**/
char *s;
{
char *junk;
long x;

	if (!*s)
		return 0;
	x = mathevall(s,TOPPREC,&junk);
	if (*junk)
		zerr("illegal character: %c",NULL,*junk);
	return x;
}

long mathevalarg(s,ss) /**/
char *s;char **ss;
{
long x;

	x = mathevall(s,ARGPREC,ss);
	if (tok == COMMA)
		(*ss)--;
	return x;
}

/* operator-precedence parse the string and execute */

void mathparse(pc) /**/
int pc;
{
	if (errflag)
		return;
	tok = zzlex();
	while (prec[tok] <= pc)
		{
		if (errflag)
			return;
		if (tok == NUM)
			push(yyval,-1);
		else if (tok == ID)
			push(getvar(yylval),yylval);
		else if (tok == INPAR)
			{
			mathparse(TOPPREC);
			if (tok != OUTPAR)
				exit(1);
			}
		else if (tok == QUEST)
			{
			int q = stack[sp].val;
			if (!q) noeval++;
			mathparse(prec[QUEST]-1);
			if (!q) noeval--; else noeval++;
			mathparse(prec[QUEST]);
			if (q) noeval--;
			op(QUEST);
			continue;
			}
		else
			{
			int otok = tok,onoeval = noeval;

			if (type[otok] == BOOL)
				bop(otok);
			mathparse(prec[otok]-(type[otok] != RL));
			noeval = onoeval;
			op(otok);
			continue;
			}
		tok = zzlex();
		}
}

