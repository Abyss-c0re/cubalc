/* CubalC lang — lang_ops_sys_path_writeout.c
 * SYS WRITEOUT family: PUTOUT · ATOMIC · COPYOUT · LINKOUT · CLAIMOUT…
 * Peel from path_fs (B_sys_path_peel). Cube is SoT. Zero bloat.
 */
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
#  include <fcntl.h>
#  include <sys/stat.h>
#endif
int cubalc_lang_ops_sys_try_path_writeout(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. SYS already consumed. */
  /* SYS WRITEOUT|PUTOUT|SAVEOUT path data — mkdir parent + write (soft).
   * Twin of top-level WRITEOUT. LAST = abs file path · LAST_N = bytes. */
  if (kw(&L->cur,"WRITEOUT") || kw(&L->cur,"PUTOUT") || kw(&L->cur,"SAVEOUT") ||
      kw(&L->cur,"ENSUREWRITE") || kw(&L->cur,"WRITEPARENT") || kw(&L->cur,"PLATEWRITE") ||
      kw(&L->cur,"SAVEPATH") || kw(&L->cur,"PUTFILE") || kw(&L->cur,"DUMPOUT")){
    char path[CUBALC_HOST_STR_MAX], data[CUBALC_HOST_STR_MAX];
    char absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0;
    cubalc_host_result hr;
    lex_next(L);
    path[0] = 0;
    data[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
      fail(vm, "SYS WRITEOUT path data");
      return -1;
    }
    if (resolve_str_arg(vm, L, data, sizeof data) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(data, sizeof data, "%ld", nv);
      } else
        data[0] = 0;
    }
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "WRITEOUT: empty path");
      var_set_str(vm, "ERR", "WRITEOUT: empty path");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEOUT_N", 0);
      var_set_num(vm, "WRITEOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(path, &hr) == 0 && hr.str[0])
      snprintf(absfile, sizeof absfile, "%s", hr.str);
    else
      snprintf(absfile, sizeof absfile, "%s", path);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absfile);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "WRITEOUT: mkdir parent failed";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "WRITEOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEOUT_N", 0);
      var_set_num(vm, "WRITEOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_write(absfile, data, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "WRITEOUT: write failed";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "WRITEOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEOUT_N", 0);
      var_set_num(vm, "WRITEOUT_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    var_set_str(vm, "LAST", absfile);
    var_set_str(vm, "WRITEOUT", absfile);
    var_set_str(vm, "PATH", absfile);
    var_set_str(vm, "WRITEOUT_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "WRITEOUT_N", hr.n);
    var_set_num(vm, "WRITEOUT_CREATED", (long)created);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# sys writeout → %s bytes=%ld created=%d\n",
              absfile, hr.n, created);
    bump(vm); return 1;
  }
  /* SYS APPENDOUT|LOGOUT path data — mkdir parent + append line (soft).
   * Twin of top-level APPENDOUT. LAST = abs file · LAST_N = data+newline bytes. */
  if (kw(&L->cur,"APPENDOUT") || kw(&L->cur,"LOGOUT") || kw(&L->cur,"APPENDLOG") ||
      kw(&L->cur,"ENSUREAPPEND") || kw(&L->cur,"APPENDPATH") || kw(&L->cur,"LOGPATH") ||
      kw(&L->cur,"HISTORYOUT") || kw(&L->cur,"AUDITOUT") || kw(&L->cur,"APPENDPARENT")){
    char path[CUBALC_HOST_STR_MAX], data[CUBALC_HOST_STR_MAX];
    char absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0;
    long nbytes = 0;
    cubalc_host_result hr;
    FILE *af;
    lex_next(L);
    path[0] = 0;
    data[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
      fail(vm, "SYS APPENDOUT path data");
      return -1;
    }
    if (resolve_str_arg(vm, L, data, sizeof data) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(data, sizeof data, "%ld", nv);
      } else
        data[0] = 0;
    }
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "APPENDOUT: empty path");
      var_set_str(vm, "ERR", "APPENDOUT: empty path");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "APPENDOUT_N", 0);
      var_set_num(vm, "APPENDOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(path, &hr) == 0 && hr.str[0])
      snprintf(absfile, sizeof absfile, "%s", hr.str);
    else
      snprintf(absfile, sizeof absfile, "%s", path);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absfile);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "APPENDOUT: mkdir parent failed";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "APPENDOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "APPENDOUT_N", 0);
      var_set_num(vm, "APPENDOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    af = fopen(absfile, "a");
    if (!af) {
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "APPENDOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", "APPENDOUT: open fail");
      var_set_str(vm, "ERR", "APPENDOUT: open fail");
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "APPENDOUT_N", 0);
      var_set_num(vm, "APPENDOUT_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    fputs(data, af);
    fputc('\n', af);
    fclose(af);
    nbytes = (long)strlen(data) + 1;
    var_set_str(vm, "LAST", absfile);
    var_set_str(vm, "APPENDOUT", absfile);
    var_set_str(vm, "PATH", absfile);
    var_set_str(vm, "APPENDOUT_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
    vm->last_n = nbytes;
    var_set_num(vm, "LAST_N", nbytes);
    var_set_num(vm, "APPENDOUT_N", nbytes);
    var_set_num(vm, "APPENDOUT_CREATED", (long)created);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# sys appendout → %s bytes=%ld created=%d\n",
              absfile, nbytes, created);
    bump(vm); return 1;
  }
  /* SYS TEE|WRITETEE|SAVEKEEP path [data]
   * — write data to path but leave content in LAST (WRITE leaves path-only).
   * Zero data arg → prior LAST. LAST = data; LAST_N = bytes; TEE_PATH = path.
   * Soft empty path / write fail → OK=0 sticky LAST_ERR.
   * Usability: agent write+chain without READ back (log/plate then GREP/CAT). */
  if (kw(&L->cur,"TEE") || kw(&L->cur,"WRITETEE") || kw(&L->cur,"SAVEKEEP") ||
      kw(&L->cur,"WRITEKEEP") || kw(&L->cur,"KEEWRITE") || kw(&L->cur,"TEEW") ||
      kw(&L->cur,"TEEWRITE") || kw(&L->cur,"PUTKEEP") || kw(&L->cur,"DUMPKEEP") ||
      kw(&L->cur,"WRITEANDKEEP")){
    char path[512], data[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    lex_next(L);
    path[0] = 0; data[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0)
      path[0] = 0;
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "TEE_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "TEE: empty path");
      var_set_str(vm, "ERR", "TEE: empty path");
      bump(vm); return 1;
    }
    if (resolve_str_arg(vm, L, data, sizeof data) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long n = parse_expr(vm, L);
        snprintf(data, sizeof data, "%ld", n);
      } else {
        /* default: prior LAST content */
        snprintf(data, sizeof data, "%s", vm->last_str);
      }
    }
    if (cubalc_host_write(path, data, &hr) != 0) {
      char err[160];
      var_set_str(vm, "LAST", data);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", data);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "TEE_N", 0);
      var_set_num(vm, "OK", 0);
      snprintf(err, sizeof err, "TEE: write fail '%s'", path);
      if (hr.err[0])
        snprintf(err, sizeof err, "%s", hr.err);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      bump(vm); return 1;
    }
    /* keep data in LAST (not path) — the whole point of TEE */
    var_set_str(vm, "LAST", data);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", data);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "TEE_N", hr.n);
    var_set_str(vm, "TEE", data);
    var_set_str(vm, "TEE_PATH", path);
    var_set_str(vm, "WRITETEE", path);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS WRITEATOMIC|ATOMICWRITE|SAFEWRITE path data
   * — write to sibling temp then rename into place (crash-safer plate update).
   * Temp: path.cubalc-tmp.<pid>. Soft empty path / write|rename fail → OK=0.
   * LAST = path; LAST_N / WRITEATOMIC_N = bytes.
   * Usability: multi-agent plate status without torn WRITE mid-read. */
  if (kw(&L->cur,"WRITEATOMIC") || kw(&L->cur,"ATOMICWRITE") ||
      kw(&L->cur,"SAFEWRITE") || kw(&L->cur,"WRITE_ATOMIC") ||
      kw(&L->cur,"ATOMWRITE") || kw(&L->cur,"WRITESAFE") ||
      kw(&L->cur,"PLATEWRITE") || kw(&L->cur,"COMMITWRITE") ||
      kw(&L->cur,"WRITEA") || kw(&L->cur,"AWRITE")){
    char path[512], tmp[560], data[CUBALC_HOST_STR_MAX], a[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr, rr;
    long pid;
    lex_next(L);
    path[0] = 0; tmp[0] = 0; data[0] = 0; a[0] = 0;
    if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
      fail(vm, "SYS WRITEATOMIC path data");
      return -1;
    }
    snprintf(path, sizeof path, "%s", a);
    if (resolve_str_arg(vm, L, data, sizeof data) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long n = parse_expr(vm, L);
        snprintf(data, sizeof data, "%ld", n);
      } else {
        /* empty data allowed */
        data[0] = 0;
      }
    }
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEATOMIC_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "WRITEATOMIC: empty path");
      var_set_str(vm, "ERR", "WRITEATOMIC: empty path");
      bump(vm); return 1;
    }
#if defined(CUBALC_OS_WINDOWS)
    pid = 0;
#else
    pid = (long)getpid();
#endif
    snprintf(tmp, sizeof tmp, "%s.cubalc-tmp.%ld", path, pid);
    if (cubalc_host_write(tmp, data, &hr) != 0) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEATOMIC_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "WRITEATOMIC: write temp fail");
      var_set_str(vm, "ERR", hr.err[0] ? hr.err : "WRITEATOMIC: write temp fail");
      bump(vm); return 1;
    }
    if (cubalc_host_rename(tmp, path, &rr) != 0) {
      /* best-effort cleanup of temp */
      cubalc_host_result trash;
      cubalc_host_rm(tmp, &trash);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEATOMIC_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", rr.err[0] ? rr.err : "WRITEATOMIC: rename fail");
      var_set_str(vm, "ERR", rr.err[0] ? rr.err : "WRITEATOMIC: rename fail");
      bump(vm); return 1;
    }
    var_set_str(vm, "LAST", path);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "WRITEATOMIC", path);
    var_set_str(vm, "ATOMICWRITE", path);
    var_set_str(vm, "SAFEWRITE", path);
    var_set_num(vm, "WRITEATOMIC_N", hr.n);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS WRITEOUTATOMIC|SAFEWRITEOUT path data — mkdir parent + temp+rename write.
   * Twin of top-level WRITEOUTATOMIC. LAST = abs file · LAST_N = bytes. */
  if (kw(&L->cur,"WRITEOUTATOMIC") || kw(&L->cur,"SAFEWRITEOUT") ||
      kw(&L->cur,"ATOMICWRITEOUT") || kw(&L->cur,"WRITEOUTSAFE") ||
      kw(&L->cur,"COMMITOUT") || kw(&L->cur,"ATOMICOUT") ||
      kw(&L->cur,"ENSUREATOMIC") || kw(&L->cur,"SAFEOUTWRITE") ||
      kw(&L->cur,"WRITEAOUT")){
    char path[CUBALC_HOST_STR_MAX], data[CUBALC_HOST_STR_MAX];
    char absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
    char tmp[CUBALC_HOST_STR_MAX + 64];
    const char *slash;
    size_t n;
    int created = 0;
    long pid;
    cubalc_host_result hr, rr, trash;
    lex_next(L);
    path[0] = 0;
    data[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
      fail(vm, "SYS WRITEOUTATOMIC path data");
      return -1;
    }
    if (resolve_str_arg(vm, L, data, sizeof data) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(data, sizeof data, "%ld", nv);
      } else
        data[0] = 0;
    }
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "WRITEOUTATOMIC: empty path");
      var_set_str(vm, "ERR", "WRITEOUTATOMIC: empty path");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEOUTATOMIC_N", 0);
      var_set_num(vm, "WRITEOUTATOMIC_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(path, &hr) == 0 && hr.str[0])
      snprintf(absfile, sizeof absfile, "%s", hr.str);
    else
      snprintf(absfile, sizeof absfile, "%s", path);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absfile);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "WRITEOUTATOMIC: mkdir parent failed";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "WRITEOUTATOMIC_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEOUTATOMIC_N", 0);
      var_set_num(vm, "WRITEOUTATOMIC_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
#if defined(CUBALC_OS_WINDOWS)
    pid = 0;
#else
    pid = (long)getpid();
#endif
    snprintf(tmp, sizeof tmp, "%s.cubalc-tmp.%ld", absfile, pid);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_write(tmp, data, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "WRITEOUTATOMIC: write temp fail";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "WRITEOUTATOMIC_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEOUTATOMIC_N", 0);
      var_set_num(vm, "WRITEOUTATOMIC_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    memset(&rr, 0, sizeof rr);
    if (cubalc_host_rename(tmp, absfile, &rr) != 0) {
      memset(&trash, 0, sizeof trash);
      cubalc_host_rm(tmp, &trash);
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "WRITEOUTATOMIC_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", rr.err[0] ? rr.err : "WRITEOUTATOMIC: rename fail");
      var_set_str(vm, "ERR", rr.err[0] ? rr.err : "WRITEOUTATOMIC: rename fail");
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEOUTATOMIC_N", 0);
      var_set_num(vm, "WRITEOUTATOMIC_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    var_set_str(vm, "LAST", absfile);
    var_set_str(vm, "WRITEOUTATOMIC", absfile);
    var_set_str(vm, "SAFEWRITEOUT", absfile);
    var_set_str(vm, "PATH", absfile);
    var_set_str(vm, "WRITEOUTATOMIC_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "WRITEOUTATOMIC_N", hr.n);
    var_set_num(vm, "WRITEATOMIC_N", hr.n);
    var_set_num(vm, "WRITEOUTATOMIC_CREATED", (long)created);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# sys writeoutatomic → %s bytes=%ld created=%d\n",
              absfile, hr.n, created);
    bump(vm); return 1;
  }
  /* SYS COPYOUT|CPOUT src dest — mkdir parent of dest + COPY (soft).
   * Twin of top-level COPYOUT. LAST = abs dest · LAST_N = bytes. */
  if (kw(&L->cur,"COPYOUT") || kw(&L->cur,"CPOUT") || kw(&L->cur,"ENSURECOPY") ||
      kw(&L->cur,"COPYPARENT") || kw(&L->cur,"PLATECOPY") || kw(&L->cur,"SNAPOUT") ||
      kw(&L->cur,"FILEOUTCOPY") || kw(&L->cur,"COPYNEST") || kw(&L->cur,"STAGECOPY")){
    char src[CUBALC_HOST_STR_MAX], dst[CUBALC_HOST_STR_MAX];
    char abssrc[CUBALC_HOST_STR_MAX], absdst[CUBALC_HOST_STR_MAX];
    char parent[512], absparent[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0;
    cubalc_host_result hr;
    lex_next(L);
    src[0] = 0; dst[0] = 0;
    abssrc[0] = 0; absdst[0] = 0;
    parent[0] = 0; absparent[0] = 0;
    if (resolve_str_arg(vm, L, src, sizeof src) != 0) {
      fail(vm, "SYS COPYOUT src dest");
      return -1;
    }
    if (resolve_str_arg(vm, L, dst, sizeof dst) != 0) {
      fail(vm, "SYS COPYOUT src dest");
      return -1;
    }
    if (!src[0] || !dst[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "COPYOUT: empty path");
      var_set_str(vm, "ERR", "COPYOUT: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "COPYOUT_N", 0);
      var_set_num(vm, "COPYOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(src, &hr) == 0 && hr.str[0])
      snprintf(abssrc, sizeof abssrc, "%s", hr.str);
    else
      snprintf(abssrc, sizeof abssrc, "%s", src);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(dst, &hr) == 0 && hr.str[0])
      snprintf(absdst, sizeof absdst, "%s", hr.str);
    else
      snprintf(absdst, sizeof absdst, "%s", dst);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absdst);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "COPYOUT: mkdir parent failed";
      var_set_str(vm, "LAST", absdst);
      var_set_str(vm, "COPYOUT_SRC", abssrc);
      var_set_str(vm, "COPYOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absdst);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "COPYOUT_N", 0);
      var_set_num(vm, "COPYOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_copy(abssrc, absdst, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "COPYOUT: copy failed";
      var_set_str(vm, "LAST", absdst);
      var_set_str(vm, "COPYOUT_SRC", abssrc);
      var_set_str(vm, "COPYOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absdst);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "COPYOUT_N", 0);
      var_set_num(vm, "COPYOUT_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    var_set_str(vm, "LAST", absdst);
    var_set_str(vm, "COPYOUT", absdst);
    var_set_str(vm, "CPOUT", absdst);
    var_set_str(vm, "PATH", absdst);
    var_set_str(vm, "COPYOUT_SRC", abssrc);
    var_set_str(vm, "COPYOUT_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absdst);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "COPYOUT_N", hr.n);
    var_set_num(vm, "COPY_N", hr.n);
    var_set_num(vm, "COPYOUT_CREATED", (long)created);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# sys copyout → %s ← %s bytes=%ld created=%d\n",
              absdst, abssrc, hr.n, created);
    bump(vm); return 1;
  }
  /* SYS MOVEOUT|MVOUT src dest — mkdir parent of dest + RENAME (soft).
   * Twin of top-level MOVEOUT. LAST = abs dest · LAST_N = 1. */
  if (kw(&L->cur,"MOVEOUT") || kw(&L->cur,"MVOUT") || kw(&L->cur,"ENSUREMOVE") ||
      kw(&L->cur,"MOVEPARENT") || kw(&L->cur,"RENAMEOUT") || kw(&L->cur,"STAGEOUT") ||
      kw(&L->cur,"RELOCATEOUT") || kw(&L->cur,"MOVENEST") || kw(&L->cur,"PLATEMOVE")){
    char src[CUBALC_HOST_STR_MAX], dst[CUBALC_HOST_STR_MAX];
    char abssrc[CUBALC_HOST_STR_MAX], absdst[CUBALC_HOST_STR_MAX];
    char parent[512], absparent[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0;
    cubalc_host_result hr;
    lex_next(L);
    src[0] = 0; dst[0] = 0;
    abssrc[0] = 0; absdst[0] = 0;
    parent[0] = 0; absparent[0] = 0;
    if (resolve_str_arg(vm, L, src, sizeof src) != 0) {
      fail(vm, "SYS MOVEOUT src dest");
      return -1;
    }
    if (resolve_str_arg(vm, L, dst, sizeof dst) != 0) {
      fail(vm, "SYS MOVEOUT src dest");
      return -1;
    }
    if (!src[0] || !dst[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "MOVEOUT: empty path");
      var_set_str(vm, "ERR", "MOVEOUT: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "MOVEOUT_N", 0);
      var_set_num(vm, "MOVEOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(src, &hr) == 0 && hr.str[0])
      snprintf(abssrc, sizeof abssrc, "%s", hr.str);
    else
      snprintf(abssrc, sizeof abssrc, "%s", src);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(dst, &hr) == 0 && hr.str[0])
      snprintf(absdst, sizeof absdst, "%s", hr.str);
    else
      snprintf(absdst, sizeof absdst, "%s", dst);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absdst);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "MOVEOUT: mkdir parent failed";
      var_set_str(vm, "LAST", absdst);
      var_set_str(vm, "MOVEOUT_SRC", abssrc);
      var_set_str(vm, "MOVEOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absdst);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "MOVEOUT_N", 0);
      var_set_num(vm, "MOVEOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_rename(abssrc, absdst, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "MOVEOUT: rename failed";
      var_set_str(vm, "LAST", absdst);
      var_set_str(vm, "MOVEOUT_SRC", abssrc);
      var_set_str(vm, "MOVEOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absdst);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "MOVEOUT_N", 0);
      var_set_num(vm, "MOVEOUT_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    var_set_str(vm, "LAST", absdst);
    var_set_str(vm, "MOVEOUT", absdst);
    var_set_str(vm, "MVOUT", absdst);
    var_set_str(vm, "PATH", absdst);
    var_set_str(vm, "MOVEOUT_SRC", abssrc);
    var_set_str(vm, "MOVEOUT_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absdst);
    vm->last_n = 1;
    var_set_num(vm, "LAST_N", 1);
    var_set_num(vm, "MOVEOUT_N", 1);
    var_set_num(vm, "MOVEOUT_CREATED", (long)created);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# sys moveout → %s ← %s created=%d\n",
              absdst, abssrc, created);
    bump(vm); return 1;
  }
  /* SYS TOUCHOUT|MKFILEOUT path — mkdir parent + TOUCH (soft).
   * Twin of top-level TOUCHOUT. LAST = abs path · LAST_N = 1 if new file. */
  if (kw(&L->cur,"TOUCHOUT") || kw(&L->cur,"MKFILEOUT") || kw(&L->cur,"ENSURETOUCH") ||
      kw(&L->cur,"TOUCHPARENT") || kw(&L->cur,"STAMPOUT") || kw(&L->cur,"LEASEOUT") ||
      kw(&L->cur,"MARKEROUT") || kw(&L->cur,"TOUCHNEST") || kw(&L->cur,"PLATETOUCH")){
    char path[CUBALC_HOST_STR_MAX], absfile[CUBALC_HOST_STR_MAX];
    char parent[512], absparent[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0;
    cubalc_host_result hr;
    lex_next(L);
    path[0] = 0; absfile[0] = 0;
    parent[0] = 0; absparent[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
      fail(vm, "SYS TOUCHOUT path");
      return -1;
    }
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "TOUCHOUT: empty path");
      var_set_str(vm, "ERR", "TOUCHOUT: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "TOUCHOUT_N", 0);
      var_set_num(vm, "TOUCHOUT_CREATED", 0);
      var_set_num(vm, "TOUCHOUT_NEW", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(path, &hr) == 0 && hr.str[0])
      snprintf(absfile, sizeof absfile, "%s", hr.str);
    else
      snprintf(absfile, sizeof absfile, "%s", path);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absfile);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "TOUCHOUT: mkdir parent failed";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "TOUCHOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "TOUCHOUT_N", 0);
      var_set_num(vm, "TOUCHOUT_CREATED", 0);
      var_set_num(vm, "TOUCHOUT_NEW", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_touch(absfile, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "TOUCHOUT: touch failed";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "TOUCHOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "TOUCHOUT_N", 0);
      var_set_num(vm, "TOUCHOUT_CREATED", (long)created);
      var_set_num(vm, "TOUCHOUT_NEW", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    var_set_str(vm, "LAST", absfile);
    var_set_str(vm, "TOUCHOUT", absfile);
    var_set_str(vm, "PATH", absfile);
    var_set_str(vm, "TOUCHOUT_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "TOUCHOUT_N", hr.n);
    var_set_num(vm, "TOUCH_N", hr.n);
    var_set_num(vm, "TOUCHOUT_NEW", hr.n);
    var_set_num(vm, "TOUCHOUT_CREATED", (long)created);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# sys touchout → %s new=%ld parent_created=%d\n",
              absfile, hr.n, created);
    bump(vm); return 1;
  }
  /* SYS WRITEOUTIF|PUTIFCHANGED path data — mkdir parent; write only if content differs.
   * Twin of top-level WRITEOUTIF. Uses temp+rename when writing. LAST = abs path. */
  if (kw(&L->cur,"WRITEOUTIF") || kw(&L->cur,"PUTIFCHANGED") || kw(&L->cur,"WRITEIFCHANGED") ||
      kw(&L->cur,"SAVEIF") || kw(&L->cur,"PUTIF") || kw(&L->cur,"STABLEWRITE") ||
      kw(&L->cur,"WRITEUNLESSSAME") || kw(&L->cur,"IFWRITEOUT") || kw(&L->cur,"WRITEOUTDIFF")){
    char path[CUBALC_HOST_STR_MAX], data[CUBALC_HOST_STR_MAX];
    char absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
    char tmp[CUBALC_HOST_STR_MAX + 64];
    const char *slash;
    size_t n;
    int created = 0, changed = 1;
    long pid;
    cubalc_host_result hr, rr, trash;
    lex_next(L);
    path[0] = 0; data[0] = 0;
    absfile[0] = 0; parent[0] = 0; absparent[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
      fail(vm, "SYS WRITEOUTIF path data");
      return -1;
    }
    if (resolve_str_arg(vm, L, data, sizeof data) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(data, sizeof data, "%ld", nv);
      } else {
        data[0] = 0;
      }
    }
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "WRITEOUTIF: empty path");
      var_set_str(vm, "ERR", "WRITEOUTIF: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEOUTIF_N", 0);
      var_set_num(vm, "WRITEOUTIF_CHANGED", 0);
      var_set_num(vm, "WRITEOUTIF_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(path, &hr) == 0 && hr.str[0])
      snprintf(absfile, sizeof absfile, "%s", hr.str);
    else
      snprintf(absfile, sizeof absfile, "%s", path);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absfile);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    /* same content → skip write (mtime-stable for watchers) */
    if (cubalc_host_exists(absfile)) {
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_read(absfile, &hr) == 0 && strcmp(hr.str, data) == 0) {
        changed = 0;
        var_set_str(vm, "LAST", absfile);
        var_set_str(vm, "WRITEOUTIF", absfile);
        var_set_str(vm, "PATH", absfile);
        var_set_str(vm, "WRITEOUTIF_PARENT", absparent);
        var_set_str(vm, "DIRNAME", absparent);
        var_set_str(vm, "PARENT", absparent);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "WRITEOUTIF_N", 0);
        var_set_num(vm, "WRITEOUTIF_CHANGED", 0);
        var_set_num(vm, "WRITEOUTIF_CREATED", 0);
        var_set_num(vm, "PATH_EXIST", 1);
        var_set_num(vm, "OK", 1);
        if (vm->trace)
          fprintf(vm->trace, "# sys writeoutif → %s unchanged\n", absfile);
        bump(vm); return 1;
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "WRITEOUTIF: mkdir parent failed";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "WRITEOUTIF_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEOUTIF_N", 0);
      var_set_num(vm, "WRITEOUTIF_CHANGED", 0);
      var_set_num(vm, "WRITEOUTIF_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
#if defined(CUBALC_OS_WINDOWS)
    pid = 0;
#else
    pid = (long)getpid();
#endif
    snprintf(tmp, sizeof tmp, "%s.cubalc-tmp.%ld", absfile, pid);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_write(tmp, data, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "WRITEOUTIF: write temp fail";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "WRITEOUTIF_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEOUTIF_N", 0);
      var_set_num(vm, "WRITEOUTIF_CHANGED", 0);
      var_set_num(vm, "WRITEOUTIF_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    memset(&rr, 0, sizeof rr);
    if (cubalc_host_rename(tmp, absfile, &rr) != 0) {
      memset(&trash, 0, sizeof trash);
      cubalc_host_rm(tmp, &trash);
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "WRITEOUTIF_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", rr.err[0] ? rr.err : "WRITEOUTIF: rename fail");
      var_set_str(vm, "ERR", rr.err[0] ? rr.err : "WRITEOUTIF: rename fail");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEOUTIF_N", 0);
      var_set_num(vm, "WRITEOUTIF_CHANGED", 0);
      var_set_num(vm, "WRITEOUTIF_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    var_set_str(vm, "LAST", absfile);
    var_set_str(vm, "WRITEOUTIF", absfile);
    var_set_str(vm, "PATH", absfile);
    var_set_str(vm, "WRITEOUTIF_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "WRITEOUTIF_N", hr.n);
    var_set_num(vm, "WRITEOUTIF_CHANGED", (long)changed);
    var_set_num(vm, "WRITEOUTIF_CREATED", (long)created);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# sys writeoutif → %s bytes=%ld changed=%d created=%d\n",
              absfile, hr.n, changed, created);
    bump(vm); return 1;
  }
  /* SYS LOGSTAMP|STAMPLOG path msg — mkdir parent + append "ISO msg" line (soft).
   * Twin of top-level LOGSTAMP. LAST = abs file · LOGSTAMP_LINE · LAST_N = bytes. */
  if (kw(&L->cur,"LOGSTAMP") || kw(&L->cur,"STAMPLOG") || kw(&L->cur,"APPENDOUTISO") ||
      kw(&L->cur,"LOGISO") || kw(&L->cur,"ISOLOG") || kw(&L->cur,"AUDITSTAMP") ||
      kw(&L->cur,"TSLOG") || kw(&L->cur,"LOGTS") || kw(&L->cur,"STAMPAPPEND")){
    char path[CUBALC_HOST_STR_MAX], msg[CUBALC_HOST_STR_MAX];
    char absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
    char iso[40], line[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0;
    long nbytes = 0;
    time_t now;
    struct tm tm_utc;
    cubalc_host_result hr;
    FILE *af;
    lex_next(L);
    path[0] = 0; msg[0] = 0;
    absfile[0] = 0; parent[0] = 0; absparent[0] = 0;
    iso[0] = 0; line[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
      fail(vm, "SYS LOGSTAMP path msg");
      return -1;
    }
    if (resolve_str_arg(vm, L, msg, sizeof msg) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(msg, sizeof msg, "%ld", nv);
      } else {
        msg[0] = 0;
      }
    }
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "LOGSTAMP: empty path");
      var_set_str(vm, "ERR", "LOGSTAMP: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "LOGSTAMP_N", 0);
      var_set_num(vm, "LOGSTAMP_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    now = time(NULL);
#if defined(CUBALC_OS_WINDOWS)
    {
      struct tm *tp = gmtime(&now);
      if (tp) tm_utc = *tp;
      else memset(&tm_utc, 0, sizeof tm_utc);
    }
#else
    if (!gmtime_r(&now, &tm_utc))
      memset(&tm_utc, 0, sizeof tm_utc);
#endif
    snprintf(iso, sizeof iso, "%04d-%02d-%02dT%02d:%02d:%02dZ",
             tm_utc.tm_year + 1900, tm_utc.tm_mon + 1, tm_utc.tm_mday,
             tm_utc.tm_hour, tm_utc.tm_min, tm_utc.tm_sec);
    if (msg[0])
      snprintf(line, sizeof line, "%s %s", iso, msg);
    else
      snprintf(line, sizeof line, "%s", iso);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(path, &hr) == 0 && hr.str[0])
      snprintf(absfile, sizeof absfile, "%s", hr.str);
    else
      snprintf(absfile, sizeof absfile, "%s", path);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absfile);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "LOGSTAMP: mkdir parent failed";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "LOGSTAMP_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "LOGSTAMP_N", 0);
      var_set_num(vm, "LOGSTAMP_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    af = fopen(absfile, "a");
    if (!af) {
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "LOGSTAMP_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", "LOGSTAMP: open fail");
      var_set_str(vm, "ERR", "LOGSTAMP: open fail");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "LOGSTAMP_N", 0);
      var_set_num(vm, "LOGSTAMP_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    fputs(line, af);
    fputc('\n', af);
    fclose(af);
    nbytes = (long)strlen(line) + 1;
    var_set_str(vm, "LAST", absfile);
    var_set_str(vm, "LOGSTAMP", absfile);
    var_set_str(vm, "PATH", absfile);
    var_set_str(vm, "LOGSTAMP_PARENT", absparent);
    var_set_str(vm, "LOGSTAMP_LINE", line);
    var_set_str(vm, "LOGSTAMP_ISO", iso);
    var_set_str(vm, "ISO", iso);
    var_set_str(vm, "DATE", iso);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
    vm->last_n = nbytes;
    var_set_num(vm, "LAST_N", nbytes);
    var_set_num(vm, "LOGSTAMP_N", nbytes);
    var_set_num(vm, "APPENDOUT_N", nbytes);
    var_set_num(vm, "LOGSTAMP_CREATED", (long)created);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# sys logstamp → %s line=%s created=%d\n",
              absfile, line, created);
    bump(vm); return 1;
  }
  /* SYS LINKOUT|LNOUT target linkpath — mkdir parent of link + SYMLINK (soft).
   * Twin of top-level LINKOUT. LAST = abs link · LINKOUT_TARGET · LAST_N = 1. */
  if (kw(&L->cur,"LINKOUT") || kw(&L->cur,"LNOUT") || kw(&L->cur,"ENSURELINK") ||
      kw(&L->cur,"LINKPARENT") || kw(&L->cur,"SYMLINKOUT") || kw(&L->cur,"ALIASOUT") ||
      kw(&L->cur,"LINKNEST") || kw(&L->cur,"PLATELINK") || kw(&L->cur,"SOFTLINKOUT")){
    char target[CUBALC_HOST_STR_MAX], linkpath[CUBALC_HOST_STR_MAX];
    char abslink[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0;
    cubalc_host_result hr;
    lex_next(L);
    target[0] = 0; linkpath[0] = 0;
    abslink[0] = 0; parent[0] = 0; absparent[0] = 0;
    if (resolve_str_arg(vm, L, target, sizeof target) != 0) {
      fail(vm, "SYS LINKOUT target linkpath");
      return -1;
    }
    if (resolve_str_arg(vm, L, linkpath, sizeof linkpath) != 0) {
      fail(vm, "SYS LINKOUT target linkpath");
      return -1;
    }
    if (!target[0] || !linkpath[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "LINKOUT: empty path");
      var_set_str(vm, "ERR", "LINKOUT: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "LINKOUT_N", 0);
      var_set_num(vm, "LINKOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(linkpath, &hr) == 0 && hr.str[0])
      snprintf(abslink, sizeof abslink, "%s", hr.str);
    else
      snprintf(abslink, sizeof abslink, "%s", linkpath);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", abslink);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "LINKOUT: mkdir parent failed";
      var_set_str(vm, "LAST", abslink);
      var_set_str(vm, "LINKOUT_TARGET", target);
      var_set_str(vm, "LINKOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", abslink);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "LINKOUT_N", 0);
      var_set_num(vm, "LINKOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_symlink(target, abslink, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "LINKOUT: symlink failed";
      var_set_str(vm, "LAST", abslink);
      var_set_str(vm, "LINKOUT_TARGET", target);
      var_set_str(vm, "LINKOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", abslink);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "LINKOUT_N", 0);
      var_set_num(vm, "LINKOUT_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    var_set_str(vm, "LAST", abslink);
    var_set_str(vm, "LINKOUT", abslink);
    var_set_str(vm, "LNOUT", abslink);
    var_set_str(vm, "PATH", abslink);
    var_set_str(vm, "SYMLINK", abslink);
    var_set_str(vm, "LINKOUT_TARGET", target);
    var_set_str(vm, "SYMLINK_TARGET", target);
    var_set_str(vm, "LINKOUT_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", abslink);
    vm->last_n = hr.n > 0 ? hr.n : 1;
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "LINKOUT_N", vm->last_n);
    var_set_num(vm, "SYMLINK_N", vm->last_n);
    var_set_num(vm, "LINKOUT_CREATED", (long)created);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# sys linkout → %s → %s created=%d\n",
              abslink, target, created);
    bump(vm); return 1;
  }
  /* SYS HARDLINKOUT|HLOUT existing newpath — mkdir parent of new + HARDLINK (soft).
   * Twin of top-level HARDLINKOUT. LAST = abs new · HARDLINKOUT_SRC · LAST_N = 1. */
  if (kw(&L->cur,"HARDLINKOUT") || kw(&L->cur,"HLOUT") || kw(&L->cur,"ENSUREHARDLINK") ||
      kw(&L->cur,"HARDLINKPARENT") || kw(&L->cur,"LINKHARDOUT") || kw(&L->cur,"SHAREOUT") ||
      kw(&L->cur,"HLINKOUT") || kw(&L->cur,"INODEOUT") || kw(&L->cur,"PLATEHLINK")){
    char existing[CUBALC_HOST_STR_MAX], newpath[CUBALC_HOST_STR_MAX];
    char abssrc[CUBALC_HOST_STR_MAX], absnew[CUBALC_HOST_STR_MAX];
    char parent[512], absparent[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0;
    cubalc_host_result hr;
    lex_next(L);
    existing[0] = 0; newpath[0] = 0;
    abssrc[0] = 0; absnew[0] = 0;
    parent[0] = 0; absparent[0] = 0;
    if (resolve_str_arg(vm, L, existing, sizeof existing) != 0) {
      fail(vm, "SYS HARDLINKOUT existing newpath");
      return -1;
    }
    if (resolve_str_arg(vm, L, newpath, sizeof newpath) != 0) {
      fail(vm, "SYS HARDLINKOUT existing newpath");
      return -1;
    }
    if (!existing[0] || !newpath[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "HARDLINKOUT: empty path");
      var_set_str(vm, "ERR", "HARDLINKOUT: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HARDLINKOUT_N", 0);
      var_set_num(vm, "HARDLINKOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(existing, &hr) == 0 && hr.str[0])
      snprintf(abssrc, sizeof abssrc, "%s", hr.str);
    else
      snprintf(abssrc, sizeof abssrc, "%s", existing);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(newpath, &hr) == 0 && hr.str[0])
      snprintf(absnew, sizeof absnew, "%s", hr.str);
    else
      snprintf(absnew, sizeof absnew, "%s", newpath);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absnew);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "HARDLINKOUT: mkdir parent failed";
      var_set_str(vm, "LAST", absnew);
      var_set_str(vm, "HARDLINKOUT_SRC", abssrc);
      var_set_str(vm, "HARDLINKOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absnew);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HARDLINKOUT_N", 0);
      var_set_num(vm, "HARDLINKOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_hardlink(abssrc, absnew, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "HARDLINKOUT: hardlink failed";
      var_set_str(vm, "LAST", absnew);
      var_set_str(vm, "HARDLINKOUT_SRC", abssrc);
      var_set_str(vm, "HARDLINKOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absnew);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HARDLINKOUT_N", 0);
      var_set_num(vm, "HARDLINKOUT_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    var_set_str(vm, "LAST", absnew);
    var_set_str(vm, "HARDLINKOUT", absnew);
    var_set_str(vm, "HLOUT", absnew);
    var_set_str(vm, "PATH", absnew);
    var_set_str(vm, "HARDLINK", absnew);
    var_set_str(vm, "HARDLINKOUT_SRC", abssrc);
    var_set_str(vm, "HARDLINK_SRC", abssrc);
    var_set_str(vm, "HARDLINKOUT_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absnew);
    vm->last_n = hr.n > 0 ? hr.n : 1;
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "HARDLINKOUT_N", vm->last_n);
    var_set_num(vm, "HARDLINK_N", vm->last_n);
    var_set_num(vm, "HLINK_N", vm->last_n);
    var_set_num(vm, "HARDLINKOUT_CREATED", (long)created);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# sys hardlinkout → %s ← %s created=%d\n",
              absnew, abssrc, created);
    bump(vm); return 1;
  }
  /* SYS RELINKOUT|RELINK target linkpath — mkdir parent; RM if exists; SYMLINK.
   * Twin of top-level RELINKOUT. LAST = abs link · REPLACED 0|1 · LAST_N = 1. */
  if (kw(&L->cur,"RELINKOUT") || kw(&L->cur,"RELINK") || kw(&L->cur,"FORCELINKOUT") ||
      kw(&L->cur,"REPLACELINK") || kw(&L->cur,"UPDATELINK") || kw(&L->cur,"RELINKPARENT") ||
      kw(&L->cur,"LINKFORCE") || kw(&L->cur,"OVERWRITELINK") || kw(&L->cur,"RELINKNEST")){
    char target[CUBALC_HOST_STR_MAX], linkpath[CUBALC_HOST_STR_MAX];
    char abslink[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0, replaced = 0;
    cubalc_host_result hr, trash;
    lex_next(L);
    target[0] = 0; linkpath[0] = 0;
    abslink[0] = 0; parent[0] = 0; absparent[0] = 0;
    if (resolve_str_arg(vm, L, target, sizeof target) != 0) {
      fail(vm, "SYS RELINKOUT target linkpath");
      return -1;
    }
    if (resolve_str_arg(vm, L, linkpath, sizeof linkpath) != 0) {
      fail(vm, "SYS RELINKOUT target linkpath");
      return -1;
    }
    if (!target[0] || !linkpath[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "RELINKOUT: empty path");
      var_set_str(vm, "ERR", "RELINKOUT: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "RELINKOUT_N", 0);
      var_set_num(vm, "RELINKOUT_CREATED", 0);
      var_set_num(vm, "RELINKOUT_REPLACED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    /* Never realpath(linkpath): existing symlink would resolve to TARGET and RM it. */
    if (linkpath[0] == '/' || linkpath[0] == '\\') {
      snprintf(abslink, sizeof abslink, "%s", linkpath);
    } else {
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_abspath(".", &hr) == 0 && hr.str[0])
        snprintf(abslink, sizeof abslink, "%s/%s", hr.str, linkpath);
      else
        snprintf(abslink, sizeof abslink, "%s", linkpath);
    }
    n = strlen(abslink);
    while (n > 1 && (abslink[n - 1] == '/' || abslink[n - 1] == '\\')) {
      abslink[n - 1] = 0;
      n--;
    }
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", abslink);
      n = strlen(work);
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "RELINKOUT: mkdir parent failed";
      var_set_str(vm, "LAST", abslink);
      var_set_str(vm, "RELINKOUT_TARGET", target);
      var_set_str(vm, "RELINKOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", abslink);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "RELINKOUT_N", 0);
      var_set_num(vm, "RELINKOUT_CREATED", 0);
      var_set_num(vm, "RELINKOUT_REPLACED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    {
      int present = 0;
      memset(&trash, 0, sizeof trash);
      cubalc_host_islink(abslink, &trash);
      if (trash.n == 1 || cubalc_host_exists(abslink))
        present = 1;
      if (present) {
        memset(&trash, 0, sizeof trash);
        /* unlink path name (symlink node or file). Prefer unlink for links. */
        if (unlink(abslink) != 0 && cubalc_host_rm(abslink, &trash) != 0) {
          const char *err = trash.err[0] ? trash.err : "RELINKOUT: rm existing failed";
          var_set_str(vm, "LAST", abslink);
          var_set_str(vm, "RELINKOUT_TARGET", target);
          var_set_str(vm, "RELINKOUT_PARENT", absparent);
          var_set_str(vm, "LAST_ERR", err);
          var_set_str(vm, "ERR", err);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", abslink);
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "RELINKOUT_N", 0);
          var_set_num(vm, "RELINKOUT_CREATED", (long)created);
          var_set_num(vm, "RELINKOUT_REPLACED", 0);
          var_set_num(vm, "OK", 0);
          bump(vm); return 1;
        }
        replaced = 1;
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_symlink(target, abslink, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "RELINKOUT: symlink failed";
      var_set_str(vm, "LAST", abslink);
      var_set_str(vm, "RELINKOUT_TARGET", target);
      var_set_str(vm, "RELINKOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", abslink);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "RELINKOUT_N", 0);
      var_set_num(vm, "RELINKOUT_CREATED", (long)created);
      var_set_num(vm, "RELINKOUT_REPLACED", (long)replaced);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    var_set_str(vm, "LAST", abslink);
    var_set_str(vm, "RELINKOUT", abslink);
    var_set_str(vm, "LINKOUT", abslink);
    var_set_str(vm, "PATH", abslink);
    var_set_str(vm, "SYMLINK", abslink);
    var_set_str(vm, "RELINKOUT_TARGET", target);
    var_set_str(vm, "LINKOUT_TARGET", target);
    var_set_str(vm, "SYMLINK_TARGET", target);
    var_set_str(vm, "RELINKOUT_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", abslink);
    vm->last_n = hr.n > 0 ? hr.n : 1;
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "RELINKOUT_N", vm->last_n);
    var_set_num(vm, "LINKOUT_N", vm->last_n);
    var_set_num(vm, "SYMLINK_N", vm->last_n);
    var_set_num(vm, "RELINKOUT_CREATED", (long)created);
    var_set_num(vm, "RELINKOUT_REPLACED", (long)replaced);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# sys relinkout → %s → %s replaced=%d created=%d\n",
              abslink, target, replaced, created);
    bump(vm); return 1;
  }
  /* SYS TEEOUT|TEEWOUT path data — mkdir parent + write keep content in LAST.
   * Twin of top-level TEEOUT. LAST = data · TEEOUT_PATH = abs file · LAST_N=bytes. */
  if (kw(&L->cur,"TEEOUT") || kw(&L->cur,"TEEWOUT") || kw(&L->cur,"ENSURETEE") ||
      kw(&L->cur,"TEEPARENT") || kw(&L->cur,"NESTTEE") || kw(&L->cur,"PLATETEE") ||
      kw(&L->cur,"SAVEKEEPOUT") || kw(&L->cur,"WRITEKEEPOUT") || kw(&L->cur,"KEEPWRITEOUT")){
    char path[CUBALC_HOST_STR_MAX], data[CUBALC_HOST_STR_MAX];
    char absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0;
    cubalc_host_result hr;
    lex_next(L);
    path[0] = 0; data[0] = 0;
    absfile[0] = 0; parent[0] = 0; absparent[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
      fail(vm, "SYS TEEOUT path data");
      return -1;
    }
    if (resolve_str_arg(vm, L, data, sizeof data) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(data, sizeof data, "%ld", nv);
      } else {
        snprintf(data, sizeof data, "%s", vm->last_str);
      }
    }
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "TEEOUT: empty path");
      var_set_str(vm, "ERR", "TEEOUT: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "TEEOUT_N", 0);
      var_set_num(vm, "TEEOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(path, &hr) == 0 && hr.str[0])
      snprintf(absfile, sizeof absfile, "%s", hr.str);
    else
      snprintf(absfile, sizeof absfile, "%s", path);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absfile);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "TEEOUT: mkdir parent failed";
      var_set_str(vm, "LAST", data);
      var_set_str(vm, "TEEOUT_PATH", absfile);
      var_set_str(vm, "TEEOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", data);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "TEEOUT_N", 0);
      var_set_num(vm, "TEEOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_write(absfile, data, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "TEEOUT: write failed";
      var_set_str(vm, "LAST", data);
      var_set_str(vm, "TEEOUT_PATH", absfile);
      var_set_str(vm, "TEEOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", data);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "TEEOUT_N", 0);
      var_set_num(vm, "TEEOUT_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    /* LAST = body (not path) — chain HAS/GREP without READ back */
    var_set_str(vm, "LAST", data);
    var_set_str(vm, "TEE", data);
    var_set_str(vm, "TEEOUT", data);
    var_set_str(vm, "TEEOUT_PATH", absfile);
    var_set_str(vm, "TEE_PATH", absfile);
    var_set_str(vm, "PATH", absfile);
    var_set_str(vm, "TEEOUT_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", data);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "TEEOUT_N", hr.n);
    var_set_num(vm, "TEE_N", hr.n);
    var_set_num(vm, "TEEOUT_CREATED", (long)created);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# sys teeout → %s bytes=%ld created=%d\n",
              absfile, hr.n, created);
    bump(vm); return 1;
  }
  /* SYS CLAIMOUT|CLAIMNEST path [body] — mkdir parent + exclusive create (soft).
   * Twin of top-level CLAIMOUT. LAST = abs path · CLAIM_HIT 1|0 · CLAIMOUT_CREATED. */
  if (kw(&L->cur,"CLAIMOUT") || kw(&L->cur,"CLAIMNEST") || kw(&L->cur,"ENSURECLAIM") ||
      kw(&L->cur,"CLAIMPARENT") || kw(&L->cur,"EXCLOUT") || kw(&L->cur,"TAKECLAIMOUT") ||
      kw(&L->cur,"ONCEOUT") || kw(&L->cur,"WORKCLAIM") || kw(&L->cur,"JOBCLAIM")){
    char path[CUBALC_HOST_STR_MAX], body[CUBALC_HOST_STR_MAX];
    char absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0, hit = 0;
    long pid = 0;
    cubalc_host_result hr;
    lex_next(L);
    path[0] = 0; body[0] = 0;
    absfile[0] = 0; parent[0] = 0; absparent[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
      fail(vm, "SYS CLAIMOUT path [body]");
      return -1;
    }
    if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (resolve_str_arg(vm, L, body, sizeof body) != 0)
        body[0] = 0;
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long nv = parse_expr(vm, L);
      snprintf(body, sizeof body, "%ld", nv);
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
      var_set_str(vm, "LAST_ERR", "CLAIMOUT: empty path");
      var_set_str(vm, "ERR", "CLAIMOUT: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "CLAIMOUT_N", 0);
      var_set_num(vm, "CLAIM_HIT", 0);
      var_set_num(vm, "CLAIMOUT_CREATED", 0);
      var_set_num(vm, "CLAIM_PID", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(path, &hr) == 0 && hr.str[0])
      snprintf(absfile, sizeof absfile, "%s", hr.str);
    else
      snprintf(absfile, sizeof absfile, "%s", path);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absfile);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "CLAIMOUT: mkdir parent failed";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "CLAIMOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "CLAIMOUT_N", 0);
      var_set_num(vm, "CLAIM_HIT", 0);
      var_set_num(vm, "CLAIMOUT_CREATED", 0);
      var_set_num(vm, "CLAIM_PID", 0);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
#if defined(CUBALC_OS_WINDOWS)
    {
      FILE *lf = fopen(absfile, "r");
      if (!lf) {
        cubalc_host_result wr;
        if (cubalc_host_write(absfile, body, &wr) == 0)
          hit = 1;
      } else {
        fclose(lf);
      }
    }
#else
    {
      int fd = open(absfile, O_CREAT | O_EXCL | O_WRONLY, 0644);
      if (fd >= 0) {
        size_t bl = strlen(body);
        ssize_t w = write(fd, body, bl);
        (void)w;
        close(fd);
        hit = 1;
      }
    }
#endif
    if (hit) {
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "CLAIMOUT", absfile);
      var_set_str(vm, "CLAIM", absfile);
      var_set_str(vm, "CLAIMFILE", absfile);
      var_set_str(vm, "PATH", absfile);
      var_set_str(vm, "CLAIMOUT_PARENT", absparent);
      var_set_str(vm, "DIRNAME", absparent);
      var_set_str(vm, "PARENT", absparent);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "CLAIMOUT_N", 1);
      var_set_num(vm, "CLAIM_HIT", 1);
      var_set_num(vm, "CLAIM_N", 1);
      var_set_num(vm, "CLAIM_PID", pid);
      var_set_num(vm, "CLAIMOUT_CREATED", (long)created);
      var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
      var_set_num(vm, "PATH_EXIST", 1);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys claimout → %s hit=1 created=%d\n", absfile, created);
      bump(vm); return 1;
    }
    var_set_str(vm, "LAST", absfile);
    var_set_str(vm, "CLAIMOUT", absfile);
    var_set_str(vm, "CLAIMOUT_PARENT", absparent);
    var_set_str(vm, "LAST_ERR", "CLAIMOUT: exists");
    var_set_str(vm, "ERR", "CLAIMOUT: exists");
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
    vm->last_n = 0;
    var_set_num(vm, "LAST_N", 0);
    var_set_num(vm, "CLAIMOUT_N", 0);
    var_set_num(vm, "CLAIM_HIT", 0);
    var_set_num(vm, "CLAIM_N", 0);
    var_set_num(vm, "CLAIM_PID", 0);
    var_set_num(vm, "CLAIMOUT_CREATED", (long)created);
    var_set_num(vm, "OK", 0);
    if (vm->trace)
      fprintf(vm->trace, "# sys claimout → %s hit=0 created=%d\n", absfile, created);
    bump(vm); return 1;
  }
  return 0;
}
