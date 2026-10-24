/* CubalC — hostops stat time plane peel (B_hostops_margin).
 * mtime · atime · ctime (SYS MTIME/ATIME/CTIME epoch stamps).
 * Mesh vote · zero bloat · cube is SoT.
 */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include "cubalc_hostops.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

static void r_clear(cubalc_host_result *r) {
  if (!r) return;
  memset(r, 0, sizeof *r);
}

/* Usability: SYS MTIME / AGE — plate freshness without shell stat.
 * r->n = st_mtime (epoch seconds). Soft miss: ok=0 code=0. */
int cubalc_host_mtime(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "mtime: empty path");
    r->code = 0;
    r->n = 0;
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "mtime: missing");
    r->code = 0;
    r->n = 0;
    r->ok = 0;
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", path);
  r->n = (long)st.st_mtime;
  r->ok = 1;
  if (S_ISREG(st.st_mode)) r->code = 1;
  else if (S_ISDIR(st.st_mode)) r->code = 2;
  else r->code = 3;
  return 0;
}

/* Usability: SYS ATIME path — last access epoch without shell stat -c %X. */
int cubalc_host_atime(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "atime: empty path");
    r->code = 0;
    r->n = 0;
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "atime: missing");
    r->code = 0;
    r->n = 0;
    r->ok = 0;
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", path);
  r->n = (long)st.st_atime;
  r->ok = 1;
  if (S_ISREG(st.st_mode)) r->code = 1;
  else if (S_ISDIR(st.st_mode)) r->code = 2;
  else r->code = 3;
  return 0;
}

/* Usability: SYS CTIME path — inode status-change epoch without shell stat -c %Z. */
int cubalc_host_ctime(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "ctime: empty path");
    r->code = 0;
    r->n = 0;
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "ctime: missing");
    r->code = 0;
    r->n = 0;
    r->ok = 0;
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", path);
  r->n = (long)st.st_ctime;
  r->ok = 1;
  if (S_ISREG(st.st_mode)) r->code = 1;
  else if (S_ISDIR(st.st_mode)) r->code = 2;
  else r->code = 3;
  return 0;
}
