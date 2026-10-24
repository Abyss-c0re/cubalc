/* CubalC lang — lang_ops_dual_imm_nsord.c
 * DUAL imm 4-bit field signed ordered-cmp: DLTS4N·DGTS4N·DLTES4N·DGTES4N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_nsord.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_nsord(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-1 dual-stack imm 4-bit field signed ordered-cmp: DLTS4N · DGTS4N · DLTES4N · DGTES4N
   * (dual of SLTS4N/SGTS4N/SLTES4N/SGTES4N; signed int4 plane on top two) */
  if (kw(&L->cur,"DLTS4N")||kw(&L->cur,"S2LTS4N")||kw(&L->cur,"STACK2LTS4N")||
      kw(&L->cur,"PAIRLTS4N")||kw(&L->cur,"DCMPLTS4N")||kw(&L->cur,"DLTSSIGN4N")||
      kw(&L->cur,"DLTS4IMM")){
    /* a b + field n → nibble n of each = (int4(nib) < int4(field)) ? 1 : 0; n 0..15 */
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
    unsigned long wa = (va < f) ? 1ul : 0ul;
    unsigned long wb = (vb < f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGTS4N")||kw(&L->cur,"S2GTS4N")||kw(&L->cur,"STACK2GTS4N")||
      kw(&L->cur,"PAIRGTS4N")||kw(&L->cur,"DCMPGTS4N")||kw(&L->cur,"DGTSSIGN4N")||
      kw(&L->cur,"DGTS4IMM")){
    /* a b + field n → nibble n of each = (int4(nib) > int4(field)) ? 1 : 0; n 0..15 */
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
    unsigned long wa = (va > f) ? 1ul : 0ul;
    unsigned long wb = (vb > f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DLTES4N")||kw(&L->cur,"S2LTES4N")||kw(&L->cur,"STACK2LTES4N")||
      kw(&L->cur,"PAIRLTES4N")||kw(&L->cur,"DCMPLES4N")||kw(&L->cur,"DLEQS4N")||
      kw(&L->cur,"DLTES4IMM")){
    /* a b + field n → nibble n of each = (int4(nib) <= int4(field)) ? 1 : 0; n 0..15 */
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
    unsigned long wa = (va <= f) ? 1ul : 0ul;
    unsigned long wb = (vb <= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGTES4N")||kw(&L->cur,"S2GTES4N")||kw(&L->cur,"STACK2GTES4N")||
      kw(&L->cur,"PAIRGTES4N")||kw(&L->cur,"DCMPGES4N")||kw(&L->cur,"DGEQS4N")||
      kw(&L->cur,"DGTES4IMM")){
    /* a b + field n → nibble n of each = (int4(nib) >= int4(field)) ? 1 : 0; n 0..15 */
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
    unsigned long wa = (va >= f) ? 1ul : 0ul;
    unsigned long wb = (vb >= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
