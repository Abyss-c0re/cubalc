/* CubalC lang — lang_ops_sys_files_cwd.c
 * SYS cwd/temp plane: CWD · PUSHD/POPD · CHDIR · STATE · ROOT · MKTEMP · MKTEMPDIR · TMP
 * Peel from sys_files (B_sys_files_cwd). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_sys_files_cwd.h"
#include "lang/cubalc_lang_ops_sys_plate.h"
#include "lang/cubalc_lang_ops_sys_path_fs.h"
#include "lang/cubalc_lang_ops_sys_env.h"
#include "lang/cubalc_lang_ops_sys_string.h"
#include "lang/cubalc_lang_ops_sys_wait.h"
#include "lang/cubalc_lang_ops_sys_stat.h"
#include "lang/cubalc_lang_ops_sys_bulk.h"
#include "lang/cubalc_lang_ops_sys_bulk_mut.h"
#include "lang/cubalc_lang_ops_sys_net.h"
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
#if defined(CUBALC_OS_WINDOWS)
#  include <direct.h>
#endif

int cubalc_lang_ops_sys_try_files_cwd(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. SYS already consumed. */
    /* SYS CWD|PWD — process working directory → LAST
     * SYS STATE — CUBALC_STATE plate dir (default state) → LAST
     * SYS ROOT — CUBALC_ROOT or cwd (INCLUDE root) → LAST
     * Usability: portable scripts locate layout without shell glue. */
    if (kw(&L->cur,"CWD") || kw(&L->cur,"PWD") || kw(&L->cur,"WORKDIR")){
      char cwd[512];
      lex_next(L);
      if (!getcwd(cwd, sizeof cwd)) {
        snprintf(cwd, sizeof cwd, ".");
        var_set_num(vm, "OK", 0);
      } else {
        var_set_num(vm, "OK", 1);
      }
      snprintf(vm->last_str, sizeof vm->last_str, "%s", cwd);
      vm->last_n = (long)strlen(cwd);
      var_set_str(vm, "LAST", cwd);
      var_set_str(vm, "CWD", cwd);
      var_set_num(vm, "LAST_N", vm->last_n);
      bump(vm); return 1;
    }
    /* SYS PUSHD|PUSHDIR path — save cwd then chdir → LAST = new cwd.
     * LAST_N = stack depth after push. Soft miss OK=0 sticky LAST_ERR.
     * SYS POPD|POPDIR — restore previous cwd → LAST = restored; LAST_N = depth.
     * SYS DIRSTACK|DIRS — newline bag of saved dirs (bottom→top); LAST_N = depth.
     * Usability: temp relative plate I/O without losing base (no shell pushd). */
    if (kw(&L->cur,"PUSHD") || kw(&L->cur,"PUSHDIR") || kw(&L->cur,"ENTERDIR") ||
        kw(&L->cur,"PUSHCWD") || kw(&L->cur,"CDPUSH") ||
        kw(&L->cur,"POPD") || kw(&L->cur,"POPDIR") || kw(&L->cur,"LEAVEDIR") ||
        kw(&L->cur,"POPCWD") || kw(&L->cur,"CDBACK") || kw(&L->cur,"BACKDIR") ||
        kw(&L->cur,"DIRSTACK") || kw(&L->cur,"DIRS") || kw(&L->cur,"DIR_STACK") ||
        kw(&L->cur,"CWDSTACK") || kw(&L->cur,"STACKDIRS")){
      char op[24], path[512];
      cubalc_host_result hr;
      int is_pop, is_list;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_pop = (strcmp(op, "POPD") == 0 || strcmp(op, "POPDIR") == 0 ||
                strcmp(op, "LEAVEDIR") == 0 || strcmp(op, "POPCWD") == 0 ||
                strcmp(op, "CDBACK") == 0 || strcmp(op, "BACKDIR") == 0);
      is_list = (strcmp(op, "DIRSTACK") == 0 || strcmp(op, "DIRS") == 0 ||
                 strcmp(op, "DIR_STACK") == 0 || strcmp(op, "CWDSTACK") == 0 ||
                 strcmp(op, "STACKDIRS") == 0);
      memset(&hr, 0, sizeof hr);
      lex_next(L);
      if (is_list) {
        if (cubalc_host_dirstack(&hr) != 0) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "DIRSTACK_N", 0);
          var_set_num(vm, "OK", 0);
          bump(vm); return 1;
        }
        var_set_str(vm, "LAST", hr.str);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
        vm->last_n = hr.n;
        var_set_num(vm, "LAST_N", hr.n);
        var_set_num(vm, "DIRSTACK_N", hr.n);
        var_set_str(vm, "DIRSTACK", hr.str);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      if (is_pop) {
        if (cubalc_host_popd(&hr) != 0) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "POPD_N", 0);
          var_set_num(vm, "DIRSTACK_N", 0);
          var_set_num(vm, "OK", 0);
          if (hr.err[0]) {
            var_set_str(vm, "LAST_ERR", hr.err);
            var_set_str(vm, "ERR", hr.err);
          } else {
            var_set_str(vm, "LAST_ERR", "POPD: empty stack");
            var_set_str(vm, "ERR", "POPD: empty stack");
          }
          bump(vm); return 1;
        }
        var_set_str(vm, "LAST", hr.str);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
        vm->last_n = hr.n;
        var_set_num(vm, "LAST_N", hr.n);
        var_set_num(vm, "POPD_N", hr.n);
        var_set_num(vm, "DIRSTACK_N", hr.n);
        var_set_str(vm, "CWD", hr.str);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      /* PUSHD path */
      path[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0)
        snprintf(path, sizeof path, "%s", vm->last_str);
      if (!path[0] || cubalc_host_pushd(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "PUSHD_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "PUSHD: fail");
          var_set_str(vm, "ERR", "PUSHD: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "PUSHD_N", hr.n);
      var_set_num(vm, "DIRSTACK_N", hr.n);
      var_set_str(vm, "CWD", hr.str);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS CHDIR|CD path — change process working directory → LAST = new cwd.
     * LAST_N = 1 success, 0 soft miss (cwd unchanged). Updates CWD var.
     * Usability: relative READ/WRITE/LIST under STATE/TMP without shell cd. */
    if (kw(&L->cur,"CHDIR") || kw(&L->cur,"CD") || kw(&L->cur,"CHDIRTO") ||
        kw(&L->cur,"SETCWD") || kw(&L->cur,"CHANGE_DIR") || kw(&L->cur,"CHANGEDIR")){
      char path[512], cwd[512];
      int soft = 0, ok = 0;
      lex_next(L);
      if (kw(&L->cur,"OR") || kw(&L->cur,"SOFT") || kw(&L->cur,"OPTIONAL") ||
          kw(&L->cur,"TRY") || kw(&L->cur,"MAYBE")){
        soft = 1;
        lex_next(L);
      }
      path[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0)
        snprintf(path, sizeof path, "%s", vm->last_str);
      if (!path[0]) {
        ok = 0;
      } else {
#if defined(CUBALC_OS_WINDOWS)
        ok = (_chdir(path) == 0) ? 1 : 0;
#else
        ok = (chdir(path) == 0) ? 1 : 0;
#endif
      }
      if (!getcwd(cwd, sizeof cwd))
        snprintf(cwd, sizeof cwd, ok ? path : ".");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", cwd);
      var_set_str(vm, "LAST", cwd);
      var_set_str(vm, "CWD", cwd);
      vm->last_n = ok ? 1 : 0;
      var_set_num(vm, "LAST_N", ok ? 1 : 0);
      var_set_num(vm, "CHDIR_N", ok ? 1 : 0);
      var_set_num(vm, "OK", ok ? 1 : 0);
      if (!ok && !soft) {
        /* sticky soft err for agents; not fatal (plate continues) */
        char em[180];
        snprintf(em, sizeof em, "CHDIR miss: %s", path[0] ? path : "(empty)");
        var_set_str(vm, "LAST_ERR", em);
        var_set_str(vm, "ERR", em);
      }
      bump(vm); return 1;
    }
    if (kw(&L->cur,"STATE") || kw(&L->cur,"STATEDIR") || kw(&L->cur,"STATE_DIR")){
      char sdir[512];
      const char *e;
      lex_next(L);
      e = getenv("CUBALC_STATE");
      if (e && e[0]) snprintf(sdir, sizeof sdir, "%s", e);
      else snprintf(sdir, sizeof sdir, "state");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", sdir);
      vm->last_n = (long)strlen(sdir);
      var_set_str(vm, "LAST", sdir);
      var_set_str(vm, "STATE_DIR", sdir);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"ROOT") || kw(&L->cur,"CUBALC_ROOT") || kw(&L->cur,"INSTALL_ROOT")){
      char root[512];
      const char *e;
      lex_next(L);
      e = getenv("CUBALC_ROOT");
      if (e && e[0]) {
        snprintf(root, sizeof root, "%s", e);
      } else if (!getcwd(root, sizeof root)) {
        snprintf(root, sizeof root, ".");
      }
      snprintf(vm->last_str, sizeof vm->last_str, "%s", root);
      vm->last_n = (long)strlen(root);
      var_set_str(vm, "LAST", root);
      var_set_str(vm, "ROOT", root);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MKTEMP|TEMPFILE|MAKETEMP [prefix]
     * — create unique empty file under TMP (mkstemp template prefix.XXXXXX).
     * Default prefix "cubalc". Prefix sanitized (alnum/_/- only, max 32).
     * LAST = path; LAST_N/MKTEMP_N = 1; soft fail OK=0.
     * Usability: collision-free agent plate paths without shell mktemp. */
    if (kw(&L->cur,"MKTEMP") || kw(&L->cur,"TEMPFILE") || kw(&L->cur,"MAKETEMP") ||
        kw(&L->cur,"MKTEMPFILE") || kw(&L->cur,"NEWTEMP") || kw(&L->cur,"TMPFILE") ||
        kw(&L->cur,"TEMPPATH") || kw(&L->cur,"UNIQUETEMP")){
      char tdir[512], prefix[40], safe[36], tmpl[560], path[560];
      const char *e;
      size_t i, j, pl;
      int fd = -1;
      lex_next(L);
      tdir[0] = 0; prefix[0] = 0; safe[0] = 0; tmpl[0] = 0; path[0] = 0;
      /* optional prefix string */
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, prefix, sizeof prefix) != 0) {
          /* no arg — default */
          prefix[0] = 0;
        }
      }
      /* resolve temp dir (same policy as SYS TMP) */
      e = getenv("TMPDIR");
      if (!e || !e[0]) e = getenv("TMP");
      if (!e || !e[0]) e = getenv("TEMP");
      if (!e || !e[0]) e = getenv("CUBALC_TMP");
      if (e && e[0]) snprintf(tdir, sizeof tdir, "%s", e);
#if defined(CUBALC_OS_WINDOWS)
      if (!tdir[0]) {
        e = getenv("LOCALAPPDATA");
        if (e && e[0]) snprintf(tdir, sizeof tdir, "%s\\Temp", e);
        else snprintf(tdir, sizeof tdir, ".");
      }
#else
      if (!tdir[0]) snprintf(tdir, sizeof tdir, "/tmp");
#endif
      {
        size_t n = strlen(tdir);
        while (n > 1 && (tdir[n - 1] == '/' || tdir[n - 1] == '\\')) {
          tdir[n - 1] = 0;
          n--;
        }
      }
      /* sanitize prefix: strip path seps, keep alnum _ - */
      if (!prefix[0]) snprintf(prefix, sizeof prefix, "cubalc");
      for (i = 0, j = 0; prefix[i] && j + 1 < sizeof safe; i++) {
        char c = prefix[i];
        if (c == '/' || c == '\\' || c == '.') continue;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_' || c == '-')
          safe[j++] = c;
      }
      safe[j] = 0;
      if (!safe[0]) snprintf(safe, sizeof safe, "cubalc");
      /* cap length so template fits */
      pl = strlen(safe);
      if (pl > 32) safe[32] = 0;
      snprintf(tmpl, sizeof tmpl, "%s/%s.XXXXXX", tdir, safe);
#if defined(CUBALC_OS_WINDOWS)
      /* Windows fallback: unique path via pid+ms (no mkstemp) */
      {
        struct timespec ts;
        long ms;
        FILE *wf;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        ms = (long)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
        snprintf(path, sizeof path, "%s/%s.%ld.%ld.tmp", tdir, safe,
                 (long)getpid(), ms);
        wf = fopen(path, "wx");
        if (!wf) {
          snprintf(path, sizeof path, "%s/%s.%ld.%ld.%d.tmp", tdir, safe,
                   (long)getpid(), ms, rand() & 0xffff);
          wf = fopen(path, "w");
        }
        if (!wf) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "MKTEMP_N", 0);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", "MKTEMP: create fail");
          var_set_str(vm, "ERR", "MKTEMP: create fail");
          bump(vm); return 1;
        }
        fclose(wf);
      }
#else
      {
        char tcopy[560];
        snprintf(tcopy, sizeof tcopy, "%s", tmpl);
        fd = mkstemp(tcopy);
        if (fd < 0) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "MKTEMP_N", 0);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", "MKTEMP: mkstemp fail");
          var_set_str(vm, "ERR", "MKTEMP: mkstemp fail");
          bump(vm); return 1;
        }
        close(fd);
        snprintf(path, sizeof path, "%s", tcopy);
      }
#endif
      var_set_str(vm, "LAST", path);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_str(vm, "MKTEMP", path);
      var_set_str(vm, "TEMPFILE", path);
      var_set_str(vm, "MAKETEMP", path);
      var_set_num(vm, "MKTEMP_N", 1);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MKTEMPDIR|MKDTEMP|MAKETEMPDIR [prefix]
     * — create unique empty directory under TMP (mkdtemp prefix.XXXXXX).
     * Default prefix "cubalc". Prefix sanitized (alnum/_/- only, max 32).
     * LAST = path; LAST_N/MKTEMPDIR_N = 1; soft fail OK=0.
     * Usability: collision-free agent work dirs without shell mktemp -d. */
    if (kw(&L->cur,"MKTEMPDIR") || kw(&L->cur,"MKDTEMP") || kw(&L->cur,"MAKETEMPDIR") ||
        kw(&L->cur,"NEWTEMPDIR") || kw(&L->cur,"TEMPMKDIR") || kw(&L->cur,"UNIQUEDIR") ||
        kw(&L->cur,"MKDIRTEMP") || kw(&L->cur,"TEMPWORKDIR") || kw(&L->cur,"WORKDIRTMP")){
      char tdir[512], prefix[40], safe[36], tmpl[560], path[560];
      const char *e;
      size_t i, j, pl;
      lex_next(L);
      tdir[0] = 0; prefix[0] = 0; safe[0] = 0; tmpl[0] = 0; path[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, prefix, sizeof prefix) != 0)
          prefix[0] = 0;
      }
      e = getenv("TMPDIR");
      if (!e || !e[0]) e = getenv("TMP");
      if (!e || !e[0]) e = getenv("TEMP");
      if (!e || !e[0]) e = getenv("CUBALC_TMP");
      if (e && e[0]) snprintf(tdir, sizeof tdir, "%s", e);
#if defined(CUBALC_OS_WINDOWS)
      if (!tdir[0]) {
        e = getenv("LOCALAPPDATA");
        if (e && e[0]) snprintf(tdir, sizeof tdir, "%s\\Temp", e);
        else snprintf(tdir, sizeof tdir, ".");
      }
#else
      if (!tdir[0]) snprintf(tdir, sizeof tdir, "/tmp");
#endif
      {
        size_t n = strlen(tdir);
        while (n > 1 && (tdir[n - 1] == '/' || tdir[n - 1] == '\\')) {
          tdir[n - 1] = 0;
          n--;
        }
      }
      if (!prefix[0]) snprintf(prefix, sizeof prefix, "cubalc");
      for (i = 0, j = 0; prefix[i] && j + 1 < sizeof safe; i++) {
        char c = prefix[i];
        if (c == '/' || c == '\\' || c == '.') continue;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_' || c == '-')
          safe[j++] = c;
      }
      safe[j] = 0;
      if (!safe[0]) snprintf(safe, sizeof safe, "cubalc");
      pl = strlen(safe);
      if (pl > 32) safe[32] = 0;
      snprintf(tmpl, sizeof tmpl, "%s/%s.XXXXXX", tdir, safe);
#if defined(CUBALC_OS_WINDOWS)
      {
        struct timespec ts;
        long ms;
        cubalc_host_result hr;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        ms = (long)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
        snprintf(path, sizeof path, "%s/%s.%ld.%ld.d", tdir, safe,
                 (long)getpid(), ms);
        if (cubalc_host_mkdir(path, &hr) != 0) {
          snprintf(path, sizeof path, "%s/%s.%ld.%ld.%d.d", tdir, safe,
                   (long)getpid(), ms, rand() & 0xffff);
          if (cubalc_host_mkdir(path, &hr) != 0) {
            var_set_str(vm, "LAST", "");
            vm->last_str[0] = 0;
            vm->last_n = 0;
            var_set_num(vm, "LAST_N", 0);
            var_set_num(vm, "MKTEMPDIR_N", 0);
            var_set_num(vm, "OK", 0);
            var_set_str(vm, "LAST_ERR", "MKTEMPDIR: mkdir fail");
            var_set_str(vm, "ERR", "MKTEMPDIR: mkdir fail");
            bump(vm); return 1;
          }
        }
      }
#else
      {
        char tcopy[560];
        char *created;
        snprintf(tcopy, sizeof tcopy, "%s", tmpl);
        created = mkdtemp(tcopy);
        if (!created) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "MKTEMPDIR_N", 0);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", "MKTEMPDIR: mkdtemp fail");
          var_set_str(vm, "ERR", "MKTEMPDIR: mkdtemp fail");
          bump(vm); return 1;
        }
        snprintf(path, sizeof path, "%s", created);
      }
#endif
      var_set_str(vm, "LAST", path);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_str(vm, "MKTEMPDIR", path);
      var_set_str(vm, "MKDTEMP", path);
      var_set_str(vm, "MAKETEMPDIR", path);
      var_set_num(vm, "MKTEMPDIR_N", 1);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS TMP|TEMP|TMPDIR — portable temp directory for agent plate writes.
     * Prefer TMPDIR / TMP / TEMP env, else /tmp (POSIX) or "." (fallback).
     * Usability: SYS JOIN TMP "plate.json" without shell $TMPDIR glue. */
    if (kw(&L->cur,"TMP") || kw(&L->cur,"TEMP") || kw(&L->cur,"TMPDIR") ||
        kw(&L->cur,"TEMPDIR") || kw(&L->cur,"TEMP_DIR") || kw(&L->cur,"TMP_DIR")){
      char tdir[512];
      const char *e;
      lex_next(L);
      tdir[0] = 0;
      e = getenv("TMPDIR");
      if (!e || !e[0]) e = getenv("TMP");
      if (!e || !e[0]) e = getenv("TEMP");
      if (!e || !e[0]) e = getenv("CUBALC_TMP");
      if (e && e[0]) snprintf(tdir, sizeof tdir, "%s", e);
#if defined(CUBALC_OS_WINDOWS)
      if (!tdir[0]) {
        e = getenv("LOCALAPPDATA");
        if (e && e[0]) snprintf(tdir, sizeof tdir, "%s\\Temp", e);
        else snprintf(tdir, sizeof tdir, ".");
      }
#else
      if (!tdir[0]) snprintf(tdir, sizeof tdir, "/tmp");
#endif
      /* strip trailing slash (keep root /) */
      {
        size_t n = strlen(tdir);
        while (n > 1 && (tdir[n - 1] == '/' || tdir[n - 1] == '\\')) {
          tdir[n - 1] = 0;
          n--;
        }
      }
      snprintf(vm->last_str, sizeof vm->last_str, "%s", tdir);
      vm->last_n = (long)strlen(tdir);
      var_set_str(vm, "LAST", tdir);
      var_set_str(vm, "TMP", tdir);
      var_set_str(vm, "TMPDIR", tdir);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
  return 0;
}
