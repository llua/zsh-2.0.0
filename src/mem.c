/*

	mem.c - memory management

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
#define HEAPSIZE 8192

/*

	There are two ways to allocate memory in zsh.  The first way is
	to call zalloc/zcalloc, which call malloc/calloc directly.  It
	is legal to call realloc() or free() on memory allocated this way.
	The second way is to call halloc/hcalloc, which allocates memory
	from one of the memory pools on the heap stack.  A pool can be
	created by calling pushheap(), and destroyed by calling popheap().
	To free the memory in the pool without destroying it, call
	freeheap(); this is equivalent to { popheap(); pushheap(); }
	Memory allocated in this way does not have to be freed explicitly;
	it will all be freed when the pool is destroyed.  In fact,
	attempting to free this memory may result in a core dump.
	The pair of pointers ncalloc and alloc may point to either
	zalloc & zcalloc or halloc & hcalloc; permalloc() sets them to the
	former, and heapalloc() sets them to the latter. This can be useful.
	For example, the dupstruct() routine duplicates a syntax tree,
	allocating the new memory for the tree using alloc().  If you want
	to duplicate a structure for a one-time use (i.e. to execute the list
	in a for loop), call heapalloc(), then dupstruct().  If you want
	to duplicate a structure in order to preserve it (i.e. a function
	definition), call permalloc(), then dupstruct().

*/

/* initialize heap stack */

void meminit() /**/
{
	permalloc();
	heaplist = newlist();
	pushheap();
}

/* set default allocation to heap stack */

void heapalloc() /**/
{
	alloc = hcalloc;
	ncalloc = halloc;
	useheap = 1;
}

static void *(*lastcalloc) DCLPROTO((int));
static void *(*lastncalloc) DCLPROTO((int));

/* set default allocation to malloc() */

void permalloc() /**/
{
	lastcalloc = alloc;
	lastncalloc = ncalloc;
	alloc = zcalloc;
	ncalloc = zalloc;
	useheap = 0;
}

/* reset previous default allocation */

void lastalloc() /**/
{
	alloc = lastcalloc;
	ncalloc = lastncalloc;
}

struct heap {
	char *pool,*ptr;
	int free;
	struct heap *next;
	};

/* create a memory pool */

void pushheap() /**/
{
Heap h;

	h = zalloc(sizeof *h);
	h->pool = h->ptr = zalloc(HEAPSIZE);
	h->free = HEAPSIZE;
	h->next = NULL;
	permalloc();
	pushnode(heaplist,h);
	lastalloc();
}

/* reset a memory pool */

void freeheap() /**/
{
Heap h = peekfirst(heaplist);

	freeh(h->next);
	h->free += (h->ptr-h->pool);
	h->ptr = h->pool;
}

/* destroy a memory pool */

void popheap() /**/
{
Heap h = getnode(heaplist);

	freeh(h);
}

void freeh(h) /**/
Heap h;
{
	if (h)
		{
		freeh(h->next);
		free(h->pool);
		free(h);
		}
}

/* allocate memory from the current memory pool */

void *halloc(size) /**/
int size;
{
Heap h = peekfirst(heaplist),h2;
char *ret;

	size = (size|7)+1;
	while (h && h->free-size < 0)
		h = h->next;
	if (!h)
		{
		h2 = zalloc(sizeof *h2);
		h2->pool = h2->ptr = zalloc(h2->free = 
			(size < HEAPSIZE) ? HEAPSIZE : (size|(HEAPSIZE-1))+1);
		h2->next = h;
		setdata(firstnode(heaplist),h2);
		h = h2;
		}
	h->free -= size;
	ret = h->ptr;
	h->ptr += size;
	return ret;
}

/* allocate memory from the current memory pool and clear it */

void *hcalloc(size) /**/
int size;
{
void *ptr;

	ptr = halloc(size);
	bzero(ptr,size);
	return ptr;
}

void *hrealloc(p,old,new) /**/
char *p;int old;int new;
{
char *ptr;

	ptr = halloc(new);
	memcpy(ptr,p,old);
	return ptr;
}

/* allocate permanent memory */

void *zalloc(l) /**/
int l;
{
void *z;
 
	if (!l)
		l = 1;
	if (!(z = malloc(l)))
		{
		zerr("fatal error: out of memory: restarting",NULL,0);
		execl(MYSELF,"zsh","-f",(void *) 0);
		exit(1);
		}
	return z;
}

void *zcalloc(size) /**/
int size;
{
void *ptr;

	ptr = zalloc(size);
	bzero(ptr,size);
	return ptr;
}

char *strdup(s) /**/
char *s;
{
char *t;

	if (!s)
		return NULL;
	t = ncalloc(strlen(s)+1);
	strcpy(t,s);
	return t;
}

char *ztrdup(s) /**/
char *s;
{
char *t;

	if (!s)
		return NULL;
	t = zalloc(strlen(s)+1);
	strcpy(t,s);
	return t;
}

