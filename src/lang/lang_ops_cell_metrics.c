/* CubalC lang — lang_ops_cell_metrics.c
 * CELL bit-metrics: PARITYCELL·BSWAP·POPCNT + CLZ·CTZ·BSWAP16 (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_metrics.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_cell_try_metrics(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* digit-9 cell bit-metrics plane: PARITYCELL · BSWAPCELL · POPCNTCELL
   * (range dual of SPARITY/SBSWAP/SPOPCNT after BITREV32CELL + SHL32CELL plane) */
  if (kw(&L->cur,"PARITYCELL")||kw(&L->cur,"CELLPARITY")||kw(&L->cur,"BPARITYCELL")||
      kw(&L->cur,"RANGEPARITY")||kw(&L->cur,"PARITYRANGE")||kw(&L->cur,"PARCELL")){
    /* PARITYCELL lo hi — cells[i] = xor of all bits (parity) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned long u = (unsigned long)vm->cells[(int)i];
      long r = 0;
      while (u){ r ^= (long)(u & 1ul); u >>= 1; }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"BSWAPCELL")||kw(&L->cur,"BSWAP32CELL")||kw(&L->cur,"CELLBSWAP")||
      kw(&L->cur,"RANGEBSWAP")||kw(&L->cur,"BSWAPRANGE")||kw(&L->cur,"BBSWAPCELL")){
    /* BSWAPCELL lo hi — cells[i] = bswap32(low32 cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i];
      w = ((w & 0x000000FFu) << 24) | ((w & 0x0000FF00u) << 8) |
          ((w & 0x00FF0000u) >> 8) | ((w & 0xFF000000u) >> 24);
      vm->cells[(int)i] = (long)w;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"POPCNTCELL")||kw(&L->cur,"CELLPOPCNT")||kw(&L->cur,"BPOPCNTCELL")||
      kw(&L->cur,"RANGEPOPCNT")||kw(&L->cur,"POPCNTRANGE")||kw(&L->cur,"PCNTCELL")||
      kw(&L->cur,"HAMMINGCELL")){
    /* POPCNTCELL lo hi — cells[i] = popcount(cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned long u = (unsigned long)vm->cells[(int)i];
      long r = 0;
      while (u){ r += (long)(u & 1ul); u >>= 1; }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  /* bit-metrics ext peel (CLZCELL/CTZCELL/BSWAP16CELL) from cell.c — G_cell_margin */
  /* digit-4 cell bit-metrics ext: CLZCELL · CTZCELL · BSWAP16CELL
   * (complete popcnt/clz/ctz + bswap16 ladder after PARITYCELL/BSWAPCELL/POPCNTCELL) */
  if (kw(&L->cur,"CLZCELL")||kw(&L->cur,"CELLCLZ")||kw(&L->cur,"BCLZCELL")||
      kw(&L->cur,"RANGECLZ")||kw(&L->cur,"CLZRANGE")||kw(&L->cur,"NLZCELL")){
    /* CLZCELL lo hi — cells[i] = clz64(cells[i]) (0 → 64) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned long u = (unsigned long)vm->cells[(int)i];
      long r = 0;
      if (u == 0) r = 64;
      else {
        for (int b = 63; b >= 0; b--){
          if (u & (1ul << (unsigned)b)) break;
          r++;
        }
      }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CTZCELL")||kw(&L->cur,"CELLCTZ")||kw(&L->cur,"BCTZCELL")||
      kw(&L->cur,"RANGECTZ")||kw(&L->cur,"CTZRANGE")||kw(&L->cur,"NTZCELL")){
    /* CTZCELL lo hi — cells[i] = ctz64(cells[i]) (0 → 64) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned long u = (unsigned long)vm->cells[(int)i];
      long r = 0;
      if (u == 0) r = 64;
      else {
        while ((u & 1ul) == 0){ r++; u >>= 1; }
      }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"BSWAP16CELL")||kw(&L->cur,"CELLBSWAP16")||kw(&L->cur,"BBSWAP16CELL")||
      kw(&L->cur,"RANGEBSWAP16")||kw(&L->cur,"BSWAP16RANGE")){
    /* BSWAP16CELL lo hi — cells[i] = bswap16(low16 cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFFFu;
      w = ((w & 0x00FFu) << 8) | ((w & 0xFF00u) >> 8);
      vm->cells[(int)i] = (long)w;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
