/* CubalC lang — lang_ops_sys_baglen.c
 * SYS bag field metrics: LENALL/MAXLEN/LONGEST/LCP/STRIPPREFIX/EMPTY/COALESCE
 * Peel from core_sys residual (B_core_sys). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_sys_baglen.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_sys_try_baglen(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. SYS already consumed. */
  /* SYS LENALL|MAPLEN|FIELDLENS [bag] — length of every newline field as a
   * decimal string bag → LAST. LAST_N/LENALL_N = field count;
   * LENALL_SUM = sum of lengths (total chars excl. newlines).
   * Usability: size rollups / max field width without EACH+LEN+PUSH glue;
   * pairs with SYS SUM/MAX/AVG/SORTN on the length bag. */
  if (kw(&L->cur,"LENALL") || kw(&L->cur,"MAPLEN") || kw(&L->cur,"FIELDLENS") ||
      kw(&L->cur,"STRLENS") || kw(&L->cur,"LENBAG") || kw(&L->cur,"LENGTHS") ||
      kw(&L->cur,"SIZES") || kw(&L->cur,"BYTELENS") || kw(&L->cur,"WIDTHS")){
    char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], nbuf[32];
    const char *p, *start;
    size_t olen = 0, flen, nlen;
    long kept = 0, sum = 0;
    lex_next(L);
    bag[0] = 0; out[0] = 0;
    if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
      snprintf(bag, sizeof bag, "%s", vm->last_str);
    if (bag[0]) {
      p = bag;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        sum += (long)flen;
        snprintf(nbuf, sizeof nbuf, "%ld", (long)flen);
        nlen = strlen(nbuf);
        if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        if (olen + nlen < sizeof out) {
          memcpy(out + olen, nbuf, nlen);
          olen += nlen;
        } else if (olen < sizeof out - 1) {
          size_t t = sizeof out - 1 - olen;
          memcpy(out + olen, nbuf, t);
          olen += t;
        }
        out[olen] = 0;
        kept++;
        if (*p == '\n') p++;
      }
    }
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = kept;
    var_set_num(vm, "LAST_N", kept);
    var_set_num(vm, "LENALL_N", kept);
    var_set_num(vm, "LENALL_SUM", sum);
    var_set_num(vm, "MAPLEN_N", kept);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS MAXLEN|MAXWIDTH|FIELDMAX [bag] — max string length among bag fields → LAST_N.
   * SYS MINLEN|MINWIDTH|FIELDMIN [bag] — min length. LAST = decimal of that length.
   * MAXLEN_N/MINLEN_N = field count; MAXLEN_I/MINLEN_I = 0-based index of first extreme
   * (-1 if empty bag). Empty bag → LAST_N 0.
   * Usability: column width for PADALL without LENALL+MAX glue. */
  if (kw(&L->cur,"MAXLEN") || kw(&L->cur,"MAXWIDTH") || kw(&L->cur,"FIELDMAX") ||
      kw(&L->cur,"LONGEST_LEN") || kw(&L->cur,"MAXSTRLEN") || kw(&L->cur,"BAGMAXLEN") ||
      kw(&L->cur,"MINLEN") || kw(&L->cur,"MINWIDTH") || kw(&L->cur,"FIELDMIN") ||
      kw(&L->cur,"SHORTEST_LEN") || kw(&L->cur,"MINSTRLEN") || kw(&L->cur,"BAGMINLEN")){
    char op[20];
    int is_min = 0;
    char bag[CUBALC_HOST_STR_MAX], out[40];
    const char *p, *start;
    size_t flen;
    long kept = 0, best = 0, best_i = -1, idx = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    for (char *q = op; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op, "MINLEN") == 0 || strcmp(op, "MINWIDTH") == 0 ||
        strcmp(op, "FIELDMIN") == 0 || strcmp(op, "SHORTEST_LEN") == 0 ||
        strcmp(op, "MINSTRLEN") == 0 || strcmp(op, "BAGMINLEN") == 0)
      is_min = 1;
    lex_next(L);
    bag[0] = 0;
    if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
      snprintf(bag, sizeof bag, "%s", vm->last_str);
    if (bag[0]) {
      p = bag;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (kept == 0) {
          best = (long)flen;
          best_i = idx;
        } else if (is_min) {
          if ((long)flen < best) {
            best = (long)flen;
            best_i = idx;
          }
        } else {
          if ((long)flen > best) {
            best = (long)flen;
            best_i = idx;
          }
        }
        kept++;
        idx++;
        if (*p == '\n') p++;
      }
    }
    snprintf(out, sizeof out, "%ld", kept ? best : 0L);
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = kept ? best : 0;
    var_set_num(vm, "LAST_N", kept ? best : 0);
    if (is_min) {
      var_set_num(vm, "MINLEN", kept ? best : 0);
      var_set_num(vm, "MINLEN_N", kept);
      var_set_num(vm, "MINLEN_I", best_i);
      var_set_num(vm, "MINWIDTH", kept ? best : 0);
    } else {
      var_set_num(vm, "MAXLEN", kept ? best : 0);
      var_set_num(vm, "MAXLEN_N", kept);
      var_set_num(vm, "MAXLEN_I", best_i);
      var_set_num(vm, "MAXWIDTH", kept ? best : 0);
    }
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS LONGEST|LONGESTLINE|MAXFIELD [bag] — first longest bag field text → LAST.
   * SYS SHORTEST|SHORTESTLINE|MINFIELD [bag] — first shortest field.
   * LAST_N 0|1 found; LONGEST_I/SHORTEST_I = index (-1 miss);
   * LONGEST_LEN/SHORTEST_LEN = length of that field; LONGEST_N = field count.
   * Distinct from MAXLEN/MINLEN (numeric length only) and LONGEST_LEN alias of MAXLEN.
   * Usability: pick longest error/payload without MAXLEN_I+NTH glue. */
  if (kw(&L->cur,"LONGEST") || kw(&L->cur,"LONGESTLINE") || kw(&L->cur,"MAXFIELD") ||
      kw(&L->cur,"LONGESTSTR") || kw(&L->cur,"PICKLONGEST") || kw(&L->cur,"BAGLONGEST") ||
      kw(&L->cur,"SHORTEST") || kw(&L->cur,"SHORTESTLINE") || kw(&L->cur,"MINFIELD") ||
      kw(&L->cur,"SHORTESTSTR") || kw(&L->cur,"PICKSHORTEST") || kw(&L->cur,"BAGSHORTEST")){
    char op[20];
    int is_short = 0;
    char bag[CUBALC_HOST_STR_MAX], out[512];
    const char *p, *start;
    size_t flen, best_len = 0;
    long kept = 0, best_i = -1, idx = 0, found = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    for (char *q = op; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op, "SHORTEST") == 0 || strcmp(op, "SHORTESTLINE") == 0 ||
        strcmp(op, "MINFIELD") == 0 || strcmp(op, "SHORTESTSTR") == 0 ||
        strcmp(op, "PICKSHORTEST") == 0 || strcmp(op, "BAGSHORTEST") == 0)
      is_short = 1;
    lex_next(L);
    bag[0] = 0; out[0] = 0;
    if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
      snprintf(bag, sizeof bag, "%s", vm->last_str);
    if (bag[0]) {
      p = bag;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (kept == 0 ||
            (is_short && flen < best_len) ||
            (!is_short && flen > best_len)) {
          best_len = flen;
          best_i = idx;
          {
            size_t take = flen;
            if (take >= sizeof out) take = sizeof out - 1;
            memcpy(out, start, take);
            out[take] = 0;
          }
          found = 1;
        }
        kept++;
        idx++;
        if (*p == '\n') p++;
      }
    }
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = found;
    var_set_num(vm, "LAST_N", found);
    if (is_short) {
      var_set_num(vm, "SHORTEST_N", kept);
      var_set_num(vm, "SHORTEST_I", best_i);
      var_set_num(vm, "SHORTEST_LEN", found ? (long)best_len : 0);
      var_set_str(vm, "SHORTEST", out);
    } else {
      var_set_num(vm, "LONGEST_N", kept);
      var_set_num(vm, "LONGEST_I", best_i);
      var_set_num(vm, "LONGEST_LEN", found ? (long)best_len : 0);
      var_set_str(vm, "LONGEST", out);
    }
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS COMMONPREFIX|LCP|SHAREDPREFIX [bag] — longest common prefix of all fields.
   * SYS COMMONSUFFIX|LCS|SHAREDSUFFIX [bag] — longest common suffix.
   * LAST = shared string; LAST_N = its length; COMMONPREFIX_N = field count.
   * Empty bag / no share → LAST empty, LAST_N 0. Single field → whole field.
   * Usability: path/peer dir roots or shared ext without EACH+STARTS glue. */
  if (kw(&L->cur,"COMMONPREFIX") || kw(&L->cur,"LCP") || kw(&L->cur,"SHAREDPREFIX") ||
      kw(&L->cur,"COMMONPRE") || kw(&L->cur,"BAGPREFIX") || kw(&L->cur,"SHAREPRE") ||
      kw(&L->cur,"COMMONSUFFIX") || kw(&L->cur,"LCS") || kw(&L->cur,"SHAREDSUFFIX") ||
      kw(&L->cur,"COMMONSUF") || kw(&L->cur,"BAGSUFFIX") || kw(&L->cur,"SHARESUF")){
    char op[20];
    int is_suf = 0;
    char bag[CUBALC_HOST_STR_MAX], out[512];
    char fields[64][256];
    size_t flens[64];
    const char *p, *start;
    size_t flen, i, j, n = 0, plen = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    for (char *q = op; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op, "COMMONSUFFIX") == 0 || strcmp(op, "LCS") == 0 ||
        strcmp(op, "SHAREDSUFFIX") == 0 || strcmp(op, "COMMONSUF") == 0 ||
        strcmp(op, "BAGSUFFIX") == 0 || strcmp(op, "SHARESUF") == 0)
      is_suf = 1;
    lex_next(L);
    bag[0] = 0; out[0] = 0;
    if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
      snprintf(bag, sizeof bag, "%s", vm->last_str);
    if (bag[0]) {
      p = bag;
      while (*p && n < 64) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen >= sizeof fields[0]) flen = sizeof fields[0] - 1;
        memcpy(fields[n], start, flen);
        fields[n][flen] = 0;
        flens[n] = flen;
        n++;
        if (*p == '\n') p++;
      }
    }
    if (n == 0) {
      plen = 0;
    } else if (n == 1) {
      plen = flens[0];
      if (plen >= sizeof out) plen = sizeof out - 1;
      memcpy(out, fields[0], plen);
      out[plen] = 0;
    } else if (!is_suf) {
      /* longest common prefix */
      plen = flens[0];
      for (i = 1; i < n; i++) {
        size_t m = flens[i] < plen ? flens[i] : plen;
        j = 0;
        while (j < m && fields[0][j] == fields[i][j]) j++;
        plen = j;
        if (plen == 0) break;
      }
      if (plen >= sizeof out) plen = sizeof out - 1;
      memcpy(out, fields[0], plen);
      out[plen] = 0;
    } else {
      /* longest common suffix */
      plen = flens[0];
      for (i = 1; i < n; i++) {
        size_t m = flens[i] < plen ? flens[i] : plen;
        j = 0;
        while (j < m &&
               fields[0][flens[0] - 1 - j] == fields[i][flens[i] - 1 - j])
          j++;
        plen = j;
        if (plen == 0) break;
      }
      if (plen >= sizeof out) plen = sizeof out - 1;
      if (plen > 0)
        memcpy(out, fields[0] + (flens[0] - plen), plen);
      out[plen] = 0;
    }
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = (long)plen;
    var_set_num(vm, "LAST_N", (long)plen);
    if (is_suf) {
      var_set_num(vm, "COMMONSUFFIX_N", (long)n);
      var_set_num(vm, "COMMONSUFFIX_LEN", (long)plen);
      var_set_str(vm, "COMMONSUFFIX", out);
      var_set_num(vm, "LCS_N", (long)n);
    } else {
      var_set_num(vm, "COMMONPREFIX_N", (long)n);
      var_set_num(vm, "COMMONPREFIX_LEN", (long)plen);
      var_set_str(vm, "COMMONPREFIX", out);
      var_set_num(vm, "LCP_N", (long)n);
    }
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS STRIPPREFIX|MAPSTRIPPRE bag [prefix] — remove leading prefix from each field
   * if present (only when field starts with prefix). Empty prefix → no-op copy.
   * SYS STRIPSUFFIX|MAPSTRIPSUF bag [suffix] — strip trailing suffix when present.
   * SYS STRIPCOMMON|STRIPLCP [bag] — compute LCP then strip it from every field
   * (relative paths in one step). LAST = bag; LAST_N = field count;
   * STRIPPREFIX_HIT / STRIPSUFFIX_HIT / STRIPCOMMON_HIT = fields shortened;
   * STRIPCOMMON_PRE = prefix removed (STRIPCOMMON only).
   * Usability: path relative after COMMONPREFIX without EACH+AFTER glue. */
  if (kw(&L->cur,"STRIPPREFIX") || kw(&L->cur,"MAPSTRIPPRE") || kw(&L->cur,"DROPPREFIX") ||
      kw(&L->cur,"REMOVEPREFIX") || kw(&L->cur,"CHOPPREFIX") || kw(&L->cur,"UNPREFIX") ||
      kw(&L->cur,"STRIPSUFFIX") || kw(&L->cur,"MAPSTRIPSUF") || kw(&L->cur,"DROPSUFFIX") ||
      kw(&L->cur,"REMOVESUFFIX") || kw(&L->cur,"CHOPSUFFIX") || kw(&L->cur,"UNSUFFIX") ||
      kw(&L->cur,"STRIPCOMMON") || kw(&L->cur,"STRIPLCP") || kw(&L->cur,"RELPATHS") ||
      kw(&L->cur,"STRIPROOT") || kw(&L->cur,"COMMONSTRIP")){
    char op[20];
    int is_suf = 0, is_common = 0;
    char bag[CUBALC_HOST_STR_MAX], needle[512], out[CUBALC_HOST_STR_MAX];
    char field[512], clipped[512];
    char fields[64][256];
    size_t flens[64];
    const char *p, *start;
    size_t flen, nlen, olen = 0, sn, i, j, n = 0, plen = 0;
    long kept = 0, hit = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    for (char *q = op; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op, "STRIPSUFFIX") == 0 || strcmp(op, "MAPSTRIPSUF") == 0 ||
        strcmp(op, "DROPSUFFIX") == 0 || strcmp(op, "REMOVESUFFIX") == 0 ||
        strcmp(op, "CHOPSUFFIX") == 0 || strcmp(op, "UNSUFFIX") == 0)
      is_suf = 1;
    if (strcmp(op, "STRIPCOMMON") == 0 || strcmp(op, "STRIPLCP") == 0 ||
        strcmp(op, "RELPATHS") == 0 || strcmp(op, "STRIPROOT") == 0 ||
        strcmp(op, "COMMONSTRIP") == 0)
      is_common = 1;
    lex_next(L);
    bag[0] = 0; needle[0] = 0; out[0] = 0;
    if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
      snprintf(bag, sizeof bag, "%s", vm->last_str);
    if (!is_common) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0] = 0;
    }
    /* parse fields; for STRIPCOMMON also compute LCP into needle */
    if (bag[0]) {
      p = bag;
      while (*p && n < 64) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen >= sizeof fields[0]) flen = sizeof fields[0] - 1;
        memcpy(fields[n], start, flen);
        fields[n][flen] = 0;
        flens[n] = flen;
        n++;
        if (*p == '\n') p++;
      }
    }
    if (is_common && n > 0) {
      plen = flens[0];
      for (i = 1; i < n; i++) {
        size_t m = flens[i] < plen ? flens[i] : plen;
        j = 0;
        while (j < m && fields[0][j] == fields[i][j]) j++;
        plen = j;
        if (plen == 0) break;
      }
      if (plen >= sizeof needle) plen = sizeof needle - 1;
      memcpy(needle, fields[0], plen);
      needle[plen] = 0;
    }
    nlen = strlen(needle);
    for (i = 0; i < n; i++) {
      flen = flens[i];
      memcpy(field, fields[i], flen + 1);
      sn = flen;
      if (nlen > 0 && flen >= nlen) {
        if (!is_suf) {
          if (memcmp(field, needle, nlen) == 0) {
            sn = flen - nlen;
            memcpy(clipped, field + nlen, sn);
            clipped[sn] = 0;
            hit++;
          } else {
            memcpy(clipped, field, sn);
            clipped[sn] = 0;
          }
        } else {
          if (memcmp(field + (flen - nlen), needle, nlen) == 0) {
            sn = flen - nlen;
            memcpy(clipped, field, sn);
            clipped[sn] = 0;
            hit++;
          } else {
            memcpy(clipped, field, sn);
            clipped[sn] = 0;
          }
        }
      } else {
        memcpy(clipped, field, sn);
        clipped[sn] = 0;
      }
      if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
      if (olen + sn < sizeof out) {
        memcpy(out + olen, clipped, sn);
        olen += sn;
      } else if (olen < sizeof out - 1) {
        size_t t = sizeof out - 1 - olen;
        memcpy(out + olen, clipped, t);
        olen += t;
      }
      out[olen] = 0;
      kept++;
    }
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = kept;
    var_set_num(vm, "LAST_N", kept);
    var_set_num(vm, "STRIPPREFIX_N", kept);
    var_set_num(vm, "STRIPSUFFIX_N", kept);
    var_set_num(vm, "STRIPCOMMON_N", kept);
    if (is_common) {
      var_set_num(vm, "STRIPCOMMON_HIT", hit);
      var_set_str(vm, "STRIPCOMMON_PRE", needle);
      var_set_num(vm, "STRIPCOMMON_LEN", (long)nlen);
    } else if (is_suf) {
      var_set_num(vm, "STRIPSUFFIX_HIT", hit);
    } else {
      var_set_num(vm, "STRIPPREFIX_HIT", hit);
    }
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS EMPTY|ISEMPTY [str|LAST] — LAST_N 1 if zero-length string.
   * SYS BLANK|ISBLANK|WS — LAST_N 1 if empty or only space/tab/CR/LF.
   * SYS NONEMPTY|NONEMPTY — invert of EMPTY (1 if any char).
   * Usability: soft plate / optional config IF without LEN + compare glue. */
  if (kw(&L->cur,"EMPTY") || kw(&L->cur,"ISEMPTY") || kw(&L->cur,"ISEMPTYSTR") ||
      kw(&L->cur,"BLANK") || kw(&L->cur,"ISBLANK") || kw(&L->cur,"WS") ||
      kw(&L->cur,"ISWS") || kw(&L->cur,"WHITESPACE") ||
      kw(&L->cur,"NONEMPTY") || kw(&L->cur,"NOTEMPTY") || kw(&L->cur,"HASCHARS")){
    char op[16]; snprintf(op, sizeof op, "%s", L->cur.text);
    for (char *q = op; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    int want_blank = (strcmp(op, "BLANK") == 0 || strcmp(op, "ISBLANK") == 0 ||
                      strcmp(op, "WS") == 0 || strcmp(op, "ISWS") == 0 ||
                      strcmp(op, "WHITESPACE") == 0);
    int want_non = (strcmp(op, "NONEMPTY") == 0 || strcmp(op, "NOTEMPTY") == 0 ||
                    strcmp(op, "HASCHARS") == 0);
    char s[CUBALC_HOST_STR_MAX];
    long hit = 0;
    lex_next(L);
    s[0] = 0;
    if (resolve_str_arg(vm, L, s, sizeof s) != 0)
      snprintf(s, sizeof s, "%s", vm->last_str);
    if (want_blank) {
      const char *p = s;
      while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
      hit = (*p == 0) ? 1 : 0;
    } else if (want_non) {
      hit = (s[0] != 0) ? 1 : 0;
    } else {
      hit = (s[0] == 0) ? 1 : 0;
    }
    vm->last_n = hit;
    var_set_num(vm, "LAST_N", hit);
    var_set_num(vm, "EMPTY_N", hit);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS COALESCE|FIRSTS|NVL|IFNUL a b [c…] — first non-empty string arg → LAST.
   * SYS COALESCE BLANK|NB a b… — first non-blank (not only whitespace).
   * LAST_N = length of chosen; all empty → "" LAST_N=0 OK=1.
   * Usability: default chain for ENV/ARG plates without nested IF EMPTY. */
  if (kw(&L->cur,"COALESCE") || kw(&L->cur,"FIRSTS") || kw(&L->cur,"NVL") ||
      kw(&L->cur,"IFNUL") || kw(&L->cur,"IFNULL") || kw(&L->cur,"FIRSTNON") ||
      kw(&L->cur,"ORSTR") || kw(&L->cur,"FIRSTNONEMPTY")){
    char chosen[CUBALC_HOST_STR_MAX];
    char arg[CUBALC_HOST_STR_MAX];
    int skip_blank = 0, got = 0;
    long out_n = 0;
    lex_next(L);
    if (kw(&L->cur,"BLANK") || kw(&L->cur,"NB") || kw(&L->cur,"NONBLANK") ||
        kw(&L->cur,"TRIM") || kw(&L->cur,"WS")) {
      skip_blank = 1;
      lex_next(L);
    }
    chosen[0] = 0;
    while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
           L->cur.kind == TK_NUM) {
      arg[0] = 0;
      if (L->cur.kind == TK_NUM) {
        snprintf(arg, sizeof arg, "%ld", L->cur.num);
        lex_next(L);
      } else if (resolve_str_arg(vm, L, arg, sizeof arg) != 0) {
        break;
      }
      if (!got) {
        int empty = (arg[0] == 0);
        if (skip_blank && !empty) {
          const char *p = arg;
          while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
          empty = (*p == 0);
        }
        if (!empty) {
          snprintf(chosen, sizeof chosen, "%s", arg);
          got = 1;
        }
      }
      /* still consume remaining args even after choice */
    }
    out_n = (long)strlen(chosen);
    var_set_str(vm, "LAST", chosen);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", chosen);
    vm->last_n = out_n;
    var_set_num(vm, "LAST_N", out_n);
    var_set_num(vm, "COALESCE_N", out_n);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  return 0;
}
