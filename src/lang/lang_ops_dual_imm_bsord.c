/* CubalC lang — lang_ops_dual_imm_bsord.c
 * DUAL imm 8-bit signed ordered-cmp: DLTS8N·DGTS8N·DLTES8N·DGTES8N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_bsord.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_bsord(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-3 dual-stack imm 8-bit field signed ordered-cmp: DLTS8N · DGTS8N · DLTES8N · DGTES8N
   * (dual of SLTS8N/SGTS8N/SLTES8N/SGTES8N; signed int8 plane on top two) */
  if (kw(&L->cur,"DLTS8N")||kw(&L->cur,"S2LTS8N")||kw(&L->cur,"STACK2LTS8N")||
      kw(&L->cur,"PAIRLTS8N")||kw(&L->cur,"DCMPLTS8N")||kw(&L->cur,"DLTSSIGN8N")||
      kw(&L->cur,"DLTS8IMM")){
    /* a b + field n → byte n of each = (int8(b) < int8(field)) ? 1 : 0; n 0..7 */
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
    unsigned long wa = (va < f) ? 1ul : 0ul;
    unsigned long wb = (vb < f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGTS8N")||kw(&L->cur,"S2GTS8N")||kw(&L->cur,"STACK2GTS8N")||
      kw(&L->cur,"PAIRGTS8N")||kw(&L->cur,"DCMPGTS8N")||kw(&L->cur,"DGTSSIGN8N")||
      kw(&L->cur,"DGTS8IMM")){
    /* a b + field n → byte n of each = (int8(b) > int8(field)) ? 1 : 0; n 0..7 */
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
    unsigned long wa = (va > f) ? 1ul : 0ul;
    unsigned long wb = (vb > f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DLTES8N")||kw(&L->cur,"S2LTES8N")||kw(&L->cur,"STACK2LTES8N")||
      kw(&L->cur,"PAIRLTES8N")||kw(&L->cur,"DCMPLES8N")||kw(&L->cur,"DLEQS8N")||
      kw(&L->cur,"DLTES8IMM")){
    /* a b + field n → byte n of each = (int8(b) <= int8(field)) ? 1 : 0; n 0..7 */
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
    unsigned long wa = (va <= f) ? 1ul : 0ul;
    unsigned long wb = (vb <= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DGTES8N")||kw(&L->cur,"S2GTES8N")||kw(&L->cur,"STACK2GTES8N")||
      kw(&L->cur,"PAIRGTES8N")||kw(&L->cur,"DCMPGES8N")||kw(&L->cur,"DGEQS8N")||
      kw(&L->cur,"DGTES8IMM")){
    /* a b + field n → byte n of each = (int8(b) >= int8(field)) ? 1 : 0; n 0..7 */
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
    unsigned long wa = (va >= f) ? 1ul : 0ul;
    unsigned long wb = (vb >= f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
