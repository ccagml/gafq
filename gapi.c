/*
** $Id: gapi.c,v 2.55.1.5 2008/07/04 18:41:18 roberto Exp $
** Gafq API
** See Copyright Notice in gafq.h
*/


#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>

#define gapi_c
#define GAFQ_CORE

#include "gafq.h"

#include "gapi.h"
#include "gdebug.h"
#include "gdo.h"
#include "gfunc.h"
#include "ggc.h"
#include "gmem.h"
#include "gobject.h"
#include "gstate.h"
#include "gstring.h"
#include "gtable.h"
#include "gtm.h"
#include "gundump.h"
#include "gvm.h"



const char gafq_ident[] =
  "$Gafq: " GAFQ_RELEASE " " GAFQ_COPYRIGHT " $\n"
  "$Authors: " GAFQ_AUTHORS " $\n"
  "$URL: www.gafq.org $\n";



#define api_checknelems(L, n)	api_check(L, (n) <= (L->top - L->base))

#define api_checkvalidindex(L, i)	api_check(L, (i) != gafqO_nilobject)

#define api_incr_top(L)   {api_check(L, L->top < L->ci->top); L->top++;}


//看着是获取偏移地址
static TValue *index2adr (gafq_State *L, int idx) {
  if (idx > 0) {
    TValue *o = L->base + (idx - 1);
    api_check(L, idx <= L->ci->top - L->base);
    if (o >= L->top) return cast(TValue *, gafqO_nilobject);
    else return o;
  }
  else if (idx > GAFQ_REGISTRYINDEX) {
    api_check(L, idx != 0 && -idx <= L->top - L->base);
    return L->top + idx;
  }
  else switch (idx) {  /* pseudo-indices */
    case GAFQ_REGISTRYINDEX: return registry(L);
    case GAFQ_ENVIRONINDEX: {
      Closure *func = curr_func(L);
      sethvalue(L, &L->env, func->c.env);
      return &L->env;
    }
    case GAFQ_GLOBALSINDEX: return gt(L);
    default: {
      Closure *func = curr_func(L);
      idx = GAFQ_GLOBALSINDEX - idx;
      return (idx <= func->c.nupvalues)
                ? &func->c.upvalue[idx-1]
                : cast(TValue *, gafqO_nilobject);
    }
  }
}

//？作用域？
static Table *getcurrenv (gafq_State *L) {
  if (L->ci == L->base_ci)  /* no enclosing function? */
    return hvalue(gt(L));  /* use global table as environment */
  else {
    Closure *func = curr_func(L);
    return func->c.env;
  }
}

//把对象放到状态顶
void gafqA_pushobject (gafq_State *L, const TValue *o) {
  setobj2s(L, L->top, o);
  api_incr_top(L);
}

//检查栈
GAFQ_API int gafq_checkstack (gafq_State *L, int size) {
  int res = 1;
  gafq_lock(L);
  if (size > GAFQI_MAXCSTACK || (L->top - L->base + size) > GAFQI_MAXCSTACK)
    res = 0;  /* stack overflow */
  else if (size > 0) {
    gafqD_checkstack(L, size);
    if (L->ci->top < L->top + size)
      L->ci->top = L->top + size;
  }
  gafq_unlock(L);
  return res;
}

//把状态from转到to， 转移n个？
GAFQ_API void gafq_xmove (gafq_State *from, gafq_State *to, int n) {
  int i;
  if (from == to) return;
  gafq_lock(to);
  api_checknelems(from, n);
  api_check(from, G(from) == G(to));
  api_check(from, to->ci->top - to->top >= n);
  from->top -= n;
  for (i = 0; i < n; i++) {
    setobj2s(to, to->top++, from->top + i);
  }
  gafq_unlock(to);
}


GAFQ_API void gafq_setlevel (gafq_State *from, gafq_State *to) {
  to->nCcalls = from->nCcalls;
}

//替换恐慌函数
GAFQ_API gafq_CFunction gafq_atpanic (gafq_State *L, gafq_CFunction panicf) {
  gafq_CFunction old;
  gafq_lock(L);
  old = G(L)->panic;
  G(L)->panic = panicf;
  gafq_unlock(L);
  return old;
}

//？新线程？
GAFQ_API gafq_State *gafq_newthread (gafq_State *L) {
  gafq_State *L1;
  gafq_lock(L);
  gafqC_checkGC(L);
  L1 = gafqE_newthread(L);
  setthvalue(L, L->top, L1);
  api_incr_top(L);
  gafq_unlock(L);
  gafqi_userstatethread(L, L1);
  return L1;
}



/*
** basic stack manipulation
*/

//取顶部
GAFQ_API int gafq_gettop (gafq_State *L) {
  return cast_int(L->top - L->base);
}

//设置顶部
GAFQ_API void gafq_settop (gafq_State *L, int idx) {
  gafq_lock(L);
  if (idx >= 0) {
    api_check(L, idx <= L->stack_last - L->base);
    while (L->top < L->base + idx)
      setnilvalue(L->top++);
    L->top = L->base + idx;
  }
  else {
    api_check(L, -(idx+1) <= (L->top - L->base));
    L->top += idx+1;  /* `subtract' index (index is negative) */
  }
  gafq_unlock(L);
}

//移除第index个
GAFQ_API void gafq_remove (gafq_State *L, int idx) {
  StkId p;
  gafq_lock(L);
  p = index2adr(L, idx);
  api_checkvalidindex(L, p);
  while (++p < L->top) setobjs2s(L, p-1, p);
  L->top--;
  gafq_unlock(L);
}

//插入
GAFQ_API void gafq_insert (gafq_State *L, int idx) {
  StkId p;
  StkId q;
  gafq_lock(L);
  p = index2adr(L, idx);
  api_checkvalidindex(L, p);
  for (q = L->top; q>p; q--) setobjs2s(L, q, q-1);
  setobjs2s(L, p, L->top);
  gafq_unlock(L);
}

//替换
GAFQ_API void gafq_replace (gafq_State *L, int idx) {
  StkId o;
  gafq_lock(L);
  /* explicit test for incompatible code */
  if (idx == GAFQ_ENVIRONINDEX && L->ci == L->base_ci)
    gafqG_runerror(L, "no calling environment");
  api_checknelems(L, 1);
  o = index2adr(L, idx);
  api_checkvalidindex(L, o);
  if (idx == GAFQ_ENVIRONINDEX) {
    Closure *func = curr_func(L);
    api_check(L, ttistable(L->top - 1)); 
    func->c.env = hvalue(L->top - 1);
    gafqC_barrier(L, func, L->top - 1);
  }
  else {
    setobj(L, o, L->top - 1);
    if (idx < GAFQ_GLOBALSINDEX)  /* function upvalue? */
      gafqC_barrier(L, curr_func(L), L->top - 1);
  }
  L->top--;
  gafq_unlock(L);
}

//推一个值到顶部
GAFQ_API void gafq_pushvalue (gafq_State *L, int idx) {
  gafq_lock(L);
  setobj2s(L, L->top, index2adr(L, idx));
  api_incr_top(L);
  gafq_unlock(L);
}



/*
** access functions (stack -> C)
*/

//类型
GAFQ_API int gafq_type (gafq_State *L, int idx) {
  StkId o = index2adr(L, idx);
  return (o == gafqO_nilobject) ? GAFQ_TNONE : ttype(o);
}

//获取类型名称， nil, boolean, number, string,table 之类的
GAFQ_API const char *gafq_typename (gafq_State *L, int t) {
  UNUSED(L);
  return (t == GAFQ_TNONE) ? "no value" : gafqT_typenames[t];
}

//判断是否函数
GAFQ_API int gafq_iscfunction (gafq_State *L, int idx) {
  StkId o = index2adr(L, idx);
  return iscfunction(o);
}

//判断整数
GAFQ_API int gafq_isnumber (gafq_State *L, int idx) {
  TValue n;
  const TValue *o = index2adr(L, idx);
  return tonumber(o, &n);
}


GAFQ_API int gafq_isstring (gafq_State *L, int idx) {
  int t = gafq_type(L, idx);
  return (t == GAFQ_TSTRING || t == GAFQ_TNUMBER);
}


GAFQ_API int gafq_isuserdata (gafq_State *L, int idx) {
  const TValue *o = index2adr(L, idx);
  return (ttisuserdata(o) || ttislightuserdata(o));
}


GAFQ_API int gafq_rawequal (gafq_State *L, int index1, int index2) {
  StkId o1 = index2adr(L, index1);
  StkId o2 = index2adr(L, index2);
  return (o1 == gafqO_nilobject || o2 == gafqO_nilobject) ? 0
         : gafqO_rawequalObj(o1, o2);
}


GAFQ_API int gafq_equal (gafq_State *L, int index1, int index2) {
  StkId o1, o2;
  int i;
  gafq_lock(L);  /* may call tag method */
  o1 = index2adr(L, index1);
  o2 = index2adr(L, index2);
  i = (o1 == gafqO_nilobject || o2 == gafqO_nilobject) ? 0 : equalobj(L, o1, o2);
  gafq_unlock(L);
  return i;
}

//小于
GAFQ_API int gafq_lessthan (gafq_State *L, int index1, int index2) {
  StkId o1, o2;
  int i;
  gafq_lock(L);  /* may call tag method */
  o1 = index2adr(L, index1);
  o2 = index2adr(L, index2);
  i = (o1 == gafqO_nilobject || o2 == gafqO_nilobject) ? 0
       : gafqV_lessthan(L, o1, o2);
  gafq_unlock(L);
  return i;
}


//转成数字
GAFQ_API gafq_Number gafq_tonumber (gafq_State *L, int idx) {
  TValue n;
  const TValue *o = index2adr(L, idx);
  if (tonumber(o, &n))
    return nvalue(o);
  else
    return 0;
}

// 获取数字
GAFQ_API gafq_Integer gafq_tointeger (gafq_State *L, int idx) {
  TValue n;
  const TValue *o = index2adr(L, idx);
  if (tonumber(o, &n)) {
    gafq_Integer res;
    gafq_Number num = nvalue(o);
    gafq_number2integer(res, num);
    return res;
  }
  else
    return 0;
}

//转成布尔
GAFQ_API int gafq_toboolean (gafq_State *L, int idx) {
  const TValue *o = index2adr(L, idx);
  return !l_isfalse(o);
}

//获取字符串，会在len的指针设置长度
GAFQ_API const char *gafq_togstring (gafq_State *L, int idx, size_t *len) {
  StkId o = index2adr(L, idx);
  if (!ttisstring(o)) {
    gafq_lock(L);  /* `gafqV_tostring' may create a new string */
    if (!gafqV_tostring(L, o)) {  /* conversion failed? */
      if (len != NULL) *len = 0;
      gafq_unlock(L);
      return NULL;
    }
    gafqC_checkGC(L);
    o = index2adr(L, idx);  /* previous call may reallocate the stack */
    gafq_unlock(L);
  }
  if (len != NULL) *len = tsvalue(o)->len;
  return svalue(o);
}

//获取对象长度
GAFQ_API size_t gafq_objlen (gafq_State *L, int idx) {
  StkId o = index2adr(L, idx);
  switch (ttype(o)) {
    case GAFQ_TSTRING: return tsvalue(o)->len;
    case GAFQ_TUSERDATA: return uvalue(o)->len;
    case GAFQ_TTABLE: return gafqH_getn(hvalue(o));
    case GAFQ_TNUMBER: {
      size_t l;
      gafq_lock(L);  /* `gafqV_tostring' may create a new string */
      l = (gafqV_tostring(L, o) ? tsvalue(o)->len : 0);
      gafq_unlock(L);
      return l;
    }
    default: return 0;
  }
}


GAFQ_API gafq_CFunction gafq_tocfunction (gafq_State *L, int idx) {
  StkId o = index2adr(L, idx);
  return (!iscfunction(o)) ? NULL : clvalue(o)->c.f;
}


GAFQ_API void *gafq_touserdata (gafq_State *L, int idx) {
  StkId o = index2adr(L, idx);
  switch (ttype(o)) {
    case GAFQ_TUSERDATA: return (rawuvalue(o) + 1);
    case GAFQ_TLIGHTUSERDATA: return pvalue(o);
    default: return NULL;
  }
}


GAFQ_API gafq_State *gafq_tothread (gafq_State *L, int idx) {
  StkId o = index2adr(L, idx);
  return (!ttisthread(o)) ? NULL : thvalue(o);
}


GAFQ_API const void *gafq_topointer (gafq_State *L, int idx) {
  StkId o = index2adr(L, idx);
  switch (ttype(o)) {
    case GAFQ_TTABLE: return hvalue(o);
    case GAFQ_TFUNCTION: return clvalue(o);
    case GAFQ_TTHREAD: return thvalue(o);
    case GAFQ_TUSERDATA:
    case GAFQ_TLIGHTUSERDATA:
      return gafq_touserdata(L, idx);
    default: return NULL;
  }
}



/*
** push functions (C -> stack)
*/


GAFQ_API void gafq_pushnil (gafq_State *L) {
  gafq_lock(L);
  setnilvalue(L->top);
  api_incr_top(L);
  gafq_unlock(L);
}


GAFQ_API void gafq_pushnumber (gafq_State *L, gafq_Number n) {
  gafq_lock(L);
  setnvalue(L->top, n);
  api_incr_top(L);
  gafq_unlock(L);
}

//插入一个整数
GAFQ_API void gafq_pushinteger (gafq_State *L, gafq_Integer n) {
  gafq_lock(L);
  setnvalue(L->top, cast_num(n));
  api_incr_top(L);
  gafq_unlock(L);
}


GAFQ_API void gafq_pushgstring (gafq_State *L, const char *s, size_t len) {
  gafq_lock(L);
  gafqC_checkGC(L);
  setsvalue2s(L, L->top, gafqS_newlstr(L, s, len));
  api_incr_top(L);
  gafq_unlock(L);
}


GAFQ_API void gafq_pushstring (gafq_State *L, const char *s) {
  if (s == NULL)
    gafq_pushnil(L);
  else
    gafq_pushgstring(L, s, strlen(s));
}


GAFQ_API const char *gafq_pushvfstring (gafq_State *L, const char *fmt,
                                      va_list argp) {
  const char *ret;
  gafq_lock(L);
  gafqC_checkGC(L);
  ret = gafqO_pushvfstring(L, fmt, argp);
  gafq_unlock(L);
  return ret;
}


GAFQ_API const char *gafq_pushfstring (gafq_State *L, const char *fmt, ...) {
  const char *ret;
  va_list argp;
  gafq_lock(L);
  gafqC_checkGC(L);
  va_start(argp, fmt);
  ret = gafqO_pushvfstring(L, fmt, argp);
  va_end(argp);
  gafq_unlock(L);
  return ret;
}


GAFQ_API void gafq_pushcclosure (gafq_State *L, gafq_CFunction fn, int n) {
  Closure *cl;
  gafq_lock(L);
  gafqC_checkGC(L);
  api_checknelems(L, n);
  cl = gafqF_newCclosure(L, n, getcurrenv(L));
  cl->c.f = fn;
  L->top -= n;
  while (n--)
    setobj2n(L, &cl->c.upvalue[n], L->top+n);
  setclvalue(L, L->top, cl);
  gafq_assert(iswhite(obj2gco(cl)));
  api_incr_top(L);
  gafq_unlock(L);
}


GAFQ_API void gafq_pushboolean (gafq_State *L, int b) {
  gafq_lock(L);
  setbvalue(L->top, (b != 0));  /* ensure that true is 1 */
  api_incr_top(L);
  gafq_unlock(L);
}


GAFQ_API void gafq_pushlightuserdata (gafq_State *L, void *p) {
  gafq_lock(L);
  setpvalue(L->top, p);
  api_incr_top(L);
  gafq_unlock(L);
}


GAFQ_API int gafq_pushthread (gafq_State *L) {
  gafq_lock(L);
  setthvalue(L, L->top, L);
  api_incr_top(L);
  gafq_unlock(L);
  return (G(L)->mainthread == L);
}



/*
** get functions (Gafq -> stack)
*/


GAFQ_API void gafq_gettable (gafq_State *L, int idx) {
  StkId t;
  gafq_lock(L);
  t = index2adr(L, idx);
  api_checkvalidindex(L, t);
  gafqV_gettable(L, t, L->top - 1, L->top - 1);
  gafq_unlock(L);
}


GAFQ_API void gafq_getfield (gafq_State *L, int idx, const char *k) {
  StkId t;
  TValue key;
  gafq_lock(L);
  t = index2adr(L, idx);
  api_checkvalidindex(L, t);
  setsvalue(L, &key, gafqS_new(L, k));
  gafqV_gettable(L, t, &key, L->top);
  api_incr_top(L);
  gafq_unlock(L);
}


GAFQ_API void gafq_rawget (gafq_State *L, int idx) {
  StkId t;
  gafq_lock(L);
  t = index2adr(L, idx);
  api_check(L, ttistable(t));
  setobj2s(L, L->top - 1, gafqH_get(hvalue(t), L->top - 1));
  gafq_unlock(L);
}


GAFQ_API void gafq_rawgeti (gafq_State *L, int idx, int n) {
  StkId o;
  gafq_lock(L);
  o = index2adr(L, idx);
  api_check(L, ttistable(o));
  setobj2s(L, L->top, gafqH_getnum(hvalue(o), n));
  api_incr_top(L);
  gafq_unlock(L);
}

//创建一个表
GAFQ_API void gafq_createtable (gafq_State *L, int narray, int nrec) {
  gafq_lock(L);
  gafqC_checkGC(L);
  sethvalue(L, L->top, gafqH_new(L, narray, nrec));
  api_incr_top(L);
  gafq_unlock(L);
}


GAFQ_API int gafq_getmetatable (gafq_State *L, int objindex) {
  const TValue *obj;
  Table *mt = NULL;
  int res;
  gafq_lock(L);
  obj = index2adr(L, objindex);
  switch (ttype(obj)) {
    case GAFQ_TTABLE:
      mt = hvalue(obj)->metatable;
      break;
    case GAFQ_TUSERDATA:
      mt = uvalue(obj)->metatable;
      break;
    default:
      mt = G(L)->mt[ttype(obj)];
      break;
  }
  if (mt == NULL)
    res = 0;
  else {
    sethvalue(L, L->top, mt);
    api_incr_top(L);
    res = 1;
  }
  gafq_unlock(L);
  return res;
}


GAFQ_API void gafq_getfenv (gafq_State *L, int idx) {
  StkId o;
  gafq_lock(L);
  o = index2adr(L, idx);
  api_checkvalidindex(L, o);
  switch (ttype(o)) {
    case GAFQ_TFUNCTION:
      sethvalue(L, L->top, clvalue(o)->c.env);
      break;
    case GAFQ_TUSERDATA:
      sethvalue(L, L->top, uvalue(o)->env);
      break;
    case GAFQ_TTHREAD:
      setobj2s(L, L->top,  gt(thvalue(o)));
      break;
    default:
      setnilvalue(L->top);
      break;
  }
  api_incr_top(L);
  gafq_unlock(L);
}


/*
** set functions (stack -> Gafq)
*/


GAFQ_API void gafq_settable (gafq_State *L, int idx) {
  StkId t;
  gafq_lock(L);
  api_checknelems(L, 2);
  t = index2adr(L, idx);
  api_checkvalidindex(L, t);
  gafqV_settable(L, t, L->top - 2, L->top - 1);
  L->top -= 2;  /* pop index and value */
  gafq_unlock(L);
}


GAFQ_API void gafq_setfield (gafq_State *L, int idx, const char *k) {
  StkId t;
  TValue key;
  gafq_lock(L);
  api_checknelems(L, 1);
  t = index2adr(L, idx);
  api_checkvalidindex(L, t);
  setsvalue(L, &key, gafqS_new(L, k));
  gafqV_settable(L, t, &key, L->top - 1);
  L->top--;  /* pop value */
  gafq_unlock(L);
}


GAFQ_API void gafq_rawset (gafq_State *L, int idx) {
  StkId t;
  gafq_lock(L);
  api_checknelems(L, 2);
  t = index2adr(L, idx);
  api_check(L, ttistable(t));
  setobj2t(L, gafqH_set(L, hvalue(t), L->top-2), L->top-1);
  gafqC_barriert(L, hvalue(t), L->top-1);
  L->top -= 2;
  gafq_unlock(L);
}


GAFQ_API void gafq_rawseti (gafq_State *L, int idx, int n) {
  StkId o;
  gafq_lock(L);
  api_checknelems(L, 1);
  o = index2adr(L, idx);
  api_check(L, ttistable(o));
  setobj2t(L, gafqH_setnum(L, hvalue(o), n), L->top-1);
  gafqC_barriert(L, hvalue(o), L->top-1);
  L->top--;
  gafq_unlock(L);
}


GAFQ_API int gafq_setmetatable (gafq_State *L, int objindex) {
  TValue *obj;
  Table *mt;
  gafq_lock(L);
  api_checknelems(L, 1);
  obj = index2adr(L, objindex);
  api_checkvalidindex(L, obj);
  if (ttisnil(L->top - 1))
    mt = NULL;
  else {
    api_check(L, ttistable(L->top - 1));
    mt = hvalue(L->top - 1);
  }
  switch (ttype(obj)) {
    case GAFQ_TTABLE: {
      hvalue(obj)->metatable = mt;
      if (mt)
        gafqC_objbarriert(L, hvalue(obj), mt);
      break;
    }
    case GAFQ_TUSERDATA: {
      uvalue(obj)->metatable = mt;
      if (mt)
        gafqC_objbarrier(L, rawuvalue(obj), mt);
      break;
    }
    default: {
      G(L)->mt[ttype(obj)] = mt;
      break;
    }
  }
  L->top--;
  gafq_unlock(L);
  return 1;
}


GAFQ_API int gafq_setfenv (gafq_State *L, int idx) {
  StkId o;
  int res = 1;
  gafq_lock(L);
  api_checknelems(L, 1);
  o = index2adr(L, idx);
  api_checkvalidindex(L, o);
  api_check(L, ttistable(L->top - 1));
  switch (ttype(o)) {
    case GAFQ_TFUNCTION:
      clvalue(o)->c.env = hvalue(L->top - 1);
      break;
    case GAFQ_TUSERDATA:
      uvalue(o)->env = hvalue(L->top - 1);
      break;
    case GAFQ_TTHREAD:
      sethvalue(L, gt(thvalue(o)), hvalue(L->top - 1));
      break;
    default:
      res = 0;
      break;
  }
  if (res) gafqC_objbarrier(L, gcvalue(o), hvalue(L->top - 1));
  L->top--;
  gafq_unlock(L);
  return res;
}


/*
** `load' and `call' functions (run Gafq code)
*/


#define adjustresults(L,nres) \
    { if (nres == GAFQ_MULTRET && L->top >= L->ci->top) L->ci->top = L->top; }


#define checkresults(L,na,nr) \
     api_check(L, (nr) == GAFQ_MULTRET || (L->ci->top - L->top >= (nr) - (na)))
	

GAFQ_API void gafq_call (gafq_State *L, int nargs, int nresults) {
  StkId func;
  gafq_lock(L);
  api_checknelems(L, nargs+1);
  checkresults(L, nargs, nresults);
  func = L->top - (nargs+1);
  gafqD_call(L, func, nresults);
  adjustresults(L, nresults);
  gafq_unlock(L);
}



/*
** Execute a protected call.
*/
struct CallS {  /* data to `f_call' */
  StkId func;
  int nresults;
};


static void f_call (gafq_State *L, void *ud) {
  struct CallS *c = cast(struct CallS *, ud);
  gafqD_call(L, c->func, c->nresults);
}



GAFQ_API int gafq_pcall (gafq_State *L, int nargs, int nresults, int errfunc) {
  struct CallS c;
  int status;
  ptrdiff_t func;
  gafq_lock(L);
  api_checknelems(L, nargs+1);
  checkresults(L, nargs, nresults);
  if (errfunc == 0)
    func = 0;
  else {
    StkId o = index2adr(L, errfunc);
    api_checkvalidindex(L, o);
    func = savestack(L, o);
  }
  c.func = L->top - (nargs+1);  /* function to be called */
  c.nresults = nresults;
  status = gafqD_pcall(L, f_call, &c, savestack(L, c.func), func);
  adjustresults(L, nresults);
  gafq_unlock(L);
  return status;
}


/*
** Execute a protected C call.
*/
struct CCallS {  /* data to `f_Ccall' */
  gafq_CFunction func;
  void *ud;
};


static void f_Ccall (gafq_State *L, void *ud) {
  struct CCallS *c = cast(struct CCallS *, ud);
  Closure *cl;
  cl = gafqF_newCclosure(L, 0, getcurrenv(L));
  cl->c.f = c->func;
  setclvalue(L, L->top, cl);  /* push function */
  api_incr_top(L);
  setpvalue(L->top, c->ud);  /* push only argument */
  api_incr_top(L);
  gafqD_call(L, L->top - 2, 0);
}


GAFQ_API int gafq_cpcall (gafq_State *L, gafq_CFunction func, void *ud) {
  struct CCallS c;
  int status;
  gafq_lock(L);
  c.func = func;
  c.ud = ud;
  status = gafqD_pcall(L, f_Ccall, &c, savestack(L, L->top), 0);
  gafq_unlock(L);
  return status;
}


GAFQ_API int gafq_load (gafq_State *L, gafq_Reader reader, void *data,
                      const char *chunkname) {
  ZIO z;
  int status;
  gafq_lock(L);
  if (!chunkname) chunkname = "?";
  gafqZ_init(L, &z, reader, data);
  status = gafqD_protectedparser(L, &z, chunkname);
  gafq_unlock(L);
  return status;
}


GAFQ_API int gafq_dump (gafq_State *L, gafq_Writer writer, void *data) {
  int status;
  TValue *o;
  gafq_lock(L);
  api_checknelems(L, 1);
  o = L->top - 1;
  if (isLfunction(o))
    status = gafqU_dump(L, clvalue(o)->l.p, writer, data, 0);
  else
    status = 1;
  gafq_unlock(L);
  return status;
}


GAFQ_API int  gafq_status (gafq_State *L) {
  return L->status;
}


/*
** Garbage-collection function
*/

GAFQ_API int gafq_gc (gafq_State *L, int what, int data) {
  int res = 0;
  global_State *g;
  gafq_lock(L);
  g = G(L);
  switch (what) {
    case GAFQ_GCSTOP: {
      g->GCthreshold = MAX_LUMEM;
      break;
    }
    case GAFQ_GCRESTART: {
      g->GCthreshold = g->totalbytes;
      break;
    }
    case GAFQ_GCCOLLECT: {
      gafqC_fullgc(L);
      break;
    }
    case GAFQ_GCCOUNT: {
      /* GC values are expressed in Kbytes: #bytes/2^10 */
      res = cast_int(g->totalbytes >> 10);
      break;
    }
    case GAFQ_GCCOUNTB: {
      res = cast_int(g->totalbytes & 0x3ff);
      break;
    }
    case GAFQ_GCSTEP: {
      lu_mem a = (cast(lu_mem, data) << 10);
      if (a <= g->totalbytes)
        g->GCthreshold = g->totalbytes - a;
      else
        g->GCthreshold = 0;
      while (g->GCthreshold <= g->totalbytes) {
        gafqC_step(L);
        if (g->gcstate == GCSpause) {  /* end of cycle? */
          res = 1;  /* signal it */
          break;
        }
      }
      break;
    }
    case GAFQ_GCSETPAUSE: {
      res = g->gcpause;
      g->gcpause = data;
      break;
    }
    case GAFQ_GCSETSTEPMUL: {
      res = g->gcstepmul;
      g->gcstepmul = data;
      break;
    }
    default: res = -1;  /* invalid option */
  }
  gafq_unlock(L);
  return res;
}



/*
** miscellaneous functions
*/


GAFQ_API int gafq_error (gafq_State *L) {
  gafq_lock(L);
  api_checknelems(L, 1);
  gafqG_errormsg(L);
  gafq_unlock(L);
  return 0;  /* to avoid warnings */
}


GAFQ_API int gafq_next (gafq_State *L, int idx) {
  StkId t;
  int more;
  gafq_lock(L);
  t = index2adr(L, idx);
  api_check(L, ttistable(t));
  more = gafqH_next(L, hvalue(t), L->top - 1);
  if (more) {
    api_incr_top(L);
  }
  else  /* no more elements */
    L->top -= 1;  /* remove key */
  gafq_unlock(L);
  return more;
}

//看着是要合并字符串
GAFQ_API void gafq_concat (gafq_State *L, int n) {
  gafq_lock(L);
  api_checknelems(L, n);
  if (n >= 2) {
    gafqC_checkGC(L);
    gafqV_concat(L, n, cast_int(L->top - L->base) - 1);
    L->top -= (n-1);
  }
  else if (n == 0) {  /* push empty string */
    setsvalue2s(L, L->top, gafqS_newlstr(L, "", 0));
    api_incr_top(L);
  }
  /* else n == 1; nothing to do */
  gafq_unlock(L);
}


GAFQ_API gafq_Alloc gafq_getallocf (gafq_State *L, void **ud) {
  gafq_Alloc f;
  gafq_lock(L);
  if (ud) *ud = G(L)->ud;
  f = G(L)->frealloc;
  gafq_unlock(L);
  return f;
}


GAFQ_API void gafq_setallocf (gafq_State *L, gafq_Alloc f, void *ud) {
  gafq_lock(L);
  G(L)->ud = ud;
  G(L)->frealloc = f;
  gafq_unlock(L);
}


GAFQ_API void *gafq_newuserdata (gafq_State *L, size_t size) {
  Udata *u;
  gafq_lock(L);
  gafqC_checkGC(L);
  u = gafqS_newudata(L, size, getcurrenv(L));
  setuvalue(L, L->top, u);
  api_incr_top(L);
  gafq_unlock(L);
  return u + 1;
}




static const char *aux_upvalue (StkId fi, int n, TValue **val) {
  Closure *f;
  if (!ttisfunction(fi)) return NULL;
  f = clvalue(fi);
  if (f->c.isC) {
    if (!(1 <= n && n <= f->c.nupvalues)) return NULL;
    *val = &f->c.upvalue[n-1];
    return "";
  }
  else {
    Proto *p = f->l.p;
    if (!(1 <= n && n <= p->sizeupvalues)) return NULL;
    *val = f->l.upvals[n-1]->v;
    return getstr(p->upvalues[n-1]);
  }
}


GAFQ_API const char *gafq_getupvalue (gafq_State *L, int funcindex, int n) {
  const char *name;
  TValue *val;
  gafq_lock(L);
  name = aux_upvalue(index2adr(L, funcindex), n, &val);
  if (name) {
    setobj2s(L, L->top, val);
    api_incr_top(L);
  }
  gafq_unlock(L);
  return name;
}


GAFQ_API const char *gafq_setupvalue (gafq_State *L, int funcindex, int n) {
  const char *name;
  TValue *val;
  StkId fi;
  gafq_lock(L);
  fi = index2adr(L, funcindex);
  api_checknelems(L, 1);
  name = aux_upvalue(fi, n, &val);
  if (name) {
    L->top--;
    setobj(L, val, L->top);
    gafqC_barrier(L, clvalue(fi), L->top);
  }
  gafq_unlock(L);
  return name;
}

