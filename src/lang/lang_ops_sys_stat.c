/* CubalC lang — lang_ops_core_sys.c
 * SYS/HOST megablock dispatch (mesh vote A_split_lang_ops_core).
 * Peel of ~33k-line SYS plane from lang_ops_core.
 */
#include "lang/cubalc_lang_ops_sys_stat.h"
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

int cubalc_lang_ops_sys_try_stat(VM *vm, Lex *L) {
  /* Stat/meta SYS plane — SIZE · IS* · MODE · UMASK · CHMOD · STAT · MTIME · MKDIR · BASENAME
   * A2 theme peel.
   */
    /* SYS SIZE|FSIZE path — regular-file bytes → LAST_N/SIZE; soft miss OK=0
     * SYS ISDIR path — LAST_N 1 if directory
     * SYS ISFILE path — LAST_N 1 if regular file
     * Usability: plate probes without full READ / shell stat. */
    if (kw(&L->cur,"SIZE") || kw(&L->cur,"FSIZE") || kw(&L->cur,"FILESIZE") ||
        kw(&L->cur,"BYTES") || kw(&L->cur,"ISDIR") || kw(&L->cur,"IS_DIR") ||
        kw(&L->cur,"ISFILE") || kw(&L->cur,"IS_FILE") || kw(&L->cur,"ISREG")){
      int want_size = (kw(&L->cur,"SIZE") || kw(&L->cur,"FSIZE") ||
                      kw(&L->cur,"FILESIZE") || kw(&L->cur,"BYTES"));
      int want_dir = (kw(&L->cur,"ISDIR") || kw(&L->cur,"IS_DIR"));
      int want_file = (kw(&L->cur,"ISFILE") || kw(&L->cur,"IS_FILE") ||
                      kw(&L->cur,"ISREG"));
      char path[512];
      cubalc_host_result hr;
      lex_next(L);
      if (resolve_str_arg(vm, L, path, sizeof path)!=0){
        fail(vm, want_size ? "SYS SIZE \"path\"|LAST"
                           : (want_dir ? "SYS ISDIR \"path\"|LAST"
                                       : "SYS ISFILE \"path\"|LAST"));
        return -1;
      }
      cubalc_host_path_kind(path, &hr);
      if (want_size) {
        if (hr.code == 0) {
          var_set_num(vm, "OK", 0);
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "SIZE", 0);
          if (hr.err[0]) {
            var_set_str(vm, "LAST_ERR", hr.err);
            var_set_str(vm, "ERR", hr.err);
            var_set_str(vm, "LAST", hr.err);
            snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.err);
          }
        } else if (hr.code == 1) {
          /* regular file — report bytes */
          vm->last_n = hr.n;
          var_set_num(vm, "LAST_N", hr.n);
          var_set_num(vm, "SIZE", hr.n);
          var_set_num(vm, "OK", 1);
          var_set_str(vm, "LAST", path);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
        } else if (hr.code == 2) {
          /* directory — size 0, still OK (exists as dir) */
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "SIZE", 0);
          var_set_num(vm, "OK", 1);
          var_set_str(vm, "LAST", path);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
        } else {
          vm->last_n = hr.n;
          var_set_num(vm, "LAST_N", hr.n);
          var_set_num(vm, "SIZE", hr.n);
          var_set_num(vm, "OK", 1);
          var_set_str(vm, "LAST", path);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
        }
        bump(vm); return 1;
      }
      if (want_dir) {
        long v = (hr.code == 2) ? 1 : 0;
        var_set_num(vm, "LAST_N", v);
        var_set_num(vm, "ISDIR", v);
        var_set_num(vm, "OK", 1);
        vm->last_n = v;
        bump(vm); return 1;
      }
      if (want_file) {
        long v = (hr.code == 1) ? 1 : 0;
        var_set_num(vm, "LAST_N", v);
        var_set_num(vm, "ISFILE", v);
        var_set_num(vm, "OK", 1);
        vm->last_n = v;
        bump(vm); return 1;
      }
      bump(vm); return 1;
    }
    /* SYS ISLINK|ISLNK|ISSYMLINK path — LAST_N 1 if symlink (lstat).
     * Missing / not-link → LAST_N 0, OK=1 (probe). Empty path → 0.
     * Usability: IF before READLINK without soft-fail glue. */
    if (kw(&L->cur,"ISLINK") || kw(&L->cur,"ISLNK") || kw(&L->cur,"ISSYMLINK") ||
        kw(&L->cur,"IS_LINK") || kw(&L->cur,"IS_SYMLINK") || kw(&L->cur,"LINKP") ||
        kw(&L->cur,"SYMLINKP") || kw(&L->cur,"ISSOFTLINK")){
      char path[512];
      cubalc_host_result hr;
      long v;
      memset(&hr, 0, sizeof hr);
      lex_next(L);
      path[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0)
        snprintf(path, sizeof path, "%s", vm->last_str);
      cubalc_host_islink(path, &hr);
      v = hr.n ? 1 : 0;
      var_set_num(vm, "LAST_N", v);
      var_set_num(vm, "ISLINK", v);
      var_set_num(vm, "ISLNK", v);
      vm->last_n = v;
      var_set_str(vm, "LAST", path[0] ? path : "");
      if (path[0])
        snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
      else
        vm->last_str[0] = 0;
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS KINDSTR|FILEKIND|PATHKIND path — human kind label via lstat.
     * LAST = missing|file|dir|link|fifo|sock|chr|blk|other
     * LAST_N = 0..8 (0 miss, 1 file, 2 dir, 3 link, 4 fifo, 5 sock, 6 chr, 7 blk, 8 other)
     * Missing is soft probe OK=1; empty path → OK=0.
     * Usability: PATHGLOB/WALK triage without ISDIR+ISFILE+ISLINK glue. */
    if (kw(&L->cur,"KINDSTR") || kw(&L->cur,"FILEKIND") || kw(&L->cur,"PATHKIND") ||
        kw(&L->cur,"KINDNAME") || kw(&L->cur,"TYPESTR") || kw(&L->cur,"PATHTYPE") ||
        kw(&L->cur,"FTYPE") || kw(&L->cur,"FILETYPE") || kw(&L->cur,"NODEKIND")){
      char path[512];
      cubalc_host_result hr;
      memset(&hr, 0, sizeof hr);
      lex_next(L);
      path[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0)
        snprintf(path, sizeof path, "%s", vm->last_str);
      if (!path[0] || cubalc_host_kindstr(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "KINDSTR", "");
        var_set_num(vm, "KIND_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "KINDSTR: empty path");
        var_set_str(vm, "ERR", "KINDSTR: empty path");
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "KINDSTR", hr.str);
      var_set_str(vm, "FILEKIND", hr.str);
      var_set_num(vm, "KIND_N", hr.n);
      var_set_str(vm, "KINDSTR_PATH", path);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MODE|PERM|PERMS|GETMODE path — permission bits via stat.
     * LAST = 4-digit octal "0644"; LAST_N / MODE_N = mode & 07777.
     * Soft miss / empty → OK=0. Usability: plate perms without shell stat. */
    if (kw(&L->cur,"MODE") || kw(&L->cur,"PERM") || kw(&L->cur,"PERMS") ||
        kw(&L->cur,"GETMODE") || kw(&L->cur,"FILEMODE") || kw(&L->cur,"GETPERM") ||
        kw(&L->cur,"STATMODE") || kw(&L->cur,"FMODE")){
      char path[512];
      cubalc_host_result hr;
      memset(&hr, 0, sizeof hr);
      lex_next(L);
      path[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0)
        snprintf(path, sizeof path, "%s", vm->last_str);
      if (!path[0] || cubalc_host_mode(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "MODE_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "MODE: fail");
          var_set_str(vm, "ERR", "MODE: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "MODE_N", hr.n);
      var_set_str(vm, "MODE", hr.str);
      var_set_str(vm, "PERM", hr.str);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS UMASK [mode] — get or set process file-creation mask.
     * No arg / GETUMASK: current mask → LAST "0ooo" LAST_N decimal.
     * With mode (num or "0077" string): set; LAST/LAST_N = previous mask.
     * Usability: private plate defaults without shell umask. */
    if (kw(&L->cur,"UMASK") || kw(&L->cur,"GETUMASK") || kw(&L->cur,"SETUMASK") ||
        kw(&L->cur,"FILEUMASK") || kw(&L->cur,"MASK") || kw(&L->cur,"CREATIONMASK")){
      char op[24], mstr[64];
      long mode = -1;
      int do_set = 0;
      cubalc_host_result hr;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      memset(&hr, 0, sizeof hr);
      lex_next(L);
      mstr[0] = 0;
      /* SETUMASK always wants a mode; UMASK optional; GETUMASK never sets */
      if (strcmp(op, "GETUMASK") == 0) {
        do_set = 0;
      } else if (strcmp(op, "SETUMASK") == 0) {
        do_set = 1;
      }
      if (strcmp(op, "GETUMASK") != 0) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS) {
          mode = parse_expr(vm, L);
          do_set = 1;
        } else if (L->cur.kind == TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
                   !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE") &&
                   !kw(&L->cur,"IF") && !kw(&L->cur,"FOR") && !kw(&L->cur,"WHILE") &&
                   !kw(&L->cur,"HOLD_FLASH") && !kw(&L->cur,"INCLUDE") && !kw(&L->cur,"NOTE")) {
          Var *mv = var_get(vm, L->cur.text, 0);
          if (mv && mv->is_str && mv->sval[0]) {
            char *end = NULL;
            unsigned long v = strtoul(mv->sval, &end, 8);
            if (end && end != mv->sval && *end == 0)
              mode = (long)v;
            else {
              end = NULL;
              v = strtoul(mv->sval, &end, 10);
              if (end && end != mv->sval && *end == 0)
                mode = (long)v;
            }
            lex_next(L);
            do_set = 1;
          } else if (mv && !mv->is_str) {
            mode = parse_expr(vm, L);
            do_set = 1;
          }
          /* else: bare unknown ident → treat as next form (get umask) */
        } else if (L->cur.kind == TK_STR) {
          snprintf(mstr, sizeof mstr, "%s", L->cur.text);
          lex_next(L);
          if (mstr[0]) {
            char *end = NULL;
            unsigned long v = strtoul(mstr, &end, 8);
            if (end && end != mstr && *end == 0)
              mode = (long)v;
            else {
              end = NULL;
              v = strtoul(mstr, &end, 10);
              if (end && end != mstr && *end == 0)
                mode = (long)v;
            }
            do_set = 1;
          }
        }
      }
      if (do_set) {
        if (mode < 0 || cubalc_host_umask_set(mode, &hr) != 0) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "UMASK_N", 0);
          var_set_num(vm, "OK", 0);
          if (hr.err[0]) {
            var_set_str(vm, "LAST_ERR", hr.err);
            var_set_str(vm, "ERR", hr.err);
          } else {
            var_set_str(vm, "LAST_ERR", "UMASK: bad mode");
            var_set_str(vm, "ERR", "UMASK: bad mode");
          }
          bump(vm); return 1;
        }
      } else {
        if (cubalc_host_umask_get(&hr) != 0) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", "UMASK: fail");
          var_set_str(vm, "ERR", "UMASK: fail");
          bump(vm); return 1;
        }
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "UMASK_N", hr.n);
      var_set_str(vm, "UMASK", hr.str);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS CHMOD|SETMODE|SETPERM path mode — set permission bits.
     * mode: integer (octal literal 0o644 or decimal 420) or "0644"/"644" string.
     * LAST = path; LAST_N / CHMOD_N = applied mode; soft miss OK=0.
     * Usability: agent plate lock-down without shell chmod. */
    if (kw(&L->cur,"CHMOD") || kw(&L->cur,"SETMODE") || kw(&L->cur,"SETPERM") ||
        kw(&L->cur,"SETPERMS") || kw(&L->cur,"FILECHMOD") || kw(&L->cur,"PERMSET") ||
        kw(&L->cur,"MODESET")){
      char path[512], mstr[64];
      long mode = -1;
      cubalc_host_result hr;
      memset(&hr, 0, sizeof hr);
      lex_next(L);
      path[0] = 0; mstr[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, "SYS CHMOD path mode");
        return -1;
      }
      /* mode: number expr, or octal/decimal string (literal or string var).
       * Usability: SYS CHMOD fp m with m="0600" must not use parse_expr
       * (string vars coerce to strlen → mode 4 for "0600"). */
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS) {
        mode = parse_expr(vm, L);
      } else if (L->cur.kind == TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
                 !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE")) {
        Var *mv = var_get(vm, L->cur.text, 0);
        if (mv && mv->is_str && mv->sval[0]) {
          char *end = NULL;
          unsigned long v = strtoul(mv->sval, &end, 8);
          if (end && end != mv->sval && *end == 0)
            mode = (long)v;
          else {
            end = NULL;
            v = strtoul(mv->sval, &end, 10);
            if (end && end != mv->sval && *end == 0)
              mode = (long)v;
          }
          lex_next(L);
        } else {
          mode = parse_expr(vm, L);
        }
      } else if (resolve_str_arg(vm, L, mstr, sizeof mstr) == 0 && mstr[0]) {
        /* octal string "0644" or "644" */
        char *end = NULL;
        unsigned long v = strtoul(mstr, &end, 8);
        if (end && end != mstr && *end == 0)
          mode = (long)v;
        else {
          end = NULL;
          v = strtoul(mstr, &end, 10);
          if (end && end != mstr && *end == 0)
            mode = (long)v;
        }
      } else {
        mode = vm->last_n;
      }
      if (!path[0] || mode < 0 || cubalc_host_chmod(path, mode, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "CHMOD_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "CHMOD: fail");
          var_set_str(vm, "ERR", "CHMOD: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", path);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "CHMOD_N", hr.n);
      var_set_str(vm, "CHMOD", hr.str);
      var_set_str(vm, "MODE", hr.str);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS STAT|FSTAT|FILESTAT path
     * — one-shot path metadata: exist + kind + size + mtime + isfile/isdir.
     * STAT_EXIST 0|1; STAT_KIND 0 miss|1 file|2 dir|3 other;
     * STAT_SIZE; STAT_MTIME; STAT_ISFILE; STAT_ISDIR; STAT_AGE (seconds).
     * LAST = path if exist else ""; LAST_N = STAT_EXIST.
     * Empty path soft OK=0. Miss still OK=1 (probe). Usability: replace
     * EXIST+SIZE+MTIME+ISDIR glue for agent plate checks. */
    if (kw(&L->cur,"STAT") || kw(&L->cur,"FSTAT") || kw(&L->cur,"FILESTAT") ||
        kw(&L->cur,"PATHSTAT") || kw(&L->cur,"STATPATH") || kw(&L->cur,"META") ||
        kw(&L->cur,"FILEMETA") || kw(&L->cur,"PATHMETA")){
      char path[512];
      cubalc_host_result kr, mr;
      long exist = 0, kind = 0, size = 0, mtime = 0, age = 0;
      long isfile = 0, isdir = 0;
      lex_next(L);
      path[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, "SYS STAT path");
        return -1;
      }
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "STAT_EXIST", 0);
        var_set_num(vm, "STAT_KIND", 0);
        var_set_num(vm, "STAT_SIZE", 0);
        var_set_num(vm, "STAT_MTIME", 0);
        var_set_num(vm, "STAT_AGE", 0);
        var_set_num(vm, "STAT_ISFILE", 0);
        var_set_num(vm, "STAT_ISDIR", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "STAT: empty path");
        var_set_str(vm, "ERR", "STAT: empty path");
        bump(vm); return 1;
      }
      cubalc_host_path_kind(path, &kr);
      kind = (long)kr.code; /* 0 miss 1 file 2 dir 3 other */
      if (kind > 0) {
        exist = 1;
        size = kr.n;
        isfile = (kind == 1) ? 1 : 0;
        isdir = (kind == 2) ? 1 : 0;
        if (cubalc_host_mtime(path, &mr) == 0 && mr.ok) {
          mtime = mr.n;
          age = (long)time(NULL) - mtime;
          if (age < 0) age = 0;
        }
      }
      if (exist) {
        var_set_str(vm, "LAST", path);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
      } else {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
      }
      vm->last_n = exist;
      var_set_num(vm, "LAST_N", exist);
      var_set_str(vm, "STAT", path);
      var_set_num(vm, "STAT_EXIST", exist);
      var_set_num(vm, "STAT_KIND", kind);
      var_set_num(vm, "STAT_SIZE", size);
      var_set_num(vm, "STAT_MTIME", mtime);
      var_set_num(vm, "STAT_AGE", age);
      var_set_num(vm, "STAT_ISFILE", isfile);
      var_set_num(vm, "STAT_ISDIR", isdir);
      var_set_num(vm, "SIZE", size);
      var_set_num(vm, "MTIME", mtime);
      var_set_num(vm, "ISFILE", isfile);
      var_set_num(vm, "ISDIR", isdir);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MTIME|MODTIME path — st_mtime epoch seconds → LAST_N/MTIME; soft miss OK=0.
     * SYS AGE|FILEAGE path — now - mtime seconds → LAST_N/AGE (0 if miss or future).
     * Usability: lease/plate freshness without shell stat. */
    if (kw(&L->cur,"MTIME") || kw(&L->cur,"MODTIME") || kw(&L->cur,"MODIFIED") ||
        kw(&L->cur,"FILETIME") || kw(&L->cur,"AGE") || kw(&L->cur,"FILEAGE") ||
        kw(&L->cur,"AGESEC") || kw(&L->cur,"STALE")){
      int want_age = (kw(&L->cur,"AGE") || kw(&L->cur,"FILEAGE") ||
                      kw(&L->cur,"AGESEC") || kw(&L->cur,"STALE"));
      char path[512];
      cubalc_host_result hr;
      lex_next(L);
      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, want_age ? "SYS AGE \"path\"|LAST" : "SYS MTIME \"path\"|LAST");
        return -1;
      }
      if (cubalc_host_mtime(path, &hr) != 0 || !hr.ok) {
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "MTIME", 0);
        var_set_num(vm, "AGE", 0);
        vm->last_n = 0;
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
          var_set_str(vm, "LAST", "");
          snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        }
        bump(vm); return 1;
      }
      if (want_age) {
        long now = (long)time(NULL);
        long age = now - hr.n;
        if (age < 0) age = 0;
        vm->last_n = age;
        var_set_num(vm, "LAST_N", age);
        var_set_num(vm, "AGE", age);
        var_set_num(vm, "MTIME", hr.n);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", path);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
      } else {
        char buf[40];
        snprintf(buf, sizeof buf, "%ld", hr.n);
        vm->last_n = hr.n;
        var_set_num(vm, "LAST_N", hr.n);
        var_set_num(vm, "MTIME", hr.n);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", buf);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      }
      bump(vm); return 1;
    }
    /* SYS ATIME|ACCESSTIME path — st_atime epoch → LAST_N/ATIME; soft miss OK=0.
     * SYS CTIME|CHANGETIME|STATUSTIME path — st_ctime epoch → LAST_N/CTIME.
     * Usability: read/status freshness beyond MTIME without shell stat. */
    if (kw(&L->cur,"ATIME") || kw(&L->cur,"ACCESSTIME") || kw(&L->cur,"ACCESS_TIME") ||
        kw(&L->cur,"LASTACCESS") || kw(&L->cur,"READTIME") ||
        kw(&L->cur,"CTIME") || kw(&L->cur,"CHANGETIME") || kw(&L->cur,"CHANGE_TIME") ||
        kw(&L->cur,"STATUSTIME") || kw(&L->cur,"INODETIME") || kw(&L->cur,"STATUS_TIME")){
      char path[512], op[24];
      cubalc_host_result hr;
      int want_ctime;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      want_ctime = (strcmp(op, "CTIME") == 0 || strcmp(op, "CHANGETIME") == 0 ||
                    strcmp(op, "CHANGE_TIME") == 0 || strcmp(op, "STATUSTIME") == 0 ||
                    strcmp(op, "INODETIME") == 0 || strcmp(op, "STATUS_TIME") == 0);
      memset(&hr, 0, sizeof hr);
      lex_next(L);
      path[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0)
        snprintf(path, sizeof path, "%s", vm->last_str);
      if (!path[0] ||
          (want_ctime ? cubalc_host_ctime(path, &hr) : cubalc_host_atime(path, &hr)) != 0 ||
          !hr.ok) {
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, want_ctime ? "CTIME" : "ATIME", 0);
        vm->last_n = 0;
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", want_ctime ? "CTIME: fail" : "ATIME: fail");
          var_set_str(vm, "ERR", want_ctime ? "CTIME: fail" : "ATIME: fail");
        }
        bump(vm); return 1;
      }
      {
        char buf[40];
        snprintf(buf, sizeof buf, "%ld", hr.n);
        vm->last_n = hr.n;
        var_set_num(vm, "LAST_N", hr.n);
        if (want_ctime) {
          var_set_num(vm, "CTIME", hr.n);
          var_set_num(vm, "CTIME_N", hr.n);
          var_set_str(vm, "CTIME_PATH", path);
        } else {
          var_set_num(vm, "ATIME", hr.n);
          var_set_num(vm, "ATIME_N", hr.n);
          var_set_str(vm, "ATIME_PATH", path);
        }
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", buf);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      }
      bump(vm); return 1;
    }
    /* SYS MKDIR|MAKEDIR path — mkdir -p; OK if dir already exists.
     * Usability: agents create STATE/TMP plate trees without shell. */
    if (kw(&L->cur,"MKDIR") || kw(&L->cur,"MAKEDIR") || kw(&L->cur,"MAKE_DIR") ||
        kw(&L->cur,"MD") || kw(&L->cur,"ENSURE_DIR")){
      char path[512];
      cubalc_host_result hr;
      lex_next(L);
      if (resolve_str_arg(vm, L, path, sizeof path)!=0){
        fail(vm,"SYS MKDIR \"path\"|LAST"); return -1;
      }
      if (cubalc_host_mkdir(path, &hr)!=0){
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
      var_set_str(vm, "LAST", hr.str[0] ? hr.str : path);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str[0] ? hr.str : path);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "MKDIR_N", hr.n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS ENSUREPARENT|MKPARENT path — dirname + mkdir -p so file WRITE can land.
     * LAST = parent dir (abs when possible) · ENSUREPARENT_CREATED 0|1 · soft OK.
     * Usability: GETFLAGPATH out then ENSUREPARENT without DIRNAME+MKDIR glue. */
    if (kw(&L->cur,"ENSUREPARENT") || kw(&L->cur,"MKPARENT") || kw(&L->cur,"PARENTMKDIR") ||
        kw(&L->cur,"MKDIRPARENT") || kw(&L->cur,"ENSURE_PARENT") || kw(&L->cur,"PARENTDIRMK") ||
        kw(&L->cur,"MAKEDIRPARENT") || kw(&L->cur,"ENSUREDIRPARENT")){
      char path[512], parent[512], absbuf[CUBALC_HOST_STR_MAX];
      const char *slash;
      size_t n;
      int created = 0;
      cubalc_host_result hr;
      lex_next(L);
      path[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, "SYS ENSUREPARENT \"path\"|LAST");
        return -1;
      }
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "ENSUREPARENT: empty path");
        var_set_str(vm, "ERR", "ENSUREPARENT: empty path");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "ENSUREPARENT_CREATED", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      n = strlen(path);
      while (n > 1 && (path[n - 1] == '/' || path[n - 1] == '\\')) {
        path[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(path);
      if (slash && slash != path) {
        size_t dn = (size_t)(slash - path);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, path, dn);
        parent[dn] = 0;
      } else if (slash && slash == path) {
        snprintf(parent, sizeof parent, "%c", path[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
        snprintf(absbuf, sizeof absbuf, "%s", hr.str);
      else
        snprintf(absbuf, sizeof absbuf, "%s", parent);
      created = (cubalc_host_exists(absbuf) || cubalc_host_exists(parent)) ? 0 : 1;
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_mkdir(absbuf, &hr) != 0) {
        const char *err = hr.err[0] ? hr.err : "ENSUREPARENT: mkdir failed";
        var_set_str(vm, "LAST", absbuf);
        var_set_str(vm, "DIRNAME", absbuf);
        var_set_str(vm, "PARENT", absbuf);
        var_set_str(vm, "LAST_ERR", err);
        var_set_str(vm, "ERR", err);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", absbuf);
        vm->last_n = (long)strlen(absbuf);
        var_set_num(vm, "LAST_N", vm->last_n);
        var_set_num(vm, "ENSUREPARENT_CREATED", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      if (hr.str[0])
        snprintf(absbuf, sizeof absbuf, "%s", hr.str);
      var_set_str(vm, "LAST", absbuf);
      var_set_str(vm, "DIRNAME", absbuf);
      var_set_str(vm, "PARENT", absbuf);
      var_set_str(vm, "ENSUREPARENT", absbuf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absbuf);
      vm->last_n = (long)strlen(absbuf);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
      var_set_num(vm, "MKDIR_N", hr.n);
      var_set_num(vm, "ISDIR", 1);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# ensureparent → %s created=%d\n", absbuf, created);
      bump(vm); return 1;
    }
    /* SYS BASENAME|LEAF path — final path component → LAST/BASENAME
     * SYS DIRNAME|PARENT path — parent directory → LAST/DIRNAME
     * Usability: split JOIN/TMP/WHICH paths without shell basename(1). */
    if (kw(&L->cur,"BASENAME") || kw(&L->cur,"LEAF") || kw(&L->cur,"FILENAME") ||
        kw(&L->cur,"DIRNAME") || kw(&L->cur,"PARENT") || kw(&L->cur,"DIR")){
      int want_base = (kw(&L->cur,"BASENAME") || kw(&L->cur,"LEAF") ||
                      kw(&L->cur,"FILENAME"));
      char path[512], out[512];
      const char *slash;
      size_t n;
      lex_next(L);
      if (resolve_str_arg(vm, L, path, sizeof path)!=0){
        fail(vm, want_base ? "SYS BASENAME \"path\"|LAST"
                           : "SYS DIRNAME \"path\"|LAST");
        return -1;
      }
      /* strip trailing separators (keep single root) */
      n = strlen(path);
      while (n > 1 && (path[n - 1] == '/' || path[n - 1] == '\\')) {
        path[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(path);
      if (want_base) {
        if (slash && slash[1])
          snprintf(out, sizeof out, "%s", slash + 1);
        else if (slash && (slash[0] == '/' || slash[0] == '\\') && !slash[1])
          snprintf(out, sizeof out, "%s", path); /* "/" → "/" */
        else
          snprintf(out, sizeof out, "%s", path[0] ? path : ".");
        var_set_str(vm, "BASENAME", out);
      } else {
        if (slash && slash != path) {
          size_t dn = (size_t)(slash - path);
          if (dn >= sizeof out) dn = sizeof out - 1;
          memcpy(out, path, dn);
          out[dn] = 0;
        } else if (slash && slash == path) {
          /* absolute at root */
          snprintf(out, sizeof out, "%c", path[0] == '\\' ? '\\' : '/');
        } else {
          snprintf(out, sizeof out, ".");
        }
        var_set_str(vm, "DIRNAME", out);
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)strlen(out);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS EXTNAME|EXT|SUFFIX path — final extension with leading '.' → LAST/EXT
     * SYS STEM path — basename without extension → LAST/STEM
     * Usability: peel JOIN/WHICH paths for plate stems and type tags. */
    if (kw(&L->cur,"EXTNAME") || kw(&L->cur,"EXT") || kw(&L->cur,"SUFFIX") ||
        kw(&L->cur,"EXTENSION") || kw(&L->cur,"STEM") || kw(&L->cur,"ROOTNAME")){
      int want_ext = (kw(&L->cur,"EXTNAME") || kw(&L->cur,"EXT") ||
                     kw(&L->cur,"SUFFIX") || kw(&L->cur,"EXTENSION"));
      char path[512], leaf[512], out[512];
      const char *slash, *dot, *base;
      size_t n;
      lex_next(L);
      if (resolve_str_arg(vm, L, path, sizeof path)!=0){
        fail(vm, want_ext ? "SYS EXTNAME \"path\"|LAST"
                          : "SYS STEM \"path\"|LAST");
        return -1;
      }
      /* strip trailing separators */
      n = strlen(path);
      while (n > 1 && (path[n - 1] == '/' || path[n - 1] == '\\')) {
        path[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(path);
      if (slash && slash[1])
        snprintf(leaf, sizeof leaf, "%s", slash + 1);
      else if (slash && (slash[0] == '/' || slash[0] == '\\') && !slash[1])
        snprintf(leaf, sizeof leaf, "%s", path);
      else
        snprintf(leaf, sizeof leaf, "%s", path[0] ? path : ".");
      base = leaf;
      /* last '.' that is not the first char (dotfiles have no extension) */
      dot = strrchr(leaf, '.');
      if (dot && dot != leaf) {
        if (want_ext)
          snprintf(out, sizeof out, "%s", dot);
        else {
          size_t sn = (size_t)(dot - leaf);
          if (sn >= sizeof out) sn = sizeof out - 1;
          memcpy(out, leaf, sn);
          out[sn] = 0;
        }
      } else {
        if (want_ext)
          out[0] = 0;
        else
          snprintf(out, sizeof out, "%s", base);
      }
      if (want_ext) {
        var_set_str(vm, "EXTNAME", out);
        var_set_str(vm, "EXT", out);
      } else {
        var_set_str(vm, "STEM", out);
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)strlen(out);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
  return 0;
}
