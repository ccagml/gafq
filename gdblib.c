/*
** $Id: gdblib.c,v 1.104.1.4 2009/08/04 18:50:18 roberto Exp $
** Interface from Gafq to its debug API
** See Copyright Notice in gafq.h
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define gdblib_c
#define GAFQ_LIB

#include "gafq.h"

#include "gauxlib.h"
#include "gafqlib.h"



static int db_getregistry (gafq_State *L) {
  gafq_pushvalue(L, GAFQ_REGISTRYINDEX);
  return 1;
}


static int db_getmetatable (gafq_State *L) {
  gafqL_checkany(L, 1);
  if (!gafq_getmetatable(L, 1)) {
    gafq_pushnil(L);  /* no metatable */
  }
  return 1;
}


static int db_setmetatable (gafq_State *L) {
  int t = gafq_type(L, 2);
  gafqL_argcheck(L, t == GAFQ_TNIL || t == GAFQ_TTABLE, 2,
                    "nil or table expected");
  gafq_settop(L, 2);
  gafq_pushboolean(L, gafq_setmetatable(L, 1));
  return 1;
}


static int db_getfenv (gafq_State *L) {
  gafqL_checkany(L, 1);
  gafq_getfenv(L, 1);
  return 1;
}


static int db_setfenv (gafq_State *L) {
  gafqL_checktype(L, 2, GAFQ_TTABLE);
  gafq_settop(L, 2);
  if (gafq_setfenv(L, 1) == 0)
    gafqL_error(L, GAFQ_QL("setfenv")
                  " cannot change environment of given object");
  return 1;
}


static void settabss (gafq_State *L, const char *i, const char *v) {
  gafq_pushstring(L, v);
  gafq_setfield(L, -2, i);
}


static void settabsi (gafq_State *L, const char *i, int v) {
  gafq_pushinteger(L, v);
  gafq_setfield(L, -2, i);
}


static gafq_State *getthread (gafq_State *L, int *arg) {
  if (gafq_isthread(L, 1)) {
    *arg = 1;
    return gafq_tothread(L, 1);
  }
  else {
    *arg = 0;
    return L;
  }
}


static void treatstackoption (gafq_State *L, gafq_State *L1, const char *fname) {
  if (L == L1) {
    gafq_pushvalue(L, -2);
    gafq_remove(L, -3);
  }
  else
    gafq_xmove(L1, L, 1);
  gafq_setfield(L, -2, fname);
}


static int db_getinfo (gafq_State *L) {
  gafq_Debug ar;
  int arg;
  gafq_State *L1 = getthread(L, &arg);
  const char *options = gafqL_optstring(L, arg+2, "flnSu");
  if (gafq_isnumber(L, arg+1)) {
    if (!gafq_getstack(L1, (int)gafq_tointeger(L, arg+1), &ar)) {
      gafq_pushnil(L);  /* level out of range */
      return 1;
    }
  }
  else if (gafq_isfunction(L, arg+1)) {
    gafq_pushfstring(L, ">%s", options);
    options = gafq_tostring(L, -1);
    gafq_pushvalue(L, arg+1);
    gafq_xmove(L, L1, 1);
  }
  else
    return gafqL_argerror(L, arg+1, "function or level expected");
  if (!gafq_getinfo(L1, options, &ar))
    return gafqL_argerror(L, arg+2, "invalid option");
  gafq_createtable(L, 0, 2);
  if (strchr(options, 'S')) {
    settabss(L, "source", ar.source);
    settabss(L, "short_src", ar.short_src);
    settabsi(L, "linedefined", ar.linedefined);
    settabsi(L, "lastlinedefined", ar.lastlinedefined);
    settabss(L, "what", ar.what);
  }
  if (strchr(options, 'l'))
    settabsi(L, "currentline", ar.currentline);
  if (strchr(options, 'u'))
    settabsi(L, "nups", ar.nups);
  if (strchr(options, 'n')) {
    settabss(L, "name", ar.name);
    settabss(L, "namewhat", ar.namewhat);
  }
  if (strchr(options, 'L'))
    treatstackoption(L, L1, "activelines");
  if (strchr(options, 'f'))
    treatstackoption(L, L1, "func");
  return 1;  /* return table */
}
    

static int db_getlocal (gafq_State *L) {
  int arg;
  gafq_State *L1 = getthread(L, &arg);
  gafq_Debug ar;
  const char *name;
  if (!gafq_getstack(L1, gafqL_checkint(L, arg+1), &ar))  /* out of range? */
    return gafqL_argerror(L, arg+1, "level out of range");
  name = gafq_getlocal(L1, &ar, gafqL_checkint(L, arg+2));
  if (name) {
    gafq_xmove(L1, L, 1);
    gafq_pushstring(L, name);
    gafq_pushvalue(L, -2);
    return 2;
  }
  else {
    gafq_pushnil(L);
    return 1;
  }
}


static int db_setlocal (gafq_State *L) {
  int arg;
  gafq_State *L1 = getthread(L, &arg);
  gafq_Debug ar;
  if (!gafq_getstack(L1, gafqL_checkint(L, arg+1), &ar))  /* out of range? */
    return gafqL_argerror(L, arg+1, "level out of range");
  gafqL_checkany(L, arg+3);
  gafq_settop(L, arg+3);
  gafq_xmove(L, L1, 1);
  gafq_pushstring(L, gafq_setlocal(L1, &ar, gafqL_checkint(L, arg+2)));
  return 1;
}


static int auxupvalue (gafq_State *L, int get) {
  const char *name;
  int n = gafqL_checkint(L, 2);
  gafqL_checktype(L, 1, GAFQ_TFUNCTION);
  if (gafq_iscfunction(L, 1)) return 0;  /* cannot touch C upvalues from Gafq */
  name = get ? gafq_getupvalue(L, 1, n) : gafq_setupvalue(L, 1, n);
  if (name == NULL) return 0;
  gafq_pushstring(L, name);
  gafq_insert(L, -(get+1));
  return get + 1;
}


static int db_getupvalue (gafq_State *L) {
  return auxupvalue(L, 1);
}


static int db_setupvalue (gafq_State *L) {
  gafqL_checkany(L, 3);
  return auxupvalue(L, 0);
}



static const char KEY_HOOK = 'h';


static void hookf (gafq_State *L, gafq_Debug *ar) {
  static const char *const hooknames[] =
    {"call", "return", "line", "count", "tail return"};
  gafq_pushlightuserdata(L, (void *)&KEY_HOOK);
  gafq_rawget(L, GAFQ_REGISTRYINDEX);
  gafq_pushlightuserdata(L, L);
  gafq_rawget(L, -2);
  if (gafq_isfunction(L, -1)) {
    gafq_pushstring(L, hooknames[(int)ar->event]);
    if (ar->currentline >= 0)
      gafq_pushinteger(L, ar->currentline);
    else gafq_pushnil(L);
    gafq_assert(gafq_getinfo(L, "lS", ar));
    gafq_call(L, 2, 0);
  }
}


static int makemask (const char *smask, int count) {
  int mask = 0;
  if (strchr(smask, 'c')) mask |= GAFQ_MASKCALL;
  if (strchr(smask, 'r')) mask |= GAFQ_MASKRET;
  if (strchr(smask, 'l')) mask |= GAFQ_MASKLINE;
  if (count > 0) mask |= GAFQ_MASKCOUNT;
  return mask;
}


static char *unmakemask (int mask, char *smask) {
  int i = 0;
  if (mask & GAFQ_MASKCALL) smask[i++] = 'c';
  if (mask & GAFQ_MASKRET) smask[i++] = 'r';
  if (mask & GAFQ_MASKLINE) smask[i++] = 'l';
  smask[i] = '\0';
  return smask;
}


static void gethooktable (gafq_State *L) {
  gafq_pushlightuserdata(L, (void *)&KEY_HOOK);
  gafq_rawget(L, GAFQ_REGISTRYINDEX);
  if (!gafq_istable(L, -1)) {
    gafq_pop(L, 1);
    gafq_createtable(L, 0, 1);
    gafq_pushlightuserdata(L, (void *)&KEY_HOOK);
    gafq_pushvalue(L, -2);
    gafq_rawset(L, GAFQ_REGISTRYINDEX);
  }
}


static int db_sethook (gafq_State *L) {
  int arg, mask, count;
  gafq_Hook func;
  gafq_State *L1 = getthread(L, &arg);
  if (gafq_isnoneornil(L, arg+1)) {
    gafq_settop(L, arg+1);
    func = NULL; mask = 0; count = 0;  /* turn off hooks */
  }
  else {
    const char *smask = gafqL_checkstring(L, arg+2);
    gafqL_checktype(L, arg+1, GAFQ_TFUNCTION);
    count = gafqL_optint(L, arg+3, 0);
    func = hookf; mask = makemask(smask, count);
  }
  gethooktable(L);
  gafq_pushlightuserdata(L, L1);
  gafq_pushvalue(L, arg+1);
  gafq_rawset(L, -3);  /* set new hook */
  gafq_pop(L, 1);  /* remove hook table */
  gafq_sethook(L1, func, mask, count);  /* set hooks */
  return 0;
}


static int db_gethook (gafq_State *L) {
  int arg;
  gafq_State *L1 = getthread(L, &arg);
  char buff[5];
  int mask = gafq_gethookmask(L1);
  gafq_Hook hook = gafq_gethook(L1);
  if (hook != NULL && hook != hookf)  /* external hook? */
    gafq_pushliteral(L, "external hook");
  else {
    gethooktable(L);
    gafq_pushlightuserdata(L, L1);
    gafq_rawget(L, -2);   /* get hook */
    gafq_remove(L, -2);  /* remove hook table */
  }
  gafq_pushstring(L, unmakemask(mask, buff));
  gafq_pushinteger(L, gafq_gethookcount(L1));
  return 3;
}


static int db_debug (gafq_State *L) {
  for (;;) {
    char buffer[250];
    fputs("gafq_debug> ", stderr);
    if (fgets(buffer, sizeof(buffer), stdin) == 0 ||
        strcmp(buffer, "cont\n") == 0)
      return 0;
    if (gafqL_loadbuffer(L, buffer, strlen(buffer), "=(debug command)") ||
        gafq_pcall(L, 0, 0, 0)) {
      fputs(gafq_tostring(L, -1), stderr);
      fputs("\n", stderr);
    }
    gafq_settop(L, 0);  /* remove eventual returns */
  }
}


#define LEVELS1	12	/* size of the first part of the stack */
#define LEVELS2	10	/* size of the second part of the stack */

static int db_errorfb (gafq_State *L) {
  int level;
  int firstpart = 1;  /* still before eventual `...' */
  int arg;
  gafq_State *L1 = getthread(L, &arg);
  gafq_Debug ar;
  if (gafq_isnumber(L, arg+2)) {
    level = (int)gafq_tointeger(L, arg+2);
    gafq_pop(L, 1);
  }
  else
    level = (L == L1) ? 1 : 0;  /* level 0 may be this own function */
  if (gafq_gettop(L) == arg)
    gafq_pushliteral(L, "");
  else if (!gafq_isstring(L, arg+1)) return 1;  /* message is not a string */
  else gafq_pushliteral(L, "\n");
  gafq_pushliteral(L, "stack traceback:");
  while (gafq_getstack(L1, level++, &ar)) {
    if (level > LEVELS1 && firstpart) {
      /* no more than `LEVELS2' more levels? */
      if (!gafq_getstack(L1, level+LEVELS2, &ar))
        level--;  /* keep going */
      else {
        gafq_pushliteral(L, "\n\t...");  /* too many levels */
        while (gafq_getstack(L1, level+LEVELS2, &ar))  /* find last levels */
          level++;
      }
      firstpart = 0;
      continue;
    }
    gafq_pushliteral(L, "\n\t");
    gafq_getinfo(L1, "Snl", &ar);
    gafq_pushfstring(L, "%s:", ar.short_src);
    if (ar.currentline > 0)
      gafq_pushfstring(L, "%d:", ar.currentline);
    if (*ar.namewhat != '\0')  /* is there a name? */
        gafq_pushfstring(L, " in function " GAFQ_QS, ar.name);
    else {
      if (*ar.what == 'm')  /* main? */
        gafq_pushfstring(L, " in main chunk");
      else if (*ar.what == 'C' || *ar.what == 't')
        gafq_pushliteral(L, " ?");  /* C function or tail call */
      else
        gafq_pushfstring(L, " in function <%s:%d>",
                           ar.short_src, ar.linedefined);
    }
    gafq_concat(L, gafq_gettop(L) - arg);
  }
  gafq_concat(L, gafq_gettop(L) - arg);
  return 1;
}


static const gafqL_Reg dblib[] = {
  {"debug", db_debug},
  {"getfenv", db_getfenv},
  {"gethook", db_gethook},
  {"getinfo", db_getinfo},
  {"getlocal", db_getlocal},
  {"getregistry", db_getregistry},
  {"getmetatable", db_getmetatable},
  {"getupvalue", db_getupvalue},
  {"setfenv", db_setfenv},
  {"sethook", db_sethook},
  {"setlocal", db_setlocal},
  {"setmetatable", db_setmetatable},
  {"setupvalue", db_setupvalue},
  {"traceback", db_errorfb},
  {NULL, NULL}
};


GAFQLIB_API int gafqopen_debug (gafq_State *L) {
  gafqL_register(L, GAFQ_DBLIBNAME, dblib);
  return 1;
}

