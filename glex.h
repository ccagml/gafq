/*
** $Id: glex.h,v 1.58.1.1 2007/12/27 13:02:25 roberto Exp $
** Lexical Analyzer
** See Copyright Notice in gafq.h
*/

#ifndef glex_h
#define glex_h

#include "gobject.h"
#include "gzio.h"


#define FIRST_RESERVED	257

/* maximum length of a reserved word */
#define TOKEN_LEN	(sizeof("function")/sizeof(char))


/*
* WARNING: if you change the order of this enumeration,
* grep "ORDER RESERVED"
*/
enum RESERVED {
  /* terminal symbols denoted by reserved words */
  TK_AND = FIRST_RESERVED, TK_BREAK,
  TK_DO, TK_ELSE, TK_ELSEIF, TK_END, TK_FALSE, TK_FOR, TK_FUNCTION,
  TK_IF, TK_IN, TK_LOCAL, TK_NIL, TK_NOT, TK_OR, TK_REPEAT,
  TK_RETURN, TK_THEN, TK_TRUE, TK_UNTIL, TK_WHILE,
  /* other terminal symbols */
  TK_CONCAT, TK_DOTS, TK_EQ, TK_GE, TK_LE, TK_NE, TK_NUMBER,
  TK_NAME, TK_STRING, TK_EOS
};

/* number of reserved words */
#define NUM_RESERVED	(cast(int, TK_WHILE-FIRST_RESERVED+1))


/* array with token `names' */
GAFQI_DATA const char *const gafqX_tokens [];


typedef union {
  gafq_Number r;
  TString *ts;
} SemInfo;  /* semantics information */


typedef struct Token {
  int token;
  SemInfo seminfo;
} Token;


typedef struct LexState {
  int current;  /* current character (charint) */
  int linenumber;  /* input line counter */
  int lastline;  /* line of last token `consumed' */
  Token t;  /* current token */
  Token lookahead;  /* look ahead token */
  struct FuncState *fs;  /* `FuncState' is private to the parser */
  struct gafq_State *L;
  ZIO *z;  /* input stream */
  Mbuffer *buff;  /* buffer for tokens */
  TString *source;  /* current source name */
  char decpoint;  /* locale decimal point */
} LexState;


GAFQI_FUNC void gafqX_init (gafq_State *L);
GAFQI_FUNC void gafqX_setinput (gafq_State *L, LexState *ls, ZIO *z,
                              TString *source);
GAFQI_FUNC TString *gafqX_newstring (LexState *ls, const char *str, size_t l);
GAFQI_FUNC void gafqX_next (LexState *ls);
GAFQI_FUNC void gafqX_lookahead (LexState *ls);
GAFQI_FUNC void gafqX_lexerror (LexState *ls, const char *msg, int token);
GAFQI_FUNC void gafqX_syntaxerror (LexState *ls, const char *s);
GAFQI_FUNC const char *gafqX_token2str (LexState *ls, int token);


#endif
