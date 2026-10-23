/* CubalC lang — lang_ops_sys_path_sync.c
 * SYS FSYNC · TRUNCATE · LOCK · CLAIM · RM · RMDIR
 * Peel from path_fs. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_sys_path_sync.h"
#include "lang/cubalc_lang_ops_sys_path_writeout.h"
#include "lang/cubalc_lang_ops_sys_path_tree.h"
#include "lang/cubalc_lang_ops_sys_codec.h"
#include "lang/cubalc_lang_ops_sys_time_walk.h"
#include "lang/cubalc_lang_ops_sys_plate_expand.h"
#include "lang/cubalc_lang_ops_sys_cli.h"
#include "lang/cubalc_lang_ops_core_help.h"
#include "cubalc_hostops.h"
#include "cubalc_async.h"
#include "cubalc_smx.h"
#include "cubalc_law.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <unistd.h>
#  include <fcntl.h>
#  include <sys/stat.h>
#endif
int cubalc_lang_ops_sys_try_path_sync(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. SYS already consumed. */
  /* SYS FSYNC|SYNCFILE|FDATASYNC path — flush file data+metadata to disk.
   * LAST = path; LAST_N = 1 success / 0 soft miss; FSYNC_N mirrors LAST_N.
   * Opens O_RDONLY, fsync(fd), close. Soft miss on empty/open/fsync fail.
   * Usability: durable plate flush after WRITE/WRITEATOMIC without shell sync. */
  if (kw(&L->cur,"FSYNC") || kw(&L->cur,"SYNCFILE") || kw(&L->cur,"FDATASYNC") ||
      kw(&L->cur,"FILESYNC") || kw(&L->cur,"FSYNCFILE") || kw(&L->cur,"DURABLE") ||
      kw(&L->cur,"FLUSHFILE")){
    char path[512];
    int fd = -1, ok = 0;
    lex_next(L);
    path[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0)
      path[0] = 0;
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "FSYNC_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "FSYNC: empty path");
      var_set_str(vm, "ERR", "FSYNC: empty path");
      vm->last_n = 0;
      vm->last_str[0] = 0;
      bump(vm); return 1;
    }
#if !defined(CUBALC_OS_WINDOWS)
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
#if defined(F_FULLFSYNC)
      /* macOS durable flush */
      if (fcntl(fd, F_FULLFSYNC) == 0)
        ok = 1;
      else if (fsync(fd) == 0)
        ok = 1;
#else
      if (fsync(fd) == 0)
        ok = 1;
#endif
      close(fd);
    }
#else
    /* Windows: best-effort via _commit if available — soft OK skip */
    ok = 0;
#endif
    var_set_str(vm, "LAST", path);
    var_set_str(vm, "FSYNC", path);
    var_set_str(vm, "SYNCFILE", path);
    var_set_str(vm, "FSYNC_PATH", path);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
    if (ok) {
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "FSYNC_N", 1);
      var_set_num(vm, "OK", 1);
    } else {
      char err[160];
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "FSYNC_N", 0);
      var_set_num(vm, "OK", 0);
      snprintf(err, sizeof err, "FSYNC: fail '%s'", path);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
    }
    bump(vm); return 1;
  }
  /* SYS TRUNCATE|FTRUNCATE|FILETRUNC path [nbytes]
   * — resize regular file to nbytes (default 0 = empty plate, keep inode/mode).
   * LAST = path; LAST_N / TRUNCATE_N = final size; soft miss empty/open/fail.
   * Not TRUNCALL (bag field clip). Cap nbytes at 1<<30.
   * Usability: clear/resize multi-agent plates without RM+TOUCH losing perms. */
  if (kw(&L->cur,"TRUNCATE") || kw(&L->cur,"FTRUNCATE") || kw(&L->cur,"FILETRUNC") ||
      kw(&L->cur,"TRUNCFILE") || kw(&L->cur,"SHRINKFILE") || kw(&L->cur,"RESIZEFILE") ||
      kw(&L->cur,"SETSIZE") || kw(&L->cur,"FILESIZESET") || kw(&L->cur,"EMPTYFILE") ||
      kw(&L->cur,"CLEARFILE")){
    char path[512];
    long nbytes = 0;
    int fd = -1, ok = 0;
    int is_clear = (kw(&L->cur,"EMPTYFILE") || kw(&L->cur,"CLEARFILE"));
    lex_next(L);
    path[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0)
      path[0] = 0;
    if (!is_clear && (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
                      L->cur.kind == TK_LPAREN || L->cur.kind == TK_IDENT)) {
      /* optional size: bare num | num-var | expr — not string path leftovers */
      if (L->cur.kind == TK_IDENT) {
        Var *v = var_get(vm, L->cur.text, 0);
        if (v && !v->is_str) {
          nbytes = parse_expr(vm, L);
        }
        /* else leave 0 — next form's IDENT */
      } else {
        nbytes = parse_expr(vm, L);
      }
    }
    if (nbytes < 0) nbytes = 0;
    if (nbytes > (1L << 30)) nbytes = (1L << 30);
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "TRUNCATE_N", 0);
      var_set_num(vm, "FTRUNCATE_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "TRUNCATE: empty path");
      var_set_str(vm, "ERR", "TRUNCATE: empty path");
      vm->last_n = 0;
      vm->last_str[0] = 0;
      bump(vm); return 1;
    }
#if !defined(CUBALC_OS_WINDOWS)
    fd = open(path, O_WRONLY);
    if (fd >= 0) {
      if (ftruncate(fd, (off_t)nbytes) == 0)
        ok = 1;
      close(fd);
    }
#else
    ok = 0;
#endif
    var_set_str(vm, "LAST", path);
    var_set_str(vm, "TRUNCATE", path);
    var_set_str(vm, "FTRUNCATE", path);
    var_set_str(vm, "TRUNCATE_PATH", path);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
    if (ok) {
      vm->last_n = nbytes;
      var_set_num(vm, "LAST_N", nbytes);
      var_set_num(vm, "TRUNCATE_N", nbytes);
      var_set_num(vm, "FTRUNCATE_N", nbytes);
      var_set_num(vm, "OK", 1);
    } else {
      char err[160];
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "TRUNCATE_N", 0);
      var_set_num(vm, "FTRUNCATE_N", 0);
      var_set_num(vm, "OK", 0);
      snprintf(err, sizeof err, "TRUNCATE: fail '%s'", path);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
    }
    bump(vm); return 1;
  }
  /* SYS DIRSYNC|FSYNCDIR|SYNCDIR path
   * — fsync a directory (or parent of a file) so create/rename/unlink is durable.
   * If path is a directory → fsync it; if a file → fsync its parent (dirname).
   * LAST = directory synced; LAST_N / DIRSYNC_N = 1|0; soft miss empty/open/fsync.
   * Usability: complete WRITEATOMIC durability without shell — file FSYNC + dir fsync. */
  if (kw(&L->cur,"DIRSYNC") || kw(&L->cur,"FSYNCDIR") || kw(&L->cur,"SYNCDIR") ||
      kw(&L->cur,"SYNCFOLDER") || kw(&L->cur,"FSYNCPARENT") || kw(&L->cur,"DIRFSYNC") ||
      kw(&L->cur,"DURABLEDIR") || kw(&L->cur,"SYNCPARENT") || kw(&L->cur,"PARENTSYNC") ||
      kw(&L->cur,"FSYNC_DIR")){
    char path[512], dir[512];
    int fd = -1, ok = 0, force_parent = 0;
    struct stat st;
    lex_next(L);
    if (kw(&L->cur,"PARENT") || kw(&L->cur,"OF") || kw(&L->cur,"UP") ||
        kw(&L->cur,"DIRNAME")){
      force_parent = 1;
      lex_next(L);
    }
    path[0] = 0; dir[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0)
      path[0] = 0;
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "DIRSYNC_N", 0);
      var_set_num(vm, "FSYNCDIR_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "DIRSYNC: empty path");
      var_set_str(vm, "ERR", "DIRSYNC: empty path");
      vm->last_n = 0;
      vm->last_str[0] = 0;
      bump(vm); return 1;
    }
    /* resolve target directory */
    {
      int is_dir = 0;
      if (!force_parent && stat(path, &st) == 0 && S_ISDIR(st.st_mode))
        is_dir = 1;
      if (is_dir) {
        snprintf(dir, sizeof dir, "%s", path);
      } else {
        /* dirname of path */
        char tmp[512];
        char *slash;
        size_t n;
        snprintf(tmp, sizeof tmp, "%s", path);
        n = strlen(tmp);
        while (n > 1 && tmp[n - 1] == '/') {
          tmp[n - 1] = 0;
          n--;
        }
        slash = strrchr(tmp, '/');
        if (!slash) {
          snprintf(dir, sizeof dir, "%s", ".");
        } else if (slash == tmp) {
          snprintf(dir, sizeof dir, "%s", "/");
        } else {
          *slash = 0;
          snprintf(dir, sizeof dir, "%s", tmp);
        }
      }
    }
#if !defined(CUBALC_OS_WINDOWS)
    fd = open(dir, O_RDONLY);
    if (fd >= 0) {
#if defined(F_FULLFSYNC)
      if (fcntl(fd, F_FULLFSYNC) == 0)
        ok = 1;
      else if (fsync(fd) == 0)
        ok = 1;
#else
      if (fsync(fd) == 0)
        ok = 1;
#endif
      close(fd);
    }
#else
    ok = 0;
#endif
    var_set_str(vm, "LAST", dir);
    var_set_str(vm, "DIRSYNC", dir);
    var_set_str(vm, "FSYNCDIR", dir);
    var_set_str(vm, "DIRSYNC_PATH", dir);
    var_set_str(vm, "DIRSYNC_SRC", path);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", dir);
    if (ok) {
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "DIRSYNC_N", 1);
      var_set_num(vm, "FSYNCDIR_N", 1);
      var_set_num(vm, "OK", 1);
    } else {
      char err[160];
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "DIRSYNC_N", 0);
      var_set_num(vm, "FSYNCDIR_N", 0);
      var_set_num(vm, "OK", 0);
      snprintf(err, sizeof err, "DIRSYNC: fail '%s'", dir);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
    }
    bump(vm); return 1;
  }
  /* SYS LOCKFILE|FLOCK|MUTEX path [timeout_ms]
   * — exclusive agent lock via path.lock (O_CREAT|O_EXCL + pid body).
   * Poll until acquired or timeout (default 30000, cap 120s, step 50ms).
   * LAST = lock path; LAST_N / LOCKFILE_HIT 1|0; LOCKFILE_MS elapsed.
   * Soft timeout / empty path → OK=0.
   * SYS UNLOCKFILE|FUNLOCK|RELEASELOCK path — remove path.lock (soft miss OK).
   * Usability: multi-agent exclusive plate critical sections without shell flock. */
  if (kw(&L->cur,"LOCKFILE") || kw(&L->cur,"FLOCK") || kw(&L->cur,"MUTEX") ||
      kw(&L->cur,"FILELOCK") || kw(&L->cur,"LOCKF") || kw(&L->cur,"ACQUIRELOCK") ||
      kw(&L->cur,"TAKELOCK") || kw(&L->cur,"LOCKPATH") ||
      kw(&L->cur,"UNLOCKFILE") || kw(&L->cur,"FUNLOCK") || kw(&L->cur,"RELEASELOCK") ||
      kw(&L->cur,"FILEUNLOCK") || kw(&L->cur,"UNLOCKF") || kw(&L->cur,"DROPLOCK") ||
      kw(&L->cur,"FREELOCK") || kw(&L->cur,"UNLOCKPATH")){
    char path[512], lockp[560], a[CUBALC_HOST_STR_MAX], body[64];
    long timeout_ms = 30000, elapsed = 0, hit = 0, pid;
    int is_unlock = (kw(&L->cur,"UNLOCKFILE") || kw(&L->cur,"FUNLOCK") ||
                    kw(&L->cur,"RELEASELOCK") || kw(&L->cur,"FILEUNLOCK") ||
                    kw(&L->cur,"UNLOCKF") || kw(&L->cur,"DROPLOCK") ||
                    kw(&L->cur,"FREELOCK") || kw(&L->cur,"UNLOCKPATH"));
    struct timespec t0, t1, sl;
    lex_next(L);
    path[0] = 0; lockp[0] = 0; a[0] = 0; body[0] = 0;
    if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
      fail(vm, is_unlock ? "SYS UNLOCKFILE path" : "SYS LOCKFILE path [timeout_ms]");
      return -1;
    }
    snprintf(path, sizeof path, "%s", a);
    if (!is_unlock) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_IDENT) {
        long t = parse_expr(vm, L);
        if (kw(&L->cur,"MS") || kw(&L->cur,"MILLIS") || kw(&L->cur,"MILLISECONDS"))
          lex_next(L);
        timeout_ms = t;
      } else if (kw(&L->cur,"MS") || kw(&L->cur,"MILLIS")) {
        lex_next(L);
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
            L->cur.kind == TK_IDENT)
          timeout_ms = parse_expr(vm, L);
      }
      if (timeout_ms < 0) timeout_ms = 0;
      if (timeout_ms > 120000) timeout_ms = 120000;
    }
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "LOCKFILE_HIT", 0);
      var_set_num(vm, "LOCKFILE_N", 0);
      var_set_num(vm, "LOCKFILE_MS", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", is_unlock ? "UNLOCKFILE: empty path" : "LOCKFILE: empty path");
      var_set_str(vm, "ERR", is_unlock ? "UNLOCKFILE: empty path" : "LOCKFILE: empty path");
      bump(vm); return 1;
    }
    /* lock path = path + ".lock" (if path already ends .lock, use as-is) */
    {
      size_t pl = strlen(path);
      if (pl >= 5 && strcmp(path + pl - 5, ".lock") == 0)
        snprintf(lockp, sizeof lockp, "%s", path);
      else
        snprintf(lockp, sizeof lockp, "%s.lock", path);
    }
#if defined(CUBALC_OS_WINDOWS)
    pid = 0;
#else
    pid = (long)getpid();
#endif
    snprintf(body, sizeof body, "%ld\n", pid);
    if (is_unlock) {
      cubalc_host_result hr;
      int existed = cubalc_host_exists(lockp);
      if (cubalc_host_rm(lockp, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "LOCKFILE_HIT", 0);
        var_set_num(vm, "UNLOCKFILE_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "UNLOCKFILE: rm fail");
        var_set_str(vm, "ERR", hr.err[0] ? hr.err : "UNLOCKFILE: rm fail");
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", lockp);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", lockp);
      vm->last_n = existed ? 1 : 0;
      var_set_num(vm, "LAST_N", existed ? 1 : 0);
      var_set_str(vm, "UNLOCKFILE", lockp);
      var_set_str(vm, "LOCKFILE", lockp);
      var_set_num(vm, "UNLOCKFILE_N", existed ? 1 : 0);
      var_set_num(vm, "LOCKFILE_HIT", 0);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* acquire: O_CREAT|O_EXCL exclusive create of lock file */
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (;;) {
#if defined(CUBALC_OS_WINDOWS)
      {
        /* Windows: try create exclusive */
        FILE *lf = fopen(lockp, "r");
        if (!lf) {
          cubalc_host_result wr;
          if (cubalc_host_write(lockp, body, &wr) == 0) {
            hit = 1;
            break;
          }
        } else {
          fclose(lf);
        }
      }
#else
      {
        int fd = open(lockp, O_CREAT | O_EXCL | O_WRONLY, 0644);
        if (fd >= 0) {
          size_t bl = strlen(body);
          ssize_t w = write(fd, body, bl);
          (void)w;
          close(fd);
          hit = 1;
          break;
        }
        /* EEXIST → contended; other errors soft-continue until timeout */
      }
#endif
      clock_gettime(CLOCK_MONOTONIC, &t1);
      elapsed = (t1.tv_sec - t0.tv_sec) * 1000L +
                (t1.tv_nsec - t0.tv_nsec) / 1000000L;
      if (elapsed >= timeout_ms)
        break;
      {
        long left = timeout_ms - elapsed;
        long step = left < 50 ? left : 50;
        if (step < 1) step = 1;
        sl.tv_sec = step / 1000;
        sl.tv_nsec = (step % 1000) * 1000000L;
        nanosleep(&sl, NULL);
      }
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    elapsed = (t1.tv_sec - t0.tv_sec) * 1000L +
              (t1.tv_nsec - t0.tv_nsec) / 1000000L;
    if (elapsed < 0) elapsed = 0;
    if (hit) {
      var_set_str(vm, "LAST", lockp);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", lockp);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_str(vm, "LOCKFILE", lockp);
      var_set_str(vm, "FLOCK", lockp);
      var_set_str(vm, "MUTEX", lockp);
      var_set_num(vm, "LOCKFILE_HIT", 1);
      var_set_num(vm, "LOCKFILE_N", 1);
      var_set_num(vm, "LOCKFILE_MS", elapsed);
      var_set_num(vm, "LOCKFILE_PID", pid);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    var_set_str(vm, "LAST", "");
    vm->last_str[0] = 0;
    vm->last_n = 0;
    var_set_num(vm, "LAST_N", 0);
    var_set_str(vm, "LOCKFILE", "");
    var_set_num(vm, "LOCKFILE_HIT", 0);
    var_set_num(vm, "LOCKFILE_N", 0);
    var_set_num(vm, "LOCKFILE_MS", elapsed);
    var_set_num(vm, "LOCKFILE_PID", 0);
    var_set_num(vm, "OK", 0);
    var_set_str(vm, "LAST_ERR", "LOCKFILE: timeout");
    var_set_str(vm, "ERR", "LOCKFILE: timeout");
    bump(vm); return 1;
  }
  /* SYS CLAIM|CLAIMFILE|CREATEEXCL path [body]
   * — exclusive create of path itself (O_CREAT|O_EXCL), not path.lock.
   * Default body = "<pid>\n". Soft miss if already exists → OK=0 HIT=0.
   * LAST = path on claim; CLAIM_HIT 1|0; CLAIM_PID. Empty path soft fail.
   * Usability: one-shot agent work-item / job-file claim without LOCKFILE
   * unlock protocol or shell mkdir race. Release with SYS RM path. */
  if (kw(&L->cur,"CLAIM") || kw(&L->cur,"CLAIMFILE") || kw(&L->cur,"CREATEEXCL") ||
      kw(&L->cur,"TOUCHEXCL") || kw(&L->cur,"EXCLCREATE") || kw(&L->cur,"TAKECLAIM") ||
      kw(&L->cur,"CLAIMPATH") || kw(&L->cur,"ONCEFILE") || kw(&L->cur,"EXCLTOUCH")){
    char path[512], body[CUBALC_HOST_STR_MAX], a[CUBALC_HOST_STR_MAX];
    long pid = 0;
    int hit = 0;
    lex_next(L);
    path[0] = 0; body[0] = 0; a[0] = 0;
    if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
      fail(vm, "SYS CLAIM path [body]");
      return -1;
    }
    snprintf(path, sizeof path, "%s", a);
    /* optional body */
    if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (resolve_str_arg(vm, L, body, sizeof body) != 0)
        body[0] = 0;
    }
#if defined(CUBALC_OS_WINDOWS)
    pid = 0;
#else
    pid = (long)getpid();
#endif
    if (!body[0])
      snprintf(body, sizeof body, "%ld\n", pid);
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "CLAIM_HIT", 0);
      var_set_num(vm, "CLAIM_N", 0);
      var_set_num(vm, "CLAIM_PID", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "CLAIM: empty path");
      var_set_str(vm, "ERR", "CLAIM: empty path");
      bump(vm); return 1;
    }
#if defined(CUBALC_OS_WINDOWS)
    {
      FILE *lf = fopen(path, "r");
      if (!lf) {
        cubalc_host_result wr;
        if (cubalc_host_write(path, body, &wr) == 0)
          hit = 1;
      } else {
        fclose(lf);
      }
    }
#else
    {
      int fd = open(path, O_CREAT | O_EXCL | O_WRONLY, 0644);
      if (fd >= 0) {
        size_t bl = strlen(body);
        ssize_t w = write(fd, body, bl);
        (void)w;
        close(fd);
        hit = 1;
      }
      /* EEXIST or other → soft miss */
    }
#endif
    if (hit) {
      var_set_str(vm, "LAST", path);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_str(vm, "CLAIM", path);
      var_set_str(vm, "CLAIMFILE", path);
      var_set_str(vm, "CREATEEXCL", path);
      var_set_num(vm, "CLAIM_HIT", 1);
      var_set_num(vm, "CLAIM_N", 1);
      var_set_num(vm, "CLAIM_PID", pid);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    var_set_str(vm, "LAST", "");
    vm->last_str[0] = 0;
    vm->last_n = 0;
    var_set_num(vm, "LAST_N", 0);
    var_set_str(vm, "CLAIM", "");
    var_set_num(vm, "CLAIM_HIT", 0);
    var_set_num(vm, "CLAIM_N", 0);
    var_set_num(vm, "CLAIM_PID", 0);
    var_set_num(vm, "OK", 0);
    var_set_str(vm, "LAST_ERR", "CLAIM: exists");
    var_set_str(vm, "ERR", "CLAIM: exists");
    bump(vm); return 1;
  }
  /* SYS RM|UNLINK|DELETE path — remove regular file; missing soft OK (LAST_N=0)
   * Usability: agents clean STATE/TMP plates without shell rm. */
  if (kw(&L->cur,"RM") || kw(&L->cur,"UNLINK") || kw(&L->cur,"DELETE") ||
      kw(&L->cur,"REMOVE") || kw(&L->cur,"DEL")){
    char path[512];
    cubalc_host_result hr;
    lex_next(L);
    if (resolve_str_arg(vm, L, path, sizeof path)!=0){
      fail(vm,"SYS RM \"path\"|LAST"); return -1;
    }
    if (cubalc_host_rm(path, &hr)!=0){
      var_set_num(vm, "OK", 0);
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "RM_N", 0);
      if (hr.err[0]) {
        var_set_str(vm, "LAST_ERR", hr.err);
        var_set_str(vm, "ERR", hr.err);
        var_set_str(vm, "LAST", hr.err);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.err);
      }
      bump(vm); return 1;
    }
    var_set_str(vm, "LAST", path);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "RM_N", hr.n);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS RMDIR|REMOVEDIR|RD path — remove empty directory; missing soft OK.
   * LAST_N/RMDIR_N = 1 removed, 0 already missing. Non-empty/file → OK=0.
   * Usability: clean MKTEMPDIR workspaces without shell rmdir. */
  if (kw(&L->cur,"RMDIR") || kw(&L->cur,"REMOVEDIR") || kw(&L->cur,"RD") ||
      kw(&L->cur,"RM_DIR") || kw(&L->cur,"DELETEDIR") || kw(&L->cur,"UNLINKDIR") ||
      kw(&L->cur,"DRODIR") || kw(&L->cur,"RMEMPTY")){
    char path[512];
    cubalc_host_result hr;
    lex_next(L);
    if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
      fail(vm, "SYS RMDIR path");
      return -1;
    }
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "RMDIR_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "RMDIR: empty path");
      var_set_str(vm, "ERR", "RMDIR: empty path");
      bump(vm); return 1;
    }
    if (cubalc_host_rmdir(path, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "RMDIR_N", 0);
      vm->last_n = 0;
      if (hr.err[0]) {
        var_set_str(vm, "LAST_ERR", hr.err);
        var_set_str(vm, "ERR", hr.err);
        var_set_str(vm, "LAST", hr.err);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.err);
      }
      bump(vm); return 1;
    }
    var_set_str(vm, "LAST", path);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "RMDIR_N", hr.n);
    var_set_str(vm, "RMDIR", path);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  return 0;
}
