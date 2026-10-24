/* CubalC — hostops inode meta plane peel (B_hostops_margin).
 * nlink · sameinode · inode · fileuid · filegid (SYS plate identity stamps).
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

/* Usability: SYS NLINK path — hard-link count for shared-plate IF guards. */
int cubalc_host_nlink(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "nlink: empty path");
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "nlink: missing");
    return -1;
  }
  r->n = (long)st.st_nlink;
  snprintf(r->str, sizeof r->str, "%ld", r->n);
  r->ok = 1;
  return 0;
}

/* Usability: SYS SAMEINODE a b — true if HARDLINK pair (dev+ino), not content EQFILE. */
int cubalc_host_sameinode(const char *a, const char *b, cubalc_host_result *r) {
  struct stat sa, sb;
  r_clear(r);
  if (!a || !a[0] || !b || !b[0]) {
    snprintf(r->err, sizeof r->err, "sameinode: empty path");
    return -1;
  }
  if (stat(a, &sa) != 0 || stat(b, &sb) != 0) {
    snprintf(r->err, sizeof r->err, "sameinode: missing");
    return -1;
  }
  r->n = (sa.st_dev == sb.st_dev && sa.st_ino == sb.st_ino) ? 1 : 0;
  snprintf(r->str, sizeof r->str, "%ld", r->n);
  r->ok = 1;
  return 0;
}

/* Usability: SYS INODE path — plate identity stamp without shell stat. */
int cubalc_host_inode(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "inode: empty path");
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "inode: missing");
    return -1;
  }
  r->n = (long)st.st_ino;
  snprintf(r->str, sizeof r->str, "%ld", r->n);
  r->ok = 1;
  return 0;
}

/* Usability: SYS FILEUID path — plate owner uid without shell stat. */
int cubalc_host_fileuid(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "fileuid: empty path");
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "fileuid: missing");
    return -1;
  }
  r->n = (long)st.st_uid;
  snprintf(r->str, sizeof r->str, "%ld", r->n);
  r->ok = 1;
  return 0;
}

/* Usability: SYS FILEGID path — plate group gid without shell stat. */
int cubalc_host_filegid(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "filegid: empty path");
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "filegid: missing");
    return -1;
  }
  r->n = (long)st.st_gid;
  snprintf(r->str, sizeof r->str, "%ld", r->n);
  r->ok = 1;
  return 0;
}

