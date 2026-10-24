/* CubalC — hostops JSON dotted path plane peel (B_hostops_margin).
 * path_get · path_get_raw · path_has · path_obj (+ split/walk helpers).
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
#include <ctype.h>
#include <limits.h>
#include <errno.h>

static void r_clear(cubalc_host_result *r) {
  if (!r) return;
  memset(r, 0, sizeof *r);
}

/* --- dotted path helpers (GETP "freq.error" / SETP "a.b" …) --- */
#define CUBALC_JSON_PATH_MAXSEG 8

static int cubalc_json_path_split(const char *path, char segs[][96], int maxsegs) {
  const char *p;
  int n = 0;
  size_t i;
  if (!path || !path[0] || maxsegs < 1) return -1;
  p = path;
  while (*p && n < maxsegs) {
    while (*p == '.' || *p == '/') p++;
    if (!*p) break;
    i = 0;
    while (*p && *p != '.' && *p != '/' && i + 1 < 96)
      segs[n][i++] = *p++;
    segs[n][i] = 0;
    if (!segs[n][0]) return -1;
    n++;
    if (*p == '.' || *p == '/') p++;
  }
  if (*p) return -1; /* too many segments */
  return n;
}

static int cubalc_json_path_is_deep(const char *path) {
  if (!path) return 0;
  for (; *path; path++)
    if (*path == '.' || *path == '/') return 1;
  return 0;
}

/* Walk intermediates to parent object of leaf; leave parent in cur. n segs out. */
static int cubalc_json_path_walk_parent(const char *json, const char *path,
                                        char segs[][96], int *nsegs,
                                        char *cur, size_t curcap,
                                        cubalc_host_result *err) {
  cubalc_host_result gr;
  int n, i;
  const char *v;
  n = cubalc_json_path_split(path, segs, CUBALC_JSON_PATH_MAXSEG);
  if (n < 1) {
    if (err) snprintf(err->err, sizeof err->err, "jsonpath: bad path");
    return -1;
  }
  *nsegs = n;
  snprintf(cur, curcap, "%s", json && json[0] ? json : "{}");
  for (i = 0; i < n - 1; i++) {
    memset(&gr, 0, sizeof gr);
    if (cubalc_host_json_get_raw(cur, segs[i], &gr) != 0) {
      if (err) snprintf(err->err, sizeof err->err, "jsonpath: miss %s", segs[i]);
      return -1;
    }
    v = gr.str;
    while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
    if (*v != '{') {
      if (err) snprintf(err->err, sizeof err->err, "jsonpath: not object at %s", segs[i]);
      return -1;
    }
    if (v != gr.str) {
      size_t m = strlen(v);
      memmove(gr.str, v, m + 1);
    }
    snprintf(cur, curcap, "%s", gr.str);
  }
  return 0;
}

/* Peel path leaf as decoded scalar (string unquoted / number text). Soft miss -1. */
int cubalc_host_json_path_get(const char *json, const char *path, cubalc_host_result *r) {
  char segs[CUBALC_JSON_PATH_MAXSEG][96];
  char cur[CUBALC_HOST_STR_MAX];
  int n;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "jsonpath: empty path");
    return -1;
  }
  if (!cubalc_json_path_is_deep(path))
    return cubalc_host_json_get(json, path, r);
  if (cubalc_json_path_walk_parent(json, path, segs, &n, cur, sizeof cur, r) != 0)
    return -1;
  return cubalc_host_json_get(cur, segs[n - 1], r);
}

/* Raw leaf peel along path. Soft miss -1. */
int cubalc_host_json_path_get_raw(const char *json, const char *path,
                                  cubalc_host_result *r) {
  char segs[CUBALC_JSON_PATH_MAXSEG][96];
  char cur[CUBALC_HOST_STR_MAX];
  int n;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "jsonpath: empty path");
    return -1;
  }
  if (!cubalc_json_path_is_deep(path))
    return cubalc_host_json_get_raw(json, path, r);
  if (cubalc_json_path_walk_parent(json, path, segs, &n, cur, sizeof cur, r) != 0)
    return -1;
  return cubalc_host_json_get_raw(cur, segs[n - 1], r);
}

/* Soft presence along dotted path. r->n = 0|1. Always r->ok=1 when args valid. */
int cubalc_host_json_path_has(const char *json, const char *path, cubalc_host_result *r) {
  char segs[CUBALC_JSON_PATH_MAXSEG][96];
  char cur[CUBALC_HOST_STR_MAX];
  cubalc_host_result gr;
  int n, i;
  const char *v;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "jsonpath: empty path");
    return -1;
  }
  if (!cubalc_json_path_is_deep(path)) {
    if (cubalc_host_json_get_raw(json, path, &gr) == 0) {
      r->n = 1;
      r->ok = 1;
      snprintf(r->str, sizeof r->str, "%s", "1");
      return 0;
    }
    r->n = 0;
    r->ok = 1;
    snprintf(r->str, sizeof r->str, "%s", "0");
    return 0;
  }
  n = cubalc_json_path_split(path, segs, CUBALC_JSON_PATH_MAXSEG);
  if (n < 1) {
    snprintf(r->err, sizeof r->err, "jsonpath: bad path");
    return -1;
  }
  snprintf(cur, sizeof cur, "%s", json && json[0] ? json : "{}");
  for (i = 0; i < n - 1; i++) {
    memset(&gr, 0, sizeof gr);
    if (cubalc_host_json_get_raw(cur, segs[i], &gr) != 0) {
      r->n = 0;
      r->ok = 1;
      snprintf(r->str, sizeof r->str, "%s", "0");
      return 0;
    }
    v = gr.str;
    while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
    if (*v != '{') {
      r->n = 0;
      r->ok = 1;
      snprintf(r->str, sizeof r->str, "%s", "0");
      return 0;
    }
    if (v != gr.str) {
      size_t m = strlen(v);
      memmove(gr.str, v, m + 1);
    }
    snprintf(cur, sizeof cur, "%s", gr.str);
  }
  memset(&gr, 0, sizeof gr);
  if (cubalc_host_json_get_raw(cur, segs[n - 1], &gr) == 0) {
    r->n = 1;
    r->ok = 1;
    snprintf(r->str, sizeof r->str, "%s", "1");
  } else {
    r->n = 0;
    r->ok = 1;
    snprintf(r->str, sizeof r->str, "%s", "0");
  }
  return 0;
}

/* Peel object at path (or whole plate) for KEYSP/LENP/VALSP nest probes.
 * Soft miss/non-object → "{}" n=0. path empty → root object (or "{}"). */
int cubalc_host_json_path_obj(const char *json, const char *path, cubalc_host_result *r) {
  cubalc_host_result gr;
  const char *v, *j;
  r_clear(r);
  j = json ? json : "";
  while (*j == ' ' || *j == '\t' || *j == '\n' || *j == '\r') j++;
  if (!path || !path[0]) {
    if (*j == '{') {
      snprintf(r->str, sizeof r->str, "%s", j);
      /* count is not required for callers that re-run keys/len; n=1 means object hit */
      r->n = 1;
      r->ok = 1;
      return 0;
    }
    snprintf(r->str, sizeof r->str, "%s", "{}");
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  memset(&gr, 0, sizeof gr);
  if (cubalc_host_json_path_get_raw(j && *j ? j : "{}", path, &gr) != 0) {
    snprintf(r->str, sizeof r->str, "%s", "{}");
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  v = gr.str;
  while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
  if (*v != '{') {
    snprintf(r->str, sizeof r->str, "%s", "{}");
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  if (v != gr.str) {
    size_t m = strlen(v);
    memmove(gr.str, v, m + 1);
  }
  snprintf(r->str, sizeof r->str, "%s", gr.str);
  r->n = 1;
  r->ok = 1;
  return 0;
}

