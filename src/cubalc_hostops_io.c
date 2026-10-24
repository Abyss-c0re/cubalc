/* CubalC — hostops core I/O peel (B_hostops_margin).
 * exists · path_kind · read · write. Mesh vote · zero bloat · cube is SoT.
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
#include <errno.h>
#include <sys/stat.h>

static void r_clear(cubalc_host_result *r) {
  if (!r) return;
  memset(r, 0, sizeof *r);
}

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
