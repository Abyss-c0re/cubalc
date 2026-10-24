/* CubalC lang — lang_ops_dual_imm_bsmm.c
 * DUAL imm 8-bit signed min/max: DMINS8N·DMAXS8N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_bsmm.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_bsmm(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-2 dual-stack imm 8-bit field signed min/max: DMINS8N · DMAXS8N
   * (dual of SMINS8N/SMAXS8N; byte ladder of DMINS16N after DLTS8N) */
  if (kw(&L->cur,"DMINS8N")||kw(&L->cur,"S2MINS8N")||kw(&L->cur,"STACK2MINS8N")||
      kw(&L->cur,"PAIRMINS8N")||kw(&L->cur,"DMINSIGN8N")||kw(&L->cur,"DMINS8IMM")||
      kw(&L->cur,"DMINSI8N")){
    /* a b + field n → byte n of each = min_signed(int8(b), int8(field)); n 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    long f = (long)((unsigned long)field & 0xFFul);
    if (f & 0x80) f -= 256;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul); if (va & 0x80) va -= 256;
    long vb = (long)((mb >> sh) & 0xFFul); if (vb & 0x80) vb -= 256;
    long ma_m = (va < f) ? va : f;
    long mb_m = (vb < f) ? vb : f;
    unsigned long wa = (unsigned long)ma_m & 0xFFul;
    unsigned long wb = (unsigned long)mb_m & 0xFFul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMAXS8N")||kw(&L->cur,"S2MAXS8N")||kw(&L->cur,"STACK2MAXS8N")||
      kw(&L->cur,"PAIRMAXS8N")||kw(&L->cur,"DMAXSIGN8N")||kw(&L->cur,"DMAXS8IMM")||
      kw(&L->cur,"DMAXSI8N")){
    /* a b + field n → byte n of each = max_signed(int8(b), int8(field)); n 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long sh = (unsigned long)(n * 8);
    long f = (long)((unsigned long)field & 0xFFul);
    if (f & 0x80) f -= 256;
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul); if (va & 0x80) va -= 256;
    long vb = (long)((mb >> sh) & 0xFFul); if (vb & 0x80) vb -= 256;
    long ma_m = (va > f) ? va : f;
    long mb_m = (vb > f) ? vb : f;
    unsigned long wa = (unsigned long)ma_m & 0xFFul;
    unsigned long wb = (unsigned long)mb_m & 0xFFul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
