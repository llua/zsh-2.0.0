#include "glob.pro"
#include "hist.pro"
#include "table.pro"
#include "subst.pro"
#include "params.pro"
#include "builtin.pro"
#include "loop.pro"
#include "jobs.pro"
#include "exec.pro"
#include "init.pro"
#include "lex.pro"
#include "parse.pro"
#include "utils.pro"
#include "cond.pro"
#include "mem.pro"
#include "text.pro"
#include "watch.pro"

#include "zle_emacs.pro"
#include "zle_utils.pro"
#include "zle_main.pro"
#include "zle_refresh.pro"
#include "zle_tricky.pro"
#include "zle_vi.pro"

char *mktemp DCLPROTO((char *));
char *malloc DCLPROTO((int));
char *realloc DCLPROTO((char *,int));
char *calloc DCLPROTO((int,int));
char *ttyname DCLPROTO((int));
char *tgetstr();
