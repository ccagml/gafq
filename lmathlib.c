/*
** $Id: lmathlib.c,v 1.67.1.1 2007/12/27 13:02:25 roberto Exp $
** Standard mathematical library
** See Copyright Notice in gafq.h
*/


#include <stdlib.h>
#include <math.h>

#define lmathlib_c
#define GAFQ_LIB

#include "gafq.h"

#include "lauxlib.h"
#include "gafqlib.h"


#undef PI
#define PI (3.14159265358979323846)
#define RADIANS_PER_DEGREE (PI/180.0)



static int math_abs (gafq_State *L) {
  gafq_pushnumber(L, fabs(gafqL_checknumber(L, 1)));
  return 1;
}

static int math_sin (gafq_State *L) {
  gafq_pushnumber(L, sin(gafqL_checknumber(L, 1)));
  return 1;
}

static int math_sinh (gafq_State *L) {
  gafq_pushnumber(L, sinh(gafqL_checknumber(L, 1)));
  return 1;
}

static int math_cos (gafq_State *L) {
  gafq_pushnumber(L, cos(gafqL_checknumber(L, 1)));
  return 1;
}

static int math_cosh (gafq_State *L) {
  gafq_pushnumber(L, cosh(gafqL_checknumber(L, 1)));
  return 1;
}

static int math_tan (gafq_State *L) {
  gafq_pushnumber(L, tan(gafqL_checknumber(L, 1)));
  return 1;
}

static int math_tanh (gafq_State *L) {
  gafq_pushnumber(L, tanh(gafqL_checknumber(L, 1)));
  return 1;
}

static int math_asin (gafq_State *L) {
  gafq_pushnumber(L, asin(gafqL_checknumber(L, 1)));
  return 1;
}

static int math_acos (gafq_State *L) {
  gafq_pushnumber(L, acos(gafqL_checknumber(L, 1)));
  return 1;
}

static int math_atan (gafq_State *L) {
  gafq_pushnumber(L, atan(gafqL_checknumber(L, 1)));
  return 1;
}

static int math_atan2 (gafq_State *L) {
  gafq_pushnumber(L, atan2(gafqL_checknumber(L, 1), gafqL_checknumber(L, 2)));
  return 1;
}

static int math_ceil (gafq_State *L) {
  gafq_pushnumber(L, ceil(gafqL_checknumber(L, 1)));
  return 1;
}

static int math_floor (gafq_State *L) {
  gafq_pushnumber(L, floor(gafqL_checknumber(L, 1)));
  return 1;
}

static int math_fmod (gafq_State *L) {
  gafq_pushnumber(L, fmod(gafqL_checknumber(L, 1), gafqL_checknumber(L, 2)));
  return 1;
}

static int math_modf (gafq_State *L) {
  double ip;
  double fp = modf(gafqL_checknumber(L, 1), &ip);
  gafq_pushnumber(L, ip);
  gafq_pushnumber(L, fp);
  return 2;
}

static int math_sqrt (gafq_State *L) {
  gafq_pushnumber(L, sqrt(gafqL_checknumber(L, 1)));
  return 1;
}

static int math_pow (gafq_State *L) {
  gafq_pushnumber(L, pow(gafqL_checknumber(L, 1), gafqL_checknumber(L, 2)));
  return 1;
}

static int math_log (gafq_State *L) {
  gafq_pushnumber(L, log(gafqL_checknumber(L, 1)));
  return 1;
}

static int math_log10 (gafq_State *L) {
  gafq_pushnumber(L, log10(gafqL_checknumber(L, 1)));
  return 1;
}

static int math_exp (gafq_State *L) {
  gafq_pushnumber(L, exp(gafqL_checknumber(L, 1)));
  return 1;
}

static int math_deg (gafq_State *L) {
  gafq_pushnumber(L, gafqL_checknumber(L, 1)/RADIANS_PER_DEGREE);
  return 1;
}

static int math_rad (gafq_State *L) {
  gafq_pushnumber(L, gafqL_checknumber(L, 1)*RADIANS_PER_DEGREE);
  return 1;
}

static int math_frexp (gafq_State *L) {
  int e;
  gafq_pushnumber(L, frexp(gafqL_checknumber(L, 1), &e));
  gafq_pushinteger(L, e);
  return 2;
}

static int math_ldexp (gafq_State *L) {
  gafq_pushnumber(L, ldexp(gafqL_checknumber(L, 1), gafqL_checkint(L, 2)));
  return 1;
}



static int math_min (gafq_State *L) {
  int n = gafq_gettop(L);  /* number of arguments */
  gafq_Number dmin = gafqL_checknumber(L, 1);
  int i;
  for (i=2; i<=n; i++) {
    gafq_Number d = gafqL_checknumber(L, i);
    if (d < dmin)
      dmin = d;
  }
  gafq_pushnumber(L, dmin);
  return 1;
}


static int math_max (gafq_State *L) {
  int n = gafq_gettop(L);  /* number of arguments */
  gafq_Number dmax = gafqL_checknumber(L, 1);
  int i;
  for (i=2; i<=n; i++) {
    gafq_Number d = gafqL_checknumber(L, i);
    if (d > dmax)
      dmax = d;
  }
  gafq_pushnumber(L, dmax);
  return 1;
}


static int math_random (gafq_State *L) {
  /* the `%' avoids the (rare) case of r==1, and is needed also because on
     some systems (SunOS!) `rand()' may return a value larger than RAND_MAX */
  gafq_Number r = (gafq_Number)(rand()%RAND_MAX) / (gafq_Number)RAND_MAX;
  switch (gafq_gettop(L)) {  /* check number of arguments */
    case 0: {  /* no arguments */
      gafq_pushnumber(L, r);  /* Number between 0 and 1 */
      break;
    }
    case 1: {  /* only upper limit */
      int u = gafqL_checkint(L, 1);
      gafqL_argcheck(L, 1<=u, 1, "interval is empty");
      gafq_pushnumber(L, floor(r*u)+1);  /* int between 1 and `u' */
      break;
    }
    case 2: {  /* lower and upper limits */
      int l = gafqL_checkint(L, 1);
      int u = gafqL_checkint(L, 2);
      gafqL_argcheck(L, l<=u, 2, "interval is empty");
      gafq_pushnumber(L, floor(r*(u-l+1))+l);  /* int between `l' and `u' */
      break;
    }
    default: return gafqL_error(L, "wrong number of arguments");
  }
  return 1;
}


static int math_randomseed (gafq_State *L) {
  srand(gafqL_checkint(L, 1));
  return 0;
}


static const gafqL_Reg mathlib[] = {
  {"abs",   math_abs},
  {"acos",  math_acos},
  {"asin",  math_asin},
  {"atan2", math_atan2},
  {"atan",  math_atan},
  {"ceil",  math_ceil},
  {"cosh",   math_cosh},
  {"cos",   math_cos},
  {"deg",   math_deg},
  {"exp",   math_exp},
  {"floor", math_floor},
  {"fmod",   math_fmod},
  {"frexp", math_frexp},
  {"ldexp", math_ldexp},
  {"log10", math_log10},
  {"log",   math_log},
  {"max",   math_max},
  {"min",   math_min},
  {"modf",   math_modf},
  {"pow",   math_pow},
  {"rad",   math_rad},
  {"random",     math_random},
  {"randomseed", math_randomseed},
  {"sinh",   math_sinh},
  {"sin",   math_sin},
  {"sqrt",  math_sqrt},
  {"tanh",   math_tanh},
  {"tan",   math_tan},
  {NULL, NULL}
};


/*
** Open math library
*/
GAFQLIB_API int gafqopen_math (gafq_State *L) {
  gafqL_register(L, GAFQ_MATHLIBNAME, mathlib);
  gafq_pushnumber(L, PI);
  gafq_setfield(L, -2, "pi");
  gafq_pushnumber(L, HUGE_VAL);
  gafq_setfield(L, -2, "huge");
#if defined(GAFQ_COMPAT_MOD)
  gafq_getfield(L, -1, "fmod");
  gafq_setfield(L, -2, "mod");
#endif
  return 1;
}

