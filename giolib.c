/*
** $Id: giolib.c,v 2.73.1.4 2010/05/14 15:33:51 roberto Exp $
** Standard I/O (and system) library
** See Copyright Notice in gafq.h
*/


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define giolib_c
#define GAFQ_LIB

#include "gafq.h"

#include "gauxlib.h"
#include "gafqlib.h"



#define IO_INPUT	1
#define IO_OUTPUT	2


static const char *const fnames[] = {"input", "output"};


static int pushresult (gafq_State *L, int i, const char *filename) {
  int en = errno;  /* calls to Gafq API may change this value */
  if (i) {
    gafq_pushboolean(L, 1);
    return 1;
  }
  else {
    gafq_pushnil(L);
    if (filename)
      gafq_pushfstring(L, "%s: %s", filename, strerror(en));
    else
      gafq_pushfstring(L, "%s", strerror(en));
    gafq_pushinteger(L, en);
    return 3;
  }
}


static void fileerror (gafq_State *L, int arg, const char *filename) {
  gafq_pushfstring(L, "%s: %s", filename, strerror(errno));
  gafqL_argerror(L, arg, gafq_tostring(L, -1));
}


#define tofilep(L)	((FILE **)gafqL_checkudata(L, 1, GAFQ_FILEHANDLE))


static int io_type (gafq_State *L) {
  void *ud;
  gafqL_checkany(L, 1);
  ud = gafq_touserdata(L, 1);
  gafq_getfield(L, GAFQ_REGISTRYINDEX, GAFQ_FILEHANDLE);
  if (ud == NULL || !gafq_getmetatable(L, 1) || !gafq_rawequal(L, -2, -1))
    gafq_pushnil(L);  /* not a file */
  else if (*((FILE **)ud) == NULL)
    gafq_pushliteral(L, "closed file");
  else
    gafq_pushliteral(L, "file");
  return 1;
}


static FILE *tofile (gafq_State *L) {
  FILE **f = tofilep(L);
  if (*f == NULL)
    gafqL_error(L, "attempt to use a closed file");
  return *f;
}



/*
** When creating file handles, always creates a `closed' file handle
** before opening the actual file; so, if there is a memory error, the
** file is not left opened.
*/
static FILE **newfile (gafq_State *L) {
  FILE **pf = (FILE **)gafq_newuserdata(L, sizeof(FILE *));
  *pf = NULL;  /* file handle is currently `closed' */
  gafqL_getmetatable(L, GAFQ_FILEHANDLE);
  gafq_setmetatable(L, -2);
  return pf;
}


/*
** function to (not) close the standard files stdin, stdout, and stderr
*/
static int io_noclose (gafq_State *L) {
  gafq_pushnil(L);
  gafq_pushliteral(L, "cannot close standard file");
  return 2;
}


/*
** function to close 'popen' files
*/
static int io_pclose (gafq_State *L) {
  FILE **p = tofilep(L);
  int ok = gafq_pclose(L, *p);
  *p = NULL;
  return pushresult(L, ok, NULL);
}


/*
** function to close regular files
*/
static int io_fclose (gafq_State *L) {
  FILE **p = tofilep(L);
  int ok = (fclose(*p) == 0);
  *p = NULL;
  return pushresult(L, ok, NULL);
}


static int aux_close (gafq_State *L) {
  gafq_getfenv(L, 1);
  gafq_getfield(L, -1, "__close");
  return (gafq_tocfunction(L, -1))(L);
}


static int io_close (gafq_State *L) {
  if (gafq_isnone(L, 1))
    gafq_rawgeti(L, GAFQ_ENVIRONINDEX, IO_OUTPUT);
  tofile(L);  /* make sure argument is a file */
  return aux_close(L);
}


static int io_gc (gafq_State *L) {
  FILE *f = *tofilep(L);
  /* ignore closed files */
  if (f != NULL)
    aux_close(L);
  return 0;
}


static int io_tostring (gafq_State *L) {
  FILE *f = *tofilep(L);
  if (f == NULL)
    gafq_pushliteral(L, "file (closed)");
  else
    gafq_pushfstring(L, "file (%p)", f);
  return 1;
}


static int io_open (gafq_State *L) {
  const char *filename = gafqL_checkstring(L, 1);
  const char *mode = gafqL_optstring(L, 2, "r");
  FILE **pf = newfile(L);
  *pf = fopen(filename, mode);
  return (*pf == NULL) ? pushresult(L, 0, filename) : 1;
}


/*
** this function has a separated environment, which defines the
** correct __close for 'popen' files
*/
static int io_popen (gafq_State *L) {
  const char *filename = gafqL_checkstring(L, 1);
  const char *mode = gafqL_optstring(L, 2, "r");
  FILE **pf = newfile(L);
  *pf = gafq_popen(L, filename, mode);
  return (*pf == NULL) ? pushresult(L, 0, filename) : 1;
}


static int io_tmpfile (gafq_State *L) {
  FILE **pf = newfile(L);
  *pf = tmpfile();
  return (*pf == NULL) ? pushresult(L, 0, NULL) : 1;
}


static FILE *getiofile (gafq_State *L, int findex) {
  FILE *f;
  gafq_rawgeti(L, GAFQ_ENVIRONINDEX, findex);
  f = *(FILE **)gafq_touserdata(L, -1);
  if (f == NULL)
    gafqL_error(L, "standard %s file is closed", fnames[findex - 1]);
  return f;
}


static int g_iofile (gafq_State *L, int f, const char *mode) {
  if (!gafq_isnoneornil(L, 1)) {
    const char *filename = gafq_tostring(L, 1);
    if (filename) {
      FILE **pf = newfile(L);
      *pf = fopen(filename, mode);
      if (*pf == NULL)
        fileerror(L, 1, filename);
    }
    else {
      tofile(L);  /* check that it's a valid file handle */
      gafq_pushvalue(L, 1);
    }
    gafq_rawseti(L, GAFQ_ENVIRONINDEX, f);
  }
  /* return current value */
  gafq_rawgeti(L, GAFQ_ENVIRONINDEX, f);
  return 1;
}


static int io_input (gafq_State *L) {
  return g_iofile(L, IO_INPUT, "r");
}


static int io_output (gafq_State *L) {
  return g_iofile(L, IO_OUTPUT, "w");
}


static int io_readline (gafq_State *L);


static void aux_lines (gafq_State *L, int idx, int toclose) {
  gafq_pushvalue(L, idx);
  gafq_pushboolean(L, toclose);  /* close/not close file when finished */
  gafq_pushcclosure(L, io_readline, 2);
}


static int f_lines (gafq_State *L) {
  tofile(L);  /* check that it's a valid file handle */
  aux_lines(L, 1, 0);
  return 1;
}


static int io_lines (gafq_State *L) {
  if (gafq_isnoneornil(L, 1)) {  /* no arguments? */
    /* will iterate over default input */
    gafq_rawgeti(L, GAFQ_ENVIRONINDEX, IO_INPUT);
    return f_lines(L);
  }
  else {
    const char *filename = gafqL_checkstring(L, 1);
    FILE **pf = newfile(L);
    *pf = fopen(filename, "r");
    if (*pf == NULL)
      fileerror(L, 1, filename);
    aux_lines(L, gafq_gettop(L), 1);
    return 1;
  }
}


/*
** {======================================================
** READ
** =======================================================
*/


static int read_number (gafq_State *L, FILE *f) {
  gafq_Number d;
  if (fscanf(f, GAFQ_NUMBER_SCAN, &d) == 1) {
    gafq_pushnumber(L, d);
    return 1;
  }
  else {
    gafq_pushnil(L);  /* "result" to be removed */
    return 0;  /* read fails */
  }
}


static int test_eof (gafq_State *L, FILE *f) {
  int c = getc(f);
  ungetc(c, f);
  gafq_pushgstring(L, NULL, 0);
  return (c != EOF);
}


static int read_line (gafq_State *L, FILE *f) {
  gafqL_Buffer b;
  gafqL_buffinit(L, &b);
  for (;;) {
    size_t l;
    char *p = gafqL_prepbuffer(&b);
    if (fgets(p, GAFQL_BUFFERSIZE, f) == NULL) {  /* eof? */
      gafqL_pushresult(&b);  /* close buffer */
      return (gafq_objlen(L, -1) > 0);  /* check whether read something */
    }
    l = strlen(p);
    if (l == 0 || p[l-1] != '\n')
      gafqL_addsize(&b, l);
    else {
      gafqL_addsize(&b, l - 1);  /* do not include `eol' */
      gafqL_pushresult(&b);  /* close buffer */
      return 1;  /* read at least an `eol' */
    }
  }
}


static int read_chars (gafq_State *L, FILE *f, size_t n) {
  size_t rlen;  /* how much to read */
  size_t nr;  /* number of chars actually read */
  gafqL_Buffer b;
  gafqL_buffinit(L, &b);
  rlen = GAFQL_BUFFERSIZE;  /* try to read that much each time */
  do {
    char *p = gafqL_prepbuffer(&b);
    if (rlen > n) rlen = n;  /* cannot read more than asked */
    nr = fread(p, sizeof(char), rlen, f);
    gafqL_addsize(&b, nr);
    n -= nr;  /* still have to read `n' chars */
  } while (n > 0 && nr == rlen);  /* until end of count or eof */
  gafqL_pushresult(&b);  /* close buffer */
  return (n == 0 || gafq_objlen(L, -1) > 0);
}


static int g_read (gafq_State *L, FILE *f, int first) {
  int nargs = gafq_gettop(L) - 1;
  int success;
  int n;
  clearerr(f);
  if (nargs == 0) {  /* no arguments? */
    success = read_line(L, f);
    n = first+1;  /* to return 1 result */
  }
  else {  /* ensure stack space for all results and for auxlib's buffer */
    gafqL_checkstack(L, nargs+GAFQ_MINSTACK, "too many arguments");
    success = 1;
    for (n = first; nargs-- && success; n++) {
      if (gafq_type(L, n) == GAFQ_TNUMBER) {
        size_t l = (size_t)gafq_tointeger(L, n);
        success = (l == 0) ? test_eof(L, f) : read_chars(L, f, l);
      }
      else {
        const char *p = gafq_tostring(L, n);
        gafqL_argcheck(L, p && p[0] == '*', n, "invalid option");
        switch (p[1]) {
          case 'n':  /* number */
            success = read_number(L, f);
            break;
          case 'l':  /* line */
            success = read_line(L, f);
            break;
          case 'a':  /* file */
            read_chars(L, f, ~((size_t)0));  /* read MAX_SIZE_T chars */
            success = 1; /* always success */
            break;
          default:
            return gafqL_argerror(L, n, "invalid format");
        }
      }
    }
  }
  if (ferror(f))
    return pushresult(L, 0, NULL);
  if (!success) {
    gafq_pop(L, 1);  /* remove last result */
    gafq_pushnil(L);  /* push nil instead */
  }
  return n - first;
}


static int io_read (gafq_State *L) {
  return g_read(L, getiofile(L, IO_INPUT), 1);
}


static int f_read (gafq_State *L) {
  return g_read(L, tofile(L), 2);
}


static int io_readline (gafq_State *L) {
  FILE *f = *(FILE **)gafq_touserdata(L, gafq_upvalueindex(1));
  int sucess;
  if (f == NULL)  /* file is already closed? */
    gafqL_error(L, "file is already closed");
  sucess = read_line(L, f);
  if (ferror(f))
    return gafqL_error(L, "%s", strerror(errno));
  if (sucess) return 1;
  else {  /* EOF */
    if (gafq_toboolean(L, gafq_upvalueindex(2))) {  /* generator created file? */
      gafq_settop(L, 0);
      gafq_pushvalue(L, gafq_upvalueindex(1));
      aux_close(L);  /* close it */
    }
    return 0;
  }
}

/* }====================================================== */


static int g_write (gafq_State *L, FILE *f, int arg) {
  int nargs = gafq_gettop(L) - 1;
  int status = 1;
  for (; nargs--; arg++) {
    if (gafq_type(L, arg) == GAFQ_TNUMBER) {
      /* optimization: could be done exactly as for strings */
      status = status &&
          fprintf(f, GAFQ_NUMBER_FMT, gafq_tonumber(L, arg)) > 0;
    }
    else {
      size_t l;
      const char *s = gafqL_checkgstring(L, arg, &l);
      status = status && (fwrite(s, sizeof(char), l, f) == l);
    }
  }
  return pushresult(L, status, NULL);
}


static int io_write (gafq_State *L) {
  return g_write(L, getiofile(L, IO_OUTPUT), 1);
}


static int f_write (gafq_State *L) {
  return g_write(L, tofile(L), 2);
}


static int f_seek (gafq_State *L) {
  static const int mode[] = {SEEK_SET, SEEK_CUR, SEEK_END};
  static const char *const modenames[] = {"set", "cur", "end", NULL};
  FILE *f = tofile(L);
  int op = gafqL_checkoption(L, 2, "cur", modenames);
  long offset = gafqL_optlong(L, 3, 0);
  op = fseek(f, offset, mode[op]);
  if (op)
    return pushresult(L, 0, NULL);  /* error */
  else {
    gafq_pushinteger(L, ftell(f));
    return 1;
  }
}


static int f_setvbuf (gafq_State *L) {
  static const int mode[] = {_IONBF, _IOFBF, _IOLBF};
  static const char *const modenames[] = {"no", "full", "line", NULL};
  FILE *f = tofile(L);
  int op = gafqL_checkoption(L, 2, NULL, modenames);
  gafq_Integer sz = gafqL_optinteger(L, 3, GAFQL_BUFFERSIZE);
  int res = setvbuf(f, NULL, mode[op], sz);
  return pushresult(L, res == 0, NULL);
}



static int io_flush (gafq_State *L) {
  return pushresult(L, fflush(getiofile(L, IO_OUTPUT)) == 0, NULL);
}


static int f_flush (gafq_State *L) {
  return pushresult(L, fflush(tofile(L)) == 0, NULL);
}


static const gafqL_Reg iolib[] = {
  {"close", io_close},
  {"flush", io_flush},
  {"input", io_input},
  {"lines", io_lines},
  {"open", io_open},
  {"output", io_output},
  {"popen", io_popen},
  {"read", io_read},
  {"tmpfile", io_tmpfile},
  {"type", io_type},
  {"write", io_write},
  {NULL, NULL}
};


static const gafqL_Reg flib[] = {
  {"close", io_close},
  {"flush", f_flush},
  {"lines", f_lines},
  {"read", f_read},
  {"seek", f_seek},
  {"setvbuf", f_setvbuf},
  {"write", f_write},
  {"__gc", io_gc},
  {"__tostring", io_tostring},
  {NULL, NULL}
};


static void createmeta (gafq_State *L) {
  gafqL_newmetatable(L, GAFQ_FILEHANDLE);  /* create metatable for file handles */
  gafq_pushvalue(L, -1);  /* push metatable */
  gafq_setfield(L, -2, "__index");  /* metatable.__index = metatable */
  gafqL_register(L, NULL, flib);  /* file methods */
}


static void createstdfile (gafq_State *L, FILE *f, int k, const char *fname) {
  *newfile(L) = f;
  if (k > 0) {
    gafq_pushvalue(L, -1);
    gafq_rawseti(L, GAFQ_ENVIRONINDEX, k);
  }
  gafq_pushvalue(L, -2);  /* copy environment */
  gafq_setfenv(L, -2);  /* set it */
  gafq_setfield(L, -3, fname);
}


static void newfenv (gafq_State *L, gafq_CFunction cls) {
  gafq_createtable(L, 0, 1);
  gafq_pushcfunction(L, cls);
  gafq_setfield(L, -2, "__close");
}


GAFQLIB_API int gafqopen_io (gafq_State *L) {
  createmeta(L);
  /* create (private) environment (with fields IO_INPUT, IO_OUTPUT, __close) */
  newfenv(L, io_fclose);
  gafq_replace(L, GAFQ_ENVIRONINDEX);
  /* open library */
  gafqL_register(L, GAFQ_IOLIBNAME, iolib);
  /* create (and set) default files */
  newfenv(L, io_noclose);  /* close function for default files */
  createstdfile(L, stdin, IO_INPUT, "stdin");
  createstdfile(L, stdout, IO_OUTPUT, "stdout");
  createstdfile(L, stderr, 0, "stderr");
  gafq_pop(L, 1);  /* pop environment for default files */
  gafq_getfield(L, -1, "popen");
  newfenv(L, io_pclose);  /* create environment for 'popen' */
  gafq_setfenv(L, -2);  /* set fenv for 'popen' */
  gafq_pop(L, 1);  /* pop 'popen' */
  return 1;
}

