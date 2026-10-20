/* CubalC lang — lang_ops_core_flat_path.c
 * PATHKEYS · FLATKV · UNFLATKV · DIFFFLAT inventory plane (A_core peel).
 * Mesh vote A_core_forms_agent_peel · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_core_flat_path.h"
#include "lang/cubalc_lang_ops_sys_plate_expand.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

int cubalc_lang_ops_core_try_flat_path(VM *vm, Lex *L) {
  /* PATHKEYS/FLATKV/UNFLATKV/DIFFFLAT plane.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
  /* PATHKEYS|LEAFKEYS|DOTPATHS|FLATPATHS [FROM plate] [path]
   * Recursively list dotted leaf paths under plate (or nest path) → LAST bag.
   * LAST_N = count · PATHKEYS_PATH · PATHKEYS_FROM.
   * Soft empty on miss/non-object. Multi-plate FROM.
   * Usability: agent discover deep plate structure without KEYSP+GETOBJ walk:
   *   PATHKEYS                          → host\ncfg.meta.role\ncfg.port\n…
   *   PATHKEYS "cfg"                    → meta.role\nport\n…
   *   PATHKEYS FROM PEER "net"
   */
  if (kw(&L->cur,"PATHKEYS") || kw(&L->cur,"LEAFKEYS") || kw(&L->cur,"DOTPATHS") ||
      kw(&L->cur,"FLATPATHS") || kw(&L->cur,"LEAFPATHS") || kw(&L->cur,"ALLDOTS") ||
      kw(&L->cur,"DOTKEYS") || kw(&L->cur,"FLATP") || kw(&L->cur,"PATHSP") ||
      kw(&L->cur,"PLATE_PATHS") || kw(&L->cur,"MPATHKEYS")) {
    char plate[CUBALC_HOST_STR_MAX];
    char nest_path[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0;
    Var *pv;

    lex_next(L);
    plate[0] = 0; nest_path[0] = 0; from_name[0] = 0; from_src[0] = 0;

    /* optional leading FROM plate_src */
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

    /* optional nest path */
    if (L->cur.kind == TK_STR ||
        (L->cur.kind == TK_IDENT &&
         !kw(&L->cur,"FROM") && !kw(&L->cur,"USING") && !kw(&L->cur,"OF") &&
         !kw(&L->cur,"WITHPLATE") && !kw(&L->cur,"PLATEFROM") &&
         !kw(&L->cur,"END") && !kw(&L->cur,"ELSE") && !kw(&L->cur,"ELIF") &&
         !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") &&
         !kw(&L->cur,"SYS") && !kw(&L->cur,"PASS") && !kw(&L->cur,"FAIL") &&
         !kw(&L->cur,"INCLUDE") && !kw(&L->cur,"SETP") && !kw(&L->cur,"GETP") &&
         !kw(&L->cur,"KEYSP") && !kw(&L->cur,"PATHKEYS") && !kw(&L->cur,"LEAFKEYS"))) {
      if (resolve_str_arg(vm, L, nest_path, sizeof nest_path) != 0)
        nest_path[0] = 0;
    }

    /* trailing FROM if not already */
    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                       kw(&L->cur,"PLATEFROM"))) {
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
      /* path after FROM PEER: PATHKEYS FROM PEER "cfg" */
      if (!nest_path[0] &&
          (L->cur.kind == TK_STR ||
           (L->cur.kind == TK_IDENT &&
            !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ELSE") &&
            !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") && !kw(&L->cur,"PASS")))) {
        if (resolve_str_arg(vm, L, nest_path, sizeof nest_path) != 0)
          nest_path[0] = 0;
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
    if (cubalc_host_json_leaf_paths(plate, nest_path[0] ? nest_path : NULL, &hr) != 0) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "PATHKEYS_N", 0);
      var_set_num(vm, "LEAFKEYS_N", 0);
      var_set_num(vm, "PATHKEYS_FROM", have_from ? 1 : 0);
      var_set_str(vm, "PATHKEYS_PATH", nest_path);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "PATHKEYS", hr.str);
    var_set_str(vm, "LEAFKEYS", hr.str);
    var_set_str(vm, "DOTPATHS", hr.str);
    var_set_num(vm, "PATHKEYS_N", hr.n);
    var_set_num(vm, "LEAFKEYS_N", hr.n);
    var_set_num(vm, "DOTPATHS_N", hr.n);
    var_set_num(vm, "PATHKEYS_FROM", have_from ? 1 : 0);
    var_set_str(vm, "PATHKEYS_PATH", nest_path);
    var_set_str(vm, "PATHKEYS_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# pathkeys n=%ld from=%d path=%s\n",
              hr.n, have_from, nest_path[0] ? nest_path : ".");
    bump(vm); return 1;
  }

  /* FLATKV|LEAFKV|TOKVPATH|FLATTENP [FROM plate] [path]
   * Recursive path:value bag (key:val lines) under plate or nest path.
   * LAST_N = pairs · FLATKV_PATH · FLATKV_FROM.
   * Soft empty on miss. Multi-plate FROM.
   * Usability: agent flat inventory without PATHKEYS+GETP or shallow TOKVP:
   *   FLATKV                     → host:cubeA\ncfg.meta.role:worker\n…
   *   FLATKV "cfg"               → meta.role:worker\nport:8080\n…
   *   FLATKV FROM PEER "net"
   */
  if (kw(&L->cur,"FLATKV") || kw(&L->cur,"LEAFKV") || kw(&L->cur,"TOKVPATH") ||
      kw(&L->cur,"PATHTOKV") || kw(&L->cur,"FLATTENP") || kw(&L->cur,"FLATPLATE") ||
      kw(&L->cur,"TOKVALL") || kw(&L->cur,"LEAFTOKV") || kw(&L->cur,"MFLATKV") ||
      kw(&L->cur,"PLATE_FLAT") || kw(&L->cur,"DOTKV")) {
    char plate[CUBALC_HOST_STR_MAX];
    char nest_path[192];
    char from_name[96], from_src[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int have_from = 0;
    Var *pv;

    lex_next(L);
    plate[0] = 0; nest_path[0] = 0; from_name[0] = 0; from_src[0] = 0;

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

    if (L->cur.kind == TK_STR ||
        (L->cur.kind == TK_IDENT &&
         !kw(&L->cur,"FROM") && !kw(&L->cur,"USING") && !kw(&L->cur,"OF") &&
         !kw(&L->cur,"WITHPLATE") && !kw(&L->cur,"PLATEFROM") &&
         !kw(&L->cur,"END") && !kw(&L->cur,"ELSE") && !kw(&L->cur,"ELIF") &&
         !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") &&
         !kw(&L->cur,"SYS") && !kw(&L->cur,"PASS") && !kw(&L->cur,"FAIL") &&
         !kw(&L->cur,"INCLUDE") && !kw(&L->cur,"SETP") && !kw(&L->cur,"GETP") &&
         !kw(&L->cur,"KEYSP") && !kw(&L->cur,"PATHKEYS") && !kw(&L->cur,"FLATKV") &&
         !kw(&L->cur,"LEAFKV") && !kw(&L->cur,"FLATTENP"))) {
      if (resolve_str_arg(vm, L, nest_path, sizeof nest_path) != 0)
        nest_path[0] = 0;
    }

    if (!have_from && (kw(&L->cur,"FROM") || kw(&L->cur,"USING") ||
                       kw(&L->cur,"OF") || kw(&L->cur,"WITHPLATE") ||
                       kw(&L->cur,"PLATEFROM"))) {
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
      if (!nest_path[0] &&
          (L->cur.kind == TK_STR ||
           (L->cur.kind == TK_IDENT &&
            !kw(&L->cur,"FROM") && !kw(&L->cur,"END") && !kw(&L->cur,"ELSE") &&
            !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") && !kw(&L->cur,"PASS")))) {
        if (resolve_str_arg(vm, L, nest_path, sizeof nest_path) != 0)
          nest_path[0] = 0;
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
    if (cubalc_host_json_leaf_kv(plate, nest_path[0] ? nest_path : NULL, &hr) != 0) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "FLATKV_N", 0);
      var_set_num(vm, "FLATKV_FROM", have_from ? 1 : 0);
      var_set_str(vm, "FLATKV_PATH", nest_path);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "FLATKV", hr.str);
    var_set_str(vm, "LEAFKV", hr.str);
    var_set_str(vm, "FLATTENP", hr.str);
    var_set_num(vm, "FLATKV_N", hr.n);
    var_set_num(vm, "LEAFKV_N", hr.n);
    var_set_num(vm, "FLATTENP_N", hr.n);
    var_set_num(vm, "FLATKV_FROM", have_from ? 1 : 0);
    var_set_str(vm, "FLATKV_PATH", nest_path);
    var_set_str(vm, "FLATKV_SRC",
                from_name[0] ? from_name : (have_from ? "" : "PLATE"));
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# flatkv n=%ld from=%d path=%s\n",
              hr.n, have_from, nest_path[0] ? nest_path : ".");
    bump(vm); return 1;
  }

  /* UNFLATKV|FROMFLAT|UNFLATTENP|PATHFROMKV [bag|LAST] [UNDER path] [INTO name]
   * Apply path:value bag → nested plate (round-trip of FLATKV).
   * Empty val → {}. Optional UNDER nest prefix for relative bags.
   * INTO writes named plate; default write-back PLATE.
   * Usability: agent edit flat inventory then re-nest without multi SETP:
   *   FLATKV → bag · …edit… · UNFLATKV bag
   *   UNFLATKV "cfg.port:9\nhost:x"
   *   UNFLATKV LAST UNDER "cfg" INTO PEER
   */
  if (kw(&L->cur,"UNFLATKV") || kw(&L->cur,"FROMFLAT") || kw(&L->cur,"UNFLATTENP") ||
      kw(&L->cur,"PATHFROMKV") || kw(&L->cur,"UNFLAT") || kw(&L->cur,"NESTFROMFLAT") ||
      kw(&L->cur,"APPLYFLAT") || kw(&L->cur,"MUNFLATKV") || kw(&L->cur,"PLATE_UNFLAT")) {
    char plate[CUBALC_HOST_STR_MAX], bag[CUBALC_HOST_STR_MAX];
    char under[192], into_name[96];
    cubalc_host_result hr;
    int have_bag = 0, have_under = 0, have_into = 0;
    Var *pv;

    lex_next(L);
    plate[0] = 0; bag[0] = 0; under[0] = 0; into_name[0] = 0;

    /* bag: LAST bare, string, or var */
    if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(bag, sizeof bag, "%s", vm->last_str);
      have_bag = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_STR ||
               (L->cur.kind == TK_IDENT &&
                !kw(&L->cur,"UNDER") && !kw(&L->cur,"AT") && !kw(&L->cur,"IN") &&
                !kw(&L->cur,"INTO") && !kw(&L->cur,"TO") && !kw(&L->cur,"FROM") &&
                !kw(&L->cur,"END") && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
                !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") && !kw(&L->cur,"PASS") &&
                !kw(&L->cur,"SETP") && !kw(&L->cur,"GETP") && !kw(&L->cur,"FLATKV") &&
                !kw(&L->cur,"UNFLATKV") && !kw(&L->cur,"PATHKEYS"))) {
      if (L->cur.kind == TK_IDENT) {
        pv = var_get(vm, L->cur.text, 0);
        if (pv && pv->is_str) {
          snprintf(bag, sizeof bag, "%s", pv->sval);
          have_bag = 1;
          lex_next(L);
        } else if (resolve_str_arg(vm, L, bag, sizeof bag) == 0) {
          have_bag = 1;
        }
      } else if (resolve_str_arg(vm, L, bag, sizeof bag) == 0) {
        have_bag = 1;
      }
    }
    if (!have_bag) {
      snprintf(bag, sizeof bag, "%s", vm->last_str);
      have_bag = 1;
    }

    /* optional UNDER|AT|IN path, or bare string path if looks like nest not bag */
    if (kw(&L->cur,"UNDER") || kw(&L->cur,"AT") || kw(&L->cur,"IN") ||
        kw(&L->cur,"PREFIX") || kw(&L->cur,"ROOT")) {
      lex_next(L);
      have_under = 1;
      if (resolve_str_arg(vm, L, under, sizeof under) != 0)
        under[0] = 0;
    } else if (L->cur.kind == TK_STR) {
      /* bare string after bag = under path */
      if (resolve_str_arg(vm, L, under, sizeof under) == 0 && under[0])
        have_under = 1;
    }

    if (kw(&L->cur,"INTO") || kw(&L->cur,"TO") || kw(&L->cur,"AS")) {
      lex_next(L);
      if (L->cur.kind == TK_IDENT) {
        snprintf(into_name, sizeof into_name, "%s", L->cur.text);
        have_into = 1;
        lex_next(L);
      }
    }

    /* base plate: INTO name if exists, else PLATE */
    if (have_into) {
      pv = var_get(vm, into_name, 0);
      if (pv && pv->is_str && pv->sval[0])
        snprintf(plate, sizeof plate, "%s", pv->sval);
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
    if (cubalc_host_json_unflat_kv(plate, bag, have_under ? under : NULL, &hr) != 0) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "UNFLATKV_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "UNFLATKV: fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "UNFLATKV: fail");
      bump(vm); return 1;
    }

    if (have_into && into_name[0] && strcmp(into_name, "LAST") != 0)
      var_set_str(vm, into_name, hr.str);
    else
      var_set_str(vm, "PLATE", hr.str);

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "UNFLATKV", hr.str);
    var_set_str(vm, "FROMFLAT", hr.str);
    var_set_str(vm, "UNFLATTENP", hr.str);
    var_set_num(vm, "UNFLATKV_N", hr.n);
    var_set_num(vm, "FROMFLAT_N", hr.n);
    var_set_str(vm, "UNFLATKV_UNDER", under);
    var_set_num(vm, "UNFLATKV_INTO", have_into ? 1 : 0);
    var_set_str(vm, "UNFLATKV_DST",
                have_into && into_name[0] ? into_name : "PLATE");
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# unflatkv n=%ld under=%s into=%s\n",
              hr.n, under[0] ? under : ".",
              have_into ? into_name : "PLATE");
    bump(vm); return 1;
  }

  /* DIFFFLAT|CHANGELOGFLAT|LEAFDIFF|CLOGFLAT a b
   * PATHDIFF|DIFFPATH a b — deep leaf-path compare of two plates.
   * DIFFFLAT → "path: old → new" bag · PATHDIFF → changed path bag only.
   * Args: plate vars / LAST / JSON strings (same as CHANGELOGP).
   * Soft always OK. LAST_N = change count.
   * Usability: nest-aware mesh sync without shallow DIFFP/CHANGELOGP:
   *   DIFFFLAT PLATE PEER
   *   PATHDIFF session peer
   *   CHANGELOGFLAT a b
   */
  if (kw(&L->cur,"DIFFFLAT") || kw(&L->cur,"CHANGELOGFLAT") || kw(&L->cur,"LEAFDIFF") ||
      kw(&L->cur,"CLOGFLAT") || kw(&L->cur,"FLATDIFF") || kw(&L->cur,"MDIFFFLAT") ||
      kw(&L->cur,"PLATE_DIFFFLAT") || kw(&L->cur,"LEAFCHANGELOG") ||
      kw(&L->cur,"PATHDIFF") || kw(&L->cur,"DIFFPATH") || kw(&L->cur,"DIFFPATHS") ||
      kw(&L->cur,"CHANGEDPATHS") || kw(&L->cur,"MPATHDIFF") || kw(&L->cur,"LEAFPATHDIFF")) {
    char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
    char aname[96], bname[96];
    cubalc_host_result hr, paths;
    int paths_only = 0;
    Var *pv;
    char op0[28];

    snprintf(op0, sizeof op0, "%s", L->cur.text);
    for (char *q = op0; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    if (strcmp(op0, "PATHDIFF") == 0 || strcmp(op0, "DIFFPATH") == 0 ||
        strcmp(op0, "DIFFPATHS") == 0 || strcmp(op0, "CHANGEDPATHS") == 0 ||
        strcmp(op0, "MPATHDIFF") == 0 || strcmp(op0, "LEAFPATHDIFF") == 0)
      paths_only = 1;

    lex_next(L);
    a[0] = 0; b[0] = 0; aname[0] = 0; bname[0] = 0;

#define CUBALC_RESOLVE_PLATE_ARG_DF(dst, dname) do { \
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

    CUBALC_RESOLVE_PLATE_ARG_DF(a, aname);
    CUBALC_RESOLVE_PLATE_ARG_DF(b, bname);
#undef CUBALC_RESOLVE_PLATE_ARG_DF

    memset(&hr, 0, sizeof hr);
    cubalc_host_json_leaf_diff(a, b, paths_only, &hr);
    /* always also compute path bag for DIFFFLAT_PATHS */
    memset(&paths, 0, sizeof paths);
    if (paths_only) {
      snprintf(paths.str, sizeof paths.str, "%s", hr.str);
      paths.n = hr.n;
    } else {
      cubalc_host_json_leaf_diff(a, b, 1, &paths);
    }

    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "DIFFFLAT", hr.str);
    var_set_str(vm, "CHANGELOGFLAT", hr.str);
    var_set_str(vm, "LEAFDIFF", hr.str);
    var_set_num(vm, "DIFFFLAT_N", hr.n);
    var_set_num(vm, "CHANGELOGFLAT_N", hr.n);
    var_set_num(vm, "PATHDIFF_N", paths.n);
    var_set_str(vm, "DIFFFLAT_PATHS", paths.str);
    var_set_str(vm, "PATHDIFF", paths.str);
    var_set_str(vm, "DIFFFLAT_A", aname);
    var_set_str(vm, "DIFFFLAT_B", bname);
    var_set_num(vm, "DIFFFLAT_EQ", hr.n == 0 ? 1 : 0);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%ld a=%s b=%s\n",
              paths_only ? "pathdiff" : "diffflat",
              hr.n, aname[0] ? aname : "?", bname[0] ? bname : "?");
    bump(vm); return 1;
  }

  return 0;
}
