/* CubalC lang — lang_ops_dual_imm_alu.c
 * DUAL imm ALU: DADDN·DSUBN·DMULN (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_alu.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_alu(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-7 dual-stack immediate ALU: DADDN · DSUBN · DMULN (dual of SADDN/SSUBN/SMULN) */
  if (kw(&L->cur,"DADDN")||kw(&L->cur,"2ADDN")||kw(&L->cur,"S2ADDN")||
      kw(&L->cur,"STACK2ADDN")||kw(&L->cur,"PAIRADDN")||kw(&L->cur,"DADDIMM")||
      kw(&L->cur,"2ADDIMM")||kw(&L->cur,"PAIRADDIMM")||kw(&L->cur,"DPLUSN")){
    /* a b + n → (a+n) (b+n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a + n;
    long y = b + n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSUBN")||kw(&L->cur,"2SUBN")||kw(&L->cur,"S2SUBN")||
      kw(&L->cur,"STACK2SUBN")||kw(&L->cur,"PAIRSUBN")||kw(&L->cur,"DSUBIMM")||
      kw(&L->cur,"2SUBIMM")||kw(&L->cur,"PAIRSUBIMM")||kw(&L->cur,"DMINUSN")){
    /* a b + n → (a-n) (b-n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a - n;
    long y = b - n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMULN")||kw(&L->cur,"2MULN")||kw(&L->cur,"S2MULN")||
      kw(&L->cur,"STACK2MULN")||kw(&L->cur,"PAIRMULN")||kw(&L->cur,"DMULIMM")||
      kw(&L->cur,"2MULIMM")||kw(&L->cur,"PAIRMULIMM")||kw(&L->cur,"DTIMESN")){
    /* a b + n → (a*n) (b*n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = a * n;
    long y = b * n;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
