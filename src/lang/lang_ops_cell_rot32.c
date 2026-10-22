/* CubalC lang — lang_ops_cell_rot32.c
 * CELL fixed-width 32 rotate/bitrev: ROL32CELL·ROR32CELL·BITREV32CELL (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_rot32.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_cell_try_rot32(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-4 cell fixed-width rotate32 + bitrev plane: ROL32CELL · ROR32CELL · BITREV32CELL
   * (range dual of SROTL32/SROTR32/SBITREV32 after SHL32CELL plane; control/bitfield digit-4) */
  if (kw(&L->cur,"ROL32CELL")||kw(&L->cur,"ROTL32CELL")||kw(&L->cur,"BROL32CELL")||
      kw(&L->cur,"RANGEROL32")||kw(&L->cur,"ROL32RANGE")||kw(&L->cur,"CELLROTL32")){
    /* ROL32CELL lo hi k — cells[i] = rotl32(low32 cells[i], k&31) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 31;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i];
      long r = (kk == 0) ? (long)w : (long)(((w << kk) | (w >> (32 - kk))));
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ROR32CELL")||kw(&L->cur,"ROTR32CELL")||kw(&L->cur,"BROR32CELL")||
      kw(&L->cur,"RANGEROR32")||kw(&L->cur,"ROR32RANGE")||kw(&L->cur,"CELLROTR32")){
    /* ROR32CELL lo hi k — cells[i] = rotr32(low32 cells[i], k&31) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 31;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i];
      long r = (kk == 0) ? (long)w : (long)(((w >> kk) | (w << (32 - kk))));
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"BITREV32CELL")||kw(&L->cur,"BREV32CELL")||kw(&L->cur,"REV32CELL")||
      kw(&L->cur,"RANGEBITREV32")||kw(&L->cur,"BITREV32RANGE")||kw(&L->cur,"CELLBITREV32R")){
    /* BITREV32CELL lo hi — cells[i] = bitrev32(low32 cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i];
      unsigned int rv = 0;
      for (int b = 0; b < 32; b++){
        rv = (rv << 1) | (w & 1u);
        w >>= 1;
      }
      vm->cells[(int)i] = (long)rv;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
