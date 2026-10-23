/* CubalC lang — lang_ops_sys_numagg.c
 * SYS host numeric residual: MIN/MAX · ARGMAX · IN · CMP · GCD · POW · SUM · RANGE (A2 peel).
 * Mesh vote B_core_sys · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_sys_numagg.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

int cubalc_lang_ops_sys_try_numagg(VM *vm, Lex *L) {
  /* SYS host numeric aggregate residual. Caller consumed SYS|HOST.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
    /* SYS MIN|MAX a b [c…]|bag — host-plane min/max of integer args → LAST_N.
     * Bag mode (like SUM): one string/LAST with newline numeric fields.
     * SYS CLAMP x lo hi — bound x into [lo,hi] (lo/hi swapped if inverted).
     * COUNT = n used in bag mode. Distinct from cube ISA stack MIN/MAX/CLAMP.
     * Usability: cap retries/jitter; after LENALL find max field width without
     * SORTN+TAIL glue. */
    if (kw(&L->cur,"MIN") || kw(&L->cur,"MINIMUM") || kw(&L->cur,"MAX") ||
        kw(&L->cur,"MAXIMUM") || kw(&L->cur,"CLAMP") || kw(&L->cur,"BOUND") ||
        kw(&L->cur,"CLIP") || kw(&L->cur,"SATURATE") ||
        kw(&L->cur,"MINBAG") || kw(&L->cur,"MAXBAG") || kw(&L->cur,"BAGMIN") ||
        kw(&L->cur,"BAGMAX") || kw(&L->cur,"MINALL") || kw(&L->cur,"MAXALL")){
      char op[16];
      long vals[64];
      int n = 0, i, is_min, is_max, is_clamp, bag = 0;
      long out = 0;
      char buf[40];
      char src[CUBALC_HOST_STR_MAX];
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_min = (strcmp(op, "MIN") == 0 || strcmp(op, "MINIMUM") == 0 ||
                strcmp(op, "MINBAG") == 0 || strcmp(op, "BAGMIN") == 0 ||
                strcmp(op, "MINALL") == 0);
      is_max = (strcmp(op, "MAX") == 0 || strcmp(op, "MAXIMUM") == 0 ||
                strcmp(op, "MAXBAG") == 0 || strcmp(op, "BAGMAX") == 0 ||
                strcmp(op, "MAXALL") == 0);
      is_clamp = (strcmp(op, "CLAMP") == 0 || strcmp(op, "BOUND") == 0 ||
                  strcmp(op, "CLIP") == 0 || strcmp(op, "SATURATE") == 0);
      /* force bag mode for *BAG / *ALL aliases */
      if (strcmp(op, "MINBAG") == 0 || strcmp(op, "MAXBAG") == 0 ||
          strcmp(op, "BAGMIN") == 0 || strcmp(op, "BAGMAX") == 0 ||
          strcmp(op, "MINALL") == 0 || strcmp(op, "MAXALL") == 0)
        bag = 1;
      lex_next(L);
      if (is_clamp) {
        /* parse_prim: spaces between args must not become binary minus. */
        while (n < 16 && (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT ||
                          L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS)) {
          vals[n++] = parse_prim(vm, L);
        }
        {
          long x, lo, hi;
          if (n < 1) { x = vm->last_n; lo = 0; hi = 0; }
          else if (n == 1) { x = vals[0]; lo = 0; hi = vals[0]; }
          else if (n == 2) { x = vals[0]; lo = vals[1]; hi = vals[1]; }
          else { x = vals[0]; lo = vals[1]; hi = vals[2]; }
          if (lo > hi) { long t = lo; lo = hi; hi = t; }
          out = x;
          if (out < lo) out = lo;
          if (out > hi) out = hi;
        }
      } else {
        /* bag mode: string literal, string var, or no numeric-looking arg (→ LAST) */
        if (!bag) {
          if (L->cur.kind == TK_STR) {
            bag = 1;
          } else if (L->cur.kind == TK_IDENT) {
            Var *v = var_get(vm, L->cur.text, 0);
            if (v && v->is_str) bag = 1;
          } else if (!(L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
                       L->cur.kind == TK_LPAREN)) {
            bag = 1; /* bare: aggregate LAST bag/text */
          }
        }
        if (bag) {
          src[0] = 0;
          if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
            if (resolve_str_arg(vm, L, src, sizeof src) != 0)
              snprintf(src, sizeof src, "%s", vm->last_str);
          } else {
            snprintf(src, sizeof src, "%s", vm->last_str);
          }
          {
            const char *p = src;
            while (*p && n < 64) {
              const char *start = p;
              char *end = NULL;
              long v;
              while (*p && *p != '\n') p++;
              if (start == p) {
                if (*p == '\n') p++;
                continue;
              }
              {
                char tmp[48];
                size_t flen = (size_t)(p - start);
                if (flen >= sizeof tmp) flen = sizeof tmp - 1;
                memcpy(tmp, start, flen);
                tmp[flen] = 0;
                v = strtol(tmp, &end, 10);
                if (end != tmp) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0)
                    vals[n++] = v;
                }
              }
              if (*p == '\n') p++;
            }
          }
        } else {
          /* parse_prim (not parse_expr): SYS MIN -3 -1 → [-3,-1] not (-3 - 1). */
          while (n < 64 && (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT ||
                            L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS)) {
            vals[n++] = parse_prim(vm, L);
          }
        }
        if (n <= 0) {
          out = bag ? 0 : vm->last_n;
        } else {
          out = vals[0];
          for (i = 1; i < n; i++) {
            if (is_min && vals[i] < out) out = vals[i];
            if (is_max && vals[i] > out) out = vals[i];
          }
        }
        var_set_num(vm, "COUNT", (long)n);
      }
      vm->last_n = out;
      var_set_num(vm, "LAST_N", out);
      if (is_min) var_set_num(vm, "MIN", out);
      if (is_max) var_set_num(vm, "MAX", out);
      if (is_clamp) var_set_num(vm, "CLAMP", out);
      var_set_num(vm, "OK", 1);
      snprintf(buf, sizeof buf, "%ld", out);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS ARGMAX|ARGMIN a b [c…]|bag — 0-based index of first extreme → LAST_N.
     * LAST = extreme value as decimal; ARGMAX_V/ARGMIN_V = value; COUNT = n.
     * Empty / no numbers → LAST_N=-1, OK=0. Bag mode like MIN/MAX/SUM.
     * Usability: LENALL → ARGMAX → NTH to pick longest field without EACH. */
    if (kw(&L->cur,"ARGMAX") || kw(&L->cur,"ARGMIN") || kw(&L->cur,"MAXIDX") ||
        kw(&L->cur,"MINIDX") || kw(&L->cur,"WHICHMAX") || kw(&L->cur,"WHICHMIN") ||
        kw(&L->cur,"IMAX") || kw(&L->cur,"IMIN") || kw(&L->cur,"INDEXMAX") ||
        kw(&L->cur,"INDEXMIN") || kw(&L->cur,"MAXI") || kw(&L->cur,"MINI")){
      char op[20];
      long vals[64];
      int n = 0, i, is_min, bag = 0;
      long best_i = -1, best_v = 0;
      char buf[40];
      char src[CUBALC_HOST_STR_MAX];
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_min = (strcmp(op, "ARGMIN") == 0 || strcmp(op, "MINIDX") == 0 ||
                strcmp(op, "WHICHMIN") == 0 || strcmp(op, "IMIN") == 0 ||
                strcmp(op, "INDEXMIN") == 0 || strcmp(op, "MINI") == 0);
      lex_next(L);
      if (L->cur.kind == TK_STR) {
        bag = 1;
      } else if (L->cur.kind == TK_IDENT) {
        Var *v = var_get(vm, L->cur.text, 0);
        if (v && v->is_str) bag = 1;
      } else if (!(L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
                   L->cur.kind == TK_LPAREN)) {
        bag = 1;
      }
      if (bag) {
        src[0] = 0;
        if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          if (resolve_str_arg(vm, L, src, sizeof src) != 0)
            snprintf(src, sizeof src, "%s", vm->last_str);
        } else {
          snprintf(src, sizeof src, "%s", vm->last_str);
        }
        {
          const char *p = src;
          while (*p && n < 64) {
            const char *start = p;
            char *end = NULL;
            long v;
            while (*p && *p != '\n') p++;
            if (start == p) {
              if (*p == '\n') p++;
              continue;
            }
            {
              char tmp[48];
              size_t flen = (size_t)(p - start);
              if (flen >= sizeof tmp) flen = sizeof tmp - 1;
              memcpy(tmp, start, flen);
              tmp[flen] = 0;
              v = strtol(tmp, &end, 10);
              if (end != tmp) {
                while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                  end++;
                if (end && *end == 0)
                  vals[n++] = v;
              }
            }
            if (*p == '\n') p++;
          }
        }
      } else {
        while (n < 64 && (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
                          L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)) {
          vals[n++] = parse_prim(vm, L);
        }
      }
      if (n <= 0) {
        best_i = -1;
        best_v = 0;
        var_set_num(vm, "OK", 0);
      } else {
        best_i = 0;
        best_v = vals[0];
        for (i = 1; i < n; i++) {
          if (is_min) {
            if (vals[i] < best_v) { best_v = vals[i]; best_i = i; }
          } else {
            if (vals[i] > best_v) { best_v = vals[i]; best_i = i; }
          }
        }
        var_set_num(vm, "OK", 1);
      }
      vm->last_n = best_i;
      var_set_num(vm, "LAST_N", best_i);
      if (is_min) {
        var_set_num(vm, "ARGMIN_I", best_i);
        var_set_num(vm, "ARGMIN_V", best_v);
        var_set_num(vm, "MINIDX", best_i);
      } else {
        var_set_num(vm, "ARGMAX_I", best_i);
        var_set_num(vm, "ARGMAX_V", best_v);
        var_set_num(vm, "MAXIDX", best_i);
      }
      var_set_num(vm, "COUNT", (long)n);
      snprintf(buf, sizeof buf, "%ld", best_v);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS IN|WITHIN|INRANGE x lo hi — inclusive numeric range membership → LAST_N 0|1.
     * lo/hi swapped if inverted. Distinct from SYS BETWEEN (string peel).
     * Usability: IF/guard retries and score bands without dual CMP tests. */
    if (kw(&L->cur,"IN") || kw(&L->cur,"WITHIN") || kw(&L->cur,"INRANGE") ||
        kw(&L->cur,"BETWEENN") || kw(&L->cur,"NUMIN") || kw(&L->cur,"ISIN") ||
        kw(&L->cur,"INCL") || kw(&L->cur,"BOUNDS")){
      long vals[8];
      int n = 0;
      long x, lo, hi, out;
      char buf[16];
      lex_next(L);
      while (n < 8 && (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
                        L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)) {
        vals[n++] = parse_prim(vm, L);
      }
      if (n < 1) { x = vm->last_n; lo = 0; hi = 0; }
      else if (n == 1) { x = vals[0]; lo = 0; hi = 0; }
      else if (n == 2) { x = vals[0]; lo = vals[1]; hi = vals[1]; }
      else { x = vals[0]; lo = vals[1]; hi = vals[2]; }
      if (lo > hi) { long t = lo; lo = hi; hi = t; }
      out = (x >= lo && x <= hi) ? 1L : 0L;
      vm->last_n = out;
      var_set_num(vm, "LAST_N", out);
      var_set_num(vm, "IN", out);
      var_set_num(vm, "OK", 1);
      snprintf(buf, sizeof buf, "%ld", out);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS CMP|NCMP a b — three-way numeric compare → LAST_N -1|0|1.
     * SYS SCMP|CMPS|STRCMP a b — lexicographic string compare → LAST_N -1|0|1.
     * SYS SCMPI|CMPSI — case-insensitive string compare.
     * Usability: sort keys / IF without dual LT+GT tests; pairs with MIN/MAX. */
    if (kw(&L->cur,"CMP") || kw(&L->cur,"NCMP") || kw(&L->cur,"ICMP") ||
        kw(&L->cur,"CMP3") || kw(&L->cur,"COMPARE") ||
        kw(&L->cur,"SCMP") || kw(&L->cur,"CMPS") || kw(&L->cur,"STRCMP") ||
        kw(&L->cur,"SCMPI") || kw(&L->cur,"CMPSI") || kw(&L->cur,"STRCMPI") ||
        kw(&L->cur,"IABS") || kw(&L->cur,"ABSVAL") || kw(&L->cur,"ABSNUM") ||
        kw(&L->cur,"NABS")){
      char op[16];
      long out = 0;
      char buf[40];
      int is_str, is_icase, is_abs;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_abs = (strcmp(op, "IABS") == 0 || strcmp(op, "ABSVAL") == 0 ||
                strcmp(op, "ABSNUM") == 0 || strcmp(op, "NABS") == 0);
      is_str = (strcmp(op, "SCMP") == 0 || strcmp(op, "CMPS") == 0 ||
                strcmp(op, "STRCMP") == 0 || strcmp(op, "SCMPI") == 0 ||
                strcmp(op, "CMPSI") == 0 || strcmp(op, "STRCMPI") == 0);
      is_icase = (strcmp(op, "SCMPI") == 0 || strcmp(op, "CMPSI") == 0 ||
                  strcmp(op, "STRCMPI") == 0);
      lex_next(L);
      if (is_abs) {
        long x = 0;
        if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
            L->cur.kind==TK_MINUS)
          x = parse_prim(vm, L);
        else
          x = vm->last_n;
        out = (x < 0) ? -x : x;
        var_set_num(vm, "ABS", out);
      } else if (is_str) {
        char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
        int r;
        a[0] = 0; b[0] = 0;
        if (!is_icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                          kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                          kw(&L->cur,"CI"))) {
          is_icase = 1;
          lex_next(L);
        }
        if (resolve_str_arg(vm, L, a, sizeof a) != 0)
          snprintf(a, sizeof a, "%s", vm->last_str);
        if (resolve_str_arg(vm, L, b, sizeof b) != 0) b[0] = 0;
        if (!is_icase) {
          r = strcmp(a, b);
        } else {
          const char *p = a, *q = b;
          r = 0;
          while (*p && *q) {
            char ca = *p, cb = *q;
            if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
            if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
            if (ca != cb) { r = (ca < cb) ? -1 : 1; break; }
            p++; q++;
          }
          if (r == 0) {
            if (*p) r = 1;
            else if (*q) r = -1;
          }
        }
        out = (r < 0) ? -1L : (r > 0 ? 1L : 0L);
        var_set_num(vm, "SCMP", out);
      } else {
        long a = 0, b = 0;
        if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
            L->cur.kind==TK_MINUS)
          a = parse_prim(vm, L);
        else
          a = vm->last_n;
        if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
            L->cur.kind==TK_MINUS)
          b = parse_prim(vm, L);
        else
          b = 0;
        out = (a < b) ? -1L : (a > b ? 1L : 0L);
        var_set_num(vm, "CMP", out);
      }
      vm->last_n = out;
      var_set_num(vm, "LAST_N", out);
      var_set_num(vm, "OK", 1);
      snprintf(buf, sizeof buf, "%ld", out);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS SIGN|SGN x — signum → LAST_N -1|0|1 (pairs with IABS/CMP).
     * SYS DIV|IDIV|QUOT a b — trunc-toward-zero integer divide; /0 → 0 soft.
     * SYS MOD|REM|REMAINDER a b — a % b; %0 → 0 soft. Not path ABS/REALPATH.
     * Usability: page index, bucket, parity without cube ISA DIV/MOD soup. */
    if (kw(&L->cur,"SIGN") || kw(&L->cur,"SGN") || kw(&L->cur,"SIGNUM") ||
        kw(&L->cur,"DIV") || kw(&L->cur,"IDIV") || kw(&L->cur,"QUOT") ||
        kw(&L->cur,"QUOTIENT") || kw(&L->cur,"INTDIV") ||
        kw(&L->cur,"MOD") || kw(&L->cur,"REM") || kw(&L->cur,"REMAINDER") ||
        kw(&L->cur,"MODULO") || kw(&L->cur,"IMOD")){
      char op[16];
      long out = 0, a = 0, b = 0;
      char buf[40];
      int is_sign, is_div, is_mod;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_sign = (strcmp(op, "SIGN") == 0 || strcmp(op, "SGN") == 0 ||
                 strcmp(op, "SIGNUM") == 0);
      is_div = (strcmp(op, "DIV") == 0 || strcmp(op, "IDIV") == 0 ||
                strcmp(op, "QUOT") == 0 || strcmp(op, "QUOTIENT") == 0 ||
                strcmp(op, "INTDIV") == 0);
      is_mod = (strcmp(op, "MOD") == 0 || strcmp(op, "REM") == 0 ||
                strcmp(op, "REMAINDER") == 0 || strcmp(op, "MODULO") == 0 ||
                strcmp(op, "IMOD") == 0);
      lex_next(L);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)
        a = parse_prim(vm, L);
      else
        a = vm->last_n;
      if (!is_sign) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
            L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)
          b = parse_prim(vm, L);
        else
          b = 0;
      }
      if (is_sign) {
        out = (a > 0) ? 1L : (a < 0 ? -1L : 0L);
        var_set_num(vm, "SIGN", out);
      } else if (is_div) {
        if (b == 0) out = 0;
        else if (a == LONG_MIN && b == -1) out = LONG_MAX; /* overflow guard */
        else out = a / b;
        var_set_num(vm, "DIV", out);
        var_set_num(vm, "QUOT", out);
      } else {
        /* mod */
        if (b == 0) out = 0;
        else if (a == LONG_MIN && b == -1) out = 0;
        else out = a % b;
        var_set_num(vm, "MOD", out);
        var_set_num(vm, "REM", out);
      }
      vm->last_n = out;
      var_set_num(vm, "LAST_N", out);
      var_set_num(vm, "OK", 1);
      snprintf(buf, sizeof buf, "%ld", out);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS GCD|HCF a b [c…] — greatest common divisor of integer args → LAST_N.
     * SYS LCM a b [c…] — least common multiple (via |a*b|/gcd, overflow-safe-ish).
     * Abs values used; GCD(0,0)=0; LCM with 0 → 0. Multi-arg reduces left-to-right.
     * Usability: reduce ratios / cycle periods without cube ISA GCD soup. */
    if (kw(&L->cur,"GCD") || kw(&L->cur,"HCF") || kw(&L->cur,"GCF") ||
        kw(&L->cur,"LCM") || kw(&L->cur,"LEASTCM") || kw(&L->cur,"LCMM")){
      char op[16];
      long vals[32];
      int n = 0, i, is_lcm;
      long out = 0;
      char buf[40];
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_lcm = (strcmp(op, "LCM") == 0 || strcmp(op, "LEASTCM") == 0 ||
                strcmp(op, "LCMM") == 0);
      lex_next(L);
      while (n < 32 && (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
                        L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)) {
        vals[n++] = parse_prim(vm, L);
      }
      if (n <= 0) {
        out = is_lcm ? 1 : 0; /* empty LCM=1, empty GCD=0 */
      } else {
        /* abs helper */
        #define CUBALC_IABS_L(x) ((x) < 0 ? -(x) : (x))
        out = CUBALC_IABS_L(vals[0]);
        for (i = 1; i < n; i++) {
          long a = out, b = CUBALC_IABS_L(vals[i]);
          if (is_lcm) {
            long g = a, h = b;
            while (h) { long t = g % h; g = h; h = t; }
            if (a == 0 || b == 0) out = 0;
            else {
              /* out = a / g * b — reduce first to limit overflow */
              out = (a / g) * b;
              if (out < 0) out = -out;
            }
          } else {
            long g = a, h = b;
            while (h) { long t = g % h; g = h; h = t; }
            out = g;
          }
        }
        #undef CUBALC_IABS_L
      }
      vm->last_n = out;
      var_set_num(vm, "LAST_N", out);
      var_set_num(vm, "OK", 1);
      if (is_lcm) {
        var_set_num(vm, "LCM", out);
      } else {
        var_set_num(vm, "GCD", out);
      }
      snprintf(buf, sizeof buf, "%ld", out);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS POW|POWER|IPOW a e — integer a^e (e>=0; e=0 → 1). Overflow soft-wraps in long.
     * SYS ISQRT|SQRT|IROOT2 n — floor integer square root (n<0 → 0).
     * Usability: backoff bases / geometry without cube ISA POW/SQRT soup. */
    if (kw(&L->cur,"POW") || kw(&L->cur,"POWER") || kw(&L->cur,"IPOW") ||
        kw(&L->cur,"EXPT") || kw(&L->cur,"EXPON") ||
        kw(&L->cur,"ISQRT") || kw(&L->cur,"SQRT") || kw(&L->cur,"IROOT2") ||
        kw(&L->cur,"SQRTR") || kw(&L->cur,"FLOORSQRT")){
      char op[16];
      long out = 0, a = 0, e = 0;
      char buf[40];
      int is_sqrt;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_sqrt = (strcmp(op, "ISQRT") == 0 || strcmp(op, "SQRT") == 0 ||
                 strcmp(op, "IROOT2") == 0 || strcmp(op, "SQRTR") == 0 ||
                 strcmp(op, "FLOORSQRT") == 0);
      lex_next(L);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)
        a = parse_prim(vm, L);
      else
        a = vm->last_n;
      if (is_sqrt) {
        if (a < 0) out = 0;
        else {
          /* binary search floor sqrt */
          long lo = 0, hi = a;
          if (hi > 3037000499L) hi = 3037000499L; /* max for hi*hi in signed 64 */
          while (lo <= hi) {
            long mid = lo + (hi - lo) / 2;
            long sq = mid * mid;
            if (sq == a) { out = mid; break; }
            if (sq < a) { out = mid; lo = mid + 1; }
            else hi = mid - 1;
          }
        }
        var_set_num(vm, "SQRT", out);
        var_set_num(vm, "ISQRT", out);
      } else {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
            L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)
          e = parse_prim(vm, L);
        else
          e = 0;
        if (e < 0) {
          out = 0; /* negative exp not supported on ints */
        } else if (e == 0) {
          out = 1;
        } else {
          long r = 1;
          long base = a;
          long ee = e;
          /* binary exponentiation; soft wrap on overflow */
          while (ee > 0) {
            if (ee & 1) r = r * base;
            ee >>= 1;
            if (ee) base = base * base;
          }
          out = r;
        }
        var_set_num(vm, "POW", out);
      }
      vm->last_n = out;
      var_set_num(vm, "LAST_N", out);
      var_set_num(vm, "OK", 1);
      snprintf(buf, sizeof buf, "%ld", out);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS SUM|TOTAL a b [c…] — sum of integer args → LAST_N.
     * SYS PROD|PRODUCT|MULALL a b [c…] — product of integers.
     * SYS AVG|MEAN|AVERAGE a b [c…] — integer mean (trunc toward 0).
     * SYS MEDIAN|P50 a b [c…]|bag — integer median (sort; even → lower mid).
     * Bag mode: one string (or bare LAST) with newline fields → aggregate numeric
     * lines; blank/non-numeric fields skipped. COUNT = n used.
     * Usability: score bags / LIST sizes without shell awk; pairs with MIN/MAX. */
    if (kw(&L->cur,"SUM") || kw(&L->cur,"TOTAL") || kw(&L->cur,"SUMALL") ||
        kw(&L->cur,"PROD") || kw(&L->cur,"PRODUCT") || kw(&L->cur,"MULALL") ||
        kw(&L->cur,"PRODUCTALL") ||
        kw(&L->cur,"AVG") || kw(&L->cur,"MEAN") || kw(&L->cur,"AVERAGE") ||
        kw(&L->cur,"AVGALL") ||
        kw(&L->cur,"MEDIAN") || kw(&L->cur,"P50") || kw(&L->cur,"MED") ||
        kw(&L->cur,"MIDVAL") || kw(&L->cur,"MEDIANINT")){
      char op[16];
      long vals[64];
      int n = 0, i, is_sum, is_prod, is_avg, is_med, bag = 0;
      long out = 0, count = 0;
      char buf[40];
      char src[CUBALC_HOST_STR_MAX];
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_sum = (strcmp(op, "SUM") == 0 || strcmp(op, "TOTAL") == 0 ||
                strcmp(op, "SUMALL") == 0);
      is_prod = (strcmp(op, "PROD") == 0 || strcmp(op, "PRODUCT") == 0 ||
                 strcmp(op, "MULALL") == 0 || strcmp(op, "PRODUCTALL") == 0);
      is_avg = (strcmp(op, "AVG") == 0 || strcmp(op, "MEAN") == 0 ||
                strcmp(op, "AVERAGE") == 0 || strcmp(op, "AVGALL") == 0);
      is_med = (strcmp(op, "MEDIAN") == 0 || strcmp(op, "P50") == 0 ||
                strcmp(op, "MED") == 0 || strcmp(op, "MIDVAL") == 0 ||
                strcmp(op, "MEDIANINT") == 0);
      lex_next(L);
      /* bag mode: string literal, string var, or no numeric-looking arg (→ LAST) */
      if (L->cur.kind == TK_STR) {
        bag = 1;
      } else if (L->cur.kind == TK_IDENT) {
        Var *v = var_get(vm, L->cur.text, 0);
        if (v && v->is_str) bag = 1;
      } else if (!(L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
                   L->cur.kind == TK_LPAREN)) {
        bag = 1; /* bare: aggregate LAST bag/text */
      }
      if (bag) {
        src[0] = 0;
        if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          if (resolve_str_arg(vm, L, src, sizeof src) != 0)
            snprintf(src, sizeof src, "%s", vm->last_str);
        } else {
          snprintf(src, sizeof src, "%s", vm->last_str);
        }
        {
          const char *p = src;
          while (*p && n < 64) {
            const char *start = p;
            char *end = NULL;
            long v;
            while (*p && *p != '\n') p++;
            /* empty field between newlines → skip */
            if (start == p) {
              if (*p == '\n') p++;
              continue;
            }
            /* parse field [start,p) as integer */
            {
              char tmp[48];
              size_t flen = (size_t)(p - start);
              if (flen >= sizeof tmp) flen = sizeof tmp - 1;
              memcpy(tmp, start, flen);
              tmp[flen] = 0;
              v = strtol(tmp, &end, 10);
              if (end != tmp) {
                /* allow trailing whitespace only */
                while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                  end++;
                if (end && *end == 0)
                  vals[n++] = v;
              }
            }
            if (*p == '\n') p++;
          }
        }
      } else {
        while (n < 64 && (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
                          L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)) {
          vals[n++] = parse_prim(vm, L);
        }
      }
      count = n;
      if (is_med) {
        /* insertion sort then middle (even n → lower mid, no float) */
        out = 0;
        if (n > 0) {
          for (i = 1; i < n; i++) {
            long key = vals[i];
            int j = i - 1;
            while (j >= 0 && vals[j] > key) {
              vals[j + 1] = vals[j];
              j--;
            }
            vals[j + 1] = key;
          }
          out = vals[(n - 1) / 2];
        }
      } else if (is_prod) {
        out = 1;
        for (i = 0; i < n; i++) out *= vals[i];
        if (n == 0) out = 1; /* empty product */
      } else if (is_avg) {
        out = 0;
        if (n > 0) {
          long s = 0;
          for (i = 0; i < n; i++) s += vals[i];
          out = s / n;
        }
      } else {
        /* sum / total */
        out = 0;
        for (i = 0; i < n; i++) out += vals[i];
      }
      vm->last_n = out;
      var_set_num(vm, "LAST_N", out);
      var_set_num(vm, "COUNT", count);
      var_set_num(vm, "OK", 1);
      if (is_sum) {
        var_set_num(vm, "SUM", out);
        var_set_num(vm, "SUM_N", out);
      }
      if (is_prod) {
        var_set_num(vm, "PROD", out);
        var_set_num(vm, "PROD_N", out);
      }
      if (is_avg) {
        var_set_num(vm, "AVG", out);
        var_set_num(vm, "AVG_N", out);
      }
      if (is_med) {
        var_set_num(vm, "MEDIAN", out);
        var_set_num(vm, "MEDIAN_N", out);
      }
      snprintf(buf, sizeof buf, "%ld", out);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS RANGE lo hi [step] — inclusive integer sequence → newline bag LAST.
     * SYS SEQ n — 1..n; SYS IOTA n — 0..n-1. Cap 256 fields.
     * Usability: EACH LINE / SORTN / SUM fixtures without shell seq(1). */
    if (kw(&L->cur,"RANGE") || kw(&L->cur,"SEQ") || kw(&L->cur,"SEQUENCE") ||
        kw(&L->cur,"IOTA") || kw(&L->cur,"ENUM") || kw(&L->cur,"NUMS") ||
        kw(&L->cur,"COUNTTO") || kw(&L->cur,"FROMTO")){
      char op[16];
      long lo = 0, hi = 0, step = 1;
      long n = 0, v, vals[256];
      int i, is_seq, is_iota, empty = 0;
      char out[CUBALC_HOST_STR_MAX];
      size_t olen = 0;
      char num[32];
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_seq = (strcmp(op, "SEQ") == 0 || strcmp(op, "SEQUENCE") == 0 ||
                strcmp(op, "COUNTTO") == 0);
      is_iota = (strcmp(op, "IOTA") == 0 || strcmp(op, "ENUM") == 0);
      lex_next(L);
      /* parse up to 3 prims: a [b [c]] */
      {
        long args[3];
        int na = 0;
        while (na < 3 && (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
                          L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)) {
          args[na++] = parse_prim(vm, L);
        }
        if (is_iota) {
          /* IOTA n → 0..n-1; IOTA lo hi → lo..hi-1 half-open; IOTA lo hi step */
          if (na <= 0 || (na == 1 && args[0] <= 0)) {
            empty = 1;
          } else if (na == 1) {
            lo = 0; hi = args[0] - 1; step = 1;
          } else if (na == 2) {
            if (args[1] <= args[0]) empty = 1;
            else { lo = args[0]; hi = args[1] - 1; step = 1; }
          } else {
            if (args[1] <= args[0] && (!args[2] || args[2] > 0)) empty = 1;
            else {
              lo = args[0]; hi = args[1] - 1; step = args[2] ? args[2] : 1;
            }
          }
        } else if (is_seq) {
          /* SEQ n → 1..n; SEQ lo hi [step] inclusive like RANGE */
          if (na <= 0 || (na == 1 && args[0] <= 0)) {
            empty = 1;
          } else if (na == 1) {
            lo = 1; hi = args[0]; step = 1;
          } else if (na == 2) {
            lo = args[0]; hi = args[1]; step = 1;
          } else {
            lo = args[0]; hi = args[1]; step = args[2] ? args[2] : 1;
          }
        } else {
          /* RANGE lo hi [step] inclusive */
          if (na <= 0) {
            empty = 1;
          } else if (na == 1) {
            if (args[0] <= 0) empty = 1;
            else { lo = 1; hi = args[0]; step = 1; } /* RANGE n ≡ SEQ n */
          } else if (na == 2) {
            lo = args[0]; hi = args[1]; step = 1;
          } else {
            lo = args[0]; hi = args[1]; step = args[2] ? args[2] : 1;
          }
        }
      }
      if (!empty) {
        if (step == 0) step = 1;
        /* auto-flip step when lo/hi inverted (RANGE 5 1 → descending) */
        if (lo > hi && step > 0) step = -step;
        if (lo < hi && step < 0) step = -step;
        v = lo;
        if (step > 0) {
          while (v <= hi && n < 256) { vals[n++] = v; v += step; }
        } else {
          while (v >= hi && n < 256) { vals[n++] = v; v += step; }
        }
      }
      out[0] = 0;
      for (i = 0; i < (int)n; i++) {
        int nn = snprintf(num, sizeof num, "%ld", vals[i]);
        if (nn < 0) nn = 0;
        if (i > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        if (olen + (size_t)nn < sizeof out) {
          memcpy(out + olen, num, (size_t)nn);
          olen += (size_t)nn;
        } else if (olen < sizeof out - 1) {
          size_t take = sizeof out - 1 - olen;
          memcpy(out + olen, num, take);
          olen += take;
        }
        out[olen] = 0;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "RANGE_N", n);
      var_set_num(vm, "SEQ_N", n);
      var_set_num(vm, "COUNT", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
  return 0;
}
