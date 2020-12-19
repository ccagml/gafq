/*
** $Id: ltm.h,v 2.6.1.1 2007/12/27 13:02:25 roberto Exp $
** Tag methods
** See Copyright Notice in gafq.h
*/

#ifndef ltm_h
#define ltm_h


#include "gobject.h"


/*
* WARNING: if you change the order of this enumeration,
* grep "ORDER TM"
*/
typedef enum {
  TM_INDEX,
  TM_NEWINDEX,
  TM_GC,
  TM_MODE,
  TM_EQ,  /* last tag method with `fast' access */
  TM_ADD,
  TM_SUB,
  TM_MUL,
  TM_DIV,
  TM_MOD,
  TM_POW,
  TM_UNM,
  TM_LEN,
  TM_LT,
  TM_LE,
  TM_CONCAT,
  TM_CALL,
  TM_N		/* number of elements in the enum */
} TMS;



#define gfasttm(g,et,e) ((et) == NULL ? NULL : \
  ((et)->flags & (1u<<(e))) ? NULL : gafqT_gettm(et, e, (g)->tmname[e]))

#define fasttm(l,et,e)	gfasttm(G(l), et, e)

GAFQI_DATA const char *const gafqT_typenames[];


GAFQI_FUNC const TValue *gafqT_gettm (Table *events, TMS event, TString *ename);
GAFQI_FUNC const TValue *gafqT_gettmbyobj (gafq_State *L, const TValue *o,
                                                       TMS event);
GAFQI_FUNC void gafqT_init (gafq_State *L);

#endif
