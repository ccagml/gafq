/*
** $Id: gstrlib.c,v 1.132.1.5 2010/05/14 15:34:19 roberto Exp $
** Standard library for string operations and pattern-matching
** See Copyright Notice in gafq.h
*/


#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define gstrlib_c
#define GAFQ_LIB

#include "gafq.h"

#include "gauxlib.h"
#include "gafqlib.h"


/* macro to `unsign' a character */
#define uchar(c)        ((unsigned char)(c))


// 获取字符串长度
static int str_len (gafq_State *L) {
  size_t l;
  gafqL_checkgstring(L, 1, &l);
  gafq_pushinteger(L, l);
  return 1;
}

// ptrdiff_t 是预定义的内容好像跟gcc有关系 https://stackoverflow.com/questions/38032035/ptrdiff-type-vs-ptrdiff-t
static ptrdiff_t posrelat (ptrdiff_t pos, size_t len) {
  /* relative string position: negative means back from end */
  if (pos < 0) pos += (ptrdiff_t)len + 1;
  return (pos >= 0) ? pos : 0;
}

//这个截取字符串
static int str_sub (gafq_State *L) {
  size_t l;
  // 取出字符串， l是字符串长度
  const char *s = gafqL_checkgstring(L, 1, &l);
  ptrdiff_t start = posrelat(gafqL_checkinteger(L, 2), l);
  ptrdiff_t end = posrelat(gafqL_optinteger(L, 3, -1), l);
  if (start < 1) start = 1;
  if (end > (ptrdiff_t)l) end = (ptrdiff_t)l;
  if (start <= end)
    gafq_pushgstring(L, s+start-1, end-start+1);
  else gafq_pushliteral(L, "");
  return 1;
}

//反转字符串
static int str_reverse (gafq_State *L) {
  size_t l;
  gafqL_Buffer b;
  const char *s = gafqL_checkgstring(L, 1, &l);
  gafqL_buffinit(L, &b);
  while (l--) gafqL_addchar(&b, s[l]);
  gafqL_pushresult(&b);
  return 1;
}

//转成消协
static int str_lower (gafq_State *L) {
  size_t l;
  size_t i;
  gafqL_Buffer b;
  const char *s = gafqL_checkgstring(L, 1, &l);
  gafqL_buffinit(L, &b);
  for (i=0; i<l; i++)
    gafqL_addchar(&b, tolower(uchar(s[i])));
  gafqL_pushresult(&b);
  return 1;
}

//转成大写
static int str_upper (gafq_State *L) {
  size_t l;
  size_t i;
  gafqL_Buffer b;
  const char *s = gafqL_checkgstring(L, 1, &l);
  gafqL_buffinit(L, &b);
  for (i=0; i<l; i++)
    gafqL_addchar(&b, toupper(uchar(s[i])));
  gafqL_pushresult(&b);
  return 1;
}

//看着是复制n次字符串
static int str_rep (gafq_State *L) {
  size_t l;
  gafqL_Buffer b;
  const char *s = gafqL_checkgstring(L, 1, &l);
  int n = gafqL_checkint(L, 2);
  gafqL_buffinit(L, &b);
  while (n-- > 0)
    gafqL_addgstring(&b, s, l);
  gafqL_pushresult(&b);
  return 1;
}

//返回char数字，可以有2个参数范围
static int str_byte (gafq_State *L) {
  size_t l;
  const char *s = gafqL_checkgstring(L, 1, &l);
  ptrdiff_t posi = posrelat(gafqL_optinteger(L, 2, 1), l);
  ptrdiff_t pose = posrelat(gafqL_optinteger(L, 3, posi), l);
  int n, i;
  if (posi <= 0) posi = 1;
  if ((size_t)pose > l) pose = l;
  if (posi > pose) return 0;  /* empty interval; return no values */
  n = (int)(pose -  posi + 1);
  if (posi + n <= pose)  /* overflow? */
    gafqL_error(L, "string slice too long");
  gafqL_checkstack(L, n, "string slice too long");
  for (i=0; i<n; i++)
    gafq_pushinteger(L, uchar(s[posi+i-1]));
  return n;
}

//取n个参数，把参数个数的整数转成字符
static int str_char (gafq_State *L) {
  int n = gafq_gettop(L);  /* number of arguments */
  int i;
  gafqL_Buffer b;
  gafqL_buffinit(L, &b);
  for (i=1; i<=n; i++) {
    int c = gafqL_checkint(L, i);
    gafqL_argcheck(L, uchar(c) == c, i, "invalid value");
    gafqL_addchar(&b, uchar(c));
  }
  gafqL_pushresult(&b);
  return 1;
}

//看着是把b写到B字符流里面
static int writer (gafq_State *L, const void* b, size_t size, void* B) {
  (void)L;
  gafqL_addgstring((gafqL_Buffer*) B, (const char *)b, size);
  return 0;
}

//看着是拷贝，？是不是递归需要？
static int str_dump (gafq_State *L) {
  gafqL_Buffer b;
  gafqL_checktype(L, 1, GAFQ_TFUNCTION);
  gafq_settop(L, 1);
  gafqL_buffinit(L,&b);
  if (gafq_dump(L, writer, &b) != 0)
    gafqL_error(L, "unable to dump given function");
  gafqL_pushresult(&b);
  return 1;
}



/*
** {======================================================
** PATTERN MATCHING
** =======================================================
*/


#define CAP_UNFINISHED	(-1)
#define CAP_POSITION	(-2)

typedef struct MatchState {
  const char *src_init;  /* init of source string */
  const char *src_end;  /* end (`\0') of source string */
  gafq_State *L;
  int level;  /* total number of captures (finished or unfinished) */
  struct {
    const char *init;
    ptrdiff_t len;
  } capture[GAFQ_MAXCAPTURES];
} MatchState;


#define L_ESC		'%'
#define SPECIALS	"^$*+?.([%-"


static int check_capture (MatchState *ms, int l) {
  l -= '1';
  if (l < 0 || l >= ms->level || ms->capture[l].len == CAP_UNFINISHED)
    return gafqL_error(ms->L, "invalid capture index");
  return l;
}


static int capture_to_close (MatchState *ms) {
  int level = ms->level;
  for (level--; level>=0; level--)
    if (ms->capture[level].len == CAP_UNFINISHED) return level;
  return gafqL_error(ms->L, "invalid pattern capture");
}


static const char *classend (MatchState *ms, const char *p) {
  switch (*p++) {
    case L_ESC: {
      if (*p == '\0')
        gafqL_error(ms->L, "malformed pattern (ends with " GAFQ_QL("%%") ")");
      return p+1;
    }
    case '[': {
      if (*p == '^') p++;
      do {  /* look for a `]' */
        if (*p == '\0')
          gafqL_error(ms->L, "malformed pattern (missing " GAFQ_QL("]") ")");
        if (*(p++) == L_ESC && *p != '\0')
          p++;  /* skip escapes (e.g. `%]') */
      } while (*p != ']');
      return p+1;
    }
    default: {
      return p;
    }
  }
}


static int match_class (int c, int cl) {
  int res;
  switch (tolower(cl)) {
    case 'a' : res = isalpha(c); break;
    case 'c' : res = iscntrl(c); break;
    case 'd' : res = isdigit(c); break;
    case 'l' : res = islower(c); break;
    case 'p' : res = ispunct(c); break;
    case 's' : res = isspace(c); break;
    case 'u' : res = isupper(c); break;
    case 'w' : res = isalnum(c); break;
    case 'x' : res = isxdigit(c); break;
    case 'z' : res = (c == 0); break;
    default: return (cl == c);
  }
  return (islower(cl) ? res : !res);
}


static int matchbracketclass (int c, const char *p, const char *ec) {
  int sig = 1;
  if (*(p+1) == '^') {
    sig = 0;
    p++;  /* skip the `^' */
  }
  while (++p < ec) {
    if (*p == L_ESC) {
      p++;
      if (match_class(c, uchar(*p)))
        return sig;
    }
    else if ((*(p+1) == '-') && (p+2 < ec)) {
      p+=2;
      if (uchar(*(p-2)) <= c && c <= uchar(*p))
        return sig;
    }
    else if (uchar(*p) == c) return sig;
  }
  return !sig;
}


static int singlematch (int c, const char *p, const char *ep) {
  switch (*p) {
    case '.': return 1;  /* matches any char */
    case L_ESC: return match_class(c, uchar(*(p+1)));
    case '[': return matchbracketclass(c, p, ep-1);
    default:  return (uchar(*p) == c);
  }
}


static const char *match (MatchState *ms, const char *s, const char *p);


static const char *matchbalance (MatchState *ms, const char *s,
                                   const char *p) {
  if (*p == 0 || *(p+1) == 0)
    gafqL_error(ms->L, "unbalanced pattern");
  if (*s != *p) return NULL;
  else {
    int b = *p;
    int e = *(p+1);
    int cont = 1;
    while (++s < ms->src_end) {
      if (*s == e) {
        if (--cont == 0) return s+1;
      }
      else if (*s == b) cont++;
    }
  }
  return NULL;  /* string ends out of balance */
}


static const char *max_expand (MatchState *ms, const char *s,
                                 const char *p, const char *ep) {
  ptrdiff_t i = 0;  /* counts maximum expand for item */
  while ((s+i)<ms->src_end && singlematch(uchar(*(s+i)), p, ep))
    i++;
  /* keeps trying to match with the maximum repetitions */
  while (i>=0) {
    const char *res = match(ms, (s+i), ep+1);
    if (res) return res;
    i--;  /* else didn't match; reduce 1 repetition to try again */
  }
  return NULL;
}


static const char *min_expand (MatchState *ms, const char *s,
                                 const char *p, const char *ep) {
  for (;;) {
    const char *res = match(ms, s, ep+1);
    if (res != NULL)
      return res;
    else if (s<ms->src_end && singlematch(uchar(*s), p, ep))
      s++;  /* try with one more repetition */
    else return NULL;
  }
}


static const char *start_capture (MatchState *ms, const char *s,
                                    const char *p, int what) {
  const char *res;
  int level = ms->level;
  if (level >= GAFQ_MAXCAPTURES) gafqL_error(ms->L, "too many captures");
  ms->capture[level].init = s;
  ms->capture[level].len = what;
  ms->level = level+1;
  if ((res=match(ms, s, p)) == NULL)  /* match failed? */
    ms->level--;  /* undo capture */
  return res;
}


static const char *end_capture (MatchState *ms, const char *s,
                                  const char *p) {
  int l = capture_to_close(ms);
  const char *res;
  ms->capture[l].len = s - ms->capture[l].init;  /* close capture */
  if ((res = match(ms, s, p)) == NULL)  /* match failed? */
    ms->capture[l].len = CAP_UNFINISHED;  /* undo capture */
  return res;
}


static const char *match_capture (MatchState *ms, const char *s, int l) {
  size_t len;
  l = check_capture(ms, l);
  len = ms->capture[l].len;
  if ((size_t)(ms->src_end-s) >= len &&
      memcmp(ms->capture[l].init, s, len) == 0)
    return s+len;
  else return NULL;
}


static const char *match (MatchState *ms, const char *s, const char *p) {
  init: /* using goto's to optimize tail recursion */
  switch (*p) {
    case '(': {  /* start capture */
      if (*(p+1) == ')')  /* position capture? */
        return start_capture(ms, s, p+2, CAP_POSITION);
      else
        return start_capture(ms, s, p+1, CAP_UNFINISHED);
    }
    case ')': {  /* end capture */
      return end_capture(ms, s, p+1);
    }
    case L_ESC: {
      switch (*(p+1)) {
        case 'b': {  /* balanced string? */
          s = matchbalance(ms, s, p+2);
          if (s == NULL) return NULL;
          p+=4; goto init;  /* else return match(ms, s, p+4); */
        }
        case 'f': {  /* frontier? */
          const char *ep; char previous;
          p += 2;
          if (*p != '[')
            gafqL_error(ms->L, "missing " GAFQ_QL("[") " after "
                               GAFQ_QL("%%f") " in pattern");
          ep = classend(ms, p);  /* points to what is next */
          previous = (s == ms->src_init) ? '\0' : *(s-1);
          if (matchbracketclass(uchar(previous), p, ep-1) ||
             !matchbracketclass(uchar(*s), p, ep-1)) return NULL;
          p=ep; goto init;  /* else return match(ms, s, ep); */
        }
        default: {
          if (isdigit(uchar(*(p+1)))) {  /* capture results (%0-%9)? */
            s = match_capture(ms, s, uchar(*(p+1)));
            if (s == NULL) return NULL;
            p+=2; goto init;  /* else return match(ms, s, p+2) */
          }
          goto dflt;  /* case default */
        }
      }
    }
    case '\0': {  /* end of pattern */
      return s;  /* match succeeded */
    }
    case '$': {
      if (*(p+1) == '\0')  /* is the `$' the last char in pattern? */
        return (s == ms->src_end) ? s : NULL;  /* check end of string */
      else goto dflt;
    }
    default: dflt: {  /* it is a pattern item */
      const char *ep = classend(ms, p);  /* points to what is next */
      int m = s<ms->src_end && singlematch(uchar(*s), p, ep);
      switch (*ep) {
        case '?': {  /* optional */
          const char *res;
          if (m && ((res=match(ms, s+1, ep+1)) != NULL))
            return res;
          p=ep+1; goto init;  /* else return match(ms, s, ep+1); */
        }
        case '*': {  /* 0 or more repetitions */
          return max_expand(ms, s, p, ep);
        }
        case '+': {  /* 1 or more repetitions */
          return (m ? max_expand(ms, s+1, p, ep) : NULL);
        }
        case '-': {  /* 0 or more repetitions (minimum) */
          return min_expand(ms, s, p, ep);
        }
        default: {
          if (!m) return NULL;
          s++; p=ep; goto init;  /* else return match(ms, s+1, ep); */
        }
      }
    }
  }
}



static const char *gmemfind (const char *s1, size_t l1,
                               const char *s2, size_t l2) {
  if (l2 == 0) return s1;  /* empty strings are everywhere */
  else if (l2 > l1) return NULL;  /* avoids a negative `l1' */
  else {
    const char *init;  /* to search for a `*s2' inside `s1' */
    l2--;  /* 1st char will be checked by `memchr' */
    l1 = l1-l2;  /* `s2' cannot be found after that */
    while (l1 > 0 && (init = (const char *)memchr(s1, *s2, l1)) != NULL) {
      init++;   /* 1st char is already checked */
      if (memcmp(init, s2+1, l2) == 0)
        return init-1;
      else {  /* correct `l1' and `s1' to try again */
        l1 -= init-s1;
        s1 = init;
      }
    }
    return NULL;  /* not found */
  }
}


static void push_onecapture (MatchState *ms, int i, const char *s,
                                                    const char *e) {
  if (i >= ms->level) {
    if (i == 0)  /* ms->level == 0, too */
      gafq_pushgstring(ms->L, s, e - s);  /* add whole match */
    else
      gafqL_error(ms->L, "invalid capture index");
  }
  else {
    ptrdiff_t l = ms->capture[i].len;
    if (l == CAP_UNFINISHED) gafqL_error(ms->L, "unfinished capture");
    if (l == CAP_POSITION)
      gafq_pushinteger(ms->L, ms->capture[i].init - ms->src_init + 1);
    else
      gafq_pushgstring(ms->L, ms->capture[i].init, l);
  }
}


static int push_captures (MatchState *ms, const char *s, const char *e) {
  int i;
  int nlevels = (ms->level == 0 && s) ? 1 : ms->level;
  gafqL_checkstack(ms->L, nlevels, "too many captures");
  for (i = 0; i < nlevels; i++)
    push_onecapture(ms, i, s, e);
  return nlevels;  /* number of strings pushed */
}


static int str_find_aux (gafq_State *L, int find) {
  size_t l1, l2;
  const char *s = gafqL_checkgstring(L, 1, &l1);
  const char *p = gafqL_checkgstring(L, 2, &l2);
  ptrdiff_t init = posrelat(gafqL_optinteger(L, 3, 1), l1) - 1;
  if (init < 0) init = 0;
  else if ((size_t)(init) > l1) init = (ptrdiff_t)l1;
  if (find && (gafq_toboolean(L, 4) ||  /* explicit request? */
      strpbrk(p, SPECIALS) == NULL)) {  /* or no special characters? */
    /* do a plain search */
    const char *s2 = gmemfind(s+init, l1-init, p, l2);
    if (s2) {
      gafq_pushinteger(L, s2-s+1);
      gafq_pushinteger(L, s2-s+l2);
      return 2;
    }
  }
  else {
    MatchState ms;
    int anchor = (*p == '^') ? (p++, 1) : 0;
    const char *s1=s+init;
    ms.L = L;
    ms.src_init = s;
    ms.src_end = s+l1;
    do {
      const char *res;
      ms.level = 0;
      if ((res=match(&ms, s1, p)) != NULL) {
        if (find) {
          gafq_pushinteger(L, s1-s+1);  /* start */
          gafq_pushinteger(L, res-s);   /* end */
          return push_captures(&ms, NULL, 0) + 2;
        }
        else
          return push_captures(&ms, s1, res);
      }
    } while (s1++ < ms.src_end && !anchor);
  }
  gafq_pushnil(L);  /* not found */
  return 1;
}


static int str_find (gafq_State *L) {
  return str_find_aux(L, 1);
}


static int str_match (gafq_State *L) {
  return str_find_aux(L, 0);
}


static int gmatch_aux (gafq_State *L) {
  MatchState ms;
  size_t ls;
  const char *s = gafq_togstring(L, gafq_upvalueindex(1), &ls);
  const char *p = gafq_tostring(L, gafq_upvalueindex(2));
  const char *src;
  ms.L = L;
  ms.src_init = s;
  ms.src_end = s+ls;
  for (src = s + (size_t)gafq_tointeger(L, gafq_upvalueindex(3));
       src <= ms.src_end;
       src++) {
    const char *e;
    ms.level = 0;
    if ((e = match(&ms, src, p)) != NULL) {
      gafq_Integer newstart = e-s;
      if (e == src) newstart++;  /* empty match? go at least one position */
      gafq_pushinteger(L, newstart);
      gafq_replace(L, gafq_upvalueindex(3));
      return push_captures(&ms, src, e);
    }
  }
  return 0;  /* not found */
}


static int gmatch (gafq_State *L) {
  gafqL_checkstring(L, 1);
  gafqL_checkstring(L, 2);
  gafq_settop(L, 2);
  gafq_pushinteger(L, 0);
  gafq_pushcclosure(L, gmatch_aux, 3);
  return 1;
}


static int gfind_nodef (gafq_State *L) {
  return gafqL_error(L, GAFQ_QL("string.gfind") " was renamed to "
                       GAFQ_QL("string.gmatch"));
}


static void add_s (MatchState *ms, gafqL_Buffer *b, const char *s,
                                                   const char *e) {
  size_t l, i;
  const char *news = gafq_togstring(ms->L, 3, &l);
  for (i = 0; i < l; i++) {
    if (news[i] != L_ESC)
      gafqL_addchar(b, news[i]);
    else {
      i++;  /* skip ESC */
      if (!isdigit(uchar(news[i])))
        gafqL_addchar(b, news[i]);
      else if (news[i] == '0')
          gafqL_addgstring(b, s, e - s);
      else {
        push_onecapture(ms, news[i] - '1', s, e);
        gafqL_addvalue(b);  /* add capture to accumulated result */
      }
    }
  }
}


static void add_value (MatchState *ms, gafqL_Buffer *b, const char *s,
                                                       const char *e) {
  gafq_State *L = ms->L;
  switch (gafq_type(L, 3)) {
    case GAFQ_TNUMBER:
    case GAFQ_TSTRING: {
      add_s(ms, b, s, e);
      return;
    }
    case GAFQ_TFUNCTION: {
      int n;
      gafq_pushvalue(L, 3);
      n = push_captures(ms, s, e);
      gafq_call(L, n, 1);
      break;
    }
    case GAFQ_TTABLE: {
      push_onecapture(ms, 0, s, e);
      gafq_gettable(L, 3);
      break;
    }
  }
  if (!gafq_toboolean(L, -1)) {  /* nil or false? */
    gafq_pop(L, 1);
    gafq_pushgstring(L, s, e - s);  /* keep original text */
  }
  else if (!gafq_isstring(L, -1))
    gafqL_error(L, "invalid replacement value (a %s)", gafqL_typename(L, -1)); 
  gafqL_addvalue(b);  /* add result to accumulator */
}


static int str_gsub (gafq_State *L) {
  size_t srcl;
  const char *src = gafqL_checkgstring(L, 1, &srcl);
  const char *p = gafqL_checkstring(L, 2);
  int  tr = gafq_type(L, 3);
  int max_s = gafqL_optint(L, 4, srcl+1);
  int anchor = (*p == '^') ? (p++, 1) : 0;
  int n = 0;
  MatchState ms;
  gafqL_Buffer b;
  gafqL_argcheck(L, tr == GAFQ_TNUMBER || tr == GAFQ_TSTRING ||
                   tr == GAFQ_TFUNCTION || tr == GAFQ_TTABLE, 3,
                      "string/function/table expected");
  gafqL_buffinit(L, &b);
  ms.L = L;
  ms.src_init = src;
  ms.src_end = src+srcl;
  while (n < max_s) {
    const char *e;
    ms.level = 0;
    e = match(&ms, src, p);
    if (e) {
      n++;
      add_value(&ms, &b, src, e);
    }
    if (e && e>src) /* non empty match? */
      src = e;  /* skip it */
    else if (src < ms.src_end)
      gafqL_addchar(&b, *src++);
    else break;
    if (anchor) break;
  }
  gafqL_addgstring(&b, src, ms.src_end-src);
  gafqL_pushresult(&b);
  gafq_pushinteger(L, n);  /* number of substitutions */
  return 2;
}

/* }====================================================== */


/* maximum size of each formatted item (> len(format('%99.99f', -1e308))) */
#define MAX_ITEM	512
/* valid flags in a format specification */
#define FLAGS	"-+ #0"
/*
** maximum size of each format specification (such as '%-099.99d')
** (+10 accounts for %99.99x plus margin of error)
*/
#define MAX_FORMAT	(sizeof(FLAGS) + sizeof(GAFQ_INTFRMLEN) + 10)


static void addquoted (gafq_State *L, gafqL_Buffer *b, int arg) {
  size_t l;
  const char *s = gafqL_checkgstring(L, arg, &l);
  gafqL_addchar(b, '"');
  while (l--) {
    switch (*s) {
      case '"': case '\\': case '\n': {
        gafqL_addchar(b, '\\');
        gafqL_addchar(b, *s);
        break;
      }
      case '\r': {
        gafqL_addgstring(b, "\\r", 2);
        break;
      }
      case '\0': {
        gafqL_addgstring(b, "\\000", 4);
        break;
      }
      default: {
        gafqL_addchar(b, *s);
        break;
      }
    }
    s++;
  }
  gafqL_addchar(b, '"');
}

static const char *scanformat (gafq_State *L, const char *strfrmt, char *form) {
  const char *p = strfrmt;
  while (*p != '\0' && strchr(FLAGS, *p) != NULL) p++;  /* skip flags */
  if ((size_t)(p - strfrmt) >= sizeof(FLAGS))
    gafqL_error(L, "invalid format (repeated flags)");
  if (isdigit(uchar(*p))) p++;  /* skip width */
  if (isdigit(uchar(*p))) p++;  /* (2 digits at most) */
  if (*p == '.') {
    p++;
    if (isdigit(uchar(*p))) p++;  /* skip precision */
    if (isdigit(uchar(*p))) p++;  /* (2 digits at most) */
  }
  if (isdigit(uchar(*p)))
    gafqL_error(L, "invalid format (width or precision too long)");
  *(form++) = '%';
  strncpy(form, strfrmt, p - strfrmt + 1);
  form += p - strfrmt + 1;
  *form = '\0';
  return p;
}


static void addintlen (char *form) {
  size_t l = strlen(form);
  char spec = form[l - 1];
  strcpy(form + l - 1, GAFQ_INTFRMLEN);
  form[l + sizeof(GAFQ_INTFRMLEN) - 2] = spec;
  form[l + sizeof(GAFQ_INTFRMLEN) - 1] = '\0';
}


static int str_format (gafq_State *L) {
  int top = gafq_gettop(L);
  int arg = 1;
  size_t sfl;
  const char *strfrmt = gafqL_checkgstring(L, arg, &sfl);
  const char *strfrmt_end = strfrmt+sfl;
  gafqL_Buffer b;
  gafqL_buffinit(L, &b);
  while (strfrmt < strfrmt_end) {
    if (*strfrmt != L_ESC)
      gafqL_addchar(&b, *strfrmt++);
    else if (*++strfrmt == L_ESC)
      gafqL_addchar(&b, *strfrmt++);  /* %% */
    else { /* format item */
      char form[MAX_FORMAT];  /* to store the format (`%...') */
      char buff[MAX_ITEM];  /* to store the formatted item */
      if (++arg > top)
        gafqL_argerror(L, arg, "no value");
      strfrmt = scanformat(L, strfrmt, form);
      switch (*strfrmt++) {
        case 'c': {
          sprintf(buff, form, (int)gafqL_checknumber(L, arg));
          break;
        }
        case 'd':  case 'i': {
          addintlen(form);
          sprintf(buff, form, (GAFQ_INTFRM_T)gafqL_checknumber(L, arg));
          break;
        }
        case 'o':  case 'u':  case 'x':  case 'X': {
          addintlen(form);
          sprintf(buff, form, (unsigned GAFQ_INTFRM_T)gafqL_checknumber(L, arg));
          break;
        }
        case 'e':  case 'E': case 'f':
        case 'g': case 'G': {
          sprintf(buff, form, (double)gafqL_checknumber(L, arg));
          break;
        }
        case 'q': {
          addquoted(L, &b, arg);
          continue;  /* skip the 'addsize' at the end */
        }
        case 's': {
          size_t l;
          const char *s = gafqL_checkgstring(L, arg, &l);
          if (!strchr(form, '.') && l >= 100) {
            /* no precision and string is too long to be formatted;
               keep original string */
            gafq_pushvalue(L, arg);
            gafqL_addvalue(&b);
            continue;  /* skip the `addsize' at the end */
          }
          else {
            sprintf(buff, form, s);
            break;
          }
        }
        default: {  /* also treat cases `pnLlh' */
          return gafqL_error(L, "invalid option " GAFQ_QL("%%%c") " to "
                               GAFQ_QL("format"), *(strfrmt - 1));
        }
      }
      gafqL_addgstring(&b, buff, strlen(buff));
    }
  }
  gafqL_pushresult(&b);
  return 1;
}

//注册函数
static const gafqL_Reg strlib[] = {
  {"byte", str_byte},
  {"char", str_char},
  {"dump", str_dump},
  {"find", str_find},
  {"format", str_format},
  {"gfind", gfind_nodef},
  {"gmatch", gmatch},
  {"gsub", str_gsub},
  {"len", str_len},
  {"lower", str_lower},
  {"match", str_match},
  {"rep", str_rep},
  {"reverse", str_reverse},
  {"sub", str_sub},
  {"upper", str_upper},
  {NULL, NULL}
};


static void createmetatable (gafq_State *L) {
  gafq_createtable(L, 0, 1);  /* create metatable for strings */
  gafq_pushliteral(L, "");  /* dummy string */
  gafq_pushvalue(L, -2);
  gafq_setmetatable(L, -2);  /* set string metatable */
  gafq_pop(L, 1);  /* pop dummy string */
  gafq_pushvalue(L, -2);  /* string library... */
  gafq_setfield(L, -2, "__index");  /* ...is the __index metamethod */
  gafq_pop(L, 1);  /* pop metatable */
}


/*
** Open string library
*/
GAFQLIB_API int gafqopen_string (gafq_State *L) {
  gafqL_register(L, GAFQ_STRLIBNAME, strlib);
#if defined(GAFQ_COMPAT_GFIND)
  gafq_getfield(L, -1, "gmatch");
  gafq_setfield(L, -2, "gfind");
#endif
  createmetatable(L);
  return 1;
}

