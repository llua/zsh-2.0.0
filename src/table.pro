Lklist newlist DCLPROTO((void));
Hashtab newhtable DCLPROTO((int size));
int hasher DCLPROTO((char *s));
void Addhnode DCLPROTO((char *nam,void *dat,Hashtab ht,FFunc freefunc,int canfree));
void expandhtab DCLPROTO((Hashtab ht));
void *gethnode DCLPROTO((char *nam,Hashtab ht));
void freehtab DCLPROTO((Hashtab ht,FFunc freefunc));
void *remhnode DCLPROTO((char *nam,Hashtab ht));
void insnode DCLPROTO((Lklist list,Lknode llast,void *dat));
void *remnode DCLPROTO((Lklist list,Lknode nd));
void *uremnode DCLPROTO((Lklist list,Lknode nd));
void chuck DCLPROTO((char *str));
void *getnode DCLPROTO((Lklist list));
void *ugetnode DCLPROTO((Lklist list));
void freetable DCLPROTO((Lklist tab,FFunc freefunc));
char *strstr DCLPROTO((char *s,char *t));
void inslist DCLPROTO((Lklist l,Lknode where,Lklist x));
int countnodes DCLPROTO((Lklist x));
