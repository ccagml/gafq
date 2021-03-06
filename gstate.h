/*
** $Id: gstate.h,v 2.24.1.2 2008/01/03 15:20:39 roberto Exp $
** Global State
** See Copyright Notice in gafq.h
*/

#ifndef gstate_h
#define gstate_h

#include "gafq.h"

#include "gobject.h"
#include "gtm.h"
#include "gzio.h"



struct gafq_longjmp;  /* defined in gdo.c */


/* table of globals */
#define gt(L)	(&L->l_gt)

/* registry */
#define registry(L)	(&G(L)->l_registry)


/* extra stack space to handle TM calls and some other extras */
#define EXTRA_STACK   5


#define BASIC_CI_SIZE           8

#define BASIC_STACK_SIZE        (2*GAFQ_MINSTACK)



typedef struct stringtable {
  GCObject **hash;
  lu_int32 nuse;  /* number of elements */
  int size;
} stringtable;


/*
** informations about a call
*/
typedef struct CallInfo {
  StkId base;  /* base for this function */
  StkId func;  /* function index in the stack */
  StkId	top;  /* top for this function */
  const Instruction *savedpc;
  int nresults;  /* expected number of results from this function */
  int tailcalls;  /* number of tail calls lost under this entry */
} CallInfo;



#define curr_func(L)	(clvalue(L->ci->func))
#define ci_func(ci)	(clvalue((ci)->func))
#define f_isGafq(ci)	(!ci_func(ci)->c.isC)
#define isGafq(ci)	(ttisfunction((ci)->func) && f_isGafq(ci))


/*
** `global state', shared by all threads of this state
** 全局状态
*/
typedef struct global_State {
  stringtable strt;  /* hash table for strings */
  gafq_Alloc frealloc;  /* function to reallocate memory */
  void *ud;         /* auxiliary data to `frealloc' */
  lu_byte currentwhite;
  lu_byte gcstate;  /* state of garbage collector */
  int sweepstrgc;  /* position of sweep in `strt' */
  GCObject *rootgc;  /* list of all collectable objects */
  GCObject **sweepgc;  /* position of sweep in `rootgc' */
  GCObject *gray;  /* list of gray objects */
  GCObject *grayagain;  /* list of objects to be traversed atomically */
  GCObject *weak;  /* list of weak tables (to be cleared) */
  GCObject *tmudata;  /* last element of list of userdata to be GC */
  Mbuffer buff;  /* temporary buffer for string concatentation */
  lu_mem GCthreshold;
  lu_mem totalbytes;  /* number of bytes currently allocated */
  lu_mem estimate;  /* an estimate of number of bytes actually in use */
  lu_mem gcdept;  /* how much GC is `behind schedule' */
  int gcpause;  /* size of pause between successive GCs */
  int gcstepmul;  /* GC `granularity' */
  gafq_CFunction panic;  /* to be called in unprotected errors */
  TValue l_registry;
  struct gafq_State *mainthread;
  UpVal uvhead;  /* head of double-linked list of all open upvalues */
  struct Table *mt[NUM_TAGS];  /* metatables for basic types */
  TString *tmname[TM_N];  /* array with tag-method names */
} global_State;


/*
** `per thread' state
** 主线程创建的一个 状态?
*/
struct gafq_State {
  // 头,好像跟gc有关的对象都有这个东西
  CommonHeader;
  lu_byte status; // char类型
  // 栈顶
  StkId top;  /* first free slot in the stack */
  // 当前位置
  StkId base;  /* base of current function */
  //全局状态
  global_State *l_G;
  CallInfo *ci;  /* call info for current function */
  const Instruction *savedpc;  /* `savedpc' of current function */
  StkId stack_last;  /* last free slot in the stack */
  StkId stack;  /* stack base */
  CallInfo *end_ci;  /* points after end of ci array*/
  CallInfo *base_ci;  /* array of CallInfo's */
  int stacksize;
  int size_ci;  /* size of array `base_ci' */
  unsigned short nCcalls;  /* number of nested C calls */
  unsigned short baseCcalls;  /* nested C calls when resuming coroutine */
  lu_byte hookmask;
  lu_byte allowhook;
  int basehookcount;
  int hookcount;
  gafq_Hook hook;  // 跟debug有关
  TValue l_gt;  /* table of globals */
  TValue env;  /* temporary place for environments */
  GCObject *openupval;  /* list of open upvalues in this stack */
  GCObject *gclist;
  struct gafq_longjmp *errorJmp;  /* current error recover point */
  ptrdiff_t errfunc;  /* current error handling function (stack index) */
};


#define G(L)	(L->l_G)


/*
** Union of all collectable objects
*/
union GCObject {
  GCheader gch;
  union TString ts;
  union Udata u;
  union Closure cl;
  struct Table h;
  struct Proto p;
  struct UpVal uv;
  struct gafq_State th;  /* thread */
};


/* macros to convert a GCObject into a specific value */
#define rawgco2ts(o)	check_exp((o)->gch.tt == GAFQ_TSTRING, &((o)->ts))
#define gco2ts(o)	(&rawgco2ts(o)->tsv)
#define rawgco2u(o)	check_exp((o)->gch.tt == GAFQ_TUSERDATA, &((o)->u))
#define gco2u(o)	(&rawgco2u(o)->uv)
#define gco2cl(o)	check_exp((o)->gch.tt == GAFQ_TFUNCTION, &((o)->cl))
#define gco2h(o)	check_exp((o)->gch.tt == GAFQ_TTABLE, &((o)->h))
#define gco2p(o)	check_exp((o)->gch.tt == GAFQ_TPROTO, &((o)->p))
#define gco2uv(o)	check_exp((o)->gch.tt == GAFQ_TUPVAL, &((o)->uv))
#define ngcotouv(o) \
	check_exp((o) == NULL || (o)->gch.tt == GAFQ_TUPVAL, &((o)->uv))
#define gco2th(o)	check_exp((o)->gch.tt == GAFQ_TTHREAD, &((o)->th))

/* macro to convert any Gafq object into a GCObject */
#define obj2gco(v)	(cast(GCObject *, (v)))


GAFQI_FUNC gafq_State *gafqE_newthread (gafq_State *L);
GAFQI_FUNC void gafqE_freethread (gafq_State *L, gafq_State *L1);

#endif

