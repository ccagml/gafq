/*
** $Id: lzio.c,v 1.31.1.1 2007/12/27 13:02:25 roberto Exp $
** a generic input stream interface
** See Copyright Notice in gafq.h
*/


#include <string.h>

#define lzio_c
#define GAFQ_CORE

#include "gafq.h"

#include "glimits.h"
#include "gmem.h"
#include "gstate.h"
#include "lzio.h"


int gafqZ_fill (ZIO *z) {
  size_t size;
  gafq_State *L = z->L;
  const char *buff;
  gafq_unlock(L);
  buff = z->reader(L, z->data, &size);
  gafq_lock(L);
  if (buff == NULL || size == 0) return EOZ;
  z->n = size - 1;
  z->p = buff;
  return char2int(*(z->p++));
}


int gafqZ_lookahead (ZIO *z) {
  if (z->n == 0) {
    if (gafqZ_fill(z) == EOZ)
      return EOZ;
    else {
      z->n++;  /* gafqZ_fill removed first byte; put back it */
      z->p--;
    }
  }
  return char2int(*z->p);
}


void gafqZ_init (gafq_State *L, ZIO *z, gafq_Reader reader, void *data) {
  z->L = L;
  z->reader = reader;
  z->data = data;
  z->n = 0;
  z->p = NULL;
}


/* --------------------------------------------------------------- read --- */
size_t gafqZ_read (ZIO *z, void *b, size_t n) {
  while (n) {
    size_t m;
    if (gafqZ_lookahead(z) == EOZ)
      return n;  /* return number of missing bytes */
    m = (n <= z->n) ? n : z->n;  /* min. between n and z->n */
    memcpy(b, z->p, m);
    z->n -= m;
    z->p += m;
    b = (char *)b + m;
    n -= m;
  }
  return 0;
}

/* ------------------------------------------------------------------------ */
char *gafqZ_openspace (gafq_State *L, Mbuffer *buff, size_t n) {
  if (n > buff->buffsize) {
    if (n < GAFQ_MINBUFFER) n = GAFQ_MINBUFFER;
    gafqZ_resizebuffer(L, buff, n);
  }
  return buff->buffer;
}


