/* CubalC flat nest shard */
#include "lang/cubalc_lang_ops_core_flat_nest_kv.h"
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

int cubalc_lang_ops_core_try_flat_nest_kv(VM *vm, Lex *L) {
  /* TOKVP|TOBAGP [FROM plate] — plate → key:val bag (JSONTOKV dual).
   * FROMKVP|BAGTOP [bag|LAST] — key=val bag → plate object (JSONFROMKV dual).
   * Optional INTO name on FROMKVP write-back. Soft empty bag/plate.
   * Usability: LOOKUP/FREQ bag bridge without SYS JSONTOKV/JSONFROMKV:
   *   TOKVP FROM PEER
   *   FROMKVP FLAGMAP
   *   FROMKVP LAST INTO session
   */
  if (kw(&L->cur,"TOKVP") || kw(&L->cur,"TOBAGP") || kw(&L->cur,"MTOKVP") ||
      kw(&L->cur,"PLATE_TOKV") || kw(&L->cur,"PTOKV") || kw(&L->cur,"PLATE2KVP") ||
      kw(&L->cur,"FROMKVP") || kw(&L->cur,"BAGTOP") || kw(&L->cur,"MFROMKVP") ||
      kw(&L->cur,"PLATE_FROMKV") || kw(&L->cur,"KVTOP") || kw(&L->cur,"BAG2PLATE")) {
    char plate[CUBALC_HOST_STR_MAX], bag[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX], into_name[96];
    cubalc_host_result hr;
    int have_from = 0, is_fromkv = 0, have_into = 0;
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "FROMKVP") == 0 || strcmp(op0, "BAGTOP") == 0 ||
        strcmp(op0, "MFROMKVP") == 0 || strcmp(op0, "PLATE_FROMKV") == 0 ||
        strcmp(op0, "KVTOP") == 0 || strcmp(op0, "BAG2PLATE") == 0)
      is_fromkv = 1;

    lex_next(L);
    plate[0] = 0; bag[0] = 0;
    from_name[0] = 0; from_src[0] = 0; into_name[0] = 0;

    if (!is_fromkv) {
      /* TOKVP [FROM plate] */
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
      if (cubalc_host_json_to_kv(plate, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "TOKVP_N", 0);
        var_set_num(vm, "TOKVP_FROM", have_from ? 1 : 0);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "TOKVP: fail");
        var_set_str(vm, "ERR", hr.err[0] ? hr.err : "TOKVP: fail");
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "TOKVP", hr.str);
      var_set_str(vm, "TOBAGP", hr.str);
      var_set_num(vm, "TOKVP_N", hr.n);
      var_set_num(vm, "JSONTOKV_N", hr.n);
      var_set_num(vm, "TOKVP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "TOKVP_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# tokvp n=%ld from=%d\n", hr.n, have_from);
      bump(vm); return 1;
    }

    /* FROMKVP [bag|LAST] [INTO name] · INTO may lead */
    if (kw(&L->cur,"INTO") || kw(&L->cur,"AS") || kw(&L->cur,"TOVAR") ||
        kw(&L->cur,"BIND") || kw(&L->cur,"STOREIN")) {
      lex_next(L);
      have_into = 1;
      if (L->cur.kind == TK_IDENT) {
        snprintf(into_name, sizeof into_name, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "FROMKVP [bag] INTO name — need var name");
        return -1;
      }
    }
    if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
        L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"INTO") || kw(&L->cur,"AS") || kw(&L->cur,"TOVAR") ||
           kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"ASSERT") ||
           kw(&L->cur,"LET") || kw(&L->cur,"PRINT") || kw(&L->cur,"SYS") ||
           kw(&L->cur,"SETP") || kw(&L->cur,"TOKVP") || kw(&L->cur,"FROMKVP"))) {
        /* stop — bare uses LAST */
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
    if (!have_into && (kw(&L->cur,"INTO") || kw(&L->cur,"AS") ||
                       kw(&L->cur,"TOVAR") || kw(&L->cur,"BIND") ||
                       kw(&L->cur,"STOREIN"))) {
      lex_next(L);
      have_into = 1;
      if (L->cur.kind == TK_IDENT) {
        snprintf(into_name, sizeof into_name, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "FROMKVP bag INTO name — need var name");
        return -1;
      }
    }

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_from_kv(bag, &hr) != 0) {
      var_set_str(vm, "LAST", "{}");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "{}");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "FROMKVP_N", 0);
      var_set_num(vm, "FROMKVP_INTO", have_into ? 1 : 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "FROMKVP: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "FROMKVP: fail");
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }

    if (have_into && into_name[0])
      var_set_str(vm, into_name, hr.str);

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "FROMKVP", hr.str);
    var_set_str(vm, "BAGTOP", hr.str);
    var_set_num(vm, "FROMKVP_N", hr.n);
    var_set_num(vm, "JSONFROMKV_N", hr.n);
    var_set_num(vm, "FROMKVP_INTO", have_into ? 1 : 0);
    var_set_str(vm, "FROMKVP_VAR", into_name);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# fromkvp n=%ld into=%d var=%s\n",
              hr.n, have_into, into_name[0] ? into_name : "-");
    bump(vm); return 1;
  }
  return 0;
}
