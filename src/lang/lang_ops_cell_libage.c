/* CubalC lang — lang_ops_cell_libage.c
 * CELL lib age: FRESHLIBS/STALELIBS·LIBAGE·HASFRESH·NEEDFRESH (G_cell_libage).
 * Peel from lang_ops_cell.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_cell_libage.h"
#include "lang/cubalc_lang_internal.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/* Resolve short lib name → path (INCLUDE search order). Returns 1 if found. */
static int lib_resolve_path(const char *name, char *path, size_t pathsz,
                            char *base_out, size_t basesz) {
  char base[160];
  const char *slash, *leaf, *ip, *root;
  size_t blen;
  FILE *f = NULL;
  if (!name || !name[0] || !path || pathsz < 8) return 0;
  path[0] = 0;
  slash = strrchr(name, '/');
  leaf = slash ? slash + 1 : name;
  snprintf(base, sizeof base, "%s", leaf);
  blen = strlen(base);
  if (blen > 7 && strcmp(base + blen - 7, ".cubalc") == 0)
    base[blen - 7] = 0;
  if (base_out && basesz)
    snprintf(base_out, basesz, "%s", base);
  if (name[0] == '/' || strchr(name, '/')) {
    f = fopen(name, "rb");
    if (f) { snprintf(path, pathsz, "%s", name); fclose(f); return 1; }
  }
  {
    char p3[768];
    snprintf(p3, sizeof p3, "programs/lib/%s.cubalc", base);
    f = fopen(p3, "rb");
    if (f) { snprintf(path, pathsz, "%s", p3); fclose(f); return 1; }
    snprintf(p3, sizeof p3, "programs/lib/%s", name);
    f = fopen(p3, "rb");
    if (f) { snprintf(path, pathsz, "%s", p3); fclose(f); return 1; }
  }
  f = fopen(name, "rb");
  if (f) { snprintf(path, pathsz, "%s", name); fclose(f); return 1; }
  root = getenv("CUBALC_ROOT");
  if (root && root[0]) {
    char p2[768];
    snprintf(p2, sizeof p2, "%s/programs/lib/%s.cubalc", root, base);
    f = fopen(p2, "rb");
    if (f) { snprintf(path, pathsz, "%s", p2); fclose(f); return 1; }
  }
  ip = getenv("CUBALC_INCLUDE_PATH");
  if (ip && ip[0]) {
    const char *seg = ip;
    while (*seg) {
      char dir[512], p3[768];
      size_t dlen = 0;
      while (*seg == ':') seg++;
      if (!*seg) break;
      while (seg[dlen] && seg[dlen] != ':' && dlen + 1 < sizeof dir)
        dir[dlen] = seg[dlen], dlen++;
      dir[dlen] = 0;
      seg += dlen;
      if (!dir[0]) continue;
      snprintf(p3, sizeof p3, "%s/%s.cubalc", dir, base);
      f = fopen(p3, "rb");
      if (f) { snprintf(path, pathsz, "%s", p3); fclose(f); return 1; }
      snprintf(p3, sizeof p3, "%s/%s", dir, name);
      f = fopen(p3, "rb");
      if (f) { snprintf(path, pathsz, "%s", p3); fclose(f); return 1; }
    }
  }
  return 0;
}

int cubalc_lang_ops_cell_try_libage(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* FRESHLIBS|STALELIBS needle age_sec — age-filter matching stems (mtime vs now).
   * Usability: keep recent / drop stale recipes without shell find -mtime or EACH+AGE.
   * FRESHLIBS: age <= max_sec (default 86400) · STALELIBS: age >= min_sec (default 86400).
   * Bag sorted newest-first · LAST_N=count · FRESHLIBS_N · soft empty OK=1. */
  if (kw(&L->cur, "FRESHLIBS") || kw(&L->cur, "RECENTLIBS") ||
      kw(&L->cur, "KEEPFRESHLIBS") || kw(&L->cur, "FRESH_LIBS") ||
      kw(&L->cur, "STALELIBS") || kw(&L->cur, "OLDERLIBS") ||
      kw(&L->cur, "KEEPSTALELIBS") || kw(&L->cur, "STALE_LIBS") ||
      kw(&L->cur, "AGEDLIBS") || kw(&L->cur, "OLD_LIBS")) {
    char needle[96], fup[96], bag[4096];
    char stems[96][96];
    char stem_paths[96][768];
    long mtimes[96];
    long ages[96];
    int nstem = 0, i, j, want_stale = 0;
    long age_lim = 86400; /* 1 day default */
    int have_age = 0;
    size_t o = 0, a;
    time_t now = time(NULL);
    DIR *d;
    struct dirent *ent;
    const char *ip;
    if (kw(&L->cur, "STALELIBS") || kw(&L->cur, "OLDERLIBS") ||
        kw(&L->cur, "KEEPSTALELIBS") || kw(&L->cur, "STALE_LIBS") ||
        kw(&L->cur, "AGEDLIBS") || kw(&L->cur, "OLD_LIBS"))
      want_stale = 1;
    lex_next(L);
    needle[0] = 0;
    bag[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(needle, sizeof needle, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(needle, sizeof needle, "%s", vm->last_str);
      else
        snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    }
    /* age threshold: number or num var · optional SEC/S/SECONDS suffix keyword skipped */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      age_lim = (long)parse_expr(vm, L);
      have_age = 1;
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && !vv->is_str) {
        age_lim = (long)vv->val;
        have_age = 1;
        lex_next(L);
      }
    }
    if (kw(&L->cur, "SEC") || kw(&L->cur, "SECS") || kw(&L->cur, "SECONDS") ||
        kw(&L->cur, "S") || kw(&L->cur, "AGE"))
      lex_next(L);
    (void)have_age;
    if (age_lim < 0) age_lim = 0;
    if (!needle[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "FRESHLIBS_N", 0);
      var_set_num(vm, "STALELIBS_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  "FRESHLIBS: need needle — FRESHLIBS fat 3600 · STALELIBS plate 86400");
      var_set_str(vm, "ERR",
                  "FRESHLIBS: need needle — FRESHLIBS fat 3600 · STALELIBS plate 86400");
      bump(vm);
      return 1;
    }
    for (a = 0; needle[a] && a + 1 < sizeof fup; a++) {
      char c = needle[a];
      if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
      fup[a] = c;
    }
    fup[a] = 0;
    {
      char dirs[10][160];
      int nd = 0, di;
      snprintf(dirs[nd++], sizeof dirs[0], "%s", "programs/lib");
      ip = getenv("CUBALC_INCLUDE_PATH");
      if (ip && ip[0]) {
        const char *p = ip;
        while (*p && nd < 10) {
          char dir[160];
          size_t len = 0;
          while (*p == ':' || *p == ' ' || *p == '\t') p++;
          if (!*p) break;
          while (p[len] && p[len] != ':' && len + 1 < sizeof dir) {
            dir[len] = p[len];
            len++;
          }
          dir[len] = 0;
          p += len;
          if (dir[0])
            snprintf(dirs[nd++], sizeof dirs[0], "%s", dir);
        }
      }
      for (di = 0; di < nd; di++) {
        d = opendir(dirs[di]);
        if (!d) continue;
        while ((ent = readdir(d)) != NULL && nstem < 96) {
          size_t len = strlen(ent->d_name);
          char stem[96], hay[192], full[768];
          int dup = 0;
          size_t b;
          struct stat st;
          long age;
          if (len < 8 || strcmp(ent->d_name + len - 7, ".cubalc") != 0)
            continue;
          if (ent->d_name[0] == '.') continue;
          if (len - 7 >= sizeof stem) continue;
          memcpy(stem, ent->d_name, len - 7);
          stem[len - 7] = 0;
          for (j = 0; j < nstem; j++) {
            if (strcmp(stems[j], stem) == 0) { dup = 1; break; }
          }
          if (dup) continue;
          snprintf(full, sizeof full, "%s/%s", dirs[di], ent->d_name);
          snprintf(hay, sizeof hay, "%s", full);
          for (b = 0; hay[b]; b++)
            if (hay[b] >= 'A' && hay[b] <= 'Z')
              hay[b] = (char)(hay[b] - 'A' + 'a');
          {
            char stem_l[96];
            for (b = 0; stem[b] && b + 1 < sizeof stem_l; b++) {
              char c = stem[b];
              if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
              stem_l[b] = c;
            }
            stem_l[b] = 0;
            if (!strstr(stem_l, fup) && !strstr(hay, fup))
              continue;
          }
          if (stat(full, &st) != 0) continue;
          age = (long)now - (long)st.st_mtime;
          if (age < 0) age = 0;
          if (want_stale) {
            if (age < age_lim) continue;
          } else {
            if (age > age_lim) continue;
          }
          snprintf(stems[nstem], sizeof stems[0], "%s", stem);
          snprintf(stem_paths[nstem], sizeof stem_paths[0], "%s", full);
          mtimes[nstem] = (long)st.st_mtime;
          ages[nstem] = age;
          nstem++;
        }
        closedir(d);
      }
    }
    /* newest-first (mtime desc) · alpha ties */
    for (i = 1; i < nstem; i++) {
      char tmp[96], tp[768];
      long tm, ag;
      snprintf(tmp, sizeof tmp, "%s", stems[i]);
      snprintf(tp, sizeof tp, "%s", stem_paths[i]);
      tm = mtimes[i];
      ag = ages[i];
      j = i;
      while (j > 0) {
        int before = 0;
        if (mtimes[j - 1] < tm ||
            (mtimes[j - 1] == tm && strcmp(stems[j - 1], tmp) > 0))
          before = 1;
        if (!before) break;
        snprintf(stems[j], sizeof stems[0], "%s", stems[j - 1]);
        snprintf(stem_paths[j], sizeof stem_paths[0], "%s", stem_paths[j - 1]);
        mtimes[j] = mtimes[j - 1];
        ages[j] = ages[j - 1];
        j--;
      }
      snprintf(stems[j], sizeof stems[0], "%s", tmp);
      snprintf(stem_paths[j], sizeof stem_paths[0], "%s", tp);
      mtimes[j] = tm;
      ages[j] = ag;
    }
    o = 0;
    for (i = 0; i < nstem; i++) {
      size_t ln = strlen(stems[i]);
      if (i > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, stems[i], ln);
        o += ln;
      }
      bag[o] = 0;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, want_stale ? "STALELIBS" : "FRESHLIBS", bag);
    var_set_str(vm, "MATCHLIBS", bag);
    var_set_str(vm, "FRESHLIBS_FILTER", needle);
    var_set_str(vm, "STALELIBS_FILTER", needle);
    var_set_str(vm, "MATCHLIBS_FILTER", needle);
    var_set_str(vm, "FRESHLIBS_MODE", want_stale ? "stale" : "fresh");
    var_set_num(vm, "FRESHLIBS_AGE", age_lim);
    var_set_num(vm, "STALELIBS_AGE", age_lim);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = nstem;
    var_set_num(vm, "LAST_N", nstem);
    var_set_num(vm, "FRESHLIBS_N", nstem);
    var_set_num(vm, "STALELIBS_N", nstem);
    var_set_num(vm, "MATCHLIBS_N", nstem);
    if (nstem > 0) {
      var_set_str(vm, "LIB_PATH", stem_paths[0]);
      var_set_num(vm, "LIB_MTIME", mtimes[0]);
      var_set_num(vm, "LIB_AGE", ages[0]);
      var_set_num(vm, "FRESHLIBS_MTIME", mtimes[0]);
      var_set_str(vm, "FRESHLIBS_HEAD", stems[0]);
      var_set_str(vm, "STALELIBS_HEAD", stems[0]);
    } else {
      var_set_str(vm, "LIB_PATH", "");
      var_set_num(vm, "LIB_MTIME", 0);
      var_set_num(vm, "LIB_AGE", 0);
      var_set_num(vm, "FRESHLIBS_MTIME", 0);
      var_set_str(vm, "FRESHLIBS_HEAD", "");
      var_set_str(vm, "STALELIBS_HEAD", "");
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LIBAGE name · HASFRESH name [max_age] · NEEDFRESH name [max_age]
   * Usability: single-lib freshness contract before INCLUDE — age seconds + soft/hard
   * gates without LIBINFO JSON peel or FRESHLIBS bag. Default max_age 86400.
   * LIBAGE → LAST_N=age · HASFRESH → LAST_N 0|1 · NEEDFRESH fail if missing/stale. */
  if (kw(&L->cur, "LIBAGE") || kw(&L->cur, "AGEOFLIB") ||
      kw(&L->cur, "LIB_AGE") || kw(&L->cur, "MTIMEAGE") ||
      kw(&L->cur, "AGE_LIB") || kw(&L->cur, "STATLIBAGE") ||
      kw(&L->cur, "HASFRESH") || kw(&L->cur, "ISFRESH") ||
      kw(&L->cur, "LIBFRESH") || kw(&L->cur, "HAS_FRESH") ||
      kw(&L->cur, "NEEDFRESH") || kw(&L->cur, "REQUIREFRESH") ||
      kw(&L->cur, "NEED_FRESH") || kw(&L->cur, "REQUIRE_FRESH")) {
    int mode_age = 0, mode_has = 0, mode_need = 0;
    char name[160], path[768], base[160];
    long max_age = 86400, age = -1, mtime = 0;
    int have_max = 0, aln = L->cur.line;
    struct stat st;
    time_t now = time(NULL);
    if (kw(&L->cur, "LIBAGE") || kw(&L->cur, "AGEOFLIB") ||
        kw(&L->cur, "LIB_AGE") || kw(&L->cur, "MTIMEAGE") ||
        kw(&L->cur, "AGE_LIB") || kw(&L->cur, "STATLIBAGE"))
      mode_age = 1;
    else if (kw(&L->cur, "NEEDFRESH") || kw(&L->cur, "REQUIREFRESH") ||
             kw(&L->cur, "NEED_FRESH") || kw(&L->cur, "REQUIRE_FRESH"))
      mode_need = 1;
    else
      mode_has = 1;
    lex_next(L);
    name[0] = 0;
    path[0] = 0;
    base[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(name, sizeof name, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(name, sizeof name, "%s", vm->last_str);
      else
        snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
    }
    if (!mode_age) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        max_age = (long)parse_expr(vm, L);
        have_max = 1;
      } else if (L->cur.kind == TK_IDENT) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && !vv->is_str) {
          max_age = (long)vv->val;
          have_max = 1;
          lex_next(L);
        }
      }
      if (kw(&L->cur, "SEC") || kw(&L->cur, "SECS") || kw(&L->cur, "SECONDS") ||
          kw(&L->cur, "S") || kw(&L->cur, "AGE"))
        lex_next(L);
      (void)have_max;
      if (max_age < 0) max_age = 0;
    }
    if (!name[0]) {
      if (mode_need) {
        fail_at(vm, L, "NEEDFRESH needs name — NEEDFRESH fat_session 86400");
        return -1;
      }
      var_set_str(vm, "LAST", mode_age ? "" : "0");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", mode_age ? "" : "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "LIB_AGE", -1);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  mode_age ? "LIBAGE: need name — LIBAGE fat_session"
                           : "HASFRESH: need name — HASFRESH fat_session 86400");
      var_set_str(vm, "ERR",
                  mode_age ? "LIBAGE: need name — LIBAGE fat_session"
                           : "HASFRESH: need name — HASFRESH fat_session 86400");
      bump(vm);
      return 1;
    }
    if (!lib_resolve_path(name, path, sizeof path, base, sizeof base) ||
        stat(path, &st) != 0) {
      char em[192];
      snprintf(em, sizeof em,
               "%s miss: '%s' — cubalc which · HASLIB · LIBAGE · install lib",
               mode_need ? "NEEDFRESH" : (mode_age ? "LIBAGE" : "HASFRESH"),
               name);
      if (mode_need) {
        fail_at(vm, L, em);
        return -1;
      }
      var_set_str(vm, "LAST", mode_age ? "" : "0");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", mode_age ? "" : "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "LIB_AGE", -1);
      var_set_num(vm, "LIB_MTIME", 0);
      var_set_str(vm, "LIB_PATH", "");
      var_set_str(vm, "LIB_STEM", base[0] ? base : name);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
    mtime = (long)st.st_mtime;
    age = (long)now - mtime;
    if (age < 0) age = 0;
    var_set_str(vm, "LIB_PATH", path);
    var_set_str(vm, "LIB_STEM", base);
    var_set_num(vm, "LIB_MTIME", mtime);
    var_set_num(vm, "LIB_AGE", age);
    var_set_num(vm, "HASFRESH_AGE", max_age);
    var_set_num(vm, "NEEDFRESH_AGE", max_age);
    if (mode_age) {
      char abuf[32];
      snprintf(abuf, sizeof abuf, "%ld", age);
      var_set_str(vm, "LAST", abuf);
      var_set_str(vm, "LIBAGE", abuf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", abuf);
      vm->last_n = age;
      var_set_num(vm, "LAST_N", age);
      var_set_num(vm, "OK", 1);
      bump(vm);
      return 1;
    }
    {
      int fresh = (age <= max_age) ? 1 : 0;
      char em[200];
      var_set_num(vm, "LAST_N", fresh);
      vm->last_n = fresh;
      {
        char nb[8];
        snprintf(nb, sizeof nb, "%d", fresh);
        var_set_str(vm, "LAST", nb);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
      }
      var_set_num(vm, "HASFRESH_N", fresh);
      if (fresh) {
        var_set_num(vm, "OK", 1);
        bump(vm);
        return 1;
      }
      snprintf(em, sizeof em,
               "%s stale: '%s' age=%ld > %ld — FRESHLIBS · touch/update lib · LIBAGE",
               mode_need ? "NEEDFRESH" : "HASFRESH", base[0] ? base : name,
               age, max_age);
      if (mode_need) {
        fail_at(vm, L, em);
        (void)aln;
        return -1;
      }
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
  }

  return 0;
}
