/*

	zle_vi.c - One True Editor emulation

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

int vigetkey() /**/
{
int ch;

	if ((ch = getkey(0)) == -1)
		return 0;
	if (ch == 22)
		{
		if ((ch = getkey(0)) == -1)
			return 0;
		return ch;
		}
	else if (ch == 27)
		return 0;
	return ch;
}

int getvirange() /**/
{
int k2,t0;

	for (;;)
		{
		k2 = getkeycmd();
		if (k2 == -1)
			{
			feep();
			return -1;
			}
		if (k2 == z_metafynext || zlecmds[k2].flags & ZLE_ARG)
			zlecmds[k2].func();
		else
			break;
		}
	if (k2 == bindk)
		{
		findline(&cs,&t0);
		return (t0 == ll) ? t0 : t0+1;
		}
	if (zlecmds[k2].flags & ZLE_MOVE)
		{
		t0 = cs;

		zlecmds[k2].func();
		if (cs > t0)
			{
			k2 = cs;
			cs = t0;
			t0 = k2;
			}
		return t0;
		}
	feep();
	return -1;
}

void viaddnext() /**/
{
	if (cs != ll)
		cs++;
	bindtab = mainbindtab;
	insmode = 1;
}

void viaddeol() /**/
{
	cs = findeol();
	bindtab = mainbindtab;
	insmode = 1;
}

void viinsert() /**/
{
	bindtab = mainbindtab;
	insmode = 1;
}

void viinsertbol() /**/
{
	cs = findbol();
	bindtab = mainbindtab;
	insmode = 1;
}

void videlete() /**/
{
int c2;

	if ((c2 = getvirange()) == -1)
		return;
	forekill(c2-cs,0);
}

void vichange() /**/
{
int c2;

	if ((c2 = getvirange()) == -1)
		return;
	forekill(c2-cs-1,0);
	bindtab = mainbindtab;
	insmode = 1;
}

void vichangeeol() /**/
{
	killline();
	bindtab = mainbindtab;
	insmode = 1;
}

void vichangewholeline() /**/
{
int cq;

	findline(&cs,&cq);
	foredel(cq-cs+1);
	bindtab = mainbindtab;
	insmode = 1;
}

void viyank() /**/
{
int c2;

	if ((c2 = getvirange()) == -1)
		return;
	cut(cs,c2-cs,0);
}

void viyankeol() /**/
{
int x = findeol();

	if (x == cs)
		feep();
	else
		cut(cs,x-cs,0);
}

void vigotocolumn() /**/
{
int x,y;

	mult--;
	findline(&x,&y);
	if (y-x < mult)
		feep();
	else
		cs = x+mult;
}

void vireplace() /**/
{
	bindtab = mainbindtab;
	insmode = 0;
}

void vireplacechars() /**/
{
int ch;

	if (mult+cs > ll)
		{
		feep();
		return;
		}
	if (ch = vigetkey())
		while (mult--)
			line[cs++] = ch;
}

static int vfindchar,vfinddir,tailadd;

void vifindnextchar() /**/
{
	if (vfindchar = vigetkey())
		{
		vfinddir = 1;
		tailadd = 0;
		virepeatfind();
		}
}

void vifindprevchar() /**/
{
	if (vfindchar = vigetkey())
		{
		vfinddir = -1;
		tailadd = 0;
		virepeatfind();
		}
}

void vifindnextcharskip() /**/
{
	if (vfindchar = vigetkey())
		{
		vfinddir = 1;
		tailadd = -1;
		virepeatfind();
		}
}

void vifindprevcharskip() /**/
{
	if (vfindchar = vigetkey())
		{
		vfinddir = -1;
		tailadd = 1;
		virepeatfind();
		}
}

void virepeatfind() /**/
{
int ocs = cs;

	while (mult--)
		{
		do
			cs += vfinddir;
		while (cs >= 0 && cs < ll && line[cs] != vfindchar && line[cs] != '\n');
		if (cs < 0 || cs >= ll || line[cs] == '\n')
			{
			feep();
			cs = ocs;
			return;
			}
		}
	cs += tailadd;
}

void virevrepeatfind() /**/
{
	vfinddir = -vfinddir;
	virepeatfind();
	vfinddir = -vfinddir;
}

void vifirstnonblank() /**/
{
	cs = findbol();
	while (cs != ll && iblank(line[cs]))
		cs++;
}

void vifetchhistory() /**/
{
char *s;

	if (histline == curhist)
		{
		if (curhistline)
			free(curhistline);
		curhistline = ztrdup(line);
		}
	if (!(s = qgetevent(mult)))
		feep();
	else
		{
		curhist = mult;
		sethistline(s);
		}
}

void vicmdmode() /**/
{
	bindtab = altbindtab;
}

void viputafter() /**/
{
int cc;

	if (!cutbuf)
		{
		feep();
		return;
		}
	while (mult--)
		{
		cc = strlen(cutbuf);
		spaceinline(cc);
		strncpy(line+cs,cutbuf,cc);
		}
}

void vimatchbracket() /**/
{
int ocs = cs,dir,ct;
char oth,me;

otog:
	if (cs == ll)
		{
		feep();
		cs = ocs;
		return;
		}
	switch(me = line[cs])
		{
		case '{': dir = 1; oth = '}'; break;
		case '}': dir = -1; oth = '{'; break;
		case '(': dir = 1; oth = ')'; break;
		case ')': dir = -1; oth = '('; break;
		case '[': dir = 1; oth = ']'; break;
		case ']': dir = -1; oth = '['; break;
		default: cs++; goto otog;
		}
	ct = 1;
	while (cs >= 0 && cs < ll && ct)
		{
		cs += dir;
		if (line[cs] == oth)
			ct--;
		else if (line[cs] == me)
			ct++;
		}
	if (cs < 0 || cs >= ll)
		{
		feep();
		cs = ocs;
		}
}

void viopenlinebelow() /**/
{
	cs = findeol();
	spaceinline(1);
	line[cs++] = '\n';
	bindtab = mainbindtab;
	insmode = 1;
}

void viopenlineabove() /**/
{
	cs = findbol();
	spaceinline(1);
	line[cs] = '\n';
	bindtab = mainbindtab;
	insmode = 1;
}

void vijoin() /**/
{
int x;

	if ((x = findeol()) == ll)
		{
		feep();
		return;
		}
	cs = x+1;
	for (x = 1; cs != ll && iblank(line[cs]); cs++,x++);
	backdel(x);
	spaceinline(1);
	line[cs] = ' ';
}

void viswapcase() /**/
{
	if (cs < ll)
		{
		int ch = line[cs];

		if (ch >= 'a' && ch <= 'z')
			ch = toupper(ch);
		else if (ch >= 'A' && ch <= 'Z')
			ch = tolower(ch);
		line[cs++] = ch;
		}
}

void vioperswapcase() /**/
{
int c2;

	if ((c2 = getvirange()) == -1)
		return;
	while (cs < c2)
		{
		int ch = line[cs];

		if (ch >= 'a' && ch <= 'z')
			ch = toupper(ch);
		else if (ch >= 'A' && ch <= 'Z')
			ch = tolower(ch);
		line[cs++] = ch;
		}
}

int getvisrchstr() /**/
{
char sbuf[80];
int sptr = 1;

	if (visrchstr)
		{
		free(visrchstr);
		visrchstr = NULL;
		}
	statusline = sbuf;
	sbuf[0] = c;
	sbuf[1] = '\0';
	while (sptr)
		{
		refresh();
		c = getkey(0);
		if (c == '\r' || c == '\n' || c == '\033')
			{
			visrchstr = ztrdup(sbuf+1);
			return 1;
			}
		if (c == '\b')
			{
			sbuf[--sptr] = '\0';
			continue;
			}
		if (sptr != 79)
			{
			sbuf[sptr++] = c;
			sbuf[sptr] = '\0';
			}
		}
	return 0;
}

void vihistorysearchforward() /**/
{
	visrchsense = 1;
	if (getvisrchstr())
		virepeatsearch();
}

void vihistorysearchbackward() /**/
{
	visrchsense = -1;
	if (getvisrchstr())
		virepeatsearch();
}

void virepeatsearch() /**/
{
int ohistline = histline,t0;
char *s;

	if (!visrchstr)
		{
		feep();
		return;
		}
	t0 = strlen(visrchstr);
	if (histline == curhist)
		{
		if (curhistline)
			free(curhistline);
		curhistline = ztrdup(line);
		}
	for (;;)
		{
		histline += visrchsense;
		if (!(s = qgetevent(histline)))
			{
			feep();
			histline = ohistline;
			return;
			}
		if (*visrchstr == '^')
			{
			if (!hstrncmp(s,visrchstr+1,t0-1))
				break;
			}
		else
			if (hstrnstr(s,visrchstr,t0))
				break;
		}
	sethistline(s);
}

void virevrepeatsearch() /**/
{
	visrchsense = -visrchsense;
	virepeatsearch();
	visrchsense = -visrchsense;
}

void vicapslockpanic() /**/
{
char ch;

	statusline = "press a lowercase key to continue";
	refresh();
	do
		ch = getkey(0);
	while (!(ch >= 'a' && ch <= 'z'));
}

