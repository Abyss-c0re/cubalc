/* CubalC lang — flat query shard
 */
#include "lang/cubalc_lang_ops_core_flat_has.h"
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

int cubalc_lang_ops_core_try_flat_has(VM *vm, Lex *L) {
  /* HASFLAT|ANYFLAT [FROM plate] [needle]
   * COUNTFLAT|NFLAT [FROM plate] [needle]
   * Soft leaf-path presence / count by needle. Read-only. Empty needle → all leaves.
   * HASFLAT: LAST_N = 0|1 · HASFLAT_N = match count
   * COUNTFLAT: LAST_N = match count
   * Usability: structure probe without GREPFLAT+LINES/EMPTY glue:
   *   HASFLAT "error"
   *   COUNTFLAT "score"
   *   HASFLAT FROM PEER "tls"
   */
  if (kw(&L->cur,"HASFLAT") || kw(&L->cur,"ANYFLAT") || kw(&L->cur,"MHASFLAT") ||
      kw(&L->cur,"PLATE_HASFLAT") || kw(&L->cur,"HITFLAT") ||
      kw(&L->cur,"COUNTFLAT") || kw(&L->cur,"NFLAT") || kw(&L->cur,"MCOUNTFLAT") ||
      kw(&L->cur,"PLATE_COUNTFLAT") || kw(&L->cur,"NCFLAT")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, is_count = 0;
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "COUNTFLAT") == 0 || strcmp(op0, "NFLAT") == 0 ||
        strcmp(op0, "MCOUNTFLAT") == 0 || strcmp(op0, "PLATE_COUNTFLAT") == 0 ||
        strcmp(op0, "NCFLAT") == 0)
      is_count = 1;

    lex_next(L);
    plate[0] = 0; needle[0] = 0; from_name[0] = 0; from_src[0] = 0;

    if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
        kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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

    if (L->cur.kind == TK_STR) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(needle, sizeof needle, "%ld", kv);
    } else if (L->cur.kind == TK_IDENT &&
               !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") &&
               !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
               !kw(&L->cur,"PASS") && !kw(&L->cur,"SETP") && !kw(&L->cur,"INCP") &&
               !kw(&L->cur,"HASFLAT") && !kw(&L->cur,"COUNTFLAT") && !kw(&L->cur,"IF")) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
    }

    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE"))) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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
      const char *b = from_src;
      while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
      if (*b == '{')
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
    if (cubalc_host_json_leaf_count(plate, needle, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "HASFLAT: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "HASFLAT: fail");
      var_set_num(vm, "HASFLAT_N", 0);
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    /* Read-only: keep plate in LAST string when count mode uses LAST_N for count.
     * HASFLAT: LAST_N = 0|1 presence; COUNTFLAT: LAST_N = match count. */
    var_set_str(vm, "LAST", plate);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
    if (is_count) {
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "COUNTFLAT", hr.str);
      var_set_num(vm, "COUNTFLAT_N", hr.n);
      var_set_str(vm, "COUNTFLAT_NEEDLE", needle);
      var_set_num(vm, "COUNTFLAT_FROM", have_from ? 1 : 0);
      var_set_str(vm, "COUNTFLAT_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    } else {
      long hit = (hr.n > 0) ? 1 : 0;
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_str(vm, "HASFLAT", hr.str);
      var_set_str(vm, "ANYFLAT", hr.str);
      var_set_num(vm, "HASFLAT_N", hr.n); /* match count */
      var_set_num(vm, "HASFLAT_HIT", hit);
      var_set_str(vm, "HASFLAT_NEEDLE", needle);
      var_set_num(vm, "HASFLAT_FROM", have_from ? 1 : 0);
      var_set_str(vm, "HASFLAT_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%ld needle=%s from=%d\n",
              is_count ? "countflat" : "hasflat",
              hr.n, needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* HASFLATN|ANYFLATN [FROM plate] [needle]
   * COUNTFLATN|NFLATN [FROM plate] [needle]
   * Soft pure-int leaf presence / count by path needle. Read-only.
   * Non-int matching leaves skipped. Empty needle → all pure-int leaves.
   * HASFLATN: LAST_N = 0|1 · HASFLATN_N = pure-int match count
   * COUNTFLATN: LAST_N = pure-int match count
   * Soft twin of NEEDFLATN. Usability: typed probe without GETFLATN+IF:
   *   HASFLATN "port"
   *   COUNTFLATN "score"
   *   HASFLATN FROM PEER "hits"
   */
  if (kw(&L->cur,"HASFLATN") || kw(&L->cur,"ANYFLATN") || kw(&L->cur,"MHASFLATN") ||
      kw(&L->cur,"PLATE_HASFLATN") || kw(&L->cur,"HITFLATN") || kw(&L->cur,"ANYNUMFLAT") ||
      kw(&L->cur,"COUNTFLATN") || kw(&L->cur,"NFLATN") || kw(&L->cur,"MCOUNTFLATN") ||
      kw(&L->cur,"PLATE_COUNTFLATN") || kw(&L->cur,"NCFLATN") || kw(&L->cur,"LEAFCOUNTN") ||
      kw(&L->cur,"NUMCOUNTFLAT") || kw(&L->cur,"INTCOUNTFLAT")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, is_count = 0;
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "COUNTFLATN") == 0 || strcmp(op0, "NFLATN") == 0 ||
        strcmp(op0, "MCOUNTFLATN") == 0 || strcmp(op0, "PLATE_COUNTFLATN") == 0 ||
        strcmp(op0, "NCFLATN") == 0 || strcmp(op0, "LEAFCOUNTN") == 0 ||
        strcmp(op0, "NUMCOUNTFLAT") == 0 || strcmp(op0, "INTCOUNTFLAT") == 0)
      is_count = 1;

    lex_next(L);
    plate[0] = 0; needle[0] = 0; from_name[0] = 0; from_src[0] = 0;

    if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
        kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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

    if (L->cur.kind == TK_STR) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(needle, sizeof needle, "%ld", kv);
    } else if (L->cur.kind == TK_IDENT &&
               !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") &&
               !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
               !kw(&L->cur,"PASS") && !kw(&L->cur,"SETP") && !kw(&L->cur,"INCP") &&
               !kw(&L->cur,"HASFLATN") && !kw(&L->cur,"COUNTFLATN") &&
               !kw(&L->cur,"HASFLAT") && !kw(&L->cur,"IF")) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
    }

    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE"))) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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
      const char *b = from_src;
      while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
      if (*b == '{')
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
    if (cubalc_host_json_leaf_countn(plate, needle, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "HASFLATN: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "HASFLATN: fail");
      var_set_num(vm, "HASFLATN_N", 0);
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    var_set_str(vm, "LAST", plate);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
    if (is_count) {
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "COUNTFLATN", hr.str);
      var_set_num(vm, "COUNTFLATN_N", hr.n);
      var_set_str(vm, "COUNTFLATN_NEEDLE", needle);
      var_set_num(vm, "COUNTFLATN_FROM", have_from ? 1 : 0);
      var_set_str(vm, "COUNTFLATN_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    } else {
      long hit = (hr.n > 0) ? 1 : 0;
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_str(vm, "HASFLATN", hr.str);
      var_set_str(vm, "ANYFLATN", hr.str);
      var_set_num(vm, "HASFLATN_N", hr.n);
      var_set_num(vm, "HASFLATN_HIT", hit);
      var_set_str(vm, "HASFLATN_NEEDLE", needle);
      var_set_num(vm, "HASFLATN_FROM", have_from ? 1 : 0);
      var_set_str(vm, "HASFLATN_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%ld needle=%s from=%d\n",
              is_count ? "countflatn" : "hasflatn",
              hr.n, needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* PATHSFLAT|MATCHPATHS [FROM plate] [needle]
   * VALSFLAT|MATCHVALS [FROM plate] [needle]
   * Matching leaf paths or values as newline bag → LAST · LAST_N = count.
   * Empty needle → all leaves. Read-only. Soft empty bag.
   * Usability: peel GREPFLAT without BEFORE/AFTER · EACH LINE walk:
   *   PATHSFLAT "score"
   *   VALSFLAT "hits"
   *   PATHSFLAT FROM PEER "tls"
   */
  if (kw(&L->cur,"PATHSFLAT") || kw(&L->cur,"MATCHPATHS") || kw(&L->cur,"MPATHSFLAT") ||
      kw(&L->cur,"PLATE_PATHSFLAT") || kw(&L->cur,"PATHMATCH") ||
      kw(&L->cur,"VALSFLAT") || kw(&L->cur,"MATCHVALS") || kw(&L->cur,"MVALSFLAT") ||
      kw(&L->cur,"PLATE_VALSFLAT") || kw(&L->cur,"VALMATCH") || kw(&L->cur,"LEAFVALS")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, want_vals = 0;
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "VALSFLAT") == 0 || strcmp(op0, "MATCHVALS") == 0 ||
        strcmp(op0, "MVALSFLAT") == 0 || strcmp(op0, "PLATE_VALSFLAT") == 0 ||
        strcmp(op0, "VALMATCH") == 0 || strcmp(op0, "LEAFVALS") == 0)
      want_vals = 1;

    lex_next(L);
    plate[0] = 0; needle[0] = 0; from_name[0] = 0; from_src[0] = 0;

    if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
        kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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

    if (L->cur.kind == TK_STR) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(needle, sizeof needle, "%ld", kv);
    } else if (L->cur.kind == TK_IDENT &&
               !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") &&
               !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
               !kw(&L->cur,"PASS") && !kw(&L->cur,"SETP") && !kw(&L->cur,"INCP") &&
               !kw(&L->cur,"PATHSFLAT") && !kw(&L->cur,"VALSFLAT") && !kw(&L->cur,"HASFLAT")) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
    }

    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE"))) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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
      const char *b = from_src;
      while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
      if (*b == '{')
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
    if (cubalc_host_json_leaf_match_bag(plate, needle, want_vals, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "PATHSFLAT: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "PATHSFLAT: fail");
      var_set_num(vm, "PATHSFLAT_N", 0);
      var_set_str(vm, "LAST", "");
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    if (want_vals) {
      var_set_str(vm, "VALSFLAT", hr.str);
      var_set_str(vm, "MATCHVALS", hr.str);
      var_set_num(vm, "VALSFLAT_N", hr.n);
      var_set_str(vm, "VALSFLAT_NEEDLE", needle);
      var_set_num(vm, "VALSFLAT_FROM", have_from ? 1 : 0);
      var_set_str(vm, "VALSFLAT_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    } else {
      var_set_str(vm, "PATHSFLAT", hr.str);
      var_set_str(vm, "MATCHPATHS", hr.str);
      var_set_num(vm, "PATHSFLAT_N", hr.n);
      var_set_str(vm, "PATHSFLAT_NEEDLE", needle);
      var_set_num(vm, "PATHSFLAT_FROM", have_from ? 1 : 0);
      var_set_str(vm, "PATHSFLAT_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%ld needle=%s from=%d\n",
              want_vals ? "valsflat" : "pathsflat",
              hr.n, needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* PATHSFLATN|MATCHPATHSN [FROM plate] [needle]
   * VALSFLATN|MATCHVALSN [FROM plate] [needle]
   * Pure-int matching leaf paths or values as newline bag → LAST · LAST_N=count.
   * Non-int matching leaves skipped. Empty needle → all pure-int leaves.
   * Read-only. Soft empty bag.
   * Usability: numeric EACH LINE / SUM without string pollution from VALSFLAT:
   *   PATHSFLATN "score"
   *   VALSFLATN "hits"
   *   VALSFLATN FROM PEER "tls"
   */
  if (kw(&L->cur,"PATHSFLATN") || kw(&L->cur,"MATCHPATHSN") || kw(&L->cur,"MPATHSFLATN") ||
      kw(&L->cur,"PLATE_PATHSFLATN") || kw(&L->cur,"PATHMATCHN") || kw(&L->cur,"NUMPATHSFLAT") ||
      kw(&L->cur,"VALSFLATN") || kw(&L->cur,"MATCHVALSN") || kw(&L->cur,"MVALSFLATN") ||
      kw(&L->cur,"PLATE_VALSFLATN") || kw(&L->cur,"VALMATCHN") || kw(&L->cur,"LEAFVALSN") ||
      kw(&L->cur,"NUMVALSFLAT") || kw(&L->cur,"INTVALSFLAT")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, want_vals = 0;
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "VALSFLATN") == 0 || strcmp(op0, "MATCHVALSN") == 0 ||
        strcmp(op0, "MVALSFLATN") == 0 || strcmp(op0, "PLATE_VALSFLATN") == 0 ||
        strcmp(op0, "VALMATCHN") == 0 || strcmp(op0, "LEAFVALSN") == 0 ||
        strcmp(op0, "NUMVALSFLAT") == 0 || strcmp(op0, "INTVALSFLAT") == 0)
      want_vals = 1;

    lex_next(L);
    plate[0] = 0; needle[0] = 0; from_name[0] = 0; from_src[0] = 0;

    if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
        kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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

    if (L->cur.kind == TK_STR) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(needle, sizeof needle, "%ld", kv);
    } else if (L->cur.kind == TK_IDENT &&
               !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") &&
               !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
               !kw(&L->cur,"PASS") && !kw(&L->cur,"SETP") && !kw(&L->cur,"INCP") &&
               !kw(&L->cur,"PATHSFLATN") && !kw(&L->cur,"VALSFLATN") &&
               !kw(&L->cur,"PATHSFLAT") && !kw(&L->cur,"HASFLATN") && !kw(&L->cur,"IF")) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
    }

    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE"))) {
      lex_next(L);
      have_from = 1;
      if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(from_src, sizeof from_src, "%s", vm->last_str);
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
      const char *b = from_src;
      while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
      if (*b == '{')
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
    if (cubalc_host_json_leaf_match_bagn(plate, needle, want_vals, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "PATHSFLATN: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "PATHSFLATN: fail");
      var_set_num(vm, "PATHSFLATN_N", 0);
      var_set_str(vm, "LAST", "");
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    if (want_vals) {
      var_set_str(vm, "VALSFLATN", hr.str);
      var_set_str(vm, "MATCHVALSN", hr.str);
      var_set_num(vm, "VALSFLATN_N", hr.n);
      var_set_str(vm, "VALSFLATN_NEEDLE", needle);
      var_set_num(vm, "VALSFLATN_FROM", have_from ? 1 : 0);
      var_set_str(vm, "VALSFLATN_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    } else {
      var_set_str(vm, "PATHSFLATN", hr.str);
      var_set_str(vm, "MATCHPATHSN", hr.str);
      var_set_num(vm, "PATHSFLATN_N", hr.n);
      var_set_str(vm, "PATHSFLATN_NEEDLE", needle);
      var_set_num(vm, "PATHSFLATN_FROM", have_from ? 1 : 0);
      var_set_str(vm, "PATHSFLATN_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%ld needle=%s from=%d\n",
              want_vals ? "valsflatn" : "pathsflatn",
              hr.n, needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  return 0;
}
