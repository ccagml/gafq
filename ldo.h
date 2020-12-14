/*
** $Id: ldo.h,v 2.7.1.1 2007/12/27 13:02:25 roberto Exp $
** Stack and Call structure of Gafq
** See Copyright Notice in gafq.h
*/

#ifndef ldo_h
#define ldo_h


#include "lobject.h"
#include "lstate.h"
#include "lzio.h"


#define gafqD_checkstack(L,n)	\
  if ((char *)L->stack_last - (char *)L->top <= (n)*(int)sizeof(TValue)) \
    gafqD_growstack(L, n); \
  else condhardstacktests(gafqD_reallocstack(L, L->stacksize - EXTRA_STACK - 1));


#define incr_top(L) {gafqD_checkstack(L,1); L->top++;}

#define savestack(L,p)		((char *)(p) - (char *)L->stack)
#define restorestack(L,n)	((TValue *)((char *)L->stack + (n)))

#define saveci(L,p)		((char *)(p) - (char *)L->base_ci)
#define restoreci(L,n)		((CallInfo *)((char *)L->base_ci + (n)))


/* results from gafqD_precall */
#define PCRGAFQ		0	/* initiated a call to a Gafq function */
#define PCRC		1	/* did a call to a C function */
#define PCRYIELD	2	/* C funtion yielded */


/* type of protected functions, to be ran by `runprotected' */
typedef void (*Pfunc) (gafq_State *L, void *ud);

GAFQI_FUNC int gafqD_protectedparser (gafq_State *L, ZIO *z, const char *name);
GAFQI_FUNC void gafqD_callhook (gafq_State *L, int event, int line);
GAFQI_FUNC int gafqD_precall (gafq_State *L, StkId func, int nresults);
GAFQI_FUNC void gafqD_call (gafq_State *L, StkId func, int nResults);
GAFQI_FUNC int gafqD_pcall (gafq_State *L, Pfunc func, void *u,
                                        ptrdiff_t oldtop, ptrdiff_t ef);
GAFQI_FUNC int gafqD_poscall (gafq_State *L, StkId firstResult);
GAFQI_FUNC void gafqD_reallocCI (gafq_State *L, int newsize);
GAFQI_FUNC void gafqD_reallocstack (gafq_State *L, int newsize);
GAFQI_FUNC void gafqD_growstack (gafq_State *L, int n);

GAFQI_FUNC void gafqD_throw (gafq_State *L, int errcode);
GAFQI_FUNC int gafqD_rawrunprotected (gafq_State *L, Pfunc f, void *ud);

GAFQI_FUNC void gafqD_seterrorobj (gafq_State *L, int errcode, StkId oldtop);

#endif

