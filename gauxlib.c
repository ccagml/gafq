/*
** $Id: gauxlib.c,v 1.159.1.3 2008/01/21 13:20:51 roberto Exp $
** Auxiliary functions for building Gafq libraries
** See Copyright Notice in gafq.h
*/


#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* This file uses only the official API of Gafq.
** Any function declared here could be written as an application function.
*/

#define gauxlib_c
#define GAFQ_LIB

#include "gafq.h"

#include "gauxlib.h"


#define FREELIST_REF	0	/* free list of references */


/* convert a stack index to positive */
#define abs_index(L, i)		((i) > 0 || (i) <= GAFQ_REGISTRYINDEX ? (i) : \
					gafq_gettop(L) + (i) + 1)


/*
** {======================================================
** Error-report functions
** =======================================================
*/


GAFQLIB_API int gafqL_argerror (gafq_State *L, int narg, const char *extramsg) {
  gafq_Debug ar;
  if (!gafq_getstack(L, 0, &ar))  /* no stack frame? */
    return gafqL_error(L, "bad argument #%d (%s)", narg, extramsg);
  gafq_getinfo(L, "n", &ar);
  if (strcmp(ar.namewhat, "method") == 0) {
    narg--;  /* do not count `self' */
    if (narg == 0)  /* error is in the self argument itself? */
      return gafqL_error(L, "calling " GAFQ_QS " on bad self (%s)",
                           ar.name, extramsg);
  }
  if (ar.name == NULL)
    ar.name = "?";
  return gafqL_error(L, "bad argument #%d to " GAFQ_QS " (%s)",
                        narg, ar.name, extramsg);
}


GAFQLIB_API int gafqL_typerror (gafq_State *L, int narg, const char *tname) {
  const char *msg = gafq_pushfstring(L, "%s expected, got %s",
                                    tname, gafqL_typename(L, narg));
  return gafqL_argerror(L, narg, msg);
}


static void tag_error (gafq_State *L, int narg, int tag) {
  gafqL_typerror(L, narg, gafq_typename(L, tag));
}


GAFQLIB_API void gafqL_where (gafq_State *L, int level) {
  gafq_Debug ar;
  if (gafq_getstack(L, level, &ar)) {  /* check function at level */
    gafq_getinfo(L, "Sl", &ar);  /* get info about it */
    if (ar.currentline > 0) {  /* is there info? */
      gafq_pushfstring(L, "%s:%d: ", ar.short_src, ar.currentline);
      return;
    }
  }
  gafq_pushliteral(L, "");  /* else, no information available... */
}


GAFQLIB_API int gafqL_error (gafq_State *L, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  gafqL_where(L, 1);
  gafq_pushvfstring(L, fmt, argp);
  va_end(argp);
  gafq_concat(L, 2);
  return gafq_error(L);
}

/* }====================================================== */


GAFQLIB_API int gafqL_checkoption (gafq_State *L, int narg, const char *def,
                                 const char *const lst[]) {
  const char *name = (def) ? gafqL_optstring(L, narg, def) :
                             gafqL_checkstring(L, narg);
  int i;
  for (i=0; lst[i]; i++)
    if (strcmp(lst[i], name) == 0)
      return i;
  return gafqL_argerror(L, narg,
                       gafq_pushfstring(L, "invalid option " GAFQ_QS, name));
}


GAFQLIB_API int gafqL_newmetatable (gafq_State *L, const char *tname) {
  gafq_getfield(L, GAFQ_REGISTRYINDEX, tname);  /* get registry.name */
  if (!gafq_isnil(L, -1))  /* name already in use? */
    return 0;  /* leave previous value on top, but return 0 */
  gafq_pop(L, 1);
  gafq_newtable(L);  /* create metatable */
  gafq_pushvalue(L, -1);
  gafq_setfield(L, GAFQ_REGISTRYINDEX, tname);  /* registry.name = metatable */
  return 1;
}


GAFQLIB_API void *gafqL_checkudata (gafq_State *L, int ud, const char *tname) {
  void *p = gafq_touserdata(L, ud);
  if (p != NULL) {  /* value is a userdata? */
    if (gafq_getmetatable(L, ud)) {  /* does it have a metatable? */
      gafq_getfield(L, GAFQ_REGISTRYINDEX, tname);  /* get correct metatable */
      if (gafq_rawequal(L, -1, -2)) {  /* does it have the correct mt? */
        gafq_pop(L, 2);  /* remove both metatables */
        return p;
      }
    }
  }
  gafqL_typerror(L, ud, tname);  /* else error */
  return NULL;  /* to avoid warnings */
}


GAFQLIB_API void gafqL_checkstack (gafq_State *L, int space, const char *mes) {
  if (!gafq_checkstack(L, space))
    gafqL_error(L, "stack overflow (%s)", mes);
}

//检查类型是不是最后一个参数
GAFQLIB_API void gafqL_checktype (gafq_State *L, int narg, int t) {
  if (gafq_type(L, narg) != t)
    tag_error(L, narg, t);
}


GAFQLIB_API void gafqL_checkany (gafq_State *L, int narg) {
  if (gafq_type(L, narg) == GAFQ_TNONE)
    gafqL_argerror(L, narg, "value expected");
}


GAFQLIB_API const char *gafqL_checkgstring (gafq_State *L, int narg, size_t *len) {
  const char *s = gafq_togstring(L, narg, len);
  if (!s) tag_error(L, narg, GAFQ_TSTRING);
  return s;
}


GAFQLIB_API const char *gafqL_optgstring (gafq_State *L, int narg,
                                        const char *def, size_t *len) {
  if (gafq_isnoneornil(L, narg)) {
    if (len)
      *len = (def ? strlen(def) : 0);
    return def;
  }
  else return gafqL_checkgstring(L, narg, len);
}

//检查数字
GAFQLIB_API gafq_Number gafqL_checknumber (gafq_State *L, int narg) {
  gafq_Number d = gafq_tonumber(L, narg);
  if (d == 0 && !gafq_isnumber(L, narg))  /* avoid extra test when d is not 0 */
    tag_error(L, narg, GAFQ_TNUMBER); // 输出类型错误的打印
  return d;
}


GAFQLIB_API gafq_Number gafqL_optnumber (gafq_State *L, int narg, gafq_Number def) {
  return gafqL_opt(L, gafqL_checknumber, narg, def);
}


GAFQLIB_API gafq_Integer gafqL_checkinteger (gafq_State *L, int narg) {
  gafq_Integer d = gafq_tointeger(L, narg);
  if (d == 0 && !gafq_isnumber(L, narg))  /* avoid extra test when d is not 0 */
    tag_error(L, narg, GAFQ_TNUMBER);
  return d;
}


GAFQLIB_API gafq_Integer gafqL_optinteger (gafq_State *L, int narg,
                                                      gafq_Integer def) {
  return gafqL_opt(L, gafqL_checkinteger, narg, def);
}


GAFQLIB_API int gafqL_getmetafield (gafq_State *L, int obj, const char *event) {
  if (!gafq_getmetatable(L, obj))  /* no metatable? */
    return 0;
  gafq_pushstring(L, event);
  gafq_rawget(L, -2);
  if (gafq_isnil(L, -1)) {
    gafq_pop(L, 2);  /* remove metatable and metafield */
    return 0;
  }
  else {
    gafq_remove(L, -2);  /* remove only metatable */
    return 1;
  }
}


GAFQLIB_API int gafqL_callmeta (gafq_State *L, int obj, const char *event) {
  obj = abs_index(L, obj);
  if (!gafqL_getmetafield(L, obj, event))  /* no metafield? */
    return 0;
  gafq_pushvalue(L, obj);
  gafq_call(L, 1, 1);
  return 1;
}


GAFQLIB_API void (gafqL_register) (gafq_State *L, const char *libname,
                                const gafqL_Reg *l) {
  gafqI_openlib(L, libname, l, 0);
}


static int libsize (const gafqL_Reg *l) {
  int size = 0;
  for (; l->name; l++) size++;
  return size;
}


GAFQLIB_API void gafqI_openlib (gafq_State *L, const char *libname,
                              const gafqL_Reg *l, int nup) {
  if (libname) {
    int size = libsize(l);
    /* check whether lib already exists */
    gafqL_findtable(L, GAFQ_REGISTRYINDEX, "_LOADED", 1);
    gafq_getfield(L, -1, libname);  /* get _LOADED[libname] */
    if (!gafq_istable(L, -1)) {  /* not found? */
      gafq_pop(L, 1);  /* remove previous result */
      /* try global variable (and create one if it does not exist) */
      if (gafqL_findtable(L, GAFQ_GLOBALSINDEX, libname, size) != NULL)
        gafqL_error(L, "name conflict for module " GAFQ_QS, libname);
      gafq_pushvalue(L, -1);
      gafq_setfield(L, -3, libname);  /* _LOADED[libname] = new table */
    }
    gafq_remove(L, -2);  /* remove _LOADED table */
    gafq_insert(L, -(nup+1));  /* move library table to below upvalues */
  }
  for (; l->name; l++) {
    int i;
    for (i=0; i<nup; i++)  /* copy upvalues to the top */
      gafq_pushvalue(L, -nup);
    gafq_pushcclosure(L, l->func, nup);
    gafq_setfield(L, -(nup+2), l->name);
  }
  gafq_pop(L, nup);  /* remove upvalues */
}



/*
** {======================================================
** getn-setn: size for arrays
** =======================================================
*/

#if defined(GAFQ_COMPAT_GETN)

static int checkint (gafq_State *L, int topop) {
  int n = (gafq_type(L, -1) == GAFQ_TNUMBER) ? gafq_tointeger(L, -1) : -1;
  gafq_pop(L, topop);
  return n;
}


static void getsizes (gafq_State *L) {
  gafq_getfield(L, GAFQ_REGISTRYINDEX, "GAFQ_SIZES");
  if (gafq_isnil(L, -1)) {  /* no `size' table? */
    gafq_pop(L, 1);  /* remove nil */
    gafq_newtable(L);  /* create it */
    gafq_pushvalue(L, -1);  /* `size' will be its own metatable */
    gafq_setmetatable(L, -2);
    gafq_pushliteral(L, "kv");
    gafq_setfield(L, -2, "__mode");  /* metatable(N).__mode = "kv" */
    gafq_pushvalue(L, -1);
    gafq_setfield(L, GAFQ_REGISTRYINDEX, "GAFQ_SIZES");  /* store in register */
  }
}


GAFQLIB_API void gafqL_setn (gafq_State *L, int t, int n) {
  t = abs_index(L, t);
  gafq_pushliteral(L, "n");
  gafq_rawget(L, t);
  if (checkint(L, 1) >= 0) {  /* is there a numeric field `n'? */
    gafq_pushliteral(L, "n");  /* use it */
    gafq_pushinteger(L, n);
    gafq_rawset(L, t);
  }
  else {  /* use `sizes' */
    getsizes(L);
    gafq_pushvalue(L, t);
    gafq_pushinteger(L, n);
    gafq_rawset(L, -3);  /* sizes[t] = n */
    gafq_pop(L, 1);  /* remove `sizes' */
  }
}


GAFQLIB_API int gafqL_getn (gafq_State *L, int t) {
  int n;
  t = abs_index(L, t);
  gafq_pushliteral(L, "n");  /* try t.n */
  gafq_rawget(L, t);
  if ((n = checkint(L, 1)) >= 0) return n;
  getsizes(L);  /* else try sizes[t] */
  gafq_pushvalue(L, t);
  gafq_rawget(L, -2);
  if ((n = checkint(L, 2)) >= 0) return n;
  return (int)gafq_objlen(L, t);
}

#endif

/* }====================================================== */



GAFQLIB_API const char *gafqL_gsub (gafq_State *L, const char *s, const char *p,
                                                               const char *r) {
  const char *wild;
  size_t l = strlen(p);
  gafqL_Buffer b;
  gafqL_buffinit(L, &b);
  while ((wild = strstr(s, p)) != NULL) {
    gafqL_addgstring(&b, s, wild - s);  /* push prefix */
    gafqL_addstring(&b, r);  /* push replacement in place of pattern */
    s = wild + l;  /* continue after `p' */
  }
  gafqL_addstring(&b, s);  /* push last suffix */
  gafqL_pushresult(&b);
  return gafq_tostring(L, -1);
}


GAFQLIB_API const char *gafqL_findtable (gafq_State *L, int idx,
                                       const char *fname, int szhint) {
  const char *e;
  gafq_pushvalue(L, idx);
  do {
    e = strchr(fname, '.');
    if (e == NULL) e = fname + strlen(fname);
    gafq_pushgstring(L, fname, e - fname);
    gafq_rawget(L, -2);
    if (gafq_isnil(L, -1)) {  /* no such field? */
      gafq_pop(L, 1);  /* remove this nil */
      gafq_createtable(L, 0, (*e == '.' ? 1 : szhint)); /* new table for field */
      gafq_pushgstring(L, fname, e - fname);
      gafq_pushvalue(L, -2);
      gafq_settable(L, -4);  /* set new table into field */
    }
    else if (!gafq_istable(L, -1)) {  /* field has a non-table value? */
      gafq_pop(L, 2);  /* remove table and value */
      return fname;  /* return problematic part of the name */
    }
    gafq_remove(L, -2);  /* remove previous table */
    fname = e + 1;
  } while (*e == '.');
  return NULL;
}



/*
** {======================================================
** Generic Buffer manipulation
** =======================================================
*/


#define bufflen(B)	((B)->p - (B)->buffer)
#define bufffree(B)	((size_t)(GAFQL_BUFFERSIZE - bufflen(B)))

#define LIMIT	(GAFQ_MINSTACK/2)

//清空字节流，把字节流放入状态里
static int emptybuffer (gafqL_Buffer *B) {
  size_t l = bufflen(B);
  if (l == 0) return 0;  /* put nothing on stack */
  else {
    gafq_pushgstring(B->L, B->buffer, l);
    B->p = B->buffer;
    B->lvl++;
    return 1;
  }
}


static void adjuststack (gafqL_Buffer *B) {
  if (B->lvl > 1) {
    gafq_State *L = B->L;
    int toget = 1;  /* number of levels to concat */
    size_t toplen = gafq_strlen(L, -1);
    do {
      size_t l = gafq_strlen(L, -(toget+1));
      if (B->lvl - toget + 1 >= LIMIT || toplen > l) {
        toplen += l;
        toget++;
      }
      else break;
    } while (toget < B->lvl);
    gafq_concat(L, toget);
    B->lvl = B->lvl - toget + 1;
  }
}


GAFQLIB_API char *gafqL_prepbuffer (gafqL_Buffer *B) {
  if (emptybuffer(B))
    adjuststack(B);
  return B->buffer;
}


GAFQLIB_API void gafqL_addgstring (gafqL_Buffer *B, const char *s, size_t l) {
  while (l--)
    gafqL_addchar(B, *s++);
}


GAFQLIB_API void gafqL_addstring (gafqL_Buffer *B, const char *s) {
  gafqL_addgstring(B, s, strlen(s));
}

//好像是把字节流放回状态里
GAFQLIB_API void gafqL_pushresult (gafqL_Buffer *B) {
  emptybuffer(B);
  gafq_concat(B->L, B->lvl);
  B->lvl = 1;
}


GAFQLIB_API void gafqL_addvalue (gafqL_Buffer *B) {
  gafq_State *L = B->L;
  size_t vl;
  const char *s = gafq_togstring(L, -1, &vl);
  if (vl <= bufffree(B)) {  /* fit into buffer? */
    memcpy(B->p, s, vl);  /* put it there */
    B->p += vl;
    gafq_pop(L, 1);  /* remove from stack */
  }
  else {
    if (emptybuffer(B))
      gafq_insert(L, -2);  /* put buffer before new value */
    B->lvl++;  /* add new value into B stack */
    adjuststack(B);
  }
}


GAFQLIB_API void gafqL_buffinit (gafq_State *L, gafqL_Buffer *B) {
  B->L = L;
  B->p = B->buffer;
  B->lvl = 0;
}

/* }====================================================== */


GAFQLIB_API int gafqL_ref (gafq_State *L, int t) {
  int ref;
  t = abs_index(L, t);
  if (gafq_isnil(L, -1)) {
    gafq_pop(L, 1);  /* remove from stack */
    return GAFQ_REFNIL;  /* `nil' has a unique fixed reference */
  }
  gafq_rawgeti(L, t, FREELIST_REF);  /* get first free element */
  ref = (int)gafq_tointeger(L, -1);  /* ref = t[FREELIST_REF] */
  gafq_pop(L, 1);  /* remove it from stack */
  if (ref != 0) {  /* any free element? */
    gafq_rawgeti(L, t, ref);  /* remove it from list */
    gafq_rawseti(L, t, FREELIST_REF);  /* (t[FREELIST_REF] = t[ref]) */
  }
  else {  /* no free elements */
    ref = (int)gafq_objlen(L, t);
    ref++;  /* create new reference */
  }
  gafq_rawseti(L, t, ref);
  return ref;
}


GAFQLIB_API void gafqL_unref (gafq_State *L, int t, int ref) {
  if (ref >= 0) {
    t = abs_index(L, t);
    gafq_rawgeti(L, t, FREELIST_REF);
    gafq_rawseti(L, t, ref);  /* t[ref] = t[FREELIST_REF] */
    gafq_pushinteger(L, ref);
    gafq_rawseti(L, t, FREELIST_REF);  /* t[FREELIST_REF] = ref */
  }
}



/*
** {======================================================
** Load functions
** =======================================================
*/

typedef struct LoadF {
  int extraline;
  FILE *f;
  char buff[GAFQL_BUFFERSIZE];
} LoadF;


static const char *getF (gafq_State *L, void *ud, size_t *size) {
  LoadF *lf = (LoadF *)ud;
  (void)L;
  if (lf->extraline) {
    lf->extraline = 0;
    *size = 1;
    return "\n";
  }
  if (feof(lf->f)) return NULL;
  *size = fread(lf->buff, 1, sizeof(lf->buff), lf->f);
  return (*size > 0) ? lf->buff : NULL;
}


static int errfile (gafq_State *L, const char *what, int fnameindex) {
  const char *serr = strerror(errno);
  const char *filename = gafq_tostring(L, fnameindex) + 1;
  gafq_pushfstring(L, "cannot %s %s: %s", what, filename, serr);
  gafq_remove(L, fnameindex);
  return GAFQ_ERRFILE;
}


GAFQLIB_API int gafqL_loadfile (gafq_State *L, const char *filename) {
  LoadF lf;
  int status, readstatus;
  int c;
  int fnameindex = gafq_gettop(L) + 1;  /* index of filename on the stack */
  lf.extraline = 0;
  if (filename == NULL) {
    gafq_pushliteral(L, "=stdin");
    lf.f = stdin;
  }
  else {
    gafq_pushfstring(L, "@%s", filename);
    lf.f = fopen(filename, "r");
    if (lf.f == NULL) return errfile(L, "open", fnameindex);
  }
  c = getc(lf.f);
  if (c == '#') {  /* Unix exec. file? */
    lf.extraline = 1;
    while ((c = getc(lf.f)) != EOF && c != '\n') ;  /* skip first line */
    if (c == '\n') c = getc(lf.f);
  }
  if (c == GAFQ_SIGNATURE[0] && filename) {  /* binary file? */
    lf.f = freopen(filename, "rb", lf.f);  /* reopen in binary mode */
    if (lf.f == NULL) return errfile(L, "reopen", fnameindex);
    /* skip eventual `#!...' */
   while ((c = getc(lf.f)) != EOF && c != GAFQ_SIGNATURE[0]) ;
    lf.extraline = 0;
  }
  ungetc(c, lf.f);
  status = gafq_load(L, getF, &lf, gafq_tostring(L, -1));
  readstatus = ferror(lf.f);
  if (filename) fclose(lf.f);  /* close file (even in case of errors) */
  if (readstatus) {
    gafq_settop(L, fnameindex);  /* ignore results from `gafq_load' */
    return errfile(L, "read", fnameindex);
  }
  gafq_remove(L, fnameindex);
  return status;
}


typedef struct LoadS {
  const char *s;
  size_t size;
} LoadS;


static const char *getS (gafq_State *L, void *ud, size_t *size) {
  LoadS *ls = (LoadS *)ud;
  (void)L;
  if (ls->size == 0) return NULL;
  *size = ls->size;
  ls->size = 0;
  return ls->s;
}


GAFQLIB_API int gafqL_loadbuffer (gafq_State *L, const char *buff, size_t size,
                                const char *name) {
  LoadS ls;
  ls.s = buff;
  ls.size = size;
  return gafq_load(L, getS, &ls, name);
}


GAFQLIB_API int (gafqL_loadstring) (gafq_State *L, const char *s) {
  return gafqL_loadbuffer(L, s, strlen(s), s);
}



/* }====================================================== */


static void *l_alloc (void *ud, void *ptr, size_t osize, size_t nsize) {
  (void)ud;
  (void)osize;
  if (nsize == 0) {
    free(ptr);
    return NULL;
  }
  else
    return realloc(ptr, nsize);
}


static int panic (gafq_State *L) {
  (void)L;  /* to avoid warnings */
  fprintf(stderr, "PANIC: unprotected error in call to Gafq API (%s)\n",
                   gafq_tostring(L, -1));
  return 0;
}


GAFQLIB_API gafq_State *gafqL_newstate (void) {
  gafq_State *L = gafq_newstate(l_alloc, NULL);
  if (L) gafq_atpanic(L, &panic);
  return L;
}

