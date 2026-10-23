/* CubalC lang — lang_ops_sys_path_tree.c
 * SYS TREE plane: RMTREE · CPTREE · MVTREE · DIRSIZE · EQTREE · GREPTREE ·
 *                 REPLACETREE · *INTREE harvest · PATHLINETREE
 * Peel from path_fs (B_sys_path_peel). Cube is SoT. Zero bloat.
 */
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
#  include <fcntl.h>
#  include <fnmatch.h>
#endif

int cubalc_lang_ops_sys_try_path_tree(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. SYS already consumed. */
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

  return 0;
}
