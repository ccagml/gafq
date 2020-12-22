/*
** $Id: loadlib.c,v 1.52.1.4 2009/09/09 13:17:16 roberto Exp $
** Dynamic library loader for Gafq
** See Copyright Notice in gafq.h
**
** This module contains an implementation of loadlib for Unix systems
** that have dlfcn, an implementation for Darwin (Mac OS X), an
** implementation for Windows, and a stub for other systems.
*/


#include <stdlib.h>
#include <string.h>


#define loadlib_c
#define GAFQ_LIB

#include "gafq.h"

#include "gauxlib.h"
#include "gafqlib.h"


/* prefix for open functions in C libraries */
#define GAFQ_POF		"gafqopen_"

/* separator for open functions in C libraries */
#define GAFQ_OFSEP	"_"


#define LIBPREFIX	"LOADLIB: "

#define POF		GAFQ_POF
#define LIB_FAIL	"open"


/* error codes for ll_loadfunc */
#define ERRLIB		1
#define ERRFUNC		2

#define setprogdir(L)		((void)0)


static void ll_unloadlib (void *lib);
static void *ll_load (gafq_State *L, const char *path);
static gafq_CFunction ll_sym (gafq_State *L, void *lib, const char *sym);



#if defined(GAFQ_DL_DLOPEN)
/*
** {========================================================================
** This is an implementation of loadlib based on the dlfcn interface.
** The dlfcn interface is available in Linux, SunOS, Solaris, IRIX, FreeBSD,
** NetBSD, AIX 4.2, HPUX 11, and  probably most other Unix flavors, at least
** as an emulation layer on top of native functions.
** =========================================================================
*/

#include <dlfcn.h>

static void ll_unloadlib (void *lib) {
  dlclose(lib);
}


static void *ll_load (gafq_State *L, const char *path) {
  void *lib = dlopen(path, RTLD_NOW);
  if (lib == NULL) gafq_pushstring(L, dlerror());
  return lib;
}


static gafq_CFunction ll_sym (gafq_State *L, void *lib, const char *sym) {
  gafq_CFunction f = (gafq_CFunction)dlsym(lib, sym);
  if (f == NULL) gafq_pushstring(L, dlerror());
  return f;
}

/* }====================================================== */



#elif defined(GAFQ_DL_DLL)
/*
** {======================================================================
** This is an implementation of loadlib for Windows using native functions.
** =======================================================================
*/

#include <windows.h>


#undef setprogdir

static void setprogdir (gafq_State *L) {
  char buff[MAX_PATH + 1];
  char *lb;
  DWORD nsize = sizeof(buff)/sizeof(char);
  DWORD n = GetModuleFileNameA(NULL, buff, nsize);
  if (n == 0 || n == nsize || (lb = strrchr(buff, '\\')) == NULL)
    gafqL_error(L, "unable to get ModuleFileName");
  else {
    *lb = '\0';
    gafqL_gsub(L, gafq_tostring(L, -1), GAFQ_EXECDIR, buff);
    gafq_remove(L, -2);  /* remove original string */
  }
}


static void pusherror (gafq_State *L) {
  int error = GetLastError();
  char buffer[128];
  if (FormatMessageA(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
      NULL, error, 0, buffer, sizeof(buffer), NULL))
    gafq_pushstring(L, buffer);
  else
    gafq_pushfstring(L, "system error %d\n", error);
}

static void ll_unloadlib (void *lib) {
  FreeLibrary((HINSTANCE)lib);
}


static void *ll_load (gafq_State *L, const char *path) {
  HINSTANCE lib = LoadLibraryA(path);
  if (lib == NULL) pusherror(L);
  return lib;
}


static gafq_CFunction ll_sym (gafq_State *L, void *lib, const char *sym) {
  gafq_CFunction f = (gafq_CFunction)GetProcAddress((HINSTANCE)lib, sym);
  if (f == NULL) pusherror(L);
  return f;
}

/* }====================================================== */



#elif defined(GAFQ_DL_DYLD)
/*
** {======================================================================
** Native Mac OS X / Darwin Implementation
** =======================================================================
*/

#include <mach-o/dyld.h>


/* Mac appends a `_' before C function names */
#undef POF
#define POF	"_" GAFQ_POF


static void pusherror (gafq_State *L) {
  const char *err_str;
  const char *err_file;
  NSLinkEditErrors err;
  int err_num;
  NSLinkEditError(&err, &err_num, &err_file, &err_str);
  gafq_pushstring(L, err_str);
}


static const char *errorfromcode (NSObjectFileImageReturnCode ret) {
  switch (ret) {
    case NSObjectFileImageInappropriateFile:
      return "file is not a bundle";
    case NSObjectFileImageArch:
      return "library is for wrong CPU type";
    case NSObjectFileImageFormat:
      return "bad format";
    case NSObjectFileImageAccess:
      return "cannot access file";
    case NSObjectFileImageFailure:
    default:
      return "unable to load library";
  }
}


static void ll_unloadlib (void *lib) {
  NSUnLinkModule((NSModule)lib, NSUNLINKMODULE_OPTION_RESET_LAZY_REFERENCES);
}


static void *ll_load (gafq_State *L, const char *path) {
  NSObjectFileImage img;
  NSObjectFileImageReturnCode ret;
  /* this would be a rare case, but prevents crashing if it happens */
  if(!_dyld_present()) {
    gafq_pushliteral(L, "dyld not present");
    return NULL;
  }
  ret = NSCreateObjectFileImageFromFile(path, &img);
  if (ret == NSObjectFileImageSuccess) {
    NSModule mod = NSLinkModule(img, path, NSLINKMODULE_OPTION_PRIVATE |
                       NSLINKMODULE_OPTION_RETURN_ON_ERROR);
    NSDestroyObjectFileImage(img);
    if (mod == NULL) pusherror(L);
    return mod;
  }
  gafq_pushstring(L, errorfromcode(ret));
  return NULL;
}


static gafq_CFunction ll_sym (gafq_State *L, void *lib, const char *sym) {
  NSSymbol nss = NSLookupSymbolInModule((NSModule)lib, sym);
  if (nss == NULL) {
    gafq_pushfstring(L, "symbol " GAFQ_QS " not found", sym);
    return NULL;
  }
  return (gafq_CFunction)NSAddressOfSymbol(nss);
}

/* }====================================================== */



#else
/*
** {======================================================
** Fallback for other systems
** =======================================================
*/

#undef LIB_FAIL
#define LIB_FAIL	"absent"


#define DLMSG	"dynamic libraries not enabled; check your Gafq installation"


static void ll_unloadlib (void *lib) {
  (void)lib;  /* to avoid warnings */
}


static void *ll_load (gafq_State *L, const char *path) {
  (void)path;  /* to avoid warnings */
  gafq_pushliteral(L, DLMSG);
  return NULL;
}


static gafq_CFunction ll_sym (gafq_State *L, void *lib, const char *sym) {
  (void)lib; (void)sym;  /* to avoid warnings */
  gafq_pushliteral(L, DLMSG);
  return NULL;
}

/* }====================================================== */
#endif



static void **ll_register (gafq_State *L, const char *path) {
  void **plib;
  gafq_pushfstring(L, "%s%s", LIBPREFIX, path);
  gafq_gettable(L, GAFQ_REGISTRYINDEX);  /* check library in registry? */
  if (!gafq_isnil(L, -1))  /* is there an entry? */
    plib = (void **)gafq_touserdata(L, -1);
  else {  /* no entry yet; create one */
    gafq_pop(L, 1);
    plib = (void **)gafq_newuserdata(L, sizeof(const void *));
    *plib = NULL;
    gafqL_getmetatable(L, "_LOADLIB");
    gafq_setmetatable(L, -2);
    gafq_pushfstring(L, "%s%s", LIBPREFIX, path);
    gafq_pushvalue(L, -2);
    gafq_settable(L, GAFQ_REGISTRYINDEX);
  }
  return plib;
}


/*
** __gc tag method: calls library's `ll_unloadlib' function with the lib
** handle
*/
static int gctm (gafq_State *L) {
  void **lib = (void **)gafqL_checkudata(L, 1, "_LOADLIB");
  if (*lib) ll_unloadlib(*lib);
  *lib = NULL;  /* mark library as closed */
  return 0;
}


static int ll_loadfunc (gafq_State *L, const char *path, const char *sym) {
  void **reg = ll_register(L, path);
  if (*reg == NULL) *reg = ll_load(L, path);
  if (*reg == NULL)
    return ERRLIB;  /* unable to load library */
  else {
    gafq_CFunction f = ll_sym(L, *reg, sym);
    if (f == NULL)
      return ERRFUNC;  /* unable to find function */
    gafq_pushcfunction(L, f);
    return 0;  /* return function */
  }
}


static int ll_loadlib (gafq_State *L) {
  const char *path = gafqL_checkstring(L, 1);
  const char *init = gafqL_checkstring(L, 2);
  int stat = ll_loadfunc(L, path, init);
  if (stat == 0)  /* no errors? */
    return 1;  /* return the loaded function */
  else {  /* error; error message is on stack top */
    gafq_pushnil(L);
    gafq_insert(L, -2);
    gafq_pushstring(L, (stat == ERRLIB) ?  LIB_FAIL : "init");
    return 3;  /* return nil, error message, and where */
  }
}



/*
** {======================================================
** 'require' function
** =======================================================
*/


static int readable (const char *filename) {
  FILE *f = fopen(filename, "r");  /* try to open file */
  if (f == NULL) return 0;  /* open failed */
  fclose(f);
  return 1;
}


static const char *pushnexttemplate (gafq_State *L, const char *path) {
  const char *l;
  while (*path == *GAFQ_PATHSEP) path++;  /* skip separators */
  if (*path == '\0') return NULL;  /* no more templates */
  l = strchr(path, *GAFQ_PATHSEP);  /* find next separator */
  if (l == NULL) l = path + strlen(path);
  gafq_pushgstring(L, path, l - path);  /* template */
  return l;
}


static const char *findfile (gafq_State *L, const char *name,
                                           const char *pname) {
  const char *path;
  name = gafqL_gsub(L, name, ".", GAFQ_DIRSEP);
  gafq_getfield(L, GAFQ_ENVIRONINDEX, pname);
  path = gafq_tostring(L, -1);
  if (path == NULL)
    gafqL_error(L, GAFQ_QL("package.%s") " must be a string", pname);
  gafq_pushliteral(L, "");  /* error accumulator */
  while ((path = pushnexttemplate(L, path)) != NULL) {
    const char *filename;
    filename = gafqL_gsub(L, gafq_tostring(L, -1), GAFQ_PATH_MARK, name);
    gafq_remove(L, -2);  /* remove path template */
    if (readable(filename))  /* does file exist and is readable? */
      return filename;  /* return that file name */
    gafq_pushfstring(L, "\n\tno file " GAFQ_QS, filename);
    gafq_remove(L, -2);  /* remove file name */
    gafq_concat(L, 2);  /* add entry to possible error message */
  }
  return NULL;  /* not found */
}


static void loaderror (gafq_State *L, const char *filename) {
  gafqL_error(L, "error loading module " GAFQ_QS " from file " GAFQ_QS ":\n\t%s",
                gafq_tostring(L, 1), filename, gafq_tostring(L, -1));
}


static int loader_Gafq (gafq_State *L) {
  const char *filename;
  const char *name = gafqL_checkstring(L, 1);
  filename = findfile(L, name, "path");
  if (filename == NULL) return 1;  /* library not found in this path */
  if (gafqL_loadfile(L, filename) != 0)
    loaderror(L, filename);
  return 1;  /* library loaded successfully */
}


static const char *mkfuncname (gafq_State *L, const char *modname) {
  const char *funcname;
  const char *mark = strchr(modname, *GAFQ_IGMARK);
  if (mark) modname = mark + 1;
  funcname = gafqL_gsub(L, modname, ".", GAFQ_OFSEP);
  funcname = gafq_pushfstring(L, POF"%s", funcname);
  gafq_remove(L, -2);  /* remove 'gsub' result */
  return funcname;
}


static int loader_C (gafq_State *L) {
  const char *funcname;
  const char *name = gafqL_checkstring(L, 1);
  const char *filename = findfile(L, name, "cpath");
  if (filename == NULL) return 1;  /* library not found in this path */
  funcname = mkfuncname(L, name);
  if (ll_loadfunc(L, filename, funcname) != 0)
    loaderror(L, filename);
  return 1;  /* library loaded successfully */
}


static int loader_Croot (gafq_State *L) {
  const char *funcname;
  const char *filename;
  const char *name = gafqL_checkstring(L, 1);
  const char *p = strchr(name, '.');
  int stat;
  if (p == NULL) return 0;  /* is root */
  gafq_pushgstring(L, name, p - name);
  filename = findfile(L, gafq_tostring(L, -1), "cpath");
  if (filename == NULL) return 1;  /* root not found */
  funcname = mkfuncname(L, name);
  if ((stat = ll_loadfunc(L, filename, funcname)) != 0) {
    if (stat != ERRFUNC) loaderror(L, filename);  /* real error */
    gafq_pushfstring(L, "\n\tno module " GAFQ_QS " in file " GAFQ_QS,
                       name, filename);
    return 1;  /* function not found */
  }
  return 1;
}


static int loader_preload (gafq_State *L) {
  const char *name = gafqL_checkstring(L, 1);
  gafq_getfield(L, GAFQ_ENVIRONINDEX, "preload");
  if (!gafq_istable(L, -1))
    gafqL_error(L, GAFQ_QL("package.preload") " must be a table");
  gafq_getfield(L, -1, name);
  if (gafq_isnil(L, -1))  /* not found? */
    gafq_pushfstring(L, "\n\tno field package.preload['%s']", name);
  return 1;
}


static const int sentinel_ = 0;
#define sentinel	((void *)&sentinel_)


static int ll_require (gafq_State *L) {
  const char *name = gafqL_checkstring(L, 1);
  int i;
  gafq_settop(L, 1);  /* _LOADED table will be at index 2 */
  gafq_getfield(L, GAFQ_REGISTRYINDEX, "_LOADED");
  gafq_getfield(L, 2, name);
  if (gafq_toboolean(L, -1)) {  /* is it there? */
    if (gafq_touserdata(L, -1) == sentinel)  /* check loops */
      gafqL_error(L, "loop or previous error loading module " GAFQ_QS, name);
    return 1;  /* package is already loaded */
  }
  /* else must load it; iterate over available loaders */
  gafq_getfield(L, GAFQ_ENVIRONINDEX, "loaders");
  if (!gafq_istable(L, -1))
    gafqL_error(L, GAFQ_QL("package.loaders") " must be a table");
  gafq_pushliteral(L, "");  /* error message accumulator */
  for (i=1; ; i++) {
    gafq_rawgeti(L, -2, i);  /* get a loader */
    if (gafq_isnil(L, -1))
      gafqL_error(L, "module " GAFQ_QS " not found:%s",
                    name, gafq_tostring(L, -2));
    gafq_pushstring(L, name);
    gafq_call(L, 1, 1);  /* call it */
    if (gafq_isfunction(L, -1))  /* did it find module? */
      break;  /* module loaded successfully */
    else if (gafq_isstring(L, -1))  /* loader returned error message? */
      gafq_concat(L, 2);  /* accumulate it */
    else
      gafq_pop(L, 1);
  }
  gafq_pushlightuserdata(L, sentinel);
  gafq_setfield(L, 2, name);  /* _LOADED[name] = sentinel */
  gafq_pushstring(L, name);  /* pass name as argument to module */
  gafq_call(L, 1, 1);  /* run loaded module */
  if (!gafq_isnil(L, -1))  /* non-nil return? */
    gafq_setfield(L, 2, name);  /* _LOADED[name] = returned value */
  gafq_getfield(L, 2, name);
  if (gafq_touserdata(L, -1) == sentinel) {   /* module did not set a value? */
    gafq_pushboolean(L, 1);  /* use true as result */
    gafq_pushvalue(L, -1);  /* extra copy to be returned */
    gafq_setfield(L, 2, name);  /* _LOADED[name] = true */
  }
  return 1;
}

/* }====================================================== */



/*
** {======================================================
** 'module' function
** =======================================================
*/
  

static void setfenv (gafq_State *L) {
  gafq_Debug ar;
  if (gafq_getstack(L, 1, &ar) == 0 ||
      gafq_getinfo(L, "f", &ar) == 0 ||  /* get calling function */
      gafq_iscfunction(L, -1))
    gafqL_error(L, GAFQ_QL("module") " not called from a Gafq function");
  gafq_pushvalue(L, -2);
  gafq_setfenv(L, -2);
  gafq_pop(L, 1);
}


static void dooptions (gafq_State *L, int n) {
  int i;
  for (i = 2; i <= n; i++) {
    gafq_pushvalue(L, i);  /* get option (a function) */
    gafq_pushvalue(L, -2);  /* module */
    gafq_call(L, 1, 0);
  }
}


static void modinit (gafq_State *L, const char *modname) {
  const char *dot;
  gafq_pushvalue(L, -1);
  gafq_setfield(L, -2, "_M");  /* module._M = module */
  gafq_pushstring(L, modname);
  gafq_setfield(L, -2, "_NAME");
  dot = strrchr(modname, '.');  /* look for last dot in module name */
  if (dot == NULL) dot = modname;
  else dot++;
  /* set _PACKAGE as package name (full module name minus last part) */
  gafq_pushgstring(L, modname, dot - modname);
  gafq_setfield(L, -2, "_PACKAGE");
}


static int ll_module (gafq_State *L) {
  const char *modname = gafqL_checkstring(L, 1);
  int loaded = gafq_gettop(L) + 1;  /* index of _LOADED table */
  gafq_getfield(L, GAFQ_REGISTRYINDEX, "_LOADED");
  gafq_getfield(L, loaded, modname);  /* get _LOADED[modname] */
  if (!gafq_istable(L, -1)) {  /* not found? */
    gafq_pop(L, 1);  /* remove previous result */
    /* try global variable (and create one if it does not exist) */
    if (gafqL_findtable(L, GAFQ_GLOBALSINDEX, modname, 1) != NULL)
      return gafqL_error(L, "name conflict for module " GAFQ_QS, modname);
    gafq_pushvalue(L, -1);
    gafq_setfield(L, loaded, modname);  /* _LOADED[modname] = new table */
  }
  /* check whether table already has a _NAME field */
  gafq_getfield(L, -1, "_NAME");
  if (!gafq_isnil(L, -1))  /* is table an initialized module? */
    gafq_pop(L, 1);
  else {  /* no; initialize it */
    gafq_pop(L, 1);
    modinit(L, modname);
  }
  gafq_pushvalue(L, -1);
  setfenv(L);
  dooptions(L, loaded - 1);
  return 0;
}


static int ll_seeall (gafq_State *L) {
  gafqL_checktype(L, 1, GAFQ_TTABLE);
  if (!gafq_getmetatable(L, 1)) {
    gafq_createtable(L, 0, 1); /* create new metatable */
    gafq_pushvalue(L, -1);
    gafq_setmetatable(L, 1);
  }
  gafq_pushvalue(L, GAFQ_GLOBALSINDEX);
  gafq_setfield(L, -2, "__index");  /* mt.__index = _G */
  return 0;
}


/* }====================================================== */



/* auxiliary mark (for internal use) */
#define AUXMARK		"\1"

static void setpath (gafq_State *L, const char *fieldname, const char *envname,
                                   const char *def) {
  const char *path = getenv(envname);
  if (path == NULL)  /* no environment variable? */
    gafq_pushstring(L, def);  /* use default */
  else {
    /* replace ";;" by ";AUXMARK;" and then AUXMARK by default path */
    path = gafqL_gsub(L, path, GAFQ_PATHSEP GAFQ_PATHSEP,
                              GAFQ_PATHSEP AUXMARK GAFQ_PATHSEP);
    gafqL_gsub(L, path, AUXMARK, def);
    gafq_remove(L, -2);
  }
  setprogdir(L);
  gafq_setfield(L, -2, fieldname);
}


static const gafqL_Reg pk_funcs[] = {
  {"loadlib", ll_loadlib},
  {"seeall", ll_seeall},
  {NULL, NULL}
};


static const gafqL_Reg ll_funcs[] = {
  {"module", ll_module},
  {"require", ll_require},
  {NULL, NULL}
};


static const gafq_CFunction loaders[] =
  {loader_preload, loader_Gafq, loader_C, loader_Croot, NULL};


GAFQLIB_API int gafqopen_package (gafq_State *L) {
  int i;
  /* create new type _LOADLIB */
  gafqL_newmetatable(L, "_LOADLIB");
  gafq_pushcfunction(L, gctm);
  gafq_setfield(L, -2, "__gc");
  /* create `package' table */
  gafqL_register(L, GAFQ_LOADLIBNAME, pk_funcs);
#if defined(GAFQ_COMPAT_LOADLIB) 
  gafq_getfield(L, -1, "loadlib");
  gafq_setfield(L, GAFQ_GLOBALSINDEX, "loadlib");
#endif
  gafq_pushvalue(L, -1);
  gafq_replace(L, GAFQ_ENVIRONINDEX);
  /* create `loaders' table */
  gafq_createtable(L, sizeof(loaders)/sizeof(loaders[0]) - 1, 0);
  /* fill it with pre-defined loaders */
  for (i=0; loaders[i] != NULL; i++) {
    gafq_pushcfunction(L, loaders[i]);
    gafq_rawseti(L, -2, i+1);
  }
  gafq_setfield(L, -2, "loaders");  /* put it in field `loaders' */
  setpath(L, "path", GAFQ_PATH, GAFQ_PATH_DEFAULT);  /* set field `path' */
  setpath(L, "cpath", GAFQ_CPATH, GAFQ_CPATH_DEFAULT); /* set field `cpath' */
  /* store config information */
  gafq_pushliteral(L, GAFQ_DIRSEP "\n" GAFQ_PATHSEP "\n" GAFQ_PATH_MARK "\n"
                     GAFQ_EXECDIR "\n" GAFQ_IGMARK);
  gafq_setfield(L, -2, "config");
  /* set field `loaded' */
  gafqL_findtable(L, GAFQ_REGISTRYINDEX, "_LOADED", 2);
  gafq_setfield(L, -2, "loaded");
  /* set field `preload' */
  gafq_newtable(L);
  gafq_setfield(L, -2, "preload");
  gafq_pushvalue(L, GAFQ_GLOBALSINDEX);
  gafqL_register(L, NULL, ll_funcs);  /* open lib into global table */
  gafq_pop(L, 1);
  return 1;  /* return 'package' table */
}

