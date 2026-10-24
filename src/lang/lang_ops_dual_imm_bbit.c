/* CubalC lang — lang_ops_dual_imm_bbit.c
 * DUAL imm 8-bit field bitwise: DAND8N·DOR8N·DXOR8N·DNAND8N·DNOR8N·DXNOR8N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_bbit.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_bbit(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-3 dual-stack imm 8-bit field bitwise: DAND8N · DOR8N · DXOR8N (dual of SAND8N/SOR8N/SXOR8N) */
  if (kw(&L->cur,"DAND8N")||kw(&L->cur,"S2AND8N")||kw(&L->cur,"STACK2AND8N")||
      kw(&L->cur,"PAIRAND8N")||kw(&L->cur,"DANDBIMM")||kw(&L->cur,"DKEEP8N")||
      kw(&L->cur,"DANDBN")){
    /* a b + field n → byte n of each &= field; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = ((ma >> sh) & 0xFFul) & f;
    unsigned long wb = ((mb >> sh) & 0xFFul) & f;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DOR8N")||kw(&L->cur,"S2OR8N")||kw(&L->cur,"STACK2OR8N")||
      kw(&L->cur,"PAIROR8N")||kw(&L->cur,"DORBIMM")||kw(&L->cur,"DSETOR8N")||
      kw(&L->cur,"DORBN")){
    /* a b + field n → byte n of each |= field; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = ((ma >> sh) & 0xFFul) | f;
    unsigned long wb = ((mb >> sh) & 0xFFul) | f;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXOR8N")||kw(&L->cur,"S2XOR8N")||kw(&L->cur,"STACK2XOR8N")||
      kw(&L->cur,"PAIRXOR8N")||kw(&L->cur,"DXORBIMM")||kw(&L->cur,"DFLIP8N")||
      kw(&L->cur,"DXORBN")){
    /* a b + field n → byte n of each ^= field; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = ((ma >> sh) & 0xFFul) ^ f;
    unsigned long wb = ((mb >> sh) & 0xFFul) ^ f;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 dual-stack imm inverted 8-bit field: DNAND8N · DNOR8N · DXNOR8N (dual of SNAND8N/SNOR8N/SXNOR8N) */
  if (kw(&L->cur,"DNAND8N")||kw(&L->cur,"S2NAND8N")||kw(&L->cur,"STACK2NAND8N")||
      kw(&L->cur,"PAIRNAND8N")||kw(&L->cur,"DNANDBIMM")||kw(&L->cur,"DINVERTAND8N")||
      kw(&L->cur,"DNANDBN")){
    /* a b + field n → byte n of each = ~(byte & field) & 0xFF; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = (~(((ma >> sh) & 0xFFul) & f)) & 0xFFul;
    unsigned long wb = (~(((mb >> sh) & 0xFFul) & f)) & 0xFFul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNOR8N")||kw(&L->cur,"S2NOR8N")||kw(&L->cur,"STACK2NOR8N")||
      kw(&L->cur,"PAIRNOR8N")||kw(&L->cur,"DNORBIMM")||kw(&L->cur,"DINVERTOR8N")||
      kw(&L->cur,"DNORBN")){
    /* a b + field n → byte n of each = ~(byte | field) & 0xFF; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = (~(((ma >> sh) & 0xFFul) | f)) & 0xFFul;
    unsigned long wb = (~(((mb >> sh) & 0xFFul) | f)) & 0xFFul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DXNOR8N")||kw(&L->cur,"S2XNOR8N")||kw(&L->cur,"STACK2XNOR8N")||
      kw(&L->cur,"PAIRXNOR8N")||kw(&L->cur,"DXNORBIMM")||kw(&L->cur,"DEQUIV8N")||
      kw(&L->cur,"DXNORBN")){
    /* a b + field n → byte n of each = ~(byte ^ field) & 0xFF; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long wa = (~(((ma >> sh) & 0xFFul) ^ f)) & 0xFFul;
    unsigned long wb = (~(((mb >> sh) & 0xFFul) ^ f)) & 0xFFul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
