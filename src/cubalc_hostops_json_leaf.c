/* CubalC — hostops JSON leaf path plane (peeled from json_delta).
 * Cube is SoT. Zero bloat.
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

static int cubalc_json_skip_value(const char **pp) {
  const char *p;
  int depth;
  if (!pp || !*pp) return -1;
  p = *pp;
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
  if (*p == '"') {
    p++;
    while (*p) {
      if (*p == '\\' && p[1]) {
        p += 2;
        continue;
      }
      if (*p == '"') {
        p++;
        break;
      }
      p++;
    }
    *pp = p;
    return 0;
  }
  if (*p == '{' || *p == '[') {
    char open = *p, close = (*p == '{') ? '}' : ']';
    depth = 1;
    p++;
    while (*p && depth > 0) {
      if (*p == '"') {
        p++;
        while (*p) {
          if (*p == '\\' && p[1]) {
            p += 2;
            continue;
          }
          if (*p == '"') {
            p++;
            break;
          }
          p++;
        }
        continue;
      }
      if (*p == open) depth++;
      else if (*p == close) depth--;
      p++;
    }
    *pp = p;
    return 0;
  }
  if (strncmp(p, "true", 4) == 0) {
    *pp = p + 4;
    return 0;
  }
  if (strncmp(p, "false", 5) == 0) {
    *pp = p + 5;
    return 0;
  }
  if (strncmp(p, "null", 4) == 0) {
    *pp = p + 4;
    return 0;
  }
  if (*p == '-' || (*p >= '0' && *p <= '9')) {
    if (*p == '-') p++;
    while (*p && ((*p >= '0' && *p <= '9') || *p == '.' || *p == 'e' || *p == 'E' ||
                  *p == '+' || *p == '-'))
      p++;
    *pp = p;
    return 0;
  }
  return -1;
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

static int cubalc_json_path_is_deep(const char *path) {
  if (!path) return 0;
  for (; *path; path++)
    if (*path == '.' || *path == '/') return 1;
  return 0;
}

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

static int json_raw_pure_int(const char *raw, long *out) {
  const char *v = raw ? raw : "";
  char *end = NULL;
  long num = 0;
  while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
  if (*v == '"') return 0;
  if (strncmp(v, "true", 4) == 0 &&
      (v[4] == 0 || v[4] == ',' || v[4] == '}' || v[4] == ' ' ||
       v[4] == '\n' || v[4] == '\r' || v[4] == '\t')) {
    if (out) *out = 1;
    return 1;
  }
  if (strncmp(v, "false", 5) == 0 &&
      (v[5] == 0 || v[5] == ',' || v[5] == '}' || v[5] == ' ' ||
       v[5] == '\n' || v[5] == '\r' || v[5] == '\t')) {
    if (out) *out = 0;
    return 1;
  }
  if (strncmp(v, "null", 4) == 0 || *v == '{' || *v == '[')
    return 0;
  num = strtol(v, &end, 10);
  if (!(end && end != v && *end == 0))
    return 0;
  if (out) *out = num;
  return 1;
}

static void ascii_lower_cpy(char *dst, size_t dlen, const char *src) {
  size_t i = 0;
  if (!dst || dlen == 0) return;
  if (!src) { dst[0] = 0; return; }
  while (src[i] && i + 1 < dlen) {
    char c = src[i];
    dst[i] = (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
    i++;
  }
  dst[i] = 0;
}
int cubalc_host_json_leaf_first_uneq(const char *json, const char *needle,
                                     cubalc_host_result *r) {
  cubalc_host_result paths, vals;
  const char *pp, *pv, *ps, *vs;
  char ref[256], path[512], val[CUBALC_HOST_STR_MAX / 4];
  long idx = 0, total = 0;
  size_t n;

  r_clear(r);
  r->str[0] = 0;
  r->err[0] = 0;
  r->n = 0;
  r->code = 0;
  r->ok = 1;
  if (!needle) needle = "";
  memset(&paths, 0, sizeof paths);
  memset(&vals, 0, sizeof vals);
  cubalc_host_json_leaf_match_bag(json ? json : "{}", needle, 0, &paths);
  cubalc_host_json_leaf_match_bag(json ? json : "{}", needle, 1, &vals);
  total = paths.n;
  if (total <= 1) {
    /* 0 or 1 match → no possible diverge */
    snprintf(r->err, sizeof r->err, "%ld", total);
    return 0;
  }
  /* first value = ref */
  pv = vals.str;
  vs = pv;
  while (*pv && *pv != '\n' && *pv != '\r') pv++;
  n = (size_t)(pv - vs);
  if (n >= sizeof ref) n = sizeof ref - 1;
  memcpy(ref, vs, n);
  ref[n] = 0;
  while (*pv == '\n' || *pv == '\r') pv++;
  pp = paths.str;
  /* skip first path */
  while (*pp && *pp != '\n' && *pp != '\r') pp++;
  while (*pp == '\n' || *pp == '\r') pp++;
  idx = 1;
  while (*pp && *pv) {
    ps = pp;
    while (*pp && *pp != '\n' && *pp != '\r') pp++;
    n = (size_t)(pp - ps);
    if (n >= sizeof path) n = sizeof path - 1;
    memcpy(path, ps, n);
    path[n] = 0;
    while (*pp == '\n' || *pp == '\r') pp++;
    vs = pv;
    while (*pv && *pv != '\n' && *pv != '\r') pv++;
    n = (size_t)(pv - vs);
    if (n >= sizeof val) n = sizeof val - 1;
    memcpy(val, vs, n);
    val[n] = 0;
    while (*pv == '\n' || *pv == '\r') pv++;
    if (strcmp(val, ref) != 0) {
      snprintf(r->str, sizeof r->str, "%s", path);
      snprintf(r->err, sizeof r->err, "%s\n%s\n%ld", ref, val, total);
      r->n = idx;
      r->code = 1;
      r->ok = 1;
      return 0;
    }
    idx++;
  }
  /* all equal */
  snprintf(r->err, sizeof r->err, "%ld", total);
  r->code = 0;
  r->n = 0;
  r->ok = 1;
  return 0;
}

/* UNEQPATHS: bag of matching leaf paths whose value differs from the first match.
 * r->str = path bag · r->n = diverge count · r->code = total matches · r->err = ref. */
int cubalc_host_json_leaf_uneq_paths(const char *json, const char *needle,
                                     cubalc_host_result *r) {
  cubalc_host_result paths, vals;
  const char *pp, *pv, *ps, *vs;
  char ref[256], path[512], val[CUBALC_HOST_STR_MAX / 4];
  long total = 0, ndiv = 0;
  size_t n, olen = 0;

  r_clear(r);
  r->str[0] = 0;
  r->err[0] = 0;
  r->n = 0;
  r->code = 0;
  r->ok = 1;
  if (!needle) needle = "";
  memset(&paths, 0, sizeof paths);
  memset(&vals, 0, sizeof vals);
  cubalc_host_json_leaf_match_bag(json ? json : "{}", needle, 0, &paths);
  cubalc_host_json_leaf_match_bag(json ? json : "{}", needle, 1, &vals);
  total = paths.n;
  r->code = (int)total;
  if (total <= 1) {
    r->err[0] = 0;
    return 0;
  }
  /* first value = ref; skip first path */
  pv = vals.str;
  vs = pv;
  while (*pv && *pv != '\n' && *pv != '\r') pv++;
  n = (size_t)(pv - vs);
  if (n >= sizeof ref) n = sizeof ref - 1;
  memcpy(ref, vs, n);
  ref[n] = 0;
  while (*pv == '\n' || *pv == '\r') pv++;
  pp = paths.str;
  while (*pp && *pp != '\n' && *pp != '\r') pp++;
  while (*pp == '\n' || *pp == '\r') pp++;
  snprintf(r->err, sizeof r->err, "%s", ref);
  while (*pp && *pv) {
    ps = pp;
    while (*pp && *pp != '\n' && *pp != '\r') pp++;
    n = (size_t)(pp - ps);
    if (n >= sizeof path) n = sizeof path - 1;
    memcpy(path, ps, n);
    path[n] = 0;
    while (*pp == '\n' || *pp == '\r') pp++;
    vs = pv;
    while (*pv && *pv != '\n' && *pv != '\r') pv++;
    n = (size_t)(pv - vs);
    if (n >= sizeof val) n = sizeof val - 1;
    memcpy(val, vs, n);
    val[n] = 0;
    while (*pv == '\n' || *pv == '\r') pv++;
    if (strcmp(val, ref) != 0)
      cubalc_bag_push(r->str, sizeof r->str, &olen, &ndiv, path);
  }
  r->n = ndiv;
  r->ok = 1;
  return 0;
}

/* SETBYVAL: rewrite leaf values equal to oldv → newv. See header. */
int cubalc_host_json_leaf_set_by_val(const char *json, const char *oldv, const char *newv,
                                     cubalc_host_result *r) {
  cubalc_host_result kv, wr;
  const char *p, *line;
  char bag[CUBALC_HOST_STR_MAX];
  size_t olen = 0;
  long updated = 0, total = 0;
  const char *want, *nv;

  r_clear(r);
  want = oldv ? oldv : "";
  nv = newv ? newv : "";
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  bag[0] = 0;
  p = kv.str;
  while (*p) {
    char path[512], oldleaf[CUBALC_HOST_STR_MAX / 2], linebuf[CUBALC_HOST_STR_MAX];
    size_t kn = 0, vn = 0;
    const char *sep, *vs, *ve;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    vs = sep ? sep + 1 : p;
    ve = p;
    while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
    while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    vn = (size_t)(ve - vs);
    if (vn >= sizeof oldleaf) vn = sizeof oldleaf - 1;
    memcpy(oldleaf, vs, vn);
    oldleaf[vn] = 0;
    if (strcmp(oldleaf, want) == 0) {
      if (snprintf(linebuf, sizeof linebuf, "%s:%s", path, nv) >= 0)
        cubalc_bag_push(bag, sizeof bag, &olen, &total, linebuf);
      updated++;
    } else {
      if (snprintf(linebuf, sizeof linebuf, "%s:%s", path, oldleaf) >= 0)
        cubalc_bag_push(bag, sizeof bag, &olen, &total, linebuf);
    }
  }
  memset(&wr, 0, sizeof wr);
  if (cubalc_host_json_unflat_kv("{}", bag, NULL, &wr) != 0) {
    snprintf(r->err, sizeof r->err, "%s",
             wr.err[0] ? wr.err : "setbyval: unflat fail");
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", wr.str);
  r->n = updated;
  r->ok = 1;
  r->code = total;
  return 0;
}

/* DELBYVAL: drop leaves whose value equals needle. See header. */
int cubalc_host_json_leaf_del_by_val(const char *json, const char *val,
                                     cubalc_host_result *r) {
  cubalc_host_result kv, wr;
  const char *p, *line;
  char bag[CUBALC_HOST_STR_MAX];
  size_t olen = 0;
  long deleted = 0, kept = 0;
  const char *want;

  r_clear(r);
  want = val ? val : "";
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  bag[0] = 0;
  p = kv.str;
  while (*p) {
    char path[512], oldleaf[CUBALC_HOST_STR_MAX / 2], linebuf[CUBALC_HOST_STR_MAX];
    size_t kn = 0, vn = 0;
    const char *sep, *vs, *ve;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    vs = sep ? sep + 1 : p;
    ve = p;
    while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
    while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    vn = (size_t)(ve - vs);
    if (vn >= sizeof oldleaf) vn = sizeof oldleaf - 1;
    memcpy(oldleaf, vs, vn);
    oldleaf[vn] = 0;
    if (strcmp(oldleaf, want) == 0) {
      deleted++;
      continue; /* drop this leaf */
    }
    if (snprintf(linebuf, sizeof linebuf, "%s:%s", path, oldleaf) >= 0)
      cubalc_bag_push(bag, sizeof bag, &olen, &kept, linebuf);
  }
  memset(&wr, 0, sizeof wr);
  if (cubalc_host_json_unflat_kv("{}", bag, NULL, &wr) != 0) {
    snprintf(r->err, sizeof r->err, "%s",
             wr.err[0] ? wr.err : "delbyval: unflat fail");
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", wr.str);
  r->n = deleted;
  r->ok = 1;
  r->code = kept;
  if (want[0])
    snprintf(r->err, sizeof r->err, "%s", want);
  return 0;
}

/* leaf_uniq · leaf_all_eq peel (G_hostops_margin) */
/* UNIQFLAT: unique matching leaf values by path needle. See header. */
int cubalc_host_json_leaf_uniq(const char *json, const char *needle,
                               cubalc_host_result *r) {
  cubalc_host_result vals;
  char keys[64][128];
  int nk = 0, k;
  const char *p, *start;
  size_t flen, olen = 0;
  long total = 0;

  r_clear(r);
  r->str[0] = 0;
  r->err[0] = 0;
  r->n = 0;
  r->code = 0;
  r->ok = 1;
  if (!needle) needle = "";
  memset(&vals, 0, sizeof vals);
  cubalc_host_json_leaf_match_bag(json ? json : "{}", needle, 1, &vals);
  if (vals.str[0]) {
    p = vals.str;
    while (*p) {
      char field[128];
      size_t take;
      start = p;
      while (*p && *p != '\n' && *p != '\r') p++;
      flen = (size_t)(p - start);
      take = flen;
      if (take >= sizeof field) take = sizeof field - 1;
      memcpy(field, start, take);
      field[take] = 0;
      total++;
      for (k = 0; k < nk; k++) {
        if (strcmp(keys[k], field) == 0)
          break;
      }
      if (k == nk && nk < 64) {
        snprintf(keys[nk], sizeof keys[0], "%s", field);
        nk++;
      }
      while (*p == '\n' || *p == '\r') p++;
    }
  }
  for (k = 0; k < nk; k++) {
    size_t n = strlen(keys[k]);
    if (k > 0 && olen + 1 < sizeof r->str) r->str[olen++] = '\n';
    if (olen + n < sizeof r->str) {
      memcpy(r->str + olen, keys[k], n);
      olen += n;
    } else if (olen < sizeof r->str - 1) {
      size_t t = sizeof r->str - 1 - olen;
      memcpy(r->str + olen, keys[k], t);
      olen += t;
    }
    r->str[olen] = 0;
  }
  r->n = nk;
  r->code = (int)total;
  r->ok = 1;
  snprintf(r->err, sizeof r->err, "%ld", total);
  return 0;
}

/* ALLEQFLAT: all matching leaf values identical? See header. */
int cubalc_host_json_leaf_all_eq(const char *json, const char *needle,
                                 cubalc_host_result *r) {
  cubalc_host_result uq;

  r_clear(r);
  r->str[0] = 0;
  r->err[0] = 0;
  r->n = 0;
  r->code = 0;
  r->ok = 1;
  memset(&uq, 0, sizeof uq);
  cubalc_host_json_leaf_uniq(json, needle, &uq);
  /* uq.n = unique count · uq.code = total matching leaves · uq.str = bag */
  r->n = uq.code; /* match count in n for agents */
  if (uq.code == 0 || uq.n == 0) {
    r->code = 0;
    r->str[0] = 0;
    snprintf(r->err, sizeof r->err, "0");
    return 0;
  }
  if (uq.n == 1) {
    /* single unique value = all equal */
    snprintf(r->str, sizeof r->str, "%s", uq.str);
    r->code = 1;
    snprintf(r->err, sizeof r->err, "1");
  } else {
    r->str[0] = 0;
    r->code = 0;
    snprintf(r->err, sizeof r->err, "%ld", uq.n);
  }
  r->ok = 1;
  return 0;
}


/* leaf_path/paths/count_by_val peel (G_hostops_margin) */
/* PATHBYVAL: first leaf path whose value equals needle. See header. */
int cubalc_host_json_leaf_path_by_val(const char *json, const char *val,
                                      cubalc_host_result *r) {
  cubalc_host_result kv;
  const char *p, *line;
  const char *want;

  r_clear(r);
  r->str[0] = 0;
  r->err[0] = 0;
  r->n = 0;
  r->code = 0;
  r->ok = 1;
  want = val ? val : "";
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  p = kv.str;
  while (*p) {
    char path[512], oldv[CUBALC_HOST_STR_MAX / 2];
    size_t kn = 0, vn = 0;
    const char *sep, *vs, *ve;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    vs = sep ? sep + 1 : p;
    ve = p;
    while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
    while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    vn = (size_t)(ve - vs);
    if (vn >= sizeof oldv) vn = sizeof oldv - 1;
    memcpy(oldv, vs, vn);
    oldv[vn] = 0;
    if (strcmp(oldv, want) == 0) {
      snprintf(r->str, sizeof r->str, "%s", path);
      snprintf(r->err, sizeof r->err, "%s", oldv);
      r->n = 1;
      r->code = 1;
      r->ok = 1;
      return 0;
    }
  }
  r->str[0] = 0;
  r->err[0] = 0;
  r->n = 0;
  r->code = 0;
  r->ok = 1;
  return 0;
}

/* PATHSBYVAL: all leaf paths whose value equals needle. See header. */
int cubalc_host_json_leaf_paths_by_val(const char *json, const char *val,
                                       cubalc_host_result *r) {
  cubalc_host_result kv;
  const char *p, *line;
  const char *want;
  size_t olen = 0;
  long n = 0;

  r_clear(r);
  r->str[0] = 0;
  r->err[0] = 0;
  r->n = 0;
  r->code = 0;
  r->ok = 1;
  want = val ? val : "";
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  p = kv.str;
  while (*p) {
    char path[512], oldv[CUBALC_HOST_STR_MAX / 2];
    size_t kn = 0, vn = 0;
    const char *sep, *vs, *ve;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    vs = sep ? sep + 1 : p;
    ve = p;
    while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
    while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    vn = (size_t)(ve - vs);
    if (vn >= sizeof oldv) vn = sizeof oldv - 1;
    memcpy(oldv, vs, vn);
    oldv[vn] = 0;
    if (strcmp(oldv, want) == 0)
      cubalc_bag_push(r->str, sizeof r->str, &olen, &n, path);
  }
  r->n = n;
  r->code = (int)n;
  r->ok = 1;
  if (want[0])
    snprintf(r->err, sizeof r->err, "%s", want);
  return 0;
}

/* COUNTBYVAL/HASVAL: count leaves whose value equals needle. See header. */
int cubalc_host_json_leaf_count_by_val(const char *json, const char *val,
                                       cubalc_host_result *r) {
  cubalc_host_result paths;

  r_clear(r);
  memset(&paths, 0, sizeof paths);
  cubalc_host_json_leaf_paths_by_val(json, val, &paths);
  r->n = paths.n;
  r->code = paths.n > 0 ? 1 : 0;
  r->ok = 1;
  snprintf(r->str, sizeof r->str, "%ld", paths.n);
  if (val && val[0])
    snprintf(r->err, sizeof r->err, "%s", val);
  else
    r->err[0] = 0;
  return 0;
}

/* cubalc_host_json_leaf_set_by_val → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_del_by_val → json_delta peel (G_hostops_margin) */


/* TYPEFLAT: first matching leaf kind by path needle. See header. */
int cubalc_host_json_leaf_type(const char *json, const char *needle,
                               cubalc_host_result *r) {
  cubalc_host_result gr, raw;
  const char *v;
  const char *kind_s = "missing";
  long kind_n = 0;

  r_clear(r);
  memset(&gr, 0, sizeof gr);
  if (cubalc_host_json_leaf_get(json, needle, &gr) != 0) {
    snprintf(r->str, sizeof r->str, "%s", "missing");
    r->n = 0;
    r->code = 0;
    r->ok = 1;
    return 0;
  }
  if (gr.code == 0 || !gr.err[0]) {
    /* soft miss */
    snprintf(r->str, sizeof r->str, "%s", "missing");
    r->err[0] = 0;
    r->n = 0;
    r->code = 0;
    r->ok = 1;
    return 0;
  }
  /* classify raw JSON at path for accurate str vs num */
  memset(&raw, 0, sizeof raw);
  if (cubalc_host_json_path_get_raw(json ? json : "{}", gr.err, &raw) != 0) {
    snprintf(r->str, sizeof r->str, "%s", "missing");
    snprintf(r->err, sizeof r->err, "%s", gr.err);
    r->n = 0;
    r->code = 0;
    r->ok = 1;
    return 0;
  }
  v = raw.str;
  while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
  if (*v == '"') {
    kind_s = "str";
    kind_n = 2;
  } else if (*v == '{') {
    kind_s = "obj";
    kind_n = 5;
  } else if (*v == '[') {
    kind_s = "arr";
    kind_n = 6;
  } else if (strncmp(v, "true", 4) == 0 || strncmp(v, "false", 5) == 0) {
    kind_s = "bool";
    kind_n = 3;
  } else if (strncmp(v, "null", 4) == 0) {
    kind_s = "null";
    kind_n = 4;
  } else if (*v == '-' || (*v >= '0' && *v <= '9')) {
    kind_s = "num";
    kind_n = 1;
  } else {
    kind_s = "str";
    kind_n = 2;
  }
  snprintf(r->str, sizeof r->str, "%s", kind_s);
  snprintf(r->err, sizeof r->err, "%s", gr.err);
  r->n = kind_n;
  r->code = 1;
  r->ok = 1;
  return 0;
}

/* FREQFLAT: value frequency among leaves whose path contains needle. See header. */
int cubalc_host_json_leaf_freq(const char *json, const char *needle,
                               cubalc_host_result *r) {
  cubalc_host_result vals;
  char keys[64][128];
  long counts[64];
  int nk = 0, k;
  const char *p, *start;
  size_t flen, olen = 0;
  long total = 0;

  r_clear(r);
  r->str[0] = 0;
  r->n = 0;
  r->code = 0;
  r->ok = 1;
  if (!needle) needle = "";
  memset(&vals, 0, sizeof vals);
  cubalc_host_json_leaf_match_bag(json ? json : "{}", needle, 1, &vals);
  memset(counts, 0, sizeof counts);
  if (vals.str[0]) {
    p = vals.str;
    while (*p) {
      char field[128];
      size_t take;
      start = p;
      while (*p && *p != '\n' && *p != '\r') p++;
      flen = (size_t)(p - start);
      take = flen;
      if (take >= sizeof field) take = sizeof field - 1;
      memcpy(field, start, take);
      field[take] = 0;
      total++;
      for (k = 0; k < nk; k++) {
        if (strcmp(keys[k], field) == 0) {
          counts[k]++;
          break;
        }
      }
      if (k == nk && nk < 64) {
        snprintf(keys[nk], sizeof keys[0], "%s", field);
        counts[nk] = 1;
        nk++;
      }
      while (*p == '\n' || *p == '\r') p++;
    }
  }
  for (k = 0; k < nk; k++) {
    char line[160];
    int n;
    n = snprintf(line, sizeof line, "%s:%ld", keys[k], counts[k]);
    if (n < 0) n = 0;
    if ((size_t)n >= sizeof line) n = (int)sizeof line - 1;
    if (k > 0 && olen + 1 < sizeof r->str) r->str[olen++] = '\n';
    if (olen + (size_t)n < sizeof r->str) {
      memcpy(r->str + olen, line, (size_t)n);
      olen += (size_t)n;
    } else if (olen < sizeof r->str - 1) {
      size_t t = sizeof r->str - 1 - olen;
      memcpy(r->str + olen, line, t);
      olen += t;
    }
    r->str[olen] = 0;
  }
  r->n = nk;
  r->code = (int)total;
  r->ok = 1;
  snprintf(r->err, sizeof r->err, "%ld", total);
  return 0;
}

/* MODEFLAT: dominant matching leaf value by path needle. See header. */
int cubalc_host_json_leaf_mode(const char *json, const char *needle,
                               cubalc_host_result *r) {
  cubalc_host_result vals;
  char keys[64][128];
  long counts[64];
  int nk = 0, k, best = -1;
  const char *p, *start;
  size_t flen;
  long total = 0, best_c = 0;

  r_clear(r);
  r->str[0] = 0;
  r->err[0] = 0;
  r->n = 0;
  r->code = 0;
  r->ok = 1;
  if (!needle) needle = "";
  memset(&vals, 0, sizeof vals);
  cubalc_host_json_leaf_match_bag(json ? json : "{}", needle, 1, &vals);
  memset(counts, 0, sizeof counts);
  if (vals.str[0]) {
    p = vals.str;
    while (*p) {
      char field[128];
      size_t take;
      start = p;
      while (*p && *p != '\n' && *p != '\r') p++;
      flen = (size_t)(p - start);
      take = flen;
      if (take >= sizeof field) take = sizeof field - 1;
      memcpy(field, start, take);
      field[take] = 0;
      total++;
      for (k = 0; k < nk; k++) {
        if (strcmp(keys[k], field) == 0) {
          counts[k]++;
          break;
        }
      }
      if (k == nk && nk < 64) {
        snprintf(keys[nk], sizeof keys[0], "%s", field);
        counts[nk] = 1;
        nk++;
      }
      while (*p == '\n' || *p == '\r') p++;
    }
  }
  for (k = 0; k < nk; k++) {
    if (best < 0 || counts[k] > best_c) {
      best = k;
      best_c = counts[k];
    }
  }
  if (best >= 0) {
    snprintf(r->str, sizeof r->str, "%s", keys[best]);
    r->n = best_c;
    r->code = (int)total;
    snprintf(r->err, sizeof r->err, "%ld", best_c);
  } else {
    r->str[0] = 0;
    r->n = 0;
    r->code = 0;
    r->err[0] = 0;
  }
  r->ok = 1;
  return 0;
}

/* GETFLAT: first matching leaf value by path needle. See header. */
int cubalc_host_json_leaf_get(const char *json, const char *needle,
                              cubalc_host_result *r) {
  cubalc_host_result kv;
  const char *p, *line;
  size_t nn;

  r_clear(r);
  if (!needle) needle = "";
  nn = strlen(needle);
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  p = kv.str;
  while (*p) {
    char path[512], oldv[CUBALC_HOST_STR_MAX / 2];
    size_t kn = 0, vn = 0;
    const char *sep, *vs, *ve;
    int hit;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    vs = sep ? sep + 1 : p;
    ve = p;
    while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
    while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    vn = (size_t)(ve - vs);
    if (vn >= sizeof oldv) vn = sizeof oldv - 1;
    memcpy(oldv, vs, vn);
    oldv[vn] = 0;
    if (!nn)
      hit = 1;
    else {
      size_t i;
      hit = 0;
      if (nn <= kn) {
        for (i = 0; i + nn <= kn; i++) {
          if (memcmp(path + i, needle, nn) == 0) {
            hit = 1;
            break;
          }
        }
      }
    }
    if (!hit) continue;
    snprintf(r->str, sizeof r->str, "%s", oldv);
    snprintf(r->err, sizeof r->err, "%s", path); /* path side-channel on hit */
    r->n = 1;
    r->code = 1;
    r->ok = 1;
    return 0;
  }
  r->str[0] = 0;
  r->err[0] = 0;
  r->n = 0;
  r->code = 0;
  r->ok = 1;
  return 0;
}

/* LASTFLAT: last matching leaf value by path needle. See header. */
int cubalc_host_json_leaf_get_last(const char *json, const char *needle,
                                   cubalc_host_result *r) {
  cubalc_host_result kv;
  const char *p, *line;
  size_t nn;
  int found = 0;

  r_clear(r);
  if (!needle) needle = "";
  nn = strlen(needle);
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  p = kv.str;
  while (*p) {
    char path[512], oldv[CUBALC_HOST_STR_MAX / 2];
    size_t kn = 0, vn = 0;
    const char *sep, *vs, *ve;
    int hit;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    vs = sep ? sep + 1 : p;
    ve = p;
    while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
    while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    vn = (size_t)(ve - vs);
    if (vn >= sizeof oldv) vn = sizeof oldv - 1;
    memcpy(oldv, vs, vn);
    oldv[vn] = 0;
    if (!nn)
      hit = 1;
    else {
      size_t i;
      hit = 0;
      if (nn <= kn) {
        for (i = 0; i + nn <= kn; i++) {
          if (memcmp(path + i, needle, nn) == 0) {
            hit = 1;
            break;
          }
        }
      }
    }
    if (!hit) continue;
    snprintf(r->str, sizeof r->str, "%s", oldv);
    snprintf(r->err, sizeof r->err, "%s", path);
    found = 1;
  }
  if (found) {
    r->n = 1;
    r->code = 1;
    r->ok = 1;
    return 0;
  }
  r->str[0] = 0;
  r->err[0] = 0;
  r->n = 0;
  r->code = 0;
  r->ok = 1;
  return 0;
}

/* NTHFLAT: 0-based Nth matching leaf value by path needle. See header. */
int cubalc_host_json_leaf_get_nth(const char *json, const char *needle, long index,
                                  cubalc_host_result *r) {
  cubalc_host_result kv;
  const char *p, *line;
  size_t nn;
  long seen = 0;

  r_clear(r);
  if (!needle) needle = "";
  nn = strlen(needle);
  if (index < 0) {
    r->str[0] = 0;
    r->err[0] = 0;
    r->n = 0;
    r->code = 0;
    r->ok = 1;
    return 0;
  }
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  p = kv.str;
  while (*p) {
    char path[512], oldv[CUBALC_HOST_STR_MAX / 2];
    size_t kn = 0, vn = 0;
    const char *sep, *vs, *ve;
    int hit;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    vs = sep ? sep + 1 : p;
    ve = p;
    while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
    while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    vn = (size_t)(ve - vs);
    if (vn >= sizeof oldv) vn = sizeof oldv - 1;
    memcpy(oldv, vs, vn);
    oldv[vn] = 0;
    if (!nn)
      hit = 1;
    else {
      size_t i;
      hit = 0;
      if (nn <= kn) {
        for (i = 0; i + nn <= kn; i++) {
          if (memcmp(path + i, needle, nn) == 0) {
            hit = 1;
            break;
          }
        }
      }
    }
    if (!hit) continue;
    if (seen == index) {
      snprintf(r->str, sizeof r->str, "%s", oldv);
      snprintf(r->err, sizeof r->err, "%s", path);
      r->n = 1;
      r->code = 1;
      r->ok = 1;
      return 0;
    }
    seen++;
  }
  r->str[0] = 0;
  r->err[0] = 0;
  r->n = 0;
  r->code = 0;
  r->ok = 1;
  return 0;
}

/* GETFLATN: first pure-int leaf value by path needle. See header. */
int cubalc_host_json_leaf_getn(const char *json, const char *needle,
                               cubalc_host_result *r) {
  cubalc_host_result kv;
  const char *p, *line;
  size_t nn;

  r_clear(r);
  if (!needle) needle = "";
  nn = strlen(needle);
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  p = kv.str;
  while (*p) {
    char path[512], oldv[CUBALC_HOST_STR_MAX / 2];
    size_t kn = 0, vn = 0;
    const char *sep, *vs, *ve;
    int hit;
    char *end = NULL;
    long num;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    vs = sep ? sep + 1 : p;
    ve = p;
    while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
    while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    vn = (size_t)(ve - vs);
    if (vn >= sizeof oldv) vn = sizeof oldv - 1;
    memcpy(oldv, vs, vn);
    oldv[vn] = 0;
    if (!nn)
      hit = 1;
    else {
      size_t i;
      hit = 0;
      if (nn <= kn) {
        for (i = 0; i + nn <= kn; i++) {
          if (memcmp(path + i, needle, nn) == 0) {
            hit = 1;
            break;
          }
        }
      }
    }
    if (!hit) continue;
    end = NULL;
    num = strtol(oldv, &end, 10);
    if (!(end && end != oldv && *end == 0))
      continue; /* skip non-int leaves */
    r->n = num;
    r->code = 1;
    r->ok = 1;
    snprintf(r->str, sizeof r->str, "%ld", num);
    snprintf(r->err, sizeof r->err, "%s", path);
    return 0;
  }
  r->str[0] = 0;
  r->err[0] = 0;
  r->n = 0;
  r->code = 0;
  r->ok = 1;
  return 0;
}

/* LASTFLATN: last pure-int leaf value by path needle. See header. */
int cubalc_host_json_leaf_getn_last(const char *json, const char *needle,
                                    cubalc_host_result *r) {
  cubalc_host_result kv;
  const char *p, *line;
  size_t nn;
  int found = 0;
  long last_num = 0;
  char last_path[512];

  r_clear(r);
  if (!needle) needle = "";
  nn = strlen(needle);
  last_path[0] = 0;
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  p = kv.str;
  while (*p) {
    char path[512], oldv[CUBALC_HOST_STR_MAX / 2];
    size_t kn = 0, vn = 0;
    const char *sep, *vs, *ve;
    int hit;
    char *end = NULL;
    long num;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    vs = sep ? sep + 1 : p;
    ve = p;
    while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
    while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    vn = (size_t)(ve - vs);
    if (vn >= sizeof oldv) vn = sizeof oldv - 1;
    memcpy(oldv, vs, vn);
    oldv[vn] = 0;
    if (!nn)
      hit = 1;
    else {
      size_t i;
      hit = 0;
      if (nn <= kn) {
        for (i = 0; i + nn <= kn; i++) {
          if (memcmp(path + i, needle, nn) == 0) {
            hit = 1;
            break;
          }
        }
      }
    }
    if (!hit) continue;
    end = NULL;
    num = strtol(oldv, &end, 10);
    if (!(end && end != oldv && *end == 0))
      continue;
    last_num = num;
    snprintf(last_path, sizeof last_path, "%s", path);
    found = 1;
  }
  if (found) {
    r->n = last_num;
    r->code = 1;
    r->ok = 1;
    snprintf(r->str, sizeof r->str, "%ld", last_num);
    snprintf(r->err, sizeof r->err, "%s", last_path);
    return 0;
  }
  r->str[0] = 0;
  r->err[0] = 0;
  r->n = 0;
  r->code = 0;
  r->ok = 1;
  return 0;
}

/* NTHFLATN: 0-based Nth pure-int leaf by path needle. See header. */
int cubalc_host_json_leaf_getn_nth(const char *json, const char *needle, long index,
                                   cubalc_host_result *r) {
  cubalc_host_result kv;
  const char *p, *line;
  size_t nn;
  long seen = 0;

  r_clear(r);
  if (!needle) needle = "";
  nn = strlen(needle);
  if (index < 0) {
    r->str[0] = 0;
    r->err[0] = 0;
    r->n = 0;
    r->code = 0;
    r->ok = 1;
    return 0;
  }
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  p = kv.str;
  while (*p) {
    char path[512], oldv[CUBALC_HOST_STR_MAX / 2];
    size_t kn = 0, vn = 0;
    const char *sep, *vs, *ve;
    int hit;
    char *end = NULL;
    long num;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    vs = sep ? sep + 1 : p;
    ve = p;
    while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
    while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    vn = (size_t)(ve - vs);
    if (vn >= sizeof oldv) vn = sizeof oldv - 1;
    memcpy(oldv, vs, vn);
    oldv[vn] = 0;
    if (!nn)
      hit = 1;
    else {
      size_t i;
      hit = 0;
      if (nn <= kn) {
        for (i = 0; i + nn <= kn; i++) {
          if (memcmp(path + i, needle, nn) == 0) {
            hit = 1;
            break;
          }
        }
      }
    }
    if (!hit) continue;
    end = NULL;
    num = strtol(oldv, &end, 10);
    if (!(end && end != oldv && *end == 0))
      continue;
    if (seen == index) {
      r->n = num;
      r->code = 1;
      r->ok = 1;
      snprintf(r->str, sizeof r->str, "%ld", num);
      snprintf(r->err, sizeof r->err, "%s", path);
      return 0;
    }
    seen++;
  }
  r->str[0] = 0;
  r->err[0] = 0;
  r->n = 0;
  r->code = 0;
  r->ok = 1;
  return 0;
}

/* SETFLAT: set matching leaf values. See header. */
int cubalc_host_json_leaf_set(const char *json, const char *needle, const char *val,
                              cubalc_host_result *r) {
  cubalc_host_result kv, wr;
  const char *p, *line;
  char bag[CUBALC_HOST_STR_MAX];
  size_t olen = 0;
  long updated = 0, total = 0;
  size_t nn;

  r_clear(r);
  if (!needle) needle = "";
  if (!val) val = "";
  nn = strlen(needle);
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  bag[0] = 0;
  p = kv.str;
  while (*p) {
    char path[512], oldv[CUBALC_HOST_STR_MAX / 2], linebuf[CUBALC_HOST_STR_MAX];
    size_t kn = 0, vn = 0;
    const char *sep, *vs, *ve;
    int hit;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    vs = sep ? sep + 1 : p;
    ve = p;
    while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
    while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    vn = (size_t)(ve - vs);
    if (vn >= sizeof oldv) vn = sizeof oldv - 1;
    memcpy(oldv, vs, vn);
    oldv[vn] = 0;
    if (!nn)
      hit = 1;
    else {
      size_t i;
      hit = 0;
      if (nn <= kn) {
        for (i = 0; i + nn <= kn; i++) {
          if (memcmp(path + i, needle, nn) == 0) {
            hit = 1;
            break;
          }
        }
      }
    }
    if (hit) {
      if (snprintf(linebuf, sizeof linebuf, "%s:%s", path, val) >= 0)
        cubalc_bag_push(bag, sizeof bag, &olen, &total, linebuf);
      updated++;
    } else {
      if (snprintf(linebuf, sizeof linebuf, "%s:%s", path, oldv) >= 0)
        cubalc_bag_push(bag, sizeof bag, &olen, &total, linebuf);
    }
  }
  memset(&wr, 0, sizeof wr);
  if (cubalc_host_json_unflat_kv("{}", bag, NULL, &wr) != 0) {
    snprintf(r->err, sizeof r->err, "%s",
             wr.err[0] ? wr.err : "setflat: unflat fail");
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", wr.str);
  r->n = updated;
  r->ok = 1;
  r->code = total;
  return 0;
}

/* INCFLAT: bump pure-int leaves matching path needle. See header. */
int cubalc_host_json_leaf_inc(const char *json, const char *needle, long delta,
                              cubalc_host_result *r) {
  cubalc_host_result kv, wr;
  const char *p, *line;
  char bag[CUBALC_HOST_STR_MAX];
  size_t olen = 0;
  long bumped = 0, total = 0;
  size_t nn;

  r_clear(r);
  if (!needle) needle = "";
  nn = strlen(needle);
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  bag[0] = 0;
  p = kv.str;
  while (*p) {
    char path[512], oldv[CUBALC_HOST_STR_MAX / 2], linebuf[CUBALC_HOST_STR_MAX];
    size_t kn = 0, vn = 0;
    const char *sep, *vs, *ve;
    int hit;
    char *end = NULL;
    long num;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    vs = sep ? sep + 1 : p;
    ve = p;
    while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
    while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    vn = (size_t)(ve - vs);
    if (vn >= sizeof oldv) vn = sizeof oldv - 1;
    memcpy(oldv, vs, vn);
    oldv[vn] = 0;
    if (!nn)
      hit = 1;
    else {
      size_t i;
      hit = 0;
      if (nn <= kn) {
        for (i = 0; i + nn <= kn; i++) {
          if (memcmp(path + i, needle, nn) == 0) {
            hit = 1;
            break;
          }
        }
      }
    }
    if (hit) {
      end = NULL;
      num = strtol(oldv, &end, 10);
      if (end && end != oldv && *end == 0) {
        char nbuf[32];
        snprintf(nbuf, sizeof nbuf, "%ld", num + delta);
        if (snprintf(linebuf, sizeof linebuf, "%s:%s", path, nbuf) >= 0)
          cubalc_bag_push(bag, sizeof bag, &olen, &total, linebuf);
        bumped++;
        continue;
      }
    }
    if (snprintf(linebuf, sizeof linebuf, "%s:%s", path, oldv) >= 0)
      cubalc_bag_push(bag, sizeof bag, &olen, &total, linebuf);
  }
  memset(&wr, 0, sizeof wr);
  if (cubalc_host_json_unflat_kv("{}", bag, NULL, &wr) != 0) {
    snprintf(r->err, sizeof r->err, "%s",
             wr.err[0] ? wr.err : "incflat: unflat fail");
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", wr.str);
  r->n = bumped;
  r->ok = 1;
  r->code = total;
  return 0;
}

/* SUMFLAT: sum pure-int leaves matching path needle. See header. */
int cubalc_host_json_leaf_sum(const char *json, const char *needle,
                              cubalc_host_result *r) {
  cubalc_host_result kv;
  const char *p, *line;
  long sum = 0, count = 0;
  size_t nn;

  r_clear(r);
  if (!needle) needle = "";
  nn = strlen(needle);
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  p = kv.str;
  while (*p) {
    char path[512], oldv[CUBALC_HOST_STR_MAX / 2];
    size_t kn = 0, vn = 0;
    const char *sep, *vs, *ve;
    int hit;
    char *end = NULL;
    long num;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    vs = sep ? sep + 1 : p;
    ve = p;
    while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
    while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    vn = (size_t)(ve - vs);
    if (vn >= sizeof oldv) vn = sizeof oldv - 1;
    memcpy(oldv, vs, vn);
    oldv[vn] = 0;
    if (!nn)
      hit = 1;
    else {
      size_t i;
      hit = 0;
      if (nn <= kn) {
        for (i = 0; i + nn <= kn; i++) {
          if (memcmp(path + i, needle, nn) == 0) {
            hit = 1;
            break;
          }
        }
      }
    }
    if (!hit) continue;
    end = NULL;
    num = strtol(oldv, &end, 10);
    if (end && end != oldv && *end == 0) {
      sum += num;
      count++;
    }
  }
  r->n = sum;
  r->code = (int)count;
  r->ok = 1;
  snprintf(r->str, sizeof r->str, "%ld", sum);
  return 0;
}

/* AVGFLAT: integer mean of pure-int leaves by path needle. See header. */
int cubalc_host_json_leaf_avg(const char *json, const char *needle,
                              cubalc_host_result *r) {
  long sum;
  int count;
  long avg;

  if (cubalc_host_json_leaf_sum(json, needle, r) != 0)
    return -1;
  sum = r->n;
  count = r->code;
  avg = (count > 0) ? (sum / (long)count) : 0;
  r->n = avg;
  r->code = count;
  r->ok = 1;
  snprintf(r->str, sizeof r->str, "%ld", avg);
  snprintf(r->err, sizeof r->err, "%ld", sum); /* sum side-channel */
  return 0;
}


/* MEDIANFLAT: integer median of pure-int leaves by path needle. See header. */
int cubalc_host_json_leaf_median(const char *json, const char *needle,
                                 cubalc_host_result *r) {
  cubalc_host_result kv;
  const char *p, *line;
  long arr[256];
  int n = 0, i;
  size_t nn;

  r_clear(r);
  if (!needle) needle = "";
  nn = strlen(needle);
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  p = kv.str;
  while (*p && n < 256) {
    char path[512], oldv[CUBALC_HOST_STR_MAX / 2];
    size_t kn = 0, vn = 0;
    const char *sep, *vs, *ve;
    int hit;
    char *end = NULL;
    long num;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    vs = sep ? sep + 1 : p;
    ve = p;
    while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
    while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    vn = (size_t)(ve - vs);
    if (vn >= sizeof oldv) vn = sizeof oldv - 1;
    memcpy(oldv, vs, vn);
    oldv[vn] = 0;
    if (!nn)
      hit = 1;
    else {
      size_t j;
      hit = 0;
      if (nn <= kn) {
        for (j = 0; j + nn <= kn; j++) {
          if (memcmp(path + j, needle, nn) == 0) {
            hit = 1;
            break;
          }
        }
      }
    }
    if (!hit) continue;
    end = NULL;
    num = strtol(oldv, &end, 10);
    if (end && end != oldv && *end == 0)
      arr[n++] = num;
  }
  if (n > 0) {
    for (i = 1; i < n; i++) {
      long key = arr[i];
      int j = i - 1;
      while (j >= 0 && arr[j] > key) {
        arr[j + 1] = arr[j];
        j--;
      }
      arr[j + 1] = key;
    }
    r->n = arr[(n - 1) / 2];
  } else {
    r->n = 0;
  }
  r->code = n;
  r->ok = 1;
  snprintf(r->str, sizeof r->str, "%ld", r->n);
  return 0;
}

/* TOPPATHFLAT/BOTPATHFLAT: extreme pure-int leaf path. See header. */
int cubalc_host_json_leaf_toppath(const char *json, const char *needle, int want_min,
                                  cubalc_host_result *r) {
  cubalc_host_result kv;
  const char *p, *line;
  long best = 0, count = 0;
  int found = 0;
  char best_path[512];
  size_t nn;

  r_clear(r);
  best_path[0] = 0;
  if (!needle) needle = "";
  nn = strlen(needle);
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  p = kv.str;
  while (*p) {
    char path[512], oldv[CUBALC_HOST_STR_MAX / 2];
    size_t kn = 0, vn = 0;
    const char *sep, *vs, *ve;
    int hit;
    char *end = NULL;
    long num;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    vs = sep ? sep + 1 : p;
    ve = p;
    while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
    while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    vn = (size_t)(ve - vs);
    if (vn >= sizeof oldv) vn = sizeof oldv - 1;
    memcpy(oldv, vs, vn);
    oldv[vn] = 0;
    if (!nn)
      hit = 1;
    else {
      size_t i;
      hit = 0;
      if (nn <= kn) {
        for (i = 0; i + nn <= kn; i++) {
          if (memcmp(path + i, needle, nn) == 0) {
            hit = 1;
            break;
          }
        }
      }
    }
    if (!hit) continue;
    end = NULL;
    num = strtol(oldv, &end, 10);
    if (!(end && end != oldv && *end == 0))
      continue;
    count++;
    if (!found || (want_min ? (num < best) : (num > best))) {
      best = num;
      found = 1;
      snprintf(best_path, sizeof best_path, "%s", path);
    }
  }
  r->n = found ? best : 0;
  r->code = (int)count;
  r->ok = 1;
  snprintf(r->str, sizeof r->str, "%s", best_path);
  return 0;
}

/* CAPFLAT: clamp pure-int leaves to max. See header. */
int cubalc_host_json_leaf_cap(const char *json, const char *needle, long maxv,
                              cubalc_host_result *r) {
  cubalc_host_result kv, wr;
  const char *p, *line;
  char bag[CUBALC_HOST_STR_MAX];
  size_t olen = 0;
  long capped = 0, considered = 0, total = 0;
  size_t nn;

  r_clear(r);
  if (!needle) needle = "";
  nn = strlen(needle);
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  bag[0] = 0;
  p = kv.str;
  while (*p) {
    char path[512], oldv[CUBALC_HOST_STR_MAX / 2], linebuf[CUBALC_HOST_STR_MAX];
    size_t kn = 0, vn = 0;
    const char *sep, *vs, *ve;
    int hit;
    char *end = NULL;
    long num;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    vs = sep ? sep + 1 : p;
    ve = p;
    while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
    while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    vn = (size_t)(ve - vs);
    if (vn >= sizeof oldv) vn = sizeof oldv - 1;
    memcpy(oldv, vs, vn);
    oldv[vn] = 0;
    if (!nn)
      hit = 1;
    else {
      size_t i;
      hit = 0;
      if (nn <= kn) {
        for (i = 0; i + nn <= kn; i++) {
          if (memcmp(path + i, needle, nn) == 0) {
            hit = 1;
            break;
          }
        }
      }
    }
    if (hit) {
      end = NULL;
      num = strtol(oldv, &end, 10);
      if (end && end != oldv && *end == 0) {
        considered++;
        if (num > maxv) {
          char nbuf[32];
          snprintf(nbuf, sizeof nbuf, "%ld", maxv);
          if (snprintf(linebuf, sizeof linebuf, "%s:%s", path, nbuf) >= 0)
            cubalc_bag_push(bag, sizeof bag, &olen, &total, linebuf);
          capped++;
          continue;
        }
      }
    }
    if (snprintf(linebuf, sizeof linebuf, "%s:%s", path, oldv) >= 0)
      cubalc_bag_push(bag, sizeof bag, &olen, &total, linebuf);
  }
  memset(&wr, 0, sizeof wr);
  if (cubalc_host_json_unflat_kv("{}", bag, NULL, &wr) != 0) {
    snprintf(r->err, sizeof r->err, "%s",
             wr.err[0] ? wr.err : "capflat: unflat fail");
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", wr.str);
  r->n = capped;
  r->code = (int)considered;
  r->ok = 1;
  return 0;
}

/* SCALEFLAT: multiply pure-int leaves by factor. See header. */
int cubalc_host_json_leaf_scale(const char *json, const char *needle, long factor,
                                cubalc_host_result *r) {
  cubalc_host_result kv, wr;
  const char *p, *line;
  char bag[CUBALC_HOST_STR_MAX];
  size_t olen = 0;
  long scaled = 0, considered = 0, total = 0;
  size_t nn;

  r_clear(r);
  if (!needle) needle = "";
  nn = strlen(needle);
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  bag[0] = 0;
  p = kv.str;
  while (*p) {
    char path[512], oldv[CUBALC_HOST_STR_MAX / 2], linebuf[CUBALC_HOST_STR_MAX];
    size_t kn = 0, vn = 0;
    const char *sep, *vs, *ve;
    int hit;
    char *end = NULL;
    long num;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    vs = sep ? sep + 1 : p;
    ve = p;
    while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
    while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    vn = (size_t)(ve - vs);
    if (vn >= sizeof oldv) vn = sizeof oldv - 1;
    memcpy(oldv, vs, vn);
    oldv[vn] = 0;
    if (!nn)
      hit = 1;
    else {
      size_t i;
      hit = 0;
      if (nn <= kn) {
        for (i = 0; i + nn <= kn; i++) {
          if (memcmp(path + i, needle, nn) == 0) {
            hit = 1;
            break;
          }
        }
      }
    }
    if (hit) {
      end = NULL;
      num = strtol(oldv, &end, 10);
      if (end && end != oldv && *end == 0) {
        char nbuf[32];
        considered++;
        snprintf(nbuf, sizeof nbuf, "%ld", num * factor);
        if (snprintf(linebuf, sizeof linebuf, "%s:%s", path, nbuf) >= 0)
          cubalc_bag_push(bag, sizeof bag, &olen, &total, linebuf);
        scaled++;
        continue;
      }
    }
    if (snprintf(linebuf, sizeof linebuf, "%s:%s", path, oldv) >= 0)
      cubalc_bag_push(bag, sizeof bag, &olen, &total, linebuf);
  }
  memset(&wr, 0, sizeof wr);
  if (cubalc_host_json_unflat_kv("{}", bag, NULL, &wr) != 0) {
    snprintf(r->err, sizeof r->err, "%s",
             wr.err[0] ? wr.err : "scaleflat: unflat fail");
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", wr.str);
  r->n = scaled;
  r->code = (int)considered;
  r->ok = 1;
  return 0;
}


/* G_hostops_margin: PATHSFLAT/VALSFLAT + PATHSFLATN/VALSFLATN peel from hostops */
/* PATHSFLAT/VALSFLAT: matching path or value bag. See header. */
int cubalc_host_json_leaf_match_bag(const char *json, const char *needle, int want_vals,
                                    cubalc_host_result *r) {
  cubalc_host_result kv;
  const char *p, *line;
  size_t olen = 0;
  long n = 0;
  size_t nn;

  r_clear(r);
  if (!needle) needle = "";
  nn = strlen(needle);
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  r->str[0] = 0;
  p = kv.str;
  while (*p) {
    char path[512], oldv[CUBALC_HOST_STR_MAX / 2];
    size_t kn = 0, vn = 0;
    const char *sep, *vs, *ve;
    int hit;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    vs = sep ? sep + 1 : p;
    ve = p;
    while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
    while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    vn = (size_t)(ve - vs);
    if (vn >= sizeof oldv) vn = sizeof oldv - 1;
    memcpy(oldv, vs, vn);
    oldv[vn] = 0;
    if (!nn)
      hit = 1;
    else {
      size_t i;
      hit = 0;
      if (nn <= kn) {
        for (i = 0; i + nn <= kn; i++) {
          if (memcmp(path + i, needle, nn) == 0) {
            hit = 1;
            break;
          }
        }
      }
    }
    if (!hit) continue;
    cubalc_bag_push(r->str, sizeof r->str, &olen, &n, want_vals ? oldv : path);
  }
  r->n = n;
  r->code = (int)n;
  r->ok = 1;
  return 0;
}

/* PATHSFLATN/VALSFLATN: pure-int path or value bag. See header. */
int cubalc_host_json_leaf_match_bagn(const char *json, const char *needle, int want_vals,
                                     cubalc_host_result *r) {
  cubalc_host_result kv;
  const char *p, *line;
  size_t olen = 0;
  long n = 0;
  size_t nn;

  r_clear(r);
  if (!needle) needle = "";
  nn = strlen(needle);
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  r->str[0] = 0;
  p = kv.str;
  while (*p) {
    char path[512], oldv[CUBALC_HOST_STR_MAX / 2];
    size_t kn = 0, vn = 0;
    const char *sep, *vs, *ve;
    int hit;
    char *end = NULL;
    long num;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    vs = sep ? sep + 1 : p;
    ve = p;
    while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
    while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    vn = (size_t)(ve - vs);
    if (vn >= sizeof oldv) vn = sizeof oldv - 1;
    memcpy(oldv, vs, vn);
    oldv[vn] = 0;
    if (!nn)
      hit = 1;
    else {
      size_t i;
      hit = 0;
      if (nn <= kn) {
        for (i = 0; i + nn <= kn; i++) {
          if (memcmp(path + i, needle, nn) == 0) {
            hit = 1;
            break;
          }
        }
      }
    }
    if (!hit) continue;
    end = NULL;
    num = strtol(oldv, &end, 10);
    if (!(end && end != oldv && *end == 0))
      continue; /* skip non-int */
    if (want_vals) {
      char dec[32];
      snprintf(dec, sizeof dec, "%ld", num);
      cubalc_bag_push(r->str, sizeof r->str, &olen, &n, dec);
    } else {
      cubalc_bag_push(r->str, sizeof r->str, &olen, &n, path);
    }
  }
  r->n = n;
  r->code = (int)n;
  r->ok = 1;
  return 0;
}


/* COUNTFLAT/HASFLAT: count leaf paths matching needle. See header. */
int cubalc_host_json_leaf_count(const char *json, const char *needle,
                                cubalc_host_result *r) {
  cubalc_host_result kv;
  const char *p, *line;
  long count = 0;
  size_t nn;

  r_clear(r);
  if (!needle) needle = "";
  nn = strlen(needle);
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  p = kv.str;
  while (*p) {
    char path[512];
    size_t kn = 0;
    const char *sep;
    int hit;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    if (!nn)
      hit = 1;
    else {
      size_t i;
      hit = 0;
      if (nn <= kn) {
        for (i = 0; i + nn <= kn; i++) {
          if (memcmp(path + i, needle, nn) == 0) {
            hit = 1;
            break;
          }
        }
      }
    }
    if (hit) count++;
  }
  r->n = count;
  r->code = (int)count;
  r->ok = 1;
  snprintf(r->str, sizeof r->str, "%ld", count);
  return 0;
}

/* COUNTFLATN/HASFLATN: count pure-int leaves matching path needle. See header. */
int cubalc_host_json_leaf_countn(const char *json, const char *needle,
                                 cubalc_host_result *r) {
  cubalc_host_result kv;
  const char *p, *line;
  long count = 0;
  size_t nn;

  r_clear(r);
  if (!needle) needle = "";
  nn = strlen(needle);
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  p = kv.str;
  while (*p) {
    char path[512], oldv[CUBALC_HOST_STR_MAX / 2];
    size_t kn = 0, vn = 0;
    const char *sep, *vs, *ve;
    int hit;
    char *end = NULL;
    long num;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    vs = sep ? sep + 1 : p;
    ve = p;
    while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
    while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    vn = (size_t)(ve - vs);
    if (vn >= sizeof oldv) vn = sizeof oldv - 1;
    memcpy(oldv, vs, vn);
    oldv[vn] = 0;
    if (!nn)
      hit = 1;
    else {
      size_t i;
      hit = 0;
      if (nn <= kn) {
        for (i = 0; i + nn <= kn; i++) {
          if (memcmp(path + i, needle, nn) == 0) {
            hit = 1;
            break;
          }
        }
      }
    }
    if (!hit) continue;
    end = NULL;
    num = strtol(oldv, &end, 10);
    if (end && end != oldv && *end == 0)
      count++;
  }
  r->n = count;
  r->code = (int)count;
  r->ok = 1;
  snprintf(r->str, sizeof r->str, "%ld", count);
  return 0;
}

/* Rename leaf path prefixes. See header. */
int cubalc_host_json_leaf_rename_pfx(const char *json, const char *old_pfx,
                                     const char *new_pfx, cubalc_host_result *r) {
  cubalc_host_result kv, wr;
  const char *p, *line;
  char bag[CUBALC_HOST_STR_MAX];
  size_t olen = 0;
  long renamed = 0, kept = 0;
  size_t opx;

  r_clear(r);
  if (!old_pfx) old_pfx = "";
  if (!new_pfx) new_pfx = "";
  opx = strlen(old_pfx);
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  bag[0] = 0;
  if (!old_pfx[0]) {
    /* no-op: return original plate */
    snprintf(r->str, sizeof r->str, "%s",
             (json && json[0]) ? json : "{}");
    {
      const char *b = r->str;
      while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
      if (*b != '{')
        snprintf(r->str, sizeof r->str, "%s", "{}");
    }
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  p = kv.str;
  while (*p) {
    char path[512], val[CUBALC_HOST_STR_MAX / 2], npath[640], linebuf[CUBALC_HOST_STR_MAX];
    size_t kn = 0, vn = 0;
    const char *sep, *vs, *ve;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    vs = sep ? sep + 1 : p;
    ve = p;
    while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
    while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    vn = (size_t)(ve - vs);
    if (vn >= sizeof val) vn = sizeof val - 1;
    memcpy(val, vs, vn);
    val[vn] = 0;
    if (kn >= opx && memcmp(path, old_pfx, opx) == 0) {
      snprintf(npath, sizeof npath, "%s%.*s", new_pfx, (int)(kn - opx), path + opx);
      renamed++;
    } else {
      snprintf(npath, sizeof npath, "%s", path);
    }
    if (snprintf(linebuf, sizeof linebuf, "%s:%s", npath, val) >= 0)
      cubalc_bag_push(bag, sizeof bag, &olen, &kept, linebuf);
  }
  memset(&wr, 0, sizeof wr);
  if (cubalc_host_json_unflat_kv("{}", bag, NULL, &wr) != 0) {
    snprintf(r->err, sizeof r->err, "%s",
             wr.err[0] ? wr.err : "renameflat: unflat fail");
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", wr.str);
  r->n = renamed;
  r->ok = 1;
  r->code = kept; /* total leaves after rename */
  return 0;
}

/* THRESHFLAT / DROPZEROFLAT: value filter pure-int leaves. See header. */
int cubalc_host_json_leaf_thresh(const char *json, const char *needle, long minv,
                                 int drop_zero_only, cubalc_host_result *r) {
  cubalc_host_result kv, wr;
  const char *p, *line;
  char bag[CUBALC_HOST_STR_MAX];
  size_t olen = 0;
  long kept = 0, dropped = 0, total = 0;
  size_t nn;

  r_clear(r);
  if (!needle) needle = "";
  nn = strlen(needle);
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  bag[0] = 0;
  p = kv.str;
  while (*p) {
    char path[512], oldv[CUBALC_HOST_STR_MAX / 2], linebuf[CUBALC_HOST_STR_MAX];
    size_t kn = 0, vn = 0;
    const char *sep, *vs, *ve;
    int hit;
    char *end = NULL;
    long num;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    sep = NULL;
    {
      const char *s = line;
      while (s < p) {
        if (*s == ':' || *s == '=') {
          sep = s;
          break;
        }
        s++;
      }
    }
    kn = sep ? (size_t)(sep - line) : (size_t)(p - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn == 0 || kn >= sizeof path) continue;
    memcpy(path, line, kn);
    path[kn] = 0;
    vs = sep ? sep + 1 : p;
    ve = p;
    while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
    while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
    vn = (size_t)(ve - vs);
    if (vn >= sizeof oldv) vn = sizeof oldv - 1;
    memcpy(oldv, vs, vn);
    oldv[vn] = 0;
    if (!nn)
      hit = 1;
    else {
      size_t i;
      hit = 0;
      if (nn <= kn) {
        for (i = 0; i + nn <= kn; i++) {
          if (memcmp(path + i, needle, nn) == 0) {
            hit = 1;
            break;
          }
        }
      }
    }
    if (hit) {
      end = NULL;
      num = strtol(oldv, &end, 10);
      if (end && end != oldv && *end == 0) {
        int drop = 0;
        if (drop_zero_only)
          drop = (num == 0);
        else
          drop = (num < minv);
        if (drop) {
          dropped++;
          continue; /* omit leaf */
        }
        kept++;
      }
    }
    if (snprintf(linebuf, sizeof linebuf, "%s:%s", path, oldv) >= 0)
      cubalc_bag_push(bag, sizeof bag, &olen, &total, linebuf);
  }
  memset(&wr, 0, sizeof wr);
  if (cubalc_host_json_unflat_kv("{}", bag, NULL, &wr) != 0) {
    snprintf(r->err, sizeof r->err, "%s",
             wr.err[0] ? wr.err : "threshflat: unflat fail");
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", wr.str);
  r->n = kept;
  r->code = (int)dropped;
  r->ok = 1;
  return 0;
}
