/* CubalC lang — lang_ops_dual_imm_hbit.c
 * DUAL imm 16-bit field bitwise: DAND16N·DOR16N·DXOR16N·DNAND16N·DNOR16N·DXNOR16N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_hbit.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_hbit(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-8 dual-stack imm 16-bit field bitwise: DAND16N · DOR16N · DXOR16N (dual of SAND16N/SOR16N/SXOR16N) */
  if (kw(&L->cur,"DAND16N")||kw(&L->cur,"S2AND16N")||kw(&L->cur,"STACK2AND16N")||
      kw(&L->cur,"PAIRAND16N")||kw(&L->cur,"DANDHIMM")||kw(&L->cur,"DKEEP16N")||
      kw(&L->cur,"DANDHN16")){
    /* a b + field n → halfword n of each &= field; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = ((ma >> sh) & 0xFFFFul) & f;
    unsigned long wb = ((mb >> sh) & 0xFFFFul) & f;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DOR16N")||kw(&L->cur,"S2OR16N")||kw(&L->cur,"STACK2OR16N")||
      kw(&L->cur,"PAIROR16N")||kw(&L->cur,"DORHIMM")||kw(&L->cur,"DSETOR16N")||
      kw(&L->cur,"DORHN16")){
    /* a b + field n → halfword n of each |= field; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = ((ma >> sh) & 0xFFFFul) | f;
    unsigned long wb = ((mb >> sh) & 0xFFFFul) | f;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXOR16N")||kw(&L->cur,"S2XOR16N")||kw(&L->cur,"STACK2XOR16N")||
      kw(&L->cur,"PAIRXOR16N")||kw(&L->cur,"DXORHIMM")||kw(&L->cur,"DFLIP16N")||
      kw(&L->cur,"DXORHN16")){
    /* a b + field n → halfword n of each ^= field; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = ((ma >> sh) & 0xFFFFul) ^ f;
    unsigned long wb = ((mb >> sh) & 0xFFFFul) ^ f;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 dual-stack imm inverted 16-bit field: DNAND16N · DNOR16N · DXNOR16N (dual of SNAND16N/SNOR16N/SXNOR16N) */
  if (kw(&L->cur,"DNAND16N")||kw(&L->cur,"S2NAND16N")||kw(&L->cur,"STACK2NAND16N")||
      kw(&L->cur,"PAIRNAND16N")||kw(&L->cur,"DNANDHIMM")||kw(&L->cur,"DINVERTAND16N")||
      kw(&L->cur,"DNANDHN16")){
    /* a b + field n → halfword n of each = ~(half & field) & 0xFFFF; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = (~(((ma >> sh) & 0xFFFFul) & f)) & 0xFFFFul;
    unsigned long wb = (~(((mb >> sh) & 0xFFFFul) & f)) & 0xFFFFul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNOR16N")||kw(&L->cur,"S2NOR16N")||kw(&L->cur,"STACK2NOR16N")||
      kw(&L->cur,"PAIRNOR16N")||kw(&L->cur,"DNORHIMM")||kw(&L->cur,"DINVERTOR16N")||
      kw(&L->cur,"DNORHN16")){
    /* a b + field n → halfword n of each = ~(half | field) & 0xFFFF; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = (~(((ma >> sh) & 0xFFFFul) | f)) & 0xFFFFul;
    unsigned long wb = (~(((mb >> sh) & 0xFFFFul) | f)) & 0xFFFFul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXNOR16N")||kw(&L->cur,"S2XNOR16N")||kw(&L->cur,"STACK2XNOR16N")||
      kw(&L->cur,"PAIRXNOR16N")||kw(&L->cur,"DXNORHIMM")||kw(&L->cur,"DEQUIV16N")||
      kw(&L->cur,"DXNORHN16")){
    /* a b + field n → halfword n of each = ~(half ^ field) & 0xFFFF; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = (~(((ma >> sh) & 0xFFFFul) ^ f)) & 0xFFFFul;
    unsigned long wb = (~(((mb >> sh) & 0xFFFFul) ^ f)) & 0xFFFFul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
