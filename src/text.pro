void taddchr DCLPROTO((int c));
void taddstr DCLPROTO((char *s));
void taddint DCLPROTO((int x));
void taddnl DCLPROTO((void));
char *gettext DCLPROTO((struct node *n,int nls));
void gettext2 DCLPROTO((struct node *n));
void getsimptext DCLPROTO((Cmd cmd));
void getredirs DCLPROTO((Cmd cmd));
void taddlist DCLPROTO((Lklist l));
