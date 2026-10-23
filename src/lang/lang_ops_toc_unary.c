/* CubalC lang — lang_ops_toc_unary.c
 * TOC unary/scale/numthy: SABS/SNEG/SNOT · SDBL/HALF · SINC/DEC · SSQR · SGCD/LCM
 * Peel from toc (B_toc_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_toc_unary.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_toc_try_unary(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* digit-7 stack↔cell unary ALU: SABSTOC · SNEGTOC · SNOTTOC (dual of SABS/SNEG/SNOT) */
  if (kw(&L->cur,"SABSTOC")||kw(&L->cur,"SABSCELL")||kw(&L->cur,"STACKABSTOC")||
      kw(&L->cur,"SCELLABS")||kw(&L->cur,"ABSAT")){
    /* i → cells[i] = |cells[i]|, leave abs (LONG_MIN stays LONG_MIN on wrap) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long v = vm->cells[(int)i];
    if (v < 0) v = -v;
    vm->cells[(int)i] = v;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNEGTOC")||kw(&L->cur,"SNEGCELL")||kw(&L->cur,"STACKNEGTOC")||
      kw(&L->cur,"SCELLNEG")||kw(&L->cur,"NEGAT")){
    /* i → cells[i] = -cells[i], leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long v = -vm->cells[(int)i];
    vm->cells[(int)i] = v;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNOTTOC")||kw(&L->cur,"SNOTTOCELL")||kw(&L->cur,"STACKNOTTOC")||
      kw(&L->cur,"BINVTOC")||kw(&L->cur,"SBNOTAT")){
    /* i → cells[i] = ~cells[i], leave result (avoid SNOTCELL range alias) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long v = ~vm->cells[(int)i];
    vm->cells[(int)i] = v;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack↔cell unary scale: SDBLTOC · SHALFTOC (dual of SDBL/SHALF after ABS/NEG/NOT TOC) */
  if (kw(&L->cur,"SDBLTOC")||kw(&L->cur,"SDBLTOCELL")||kw(&L->cur,"STACKDBLTOC")||
      kw(&L->cur,"SDOUBLETOC")||kw(&L->cur,"SCELLDBL")||kw(&L->cur,"SDBLAT")||
      kw(&L->cur,"DBLTOC")){
    /* i → cells[i] *= 2, leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long v = vm->cells[(int)i] * 2;
    vm->cells[(int)i] = v;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHALFTOC")||kw(&L->cur,"SHALFTOCELL")||kw(&L->cur,"STACKHALFTOC")||
      kw(&L->cur,"SHALVETOC")||kw(&L->cur,"SCELLHALF")||kw(&L->cur,"SHALFAT")||
      kw(&L->cur,"HALFTOC")){
    /* i → cells[i] /= 2 (toward zero), leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long v = vm->cells[(int)i] / 2;
    vm->cells[(int)i] = v;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack↔cell unary mutator: SINCTOC · SDECTOC (dual of SINC/SDEC after SDBL/SHALF TOC) */
  if (kw(&L->cur,"SINCTOC")||kw(&L->cur,"SINCTOCELL")||kw(&L->cur,"STACKINCTOC")||
      kw(&L->cur,"SINCELLTOC")||kw(&L->cur,"SINCAT")||kw(&L->cur,"INCTOC")||
      kw(&L->cur,"SCELLINCTOC")){
    /* i → cells[i] += 1, leave result (TOC dual of SINC; peer of SINCCELL) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long v = vm->cells[(int)i] + 1;
    vm->cells[(int)i] = v;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SDECTOC")||kw(&L->cur,"SDECTOCELL")||kw(&L->cur,"STACKDECTOC")||
      kw(&L->cur,"SDECELLTOC")||kw(&L->cur,"SDECAT")||kw(&L->cur,"DECTOC")||
      kw(&L->cur,"SCELLDECTOC")){
    /* i → cells[i] -= 1, leave result (TOC dual of SDEC; peer of SDECCELL) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long v = vm->cells[(int)i] - 1;
    vm->cells[(int)i] = v;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 stack↔cell math dual: SSQRTOC · SISQRTTOC (dual of SSQR/SISQRT after scale TOC) */
  if (kw(&L->cur,"SSQRTOC")||kw(&L->cur,"SSQRTOCELL")||kw(&L->cur,"STACKSQRTOC")||
      kw(&L->cur,"SSQUARETOC")||kw(&L->cur,"SCELLSQR")||kw(&L->cur,"SQRTOC")||
      kw(&L->cur,"SSQRAT")){
    /* i → cells[i] = cells[i]*cells[i], leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long v = a * a;
    vm->cells[(int)i] = v;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SISQRTTOC")||kw(&L->cur,"SISQRTTOCELL")||kw(&L->cur,"STACKISQRTTOC")||
      kw(&L->cur,"SSQRTTOC")||kw(&L->cur,"SCELLISQRT")||kw(&L->cur,"ISQRTTOC")||
      kw(&L->cur,"SISQRTAT")){
    /* i → cells[i] = isqrt(cells[i]); neg → 0, leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long v = 0;
    if (a < 0) v = 0;
    else {
      long t = 0;
      while ((t + 1) * (t + 1) <= a) t++;
      v = t;
    }
    vm->cells[(int)i] = v;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 imm numthy TOC: SGCDTOCN · SLCMTOCN
   * (imm dual of SGCDN/SLCMN into cell; complete SSQRTOC plane with gcd/lcm) */
  if (kw(&L->cur,"SGCDTOCN")||kw(&L->cur,"SGCDTOCIMM")||kw(&L->cur,"STACKGCDTOCN")||
      kw(&L->cur,"SGCDATN")||kw(&L->cur,"GCDTOCN")||kw(&L->cur,"SCELLGCDN")||
      kw(&L->cur,"GCDINTOCN")){
    /* i + n → cells[i] = gcd(|cells[i]|,|n|), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long x = a < 0 ? -a : a, y = n < 0 ? -n : n;
    while (y){ long t = x % y; x = y; y = t; }
    long r = x;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SLCMTOCN")||kw(&L->cur,"SLCMTOCIMM")||kw(&L->cur,"STACKLCMTOCN")||
      kw(&L->cur,"SLCMATN")||kw(&L->cur,"LCMTOCN")||kw(&L->cur,"SCELLLCMN")||
      kw(&L->cur,"LCMINTOCN")){
    /* i + n → cells[i] = lcm(|cells[i]|,|n|); 0 if either side 0; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long x = a < 0 ? -a : a, y = n < 0 ? -n : n;
    long r = 0;
    if (x && y){
      long g = x, h = y;
      while (h){ long t = g % h; g = h; h = t; }
      r = (x / g) * y;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
