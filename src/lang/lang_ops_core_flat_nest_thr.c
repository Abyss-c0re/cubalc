/* CubalC flat nest shard */
#include "lang/cubalc_lang_ops_core_flat_nest_thr.h"
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

int cubalc_lang_ops_core_try_flat_nest_thr(VM *vm, Lex *L) {
  /* THRESHOBJ|KEEPVOBJ|MINCOUNTOBJ [FROM plate] nest min —
   * keep pure-int nest keys with value >= min → write-back outer (THRESHP dual).
   * DROPZEROOBJ|KEEPNZOBJ [FROM plate] nest — drop nest keys with value == 0.
   * CAPOBJ|CLAMPOBJ [FROM plate] nest max — clamp pure-int nest values to max.
   * Soft nest miss: outer unchanged · LAST_N=0 · NEST_HIT=0.
   * Usability: nest FREQ denoise without GETOBJ+THRESHP/DROPZEROP/CAPP+SETOBJ:
   *   THRESHOBJ "freq" 5
   *   DROPZEROOBJ FROM PEER "delta"
   *   CAPOBJ "scores" 100
   */
  if (kw(&L->cur,"THRESHOBJ") || kw(&L->cur,"KEEPVOBJ") || kw(&L->cur,"MINCOUNTOBJ") ||
      kw(&L->cur,"NESTTHRESH") || kw(&L->cur,"MTHRESHOBJ") || kw(&L->cur,"KEEPMINOBJ") ||
      kw(&L->cur,"DROPZEROOBJ") || kw(&L->cur,"KEEPNZOBJ") || kw(&L->cur,"NESTDROPZERO") ||
      kw(&L->cur,"MDROPZEROOBJ") || kw(&L->cur,"NZOBJ") ||
      kw(&L->cur,"CAPOBJ") || kw(&L->cur,"CLAMPOBJ") || kw(&L->cur,"NESTCAP") ||
      kw(&L->cur,"MCAPOBJ") || kw(&L->cur,"MAXVALOBJ") || kw(&L->cur,"CLAMPVALOBJ")) {
    char plate[CUBALC_HOST_STR_MAX], nestk[96], nest[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result ngr, hr, wr;
    int have_from = 0, nest_hit = 0, mode = 0; /* 0 thresh 1 dropzero 2 cap */
    long limit = 1;
    int have_limit = 0;
    Var *pv;
    const char *v;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "DROPZEROOBJ") == 0 || strcmp(op0, "KEEPNZOBJ") == 0 ||
        strcmp(op0, "NESTDROPZERO") == 0 || strcmp(op0, "MDROPZEROOBJ") == 0 ||
        strcmp(op0, "NZOBJ") == 0)
      mode = 1;
    else if (strcmp(op0, "CAPOBJ") == 0 || strcmp(op0, "CLAMPOBJ") == 0 ||
             strcmp(op0, "NESTCAP") == 0 || strcmp(op0, "MCAPOBJ") == 0 ||
             strcmp(op0, "MAXVALOBJ") == 0 || strcmp(op0, "CLAMPVALOBJ") == 0)
      mode = 2;
    else
      mode = 0;

    lex_next(L);
    plate[0] = 0; nestk[0] = 0; nest[0] = 0;
    from_name[0] = 0; from_src[0] = 0;

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
        fail(vm, "THRESHOBJ|DROPZEROOBJ|CAPOBJ nest — need nest key");
        return -1;
      }
      if (resolve_str_arg(vm, L, nestk, sizeof nestk) != 0)
        nestk[0] = 0;
    } else {
      fail(vm, "THRESHOBJ|DROPZEROOBJ|CAPOBJ nest — need nest key");
      return -1;
    }

    /* limit for thresh/cap */
    if (mode != 1) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        limit = parse_expr(vm, L);
        have_limit = 1;
      } else if (L->cur.kind == TK_IDENT) {
        Var *dv = var_get(vm, L->cur.text, 0);
        if (dv && !dv->is_str) {
          limit = (long)dv->val;
          have_limit = 1;
          lex_next(L);
        }
      }
    }

    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                       kw(&L->cur,"PLATEFROM"))) {
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

    if (!nestk[0]) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "THRESHOBJ|CAPOBJ: empty nest");
      var_set_str(vm, "ERR", "THRESHOBJ|CAPOBJ: empty nest");
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }
    if (mode != 1 && !have_limit) {
      fail(vm, mode == 2
           ? "CAPOBJ nest max — need max int"
           : "THRESHOBJ nest min — need min int");
      return -1;
    }
    if (mode == 1)
      limit = 0;

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

    /* soft nest miss — outer unchanged (no invent empty nest) */
    if (!nest_hit) {
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "NEST", "");
      var_set_num(vm, "THRESHOBJ_N", 0);
      var_set_num(vm, "DROPZEROOBJ_N", 0);
      var_set_num(vm, "CAPOBJ_N", 0);
      var_set_num(vm, "THRESHOBJ_DROP", 0);
      var_set_num(vm, "DROPZEROOBJ_DROP", 0);
      var_set_str(vm, "THRESHOBJ_NEST", nestk);
      var_set_num(vm, "THRESHOBJ_FROM", have_from ? 1 : 0);
      var_set_str(vm, "THRESHOBJ_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "THRESHOBJ_NEST_HIT", 0);
      if (mode == 2)
        var_set_num(vm, "CAPOBJ_MAX", limit);
      else if (mode == 0)
        var_set_num(vm, "THRESHOBJ_MIN", limit);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# threshobj nest-miss nest=%s mode=%d\n", nestk, mode);
      bump(vm); return 1;
    }

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_valfilter(nest, mode, limit, &hr) != 0) {
      var_set_str(vm, "LAST", "{}");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "{}");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "THRESHOBJ_N", 0);
      var_set_num(vm, "DROPZEROOBJ_N", 0);
      var_set_num(vm, "CAPOBJ_N", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err :
                  (mode == 2 ? "CAPOBJ: fail" : mode == 1 ? "DROPZEROOBJ: fail" : "THRESHOBJ: fail"));
      var_set_str(vm, "ERR", hr.err[0] ? hr.err :
                  (mode == 2 ? "CAPOBJ: fail" : mode == 1 ? "DROPZEROOBJ: fail" : "THRESHOBJ: fail"));
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    snprintf(nest, sizeof nest, "%s", hr.str);

    memset(&wr, 0, sizeof wr);
    if (cubalc_host_json_set(plate, nestk, nest, 1, &wr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "THRESHOBJ: write-back fail");
      var_set_str(vm, "ERR", wr.err[0] ? wr.err : "THRESHOBJ: write-back fail");
      var_set_num(vm, "THRESHOBJ_N", 0);
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
    var_set_str(vm, "THRESHOBJ_NEST", nestk);
    var_set_num(vm, "THRESHOBJ_FROM", have_from ? 1 : 0);
    var_set_str(vm, "THRESHOBJ_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "THRESHOBJ_NEST_HIT", nest_hit ? 1 : 0);
    if (mode == 2) {
      var_set_str(vm, "CAPOBJ", wr.str);
      var_set_str(vm, "CLAMPOBJ", wr.str);
      var_set_num(vm, "CAPOBJ_N", hr.n);
      var_set_num(vm, "CAPOBJ_MAX", limit);
      var_set_num(vm, "CAPOBJ_KEYS", (long)hr.code);
      var_set_num(vm, "CAPOBJ_FROM", have_from ? 1 : 0);
    } else if (mode == 1) {
      var_set_str(vm, "DROPZEROOBJ", wr.str);
      var_set_str(vm, "KEEPNZOBJ", wr.str);
      var_set_num(vm, "DROPZEROOBJ_N", hr.n);
      var_set_num(vm, "DROPZEROOBJ_DROP", (long)hr.code);
      var_set_num(vm, "DROPZEROOBJ_FROM", have_from ? 1 : 0);
    } else {
      var_set_str(vm, "THRESHOBJ", wr.str);
      var_set_str(vm, "KEEPVOBJ", wr.str);
      var_set_num(vm, "THRESHOBJ_N", hr.n);
      var_set_num(vm, "THRESHOBJ_DROP", (long)hr.code);
      var_set_num(vm, "THRESHOBJ_MIN", limit);
      var_set_num(vm, "KEEPVOBJ_N", hr.n);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s nest=%s n=%ld drop/cap=%d limit=%ld from=%d\n",
              mode == 2 ? "capobj" : mode == 1 ? "dropzeroobj" : "threshobj",
              nestk, hr.n, hr.code, limit, have_from);
    bump(vm); return 1;
  }

  /* THRESHP|KEEPVP|MINCOUNTP [FROM plate] min
   * — keep pure-int keys with value >= min → plate (JSON dual of SYS THRESHKV).
   * DROPZEROP|KEEPNZP [FROM plate] — drop pure-int keys with value == 0.
   * CAPP|CLAMPP [FROM plate] max — clamp pure-int values to max (CAPKV dual).
   * Bare uses PLATE. Does not mutate source (PICKP style) — LAST = result plate.
   * LAST_N = kept|capped · THRESHP_DROP / DROPZEROP_DROP · THRESHP_MIN / CAPP_MAX.
   * Usability: denoise FREQ/score plates without TOKV+THRESHKV/DROPZERO/CAPKV+FROMKVP:
   *   THRESHP 5 FROM freq
   *   DROPZEROP FROM delta
   *   CAPP 100 FROM scores
   */
  if (kw(&L->cur,"THRESHP") || kw(&L->cur,"KEEPVP") || kw(&L->cur,"MINCOUNTP") ||
      kw(&L->cur,"MTHRESHP") || kw(&L->cur,"PLATE_THRESH") || kw(&L->cur,"KEEPMINP") ||
      kw(&L->cur,"MINFREQP") || kw(&L->cur,"DROPRAREP") || kw(&L->cur,"PRUNEP") ||
      kw(&L->cur,"DROPZEROP") || kw(&L->cur,"KEEPNZP") || kw(&L->cur,"MDROPZEROP") ||
      kw(&L->cur,"PLATE_DROPZERO") || kw(&L->cur,"NZP") || kw(&L->cur,"KEEPNZP") ||
      kw(&L->cur,"CAPP") || kw(&L->cur,"CLAMPP") || kw(&L->cur,"MCAPP") ||
      kw(&L->cur,"PLATE_CAP") || kw(&L->cur,"MAXVALP") || kw(&L->cur,"CLAMPVALP")) {
    char plate[CUBALC_HOST_STR_MAX], from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, mode = 0; /* 0 thresh 1 dropzero 2 cap */
    long limit = 1;
    int have_limit = 0;
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "DROPZEROP") == 0 || strcmp(op0, "KEEPNZP") == 0 ||
        strcmp(op0, "MDROPZEROP") == 0 || strcmp(op0, "PLATE_DROPZERO") == 0 ||
        strcmp(op0, "NZP") == 0)
      mode = 1;
    else if (strcmp(op0, "CAPP") == 0 || strcmp(op0, "CLAMPP") == 0 ||
             strcmp(op0, "MCAPP") == 0 || strcmp(op0, "PLATE_CAP") == 0 ||
             strcmp(op0, "MAXVALP") == 0 || strcmp(op0, "CLAMPVALP") == 0)
      mode = 2;
    else
      mode = 0;

    lex_next(L);
    plate[0] = 0; from_name[0] = 0; from_src[0] = 0;

    /* optional leading FROM plate */
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

    /* threshold / cap limit (required for thresh/cap; optional ignored for dropzero) */
    if (mode != 1) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        limit = parse_expr(vm, L);
        have_limit = 1;
      } else if (L->cur.kind == TK_IDENT) {
        Var *dv = var_get(vm, L->cur.text, 0);
        if (dv && !dv->is_str) {
          limit = (long)dv->val;
          have_limit = 1;
          lex_next(L);
        }
      }
    }

    /* trailing FROM plate */
    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                       kw(&L->cur,"PLATEFROM"))) {
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

    if (mode != 1 && !have_limit) {
      fail(vm, mode == 2
           ? "CAPP [FROM plate] max — need max int"
           : "THRESHP [FROM plate] min — need min int");
      return -1;
    }
    if (mode == 1)
      limit = 0;

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_valfilter(plate, mode, limit, &hr) != 0) {
      var_set_str(vm, "LAST", "{}");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "{}");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "THRESHP_N", 0);
      var_set_num(vm, "DROPZEROP_N", 0);
      var_set_num(vm, "CAPP_N", 0);
      var_set_num(vm, "THRESHP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err :
                  (mode == 2 ? "CAPP: fail" : mode == 1 ? "DROPZEROP: fail" : "THRESHP: fail"));
      var_set_str(vm, "ERR", hr.err[0] ? hr.err :
                  (mode == 2 ? "CAPP: fail" : mode == 1 ? "DROPZEROP: fail" : "THRESHP: fail"));
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    if (mode == 2) {
      var_set_str(vm, "CAPP", hr.str);
      var_set_str(vm, "CLAMPP", hr.str);
      var_set_num(vm, "CAPP_N", hr.n);
      var_set_num(vm, "CAPP_MAX", limit);
      var_set_num(vm, "CAPP_KEYS", (long)hr.code);
      var_set_num(vm, "CAPP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "CAPP_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    } else if (mode == 1) {
      var_set_str(vm, "DROPZEROP", hr.str);
      var_set_str(vm, "KEEPNZP", hr.str);
      var_set_num(vm, "DROPZEROP_N", hr.n);
      var_set_num(vm, "DROPZEROP_DROP", (long)hr.code);
      var_set_num(vm, "DROPZEROP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "DROPZEROP_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    } else {
      var_set_str(vm, "THRESHP", hr.str);
      var_set_str(vm, "KEEPVP", hr.str);
      var_set_num(vm, "THRESHP_N", hr.n);
      var_set_num(vm, "THRESHP_DROP", (long)hr.code);
      var_set_num(vm, "THRESHP_MIN", limit);
      var_set_num(vm, "KEEPVP_N", hr.n);
      var_set_num(vm, "THRESHP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "THRESHP_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%ld drop/cap=%d limit=%ld from=%d\n",
              mode == 2 ? "capp" : mode == 1 ? "dropzerop" : "threshp",
              hr.n, hr.code, limit, have_from);
    bump(vm); return 1;
  }
  /* PCTOBJ|SHAREOBJ|NESTPCT [FROM plate] nest — nest int values as %% of sum write-back (PCTP dual).
   * SCALEPOBJ|MULPOBJ|NESTSCALE [FROM plate] nest factor — multiply nest ints (SCALEP dual).
   * ADDPOBJ|OFFSETPOBJ|NESTADD [FROM plate] nest delta — offset nest ints (ADDP dual).
   * DIVPOBJ|NESTDIV [FROM plate] nest divisor — integer-divide nest ints (DIVP dual).
   * ABSPOBJ|MAGPOBJ|NESTABS [FROM plate] nest — abs nest ints (ABSP dual).
   * SIGNPOBJ|DIRPOBJ|NESTSIGN [FROM plate] nest — map nest ints to −1|0|1 (SIGNP dual).
   * Not OOP SCALEOBJ/MULOBJ/ABSOBJ/SIGNOBJ (field TIMESF/ABSF family).
   * Soft nest miss: outer unchanged · NEST_HIT=0.
   * Usability: nest FREQ rewrite without GETOBJ+PCTP/SCALEP+SETOBJ:
   *   PCTOBJ "freq"
   *   SCALEPOBJ FROM PEER "scores" 2
   *   ADDPOBJ "counts" 1
   */
  if (kw(&L->cur,"PCTOBJ") || kw(&L->cur,"SHAREOBJ") || kw(&L->cur,"NESTPCT") ||
      kw(&L->cur,"PERCENTOBJ") || kw(&L->cur,"MPCTOBJ") || kw(&L->cur,"NORMOBJ") ||
      kw(&L->cur,"SCALEPOBJ") || kw(&L->cur,"MULPOBJ") || kw(&L->cur,"NESTSCALE") ||
      kw(&L->cur,"WEIGHTPOBJ") || kw(&L->cur,"MSCALEPOBJ") || kw(&L->cur,"MULVPOBJ") ||
      kw(&L->cur,"ADDPOBJ") || kw(&L->cur,"OFFSETPOBJ") || kw(&L->cur,"NESTADD") ||
      kw(&L->cur,"ADDVPOBJ") || kw(&L->cur,"MADDPOBJ") ||
      kw(&L->cur,"DIVPOBJ") || kw(&L->cur,"NESTDIV") || kw(&L->cur,"IDIVPOBJ") ||
      kw(&L->cur,"QUOTPOBJ") || kw(&L->cur,"MDIVPOBJ") ||
      kw(&L->cur,"ABSPOBJ") || kw(&L->cur,"MAGPOBJ") || kw(&L->cur,"NESTABS") ||
      kw(&L->cur,"ABSVPOBJ") || kw(&L->cur,"MABSPOBJ") ||
      kw(&L->cur,"SIGNPOBJ") || kw(&L->cur,"DIRPOBJ") || kw(&L->cur,"NESTSIGN") ||
      kw(&L->cur,"SGNPOBJ") || kw(&L->cur,"MSIGNPOBJ")) {
    char plate[CUBALC_HOST_STR_MAX], nestk[96], nest[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result ngr, hr, wr;
    int have_from = 0, nest_hit = 0, mode = 0; /* 0 pct 1 scale 2 add 3 div 4 abs 5 sign */
    long arg = 1;
    int have_arg = 0;
    Var *pv;
    const char *v;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "SCALEPOBJ") == 0 || strcmp(op0, "MULPOBJ") == 0 ||
        strcmp(op0, "NESTSCALE") == 0 || strcmp(op0, "WEIGHTPOBJ") == 0 ||
        strcmp(op0, "MSCALEPOBJ") == 0 || strcmp(op0, "MULVPOBJ") == 0)
      mode = 1;
    else if (strcmp(op0, "ADDPOBJ") == 0 || strcmp(op0, "OFFSETPOBJ") == 0 ||
             strcmp(op0, "NESTADD") == 0 || strcmp(op0, "ADDVPOBJ") == 0 ||
             strcmp(op0, "MADDPOBJ") == 0)
      mode = 2;
    else if (strcmp(op0, "DIVPOBJ") == 0 || strcmp(op0, "NESTDIV") == 0 ||
             strcmp(op0, "IDIVPOBJ") == 0 || strcmp(op0, "QUOTPOBJ") == 0 ||
             strcmp(op0, "MDIVPOBJ") == 0)
      mode = 3;
    else if (strcmp(op0, "ABSPOBJ") == 0 || strcmp(op0, "MAGPOBJ") == 0 ||
             strcmp(op0, "NESTABS") == 0 || strcmp(op0, "ABSVPOBJ") == 0 ||
             strcmp(op0, "MABSPOBJ") == 0)
      mode = 4;
    else if (strcmp(op0, "SIGNPOBJ") == 0 || strcmp(op0, "DIRPOBJ") == 0 ||
             strcmp(op0, "NESTSIGN") == 0 || strcmp(op0, "SGNPOBJ") == 0 ||
             strcmp(op0, "MSIGNPOBJ") == 0)
      mode = 5;
    else
      mode = 0;

    lex_next(L);
    plate[0] = 0; nestk[0] = 0; nest[0] = 0;
    from_name[0] = 0; from_src[0] = 0;

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
        fail(vm, "PCTOBJ|SCALEPOBJ nest — need nest key");
        return -1;
      }
      if (resolve_str_arg(vm, L, nestk, sizeof nestk) != 0)
        nestk[0] = 0;
    } else {
      fail(vm, "PCTOBJ|SCALEPOBJ nest — need nest key");
      return -1;
    }

    /* factor/delta/divisor for scale/add/div */
    if (mode == 1 || mode == 2 || mode == 3) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        arg = parse_expr(vm, L);
        have_arg = 1;
      } else if (L->cur.kind == TK_IDENT) {
        Var *dv = var_get(vm, L->cur.text, 0);
        if (dv && !dv->is_str) {
          arg = (long)dv->val;
          have_arg = 1;
          lex_next(L);
        }
      }
    }

    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                       kw(&L->cur,"PLATEFROM"))) {
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

    if (!nestk[0]) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "PCTOBJ|SCALEPOBJ: empty nest");
      var_set_str(vm, "ERR", "PCTOBJ|SCALEPOBJ: empty nest");
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }
    if ((mode == 1 || mode == 2 || mode == 3) && !have_arg) {
      fail(vm, mode == 1
           ? "SCALEPOBJ nest factor — need int factor"
           : mode == 3
           ? "DIVPOBJ nest divisor — need int divisor"
           : "ADDPOBJ nest delta — need int delta");
      return -1;
    }
    if (mode == 0 || mode == 4 || mode == 5) arg = 0;

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

    if (!nest_hit) {
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "NEST", "");
      var_set_num(vm, "PCTOBJ_N", 0);
      var_set_num(vm, "SCALEPOBJ_N", 0);
      var_set_num(vm, "ADDPOBJ_N", 0);
      var_set_num(vm, "DIVPOBJ_N", 0);
      var_set_num(vm, "ABSPOBJ_N", 0);
      var_set_num(vm, "SIGNPOBJ_N", 0);
      var_set_str(vm, "PCTOBJ_NEST", nestk);
      var_set_num(vm, "PCTOBJ_FROM", have_from ? 1 : 0);
      var_set_str(vm, "PCTOBJ_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "PCTOBJ_NEST_HIT", 0);
      if (mode == 1) var_set_num(vm, "SCALEPOBJ_F", arg);
      else if (mode == 2) var_set_num(vm, "ADDPOBJ_D", arg);
      else if (mode == 3) var_set_num(vm, "DIVPOBJ_D", arg);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# pctobj nest-miss nest=%s mode=%d\n", nestk, mode);
      bump(vm); return 1;
    }

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_valmap(nest, mode, arg, &hr) != 0) {
      var_set_str(vm, "LAST", "{}");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "{}");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "PCTOBJ_N", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "PCTOBJ: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "PCTOBJ: fail");
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    snprintf(nest, sizeof nest, "%s", hr.str);

    memset(&wr, 0, sizeof wr);
    if (cubalc_host_json_set(plate, nestk, nest, 1, &wr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "PCTOBJ: write-back fail");
      var_set_str(vm, "ERR", wr.err[0] ? wr.err : "PCTOBJ: write-back fail");
      var_set_num(vm, "PCTOBJ_N", 0);
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
    var_set_str(vm, "PCTOBJ_NEST", nestk);
    var_set_num(vm, "PCTOBJ_FROM", have_from ? 1 : 0);
    var_set_str(vm, "PCTOBJ_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "PCTOBJ_NEST_HIT", 1);

    if (mode == 5) {
      long npos = (hr.code >> 16) & 0xff;
      long nneg = (hr.code >> 8) & 0xff;
      long nzero = hr.code & 0xff;
      var_set_str(vm, "SIGNPOBJ", wr.str);
      var_set_str(vm, "DIRPOBJ", wr.str);
      var_set_num(vm, "SIGNPOBJ_N", hr.n);
      var_set_num(vm, "SIGNPOBJ_POS", npos);
      var_set_num(vm, "SIGNPOBJ_NEG", nneg);
      var_set_num(vm, "SIGNPOBJ_ZERO", nzero);
    } else if (mode == 4) {
      var_set_str(vm, "ABSPOBJ", wr.str);
      var_set_str(vm, "MAGPOBJ", wr.str);
      var_set_num(vm, "ABSPOBJ_N", hr.n);
    } else if (mode == 3) {
      var_set_str(vm, "DIVPOBJ", wr.str);
      var_set_num(vm, "DIVPOBJ_N", hr.n);
      var_set_num(vm, "DIVPOBJ_D", arg);
    } else if (mode == 2) {
      var_set_str(vm, "ADDPOBJ", wr.str);
      var_set_str(vm, "OFFSETPOBJ", wr.str);
      var_set_num(vm, "ADDPOBJ_N", hr.n);
      var_set_num(vm, "ADDPOBJ_D", arg);
    } else if (mode == 1) {
      var_set_str(vm, "SCALEPOBJ", wr.str);
      var_set_str(vm, "MULPOBJ", wr.str);
      var_set_num(vm, "SCALEPOBJ_N", hr.n);
      var_set_num(vm, "SCALEPOBJ_F", arg);
    } else {
      var_set_str(vm, "PCTOBJ", wr.str);
      var_set_str(vm, "SHAREOBJ", wr.str);
      var_set_num(vm, "PCTOBJ_N", hr.n);
      var_set_num(vm, "PCTOBJ_SUM", (long)hr.code);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# pctobj nest=%s mode=%d n=%ld arg=%ld from=%d\n",
              nestk, mode, hr.n, arg, have_from);
    bump(vm); return 1;
  }

  /* PCTP|SHAREP [FROM plate] — rewrite pure-int values as integer %% of sum (PCTKV dual).
   * SCALEP|MULP [FROM plate] factor — multiply pure-int values (SCALEKV dual).
   * ADDP|OFFSETP [FROM plate] delta — add delta to pure-int values (ADDKV dual).
   * DIVP|IDIVP [FROM plate] divisor — integer-divide pure-int values (DIVKV dual).
   * ABSP|MAGP [FROM plate] — absolute pure-int values (ABSKV dual · rank SUBP deltas).
   * SIGNP|DIRP [FROM plate] — map pure-int to −1|0|1 (SIGNKV dual · polarity).
   * Non-int keys kept. Bare uses PLATE. Does not mutate source (PICKP style).
   * LAST = plate · LAST_N = rewritten · PCTP_SUM / SCALEP_F / ADDP_D / DIVP_D.
   * ABSP_N = negatives flipped · SIGNP_POS/NEG/ZERO class counts.
   * Usability: FREQ rewrite without TOKV+…+FROMKVP:
   *   PCTP FROM freq · SCALEP 2 · ADDP 1 · DIVP 3 FROM merged
   *   SUBP now base · ABSP FROM LAST · TOPKEYP FROM LAST
   *   SIGNP FROM LAST
   */
  if (kw(&L->cur,"PCTP") || kw(&L->cur,"SHAREP") || kw(&L->cur,"PERCENTP") ||
      kw(&L->cur,"MPCTP") || kw(&L->cur,"PLATE_PCT") || kw(&L->cur,"NORMP") ||
      kw(&L->cur,"FREQPCTP") || kw(&L->cur,"PCTFREQP") ||
      kw(&L->cur,"SCALEP") || kw(&L->cur,"MULP") || kw(&L->cur,"WEIGHTP") ||
      kw(&L->cur,"MSCALEP") || kw(&L->cur,"PLATE_SCALE") || kw(&L->cur,"MULVP") ||
      kw(&L->cur,"ADDP") || kw(&L->cur,"OFFSETP") || kw(&L->cur,"ADDVP") ||
      kw(&L->cur,"MADDP") || kw(&L->cur,"PLATE_ADD") || kw(&L->cur,"OFFSETVP") ||
      kw(&L->cur,"DIVP") || kw(&L->cur,"IDIVP") || kw(&L->cur,"QUOTP") ||
      kw(&L->cur,"MDIVP") || kw(&L->cur,"PLATE_DIV") || kw(&L->cur,"DIVVP") ||
      kw(&L->cur,"ABSP") || kw(&L->cur,"MAGP") || kw(&L->cur,"MABSP") ||
      kw(&L->cur,"PLATE_ABS") || kw(&L->cur,"ABSVP") || kw(&L->cur,"MAGVP") ||
      kw(&L->cur,"SIGNP") || kw(&L->cur,"DIRP") || kw(&L->cur,"SGNP") ||
      kw(&L->cur,"MSIGNP") || kw(&L->cur,"PLATE_SIGN") || kw(&L->cur,"DIRVP")) {
    char plate[CUBALC_HOST_STR_MAX], from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, mode = 0; /* 0 pct 1 scale 2 add 3 div 4 abs 5 sign */
    long arg = 1;
    int have_arg = 0;
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "SCALEP") == 0 || strcmp(op0, "MULP") == 0 ||
        strcmp(op0, "WEIGHTP") == 0 || strcmp(op0, "MSCALEP") == 0 ||
        strcmp(op0, "PLATE_SCALE") == 0 || strcmp(op0, "MULVP") == 0)
      mode = 1;
    else if (strcmp(op0, "ADDP") == 0 || strcmp(op0, "OFFSETP") == 0 ||
             strcmp(op0, "ADDVP") == 0 || strcmp(op0, "MADDP") == 0 ||
             strcmp(op0, "PLATE_ADD") == 0 || strcmp(op0, "OFFSETVP") == 0)
      mode = 2;
    else if (strcmp(op0, "DIVP") == 0 || strcmp(op0, "IDIVP") == 0 ||
             strcmp(op0, "QUOTP") == 0 || strcmp(op0, "MDIVP") == 0 ||
             strcmp(op0, "PLATE_DIV") == 0 || strcmp(op0, "DIVVP") == 0)
      mode = 3;
    else if (strcmp(op0, "ABSP") == 0 || strcmp(op0, "MAGP") == 0 ||
             strcmp(op0, "MABSP") == 0 || strcmp(op0, "PLATE_ABS") == 0 ||
             strcmp(op0, "ABSVP") == 0 || strcmp(op0, "MAGVP") == 0)
      mode = 4;
    else if (strcmp(op0, "SIGNP") == 0 || strcmp(op0, "DIRP") == 0 ||
             strcmp(op0, "SGNP") == 0 || strcmp(op0, "MSIGNP") == 0 ||
             strcmp(op0, "PLATE_SIGN") == 0 || strcmp(op0, "DIRVP") == 0)
      mode = 5;
    else
      mode = 0;

    lex_next(L);
    plate[0] = 0; from_name[0] = 0; from_src[0] = 0;

    /* optional leading FROM plate */
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

    /* factor/delta/divisor for scale/add/div; PCT/ABS/SIGN have no arg */
    if (mode == 1 || mode == 2 || mode == 3) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        arg = parse_expr(vm, L);
        have_arg = 1;
      } else if (L->cur.kind == TK_IDENT) {
        Var *dv = var_get(vm, L->cur.text, 0);
        if (dv && !dv->is_str) {
          arg = (long)dv->val;
          have_arg = 1;
          lex_next(L);
        }
      }
    }

    /* trailing FROM plate */
    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                       kw(&L->cur,"PLATEFROM"))) {
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

    if ((mode == 1 || mode == 2 || mode == 3) && !have_arg) {
      fail(vm, mode == 1
           ? "SCALEP [FROM plate] factor — need int factor"
           : mode == 3
           ? "DIVP [FROM plate] divisor — need int divisor"
           : "ADDP [FROM plate] delta — need int delta");
      return -1;
    }
    if (mode == 0 || mode == 4 || mode == 5) arg = 0;

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_valmap(plate, mode, arg, &hr) != 0) {
      var_set_str(vm, "LAST", "{}");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "{}");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "PCTP_N", 0);
      var_set_num(vm, "SCALEP_N", 0);
      var_set_num(vm, "ADDP_N", 0);
      var_set_num(vm, "DIVP_N", 0);
      var_set_num(vm, "ABSP_N", 0);
      var_set_num(vm, "SIGNP_N", 0);
      var_set_num(vm, "PCTP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err :
                  (mode == 5 ? "SIGNP: fail" : mode == 4 ? "ABSP: fail" :
                   mode == 3 ? "DIVP: fail" : mode == 2 ? "ADDP: fail" :
                   mode == 1 ? "SCALEP: fail" : "PCTP: fail"));
      var_set_str(vm, "ERR", hr.err[0] ? hr.err :
                  (mode == 5 ? "SIGNP: fail" : mode == 4 ? "ABSP: fail" :
                   mode == 3 ? "DIVP: fail" : mode == 2 ? "ADDP: fail" :
                   mode == 1 ? "SCALEP: fail" : "PCTP: fail"));
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    if (mode == 5) {
      long npos = (hr.code >> 16) & 0xff;
      long nneg = (hr.code >> 8) & 0xff;
      long nzero = hr.code & 0xff;
      var_set_str(vm, "SIGNP", hr.str);
      var_set_str(vm, "DIRP", hr.str);
      var_set_num(vm, "SIGNP_N", hr.n);
      var_set_num(vm, "SIGNP_POS", npos);
      var_set_num(vm, "SIGNP_NEG", nneg);
      var_set_num(vm, "SIGNP_ZERO", nzero);
      var_set_num(vm, "DIRP_N", hr.n);
      var_set_num(vm, "SIGNP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "SIGNP_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    } else if (mode == 4) {
      var_set_str(vm, "ABSP", hr.str);
      var_set_str(vm, "MAGP", hr.str);
      var_set_num(vm, "ABSP_N", hr.n);
      var_set_num(vm, "ABSP_KEYS", (long)hr.code);
      var_set_num(vm, "MAGP_N", hr.n);
      var_set_num(vm, "ABSP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "ABSP_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    } else if (mode == 3) {
      var_set_str(vm, "DIVP", hr.str);
      var_set_str(vm, "IDIVP", hr.str);
      var_set_num(vm, "DIVP_N", hr.n);
      var_set_num(vm, "DIVP_D", arg);
      var_set_num(vm, "DIVP_KEYS", (long)hr.code);
      var_set_num(vm, "DIVP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "DIVP_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    } else if (mode == 2) {
      var_set_str(vm, "ADDP", hr.str);
      var_set_str(vm, "OFFSETP", hr.str);
      var_set_num(vm, "ADDP_N", hr.n);
      var_set_num(vm, "ADDP_D", arg);
      var_set_num(vm, "ADDP_KEYS", (long)hr.code);
      var_set_num(vm, "ADDP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "ADDP_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    } else if (mode == 1) {
      var_set_str(vm, "SCALEP", hr.str);
      var_set_str(vm, "MULP", hr.str);
      var_set_num(vm, "SCALEP_N", hr.n);
      var_set_num(vm, "SCALEP_F", arg);
      var_set_num(vm, "SCALEP_KEYS", (long)hr.code);
      var_set_num(vm, "SCALEP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "SCALEP_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    } else {
      var_set_str(vm, "PCTP", hr.str);
      var_set_str(vm, "SHAREP", hr.str);
      var_set_num(vm, "PCTP_N", hr.n);
      var_set_num(vm, "PCTP_SUM", (long)hr.code);
      var_set_num(vm, "SHAREP_SUM", (long)hr.code);
      var_set_num(vm, "PCTP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "PCTP_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%ld arg=%ld code=%d from=%d\n",
              mode == 5 ? "signp" : mode == 4 ? "absp" :
              mode == 3 ? "divp" : mode == 2 ? "addp" : mode == 1 ? "scalep" : "pctp",
              hr.n, arg, hr.code, have_from);
    bump(vm); return 1;
  }
  /* SUMMERGEP|ADDFREQP|MERGEKP a b — sum pure-int keys across two plates (MERGEKV dual).
   * SUBP|SUBVALP|DELTAVP a b — a−b by pure-int key (DIFFKV dual; not DIFFP key-bag).
   * Keys only in b: SUM appends as-is · SUB appends as −b. Non-int in a kept.
   * LAST = plate · LAST_N = key count · *_HIT = shared int keys combined.
   * Usability: peer FREQ combine/delta without TOKV+MERGEKV/DIFFKV+FROMKVP:
   *   SUMMERGEP PLATE PEER
   *   SUBP now baseline
   *   DIVP 2 FROM LAST
   */
  if (kw(&L->cur,"SUMMERGEP") || kw(&L->cur,"ADDFREQP") || kw(&L->cur,"MERGEKP") ||
      kw(&L->cur,"MSUMMERGEP") || kw(&L->cur,"PLATE_SUMMERGE") || kw(&L->cur,"KVADDP") ||
      kw(&L->cur,"COMBINEP") || kw(&L->cur,"FUSEP") || kw(&L->cur,"SUMFREQP") ||
      kw(&L->cur,"SUBP") || kw(&L->cur,"SUBVALP") || kw(&L->cur,"DELTAVP") ||
      kw(&L->cur,"MSUBP") || kw(&L->cur,"PLATE_SUB") || kw(&L->cur,"SUBFREQP") ||
      kw(&L->cur,"DELTAFREQP") || kw(&L->cur,"SUBHISTP")) {
    char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
    char aname[96], bname[96];
    cubalc_host_result hr;
    int mode = 0; /* 0 sum 1 sub */
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "SUBP") == 0 || strcmp(op0, "SUBVALP") == 0 ||
        strcmp(op0, "DELTAVP") == 0 || strcmp(op0, "MSUBP") == 0 ||
        strcmp(op0, "PLATE_SUB") == 0 || strcmp(op0, "SUBFREQP") == 0 ||
        strcmp(op0, "DELTAFREQP") == 0 || strcmp(op0, "SUBHISTP") == 0)
      mode = 1;
    else
      mode = 0;

    lex_next(L);
    a[0] = 0; b[0] = 0; aname[0] = 0; bname[0] = 0;

#define CUBALC_RESOLVE_PLATE_ARG2(dst, dname) do { \
      (dst)[0] = 0; (dname)[0] = 0; \
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) { \
        snprintf((dname), sizeof(dname), "%s", "LAST"); \
        snprintf((dst), sizeof(dst), "%s", vm->last_str); \
        lex_next(L); \
      } else if (L->cur.kind == TK_IDENT) { \
        pv = var_get(vm, L->cur.text, 0); \
        if (pv && pv->is_str) { \
          snprintf((dname), sizeof(dname), "%s", L->cur.text); \
          snprintf((dst), sizeof(dst), "%s", pv->sval); \
          lex_next(L); \
        } else if (pv) { \
          snprintf((dname), sizeof(dname), "%s", L->cur.text); \
          snprintf((dst), sizeof(dst), "%ld", pv->val); \
          lex_next(L); \
        } else if (resolve_str_arg(vm, L, (dst), sizeof(dst)) != 0) { \
          (dst)[0] = 0; \
        } \
      } else if (resolve_str_arg(vm, L, (dst), sizeof(dst)) != 0) { \
        snprintf((dst), sizeof(dst), "%s", vm->last_str); \
      } \
      { const char *bp = (dst); \
        while (*bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\r') bp++; \
        if (*bp != '{') snprintf((dst), sizeof(dst), "%s", "{}"); \
      } \
    } while (0)

    CUBALC_RESOLVE_PLATE_ARG2(a, aname);
    CUBALC_RESOLVE_PLATE_ARG2(b, bname);
#undef CUBALC_RESOLVE_PLATE_ARG2

    if (!aname[0] && !bname[0] && (!a[0] || strcmp(a, "{}") == 0) &&
        (!b[0] || strcmp(b, "{}") == 0)) {
      fail(vm, mode == 1
           ? "SUBP a b — need two plates (DIFFKV dual; not DIFFP key bag)"
           : "SUMMERGEP a b — need two plates (MERGEKV dual)");
      return -1;
    }

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_valmerge(a, b, mode, &hr) != 0) {
      var_set_str(vm, "LAST", "{}");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "{}");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "SUMMERGEP_N", 0);
      var_set_num(vm, "SUBP_N", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err :
                  (mode == 1 ? "SUBP: fail" : "SUMMERGEP: fail"));
      var_set_str(vm, "ERR", hr.err[0] ? hr.err :
                  (mode == 1 ? "SUBP: fail" : "SUMMERGEP: fail"));
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    if (mode == 1) {
      var_set_str(vm, "SUBP", hr.str);
      var_set_str(vm, "SUBVALP", hr.str);
      var_set_str(vm, "DELTAVP", hr.str);
      var_set_num(vm, "SUBP_N", hr.n);
      var_set_num(vm, "SUBP_HIT", (long)hr.code);
      var_set_str(vm, "SUBP_A", aname);
      var_set_str(vm, "SUBP_B", bname);
    } else {
      var_set_str(vm, "SUMMERGEP", hr.str);
      var_set_str(vm, "ADDFREQP", hr.str);
      var_set_str(vm, "MERGEKP", hr.str);
      var_set_num(vm, "SUMMERGEP_N", hr.n);
      var_set_num(vm, "SUMMERGEP_HIT", (long)hr.code);
      var_set_str(vm, "SUMMERGEP_A", aname);
      var_set_str(vm, "SUMMERGEP_B", bname);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%ld hit=%d a=%s b=%s\n",
              mode == 1 ? "subp" : "summergep", hr.n, hr.code,
              aname[0] ? aname : "?", bname[0] ? bname : "?");
    bump(vm); return 1;
  }
  /* KEEPKEYOBJ|GREPKEYOBJ|NESTKEEPKEY [FROM plate] nest needle —
   * keep nest keys containing needle → write-back outer (KEEPKEYP dual).
   * DROPKEYOBJ|GREPVKEYOBJ|NESTDROPKEY [FROM plate] nest needle — drop matching keys.
   * KEEPKEYOBJI / DROPKEYOBJI — case-insensitive.
   * Complements PICKOBJ (exact keys) with substring key filter on nests.
   * Soft nest miss: outer unchanged · NEST_HIT=0.
   * Usability: nest FREQ key pattern without GETOBJ+KEEPKEYP+SETOBJ:
   *   KEEPKEYOBJ "freq" "error"
   *   DROPKEYOBJ FROM PEER "cfg" "tmp"
   *   KEEPKEYOBJI "meta" "Warn"
   */
  if (kw(&L->cur,"KEEPKEYOBJ") || kw(&L->cur,"GREPKEYOBJ") || kw(&L->cur,"NESTKEEPKEY") ||
      kw(&L->cur,"FILTERKEYOBJ") || kw(&L->cur,"MKEEPKEYOBJ") || kw(&L->cur,"KEYGREPOBJ") ||
      kw(&L->cur,"DROPKEYOBJ") || kw(&L->cur,"GREPVKEYOBJ") || kw(&L->cur,"NESTDROPKEY") ||
      kw(&L->cur,"RMKEYOBJ") || kw(&L->cur,"MDROPKEYOBJ") ||
      kw(&L->cur,"KEEPKEYOBJI") || kw(&L->cur,"GREPKEYOBJI") || kw(&L->cur,"FILTERKEYOBJI") ||
      kw(&L->cur,"DROPKEYOBJI") || kw(&L->cur,"GREPVKEYOBJI") || kw(&L->cur,"RMKEYOBJI")) {
    char plate[CUBALC_HOST_STR_MAX], nestk[96], nest[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX], needle[256];
    cubalc_host_result ngr, hr, wr;
    int have_from = 0, nest_hit = 0, invert = 0, icase = 0;
    Var *pv;
    const char *v;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "DROPKEYOBJ") == 0 || strcmp(op0, "GREPVKEYOBJ") == 0 ||
        strcmp(op0, "NESTDROPKEY") == 0 || strcmp(op0, "RMKEYOBJ") == 0 ||
        strcmp(op0, "MDROPKEYOBJ") == 0 || strcmp(op0, "DROPKEYOBJI") == 0 ||
        strcmp(op0, "GREPVKEYOBJI") == 0 || strcmp(op0, "RMKEYOBJI") == 0)
      invert = 1;
    if (strcmp(op0, "KEEPKEYOBJI") == 0 || strcmp(op0, "GREPKEYOBJI") == 0 ||
        strcmp(op0, "FILTERKEYOBJI") == 0 || strcmp(op0, "DROPKEYOBJI") == 0 ||
        strcmp(op0, "GREPVKEYOBJI") == 0 || strcmp(op0, "RMKEYOBJI") == 0)
      icase = 1;

    lex_next(L);
    plate[0] = 0; nestk[0] = 0; nest[0] = 0;
    from_name[0] = 0; from_src[0] = 0; needle[0] = 0;

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
        fail(vm, "KEEPKEYOBJ|DROPKEYOBJ nest needle — need nest key");
        return -1;
      }
      if (resolve_str_arg(vm, L, nestk, sizeof nestk) != 0)
        nestk[0] = 0;
    } else {
      fail(vm, "KEEPKEYOBJ|DROPKEYOBJ nest needle — need nest key");
      return -1;
    }

    /* needle */
    if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT || L->cur.kind == TK_NUM ||
        L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
           kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
           kw(&L->cur,"END") || kw(&L->cur,"ASSERT") || kw(&L->cur,"LET"))) {
        /* empty needle allowed via missing — treat as "" */
      } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(needle, sizeof needle, "%ld", nv);
      } else if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
        needle[0] = 0;
      }
    }

    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                       kw(&L->cur,"PLATEFROM"))) {
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

    if (!nestk[0]) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "KEEPKEYOBJ: empty nest");
      var_set_str(vm, "ERR", "KEEPKEYOBJ: empty nest");
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

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

    if (!nest_hit) {
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "NEST", "");
      var_set_num(vm, "KEEPKEYOBJ_N", 0);
      var_set_num(vm, "DROPKEYOBJ_N", 0);
      var_set_num(vm, "KEEPKEYOBJ_DROP", 0);
      var_set_str(vm, "KEEPKEYOBJ_NEST", nestk);
      var_set_str(vm, "KEEPKEYOBJ_NEEDLE", needle);
      var_set_num(vm, "KEEPKEYOBJ_FROM", have_from ? 1 : 0);
      var_set_str(vm, "KEEPKEYOBJ_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "KEEPKEYOBJ_NEST_HIT", 0);
      var_set_num(vm, "KEEPKEYOBJ_ICASE", icase ? 1 : 0);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# keepkeyobj nest-miss nest=%s\n", nestk);
      bump(vm); return 1;
    }

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_keygrep(nest, needle, invert, icase, &hr) != 0) {
      var_set_str(vm, "LAST", "{}");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "{}");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "KEEPKEYOBJ_N", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err :
                  (invert ? "DROPKEYOBJ: fail" : "KEEPKEYOBJ: fail"));
      var_set_str(vm, "ERR", hr.err[0] ? hr.err :
                  (invert ? "DROPKEYOBJ: fail" : "KEEPKEYOBJ: fail"));
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    snprintf(nest, sizeof nest, "%s", hr.str);

    memset(&wr, 0, sizeof wr);
    if (cubalc_host_json_set(plate, nestk, nest, 1, &wr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "KEEPKEYOBJ: write-back fail");
      var_set_str(vm, "ERR", wr.err[0] ? wr.err : "KEEPKEYOBJ: write-back fail");
      var_set_num(vm, "KEEPKEYOBJ_N", 0);
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
    var_set_str(vm, "KEEPKEYOBJ_NEST", nestk);
    var_set_str(vm, "KEEPKEYOBJ_NEEDLE", needle);
    var_set_num(vm, "KEEPKEYOBJ_FROM", have_from ? 1 : 0);
    var_set_str(vm, "KEEPKEYOBJ_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "KEEPKEYOBJ_NEST_HIT", 1);
    var_set_num(vm, "KEEPKEYOBJ_ICASE", icase ? 1 : 0);
    if (invert) {
      var_set_str(vm, "DROPKEYOBJ", wr.str);
      var_set_str(vm, "GREPVKEYOBJ", wr.str);
      var_set_num(vm, "DROPKEYOBJ_N", hr.n);
      var_set_num(vm, "DROPKEYOBJ_DROP", (long)hr.code);
      var_set_num(vm, "DROPKEYOBJ_FROM", have_from ? 1 : 0);
      var_set_str(vm, "DROPKEYOBJ_NEEDLE", needle);
    } else {
      var_set_str(vm, "KEEPKEYOBJ", wr.str);
      var_set_str(vm, "GREPKEYOBJ", wr.str);
      var_set_num(vm, "KEEPKEYOBJ_N", hr.n);
      var_set_num(vm, "KEEPKEYOBJ_DROP", (long)hr.code);
      var_set_num(vm, "GREPKEYOBJ_N", hr.n);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s nest=%s n=%ld drop=%d needle=%s icase=%d from=%d\n",
              invert ? "dropkeyobj" : "keepkeyobj", nestk, hr.n, hr.code, needle,
              icase, have_from);
    bump(vm); return 1;
  }

  /* KEEPKEYP|GREPKEYP [FROM plate] needle — keep keys containing needle (KEEPKEY dual).
   * DROPKEYP|GREPVKEYP [FROM plate] needle — drop matching keys (DROPKEY dual).
   * KEEPKEYPI|GREPKEYPI / DROPKEYPI — case-insensitive key match.
   * Complements PICKP (exact keys) with substring key filter.
   * Empty needle: keep all / drop all (invert). Bare uses PLATE. No source mutate.
   * LAST = plate · LAST_N = kept · KEEPKEYP_DROP = removed.
   * Usability: FREQ key pattern without TOKV+KEEPKEY+FROMKVP or multi PICKP:
   *   KEEPKEYP "error" FROM logs
   *   DROPKEYP "tmp"
   *   KEEPKEYPI "Warn" FROM PEER
   */
  if (kw(&L->cur,"KEEPKEYP") || kw(&L->cur,"GREPKEYP") || kw(&L->cur,"FILTERKEYP") ||
      kw(&L->cur,"KEYGREPP") || kw(&L->cur,"MKEEPKEYP") || kw(&L->cur,"PLATE_KEEPKEY") ||
      kw(&L->cur,"MATCHKEYP") ||
      kw(&L->cur,"DROPKEYP") || kw(&L->cur,"GREPVKEYP") || kw(&L->cur,"RMKEYP") ||
      kw(&L->cur,"MDROPKEYP") || kw(&L->cur,"PLATE_DROPKEY") ||
      kw(&L->cur,"KEEPKEYPI") || kw(&L->cur,"GREPKEYPI") || kw(&L->cur,"FILTERKEYPI") ||
      kw(&L->cur,"DROPKEYPI") || kw(&L->cur,"GREPVKEYPI") || kw(&L->cur,"RMKEYPI")) {
    char plate[CUBALC_HOST_STR_MAX], from_name[96], from_src[CUBALC_HOST_STR_MAX];
    char needle[256];
    cubalc_host_result hr;
    int have_from = 0, invert = 0, icase = 0;
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "DROPKEYP") == 0 || strcmp(op0, "GREPVKEYP") == 0 ||
        strcmp(op0, "RMKEYP") == 0 || strcmp(op0, "MDROPKEYP") == 0 ||
        strcmp(op0, "PLATE_DROPKEY") == 0 || strcmp(op0, "DROPKEYPI") == 0 ||
        strcmp(op0, "GREPVKEYPI") == 0 || strcmp(op0, "RMKEYPI") == 0)
      invert = 1;
    if (strcmp(op0, "KEEPKEYPI") == 0 || strcmp(op0, "GREPKEYPI") == 0 ||
        strcmp(op0, "FILTERKEYPI") == 0 || strcmp(op0, "DROPKEYPI") == 0 ||
        strcmp(op0, "GREPVKEYPI") == 0 || strcmp(op0, "RMKEYPI") == 0)
      icase = 1;

    lex_next(L);
    plate[0] = 0; from_name[0] = 0; from_src[0] = 0; needle[0] = 0;

    /* optional leading FROM plate */
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

    /* needle (required unless empty string) */
    if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT || L->cur.kind == TK_NUM ||
        L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(needle, sizeof needle, "%ld", nv);
      } else if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
        needle[0] = 0;
      }
    }

    /* trailing FROM plate */
    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                       kw(&L->cur,"PLATEFROM"))) {
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

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_keygrep(plate, needle, invert, icase, &hr) != 0) {
      var_set_str(vm, "LAST", "{}");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "{}");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "KEEPKEYP_N", 0);
      var_set_num(vm, "DROPKEYP_N", 0);
      var_set_num(vm, "KEEPKEYP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err :
                  (invert ? "DROPKEYP: fail" : "KEEPKEYP: fail"));
      var_set_str(vm, "ERR", hr.err[0] ? hr.err :
                  (invert ? "DROPKEYP: fail" : "KEEPKEYP: fail"));
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    if (invert) {
      var_set_str(vm, "DROPKEYP", hr.str);
      var_set_str(vm, "GREPVKEYP", hr.str);
      var_set_num(vm, "DROPKEYP_N", hr.n);
      var_set_num(vm, "DROPKEYP_DROP", (long)hr.code);
      var_set_num(vm, "DROPKEYP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "DROPKEYP_NEEDLE", needle);
      var_set_str(vm, "DROPKEYP_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    } else {
      var_set_str(vm, "KEEPKEYP", hr.str);
      var_set_str(vm, "GREPKEYP", hr.str);
      var_set_num(vm, "KEEPKEYP_N", hr.n);
      var_set_num(vm, "KEEPKEYP_DROP", (long)hr.code);
      var_set_num(vm, "GREPKEYP_N", hr.n);
      var_set_num(vm, "KEEPKEYP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "KEEPKEYP_NEEDLE", needle);
      var_set_str(vm, "KEEPKEYP_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    }
    var_set_num(vm, "KEEPKEYP_ICASE", icase ? 1 : 0);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%ld drop=%d needle=%s icase=%d from=%d\n",
              invert ? "dropkeyp" : "keepkeyp", hr.n, hr.code, needle, icase,
              have_from);
    bump(vm); return 1;
  }
  return 0;
}
