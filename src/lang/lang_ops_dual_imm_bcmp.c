/* CubalC lang — lang_ops_dual_imm_bcmp.c
 * DUAL imm 8-bit field max/eq/ne: DMAX8N·DEQ8N·DNE8N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_bcmp.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_bcmp(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-7 dual-stack imm 8-bit field max+eq: DMAX8N · DEQ8N · DNE8N
   * (dual of SMAX8N/SEQ8N/SNE8N; complete min/max + equality on pair bytes) */
  if (kw(&L->cur,"DMAX8N")||kw(&L->cur,"S2MAX8N")||kw(&L->cur,"STACK2MAX8N")||
      kw(&L->cur,"PAIRMAX8N")||kw(&L->cur,"DMAXBIMM")||kw(&L->cur,"DGE8N")||
      kw(&L->cur,"DMAXBN")){
    /* a b + field n → byte n of each = max(byte, field); n clamped 0..7 */
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
    unsigned long wa = (va > f) ? va : f;
    unsigned long wb = (vb > f) ? vb : f;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DEQ8N")||kw(&L->cur,"S2EQ8N")||kw(&L->cur,"STACK2EQ8N")||
      kw(&L->cur,"PAIREQ8N")||kw(&L->cur,"DEQBIMM")||kw(&L->cur,"DCMPEQ8N")||
      kw(&L->cur,"DEQBN")){
    /* a b + field n → byte n of each = (byte == field) ? 1 : 0; n clamped 0..7 */
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
    unsigned long wa = (va == f) ? 1ul : 0ul;
    unsigned long wb = (vb == f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DNE8N")||kw(&L->cur,"S2NE8N")||kw(&L->cur,"STACK2NE8N")||
      kw(&L->cur,"PAIRNE8N")||kw(&L->cur,"DNEBIMM")||kw(&L->cur,"DCMPNE8N")||
      kw(&L->cur,"DNEBN")){
    /* a b + field n → byte n of each = (byte != field) ? 1 : 0; n clamped 0..7 */
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
    unsigned long wa = (va != f) ? 1ul : 0ul;
    unsigned long wb = (vb != f) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
