/*
** $Id: lvm.h,v 2.5.1.1 2007/12/27 13:02:25 roberto Exp $
** Gafq virtual machine
** See Copyright Notice in gafq.h
*/

#ifndef lvm_h
#define lvm_h


#include "gdo.h"
#include "lobject.h"
#include "ltm.h"


#define tostring(L,o) ((ttype(o) == GAFQ_TSTRING) || (gafqV_tostring(L, o)))

#define tonumber(o,n)	(ttype(o) == GAFQ_TNUMBER || \
                         (((o) = gafqV_tonumber(o,n)) != NULL))

#define equalobj(L,o1,o2) \
	(ttype(o1) == ttype(o2) && gafqV_equalval(L, o1, o2))


GAFQI_FUNC int gafqV_lessthan (gafq_State *L, const TValue *l, const TValue *r);
GAFQI_FUNC int gafqV_equalval (gafq_State *L, const TValue *t1, const TValue *t2);
GAFQI_FUNC const TValue *gafqV_tonumber (const TValue *obj, TValue *n);
GAFQI_FUNC int gafqV_tostring (gafq_State *L, StkId obj);
GAFQI_FUNC void gafqV_gettable (gafq_State *L, const TValue *t, TValue *key,
                                            StkId val);
GAFQI_FUNC void gafqV_settable (gafq_State *L, const TValue *t, TValue *key,
                                            StkId val);
GAFQI_FUNC void gafqV_execute (gafq_State *L, int nexeccalls);
GAFQI_FUNC void gafqV_concat (gafq_State *L, int total, int last);

#endif
