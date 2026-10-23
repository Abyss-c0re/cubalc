/* CubalC lang — lang_ops_sys_kvmath.c
 * SYS bag KV math residual: MERGEKV/DIFFKV/SUMKV/AVGKV/MEDIANKV/TOPKEY (A2 peel).
 * Mesh vote A2_theme_peel_core_sys · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_sys_kvmath.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_sys_try_kvmath(VM *vm, Lex *L) {
  /* SYS bag-KV math residual. Caller consumed SYS|HOST.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
    /* SYS MERGEKV|KVADDALL|ADDFREQ bag_a bag_b [sep]
     * — merge two key:val bags by summing numeric values for shared keys;
     * keys only in bag_b are appended. Default sep ":". LAST = merged bag;
     * LAST_N = field count; MERGEKV_N = same; MERGEKV_ADDED = keys appended
     * from bag_b; MERGEKV_HIT = keys that already existed in bag_a and got
     * their values summed.
     * Usability: combine FREQ histograms from multiple logs without EACH+KVINC. */
    if (kw(&L->cur,"MERGEKV") || kw(&L->cur,"KVADDALL") || kw(&L->cur,"ADDFREQ") ||
        kw(&L->cur,"KVUNION") || kw(&L->cur,"MERGEHIST") || kw(&L->cur,"SUMFREQ") ||
        kw(&L->cur,"COMBINEKV") || kw(&L->cur,"KVCOMBINE") || kw(&L->cur,"FUSEKV")){
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX], sep[32];
      char out[CUBALC_HOST_STR_MAX], field[512], left[256], newline[768], vbuf[40];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long kept = 0, added = 0, hit_n = 0;
      lex_next(L);
      a[0] = 0; b[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, a, sizeof a) != 0)
        snprintf(a, sizeof a, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, b, sizeof b) != 0) b[0] = 0;
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      /* seed out from bag_a */
      if (a[0]) {
        p = a;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
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
            }
            kept++;
          }
          if (*p == '\n') p++;
        }
      }
      /* fold bag_b keys into out (KVINC-style) */
      if (b[0]) {
        p = b;
        while (*p) {
          long delta = 0, found_i = -1, hit = 0;
          char key[256];
          char tmp[CUBALC_HOST_STR_MAX];
          size_t tolen = 0;
          long tkept = 0, idx = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            size_t take = flen;
            const char *sp;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            key[0] = 0;
            if (sn == 0) {
              snprintf(key, sizeof key, "%s", field);
              delta = 1;
            } else {
              sp = strstr(field, sep);
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof key) left_n = sizeof key - 1;
                memcpy(key, field, left_n);
                key[left_n] = 0;
                {
                  char *end = 0;
                  delta = strtol(sp + sn, &end, 10);
                  if (end == sp + sn) delta = 0;
                }
              } else {
                snprintf(key, sizeof key, "%s", field);
                delta = 0;
              }
            }
          }
          if (key[0]) {
            const char *q = out;
            tmp[0] = 0;
            tolen = 0;
            tkept = 0;
            idx = 0;
            found_i = -1;
            hit = 0;
            if (out[0]) {
              while (*q) {
                const char *qs = q;
                size_t qflen;
                int match = 0;
                long oldv = 0;
                while (*q && *q != '\n') q++;
                qflen = (size_t)(q - qs);
                {
                  size_t take = qflen;
                  if (take >= sizeof field) take = sizeof field - 1;
                  memcpy(field, qs, take);
                  field[take] = 0;
                  {
                    const char *sp = sn ? strstr(field, sep) : NULL;
                    if (sp) {
                      left_n = (size_t)(sp - field);
                      if (left_n >= sizeof left) left_n = sizeof left - 1;
                      memcpy(left, field, left_n);
                      left[left_n] = 0;
                      match = (strcmp(left, key) == 0);
                      if (match) {
                        char *end = 0;
                        oldv = strtol(sp + sn, &end, 10);
                      }
                    } else {
                      match = (strcmp(field, key) == 0);
                    }
                  }
                  if (match && found_i < 0) {
                    long newv = oldv + delta;
                    snprintf(vbuf, sizeof vbuf, "%ld", newv);
                    if (sn == 0)
                      snprintf(newline, sizeof newline, "%s", vbuf);
                    else
                      snprintf(newline, sizeof newline, "%s%s%s", key, sep, vbuf);
                    found_i = idx;
                    hit = 1;
                  } else {
                    snprintf(newline, sizeof newline, "%s", field);
                  }
                }
                {
                  size_t nlen = strlen(newline);
                  if (tkept > 0 && tolen + 1 < sizeof tmp) tmp[tolen++] = '\n';
                  if (tolen + nlen < sizeof tmp) {
                    memcpy(tmp + tolen, newline, nlen);
                    tolen += nlen;
                  } else if (tolen < sizeof tmp - 1) {
                    size_t t = sizeof tmp - 1 - tolen;
                    memcpy(tmp + tolen, newline, t);
                    tolen += t;
                  }
                  tmp[tolen] = 0;
                }
                tkept++;
                idx++;
                if (*q == '\n') q++;
              }
            }
            if (!hit) {
              if (sn == 0)
                snprintf(newline, sizeof newline, "%s", key);
              else {
                snprintf(vbuf, sizeof vbuf, "%ld", delta);
                snprintf(newline, sizeof newline, "%s%s%s", key, sep, vbuf);
              }
              {
                size_t nlen = strlen(newline);
                if (tkept > 0 && tolen + 1 < sizeof tmp) tmp[tolen++] = '\n';
                if (tolen + nlen < sizeof tmp) {
                  memcpy(tmp + tolen, newline, nlen);
                  tolen += nlen;
                } else if (tolen < sizeof tmp - 1) {
                  size_t t = sizeof tmp - 1 - tolen;
                  memcpy(tmp + tolen, newline, t);
                  tolen += t;
                }
                tmp[tolen] = 0;
              }
              tkept++;
              added++;
            } else {
              hit_n++;
            }
            snprintf(out, sizeof out, "%s", tmp);
            olen = tolen;
            kept = tkept;
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "MERGEKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "MERGEKV_N", kept);
      var_set_num(vm, "MERGEKV_ADDED", added);
      var_set_num(vm, "MERGEKV_HIT", hit_n);
      var_set_num(vm, "KVADDALL_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS DIFFKV|SUBKV|DELTAKV bag_a bag_b [sep]
     * — subtract bag_b key:val values from bag_a (shared keys: a-b);
     * keys only in bag_b are appended as -b. Default sep ":".
     * LAST = delta bag; LAST_N = field count; DIFFKV_HIT = shared keys
     * subtracted; DIFFKV_ADDED = keys appended from bag_b only.
     * Usability: FREQ delta (now − baseline) without EACH+LOOKUPN+arith+KVSET. */
    if (kw(&L->cur,"DIFFKV") || kw(&L->cur,"SUBKV") || kw(&L->cur,"DELTAKV") ||
        kw(&L->cur,"SUBFREQ") || kw(&L->cur,"KVSUB") || kw(&L->cur,"DELTFREQ") ||
        kw(&L->cur,"DELTAFREQ") || kw(&L->cur,"KVDIFF") || kw(&L->cur,"SUBHIST")){
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX], sep[32];
      char out[CUBALC_HOST_STR_MAX], field[512], left[256], newline[768], vbuf[40];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long kept = 0, added = 0, hit_n = 0;
      lex_next(L);
      a[0] = 0; b[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, a, sizeof a) != 0)
        snprintf(a, sizeof a, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, b, sizeof b) != 0) b[0] = 0;
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      /* seed out from bag_a */
      if (a[0]) {
        p = a;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
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
            }
            kept++;
          }
          if (*p == '\n') p++;
        }
      }
      /* fold bag_b keys into out (subtract) */
      if (b[0]) {
        p = b;
        while (*p) {
          long delta = 0, found_i = -1, hit = 0;
          char key[256];
          char tmp[CUBALC_HOST_STR_MAX];
          size_t tolen = 0;
          long tkept = 0, idx = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            size_t take = flen;
            const char *sp;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            key[0] = 0;
            if (sn == 0) {
              snprintf(key, sizeof key, "%s", field);
              delta = 1;
            } else {
              sp = strstr(field, sep);
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof key) left_n = sizeof key - 1;
                memcpy(key, field, left_n);
                key[left_n] = 0;
                {
                  char *end = 0;
                  delta = strtol(sp + sn, &end, 10);
                  if (end == sp + sn) delta = 0;
                }
              } else {
                snprintf(key, sizeof key, "%s", field);
                delta = 0;
              }
            }
          }
          if (key[0]) {
            const char *q = out;
            tmp[0] = 0;
            tolen = 0;
            tkept = 0;
            idx = 0;
            found_i = -1;
            hit = 0;
            if (out[0]) {
              while (*q) {
                const char *qs = q;
                size_t qflen;
                int match = 0;
                long oldv = 0;
                while (*q && *q != '\n') q++;
                qflen = (size_t)(q - qs);
                {
                  size_t take = qflen;
                  if (take >= sizeof field) take = sizeof field - 1;
                  memcpy(field, qs, take);
                  field[take] = 0;
                  {
                    const char *sp = sn ? strstr(field, sep) : NULL;
                    if (sp) {
                      left_n = (size_t)(sp - field);
                      if (left_n >= sizeof left) left_n = sizeof left - 1;
                      memcpy(left, field, left_n);
                      left[left_n] = 0;
                      match = (strcmp(left, key) == 0);
                      if (match) {
                        char *end = 0;
                        oldv = strtol(sp + sn, &end, 10);
                      }
                    } else {
                      match = (strcmp(field, key) == 0);
                    }
                  }
                  if (match && found_i < 0) {
                    long newv = oldv - delta;
                    snprintf(vbuf, sizeof vbuf, "%ld", newv);
                    if (sn == 0)
                      snprintf(newline, sizeof newline, "%s", vbuf);
                    else
                      snprintf(newline, sizeof newline, "%s%s%s", key, sep, vbuf);
                    found_i = idx;
                    hit = 1;
                  } else {
                    snprintf(newline, sizeof newline, "%s", field);
                  }
                }
                {
                  size_t nlen = strlen(newline);
                  if (tkept > 0 && tolen + 1 < sizeof tmp) tmp[tolen++] = '\n';
                  if (tolen + nlen < sizeof tmp) {
                    memcpy(tmp + tolen, newline, nlen);
                    tolen += nlen;
                  } else if (tolen < sizeof tmp - 1) {
                    size_t t = sizeof tmp - 1 - tolen;
                    memcpy(tmp + tolen, newline, t);
                    tolen += t;
                  }
                  tmp[tolen] = 0;
                }
                tkept++;
                idx++;
                if (*q == '\n') q++;
              }
            }
            if (!hit) {
              /* key only in bag_b → append as -delta */
              long neg = -delta;
              if (sn == 0)
                snprintf(newline, sizeof newline, "%s", key);
              else {
                snprintf(vbuf, sizeof vbuf, "%ld", neg);
                snprintf(newline, sizeof newline, "%s%s%s", key, sep, vbuf);
              }
              {
                size_t nlen = strlen(newline);
                if (tkept > 0 && tolen + 1 < sizeof tmp) tmp[tolen++] = '\n';
                if (tolen + nlen < sizeof tmp) {
                  memcpy(tmp + tolen, newline, nlen);
                  tolen += nlen;
                } else if (tolen < sizeof tmp - 1) {
                  size_t t = sizeof tmp - 1 - tolen;
                  memcpy(tmp + tolen, newline, t);
                  tolen += t;
                }
                tmp[tolen] = 0;
              }
              tkept++;
              added++;
            } else {
              hit_n++;
            }
            snprintf(out, sizeof out, "%s", tmp);
            olen = tolen;
            kept = tkept;
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "DIFFKV", out);
      var_set_str(vm, "SUBKV", out);
      var_set_str(vm, "DELTAKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "DIFFKV_N", kept);
      var_set_num(vm, "DIFFKV_ADDED", added);
      var_set_num(vm, "DIFFKV_HIT", hit_n);
      var_set_num(vm, "SUBKV_N", kept);
      var_set_num(vm, "DELTAKV_HIT", hit_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SUMKV|TOTALKV|SUMVALS bag [sep]
     * — sum numeric right-hand sides of key:val bag fields (default sep ":").
     * LAST = decimal string of sum; LAST_N = sum; SUMKV_N = fields used;
     * SUMKV_KEYS = total fields seen (incl. non-numeric skipped).
     * Usability: FREQ total event count without VALS+SUM two-step. */
    if (kw(&L->cur,"SUMKV") || kw(&L->cur,"TOTALKV") || kw(&L->cur,"SUMVALS") ||
        kw(&L->cur,"KVSUM") || kw(&L->cur,"SUMCOUNTS") || kw(&L->cur,"FREQTOTAL") ||
        kw(&L->cur,"TOTALCOUNTS") || kw(&L->cur,"SUMDICT") || kw(&L->cur,"KVTOTAL")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[40];
      const char *p, *start;
      size_t flen, sn;
      long sum = 0, used = 0, keys = 0;
      lex_next(L);
      bag[0] = 0;
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
            keys++;
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
                else rhs = NULL; /* no sep → skip (key-only line) */
              }
              if (rhs) {
                v = strtol(rhs, &end, 10);
                if (end != rhs) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0) {
                    sum += v;
                    used++;
                  }
                }
              }
            }
          }
          if (*p == '\n') p++;
        }
      }
      snprintf(out, sizeof out, "%ld", sum);
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "SUMKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = sum;
      var_set_num(vm, "LAST_N", sum);
      var_set_num(vm, "SUMKV_N", used);
      var_set_num(vm, "SUMKV_KEYS", keys);
      var_set_num(vm, "TOTALKV_N", sum);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS AVGKV|MEANKV|MEANVAL bag [sep]
     * — integer mean of key:val numeric values (sum/used, trunc toward 0).
     * Empty/no numeric fields → 0. Default sep ":".
     * LAST = decimal mean; LAST_N = mean; AVGKV_N = fields used;
     * AVGKV_SUM = sum; AVGKV_KEYS = fields seen.
     * Usability: typical FREQ count without SUMKV+count+DIV glue (≠ DIVKV per-key). */
    if (kw(&L->cur,"AVGKV") || kw(&L->cur,"MEANKV") || kw(&L->cur,"MEANVAL") ||
        kw(&L->cur,"KVAVG") || kw(&L->cur,"AVGVALS") || kw(&L->cur,"MEANCOUNTS") ||
        kw(&L->cur,"FREQAVG") || kw(&L->cur,"AVGHIST") || kw(&L->cur,"KVMEAN")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[40];
      const char *p, *start;
      size_t flen, sn;
      long sum = 0, used = 0, keys = 0, avg = 0;
      lex_next(L);
      bag[0] = 0;
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
            keys++;
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
                  if (end && *end == 0) {
                    sum += v;
                    used++;
                  }
                }
              }
            }
          }
          if (*p == '\n') p++;
        }
      }
      if (used > 0) avg = sum / used;
      snprintf(out, sizeof out, "%ld", avg);
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "AVGKV", out);
      var_set_str(vm, "MEANKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = avg;
      var_set_num(vm, "LAST_N", avg);
      var_set_num(vm, "AVGKV_N", used);
      var_set_num(vm, "AVGKV_SUM", sum);
      var_set_num(vm, "AVGKV_KEYS", keys);
      var_set_num(vm, "MEANKV_N", used);
      var_set_num(vm, "MEANKV_SUM", sum);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MEDIANKV|P50KV|MIDKV bag [sep]
     * — integer median of key:val numeric values (sort; even n → lower mid).
     * Cap 256 values. Empty → 0. Default sep ":".
     * LAST = decimal median; LAST_N = median; MEDIANKV_N = fields used.
     * Usability: outlier-robust typical FREQ count (vs AVGKV) without VALS+MEDIAN. */
    if (kw(&L->cur,"MEDIANKV") || kw(&L->cur,"P50KV") || kw(&L->cur,"MIDKV") ||
        kw(&L->cur,"KVMEDIAN") || kw(&L->cur,"MEDVALS") || kw(&L->cur,"MIDCOUNT") ||
        kw(&L->cur,"FREQMED") || kw(&L->cur,"MEDHIST") || kw(&L->cur,"P50HIST")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[40];
      long vals[256];
      const char *p, *start;
      size_t flen, sn;
      long keys = 0, med = 0;
      int n = 0, i;
      lex_next(L);
      bag[0] = 0;
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
            keys++;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            if (n < 256) {
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
                  if (end && *end == 0)
                    vals[n++] = v;
                }
              }
            }
          }
          if (*p == '\n') p++;
        }
      }
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
        med = vals[(n - 1) / 2];
      }
      snprintf(out, sizeof out, "%ld", med);
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "MEDIANKV", out);
      var_set_str(vm, "P50KV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = med;
      var_set_num(vm, "LAST_N", med);
      var_set_num(vm, "MEDIANKV_N", n);
      var_set_num(vm, "MEDIANKV_KEYS", keys);
      var_set_num(vm, "P50KV_N", n);
      var_set_num(vm, "MIDKV_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS TOPKEY|ARGMAXKV|MAXKEY bag [sep]
     * SYS BOTKEY|ARGMINKV|MINKEY bag [sep]
     * — pick first key with max (or min) numeric value in a key:val bag.
     * LAST = winning key; LAST_N = that value; TOPKEY_V/BOTKEY_V = same;
     * TOPKEY_I = 0-based field index (-1 empty). Default sep ":".
     * Usability: dominant FREQ severity without SORTFREQ+TAKE+BEFORE glue. */
    if (kw(&L->cur,"TOPKEY") || kw(&L->cur,"ARGMAXKV") || kw(&L->cur,"MAXKEY") ||
        kw(&L->cur,"KEYMAX") || kw(&L->cur,"MOSTKEY") || kw(&L->cur,"DOMINANT") ||
        kw(&L->cur,"BOTKEY") || kw(&L->cur,"ARGMINKV") || kw(&L->cur,"MINKEY") ||
        kw(&L->cur,"KEYMIN") || kw(&L->cur,"LEASTKEY") || kw(&L->cur,"RAREST")){
      char op[20];
      int want_min = 0;
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], key[256], best_key[256];
      const char *p, *start;
      size_t flen, sn;
      long idx = 0, best_i = -1, best_v = 0, found = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "BOTKEY") == 0 || strcmp(op, "ARGMINKV") == 0 ||
          strcmp(op, "MINKEY") == 0 || strcmp(op, "KEYMIN") == 0 ||
          strcmp(op, "LEASTKEY") == 0 || strcmp(op, "RAREST") == 0)
        want_min = 1;
      lex_next(L);
      bag[0] = 0; best_key[0] = 0;
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
              long v = 0;
              int okv = 0;
              key[0] = 0;
              if (sp) {
                size_t kn = (size_t)(sp - field);
                if (kn >= sizeof key) kn = sizeof key - 1;
                memcpy(key, field, kn);
                key[kn] = 0;
                v = strtol(sp + sn, &end, 10);
                if (end != sp + sn) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0) okv = 1;
                }
              } else if (sn == 0) {
                /* bare fields: key is field, value treated as 0 */
                snprintf(key, sizeof key, "%s", field);
                v = 0;
                okv = 1;
              }
              if (okv && key[0]) {
                if (!found || (want_min ? (v < best_v) : (v > best_v))) {
                  best_v = v;
                  best_i = idx;
                  snprintf(best_key, sizeof best_key, "%s", key);
                  found = 1;
                }
              }
            }
          }
          idx++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", best_key);
      if (want_min) {
        var_set_str(vm, "BOTKEY", best_key);
        var_set_str(vm, "MINKEY", best_key);
        var_set_num(vm, "BOTKEY_V", found ? best_v : 0);
        var_set_num(vm, "BOTKEY_I", best_i);
        var_set_num(vm, "ARGMINKV_V", found ? best_v : 0);
      } else {
        var_set_str(vm, "TOPKEY", best_key);
        var_set_str(vm, "MAXKEY", best_key);
        var_set_num(vm, "TOPKEY_V", found ? best_v : 0);
        var_set_num(vm, "TOPKEY_I", best_i);
        var_set_num(vm, "ARGMAXKV_V", found ? best_v : 0);
      }
      snprintf(vm->last_str, sizeof vm->last_str, "%s", best_key);
      vm->last_n = found ? best_v : 0;
      var_set_num(vm, "LAST_N", found ? best_v : 0);
      var_set_num(vm, "OK", found ? 1 : 0);
      bump(vm); return 1;
    }

  return 0;
}
