/*
** $Id: ginit.c,v 1.14.1.1 2007/12/27 13:02:25 roberto Exp $
** Initialization of libraries for gafq.c
** See Copyright Notice in gafq.h
*/


#define ginit_c
#define GAFQ_LIB

#include "gafq.h"

#include "gafqlib.h"
#include "gauxlib.h"


static const gafqL_Reg gafqlibs[] = {
  {"", gafqopen_base},
  {GAFQ_LOADLIBNAME, gafqopen_package},
  {GAFQ_TABLIBNAME, gafqopen_table},
  {GAFQ_IOLIBNAME, gafqopen_io},
  {GAFQ_OSLIBNAME, gafqopen_os},
  {GAFQ_STRLIBNAME, gafqopen_string},
  {GAFQ_MATHLIBNAME, gafqopen_math},
  {GAFQ_DBLIBNAME, gafqopen_debug},
  {NULL, NULL}
};


GAFQLIB_API void gafqL_openlibs (gafq_State *L) {
  const gafqL_Reg *lib = gafqlibs;
  for (; lib->func; lib++) {
    gafq_pushcfunction(L, lib->func);
    gafq_pushstring(L, lib->name);
    gafq_call(L, 1, 0);
  }
}

