/* CubalC lang — lang_ops_dual_imm_h0ord.c
 * DUAL imm 16-bit signed zero-ord: D0LT16N·D0GT16N·D0LE16N·D0GE16N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_h0ord.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_h0ord(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-8 dual-stack imm 16-bit field signed LT0: D0LT16N (dual of S0LT16N) */
  if (kw(&L->cur,"D0LT16N")||kw(&L->cur,"S20LT16N")||kw(&L->cur,"STACK20LT16N")||
      kw(&L->cur,"PAIR0LT16N")||kw(&L->cur,"DLT016N")||kw(&L->cur,"DNEGZ16N")||
      kw(&L->cur,"D0LT16IMM")){
    /* a b + n → halfword n of each = (int16(hw) < 0) ? 1 : 0; n 0..3 */
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
    unsigned long wa = (va < 0) ? 1ul : 0ul;
    unsigned long wb = (vb < 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 dual-stack imm 16-bit field signed GT0: D0GT16N (dual of S0GT16N) */
  if (kw(&L->cur,"D0GT16N")||kw(&L->cur,"S20GT16N")||kw(&L->cur,"STACK20GT16N")||
      kw(&L->cur,"PAIR0GT16N")||kw(&L->cur,"DGT016N")||kw(&L->cur,"DPOSZ16N")||
      kw(&L->cur,"D0GT16IMM")){
    /* a b + n → halfword n of each = (int16(hw) > 0) ? 1 : 0; n 0..3 */
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
    unsigned long wa = (va > 0) ? 1ul : 0ul;
    unsigned long wb = (vb > 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 dual-stack imm 16-bit field signed LE0: D0LE16N (dual of S0LE16N) */
  if (kw(&L->cur,"D0LE16N")||kw(&L->cur,"S20LE16N")||kw(&L->cur,"STACK20LE16N")||
      kw(&L->cur,"PAIR0LE16N")||kw(&L->cur,"DLE016N")||kw(&L->cur,"DNONPOS16N")||
      kw(&L->cur,"D0LE16IMM")){
    /* a b + n → halfword n of each = (int16(hw) <= 0) ? 1 : 0; n 0..3 */
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
    unsigned long wa = (va <= 0) ? 1ul : 0ul;
    unsigned long wb = (vb <= 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 dual-stack imm 16-bit field signed GE0: D0GE16N (dual of S0GE16N) */
  if (kw(&L->cur,"D0GE16N")||kw(&L->cur,"S20GE16N")||kw(&L->cur,"STACK20GE16N")||
      kw(&L->cur,"PAIR0GE16N")||kw(&L->cur,"DGE016N")||kw(&L->cur,"DNONNEG16N")||
      kw(&L->cur,"D0GE16IMM")){
    /* a b + n → halfword n of each = (int16(hw) >= 0) ? 1 : 0; n 0..3 */
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
    unsigned long wa = (va >= 0) ? 1ul : 0ul;
    unsigned long wb = (vb >= 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
