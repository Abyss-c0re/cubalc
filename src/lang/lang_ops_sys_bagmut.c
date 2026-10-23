/* CubalC lang — lang_ops_sys_bagmut.c
 * SYS bag-line mutators: PUSH/POP/SETLINE/INSERT/DROP/MOVE/REMOVE…
 * Peel from core_sys residual (B_core_sys). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_sys_bagmut.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_sys_try_bagmut(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. SYS already consumed. */
  /* SYS PUSH|ADDLINE|PUSHLINE bag [line|LAST] — append a newline field to bag.
   * Empty bag → line only; empty line still adds a blank field after bag.
   * LAST = bag; LAST_N/PUSH_N = field count after push.
   * Usability: multi-file hit accumulate without CAT + "\\n" glue. */
  if (kw(&L->cur,"PUSH") || kw(&L->cur,"ADDLINE") || kw(&L->cur,"PUSHLINE") ||
      kw(&L->cur,"LINEPUSH") || kw(&L->cur,"BAGPUSH") || kw(&L->cur,"ACCUM")){
    char bag[CUBALC_HOST_STR_MAX], line[CUBALC_HOST_STR_MAX];
    char out[CUBALC_HOST_STR_MAX];
    const char *p;
    long nfields = 0;
    size_t blen, llen, o;
    lex_next(L);
    if (resolve_str_arg(vm, L, bag, sizeof bag) != 0) bag[0] = 0;
    if (resolve_str_arg(vm, L, line, sizeof line) != 0)
      snprintf(line, sizeof line, "%s", vm->last_str);
    blen = strlen(bag);
    llen = strlen(line);
    out[0] = 0;
    o = 0;
    if (blen == 0) {
      if (llen < sizeof out) {
        memcpy(out, line, llen);
        o = llen;
      } else {
        o = sizeof out - 1;
        memcpy(out, line, o);
      }
      out[o] = 0;
      /* first fill: count fields inside line (may itself be multi-line) */
      if (out[0]) {
        p = out;
        while (*p) {
          while (*p && *p != '\n') p++;
          nfields++;
          if (*p == '\n') p++;
        }
      }
    } else {
      long bag_n = 0;
      p = bag;
      while (*p) {
        while (*p && *p != '\n') p++;
        bag_n++;
        if (*p == '\n') p++;
      }
      if (blen < sizeof out) {
        memcpy(out, bag, blen);
        o = blen;
      } else {
        o = sizeof out - 1;
        memcpy(out, bag, o);
      }
      if (o + 1 < sizeof out) out[o++] = '\n';
      if (o + llen < sizeof out) {
        memcpy(out + o, line, llen);
        o += llen;
      } else if (o < sizeof out - 1) {
        size_t take = sizeof out - 1 - o;
        memcpy(out + o, line, take);
        o += take;
      }
      out[o] = 0;
      /* always +1 field even if line is empty (trailing blank field) */
      nfields = bag_n + 1;
    }
    var_set_str(vm, "LAST", out);
    var_set_str(vm, "PUSH", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = nfields;
    var_set_num(vm, "LAST_N", nfields);
    var_set_num(vm, "PUSH_N", nfields);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS PREPEND|UNSHIFT|PUSHFRONT bag [line|LAST] — insert field at front of bag.
   * Dual of PUSH (append). LAST = bag; LAST_N/PREPEND_N = field count.
   * Usability: priority / FIFO enqueue front without REVL+PUSH glue. */
  if (kw(&L->cur,"PREPEND") || kw(&L->cur,"UNSHIFT") || kw(&L->cur,"PUSHFRONT") ||
      kw(&L->cur,"PREPENDLINE") || kw(&L->cur,"LINEPREPEND") || kw(&L->cur,"BAGPREPEND") ||
      kw(&L->cur,"INSERTFRONT")){
    char bag[CUBALC_HOST_STR_MAX], line[CUBALC_HOST_STR_MAX];
    char out[CUBALC_HOST_STR_MAX];
    const char *p;
    long nfields = 0;
    size_t blen, llen, o;
    lex_next(L);
    if (resolve_str_arg(vm, L, bag, sizeof bag) != 0) bag[0] = 0;
    if (resolve_str_arg(vm, L, line, sizeof line) != 0)
      snprintf(line, sizeof line, "%s", vm->last_str);
    blen = strlen(bag);
    llen = strlen(line);
    out[0] = 0;
    o = 0;
    /* front field */
    if (llen < sizeof out) {
      memcpy(out, line, llen);
      o = llen;
    } else {
      o = sizeof out - 1;
      memcpy(out, line, o);
    }
    out[o] = 0;
    if (blen > 0) {
      if (o + 1 < sizeof out) out[o++] = '\n';
      if (o + blen < sizeof out) {
        memcpy(out + o, bag, blen);
        o += blen;
      } else if (o < sizeof out - 1) {
        size_t take = sizeof out - 1 - o;
        memcpy(out + o, bag, take);
        o += take;
      }
      out[o] = 0;
      nfields = 1; /* new front */
      p = bag;
      while (*p) {
        while (*p && *p != '\n') p++;
        nfields++;
        if (*p == '\n') p++;
      }
    } else {
      nfields = 1; /* only the new front field (even if empty) */
    }
    var_set_str(vm, "LAST", out);
    var_set_str(vm, "PREPEND", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = nfields;
    var_set_num(vm, "LAST_N", nfields);
    var_set_num(vm, "PREPEND_N", nfields);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS POP|POPLINE|SHIFT [bag|LAST] — remove last newline field from bag.
   * LAST = popped field; POP_REST/REST = remaining bag; LAST_N = 1 if popped, 0 if empty.
   * POP_N = remaining field count. Empty bag → LAST "", REST "", LAST_N=0.
   * Usability: process PUSH bags one field at a time without NTH index glue.
   * Note: not stack POP (that is bare POP outside SYS). */
  if (kw(&L->cur,"POP") || kw(&L->cur,"POPLINE") || kw(&L->cur,"LINEPOP") ||
      kw(&L->cur,"BAGPOP") || kw(&L->cur,"SHIFT") || kw(&L->cur,"UNPUSH")){
    char bag[CUBALC_HOST_STR_MAX], lastf[512], rest[CUBALC_HOST_STR_MAX];
    const char *p, *last_nl = NULL;
    long rest_n = 0, found = 0;
    size_t flen;
    lex_next(L);
    bag[0] = 0;
    if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
      snprintf(bag, sizeof bag, "%s", vm->last_str);
    lastf[0] = 0;
    rest[0] = 0;
    if (!bag[0]) {
      found = 0;
    } else {
      /* find last newline; if none, whole bag is the only field */
      for (p = bag; *p; p++)
        if (*p == '\n') last_nl = p;
      if (!last_nl) {
        snprintf(lastf, sizeof lastf, "%s", bag);
        rest[0] = 0;
        found = 1;
        rest_n = 0;
      } else {
        flen = strlen(last_nl + 1);
        if (flen >= sizeof lastf) flen = sizeof lastf - 1;
        memcpy(lastf, last_nl + 1, flen);
        lastf[flen] = 0;
        {
          size_t rlen = (size_t)(last_nl - bag);
          if (rlen >= sizeof rest) rlen = sizeof rest - 1;
          memcpy(rest, bag, rlen);
          rest[rlen] = 0;
        }
        found = 1;
        if (rest[0]) {
          p = rest;
          while (*p) {
            while (*p && *p != '\n') p++;
            rest_n++;
            if (*p == '\n') p++;
          }
        }
      }
    }
    var_set_str(vm, "LAST", lastf);
    var_set_str(vm, "POP", lastf);
    var_set_str(vm, "POP_REST", rest);
    var_set_str(vm, "REST", rest);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", lastf);
    vm->last_n = found;
    var_set_num(vm, "LAST_N", found);
    var_set_num(vm, "POP_N", rest_n);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS POPHEAD|HEADPOP|SHIFTF [bag|LAST] — peel first newline field (FIFO).
   * LAST = first field; POP_REST/REST = remaining; LAST_N = 1 if peeled, 0 empty.
   * Complements POP (last/LIFO). Note: SYS SHIFT remains POP alias (compat).
   * Usability: FIFO work queue without NTH 0 + DROP 1 glue. */
  if (kw(&L->cur,"POPHEAD") || kw(&L->cur,"HEADPOP") || kw(&L->cur,"SHIFTF") ||
      kw(&L->cur,"SHIFTFRONT") || kw(&L->cur,"DEQUEUE") || kw(&L->cur,"POPFIRST") ||
      kw(&L->cur,"FIRSTPOP") || kw(&L->cur,"UNSHIFT_POP")){
    char bag[CUBALC_HOST_STR_MAX], first[512], rest[CUBALC_HOST_STR_MAX];
    const char *p;
    long rest_n = 0, found = 0;
    size_t flen;
    lex_next(L);
    bag[0] = 0;
    if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
      snprintf(bag, sizeof bag, "%s", vm->last_str);
    first[0] = 0;
    rest[0] = 0;
    if (!bag[0]) {
      found = 0;
    } else {
      p = bag;
      while (*p && *p != '\n') p++;
      flen = (size_t)(p - bag);
      if (flen >= sizeof first) flen = sizeof first - 1;
      memcpy(first, bag, flen);
      first[flen] = 0;
      found = 1;
      if (*p == '\n') {
        snprintf(rest, sizeof rest, "%s", p + 1);
        if (rest[0]) {
          const char *q = rest;
          while (*q) {
            while (*q && *q != '\n') q++;
            rest_n++;
            if (*q == '\n') q++;
          }
        }
      } else {
        rest[0] = 0;
        rest_n = 0;
      }
    }
    var_set_str(vm, "LAST", first);
    var_set_str(vm, "POPHEAD", first);
    var_set_str(vm, "POP_REST", rest);
    var_set_str(vm, "REST", rest);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", first);
    vm->last_n = found;
    var_set_num(vm, "LAST_N", found);
    var_set_num(vm, "POPHEAD_N", rest_n);
    var_set_num(vm, "POP_N", rest_n);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS LINES|NLINES|WC|COUNTLINES [str|LAST] — count newline fields → LAST_N.
   * LAST kept as source text; LINES_N/WC_N = count. Empty → 0.
   * Usability: bag/LIST size after other ops overwrite LAST_N. */
  if (kw(&L->cur,"LINES") || kw(&L->cur,"NLINES") || kw(&L->cur,"WC") ||
      kw(&L->cur,"COUNTLINES") || kw(&L->cur,"LINECOUNT") ||
      kw(&L->cur,"NF") || kw(&L->cur,"NFIELDS")){
    char src[CUBALC_HOST_STR_MAX];
    const char *p;
    long nfields = 0;
    lex_next(L);
    src[0] = 0;
    if (resolve_str_arg(vm, L, src, sizeof src) != 0)
      snprintf(src, sizeof src, "%s", vm->last_str);
    if (src[0]) {
      p = src;
      while (*p) {
        while (*p && *p != '\n') p++;
        nfields++;
        if (*p == '\n') p++;
      }
    }
    var_set_str(vm, "LAST", src);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", src);
    vm->last_n = nfields;
    var_set_num(vm, "LAST_N", nfields);
    var_set_num(vm, "LINES_N", nfields);
    var_set_num(vm, "WC_N", nfields);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS HASLINE|HASFIELD bag needle — exact newline-field membership → LAST_N 0|1.
   * SYS HASLINEI|ILINEIN — case-insensitive exact field (not substring GREP).
   * Usability: work-bag membership after LIST/PUSH without EACH LINE + EQS glue. */
  if (kw(&L->cur,"HASLINE") || kw(&L->cur,"HASFIELD") || kw(&L->cur,"CONTAINSLINE") ||
      kw(&L->cur,"INLINES") || kw(&L->cur,"LINEIN") || kw(&L->cur,"MEMBER") ||
      kw(&L->cur,"HASLINEI") || kw(&L->cur,"HASFIELDI") || kw(&L->cur,"ILINEIN") ||
      kw(&L->cur,"LINEINI") || kw(&L->cur,"MEMBERI")){
    char op[20]; snprintf(op, sizeof op, "%s", L->cur.text);
    for (char *q = op; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    int icase = (strcmp(op, "HASLINEI") == 0 || strcmp(op, "HASFIELDI") == 0 ||
                 strcmp(op, "ILINEIN") == 0 || strcmp(op, "LINEINI") == 0 ||
                 strcmp(op, "MEMBERI") == 0);
    char bag[CUBALC_HOST_STR_MAX], needle[512];
    const char *p, *start;
    long hit = 0;
    size_t nn, flen;
    lex_next(L);
    if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                   kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                   kw(&L->cur,"CI"))){
      icase = 1;
      lex_next(L);
    }
    bag[0] = 0; needle[0] = 0;
    if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
      snprintf(bag, sizeof bag, "%s", vm->last_str);
    if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0] = 0;
    nn = strlen(needle);
    if (bag[0]) {
      p = bag;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen == nn) {
          if (!icase) {
            if (nn == 0 || memcmp(start, needle, nn) == 0) { hit = 1; break; }
          } else {
            size_t i; int ok = 1;
            for (i = 0; i < nn; i++) {
              char a = start[i], b = needle[i];
              if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
              if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
              if (a != b) { ok = 0; break; }
            }
            if (ok) { hit = 1; break; }
          }
        }
        if (*p == '\n') p++;
      }
    } else if (nn == 0) {
      /* empty bag: empty needle is not a field unless bag has empty field */
      hit = 0;
    }
    vm->last_n = hit;
    var_set_num(vm, "LAST_N", hit);
    var_set_num(vm, "HASLINE_N", hit);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS COUNTLINE|COUNTFIELD bag needle — count exact field matches → LAST_N.
   * SYS COUNTLINEI — case-insensitive exact field. Distinct from SYS LINES/COUNTLINES
   * (total fields) and HASLINE (0|1). Usability: how many idle peers / ERROR lines. */
  if (kw(&L->cur,"COUNTLINE") || kw(&L->cur,"COUNTFIELD") || kw(&L->cur,"NMATCH") ||
      kw(&L->cur,"OCCUR") || kw(&L->cur,"OCCURS") || kw(&L->cur,"TALLY") ||
      kw(&L->cur,"COUNTLINEI") || kw(&L->cur,"COUNTFIELDI") || kw(&L->cur,"NMATCHI") ||
      kw(&L->cur,"TALLYI") || kw(&L->cur,"OCCURI")){
    char op[20]; snprintf(op, sizeof op, "%s", L->cur.text);
    for (char *q = op; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    int icase = (strcmp(op, "COUNTLINEI") == 0 || strcmp(op, "COUNTFIELDI") == 0 ||
                 strcmp(op, "NMATCHI") == 0 || strcmp(op, "TALLYI") == 0 ||
                 strcmp(op, "OCCURI") == 0);
    char bag[CUBALC_HOST_STR_MAX], needle[512];
    const char *p, *start;
    long hit = 0;
    size_t nn, flen;
    lex_next(L);
    if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                   kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                   kw(&L->cur,"CI"))){
      icase = 1;
      lex_next(L);
    }
    bag[0] = 0; needle[0] = 0;
    if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
      snprintf(bag, sizeof bag, "%s", vm->last_str);
    if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0] = 0;
    nn = strlen(needle);
    if (bag[0]) {
      p = bag;
      while (*p) {
        int match = 0;
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen == nn) {
          if (!icase) {
            match = (nn == 0 || memcmp(start, needle, nn) == 0) ? 1 : 0;
          } else {
            size_t i; match = 1;
            for (i = 0; i < nn; i++) {
              char a = start[i], b = needle[i];
              if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
              if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
              if (a != b) { match = 0; break; }
            }
          }
        }
        if (match) hit++;
        if (*p == '\n') p++;
      }
    }
    vm->last_n = hit;
    var_set_num(vm, "LAST_N", hit);
    var_set_num(vm, "COUNTLINE_N", hit);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS COUNTMATCH|GREPCOUNT bag needle — count fields containing needle (substring).
   * SYS COUNTMATCHI|GREPCOUNTI — case-insensitive. Empty needle counts all fields.
   * LAST_N/COUNTMATCH_N = hits; does not replace LAST with filtered bag (unlike GREP).
   * Distinct from COUNTLINE (exact field) and LINES (total). Usability: log severity
   * tallies without GREP clobber or EACH+HAS glue. */
  if (kw(&L->cur,"COUNTMATCH") || kw(&L->cur,"GREPCOUNT") || kw(&L->cur,"COUNTGREP") ||
      kw(&L->cur,"MATCHCOUNT") || kw(&L->cur,"SUBCOUNT") || kw(&L->cur,"CONTAINSCOUNT") ||
      kw(&L->cur,"COUNTMATCHI") || kw(&L->cur,"GREPCOUNTI") || kw(&L->cur,"COUNTGREPI") ||
      kw(&L->cur,"MATCHCOUNTI") || kw(&L->cur,"SUBCOUNTI")){
    char op[20]; snprintf(op, sizeof op, "%s", L->cur.text);
    for (char *q = op; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    int icase = (strcmp(op, "COUNTMATCHI") == 0 || strcmp(op, "GREPCOUNTI") == 0 ||
                 strcmp(op, "COUNTGREPI") == 0 || strcmp(op, "MATCHCOUNTI") == 0 ||
                 strcmp(op, "SUBCOUNTI") == 0);
    char bag[CUBALC_HOST_STR_MAX], needle[512];
    const char *p, *start;
    long hit = 0, total = 0;
    size_t nn, flen;
    lex_next(L);
    if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                   kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                   kw(&L->cur,"CI"))){
      icase = 1;
      lex_next(L);
    }
    bag[0] = 0; needle[0] = 0;
    if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
      snprintf(bag, sizeof bag, "%s", vm->last_str);
    if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0] = 0;
    nn = strlen(needle);
    if (bag[0]) {
      p = bag;
      while (*p) {
        int match = 0;
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        total++;
        {
          char field[512];
          size_t take = flen;
          if (take >= sizeof field) take = sizeof field - 1;
          memcpy(field, start, take);
          field[take] = 0;
          if (nn == 0) {
            match = 1;
          } else if (!icase) {
            if (strstr(field, needle) != NULL) match = 1;
          } else {
            size_t fi, j;
            for (fi = 0; field[fi] && !match; fi++) {
              for (j = 0; j < nn; j++) {
                char a = field[fi + j], b = needle[j];
                if (!a) break;
                if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                if (a != b) break;
              }
              if (j == nn) match = 1;
            }
          }
        }
        if (match) hit++;
        if (*p == '\n') p++;
      }
    }
    vm->last_n = hit;
    var_set_num(vm, "LAST_N", hit);
    var_set_num(vm, "COUNTMATCH_N", hit);
    var_set_num(vm, "GREPCOUNT_N", hit);
    var_set_num(vm, "COUNTMATCH_TOTAL", total);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS FINDLINE|LINEINDEX bag needle — 0-based index of first exact field → LAST_N.
   * Miss → LAST_N=-1, OK=0. SYS FINDLINEI — case-insensitive exact field.
   * Completes HASLINE/REMOVELINE for locate-then-NTH without EACH glue. */
  if (kw(&L->cur,"FINDLINE") || kw(&L->cur,"LINEINDEX") || kw(&L->cur,"INDEXLINE") ||
      kw(&L->cur,"FIELDINDEX") || kw(&L->cur,"WHICHLINE") || kw(&L->cur,"LINEOF") ||
      kw(&L->cur,"FINDLINEI") || kw(&L->cur,"LINEINDEXI") || kw(&L->cur,"INDEXLINEI") ||
      kw(&L->cur,"FIELDINDEXI") || kw(&L->cur,"WHICHLINEI")){
    char op[20]; snprintf(op, sizeof op, "%s", L->cur.text);
    for (char *q = op; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    int icase = (strcmp(op, "FINDLINEI") == 0 || strcmp(op, "LINEINDEXI") == 0 ||
                 strcmp(op, "INDEXLINEI") == 0 || strcmp(op, "FIELDINDEXI") == 0 ||
                 strcmp(op, "WHICHLINEI") == 0);
    char bag[CUBALC_HOST_STR_MAX], needle[512];
    const char *p, *start;
    long idx = -1, cur = 0;
    size_t nn, flen;
    lex_next(L);
    if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                   kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                   kw(&L->cur,"CI"))){
      icase = 1;
      lex_next(L);
    }
    bag[0] = 0; needle[0] = 0;
    if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
      snprintf(bag, sizeof bag, "%s", vm->last_str);
    if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0] = 0;
    nn = strlen(needle);
    if (bag[0]) {
      p = bag;
      while (*p) {
        int match = 0;
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen == nn) {
          if (!icase) {
            match = (nn == 0 || memcmp(start, needle, nn) == 0) ? 1 : 0;
          } else {
            size_t i; match = 1;
            for (i = 0; i < nn; i++) {
              char a = start[i], b = needle[i];
              if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
              if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
              if (a != b) { match = 0; break; }
            }
          }
        }
        if (match) { idx = cur; break; }
        cur++;
        if (*p == '\n') p++;
      }
    }
    vm->last_n = idx;
    var_set_num(vm, "LAST_N", idx);
    var_set_num(vm, "FINDLINE_N", idx);
    var_set_num(vm, "OK", idx >= 0 ? 1 : 0);
    bump(vm); return 1;
  }
  /* SYS SETLINE|REPLACELINE bag n value — set 0-based field → LAST bag.
   * LAST_N = 1 if index in range, 0 soft miss (bag unchanged).
   * Usability: FINDLINE then update status in place without EACH rebuild. */
  if (kw(&L->cur,"SETLINE") || kw(&L->cur,"REPLACELINE") || kw(&L->cur,"PUTLINE") ||
      kw(&L->cur,"SETFIELD") || kw(&L->cur,"PUTFIELD") || kw(&L->cur,"REPLACEFIELD") ||
      kw(&L->cur,"LINESET") || kw(&L->cur,"FIELDSET")){
    char bag[CUBALC_HOST_STR_MAX], val[512], out[CUBALC_HOST_STR_MAX];
    const char *p, *start;
    long want = 0, cur = 0, hit = 0;
    size_t flen, vlen, o = 0;
    int first = 1;
    lex_next(L);
    bag[0] = 0; val[0] = 0; out[0] = 0;
    if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
      snprintf(bag, sizeof bag, "%s", vm->last_str);
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
        L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
      want = parse_expr(vm, L);
    else
      want = 0;
    if (resolve_str_arg(vm, L, val, sizeof val) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long n = parse_expr(vm, L);
        snprintf(val, sizeof val, "%ld", n);
      } else {
        val[0] = 0;
      }
    }
    vlen = strlen(val);
    if (want < 0 || !bag[0]) {
      snprintf(out, sizeof out, "%s", bag);
      hit = 0;
    } else {
      p = bag;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (!first && o + 1 < sizeof out) out[o++] = '\n';
        first = 0;
        if (cur == want) {
          hit = 1;
          if (o + vlen < sizeof out) {
            memcpy(out + o, val, vlen);
            o += vlen;
          } else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, val, take);
            o += take;
          }
        } else {
          if (o + flen < sizeof out) {
            memcpy(out + o, start, flen);
            o += flen;
          } else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, start, take);
            o += take;
          }
        }
        out[o] = 0;
        cur++;
        if (*p == '\n') p++;
      }
      if (!hit) {
        /* out-of-range: restore original */
        snprintf(out, sizeof out, "%s", bag);
      }
    }
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = hit;
    var_set_num(vm, "LAST_N", hit);
    var_set_num(vm, "SETLINE_N", hit);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS SETMATCH|REPLACEMATCH bag old new — replace first exact field value → LAST bag.
   * LAST_N = 1 if replaced, 0 soft miss (bag unchanged). SYS SETMATCHI — icase on old.
   * Distinct from SETLINE (by index) and REPLACE (substring). Usability: status by name. */
  if (kw(&L->cur,"SETMATCH") || kw(&L->cur,"REPLACEMATCH") || kw(&L->cur,"CHANGEFIELD") ||
      kw(&L->cur,"UPDATEFIELD") || kw(&L->cur,"SUBFIELD") || kw(&L->cur,"MAPFIELD") ||
      kw(&L->cur,"REWRITEFIELD") || kw(&L->cur,"SETMATCHI") || kw(&L->cur,"REPLACEMATCHI") ||
      kw(&L->cur,"CHANGEFIELDI") || kw(&L->cur,"UPDATEFIELDI") || kw(&L->cur,"MAPFIELDI")){
    char op[24]; snprintf(op, sizeof op, "%s", L->cur.text);
    for (char *q = op; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    int icase = (strcmp(op, "SETMATCHI") == 0 || strcmp(op, "REPLACEMATCHI") == 0 ||
                 strcmp(op, "CHANGEFIELDI") == 0 || strcmp(op, "UPDATEFIELDI") == 0 ||
                 strcmp(op, "MAPFIELDI") == 0);
    char bag[CUBALC_HOST_STR_MAX], oldv[512], newv[512], out[CUBALC_HOST_STR_MAX];
    const char *p, *start;
    long hit = 0;
    size_t nn, flen, vlen, o = 0;
    int first = 1;
    lex_next(L);
    if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                   kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                   kw(&L->cur,"CI"))){
      icase = 1;
      lex_next(L);
    }
    bag[0] = 0; oldv[0] = 0; newv[0] = 0; out[0] = 0;
    if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
      snprintf(bag, sizeof bag, "%s", vm->last_str);
    if (resolve_str_arg(vm, L, oldv, sizeof oldv) != 0) oldv[0] = 0;
    if (resolve_str_arg(vm, L, newv, sizeof newv) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long n = parse_expr(vm, L);
        snprintf(newv, sizeof newv, "%ld", n);
      } else {
        newv[0] = 0;
      }
    }
    nn = strlen(oldv);
    vlen = strlen(newv);
    if (!bag[0]) {
      hit = 0;
    } else {
      p = bag;
      while (*p) {
        int match = 0;
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (!hit && flen == nn) {
          if (!icase) {
            match = (nn == 0 || memcmp(start, oldv, nn) == 0) ? 1 : 0;
          } else {
            size_t i; match = 1;
            for (i = 0; i < nn; i++) {
              char a = start[i], b = oldv[i];
              if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
              if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
              if (a != b) { match = 0; break; }
            }
          }
        }
        if (!first && o + 1 < sizeof out) out[o++] = '\n';
        first = 0;
        if (match) {
          hit = 1;
          if (o + vlen < sizeof out) {
            memcpy(out + o, newv, vlen);
            o += vlen;
          } else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, newv, take);
            o += take;
          }
        } else {
          if (o + flen < sizeof out) {
            memcpy(out + o, start, flen);
            o += flen;
          } else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, start, take);
            o += take;
          }
        }
        out[o] = 0;
        if (*p == '\n') p++;
      }
      if (!hit) {
        snprintf(out, sizeof out, "%s", bag);
      }
    }
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = hit;
    var_set_num(vm, "LAST_N", hit);
    var_set_num(vm, "SETMATCH_N", hit);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS INSERTLINE|INSLINE bag n value — insert field before 0-based index → LAST bag.
   * n < 0 soft miss (bag unchanged, LAST_N=0). n >= field count → append (like PUSH).
   * n == 0 → front (like PREPEND). LAST_N = field count after insert.
   * Usability: priority/ordered work bags without PREPEND+TAKE+DROP glue. */
  if (kw(&L->cur,"INSERTLINE") || kw(&L->cur,"INSLINE") || kw(&L->cur,"INSERTFIELD") ||
      kw(&L->cur,"LINEINSERT") || kw(&L->cur,"ADDAT") || kw(&L->cur,"INSERTAT") ||
      kw(&L->cur,"PUTAT") || kw(&L->cur,"SPLICEIN")){
    char bag[CUBALC_HOST_STR_MAX], val[512], out[CUBALC_HOST_STR_MAX];
    const char *p, *start;
    long want = 0, cur = 0, nfields = 0, ok_ins = 0;
    size_t flen, vlen, o = 0;
    int first = 1, inserted = 0;
    lex_next(L);
    bag[0] = 0; val[0] = 0; out[0] = 0;
    if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
      snprintf(bag, sizeof bag, "%s", vm->last_str);
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
        L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
      want = parse_expr(vm, L);
    else
      want = 0;
    if (resolve_str_arg(vm, L, val, sizeof val) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long n = parse_expr(vm, L);
        snprintf(val, sizeof val, "%ld", n);
      } else {
        val[0] = 0;
      }
    }
    vlen = strlen(val);
    if (want < 0) {
      snprintf(out, sizeof out, "%s", bag);
      ok_ins = 0;
      if (bag[0]) {
        p = bag;
        while (*p) {
          while (*p && *p != '\n') p++;
          nfields++;
          if (*p == '\n') p++;
        }
      }
    } else {
      ok_ins = 1;
      p = bag;
      /* empty bag: just the value */
      if (!bag[0]) {
        if (vlen < sizeof out) {
          memcpy(out, val, vlen);
          o = vlen;
        } else {
          size_t take = sizeof out - 1;
          memcpy(out, val, take);
          o = take;
        }
        out[o] = 0;
        inserted = 1;
        nfields = 1;
      } else {
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (cur == want && !inserted) {
            if (!first && o + 1 < sizeof out) out[o++] = '\n';
            first = 0;
            if (o + vlen < sizeof out) {
              memcpy(out + o, val, vlen);
              o += vlen;
            } else if (o < sizeof out - 1) {
              size_t take = sizeof out - 1 - o;
              memcpy(out + o, val, take);
              o += take;
            }
            out[o] = 0;
            inserted = 1;
            nfields++;
          }
          if (!first && o + 1 < sizeof out) out[o++] = '\n';
          first = 0;
          if (o + flen < sizeof out) {
            memcpy(out + o, start, flen);
            o += flen;
          } else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, start, take);
            o += take;
          }
          out[o] = 0;
          nfields++;
          cur++;
          if (*p == '\n') p++;
        }
        if (!inserted) {
          /* n past end → append */
          if (!first && o + 1 < sizeof out) out[o++] = '\n';
          if (o + vlen < sizeof out) {
            memcpy(out + o, val, vlen);
            o += vlen;
          } else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, val, take);
            o += take;
          }
          out[o] = 0;
          inserted = 1;
          nfields++;
        }
      }
    }
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = ok_ins ? nfields : 0;
    var_set_num(vm, "LAST_N", ok_ins ? nfields : 0);
    var_set_num(vm, "INSERTLINE_N", ok_ins ? nfields : 0);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS DROPNTH|DROPAT bag n — drop 0-based field by index → LAST bag.
   * LAST_N = 1 if dropped, 0 soft miss (OOR/empty/neg; bag unchanged).
   * Distinct from SYS DROP n (drop first-n window) and REMOVELINE (by value).
   * Usability: FINDLINE then delete without needing exact needle after mutate. */
  if (kw(&L->cur,"DROPNTH") || kw(&L->cur,"DROPAT") || kw(&L->cur,"REMOVEAT") ||
      kw(&L->cur,"DELETEAT") || kw(&L->cur,"DELAT") || kw(&L->cur,"DROPINDEX") ||
      kw(&L->cur,"REMOVEINDEX") || kw(&L->cur,"DELETEINDEX") || kw(&L->cur,"ERASEN") ||
      kw(&L->cur,"LINEDEL") || kw(&L->cur,"DELLINEAT") || kw(&L->cur,"OMITN")){
    char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
    const char *p, *start;
    long want = 0, cur = 0, hit = 0;
    size_t flen, o = 0;
    int first_kept = 1;
    lex_next(L);
    bag[0] = 0; out[0] = 0;
    if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
      snprintf(bag, sizeof bag, "%s", vm->last_str);
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
        L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
      want = parse_expr(vm, L);
    else
      want = 0;
    if (want < 0 || !bag[0]) {
      snprintf(out, sizeof out, "%s", bag);
      hit = 0;
    } else {
      p = bag;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (cur == want) {
          hit = 1;
          /* skip this field */
        } else {
          if (!first_kept && o + 1 < sizeof out) out[o++] = '\n';
          first_kept = 0;
          if (o + flen < sizeof out) {
            memcpy(out + o, start, flen);
            o += flen;
          } else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, start, take);
            o += take;
          }
          out[o] = 0;
        }
        cur++;
        if (*p == '\n') p++;
      }
      if (!hit) {
        snprintf(out, sizeof out, "%s", bag);
      }
    }
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = hit;
    var_set_num(vm, "LAST_N", hit);
    var_set_num(vm, "DROPNTH_N", hit);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS MOVELINE|MOVEAT bag from to — move 0-based field to final index → LAST bag.
   * LAST_N = 1 success, 0 soft miss (from OOR / to<0 / empty). to >= n → end.
   * Usability: promote/demote priority work without EACH rebuild (FINDLINE then MOVE). */
  if (kw(&L->cur,"MOVELINE") || kw(&L->cur,"MOVEAT") || kw(&L->cur,"MOVEFIELD") ||
      kw(&L->cur,"LINEMOVE") || kw(&L->cur,"REORDER") || kw(&L->cur,"SHIFTTO") ||
      kw(&L->cur,"MOVEN") || kw(&L->cur,"RELOCATE")){
    enum { MV_MAX = 256, MV_FLEN = 192 };
    char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
    char fields[MV_MAX][MV_FLEN];
    char held[MV_FLEN];
    const char *p, *start;
    long from_i = 0, to_i = 0, n = 0, hit = 0;
    size_t flen, o = 0;
    int i, first = 1;
    lex_next(L);
    bag[0] = 0; out[0] = 0;
    if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
      snprintf(bag, sizeof bag, "%s", vm->last_str);
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
        L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
      from_i = parse_expr(vm, L);
    else
      from_i = 0;
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
        L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
      to_i = parse_expr(vm, L);
    else
      to_i = 0;
    if (bag[0]) {
      p = bag;
      while (*p && n < MV_MAX) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen >= MV_FLEN) flen = MV_FLEN - 1;
        memcpy(fields[n], start, flen);
        fields[n][flen] = 0;
        n++;
        if (*p == '\n') p++;
      }
    }
    if (n <= 0 || from_i < 0 || from_i >= n || to_i < 0) {
      snprintf(out, sizeof out, "%s", bag);
      hit = 0;
    } else {
      hit = 1;
      if (to_i >= n) to_i = n - 1;
      if (from_i != to_i) {
        snprintf(held, sizeof held, "%s", fields[from_i]);
        if (from_i < to_i) {
          for (i = (int)from_i; i < (int)to_i; i++)
            memcpy(fields[i], fields[i + 1], MV_FLEN);
          memcpy(fields[to_i], held, MV_FLEN);
        } else {
          for (i = (int)from_i; i > (int)to_i; i--)
            memcpy(fields[i], fields[i - 1], MV_FLEN);
          memcpy(fields[to_i], held, MV_FLEN);
        }
      }
      o = 0; first = 1;
      for (i = 0; i < (int)n; i++) {
        flen = strlen(fields[i]);
        if (!first && o + 1 < sizeof out) out[o++] = '\n';
        first = 0;
        if (o + flen < sizeof out) {
          memcpy(out + o, fields[i], flen);
          o += flen;
        } else if (o < sizeof out - 1) {
          size_t take = sizeof out - 1 - o;
          memcpy(out + o, fields[i], take);
          o += take;
        }
        out[o] = 0;
      }
    }
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = hit;
    var_set_num(vm, "LAST_N", hit);
    var_set_num(vm, "MOVELINE_N", hit);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS REMOVELINE|DROPLINE bag needle — drop first exact field match → LAST bag.
   * LAST_N = 1 if removed, 0 if miss (bag unchanged). REST not used.
   * SYS REMOVELINEI — case-insensitive exact field.
   * Usability: dequeue/ack work items after HASLINE without EACH rebuild. */
  if (kw(&L->cur,"REMOVELINE") || kw(&L->cur,"DROPLINE") || kw(&L->cur,"DELETELINE") ||
      kw(&L->cur,"REMOVEFIELD") || kw(&L->cur,"DROPFIELD") || kw(&L->cur,"UNLINE") ||
      kw(&L->cur,"REMOVELINEI") || kw(&L->cur,"DROPLINEI") || kw(&L->cur,"DELETELINEI") ||
      kw(&L->cur,"DROPFIELDI")){
    char op[20]; snprintf(op, sizeof op, "%s", L->cur.text);
    for (char *q = op; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    int icase = (strcmp(op, "REMOVELINEI") == 0 || strcmp(op, "DROPLINEI") == 0 ||
                 strcmp(op, "DELETELINEI") == 0 || strcmp(op, "DROPFIELDI") == 0);
    char bag[CUBALC_HOST_STR_MAX], needle[512], out[CUBALC_HOST_STR_MAX];
    const char *p, *start;
    long removed = 0;
    size_t nn, flen, o = 0;
    int first_kept = 1;
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
    p = bag;
    while (*p) {
      int match = 0;
      start = p;
      while (*p && *p != '\n') p++;
      flen = (size_t)(p - start);
      if (!removed && flen == nn) {
        if (!icase) {
          match = (nn == 0 || memcmp(start, needle, nn) == 0) ? 1 : 0;
        } else {
          size_t i; match = 1;
          for (i = 0; i < nn; i++) {
            char a = start[i], b = needle[i];
            if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
            if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
            if (a != b) { match = 0; break; }
          }
        }
      }
      if (match) {
        removed = 1;
      } else {
        if (!first_kept && o + 1 < sizeof out) out[o++] = '\n';
        first_kept = 0;
        if (o + flen < sizeof out) {
          memcpy(out + o, start, flen);
          o += flen;
        } else if (o < sizeof out - 1) {
          size_t take = sizeof out - 1 - o;
          memcpy(out + o, start, take);
          o += take;
        }
        out[o] = 0;
      }
      if (*p == '\n') p++;
    }
    if (!removed) {
      /* keep original bag */
      snprintf(out, sizeof out, "%s", bag);
    }
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = removed;
    var_set_num(vm, "LAST_N", removed);
    var_set_num(vm, "REMOVE_N", removed);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  return 0;
}
