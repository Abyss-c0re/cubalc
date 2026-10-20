/* CubalC flat nest shard */
#include "lang/cubalc_lang_ops_core_flat_nest_pick.h"
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

int cubalc_lang_ops_core_try_flat_nest_pick(VM *vm, Lex *L) {
  /* PICKOBJ|KEEPOBJ|SELECTNEST [FROM plate] nest field…
   * OMITOBJ|DROPOBJKEYS|STRIPNEST [FROM plate] nest field…
   * Keep/drop listed keys inside a nested object → write-back outer plate.
   * Nest may be dotted/slash path: PICKOBJ "cfg.flags" "debug"
   * Soft nest miss → creates {} then pick (empty) / omit (empty).
   * LAST = outer plate · NEST/MERGED = nested result · LAST_N = keys kept/dropped.
   * Usability: nested dual of PICKP/OMITP without GETOBJ+PICKP+SETOBJ glue:
   *   PICKOBJ "meta" "role" "zone"
   *   PICKOBJ "cfg.flags" "debug"
   *   OMITOBJ FROM PEER "cfg" "tmp" "debug"
   */
  if (kw(&L->cur,"PICKOBJ") || kw(&L->cur,"KEEPOBJ") || kw(&L->cur,"SELECTNEST") ||
      kw(&L->cur,"NESTPICK") || kw(&L->cur,"PICKNEST") || kw(&L->cur,"MPICKOBJ") ||
      kw(&L->cur,"OMITOBJ") || kw(&L->cur,"DROPOBJKEYS") || kw(&L->cur,"STRIPNEST") ||
      kw(&L->cur,"NESTOMIT") || kw(&L->cur,"OMITNEST") || kw(&L->cur,"MOMITOBJ") ||
      kw(&L->cur,"DROPNESTKEYS") || kw(&L->cur,"STRIPOBJ")) {
    char plate[CUBALC_HOST_STR_MAX], nestk[192], nest[CUBALC_HOST_STR_MAX];
    char keys_nl[CUBALC_HOST_STR_MAX], arg[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result ngr, pr, hr;
    int have_from = 0, is_omit = 0, n_req = 0, nest_hit = 0;
    size_t olen = 0;
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "OMITOBJ") == 0 || strcmp(op0, "DROPOBJKEYS") == 0 ||
        strcmp(op0, "STRIPNEST") == 0 || strcmp(op0, "NESTOMIT") == 0 ||
        strcmp(op0, "OMITNEST") == 0 || strcmp(op0, "MOMITOBJ") == 0 ||
        strcmp(op0, "DROPNESTKEYS") == 0 || strcmp(op0, "STRIPOBJ") == 0)
      is_omit = 1;

    lex_next(L);
    plate[0] = 0; nestk[0] = 0; nest[0] = 0; keys_nl[0] = 0;
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
        fail(vm, is_omit ? "OMITOBJ nest field… — need nest key"
                         : "PICKOBJ nest field… — need nest key");
        return -1;
      }
      if (resolve_str_arg(vm, L, nestk, sizeof nestk) != 0)
        nestk[0] = 0;
    } else {
      fail(vm, is_omit ? "OMITOBJ nest field… — need nest key"
                       : "PICKOBJ nest field… — need nest key");
      return -1;
    }

    while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
           L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
           kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
           kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"ASSERT") ||
           kw(&L->cur,"PRINT") || kw(&L->cur,"LET") || kw(&L->cur,"SYS") ||
           kw(&L->cur,"SETP") || kw(&L->cur,"GETP") || kw(&L->cur,"PICKP") ||
           kw(&L->cur,"OMITP") || kw(&L->cur,"PICKOBJ") || kw(&L->cur,"OMITOBJ") ||
           kw(&L->cur,"PLUCKOBJ") || kw(&L->cur,"GETPOBJ") || kw(&L->cur,"INCLUDE") ||
           kw(&L->cur,"REQUIRE") || kw(&L->cur,"FAIL") || kw(&L->cur,"PASS") ||
           kw(&L->cur,"NOTE") || kw(&L->cur,"EXIT")))
        break;
      arg[0] = 0;
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long kv = parse_expr(vm, L);
        snprintf(arg, sizeof arg, "%ld", kv);
      } else if (resolve_str_arg(vm, L, arg, sizeof arg) != 0) {
        break;
      }
      if (!arg[0]) continue;
      if (olen > 0) {
        if (olen + 1 >= sizeof keys_nl) break;
        keys_nl[olen++] = '\n';
        keys_nl[olen] = 0;
      }
      {
        size_t al = strlen(arg);
        if (olen + al + 1 >= sizeof keys_nl) break;
        memcpy(keys_nl + olen, arg, al + 1);
        olen += al;
      }
      n_req++;
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

    if (!nestk[0] || n_req == 0) {
      if (n_req == 0) {
        fail(vm, is_omit ? "OMITOBJ nest field… — need fields"
                         : "PICKOBJ nest field… — need fields");
        return -1;
      }
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "PICKOBJ|OMITOBJ: empty nest");
      var_set_str(vm, "ERR", "PICKOBJ|OMITOBJ: empty nest");
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

    /* path-aware peel: "cfg.flags" / "a/b" · soft miss → {} */
    snprintf(nest, sizeof nest, "%s", "{}");
    nest_hit = 0;
    memset(&ngr, 0, sizeof ngr);
    cubalc_host_json_path_obj(plate, nestk, &ngr);
    snprintf(nest, sizeof nest, "%s", ngr.str);
    nest_hit = (ngr.n != 0) ? 1 : 0;

    memset(&pr, 0, sizeof pr);
    if (is_omit) {
      if (cubalc_host_json_drop(nest, keys_nl, &pr) != 0) {
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", pr.err[0] ? pr.err : "OMITOBJ: drop fail");
        var_set_str(vm, "ERR", pr.err[0] ? pr.err : "OMITOBJ: drop fail");
        bump(vm); return 1;
      }
    } else {
      if (cubalc_host_json_pick(nest, keys_nl, &pr) != 0) {
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", pr.err[0] ? pr.err : "PICKOBJ: pick fail");
        var_set_str(vm, "ERR", pr.err[0] ? pr.err : "PICKOBJ: pick fail");
        bump(vm); return 1;
      }
    }
    snprintf(nest, sizeof nest, "%s", pr.str);

    /* write-back nest object along path (creates intermediate {} if needed) */
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_path_set(plate, nestk, nest, 1, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "PICKOBJ: write-back fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "PICKOBJ: write-back fail");
      bump(vm); return 1;
    }

    if (have_from && from_name[0] && strcmp(from_name, "LAST") != 0)
      var_set_str(vm, from_name, hr.str);
    else if (!have_from)
      var_set_str(vm, "PLATE", hr.str);

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = pr.n;
    var_set_num(vm, "LAST_N", pr.n);
    var_set_str(vm, "NEST", nest);
    var_set_str(vm, "MERGED", nest);
    var_set_str(vm, is_omit ? "OMITOBJ" : "PICKOBJ", hr.str);
    var_set_num(vm, is_omit ? "OMITOBJ_N" : "PICKOBJ_N", pr.n);
    var_set_num(vm, "PICKOBJ_REQ", n_req);
    var_set_str(vm, "PICKOBJ_NEST", nestk);
    var_set_num(vm, "PICKOBJ_FROM", have_from ? 1 : 0);
    var_set_str(vm, "PICKOBJ_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "PICKOBJ_NEST_HIT", nest_hit ? 1 : 0);
    var_set_num(vm, "PICKOBJ_OMIT", is_omit ? 1 : 0);
    var_set_num(vm, "PATH_DEPTH", (long)hr.code > 0 ? (long)hr.code : 1L);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s nest=%s n=%ld req=%d from=%d\n",
              is_omit ? "omitobj" : "pickobj", nestk, pr.n, n_req, have_from);
    bump(vm); return 1;
  }

  /* LENOBJ|NESTLEN|NKEYSOBJ [FROM plate] nest — nested key count → LAST_N (LENP dual).
   * EMPTYOBJ|ISEMPTYNEST [FROM plate] nest — soft empty nest 0|1 (EMPTYP dual).
   * NONEMPTYOBJ|HASKEYSOBJ [FROM plate] nest — soft nonempty 0|1.
   * VALSOBJ|NESTVALS [FROM plate] nest — nested values bag → LAST (VALSP dual).
   * Soft nest miss: LEN=0 · EMPTY=1 · NONEMPTY=0 · VALS="".
   * Usability: nest size/empty/values without GETOBJ+LENP/EMPTYP/VALSP glue:
   *   LENOBJ "meta"
   *   EMPTYOBJ "stats"
   *   VALSOBJ FROM PEER "cfg"
   */
  if (kw(&L->cur,"LENOBJ") || kw(&L->cur,"NESTLEN") || kw(&L->cur,"NKEYSOBJ") ||
      kw(&L->cur,"OBJLEN") || kw(&L->cur,"MLENOBJ") || kw(&L->cur,"COUNTNEST") ||
      kw(&L->cur,"EMPTYOBJ") || kw(&L->cur,"ISEMPTYNEST") || kw(&L->cur,"NESTEMPTY") ||
      kw(&L->cur,"ISEMPTYOBJ") || kw(&L->cur,"MEMPTYOBJ") ||
      kw(&L->cur,"NONEMPTYOBJ") || kw(&L->cur,"HASKEYSOBJ") || kw(&L->cur,"NESTNONEMPTY") ||
      kw(&L->cur,"HASNESTKEYS") || kw(&L->cur,"MNONEMPTYOBJ") ||
      kw(&L->cur,"VALSOBJ") || kw(&L->cur,"NESTVALS") || kw(&L->cur,"OBJVALS") ||
      kw(&L->cur,"VALUESNEST") || kw(&L->cur,"MVALSOBJ") || kw(&L->cur,"NESTVALUES")) {
    char plate[CUBALC_HOST_STR_MAX], nestk[96], nest[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result ngr, hr;
    int have_from = 0, nest_hit = 0;
    int is_len = 0, is_empty = 0, is_nonempty = 0, is_vals = 0;
    long nkeys = 0;
    Var *pv;
    const char *v;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "LENOBJ") == 0 || strcmp(op0, "NESTLEN") == 0 ||
        strcmp(op0, "NKEYSOBJ") == 0 || strcmp(op0, "OBJLEN") == 0 ||
        strcmp(op0, "MLENOBJ") == 0 || strcmp(op0, "COUNTNEST") == 0)
      is_len = 1;
    else if (strcmp(op0, "EMPTYOBJ") == 0 || strcmp(op0, "ISEMPTYNEST") == 0 ||
             strcmp(op0, "NESTEMPTY") == 0 || strcmp(op0, "ISEMPTYOBJ") == 0 ||
             strcmp(op0, "MEMPTYOBJ") == 0)
      is_empty = 1;
    else if (strcmp(op0, "NONEMPTYOBJ") == 0 || strcmp(op0, "HASKEYSOBJ") == 0 ||
             strcmp(op0, "NESTNONEMPTY") == 0 || strcmp(op0, "HASNESTKEYS") == 0 ||
             strcmp(op0, "MNONEMPTYOBJ") == 0)
      is_nonempty = 1;
    else
      is_vals = 1;

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

    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(nestk, sizeof nestk, "%ld", kv);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
          kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
          kw(&L->cur,"END") || kw(&L->cur,"ASSERT") || kw(&L->cur,"LET")) {
        fail(vm, "LENOBJ|EMPTYOBJ|VALSOBJ nest — need nest key");
        return -1;
      }
      if (resolve_str_arg(vm, L, nestk, sizeof nestk) != 0)
        nestk[0] = 0;
    } else {
      fail(vm, "LENOBJ|EMPTYOBJ|VALSOBJ nest — need nest key");
      return -1;
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
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "LENOBJ|EMPTYOBJ|VALSOBJ: empty nest");
      var_set_str(vm, "ERR", "LENOBJ|EMPTYOBJ|VALSOBJ: empty nest");
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

    nkeys = 0;
    if (nest_hit) {
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_len(nest, &hr) == 0)
        nkeys = hr.n;
      else if (cubalc_host_json_keys(nest, &hr) == 0)
        nkeys = hr.n;
    }

    var_set_str(vm, "LENOBJ_NEST", nestk);
    var_set_num(vm, "LENOBJ_FROM", have_from ? 1 : 0);
    var_set_str(vm, "LENOBJ_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "LENOBJ_NEST_HIT", nest_hit ? 1 : 0);

    if (is_len) {
      var_set_num(vm, "LAST_N", nkeys);
      vm->last_n = nkeys;
      {
        char nb[32];
        snprintf(nb, sizeof nb, "%ld", nkeys);
        var_set_str(vm, "LAST", nb);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
      }
      var_set_num(vm, "LENOBJ", nkeys);
      var_set_num(vm, "LENOBJ_N", nkeys);
      var_set_num(vm, "NKEYS", nkeys);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# lenobj nest=%s n=%ld from=%d\n",
                nestk, nkeys, have_from);
      bump(vm); return 1;
    }

    if (is_empty || is_nonempty) {
      int empty = (!nest_hit || nkeys == 0) ? 1 : 0;
      int flag = is_empty ? empty : !empty;
      var_set_num(vm, "LAST_N", flag ? 1 : 0);
      vm->last_n = flag ? 1 : 0;
      var_set_str(vm, "LAST", flag ? "1" : "0");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", flag ? "1" : "0");
      if (is_empty) {
        var_set_num(vm, "EMPTYOBJ", empty ? 1 : 0);
        var_set_num(vm, "EMPTYOBJ_N", empty ? 1 : 0);
      } else {
        var_set_num(vm, "NONEMPTYOBJ", empty ? 0 : 1);
        var_set_num(vm, "NONEMPTYOBJ_N", empty ? 0 : 1);
      }
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# %s nest=%s flag=%d\n",
                is_empty ? "emptyobj" : "nonemptyobj", nestk, flag);
      bump(vm); return 1;
    }

    /* VALSOBJ */
    {
      char bag[CUBALC_HOST_STR_MAX];
      bag[0] = 0;
      if (nest_hit) {
        memset(&hr, 0, sizeof hr);
        if (cubalc_host_json_values(nest, &hr) == 0) {
          snprintf(bag, sizeof bag, "%s", hr.str);
          nkeys = hr.n;
        }
      }
      var_set_str(vm, "LAST", bag);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
      vm->last_n = nkeys;
      var_set_num(vm, "LAST_N", nkeys);
      var_set_str(vm, "VALSOBJ", bag);
      var_set_str(vm, "NESTVALS", bag);
      var_set_num(vm, "VALSOBJ_N", nkeys);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# valsobj nest=%s n=%ld from=%d\n",
                nestk, nkeys, have_from);
      bump(vm); return 1;
    }
  }







  /* RENAMEPOBJ|MOVEKEYOBJ|NESTRENAME [FROM plate] nest old new —
   * Rename key inside nested object → write-back outer plate (RENAMEP dual).
   * Nest may be dotted/slash path: RENAMEPOBJ "cfg.flags" "tmp" "debug"
   * Soft nest miss or old-key miss: LAST_N=0 · outer unchanged · OK=1.
   * Dest overwrites if already present (same as RENAMEP/JSONRENAME).
   * Not RENAMEOBJ (OOP live-slot rename) — nest keys only.
   * Usability: promote nest tmp→status without GETOBJ+RENAMEP+SETOBJ glue:
   *   RENAMEPOBJ "meta" "tmp" "status"
   *   RENAMEPOBJ "cfg.flags" "dbg" "debug"
   *   RENAMEPOBJ FROM PEER "cfg" "tmp_port" "port"
   */
  if (kw(&L->cur,"RENAMEPOBJ") || kw(&L->cur,"MOVEKEYOBJ") || kw(&L->cur,"NESTRENAME") ||
      kw(&L->cur,"RENAMEKEYOBJ") || kw(&L->cur,"MVKEYOBJ") || kw(&L->cur,"REKEYOBJ") ||
      kw(&L->cur,"MRENAMEPOBJ") || kw(&L->cur,"NEST_RENAME") || kw(&L->cur,"OBJRENAMEKEY")) {
    char plate[CUBALC_HOST_STR_MAX], nestk[192], nest[CUBALC_HOST_STR_MAX];
    char oldk[96], newk[96];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result ngr, rr, hr;
    int have_from = 0, nest_hit = 0;
    Var *pv;

    lex_next(L);
    plate[0] = 0; nestk[0] = 0; nest[0] = 0; oldk[0] = 0; newk[0] = 0;
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
        fail(vm, "RENAMEPOBJ [FROM plate] nest old new — need nest key");
        return -1;
      }
      if (resolve_str_arg(vm, L, nestk, sizeof nestk) != 0)
        nestk[0] = 0;
    } else {
      fail(vm, "RENAMEPOBJ [FROM plate] nest old new — need nest key");
      return -1;
    }

    /* old key */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(oldk, sizeof oldk, "%ld", kv);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
          kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
          kw(&L->cur,"AS") || kw(&L->cur,"TO") ||
          kw(&L->cur,"END") || kw(&L->cur,"ASSERT") || kw(&L->cur,"LET")) {
        fail(vm, "RENAMEPOBJ nest old new — need old key");
        return -1;
      }
      if (resolve_str_arg(vm, L, oldk, sizeof oldk) != 0)
        oldk[0] = 0;
    } else {
      fail(vm, "RENAMEPOBJ nest old new — need old key");
      return -1;
    }

    /* optional AS|TO before new key */
    if (kw(&L->cur,"AS") || kw(&L->cur,"TO") || kw(&L->cur,"INTO"))
      lex_next(L);

    /* new key */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(newk, sizeof newk, "%ld", kv);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
          kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
          kw(&L->cur,"END") || kw(&L->cur,"ASSERT") || kw(&L->cur,"LET")) {
        fail(vm, "RENAMEPOBJ nest old new — need new key");
        return -1;
      }
      if (resolve_str_arg(vm, L, newk, sizeof newk) != 0)
        newk[0] = 0;
    } else {
      fail(vm, "RENAMEPOBJ nest old new — need new key");
      return -1;
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

    if (!nestk[0] || !oldk[0] || !newk[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "RENAMEPOBJ_N", 0);
      var_set_num(vm, "RENAMEPOBJ_FROM", have_from ? 1 : 0);
      var_set_num(vm, "RENAMEPOBJ_NEST_HIT", 0);
      var_set_str(vm, "LAST_ERR", "RENAMEPOBJ: empty nest/key");
      var_set_str(vm, "ERR", "RENAMEPOBJ: empty nest/key");
      var_set_num(vm, "OK", 0);
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

    /* path-aware peel: "cfg.flags" · soft miss → n=0 (outer unchanged) */
    nest_hit = 0;
    nest[0] = 0;
    memset(&ngr, 0, sizeof ngr);
    cubalc_host_json_path_obj(plate, nestk, &ngr);
    snprintf(nest, sizeof nest, "%s", ngr.str);
    nest_hit = (ngr.n != 0) ? 1 : 0;

    /* soft nest miss — no invent empty nest; outer unchanged */
    if (!nest_hit) {
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "RENAMEPOBJ", plate);
      var_set_num(vm, "RENAMEPOBJ_N", 0);
      var_set_str(vm, "RENAMEPOBJ_NEST", nestk);
      var_set_str(vm, "RENAMEPOBJ_OLD", oldk);
      var_set_str(vm, "RENAMEPOBJ_NEW", newk);
      var_set_num(vm, "RENAMEPOBJ_FROM", have_from ? 1 : 0);
      var_set_str(vm, "RENAMEPOBJ_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "RENAMEPOBJ_NEST_HIT", 0);
      var_set_str(vm, "NEST", "");
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# renamepobj nest-miss nest=%s %s→%s from=%d\n",
                nestk, oldk, newk, have_from);
      bump(vm); return 1;
    }

    memset(&rr, 0, sizeof rr);
    if (cubalc_host_json_rename(nest, oldk, newk, &rr) != 0) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "RENAMEPOBJ_N", 0);
      var_set_num(vm, "RENAMEPOBJ_FROM", have_from ? 1 : 0);
      var_set_num(vm, "RENAMEPOBJ_NEST_HIT", 1);
      var_set_str(vm, "LAST_ERR", rr.err[0] ? rr.err : "RENAMEPOBJ: rename fail");
      var_set_str(vm, "ERR", rr.err[0] ? rr.err : "RENAMEPOBJ: rename fail");
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }

    /* soft old-key miss: n=0 · leave outer unchanged */
    if (rr.n == 0) {
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "RENAMEPOBJ", plate);
      var_set_num(vm, "RENAMEPOBJ_N", 0);
      var_set_str(vm, "RENAMEPOBJ_NEST", nestk);
      var_set_str(vm, "RENAMEPOBJ_OLD", oldk);
      var_set_str(vm, "RENAMEPOBJ_NEW", newk);
      var_set_num(vm, "RENAMEPOBJ_FROM", have_from ? 1 : 0);
      var_set_str(vm, "RENAMEPOBJ_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "RENAMEPOBJ_NEST_HIT", 1);
      var_set_str(vm, "NEST", nest);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# renamepobj key-miss nest=%s %s→%s from=%d\n",
                nestk, oldk, newk, have_from);
      bump(vm); return 1;
    }

    snprintf(nest, sizeof nest, "%s", rr.str);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_path_set(plate, nestk, nest, 1, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "RENAMEPOBJ: write-back fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "RENAMEPOBJ: write-back fail");
      var_set_num(vm, "RENAMEPOBJ_N", 0);
      bump(vm); return 1;
    }

    if (have_from && from_name[0] && strcmp(from_name, "LAST") != 0)
      var_set_str(vm, from_name, hr.str);
    else if (!have_from)
      var_set_str(vm, "PLATE", hr.str);

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = rr.n;
    var_set_num(vm, "LAST_N", rr.n);
    var_set_str(vm, "NEST", nest);
    var_set_str(vm, "MERGED", nest);
    var_set_str(vm, "RENAMEPOBJ", hr.str);
    var_set_str(vm, "MOVEKEYOBJ", hr.str);
    var_set_str(vm, "NESTRENAME", hr.str);
    var_set_num(vm, "RENAMEPOBJ_N", rr.n);
    var_set_str(vm, "RENAMEPOBJ_NEST", nestk);
    var_set_str(vm, "RENAMEPOBJ_OLD", oldk);
    var_set_str(vm, "RENAMEPOBJ_NEW", newk);
    var_set_num(vm, "RENAMEPOBJ_FROM", have_from ? 1 : 0);
    var_set_str(vm, "RENAMEPOBJ_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "RENAMEPOBJ_NEST_HIT", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# renamepobj nest=%s %s→%s n=%ld from=%d\n",
              nestk, oldk, newk, rr.n, have_from);
    bump(vm); return 1;
  }

  /* COPYPOBJ|DUPKEYOBJ|NESTCOPY [FROM plate] nest src dst —
   * SWAPPOBJ|XCHGKEYOBJ|NESTSWAP [FROM plate] nest a b —
   * Copy/swap keys inside nested object → write-back outer (COPYP/SWAPP duals).
   * Nest may be dotted/slash path: COPYPOBJ "cfg.flags" "debug" "trace"
   * Soft nest miss or src miss (copy): LAST_N=0 · outer unchanged · OK=1.
   * SWAPP: both miss n=0 · one miss = move · both hit = exchange.
   * Not CLONEOBJ/COPYOBJ (OOP live-slot clone).
   * Usability: nest dual-buffer / snapshot without GETOBJ+COPYP/SWAPP+SETOBJ:
   *   COPYPOBJ "meta" "status" "prev"
   *   COPYPOBJ "cfg.flags" "debug" "mirror"
   *   SWAPPOBJ FROM PEER "cfg" "cur" "prev"
   */
  if (kw(&L->cur,"COPYPOBJ") || kw(&L->cur,"DUPKEYOBJ") || kw(&L->cur,"NESTCOPY") ||
      kw(&L->cur,"COPYKEYOBJ") || kw(&L->cur,"SNAPSHOTNEST") || kw(&L->cur,"MCOPYPOBJ") ||
      kw(&L->cur,"SWAPPOBJ") || kw(&L->cur,"XCHGKEYOBJ") || kw(&L->cur,"NESTSWAP") ||
      kw(&L->cur,"SWAPKEYOBJ") || kw(&L->cur,"FLIPKEYOBJ") || kw(&L->cur,"MSWAPPOBJ") ||
      kw(&L->cur,"EXCHKEYOBJ") || kw(&L->cur,"OBJCOPYKEY") || kw(&L->cur,"OBJSWAPKEY")) {
    char plate[CUBALC_HOST_STR_MAX], nestk[192], nest[CUBALC_HOST_STR_MAX];
    char ka[96], kb[96];
    char ra[CUBALC_HOST_STR_MAX], rb[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result ngr, gr, hr, dr;
    int have_from = 0, nest_hit = 0, is_swap = 0, has_a = 0, has_b = 0;
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "SWAPPOBJ") == 0 || strcmp(op0, "XCHGKEYOBJ") == 0 ||
        strcmp(op0, "NESTSWAP") == 0 || strcmp(op0, "SWAPKEYOBJ") == 0 ||
        strcmp(op0, "FLIPKEYOBJ") == 0 || strcmp(op0, "MSWAPPOBJ") == 0 ||
        strcmp(op0, "EXCHKEYOBJ") == 0 || strcmp(op0, "OBJSWAPKEY") == 0)
      is_swap = 1;

    lex_next(L);
    plate[0] = 0; nestk[0] = 0; nest[0] = 0; ka[0] = 0; kb[0] = 0;
    ra[0] = 0; rb[0] = 0;
    from_name[0] = 0; from_src[0] = 0;
    memset(&hr, 0, sizeof hr);
    memset(&dr, 0, sizeof dr);

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
        fail(vm, is_swap ? "SWAPPOBJ nest a b — need nest key"
                         : "COPYPOBJ nest src dst — need nest key");
        return -1;
      }
      if (resolve_str_arg(vm, L, nestk, sizeof nestk) != 0)
        nestk[0] = 0;
    } else {
      fail(vm, is_swap ? "SWAPPOBJ nest a b — need nest key"
                       : "COPYPOBJ nest src dst — need nest key");
      return -1;
    }

    /* key A */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(ka, sizeof ka, "%ld", kv);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
          kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
          kw(&L->cur,"END") || kw(&L->cur,"ASSERT") || kw(&L->cur,"LET")) {
        fail(vm, is_swap ? "SWAPPOBJ nest a b — need key A"
                         : "COPYPOBJ nest src dst — need src key");
        return -1;
      }
      if (resolve_str_arg(vm, L, ka, sizeof ka) != 0)
        ka[0] = 0;
    } else {
      fail(vm, is_swap ? "SWAPPOBJ nest a b — need key A"
                       : "COPYPOBJ nest src dst — need src key");
      return -1;
    }

    /* key B */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(kb, sizeof kb, "%ld", kv);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
          kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
          kw(&L->cur,"END") || kw(&L->cur,"ASSERT") || kw(&L->cur,"LET")) {
        fail(vm, is_swap ? "SWAPPOBJ nest a b — need key B"
                         : "COPYPOBJ nest src dst — need dst key");
        return -1;
      }
      if (resolve_str_arg(vm, L, kb, sizeof kb) != 0)
        kb[0] = 0;
    } else {
      fail(vm, is_swap ? "SWAPPOBJ nest a b — need key B"
                       : "COPYPOBJ nest src dst — need dst key");
      return -1;
    }

    /* trailing FROM */
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

    if (!nestk[0] || !ka[0] || !kb[0]) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", is_swap ? "SWAPPOBJ: empty nest/key" : "COPYPOBJ: empty nest/key");
      var_set_str(vm, "ERR", is_swap ? "SWAPPOBJ: empty nest/key" : "COPYPOBJ: empty nest/key");
      var_set_num(vm, "COPYPOBJ_N", 0);
      var_set_num(vm, "SWAPPOBJ_N", 0);
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "COPYPOBJ_FROM", have_from ? 1 : 0);
      var_set_num(vm, "SWAPPOBJ_FROM", have_from ? 1 : 0);
      var_set_num(vm, "COPYPOBJ_NEST_HIT", 0);
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

    /* path-aware peel: "cfg.flags" · soft miss → n=0 */
    nest_hit = 0;
    nest[0] = 0;
    memset(&ngr, 0, sizeof ngr);
    cubalc_host_json_path_obj(plate, nestk, &ngr);
    snprintf(nest, sizeof nest, "%s", ngr.str);
    nest_hit = (ngr.n != 0) ? 1 : 0;

    /* soft nest miss — outer unchanged */
    if (!nest_hit) {
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, is_swap ? "SWAPPOBJ" : "COPYPOBJ", plate);
      var_set_num(vm, "COPYPOBJ_N", 0);
      var_set_num(vm, "SWAPPOBJ_N", 0);
      var_set_str(vm, "COPYPOBJ_NEST", nestk);
      var_set_str(vm, "SWAPPOBJ_NEST", nestk);
      var_set_str(vm, "COPYPOBJ_SRC", ka);
      var_set_str(vm, "COPYPOBJ_DST", kb);
      var_set_str(vm, "SWAPPOBJ_A", ka);
      var_set_str(vm, "SWAPPOBJ_B", kb);
      var_set_num(vm, "COPYPOBJ_FROM", have_from ? 1 : 0);
      var_set_num(vm, "SWAPPOBJ_FROM", have_from ? 1 : 0);
      var_set_str(vm, "COPYPOBJ_VAR",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_str(vm, "SWAPPOBJ_VAR",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "COPYPOBJ_NEST_HIT", 0);
      var_set_num(vm, "SWAPPOBJ_NEST_HIT", 0);
      var_set_str(vm, "NEST", "");
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# %s nest-miss nest=%s from=%d\n",
                is_swap ? "swappobj" : "copypobj", nestk, have_from);
      bump(vm); return 1;
    }

    if (!is_swap) {
      /* COPYPOBJ */
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_get_raw(nest, ka, &gr) != 0) {
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "COPYPOBJ", plate);
        var_set_num(vm, "COPYPOBJ_N", 0);
        var_set_str(vm, "COPYPOBJ_NEST", nestk);
        var_set_str(vm, "COPYPOBJ_SRC", ka);
        var_set_str(vm, "COPYPOBJ_DST", kb);
        var_set_num(vm, "COPYPOBJ_FROM", have_from ? 1 : 0);
        var_set_str(vm, "COPYPOBJ_VAR",
                    from_name[0] ? from_name : (have_from ? "" : "PLATE"));
        var_set_num(vm, "COPYPOBJ_NEST_HIT", 1);
        var_set_str(vm, "NEST", nest);
        var_set_num(vm, "OK", 1);
        if (vm->trace)
          fprintf(vm->trace, "# copypobj key-miss nest=%s %s→%s\n", nestk, ka, kb);
        bump(vm); return 1;
      }
      if (strcmp(ka, kb) == 0) {
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_str(vm, "COPYPOBJ", plate);
        var_set_num(vm, "COPYPOBJ_N", 1);
        var_set_str(vm, "COPYPOBJ_NEST", nestk);
        var_set_str(vm, "COPYPOBJ_SRC", ka);
        var_set_str(vm, "COPYPOBJ_DST", kb);
        var_set_num(vm, "COPYPOBJ_FROM", have_from ? 1 : 0);
        var_set_str(vm, "COPYPOBJ_VAR",
                    from_name[0] ? from_name : (have_from ? "" : "PLATE"));
        var_set_num(vm, "COPYPOBJ_NEST_HIT", 1);
        var_set_str(vm, "NEST", nest);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_set(nest, kb, gr.str, 1, &hr) != 0) {
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "COPYPOBJ: set fail");
        var_set_str(vm, "ERR", hr.err[0] ? hr.err : "COPYPOBJ: set fail");
        var_set_num(vm, "COPYPOBJ_N", 0);
        bump(vm); return 1;
      }
      snprintf(nest, sizeof nest, "%s", hr.str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_path_set(plate, nestk, nest, 1, &hr) != 0) {
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "COPYPOBJ: write-back fail");
        var_set_str(vm, "ERR", hr.err[0] ? hr.err : "COPYPOBJ: write-back fail");
        var_set_num(vm, "COPYPOBJ_N", 0);
        bump(vm); return 1;
      }
      if (have_from && from_name[0] && strcmp(from_name, "LAST") != 0)
        var_set_str(vm, from_name, hr.str);
      else if (!have_from)
        var_set_str(vm, "PLATE", hr.str);
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_str(vm, "NEST", nest);
      var_set_str(vm, "MERGED", nest);
      var_set_str(vm, "COPYPOBJ", hr.str);
      var_set_str(vm, "DUPKEYOBJ", hr.str);
      var_set_str(vm, "NESTCOPY", hr.str);
      var_set_num(vm, "COPYPOBJ_N", 1);
      var_set_str(vm, "COPYPOBJ_NEST", nestk);
      var_set_str(vm, "COPYPOBJ_SRC", ka);
      var_set_str(vm, "COPYPOBJ_DST", kb);
      var_set_num(vm, "COPYPOBJ_FROM", have_from ? 1 : 0);
      var_set_str(vm, "COPYPOBJ_VAR",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "COPYPOBJ_NEST_HIT", 1);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# copypobj nest=%s %s→%s from=%d\n",
                nestk, ka, kb, have_from);
      bump(vm); return 1;
    }

    /* SWAPPOBJ */
    if (strcmp(ka, kb) == 0) {
      memset(&gr, 0, sizeof gr);
      has_a = (cubalc_host_json_get_raw(nest, ka, &gr) == 0) ? 1 : 0;
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = has_a;
      var_set_num(vm, "LAST_N", has_a);
      var_set_str(vm, "SWAPPOBJ", plate);
      var_set_num(vm, "SWAPPOBJ_N", has_a);
      var_set_str(vm, "SWAPPOBJ_NEST", nestk);
      var_set_str(vm, "SWAPPOBJ_A", ka);
      var_set_str(vm, "SWAPPOBJ_B", kb);
      var_set_num(vm, "SWAPPOBJ_FROM", have_from ? 1 : 0);
      var_set_str(vm, "SWAPPOBJ_VAR",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "SWAPPOBJ_NEST_HIT", 1);
      var_set_str(vm, "NEST", nest);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    memset(&gr, 0, sizeof gr);
    if (cubalc_host_json_get_raw(nest, ka, &gr) == 0) {
      has_a = 1;
      snprintf(ra, sizeof ra, "%s", gr.str);
    }
    memset(&gr, 0, sizeof gr);
    if (cubalc_host_json_get_raw(nest, kb, &gr) == 0) {
      has_b = 1;
      snprintf(rb, sizeof rb, "%s", gr.str);
    }
    if (!has_a && !has_b) {
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "SWAPPOBJ", plate);
      var_set_num(vm, "SWAPPOBJ_N", 0);
      var_set_str(vm, "SWAPPOBJ_NEST", nestk);
      var_set_str(vm, "SWAPPOBJ_A", ka);
      var_set_str(vm, "SWAPPOBJ_B", kb);
      var_set_num(vm, "SWAPPOBJ_FROM", have_from ? 1 : 0);
      var_set_str(vm, "SWAPPOBJ_VAR",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "SWAPPOBJ_NEST_HIT", 1);
      var_set_str(vm, "NEST", nest);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# swappobj both-miss nest=%s\n", nestk);
      bump(vm); return 1;
    }
    if (has_a && has_b) {
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_set(nest, ka, rb, 1, &hr) != 0) goto cubalc_swappobj_fail;
      snprintf(nest, sizeof nest, "%s", hr.str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_set(nest, kb, ra, 1, &hr) != 0) goto cubalc_swappobj_fail;
      snprintf(nest, sizeof nest, "%s", hr.str);
    } else if (has_a && !has_b) {
      memset(&dr, 0, sizeof dr);
      if (cubalc_host_json_del(nest, ka, &dr) != 0) goto cubalc_swappobj_fail;
      snprintf(nest, sizeof nest, "%s", dr.str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_set(nest, kb, ra, 1, &hr) != 0) goto cubalc_swappobj_fail;
      snprintf(nest, sizeof nest, "%s", hr.str);
    } else {
      memset(&dr, 0, sizeof dr);
      if (cubalc_host_json_del(nest, kb, &dr) != 0) goto cubalc_swappobj_fail;
      snprintf(nest, sizeof nest, "%s", dr.str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_json_set(nest, ka, rb, 1, &hr) != 0) goto cubalc_swappobj_fail;
      snprintf(nest, sizeof nest, "%s", hr.str);
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_path_set(plate, nestk, nest, 1, &hr) != 0) goto cubalc_swappobj_fail;
    if (have_from && from_name[0] && strcmp(from_name, "LAST") != 0)
      var_set_str(vm, from_name, hr.str);
    else if (!have_from)
      var_set_str(vm, "PLATE", hr.str);
    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = 1;
    var_set_num(vm, "LAST_N", 1);
    var_set_str(vm, "NEST", nest);
    var_set_str(vm, "MERGED", nest);
    var_set_str(vm, "SWAPPOBJ", hr.str);
    var_set_str(vm, "XCHGKEYOBJ", hr.str);
    var_set_str(vm, "NESTSWAP", hr.str);
    var_set_num(vm, "SWAPPOBJ_N", 1);
    var_set_str(vm, "SWAPPOBJ_NEST", nestk);
    var_set_str(vm, "SWAPPOBJ_A", ka);
    var_set_str(vm, "SWAPPOBJ_B", kb);
    var_set_num(vm, "SWAPPOBJ_FROM", have_from ? 1 : 0);
    var_set_str(vm, "SWAPPOBJ_VAR",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "SWAPPOBJ_NEST_HIT", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# swappobj nest=%s %s↔%s from=%d\n",
              nestk, ka, kb, have_from);
    bump(vm); return 1;
  cubalc_swappobj_fail:
    var_set_str(vm, "LAST", "");
    vm->last_str[0] = 0;
    vm->last_n = 0;
    var_set_num(vm, "LAST_N", 0);
    var_set_num(vm, "SWAPPOBJ_N", 0);
    var_set_num(vm, "SWAPPOBJ_FROM", have_from ? 1 : 0);
    var_set_num(vm, "OK", 0);
    {
      const char *e = hr.err[0] ? hr.err : (dr.err[0] ? dr.err : "SWAPPOBJ: fail");
      var_set_str(vm, "LAST_ERR", e);
      var_set_str(vm, "ERR", e);
    }
    bump(vm); return 1;
  }

  /* TOKVOBJ|NESTTOKV|OBJTOKV [FROM plate] nest — nest → key:val bag (TOKVP dual).
   * FROMKVOBJ|NESTFROMKV|BAGTONEST [FROM plate] nest [bag|LAST] —
   *   key=val bag → nested object write-back outer plate (FROMKVP dual).
   * Soft nest miss: TOKV empty bag NEST_HIT=0 · FROMKV creates nest from bag.
   * Usability: FREQ/LOOKUP bag bridge on nests without GETOBJ+TOKVP/FROMKVP+SETOBJ:
   *   TOKVOBJ "meta"
   *   FROMKVOBJ "stats" "hits:3\nmiss:1"
   *   FROMKVOBJ FROM PEER "cfg" FLAGMAP
   */
  if (kw(&L->cur,"TOKVOBJ") || kw(&L->cur,"NESTTOKV") || kw(&L->cur,"OBJTOKV") ||
      kw(&L->cur,"TOBAGOBJ") || kw(&L->cur,"NEST2KV") || kw(&L->cur,"MTOKVOBJ") ||
      kw(&L->cur,"FROMKVOBJ") || kw(&L->cur,"NESTFROMKV") || kw(&L->cur,"BAGTONEST") ||
      kw(&L->cur,"OBJFROMKV") || kw(&L->cur,"KVTONEST") || kw(&L->cur,"MFROMKVOBJ") ||
      kw(&L->cur,"NEST_FROMKV") || kw(&L->cur,"NEST_TOKV")) {
    char plate[CUBALC_HOST_STR_MAX], nestk[96], nest[CUBALC_HOST_STR_MAX];
    char bag[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result ngr, hr, wr;
    int have_from = 0, nest_hit = 0, is_from = 0;
    Var *pv;
    const char *v;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "FROMKVOBJ") == 0 || strcmp(op0, "NESTFROMKV") == 0 ||
        strcmp(op0, "BAGTONEST") == 0 || strcmp(op0, "OBJFROMKV") == 0 ||
        strcmp(op0, "KVTONEST") == 0 || strcmp(op0, "MFROMKVOBJ") == 0 ||
        strcmp(op0, "NEST_FROMKV") == 0)
      is_from = 1;

    lex_next(L);
    plate[0] = 0; nestk[0] = 0; nest[0] = 0; bag[0] = 0;
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
        fail(vm, is_from ? "FROMKVOBJ nest [bag] — need nest key"
                         : "TOKVOBJ nest — need nest key");
        return -1;
      }
      if (resolve_str_arg(vm, L, nestk, sizeof nestk) != 0)
        nestk[0] = 0;
    } else {
      fail(vm, is_from ? "FROMKVOBJ nest [bag] — need nest key"
                       : "TOKVOBJ nest — need nest key");
      return -1;
    }

    /* FROMKVOBJ optional bag (else LAST); allow trailing FROM after nest for TOKVOBJ */
    if (is_from) {
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        if (L->cur.kind == TK_IDENT &&
            (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
             kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
             kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"ASSERT") ||
             kw(&L->cur,"LET") || kw(&L->cur,"PRINT") || kw(&L->cur,"SYS") ||
             kw(&L->cur,"SETP") || kw(&L->cur,"TOKVP") || kw(&L->cur,"FROMKVP") ||
             kw(&L->cur,"TOKVOBJ") || kw(&L->cur,"FROMKVOBJ"))) {
          /* stop — use LAST */
        } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
                   L->cur.kind == TK_LPAREN) {
          long n = parse_expr(vm, L);
          snprintf(bag, sizeof bag, "%ld", n);
        } else if (resolve_str_arg(vm, L, bag, sizeof bag) != 0) {
          bag[0] = 0;
        }
      }
      if (!bag[0])
        snprintf(bag, sizeof bag, "%s", vm->last_str);
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
      var_set_str(vm, "LAST_ERR", is_from ? "FROMKVOBJ: empty nest" : "TOKVOBJ: empty nest");
      var_set_str(vm, "ERR", is_from ? "FROMKVOBJ: empty nest" : "TOKVOBJ: empty nest");
      var_set_num(vm, "TOKVOBJ_N", 0);
      var_set_num(vm, "FROMKVOBJ_N", 0);
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

    if (!is_from) {
      /* TOKVOBJ */
      long nkeys = 0;
      char out[CUBALC_HOST_STR_MAX];
      out[0] = 0;
      if (nest_hit) {
        memset(&hr, 0, sizeof hr);
        if (cubalc_host_json_to_kv(nest, &hr) != 0) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "TOKVOBJ_N", 0);
          var_set_num(vm, "TOKVOBJ_FROM", have_from ? 1 : 0);
          var_set_num(vm, "TOKVOBJ_NEST_HIT", 1);
          var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "TOKVOBJ: fail");
          var_set_str(vm, "ERR", hr.err[0] ? hr.err : "TOKVOBJ: fail");
          var_set_num(vm, "OK", 0);
          bump(vm); return 1;
        }
        snprintf(out, sizeof out, "%s", hr.str);
        nkeys = hr.n;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = nkeys;
      var_set_num(vm, "LAST_N", nkeys);
      var_set_str(vm, "TOKVOBJ", out);
      var_set_str(vm, "NESTTOKV", out);
      var_set_str(vm, "TOBAGOBJ", out);
      var_set_num(vm, "TOKVOBJ_N", nkeys);
      var_set_str(vm, "TOKVOBJ_NEST", nestk);
      var_set_num(vm, "TOKVOBJ_FROM", have_from ? 1 : 0);
      var_set_str(vm, "TOKVOBJ_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "TOKVOBJ_NEST_HIT", nest_hit ? 1 : 0);
      var_set_str(vm, "NEST", nest_hit ? nest : "");
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# tokvobj nest=%s n=%ld hit=%d from=%d\n",
                nestk, nkeys, nest_hit, have_from);
      bump(vm); return 1;
    }

    /* FROMKVOBJ — bag → nest object, write-back outer */
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_from_kv(bag, &hr) != 0) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "FROMKVOBJ_N", 0);
      var_set_num(vm, "FROMKVOBJ_FROM", have_from ? 1 : 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "FROMKVOBJ: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "FROMKVOBJ: fail");
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    snprintf(nest, sizeof nest, "%s", hr.str);
    memset(&wr, 0, sizeof wr);
    if (cubalc_host_json_set(plate, nestk, nest, 1, &wr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "FROMKVOBJ: write-back fail");
      var_set_str(vm, "ERR", wr.err[0] ? wr.err : "FROMKVOBJ: write-back fail");
      var_set_num(vm, "FROMKVOBJ_N", 0);
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
    var_set_str(vm, "FROMKVOBJ", wr.str);
    var_set_str(vm, "NESTFROMKV", wr.str);
    var_set_str(vm, "BAGTONEST", wr.str);
    var_set_num(vm, "FROMKVOBJ_N", hr.n);
    var_set_str(vm, "FROMKVOBJ_NEST", nestk);
    var_set_num(vm, "FROMKVOBJ_FROM", have_from ? 1 : 0);
    var_set_str(vm, "FROMKVOBJ_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "FROMKVOBJ_NEST_HIT", nest_hit ? 1 : 0);
    var_set_num(vm, "FROMKVOBJ_CREATED", nest_hit ? 0 : 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# fromkvobj nest=%s n=%ld hit=%d from=%d\n",
              nestk, hr.n, nest_hit, have_from);
    bump(vm); return 1;
  }

  return 0;
}
