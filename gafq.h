/*
** $Id: gafq.h,v 1.218.1.7 2012/01/13 20:36:20 roberto Exp $
** Gafq - An Extensible Extension Language
** Gafq.org, PUC-Rio, Brazil (http://www.gafq.org)
** See Copyright Notice at the end of this file
*/


#ifndef gafq_h
#define gafq_h

#include <stdarg.h>
#include <stddef.h>


#include "gafqconf.h"


#define GAFQ_VERSION	"Gafq 5.1"
#define GAFQ_RELEASE	"Gafq 5.1.5"
#define GAFQ_VERSION_NUM	501
#define GAFQ_COPYRIGHT	"Copyright (C) 1994-2012 Gafq.org, PUC-Rio"
#define GAFQ_AUTHORS 	"R. Ierusalimschy, L. H. de Figueiredo & W. Celes"


/* mark for precompiled code (`<esc>Gafq') */
#define	GAFQ_SIGNATURE	"\033Gafq"

/* option for multiple returns in `gafq_pcall' and `gafq_call' */
#define GAFQ_MULTRET	(-1)


/*
** pseudo-indices
*/
#define GAFQ_REGISTRYINDEX	(-10000)
#define GAFQ_ENVIRONINDEX	(-10001)
#define GAFQ_GLOBALSINDEX	(-10002)
#define gafq_upvalueindex(i)	(GAFQ_GLOBALSINDEX-(i))


/* thread status; 0 is OK */
#define GAFQ_YIELD	1
#define GAFQ_ERRRUN	2
#define GAFQ_ERRSYNTAX	3
#define GAFQ_ERRMEM	4
#define GAFQ_ERRERR	5


typedef struct gafq_State gafq_State;

typedef int (*gafq_CFunction) (gafq_State *L);


/*
** functions that read/write blocks when loading/dumping Gafq chunks
*/
typedef const char * (*gafq_Reader) (gafq_State *L, void *ud, size_t *sz);

typedef int (*gafq_Writer) (gafq_State *L, const void* p, size_t sz, void* ud);


/*
** prototype for memory-allocation functions
*/
typedef void * (*gafq_Alloc) (void *ud, void *ptr, size_t osize, size_t nsize);


/*
** basic types
*/
#define GAFQ_TNONE		(-1)

#define GAFQ_TNIL		0
#define GAFQ_TBOOLEAN		1
#define GAFQ_TLIGHTUSERDATA	2
#define GAFQ_TNUMBER		3
#define GAFQ_TSTRING		4
#define GAFQ_TTABLE		5
#define GAFQ_TFUNCTION		6
#define GAFQ_TUSERDATA		7
#define GAFQ_TTHREAD		8



/* minimum Gafq stack available to a C function */
#define GAFQ_MINSTACK	20


/*
** generic extra include file
*/
#if defined(GAFQ_USER_H)
#include GAFQ_USER_H
#endif


/* type of numbers in Gafq */
typedef GAFQ_NUMBER gafq_Number;


/* type for integer functions */
typedef GAFQ_INTEGER gafq_Integer;



/*
** state manipulation
*/
GAFQ_API gafq_State *(gafq_newstate) (gafq_Alloc f, void *ud);
GAFQ_API void       (gafq_close) (gafq_State *L);
GAFQ_API gafq_State *(gafq_newthread) (gafq_State *L);

GAFQ_API gafq_CFunction (gafq_atpanic) (gafq_State *L, gafq_CFunction panicf);


/*
** basic stack manipulation
*/
GAFQ_API int   (gafq_gettop) (gafq_State *L);
GAFQ_API void  (gafq_settop) (gafq_State *L, int idx);
GAFQ_API void  (gafq_pushvalue) (gafq_State *L, int idx);
GAFQ_API void  (gafq_remove) (gafq_State *L, int idx);
GAFQ_API void  (gafq_insert) (gafq_State *L, int idx);
GAFQ_API void  (gafq_replace) (gafq_State *L, int idx);
GAFQ_API int   (gafq_checkstack) (gafq_State *L, int sz);

GAFQ_API void  (gafq_xmove) (gafq_State *from, gafq_State *to, int n);


/*
** access functions (stack -> C)
*/

GAFQ_API int             (gafq_isnumber) (gafq_State *L, int idx);
GAFQ_API int             (gafq_isstring) (gafq_State *L, int idx);
GAFQ_API int             (gafq_iscfunction) (gafq_State *L, int idx);
GAFQ_API int             (gafq_isuserdata) (gafq_State *L, int idx);
GAFQ_API int             (gafq_type) (gafq_State *L, int idx);
GAFQ_API const char     *(gafq_typename) (gafq_State *L, int tp);

GAFQ_API int            (gafq_equal) (gafq_State *L, int idx1, int idx2);
GAFQ_API int            (gafq_rawequal) (gafq_State *L, int idx1, int idx2);
GAFQ_API int            (gafq_lessthan) (gafq_State *L, int idx1, int idx2);

GAFQ_API gafq_Number      (gafq_tonumber) (gafq_State *L, int idx);
GAFQ_API gafq_Integer     (gafq_tointeger) (gafq_State *L, int idx);
GAFQ_API int             (gafq_toboolean) (gafq_State *L, int idx);
GAFQ_API const char     *(gafq_togstring) (gafq_State *L, int idx, size_t *len);
GAFQ_API size_t          (gafq_objlen) (gafq_State *L, int idx);
GAFQ_API gafq_CFunction   (gafq_tocfunction) (gafq_State *L, int idx);
GAFQ_API void	       *(gafq_touserdata) (gafq_State *L, int idx);
GAFQ_API gafq_State      *(gafq_tothread) (gafq_State *L, int idx);
GAFQ_API const void     *(gafq_topointer) (gafq_State *L, int idx);


/*
** push functions (C -> stack)
*/
GAFQ_API void  (gafq_pushnil) (gafq_State *L);
GAFQ_API void  (gafq_pushnumber) (gafq_State *L, gafq_Number n);
GAFQ_API void  (gafq_pushinteger) (gafq_State *L, gafq_Integer n);
GAFQ_API void  (gafq_pushgstring) (gafq_State *L, const char *s, size_t l);
GAFQ_API void  (gafq_pushstring) (gafq_State *L, const char *s);
GAFQ_API const char *(gafq_pushvfstring) (gafq_State *L, const char *fmt,
                                                      va_list argp);
GAFQ_API const char *(gafq_pushfstring) (gafq_State *L, const char *fmt, ...);
GAFQ_API void  (gafq_pushcclosure) (gafq_State *L, gafq_CFunction fn, int n);
GAFQ_API void  (gafq_pushboolean) (gafq_State *L, int b);
GAFQ_API void  (gafq_pushlightuserdata) (gafq_State *L, void *p);
GAFQ_API int   (gafq_pushthread) (gafq_State *L);


/*
** get functions (Gafq -> stack)
*/
GAFQ_API void  (gafq_gettable) (gafq_State *L, int idx);
GAFQ_API void  (gafq_getfield) (gafq_State *L, int idx, const char *k);
GAFQ_API void  (gafq_rawget) (gafq_State *L, int idx);
GAFQ_API void  (gafq_rawgeti) (gafq_State *L, int idx, int n);
GAFQ_API void  (gafq_createtable) (gafq_State *L, int narr, int nrec);
GAFQ_API void *(gafq_newuserdata) (gafq_State *L, size_t sz);
GAFQ_API int   (gafq_getmetatable) (gafq_State *L, int objindex);
GAFQ_API void  (gafq_getfenv) (gafq_State *L, int idx);


/*
** set functions (stack -> Gafq)
*/
GAFQ_API void  (gafq_settable) (gafq_State *L, int idx);
GAFQ_API void  (gafq_setfield) (gafq_State *L, int idx, const char *k);
GAFQ_API void  (gafq_rawset) (gafq_State *L, int idx);
GAFQ_API void  (gafq_rawseti) (gafq_State *L, int idx, int n);
GAFQ_API int   (gafq_setmetatable) (gafq_State *L, int objindex);
GAFQ_API int   (gafq_setfenv) (gafq_State *L, int idx);


/*
** `load' and `call' functions (load and run Gafq code)
*/
GAFQ_API void  (gafq_call) (gafq_State *L, int nargs, int nresults);
GAFQ_API int   (gafq_pcall) (gafq_State *L, int nargs, int nresults, int errfunc);
GAFQ_API int   (gafq_cpcall) (gafq_State *L, gafq_CFunction func, void *ud);
GAFQ_API int   (gafq_load) (gafq_State *L, gafq_Reader reader, void *dt,
                                        const char *chunkname);

GAFQ_API int (gafq_dump) (gafq_State *L, gafq_Writer writer, void *data);


/*
** coroutine functions
*/
GAFQ_API int  (gafq_yield) (gafq_State *L, int nresults);
GAFQ_API int  (gafq_resume) (gafq_State *L, int narg);
GAFQ_API int  (gafq_status) (gafq_State *L);

/*
** garbage-collection function and options
*/

#define GAFQ_GCSTOP		0
#define GAFQ_GCRESTART		1
#define GAFQ_GCCOLLECT		2
#define GAFQ_GCCOUNT		3
#define GAFQ_GCCOUNTB		4
#define GAFQ_GCSTEP		5
#define GAFQ_GCSETPAUSE		6
#define GAFQ_GCSETSTEPMUL	7

GAFQ_API int (gafq_gc) (gafq_State *L, int what, int data);


/*
** miscellaneous functions
*/

GAFQ_API int   (gafq_error) (gafq_State *L);

GAFQ_API int   (gafq_next) (gafq_State *L, int idx);

GAFQ_API void  (gafq_concat) (gafq_State *L, int n);

GAFQ_API gafq_Alloc (gafq_getallocf) (gafq_State *L, void **ud);
GAFQ_API void gafq_setallocf (gafq_State *L, gafq_Alloc f, void *ud);



/* 
** ===============================================================
** some useful macros
** ===============================================================
*/

#define gafq_pop(L,n)		gafq_settop(L, -(n)-1)

#define gafq_newtable(L)		gafq_createtable(L, 0, 0)

#define gafq_register(L,n,f) (gafq_pushcfunction(L, (f)), gafq_setglobal(L, (n)))

#define gafq_pushcfunction(L,f)	gafq_pushcclosure(L, (f), 0)

#define gafq_strlen(L,i)		gafq_objlen(L, (i))

#define gafq_isfunction(L,n)	(gafq_type(L, (n)) == GAFQ_TFUNCTION)
#define gafq_istable(L,n)	(gafq_type(L, (n)) == GAFQ_TTABLE)
#define gafq_islightuserdata(L,n)	(gafq_type(L, (n)) == GAFQ_TLIGHTUSERDATA)
#define gafq_isnil(L,n)		(gafq_type(L, (n)) == GAFQ_TNIL)
#define gafq_isboolean(L,n)	(gafq_type(L, (n)) == GAFQ_TBOOLEAN)
#define gafq_isthread(L,n)	(gafq_type(L, (n)) == GAFQ_TTHREAD)
#define gafq_isnone(L,n)		(gafq_type(L, (n)) == GAFQ_TNONE)
#define gafq_isnoneornil(L, n)	(gafq_type(L, (n)) <= 0)

#define gafq_pushliteral(L, s)	\
	gafq_pushgstring(L, "" s, (sizeof(s)/sizeof(char))-1)

#define gafq_setglobal(L,s)	gafq_setfield(L, GAFQ_GLOBALSINDEX, (s))
#define gafq_getglobal(L,s)	gafq_getfield(L, GAFQ_GLOBALSINDEX, (s))

#define gafq_tostring(L,i)	gafq_togstring(L, (i), NULL)



/*
** compatibility macros and functions
*/

#define gafq_open()	gafqL_newstate()

#define gafq_getregistry(L)	gafq_pushvalue(L, GAFQ_REGISTRYINDEX)

#define gafq_getgccount(L)	gafq_gc(L, GAFQ_GCCOUNT, 0)

#define gafq_Chunkreader		gafq_Reader
#define gafq_Chunkwriter		gafq_Writer


/* hack */
GAFQ_API void gafq_setlevel	(gafq_State *from, gafq_State *to);


/*
** {======================================================================
** Debug API
** =======================================================================
*/


/*
** Event codes
*/
#define GAFQ_HOOKCALL	0
#define GAFQ_HOOKRET	1
#define GAFQ_HOOKLINE	2
#define GAFQ_HOOKCOUNT	3
#define GAFQ_HOOKTAILRET 4


/*
** Event masks
*/
#define GAFQ_MASKCALL	(1 << GAFQ_HOOKCALL)
#define GAFQ_MASKRET	(1 << GAFQ_HOOKRET)
#define GAFQ_MASKLINE	(1 << GAFQ_HOOKLINE)
#define GAFQ_MASKCOUNT	(1 << GAFQ_HOOKCOUNT)

typedef struct gafq_Debug gafq_Debug;  /* activation record */


/* Functions to be called by the debuger in specific events */
typedef void (*gafq_Hook) (gafq_State *L, gafq_Debug *ar);


GAFQ_API int gafq_getstack (gafq_State *L, int level, gafq_Debug *ar);
GAFQ_API int gafq_getinfo (gafq_State *L, const char *what, gafq_Debug *ar);
GAFQ_API const char *gafq_getlocal (gafq_State *L, const gafq_Debug *ar, int n);
GAFQ_API const char *gafq_setlocal (gafq_State *L, const gafq_Debug *ar, int n);
GAFQ_API const char *gafq_getupvalue (gafq_State *L, int funcindex, int n);
GAFQ_API const char *gafq_setupvalue (gafq_State *L, int funcindex, int n);

GAFQ_API int gafq_sethook (gafq_State *L, gafq_Hook func, int mask, int count);
GAFQ_API gafq_Hook gafq_gethook (gafq_State *L);
GAFQ_API int gafq_gethookmask (gafq_State *L);
GAFQ_API int gafq_gethookcount (gafq_State *L);


struct gafq_Debug {
  int event;
  const char *name;	/* (n) */
  const char *namewhat;	/* (n) `global', `local', `field', `method' */
  const char *what;	/* (S) `Gafq', `C', `main', `tail' */
  const char *source;	/* (S) */
  int currentline;	/* (l) */
  int nups;		/* (u) number of upvalues */
  int linedefined;	/* (S) */
  int lastlinedefined;	/* (S) */
  char short_src[GAFQ_IDSIZE]; /* (S) */
  /* private part */
  int i_ci;  /* active function */
};

/* }====================================================================== */


/******************************************************************************
* Copyright (C) 1994-2012 Gafq.org, PUC-Rio.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/


#endif
