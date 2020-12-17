/*
** $Id: loslib.c,v 1.19.1.3 2008/01/18 16:38:18 roberto Exp $
** Standard Operating System library
** See Copyright Notice in gafq.h
*/


#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define loslib_c
#define GAFQ_LIB

#include "gafq.h"

#include "gauxlib.h"
#include "gafqlib.h"


static int os_pushresult (gafq_State *L, int i, const char *filename) {
  int en = errno;  /* calls to Gafq API may change this value */
  if (i) {
    gafq_pushboolean(L, 1);
    return 1;
  }
  else {
    gafq_pushnil(L);
    gafq_pushfstring(L, "%s: %s", filename, strerror(en));
    gafq_pushinteger(L, en);
    return 3;
  }
}


static int os_execute (gafq_State *L) {
  gafq_pushinteger(L, system(gafqL_optstring(L, 1, NULL)));
  return 1;
}


static int os_remove (gafq_State *L) {
  const char *filename = gafqL_checkstring(L, 1);
  return os_pushresult(L, remove(filename) == 0, filename);
}


static int os_rename (gafq_State *L) {
  const char *fromname = gafqL_checkstring(L, 1);
  const char *toname = gafqL_checkstring(L, 2);
  return os_pushresult(L, rename(fromname, toname) == 0, fromname);
}


static int os_tmpname (gafq_State *L) {
  char buff[GAFQ_TMPNAMBUFSIZE];
  int err;
  gafq_tmpnam(buff, err);
  if (err)
    return gafqL_error(L, "unable to generate a unique filename");
  gafq_pushstring(L, buff);
  return 1;
}


static int os_getenv (gafq_State *L) {
  gafq_pushstring(L, getenv(gafqL_checkstring(L, 1)));  /* if NULL push nil */
  return 1;
}


static int os_clock (gafq_State *L) {
  gafq_pushnumber(L, ((gafq_Number)clock())/(gafq_Number)CLOCKS_PER_SEC);
  return 1;
}


/*
** {======================================================
** Time/Date operations
** { year=%Y, month=%m, day=%d, hour=%H, min=%M, sec=%S,
**   wday=%w+1, yday=%j, isdst=? }
** =======================================================
*/

static void setfield (gafq_State *L, const char *key, int value) {
  gafq_pushinteger(L, value);
  gafq_setfield(L, -2, key);
}

static void setboolfield (gafq_State *L, const char *key, int value) {
  if (value < 0)  /* undefined? */
    return;  /* does not set field */
  gafq_pushboolean(L, value);
  gafq_setfield(L, -2, key);
}

static int getboolfield (gafq_State *L, const char *key) {
  int res;
  gafq_getfield(L, -1, key);
  res = gafq_isnil(L, -1) ? -1 : gafq_toboolean(L, -1);
  gafq_pop(L, 1);
  return res;
}


static int getfield (gafq_State *L, const char *key, int d) {
  int res;
  gafq_getfield(L, -1, key);
  if (gafq_isnumber(L, -1))
    res = (int)gafq_tointeger(L, -1);
  else {
    if (d < 0)
      return gafqL_error(L, "field " GAFQ_QS " missing in date table", key);
    res = d;
  }
  gafq_pop(L, 1);
  return res;
}


static int os_date (gafq_State *L) {
  const char *s = gafqL_optstring(L, 1, "%c");
  time_t t = gafqL_opt(L, (time_t)gafqL_checknumber, 2, time(NULL));
  struct tm *stm;
  if (*s == '!') {  /* UTC? */
    stm = gmtime(&t);
    s++;  /* skip `!' */
  }
  else
    stm = localtime(&t);
  if (stm == NULL)  /* invalid date? */
    gafq_pushnil(L);
  else if (strcmp(s, "*t") == 0) {
    gafq_createtable(L, 0, 9);  /* 9 = number of fields */
    setfield(L, "sec", stm->tm_sec);
    setfield(L, "min", stm->tm_min);
    setfield(L, "hour", stm->tm_hour);
    setfield(L, "day", stm->tm_mday);
    setfield(L, "month", stm->tm_mon+1);
    setfield(L, "year", stm->tm_year+1900);
    setfield(L, "wday", stm->tm_wday+1);
    setfield(L, "yday", stm->tm_yday+1);
    setboolfield(L, "isdst", stm->tm_isdst);
  }
  else {
    char cc[3];
    gafqL_Buffer b;
    cc[0] = '%'; cc[2] = '\0';
    gafqL_buffinit(L, &b);
    for (; *s; s++) {
      if (*s != '%' || *(s + 1) == '\0')  /* no conversion specifier? */
        gafqL_addchar(&b, *s);
      else {
        size_t reslen;
        char buff[200];  /* should be big enough for any conversion result */
        cc[1] = *(++s);
        reslen = strftime(buff, sizeof(buff), cc, stm);
        gafqL_addlstring(&b, buff, reslen);
      }
    }
    gafqL_pushresult(&b);
  }
  return 1;
}


static int os_time (gafq_State *L) {
  time_t t;
  if (gafq_isnoneornil(L, 1))  /* called without args? */
    t = time(NULL);  /* get current time */
  else {
    struct tm ts;
    gafqL_checktype(L, 1, GAFQ_TTABLE);
    gafq_settop(L, 1);  /* make sure table is at the top */
    ts.tm_sec = getfield(L, "sec", 0);
    ts.tm_min = getfield(L, "min", 0);
    ts.tm_hour = getfield(L, "hour", 12);
    ts.tm_mday = getfield(L, "day", -1);
    ts.tm_mon = getfield(L, "month", -1) - 1;
    ts.tm_year = getfield(L, "year", -1) - 1900;
    ts.tm_isdst = getboolfield(L, "isdst");
    t = mktime(&ts);
  }
  if (t == (time_t)(-1))
    gafq_pushnil(L);
  else
    gafq_pushnumber(L, (gafq_Number)t);
  return 1;
}


static int os_difftime (gafq_State *L) {
  gafq_pushnumber(L, difftime((time_t)(gafqL_checknumber(L, 1)),
                             (time_t)(gafqL_optnumber(L, 2, 0))));
  return 1;
}

/* }====================================================== */


static int os_setlocale (gafq_State *L) {
  static const int cat[] = {LC_ALL, LC_COLLATE, LC_CTYPE, LC_MONETARY,
                      LC_NUMERIC, LC_TIME};
  static const char *const catnames[] = {"all", "collate", "ctype", "monetary",
     "numeric", "time", NULL};
  const char *l = gafqL_optstring(L, 1, NULL);
  int op = gafqL_checkoption(L, 2, "all", catnames);
  gafq_pushstring(L, setlocale(cat[op], l));
  return 1;
}


static int os_exit (gafq_State *L) {
  exit(gafqL_optint(L, 1, EXIT_SUCCESS));
}

static const gafqL_Reg syslib[] = {
  {"clock",     os_clock},
  {"date",      os_date},
  {"difftime",  os_difftime},
  {"execute",   os_execute},
  {"exit",      os_exit},
  {"getenv",    os_getenv},
  {"remove",    os_remove},
  {"rename",    os_rename},
  {"setlocale", os_setlocale},
  {"time",      os_time},
  {"tmpname",   os_tmpname},
  {NULL, NULL}
};

/* }====================================================== */



GAFQLIB_API int gafqopen_os (gafq_State *L) {
  gafqL_register(L, GAFQ_OSLIBNAME, syslib);
  return 1;
}

