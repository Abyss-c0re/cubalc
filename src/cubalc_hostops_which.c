/* CubalC — hostops find_cubalc/which peel (B_hostops_margin).
 * SYS WHICH · WHICH_BIN · find_cubalc resolve. Mesh vote · zero bloat · cube is SoT.
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

static void r_clear(cubalc_host_result *r) {
  if (!r) return;
  memset(r, 0, sizeof *r);
}

/* Usability: INCLUDE-style resolve — short lib name → programs/lib/<stem>.cubalc
 * Also programs/, programs/proof|p2p|protect, CUBALC_ROOT, readable path as-is. */
int cubalc_host_find_cubalc(const char *name, cubalc_host_result *r) {
  char stem[256], try[768];
  const char *root, *slash, *leaf;
  size_t blen;
  int i;
  static const char *dirs[] = {
    "programs/lib", "programs", "programs/proof", "programs/p2p",
    "programs/protect", "docs", NULL
  };
  r_clear(r);
  if (!name || !name[0]) {
    snprintf(r->err, sizeof r->err, "find: empty name");
    return -1;
  }
  /* path as given (readable) */
  if (access(name, R_OK) == 0) {
    snprintf(r->str, sizeof r->str, "%s", name);
    r->ok = 1; r->n = 1; return 0;
  }
  slash = strrchr(name, '/');
  leaf = slash ? slash + 1 : name;
  snprintf(stem, sizeof stem, "%s", leaf);
  blen = strlen(stem);
  if (blen > 7 && strcmp(stem + blen - 7, ".cubalc") == 0)
    stem[blen - 7] = 0;
  for (i = 0; dirs[i]; i++) {
    snprintf(try, sizeof try, "%s/%s.cubalc", dirs[i], stem);
    if (access(try, R_OK) == 0) {
      snprintf(r->str, sizeof r->str, "%s", try);
      r->ok = 1; r->n = 1; return 0;
    }
    snprintf(try, sizeof try, "%s/%s", dirs[i], stem);
    if (access(try, R_OK) == 0) {
      snprintf(r->str, sizeof r->str, "%s", try);
      r->ok = 1; r->n = 1; return 0;
    }
    /* docs: COOKBOOK.md style */
    snprintf(try, sizeof try, "%s/%s.md", dirs[i], stem);
    if (access(try, R_OK) == 0) {
      snprintf(r->str, sizeof r->str, "%s", try);
      r->ok = 1; r->n = 1; return 0;
    }
  }
  root = getenv("CUBALC_ROOT");
  if (root && root[0]) {
    for (i = 0; dirs[i]; i++) {
      snprintf(try, sizeof try, "%s/%s/%s.cubalc", root, dirs[i], stem);
      if (access(try, R_OK) == 0) {
        snprintf(r->str, sizeof r->str, "%s", try);
        r->ok = 1; r->n = 1; return 0;
      }
      snprintf(try, sizeof try, "%s/%s/%s", root, dirs[i], stem);
      if (access(try, R_OK) == 0) {
        snprintf(r->str, sizeof r->str, "%s", try);
        r->ok = 1; r->n = 1; return 0;
      }
    }
  }
  /* CUBALC_INCLUDE_PATH=dir:dir — project libs (INCLUDE / which / SYS WHICH dual). */
  {
    const char *ip = getenv("CUBALC_INCLUDE_PATH");
    if (ip && ip[0]) {
      const char *seg = ip;
      while (*seg) {
        char dir[512];
        size_t dlen = 0;
        while (*seg == ':') seg++;
        if (!*seg) break;
        while (seg[dlen] && seg[dlen] != ':' && dlen + 1 < sizeof dir) {
          dir[dlen] = seg[dlen];
          dlen++;
        }
        dir[dlen] = 0;
        seg += dlen;
        if (!dir[0]) continue;
        snprintf(try, sizeof try, "%s/%s.cubalc", dir, stem);
        if (access(try, R_OK) == 0) {
          snprintf(r->str, sizeof r->str, "%s", try);
          r->ok = 1; r->n = 1; return 0;
        }
        snprintf(try, sizeof try, "%s/%s", dir, leaf);
        if (access(try, R_OK) == 0) {
          snprintf(r->str, sizeof r->str, "%s", try);
          r->ok = 1; r->n = 1; return 0;
        }
        if (!strchr(leaf, '.')) {
          snprintf(try, sizeof try, "%s/%s.cubalc", dir, leaf);
          if (access(try, R_OK) == 0) {
            snprintf(r->str, sizeof r->str, "%s", try);
            r->ok = 1; r->n = 1; return 0;
          }
        }
      }
    }
  }
  snprintf(r->err, sizeof r->err, "find: not found %s", name);
  return -1;
}

/* PATH/abs/out/local-bin only — shared by which_bin and which. */
static int cubalc_host_which_bin_inner(const char *name, cubalc_host_result *r) {
  char try[512];
  const char *home = getenv("HOME");
  const char *cbin = getenv("CUBALC_BIN");
  if (!name || !name[0]) return -1;
  if (cbin && strstr(name, "cubalc") && access(cbin, X_OK) == 0) {
    snprintf(r->str, sizeof r->str, "%s", cbin); r->ok=1; r->n=1; return 0;
  }
  if (name[0]=='/' && access(name, X_OK)==0) {
    snprintf(r->str, sizeof r->str, "%s", name); r->ok=1; r->n=1; return 0;
  }
  snprintf(try, sizeof try, "./out/%s", name);
  if (access(try, X_OK) == 0) { snprintf(r->str, sizeof r->str, "%s", try); r->ok=1; r->n=1; return 0; }
  if (home) {
    snprintf(try, sizeof try, "%s/.local/bin/%s", home, name);
    if (access(try, X_OK) == 0) { snprintf(r->str, sizeof r->str, "%s", try); r->ok=1; r->n=1; return 0; }
  }
  /* PATH search */
  {
    const char *path = getenv("PATH");
    if (path) {
      char pbuf[4096]; snprintf(pbuf, sizeof pbuf, "%s", path);
      for (char *tok = strtok(pbuf, ":"); tok; tok = strtok(NULL, ":")) {
        snprintf(try, sizeof try, "%s/%s", tok, name);
        if (access(try, X_OK) == 0) {
          snprintf(r->str, sizeof r->str, "%s", try); r->ok=1; r->n=1; return 0;
        }
      }
    }
  }
  return -1;
}

int cubalc_host_which_bin(const char *name, cubalc_host_result *r) {
  r_clear(r);
  if (!name) return -1;
  if (cubalc_host_which_bin_inner(name, r) == 0)
    return 0;
  snprintf(r->err, sizeof r->err, "which_bin: not found %s", name);
  r->ok = 0;
  r->str[0] = 0;
  return -1;
}

int cubalc_host_which(const char *name, cubalc_host_result *r) {
  r_clear(r);
  if (!name) return -1;
  if (cubalc_host_which_bin_inner(name, r) == 0)
    return 0;
  /* Usability: fall back to CubalC lib/program resolve (SYS WHICH hold_seed). */
  if (cubalc_host_find_cubalc(name, r) == 0)
    return 0;
  snprintf(r->err, sizeof r->err, "which: not found %s", name);
  r->ok = 0;
  r->str[0] = 0;
  return -1;
}

