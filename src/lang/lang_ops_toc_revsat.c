/* CubalC lang — lang_ops_toc_revsat.c
 * TOC reverse + sat plane: SSUBFROM · reverse mod · sat/clamp duals
 * Peel from toc (B_toc_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_toc_revsat.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_toc_try_revsat(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* digit-9 stack↔cell reverse accumulate: SSUBFROMTOC · SDIVFROMTOC · SMODFROMTOC */
  if (kw(&L->cur,"SSUBFROMTOC")||kw(&L->cur,"SCELLSUBFROM")||kw(&L->cur,"SRSUBCELL")||
      kw(&L->cur,"STACKSUBFROMCELL")||kw(&L->cur,"SSUBFROMCELL")){
    /* i v → cells[i] = v - cells[i], leave result (reverse of SSUBTOC) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = v - vm->cells[(int)i];
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SDIVFROMTOC")||kw(&L->cur,"SCELLDIVFROM")||kw(&L->cur,"SRDIVCELL")||
      kw(&L->cur,"STACKDIVFROMCELL")||kw(&L->cur,"SDIVFROMCELL")){
    /* i v → cells[i] = v / cells[i] (cells[i]==0 → 0), leave quotient */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long c = vm->cells[(int)i];
    long r = c ? (v / c) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMODFROMTOC")||kw(&L->cur,"SCELLMODFROM")||kw(&L->cur,"SRMODCELL")||
      kw(&L->cur,"STACKMODFROMCELL")||kw(&L->cur,"SMODFROMCELL")||kw(&L->cur,"SREMFROMCELL")){
    /* i v → cells[i] = v % cells[i] (cells[i]==0 → 0), leave remainder */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long c = vm->cells[(int)i];
    long r = c ? (v % c) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 reverse imm accumulate TOC: SSUBFROMTOCN · SDIVFROMTOCN · SMODFROMTOCN
   * (imm dual of SSUBFROMTOC/SDIVFROMTOC/SMODFROMTOC; peer of SSUBFROMN into cell) */
  if (kw(&L->cur,"SSUBFROMTOCN")||kw(&L->cur,"SSUBFROMTOCIMM")||kw(&L->cur,"STACKSUBFROMTOCN")||
      kw(&L->cur,"SSUBFROMATN")||kw(&L->cur,"SRSUBTOCN")||kw(&L->cur,"RSUBTOCN")||
      kw(&L->cur,"SCELLSUBFROMN")){
    /* i + n → cells[i] = n - cells[i], leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = n - vm->cells[(int)i];
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SDIVFROMTOCN")||kw(&L->cur,"SDIVFROMTOCIMM")||kw(&L->cur,"STACKDIVFROMTOCN")||
      kw(&L->cur,"SDIVFROMATN")||kw(&L->cur,"SRDIVTOCN")||kw(&L->cur,"RDIVTOCN")||
      kw(&L->cur,"SCELLDIVFROMN")){
    /* i + n → cells[i] = n / cells[i] (cells[i]==0 → 0 soft), leave quotient */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long c = vm->cells[(int)i];
    long r = c ? (n / c) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMODFROMTOCN")||kw(&L->cur,"SMODFROMTOCIMM")||kw(&L->cur,"STACKMODFROMTOCN")||
      kw(&L->cur,"SMODFROMATN")||kw(&L->cur,"SRMODTOCN")||kw(&L->cur,"RMODTOCN")||
      kw(&L->cur,"SCELLMODFROMN")||kw(&L->cur,"SREMFROMTOCN")){
    /* i + n → cells[i] = n % cells[i] (cells[i]==0 → 0 soft), leave remainder */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long c = vm->cells[(int)i];
    long r = c ? (n % c) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 reverse unsigned imm TOC: SUDIVFROMTOCN · SUMODFROMTOCN
   * (imm dual of reverse unsigned after SUDIVFROMN; peer of SDIVFROMTOCN unsigned) */
  if (kw(&L->cur,"SUDIVFROMTOCN")||kw(&L->cur,"SUDIVFROMTOCIMM")||kw(&L->cur,"STACKUDIVFROMTOCN")||
      kw(&L->cur,"SUDIVFROMATN")||kw(&L->cur,"SRUDIVTOCN")||kw(&L->cur,"RUDIVTOCN")||
      kw(&L->cur,"SCELLUDIVFROMN")||kw(&L->cur,"UDIVFROMTOCN")){
    /* i + n → cells[i] = (u)n / (u)cells[i] (cell0 → 0 soft), leave quotient */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long c = (unsigned long)vm->cells[(int)i];
    long r = c ? (long)((unsigned long)n / c) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUMODFROMTOCN")||kw(&L->cur,"SUMODFROMTOCIMM")||kw(&L->cur,"STACKUMODFROMTOCN")||
      kw(&L->cur,"SUMODFROMATN")||kw(&L->cur,"SRUMODTOCN")||kw(&L->cur,"RUMODTOCN")||
      kw(&L->cur,"SCELLUMODFROMN")||kw(&L->cur,"UMODFROMTOCN")||kw(&L->cur,"SUREMFROMTOCN")){
    /* i + n → cells[i] = (u)n % (u)cells[i] (cell0 → 0 soft), leave remainder */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long c = (unsigned long)vm->cells[(int)i];
    long r = c ? (long)((unsigned long)n % c) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 reverse unsigned stack↔cell TOC: SUDIVFROMTOC · SUMODFROMTOC
   * (stack dual of SUDIVFROMTOCN/SUMODFROMTOCN; reverse of SUDIVTOC/SUMODTOC unsigned plane) */
  if (kw(&L->cur,"SUDIVFROMTOC")||kw(&L->cur,"SCELLUDIVFROM")||kw(&L->cur,"STACKUDIVFROMTOC")||
      kw(&L->cur,"SUDIVFROMCELL")||kw(&L->cur,"UDIVFROMTOC")||kw(&L->cur,"SRUDIVTOC")||
      kw(&L->cur,"RUDIVTOC")){
    /* i v → cells[i] = (u)v / (u)cells[i]; cell0 → 0 soft; leave quotient */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long c = (unsigned long)vm->cells[(int)i];
    long r = c ? (long)((unsigned long)v / c) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUMODFROMTOC")||kw(&L->cur,"SCELLUMODFROM")||kw(&L->cur,"STACKUMODFROMTOC")||
      kw(&L->cur,"SUMODFROMCELL")||kw(&L->cur,"UMODFROMTOC")||kw(&L->cur,"SRUMODTOC")||
      kw(&L->cur,"RUMODTOC")||kw(&L->cur,"SUREMFROMTOC")||kw(&L->cur,"UREMFROMTOC")){
    /* i v → cells[i] = (u)v % (u)cells[i]; cell0 → 0 soft; leave remainder */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long c = (unsigned long)vm->cells[(int)i];
    long r = c ? (long)((unsigned long)v % c) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 reverse imm modular TOC: SSUBMODFROMTOCN · SPOWMODFROMTOCN · SMODDIVFROMTOCN
   * (i + args → cells[i] = reverse-mod; imm dual of SSUBMODFROMN after SMODFROMTOCN plane) */
  if (kw(&L->cur,"SSUBMODFROMTOCN")||kw(&L->cur,"SSUBMODFROMTOCIMM")||kw(&L->cur,"STACKSUBMODFROMTOCN")||
      kw(&L->cur,"SSUBMODFROMATN")||kw(&L->cur,"SUBMODFROMTOCN")||kw(&L->cur,"SRSUBMODTOCN")||
      kw(&L->cur,"RSUBMODTOCN")||kw(&L->cur,"SCELLSUBMODFROMN")){
    /* i + k m → cells[i] = (k - cells[i]) mod m; m<=0 → 0; leave result */
    lex_next(L);
    long k = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (m > 0){
      long kk = k % m; if (kk < 0) kk += m;
      long aa = a % m; if (aa < 0) aa += m;
      r = (kk - aa + m) % m;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPOWMODFROMTOCN")||kw(&L->cur,"SPOWMODFROMTOCIMM")||kw(&L->cur,"STACKPOWMODFROMTOCN")||
      kw(&L->cur,"SPOWMODFROMATN")||kw(&L->cur,"POWMODFROMTOCN")||kw(&L->cur,"SRPOWMODTOCN")||
      kw(&L->cur,"RPOWMODTOCN")||kw(&L->cur,"SCELLPOWMODFROMN")||kw(&L->cur,"SBASEPOWMODTOCN")){
    /* i + base m → cells[i] = base^cells[i] mod m; m<=0 or exp<0 → 0; leave result */
    lex_next(L);
    long base_in = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long exp = vm->cells[(int)i];
    long r = 0;
    if (m > 0 && exp >= 0){
      long base = base_in % m; if (base < 0) base += m;
      r = 1 % m;
      long e = exp;
      while (e > 0){
        if (e & 1){
          long y = r, x = base, acc = 0;
          while (y > 0){
            if (y & 1) acc = (acc + x) % m;
            x = (x + x) % m;
            y >>= 1;
          }
          r = acc;
        }
        {
          long x = base, acc = 0, y = base;
          while (y > 0){
            if (y & 1) acc = (acc + x) % m;
            x = (x + x) % m;
            y >>= 1;
          }
          base = acc;
        }
        e >>= 1;
      }
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMODDIVFROMTOCN")||kw(&L->cur,"SMODDIVFROMTOCIMM")||kw(&L->cur,"STACKMODDIVFROMTOCN")||
      kw(&L->cur,"SMODDIVFROMATN")||kw(&L->cur,"MODDIVFROMTOCN")||kw(&L->cur,"SRMODDIVTOCN")||
      kw(&L->cur,"RMODDIVTOCN")||kw(&L->cur,"SCELLMODDIVFROMN")||kw(&L->cur,"SDIVMODFROMTOCN")){
    /* i + a m → cells[i] = a * cells[i]^{-1} mod m; 0 if none / m<=0; leave result */
    lex_next(L);
    long a = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long b = vm->cells[(int)i];
    long r = 0;
    if (m > 0){
      long bb = b % m; if (bb < 0) bb += m;
      if (bb != 0){
        long t = 0, nt = 1;
        long rr = m, nr = bb;
        while (nr != 0){
          long q = rr / nr;
          long tmp = nt; nt = t - q * nt; t = tmp;
          tmp = nr; nr = rr - q * nr; rr = tmp;
        }
        if (rr == 1){
          if (t < 0) t += m;
          long x = a % m; if (x < 0) x += m;
          long y = t, acc = 0;
          while (y > 0){
            if (y & 1) acc = (acc + x) % m;
            x = (x + x) % m;
            y >>= 1;
          }
          r = acc;
        }
      }
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 reverse modular stack↔cell TOC: SSUBMODFROMTOC · SPOWMODFROMTOC · SMODDIVFROMTOC
   * (stack dual of SSUBMODFROMTOCN/SPOWMODFROMTOCN/SMODDIVFROMTOCN; reverse of SPOWMODTOC plane) */
  if (kw(&L->cur,"SSUBMODFROMTOC")||kw(&L->cur,"SCELLSUBMODFROM")||kw(&L->cur,"STACKSUBMODFROMTOC")||
      kw(&L->cur,"SSUBMODFROMCELL")||kw(&L->cur,"SUBMODFROMTOC")||kw(&L->cur,"SRSUBMODTOC")||
      kw(&L->cur,"RSUBMODTOC")){
    /* i k m → cells[i] = (k - cells[i]) mod m; m<=0 → 0; leave result */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long k = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (m > 0){
      long kk = k % m; if (kk < 0) kk += m;
      long aa = a % m; if (aa < 0) aa += m;
      r = (kk - aa + m) % m;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPOWMODFROMTOC")||kw(&L->cur,"SCELLPOWMODFROM")||kw(&L->cur,"STACKPOWMODFROMTOC")||
      kw(&L->cur,"SPOWMODFROMCELL")||kw(&L->cur,"POWMODFROMTOC")||kw(&L->cur,"SRPOWMODTOC")||
      kw(&L->cur,"RPOWMODTOC")||kw(&L->cur,"SBASEPOWMODTOC")){
    /* i base m → cells[i] = base^cells[i] mod m; m<=0 or exp<0 → 0; leave result */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long base_in = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long exp = vm->cells[(int)i];
    long r = 0;
    if (m > 0 && exp >= 0){
      long base = base_in % m; if (base < 0) base += m;
      r = 1 % m;
      long e = exp;
      while (e > 0){
        if (e & 1){
          long y = r, x = base, acc = 0;
          while (y > 0){
            if (y & 1) acc = (acc + x) % m;
            x = (x + x) % m;
            y >>= 1;
          }
          r = acc;
        }
        {
          long x = base, acc = 0, y = base;
          while (y > 0){
            if (y & 1) acc = (acc + x) % m;
            x = (x + x) % m;
            y >>= 1;
          }
          base = acc;
        }
        e >>= 1;
      }
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMODDIVFROMTOC")||kw(&L->cur,"SCELLMODDIVFROM")||kw(&L->cur,"STACKMODDIVFROMTOC")||
      kw(&L->cur,"SMODDIVFROMCELL")||kw(&L->cur,"MODDIVFROMTOC")||kw(&L->cur,"SRMODDIVTOC")||
      kw(&L->cur,"RMODDIVTOC")||kw(&L->cur,"SDIVMODFROMTOC")){
    /* i a m → cells[i] = a * cells[i]^{-1} mod m; 0 if none / m<=0; leave result */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long b = vm->cells[(int)i];
    long r = 0;
    if (m > 0){
      long bb = b % m; if (bb < 0) bb += m;
      if (bb != 0){
        long t = 0, nt = 1;
        long rr = m, nr = bb;
        while (nr != 0){
          long q = rr / nr;
          long tmp = nt; nt = t - q * nt; t = tmp;
          tmp = nr; nr = rr - q * nr; rr = tmp;
        }
        if (rr == 1){
          if (t < 0) t += m;
          long x = a % m; if (x < 0) x += m;
          long y = t, acc = 0;
          while (y > 0){
            if (y & 1) acc = (acc + x) % m;
            x = (x + x) % m;
            y >>= 1;
          }
          r = acc;
        }
      }
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 reverse imm ANDN-plane TOC: SANDNFROMTOCN · SORNFROMTOCN · SXORNFROMTOCN
   * (imm reverse of SANDNTOCN/SORNTOCN/SXORNTOCN; n op ~cells[i] after ANDN energy plane) */
  if (kw(&L->cur,"SANDNFROMTOCN")||kw(&L->cur,"SANDNFROMTOCIMM")||kw(&L->cur,"STACKANDNFROMTOCN")||
      kw(&L->cur,"SANDNFROMATN")||kw(&L->cur,"ANDNFROMTOCN")||kw(&L->cur,"SBICFROMTOCN")||
      kw(&L->cur,"BICFROMTOCN")||kw(&L->cur,"SCELLANDNFROMN")||kw(&L->cur,"SRANDNTOCN")||
      kw(&L->cur,"RANDNTOCN")){
    /* i + n → cells[i] = n & ~cells[i], leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = n & ~vm->cells[(int)i];
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORNFROMTOCN")||kw(&L->cur,"SORNFROMTOCIMM")||kw(&L->cur,"STACKORNFROMTOCN")||
      kw(&L->cur,"SORNFROMATN")||kw(&L->cur,"ORNFROMTOCN")||kw(&L->cur,"SORNOTFROMTOCN")||
      kw(&L->cur,"SCELLORNFROMN")||kw(&L->cur,"SRORNTOCN")||kw(&L->cur,"RORNTOCN")){
    /* i + n → cells[i] = n | ~cells[i], leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = n | ~vm->cells[(int)i];
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXORNFROMTOCN")||kw(&L->cur,"SXORNFROMTOCIMM")||kw(&L->cur,"STACKXORNFROMTOCN")||
      kw(&L->cur,"SXORNFROMATN")||kw(&L->cur,"XORNFROMTOCN")||kw(&L->cur,"SXORNOTFROMTOCN")||
      kw(&L->cur,"SCELLXORNFROMN")||kw(&L->cur,"SRXORNTOCN")||kw(&L->cur,"RXORNTOCN")){
    /* i + n → cells[i] = n ^ ~cells[i], leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = n ^ ~vm->cells[(int)i];
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 reverse imm inverted ANDN TOC: SNANDNFROMTOCN · SNORNFROMTOCN · SXNORNFROMTOCN
   * (i + n → cells[i] = ~(n op ~cells[i]); imm dual of SNANDNFROMN after SANDNFROMTOCN) */
  if (kw(&L->cur,"SNANDNFROMTOCN")||kw(&L->cur,"SNANDNFROMTOCIMM")||kw(&L->cur,"STACKNANDNFROMTOCN")||
      kw(&L->cur,"SNANDNFROMATN")||kw(&L->cur,"NANDNFROMTOCN")||kw(&L->cur,"SCELLNANDNFROMN")||
      kw(&L->cur,"RNANDNFROMTOCN")||kw(&L->cur,"SINVERTANDNFROMTOCN")){
    /* i + n → cells[i] = ~(n & ~cells[i])  (= ~n | cells[i]), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ~(n & ~vm->cells[(int)i]);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNORNFROMTOCN")||kw(&L->cur,"SNORNFROMTOCIMM")||kw(&L->cur,"STACKNORNFROMTOCN")||
      kw(&L->cur,"SNORNFROMATN")||kw(&L->cur,"NORNFROMTOCN")||kw(&L->cur,"SCELLNORNFROMN")||
      kw(&L->cur,"RNORNFROMTOCN")||kw(&L->cur,"SINVERTORNFROMTOCN")){
    /* i + n → cells[i] = ~(n | ~cells[i])  (= ~n & cells[i]), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ~(n | ~vm->cells[(int)i]);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNORNFROMTOCN")||kw(&L->cur,"SXNORNFROMTOCIMM")||kw(&L->cur,"STACKXNORNFROMTOCN")||
      kw(&L->cur,"SXNORNFROMATN")||kw(&L->cur,"XNORNFROMTOCN")||kw(&L->cur,"SCELLXNORNFROMN")||
      kw(&L->cur,"RXNORNFROMTOCN")||kw(&L->cur,"SEQUIVNFROMTOCN")){
    /* i + n → cells[i] = ~(n ^ ~cells[i])  (equiv n ^ cells[i]), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ~(n ^ ~vm->cells[(int)i]);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 reverse sat stack↔cell: SSATSUBFROMTOC · SSATDIVFROMTOC (sat dual of reverse TOC) */
  if (kw(&L->cur,"SSATSUBFROMTOC")||kw(&L->cur,"SCELLSATSUBFROM")||kw(&L->cur,"SSATRSUBCELL")||
      kw(&L->cur,"STACKSATSUBFROMCELL")||kw(&L->cur,"SSATSUBFROMCELL")||kw(&L->cur,"SATSUBFROMTOC")){
    /* i v → cells[i] = sat(v - cells[i]) to LONG_MIN..LONG_MAX, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (a > 0 && v < LONG_MIN + a) r = LONG_MIN;
    else if (a < 0 && v > LONG_MAX + a) r = LONG_MAX;
    else r = v - a;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSATDIVFROMTOC")||kw(&L->cur,"SCELLSATDIVFROM")||kw(&L->cur,"SSATRDIVCELL")||
      kw(&L->cur,"STACKSATDIVFROMCELL")||kw(&L->cur,"SSATDIVFROMCELL")||kw(&L->cur,"SATDIVFROMTOC")){
    /* i v → cells[i] = sat(v / cells[i]); cell==0 → 0; LONG_MIN/-1 → LONG_MAX */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (a == 0) r = 0;
    else if (v == LONG_MIN && a == -1) r = LONG_MAX;
    else r = v / a;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 forward sat stack↔cell energy: SSATADDTOC · SSATSUBTOC · SSATDIVTOC · SSATMULTOC · SCLAMPTOC */
  if (kw(&L->cur,"SSATADDTOC")||kw(&L->cur,"SCELLSATADD")||kw(&L->cur,"SSATADDCELL")||
      kw(&L->cur,"STACKSATADDCELL")||kw(&L->cur,"SATADDTOC")){
    /* i v → cells[i] = sat(cells[i] + v), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (v > 0 && a > LONG_MAX - v) r = LONG_MAX;
    else if (v < 0 && a < LONG_MIN - v) r = LONG_MIN;
    else r = a + v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSATSUBTOC")||kw(&L->cur,"SCELLSATSUB")||kw(&L->cur,"SSATSUBCELL")||
      kw(&L->cur,"STACKSATSUBCELL")||kw(&L->cur,"SATSUBTOC")){
    /* i v → cells[i] = sat(cells[i] - v), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (v > 0 && a < LONG_MIN + v) r = LONG_MIN;
    else if (v < 0 && a > LONG_MAX + v) r = LONG_MAX;
    else r = a - v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSATDIVTOC")||kw(&L->cur,"SCELLSATDIV")||kw(&L->cur,"SSATDIVCELL")||
      kw(&L->cur,"STACKSATDIVCELL")||kw(&L->cur,"SATDIVTOC")){
    /* i v → cells[i] = sat(cells[i] / v); v==0 → 0; LONG_MIN/-1 → LONG_MAX */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (v == 0) r = 0;
    else if (a == LONG_MIN && v == -1) r = LONG_MAX;
    else r = a / v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 sat energy ext: SSATMULTOC · SCLAMPTOC (complete sat TOC + bound) */
  if (kw(&L->cur,"SSATMULTOC")||kw(&L->cur,"SCELLSATMUL")||kw(&L->cur,"SSATMULCELL")||
      kw(&L->cur,"STACKSATMULCELL")||kw(&L->cur,"SATMULTOC")){
    /* i v → cells[i] = sat(cells[i] * v), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (a == 0 || v == 0) r = 0;
    else {
      __int128 p = (__int128)a * (__int128)v;
      if (p > (__int128)LONG_MAX) r = LONG_MAX;
      else if (p < (__int128)LONG_MIN) r = LONG_MIN;
      else r = (long)p;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLAMPTOC")||kw(&L->cur,"SCELLCLAMP")||kw(&L->cur,"SBOUNDTOC")||
      kw(&L->cur,"STACKCLAMPTOC")||kw(&L->cur,"CLAMPTOC")||kw(&L->cur,"SCLIPTOC")){
    /* i lo hi → cells[i] = clamp(cells[i], lo, hi), leave result */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (lo > hi){ long t = lo; lo = hi; hi = t; }
    long a = vm->cells[(int)i];
    long r = a;
    if (r < lo) r = lo;
    if (r > hi) r = hi;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 imm sat TOC: SSATADDTOCN · SSATSUBTOCN · SSATMULTOCN · SCLAMPTOCN
   * (imm dual of SSATADDTOC/SSATSUBTOC/SSATMULTOC/SCLAMPTOC; peer of SSATADDN into cell) */
  if (kw(&L->cur,"SSATADDTOCN")||kw(&L->cur,"SSATADDTOCIMM")||kw(&L->cur,"STACKSATADDTOCN")||
      kw(&L->cur,"SSATADDATN")||kw(&L->cur,"SATADDTOCN")||kw(&L->cur,"SCELLSATADDN")||
      kw(&L->cur,"SADDSATTOCN")){
    /* i + n → cells[i] = sat(cells[i]+n), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (n > 0 && a > LONG_MAX - n) r = LONG_MAX;
    else if (n < 0 && a < LONG_MIN - n) r = LONG_MIN;
    else r = a + n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSATSUBTOCN")||kw(&L->cur,"SSATSUBTOCIMM")||kw(&L->cur,"STACKSATSUBTOCN")||
      kw(&L->cur,"SSATSUBATN")||kw(&L->cur,"SATSUBTOCN")||kw(&L->cur,"SCELLSATSUBN")||
      kw(&L->cur,"SSUBSATTOCN")){
    /* i + n → cells[i] = sat(cells[i]-n), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (n > 0 && a < LONG_MIN + n) r = LONG_MIN;
    else if (n < 0 && a > LONG_MAX + n) r = LONG_MAX;
    else r = a - n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSATMULTOCN")||kw(&L->cur,"SSATMULTOCIMM")||kw(&L->cur,"STACKSATMULTOCN")||
      kw(&L->cur,"SSATMULATN")||kw(&L->cur,"SATMULTOCN")||kw(&L->cur,"SCELLSATMULN")||
      kw(&L->cur,"SMULSATTOCN")){
    /* i + n → cells[i] = sat(cells[i]*n), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (a == 0 || n == 0) r = 0;
    else {
      __int128 p = (__int128)a * (__int128)n;
      if (p > (__int128)LONG_MAX) r = LONG_MAX;
      else if (p < (__int128)LONG_MIN) r = LONG_MIN;
      else r = (long)p;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLAMPTOCN")||kw(&L->cur,"SCLAMPTOCIMM")||kw(&L->cur,"STACKCLAMPTOCN")||
      kw(&L->cur,"SCLAMPATN")||kw(&L->cur,"CLAMPTOCN")||kw(&L->cur,"SCELLCLAMPN")||
      kw(&L->cur,"SBOUNDTOCN")||kw(&L->cur,"BOUNDTOCN")){
    /* i + lo hi → cells[i] = clamp(cells[i], lo, hi), leave result */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (lo > hi){ long t = lo; lo = hi; hi = t; }
    long a = vm->cells[(int)i];
    long r = a;
    if (r < lo) r = lo;
    if (r > hi) r = hi;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 reverse imm sat TOC: SSATDIVTOCN · SSATSUBFROMTOCN · SSATDIVFROMTOCN
   * (complete SSATADDTOCN plane with /; reverse sat dual of SSATSUBFROMTOC/SSATDIVFROMTOC) */
  if (kw(&L->cur,"SSATDIVTOCN")||kw(&L->cur,"SSATDIVTOCIMM")||kw(&L->cur,"STACKSATDIVTOCN")||
      kw(&L->cur,"SSATDIVATN")||kw(&L->cur,"SATDIVTOCN")||kw(&L->cur,"SCELLSATDIVN")||
      kw(&L->cur,"SDIVSATTOCN")){
    /* i + n → cells[i] = sat(cells[i]/n); n==0 → 0; LONG_MIN/-1 → LONG_MAX; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (n == 0) r = 0;
    else if (a == LONG_MIN && n == -1) r = LONG_MAX;
    else r = a / n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSATSUBFROMTOCN")||kw(&L->cur,"SSATSUBFROMTOCIMM")||kw(&L->cur,"STACKSATSUBFROMTOCN")||
      kw(&L->cur,"SSATSUBFROMATN")||kw(&L->cur,"SATSUBFROMTOCN")||kw(&L->cur,"SCELLSATSUBFROMN")||
      kw(&L->cur,"SSATRSUBTOCN")||kw(&L->cur,"SRSATSUBTOCN")||kw(&L->cur,"RSATSUBTOCN")){
    /* i + n → cells[i] = sat(n - cells[i]), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (a > 0 && n < LONG_MIN + a) r = LONG_MIN;
    else if (a < 0 && n > LONG_MAX + a) r = LONG_MAX;
    else r = n - a;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSATDIVFROMTOCN")||kw(&L->cur,"SSATDIVFROMTOCIMM")||kw(&L->cur,"STACKSATDIVFROMTOCN")||
      kw(&L->cur,"SSATDIVFROMATN")||kw(&L->cur,"SATDIVFROMTOCN")||kw(&L->cur,"SCELLSATDIVFROMN")||
      kw(&L->cur,"SSATRDIVTOCN")||kw(&L->cur,"SRSATDIVTOCN")||kw(&L->cur,"RSATDIVTOCN")){
    /* i + n → cells[i] = sat(n / cells[i]); cell0 → 0; LONG_MIN/-1 → LONG_MAX; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (a == 0) r = 0;
    else if (n == LONG_MIN && a == -1) r = LONG_MAX;
    else r = n / a;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
