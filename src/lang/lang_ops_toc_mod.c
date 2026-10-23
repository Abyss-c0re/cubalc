/* CubalC lang — lang_ops_toc_mod.c
 * TOC modular plane: SADDMOD..SMULMOD · SPOWMOD · SMODDIV · SMODINV (imm + stack)
 * Peel from toc (B_toc_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_toc_mod.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_toc_try_mod(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* digit-3 forward imm modular TOC: SADDMODTOCN · SSUBMODTOCN · SMULMODTOCN
   * (i + k m → cells[i] = (cells[i] op k) mod m; imm dual of SADDMODN after reverse modular TOC) */
  if (kw(&L->cur,"SADDMODTOCN")||kw(&L->cur,"SADDMODTOCIMM")||kw(&L->cur,"STACKADDMODTOCN")||
      kw(&L->cur,"SADDMODATN")||kw(&L->cur,"ADDMODTOCN")||kw(&L->cur,"SCELLADDMODN")){
    /* i + k m → cells[i] = (cells[i]+k) mod m; m<=0 → 0; leave result */
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
      r = (aa + kk) % m;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSUBMODTOCN")||kw(&L->cur,"SSUBMODTOCIMM")||kw(&L->cur,"STACKSUBMODTOCN")||
      kw(&L->cur,"SSUBMODATN")||kw(&L->cur,"SUBMODTOCN")||kw(&L->cur,"SCELLSUBMODN")){
    /* i + k m → cells[i] = (cells[i]-k) mod m; m<=0 → 0; leave result */
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
      r = (aa - kk + m) % m;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMULMODTOCN")||kw(&L->cur,"SMULMODTOCIMM")||kw(&L->cur,"STACKMULMODTOCN")||
      kw(&L->cur,"SMULMODATN")||kw(&L->cur,"MULMODTOCN")||kw(&L->cur,"SCELLMULMODN")){
    /* i + k m → cells[i] = (cells[i]*k) mod m; m<=0 → 0; leave result */
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
      long acc = 0, xx = aa, kk2 = kk;
      while (kk2 > 0){
        if (kk2 & 1) acc = (acc + xx) % m;
        xx = (xx + xx) % m;
        kk2 >>= 1;
      }
      r = acc;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack↔cell modular accumulate TOC: SADDMODTOC · SSUBMODTOC · SMULMODTOC
   * (stack dual of SADDMODTOCN/SSUBMODTOCN/SMULMODTOCN; modular peer of SADDTOC after imm plane) */
  if (kw(&L->cur,"SADDMODTOC")||kw(&L->cur,"SCELLADDMOD")||kw(&L->cur,"STACKADDMODTOC")||
      kw(&L->cur,"SADDMODTOCELL")||kw(&L->cur,"ADDMODTOC")||kw(&L->cur,"SACCUMMODTOC")){
    /* i k m → cells[i] = (cells[i]+k) mod m; m<=0 → 0; leave result */
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
      r = (aa + kk) % m;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSUBMODTOC")||kw(&L->cur,"SCELLSUBMOD")||kw(&L->cur,"STACKSUBMODTOC")||
      kw(&L->cur,"SSUBMODTOCELL")||kw(&L->cur,"SUBMODTOC")){
    /* i k m → cells[i] = (cells[i]-k) mod m; m<=0 → 0; leave result */
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
      r = (aa - kk + m) % m;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMULMODTOC")||kw(&L->cur,"SCELLMULMOD")||kw(&L->cur,"STACKMULMODTOC")||
      kw(&L->cur,"SMULMODTOCELL")||kw(&L->cur,"MULMODTOC")){
    /* i k m → cells[i] = (cells[i]*k) mod m; m<=0 → 0; leave result */
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
      long acc = 0, xx = aa, kk2 = kk;
      while (kk2 > 0){
        if (kk2 & 1) acc = (acc + xx) % m;
        xx = (xx + xx) % m;
        kk2 >>= 1;
      }
      r = acc;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 forward imm modular inv/pow/div TOC: SPOWMODTOCN · SMODDIVTOCN · SMODINVTOCN
   * (i + args → cells[i] = inv/pow/div-mod; imm dual of SPOWMODN after SADDMODTOCN plane) */
  if (kw(&L->cur,"SPOWMODTOCN")||kw(&L->cur,"SPOWMODTOCIMM")||kw(&L->cur,"STACKPOWMODTOCN")||
      kw(&L->cur,"SPOWMODATN")||kw(&L->cur,"POWMODTOCN")||kw(&L->cur,"SCELLPOWMODN")||
      kw(&L->cur,"SEXPMMODTOCN")){
    /* i + exp m → cells[i] = cells[i]^exp mod m; m<=0 or exp<0 → 0; leave result */
    lex_next(L);
    long exp = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (m > 0 && exp >= 0){
      long base = a % m; if (base < 0) base += m;
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
  if (kw(&L->cur,"SMODDIVTOCN")||kw(&L->cur,"SMODDIVTOCIMM")||kw(&L->cur,"STACKMODDIVTOCN")||
      kw(&L->cur,"SMODDIVATN")||kw(&L->cur,"MODDIVTOCN")||kw(&L->cur,"SCELLMODDIVN")||
      kw(&L->cur,"SDIVMODTOCN")||kw(&L->cur,"DIVMODMTOCN")){
    /* i + b m → cells[i] = cells[i] * b^{-1} mod m; 0 if none / m<=0; leave result */
    lex_next(L);
    long b = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
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
  if (kw(&L->cur,"SMODINVTOCN")||kw(&L->cur,"SMODINVTOCIMM")||kw(&L->cur,"STACKMODINVTOCN")||
      kw(&L->cur,"SMODINVATN")||kw(&L->cur,"MODINVTOCN")||kw(&L->cur,"SCELLMODINVN")||
      kw(&L->cur,"SINVMODTOCN")||kw(&L->cur,"INVMODTOCN")){
    /* i + m → cells[i] = cells[i]^{-1} mod m; 0 if none / m<=1; leave result */
    lex_next(L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (m > 1){
      long aa = a % m; if (aa < 0) aa += m;
      if (aa != 0){
        long t = 0, nt = 1;
        long rr = m, nr = aa;
        while (nr != 0){
          long q = rr / nr;
          long tmp = nt; nt = t - q * nt; t = tmp;
          tmp = nr; nr = rr - q * nr; rr = tmp;
        }
        if (rr == 1){ if (t < 0) t += m; r = t; }
      }
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 stack↔cell modular inv/pow/div TOC: SPOWMODTOC · SMODDIVTOC · SMODINVTOC
   * (stack dual of SPOWMODTOCN/SMODDIVTOCN/SMODINVTOCN; complete modular TOC after SADDMODTOCN) */
  if (kw(&L->cur,"SPOWMODTOC")||kw(&L->cur,"SCELLPOWMOD")||kw(&L->cur,"STACKPOWMODTOC")||
      kw(&L->cur,"SPOWMODTOCELL")||kw(&L->cur,"POWMODTOC")||kw(&L->cur,"SEXPMMODTOC")){
    /* i exp m → cells[i] = cells[i]^exp mod m; m<=0 or exp<0 → 0; leave result */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long exp = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (m > 0 && exp >= 0){
      long base = a % m; if (base < 0) base += m;
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
  if (kw(&L->cur,"SMODDIVTOC")||kw(&L->cur,"SCELLMODDIV")||kw(&L->cur,"STACKMODDIVTOC")||
      kw(&L->cur,"SMODDIVTOCELL")||kw(&L->cur,"MODDIVTOC")||kw(&L->cur,"SDIVMODMTOC")||
      kw(&L->cur,"DIVMODMTOC")){
    /* i b m → cells[i] = cells[i] * b^{-1} mod m; 0 if none / m<=0; leave result */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
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
  if (kw(&L->cur,"SMODINVTOC")||kw(&L->cur,"SCELLMODINV")||kw(&L->cur,"STACKMODINVTOC")||
      kw(&L->cur,"SMODINVTOCELL")||kw(&L->cur,"MODINVTOC")||kw(&L->cur,"SINVMODTOC")||
      kw(&L->cur,"INVMODTOC")){
    /* i m → cells[i] = cells[i]^{-1} mod m; 0 if none / m<=1; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (m > 1){
      long aa = a % m; if (aa < 0) aa += m;
      if (aa != 0){
        long t = 0, nt = 1;
        long rr = m, nr = aa;
        while (nr != 0){
          long q = rr / nr;
          long tmp = nt; nt = t - q * nt; t = tmp;
          tmp = nr; nr = rr - q * nr; rr = tmp;
        }
        if (rr == 1){ if (t < 0) t += m; r = t; }
      }
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
