
typedef union  {
	Pline Pline;
	List List;
	Sublist Sublist;
	struct cmd *Comm;
	struct redir *Fnode;
	struct cond *Cond;
	struct forcmd *Fornode;
	struct casecmd *Casenode;
	struct ifcmd *Ifnode;
	struct whilecmd *Whilenode;
	struct repeatcmd *Repeatnode;
	struct varasg *Varnode;
	Lklist Table;
	struct fdpair fds;
	char *str;
	int value;
} YYSTYPE;
extern YYSTYPE yylval;
# define DOITNOW 257
# define EMPTY 258
# define LEXERR 259
# define SEPER 260
# define NEWLIN 261
# define SEMI 262
# define DSEMI 263
# define AMPER 264
# define INPAR 265
# define INBRACE 266
# define OUTPAR 267
# define DBAR 268
# define DAMPER 269
# define BANG 270
# define OUTBRACE 271
# define OUTANG 272
# define OUTANGBANG 273
# define DOUTANG 274
# define DOUTANGBANG 275
# define INANG 276
# define DINANG 277
# define INANGAMP 278
# define OUTANGAMP 279
# define OUTANGAMPBANG 280
# define DOUTANGAMP 281
# define DOUTANGAMPBANG 282
# define TRINANG 283
# define BAR 284
# define BARAMP 285
# define DINBRACK 286
# define DOUTBRACK 287
# define STRING 288
# define ENVSTRING 289
# define ENVARRAY 290
# define ENDINPUT 291
# define INOUTPAR 292
# define DO 293
# define DONE 294
# define ESAC 295
# define THEN 296
# define ELIF 297
# define ELSE 298
# define FI 299
# define FOR 300
# define CASE 301
# define IF 302
# define WHILE 303
# define FUNC 304
# define REPEAT 305
# define TIME 306
# define UNTIL 307
# define EXEC 308
# define COMMAND 309
# define SELECT 310
# define COPROC 311
# define NOGLOB 312
# define DASH 313
# define DOITLATER 314
