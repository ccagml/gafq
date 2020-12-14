/*
** $Id: gafqlib.h,v 1.36.1.1 2007/12/27 13:02:25 roberto Exp $
** Gafq standard libraries
** See Copyright Notice in gafq.h
*/


#ifndef gafqlib_h
#define gafqlib_h

#include "gafq.h"


/* Key to file-handle type */
#define GAFQ_FILEHANDLE		"FILE*"


#define GAFQ_COLIBNAME	"coroutine"
GAFQLIB_API int (gafqopen_base) (gafq_State *L);

#define GAFQ_TABLIBNAME	"table"
GAFQLIB_API int (gafqopen_table) (gafq_State *L);

#define GAFQ_IOLIBNAME	"io"
GAFQLIB_API int (gafqopen_io) (gafq_State *L);

#define GAFQ_OSLIBNAME	"os"
GAFQLIB_API int (gafqopen_os) (gafq_State *L);

#define GAFQ_STRLIBNAME	"string"
GAFQLIB_API int (gafqopen_string) (gafq_State *L);

#define GAFQ_MATHLIBNAME	"math"
GAFQLIB_API int (gafqopen_math) (gafq_State *L);

#define GAFQ_DBLIBNAME	"debug"
GAFQLIB_API int (gafqopen_debug) (gafq_State *L);

#define GAFQ_LOADLIBNAME	"package"
GAFQLIB_API int (gafqopen_package) (gafq_State *L);


/* open all previous libraries */
GAFQLIB_API void (gafqL_openlibs) (gafq_State *L); 



#ifndef gafq_assert
#define gafq_assert(x)	((void)0)
#endif


#endif
