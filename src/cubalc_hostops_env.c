/* CubalC — hostops env plane peel (B_hostops_margin).
 * env · env_set · env_unset (SYS ENV / SETENV / UNSETENV).
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void r_clear(cubalc_host_result *r) {
  if (!r) return;
  memset(r, 0, sizeof *r);
}

int cubalc_host_env(const char *name, cubalc_host_result *r) {
  r_clear(r);
  if (!name) { snprintf(r->err, sizeof r->err, "env: empty"); return -1; }
  const char *v = getenv(name);
  if (!v) { r->ok = 1; r->str[0] = 0; r->n = 0; return 0; }
  snprintf(r->str, sizeof r->str, "%s", v);
  r->n = (long)strlen(r->str);
  r->ok = 1;
  return 0;
}

/* Usability: in-language CUBALC_* / agent config without shell export.
 * Process-local only (setenv overwrite). Empty val → set empty string. */
int cubalc_host_env_set(const char *name, const char *val, cubalc_host_result *r) {
  r_clear(r);
  if (!name || !name[0]) {
    snprintf(r->err, sizeof r->err, "env set: empty name");
    return -1;
  }
  if (!val) val = "";
  if (setenv(name, val, 1) != 0) {
    snprintf(r->err, sizeof r->err, "env set: %s", strerror(errno));
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", val);
  r->n = (long)strlen(r->str);
  r->ok = 1;
  return 0;
}

/* Soft: already-absent is OK with n=0; removed → n=1. */
int cubalc_host_env_unset(const char *name, cubalc_host_result *r) {
  int was;
  r_clear(r);
  if (!name || !name[0]) {
    snprintf(r->err, sizeof r->err, "env unset: empty name");
    return -1;
  }
  was = (getenv(name) != NULL) ? 1 : 0;
  if (unsetenv(name) != 0) {
    snprintf(r->err, sizeof r->err, "env unset: %s", strerror(errno));
    return -1;
  }
  r->str[0] = 0;
  r->n = was;
  r->ok = 1;
  return 0;
}

