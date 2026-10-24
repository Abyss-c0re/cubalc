/* CubalC lang — lang_ops_dual_imm_barith.c
 * DUAL imm 8-bit field arith: DADD8N·DSUB8N·DMUL8N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_barith.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_barith(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-7 dual-stack imm 8-bit field arith: DADD8N · DSUB8N · DMUL8N
   * (dual of SADD8N/SSUB8N/SMUL8N; wrap uint8 byte plane on top two cells after dual halfword ALU) */
  if (kw(&L->cur,"DADD8N")||kw(&L->cur,"S2ADD8N")||kw(&L->cur,"STACK2ADD8N")||
      kw(&L->cur,"PAIRADD8N")||kw(&L->cur,"DADDBIMM")||kw(&L->cur,"DINC8N")||
      kw(&L->cur,"DADDBN")){
    /* a b + field n → byte n of each = (byte + field) & 0xFF; n clamped 0..7 */
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
    unsigned long wa = (((ma >> sh) & 0xFFul) + f) & 0xFFul;
    unsigned long wb = (((mb >> sh) & 0xFFul) + f) & 0xFFul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DSUB8N")||kw(&L->cur,"S2SUB8N")||kw(&L->cur,"STACK2SUB8N")||
      kw(&L->cur,"PAIRSUB8N")||kw(&L->cur,"DSUBBIMM")||kw(&L->cur,"DDEC8N")||
      kw(&L->cur,"DSUBBN")){
    /* a b + field n → byte n of each = (byte - field) & 0xFF; n clamped 0..7 */
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
    unsigned long wa = (((ma >> sh) & 0xFFul) - f) & 0xFFul;
    unsigned long wb = (((mb >> sh) & 0xFFul) - f) & 0xFFul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMUL8N")||kw(&L->cur,"S2MUL8N")||kw(&L->cur,"STACK2MUL8N")||
      kw(&L->cur,"PAIRMUL8N")||kw(&L->cur,"DMULBIMM")||kw(&L->cur,"DTIMES8N")||
      kw(&L->cur,"DMULBN")){
    /* a b + field n → byte n of each = (byte * field) & 0xFF; n clamped 0..7 */
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
    unsigned long wa = (((ma >> sh) & 0xFFul) * f) & 0xFFul;
    unsigned long wb = (((mb >> sh) & 0xFFul) * f) & 0xFFul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
