/* CubalC — hostops path peel (B_hostops_margin).
 * SYS RELPATH · KINDSTR · ABSPATH/REALPATH · JOIN. Mesh vote · zero bloat · cube is SoT.
 */
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
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>

static void r_clear(cubalc_host_result *r) {
  if (!r) return;
  memset(r, 0, sizeof *r);
}

/* Usability: SYS RELPATH base path — portable relative plate paths without shell. */
int cubalc_host_relpath(const char *base, const char *path, cubalc_host_result *r) {
  cubalc_host_result ba, pa;
  size_t bl, pl;
  r_clear(r);
  if (!base || !base[0] || !path || !path[0]) {
    snprintf(r->err, sizeof r->err, "relpath: empty base or path");
    return -1;
  }
  memset(&ba, 0, sizeof ba);
  memset(&pa, 0, sizeof pa);
  if (cubalc_host_abspath(base, &ba) != 0 || !ba.str[0]) {
    snprintf(r->err, sizeof r->err, "relpath: bad base");
    return -1;
  }
  if (cubalc_host_abspath(path, &pa) != 0 || !pa.str[0]) {
    snprintf(r->err, sizeof r->err, "relpath: bad path");
    return -1;
  }
  bl = strlen(ba.str);
  pl = strlen(pa.str);
  while (bl > 1 && ba.str[bl - 1] == '/') {
    ba.str[bl - 1] = 0;
    bl--;
  }
  while (pl > 1 && pa.str[pl - 1] == '/') {
    pa.str[pl - 1] = 0;
    pl--;
  }
  if (strcmp(ba.str, pa.str) == 0) {
    snprintf(r->str, sizeof r->str, "%s", ".");
    r->n = 1;
    r->code = 1; /* under / same */
    r->ok = 1;
    return 0;
  }
  if (pl > bl && strncmp(pa.str, ba.str, bl) == 0 && pa.str[bl] == '/') {
    snprintf(r->str, sizeof r->str, "%s", pa.str + bl + 1);
    r->n = (long)strlen(r->str);
    r->code = 1;
    r->ok = 1;
    return 0;
  }
  /* outside base — keep absolute */
  snprintf(r->str, sizeof r->str, "%s", pa.str);
  r->n = (long)strlen(r->str);
  r->code = 0;
  r->ok = 1;
  return 0;
}

/* Usability: SYS KINDSTR path — human path kind without multi ISDIR/ISFILE/ISLINK glue. */
int cubalc_host_kindstr(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "kindstr: empty path");
    return -1;
  }
  if (lstat(path, &st) != 0) {
    snprintf(r->str, sizeof r->str, "%s", "missing");
    r->n = 0;
    r->code = 0;
    r->ok = 1; /* soft probe — not a hard fail */
    return 0;
  }
  if (S_ISLNK(st.st_mode)) {
    snprintf(r->str, sizeof r->str, "%s", "link");
    r->n = 3;
  } else if (S_ISREG(st.st_mode)) {
    snprintf(r->str, sizeof r->str, "%s", "file");
    r->n = 1;
  } else if (S_ISDIR(st.st_mode)) {
    snprintf(r->str, sizeof r->str, "%s", "dir");
    r->n = 2;
  } else if (S_ISFIFO(st.st_mode)) {
    snprintf(r->str, sizeof r->str, "%s", "fifo");
    r->n = 4;
  } else if (S_ISSOCK(st.st_mode)) {
    snprintf(r->str, sizeof r->str, "%s", "sock");
    r->n = 5;
  } else if (S_ISCHR(st.st_mode)) {
    snprintf(r->str, sizeof r->str, "%s", "chr");
    r->n = 6;
  } else if (S_ISBLK(st.st_mode)) {
    snprintf(r->str, sizeof r->str, "%s", "blk");
    r->n = 7;
  } else {
    snprintf(r->str, sizeof r->str, "%s", "other");
    r->n = 8;
  }
  r->code = (int)r->n;
  r->ok = 1;
  return 0;
}

int cubalc_host_abspath(const char *path, cubalc_host_result *r) {
  char cwd[512], joined[1024], resolved[4096];
  const char *p = path ? path : "";
  size_t n;
  r_clear(r);
  /* empty → cwd */
  if (!p[0] || strcmp(p, ".") == 0) {
    if (!getcwd(r->str, sizeof r->str)) {
      snprintf(r->err, sizeof r->err, "abspath: getcwd fail");
      return -1;
    }
    r->n = (long)strlen(r->str);
    r->ok = 1;
    return 0;
  }
  /* try realpath when node exists (or intermediate for symlink) */
  if (realpath(p, resolved) != NULL) {
    snprintf(r->str, sizeof r->str, "%s", resolved);
    r->n = (long)strlen(r->str);
    r->ok = 1;
    return 0;
  }
  /* absolute path that does not exist yet — return as-is (strip trailing /) */
  if (p[0] == '/') {
    snprintf(r->str, sizeof r->str, "%s", p);
    n = strlen(r->str);
    while (n > 1 && r->str[n - 1] == '/') {
      r->str[n - 1] = 0;
      n--;
    }
    r->n = (long)n;
    r->ok = 1;
    return 0;
  }
  /* relative missing — join with cwd */
  if (!getcwd(cwd, sizeof cwd)) {
    snprintf(r->err, sizeof r->err, "abspath: getcwd fail");
    return -1;
  }
  n = strlen(cwd);
  while (n > 1 && cwd[n - 1] == '/') {
    cwd[n - 1] = 0;
    n--;
  }
  /* strip leading ./ from relative */
  if (p[0] == '.' && p[1] == '/')
    p += 2;
  snprintf(joined, sizeof joined, "%s/%s", cwd, p);
  /* light normalize: remove trailing slash */
  n = strlen(joined);
  while (n > 1 && joined[n - 1] == '/') {
    joined[n - 1] = 0;
    n--;
  }
  snprintf(r->str, sizeof r->str, "%s", joined);
  r->n = (long)strlen(r->str);
  r->ok = 1;
  return 0;
}

int cubalc_host_join(const char *a, const char *b, cubalc_host_result *r) {
  r_clear(r);
  if (!a) a = "";
  if (!b) b = "";
  size_t la = strlen(a), lb = strlen(b);
  int need_slash = 0;
  if (la > 0 && lb > 0 && a[la - 1] != '/' && b[0] != '/') need_slash = 1;
  if (la + lb + need_slash + 1 >= sizeof r->str) {
    snprintf(r->err, sizeof r->err, "join: too long");
    return -1;
  }
  if (need_slash)
    snprintf(r->str, sizeof r->str, "%s/%s", a, b);
  else
    snprintf(r->str, sizeof r->str, "%s%s", a, b);
  r->n = (long)strlen(r->str);
  r->ok = 1;
  return 0;
}

