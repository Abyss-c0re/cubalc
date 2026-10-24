/* CubalC lang — lang_ops_dual_imm_b0ord.c
 * DUAL imm 8-bit signed zero-ord: D0LT8N·D0GT8N·D0LE8N·D0GE8N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_b0ord.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_b0ord(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-7 dual-stack imm 8-bit field signed LT0: D0LT8N (dual of S0LT8N) */
  if (kw(&L->cur,"D0LT8N")||kw(&L->cur,"S20LT8N")||kw(&L->cur,"STACK20LT8N")||
      kw(&L->cur,"PAIR0LT8N")||kw(&L->cur,"DLT08N")||kw(&L->cur,"DNEGZ8N")||
      kw(&L->cur,"D0LT8IMM")){
    /* a b + n → byte n of each = (int8(b) < 0) ? 1 : 0; n 0..7 */
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
    unsigned long wa = (va < 0) ? 1ul : 0ul;
    unsigned long wb = (vb < 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack imm 8-bit field signed GT0: D0GT8N (dual of S0GT8N) */
  if (kw(&L->cur,"D0GT8N")||kw(&L->cur,"S20GT8N")||kw(&L->cur,"STACK20GT8N")||
      kw(&L->cur,"PAIR0GT8N")||kw(&L->cur,"DGT08N")||kw(&L->cur,"DPOSZ8N")||
      kw(&L->cur,"D0GT8IMM")){
    /* a b + n → byte n of each = (int8(b) > 0) ? 1 : 0; n 0..7 */
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
    unsigned long wa = (va > 0) ? 1ul : 0ul;
    unsigned long wb = (vb > 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 dual-stack imm 8-bit field signed LE0: D0LE8N (dual of S0LE8N) */
  if (kw(&L->cur,"D0LE8N")||kw(&L->cur,"S20LE8N")||kw(&L->cur,"STACK20LE8N")||
      kw(&L->cur,"PAIR0LE8N")||kw(&L->cur,"DLE08N")||kw(&L->cur,"DNONPOS8N")||
      kw(&L->cur,"D0LE8IMM")){
    /* a b + n → byte n of each = (int8(b) <= 0) ? 1 : 0; n 0..7 */
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
    unsigned long wa = (va <= 0) ? 1ul : 0ul;
    unsigned long wb = (vb <= 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack imm 8-bit field signed GE0: D0GE8N (dual of S0GE8N) */
  if (kw(&L->cur,"D0GE8N")||kw(&L->cur,"S20GE8N")||kw(&L->cur,"STACK20GE8N")||
      kw(&L->cur,"PAIR0GE8N")||kw(&L->cur,"DGE08N")||kw(&L->cur,"DNONNEG8N")||
      kw(&L->cur,"D0GE8IMM")){
    /* a b + n → byte n of each = (int8(b) >= 0) ? 1 : 0; n 0..7 */
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
    unsigned long wa = (va >= 0) ? 1ul : 0ul;
    unsigned long wb = (vb >= 0) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
