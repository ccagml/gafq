/*
** $Id: gcode.h,v 1.48.1.1 2007/12/27 13:02:25 roberto Exp $
** Code generator for Gafq
** See Copyright Notice in gafq.h
*/

#ifndef gcode_h
#define gcode_h

#include "glex.h"
#include "gobject.h"
#include "gopcodes.h"
#include "lparser.h"


/*
** Marks the end of a patch list. It is an invalid value both as an absolute
** address, and as a list link (would link an element to itself).
*/
#define NO_JUMP (-1)


/*
** grep "ORDER OPR" if you change these enums
*/
typedef enum BinOpr {
  OPR_ADD, OPR_SUB, OPR_MUL, OPR_DIV, OPR_MOD, OPR_POW,
  OPR_CONCAT,
  OPR_NE, OPR_EQ,
  OPR_LT, OPR_LE, OPR_GT, OPR_GE,
  OPR_AND, OPR_OR,
  OPR_NOBINOPR
} BinOpr;


typedef enum UnOpr { OPR_MINUS, OPR_NOT, OPR_LEN, OPR_NOUNOPR } UnOpr;


#define getcode(fs,e)	((fs)->f->code[(e)->u.s.info])

#define gafqK_codeAsBx(fs,o,A,sBx)	gafqK_codeABx(fs,o,A,(sBx)+MAXARG_sBx)

#define gafqK_setmultret(fs,e)	gafqK_setreturns(fs, e, GAFQ_MULTRET)

GAFQI_FUNC int gafqK_codeABx (FuncState *fs, OpCode o, int A, unsigned int Bx);
GAFQI_FUNC int gafqK_codeABC (FuncState *fs, OpCode o, int A, int B, int C);
GAFQI_FUNC void gafqK_fixline (FuncState *fs, int line);
GAFQI_FUNC void gafqK_nil (FuncState *fs, int from, int n);
GAFQI_FUNC void gafqK_reserveregs (FuncState *fs, int n);
GAFQI_FUNC void gafqK_checkstack (FuncState *fs, int n);
GAFQI_FUNC int gafqK_stringK (FuncState *fs, TString *s);
GAFQI_FUNC int gafqK_numberK (FuncState *fs, gafq_Number r);
GAFQI_FUNC void gafqK_dischargevars (FuncState *fs, expdesc *e);
GAFQI_FUNC int gafqK_exp2anyreg (FuncState *fs, expdesc *e);
GAFQI_FUNC void gafqK_exp2nextreg (FuncState *fs, expdesc *e);
GAFQI_FUNC void gafqK_exp2val (FuncState *fs, expdesc *e);
GAFQI_FUNC int gafqK_exp2RK (FuncState *fs, expdesc *e);
GAFQI_FUNC void gafqK_self (FuncState *fs, expdesc *e, expdesc *key);
GAFQI_FUNC void gafqK_indexed (FuncState *fs, expdesc *t, expdesc *k);
GAFQI_FUNC void gafqK_goiftrue (FuncState *fs, expdesc *e);
GAFQI_FUNC void gafqK_storevar (FuncState *fs, expdesc *var, expdesc *e);
GAFQI_FUNC void gafqK_setreturns (FuncState *fs, expdesc *e, int nresults);
GAFQI_FUNC void gafqK_setoneret (FuncState *fs, expdesc *e);
GAFQI_FUNC int gafqK_jump (FuncState *fs);
GAFQI_FUNC void gafqK_ret (FuncState *fs, int first, int nret);
GAFQI_FUNC void gafqK_patchlist (FuncState *fs, int list, int target);
GAFQI_FUNC void gafqK_patchtohere (FuncState *fs, int list);
GAFQI_FUNC void gafqK_concat (FuncState *fs, int *l1, int l2);
GAFQI_FUNC int gafqK_getlabel (FuncState *fs);
GAFQI_FUNC void gafqK_prefix (FuncState *fs, UnOpr op, expdesc *v);
GAFQI_FUNC void gafqK_infix (FuncState *fs, BinOpr op, expdesc *v);
GAFQI_FUNC void gafqK_posfix (FuncState *fs, BinOpr op, expdesc *v1, expdesc *v2);
GAFQI_FUNC void gafqK_setlist (FuncState *fs, int base, int nelems, int tostore);


#endif
