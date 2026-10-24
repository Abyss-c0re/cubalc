/* CubalC lang — lang_ops_dual_imm_npred.c
 * DUAL imm 4-bit sign/zero preds: DSIGN4N·D0EQ4N·D0NE4N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_npred.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_npred(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  if (kw(&L->cur,"DSIGN4N")||kw(&L->cur,"S2SIGN4N")||kw(&L->cur,"STACK2SIGN4N")||
      kw(&L->cur,"PAIRSIGN4N")||kw(&L->cur,"DSIGNUM4N")||kw(&L->cur,"DSGN4N")||
      kw(&L->cur,"DSIGN4IMM")){
    /* a b + n → nibble n of each = sgn(int4): +1 / 0 / 0xF; n 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul); if (va & 0x8) va -= 16;
    long vb = (long)((mb >> sh) & 0xFul); if (vb & 0x8) vb -= 16;
    unsigned long wa = (va > 0) ? 1ul : ((va < 0) ? 0xFul : 0ul);
    unsigned long wb = (vb > 0) ? 1ul : ((vb < 0) ? 0xFul : 0ul);
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack imm 4-bit field zero-eq: D0EQ4N (completes field zero-eq all widths dual) */
  if (kw(&L->cur,"D0EQ4N")||kw(&L->cur,"S20EQ4N")||kw(&L->cur,"STACK20EQ4N")||
      kw(&L->cur,"PAIR0EQ4N")||kw(&L->cur,"DZ4N")||kw(&L->cur,"DEQZ4N")||
      kw(&L->cur,"D0EQ4IMM")){
    /* a b + n → nibble n of each = (nib == 0) ? 1 : 0; n 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long wa = (va == 0) ? 1ul : 0ul;
    unsigned long wb = (vb == 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack imm 4-bit field nonzero: D0NE4N (completes field nonzero all widths dual) */
  if (kw(&L->cur,"D0NE4N")||kw(&L->cur,"S20NE4N")||kw(&L->cur,"STACK20NE4N")||
      kw(&L->cur,"PAIR0NE4N")||kw(&L->cur,"DNZ4N")||kw(&L->cur,"DNEZ4N")||
      kw(&L->cur,"D0NE4IMM")){
    /* a b + n → nibble n of each = (nib != 0) ? 1 : 0; n 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long wa = (va != 0) ? 1ul : 0ul;
    unsigned long wb = (vb != 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
