/*
** $Id: lstring.h,v 1.43.1.1 2007/12/27 13:02:25 roberto Exp $
** String table (keep all strings handled by Gafq)
** See Copyright Notice in gafq.h
*/

#ifndef lstring_h
#define lstring_h


#include "lgc.h"
#include "lobject.h"
#include "lstate.h"


#define sizestring(s)	(sizeof(union TString)+((s)->len+1)*sizeof(char))

#define sizeudata(u)	(sizeof(union Udata)+(u)->len)

#define gafqS_new(L, s)	(gafqS_newlstr(L, s, strlen(s)))
#define gafqS_newliteral(L, s)	(gafqS_newlstr(L, "" s, \
                                 (sizeof(s)/sizeof(char))-1))

#define gafqS_fix(s)	l_setbit((s)->tsv.marked, FIXEDBIT)

GAFQI_FUNC void gafqS_resize (gafq_State *L, int newsize);
GAFQI_FUNC Udata *gafqS_newudata (gafq_State *L, size_t s, Table *e);
GAFQI_FUNC TString *gafqS_newlstr (gafq_State *L, const char *str, size_t l);


#endif
