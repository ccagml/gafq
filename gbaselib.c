/*
** $Id: gbaselib.c,v 1.191.1.6 2008/02/14 16:46:22 roberto Exp $
** Basic library
** See Copyright Notice in gafq.h
*/



#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define gbaselib_c
#define GAFQ_LIB

#include "gafq.h"

#include "gauxlib.h"
#include "gafqlib.h"




/*
** If your system does not support `stdout', you can just remove this function.
** If you need, you can define your own `print' function, following this
** model but changing `fputs' to put the strings at a proper place
** (a console window or a log file, for instance).
*/
static int gafqB_print (gafq_State *L) {
  int n = gafq_gettop(L);  /* number of arguments */
  int i;
  gafq_getglobal(L, "tostring");
  for (i=1; i<=n; i++) {
    const char *s;
    gafq_pushvalue(L, -1);  /* function to be called */
    gafq_pushvalue(L, i);   /* value to print */
    gafq_call(L, 1, 1);
    s = gafq_tostring(L, -1);  /* get result */
    if (s == NULL)
      return gafqL_error(L, GAFQ_QL("tostring") " must return a string to "
                           GAFQ_QL("print"));
    if (i>1) fputs("\t", stdout);
    fputs(s, stdout);
    gafq_pop(L, 1);  /* pop result */
  }
  fputs("\n", stdout);
  return 0;
}


static int gafqB_tonumber (gafq_State *L) {
  int base = gafqL_optint(L, 2, 10);
  if (base == 10) {  /* standard conversion */
    gafqL_checkany(L, 1);
    if (gafq_isnumber(L, 1)) {
      gafq_pushnumber(L, gafq_tonumber(L, 1));
      return 1;
    }
  }
  else {
    const char *s1 = gafqL_checkstring(L, 1);
    char *s2;
    unsigned long n;
    gafqL_argcheck(L, 2 <= base && base <= 36, 2, "base out of range");
    n = strtoul(s1, &s2, base);
    if (s1 != s2) {  /* at least one valid digit? */
      while (isspace((unsigned char)(*s2))) s2++;  /* skip trailing spaces */
      if (*s2 == '\0') {  /* no invalid trailing characters? */
        gafq_pushnumber(L, (gafq_Number)n);
        return 1;
      }
    }
  }
  gafq_pushnil(L);  /* else not a number */
  return 1;
}


static int gafqB_error (gafq_State *L) {
  int level = gafqL_optint(L, 2, 1);
  gafq_settop(L, 1);
  if (gafq_isstring(L, 1) && level > 0) {  /* add extra information? */
    gafqL_where(L, level);
    gafq_pushvalue(L, 1);
    gafq_concat(L, 2);
  }
  return gafq_error(L);
}


static int gafqB_getmetatable (gafq_State *L) {
  gafqL_checkany(L, 1);
  if (!gafq_getmetatable(L, 1)) {
    gafq_pushnil(L);
    return 1;  /* no metatable */
  }
  gafqL_getmetafield(L, 1, "__metatable");
  return 1;  /* returns either __metatable field (if present) or metatable */
}


static int gafqB_setmetatable (gafq_State *L) {
  int t = gafq_type(L, 2);
  gafqL_checktype(L, 1, GAFQ_TTABLE);
  gafqL_argcheck(L, t == GAFQ_TNIL || t == GAFQ_TTABLE, 2,
                    "nil or table expected");
  if (gafqL_getmetafield(L, 1, "__metatable"))
    gafqL_error(L, "cannot change a protected metatable");
  gafq_settop(L, 2);
  gafq_setmetatable(L, 1);
  return 1;
}


static void getfunc (gafq_State *L, int opt) {
  if (gafq_isfunction(L, 1)) gafq_pushvalue(L, 1);
  else {
    gafq_Debug ar;
    int level = opt ? gafqL_optint(L, 1, 1) : gafqL_checkint(L, 1);
    gafqL_argcheck(L, level >= 0, 1, "level must be non-negative");
    if (gafq_getstack(L, level, &ar) == 0)
      gafqL_argerror(L, 1, "invalid level");
    gafq_getinfo(L, "f", &ar);
    if (gafq_isnil(L, -1))
      gafqL_error(L, "no function environment for tail call at level %d",
                    level);
  }
}


static int gafqB_getfenv (gafq_State *L) {
  getfunc(L, 1);
  if (gafq_iscfunction(L, -1))  /* is a C function? */
    gafq_pushvalue(L, GAFQ_GLOBALSINDEX);  /* return the thread's global env. */
  else
    gafq_getfenv(L, -1);
  return 1;
}


static int gafqB_setfenv (gafq_State *L) {
  gafqL_checktype(L, 2, GAFQ_TTABLE);
  getfunc(L, 0);
  gafq_pushvalue(L, 2);
  if (gafq_isnumber(L, 1) && gafq_tonumber(L, 1) == 0) {
    /* change environment of current thread */
    gafq_pushthread(L);
    gafq_insert(L, -2);
    gafq_setfenv(L, -2);
    return 0;
  }
  else if (gafq_iscfunction(L, -2) || gafq_setfenv(L, -2) == 0)
    gafqL_error(L,
          GAFQ_QL("setfenv") " cannot change environment of given object");
  return 1;
}


static int gafqB_rawequal (gafq_State *L) {
  gafqL_checkany(L, 1);
  gafqL_checkany(L, 2);
  gafq_pushboolean(L, gafq_rawequal(L, 1, 2));
  return 1;
}


static int gafqB_rawget (gafq_State *L) {
  gafqL_checktype(L, 1, GAFQ_TTABLE);
  gafqL_checkany(L, 2);
  gafq_settop(L, 2);
  gafq_rawget(L, 1);
  return 1;
}

static int gafqB_rawset (gafq_State *L) {
  gafqL_checktype(L, 1, GAFQ_TTABLE);
  gafqL_checkany(L, 2);
  gafqL_checkany(L, 3);
  gafq_settop(L, 3);
  gafq_rawset(L, 1);
  return 1;
}


static int gafqB_gcinfo (gafq_State *L) {
  gafq_pushinteger(L, gafq_getgccount(L));
  return 1;
}


static int gafqB_collectgarbage (gafq_State *L) {
  static const char *const opts[] = {"stop", "restart", "collect",
    "count", "step", "setpause", "setstepmul", NULL};
  static const int optsnum[] = {GAFQ_GCSTOP, GAFQ_GCRESTART, GAFQ_GCCOLLECT,
    GAFQ_GCCOUNT, GAFQ_GCSTEP, GAFQ_GCSETPAUSE, GAFQ_GCSETSTEPMUL};
  int o = gafqL_checkoption(L, 1, "collect", opts);
  int ex = gafqL_optint(L, 2, 0);
  int res = gafq_gc(L, optsnum[o], ex);
  switch (optsnum[o]) {
    case GAFQ_GCCOUNT: {
      int b = gafq_gc(L, GAFQ_GCCOUNTB, 0);
      gafq_pushnumber(L, res + ((gafq_Number)b/1024));
      return 1;
    }
    case GAFQ_GCSTEP: {
      gafq_pushboolean(L, res);
      return 1;
    }
    default: {
      gafq_pushnumber(L, res);
      return 1;
    }
  }
}


static int gafqB_type (gafq_State *L) {
  gafqL_checkany(L, 1);
  gafq_pushstring(L, gafqL_typename(L, 1));
  return 1;
}


static int gafqB_next (gafq_State *L) {
  gafqL_checktype(L, 1, GAFQ_TTABLE);
  gafq_settop(L, 2);  /* create a 2nd argument if there isn't one */
  if (gafq_next(L, 1))
    return 2;
  else {
    gafq_pushnil(L);
    return 1;
  }
}


static int gafqB_pairs (gafq_State *L) {
  gafqL_checktype(L, 1, GAFQ_TTABLE);
  gafq_pushvalue(L, gafq_upvalueindex(1));  /* return generator, */
  gafq_pushvalue(L, 1);  /* state, */
  gafq_pushnil(L);  /* and initial value */
  return 3;
}


static int ipairsaux (gafq_State *L) {
  int i = gafqL_checkint(L, 2);
  gafqL_checktype(L, 1, GAFQ_TTABLE);
  i++;  /* next value */
  gafq_pushinteger(L, i);
  gafq_rawgeti(L, 1, i);
  return (gafq_isnil(L, -1)) ? 0 : 2;
}


static int gafqB_ipairs (gafq_State *L) {
  gafqL_checktype(L, 1, GAFQ_TTABLE);
  gafq_pushvalue(L, gafq_upvalueindex(1));  /* return generator, */
  gafq_pushvalue(L, 1);  /* state, */
  gafq_pushinteger(L, 0);  /* and initial value */
  return 3;
}


static int load_aux (gafq_State *L, int status) {
  if (status == 0)  /* OK? */
    return 1;
  else {
    gafq_pushnil(L);
    gafq_insert(L, -2);  /* put before error message */
    return 2;  /* return nil plus error message */
  }
}


static int gafqB_loadstring (gafq_State *L) {
  size_t l;
  const char *s = gafqL_checklstring(L, 1, &l);
  const char *chunkname = gafqL_optstring(L, 2, s);
  return load_aux(L, gafqL_loadbuffer(L, s, l, chunkname));
}


static int gafqB_loadfile (gafq_State *L) {
  const char *fname = gafqL_optstring(L, 1, NULL);
  return load_aux(L, gafqL_loadfile(L, fname));
}


/*
** Reader for generic `load' function: `gafq_load' uses the
** stack for internal stuff, so the reader cannot change the
** stack top. Instead, it keeps its resulting string in a
** reserved slot inside the stack.
*/
static const char *generic_reader (gafq_State *L, void *ud, size_t *size) {
  (void)ud;  /* to avoid warnings */
  gafqL_checkstack(L, 2, "too many nested functions");
  gafq_pushvalue(L, 1);  /* get function */
  gafq_call(L, 0, 1);  /* call it */
  if (gafq_isnil(L, -1)) {
    *size = 0;
    return NULL;
  }
  else if (gafq_isstring(L, -1)) {
    gafq_replace(L, 3);  /* save string in a reserved stack slot */
    return gafq_tolstring(L, 3, size);
  }
  else gafqL_error(L, "reader function must return a string");
  return NULL;  /* to avoid warnings */
}


static int gafqB_load (gafq_State *L) {
  int status;
  const char *cname = gafqL_optstring(L, 2, "=(load)");
  gafqL_checktype(L, 1, GAFQ_TFUNCTION);
  gafq_settop(L, 3);  /* function, eventual name, plus one reserved slot */
  status = gafq_load(L, generic_reader, NULL, cname);
  return load_aux(L, status);
}


static int gafqB_dofile (gafq_State *L) {
  const char *fname = gafqL_optstring(L, 1, NULL);
  int n = gafq_gettop(L);
  if (gafqL_loadfile(L, fname) != 0) gafq_error(L);
  gafq_call(L, 0, GAFQ_MULTRET);
  return gafq_gettop(L) - n;
}


static int gafqB_assert (gafq_State *L) {
  gafqL_checkany(L, 1);
  if (!gafq_toboolean(L, 1))
    return gafqL_error(L, "%s", gafqL_optstring(L, 2, "assertion failed!"));
  return gafq_gettop(L);
}


static int gafqB_unpack (gafq_State *L) {
  int i, e, n;
  gafqL_checktype(L, 1, GAFQ_TTABLE);
  i = gafqL_optint(L, 2, 1);
  e = gafqL_opt(L, gafqL_checkint, 3, gafqL_getn(L, 1));
  if (i > e) return 0;  /* empty range */
  n = e - i + 1;  /* number of elements */
  if (n <= 0 || !gafq_checkstack(L, n))  /* n <= 0 means arith. overflow */
    return gafqL_error(L, "too many results to unpack");
  gafq_rawgeti(L, 1, i);  /* push arg[i] (avoiding overflow problems) */
  while (i++ < e)  /* push arg[i + 1...e] */
    gafq_rawgeti(L, 1, i);
  return n;
}


static int gafqB_select (gafq_State *L) {
  int n = gafq_gettop(L);
  if (gafq_type(L, 1) == GAFQ_TSTRING && *gafq_tostring(L, 1) == '#') {
    gafq_pushinteger(L, n-1);
    return 1;
  }
  else {
    int i = gafqL_checkint(L, 1);
    if (i < 0) i = n + i;
    else if (i > n) i = n;
    gafqL_argcheck(L, 1 <= i, 1, "index out of range");
    return n - i;
  }
}


static int gafqB_pcall (gafq_State *L) {
  int status;
  gafqL_checkany(L, 1);
  status = gafq_pcall(L, gafq_gettop(L) - 1, GAFQ_MULTRET, 0);
  gafq_pushboolean(L, (status == 0));
  gafq_insert(L, 1);
  return gafq_gettop(L);  /* return status + all results */
}


static int gafqB_xpcall (gafq_State *L) {
  int status;
  gafqL_checkany(L, 2);
  gafq_settop(L, 2);
  gafq_insert(L, 1);  /* put error function under function to be called */
  status = gafq_pcall(L, 0, GAFQ_MULTRET, 1);
  gafq_pushboolean(L, (status == 0));
  gafq_replace(L, 1);
  return gafq_gettop(L);  /* return status + all results */
}


static int gafqB_tostring (gafq_State *L) {
  gafqL_checkany(L, 1);
  if (gafqL_callmeta(L, 1, "__tostring"))  /* is there a metafield? */
    return 1;  /* use its value */
  switch (gafq_type(L, 1)) {
    case GAFQ_TNUMBER:
      gafq_pushstring(L, gafq_tostring(L, 1));
      break;
    case GAFQ_TSTRING:
      gafq_pushvalue(L, 1);
      break;
    case GAFQ_TBOOLEAN:
      gafq_pushstring(L, (gafq_toboolean(L, 1) ? "true" : "false"));
      break;
    case GAFQ_TNIL:
      gafq_pushliteral(L, "nil");
      break;
    default:
      gafq_pushfstring(L, "%s: %p", gafqL_typename(L, 1), gafq_topointer(L, 1));
      break;
  }
  return 1;
}


static int gafqB_newproxy (gafq_State *L) {
  gafq_settop(L, 1);
  gafq_newuserdata(L, 0);  /* create proxy */
  if (gafq_toboolean(L, 1) == 0)
    return 1;  /* no metatable */
  else if (gafq_isboolean(L, 1)) {
    gafq_newtable(L);  /* create a new metatable `m' ... */
    gafq_pushvalue(L, -1);  /* ... and mark `m' as a valid metatable */
    gafq_pushboolean(L, 1);
    gafq_rawset(L, gafq_upvalueindex(1));  /* weaktable[m] = true */
  }
  else {
    int validproxy = 0;  /* to check if weaktable[metatable(u)] == true */
    if (gafq_getmetatable(L, 1)) {
      gafq_rawget(L, gafq_upvalueindex(1));
      validproxy = gafq_toboolean(L, -1);
      gafq_pop(L, 1);  /* remove value */
    }
    gafqL_argcheck(L, validproxy, 1, "boolean or proxy expected");
    gafq_getmetatable(L, 1);  /* metatable is valid; get it */
  }
  gafq_setmetatable(L, 2);
  return 1;
}


static const gafqL_Reg base_funcs[] = {
  {"assert", gafqB_assert},
  {"collectgarbage", gafqB_collectgarbage},
  {"dofile", gafqB_dofile},
  {"error", gafqB_error},
  {"gcinfo", gafqB_gcinfo},
  {"getfenv", gafqB_getfenv},
  {"getmetatable", gafqB_getmetatable},
  {"loadfile", gafqB_loadfile},
  {"load", gafqB_load},
  {"loadstring", gafqB_loadstring},
  {"next", gafqB_next},
  {"pcall", gafqB_pcall},
  {"print", gafqB_print},
  {"rawequal", gafqB_rawequal},
  {"rawget", gafqB_rawget},
  {"rawset", gafqB_rawset},
  {"select", gafqB_select},
  {"setfenv", gafqB_setfenv},
  {"setmetatable", gafqB_setmetatable},
  {"tonumber", gafqB_tonumber},
  {"tostring", gafqB_tostring},
  {"type", gafqB_type},
  {"unpack", gafqB_unpack},
  {"xpcall", gafqB_xpcall},
  {NULL, NULL}
};


/*
** {======================================================
** Coroutine library
** =======================================================
*/

#define CO_RUN	0	/* running */
#define CO_SUS	1	/* suspended */
#define CO_NOR	2	/* 'normal' (it resumed another coroutine) */
#define CO_DEAD	3

static const char *const statnames[] =
    {"running", "suspended", "normal", "dead"};

static int costatus (gafq_State *L, gafq_State *co) {
  if (L == co) return CO_RUN;
  switch (gafq_status(co)) {
    case GAFQ_YIELD:
      return CO_SUS;
    case 0: {
      gafq_Debug ar;
      if (gafq_getstack(co, 0, &ar) > 0)  /* does it have frames? */
        return CO_NOR;  /* it is running */
      else if (gafq_gettop(co) == 0)
          return CO_DEAD;
      else
        return CO_SUS;  /* initial state */
    }
    default:  /* some error occured */
      return CO_DEAD;
  }
}


static int gafqB_costatus (gafq_State *L) {
  gafq_State *co = gafq_tothread(L, 1);
  gafqL_argcheck(L, co, 1, "coroutine expected");
  gafq_pushstring(L, statnames[costatus(L, co)]);
  return 1;
}


static int auxresume (gafq_State *L, gafq_State *co, int narg) {
  int status = costatus(L, co);
  if (!gafq_checkstack(co, narg))
    gafqL_error(L, "too many arguments to resume");
  if (status != CO_SUS) {
    gafq_pushfstring(L, "cannot resume %s coroutine", statnames[status]);
    return -1;  /* error flag */
  }
  gafq_xmove(L, co, narg);
  gafq_setlevel(L, co);
  status = gafq_resume(co, narg);
  if (status == 0 || status == GAFQ_YIELD) {
    int nres = gafq_gettop(co);
    if (!gafq_checkstack(L, nres + 1))
      gafqL_error(L, "too many results to resume");
    gafq_xmove(co, L, nres);  /* move yielded values */
    return nres;
  }
  else {
    gafq_xmove(co, L, 1);  /* move error message */
    return -1;  /* error flag */
  }
}


static int gafqB_coresume (gafq_State *L) {
  gafq_State *co = gafq_tothread(L, 1);
  int r;
  gafqL_argcheck(L, co, 1, "coroutine expected");
  r = auxresume(L, co, gafq_gettop(L) - 1);
  if (r < 0) {
    gafq_pushboolean(L, 0);
    gafq_insert(L, -2);
    return 2;  /* return false + error message */
  }
  else {
    gafq_pushboolean(L, 1);
    gafq_insert(L, -(r + 1));
    return r + 1;  /* return true + `resume' returns */
  }
}


static int gafqB_auxwrap (gafq_State *L) {
  gafq_State *co = gafq_tothread(L, gafq_upvalueindex(1));
  int r = auxresume(L, co, gafq_gettop(L));
  if (r < 0) {
    if (gafq_isstring(L, -1)) {  /* error object is a string? */
      gafqL_where(L, 1);  /* add extra info */
      gafq_insert(L, -2);
      gafq_concat(L, 2);
    }
    gafq_error(L);  /* propagate error */
  }
  return r;
}


static int gafqB_cocreate (gafq_State *L) {
  gafq_State *NL = gafq_newthread(L);
  gafqL_argcheck(L, gafq_isfunction(L, 1) && !gafq_iscfunction(L, 1), 1,
    "Gafq function expected");
  gafq_pushvalue(L, 1);  /* move function to top */
  gafq_xmove(L, NL, 1);  /* move function from L to NL */
  return 1;
}


static int gafqB_cowrap (gafq_State *L) {
  gafqB_cocreate(L);
  gafq_pushcclosure(L, gafqB_auxwrap, 1);
  return 1;
}


static int gafqB_yield (gafq_State *L) {
  return gafq_yield(L, gafq_gettop(L));
}


static int gafqB_corunning (gafq_State *L) {
  if (gafq_pushthread(L))
    gafq_pushnil(L);  /* main thread is not a coroutine */
  return 1;
}


static const gafqL_Reg co_funcs[] = {
  {"create", gafqB_cocreate},
  {"resume", gafqB_coresume},
  {"running", gafqB_corunning},
  {"status", gafqB_costatus},
  {"wrap", gafqB_cowrap},
  {"yield", gafqB_yield},
  {NULL, NULL}
};

/* }====================================================== */


static void auxopen (gafq_State *L, const char *name,
                     gafq_CFunction f, gafq_CFunction u) {
  gafq_pushcfunction(L, u);
  gafq_pushcclosure(L, f, 1);
  gafq_setfield(L, -2, name);
}


static void base_open (gafq_State *L) {
  /* set global _G */
  gafq_pushvalue(L, GAFQ_GLOBALSINDEX);
  gafq_setglobal(L, "_G");
  /* open lib into global table */
  gafqL_register(L, "_G", base_funcs);
  gafq_pushliteral(L, GAFQ_VERSION);
  gafq_setglobal(L, "_VERSION");  /* set global _VERSION */
  /* `ipairs' and `pairs' need auxiliary functions as upvalues */
  auxopen(L, "ipairs", gafqB_ipairs, ipairsaux);
  auxopen(L, "pairs", gafqB_pairs, gafqB_next);
  /* `newproxy' needs a weaktable as upvalue */
  gafq_createtable(L, 0, 1);  /* new table `w' */
  gafq_pushvalue(L, -1);  /* `w' will be its own metatable */
  gafq_setmetatable(L, -2);
  gafq_pushliteral(L, "kv");
  gafq_setfield(L, -2, "__mode");  /* metatable(w).__mode = "kv" */
  gafq_pushcclosure(L, gafqB_newproxy, 1);
  gafq_setglobal(L, "newproxy");  /* set global `newproxy' */
}


GAFQLIB_API int gafqopen_base (gafq_State *L) {
  base_open(L);
  gafqL_register(L, GAFQ_COLIBNAME, co_funcs);
  return 2;
}

