/* CubalC lang — lang_ops_cell_libcat.c
 * CELL lib catalog: LISTINCLUDES·LISTLIBS·MATCHLIBS·COUNT·HAS* (G_cell_margin).
 * Peel from lang_ops_cell.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_cell_libcat.h"
#include "lang/cubalc_lang_internal.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

int cubalc_lang_ops_cell_try_libcat(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* LISTINCLUDES / INCLUDES / LOADED — bag of resolved INCLUDE paths this run.
   * Usability: after cubalc run -I / INCLUDE ONCE, agents audit without guessing.
   * LAST = newline paths · LAST_N / INCLUDE_N = count · empty bag when none. */
  if (kw(&L->cur, "LISTINCLUDES") || kw(&L->cur, "INCLUDES") ||
      kw(&L->cur, "LOADED") || kw(&L->cur, "LISTINCLUDED") ||
      kw(&L->cur, "INCLUDED") || kw(&L->cur, "LIST_INCLUDES") ||
      kw(&L->cur, "INCLUDELIST")) {
    char bag[4096];
    size_t o = 0;
    int i, n = 0;
    lex_next(L);
    bag[0] = 0;
    for (i = 0; i < vm->n_included; i++) {
      size_t ln = strlen(vm->included[i]);
      if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, vm->included[i], ln);
        o += ln;
      }
      bag[o] = 0;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LISTINCLUDES", bag);
    var_set_str(vm, "INCLUDES", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "INCLUDE_N", n);
    var_set_num(vm, "LISTINCLUDES_N", n);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LISTLIBS / LIBSTEMS / STDLIBS — short-name bag from programs/lib (+ INCLUDE_PATH).
   * Usability: in-lang dual of cubalc libs — agents discover INCLUDE targets without shell.
   * LAST = newline stems (no .cubalc) · LAST_N / LIBS_N = count · HASLIB soft dual. */
  if (kw(&L->cur, "LISTLIBS") || kw(&L->cur, "LIBSTEMS") ||
      kw(&L->cur, "STDLIBS") || kw(&L->cur, "LIST_LIBS") ||
      kw(&L->cur, "LIBLIST") || kw(&L->cur, "CATALOG_LIBS") ||
      kw(&L->cur, "HASLIB") || kw(&L->cur, "HAS_LIB") ||
      kw(&L->cur, "LIBEXISTS") || kw(&L->cur, "HAVE_LIB")) {
    int soft_has = kw(&L->cur, "HASLIB") || kw(&L->cur, "HAS_LIB") ||
                   kw(&L->cur, "LIBEXISTS") || kw(&L->cur, "HAVE_LIB");
    char bag[4096];
    char stems[96][96];
    int nstem = 0, i, j, hit = 0;
    char want[96];
    size_t o = 0;
    DIR *d;
    struct dirent *ent;
    const char *ip;
    lex_next(L);
    want[0] = 0;
    bag[0] = 0;
    if (soft_has) {
      if (L->cur.kind == TK_STR) {
        snprintf(want, sizeof want, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(want, sizeof want, "%s", vv->sval);
        else if (strcmp(L->cur.text, "LAST") == 0)
          snprintf(want, sizeof want, "%s", vm->last_str);
        else
          snprintf(want, sizeof want, "%s", L->cur.text);
        lex_next(L);
      }
      /* strip optional .cubalc */
      {
        size_t wl = strlen(want);
        if (wl > 7 && strcmp(want + wl - 7, ".cubalc") == 0)
          want[wl - 7] = 0;
      }
    }
    /* scan programs/lib then CUBALC_INCLUDE_PATH dirs */
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
          char stem[96];
          int dup = 0;
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
          snprintf(stems[nstem++], sizeof stems[0], "%s", stem);
        }
        closedir(d);
      }
    }
    /* simple insertion sort for stable agent bags */
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
    if (soft_has) {
      for (i = 0; i < nstem; i++) {
        if (want[0] && strcmp(stems[i], want) == 0) { hit = 1; break; }
      }
      var_set_num(vm, "LAST_N", hit);
      vm->last_n = hit;
      var_set_num(vm, "HASLIB_N", hit);
      var_set_num(vm, "LIBS_N", nstem);
      {
        char nb[8];
        snprintf(nb, sizeof nb, "%d", hit);
        var_set_str(vm, "LAST", nb);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
      }
      if (hit) {
        var_set_num(vm, "OK", 1);
      } else {
        char em[160];
        snprintf(em, sizeof em,
                 "HASLIB miss: '%s' not in programs/lib · cubalc libs · LISTLIBS",
                 want[0] ? want : "?");
        var_set_str(vm, "ERR", em);
        var_set_str(vm, "LAST_ERR", em);
        var_set_num(vm, "OK", 0);
      }
      bump(vm);
      return 1;
    }
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
    var_set_str(vm, "LISTLIBS", bag);
    var_set_str(vm, "LIBSTEMS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = nstem;
    var_set_num(vm, "LAST_N", nstem);
    var_set_num(vm, "LIBS_N", nstem);
    var_set_num(vm, "LISTLIBS_N", nstem);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* MATCHLIBS / FILTERLIBS / LIBMATCH needle — bag of lib stems matching filter.
   * Usability: in-lang dual of cubalc libs [filter] — agents pick INCLUDE targets
   * without shell · case-insensitive stem/path substring · soft empty bag. */
  if (kw(&L->cur, "MATCHLIBS") || kw(&L->cur, "FILTERLIBS") ||
      kw(&L->cur, "LIBMATCH") || kw(&L->cur, "LIBSFILTER") ||
      kw(&L->cur, "GREPSTEMS") || kw(&L->cur, "FINDLIBS") ||
      kw(&L->cur, "MATCH_LIBS") || kw(&L->cur, "FILTER_LIBS")) {
    char needle[96], fup[96], bag[4096];
    char stems[96][96];
    int nstem = 0, nmatch = 0, i, j;
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
    if (!needle[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "MATCHLIBS_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "MATCHLIBS: need needle — MATCHLIBS plate");
      var_set_str(vm, "ERR", "MATCHLIBS: need needle — MATCHLIBS plate");
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
          /* case-insensitive stem + full path haystack (dual of cubalc libs filter) */
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
    /* sort */
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
    nmatch = nstem;
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
    var_set_str(vm, "MATCHLIBS", bag);
    var_set_str(vm, "FILTERLIBS", bag);
    var_set_str(vm, "MATCHLIBS_FILTER", needle);
    var_set_str(vm, "LIBS_FILTER", needle);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = nmatch;
    var_set_num(vm, "LAST_N", nmatch);
    var_set_num(vm, "MATCHLIBS_N", nmatch);
    var_set_num(vm, "FILTERLIBS_N", nmatch);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* COUNTMATCHLIBS|COUNTLIBS needle — match count → LAST_N without bag materialize.
   * Usability: IF/gates on how many libs match without MATCHLIBS clobbering LAST.
   * Completes bag · has · need · count quartet. Soft empty needle → OK=0. */
  if (kw(&L->cur, "COUNTMATCHLIBS") || kw(&L->cur, "COUNTLIBS") ||
      kw(&L->cur, "NMATCHLIBS") || kw(&L->cur, "MATCHLIBSCOUNT") ||
      kw(&L->cur, "LIBCOUNT") || kw(&L->cur, "COUNT_MATCHLIBS") ||
      kw(&L->cur, "COUNTFILTERLIBS")) {
    char needle[96], fup[96];
    int nmatch = 0, j;
    size_t a;
    DIR *d;
    struct dirent *ent;
    const char *ip;
    char stems[96][96];
    int nstem = 0;
    lex_next(L);
    needle[0] = 0;
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
    if (!needle[0]) {
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "MATCHLIBS_N", 0);
      var_set_num(vm, "COUNTMATCHLIBS_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "COUNTMATCHLIBS: need needle — COUNTMATCHLIBS plate");
      var_set_str(vm, "ERR", "COUNTMATCHLIBS: need needle — COUNTMATCHLIBS plate");
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
    nmatch = nstem;
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", nmatch);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    vm->last_n = nmatch;
    var_set_num(vm, "LAST_N", nmatch);
    var_set_num(vm, "MATCHLIBS_N", nmatch);
    var_set_num(vm, "COUNTMATCHLIBS_N", nmatch);
    var_set_str(vm, "MATCHLIBS_FILTER", needle);
    var_set_str(vm, "COUNTMATCHLIBS_FILTER", needle);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASCOUNTMATCHLIBS|NEEDCOUNTMATCHLIBS min needle — soft 0|1 / fail if count < min.
   * Usability: install gates “need ≥2 fat libs” without COUNTMATCHLIBS+IF compare glue.
   * Completes count · soft min · hard min triad. */
  if (kw(&L->cur, "HASCOUNTMATCHLIBS") || kw(&L->cur, "NEEDCOUNTMATCHLIBS") ||
      kw(&L->cur, "HASCOUNTLIBS") || kw(&L->cur, "NEEDCOUNTLIBS") ||
      kw(&L->cur, "REQUIRECOUNTLIBS") || kw(&L->cur, "REQUIRECOUNTMATCHLIBS") ||
      kw(&L->cur, "HAS_COUNTMATCHLIBS") || kw(&L->cur, "NEED_COUNTMATCHLIBS") ||
      kw(&L->cur, "MINMATCHLIBS") || kw(&L->cur, "NEEDMINLIBS")) {
    char needle[96], fup[96];
    int nmatch = 0, j, hard = 0, aln = L->cur.line;
    int min_n = 1, have_min = 0;
    size_t a;
    DIR *d;
    struct dirent *ent;
    const char *ip;
    char stems[96][96];
    int nstem = 0;
    if (kw(&L->cur, "NEEDCOUNTMATCHLIBS") || kw(&L->cur, "NEEDCOUNTLIBS") ||
        kw(&L->cur, "REQUIRECOUNTLIBS") || kw(&L->cur, "REQUIRECOUNTMATCHLIBS") ||
        kw(&L->cur, "NEED_COUNTMATCHLIBS") || kw(&L->cur, "MINMATCHLIBS") ||
        kw(&L->cur, "NEEDMINLIBS"))
      hard = 1;
    lex_next(L);
    needle[0] = 0;
    /* min then needle · min defaults to 1 if omitted before str needle is ambiguous */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS) {
      min_n = (int)parse_expr(vm, L);
      have_min = 1;
    } else if (L->cur.kind == TK_IDENT) {
      Var *vi = var_get(vm, L->cur.text, 0);
      if (vi && !vi->is_str) {
        min_n = (int)vi->val;
        have_min = 1;
        lex_next(L);
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
    if (!have_min) min_n = 1;
    if (min_n < 0) min_n = 0;
    if (!needle[0]) {
      if (hard) {
        fail_at(vm, L,
                "NEEDCOUNTMATCHLIBS needs min needle — NEEDCOUNTMATCHLIBS 2 fat");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "MATCHLIBS_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASCOUNTMATCHLIBS: need min needle");
      var_set_str(vm, "ERR", "HASCOUNTMATCHLIBS: need min needle");
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
    nmatch = nstem;
    var_set_num(vm, "MATCHLIBS_N", nmatch);
    var_set_num(vm, "COUNTMATCHLIBS_N", nmatch);
    var_set_num(vm, "HASCOUNTMATCHLIBS_MIN", min_n);
    var_set_str(vm, "MATCHLIBS_FILTER", needle);
    var_set_str(vm, "COUNTMATCHLIBS_FILTER", needle);
    {
      int ok = (nmatch >= min_n);
      if (hard) {
        if (ok) {
          var_set_str(vm, "LAST", "1");
          snprintf(vm->last_str, sizeof vm->last_str, "1");
          vm->last_n = 1;
          var_set_num(vm, "LAST_N", 1);
          var_set_num(vm, "OK", 1);
          if (vm->res) vm->res->asserts_ok++;
          bump(vm);
          return 1;
        }
        {
          char msg[200];
          snprintf(msg, sizeof msg,
                   "NEEDCOUNTMATCHLIBS miss line %d: need ≥%d '%s' have %d — MATCHLIBS · cubalc libs %s",
                   aln, min_n, needle, nmatch, needle);
          var_set_str(vm, "ERR", msg);
          var_set_str(vm, "LAST_ERR", msg);
          if (vm->res) vm->res->asserts_fail++;
          fail(vm, msg);
          return -1;
        }
      }
      /* soft HASCOUNTMATCHLIBS */
      {
        char nb[4];
        snprintf(nb, sizeof nb, "%d", ok ? 1 : 0);
        var_set_str(vm, "LAST", nb);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
      }
      vm->last_n = ok ? 1 : 0;
      var_set_num(vm, "LAST_N", ok ? 1 : 0);
      var_set_num(vm, "HASCOUNTMATCHLIBS_N", nmatch);
      if (ok) {
        var_set_num(vm, "OK", 1);
      } else {
        char em[176];
        snprintf(em, sizeof em,
                 "HASCOUNTMATCHLIBS miss: need ≥%d '%s' have %d — COUNTMATCHLIBS",
                 min_n, needle, nmatch);
        var_set_str(vm, "ERR", em);
        var_set_str(vm, "LAST_ERR", em);
        var_set_num(vm, "OK", 0);
      }
      bump(vm);
      return 1;
    }
  }

  /* HASMATCHLIBS / NEEDMATCHLIBS needle — soft 0|1 / fail-fast if any stem matches filter.
   * Usability: agent install/discovery gate without MATCHLIBS bag + LINES compare.
   * Completes MATCHLIBS triad (bag · soft probe · hard gate). */
  if (kw(&L->cur, "HASMATCHLIBS") || kw(&L->cur, "NEEDMATCHLIBS") ||
      kw(&L->cur, "ANYMATCHLIBS") || kw(&L->cur, "HASLIBS") ||
      kw(&L->cur, "NEEDLIBS") || kw(&L->cur, "REQUIREMATCHLIBS") ||
      kw(&L->cur, "HAS_MATCHLIBS") || kw(&L->cur, "NEED_MATCHLIBS")) {
    char needle[96], fup[96];
    int nmatch = 0, hard = 0, aln = L->cur.line;
    size_t a;
    DIR *d;
    struct dirent *ent;
    const char *ip;
    if (kw(&L->cur, "NEEDMATCHLIBS") || kw(&L->cur, "NEEDLIBS") ||
        kw(&L->cur, "REQUIREMATCHLIBS") || kw(&L->cur, "NEED_MATCHLIBS"))
      hard = 1;
    lex_next(L);
    needle[0] = 0;
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
    if (!needle[0]) {
      if (hard) {
        fail_at(vm, L, "NEEDMATCHLIBS needs needle — NEEDMATCHLIBS plate");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "MATCHLIBS_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASMATCHLIBS: need needle");
      var_set_str(vm, "ERR", "HASMATCHLIBS: need needle");
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
      for (di = 0; di < nd && nmatch == 0; di++) {
        d = opendir(dirs[di]);
        if (!d) continue;
        while ((ent = readdir(d)) != NULL) {
          size_t len = strlen(ent->d_name);
          char stem[96], hay[192], stem_l[96];
          size_t b;
          if (len < 8 || strcmp(ent->d_name + len - 7, ".cubalc") != 0)
            continue;
          if (ent->d_name[0] == '.') continue;
          if (len - 7 >= sizeof stem) continue;
          memcpy(stem, ent->d_name, len - 7);
          stem[len - 7] = 0;
          snprintf(hay, sizeof hay, "%s/%s", dirs[di], ent->d_name);
          for (b = 0; hay[b]; b++)
            if (hay[b] >= 'A' && hay[b] <= 'Z')
              hay[b] = (char)(hay[b] - 'A' + 'a');
          for (b = 0; stem[b] && b + 1 < sizeof stem_l; b++) {
            char c = stem[b];
            if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
            stem_l[b] = c;
          }
          stem_l[b] = 0;
          if (strstr(stem_l, fup) || strstr(hay, fup)) {
            nmatch = 1;
            break;
          }
        }
        closedir(d);
      }
    }
    var_set_str(vm, "MATCHLIBS_FILTER", needle);
    var_set_num(vm, "MATCHLIBS_N", nmatch);
    if (hard) {
      if (nmatch) {
        var_set_str(vm, "LAST", "1");
        snprintf(vm->last_str, sizeof vm->last_str, "1");
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_num(vm, "OK", 1);
        if (vm->res) vm->res->asserts_ok++;
        bump(vm);
        return 1;
      }
      {
        char msg[192];
        snprintf(msg, sizeof msg,
                 "NEEDMATCHLIBS miss line %d: '%s' — MATCHLIBS · cubalc libs %s · install libs",
                 aln, needle, needle);
        var_set_str(vm, "ERR", msg);
        var_set_str(vm, "LAST_ERR", msg);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
    }
    /* soft HASMATCHLIBS */
    {
      char nb[4];
      snprintf(nb, sizeof nb, "%d", nmatch);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    vm->last_n = nmatch;
    var_set_num(vm, "LAST_N", nmatch);
    var_set_num(vm, "HASMATCHLIBS_N", nmatch);
    if (nmatch) {
      var_set_num(vm, "OK", 1);
    } else {
      char em[160];
      snprintf(em, sizeof em,
               "HASMATCHLIBS miss: '%s' — MATCHLIBS · cubalc libs %s",
               needle, needle);
      var_set_str(vm, "ERR", em);
      var_set_str(vm, "LAST_ERR", em);
      var_set_num(vm, "OK", 0);
    }
    bump(vm);
    return 1;
  }


  return 0;
}
