/* CubalC lang — lang_ops_math_mod.c
 * MATH modular plane: SMODDIV · order · primes metrics · SADDMODN · reverse imm
 * Peel from math residual (B_math_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_math_mod.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_math_try_mod(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* digit-2 modular duals: SMODDIV · SJACOBI · SSPF (complete after SPOWMOD/SMODINV) */
  if (kw(&L->cur,"SMODDIV")||kw(&L->cur,"SDIVMODM")||kw(&L->cur,"STACKMODDIV")||
      kw(&L->cur,"SMODDIVIDE")){
    /* a b m → a * b^{-1} mod m (0 if none / m<=0) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
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
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SJACOBI")||kw(&L->cur,"SLEGENDRE")||kw(&L->cur,"STACKJACOBI")||
      kw(&L->cur,"STACKLEGENDRE")){
    /* a n → Jacobi(a|n) ∈ {-1,0,1}; n must be odd positive else 0 */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (n > 0 && (n & 1L) != 0){
      long aa = a % n; if (aa < 0) aa += n;
      int res = 1;
      while (aa != 0){
        while ((aa & 1L) == 0){
          aa >>= 1;
          long n8 = n & 7L;
          if (n8 == 3 || n8 == 5) res = -res;
        }
        long tmp = aa; aa = n; n = tmp;
        if ((aa & 3L) == 3 && (n & 3L) == 3) res = -res;
        aa %= n;
      }
      r = (n == 1) ? (long)res : 0;
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSPF")||kw(&L->cur,"SSMALLPF")||kw(&L->cur,"SMINPF")||
      kw(&L->cur,"STACKSPF")||kw(&L->cur,"STACKSMALLPF")){
    /* n → smallest prime factor; n<=1 → 0 */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long n = a < 0 ? -a : a;
    long r = 0;
    if (n > 1){
      if ((n & 1L) == 0) r = 2;
      else if ((n % 3L) == 0) r = 3;
      else {
        r = n;
        for (long i = 5; i * i <= n; i += 6){
          if ((n % i) == 0){ r = i; break; }
          if ((n % (i + 2)) == 0){ r = i + 2; break; }
        }
      }
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 factor metrics stack: SVAL · SOMEGA · SOMEGA0 */
  if (kw(&L->cur,"SVAL")||kw(&L->cur,"SPVAL")||kw(&L->cur,"SVALUATION")||
      kw(&L->cur,"STACKVAL")||kw(&L->cur,"SVP")){
    /* n p → v_p(n)  largest k with p^k | n */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long p = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long n = a < 0 ? -a : a;
    long pp = p < 0 ? -p : p;
    long r = 0;
    if (n != 0 && pp > 1){
      while (n % pp == 0){ n /= pp; r++; if (n == 0) break; }
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SOMEGA")||kw(&L->cur,"SBIGOMEGA")||kw(&L->cur,"STACKOMEGA")||
      kw(&L->cur,"SOMGA")){
    /* n → Ω(n) total prime factors with multiplicity */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long n = a < 0 ? -a : a;
    long r = 0;
    if (n > 1){
      while ((n & 1L) == 0){ n >>= 1; r++; }
      for (long p = 3; p * p <= n; p += 2){
        while ((n % p) == 0){ n /= p; r++; }
      }
      if (n > 1) r++;
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SOMEGA0")||kw(&L->cur,"SLITTLEOMEGA")||kw(&L->cur,"SNUOMEGA")||
      kw(&L->cur,"STACKOMEGA0")||kw(&L->cur,"SOMGA0")){
    /* n → ω(n) distinct prime factors */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long n = a < 0 ? -a : a;
    long r = 0;
    if (n > 1){
      if ((n & 1L) == 0){
        r++;
        while ((n & 1L) == 0) n >>= 1;
      }
      for (long p = 3; p * p <= n; p += 2){
        if ((n % p) == 0){
          r++;
          while ((n % p) == 0) n /= p;
        }
      }
      if (n > 1) r++;
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 modular order stack: SSOPF · SCARMICHAEL · SORDER */
  if (kw(&L->cur,"SSOPF")||kw(&L->cur,"STACKSOPF")||kw(&L->cur,"SSUMOPF")){
    /* n → sum of distinct prime factors */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long n = a < 0 ? -a : a;
    long s = 0;
    if (n > 1){
      if ((n & 1L) == 0){
        s += 2;
        while ((n & 1L) == 0) n >>= 1;
      }
      for (long p = 3; p * p <= n; p += 2){
        if ((n % p) == 0){
          s += p;
          while ((n % p) == 0) n /= p;
        }
      }
      if (n > 1) s += n;
    }
    vm->stack[vm->sp - 1] = s;
    var_set_num(vm,"LAST_N",s); vm->last_n=s;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCARMICHAEL")||kw(&L->cur,"SLAMBDA")||kw(&L->cur,"SCARM")||
      kw(&L->cur,"STACKCARMICHAEL")||kw(&L->cur,"STACKLAMBDA")){
    /* n → Carmichael λ(n) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long n = a < 0 ? -a : a;
    long r = 0;
    if (n == 1) r = 1;
    else if (n > 1){
      r = 1;
      long e2 = 0;
      while ((n & 1L) == 0){ n >>= 1; e2++; }
      if (e2){
        if (e2 == 1) r = 1;
        else if (e2 == 2) r = 2;
        else r = 1L << (e2 - 2);
      }
      for (long p = 3; p * p <= n; p += 2){
        if ((n % p) == 0){
          long pk = 1;
          while ((n % p) == 0){ n /= p; pk *= p; }
          long lam = (pk / p) * (p - 1);
          long x = r, y = lam;
          while (y){ long t = x % y; x = y; y = t; }
          r = (x == 0) ? 0 : (r / x) * lam;
        }
      }
      if (n > 1){
        long lam = n - 1;
        long x = r, y = lam;
        while (y){ long t = x % y; x = y; y = t; }
        r = (x == 0) ? 0 : (r / x) * lam;
      }
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORDER")||kw(&L->cur,"SMULTORDER")||kw(&L->cur,"STACKORDER")||
      kw(&L->cur,"SORD")){
    /* a m → multiplicative order of a mod m (0 if none) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (m > 1){
      long aa = a % m; if (aa < 0) aa += m;
      if (aa != 0){
        long gx = aa, gy = m;
        while (gy){ long t = gx % gy; gx = gy; gy = t; }
        if (gx == 1){
          long mm = m, res = 1, e2 = 0;
          while ((mm & 1L) == 0){ mm >>= 1; e2++; }
          if (e2){
            if (e2 == 1) res = 1;
            else if (e2 == 2) res = 2;
            else res = 1L << (e2 - 2);
          }
          for (long p = 3; p * p <= mm; p += 2){
            if ((mm % p) == 0){
              long pk = 1;
              while ((mm % p) == 0){ mm /= p; pk *= p; }
              long lam = (pk / p) * (p - 1);
              long x = res, y = lam;
              while (y){ long t = x % y; x = y; y = t; }
              res = (x == 0) ? 0 : (res / x) * lam;
            }
          }
          if (mm > 1){
            long lam = mm - 1;
            long x = res, y = lam;
            while (y){ long t = x % y; x = y; y = t; }
            res = (x == 0) ? 0 : (res / x) * lam;
          }
          long lam = res;
          for (long k = 1; k <= lam; k++){
            long base = aa, exp = k, rr = 1 % m;
            while (exp > 0){
              if (exp & 1){
                long y2 = rr, x2 = base, acc = 0;
                while (y2 > 0){
                  if (y2 & 1) acc = (acc + x2) % m;
                  x2 = (x2 + x2) % m;
                  y2 >>= 1;
                }
                rr = acc;
              }
              {
                long x2 = base, acc = 0, y2 = base;
                while (y2 > 0){
                  if (y2 & 1) acc = (acc + x2) % m;
                  x2 = (x2 + x2) % m;
                  y2 >>= 1;
                }
                base = acc;
              }
              exp >>= 1;
            }
            if (rr == 1){ r = k; break; }
          }
        }
      }
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 modular ext stack: SSQPART · SIPRIMITIVE · SCRT */
  if (kw(&L->cur,"SSQPART")||kw(&L->cur,"SLARGESQ")||kw(&L->cur,"SMAXSQ")||
      kw(&L->cur,"STACKSQPART")){
    /* n → largest square dividing n */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long n = a < 0 ? -a : a;
    long r = 0;
    if (n == 1) r = 1;
    else if (n > 1){
      r = 1;
      long e = 0;
      while ((n & 1L) == 0){ n >>= 1; e++; }
      e = (e / 2) * 2;
      for (long i = 0; i < e; i++) r *= 2;
      for (long p = 3; p * p <= n; p += 2){
        e = 0;
        while ((n % p) == 0){ n /= p; e++; }
        e = (e / 2) * 2;
        for (long i = 0; i < e; i++) r *= p;
      }
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SIPRIMITIVE")||kw(&L->cur,"SIPRROOT")||kw(&L->cur,"SPRIMROOTP")||
      kw(&L->cur,"STACKISPRIMITIVE")||kw(&L->cur,"SIPROOT")){
    /* a m → 1 if a is primitive root mod m */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (m > 1){
      long aa = a % m; if (aa < 0) aa += m;
      if (aa != 0){
        long gx = aa, gy = m;
        while (gy){ long t = gx % gy; gx = gy; gy = t; }
        if (gx == 1){
          long mm = m, phi = m;
          if ((mm & 1L) == 0){
            phi -= phi / 2;
            while ((mm & 1L) == 0) mm >>= 1;
          }
          for (long p = 3; p * p <= mm; p += 2){
            if ((mm % p) == 0){
              phi -= phi / p;
              while ((mm % p) == 0) mm /= p;
            }
          }
          if (mm > 1) phi -= phi / mm;
          for (long k = 1; k <= phi; k++){
            long base = aa, exp = k, rr = 1 % m;
            while (exp > 0){
              if (exp & 1){
                long y2 = rr, x2 = base, acc = 0;
                while (y2 > 0){
                  if (y2 & 1) acc = (acc + x2) % m;
                  x2 = (x2 + x2) % m;
                  y2 >>= 1;
                }
                rr = acc;
              }
              {
                long x2 = base, acc = 0, y2 = base;
                while (y2 > 0){
                  if (y2 & 1) acc = (acc + x2) % m;
                  x2 = (x2 + x2) % m;
                  y2 >>= 1;
                }
                base = acc;
              }
              exp >>= 1;
            }
            if (rr == 1){ r = (k == phi) ? 1 : 0; break; }
          }
        }
      }
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCRT")||kw(&L->cur,"SCHINREM")||kw(&L->cur,"STACKCRT")||
      kw(&L->cur,"SCRT2")){
    /* a m b n → x with x≡a (mod m), x≡b (mod n); 0 if inconsistent / bad mods */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long m = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (m > 0 && n > 0){
      long aa = a % m; if (aa < 0) aa += m;
      long bb = b % n; if (bb < 0) bb += n;
      long old_r = m, rr = n;
      long old_s = 1, ss = 0;
      while (rr != 0){
        long q = old_r / rr;
        long tmp = rr; rr = old_r - q * rr; old_r = tmp;
        tmp = ss; ss = old_s - q * ss; old_s = tmp;
      }
      long g = old_r < 0 ? -old_r : old_r;
      long s = old_r < 0 ? -old_s : old_s;
      long diff = bb - aa;
      if (g != 0 && (diff % g) == 0){
        long mod = (m / g) * n;
        long ng = n / g;
        long k = (diff / g) * s;
        k %= ng; if (k < 0) k += ng;
        r = aa + m * k;
        r %= mod; if (r < 0) r += mod;
      }
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 primes/powers stack: SISPOWER · SIPP · SNTHPRIME */
  if (kw(&L->cur,"SISPOWER")||kw(&L->cur,"SPERFPOW")||kw(&L->cur,"SISPOW")||
      kw(&L->cur,"STACKISPOWER")||kw(&L->cur,"SISPOWERP")){
    /* n → 1 if perfect power b^e, e>=2 */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[vm->sp - 1];
    long r = 0;
    if (n > 1){
      for (long e = 2; e <= 62 && !r; e++){
        long lo = 1, hi = n, ans = 1;
        if (e == 2){ if (hi > 3037000499L) hi = 3037000499L; }
        else {
          if (e >= 3 && hi > 1000000L) hi = 1000000L;
          if (e >= 10 && hi > 10000L) hi = 10000L;
          if (e >= 20 && hi > 100L) hi = 100L;
          if (e >= 40 && hi > 4L) hi = 4L;
        }
        while (lo <= hi){
          long mid = lo + (hi - lo) / 2;
          long p = 1; int ov = 0;
          for (long i = 0; i < e; i++){
            if (mid != 0 && p > n / mid){ ov = 1; break; }
            p *= mid;
          }
          if (ov || p > n) hi = mid - 1;
          else { ans = mid; lo = mid + 1; }
        }
        long p = 1; int ov = 0;
        for (long i = 0; i < e; i++){
          if (ans != 0 && p > n / ans){ ov = 1; break; }
          p *= ans;
        }
        if (!ov && p == n && ans > 1) r = 1;
      }
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SIPP")||kw(&L->cur,"SISPRIMEPOWER")||kw(&L->cur,"SPRIMEPOWERP")||
      kw(&L->cur,"STACKIPP")||kw(&L->cur,"SIPOWP")){
    /* n → 1 if prime power p^k */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long n = a < 0 ? -a : a;
    long r = 0;
    if (n > 1){
      if ((n & 1L) == 0){
        while ((n & 1L) == 0) n >>= 1;
        r = (n == 1) ? 1 : 0;
      } else {
        r = 1;
        for (long p = 3; p * p <= n; p += 2){
          if ((n % p) == 0){
            while ((n % p) == 0) n /= p;
            r = (n == 1) ? 1 : 0;
            break;
          }
        }
      }
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNTHPRIME")||kw(&L->cur,"SPRIMEN")||kw(&L->cur,"SPRIMEK")||
      kw(&L->cur,"STACKNTHPRIME")||kw(&L->cur,"SNPRIME")){
    /* k → k-th prime (1→2); k<=0 → 0; cap 10000 */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long k = vm->stack[vm->sp - 1];
    long r = 0;
    if (k > 0){
      if (k > 10000) k = 10000;
      if (k == 1) r = 2;
      else {
        long found = 1;
        for (long n = 3; ; n += 2){
          int prime = 1;
          if ((n % 3) == 0){ if (n != 3) prime = 0; }
          else {
            for (long i = 5; i * i <= n; i += 6){
              if ((n % i) == 0 || (n % (i + 2)) == 0){ prime = 0; break; }
            }
          }
          if (prime){
            found++;
            if (found == k){ r = n; break; }
          }
        }
      }
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 prime metrics stack: SPRIMECOUNT · SPRIMEGAP · SISCOMPOSITE */
  if (kw(&L->cur,"SPRIMECOUNT")||kw(&L->cur,"SPRIMEPI")||kw(&L->cur,"SPIN")||
      kw(&L->cur,"STACKPRIMECOUNT")||kw(&L->cur,"SCOUNTPRIMES")||
      kw(&L->cur,"SPRIMEGAP")||kw(&L->cur,"SPGAP")||kw(&L->cur,"STACKPRIMEGAP")||
      kw(&L->cur,"SNEXTGAP")||
      kw(&L->cur,"SISCOMPOSITE")||kw(&L->cur,"SCOMPOSITEP")||kw(&L->cur,"SCOMPP")||
      kw(&L->cur,"STACKISCOMPOSITE")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (strcmp(op,"SPRIMECOUNT")==0 || strcmp(op,"SPRIMEPI")==0 ||
        strcmp(op,"SPIN")==0 || strcmp(op,"STACKPRIMECOUNT")==0 ||
        strcmp(op,"SCOUNTPRIMES")==0){
      long n = a;
      if (n >= 2){
        if (n > 200000L) n = 200000L;
        for (long x = 2; x <= n; x++){
          int okp = 1;
          if (x <= 3){ r++; continue; }
          if ((x % 2) == 0 || (x % 3) == 0){ okp = 0; }
          else {
            for (long i = 5; i * i <= x; i += 6){
              if ((x % i) == 0 || (x % (i + 2)) == 0){ okp = 0; break; }
            }
          }
          if (okp) r++;
        }
      }
    } else if (strcmp(op,"SPRIMEGAP")==0 || strcmp(op,"SPGAP")==0 ||
               strcmp(op,"STACKPRIMEGAP")==0 || strcmp(op,"SNEXTGAP")==0){
      long x = a + 1;
      if (x <= 2) r = 2 - a;
      else {
        if ((x & 1L) == 0) x++;
        for (long guard = 0; guard < 200000; guard++, x += 2){
          long t = x;
          int okp = 1;
          if (t <= 3){ r = t - a; break; }
          if ((t % 3) == 0){ okp = 0; }
          else {
            for (long i = 5; i * i <= t; i += 6){
              if ((t % i) == 0 || (t % (i + 2)) == 0){ okp = 0; break; }
            }
          }
          if (okp){ r = t - a; break; }
        }
      }
    } else {
      /* SISCOMPOSITE */
      long n = a < 0 ? -a : a;
      if (n > 3){
        if ((n % 2) == 0 || (n % 3) == 0) r = 1;
        else {
          r = 0;
          for (long i = 5; i * i <= n; i += 6){
            if ((n % i) == 0 || (n % (i + 2)) == 0){ r = 1; break; }
          }
        }
      }
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 abundance stack: SALIQUOT · SISPERFECT · SISABUNDANT · SISDEFICIENT */
  if (kw(&L->cur,"SALIQUOT")||kw(&L->cur,"SPROPERSIGMA")||kw(&L->cur,"STACKALIQUOT")||
      kw(&L->cur,"SS0")||
      kw(&L->cur,"SISPERFECT")||kw(&L->cur,"SPERFECTP")||kw(&L->cur,"STACKISPERFECT")||
      kw(&L->cur,"SISABUNDANT")||kw(&L->cur,"SABUNDANTP")||kw(&L->cur,"STACKISABUNDANT")||
      kw(&L->cur,"SISDEFICIENT")||kw(&L->cur,"SDEFICIENTP")||kw(&L->cur,"STACKISDEFICIENT")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (a > 0){
      long n = a, sum = 0;
      for (long i = 1; i * i <= n; i++){
        if ((n % i) == 0){
          sum += i;
          if (i * i != n) sum += n / i;
        }
      }
      long s = sum - n;
      if (strcmp(op,"SALIQUOT")==0 || strcmp(op,"SPROPERSIGMA")==0 ||
          strcmp(op,"STACKALIQUOT")==0 || strcmp(op,"SS0")==0)
        r = s;
      else if (strcmp(op,"SISPERFECT")==0 || strcmp(op,"SPERFECTP")==0 ||
               strcmp(op,"STACKISPERFECT")==0)
        r = (s == n) ? 1 : 0;
      else if (strcmp(op,"SISABUNDANT")==0 || strcmp(op,"SABUNDANTP")==0 ||
               strcmp(op,"STACKISABUNDANT")==0)
        r = (s > n) ? 1 : 0;
      else
        r = (s < n) ? 1 : 0; /* SISDEFICIENT */
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 stack combinatorics + add/sub mod: SBINOM SPERM SADDMOD SSUBMOD */
  if (kw(&L->cur,"SBINOM")||kw(&L->cur,"SCHOOSE")||kw(&L->cur,"STACKBINOM")||
      kw(&L->cur,"SPERM")||kw(&L->cur,"SPNR")||kw(&L->cur,"STACKPERM")){
    /* n k → C(n,k) or P(n,k) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long k = vm->stack[--vm->sp];
    long n = vm->stack[--vm->sp];
    long r = 0;
    if (n < 0 || k < 0 || k > n) r = 0;
    else if (strcmp(op,"SPERM")==0 || strcmp(op,"SPNR")==0 || strcmp(op,"STACKPERM")==0){
      r = 1;
      for (long i = 0; i < k; i++) r *= (n - i);
    } else {
      /* SBINOM / SCHOOSE / STACKBINOM */
      long kk = k;
      if (kk > n - kk) kk = n - kk;
      r = 1;
      for (long i = 1; i <= kk; i++)
        r = r * (n - kk + i) / i;
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SADDMOD")||kw(&L->cur,"STACKADDMOD")||
      kw(&L->cur,"SSUBMOD")||kw(&L->cur,"STACKSUBMOD")){
    /* a b m → (a±b) mod m */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (m <= 0) r = 0;
    else {
      long x = a % m; if (x < 0) x += m;
      long y = b % m; if (y < 0) y += m;
      if (strcmp(op,"SADDMOD")==0 || strcmp(op,"STACKADDMOD")==0)
        r = (x + y) % m;
      else
        r = (x - y + m) % m;
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 stack immediate modular ALU: SADDMODN · SSUBMODN · SMULMODN (dual of DADDMODN plane) */
  if (kw(&L->cur,"SADDMODN")||kw(&L->cur,"STACKADDMODN")||kw(&L->cur,"ADDMODN")||
      kw(&L->cur,"SADDMODIMM")||
      kw(&L->cur,"SSUBMODN")||kw(&L->cur,"STACKSUBMODN")||kw(&L->cur,"SUBMODN")||
      kw(&L->cur,"SSUBMODIMM")||
      kw(&L->cur,"SMULMODN")||kw(&L->cur,"STACKMULMODN")||kw(&L->cur,"MULMODN")||
      kw(&L->cur,"SMULMODIMM")){
    /* TOS + k m → f(TOS); m<=0 → 0; result in [0,m)
     * ADD: (x+k)%m  SUB: (x-k)%m  MUL: (x*k)%m */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_add = (strcmp(op,"SADDMODN")==0 || strcmp(op,"STACKADDMODN")==0 ||
                  strcmp(op,"ADDMODN")==0 || strcmp(op,"SADDMODIMM")==0);
    int is_sub = (strcmp(op,"SSUBMODN")==0 || strcmp(op,"STACKSUBMODN")==0 ||
                  strcmp(op,"SUBMODN")==0 || strcmp(op,"SSUBMODIMM")==0);
    /* else SMULMODN */
    lex_next(L);
    long k = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long x = 0;
    if (m > 0){
      long kk = k % m; if (kk < 0) kk += m;
      long aa = a % m; if (aa < 0) aa += m;
      if (is_add){
        x = (aa + kk) % m;
      } else if (is_sub){
        x = (aa - kk + m) % m;
      } else {
        long acc = 0, xx = aa, kk2 = kk;
        while (kk2 > 0){
          if (kk2 & 1) acc = (acc + xx) % m;
          xx = (xx + xx) % m;
          kk2 >>= 1;
        }
        x = acc;
      }
    }
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 stack immediate modular inv/pow/div: SMODINVN · SPOWMODN · SMODDIVN (complete SADDMODN plane) */
  if (kw(&L->cur,"SMODINVN")||kw(&L->cur,"STACKMODINVN")||kw(&L->cur,"SINVMODN")||
      kw(&L->cur,"MODINVN")||kw(&L->cur,"INVMODN")||kw(&L->cur,"SMODINVIMM")){
    /* SMODINVN m — TOS = TOS^{-1} mod m; 0 if none / m<=1 */
    lex_next(L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
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
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPOWMODN")||kw(&L->cur,"STACKPOWMODN")||kw(&L->cur,"POWMODN")||
      kw(&L->cur,"SPOWMODIMM")||kw(&L->cur,"SEXPMMODN")){
    /* SPOWMODN exp m — TOS = TOS^exp mod m; m<=0 → 0; exp<0 → 0 */
    lex_next(L);
    long exp = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
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
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMODDIVN")||kw(&L->cur,"STACKMODDIVN")||kw(&L->cur,"SDIVMODMN")||
      kw(&L->cur,"MODDIVN")||kw(&L->cur,"SMODDIVIMM")){
    /* SMODDIVN b m — TOS = TOS * b^{-1} mod m; 0 if none / m<=0 */
    lex_next(L);
    long b = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
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
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 reverse imm modular plane: SSUBMODFROMN · SPOWMODFROMN · SMODDIVFROMN
   * (n op_mod TOS; reverse dual of SSUBMODN/SPOWMODN/SMODDIVN after reverse ALU FROMN) */
  if (kw(&L->cur,"SSUBMODFROMN")||kw(&L->cur,"STACKSUBMODFROMN")||kw(&L->cur,"SUBMODFROMN")||
      kw(&L->cur,"SRSUBMODN")||kw(&L->cur,"RSUBMODN")||kw(&L->cur,"SSUBMODFROMIMM")){
    /* SSUBMODFROMN k m — TOS = (k - TOS) mod m; m<=0 → 0 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long x = 0;
    if (m > 0){
      long kk = k % m; if (kk < 0) kk += m;
      long aa = a % m; if (aa < 0) aa += m;
      x = (kk - aa + m) % m;
    }
    vm->stack[vm->sp - 1] = x;
    var_set_num(vm,"LAST_N",x); vm->last_n=x;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPOWMODFROMN")||kw(&L->cur,"STACKPOWMODFROMN")||kw(&L->cur,"POWMODFROMN")||
      kw(&L->cur,"SRPOWMODN")||kw(&L->cur,"RPOWMODN")||kw(&L->cur,"SPOWMODFROMIMM")||
      kw(&L->cur,"SBASEPOWMODN")){
    /* SPOWMODFROMN base m — TOS = base^TOS mod m; m<=0 or exp<0 → 0 */
    lex_next(L);
    long base_in = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long exp = vm->stack[vm->sp - 1];
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
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMODDIVFROMN")||kw(&L->cur,"STACKMODDIVFROMN")||kw(&L->cur,"MODDIVFROMN")||
      kw(&L->cur,"SRMODDIVN")||kw(&L->cur,"RMODDIVN")||kw(&L->cur,"SMODDIVFROMIMM")||
      kw(&L->cur,"SDIVMODFROMN")){
    /* SMODDIVFROMN a m — TOS = a * TOS^{-1} mod m; 0 if none / m<=0 */
    lex_next(L);
    long a = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[vm->sp - 1];
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
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
