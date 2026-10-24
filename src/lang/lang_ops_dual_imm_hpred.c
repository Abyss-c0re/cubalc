/* CubalC lang — lang_ops_dual_imm_hpred.c
 * DUAL imm 16-bit sign/zero preds: DSIGN16N·D0EQ16N·D0NE16N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_hpred.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_hpred(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-0 dual-stack imm 16-bit field signed signum: DSIGN16N (dual of SSIGN16N) */
  if (kw(&L->cur,"DSIGN16N")||kw(&L->cur,"S2SIGN16N")||kw(&L->cur,"STACK2SIGN16N")||
      kw(&L->cur,"PAIRSIGN16N")||kw(&L->cur,"DSIGNUM16N")||kw(&L->cur,"DSGN16N")||
      kw(&L->cur,"DSIGN16IMM")){
    /* a b + n → halfword n of each = sgn(int16): +1 / 0 / 0xFFFF; n 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul); if (va & 0x8000) va -= 65536;
    long vb = (long)((mb >> sh) & 0xFFFFul); if (vb & 0x8000) vb -= 65536;
    unsigned long wa = (va > 0) ? 1ul : ((va < 0) ? 0xFFFFul : 0ul);
    unsigned long wb = (vb > 0) ? 1ul : ((vb < 0) ? 0xFFFFul : 0ul);
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 dual-stack imm 16-bit field zero-eq: D0EQ16N (dual of S0EQ16N) */
  if (kw(&L->cur,"D0EQ16N")||kw(&L->cur,"S20EQ16N")||kw(&L->cur,"STACK20EQ16N")||
      kw(&L->cur,"PAIR0EQ16N")||kw(&L->cur,"DZ16N")||kw(&L->cur,"DEQZ16N")||
      kw(&L->cur,"D0EQ16IMM")){
    /* a b + n → halfword n of each = (hw == 0) ? 1 : 0; n 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFul;
    unsigned long wa = (va == 0) ? 1ul : 0ul;
    unsigned long wb = (vb == 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack imm 16-bit field nonzero: D0NE16N (dual of S0NE16N) */
  if (kw(&L->cur,"D0NE16N")||kw(&L->cur,"S20NE16N")||kw(&L->cur,"STACK20NE16N")||
      kw(&L->cur,"PAIR0NE16N")||kw(&L->cur,"DNZ16N")||kw(&L->cur,"DNEZ16N")||
      kw(&L->cur,"D0NE16IMM")){
    /* a b + n → halfword n of each = (hw != 0) ? 1 : 0; n 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFFFul;
    unsigned long vb = (mb >> sh) & 0xFFFFul;
    unsigned long wa = (va != 0) ? 1ul : 0ul;
    unsigned long wb = (vb != 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
