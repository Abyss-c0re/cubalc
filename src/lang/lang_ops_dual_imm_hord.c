/* CubalC lang — lang_ops_dual_imm_hord.c
 * DUAL imm 16-bit field ordered-cmp: DLT16N·DGT16N·DLTE16N·DGTE16N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_hord.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_hord(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-3 dual-stack imm 16-bit field ordered-cmp: DLT16N · DGT16N · DLTE16N
   * (dual of SLT16N/SGT16N/SLTE16N; unsigned halfword predicates after DEQ16N/DNE16N) */
  if (kw(&L->cur,"DLT16N")||kw(&L->cur,"S2LT16N")||kw(&L->cur,"STACK2LT16N")||
      kw(&L->cur,"PAIRLT16N")||kw(&L->cur,"DCMPLT16N")||kw(&L->cur,"DLTHIMM")||
      kw(&L->cur,"DLTHN16")){
    /* a b + field n → halfword n of each = (hw < field) ? 1 : 0; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFul;
    unsigned long wa = (va < f) ? 1ul : 0ul;
    unsigned long wb = (vb < f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGT16N")||kw(&L->cur,"S2GT16N")||kw(&L->cur,"STACK2GT16N")||
      kw(&L->cur,"PAIRGT16N")||kw(&L->cur,"DCMPGT16N")||kw(&L->cur,"DGTHIMM")||
      kw(&L->cur,"DGTHN16")){
    /* a b + field n → halfword n of each = (hw > field) ? 1 : 0; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFul;
    unsigned long wa = (va > f) ? 1ul : 0ul;
    unsigned long wb = (vb > f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DLTE16N")||kw(&L->cur,"S2LTE16N")||kw(&L->cur,"STACK2LTE16N")||
      kw(&L->cur,"PAIRLTE16N")||kw(&L->cur,"DCMPLE16N")||kw(&L->cur,"DLEQ16N")||
      kw(&L->cur,"DLTEHIMM")||kw(&L->cur,"DLTEHN16")){
    /* a b + field n → halfword n of each = (hw <= field) ? 1 : 0; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFul;
    unsigned long wa = (va <= f) ? 1ul : 0ul;
    unsigned long wb = (vb <= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack imm 16-bit field GTE: DGTE16N (dual of SGTE16N after DLTE16N) */
  if (kw(&L->cur,"DGTE16N")||kw(&L->cur,"S2GTE16N")||kw(&L->cur,"STACK2GTE16N")||
      kw(&L->cur,"PAIRGTE16N")||kw(&L->cur,"DCMPGE16N")||kw(&L->cur,"DGEQ16N")||
      kw(&L->cur,"DGTEHIMM")||kw(&L->cur,"DGTEHN16")){
    /* a b + field n → halfword n of each = (hw >= field) ? 1 : 0; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFul;
    unsigned long wa = (va >= f) ? 1ul : 0ul;
    unsigned long wb = (vb >= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
