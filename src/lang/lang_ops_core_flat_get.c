/* CubalC lang — flat query shard
 */
#include "lang/cubalc_lang_ops_core_flat_get.h"
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

int cubalc_lang_ops_core_try_flat_get(VM *vm, Lex *L) {
  /* GETFLAT|FIRSTFLAT [FROM plate] needle [OR|DEFAULT fallback]
   * First leaf whose path contains needle → LAST=value · LAST_N=hit 0|1.
   * GETFLAT_PATH = winning path on hit. Soft miss → OR fallback or empty.
   * Empty needle → first leaf. Read-only.
   * Usability: nest peel by needle without PATHSFLAT+NTH+GETP glue:
   *   GETFLAT "port" OR "8080"
   *   GETFLAT FROM PEER "role" OR "worker"
   */
  if (kw(&L->cur,"GETFLAT") || kw(&L->cur,"FIRSTFLAT") || kw(&L->cur,"PEELFLAT") ||
      kw(&L->cur,"MGETFLAT") || kw(&L->cur,"PLATE_GETFLAT") || kw(&L->cur,"LEAFGET") ||
      kw(&L->cur,"GETLEAFP")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192], fallback[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, have_fb = 0;
    Var *pv;

    lex_next(L);
    plate[0] = 0; needle[0] = 0; fallback[0] = 0; from_name[0] = 0; from_src[0] = 0;

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
               !kw(&L->cur,"FROM") && !kw(&L->cur,"OR") && !kw(&L->cur,"DEFAULT") &&
               !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
               !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") && !kw(&L->cur,"PASS") &&
               !kw(&L->cur,"GETFLAT") && !kw(&L->cur,"PATHSFLAT") && !kw(&L->cur,"HASFLAT")) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
    }

    if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
        kw(&L->cur,"FALLBACK")) {
      lex_next(L);
      have_fb = 1;
      if (resolve_str_arg(vm, L, fallback, sizeof fallback) != 0)
        fallback[0] = 0;
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

    if (!have_fb && (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
                     kw(&L->cur,"FALLBACK"))) {
      lex_next(L);
      have_fb = 1;
      if (resolve_str_arg(vm, L, fallback, sizeof fallback) != 0)
        fallback[0] = 0;
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
    if (cubalc_host_json_leaf_get(plate, needle, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "GETFLAT: fail");
      var_set_str(vm, "ERR", "GETFLAT: fail");
      var_set_num(vm, "GETFLAT_HIT", 0);
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    if (hr.n == 0 && have_fb) {
      var_set_str(vm, "LAST", fallback);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", fallback);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "GETFLAT", fallback);
      var_set_str(vm, "GETFLAT_PATH", "");
      var_set_num(vm, "GETFLAT_HIT", 0);
      var_set_num(vm, "GETFLAT_FALLBACK", 1);
    } else if (hr.n == 0) {
      var_set_str(vm, "LAST", "");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "GETFLAT", "");
      var_set_str(vm, "GETFLAT_PATH", "");
      var_set_num(vm, "GETFLAT_HIT", 0);
      var_set_num(vm, "GETFLAT_FALLBACK", 0);
    } else {
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_str(vm, "GETFLAT", hr.str);
      var_set_str(vm, "FIRSTFLAT", hr.str);
      var_set_str(vm, "GETFLAT_PATH", hr.err); /* path side-channel */
      var_set_num(vm, "GETFLAT_HIT", 1);
      var_set_num(vm, "GETFLAT_FALLBACK", 0);
    }
    var_set_str(vm, "GETFLAT_NEEDLE", needle);
    var_set_num(vm, "GETFLAT_FROM", have_from ? 1 : 0);
    var_set_str(vm, "GETFLAT_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# getflat hit=%ld path=%s needle=%s from=%d\n",
              hr.n, hr.err[0] ? hr.err : "(miss)", needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* LASTFLAT|ENDFLAT [FROM plate] needle [OR|DEFAULT fallback]
   * Last leaf whose path contains needle → LAST=value · LAST_N=hit 0|1.
   * LASTFLAT_PATH = winning path on hit. Soft miss → OR fallback or empty.
   * Empty needle → last leaf. Read-only.
   * Usability: newest-wins nest peel without REVL+GETFLAT glue:
   *   LASTFLAT "port" OR "8080"
   *   LASTFLAT FROM PEER "role" OR "worker"
   */
  if (kw(&L->cur,"LASTFLAT") || kw(&L->cur,"ENDFLAT") || kw(&L->cur,"TAILFLAT") ||
      kw(&L->cur,"MLASTFLAT") || kw(&L->cur,"PLATE_LASTFLAT") || kw(&L->cur,"LEAFGETL") ||
      kw(&L->cur,"GETLEAFL") || kw(&L->cur,"LASTLEAFP")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192], fallback[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, have_fb = 0;
    Var *pv;

    lex_next(L);
    plate[0] = 0; needle[0] = 0; fallback[0] = 0; from_name[0] = 0; from_src[0] = 0;

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
               !kw(&L->cur,"FROM") && !kw(&L->cur,"OR") && !kw(&L->cur,"DEFAULT") &&
               !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
               !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") && !kw(&L->cur,"PASS") &&
               !kw(&L->cur,"LASTFLAT") && !kw(&L->cur,"GETFLAT") && !kw(&L->cur,"HASFLAT") &&
               !kw(&L->cur,"LASTFLATN")) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
    }

    if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
        kw(&L->cur,"FALLBACK")) {
      lex_next(L);
      have_fb = 1;
      if (resolve_str_arg(vm, L, fallback, sizeof fallback) != 0)
        fallback[0] = 0;
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

    if (!have_fb && (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
                     kw(&L->cur,"FALLBACK"))) {
      lex_next(L);
      have_fb = 1;
      if (resolve_str_arg(vm, L, fallback, sizeof fallback) != 0)
        fallback[0] = 0;
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
    if (cubalc_host_json_leaf_get_last(plate, needle, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "LASTFLAT: fail");
      var_set_str(vm, "ERR", "LASTFLAT: fail");
      var_set_num(vm, "LASTFLAT_HIT", 0);
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    if (hr.n == 0 && have_fb) {
      var_set_str(vm, "LAST", fallback);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", fallback);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "LASTFLAT", fallback);
      var_set_str(vm, "LASTFLAT_PATH", "");
      var_set_num(vm, "LASTFLAT_HIT", 0);
      var_set_num(vm, "LASTFLAT_FALLBACK", 1);
    } else if (hr.n == 0) {
      var_set_str(vm, "LAST", "");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "LASTFLAT", "");
      var_set_str(vm, "LASTFLAT_PATH", "");
      var_set_num(vm, "LASTFLAT_HIT", 0);
      var_set_num(vm, "LASTFLAT_FALLBACK", 0);
    } else {
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_str(vm, "LASTFLAT", hr.str);
      var_set_str(vm, "ENDFLAT", hr.str);
      var_set_str(vm, "LASTFLAT_PATH", hr.err);
      var_set_num(vm, "LASTFLAT_HIT", 1);
      var_set_num(vm, "LASTFLAT_FALLBACK", 0);
    }
    var_set_str(vm, "LASTFLAT_NEEDLE", needle);
    var_set_num(vm, "LASTFLAT_FROM", have_from ? 1 : 0);
    var_set_str(vm, "LASTFLAT_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# lastflat hit=%ld path=%s needle=%s from=%d\n",
              hr.n, hr.err[0] ? hr.err : "(miss)", needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* NTHFLAT|INDEXFLAT [FROM plate] needle index [OR|DEFAULT fallback]
   * 0-based Nth leaf whose path contains needle → LAST=value · LAST_N=hit 0|1.
   * Soft miss → OR string fallback or empty. NTHFLAT_PATH / NTHFLAT_I side channels.
   * Usability: index nest peels without PATHSFLAT+NTH+GETP glue:
   *   NTHFLAT "role" 0
   *   NTHFLAT "role" 1 OR "worker"
   *   NTHFLAT FROM PEER "name" 0
   */
  /* NTHLEAF reserved for CLASS EXTEND leaf-index (lang_ops_flow). Cube is SoT. */
  if (kw(&L->cur,"NTHFLAT") || kw(&L->cur,"INDEXFLAT") || kw(&L->cur,"ATFLAT") ||
      kw(&L->cur,"MNTHFLAT") || kw(&L->cur,"PLATE_NTHFLAT") || kw(&L->cur,"LEAFNTH") ||
      kw(&L->cur,"GETFLATNTH")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192], fallback[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, have_fb = 0, have_idx = 0;
    long idx = 0;
    Var *pv;

    lex_next(L);
    plate[0] = 0; needle[0] = 0; fallback[0] = 0; from_name[0] = 0; from_src[0] = 0;

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
    } else if (L->cur.kind == TK_IDENT &&
               !kw(&L->cur,"FROM") && !kw(&L->cur,"OR") && !kw(&L->cur,"DEFAULT") &&
               !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
               !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") && !kw(&L->cur,"PASS") &&
               !kw(&L->cur,"NTHFLAT") && !kw(&L->cur,"NTHFLATN") && !kw(&L->cur,"GETFLAT") &&
               !kw(&L->cur,"IF")) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
    }

    /* index — literal only (do not parse_expr: would swallow OR fallback) */
    if (L->cur.kind == TK_NUM) {
      idx = (long)L->cur.num;
      have_idx = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_MINUS) {
      lex_next(L);
      if (L->cur.kind == TK_NUM) {
        idx = -(long)L->cur.num;
        have_idx = 1;
        lex_next(L);
      }
    } else if (L->cur.kind == TK_LPAREN) {
      idx = parse_expr(vm, L);
      have_idx = 1;
    } else if (L->cur.kind == TK_IDENT) {
      pv = var_get(vm, L->cur.text, 0);
      if (pv && !pv->is_str) {
        idx = pv->val;
        have_idx = 1;
        lex_next(L);
      }
    }

    if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
        kw(&L->cur,"FALLBACK")) {
      lex_next(L);
      have_fb = 1;
      if (resolve_str_arg(vm, L, fallback, sizeof fallback) != 0)
        fallback[0] = 0;
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

    if (!have_fb && (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
                     kw(&L->cur,"FALLBACK"))) {
      lex_next(L);
      have_fb = 1;
      if (resolve_str_arg(vm, L, fallback, sizeof fallback) != 0)
        fallback[0] = 0;
    }

    if (!have_idx) {
      fail(vm, "NTHFLAT [FROM plate] needle index [OR def] — need 0-based index");
      return 1;
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
    if (cubalc_host_json_leaf_get_nth(plate, needle, idx, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "NTHFLAT: fail");
      var_set_str(vm, "ERR", "NTHFLAT: fail");
      var_set_num(vm, "NTHFLAT_HIT", 0);
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    if (hr.n == 0 && have_fb) {
      var_set_str(vm, "LAST", fallback);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", fallback);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "NTHFLAT", fallback);
      var_set_str(vm, "NTHFLAT_PATH", "");
      var_set_num(vm, "NTHFLAT_HIT", 0);
      var_set_num(vm, "NTHFLAT_FALLBACK", 1);
    } else if (hr.n == 0) {
      var_set_str(vm, "LAST", "");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "NTHFLAT", "");
      var_set_str(vm, "NTHFLAT_PATH", "");
      var_set_num(vm, "NTHFLAT_HIT", 0);
      var_set_num(vm, "NTHFLAT_FALLBACK", 0);
    } else {
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_str(vm, "NTHFLAT", hr.str);
      var_set_str(vm, "INDEXFLAT", hr.str);
      var_set_str(vm, "NTHFLAT_PATH", hr.err);
      var_set_num(vm, "NTHFLAT_HIT", 1);
      var_set_num(vm, "NTHFLAT_FALLBACK", 0);
    }
    var_set_num(vm, "NTHFLAT_I", idx);
    var_set_str(vm, "NTHFLAT_NEEDLE", needle);
    var_set_num(vm, "NTHFLAT_FROM", have_from ? 1 : 0);
    var_set_str(vm, "NTHFLAT_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# nthflat hit=%ld i=%ld path=%s needle=%s from=%d\n",
              hr.n, idx, hr.err[0] ? hr.err : "(miss)",
              needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* GETFLATN|FIRSTFLATN [FROM plate] needle [OR|DEFAULT fallback]
   * First pure-int leaf whose path contains needle → LAST_N=value.
   * Non-int matching leaves skipped. Soft miss → OR numeric fallback or 0.
   * GETFLATN_PATH = winning path · GETFLATN_HIT 0|1. Read-only.
   * Usability: nest numeric peel without GETFLAT+NUM glue:
   *   GETFLATN "port" OR 8080
   *   GETFLATN FROM PEER "hits" OR 0
   */
  if (kw(&L->cur,"GETFLATN") || kw(&L->cur,"FIRSTFLATN") || kw(&L->cur,"NUMFLAT") ||
      kw(&L->cur,"MGETFLATN") || kw(&L->cur,"PLATE_GETFLATN") || kw(&L->cur,"LEAFGETN") ||
      kw(&L->cur,"GETNFLAT") || kw(&L->cur,"NGETFLAT")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, have_fb = 0;
    long fb_n = 0;
    Var *pv;

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
               !kw(&L->cur,"FROM") && !kw(&L->cur,"OR") && !kw(&L->cur,"DEFAULT") &&
               !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
               !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") && !kw(&L->cur,"PASS") &&
               !kw(&L->cur,"GETFLATN") && !kw(&L->cur,"GETFLAT") && !kw(&L->cur,"HASFLAT")) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
    }

    if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
        kw(&L->cur,"FALLBACK")) {
      lex_next(L);
      have_fb = 1;
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        fb_n = parse_expr(vm, L);
      } else if (L->cur.kind == TK_IDENT) {
        pv = var_get(vm, L->cur.text, 0);
        if (pv && !pv->is_str) {
          fb_n = pv->val;
          lex_next(L);
        } else {
          char fbs[64];
          if (resolve_str_arg(vm, L, fbs, sizeof fbs) == 0) {
            char *end = NULL;
            long d = strtol(fbs, &end, 10);
            if (end && end != fbs && *end == 0)
              fb_n = d;
          }
        }
      } else {
        char fbs[64];
        if (resolve_str_arg(vm, L, fbs, sizeof fbs) == 0) {
          char *end = NULL;
          long d = strtol(fbs, &end, 10);
          if (end && end != fbs && *end == 0)
            fb_n = d;
        }
      }
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

    if (!have_fb && (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
                     kw(&L->cur,"FALLBACK"))) {
      lex_next(L);
      have_fb = 1;
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN)
        fb_n = parse_expr(vm, L);
      else if (L->cur.kind == TK_IDENT) {
        pv = var_get(vm, L->cur.text, 0);
        if (pv && !pv->is_str) {
          fb_n = pv->val;
          lex_next(L);
        }
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
    if (cubalc_host_json_leaf_getn(plate, needle, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "GETFLATN: fail");
      var_set_str(vm, "ERR", "GETFLATN: fail");
      var_set_num(vm, "GETFLATN_HIT", 0);
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    if (hr.code == 0 && have_fb) {
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = fb_n;
      var_set_num(vm, "LAST_N", fb_n);
      var_set_num(vm, "GETFLATN", fb_n);
      var_set_str(vm, "GETFLATN_PATH", "");
      var_set_num(vm, "GETFLATN_HIT", 0);
      var_set_num(vm, "GETFLATN_FALLBACK", 1);
    } else if (hr.code == 0) {
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "GETFLATN", 0);
      var_set_str(vm, "GETFLATN_PATH", "");
      var_set_num(vm, "GETFLATN_HIT", 0);
      var_set_num(vm, "GETFLATN_FALLBACK", 0);
    } else {
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "GETFLATN", hr.n);
      var_set_str(vm, "GETFLATN_STR", hr.str);
      var_set_str(vm, "GETFLATN_PATH", hr.err);
      var_set_num(vm, "GETFLATN_HIT", 1);
      var_set_num(vm, "GETFLATN_FALLBACK", 0);
    }
    var_set_str(vm, "GETFLATN_NEEDLE", needle);
    var_set_num(vm, "GETFLATN_FROM", have_from ? 1 : 0);
    var_set_str(vm, "GETFLATN_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# getflatn hit=%d v=%ld path=%s needle=%s from=%d\n",
              hr.code, hr.n, hr.err[0] ? hr.err : "(miss)",
              needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* LASTFLATN|ENDFLATN [FROM plate] needle [OR|DEFAULT fallback]
   * Last pure-int leaf whose path contains needle → LAST_N=value.
   * Non-int matching leaves skipped. Soft miss → OR numeric fallback or 0.
   * LASTFLATN_PATH = winning path · LASTFLATN_HIT 0|1. Read-only.
   * Usability: newest-wins nest numeric peel without REVL+GETFLATN glue:
   *   LASTFLATN "port" OR 8080
   *   LASTFLATN FROM PEER "hits" OR 0
   */
  if (kw(&L->cur,"LASTFLATN") || kw(&L->cur,"ENDFLATN") || kw(&L->cur,"TAILFLATN") ||
      kw(&L->cur,"MLASTFLATN") || kw(&L->cur,"PLATE_LASTFLATN") || kw(&L->cur,"LEAFGETNL") ||
      kw(&L->cur,"GETNFLATL") || kw(&L->cur,"LASTNUMFLAT")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, have_fb = 0;
    long fb_n = 0;
    Var *pv;

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
               !kw(&L->cur,"FROM") && !kw(&L->cur,"OR") && !kw(&L->cur,"DEFAULT") &&
               !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
               !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") && !kw(&L->cur,"PASS") &&
               !kw(&L->cur,"LASTFLATN") && !kw(&L->cur,"GETFLATN") && !kw(&L->cur,"HASFLAT")) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
    }

    if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
        kw(&L->cur,"FALLBACK")) {
      lex_next(L);
      have_fb = 1;
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        fb_n = parse_expr(vm, L);
      } else if (L->cur.kind == TK_IDENT) {
        pv = var_get(vm, L->cur.text, 0);
        if (pv && !pv->is_str) {
          fb_n = pv->val;
          lex_next(L);
        } else {
          char fbs[64];
          if (resolve_str_arg(vm, L, fbs, sizeof fbs) == 0) {
            char *end = NULL;
            long d = strtol(fbs, &end, 10);
            if (end && end != fbs && *end == 0)
              fb_n = d;
          }
        }
      } else {
        char fbs[64];
        if (resolve_str_arg(vm, L, fbs, sizeof fbs) == 0) {
          char *end = NULL;
          long d = strtol(fbs, &end, 10);
          if (end && end != fbs && *end == 0)
            fb_n = d;
        }
      }
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

    if (!have_fb && (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
                     kw(&L->cur,"FALLBACK"))) {
      lex_next(L);
      have_fb = 1;
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN)
        fb_n = parse_expr(vm, L);
      else if (L->cur.kind == TK_IDENT) {
        pv = var_get(vm, L->cur.text, 0);
        if (pv && !pv->is_str) {
          fb_n = pv->val;
          lex_next(L);
        }
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
    if (cubalc_host_json_leaf_getn_last(plate, needle, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "LASTFLATN: fail");
      var_set_str(vm, "ERR", "LASTFLATN: fail");
      var_set_num(vm, "LASTFLATN_HIT", 0);
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    if (hr.code == 0 && have_fb) {
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = fb_n;
      var_set_num(vm, "LAST_N", fb_n);
      var_set_num(vm, "LASTFLATN", fb_n);
      var_set_str(vm, "LASTFLATN_PATH", "");
      var_set_num(vm, "LASTFLATN_HIT", 0);
      var_set_num(vm, "LASTFLATN_FALLBACK", 1);
    } else if (hr.code == 0) {
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "LASTFLATN", 0);
      var_set_str(vm, "LASTFLATN_PATH", "");
      var_set_num(vm, "LASTFLATN_HIT", 0);
      var_set_num(vm, "LASTFLATN_FALLBACK", 0);
    } else {
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "LASTFLATN", hr.n);
      var_set_str(vm, "LASTFLATN_STR", hr.str);
      var_set_str(vm, "LASTFLATN_PATH", hr.err);
      var_set_num(vm, "LASTFLATN_HIT", 1);
      var_set_num(vm, "LASTFLATN_FALLBACK", 0);
    }
    var_set_str(vm, "LASTFLATN_NEEDLE", needle);
    var_set_num(vm, "LASTFLATN_FROM", have_from ? 1 : 0);
    var_set_str(vm, "LASTFLATN_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# lastflatn hit=%d v=%ld path=%s needle=%s from=%d\n",
              hr.code, hr.n, hr.err[0] ? hr.err : "(miss)",
              needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* NTHFLATN|INDEXFLATN [FROM plate] needle index [OR|DEFAULT fallback]
   * 0-based Nth pure-int leaf whose path contains needle → LAST_N=value.
   * Non-int matching leaves skipped. Soft miss → OR numeric fallback or 0.
   * NTHFLATN_PATH = path · NTHFLATN_HIT 0|1 · NTHFLATN_I = index. Read-only.
   * Usability: index pure-int nest peels after COUNTFLATN without VALSFLATN+NTH:
   *   NTHFLATN "score" 0
   *   NTHFLATN "hits" 1 OR 0
   *   NTHFLATN FROM PEER "score" 2
   */
  if (kw(&L->cur,"NTHFLATN") || kw(&L->cur,"INDEXFLATN") || kw(&L->cur,"ATFLATN") ||
      kw(&L->cur,"MNTHFLATN") || kw(&L->cur,"PLATE_NTHFLATN") || kw(&L->cur,"LEAFNTHN") ||
      kw(&L->cur,"NTHNUMFLAT") || kw(&L->cur,"GETNFLATN")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, have_fb = 0, have_idx = 0;
    long fb_n = 0, idx = 0;
    Var *pv;

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

    /* needle */
    if (L->cur.kind == TK_STR) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
    } else if (L->cur.kind == TK_IDENT &&
               !kw(&L->cur,"FROM") && !kw(&L->cur,"OR") && !kw(&L->cur,"DEFAULT") &&
               !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
               !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") && !kw(&L->cur,"PASS") &&
               !kw(&L->cur,"NTHFLATN") && !kw(&L->cur,"GETFLATN") && !kw(&L->cur,"IF")) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
    }

    /* index — literal only (do not parse_expr: would swallow OR fallback as bitwise/logical OR) */
    if (L->cur.kind == TK_NUM) {
      idx = (long)L->cur.num;
      have_idx = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_MINUS) {
      lex_next(L);
      if (L->cur.kind == TK_NUM) {
        idx = -(long)L->cur.num;
        have_idx = 1;
        lex_next(L);
      }
    } else if (L->cur.kind == TK_LPAREN) {
      idx = parse_expr(vm, L);
      have_idx = 1;
    } else if (L->cur.kind == TK_IDENT) {
      pv = var_get(vm, L->cur.text, 0);
      if (pv && !pv->is_str) {
        idx = pv->val;
        have_idx = 1;
        lex_next(L);
      }
    }

    if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
        kw(&L->cur,"FALLBACK")) {
      lex_next(L);
      have_fb = 1;
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN)
        fb_n = parse_expr(vm, L);
      else if (L->cur.kind == TK_IDENT) {
        pv = var_get(vm, L->cur.text, 0);
        if (pv && !pv->is_str) {
          fb_n = pv->val;
          lex_next(L);
        }
      }
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

    if (!have_fb && (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
                     kw(&L->cur,"FALLBACK"))) {
      lex_next(L);
      have_fb = 1;
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN)
        fb_n = parse_expr(vm, L);
      else if (L->cur.kind == TK_IDENT) {
        pv = var_get(vm, L->cur.text, 0);
        if (pv && !pv->is_str) {
          fb_n = pv->val;
          lex_next(L);
        }
      }
    }

    if (!have_idx) {
      fail(vm, "NTHFLATN [FROM plate] needle index [OR n] — need 0-based index");
      return 1;
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
    if (cubalc_host_json_leaf_getn_nth(plate, needle, idx, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "NTHFLATN: fail");
      var_set_str(vm, "ERR", "NTHFLATN: fail");
      var_set_num(vm, "NTHFLATN_HIT", 0);
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    if (hr.code == 0 && have_fb) {
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = fb_n;
      var_set_num(vm, "LAST_N", fb_n);
      var_set_num(vm, "NTHFLATN", fb_n);
      var_set_str(vm, "NTHFLATN_PATH", "");
      var_set_num(vm, "NTHFLATN_HIT", 0);
      var_set_num(vm, "NTHFLATN_FALLBACK", 1);
    } else if (hr.code == 0) {
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "NTHFLATN", 0);
      var_set_str(vm, "NTHFLATN_PATH", "");
      var_set_num(vm, "NTHFLATN_HIT", 0);
      var_set_num(vm, "NTHFLATN_FALLBACK", 0);
    } else {
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "NTHFLATN", hr.n);
      var_set_str(vm, "NTHFLATN_STR", hr.str);
      var_set_str(vm, "NTHFLATN_PATH", hr.err);
      var_set_num(vm, "NTHFLATN_HIT", 1);
      var_set_num(vm, "NTHFLATN_FALLBACK", 0);
    }
    var_set_num(vm, "NTHFLATN_I", idx);
    var_set_str(vm, "NTHFLATN_NEEDLE", needle);
    var_set_num(vm, "NTHFLATN_FROM", have_from ? 1 : 0);
    var_set_str(vm, "NTHFLATN_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# nthflatn hit=%d v=%ld i=%ld path=%s needle=%s from=%d\n",
              hr.code, hr.n, idx, hr.err[0] ? hr.err : "(miss)",
              needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  return 0;
}
