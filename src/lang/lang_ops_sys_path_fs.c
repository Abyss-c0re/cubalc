/* CubalC lang — lang_ops_core_sys.c
 * SYS/HOST megablock dispatch (mesh vote A_split_lang_ops_core).
 * Peel of ~33k-line SYS plane from lang_ops_core.
 */
#include "lang/cubalc_lang_ops_sys_path_fs.h"
#include "lang/cubalc_lang_ops_sys_path_writeout.h"
#include "lang/cubalc_lang_ops_sys_path_sync.h"
#include "lang/cubalc_lang_ops_sys_path_util.h"
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

  /* RENAME/GLOB/util → lang_ops_sys_path_util.c */
  {
    int pu = cubalc_lang_ops_sys_try_path_util(vm, L);
    if (pu != 0) return pu;
  }

  return 0;
}
