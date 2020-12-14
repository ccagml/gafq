/*
** $Id: llimits.h,v 1.69.1.1 2007/12/27 13:02:25 roberto Exp $
** Limits, basic types, and some other `installation-dependent' definitions
** See Copyright Notice in gafq.h
*/

#ifndef llimits_h
#define llimits_h


#include <limits.h>
#include <stddef.h>


#include "gafq.h"


typedef GAFQI_UINT32 lu_int32;

typedef GAFQI_UMEM lu_mem;

typedef GAFQI_MEM l_mem;



/* chars used as small naturals (so that `char' is reserved for characters) */
typedef unsigned char lu_byte;


#define MAX_SIZET	((size_t)(~(size_t)0)-2)

#define MAX_LUMEM	((lu_mem)(~(lu_mem)0)-2)


#define MAX_INT (INT_MAX-2)  /* maximum value of an int (-2 for safety) */

/*
** conversion of pointer to integer
** this is for hashing only; there is no problem if the integer
** cannot hold the whole pointer value
*/
#define IntPoint(p)  ((unsigned int)(lu_mem)(p))



/* type to ensure maximum alignment */
typedef GAFQI_USER_ALIGNMENT_T L_Umaxalign;


/* result of a `usual argument conversion' over gafq_Number */
typedef GAFQI_UACNUMBER l_uacNumber;


/* internal assertions for in-house debugging */
#ifdef gafq_assert

#define check_exp(c,e)		(gafq_assert(c), (e))
#define api_check(l,e)		gafq_assert(e)

#else

#define gafq_assert(c)		((void)0)
#define check_exp(c,e)		(e)
#define api_check		gafqi_apicheck

#endif


#ifndef UNUSED
#define UNUSED(x)	((void)(x))	/* to avoid warnings */
#endif


#ifndef cast
#define cast(t, exp)	((t)(exp))
#endif

#define cast_byte(i)	cast(lu_byte, (i))
#define cast_num(i)	cast(gafq_Number, (i))
#define cast_int(i)	cast(int, (i))



/*
** type for virtual-machine instructions
** must be an unsigned with (at least) 4 bytes (see details in lopcodes.h)
*/
typedef lu_int32 Instruction;



/* maximum stack for a Gafq function */
#define MAXSTACK	250



/* minimum size for the string table (must be power of 2) */
#ifndef MINSTRTABSIZE
#define MINSTRTABSIZE	32
#endif


/* minimum size for string buffer */
#ifndef GAFQ_MINBUFFER
#define GAFQ_MINBUFFER	32
#endif


#ifndef gafq_lock
#define gafq_lock(L)     ((void) 0) 
#define gafq_unlock(L)   ((void) 0)
#endif

#ifndef gafqi_threadyield
#define gafqi_threadyield(L)     {gafq_unlock(L); gafq_lock(L);}
#endif


/*
** macro to control inclusion of some hard tests on stack reallocation
*/ 
#ifndef HARDSTACKTESTS
#define condhardstacktests(x)	((void)0)
#else
#define condhardstacktests(x)	x
#endif

#endif
