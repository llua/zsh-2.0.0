/*

	zle_emacs.c - eight megabytes and constantly swapping

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

void beginningofline() /**/
{
	while (mult--)
		{
		if (cs == 0)
			return;
		if (line[cs-1] == '\n')
			if (!--cs)
				return;
		while (cs && line[cs-1] != '\n') cs--;
		}
}

void endofline() /**/
{
	while (mult--)
		{
		if (cs == ll)
			return;
		if (line[cs] == '\n')
			if (++cs == ll)
				return;
		while (cs != ll && line[cs] != '\n') cs++;
		}
}

void forwardchar() /**/
{
	if ((cs += mult) > ll) cs = ll;
}

void backwardchar() /**/
{
	if ((cs -= mult) < 0) cs = 0;
}

void selfinsert() /**/
{
	if (insmode)
		spaceinline(mult);
	else if (mult+cs > ll)
		spaceinline(ll-(mult+cs));
	while (mult--)
		line[cs++] = c;
}

void selfinsertunmeta() /**/
{
	c &= 0x7f;
	if (c == '\r') c = '\n';
	if (insmode)
		spaceinline(mult);
	else if (mult+cs > ll)
		spaceinline(ll-(mult+cs));
	while (mult--)
		line[cs++] = c;
}

void deletechar() /**/
{
	if (c == 4 && !ll)
		{
		eofsent = 1;
		return;
		}
	if (cs+mult > ll)
		{
		feep();
		return;
		}
	cs += mult;
	backdel(mult);
}

void backwarddeletechar() /**/
{
	if (mult > cs)
		mult = cs;
	backdel(mult);
}

void killwholeline() /**/
{
int i,fg;

	while (mult--)
		{
		if (fg = (cs && cs == ll))
			cs--;
		while (cs && line[cs-1] != '\n') cs--;
		for (i = cs; i != ll && line[i] != '\n'; i++);
		forekill(i-cs+(i != ll),fg);
		}
}

void killbuffer() /**/
{
	cs = 0;
	forekill(ll,0);
}

void backwardkillline() /**/
{
int i = 0;

	while (mult--)
		{
		while (cs && line[cs-1] != '\n') cs--,i++;
		if (mult && cs && line[cs-1] == '\n')
			cs--,i++;
		}
	forekill(i,1);
}

void setmarkcommand() /**/
{
	mark = cs;
}

void exchangepointandmark() /**/
{
int x;

	x = mark;
	mark = cs;
	cs = x;
	if (cs > ll)
		cs = ll;
}

void forwardword() /**/
{
	while (mult--)
		{
		while (cs != ll && iword(line[cs])) cs++;
		while (cs != ll && !iword(line[cs])) cs++;
		}
}

void viforwardwordend() /**/
{
	while (mult--)
		{
		while (cs != ll && !iword(line[cs+1])) cs++;
		while (cs != ll && iword(line[cs+1])) cs++;
		}
}

void backwardword() /**/
{
	while (mult--)
		{
		while (cs && !iword(line[cs-1])) cs--;
		while (cs && iword(line[cs-1])) cs--;
		}
}

void backwarddeleteword() /**/
{
int x = cs;

	while (mult--)
		{
		while (x && !iword(line[x-1])) x--;
		while (x && iword(line[x-1])) x--;
		}
	backdel(cs-x);
}

void backwardkillword() /**/
{
int x = cs;

	while (mult--)
		{
		while (x && !iword(line[x-1])) x--;
		while (x && iword(line[x-1])) x--;
		}
	backkill(cs-x,1);
}

void gosmacstransposechars() /**/
{
int cc;

	if (cs < 2 || line[cs-1] == '\n' || line[cs-2] == '\n')
		{
		if (line[cs] == '\n' || line[cs+1] == '\n')
			{
			feep();
			return;
			}
		cs += (cs == 0 || line[cs-1] == '\n') ? 2 : 1;
		}
	cc = line[cs-2];
	line[cs-2] = line[cs-1];
	line[cs-1] = cc;
}

void transposechars() /**/
{
int cc;

	while (mult--)
		{
		if (cs == 0 || line[cs-1] == '\n')
			{
			if (ll == cs || line[cs] == '\n' || line[cs+1] == '\n')
				{
				feep();
				return;
				}
			cs++;
			}
		if (cs != ll && line[cs] != '\n')
			cs++;
		cc = line[cs-2];
		line[cs-2] = line[cs-1];
		line[cs-1] = cc;
		}
}

void acceptline() /**/
{
	done = 1;
}

void acceptandhold() /**/
{
	pushnode(bufstack,ztrdup(line));
	stackcs = cs;
	done = 1;
}

void upcaseword() /**/
{
	while (mult--)
		{
		while (cs != ll && !iword(line[cs])) cs++;
		while (cs != ll && iword(line[cs]))
			{
			line[cs] = toupper(line[cs]);
			cs++;
			}
		}
}

void downcaseword() /**/
{
	while (mult--)
		{
		while (cs != ll && !iword(line[cs])) cs++;
		while (cs != ll && iword(line[cs]))
			{
			line[cs] = tolower(line[cs]);
			cs++;
			}
		}
}

void capitalizeword() /**/
{
int first;
	
	while (mult--)
		{
		first = 1;
		while (cs != ll && !iword(line[cs])) cs++;
		while (cs != ll && iword(line[cs]))
			{
			line[cs] = (first) ? toupper(line[cs]) : tolower(line[cs]);
			first = 0;
			cs++;
			}
		}
}

void killline() /**/
{
int i = 0;

	while (mult--)
		{
		if (line[cs] == '\n')
			cs++,i++;
		while (cs != ll && line[cs] != '\n') cs++,i++;
		}
	backkill(i,0);
}

void killregion() /**/
{
	if (mark > ll)
		mark = ll;
	if (mark > cs)
		forekill(mark-cs,0);
	else
		backkill(cs-mark,1);
}

void copyregionaskill() /**/
{
	if (mark > ll)
		mark = ll;
	if (mark > cs)
		cut(cs,mark-cs,0);
	else
		cut(mark,cs-mark,1);
}

void deleteword() /**/
{
int x = cs;

	while (mult--)
		{
		while (x != ll && !iword(line[x])) x++;
		while (x != ll && iword(line[x])) x++;
		}
	foredel(x-cs);
}

void transposewords() /**/
{
int p1,p2,p3,p4,x = cs;
char *temp,*pp;

	while (mult--)
		{
		while (x != ll && line[x] != '\n' && !iword(line[x]))
			x++;
		if (x == ll || line[x] == '\n')
			{
			x = cs;
			while (x && line[x-1] != '\n' && !iword(line[x]))
				x--;
			if (!x || line[x-1] == '\n')
				{
				feep();
				return;
				}
			}
		for (p4 = x; p4 != ll && iword(line[p4]); p4++);
		for (p3 = p4; p3 && iword(line[p3-1]); p3--);
		if (!p3)
			{
			feep();
			return;
			}
		for (p2 = p3; p2 && !iword(line[p2-1]); p2--);
		if (!p2)
			{
			feep();
			return;
			}
		for (p1 = p2; p1 && iword(line[p1-1]); p1--);
		pp = temp = zalloc(p4-p1+1);
		struncpy(&pp,line+p3,p4-p3);
		struncpy(&pp,line+p2,p3-p2);
		struncpy(&pp,line+p1,p2-p1);
		strncpy(line+p1,temp,p4-p1);
		free(temp);
		cs = p4;
		}
}

static int kct,yankb,yanke;

void yank() /**/
{
int cc;

	if (!cutbuf)
		{
		feep();
		return;
		}
	while (mult--)
		{
		kct = kringnum;
		cc = strlen(cutbuf);
		yankb = cs;
		spaceinline(cc);
		strncpy(line+cs,cutbuf,cc);
		cs += cc;
		yanke = cs;
		}
}

void yankpop() /**/
{
int cc;

	if (!(lastcmd & ZLE_YANK) || !kring[kct])
		{
		feep();
		return;
		}
	cs = yankb;
	foredel(yanke-yankb);
	cc = strlen(kring[kct]);
	spaceinline(cc);
	strncpy(line+cs,kring[kct],cc);
	cs += cc;
	yanke = cs;
	kct = (kct-1) & (KRINGCT-1);
}

void overwritemode() /**/
{
	insmode ^= 1;
}

void undefinedkey() /**/
{
	feep();
}

void quotedinsert() /**/
{
	if (c = getkey(0))
		selfinsert();
	else
		feep();
}

void digitargument() /**/
{
	if (!(lastcmd & ZLE_ARG))
		mult = 0;
	mult = mult*10+(c&0xf);
}

void universalargument() /**/
{
	if (!(lastcmd & ZLE_ARG))
		mult = 4;
	else
		mult *= 4;
}

void toggleliteralhistory() /**/
{
char *s;

	if (histline == curhist)
		{
		if (curhistline)
			free(curhistline);
		curhistline = ztrdup(line);
		}
	lithist ^= 1;
	if (!(s = qgetevent(histline)))
		feep();
	else
		sethistline(s);
}

void uphistory() /**/
{
char *s;

	if (histline == curhist)
		{
		if (curhistline)
			free(curhistline);
		curhistline = ztrdup(line);
		}
	histline -= mult;
	if (!(s = qgetevent(histline)))
		{
		feep();
		histline += mult;
		}
	else
		sethistline(s);
}

void uplineorhistory() /**/
{
int ocs = cs;

	if ((lastcmd & ZLE_LINEMOVE2) != ZLE_LINEMOVE2)
		lastcol = cs-findbol();
	cs = findbol();
	while (mult)
		{
		if (!cs)
			break;
		cs--;
		cs = findbol();
		mult--;
		}
	if (mult)
		{
		cs = ocs;
		uphistory();
		}
	else
		{
		int x = findeol();
		if ((cs += lastcol) > x)
			cs = x;
		}
}

void downlineorhistory() /**/
{
int ocs = cs;

	if ((lastcmd & ZLE_LINEMOVE2) != ZLE_LINEMOVE2)
		lastcol = cs-findbol();
	while (mult)
		{
		int x = findeol();
		if (x == ll)
			break;
		cs = x+1;
		mult--;
		}
	if (mult)
		{
		cs = ocs;
		downhistory();
		}
	else
		{
		int x = findeol();
		if ((cs += lastcol) > x)
			cs = x;
		}
}

void acceptlineanddownhistory() /**/
{
char *s,*t;

	if (!(s = qgetevent(histline+1)))
		{
		feep();
		return;
		}
	pushnode(bufstack,t = ztrdup(s));
	for (; *t; t++)
		if (*t == HISTSPACE)
			*t = ' ';
	done = 1;
	stackhist = histline+1;
}

void downhistory() /**/
{
char *s;

	histline += mult;
	if (!(s = qgetevent(histline)))
		{
		feep();
		histline -= mult;
		return;
		}
	sethistline(s);
}

void historysearchbackward() /**/
{
int t0,ohistline = histline;
char *s;

	if (histline == curhist)
		{
		if (curhistline)
			free(curhistline);
		curhistline = ztrdup(line);
		}
	for (t0 = 0; line[t0] && iword(line[t0]); t0++);
	for (;;)
		{
		histline--;
		if (!(s = qgetevent(histline)))
			{
			feep();
			histline = ohistline;
			return;
			}
		if (!hstrncmp(s,line,t0))
			break;
		}
	sethistline(s);
}

void historysearchforward() /**/
{
int t0,ohistline = histline;
char *s;

	if (histline == curhist)
		{
		if (curhistline)
			free(curhistline);
		curhistline = ztrdup(line);
		}
	for (t0 = 0; line[t0] && iword(line[t0]); t0++);
	for (;;)
		{
		histline++;
		if (!(s = qgetevent(histline)))
			{
			feep();
			histline = ohistline;
			return;
			}
		if (!hstrncmp(s,line,t0))
			break;
		}
	sethistline(s);
}

void beginningofbufferorhistory() /**/
{
	if (findbol())
		cs = 0;
	else
		beginningofhistory();
}

void beginningofhistory() /**/
{
char *s;

	if (histline == curhist)
		{
		if (curhistline)
			free(curhistline);
		curhistline = ztrdup(line);
		}
	if (!(s = qgetevent(firsthist)))
		{
		feep();
		return;
		}
	histline = firsthist;
	sethistline(s);
}

void endofbufferorhistory() /**/
{
	if (findeol() != ll)
		cs = ll;
	else
		endofhistory();
}

void endofhistory() /**/
{
	if (histline == curhist)
		feep();
	else
		{
		histline = curhist;
		sethistline(curhistline);
		}
}

void insertlastword() /**/
{
char *s,*t;
int len,z = lithist;

	lithist = 0;
	if (!(s = qgetevent(curhist-1), lithist = z, s))
		{
		feep();
		return;
		}
	for (t = s+strlen(s); t > s; t--)
		if (*t == HISTSPACE)
			break;
	if (t != s)
		t++;
	spaceinline(len = strlen(t));
	strncpy(line+cs,t,len);
	cs += len;
}

void copyprevword() /**/
{
int len,t0;

	for (t0 = cs-1; t0 >= 0; t0--)
		if (iword(line[t0]))
			break;
	for (; t0 >= 0; t0--)
		if (!iword(line[t0]))
			break;
	if (t0)
		t0++;
	len = cs-t0;
	spaceinline(len);
	strncpy(line+cs,line+t0,len);
	cs += len;
}

char *qgetevent(ev) /**/
int ev;
{
	if (ev > curhist)
		return NULL;
	return ((ev == curhist) ? curhistline : quietgetevent(ev));
}

void pushline() /**/
{
	while (mult--)
		pushnode(bufstack,ztrdup(line));
	stackcs = cs;
	*line = '\0';
	ll = cs = 0;
}

void getline() /**/
{
char *s = getnode(bufstack);

	if (!s)
		feep();
	else
		{
		int cc;

		cc = strlen(s);
		spaceinline(cc);
		strncpy(line+cs,s,cc);
		cs += cc;
		free(s);
		}
}

void sendbreak() /**/
{
	errflag = done = 1;
}

void undo() /**/
{
char *s;
struct undoent *ue;

	ue = undos+undoct;
	if (!ue->change)
		{
		feep();
		return;
		}
	line[ll] = '\0';
	s = ztrdup(line+ll-ue->suff);
	sizeline((ll = ue->pref+ue->suff+ue->len)+1);
	strncpy(line+ue->pref,ue->change,ue->len);
	strcpy(line+ue->pref+ue->len,s);
	free(s);
	free(ue->change);
	ue->change = NULL;
	undoct = (undoct-1) & (UNDOCT-1);
	cs = ue->cs;
}

void historyincrementalsearchbackward() /**/
{
	doisearch(-1);
}

void historyincrementalsearchforward() /**/
{
	doisearch(1);
}

void doisearch(dir) /**/
int dir;
{
char *s,*oldl;
char ibuf[256],*sbuf = ibuf+10;
int sbptr = 0,ch,ohl = histline,ocs = cs;
int nomatch = 0;

	strcpy(ibuf,"i-search: ");
	statusline = ibuf;
	oldl = ztrdup(line);
	if (histline == curhist)
		{
		if (curhistline)
			free(curhistline);
		curhistline = ztrdup(line);
		}
	for (;;)
		{
		nomatch = 0;
		if (sbptr > 1 || (sbptr == 1 && sbuf[0] != '^'))
			{
			int ohistline = histline;

			for (;;)
				{
				char *t;

				if (!(s = qgetevent(histline)))
					{
					feep();
					nomatch = 1;
					histline = ohistline;
					break;
					}
				if ((sbuf[0] == '^') ?
						(t = (hstrncmp(s,sbuf+1,sbptr-1)) ? NULL : s) :
						(t = hstrnstr(s,sbuf,sbptr)))
					{
					sethistline(s);
					cs = t-s+sbptr;
					break;
					}
				histline += dir;
				}
			}
		refresh();
		if ((ch = getkey(0)) == -1)
			break;
		if (ch == 22 || ch == 17)
			{
			if ((ch = getkey(0)) == -1)
				break;
			}
		else if (ch == 8 || ch == 127)
			{
			if (sbptr)
				sbuf[--sbptr] = '\0';
			else
				feep();
			histline = ohl;
			continue;
			}
		else if (ch == 7 || ch == 3)
			{
			setline(oldl);
			cs = ocs;
			histline = ohl;
			statusline = NULL;
			break;
			}
		else if (ch == 27 || ch == 10 || ch == 13)
			break;
		else if (ch == 18)
			{
			ohl = (histline += (dir = -1));
			continue;
			}
		else if (ch == 19)
			{
			ohl = (histline += (dir = 1));
			continue;
			}
		if (!nomatch && sbptr != 39 && !icntrl(ch))
			{
			sbuf[sbptr++] = ch;
			sbuf[sbptr] = '\0';
			}
		}
	free(oldl);
	statusline = NULL;
}

void quoteregion() /**/
{
char *s,*t;
int x,y;

	if (mark > ll)
		mark = ll;
	if (mark < cs)
		{
		x = mark;
		mark = cs;
		cs = x;
		}
	s = zcalloc((y = mark-cs)+1);
	strncpy(s,line+cs,y);
	s[y] = '\0';
	foredel(mark-cs);
	t = makequote(s);
	spaceinline(x = strlen(t));
	strncpy(line+cs,t,x);
	free(t);
	free(s);
	mark = cs;
	cs += x;
}

void quoteline() /**/
{
char *s;

	line[ll] = '\0';
	s = makequote(line);
	setline(s);
	free(s);
}

char *makequote(s) /**/
char *s;
{
int qtct = 0;
char *l,*ol;

	for (l = s; *l; l++)
		if (*l == '\'')
			qtct++;
	l = ol = zalloc((qtct*3)+3+strlen(s));
	*l++ = '\'';
	for (; *s; s++)
		if (*s == '\'')
			{
			*l++ = '\'';
			*l++ = '\\';
			*l++ = '\'';
			*l++ = '\'';
			}
		else
			*l++ = *s;
	*l++ = '\'';
	*l = '\0';
	return ol;
}

void acceptandinfernexthistory() /**/
{
int t0;
char *s,*t;

	done = 1;
	for (t0 = histline-2;;t0--)
		{
		if (!(s = qgetevent(t0)))
			return;
		if (!hstrncmp(s,line,ll))
			break;
		}
	if (!(s = qgetevent(t0+1)))
		return;
	pushnode(bufstack,t = ztrdup(s));
	for (; *t; t++)
		if (*t == HISTSPACE)
			*t = ' ';
	stackhist = t0+1;
}

void infernexthistory() /**/
{
int t0;
char *s,*t;

	if (!(t = qgetevent(histline-1)))
		{
		feep();
		return;
		}
	for (t0 = histline-2;;t0--)
		{
		if (!(s = qgetevent(t0)))
			{
			feep();
			return;
			}
		if (!strcmp(s,t))
			break;
		}
	if (!(s = qgetevent(t0+1)))
		{
		feep();
		return;
		}
	sethistline(s);
}

