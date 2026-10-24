/* CubalC — hostops link plane peel (B_hostops_margin).
 * symlink · hardlink · readlink · islink (SYS plate link ops).
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
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

static void r_clear(cubalc_host_result *r) {
  if (!r) return;
  memset(r, 0, sizeof *r);
}

/* Usability: SYS SYMLINK|LN target linkpath — plate alias without shell ln -s. */
int cubalc_host_symlink(const char *target, const char *linkpath, cubalc_host_result *r) {
  r_clear(r);
  if (!target || !target[0] || !linkpath || !linkpath[0]) {
    snprintf(r->err, sizeof r->err, "symlink: empty path");
    return -1;
  }
  if (access(linkpath, F_OK) == 0) {
    snprintf(r->err, sizeof r->err, "symlink: link exists");
    return -1;
  }
  if (symlink(target, linkpath) != 0) {
    snprintf(r->err, sizeof r->err, "symlink: %s", strerror(errno));
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", linkpath);
  r->n = 1;
  r->ok = 1;
  return 0;
}

/* Usability: SYS HARDLINK existing newpath — shared plate inode without copy/shell ln. */
int cubalc_host_hardlink(const char *existing, const char *newpath, cubalc_host_result *r) {
  r_clear(r);
  if (!existing || !existing[0] || !newpath || !newpath[0]) {
    snprintf(r->err, sizeof r->err, "hardlink: empty path");
    return -1;
  }
  if (access(existing, F_OK) != 0) {
    snprintf(r->err, sizeof r->err, "hardlink: source missing");
    return -1;
  }
  if (access(newpath, F_OK) == 0) {
    snprintf(r->err, sizeof r->err, "hardlink: dest exists");
    return -1;
  }
  if (link(existing, newpath) != 0) {
    snprintf(r->err, sizeof r->err, "hardlink: %s", strerror(errno));
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", newpath);
  r->n = 1;
  r->ok = 1;
  return 0;
}

/* Usability: SYS READLINK path — peel symlink target without shell. */
int cubalc_host_readlink(const char *path, cubalc_host_result *r) {
  char buf[CUBALC_HOST_STR_MAX];
  ssize_t n;
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "readlink: empty path");
    return -1;
  }
  if (lstat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "readlink: missing");
    return -1;
  }
  if (!S_ISLNK(st.st_mode)) {
    snprintf(r->err, sizeof r->err, "readlink: not a symlink");
    return -1;
  }
  n = readlink(path, buf, sizeof buf - 1);
  if (n < 0) {
    snprintf(r->err, sizeof r->err, "readlink: %s", strerror(errno));
    return -1;
  }
  buf[n] = 0;
  snprintf(r->str, sizeof r->str, "%s", buf);
  r->n = (long)n;
  r->ok = 1;
  return 0;
}

/* Usability: SYS ISLINK path — probe without READLINK soft-fail glue. */
int cubalc_host_islink(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  if (lstat(path, &st) != 0) {
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  r->n = S_ISLNK(st.st_mode) ? 1 : 0;
  snprintf(r->str, sizeof r->str, "%s", path);
  r->ok = 1;
  return 0;
}

