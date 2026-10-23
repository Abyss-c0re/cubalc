/* CubalC lang — lang_ops_sys_bulk_mut.c
 * SYS bulk mutate plane (RMALL, TOUCHALL, COPYALL, MKDIRALL, MOVEALL, WALK, EQFILE, SWAP, DUPLEX) (A2).
 * Mesh vote A2_theme_peel_core_sys · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_sys_bulk_mut.h"
#include "lang/cubalc_lang_ops_sys_time_walk.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <unistd.h>
#  include <dirent.h>
#  include <sys/stat.h>
#  include <fcntl.h>
#endif

int cubalc_lang_ops_sys_try_bulk_mut(VM *vm, Lex *L) {
  /* Bulk mutate / local xfer SYS plane.
   * Caller: after bulk map-filter miss; L at subcommand.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
    /* SYS RMALL|UNLINKALL|DELETEALL [bag]
     * — remove every regular-file path in a newline bag (PATHGLOB/KEEP* pipeline).
     * Soft: missing paths count as MISS (idempotent); dirs/errors count FAIL but continue.
     * LAST = paths actually unlinked; LAST_N / RMALL_N = removed; RMALL_MISS / RMALL_FAIL.
     * OK=1 when FAIL==0. Usability: GC reclaim without EACH+RM glue. */
    if (kw(&L->cur,"RMALL") || kw(&L->cur,"UNLINKALL") || kw(&L->cur,"DELETEALL") ||
        kw(&L->cur,"REMOVEALL") || kw(&L->cur,"RMF") || kw(&L->cur,"PURGE") ||
        kw(&L->cur,"RMANY") || kw(&L->cur,"BULKRM") || kw(&L->cur,"RMFILES")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t flen, olen = 0;
      long removed = 0, miss = 0, failc = 0, nscan = 0;
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
          size_t take = flen;
          nscan++;
          if (take >= sizeof path) take = sizeof path - 1;
          memcpy(path, start, take);
          path[take] = 0;
          if (cubalc_host_rm(path, &hr) != 0) {
            failc++;
            if (failc == 1 && hr.err[0]) {
              var_set_str(vm, "LAST_ERR", hr.err);
              var_set_str(vm, "ERR", hr.err);
            }
          } else if (hr.n > 0) {
            if (removed > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + take < sizeof out) {
              memcpy(out + olen, path, take);
              olen += take;
            }
            out[olen] = 0;
            removed++;
          } else {
            miss++;
          }
        }
        if (*p == '\n') p++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = removed;
      var_set_num(vm, "LAST_N", removed);
      var_set_str(vm, "RMALL", out);
      var_set_str(vm, "UNLINKALL", out);
      var_set_str(vm, "DELETEALL", out);
      var_set_num(vm, "RMALL_N", removed);
      var_set_num(vm, "UNLINKALL_N", removed);
      var_set_num(vm, "DELETEALL_N", removed);
      var_set_num(vm, "RMALL_MISS", miss);
      var_set_num(vm, "RMALL_FAIL", failc);
      var_set_num(vm, "RMALL_SCAN", nscan);
      var_set_num(vm, "OK", failc == 0 ? 1 : 0);
      bump(vm); return 1;
    }
    /* SYS TOUCHALL|ENSUREALL|CREATEALL [bag]
     * — create empty / refresh mtime for every path in a newline bag.
     * Soft continue on failure (dirs etc.); LAST = succeeded paths.
     * LAST_N / TOUCHALL_N = success count; TOUCHALL_NEW (created) · TOUCHALL_UPD
     * (mtime refresh) · TOUCHALL_FAIL · TOUCHALL_SCAN. OK=1 when FAIL==0.
     * Usability: lease/marker stamps without EACH+TOUCH glue (dual of RMALL). */
    if (kw(&L->cur,"TOUCHALL") || kw(&L->cur,"ENSUREALL") || kw(&L->cur,"CREATEALL") ||
        kw(&L->cur,"TOUCHFILES") || kw(&L->cur,"BULKTOUCH") || kw(&L->cur,"STAMPALL") ||
        kw(&L->cur,"MARKALL") || kw(&L->cur,"ENSUREFILES")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t flen, olen = 0;
      long okc = 0, newc = 0, updc = 0, failc = 0, nscan = 0;
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
          size_t take = flen;
          nscan++;
          if (take >= sizeof path) take = sizeof path - 1;
          memcpy(path, start, take);
          path[take] = 0;
          if (cubalc_host_touch(path, &hr) != 0) {
            failc++;
            if (failc == 1 && hr.err[0]) {
              var_set_str(vm, "LAST_ERR", hr.err);
              var_set_str(vm, "ERR", hr.err);
            }
          } else {
            if (hr.n > 0) newc++;
            else updc++;
            if (okc > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + take < sizeof out) {
              memcpy(out + olen, path, take);
              olen += take;
            }
            out[olen] = 0;
            okc++;
          }
        }
        if (*p == '\n') p++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = okc;
      var_set_num(vm, "LAST_N", okc);
      var_set_str(vm, "TOUCHALL", out);
      var_set_str(vm, "ENSUREALL", out);
      var_set_str(vm, "CREATEALL", out);
      var_set_num(vm, "TOUCHALL_N", okc);
      var_set_num(vm, "ENSUREALL_N", okc);
      var_set_num(vm, "CREATEALL_N", okc);
      var_set_num(vm, "TOUCHALL_NEW", newc);
      var_set_num(vm, "TOUCHALL_UPD", updc);
      var_set_num(vm, "TOUCHALL_FAIL", failc);
      var_set_num(vm, "TOUCHALL_SCAN", nscan);
      var_set_num(vm, "OK", failc == 0 ? 1 : 0);
      bump(vm); return 1;
    }
    /* SYS COPYALL|CPALL|BULKCOPY bag dest_dir
     * — copy every regular-file path in bag into dest_dir (basename join).
     * Soft continue on miss/fail. LAST = dest paths written; LAST_N = count.
     * COPYALL_N count · COPYALL_BYTES total · COPYALL_MISS · COPYALL_FAIL ·
     * COPYALL_SCAN · COPYALL_DEST. One-arg form: dest_dir uses LAST bag.
     * Usability: snapshot/backup path bags without EACH+COPY glue. */
    if (kw(&L->cur,"COPYALL") || kw(&L->cur,"CPALL") || kw(&L->cur,"BULKCOPY") ||
        kw(&L->cur,"COPYFILES") || kw(&L->cur,"CPFILES") || kw(&L->cur,"BACKUPALL") ||
        kw(&L->cur,"SNAPSHOTALL") || kw(&L->cur,"FILECOPYALL")){
      char bag[CUBALC_HOST_STR_MAX], dest[512], out[CUBALC_HOST_STR_MAX];
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t flen, olen = 0;
      long okc = 0, miss = 0, failc = 0, nscan = 0, bytes = 0;
      cubalc_host_result hr, jr;
      lex_next(L);
      bag[0] = 0; dest[0] = 0; out[0] = 0; a[0] = 0; b[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS COPYALL [bag] dest_dir");
        return -1;
      }
      if (resolve_str_arg(vm, L, b, sizeof b) == 0) {
        snprintf(bag, sizeof bag, "%s", a);
        snprintf(dest, sizeof dest, "%s", b);
      } else {
        snprintf(bag, sizeof bag, "%s", vm->last_str);
        snprintf(dest, sizeof dest, "%s", a);
      }
      if (!dest[0]) {
        fail(vm, "SYS COPYALL [bag] dest_dir");
        return -1;
      }
      p = bag;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen > 0) {
          char path[512], base[256], dstpath[512];
          size_t take = flen;
          const char *slash;
          nscan++;
          if (take >= sizeof path) take = sizeof path - 1;
          memcpy(path, start, take);
          path[take] = 0;
          /* basename */
          slash = strrchr(path, '/');
          if (slash && slash[1])
            snprintf(base, sizeof base, "%s", slash + 1);
          else
            snprintf(base, sizeof base, "%s", path);
          if (!base[0] || strcmp(base, ".") == 0 || strcmp(base, "..") == 0) {
            failc++;
          } else if (cubalc_host_join(dest, base, &jr) != 0) {
            failc++;
            if (failc == 1 && jr.err[0]) {
              var_set_str(vm, "LAST_ERR", jr.err);
              var_set_str(vm, "ERR", jr.err);
            }
          } else {
            snprintf(dstpath, sizeof dstpath, "%s", jr.str);
            if (cubalc_host_copy(path, dstpath, &hr) != 0) {
              /* soft miss src → miss; dirs/other → fail */
              if (!cubalc_host_exists(path) ||
                  (hr.err[0] && strstr(hr.err, "missing"))) {
                miss++;
              } else {
                failc++;
                if (failc == 1 && hr.err[0]) {
                  var_set_str(vm, "LAST_ERR", hr.err);
                  var_set_str(vm, "ERR", hr.err);
                }
              }
            } else {
              size_t dlen = strlen(dstpath);
              if (okc > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + dlen < sizeof out) {
                memcpy(out + olen, dstpath, dlen);
                olen += dlen;
              }
              out[olen] = 0;
              okc++;
              bytes += hr.n;
            }
          }
        }
        if (*p == '\n') p++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = okc;
      var_set_num(vm, "LAST_N", okc);
      var_set_str(vm, "COPYALL", out);
      var_set_str(vm, "CPALL", out);
      var_set_str(vm, "COPYALL_DEST", dest);
      var_set_num(vm, "COPYALL_N", okc);
      var_set_num(vm, "CPALL_N", okc);
      var_set_num(vm, "COPYALL_BYTES", bytes);
      var_set_num(vm, "COPYALL_MISS", miss);
      var_set_num(vm, "COPYALL_FAIL", failc);
      var_set_num(vm, "COPYALL_SCAN", nscan);
      var_set_num(vm, "OK", failc == 0 ? 1 : 0);
      bump(vm); return 1;
    }
    /* SYS MKDIRALL|MAKEDIRALL|ENSUREDIRS [bag]
     * — mkdir -p every path in a newline bag (layout before WRITE/TOUCHALL).
     * Soft continue on fail; OK if already a directory (host_mkdir).
     * LAST = succeeded paths; LAST_N / MKDIRALL_N; MKDIRALL_FAIL · MKDIRALL_SCAN.
     * Usability: multi-dir plate trees without EACH+MKDIR glue. */
    if (kw(&L->cur,"MKDIRALL") || kw(&L->cur,"MAKEDIRALL") || kw(&L->cur,"ENSUREDIRS") ||
        kw(&L->cur,"MKDIRS") || kw(&L->cur,"BULKMKDIR") || kw(&L->cur,"CREATEDIRS") ||
        kw(&L->cur,"ENSURE_DIRS") || kw(&L->cur,"MDALL")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t flen, olen = 0;
      long okc = 0, failc = 0, nscan = 0;
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
          size_t take = flen;
          nscan++;
          if (take >= sizeof path) take = sizeof path - 1;
          memcpy(path, start, take);
          path[take] = 0;
          if (cubalc_host_mkdir(path, &hr) != 0) {
            failc++;
            if (failc == 1 && hr.err[0]) {
              var_set_str(vm, "LAST_ERR", hr.err);
              var_set_str(vm, "ERR", hr.err);
            }
          } else {
            if (okc > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + take < sizeof out) {
              memcpy(out + olen, path, take);
              olen += take;
            }
            out[olen] = 0;
            okc++;
          }
        }
        if (*p == '\n') p++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = okc;
      var_set_num(vm, "LAST_N", okc);
      var_set_str(vm, "MKDIRALL", out);
      var_set_str(vm, "ENSUREDIRS", out);
      var_set_str(vm, "MKDIRS", out);
      var_set_num(vm, "MKDIRALL_N", okc);
      var_set_num(vm, "ENSUREDIRS_N", okc);
      var_set_num(vm, "MKDIRS_N", okc);
      var_set_num(vm, "MKDIRALL_FAIL", failc);
      var_set_num(vm, "MKDIRALL_SCAN", nscan);
      var_set_num(vm, "OK", failc == 0 ? 1 : 0);
      bump(vm); return 1;
    }
    /* SYS MOVEALL|MVALL|BULKMOVE bag dest_dir
     * — move/rename every path in bag into dest_dir (basename join).
     * Soft miss on missing src; soft continue on fail. LAST = dest paths.
     * MOVEALL_N · MOVEALL_MISS · MOVEALL_FAIL · MOVEALL_SCAN · MOVEALL_DEST.
     * One-arg: dest_dir uses LAST bag.
     * Usability: archive/reclaim relocate without EACH+RENAME glue (dual COPYALL). */
    if (kw(&L->cur,"MOVEALL") || kw(&L->cur,"MVALL") || kw(&L->cur,"BULKMOVE") ||
        kw(&L->cur,"RENAMEALL") || kw(&L->cur,"ARCHIVEALL") || kw(&L->cur,"RELOCATE") ||
        kw(&L->cur,"MOVEFILES") || kw(&L->cur,"MVFILES")){
      char bag[CUBALC_HOST_STR_MAX], dest[512], out[CUBALC_HOST_STR_MAX];
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t flen, olen = 0;
      long okc = 0, miss = 0, failc = 0, nscan = 0;
      cubalc_host_result hr, jr;
      lex_next(L);
      bag[0] = 0; dest[0] = 0; out[0] = 0; a[0] = 0; b[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS MOVEALL [bag] dest_dir");
        return -1;
      }
      if (resolve_str_arg(vm, L, b, sizeof b) == 0) {
        snprintf(bag, sizeof bag, "%s", a);
        snprintf(dest, sizeof dest, "%s", b);
      } else {
        snprintf(bag, sizeof bag, "%s", vm->last_str);
        snprintf(dest, sizeof dest, "%s", a);
      }
      if (!dest[0]) {
        fail(vm, "SYS MOVEALL [bag] dest_dir");
        return -1;
      }
      p = bag;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen > 0) {
          char path[512], base[256], dstpath[512];
          size_t take = flen;
          const char *slash;
          nscan++;
          if (take >= sizeof path) take = sizeof path - 1;
          memcpy(path, start, take);
          path[take] = 0;
          slash = strrchr(path, '/');
          if (slash && slash[1])
            snprintf(base, sizeof base, "%s", slash + 1);
          else
            snprintf(base, sizeof base, "%s", path);
          if (!base[0] || strcmp(base, ".") == 0 || strcmp(base, "..") == 0) {
            failc++;
          } else if (!cubalc_host_exists(path)) {
            miss++;
          } else if (cubalc_host_join(dest, base, &jr) != 0) {
            failc++;
            if (failc == 1 && jr.err[0]) {
              var_set_str(vm, "LAST_ERR", jr.err);
              var_set_str(vm, "ERR", jr.err);
            }
          } else {
            snprintf(dstpath, sizeof dstpath, "%s", jr.str);
            if (cubalc_host_rename(path, dstpath, &hr) != 0) {
              failc++;
              if (failc == 1 && hr.err[0]) {
                var_set_str(vm, "LAST_ERR", hr.err);
                var_set_str(vm, "ERR", hr.err);
              }
            } else {
              size_t dlen = strlen(dstpath);
              if (okc > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + dlen < sizeof out) {
                memcpy(out + olen, dstpath, dlen);
                olen += dlen;
              }
              out[olen] = 0;
              okc++;
            }
          }
        }
        if (*p == '\n') p++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = okc;
      var_set_num(vm, "LAST_N", okc);
      var_set_str(vm, "MOVEALL", out);
      var_set_str(vm, "MVALL", out);
      var_set_str(vm, "RENAMEALL", out);
      var_set_str(vm, "MOVEALL_DEST", dest);
      var_set_num(vm, "MOVEALL_N", okc);
      var_set_num(vm, "MVALL_N", okc);
      var_set_num(vm, "RENAMEALL_N", okc);
      var_set_num(vm, "MOVEALL_MISS", miss);
      var_set_num(vm, "MOVEALL_FAIL", failc);
      var_set_num(vm, "MOVEALL_SCAN", nscan);
      var_set_num(vm, "OK", failc == 0 ? 1 : 0);
      bump(vm); return 1;
    }
    /* SYS WALK|TREEGLOB|FINDALL root [pattern]
     * — recursive full paths under root whose basename matches pattern (*?[]).
     * Recurses into all dirs (find -name style). Soft miss non-dir → OK=0.
     * Do NOT use FIND alone (reserved for string index SYS FIND).
     * LAST = paths; LAST_N / WALK_N; WALK_FILES · WALK_DIRS · WALK_ROOT · WALK_PAT.
     * Usability: deep plate discovery beyond non-recursive PATHGLOB. */
    if (kw(&L->cur,"WALK") || kw(&L->cur,"FINDALL") || kw(&L->cur,"TREEGLOB") ||
        kw(&L->cur,"RGLOB") || kw(&L->cur,"WALKDIR") || kw(&L->cur,"FINDFILES") ||
        kw(&L->cur,"TREEWALK") || kw(&L->cur,"RWALK") || kw(&L->cur,"DEEPGLOB")){
      char root[512], pat[256], out[CUBALC_HOST_STR_MAX];
      size_t olen = 0;
      long kept = 0, nfiles = 0, ndirs = 0;
      cubalc_host_result hr;
      lex_next(L);
      root[0] = 0; pat[0] = 0; out[0] = 0;
      snprintf(pat, sizeof pat, "%s", "*");
      if (resolve_str_arg(vm, L, root, sizeof root) != 0) {
        fail(vm, "SYS WALK root [pattern]");
        return -1;
      }
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, pat, sizeof pat) != 0)
          snprintf(pat, sizeof pat, "%s", "*");
        if (!pat[0]) snprintf(pat, sizeof pat, "%s", "*");
      }
      if (cubalc_host_path_kind(root, &hr) != 0 || hr.code != 2) {
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "WALK_N", 0);
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "WALK", "");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "walk: not a directory");
          var_set_str(vm, "ERR", "walk: not a directory");
        }
        bump(vm); return 1;
      }
      cubalc_sys_walk_rec(root, pat, out, sizeof out, &olen, &kept, 0, &ndirs, &nfiles);
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_str(vm, "WALK", out);
      var_set_str(vm, "FINDALL", out);
      var_set_str(vm, "TREEGLOB", out);
      var_set_str(vm, "WALK_ROOT", root);
      var_set_str(vm, "WALK_PAT", pat);
      var_set_num(vm, "WALK_N", kept);
      var_set_num(vm, "FINDALL_N", kept);
      var_set_num(vm, "TREEGLOB_N", kept);
      var_set_num(vm, "WALK_FILES", nfiles);
      var_set_num(vm, "WALK_DIRS", ndirs);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS EQFILE|SAMEFILE|CMPFILE path_a path_b
     * — 1 if both regular files have identical content; 0 if differ.
     * Soft miss / non-file → OK=0 LAST_N=0. Streamed byte compare (no full load).
     * LAST_N / EQFILE = 0|1; EQFILE_BYTES = size when equal (else -1 or 0).
     * Usability: verify COPYALL/plates without shell cmp. */
    if (kw(&L->cur,"EQFILE") || kw(&L->cur,"SAMEFILE") || kw(&L->cur,"CMPFILE") ||
        kw(&L->cur,"FILEEQ") || kw(&L->cur,"IDENTICAL") || kw(&L->cur,"SAME_FILE") ||
        kw(&L->cur,"FILES_EQ") || kw(&L->cur,"EQPATH")){
      char a[512], b[512];
      cubalc_host_result ha, hb;
      FILE *fa = NULL, *fb = NULL;
      long eq = 0, bytes = 0;
      int ok = 0;
      lex_next(L);
      a[0] = 0; b[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS EQFILE path_a path_b");
        return -1;
      }
      if (resolve_str_arg(vm, L, b, sizeof b) != 0) {
        fail(vm, "SYS EQFILE path_a path_b");
        return -1;
      }
      if (cubalc_host_path_kind(a, &ha) != 0 || ha.code != 1 ||
          cubalc_host_path_kind(b, &hb) != 0 || hb.code != 1) {
        /* miss or not regular file */
        ok = 0;
        eq = 0;
        bytes = 0;
      } else if (ha.n != hb.n) {
        ok = 1;
        eq = 0;
        bytes = 0;
      } else if (ha.n == 0) {
        ok = 1;
        eq = 1;
        bytes = 0;
      } else {
        fa = fopen(a, "rb");
        fb = fopen(b, "rb");
        if (!fa || !fb) {
          ok = 0;
          eq = 0;
          bytes = 0;
          if (!fa && !fb) {
            var_set_str(vm, "LAST_ERR", "eqfile: open fail");
            var_set_str(vm, "ERR", "eqfile: open fail");
          }
        } else {
          char ba[4096], bb[4096];
          size_t na, nb;
          int differ = 0;
          long total = 0;
          ok = 1;
          while (!differ) {
            na = fread(ba, 1, sizeof ba, fa);
            nb = fread(bb, 1, sizeof bb, fb);
            if (na != nb) { differ = 1; break; }
            if (na == 0) break;
            if (memcmp(ba, bb, na) != 0) { differ = 1; break; }
            total += (long)na;
          }
          eq = differ ? 0 : 1;
          bytes = eq ? total : 0;
        }
        if (fa) fclose(fa);
        if (fb) fclose(fb);
      }
      vm->last_n = eq;
      var_set_num(vm, "LAST_N", eq);
      var_set_num(vm, "EQFILE", eq);
      var_set_num(vm, "SAMEFILE", eq);
      var_set_num(vm, "CMPFILE", eq);
      var_set_num(vm, "EQFILE_BYTES", bytes);
      var_set_num(vm, "OK", ok ? 1 : 0);
      {
        char buf[16];
        snprintf(buf, sizeof buf, "%ld", eq);
        var_set_str(vm, "LAST", buf);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      }
      bump(vm); return 1;
    }
    /* SYS SWAPFILES|FILESWAP|BIDIRSWAP path_a path_b
     * — bidirectional content exchange of two files/plates (any text/bag data).
     * Soft miss if either path missing/unreadable. Uses tmp under same dir as a.
     * LAST_N = bytes swapped (size of a after = former b); SWAPFILES_A/B bytes.
     * Usability: true duplex plate swap without READ+WRITE thrash. */
    if (kw(&L->cur,"SWAPFILES") || kw(&L->cur,"FILESWAP") || kw(&L->cur,"BIDIRSWAP") ||
        kw(&L->cur,"SWAPPLATE") || kw(&L->cur,"EXCHANGEFILES") || kw(&L->cur,"XFERSWAP")){
      char a[512], b[512], tmp[576];
      cubalc_host_result ra, rb, rt;
      lex_next(L);
      a[0] = 0; b[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0 ||
          resolve_str_arg(vm, L, b, sizeof b) != 0) {
        fail(vm, "SYS SWAPFILES path_a path_b");
        return -1;
      }
      if (cubalc_host_read(a, &ra) != 0 || cubalc_host_read(b, &rb) != 0) {
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SWAPFILES_N", 0);
        var_set_num(vm, "SWAPFILES_A", 0);
        var_set_num(vm, "SWAPFILES_B", 0);
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "SWAPFILES: read fail");
        var_set_str(vm, "ERR", "SWAPFILES: read fail");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        vm->last_n = 0;
        bump(vm); return 1;
      }
      snprintf(tmp, sizeof tmp, "%s.cubalc_swap_%ld", a, (long)getpid());
      if (cubalc_host_write(tmp, ra.str, &rt) != 0 ||
          cubalc_host_write(a, rb.str, &rt) != 0 ||
          cubalc_host_write(b, ra.str, &rt) != 0) {
        remove(tmp);
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SWAPFILES_N", 0);
        var_set_str(vm, "LAST_ERR", "SWAPFILES: write fail");
        var_set_str(vm, "ERR", "SWAPFILES: write fail");
        var_set_str(vm, "LAST", "");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        vm->last_n = 0;
        bump(vm); return 1;
      }
      remove(tmp);
      var_set_num(vm, "OK", 1);
      var_set_num(vm, "LAST_N", (long)rb.n);
      var_set_num(vm, "SWAPFILES_N", (long)rb.n);
      var_set_num(vm, "SWAPFILES_A", (long)rb.n); /* a now holds former b */
      var_set_num(vm, "SWAPFILES_B", (long)ra.n); /* b now holds former a */
      var_set_str(vm, "LAST", rb.str); /* content now in a */
      snprintf(vm->last_str, sizeof vm->last_str, "%s", rb.str);
      vm->last_n = (long)rb.n;
      bump(vm); return 1;
    }
    /* SYS DUPLEX|BIDXFER|PUTGET path_out path_in [payload]
     * — bidirectional mailbox RTT: WRITE payload→out, READ in→LAST.
     * Any string/bag data (≤ HOST_STR_MAX). Two-arg: payload = prior LAST.
     * Soft if out write or in read fails. DUPLEX_TX / DUPLEX_RX byte counts.
     * Usability: agent request/response via plates without shell pipes. */
    if (kw(&L->cur,"DUPLEX") || kw(&L->cur,"BIDXFER") || kw(&L->cur,"PUTGET") ||
        kw(&L->cur,"FLOWDATA") || kw(&L->cur,"MAILBOX") || kw(&L->cur,"XFER") ||
        kw(&L->cur,"REQRESP") || kw(&L->cur,"BIDIR")){
      char outp[512], inp[512], payload[CUBALC_HOST_STR_MAX];
      cubalc_host_result wr, rr;
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX], c[CUBALC_HOST_STR_MAX];
      lex_next(L);
      outp[0] = 0; inp[0] = 0; payload[0] = 0; a[0] = 0; b[0] = 0; c[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS DUPLEX path_out path_in [payload]");
        return -1;
      }
      if (resolve_str_arg(vm, L, b, sizeof b) != 0) {
        fail(vm, "SYS DUPLEX path_out path_in [payload]");
        return -1;
      }
      if (resolve_str_arg(vm, L, c, sizeof c) == 0) {
        snprintf(outp, sizeof outp, "%s", a);
        snprintf(inp, sizeof inp, "%s", b);
        snprintf(payload, sizeof payload, "%s", c);
      } else {
        /* two-arg: payload = LAST */
        snprintf(outp, sizeof outp, "%s", a);
        snprintf(inp, sizeof inp, "%s", b);
        snprintf(payload, sizeof payload, "%s", vm->last_str);
      }
      if (cubalc_host_write(outp, payload, &wr) != 0) {
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DUPLEX_TX", 0);
        var_set_num(vm, "DUPLEX_RX", 0);
        var_set_num(vm, "BIDXFER_TX", 0);
        var_set_num(vm, "BIDXFER_RX", 0);
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "DUPLEX: write out fail");
        var_set_str(vm, "ERR", wr.err[0] ? wr.err : "DUPLEX: write out fail");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        vm->last_n = 0;
        bump(vm); return 1;
      }
      if (cubalc_host_read(inp, &rr) != 0) {
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DUPLEX_TX", (long)wr.n);
        var_set_num(vm, "DUPLEX_RX", 0);
        var_set_num(vm, "BIDXFER_TX", (long)wr.n);
        var_set_num(vm, "BIDXFER_RX", 0);
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", rr.err[0] ? rr.err : "DUPLEX: read in fail");
        var_set_str(vm, "ERR", rr.err[0] ? rr.err : "DUPLEX: read in fail");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        vm->last_n = 0;
        bump(vm); return 1;
      }
      var_set_num(vm, "OK", 1);
      var_set_num(vm, "LAST_N", (long)rr.n);
      var_set_num(vm, "DUPLEX_TX", (long)wr.n);
      var_set_num(vm, "DUPLEX_RX", (long)rr.n);
      var_set_num(vm, "BIDXFER_TX", (long)wr.n);
      var_set_num(vm, "BIDXFER_RX", (long)rr.n);
      var_set_str(vm, "LAST", rr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", rr.str);
      vm->last_n = (long)rr.n;
      bump(vm); return 1;
    }
  return 0;
}
