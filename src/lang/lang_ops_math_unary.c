/* CubalC lang — lang_ops_math_unary.c
 * MATH stack unary: SFACT SILOG SODD SFIB · leftover inv/powmod
 * Peel from math residual (B_math_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_math_unary.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_math_try_unary(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* digit-2/6 stack unary math: SFACT SILOG2 SILOG10 SODD SEVEN SFIB ... */
  if (kw(&L->cur,"SFACT")||kw(&L->cur,"STACKFACT")||kw(&L->cur,"SFACTORIAL")||
      kw(&L->cur,"SILOG2")||kw(&L->cur,"SLOG2")||kw(&L->cur,"STACKILOG2")||
      kw(&L->cur,"SILOG10")||kw(&L->cur,"SLOG10")||kw(&L->cur,"STACKILOG10")||
      kw(&L->cur,"SODD")||kw(&L->cur,"STACKODD")||
      kw(&L->cur,"SEVEN")||kw(&L->cur,"STACKEVEN")||
      kw(&L->cur,"SFIB")||kw(&L->cur,"SFIBONACCI")||kw(&L->cur,"STACKFIB")||
      kw(&L->cur,"SISPRIME")||kw(&L->cur,"SPRIME")||kw(&L->cur,"SPRIMEP")||
      kw(&L->cur,"SISPOW2")||kw(&L->cur,"SISPOWER2")||kw(&L->cur,"SPOW2P")||
      kw(&L->cur,"SPOW2")||kw(&L->cur,"STACKPOW2")||
      kw(&L->cur,"SPOW10")||kw(&L->cur,"STENPOW")||kw(&L->cur,"STACKPOW10")||
      kw(&L->cur,"SNDIGITS")||kw(&L->cur,"SNDIG")||kw(&L->cur,"STACKNDIGITS")||
      kw(&L->cur,"SDIGSUM")||kw(&L->cur,"SDIGITSUM")||kw(&L->cur,"STACKDIGSUM")||
      kw(&L->cur,"SMOBIUS")||kw(&L->cur,"SMU")||kw(&L->cur,"STACKMOBIUS")||
      kw(&L->cur,"SRAD")||kw(&L->cur,"SRADICAL")||kw(&L->cur,"STACKRAD")||
      kw(&L->cur,"SISSQFREE")||kw(&L->cur,"SSQFREE")||kw(&L->cur,"STACKSQFREE")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (strcmp(op,"SFACT")==0 || strcmp(op,"STACKFACT")==0 || strcmp(op,"SFACTORIAL")==0){
      if (a < 0) r = 0;
      else {
        if (a > 20) a = 20;
        r = 1;
        for (long i = 2; i <= a; i++) r *= i;
      }
    } else if (strcmp(op,"SILOG2")==0 || strcmp(op,"SLOG2")==0 || strcmp(op,"STACKILOG2")==0){
      if (a <= 0) r = -1;
      else {
        unsigned long u = (unsigned long)a;
        r = -1;
        while (u){ r++; u >>= 1; }
      }
    } else if (strcmp(op,"SILOG10")==0 || strcmp(op,"SLOG10")==0 || strcmp(op,"STACKILOG10")==0){
      if (a <= 0) r = -1;
      else {
        r = 0;
        long x = a;
        while (x >= 10){ r++; x /= 10; }
      }
    } else if (strcmp(op,"SODD")==0 || strcmp(op,"STACKODD")==0){
      r = (a & 1L) ? 1 : 0;
    } else if (strcmp(op,"SEVEN")==0 || strcmp(op,"STACKEVEN")==0){
      r = (a & 1L) ? 0 : 1;
    } else if (strcmp(op,"SFIB")==0 || strcmp(op,"SFIBONACCI")==0 || strcmp(op,"STACKFIB")==0){
      if (a <= 0) r = 0;
      else if (a == 1 || a == 2) r = 1;
      else {
        if (a > 92) a = 92;
        long f0 = 0, f1 = 1;
        for (long i = 2; i <= a; i++){
          long f2 = f0 + f1;
          f0 = f1; f1 = f2;
        }
        r = f1;
      }
    } else if (strcmp(op,"SISPRIME")==0 || strcmp(op,"SPRIME")==0 || strcmp(op,"SPRIMEP")==0){
      if (a <= 1) r = 0;
      else if (a <= 3) r = 1;
      else if ((a % 2) == 0 || (a % 3) == 0) r = 0;
      else {
        r = 1;
        for (long i = 5; i * i <= a; i += 6){
          if ((a % i) == 0 || (a % (i + 2)) == 0){ r = 0; break; }
        }
      }
    } else if (strcmp(op,"SMOBIUS")==0 || strcmp(op,"SMU")==0 || strcmp(op,"STACKMOBIUS")==0){
      if (a <= 0) r = 0;
      else if (a == 1) r = 1;
      else {
        long n = a; int k = 0; r = 1;
        if ((n % 2) == 0){ n /= 2; k++; if ((n % 2) == 0) r = 0; }
        if (r){
          for (long p = 3; p * p <= n; p += 2){
            if ((n % p) == 0){ n /= p; k++; if ((n % p) == 0){ r = 0; break; } }
          }
          if (r){ if (n > 1) k++; r = (k & 1) ? -1 : 1; }
        }
      }
    } else if (strcmp(op,"SRAD")==0 || strcmp(op,"SRADICAL")==0 || strcmp(op,"STACKRAD")==0){
      if (a <= 0) r = 0;
      else if (a == 1) r = 1;
      else {
        long n = a; r = 1;
        if ((n % 2) == 0){ r *= 2; while ((n % 2) == 0) n /= 2; }
        for (long p = 3; p * p <= n; p += 2){
          if ((n % p) == 0){ r *= p; while ((n % p) == 0) n /= p; }
        }
        if (n > 1) r *= n;
      }
    } else if (strcmp(op,"SISSQFREE")==0 || strcmp(op,"SSQFREE")==0 || strcmp(op,"STACKSQFREE")==0){
      if (a <= 0) r = 0;
      else if (a == 1) r = 1;
      else {
        long n = a; r = 1;
        if ((n % 2) == 0){ n /= 2; if ((n % 2) == 0) r = 0; }
        if (r){
          for (long p = 3; p * p <= n; p += 2){
            if ((n % p) == 0){ n /= p; if ((n % p) == 0){ r = 0; break; } }
          }
        }
      }
    } else if (strcmp(op,"SISPOW2")==0 || strcmp(op,"SISPOWER2")==0 || strcmp(op,"SPOW2P")==0){
      if (a <= 0) r = 0;
      else {
        unsigned long u = (unsigned long)a;
        r = ((u & (u - 1ul)) == 0ul) ? 1 : 0;
      }
    } else if (strcmp(op,"SPOW2")==0 || strcmp(op,"STACKPOW2")==0){
      if (a < 0 || a > 62) r = 0;
      else r = 1L << a;
    } else if (strcmp(op,"SPOW10")==0 || strcmp(op,"STENPOW")==0 || strcmp(op,"STACKPOW10")==0){
      if (a < 0 || a > 18) r = 0;
      else {
        r = 1;
        for (long i = 0; i < a; i++) r *= 10;
      }
    } else if (strcmp(op,"SNDIGITS")==0 || strcmp(op,"SNDIG")==0 || strcmp(op,"STACKNDIGITS")==0){
      long x = a < 0 ? -a : a;
      if (x == 0) r = 1;
      else {
        r = 0;
        while (x){ r++; x /= 10; }
      }
    } else {
      /* SDIGSUM / SDIGITSUM / STACKDIGSUM */
      long x = a < 0 ? -a : a;
      r = 0;
      if (x == 0) r = 0;
      else while (x){ r += x % 10; x /= 10; }
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMODINV")||kw(&L->cur,"SINVMOD")||kw(&L->cur,"STACKMODINV")){
    /* a m → a^{-1} mod m (0 if none) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
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
        if (rr == 1){
          if (t < 0) t += m;
          r = t;
        }
      }
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMULMOD")||kw(&L->cur,"STACKMULMOD")||
      kw(&L->cur,"SPOWMOD")||kw(&L->cur,"STACKPOWMOD")){
    /* a b m → (a*b)%m or pow(a,b)%m */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (m <= 0) r = 0;
    else if (strcmp(op,"SMULMOD")==0 || strcmp(op,"STACKMULMOD")==0){
      long x = a % m; if (x < 0) x += m;
      long y = b % m; if (y < 0) y += m;
      r = 0;
      while (y > 0){
        if (y & 1) r = (r + x) % m;
        x = (x + x) % m;
        y >>= 1;
      }
    } else {
      /* SPOWMOD / STACKPOWMOD */
      long base = a % m; if (base < 0) base += m;
      long exp = b;
      if (exp < 0) r = 0;
      else {
        r = 1 % m;
        while (exp > 0){
          if (exp & 1){
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
          exp >>= 1;
        }
      }
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
