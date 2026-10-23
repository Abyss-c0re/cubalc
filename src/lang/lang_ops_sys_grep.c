/* CubalC lang — lang_ops_sys_grep.c
 * SYS bag GREP residual: GREP/FILTER/GREPANY/GREPALL (A2 peel).
 * Mesh vote A2_theme_peel_core_sys · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_sys_grep.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_sys_try_grep(VM *vm, Lex *L) {
  /* SYS bag-GREP residual. Caller consumed SYS|HOST.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
    /* SYS GREP|FILTER needle [str|LAST] — keep newline fields containing needle.
     * SYS GREPV|VGREP — invert. SYS GREPI|IGREP — case-insensitive.
     * SYS GREP I|ICASE|-I needle — same as GREPI. GREPVI for invert+icase.
     * LAST = kept lines; LAST_N/GREP_N = count. Empty needle: GREP keeps all.
     * Usability: filter LIST/logs without shell grep -i. */
    if (kw(&L->cur,"GREP") || kw(&L->cur,"FILTER") || kw(&L->cur,"MATCHLINES") ||
        kw(&L->cur,"GREPV") || kw(&L->cur,"VGREP") || kw(&L->cur,"FILTERV") ||
        kw(&L->cur,"NOMATCH") ||
        kw(&L->cur,"GREPI") || kw(&L->cur,"IGREP") || kw(&L->cur,"FILTERI") ||
        kw(&L->cur,"GREPVI") || kw(&L->cur,"VGREPI") || kw(&L->cur,"IFILTERV")){
      int invert = (kw(&L->cur,"GREPV") || kw(&L->cur,"VGREP") ||
                    kw(&L->cur,"FILTERV") || kw(&L->cur,"NOMATCH") ||
                    kw(&L->cur,"GREPVI") || kw(&L->cur,"VGREPI") ||
                    kw(&L->cur,"IFILTERV"));
      int icase = (kw(&L->cur,"GREPI") || kw(&L->cur,"IGREP") ||
                   kw(&L->cur,"FILTERI") || kw(&L->cur,"GREPVI") ||
                   kw(&L->cur,"VGREPI") || kw(&L->cur,"IFILTERV"));
      char needle[256]="", src[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t olen = 0;
      long kept = 0;
      lex_next(L);
      /* optional I / ICASE / -I after plain GREP/FILTER */
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") || kw(&L->cur,"IGNORECASE") ||
                     kw(&L->cur,"-I") || kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0]=0;
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      out[0] = 0;
      if (src[0]) {
        p = src;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          {
            size_t flen = (size_t)(p - start);
            char field[512];
            int hit;
            if (flen >= sizeof field) flen = sizeof field - 1;
            memcpy(field, start, flen);
            field[flen] = 0;
            if (needle[0] == 0) {
              hit = 1;
            } else if (!icase) {
              hit = (strstr(field, needle) != NULL) ? 1 : 0;
            } else {
              /* case-insensitive substring */
              size_t nl = strlen(needle), fi;
              hit = 0;
              for (fi = 0; field[fi] && !hit; fi++) {
                size_t j;
                for (j = 0; j < nl; j++) {
                  char a = field[fi + j], b = needle[j];
                  if (!a) break;
                  if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                  if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                  if (a != b) break;
                }
                if (j == nl) hit = 1;
              }
            }
            if (invert) hit = !hit;
            if (hit) {
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + flen < sizeof out) {
                memcpy(out + olen, field, flen);
                olen += flen;
              } else if (olen < sizeof out - 1) {
                size_t take = sizeof out - 1 - olen;
                memcpy(out + olen, field, take);
                olen += take;
              }
              out[olen] = 0;
              kept++;
            }
          }
          if (*p == '\n') p++;
        }
      }
      out[olen] = 0;
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "GREP", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "GREP_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS GREPANY|GREPOR|MULTIGREP [I] bag n1 [n2…] — keep fields matching any needle.
     * SYS GREPANYI — case-insensitive. Up to 8 needles. Empty needle list → keep all.
     * LAST_N/GREP_N = kept count.
     * Usability: multi-severity log triage without chained GREP/UNION glue. */
    if (kw(&L->cur,"GREPANY") || kw(&L->cur,"GREPOR") || kw(&L->cur,"MULTIGREP") ||
        kw(&L->cur,"ANYGREP") || kw(&L->cur,"GREPONEOF") || kw(&L->cur,"MATCHANY") ||
        kw(&L->cur,"GREPANYI") || kw(&L->cur,"GREPORI") || kw(&L->cur,"MULTIGREPI")){
      char op[20];
      int icase = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      char needles[8][128];
      int nn = 0, i;
      const char *p, *start;
      size_t olen = 0, flen;
      long kept = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "GREPANYI") == 0 || strcmp(op, "GREPORI") == 0 ||
          strcmp(op, "MULTIGREPI") == 0)
        icase = 1;
      lex_next(L);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      while (nn < 8 && (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
                        L->cur.kind == TK_NUM)) {
        if (L->cur.kind == TK_NUM) {
          snprintf(needles[nn], sizeof needles[0], "%ld", L->cur.num);
          lex_next(L);
          nn++;
        } else {
          if (resolve_str_arg(vm, L, needles[nn], sizeof needles[0]) != 0)
            break;
          nn++;
        }
      }
      if (bag[0]) {
        p = bag;
        while (*p) {
          int hit = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            char field[512];
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            if (nn == 0) {
              hit = 1; /* no needles → keep all */
            } else {
              for (i = 0; i < nn && !hit; i++) {
                if (needles[i][0] == 0) {
                  hit = 1;
                } else if (!icase) {
                  if (strstr(field, needles[i]) != NULL) hit = 1;
                } else {
                  size_t nl = strlen(needles[i]), fi, j;
                  for (fi = 0; field[fi] && !hit; fi++) {
                    for (j = 0; j < nl; j++) {
                      char a = field[fi + j], b = needles[i][j];
                      if (!a) break;
                      if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                      if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                      if (a != b) break;
                    }
                    if (j == nl) hit = 1;
                  }
                }
              }
            }
            if (hit) {
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + flen < sizeof out) {
                memcpy(out + olen, start, flen);
                olen += flen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, start, t);
                olen += t;
              }
              out[olen] = 0;
              kept++;
            }
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "GREP", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "GREP_N", kept);
      var_set_num(vm, "GREPANY_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS GREPALL|GREPAND|MATCHALL [I] bag n1 [n2…] — keep fields matching every needle.
     * SYS GREPALLI — case-insensitive. Up to 8 needles. Empty needle list → keep all.
     * Empty needle is always-match (does not drop fields). LAST_N/GREP_N/GREPALL_N = kept.
     * Usability: multi-tag AND triage without chained GREP (e.g. ERROR+disk without EACH). */
    if (kw(&L->cur,"GREPALL") || kw(&L->cur,"GREPAND") || kw(&L->cur,"MATCHALL") ||
        kw(&L->cur,"ALLGREP") || kw(&L->cur,"ANDGREP") || kw(&L->cur,"MULTIGREPAND") ||
        kw(&L->cur,"GREPALLI") || kw(&L->cur,"GREPANDI") || kw(&L->cur,"MATCHALLI")){
      char op[20];
      int icase = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      char needles[8][128];
      int nn = 0, i;
      const char *p, *start;
      size_t olen = 0, flen;
      long kept = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "GREPALLI") == 0 || strcmp(op, "GREPANDI") == 0 ||
          strcmp(op, "MATCHALLI") == 0)
        icase = 1;
      lex_next(L);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      while (nn < 8 && (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
                        L->cur.kind == TK_NUM)) {
        if (L->cur.kind == TK_NUM) {
          snprintf(needles[nn], sizeof needles[0], "%ld", L->cur.num);
          lex_next(L);
          nn++;
        } else {
          if (resolve_str_arg(vm, L, needles[nn], sizeof needles[0]) != 0)
            break;
          nn++;
        }
      }
      if (bag[0]) {
        p = bag;
        while (*p) {
          int hit = 1;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            char field[512];
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            if (nn > 0) {
              for (i = 0; i < nn && hit; i++) {
                int one = 0;
                if (needles[i][0] == 0) {
                  one = 1; /* empty needle always matches */
                } else if (!icase) {
                  if (strstr(field, needles[i]) != NULL) one = 1;
                } else {
                  size_t nl = strlen(needles[i]), fi, j;
                  for (fi = 0; field[fi] && !one; fi++) {
                    for (j = 0; j < nl; j++) {
                      char a = field[fi + j], b = needles[i][j];
                      if (!a) break;
                      if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                      if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                      if (a != b) break;
                    }
                    if (j == nl) one = 1;
                  }
                }
                if (!one) hit = 0;
              }
            }
            if (hit) {
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + flen < sizeof out) {
                memcpy(out + olen, start, flen);
                olen += flen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, start, t);
                olen += t;
              }
              out[olen] = 0;
              kept++;
            }
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "GREP", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "GREP_N", kept);
      var_set_num(vm, "GREPALL_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }

  return 0;
}
