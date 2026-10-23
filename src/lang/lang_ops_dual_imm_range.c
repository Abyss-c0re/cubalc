/* CubalC lang — lang_ops_dual_imm_range.c
 * DUAL imm range: DMINN·DMAXN·DCLAMPN·DBETWEENN·DWITHINN (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_range.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_range(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-9 dual-stack immediate min/max/clamp: DMINN · DMAXN · DCLAMPN (dual of SMINN/SMAXN) */
  if (kw(&L->cur,"DMINN")||kw(&L->cur,"2MINN")||kw(&L->cur,"S2MINN")||
      kw(&L->cur,"STACK2MINN")||kw(&L->cur,"PAIRMINN")||kw(&L->cur,"DMINIMM")||
      kw(&L->cur,"2MINIMM")||kw(&L->cur,"PAIRMINIMM")){
    /* a b + n → min(a,n) min(b,n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a < n ? a : n;
    long y = b < n ? b : n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMAXN")||kw(&L->cur,"2MAXN")||kw(&L->cur,"S2MAXN")||
      kw(&L->cur,"STACK2MAXN")||kw(&L->cur,"PAIRMAXN")||kw(&L->cur,"DMAXIMM")||
      kw(&L->cur,"2MAXIMM")||kw(&L->cur,"PAIRMAXIMM")){
    /* a b + n → max(a,n) max(b,n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a > n ? a : n;
    long y = b > n ? b : n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* DUMINN·DUMAXN → dual_clip peel (G_dual_margin) */
  if (kw(&L->cur,"DCLAMPN")||kw(&L->cur,"2CLAMPN")||kw(&L->cur,"S2CLAMPN")||
      kw(&L->cur,"STACK2CLAMPN")||kw(&L->cur,"PAIRCLAMPN")||kw(&L->cur,"DCLAMPIMM")||
      kw(&L->cur,"2CLAMPIMM")||kw(&L->cur,"PAIRCLAMPIMM")||kw(&L->cur,"DBOUNDN")){
    /* a b + lo hi → clamp(a,[lo,hi]) clamp(b,[lo,hi]); swap lo/hi if needed */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (lo > hi){ long t=lo; lo=hi; hi=t; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a; if (x < lo) x = lo; if (x > hi) x = hi;
    long y = b; if (y < lo) y = lo; if (y > hi) y = hi;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack imm range preds: DBETWEENN · DWITHINN (dual of SBETWEENN/SWITHINN; complete DCLAMPN) */
  if (kw(&L->cur,"DBETWEENN")||kw(&L->cur,"S2BETWEENN")||kw(&L->cur,"STACK2BETWEENN")||
      kw(&L->cur,"PAIRBETWEENN")||kw(&L->cur,"DINRANGEN")||kw(&L->cur,"PAIRINRANGEN")||
      kw(&L->cur,"DBETWEENIMM")||kw(&L->cur,"PAIRBETWEENIMM")){
    /* a b + lo hi → (a in [lo,hi]?1:0) (b in [lo,hi]?1:0); swap lo/hi if needed */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (lo > hi){ long t=lo; lo=hi; hi=t; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a >= lo && a <= hi) ? 1 : 0;
    long y = (b >= lo && b <= hi) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DWITHINN")||kw(&L->cur,"S2WITHINN")||kw(&L->cur,"STACK2WITHINN")||
      kw(&L->cur,"PAIRWITHINN")||kw(&L->cur,"DINTERVALN")||kw(&L->cur,"PAIRINTERVALN")||
      kw(&L->cur,"DWITHINIMM")||kw(&L->cur,"PAIRWITHINIMM")){
    /* a b + lo hi → (lo<=a<hi?1:0) (lo<=b<hi?1:0); hi exclusive, no lo/hi swap */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a >= lo && a < hi) ? 1 : 0;
    long y = (b >= lo && b < hi) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
