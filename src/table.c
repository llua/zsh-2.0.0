/*

	table.c - linked lists and hash tables

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

#define TABLE_C
#include "zsh.h"
#include "funcs.h"

/* get an empty linked list header */

Lklist newlist() /**/
{
Lklist list;
 
	list = alloc(sizeof *list);
	list->first = 0;
	list->last = (Lknode) list;
	return list;
}

/* get an empty hash table */

Hashtab newhtable(size) /**/
int size;
{
Hashtab ret;
 
	ret = zcalloc(sizeof *ret);
	ret->hsize = size;
	ret->nodes = zcalloc(size*sizeof(Hashnode));
	return ret;
}
 
/* Peter Weinberger's hash function */

int hasher(s) /**/
char *s;
{
unsigned hash = 0,g;
 
	for (; *s; s++)
		{
		hash = (hash << 4) + *s;
		if (g = hash & 0xf0000000)
			{
			hash ^= g;
			hash ^= g >> 24;
			}
		}
	return hash;
}

/* add a node to a hash table */

void Addhnode(nam,dat,ht,freefunc,canfree) /**/
char *nam;void *dat;Hashtab ht;FFunc freefunc;int canfree;
{
int hval = hasher(nam) % ht->hsize;
struct hashnode *hp = ht->nodes[hval],*hn;
 
	for (; hp; hp = hp->next)
		if (!strcmp(hp->nam,nam))
			{
			if (!freefunc)
				zerr("attempt to call NULL freefunc",NULL,0);
			else
				freefunc(hp->dat);
			hp->dat = dat;
			if (hp->canfree)
				{
				free(hp->nam);
				hp->nam = nam;
				}
			hp->canfree = canfree;
			return;
			}
	hn = (void *) zcalloc(sizeof *hn);
	hn->nam = nam;
	hn->dat = dat;
	hn->canfree = canfree;
	hn->next = ht->nodes[hval];
	ht->nodes[hval] = hn;
	if (++ht->ct == ht->hsize*4)
		expandhtab(ht);
}

/* expand hash tables when they get too many entries */

void expandhtab(ht) /**/
Hashtab ht;
{
struct hashnode *hp,**arr,**ha,*hn;
int osize = ht->hsize,nsize = osize*8;

	ht->hsize = nsize;
	arr = ht->nodes;
	ht->nodes = zcalloc(nsize*sizeof(struct hashnode *));
	for (ha = arr; osize; osize--,ha++)
		for (hn = *ha; hn; )
			{
			Addhnode(hn->nam,hn->dat,ht,NULL,hn->canfree);
			hp = hn->next;
			free(hn);
			hn = hp;
			}
	free(arr);
}

/* get an entry in a hash table */

void *gethnode(nam,ht) /**/
char *nam;Hashtab ht;
{
int hval = hasher(nam) % ht->hsize;
struct hashnode *hn = ht->nodes[hval];
 
	for (; hn; hn = hn->next)
		if (!strcmp(hn->nam,nam))
			return hn->dat;
	return NULL;
}
 
void freehtab(ht,freefunc) /**/
Hashtab ht;FFunc freefunc;
{
int val;
struct hashnode *hn,**hp = &ht->nodes[0],*next;
 
	for (val = ht->hsize; val; val--,hp++)
		for (hn = *hp; hn; )
			{
			next = hn->next;
			freefunc(hn->dat);
			free(hn);
			hn = next;
			}
}

/* remove a hash table entry and return a pointer to it */

void *remhnode(nam,ht) /**/
char *nam;Hashtab ht;
{
int hval = hasher(nam) % ht->hsize;
struct hashnode *hn = ht->nodes[hval],*hp;
void *dat;

	if (!hn)
		return NULL;
	if (!strcmp(hn->nam,nam))
		{
		ht->nodes[hval] = hn->next;
		dat = hn->dat;
		if (hn->canfree)
			free(hn->nam);
		free(hn);
		ht->ct--;
		return dat;
		}
	for (hp = hn, hn = hn->next; hn; hn = (hp = hn)->next)
		if (!strcmp(hn->nam,nam))
			{
			hp->next = hn->next;
			dat = hn->dat;
			if (hn->canfree)
				free(hn->nam);
			free(hn);
			ht->ct--;
			return dat;
			}
	return NULL;
}

/* insert a node in a linked list after 'llast' */

void insnode(list,llast,dat) /**/
Lklist list;Lknode llast;void *dat;
{
Lknode tmp;
 
	tmp = llast->next;
	llast->next = alloc(sizeof *tmp);
	llast->next->last = llast;
	llast->next->dat = dat;
	llast->next->next = tmp;
	if (tmp)
		tmp->last = llast->next;
	else
		list->last = llast->next;
}

/* remove a node from a linked list */

void *remnode(list,nd) /**/
Lklist list;Lknode nd;
{
void *dat;

	nd->last->next = nd->next;
	if (nd->next)
		nd->next->last = nd->last;
	else
		list->last = nd->last;
	free(nd);
	dat = nd->dat;
	return dat;
}

/* remove a node from a linked list */

void *uremnode(list,nd) /**/
Lklist list;Lknode nd;
{
void *dat;

	nd->last->next = nd->next;
	if (nd->next)
		nd->next->last = nd->last;
	else
		list->last = nd->last;
	dat = nd->dat;
	return dat;
}

/* delete a character in a string */

void chuck(str) /**/
char *str;
{
	while (str[0] = str[1])
		str++;
}

/* get top node in a linked list */

void *getnode(list) /**/
Lklist list;
{
void *dat;
Lknode node = list->first;
 
	if (!node)
		return NULL;
	dat = node->dat;
	list->first = node->next;
	if (node->next)
		node->next->last = (Lknode) list;
	else
		list->last = (Lknode) list;
	free(node);
	return dat;
}

/* get top node in a linked list without freeing */

void *ugetnode(list) /**/
Lklist list;
{
void *dat;
Lknode node = list->first;
 
	if (!node)
		return NULL;
	dat = node->dat;
	list->first = node->next;
	if (node->next)
		node->next->last = (Lknode) list;
	else
		list->last = (Lknode) list;
	return dat;
}

void freetable(tab,freefunc) /**/
Lklist tab;FFunc freefunc;
{
Lknode node = tab->first,next;
 
	while (node)
		{
		next = node->next;
		if (freefunc)
			freefunc(node->dat);
		free(node);
		node = next;
		}
	free(tab);
}
 
char *strstr(s,t) /**/
char *s;char *t;
{
char *p1,*p2;
 
	for (; *s; s++)
		{
		for (p1 = s, p2 = t; *p2; p1++,p2++)
			if (*p1 != *p2)
				break;
		if (!*p2)
			 return (char *) s;
		}
	return NULL;
}

/* insert a list in another list */

void inslist(l,where,x) /**/
Lklist l;Lknode where;Lklist x;
{
Lknode nx = where->next;

	if (!l->first)
		return;
	where->next = l->first;
	l->last->next = nx;
	l->first->last = where;
	if (nx)
		nx->last = l->last;
	else
		x->last = l->last;
}

int countnodes(x) /**/
Lklist x;
{
Lknode y;
int ct = 0;

	for (y = firstnode(x); y; incnode(y),ct++);
	return ct;
}

