/*
** $Id: ldebug.h,v 2.3.1.1 2007/12/27 13:02:25 roberto Exp $
** Auxiliary functions from Debug Interface module
** See Copyright Notice in gafq.h
*/

#ifndef ldebug_h
#define ldebug_h


#include "lstate.h"


#define pcRel(pc, p)	(cast(int, (pc) - (p)->code) - 1)

#define getline(f,pc)	(((f)->lineinfo) ? (f)->lineinfo[pc] : 0)

#define resethookcount(L)	(L->hookcount = L->basehookcount)


GAFQI_FUNC void gafqG_typeerror (gafq_State *L, const TValue *o,
                                             const char *opname);
GAFQI_FUNC void gafqG_concaterror (gafq_State *L, StkId p1, StkId p2);
GAFQI_FUNC void gafqG_aritherror (gafq_State *L, const TValue *p1,
                                              const TValue *p2);
GAFQI_FUNC int gafqG_ordererror (gafq_State *L, const TValue *p1,
                                             const TValue *p2);
GAFQI_FUNC void gafqG_runerror (gafq_State *L, const char *fmt, ...);
GAFQI_FUNC void gafqG_errormsg (gafq_State *L);
GAFQI_FUNC int gafqG_checkcode (const Proto *pt);
GAFQI_FUNC int gafqG_checkopenop (Instruction i);

#endif
