/*

	zle.h - header file for line editor

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

#ifdef ZLEGLOBALS
#define ZLEXTERN
#else
#define ZLEXTERN extern
#endif

#ifdef ZLE

/* cursor position */
ZLEXTERN int cs;

/* line length */
ZLEXTERN int ll;

/* size of line buffer */
ZLEXTERN int linesz;

/* location of mark */
ZLEXTERN int mark;

/* last character pressed */
ZLEXTERN int c;

/* forgot what this does */
ZLEXTERN int bindk;

/* command argument */
ZLEXTERN int mult;

/* insert mode flag */
ZLEXTERN int insmode;

/* cost of last update */
ZLEXTERN int cost;

/* flags associated with last command */
ZLEXTERN int lastcmd;

/* column position before last movement */
ZLEXTERN int lastcol;

#endif

/* != 0 if we're done */
ZLEXTERN int done;

/* length of prompt on screen */
ZLEXTERN int pptlen;

/* current history line number */
ZLEXTERN int histline;

ZLEXTERN int eofsent;

/* != 0 if we need to call resetvideo() */
ZLEXTERN int resetneeded;

/* != 0 if the line editor is active */
ZLEXTERN int zleactive;

/* the line buffer */
ZLEXTERN char *line;

/* the cut buffer */
ZLEXTERN char *cutbuf;

/* prompt and rprompt */
ZLEXTERN char *pmpt, *pmpt2;

/* the last line in the history (the current one) */
ZLEXTERN char *curhistline;

/* the status line */
ZLEXTERN char *statusline;

/*
	the current history line and cursor position for the top line
	on the buffer stack
*/

ZLEXTERN int stackhist,stackcs;

/* != 0 if we are in the middle of a menu completion */
ZLEXTERN int menucmp;

typedef void bindfunc DCLPROTO((void));
typedef bindfunc *F;

struct key {
	int func;			/* function code for this key */
	char *str;			/* string corresponding to this key,
								if func = z_sequenceleadin				 */
	int len;				/* length of string */
	};
struct zlecmd {
	char *name;			/* name of function */
	F func;				/* handler function */
	int flags;
	};

/* undo event */

struct undoent {
	int pref;		/* number of initial chars unchanged */
	int suff;		/* number of trailing chars unchanged */
	int len;			/* length of changed chars */
	int cs;			/* cursor pos before change */
	char *change;	/* NOT null terminated */
	};

#define UNDOCT 64

struct undoent undos[UNDOCT];

/* the line before last mod */
ZLEXTERN char *lastline;

ZLEXTERN int undoct,lastcs;

ZLEXTERN char *visrchstr;
ZLEXTERN int visrchsense;

#define ZLE_ABORT		1					/* abort cmd */
#define ZLE_MOVE		2					/* movement */
#define ZLE_MOD		4					/* text modification */
#define ZLE_LINEMOVE (8|ZLE_MOD)		/* movement or modification */
#define ZLE_INSMOD	16					/* character insertion */
#define ZLE_UNDO		32					/* undo */
#define ZLE_LINEMOVE2 (64|ZLE_LINEMOVE)	/* movement or mod */
#define ZLE_ARG      128				/* argument */
#define ZLE_KILL     256				/* killing text */
#define ZLE_MENUCMP	512				/* menu completion */
#define ZLE_YANK		1024

typedef struct key *Key;

ZLEXTERN int *bindtab;
extern int emacsbind[256];
ZLEXTERN int altbindtab[256],mainbindtab[256];
extern int viinsbind[],vicmdbind[];

#define KRINGCT 8
ZLEXTERN char *kring[KRINGCT];
ZLEXTERN int kringnum;

enum xbindings {
z_acceptandhold,
z_acceptandinfernexthistory,
z_acceptline,
z_acceptlineanddownhistory,
z_backwardchar,
z_backwarddeletechar,
z_backwarddeleteword,
z_backwardkillline,
z_backwardkillword,
z_backwardword,
z_beginningofbufferorhistory,
z_beginningofhistory,
z_beginningofline,
z_capitalizeword,
z_clearscreen,
z_completeword,
z_copyprevword,
z_copyregionaskill,
z_deletechar,
z_deletecharorlist,
z_deleteword,
z_digitargument,
z_downcaseword,
z_downhistory,
z_downlineorhistory,
z_endofbufferorhistory,
z_endofhistory,
z_endofline,
z_exchangepointandmark,
z_expandhistory,
z_expandorcomplete,
z_expandword,
z_forwardchar,
z_forwardword,
z_getline,
z_gosmacstransposechars,
z_historyincrementalsearchbackward,
z_historyincrementalsearchforward,
z_historysearchbackward,
z_historysearchforward,
z_infernexthistory,
z_insertlastword,
z_killbuffer,
z_killline,
z_killregion,
z_killwholeline,
z_listchoices,
z_listexpand,
z_magicspace,
z_metafynext,
z_overwritemode,
z_pushline,
z_quotedinsert,
z_quoteline,
z_quoteregion,
z_redisplay,
z_reversemenucomplete,
z_runhelp,
z_selfinsert,
z_selfinsertunmeta,
z_sendbreak,
z_sendstring,
z_sequenceleadin,
z_setmarkcommand,
z_spellword,
z_toggleliteralhistory,
z_transposechars,
z_transposewords,
z_undefinedkey,
z_undo,
z_universalargument,
z_upcaseword,
z_uphistory,
z_uplineorhistory,
z_viaddeol,
z_viaddnext,
z_vicapslockpanic,
z_vichange,
z_vichangeeol,
z_vichangewholeline,
z_vicmdmode,
z_videlete,
z_vidigitorbeginningofline,
z_vifetchhistory,
z_vifindnextchar,
z_vifindnextcharskip,
z_vifindprevchar,
z_vifindprevcharskip,
z_vifirstnonblank,
z_viforwardwordend,
z_vigotocolumn,
z_vihistorysearchbackward,
z_vihistorysearchforward,
z_viinsert,
z_viinsertbol,
z_vijoin,
z_vimatchbracket,
z_viopenlineabove,
z_viopenlinebelow,
z_vioperswapcases,
z_viputafter,
z_virepeatfind,
z_virepeatsearch,
z_vireplace,
z_vireplacechars,
z_virevrepeatfind,
z_virevrepeatsearch,
z_viswapcase,
z_viundo,
z_viyank,
z_viyankeol,
z_whichcommand,
z_yank,
z_yankpop,
ZLECMDCOUNT
};

extern struct zlecmd zlecmds[];

