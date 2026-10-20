/* CubalC lang — lang_ops_core_flat_mut.c
 * GREP/PRUNE/MERGE/SET/INC/SUM/AVG/CAP/SCALE FLAT ops.
 */
#include "lang/cubalc_lang_ops_core_flat_mut.h"
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

int cubalc_lang_ops_core_try_flat_mut(VM *vm, Lex *L) {
  /* GREPFLAT|KEEPFLAT|FILTERFLAT [FROM plate] needle
   * GREPVFLAT|DROPFLAT [FROM plate] needle — invert path filter.
   * GREPFLATI|IGREPFLAT — case-insensitive path needle.
   * Flattens plate to path:val then keeps lines whose *path* contains needle.
   * Soft always OK. LAST_N = kept.
   * Usability: nest triage without FLATKV+SYS GREP glue:
   *   GREPFLAT "cfg"
   *   GREPFLAT FROM PEER "meta"
   *   GREPVFLAT "tmp"
   *   GREPFLATI "CFG.META"
   */
  if (kw(&L->cur,"GREPFLAT") || kw(&L->cur,"KEEPFLAT") || kw(&L->cur,"FILTERFLAT") ||
      kw(&L->cur,"MGREPFLAT") || kw(&L->cur,"PLATE_GREPFLAT") || kw(&L->cur,"LEAFGREP") ||
      kw(&L->cur,"GREPVFLAT") || kw(&L->cur,"DROPFLAT") || kw(&L->cur,"VGREPFLAT") ||
      kw(&L->cur,"GREPFLATI") || kw(&L->cur,"IGREPFLAT") || kw(&L->cur,"GREPIFLAT") ||
      kw(&L->cur,"GREPVFLATI") || kw(&L->cur,"DROPFLATI")) {
    char plate[CUBALC_HOST_STR_MAX], needle[256];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, invert = 0, icase = 0;
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "GREPVFLAT") == 0 || strcmp(op0, "DROPFLAT") == 0 ||
        strcmp(op0, "VGREPFLAT") == 0 || strcmp(op0, "GREPVFLATI") == 0 ||
        strcmp(op0, "DROPFLATI") == 0)
      invert = 1;
    if (strcmp(op0, "GREPFLATI") == 0 || strcmp(op0, "IGREPFLAT") == 0 ||
        strcmp(op0, "GREPIFLAT") == 0 || strcmp(op0, "GREPVFLATI") == 0 ||
        strcmp(op0, "DROPFLATI") == 0)
      icase = 1;

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
    if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
        L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"FROM") || kw(&L->cur,"END") || kw(&L->cur,"ASSERT") ||
           kw(&L->cur,"LET") || kw(&L->cur,"PRINT") || kw(&L->cur,"SYS") ||
           kw(&L->cur,"PASS") || kw(&L->cur,"FLATKV") || kw(&L->cur,"GREPFLAT") ||
           kw(&L->cur,"SETP") || kw(&L->cur,"GETP"))) {
        /* no needle → empty = all */
        needle[0] = 0;
      } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long kv = parse_expr(vm, L);
        snprintf(needle, sizeof needle, "%ld", kv);
      } else if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
        needle[0] = 0;
      }
    }

    /* trailing FROM */
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
    cubalc_host_json_leaf_grep(plate, needle, invert, icase, &hr);
    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "GREPFLAT", hr.str);
    var_set_str(vm, "KEEPFLAT", hr.str);
    var_set_num(vm, "GREPFLAT_N", hr.n);
    var_set_num(vm, "KEEPFLAT_N", hr.n);
    var_set_str(vm, "GREPFLAT_NEEDLE", needle);
    var_set_num(vm, "GREPFLAT_INVERT", invert);
    var_set_num(vm, "GREPFLAT_ICASE", icase);
    var_set_num(vm, "GREPFLAT_FROM", have_from ? 1 : 0);
    var_set_str(vm, "GREPFLAT_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# grepflat n=%ld inv=%d icase=%d needle=%s from=%d\n",
              hr.n, invert, icase, needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* PRUNEFLAT|DELFLAT|SCRUBFLAT [FROM plate] needle — drop matching leaves write-back.
   * KEEONLYFLAT|RETAINFLAT|PROJECTFLAT [FROM plate] needle — keep only matching rewrite.
   * PRUNEFLATI / KEEONLYFLATI — case-insensitive path needle.
   * LAST = plate · LAST_N = deleted|kept · write-back PLATE or FROM var.
   * Usability: scrub/project nested plate without multi DELP or GREPFLAT+UNFLAT glue:
   *   PRUNEFLAT "tmp"
   *   KEEONLYFLAT "cfg"
   *   PRUNEFLAT FROM PEER "debug"
   */
  if (kw(&L->cur,"PRUNEFLAT") || kw(&L->cur,"DELFLAT") || kw(&L->cur,"SCRUBFLAT") ||
      kw(&L->cur,"RMFLAT") || kw(&L->cur,"MPRUNEFLAT") || kw(&L->cur,"PLATE_PRUNE") ||
      kw(&L->cur,"PRUNEFLATI") || kw(&L->cur,"DELFLATI") ||
      kw(&L->cur,"KEEPONLYFLAT") || kw(&L->cur,"RETAINFLAT") || kw(&L->cur,"PROJECTFLAT") ||
      kw(&L->cur,"KEEPONLY") || kw(&L->cur,"MKEEPONLYFLAT") || kw(&L->cur,"PLATE_KEEPONLY") ||
      kw(&L->cur,"KEEPONLYFLATI") || kw(&L->cur,"RETAINFLATI")) {
    char plate[CUBALC_HOST_STR_MAX], needle[256];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, keep_only = 0, icase = 0;
    Var *pv;
    char op0[28];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "KEEPONLYFLAT") == 0 || strcmp(op0, "RETAINFLAT") == 0 ||
        strcmp(op0, "PROJECTFLAT") == 0 || strcmp(op0, "KEEPONLY") == 0 ||
        strcmp(op0, "MKEEPONLYFLAT") == 0 || strcmp(op0, "PLATE_KEEPONLY") == 0 ||
        strcmp(op0, "KEEPONLYFLATI") == 0 || strcmp(op0, "RETAINFLATI") == 0)
      keep_only = 1;
    if (strcmp(op0, "PRUNEFLATI") == 0 || strcmp(op0, "DELFLATI") == 0 ||
        strcmp(op0, "KEEPONLYFLATI") == 0 || strcmp(op0, "RETAINFLATI") == 0)
      icase = 1;

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

    if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
        L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"FROM") || kw(&L->cur,"END") || kw(&L->cur,"ASSERT") ||
           kw(&L->cur,"LET") || kw(&L->cur,"PRINT") || kw(&L->cur,"SYS") ||
           kw(&L->cur,"PASS") || kw(&L->cur,"SETP") || kw(&L->cur,"GETP") ||
           kw(&L->cur,"FLATKV") || kw(&L->cur,"GREPFLAT") || kw(&L->cur,"PRUNEFLAT"))) {
        needle[0] = 0;
      } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long kv = parse_expr(vm, L);
        snprintf(needle, sizeof needle, "%ld", kv);
      } else if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
        needle[0] = 0;
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
    if (cubalc_host_json_leaf_filter_write(plate, needle, keep_only, icase, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "PRUNEFLAT: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "PRUNEFLAT: fail");
      var_set_num(vm, "PRUNEFLAT_N", 0);
      bump(vm); return 1;
    }

    if (have_from && from_name[0] && strcmp(from_name, "LAST") != 0)
      var_set_str(vm, from_name, hr.str);
    else if (!have_from)
      var_set_str(vm, "PLATE", hr.str);
    else if (have_from && !from_name[0])
      var_set_str(vm, "PLATE", hr.str);

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "PRUNEFLAT", hr.str);
    var_set_str(vm, "KEEPONLYFLAT", hr.str);
    var_set_num(vm, "PRUNEFLAT_N", hr.n);
    var_set_num(vm, "KEEPONLYFLAT_N", hr.n);
    var_set_num(vm, "DELFLAT_N", hr.n);
    var_set_str(vm, "PRUNEFLAT_NEEDLE", needle);
    var_set_num(vm, "PRUNEFLAT_KEEP", keep_only);
    var_set_num(vm, "PRUNEFLAT_ICASE", icase);
    var_set_num(vm, "PRUNEFLAT_FROM", have_from ? 1 : 0);
    var_set_str(vm, "PRUNEFLAT_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%ld needle=%s from=%d\n",
              keep_only ? "keeponlyflat" : "pruneflat",
              hr.n, needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* MERGEFLAT|OVERLAYFLAT|PATCHFLAT [FROM base] overlay
   * Deep leaf overlay: apply overlay's FLATKV leaves onto base (nest-aware MERGEP).
   * Overlay wins per leaf path; base-only leaves kept. Write-back base (PLATE or FROM).
   * LAST = plate · LAST_N = overlay leaf count.
   * Usability: peer deep sync without FLATKV+UNFLATKV or shallow MERGEP:
   *   MERGEFLAT PEER
   *   MERGEFLAT FROM session PEER
   *   OVERLAYFLAT LAST
   */
  if (kw(&L->cur,"MERGEFLAT") || kw(&L->cur,"OVERLAYFLAT") || kw(&L->cur,"PATCHFLAT") ||
      kw(&L->cur,"DEEPMERGEP") || kw(&L->cur,"LEAFMERGE") || kw(&L->cur,"MMERGEFLAT") ||
      kw(&L->cur,"PLATE_MERGEFLAT") || kw(&L->cur,"APPLYFLAT") || kw(&L->cur,"SYNCFLAT")) {
    char base[CUBALC_HOST_STR_MAX], over[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    char oname[96];
    cubalc_host_result hr;
    int have_from = 0;
    Var *pv;

    lex_next(L);
    base[0] = 0; over[0] = 0; from_name[0] = 0; from_src[0] = 0; oname[0] = 0;

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

    /* overlay plate arg */
    if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(over, sizeof over, "%s", vm->last_str);
      snprintf(oname, sizeof oname, "%s", "LAST");
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      pv = var_get(vm, L->cur.text, 0);
      if (pv && pv->is_str) {
        snprintf(oname, sizeof oname, "%s", L->cur.text);
        snprintf(over, sizeof over, "%s", pv->sval);
        lex_next(L);
      } else if (pv) {
        snprintf(oname, sizeof oname, "%s", L->cur.text);
        snprintf(over, sizeof over, "%ld", pv->val);
        lex_next(L);
      } else if (resolve_str_arg(vm, L, over, sizeof over) != 0) {
        over[0] = 0;
      }
    } else if (resolve_str_arg(vm, L, over, sizeof over) != 0) {
      snprintf(over, sizeof over, "%s", vm->last_str);
    }
    {
      const char *bp = over;
      while (*bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\r') bp++;
      if (*bp != '{')
        snprintf(over, sizeof over, "%s", "{}");
    }

    if (have_from) {
      const char *bp = from_src;
      while (*bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\r') bp++;
      if (*bp == '{')
        snprintf(base, sizeof base, "%s", from_src);
      else
        snprintf(base, sizeof base, "%s", "{}");
    } else {
      pv = var_get(vm, "PLATE", 0);
      if (pv && pv->is_str && pv->sval[0])
        snprintf(base, sizeof base, "%s", pv->sval);
      else
        snprintf(base, sizeof base, "%s", "{}");
    }

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_leaf_merge(base, over, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "MERGEFLAT: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "MERGEFLAT: fail");
      var_set_num(vm, "MERGEFLAT_N", 0);
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
    var_set_str(vm, "MERGEFLAT", hr.str);
    var_set_str(vm, "OVERLAYFLAT", hr.str);
    var_set_num(vm, "MERGEFLAT_N", hr.n);
    var_set_num(vm, "OVERLAYFLAT_N", hr.n);
    var_set_num(vm, "MERGEFLAT_FROM", have_from ? 1 : 0);
    var_set_str(vm, "MERGEFLAT_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_str(vm, "MERGEFLAT_OVER", oname[0] ? oname : "");
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# mergeflat n=%ld base=%s over=%s\n",
              hr.n,
              from_name[0] ? from_name : "PLATE",
              oname[0] ? oname : "?");
    bump(vm); return 1;
  }

  /* RENAMEFLAT|MOVEFLAT|PREFIXFLAT [FROM plate] old_pfx new_pfx
   * Rewrite all leaf paths starting with old_pfx → new_pfx; write-back plate.
   * LAST = plate · LAST_N = renamed count · RENAMEFLAT_TOTAL = leaf count.
   * Usability: restructure nests without multi RENAMEP/DELP+SETP glue:
   *   RENAMEFLAT "cfg." "config."
   *   RENAMEFLAT FROM PEER "tmp." "meta."
   *   MOVEFLAT "a.b." "x."
   */
  if (kw(&L->cur,"RENAMEFLAT") || kw(&L->cur,"MOVEFLAT") || kw(&L->cur,"PREFIXFLAT") ||
      kw(&L->cur,"REPFXFLAT") || kw(&L->cur,"MVFLAT") || kw(&L->cur,"MRENAMEFLAT") ||
      kw(&L->cur,"PLATE_RENAMEFLAT") || kw(&L->cur,"REKEYFLAT") || kw(&L->cur,"REPATHFLAT")) {
    char plate[CUBALC_HOST_STR_MAX], oldp[192], newp[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0;
    Var *pv;

    lex_next(L);
    plate[0] = 0; oldp[0] = 0; newp[0] = 0; from_name[0] = 0; from_src[0] = 0;

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

    if (resolve_str_arg(vm, L, oldp, sizeof oldp) != 0)
      oldp[0] = 0;
    if (resolve_str_arg(vm, L, newp, sizeof newp) != 0)
      newp[0] = 0;

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

    if (!oldp[0]) {
      fail(vm, "RENAMEFLAT [FROM plate] old_pfx new_pfx — need old prefix");
      return -1;
    }

    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_leaf_rename_pfx(plate, oldp, newp, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "RENAMEFLAT: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "RENAMEFLAT: fail");
      var_set_num(vm, "RENAMEFLAT_N", 0);
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
    var_set_str(vm, "RENAMEFLAT", hr.str);
    var_set_str(vm, "MOVEFLAT", hr.str);
    var_set_num(vm, "RENAMEFLAT_N", hr.n);
    var_set_num(vm, "MOVEFLAT_N", hr.n);
    var_set_num(vm, "RENAMEFLAT_TOTAL", hr.code);
    var_set_str(vm, "RENAMEFLAT_OLD", oldp);
    var_set_str(vm, "RENAMEFLAT_NEW", newp);
    var_set_num(vm, "RENAMEFLAT_FROM", have_from ? 1 : 0);
    var_set_str(vm, "RENAMEFLAT_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# renameflat n=%ld total=%ld %s→%s from=%d\n",
              hr.n, hr.code, oldp, newp, have_from);
    bump(vm); return 1;
  }

  /* SETFLAT|MAPFLAT|SETLEAFP [FROM plate] needle value
   * Set every leaf whose path contains needle to value; write-back plate.
   * Empty needle → all leaves. LAST_N = updated · SETFLAT_TOTAL = leaf count.
   * Usability: bulk nest flags/defaults without multi SETP/GREPFLAT glue:
   *   SETFLAT "debug" 0
   *   SETFLAT "cfg.port" 9090
   *   SETFLAT FROM PEER "tls" 1
   *   MAPFLAT "" "null"
   */
  if (kw(&L->cur,"SETFLAT") || kw(&L->cur,"MAPFLAT") || kw(&L->cur,"SETLEAFP") ||
      kw(&L->cur,"SETALLFLAT") || kw(&L->cur,"MSETFLAT") || kw(&L->cur,"PLATE_SETFLAT") ||
      kw(&L->cur,"PUTFLAT") || kw(&L->cur,"ASSIGNFLAT") || kw(&L->cur,"BULKSETP")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192], val[CUBALC_HOST_STR_MAX];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0;
    Var *pv;

    lex_next(L);
    plate[0] = 0; needle[0] = 0; val[0] = 0; from_name[0] = 0; from_src[0] = 0;

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

    /* needle — allow empty string */
    if (L->cur.kind == TK_STR) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(needle, sizeof needle, "%ld", kv);
    } else if (L->cur.kind == TK_IDENT &&
               !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") &&
               !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
               !kw(&L->cur,"PASS") && !kw(&L->cur,"SETP") && !kw(&L->cur,"GETP") &&
               !kw(&L->cur,"SETFLAT") && !kw(&L->cur,"FLATKV")) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
    }

    /* value */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long kv = parse_expr(vm, L);
      snprintf(val, sizeof val, "%ld", kv);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur,"FROM") || kw(&L->cur,"END") || kw(&L->cur,"ASSERT") ||
           kw(&L->cur,"LET") || kw(&L->cur,"PRINT") || kw(&L->cur,"SYS") ||
           kw(&L->cur,"PASS") || kw(&L->cur,"SETP") || kw(&L->cur,"GETP"))) {
        fail(vm, "SETFLAT [FROM plate] needle value — need value");
        return -1;
      }
      if (resolve_str_arg(vm, L, val, sizeof val) != 0)
        val[0] = 0;
    } else {
      fail(vm, "SETFLAT [FROM plate] needle value — need value");
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
    if (cubalc_host_json_leaf_set(plate, needle, val, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "SETFLAT: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "SETFLAT: fail");
      var_set_num(vm, "SETFLAT_N", 0);
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
    var_set_str(vm, "SETFLAT", hr.str);
    var_set_str(vm, "MAPFLAT", hr.str);
    var_set_num(vm, "SETFLAT_N", hr.n);
    var_set_num(vm, "MAPFLAT_N", hr.n);
    var_set_num(vm, "SETFLAT_TOTAL", hr.code);
    var_set_str(vm, "SETFLAT_NEEDLE", needle);
    var_set_str(vm, "SETFLAT_VAL", val);
    var_set_num(vm, "SETFLAT_FROM", have_from ? 1 : 0);
    var_set_str(vm, "SETFLAT_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# setflat n=%ld total=%ld needle=%s from=%d\n",
              hr.n, hr.code, needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* INCFLAT|BUMPFLAT|ADDFLAT [FROM plate] needle [delta]
   * Bump pure-int leaves whose path contains needle (default delta 1).
   * Non-numeric matching leaves soft-skipped. Empty needle → all pure-int leaves.
   * LAST_N = bumped · INCFLAT_TOTAL = leaf count. Write-back plate.
   * Usability: bulk nest counters without multi INCP glue:
   *   INCFLAT "hits"
   *   INCFLAT "cfg.meta.x" 5
   *   INCFLAT FROM PEER "stats" -1
   */
  if (kw(&L->cur,"INCFLAT") || kw(&L->cur,"BUMPFLAT") || kw(&L->cur,"ADDFLAT") ||
      kw(&L->cur,"TICKFLAT") || kw(&L->cur,"MINCFLAT") || kw(&L->cur,"PLATE_INCFLAT") ||
      kw(&L->cur,"INCALLFLAT") || kw(&L->cur,"BULKINC") || kw(&L->cur,"DECFLAT")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, is_dec = 0;
    long delta = 1;
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "DECFLAT") == 0)
      is_dec = 1;

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
      /* bare number as needle only if next is also number (delta) — treat as needle str */
      long kv = parse_expr(vm, L);
      snprintf(needle, sizeof needle, "%ld", kv);
    } else if (L->cur.kind == TK_IDENT &&
               !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") &&
               !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
               !kw(&L->cur,"PASS") && !kw(&L->cur,"SETP") && !kw(&L->cur,"INCP") &&
               !kw(&L->cur,"INCFLAT") && !kw(&L->cur,"SETFLAT")) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
    }

    /* optional delta */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      delta = parse_expr(vm, L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      /* avoid LAST as delta unless numeric var — skip */
    } else if (L->cur.kind == TK_IDENT) {
      pv = var_get(vm, L->cur.text, 0);
      if (pv && !pv->is_str) {
        delta = pv->val;
        lex_next(L);
      }
    }
    if (is_dec && delta > 0)
      delta = -delta;
    else if (is_dec && delta == 1)
      delta = -1;

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
    if (cubalc_host_json_leaf_inc(plate, needle, delta, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "INCFLAT: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "INCFLAT: fail");
      var_set_num(vm, "INCFLAT_N", 0);
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
    var_set_str(vm, "INCFLAT", hr.str);
    var_set_str(vm, "BUMPFLAT", hr.str);
    var_set_num(vm, "INCFLAT_N", hr.n);
    var_set_num(vm, "BUMPFLAT_N", hr.n);
    var_set_num(vm, "INCFLAT_TOTAL", hr.code);
    var_set_num(vm, "INCFLAT_DELTA", delta);
    var_set_str(vm, "INCFLAT_NEEDLE", needle);
    var_set_num(vm, "INCFLAT_FROM", have_from ? 1 : 0);
    var_set_str(vm, "INCFLAT_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# incflat n=%ld total=%ld delta=%ld needle=%s from=%d\n",
              hr.n, hr.code, delta, needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* SUMFLAT|TOTALFLAT [FROM plate] [needle]
   * Sum pure-int leaves whose path contains needle → LAST_N.
   * Non-numeric matching leaves soft-skipped. Empty/omitted needle → all pure-int.
   * Read-only (plate unchanged). SUMFLAT_N = count of ints summed.
   * Usability: nest counter rollups without FLATKV+GREPFLAT+SUM glue:
   *   SUMFLAT "hits"
   *   SUMFLAT ""
   *   SUMFLAT FROM PEER "stats"
   */
  if (kw(&L->cur,"SUMFLAT") || kw(&L->cur,"TOTALFLAT") || kw(&L->cur,"LEAFSUM") ||
      kw(&L->cur,"MSUMFLAT") || kw(&L->cur,"PLATE_SUMFLAT") || kw(&L->cur,"SUMLEAFP") ||
      kw(&L->cur,"FLATTOTAL") || kw(&L->cur,"SUMALLFLAT")) {
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
               !kw(&L->cur,"INCFLAT") && !kw(&L->cur,"SETFLAT") && !kw(&L->cur,"SUMFLAT")) {
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
    if (cubalc_host_json_leaf_sum(plate, needle, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "SUMFLAT: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "SUMFLAT: fail");
      var_set_num(vm, "SUMFLAT_N", 0);
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    /* Read-only: keep plate in LAST (like SUMNP); value in LAST_N */
    var_set_str(vm, "LAST", plate);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "SUMFLAT", hr.str);
    var_set_str(vm, "TOTALFLAT", hr.str);
    var_set_num(vm, "SUMFLAT_N", (long)hr.code); /* count of ints */
    var_set_num(vm, "TOTALFLAT_N", (long)hr.code);
    var_set_num(vm, "SUMFLAT_SUM", hr.n);
    var_set_str(vm, "SUMFLAT_NEEDLE", needle);
    var_set_num(vm, "SUMFLAT_FROM", have_from ? 1 : 0);
    var_set_str(vm, "SUMFLAT_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# sumflat sum=%ld n=%ld needle=%s from=%d\n",
              hr.n, (long)hr.code, needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* AVGFLAT|MEANFLAT [FROM plate] [needle]
   * Integer mean of pure-int leaves whose path contains needle → LAST_N.
   * Truncates toward zero (like SYS AVG). Non-numeric matching leaves soft-skipped.
   * Empty/omitted needle → all pure-int. Read-only. AVGFLAT_N = count used.
   * AVGFLAT_SUM = sum of ints. Soft empty → LAST_N=0 AVGFLAT_N=0.
   * Usability: nest mean scores without FLATKV+GREP+AVG or SUMFLAT+DIV glue:
   *   AVGFLAT "score"
   *   AVGFLAT ""
   *   AVGFLAT FROM PEER "latency"
   */
  if (kw(&L->cur,"AVGFLAT") || kw(&L->cur,"MEANFLAT") || kw(&L->cur,"AVGLEAF") ||
      kw(&L->cur,"MAVGFLAT") || kw(&L->cur,"PLATE_AVGFLAT") || kw(&L->cur,"LEAFMEAN") ||
      kw(&L->cur,"FLATMEAN") || kw(&L->cur,"AVGALLFLAT")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0;
    long sum_side = 0;
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
               !kw(&L->cur,"AVGFLAT") && !kw(&L->cur,"SUMFLAT") && !kw(&L->cur,"MEANFLAT")) {
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
    if (cubalc_host_json_leaf_avg(plate, needle, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "AVGFLAT: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "AVGFLAT: fail");
      var_set_num(vm, "AVGFLAT_N", 0);
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    sum_side = 0;
    if (hr.err[0]) {
      char *end = NULL;
      long s = strtol(hr.err, &end, 10);
      if (end && end != hr.err && *end == 0)
        sum_side = s;
    }

    /* Read-only: keep plate in LAST; mean in LAST_N */
    var_set_str(vm, "LAST", plate);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "AVGFLAT", hr.str);
    var_set_str(vm, "MEANFLAT", hr.str);
    var_set_num(vm, "AVGFLAT_N", (long)hr.code);
    var_set_num(vm, "MEANFLAT_N", (long)hr.code);
    var_set_num(vm, "AVGFLAT_AVG", hr.n);
    var_set_num(vm, "AVGFLAT_SUM", sum_side);
    var_set_str(vm, "AVGFLAT_NEEDLE", needle);
    var_set_num(vm, "AVGFLAT_FROM", have_from ? 1 : 0);
    var_set_str(vm, "AVGFLAT_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# avgflat avg=%ld n=%ld sum=%ld needle=%s from=%d\n",
              hr.n, (long)hr.code, sum_side, needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* MEDIANFLAT|P50FLAT [FROM plate] [needle]
   * Integer median of pure-int leaves whose path contains needle → LAST_N.
   * Sort ascending; even n → lower mid (like SYS MEDIAN). Cap 256.
   * Empty/omitted needle → all pure-int. Read-only. MEDIANFLAT_N = count used.
   * Soft empty → LAST_N=0 MEDIANFLAT_N=0.
   * Usability: robust nest mid scores without FLATKV+GREP+MEDIAN glue:
   *   MEDIANFLAT "score"
   *   MEDIANFLAT ""
   *   MEDIANFLAT FROM PEER "latency"
   */
  if (kw(&L->cur,"MEDIANFLAT") || kw(&L->cur,"P50FLAT") || kw(&L->cur,"MIDFLAT") ||
      kw(&L->cur,"MMEDIANFLAT") || kw(&L->cur,"PLATE_MEDIANFLAT") || kw(&L->cur,"LEAFMEDIAN") ||
      kw(&L->cur,"FLATMEDIAN") || kw(&L->cur,"MEDALLFLAT")) {
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
               !kw(&L->cur,"MEDIANFLAT") && !kw(&L->cur,"AVGFLAT") && !kw(&L->cur,"SUMFLAT")) {
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
    if (cubalc_host_json_leaf_median(plate, needle, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "MEDIANFLAT: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "MEDIANFLAT: fail");
      var_set_num(vm, "MEDIANFLAT_N", 0);
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    var_set_str(vm, "LAST", plate);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "MEDIANFLAT", hr.str);
    var_set_str(vm, "P50FLAT", hr.str);
    var_set_num(vm, "MEDIANFLAT_N", (long)hr.code);
    var_set_num(vm, "P50FLAT_N", (long)hr.code);
    var_set_num(vm, "MEDIANFLAT_MED", hr.n);
    var_set_str(vm, "MEDIANFLAT_NEEDLE", needle);
    var_set_num(vm, "MEDIANFLAT_FROM", have_from ? 1 : 0);
    var_set_str(vm, "MEDIANFLAT_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# medianflat med=%ld n=%ld needle=%s from=%d\n",
              hr.n, (long)hr.code, needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* TOPPATHFLAT|MAXPATHFLAT [FROM plate] [needle]
   * BOTPATHFLAT|MINPATHFLAT [FROM plate] [needle]
   * Path of max/min pure-int leaf matching needle → LAST; value → LAST_N.
   * Empty needle → all pure-int. Soft miss → LAST="" LAST_N=0. First path wins ties.
   * Usability: locate dominant/min nest counter without GREPFLAT+EACH walk:
   *   TOPPATHFLAT "score"
   *   BOTPATHFLAT "latency"
   *   TOPPATHFLAT FROM PEER "stats"
   */
  if (kw(&L->cur,"TOPPATHFLAT") || kw(&L->cur,"MAXPATHFLAT") || kw(&L->cur,"TOPFLAT") ||
      kw(&L->cur,"MTOPPATHFLAT") || kw(&L->cur,"PLATE_TOPPATH") ||
      kw(&L->cur,"BOTPATHFLAT") || kw(&L->cur,"MINPATHFLAT") || kw(&L->cur,"BOTFLAT") ||
      kw(&L->cur,"MBOTPATHFLAT") || kw(&L->cur,"PLATE_BOTPATH")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, want_min = 0;
    Var *pv;
    char op0[24];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "BOTPATHFLAT") == 0 || strcmp(op0, "MINPATHFLAT") == 0 ||
        strcmp(op0, "BOTFLAT") == 0 || strcmp(op0, "MBOTPATHFLAT") == 0 ||
        strcmp(op0, "PLATE_BOTPATH") == 0)
      want_min = 1;

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
               !kw(&L->cur,"SUMFLAT") && !kw(&L->cur,"TOPPATHFLAT") && !kw(&L->cur,"BOTPATHFLAT")) {
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
    if (cubalc_host_json_leaf_toppath(plate, needle, want_min, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "TOPPATHFLAT: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "TOPPATHFLAT: fail");
      var_set_num(vm, "TOPPATHFLAT_N", 0);
      var_set_str(vm, "LAST", "");
      var_set_num(vm, "LAST_N", 0);
      bump(vm); return 1;
    }

    /* LAST = winning path · LAST_N = extreme value (TOPKEYP dual for deep leaves) */
    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    if (want_min) {
      var_set_str(vm, "BOTPATHFLAT", hr.str);
      var_set_str(vm, "MINPATHFLAT", hr.str);
      var_set_num(vm, "BOTPATHFLAT_N", (long)hr.code);
      var_set_num(vm, "BOTPATHFLAT_V", hr.n);
      var_set_str(vm, "BOTPATHFLAT_NEEDLE", needle);
      var_set_num(vm, "BOTPATHFLAT_FROM", have_from ? 1 : 0);
      var_set_str(vm, "BOTPATHFLAT_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    } else {
      var_set_str(vm, "TOPPATHFLAT", hr.str);
      var_set_str(vm, "MAXPATHFLAT", hr.str);
      var_set_num(vm, "TOPPATHFLAT_N", (long)hr.code);
      var_set_num(vm, "TOPPATHFLAT_V", hr.n);
      var_set_str(vm, "TOPPATHFLAT_NEEDLE", needle);
      var_set_num(vm, "TOPPATHFLAT_FROM", have_from ? 1 : 0);
      var_set_str(vm, "TOPPATHFLAT_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s path=%s v=%ld n=%ld needle=%s from=%d\n",
              want_min ? "botpathflat" : "toppathflat",
              hr.str[0] ? hr.str : "(none)", hr.n, (long)hr.code,
              needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* THRESHFLAT|KEEPVFLAT [FROM plate] needle min
   * DROPZEROFLAT|NZFLAT [FROM plate] [needle]
   * Keep pure-int leaves value>=min (or drop zeros) among path matches · write-back.
   * Non-int leaves kept. Empty needle → all pure-int. LAST_N = kept · DROP = dropped.
   * Usability: nest FREQ denoise without GETOBJ+THRESHP/DROPZEROP+SETOBJ:
   *   THRESHFLAT "hits" 5
   *   DROPZEROFLAT "delta"
   *   THRESHFLAT FROM PEER "stats" 1
   */
  if (kw(&L->cur,"THRESHFLAT") || kw(&L->cur,"KEEPVFLAT") || kw(&L->cur,"MINCOUNTFLAT") ||
      kw(&L->cur,"MTHRESHFLAT") || kw(&L->cur,"PLATE_THRESHFLAT") || kw(&L->cur,"KEEPMINFLAT") ||
      kw(&L->cur,"DROPZEROFLAT") || kw(&L->cur,"NZFLAT") || kw(&L->cur,"KEEPNZFLAT") ||
      kw(&L->cur,"MDROPZEROFLAT") || kw(&L->cur,"PLATE_DROPZEROFLAT")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, drop_zero = 0, have_min = 0;
    long minv = 0;
    Var *pv;
    char op0[28];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "DROPZEROFLAT") == 0 || strcmp(op0, "NZFLAT") == 0 ||
        strcmp(op0, "KEEPNZFLAT") == 0 || strcmp(op0, "MDROPZEROFLAT") == 0 ||
        strcmp(op0, "PLATE_DROPZEROFLAT") == 0)
      drop_zero = 1;

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
    } else if (!drop_zero && (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
                              L->cur.kind == TK_LPAREN)) {
      /* THRESHFLAT min with empty needle: THRESHFLAT 5 */
      minv = parse_expr(vm, L);
      have_min = 1;
    } else if (L->cur.kind == TK_IDENT &&
               !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") &&
               !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
               !kw(&L->cur,"PASS") && !kw(&L->cur,"SETP") && !kw(&L->cur,"INCP") &&
               !kw(&L->cur,"THRESHFLAT") && !kw(&L->cur,"DROPZEROFLAT")) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
    }

    if (!drop_zero && !have_min) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        minv = parse_expr(vm, L);
        have_min = 1;
      } else if (L->cur.kind == TK_IDENT) {
        pv = var_get(vm, L->cur.text, 0);
        if (pv && !pv->is_str) {
          minv = pv->val;
          have_min = 1;
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

    if (!drop_zero && !have_min) {
      fail(vm, "THRESHFLAT [FROM plate] [needle] min — need min int");
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
    if (cubalc_host_json_leaf_thresh(plate, needle, minv, drop_zero, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "THRESHFLAT: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "THRESHFLAT: fail");
      var_set_num(vm, "THRESHFLAT_N", 0);
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
    if (drop_zero) {
      var_set_str(vm, "DROPZEROFLAT", hr.str);
      var_set_str(vm, "NZFLAT", hr.str);
      var_set_num(vm, "DROPZEROFLAT_N", hr.n);
      var_set_num(vm, "DROPZEROFLAT_DROP", (long)hr.code);
      var_set_str(vm, "DROPZEROFLAT_NEEDLE", needle);
      var_set_num(vm, "DROPZEROFLAT_FROM", have_from ? 1 : 0);
      var_set_str(vm, "DROPZEROFLAT_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    } else {
      var_set_str(vm, "THRESHFLAT", hr.str);
      var_set_str(vm, "KEEPVFLAT", hr.str);
      var_set_num(vm, "THRESHFLAT_N", hr.n);
      var_set_num(vm, "THRESHFLAT_DROP", (long)hr.code);
      var_set_num(vm, "THRESHFLAT_MIN", minv);
      var_set_str(vm, "THRESHFLAT_NEEDLE", needle);
      var_set_num(vm, "THRESHFLAT_FROM", have_from ? 1 : 0);
      var_set_str(vm, "THRESHFLAT_SRC",
                  from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s kept=%ld drop=%ld min=%ld needle=%s from=%d\n",
              drop_zero ? "dropzeroflat" : "threshflat",
              hr.n, (long)hr.code, minv, needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* CAPFLAT|CLAMPFLAT [FROM plate] [needle] max
   * Clamp pure-int leaves value>max down to max among path matches · write-back.
   * Non-int kept. Empty needle → all pure-int. LAST_N = capped · CAPFLAT_MATCH = considered.
   * Usability: nest score ceiling without multi GETP+SETP or CAPP glue:
   *   CAPFLAT "score" 100
   *   CAPFLAT 50
   *   CAPFLAT FROM PEER "latency" 1000
   */
  if (kw(&L->cur,"CAPFLAT") || kw(&L->cur,"CLAMPFLAT") || kw(&L->cur,"MAXVFLAT") ||
      kw(&L->cur,"MCAPFLAT") || kw(&L->cur,"PLATE_CAPFLAT") || kw(&L->cur,"CEILFLAT") ||
      kw(&L->cur,"MAXCAPFLAT")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, have_max = 0;
    long maxv = 0;
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
      /* CAPFLAT max with empty needle: CAPFLAT 100 */
      maxv = parse_expr(vm, L);
      have_max = 1;
    } else if (L->cur.kind == TK_IDENT &&
               !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") &&
               !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
               !kw(&L->cur,"PASS") && !kw(&L->cur,"SETP") && !kw(&L->cur,"INCP") &&
               !kw(&L->cur,"CAPFLAT") && !kw(&L->cur,"THRESHFLAT")) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
    }

    if (!have_max) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        maxv = parse_expr(vm, L);
        have_max = 1;
      } else if (L->cur.kind == TK_IDENT) {
        pv = var_get(vm, L->cur.text, 0);
        if (pv && !pv->is_str) {
          maxv = pv->val;
          have_max = 1;
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

    if (!have_max) {
      fail(vm, "CAPFLAT [FROM plate] [needle] max — need max int");
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
    if (cubalc_host_json_leaf_cap(plate, needle, maxv, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "CAPFLAT: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "CAPFLAT: fail");
      var_set_num(vm, "CAPFLAT_N", 0);
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
    var_set_str(vm, "CAPFLAT", hr.str);
    var_set_str(vm, "CLAMPFLAT", hr.str);
    var_set_num(vm, "CAPFLAT_N", hr.n);
    var_set_num(vm, "CLAMPFLAT_N", hr.n);
    var_set_num(vm, "CAPFLAT_MATCH", (long)hr.code);
    var_set_num(vm, "CAPFLAT_MAX", maxv);
    var_set_str(vm, "CAPFLAT_NEEDLE", needle);
    var_set_num(vm, "CAPFLAT_FROM", have_from ? 1 : 0);
    var_set_str(vm, "CAPFLAT_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# capflat n=%ld match=%ld max=%ld needle=%s from=%d\n",
              hr.n, (long)hr.code, maxv, needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  /* SCALEFLAT|MULFLAT [FROM plate] [needle] factor
   * Multiply pure-int leaves by factor among path matches · write-back.
   * Non-int kept. Empty needle → all pure-int. LAST_N = scaled · SCALEFLAT_MATCH.
   * Usability: nest score scale without multi GETP+arith+SETP glue:
   *   SCALEFLAT "score" 2
   *   SCALEFLAT 10
   *   MULFLAT FROM PEER "stats" 3
   */
  if (kw(&L->cur,"SCALEFLAT") || kw(&L->cur,"MULFLAT") || kw(&L->cur,"TIMESFLAT") ||
      kw(&L->cur,"MSCALEFLAT") || kw(&L->cur,"PLATE_SCALEFLAT") || kw(&L->cur,"MULTFLAT") ||
      kw(&L->cur,"FACTORFLAT")) {
    char plate[CUBALC_HOST_STR_MAX], needle[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0, have_fac = 0;
    long factor = 1;
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
      /* SCALEFLAT factor with empty needle: SCALEFLAT 2 */
      factor = parse_expr(vm, L);
      have_fac = 1;
    } else if (L->cur.kind == TK_IDENT &&
               !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") &&
               !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
               !kw(&L->cur,"PASS") && !kw(&L->cur,"SETP") && !kw(&L->cur,"INCP") &&
               !kw(&L->cur,"SCALEFLAT") && !kw(&L->cur,"CAPFLAT") && !kw(&L->cur,"INCFLAT")) {
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
    }

    if (!have_fac) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        factor = parse_expr(vm, L);
        have_fac = 1;
      } else if (L->cur.kind == TK_IDENT) {
        pv = var_get(vm, L->cur.text, 0);
        if (pv && !pv->is_str) {
          factor = pv->val;
          have_fac = 1;
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

    if (!have_fac) {
      fail(vm, "SCALEFLAT [FROM plate] [needle] factor — need factor int");
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
    if (cubalc_host_json_leaf_scale(plate, needle, factor, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "SCALEFLAT: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "SCALEFLAT: fail");
      var_set_num(vm, "SCALEFLAT_N", 0);
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
    var_set_str(vm, "SCALEFLAT", hr.str);
    var_set_str(vm, "MULFLAT", hr.str);
    var_set_num(vm, "SCALEFLAT_N", hr.n);
    var_set_num(vm, "MULFLAT_N", hr.n);
    var_set_num(vm, "SCALEFLAT_MATCH", (long)hr.code);
    var_set_num(vm, "SCALEFLAT_FACTOR", factor);
    var_set_str(vm, "SCALEFLAT_NEEDLE", needle);
    var_set_num(vm, "SCALEFLAT_FROM", have_from ? 1 : 0);
    var_set_str(vm, "SCALEFLAT_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# scaleflat n=%ld match=%ld factor=%ld needle=%s from=%d\n",
              hr.n, (long)hr.code, factor, needle[0] ? needle : "*", have_from);
    bump(vm); return 1;
  }

  return 0;
}
