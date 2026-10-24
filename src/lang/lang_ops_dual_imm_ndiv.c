/* CubalC lang — lang_ops_dual_imm_ndiv.c
 * DUAL imm 4-bit field div/mod/min: DDIV4N·DMOD4N·DMIN4N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_ndiv.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_ndiv(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-4 dual-stack imm 4-bit field div/mod/min: DDIV4N · DMOD4N · DMIN4N
   * (dual of SDIV4N/SMOD4N/SMIN4N; field 0 → div/mod 0 on each lane) */
  if (kw(&L->cur,"DDIV4N")||kw(&L->cur,"S2DIV4N")||kw(&L->cur,"STACK2DIV4N")||
      kw(&L->cur,"PAIRDIV4N")||kw(&L->cur,"DDIVNIMM")||kw(&L->cur,"DQUO4N")||
      kw(&L->cur,"DDIVNIBN")){
    /* a b + field n → nibble n of each = nib / field (field 0 → 0); n clamped 0..15 */
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
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long wa = (f == 0) ? 0ul : (va / f);
    unsigned long wb = (f == 0) ? 0ul : (vb / f);
    long x = (long)((ma & ~(0xFul << sh)) | ((wa & 0xFul) << sh));
    long y = (long)((mb & ~(0xFul << sh)) | ((wb & 0xFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMOD4N")||kw(&L->cur,"S2MOD4N")||kw(&L->cur,"STACK2MOD4N")||
      kw(&L->cur,"PAIRMOD4N")||kw(&L->cur,"DMODNIMM")||kw(&L->cur,"DREM4N")||
      kw(&L->cur,"DMODNIBN")){
    /* a b + field n → nibble n of each = nib % field (field 0 → 0); n clamped 0..15 */
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
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long wa = (f == 0) ? 0ul : (va % f);
    unsigned long wb = (f == 0) ? 0ul : (vb % f);
    long x = (long)((ma & ~(0xFul << sh)) | ((wa & 0xFul) << sh));
    long y = (long)((mb & ~(0xFul << sh)) | ((wb & 0xFul) << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DMIN4N")||kw(&L->cur,"S2MIN4N")||kw(&L->cur,"STACK2MIN4N")||
      kw(&L->cur,"PAIRMIN4N")||kw(&L->cur,"DMINNIMM")||kw(&L->cur,"DLE4N")||
      kw(&L->cur,"DMINNIBN")){
    /* a b + field n → nibble n of each = min(nib, field); n clamped 0..15 */
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
    unsigned long va = (ma >> sh) & 0xFul;
    unsigned long vb = (mb >> sh) & 0xFul;
    unsigned long wa = (va < f) ? va : f;
    unsigned long wb = (vb < f) ? vb : f;
    long x = (long)((ma & ~(0xFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
