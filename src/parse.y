%{

/*

	parse.y - yacc parser specification

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

struct list *tree;
%}

%left DOITNOW
%left EMPTY LEXERR SEPER NEWLIN SEMI
%left DSEMI AMPER INPAR INBRACE OUTPAR
%right DBAR
%right DAMPER
%right BANG
%left OUTBRACE OUTANG OUTANGBANG DOUTANG DOUTANGBANG INANG DINANG
%left INANGAMP OUTANGAMP OUTANGAMPBANG DOUTANGAMP DOUTANGAMPBANG
%left TRINANG
%left BAR BARAMP DINBRACK DOUTBRACK STRING ENVSTRING
%left ENVARRAY ENDINPUT INOUTPAR
%left DO DONE ESAC THEN ELIF ELSE FI FOR CASE IF WHILE
%left FUNC REPEAT TIME UNTIL EXEC COMMAND SELECT COPROC NOGLOB DASH
%left DOITLATER

%start event

%union {
	Pline Pline;
	List List;
	Sublist Sublist;
	struct cmd *Comm;
	struct redir *Fnode;
	struct cond *Cond;
	struct forcmd *Fornode;
	struct casecmd *Casenode;
	struct ifcmd *Ifnode;
	struct whilecmd *Whilenode;
	struct repeatcmd *Repeatnode;
	struct varasg *Varnode;
	Lklist Table;
	struct fdpair fds;
	char *str;
	int value;
}

%type <List> event list list1 list2
%type <Sublist> sublist sublist2
%type <Pline> pline
%type <Comm> xcommand command simplecommand stufflist 
%type <Cond> cond
%type <Table> redirstring 
%type <Fnode> redir
%type <fds> redirop BAR
%type <fds> OUTANG OUTANGBANG DOUTANG DOUTANGBANG
%type <fds> INANG DINANG INANGAMP OUTANGAMP TRINANG
%type <fds> OUTANGAMPBANG DOUTANGAMP DOUTANGAMPBANG
%type <str> STRING ENVSTRING ENVARRAY word
%type <Table> optinword wordlist
%type <Ifnode> optelsifs 
%type <Casenode> caselist
%%

event	: ENDINPUT { tree = NULL; eofseen = 1; return 0; }
		| SEPER { tree = NULL; lsep = 0; return 0; }
		| list1 { tree = $1; return 0; }
		| error ENDINPUT { errflag = 1; tree = NULL; eofseen = 1; return 1; }
		| error SEPER { errflag = 1; tree = NULL; return 1; }
		| error LEXERR { errflag = 1; tree = NULL; return 1; }
		;

list1	: sublist ENDINPUT { $$ = makelnode($1,SYNC); eofseen = 1; }
		| sublist SEPER { $$ = makelnode($1,SYNC); }
		| sublist AMPER { $$ = makelnode($1,ASYNC); }
		;

list2	: sublist { $$ = makelnode($1,SYNC); }
		;

list	: sublist SEPER list { $$ = makelnode($1,SYNC); $$->right = $3; }
		| sublist AMPER list { $$ = makelnode($1,ASYNC); $$->right = $3; }
		| sublist { $$ = makelnode($1,SYNC); }
		;

sublist	: sublist2
			| sublist DBAR sublist
				{ $$ = $1; $$->right = $3; $$->type = ORNEXT; }
			| sublist DAMPER sublist %prec DBAR 
				{ $$ = $1; $$->right = $3; $$->type = ANDNEXT; }
			;

sublist2	: pline { $$ = makel2node($1,0); }
			| COPROC pline { $$ = makel2node($2,PFLAG_COPROC); }
			| BANG pline { $$ = makel2node($2,PFLAG_NOT); }
			;

pline	: xcommand { $$ = makepnode($1,NULL,END); }
		| xcommand BAR pline 
			{ $$ = makepnode($1,$3,PIPE); }
		| xcommand BARAMP pline 
			{ struct redir *rdr = alloc(sizeof *rdr);
			rdr->type = MERGE; rdr->fd1 = 2; rdr->fd2 = 1;
			addnode($1->redir,rdr); $$ = makepnode($1,$3,PIPE);
			}
		;

xcommand	: redirstring command redirstring
				{ $$ = $2;
				inslist($1,(Lknode) ($2->redir),$2->redir);
				inslist($3,lastnode($2->redir),$2->redir); }
			;

stufflist	: STRING stufflist
					{ $$ = $2; pushnode($$->args,$1); }
				| ENVSTRING stufflist
					{ struct varasg *v = makevarnode(PMFLAG_s);
					equalsplit(v->name = $1,&v->str); $$ = $2;
					pushnode($$->vars,v); }
				| ENVARRAY wordlist OUTPAR stufflist
					{ struct varasg *v = makevarnode(PMFLAG_A);
					v->name = $1; v->arr = $2; $$ = $4;
					pushnode($$->vars,v); }
				| redir stufflist { $$ = $2;
					if ($1->pair) pushnode($2->redir,$1->pair);
					pushnode($2->redir,$1);
					}
				| INOUTPAR INBRACE list OUTBRACE
					{ $$ = makefuncdef(newlist(),$3); }
				| %prec DOITNOW { $$ = makecnode(SIMPLE); }
				;

simplecommand	: COMMAND simplecommand
						{ $$ = $2; $$->flags |= CFLAG_COMMAND; }
					| EXEC simplecommand
						{ $$ = $2; $$->flags |= CFLAG_EXEC; }
					| NOGLOB simplecommand
						{ $$ = $2; $$->flags |= CFLAG_NOGLOB; }
					| DASH simplecommand
						{ $$ = $2; $$->flags |= CFLAG_DASH; }
					| stufflist { $$ = $1;
							if (full($$->args))
								{
								if (underscore)
									free(underscore);
								underscore = ztrdup(getdata(lastnode($$->args)));
								untokenize(underscore);
								}
							}
					;

command	: simplecommand { $$=$1; }
			| FOR STRING optinword SEPER DO list DONE
				{ $$ = makefornode($2,$3,$6,CFOR); }
			| FOR STRING optinword SEPER list2
				{ $$ = makefornode($2,$3,$5,CFOR); }
			| SELECT STRING optinword SEPER DO list DONE
				{ $$ = makefornode($2,$3,$6,CSELECT); }
			| SELECT STRING optinword SEPER list2
				{ $$ = makefornode($2,$3,$5,CSELECT); }
			| CASE word STRING optbreak caselist ESAC
				{ $$ = makecnode(CCASE); $$->u.casecmd = $5;
				addnode($$->args,$2); }
			| IF list THEN list optelsifs FI
				{ $$ = makecnode(CIF); $$->u.ifcmd = makeifnode($2,$4,$5); }
			| WHILE list DO list DONE
				{ $$ = makewhilenode($2,$4,0); }
			| UNTIL list DO list DONE
				{ $$ = makewhilenode($2,$4,1); }
			| REPEAT word SEPER DO list DONE
				{ $$ = makecnode(CREPEAT); $$->u.list = $5;
				addnode($$->args,$2); }
			| REPEAT word list2
				{ $$ = makecnode(CREPEAT); $$->u.list = $3;
				addnode($$->args,$2); }
			| INPAR list OUTPAR
				{ $$ = makecnode(SUBSH); $$->u.list = $2; }
			| INBRACE list OUTBRACE
				{ $$ = makecnode(CURSH); $$->u.list = $2; }
			| FUNC wordlist INBRACE list OUTBRACE
				{ $$ = makefuncdef($2,$4); }
			| TIME sublist2
				{ $$ = makecnode(CTIME); $$->u.pline = $2; }
			| DINBRACK cond DOUTBRACK
				{ $$ = makecnode(COND); $$->u.cond = $2; }
			;

cond	: word word { $$ = makecond(); parcond2($1,$2,$$); }
		| word word word { $$ = makecond(); parcond3($1,$2,$3,$$); }
		| word INANG word
			{ $$ = makecond(); $$->left = $1;
			$$->right = $3; $$->type = COND_STRLT;
			$$->types[0] = $$->types[1] = NT_STR; }
		| word OUTANG word
			{ $$ = makecond(); $$->left = $1;
			$$->right = $3; $$->type = COND_STRGTR;
			$$->types[0] = $$->types[1] = NT_STR; }
		| INPAR cond OUTPAR { $$ = $2; }
		| BANG cond { $$ = makecond(); $$->left = $2; $$->type = COND_NOT; }
		| cond DAMPER cond
			{ $$ = makecond(); $$->left = $1; $$->right = $3; 
			$$->type = COND_AND; }
		| cond DBAR cond
			{ $$ = makecond(); $$->left = $1; $$->right = $3;
			$$->type = COND_OR; }
		;

redir	: redirop word { $$ = parredir($1,$2); }
		;

redirstring : redir redirstring { $$ = $2;
					pushnode($2,$1);
					if ($1->pair) pushnode($2,$1->pair); }
				| %prec DOITNOW { $$ = newlist(); }
				;

redirop	: OUTANG { $$.fd1 = $1.fd1; $$.fd2 = WRITE; }
			| OUTANGBANG { $$.fd1 = $1.fd1; $$.fd2 = WRITENOW; }
			| DOUTANG { $$.fd1 = $1.fd1; $$.fd2 = APP; }
			| DOUTANGBANG { $$.fd1 = $1.fd1; $$.fd2 = APPNOW; }
			| INANG { $$.fd1 = $1.fd1; $$.fd2 = READ; }
			| DINANG { $$.fd1 = $1.fd1; $$.fd2 = HEREDOC; }
			| INANGAMP { $$.fd1 = $1.fd1; $$.fd2 = MERGE; }
			| OUTANGAMP { $$.fd1 = $1.fd1; $$.fd2 = MERGEOUT; }
			| OUTANGAMPBANG { $$.fd1 = $1.fd1; $$.fd2 = MERGEOUTNOW; }
			| DOUTANGAMP { $$.fd1 = $1.fd1; $$.fd2 = ERRAPP; }
			| DOUTANGAMPBANG { $$.fd1 = $1.fd1; $$.fd2 = ERRAPPNOW; }
			| TRINANG { $$.fd1 = $1.fd1; $$.fd2 = HERESTR; }
			;

optinword	: { $$ = NULL; }
				| word wordlist { $$ = $2; pushnode($2,$1); }
				;

optelsifs	: { $$ = NULL; }
				| ELSE list { $$ = makeifnode(NULL,$2,NULL); }
				| ELIF list THEN list optelsifs
					{ $$ = makeifnode($2,$4,$5); $$->ifl = $2; }
				;

caselist	: word OUTPAR list DSEMI optbreak caselist
				{ $$ = makecasenode($1,$3,$6); }
			| word OUTPAR list optbreak
				{ $$ = makecasenode($1,$3,NULL); }
			| { $$ = NULL; }
			;

word	: STRING
		| ENVSTRING
		;

optbreak	:
			| SEPER
			;

wordlist	: word wordlist { $$ = $2; pushnode($2,$1); }
			| { $$ = newlist(); }
			;
%%

/* get fd associated with str */

int getfdstr(s) /**/
char *s;
{
	if (s[1])
		return -1;
	if (idigit(*s))
		return *s-'0';
	if (*s == 'p')
		return -2;
	return -1;
}

struct redir *parredir(fdp,toks) /**/
struct fdpair fdp;char *toks;
{
struct redir *fn = allocnode(N_REDIR);
int mrg2 = 0;

	fn->type = fdp.fd2;
	if (fdp.fd1 != -1)
		fn->fd1 = fdp.fd1;
	else
		if (fn->type < READ)
			fn->fd1 = 1;
		else
			fn->fd1 = 0;
	if ((*toks == Inang || *toks == Outang) && toks[1] == Inpar)
		{
		if (fn->type == WRITE)
			fn->type = OUTPIPE;
		else if (fn->type == READ)
			fn->type = INPIPE;
		else
			{
			zerr("parse error: bad process redirection",NULL,0);
			return fn;
			}
		fn->name = toks;
		}
	else if (fn->type == HEREDOC)
		{
		fn->name = gethere(toks);
		fn->type = HERESTR;
		}
	else if (fn->type >= MERGEOUT && fn->type <= ERRAPPNOW &&
			getfdstr(toks) == -1)
		{
		mrg2 = 1;
		fn->name = toks;
		fn->type = fn->type-MERGEOUT+WRITE;
		}
	else if (fn->type == ERRAPP || fn->type == ERRAPPNOW)
		{
		zerr("parse error: filename expected",NULL,0);
		return fn;
		}
	else if (fn->type == MERGEOUT)
		{
		struct redir *fe = allocnode(N_REDIR);

		fn->type = CLOSE;
		fn->pair = fe;
		fe->fd1 = fn->fd1;
		fe->fd2 = getfdstr(toks);
		if (fe->fd2 == -2)
			fe->fd2 = coprocout;
		fe->type = MERGEOUT;
		}
	else if (fn->type == MERGE || fn->type == MERGEOUT)
		{
		if (*toks == '-')
			fn->type = CLOSE;
		else
			{
			fn->fd2 = getfdstr(toks);
			if (fn->fd2 == -2)
				fn->fd2 = (fn->type == MERGEOUT) ? coprocout : coprocin;
			}
		}
	else
		fn->name = toks;
	if (mrg2)
		{
		struct redir *fe = allocnode(N_REDIR);

		fe->fd1 = 2;
		fe->fd2 = fn->fd1;
		fe->type = MERGEOUT;
		fn->pair = fe;
		}
	return fn;
}

struct list *parev() /**/
{
	yyparse();
	return tree;
}

struct list *parlist() /**/
{
struct list *l,*lp;

	eofseen = 0;
	do
		lp = l = parev();
	while (!lp && !errflag && inbufct);
	while (!eofseen && inbufct && !errflag)
		{
		lsep = 0;
		if (lp->right = parev())
			lp = lp->right;
		}
	return (errflag)? NULL : l;
}

struct list *makelnode(x,type) /**/
struct sublist *x;int type;
{
struct list *n = allocnode(N_LIST);

	n->left = x;
	n->type = type;
	return n;
}

struct sublist *makel2node(p,type) /**/
struct pline *p;int type;
{
struct sublist *n = allocnode(N_SUBLIST);

	n->left = p;
	n->flags = type;
	return n;
}

struct pline *makepnode(c,p,type) /**/
struct cmd *c;struct pline *p;int type;
{
struct pline *n = allocnode(N_PLINE);

	n->left = c;
	n->right = p;
	n->type = type;
	return n;
}

struct cmd *makefornode(str,t,l,type) /**/
char *str;Lklist t;struct list *l;int type;
{
struct cmd *c = makecnode(type);
struct forcmd *f = allocnode(N_FOR);
char *s;

	if (t)
		{
		c->args = t;
		s = ugetnode(t);
		if (strcmp(s,"in"))
			{
			errflag = 1;
			yyerror();
			}
		}
	c->u.forcmd = f;
	f->name = str;
	f->list = l;
	f->inflag = !!t;
	return c;
}

struct ifcmd *makeifnode(l1,l2,i2) /**/
struct list *l1;struct list *l2; struct ifcmd *i2;
{
struct ifcmd *i1 = allocnode(N_IF);

	i1->next = i2;
	i1->ifl = l1;
	i1->thenl = l2;
	return i1;
}

struct cmd *makewhilenode(l1,l2,sense) /**/
struct list *l1;struct list *l2; int sense;
{
struct cmd *c = makecnode(CWHILE);
struct whilecmd *w = allocnode(N_WHILE);

	c->u.whilecmd = w;
	w->cont = l1;
	w->loop = l2;
	w->cond = sense;
	return c;
}

struct cmd *makecnode(type) /**/
int type;
{
struct cmd *c = allocnode(N_CMD);

	c->args = newlist();
	c->redir = newlist();
	c->vars = newlist();
	c->type = type;
	return c;
}

struct cmd *makefuncdef(x,l) /**/
Lklist x;struct list *l;
{
struct cmd *c = makecnode(FUNCDEF);

	c->args = x;
	c->u.list = l;
	return c;
}

struct cond *makecond() /**/
{
struct cond *cn = allocnode(N_COND);

	return cn;
}

struct varasg *makevarnode(type) /**/
int type;
{
struct varasg *v = allocnode(N_VARASG);

	v->type = type;
	return v;
}

struct casecmd *makecasenode(str,l,c) /**/
char *str;struct list *l;struct casecmd *c;
{
struct casecmd *n = allocnode(N_CASE);

	n->next = c;
	n->pat = str;
	n->list = l;
	return n;
}

void parcond2(a,b,n) /**/
char *a;char *b;struct cond *n;
{
	if (a[0] != '-' || !a[1] || a[2])
		{
		zerr("parse error: condition expected: %s",a,0);
		return;
		}
	n->left = b;
	n->type = a[1];
	n->types[0] = n->types[1] = NT_STR;
}

void parcond3(a,b,c,n) /**/
char *a;char *b;char *c;struct cond *n;
{
static char *condstrs[] = {
	"nt","ot","ef","eq","ne","lt","gt","le","ge",NULL
	};
int t0;

	if (b[0] == Equals && !b[1])
		n->type = COND_STREQ;
	else if (b[0] == '!' && b[1] == Equals && !b[2])
		n->type = COND_STRNEQ;
	else if (b[0] == '-')
		{
		for (t0 = 0; condstrs[t0]; t0++)
			if (!strcmp(condstrs[t0],b+1))
				break;
		if (condstrs[t0])
			n->type = t0+COND_NT;
		else
			zerr("unrecognized condition: %s",b,0);
		}
	else
		zerr("condition expected: %s",b,0);
	n->left = a;
	n->right = c;
	n->types[0] = n->types[1] = NT_STR;
}

yyerror() /* NO PROTO */
{
int t0;

	for (t0 = 0; t0 != 20; t0++)
		if (!yytext[t0] || yytext[t0] == '\n' || yytext[t0] == HISTSPACE)
			break;
	if (t0 == 20)
		zerr("parse error near `%l...'",yytext,20);
	else if (t0)
		zerr("parse error near `%l'",yytext,t0);
	else
		zerr("parse error",NULL,0);
}

