void sizeline DCLPROTO((int sz));
void spaceinline DCLPROTO((int ct));
void backkill DCLPROTO((int ct,int dir));
void forekill DCLPROTO((int ct,int dir));
void cut DCLPROTO((int i,int ct,int dir));
void backdel DCLPROTO((int ct));
void foredel DCLPROTO((int ct));
void setline DCLPROTO((char *s));
void sethistline DCLPROTO((char *s));
int findbol DCLPROTO((void));
int findeol DCLPROTO((void));
void findline DCLPROTO((int *a,int *b));
void initundo DCLPROTO((void));
void addundo DCLPROTO((void));
void freeundo DCLPROTO((void));
int hstrncmp DCLPROTO((char *s,char *t,int len));
char *hstrnstr DCLPROTO((char *s,char *t,int len));
