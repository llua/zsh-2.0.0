/*

	loop.c - loop execution

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

int execfor(cmd) /**/
Cmd cmd;
{
List list;
struct forcmd *node;
char *str;
Lklist args;
int cj = thisjob;

	loops++;
	exiting = 0;
	node = cmd->u.forcmd;
	args = cmd->args;
	if (!node->inflag)
		{
		char **x;

		args = newlist();
		for (x = pparams; *x; x++)
			addnode(args,ztrdup(*x));
		}
	pushheap();
	while (str = ugetnode(args))
		{
		setsparam(node->name,ztrdup(str));
		list = dupstruct(node->list);
		execlist(list);
		if (breaks)
			{
			breaks--;
			if (breaks || !contflag)
				break;
			contflag = 0;
			}
		if (errflag)
			{
			lastval = 1;
			break;
			}
		freeheap();
		}
	popheap();
	thisjob = cj;
	return lastval;
}

int execselect(cmd) /**/
Cmd cmd;
{
List list;
struct forcmd *node;
char *str,*s;
Lklist args;
Lknode n;
int cj = thisjob,t0;

	loops++;
	node = cmd->u.forcmd;
	args = cmd->args;
	if (!full(args))
		return 1;
	exiting = 0;
	pushheap();
	for (;;)
		{
		do
			{
			selectlist(args);
			if (interact && SHTTY != -1 && isset(USEZLE))
				{
				int pl;

				inittty();
				str = zleread(putprompt(prompt3,&pl),NULL,pl);
				}
			else
				str = fgets(zalloc(256),256,bshin);
			if (!str || errflag)
				{
				fprintf(stderr,"\n");
				fflush(stderr);
				goto done;
				}
			if (s = strchr(str,'\n'))
				*s = '\0';
			}
		while (!*str);
		setsparam("REPLY",ztrdup(str));
		t0 = atoi(str);
		if (!t0)
			str = "";
		else
			{
			for (t0--,n = firstnode(args); n && t0; incnode(n),t0--);
			if (n)
				str = getdata(n);
			else
				str = "";
			}
		setsparam(node->name,ztrdup(str));
		list = dupstruct(node->list);
		execlist(list);
		freeheap();
		if (breaks)
			{
			breaks--;
			if (breaks || !contflag)
				break;
			contflag = 0;
			}
		if (errflag)
			break;
		}
done:
	popheap();
	thisjob = cj;
	return lastval;
}
 
int execwhile(cmd) /**/
Cmd cmd;
{
List list;
struct whilecmd *node;
int cj = thisjob; 

	loops++;
	node = cmd->u.whilecmd;
	exiting = 0;
	pushheap();
	for(;;)
		{
		list = dupstruct(node->cont);
		execlist(list);
		if (!((lastval == 0) ^ node->cond))
			break;
		if (breaks)
			{
			breaks--;
			if (breaks || !contflag)
				break;
			contflag = 0;
			}
		list = dupstruct(node->loop);
		execlist(list);
		freeheap();
		if (errflag)
			{
			lastval = 1;
			break;
			}
		}
	popheap();
	thisjob = cj;
	return lastval;
}
 
int execrepeat(cmd) /**/
Cmd cmd;
{
List list;
int cj = thisjob,count;

	loops++;
	exiting = 0;
	if (!full(cmd->args) || nextnode(firstnode(cmd->args)))
		{
		zerr("bad argument for repeat",NULL,0);
		return 1;
		}
	count = atoi(peekfirst(cmd->args));
	pushheap();
	while (count--)
		{
		list = dupstruct(cmd->u.list);
		execlist(list);
		freeheap();
		if (breaks)
			{
			breaks--;
			if (breaks || !contflag)
				break;
			contflag = 0;
			}
		if (lastval)
			break;
		if (errflag)
			{
			lastval = 1;
			break;
			}
		}
	popheap();
	thisjob = cj;
	return lastval;
}
 
int execif(cmd) /**/
Cmd cmd;
{
struct ifcmd *node;
int cj = thisjob;

	lastval = 0;
	node = cmd->u.ifcmd;
	exiting = 0;
	while (node)
		{
		if (node->ifl)
			{
			execlist(node->ifl);
			if (lastval)
				{
				node = node->next;
				continue;
				}
			}
		execlist(node->thenl);
		break;
		}
	thisjob = cj;
	return lastval;
}
 
int execcase(cmd) /**/
Cmd cmd;
{
struct casecmd *node;
char *word;
Lklist args;
int cj = thisjob;

	node = cmd->u.casecmd;
	args = cmd->args;
	exiting = 0;
	if (firstnode(args) && nextnode(firstnode(args)))
		{
		zerr("bad case statement",NULL,0);
		errflag = 1;
		return 1;
		}
	if (!full(args))
		word = strdup("");
	else
		word = peekfirst(args);
	while (node)
		if (matchpat(word,node->pat))
			break;
		else
			node = node->next;
	if (node)
		execlist(node->list);
	thisjob = cj;
	return lastval;
}
