/*
** $Id: gdo.c,v 2.38.1.4 2012/01/18 02:27:10 roberto Exp $
** Stack and Call structure of Gafq
** See Copyright Notice in gafq.h
*/


#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#define gdo_c
#define GAFQ_CORE

#include "gafq.h"

#include "gdebug.h"
#include "gdo.h"
#include "gfunc.h"
#include "ggc.h"
#include "gmem.h"
#include "gobject.h"
#include "gopcodes.h"
#include "gparser.h"
#include "gstate.h"
#include "gstring.h"
#include "gtable.h"
#include "gtm.h"
#include "gundump.h"
#include "gvm.h"
#include "gzio.h"




/*
** {======================================================
** Error-recovery functions
** =======================================================
*/


/* chain list of long jump buffers */
struct gafq_longjmp {
  struct gafq_longjmp *previous;
  gafqi_jmpbuf b;
  volatile int status;  /* error code */
};


void gafqD_seterrorobj (gafq_State *L, int errcode, StkId oldtop) {
  switch (errcode) {
    case GAFQ_ERRMEM: {
      setsvalue2s(L, oldtop, gafqS_newliteral(L, MEMERRMSG));
      break;
    }
    case GAFQ_ERRERR: {
      setsvalue2s(L, oldtop, gafqS_newliteral(L, "error in error handling"));
      break;
    }
    case GAFQ_ERRSYNTAX:
    case GAFQ_ERRRUN: {
      setobjs2s(L, oldtop, L->top - 1);  /* error message on current top */
      break;
    }
  }
  L->top = oldtop + 1;
}


static void restore_stack_limit (gafq_State *L) {
  gafq_assert(L->stack_last - L->stack == L->stacksize - EXTRA_STACK - 1);
  if (L->size_ci > GAFQI_MAXCALLS) {  /* there was an overflow? */
    int inuse = cast_int(L->ci - L->base_ci);
    if (inuse + 1 < GAFQI_MAXCALLS)  /* can `undo' overflow? */
      gafqD_reallocCI(L, GAFQI_MAXCALLS);
  }
}


static void resetstack (gafq_State *L, int status) {
  L->ci = L->base_ci;
  L->base = L->ci->base;
  gafqF_close(L, L->base);  /* close eventual pending closures */
  gafqD_seterrorobj(L, status, L->base);
  L->nCcalls = L->baseCcalls;
  L->allowhook = 1;
  restore_stack_limit(L);
  L->errfunc = 0;
  L->errorJmp = NULL;
}


void gafqD_throw (gafq_State *L, int errcode) {
  if (L->errorJmp) {
    L->errorJmp->status = errcode;
    GAFQI_THROW(L, L->errorJmp);
  }
  else {
    L->status = cast_byte(errcode);
    if (G(L)->panic) {
      resetstack(L, errcode);
      gafq_unlock(L);
      G(L)->panic(L);
    }
    exit(EXIT_FAILURE);
  }
}

// 在保护模式中运行函数f 启动时对应gapi中的f_Ccall
int gafqD_rawrunprotected (gafq_State *L, Pfunc f, void *ud) {
  struct gafq_longjmp lj;
  lj.status = 0;
  lj.previous = L->errorJmp;  /* chain new error handler */
  L->errorJmp = &lj;
  GAFQI_TRY(L, &lj,
    (*f)(L, ud);
  );
  L->errorJmp = lj.previous;  /* restore old error handler */
  return lj.status;
}

/* }====================================================== */


static void correctstack (gafq_State *L, TValue *oldstack) {
  CallInfo *ci;
  GCObject *up;
  L->top = (L->top - oldstack) + L->stack;
  for (up = L->openupval; up != NULL; up = up->gch.next)
    gco2uv(up)->v = (gco2uv(up)->v - oldstack) + L->stack;
  for (ci = L->base_ci; ci <= L->ci; ci++) {
    ci->top = (ci->top - oldstack) + L->stack;
    ci->base = (ci->base - oldstack) + L->stack;
    ci->func = (ci->func - oldstack) + L->stack;
  }
  L->base = (L->base - oldstack) + L->stack;
}


void gafqD_reallocstack (gafq_State *L, int newsize) {
  TValue *oldstack = L->stack;
  int realsize = newsize + 1 + EXTRA_STACK;
  gafq_assert(L->stack_last - L->stack == L->stacksize - EXTRA_STACK - 1);
  gafqM_reallocvector(L, L->stack, L->stacksize, realsize, TValue);
  L->stacksize = realsize;
  L->stack_last = L->stack+newsize;
  correctstack(L, oldstack);
}


void gafqD_reallocCI (gafq_State *L, int newsize) {
  CallInfo *oldci = L->base_ci;
  gafqM_reallocvector(L, L->base_ci, L->size_ci, newsize, CallInfo);
  L->size_ci = newsize;
  L->ci = (L->ci - oldci) + L->base_ci;
  L->end_ci = L->base_ci + L->size_ci - 1;
}


void gafqD_growstack (gafq_State *L, int n) {
  if (n <= L->stacksize)  /* double size is enough? */
    gafqD_reallocstack(L, 2*L->stacksize);
  else
    gafqD_reallocstack(L, L->stacksize + n);
}


static CallInfo *growCI (gafq_State *L) {
  if (L->size_ci > GAFQI_MAXCALLS)  /* overflow while handling overflow? */
    gafqD_throw(L, GAFQ_ERRERR);
  else {
    gafqD_reallocCI(L, 2*L->size_ci);
    if (L->size_ci > GAFQI_MAXCALLS)
      gafqG_runerror(L, "stack overflow");
  }
  return ++L->ci;
}


void gafqD_callhook (gafq_State *L, int event, int line) {
  gafq_Hook hook = L->hook;
  if (hook && L->allowhook) {
    ptrdiff_t top = savestack(L, L->top);
    ptrdiff_t ci_top = savestack(L, L->ci->top);
    gafq_Debug ar;
    ar.event = event;
    ar.currentline = line;
    if (event == GAFQ_HOOKTAILRET)
      ar.i_ci = 0;  /* tail call; no debug information about it */
    else
      ar.i_ci = cast_int(L->ci - L->base_ci);
    gafqD_checkstack(L, GAFQ_MINSTACK);  /* ensure minimum stack size */
    L->ci->top = L->top + GAFQ_MINSTACK;
    gafq_assert(L->ci->top <= L->stack_last);
    L->allowhook = 0;  /* cannot call hooks inside a hook */
    gafq_unlock(L);
    (*hook)(L, &ar);
    gafq_lock(L);
    gafq_assert(!L->allowhook);
    L->allowhook = 1;
    L->ci->top = restorestack(L, ci_top);
    L->top = restorestack(L, top);
  }
}


static StkId adjust_varargs (gafq_State *L, Proto *p, int actual) {
  int i;
  int nfixargs = p->numparams;
  Table *htab = NULL;
  StkId base, fixed;
  for (; actual < nfixargs; ++actual)
    setnilvalue(L->top++);
#if defined(GAFQ_COMPAT_VARARG)
  if (p->is_vararg & VARARG_NEEDSARG) { /* compat. with old-style vararg? */
    int nvar = actual - nfixargs;  /* number of extra arguments */
    gafq_assert(p->is_vararg & VARARG_HASARG);
    gafqC_checkGC(L);
    gafqD_checkstack(L, p->maxstacksize);
    htab = gafqH_new(L, nvar, 1);  /* create `arg' table */
    for (i=0; i<nvar; i++)  /* put extra arguments into `arg' table */
      setobj2n(L, gafqH_setnum(L, htab, i+1), L->top - nvar + i);
    /* store counter in field `n' */
    setnvalue(gafqH_setstr(L, htab, gafqS_newliteral(L, "n")), cast_num(nvar));
  }
#endif
  /* move fixed parameters to final position */
  fixed = L->top - actual;  /* first fixed argument */
  base = L->top;  /* final position of first argument */
  for (i=0; i<nfixargs; i++) {
    setobjs2s(L, L->top++, fixed+i);
    setnilvalue(fixed+i);
  }
  /* add `arg' parameter */
  if (htab) {
    sethvalue(L, L->top++, htab);
    gafq_assert(iswhite(obj2gco(htab)));
  }
  return base;
}


static StkId tryfuncTM (gafq_State *L, StkId func) {
  const TValue *tm = gafqT_gettmbyobj(L, func, TM_CALL);
  StkId p;
  ptrdiff_t funcr = savestack(L, func);
  if (!ttisfunction(tm))
    gafqG_typeerror(L, func, "call");
  /* Open a hole inside the stack at `func' */
  for (p = L->top; p > func; p--) setobjs2s(L, p, p-1);
  incr_top(L);
  func = restorestack(L, funcr);  /* previous call may change stack */
  setobj2s(L, func, tm);  /* tag method is the new function to be called */
  return func;
}



#define inc_ci(L) \
  ((L->ci == L->end_ci) ? growCI(L) : \
   (condhardstacktests(gafqD_reallocCI(L, L->size_ci)), ++L->ci))


int gafqD_precall (gafq_State *L, StkId func, int nresults) {
  LClosure *cl;
  ptrdiff_t funcr;
  if (!ttisfunction(func)) /* `func' is not a function? */
    func = tryfuncTM(L, func);  /* check the `function' tag method */
  funcr = savestack(L, func);
  cl = &clvalue(func)->l;
  L->ci->savedpc = L->savedpc;
  if (!cl->isC) {  /* Gafq function? prepare its call */
    CallInfo *ci;
    StkId st, base;
    Proto *p = cl->p;
    gafqD_checkstack(L, p->maxstacksize);
    func = restorestack(L, funcr);
    if (!p->is_vararg) {  /* no varargs? */
      base = func + 1;
      if (L->top > base + p->numparams)
        L->top = base + p->numparams;
    }
    else {  /* vararg function */
      int nargs = cast_int(L->top - func) - 1;
      base = adjust_varargs(L, p, nargs);
      func = restorestack(L, funcr);  /* previous call may change the stack */
    }
    ci = inc_ci(L);  /* now `enter' new function */
    ci->func = func;
    L->base = ci->base = base;
    ci->top = L->base + p->maxstacksize;
    gafq_assert(ci->top <= L->stack_last);
    L->savedpc = p->code;  /* starting point */
    ci->tailcalls = 0;
    ci->nresults = nresults;
    for (st = L->top; st < ci->top; st++)
      setnilvalue(st);
    L->top = ci->top;
    if (L->hookmask & GAFQ_MASKCALL) {
      L->savedpc++;  /* hooks assume 'pc' is already incremented */
      gafqD_callhook(L, GAFQ_HOOKCALL, -1);
      L->savedpc--;  /* correct 'pc' */
    }
    return PCRGAFQ;
  }
  else {  /* if is a C function, call it */
    CallInfo *ci;
    int n;
    gafqD_checkstack(L, GAFQ_MINSTACK);  /* ensure minimum stack size */
    ci = inc_ci(L);  /* now `enter' new function */
    ci->func = restorestack(L, funcr);
    L->base = ci->base = ci->func + 1;
    ci->top = L->top + GAFQ_MINSTACK;
    gafq_assert(ci->top <= L->stack_last);
    ci->nresults = nresults;
    if (L->hookmask & GAFQ_MASKCALL)
      gafqD_callhook(L, GAFQ_HOOKCALL, -1);
    gafq_unlock(L);
    n = (*curr_func(L)->c.f)(L);  /* do the actual call */
    gafq_lock(L);
    if (n < 0)  /* yielding? */
      return PCRYIELD;
    else {
      gafqD_poscall(L, L->top - n);
      return PCRC;
    }
  }
}


static StkId callrethooks (gafq_State *L, StkId firstResult) {
  ptrdiff_t fr = savestack(L, firstResult);  /* next call may change stack */
  gafqD_callhook(L, GAFQ_HOOKRET, -1);
  if (f_isGafq(L->ci)) {  /* Gafq function? */
    while ((L->hookmask & GAFQ_MASKRET) && L->ci->tailcalls--) /* tail calls */
      gafqD_callhook(L, GAFQ_HOOKTAILRET, -1);
  }
  return restorestack(L, fr);
}


int gafqD_poscall (gafq_State *L, StkId firstResult) {
  StkId res;
  int wanted, i;
  CallInfo *ci;
  if (L->hookmask & GAFQ_MASKRET)
    firstResult = callrethooks(L, firstResult);
  ci = L->ci--;
  res = ci->func;  /* res == final position of 1st result */
  wanted = ci->nresults;
  L->base = (ci - 1)->base;  /* restore base */
  L->savedpc = (ci - 1)->savedpc;  /* restore savedpc */
  /* move results to correct place */
  for (i = wanted; i != 0 && firstResult < L->top; i--)
    setobjs2s(L, res++, firstResult++);
  while (i-- > 0)
    setnilvalue(res++);
  L->top = res;
  return (wanted - GAFQ_MULTRET);  /* 0 iff wanted == GAFQ_MULTRET */
}


/*
** Call a function (C or Gafq). The function to be called is at *func.
** The arguments are on the stack, right after the function.
** When returns, all the results are on the stack, starting at the original
** function position.
*/ 
// 普通调用函数
void gafqD_call (gafq_State *L, StkId func, int nResults) {
  if (++L->nCcalls >= GAFQI_MAXCCALLS) {
    if (L->nCcalls == GAFQI_MAXCCALLS)
      gafqG_runerror(L, "C stack overflow");
    else if (L->nCcalls >= (GAFQI_MAXCCALLS + (GAFQI_MAXCCALLS>>3)))
      gafqD_throw(L, GAFQ_ERRERR);  /* error while handing stack error */
  }
  //好像如果是c方法gafqD_precall这里会执行, gafq方法就gafqV_execute执行
  if (gafqD_precall(L, func, nResults) == PCRGAFQ)  /* is a Gafq function? */
    gafqV_execute(L, 1);  /* call it */
  L->nCcalls--;
  gafqC_checkGC(L);
}


static void resume (gafq_State *L, void *ud) {
  StkId firstArg = cast(StkId, ud);
  CallInfo *ci = L->ci;
  if (L->status == 0) {  /* start coroutine? */
    gafq_assert(ci == L->base_ci && firstArg > L->base);
    if (gafqD_precall(L, firstArg - 1, GAFQ_MULTRET) != PCRGAFQ)
      return;
  }
  else {  /* resuming from previous yield */
    gafq_assert(L->status == GAFQ_YIELD);
    L->status = 0;
    if (!f_isGafq(ci)) {  /* `common' yield? */
      /* finish interrupted execution of `OP_CALL' */
      gafq_assert(GET_OPCODE(*((ci-1)->savedpc - 1)) == OP_CALL ||
                 GET_OPCODE(*((ci-1)->savedpc - 1)) == OP_TAILCALL);
      if (gafqD_poscall(L, firstArg))  /* complete it... */
        L->top = L->ci->top;  /* and correct top if not multiple results */
    }
    else  /* yielded inside a hook: just continue its execution */
      L->base = L->ci->base;
  }
  gafqV_execute(L, cast_int(L->ci - L->base_ci));
}


static int resume_error (gafq_State *L, const char *msg) {
  L->top = L->ci->base;
  setsvalue2s(L, L->top, gafqS_new(L, msg));
  incr_top(L);
  gafq_unlock(L);
  return GAFQ_ERRRUN;
}


GAFQ_API int gafq_resume (gafq_State *L, int nargs) {
  int status;
  gafq_lock(L);
  if (L->status != GAFQ_YIELD && (L->status != 0 || L->ci != L->base_ci))
      return resume_error(L, "cannot resume non-suspended coroutine");
  if (L->nCcalls >= GAFQI_MAXCCALLS)
    return resume_error(L, "C stack overflow");
  gafqi_userstateresume(L, nargs);
  gafq_assert(L->errfunc == 0);
  L->baseCcalls = ++L->nCcalls;
  status = gafqD_rawrunprotected(L, resume, L->top - nargs);
  if (status != 0) {  /* error? */
    L->status = cast_byte(status);  /* mark thread as `dead' */
    gafqD_seterrorobj(L, status, L->top);
    L->ci->top = L->top;
  }
  else {
    gafq_assert(L->nCcalls == L->baseCcalls);
    status = L->status;
  }
  --L->nCcalls;
  gafq_unlock(L);
  return status;
}


GAFQ_API int gafq_yield (gafq_State *L, int nresults) {
  gafqi_userstateyield(L, nresults);
  gafq_lock(L);
  if (L->nCcalls > L->baseCcalls)
    gafqG_runerror(L, "attempt to yield across metamethod/C-call boundary");
  L->base = L->top - nresults;  /* protect stack slots below */
  L->status = GAFQ_YIELD;
  gafq_unlock(L);
  return -1;
}

// 保护模式call： func为要调用的回调函数，u为用户数据,
// old_top 旧的栈顶，即pcall之前的栈顶
// ef 为错误处理函数的绝对偏移
int gafqD_pcall (gafq_State *L, Pfunc func, void *u,
                ptrdiff_t old_top, ptrdiff_t ef) {
  int status;
  unsigned short oldnCcalls = L->nCcalls;
  ptrdiff_t old_ci = saveci(L, L->ci);
  lu_byte old_allowhooks = L->allowhook;
  ptrdiff_t old_errfunc = L->errfunc;
  L->errfunc = ef;
  // 保护模式下调用回调函数
  status = gafqD_rawrunprotected(L, func, u);
  if (status != 0) {  /* an error occurred? */
                      // 如果发生错误，
    StkId oldtop = restorestack(L, old_top); // 取出旧的栈顶：即调用pcall时的那个栈
    gafqF_close(L, oldtop); /* close eventual pending closures */ // 把在恢复的栈上的upvalues关闭
    // 把栈顶的错误对象设给旧栈顶，并重设栈顶
    gafqD_seterrorobj(L, status, oldtop);
    L->nCcalls = oldnCcalls;
    L->ci = restoreci(L, old_ci);
    L->base = L->ci->base;
    L->savedpc = L->ci->savedpc;
    L->allowhook = old_allowhooks;
    restore_stack_limit(L);
  }
  L->errfunc = old_errfunc;
  return status;
}



/*
** Execute a protected parser.
*/
struct SParser {  /* data to `f_parser' */
  ZIO *z;
  Mbuffer buff;  /* buffer to be used by the scanner */
  const char *name;
};

// 解释器? 读取文件内容
static void f_parser (gafq_State *L, void *ud) {
  int i;
  Proto *tf;
  Closure *cl;
  struct SParser *p = cast(struct SParser *, ud);
  int c = gafqZ_lookahead(p->z);
  gafqC_checkGC(L);
  tf = ((c == GAFQ_SIGNATURE[0]) ? gafqU_undump : gafqY_parser)(L, p->z,
                                                             &p->buff, p->name);
  cl = gafqF_newLclosure(L, tf->nups, hvalue(gt(L)));
  cl->l.p = tf;
  for (i = 0; i < tf->nups; i++)  /* initialize eventual upvalues */
    cl->l.upvals[i] = gafqF_newupval(L);
  setclvalue(L, L->top, cl);
  incr_top(L);
}


int gafqD_protectedparser (gafq_State *L, ZIO *z, const char *name) {
  struct SParser p;
  int status;
  p.z = z; p.name = name;
  gafqZ_initbuffer(L, &p.buff);
  status = gafqD_pcall(L, f_parser, &p, savestack(L, L->top), L->errfunc);
  gafqZ_freebuffer(L, &p.buff);
  return status;
}


