/* CubalC lang — lang_ops_dual_imm_bsclamp.c
 * DUAL imm 8-bit signed clamp/between: DCLAMPS8N·DBETWEENS8N·DWITHINS8N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_bsclamp.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_bsclamp(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-5 dual-stack imm 8-bit field signed clamp+between: DCLAMPS8N · DBETWEENS8N
   * (dual of SCLAMPS8N/SBETWEENS8N; byte ladder of DCLAMPS16N after DMINS8N) */
  if (kw(&L->cur,"DCLAMPS8N")||kw(&L->cur,"S2CLAMPS8N")||kw(&L->cur,"STACK2CLAMPS8N")||
      kw(&L->cur,"PAIRCLAMPS8N")||kw(&L->cur,"DCLAMPSIGN8N")||kw(&L->cur,"DCLAMPS8IMM")||
      kw(&L->cur,"DBOUNDS8N")){
    /* a b + lo hi n → byte n of each = clamp_signed(int8(b),[lo,hi]); n 0..7 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    long slo = (long)((unsigned long)lo & 0xFFul); if (slo & 0x80) slo -= 256;
    long shi = (long)((unsigned long)hi & 0xFFul); if (shi & 0x80) shi -= 256;
    if (slo > shi){ long t = slo; slo = shi; shi = t; }
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul); if (va & 0x80) va -= 256;
    long vb = (long)((mb >> sh) & 0xFFul); if (vb & 0x80) vb -= 256;
    if (va < slo) va = slo; if (va > shi) va = shi;
    if (vb < slo) vb = slo; if (vb > shi) vb = shi;
    unsigned long wa = (unsigned long)va & 0xFFul;
    unsigned long wb = (unsigned long)vb & 0xFFul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DBETWEENS8N")||kw(&L->cur,"S2BETWEENS8N")||kw(&L->cur,"STACK2BETWEENS8N")||
      kw(&L->cur,"PAIRBETWEENS8N")||kw(&L->cur,"DINRANGES8N")||kw(&L->cur,"DBETWEENS8IMM")||
      kw(&L->cur,"DINRANGESIGN8N")){
    /* a b + lo hi n → byte n of each = (lo <= int8(b) <= hi) ? 1 : 0; n 0..7 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    long slo = (long)((unsigned long)lo & 0xFFul); if (slo & 0x80) slo -= 256;
    long shi = (long)((unsigned long)hi & 0xFFul); if (shi & 0x80) shi -= 256;
    if (slo > shi){ long t = slo; slo = shi; shi = t; }
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul); if (va & 0x80) va -= 256;
    long vb = (long)((mb >> sh) & 0xFFul); if (vb & 0x80) vb -= 256;
    unsigned long wa = (va >= slo && va <= shi) ? 1ul : 0ul;
    unsigned long wb = (vb >= slo && vb <= shi) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 dual-stack imm 8-bit field signed WITHIN: DWITHINS8N (dual of SWITHINS8N) */
  if (kw(&L->cur,"DWITHINS8N")||kw(&L->cur,"S2WITHINS8N")||kw(&L->cur,"STACK2WITHINS8N")||
      kw(&L->cur,"PAIRWITHINS8N")||kw(&L->cur,"DWITHINSIGN8N")||kw(&L->cur,"DWITHINS8IMM")||
      kw(&L->cur,"DINTERVALS8N")){
    /* a b + lo hi n → byte n of each = (lo <= int8(b) < hi) ? 1 : 0; n 0..7; no swap */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    long slo = (long)((unsigned long)lo & 0xFFul); if (slo & 0x80) slo -= 256;
    long shi = (long)((unsigned long)hi & 0xFFul); if (shi & 0x80) shi -= 256;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFul); if (va & 0x80) va -= 256;
    long vb = (long)((mb >> sh) & 0xFFul); if (vb & 0x80) vb -= 256;
    unsigned long wa = (va >= slo && va < shi) ? 1ul : 0ul;
    unsigned long wb = (vb >= slo && vb < shi) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
