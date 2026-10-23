/* CubalC lang — lang_ops_sys_path_link.c
 * SYS LINK/STAT: SYMLINK · HARDLINK · NLINK · SAMEINODE · INODE ·
 *                FILEUID · CANREAD · CANCREATE · OWNERNAME · READLINK
 * Peel from path_fs (B_sys_path_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_sys_path_link.h"
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
#  include <sys/stat.h>
#  include <pwd.h>
#  include <grp.h>
#  include <fcntl.h>
#endif

int cubalc_lang_ops_sys_try_path_link(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. SYS already consumed. */
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

  return 0;
}
