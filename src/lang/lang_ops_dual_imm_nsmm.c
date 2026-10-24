/* CubalC lang — lang_ops_dual_imm_nsmm.c
 * DUAL imm 4-bit signed min/max: DMINS4N·DMAXS4N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_nsmm.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_nsmm(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-4 dual-stack imm 4-bit field signed min/max: DMINS4N · DMAXS4N
   * (dual of SMINS4N/SMAXS4N; nibble ladder of DMINS8N after DLTS4N; completes signed select all widths) */
  if (kw(&L->cur,"DMINS4N")||kw(&L->cur,"S2MINS4N")||kw(&L->cur,"STACK2MINS4N")||
      kw(&L->cur,"PAIRMINS4N")||kw(&L->cur,"DMINSIGN4N")||kw(&L->cur,"DMINS4IMM")||
      kw(&L->cur,"DMINSI4N")){
    /* a b + field n → nibble n of each = min_signed(int4(nib), int4(field)); n 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    long f = (long)((unsigned long)field & 0xFul);
    if (f & 0x8) f -= 16;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul); if (va & 0x8) va -= 16;
    long vb = (long)((mb >> sh) & 0xFul); if (vb & 0x8) vb -= 16;
    long ma_m = (va < f) ? va : f;
    long mb_m = (vb < f) ? vb : f;
    unsigned long wa = (unsigned long)ma_m & 0xFul;
    unsigned long wb = (unsigned long)mb_m & 0xFul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMAXS4N")||kw(&L->cur,"S2MAXS4N")||kw(&L->cur,"STACK2MAXS4N")||
      kw(&L->cur,"PAIRMAXS4N")||kw(&L->cur,"DMAXSIGN4N")||kw(&L->cur,"DMAXS4IMM")||
      kw(&L->cur,"DMAXSI4N")){
    /* a b + field n → nibble n of each = max_signed(int4(nib), int4(field)); n 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long sh = (unsigned long)(n * 4);
    long f = (long)((unsigned long)field & 0xFul);
    if (f & 0x8) f -= 16;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFul); if (va & 0x8) va -= 16;
    long vb = (long)((mb >> sh) & 0xFul); if (vb & 0x8) vb -= 16;
    long ma_m = (va > f) ? va : f;
    long mb_m = (vb > f) ? vb : f;
    unsigned long wa = (unsigned long)ma_m & 0xFul;
    unsigned long wb = (unsigned long)mb_m & 0xFul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
