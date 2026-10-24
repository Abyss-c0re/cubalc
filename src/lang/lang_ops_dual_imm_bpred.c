/* CubalC lang — lang_ops_dual_imm_bpred.c
 * DUAL imm 8-bit sign/zero preds: DSIGN8N·D0EQ8N·D0NE8N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_bpred.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_bpred(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-7 dual-stack imm 8-bit field signed signum: DSIGN8N (dual of SSIGN8N) */
  if (kw(&L->cur,"DSIGN8N")||kw(&L->cur,"S2SIGN8N")||kw(&L->cur,"STACK2SIGN8N")||
      kw(&L->cur,"PAIRSIGN8N")||kw(&L->cur,"DSIGNUM8N")||kw(&L->cur,"DSGN8N")||
      kw(&L->cur,"DSIGN8IMM")){
    /* a b + n → byte n of each = sgn(int8): +1 / 0 / 0xFF; n 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul); if (va & 0x80) va -= 256;
    long vb = (long)((mb >> sh) & 0xFFul); if (vb & 0x80) vb -= 256;
    unsigned long wa = (va > 0) ? 1ul : ((va < 0) ? 0xFFul : 0ul);
    unsigned long wb = (vb > 0) ? 1ul : ((vb < 0) ? 0xFFul : 0ul);
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack imm 8-bit field zero-eq: D0EQ8N (dual of S0EQ8N) */
  if (kw(&L->cur,"D0EQ8N")||kw(&L->cur,"S20EQ8N")||kw(&L->cur,"STACK20EQ8N")||
      kw(&L->cur,"PAIR0EQ8N")||kw(&L->cur,"DZ8N")||kw(&L->cur,"DEQZ8N")||
      kw(&L->cur,"D0EQ8IMM")){
    /* a b + n → byte n of each = (b == 0) ? 1 : 0; n 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFul;
    unsigned long vb = (mb >> sh) & 0xFFul;
    unsigned long wa = (va == 0) ? 1ul : 0ul;
    unsigned long wb = (vb == 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack imm 8-bit field nonzero: D0NE8N (dual of S0NE8N) */
  if (kw(&L->cur,"D0NE8N")||kw(&L->cur,"S20NE8N")||kw(&L->cur,"STACK20NE8N")||
      kw(&L->cur,"PAIR0NE8N")||kw(&L->cur,"DNZ8N")||kw(&L->cur,"DNEZ8N")||
      kw(&L->cur,"D0NE8IMM")){
    /* a b + n → byte n of each = (b != 0) ? 1 : 0; n 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long va = (ma >> sh) & 0xFFul;
    unsigned long vb = (mb >> sh) & 0xFFul;
    unsigned long wa = (va != 0) ? 1ul : 0ul;
    unsigned long wb = (vb != 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
