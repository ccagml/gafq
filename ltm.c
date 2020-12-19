/*
** $Id: ltm.c,v 2.8.1.1 2007/12/27 13:02:25 roberto Exp $
** Tag methods
** See Copyright Notice in gafq.h
*/


#include <string.h>

#define ltm_c
#define GAFQ_CORE

#include "gafq.h"

#include "gobject.h"
#include "gstate.h"
#include "gstring.h"
#include "ltable.h"
#include "ltm.h"



const char *const gafqT_typenames[] = {
  "nil", "boolean", "userdata", "number",
  "string", "table", "function", "userdata", "thread",
  "proto", "upval"
};


void gafqT_init (gafq_State *L) {
  static const char *const gafqT_eventname[] = {  /* ORDER TM */
    "__index", "__newindex",
    "__gc", "__mode", "__eq",
    "__add", "__sub", "__mul", "__div", "__mod",
    "__pow", "__unm", "__len", "__lt", "__le",
    "__concat", "__call"
  };
  int i;
  for (i=0; i<TM_N; i++) {
    G(L)->tmname[i] = gafqS_new(L, gafqT_eventname[i]);
    gafqS_fix(G(L)->tmname[i]);  /* never collect these names */
  }
}


/*
** function to be used with macro "fasttm": optimized for absence of
** tag methods
*/
const TValue *gafqT_gettm (Table *events, TMS event, TString *ename) {
  const TValue *tm = gafqH_getstr(events, ename);
  gafq_assert(event <= TM_EQ);
  if (ttisnil(tm)) {  /* no tag method? */
    events->flags |= cast_byte(1u<<event);  /* cache this fact */
    return NULL;
  }
  else return tm;
}


const TValue *gafqT_gettmbyobj (gafq_State *L, const TValue *o, TMS event) {
  Table *mt;
  switch (ttype(o)) {
    case GAFQ_TTABLE:
      mt = hvalue(o)->metatable;
      break;
    case GAFQ_TUSERDATA:
      mt = uvalue(o)->metatable;
      break;
    default:
      mt = G(L)->mt[ttype(o)];
  }
  return (mt ? gafqH_getstr(mt, G(L)->tmname[event]) : gafqO_nilobject);
}

