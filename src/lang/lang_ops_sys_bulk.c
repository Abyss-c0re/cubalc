/* CubalC lang — lang_ops_sys_bulk.c
 * SYS bulk bag map/filter plane (BASENAMEALL, KEEP*, SIZEALL, MTIMEALL) (A2).
 * Mesh vote A2_theme_peel_core_sys · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_sys_bulk.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <unistd.h>
#  include <sys/stat.h>
#endif

int cubalc_lang_ops_sys_try_bulk(VM *vm, Lex *L) {
  /* Bulk bag map/filter SYS plane.
   * Caller: after plate/path_fs/env/string/wait/stat miss; L at subcommand.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
    /* SYS BASENAMEALL|MAPBASE|MAPBASENAME [bag]
     * SYS DIRNAMEALL|MAPDIR|MAPDIRNAME [bag]
     * SYS EXTALL|MAPEXT [bag]
     * SYS STEMALL|MAPSTEM [bag]
     * — map path peels over newline bag fields (default LAST).
     * Usability: PATHGLOB → basenames/stems/dirs without EACH + BASENAME glue. */
    if (kw(&L->cur,"BASENAMEALL") || kw(&L->cur,"MAPBASE") || kw(&L->cur,"MAPBASENAME") ||
        kw(&L->cur,"LEAFALL") || kw(&L->cur,"DIRNAMEALL") || kw(&L->cur,"MAPDIR") ||
        kw(&L->cur,"MAPDIRNAME") || kw(&L->cur,"PARENTALL") || kw(&L->cur,"EXTALL") ||
        kw(&L->cur,"MAPEXT") || kw(&L->cur,"STEMALL") || kw(&L->cur,"MAPSTEM")){
      int mode = 0; /* 0 base · 1 dir · 2 ext · 3 stem */
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t flen, olen = 0;
      long kept = 0;
      if (kw(&L->cur,"DIRNAMEALL") || kw(&L->cur,"MAPDIR") || kw(&L->cur,"MAPDIRNAME") ||
          kw(&L->cur,"PARENTALL"))
        mode = 1;
      else if (kw(&L->cur,"EXTALL") || kw(&L->cur,"MAPEXT"))
        mode = 2;
      else if (kw(&L->cur,"STEMALL") || kw(&L->cur,"MAPSTEM"))
        mode = 3;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      p = bag;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen > 0) {
          char path[512], piece[512];
          const char *slash, *dot, *base;
          size_t n, take = flen;
          if (take >= sizeof path) take = sizeof path - 1;
          memcpy(path, start, take);
          path[take] = 0;
          n = strlen(path);
          while (n > 1 && (path[n - 1] == '/' || path[n - 1] == '\\')) {
            path[n - 1] = 0;
            n--;
          }
          slash = cubalc_path_slash(path);
          if (mode == 0) {
            if (slash && slash[1])
              snprintf(piece, sizeof piece, "%s", slash + 1);
            else if (slash && (slash[0] == '/' || slash[0] == '\\') && !slash[1])
              snprintf(piece, sizeof piece, "%s", path);
            else
              snprintf(piece, sizeof piece, "%s", path[0] ? path : ".");
          } else if (mode == 1) {
            if (slash && slash != path) {
              size_t dn = (size_t)(slash - path);
              if (dn >= sizeof piece) dn = sizeof piece - 1;
              memcpy(piece, path, dn);
              piece[dn] = 0;
            } else if (slash && slash == path)
              snprintf(piece, sizeof piece, "%c", path[0] == '\\' ? '\\' : '/');
            else
              snprintf(piece, sizeof piece, ".");
          } else {
            char leaf[512];
            if (slash && slash[1])
              snprintf(leaf, sizeof leaf, "%s", slash + 1);
            else if (slash && (slash[0] == '/' || slash[0] == '\\') && !slash[1])
              snprintf(leaf, sizeof leaf, "%s", path);
            else
              snprintf(leaf, sizeof leaf, "%s", path[0] ? path : ".");
            base = leaf;
            dot = strrchr(leaf, '.');
            if (dot && dot != leaf) {
              if (mode == 2)
                snprintf(piece, sizeof piece, "%s", dot);
              else {
                size_t sn = (size_t)(dot - leaf);
                if (sn >= sizeof piece) sn = sizeof piece - 1;
                memcpy(piece, leaf, sn);
                piece[sn] = 0;
              }
            } else {
              if (mode == 2)
                piece[0] = 0;
              else
                snprintf(piece, sizeof piece, "%s", base);
            }
          }
          {
            size_t plen = strlen(piece);
            if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + plen < sizeof out) {
              memcpy(out + olen, piece, plen);
              olen += plen;
            }
            out[olen] = 0;
            kept++;
          }
        }
        if (*p == '\n') p++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "OK", 1);
      if (mode == 0) {
        var_set_str(vm, "BASENAMEALL", out);
        var_set_str(vm, "MAPBASE", out);
        var_set_num(vm, "BASENAMEALL_N", kept);
        var_set_num(vm, "MAPBASE_N", kept);
      } else if (mode == 1) {
        var_set_str(vm, "DIRNAMEALL", out);
        var_set_str(vm, "MAPDIR", out);
        var_set_num(vm, "DIRNAMEALL_N", kept);
        var_set_num(vm, "MAPDIR_N", kept);
      } else if (mode == 2) {
        var_set_str(vm, "EXTALL", out);
        var_set_str(vm, "MAPEXT", out);
        var_set_num(vm, "EXTALL_N", kept);
        var_set_num(vm, "MAPEXT_N", kept);
      } else {
        var_set_str(vm, "STEMALL", out);
        var_set_str(vm, "MAPSTEM", out);
        var_set_num(vm, "STEMALL_N", kept);
        var_set_num(vm, "MAPSTEM_N", kept);
      }
      bump(vm); return 1;
    }
    /* SYS KEEPFILES|FILTERFILES|ONLYFILES [bag]
     * SYS KEEPDIRS|FILTERDIRS|ONLYDIRS [bag]
     * SYS KEEPEXIST|FILTEREXIST|ONLYEXIST [bag]
     * — keep path bag fields that are regular files / directories / any existing path.
     * Default bag LAST. LAST = kept paths; LAST_N = count; KEEP*_DROP = removed.
     * Usability: PATHGLOB/LIST then drop dirs/missing before READ without EACH+ISFILE. */
    if (kw(&L->cur,"KEEPFILES") || kw(&L->cur,"FILTERFILES") || kw(&L->cur,"ONLYFILES") ||
        kw(&L->cur,"KEEPFILE") || kw(&L->cur,"FILESONLY") ||
        kw(&L->cur,"KEEPDIRS") || kw(&L->cur,"FILTERDIRS") || kw(&L->cur,"ONLYDIRS") ||
        kw(&L->cur,"KEEPDIR") || kw(&L->cur,"DIRSONLY") ||
        kw(&L->cur,"KEEPEXIST") || kw(&L->cur,"FILTEREXIST") || kw(&L->cur,"ONLYEXIST") ||
        kw(&L->cur,"KEEPEXISTS") || kw(&L->cur,"EXISTONLY") || kw(&L->cur,"EXISTING")){
      int mode = 0; /* 0 files · 1 dirs · 2 exist */
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t flen, olen = 0;
      long kept = 0, drop = 0;
      cubalc_host_result hr;
      if (kw(&L->cur,"KEEPDIRS") || kw(&L->cur,"FILTERDIRS") || kw(&L->cur,"ONLYDIRS") ||
          kw(&L->cur,"KEEPDIR") || kw(&L->cur,"DIRSONLY"))
        mode = 1;
      else if (kw(&L->cur,"KEEPEXIST") || kw(&L->cur,"FILTEREXIST") || kw(&L->cur,"ONLYEXIST") ||
               kw(&L->cur,"KEEPEXISTS") || kw(&L->cur,"EXISTONLY") || kw(&L->cur,"EXISTING"))
        mode = 2;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      p = bag;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen > 0) {
          char path[512];
          size_t take = flen;
          int hit = 0;
          if (take >= sizeof path) take = sizeof path - 1;
          memcpy(path, start, take);
          path[take] = 0;
          if (mode == 2) {
            hit = cubalc_host_exists(path) ? 1 : 0;
          } else {
            if (cubalc_host_path_kind(path, &hr) == 0) {
              if (mode == 0) hit = (hr.code == 1);
              else hit = (hr.code == 2);
            } else {
              hit = 0;
            }
          }
          if (hit) {
            if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + take < sizeof out) {
              memcpy(out + olen, path, take);
              olen += take;
            }
            out[olen] = 0;
            kept++;
          } else {
            drop++;
          }
        }
        if (*p == '\n') p++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "OK", 1);
      if (mode == 0) {
        var_set_str(vm, "KEEPFILES", out);
        var_set_str(vm, "FILTERFILES", out);
        var_set_str(vm, "ONLYFILES", out);
        var_set_num(vm, "KEEPFILES_N", kept);
        var_set_num(vm, "KEEPFILES_DROP", drop);
        var_set_num(vm, "FILTERFILES_N", kept);
        var_set_num(vm, "ONLYFILES_N", kept);
      } else if (mode == 1) {
        var_set_str(vm, "KEEPDIRS", out);
        var_set_str(vm, "FILTERDIRS", out);
        var_set_str(vm, "ONLYDIRS", out);
        var_set_num(vm, "KEEPDIRS_N", kept);
        var_set_num(vm, "KEEPDIRS_DROP", drop);
        var_set_num(vm, "FILTERDIRS_N", kept);
        var_set_num(vm, "ONLYDIRS_N", kept);
      } else {
        var_set_str(vm, "KEEPEXIST", out);
        var_set_str(vm, "FILTEREXIST", out);
        var_set_str(vm, "ONLYEXIST", out);
        var_set_num(vm, "KEEPEXIST_N", kept);
        var_set_num(vm, "KEEPEXIST_DROP", drop);
        var_set_num(vm, "FILTEREXIST_N", kept);
        var_set_num(vm, "ONLYEXIST_N", kept);
      }
      bump(vm); return 1;
    }
    /* SYS SIZEALL|MAPSIZE|FILESIZES [bag]
     * — map each path field to decimal byte size (regular files).
     * Missing / dirs / other → "0". LAST = size bag; LAST_N = field count;
     * SIZEALL_SUM = total bytes of regular files; SIZEALL_FILES = file hits;
     * SIZEALL_MISS = non-file fields.
     * Usability: PATHGLOB+KEEPFILES inventory without EACH+SIZE glue. */
    if (kw(&L->cur,"SIZEALL") || kw(&L->cur,"MAPSIZE") || kw(&L->cur,"FILESIZES") ||
        kw(&L->cur,"SIZES") || kw(&L->cur,"MAPFSIZE") || kw(&L->cur,"BYTESALL")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], nbuf[32];
      const char *p, *start;
      size_t flen, olen = 0;
      long kept = 0, sum = 0, files = 0, miss = 0;
      cubalc_host_result hr;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      p = bag;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen > 0) {
          char path[512];
          size_t take = flen, nlen;
          long sz = 0;
          if (take >= sizeof path) take = sizeof path - 1;
          memcpy(path, start, take);
          path[take] = 0;
          if (cubalc_host_path_kind(path, &hr) == 0 && hr.code == 1) {
            sz = hr.n;
            sum += sz;
            files++;
          } else {
            miss++;
          }
          snprintf(nbuf, sizeof nbuf, "%ld", sz);
          nlen = strlen(nbuf);
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + nlen < sizeof out) {
            memcpy(out + olen, nbuf, nlen);
            olen += nlen;
          }
          out[olen] = 0;
          kept++;
        }
        if (*p == '\n') p++;
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "SIZEALL", out);
      var_set_str(vm, "MAPSIZE", out);
      var_set_str(vm, "FILESIZES", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "SIZEALL_N", kept);
      var_set_num(vm, "MAPSIZE_N", kept);
      var_set_num(vm, "SIZEALL_SUM", sum);
      var_set_num(vm, "SIZEALL_FILES", files);
      var_set_num(vm, "SIZEALL_MISS", miss);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MTIMEALL|MAPMTIME [bag] — epoch mtime of each path field (0 if miss).
     * SYS AGEALL|MAPAGE [bag] — age seconds (now-mtime, clamped ≥0; 0 if miss).
     * LAST = decimal bag; LAST_N = count; MTIMEALL_HIT/AGEALL_HIT = existing paths;
     * MTIMEALL_MAX / MTIMEALL_MIN among hits (0 if none); AGEALL_MAX / AGEALL_MIN.
     * Usability: PATHGLOB freshness inventory without EACH+MTIME/AGE glue. */
    if (kw(&L->cur,"MTIMEALL") || kw(&L->cur,"MAPMTIME") || kw(&L->cur,"MTIMES") ||
        kw(&L->cur,"FILETIMES") || kw(&L->cur,"AGEALL") || kw(&L->cur,"MAPAGE") ||
        kw(&L->cur,"AGES") || kw(&L->cur,"FILEAGES") || kw(&L->cur,"STALEALL")){
      int want_age = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], nbuf[32];
      const char *p, *start;
      size_t flen, olen = 0;
      long kept = 0, hit = 0, miss = 0;
      long vmax = 0, vmin = 0, have_ex = 0;
      long now = (long)time(NULL);
      cubalc_host_result hr;
      if (kw(&L->cur,"AGEALL") || kw(&L->cur,"MAPAGE") || kw(&L->cur,"AGES") ||
          kw(&L->cur,"FILEAGES") || kw(&L->cur,"STALEALL"))
        want_age = 1;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      p = bag;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen > 0) {
          char path[512];
          size_t take = flen, nlen;
          long v = 0;
          if (take >= sizeof path) take = sizeof path - 1;
          memcpy(path, start, take);
          path[take] = 0;
          if (cubalc_host_mtime(path, &hr) == 0 && hr.ok) {
            if (want_age) {
              v = now - hr.n;
              if (v < 0) v = 0;
            } else {
              v = hr.n;
            }
            hit++;
            if (!have_ex) {
              vmax = vmin = v;
              have_ex = 1;
            } else {
              if (v > vmax) vmax = v;
              if (v < vmin) vmin = v;
            }
          } else {
            miss++;
            v = 0;
          }
          snprintf(nbuf, sizeof nbuf, "%ld", v);
          nlen = strlen(nbuf);
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + nlen < sizeof out) {
            memcpy(out + olen, nbuf, nlen);
            olen += nlen;
          }
          out[olen] = 0;
          kept++;
        }
        if (*p == '\n') p++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "OK", 1);
      if (want_age) {
        var_set_str(vm, "AGEALL", out);
        var_set_str(vm, "MAPAGE", out);
        var_set_num(vm, "AGEALL_N", kept);
        var_set_num(vm, "MAPAGE_N", kept);
        var_set_num(vm, "AGEALL_HIT", hit);
        var_set_num(vm, "AGEALL_MISS", miss);
        var_set_num(vm, "AGEALL_MAX", have_ex ? vmax : 0);
        var_set_num(vm, "AGEALL_MIN", have_ex ? vmin : 0);
      } else {
        var_set_str(vm, "MTIMEALL", out);
        var_set_str(vm, "MAPMTIME", out);
        var_set_num(vm, "MTIMEALL_N", kept);
        var_set_num(vm, "MAPMTIME_N", kept);
        var_set_num(vm, "MTIMEALL_HIT", hit);
        var_set_num(vm, "MTIMEALL_MISS", miss);
        var_set_num(vm, "MTIMEALL_MAX", have_ex ? vmax : 0);
        var_set_num(vm, "MTIMEALL_MIN", have_ex ? vmin : 0);
      }
      bump(vm); return 1;
    }
    /* SYS NEWEST|LATEST [bag] — path with max mtime (first on ties).
     * SYS OLDEST|EARLIEST [bag] — path with min mtime among existing.
     * SYS LARGEST|BIGGEST [bag] — regular file with max size.
     * SYS SMALLEST|TINIEST [bag] — regular file with min size (incl. empty).
     * LAST = winning path; LAST_N = metric (mtime epoch or bytes); *_I = 0-based index;
     * soft empty bag / no hit → OK=0 empty LAST LAST_N=0.
     * Usability: freshest/heaviest plate without MTIMEALL+ARGMAX+NTH glue. */
    if (kw(&L->cur,"NEWEST") || kw(&L->cur,"LATEST") || kw(&L->cur,"MOSTRECENT") ||
        kw(&L->cur,"OLDEST") || kw(&L->cur,"EARLIEST") || kw(&L->cur,"LEASTRECENT") ||
        kw(&L->cur,"LARGEST") || kw(&L->cur,"BIGGEST") || kw(&L->cur,"MAXSIZE") ||
        kw(&L->cur,"SMALLEST") || kw(&L->cur,"TINIEST") || kw(&L->cur,"MINSIZE")){
      int mode = 0; /* 0 newest · 1 oldest · 2 largest · 3 smallest */
      char bag[CUBALC_HOST_STR_MAX], best[512];
      const char *p, *start;
      size_t flen;
      long idx = 0, best_i = -1, best_v = 0, nscan = 0;
      cubalc_host_result hr;
      if (kw(&L->cur,"OLDEST") || kw(&L->cur,"EARLIEST") || kw(&L->cur,"LEASTRECENT"))
        mode = 1;
      else if (kw(&L->cur,"LARGEST") || kw(&L->cur,"BIGGEST") || kw(&L->cur,"MAXSIZE"))
        mode = 2;
      else if (kw(&L->cur,"SMALLEST") || kw(&L->cur,"TINIEST") || kw(&L->cur,"MINSIZE"))
        mode = 3;
      lex_next(L);
      bag[0] = 0; best[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      p = bag;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen > 0) {
          char path[512];
          size_t take = flen;
          long v = 0;
          int okv = 0;
          if (take >= sizeof path) take = sizeof path - 1;
          memcpy(path, start, take);
          path[take] = 0;
          if (mode == 0 || mode == 1) {
            if (cubalc_host_mtime(path, &hr) == 0 && hr.ok) {
              v = hr.n;
              okv = 1;
            }
          } else {
            if (cubalc_host_path_kind(path, &hr) == 0 && hr.code == 1) {
              v = hr.n;
              okv = 1;
            }
          }
          if (okv) {
            if (best_i < 0 ||
                (mode == 0 && v > best_v) ||
                (mode == 1 && v < best_v) ||
                (mode == 2 && v > best_v) ||
                (mode == 3 && v < best_v)) {
              best_i = idx;
              best_v = v;
              snprintf(best, sizeof best, "%s", path);
            }
          }
          idx++;
          nscan++;
        }
        if (*p == '\n') p++;
      }
      if (best_i < 0) {
        var_set_str(vm, "LAST", "");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "NEWEST_I", -1);
        var_set_num(vm, "OLDEST_I", -1);
        var_set_num(vm, "LARGEST_I", -1);
        var_set_num(vm, "SMALLEST_I", -1);
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", best);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", best);
      vm->last_n = best_v;
      var_set_num(vm, "LAST_N", best_v);
      var_set_num(vm, "OK", 1);
      if (mode == 0) {
        var_set_str(vm, "NEWEST", best);
        var_set_str(vm, "LATEST", best);
        var_set_num(vm, "NEWEST_I", best_i);
        var_set_num(vm, "NEWEST_N", best_v);
        var_set_num(vm, "LATEST_N", best_v);
      } else if (mode == 1) {
        var_set_str(vm, "OLDEST", best);
        var_set_str(vm, "EARLIEST", best);
        var_set_num(vm, "OLDEST_I", best_i);
        var_set_num(vm, "OLDEST_N", best_v);
      } else if (mode == 2) {
        var_set_str(vm, "LARGEST", best);
        var_set_str(vm, "BIGGEST", best);
        var_set_num(vm, "LARGEST_I", best_i);
        var_set_num(vm, "LARGEST_N", best_v);
      } else {
        var_set_str(vm, "SMALLEST", best);
        var_set_str(vm, "TINIEST", best);
        var_set_num(vm, "SMALLEST_I", best_i);
        var_set_num(vm, "SMALLEST_N", best_v);
      }
      (void)nscan;
      bump(vm); return 1;
    }
    /* SYS SORTMTIME|MTIMESORT [bag] [ASC|DESC]
     * SYS SORTSIZE|SIZESORT [bag] [ASC|DESC]
     * — order path bag by mtime or regular-file size.
     * Default DESC (newest / largest first). Missing paths sort last (metric 0).
     * LAST = ordered paths; LAST_N = count; SORTMTIME_N / SORTSIZE_N = count.
     * Usability: process freshest/heaviest plates first without shell ls -t. */
    if (kw(&L->cur,"SORTMTIME") || kw(&L->cur,"MTIMESORT") || kw(&L->cur,"SORTBYMTIME") ||
        kw(&L->cur,"SORTSIZE") || kw(&L->cur,"SIZESORT") || kw(&L->cur,"SORTBYSIZE") ||
        kw(&L->cur,"LSORT") || kw(&L->cur,"TSORT")){
      int by_size = 0, asc = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      char paths[96][320];
      long mets[96];
      int ord[96];
      const char *p, *start;
      size_t flen, olen = 0;
      int n = 0, i, j;
      cubalc_host_result hr;
      if (kw(&L->cur,"SORTSIZE") || kw(&L->cur,"SIZESORT") || kw(&L->cur,"SORTBYSIZE") ||
          kw(&L->cur,"LSORT"))
        by_size = 1;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      /* optional ASC|DESC|UP|DOWN|REV after bag */
      if (kw(&L->cur,"ASC") || kw(&L->cur,"ASCENDING") || kw(&L->cur,"UP") ||
          kw(&L->cur,"OLDFIRST") || kw(&L->cur,"SMALLFIRST")){
        asc = 1; lex_next(L);
      } else if (kw(&L->cur,"DESC") || kw(&L->cur,"DESCENDING") || kw(&L->cur,"DOWN") ||
                 kw(&L->cur,"NEWFIRST") || kw(&L->cur,"BIGFIRST") || kw(&L->cur,"REV") ||
                 kw(&L->cur,"REVERSE")){
        asc = 0; lex_next(L);
      }
      p = bag;
      while (*p && n < 96) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen > 0) {
          size_t take = flen;
          long v = 0;
          if (take >= sizeof paths[0]) take = sizeof paths[0] - 1;
          memcpy(paths[n], start, take);
          paths[n][take] = 0;
          if (by_size) {
            if (cubalc_host_path_kind(paths[n], &hr) == 0 && hr.code == 1)
              v = hr.n;
            else
              v = -1; /* miss/dir → last when DESC */
          } else {
            if (cubalc_host_mtime(paths[n], &hr) == 0 && hr.ok)
              v = hr.n;
            else
              v = -1;
          }
          mets[n] = v;
          ord[n] = n;
          n++;
        }
        if (*p == '\n') p++;
      }
      /* stable-ish insertion sort on ord[] by mets */
      for (i = 1; i < n; i++) {
        int key = ord[i];
        long kv = mets[key];
        j = i - 1;
        while (j >= 0) {
          long jv = mets[ord[j]];
          int less = asc ? (kv < jv) : (kv > jv);
          /* treat -1 as always worse (end) */
          if (kv < 0 && jv >= 0) less = 0;
          else if (kv >= 0 && jv < 0) less = 1;
          if (!less) break;
          ord[j + 1] = ord[j];
          j--;
        }
        ord[j + 1] = key;
      }
      for (i = 0; i < n; i++) {
        size_t plen = strlen(paths[ord[i]]);
        if (i > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        if (olen + plen < sizeof out) {
          memcpy(out + olen, paths[ord[i]], plen);
          olen += plen;
        }
        out[olen] = 0;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "OK", 1);
      if (by_size) {
        var_set_str(vm, "SORTSIZE", out);
        var_set_str(vm, "SIZESORT", out);
        var_set_num(vm, "SORTSIZE_N", n);
        var_set_num(vm, "SIZESORT_N", n);
      } else {
        var_set_str(vm, "SORTMTIME", out);
        var_set_str(vm, "MTIMESORT", out);
        var_set_num(vm, "SORTMTIME_N", n);
        var_set_num(vm, "MTIMESORT_N", n);
      }
      bump(vm); return 1;
    }
    /* SYS FRESH|KEEPFRESH bag max_age_sec
     * SYS KEEPSTALE|AGED|OLDERTHAN bag min_age_sec
     * — keep age <= max (FRESH) or age >= min (KEEPSTALE). Bare STALE = single-path AGE.
     * Age = now - mtime, clamped ≥0. Missing paths dropped.
     * LAST = kept paths; LAST_N / FRESH_N / STALE_N = count; *_DROP = removed.
     * Usability: cache/lease plate filters without EACH+AGE glue. */
    if (kw(&L->cur,"FRESH") || kw(&L->cur,"KEEPFRESH") || kw(&L->cur,"ONLYFRESH") ||
        kw(&L->cur,"NOTSTALE") || kw(&L->cur,"KEEPSTALE") || kw(&L->cur,"ONLYSTALE") ||
        kw(&L->cur,"NOTFRESH") || kw(&L->cur,"OLDFILES") || kw(&L->cur,"AGED") ||
        kw(&L->cur,"OLDERTHAN") || kw(&L->cur,"MINAGE") || kw(&L->cur,"STALEFILES") ||
        kw(&L->cur,"STALEBAG")){
      int want_stale = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t flen, olen = 0;
      long kept = 0, drop = 0, age_lim = 0;
      long now = (long)time(NULL);
      cubalc_host_result hr;
      if (kw(&L->cur,"KEEPSTALE") || kw(&L->cur,"ONLYSTALE") ||
          kw(&L->cur,"NOTFRESH") || kw(&L->cur,"OLDFILES") || kw(&L->cur,"AGED") ||
          kw(&L->cur,"OLDERTHAN") || kw(&L->cur,"MINAGE") || kw(&L->cur,"STALEFILES") ||
          kw(&L->cur,"STALEBAG"))
        want_stale = 1;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      /* age limit: number, or string var holding number */
      if (L->cur.kind == TK_NUM) {
        age_lim = L->cur.num;
        if (age_lim < 0) age_lim = 0;
        lex_next(L);
      } else {
        char tbuf[64];
        if (resolve_str_arg(vm, L, tbuf, sizeof tbuf) == 0) {
          age_lim = strtol(tbuf, NULL, 10);
          if (age_lim < 0) age_lim = 0;
        } else {
          fail(vm, want_stale ? "SYS KEEPSTALE bag min_age_sec"
                              : "SYS FRESH bag max_age_sec");
          return -1;
        }
      }
      p = bag;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen > 0) {
          char path[512];
          size_t take = flen;
          int hit = 0;
          if (take >= sizeof path) take = sizeof path - 1;
          memcpy(path, start, take);
          path[take] = 0;
          if (cubalc_host_mtime(path, &hr) == 0 && hr.ok) {
            long age = now - hr.n;
            if (age < 0) age = 0;
            if (want_stale)
              hit = (age >= age_lim);
            else
              hit = (age <= age_lim);
          }
          if (hit) {
            if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + take < sizeof out) {
              memcpy(out + olen, path, take);
              olen += take;
            }
            out[olen] = 0;
            kept++;
          } else {
            drop++;
          }
        }
        if (*p == '\n') p++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "OK", 1);
      if (want_stale) {
        var_set_str(vm, "STALE", out);
        var_set_str(vm, "KEEPSTALE", out);
        var_set_num(vm, "STALE_N", kept);
        var_set_num(vm, "STALE_DROP", drop);
        var_set_num(vm, "KEEPSTALE_N", kept);
        var_set_num(vm, "STALE_AGE", age_lim);
      } else {
        var_set_str(vm, "FRESH", out);
        var_set_str(vm, "KEEPFRESH", out);
        var_set_num(vm, "FRESH_N", kept);
        var_set_num(vm, "FRESH_DROP", drop);
        var_set_num(vm, "KEEPFRESH_N", kept);
        var_set_num(vm, "FRESH_AGE", age_lim);
      }
      bump(vm); return 1;
    }
    /* SYS KEEPNEWER|NEWERTHAN bag ref
     * SYS KEEPOLDER|OLDERREF bag ref
     * — keep bag paths with mtime > ref (KEEPNEWER) or mtime < ref (KEEPOLDER).
     * Make-style: missing ref ⇒ KEEPNEWER keeps all existing paths (rebuild);
     * KEEPOLDER keeps none when ref missing. Missing bag paths dropped.
     * Strict compare (equal mtime not newer/older). One-arg form uses LAST bag.
     * LAST = kept; LAST_N / KEEPNEWER_N / KEEPOLDER_N; *_DROP; *_REF.
     * Usability: rebuild gates without EACH+MTIME dual CMP. */
    if (kw(&L->cur,"KEEPNEWER") || kw(&L->cur,"NEWERTHAN") || kw(&L->cur,"AFTERFILE") ||
        kw(&L->cur,"NEWERREF") || kw(&L->cur,"MTGT") || kw(&L->cur,"KEEPIFNEWER") ||
        kw(&L->cur,"NEWERFILES") || kw(&L->cur,"KEEPOLDER") || kw(&L->cur,"OLDERREF") ||
        kw(&L->cur,"BEFOREFILE") || kw(&L->cur,"MTLT") || kw(&L->cur,"KEEPIFOLDER") ||
        kw(&L->cur,"OLDERFILES") || kw(&L->cur,"OLDERTHANREF")){
      int want_older = 0;
      char bag[CUBALC_HOST_STR_MAX], ref[512], out[CUBALC_HOST_STR_MAX];
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t flen, olen = 0;
      long kept = 0, drop = 0;
      long ref_mt = 0;
      int ref_ok = 0;
      cubalc_host_result hr;
      if (kw(&L->cur,"KEEPOLDER") || kw(&L->cur,"OLDERREF") ||
          kw(&L->cur,"BEFOREFILE") || kw(&L->cur,"MTLT") || kw(&L->cur,"KEEPIFOLDER") ||
          kw(&L->cur,"OLDERFILES") || kw(&L->cur,"OLDERTHANREF"))
        want_older = 1;
      lex_next(L);
      bag[0] = 0; ref[0] = 0; out[0] = 0; a[0] = 0; b[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, want_older ? "SYS KEEPOLDER [bag] ref"
                            : "SYS KEEPNEWER [bag] ref");
        return -1;
      }
      if (resolve_str_arg(vm, L, b, sizeof b) == 0) {
        snprintf(bag, sizeof bag, "%s", a);
        snprintf(ref, sizeof ref, "%s", b);
      } else {
        snprintf(bag, sizeof bag, "%s", vm->last_str);
        snprintf(ref, sizeof ref, "%s", a);
      }
      if (cubalc_host_mtime(ref, &hr) == 0 && hr.ok) {
        ref_ok = 1;
        ref_mt = hr.n;
      }
      p = bag;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen > 0) {
          char path[512];
          size_t take = flen;
          int hit = 0;
          if (take >= sizeof path) take = sizeof path - 1;
          memcpy(path, start, take);
          path[take] = 0;
          if (cubalc_host_mtime(path, &hr) == 0 && hr.ok) {
            if (want_older) {
              /* older than ref: need ref present and path_mt < ref_mt */
              hit = ref_ok && (hr.n < ref_mt);
            } else {
              /* newer than ref: missing ref ⇒ rebuild (keep); else path_mt > ref_mt */
              hit = (!ref_ok) || (hr.n > ref_mt);
            }
          }
          if (hit) {
            if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + take < sizeof out) {
              memcpy(out + olen, path, take);
              olen += take;
            }
            out[olen] = 0;
            kept++;
          } else {
            drop++;
          }
        }
        if (*p == '\n') p++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "OK", 1);
      var_set_str(vm, want_older ? "KEEPOLDER_REF" : "KEEPNEWER_REF", ref);
      var_set_num(vm, want_older ? "KEEPOLDER_REF_OK" : "KEEPNEWER_REF_OK", ref_ok ? 1 : 0);
      if (ref_ok)
        var_set_num(vm, want_older ? "KEEPOLDER_REF_MT" : "KEEPNEWER_REF_MT", ref_mt);
      if (want_older) {
        var_set_str(vm, "KEEPOLDER", out);
        var_set_str(vm, "OLDERREF", out);
        var_set_num(vm, "KEEPOLDER_N", kept);
        var_set_num(vm, "KEEPOLDER_DROP", drop);
        var_set_num(vm, "OLDERREF_N", kept);
      } else {
        var_set_str(vm, "KEEPNEWER", out);
        var_set_str(vm, "NEWERTHAN", out);
        var_set_num(vm, "KEEPNEWER_N", kept);
        var_set_num(vm, "KEEPNEWER_DROP", drop);
        var_set_num(vm, "NEWERTHAN_N", kept);
      }
      bump(vm); return 1;
    }
    /* SYS KEEPBIGGER|BIGFILES|SIZEGE bag min_bytes
     * SYS KEEPSMALLER|SMALLFILES|SIZELE bag max_bytes
     * — keep regular-file paths with size >= min (BIGGER) or size <= max (SMALLER).
     * Dirs/missing/other dropped. Inclusive thresholds. One-arg uses LAST bag.
     * (MINSIZE/MAXSIZE remain SMALLEST/LARGEST pick aliases — not used here.)
     * LAST = kept paths; LAST_N / KEEPBIGGER_N / KEEPSMALLER_N; *_DROP; *_BYTES.
     * Usability: GC/reclaim path filters without EACH+SIZE glue. */
    if (kw(&L->cur,"KEEPBIGGER") || kw(&L->cur,"BIGFILES") || kw(&L->cur,"SIZEGE") ||
        kw(&L->cur,"KEEPMINSIZE") || kw(&L->cur,"ATLEAST") || kw(&L->cur,"BIGGERTHAN") ||
        kw(&L->cur,"KEEPSMALLER") || kw(&L->cur,"SMALLFILES") || kw(&L->cur,"SIZELE") ||
        kw(&L->cur,"KEEPMAXSIZE") || kw(&L->cur,"ATMOST") || kw(&L->cur,"TINYFILES") ||
        kw(&L->cur,"SMALLERTHAN")){
      int want_small = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t flen, olen = 0;
      long kept = 0, drop = 0, lim = 0;
      cubalc_host_result hr;
      if (kw(&L->cur,"KEEPSMALLER") || kw(&L->cur,"SMALLFILES") || kw(&L->cur,"SIZELE") ||
          kw(&L->cur,"KEEPMAXSIZE") || kw(&L->cur,"ATMOST") || kw(&L->cur,"TINYFILES") ||
          kw(&L->cur,"SMALLERTHAN"))
        want_small = 1;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      /* size limit: number, or string var holding number */
      if (L->cur.kind == TK_NUM) {
        lim = L->cur.num;
        if (lim < 0) lim = 0;
        lex_next(L);
      } else {
        char tbuf[64];
        if (resolve_str_arg(vm, L, tbuf, sizeof tbuf) == 0) {
          lim = strtol(tbuf, NULL, 10);
          if (lim < 0) lim = 0;
        } else {
          /* one-arg numeric-as-bag failed: bag was LAST, need limit from bag if it was num?
           * If bag resolved but no limit, treat bag content as limit and use LAST...
           * Simpler: if first arg looked like number via string and no second, re-parse. */
          fail(vm, want_small ? "SYS KEEPSMALLER bag max_bytes"
                              : "SYS KEEPBIGGER bag min_bytes");
          return -1;
        }
      }
      /* Detect one-arg: first resolve took LAST_str as bag, second was the only arg
       * which we put in bag — if we got lim from second and bag was from first resolve
       * that succeeded... Standard FRESH pattern: bag first (or LAST), then lim.
       * For `SYS KEEPBIGGER 1024` with LAST bag: resolve_str_arg fails on TK_NUM,
       * bag=LAST, then TK_NUM → lim. Good.
       * For `SYS KEEPBIGGER bag 1024`: resolve bag, then TK_NUM lim. Good. */
      p = bag;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen > 0) {
          char path[512];
          size_t take = flen;
          int hit = 0;
          if (take >= sizeof path) take = sizeof path - 1;
          memcpy(path, start, take);
          path[take] = 0;
          if (cubalc_host_path_kind(path, &hr) == 0 && hr.code == 1) {
            long sz = hr.n;
            if (want_small)
              hit = (sz <= lim);
            else
              hit = (sz >= lim);
          }
          if (hit) {
            if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + take < sizeof out) {
              memcpy(out + olen, path, take);
              olen += take;
            }
            out[olen] = 0;
            kept++;
          } else {
            drop++;
          }
        }
        if (*p == '\n') p++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "OK", 1);
      if (want_small) {
        var_set_str(vm, "KEEPSMALLER", out);
        var_set_str(vm, "SMALLFILES", out);
        var_set_str(vm, "SIZELE", out);
        var_set_num(vm, "KEEPSMALLER_N", kept);
        var_set_num(vm, "KEEPSMALLER_DROP", drop);
        var_set_num(vm, "KEEPSMALLER_BYTES", lim);
        var_set_num(vm, "SMALLFILES_N", kept);
        var_set_num(vm, "SIZELE_N", kept);
      } else {
        var_set_str(vm, "KEEPBIGGER", out);
        var_set_str(vm, "BIGFILES", out);
        var_set_str(vm, "SIZEGE", out);
        var_set_num(vm, "KEEPBIGGER_N", kept);
        var_set_num(vm, "KEEPBIGGER_DROP", drop);
        var_set_num(vm, "KEEPBIGGER_BYTES", lim);
        var_set_num(vm, "BIGFILES_N", kept);
        var_set_num(vm, "SIZEGE_N", kept);
      }
      bump(vm); return 1;
    }
  return 0;
}
