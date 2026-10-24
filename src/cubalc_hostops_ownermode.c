/* CubalC — hostops owner/mode plane peel (B_hostops_margin).
 * ownername · groupname · mode · chmod (SYS plate identity + perms).
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
#if !defined(CUBALC_OS_WINDOWS)
#  include <pwd.h>
#  include <grp.h>
#endif

static void r_clear(cubalc_host_result *r) {
  if (!r) return;
  memset(r, 0, sizeof *r);
}

/* Usability: SYS OWNERNAME path — login name for st_uid without shell stat -c %U. */
int cubalc_host_ownername(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "ownername: empty path");
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "ownername: missing");
    return -1;
  }
  r->n = (long)st.st_uid;
#if !defined(CUBALC_OS_WINDOWS)
  {
    struct passwd *pw = getpwuid(st.st_uid);
    if (pw && pw->pw_name && pw->pw_name[0]) {
      snprintf(r->str, sizeof r->str, "%s", pw->pw_name);
      r->ok = 1;
      return 0;
    }
  }
#endif
  /* numeric fallback when passwd has no entry (containers, deleted users) */
  snprintf(r->str, sizeof r->str, "%ld", r->n);
  r->ok = 1;
  return 0;
}

/* Usability: SYS GROUPNAME path — group name for st_gid without shell stat -c %G. */
int cubalc_host_groupname(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "groupname: empty path");
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "groupname: missing");
    return -1;
  }
  r->n = (long)st.st_gid;
#if !defined(CUBALC_OS_WINDOWS)
  {
    struct group *gr = getgrgid(st.st_gid);
    if (gr && gr->gr_name && gr->gr_name[0]) {
      snprintf(r->str, sizeof r->str, "%s", gr->gr_name);
      r->ok = 1;
      return 0;
    }
  }
#endif
  snprintf(r->str, sizeof r->str, "%ld", r->n);
  r->ok = 1;
  return 0;
}

/* Usability: SYS MODE path — octal permission stamp without shell stat -c %a. */
int cubalc_host_mode(const char *path, cubalc_host_result *r) {
  struct stat st;
  unsigned m;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "mode: empty path");
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "mode: missing");
    return -1;
  }
  m = (unsigned)(st.st_mode & 07777);
  r->n = (long)m;
  snprintf(r->str, sizeof r->str, "%04o", m);
  r->ok = 1;
  return 0;
}

/* Usability: SYS CHMOD path mode — set plate perms without shell chmod. */
int cubalc_host_chmod(const char *path, long mode, cubalc_host_result *r) {
  mode_t m;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "chmod: empty path");
    return -1;
  }
  if (mode < 0) mode = 0;
  m = (mode_t)(mode & 07777);
  if (chmod(path, m) != 0) {
    snprintf(r->err, sizeof r->err, "chmod: %s", strerror(errno));
    return -1;
  }
  r->n = (long)m;
  snprintf(r->str, sizeof r->str, "%04o", (unsigned)m);
  r->ok = 1;
  return 0;
}

