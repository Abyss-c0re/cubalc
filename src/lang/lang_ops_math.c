/* CubalC lang — lang_ops_math.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_math(VM *vm, Lex *L){
  /* plane ops_math: L20289-22808 */
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
  /* digit-3/0 stack pack+byte: SPACK16 SHI16 SLO16 SBYTE SLOBYTE SHIBYTE */
  if (kw(&L->cur,"SLOBYTE")||kw(&L->cur,"STACKLOBYTE")||
      kw(&L->cur,"SHIBYTE")||kw(&L->cur,"STACKHIBYTE")||
      kw(&L->cur,"SHI16")||kw(&L->cur,"SHIWORD")||kw(&L->cur,"STACKHI16")||
      kw(&L->cur,"SLO16")||kw(&L->cur,"SLOWORD")||kw(&L->cur,"STACKLO16")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (strcmp(op,"SLOBYTE")==0 || strcmp(op,"STACKLOBYTE")==0)
      r = (long)((unsigned long)a & 0xFFul);
    else if (strcmp(op,"SHIBYTE")==0 || strcmp(op,"STACKHIBYTE")==0)
      r = (long)(((unsigned long)a >> 8) & 0xFFul);
    else if (strcmp(op,"SHI16")==0 || strcmp(op,"SHIWORD")==0 || strcmp(op,"STACKHI16")==0)
      r = (long)(((unsigned int)a >> 16) & 0xFFFFu);
    else
      r = (long)((unsigned int)a & 0xFFFFu);
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SBYTE")||kw(&L->cur,"STACKBYTE")||kw(&L->cur,"SGETBYTE")){
    /* a i → i-th little-endian byte of a (i clamped 0..7) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i > 7) i = 7;
    long r = (long)(((unsigned long)a >> (unsigned)(i * 8)) & 0xFFul);
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 SETBYTE + ALIGN: SSETBYTE SALIGN SALIGNDN */
  if (kw(&L->cur,"SSETBYTE")||kw(&L->cur,"STACKSETBYTE")||kw(&L->cur,"SSETBY")){
    /* a field i → deposit 8-bit at byte index i (SSETB reserved for bit set) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    long field = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i > 7) i = 7;
    unsigned long base = (unsigned long)a;
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long shift = (unsigned long)(i * 8);
    long r = (long)((base & ~(0xFFul << shift)) | (f << shift));
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack imm byte field: SBYTEN · SSETBYTEN · SCLRBYTEN (imm dual of SBYTE/SSETBYTE) */
  if (kw(&L->cur,"SBYTEN")||kw(&L->cur,"STACKBYTEN")||kw(&L->cur,"SGETBYTEN")||
      kw(&L->cur,"BYTEN")||kw(&L->cur,"GETBYTEN")){
    /* SBYTEN n — TOS = little-endian byte n of TOS; n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    long r = (long)(((unsigned long)vm->stack[vm->sp - 1] >> (unsigned)(n * 8)) & 0xFFul);
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSETBYTEN")||kw(&L->cur,"STACKSETBYTEN")||kw(&L->cur,"SSETBYIMM")||
      kw(&L->cur,"SETBYTEN")||kw(&L->cur,"PUTBYTEN")){
    /* SSETBYTEN field n — deposit low 8 bits of field into byte n of TOS; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long shift = (unsigned long)(n * 8);
    long r = (long)((base & ~(0xFFul << shift)) | (f << shift));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLRBYTEN")||kw(&L->cur,"STACKCLRBYTEN")||kw(&L->cur,"SCLRBYIMM")||
      kw(&L->cur,"CLRBYTEN")||kw(&L->cur,"ZAPBYTEN")){
    /* SCLRBYTEN n — clear little-endian byte n of TOS; n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long shift = (unsigned long)(n * 8);
    long r = (long)(base & ~(0xFFul << shift));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack imm 16-bit halfword field: SWORDN · SSET16N · SCLR16N (complete nibble/byte ladder) */
  if (kw(&L->cur,"SWORDN")||kw(&L->cur,"STACKWORDN")||kw(&L->cur,"SGET16N")||
      kw(&L->cur,"SHALFN")||kw(&L->cur,"GET16N")||kw(&L->cur,"WORDN")||
      kw(&L->cur,"SGETHALF")){
    /* SWORDN n — TOS = little-endian 16-bit halfword n; n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    long r = (long)(((unsigned long)vm->stack[vm->sp - 1] >> (unsigned)(n * 16)) & 0xFFFFul);
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSET16N")||kw(&L->cur,"STACKSET16N")||kw(&L->cur,"SSETWORDN")||
      kw(&L->cur,"SET16N")||kw(&L->cur,"PUT16N")||kw(&L->cur,"SSETHALFN")||
      kw(&L->cur,"SSETWIMM")){
    /* SSET16N field n — deposit low 16 bits of field into halfword n of TOS; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long shift = (unsigned long)(n * 16);
    long r = (long)((base & ~(0xFFFFul << shift)) | (f << shift));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLR16N")||kw(&L->cur,"STACKCLR16N")||kw(&L->cur,"SCLRWORDN")||
      kw(&L->cur,"CLR16N")||kw(&L->cur,"ZAP16N")||kw(&L->cur,"SCLRHAFN")||
      kw(&L->cur,"SCLRWIMM")){
    /* SCLR16N n — clear little-endian halfword n of TOS; n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long shift = (unsigned long)(n * 16);
    long r = (long)(base & ~(0xFFFFul << shift));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack imm 32-bit field: SGET32N · SSET32N · SCLR32N (complete nibble/byte/half/word ladder) */
  if (kw(&L->cur,"SGET32N")||kw(&L->cur,"STACKGET32N")||kw(&L->cur,"SWORD32N")||
      kw(&L->cur,"GET32N")||kw(&L->cur,"SLOW32N")||kw(&L->cur,"SDWORDN")||
      kw(&L->cur,"SGETDWN")||kw(&L->cur,"WORD32N")){
    /* SGET32N n — TOS = little-endian 32-bit word n; n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    long r = (long)(((unsigned long)vm->stack[vm->sp - 1] >> (unsigned)(n * 32)) & 0xFFFFFFFFul);
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSET32N")||kw(&L->cur,"STACKSET32N")||kw(&L->cur,"SSETWORD32N")||
      kw(&L->cur,"SET32N")||kw(&L->cur,"PUT32N")||kw(&L->cur,"SSETDWN")||
      kw(&L->cur,"SSETW32IMM")){
    /* SSET32N field n — deposit low 32 bits of field into word n of TOS; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long shift = (unsigned long)(n * 32);
    long r = (long)((base & ~(0xFFFFFFFFul << shift)) | (f << shift));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLR32N")||kw(&L->cur,"STACKCLR32N")||kw(&L->cur,"SCLRWORD32N")||
      kw(&L->cur,"CLR32N")||kw(&L->cur,"ZAP32N")||kw(&L->cur,"SCLRDWN")||
      kw(&L->cur,"SCLRW32IMM")){
    /* SCLR32N n — clear little-endian 32-bit word n of TOS; n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long shift = (unsigned long)(n * 32);
    long r = (long)(base & ~(0xFFFFFFFFul << shift));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack imm 32-bit field bitwise: SAND32N · SOR32N · SXOR32N (merge into word n; after SSET32N) */
  if (kw(&L->cur,"SAND32N")||kw(&L->cur,"STACKAND32N")||kw(&L->cur,"AND32N")||
      kw(&L->cur,"SANDWN")||kw(&L->cur,"SANDWIMM")||kw(&L->cur,"SKEEP32N")){
    /* SAND32N field n — word n of TOS &= field; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = ((base >> sh) & 0xFFFFFFFFul) & f;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SOR32N")||kw(&L->cur,"STACKOR32N")||kw(&L->cur,"OR32N")||
      kw(&L->cur,"SORWN")||kw(&L->cur,"SORWIMM")||kw(&L->cur,"SSETOR32N")){
    /* SOR32N field n — word n of TOS |= field; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = ((base >> sh) & 0xFFFFFFFFul) | f;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXOR32N")||kw(&L->cur,"STACKXOR32N")||kw(&L->cur,"XOR32N")||
      kw(&L->cur,"SXORWN")||kw(&L->cur,"SXORWIMM")||kw(&L->cur,"SFLIP32N")){
    /* SXOR32N field n — word n of TOS ^= field; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = ((base >> sh) & 0xFFFFFFFFul) ^ f;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack imm inverted 32-bit field: SNAND32N · SNOR32N · SXNOR32N (complete after SAND32N; ladder after SNAND16N) */
  if (kw(&L->cur,"SNAND32N")||kw(&L->cur,"STACKNAND32N")||kw(&L->cur,"NAND32N")||
      kw(&L->cur,"SNANDWN")||kw(&L->cur,"SNANDWIMM")||kw(&L->cur,"SINVERTAND32N")){
    /* SNAND32N field n — word n of TOS = ~(word & field) & 0xFFFFFFFF; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = (~(((base >> sh) & 0xFFFFFFFFul) & f)) & 0xFFFFFFFFul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNOR32N")||kw(&L->cur,"STACKNOR32N")||kw(&L->cur,"NOR32N")||
      kw(&L->cur,"SNORWN")||kw(&L->cur,"SNORWIMM")||kw(&L->cur,"SINVERTOR32N")){
    /* SNOR32N field n — word n of TOS = ~(word | field) & 0xFFFFFFFF; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = (~(((base >> sh) & 0xFFFFFFFFul) | f)) & 0xFFFFFFFFul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNOR32N")||kw(&L->cur,"STACKXNOR32N")||kw(&L->cur,"XNOR32N")||
      kw(&L->cur,"SXNORWN")||kw(&L->cur,"SXNORWIMM")||kw(&L->cur,"SEQUIV32N")){
    /* SXNOR32N field n — word n of TOS = ~(word ^ field) & 0xFFFFFFFF; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = (~(((base >> sh) & 0xFFFFFFFFul) ^ f)) & 0xFFFFFFFFul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack imm 16-bit field bitwise: SAND16N · SOR16N · SXOR16N (merge into halfword n; after SSET16N/SAND32N) */
  if (kw(&L->cur,"SAND16N")||kw(&L->cur,"STACKAND16N")||kw(&L->cur,"AND16N")||
      kw(&L->cur,"SANDHN16")||kw(&L->cur,"SANDHIMM")||kw(&L->cur,"SKEEPH16N")){
    /* SAND16N field n — halfword n of TOS &= field; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = ((base >> sh) & 0xFFFFul) & f;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SOR16N")||kw(&L->cur,"STACKOR16N")||kw(&L->cur,"OR16N")||
      kw(&L->cur,"SORHN16")||kw(&L->cur,"SORHIMM")||kw(&L->cur,"SSETOR16N")){
    /* SOR16N field n — halfword n of TOS |= field; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = ((base >> sh) & 0xFFFFul) | f;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXOR16N")||kw(&L->cur,"STACKXOR16N")||kw(&L->cur,"XOR16N")||
      kw(&L->cur,"SXORHN16")||kw(&L->cur,"SXORHIMM")||kw(&L->cur,"SFLIP16N")){
    /* SXOR16N field n — halfword n of TOS ^= field; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = ((base >> sh) & 0xFFFFul) ^ f;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack imm inverted 16-bit field: SNAND16N · SNOR16N · SXNOR16N (complete after SAND16N; ladder after SNAND8N) */
  if (kw(&L->cur,"SNAND16N")||kw(&L->cur,"STACKNAND16N")||kw(&L->cur,"NAND16N")||
      kw(&L->cur,"SNANDHN16")||kw(&L->cur,"SNANDHIMM")||kw(&L->cur,"SINVERTAND16N")){
    /* SNAND16N field n — halfword n of TOS = ~(half & field) & 0xFFFF; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = (~(((base >> sh) & 0xFFFFul) & f)) & 0xFFFFul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNOR16N")||kw(&L->cur,"STACKNOR16N")||kw(&L->cur,"NOR16N")||
      kw(&L->cur,"SNORHN16")||kw(&L->cur,"SNORHIMM")||kw(&L->cur,"SINVERTOR16N")){
    /* SNOR16N field n — halfword n of TOS = ~(half | field) & 0xFFFF; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = (~(((base >> sh) & 0xFFFFul) | f)) & 0xFFFFul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNOR16N")||kw(&L->cur,"STACKXNOR16N")||kw(&L->cur,"XNOR16N")||
      kw(&L->cur,"SXNORHN16")||kw(&L->cur,"SXNORHIMM")||kw(&L->cur,"SEQUIV16N")){
    /* SXNOR16N field n — halfword n of TOS = ~(half ^ field) & 0xFFFF; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = (~(((base >> sh) & 0xFFFFul) ^ f)) & 0xFFFFul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack imm 8-bit field bitwise: SAND8N · SOR8N · SXOR8N (merge into byte n; complete after SAND16N/SAND32N) */
  if (kw(&L->cur,"SAND8N")||kw(&L->cur,"STACKAND8N")||kw(&L->cur,"AND8N")||
      kw(&L->cur,"SANDBN")||kw(&L->cur,"SANDBIMM")||kw(&L->cur,"SKEEP8N")){
    /* SAND8N field n — byte n of TOS &= field; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = ((base >> sh) & 0xFFul) & f;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SOR8N")||kw(&L->cur,"STACKOR8N")||kw(&L->cur,"OR8N")||
      kw(&L->cur,"SORBN")||kw(&L->cur,"SORBIMM")||kw(&L->cur,"SSETOR8N")){
    /* SOR8N field n — byte n of TOS |= field; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = ((base >> sh) & 0xFFul) | f;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXOR8N")||kw(&L->cur,"STACKXOR8N")||kw(&L->cur,"XOR8N")||
      kw(&L->cur,"SXORBN")||kw(&L->cur,"SXORBIMM")||kw(&L->cur,"SFLIP8N")){
    /* SXOR8N field n — byte n of TOS ^= field; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = ((base >> sh) & 0xFFul) ^ f;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack imm inverted 8-bit field: SNAND8N · SNOR8N · SXNOR8N (complete after SAND8N/SOR8N/SXOR8N; ladder after SNAND4N) */
  if (kw(&L->cur,"SNAND8N")||kw(&L->cur,"STACKNAND8N")||kw(&L->cur,"NAND8N")||
      kw(&L->cur,"SNANDBN")||kw(&L->cur,"SNANDBIMM")||kw(&L->cur,"SINVERTAND8N")){
    /* SNAND8N field n — byte n of TOS = ~(byte & field) & 0xFF; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = (~(((base >> sh) & 0xFFul) & f)) & 0xFFul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNOR8N")||kw(&L->cur,"STACKNOR8N")||kw(&L->cur,"NOR8N")||
      kw(&L->cur,"SNORBN")||kw(&L->cur,"SNORBIMM")||kw(&L->cur,"SINVERTOR8N")){
    /* SNOR8N field n — byte n of TOS = ~(byte | field) & 0xFF; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = (~(((base >> sh) & 0xFFul) | f)) & 0xFFul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNOR8N")||kw(&L->cur,"STACKXNOR8N")||kw(&L->cur,"XNOR8N")||
      kw(&L->cur,"SXNORBN")||kw(&L->cur,"SXNORBIMM")||kw(&L->cur,"SEQUIV8N")){
    /* SXNOR8N field n — byte n of TOS = ~(byte ^ field) & 0xFF; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = (~(((base >> sh) & 0xFFul) ^ f)) & 0xFFul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack imm 4-bit field bitwise: SAND4N · SOR4N · SXOR4N (merge into nibble n; complete field bitwise ladder) */
  if (kw(&L->cur,"SAND4N")||kw(&L->cur,"STACKAND4N")||kw(&L->cur,"AND4N")||
      kw(&L->cur,"SANDNIBN")||kw(&L->cur,"SANDNIMM")||kw(&L->cur,"SKEEP4N")){
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = ((base >> sh) & 0xFul) & f;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SOR4N")||kw(&L->cur,"STACKOR4N")||kw(&L->cur,"OR4N")||
      kw(&L->cur,"SORNIBN")||kw(&L->cur,"SORNIMM")||kw(&L->cur,"SSETOR4N")){
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = ((base >> sh) & 0xFul) | f;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXOR4N")||kw(&L->cur,"STACKXOR4N")||kw(&L->cur,"XOR4N")||
      kw(&L->cur,"SXORNIBN")||kw(&L->cur,"SXORNIMM")||kw(&L->cur,"SFLIP4N")){
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = ((base >> sh) & 0xFul) ^ f;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack imm inverted 4-bit field: SNAND4N · SNOR4N · SXNOR4N (complete after SAND4N/SOR4N/SXOR4N) */
  if (kw(&L->cur,"SNAND4N")||kw(&L->cur,"STACKNAND4N")||kw(&L->cur,"NAND4N")||
      kw(&L->cur,"SNANDNIBN")||kw(&L->cur,"SNANDNIMM")||kw(&L->cur,"SINVERTAND4N")){
    /* SNAND4N field n — nibble n of TOS = ~(nibble & field) & 0xF; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (~(((base >> sh) & 0xFul) & f)) & 0xFul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNOR4N")||kw(&L->cur,"STACKNOR4N")||kw(&L->cur,"NOR4N")||
      kw(&L->cur,"SNORNIBN")||kw(&L->cur,"SNORNIMM")||kw(&L->cur,"SINVERTOR4N")){
    /* SNOR4N field n — nibble n of TOS = ~(nibble | field) & 0xF; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (~(((base >> sh) & 0xFul) | f)) & 0xFul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNOR4N")||kw(&L->cur,"STACKXNOR4N")||kw(&L->cur,"XNOR4N")||
      kw(&L->cur,"SXNORNIBN")||kw(&L->cur,"SXNORNIMM")||kw(&L->cur,"SEQUIV4N")){
    /* SXNOR4N field n — nibble n of TOS = ~(nibble ^ field) & 0xF; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (~(((base >> sh) & 0xFul) ^ f)) & 0xFul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack imm 4-bit field unary: SNOT4N · SROL4N · SROR4N
   * (nibble-field dual of NOT4CELL/ROL4CELL after SNAND4N; complete stack nibble unary+rotate) */
  if (kw(&L->cur,"SNOT4N")||kw(&L->cur,"STACKNOT4N")||kw(&L->cur,"NOT4N")||
      kw(&L->cur,"SNOTNIBN")||kw(&L->cur,"SINV4N")||kw(&L->cur,"SINVERT4N")){
    /* SNOT4N n — nibble n of TOS = ~nibble & 0xF; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (~((base >> sh) & 0xFul)) & 0xFul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROL4N")||kw(&L->cur,"STACKROL4N")||kw(&L->cur,"ROL4N")||
      kw(&L->cur,"SROTL4N")||kw(&L->cur,"SROLNIBN")||kw(&L->cur,"SROTNIBL")){
    /* SROL4N k n — rotl4 nibble n of TOS by k&3; n clamped 0..15 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (base >> sh) & 0xFul;
    unsigned long rot = (kk == 0) ? w : (((w << (unsigned)kk) | (w >> (unsigned)(4 - kk))) & 0xFul);
    long r = (long)((base & ~(0xFul << sh)) | (rot << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROR4N")||kw(&L->cur,"STACKROR4N")||kw(&L->cur,"ROR4N")||
      kw(&L->cur,"SROTR4N")||kw(&L->cur,"SRORNIBN")||kw(&L->cur,"SROTNIBR")){
    /* SROR4N k n — rotr4 nibble n of TOS by k&3; n clamped 0..15 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (base >> sh) & 0xFul;
    unsigned long rot = (kk == 0) ? w : (((w >> (unsigned)kk) | (w << (unsigned)(4 - kk))) & 0xFul);
    long r = (long)((base & ~(0xFul << sh)) | (rot << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack imm 4-bit field shift: SSHL4N · SSHR4N · SSAR4N
   * (nibble-field dual of SHL4CELL/SHR4CELL/SAR4CELL after SROL4N; complete stack nibble shift) */
  if (kw(&L->cur,"SSHL4N")||kw(&L->cur,"STACKSHL4N")||kw(&L->cur,"SHL4N")||
      kw(&L->cur,"SLSH4N")||kw(&L->cur,"SSHLNIBN")||kw(&L->cur,"SSHIFTNIBL")){
    /* SSHL4N k n — nibble n of TOS = (uint4)<<k (k>=4 → 0); n clamped 0..15 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (base >> sh) & 0xFul;
    unsigned long nw = (kk >= 4) ? 0ul : ((w << (unsigned)kk) & 0xFul);
    long r = (long)((base & ~(0xFul << sh)) | (nw << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSHR4N")||kw(&L->cur,"STACKSHR4N")||kw(&L->cur,"SHR4N")||
      kw(&L->cur,"SLSHR4N")||kw(&L->cur,"SSHRNIBN")||kw(&L->cur,"SSHIFTNIBR")){
    /* SSHR4N k n — nibble n of TOS = (uint4)>>k logical (k>=4 → 0); n clamped 0..15 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (base >> sh) & 0xFul;
    unsigned long nw = (kk >= 4) ? 0ul : (w >> (unsigned)kk);
    long r = (long)((base & ~(0xFul << sh)) | (nw << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSAR4N")||kw(&L->cur,"STACKSAR4N")||kw(&L->cur,"SAR4N")||
      kw(&L->cur,"SASHR4N")||kw(&L->cur,"SSARNIBN")||kw(&L->cur,"SSARSHIFTNIB")){
    /* SSAR4N k n — nibble n of TOS = (int4)>>k arithmetic (k>=4 → all sign); n clamped 0..15 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long va = (long)((base >> sh) & 0xFul);
    if (va & 0x8) va -= 16; /* sign-extend nibble */
    long shifted;
    if (kk >= 4) shifted = (va < 0) ? -1L : 0L;
    else shifted = va >> kk;
    unsigned long nw = (unsigned long)shifted & 0xFul;
    long r = (long)((base & ~(0xFul << sh)) | (nw << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack imm 4-bit field metrics: SBITREV4N · SPOPCNT4N · SPARITY4N
   * (nibble-field dual of BITREV4CELL/POPCNT4CELL/PARITY4CELL after SSHL4N; complete stack nibble metrics) */
  if (kw(&L->cur,"SBITREV4N")||kw(&L->cur,"STACKBITREV4N")||kw(&L->cur,"BITREV4N")||
      kw(&L->cur,"SBREV4N")||kw(&L->cur,"SREV4N")||kw(&L->cur,"SBITREVNIBN")){
    /* SBITREV4N n — nibble n of TOS = bitrev4(nibble); n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (base >> sh) & 0xFul;
    unsigned long rv = 0;
    for (int b = 0; b < 4; b++){
      rv = (rv << 1) | (w & 1u);
      w >>= 1;
    }
    long r = (long)((base & ~(0xFul << sh)) | (rv << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPOPCNT4N")||kw(&L->cur,"STACKPOPCNT4N")||kw(&L->cur,"POPCNT4N")||
      kw(&L->cur,"SPCNT4N")||kw(&L->cur,"SPOPNIBN")||kw(&L->cur,"SNIBPOPN")){
    /* SPOPCNT4N n — nibble n of TOS = popcount(nibble); n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (base >> sh) & 0xFul;
    unsigned long pc = 0;
    while (w){ pc += (w & 1u); w >>= 1; }
    long r = (long)((base & ~(0xFul << sh)) | (pc << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPARITY4N")||kw(&L->cur,"STACKPARITY4N")||kw(&L->cur,"PARITY4N")||
      kw(&L->cur,"SXORRED4N")||kw(&L->cur,"SPARITYNIBN")||kw(&L->cur,"SNIBPARN")){
    /* SPARITY4N n — nibble n of TOS = xor-reduce(nibble) in low bit; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (base >> sh) & 0xFul;
    unsigned long pr = 0;
    while (w){ pr ^= (w & 1u); w >>= 1; }
    long r = (long)((base & ~(0xFul << sh)) | (pr << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack imm 4-bit field zeros+signed: SCLZ4N · SCTZ4N · SNEG4N
   * (nibble-field dual of CLZ4CELL/CTZ4CELL/NEG4CELL after SPARITY4N; complete stack nibble zeros+negate) */
  if (kw(&L->cur,"SCLZ4N")||kw(&L->cur,"STACKCLZ4N")||kw(&L->cur,"CLZ4N")||
      kw(&L->cur,"SNLZ4N")||kw(&L->cur,"SCLZNIBN")||kw(&L->cur,"SNIBCLZN")){
    /* SCLZ4N n — nibble n of TOS = clz4(nibble); 0 → 4; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    unsigned int w = (unsigned int)((base >> sh) & 0xFul);
    unsigned long cz = 0;
    if (w == 0) cz = 4;
    else {
      for (int b = 3; b >= 0; b--){
        if (w & (1u << (unsigned)b)) break;
        cz++;
      }
    }
    long r = (long)((base & ~(0xFul << sh)) | (cz << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCTZ4N")||kw(&L->cur,"STACKCTZ4N")||kw(&L->cur,"CTZ4N")||
      kw(&L->cur,"SNTZ4N")||kw(&L->cur,"SCTZNIBN")||kw(&L->cur,"SNIBCTZN")){
    /* SCTZ4N n — nibble n of TOS = ctz4(nibble); 0 → 4; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    unsigned int w = (unsigned int)((base >> sh) & 0xFul);
    unsigned long tz = 0;
    if (w == 0) tz = 4;
    else {
      while ((w & 1u) == 0){ tz++; w >>= 1; }
    }
    long r = (long)((base & ~(0xFul << sh)) | (tz << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNEG4N")||kw(&L->cur,"STACKNEG4N")||kw(&L->cur,"NEG4N")||
      kw(&L->cur,"SINEG4N")||kw(&L->cur,"SNEGNIBN")||kw(&L->cur,"SNIBNEGN")){
    /* SNEG4N n — nibble n of TOS = -(int4)nibble as uint4; min int4 -8 stays -8 (0x8) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long va = (long)((base >> sh) & 0xFul);
    if (va & 0x8) va -= 16;
    long nr;
    if (va == -8L) nr = -8L; /* keep min int4 */
    else nr = -va;
    unsigned long nv = (unsigned long)(nr & 0xF);
    long r = (long)((base & ~(0xFul << sh)) | (nv << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack imm 4-bit field abs+extend: SABS4N · SSEXT4N · SZEXT4N
   * (nibble-field dual of ABS4CELL/SEXT4CELL/ZEXT4CELL after SNEG4N; complete stack nibble signed extract) */
  if (kw(&L->cur,"SABS4N")||kw(&L->cur,"STACKABS4N")||kw(&L->cur,"ABS4N")||
      kw(&L->cur,"SIABS4N")||kw(&L->cur,"SABSNIBN")||kw(&L->cur,"SNIBABSN")){
    /* SABS4N n — nibble n of TOS = abs(int4); min int4 -8 → +8 (0x8); n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long va = (long)((base >> sh) & 0xFul);
    if (va & 0x8) va -= 16;
    long ar = (va < 0) ? -va : va; /* -8 → 8 fits in nibble */
    unsigned long nv = (unsigned long)(ar & 0xF);
    long r = (long)((base & ~(0xFul << sh)) | (nv << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEXT4N")||kw(&L->cur,"STACKSEXT4N")||kw(&L->cur,"SEXT4N")||
      kw(&L->cur,"SSIGNEXT4N")||kw(&L->cur,"SSEXTNIBN")||kw(&L->cur,"SNIBSEXTN")){
    /* SSEXT4N n — TOS = sign-extend nibble n of TOS to full width; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long va = (long)((base >> sh) & 0xFul);
    if (va & 0x8) va -= 16; /* sign-extend int4 */
    long r = va;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SZEXT4N")||kw(&L->cur,"STACKZEXT4N")||kw(&L->cur,"ZEXT4N")||
      kw(&L->cur,"SZEROEXT4N")||kw(&L->cur,"SZEXTNIBN")||kw(&L->cur,"SNIBZEXTN")){
    /* SZEXT4N n — TOS = zero-extend nibble n of TOS (low4 only); n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long r = (long)((base >> sh) & 0xFul);
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack imm 4-bit field arith merge: SADD4N · SSUB4N · SMUL4N
   * (nibble-field dual of bitwise SAND4N plane; wrap uint4 ALU foundation after SZEXT4N) */
  if (kw(&L->cur,"SADD4N")||kw(&L->cur,"STACKADD4N")||kw(&L->cur,"ADD4N")||
      kw(&L->cur,"SADDNIBN")||kw(&L->cur,"SADDNIMM")||kw(&L->cur,"SINC4N")){
    /* SADD4N field n — nibble n of TOS = (nibble + field) & 0xF; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (((base >> sh) & 0xFul) + f) & 0xFul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSUB4N")||kw(&L->cur,"STACKSUB4N")||kw(&L->cur,"SUB4N")||
      kw(&L->cur,"SSUBNIBN")||kw(&L->cur,"SSUBNIMM")||kw(&L->cur,"SDEC4N")){
    /* SSUB4N field n — nibble n of TOS = (nibble - field) & 0xF; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (((base >> sh) & 0xFul) - f) & 0xFul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMUL4N")||kw(&L->cur,"STACKMUL4N")||kw(&L->cur,"MUL4N")||
      kw(&L->cur,"SMULNIBN")||kw(&L->cur,"SMULNIMM")||kw(&L->cur,"STIMES4N")){
    /* SMUL4N field n — nibble n of TOS = (nibble * field) & 0xF; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (((base >> sh) & 0xFul) * f) & 0xFul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack imm 4-bit field div/mod/min: SDIV4N · SMOD4N · SMIN4N
   * (complete uint4 field ALU after SADD4N/SSUB4N/SMUL4N; field 0 → div/mod 0) */
  if (kw(&L->cur,"SDIV4N")||kw(&L->cur,"STACKDIV4N")||kw(&L->cur,"DIV4N")||
      kw(&L->cur,"SDIVNIBN")||kw(&L->cur,"SDIVNIMM")||kw(&L->cur,"SQUO4N")){
    /* SDIV4N field n — nibble n of TOS = nibble / field (field 0 → 0); n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long v = (base >> sh) & 0xFul;
    unsigned long w = (f == 0) ? 0ul : (v / f);
    long r = (long)((base & ~(0xFul << sh)) | ((w & 0xFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMOD4N")||kw(&L->cur,"STACKMOD4N")||kw(&L->cur,"MOD4N")||
      kw(&L->cur,"SMODNIBN")||kw(&L->cur,"SMODNIMM")||kw(&L->cur,"SREM4N")){
    /* SMOD4N field n — nibble n of TOS = nibble % field (field 0 → 0); n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long v = (base >> sh) & 0xFul;
    unsigned long w = (f == 0) ? 0ul : (v % f);
    long r = (long)((base & ~(0xFul << sh)) | ((w & 0xFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMIN4N")||kw(&L->cur,"STACKMIN4N")||kw(&L->cur,"MIN4N")||
      kw(&L->cur,"SMINNIBN")||kw(&L->cur,"SMINNIMM")||kw(&L->cur,"SLE4N")){
    /* SMIN4N field n — nibble n of TOS = min(nibble, field); n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long v = (base >> sh) & 0xFul;
    unsigned long w = (v < f) ? v : f;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack imm 4-bit field max+eq: SMAX4N · SEQ4N · SNE4N
   * (complete min/max + equality compare plane after SMIN4N; bool writes 0/1 into nibble) */
  if (kw(&L->cur,"SMAX4N")||kw(&L->cur,"STACKMAX4N")||kw(&L->cur,"MAX4N")||
      kw(&L->cur,"SMAXNIBN")||kw(&L->cur,"SMAXNIMM")||kw(&L->cur,"SGE4N")){
    /* SMAX4N field n — nibble n of TOS = max(nibble, field); n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long v = (base >> sh) & 0xFul;
    unsigned long w = (v > f) ? v : f;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SEQ4N")||kw(&L->cur,"STACKEQ4N")||kw(&L->cur,"EQ4N")||
      kw(&L->cur,"SEQNIBN")||kw(&L->cur,"SEQNIMM")||kw(&L->cur,"SCMPEQ4N")){
    /* SEQ4N field n — nibble n of TOS = (nibble == field) ? 1 : 0; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long v = (base >> sh) & 0xFul;
    unsigned long w = (v == f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNE4N")||kw(&L->cur,"STACKNE4N")||kw(&L->cur,"NE4N")||
      kw(&L->cur,"SNENIBN")||kw(&L->cur,"SNENIMM")||kw(&L->cur,"SCMPNE4N")){
    /* SNE4N field n — nibble n of TOS = (nibble != field) ? 1 : 0; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long v = (base >> sh) & 0xFul;
    unsigned long w = (v != f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack imm 8-bit field arith merge: SADD8N · SSUB8N · SMUL8N
   * (byte-field dual of SADD4N/SSUB4N/SMUL4N; wrap uint8 ALU foundation after SAND8N plane) */
  if (kw(&L->cur,"SADD8N")||kw(&L->cur,"STACKADD8N")||kw(&L->cur,"ADD8N")||
      kw(&L->cur,"SADDBYTEN")||kw(&L->cur,"SADD8IMM")||kw(&L->cur,"SINC8N")){
    /* SADD8N field n — byte n of TOS = (byte + field) & 0xFF; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = (((base >> sh) & 0xFFul) + f) & 0xFFul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSUB8N")||kw(&L->cur,"STACKSUB8N")||kw(&L->cur,"SUB8N")||
      kw(&L->cur,"SSUBBYTEN")||kw(&L->cur,"SSUB8IMM")||kw(&L->cur,"SDEC8N")){
    /* SSUB8N field n — byte n of TOS = (byte - field) & 0xFF; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = (((base >> sh) & 0xFFul) - f) & 0xFFul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMUL8N")||kw(&L->cur,"STACKMUL8N")||kw(&L->cur,"MUL8N")||
      kw(&L->cur,"SMULBYTEN")||kw(&L->cur,"SMUL8IMM")||kw(&L->cur,"STIMES8N")){
    /* SMUL8N field n — byte n of TOS = (byte * field) & 0xFF; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = (((base >> sh) & 0xFFul) * f) & 0xFFul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 stack imm 8-bit field div/mod/min: SDIV8N · SMOD8N · SMIN8N
   * (complete uint8 field ALU after SADD8N/SSUB8N/SMUL8N; field 0 → div/mod 0) */
  if (kw(&L->cur,"SDIV8N")||kw(&L->cur,"STACKDIV8N")||kw(&L->cur,"DIV8N")||
      kw(&L->cur,"SDIVBYTEN")||kw(&L->cur,"SDIV8IMM")||kw(&L->cur,"SQUO8N")){
    /* SDIV8N field n — byte n of TOS = byte / field (field 0 → 0); n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long v = (base >> sh) & 0xFFul;
    unsigned long w = (f == 0) ? 0ul : (v / f);
    long r = (long)((base & ~(0xFFul << sh)) | ((w & 0xFFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMOD8N")||kw(&L->cur,"STACKMOD8N")||kw(&L->cur,"MOD8N")||
      kw(&L->cur,"SMODBYTEN")||kw(&L->cur,"SMOD8IMM")||kw(&L->cur,"SREM8N")){
    /* SMOD8N field n — byte n of TOS = byte % field (field 0 → 0); n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long v = (base >> sh) & 0xFFul;
    unsigned long w = (f == 0) ? 0ul : (v % f);
    long r = (long)((base & ~(0xFFul << sh)) | ((w & 0xFFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMIN8N")||kw(&L->cur,"STACKMIN8N")||kw(&L->cur,"MIN8N")||
      kw(&L->cur,"SMINBYTEN")||kw(&L->cur,"SMIN8IMM")||kw(&L->cur,"SLE8N")){
    /* SMIN8N field n — byte n of TOS = min(byte, field); n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long v = (base >> sh) & 0xFFul;
    unsigned long w = (v < f) ? v : f;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SALIGN")||kw(&L->cur,"SROUNDUP")||kw(&L->cur,"STACKALIGN")||
      kw(&L->cur,"SALIGNDN")||kw(&L->cur,"SROUNDDN")||kw(&L->cur,"STACKALIGNDN")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long al = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = a;
    int is_dn = (strcmp(op,"SALIGNDN")==0 || strcmp(op,"SROUNDDN")==0 ||
                 strcmp(op,"STACKALIGNDN")==0);
    if (al > 0){
      long q = a / al, rem = a % al;
      if (rem != 0){
        if (is_dn) r = (a > 0) ? q * al : (q - 1) * al;
        else r = (a > 0) ? (q + 1) * al : q * al;
      }
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPACK16")||kw(&L->cur,"SPACK")||kw(&L->cur,"STACKPACK")||
      kw(&L->cur,"STACKPACK16")){
    /* hi lo → (hi<<16)|lo  (16-bit halves) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long lo = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    unsigned int h = (unsigned int)hi & 0xFFFFu;
    unsigned int l = (unsigned int)lo & 0xFFFFu;
    long r = (long)((h << 16) | l);
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 pack8/nibble + set nibble: SPACK8 SPACKNIB SSETNIB */
  if (kw(&L->cur,"SPACK8")||kw(&L->cur,"SPACKB")||kw(&L->cur,"STACKPACK8")||
      kw(&L->cur,"SPACKNIB")||kw(&L->cur,"SPACK4")||kw(&L->cur,"STACKPACKNIB")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long lo = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long r;
    if (strcmp(op,"SPACKNIB")==0 || strcmp(op,"SPACK4")==0 || strcmp(op,"STACKPACKNIB")==0)
      r = (long)((((unsigned int)hi & 0xFu) << 4) | ((unsigned int)lo & 0xFu));
    else
      r = (long)((((unsigned int)hi & 0xFFu) << 8) | ((unsigned int)lo & 0xFFu));
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSETNIB")||kw(&L->cur,"SSETNIBBLE")||kw(&L->cur,"STACKSETNIB")){
    /* a field i → deposit 4-bit field at nibble index i */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    long field = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i > 15) i = 15;
    unsigned long base = (unsigned long)a;
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long shift = (unsigned long)(i * 4);
    long r = (long)((base & ~(0xFul << shift)) | (f << shift));
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 word data path: SBSWAP SBITREV SPARITY SNIBBLE */
  if (kw(&L->cur,"SBSWAP")||kw(&L->cur,"SBSWAP32")||kw(&L->cur,"STACKBSWAP")||
      kw(&L->cur,"SBITREV")||kw(&L->cur,"SREVBITS")||kw(&L->cur,"STACKBITREV")||
      kw(&L->cur,"SPARITY")||kw(&L->cur,"STACKPARITY")||kw(&L->cur,"SPAR")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (strcmp(op,"SBSWAP")==0 || strcmp(op,"SBSWAP32")==0 || strcmp(op,"STACKBSWAP")==0){
      unsigned int w = (unsigned int)a;
      w = ((w & 0x000000FFu) << 24) | ((w & 0x0000FF00u) << 8) |
          ((w & 0x00FF0000u) >> 8) | ((w & 0xFF000000u) >> 24);
      r = (long)w;
    } else if (strcmp(op,"SBITREV")==0 || strcmp(op,"SREVBITS")==0 ||
               strcmp(op,"STACKBITREV")==0){
      unsigned int w = (unsigned int)a;
      unsigned int rv = 0;
      for (int i = 0; i < 32; i++){
        rv = (rv << 1) | (w & 1u);
        w >>= 1;
      }
      r = (long)rv;
    } else {
      /* SPARITY / STACKPARITY / SPAR */
      unsigned long u = (unsigned long)a;
      int n = 0;
      while (u){ n ^= (int)(u & 1u); u >>= 1; }
      r = (long)n;
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNIB")||kw(&L->cur,"SNIBBLE")||kw(&L->cur,"STACKNIBBLE")||
      kw(&L->cur,"SGETNIB")){
    /* a i → i-th little-endian nibble of a (i clamped 0..15) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i > 15) i = 15;
    long r = (long)(((unsigned long)a >> (unsigned)(i * 4)) & 0xFul);
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack imm nibble field: SNIBN · SSETNIBN · SCLRNIBN (imm dual of SNIB/SSETNIB; control-word plane) */
  if (kw(&L->cur,"SNIBN")||kw(&L->cur,"STACKNIBN")||kw(&L->cur,"SGETNIBN")||
      kw(&L->cur,"NIBN")||kw(&L->cur,"GETNIBN")||kw(&L->cur,"SNIBBLEN")){
    /* SNIBN n — TOS = little-endian nibble n of TOS; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    long r = (long)(((unsigned long)vm->stack[vm->sp - 1] >> (unsigned)(n * 4)) & 0xFul);
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSETNIBN")||kw(&L->cur,"STACKSETNIBN")||kw(&L->cur,"SSETNIBIMM")||
      kw(&L->cur,"SETNIBN")||kw(&L->cur,"PUTNIBN")){
    /* SSETNIBN field n — deposit low 4 bits of field into nibble n of TOS; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long shift = (unsigned long)(n * 4);
    long r = (long)((base & ~(0xFul << shift)) | (f << shift));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLRNIBN")||kw(&L->cur,"STACKCLRNIBN")||kw(&L->cur,"SCLRNIBIMM")||
      kw(&L->cur,"CLRNIBN")||kw(&L->cur,"ZAPNIBN")){
    /* SCLRNIBN n — clear little-endian nibble n of TOS; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long shift = (unsigned long)(n * 4);
    long r = (long)(base & ~(0xFul << shift));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 pack32 + PEXT/PDEP + interleave (universal bit data-path) */
  if (kw(&L->cur,"SPACK32")||kw(&L->cur,"SPACKW")||kw(&L->cur,"STACKPACK32")){
    /* hi lo → (hi<<32)|lo  (32-bit halves → 64-bit word) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long lo = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    unsigned long long h = (unsigned long long)(unsigned int)hi;
    unsigned long long l = (unsigned long long)(unsigned int)lo;
    long r = (long)((h << 32) | l);
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHI32")||kw(&L->cur,"SHIWORD")||kw(&L->cur,"STACKHI32")||
      kw(&L->cur,"SLO32")||kw(&L->cur,"SLOWORD")||kw(&L->cur,"STACKLO32")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long long u = (unsigned long long)vm->stack[--vm->sp];
    long r;
    if (strcmp(op,"SHI32")==0 || strcmp(op,"SHIWORD")==0 || strcmp(op,"STACKHI32")==0)
      r = (long)(unsigned int)(u >> 32);
    else
      r = (long)(unsigned int)(u & 0xFFFFFFFFu);
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPEXT")||kw(&L->cur,"SPEXTRACT")||kw(&L->cur,"STACKPEXT")||
      kw(&L->cur,"SPEX")||kw(&L->cur,"SPDEP")||kw(&L->cur,"SPDEPOSIT")||
      kw(&L->cur,"STACKPDEP")||kw(&L->cur,"SPDP")){
    /* SPEXT: src mask → parallel extract bits of src under mask (BMI2 PEXT)
     * SPDEP: src mask → deposit low bits of src into mask positions (BMI2 PDEP) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_dep = (strcmp(op,"SPDEP")==0 || strcmp(op,"SPDEPOSIT")==0 ||
                  strcmp(op,"STACKPDEP")==0 || strcmp(op,"SPDP")==0);
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long long mask = (unsigned long long)vm->stack[--vm->sp];
    unsigned long long src  = (unsigned long long)vm->stack[--vm->sp];
    unsigned long long res = 0;
    if (is_dep){
      unsigned long long bb = 1;
      for (int i=0;i<64;i++){
        if ((mask >> i) & 1ull){
          if (src & bb) res |= (1ull << i);
          bb <<= 1;
        }
      }
    } else {
      unsigned long long k = 0;
      for (int i=0;i<64;i++){
        if ((mask >> i) & 1ull){
          if ((src >> i) & 1ull) res |= (1ull << k);
          k++;
        }
      }
    }
    long r = (long)res;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SZIP")||kw(&L->cur,"SINTERLEAVE")||kw(&L->cur,"STACKZIP")||
      kw(&L->cur,"SZIPBITS")||kw(&L->cur,"SMORTON")){
    /* a b → interleave low 32 bits: ... b1 a1 b0 a0 (Morton / zip) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long long b = (unsigned long long)(unsigned int)vm->stack[--vm->sp];
    unsigned long long a = (unsigned long long)(unsigned int)vm->stack[--vm->sp];
    unsigned long long r = 0;
    for (int i=0;i<32;i++){
      if ((a >> i) & 1ull) r |= (1ull << (2*i));
      if ((b >> i) & 1ull) r |= (1ull << (2*i + 1));
    }
    long out = (long)r;
    vm->stack[vm->sp++] = out;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",out); vm->last_n=out;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUNZIP")||kw(&L->cur,"SDEINTERLEAVE")||kw(&L->cur,"STACKUNZIP")||
      kw(&L->cur,"SUNZIPBITS")){
    /* z → push even bits (lo), odd bits (hi) as two stack values: lo hi (TOS=hi)
     * convention: after SUNZIP, TOS=odd/hi half, NOS=even/lo half */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 1 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    unsigned long long z = (unsigned long long)vm->stack[--vm->sp];
    unsigned long long even = 0, odd = 0;
    for (int i=0;i<32;i++){
      if ((z >> (2*i)) & 1ull) even |= (1ull << i);
      if ((z >> (2*i + 1)) & 1ull) odd |= (1ull << i);
    }
    vm->stack[vm->sp++] = (long)even;
    vm->stack[vm->sp++] = (long)odd;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",(long)odd); vm->last_n=(long)odd;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 sign/zero extend: SSEXT SZEXT SSEXT8 SSEXT16 */
  if (kw(&L->cur,"SSEXT8")||kw(&L->cur,"SSEXTB")||kw(&L->cur,"STACKSEXT8")||
      kw(&L->cur,"SSEXT16")||kw(&L->cur,"SSEXTW")||kw(&L->cur,"STACKSEXT16")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r;
    if (strcmp(op,"SSEXT16")==0 || strcmp(op,"SSEXTW")==0 || strcmp(op,"STACKSEXT16")==0){
      r = a & 0xFFFFL;
      if (r & 0x8000L) r |= ~0xFFFFL;
    } else {
      r = a & 0xFFL;
      if (r & 0x80L) r |= ~0xFFL;
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEXT")||kw(&L->cur,"STACKSEXT")||kw(&L->cur,"SSIGNEXT")||
      kw(&L->cur,"SZEXT")||kw(&L->cur,"STACKZEXT")||kw(&L->cur,"SZEROEXT")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long w = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    int is_z = (strcmp(op,"SZEXT")==0 || strcmp(op,"STACKZEXT")==0 ||
                strcmp(op,"SZEROEXT")==0);
    if (w <= 0) r = 0;
    else if (w >= 63) r = a;
    else {
      unsigned long mask = (1ul << (unsigned)w) - 1ul;
      unsigned long v = (unsigned long)a & mask;
      if (!is_z){
        unsigned long sign = 1ul << (unsigned)(w - 1);
        if (v & sign) v |= ~mask;
      }
      r = (long)v;
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack science/math duals: SAVG SPCT SHYP SHAM SDIST */
  if (kw(&L->cur,"SAVG")||kw(&L->cur,"STACKAVG")||
      kw(&L->cur,"SPCT")||kw(&L->cur,"STACKPCT")||kw(&L->cur,"SPERCENT")||
      kw(&L->cur,"SHYP")||kw(&L->cur,"STACKHYP")||kw(&L->cur,"SHYPOT")||
      kw(&L->cur,"SHAM")||kw(&L->cur,"SHAMMING")||kw(&L->cur,"STACKHAMMING")||
      kw(&L->cur,"SDIST")||kw(&L->cur,"SABSDIFF")||kw(&L->cur,"STACKDIST")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (strcmp(op,"SAVG")==0 || strcmp(op,"STACKAVG")==0)
      r = (a + b) / 2;
    else if (strcmp(op,"SPCT")==0 || strcmp(op,"STACKPCT")==0 || strcmp(op,"SPERCENT")==0)
      r = b ? (a * 100 / b) : 0;
    else if (strcmp(op,"SHYP")==0 || strcmp(op,"STACKHYP")==0 || strcmp(op,"SHYPOT")==0){
      long s = a * a + b * b;
      if (s < 0) r = 0;
      else {
        long t = 0;
        while ((t + 1) * (t + 1) <= s) t++;
        r = t;
      }
    } else if (strcmp(op,"SHAM")==0 || strcmp(op,"SHAMMING")==0 ||
               strcmp(op,"STACKHAMMING")==0){
      unsigned long u = (unsigned long)(a ^ b);
      int n = 0;
      while (u){ n += (int)(u & 1u); u >>= 1; }
      r = (long)n;
    } else {
      /* SDIST / SABSDIFF / STACKDIST — |a-b| */
      long d = a - b;
      r = d < 0 ? -d : d;
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 foundation stack: SMASK SISDIV */
  if (kw(&L->cur,"SMASK")||kw(&L->cur,"SBITMASK")||kw(&L->cur,"STACKMASK")){
    /* n → low n bits set */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r;
    if (a <= 0) r = 0;
    else if (a >= 63) r = (long)~0ul;
    else r = (long)((1ul << (unsigned)a) - 1ul);
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SISDIV")||kw(&L->cur,"SDIVISIBLE")||kw(&L->cur,"STACKISDIV")){
    /* a b → 1 if b!=0 and a multiple of b */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = (b != 0 && (a % b) == 0) ? 1 : 0;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
