/*

	cond.c - evaluate conditional expressions

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

#ifndef F_OK
#define F_OK 00
#define R_OK 04
#define W_OK 02
#define X_OK 01
#endif

int evalcond(c) /**/
Cond c;
{
struct stat *st;

	switch (c->type)
		{
		case COND_NOT: return !evalcond(c->left);
		case COND_AND: return evalcond(c->left) && evalcond(c->right);
		case COND_OR: return evalcond(c->left) || evalcond(c->right);
		}
	singsub((char **) &c->left);
	if (c->right)
		singsub((char **) &c->right);
	switch (c->type)
		{
		case COND_STREQ: return matchpat(c->left,c->right);
		case COND_STRNEQ: return !matchpat(c->left,c->right);
		case COND_STRLT: return strcmp(c->left,c->right) < 0;
		case COND_STRGTR: return strcmp(c->left,c->right) > 0;
		case 'a': return(doaccess(c->left,F_OK));
		case 'b': return(S_ISBLK(dostat(c->left)));
		case 'c': return(S_ISCHR(dostat(c->left)));
		case 'd': return(S_ISDIR(dostat(c->left)));
		case 'f': return(S_ISREG(dostat(c->left)));
		case 'g': return(!!(dostat(c->left) & S_ISGID));
		case 'k': return(!!(dostat(c->left) & S_ISVTX));
		case 'n': return(!strlen(c->left));
		case 'o': return(optison(c->left));
		case 'p': return(S_ISFIFO(dostat(c->left)));
		case 'r': return(doaccess(c->left,R_OK));
		case 's': return((st = getstat(c->left)) && !!(st->st_size));
		case 'S': return(S_ISSOCK(dostat(c->left)));
		case 'u': return(!!(dostat(c->left) & S_ISUID));
		case 'w': return(doaccess(c->left,W_OK));
		case 'x': return(doaccess(c->left,X_OK));
		case 'z': return(!!strlen(c->left));
		case 'L': return(S_ISLNK(dostat(c->left)));
		case 'O': return((st = getstat(c->left)) && st->st_uid == geteuid());
		case 'G': return((st = getstat(c->left)) && st->st_gid == getegid());
		case 't': return isatty(matheval(c->left));
		case COND_EQ: return matheval(c->left) == matheval(c->right);
		case COND_NE: return matheval(c->left) != matheval(c->right);
		case COND_LT: return matheval(c->left) < matheval(c->right);
		case COND_GT: return matheval(c->left) > matheval(c->right);
		case COND_LE: return matheval(c->left) <= matheval(c->right);
		case COND_GE: return matheval(c->left) >= matheval(c->right);
		case COND_NT: case COND_OT:
			{
			time_t a;
			if (!(st = getstat(c->left)))
				return 0;
			a = st->st_mtime;
			if (!(st = getstat(c->right)))
				return 0;
			return (c->type == COND_NT) ? a > st->st_mtime : a < st->st_mtime;
			}
		case COND_EF:
			{
			dev_t d;
			ino_t i;

			if (!(st = getstat(c->left)))
				return 0;
			d = st->st_dev;
			i = st->st_ino;
			if (!(st = getstat(c->right)))
				return 0;
			return d == st->st_dev && i == st->st_ino;
			}
		default: zerr("bad cond structure",NULL,0);
		}
	return 0;
}

int doaccess(s,c) /**/
char *s;int c;
{
	return !access(s,c);
}

struct stat *getstat(s) /**/
char *s;
{
static struct stat st;

	if (!strncmp(s,"/dev/fd/",8))
		{
		if (fstat(atoi(s+8),&st))
			return NULL;
		}
	else if (lstat(s,&st))
		return NULL;
	return &st;
}

unsigned short dostat(s) /**/
char *s;
{
struct stat *st;

	if (!(st = getstat(s)))
		return 0;
	return st->st_mode;
}

int optison(s) /**/
char *s;
{
int i;

	if (strlen(s) == 1)
		return opts[*s];
	if ((i = optlookup(s)) != -1)
		return opts[i];
	zerr("no such option: %s",s,0);
	return 0;
}

