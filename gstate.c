/*
** $Id: gstate.c,v 2.36.1.2 2008/01/03 15:20:39 roberto Exp $
** Global State
** See Copyright Notice in gafq.h
*/


#include <stddef.h>

#define gstate_c
#define GAFQ_CORE

#include "gafq.h"

#include "gdebug.h"
#include "gdo.h"
#include "gfunc.h"
#include "ggc.h"
#include "glex.h"
#include "gmem.h"
#include "gstate.h"
#include "gstring.h"
#include "gtable.h"
#include "gtm.h"


#define state_size(x)	(sizeof(x) + GAFQI_EXTRASPACE)
#define fromstate(l)	(cast(lu_byte *, (l)) - GAFQI_EXTRASPACE)
#define tostate(l)   (cast(gafq_State *, cast(lu_byte *, l) + GAFQI_EXTRASPACE))


/*
** Main thread combines a thread state and the global state
*/
typedef struct LG {
  gafq_State l;
  global_State g;
} LG;
  


static void stack_init (gafq_State *L1, gafq_State *L) {
  /* initialize CallInfo array */
  L1->base_ci = gafqM_newvector(L, BASIC_CI_SIZE, CallInfo);
  L1->ci = L1->base_ci;
  L1->size_ci = BASIC_CI_SIZE;
  L1->end_ci = L1->base_ci + L1->size_ci - 1;
  /* initialize stack array */
  L1->stack = gafqM_newvector(L, BASIC_STACK_SIZE + EXTRA_STACK, TValue);
  L1->stacksize = BASIC_STACK_SIZE + EXTRA_STACK;
  L1->top = L1->stack;
  L1->stack_last = L1->stack+(L1->stacksize - EXTRA_STACK)-1;
  /* initialize first ci */
  L1->ci->func = L1->top;
  setnilvalue(L1->top++);  /* `function' entry for this `ci' */
  L1->base = L1->ci->base = L1->top;
  L1->ci->top = L1->top + GAFQ_MINSTACK;
}


static void freestack (gafq_State *L, gafq_State *L1) {
  gafqM_freearray(L, L1->base_ci, L1->size_ci, CallInfo);
  gafqM_freearray(L, L1->stack, L1->stacksize, TValue);
}


/*
** open parts that may cause memory-allocation errors
*/
static void f_gafqopen (gafq_State *L, void *ud) {
  global_State *g = G(L);
  UNUSED(ud);
  stack_init(L, L);  /* init stack */
  sethvalue(L, gt(L), gafqH_new(L, 0, 2));  /* table of globals */
  sethvalue(L, registry(L), gafqH_new(L, 0, 2));  /* registry */
  gafqS_resize(L, MINSTRTABSIZE);  /* initial size of string table */
  gafqT_init(L);
  gafqX_init(L);
  gafqS_fix(gafqS_newliteral(L, MEMERRMSG));
  g->GCthreshold = 4*g->totalbytes;
}


static void preinit_state (gafq_State *L, global_State *g) {
  G(L) = g;
  L->stack = NULL;
  L->stacksize = 0;
  L->errorJmp = NULL;
  L->hook = NULL;
  L->hookmask = 0;
  L->basehookcount = 0;
  L->allowhook = 1;
  resethookcount(L);
  L->openupval = NULL;
  L->size_ci = 0;
  L->nCcalls = L->baseCcalls = 0;
  L->status = 0;
  L->base_ci = L->ci = NULL;
  L->savedpc = NULL;
  L->errfunc = 0;
  setnilvalue(gt(L));
}


static void close_state (gafq_State *L) {
  global_State *g = G(L);
  gafqF_close(L, L->stack);  /* close all upvalues for this thread */
  gafqC_freeall(L);  /* collect all objects */
  gafq_assert(g->rootgc == obj2gco(L));
  gafq_assert(g->strt.nuse == 0);
  gafqM_freearray(L, G(L)->strt.hash, G(L)->strt.size, TString *);
  gafqZ_freebuffer(L, &g->buff);
  freestack(L, L);
  gafq_assert(g->totalbytes == sizeof(LG));
  (*g->frealloc)(g->ud, fromstate(L), state_size(LG), 0);
}


gafq_State *gafqE_newthread (gafq_State *L) {
  gafq_State *L1 = tostate(gafqM_malloc(L, state_size(gafq_State)));
  gafqC_link(L, obj2gco(L1), GAFQ_TTHREAD);
  preinit_state(L1, G(L));
  stack_init(L1, L);  /* init stack */
  setobj2n(L, gt(L1), gt(L));  /* share table of globals */
  L1->hookmask = L->hookmask;
  L1->basehookcount = L->basehookcount;
  L1->hook = L->hook;
  resethookcount(L1);
  gafq_assert(iswhite(obj2gco(L1)));
  return L1;
}


void gafqE_freethread (gafq_State *L, gafq_State *L1) {
  gafqF_close(L1, L1->stack);  /* close all upvalues for this thread */
  gafq_assert(L1->openupval == NULL);
  gafqi_userstatefree(L1);
  freestack(L, L1);
  gafqM_freemem(L, fromstate(L1), state_size(gafq_State));
}

// 创建新状态，f是一个申请内存的方法？
GAFQ_API gafq_State *gafq_newstate (gafq_Alloc f, void *ud) {
  int i;
  gafq_State *L;  // 单个线程状态
  global_State *g; // 全局线程状态
  void *l = (*f)(ud, NULL, 0, state_size(LG));
  if (l == NULL) return NULL;
  L = tostate(l);
  g = &((LG *)L)->g;
  L->next = NULL;
  L->tt = GAFQ_TTHREAD;
  g->currentwhite = bit2mask(WHITE0BIT, FIXEDBIT);
  L->marked = gafqC_white(g);
  set2bits(L->marked, FIXEDBIT, SFIXEDBIT);
  preinit_state(L, g);
  g->frealloc = f;
  g->ud = ud;
  g->mainthread = L;
  g->uvhead.u.l.prev = &g->uvhead;
  g->uvhead.u.l.next = &g->uvhead;
  g->GCthreshold = 0;  /* mark it as unfinished state */
  g->strt.size = 0;
  g->strt.nuse = 0;
  g->strt.hash = NULL;
  setnilvalue(registry(L));
  gafqZ_initbuffer(L, &g->buff);
  g->panic = NULL;
  g->gcstate = GCSpause;
  g->rootgc = obj2gco(L);
  g->sweepstrgc = 0;
  g->sweepgc = &g->rootgc;
  g->gray = NULL;
  g->grayagain = NULL;
  g->weak = NULL;
  g->tmudata = NULL;
  g->totalbytes = sizeof(LG);
  g->gcpause = GAFQI_GCPAUSE;
  g->gcstepmul = GAFQI_GCMUL;
  g->gcdept = 0;
  for (i=0; i<NUM_TAGS; i++) g->mt[i] = NULL;
  if (gafqD_rawrunprotected(L, f_gafqopen, NULL) != 0) {
    /* memory allocation error: free partial state */
    close_state(L);
    L = NULL;
  }
  else
    gafqi_userstateopen(L);
  return L;
}


static void callalggcTM (gafq_State *L, void *ud) {
  UNUSED(ud);
  gafqC_callGCTM(L);  /* call GC metamethods for all udata */
}


GAFQ_API void gafq_close (gafq_State *L) {
  L = G(L)->mainthread;  /* only the main thread can be closed */
  gafq_lock(L);
  gafqF_close(L, L->stack);  /* close all upvalues for this thread */
  gafqC_separateudata(L, 1);  /* separate udata that have GC metamethods */
  L->errfunc = 0;  /* no error function during GC metamethods */
  do {  /* repeat until no more errors */
    L->ci = L->base_ci;
    L->base = L->top = L->ci->base;
    L->nCcalls = L->baseCcalls = 0;
  } while (gafqD_rawrunprotected(L, callalggcTM, NULL) != 0);
  gafq_assert(G(L)->tmudata == NULL);
  gafqi_userstateclose(L);
  close_state(L);
}

