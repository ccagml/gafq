/*
** $Id: lgc.h,v 2.15.1.1 2007/12/27 13:02:25 roberto Exp $
** Garbage Collector
** See Copyright Notice in gafq.h
*/

#ifndef lgc_h
#define lgc_h


#include "gobject.h"


/*
** Possible states of the Garbage Collector
*/
#define GCSpause	0
#define GCSpropagate	1
#define GCSsweepstring	2
#define GCSsweep	3
#define GCSfinalize	4


/*
** some userful bit tricks
*/
#define resetbits(x,m)	((x) &= cast(lu_byte, ~(m)))
#define setbits(x,m)	((x) |= (m))
#define testbits(x,m)	((x) & (m))
#define bitmask(b)	(1<<(b))
#define bit2mask(b1,b2)	(bitmask(b1) | bitmask(b2))
#define l_setbit(x,b)	setbits(x, bitmask(b))
#define resetbit(x,b)	resetbits(x, bitmask(b))
#define testbit(x,b)	testbits(x, bitmask(b))
#define set2bits(x,b1,b2)	setbits(x, (bit2mask(b1, b2)))
#define reset2bits(x,b1,b2)	resetbits(x, (bit2mask(b1, b2)))
#define test2bits(x,b1,b2)	testbits(x, (bit2mask(b1, b2)))



/*
** Layout for bit use in `marked' field:
** bit 0 - object is white (type 0)
** bit 1 - object is white (type 1)
** bit 2 - object is black
** bit 3 - for userdata: has been finalized
** bit 3 - for tables: has weak keys
** bit 4 - for tables: has weak values
** bit 5 - object is fixed (should not be collected)
** bit 6 - object is "super" fixed (only the main thread)
*/


#define WHITE0BIT	0
#define WHITE1BIT	1
#define BLACKBIT	2
#define FINALIZEDBIT	3
#define KEYWEAKBIT	3
#define VALUEWEAKBIT	4
#define FIXEDBIT	5
#define SFIXEDBIT	6
#define WHITEBITS	bit2mask(WHITE0BIT, WHITE1BIT)


#define iswhite(x)      test2bits((x)->gch.marked, WHITE0BIT, WHITE1BIT)
#define isblack(x)      testbit((x)->gch.marked, BLACKBIT)
#define isgray(x)	(!isblack(x) && !iswhite(x))

#define otherwhite(g)	(g->currentwhite ^ WHITEBITS)
#define isdead(g,v)	((v)->gch.marked & otherwhite(g) & WHITEBITS)

#define changewhite(x)	((x)->gch.marked ^= WHITEBITS)
#define gray2black(x)	l_setbit((x)->gch.marked, BLACKBIT)

#define valiswhite(x)	(iscollectable(x) && iswhite(gcvalue(x)))

#define gafqC_white(g)	cast(lu_byte, (g)->currentwhite & WHITEBITS)


#define gafqC_checkGC(L) { \
  condhardstacktests(gafqD_reallocstack(L, L->stacksize - EXTRA_STACK - 1)); \
  if (G(L)->totalbytes >= G(L)->GCthreshold) \
	gafqC_step(L); }


#define gafqC_barrier(L,p,v) { if (valiswhite(v) && isblack(obj2gco(p)))  \
	gafqC_barrierf(L,obj2gco(p),gcvalue(v)); }

#define gafqC_barriert(L,t,v) { if (valiswhite(v) && isblack(obj2gco(t)))  \
	gafqC_barrierback(L,t); }

#define gafqC_objbarrier(L,p,o)  \
	{ if (iswhite(obj2gco(o)) && isblack(obj2gco(p))) \
		gafqC_barrierf(L,obj2gco(p),obj2gco(o)); }

#define gafqC_objbarriert(L,t,o)  \
   { if (iswhite(obj2gco(o)) && isblack(obj2gco(t))) gafqC_barrierback(L,t); }

GAFQI_FUNC size_t gafqC_separateudata (gafq_State *L, int all);
GAFQI_FUNC void gafqC_callGCTM (gafq_State *L);
GAFQI_FUNC void gafqC_freeall (gafq_State *L);
GAFQI_FUNC void gafqC_step (gafq_State *L);
GAFQI_FUNC void gafqC_fullgc (gafq_State *L);
GAFQI_FUNC void gafqC_link (gafq_State *L, GCObject *o, lu_byte tt);
GAFQI_FUNC void gafqC_linkupval (gafq_State *L, UpVal *uv);
GAFQI_FUNC void gafqC_barrierf (gafq_State *L, GCObject *o, GCObject *v);
GAFQI_FUNC void gafqC_barrierback (gafq_State *L, Table *t);


#endif
