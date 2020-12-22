/*
** $Id: gmem.h,v 1.31.1.1 2007/12/27 13:02:25 roberto Exp $
** Interface to Memory Manager
** See Copyright Notice in gafq.h
*/

#ifndef gmem_h
#define gmem_h


#include <stddef.h>

#include "glimits.h"
#include "gafq.h"

#define MEMERRMSG	"not enough memory"


#define gafqM_reallocv(L,b,on,n,e) \
	((cast(size_t, (n)+1) <= MAX_SIZET/(e)) ?  /* +1 to avoid warnings */ \
		gafqM_realloc_(L, (b), (on)*(e), (n)*(e)) : \
		gafqM_toobig(L))

#define gafqM_freemem(L, b, s)	gafqM_realloc_(L, (b), (s), 0)
#define gafqM_free(L, b)		gafqM_realloc_(L, (b), sizeof(*(b)), 0)
#define gafqM_freearray(L, b, n, t)   gafqM_reallocv(L, (b), n, 0, sizeof(t))

#define gafqM_malloc(L,t)	gafqM_realloc_(L, NULL, 0, (t))
#define gafqM_new(L,t)		cast(t *, gafqM_malloc(L, sizeof(t)))
#define gafqM_newvector(L,n,t) \
		cast(t *, gafqM_reallocv(L, NULL, 0, n, sizeof(t)))

#define gafqM_growvector(L,v,nelems,size,t,limit,e) \
          if ((nelems)+1 > (size)) \
            ((v)=cast(t *, gafqM_growaux_(L,v,&(size),sizeof(t),limit,e)))

#define gafqM_reallocvector(L, v,oldn,n,t) \
   ((v)=cast(t *, gafqM_reallocv(L, v, oldn, n, sizeof(t))))


GAFQI_FUNC void *gafqM_realloc_ (gafq_State *L, void *block, size_t oldsize,
                                                          size_t size);
GAFQI_FUNC void *gafqM_toobig (gafq_State *L);
GAFQI_FUNC void *gafqM_growaux_ (gafq_State *L, void *block, int *size,
                               size_t size_elem, int limit,
                               const char *errormsg);

#endif

