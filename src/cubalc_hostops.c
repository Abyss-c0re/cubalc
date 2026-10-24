/* glibc: realpath + POSIX APIs without macro fights */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <dirent.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <pwd.h>
#  include <grp.h>
#endif

static void r_clear(cubalc_host_result *r) {
  if (!r) return;
  memset(r, 0, sizeof *r);
}

/* protect_mode·url_allowed·bin_allowed·http·spawn → cubalc_hostops_httpspawn.c (B_hostops_margin) */

int cubalc_host_exists(const char *path) {
  struct stat st;
  if (!path || !path[0]) return 0;
  return stat(path, &st) == 0 ? 1 : 0;
}

/* Usability: SYS SIZE / ISDIR / ISFILE — path metadata without full READ.
 * r->code: 0 missing, 1 regular file, 2 directory, 3 other
 * r->n: byte size for regular files (0 for dirs/other/missing) */
int cubalc_host_path_kind(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "path: empty");
    r->code = 0;
    r->n = 0;
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "path: missing");
    r->code = 0;
    r->n = 0;
    r->ok = 0;
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", path);
  if (S_ISREG(st.st_mode)) {
    r->code = 1;
    r->n = (long)st.st_size;
    r->ok = 1;
    return 0;
  }
  if (S_ISDIR(st.st_mode)) {
    r->code = 2;
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  r->code = 3;
  r->n = (long)st.st_size;
  r->ok = 1;
  return 0;
}

/* mtime·atime·ctime → cubalc_hostops_stat_time.c (B_hostops_margin) */

int cubalc_host_read(const char *path, cubalc_host_result *r) {
  r_clear(r);
  if (!path || !path[0]) { snprintf(r->err, sizeof r->err, "read: empty path"); return -1; }
  FILE *f = fopen(path, "rb");
  if (!f) { snprintf(r->err, sizeof r->err, "read: %s", strerror(errno)); return -1; }
  size_t n = fread(r->str, 1, sizeof r->str - 1, f);
  r->str[n] = 0;
  fclose(f);
  r->n = (long)n;
  r->ok = 1;
  r->code = 0;
  return 0;
}

int cubalc_host_write(const char *path, const char *data, cubalc_host_result *r) {
  r_clear(r);
  if (!path || !path[0]) { snprintf(r->err, sizeof r->err, "write: empty path"); return -1; }
  FILE *f = fopen(path, "wb");
  if (!f) { snprintf(r->err, sizeof r->err, "write: %s", strerror(errno)); return -1; }
  const char *d = data ? data : "";
  size_t n = fwrite(d, 1, strlen(d), f);
  fclose(f);
  r->n = (long)n;
  r->ok = 1;
  return 0;
}

/* rm·rmdir·rename·can_access·can_create → cubalc_hostops_fs_mut.c (B_hostops_margin) */
/* bag_push dead in monolit — tree/json shards own copies */

/* umask·pushd·popd·dirstack → cubalc_hostops_dirstack.c (B_hostops_margin) */

/* cubalc_host_relpath → cubalc_hostops_path.c (B_hostops_margin) */


/* cubalc_host_kindstr → cubalc_hostops_path.c (B_hostops_margin) */


/* groups → cubalc_hostops_groupscopy.c (B_hostops_margin) */


/* ingroup → cubalc_hostops_groupscopy.c (B_hostops_margin) */


/* ownername·groupname → cubalc_hostops_ownermode.c (B_hostops_margin) */

/* readlink·islink → cubalc_hostops_link.c (B_hostops_margin) */

/* mode·chmod → cubalc_hostops_ownermode.c (B_hostops_margin) */

/* copy → cubalc_hostops_groupscopy.c (B_hostops_margin) */


/* Usability: SYS REALPATH|ABSPATH path — absolute path without shell.
 * Existing paths: realpath(3) (resolves . .. symlinks).
 * Missing/relative: cwd + "/" + path (no fatal). Empty path → cwd. */
/* cubalc_host_abspath → cubalc_hostops_path.c (B_hostops_margin) */


/* touch·listdir·mkdir → cubalc_hostops_fs_basic.c (B_hostops_margin) */

/* env·env_set·env_unset → cubalc_hostops_env.c (B_hostops_margin) */

/* find_cubalc·which_bin·which → cubalc_hostops_which.c (B_hostops_margin) */

/* cubalc_host_http · cubalc_host_spawn → cubalc_hostops_httpspawn.c (B_hostops_margin) */

/* ---- Grokium CubalC surface: join / json / chat ---- */

/* cubalc_host_join → cubalc_hostops_path.c (B_hostops_margin) */



/* ---- Grokium surface peels (already out of monolit) ----
 * json_* → json_*.c · chat → hostops_chat.c
 */
