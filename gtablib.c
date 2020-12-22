/*
** $Id: gtablib.c,v 1.38.1.3 2008/02/14 16:46:58 roberto Exp $
** Library for Table Manipulation
** See Copyright Notice in gafq.h
*/


#include <stddef.h>

#define gtablib_c
#define GAFQ_LIB

#include "gafq.h"

#include "gauxlib.h"
#include "gafqlib.h"


#define aux_getn(L,n)	(gafqL_checktype(L, n, GAFQ_TTABLE), gafqL_getn(L, n))


static int foreachi (gafq_State *L) {
  int i;
  int n = aux_getn(L, 1);
  gafqL_checktype(L, 2, GAFQ_TFUNCTION);
  for (i=1; i <= n; i++) {
    gafq_pushvalue(L, 2);  /* function */
    gafq_pushinteger(L, i);  /* 1st argument */
    gafq_rawgeti(L, 1, i);  /* 2nd argument */
    gafq_call(L, 2, 1);
    if (!gafq_isnil(L, -1))
      return 1;
    gafq_pop(L, 1);  /* remove nil result */
  }
  return 0;
}


static int foreach (gafq_State *L) {
  gafqL_checktype(L, 1, GAFQ_TTABLE);
  gafqL_checktype(L, 2, GAFQ_TFUNCTION);
  gafq_pushnil(L);  /* first key */
  while (gafq_next(L, 1)) {
    gafq_pushvalue(L, 2);  /* function */
    gafq_pushvalue(L, -3);  /* key */
    gafq_pushvalue(L, -3);  /* value */
    gafq_call(L, 2, 1);
    if (!gafq_isnil(L, -1))
      return 1;
    gafq_pop(L, 2);  /* remove value and result */
  }
  return 0;
}


static int maxn (gafq_State *L) {
  gafq_Number max = 0;
  gafqL_checktype(L, 1, GAFQ_TTABLE);
  gafq_pushnil(L);  /* first key */
  while (gafq_next(L, 1)) {
    gafq_pop(L, 1);  /* remove value */
    if (gafq_type(L, -1) == GAFQ_TNUMBER) {
      gafq_Number v = gafq_tonumber(L, -1);
      if (v > max) max = v;
    }
  }
  gafq_pushnumber(L, max);
  return 1;
}


static int getn (gafq_State *L) {
  gafq_pushinteger(L, aux_getn(L, 1));
  return 1;
}


static int setn (gafq_State *L) {
  gafqL_checktype(L, 1, GAFQ_TTABLE);
#ifndef gafqL_setn
  gafqL_setn(L, 1, gafqL_checkint(L, 2));
#else
  gafqL_error(L, GAFQ_QL("setn") " is obsolete");
#endif
  gafq_pushvalue(L, 1);
  return 1;
}


static int tinsert (gafq_State *L) {
  int e = aux_getn(L, 1) + 1;  /* first empty element */
  int pos;  /* where to insert new element */
  switch (gafq_gettop(L)) {
    case 2: {  /* called with only 2 arguments */
      pos = e;  /* insert new element at the end */
      break;
    }
    case 3: {
      int i;
      pos = gafqL_checkint(L, 2);  /* 2nd argument is the position */
      if (pos > e) e = pos;  /* `grow' array if necessary */
      for (i = e; i > pos; i--) {  /* move up elements */
        gafq_rawgeti(L, 1, i-1);
        gafq_rawseti(L, 1, i);  /* t[i] = t[i-1] */
      }
      break;
    }
    default: {
      return gafqL_error(L, "wrong number of arguments to " GAFQ_QL("insert"));
    }
  }
  gafqL_setn(L, 1, e);  /* new size */
  gafq_rawseti(L, 1, pos);  /* t[pos] = v */
  return 0;
}


static int tremove (gafq_State *L) {
  int e = aux_getn(L, 1);
  int pos = gafqL_optint(L, 2, e);
  if (!(1 <= pos && pos <= e))  /* position is outside bounds? */
   return 0;  /* nothing to remove */
  gafqL_setn(L, 1, e - 1);  /* t.n = n-1 */
  gafq_rawgeti(L, 1, pos);  /* result = t[pos] */
  for ( ;pos<e; pos++) {
    gafq_rawgeti(L, 1, pos+1);
    gafq_rawseti(L, 1, pos);  /* t[pos] = t[pos+1] */
  }
  gafq_pushnil(L);
  gafq_rawseti(L, 1, e);  /* t[e] = nil */
  return 1;
}


static void addfield (gafq_State *L, gafqL_Buffer *b, int i) {
  gafq_rawgeti(L, 1, i);
  if (!gafq_isstring(L, -1))
    gafqL_error(L, "invalid value (%s) at index %d in table for "
                  GAFQ_QL("concat"), gafqL_typename(L, -1), i);
    gafqL_addvalue(b);
}


static int tconcat (gafq_State *L) {
  gafqL_Buffer b;
  size_t lsep;
  int i, last;
  const char *sep = gafqL_optgstring(L, 2, "", &lsep);
  gafqL_checktype(L, 1, GAFQ_TTABLE);
  i = gafqL_optint(L, 3, 1);
  last = gafqL_opt(L, gafqL_checkint, 4, gafqL_getn(L, 1));
  gafqL_buffinit(L, &b);
  for (; i < last; i++) {
    addfield(L, &b, i);
    gafqL_addgstring(&b, sep, lsep);
  }
  if (i == last)  /* add last value (if interval was not empty) */
    addfield(L, &b, i);
  gafqL_pushresult(&b);
  return 1;
}



/*
** {======================================================
** Quicksort
** (based on `Algorithms in MODULA-3', Robert Sedgewick;
**  Addison-Wesley, 1993.)
*/


static void set2 (gafq_State *L, int i, int j) {
  gafq_rawseti(L, 1, i);
  gafq_rawseti(L, 1, j);
}

static int sort_comp (gafq_State *L, int a, int b) {
  if (!gafq_isnil(L, 2)) {  /* function? */
    int res;
    gafq_pushvalue(L, 2);
    gafq_pushvalue(L, a-1);  /* -1 to compensate function */
    gafq_pushvalue(L, b-2);  /* -2 to compensate function and `a' */
    gafq_call(L, 2, 1);
    res = gafq_toboolean(L, -1);
    gafq_pop(L, 1);
    return res;
  }
  else  /* a < b? */
    return gafq_lessthan(L, a, b);
}

static void auxsort (gafq_State *L, int l, int u) {
  while (l < u) {  /* for tail recursion */
    int i, j;
    /* sort elements a[l], a[(l+u)/2] and a[u] */
    gafq_rawgeti(L, 1, l);
    gafq_rawgeti(L, 1, u);
    if (sort_comp(L, -1, -2))  /* a[u] < a[l]? */
      set2(L, l, u);  /* swap a[l] - a[u] */
    else
      gafq_pop(L, 2);
    if (u-l == 1) break;  /* only 2 elements */
    i = (l+u)/2;
    gafq_rawgeti(L, 1, i);
    gafq_rawgeti(L, 1, l);
    if (sort_comp(L, -2, -1))  /* a[i]<a[l]? */
      set2(L, i, l);
    else {
      gafq_pop(L, 1);  /* remove a[l] */
      gafq_rawgeti(L, 1, u);
      if (sort_comp(L, -1, -2))  /* a[u]<a[i]? */
        set2(L, i, u);
      else
        gafq_pop(L, 2);
    }
    if (u-l == 2) break;  /* only 3 elements */
    gafq_rawgeti(L, 1, i);  /* Pivot */
    gafq_pushvalue(L, -1);
    gafq_rawgeti(L, 1, u-1);
    set2(L, i, u-1);
    /* a[l] <= P == a[u-1] <= a[u], only need to sort from l+1 to u-2 */
    i = l; j = u-1;
    for (;;) {  /* invariant: a[l..i] <= P <= a[j..u] */
      /* repeat ++i until a[i] >= P */
      while (gafq_rawgeti(L, 1, ++i), sort_comp(L, -1, -2)) {
        if (i>u) gafqL_error(L, "invalid order function for sorting");
        gafq_pop(L, 1);  /* remove a[i] */
      }
      /* repeat --j until a[j] <= P */
      while (gafq_rawgeti(L, 1, --j), sort_comp(L, -3, -1)) {
        if (j<l) gafqL_error(L, "invalid order function for sorting");
        gafq_pop(L, 1);  /* remove a[j] */
      }
      if (j<i) {
        gafq_pop(L, 3);  /* pop pivot, a[i], a[j] */
        break;
      }
      set2(L, i, j);
    }
    gafq_rawgeti(L, 1, u-1);
    gafq_rawgeti(L, 1, i);
    set2(L, u-1, i);  /* swap pivot (a[u-1]) with a[i] */
    /* a[l..i-1] <= a[i] == P <= a[i+1..u] */
    /* adjust so that smaller half is in [j..i] and larger one in [l..u] */
    if (i-l < u-i) {
      j=l; i=i-1; l=i+2;
    }
    else {
      j=i+1; i=u; u=j-2;
    }
    auxsort(L, j, i);  /* call recursively the smaller one */
  }  /* repeat the routine for the larger one */
}

static int sort (gafq_State *L) {
  int n = aux_getn(L, 1);
  gafqL_checkstack(L, 40, "");  /* assume array is smaller than 2^40 */
  if (!gafq_isnoneornil(L, 2))  /* is there a 2nd argument? */
    gafqL_checktype(L, 2, GAFQ_TFUNCTION);
  gafq_settop(L, 2);  /* make sure there is two arguments */
  auxsort(L, 1, n);
  return 0;
}

/* }====================================================== */


static const gafqL_Reg tab_funcs[] = {
  {"concat", tconcat},
  {"foreach", foreach},
  {"foreachi", foreachi},
  {"getn", getn},
  {"maxn", maxn},
  {"insert", tinsert},
  {"remove", tremove},
  {"setn", setn},
  {"sort", sort},
  {NULL, NULL}
};


GAFQLIB_API int gafqopen_table (gafq_State *L) {
  gafqL_register(L, GAFQ_TABLIBNAME, tab_funcs);
  return 1;
}

