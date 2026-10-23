/* CubalC lang — lang_ops_cell_libread.c
 * CELL lib read: CATLIB·GREPLIB·HEADLIB/TAILLIB (G_cell_libread).
 * Peel from lang_ops_cell.c. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_cell_libread.h"
#include "lang/cubalc_lang_internal.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

int cubalc_lang_ops_cell_try_libread(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* CATLIB / READLIB / LIBSRC name — soft dump lib source → LAST (dual of cubalc cat).
   * Usability: agents inspect INCLUDE recipes without shell · resolve like INCLUDE short name.
   * LAST_N = bytes (capped) · LIB_PATH · soft miss OK=0 sticky LAST_ERR. */
  if (kw(&L->cur, "CATLIB") || kw(&L->cur, "READLIB") ||
      kw(&L->cur, "LIBSRC") || kw(&L->cur, "LIBCAT") ||
      kw(&L->cur, "SHOWLIB") || kw(&L->cur, "DUMPLIB") ||
      kw(&L->cur, "CAT_LIB") || kw(&L->cur, "READ_LIB")) {
    char name[160], path[768], base[160];
    char *buf = NULL;
    FILE *f = NULL;
    long sz = 0;
    size_t nr = 0, cap = 0;
    const char *slash, *leaf, *ip, *root;
    size_t blen;
    lex_next(L);
    name[0] = 0;
    path[0] = 0;
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
    if (!name[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "CATLIB: need name");
      var_set_str(vm, "ERR", "CATLIB: need name");
      bump(vm);
      return 1;
    }
    slash = strrchr(name, '/');
    leaf = slash ? slash + 1 : name;
    snprintf(base, sizeof base, "%s", leaf);
    blen = strlen(base);
    if (blen > 7 && strcmp(base + blen - 7, ".cubalc") == 0)
      base[blen - 7] = 0;
    /* direct path */
    if (name[0] == '/' || strchr(name, '/')) {
      f = fopen(name, "rb");
      if (f) snprintf(path, sizeof path, "%s", name);
    }
    if (!f) {
      char p3[768];
      snprintf(p3, sizeof p3, "programs/lib/%s.cubalc", base);
      f = fopen(p3, "rb");
      if (f) snprintf(path, sizeof path, "%s", p3);
    }
    if (!f) {
      char p3[768];
      snprintf(p3, sizeof p3, "programs/lib/%s", name);
      f = fopen(p3, "rb");
      if (f) snprintf(path, sizeof path, "%s", p3);
    }
    if (!f) {
      f = fopen(name, "rb");
      if (f) snprintf(path, sizeof path, "%s", name);
    }
    root = getenv("CUBALC_ROOT");
    if (!f && root && root[0]) {
      char p2[768];
      snprintf(p2, sizeof p2, "%s/programs/lib/%s.cubalc", root, base);
      f = fopen(p2, "rb");
      if (f) snprintf(path, sizeof path, "%s", p2);
    }
    ip = getenv("CUBALC_INCLUDE_PATH");
    if (!f && ip && ip[0]) {
      const char *seg = ip;
      while (*seg && !f) {
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
        if (f) { snprintf(path, sizeof path, "%s", p3); break; }
        snprintf(p3, sizeof p3, "%s/%s", dir, name);
        f = fopen(p3, "rb");
        if (f) { snprintf(path, sizeof path, "%s", p3); break; }
      }
    }
    if (!f) {
      char em[192];
      snprintf(em, sizeof em,
               "CATLIB miss: '%s' — programs/lib · CUBALC_INCLUDE_PATH · cubalc cat",
               name);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "LIB_PATH", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) sz = 0;
    /* Cap to var string budget so agents can hold source in LAST */
    cap = (size_t)CUBALC_VAR_STR_MAX - 1;
    if ((size_t)sz > cap) sz = (long)cap;
    buf = (char *)malloc((size_t)sz + 1);
    if (!buf) {
      fclose(f);
      var_set_str(vm, "LAST", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "CATLIB: oom");
      var_set_str(vm, "ERR", "CATLIB: oom");
      bump(vm);
      return 1;
    }
    nr = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[nr] = 0;
    var_set_str(vm, "LAST", buf);
    var_set_str(vm, "LIBSRC", buf);
    var_set_str(vm, "CATLIB", buf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
    free(buf);
    vm->last_n = (long)nr;
    var_set_num(vm, "LAST_N", (long)nr);
    var_set_num(vm, "CATLIB_N", (long)nr);
    var_set_str(vm, "LIB_PATH", path);
    var_set_str(vm, "LIB_STEM", base);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* GREPLIB name needle — matching lines from one lib → LAST bag (soft miss).
   * SEARCHLIBS / LIBGREP / GREPLIBS needle — stems whose source contains needle.
   * Completes LISTLIBS/HASLIB/CATLIB discovery: agents find which INCLUDE recipe
   * holds a form/keyword without shell grep across programs/lib. */
  if (kw(&L->cur, "GREPLIB") || kw(&L->cur, "SEARCHLIB") ||
      kw(&L->cur, "LIBGREPONE") || kw(&L->cur, "GREP_LIB") ||
      kw(&L->cur, "SEARCHLIBS") || kw(&L->cur, "LIBGREP") ||
      kw(&L->cur, "GREPLIBS") || kw(&L->cur, "FINDINLIBS") ||
      kw(&L->cur, "SEARCH_LIBS") || kw(&L->cur, "LIBS_GREP")) {
    int all_stems = kw(&L->cur, "SEARCHLIBS") || kw(&L->cur, "LIBGREP") ||
                    kw(&L->cur, "GREPLIBS") || kw(&L->cur, "FINDINLIBS") ||
                    kw(&L->cur, "SEARCH_LIBS") || kw(&L->cur, "LIBS_GREP");
    char name[160], needle[256], path[768], base[160];
    char bag[CUBALC_VAR_STR_MAX];
    size_t o = 0;
    int nmatch = 0;
    FILE *f = NULL;
    const char *slash, *leaf, *ip, *root;
    size_t blen;
    lex_next(L);
    name[0] = 0;
    needle[0] = 0;
    path[0] = 0;
    bag[0] = 0;
    /* parse 1 or 2 string/ident args */
    {
      char a1[256], a2[256];
      a1[0] = a2[0] = 0;
      if (L->cur.kind == TK_STR) {
        snprintf(a1, sizeof a1, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(a1, sizeof a1, "%s", vv->sval);
        else if (strcmp(L->cur.text, "LAST") == 0)
          snprintf(a1, sizeof a1, "%s", vm->last_str);
        else
          snprintf(a1, sizeof a1, "%s", L->cur.text);
        lex_next(L);
      }
      if (L->cur.kind == TK_STR) {
        snprintf(a2, sizeof a2, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(a2, sizeof a2, "%s", vv->sval);
        else if (strcmp(L->cur.text, "LAST") == 0)
          snprintf(a2, sizeof a2, "%s", vm->last_str);
        else
          snprintf(a2, sizeof a2, "%s", L->cur.text);
        lex_next(L);
      }
      if (all_stems || !a2[0]) {
        /* one arg = needle across all libs */
        snprintf(needle, sizeof needle, "%s", a1);
        all_stems = 1;
      } else {
        snprintf(name, sizeof name, "%s", a1);
        snprintf(needle, sizeof needle, "%s", a2);
      }
    }
    if (!needle[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "GREPLIB: need needle");
      var_set_str(vm, "ERR", "GREPLIB: need needle");
      bump(vm);
      return 1;
    }

    if (all_stems) {
      /* scan programs/lib + INCLUDE_PATH; keep stems whose source contains needle */
      char stems[96][96];
      int nstem = 0, i, j;
      DIR *d;
      struct dirent *ent;
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
          char stem[96], fpath[768];
          int dup = 0;
          FILE *lf;
          char *src = NULL;
          long sz = 0;
          size_t nr = 0;
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
          snprintf(fpath, sizeof fpath, "%s/%s", dirs[di], ent->d_name);
          lf = fopen(fpath, "rb");
          if (!lf) continue;
          fseek(lf, 0, SEEK_END);
          sz = ftell(lf);
          fseek(lf, 0, SEEK_SET);
          if (sz < 0) sz = 0;
          if (sz > 256 * 1024) sz = 256 * 1024; /* safety */
          src = (char *)malloc((size_t)sz + 1);
          if (!src) { fclose(lf); continue; }
          nr = fread(src, 1, (size_t)sz, lf);
          fclose(lf);
          src[nr] = 0;
          if (strstr(src, needle)) {
            snprintf(stems[nstem++], sizeof stems[0], "%s", stem);
          }
          free(src);
        }
        closedir(d);
      }
      /* stable sort */
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
      nmatch = nstem;
      var_set_str(vm, "LAST", bag);
      var_set_str(vm, "SEARCHLIBS", bag);
      var_set_str(vm, "LIBGREP", bag);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
      vm->last_n = nmatch;
      var_set_num(vm, "LAST_N", nmatch);
      var_set_num(vm, "GREPLIB_N", nmatch);
      var_set_num(vm, "SEARCHLIBS_N", nmatch);
      var_set_str(vm, "GREPLIB_NEEDLE", needle);
      var_set_num(vm, "OK", 1);
      bump(vm);
      return 1;
    }

    /* single-lib line grep (GREPLIB name needle) */
    slash = strrchr(name, '/');
    leaf = slash ? slash + 1 : name;
    snprintf(base, sizeof base, "%s", leaf);
    blen = strlen(base);
    if (blen > 7 && strcmp(base + blen - 7, ".cubalc") == 0)
      base[blen - 7] = 0;
    if (name[0] == '/' || strchr(name, '/')) {
      f = fopen(name, "rb");
      if (f) snprintf(path, sizeof path, "%s", name);
    }
    if (!f) {
      char p3[768];
      snprintf(p3, sizeof p3, "programs/lib/%s.cubalc", base);
      f = fopen(p3, "rb");
      if (f) snprintf(path, sizeof path, "%s", p3);
    }
    if (!f) {
      char p3[768];
      snprintf(p3, sizeof p3, "programs/lib/%s", name);
      f = fopen(p3, "rb");
      if (f) snprintf(path, sizeof path, "%s", p3);
    }
    if (!f) {
      f = fopen(name, "rb");
      if (f) snprintf(path, sizeof path, "%s", name);
    }
    root = getenv("CUBALC_ROOT");
    if (!f && root && root[0]) {
      char p2[768];
      snprintf(p2, sizeof p2, "%s/programs/lib/%s.cubalc", root, base);
      f = fopen(p2, "rb");
      if (f) snprintf(path, sizeof path, "%s", p2);
    }
    ip = getenv("CUBALC_INCLUDE_PATH");
    if (!f && ip && ip[0]) {
      const char *seg = ip;
      while (*seg && !f) {
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
        if (f) { snprintf(path, sizeof path, "%s", p3); break; }
        snprintf(p3, sizeof p3, "%s/%s", dir, name);
        f = fopen(p3, "rb");
        if (f) { snprintf(path, sizeof path, "%s", p3); break; }
      }
    }
    if (!f) {
      char em[192];
      snprintf(em, sizeof em,
               "GREPLIB miss: '%s' — programs/lib · CUBALC_INCLUDE_PATH · cubalc cat",
               name);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "LIB_PATH", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
    /* stream lines; keep those containing needle */
    {
      char line[1024];
      o = 0;
      nmatch = 0;
      while (fgets(line, sizeof line, f)) {
        size_t ll = strlen(line);
        while (ll > 0 && (line[ll - 1] == '\n' || line[ll - 1] == '\r'))
          line[--ll] = 0;
        if (!strstr(line, needle)) continue;
        if (nmatch > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + ll < sizeof bag) {
          memcpy(bag + o, line, ll);
          o += ll;
          bag[o] = 0;
          nmatch++;
        } else {
          break; /* bag full */
        }
      }
    }
    fclose(f);
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "GREPLIB", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = nmatch;
    var_set_num(vm, "LAST_N", nmatch);
    var_set_num(vm, "GREPLIB_N", nmatch);
    var_set_str(vm, "LIB_PATH", path);
    var_set_str(vm, "LIB_STEM", base);
    var_set_str(vm, "GREPLIB_NEEDLE", needle);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HEADLIB / LIBHEAD / PEEKLIB name [n] — first n lines of a lib recipe → LAST bag.
   * TAILLIB / LIBTAIL name [n] — last n lines (default 16, cap 256).
   * Usability: peek INCLUDE recipe headers without full CATLIB clobber · dual of
   * HEADFILE/TAILFILE with short-name resolve (programs/lib · INCLUDE_PATH). Soft miss. */
  if (kw(&L->cur, "HEADLIB") || kw(&L->cur, "LIBHEAD") ||
      kw(&L->cur, "PEEKLIB") || kw(&L->cur, "TOPLIB") ||
      kw(&L->cur, "HEAD_LIB") || kw(&L->cur, "LIB_HEAD") ||
      kw(&L->cur, "TAILLIB") || kw(&L->cur, "LIBTAIL") ||
      kw(&L->cur, "BOTTOMLIB") || kw(&L->cur, "ENDLIB") ||
      kw(&L->cur, "TAIL_LIB") || kw(&L->cur, "LIB_TAIL")) {
    int want_tail = kw(&L->cur, "TAILLIB") || kw(&L->cur, "LIBTAIL") ||
                    kw(&L->cur, "BOTTOMLIB") || kw(&L->cur, "ENDLIB") ||
                    kw(&L->cur, "TAIL_LIB") || kw(&L->cur, "LIB_TAIL");
    char name[160], path[768], base[160];
    char bag[CUBALC_VAR_STR_MAX];
    char *src = NULL;
    FILE *f = NULL;
    long sz = 0, nwant = 16, total = 0, kept = 0;
    size_t nr = 0, o = 0;
    const char *slash, *leaf, *ip, *root, *lp, *ls;
    size_t blen, llen;
    lex_next(L);
    name[0] = 0;
    path[0] = 0;
    bag[0] = 0;
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
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      nwant = parse_expr(vm, L);
    } else if (L->cur.kind == TK_STR) {
      nwant = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && !vv->is_str)
        nwant = vv->val;
      else if (vv && vv->is_str)
        nwant = atol(vv->sval);
      else if (strcmp(L->cur.text, "LAST_N") == 0)
        nwant = vm->last_n;
      lex_next(L);
    }
    if (nwant < 0) nwant = 0;
    if (nwant > 256) nwant = 256;
    if (!name[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", want_tail ? "TAILLIB: need name" : "HEADLIB: need name");
      var_set_str(vm, "ERR", want_tail ? "TAILLIB: need name" : "HEADLIB: need name");
      bump(vm);
      return 1;
    }
    slash = strrchr(name, '/');
    leaf = slash ? slash + 1 : name;
    snprintf(base, sizeof base, "%s", leaf);
    blen = strlen(base);
    if (blen > 7 && strcmp(base + blen - 7, ".cubalc") == 0)
      base[blen - 7] = 0;
    if (name[0] == '/' || strchr(name, '/')) {
      f = fopen(name, "rb");
      if (f) snprintf(path, sizeof path, "%s", name);
    }
    if (!f) {
      char p3[768];
      snprintf(p3, sizeof p3, "programs/lib/%s.cubalc", base);
      f = fopen(p3, "rb");
      if (f) snprintf(path, sizeof path, "%s", p3);
    }
    if (!f) {
      char p3[768];
      snprintf(p3, sizeof p3, "programs/lib/%s", name);
      f = fopen(p3, "rb");
      if (f) snprintf(path, sizeof path, "%s", p3);
    }
    if (!f) {
      f = fopen(name, "rb");
      if (f) snprintf(path, sizeof path, "%s", name);
    }
    root = getenv("CUBALC_ROOT");
    if (!f && root && root[0]) {
      char p2[768];
      snprintf(p2, sizeof p2, "%s/programs/lib/%s.cubalc", root, base);
      f = fopen(p2, "rb");
      if (f) snprintf(path, sizeof path, "%s", p2);
    }
    ip = getenv("CUBALC_INCLUDE_PATH");
    if (!f && ip && ip[0]) {
      const char *seg = ip;
      while (*seg && !f) {
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
        if (f) { snprintf(path, sizeof path, "%s", p3); break; }
        snprintf(p3, sizeof p3, "%s/%s", dir, name);
        f = fopen(p3, "rb");
        if (f) { snprintf(path, sizeof path, "%s", p3); break; }
      }
    }
    if (!f) {
      char em[192];
      snprintf(em, sizeof em,
               "%s miss: '%s' — programs/lib · CUBALC_INCLUDE_PATH · cubalc cat",
               want_tail ? "TAILLIB" : "HEADLIB", name);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "LIB_PATH", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) sz = 0;
    if (sz > 256 * 1024) sz = 256 * 1024;
    src = (char *)malloc((size_t)sz + 1);
    if (!src) {
      fclose(f);
      var_set_str(vm, "LAST", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", want_tail ? "TAILLIB: oom" : "HEADLIB: oom");
      var_set_str(vm, "ERR", want_tail ? "TAILLIB: oom" : "HEADLIB: oom");
      bump(vm);
      return 1;
    }
    nr = fread(src, 1, (size_t)sz, f);
    fclose(f);
    src[nr] = 0;
    /* count total lines */
    total = 0;
    lp = src;
    if (nr == 0) {
      total = 0;
    } else {
      while (*lp) {
        while (*lp && *lp != '\n') lp++;
        total++;
        if (*lp == '\n') lp++;
      }
    }
    o = 0;
    kept = 0;
    if (want_tail) {
      long skip = total > nwant ? total - nwant : 0;
      long seen = 0;
      lp = src;
      while (*lp) {
        ls = lp;
        while (*lp && *lp != '\n') lp++;
        llen = (size_t)(lp - ls);
        if (seen >= skip) {
          if (kept > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
          if (o + llen < sizeof bag) {
            memcpy(bag + o, ls, llen);
            o += llen;
          }
          bag[o] = 0;
          kept++;
        }
        seen++;
        if (*lp == '\n') lp++;
      }
    } else {
      lp = src;
      while (*lp && kept < nwant) {
        ls = lp;
        while (*lp && *lp != '\n') lp++;
        llen = (size_t)(lp - ls);
        if (kept > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + llen < sizeof bag) {
          memcpy(bag + o, ls, llen);
          o += llen;
        }
        bag[o] = 0;
        kept++;
        if (*lp == '\n') lp++;
      }
    }
    free(src);
    var_set_str(vm, "LAST", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = kept;
    var_set_num(vm, "LAST_N", kept);
    var_set_str(vm, "LIB_PATH", path);
    var_set_str(vm, "LIB_STEM", base);
    if (want_tail) {
      var_set_str(vm, "TAILLIB", bag);
      var_set_str(vm, "LIBTAIL", bag);
      var_set_num(vm, "TAILLIB_N", kept);
      var_set_num(vm, "TAILLIB_TOTAL", total);
      var_set_num(vm, "TAILLIB_WANT", nwant);
    } else {
      var_set_str(vm, "HEADLIB", bag);
      var_set_str(vm, "LIBHEAD", bag);
      var_set_num(vm, "HEADLIB_N", kept);
      var_set_num(vm, "HEADLIB_TOTAL", total);
      var_set_num(vm, "HEADLIB_WANT", nwant);
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  return 0;
}
