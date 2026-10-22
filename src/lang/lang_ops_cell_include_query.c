/* CubalC lang — lang_ops_cell_include_query.c
 * CELL include query: INCLUDESTEMS·HASINCLUDE·HASINCLUDEALL/NEEDINCLUDE (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_include_query.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

/* Stem/path match against vm->included[] (HASINCLUDE / NEEDINCLUDE). */
static int include_loaded_match(VM *vm, const char *want, char *hit_path, size_t hit_sz) {
  char stem[256];
  size_t wlen;
  const char *slash;
  int i;
  if (!vm || !want || !want[0]) return 0;
  slash = strrchr(want, '/');
  snprintf(stem, sizeof stem, "%s", slash ? slash + 1 : want);
  wlen = strlen(stem);
  if (wlen > 7 && strcmp(stem + wlen - 7, ".cubalc") == 0)
    stem[wlen - 7] = 0;
  for (i = 0; i < vm->n_included; i++) {
    const char *p = vm->included[i];
    const char *b = strrchr(p, '/');
    char pstem[256];
    size_t plen;
    b = b ? b + 1 : p;
    snprintf(pstem, sizeof pstem, "%s", b);
    plen = strlen(pstem);
    if (plen > 7 && strcmp(pstem + plen - 7, ".cubalc") == 0)
      pstem[plen - 7] = 0;
    if (strcmp(p, want) == 0 || strcmp(b, want) == 0 ||
        strcmp(pstem, want) == 0 || strcmp(pstem, stem) == 0 ||
        strcmp(b, stem) == 0 ||
        (stem[0] && strstr(p, stem))) {
      if (hit_path && hit_sz)
        snprintf(hit_path, hit_sz, "%s", p);
      return 1;
    }
  }
  if (hit_path && hit_sz) hit_path[0] = 0;
  return 0;
}

/* Parse one name|path token for INCLUDE probes (IDENT/STR/string-var/LAST). */
static int include_parse_want(VM *vm, Lex *L, char *want, size_t want_sz) {
  if (L->cur.kind == TK_STR) {
    snprintf(want, want_sz, "%s", L->cur.text);
    lex_next(L);
    return 1;
  }
  if (L->cur.kind == TK_IDENT) {
    Var *vv = var_get(vm, L->cur.text, 0);
    if (vv && vv->is_str && vv->sval[0])
      snprintf(want, want_sz, "%s", vv->sval);
    else if (strcmp(L->cur.text, "LAST") == 0)
      snprintf(want, want_sz, "%s", vm->last_str);
    else
      snprintf(want, want_sz, "%s", L->cur.text);
    lex_next(L);
    return 1;
  }
  return 0;
}


int cubalc_lang_ops_cell_try_include_query(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* INCLUDESTEMS / LISTINCLUDESTEMS — basenames without .cubalc from loaded modules.
   * Usability: short-name bag for HASLINE/NEEDINCLUDE glue after -I without BASENAMEALL. */
  if (kw(&L->cur, "INCLUDESTEMS") || kw(&L->cur, "LISTINCLUDESTEMS") ||
      kw(&L->cur, "INCLUDE_NAMES") || kw(&L->cur, "LOADEDSTEMS") ||
      kw(&L->cur, "INCLUDEBASENAMES") || kw(&L->cur, "STEMSINCLUDES")) {
    char bag[4096];
    size_t o = 0;
    int i, n = 0;
    lex_next(L);
    bag[0] = 0;
    for (i = 0; i < vm->n_included; i++) {
      const char *p = vm->included[i];
      const char *b = strrchr(p, '/');
      char stem[256];
      size_t sl;
      b = b ? b + 1 : p;
      snprintf(stem, sizeof stem, "%s", b);
      sl = strlen(stem);
      if (sl > 7 && strcmp(stem + sl - 7, ".cubalc") == 0)
        stem[sl - 7] = 0, sl -= 7;
      if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + sl < sizeof bag) {
        memcpy(bag + o, stem, sl);
        o += sl;
      }
      bag[o] = 0;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "INCLUDESTEMS", bag);
    var_set_str(vm, "LOADEDSTEMS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "INCLUDE_N", n);
    var_set_num(vm, "INCLUDESTEMS_N", n);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASINCLUDE name|path — soft 0|1 if a loaded INCLUDE matches stem or path.
   * Complements LISTINCLUDES · IF without bag GREP after -I preload. */
  if (kw(&L->cur, "HASINCLUDE") || kw(&L->cur, "HAVEINCLUDE") ||
      kw(&L->cur, "HASLOADED") || kw(&L->cur, "ISINCLUDED") ||
      kw(&L->cur, "INCLUDELOADED")) {
    char want[256], hitp[256];
    int hit;
    lex_next(L);
    if (kw(&L->cur, "LIB") || kw(&L->cur, "OF") || kw(&L->cur, "MODULE"))
      lex_next(L);
    if (!include_parse_want(vm, L, want, sizeof want)) {
      fail(vm, "HASINCLUDE name|path");
      return -1;
    }
    hit = include_loaded_match(vm, want, hitp, sizeof hitp);
    if (hit)
      var_set_str(vm, "INCLUDE_PATH", hitp);
    else
      var_set_str(vm, "INCLUDE_PATH", "");
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    {
      char nb[8];
      snprintf(nb, sizeof nb, "%d", hit);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "HASINCLUDE_N", hit);
    var_set_num(vm, "INCLUDE_N", vm->n_included);
    var_set_str(vm, "INCLUDE_MISS", hit ? "" : want);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASINCLUDEALL a b c — soft 0|1 all named modules loaded · miss bag.
   * NEEDINCLUDE / REQUIRE LOADED a b — fail-fast if any not loaded this run.
   * Usability: agent boot contracts after -I / INCLUDE without multi HASINCLUDE. */
  if (kw(&L->cur, "HASINCLUDEALL") || kw(&L->cur, "HASINCLUDES") ||
      kw(&L->cur, "ALLINCLUDED") || kw(&L->cur, "NEEDINCLUDE") ||
      kw(&L->cur, "NEEDINCLUDES") || kw(&L->cur, "REQUIRELOADED") ||
      kw(&L->cur, "NEEDLOADED") || kw(&L->cur, "REQUIRE_INCLUDED")) {
    int hard = kw(&L->cur, "NEEDINCLUDE") || kw(&L->cur, "NEEDINCLUDES") ||
               kw(&L->cur, "REQUIRELOADED") || kw(&L->cur, "NEEDLOADED") ||
               kw(&L->cur, "REQUIRE_INCLUDED");
    int aln = L->cur.line;
    char miss[512];
    size_t mo = 0;
    int n_need = 0, n_hit = 0, n_miss = 0;
    char last_hit[256];
    last_hit[0] = 0;
    miss[0] = 0;
    lex_next(L);
    if (kw(&L->cur, "LIB") || kw(&L->cur, "OF") || kw(&L->cur, "MODULE") ||
        kw(&L->cur, "LOADED") || kw(&L->cur, "INCLUDE") || kw(&L->cur, "INCLUDES"))
      lex_next(L);
    while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      char want[256], hitp[256];
      /* stop words that start next form */
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur, "ASSERT") || kw(&L->cur, "EXPECT") ||
           kw(&L->cur, "IF") || kw(&L->cur, "LET") ||
           kw(&L->cur, "INCLUDE") || kw(&L->cur, "PASS") ||
           kw(&L->cur, "FAIL") || kw(&L->cur, "PRINT") ||
           kw(&L->cur, "SYS") || kw(&L->cur, "LISTINCLUDES") ||
           kw(&L->cur, "HASINCLUDE") || kw(&L->cur, "NEEDINCLUDE") ||
           kw(&L->cur, "HASINCLUDEALL") || kw(&L->cur, "REQUIRE") ||
           kw(&L->cur, "END") || kw(&L->cur, "ELSE") ||
           kw(&L->cur, "WHILE") || kw(&L->cur, "FOR") ||
           kw(&L->cur, "FN") || kw(&L->cur, "CLASS") ||
           kw(&L->cur, "HOLD_FLASH") || kw(&L->cur, "VERSION") ||
           kw(&L->cur, "STATUS") || kw(&L->cur, "WHY") ||
           kw(&L->cur, "NOTE") || kw(&L->cur, "EXIT") ||
           kw(&L->cur, "CLEAR_ERR") || kw(&L->cur, "DEFAULT")))
        break;
      if (!include_parse_want(vm, L, want, sizeof want)) break;
      if (!want[0]) continue;
      n_need++;
      if (include_loaded_match(vm, want, hitp, sizeof hitp)) {
        n_hit++;
        snprintf(last_hit, sizeof last_hit, "%s", hitp);
      } else {
        size_t wl = strlen(want);
        if (n_miss > 0 && mo + 1 < sizeof miss) miss[mo++] = '\n';
        if (mo + wl < sizeof miss) {
          memcpy(miss + mo, want, wl);
          mo += wl;
        }
        miss[mo] = 0;
        n_miss++;
      }
      /* optional commas between names */
      if (L->cur.kind == TK_COMMA) lex_next(L);
    }
    if (n_need == 0) {
      fail_at(vm, L, hard ? "NEEDINCLUDE name…" : "HASINCLUDEALL name…");
      return -1;
    }
    var_set_num(vm, "INCLUDE_N", vm->n_included);
    var_set_num(vm, "INCLUDE_NEED_N", n_need);
    var_set_num(vm, "INCLUDE_HIT_N", n_hit);
    var_set_num(vm, "INCLUDE_MISS_N", n_miss);
    var_set_str(vm, "INCLUDE_MISS", miss);
    if (n_miss == 0) {
      var_set_str(vm, "INCLUDE_PATH", last_hit);
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST", last_hit[0] ? last_hit : "1");
      snprintf(vm->last_str, sizeof vm->last_str, "%s",
               last_hit[0] ? last_hit : "1");
      var_set_num(vm, "OK", 1);
      var_set_num(vm, "HASINCLUDEALL_N", 1);
      if (vm->res) vm->res->asserts_ok++;
      if (vm->trace)
        fprintf(vm->trace, "# %s hit need=%d\n",
                hard ? "needinclude" : "hasincludeall", n_need);
      bump(vm);
      return 1;
    }
    /* soft miss */
    if (!hard) {
      var_set_str(vm, "INCLUDE_PATH", "");
      var_set_str(vm, "LAST", miss);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", miss);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASINCLUDEALL_N", 0);
      var_set_num(vm, "OK", 0);
      {
        char em[192];
        snprintf(em, sizeof em,
                 "HASINCLUDEALL miss line %d: %s — INCLUDE / cubalc run -I",
                 aln, miss);
        var_set_str(vm, "ERR", em);
        var_set_str(vm, "LAST_ERR", em);
      }
      bump(vm);
      return 1;
    }
    /* hard fail */
    {
      char msg[240];
      char flat[160];
      size_t fi = 0;
      const char *mp;
      flat[0] = 0;
      for (mp = miss; *mp && fi + 1 < sizeof flat; mp++) {
        if (*mp == '\n') {
          if (fi > 0 && flat[fi - 1] != ',') {
            flat[fi++] = ',';
            if (fi + 1 < sizeof flat) flat[fi++] = ' ';
          }
        } else {
          flat[fi++] = *mp;
        }
      }
      flat[fi] = 0;
      snprintf(msg, sizeof msg,
               "NEEDINCLUDE missing line %d: %s — INCLUDE / cubalc run -I · HASINCLUDE",
               aln, flat[0] ? flat : "?");
      if (vm->res) vm->res->asserts_fail++;
      fail(vm, msg);
      return -1;
    }
  }

  return 0;
}
