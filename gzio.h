/*
** $Id: gzio.h,v 1.21.1.1 2007/12/27 13:02:25 roberto Exp $
** Buffered streams
** See Copyright Notice in gafq.h
*/


#ifndef gzio_h
#define gzio_h

#include "gafq.h"

#include "gmem.h"


#define EOZ	(-1)			/* end of stream */
// 看着是一个通用io的包装,读取文件
typedef struct Zio ZIO;

#define char2int(c)	cast(int, cast(unsigned char, (c)))

#define zgetc(z)  (((z)->n--)>0 ?  char2int(*(z)->p++) : gafqZ_fill(z))

typedef struct Mbuffer {
  char *buffer;
  size_t n;
  size_t buffsize;
} Mbuffer;

#define gafqZ_initbuffer(L, buff) ((buff)->buffer = NULL, (buff)->buffsize = 0)

#define gafqZ_buffer(buff)	((buff)->buffer)
#define gafqZ_sizebuffer(buff)	((buff)->buffsize)
#define gafqZ_bufflen(buff)	((buff)->n)

#define gafqZ_resetbuffer(buff) ((buff)->n = 0)


#define gafqZ_resizebuffer(L, buff, size) \
	(gafqM_reallocvector(L, (buff)->buffer, (buff)->buffsize, size, char), \
	(buff)->buffsize = size)

#define gafqZ_freebuffer(L, buff)	gafqZ_resizebuffer(L, buff, 0)


GAFQI_FUNC char *gafqZ_openspace (gafq_State *L, Mbuffer *buff, size_t n);
GAFQI_FUNC void gafqZ_init (gafq_State *L, ZIO *z, gafq_Reader reader,
                                        void *data);
GAFQI_FUNC size_t gafqZ_read (ZIO* z, void* b, size_t n);	/* read next n bytes */
GAFQI_FUNC int gafqZ_lookahead (ZIO *z);



/* --------- Private Part ------------------ */

struct Zio {
  size_t n;			/* bytes still unread */
  const char *p;		/* current position in buffer */
  gafq_Reader reader;
  void* data;			/* additional data */
  gafq_State *L;			/* Gafq state (for reader) */
};


GAFQI_FUNC int gafqZ_fill (ZIO *z);

#endif
