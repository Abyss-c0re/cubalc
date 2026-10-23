/* CubalC lang — lang_ops_dual_imm_cmp.c
 * DUAL imm compare: DEQN·DLTN·DGTN·DNEN·DLENN·DGENN (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_cmp.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_cmp(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-3 dual-stack immediate compare: DEQN · DLTN · DGTN (dual of SEQN/SLTN/SGTN) */
  if (kw(&L->cur,"DEQN")||kw(&L->cur,"2EQN")||kw(&L->cur,"S2EQN")||
      kw(&L->cur,"STACK2EQN")||kw(&L->cur,"PAIREQN")||kw(&L->cur,"DEQIMM")||
      kw(&L->cur,"2EQIMM")||kw(&L->cur,"PAIREQIMM")){
    /* a b + n → (a==n?1:0) (b==n?1:0) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a == n) ? 1 : 0;
    long y = (b == n) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DLTN")||kw(&L->cur,"2LTN")||kw(&L->cur,"S2LTN")||
      kw(&L->cur,"STACK2LTN")||kw(&L->cur,"PAIRLTN")||kw(&L->cur,"DLTIMM")||
      kw(&L->cur,"2LTIMM")||kw(&L->cur,"PAIRLTIMM")){
    /* a b + n → (a<n?1:0) (b<n?1:0) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a < n) ? 1 : 0;
    long y = (b < n) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGTN")||kw(&L->cur,"2GTN")||kw(&L->cur,"S2GTN")||
      kw(&L->cur,"STACK2GTN")||kw(&L->cur,"PAIRGTN")||kw(&L->cur,"DGTIMM")||
      kw(&L->cur,"2GTIMM")||kw(&L->cur,"PAIRGTIMM")){
    /* a b + n → (a>n?1:0) (b>n?1:0) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a > n) ? 1 : 0;
    long y = (b > n) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack immediate compare ext: DNEN · DLENN · DGENN (dual of SNEN/SLENN/SGENN) */
  if (kw(&L->cur,"DNEN")||kw(&L->cur,"2NEN")||kw(&L->cur,"S2NEN")||
      kw(&L->cur,"STACK2NEN")||kw(&L->cur,"PAIRNEN")||kw(&L->cur,"DNEIMM")||
      kw(&L->cur,"2NEIMM")||kw(&L->cur,"PAIRNEIMM")||kw(&L->cur,"DNEQN")){
    /* a b + n → (a!=n?1:0) (b!=n?1:0) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a != n) ? 1 : 0;
    long y = (b != n) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DLENN")||kw(&L->cur,"2LENN")||kw(&L->cur,"S2LENN")||
      kw(&L->cur,"STACK2LENN")||kw(&L->cur,"PAIRLENN")||kw(&L->cur,"DLEQN")||
      kw(&L->cur,"2LEQN")||kw(&L->cur,"DLEIMM")||kw(&L->cur,"2LEIMM")||
      kw(&L->cur,"PAIRLEQN")){
    /* a b + n → (a<=n?1:0) (b<=n?1:0) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a <= n) ? 1 : 0;
    long y = (b <= n) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGENN")||kw(&L->cur,"2GENN")||kw(&L->cur,"S2GENN")||
      kw(&L->cur,"STACK2GENN")||kw(&L->cur,"PAIRGENN")||kw(&L->cur,"DGEQN")||
      kw(&L->cur,"2GEQN")||kw(&L->cur,"DGEIMM")||kw(&L->cur,"2GEIMM")||
      kw(&L->cur,"PAIRGEQN")){
    /* a b + n → (a>=n?1:0) (b>=n?1:0) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 2];
    long b = vm->stack[vm->sp - 1];
    long x = (a >= n) ? 1 : 0;
    long y = (b >= n) ? 1 : 0;
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* DULTN·DUGTN·DULEN·DUGEN → dual_clip peel (G_dual_margin) */
  return 0;
}
