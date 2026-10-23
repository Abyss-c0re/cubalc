/* CubalC lang — lang_ops_core_sys.c
 * SYS/HOST megablock dispatch (mesh vote A_split_lang_ops_core).
 * Peel of ~33k-line SYS plane from lang_ops_core.
 */
#include "lang/cubalc_lang_ops_sys_path_fs.h"
#include "lang/cubalc_lang_ops_sys_path_writeout.h"
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
#  include <dirent.h>
#  include <sys/stat.h>
#  include <sys/statvfs.h>
#  include <fcntl.h>
#  include <poll.h>
#  include <pwd.h>
#  include <grp.h>
#  include <fnmatch.h>
#  include <sys/utsname.h>
#  include <sys/resource.h>
#  include <sys/socket.h>
#  include <sys/wait.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
/* getloadavg is BSD/POSIX extension; declare if libc omits under strict feature macros. */
int getloadavg(double loadavg[], int nelem);
#endif

int cubalc_lang_ops_sys_try_path_fs(VM *vm, Lex *L) {
  /* Path/FS SYS plane — WRITEOUT family · trees · pathlines · links · …
   * Caller: after SYS keyword + plate try miss; L at subcommand.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
  /* WRITEOUT family → lang_ops_sys_path_writeout.c */
  {
    int wo = cubalc_lang_ops_sys_try_path_writeout(vm, L);
    if (wo != 0) return wo;
  }

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
    /* SYS RMTREE|RMNEST|RMRF|DELTREE path — recursive remove file or directory tree.
     * File/symlink → unlink; dir → depth-first children then rmdir. Missing soft OK (LAST_N=0).
     * LAST = path · LAST_N / RMTREE_N = entries removed · soft empty/fail.
     * Usability: clean nested MKTEMPDIR/CLAIMOUT/WRITEOUT workspaces without shell rm -rf
     * or WALK+RMALL+RMDIR glue (RM is file-only; RMDIR empty-only). */
    if (kw(&L->cur,"RMTREE") || kw(&L->cur,"RMNEST") || kw(&L->cur,"RMRF") ||
        kw(&L->cur,"DELTREE") || kw(&L->cur,"RM_TREE") || kw(&L->cur,"PURGETREE") ||
        kw(&L->cur,"REMOVETREE") || kw(&L->cur,"RMTREE_RF") ||
        kw(&L->cur,"WIPETREE") || kw(&L->cur,"RMREC") || kw(&L->cur,"RECURSERM")){
      char path[CUBALC_HOST_STR_MAX], abs[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      lex_next(L);
      path[0] = 0; abs[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, "SYS RMTREE path");
        return -1;
      }
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "RMTREE_N", 0);
        var_set_num(vm, "RMNEST_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "RMTREE: empty path");
        var_set_str(vm, "ERR", "RMTREE: empty path");
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_abspath(path, &hr) == 0 && hr.str[0])
        snprintf(abs, sizeof abs, "%s", hr.str);
      else
        snprintf(abs, sizeof abs, "%s", path);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_rmtree(abs, &hr) != 0) {
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "RMTREE_N", 0);
        var_set_num(vm, "RMNEST_N", 0);
        vm->last_n = 0;
        var_set_str(vm, "LAST", abs);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", abs);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "RMTREE: fail");
          var_set_str(vm, "ERR", "RMTREE: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", abs);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", abs);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "RMTREE_N", hr.n);
      var_set_num(vm, "RMNEST_N", hr.n);
      var_set_num(vm, "RMRF_N", hr.n);
      var_set_str(vm, "RMTREE", abs);
      var_set_str(vm, "RMNEST", abs);
      var_set_str(vm, "PATH", abs);
      var_set_num(vm, "PATH_EXIST", 0);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys rmtree → %s n=%ld\n", abs, hr.n);
      bump(vm); return 1;
    }
    /* SYS CPTREE|COPYTREE|CPDIR src dest — recursive copy file or directory tree.
     * File → COPY; dir → mkdir dest + children. LAST = abs dest · LAST_N/CPTREE_N = entries.
     * CPTREE_BYTES · soft empty/missing src · dest-inside-src refused.
     * Usability: snapshot nested agent workspaces without shell cp -r or WALK+COPYALL glue. */
    if (kw(&L->cur,"CPTREE") || kw(&L->cur,"COPYTREE") || kw(&L->cur,"CPDIR") ||
        kw(&L->cur,"COPYDIR") || kw(&L->cur,"CP_TREE") || kw(&L->cur,"TREECIPY") ||
        kw(&L->cur,"TREECOPY") || kw(&L->cur,"RCPY") || kw(&L->cur,"RCOPY") ||
        kw(&L->cur,"SNAPSHOTTREE") || kw(&L->cur,"DUPTREE")){
      char src[CUBALC_HOST_STR_MAX], dst[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      lex_next(L);
      src[0] = 0; dst[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0) {
        fail(vm, "SYS CPTREE src dest");
        return -1;
      }
      if (resolve_str_arg(vm, L, dst, sizeof dst) != 0) {
        fail(vm, "SYS CPTREE src dest");
        return -1;
      }
      if (!src[0] || !dst[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "CPTREE_N", 0);
        var_set_num(vm, "CPTREE_BYTES", 0);
        var_set_num(vm, "COPYTREE_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "CPTREE: empty path");
        var_set_str(vm, "ERR", "CPTREE: empty path");
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_cptree(src, dst, &hr) != 0) {
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "CPTREE_N", 0);
        var_set_num(vm, "CPTREE_BYTES", 0);
        var_set_num(vm, "COPYTREE_N", 0);
        vm->last_n = 0;
        var_set_str(vm, "LAST", dst);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", dst);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "CPTREE: fail");
          var_set_str(vm, "ERR", "CPTREE: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str[0] ? hr.str : dst);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str[0] ? hr.str : dst);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "CPTREE_N", hr.n);
      var_set_num(vm, "COPYTREE_N", hr.n);
      var_set_num(vm, "CPDIR_N", hr.n);
      var_set_num(vm, "CPTREE_BYTES", (long)hr.code);
      var_set_str(vm, "CPTREE", hr.str[0] ? hr.str : dst);
      var_set_str(vm, "COPYTREE", hr.str[0] ? hr.str : dst);
      var_set_str(vm, "PATH", hr.str[0] ? hr.str : dst);
      var_set_num(vm, "PATH_EXIST", 1);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys cptree → %s n=%ld bytes=%d\n",
                hr.str[0] ? hr.str : dst, hr.n, hr.code);
      bump(vm); return 1;
    }
    /* SYS MVTREE|MOVETREE|MVDIR src dest — recursive move (rename or cptree+rmtree).
     * LAST = abs dest · LAST_N/MVTREE_N = entries · MVTREE_RENAMED 1|0 · soft miss/empty.
     * Usability: promote nested agent workspaces without shell mv or CPTREE+RMTREE glue. */
    if (kw(&L->cur,"MVTREE") || kw(&L->cur,"MOVETREE") || kw(&L->cur,"MVDIR") ||
        kw(&L->cur,"MOVEDIR") || kw(&L->cur,"MV_TREE") || kw(&L->cur,"RELOCATETREE") ||
        kw(&L->cur,"PROMOVETREE") || kw(&L->cur,"TREEMV") || kw(&L->cur,"RMV") ||
        kw(&L->cur,"RENAMETREE")){
      char src[CUBALC_HOST_STR_MAX], dst[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      lex_next(L);
      src[0] = 0; dst[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0) {
        fail(vm, "SYS MVTREE src dest");
        return -1;
      }
      if (resolve_str_arg(vm, L, dst, sizeof dst) != 0) {
        fail(vm, "SYS MVTREE src dest");
        return -1;
      }
      if (!src[0] || !dst[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "MVTREE_N", 0);
        var_set_num(vm, "MOVETREE_N", 0);
        var_set_num(vm, "MVTREE_RENAMED", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "MVTREE: empty path");
        var_set_str(vm, "ERR", "MVTREE: empty path");
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_mvtree(src, dst, &hr) != 0) {
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "MVTREE_N", 0);
        var_set_num(vm, "MOVETREE_N", 0);
        var_set_num(vm, "MVTREE_RENAMED", 0);
        vm->last_n = 0;
        var_set_str(vm, "LAST", dst);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", dst);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "MVTREE: fail");
          var_set_str(vm, "ERR", "MVTREE: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str[0] ? hr.str : dst);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str[0] ? hr.str : dst);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "MVTREE_N", hr.n);
      var_set_num(vm, "MOVETREE_N", hr.n);
      var_set_num(vm, "MVDIR_N", hr.n);
      var_set_num(vm, "MVTREE_RENAMED", hr.code ? 1 : 0);
      var_set_num(vm, "MOVETREE_RENAMED", hr.code ? 1 : 0);
      var_set_str(vm, "MVTREE", hr.str[0] ? hr.str : dst);
      var_set_str(vm, "MOVETREE", hr.str[0] ? hr.str : dst);
      var_set_str(vm, "PATH", hr.str[0] ? hr.str : dst);
      var_set_num(vm, "PATH_EXIST", 1);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys mvtree → %s n=%ld renamed=%d\n",
                hr.str[0] ? hr.str : dst, hr.n, hr.code);
      bump(vm); return 1;
    }
    /* SYS DIRSIZE|DU|TREESIZE path — recursive total file bytes under path.
     * LAST = abs path · LAST_N / DIRSIZE = bytes · DIRSIZE_FILES · DIRSIZE_DIRS.
     * Soft empty/missing. File → own size; dir → sum tree (no dir-symlink follow).
     * Usability: agent disk inventory / DF gate without shell du or WALK+SIZEALL. */
    if (kw(&L->cur,"DIRSIZE") || kw(&L->cur,"DU") || kw(&L->cur,"TREESIZE") ||
        kw(&L->cur,"DISKUSAGE") || kw(&L->cur,"DIR_SIZE") || kw(&L->cur,"TREEBYTES") ||
        kw(&L->cur,"BYTESOF") || kw(&L->cur,"SIZEOFDIR") || kw(&L->cur,"DUSIZE") ||
        kw(&L->cur,"USAGE")){
      char path[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      long files, dirs;
      lex_next(L);
      path[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, "SYS DIRSIZE path");
        return -1;
      }
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DIRSIZE", 0);
        var_set_num(vm, "DIRSIZE_N", 0);
        var_set_num(vm, "DIRSIZE_FILES", 0);
        var_set_num(vm, "DIRSIZE_DIRS", 0);
        var_set_num(vm, "DU_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "DIRSIZE: empty path");
        var_set_str(vm, "ERR", "DIRSIZE: empty path");
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_dirsize(path, &hr) != 0) {
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DIRSIZE", 0);
        var_set_num(vm, "DIRSIZE_N", 0);
        var_set_num(vm, "DIRSIZE_FILES", 0);
        var_set_num(vm, "DIRSIZE_DIRS", 0);
        var_set_num(vm, "DU_N", 0);
        vm->last_n = 0;
        var_set_str(vm, "LAST", path);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "DIRSIZE: fail");
          var_set_str(vm, "ERR", "DIRSIZE: fail");
        }
        bump(vm); return 1;
      }
      files = (long)((unsigned)hr.code & 0xffffu);
      dirs = (long)(((unsigned)hr.code >> 16) & 0xffffu);
      var_set_str(vm, "LAST", hr.str[0] ? hr.str : path);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str[0] ? hr.str : path);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "DIRSIZE", hr.n);
      var_set_num(vm, "DIRSIZE_N", hr.n);
      var_set_num(vm, "TREESIZE", hr.n);
      var_set_num(vm, "TREESIZE_N", hr.n);
      var_set_num(vm, "DU_N", hr.n);
      var_set_num(vm, "DIRSIZE_FILES", files);
      var_set_num(vm, "DIRSIZE_DIRS", dirs);
      var_set_num(vm, "DU_FILES", files);
      var_set_num(vm, "DU_DIRS", dirs);
      var_set_str(vm, "DIRSIZE_PATH", hr.str[0] ? hr.str : path);
      var_set_str(vm, "PATH", hr.str[0] ? hr.str : path);
      var_set_num(vm, "PATH_EXIST", 1);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys dirsize → %s bytes=%ld files=%ld dirs=%ld\n",
                hr.str[0] ? hr.str : path, hr.n, files, dirs);
      bump(vm); return 1;
    }
    /* SYS EQTREE|SAMETREE|CMPTREE a b — recursive structure + content equality.
     * LAST_N / EQTREE = 1|0 · EQTREE_DIFF relative path · EQTREE_REASON when differ.
     * Soft empty/missing root. File pairs use content compare; dirs walk both ways.
     * Usability: verify CPTREE snapshots without shell diff -r or EACH+EQFILE glue. */
    if (kw(&L->cur,"EQTREE") || kw(&L->cur,"SAMETREE") || kw(&L->cur,"CMPTREE") ||
        kw(&L->cur,"TREEEQ") || kw(&L->cur,"EQDIR") || kw(&L->cur,"SAMEDIR") ||
        kw(&L->cur,"DIRSEQ") || kw(&L->cur,"IDENTICALTREE") || kw(&L->cur,"TREE_EQ") ||
        kw(&L->cur,"DIFFTREE")){
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      long eq;
      lex_next(L);
      a[0] = 0; b[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS EQTREE path_a path_b");
        return -1;
      }
      if (resolve_str_arg(vm, L, b, sizeof b) != 0) {
        fail(vm, "SYS EQTREE path_a path_b");
        return -1;
      }
      if (!a[0] || !b[0]) {
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "EQTREE", 0);
        var_set_num(vm, "SAMETREE", 0);
        var_set_num(vm, "EQTREE_N", 0);
        var_set_str(vm, "EQTREE_DIFF", "");
        var_set_str(vm, "EQTREE_REASON", "empty");
        var_set_str(vm, "LAST", "0");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "0");
        vm->last_n = 0;
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "EQTREE: empty path");
        var_set_str(vm, "ERR", "EQTREE: empty path");
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_eqtree(a, b, &hr) != 0) {
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "EQTREE", 0);
        var_set_num(vm, "SAMETREE", 0);
        var_set_num(vm, "EQTREE_N", 0);
        var_set_str(vm, "EQTREE_DIFF", "");
        var_set_str(vm, "EQTREE_REASON", hr.err[0] ? hr.err : "fail");
        var_set_str(vm, "LAST", "0");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "0");
        vm->last_n = 0;
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "EQTREE: fail");
          var_set_str(vm, "ERR", "EQTREE: fail");
        }
        bump(vm); return 1;
      }
      eq = hr.n ? 1 : 0;
      vm->last_n = eq;
      var_set_num(vm, "LAST_N", eq);
      var_set_num(vm, "EQTREE", eq);
      var_set_num(vm, "SAMETREE", eq);
      var_set_num(vm, "CMPTREE", eq);
      var_set_num(vm, "EQTREE_N", eq);
      var_set_str(vm, "EQTREE_DIFF", eq ? "" : (hr.str[0] ? hr.str : "."));
      var_set_str(vm, "EQTREE_REASON", eq ? "" : (hr.err[0] ? hr.err : "differ"));
      {
        char buf[16];
        snprintf(buf, sizeof buf, "%ld", eq);
        var_set_str(vm, "LAST", buf);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      }
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys eqtree → %ld diff=%s reason=%s\n",
                eq, eq ? "" : hr.str, eq ? "" : hr.err);
      bump(vm); return 1;
    }
    /* SYS GREPTREE|SEARCHTREE root needle — recursive content search under root.
     * LAST = matching full paths bag · LAST_N / GREPTREE_N = hits · GREPTREE_SCAN.
     * GREPTREEI case-insensitive. Soft empty/missing root. Empty needle → all files.
     * Usability: agent content discovery without WALK+GREPFILES or shell grep -r. */
    if (kw(&L->cur,"GREPTREE") || kw(&L->cur,"SEARCHTREE") || kw(&L->cur,"RGREP") ||
        kw(&L->cur,"TREEGREP") || kw(&L->cur,"GREPDIR") || kw(&L->cur,"CONTENTTREE") ||
        kw(&L->cur,"GREPTREEI") || kw(&L->cur,"SEARCHTREEI") || kw(&L->cur,"RGREPI") ||
        kw(&L->cur,"TREEGREPI") || kw(&L->cur,"GREPDIRI") || kw(&L->cur,"FINDCONTENT") ||
        kw(&L->cur,"FINDIN")){
      char root[CUBALC_HOST_STR_MAX], needle[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      int icase = 0;
      if (kw(&L->cur,"GREPTREEI") || kw(&L->cur,"SEARCHTREEI") || kw(&L->cur,"RGREPI") ||
          kw(&L->cur,"TREEGREPI") || kw(&L->cur,"GREPDIRI"))
        icase = 1;
      lex_next(L);
      root[0] = 0; needle[0] = 0;
      if (resolve_str_arg(vm, L, root, sizeof root) != 0) {
        fail(vm, "SYS GREPTREE root needle");
        return -1;
      }
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long nv = parse_expr(vm, L);
          snprintf(needle, sizeof needle, "%ld", nv);
        } else if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
          needle[0] = 0;
        }
      }
      if (!root[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "GREPTREE_N", 0);
        var_set_num(vm, "GREPTREE_SCAN", 0);
        var_set_num(vm, "SEARCHTREE_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "GREPTREE: empty path");
        var_set_str(vm, "ERR", "GREPTREE: empty path");
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_greptree(root, needle, icase, 0, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "GREPTREE_N", 0);
        var_set_num(vm, "GREPTREE_SCAN", 0);
        var_set_num(vm, "SEARCHTREE_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "GREPTREE: fail");
          var_set_str(vm, "ERR", "GREPTREE: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "GREPTREE", hr.str);
      var_set_str(vm, "SEARCHTREE", hr.str);
      var_set_str(vm, "RGREP", hr.str);
      var_set_num(vm, "GREPTREE_N", hr.n);
      var_set_num(vm, "SEARCHTREE_N", hr.n);
      var_set_num(vm, "RGREP_N", hr.n);
      var_set_num(vm, "GREPTREE_SCAN", (long)hr.code);
      var_set_str(vm, "GREPTREE_ROOT", root);
      var_set_str(vm, "GREPTREE_NEEDLE", needle);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys greptree → hits=%ld scan=%d icase=%d\n",
                hr.n, hr.code, icase);
      bump(vm); return 1;
    }
    /* SYS REPLACETREE|GSUBTREE|SEDTREE root old new — recursive REPLACEALL under root.
     * LAST = rewritten path bag · LAST_N / REPLACETREE_N = files · REPLACETREE_SUBS total.
     * Soft empty/missing · empty old → no-op. Only writes when content changes.
     * Usability: bulk nested plate rewrites without WALK+REPLACEFILES or shell find|sed. */
    if (kw(&L->cur,"REPLACETREE") || kw(&L->cur,"GSUBTREE") || kw(&L->cur,"SEDTREE") ||
        kw(&L->cur,"TREEREPLACE") || kw(&L->cur,"RPLTREE") || kw(&L->cur,"SUBALLTREE") ||
        kw(&L->cur,"REWRITETREE") || kw(&L->cur,"TREESED") || kw(&L->cur,"BULKTREE") ||
        kw(&L->cur,"SEDALLTREE")){
      char root[CUBALC_HOST_STR_MAX], olds[CUBALC_HOST_STR_MAX], news[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      lex_next(L);
      root[0] = 0; olds[0] = 0; news[0] = 0;
      if (resolve_str_arg(vm, L, root, sizeof root) != 0) {
        fail(vm, "SYS REPLACETREE root old new");
        return -1;
      }
      if (resolve_str_arg(vm, L, olds, sizeof olds) != 0) {
        fail(vm, "SYS REPLACETREE root old new");
        return -1;
      }
      if (resolve_str_arg(vm, L, news, sizeof news) != 0) {
        /* allow empty new via missing? require third arg */
        news[0] = 0;
      }
      if (!root[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "REPLACETREE_N", 0);
        var_set_num(vm, "REPLACETREE_SUBS", 0);
        var_set_num(vm, "GSUBTREE_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "REPLACETREE: empty path");
        var_set_str(vm, "ERR", "REPLACETREE: empty path");
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_replacetree(root, olds, news, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "REPLACETREE_N", 0);
        var_set_num(vm, "REPLACETREE_SUBS", 0);
        var_set_num(vm, "GSUBTREE_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "REPLACETREE: fail");
          var_set_str(vm, "ERR", "REPLACETREE: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "REPLACETREE", hr.str);
      var_set_str(vm, "GSUBTREE", hr.str);
      var_set_str(vm, "SEDTREE", hr.str);
      var_set_num(vm, "REPLACETREE_N", hr.n);
      var_set_num(vm, "GSUBTREE_N", hr.n);
      var_set_num(vm, "SEDTREE_N", hr.n);
      var_set_num(vm, "REPLACETREE_SUBS", (long)hr.code);
      var_set_num(vm, "GSUBTREE_SUBS", (long)hr.code);
      var_set_str(vm, "REPLACETREE_ROOT", root);
      var_set_str(vm, "REPLACETREE_OLD", olds);
      var_set_str(vm, "REPLACETREE_NEW", news);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys replacetree → files=%ld subs=%d\n",
                hr.n, hr.code);
      bump(vm); return 1;
    }
    /* SYS COUNTINTREE|COUNTTREE|GREPCOUNTTREE root needle — recursive occurrence tally.
     * LAST_N / COUNTINTREE = total hits · COUNTINTREE_FILES = files with ≥1 hit.
     * COUNTINTREEI case-insensitive. Soft empty/missing. Empty needle → 0.
     * Usability: severity/error rollups under a tree without WALK+COUNTINFILES. */
    if (kw(&L->cur,"COUNTINTREE") || kw(&L->cur,"COUNTTREE") || kw(&L->cur,"GREPCOUNTTREE") ||
        kw(&L->cur,"TREECOUNT") || kw(&L->cur,"OCCURTREE") || kw(&L->cur,"TOTALINTREE") ||
        kw(&L->cur,"COUNTINTREEI") || kw(&L->cur,"COUNTTREEI") || kw(&L->cur,"GREPCOUNTTREEI") ||
        kw(&L->cur,"TREECOUNTI") || kw(&L->cur,"OCCURTREEI")){
      char root[CUBALC_HOST_STR_MAX], needle[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      int icase = 0;
      if (kw(&L->cur,"COUNTINTREEI") || kw(&L->cur,"COUNTTREEI") ||
          kw(&L->cur,"GREPCOUNTTREEI") || kw(&L->cur,"TREECOUNTI") ||
          kw(&L->cur,"OCCURTREEI"))
        icase = 1;
      lex_next(L);
      root[0] = 0; needle[0] = 0;
      if (resolve_str_arg(vm, L, root, sizeof root) != 0) {
        fail(vm, "SYS COUNTINTREE root needle");
        return -1;
      }
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long nv = parse_expr(vm, L);
          snprintf(needle, sizeof needle, "%ld", nv);
        } else if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
          needle[0] = 0;
        }
      }
      if (!root[0]) {
        var_set_str(vm, "LAST", "0");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "0");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "COUNTINTREE", 0);
        var_set_num(vm, "COUNTINTREE_N", 0);
        var_set_num(vm, "COUNTINTREE_FILES", 0);
        var_set_num(vm, "COUNTTREE_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "COUNTINTREE: empty path");
        var_set_str(vm, "ERR", "COUNTINTREE: empty path");
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_countintree(root, needle, icase, &hr) != 0) {
        var_set_str(vm, "LAST", "0");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "0");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "COUNTINTREE", 0);
        var_set_num(vm, "COUNTINTREE_N", 0);
        var_set_num(vm, "COUNTINTREE_FILES", 0);
        var_set_num(vm, "COUNTTREE_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "COUNTINTREE: fail");
          var_set_str(vm, "ERR", "COUNTINTREE: fail");
        }
        bump(vm); return 1;
      }
      {
        char buf[32];
        snprintf(buf, sizeof buf, "%ld", hr.n);
        var_set_str(vm, "LAST", buf);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      }
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "COUNTINTREE", hr.n);
      var_set_num(vm, "COUNTINTREE_N", hr.n);
      var_set_num(vm, "COUNTTREE", hr.n);
      var_set_num(vm, "COUNTTREE_N", hr.n);
      var_set_num(vm, "GREPCOUNTTREE", hr.n);
      var_set_num(vm, "TREECOUNT", hr.n);
      var_set_num(vm, "COUNTINTREE_FILES", (long)hr.code);
      var_set_num(vm, "COUNTTREE_FILES", (long)hr.code);
      var_set_str(vm, "COUNTINTREE_ROOT", hr.str[0] ? hr.str : root);
      var_set_str(vm, "COUNTINTREE_NEEDLE", needle);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys countintree → hits=%ld files=%d icase=%d\n",
                hr.n, hr.code, icase);
      bump(vm); return 1;
    }
    /* SYS GREPLINESTREE|EXTRACTLINETREE root needle — recursive matching-line harvest.
     * LAST = line bag · LAST_N / GREPLINESTREE_N = lines · GREPLINESTREE_FILES.
     * GREPLINESTREEI case-insensitive. Soft empty/missing. Empty needle → all lines.
     * Complements GREPTREE (paths) / COUNTINTREE (tally) with line extract.
     * Usability: log harvest under a tree without WALK+GREPLINES or shell grep -rn. */
    if (kw(&L->cur,"GREPLINESTREE") || kw(&L->cur,"EXTRACTLINETREE") ||
        kw(&L->cur,"TREEGREPLINES") || kw(&L->cur,"HARVESTTREE") ||
        kw(&L->cur,"LINETREE") || kw(&L->cur,"GREPLINEST") ||
        kw(&L->cur,"GREPLINESTREEI") || kw(&L->cur,"EXTRACTLINETREEI") ||
        kw(&L->cur,"TREEGREPLINESI") || kw(&L->cur,"HARVESTTREEI") ||
        kw(&L->cur,"LINETREEI")){
      char root[CUBALC_HOST_STR_MAX], needle[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      int icase = 0;
      if (kw(&L->cur,"GREPLINESTREEI") || kw(&L->cur,"EXTRACTLINETREEI") ||
          kw(&L->cur,"TREEGREPLINESI") || kw(&L->cur,"HARVESTTREEI") ||
          kw(&L->cur,"LINETREEI"))
        icase = 1;
      lex_next(L);
      root[0] = 0; needle[0] = 0;
      if (resolve_str_arg(vm, L, root, sizeof root) != 0) {
        fail(vm, "SYS GREPLINESTREE root needle");
        return -1;
      }
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long nv = parse_expr(vm, L);
          snprintf(needle, sizeof needle, "%ld", nv);
        } else if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
          needle[0] = 0;
        }
      }
      if (!root[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "GREPLINESTREE_N", 0);
        var_set_num(vm, "GREPLINESTREE_FILES", 0);
        var_set_num(vm, "EXTRACTLINETREE_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "GREPLINESTREE: empty path");
        var_set_str(vm, "ERR", "GREPLINESTREE: empty path");
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_greplines_tree(root, needle, icase, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "GREPLINESTREE_N", 0);
        var_set_num(vm, "GREPLINESTREE_FILES", 0);
        var_set_num(vm, "EXTRACTLINETREE_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "GREPLINESTREE: fail");
          var_set_str(vm, "ERR", "GREPLINESTREE: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "GREPLINESTREE", hr.str);
      var_set_str(vm, "EXTRACTLINETREE", hr.str);
      var_set_str(vm, "HARVESTTREE", hr.str);
      var_set_num(vm, "GREPLINESTREE_N", hr.n);
      var_set_num(vm, "EXTRACTLINETREE_N", hr.n);
      var_set_num(vm, "HARVESTTREE_N", hr.n);
      var_set_num(vm, "GREPLINESTREE_FILES", (long)hr.code);
      var_set_num(vm, "EXTRACTLINETREE_FILES", (long)hr.code);
      var_set_str(vm, "GREPLINESTREE_ROOT", root);
      var_set_str(vm, "GREPLINESTREE_NEEDLE", needle);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys greplinestree → lines=%ld files=%d icase=%d\n",
                hr.n, hr.code, icase);
      bump(vm); return 1;
    }
    /* SYS PATHLINETREE|GREPNTREE|RGLINES root needle — recursive path:lineno:line harvest.
     * LAST = annotated bag · PATHLINETREE_N · PATHLINETREE_FILES. 1-based lineno.
     * PATHLINETREEI case-insensitive. Soft empty/missing. Empty needle → all lines.
     * Complements GREPLINESTREE (line text only) with provenance for agents.
     * Usability: agent grep -rn without WALK+EACH+READ or shell. */
    if (kw(&L->cur,"PATHLINETREE") || kw(&L->cur,"GREPNTREE") || kw(&L->cur,"RGLINES") ||
        kw(&L->cur,"ANNOTREE") || kw(&L->cur,"GREPRN") || kw(&L->cur,"PATHLINES") ||
        kw(&L->cur,"PATHLINETREEI") || kw(&L->cur,"GREPNTREEI") || kw(&L->cur,"RGLINESI") ||
        kw(&L->cur,"ANNOTREEI") || kw(&L->cur,"GREPRNI") || kw(&L->cur,"PATHLINESI")){
      char root[CUBALC_HOST_STR_MAX], needle[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      int icase = 0;
      if (kw(&L->cur,"PATHLINETREEI") || kw(&L->cur,"GREPNTREEI") || kw(&L->cur,"RGLINESI") ||
          kw(&L->cur,"ANNOTREEI") || kw(&L->cur,"GREPRNI") || kw(&L->cur,"PATHLINESI"))
        icase = 1;
      lex_next(L);
      root[0] = 0; needle[0] = 0;
      if (resolve_str_arg(vm, L, root, sizeof root) != 0) {
        fail(vm, "SYS PATHLINETREE root needle");
        return -1;
      }
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long nv = parse_expr(vm, L);
          snprintf(needle, sizeof needle, "%ld", nv);
        } else if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
          needle[0] = 0;
        }
      }
      if (!root[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "PATHLINETREE_N", 0);
        var_set_num(vm, "PATHLINETREE_FILES", 0);
        var_set_num(vm, "GREPNTREE_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "PATHLINETREE: empty path");
        var_set_str(vm, "ERR", "PATHLINETREE: empty path");
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_pathlines_tree(root, needle, icase, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "PATHLINETREE_N", 0);
        var_set_num(vm, "PATHLINETREE_FILES", 0);
        var_set_num(vm, "GREPNTREE_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "PATHLINETREE: fail");
          var_set_str(vm, "ERR", "PATHLINETREE: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "PATHLINETREE", hr.str);
      var_set_str(vm, "GREPNTREE", hr.str);
      var_set_str(vm, "RGLINES", hr.str);
      var_set_num(vm, "PATHLINETREE_N", hr.n);
      var_set_num(vm, "GREPNTREE_N", hr.n);
      var_set_num(vm, "RGLINES_N", hr.n);
      var_set_num(vm, "PATHLINETREE_FILES", (long)hr.code);
      var_set_num(vm, "GREPNTREE_FILES", (long)hr.code);
      var_set_str(vm, "PATHLINETREE_ROOT", root);
      var_set_str(vm, "PATHLINETREE_NEEDLE", needle);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys pathlinestree → lines=%ld files=%d icase=%d\n",
                hr.n, hr.code, icase);
      bump(vm); return 1;
    }
    /* SYS PARSEPATHLINE|SPLITPATHLINE|PEELPATHLINE entry
     * — peel path:lineno:text (PATHLINETREE / grep -rn) → PATH · LINE_N · LAST=text.
     * Soft miss non-matching → HIT=0 LAST="" PATH="" LINE_N=0 (OK=1).
     * Empty entry → OK=0 sticky. Bare form uses LAST.
     * Usability: walk PATHLINETREE bag without FIND+MID colon glue. */
    if (kw(&L->cur,"PARSEPATHLINE") || kw(&L->cur,"SPLITPATHLINE") ||
        kw(&L->cur,"PEELPATHLINE") || kw(&L->cur,"PATHLINEPEEL") ||
        kw(&L->cur,"UNPATHLINE") || kw(&L->cur,"DECODEPATHLINE") ||
        kw(&L->cur,"PARSEPL") || kw(&L->cur,"PLPEEL")){
      char entry[CUBALC_HOST_STR_MAX], path[CUBALC_HOST_STR_MAX], text[CUBALC_HOST_STR_MAX];
      long lineno = 0, hit = 0;
      lex_next(L);
      entry[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, entry, sizeof entry) != 0)
          entry[0] = 0;
      } else {
        snprintf(entry, sizeof entry, "%s", vm->last_str);
      }
      if (!entry[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "PATH", "");
        var_set_str(vm, "TEXT", "");
        var_set_num(vm, "LINE_N", 0);
        var_set_num(vm, "PARSEPATHLINE_N", 0);
        var_set_num(vm, "PARSEPATHLINE_HIT", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "PARSEPATHLINE: empty entry");
        var_set_str(vm, "ERR", "PARSEPATHLINE: empty entry");
        bump(vm); return 1;
      }
      path[0] = 0; text[0] = 0;
      hit = cubalc_peel_pathline(entry, path, sizeof path, &lineno, text, sizeof text) ? 1 : 0;
      if (!hit) {
        path[0] = 0; text[0] = 0; lineno = 0;
      }
      var_set_str(vm, "LAST", text);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", text);
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_str(vm, "PATH", path);
      var_set_str(vm, "TEXT", text);
      var_set_str(vm, "PARSEPATHLINE", text);
      var_set_str(vm, "PARSEPATHLINE_PATH", path);
      var_set_str(vm, "PARSEPATHLINE_TEXT", text);
      var_set_str(vm, "PATHLINE_PATH", path);
      var_set_str(vm, "PATHLINE_TEXT", text);
      var_set_num(vm, "LINE_N", hit ? lineno : 0);
      var_set_num(vm, "PATHLINE_N", hit ? lineno : 0);
      var_set_num(vm, "PARSEPATHLINE_N", hit ? lineno : 0);
      var_set_num(vm, "PARSEPATHLINE_HIT", hit);
      var_set_num(vm, "PATHLINE_HIT", hit);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys parsepathline → hit=%ld path=%s line=%ld text=%s\n",
                hit, path, lineno, text);
      bump(vm); return 1;
    }
    /* SYS FORMATPATHLINE|MKPATHLINE|TOPATHLINE path lineno [text]
     * — build path:lineno:text (inverse of PARSEPATHLINE) for evidence cites.
     * LAST = annotated entry · FORMATPATHLINE_HIT. Empty path → OK=0 sticky.
     * Omitted text uses LAST (or ""). lineno via parse_expr (var/num).
     * Usability: agent grep -n cites without STR+CAT colon glue. */
    if (kw(&L->cur,"FORMATPATHLINE") || kw(&L->cur,"MKPATHLINE") ||
        kw(&L->cur,"TOPATHLINE") || kw(&L->cur,"MAKEPATHLINE") ||
        kw(&L->cur,"BUILDPATHLINE") || kw(&L->cur,"PATHLINEFMT") ||
        kw(&L->cur,"CITEPATHLINE") || kw(&L->cur,"PLFMT")){
      char path[CUBALC_HOST_STR_MAX], text[CUBALC_HOST_STR_MAX], entry[CUBALC_HOST_STR_MAX];
      long lineno = 0;
      lex_next(L);
      path[0] = 0; text[0] = 0; entry[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, "SYS FORMATPATHLINE path lineno [text]");
        return -1;
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_IDENT) {
        lineno = parse_expr(vm, L);
      } else {
        fail(vm, "SYS FORMATPATHLINE path lineno [text]");
        return -1;
      }
      if (lineno < 0) lineno = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, text, sizeof text) != 0)
          text[0] = 0;
      } else {
        snprintf(text, sizeof text, "%s", vm->last_str);
      }
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "FORMATPATHLINE_HIT", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "FORMATPATHLINE: empty path");
        var_set_str(vm, "ERR", "FORMATPATHLINE: empty path");
        bump(vm); return 1;
      }
      /* strip newlines from text (single field) */
      {
        char *q;
        for (q = text; *q; q++)
          if (*q == '\n' || *q == '\r') *q = ' ';
      }
      snprintf(entry, sizeof entry, "%s:%ld:%s", path, lineno, text);
      var_set_str(vm, "LAST", entry);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", entry);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_str(vm, "FORMATPATHLINE", entry);
      var_set_str(vm, "MKPATHLINE", entry);
      var_set_str(vm, "TOPATHLINE", entry);
      var_set_str(vm, "PATH", path);
      var_set_str(vm, "TEXT", text);
      var_set_str(vm, "PATHLINE_PATH", path);
      var_set_str(vm, "PATHLINE_TEXT", text);
      var_set_num(vm, "LINE_N", lineno);
      var_set_num(vm, "PATHLINE_N", lineno);
      var_set_num(vm, "FORMATPATHLINE_N", lineno);
      var_set_num(vm, "FORMATPATHLINE_HIT", 1);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys formatpathline → %s\n", entry);
      bump(vm); return 1;
    }
    /* SYS PATHLINEPATHS|PATHSFROMPL|UNIQPATHLINES [bag]
     * — unique paths from path:lineno:text bag (PATHLINETREE / grep -rn).
     * LAST = path bag · LAST_N / PATHLINEPATHS_N = unique count.
     * PATHLINEPATHS_SKIP = non-annotated fields skipped. Order = first-seen.
     * Bare uses LAST. Soft empty bag → empty OK=1.
     * Usability: which files hit without EACH+PARSEPATHLINE+HASLINE+PUSH. */
    if (kw(&L->cur,"PATHLINEPATHS") || kw(&L->cur,"PATHSFROMPL") ||
        kw(&L->cur,"UNIQPATHLINES") || kw(&L->cur,"PATHLINEFILES") ||
        kw(&L->cur,"PLPATHS") || kw(&L->cur,"UNIQPLPATHS") ||
        kw(&L->cur,"PATHSFROMPATHLINE")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      char path[CUBALC_HOST_STR_MAX], text[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t olen = 0, flen;
      long kept = 0, skip = 0, lineno = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
          snprintf(bag, sizeof bag, "%s", vm->last_str);
      } else {
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      }
      if (bag[0]) {
        p = bag;
        while (*p) {
          char field[CUBALC_HOST_STR_MAX];
          int hit, already;
          const char *q, *fs;
          size_t pln, qlen;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen >= sizeof field) flen = sizeof field - 1;
          if (flen > 0) {
            memcpy(field, start, flen);
            field[flen] = 0;
            path[0] = 0; text[0] = 0; lineno = 0;
            hit = cubalc_peel_pathline(field, path, sizeof path, &lineno, text, sizeof text);
            if (hit && path[0]) {
              /* unique: exact path already in out? */
              already = 0;
              pln = strlen(path);
              q = out;
              while (*q) {
                fs = q;
                while (*q && *q != '\n') q++;
                qlen = (size_t)(q - fs);
                if (qlen == pln && memcmp(fs, path, pln) == 0) {
                  already = 1;
                  break;
                }
                if (*q == '\n') q++;
              }
              if (!already) {
                size_t need = pln + (olen > 0 ? 1 : 0);
                if (olen + need < sizeof out) {
                  if (olen > 0) out[olen++] = '\n';
                  memcpy(out + olen, path, pln);
                  olen += pln;
                  out[olen] = 0;
                  kept++;
                }
              }
            } else {
              skip++;
            }
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_str(vm, "PATHLINEPATHS", out);
      var_set_str(vm, "PATHSFROMPL", out);
      var_set_str(vm, "UNIQPATHLINES", out);
      var_set_num(vm, "PATHLINEPATHS_N", kept);
      var_set_num(vm, "PATHSFROMPL_N", kept);
      var_set_num(vm, "UNIQPATHLINES_N", kept);
      var_set_num(vm, "PATHLINEPATHS_SKIP", skip);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys pathlinepaths → unique=%ld skip=%ld\n", kept, skip);
      bump(vm); return 1;
    }
    /* SYS PATHLINETEXTS|TEXTSFROMPL|PLTEXTS [bag]
     * — peel message text from every path:lineno:text field (all hits, order kept).
     * LAST = text bag · PATHLINETEXTS_N · PATHLINETEXTS_SKIP non-annotated.
     * Complements PATHLINEPATHS (unique files) with FREQ-ready message bodies.
     * Bare uses LAST. Soft empty bag → empty OK=1.
     * Usability: message harvest without EACH+PARSEPATHLINE+PUSH. */
    if (kw(&L->cur,"PATHLINETEXTS") || kw(&L->cur,"TEXTSFROMPL") ||
        kw(&L->cur,"PLTEXTS") || kw(&L->cur,"PATHLINEMSGS") ||
        kw(&L->cur,"MSGSFROMPL") || kw(&L->cur,"PATHLINEBODIES") ||
        kw(&L->cur,"BODIESFROMPL")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      char path[CUBALC_HOST_STR_MAX], text[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t olen = 0, flen;
      long kept = 0, skip = 0, lineno = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
          snprintf(bag, sizeof bag, "%s", vm->last_str);
      } else {
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      }
      if (bag[0]) {
        p = bag;
        while (*p) {
          char field[CUBALC_HOST_STR_MAX];
          int hit;
          size_t tln, need;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen >= sizeof field) flen = sizeof field - 1;
          if (flen > 0) {
            memcpy(field, start, flen);
            field[flen] = 0;
            path[0] = 0; text[0] = 0; lineno = 0;
            hit = cubalc_peel_pathline(field, path, sizeof path, &lineno, text, sizeof text);
            if (hit) {
              tln = strlen(text);
              need = tln + (olen > 0 ? 1 : 0);
              if (olen + need < sizeof out) {
                if (olen > 0) out[olen++] = '\n';
                if (tln > 0) {
                  memcpy(out + olen, text, tln);
                  olen += tln;
                }
                out[olen] = 0;
                kept++;
              }
            } else {
              skip++;
            }
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_str(vm, "PATHLINETEXTS", out);
      var_set_str(vm, "TEXTSFROMPL", out);
      var_set_str(vm, "PLTEXTS", out);
      var_set_num(vm, "PATHLINETEXTS_N", kept);
      var_set_num(vm, "TEXTSFROMPL_N", kept);
      var_set_num(vm, "PLTEXTS_N", kept);
      var_set_num(vm, "PATHLINETEXTS_SKIP", skip);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys pathlinetexts → texts=%ld skip=%ld\n", kept, skip);
      bump(vm); return 1;
    }
    /* SYS PATHLINEFREQ|COUNTBYPATH|PLFREQ [bag]
     * — path:count histogram from path:lineno:text bag (FREQ-compatible).
     * LAST = path:n bag · PATHLINEFREQ_N unique · PATHLINEFREQ_TOTAL hits.
     * PATHLINEFREQ_SKIP non-annotated. First-seen path order.
     * Usability: hottest files after PATHLINETREE without EACH+PARSEPATHLINE+KVINC. */
    if (kw(&L->cur,"PATHLINEFREQ") || kw(&L->cur,"COUNTBYPATH") ||
        kw(&L->cur,"PLFREQ") || kw(&L->cur,"PATHLINEHIST") ||
        kw(&L->cur,"HITBYPATH") || kw(&L->cur,"PATHHITCOUNT") ||
        kw(&L->cur,"FREQBYPATH")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      char path[CUBALC_HOST_STR_MAX], text[CUBALC_HOST_STR_MAX];
      char keys[128][512];
      long counts[128];
      const char *p, *start;
      size_t flen, olen = 0;
      long total = 0, skip = 0, lineno = 0;
      int nkeys = 0, i;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      memset(counts, 0, sizeof counts);
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
          snprintf(bag, sizeof bag, "%s", vm->last_str);
      } else {
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      }
      if (bag[0]) {
        p = bag;
        while (*p) {
          char field[CUBALC_HOST_STR_MAX];
          int hit, found;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen >= sizeof field) flen = sizeof field - 1;
          if (flen > 0) {
            memcpy(field, start, flen);
            field[flen] = 0;
            path[0] = 0; text[0] = 0; lineno = 0;
            hit = cubalc_peel_pathline(field, path, sizeof path, &lineno, text, sizeof text);
            if (hit && path[0]) {
              found = -1;
              for (i = 0; i < nkeys; i++) {
                if (strcmp(keys[i], path) == 0) { found = i; break; }
              }
              if (found >= 0) {
                counts[found]++;
              } else if (nkeys < 128) {
                snprintf(keys[nkeys], sizeof keys[nkeys], "%s", path);
                counts[nkeys] = 1;
                nkeys++;
              }
              total++;
            } else {
              skip++;
            }
          }
          if (*p == '\n') p++;
        }
      }
      for (i = 0; i < nkeys; i++) {
        char entry[640];
        size_t eln;
        snprintf(entry, sizeof entry, "%s:%ld", keys[i], counts[i]);
        eln = strlen(entry);
        if (i > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        if (olen + eln < sizeof out) {
          memcpy(out + olen, entry, eln);
          olen += eln;
          out[olen] = 0;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)nkeys;
      var_set_num(vm, "LAST_N", (long)nkeys);
      var_set_str(vm, "PATHLINEFREQ", out);
      var_set_str(vm, "COUNTBYPATH", out);
      var_set_str(vm, "PLFREQ", out);
      var_set_num(vm, "PATHLINEFREQ_N", (long)nkeys);
      var_set_num(vm, "COUNTBYPATH_N", (long)nkeys);
      var_set_num(vm, "PLFREQ_N", (long)nkeys);
      var_set_num(vm, "PATHLINEFREQ_TOTAL", total);
      var_set_num(vm, "PATHLINEFREQ_SKIP", skip);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys pathlinefreq → unique=%d total=%ld skip=%ld\n",
                nkeys, total, skip);
      bump(vm); return 1;
    }
    /* SYS FIRSTINTREE|GREP1TREE|LASTINTREE root needle — first/last content-matching file.
     * LAST = path · LAST_N 0|1 · FIRSTINTREE_HIT. FIRSTINTREEI / LASTINTREEI icase.
     * Soft empty/missing. Empty needle → first/last readable regular file.
     * Usability: one-shot pick under tree without GREPTREE+TAKE/REVL glue. */
    if (kw(&L->cur,"FIRSTINTREE") || kw(&L->cur,"GREP1TREE") || kw(&L->cur,"HITINTREE") ||
        kw(&L->cur,"FINDINTREE") || kw(&L->cur,"TREEFIRST") ||
        kw(&L->cur,"FIRSTINTREEI") || kw(&L->cur,"GREP1TREEI") || kw(&L->cur,"HITINTREEI") ||
        kw(&L->cur,"LASTINTREE") || kw(&L->cur,"GREP1TREEL") || kw(&L->cur,"HITINTREEL") ||
        kw(&L->cur,"TREELAST") || kw(&L->cur,"LASTINTREEI") || kw(&L->cur,"GREP1TREELI") ||
        kw(&L->cur,"HITINTREELI")){
      char root[CUBALC_HOST_STR_MAX], needle[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      int icase = 0, want_last = 0;
      long hit;
      if (kw(&L->cur,"LASTINTREE") || kw(&L->cur,"GREP1TREEL") || kw(&L->cur,"HITINTREEL") ||
          kw(&L->cur,"TREELAST") || kw(&L->cur,"LASTINTREEI") || kw(&L->cur,"GREP1TREELI") ||
          kw(&L->cur,"HITINTREELI"))
        want_last = 1;
      if (kw(&L->cur,"FIRSTINTREEI") || kw(&L->cur,"GREP1TREEI") || kw(&L->cur,"HITINTREEI") ||
          kw(&L->cur,"LASTINTREEI") || kw(&L->cur,"GREP1TREELI") || kw(&L->cur,"HITINTREELI"))
        icase = 1;
      lex_next(L);
      root[0] = 0; needle[0] = 0;
      if (resolve_str_arg(vm, L, root, sizeof root) != 0) {
        fail(vm, want_last ? "SYS LASTINTREE root needle" : "SYS FIRSTINTREE root needle");
        return -1;
      }
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long nv = parse_expr(vm, L);
          snprintf(needle, sizeof needle, "%ld", nv);
        } else if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
          needle[0] = 0;
        }
      }
      if (!root[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "FIRSTINTREE_N", 0);
        var_set_num(vm, "FIRSTINTREE_HIT", 0);
        var_set_num(vm, "LASTINTREE_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", want_last ? "LASTINTREE: empty path" : "FIRSTINTREE: empty path");
        var_set_str(vm, "ERR", want_last ? "LASTINTREE: empty path" : "FIRSTINTREE: empty path");
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_findintree(root, needle, icase, want_last, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "FIRSTINTREE_N", 0);
        var_set_num(vm, "FIRSTINTREE_HIT", 0);
        var_set_num(vm, "LASTINTREE_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", want_last ? "LASTINTREE: fail" : "FIRSTINTREE: fail");
          var_set_str(vm, "ERR", want_last ? "LASTINTREE: fail" : "FIRSTINTREE: fail");
        }
        bump(vm); return 1;
      }
      hit = hr.n ? 1 : 0;
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      if (want_last) {
        var_set_str(vm, "LASTINTREE", hr.str);
        var_set_str(vm, "GREP1TREEL", hr.str);
        var_set_num(vm, "LASTINTREE_N", hit);
        var_set_num(vm, "LASTINTREE_HIT", hit);
      } else {
        var_set_str(vm, "FIRSTINTREE", hr.str);
        var_set_str(vm, "GREP1TREE", hr.str);
        var_set_num(vm, "FIRSTINTREE_N", hit);
        var_set_num(vm, "FIRSTINTREE_HIT", hit);
      }
      var_set_str(vm, "PATH", hr.str);
      var_set_num(vm, "PATH_EXIST", hit);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys %sintree → hit=%ld path=%s icase=%d\n",
                want_last ? "last" : "first", hit, hr.str, icase);
      bump(vm); return 1;
    }
    /* SYS HASINTREE|ANYINTREE|CONTAINSINTREE root needle
     * — soft 0|1 if any file under tree contains needle (early-stop).
     * LAST_N / HASINTREE_HIT = 0|1 · PATH set on hit. HASINTREEI icase.
     * Soft empty/missing root. Does not clobber LAST string (probe like HASLINE).
     * Usability: IF guards without FIRSTINTREE+HIT or GREPTREE+EMPTY glue. */
    if (kw(&L->cur,"HASINTREE") || kw(&L->cur,"ANYINTREE") ||
        kw(&L->cur,"CONTAINSINTREE") || kw(&L->cur,"TREEHAS") ||
        kw(&L->cur,"HASTREE") || kw(&L->cur,"ANYINTREECONTENT") ||
        kw(&L->cur,"HASINTREEI") || kw(&L->cur,"ANYINTREEI") ||
        kw(&L->cur,"CONTAINSINTREEI") || kw(&L->cur,"TREEHASI")){
      char root[CUBALC_HOST_STR_MAX], needle[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      int icase = 0;
      long hit;
      if (kw(&L->cur,"HASINTREEI") || kw(&L->cur,"ANYINTREEI") ||
          kw(&L->cur,"CONTAINSINTREEI") || kw(&L->cur,"TREEHASI"))
        icase = 1;
      lex_next(L);
      root[0] = 0; needle[0] = 0;
      if (resolve_str_arg(vm, L, root, sizeof root) != 0) {
        fail(vm, "SYS HASINTREE root needle");
        return -1;
      }
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long nv = parse_expr(vm, L);
          snprintf(needle, sizeof needle, "%ld", nv);
        } else if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
          needle[0] = 0;
        }
      }
      if (!root[0]) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "HASINTREE_N", 0);
        var_set_num(vm, "HASINTREE_HIT", 0);
        var_set_str(vm, "PATH", "");
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "HASINTREE: empty path");
        var_set_str(vm, "ERR", "HASINTREE: empty path");
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_findintree(root, needle, icase, 0, &hr) != 0) {
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "HASINTREE_N", 0);
        var_set_num(vm, "HASINTREE_HIT", 0);
        var_set_str(vm, "PATH", "");
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "HASINTREE: fail");
          var_set_str(vm, "ERR", "HASINTREE: fail");
        }
        bump(vm); return 1;
      }
      hit = hr.n ? 1 : 0;
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "HASINTREE_N", hit);
      var_set_num(vm, "HASINTREE_HIT", hit);
      var_set_num(vm, "ANYINTREE_N", hit);
      var_set_str(vm, "PATH", hit ? hr.str : "");
      var_set_num(vm, "PATH_EXIST", hit);
      if (hit) {
        var_set_str(vm, "HASINTREE_PATH", hr.str);
        var_set_str(vm, "ANYINTREE_PATH", hr.str);
      } else {
        var_set_str(vm, "HASINTREE_PATH", "");
        var_set_str(vm, "ANYINTREE_PATH", "");
      }
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys hasintree → hit=%ld path=%s icase=%d\n",
                hit, hit ? hr.str : "", icase);
      bump(vm); return 1;
    }
    /* SYS TAKEINTREE|FIRSTNINTREE|HEADINTREE root needle n
     * — first n content-matching paths under tree (sorted DFS path bag).
     * LAST = path bag · TAKEINTREE_N · TAKEINTREEI icase. Soft empty/missing.
     * Usability: sample top hits without GREPTREE+TAKE glue. */
    if (kw(&L->cur,"TAKEINTREE") || kw(&L->cur,"FIRSTNINTREE") ||
        kw(&L->cur,"HEADINTREE") || kw(&L->cur,"TAKETREE") ||
        kw(&L->cur,"FIRSTNTREE") || kw(&L->cur,"LIMITINTREE") ||
        kw(&L->cur,"TAKEINTREEI") || kw(&L->cur,"FIRSTNINTREEI") ||
        kw(&L->cur,"HEADINTREEI") || kw(&L->cur,"TAKETREEI")){
      char root[CUBALC_HOST_STR_MAX], needle[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      int icase = 0;
      long maxn = 1;
      if (kw(&L->cur,"TAKEINTREEI") || kw(&L->cur,"FIRSTNINTREEI") ||
          kw(&L->cur,"HEADINTREEI") || kw(&L->cur,"TAKETREEI"))
        icase = 1;
      lex_next(L);
      root[0] = 0; needle[0] = 0;
      if (resolve_str_arg(vm, L, root, sizeof root) != 0) {
        fail(vm, "SYS TAKEINTREE root needle n");
        return -1;
      }
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long nv = parse_expr(vm, L);
          snprintf(needle, sizeof needle, "%ld", nv);
        } else if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
          needle[0] = 0;
        }
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_IDENT) {
        maxn = parse_expr(vm, L);
      }
      if (maxn < 0) maxn = 0;
      if (!root[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "TAKEINTREE_N", 0);
        var_set_num(vm, "FIRSTNINTREE_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "TAKEINTREE: empty path");
        var_set_str(vm, "ERR", "TAKEINTREE: empty path");
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_takeintree(root, needle, icase, maxn, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "TAKEINTREE_N", 0);
        var_set_num(vm, "FIRSTNINTREE_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "TAKEINTREE: fail");
          var_set_str(vm, "ERR", "TAKEINTREE: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "TAKEINTREE", hr.str);
      var_set_str(vm, "FIRSTNINTREE", hr.str);
      var_set_str(vm, "HEADINTREE", hr.str);
      var_set_num(vm, "TAKEINTREE_N", hr.n);
      var_set_num(vm, "FIRSTNINTREE_N", hr.n);
      var_set_num(vm, "HEADINTREE_N", hr.n);
      var_set_num(vm, "TAKEINTREE_MAX", maxn);
      var_set_str(vm, "TAKEINTREE_ROOT", root);
      var_set_str(vm, "TAKEINTREE_NEEDLE", needle);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys takeintree → n=%ld max=%ld icase=%d\n",
                hr.n, maxn, icase);
      bump(vm); return 1;
    }
    /* SYS NTHINTREE|INDEXINTREE root needle index
     * — 0-based Nth content-matching path under tree (sorted DFS, early-stop).
     * LAST=path · LAST_N 0|1 · NTHINTREE_HIT · NTHINTREEI icase. Soft empty/missing.
     * Usability: pick k-th hit without GREPTREE+NTH or TAKEINTREE+NTH glue. */
    if (kw(&L->cur,"NTHINTREE") || kw(&L->cur,"INDEXINTREE") ||
        kw(&L->cur,"ATINTREE") || kw(&L->cur,"NTHTREE") ||
        kw(&L->cur,"TREEINDEX") || kw(&L->cur,"PICKINTREE") ||
        kw(&L->cur,"NTHINTREEI") || kw(&L->cur,"INDEXINTREEI") ||
        kw(&L->cur,"ATINTREEI") || kw(&L->cur,"NTHTREEI")){
      char root[CUBALC_HOST_STR_MAX], needle[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      int icase = 0;
      long index = 0, hit;
      if (kw(&L->cur,"NTHINTREEI") || kw(&L->cur,"INDEXINTREEI") ||
          kw(&L->cur,"ATINTREEI") || kw(&L->cur,"NTHTREEI"))
        icase = 1;
      lex_next(L);
      root[0] = 0; needle[0] = 0;
      if (resolve_str_arg(vm, L, root, sizeof root) != 0) {
        fail(vm, "SYS NTHINTREE root needle index");
        return -1;
      }
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long nv = parse_expr(vm, L);
          snprintf(needle, sizeof needle, "%ld", nv);
        } else if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
          needle[0] = 0;
        }
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_IDENT) {
        index = parse_expr(vm, L);
      }
      if (!root[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "NTHINTREE_N", 0);
        var_set_num(vm, "NTHINTREE_HIT", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "NTHINTREE: empty path");
        var_set_str(vm, "ERR", "NTHINTREE: empty path");
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_nthintree(root, needle, icase, index, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "NTHINTREE_N", 0);
        var_set_num(vm, "NTHINTREE_HIT", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "NTHINTREE: fail");
          var_set_str(vm, "ERR", "NTHINTREE: fail");
        }
        bump(vm); return 1;
      }
      hit = hr.n ? 1 : 0;
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_str(vm, "NTHINTREE", hr.str);
      var_set_str(vm, "INDEXINTREE", hr.str);
      var_set_num(vm, "NTHINTREE_N", hit);
      var_set_num(vm, "NTHINTREE_HIT", hit);
      var_set_num(vm, "INDEXINTREE_N", hit);
      var_set_num(vm, "NTHINTREE_I", index);
      var_set_str(vm, "PATH", hr.str);
      var_set_num(vm, "PATH_EXIST", hit);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys nthintree → hit=%ld i=%ld path=%s icase=%d\n",
                hit, index, hr.str, icase);
      bump(vm); return 1;
    }
    /* SYS SLICEINTREE|MIDINTREE|WINDOWINTREE root needle start count
     * — path window [start,start+count) of content matches under tree (sorted DFS).
     * LAST = path bag · SLICEINTREE_N · SLICEINTREEI icase. Soft empty/missing.
     * Usability: page tree hits without GREPTREE+DROP+TAKE glue. */
    if (kw(&L->cur,"SLICEINTREE") || kw(&L->cur,"MIDINTREE") ||
        kw(&L->cur,"WINDOWINTREE") || kw(&L->cur,"SLICEBAGINTREE") ||
        kw(&L->cur,"PAGEINTREE") || kw(&L->cur,"RANGEINTREE") ||
        kw(&L->cur,"SLICEINTREEI") || kw(&L->cur,"MIDINTREEI") ||
        kw(&L->cur,"WINDOWINTREEI") || kw(&L->cur,"PAGEINTREEI")){
      char root[CUBALC_HOST_STR_MAX], needle[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      int icase = 0;
      long start = 0, count = 1;
      if (kw(&L->cur,"SLICEINTREEI") || kw(&L->cur,"MIDINTREEI") ||
          kw(&L->cur,"WINDOWINTREEI") || kw(&L->cur,"PAGEINTREEI"))
        icase = 1;
      lex_next(L);
      root[0] = 0; needle[0] = 0;
      if (resolve_str_arg(vm, L, root, sizeof root) != 0) {
        fail(vm, "SYS SLICEINTREE root needle start count");
        return -1;
      }
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long nv = parse_expr(vm, L);
          snprintf(needle, sizeof needle, "%ld", nv);
        } else if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
          needle[0] = 0;
        }
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_IDENT) {
        start = parse_expr(vm, L);
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_IDENT) {
        count = parse_expr(vm, L);
      }
      if (start < 0) start = 0;
      if (count < 0) count = 0;
      if (!root[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SLICEINTREE_N", 0);
        var_set_num(vm, "MIDINTREE_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "SLICEINTREE: empty path");
        var_set_str(vm, "ERR", "SLICEINTREE: empty path");
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_sliceintree(root, needle, icase, start, count, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SLICEINTREE_N", 0);
        var_set_num(vm, "MIDINTREE_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "SLICEINTREE: fail");
          var_set_str(vm, "ERR", "SLICEINTREE: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "SLICEINTREE", hr.str);
      var_set_str(vm, "MIDINTREE", hr.str);
      var_set_str(vm, "WINDOWINTREE", hr.str);
      var_set_num(vm, "SLICEINTREE_N", hr.n);
      var_set_num(vm, "MIDINTREE_N", hr.n);
      var_set_num(vm, "WINDOWINTREE_N", hr.n);
      var_set_num(vm, "SLICEINTREE_START", start);
      var_set_num(vm, "SLICEINTREE_COUNT", count);
      var_set_str(vm, "SLICEINTREE_ROOT", root);
      var_set_str(vm, "SLICEINTREE_NEEDLE", needle);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys sliceintree → n=%ld start=%ld count=%ld icase=%d\n",
                hr.n, start, count, icase);
      bump(vm); return 1;
    }
    /* SYS DROPINTREE|SKIPINTREE|RESTINTREE root needle n
     * — skip first n content-matching paths under tree → remaining path bag.
     * LAST = path bag · DROPINTREE_N · DROPINTREEI icase. Soft empty/missing.
     * Usability: residual after TAKE page without GREPTREE+DROP glue. Dual of TAKEINTREE. */
    if (kw(&L->cur,"DROPINTREE") || kw(&L->cur,"SKIPINTREE") ||
        kw(&L->cur,"RESTINTREE") || kw(&L->cur,"DROPTREE") ||
        kw(&L->cur,"SKIPNTREE") || kw(&L->cur,"TAILRESTINTREE") ||
        kw(&L->cur,"DROPINTREEI") || kw(&L->cur,"SKIPINTREEI") ||
        kw(&L->cur,"RESTINTREEI") || kw(&L->cur,"DROPTREEI")){
      char root[CUBALC_HOST_STR_MAX], needle[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      int icase = 0;
      long skipn = 0;
      if (kw(&L->cur,"DROPINTREEI") || kw(&L->cur,"SKIPINTREEI") ||
          kw(&L->cur,"RESTINTREEI") || kw(&L->cur,"DROPTREEI"))
        icase = 1;
      lex_next(L);
      root[0] = 0; needle[0] = 0;
      if (resolve_str_arg(vm, L, root, sizeof root) != 0) {
        fail(vm, "SYS DROPINTREE root needle n");
        return -1;
      }
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long nv = parse_expr(vm, L);
          snprintf(needle, sizeof needle, "%ld", nv);
        } else if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
          needle[0] = 0;
        }
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_IDENT) {
        skipn = parse_expr(vm, L);
      }
      if (skipn < 0) skipn = 0;
      if (!root[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DROPINTREE_N", 0);
        var_set_num(vm, "SKIPINTREE_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "DROPINTREE: empty path");
        var_set_str(vm, "ERR", "DROPINTREE: empty path");
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_dropintree(root, needle, icase, skipn, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DROPINTREE_N", 0);
        var_set_num(vm, "SKIPINTREE_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "DROPINTREE: fail");
          var_set_str(vm, "ERR", "DROPINTREE: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "DROPINTREE", hr.str);
      var_set_str(vm, "SKIPINTREE", hr.str);
      var_set_str(vm, "RESTINTREE", hr.str);
      var_set_num(vm, "DROPINTREE_N", hr.n);
      var_set_num(vm, "SKIPINTREE_N", hr.n);
      var_set_num(vm, "RESTINTREE_N", hr.n);
      var_set_num(vm, "DROPINTREE_SKIP", skipn);
      var_set_str(vm, "DROPINTREE_ROOT", root);
      var_set_str(vm, "DROPINTREE_NEEDLE", needle);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys dropintree → n=%ld skip=%ld icase=%d\n",
                hr.n, skipn, icase);
      bump(vm); return 1;
    }
    /* SYS FIRSTLINEINTREE|GREP1LINETREE|LASTLINEINTREE root needle —
     * first/last matching *line* under tree. LAST=line · PATH=file · *_HIT · *_I.
     * FIRSTLINEINTREEI / LASTLINEINTREEI icase. Soft empty/missing.
     * Usability: one-shot line peel without GREPLINESTREE+TAKE/REVL glue. */
    if (kw(&L->cur,"FIRSTLINEINTREE") || kw(&L->cur,"GREP1LINETREE") ||
        kw(&L->cur,"HITLINEINTREE") || kw(&L->cur,"FINDLINEINTREE") ||
        kw(&L->cur,"TREEFIRSTLINE") ||
        kw(&L->cur,"FIRSTLINEINTREEI") || kw(&L->cur,"GREP1LINETREEI") ||
        kw(&L->cur,"HITLINEINTREEI") ||
        kw(&L->cur,"LASTLINEINTREE") || kw(&L->cur,"GREP1LINETREEL") ||
        kw(&L->cur,"HITLINEINTREEL") || kw(&L->cur,"TREELASTLINE") ||
        kw(&L->cur,"LASTLINEINTREEI") || kw(&L->cur,"GREP1LINETREELI") ||
        kw(&L->cur,"HITLINEINTREELI")){
      char root[CUBALC_HOST_STR_MAX], needle[CUBALC_HOST_STR_MAX];
      char path[CUBALC_HOST_STR_MAX], line[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      int icase = 0, want_last = 0;
      long hit;
      char *nl;
      if (kw(&L->cur,"LASTLINEINTREE") || kw(&L->cur,"GREP1LINETREEL") ||
          kw(&L->cur,"HITLINEINTREEL") || kw(&L->cur,"TREELASTLINE") ||
          kw(&L->cur,"LASTLINEINTREEI") || kw(&L->cur,"GREP1LINETREELI") ||
          kw(&L->cur,"HITLINEINTREELI"))
        want_last = 1;
      if (kw(&L->cur,"FIRSTLINEINTREEI") || kw(&L->cur,"GREP1LINETREEI") ||
          kw(&L->cur,"HITLINEINTREEI") || kw(&L->cur,"LASTLINEINTREEI") ||
          kw(&L->cur,"GREP1LINETREELI") || kw(&L->cur,"HITLINEINTREELI"))
        icase = 1;
      lex_next(L);
      root[0] = 0; needle[0] = 0;
      if (resolve_str_arg(vm, L, root, sizeof root) != 0) {
        fail(vm, want_last ? "SYS LASTLINEINTREE root needle"
                           : "SYS FIRSTLINEINTREE root needle");
        return -1;
      }
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long nv = parse_expr(vm, L);
          snprintf(needle, sizeof needle, "%ld", nv);
        } else if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
          needle[0] = 0;
        }
      }
      if (!root[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "FIRSTLINEINTREE_N", 0);
        var_set_num(vm, "FIRSTLINEINTREE_HIT", 0);
        var_set_num(vm, "LASTLINEINTREE_N", 0);
        var_set_num(vm, "LASTLINEINTREE_HIT", 0);
        var_set_str(vm, "FIRSTLINEINTREE_PATH", "");
        var_set_str(vm, "LASTLINEINTREE_PATH", "");
        var_set_num(vm, "FIRSTLINEINTREE_I", -1);
        var_set_num(vm, "LASTLINEINTREE_I", -1);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR",
                    want_last ? "LASTLINEINTREE: empty path" : "FIRSTLINEINTREE: empty path");
        var_set_str(vm, "ERR",
                    want_last ? "LASTLINEINTREE: empty path" : "FIRSTLINEINTREE: empty path");
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_findlineintree(root, needle, icase, want_last, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "FIRSTLINEINTREE_N", 0);
        var_set_num(vm, "FIRSTLINEINTREE_HIT", 0);
        var_set_num(vm, "LASTLINEINTREE_N", 0);
        var_set_num(vm, "LASTLINEINTREE_HIT", 0);
        var_set_str(vm, "FIRSTLINEINTREE_PATH", "");
        var_set_str(vm, "LASTLINEINTREE_PATH", "");
        var_set_num(vm, "FIRSTLINEINTREE_I", -1);
        var_set_num(vm, "LASTLINEINTREE_I", -1);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR",
                      want_last ? "LASTLINEINTREE: fail" : "FIRSTLINEINTREE: fail");
          var_set_str(vm, "ERR",
                      want_last ? "LASTLINEINTREE: fail" : "FIRSTLINEINTREE: fail");
        }
        bump(vm); return 1;
      }
      path[0] = 0; line[0] = 0;
      hit = hr.n ? 1 : 0;
      if (hit && hr.str[0]) {
        nl = strchr(hr.str, '\n');
        if (nl) {
          size_t pl = (size_t)(nl - hr.str);
          if (pl >= sizeof path) pl = sizeof path - 1;
          memcpy(path, hr.str, pl);
          path[pl] = 0;
          snprintf(line, sizeof line, "%s", nl + 1);
        } else {
          snprintf(path, sizeof path, "%s", hr.str);
          line[0] = 0;
        }
      }
      var_set_str(vm, "LAST", line);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", line);
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      if (want_last) {
        var_set_str(vm, "LASTLINEINTREE", line);
        var_set_str(vm, "GREP1LINETREEL", line);
        var_set_str(vm, "LASTLINEINTREE_PATH", path);
        var_set_num(vm, "LASTLINEINTREE_N", hit);
        var_set_num(vm, "LASTLINEINTREE_HIT", hit);
        var_set_num(vm, "LASTLINEINTREE_I", hit ? (long)hr.code : -1);
      } else {
        var_set_str(vm, "FIRSTLINEINTREE", line);
        var_set_str(vm, "GREP1LINETREE", line);
        var_set_str(vm, "FIRSTLINEINTREE_PATH", path);
        var_set_num(vm, "FIRSTLINEINTREE_N", hit);
        var_set_num(vm, "FIRSTLINEINTREE_HIT", hit);
        var_set_num(vm, "FIRSTLINEINTREE_I", hit ? (long)hr.code : -1);
      }
      var_set_str(vm, "PATH", path);
      var_set_num(vm, "PATH_EXIST", hit);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys %slineintree → hit=%ld path=%s line=%s icase=%d\n",
                want_last ? "last" : "first", hit, path, line, icase);
      bump(vm); return 1;
    }
    /* SYS SYMLINK|LN|LINK target linkpath — create symlink (plate alias).
     * LAST = linkpath; LAST_N/SYMLINK_N = 1; soft fail if link exists or empty.
     * Usability: plate layout aliases without shell ln -s. */
    if (kw(&L->cur,"SYMLINK") || kw(&L->cur,"LN") || kw(&L->cur,"MAKELINK") ||
        kw(&L->cur,"LINKTO") || kw(&L->cur,"SOFTLINK") || kw(&L->cur,"SYMLN") ||
        kw(&L->cur,"MLINK")){
      char target[512], linkpath[512];
      cubalc_host_result hr;
      memset(&hr, 0, sizeof hr);
      lex_next(L);
      target[0] = 0; linkpath[0] = 0;
      if (resolve_str_arg(vm, L, target, sizeof target) != 0) {
        fail(vm, "SYS SYMLINK target linkpath");
        return -1;
      }
      if (resolve_str_arg(vm, L, linkpath, sizeof linkpath) != 0) {
        fail(vm, "SYS SYMLINK target linkpath");
        return -1;
      }
      if (!target[0] || !linkpath[0] || cubalc_host_symlink(target, linkpath, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SYMLINK_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "SYMLINK: fail");
          var_set_str(vm, "ERR", "SYMLINK: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", linkpath);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", linkpath);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "SYMLINK_N", hr.n);
      var_set_str(vm, "SYMLINK", linkpath);
      var_set_str(vm, "SYMLINK_TARGET", target);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS HARDLINK|HLINK|LINKHARD existing newpath — hard link (same inode).
     * LAST = newpath; LAST_N / HARDLINK_N = 1; soft fail if dest exists / src miss.
     * Usability: multi-agent shared plates without COPY shell ln; dual of SYMLINK. */
    if (kw(&L->cur,"HARDLINK") || kw(&L->cur,"HLINK") || kw(&L->cur,"LINKHARD") ||
        kw(&L->cur,"HLN") || kw(&L->cur,"HARDLN") || kw(&L->cur,"LINKFILE") ||
        kw(&L->cur,"MAKEHARD") || kw(&L->cur,"HLINKTO") || kw(&L->cur,"LN_H")){
      char existing[512], newpath[512];
      cubalc_host_result hr;
      memset(&hr, 0, sizeof hr);
      lex_next(L);
      existing[0] = 0; newpath[0] = 0;
      if (resolve_str_arg(vm, L, existing, sizeof existing) != 0) {
        fail(vm, "SYS HARDLINK existing newpath");
        return -1;
      }
      if (resolve_str_arg(vm, L, newpath, sizeof newpath) != 0) {
        fail(vm, "SYS HARDLINK existing newpath");
        return -1;
      }
      if (!existing[0] || !newpath[0] || cubalc_host_hardlink(existing, newpath, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "HARDLINK_N", 0);
        var_set_num(vm, "HLINK_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "HARDLINK: fail");
          var_set_str(vm, "ERR", "HARDLINK: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", newpath);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", newpath);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "HARDLINK_N", hr.n);
      var_set_num(vm, "HLINK_N", hr.n);
      var_set_str(vm, "HARDLINK", newpath);
      var_set_str(vm, "HARDLINK_SRC", existing);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS NLINK|LINKS|LINKCOUNT path — hard-link count → LAST_N / NLINK.
     * Soft miss missing path → OK=0. Usability: IF after HARDLINK without stat shell. */
    if (kw(&L->cur,"NLINK") || kw(&L->cur,"LINKS") || kw(&L->cur,"LINKCOUNT") ||
        kw(&L->cur,"HARDLINKCOUNT") || kw(&L->cur,"ST_NLINK") || kw(&L->cur,"NUMLINKS") ||
        kw(&L->cur,"REFLINKCOUNT")){
      char path[512];
      cubalc_host_result hr;
      memset(&hr, 0, sizeof hr);
      lex_next(L);
      path[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0)
        snprintf(path, sizeof path, "%s", vm->last_str);
      if (!path[0] || cubalc_host_nlink(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "NLINK", 0);
        var_set_num(vm, "NLINK_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "NLINK: fail");
          var_set_str(vm, "ERR", "NLINK: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "NLINK", hr.n);
      var_set_num(vm, "NLINK_N", hr.n);
      var_set_num(vm, "LINKCOUNT", hr.n);
      var_set_str(vm, "NLINK_PATH", path);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SAMEINODE|EQINODE|SAMEFILEINODE a b — 1 if same device+inode (hard-link pair).
     * LAST_N / SAMEINODE_N = 0|1; soft miss either path → OK=0 (not content EQFILE).
     * Usability: verify HARDLINK without shell stat/cmp inode glue. */
    if (kw(&L->cur,"SAMEINODE") || kw(&L->cur,"EQINODE") || kw(&L->cur,"SAMEFILEINODE") ||
        kw(&L->cur,"IDENTINODE") || kw(&L->cur,"SAMEINO") || kw(&L->cur,"INODEEQ") ||
        kw(&L->cur,"HARDLINKEQ") || kw(&L->cur,"SAMELINK") || kw(&L->cur,"ISHARDLINK")){
      char a[512], b[512];
      cubalc_host_result hr;
      memset(&hr, 0, sizeof hr);
      lex_next(L);
      a[0] = 0; b[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS SAMEINODE pathA pathB");
        return -1;
      }
      if (resolve_str_arg(vm, L, b, sizeof b) != 0) {
        fail(vm, "SYS SAMEINODE pathA pathB");
        return -1;
      }
      if (!a[0] || !b[0] || cubalc_host_sameinode(a, b, &hr) != 0) {
        var_set_str(vm, "LAST", "0");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "0");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SAMEINODE_N", 0);
        var_set_num(vm, "EQINODE_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "SAMEINODE: fail");
          var_set_str(vm, "ERR", "SAMEINODE: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "SAMEINODE_N", hr.n);
      var_set_num(vm, "EQINODE_N", hr.n);
      var_set_num(vm, "SAMEINODE", hr.n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS INODE|INO path — inode number → LAST_N / INODE (soft miss).
     * Usability: plate identity stamp / match without shell stat. */
    if (kw(&L->cur,"INODE") || kw(&L->cur,"INO") || kw(&L->cur,"FILENO") ||
        kw(&L->cur,"ST_INO") || kw(&L->cur,"GETINODE") || kw(&L->cur,"INODEOF")){
      char path[512];
      cubalc_host_result hr;
      memset(&hr, 0, sizeof hr);
      lex_next(L);
      path[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0)
        snprintf(path, sizeof path, "%s", vm->last_str);
      if (!path[0] || cubalc_host_inode(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "INODE", 0);
        var_set_num(vm, "INODE_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "INODE: fail");
          var_set_str(vm, "ERR", "INODE: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "INODE", hr.n);
      var_set_num(vm, "INODE_N", hr.n);
      var_set_str(vm, "INODE_PATH", path);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS FILEUID|OWNERUID|FUID path — st_uid → LAST_N / FILEUID (soft miss).
     * SYS FILEGID|OWNERGID|FGID path — st_gid → LAST_N / FILEGID.
     * SYS OWNEDBY|ISOWNER|MYFILE path — LAST_N 1 if geteuid()==st_uid.
     * Usability: agent plate ownership gates without shell stat. */
    if (kw(&L->cur,"FILEUID") || kw(&L->cur,"OWNERUID") || kw(&L->cur,"FUID") ||
        kw(&L->cur,"ST_UID") || kw(&L->cur,"FILEOWNER") || kw(&L->cur,"UIDOF") ||
        kw(&L->cur,"FILEGID") || kw(&L->cur,"OWNERGID") || kw(&L->cur,"FGID") ||
        kw(&L->cur,"ST_GID") || kw(&L->cur,"FILEGROUP") || kw(&L->cur,"GIDOF") ||
        kw(&L->cur,"OWNEDBY") || kw(&L->cur,"ISOWNER") || kw(&L->cur,"MYFILE") ||
        kw(&L->cur,"IAMOWNER") || kw(&L->cur,"OWNEDBYME") || kw(&L->cur,"ISOWNED")){
      char path[512], op[24];
      cubalc_host_result hr;
      int is_gid, is_owned;
      long euid;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_gid = (strcmp(op, "FILEGID") == 0 || strcmp(op, "OWNERGID") == 0 ||
                strcmp(op, "FGID") == 0 || strcmp(op, "ST_GID") == 0 ||
                strcmp(op, "FILEGROUP") == 0 || strcmp(op, "GIDOF") == 0);
      is_owned = (strcmp(op, "OWNEDBY") == 0 || strcmp(op, "ISOWNER") == 0 ||
                  strcmp(op, "MYFILE") == 0 || strcmp(op, "IAMOWNER") == 0 ||
                  strcmp(op, "OWNEDBYME") == 0 || strcmp(op, "ISOWNED") == 0);
      memset(&hr, 0, sizeof hr);
      lex_next(L);
      path[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0)
        snprintf(path, sizeof path, "%s", vm->last_str);
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", is_owned ? "OWNEDBY: empty path" :
                    (is_gid ? "FILEGID: empty path" : "FILEUID: empty path"));
        var_set_str(vm, "ERR", is_owned ? "OWNEDBY: empty path" :
                    (is_gid ? "FILEGID: empty path" : "FILEUID: empty path"));
        bump(vm); return 1;
      }
      if (is_owned) {
        if (cubalc_host_fileuid(path, &hr) != 0) {
          var_set_str(vm, "LAST", "0");
          snprintf(vm->last_str, sizeof vm->last_str, "%s", "0");
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "OWNEDBY_N", 0);
          var_set_num(vm, "ISOWNER", 0);
          var_set_num(vm, "OK", 0);
          if (hr.err[0]) {
            var_set_str(vm, "LAST_ERR", hr.err);
            var_set_str(vm, "ERR", hr.err);
          } else {
            var_set_str(vm, "LAST_ERR", "OWNEDBY: fail");
            var_set_str(vm, "ERR", "OWNEDBY: fail");
          }
          bump(vm); return 1;
        }
#if !defined(CUBALC_OS_WINDOWS)
        euid = (long)geteuid();
#else
        euid = 0;
#endif
        {
          long hit = (hr.n == euid) ? 1 : 0;
          snprintf(vm->last_str, sizeof vm->last_str, "%ld", hit);
          var_set_str(vm, "LAST", vm->last_str);
          vm->last_n = hit;
          var_set_num(vm, "LAST_N", hit);
          var_set_num(vm, "OWNEDBY_N", hit);
          var_set_num(vm, "ISOWNER", hit);
          var_set_num(vm, "MYFILE", hit);
          var_set_num(vm, "FILEUID", hr.n);
          var_set_num(vm, "OK", 1);
          bump(vm); return 1;
        }
      }
      if (is_gid) {
        if (cubalc_host_filegid(path, &hr) != 0) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "FILEGID", 0);
          var_set_num(vm, "FILEGID_N", 0);
          var_set_num(vm, "OK", 0);
          if (hr.err[0]) {
            var_set_str(vm, "LAST_ERR", hr.err);
            var_set_str(vm, "ERR", hr.err);
          } else {
            var_set_str(vm, "LAST_ERR", "FILEGID: fail");
            var_set_str(vm, "ERR", "FILEGID: fail");
          }
          bump(vm); return 1;
        }
        var_set_str(vm, "LAST", hr.str);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
        vm->last_n = hr.n;
        var_set_num(vm, "LAST_N", hr.n);
        var_set_num(vm, "FILEGID", hr.n);
        var_set_num(vm, "FILEGID_N", hr.n);
        var_set_num(vm, "OWNERGID", hr.n);
        var_set_str(vm, "FILEGID_PATH", path);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      /* FILEUID */
      if (cubalc_host_fileuid(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "FILEUID", 0);
        var_set_num(vm, "FILEUID_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "FILEUID: fail");
          var_set_str(vm, "ERR", "FILEUID: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "FILEUID", hr.n);
      var_set_num(vm, "FILEUID_N", hr.n);
      var_set_num(vm, "OWNERUID", hr.n);
      var_set_str(vm, "FILEUID_PATH", path);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS CANREAD|READABLE|R_OK path — access R_OK → LAST_N 0|1 (OK=1 even if 0).
     * SYS CANWRITE|WRITABLE|W_OK path — W_OK.
     * SYS CANEXEC|EXECUTABLE|X_OK path — X_OK.
     * SYS CANACCESS|EXISTACCESS|F_OK path — F_OK (exists for caller).
     * Empty path → OK=0. Usability: pre-flight plate I/O without soft READ/WRITE fail. */
    if (kw(&L->cur,"CANREAD") || kw(&L->cur,"READABLE") || kw(&L->cur,"R_OK") ||
        kw(&L->cur,"CAN_READ") || kw(&L->cur,"ISREADABLE") || kw(&L->cur,"ACCESSR") ||
        kw(&L->cur,"CANWRITE") || kw(&L->cur,"WRITABLE") || kw(&L->cur,"W_OK") ||
        kw(&L->cur,"CAN_WRITE") || kw(&L->cur,"ISWRITABLE") || kw(&L->cur,"ACCESSW") ||
        kw(&L->cur,"CANEXEC") || kw(&L->cur,"EXECUTABLE") || kw(&L->cur,"X_OK") ||
        kw(&L->cur,"CAN_EXEC") || kw(&L->cur,"ISEXECUTABLE") || kw(&L->cur,"ACCESSX") ||
        kw(&L->cur,"CANACCESS") || kw(&L->cur,"EXISTACCESS") || kw(&L->cur,"F_OK") ||
        kw(&L->cur,"CAN_ACCESS") || kw(&L->cur,"ACCESSIBLE") || kw(&L->cur,"ACCESSF")){
      char path[512], op[24];
      cubalc_host_result hr;
      int mode = R_OK;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "CANWRITE") == 0 || strcmp(op, "WRITABLE") == 0 ||
          strcmp(op, "W_OK") == 0 || strcmp(op, "CAN_WRITE") == 0 ||
          strcmp(op, "ISWRITABLE") == 0 || strcmp(op, "ACCESSW") == 0)
        mode = W_OK;
      else if (strcmp(op, "CANEXEC") == 0 || strcmp(op, "EXECUTABLE") == 0 ||
               strcmp(op, "X_OK") == 0 || strcmp(op, "CAN_EXEC") == 0 ||
               strcmp(op, "ISEXECUTABLE") == 0 || strcmp(op, "ACCESSX") == 0)
        mode = X_OK;
      else if (strcmp(op, "CANACCESS") == 0 || strcmp(op, "EXISTACCESS") == 0 ||
               strcmp(op, "F_OK") == 0 || strcmp(op, "CAN_ACCESS") == 0 ||
               strcmp(op, "ACCESSIBLE") == 0 || strcmp(op, "ACCESSF") == 0)
        mode = F_OK;
      else
        mode = R_OK;
      memset(&hr, 0, sizeof hr);
      lex_next(L);
      path[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0)
        snprintf(path, sizeof path, "%s", vm->last_str);
      if (!path[0] || cubalc_host_can_access(path, mode, &hr) != 0) {
        var_set_str(vm, "LAST", "0");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "0");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "CANREAD_N", 0);
        var_set_num(vm, "CANWRITE_N", 0);
        var_set_num(vm, "CANEXEC_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "CAN*: empty path");
        var_set_str(vm, "ERR", "CAN*: empty path");
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      if (mode == R_OK) {
        var_set_num(vm, "CANREAD", hr.n);
        var_set_num(vm, "CANREAD_N", hr.n);
        var_set_num(vm, "READABLE", hr.n);
      } else if (mode == W_OK) {
        var_set_num(vm, "CANWRITE", hr.n);
        var_set_num(vm, "CANWRITE_N", hr.n);
        var_set_num(vm, "WRITABLE", hr.n);
      } else if (mode == X_OK) {
        var_set_num(vm, "CANEXEC", hr.n);
        var_set_num(vm, "CANEXEC_N", hr.n);
        var_set_num(vm, "EXECUTABLE", hr.n);
      } else {
        var_set_num(vm, "CANACCESS", hr.n);
        var_set_num(vm, "CANACCESS_N", hr.n);
        var_set_num(vm, "ACCESSIBLE", hr.n);
      }
      var_set_str(vm, "CAN_PATH", path);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS CANCREATE|CREATABLE|MAYCREATE path — LAST_N 1 if create/overwrite allowed.
     * Exists → W_OK; missing → parent W_OK|X_OK. Empty → OK=0.
     * Usability: pre-flight plate WRITE/TOUCH/MKDIR without soft-fail glue. */
    if (kw(&L->cur,"CANCREATE") || kw(&L->cur,"CREATABLE") || kw(&L->cur,"MAYCREATE") ||
        kw(&L->cur,"CAN_CREATE") || kw(&L->cur,"ISCREATABLE") || kw(&L->cur,"CANMK") ||
        kw(&L->cur,"CANTOUCH") || kw(&L->cur,"CANWRITEPATH") || kw(&L->cur,"PARENTWRITABLE")){
      char path[512];
      cubalc_host_result hr;
      memset(&hr, 0, sizeof hr);
      lex_next(L);
      path[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0)
        snprintf(path, sizeof path, "%s", vm->last_str);
      if (!path[0] || cubalc_host_can_create(path, &hr) != 0) {
        var_set_str(vm, "LAST", "0");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "0");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "CANCREATE_N", 0);
        var_set_num(vm, "CREATABLE", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "CANCREATE: empty path");
        var_set_str(vm, "ERR", "CANCREATE: empty path");
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "CANCREATE", hr.n);
      var_set_num(vm, "CANCREATE_N", hr.n);
      var_set_num(vm, "CREATABLE", hr.n);
      var_set_num(vm, "MAYCREATE", hr.n);
      var_set_str(vm, "CANCREATE_PATH", path);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS OWNERNAME|OWNERUSER|NAMEOWNER path — st_uid → login name in LAST.
     * SYS GROUPNAME|OWNERGROUP|GRNAME path — st_gid → group name in LAST.
     * LAST_N = uid/gid; soft miss missing path. Numeric fallback if no pw/gr entry.
     * Usability: human owner labels for plates without shell stat -c %U/%G. */
    if (kw(&L->cur,"OWNERNAME") || kw(&L->cur,"OWNERUSER") || kw(&L->cur,"NAMEOWNER") ||
        kw(&L->cur,"OWNER_NAME") || kw(&L->cur,"PWNAME") || kw(&L->cur,"UNAMEOF") ||
        kw(&L->cur,"FILEUNAME") || kw(&L->cur,"OWNERLOGIN") ||
        kw(&L->cur,"GROUPNAME") || kw(&L->cur,"OWNERGROUP") || kw(&L->cur,"GRNAME") ||
        kw(&L->cur,"GROUP_NAME") || kw(&L->cur,"GNAMEOF") || kw(&L->cur,"FILEGNAME") ||
        kw(&L->cur,"OWNERGRP") || kw(&L->cur,"NAMEGROUP")){
      char path[512], op[24];
      cubalc_host_result hr;
      int is_grp;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_grp = (strcmp(op, "GROUPNAME") == 0 || strcmp(op, "OWNERGROUP") == 0 ||
                strcmp(op, "GRNAME") == 0 || strcmp(op, "GROUP_NAME") == 0 ||
                strcmp(op, "GNAMEOF") == 0 || strcmp(op, "FILEGNAME") == 0 ||
                strcmp(op, "OWNERGRP") == 0 || strcmp(op, "NAMEGROUP") == 0);
      memset(&hr, 0, sizeof hr);
      lex_next(L);
      path[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0)
        snprintf(path, sizeof path, "%s", vm->last_str);
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", is_grp ? "GROUPNAME: empty path" : "OWNERNAME: empty path");
        var_set_str(vm, "ERR", is_grp ? "GROUPNAME: empty path" : "OWNERNAME: empty path");
        bump(vm); return 1;
      }
      if (is_grp) {
        if (cubalc_host_groupname(path, &hr) != 0) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_str(vm, "GROUPNAME", "");
          var_set_num(vm, "OK", 0);
          if (hr.err[0]) {
            var_set_str(vm, "LAST_ERR", hr.err);
            var_set_str(vm, "ERR", hr.err);
          } else {
            var_set_str(vm, "LAST_ERR", "GROUPNAME: fail");
            var_set_str(vm, "ERR", "GROUPNAME: fail");
          }
          bump(vm); return 1;
        }
        var_set_str(vm, "LAST", hr.str);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
        vm->last_n = hr.n;
        var_set_num(vm, "LAST_N", hr.n);
        var_set_str(vm, "GROUPNAME", hr.str);
        var_set_str(vm, "OWNERGROUP", hr.str);
        var_set_str(vm, "GRNAME", hr.str);
        var_set_num(vm, "FILEGID", hr.n);
        var_set_str(vm, "GROUPNAME_PATH", path);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      /* OWNERNAME */
      if (cubalc_host_ownername(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "OWNERNAME", "");
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "OWNERNAME: fail");
          var_set_str(vm, "ERR", "OWNERNAME: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "OWNERNAME", hr.str);
      var_set_str(vm, "OWNERUSER", hr.str);
      var_set_str(vm, "NAMEOWNER", hr.str);
      var_set_num(vm, "FILEUID", hr.n);
      var_set_str(vm, "OWNERNAME_PATH", path);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS READLINK|READLN|LINKTARGET path — peel symlink target → LAST.
     * LAST_N = target length; soft miss / not-link → OK=0.
     * Usability: resolve plate aliases without shell readlink. */
    if (kw(&L->cur,"READLINK") || kw(&L->cur,"READLN") || kw(&L->cur,"LINKTARGET") ||
        kw(&L->cur,"GETLINK") || kw(&L->cur,"LNKTARGET") || kw(&L->cur,"SYMLINKTARGET") ||
        kw(&L->cur,"READSYMLINK") || kw(&L->cur,"RESOLVELINK")){
      char path[512];
      cubalc_host_result hr;
      memset(&hr, 0, sizeof hr);
      lex_next(L);
      path[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0)
        snprintf(path, sizeof path, "%s", vm->last_str);
      if (!path[0] || cubalc_host_readlink(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "READLINK_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "READLINK: fail");
          var_set_str(vm, "ERR", "READLINK: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "READLINK_N", hr.n);
      var_set_str(vm, "READLINK", hr.str);
      var_set_str(vm, "LINKTARGET", hr.str);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS RENAME|MV|MOVE from to — move plate path without shell. */
    if (kw(&L->cur,"RENAME") || kw(&L->cur,"MV") || kw(&L->cur,"MOVE")){
      char from[512], to[512];
      cubalc_host_result hr;
      lex_next(L);
      if (resolve_str_arg(vm, L, from, sizeof from)!=0){
        fail(vm,"SYS RENAME \"from\" \"to\""); return -1;
      }
      if (resolve_str_arg(vm, L, to, sizeof to)!=0){
        fail(vm,"SYS RENAME \"from\" \"to\""); return -1;
      }
      if (cubalc_host_rename(from, to, &hr)!=0){
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
          var_set_str(vm, "LAST", hr.err);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.err);
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", to);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", to);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS COPY|CP|CLONE src dst — duplicate regular file plate.
     * Usability: agents snapshot plates without shell cp. Soft miss on src. */
    if (kw(&L->cur,"COPY") || kw(&L->cur,"CP") || kw(&L->cur,"CLONE") ||
        kw(&L->cur,"FILECOPY") || kw(&L->cur,"FCOPY")){
      char src[512], dst[512];
      cubalc_host_result hr;
      lex_next(L);
      if (resolve_str_arg(vm, L, src, sizeof src)!=0){
        fail(vm,"SYS COPY \"src\" \"dst\""); return -1;
      }
      if (resolve_str_arg(vm, L, dst, sizeof dst)!=0){
        fail(vm,"SYS COPY \"src\" \"dst\""); return -1;
      }
      if (cubalc_host_copy(src, dst, &hr)!=0){
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "COPY_N", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
          var_set_str(vm, "LAST", hr.err);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.err);
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", dst);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", dst);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "COPY_N", hr.n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS REALPATH|ABSPATH|ABS [path|LAST] — absolute path for portable plates.
     * Existing → realpath; missing relative → cwd/path; empty → cwd. */
    if (kw(&L->cur,"REALPATH") || kw(&L->cur,"ABSPATH") || kw(&L->cur,"ABS") ||
        kw(&L->cur,"ABSOLUTE") || kw(&L->cur,"CANON") || kw(&L->cur,"CANONICAL")){
      char path[512];
      cubalc_host_result hr;
      lex_next(L);
      path[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, path, sizeof path) != 0)
          path[0] = 0;
      }
      /* no arg → LAST if set, else cwd via empty path */
      if (!path[0] && vm->last_str[0])
        snprintf(path, sizeof path, "%s", vm->last_str);
      if (cubalc_host_abspath(path, &hr) != 0) {
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
          var_set_str(vm, "LAST", hr.err);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.err);
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      var_set_str(vm, "REALPATH", hr.str);
      var_set_str(vm, "ABSPATH", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS RELPATH|REL|MAKE_RELATIVE base path — path relative to base.
     * Under base → strip prefix; same → "."; outside → absolute path.
     * LAST_N = strlen; RELPATH_UNDER 1|0. Soft fail empty args.
     * Usability: portable plate paths after WALK/PUSHD without shell. */
    if (kw(&L->cur,"RELPATH") || kw(&L->cur,"REL") || kw(&L->cur,"MAKE_RELATIVE") ||
        kw(&L->cur,"MAKEREL") || kw(&L->cur,"STRIPBASE") || kw(&L->cur,"PATHREL") ||
        kw(&L->cur,"REL_TO") || kw(&L->cur,"RELATIVE_TO")){
      char base[512], path[512];
      cubalc_host_result hr;
      memset(&hr, 0, sizeof hr);
      lex_next(L);
      base[0] = 0; path[0] = 0;
      if (resolve_str_arg(vm, L, base, sizeof base) != 0)
        base[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0)
        path[0] = 0;
      if (!base[0] || !path[0] || cubalc_host_relpath(base, path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "RELPATH_UNDER", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "RELPATH: empty base or path");
          var_set_str(vm, "ERR", "RELPATH: empty base or path");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "RELPATH", hr.str);
      var_set_num(vm, "RELPATH_UNDER", hr.code ? 1 : 0);
      var_set_num(vm, "REL_UNDER", hr.code ? 1 : 0);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS TOUCH path — create empty file or refresh mtime (plate markers).
     * LAST_N/TOUCH_N: 1 if newly created, 0 if updated existing. Soft fail on dir. */
    if (kw(&L->cur,"TOUCH") || kw(&L->cur,"ENSURE_FILE") || kw(&L->cur,"CREATE") ||
        kw(&L->cur,"MAKEFILE")){
      char path[512];
      cubalc_host_result hr;
      lex_next(L);
      if (resolve_str_arg(vm, L, path, sizeof path)!=0){
        fail(vm,"SYS TOUCH \"path\"|LAST"); return -1;
      }
      if (cubalc_host_touch(path, &hr)!=0){
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "TOUCH_N", 0);
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
      var_set_num(vm, "TOUCH_N", hr.n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS LIST|LS|LISTDIR path — directory basenames → LAST (newline-joined).
     * LAST_N/LIST_N = count. Soft miss / non-dir. Usability: scan STATE plates. */
    if (kw(&L->cur,"LIST") || kw(&L->cur,"LS") || kw(&L->cur,"LISTDIR") ||
        kw(&L->cur,"DIRLIST") || kw(&L->cur,"SCANDIR") || kw(&L->cur,"READDIR")){
      char path[512];
      cubalc_host_result hr;
      lex_next(L);
      if (resolve_str_arg(vm, L, path, sizeof path)!=0){
        fail(vm,"SYS LIST \"path\"|LAST"); return -1;
      }
      if (cubalc_host_listdir(path, &hr)!=0){
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "LIST_N", 0);
        var_set_str(vm, "LAST", "");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      var_set_str(vm, "LIST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "LIST_N", hr.n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS GLOB|MATCHFILES|WILDCARD path pattern
     * — list basenames under path matching shell-style pattern (* ? []).
     * Default pattern "*" when omitted. Soft miss / non-dir: OK=0 empty LAST.
     * LAST = matching names newline-joined; LAST_N/GLOB_N = count.
     * Usability: find plates/proofs without shell ls|grep glue. */
    if (kw(&L->cur,"GLOB") || kw(&L->cur,"MATCHFILES") || kw(&L->cur,"WILDCARD") ||
        kw(&L->cur,"FILEGLOB") || kw(&L->cur,"LSGLOB") || kw(&L->cur,"DIRGLOB") ||
        kw(&L->cur,"MATCHDIR") || kw(&L->cur,"FNMATCH") || kw(&L->cur,"GLOBLS")){
      char path[512], pat[256], out[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      const char *p, *start;
      size_t flen, olen = 0;
      long kept = 0;
      lex_next(L);
      path[0] = 0; pat[0] = 0; out[0] = 0;
      snprintf(pat, sizeof pat, "%s", "*");
      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, "SYS GLOB \"path\" [\"pattern\"]"); return -1;
      }
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, pat, sizeof pat) != 0)
          snprintf(pat, sizeof pat, "%s", "*");
        if (!pat[0]) snprintf(pat, sizeof pat, "%s", "*");
      }
      if (cubalc_host_listdir(path, &hr) != 0) {
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "GLOB_N", 0);
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "GLOB", "");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        }
        bump(vm); return 1;
      }
#if defined(CUBALC_OS_WINDOWS)
      /* no fnmatch: fall back to full list when pattern is "*", else substring * strip */
      if (strcmp(pat, "*") == 0) {
        snprintf(out, sizeof out, "%s", hr.str);
        kept = hr.n;
      } else {
        p = hr.str;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            char name[512];
            size_t take = flen;
            int hit = 0;
            if (take >= sizeof name) take = sizeof name - 1;
            memcpy(name, start, take);
            name[take] = 0;
            /* minimal: *suffix, prefix*, or exact */
            if (pat[0] == '*' && pat[1] && !strchr(pat + 1, '*'))
              hit = (take >= strlen(pat + 1) &&
                     strcmp(name + take - strlen(pat + 1), pat + 1) == 0);
            else if (pat[0] && pat[strlen(pat) - 1] == '*' &&
                     !strchr(pat, '*') /* only trailing */)
              hit = 0; /* handled below */
            else if (pat[0] && pat[strlen(pat) - 1] == '*') {
              size_t pn = strlen(pat) - 1;
              hit = (strncmp(name, pat, pn) == 0);
            } else
              hit = (strcmp(name, pat) == 0);
            if (hit) {
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + take < sizeof out) {
                memcpy(out + olen, name, take);
                olen += take;
              }
              out[olen] = 0;
              kept++;
            }
          }
          if (*p == '\n') p++;
        }
      }
#else
      p = hr.str;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen > 0) {
          char name[512];
          size_t take = flen;
          if (take >= sizeof name) take = sizeof name - 1;
          memcpy(name, start, take);
          name[take] = 0;
          if (fnmatch(pat, name, 0) == 0) {
            if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + take < sizeof out) {
              memcpy(out + olen, name, take);
              olen += take;
            }
            out[olen] = 0;
            kept++;
          }
        }
        if (*p == '\n') p++;
      }
#endif
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "GLOB", out);
      var_set_str(vm, "MATCHFILES", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "GLOB_N", kept);
      var_set_num(vm, "MATCHFILES_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS PATHGLOB|PGLOB|FULLGLOB|GLOBPATH path_or_pattern [pattern]
     * — like GLOB but LAST holds full paths (dir/basename), ready for READ/RM.
     * One-arg shell style: "dir/*.plate" splits on last /; "*.log" uses cwd;
     * bare "dir" (no meta) → all entries under dir. Two-arg: path + pattern.
     * Soft miss / non-dir: OK=0 empty LAST. Usability: one-shot plate discovery. */
    if (kw(&L->cur,"PATHGLOB") || kw(&L->cur,"PGLOB") || kw(&L->cur,"FULLGLOB") ||
        kw(&L->cur,"GLOBPATH") || kw(&L->cur,"PATHMATCH") || kw(&L->cur,"LSPATH") ||
        kw(&L->cur,"MATCHPATHS") || kw(&L->cur,"FILEPATHS") || kw(&L->cur,"GLOBFULL")){
      char arg1[512], path[512], pat[256], out[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr, jr;
      const char *p, *start, *slash;
      size_t flen, olen = 0;
      long kept = 0;
      int has_meta = 0;
      lex_next(L);
      arg1[0] = 0; path[0] = 0; pat[0] = 0; out[0] = 0;
      snprintf(pat, sizeof pat, "%s", "*");
      if (resolve_str_arg(vm, L, arg1, sizeof arg1) != 0) {
        fail(vm, "SYS PATHGLOB \"path|pattern\" [\"pattern\"]"); return -1;
      }
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        /* two-arg: path pattern */
        snprintf(path, sizeof path, "%s", arg1);
        if (resolve_str_arg(vm, L, pat, sizeof pat) != 0)
          snprintf(pat, sizeof pat, "%s", "*");
        if (!pat[0]) snprintf(pat, sizeof pat, "%s", "*");
      } else {
        /* one-arg: shell-style. Split dir/pattern only when basename has meta
         * (* ? []) — else whole arg is the directory (avoid /tmp/dir → pat=dir). */
        slash = strrchr(arg1, '/');
#ifdef CUBALC_OS_WINDOWS
        {
          const char *bs = strrchr(arg1, '\\');
          if (bs && (!slash || bs > slash)) slash = bs;
        }
#endif
        {
          const char *base = slash ? slash + 1 : arg1;
          for (p = base; *p; p++) {
            if (*p == '*' || *p == '?' || *p == '[') { has_meta = 1; break; }
          }
          /* trailing slash → all under dir */
          if (slash && !slash[1]) has_meta = 1;
        }
        if (slash && has_meta) {
          size_t dlen = (size_t)(slash - arg1);
          if (dlen == 0) {
            snprintf(path, sizeof path, "%s", "/");
          } else {
            if (dlen >= sizeof path) dlen = sizeof path - 1;
            memcpy(path, arg1, dlen);
            path[dlen] = 0;
          }
          if (slash[1])
            snprintf(pat, sizeof pat, "%s", slash + 1);
          else
            snprintf(pat, sizeof pat, "%s", "*");
        } else if (!slash && has_meta) {
          snprintf(path, sizeof path, "%s", ".");
          snprintf(pat, sizeof pat, "%s", arg1);
        } else {
          /* bare directory path (may contain /) → all entries full paths */
          snprintf(path, sizeof path, "%s", arg1);
          snprintf(pat, sizeof pat, "%s", "*");
        }
      }
      if (cubalc_host_listdir(path, &hr) != 0) {
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "PATHGLOB_N", 0);
        var_set_num(vm, "PGLOB_N", 0);
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "PATHGLOB", "");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        }
        bump(vm); return 1;
      }
      p = hr.str;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen > 0) {
          char name[512];
          size_t take = flen;
          int hit = 0;
          if (take >= sizeof name) take = sizeof name - 1;
          memcpy(name, start, take);
          name[take] = 0;
#if defined(CUBALC_OS_WINDOWS)
          if (strcmp(pat, "*") == 0)
            hit = 1;
          else if (pat[0] == '*' && pat[1] && !strchr(pat + 1, '*'))
            hit = (take >= strlen(pat + 1) &&
                   strcmp(name + take - strlen(pat + 1), pat + 1) == 0);
          else if (pat[0] && pat[strlen(pat) - 1] == '*') {
            size_t pn = strlen(pat) - 1;
            hit = (strncmp(name, pat, pn) == 0);
          } else
            hit = (strcmp(name, pat) == 0);
#else
          hit = (fnmatch(pat, name, 0) == 0);
#endif
          if (hit) {
            if (cubalc_host_join(path, name, &jr) != 0)
              snprintf(jr.str, sizeof jr.str, "%s/%s", path, name);
            {
              size_t jlen = strlen(jr.str);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + jlen < sizeof out) {
                memcpy(out + olen, jr.str, jlen);
                olen += jlen;
              }
              out[olen] = 0;
              kept++;
            }
          }
        }
        if (*p == '\n') p++;
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "PATHGLOB", out);
      var_set_str(vm, "PGLOB", out);
      var_set_str(vm, "FULLGLOB", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "PATHGLOB_N", kept);
      var_set_num(vm, "PGLOB_N", kept);
      var_set_num(vm, "FULLGLOB_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS FILTERGLOB|MATCHBAG|GREPGLOB bag pattern
     * SYS VFILTERGLOB|GREPVGLOB|DROPGLOB bag pattern — invert (drop matches).
     * Keep/drop newline fields matching shell pattern (* ? []).
     * Matches full field OR its basename (so "*.plate" works after PATHGLOB).
     * LAST = filtered bag; LAST_N/FILTERGLOB_N = kept; FILTERGLOB_DROP = dropped.
     * Usability: re-filter LIST/PATHGLOB bags without shell or EACH+IF glue. */
    if (kw(&L->cur,"FILTERGLOB") || kw(&L->cur,"MATCHBAG") || kw(&L->cur,"GREPGLOB") ||
        kw(&L->cur,"GLOBFILTER") || kw(&L->cur,"FNMATCHBAG") || kw(&L->cur,"GLOBBAG") ||
        kw(&L->cur,"VFILTERGLOB") || kw(&L->cur,"GREPVGLOB") || kw(&L->cur,"DROPGLOB") ||
        kw(&L->cur,"VMATCHBAG") || kw(&L->cur,"VGREPGLOB") || kw(&L->cur,"DROPBAGGLOB")){
      char bag[CUBALC_HOST_STR_MAX], pat[256], out[CUBALC_HOST_STR_MAX];
      const char *p, *start, *slash, *base;
      size_t flen, olen = 0;
      long kept = 0, drop = 0;
      int invert = 0;
      if (kw(&L->cur,"VFILTERGLOB") || kw(&L->cur,"GREPVGLOB") || kw(&L->cur,"DROPGLOB") ||
          kw(&L->cur,"VMATCHBAG") || kw(&L->cur,"VGREPGLOB") || kw(&L->cur,"DROPBAGGLOB"))
        invert = 1;
      lex_next(L);
      bag[0] = 0; pat[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0) {
        /* no bag arg — use LAST, then require pattern */
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      }
      if (resolve_str_arg(vm, L, pat, sizeof pat) != 0) {
        /* one-arg form: SYS FILTERGLOB "*.plate" → bag was pattern, source LAST */
        if (bag[0] && (strchr(bag, '*') || strchr(bag, '?') || strchr(bag, '['))) {
          snprintf(pat, sizeof pat, "%s", bag);
          snprintf(bag, sizeof bag, "%s", vm->last_str);
        } else {
          fail(vm, "SYS FILTERGLOB bag pattern | SYS FILTERGLOB pattern (uses LAST)");
          return -1;
        }
      }
      if (!pat[0]) snprintf(pat, sizeof pat, "%s", "*");
      p = bag;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen > 0) {
          char field[CUBALC_HOST_STR_MAX];
          size_t take = flen;
          int hit = 0;
          if (take >= sizeof field) take = sizeof field - 1;
          memcpy(field, start, take);
          field[take] = 0;
          slash = strrchr(field, '/');
          base = slash ? slash + 1 : field;
#if defined(CUBALC_OS_WINDOWS)
          {
            const char *cands[2];
            int ci;
            cands[0] = field;
            cands[1] = base;
            for (ci = 0; ci < 2 && !hit; ci++) {
              const char *name = cands[ci];
              size_t nlen = strlen(name);
              if (ci == 1 && name == field) continue;
              if (strcmp(pat, "*") == 0) hit = 1;
              else if (pat[0] == '*' && pat[1] && !strchr(pat + 1, '*'))
                hit = (nlen >= strlen(pat + 1) &&
                       strcmp(name + nlen - strlen(pat + 1), pat + 1) == 0);
              else if (pat[0] && pat[strlen(pat) - 1] == '*') {
                size_t pn = strlen(pat) - 1;
                hit = (strncmp(name, pat, pn) == 0);
              } else
                hit = (strcmp(name, pat) == 0);
            }
          }
#else
          hit = (fnmatch(pat, field, 0) == 0) ||
                (base != field && fnmatch(pat, base, 0) == 0);
#endif
          if (invert) hit = !hit;
          if (hit) {
            if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + take < sizeof out) {
              memcpy(out + olen, field, take);
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
      var_set_str(vm, "FILTERGLOB", out);
      var_set_str(vm, "MATCHBAG", out);
      var_set_str(vm, "GREPGLOB", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "FILTERGLOB_N", kept);
      var_set_num(vm, "MATCHBAG_N", kept);
      var_set_num(vm, "GREPGLOB_N", kept);
      var_set_num(vm, "FILTERGLOB_DROP", drop);
      var_set_num(vm, "MATCHBAG_DROP", drop);
      if (invert) {
        var_set_num(vm, "VFILTERGLOB_N", kept);
        var_set_num(vm, "GREPVGLOB_N", kept);
        var_set_num(vm, "DROPGLOB_N", drop);
      }
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS HASENV|ENVHAS|HAS_ENV name — LAST_N 1 if host env set and non-empty.
     * Probe: OK=1 even when 0 (missing). Empty name → OK=0.
     * Usability: IF before REQUIRE ENV / SYS ENV without soft-fail or fatal. */
    if (kw(&L->cur,"HASENV") || kw(&L->cur,"ENVHAS") || kw(&L->cur,"HAS_ENV") ||
        kw(&L->cur,"ENV_HAS") || kw(&L->cur,"ENVDEFINED") || kw(&L->cur,"DEFINEDENV") ||
        kw(&L->cur,"ENVPRESENT") || kw(&L->cur,"ENV_PRESENT") || kw(&L->cur,"ENVOK")){
      char ename[256];
      const char *e;
      long hit;
      lex_next(L);
      ename[0] = 0;
      if (L->cur.kind == TK_STR) {
        snprintf(ename, sizeof ename, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        if (strcmp(L->cur.text, "LAST") == 0) {
          snprintf(ename, sizeof ename, "%s", vm->last_str);
          lex_next(L);
        } else {
          Var *ev = var_get(vm, L->cur.text, 0);
          if (ev && ev->is_str && ev->sval[0]) {
            snprintf(ename, sizeof ename, "%s", ev->sval);
            lex_next(L);
          } else {
            /* bare unquoted key: SYS HASENV PATH */
            snprintf(ename, sizeof ename, "%s", L->cur.text);
            lex_next(L);
          }
        }
      }
      if (!ename[0]) {
        var_set_str(vm, "LAST", "0");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "0");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "HASENV", 0);
        var_set_num(vm, "HASENV_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "HASENV: empty name");
        var_set_str(vm, "ERR", "HASENV: empty name");
        bump(vm); return 1;
      }
      e = getenv(ename);
      hit = (e && e[0]) ? 1 : 0;
      {
        char buf[8];
        snprintf(buf, sizeof buf, "%ld", hit);
        var_set_str(vm, "LAST", buf);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      }
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "HASENV", hit);
      var_set_num(vm, "HASENV_N", hit);
      var_set_num(vm, "ENVHAS", hit);
      var_set_str(vm, "HASENV_NAME", ename);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
  return 0;
}
