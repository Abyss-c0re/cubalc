/* CubalC lang — lang_ops_sys_kvfilter.c
 * SYS bag KV filter/scale residual: THRESH/DROPZERO/KEEPKEY/PCT/CAP/SCALE/DIV/ADD/ABS/SIGN (A2 peel).
 * Mesh vote A2_theme_peel_core_sys · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_sys_kvfilter.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_sys_try_kvfilter(VM *vm, Lex *L) {
  /* SYS bag-KV filter residual. Caller consumed SYS|HOST.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
    /* SYS THRESHKV|KEEPVAL|MINCOUNT bag min [sep]
     * — keep key:val fields whose numeric value >= min (default sep ":").
     * LAST = filtered bag; LAST_N = remaining field count;
     * THRESHKV_DROP = removed count; THRESHKV_MIN = threshold used.
     * Usability: denoise FREQ (drop rare noise) without EACH+LOOKUPN rebuild. */
    if (kw(&L->cur,"THRESHKV") || kw(&L->cur,"KEEPVAL") || kw(&L->cur,"MINCOUNT") ||
        kw(&L->cur,"KEEPMIN") || kw(&L->cur,"KVTHRESH") || kw(&L->cur,"FILTERVAL") ||
        kw(&L->cur,"MINFREQ") || kw(&L->cur,"DROPRARE") || kw(&L->cur,"PRUNEKV")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t flen, sn, olen = 0;
      long minv = 1, kept = 0, drop = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      /* min threshold: number, expr, or numeric var */
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        minv = parse_expr(vm, L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *dv = var_get(vm, L->cur.text, 0);
        if (dv && !dv->is_str) {
          minv = (long)dv->val;
          lex_next(L);
        }
      }
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            int keep = 0;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *rhs = field;
              char *end = 0;
              long v;
              if (sn > 0) {
                const char *sp = strstr(field, sep);
                if (sp) rhs = sp + sn;
                else rhs = NULL;
              }
              if (rhs) {
                v = strtol(rhs, &end, 10);
                if (end != rhs) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0 && v >= minv) keep = 1;
                }
              }
            }
            if (keep) {
              size_t nlen = strlen(field);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, field, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, field, t);
                olen += t;
              }
              out[olen] = 0;
              kept++;
            } else {
              drop++;
            }
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "THRESHKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "THRESHKV_N", kept);
      var_set_num(vm, "THRESHKV_DROP", drop);
      var_set_num(vm, "THRESHKV_MIN", minv);
      var_set_num(vm, "KEEPVAL_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS DROPZERO|KEEPNZ|NZKV bag [sep]
     * — drop key:val fields whose numeric value is exactly 0; keep negatives and
     * positives. Non-numeric fields kept. Default sep ":".
     * LAST = filtered bag; LAST_N = remaining; DROPZERO_N = dropped count.
     * Usability: clean DIFFKV stable keys (delta 0) without THRESHKV (which drops
     * negatives) or EACH rebuild. */
    if (kw(&L->cur,"DROPZERO") || kw(&L->cur,"KEEPNZ") || kw(&L->cur,"NZKV") ||
        kw(&L->cur,"DROPZ") || kw(&L->cur,"DROP0") || kw(&L->cur,"FILTERZERO") ||
        kw(&L->cur,"NONZERO") || kw(&L->cur,"KEEPNONZERO") || kw(&L->cur,"PRUNEZERO")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t flen, sn, olen = 0;
      long kept = 0, drop = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            int keep = 1;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *rhs = field;
              char *end = 0;
              long v;
              if (sn > 0) {
                const char *sp = strstr(field, sep);
                if (sp) rhs = sp + sn;
                else rhs = NULL;
              }
              if (rhs) {
                v = strtol(rhs, &end, 10);
                if (end != rhs) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0 && v == 0) keep = 0;
                }
              }
            }
            if (keep) {
              size_t nlen = strlen(field);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, field, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, field, t);
                olen += t;
              }
              out[olen] = 0;
              kept++;
            } else {
              drop++;
            }
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "DROPZERO", out);
      var_set_str(vm, "KEEPNZ", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "DROPZERO_N", drop);
      var_set_num(vm, "DROPZERO_KEEP", kept);
      var_set_num(vm, "KEEPNZ_N", kept);
      var_set_num(vm, "NZKV_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS KEEPKEY|GREPKEY bag needle [sep]
     * SYS DROPKEY|GREPVKEY bag needle [sep] — invert (drop matching keys).
     * SYS KEEPKEYI|GREPKEYI — case-insensitive key match.
     * Keep (or drop) key:val fields whose key contains needle. Default sep ":".
     * Empty needle → keep all (or drop all for invert).
     * LAST = filtered bag; LAST_N = remaining; KEEPKEY_DROP = removed count.
     * Usability: filter FREQ/plate by key pattern without EACH+BEFORE+HAS. */
    if (kw(&L->cur,"KEEPKEY") || kw(&L->cur,"GREPKEY") || kw(&L->cur,"FILTERKEY") ||
        kw(&L->cur,"KEYGREP") || kw(&L->cur,"HASKEY") || kw(&L->cur,"MATCHKEY") ||
        kw(&L->cur,"DROPKEY") || kw(&L->cur,"GREPVKEY") || kw(&L->cur,"RMKEY") ||
        kw(&L->cur,"KEEPKEYI") || kw(&L->cur,"GREPKEYI") || kw(&L->cur,"DROPKEYI")){
      char op[24], bag[CUBALC_HOST_STR_MAX], needle[256], sep[32];
      char field[512], key[256], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t flen, sn, nlen, olen = 0;
      long kept = 0, drop = 0;
      int invert = 0, icase = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "DROPKEY") == 0 || strcmp(op, "GREPVKEY") == 0 ||
          strcmp(op, "RMKEY") == 0 || strcmp(op, "DROPKEYI") == 0)
        invert = 1;
      if (strcmp(op, "KEEPKEYI") == 0 || strcmp(op, "GREPKEYI") == 0 ||
          strcmp(op, "DROPKEYI") == 0)
        icase = 1;
      lex_next(L);
      bag[0] = 0; needle[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0] = 0;
      if (L->cur.kind == TK_STR) {
        /* optional sep — only if looks like sep (short) or explicit after needle */
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      nlen = strlen(needle);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            int match = 0, keep;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            key[0] = 0;
            if (sn > 0) {
              const char *sp = strstr(field, sep);
              if (sp) {
                size_t kn = (size_t)(sp - field);
                if (kn >= sizeof key) kn = sizeof key - 1;
                memcpy(key, field, kn);
                key[kn] = 0;
              } else {
                snprintf(key, sizeof key, "%s", field);
              }
            } else {
              snprintf(key, sizeof key, "%s", field);
            }
            if (nlen == 0) {
              match = 1; /* empty needle matches all */
            } else if (!icase) {
              match = (strstr(key, needle) != NULL);
            } else {
              /* ASCII case-insensitive contains */
              char kl[256], nl[256];
              size_t i, kn = strlen(key);
              if (kn >= sizeof kl) kn = sizeof kl - 1;
              for (i = 0; i < kn; i++) {
                char c = key[i];
                kl[i] = (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
              }
              kl[kn] = 0;
              for (i = 0; i < nlen && i < sizeof nl - 1; i++) {
                char c = needle[i];
                nl[i] = (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
              }
              nl[i] = 0;
              match = (strstr(kl, nl) != NULL);
            }
            keep = invert ? !match : match;
            if (keep) {
              size_t fl = strlen(field);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + fl < sizeof out) {
                memcpy(out + olen, field, fl);
                olen += fl;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, field, t);
                olen += t;
              }
              out[olen] = 0;
              kept++;
            } else {
              drop++;
            }
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "KEEPKEY", out);
      var_set_str(vm, "GREPKEY", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "KEEPKEY_N", kept);
      var_set_num(vm, "KEEPKEY_DROP", drop);
      var_set_num(vm, "GREPKEY_N", kept);
      var_set_num(vm, "DROPKEY_N", drop);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS PCTKV|PERCENTKV|SHAREKV bag [sep]
     * — rewrite each numeric key:val value as integer percent of the bag total
     * (v*100)/sum, trunc toward 0. Non-numeric fields kept as-is. Default sep ":".
     * LAST = bag; LAST_N = field count; PCTKV_SUM = original total; PCTKV_N = fields rewritten.
     * Empty/zero total → all rewritten values become 0.
     * Usability: FREQ share-of-total without EACH+SUMKV+arith+KVSET glue. */
    if (kw(&L->cur,"PCTKV") || kw(&L->cur,"PERCENTKV") || kw(&L->cur,"SHAREKV") ||
        kw(&L->cur,"KVPCT") || kw(&L->cur,"PCTFREQ") || kw(&L->cur,"FREQPCT") ||
        kw(&L->cur,"PCTDICT") || kw(&L->cur,"NORMKV") || kw(&L->cur,"KVSHARE")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[CUBALC_HOST_STR_MAX];
      char left[256], newline[768], vbuf[40];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long total = 0, kept = 0, rew = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      /* pass 1: sum numeric values */
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *rhs = field;
              char *end = 0;
              long v;
              if (sn > 0) {
                const char *sp = strstr(field, sep);
                if (sp) rhs = sp + sn;
                else rhs = NULL;
              }
              if (rhs) {
                v = strtol(rhs, &end, 10);
                if (end != rhs) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0) total += v;
                }
              }
            }
          }
          if (*p == '\n') p++;
        }
      }
      /* pass 2: rewrite */
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *sp = sn ? strstr(field, sep) : NULL;
              char *end = 0;
              long v, pct;
              int did = 0;
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
                v = strtol(sp + sn, &end, 10);
                if (end != sp + sn) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0) {
                    pct = (total != 0) ? ((v * 100) / total) : 0;
                    snprintf(vbuf, sizeof vbuf, "%ld", pct);
                    snprintf(newline, sizeof newline, "%s%s%s", left, sep, vbuf);
                    did = 1;
                    rew++;
                  }
                }
              }
              if (!did)
                snprintf(newline, sizeof newline, "%s", field);
            }
            {
              size_t nlen = strlen(newline);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, newline, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, newline, t);
                olen += t;
              }
              out[olen] = 0;
            }
            kept++;
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "PCTKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "PCTKV_N", rew);
      var_set_num(vm, "PCTKV_SUM", total);
      var_set_num(vm, "PCTKV_KEYS", kept);
      var_set_num(vm, "SHAREKV_SUM", total);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS CAPKV|CLAMPKV|MAXVAL bag max [sep]
     * — clamp each numeric key:val value to max (values > max become max).
     * Default sep ":". LAST = bag; LAST_N = field count;
     * CAPKV_N = fields capped; CAPKV_MAX = threshold used.
     * Usability: cap FREQ outliers before PCTKV/TOPKEY without EACH+LOOKUPN+KVSET. */
    if (kw(&L->cur,"CAPKV") || kw(&L->cur,"CLAMPKV") || kw(&L->cur,"MAXVAL") ||
        kw(&L->cur,"KVCAP") || kw(&L->cur,"CAPVAL") || kw(&L->cur,"CEILKV") ||
        kw(&L->cur,"LIMITKV") || kw(&L->cur,"KVCLAMP") || kw(&L->cur,"MAXCOUNT")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[CUBALC_HOST_STR_MAX];
      char left[256], newline[768], vbuf[40];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long maxv = 0, kept = 0, capped = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        maxv = parse_expr(vm, L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *dv = var_get(vm, L->cur.text, 0);
        if (dv && !dv->is_str) {
          maxv = (long)dv->val;
          lex_next(L);
        }
      }
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *sp = sn ? strstr(field, sep) : NULL;
              char *end = 0;
              long v;
              int did = 0;
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
                v = strtol(sp + sn, &end, 10);
                if (end != sp + sn) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0) {
                    if (v > maxv) {
                      v = maxv;
                      capped++;
                    }
                    snprintf(vbuf, sizeof vbuf, "%ld", v);
                    snprintf(newline, sizeof newline, "%s%s%s", left, sep, vbuf);
                    did = 1;
                  }
                }
              }
              if (!did)
                snprintf(newline, sizeof newline, "%s", field);
            }
            {
              size_t nlen = strlen(newline);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, newline, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, newline, t);
                olen += t;
              }
              out[olen] = 0;
            }
            kept++;
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "CAPKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "CAPKV_N", capped);
      var_set_num(vm, "CAPKV_MAX", maxv);
      var_set_num(vm, "CLAMPKV_N", capped);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SCALEKV|MULKV|WEIGHTKV bag factor [sep]
     * — multiply each numeric key:val value by factor (integer). Default sep ":".
     * LAST = bag; LAST_N = field count; SCALEKV_N = fields scaled;
     * SCALEKV_F = factor used.
     * Usability: weight a FREQ source before MERGEKV without EACH+LOOKUPN+KVSET. */
    if (kw(&L->cur,"SCALEKV") || kw(&L->cur,"MULKV") || kw(&L->cur,"WEIGHTKV") ||
        kw(&L->cur,"MULTKV") || kw(&L->cur,"MULVAL") || kw(&L->cur,"KVMUL") ||
        kw(&L->cur,"SCALEVAL") || kw(&L->cur,"TIMSKV") || kw(&L->cur,"FACTORKV")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[CUBALC_HOST_STR_MAX];
      char left[256], newline[768], vbuf[40];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long factor = 1, kept = 0, scaled = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        factor = parse_expr(vm, L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *dv = var_get(vm, L->cur.text, 0);
        if (dv && !dv->is_str) {
          factor = (long)dv->val;
          lex_next(L);
        }
      }
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *sp = sn ? strstr(field, sep) : NULL;
              char *end = 0;
              long v;
              int did = 0;
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
                v = strtol(sp + sn, &end, 10);
                if (end != sp + sn) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0) {
                    v = v * factor;
                    snprintf(vbuf, sizeof vbuf, "%ld", v);
                    snprintf(newline, sizeof newline, "%s%s%s", left, sep, vbuf);
                    did = 1;
                    scaled++;
                  }
                }
              }
              if (!did)
                snprintf(newline, sizeof newline, "%s", field);
            }
            {
              size_t nlen = strlen(newline);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, newline, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, newline, t);
                olen += t;
              }
              out[olen] = 0;
            }
            kept++;
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "SCALEKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "SCALEKV_N", scaled);
      var_set_num(vm, "SCALEKV_F", factor);
      var_set_num(vm, "MULKV_N", scaled);
      var_set_num(vm, "WEIGHTKV_F", factor);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS DIVKV|IDIVKV|QUOTKV bag divisor [sep]
     * — integer-divide each numeric key:val value by divisor (trunc toward 0).
     * Default sep ":". Divisor 0 → all rewritten values become 0 (soft, no fatal).
     * LAST = bag; LAST_N = field count; DIVKV_N = fields divided; DIVKV_D = divisor.
     * Usability: mean of N MERGEKV FREQ sources without EACH+LOOKUPN+DIV+KVSET. */
    if (kw(&L->cur,"DIVKV") || kw(&L->cur,"IDIVKV") || kw(&L->cur,"QUOTKV") ||
        kw(&L->cur,"KVDIV") || kw(&L->cur,"DIVVAL") || kw(&L->cur,"SCALEKVD") ||
        kw(&L->cur,"AVGSCALE") || kw(&L->cur,"MEANKVD") || kw(&L->cur,"DIVALL")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[CUBALC_HOST_STR_MAX];
      char left[256], newline[768], vbuf[40];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long div = 1, kept = 0, divided = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        div = parse_expr(vm, L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *dv = var_get(vm, L->cur.text, 0);
        if (dv && !dv->is_str) {
          div = (long)dv->val;
          lex_next(L);
        }
      }
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *sp = sn ? strstr(field, sep) : NULL;
              char *end = 0;
              long v;
              int did = 0;
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
                v = strtol(sp + sn, &end, 10);
                if (end != sp + sn) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0) {
                    if (div == 0) v = 0;
                    else v = v / div;
                    snprintf(vbuf, sizeof vbuf, "%ld", v);
                    snprintf(newline, sizeof newline, "%s%s%s", left, sep, vbuf);
                    did = 1;
                    divided++;
                  }
                }
              }
              if (!did)
                snprintf(newline, sizeof newline, "%s", field);
            }
            {
              size_t nlen = strlen(newline);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, newline, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, newline, t);
                olen += t;
              }
              out[olen] = 0;
            }
            kept++;
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "DIVKV", out);
      var_set_str(vm, "IDIVKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "DIVKV_N", divided);
      var_set_num(vm, "DIVKV_D", div);
      var_set_num(vm, "IDIVKV_N", divided);
      var_set_num(vm, "QUOTKV_D", div);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS ADDKV|OFFSETKV|ADDVAL bag delta [sep]
     * — add integer delta to each numeric key:val value. Default sep ":".
     * LAST = bag; LAST_N = field count; ADDKV_N = fields adjusted;
     * ADDKV_D = delta used.
     * Usability: Laplace +1 / score offset before PCTKV without EACH+KVINC. */
    if (kw(&L->cur,"ADDKV") || kw(&L->cur,"OFFSETKV") || kw(&L->cur,"ADDVAL") ||
        kw(&L->cur,"BIASKV") || kw(&L->cur,"SHIFTKV") || kw(&L->cur,"ADDFALL") ||
        kw(&L->cur,"INCALL") || kw(&L->cur,"BUMPALL") || kw(&L->cur,"LAPLACE")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[CUBALC_HOST_STR_MAX];
      char left[256], newline[768], vbuf[40];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long delta = 1, kept = 0, adjusted = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        delta = parse_expr(vm, L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *dv = var_get(vm, L->cur.text, 0);
        if (dv && !dv->is_str) {
          delta = (long)dv->val;
          lex_next(L);
        }
      }
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *sp = sn ? strstr(field, sep) : NULL;
              char *end = 0;
              long v;
              int did = 0;
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
                v = strtol(sp + sn, &end, 10);
                if (end != sp + sn) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0) {
                    v = v + delta;
                    snprintf(vbuf, sizeof vbuf, "%ld", v);
                    snprintf(newline, sizeof newline, "%s%s%s", left, sep, vbuf);
                    did = 1;
                    adjusted++;
                  }
                }
              }
              if (!did)
                snprintf(newline, sizeof newline, "%s", field);
            }
            {
              size_t nlen = strlen(newline);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, newline, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, newline, t);
                olen += t;
              }
              out[olen] = 0;
            }
            kept++;
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "ADDKV", out);
      var_set_str(vm, "OFFSETKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "ADDKV_N", adjusted);
      var_set_num(vm, "ADDKV_D", delta);
      var_set_num(vm, "OFFSETKV_N", adjusted);
      var_set_num(vm, "OFFSETKV_D", delta);
      var_set_num(vm, "ADDVAL_N", adjusted);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS ABSKV|MAGKV|ABSALL bag [sep]
     * — take absolute value of each numeric key:val value. Default sep ":".
     * LAST = bag; LAST_N = field count; ABSKV_N = fields rewritten (negatives).
     * Usability: rank DIFFKV deltas by magnitude with TOPKEY without EACH+IABS. */
    if (kw(&L->cur,"ABSKV") || kw(&L->cur,"MAGKV") || kw(&L->cur,"ABSALL") ||
        kw(&L->cur,"KVMAG") || kw(&L->cur,"ABSHIST") || kw(&L->cur,"MAGNITUDE") ||
        kw(&L->cur,"ABSCOUNT") || kw(&L->cur,"POSMAG") || kw(&L->cur,"MAGNITUDEKV")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[CUBALC_HOST_STR_MAX];
      char left[256], newline[768], vbuf[40];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long kept = 0, rewritten = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *sp = sn ? strstr(field, sep) : NULL;
              char *end = 0;
              long v;
              int did = 0;
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
                v = strtol(sp + sn, &end, 10);
                if (end != sp + sn) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0) {
                    if (v < 0) {
                      v = -v;
                      rewritten++;
                    }
                    snprintf(vbuf, sizeof vbuf, "%ld", v);
                    snprintf(newline, sizeof newline, "%s%s%s", left, sep, vbuf);
                    did = 1;
                  }
                }
              }
              if (!did)
                snprintf(newline, sizeof newline, "%s", field);
            }
            {
              size_t nlen = strlen(newline);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, newline, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, newline, t);
                olen += t;
              }
              out[olen] = 0;
            }
            kept++;
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "ABSKV", out);
      var_set_str(vm, "MAGKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "ABSKV_N", rewritten);
      var_set_num(vm, "ABSKV_KEYS", kept);
      var_set_num(vm, "MAGKV_N", rewritten);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SIGNKV|DIRKV|SGNKV bag [sep]
     * — map each numeric key:val value to signum: -1, 0, or 1. Default sep ":".
     * LAST = bag; LAST_N = field count; SIGNKV_N = fields rewritten;
     * SIGNKV_POS / SIGNKV_NEG / SIGNKV_ZERO = counts per sign class.
     * Usability: DIFFKV direction map without EACH+SIGN+KVSET (pairs with ABSKV). */
    if (kw(&L->cur,"SIGNKV") || kw(&L->cur,"DIRKV") || kw(&L->cur,"SGNKV") ||
        kw(&L->cur,"SIGNUMKV") || kw(&L->cur,"DIRHIST") || kw(&L->cur,"TRENDKV") ||
        kw(&L->cur,"POLKV") || kw(&L->cur,"POLARITY") || kw(&L->cur,"SGNALL")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[CUBALC_HOST_STR_MAX];
      char left[256], newline[768], vbuf[40];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long kept = 0, rew = 0, npos = 0, nneg = 0, nzero = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *sp = sn ? strstr(field, sep) : NULL;
              char *end = 0;
              long v, s;
              int did = 0;
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
                v = strtol(sp + sn, &end, 10);
                if (end != sp + sn) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0) {
                    if (v > 0) { s = 1; npos++; }
                    else if (v < 0) { s = -1; nneg++; }
                    else { s = 0; nzero++; }
                    snprintf(vbuf, sizeof vbuf, "%ld", s);
                    snprintf(newline, sizeof newline, "%s%s%s", left, sep, vbuf);
                    did = 1;
                    rew++;
                  }
                }
              }
              if (!did)
                snprintf(newline, sizeof newline, "%s", field);
            }
            {
              size_t nlen = strlen(newline);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, newline, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, newline, t);
                olen += t;
              }
              out[olen] = 0;
            }
            kept++;
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "SIGNKV", out);
      var_set_str(vm, "DIRKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "SIGNKV_N", rew);
      var_set_num(vm, "SIGNKV_POS", npos);
      var_set_num(vm, "SIGNKV_NEG", nneg);
      var_set_num(vm, "SIGNKV_ZERO", nzero);
      var_set_num(vm, "DIRKV_POS", npos);
      var_set_num(vm, "DIRKV_NEG", nneg);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }

  return 0;
}
