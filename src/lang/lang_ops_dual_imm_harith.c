/* CubalC lang — lang_ops_dual_imm_harith.c
 * DUAL imm 16-bit field arith: DADD16N·DSUB16N·DMUL16N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_harith.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_harith(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-2 dual-stack imm 16-bit field arith: DADD16N · DSUB16N · DMUL16N
   * (dual of SADD16N/SSUB16N/SMUL16N; wrap uint16 halfword plane on top two cells) */
  if (kw(&L->cur,"DADD16N")||kw(&L->cur,"S2ADD16N")||kw(&L->cur,"STACK2ADD16N")||
      kw(&L->cur,"PAIRADD16N")||kw(&L->cur,"DADDHIMM")||kw(&L->cur,"DINC16N")||
      kw(&L->cur,"DADDHN16")){
    /* a b + field n → halfword n of each = (hw + field) & 0xFFFF; n clamped 0..3 */
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
    unsigned long wa = (((ma >> sh) & 0xFFFFul) + f) & 0xFFFFul;
    unsigned long wb = (((mb >> sh) & 0xFFFFul) + f) & 0xFFFFul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSUB16N")||kw(&L->cur,"S2SUB16N")||kw(&L->cur,"STACK2SUB16N")||
      kw(&L->cur,"PAIRSUB16N")||kw(&L->cur,"DSUBHIMM")||kw(&L->cur,"DDEC16N")||
      kw(&L->cur,"DSUBHN16")){
    /* a b + field n → halfword n of each = (hw - field) & 0xFFFF; n clamped 0..3 */
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
    unsigned long wa = (((ma >> sh) & 0xFFFFul) - f) & 0xFFFFul;
    unsigned long wb = (((mb >> sh) & 0xFFFFul) - f) & 0xFFFFul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMUL16N")||kw(&L->cur,"S2MUL16N")||kw(&L->cur,"STACK2MUL16N")||
      kw(&L->cur,"PAIRMUL16N")||kw(&L->cur,"DMULHIMM")||kw(&L->cur,"DTIMES16N")||
      kw(&L->cur,"DMULHN16")){
    /* a b + field n → halfword n of each = (hw * field) & 0xFFFF; n clamped 0..3 */
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
    unsigned long wa = (((ma >> sh) & 0xFFFFul) * f) & 0xFFFFul;
    unsigned long wb = (((mb >> sh) & 0xFFFFul) * f) & 0xFFFFul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
