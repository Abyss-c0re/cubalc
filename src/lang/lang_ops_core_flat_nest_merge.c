/* CubalC flat nest sort shard */
#include "lang/cubalc_lang_ops_core_flat_nest_merge.h"
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

int cubalc_lang_ops_core_try_flat_nest_merge(VM *vm, Lex *L) {
  /* SUMMERGEOBJ|ADDFREQOBJ|MERGEKOBJ [FROM plate] nest overlay
   *   or … nest FROM srcplate [srcnest]
   * SUBVALOBJ|SUBFREQOBJ|DELTAVOBJ [FROM plate] nest overlay|FROM src [srcnest]
   * Sum-merge / subtract pure-int keys into nested object → write-back outer.
   * Not MERGEOBJ (key overlay replace) · not GETOBJ alias SUBOBJ · not OOP.
   * Soft target-nest miss → start {} then fold overlay.
   * Usability: peer nest FREQ combine/delta without GETOBJ+SUMMERGEP/SUBP+SETOBJ:
   *   SUMMERGEOBJ "freq" FROM PEER "freq"
   *   SUMMERGEOBJ "freq" FROM PEER          # same nest key on PEER
   *   SUMMERGEOBJ "freq" "{\"error\":1}"
   *   SUBVALOBJ "freq" FROM BASE "freq"
   */
  if (kw(&L->cur,"SUMMERGEOBJ") || kw(&L->cur,"ADDFREQOBJ") || kw(&L->cur,"MERGEKOBJ") ||
      kw(&L->cur,"NESTSUMMERGE") || kw(&L->cur,"MSUMMERGEOBJ") || kw(&L->cur,"COMBINEOBJ") ||
      kw(&L->cur,"FUSEOBJ") || kw(&L->cur,"SUMFREQOBJ") ||
      kw(&L->cur,"SUBVALOBJ") || kw(&L->cur,"SUBFREQOBJ") || kw(&L->cur,"DELTAVOBJ") ||
      kw(&L->cur,"NESTSUB") || kw(&L->cur,"MSUBVALOBJ") || kw(&L->cur,"SUBHISTOBJ") ||
      kw(&L->cur,"DELTAFREQOBJ") || kw(&L->cur,"NESTDELTA")) {
    char plate[CUBALC_HOST_STR_MAX], nestk[96], nest[CUBALC_HOST_STR_MAX];
    char over[CUBALC_HOST_STR_MAX], src_nestk[96];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    char src_name[96], src_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result ngr, sgr, hr, wr;
    int have_from = 0, have_src = 0, nest_hit = 0, mode = 0; /* 0 sum 1 sub */
    Var *pv;
    const char *v;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "SUBVALOBJ") == 0 || strcmp(op0, "SUBFREQOBJ") == 0 ||
        strcmp(op0, "DELTAVOBJ") == 0 || strcmp(op0, "NESTSUB") == 0 ||
        strcmp(op0, "MSUBVALOBJ") == 0 || strcmp(op0, "SUBHISTOBJ") == 0 ||
        strcmp(op0, "DELTAFREQOBJ") == 0 || strcmp(op0, "NESTDELTA") == 0)
      mode = 1;
    else
      mode = 0;

    lex_next(L);
    plate[0] = 0; nestk[0] = 0; nest[0] = 0; over[0] = 0; src_nestk[0] = 0;
    from_name[0] = 0; from_src[0] = 0; src_name[0] = 0; src_src[0] = 0;

    /* optional target FROM plate */
    if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
        kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
        snprintf(from_name, sizeof from_name, "%s", "LAST");
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        pv = var_get(vm, L->cur.text, 0);
        if (pv && pv->is_str) {
          snprintf(from_name, sizeof from_name, "%s", L->cur.text);
          snprintf(from_src, sizeof from_src, "%s", pv->sval);
          lex_next(L);
        } else if (pv) {
          snprintf(from_name, sizeof from_name, "%s", L->cur.text);
          snprintf(from_src, sizeof from_src, "%ld", pv->val);
          lex_next(L);
        } else if (resolve_str_arg(vm, L, from_src, sizeof from_src) != 0) {
          from_src[0] = 0;
        }
      } else if (resolve_str_arg(vm, L, from_src, sizeof from_src) != 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
      }
    }

    /* nest key */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(nestk, sizeof nestk, "%ld", kv);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
          kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
          kw(&L->cur,"END") || kw(&L->cur,"ASSERT") || kw(&L->cur,"LET")) {
        fail(vm, mode == 1
             ? "SUBVALOBJ nest overlay — need nest key"
             : "SUMMERGEOBJ nest overlay — need nest key");
        return -1;
      }
      if (resolve_str_arg(vm, L, nestk, sizeof nestk) != 0)
        nestk[0] = 0;
    } else {
      fail(vm, mode == 1
           ? "SUBVALOBJ nest overlay — need nest key"
           : "SUMMERGEOBJ nest overlay — need nest key");
      return -1;
    }

    if (!nestk[0]) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  mode == 1 ? "SUBVALOBJ: empty nest" : "SUMMERGEOBJ: empty nest");
      var_set_str(vm, "ERR",
                  mode == 1 ? "SUBVALOBJ: empty nest" : "SUMMERGEOBJ: empty nest");
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    /* overlay: FROM srcplate [srcnest]  OR  object/var/LAST */
    if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
        kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
        kw(&L->cur,"WITH") || kw(&L->cur,"PEERFROM") || kw(&L->cur,"SRC")) {
      lex_next(L);
      have_src = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(src_src, sizeof src_src, "%s", vm->last_str);
        snprintf(src_name, sizeof src_name, "%s", "LAST");
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        pv = var_get(vm, L->cur.text, 0);
        if (pv && pv->is_str) {
          snprintf(src_name, sizeof src_name, "%s", L->cur.text);
          snprintf(src_src, sizeof src_src, "%s", pv->sval);
          lex_next(L);
        } else if (pv) {
          snprintf(src_name, sizeof src_name, "%s", L->cur.text);
          snprintf(src_src, sizeof src_src, "%ld", pv->val);
          lex_next(L);
        } else if (resolve_str_arg(vm, L, src_src, sizeof src_src) != 0) {
          src_src[0] = 0;
        }
      } else if (resolve_str_arg(vm, L, src_src, sizeof src_src) != 0) {
        snprintf(src_src, sizeof src_src, "%s", vm->last_str);
      }
      /* optional src nest key (default same as target nest) */
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long kv = parse_expr(vm, L);
        snprintf(src_nestk, sizeof src_nestk, "%ld", kv);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (!(kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
              kw(&L->cur,"END") || kw(&L->cur,"ASSERT") || kw(&L->cur,"LET") ||
              kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM"))) {
          if (resolve_str_arg(vm, L, src_nestk, sizeof src_nestk) != 0)
            src_nestk[0] = 0;
        }
      }
      if (!src_nestk[0])
        snprintf(src_nestk, sizeof src_nestk, "%s", nestk);

      /* peel src nest as overlay; soft miss → {} */
      {
        const char *bp = src_src;
        char srcplate[CUBALC_HOST_STR_MAX];
        while (*bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\r') bp++;
        if (*bp == '{')
          snprintf(srcplate, sizeof srcplate, "%s", src_src);
        else
          snprintf(srcplate, sizeof srcplate, "%s", "{}");
        over[0] = 0;
        memset(&sgr, 0, sizeof sgr);
        if (cubalc_host_json_get_raw(srcplate, src_nestk, &sgr) == 0) {
          v = sgr.str;
          while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
          if (*v == '{') {
            if (v != sgr.str) {
              size_t n = strlen(v);
              memmove(sgr.str, v, n + 1);
            }
            snprintf(over, sizeof over, "%s", sgr.str);
          }
        }
        if (!over[0])
          snprintf(over, sizeof over, "%s", "{}");
      }
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(over, sizeof over, "%s", vm->last_str);
      lex_next(L);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (resolve_str_arg(vm, L, over, sizeof over) != 0) {
        fail(vm, mode == 1
             ? "SUBVALOBJ nest overlay — need overlay or FROM src"
             : "SUMMERGEOBJ nest overlay — need overlay or FROM src");
        return -1;
      }
    } else {
      fail(vm, mode == 1
           ? "SUBVALOBJ nest overlay — need overlay or FROM src"
           : "SUMMERGEOBJ nest overlay — need overlay or FROM src");
      return -1;
    }

    /* normalize overlay to object */
    {
      const char *bp = over;
      while (*bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\r') bp++;
      if (*bp != '{')
        snprintf(over, sizeof over, "%s", "{}");
    }

    /* load target plate */
    if (have_from) {
      const char *bp = from_src;
      while (*bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\r') bp++;
      if (*bp == '{')
        snprintf(plate, sizeof plate, "%s", from_src);
      else
        snprintf(plate, sizeof plate, "%s", "{}");
    } else {
      pv = var_get(vm, "PLATE", 0);
      if (pv && pv->is_str && pv->sval[0])
        snprintf(plate, sizeof plate, "%s", pv->sval);
      else
        snprintf(plate, sizeof plate, "%s", "{}");
    }

    /* peel target nest (miss → {}) */
    nest_hit = 0;
    nest[0] = 0;
    memset(&ngr, 0, sizeof ngr);
    if (cubalc_host_json_get_raw(plate, nestk, &ngr) == 0) {
      v = ngr.str;
      while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
      if (*v == '{') {
        if (v != ngr.str) {
          size_t n = strlen(v);
          memmove(ngr.str, v, n + 1);
        }
        snprintf(nest, sizeof nest, "%s", ngr.str);
        nest_hit = 1;
      }
    }
    if (!nest_hit)
      snprintf(nest, sizeof nest, "%s", "{}");

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_valmerge(nest, over, mode, &hr) != 0) {
      var_set_str(vm, "LAST", "{}");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "{}");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "SUMMERGEOBJ_N", 0);
      var_set_num(vm, "SUBVALOBJ_N", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err :
                  (mode == 1 ? "SUBVALOBJ: fail" : "SUMMERGEOBJ: fail"));
      var_set_str(vm, "ERR", hr.err[0] ? hr.err :
                  (mode == 1 ? "SUBVALOBJ: fail" : "SUMMERGEOBJ: fail"));
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }

    /* write-back nested result */
    snprintf(nest, sizeof nest, "%s", hr.str);
    memset(&wr, 0, sizeof wr);
    if (cubalc_host_json_set(plate, nestk, nest, 1, &wr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err :
                  (mode == 1 ? "SUBVALOBJ: write-back fail" : "SUMMERGEOBJ: write-back fail"));
      var_set_str(vm, "ERR", wr.err[0] ? wr.err :
                  (mode == 1 ? "SUBVALOBJ: write-back fail" : "SUMMERGEOBJ: write-back fail"));
      var_set_num(vm, mode == 1 ? "SUBVALOBJ_N" : "SUMMERGEOBJ_N", 0);
      bump(vm); return 1;
    }

    if (have_from && from_name[0] && strcmp(from_name, "LAST") != 0)
      var_set_str(vm, from_name, wr.str);
    else if (!have_from)
      var_set_str(vm, "PLATE", wr.str);

    var_set_str(vm, "LAST", wr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", wr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "NEST", nest);
    var_set_str(vm, "MERGED", nest);
    if (mode == 1) {
      var_set_str(vm, "SUBVALOBJ", wr.str);
      var_set_str(vm, "SUBFREQOBJ", wr.str);
      var_set_str(vm, "DELTAVOBJ", wr.str);
      var_set_str(vm, "NESTSUB", wr.str);
      var_set_num(vm, "SUBVALOBJ_N", hr.n);
      var_set_num(vm, "SUBVALOBJ_HIT", (long)hr.code);
      var_set_str(vm, "SUBVALOBJ_NEST", nestk);
      var_set_num(vm, "SUBVALOBJ_FROM", have_from ? 1 : 0);
      var_set_str(vm, "SUBVALOBJ_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "SUBVALOBJ_NEST_HIT", nest_hit ? 1 : 0);
      var_set_num(vm, "SUBVALOBJ_HAVE_SRC", have_src ? 1 : 0);
      if (have_src) {
        var_set_str(vm, "SUBVALOBJ_PEER", src_name);
        var_set_str(vm, "SUBVALOBJ_PEER_NEST", src_nestk);
      }
    } else {
      var_set_str(vm, "SUMMERGEOBJ", wr.str);
      var_set_str(vm, "ADDFREQOBJ", wr.str);
      var_set_str(vm, "MERGEKOBJ", wr.str);
      var_set_str(vm, "NESTSUMMERGE", wr.str);
      var_set_num(vm, "SUMMERGEOBJ_N", hr.n);
      var_set_num(vm, "SUMMERGEOBJ_HIT", (long)hr.code);
      var_set_str(vm, "SUMMERGEOBJ_NEST", nestk);
      var_set_num(vm, "SUMMERGEOBJ_FROM", have_from ? 1 : 0);
      var_set_str(vm, "SUMMERGEOBJ_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "SUMMERGEOBJ_NEST_HIT", nest_hit ? 1 : 0);
      var_set_num(vm, "SUMMERGEOBJ_HAVE_SRC", have_src ? 1 : 0);
      if (have_src) {
        var_set_str(vm, "SUMMERGEOBJ_PEER", src_name);
        var_set_str(vm, "SUMMERGEOBJ_PEER_NEST", src_nestk);
      }
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s nest=%s n=%ld hit=%d from=%d src=%d\n",
              mode == 1 ? "subvalobj" : "summergeobj", nestk, hr.n, hr.code,
              have_from, have_src);
    bump(vm); return 1;
  }

  return 0;
}
