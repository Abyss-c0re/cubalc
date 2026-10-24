/* CubalC lang — lang_ops_dual_imm_bdiv.c
 * DUAL imm 8-bit field div/mod/min: DDIV8N·DMOD8N·DMIN8N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_bdiv.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_bdiv(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-1 dual-stack imm 8-bit field div/mod/min: DDIV8N · DMOD8N · DMIN8N
   * (dual of SDIV8N/SMOD8N/SMIN8N; field 0 → div/mod 0 on each lane) */
  if (kw(&L->cur,"DDIV8N")||kw(&L->cur,"S2DIV8N")||kw(&L->cur,"STACK2DIV8N")||
      kw(&L->cur,"PAIRDIV8N")||kw(&L->cur,"DDIVBIMM")||kw(&L->cur,"DQUO8N")||
      kw(&L->cur,"DDIVBN")){
    /* a b + field n → byte n of each = byte / field (field 0 → 0); n clamped 0..7 */
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
    unsigned long va = (ma >> sh) & 0xFFul;
    unsigned long vb = (mb >> sh) & 0xFFul;
    unsigned long wa = (f == 0) ? 0ul : (va / f);
    unsigned long wb = (f == 0) ? 0ul : (vb / f);
    long x = (long)((ma & ~(0xFFul << sh)) | ((wa & 0xFFul) << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | ((wb & 0xFFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMOD8N")||kw(&L->cur,"S2MOD8N")||kw(&L->cur,"STACK2MOD8N")||
      kw(&L->cur,"PAIRMOD8N")||kw(&L->cur,"DMODBIMM")||kw(&L->cur,"DREM8N")||
      kw(&L->cur,"DMODBN")){
    /* a b + field n → byte n of each = byte % field (field 0 → 0); n clamped 0..7 */
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
    unsigned long va = (ma >> sh) & 0xFFul;
    unsigned long vb = (mb >> sh) & 0xFFul;
    unsigned long wa = (f == 0) ? 0ul : (va % f);
    unsigned long wb = (f == 0) ? 0ul : (vb % f);
    long x = (long)((ma & ~(0xFFul << sh)) | ((wa & 0xFFul) << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | ((wb & 0xFFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMIN8N")||kw(&L->cur,"S2MIN8N")||kw(&L->cur,"STACK2MIN8N")||
      kw(&L->cur,"PAIRMIN8N")||kw(&L->cur,"DMINBIMM")||kw(&L->cur,"DLE8N")||
      kw(&L->cur,"DMINBN")){
    /* a b + field n → byte n of each = min(byte, field); n clamped 0..7 */
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
    unsigned long va = (ma >> sh) & 0xFFul;
    unsigned long vb = (mb >> sh) & 0xFFul;
    unsigned long wa = (va < f) ? va : f;
    unsigned long wb = (vb < f) ? vb : f;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
