/* CubalC flat nest sort shard */
#include "lang/cubalc_lang_ops_core_flat_nest_sortobj.h"
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

int cubalc_lang_ops_core_try_flat_nest_sortobj(VM *vm, Lex *L) {
  /* SORTOBJ|SORTVOBJ|NESTSORT [FROM plate] nest [ASC|DESC] —
   * sort pure-int nest keys by value → write-back outer (SORTP dual).
   * SORTBAGOBJ|SORTTOKVOBJ|SORTFREQOBJ [FROM plate] nest [ASC|DESC] —
   * key:val bag sorted by value (no write-back · SORTBAGP dual for nests).
   * Default DESC. Soft nest miss: outer unchanged / empty bag · NEST_HIT=0.
   * Not SORTOBJS (OOP fleet SORTBYF).
   * Usability: full nest FREQ rank without GETOBJ+SORTP+SETOBJ:
   *   SORTOBJ "freq"
   *   SORTOBJ ASC FROM PEER "scores"
   *   SORTBAGOBJ "freq"   # bag for EACH LINE
   */
  if (kw(&L->cur,"SORTOBJ") || kw(&L->cur,"SORTVOBJ") || kw(&L->cur,"NESTSORT") ||
      kw(&L->cur,"MSORTOBJ") || kw(&L->cur,"ORDEROBJ") || kw(&L->cur,"RANKOBJ") ||
      kw(&L->cur,"SORTBAGOBJ") || kw(&L->cur,"SORTTOKVOBJ") || kw(&L->cur,"SORTFREQOBJ") ||
      kw(&L->cur,"MSORTBAGOBJ") || kw(&L->cur,"NEST_SORTBAG") || kw(&L->cur,"TOKV_SORTOBJ") ||
      kw(&L->cur,"SORTKEYVALOBJ") || kw(&L->cur,"NESTSORTBAG")) {
    char plate[CUBALC_HOST_STR_MAX], nestk[96], nest[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result ngr, hr, wr;
    int have_from = 0, nest_hit = 0, want_asc = 0, as_bag = 0;
    Var *pv;
    const char *v;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "SORTBAGOBJ") == 0 || strcmp(op0, "SORTTOKVOBJ") == 0 ||
        strcmp(op0, "SORTFREQOBJ") == 0 || strcmp(op0, "MSORTBAGOBJ") == 0 ||
        strcmp(op0, "NEST_SORTBAG") == 0 || strcmp(op0, "TOKV_SORTOBJ") == 0 ||
        strcmp(op0, "SORTKEYVALOBJ") == 0 || strcmp(op0, "NESTSORTBAG") == 0)
      as_bag = 1;

    lex_next(L);
    plate[0] = 0; nestk[0] = 0; nest[0] = 0;
    from_name[0] = 0; from_src[0] = 0;

    /* leading FROM / ASC / DESC before nest */
    for (;;) {
      if (kw(&L->cur,"DESC") || kw(&L->cur,"DOWN") || kw(&L->cur,"REV") ||
          kw(&L->cur,"HEAVY") || kw(&L->cur,"HIGH") || kw(&L->cur,"DESCENDING")) {
        want_asc = 0; lex_next(L); continue;
      }
      if (kw(&L->cur,"ASC") || kw(&L->cur,"UP") || kw(&L->cur,"LIGHT") ||
          kw(&L->cur,"LOW") || kw(&L->cur,"ASCENDING")) {
        want_asc = 1; lex_next(L); continue;
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
        continue;
      }
      if (kw(&L->cur,"BAG") || kw(&L->cur,"ASBAG") || kw(&L->cur,"TOKV") ||
          kw(&L->cur,"AS_KV") || kw(&L->cur,"KV")) {
        as_bag = 1; lex_next(L); continue;
      }
      break;
    }

    /* nest key */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(nestk, sizeof nestk, "%ld", kv);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
          kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
          kw(&L->cur,"ASC") || kw(&L->cur,"DESC") ||
          kw(&L->cur,"END") || kw(&L->cur,"ASSERT") || kw(&L->cur,"LET")) {
        fail(vm, "SORTOBJ nest — need nest key");
        return -1;
      }
      if (resolve_str_arg(vm, L, nestk, sizeof nestk) != 0)
        nestk[0] = 0;
    } else {
      fail(vm, "SORTOBJ nest — need nest key");
      return -1;
    }

    /* trailing ASC/DESC / FROM / BAG */
    for (;;) {
      if (kw(&L->cur,"DESC") || kw(&L->cur,"DOWN") || kw(&L->cur,"REV") ||
          kw(&L->cur,"HEAVY") || kw(&L->cur,"HIGH") || kw(&L->cur,"DESCENDING")) {
        want_asc = 0; lex_next(L); continue;
      }
      if (kw(&L->cur,"ASC") || kw(&L->cur,"UP") || kw(&L->cur,"LIGHT") ||
          kw(&L->cur,"LOW") || kw(&L->cur,"ASCENDING")) {
        want_asc = 1; lex_next(L); continue;
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
        continue;
      }
      if (kw(&L->cur,"BAG") || kw(&L->cur,"ASBAG") || kw(&L->cur,"TOKV") ||
          kw(&L->cur,"AS_KV") || kw(&L->cur,"KV")) {
        as_bag = 1; lex_next(L); continue;
      }
      break;
    }

    if (!nestk[0]) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "SORTOBJ: empty nest");
      var_set_str(vm, "ERR", "SORTOBJ: empty nest");
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
      if (as_bag) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
      } else {
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      }
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "NEST", "");
      var_set_num(vm, "SORTOBJ_N", 0);
      var_set_num(vm, "SORTBAGOBJ_N", 0);
      var_set_num(vm, "SORTOBJ_ASC", want_asc ? 1 : 0);
      var_set_str(vm, "SORTOBJ_NEST", nestk);
      var_set_num(vm, "SORTOBJ_FROM", have_from ? 1 : 0);
      var_set_str(vm, "SORTOBJ_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "SORTOBJ_NEST_HIT", 0);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sortobj nest-miss nest=%s bag=%d\n", nestk, as_bag);
      bump(vm); return 1;
    }

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_sortbyval(nest, want_asc, as_bag, &hr) != 0) {
      var_set_str(vm, "LAST", as_bag ? "" : "{}");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", as_bag ? "" : "{}");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "SORTOBJ_N", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err :
                  (as_bag ? "SORTBAGOBJ: fail" : "SORTOBJ: fail"));
      var_set_str(vm, "ERR", hr.err[0] ? hr.err :
                  (as_bag ? "SORTBAGOBJ: fail" : "SORTOBJ: fail"));
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }

    if (as_bag) {
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "NEST", nest);
      var_set_str(vm, "SORTBAGOBJ", hr.str);
      var_set_str(vm, "SORTTOKVOBJ", hr.str);
      var_set_str(vm, "SORTFREQOBJ", hr.str);
      var_set_num(vm, "SORTBAGOBJ_N", hr.n);
      var_set_num(vm, "SORTBAGOBJ_FROM", have_from ? 1 : 0);
      var_set_str(vm, "SORTBAGOBJ_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_str(vm, "SORTOBJ_NEST", nestk);
      var_set_num(vm, "SORTOBJ_FROM", have_from ? 1 : 0);
      var_set_str(vm, "SORTOBJ_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "SORTOBJ_NEST_HIT", 1);
      var_set_num(vm, "SORTOBJ_ASC", want_asc ? 1 : 0);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sortbagobj nest=%s n=%ld asc=%d from=%d\n",
                nestk, hr.n, want_asc, have_from);
      bump(vm); return 1;
    }

    /* write-back sorted nest */
    snprintf(nest, sizeof nest, "%s", hr.str);
    memset(&wr, 0, sizeof wr);
    if (cubalc_host_json_set(plate, nestk, nest, 1, &wr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "SORTOBJ: write-back fail");
      var_set_str(vm, "ERR", wr.err[0] ? wr.err : "SORTOBJ: write-back fail");
      var_set_num(vm, "SORTOBJ_N", 0);
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
    var_set_str(vm, "SORTOBJ", wr.str);
    var_set_str(vm, "SORTVOBJ", wr.str);
    var_set_str(vm, "NESTSORT", wr.str);
    var_set_num(vm, "SORTOBJ_N", hr.n);
    var_set_num(vm, "SORTOBJ_ASC", want_asc ? 1 : 0);
    var_set_str(vm, "SORTOBJ_NEST", nestk);
    var_set_num(vm, "SORTOBJ_FROM", have_from ? 1 : 0);
    var_set_str(vm, "SORTOBJ_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "SORTOBJ_NEST_HIT", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# sortobj nest=%s n=%ld asc=%d from=%d\n",
              nestk, hr.n, want_asc, have_from);
    bump(vm); return 1;
  }

  return 0;
}
