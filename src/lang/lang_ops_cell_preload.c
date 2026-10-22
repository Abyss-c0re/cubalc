/* CubalC lang — lang_ops_cell_preload.c
 * CELL preload query: LISTPRELOAD·PRELOADMISS·HASPRELOAD (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_preload.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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



int cubalc_lang_ops_cell_try_preload(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  /* LISTPRELOAD / PRELOADS — short-name bag of effective -I / CUBALC_PRELOAD.
   * Usability: CLI sets CUBALC_PRELOAD_ACTIVE; programs audit request vs INCLUDESTEMS. */
  if (kw(&L->cur, "LISTPRELOAD") || kw(&L->cur, "PRELOADS") ||
      kw(&L->cur, "PRELOADLIST") || kw(&L->cur, "LIST_PRELOAD") ||
      kw(&L->cur, "PRELOAD_NAMES") || kw(&L->cur, "PRELOADED")) {
    const char *env = getenv("CUBALC_PRELOAD_ACTIVE");
    char bag[1024];
    size_t o = 0;
    int n = 0;
    lex_next(L);
    if (!env || !env[0])
      env = getenv("CUBALC_PRELOAD");
    bag[0] = 0;
    if (env && env[0]) {
      const char *p = env;
      while (*p) {
        char name[96];
        size_t len = 0;
        while (*p == ':' || *p == ',' || *p == ' ' || *p == '\t') p++;
        if (!*p) break;
        while (p[len] && p[len] != ':' && p[len] != ',' && p[len] != ' ' &&
               p[len] != '\t' && len + 1 < sizeof name) {
          name[len] = p[len];
          len++;
        }
        name[len] = 0;
        p += len;
        if (!name[0]) continue;
        if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + len < sizeof bag) {
          memcpy(bag + o, name, len);
          o += len;
        }
        bag[o] = 0;
        n++;
      }
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LISTPRELOAD", bag);
    var_set_str(vm, "PRELOADS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "PRELOAD_N", n);
    var_set_num(vm, "LISTPRELOAD_N", n);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* PRELOADMISS / PRELOADOK / NEEDPRELOAD — audit CLI -I/CUBALC_PRELOAD vs loaded.
   * Usability: request bag from CUBALC_PRELOAD_ACTIVE; miss = not in include_loaded.
   * Soft PRELOADMISS → LAST=miss bag LAST_N=count; PRELOADOK → 0|1; NEEDPRELOAD hard. */
  if (kw(&L->cur, "PRELOADMISS") || kw(&L->cur, "MISSINGPRELOAD") ||
      kw(&L->cur, "PRELOAD_MISS") || kw(&L->cur, "CHECKPRELOAD") ||
      kw(&L->cur, "PRELOADOK") || kw(&L->cur, "HASPRELOADALL") ||
      kw(&L->cur, "PRELOAD_OK") || kw(&L->cur, "ALLPRELOADED") ||
      kw(&L->cur, "NEEDPRELOAD") || kw(&L->cur, "REQUIREPRELOAD") ||
      kw(&L->cur, "NEED_PRELOAD") || kw(&L->cur, "REQUIRE_PRELOAD")) {
    int hard = kw(&L->cur, "NEEDPRELOAD") || kw(&L->cur, "REQUIREPRELOAD") ||
               kw(&L->cur, "NEED_PRELOAD") || kw(&L->cur, "REQUIRE_PRELOAD");
    int as_ok = kw(&L->cur, "PRELOADOK") || kw(&L->cur, "HASPRELOADALL") ||
                kw(&L->cur, "PRELOAD_OK") || kw(&L->cur, "ALLPRELOADED");
    int aln = L->cur.line;
    const char *env = getenv("CUBALC_PRELOAD_ACTIVE");
    char miss[512];
    size_t mo = 0;
    int n_need = 0, n_hit = 0, n_miss = 0;
    miss[0] = 0;
    lex_next(L);
    if (!env || !env[0])
      env = getenv("CUBALC_PRELOAD");
    if (env && env[0]) {
      const char *p = env;
      while (*p) {
        char name[96], hitp[256];
        size_t len = 0;
        while (*p == ':' || *p == ',' || *p == ' ' || *p == '\t') p++;
        if (!*p) break;
        while (p[len] && p[len] != ':' && p[len] != ',' && p[len] != ' ' &&
               p[len] != '\t' && len + 1 < sizeof name) {
          name[len] = p[len];
          len++;
        }
        name[len] = 0;
        p += len;
        if (!name[0]) continue;
        n_need++;
        if (include_loaded_match(vm, name, hitp, sizeof hitp)) {
          n_hit++;
        } else {
          if (n_miss > 0 && mo + 1 < sizeof miss) miss[mo++] = '\n';
          if (mo + len < sizeof miss) {
            memcpy(miss + mo, name, len);
            mo += len;
          }
          miss[mo] = 0;
          n_miss++;
        }
      }
    }
    var_set_num(vm, "PRELOAD_N", n_need);
    var_set_num(vm, "PRELOAD_HIT_N", n_hit);
    var_set_num(vm, "PRELOAD_MISS_N", n_miss);
    var_set_str(vm, "PRELOAD_MISS", miss);
    var_set_num(vm, "INCLUDE_N", vm->n_included);

    if (hard) {
      if (n_miss == 0) {
        var_set_num(vm, "LAST_N", 1);
        vm->last_n = 1;
        var_set_str(vm, "LAST", "1");
        snprintf(vm->last_str, sizeof vm->last_str, "1");
        var_set_num(vm, "PRELOAD_OK_N", 1);
        var_set_num(vm, "OK", 1);
        if (vm->res) vm->res->asserts_ok++;
        if (vm->trace)
          fprintf(vm->trace, "# needpreload ok need=%d\n", n_need);
        bump(vm);
        return 1;
      }
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
                 "NEEDPRELOAD missing line %d: %s — cubalc run -I name · "
                 "INCLUDE / LISTPRELOAD vs INCLUDESTEMS",
                 aln, flat[0] ? flat : "?");
        var_set_str(vm, "ERR", msg);
        var_set_str(vm, "LAST_ERR", msg);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
    }

    /* Soft: PRELOADOK → 0|1; PRELOADMISS/CHECKPRELOAD → miss bag + count */
    if (as_ok) {
      int ok = (n_miss == 0);
      var_set_num(vm, "LAST_N", ok);
      vm->last_n = ok;
      if (ok) {
        var_set_str(vm, "LAST", "1");
        snprintf(vm->last_str, sizeof vm->last_str, "1");
        var_set_num(vm, "OK", 1);
      } else {
        var_set_str(vm, "LAST", miss);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", miss);
        var_set_num(vm, "OK", 0);
        {
          char em[192];
          snprintf(em, sizeof em,
                   "PRELOADOK miss line %d: %s — cubalc run -I / INCLUDE",
                   aln, miss);
          var_set_str(vm, "ERR", em);
          var_set_str(vm, "LAST_ERR", em);
        }
      }
      var_set_num(vm, "PRELOAD_OK_N", ok);
      bump(vm);
      return 1;
    }

    /* PRELOADMISS / CHECKPRELOAD — always OK=1 audit; LAST_N = miss count */
    var_set_str(vm, "LAST", miss);
    var_set_str(vm, "PRELOADMISS", miss);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", miss);
    vm->last_n = n_miss;
    var_set_num(vm, "LAST_N", n_miss);
    var_set_num(vm, "PRELOAD_OK_N", n_miss == 0 ? 1 : 0);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASPRELOAD name — soft 0|1 if name is in -I / CUBALC_PRELOAD request bag.
   * Dual of HASINCLUDE (loaded) · IF branch on launch contract without LISTPRELOAD+HASLINE. */
  if (kw(&L->cur, "HASPRELOAD") || kw(&L->cur, "HAVEPRELOAD") ||
      kw(&L->cur, "ISPRELOAD") || kw(&L->cur, "PRELOADHAS") ||
      kw(&L->cur, "INPRELOAD")) {
    char want[256], wstem[96];
    const char *env;
    int hit = 0, n_req = 0;
    size_t wlen;
    lex_next(L);
    if (kw(&L->cur, "LIB") || kw(&L->cur, "OF") || kw(&L->cur, "MODULE") ||
        kw(&L->cur, "NAME"))
      lex_next(L);
    if (!include_parse_want(vm, L, want, sizeof want)) {
      fail(vm, "HASPRELOAD name");
      return -1;
    }
    {
      const char *slash = strrchr(want, '/');
      snprintf(wstem, sizeof wstem, "%s", slash ? slash + 1 : want);
    }
    wlen = strlen(wstem);
    if (wlen > 7 && strcmp(wstem + wlen - 7, ".cubalc") == 0)
      wstem[wlen - 7] = 0;
    env = getenv("CUBALC_PRELOAD_ACTIVE");
    if (!env || !env[0])
      env = getenv("CUBALC_PRELOAD");
    if (env && env[0] && wstem[0]) {
      const char *p = env;
      while (*p) {
        char name[96], nstem[96];
        size_t len = 0, nlen;
        const char *slash;
        while (*p == ':' || *p == ',' || *p == ' ' || *p == '\t') p++;
        if (!*p) break;
        while (p[len] && p[len] != ':' && p[len] != ',' && p[len] != ' ' &&
               p[len] != '\t' && len + 1 < sizeof name) {
          name[len] = p[len];
          len++;
        }
        name[len] = 0;
        p += len;
        if (!name[0]) continue;
        n_req++;
        slash = strrchr(name, '/');
        snprintf(nstem, sizeof nstem, "%s", slash ? slash + 1 : name);
        nlen = strlen(nstem);
        if (nlen > 7 && strcmp(nstem + nlen - 7, ".cubalc") == 0)
          nstem[nlen - 7] = 0;
        if (strcmp(nstem, wstem) == 0 || strcmp(name, want) == 0 ||
            strcmp(nstem, want) == 0) {
          hit = 1;
          break;
        }
      }
    }
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    {
      char nb[8];
      snprintf(nb, sizeof nb, "%d", hit);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "HASPRELOAD_N", hit);
    var_set_num(vm, "PRELOAD_N", n_req);
    var_set_str(vm, "PRELOAD_WANT", want);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  return 0;
}
