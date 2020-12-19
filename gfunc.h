/*
** $Id: gfunc.h,v 2.4.1.1 2007/12/27 13:02:25 roberto Exp $
** Auxiliary functions to manipulate prototypes and closures
** See Copyright Notice in gafq.h
*/

#ifndef gfunc_h
#define gfunc_h


#include "gobject.h"


#define sizeCclosure(n)	(cast(int, sizeof(CClosure)) + \
                         cast(int, sizeof(TValue)*((n)-1)))

#define sizeLclosure(n)	(cast(int, sizeof(LClosure)) + \
                         cast(int, sizeof(TValue *)*((n)-1)))


GAFQI_FUNC Proto *gafqF_newproto (gafq_State *L);
GAFQI_FUNC Closure *gafqF_newCclosure (gafq_State *L, int nelems, Table *e);
GAFQI_FUNC Closure *gafqF_newLclosure (gafq_State *L, int nelems, Table *e);
GAFQI_FUNC UpVal *gafqF_newupval (gafq_State *L);
GAFQI_FUNC UpVal *gafqF_findupval (gafq_State *L, StkId level);
GAFQI_FUNC void gafqF_close (gafq_State *L, StkId level);
GAFQI_FUNC void gafqF_freeproto (gafq_State *L, Proto *f);
GAFQI_FUNC void gafqF_freeclosure (gafq_State *L, Closure *c);
GAFQI_FUNC void gafqF_freeupval (gafq_State *L, UpVal *uv);
GAFQI_FUNC const char *gafqF_getlocalname (const Proto *func, int local_number,
                                         int pc);


#endif
