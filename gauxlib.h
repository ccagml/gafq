/*
** $Id: gauxlib.h,v 1.88.1.1 2007/12/27 13:02:25 roberto Exp $
** Auxiliary functions for building Gafq libraries
** See Copyright Notice in gafq.h
*/


#ifndef gauxlib_h
#define gauxlib_h


#include <stddef.h>
#include <stdio.h>

#include "gafq.h"


#if defined(GAFQ_COMPAT_GETN)
GAFQLIB_API int (gafqL_getn) (gafq_State *L, int t);
GAFQLIB_API void (gafqL_setn) (gafq_State *L, int t, int n);
#else
#define gafqL_getn(L,i)          ((int)gafq_objlen(L, i))
#define gafqL_setn(L,i,j)        ((void)0)  /* no op! */
#endif

#if defined(GAFQ_COMPAT_OPENLIB)
#define gafqI_openlib	gafqL_openlib
#endif


/* extra error code for `gafqL_load' */
#define GAFQ_ERRFILE     (GAFQ_ERRERR+1)


typedef struct gafqL_Reg {
  const char *name;
  gafq_CFunction func;
} gafqL_Reg;



GAFQLIB_API void (gafqI_openlib) (gafq_State *L, const char *libname,
                                const gafqL_Reg *l, int nup);
GAFQLIB_API void (gafqL_register) (gafq_State *L, const char *libname,
                                const gafqL_Reg *l);
GAFQLIB_API int (gafqL_getmetafield) (gafq_State *L, int obj, const char *e);
GAFQLIB_API int (gafqL_callmeta) (gafq_State *L, int obj, const char *e);
GAFQLIB_API int (gafqL_typerror) (gafq_State *L, int narg, const char *tname);
GAFQLIB_API int (gafqL_argerror) (gafq_State *L, int numarg, const char *extramsg);
GAFQLIB_API const char *(gafqL_checkgstring) (gafq_State *L, int numArg,
                                                          size_t *l);
GAFQLIB_API const char *(gafqL_optgstring) (gafq_State *L, int numArg,
                                          const char *def, size_t *l);
GAFQLIB_API gafq_Number (gafqL_checknumber) (gafq_State *L, int numArg);
GAFQLIB_API gafq_Number (gafqL_optnumber) (gafq_State *L, int nArg, gafq_Number def);

GAFQLIB_API gafq_Integer (gafqL_checkinteger) (gafq_State *L, int numArg);
GAFQLIB_API gafq_Integer (gafqL_optinteger) (gafq_State *L, int nArg,
                                          gafq_Integer def);

GAFQLIB_API void (gafqL_checkstack) (gafq_State *L, int sz, const char *msg);
GAFQLIB_API void (gafqL_checktype) (gafq_State *L, int narg, int t);
GAFQLIB_API void (gafqL_checkany) (gafq_State *L, int narg);

GAFQLIB_API int   (gafqL_newmetatable) (gafq_State *L, const char *tname);
GAFQLIB_API void *(gafqL_checkudata) (gafq_State *L, int ud, const char *tname);

GAFQLIB_API void (gafqL_where) (gafq_State *L, int lvl);
GAFQLIB_API int (gafqL_error) (gafq_State *L, const char *fmt, ...);

GAFQLIB_API int (gafqL_checkoption) (gafq_State *L, int narg, const char *def,
                                   const char *const lst[]);

GAFQLIB_API int (gafqL_ref) (gafq_State *L, int t);
GAFQLIB_API void (gafqL_unref) (gafq_State *L, int t, int ref);

GAFQLIB_API int (gafqL_loadfile) (gafq_State *L, const char *filename);
GAFQLIB_API int (gafqL_loadbuffer) (gafq_State *L, const char *buff, size_t sz,
                                  const char *name);
GAFQLIB_API int (gafqL_loadstring) (gafq_State *L, const char *s);

GAFQLIB_API gafq_State *(gafqL_newstate) (void);


GAFQLIB_API const char *(gafqL_gsub) (gafq_State *L, const char *s, const char *p,
                                                  const char *r);

GAFQLIB_API const char *(gafqL_findtable) (gafq_State *L, int idx,
                                         const char *fname, int szhint);




/*
** ===============================================================
** some useful macros
** ===============================================================
*/

#define gafqL_argcheck(L, cond,numarg,extramsg)	\
		((void)((cond) || gafqL_argerror(L, (numarg), (extramsg))))
#define gafqL_checkstring(L,n)	(gafqL_checkgstring(L, (n), NULL))
#define gafqL_optstring(L,n,d)	(gafqL_optgstring(L, (n), (d), NULL))
#define gafqL_checkint(L,n)	((int)gafqL_checkinteger(L, (n)))
#define gafqL_optint(L,n,d)	((int)gafqL_optinteger(L, (n), (d)))
#define gafqL_checklong(L,n)	((long)gafqL_checkinteger(L, (n)))
#define gafqL_optlong(L,n,d)	((long)gafqL_optinteger(L, (n), (d)))

#define gafqL_typename(L,i)	gafq_typename(L, gafq_type(L,(i)))

#define gafqL_dofile(L, fn) \
	(gafqL_loadfile(L, fn) || gafq_pcall(L, 0, GAFQ_MULTRET, 0))

#define gafqL_dostring(L, s) \
	(gafqL_loadstring(L, s) || gafq_pcall(L, 0, GAFQ_MULTRET, 0))

#define gafqL_getmetatable(L,n)	(gafq_getfield(L, GAFQ_REGISTRYINDEX, (n)))

#define gafqL_opt(L,f,n,d)	(gafq_isnoneornil(L,(n)) ? (d) : f(L,(n)))

/*
** {======================================================
** Generic Buffer manipulation
** =======================================================
*/


// 应该是个字节流结构,在gstrlib看到
typedef struct gafqL_Buffer {
  char *p;			/* current position in buffer */
  int lvl;  /* number of strings in the stack (level) */
  gafq_State *L;
  char buffer[GAFQL_BUFFERSIZE];
} gafqL_Buffer;

// 字节流假如一个字符
#define gafqL_addchar(B,c) \
  ((void)((B)->p < ((B)->buffer+GAFQL_BUFFERSIZE) || gafqL_prepbuffer(B)), \
   (*(B)->p++ = (char)(c)))

/* compatibility only */
#define gafqL_putchar(B,c)	gafqL_addchar(B,c)

#define gafqL_addsize(B,n)	((B)->p += (n))

GAFQLIB_API void (gafqL_buffinit) (gafq_State *L, gafqL_Buffer *B);
GAFQLIB_API char *(gafqL_prepbuffer) (gafqL_Buffer *B);
GAFQLIB_API void (gafqL_addgstring) (gafqL_Buffer *B, const char *s, size_t l);
GAFQLIB_API void (gafqL_addstring) (gafqL_Buffer *B, const char *s);
GAFQLIB_API void (gafqL_addvalue) (gafqL_Buffer *B);
GAFQLIB_API void (gafqL_pushresult) (gafqL_Buffer *B);


/* }====================================================== */


/* compatibility with ref system */

/* pre-defined references */
#define GAFQ_NOREF       (-2)
#define GAFQ_REFNIL      (-1)

#define gafq_ref(L,lock) ((lock) ? gafqL_ref(L, GAFQ_REGISTRYINDEX) : \
      (gafq_pushstring(L, "unlocked references are obsolete"), gafq_error(L), 0))

#define gafq_unref(L,ref)        gafqL_unref(L, GAFQ_REGISTRYINDEX, (ref))

#define gafq_getref(L,ref)       gafq_rawgeti(L, GAFQ_REGISTRYINDEX, (ref))


#define gafqL_reg	gafqL_Reg

#endif


