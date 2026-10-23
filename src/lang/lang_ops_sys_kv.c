/* CubalC lang — lang_ops_sys_kv.c
 * SYS bag KV residual: LOOKUP/LOOKUPN/KVSET/KVINC/KVDEL (A2 peel).
 * Mesh vote A2_theme_peel_core_sys · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_sys_kv.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_sys_try_kv(VM *vm, Lex *L) {
  /* SYS bag-KV residual. Caller consumed SYS|HOST.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
    /* SYS LOOKUP|KVGET|GETKV [I] bag key [sep] [OR|DEFAULT fallback]
     * — find first bag field whose left side (before sep, default ":") equals key;
     * LAST = right side (value); LOOKUP_LINE = full field; LAST_N 0|1 hit;
     * LOOKUP_I = index (-1 miss). Empty sep → exact field equality → LAST=field.
     * LOOKUPI / KVGETI — case-insensitive key match.
     * OR|DEFAULT|ELSE|FALLBACK: on miss LAST=fallback, OK=1, LOOKUP_N=0, LOOKUP_OR=1
     * (hit ignores trailing OR — same as SYS ENV/READ). Soft miss without empty-value
     * ambiguity for FREQ counts: SYS LOOKUP hist "FATAL" OR "0".
     * Usability: FREQ/plate kv defaults without LOOKUP+IF EMPTY glue. */
    if (kw(&L->cur,"LOOKUP") || kw(&L->cur,"KVGET") || kw(&L->cur,"GETKV") ||
        kw(&L->cur,"DICTGET") || kw(&L->cur,"MAPGET") || kw(&L->cur,"BAGGET") ||
        kw(&L->cur,"LOOKUPI") || kw(&L->cur,"KVGETI") || kw(&L->cur,"GETKVI") ||
        kw(&L->cur,"DICTGETI") || kw(&L->cur,"MAPGETI")){
      char op[20];
      int icase = 0;
      char bag[CUBALC_HOST_STR_MAX], key[256], sep[32], out[512], line[512];
      char fb[512];
      int have_fb = 0, used_or = 0;
      const char *p, *start;
      size_t flen, kn, sn, i;
      long idx = 0, found = 0, found_i = -1;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "LOOKUPI") == 0 || strcmp(op, "KVGETI") == 0 ||
          strcmp(op, "GETKVI") == 0 || strcmp(op, "DICTGETI") == 0 ||
          strcmp(op, "MAPGETI") == 0)
        icase = 1;
      lex_next(L);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      bag[0] = 0; key[0] = 0; out[0] = 0; line[0] = 0; fb[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, key, sizeof key) != 0) key[0] = 0;
      /* optional sep (string or bare token) — never OR/DEFAULT/… */
      if (L->cur.kind == TK_STR) {
        /* string is sep only when not following OR keyword (handled below) */
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT &&
                 (strcmp(L->cur.text, "OR") != 0 &&
                  strcmp(L->cur.text, "SOFT") != 0 &&
                  strcmp(L->cur.text, "DEFAULT") != 0 &&
                  strcmp(L->cur.text, "ELSE") != 0 &&
                  strcmp(L->cur.text, "FALLBACK") != 0)) {
        /* allow bare = or : only if single-char-ish */
        if (strlen(L->cur.text) <= 2) {
          snprintf(sep, sizeof sep, "%s", L->cur.text);
          lex_next(L);
        }
      }
      /* optional OR|DEFAULT fallback (like SYS ENV / ARG / READ) */
      if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
          kw(&L->cur,"FALLBACK") || kw(&L->cur,"SOFT")){
        lex_next(L);
        if (resolve_str_arg(vm, L, fb, sizeof fb) != 0) {
          fail(vm, "SYS LOOKUP bag key [sep] OR \"fallback\"");
          return -1;
        }
        have_fb = 1;
      }
      kn = strlen(key);
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          int hit = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            char field[512];
            char left[256];
            size_t take = flen, left_n = 0;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            if (sn == 0) {
              /* exact field match */
              if (!icase) {
                hit = (strcmp(field, key) == 0);
              } else {
                size_t a = strlen(field), j;
                if (a == kn) {
                  hit = 1;
                  for (j = 0; j < kn; j++) {
                    char ca = field[j], cb = key[j];
                    if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
                    if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
                    if (ca != cb) { hit = 0; break; }
                  }
                }
              }
              if (hit) {
                snprintf(out, sizeof out, "%s", field);
              }
            } else {
              /* key is left of first sep */
              const char *sp = strstr(field, sep);
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
              } else {
                left_n = take;
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
              }
              if (!icase) {
                hit = (strcmp(left, key) == 0);
              } else {
                size_t j;
                if (left_n == kn) {
                  hit = 1;
                  for (j = 0; j < kn; j++) {
                    char ca = left[j], cb = key[j];
                    if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
                    if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
                    if (ca != cb) { hit = 0; break; }
                  }
                }
              }
              if (hit) {
                if (sp) {
                  snprintf(out, sizeof out, "%s", sp + sn);
                } else {
                  out[0] = 0; /* key-only line, empty value */
                }
              }
            }
            if (hit) {
              if (flen >= sizeof line) flen = sizeof line - 1;
              memcpy(line, start, flen);
              line[flen] = 0;
              found = 1;
              found_i = idx;
              break;
            }
          }
          idx++;
          if (*p == '\n') p++;
        }
      }
      (void)i;
      if (!found && have_fb) {
        snprintf(out, sizeof out, "%s", fb);
        used_or = 1;
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "LOOKUP", out);
      var_set_str(vm, "LOOKUP_LINE", line);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      var_set_num(vm, "LOOKUP_N", found);
      var_set_num(vm, "LOOKUP_I", found_i);
      var_set_num(vm, "LOOKUP_OR", used_or);
      var_set_num(vm, "KVGET_N", found);
      var_set_num(vm, "KVGET_OR", used_or);
      /* hit → OK=1; miss+OR → OK=1 (soft default); bare miss → OK=0 */
      var_set_num(vm, "OK", (found || used_or) ? 1 : 0);
      bump(vm); return 1;
    }
    /* SYS LOOKUPN|KVGETN|GETKVN bag key [sep] [OR default]
     * — like LOOKUP but LAST_N = integer parse of the value (not 0|1 hit).
     * LAST still holds value string. On miss+OR: LAST_N = default int, OK=1,
     * LOOKUPN_OR=1. Bare miss: LAST_N=0, OK=0.
     * Usability: FREQ count arithmetic without LOOKUP+SYS NUM glue. */
    if (kw(&L->cur,"LOOKUPN") || kw(&L->cur,"KVGETN") || kw(&L->cur,"GETKVN") ||
        kw(&L->cur,"DICTGETN") || kw(&L->cur,"MAPGETN") || kw(&L->cur,"NUMKV") ||
        kw(&L->cur,"KVNUM") || kw(&L->cur,"ICOUNT") || kw(&L->cur,"NLOOKUP")){
      char bag[CUBALC_HOST_STR_MAX], key[256], sep[32], out[512], line[512];
      char fb[64];
      int have_fb = 0, used_or = 0;
      const char *p, *start;
      size_t flen, kn, sn;
      long idx = 0, found = 0, found_i = -1, nval = 0, defv = 0;
      lex_next(L);
      bag[0] = 0; key[0] = 0; out[0] = 0; line[0] = 0; fb[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, key, sizeof key) != 0) key[0] = 0;
      /* optional sep string (not OR keywords) */
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT &&
                 strcmp(L->cur.text, "OR") != 0 &&
                 strcmp(L->cur.text, "DEFAULT") != 0 &&
                 strcmp(L->cur.text, "ELSE") != 0 &&
                 strcmp(L->cur.text, "FALLBACK") != 0 &&
                 strcmp(L->cur.text, "SOFT") != 0) {
        if (strlen(L->cur.text) <= 2) {
          snprintf(sep, sizeof sep, "%s", L->cur.text);
          lex_next(L);
        }
      }
      if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
          kw(&L->cur,"FALLBACK") || kw(&L->cur,"SOFT")){
        lex_next(L);
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
            L->cur.kind == TK_LPAREN) {
          defv = parse_expr(vm, L);
          snprintf(fb, sizeof fb, "%ld", defv);
          have_fb = 1;
        } else {
          if (resolve_str_arg(vm, L, fb, sizeof fb) != 0) {
            fail(vm, "SYS LOOKUPN bag key [sep] OR default");
            return -1;
          }
          {
            char *end = 0;
            defv = strtol(fb, &end, 10);
            if (end == fb) defv = 0;
          }
          have_fb = 1;
        }
      }
      kn = strlen(key);
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          int hit = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            char field[512];
            char left[256];
            size_t take = flen, left_n = 0;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            if (sn == 0) {
              hit = (strcmp(field, key) == 0);
              if (hit) snprintf(out, sizeof out, "%s", field);
            } else {
              const char *sp = strstr(field, sep);
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
              } else {
                left_n = take;
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
              }
              hit = (strcmp(left, key) == 0);
              if (hit) {
                if (sp) snprintf(out, sizeof out, "%s", sp + sn);
                else out[0] = 0;
              }
            }
            if (hit) {
              if (flen >= sizeof line) flen = sizeof line - 1;
              memcpy(line, start, flen);
              line[flen] = 0;
              found = 1;
              found_i = idx;
              break;
            }
          }
          idx++;
          if (*p == '\n') p++;
        }
      }
      (void)kn;
      if (found) {
        char *end = 0;
        nval = strtol(out, &end, 10);
        if (end == out) nval = 0;
      } else if (have_fb) {
        snprintf(out, sizeof out, "%s", fb);
        nval = defv;
        used_or = 1;
      } else {
        out[0] = 0;
        nval = 0;
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "LOOKUPN", out);
      var_set_str(vm, "LOOKUP_LINE", line);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = nval;
      var_set_num(vm, "LAST_N", nval);
      var_set_num(vm, "LOOKUPN_N", found);
      var_set_num(vm, "LOOKUPN_I", found_i);
      var_set_num(vm, "LOOKUPN_OR", used_or);
      var_set_num(vm, "LOOKUPN_V", nval);
      var_set_num(vm, "KVGETN_V", nval);
      var_set_num(vm, "OK", (found || used_or) ? 1 : 0);
      bump(vm); return 1;
    }
    /* SYS KVSET|SETKV|DICTSET bag key value [sep]
     * — set key:value in bag: if a field's left-of-sep equals key, replace value;
     * else append key+sep+value. Default sep ":". LAST = bag; LAST_N = field count;
     * KVSET_I = index of set field; KVSET_HIT 1=replaced 0=appended.
     * Usability: update FREQ/plate kv without FINDLINE+SETLINE glue; dual of LOOKUP. */
    if (kw(&L->cur,"KVSET") || kw(&L->cur,"SETKV") || kw(&L->cur,"DICTSET") ||
        kw(&L->cur,"MAPSET") || kw(&L->cur,"BAGSET") || kw(&L->cur,"PUTKV") ||
        kw(&L->cur,"KVPUT") || kw(&L->cur,"SETKEY")){
      char bag[CUBALC_HOST_STR_MAX], key[256], val[512], sep[32], out[CUBALC_HOST_STR_MAX];
      char field[512], left[256], newline[768];
      const char *p, *start;
      size_t flen, kn, sn, vn, olen = 0, left_n;
      long idx = 0, kept = 0, hit = 0, found_i = -1;
      lex_next(L);
      bag[0] = 0; key[0] = 0; val[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, key, sizeof key) != 0) key[0] = 0;
      if (resolve_str_arg(vm, L, val, sizeof val) != 0) val[0] = 0;
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      kn = strlen(key);
      sn = strlen(sep);
      vn = strlen(val);
      if (bag[0]) {
        p = bag;
        while (*p) {
          int match = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            if (sn == 0) {
              match = (strcmp(field, key) == 0);
            } else {
              const char *sp = strstr(field, sep);
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
              } else {
                left_n = take;
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
              }
              match = (strcmp(left, key) == 0);
            }
            if (match && found_i < 0) {
              /* replace first match */
              if (sn == 0)
                snprintf(newline, sizeof newline, "%s", val);
              else
                snprintf(newline, sizeof newline, "%s%s%s", key, sep, val);
              found_i = idx;
              hit = 1;
            } else {
              snprintf(newline, sizeof newline, "%s", field);
            }
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
          idx++;
          if (*p == '\n') p++;
        }
      }
      if (!hit) {
        /* append new key:value */
        if (sn == 0)
          snprintf(newline, sizeof newline, "%s", key[0] ? key : val);
        else
          snprintf(newline, sizeof newline, "%s%s%s", key, sep, val);
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
        found_i = kept;
        kept++;
      }
      (void)kn; (void)vn;
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "KVSET", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "KVSET_N", kept);
      var_set_num(vm, "KVSET_I", found_i);
      var_set_num(vm, "KVSET_HIT", hit);
      var_set_num(vm, "SETKV_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS KVINC|INCKV|BUMPKV bag key [delta] [sep]
     * — add delta (default 1) to numeric value of key:val field; if missing, insert
     * key:delta. Default sep ":". LAST = bag; LAST_N = field count;
     * KVINC_V = new value; KVINC_I = index; KVINC_HIT 1=existed 0=inserted.
     * Usability: bump FREQ/severity counters without LOOKUP+arith+KVSET glue. */
    if (kw(&L->cur,"KVINC") || kw(&L->cur,"INCKV") || kw(&L->cur,"BUMPKV") ||
        kw(&L->cur,"KVADD") || kw(&L->cur,"INCKEY") ||
        kw(&L->cur,"COUNTUP") || kw(&L->cur,"KVBUMP")){
      char bag[CUBALC_HOST_STR_MAX], key[256], sep[32], out[CUBALC_HOST_STR_MAX];
      char field[512], left[256], newline[768], vbuf[40];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long idx = 0, kept = 0, hit = 0, found_i = -1, delta = 1, newv = 0;
      lex_next(L);
      bag[0] = 0; key[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, key, sizeof key) != 0) key[0] = 0;
      /* optional delta (number) then optional sep string */
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        delta = parse_expr(vm, L);
      } else if (L->cur.kind == TK_IDENT) {
        /* allow numeric var as delta when next is not a form keyword we care about */
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
          int match = 0;
          long oldv = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
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
              } else if (sn == 0) {
                match = (strcmp(field, key) == 0);
              } else {
                match = (strcmp(field, key) == 0);
                if (match) oldv = 0;
              }
            }
            if (match && found_i < 0) {
              newv = oldv + delta;
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
          idx++;
          if (*p == '\n') p++;
        }
      }
      if (!hit) {
        newv = delta;
        snprintf(vbuf, sizeof vbuf, "%ld", newv);
        if (sn == 0)
          snprintf(newline, sizeof newline, "%s", key[0] ? key : vbuf);
        else
          snprintf(newline, sizeof newline, "%s%s%s", key, sep, vbuf);
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
        found_i = kept;
        kept++;
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "KVINC", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "KVINC_N", kept);
      var_set_num(vm, "KVINC_I", found_i);
      var_set_num(vm, "KVINC_HIT", hit);
      var_set_num(vm, "KVINC_V", newv);
      var_set_num(vm, "INCKV_V", newv);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS KVDEL|DELKV|RMKV bag key [sep]
     * — drop first bag field whose left-of-sep equals key. Default sep ":".
     * LAST = bag (unchanged on miss); LAST_N = remaining field count;
     * KVDEL_HIT 1=removed 0=miss; KVDEL_I = removed index (-1 miss);
     * KVDEL_V = removed value (empty on miss). Soft: miss does not fail OK.
     * Usability: ack/remove FREQ key after handling without FINDLINE+DROPNTH. */
    if (kw(&L->cur,"KVDEL") || kw(&L->cur,"DELKV") || kw(&L->cur,"RMKV") ||
        kw(&L->cur,"DELKEY") || kw(&L->cur,"UNSETKV") || kw(&L->cur,"KVUNSET") ||
        kw(&L->cur,"KVDROP") || kw(&L->cur,"REMOVEKV")){
      char bag[CUBALC_HOST_STR_MAX], key[256], sep[32], out[CUBALC_HOST_STR_MAX];
      char field[512], left[256], dropped_val[512];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long idx = 0, kept = 0, hit = 0, found_i = -1;
      lex_next(L);
      bag[0] = 0; key[0] = 0; out[0] = 0; dropped_val[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, key, sizeof key) != 0) key[0] = 0;
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          int match = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *sp = sn ? strstr(field, sep) : NULL;
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
                match = (strcmp(left, key) == 0);
                if (match && found_i < 0) {
                  size_t vn = strlen(sp + sn);
                  if (vn >= sizeof dropped_val) vn = sizeof dropped_val - 1;
                  memcpy(dropped_val, sp + sn, vn);
                  dropped_val[vn] = 0;
                }
              } else if (sn == 0) {
                match = (strcmp(field, key) == 0);
                if (match && found_i < 0) {
                  snprintf(dropped_val, sizeof dropped_val, "%s", field);
                }
              } else {
                match = (strcmp(field, key) == 0);
                if (match && found_i < 0) dropped_val[0] = 0;
              }
            }
            if (match && found_i < 0) {
              /* skip first match (delete) */
              found_i = idx;
              hit = 1;
            } else {
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
            }
          }
          idx++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "KVDEL", out);
      var_set_str(vm, "KVDEL_V", dropped_val);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "KVDEL_N", kept);
      var_set_num(vm, "KVDEL_I", found_i);
      var_set_num(vm, "KVDEL_HIT", hit);
      var_set_num(vm, "DELKV_N", kept);
      var_set_num(vm, "DELKV_HIT", hit);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }

  return 0;
}
