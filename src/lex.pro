void lexinit DCLPROTO((void));
void lexsave DCLPROTO((void));
void lexrestore DCLPROTO((void));
int yylex DCLPROTO((void));
void add DCLPROTO((int c));
int gettok DCLPROTO((void));
int exalias DCLPROTO((int *pk));
void skipcomm DCLPROTO((void));
