/* CubalC lang — lang_ops_sys_bagmap.c
 * SYS bag map residual: PREFIXALL · FILL · TRIMALL · FREQ · BEFOREALL · FIRSTMATCH (A2 peel).
 * Mesh vote B_core_sys · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_sys_bagmap.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

int cubalc_lang_ops_sys_try_bagmap(VM *vm, Lex *L) {
  /* SYS bag map/transform residual. Caller consumed SYS|HOST.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
    /* SYS PREFIXALL|PREPENDALL|MAPPRE bag prefix — prepend string to every field → LAST.
     * SYS SUFFIXALL|APPENDALL|MAPSUF bag suffix — append string to every field → LAST.
     * LAST_N = field count. Empty bag → "" OK=1.
     * Usability: tag LIST/peer bags (path/, .json) without EACH+CAT rebuild. */
    if (kw(&L->cur,"PREFIXALL") || kw(&L->cur,"PREPENDALL") || kw(&L->cur,"MAPPRE") ||
        kw(&L->cur,"PREALL") || kw(&L->cur,"ADDFRONT") || kw(&L->cur,"TAGPRE") ||
        kw(&L->cur,"SUFFIXALL") || kw(&L->cur,"APPENDALL") || kw(&L->cur,"MAPSUF") ||
        kw(&L->cur,"SUFALL") || kw(&L->cur,"ADDEND") || kw(&L->cur,"TAGSUF") ||
        kw(&L->cur,"POSTFIXALL")){
      char op[20];
      int is_suf;
      char bag[CUBALC_HOST_STR_MAX], affix[512], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t olen = 0, alen, flen;
      long kept = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_suf = (strcmp(op, "SUFFIXALL") == 0 || strcmp(op, "APPENDALL") == 0 ||
                strcmp(op, "MAPSUF") == 0 || strcmp(op, "SUFALL") == 0 ||
                strcmp(op, "ADDEND") == 0 || strcmp(op, "TAGSUF") == 0 ||
                strcmp(op, "POSTFIXALL") == 0);
      lex_next(L);
      bag[0] = 0; affix[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, affix, sizeof affix) != 0)
        affix[0] = 0;
      alen = strlen(affix);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (!is_suf) {
            /* prefix + field */
            if (olen + alen < sizeof out) {
              memcpy(out + olen, affix, alen);
              olen += alen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, affix, t);
              olen += t;
            }
            if (olen + flen < sizeof out) {
              memcpy(out + olen, start, flen);
              olen += flen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, start, t);
              olen += t;
            }
          } else {
            /* field + suffix */
            if (olen + flen < sizeof out) {
              memcpy(out + olen, start, flen);
              olen += flen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, start, t);
              olen += t;
            }
            if (olen + alen < sizeof out) {
              memcpy(out + olen, affix, alen);
              olen += alen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, affix, t);
              olen += t;
            }
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
      var_set_num(vm, "PREFIXALL_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS FILL|REPEATL|REPEATS|BAGFILL n value — bag of n copies of value → LAST.
     * n capped 0..256; n<=0 → empty. Value may be str/ident/num (num stringified).
     * Usability: default status columns for ZIP / pad peer bags without EACH. */
    if (kw(&L->cur,"FILL") || kw(&L->cur,"REPEATL") || kw(&L->cur,"REPEATS") ||
        kw(&L->cur,"BAGFILL") || kw(&L->cur,"FILLBAG") || kw(&L->cur,"NFILL") ||
        kw(&L->cur,"TIMESLINE") || kw(&L->cur,"DUPFIELD")){
      long n = 0;
      int i;
      char val[512], out[CUBALC_HOST_STR_MAX], nbuf[40];
      size_t olen = 0, vlen;
      long kept = 0;
      lex_next(L);
      out[0] = 0; val[0] = 0;
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS) {
        n = parse_prim(vm, L);
      } else {
        n = vm->last_n;
      }
      if (n < 0) n = 0;
      if (n > 256) n = 256;
      if (L->cur.kind == TK_NUM) {
        snprintf(val, sizeof val, "%ld", L->cur.num);
        lex_next(L);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, val, sizeof val) != 0)
          val[0] = 0;
      } else if (vm->last_str[0]) {
        snprintf(val, sizeof val, "%s", vm->last_str);
      } else {
        snprintf(val, sizeof val, "%ld", vm->last_n);
      }
      vlen = strlen(val);
      for (i = 0; i < (int)n; i++) {
        if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        if (olen + vlen < sizeof out) {
          memcpy(out + olen, val, vlen);
          olen += vlen;
        } else if (olen < sizeof out - 1) {
          size_t t = sizeof out - 1 - olen;
          memcpy(out + olen, val, t);
          olen += t;
        }
        out[olen] = 0;
        kept++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "FILL_N", kept);
      snprintf(nbuf, sizeof nbuf, "%ld", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS ENUMERATE|NUMBER|INDEXLINES|NL|ENUM bag [start] [sep]
     * — prefix each field with start+i and sep (default start=0, sep=":").
     * LAST_N = field count. Empty bag → "" OK=1.
     * Usability: ranked/priority plates without EACH+STR+CAT index glue. */
    /* Note: SYS ENUM is IOTA alias (RANGE plane) — do not claim ENUM here. */
    if (kw(&L->cur,"ENUMERATE") || kw(&L->cur,"NUMBER") || kw(&L->cur,"INDEXLINES") ||
        kw(&L->cur,"NUMBERLINES") || kw(&L->cur,"NLNUM") ||
        kw(&L->cur,"IDXLINES") || kw(&L->cur,"WITHINDEX") || kw(&L->cur,"RANKLINES")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], out[CUBALC_HOST_STR_MAX], ibuf[40];
      const char *p, *start;
      size_t olen = 0, sepn, flen, ilen;
      long kept = 0, base = 0, idx;
      lex_next(L);
      bag[0] = 0; sep[0] = ':'; sep[1] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      /* optional start index (num/ident/paren); then optional sep string */
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS) {
        /* Ambiguity: IDENT could be next form. Only treat as start if num/minus/paren
         * or IDENT that resolves as number var. Prefer: num or minus always start;
         * IDENT only if looks numeric after resolve as number via parse_prim when
         * next is also present or kind is clearly numeric. */
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
            L->cur.kind == TK_LPAREN) {
          base = parse_prim(vm, L);
        } else if (L->cur.kind == TK_IDENT) {
          /* peek: if IDENT is a known numeric-ish we still parse_prim (var num) */
          base = parse_prim(vm, L);
        }
      }
      if (L->cur.kind == TK_STR) {
        if (resolve_str_arg(vm, L, sep, sizeof sep) != 0)
          { sep[0] = ':'; sep[1] = 0; }
        if (!sep[0]) { sep[0] = ':'; sep[1] = 0; }
      }
      sepn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          idx = base + kept;
          snprintf(ibuf, sizeof ibuf, "%ld", idx);
          ilen = strlen(ibuf);
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + ilen < sizeof out) {
            memcpy(out + olen, ibuf, ilen);
            olen += ilen;
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            memcpy(out + olen, ibuf, t);
            olen += t;
          }
          if (olen + sepn < sizeof out) {
            memcpy(out + olen, sep, sepn);
            olen += sepn;
          }
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
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "ENUM_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SQUEEZE|COMPACT|DROPEMPTY [BLANK|NB] [bag]
     * — drop empty newline fields (and whitespace-only if BLANK/NB).
     * LAST_N = kept count. Usability: clean VALS/SPLIT/LIST bags without EACH. */
    if (kw(&L->cur,"SQUEEZE") || kw(&L->cur,"COMPACT") || kw(&L->cur,"DROPEMPTY") ||
        kw(&L->cur,"DROPBLANK") || kw(&L->cur,"TRIMLINES") || kw(&L->cur,"STRIPLINES") ||
        kw(&L->cur,"NOTEMPTYL") || kw(&L->cur,"KEEPEMPTY0")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t olen = 0, flen;
      long kept = 0;
      int drop_blank = 0;
      char op[20];
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "DROPBLANK") == 0 || strcmp(op, "TRIMLINES") == 0 ||
          strcmp(op, "STRIPLINES") == 0)
        drop_blank = 1;
      lex_next(L);
      if (kw(&L->cur,"BLANK") || kw(&L->cur,"NB") || kw(&L->cur,"WS") ||
          kw(&L->cur,"WHITESPACE") || kw(&L->cur,"TRIM") || kw(&L->cur,"NONBLANK")) {
        drop_blank = 1;
        lex_next(L);
      }
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (bag[0]) {
        p = bag;
        while (*p) {
          int drop = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen == 0) {
            drop = 1;
          } else if (drop_blank) {
            size_t i;
            drop = 1;
            for (i = 0; i < flen; i++) {
              char c = start[i];
              if (c != ' ' && c != '\t' && c != '\r') { drop = 0; break; }
            }
          }
          if (!drop) {
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
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "SQUEEZE_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS TRIMALL|MAPTRIM|STRIPALL [L|R|BOTH] [bag]
     * — trim leading/trailing whitespace on every newline field → LAST.
     * Default BOTH (LTRIM+RTRIM). LAST_N = field count (empty fields kept).
     * Usability: clean plate lines after READ/SPLIT before SQUEEZE/EQS. */
    if (kw(&L->cur,"TRIMALL") || kw(&L->cur,"MAPTRIM") || kw(&L->cur,"STRIPALL") ||
        kw(&L->cur,"WSALL") || kw(&L->cur,"LTRIMALL") || kw(&L->cur,"RTRIMALL")){
      char op[20];
      int do_l = 1, do_r = 1;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      const char *p, *start, *fe, *fs;
      size_t olen = 0, flen;
      long kept = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "LTRIMALL") == 0) { do_l = 1; do_r = 0; }
      else if (strcmp(op, "RTRIMALL") == 0) { do_l = 0; do_r = 1; }
      lex_next(L);
      if (kw(&L->cur,"L") || kw(&L->cur,"LEFT") || kw(&L->cur,"LTRIM") ||
          kw(&L->cur,"LEADING")) {
        do_l = 1; do_r = 0; lex_next(L);
      } else if (kw(&L->cur,"R") || kw(&L->cur,"RIGHT") || kw(&L->cur,"RTRIM") ||
                 kw(&L->cur,"TRAILING")) {
        do_l = 0; do_r = 1; lex_next(L);
      } else if (kw(&L->cur,"BOTH") || kw(&L->cur,"LR") || kw(&L->cur,"FULL")) {
        do_l = 1; do_r = 1; lex_next(L);
      }
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          fs = start;
          fe = p;
          if (do_l) {
            while (fs < fe && (*fs == ' ' || *fs == '\t' || *fs == '\r')) fs++;
          }
          if (do_r) {
            while (fe > fs && (fe[-1] == ' ' || fe[-1] == '\t' || fe[-1] == '\r'))
              fe--;
          }
          flen = (size_t)(fe - fs);
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + flen < sizeof out) {
            memcpy(out + olen, fs, flen);
            olen += flen;
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            memcpy(out + olen, fs, t);
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
      var_set_num(vm, "TRIMALL_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS UPPERALL|MAPUPPER|UCASEALL [bag] — ASCII upper every newline field → LAST.
     * SYS LOWERALL|MAPLOWER|LCASEALL [bag] — ASCII lower every field.
     * LAST_N = field count. Usability: normalize LIST/severity bags before HASLINE/set ops
     * without EACH LINE + UPPER/LOWER glue. */
    if (kw(&L->cur,"UPPERALL") || kw(&L->cur,"MAPUPPER") || kw(&L->cur,"UCASEALL") ||
        kw(&L->cur,"TOUPPERALL") || kw(&L->cur,"UPCASEALL") ||
        kw(&L->cur,"LOWERALL") || kw(&L->cur,"MAPLOWER") || kw(&L->cur,"LCASEALL") ||
        kw(&L->cur,"TOLOWERALL") || kw(&L->cur,"DOWNCASEALL")){
      char op[20];
      int to_upper = 1;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t olen = 0, flen, i;
      long kept = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "LOWERALL") == 0 || strcmp(op, "MAPLOWER") == 0 ||
          strcmp(op, "LCASEALL") == 0 || strcmp(op, "TOLOWERALL") == 0 ||
          strcmp(op, "DOWNCASEALL") == 0)
        to_upper = 0;
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
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + flen < sizeof out) {
            for (i = 0; i < flen; i++) {
              char c = start[i];
              if (to_upper) {
                if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
              } else {
                if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
              }
              out[olen++] = c;
            }
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            for (i = 0; i < t; i++) {
              char c = start[i];
              if (to_upper) {
                if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
              } else {
                if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
              }
              out[olen++] = c;
            }
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
      var_set_num(vm, "UPPERALL_N", kept);
      var_set_num(vm, "LOWERALL_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MAPREPLACE|GSUBALL|REPLACEBAG bag old new — REPLACEALL on every field.
     * Empty old → no-op (bag unchanged). LAST = bag; LAST_N/MAPREPLACE_N = total
     * replacements; MAPREPLACE_FIELDS = field count.
     * Usability: rewrite path prefixes / tags on LIST bags without EACH+REPLACEALL. */
    if (kw(&L->cur,"MAPREPLACE") || kw(&L->cur,"GSUBALL") || kw(&L->cur,"REPLACEBAG") ||
        kw(&L->cur,"REPLACEALLBAG") || kw(&L->cur,"BAGREPLACE") || kw(&L->cur,"SUBALLBAG") ||
        kw(&L->cur,"MAPGSUB") || kw(&L->cur,"MAPSUBST")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      char olds[256], news[256];
      const char *p, *start, *src;
      size_t olen = 0, flen, oldn, newn, pre, rest, take;
      long fields = 0, did = 0;
      char field[512], fbuf[768];
      size_t fo;
      lex_next(L);
      bag[0] = 0; out[0] = 0; olds[0] = 0; news[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, olds, sizeof olds) != 0) olds[0] = 0;
      if (resolve_str_arg(vm, L, news, sizeof news) != 0) news[0] = 0;
      oldn = strlen(olds);
      newn = strlen(news);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          take = flen;
          if (take >= sizeof field) take = sizeof field - 1;
          memcpy(field, start, take);
          field[take] = 0;
          /* apply REPLACEALL on this field */
          fo = 0;
          fbuf[0] = 0;
          if (oldn == 0) {
            if (take < sizeof fbuf) {
              memcpy(fbuf, field, take);
              fo = take;
              fbuf[fo] = 0;
            }
          } else {
            src = field;
            for (;;) {
              const char *hit = strstr(src, olds);
              if (!hit) {
                rest = strlen(src);
                if (fo + rest >= sizeof fbuf) rest = sizeof fbuf - 1 - fo;
                memcpy(fbuf + fo, src, rest);
                fo += rest;
                fbuf[fo] = 0;
                break;
              }
              pre = (size_t)(hit - src);
              if (fo + pre >= sizeof fbuf) pre = sizeof fbuf - 1 - fo;
              memcpy(fbuf + fo, src, pre);
              fo += pre;
              if (fo + newn < sizeof fbuf) {
                memcpy(fbuf + fo, news, newn);
                fo += newn;
              } else if (fo < sizeof fbuf - 1) {
                size_t nt = sizeof fbuf - 1 - fo;
                memcpy(fbuf + fo, news, nt);
                fo += nt;
              }
              fbuf[fo] = 0;
              did++;
              src = hit + oldn;
              if (fo >= sizeof fbuf - 1) break;
            }
          }
          if (fields > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          {
            size_t n = fo;
            if (olen + n < sizeof out) {
              memcpy(out + olen, fbuf, n);
              olen += n;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, fbuf, t);
              olen += t;
            }
            out[olen] = 0;
          }
          fields++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = did;
      var_set_num(vm, "LAST_N", did);
      var_set_num(vm, "MAPREPLACE_N", did);
      var_set_num(vm, "MAPREPLACE_FIELDS", fields);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS FREQ|HIST|COUNTS|TALLY [bag] [sep] — frequency of each exact field.
     * LAST = "key{sep}count" bag (first-seen order). Default sep ":".
     * LAST_N/FREQ_N = unique keys; FREQ_TOTAL = input field count (cap 64 uniques).
     * Usability: status/severity rollups without EACH LINE + COUNTLINE rebuild. */
    if (kw(&L->cur,"FREQ") || kw(&L->cur,"HIST") || kw(&L->cur,"COUNTS") ||
        kw(&L->cur,"FREQUENCY") || kw(&L->cur,"HISTOGRAM") ||
        kw(&L->cur,"COUNTBAG") || kw(&L->cur,"BAGCOUNT") || kw(&L->cur,"FREQBAG")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], sep[16];
      char keys[64][128];
      long counts[64];
      int nk = 0, k;
      const char *p, *start;
      size_t flen, olen = 0;
      long total = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      /* optional sep if next is a short string (not a form keyword start) */
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      memset(counts, 0, sizeof counts);
      if (bag[0]) {
        p = bag;
        while (*p) {
          char field[128];
          size_t take;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          take = flen;
          if (take >= sizeof field) take = sizeof field - 1;
          memcpy(field, start, take);
          field[take] = 0;
          total++;
          for (k = 0; k < nk; k++) {
            if (strcmp(keys[k], field) == 0) {
              counts[k]++;
              break;
            }
          }
          if (k == nk && nk < 64) {
            snprintf(keys[nk], sizeof keys[0], "%s", field);
            counts[nk] = 1;
            nk++;
          } else if (k == nk) {
            /* overflow uniques: drop further new keys but still count total */
          }
          if (*p == '\n') p++;
        }
      }
      for (k = 0; k < nk; k++) {
        char line[160];
        int n;
        n = snprintf(line, sizeof line, "%s%s%ld", keys[k], sep, counts[k]);
        if (n < 0) n = 0;
        if ((size_t)n >= sizeof line) n = (int)sizeof line - 1;
        if (k > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        if (olen + (size_t)n < sizeof out) {
          memcpy(out + olen, line, (size_t)n);
          olen += (size_t)n;
        } else if (olen < sizeof out - 1) {
          size_t t = sizeof out - 1 - olen;
          memcpy(out + olen, line, t);
          olen += t;
        }
        out[olen] = 0;
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "FREQ", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = nk;
      var_set_num(vm, "LAST_N", nk);
      var_set_num(vm, "FREQ_N", nk);
      var_set_num(vm, "FREQ_TOTAL", total);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SORTFREQ|SORTBYCOUNT|FSORT [bag] [sep] [DESC|ASC]
     * — sort "key{sep}count" FREQ lines by numeric count (default sep ":", DESC).
     * Count is digits after last sep on each line. Stable for equal counts.
     * LAST_N = line count. Usability: top severities after FREQ without shell sort -n. */
    if (kw(&L->cur,"SORTFREQ") || kw(&L->cur,"SORTBYCOUNT") || kw(&L->cur,"FSORT") ||
        kw(&L->cur,"FREQSORT") || kw(&L->cur,"SORTCOUNTS") || kw(&L->cur,"RANKFREQ") ||
        kw(&L->cur,"TOPFREQ") || kw(&L->cur,"HEAVYFIRST")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], sep[16];
      char lines[64][160];
      long counts[64];
      int order[64];
      int n = 0, i, j, desc = 1;
      const char *p, *start;
      size_t sepn, olen = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      /* optional sep (string) and/or DESC|ASC */
      for (;;) {
        if (L->cur.kind == TK_STR) {
          snprintf(sep, sizeof sep, "%s", L->cur.text);
          if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
          lex_next(L);
          continue;
        }
        if (kw(&L->cur,"DESC") || kw(&L->cur,"DOWN") || kw(&L->cur,"REV") ||
            kw(&L->cur,"HEAVY") || kw(&L->cur,"HIGH")) {
          desc = 1; lex_next(L); continue;
        }
        if (kw(&L->cur,"ASC") || kw(&L->cur,"UP") || kw(&L->cur,"LIGHT") ||
            kw(&L->cur,"LOW") || kw(&L->cur,"ASCENDING")) {
          desc = 0; lex_next(L); continue;
        }
        break;
      }
      sepn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p && n < 64) {
          size_t flen, take;
          const char *last;
          char *endc;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          take = flen;
          if (take >= sizeof lines[0]) take = sizeof lines[0] - 1;
          memcpy(lines[n], start, take);
          lines[n][take] = 0;
          /* count = number after last sep */
          counts[n] = 0;
          if (sepn == 0) {
            counts[n] = strtol(lines[n], &endc, 10);
          } else {
            last = NULL;
            {
              const char *q = lines[n];
              while (*q) {
                if (strncmp(q, sep, sepn) == 0) last = q;
                q++;
              }
            }
            if (last) counts[n] = strtol(last + sepn, &endc, 10);
            else counts[n] = strtol(lines[n], &endc, 10);
          }
          order[n] = n;
          n++;
          if (*p == '\n') p++;
        }
      }
      /* stable insertion sort by count */
      for (i = 1; i < n; i++) {
        int oi = order[i];
        long ci = counts[oi];
        j = i - 1;
        while (j >= 0) {
          long cj = counts[order[j]];
          int swap = desc ? (cj < ci) : (cj > ci);
          if (!swap) break;
          order[j + 1] = order[j];
          j--;
        }
        order[j + 1] = oi;
      }
      for (i = 0; i < n; i++) {
        size_t ln = strlen(lines[order[i]]);
        if (i > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        if (olen + ln < sizeof out) {
          memcpy(out + olen, lines[order[i]], ln);
          olen += ln;
        } else if (olen < sizeof out - 1) {
          size_t t = sizeof out - 1 - olen;
          memcpy(out + olen, lines[order[i]], t);
          olen += t;
        }
        out[olen] = 0;
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "FREQ", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "SORTFREQ_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS BEFOREALL|MAPBEFORE bag needle — BEFORE on every bag field → LAST bag.
     * SYS AFTERALL|MAPAFTER bag needle — AFTER on every field.
     * Same miss semantics as BEFORE/AFTER (before→whole field, after→empty).
     * LAST_N = field count; BEFOREALL_HIT = how many fields contained needle.
     * Usability: FREQ key/count peel or kv bags without EACH+BEFORE/AFTER. */
    if (kw(&L->cur,"BEFOREALL") || kw(&L->cur,"MAPBEFORE") || kw(&L->cur,"LEFTALL") ||
        kw(&L->cur,"PREALL") || kw(&L->cur,"BAGBEFORE") ||
        kw(&L->cur,"AFTERALL") || kw(&L->cur,"MAPAFTER") || kw(&L->cur,"RIGHTALL") ||
        kw(&L->cur,"POSTALL") || kw(&L->cur,"BAGAFTER")){
      char op[20];
      int want_after = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], needle[256];
      const char *p, *start;
      size_t olen = 0, flen, nn, take;
      long lines = 0, hits = 0;
      char field[512], cell[512];
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "AFTERALL") == 0 || strcmp(op, "MAPAFTER") == 0 ||
          strcmp(op, "RIGHTALL") == 0 || strcmp(op, "POSTALL") == 0 ||
          strcmp(op, "BAGAFTER") == 0)
        want_after = 1;
      lex_next(L);
      bag[0] = 0; out[0] = 0; needle[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0] = 0;
      nn = strlen(needle);
      if (bag[0]) {
        p = bag;
        while (*p) {
          const char *hit;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          take = flen;
          if (take >= sizeof field) take = sizeof field - 1;
          memcpy(field, start, take);
          field[take] = 0;
          cell[0] = 0;
          if (nn == 0) {
            hits++;
            if (want_after) snprintf(cell, sizeof cell, "%s", field);
            else cell[0] = 0;
          } else {
            hit = strstr(field, needle);
            if (!hit) {
              if (want_after) cell[0] = 0;
              else snprintf(cell, sizeof cell, "%s", field);
            } else {
              hits++;
              if (want_after) {
                snprintf(cell, sizeof cell, "%s", hit + nn);
              } else {
                size_t pre = (size_t)(hit - field);
                if (pre >= sizeof cell) pre = sizeof cell - 1;
                memcpy(cell, field, pre);
                cell[pre] = 0;
              }
            }
          }
          if (lines > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          {
            size_t n = strlen(cell);
            if (olen + n < sizeof out) {
              memcpy(out + olen, cell, n);
              olen += n;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, cell, t);
              olen += t;
            }
            out[olen] = 0;
          }
          lines++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = lines;
      var_set_num(vm, "LAST_N", lines);
      var_set_num(vm, "BEFOREALL_N", lines);
      var_set_num(vm, "AFTERALL_N", lines);
      var_set_num(vm, "BEFOREALL_HIT", hits);
      var_set_num(vm, "AFTERALL_HIT", hits);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS FIRSTMATCH|GREP1|FINDFIELD [I] bag needle — first field containing needle.
     * SYS FIRSTMATCHI — case-insensitive. LAST = field (empty if miss); LAST_N 0|1.
     * FIRSTMATCH_I = 0-based index of hit (-1 miss). Empty needle → first field.
     * Usability: pick one log/peer line without GREP+TAKE 1 glue. */
    if (kw(&L->cur,"FIRSTMATCH") || kw(&L->cur,"GREP1") || kw(&L->cur,"FINDFIELD") ||
        kw(&L->cur,"FIRSTGREP") || kw(&L->cur,"MATCH1") || kw(&L->cur,"FINDLINE_SUB") ||
        kw(&L->cur,"FIRSTMATCHI") || kw(&L->cur,"GREP1I") || kw(&L->cur,"FINDFIELDI")){
      char op[20];
      int icase = 0;
      char bag[CUBALC_HOST_STR_MAX], needle[256], out[512];
      const char *p, *start;
      size_t flen, nn;
      long idx = 0, found = 0, found_i = -1;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "FIRSTMATCHI") == 0 || strcmp(op, "GREP1I") == 0 ||
          strcmp(op, "FINDFIELDI") == 0)
        icase = 1;
      lex_next(L);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      bag[0] = 0; needle[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0] = 0;
      nn = strlen(needle);
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
              hit = 1;
            } else if (!icase) {
              if (strstr(field, needle) != NULL) hit = 1;
            } else {
              size_t fi, j;
              for (fi = 0; field[fi] && !hit; fi++) {
                for (j = 0; j < nn; j++) {
                  char a = field[fi + j], b = needle[j];
                  if (!a) break;
                  if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                  if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                  if (a != b) break;
                }
                if (j == nn) hit = 1;
              }
            }
            if (hit) {
              if (flen >= sizeof out) flen = sizeof out - 1;
              memcpy(out, start, flen);
              out[flen] = 0;
              found = 1;
              found_i = idx;
              break;
            }
          }
          idx++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "GREP", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      var_set_num(vm, "FIRSTMATCH_N", found);
      var_set_num(vm, "FIRSTMATCH_I", found_i);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
  return 0;
}
