/* CubalC flat nest shard */
#include "lang/cubalc_lang_ops_core_flat_nest_top.h"
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

int cubalc_lang_ops_core_try_flat_nest_top(VM *vm, Lex *L) {
  /* TOPNOBJ|TOPNKEYOBJ|NESTTOPN [FROM plate] nest n —
   * nest of top n pure-int keys by value DESC → write-back (TOPNP dual).
   * BOTNOBJ|BOTNKEYOBJ|NESTBOTN [FROM plate] nest n — bottom n ASC.
   * Soft nest miss: outer unchanged · NEST_HIT=0.
   * Usability: top nest FREQ without GETOBJ+TOPNP+SETOBJ:
   *   TOPNOBJ "freq" 3
   *   BOTNOBJ FROM PEER "scores" 2
   */
  if (kw(&L->cur,"TOPNOBJ") || kw(&L->cur,"TOPNKEYOBJ") || kw(&L->cur,"NESTTOPN") ||
      kw(&L->cur,"HEADNOBJ") || kw(&L->cur,"MTOPNOBJ") || kw(&L->cur,"TAKEBYVOBJ") ||
      kw(&L->cur,"BOTNOBJ") || kw(&L->cur,"BOTNKEYOBJ") || kw(&L->cur,"NESTBOTN") ||
      kw(&L->cur,"TAILNOBJ") || kw(&L->cur,"MBOTNOBJ") || kw(&L->cur,"BOTTAKEOBJ")) {
    char plate[CUBALC_HOST_STR_MAX], nestk[96], nest[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result ngr, hr, wr;
    int have_from = 0, nest_hit = 0, want_bot = 0, have_n = 0;
    long ntake = 1;
    Var *pv;
    const char *v;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "BOTNOBJ") == 0 || strcmp(op0, "BOTNKEYOBJ") == 0 ||
        strcmp(op0, "NESTBOTN") == 0 || strcmp(op0, "TAILNOBJ") == 0 ||
        strcmp(op0, "MBOTNOBJ") == 0 || strcmp(op0, "BOTTAKEOBJ") == 0)
      want_bot = 1;

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
        fail(vm, "TOPNOBJ|BOTNOBJ nest n — need nest key");
        return -1;
      }
      if (resolve_str_arg(vm, L, nestk, sizeof nestk) != 0)
        nestk[0] = 0;
    } else {
      fail(vm, "TOPNOBJ|BOTNOBJ nest n — need nest key");
      return -1;
    }

    /* count n */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      ntake = parse_expr(vm, L);
      have_n = 1;
    } else if (L->cur.kind == TK_IDENT) {
      Var *dv = var_get(vm, L->cur.text, 0);
      if (dv && !dv->is_str) {
        ntake = (long)dv->val;
        have_n = 1;
        lex_next(L);
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
      var_set_str(vm, "LAST_ERR", "TOPNOBJ: empty nest");
      var_set_str(vm, "ERR", "TOPNOBJ: empty nest");
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }
    if (!have_n) {
      fail(vm, want_bot
           ? "BOTNOBJ nest n — need count"
           : "TOPNOBJ nest n — need count");
      return -1;
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
      var_set_num(vm, "TOPNOBJ_N", 0);
      var_set_num(vm, "BOTNOBJ_N", 0);
      var_set_num(vm, "TOPNOBJ_CAND", 0);
      var_set_str(vm, "TOPNOBJ_NEST", nestk);
      var_set_num(vm, "TOPNOBJ_FROM", have_from ? 1 : 0);
      var_set_str(vm, "TOPNOBJ_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "TOPNOBJ_NEST_HIT", 0);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# topnobj nest-miss nest=%s\n", nestk);
      bump(vm); return 1;
    }

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_topn(nest, ntake, want_bot, &hr) != 0) {
      var_set_str(vm, "LAST", "{}");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "{}");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "TOPNOBJ_N", 0);
      var_set_num(vm, "BOTNOBJ_N", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err :
                  (want_bot ? "BOTNOBJ: fail" : "TOPNOBJ: fail"));
      var_set_str(vm, "ERR", hr.err[0] ? hr.err :
                  (want_bot ? "BOTNOBJ: fail" : "TOPNOBJ: fail"));
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    snprintf(nest, sizeof nest, "%s", hr.str);

    memset(&wr, 0, sizeof wr);
    if (cubalc_host_json_set(plate, nestk, nest, 1, &wr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "TOPNOBJ: write-back fail");
      var_set_str(vm, "ERR", wr.err[0] ? wr.err : "TOPNOBJ: write-back fail");
      var_set_num(vm, "TOPNOBJ_N", 0);
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
    var_set_str(vm, "TOPNOBJ_NEST", nestk);
    var_set_num(vm, "TOPNOBJ_FROM", have_from ? 1 : 0);
    var_set_str(vm, "TOPNOBJ_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "TOPNOBJ_NEST_HIT", 1);
    if (want_bot) {
      var_set_str(vm, "BOTNOBJ", wr.str);
      var_set_str(vm, "BOTNKEYOBJ", wr.str);
      var_set_num(vm, "BOTNOBJ_N", hr.n);
      var_set_num(vm, "BOTNOBJ_CAND", (long)hr.code);
      var_set_num(vm, "BOTNOBJ_FROM", have_from ? 1 : 0);
    } else {
      var_set_str(vm, "TOPNOBJ", wr.str);
      var_set_str(vm, "TOPNKEYOBJ", wr.str);
      var_set_num(vm, "TOPNOBJ_N", hr.n);
      var_set_num(vm, "TOPNOBJ_CAND", (long)hr.code);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s nest=%s n=%ld cand=%d take=%ld from=%d\n",
              want_bot ? "botnobj" : "topnobj", nestk, hr.n, hr.code, ntake,
              have_from);
    bump(vm); return 1;
  }

  /* TOPNP|TOPNKEYP [FROM plate] n — plate of top n pure-int keys by value DESC.
   * BOTNP|BOTNKEYP [FROM plate] n — bottom n pure-int keys ASC (smallest first).
   * Non-int keys excluded. Cap 256 candidates. Bare uses PLATE. No source mutate.
   * LAST = plate · LAST_N = kept · TOPNP_CAND = pure-int candidates seen.
   * Usability: top FREQ severities without TOKV+SORTFREQ+TAKE+FROMKVP:
   *   TOPNP 3 FROM freq
   *   BOTNP 2 FROM scores
   */
  if (kw(&L->cur,"TOPNP") || kw(&L->cur,"TOPNKEYP") || kw(&L->cur,"HEADNP") ||
      kw(&L->cur,"MTOPNP") || kw(&L->cur,"PLATE_TOPN") || kw(&L->cur,"TAKEBYVP") ||
      kw(&L->cur,"BOTNP") || kw(&L->cur,"BOTNKEYP") || kw(&L->cur,"TAILNP") ||
      kw(&L->cur,"MBOTNP") || kw(&L->cur,"PLATE_BOTN") || kw(&L->cur,"BOTTAKEP")) {
    char plate[CUBALC_HOST_STR_MAX], from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, want_bot = 0, have_n = 0;
    long ntake = 1;
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "BOTNP") == 0 || strcmp(op0, "BOTNKEYP") == 0 ||
        strcmp(op0, "TAILNP") == 0 || strcmp(op0, "MBOTNP") == 0 ||
        strcmp(op0, "PLATE_BOTN") == 0 || strcmp(op0, "BOTTAKEP") == 0)
      want_bot = 1;

    lex_next(L);
    plate[0] = 0; from_name[0] = 0; from_src[0] = 0;

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

    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      ntake = parse_expr(vm, L);
      have_n = 1;
    } else if (L->cur.kind == TK_IDENT) {
      Var *dv = var_get(vm, L->cur.text, 0);
      if (dv && !dv->is_str) {
        ntake = (long)dv->val;
        have_n = 1;
        lex_next(L);
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

    if (!have_n) {
      fail(vm, want_bot
           ? "BOTNP [FROM plate] n — need count"
           : "TOPNP [FROM plate] n — need count");
      return -1;
    }

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_topn(plate, ntake, want_bot, &hr) != 0) {
      var_set_str(vm, "LAST", "{}");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "{}");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "TOPNP_N", 0);
      var_set_num(vm, "BOTNP_N", 0);
      var_set_num(vm, "TOPNP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err :
                  (want_bot ? "BOTNP: fail" : "TOPNP: fail"));
      var_set_str(vm, "ERR", hr.err[0] ? hr.err :
                  (want_bot ? "BOTNP: fail" : "TOPNP: fail"));
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    if (want_bot) {
      var_set_str(vm, "BOTNP", hr.str);
      var_set_str(vm, "BOTNKEYP", hr.str);
      var_set_num(vm, "BOTNP_N", hr.n);
      var_set_num(vm, "BOTNP_CAND", (long)hr.code);
      var_set_num(vm, "BOTNP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "BOTNP_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    } else {
      var_set_str(vm, "TOPNP", hr.str);
      var_set_str(vm, "TOPNKEYP", hr.str);
      var_set_num(vm, "TOPNP_N", hr.n);
      var_set_num(vm, "TOPNP_CAND", (long)hr.code);
      var_set_num(vm, "TOPNP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "TOPNP_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%ld cand=%d take=%ld from=%d\n",
              want_bot ? "botnp" : "topnp", hr.n, hr.code, ntake, have_from);
    bump(vm); return 1;
  }
  return 0;
}
