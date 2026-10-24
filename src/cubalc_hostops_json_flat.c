/* CubalC — hostops JSON leaf flat plane peel (B_hostops_margin).
 * leaf_paths · leaf_kv · unflat_kv (+ leaf_walk helpers).
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

static void cubalc_bag_push(char *out, size_t outsz, size_t *olen, long *kept,
                            const char *path) {
  size_t pl;
  if (!out || !olen || !kept || !path || !path[0]) return;
  pl = strlen(path);
  if (*kept > 0 && *olen + 1 < outsz) out[(*olen)++] = '\n';
  if (*olen + pl < outsz) {
    memcpy(out + *olen, path, pl);
    *olen += pl;
  }
  out[*olen] = 0;
  (*kept)++;
}

/* DFS walk: emit dotted leaf paths under obj with optional prefix. */
static void cubalc_json_leaf_walk(const char *obj, const char *pfx,
                                  char *out, size_t outcap, size_t *olen,
                                  long *kept, int depth) {
  cubalc_host_result keys, raw;
  const char *p, *v;
  char key[256], child[512];

  if (!obj || !out || !olen || !kept || depth > 8) return;
  memset(&keys, 0, sizeof keys);
  if (cubalc_host_json_keys(obj, &keys) != 0) return;
  if (keys.n == 0) {
    /* empty object terminal: emit path when nested under a key */
    if (pfx && pfx[0])
      cubalc_bag_push(out, outcap, olen, kept, pfx);
    return;
  }
  p = keys.str;
  while (*p) {
    size_t kn = 0;
    while (p[kn] && p[kn] != '\n' && p[kn] != '\r') kn++;
    if (kn >= sizeof key) kn = sizeof key - 1;
    if (kn > 0) {
      memcpy(key, p, kn);
      key[kn] = 0;
      memset(&raw, 0, sizeof raw);
      if (cubalc_host_json_get_raw(obj, key, &raw) == 0) {
        if (pfx && pfx[0])
          snprintf(child, sizeof child, "%s.%s", pfx, key);
        else
          snprintf(child, sizeof child, "%s", key);
        v = raw.str;
        while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
        if (*v == '{') {
          cubalc_json_leaf_walk(v, child, out, outcap, olen, kept, depth + 1);
        } else {
          cubalc_bag_push(out, outcap, olen, kept, child);
        }
      }
    }
    p += kn;
    while (*p == '\n' || *p == '\r') p++;
  }
}

/* Recursively collect dotted leaf paths · optional nest path root.
 * Paths relative to nest root. Soft always OK. See header. */
int cubalc_host_json_leaf_paths(const char *json, const char *path,
                                cubalc_host_result *r) {
  cubalc_host_result obj;
  size_t olen = 0;
  long kept = 0;
  r_clear(r);
  r->str[0] = 0;
  memset(&obj, 0, sizeof obj);
  if (cubalc_host_json_path_obj(json ? json : "{}", path, &obj) != 0) {
    r->ok = 1;
    r->n = 0;
    return 0;
  }
  cubalc_json_leaf_walk(obj.str, "", r->str, sizeof r->str, &olen, &kept, 0);
  r->n = kept;
  r->ok = 1;
  return 0;
}

/* Decode a leaf raw JSON value into valbuf (string unquoted; tokens as-is). */
static void cubalc_json_leaf_valtext(const char *obj, const char *key,
                                     const char *rawv, char *valbuf, size_t vcap,
                                     const char **valtext_out) {
  cubalc_host_result gr;
  const char *v = rawv ? rawv : "";
  while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
  valbuf[0] = 0;
  *valtext_out = "";
  if (*v == '"') {
    memset(&gr, 0, sizeof gr);
    if (cubalc_host_json_get(obj, key, &gr) == 0) {
      *valtext_out = gr.str;
      /* copy into valbuf so gr can go out of scope for caller — caller must use
       * immediately; copy for safety */
      snprintf(valbuf, vcap, "%s", gr.str);
      *valtext_out = valbuf;
    } else {
      size_t rl = strlen(v);
      if (rl >= 2 && v[0] == '"' && v[rl - 1] == '"') {
        size_t n = rl - 2;
        if (n >= vcap) n = vcap - 1;
        memcpy(valbuf, v + 1, n);
        valbuf[n] = 0;
        *valtext_out = valbuf;
      } else {
        snprintf(valbuf, vcap, "%s", v);
        *valtext_out = valbuf;
      }
    }
  } else if (strncmp(v, "true", 4) == 0 &&
             (v[4] == 0 || v[4] == ',' || v[4] == '}' || v[4] == ' ' ||
              v[4] == '\n' || v[4] == '\r' || v[4] == '\t')) {
    *valtext_out = "true";
  } else if (strncmp(v, "false", 5) == 0 &&
             (v[5] == 0 || v[5] == ',' || v[5] == '}' || v[5] == ' ' ||
              v[5] == '\n' || v[5] == '\r' || v[5] == '\t')) {
    *valtext_out = "false";
  } else if (strncmp(v, "null", 4) == 0 &&
             (v[4] == 0 || v[4] == ',' || v[4] == '}' || v[4] == ' ' ||
              v[4] == '\n' || v[4] == '\r' || v[4] == '\t')) {
    *valtext_out = "null";
  } else {
    size_t i = 0;
    while (v[i] && v[i] != '\n' && v[i] != '\r' && i + 1 < vcap) {
      valbuf[i] = v[i];
      i++;
    }
    valbuf[i] = 0;
    *valtext_out = valbuf;
  }
}

/* DFS: emit path:val lines under obj. */
static void cubalc_json_leaf_kv_walk(const char *obj, const char *pfx,
                                    char *out, size_t outcap, size_t *olen,
                                    long *kept, int depth) {
  cubalc_host_result keys, raw;
  const char *p, *v, *valtext;
  char key[256], child[512], linebuf[CUBALC_HOST_STR_MAX], valbuf[CUBALC_HOST_STR_MAX];

  if (!obj || !out || !olen || !kept || depth > 8) return;
  memset(&keys, 0, sizeof keys);
  if (cubalc_host_json_keys(obj, &keys) != 0) return;
  if (keys.n == 0) {
    if (pfx && pfx[0]) {
      if (snprintf(linebuf, sizeof linebuf, "%s:", pfx) >= 0)
        cubalc_bag_push(out, outcap, olen, kept, linebuf);
    }
    return;
  }
  p = keys.str;
  while (*p) {
    size_t kn = 0;
    while (p[kn] && p[kn] != '\n' && p[kn] != '\r') kn++;
    if (kn >= sizeof key) kn = sizeof key - 1;
    if (kn > 0) {
      memcpy(key, p, kn);
      key[kn] = 0;
      memset(&raw, 0, sizeof raw);
      if (cubalc_host_json_get_raw(obj, key, &raw) == 0) {
        if (pfx && pfx[0])
          snprintf(child, sizeof child, "%s.%s", pfx, key);
        else
          snprintf(child, sizeof child, "%s", key);
        v = raw.str;
        while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
        if (*v == '{') {
          cubalc_json_leaf_kv_walk(v, child, out, outcap, olen, kept, depth + 1);
        } else {
          cubalc_json_leaf_valtext(obj, key, raw.str, valbuf, sizeof valbuf, &valtext);
          if (snprintf(linebuf, sizeof linebuf, "%s:%s", child,
                       valtext ? valtext : "") >= 0)
            cubalc_bag_push(out, outcap, olen, kept, linebuf);
        }
      }
    }
    p += kn;
    while (*p == '\n' || *p == '\r') p++;
  }
}

/* Recursive path:value bag · optional nest path root. Soft always OK. */
int cubalc_host_json_leaf_kv(const char *json, const char *path,
                             cubalc_host_result *r) {
  cubalc_host_result obj;
  size_t olen = 0;
  long kept = 0;
  r_clear(r);
  r->str[0] = 0;
  memset(&obj, 0, sizeof obj);
  if (cubalc_host_json_path_obj(json ? json : "{}", path, &obj) != 0) {
    r->ok = 1;
    r->n = 0;
    return 0;
  }
  cubalc_json_leaf_kv_walk(obj.str, "", r->str, sizeof r->str, &olen, &kept, 0);
  r->n = kept;
  r->ok = 1;
  return 0;
}

/* Apply path:value bag onto base (optional under prefix). See header. */
int cubalc_host_json_unflat_kv(const char *base, const char *bag, const char *under,
                               cubalc_host_result *r) {
  char cur[CUBALC_HOST_STR_MAX];
  char full[640];
  const char *p, *line, *j;
  long applied = 0;
  cubalc_host_result hr;
  r_clear(r);
  j = base ? base : "";
  while (*j == ' ' || *j == '\t' || *j == '\n' || *j == '\r') j++;
  if (*j == '{')
    snprintf(cur, sizeof cur, "%s", j);
  else
    snprintf(cur, sizeof cur, "%s", "{}");
  if (!bag || !bag[0]) {
    snprintf(r->str, sizeof r->str, "%s", cur);
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  p = bag;
  while (*p) {
    char key[512], val[CUBALC_HOST_STR_MAX];
    size_t kn = 0, vn = 0;
    const char *sep, *eq, *col, *vstart, *vend;
    int val_kind = 0;
    char *end = NULL;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    eq = NULL;
    col = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (!eq && *s == '=') eq = s;
        if (!col && *s == ':') col = s;
        s++;
      }
    }
    if (eq && col)
      sep = (eq < col) ? eq : col;
    else if (eq)
      sep = eq;
    else if (col)
      sep = col;
    else
      continue;
    kn = (size_t)(sep - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t skip = 0;
      while (skip < kn && (line[skip] == ' ' || line[skip] == '\t')) skip++;
      if (skip > 0) {
        line += skip;
        kn -= skip;
      }
    }
    if (kn == 0 || kn >= sizeof key) continue;
    memcpy(key, line, kn);
    key[kn] = 0;
    vstart = sep + 1;
    while (vstart < p && (*vstart == ' ' || *vstart == '\t')) vstart++;
    vend = p;
    while (vend > vstart && (vend[-1] == ' ' || vend[-1] == '\t')) vend--;
    vn = (size_t)(vend - vstart);
    if (vn >= sizeof val) vn = sizeof val - 1;
    memcpy(val, vstart, vn);
    val[vn] = 0;
    if (under && under[0])
      snprintf(full, sizeof full, "%s.%s", under, key);
    else
      snprintf(full, sizeof full, "%s", key);
    /* empty value → raw {} (leaf_kv empty-object terminal) */
    if (!val[0]) {
      snprintf(val, sizeof val, "%s", "{}");
      val_kind = 1;
    } else if (strcmp(val, "true") == 0 || strcmp(val, "false") == 0 ||
               strcmp(val, "null") == 0) {
      val_kind = 1;
    } else if (val[0] == '{' || val[0] == '[') {
      val_kind = 1; /* auto-raw JSON-shaped */
    } else {
      (void)strtol(val, &end, 10);
      if (end && end != val && *end == 0)
        val_kind = 1;
      else {
        char *ed = NULL;
        (void)strtod(val, &ed);
        if (ed && ed != val && *ed == 0)
          val_kind = 1;
        else
          val_kind = 0;
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_path_set(cur, full, val, val_kind, &hr) != 0) {
      snprintf(r->err, sizeof r->err, "%s",
               hr.err[0] ? hr.err : "unflat: path set fail");
      return -1;
    }
    snprintf(cur, sizeof cur, "%s", hr.str);
    applied++;
  }
  snprintf(r->str, sizeof r->str, "%s", cur);
  r->n = applied;
  r->ok = 1;
  return 0;
}

