int getfdstr DCLPROTO((char *s));
struct redir *parredir DCLPROTO((struct fdpair fdp,char *toks));
struct list *parev DCLPROTO((void));
struct list *parlist DCLPROTO((void));
struct list *makelnode DCLPROTO((struct sublist *x,int type));
struct sublist *makel2node DCLPROTO((struct pline *p,int type));
struct pline *makepnode DCLPROTO((struct cmd *c,struct pline *p,int type));
struct cmd *makefornode DCLPROTO((char *str,Lklist t,struct list *l,int type));
struct ifcmd *makeifnode DCLPROTO((struct list *l1,struct list *l2, struct ifcmd *i2));
struct cmd *makewhilenode DCLPROTO((struct list *l1,struct list *l2, int sense));
struct cmd *makecnode DCLPROTO((int type));
struct cmd *makefuncdef DCLPROTO((Lklist x,struct list *l));
struct cond *makecond DCLPROTO((void));
struct varasg *makevarnode DCLPROTO((int type));
struct casecmd *makecasenode DCLPROTO((char *str,struct list *l,struct casecmd *c));
void parcond2 DCLPROTO((char *a,char *b,struct cond *n));
void parcond3 DCLPROTO((char *a,char *b,char *c,struct cond *n));