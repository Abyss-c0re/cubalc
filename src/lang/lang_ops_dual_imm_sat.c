/* CubalC lang — lang_ops_dual_imm_sat.c
 * DUAL imm sat: DSATADDN·DSATSUBN·DSATMULN·DSATDIVN·DSATSUBFROMN·DSATDIVFROMN (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_sat.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_sat(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-3 dual-stack imm saturating ALU: DSATADDN · DSATSUBN · DSATMULN (dual of SSATADDN) */
  if (kw(&L->cur,"DSATADDN")||kw(&L->cur,"S2SATADDN")||kw(&L->cur,"STACK2SATADDN")||
      kw(&L->cur,"PAIRSATADDN")||kw(&L->cur,"DADDSATN")||kw(&L->cur,"PAIRADDSATN")||
      kw(&L->cur,"DSATADDIMM")){
    /* a b + n → sat(a+n) sat(b+n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x, y;
    if (n > 0 && a > LONG_MAX - n) x = LONG_MAX;
    else if (n < 0 && a < LONG_MIN - n) x = LONG_MIN;
    else x = a + n;
    if (n > 0 && b > LONG_MAX - n) y = LONG_MAX;
    else if (n < 0 && b < LONG_MIN - n) y = LONG_MIN;
    else y = b + n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSATSUBN")||kw(&L->cur,"S2SATSUBN")||kw(&L->cur,"STACK2SATSUBN")||
      kw(&L->cur,"PAIRSATSUBN")||kw(&L->cur,"DSUBSATN")||kw(&L->cur,"PAIRSUBSATN")||
      kw(&L->cur,"DSATSUBIMM")){
    /* a b + n → sat(a-n) sat(b-n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x, y;
    if (n > 0 && a < LONG_MIN + n) x = LONG_MIN;
    else if (n < 0 && a > LONG_MAX + n) x = LONG_MAX;
    else x = a - n;
    if (n > 0 && b < LONG_MIN + n) y = LONG_MIN;
    else if (n < 0 && b > LONG_MAX + n) y = LONG_MAX;
    else y = b - n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSATMULN")||kw(&L->cur,"S2SATMULN")||kw(&L->cur,"STACK2SATMULN")||
      kw(&L->cur,"PAIRSATMULN")||kw(&L->cur,"DMULSATN")||kw(&L->cur,"PAIRMULSATN")||
      kw(&L->cur,"DSATMULIMM")){
    /* a b + n → sat(a*n) sat(b*n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x, y;
    if (a == 0 || n == 0) x = 0;
    else {
      __int128 p = (__int128)a * (__int128)n;
      if (p > (__int128)LONG_MAX) x = LONG_MAX;
      else if (p < (__int128)LONG_MIN) x = LONG_MIN;
      else x = (long)p;
    }
    if (b == 0 || n == 0) y = 0;
    else {
      __int128 p = (__int128)b * (__int128)n;
      if (p > (__int128)LONG_MAX) y = LONG_MAX;
      else if (p < (__int128)LONG_MIN) y = LONG_MIN;
      else y = (long)p;
    }
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 dual-stack imm sat div: DSATDIVN (completes sat imm ALU + energy plane) */
  if (kw(&L->cur,"DSATDIVN")||kw(&L->cur,"S2SATDIVN")||kw(&L->cur,"STACK2SATDIVN")||
      kw(&L->cur,"PAIRSATDIVN")||kw(&L->cur,"DDIVSATN")||kw(&L->cur,"PAIRDIVSATN")||
      kw(&L->cur,"DSATDIVIMM")){
    /* a b + n → sat(a/n) sat(b/n); n==0 → 0; LONG_MIN/-1 → LONG_MAX */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x, y;
    if (n == 0) x = 0;
    else if (a == LONG_MIN && n == -1) x = LONG_MAX;
    else x = a / n;
    if (n == 0) y = 0;
    else if (b == LONG_MIN && n == -1) y = LONG_MAX;
    else y = b / n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 dual-stack imm reverse sat: DSATSUBFROMN · DSATDIVFROMN (dual of SSATSUBFROMN plane) */
  if (kw(&L->cur,"DSATSUBFROMN")||kw(&L->cur,"S2SATSUBFROMN")||kw(&L->cur,"STACK2SATSUBFROMN")||
      kw(&L->cur,"PAIRSATSUBFROMN")||kw(&L->cur,"DSATRSUBN")||kw(&L->cur,"PAIRSATRSUBN")||
      kw(&L->cur,"DSATSUBFROMIMM")||kw(&L->cur,"DNSATSUBN")){
    /* a b + n → sat(n-a) sat(n-b) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x, y;
    if (a > 0 && n < LONG_MIN + a) x = LONG_MIN;
    else if (a < 0 && n > LONG_MAX + a) x = LONG_MAX;
    else x = n - a;
    if (b > 0 && n < LONG_MIN + b) y = LONG_MIN;
    else if (b < 0 && n > LONG_MAX + b) y = LONG_MAX;
    else y = n - b;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSATDIVFROMN")||kw(&L->cur,"S2SATDIVFROMN")||kw(&L->cur,"STACK2SATDIVFROMN")||
      kw(&L->cur,"PAIRSATDIVFROMN")||kw(&L->cur,"DSATRDIVN")||kw(&L->cur,"PAIRSATRDIVN")||
      kw(&L->cur,"DSATDIVFROMIMM")||kw(&L->cur,"DNSATDIVN")){
    /* a b + n → sat(n/a) sat(n/b); lane0 → 0; LONG_MIN/-1 → LONG_MAX */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x, y;
    if (a == 0) x = 0;
    else if (n == LONG_MIN && a == -1) x = LONG_MAX;
    else x = n / a;
    if (b == 0) y = 0;
    else if (n == LONG_MIN && b == -1) y = LONG_MAX;
    else y = n / b;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
