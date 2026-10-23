/* CubalC lang — lang_ops_cell_libpick.c
 * CELL lib pick: PICKLIB·NTHLIB·NEWEST/OLDEST·SORTLIBS (G_cell_libpick).
 * Peel from lang_ops_cell.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_cell_libpick.h"
#include "lang/cubalc_lang_internal.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

int cubalc_lang_ops_cell_try_libpick(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* PICKLIB / FIRSTLIB needle [OR fallback] — first matching stem (sorted).
   * Usability: INCLUDE without MATCHLIBS+NTH glue · soft miss or OR default.
   * LAST = stem · LAST_N 0|1 · PICKLIB_N = match count · soft. */
  if (kw(&L->cur, "PICKLIB") || kw(&L->cur, "FIRSTLIB") ||
      kw(&L->cur, "LIBPICK") || kw(&L->cur, "CHOOSELIB") ||
      kw(&L->cur, "ONEOFLIBS") || kw(&L->cur, "ANYLIBSTEM") ||
      kw(&L->cur, "PICK_LIB") || kw(&L->cur, "FIRST_LIB")) {
    char needle[96], fup[96], fb[96], pick[96];
    char stems[96][96];
    int nstem = 0, i, j, have_fb = 0;
    size_t a;
    DIR *d;
    struct dirent *ent;
    const char *ip;
    lex_next(L);
    needle[0] = 0;
    fb[0] = 0;
    pick[0] = 0;
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
    if (kw(&L->cur, "OR") || kw(&L->cur, "ELSE") || kw(&L->cur, "DEFAULT") ||
        kw(&L->cur, "FALLBACK")) {
      lex_next(L);
      if (L->cur.kind == TK_STR) {
        snprintf(fb, sizeof fb, "%s", L->cur.text);
        have_fb = 1;
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str)
          snprintf(fb, sizeof fb, "%s", vv->sval);
        else if (strcmp(L->cur.text, "LAST") == 0)
          snprintf(fb, sizeof fb, "%s", vm->last_str);
        else
          snprintf(fb, sizeof fb, "%s", L->cur.text);
        have_fb = 1;
        lex_next(L);
      }
    }
    if (!needle[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "PICKLIB_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "PICKLIB: need needle — PICKLIB plate OR agent_boot");
      var_set_str(vm, "ERR", "PICKLIB: need needle — PICKLIB plate OR agent_boot");
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
          char stem[96], hay[192];
          int dup = 0;
          size_t b;
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
          snprintf(hay, sizeof hay, "%s/%s", dirs[di], ent->d_name);
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
          snprintf(stems[nstem++], sizeof stems[0], "%s", stem);
        }
        closedir(d);
      }
    }
    for (i = 1; i < nstem; i++) {
      char tmp[96];
      snprintf(tmp, sizeof tmp, "%s", stems[i]);
      j = i;
      while (j > 0 && strcmp(stems[j - 1], tmp) > 0) {
        snprintf(stems[j], sizeof stems[0], "%s", stems[j - 1]);
        j--;
      }
      snprintf(stems[j], sizeof stems[0], "%s", tmp);
    }
    if (nstem > 0)
      snprintf(pick, sizeof pick, "%s", stems[0]);
    else if (have_fb)
      snprintf(pick, sizeof pick, "%s", fb);
    var_set_num(vm, "PICKLIB_N", nstem);
    var_set_num(vm, "MATCHLIBS_N", nstem);
    var_set_str(vm, "PICKLIB_FILTER", needle);
    var_set_str(vm, "MATCHLIBS_FILTER", needle);
    if (pick[0]) {
      var_set_str(vm, "LAST", pick);
      var_set_str(vm, "PICKLIB", pick);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", pick);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "OK", 1);
    } else {
      char em[160];
      snprintf(em, sizeof em,
               "PICKLIB miss: '%s' — MATCHLIBS · cubalc libs %s · OR fallback",
               needle, needle);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    bump(vm);
    return 1;
  }

  /* NTHLIB idx needle [OR fallback] · LASTLIB needle [OR fallback]
   * Usability: index/last pick from MATCHLIBS without bag+NTH glue.
   * Completes PICKLIB (first only) — agents INCLUDE the 2nd/last match.
   * LAST = stem · LAST_N 0|1 · NTHLIB_N / MATCHLIBS_N = match count · soft. */
  if (kw(&L->cur, "NTHLIB") || kw(&L->cur, "INDEXLIB") ||
      kw(&L->cur, "ATLIB") || kw(&L->cur, "LIBNTH") ||
      kw(&L->cur, "NTH_LIB") || kw(&L->cur, "INDEX_LIB") ||
      kw(&L->cur, "LASTLIB") || kw(&L->cur, "ENDLIB") ||
      kw(&L->cur, "LIBLAST") || kw(&L->cur, "LAST_LIB") ||
      kw(&L->cur, "END_LIB") || kw(&L->cur, "TAILLIBSTEM")) {
    char needle[96], fup[96], fb[96], pick[96];
    char stems[96][96];
    int nstem = 0, i, j, have_fb = 0, want_last = 0, idx = 0, have_idx = 0;
    size_t a;
    DIR *d;
    struct dirent *ent;
    const char *ip;
    if (kw(&L->cur, "LASTLIB") || kw(&L->cur, "ENDLIB") ||
        kw(&L->cur, "LIBLAST") || kw(&L->cur, "LAST_LIB") ||
        kw(&L->cur, "END_LIB") || kw(&L->cur, "TAILLIBSTEM"))
      want_last = 1;
    lex_next(L);
    needle[0] = 0;
    fb[0] = 0;
    pick[0] = 0;
    /* NTHLIB: index then needle · LASTLIB: needle only */
    if (!want_last) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS) {
        idx = (int)parse_expr(vm, L);
        have_idx = 1;
      } else if (L->cur.kind == TK_IDENT) {
        Var *vi = var_get(vm, L->cur.text, 0);
        if (vi && !vi->is_str) {
          idx = (int)vi->val;
          have_idx = 1;
          lex_next(L);
        }
      }
      if (!have_idx) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "NTHLIB_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR",
                    "NTHLIB: need index — NTHLIB 0 plate · LASTLIB plate");
        var_set_str(vm, "ERR",
                    "NTHLIB: need index — NTHLIB 0 plate · LASTLIB plate");
        bump(vm);
        return 1;
      }
    }
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
    if (kw(&L->cur, "OR") || kw(&L->cur, "ELSE") || kw(&L->cur, "DEFAULT") ||
        kw(&L->cur, "FALLBACK")) {
      lex_next(L);
      if (L->cur.kind == TK_STR) {
        snprintf(fb, sizeof fb, "%s", L->cur.text);
        have_fb = 1;
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str)
          snprintf(fb, sizeof fb, "%s", vv->sval);
        else if (strcmp(L->cur.text, "LAST") == 0)
          snprintf(fb, sizeof fb, "%s", vm->last_str);
        else
          snprintf(fb, sizeof fb, "%s", L->cur.text);
        have_fb = 1;
        lex_next(L);
      }
    }
    if (!needle[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "NTHLIB_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  "NTHLIB/LASTLIB: need needle — NTHLIB 0 plate · LASTLIB fat");
      var_set_str(vm, "ERR",
                  "NTHLIB/LASTLIB: need needle — NTHLIB 0 plate · LASTLIB fat");
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
          char stem[96], hay[192];
          int dup = 0;
          size_t b;
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
          snprintf(hay, sizeof hay, "%s/%s", dirs[di], ent->d_name);
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
          snprintf(stems[nstem++], sizeof stems[0], "%s", stem);
        }
        closedir(d);
      }
    }
    for (i = 1; i < nstem; i++) {
      char tmp[96];
      snprintf(tmp, sizeof tmp, "%s", stems[i]);
      j = i;
      while (j > 0 && strcmp(stems[j - 1], tmp) > 0) {
        snprintf(stems[j], sizeof stems[0], "%s", stems[j - 1]);
        j--;
      }
      snprintf(stems[j], sizeof stems[0], "%s", tmp);
    }
    if (want_last)
      idx = nstem > 0 ? nstem - 1 : 0;
    if (nstem > 0 && idx >= 0 && idx < nstem)
      snprintf(pick, sizeof pick, "%s", stems[idx]);
    else if (have_fb)
      snprintf(pick, sizeof pick, "%s", fb);
    var_set_num(vm, "NTHLIB_N", nstem);
    var_set_num(vm, "LASTLIB_N", nstem);
    var_set_num(vm, "PICKLIB_N", nstem);
    var_set_num(vm, "MATCHLIBS_N", nstem);
    var_set_num(vm, "NTHLIB_I", want_last ? (nstem > 0 ? nstem - 1 : -1) : idx);
    var_set_str(vm, "NTHLIB_FILTER", needle);
    var_set_str(vm, "LASTLIB_FILTER", needle);
    var_set_str(vm, "MATCHLIBS_FILTER", needle);
    if (pick[0]) {
      var_set_str(vm, "LAST", pick);
      var_set_str(vm, "NTHLIB", pick);
      if (want_last)
        var_set_str(vm, "LASTLIB", pick);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", pick);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "OK", 1);
    } else {
      char em[176];
      if (want_last)
        snprintf(em, sizeof em,
                 "LASTLIB miss: '%s' — MATCHLIBS · cubalc libs %s · OR fallback",
                 needle, needle);
      else
        snprintf(em, sizeof em,
                 "NTHLIB miss: idx=%d '%s' — MATCHLIBS · OR fallback",
                 idx, needle);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    bump(vm);
    return 1;
  }

  /* NEWESTLIB|OLDESTLIB needle [OR fallback] — pick matching stem by mtime.
   * Usability: freshest/oldest recipe among filter matches without shell ls -t.
   * Dual of PICKLIB (alpha first) · LAST = stem · NEWESTLIB_MTIME epoch · soft. */
  if (kw(&L->cur, "NEWESTLIB") || kw(&L->cur, "LATESTLIB") ||
      kw(&L->cur, "FRESHLIB") || kw(&L->cur, "NEWLIB") ||
      kw(&L->cur, "OLDESTLIB") || kw(&L->cur, "EARLIESTLIB") ||
      kw(&L->cur, "STALELIB") || kw(&L->cur, "OLDLIB") ||
      kw(&L->cur, "NEWEST_LIB") || kw(&L->cur, "OLDEST_LIB")) {
    char needle[96], fup[96], fb[96], pick[96], pick_path[768];
    int want_old = 0, have_fb = 0, nstem = 0, j;
    long best_mt = 0;
    int have_best = 0;
    size_t a;
    DIR *d;
    struct dirent *ent;
    const char *ip;
    if (kw(&L->cur, "OLDESTLIB") || kw(&L->cur, "EARLIESTLIB") ||
        kw(&L->cur, "STALELIB") || kw(&L->cur, "OLDLIB") ||
        kw(&L->cur, "OLDEST_LIB"))
      want_old = 1;
    lex_next(L);
    needle[0] = 0;
    fb[0] = 0;
    pick[0] = 0;
    pick_path[0] = 0;
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
    if (kw(&L->cur, "OR") || kw(&L->cur, "ELSE") || kw(&L->cur, "DEFAULT") ||
        kw(&L->cur, "FALLBACK")) {
      lex_next(L);
      if (L->cur.kind == TK_STR) {
        snprintf(fb, sizeof fb, "%s", L->cur.text);
        have_fb = 1;
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str)
          snprintf(fb, sizeof fb, "%s", vv->sval);
        else if (strcmp(L->cur.text, "LAST") == 0)
          snprintf(fb, sizeof fb, "%s", vm->last_str);
        else
          snprintf(fb, sizeof fb, "%s", L->cur.text);
        have_fb = 1;
        lex_next(L);
      }
    }
    if (!needle[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "NEWESTLIB_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  "NEWESTLIB: need needle — NEWESTLIB plate · OLDESTLIB fat");
      var_set_str(vm, "ERR",
                  "NEWESTLIB: need needle — NEWESTLIB plate · OLDESTLIB fat");
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
      char seen[96][96];
      int nseen = 0;
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
        while ((ent = readdir(d)) != NULL && nseen < 96) {
          size_t len = strlen(ent->d_name);
          char stem[96], hay[192], full[768];
          int dup = 0;
          size_t b;
          struct stat st;
          if (len < 8 || strcmp(ent->d_name + len - 7, ".cubalc") != 0)
            continue;
          if (ent->d_name[0] == '.') continue;
          if (len - 7 >= sizeof stem) continue;
          memcpy(stem, ent->d_name, len - 7);
          stem[len - 7] = 0;
          for (j = 0; j < nseen; j++) {
            if (strcmp(seen[j], stem) == 0) { dup = 1; break; }
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
          snprintf(seen[nseen++], sizeof seen[0], "%s", stem);
          nstem++;
          if (!have_best ||
              (want_old ? (st.st_mtime < best_mt) : (st.st_mtime > best_mt)) ||
              (st.st_mtime == best_mt && strcmp(stem, pick) < 0)) {
            best_mt = (long)st.st_mtime;
            have_best = 1;
            snprintf(pick, sizeof pick, "%s", stem);
            snprintf(pick_path, sizeof pick_path, "%s", full);
          }
        }
        closedir(d);
      }
    }
    var_set_num(vm, "NEWESTLIB_N", nstem);
    var_set_num(vm, "OLDESTLIB_N", nstem);
    var_set_num(vm, "MATCHLIBS_N", nstem);
    var_set_str(vm, "MATCHLIBS_FILTER", needle);
    var_set_str(vm, "NEWESTLIB_FILTER", needle);
    if (have_best) {
      var_set_str(vm, "LAST", pick);
      var_set_str(vm, want_old ? "OLDESTLIB" : "NEWESTLIB", pick);
      var_set_str(vm, "LIB_PATH", pick_path);
      var_set_num(vm, "NEWESTLIB_MTIME", best_mt);
      var_set_num(vm, "OLDESTLIB_MTIME", best_mt);
      var_set_num(vm, "LIB_MTIME", best_mt);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", pick);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "OK", 1);
    } else if (have_fb) {
      var_set_str(vm, "LAST", fb);
      var_set_str(vm, want_old ? "OLDESTLIB" : "NEWESTLIB", fb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", fb);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "OK", 1);
      var_set_num(vm, "NEWESTLIB_FALLBACK", 1);
    } else {
      char em[176];
      snprintf(em, sizeof em,
               "%s miss: '%s' — MATCHLIBS · cubalc libs %s · OR fallback",
               want_old ? "OLDESTLIB" : "NEWESTLIB", needle, needle);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    bump(vm);
    return 1;
  }

  /* SORTLIBS needle [ASC|DESC|OLDEST|NEWEST] — bag of matching stems ordered by mtime.
   * Usability: walk recipes freshest-first (default DESC/NEWEST) without shell ls -t
   * or NEWESTLIB single-pick. Dual of alpha MATCHLIBS · head of bag == NEWESTLIB.
   * LAST = newline bag · LAST_N = count · LIB_PATH / SORTLIBS_MTIME for first · soft empty. */
  if (kw(&L->cur, "SORTLIBS") || kw(&L->cur, "SORTLIB") ||
      kw(&L->cur, "LIBSORT") || kw(&L->cur, "SORTBYMTIME") ||
      kw(&L->cur, "LIBSBYMTIME") || kw(&L->cur, "MATCHLIBSMT") ||
      kw(&L->cur, "MTIMESORTLIBS") || kw(&L->cur, "SORT_LIBS") ||
      kw(&L->cur, "LIBS_BY_MTIME") || kw(&L->cur, "SORTMTIMELIBS")) {
    char needle[96], fup[96], bag[4096];
    char stems[96][96];
    char stem_paths[96][768];
    long mtimes[96];
    int nstem = 0, i, j, want_old = 0;
    size_t o = 0, a;
    DIR *d;
    struct dirent *ent;
    const char *ip;
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
    /* optional order: ASC|OLDEST|EARLIEST → oldest first · else newest first */
    if (kw(&L->cur, "ASC") || kw(&L->cur, "OLDEST") || kw(&L->cur, "EARLIEST") ||
        kw(&L->cur, "OLD") || kw(&L->cur, "STALE") || kw(&L->cur, "OLDESTFIRST") ||
        kw(&L->cur, "ASCENDING")) {
      want_old = 1;
      lex_next(L);
    } else if (kw(&L->cur, "DESC") || kw(&L->cur, "NEWEST") ||
               kw(&L->cur, "LATEST") || kw(&L->cur, "NEW") ||
               kw(&L->cur, "FRESH") || kw(&L->cur, "NEWESTFIRST") ||
               kw(&L->cur, "DESCENDING")) {
      want_old = 0;
      lex_next(L);
    }
    if (!needle[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "SORTLIBS_N", 0);
      var_set_num(vm, "MATCHLIBS_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  "SORTLIBS: need needle — SORTLIBS fat · SORTLIBS plate ASC");
      var_set_str(vm, "ERR",
                  "SORTLIBS: need needle — SORTLIBS fat · SORTLIBS plate ASC");
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
          snprintf(stems[nstem], sizeof stems[0], "%s", stem);
          snprintf(stem_paths[nstem], sizeof stem_paths[0], "%s", full);
          mtimes[nstem] = (long)st.st_mtime;
          nstem++;
        }
        closedir(d);
      }
    }
    /* insertion sort by mtime (want_old: asc · else desc) · stem alpha ties */
    for (i = 1; i < nstem; i++) {
      char tmp[96], tp[768];
      long tm;
      snprintf(tmp, sizeof tmp, "%s", stems[i]);
      snprintf(tp, sizeof tp, "%s", stem_paths[i]);
      tm = mtimes[i];
      j = i;
      while (j > 0) {
        int before = 0;
        if (want_old) {
          if (mtimes[j - 1] > tm ||
              (mtimes[j - 1] == tm && strcmp(stems[j - 1], tmp) > 0))
            before = 1;
        } else {
          if (mtimes[j - 1] < tm ||
              (mtimes[j - 1] == tm && strcmp(stems[j - 1], tmp) > 0))
            before = 1;
        }
        if (!before) break;
        snprintf(stems[j], sizeof stems[0], "%s", stems[j - 1]);
        snprintf(stem_paths[j], sizeof stem_paths[0], "%s", stem_paths[j - 1]);
        mtimes[j] = mtimes[j - 1];
        j--;
      }
      snprintf(stems[j], sizeof stems[0], "%s", tmp);
      snprintf(stem_paths[j], sizeof stem_paths[0], "%s", tp);
      mtimes[j] = tm;
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
    var_set_str(vm, "SORTLIBS", bag);
    var_set_str(vm, "MATCHLIBS", bag);
    var_set_str(vm, "SORTLIBS_FILTER", needle);
    var_set_str(vm, "MATCHLIBS_FILTER", needle);
    var_set_str(vm, "SORTLIBS_ORDER", want_old ? "oldest" : "newest");
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = nstem;
    var_set_num(vm, "LAST_N", nstem);
    var_set_num(vm, "SORTLIBS_N", nstem);
    var_set_num(vm, "MATCHLIBS_N", nstem);
    if (nstem > 0) {
      var_set_str(vm, "LIB_PATH", stem_paths[0]);
      var_set_num(vm, "SORTLIBS_MTIME", mtimes[0]);
      var_set_num(vm, "LIB_MTIME", mtimes[0]);
      var_set_str(vm, "SORTLIBS_HEAD", stems[0]);
    } else {
      var_set_str(vm, "LIB_PATH", "");
      var_set_num(vm, "SORTLIBS_MTIME", 0);
      var_set_num(vm, "LIB_MTIME", 0);
      var_set_str(vm, "SORTLIBS_HEAD", "");
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  return 0;
}
