/* CubalC lang — lang_ops_dual_imm_narith.c
 * DUAL imm 4-bit field arith: DADD4N·DSUB4N·DMUL4N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_narith.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_narith(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-2 dual-stack imm 4-bit field arith: DADD4N · DSUB4N · DMUL4N
   * (dual of SADD4N/SSUB4N/SMUL4N; wrap uint4 nibble plane on top two cells after dual 8n ALU) */
  if (kw(&L->cur,"DADD4N")||kw(&L->cur,"S2ADD4N")||kw(&L->cur,"STACK2ADD4N")||
      kw(&L->cur,"PAIRADD4N")||kw(&L->cur,"DADDNIMM")||kw(&L->cur,"DINC4N")||
      kw(&L->cur,"DADDNIBN")){
    /* a b + field n → nibble n of each = (nib + field) & 0xF; n clamped 0..15 */
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
    unsigned long wa = (((ma >> sh) & 0xFul) + f) & 0xFul;
    unsigned long wb = (((mb >> sh) & 0xFul) + f) & 0xFul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSUB4N")||kw(&L->cur,"S2SUB4N")||kw(&L->cur,"STACK2SUB4N")||
      kw(&L->cur,"PAIRSUB4N")||kw(&L->cur,"DSUBNIMM")||kw(&L->cur,"DDEC4N")||
      kw(&L->cur,"DSUBNIBN")){
    /* a b + field n → nibble n of each = (nib - field) & 0xF; n clamped 0..15 */
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
    unsigned long wa = (((ma >> sh) & 0xFul) - f) & 0xFul;
    unsigned long wb = (((mb >> sh) & 0xFul) - f) & 0xFul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMUL4N")||kw(&L->cur,"S2MUL4N")||kw(&L->cur,"STACK2MUL4N")||
      kw(&L->cur,"PAIRMUL4N")||kw(&L->cur,"DMULNIMM")||kw(&L->cur,"DTIMES4N")||
      kw(&L->cur,"DMULNIBN")){
    /* a b + field n → nibble n of each = (nib * field) & 0xF; n clamped 0..15 */
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
    unsigned long wa = (((ma >> sh) & 0xFul) * f) & 0xFul;
    unsigned long wb = (((mb >> sh) & 0xFul) * f) & 0xFul;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
