/*
** $Id: gafq.c,v 1.160.1.2 2007/12/28 15:32:23 roberto Exp $
** Gafq stand-alone interpreter
** See Copyright Notice in gafq.h
*/


#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define gafq_c

#include "gafq.h"

#include "gauxlib.h"
#include "gafqlib.h"



static gafq_State *globalL = NULL;

static const char *progname = GAFQ_PROGNAME;



static void lstop (gafq_State *L, gafq_Debug *ar) {
  (void)ar;  /* unused arg. */
  gafq_sethook(L, NULL, 0, 0);
  gafqL_error(L, "interrupted!");
}


static void laction (int i) {
  signal(i, SIG_DFL); /* if another SIGINT happens before lstop,
                              terminate process (default action) */
  gafq_sethook(globalL, lstop, GAFQ_MASKCALL | GAFQ_MASKRET | GAFQ_MASKCOUNT, 1);
}


static void print_usage (void) {
  fprintf(stderr,
  "usage: %s [options] [script [args]].\n"
  "Available options are:\n"
  "  -e stat  execute string " GAFQ_QL("stat") "\n"
  "  -l name  require library " GAFQ_QL("name") "\n"
  "  -i       enter interactive mode after executing " GAFQ_QL("script") "\n"
  "  -v       show version information\n"
  "  --       stop handling options\n"
  "  -        execute stdin and stop handling options\n"
  ,
  progname);
  fflush(stderr);
}


static void l_message (const char *pname, const char *msg) {
  if (pname) fprintf(stderr, "%s: ", pname);
  fprintf(stderr, "%s\n", msg);
  fflush(stderr);
}


static int report (gafq_State *L, int status) {
  if (status && !gafq_isnil(L, -1)) {
    const char *msg = gafq_tostring(L, -1);
    if (msg == NULL) msg = "(error object is not a string)";
    l_message(progname, msg);
    gafq_pop(L, 1);
  }
  return status;
}


static int traceback (gafq_State *L) {
  if (!gafq_isstring(L, 1))  /* 'message' not a string? */
    return 1;  /* keep it intact */
  gafq_getfield(L, GAFQ_GLOBALSINDEX, "debug");
  if (!gafq_istable(L, -1)) {
    gafq_pop(L, 1);
    return 1;
  }
  gafq_getfield(L, -1, "traceback");
  if (!gafq_isfunction(L, -1)) {
    gafq_pop(L, 2);
    return 1;
  }
  gafq_pushvalue(L, 1);  /* pass error message */
  gafq_pushinteger(L, 2);  /* skip this function and traceback */
  gafq_call(L, 2, 1);  /* call debug.traceback */
  return 1;
}

//加载后文件，应该是执行
static int docall (gafq_State *L, int narg, int clear) {
  int status;
  int base = gafq_gettop(L) - narg;  /* function index */
  gafq_pushcfunction(L, traceback);  /* push traceback function */
  gafq_insert(L, base);  /* put it under chunk and args */
  signal(SIGINT, laction);
  status = gafq_pcall(L, narg, (clear ? 0 : GAFQ_MULTRET), base);
  signal(SIGINT, SIG_DFL);
  gafq_remove(L, base);  /* remove traceback function */
  /* force a complete garbage collection in case of errors */
  if (status != 0) gafq_gc(L, GAFQ_GCCOLLECT, 0);
  return status;
}

// 打印版本号，如果有has_v
static void print_version (void) {
  l_message(NULL, GAFQ_RELEASE "  " GAFQ_COPYRIGHT);
}


static int getargs (gafq_State *L, char **argv, int n) {
  int narg;
  int i;
  int argc = 0;
  while (argv[argc]) argc++;  /* count total number of arguments */
  narg = argc - (n + 1);  /* number of arguments to the script */
  gafqL_checkstack(L, narg + 3, "too many arguments to script");
  for (i=n+1; i < argc; i++)
    gafq_pushstring(L, argv[i]);
  gafq_createtable(L, narg, n + 1);
  for (i=0; i < argc; i++) {
    gafq_pushstring(L, argv[i]);
    gafq_rawseti(L, -2, i - n);
  }
  return narg;
}


static int dofile (gafq_State *L, const char *name) {
  int status = gafqL_loadfile(L, name) || docall(L, 0, 1);
  return report(L, status);
}


static int dostring (gafq_State *L, const char *s, const char *name) {
  int status = gafqL_loadbuffer(L, s, strlen(s), name) || docall(L, 0, 1);
  return report(L, status);
}


static int dolibrary (gafq_State *L, const char *name) {
  gafq_getglobal(L, "require");
  gafq_pushstring(L, name);
  return report(L, docall(L, 1, 1));
}


static const char *get_prompt (gafq_State *L, int firstline) {
  const char *p;
  gafq_getfield(L, GAFQ_GLOBALSINDEX, firstline ? "_PROMPT" : "_PROMPT2");
  p = gafq_tostring(L, -1);
  if (p == NULL) p = (firstline ? GAFQ_PROMPT : GAFQ_PROMPT2);
  gafq_pop(L, 1);  /* remove global */
  return p;
}


static int incomplete (gafq_State *L, int status) {
  if (status == GAFQ_ERRSYNTAX) {
    size_t lmsg;
    const char *msg = gafq_togstring(L, -1, &lmsg);
    const char *tp = msg + lmsg - (sizeof(GAFQ_QL("<eof>")) - 1);
    if (strstr(msg, GAFQ_QL("<eof>")) == tp) {
      gafq_pop(L, 1);
      return 1;
    }
  }
  return 0;  /* else... */
}


static int pushline (gafq_State *L, int firstline) {
  char buffer[GAFQ_MAXINPUT];
  char *b = buffer;
  size_t l;
  const char *prmt = get_prompt(L, firstline);
  if (gafq_readline(L, b, prmt) == 0)
    return 0;  /* no input */
  l = strlen(b);
  if (l > 0 && b[l-1] == '\n')  /* line ends with newline? */
    b[l-1] = '\0';  /* remove it */
  if (firstline && b[0] == '=')  /* first line starts with `=' ? */
    gafq_pushfstring(L, "return %s", b+1);  /* change it to `return' */
  else
    gafq_pushstring(L, b);
  gafq_freeline(L, b);
  return 1;
}


static int loadline (gafq_State *L) {
  int status;
  gafq_settop(L, 0);
  if (!pushline(L, 1))
    return -1;  /* no input */
  for (;;) {  /* repeat until gets a complete line */
    status = gafqL_loadbuffer(L, gafq_tostring(L, 1), gafq_strlen(L, 1), "=stdin");
    if (!incomplete(L, status)) break;  /* cannot try to add lines? */
    if (!pushline(L, 0))  /* no more input? */
      return -1;
    gafq_pushliteral(L, "\n");  /* add a new line... */
    gafq_insert(L, -2);  /* ...between the two lines */
    gafq_concat(L, 3);  /* join them */
  }
  gafq_saveline(L, 1);
  gafq_remove(L, 1);  /* remove line */
  return status;
}


static void dotty (gafq_State *L) {
  int status;
  const char *oldprogname = progname;
  progname = NULL;
  while ((status = loadline(L)) != -1) {
    if (status == 0) status = docall(L, 0, 0);
    report(L, status);
    if (status == 0 && gafq_gettop(L) > 0) {  /* any result to print? */
      gafq_getglobal(L, "print");
      gafq_insert(L, 1);
      if (gafq_pcall(L, gafq_gettop(L)-1, 0, 0) != 0)
        l_message(progname, gafq_pushfstring(L,
                               "error calling " GAFQ_QL("print") " (%s)",
                               gafq_tostring(L, -1)));
    }
  }
  gafq_settop(L, 0);  /* clear stack */
  fputs("\n", stdout);
  fflush(stdout);
  progname = oldprogname;
}


static int handle_script (gafq_State *L, char **argv, int n) {
  int status;
  const char *fname;
  int narg = getargs(L, argv, n);  /* collect arguments */
  gafq_setglobal(L, "arg");
  fname = argv[n];
  if (strcmp(fname, "-") == 0 && strcmp(argv[n-1], "--") != 0) 
    fname = NULL;  /* stdin */
  status = gafqL_loadfile(L, fname);
  gafq_insert(L, -(narg+1));
  if (status == 0)
    status = docall(L, narg, 0);// 这边执行了文件好像
  else
    gafq_pop(L, narg);      
  return report(L, status);
}


/* check that argument has no extra characters at the end */
#define notail(x)	{if ((x)[2] != '\0') return -1;}

//解析参数?
static int collectargs (char **argv, int *pi, int *pv, int *pe) {
  int i;
  for (i = 1; argv[i] != NULL; i++) {
    if (argv[i][0] != '-')  /* not an option? */
        return i;
    switch (argv[i][1]) {  /* option */
      case '-':
        notail(argv[i]);
        return (argv[i+1] != NULL ? i+1 : 0);
      case '\0':
        return i;
      case 'i':
        notail(argv[i]);
        *pi = 1;  /* go through */
      case 'v':
        notail(argv[i]);
        *pv = 1;
        break;
      case 'e':
        *pe = 1;  /* go through */
      case 'l':
        if (argv[i][2] == '\0') {
          i++;
          if (argv[i] == NULL) return -1;
        }
        break;
      default: return -1;  /* invalid option */
    }
  }
  return 0;
}

// 看着是解析参数
static int runargs (gafq_State *L, char **argv, int n) {
  int i;
  for (i = 1; i < n; i++) {
    if (argv[i] == NULL) continue;
    gafq_assert(argv[i][0] == '-');
    switch (argv[i][1]) {  /* option */
      case 'e': {
        const char *chunk = argv[i] + 2;
        if (*chunk == '\0') chunk = argv[++i];
        gafq_assert(chunk != NULL);
        if (dostring(L, chunk, "=(command line)") != 0)
          return 1;
        break;
      }
      case 'l': {
        const char *filename = argv[i] + 2;
        if (*filename == '\0') filename = argv[++i];
        gafq_assert(filename != NULL);
        if (dolibrary(L, filename))
          return 1;  /* stop if file fails */
        break;
      }
      default: break;
    }
  }
  return 0;
}


static int handle_gafqinit (gafq_State *L) {
  const char *init = getenv(GAFQ_INIT);
  if (init == NULL) return 0;  /* status OK */
  else if (init[0] == '@')
    return dofile(L, init+1);
  else
    return dostring(L, init, "=" GAFQ_INIT);
}


struct Smain {
  int argc;
  char **argv;
  int status;
};

// 保护运行一开始传入的方法
// pmain(gafq_State * L) (\root\gafq\gafq.c:341)
// gafqD_precall(gafq_State * L, StkId func, int nresults) (\root\gafq\gdo.c:320)
// gafqD_call(gafq_State * L, StkId func, int nResults) (\root\gafq\gdo.c:377)
// f_Ccall(gafq_State * L, void * ud) (\root\gafq\gapi.c:847)
// gafqD_rawrunprotected(gafq_State * L, Pfunc f, void * ud) (\root\gafq\gdo.c:116)
// gafqD_pcall(gafq_State * L, Pfunc func, void * u, ptrdiff_t old_top, ptrdiff_t ef) (\root\gafq\gdo.c:464)
// gafq_cpcall(gafq_State * L, gafq_CFunction func, void * ud) (\root\gafq\gapi.c:857)
// main(int argc, char ** argv) (\root\gafq\gafq.c:388)
static int pmain (gafq_State *L) {
  struct Smain *s = (struct Smain *)gafq_touserdata(L, 1);  // 传进来的启动命令文件参数
  char **argv = s->argv;
  int script;
  int has_i = 0, has_v = 0, has_e = 0;
  globalL = L;
  if (argv[0] && argv[0][0]) progname = argv[0];
  gafq_gc(L, GAFQ_GCSTOP, 0);  /* stop collector during initialization */
  gafqL_openlibs(L);  /* open libraries */ // 好像是初始化一些调用函数啥的
  gafq_gc(L, GAFQ_GCRESTART, 0);
  s->status = handle_gafqinit(L);
  if (s->status != 0) return 0;
  // has_v 版本号
  script = collectargs(argv, &has_i, &has_v, &has_e);
  if (script < 0) {  /* invalid args? */
    print_usage();
    s->status = 1;
    return 0;
  }
  if (has_v) print_version();
  s->status = runargs(L, argv, (script > 0) ? script : s->argc);
  if (s->status != 0) return 0;
  if (script)
    s->status = handle_script(L, argv, script); // 这里执行文件内容
  if (s->status != 0) return 0;
  if (has_i)
    dotty(L);
  else if (script == 0 && !has_e && !has_v) {
    if (gafq_stdin_is_tty()) {
      print_version();
      dotty(L);
    }
    else dofile(L, NULL);  /* executes stdin as a file */
  }
  return 0;
}

//入口
int main (int argc, char **argv) {
  int status;
  // 存命令
  struct Smain s;
  gafq_State *L = gafq_open();  /* create state */
  if (L == NULL) {
    l_message(argv[0], "cannot create state: not enough memory");
    return EXIT_FAILURE;
  }
  s.argc = argc;
  s.argv = argv;
  status = gafq_cpcall(L, &pmain, &s);
  report(L, status);
  gafq_close(L);
  return (status || s.status) ? EXIT_FAILURE : EXIT_SUCCESS;
}

