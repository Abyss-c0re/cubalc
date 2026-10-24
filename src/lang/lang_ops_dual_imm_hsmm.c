/* CubalC lang — lang_ops_dual_imm_hsmm.c
 * DUAL imm 16-bit signed min/max: DMINS16N·DMAXS16N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_hsmm.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_hsmm(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-8 dual-stack imm 16-bit field signed min/max: DMINS16N · DMAXS16N
   * (dual of SMINS16N/SMAXS16N; halfword ladder of DMINS32N after DLTS16N) */
  if (kw(&L->cur,"DMINS16N")||kw(&L->cur,"S2MINS16N")||kw(&L->cur,"STACK2MINS16N")||
      kw(&L->cur,"PAIRMINS16N")||kw(&L->cur,"DMINSIGN16N")||kw(&L->cur,"DMINS16IMM")||
      kw(&L->cur,"DMINSI16N")){
    /* a b + field n → halfword n of each = min_signed(int16(hw), int16(field)); n 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    long f = (long)((unsigned long)field & 0xFFFFul);
    if (f & 0x8000) f -= 65536;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul); if (va & 0x8000) va -= 65536;
    long vb = (long)((mb >> sh) & 0xFFFFul); if (vb & 0x8000) vb -= 65536;
    long ma_m = (va < f) ? va : f;
    long mb_m = (vb < f) ? vb : f;
    unsigned long wa = (unsigned long)ma_m & 0xFFFFul;
    unsigned long wb = (unsigned long)mb_m & 0xFFFFul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMAXS16N")||kw(&L->cur,"S2MAXS16N")||kw(&L->cur,"STACK2MAXS16N")||
      kw(&L->cur,"PAIRMAXS16N")||kw(&L->cur,"DMAXSIGN16N")||kw(&L->cur,"DMAXS16IMM")||
      kw(&L->cur,"DMAXSI16N")){
    /* a b + field n → halfword n of each = max_signed(int16(hw), int16(field)); n 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long sh = (unsigned long)(n * 16);
    long f = (long)((unsigned long)field & 0xFFFFul);
    if (f & 0x8000) f -= 65536;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul); if (va & 0x8000) va -= 65536;
    long vb = (long)((mb >> sh) & 0xFFFFul); if (vb & 0x8000) vb -= 65536;
    long ma_m = (va > f) ? va : f;
    long mb_m = (vb > f) ? vb : f;
    unsigned long wa = (unsigned long)ma_m & 0xFFFFul;
    unsigned long wb = (unsigned long)mb_m & 0xFFFFul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
