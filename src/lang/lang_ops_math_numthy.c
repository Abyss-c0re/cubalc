/* CubalC lang — lang_ops_math_numthy.c
 * MATH stack number theory: SPOW SGCD SLCM · roots/primes · imm duals
 * Peel from math residual (B_math_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_math_numthy.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_math_try_numthy(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* digit-2 stack number theory / div modes: SPOW SGCD SLCM SSQR SISQRT SDIVCEIL SDIVFLOOR */
  if (kw(&L->cur,"SSQR")||kw(&L->cur,"STACKSQR")||kw(&L->cur,"SSQUARE")||
      kw(&L->cur,"SISQRT")||kw(&L->cur,"SSQRT")||kw(&L->cur,"STACKISQRT")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (strcmp(op,"SSQR")==0 || strcmp(op,"STACKSQR")==0 || strcmp(op,"SSQUARE")==0)
      r = a * a;
    else {
      /* SISQRT / SSQRT / STACKISQRT — integer square root; neg → 0 */
      if (a < 0) r = 0;
      else {
        long t = 0;
        while ((t + 1) * (t + 1) <= a) t++;
        r = t;
      }
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPOW")||kw(&L->cur,"STACKPOW")||kw(&L->cur,"SPOWER")||
      kw(&L->cur,"SGCD")||kw(&L->cur,"STACKGCD")||
      kw(&L->cur,"SLCM")||kw(&L->cur,"STACKLCM")||
      kw(&L->cur,"SDIVCEIL")||kw(&L->cur,"SCEILDIV")||kw(&L->cur,"STACKDIVCEIL")||
      kw(&L->cur,"SDIVFLOOR")||kw(&L->cur,"SFLOORDIV")||kw(&L->cur,"STACKDIVFLOOR")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (strcmp(op,"SPOW")==0 || strcmp(op,"STACKPOW")==0 || strcmp(op,"SPOWER")==0){
      long e = b;
      if (e < 0) r = 0;
      else {
        r = 1;
        while (e-- > 0) r *= a;
      }
    } else if (strcmp(op,"SGCD")==0 || strcmp(op,"STACKGCD")==0){
      long x = a < 0 ? -a : a, y = b < 0 ? -b : b;
      while (y){ long t = x % y; x = y; y = t; }
      r = x;
    } else if (strcmp(op,"SLCM")==0 || strcmp(op,"STACKLCM")==0){
      long x = a < 0 ? -a : a, y = b < 0 ? -b : b;
      if (!x || !y) r = 0;
      else {
        long g = x, h = y;
        while (h){ long t = g % h; g = h; h = t; }
        r = (x / g) * y;
      }
    } else if (strcmp(op,"SDIVCEIL")==0 || strcmp(op,"SCEILDIV")==0 ||
               strcmp(op,"STACKDIVCEIL")==0){
      if (b == 0) r = 0;
      else if (a >= 0 && b > 0) r = (a + b - 1) / b;
      else if (a <= 0 && b < 0){
        long aa = -a, bb = -b;
        r = (aa + bb - 1) / bb;
      } else r = a / b;
    } else {
      /* SDIVFLOOR / SFLOORDIV / STACKDIVFLOOR */
      if (b == 0) r = 0;
      else {
        long q = a / b, rem = a % b;
        if (rem != 0 && ((a < 0) != (b < 0))) q--;
        r = q;
      }
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 stack imm numthy: SGCDN · SLCMN (imm dual of SGCD/SLCM) */
  if (kw(&L->cur,"SGCDN")||kw(&L->cur,"STACKGCDN")||kw(&L->cur,"GCDN")||
      kw(&L->cur,"SGCDIMM")||kw(&L->cur,"GCDIMM")){
    /* SGCDN n — TOS = gcd(|TOS|,|n|) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long x = a < 0 ? -a : a, y = n < 0 ? -n : n;
    while (y){ long t = x % y; x = y; y = t; }
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SLCMN")||kw(&L->cur,"STACKLCMN")||kw(&L->cur,"LCMN")||
      kw(&L->cur,"SLCMIMM")||kw(&L->cur,"LCMIMM")){
    /* SLCMN n — TOS = lcm(|TOS|,|n|); 0 if either side 0 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long x = a < 0 ? -a : a, y = n < 0 ? -n : n;
    long r = 0;
    if (x && y){
      long g = x, h = y;
      while (h){ long t = g % h; g = h; h = t; }
      r = (x / g) * y;
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 stack imm numthy ext: SCOPRIMEN · SPOWN (imm dual of SCOPRIME/SPOW after SGCDN) */
  if (kw(&L->cur,"SCOPRIMEN")||kw(&L->cur,"SISCOPRIMEN")||kw(&L->cur,"STACKCOPRIMEN")||
      kw(&L->cur,"COPRIMEN")||kw(&L->cur,"SCOPRIMEIMM")||kw(&L->cur,"ISCOPRIMEN")){
    /* SCOPRIMEN n — TOS = 1 if gcd(|TOS|,|n|)==1 else 0; (0,0)→0 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long x = a < 0 ? -a : a, y = n < 0 ? -n : n;
    long r = 0;
    if (!(x == 0 && y == 0)){
      while (y){ long t = x % y; x = y; y = t; }
      r = (x == 1) ? 1 : 0;
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPOWN")||kw(&L->cur,"STACKPOWN")||kw(&L->cur,"POWN")||
      kw(&L->cur,"SPOWIMM")||kw(&L->cur,"POWERN")||kw(&L->cur,"SEXPN")){
    /* SPOWN n — TOS = TOS^n; n<0 → 0 (match SPOW) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (n < 0) r = 0;
    else {
      r = 1;
      long e = n;
      while (e-- > 0) r *= a;
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 stack: SCOPRIME · SCEILPOW2 · SEGCD */
  if (kw(&L->cur,"SCOPRIME")||kw(&L->cur,"SISCOPRIME")||kw(&L->cur,"STACKCOPRIME")){
    /* a b → 1 if gcd==1 */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long x = a < 0 ? -a : a, y = b < 0 ? -b : b;
    long r = 0;
    if (!(x == 0 && y == 0)){
      while (y){ long t = x % y; x = y; y = t; }
      r = (x == 1) ? 1 : 0;
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCEILPOW2")||kw(&L->cur,"SNEXTPOW2")||kw(&L->cur,"SCPOW2")||
      kw(&L->cur,"STACKCEILPOW2")||kw(&L->cur,"STACKNEXTPOW2")){
    /* a → smallest power of 2 ≥ a */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (a > 0){
      if (a == 1) r = 1;
      else {
        unsigned long u = (unsigned long)a;
        if ((u & (u - 1ul)) == 0ul) r = a;
        else if (a <= (1L << 62)){
          r = 1;
          while (r < a){
            if (r > (1L << 61)){ r = 0; break; }
            r <<= 1;
          }
        }
      }
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SEGCD")||kw(&L->cur,"SXGCD")||kw(&L->cur,"SEXTGCD")||
      kw(&L->cur,"STACKEGCD")||kw(&L->cur,"STACKXGCD")){
    /* a b → g x y  where a*x + b*y = g = gcd(|a|,|b|) (signed Bézout) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 1 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long old_r = a, r = b;
    long old_s = 1, ss = 0;
    long old_t = 0, tt = 1;
    while (r != 0){
      long q = old_r / r;
      long tmp = r; r = old_r - q * r; old_r = tmp;
      tmp = ss; ss = old_s - q * ss; old_s = tmp;
      tmp = tt; tt = old_t - q * tt; old_t = tmp;
    }
    long g = old_r < 0 ? -old_r : old_r;
    long x = old_r < 0 ? -old_s : old_s;
    long y = old_r < 0 ? -old_t : old_t;
    /* if a==b==0 → g=0,x=0,y=0 */
    if (a == 0 && b == 0){ g = 0; x = 0; y = 0; }
    vm->stack[vm->sp++] = g;
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",y); vm->last_n=y;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 stack roots/primes: SIROOT SISSQUARE SISCUBE SNEXTPRIME SPREVPRIME */
  if (kw(&L->cur,"SIROOT")||kw(&L->cur,"SNTHROOT")||kw(&L->cur,"STACKIROOT")){
    /* a k → IROOT(a,k) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long k = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    /* reuse expression path via local recompute */
    long r = 0;
    {
      if (k < 1) r = 0;
      else if (k == 1) r = a;
      else {
        int neg = 0;
        long n = a;
        if (n < 0){
          if ((k & 1L) == 0) r = 0;
          else { neg = 1; n = -n; }
        }
        if (n == 0 || n == 1) r = neg ? -n : n;
        else if (!(n < 0 && (k & 1L) == 0)){
          long lo = 1, hi = n, ans = 1;
          if (k == 2){ if (hi > 3037000499L) hi = 3037000499L; }
          else {
            if (hi > 1000000L && k >= 3) hi = 1000000L;
            if (k >= 4 && hi > 10000L) hi = 10000L;
            if (k >= 10 && hi > 100L) hi = 100L;
            if (k >= 40 && hi > 10L) hi = 10L;
            if (k >= 64) hi = 2;
          }
          while (lo <= hi){
            long mid = lo + (hi - lo) / 2;
            __int128 p = 1;
            int ov = 0;
            for (long i = 0; i < k; i++){
              p *= (__int128)mid;
              if (p > (__int128)n){ ov = 1; break; }
            }
            if (!ov && p <= (__int128)n){ ans = mid; lo = mid + 1; }
            else hi = mid - 1;
          }
          r = neg ? -ans : ans;
        }
      }
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNDIVS")||kw(&L->cur,"SNUMDIV")||kw(&L->cur,"STACKNDIVS")||
      kw(&L->cur,"SSIGMA")||kw(&L->cur,"SDIVSUM")||kw(&L->cur,"STACKSIGMA")||
      kw(&L->cur,"SPHI")||kw(&L->cur,"STOTIENT")||kw(&L->cur,"STACKPHI")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (strcmp(op,"SNDIVS")==0 || strcmp(op,"SNUMDIV")==0 || strcmp(op,"STACKNDIVS")==0){
      if (a > 0){
        long n = a;
        for (long i = 1; i * i <= n; i++){
          if ((n % i) == 0){ r++; if (i * i != n) r++; }
        }
      }
    } else if (strcmp(op,"SSIGMA")==0 || strcmp(op,"SDIVSUM")==0 || strcmp(op,"STACKSIGMA")==0){
      if (a > 0){
        long n = a;
        for (long i = 1; i * i <= n; i++){
          if ((n % i) == 0){ r += i; if (i * i != n) r += n / i; }
        }
      }
    } else {
      /* SPHI / STOTIENT */
      if (a > 0){
        if (a == 1) r = 1;
        else {
          long n = a; r = n;
          if ((n % 2) == 0){ while ((n % 2) == 0) n /= 2; r -= r / 2; }
          for (long p = 3; p * p <= n; p += 2){
            if ((n % p) == 0){ while ((n % p) == 0) n /= p; r -= r / p; }
          }
          if (n > 1) r -= r / n;
        }
      }
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SISSQUARE")||kw(&L->cur,"SISSQR")||kw(&L->cur,"STACKISSQUARE")||
      kw(&L->cur,"SISCUBE")||kw(&L->cur,"SISCUB")||kw(&L->cur,"STACKISCUBE")||
      kw(&L->cur,"SNEXTPRIME")||kw(&L->cur,"SNXTPRIME")||kw(&L->cur,"STACKNEXTPRIME")||
      kw(&L->cur,"SPREVPRIME")||kw(&L->cur,"SPRVPRIME")||kw(&L->cur,"STACKPREVPRIME")){
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (strcmp(op,"SISSQUARE")==0 || strcmp(op,"SISSQR")==0 || strcmp(op,"STACKISSQUARE")==0){
      if (a < 0) r = 0;
      else if (a <= 1) r = 1;
      else {
        long lo = 1, hi = a;
        if (hi > 3037000499L) hi = 3037000499L;
        while (lo <= hi){
          long mid = lo + (hi - lo) / 2;
          __int128 p = (__int128)mid * mid;
          if (p == (__int128)a){ r = 1; break; }
          if (p < (__int128)a) lo = mid + 1;
          else hi = mid - 1;
        }
      }
    } else if (strcmp(op,"SISCUBE")==0 || strcmp(op,"SISCUB")==0 || strcmp(op,"STACKISCUBE")==0){
      if (a == 0 || a == 1 || a == -1) r = 1;
      else {
        int neg = a < 0;
        long n = neg ? -a : a;
        long lo = 1, hi = n;
        if (hi > 2097151L) hi = 2097151L;
        while (lo <= hi){
          long mid = lo + (hi - lo) / 2;
          __int128 p = (__int128)mid * mid * mid;
          if (p == (__int128)n){ r = 1; break; }
          if (p < (__int128)n) lo = mid + 1;
          else hi = mid - 1;
        }
      }
    } else if (strcmp(op,"SNEXTPRIME")==0 || strcmp(op,"SNXTPRIME")==0 ||
               strcmp(op,"STACKNEXTPRIME")==0){
      long x = a + 1;
      if (x <= 2) r = 2;
      else {
        if ((x & 1L) == 0) x++;
        r = 0;
        for (long guard = 0; guard < 200000; guard++, x += 2){
          long t = x; int okp = 1;
          if (t <= 3){ r = t; break; }
          if ((t % 3) == 0) okp = 0;
          else {
            for (long i = 5; i * i <= t; i += 6){
              if ((t % i) == 0 || (t % (i + 2)) == 0){ okp = 0; break; }
            }
          }
          if (okp){ r = t; break; }
        }
      }
    } else {
      /* SPREVPRIME */
      if (a <= 2) r = 0;
      else if (a == 3) r = 2;
      else {
        long x = a - 1;
        if ((x & 1L) == 0) x--;
        r = 0;
        for (long guard = 0; guard < 200000 && x >= 2; guard++, x -= 2){
          long t = x; int okp = 1;
          if (t == 2 || t == 3){ r = t; break; }
          if ((t % 3) == 0) okp = 0;
          else {
            for (long i = 5; i * i <= t; i += 6){
              if ((t % i) == 0 || (t % (i + 2)) == 0){ okp = 0; break; }
            }
          }
          if (okp){ r = t; break; }
        }
      }
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
