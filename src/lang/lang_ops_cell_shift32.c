/* CubalC lang — lang_ops_cell_shift32.c
 * CELL fixed-width 32 shift ALU: SHL32CELL·SHR32CELL·SAR32CELL (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_shift32.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_cell_try_shift32(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-9 cell fixed-width shift32 plane: SHL32CELL · SHR32CELL · SAR32CELL
   * (range dual of SSHL32/SSHR32/SSAR32 and SSHL32TOC plane after NANDCELL logic) */
  if (kw(&L->cur,"SHL32CELL")||kw(&L->cur,"BSHL32CELL")||kw(&L->cur,"RANGESHL32")||
      kw(&L->cur,"LSH32CELL")||kw(&L->cur,"SHL32RANGE")){
    /* SHL32CELL lo hi k — cells[i] = (uint32)cells[i] << k (k>=32 → 0) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i];
      vm->cells[(int)i] = (kk >= 32) ? 0L : (long)(w << kk);
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHR32CELL")||kw(&L->cur,"BSHR32CELL")||kw(&L->cur,"RANGESHR32")||
      kw(&L->cur,"LSHR32CELL")||kw(&L->cur,"SHR32RANGE")){
    /* SHR32CELL lo hi k — cells[i] = (uint32)cells[i] >> k logical (k>=32 → 0) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i];
      vm->cells[(int)i] = (kk >= 32) ? 0L : (long)(w >> kk);
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SAR32CELL")||kw(&L->cur,"BSAR32CELL")||kw(&L->cur,"RANGESAR32")||
      kw(&L->cur,"ASHR32CELL")||kw(&L->cur,"SAR32RANGE")||kw(&L->cur,"ASHRCELL32")){
    /* SAR32CELL lo hi k — arithmetic right shift low32 (k>=32 → all sign) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    for (long i=lo;i<=hi;i++){
      long va = (long)(int)(unsigned int)vm->cells[(int)i];
      long r;
      if (kk >= 32) r = (va < 0) ? -1L : 0L;
      else r = va >> kk;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
