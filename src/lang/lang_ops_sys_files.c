/* CubalC lang — lang_ops_sys_files.c
 * SYS/HOST megablock dispatch (mesh vote A_split_lang_ops_core).
 * Peel of ~33k-line SYS plane from lang_ops_core.
 */
#include "lang/cubalc_lang_ops_sys_files.h"
#include "lang/cubalc_lang_ops_sys_files_bulk.h"
#include "lang/cubalc_lang_ops_sys_files_match.h"
#include "lang/cubalc_lang_ops_sys_files_net.h"
#include "lang/cubalc_lang_ops_sys_files_line.h"
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

int cubalc_lang_ops_sys_try_files(VM *vm, Lex *L) {
  /* bulk multi-file → lang_ops_sys_files_bulk.c */
  {
    int fb = cubalc_lang_ops_sys_try_files_bulk(vm, L);
    if (fb != 0) return fb;
  }

  /* line HEADFILE..COUNTFILELINE → lang_ops_sys_files_line.c (B_sys_files_peel) */
  {
    int fl = cubalc_lang_ops_sys_try_files_line(vm, L);
    if (fl != 0) return fl;
  }

  /* match/KV → lang_ops_sys_files_match.c */
  {
    int fm = cubalc_lang_ops_sys_try_files_match(vm, L);
    if (fm != 0) return fm;
  }

  /* READALL/WHICH/HTTP → lang_ops_sys_files_net.c */
  {
    int fn = cubalc_lang_ops_sys_try_files_net(vm, L);
    if (fn != 0) return fn;
  }

  /* cwd/temp CWD..TMP → lang_ops_sys_files_cwd.c (B_sys_files_cwd) */
  {
    int fc = cubalc_lang_ops_sys_try_files_cwd(vm, L);
    if (fc != 0) return fc;
  }

  return 0;
}

