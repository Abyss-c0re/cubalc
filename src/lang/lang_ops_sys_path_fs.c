/* CubalC lang — lang_ops_core_sys.c
 * SYS/HOST megablock dispatch (mesh vote A_split_lang_ops_core).
 * Peel of ~33k-line SYS plane from lang_ops_core.
 */
#include "lang/cubalc_lang_ops_sys_path_fs.h"
#include "lang/cubalc_lang_ops_sys_path_writeout.h"
#include "lang/cubalc_lang_ops_sys_path_sync.h"
#include "lang/cubalc_lang_ops_sys_path_link.h"
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

  /* TREE RMTREE..*INTREE → lang_ops_sys_path_tree.c (B_sys_path_peel) */
  {
    int tr = cubalc_lang_ops_sys_try_path_tree(vm, L);
    if (tr != 0) return tr;
  }

  /* FSYNC/RM → lang_ops_sys_path_sync.c */
  {
    int ps = cubalc_lang_ops_sys_try_path_sync(vm, L);
    if (ps != 0) return ps;
  }

  /* LINK/STAT SYMLINK..READLINK → lang_ops_sys_path_link.c (B_sys_path_peel) */
  {
    int pl = cubalc_lang_ops_sys_try_path_link(vm, L);
    if (pl != 0) return pl;
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
