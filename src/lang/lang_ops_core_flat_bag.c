/* CubalC flat bag-in plane */
#include "lang/cubalc_lang_ops_core_flat_bag.h"
#include "lang/cubalc_lang_ops_core_help.h"
#include "lang/cubalc_lang_ops_sys_cli.h"
#include "lang/cubalc_lang_ops_sys_codec.h"
#include "lang/cubalc_lang_ops_sys_plate_expand.h"
#include "lang/cubalc_lang_ops_sys_time_walk.h"
#include "cubalc_hostops.h"
#include "cubalc_platform.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <unistd.h>
#endif

int cubalc_lang_ops_core_try_flat_bag(VM *vm, Lex *L) {
  /* ENSUREIN name line — append exact bag field if missing (idempotent work set).
   * DROPLINEIN name line — drop first exact bag field (ack without substring GREPVIN).
   * ENSUREIN_HIT 1=added 0=already present; DROPLINEIN_HIT 1=dropped 0=miss.
   * LAST_N = hit; LAST = bag after mutate; LINESIN dual via LAST bag length count.
   * Usability: work-bag sets without HASINBAG+IF+PUSHTO / EACH rebuild glue.
   * Note: DROPIN stays GREPVIN (substring filter); DROPLINEIN is exact membership. */
  if (kw(&L->cur,"ENSUREIN")||kw(&L->cur,"BAGENSUREIN")||kw(&L->cur,"ADDIFMISSINGIN")||
      kw(&L->cur,"ENSUREBAGIN")||kw(&L->cur,"ENSURELINEIN")||
      kw(&L->cur,"DROPLINEIN")||kw(&L->cur,"REMOVELINEIN")||kw(&L->cur,"ACKLINEIN")||
      kw(&L->cur,"DROPBAGLINEIN")||kw(&L->cur,"ACKIN")){
    char op[24], name[48];
    int is_drop = 0;
    char bag[CUBALC_HOST_STR_MAX], line[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
    long hit = 0, nfields = 0;
    size_t llen, blen;
    const char *p, *start;
    Var *vv;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "DROPLINEIN") == 0 || strcmp(op, "REMOVELINEIN") == 0 ||
        strcmp(op, "ACKLINEIN") == 0 || strcmp(op, "DROPBAGLINEIN") == 0 ||
        strcmp(op, "ACKIN") == 0)
      is_drop = 1;
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, is_drop ? "DROPLINEIN needs name line — DROPLINEIN work \"job\""
                       : "ENSUREIN needs name line — ENSUREIN work \"job\"");
      return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    if (kw(&L->cur, "WITH") || kw(&L->cur, "OF") || kw(&L->cur, "LINE") ||
        kw(&L->cur, "FOR") || kw(&L->cur, "AND"))
      lex_next(L);
    line[0] = 0;
    if (resolve_str_arg(vm, L, line, sizeof line) != 0) {
      fail(vm, is_drop ? "DROPLINEIN needs line" : "ENSUREIN needs line");
      return -1;
    }
    bag[0] = 0;
    vv = var_get(vm, name, 0);
    if (vv && vv->is_str)
      snprintf(bag, sizeof bag, "%s", vv->sval);
    else if (vv && !vv->is_str)
      snprintf(bag, sizeof bag, "%ld", vv->val);
    llen = strlen(line);
    blen = strlen(bag);
    hit = 0;
    out[0] = 0;

    if (!is_drop) {
      /* ENSUREIN: membership probe then optional PUSHTO */
      int present = 0;
      p = bag;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        {
          size_t fl = (size_t)(p - start);
          if (fl == llen && (fl == 0 || memcmp(start, line, fl) == 0)) {
            present = 1;
            break;
          }
        }
        if (*p == '\n') p++;
      }
      if (present) {
        snprintf(out, sizeof out, "%s", bag);
        hit = 0;
      } else {
        size_t o = 0;
        if (blen == 0) {
          if (llen < sizeof out) { memcpy(out, line, llen); o = llen; }
          else { o = sizeof out - 1; memcpy(out, line, o); }
          out[o] = 0;
        } else {
          if (blen < sizeof out) { memcpy(out, bag, blen); o = blen; }
          else { o = sizeof out - 1; memcpy(out, bag, o); }
          if (o + 1 < sizeof out) out[o++] = '\n';
          if (o + llen < sizeof out) { memcpy(out + o, line, llen); o += llen; }
          else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, line, take); o += take;
          }
          out[o] = 0;
        }
        hit = 1;
      }
    } else {
      /* DROPLINEIN: drop first exact field */
      size_t o = 0;
      int first = 1;
      p = bag;
      while (*p || (p == bag && bag[0] == 0 && 0)) {
        if (!*p && p != bag) break;
        if (!bag[0]) break;
        start = p;
        while (*p && *p != '\n') p++;
        {
          size_t fl = (size_t)(p - start);
          int match = (fl == llen && (fl == 0 || memcmp(start, line, fl) == 0));
          if (match && !hit) {
            hit = 1; /* skip this field once */
          } else {
            if (!first && o + 1 < sizeof out) out[o++] = '\n';
            if (o + fl < sizeof out) { memcpy(out + o, start, fl); o += fl; }
            else if (o < sizeof out - 1) {
              size_t take = sizeof out - 1 - o;
              memcpy(out + o, start, take); o += take;
            }
            out[o] = 0;
            first = 0;
          }
        }
        if (*p == '\n') p++;
        else break;
      }
    }

    nfields = 0;
    if (out[0]) {
      p = out;
      while (*p) {
        while (*p && *p != '\n') p++;
        nfields++;
        if (*p == '\n') p++;
      }
    }
    var_set_str(vm, name, out);
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    if (is_drop) {
      var_set_num(vm, "DROPLINEIN_HIT", hit);
      var_set_num(vm, "DROPLINEIN_N", nfields);
      var_set_num(vm, "ENSUREIN_HIT", 0);
    } else {
      var_set_num(vm, "ENSUREIN_HIT", hit);
      var_set_num(vm, "ENSUREIN_N", nfields);
      var_set_num(vm, "DROPLINEIN_HIT", 0);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s hit=%ld n=%ld\n", op, name, hit, nfields);
    bump(vm);
    return 1;
  }
  /* NUMTO name — coerce named var to numeric (strtol) in place.
   * STRTO name — coerce named var to decimal string (itoa) in place.
   * NUMTO_OK = 1 if digits consumed (partial "12x"→12 OK); miss/empty → 0.
   * LAST = decimal text always; LAST_N = numeric value.
   * Usability: after BEFOREIN/BETWEENIN peels, INC without LET n = SYS NUM s glue;
   * dual of NUMF/STRF for plain vars (not object fields). */
  if (kw(&L->cur,"NUMTO")||kw(&L->cur,"INTTO")||kw(&L->cur,"ATOITO")||
      kw(&L->cur,"TONUM")||kw(&L->cur,"ASNUMTO")||kw(&L->cur,"PARSEINTO")||
      kw(&L->cur,"STRTO")||kw(&L->cur,"ITOATO")||kw(&L->cur,"TOSTR")||
      kw(&L->cur,"ASSTRTO")||kw(&L->cur,"STRINGIFY")){
    char op[24], name[48], cur[CUBALC_HOST_STR_MAX], buf[48];
    int to_str = 0, parse_ok = 0;
    long nval = 0;
    char *endp = NULL;
    Var *vv;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "STRTO") == 0 || strcmp(op, "ITOATO") == 0 ||
        strcmp(op, "TOSTR") == 0 || strcmp(op, "ASSTRTO") == 0 ||
        strcmp(op, "STRINGIFY") == 0)
      to_str = 1;
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, to_str ? "STRTO needs name — STRTO n"
                      : "NUMTO needs name — NUMTO s");
      return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    cur[0] = 0;
    vv = var_get(vm, name, 0);
    if (vv && vv->is_str)
      snprintf(cur, sizeof cur, "%s", vv->sval);
    else if (vv && !vv->is_str)
      snprintf(cur, sizeof cur, "%ld", vv->val);

    if (to_str) {
      if (vv && !vv->is_str)
        nval = vv->val;
      else {
        endp = NULL;
        nval = strtol(cur, &endp, 10);
        if (cur[0] && endp && endp != cur)
          parse_ok = 1;
        /* stringify raw text if non-numeric? keep original when already str non-num */
        if (!parse_ok && vv && vv->is_str) {
          /* keep string as-is */
          snprintf(buf, sizeof buf, "%s", cur);
          /* truncate to buf only if huge — prefer full cur via var_set_str */
          var_set_str(vm, name, cur);
          var_set_str(vm, "LAST", cur);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", cur);
          var_set_num(vm, "LAST_N", (long)strlen(cur));
          vm->last_n = (long)strlen(cur);
          var_set_num(vm, "STRTO_N", (long)strlen(cur));
          var_set_num(vm, "NUMTO_OK", 0);
          var_set_num(vm, "OK", 1);
          if (vm->trace)
            fprintf(vm->trace, "# STRTO %s (keep)\n", name);
          bump(vm);
          return 1;
        }
      }
      snprintf(buf, sizeof buf, "%ld", nval);
      var_set_str(vm, name, buf);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      var_set_num(vm, "LAST_N", nval);
      vm->last_n = nval;
      var_set_num(vm, "STRTO_N", nval);
      var_set_num(vm, "NUMTO_OK", 1);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# STRTO %s -> %s\n", name, buf);
      bump(vm);
      return 1;
    }

    /* NUMTO */
    endp = NULL;
    nval = strtol(cur, &endp, 10);
    if (cur[0] && endp && endp != cur)
      parse_ok = 1;
    else if (vv && !vv->is_str) {
      nval = vv->val;
      parse_ok = 1;
    } else {
      nval = 0;
      parse_ok = 0;
    }
    var_set_num(vm, name, nval);
    snprintf(buf, sizeof buf, "%ld", nval);
    var_set_str(vm, "LAST", buf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
    var_set_num(vm, "LAST_N", nval);
    vm->last_n = nval;
    var_set_num(vm, "NUMTO_N", nval);
    var_set_num(vm, "NUMTO_OK", parse_ok ? 1 : 0);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# NUMTO %s -> %ld ok=%d\n", name, nval, parse_ok);
    bump(vm);
    return 1;
  }
  /* LPADTO name width [pad] — left-pad named var to width in place (promotes num).
   * RPADTO name width [pad] — right-pad named var to width in place.
   * PADTO alias of LPADTO. Default pad=' '. Longer than width → truncate to width.
   * LAST = result; LAST_N = length.
   * Usability: fixed-width plate IDs after STRTO/NUMTO without LET=SYS LPAD glue;
   * dual of LPADF/RPADF for plain vars. */
  if (kw(&L->cur,"LPADTO")||kw(&L->cur,"PADTO")||kw(&L->cur,"PADLEFTTO")||
      kw(&L->cur,"STRPADTO")||kw(&L->cur,"LEFTPADTO")||
      kw(&L->cur,"RPADTO")||kw(&L->cur,"PADRIGHTTO")||kw(&L->cur,"RIGHTPADTO")){
    char op[24], name[48];
    char cur[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], pad[16];
    int is_left = 1;
    long width = 0, max_w = (long)(CUBALC_HOST_STR_MAX - 1), out_n = 0;
    char padc = ' ';
    size_t sn, need, o, take, i;
    Var *vv;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "RPADTO") == 0 || strcmp(op, "PADRIGHTTO") == 0 ||
        strcmp(op, "RIGHTPADTO") == 0)
      is_left = 0;
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, is_left ? "LPADTO needs name width [pad] — LPADTO id 6 \"0\""
                       : "RPADTO needs name width [pad] — RPADTO label 10");
      return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    if (kw(&L->cur, "TO") || kw(&L->cur, "WIDTH") || kw(&L->cur, "OF") ||
        kw(&L->cur, "BY") || kw(&L->cur, "FOR"))
      lex_next(L);
    if (!cubalc_read_num_atom(vm, L, &width)) {
      fail(vm, is_left ? "LPADTO needs width — LPADTO id 6 \"0\""
                       : "RPADTO needs width — RPADTO label 10");
      return -1;
    }
    if (width < 0) width = 0;
    if (width > max_w) width = max_w;
    pad[0] = 0;
    if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
        (L->cur.kind == TK_NUM /* allow literal? skip */)) {
      /* optional pad char — only if string/ident (not next statement keyword) */
      if (L->cur.kind == TK_STR ||
          (L->cur.kind == TK_IDENT && var_get(vm, L->cur.text, 0) &&
           var_get(vm, L->cur.text, 0)->is_str)) {
        if (resolve_str_arg(vm, L, pad, sizeof pad) == 0 && pad[0])
          padc = pad[0];
      } else if (L->cur.kind == TK_IDENT &&
                 (strcmp(L->cur.text, "WITH") == 0 ||
                  strcmp(L->cur.text, "PAD") == 0 ||
                  strcmp(L->cur.text, "CHAR") == 0 ||
                  strcmp(L->cur.text, "BY") == 0)) {
        lex_next(L);
        if (resolve_str_arg(vm, L, pad, sizeof pad) == 0 && pad[0])
          padc = pad[0];
      }
    }
    cur[0] = 0;
    vv = var_get(vm, name, 0);
    if (vv && vv->is_str)
      snprintf(cur, sizeof cur, "%s", vv->sval);
    else if (vv && !vv->is_str)
      snprintf(cur, sizeof cur, "%ld", vv->val);
    sn = strlen(cur);
    out[0] = 0;
    if ((long)sn >= width) {
      take = (size_t)width;
      if (take >= sizeof out) take = sizeof out - 1;
      memcpy(out, cur, take);
      out[take] = 0;
    } else {
      need = (size_t)width - sn;
      if (is_left) {
        o = 0;
        for (i = 0; i < need && o + 1 < sizeof out; i++) out[o++] = padc;
        take = sn;
        if (o + take >= sizeof out) take = sizeof out - 1 - o;
        if (take) memcpy(out + o, cur, take);
        o += take;
        out[o] = 0;
      } else {
        o = 0;
        take = sn;
        if (take >= sizeof out) take = sizeof out - 1;
        if (take) memcpy(out, cur, take);
        o = take;
        for (i = 0; i < need && o + 1 < sizeof out; i++) out[o++] = padc;
        out[o] = 0;
      }
    }
    out_n = (long)strlen(out);
    var_set_str(vm, name, out);
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    var_set_num(vm, "LAST_N", out_n);
    vm->last_n = out_n;
    var_set_num(vm, is_left ? "LPADTO_N" : "RPADTO_N", out_n);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s w=%ld len=%ld\n", op, name, width, out_n);
    bump(vm);
    return 1;
  }
  /* FINDLINEIN name line — 0-based exact bag field index → LAST_N (−1 miss).
   * SETLINEIN name i value — set bag field by 0-based index (extend empty if past end).
   * DROPNTHIN|DROPATIN name i — drop bag field by 0-based index · HIT 0|1.
   * INSERTLINEIN|INSLINEIN name i line — insert field at 0-based index (append if past end).
   * Usability: locate-then-update/ack/insert work bags without NTHIN+rebuild glue;
   * dual of SYS FINDLINE/SETLINE/DROPNTH/INSERTLINE for named vars. */
  if (kw(&L->cur,"FINDLINEIN")||kw(&L->cur,"LINEINDEXIN")||kw(&L->cur,"INDEXLINEIN")||
      kw(&L->cur,"BAGFINDIN")||kw(&L->cur,"FINDBAGIN")||
      kw(&L->cur,"SETLINEIN")||kw(&L->cur,"PUTLINEIN")||kw(&L->cur,"REPLACELINEIN")||
      kw(&L->cur,"BAGSETIN")||kw(&L->cur,"SETBAGIN")||
      kw(&L->cur,"DROPNTHIN")||kw(&L->cur,"DROPATIN")||kw(&L->cur,"DELETEATIN")||
      kw(&L->cur,"BAGDROPAT")||kw(&L->cur,"REMOVENTHIN")||
      kw(&L->cur,"INSERTLINEIN")||kw(&L->cur,"INSLINEIN")||kw(&L->cur,"INSERTATIN")||
      kw(&L->cur,"BAGINSERTIN")||kw(&L->cur,"SPLICEIN")){
    char op[24], name[48];
    int mode = 0; /* 0 find, 1 set, 2 dropnth, 3 insert */
    char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
    char line[CUBALC_HOST_STR_MAX], val[CUBALC_HOST_STR_MAX];
    long idx = 0, hit = 0, nfields = 0, found = -1;
    const char *p, *start;
    size_t o = 0;
    int first = 1;
    long fi = 0;
    Var *vv;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "SETLINEIN") == 0 || strcmp(op, "PUTLINEIN") == 0 ||
        strcmp(op, "REPLACELINEIN") == 0 || strcmp(op, "BAGSETIN") == 0 ||
        strcmp(op, "SETBAGIN") == 0)
      mode = 1;
    else if (strcmp(op, "DROPNTHIN") == 0 || strcmp(op, "DROPATIN") == 0 ||
             strcmp(op, "DELETEATIN") == 0 || strcmp(op, "BAGDROPAT") == 0 ||
             strcmp(op, "REMOVENTHIN") == 0)
      mode = 2;
    else if (strcmp(op, "INSERTLINEIN") == 0 || strcmp(op, "INSLINEIN") == 0 ||
             strcmp(op, "INSERTATIN") == 0 || strcmp(op, "BAGINSERTIN") == 0 ||
             strcmp(op, "SPLICEIN") == 0)
      mode = 3;
    else
      mode = 0;
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, mode == 0 ? "FINDLINEIN needs name line — FINDLINEIN work \"job\""
           : mode == 1 ? "SETLINEIN needs name i value — SETLINEIN work 0 \"x\""
           : mode == 3 ? "INSERTLINEIN needs name i line — INSERTLINEIN work 0 \"x\""
                       : "DROPNTHIN needs name i — DROPNTHIN work 0");
      return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    bag[0] = 0;
    vv = var_get(vm, name, 0);
    if (vv && vv->is_str)
      snprintf(bag, sizeof bag, "%s", vv->sval);
    else if (vv && !vv->is_str)
      snprintf(bag, sizeof bag, "%ld", vv->val);

    if (mode == 0) {
      /* FINDLINEIN */
      size_t llen;
      if (kw(&L->cur, "WITH") || kw(&L->cur, "OF") || kw(&L->cur, "LINE") ||
          kw(&L->cur, "FOR"))
        lex_next(L);
      line[0] = 0;
      if (resolve_str_arg(vm, L, line, sizeof line) != 0) {
        fail(vm, "FINDLINEIN needs line — FINDLINEIN work \"job\"");
        return -1;
      }
      llen = strlen(line);
      found = -1;
      fi = 0;
      p = bag;
      while (*p || (bag[0] && p == bag)) {
        if (!bag[0]) break;
        start = p;
        while (*p && *p != '\n') p++;
        {
          size_t fl = (size_t)(p - start);
          if (fl == llen && (fl == 0 || memcmp(start, line, fl) == 0)) {
            found = fi;
            break;
          }
        }
        fi++;
        if (*p == '\n') p++;
        else break;
      }
      var_set_num(vm, "LAST_N", found);
      vm->last_n = found;
      {
        char nb[32];
        snprintf(nb, sizeof nb, "%ld", found);
        var_set_str(vm, "LAST", nb);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
      }
      var_set_num(vm, "FINDLINEIN_N", found);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# FINDLINEIN %s -> %ld\n", name, found);
      bump(vm);
      return 1;
    }

    /* SETLINEIN / DROPNTHIN / INSERTLINEIN need index */
    if (kw(&L->cur, "AT") || kw(&L->cur, "INDEX") || kw(&L->cur, "OF") ||
        kw(&L->cur, "NTH"))
      lex_next(L);
    if (!cubalc_read_num_atom(vm, L, &idx)) {
      fail(vm, mode == 1 ? "SETLINEIN needs i — SETLINEIN work 0 \"x\""
           : mode == 3 ? "INSERTLINEIN needs i — INSERTLINEIN work 0 \"x\""
                       : "DROPNTHIN needs i — DROPNTHIN work 0");
      return -1;
    }
    if (idx < 0) idx = 0;

    if (mode == 1 || mode == 3) {
      if (kw(&L->cur, "TO") || kw(&L->cur, "WITH") || kw(&L->cur, "AS") ||
          kw(&L->cur, "VALUE") || kw(&L->cur, "LINE") || kw(&L->cur, "="))
        lex_next(L);
      val[0] = 0;
      if (resolve_str_arg(vm, L, val, sizeof val) != 0) {
        fail(vm, mode == 3 ? "INSERTLINEIN needs line — INSERTLINEIN work 0 \"x\""
                           : "SETLINEIN needs value — SETLINEIN work 0 \"x\"");
        return -1;
      }
    }

    /* rebuild bag */
    out[0] = 0;
    o = 0;
    first = 1;
    fi = 0;
    hit = 0;
    p = bag;
    if (!bag[0] && (mode == 1 || mode == 3)) {
      /* empty bag: pad empty fields then set/insert value */
      long i;
      for (i = 0; i < idx; i++) {
        if (!first && o + 1 < sizeof out) out[o++] = '\n';
        first = 0;
      }
      if (!first && o + 1 < sizeof out) out[o++] = '\n';
      {
        size_t vl = strlen(val);
        if (o + vl < sizeof out) { memcpy(out + o, val, vl); o += vl; }
        else if (o < sizeof out - 1) {
          size_t take = sizeof out - 1 - o;
          memcpy(out + o, val, take); o += take;
        }
      }
      out[o] = 0;
      hit = 1;
    } else {
      while (bag[0] && *p) {
        start = p;
        while (*p && *p != '\n') p++;
        {
          size_t fl = (size_t)(p - start);
          int drop_this = (mode == 2 && fi == idx);
          int set_this = (mode == 1 && fi == idx);
          int ins_before = (mode == 3 && fi == idx && !hit);
          if (ins_before) {
            size_t vl = strlen(val);
            if (!first && o + 1 < sizeof out) out[o++] = '\n';
            if (o + vl < sizeof out) { memcpy(out + o, val, vl); o += vl; }
            else if (o < sizeof out - 1) {
              size_t take = sizeof out - 1 - o;
              memcpy(out + o, val, take); o += take;
            }
            out[o] = 0;
            first = 0;
            hit = 1;
          }
          if (drop_this) {
            hit = 1;
          } else {
            if (!first && o + 1 < sizeof out) out[o++] = '\n';
            if (set_this) {
              size_t vl = strlen(val);
              if (o + vl < sizeof out) { memcpy(out + o, val, vl); o += vl; }
              else if (o < sizeof out - 1) {
                size_t take = sizeof out - 1 - o;
                memcpy(out + o, val, take); o += take;
              }
              hit = 1;
            } else {
              if (o + fl < sizeof out) { memcpy(out + o, start, fl); o += fl; }
              else if (o < sizeof out - 1) {
                size_t take = sizeof out - 1 - o;
                memcpy(out + o, start, take); o += take;
              }
            }
            out[o] = 0;
            first = 0;
          }
        }
        fi++;
        if (*p == '\n') p++;
        else break;
      }
      /* SETLINEIN past end: append empties then value */
      if (mode == 1 && !hit) {
        while (fi <= idx) {
          if (!first && o + 1 < sizeof out) out[o++] = '\n';
          if (fi == idx) {
            size_t vl = strlen(val);
            if (o + vl < sizeof out) { memcpy(out + o, val, vl); o += vl; }
            else if (o < sizeof out - 1) {
              size_t take = sizeof out - 1 - o;
              memcpy(out + o, val, take); o += take;
            }
            hit = 1;
          }
          out[o] = 0;
          first = 0;
          fi++;
        }
      }
      /* INSERTLINEIN past end: append (no empty pad — priority/queue tail) */
      if (mode == 3 && !hit) {
        size_t vl = strlen(val);
        if (!first && o + 1 < sizeof out) out[o++] = '\n';
        if (o + vl < sizeof out) { memcpy(out + o, val, vl); o += vl; }
        else if (o < sizeof out - 1) {
          size_t take = sizeof out - 1 - o;
          memcpy(out + o, val, take); o += take;
        }
        out[o] = 0;
        hit = 1;
      }
    }

    nfields = 0;
    if (out[0]) {
      p = out;
      while (*p) {
        while (*p && *p != '\n') p++;
        nfields++;
        if (*p == '\n') p++;
      }
    }
    var_set_str(vm, name, out);
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    var_set_num(vm, "LAST_N", mode == 2 ? hit : nfields);
    vm->last_n = mode == 2 ? hit : nfields;
    if (mode == 1) {
      var_set_num(vm, "SETLINEIN_N", nfields);
      var_set_num(vm, "SETLINEIN_HIT", hit);
      var_set_num(vm, "DROPNTHIN_HIT", 0);
      var_set_num(vm, "INSERTLINEIN_HIT", 0);
    } else if (mode == 3) {
      var_set_num(vm, "INSERTLINEIN_N", nfields);
      var_set_num(vm, "INSERTLINEIN_HIT", hit);
      var_set_num(vm, "SETLINEIN_HIT", 0);
      var_set_num(vm, "DROPNTHIN_HIT", 0);
    } else {
      var_set_num(vm, "DROPNTHIN_HIT", hit);
      var_set_num(vm, "DROPNTHIN_N", nfields);
      var_set_num(vm, "SETLINEIN_HIT", 0);
      var_set_num(vm, "INSERTLINEIN_HIT", 0);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s idx=%ld hit=%ld n=%ld\n", op, name, idx, hit, nfields);
    bump(vm);
    return 1;
  }
  /* MOVELINEIN|MOVEATIN name from to — move 0-based bag field to final index in place.
   * LAST_N / MOVELINEIN_HIT = 1 success, 0 soft miss (from OOR / empty / to<0).
   * to >= n → end. Usability: promote/demote priority queues without DROPNTHIN+INSERTLINEIN glue;
   * dual of SYS MOVELINE for named vars. */
  if (kw(&L->cur,"MOVELINEIN")||kw(&L->cur,"MOVEATIN")||kw(&L->cur,"MOVEBAGIN")||
      kw(&L->cur,"REORDERIN")||kw(&L->cur,"SHIFTTOIN")||kw(&L->cur,"PROMOTEIN")||
      kw(&L->cur,"DEMOTEIN")){
    enum { MV_MAX = 256, MV_FLEN = 192 };
    char op[24], name[48];
    char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
    char fields[MV_MAX][MV_FLEN];
    char held[MV_FLEN];
    const char *p, *start;
    long from_i = 0, to_i = 0, n = 0, hit = 0;
    size_t flen, o = 0;
    int i, first = 1;
    Var *vv;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, "MOVELINEIN needs name from to — MOVELINEIN work 2 0");
      return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    if (kw(&L->cur, "FROM") || kw(&L->cur, "AT") || kw(&L->cur, "OF") ||
        kw(&L->cur, "INDEX"))
      lex_next(L);
    if (!cubalc_read_num_atom(vm, L, &from_i)) {
      fail(vm, "MOVELINEIN needs from — MOVELINEIN work 2 0");
      return -1;
    }
    if (kw(&L->cur, "TO") || kw(&L->cur, "INTO") || kw(&L->cur, "AT") ||
        kw(&L->cur, ","))
      lex_next(L);
    if (!cubalc_read_num_atom(vm, L, &to_i)) {
      fail(vm, "MOVELINEIN needs to — MOVELINEIN work 2 0");
      return -1;
    }
    bag[0] = out[0] = 0;
    vv = var_get(vm, name, 0);
    if (vv && vv->is_str)
      snprintf(bag, sizeof bag, "%s", vv->sval);
    else if (vv && !vv->is_str)
      snprintf(bag, sizeof bag, "%ld", vv->val);
    n = 0;
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
    var_set_str(vm, name, out);
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    var_set_num(vm, "MOVELINEIN_HIT", hit);
    var_set_num(vm, "MOVELINEIN_N", hit);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s from=%ld to=%ld hit=%ld\n", op, name, from_i, to_i, hit);
    bump(vm);
    return 1;
  }
  /* COUNTIN name needle — non-overlapping needle hit count in var → LAST_N (no mutate).
   * COUNTLINEIN name line — exact bag field match count → LAST_N (no mutate).
   * Empty needle → 0. Usability: path depth / work multi-ack tallies without loop glue;
   * dual of COUNTINF (OOP) and SYS COUNTLINE / COUNTMATCH for named vars. */
  if (kw(&L->cur,"COUNTIN")||kw(&L->cur,"OCCURSIN")||kw(&L->cur,"NCOUNTIN")||
      kw(&L->cur,"TALLYIN")||kw(&L->cur,"HITCOUNTIN")||
      kw(&L->cur,"COUNTLINEIN")||kw(&L->cur,"COUNTBAGIN")||kw(&L->cur,"NLINEIN")||
      kw(&L->cur,"TALLYLINEIN")||kw(&L->cur,"BAGCOUNTIN")){
    char op[24], name[48];
    char cur[CUBALC_HOST_STR_MAX], needle[CUBALC_HOST_STR_MAX];
    int is_line = 0;
    long count = 0;
    size_t nn, cn;
    const char *p, *start;
    Var *vv;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "COUNTLINEIN") == 0 || strcmp(op, "COUNTBAGIN") == 0 ||
        strcmp(op, "NLINEIN") == 0 || strcmp(op, "TALLYLINEIN") == 0 ||
        strcmp(op, "BAGCOUNTIN") == 0)
      is_line = 1;
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, is_line ? "COUNTLINEIN needs name line — COUNTLINEIN work \"job\""
                       : "COUNTIN needs name needle — COUNTIN path \"/\"");
      return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    if (kw(&L->cur, "OF") || kw(&L->cur, "WITH") || kw(&L->cur, "NEEDLE") ||
        kw(&L->cur, "LINE") || kw(&L->cur, "FOR"))
      lex_next(L);
    needle[0] = 0;
    if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
      fail(vm, is_line ? "COUNTLINEIN needs line" : "COUNTIN needs needle");
      return -1;
    }
    cur[0] = 0;
    vv = var_get(vm, name, 0);
    if (vv && vv->is_str)
      snprintf(cur, sizeof cur, "%s", vv->sval);
    else if (vv && !vv->is_str)
      snprintf(cur, sizeof cur, "%ld", vv->val);
    nn = strlen(needle);
    cn = strlen(cur);
    count = 0;
    if (is_line) {
      /* exact bag field matches */
      p = cur;
      while (*p || (cur[0] && p == cur)) {
        if (!cur[0]) break;
        start = p;
        while (*p && *p != '\n') p++;
        {
          size_t fl = (size_t)(p - start);
          if (fl == nn && (fl == 0 || memcmp(start, needle, fl) == 0))
            count++;
        }
        if (*p == '\n') p++;
        else break;
      }
    } else {
      /* non-overlapping substring hits */
      if (nn > 0 && cn >= nn) {
        p = cur;
        while (*p) {
          const char *hit = strstr(p, needle);
          if (!hit) break;
          count++;
          p = hit + nn;
        }
      }
    }
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", count);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "LAST_N", count);
    vm->last_n = count;
    if (is_line) {
      var_set_num(vm, "COUNTLINEIN_N", count);
      var_set_num(vm, "COUNTIN_N", 0);
    } else {
      var_set_num(vm, "COUNTIN_N", count);
      var_set_num(vm, "COUNTLINEIN_N", 0);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s -> %ld\n", op, name, count);
    bump(vm);
    return 1;
  }
  /* SUMIN|MAXIN|MININ|AVGIN name — numeric bag field aggregates → LAST_N (no mutate).
   * Walks newline fields; strtol per field; skips empty/non-numeric.
   * Empty bag or no nums → 0. AVGIN is integer average (sum/count).
   * Usability: score/metric bags without EACH LINE + INC glue · dual of SYS SUM on bag. */
  if (kw(&L->cur,"SUMIN")||kw(&L->cur,"BAGSUM")||kw(&L->cur,"SUMBAGIN")||
      kw(&L->cur,"TOTALIN")||kw(&L->cur,"SUMLINESIN")||
      kw(&L->cur,"MAXIN")||kw(&L->cur,"BAGMAX")||kw(&L->cur,"MAXBAGIN")||
      kw(&L->cur,"MAXLINESIN")||
      kw(&L->cur,"MININ")||kw(&L->cur,"BAGMIN")||kw(&L->cur,"MINBAGIN")||
      kw(&L->cur,"MINLINESIN")||
      kw(&L->cur,"AVGIN")||kw(&L->cur,"BAGAVG")||kw(&L->cur,"AVGBAGIN")||
      kw(&L->cur,"MEANIN")||kw(&L->cur,"AVGLINESIN")){
    char op[24], name[48];
    char cur[CUBALC_HOST_STR_MAX];
    int mode = 0; /* 0 sum, 1 max, 2 min, 3 avg */
    long sum = 0, mx = 0, mn = 0, nnum = 0, out = 0;
    const char *p, *start;
    Var *vv;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "MAXIN") == 0 || strcmp(op, "BAGMAX") == 0 ||
        strcmp(op, "MAXBAGIN") == 0 || strcmp(op, "MAXLINESIN") == 0)
      mode = 1;
    else if (strcmp(op, "MININ") == 0 || strcmp(op, "BAGMIN") == 0 ||
             strcmp(op, "MINBAGIN") == 0 || strcmp(op, "MINLINESIN") == 0)
      mode = 2;
    else if (strcmp(op, "AVGIN") == 0 || strcmp(op, "BAGAVG") == 0 ||
             strcmp(op, "AVGBAGIN") == 0 || strcmp(op, "MEANIN") == 0 ||
             strcmp(op, "AVGLINESIN") == 0)
      mode = 3;
    else
      mode = 0;
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, mode == 1 ? "MAXIN needs name — MAXIN scores"
           : mode == 2 ? "MININ needs name — MININ scores"
           : mode == 3 ? "AVGIN needs name — AVGIN scores"
                       : "SUMIN needs name — SUMIN scores");
      return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    if (kw(&L->cur, "OF") || kw(&L->cur, "IN") || kw(&L->cur, "FROM") ||
        kw(&L->cur, "BAG"))
      lex_next(L);
    cur[0] = 0;
    vv = var_get(vm, name, 0);
    if (vv && vv->is_str)
      snprintf(cur, sizeof cur, "%s", vv->sval);
    else if (vv && !vv->is_str)
      snprintf(cur, sizeof cur, "%ld", vv->val);
    p = cur;
    while (*p || (cur[0] && p == cur)) {
      char *end = NULL;
      long v;
      size_t fl;
      if (!cur[0]) break;
      start = p;
      while (*p && *p != '\n') p++;
      fl = (size_t)(p - start);
      if (fl > 0) {
        char tmp[64];
        if (fl >= sizeof tmp) fl = sizeof tmp - 1;
        memcpy(tmp, start, fl);
        tmp[fl] = 0;
        v = strtol(tmp, &end, 10);
        if (end && end != tmp) {
          /* accept trailing junk after number only if we parsed something */
          if (nnum == 0) { mx = v; mn = v; }
          else {
            if (v > mx) mx = v;
            if (v < mn) mn = v;
          }
          sum += v;
          nnum++;
        }
      }
      if (*p == '\n') p++;
      else break;
    }
    if (nnum == 0)
      out = 0;
    else if (mode == 1) out = mx;
    else if (mode == 2) out = mn;
    else if (mode == 3) out = sum / nnum;
    else out = sum;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", out);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "LAST_N", out);
    vm->last_n = out;
    var_set_num(vm, "SUMIN_COUNT", nnum);
    var_set_num(vm, "SUMIN_SUM", sum);
    if (mode == 0) {
      var_set_num(vm, "SUMIN_N", out);
      var_set_num(vm, "MAXIN_N", 0);
      var_set_num(vm, "MININ_N", 0);
      var_set_num(vm, "AVGIN_N", 0);
    } else if (mode == 1) {
      var_set_num(vm, "MAXIN_N", out);
      var_set_num(vm, "SUMIN_N", 0);
      var_set_num(vm, "MININ_N", 0);
      var_set_num(vm, "AVGIN_N", 0);
    } else if (mode == 2) {
      var_set_num(vm, "MININ_N", out);
      var_set_num(vm, "SUMIN_N", 0);
      var_set_num(vm, "MAXIN_N", 0);
      var_set_num(vm, "AVGIN_N", 0);
    } else {
      var_set_num(vm, "AVGIN_N", out);
      var_set_num(vm, "SUMIN_N", 0);
      var_set_num(vm, "MAXIN_N", 0);
      var_set_num(vm, "MININ_N", 0);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s -> %ld (n=%ld)\n", op, name, out, nnum);
    bump(vm);
    return 1;
  }
  return 0;
}
