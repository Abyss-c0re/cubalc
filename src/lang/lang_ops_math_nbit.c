/* CubalC lang — lang_ops_math_nbit.c
 * MATH N-suffix plane: DIVFLOOR/CEIL · SNAP/ALIGN · POW2 · ILOG2 · POPCOUNT · BITREV
 * Peel from math residual (B_math_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_math_nbit.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_math_try_nbit(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* DIVFLOORN|TRUEFLOORN|FLOORDIVN a b — true integer floor division floor(a/b) → LAST_N.
   * b==0 soft LAST_N=0 + sticky LAST_ERR. Toward -∞ when rem and signs differ.
   * Twin of DIVCEILN. Usability: bucket = DIVFLOORN idx width without shell glue. */
  if (kw(&L->cur,"DIVFLOORN") || kw(&L->cur,"TRUEFLOORN") || kw(&L->cur,"FLOORDIVN") ||
      kw(&L->cur,"FLOOR_DIVN") || kw(&L->cur,"IDIVFLOOR") ||
      kw(&L->cur,"BUCKETN") || kw(&L->cur,"BININDEXN")){
    long a = 0, b = 0, out = 0;
    int bad = 0;
    char nbuf[32];
    lex_next(L);
    if (L->cur.kind == TK_NUM) { a = L->cur.num; lex_next(L); }
    else if (L->cur.kind == TK_MINUS) {
      lex_next(L);
      if (L->cur.kind != TK_NUM) { var_set_num(vm,"OK",0); bump(vm); return 1; }
      a = -L->cur.num; lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *v = var_get(vm, L->cur.text, 0);
      if (!v) { fail_at(vm, L, "DIVFLOORN a b — unknown a"); return -1; }
      a = v->val; lex_next(L);
    } else {
      fail_at(vm, L, "DIVFLOORN a b — DIVFLOORN idx width");
      return -1;
    }
    if (kw(&L->cur,"BY") || kw(&L->cur,"OVER") || kw(&L->cur,"INTO") ||
        kw(&L->cur,"PER"))
      lex_next(L);
    if (L->cur.kind == TK_NUM) { b = L->cur.num; lex_next(L); }
    else if (L->cur.kind == TK_MINUS) {
      lex_next(L);
      if (L->cur.kind != TK_NUM) { var_set_num(vm,"OK",0); bump(vm); return 1; }
      b = -L->cur.num; lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *v = var_get(vm, L->cur.text, 0);
      if (!v) { fail_at(vm, L, "DIVFLOORN a b — unknown b"); return -1; }
      b = v->val; lex_next(L);
    } else {
      fail_at(vm, L, "DIVFLOORN a b — missing divisor");
      return -1;
    }
    if (b == 0) {
      out = 0; bad = 1;
      var_set_str(vm, "LAST_ERR", "DIVFLOORN: divide by zero");
      var_set_str(vm, "ERR", "DIVFLOORN: divide by zero");
    } else if (a == LONG_MIN && b == -1) {
      out = LONG_MAX; bad = 1;
      var_set_str(vm, "LAST_ERR", "DIVFLOORN: overflow");
      var_set_str(vm, "ERR", "DIVFLOORN: overflow");
    } else {
      long q = a / b, rem = a % b;
      if (rem != 0 && ((a < 0) != (b < 0))) q--;
      out = q;
    }
    snprintf(nbuf, sizeof nbuf, "%ld", out);
    var_set_num(vm, "LAST_N", out);
    vm->last_n = out;
    var_set_num(vm, "DIVFLOORN", out);
    var_set_num(vm, "TRUEFLOORN", out);
    var_set_num(vm, "FLOORDIVN", out);
    var_set_num(vm, "BUCKETN", out);
    var_set_num(vm, "DIVFLOORN_A", a);
    var_set_num(vm, "DIVFLOORN_B", b);
    var_set_num(vm, "DIVFLOORN_OK", bad ? 0L : 1L);
    var_set_num(vm, "OK", 1);
    var_set_str(vm, "LAST", nbuf);
    var_set_str(vm, "FLAG", nbuf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", nbuf);
    if (vm->trace)
      fprintf(vm->trace, "# divfloorn %ld/%ld -> %ld bad=%d\n", a, b, out, bad);
    bump(vm); return 1;
  }



  /* DIVCEILN|TRUECEILN|CEILDIVN a b — true integer ceiling division ceil(a/b) → LAST_N.
   * b==0 soft LAST_N=0 + sticky LAST_ERR. Toward +∞ when rem and signs same.
   * Twin of DIVFLOORN. Usability: pages = DIVCEILN bytes pagesz without shell glue. */
  if (kw(&L->cur,"DIVCEILN") || kw(&L->cur,"TRUECEILN") || kw(&L->cur,"CEILDIVN") ||
      kw(&L->cur,"CEIL_DIVN") || kw(&L->cur,"IDIVCEIL") ||
      kw(&L->cur,"PAGESN") || kw(&L->cur,"COVERN") || kw(&L->cur,"CHUNKCOUNTN")){
    long a = 0, b = 0, out = 0;
    int bad = 0;
    char nbuf[32];
    lex_next(L);
    if (L->cur.kind == TK_NUM) { a = L->cur.num; lex_next(L); }
    else if (L->cur.kind == TK_MINUS) {
      lex_next(L);
      if (L->cur.kind != TK_NUM) { var_set_num(vm,"OK",0); bump(vm); return 1; }
      a = -L->cur.num; lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *v = var_get(vm, L->cur.text, 0);
      if (!v) { fail_at(vm, L, "DIVCEILN a b — unknown a"); return -1; }
      a = v->val; lex_next(L);
    } else {
      fail_at(vm, L, "DIVCEILN a b — DIVCEILN bytes pagesz");
      return -1;
    }
    if (kw(&L->cur,"BY") || kw(&L->cur,"OVER") || kw(&L->cur,"INTO") ||
        kw(&L->cur,"PER"))
      lex_next(L);
    if (L->cur.kind == TK_NUM) { b = L->cur.num; lex_next(L); }
    else if (L->cur.kind == TK_MINUS) {
      lex_next(L);
      if (L->cur.kind != TK_NUM) { var_set_num(vm,"OK",0); bump(vm); return 1; }
      b = -L->cur.num; lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *v = var_get(vm, L->cur.text, 0);
      if (!v) { fail_at(vm, L, "DIVCEILN a b — unknown b"); return -1; }
      b = v->val; lex_next(L);
    } else {
      fail_at(vm, L, "DIVCEILN a b — missing divisor");
      return -1;
    }
    if (b == 0) {
      out = 0; bad = 1;
      var_set_str(vm, "LAST_ERR", "DIVCEILN: divide by zero");
      var_set_str(vm, "ERR", "DIVCEILN: divide by zero");
    } else if (a == LONG_MIN && b == -1) {
      out = LONG_MAX; bad = 1;
      var_set_str(vm, "LAST_ERR", "DIVCEILN: overflow");
      var_set_str(vm, "ERR", "DIVCEILN: overflow");
    } else {
      long q = a / b, rem = a % b;
      /* ceil: bump when nonzero remainder and same-sign operands */
      if (rem != 0 && ((a < 0) == (b < 0))) q++;
      out = q;
    }
    snprintf(nbuf, sizeof nbuf, "%ld", out);
    var_set_num(vm, "LAST_N", out);
    vm->last_n = out;
    var_set_num(vm, "DIVCEILN", out);
    var_set_num(vm, "TRUECEILN", out);
    var_set_num(vm, "CEILDIVN", out);
    var_set_num(vm, "PAGESN", out);
    var_set_num(vm, "COVERN", out);
    var_set_num(vm, "CHUNKCOUNTN", out);
    var_set_num(vm, "DIVCEILN_A", a);
    var_set_num(vm, "DIVCEILN_B", b);
    var_set_num(vm, "DIVCEILN_OK", bad ? 0L : 1L);
    var_set_num(vm, "OK", 1);
    var_set_str(vm, "LAST", nbuf);
    var_set_str(vm, "FLAG", nbuf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", nbuf);
    if (vm->trace)
      fprintf(vm->trace, "# divceiln %ld/%ld -> %ld bad=%d\n", a, b, out, bad);
    bump(vm); return 1;
  }


  /* SNAPN|ALIGNN|ROUNDTON|GRIDSNAPN|NEARESTN|GRIDN|QUANTIZEN x step —
   * snap x to nearest multiple of |step| (half away from zero) → LAST_N.
   * step==0 soft keep x + sticky LAST_ERR. Optional TO/BY before step.
   * Usability: GETFLAGN offset then SNAPN LAST_N 64 without shell round glue. */
  if (kw(&L->cur,"SNAPN") || kw(&L->cur,"ALIGNN") || kw(&L->cur,"ROUNDTON") ||
      kw(&L->cur,"GRIDSNAPN") || kw(&L->cur,"NEARESTN") || kw(&L->cur,"GRIDN") ||
      kw(&L->cur,"QUANTIZEN") || kw(&L->cur,"SNAP_TO_N") || kw(&L->cur,"ALIGN_TO_N")){
    long x = 0, step = 0, out = 0, astep = 0;
    int bad = 0, changed = 0;
    char nbuf[32];
    lex_next(L);
    if (L->cur.kind == TK_NUM) { x = L->cur.num; lex_next(L); }
    else if (L->cur.kind == TK_MINUS) {
      lex_next(L);
      if (L->cur.kind != TK_NUM) { var_set_num(vm,"OK",0); bump(vm); return 1; }
      x = -L->cur.num; lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *v = var_get(vm, L->cur.text, 0);
      if (!v) { fail_at(vm, L, "SNAPN x step — unknown x"); return -1; }
      x = v->val; lex_next(L);
    } else {
      fail_at(vm, L, "SNAPN x step — SNAPN value step");
      return -1;
    }
    if (kw(&L->cur,"TO") || kw(&L->cur,"BY") || kw(&L->cur,"STEP") ||
        kw(&L->cur,"GRID") || kw(&L->cur,"OF"))
      lex_next(L);
    if (L->cur.kind == TK_NUM) { step = L->cur.num; lex_next(L); }
    else if (L->cur.kind == TK_MINUS) {
      lex_next(L);
      if (L->cur.kind != TK_NUM) { var_set_num(vm,"OK",0); bump(vm); return 1; }
      step = -L->cur.num; lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *v = var_get(vm, L->cur.text, 0);
      if (!v) { fail_at(vm, L, "SNAPN x step — unknown step"); return -1; }
      step = v->val; lex_next(L);
    } else {
      fail_at(vm, L, "SNAPN x step — missing step");
      return -1;
    }
    astep = step < 0 ? -step : step;
    if (astep == 0) {
      out = x; bad = 1;
      var_set_str(vm, "LAST_ERR", "SNAPN: step zero");
      var_set_str(vm, "ERR", "SNAPN: step zero");
    } else {
      /* half away from zero: q = round(x / astep) * astep */
      long ax = x < 0 ? -x : x;
      long q = ax / astep;
      long r = ax % astep;
      if (r * 2 >= astep) q++;
      out = q * astep;
      if (x < 0) out = -out;
    }
    changed = (out != x) ? 1 : 0;
    snprintf(nbuf, sizeof nbuf, "%ld", out);
    var_set_num(vm, "LAST_N", out);
    vm->last_n = out;
    var_set_num(vm, "SNAPN", out);
    var_set_num(vm, "ALIGNN", out);
    var_set_num(vm, "ROUNDTON", out);
    var_set_num(vm, "GRIDSNAPN", out);
    var_set_num(vm, "NEARESTN", out);
    var_set_num(vm, "GRIDN", out);
    var_set_num(vm, "QUANTIZEN", out);
    var_set_num(vm, "SNAPN_X", x);
    var_set_num(vm, "SNAPN_STEP", astep);
    var_set_num(vm, "SNAPN_CHANGED", (long)changed);
    var_set_num(vm, "SNAPN_OK", bad ? 0L : 1L);
    var_set_num(vm, "OK", 1);
    var_set_str(vm, "LAST", nbuf);
    var_set_str(vm, "FLAG", nbuf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", nbuf);
    if (vm->trace)
      fprintf(vm->trace, "# snapn %ld step=%ld -> %ld bad=%d\n", x, astep, out, bad);
    bump(vm); return 1;
  }



  /* ALIGNUPN|CEILALIGNN|SNAPUPN x step — raise x to next multiple of |step| → LAST_N.
   * ALIGNDOWNN|FLOORALIGNN|SNAPDOWNN x step — lower x to previous multiple of |step|.
   * step==0 soft keep x + sticky LAST_ERR. Optional TO/BY before step.
   * Exact multiples stay put. Toward +∞ / −∞ on the number line (sign-aware).
   * Twin of SNAPN (nearest). Usability: pad = ALIGNUPN size 64; base = ALIGNDOWNN addr 4096. */
  if (kw(&L->cur,"ALIGNUPN") || kw(&L->cur,"CEILALIGNN") || kw(&L->cur,"SNAPUPN") ||
      kw(&L->cur,"PADALIGNN") || kw(&L->cur,"ALIGN_UP_N") || kw(&L->cur,"CEIL_TO_N") ||
      kw(&L->cur,"ALIGNDOWNN") || kw(&L->cur,"FLOORALIGNN") || kw(&L->cur,"SNAPDOWNN") ||
      kw(&L->cur,"TRUNCALIGNN") || kw(&L->cur,"ALIGN_DOWN_N") || kw(&L->cur,"FLOOR_TO_N")){
    long x = 0, step = 0, out = 0, astep = 0;
    int bad = 0, changed = 0, is_up = 0;
    char nbuf[32];
    is_up = kw(&L->cur,"ALIGNUPN") || kw(&L->cur,"CEILALIGNN") || kw(&L->cur,"SNAPUPN") ||
            kw(&L->cur,"PADALIGNN") || kw(&L->cur,"ALIGN_UP_N") || kw(&L->cur,"CEIL_TO_N");
    lex_next(L);
    if (L->cur.kind == TK_NUM) { x = L->cur.num; lex_next(L); }
    else if (L->cur.kind == TK_MINUS) {
      lex_next(L);
      if (L->cur.kind != TK_NUM) { var_set_num(vm,"OK",0); bump(vm); return 1; }
      x = -L->cur.num; lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *v = var_get(vm, L->cur.text, 0);
      if (!v) { fail_at(vm, L, is_up ? "ALIGNUPN x step — unknown x" : "ALIGNDOWNN x step — unknown x"); return -1; }
      x = v->val; lex_next(L);
    } else {
      fail_at(vm, L, is_up ? "ALIGNUPN x step — ALIGNUPN size 64" : "ALIGNDOWNN x step — ALIGNDOWNN addr 4096");
      return -1;
    }
    if (kw(&L->cur,"TO") || kw(&L->cur,"BY") || kw(&L->cur,"STEP") ||
        kw(&L->cur,"GRID") || kw(&L->cur,"OF"))
      lex_next(L);
    if (L->cur.kind == TK_NUM) { step = L->cur.num; lex_next(L); }
    else if (L->cur.kind == TK_MINUS) {
      lex_next(L);
      if (L->cur.kind != TK_NUM) { var_set_num(vm,"OK",0); bump(vm); return 1; }
      step = -L->cur.num; lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *v = var_get(vm, L->cur.text, 0);
      if (!v) { fail_at(vm, L, is_up ? "ALIGNUPN x step — unknown step" : "ALIGNDOWNN x step — unknown step"); return -1; }
      step = v->val; lex_next(L);
    } else {
      fail_at(vm, L, is_up ? "ALIGNUPN x step — missing step" : "ALIGNDOWNN x step — missing step");
      return -1;
    }
    astep = step < 0 ? -step : step;
    if (astep == 0) {
      out = x; bad = 1;
      var_set_str(vm, "LAST_ERR", is_up ? "ALIGNUPN: step zero" : "ALIGNDOWNN: step zero");
      var_set_str(vm, "ERR", is_up ? "ALIGNUPN: step zero" : "ALIGNDOWNN: step zero");
    } else if (x >= 0) {
      long r = x % astep;
      if (is_up) {
        out = (r == 0) ? x : x + (astep - r);
      } else {
        out = x - r;
      }
    } else {
      /* negative: toward +∞ for up, toward −∞ for down on the number line */
      long ax = -x;
      long r = ax % astep;
      if (is_up) {
        /* ceil toward +∞: -17 step 8 → -16 */
        out = (r == 0) ? x : -(ax - r);
      } else {
        /* floor toward −∞: -17 step 8 → -24 */
        out = (r == 0) ? x : -(ax + (astep - r));
      }
    }
    changed = (out != x) ? 1 : 0;
    snprintf(nbuf, sizeof nbuf, "%ld", out);
    var_set_num(vm, "LAST_N", out);
    vm->last_n = out;
    if (is_up) {
      var_set_num(vm, "ALIGNUPN", out);
      var_set_num(vm, "CEILALIGNN", out);
      var_set_num(vm, "SNAPUPN", out);
      var_set_num(vm, "PADALIGNN", out);
      var_set_num(vm, "ALIGNUPN_X", x);
      var_set_num(vm, "ALIGNUPN_STEP", astep);
      var_set_num(vm, "ALIGNUPN_CHANGED", (long)changed);
      var_set_num(vm, "ALIGNUPN_OK", bad ? 0L : 1L);
    } else {
      var_set_num(vm, "ALIGNDOWNN", out);
      var_set_num(vm, "FLOORALIGNN", out);
      var_set_num(vm, "SNAPDOWNN", out);
      var_set_num(vm, "TRUNCALIGNN", out);
      var_set_num(vm, "ALIGNDOWNN_X", x);
      var_set_num(vm, "ALIGNDOWNN_STEP", astep);
      var_set_num(vm, "ALIGNDOWNN_CHANGED", (long)changed);
      var_set_num(vm, "ALIGNDOWNN_OK", bad ? 0L : 1L);
    }
    var_set_num(vm, "OK", 1);
    var_set_str(vm, "LAST", nbuf);
    var_set_str(vm, "FLAG", nbuf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", nbuf);
    if (vm->trace)
      fprintf(vm->trace, "# align%sn %ld step=%ld -> %ld bad=%d\n",
              is_up ? "up" : "down", x, astep, out, bad);
    bump(vm); return 1;
  }


  /* ISQRTN|SQRTN|IROOT2N|FLOORSQRTN x — floor integer square root → LAST_N.
   * x<0 soft LAST_N=0 + sticky LAST_ERR (ISQRTN_OK=0). Binary-search floor sqrt.
   * Twin of SYS ISQRT without SYS glue. Usability: side = ISQRTN area after GETFLAGN. */
  if (kw(&L->cur,"ISQRTN") || kw(&L->cur,"SQRTN") || kw(&L->cur,"IROOT2N") ||
      kw(&L->cur,"FLOORSQRTN") || kw(&L->cur,"INTSQRTN") || kw(&L->cur,"ROOT2N") ||
      kw(&L->cur,"ISQRT_N")){
    long x = 0, out = 0;
    int bad = 0;
    char nbuf[32];
    lex_next(L);
    if (L->cur.kind == TK_NUM) { x = L->cur.num; lex_next(L); }
    else if (L->cur.kind == TK_MINUS) {
      lex_next(L);
      if (L->cur.kind != TK_NUM) { var_set_num(vm,"OK",0); bump(vm); return 1; }
      x = -L->cur.num; lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *v = var_get(vm, L->cur.text, 0);
      if (!v) { fail_at(vm, L, "ISQRTN x — unknown x"); return -1; }
      x = v->val; lex_next(L);
    } else {
      fail_at(vm, L, "ISQRTN x — ISQRTN area");
      return -1;
    }
    if (x < 0) {
      out = 0; bad = 1;
      var_set_str(vm, "LAST_ERR", "ISQRTN: negative");
      var_set_str(vm, "ERR", "ISQRTN: negative");
    } else if (x == 0) {
      out = 0;
    } else {
      /* binary search floor sqrt; hi capped so mid*mid stays in signed 64 */
      long lo = 1, hi = x;
      if (hi > 3037000499L) hi = 3037000499L;
      out = 1;
      while (lo <= hi) {
        long mid = lo + (hi - lo) / 2;
        unsigned long long sq = (unsigned long long)mid * (unsigned long long)mid;
        if (sq == (unsigned long long)x) { out = mid; break; }
        if (sq < (unsigned long long)x) { out = mid; lo = mid + 1; }
        else hi = mid - 1;
      }
    }
    snprintf(nbuf, sizeof nbuf, "%ld", out);
    var_set_num(vm, "LAST_N", out);
    vm->last_n = out;
    var_set_num(vm, "ISQRTN", out);
    var_set_num(vm, "SQRTN", out);
    var_set_num(vm, "IROOT2N", out);
    var_set_num(vm, "FLOORSQRTN", out);
    var_set_num(vm, "ISQRTN_X", x);
    var_set_num(vm, "ISQRTN_OK", bad ? 0L : 1L);
    var_set_num(vm, "OK", 1);
    var_set_str(vm, "LAST", nbuf);
    var_set_str(vm, "FLAG", nbuf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", nbuf);
    if (vm->trace)
      fprintf(vm->trace, "# isqrtn %ld -> %ld bad=%d\n", x, out, bad);
    bump(vm); return 1;
  }


  /* ICBRTN|CBRTN|IROOT3N|FLOORCBRTN x — floor integer cube root → LAST_N.
   * x may be negative (odd root preserves sign). Binary-search |x| then re-sign.
   * Twin of ISQRTN. Usability: edge = ICBRTN volume after GETFLAGN. */
  if (kw(&L->cur,"ICBRTN") || kw(&L->cur,"CBRTN") || kw(&L->cur,"IROOT3N") ||
      kw(&L->cur,"FLOORCBRTN") || kw(&L->cur,"INTCBRTN") || kw(&L->cur,"ROOT3N") ||
      kw(&L->cur,"ICBRT_N") || kw(&L->cur,"CUBE_ROOT_N")){
    long x = 0, out = 0, ax = 0;
    int bad = 0, neg = 0;
    char nbuf[32];
    lex_next(L);
    if (L->cur.kind == TK_NUM) { x = L->cur.num; lex_next(L); }
    else if (L->cur.kind == TK_MINUS) {
      lex_next(L);
      if (L->cur.kind != TK_NUM) { var_set_num(vm,"OK",0); bump(vm); return 1; }
      x = -L->cur.num; lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *v = var_get(vm, L->cur.text, 0);
      if (!v) { fail_at(vm, L, "ICBRTN x — unknown x"); return -1; }
      x = v->val; lex_next(L);
    } else {
      fail_at(vm, L, "ICBRTN x — ICBRTN volume");
      return -1;
    }
    if (x == 0) {
      out = 0;
    } else {
      if (x < 0) { neg = 1; ax = -x; }
      else ax = x;
      /* binary search floor cbrt; hi capped so mid^3 stays in signed 64 (~2^63-1) */
      {
        long lo = 1, hi = ax;
        if (hi > 2097151L) hi = 2097151L; /* 2097151^3 < 2^63-1 < 2097152^3 */
        out = 1;
        while (lo <= hi) {
          long mid = lo + (hi - lo) / 2;
          unsigned long long m = (unsigned long long)mid;
          unsigned long long cu = m * m * m;
          if (cu == (unsigned long long)ax) { out = mid; break; }
          if (cu < (unsigned long long)ax) { out = mid; lo = mid + 1; }
          else hi = mid - 1;
        }
      }
      if (neg) out = -out;
    }
    snprintf(nbuf, sizeof nbuf, "%ld", out);
    var_set_num(vm, "LAST_N", out);
    vm->last_n = out;
    var_set_num(vm, "ICBRTN", out);
    var_set_num(vm, "CBRTN", out);
    var_set_num(vm, "IROOT3N", out);
    var_set_num(vm, "FLOORCBRTN", out);
    var_set_num(vm, "ICBRTN_X", x);
    var_set_num(vm, "ICBRTN_OK", bad ? 0L : 1L);
    var_set_num(vm, "OK", 1);
    var_set_str(vm, "LAST", nbuf);
    var_set_str(vm, "FLAG", nbuf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", nbuf);
    if (vm->trace)
      fprintf(vm->trace, "# icbrtn %ld -> %ld bad=%d\n", x, out, bad);
    bump(vm); return 1;
  }


  /* NEXTPOW2N|CEILPOW2N|SNEXTPOW2N x — smallest power of 2 ≥ x → LAST_N.
   * PREVPOW2N|FLOORPOW2N|SPREVPOW2N x — largest power of 2 ≤ x → LAST_N (0 if x≤0).
   * ISPOW2N|ISPOWER2N|POW2PN x — 1 if x is power of two (>0), else 0.
   * x≤0: NEXT soft 0+ERR (except ISPOW2→0 ok); PREV→0; ISPOW2→0.
   * Twin of stack SCEILPOW2. Usability: pagesz = NEXTPOW2N nbytes after GETFLAGN. */
  if (kw(&L->cur,"NEXTPOW2N") || kw(&L->cur,"CEILPOW2N") || kw(&L->cur,"SNEXTPOW2N") ||
      kw(&L->cur,"NEXT_POW2_N") || kw(&L->cur,"CEIL_POW2_N") || kw(&L->cur,"PO2CEILN") ||
      kw(&L->cur,"PREVPOW2N") || kw(&L->cur,"FLOORPOW2N") || kw(&L->cur,"SPREVPOW2N") ||
      kw(&L->cur,"PREV_POW2_N") || kw(&L->cur,"FLOOR_POW2_N") || kw(&L->cur,"PO2FLOORN") ||
      kw(&L->cur,"ISPOW2N") || kw(&L->cur,"ISPOWER2N") || kw(&L->cur,"POW2PN") ||
      kw(&L->cur,"IS_POW2_N") || kw(&L->cur,"POWEROF2N") || kw(&L->cur,"ISPO2N")){
    long x = 0, out = 0;
    int bad = 0, mode = 0; /* 0=next 1=prev 2=is */
    char nbuf[32];
    if (kw(&L->cur,"PREVPOW2N") || kw(&L->cur,"FLOORPOW2N") || kw(&L->cur,"SPREVPOW2N") ||
        kw(&L->cur,"PREV_POW2_N") || kw(&L->cur,"FLOOR_POW2_N") || kw(&L->cur,"PO2FLOORN"))
      mode = 1;
    else if (kw(&L->cur,"ISPOW2N") || kw(&L->cur,"ISPOWER2N") || kw(&L->cur,"POW2PN") ||
             kw(&L->cur,"IS_POW2_N") || kw(&L->cur,"POWEROF2N") || kw(&L->cur,"ISPO2N"))
      mode = 2;
    lex_next(L);
    if (L->cur.kind == TK_NUM) { x = L->cur.num; lex_next(L); }
    else if (L->cur.kind == TK_MINUS) {
      lex_next(L);
      if (L->cur.kind != TK_NUM) { var_set_num(vm,"OK",0); bump(vm); return 1; }
      x = -L->cur.num; lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *v = var_get(vm, L->cur.text, 0);
      if (!v) {
        fail_at(vm, L, mode==2 ? "ISPOW2N x — unknown x" :
                        mode==1 ? "PREVPOW2N x — unknown x" : "NEXTPOW2N x — unknown x");
        return -1;
      }
      x = v->val; lex_next(L);
    } else {
      fail_at(vm, L, mode==2 ? "ISPOW2N x — ISPOW2N n" :
                      mode==1 ? "PREVPOW2N x — PREVPOW2N n" : "NEXTPOW2N x — NEXTPOW2N n");
      return -1;
    }
    if (mode == 2) {
      /* ISPOW2N: power of two iff x>0 and single bit set */
      out = (x > 0 && ((unsigned long)x & ((unsigned long)x - 1ul)) == 0ul) ? 1L : 0L;
    } else if (x <= 0) {
      out = 0;
      if (mode == 0) {
        bad = 1;
        var_set_str(vm, "LAST_ERR", "NEXTPOW2N: non-positive");
        var_set_str(vm, "ERR", "NEXTPOW2N: non-positive");
      }
    } else if (mode == 1) {
      /* PREVPOW2N: largest pow2 ≤ x */
      unsigned long u = (unsigned long)x;
      if ((u & (u - 1ul)) == 0ul) out = x;
      else {
        /* floor: smear bits then clear low */
        u |= u >> 1; u |= u >> 2; u |= u >> 4; u |= u >> 8;
        u |= u >> 16;
#if ULONG_MAX > 0xfffffffful
        u |= u >> 32;
#endif
        out = (long)((u - (u >> 1)));
      }
    } else {
      /* NEXTPOW2N: smallest pow2 ≥ x */
      unsigned long u = (unsigned long)x;
      if ((u & (u - 1ul)) == 0ul) out = x;
      else if (x > (1L << 62)) {
        out = 0; bad = 1;
        var_set_str(vm, "LAST_ERR", "NEXTPOW2N: overflow");
        var_set_str(vm, "ERR", "NEXTPOW2N: overflow");
      } else {
        u--; /* prepare for smear-ceil */
        u |= u >> 1; u |= u >> 2; u |= u >> 4; u |= u >> 8;
        u |= u >> 16;
#if ULONG_MAX > 0xfffffffful
        u |= u >> 32;
#endif
        u++;
        out = (long)u;
      }
    }
    snprintf(nbuf, sizeof nbuf, "%ld", out);
    var_set_num(vm, "LAST_N", out);
    vm->last_n = out;
    if (mode == 2) {
      var_set_num(vm, "ISPOW2N", out);
      var_set_num(vm, "ISPOWER2N", out);
      var_set_num(vm, "POW2PN", out);
      var_set_num(vm, "ISPOW2N_X", x);
      var_set_num(vm, "ISPOW2N_OK", 1L);
    } else if (mode == 1) {
      var_set_num(vm, "PREVPOW2N", out);
      var_set_num(vm, "FLOORPOW2N", out);
      var_set_num(vm, "SPREVPOW2N", out);
      var_set_num(vm, "PREVPOW2N_X", x);
      var_set_num(vm, "PREVPOW2N_OK", bad ? 0L : 1L);
    } else {
      var_set_num(vm, "NEXTPOW2N", out);
      var_set_num(vm, "CEILPOW2N", out);
      var_set_num(vm, "SNEXTPOW2N", out);
      var_set_num(vm, "NEXTPOW2N_X", x);
      var_set_num(vm, "NEXTPOW2N_OK", bad ? 0L : 1L);
    }
    var_set_num(vm, "OK", 1);
    var_set_str(vm, "LAST", nbuf);
    var_set_str(vm, "FLAG", nbuf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", nbuf);
    if (vm->trace)
      fprintf(vm->trace, "# %s %ld -> %ld bad=%d\n",
              mode==2?"ispow2n":(mode==1?"prevpow2n":"nextpow2n"), x, out, bad);
    bump(vm); return 1;
  }


  /* ILOG2N|FLOORLOG2N|LOG2N|INTLOG2N x — floor(log2(x)) → LAST_N.
   * BITWIDTHN|NBITSN|BITLENN|WIDTHN x — bit length = floor(log2(x))+1 (0→0) → LAST_N.
   * x≤0: ILOG2 soft 0+ERR (ILOG2N_OK=0); BITWIDTH 0 ok for x==0, soft fail x<0.
   * Twin of stack SILOG2. Usability: shift = ILOG2N pagesz after NEXTPOW2N; width = BITWIDTHN n after GETFLAGN. */
  if (kw(&L->cur,"ILOG2N") || kw(&L->cur,"FLOORLOG2N") || kw(&L->cur,"LOG2N") ||
      kw(&L->cur,"INTLOG2N") || kw(&L->cur,"FLOOR_LOG2_N") || kw(&L->cur,"ILOG2_N") ||
      kw(&L->cur,"BITWIDTHN") || kw(&L->cur,"NBITSN") || kw(&L->cur,"BITLENN") ||
      kw(&L->cur,"WIDTHN") || kw(&L->cur,"BIT_WIDTH_N") || kw(&L->cur,"BITWIDTH_N") ||
      kw(&L->cur,"NBITS_N") || kw(&L->cur,"BITLEN_N")){
    long x = 0, out = 0;
    int bad = 0, mode = 0; /* 0=ilog2 1=bitwidth */
    char nbuf[32];
    if (kw(&L->cur,"BITWIDTHN") || kw(&L->cur,"NBITSN") || kw(&L->cur,"BITLENN") ||
        kw(&L->cur,"WIDTHN") || kw(&L->cur,"BIT_WIDTH_N") || kw(&L->cur,"BITWIDTH_N") ||
        kw(&L->cur,"NBITS_N") || kw(&L->cur,"BITLEN_N"))
      mode = 1;
    lex_next(L);
    if (L->cur.kind == TK_NUM) { x = L->cur.num; lex_next(L); }
    else if (L->cur.kind == TK_MINUS) {
      lex_next(L);
      if (L->cur.kind != TK_NUM) { var_set_num(vm,"OK",0); bump(vm); return 1; }
      x = -L->cur.num; lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *v = var_get(vm, L->cur.text, 0);
      if (!v) {
        fail_at(vm, L, mode==1 ? "BITWIDTHN x — unknown x" : "ILOG2N x — unknown x");
        return -1;
      }
      x = v->val; lex_next(L);
    } else {
      fail_at(vm, L, mode==1 ? "BITWIDTHN x — BITWIDTHN n" : "ILOG2N x — ILOG2N n");
      return -1;
    }
    if (mode == 1) {
      if (x < 0) {
        out = 0; bad = 1;
        var_set_str(vm, "LAST_ERR", "BITWIDTHN: negative");
        var_set_str(vm, "ERR", "BITWIDTHN: negative");
      } else if (x == 0) {
        out = 0;
      } else {
        unsigned long u = (unsigned long)x;
        int w = 0;
        while (u){ w++; u >>= 1; }
        out = (long)w;
      }
    } else {
      if (x <= 0) {
        out = 0; bad = 1;
        var_set_str(vm, "LAST_ERR", "ILOG2N: non-positive");
        var_set_str(vm, "ERR", "ILOG2N: non-positive");
      } else {
        unsigned long u = (unsigned long)x;
        int r = -1;
        while (u){ r++; u >>= 1; }
        out = (long)r;
      }
    }
    snprintf(nbuf, sizeof nbuf, "%ld", out);
    var_set_num(vm, "LAST_N", out);
    vm->last_n = out;
    if (mode == 1) {
      var_set_num(vm, "BITWIDTHN", out);
      var_set_num(vm, "NBITSN", out);
      var_set_num(vm, "BITLENN", out);
      var_set_num(vm, "WIDTHN", out);
      var_set_num(vm, "BITWIDTHN_X", x);
      var_set_num(vm, "BITWIDTHN_OK", bad ? 0L : 1L);
    } else {
      var_set_num(vm, "ILOG2N", out);
      var_set_num(vm, "FLOORLOG2N", out);
      var_set_num(vm, "LOG2N", out);
      var_set_num(vm, "INTLOG2N", out);
      var_set_num(vm, "ILOG2N_X", x);
      var_set_num(vm, "ILOG2N_OK", bad ? 0L : 1L);
    }
    var_set_num(vm, "OK", 1);
    var_set_str(vm, "LAST", nbuf);
    var_set_str(vm, "FLAG", nbuf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", nbuf);
    if (vm->trace)
      fprintf(vm->trace, "# %s %ld -> %ld bad=%d\n",
              mode==1?"bitwidthn":"ilog2n", x, out, bad);
    bump(vm); return 1;
  }


  /* POPCOUNTN|BITCOUNTN|PCNTN x — Hamming weight of 64-bit pattern → LAST_N.
   * CLZN|NLZN|LZCNTN x — leading zero count in 64-bit (0→64).
   * CTZN|NTZN|TZCNTN x — trailing zero count in 64-bit (0→64).
   * PARITYN|XORREDN|PARN x — xor-reduce of bits (popcount & 1).
   * Twin of stack SPOPCNT/SCLZ/SCTZ. Usability: bits = POPCOUNTN flags after GETFLAGN. */
  if (kw(&L->cur,"POPCOUNTN") || kw(&L->cur,"BITCOUNTN") || kw(&L->cur,"PCNTN") ||
      kw(&L->cur,"POPCNTN") || kw(&L->cur,"HAMMINGN") || kw(&L->cur,"ONESN") ||
      kw(&L->cur,"CLZN") || kw(&L->cur,"NLZN") || kw(&L->cur,"LZCNTN") ||
      kw(&L->cur,"LEADINGZEROSN") || kw(&L->cur,"COUNTLZN") || kw(&L->cur,"CLZ_N") ||
      kw(&L->cur,"CTZN") || kw(&L->cur,"NTZN") || kw(&L->cur,"TZCNTN") ||
      kw(&L->cur,"TRAILINGZEROSN") || kw(&L->cur,"COUNTTZN") || kw(&L->cur,"CTZ_N") ||
      kw(&L->cur,"PARITYN") || kw(&L->cur,"XORREDN") || kw(&L->cur,"PARN") ||
      kw(&L->cur,"PARITY_N") || kw(&L->cur,"XORREDUCEN") || kw(&L->cur,"BITPARITYN")){
    long x = 0, out = 0;
    int mode = 0; /* 0=pop 1=clz 2=ctz 3=parity */
    char nbuf[32];
    if (kw(&L->cur,"CLZN") || kw(&L->cur,"NLZN") || kw(&L->cur,"LZCNTN") ||
        kw(&L->cur,"LEADINGZEROSN") || kw(&L->cur,"COUNTLZN") || kw(&L->cur,"CLZ_N"))
      mode = 1;
    else if (kw(&L->cur,"CTZN") || kw(&L->cur,"NTZN") || kw(&L->cur,"TZCNTN") ||
             kw(&L->cur,"TRAILINGZEROSN") || kw(&L->cur,"COUNTTZN") || kw(&L->cur,"CTZ_N"))
      mode = 2;
    else if (kw(&L->cur,"PARITYN") || kw(&L->cur,"XORREDN") || kw(&L->cur,"PARN") ||
             kw(&L->cur,"PARITY_N") || kw(&L->cur,"XORREDUCEN") || kw(&L->cur,"BITPARITYN"))
      mode = 3;
    lex_next(L);
    if (L->cur.kind == TK_NUM) { x = L->cur.num; lex_next(L); }
    else if (L->cur.kind == TK_MINUS) {
      lex_next(L);
      if (L->cur.kind != TK_NUM) { var_set_num(vm,"OK",0); bump(vm); return 1; }
      x = -L->cur.num; lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *v = var_get(vm, L->cur.text, 0);
      if (!v) {
        fail_at(vm, L, mode==1 ? "CLZN x — unknown x" :
                        mode==2 ? "CTZN x — unknown x" :
                        mode==3 ? "PARITYN x — unknown x" : "POPCOUNTN x — unknown x");
        return -1;
      }
      x = v->val; lex_next(L);
    } else {
      fail_at(vm, L, mode==1 ? "CLZN x — CLZN n" :
                      mode==2 ? "CTZN x — CTZN n" :
                      mode==3 ? "PARITYN x — PARITYN n" : "POPCOUNTN x — POPCOUNTN flags");
      return -1;
    }
    {
      unsigned long u = (unsigned long)x;
      if (mode == 0) {
        out = 0;
        while (u) { out += (long)(u & 1ul); u >>= 1; }
      } else if (mode == 1) {
        if (u == 0) out = 64;
        else {
          out = 0;
          for (int i = 63; i >= 0; i--) {
            if (u & (1ul << (unsigned)i)) break;
            out++;
          }
        }
      } else if (mode == 2) {
        if (u == 0) out = 64;
        else {
          out = 0;
          while ((u & 1ul) == 0) { out++; u >>= 1; }
        }
      } else {
        out = 0;
        while (u) { out ^= (long)(u & 1ul); u >>= 1; }
      }
    }
    snprintf(nbuf, sizeof nbuf, "%ld", out);
    var_set_num(vm, "LAST_N", out);
    vm->last_n = out;
    if (mode == 0) {
      var_set_num(vm, "POPCOUNTN", out);
      var_set_num(vm, "BITCOUNTN", out);
      var_set_num(vm, "PCNTN", out);
      var_set_num(vm, "POPCNTN", out);
      var_set_num(vm, "POPCOUNTN_X", x);
      var_set_num(vm, "POPCOUNTN_OK", 1L);
    } else if (mode == 1) {
      var_set_num(vm, "CLZN", out);
      var_set_num(vm, "NLZN", out);
      var_set_num(vm, "LZCNTN", out);
      var_set_num(vm, "CLZN_X", x);
      var_set_num(vm, "CLZN_OK", 1L);
    } else if (mode == 2) {
      var_set_num(vm, "CTZN", out);
      var_set_num(vm, "NTZN", out);
      var_set_num(vm, "TZCNTN", out);
      var_set_num(vm, "CTZN_X", x);
      var_set_num(vm, "CTZN_OK", 1L);
    } else {
      var_set_num(vm, "PARITYN", out);
      var_set_num(vm, "XORREDN", out);
      var_set_num(vm, "PARN", out);
      var_set_num(vm, "PARITYN_X", x);
      var_set_num(vm, "PARITYN_OK", 1L);
    }
    var_set_num(vm, "OK", 1);
    var_set_str(vm, "LAST", nbuf);
    var_set_str(vm, "FLAG", nbuf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", nbuf);
    if (vm->trace)
      fprintf(vm->trace, "# %s %ld -> %ld\n",
              mode==1?"clzn":mode==2?"ctzn":mode==3?"parityn":"popcountn", x, out);
    bump(vm); return 1;
  }



  /* BITREVN|REVBITSN|BITREVERSEN|BREVN x — reverse all 64 bits of pattern → LAST_N.
   * Twin of stack SBITREV (32-bit) / SBREVN (low-n). Full-word usability dual of POPCOUNTN plane.
   * Usability: mirrored = BITREVN flags after GETFLAGN; FFT bit-reverse index = BITREVN i after ILOG2N n.
   * Aliases: RBITN REV64N BITREV64N REVERBN BITFLIPORDN (order reverse, not complement). */
  if (kw(&L->cur,"BITREVN") || kw(&L->cur,"REVBITSN") || kw(&L->cur,"BITREVERSEN") ||
      kw(&L->cur,"BREVN") || kw(&L->cur,"RBITN") || kw(&L->cur,"REV64N") ||
      kw(&L->cur,"BITFLIPORDN") || kw(&L->cur,"REVERBN") || kw(&L->cur,"BITREV64N")){
    long x = 0;
    char nbuf[32];
    lex_next(L);
    if (L->cur.kind == TK_NUM) { x = L->cur.num; lex_next(L); }
    else if (L->cur.kind == TK_MINUS) {
      lex_next(L);
      if (L->cur.kind != TK_NUM) { var_set_num(vm,"OK",0); bump(vm); return 1; }
      x = -L->cur.num; lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *v = var_get(vm, L->cur.text, 0);
      if (!v) {
        fail_at(vm, L, "BITREVN x — unknown x");
        return -1;
      }
      x = v->val; lex_next(L);
    } else {
      fail_at(vm, L, "BITREVN x — BITREVN n");
      return -1;
    }
    {
      unsigned long u = (unsigned long)x;
      unsigned long ra = 0;
      for (int i = 0; i < 64; i++){
        ra = (ra << 1) | (u & 1ul);
        u >>= 1;
      }
      x = (long)ra;
    }
    snprintf(nbuf, sizeof nbuf, "%ld", x);
    var_set_num(vm, "LAST_N", x);
    vm->last_n = x;
    var_set_num(vm, "BITREVN", x);
    var_set_num(vm, "REVBITSN", x);
    var_set_num(vm, "BREVN", x);
    var_set_num(vm, "BITREVN_OK", 1L);
    var_set_num(vm, "OK", 1);
    var_set_str(vm, "LAST", nbuf);
    var_set_str(vm, "FLAG", nbuf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", nbuf);
    if (vm->trace)
      fprintf(vm->trace, "# bitrevn -> %ld\n", x);
    bump(vm); return 1;
  }
  return 0;
}
