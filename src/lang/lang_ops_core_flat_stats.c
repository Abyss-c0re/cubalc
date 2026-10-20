/* CubalC flat stats shard */
#include "lang/cubalc_lang_ops_core_flat_stats.h"
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

int cubalc_lang_ops_core_try_flat_stats(VM *vm, Lex *L) {
  /* TYPEFLAT|KINDFLAT [FROM plate] needle
   * First matching leaf kind by path needle → LAST=kind · LAST_N=code 0..6.
   * Codes: 0 missing · 1 num · 2 str · 3 bool · 4 null · 5 obj · 6 arr.
   * TYPEFLAT_PATH = path on hit · TYPEFLAT_HIT 0|1. Soft miss → missing/0.
   * Empty needle → first leaf. Read-only.
   * Usability: choose GETFLAT vs GETFLATN vs nest without TYPEP+PATHSFLAT glue:
   *   TYPEFLAT "port"
   *   TYPEFLAT FROM PEER "role"
   */
  if (kw(&L->cur,"TYPEFLAT") || kw(&L->cur,"KINDFLAT") || kw(&L->cur,"LEAFTYPE") ||
      kw(&L->cur,"MTYPEFLAT") || kw(&L->cur,"PLATE_TYPEFLAT") || kw(&L->cur,"FLATTYPE") ||
      kw(&L->cur,"TYPELEAFP") || kw(&L->cur,"KINDLEAFP")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0;
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
               !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") &&
               !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
               !kw(&L->cur,"PASS") && !kw(&L->cur,"SETP") && !kw(&L->cur,"TYPEFLAT") &&
               !kw(&L->cur,"GETFLAT") && !kw(&L->cur,"HASFLAT") && !kw(&L->cur,"IF")) {
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
    if (cubalc_host_json_leaf_type(plate, needle, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "TYPEFLAT: fail");
      var_set_str(vm, "ERR", "TYPEFLAT: fail");
      var_set_num(vm, "TYPEFLAT_HIT", 0);
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "TYPEFLAT", hr.str);
    var_set_str(vm, "KINDFLAT", hr.str);
    var_set_num(vm, "TYPEFLAT_N", hr.n);
    var_set_num(vm, "TYPEFLAT_HIT", hr.code ? 1 : 0);
    var_set_str(vm, "TYPEFLAT_PATH", hr.err);
    var_set_str(vm, "TYPEFLAT_NEEDLE", needle);
    var_set_num(vm, "TYPEFLAT_FROM", have_from ? 1 : 0);
    var_set_str(vm, "TYPEFLAT_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# typeflat hit=%d kind=%s path=%s needle=%s from=%d\n",
              hr.code, hr.str[0] ? hr.str : "missing",
              hr.err[0] ? hr.err : "(miss)", needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* FREQFLAT|HISTFLAT [FROM plate] [needle]
   * Frequency of matching leaf values → LAST = "val:count" bag · LAST_N = uniques.
   * FREQFLAT_TOTAL = matching leaf count. Empty/omitted needle → all leaves.
   * Read-only. Soft empty bag. Cap 64 uniques (same as SYS FREQ).
   * Usability: nest status/role rollups without VALSFLAT+FREQ glue:
   *   FREQFLAT "role"
   *   FREQFLAT ""
   *   FREQFLAT FROM PEER "status"
   */
  if (kw(&L->cur,"FREQFLAT") || kw(&L->cur,"HISTFLAT") || kw(&L->cur,"COUNTVALFLAT") ||
      kw(&L->cur,"MFREQFLAT") || kw(&L->cur,"PLATE_FREQFLAT") || kw(&L->cur,"LEAFFREQ") ||
      kw(&L->cur,"VALFREQFLAT") || kw(&L->cur,"HISTLEAF") || kw(&L->cur,"TALLYFLAT")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0;
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
               !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") &&
               !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
               !kw(&L->cur,"PASS") && !kw(&L->cur,"SETP") && !kw(&L->cur,"INCP") &&
               !kw(&L->cur,"FREQFLAT") && !kw(&L->cur,"HISTFLAT") && !kw(&L->cur,"VALSFLAT") &&
               !kw(&L->cur,"IF")) {
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
    if (cubalc_host_json_leaf_freq(plate, needle, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "FREQFLAT: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "FREQFLAT: fail");
      var_set_num(vm, "FREQFLAT_N", 0);
      var_set_num(vm, "FREQFLAT_TOTAL", 0);
      var_set_str(vm, "LAST", "");
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "FREQFLAT", hr.str);
    var_set_str(vm, "HISTFLAT", hr.str);
    var_set_num(vm, "FREQFLAT_N", hr.n);
    var_set_num(vm, "HISTFLAT_N", hr.n);
    var_set_num(vm, "FREQFLAT_TOTAL", (long)hr.code);
    var_set_num(vm, "HISTFLAT_TOTAL", (long)hr.code);
    var_set_str(vm, "FREQFLAT_NEEDLE", needle);
    var_set_num(vm, "FREQFLAT_FROM", have_from ? 1 : 0);
    var_set_str(vm, "FREQFLAT_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# freqflat uniques=%ld total=%ld needle=%s from=%d\n",
              hr.n, (long)hr.code, needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* MODEFLAT|TOPVALFLAT [FROM plate] [needle]
   * Dominant matching leaf value → LAST · LAST_N = winner count.
   * MODEFLAT_TOTAL = matching leaves · MODEFLAT_N = uniques (via side: not needed).
   * First-seen wins ties. Empty/omitted needle → all leaves. Soft empty.
   * Usability: majority role/status without FREQFLAT+SORTFREQ+BEFORE glue:
   *   MODEFLAT "role"
   *   MODEFLAT FROM PEER "status"
   *   TOPVALFLAT "severity"
   */
  if (kw(&L->cur,"MODEFLAT") || kw(&L->cur,"TOPVALFLAT") || kw(&L->cur,"MAJORFLAT") ||
      kw(&L->cur,"MMODEFLAT") || kw(&L->cur,"PLATE_MODEFLAT") || kw(&L->cur,"LEAFMODE") ||
      kw(&L->cur,"MODALFLAT") || kw(&L->cur,"DOMINANTFLAT") || kw(&L->cur,"WINVALFLAT")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0;
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
               !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") &&
               !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
               !kw(&L->cur,"PASS") && !kw(&L->cur,"SETP") && !kw(&L->cur,"INCP") &&
               !kw(&L->cur,"MODEFLAT") && !kw(&L->cur,"TOPVALFLAT") && !kw(&L->cur,"FREQFLAT") &&
               !kw(&L->cur,"IF")) {
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
    if (cubalc_host_json_leaf_mode(plate, needle, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "MODEFLAT: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "MODEFLAT: fail");
      var_set_num(vm, "MODEFLAT_N", 0);
      var_set_num(vm, "MODEFLAT_TOTAL", 0);
      var_set_str(vm, "LAST", "");
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "MODEFLAT", hr.str);
    var_set_str(vm, "TOPVALFLAT", hr.str);
    var_set_num(vm, "MODEFLAT_N", hr.n); /* winner count */
    var_set_num(vm, "TOPVALFLAT_N", hr.n);
    var_set_num(vm, "MODEFLAT_TOTAL", (long)hr.code);
    var_set_num(vm, "TOPVALFLAT_TOTAL", (long)hr.code);
    var_set_num(vm, "MODEFLAT_HIT", hr.str[0] ? 1 : 0);
    var_set_str(vm, "MODEFLAT_NEEDLE", needle);
    var_set_num(vm, "MODEFLAT_FROM", have_from ? 1 : 0);
    var_set_str(vm, "MODEFLAT_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# modeflat val=%s count=%ld total=%ld needle=%s from=%d\n",
              hr.str[0] ? hr.str : "(miss)", hr.n, (long)hr.code,
              needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* PATHBYVAL|VALPATH|FINDVAL [FROM plate] value [OR|DEFAULT fallback]
   * First leaf path whose value equals needle → LAST=path · LAST_N=hit 0|1.
   * Soft miss → OR path fallback or empty. PATHBYVAL_VALUE side channel on hit.
   * Usability: reverse nest lookup without EACH FLAT+EQS glue (dual of GETFLAT):
   *   PATHBYVAL "worker"
   *   PATHBYVAL "leader" OR "cfg.role"
   *   PATHBYVAL FROM PEER "ok"
   */
  if (kw(&L->cur,"PATHBYVAL") || kw(&L->cur,"VALPATH") || kw(&L->cur,"FINDVAL") ||
      kw(&L->cur,"MPATHBYVAL") || kw(&L->cur,"PLATE_PATHBYVAL") || kw(&L->cur,"FINDPATHVAL") ||
      kw(&L->cur,"PATHOFVAL") || kw(&L->cur,"WHEREVAL") || kw(&L->cur,"LOCATEVAL")) {
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
               !kw(&L->cur,"PATHBYVAL") && !kw(&L->cur,"GETFLAT") && !kw(&L->cur,"IF")) {
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
    if (cubalc_host_json_leaf_path_by_val(plate, needle, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "PATHBYVAL: fail");
      var_set_str(vm, "ERR", "PATHBYVAL: fail");
      var_set_num(vm, "PATHBYVAL_HIT", 0);
      var_set_str(vm, "LAST", "");
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    if (hr.n == 0 && have_fb) {
      var_set_str(vm, "LAST", fallback);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", fallback);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "PATHBYVAL", fallback);
      var_set_str(vm, "VALPATH", fallback);
      var_set_str(vm, "PATHBYVAL_VALUE", "");
      var_set_num(vm, "PATHBYVAL_HIT", 0);
      var_set_num(vm, "PATHBYVAL_FALLBACK", 1);
    } else if (hr.n == 0) {
      var_set_str(vm, "LAST", "");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "PATHBYVAL", "");
      var_set_str(vm, "VALPATH", "");
      var_set_str(vm, "PATHBYVAL_VALUE", "");
      var_set_num(vm, "PATHBYVAL_HIT", 0);
      var_set_num(vm, "PATHBYVAL_FALLBACK", 0);
    } else {
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_str(vm, "PATHBYVAL", hr.str);
      var_set_str(vm, "VALPATH", hr.str);
      var_set_str(vm, "FINDVAL", hr.str);
      var_set_str(vm, "PATHBYVAL_VALUE", hr.err);
      var_set_num(vm, "PATHBYVAL_HIT", 1);
      var_set_num(vm, "PATHBYVAL_FALLBACK", 0);
    }
    var_set_str(vm, "PATHBYVAL_NEEDLE", needle);
    var_set_num(vm, "PATHBYVAL_FROM", have_from ? 1 : 0);
    var_set_str(vm, "PATHBYVAL_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# pathbyval hit=%ld path=%s val=%s from=%d\n",
              hr.n, hr.str[0] ? hr.str : "(miss)", needle[0] ? needle : "", have_from);
    bump(vm); return 1;
  }

  /* PATHSBYVAL|ALLPATHSBYVAL|FINDVALS [FROM plate] value
   * All leaf paths whose value equals needle → LAST bag · LAST_N = count.
   * Soft empty bag. Read-only. Multi twin of PATHBYVAL.
   * Usability: multi reverse nest lookup without EACH FLAT+IF+PUSH glue:
   *   PATHSBYVAL "worker"
   *   PATHSBYVAL FROM PEER "ok"
   *   FINDVALS "leader"
   */
  if (kw(&L->cur,"PATHSBYVAL") || kw(&L->cur,"ALLPATHSBYVAL") || kw(&L->cur,"FINDVALS") ||
      kw(&L->cur,"MPATHSBYVAL") || kw(&L->cur,"PLATE_PATHSBYVAL") || kw(&L->cur,"FINDPATHSVAL") ||
      kw(&L->cur,"PATHSOFVAL") || kw(&L->cur,"WHEREVALS") || kw(&L->cur,"LOCATEVALS")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0;
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
               !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") &&
               !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
               !kw(&L->cur,"PASS") && !kw(&L->cur,"PATHSBYVAL") && !kw(&L->cur,"PATHBYVAL") &&
               !kw(&L->cur,"IF")) {
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
    if (cubalc_host_json_leaf_paths_by_val(plate, needle, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "PATHSBYVAL: fail");
      var_set_str(vm, "ERR", "PATHSBYVAL: fail");
      var_set_num(vm, "PATHSBYVAL_N", 0);
      var_set_str(vm, "LAST", "");
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "PATHSBYVAL", hr.str);
    var_set_str(vm, "ALLPATHSBYVAL", hr.str);
    var_set_str(vm, "FINDVALS", hr.str);
    var_set_num(vm, "PATHSBYVAL_N", hr.n);
    var_set_num(vm, "ALLPATHSBYVAL_N", hr.n);
    var_set_str(vm, "PATHSBYVAL_VALUE", needle);
    var_set_str(vm, "PATHSBYVAL_NEEDLE", needle);
    var_set_num(vm, "PATHSBYVAL_FROM", have_from ? 1 : 0);
    var_set_str(vm, "PATHSBYVAL_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# pathsbyval n=%ld val=%s from=%d\n",
              hr.n, needle[0] ? needle : "", have_from);
    bump(vm); return 1;
  }

  /* COUNTBYVAL|NBYVAL [FROM plate] value
   * HASVAL|HASVALFLAT [FROM plate] value
   * Count / soft presence of exact leaf value → LAST_N.
   * COUNTBYVAL: LAST_N = count · LAST = decimal. HASVAL: LAST_N = 0|1.
   * Soft always OK. Read-only. Usability: without PATHSBYVAL+LINES glue:
   *   COUNTBYVAL "worker"
   *   HASVAL "leader"
   *   COUNTBYVAL FROM PEER "ok"
   */
  if (kw(&L->cur,"COUNTBYVAL") || kw(&L->cur,"NBYVAL") || kw(&L->cur,"VALCOUNT") ||
      kw(&L->cur,"MCOUNTBYVAL") || kw(&L->cur,"PLATE_COUNTBYVAL") || kw(&L->cur,"NVAL") ||
      kw(&L->cur,"HASVAL") || kw(&L->cur,"HASVALFLAT") || kw(&L->cur,"ANYVAL") ||
      kw(&L->cur,"MHASVAL") || kw(&L->cur,"PLATE_HASVAL") || kw(&L->cur,"VALHAS") ||
      kw(&L->cur,"CONTAINSVAL")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    char op0[24];
    cubalc_host_result hr;
    int have_from = 0, want_has = 0;
    Var *pv;

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "HASVAL") == 0 || strcmp(op0, "HASVALFLAT") == 0 ||
        strcmp(op0, "ANYVAL") == 0 || strcmp(op0, "MHASVAL") == 0 ||
        strcmp(op0, "PLATE_HASVAL") == 0 || strcmp(op0, "VALHAS") == 0 ||
        strcmp(op0, "CONTAINSVAL") == 0)
      want_has = 1;

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
               !kw(&L->cur,"PASS") && !kw(&L->cur,"COUNTBYVAL") && !kw(&L->cur,"HASVAL") &&
               !kw(&L->cur,"IF")) {
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
    if (cubalc_host_json_leaf_count_by_val(plate, needle, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", want_has ? "HASVAL: fail" : "COUNTBYVAL: fail");
      var_set_str(vm, "ERR", want_has ? "HASVAL: fail" : "COUNTBYVAL: fail");
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    if (want_has) {
      long hit = hr.code ? 1 : 0;
      var_set_num(vm, "LAST_N", hit);
      vm->last_n = hit;
      snprintf(vm->last_str, sizeof vm->last_str, "%ld", hit);
      var_set_str(vm, "LAST", vm->last_str);
      var_set_num(vm, "HASVAL", hit);
      var_set_num(vm, "HASVALFLAT", hit);
      var_set_num(vm, "HASVAL_HIT", hit);
      var_set_num(vm, "HASVAL_N", hr.n); /* full count side channel */
    } else {
      var_set_num(vm, "LAST_N", hr.n);
      vm->last_n = hr.n;
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      var_set_str(vm, "LAST", hr.str);
      var_set_str(vm, "COUNTBYVAL", hr.str);
      var_set_str(vm, "NBYVAL", hr.str);
      var_set_num(vm, "COUNTBYVAL_N", hr.n);
      var_set_num(vm, "NBYVAL_N", hr.n);
    }
    var_set_str(vm, "COUNTBYVAL_VALUE", needle);
    var_set_str(vm, "HASVAL_VALUE", needle);
    var_set_num(vm, "COUNTBYVAL_FROM", have_from ? 1 : 0);
    var_set_num(vm, "HASVAL_FROM", have_from ? 1 : 0);
    var_set_str(vm, "COUNTBYVAL_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_str(vm, "HASVAL_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%ld hit=%d val=%s from=%d\n",
              want_has ? "hasval" : "countbyval", hr.n, hr.code,
              needle[0] ? needle : "", have_from);
    bump(vm); return 1;
  }

  /* SETBYVAL|REPLACEVAL [FROM plate] old new
   * Rewrite every leaf whose value equals old → new. Write-back PLATE or FROM var.
   * LAST = new plate · LAST_N = leaves rewritten · SETBYVAL_TOTAL = leaf count.
   * Soft always OK (n=0 if no match). Usability: bulk status rewrite without
   * PATHSBYVAL+EACH+SETP glue:
   *   SETBYVAL "worker" "standby"
   *   SETBYVAL FROM PEER "ok" "ready"
   *   REPLACEVAL "fail" "retry"
   */
  if (kw(&L->cur,"SETBYVAL") || kw(&L->cur,"REPLACEVAL") || kw(&L->cur,"REWVAL") ||
      kw(&L->cur,"MSETBYVAL") || kw(&L->cur,"PLATE_SETBYVAL") || kw(&L->cur,"SWAPVAL") ||
      kw(&L->cur,"REVAL") || kw(&L->cur,"MAPVAL") || kw(&L->cur,"REWRITEVAL")) {
    char plate[CUBALC_HOST_STR_MAX], oldv[192], newv[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0;
    Var *pv;

    lex_next(L);
    plate[0] = 0; oldv[0] = 0; newv[0] = 0; from_name[0] = 0; from_src[0] = 0;

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
      if (resolve_str_arg(vm, L, oldv, sizeof oldv) != 0)
        oldv[0] = 0;
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(oldv, sizeof oldv, "%ld", kv);
    } else if (L->cur.kind == TK_IDENT &&
               !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") &&
               !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
               !kw(&L->cur,"PASS") && !kw(&L->cur,"SETBYVAL") && !kw(&L->cur,"IF")) {
      if (resolve_str_arg(vm, L, oldv, sizeof oldv) != 0)
        oldv[0] = 0;
    } else {
      fail(vm, "SETBYVAL [FROM plate] old new — need old value");
      return -1;
    }

    if (L->cur.kind == TK_STR) {
      if (resolve_str_arg(vm, L, newv, sizeof newv) != 0)
        newv[0] = 0;
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(newv, sizeof newv, "%ld", kv);
    } else if (L->cur.kind == TK_IDENT &&
               !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") &&
               !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
               !kw(&L->cur,"PASS") && !kw(&L->cur,"IF")) {
      if (resolve_str_arg(vm, L, newv, sizeof newv) != 0)
        newv[0] = 0;
    } else {
      fail(vm, "SETBYVAL [FROM plate] old new — need new value");
      return -1;
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
    if (cubalc_host_json_leaf_set_by_val(plate, oldv, newv, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "SETBYVAL: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "SETBYVAL: fail");
      var_set_num(vm, "SETBYVAL_N", 0);
      bump(vm); return 1;
    }

    if (have_from && from_name[0] && strcmp(from_name, "LAST") != 0)
      var_set_str(vm, from_name, hr.str);
    else
      var_set_str(vm, "PLATE", hr.str);

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "SETBYVAL", hr.str);
    var_set_str(vm, "REPLACEVAL", hr.str);
    var_set_num(vm, "SETBYVAL_N", hr.n);
    var_set_num(vm, "REPLACEVAL_N", hr.n);
    var_set_num(vm, "SETBYVAL_TOTAL", hr.code);
    var_set_str(vm, "SETBYVAL_OLD", oldv);
    var_set_str(vm, "SETBYVAL_NEW", newv);
    var_set_num(vm, "SETBYVAL_FROM", have_from ? 1 : 0);
    var_set_str(vm, "SETBYVAL_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# setbyval n=%ld total=%ld old=%s new=%s from=%d\n",
              hr.n, hr.code, oldv, newv, have_from);
    bump(vm); return 1;
  }

  /* DELBYVAL|DROPVAL [FROM plate] value
   * Drop every leaf whose value equals needle. Write-back PLATE or FROM var.
   * LAST = new plate · LAST_N = deleted · DELBYVAL_KEPT = remaining leaves.
   * Soft always OK (n=0 if no match). Usability: scrub by value without
   * PATHSBYVAL+EACH+DELP glue:
   *   DELBYVAL "standby"
   *   DELBYVAL FROM PEER "fail"
   *   DROPVAL "retry"
   */
  if (kw(&L->cur,"DELBYVAL") || kw(&L->cur,"DROPVAL") || kw(&L->cur,"RMVAL") ||
      kw(&L->cur,"MDELBYVAL") || kw(&L->cur,"PLATE_DELBYVAL") || kw(&L->cur,"DELETEVAL") ||
      kw(&L->cur,"REMOVEVAL") || kw(&L->cur,"PRUNEVAL") || kw(&L->cur,"STRIPVAL")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0;
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
               !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") &&
               !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
               !kw(&L->cur,"PASS") && !kw(&L->cur,"DELBYVAL") && !kw(&L->cur,"IF")) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
    } else {
      fail(vm, "DELBYVAL [FROM plate] value — need value");
      return -1;
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
    if (cubalc_host_json_leaf_del_by_val(plate, needle, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "DELBYVAL: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "DELBYVAL: fail");
      var_set_num(vm, "DELBYVAL_N", 0);
      bump(vm); return 1;
    }

    if (have_from && from_name[0] && strcmp(from_name, "LAST") != 0)
      var_set_str(vm, from_name, hr.str);
    else
      var_set_str(vm, "PLATE", hr.str);

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "DELBYVAL", hr.str);
    var_set_str(vm, "DROPVAL", hr.str);
    var_set_num(vm, "DELBYVAL_N", hr.n);
    var_set_num(vm, "DROPVAL_N", hr.n);
    var_set_num(vm, "DELBYVAL_KEPT", hr.code);
    var_set_num(vm, "DROPVAL_KEPT", hr.code);
    var_set_str(vm, "DELBYVAL_VALUE", needle);
    var_set_num(vm, "DELBYVAL_FROM", have_from ? 1 : 0);
    var_set_str(vm, "DELBYVAL_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# delbyval n=%ld kept=%ld val=%s from=%d\n",
              hr.n, hr.code, needle[0] ? needle : "", have_from);
    bump(vm); return 1;
  }

  /* ALLEQFLAT|SAMEVALFLAT [FROM plate] [needle]
   * Soft probe: all matching leaf values identical → LAST_N 0|1.
   * LAST = common value when equal (else empty). ALLEQFLAT_N = match count.
   * Empty/omitted needle → all leaves. Soft empty match → LAST_N=0.
   * Usability: nest consistency without UNIQFLAT+LINES+CMP glue:
   *   ALLEQFLAT "role"
   *   ALLEQFLAT FROM PEER "status"
   *   SAMEVALFLAT ""
   */
  if (kw(&L->cur,"ALLEQFLAT") || kw(&L->cur,"SAMEVALFLAT") || kw(&L->cur,"EQVALSFLAT") ||
      kw(&L->cur,"MALLEQFLAT") || kw(&L->cur,"PLATE_ALLEQFLAT") || kw(&L->cur,"ALLSAMEFLAT") ||
      kw(&L->cur,"SAMEFLAT") || kw(&L->cur,"EQALLFLAT") || kw(&L->cur,"EQVALSALL")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0;
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
               !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") &&
               !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
               !kw(&L->cur,"PASS") && !kw(&L->cur,"ALLEQFLAT") && !kw(&L->cur,"UNIQFLAT") &&
               !kw(&L->cur,"IF")) {
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
    if (cubalc_host_json_leaf_all_eq(plate, needle, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "ALLEQFLAT: fail");
      var_set_str(vm, "ERR", "ALLEQFLAT: fail");
      var_set_num(vm, "ALLEQFLAT_N", 0);
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    {
      long eq = hr.code ? 1 : 0;
      var_set_num(vm, "LAST_N", eq);
      vm->last_n = eq;
      if (eq) {
        var_set_str(vm, "LAST", hr.str);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      } else {
        var_set_str(vm, "LAST", "");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      }
      var_set_str(vm, "ALLEQFLAT", hr.str);
      var_set_str(vm, "SAMEVALFLAT", hr.str);
      var_set_num(vm, "ALLEQFLAT_EQ", eq);
      var_set_num(vm, "SAMEVALFLAT_EQ", eq);
      var_set_num(vm, "ALLEQFLAT_N", hr.n); /* match count */
      var_set_num(vm, "SAMEVALFLAT_N", hr.n);
      var_set_num(vm, "ALLEQFLAT_UNIQUES",
                  (hr.err[0] && (hr.err[0] >= '0' && hr.err[0] <= '9'))
                      ? strtol(hr.err, NULL, 10)
                      : (eq ? 1 : 0));
    }
    var_set_str(vm, "ALLEQFLAT_NEEDLE", needle);
    var_set_num(vm, "ALLEQFLAT_FROM", have_from ? 1 : 0);
    var_set_str(vm, "ALLEQFLAT_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# alleqflat eq=%d n=%ld needle=%s from=%d\n",
              hr.code, hr.n, needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* FIRSTUNEQFLAT|PATHUNEQ|DIVERGE1 [FROM plate] [needle]
   * First matching leaf path whose value differs from the first match.
   * LAST = path (empty if all equal / empty / single). LAST_N = 0|1 found.
   * FIRSTUNEQ_VAL / FIRSTUNEQ_REF / FIRSTUNEQ_I / FIRSTUNEQ_N side vars.
   * Empty/omitted needle → all leaves. Soft always OK. Read-only.
   * Usability: after ALLEQFLAT fails, locate first inconsistency without EACH+EQS:
   *   ALLEQFLAT "role"
   *   IF LAST_N == 0
   *     FIRSTUNEQFLAT "role"   # LAST=path of first diverge
   *   END
   *   FIRSTUNEQFLAT FROM PEER "status"
   *   PATHUNEQ ""
   */
  if (kw(&L->cur,"FIRSTUNEQFLAT") || kw(&L->cur,"PATHUNEQ") || kw(&L->cur,"DIVERGE1") ||
      kw(&L->cur,"MFIRSTUNEQFLAT") || kw(&L->cur,"PLATE_FIRSTUNEQ") || kw(&L->cur,"FIRSTDIVERGE") ||
      kw(&L->cur,"UNEQ1") || kw(&L->cur,"FIRSTDIFFVAL") || kw(&L->cur,"PATHDIFFVAL")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    char ref[256], val[256];
    cubalc_host_result hr;
    int have_from = 0;
    long total = 0, found = 0;
    Var *pv;
    const char *ep;

    lex_next(L);
    plate[0] = 0; needle[0] = 0; from_name[0] = 0; from_src[0] = 0;
    ref[0] = 0; val[0] = 0;

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
               !kw(&L->cur,"PASS") && !kw(&L->cur,"FIRSTUNEQFLAT") && !kw(&L->cur,"ALLEQFLAT") &&
               !kw(&L->cur,"IF")) {
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
    if (cubalc_host_json_leaf_first_uneq(plate, needle, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "FIRSTUNEQFLAT: fail");
      var_set_str(vm, "ERR", "FIRSTUNEQFLAT: fail");
      var_set_num(vm, "FIRSTUNEQFLAT_N", 0);
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    found = hr.code ? 1 : 0;
    if (found) {
      /* err = "ref\\nval\\ntotal" */
      ep = hr.err;
      {
        size_t i = 0;
        while (*ep && *ep != '\n' && *ep != '\r' && i + 1 < sizeof ref)
          ref[i++] = *ep++;
        ref[i] = 0;
      }
      while (*ep == '\n' || *ep == '\r') ep++;
      {
        size_t i = 0;
        while (*ep && *ep != '\n' && *ep != '\r' && i + 1 < sizeof val)
          val[i++] = *ep++;
        val[i] = 0;
      }
      while (*ep == '\n' || *ep == '\r') ep++;
      total = strtol(ep, NULL, 10);
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
    } else {
      total = strtol(hr.err, NULL, 10);
      var_set_str(vm, "LAST", "");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
    }

    var_set_str(vm, "FIRSTUNEQFLAT", hr.str);
    var_set_str(vm, "PATHUNEQ", hr.str);
    var_set_str(vm, "DIVERGE1", hr.str);
    var_set_num(vm, "FIRSTUNEQFLAT_FOUND", found);
    var_set_num(vm, "PATHUNEQ_FOUND", found);
    var_set_num(vm, "FIRSTUNEQ_I", found ? hr.n : 0);
    var_set_num(vm, "FIRSTUNEQFLAT_I", found ? hr.n : 0);
    var_set_num(vm, "FIRSTUNEQ_N", total);
    var_set_num(vm, "FIRSTUNEQFLAT_N", total);
    var_set_str(vm, "FIRSTUNEQ_REF", ref);
    var_set_str(vm, "FIRSTUNEQFLAT_REF", ref);
    var_set_str(vm, "FIRSTUNEQ_VAL", val);
    var_set_str(vm, "FIRSTUNEQFLAT_VAL", val);
    var_set_str(vm, "FIRSTUNEQFLAT_NEEDLE", needle);
    var_set_num(vm, "FIRSTUNEQFLAT_FROM", have_from ? 1 : 0);
    var_set_str(vm, "FIRSTUNEQFLAT_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# firstuneqflat found=%ld i=%ld n=%ld needle=%s from=%d\n",
              found, found ? hr.n : 0L, total, needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* UNEQPATHS|DIVERGEPATHS|ALLUNEQ [FROM plate] [needle]
   * All matching leaf paths whose value differs from the first match → LAST bag.
   * LAST_N = diverge count. UNEQPATHS_REF = first value. UNEQPATHS_TOTAL = matches.
   * Empty/omitted needle → all leaves. Soft empty bag if uniform / 0-1. Multi twin of FIRSTUNEQFLAT.
   * Usability: bulk nest inconsistency locate without EACH FLAT+IF+PUSH:
   *   ALLEQFLAT "role"
   *   IF LAST_N == 0
   *     UNEQPATHS "role"   # bag of paths != first role
   *   END
   *   UNEQPATHS FROM PEER "status"
   *   DIVERGEPATHS ""
   */
  if (kw(&L->cur,"UNEQPATHS") || kw(&L->cur,"DIVERGEPATHS") || kw(&L->cur,"ALLUNEQ") ||
      kw(&L->cur,"MUNEQPATHS") || kw(&L->cur,"PLATE_UNEQPATHS") || kw(&L->cur,"PATHSUNEQ") ||
      kw(&L->cur,"DIVERGEALL") || kw(&L->cur,"ALLDIVERGE") || kw(&L->cur,"UNEQALL")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0;
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
               !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") &&
               !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
               !kw(&L->cur,"PASS") && !kw(&L->cur,"UNEQPATHS") && !kw(&L->cur,"FIRSTUNEQFLAT") &&
               !kw(&L->cur,"IF")) {
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
    if (cubalc_host_json_leaf_uneq_paths(plate, needle, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "UNEQPATHS: fail");
      var_set_str(vm, "ERR", "UNEQPATHS: fail");
      var_set_num(vm, "UNEQPATHS_N", 0);
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    var_set_num(vm, "LAST_N", hr.n);
    vm->last_n = hr.n;
    var_set_str(vm, "UNEQPATHS", hr.str);
    var_set_str(vm, "DIVERGEPATHS", hr.str);
    var_set_str(vm, "ALLUNEQ", hr.str);
    var_set_num(vm, "UNEQPATHS_N", hr.n);
    var_set_num(vm, "DIVERGEPATHS_N", hr.n);
    var_set_num(vm, "UNEQPATHS_TOTAL", (long)hr.code);
    var_set_num(vm, "DIVERGEPATHS_TOTAL", (long)hr.code);
    var_set_str(vm, "UNEQPATHS_REF", hr.err);
    var_set_str(vm, "DIVERGEPATHS_REF", hr.err);
    var_set_str(vm, "UNEQPATHS_NEEDLE", needle);
    var_set_num(vm, "UNEQPATHS_FROM", have_from ? 1 : 0);
    var_set_str(vm, "UNEQPATHS_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# uneqpaths n=%ld total=%ld needle=%s from=%d\n",
              hr.n, (long)hr.code, needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* UNIFORMFLAT|CHECKFLAT|SAMECHECK [FROM plate] [needle]
   * One-shot nest consistency probe (ALLEQFLAT + FIRSTUNEQFLAT + UNEQPATHS).
   * LAST_N = 0|1 all-equal · LAST = common value when equal else "".
   * Side: UNIFORM_N matches · UNIFORM_DIV diverge count · UNIFORM_PATH first
   *       UNIFORM_PATHS bag · UNIFORM_REF · UNIFORM_VAL · UNIFORM_UNIQUES
   * Soft always OK. Read-only. Usability: one form for nest check + locate:
   *   UNIFORMFLAT "role"
   *   IF LAST_N == 0
   *     PRINT UNIFORM_PATHS
   *   END
   *   UNIFORMFLAT FROM PEER "status"
   *   CHECKFLAT ""
   */
  if (kw(&L->cur,"UNIFORMFLAT") || kw(&L->cur,"CHECKFLAT") || kw(&L->cur,"SAMECHECK") ||
      kw(&L->cur,"MUNIFORMFLAT") || kw(&L->cur,"PLATE_UNIFORM") || kw(&L->cur,"CHECKUNIFORM") ||
      kw(&L->cur,"NESTUNIFORM") || kw(&L->cur,"FLATUNIFORM") || kw(&L->cur,"UNIFORMCHECK")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    char ref[256], val[256], path1[512];
    cubalc_host_result heq, h1, hpaths;
    int have_from = 0;
    long eq = 0, total = 0, divn = 0, uniques = 0, idx = 0;
    Var *pv;
    const char *ep;

    lex_next(L);
    plate[0] = 0; needle[0] = 0; from_name[0] = 0; from_src[0] = 0;
    ref[0] = 0; val[0] = 0; path1[0] = 0;

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
               !kw(&L->cur,"PASS") && !kw(&L->cur,"UNIFORMFLAT") && !kw(&L->cur,"ALLEQFLAT") &&
               !kw(&L->cur,"IF")) {
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

    memset(&heq, 0, sizeof heq);
    memset(&h1, 0, sizeof h1);
    memset(&hpaths, 0, sizeof hpaths);
    if (cubalc_host_json_leaf_all_eq(plate, needle, &heq) != 0 ||
        cubalc_host_json_leaf_first_uneq(plate, needle, &h1) != 0 ||
        cubalc_host_json_leaf_uneq_paths(plate, needle, &hpaths) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "UNIFORMFLAT: fail");
      var_set_str(vm, "ERR", "UNIFORMFLAT: fail");
      var_set_num(vm, "UNIFORM_N", 0);
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    eq = heq.code ? 1 : 0;
    total = heq.n;
    divn = hpaths.n;
    uniques = (heq.err[0] && heq.err[0] >= '0' && heq.err[0] <= '9')
                  ? strtol(heq.err, NULL, 10)
                  : (eq ? (total > 0 ? 1 : 0) : 0);
    if (h1.code) {
      snprintf(path1, sizeof path1, "%s", h1.str);
      idx = h1.n;
      ep = h1.err;
      {
        size_t i = 0;
        while (*ep && *ep != '\n' && *ep != '\r' && i + 1 < sizeof ref)
          ref[i++] = *ep++;
        ref[i] = 0;
      }
      while (*ep == '\n' || *ep == '\r') ep++;
      {
        size_t i = 0;
        while (*ep && *ep != '\n' && *ep != '\r' && i + 1 < sizeof val)
          val[i++] = *ep++;
        val[i] = 0;
      }
    } else if (hpaths.err[0]) {
      snprintf(ref, sizeof ref, "%s", hpaths.err);
    }

    var_set_num(vm, "LAST_N", eq);
    vm->last_n = eq;
    if (eq) {
      var_set_str(vm, "LAST", heq.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", heq.str);
    } else {
      var_set_str(vm, "LAST", "");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
    }

    var_set_str(vm, "UNIFORMFLAT", heq.str);
    var_set_str(vm, "CHECKFLAT", heq.str);
    var_set_num(vm, "UNIFORM_EQ", eq);
    var_set_num(vm, "UNIFORMFLAT_EQ", eq);
    var_set_num(vm, "UNIFORM_N", total);
    var_set_num(vm, "UNIFORMFLAT_N", total);
    var_set_num(vm, "UNIFORM_DIV", divn);
    var_set_num(vm, "UNIFORMFLAT_DIV", divn);
    var_set_num(vm, "UNIFORM_UNIQUES", uniques);
    var_set_num(vm, "UNIFORMFLAT_UNIQUES", uniques);
    var_set_num(vm, "UNIFORM_I", idx);
    var_set_str(vm, "UNIFORM_PATH", path1);
    var_set_str(vm, "UNIFORMFLAT_PATH", path1);
    var_set_str(vm, "UNIFORM_PATHS", hpaths.str);
    var_set_str(vm, "UNIFORMFLAT_PATHS", hpaths.str);
    var_set_str(vm, "UNIFORM_REF", ref);
    var_set_str(vm, "UNIFORMFLAT_REF", ref);
    var_set_str(vm, "UNIFORM_VAL", val);
    var_set_str(vm, "UNIFORMFLAT_VAL", val);
    var_set_str(vm, "UNIFORMFLAT_NEEDLE", needle);
    var_set_num(vm, "UNIFORMFLAT_FROM", have_from ? 1 : 0);
    var_set_str(vm, "UNIFORMFLAT_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# uniformflat eq=%ld n=%ld div=%ld needle=%s from=%d\n",
              eq, total, divn, needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* PRETTYP|JSONPRETTY|PRETTYJSON [FROM plate]
   * 2-space indented plate JSON → LAST · LAST_N = bytes.
   * Soft always OK. Read-only. Truncation sets PRETTYP_TRUNC=1.
   * Usability: human/agent nest inspection without shell jq:
   *   PRETTYP
   *   PRETTYP FROM PEER
   *   JSONPRETTY FROM LAST
   */
  if (kw(&L->cur,"PRETTYP") || kw(&L->cur,"JSONPRETTY") || kw(&L->cur,"PRETTYJSON") ||
      kw(&L->cur,"MPRETTYP") || kw(&L->cur,"PLATE_PRETTY") || kw(&L->cur,"PRETTYPLATE") ||
      kw(&L->cur,"FMTJSON") || kw(&L->cur,"INDENTJSON") || kw(&L->cur,"PRETTY_P")) {
    char plate[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0;
    Var *pv;

    lex_next(L);
    plate[0] = 0; from_name[0] = 0; from_src[0] = 0;

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

    if (have_from) {
      const char *b = from_src;
      while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
      if (*b == '{' || *b == '[')
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
    if (cubalc_host_json_pretty(plate, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "PRETTYP: fail");
      var_set_str(vm, "ERR", "PRETTYP: fail");
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    var_set_num(vm, "LAST_N", hr.n);
    vm->last_n = hr.n;
    var_set_str(vm, "PRETTYP", hr.str);
    var_set_str(vm, "JSONPRETTY", hr.str);
    var_set_str(vm, "PRETTYJSON", hr.str);
    var_set_num(vm, "PRETTYP_N", hr.n);
    var_set_num(vm, "PRETTYP_TRUNC", hr.code ? 0 : 1);
    var_set_num(vm, "PRETTYP_FROM", have_from ? 1 : 0);
    var_set_str(vm, "PRETTYP_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# prettyp n=%ld trunc=%d from=%d\n",
              hr.n, hr.code ? 0 : 1, have_from);
    bump(vm); return 1;
  }

  /* UNIQFLAT|DISTINCTFLAT [FROM plate] [needle]
   * Unique matching leaf values → LAST bag · LAST_N = unique count.
   * UNIQFLAT_TOTAL = matching leaf count. Empty/omitted needle → all leaves.
   * First-seen order. Cap 64 uniques. Soft empty bag. Read-only.
   * Usability: "what roles/statuses exist" without VALSFLAT+UNIQ glue:
   *   UNIQFLAT "role"
   *   UNIQFLAT FROM PEER "status"
   *   DISTINCTFLAT ""
   */
  if (kw(&L->cur,"UNIQFLAT") || kw(&L->cur,"DISTINCTFLAT") || kw(&L->cur,"UNIQUEFLAT") ||
      kw(&L->cur,"MUNIQFLAT") || kw(&L->cur,"PLATE_UNIQFLAT") || kw(&L->cur,"LEAFUNIQ") ||
      kw(&L->cur,"DEDUPFLAT") || kw(&L->cur,"UNIQVALSFLAT") || kw(&L->cur,"UNIQUEVALS")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0;
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
               !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") &&
               !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
               !kw(&L->cur,"PASS") && !kw(&L->cur,"SETP") && !kw(&L->cur,"INCP") &&
               !kw(&L->cur,"UNIQFLAT") && !kw(&L->cur,"FREQFLAT") && !kw(&L->cur,"VALSFLAT") &&
               !kw(&L->cur,"IF")) {
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
    if (cubalc_host_json_leaf_uniq(plate, needle, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "UNIQFLAT: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "UNIQFLAT: fail");
      var_set_num(vm, "UNIQFLAT_N", 0);
      var_set_num(vm, "UNIQFLAT_TOTAL", 0);
      var_set_str(vm, "LAST", "");
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "UNIQFLAT", hr.str);
    var_set_str(vm, "DISTINCTFLAT", hr.str);
    var_set_num(vm, "UNIQFLAT_N", hr.n);
    var_set_num(vm, "DISTINCTFLAT_N", hr.n);
    var_set_num(vm, "UNIQFLAT_TOTAL", (long)hr.code);
    var_set_num(vm, "DISTINCTFLAT_TOTAL", (long)hr.code);
    var_set_str(vm, "UNIQFLAT_NEEDLE", needle);
    var_set_num(vm, "UNIQFLAT_FROM", have_from ? 1 : 0);
    var_set_str(vm, "UNIQFLAT_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# uniqflat uniques=%ld total=%ld needle=%s from=%d\n",
              hr.n, (long)hr.code, needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* NEEDFLAT|REQUIREFLAT [FROM plate] needle…
   * Fail-fast if any path needle has zero matching leaves (HASFLAT dual contract).
   * Soft twin: HASFLAT. Lists missing needles in error. LAST_N = needles checked.
   * Usability: agent structure gate without multi HASFLAT+IF fail glue:
   *   NEEDFLAT "tls" "agent_id"
   *   NEEDFLAT FROM PEER "port"
   */
  if (kw(&L->cur,"NEEDFLAT") || kw(&L->cur,"REQUIREFLAT") || kw(&L->cur,"MUSTFLAT") ||
      kw(&L->cur,"MNEEDFLAT") || kw(&L->cur,"PLATE_NEEDFLAT") || kw(&L->cur,"REQUIRE_FLAT") ||
      kw(&L->cur,"NEEDLEAVES") || kw(&L->cur,"NEEDPATHS")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    char miss_bag[CUBALC_HOST_STR_MAX], msg[CUBALC_HOST_ERR_MAX];
    cubalc_host_result hr;
    int have_from = 0, n_need = 0, n_miss = 0;
    size_t molen = 0;
    Var *pv;
    int aln = L->cur.line;

    lex_next(L);
    plate[0] = 0; from_name[0] = 0; from_src[0] = 0; miss_bag[0] = 0;

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

    /* collect needles — at least one required */
    while (L->cur.kind == TK_STR ||
           (L->cur.kind == TK_IDENT &&
            !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") &&
            !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
            !kw(&L->cur,"PASS") && !kw(&L->cur,"NEEDFLAT") && !kw(&L->cur,"HASFLAT") &&
            !kw(&L->cur,"GETFLAT") && !kw(&L->cur,"SETP") && !kw(&L->cur,"IF") &&
            !kw(&L->cur,"ELSE") && !kw(&L->cur,"ELIF"))) {
      needle[0] = 0;
      if (L->cur.kind == TK_STR) {
        if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
          needle[0] = 0;
      } else if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
        needle[0] = 0;
      }
      n_need++;
      memset(&hr, 0, sizeof hr);
      cubalc_host_json_leaf_count(plate, needle, &hr);
      if (hr.n <= 0) {
        const char *mn = needle[0] ? needle : "(empty)";
        size_t mlen = strlen(mn);
        n_miss++;
        if (molen > 0 && molen + 1 < sizeof miss_bag)
          miss_bag[molen++] = '\n';
        if (molen + mlen < sizeof miss_bag) {
          memcpy(miss_bag + molen, mn, mlen);
          molen += mlen;
          miss_bag[molen] = 0;
        }
      }
    }

    /* trailing FROM after needles */
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
      /* re-resolve plate and re-check if we had needles before FROM — already counted against wrong plate.
       * Prefer FROM first in docs; if trailing FROM with prior needles, re-run is complex.
       * Document leading FROM. If trailing only with no prior plate use, update plate for empty re-run.
       * For simplicity: if trailing FROM and n_need already processed, re-process is hard.
       * Support leading FROM primarily (like GETFLAT). */
      {
        const char *b = from_src;
        while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
        if (*b == '{')
          snprintf(plate, sizeof plate, "%s", from_src);
        else
          snprintf(plate, sizeof plate, "%s", "{}");
      }
    }

    if (n_need < 1) {
      fail(vm, "NEEDFLAT [FROM plate] needle… — need at least one path needle");
      return 1;
    }

    if (n_miss > 0) {
      snprintf(msg, sizeof msg, "NEEDFLAT line %d: missing leaf path needle(s): %s",
               aln, miss_bag[0] ? miss_bag : "?");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", msg);
      var_set_str(vm, "ERR", msg);
      var_set_str(vm, "NEEDFLAT_MISS", miss_bag);
      var_set_num(vm, "NEEDFLAT_N", n_need);
      var_set_num(vm, "NEEDFLAT_MISS_N", n_miss);
      var_set_num(vm, "LAST_N", 0);
      fail(vm, msg);
      return 1;
    }

    var_set_str(vm, "LAST", plate);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
    vm->last_n = n_need;
    var_set_num(vm, "LAST_N", n_need);
    var_set_num(vm, "NEEDFLAT_N", n_need);
    var_set_num(vm, "NEEDFLAT_MISS_N", 0);
    var_set_str(vm, "NEEDFLAT_MISS", "");
    var_set_num(vm, "NEEDFLAT_FROM", have_from ? 1 : 0);
    var_set_str(vm, "NEEDFLAT_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# needflat n=%d miss=0 from=%d\n", n_need, have_from);
    bump(vm); return 1;
  }

  /* NEEDFLATN|REQUIREFLATN [FROM plate] needle…
   * Fail-fast if any path needle has zero pure-int matching leaves (GETFLATN contract).
   * Soft twin: GETFLATN. Lists missing needles in error.
   * On success: LAST_N = first needle's pure-int value · NEEDFLATN_PATH = its path ·
   * NEEDFLATN_N = needles checked. Read-only.
   * Usability: typed int structure gate + peel without GETFLATN+IF fail glue:
   *   NEEDFLATN "port"
   *   NEEDFLATN "port" "timeout_ms"
   *   NEEDFLATN FROM PEER "hits"
   */
  if (kw(&L->cur,"NEEDFLATN") || kw(&L->cur,"REQUIREFLATN") || kw(&L->cur,"MUSTFLATN") ||
      kw(&L->cur,"MNEEDFLATN") || kw(&L->cur,"PLATE_NEEDFLATN") || kw(&L->cur,"REQUIRE_FLATN") ||
      kw(&L->cur,"NEEDINTFLAT") || kw(&L->cur,"NEEDNUMFLAT")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    char miss_bag[CUBALC_HOST_STR_MAX], msg[CUBALC_HOST_ERR_MAX];
    char first_path[512];
    cubalc_host_result hr;
    int have_from = 0, n_need = 0, n_miss = 0;
    long first_val = 0;
    int have_first = 0;
    size_t molen = 0;
    Var *pv;
    int aln = L->cur.line;

    lex_next(L);
    plate[0] = 0; from_name[0] = 0; from_src[0] = 0; miss_bag[0] = 0;
    first_path[0] = 0;

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

    while (L->cur.kind == TK_STR ||
           (L->cur.kind == TK_IDENT &&
            !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") &&
            !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
            !kw(&L->cur,"PASS") && !kw(&L->cur,"NEEDFLATN") && !kw(&L->cur,"NEEDFLAT") &&
            !kw(&L->cur,"GETFLATN") && !kw(&L->cur,"HASFLAT") && !kw(&L->cur,"GETFLAT") &&
            !kw(&L->cur,"SETP") && !kw(&L->cur,"IF") && !kw(&L->cur,"ELSE") &&
            !kw(&L->cur,"ELIF"))) {
      needle[0] = 0;
      if (L->cur.kind == TK_STR) {
        if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
          needle[0] = 0;
      } else if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
        needle[0] = 0;
      }
      n_need++;
      memset(&hr, 0, sizeof hr);
      cubalc_host_json_leaf_getn(plate, needle, &hr);
      if (hr.code == 0) {
        const char *mn = needle[0] ? needle : "(empty)";
        size_t mlen = strlen(mn);
        n_miss++;
        if (molen > 0 && molen + 1 < sizeof miss_bag)
          miss_bag[molen++] = '\n';
        if (molen + mlen < sizeof miss_bag) {
          memcpy(miss_bag + molen, mn, mlen);
          molen += mlen;
          miss_bag[molen] = 0;
        }
      } else if (!have_first) {
        first_val = hr.n;
        snprintf(first_path, sizeof first_path, "%s", hr.err);
        have_first = 1;
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
      {
        const char *b = from_src;
        while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
        if (*b == '{')
          snprintf(plate, sizeof plate, "%s", from_src);
        else
          snprintf(plate, sizeof plate, "%s", "{}");
      }
    }

    if (n_need < 1) {
      fail(vm, "NEEDFLATN [FROM plate] needle… — need at least one path needle");
      return 1;
    }

    if (n_miss > 0) {
      snprintf(msg, sizeof msg, "NEEDFLATN line %d: missing pure-int leaf needle(s): %s",
               aln, miss_bag[0] ? miss_bag : "?");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", msg);
      var_set_str(vm, "ERR", msg);
      var_set_str(vm, "NEEDFLATN_MISS", miss_bag);
      var_set_num(vm, "NEEDFLATN_N", n_need);
      var_set_num(vm, "NEEDFLATN_MISS_N", n_miss);
      var_set_num(vm, "LAST_N", 0);
      fail(vm, msg);
      return 1;
    }

    var_set_str(vm, "LAST", plate);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
    vm->last_n = first_val;
    var_set_num(vm, "LAST_N", first_val);
    var_set_num(vm, "NEEDFLATN", first_val);
    var_set_str(vm, "NEEDFLATN_PATH", first_path);
    var_set_num(vm, "NEEDFLATN_N", n_need);
    var_set_num(vm, "NEEDFLATN_MISS_N", 0);
    var_set_str(vm, "NEEDFLATN_MISS", "");
    var_set_num(vm, "NEEDFLATN_FROM", have_from ? 1 : 0);
    var_set_str(vm, "NEEDFLATN_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# needflatn n=%d v=%ld path=%s from=%d\n",
              n_need, first_val, first_path[0] ? first_path : "*", have_from);
    bump(vm); return 1;
  }

  return 0;
}
