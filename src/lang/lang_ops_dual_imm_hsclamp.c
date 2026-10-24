/* CubalC lang — lang_ops_dual_imm_hsclamp.c
 * DUAL imm 16-bit signed clamp/between: DCLAMPS16N·DBETWEENS16N·DWITHINS16N (B_dual_peel).
 * Peel from lang_ops_dual.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_dual_imm_hsclamp.h"
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_dual_try_imm_hsclamp(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-7 dual-stack imm 16-bit field signed clamp+between: DCLAMPS16N · DBETWEENS16N
   * (dual of SCLAMPS16N/SBETWEENS16N; halfword ladder of DCLAMPS32N after DMINS16N) */
  if (kw(&L->cur,"DCLAMPS16N")||kw(&L->cur,"S2CLAMPS16N")||kw(&L->cur,"STACK2CLAMPS16N")||
      kw(&L->cur,"PAIRCLAMPS16N")||kw(&L->cur,"DCLAMPSIGN16N")||kw(&L->cur,"DCLAMPS16IMM")||
      kw(&L->cur,"DBOUNDS16N")){
    /* a b + lo hi n → halfword n of each = clamp_signed(int16(hw),[lo,hi]); n 0..3 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    long slo = (long)((unsigned long)lo & 0xFFFFul); if (slo & 0x8000) slo -= 65536;
    long shi = (long)((unsigned long)hi & 0xFFFFul); if (shi & 0x8000) shi -= 65536;
    if (slo > shi){ long t = slo; slo = shi; shi = t; }
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul); if (va & 0x8000) va -= 65536;
    long vb = (long)((mb >> sh) & 0xFFFFul); if (vb & 0x8000) vb -= 65536;
    if (va < slo) va = slo; if (va > shi) va = shi;
    if (vb < slo) vb = slo; if (vb > shi) vb = shi;
    unsigned long wa = (unsigned long)va & 0xFFFFul;
    unsigned long wb = (unsigned long)vb & 0xFFFFul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DBETWEENS16N")||kw(&L->cur,"S2BETWEENS16N")||kw(&L->cur,"STACK2BETWEENS16N")||
      kw(&L->cur,"PAIRBETWEENS16N")||kw(&L->cur,"DINRANGES16N")||kw(&L->cur,"DBETWEENS16IMM")||
      kw(&L->cur,"DINRANGESIGN16N")){
    /* a b + lo hi n → halfword n of each = (lo <= int16(hw) <= hi) ? 1 : 0; n 0..3 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    long slo = (long)((unsigned long)lo & 0xFFFFul); if (slo & 0x8000) slo -= 65536;
    long shi = (long)((unsigned long)hi & 0xFFFFul); if (shi & 0x8000) shi -= 65536;
    if (slo > shi){ long t = slo; slo = shi; shi = t; }
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul); if (va & 0x8000) va -= 65536;
    long vb = (long)((mb >> sh) & 0xFFFFul); if (vb & 0x8000) vb -= 65536;
    unsigned long wa = (va >= slo && va <= shi) ? 1ul : 0ul;
    unsigned long wb = (vb >= slo && vb <= shi) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 dual-stack imm 16-bit field signed WITHIN: DWITHINS16N (dual of SWITHINS16N) */
  if (kw(&L->cur,"DWITHINS16N")||kw(&L->cur,"S2WITHINS16N")||kw(&L->cur,"STACK2WITHINS16N")||
      kw(&L->cur,"PAIRWITHINS16N")||kw(&L->cur,"DWITHINSIGN16N")||kw(&L->cur,"DWITHINS16IMM")||
      kw(&L->cur,"DINTERVALS16N")){
    /* a b + lo hi n → halfword n of each = (lo <= int16(hw) < hi) ? 1 : 0; n 0..3; no swap */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    long slo = (long)((unsigned long)lo & 0xFFFFul); if (slo & 0x8000) slo -= 65536;
    long shi = (long)((unsigned long)hi & 0xFFFFul); if (shi & 0x8000) shi -= 65536;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long ma = (unsigned long)vm->stack[vm->sp - 2];
    unsigned long mb = (unsigned long)vm->stack[vm->sp - 1];
    long va = (long)((ma >> sh) & 0xFFFFul); if (va & 0x8000) va -= 65536;
    long vb = (long)((mb >> sh) & 0xFFFFul); if (vb & 0x8000) vb -= 65536;
    unsigned long wa = (va >= slo && va < shi) ? 1ul : 0ul;
    unsigned long wb = (vb >= slo && vb < shi) ? 1ul : 0ul;
    long x = (long)((ma & ~(0xFFFFul << sh)) | (wa << sh));
    long y = (long)((mb & ~(0xFFFFul << sh)) | (wb << sh));
    vm->stack[vm->sp - 2] = x;
    vm->stack[vm->sp - 1] = y;
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
