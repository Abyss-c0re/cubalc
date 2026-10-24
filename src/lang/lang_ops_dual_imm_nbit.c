/* CubalC lang — lang_ops_dual_imm_nbit.c
 * DUAL imm 4-bit field bitwise: DAND4N·DOR4N·DXOR4N·DNAND4N·DNOR4N·DXNOR4N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_nbit.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_nbit(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-4 dual-stack imm 4-bit field bitwise: DAND4N · DOR4N · DXOR4N (dual of SAND4N/SOR4N/SXOR4N) */
  if (kw(&L->cur,"DAND4N")||kw(&L->cur,"S2AND4N")||kw(&L->cur,"STACK2AND4N")||
      kw(&L->cur,"PAIRAND4N")||kw(&L->cur,"DANDNIMM")||kw(&L->cur,"DKEEP4N")||
      kw(&L->cur,"DANDNIBN")){
    /* a b + field n → nibble n of each &= field; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = ((ma >> sh) & 0xFul) & f;
    unsigned long wb = ((mb >> sh) & 0xFul) & f;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DOR4N")||kw(&L->cur,"S2OR4N")||kw(&L->cur,"STACK2OR4N")||
      kw(&L->cur,"PAIROR4N")||kw(&L->cur,"DORNIMM")||kw(&L->cur,"DSETOR4N")||
      kw(&L->cur,"DORNIBN")){
    /* a b + field n → nibble n of each |= field; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = ((ma >> sh) & 0xFul) | f;
    unsigned long wb = ((mb >> sh) & 0xFul) | f;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXOR4N")||kw(&L->cur,"S2XOR4N")||kw(&L->cur,"STACK2XOR4N")||
      kw(&L->cur,"PAIRXOR4N")||kw(&L->cur,"DXORNIMM")||kw(&L->cur,"DFLIP4N")||
      kw(&L->cur,"DXORNIBN")){
    /* a b + field n → nibble n of each ^= field; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = ((ma >> sh) & 0xFul) ^ f;
    unsigned long wb = ((mb >> sh) & 0xFul) ^ f;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 dual-stack imm inverted 4-bit field: DNAND4N · DNOR4N · DXNOR4N (dual of SNAND4N/SNOR4N/SXNOR4N) */
  if (kw(&L->cur,"DNAND4N")||kw(&L->cur,"S2NAND4N")||kw(&L->cur,"STACK2NAND4N")||
      kw(&L->cur,"PAIRNAND4N")||kw(&L->cur,"DNANDNIMM")||kw(&L->cur,"DINVERTAND4N")||
      kw(&L->cur,"DNANDNIBN")){
    /* a b + field n → nibble n of each = ~(nibble & field) & 0xF; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = (~(((ma >> sh) & 0xFul) & f)) & 0xFul;
    unsigned long wb = (~(((mb >> sh) & 0xFul) & f)) & 0xFul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNOR4N")||kw(&L->cur,"S2NOR4N")||kw(&L->cur,"STACK2NOR4N")||
      kw(&L->cur,"PAIRNOR4N")||kw(&L->cur,"DNORNIMM")||kw(&L->cur,"DINVERTOR4N")||
      kw(&L->cur,"DNORNIBN")){
    /* a b + field n → nibble n of each = ~(nibble | field) & 0xF; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = (~(((ma >> sh) & 0xFul) | f)) & 0xFul;
    unsigned long wb = (~(((mb >> sh) & 0xFul) | f)) & 0xFul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXNOR4N")||kw(&L->cur,"S2XNOR4N")||kw(&L->cur,"STACK2XNOR4N")||
      kw(&L->cur,"PAIRXNOR4N")||kw(&L->cur,"DXNORNIMM")||kw(&L->cur,"DEQUIV4N")||
      kw(&L->cur,"DXNORNIBN")){
    /* a b + field n → nibble n of each = ~(nibble ^ field) & 0xF; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = (~(((ma >> sh) & 0xFul) ^ f)) & 0xFul;
    unsigned long wb = (~(((mb >> sh) & 0xFul) ^ f)) & 0xFul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
