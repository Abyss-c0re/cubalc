/* CubalC lang — lang_ops_dual_imm_hsord.c
 * DUAL imm 16-bit signed ordered-cmp: DLTS16N·DGTS16N·DLTES16N·DGTES16N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_hsord.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_hsord(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-8 dual-stack imm 16-bit field signed ordered-cmp: DLTS16N · DGTS16N · DLTES16N · DGTES16N
   * (dual of SLTS16N/SGTS16N/SLTES16N/SGTES16N; signed int16 plane on top two) */
  if (kw(&L->cur,"DLTS16N")||kw(&L->cur,"S2LTS16N")||kw(&L->cur,"STACK2LTS16N")||
      kw(&L->cur,"PAIRLTS16N")||kw(&L->cur,"DCMPLTS16N")||kw(&L->cur,"DLTSSIGN16N")||
      kw(&L->cur,"DLTS16IMM")){
    /* a b + field n → halfword n of each = (int16(hw) < int16(field)) ? 1 : 0; n 0..3 */
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
    unsigned long wa = (va < f) ? 1ul : 0ul;
    unsigned long wb = (vb < f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGTS16N")||kw(&L->cur,"S2GTS16N")||kw(&L->cur,"STACK2GTS16N")||
      kw(&L->cur,"PAIRGTS16N")||kw(&L->cur,"DCMPGTS16N")||kw(&L->cur,"DGTSSIGN16N")||
      kw(&L->cur,"DGTS16IMM")){
    /* a b + field n → halfword n of each = (int16(hw) > int16(field)) ? 1 : 0; n 0..3 */
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
    unsigned long wa = (va > f) ? 1ul : 0ul;
    unsigned long wb = (vb > f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DLTES16N")||kw(&L->cur,"S2LTES16N")||kw(&L->cur,"STACK2LTES16N")||
      kw(&L->cur,"PAIRLTES16N")||kw(&L->cur,"DCMPLES16N")||kw(&L->cur,"DLEQS16N")||
      kw(&L->cur,"DLTES16IMM")){
    /* a b + field n → halfword n of each = (int16(hw) <= int16(field)) ? 1 : 0; n 0..3 */
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
    unsigned long wa = (va <= f) ? 1ul : 0ul;
    unsigned long wb = (vb <= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGTES16N")||kw(&L->cur,"S2GTES16N")||kw(&L->cur,"STACK2GTES16N")||
      kw(&L->cur,"PAIRGTES16N")||kw(&L->cur,"DCMPGES16N")||kw(&L->cur,"DGEQS16N")||
      kw(&L->cur,"DGTES16IMM")){
    /* a b + field n → halfword n of each = (int16(hw) >= int16(field)) ? 1 : 0; n 0..3 */
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
