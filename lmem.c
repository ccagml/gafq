/*
** $Id: lmem.c,v 1.70.1.1 2007/12/27 13:02:25 roberto Exp $
** Interface to Memory Manager
** See Copyright Notice in gafq.h
*/


#include <stddef.h>

#define lmem_c
#define GAFQ_CORE

#include "gafq.h"

#include "ldebug.h"
#include "ldo.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"



/*
** About the realloc function:
** void * frealloc (void *ud, void *ptr, size_t osize, size_t nsize);
** (`osize' is the old size, `nsize' is the new size)
**
** Gafq ensures that (ptr == NULL) iff (osize == 0).
**
** * frealloc(ud, NULL, 0, x) creates a new block of size `x'
**
** * frealloc(ud, p, x, 0) frees the block `p'
** (in this specific case, frealloc must return NULL).
** particularly, frealloc(ud, NULL, 0, 0) does nothing
** (which is equivalent to free(NULL) in ANSI C)
**
** frealloc returns NULL if it cannot create or reallocate the area
** (any reallocation to an equal or smaller size cannot fail!)
*/



#define MINSIZEARRAY	4


void *gafqM_growaux_ (gafq_State *L, void *block, int *size, size_t size_elems,
                     int limit, const char *errormsg) {
  void *newblock;
  int newsize;
  if (*size >= limit/2) {  /* cannot double it? */
    if (*size >= limit)  /* cannot grow even a little? */
      gafqG_runerror(L, errormsg);
    newsize = limit;  /* still have at least one free place */
  }
  else {
    newsize = (*size)*2;
    if (newsize < MINSIZEARRAY)
      newsize = MINSIZEARRAY;  /* minimum size */
  }
  newblock = gafqM_reallocv(L, block, *size, newsize, size_elems);
  *size = newsize;  /* update only when everything else is OK */
  return newblock;
}


void *gafqM_toobig (gafq_State *L) {
  gafqG_runerror(L, "memory allocation error: block too big");
  return NULL;  /* to avoid warnings */
}



/*
** generic allocation routine.
*/
void *gafqM_realloc_ (gafq_State *L, void *block, size_t osize, size_t nsize) {
  global_State *g = G(L);
  gafq_assert((osize == 0) == (block == NULL));
  block = (*g->frealloc)(g->ud, block, osize, nsize);
  if (block == NULL && nsize > 0)
    gafqD_throw(L, GAFQ_ERRMEM);
  gafq_assert((nsize == 0) == (block == NULL));
  g->totalbytes = (g->totalbytes - osize) + nsize;
  return block;
}

