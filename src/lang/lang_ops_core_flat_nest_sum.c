/* CubalC flat nest shard */
#include "lang/cubalc_lang_ops_core_flat_nest_sum.h"
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

int cubalc_lang_ops_core_try_flat_nest_sum(VM *vm, Lex *L) {
  /* SUMNOBJ|TOTALOBJ|NESTSUM [FROM plate] nest — sum int nest values → LAST_N (SUMNP dual).
   * MAXNOBJ|MINNOBJ|AVGNOBJ|MEDIANOBJ [FROM plate] nest — max/min/mean/median → LAST_N.
   * TOPKEYOBJ|BOTKEYOBJ [FROM plate] nest — key with max/min int · LAST=key LAST_N=v.
   * Soft nest miss / no ints: 0 / empty key · NEST_HIT=0.
   * Usability: nest FREQ/score without GETOBJ+SUMNP/TOPKEYP glue:
   *   SUMNOBJ "scores"
   *   TOPKEYOBJ FROM PEER "freq"
   *   AVGNOBJ "stats"
   */
  if (kw(&L->cur,"SUMNOBJ") || kw(&L->cur,"TOTALOBJ") || kw(&L->cur,"NESTSUM") ||
      kw(&L->cur,"OBJSUM") || kw(&L->cur,"MSUMNOBJ") ||
      kw(&L->cur,"MAXNOBJ") || kw(&L->cur,"NESTMAX") || kw(&L->cur,"OBJMAXN") ||
      kw(&L->cur,"MINNOBJ") || kw(&L->cur,"NESTMIN") || kw(&L->cur,"OBJMINN") ||
      kw(&L->cur,"AVGNOBJ") || kw(&L->cur,"MEANOBJ") || kw(&L->cur,"NESTAVG") ||
      kw(&L->cur,"MEDIANOBJ") || kw(&L->cur,"P50OBJ") || kw(&L->cur,"MIDOBJ") ||
      kw(&L->cur,"NESTMEDIAN") ||
      kw(&L->cur,"TOPKEYOBJ") || kw(&L->cur,"MAXKEYOBJ") || kw(&L->cur,"NESTTOPKEY") ||
      kw(&L->cur,"BOTKEYOBJ") || kw(&L->cur,"MINKEYOBJ") || kw(&L->cur,"NESTBOTKEY")) {
    char plate[CUBALC_HOST_STR_MAX], nestk[96], nest[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result ngr, hr;
    int have_from = 0, nest_hit = 0, mode = 0; /* 0 sum 1 max 2 min 3 avg 4 top 5 bot 6 median */
    Var *pv;
    const char *v;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "MAXNOBJ") == 0 || strcmp(op0, "NESTMAX") == 0 ||
        strcmp(op0, "OBJMAXN") == 0)
      mode = 1;
    else if (strcmp(op0, "MINNOBJ") == 0 || strcmp(op0, "NESTMIN") == 0 ||
             strcmp(op0, "OBJMINN") == 0)
      mode = 2;
    else if (strcmp(op0, "AVGNOBJ") == 0 || strcmp(op0, "MEANOBJ") == 0 ||
             strcmp(op0, "NESTAVG") == 0)
      mode = 3;
    else if (strcmp(op0, "TOPKEYOBJ") == 0 || strcmp(op0, "MAXKEYOBJ") == 0 ||
             strcmp(op0, "NESTTOPKEY") == 0)
      mode = 4;
    else if (strcmp(op0, "BOTKEYOBJ") == 0 || strcmp(op0, "MINKEYOBJ") == 0 ||
             strcmp(op0, "NESTBOTKEY") == 0)
      mode = 5;
    else if (strcmp(op0, "MEDIANOBJ") == 0 || strcmp(op0, "P50OBJ") == 0 ||
             strcmp(op0, "MIDOBJ") == 0 || strcmp(op0, "NESTMEDIAN") == 0)
      mode = 6;
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

    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(nestk, sizeof nestk, "%ld", kv);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
          kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM") ||
          kw(&L->cur,"END") || kw(&L->cur,"ASSERT") || kw(&L->cur,"LET")) {
        fail(vm, "SUMNOBJ|TOPKEYOBJ nest — need nest key");
        return -1;
      }
      if (resolve_str_arg(vm, L, nestk, sizeof nestk) != 0)
        nestk[0] = 0;
    } else {
      fail(vm, "SUMNOBJ|TOPKEYOBJ nest — need nest key");
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
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "SUMNOBJ|TOPKEYOBJ: empty nest");
      var_set_str(vm, "ERR", "SUMNOBJ|TOPKEYOBJ: empty nest");
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "SUMNOBJ_N", 0);
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

    /* soft nest miss */
    if (!nest_hit) {
      if (mode == 4 || mode == 5) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
      } else {
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      }
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "SUMNOBJ_N", 0);
      var_set_num(vm, "MAXNOBJ_N", 0);
      var_set_num(vm, "MINNOBJ_N", 0);
      var_set_num(vm, "AVGNOBJ_N", 0);
      var_set_num(vm, "MEDIANOBJ_N", 0);
      var_set_num(vm, "TOPKEYOBJ_N", 0);
      var_set_num(vm, "BOTKEYOBJ_N", 0);
      var_set_str(vm, "SUMNOBJ_NEST", nestk);
      var_set_str(vm, "TOPKEYOBJ_NEST", nestk);
      var_set_num(vm, "SUMNOBJ_FROM", have_from ? 1 : 0);
      var_set_num(vm, "TOPKEYOBJ_FROM", have_from ? 1 : 0);
      var_set_str(vm, "SUMNOBJ_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "SUMNOBJ_NEST_HIT", 0);
      var_set_num(vm, "TOPKEYOBJ_NEST_HIT", 0);
      var_set_str(vm, "NEST", "");
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sumnobj nest-miss nest=%s mode=%d\n", nestk, mode);
      bump(vm); return 1;
    }

    memset(&hr, 0, sizeof hr);
    if (mode == 4 || mode == 5) {
      if (cubalc_host_json_topkey(nest, mode == 5 ? 1 : 0, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "TOPKEYOBJ_N", 0);
        var_set_num(vm, "BOTKEYOBJ_N", 0);
        var_set_num(vm, "TOPKEYOBJ_FROM", have_from ? 1 : 0);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err :
                    (mode == 5 ? "BOTKEYOBJ: fail" : "TOPKEYOBJ: fail"));
        var_set_str(vm, "ERR", hr.err[0] ? hr.err :
                    (mode == 5 ? "BOTKEYOBJ: fail" : "TOPKEYOBJ: fail"));
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      if (mode == 5) {
        var_set_str(vm, "BOTKEYOBJ", hr.str);
        var_set_num(vm, "BOTKEYOBJ_N", hr.n);
        var_set_num(vm, "BOTKEYOBJ_V", hr.n);
      } else {
        var_set_str(vm, "TOPKEYOBJ", hr.str);
        var_set_num(vm, "TOPKEYOBJ_N", hr.n);
        var_set_num(vm, "TOPKEYOBJ_V", hr.n);
      }
      var_set_str(vm, "TOPKEYOBJ_NEST", nestk);
      var_set_num(vm, "TOPKEYOBJ_FROM", have_from ? 1 : 0);
      var_set_num(vm, "BOTKEYOBJ_FROM", have_from ? 1 : 0);
      var_set_str(vm, "TOPKEYOBJ_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "TOPKEYOBJ_NEST_HIT", 1);
      var_set_str(vm, "NEST", nest);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# %s nest=%s key=%s v=%ld from=%d\n",
                mode == 5 ? "botkeyobj" : "topkeyobj", nestk, hr.str, hr.n, have_from);
      bump(vm); return 1;
    }

    if (mode == 1 || mode == 2) {
      if (cubalc_host_json_minmax(nest, mode == 2 ? 1 : 0, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "MAXNOBJ_N", 0);
        var_set_num(vm, "MINNOBJ_N", 0);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err :
                    (mode == 2 ? "MINNOBJ: fail" : "MAXNOBJ: fail"));
        var_set_str(vm, "ERR", hr.err[0] ? hr.err :
                    (mode == 2 ? "MINNOBJ: fail" : "MAXNOBJ: fail"));
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
    } else if (mode == 3) {
      if (cubalc_host_json_avg(nest, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "AVGNOBJ_N", 0);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "AVGNOBJ: fail");
        var_set_str(vm, "ERR", hr.err[0] ? hr.err : "AVGNOBJ: fail");
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
    } else if (mode == 6) {
      if (cubalc_host_json_median(nest, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "MEDIANOBJ_N", 0);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "MEDIANOBJ: fail");
        var_set_str(vm, "ERR", hr.err[0] ? hr.err : "MEDIANOBJ: fail");
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
    } else {
      if (cubalc_host_json_sum(nest, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SUMNOBJ_N", 0);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "SUMNOBJ: fail");
        var_set_str(vm, "ERR", hr.err[0] ? hr.err : "SUMNOBJ: fail");
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
    }

    /* match SUMNP: keep outer plate in LAST; value in LAST_N */
    var_set_str(vm, "LAST", plate);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "NEST", nest);
    var_set_str(vm, "SUMNOBJ_NEST", nestk);
    var_set_num(vm, "SUMNOBJ_FROM", have_from ? 1 : 0);
    var_set_str(vm, "SUMNOBJ_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "SUMNOBJ_NEST_HIT", 1);
    if (mode == 1) {
      var_set_num(vm, "MAXNOBJ_N", hr.n);
      var_set_num(vm, "MAXNOBJ_FROM", have_from ? 1 : 0);
    } else if (mode == 2) {
      var_set_num(vm, "MINNOBJ_N", hr.n);
      var_set_num(vm, "MINNOBJ_FROM", have_from ? 1 : 0);
    } else if (mode == 3) {
      var_set_num(vm, "AVGNOBJ_N", hr.n);
      var_set_num(vm, "AVGNOBJ_USED", (long)hr.code);
      var_set_num(vm, "AVGNOBJ_FROM", have_from ? 1 : 0);
    } else if (mode == 6) {
      var_set_num(vm, "MEDIANOBJ_N", hr.n);
      var_set_num(vm, "P50OBJ_N", hr.n);
      var_set_num(vm, "MEDIANOBJ_USED", (long)hr.code);
      var_set_num(vm, "MEDIANOBJ_FROM", have_from ? 1 : 0);
    } else {
      var_set_num(vm, "SUMNOBJ_N", hr.n);
      var_set_num(vm, "TOTALOBJ_N", hr.n);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# sumnobj nest=%s mode=%d n=%ld from=%d\n",
              nestk, mode, hr.n, have_from);
    bump(vm); return 1;
  }

  /* SUMNP|TOTALP [FROM plate] — sum int values → LAST_N (JSONSUMN dual).
   * MAXNP|MINNP|AVGNP|MEDIANP [FROM plate] — max/min/mean/median int → LAST_N.
   * TOPKEYP|BOTKEYP [FROM plate] — key with max/min int · LAST=key LAST_N=v.
   * Bare uses PLATE. Soft 0 / empty key if no numeric fields.
   * Usability: score/FREQ plates without SYS JSONSUMN/JSONTOPKEY/MEDIANKV glue:
   *   SUMNP FROM scores
   *   TOPKEYP FROM freq
   *   AVGNP FROM PEER
   *   MEDIANP FROM scores   # robust mid vs AVGNP
   */
  if (kw(&L->cur,"SUMNP") || kw(&L->cur,"TOTALP") || kw(&L->cur,"MSUMNP") ||
      kw(&L->cur,"PLATE_SUM") || kw(&L->cur,"SUMP") ||
      kw(&L->cur,"MAXNP") || kw(&L->cur,"MMAXNP") || kw(&L->cur,"PLATE_MAXN") ||
      kw(&L->cur,"MINNP") || kw(&L->cur,"MMINNP") || kw(&L->cur,"PLATE_MINN") ||
      kw(&L->cur,"AVGNP") || kw(&L->cur,"MEANP") || kw(&L->cur,"MAVGNP") ||
      kw(&L->cur,"PLATE_AVGN") ||
      kw(&L->cur,"MEDIANP") || kw(&L->cur,"P50P") || kw(&L->cur,"MIDP") ||
      kw(&L->cur,"MMEDIANP") || kw(&L->cur,"PLATE_MEDIAN") || kw(&L->cur,"MEDP") ||
      kw(&L->cur,"TOPKEYP") || kw(&L->cur,"MAXKEYP") || kw(&L->cur,"MTOPKEYP") ||
      kw(&L->cur,"PLATE_TOPKEY") ||
      kw(&L->cur,"BOTKEYP") || kw(&L->cur,"MINKEYP") || kw(&L->cur,"MBOTKEYP") ||
      kw(&L->cur,"PLATE_BOTKEY")) {
    char plate[CUBALC_HOST_STR_MAX], from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, mode = 0; /* 0 sum 1 max 2 min 3 avg 4 top 5 bot 6 median */
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "MAXNP") == 0 || strcmp(op0, "MMAXNP") == 0 ||
        strcmp(op0, "PLATE_MAXN") == 0)
      mode = 1;
    else if (strcmp(op0, "MINNP") == 0 || strcmp(op0, "MMINNP") == 0 ||
             strcmp(op0, "PLATE_MINN") == 0)
      mode = 2;
    else if (strcmp(op0, "AVGNP") == 0 || strcmp(op0, "MEANP") == 0 ||
             strcmp(op0, "MAVGNP") == 0 || strcmp(op0, "PLATE_AVGN") == 0)
      mode = 3;
    else if (strcmp(op0, "TOPKEYP") == 0 || strcmp(op0, "MAXKEYP") == 0 ||
             strcmp(op0, "MTOPKEYP") == 0 || strcmp(op0, "PLATE_TOPKEY") == 0)
      mode = 4;
    else if (strcmp(op0, "BOTKEYP") == 0 || strcmp(op0, "MINKEYP") == 0 ||
             strcmp(op0, "MBOTKEYP") == 0 || strcmp(op0, "PLATE_BOTKEY") == 0)
      mode = 5;
    else if (strcmp(op0, "MEDIANP") == 0 || strcmp(op0, "P50P") == 0 ||
             strcmp(op0, "MIDP") == 0 || strcmp(op0, "MMEDIANP") == 0 ||
             strcmp(op0, "PLATE_MEDIAN") == 0 || strcmp(op0, "MEDP") == 0)
      mode = 6;
    else
      mode = 0;

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
    if (mode == 4 || mode == 5) {
      if (cubalc_host_json_topkey(plate, mode == 5 ? 1 : 0, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "TOPKEYP_N", 0);
        var_set_num(vm, "BOTKEYP_N", 0);
        var_set_num(vm, "TOPKEYP_FROM", have_from ? 1 : 0);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err :
                    (mode == 5 ? "BOTKEYP: fail" : "TOPKEYP: fail"));
        var_set_str(vm, "ERR", hr.err[0] ? hr.err :
                    (mode == 5 ? "BOTKEYP: fail" : "TOPKEYP: fail"));
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      if (mode == 5) {
        var_set_str(vm, "BOTKEYP", hr.str);
        var_set_num(vm, "BOTKEYP_N", hr.n);
        var_set_num(vm, "BOTKEYP_V", hr.n);
        var_set_num(vm, "JSONBOTKEY_N", hr.n);
      } else {
        var_set_str(vm, "TOPKEYP", hr.str);
        var_set_num(vm, "TOPKEYP_N", hr.n);
        var_set_num(vm, "TOPKEYP_V", hr.n);
        var_set_num(vm, "JSONTOPKEY_N", hr.n);
      }
      var_set_num(vm, "TOPKEYP_FROM", have_from ? 1 : 0);
      var_set_num(vm, "BOTKEYP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "TOPKEYP_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# %s key=%s v=%ld from=%d\n",
                mode == 5 ? "botkeyp" : "topkeyp", hr.str, hr.n, have_from);
      bump(vm); return 1;
    }

    if (mode == 1 || mode == 2) {
      if (cubalc_host_json_minmax(plate, mode == 2 ? 1 : 0, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "MAXNP_N", 0);
        var_set_num(vm, "MINNP_N", 0);
        var_set_num(vm, "MAXNP_FROM", have_from ? 1 : 0);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err :
                    (mode == 2 ? "MINNP: fail" : "MAXNP: fail"));
        var_set_str(vm, "ERR", hr.err[0] ? hr.err :
                    (mode == 2 ? "MINNP: fail" : "MAXNP: fail"));
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
    } else if (mode == 3) {
      if (cubalc_host_json_avg(plate, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "AVGNP_N", 0);
        var_set_num(vm, "AVGNP_FROM", have_from ? 1 : 0);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "AVGNP: fail");
        var_set_str(vm, "ERR", hr.err[0] ? hr.err : "AVGNP: fail");
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
    } else if (mode == 6) {
      if (cubalc_host_json_median(plate, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "MEDIANP_N", 0);
        var_set_num(vm, "MEDIANP_FROM", have_from ? 1 : 0);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "MEDIANP: fail");
        var_set_str(vm, "ERR", hr.err[0] ? hr.err : "MEDIANP: fail");
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
    } else {
      if (cubalc_host_json_sum(plate, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SUMNP_N", 0);
        var_set_num(vm, "SUMNP_FROM", have_from ? 1 : 0);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "SUMNP: fail");
        var_set_str(vm, "ERR", hr.err[0] ? hr.err : "SUMNP: fail");
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
    }

    /* keep plate in LAST for chain (JSONSUMN style) */
    var_set_str(vm, "LAST", plate);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    if (mode == 1) {
      var_set_num(vm, "MAXNP_N", hr.n);
      var_set_num(vm, "JSONMAXN_N", hr.n);
      var_set_num(vm, "MAXNP_FROM", have_from ? 1 : 0);
    } else if (mode == 2) {
      var_set_num(vm, "MINNP_N", hr.n);
      var_set_num(vm, "JSONMINN_N", hr.n);
      var_set_num(vm, "MINNP_FROM", have_from ? 1 : 0);
    } else if (mode == 3) {
      var_set_num(vm, "AVGNP_N", hr.n);
      var_set_num(vm, "JSONAVGN_N", hr.n);
      var_set_num(vm, "AVGNP_FROM", have_from ? 1 : 0);
      var_set_num(vm, "AVGNP_USED", (long)hr.code);
    } else if (mode == 6) {
      var_set_num(vm, "MEDIANP_N", hr.n);
      var_set_num(vm, "P50P_N", hr.n);
      var_set_num(vm, "MEDIANP_USED", (long)hr.code);
      var_set_num(vm, "MEDIANP_FROM", have_from ? 1 : 0);
      var_set_str(vm, "MEDIANP_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    } else {
      var_set_num(vm, "SUMNP_N", hr.n);
      var_set_num(vm, "JSONSUMN_N", hr.n);
      var_set_num(vm, "SUMNP_FROM", have_from ? 1 : 0);
    }
    var_set_str(vm, "SUMNP_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%ld from=%d\n",
              mode == 1 ? "maxnp" : mode == 2 ? "minnp" : mode == 3 ? "avgnp" :
              mode == 6 ? "medianp" : "sumnp",
              hr.n, have_from);
    bump(vm); return 1;
  }
  return 0;
}
