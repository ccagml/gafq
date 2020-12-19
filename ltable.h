/*
** $Id: ltable.h,v 2.10.1.1 2007/12/27 13:02:25 roberto Exp $
** Gafq tables (hash)
** See Copyright Notice in gafq.h
*/

#ifndef ltable_h
#define ltable_h

#include "gobject.h"


#define gnode(t,i)	(&(t)->node[i])
#define gkey(n)		(&(n)->i_key.nk)
#define gval(n)		(&(n)->i_val)
#define gnext(n)	((n)->i_key.nk.next)

#define key2tval(n)	(&(n)->i_key.tvk)


GAFQI_FUNC const TValue *gafqH_getnum (Table *t, int key);
GAFQI_FUNC TValue *gafqH_setnum (gafq_State *L, Table *t, int key);
GAFQI_FUNC const TValue *gafqH_getstr (Table *t, TString *key);
GAFQI_FUNC TValue *gafqH_setstr (gafq_State *L, Table *t, TString *key);
GAFQI_FUNC const TValue *gafqH_get (Table *t, const TValue *key);
GAFQI_FUNC TValue *gafqH_set (gafq_State *L, Table *t, const TValue *key);
GAFQI_FUNC Table *gafqH_new (gafq_State *L, int narray, int lnhash);
GAFQI_FUNC void gafqH_resizearray (gafq_State *L, Table *t, int nasize);
GAFQI_FUNC void gafqH_free (gafq_State *L, Table *t);
GAFQI_FUNC int gafqH_next (gafq_State *L, Table *t, StkId key);
GAFQI_FUNC int gafqH_getn (Table *t);


#if defined(GAFQ_DEBUG)
GAFQI_FUNC Node *gafqH_mainposition (const Table *t, const TValue *key);
GAFQI_FUNC int gafqH_isdummy (Node *n);
#endif


#endif
