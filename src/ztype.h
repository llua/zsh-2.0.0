/*

	ztype.h - character classification macros

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

#define IDIGIT  1
#define IALNUM  2
#define IBLANK  4
#define INBLANK 8
#define ITOK    16
#define ISEP    32
#define IALPHA  64
#define IIDENT  128
#define IUSER   256
#define ICNTRL  512
#define IWORD	 1024
#define ISPECIAL 2048
#define _icom(X,Y) (typtab[(int) (unsigned char) (X)] & Y)
#define idigit(X) _icom(X,IDIGIT)
#define ialnum(X) _icom(X,IALNUM)
#define iblank(X) _icom(X,IBLANK)		/* blank, not including \n */
#define inblank(X) _icom(X,INBLANK)		/* blank or \n */
#define itok(X) _icom(X,ITOK)
#define isep(X) _icom(X,ISEP)
#define ialpha(X) _icom(X,IALPHA)
#define iident(X) _icom(X,IIDENT)
#define iuser(X) _icom(X,IUSER)			/* username char */
#define icntrl(X) _icom(X,ICNTRL)
#define iword(X) _icom(X,IWORD)
#define ispecial(X) _icom(X,ISPECIAL)

EXTERN short int typtab[256];

