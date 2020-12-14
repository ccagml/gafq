/*
** $Id: lundump.h,v 1.37.1.1 2007/12/27 13:02:25 roberto Exp $
** load precompiled Gafq chunks
** See Copyright Notice in gafq.h
*/

#ifndef lundump_h
#define lundump_h

#include "lobject.h"
#include "lzio.h"

/* load one chunk; from lundump.c */
GAFQI_FUNC Proto* gafqU_undump (gafq_State* L, ZIO* Z, Mbuffer* buff, const char* name);

/* make header; from lundump.c */
GAFQI_FUNC void gafqU_header (char* h);

/* dump one chunk; from ldump.c */
GAFQI_FUNC int gafqU_dump (gafq_State* L, const Proto* f, gafq_Writer w, void* data, int strip);

#ifdef gafqc_c
/* print one chunk; from print.c */
GAFQI_FUNC void gafqU_print (const Proto* f, int full);
#endif

/* for header of binary files -- this is Gafq 5.1 */
#define GAFQC_VERSION		0x51

/* for header of binary files -- this is the official format */
#define GAFQC_FORMAT		0

/* size of header of binary files */
#define GAFQC_HEADERSIZE		12

#endif
