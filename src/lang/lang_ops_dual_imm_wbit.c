/* CubalC lang — lang_ops_dual_imm_wbit.c
 * DUAL imm 32-bit field bitwise: DAND32N·DOR32N·DXOR32N·DNAND32N·DNOR32N·DXNOR32N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_wbit.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_wbit(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-7 dual-stack imm 32-bit field bitwise: DAND32N · DOR32N · DXOR32N (dual of SAND32N/SOR32N/SXOR32N) */
  if (kw(&L->cur,"DAND32N")||kw(&L->cur,"S2AND32N")||kw(&L->cur,"STACK2AND32N")||
      kw(&L->cur,"PAIRAND32N")||kw(&L->cur,"DANDWN")||kw(&L->cur,"DKEEP32N")||
      kw(&L->cur,"DANDWIMM")){
    /* a b + field n → word n of each &= field; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = ((ma >> sh) & 0xFFFFFFFFul) & f;
    unsigned long wb = ((mb >> sh) & 0xFFFFFFFFul) & f;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DOR32N")||kw(&L->cur,"S2OR32N")||kw(&L->cur,"STACK2OR32N")||
      kw(&L->cur,"PAIROR32N")||kw(&L->cur,"DORWN")||kw(&L->cur,"DSETOR32N")||
      kw(&L->cur,"DORWIMM")){
    /* a b + field n → word n of each |= field; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = ((ma >> sh) & 0xFFFFFFFFul) | f;
    unsigned long wb = ((mb >> sh) & 0xFFFFFFFFul) | f;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXOR32N")||kw(&L->cur,"S2XOR32N")||kw(&L->cur,"STACK2XOR32N")||
      kw(&L->cur,"PAIRXOR32N")||kw(&L->cur,"DXORWN")||kw(&L->cur,"DFLIP32N")||
      kw(&L->cur,"DXORWIMM")){
    /* a b + field n → word n of each ^= field; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = ((ma >> sh) & 0xFFFFFFFFul) ^ f;
    unsigned long wb = ((mb >> sh) & 0xFFFFFFFFul) ^ f;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 dual-stack imm inverted 32-bit field: DNAND32N · DNOR32N · DXNOR32N (dual of SNAND32N; complete dual inverted ladder) */
  if (kw(&L->cur,"DNAND32N")||kw(&L->cur,"S2NAND32N")||kw(&L->cur,"STACK2NAND32N")||
      kw(&L->cur,"PAIRNAND32N")||kw(&L->cur,"DNANDWN")||kw(&L->cur,"DINVERTAND32N")||
      kw(&L->cur,"DNANDWIMM")){
    /* a b + field n → word n of each = ~(word & field) & 0xFFFFFFFF; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = (~(((ma >> sh) & 0xFFFFFFFFul) & f)) & 0xFFFFFFFFul;
    unsigned long wb = (~(((mb >> sh) & 0xFFFFFFFFul) & f)) & 0xFFFFFFFFul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNOR32N")||kw(&L->cur,"S2NOR32N")||kw(&L->cur,"STACK2NOR32N")||
      kw(&L->cur,"PAIRNOR32N")||kw(&L->cur,"DNORWN")||kw(&L->cur,"DINVERTOR32N")||
      kw(&L->cur,"DNORWIMM")){
    /* a b + field n → word n of each = ~(word | field) & 0xFFFFFFFF; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = (~(((ma >> sh) & 0xFFFFFFFFul) | f)) & 0xFFFFFFFFul;
    unsigned long wb = (~(((mb >> sh) & 0xFFFFFFFFul) | f)) & 0xFFFFFFFFul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXNOR32N")||kw(&L->cur,"S2XNOR32N")||kw(&L->cur,"STACK2XNOR32N")||
      kw(&L->cur,"PAIRXNOR32N")||kw(&L->cur,"DXNORWN")||kw(&L->cur,"DEQUIV32N")||
      kw(&L->cur,"DXNORWIMM")){
    /* a b + field n → word n of each = ~(word ^ field) & 0xFFFFFFFF; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = (~(((ma >> sh) & 0xFFFFFFFFul) ^ f)) & 0xFFFFFFFFul;
    unsigned long wb = (~(((mb >> sh) & 0xFFFFFFFFul) ^ f)) & 0xFFFFFFFFul;
    long x = (long)((ma & ~(0xFFFFFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
