/* CubalC — hostops JSON agg/num plane: topn·sort·valmerge·keygrep·sum·minmax·avg·median·valfilter·valmap
 * Peel from cubalc_hostops_json_delta.c (B_hostops_json_agg). Cube is SoT. Zero bloat.
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

/* ---- G_hostops_margin peel: JSONtopn + sortbyval ---- */
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

/* Usability: multi-plate PCTP/SCALEP/ADDP/DIVP/ABSP/SIGNP — rewrite int values.
 * mode 0 PCT · 1 SCALE · 2 ADD · 3 DIV · 4 ABS · 5 SIGN (−1|0|1).
 * Non-int keys kept. r->n = rewritten (ABS: negatives flipped) · r->code = sum|nkeys|
 * for SIGN: (pos&0xff)<<16 | (neg&0xff)<<8 | (zero&0xff). */
int cubalc_host_json_topn(const char *json, long ntake, int want_bot,
                          cubalc_host_result *r) {
  typedef struct { char key[256]; long val; } pair_t;
  cubalc_host_result keys, raw, setr;
  pair_t pairs[256];
  char cur[CUBALC_HOST_STR_MAX];
  char numbuf[40];
  const char *p, *line;
  int np = 0, i, j, take;
  r_clear(r);
  memset(&keys, 0, sizeof keys);
  snprintf(cur, sizeof cur, "%s", "{}");
  if (ntake < 0) ntake = 0;
  if (cubalc_host_json_keys(json, &keys) != 0) {
    snprintf(r->str, sizeof r->str, "%s", "{}");
    r->n = 0;
    r->code = 0;
    r->ok = 1;
    return 0;
  }
  p = keys.str;
  while (*p && np < 256) {
    char key[256];
    size_t kn = 0;
    long num = 0;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    kn = (size_t)(p - line);
    if (kn >= sizeof key) kn = sizeof key - 1;
    memcpy(key, line, kn);
    key[kn] = 0;
    if (!key[0]) continue;
    memset(&raw, 0, sizeof raw);
    if (cubalc_host_json_get_raw(json, key, &raw) != 0) continue;
    if (!json_raw_pure_int(raw.str, &num)) continue;
    snprintf(pairs[np].key, sizeof pairs[np].key, "%s", key);
    pairs[np].val = num;
    np++;
  }
  /* stable insertion sort by value desc (top) or asc (bot); equal keeps order */
  for (i = 1; i < np; i++) {
    pair_t tmp = pairs[i];
    j = i - 1;
    while (j >= 0) {
      int better = want_bot ? (pairs[j].val > tmp.val) : (pairs[j].val < tmp.val);
      if (!better) break;
      pairs[j + 1] = pairs[j];
      j--;
    }
    pairs[j + 1] = tmp;
  }
  take = (ntake > np) ? np : (int)ntake;
  for (i = 0; i < take; i++) {
    snprintf(numbuf, sizeof numbuf, "%ld", pairs[i].val);
    memset(&setr, 0, sizeof setr);
    if (cubalc_host_json_set(cur, pairs[i].key, numbuf, 1, &setr) != 0) {
      snprintf(r->err, sizeof r->err, "%s",
               setr.err[0] ? setr.err : "jsontopn: set fail");
      return -1;
    }
    snprintf(cur, sizeof cur, "%s", setr.str);
  }
  snprintf(r->str, sizeof r->str, "%s", cur);
  r->n = take;
  r->code = np;
  r->ok = 1;
  return 0;
}

/* Usability: multi-plate SORTP/SORTBAGP — full sort pure-int keys by value. */
int cubalc_host_json_sortbyval(const char *json, int want_asc, int as_bag,
                               cubalc_host_result *r) {
  typedef struct { char key[256]; long val; } pair_t;
  cubalc_host_result keys, raw, setr;
  pair_t pairs[256];
  char cur[CUBALC_HOST_STR_MAX];
  char numbuf[40], linebuf[320];
  const char *p, *line;
  int np = 0, i, j;
  size_t olen = 0;
  r_clear(r);
  memset(&keys, 0, sizeof keys);
  snprintf(cur, sizeof cur, "%s", "{}");
  r->str[0] = 0;
  if (cubalc_host_json_keys(json, &keys) != 0) {
    if (as_bag) {
      r->str[0] = 0;
    } else {
      snprintf(r->str, sizeof r->str, "%s", "{}");
    }
    r->n = 0;
    r->code = 0;
    r->ok = 1;
    return 0;
  }
  p = keys.str;
  while (*p && np < 256) {
    char key[256];
    size_t kn = 0;
    long num = 0;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    kn = (size_t)(p - line);
    if (kn >= sizeof key) kn = sizeof key - 1;
    memcpy(key, line, kn);
    key[kn] = 0;
    if (!key[0]) continue;
    memset(&raw, 0, sizeof raw);
    if (cubalc_host_json_get_raw(json, key, &raw) != 0) continue;
    if (!json_raw_pure_int(raw.str, &num)) continue;
    snprintf(pairs[np].key, sizeof pairs[np].key, "%s", key);
    pairs[np].val = num;
    np++;
  }
  for (i = 1; i < np; i++) {
    pair_t tmp = pairs[i];
    j = i - 1;
    while (j >= 0) {
      int better = want_asc ? (pairs[j].val > tmp.val) : (pairs[j].val < tmp.val);
      if (!better) break;
      pairs[j + 1] = pairs[j];
      j--;
    }
    pairs[j + 1] = tmp;
  }
  if (as_bag) {
    for (i = 0; i < np; i++) {
      size_t llen;
      snprintf(linebuf, sizeof linebuf, "%s:%ld", pairs[i].key, pairs[i].val);
      llen = strlen(linebuf);
      if (olen > 0) {
        if (olen + 1 >= sizeof r->str) break;
        r->str[olen++] = '\n';
        r->str[olen] = 0;
      }
      if (olen + llen + 1 >= sizeof r->str) break;
      memcpy(r->str + olen, linebuf, llen + 1);
      olen += llen;
    }
    r->n = np;
  } else {
    for (i = 0; i < np; i++) {
      snprintf(numbuf, sizeof numbuf, "%ld", pairs[i].val);
      memset(&setr, 0, sizeof setr);
      if (cubalc_host_json_set(cur, pairs[i].key, numbuf, 1, &setr) != 0) {
        snprintf(r->err, sizeof r->err, "%s",
                 setr.err[0] ? setr.err : "jsonsort: set fail");
        return -1;
      }
      snprintf(cur, sizeof cur, "%s", setr.str);
    }
    snprintf(r->str, sizeof r->str, "%s", cur);
    r->n = np;
  }
  r->code = np;
  r->ok = 1;
  return 0;
}

/* Usability: multi-plate SUMMERGEP/SUBP — combine two FREQ plates by key.
 * mode 0: sum shared pure-int keys; append b-only keys.
 * mode 1: a−b shared pure-int; append b-only as −b; non-int in a kept. */
int cubalc_host_json_valmerge(const char *a, const char *b, int mode,
                              cubalc_host_result *r) {
  cubalc_host_result ka, kb, rawa, rawb, setr;
  char cur[CUBALC_HOST_STR_MAX];
  const char *p, *line;
  long nkeys = 0, hit = 0;
  r_clear(r);
  memset(&ka, 0, sizeof ka);
  memset(&kb, 0, sizeof kb);
  /* seed from a (or {}) */
  if (cubalc_host_json_keys(a, &ka) != 0) {
    snprintf(cur, sizeof cur, "%s", "{}");
    ka.str[0] = 0;
    ka.n = 0;
  } else {
    snprintf(cur, sizeof cur, "%s", a && a[0] ? a : "{}");
  }
  /* walk b keys and fold */
  if (cubalc_host_json_keys(b, &kb) != 0)
    kb.str[0] = 0;
  p = kb.str;
  while (*p) {
    char key[256];
    size_t kn = 0;
    long va = 0, vb = 0;
    int a_num = 0, b_num = 0, a_has = 0;
    char numbuf[40];
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    kn = (size_t)(p - line);
    if (kn >= sizeof key) kn = sizeof key - 1;
    memcpy(key, line, kn);
    key[kn] = 0;
    if (!key[0]) continue;
    memset(&rawb, 0, sizeof rawb);
    if (cubalc_host_json_get_raw(b, key, &rawb) != 0) continue;
    b_num = json_raw_pure_int(rawb.str, &vb);
    memset(&rawa, 0, sizeof rawa);
    a_has = (cubalc_host_json_get_raw(cur, key, &rawa) == 0) ? 1 : 0;
    if (a_has)
      a_num = json_raw_pure_int(rawa.str, &va);

    if (mode == 0) {
      /* SUMMERGE */
      if (a_has && a_num && b_num) {
        snprintf(numbuf, sizeof numbuf, "%ld", va + vb);
        memset(&setr, 0, sizeof setr);
        if (cubalc_host_json_set(cur, key, numbuf, 1, &setr) != 0) {
          snprintf(r->err, sizeof r->err, "%s",
                   setr.err[0] ? setr.err : "jsonsummerge: set fail");
          return -1;
        }
        snprintf(cur, sizeof cur, "%s", setr.str);
        hit++;
      } else if (!a_has) {
        memset(&setr, 0, sizeof setr);
        if (cubalc_host_json_set(cur, key, rawb.str, 1, &setr) != 0) {
          snprintf(r->err, sizeof r->err, "%s",
                   setr.err[0] ? setr.err : "jsonsummerge: set fail");
          return -1;
        }
        snprintf(cur, sizeof cur, "%s", setr.str);
      }
      /* else a has non-int or b non-int: keep a */
    } else {
      /* SUB a−b */
      if (!b_num) continue; /* ignore non-int b for subtract */
      if (a_has && a_num) {
        snprintf(numbuf, sizeof numbuf, "%ld", va - vb);
        memset(&setr, 0, sizeof setr);
        if (cubalc_host_json_set(cur, key, numbuf, 1, &setr) != 0) {
          snprintf(r->err, sizeof r->err, "%s",
                   setr.err[0] ? setr.err : "jsonsubp: set fail");
          return -1;
        }
        snprintf(cur, sizeof cur, "%s", setr.str);
        hit++;
      } else if (!a_has) {
        snprintf(numbuf, sizeof numbuf, "%ld", -vb);
        memset(&setr, 0, sizeof setr);
        if (cubalc_host_json_set(cur, key, numbuf, 1, &setr) != 0) {
          snprintf(r->err, sizeof r->err, "%s",
                   setr.err[0] ? setr.err : "jsonsubp: set fail");
          return -1;
        }
        snprintf(cur, sizeof cur, "%s", setr.str);
      }
      /* a has non-int: keep a */
    }
  }
  /* count keys in result */
  {
    cubalc_host_result kl;
    memset(&kl, 0, sizeof kl);
    if (cubalc_host_json_keys(cur, &kl) == 0)
      nkeys = kl.n;
  }
  snprintf(r->str, sizeof r->str, "%s", cur);
  r->n = nkeys;
  r->code = (int)hit;
  r->ok = 1;
  return 0;
}

/* ASCII lowercase copy into dst (max dlen-1). */
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

/* Usability: multi-plate KEEPKEYP/DROPKEYP — filter keys by needle substring. */
int cubalc_host_json_keygrep(const char *json, const char *needle, int invert,
                             int icase, cubalc_host_result *r) {
  cubalc_host_result keys, raw, setr;
  char cur[CUBALC_HOST_STR_MAX];
  char nl[256], kl[256];
  const char *p, *line;
  const char *need = needle ? needle : "";
  size_t nlen;
  long kept = 0, drop = 0;
  r_clear(r);
  memset(&keys, 0, sizeof keys);
  snprintf(cur, sizeof cur, "%s", "{}");
  nlen = strlen(need);
  if (icase) ascii_lower_cpy(nl, sizeof nl, need);
  if (cubalc_host_json_keys(json, &keys) != 0) {
    snprintf(r->str, sizeof r->str, "%s", "{}");
    r->n = 0;
    r->code = 0;
    r->ok = 1;
    return 0;
  }
  p = keys.str;
  while (*p) {
    char key[256];
    size_t kn = 0;
    int match = 0, keep;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    kn = (size_t)(p - line);
    if (kn >= sizeof key) kn = sizeof key - 1;
    memcpy(key, line, kn);
    key[kn] = 0;
    if (!key[0]) continue;
    if (nlen == 0) {
      match = 1;
    } else if (!icase) {
      match = (strstr(key, need) != NULL);
    } else {
      ascii_lower_cpy(kl, sizeof kl, key);
      match = (strstr(kl, nl) != NULL);
    }
    keep = invert ? !match : match;
    if (keep) {
      memset(&raw, 0, sizeof raw);
      if (cubalc_host_json_get_raw(json, key, &raw) != 0) {
        drop++;
        continue;
      }
      memset(&setr, 0, sizeof setr);
      if (cubalc_host_json_set(cur, key, raw.str, 1, &setr) != 0) {
        snprintf(r->err, sizeof r->err, "%s",
                 setr.err[0] ? setr.err : "jsonkeygrep: set fail");
        return -1;
      }
      snprintf(cur, sizeof cur, "%s", setr.str);
      kept++;
    } else {
      drop++;
    }
  }
  snprintf(r->str, sizeof r->str, "%s", cur);
  r->n = kept;
  r->code = (int)drop;
  r->ok = 1;
  return 0;
}

/* Peeled from cubalc_hostops.c (G_hostops_margin mesh vote). */
/* Usability: SYS JSONSUMN plate — sum integer top-level values without VALUES+SUM. */
int cubalc_host_json_sum(const char *json, cubalc_host_result *r) {
  cubalc_host_result vals;
  const char *p, *line;
  long sum = 0;
  r_clear(r);
  memset(&vals, 0, sizeof vals);
  if (cubalc_host_json_values(json, &vals) != 0) {
    r->n = 0;
    r->ok = 1;
    snprintf(r->str, sizeof r->str, "0");
    return 0;
  }
  p = vals.str;
  while (*p) {
    char field[CUBALC_HOST_STR_MAX];
    size_t flen;
    char *end = NULL;
    long v;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    flen = (size_t)(p - line);
    if (flen >= sizeof field) flen = sizeof field - 1;
    memcpy(field, line, flen);
    field[flen] = 0;
    if (!field[0]) continue;
    v = strtol(field, &end, 10);
    if (end && end != field && *end == 0)
      sum += v;
  }
  r->n = sum;
  r->ok = 1;
  snprintf(r->str, sizeof r->str, "%ld", sum);
  return 0;
}

/* Usability: SYS JSONMAXN/JSONMINN — extreme integer values without VALUES+MIN/MAX. */
int cubalc_host_json_minmax(const char *json, int want_min, cubalc_host_result *r) {
  cubalc_host_result vals;
  const char *p, *line;
  long best = 0;
  int found = 0;
  r_clear(r);
  memset(&vals, 0, sizeof vals);
  if (cubalc_host_json_values(json, &vals) != 0) {
    r->n = 0;
    r->ok = 1;
    snprintf(r->str, sizeof r->str, "0");
    return 0;
  }
  p = vals.str;
  while (*p) {
    char field[CUBALC_HOST_STR_MAX];
    size_t flen;
    char *end = NULL;
    long v;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    flen = (size_t)(p - line);
    if (flen >= sizeof field) flen = sizeof field - 1;
    memcpy(field, line, flen);
    field[flen] = 0;
    if (!field[0]) continue;
    v = strtol(field, &end, 10);
    if (!(end && end != field && *end == 0))
      continue;
    if (!found || (want_min ? (v < best) : (v > best))) {
      best = v;
      found = 1;
    }
  }
  r->n = found ? best : 0;
  r->ok = 1;
  snprintf(r->str, sizeof r->str, "%ld", r->n);
  return 0;
}

/* Usability: SYS JSONAVGN plate — integer mean of top-level ints without VALUES+AVG. */
int cubalc_host_json_avg(const char *json, cubalc_host_result *r) {
  cubalc_host_result vals;
  const char *p, *line;
  long sum = 0;
  long used = 0;
  r_clear(r);
  memset(&vals, 0, sizeof vals);
  if (cubalc_host_json_values(json, &vals) != 0) {
    r->n = 0;
    r->code = 0;
    r->ok = 1;
    snprintf(r->str, sizeof r->str, "0");
    return 0;
  }
  p = vals.str;
  while (*p) {
    char field[CUBALC_HOST_STR_MAX];
    size_t flen;
    char *end = NULL;
    long v;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    flen = (size_t)(p - line);
    if (flen >= sizeof field) flen = sizeof field - 1;
    memcpy(field, line, flen);
    field[flen] = 0;
    if (!field[0]) continue;
    v = strtol(field, &end, 10);
    if (end && end != field && *end == 0) {
      sum += v;
      used++;
    }
  }
  r->code = (int)used;
  if (used == 0)
    r->n = 0;
  else
    r->n = sum / used; /* trunc toward 0 like integer AVG */
  r->ok = 1;
  snprintf(r->str, sizeof r->str, "%ld", r->n);
  return 0;
}

/* Peeled from cubalc_hostops.c (G_hostops_margin mesh vote). */
/* Usability: multi-plate MEDIANP — integer median of top-level ints (MEDIANKV dual).
 * Sort ascending; even n → lower mid vals[(n-1)/2]. Cap 256. Soft 0 if none. */
int cubalc_host_json_median(const char *json, cubalc_host_result *r) {
  cubalc_host_result vals;
  const char *p, *line;
  long arr[256];
  int n = 0, i;
  r_clear(r);
  memset(&vals, 0, sizeof vals);
  if (cubalc_host_json_values(json, &vals) != 0) {
    r->n = 0;
    r->code = 0;
    r->ok = 1;
    snprintf(r->str, sizeof r->str, "0");
    return 0;
  }
  p = vals.str;
  while (*p && n < 256) {
    char field[CUBALC_HOST_STR_MAX];
    size_t flen;
    char *end = NULL;
    long v;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    flen = (size_t)(p - line);
    if (flen >= sizeof field) flen = sizeof field - 1;
    memcpy(field, line, flen);
    field[flen] = 0;
    if (!field[0]) continue;
    v = strtol(field, &end, 10);
    if (end && end != field && *end == 0)
      arr[n++] = v;
  }
  if (n > 0) {
    /* insertion sort (same as MEDIANKV) */
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

/* Peeled from cubalc_hostops.c (G_hostops_margin mesh vote) — valfilter+valmap. */
/* Uses local json_raw_pure_int already defined above in this TU. */

/* Usability: multi-plate THRESHP/DROPZEROP/CAPP — value filter/clamp without bag glue.
 * mode 0: keep pure-int value >= limit (drop non-int + below). r->n=kept r->code=drop.
 * mode 1: drop pure-int value == 0 (keep non-int + nonzero). r->n=kept r->code=drop.
 * mode 2: clamp pure-int value to <= limit (keep all). r->n=capped r->code=nkeys. */
int cubalc_host_json_valfilter(const char *json, int mode, long limit,
                               cubalc_host_result *r) {
  cubalc_host_result keys, raw, setr;
  char cur[CUBALC_HOST_STR_MAX];
  const char *p, *line;
  long kept = 0, drop = 0, capped = 0, nkeys = 0;
  r_clear(r);
  memset(&keys, 0, sizeof keys);
  snprintf(cur, sizeof cur, "%s", "{}");
  if (cubalc_host_json_keys(json, &keys) != 0) {
    snprintf(r->str, sizeof r->str, "%s", "{}");
    r->n = 0;
    r->code = 0;
    r->ok = 1;
    return 0;
  }
  p = keys.str;
  while (*p) {
    char key[256];
    size_t kn = 0;
    const char *v;
    char *end = NULL;
    long num = 0;
    int is_num = 0;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    kn = (size_t)(p - line);
    if (kn >= sizeof key) kn = sizeof key - 1;
    memcpy(key, line, kn);
    key[kn] = 0;
    if (!key[0]) continue;
    nkeys++;
    memset(&raw, 0, sizeof raw);
    if (cubalc_host_json_get_raw(json, key, &raw) != 0) {
      drop++;
      continue;
    }
    v = raw.str;
    while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
    if (*v == '"') {
      is_num = 0;
    } else if (strncmp(v, "true", 4) == 0 &&
               (v[4] == 0 || v[4] == ',' || v[4] == '}' || v[4] == ' ' ||
                v[4] == '\n' || v[4] == '\r' || v[4] == '\t')) {
      num = 1;
      is_num = 1;
    } else if (strncmp(v, "false", 5) == 0 &&
               (v[5] == 0 || v[5] == ',' || v[5] == '}' || v[5] == ' ' ||
                v[5] == '\n' || v[5] == '\r' || v[5] == '\t')) {
      num = 0;
      is_num = 1;
    } else if (strncmp(v, "null", 4) == 0 || *v == '{' || *v == '[') {
      is_num = 0;
    } else {
      num = strtol(v, &end, 10);
      if (end && end != v && *end == 0)
        is_num = 1;
    }

    if (mode == 0) {
      /* THRESH: keep pure-int value >= limit */
      if (!is_num || num < limit) {
        drop++;
        continue;
      }
      memset(&setr, 0, sizeof setr);
      if (cubalc_host_json_set(cur, key, raw.str, 1, &setr) != 0) {
        snprintf(r->err, sizeof r->err, "%s",
                 setr.err[0] ? setr.err : "jsonthresh: set fail");
        return -1;
      }
      snprintf(cur, sizeof cur, "%s", setr.str);
      kept++;
    } else if (mode == 1) {
      /* DROPZERO: drop pure-int == 0; keep rest */
      if (is_num && num == 0) {
        drop++;
        continue;
      }
      memset(&setr, 0, sizeof setr);
      if (cubalc_host_json_set(cur, key, raw.str, 1, &setr) != 0) {
        snprintf(r->err, sizeof r->err, "%s",
                 setr.err[0] ? setr.err : "jsondropzero: set fail");
        return -1;
      }
      snprintf(cur, sizeof cur, "%s", setr.str);
      kept++;
    } else {
      /* CAP: clamp pure-int to limit; keep all keys */
      char numbuf[32];
      const char *use_raw = raw.str;
      if (is_num && num > limit) {
        snprintf(numbuf, sizeof numbuf, "%ld", limit);
        use_raw = numbuf;
        capped++;
      }
      memset(&setr, 0, sizeof setr);
      if (cubalc_host_json_set(cur, key, use_raw, 1, &setr) != 0) {
        snprintf(r->err, sizeof r->err, "%s",
                 setr.err[0] ? setr.err : "jsoncap: set fail");
        return -1;
      }
      snprintf(cur, sizeof cur, "%s", setr.str);
      kept++;
    }
  }
  snprintf(r->str, sizeof r->str, "%s", cur);
  if (mode == 2) {
    r->n = capped;
    r->code = (int)nkeys;
  } else {
    r->n = kept;
    r->code = (int)drop;
  }
  r->ok = 1;
  return 0;
}

/* Usability: multi-plate PCTP/SCALEP/ADDP/DIVP/ABSP/SIGNP — rewrite int values.
 * mode 0 PCT · 1 SCALE · 2 ADD · 3 DIV · 4 ABS · 5 SIGN (−1|0|1).
 * Non-int keys kept. r->n = rewritten (ABS: negatives flipped) · r->code = sum|nkeys|
 * for SIGN: (pos&0xff)<<16 | (neg&0xff)<<8 | (zero&0xff). */
int cubalc_host_json_valmap(const char *json, int mode, long arg,
                            cubalc_host_result *r) {
  cubalc_host_result keys, raw, setr;
  char cur[CUBALC_HOST_STR_MAX];
  const char *p, *line;
  long total = 0, rew = 0, nkeys = 0;
  long npos = 0, nneg = 0, nzero = 0;
  r_clear(r);
  memset(&keys, 0, sizeof keys);
  snprintf(cur, sizeof cur, "%s", "{}");
  if (cubalc_host_json_keys(json, &keys) != 0) {
    snprintf(r->str, sizeof r->str, "%s", "{}");
    r->n = 0;
    r->code = 0;
    r->ok = 1;
    return 0;
  }
  /* PCT: sum pure-int values first */
  if (mode == 0) {
    p = keys.str;
    while (*p) {
      char key[256];
      size_t kn = 0;
      long num = 0;
      while (*p == '\n' || *p == '\r') p++;
      if (!*p) break;
      line = p;
      while (*p && *p != '\n' && *p != '\r') p++;
      kn = (size_t)(p - line);
      if (kn >= sizeof key) kn = sizeof key - 1;
      memcpy(key, line, kn);
      key[kn] = 0;
      if (!key[0]) continue;
      memset(&raw, 0, sizeof raw);
      if (cubalc_host_json_get_raw(json, key, &raw) != 0) continue;
      if (json_raw_pure_int(raw.str, &num)) total += num;
    }
  }
  /* rebuild plate */
  p = keys.str;
  while (*p) {
    char key[256];
    size_t kn = 0;
    long num = 0;
    char numbuf[40];
    const char *use_raw;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    kn = (size_t)(p - line);
    if (kn >= sizeof key) kn = sizeof key - 1;
    memcpy(key, line, kn);
    key[kn] = 0;
    if (!key[0]) continue;
    nkeys++;
    memset(&raw, 0, sizeof raw);
    if (cubalc_host_json_get_raw(json, key, &raw) != 0) continue;
    use_raw = raw.str;
    if (json_raw_pure_int(raw.str, &num)) {
      long outv;
      if (mode == 0)
        outv = (total != 0) ? ((num * 100) / total) : 0;
      else if (mode == 1)
        outv = num * arg;
      else if (mode == 3)
        outv = (arg != 0) ? (num / arg) : 0;
      else if (mode == 4) {
        outv = (num < 0) ? -num : num;
        if (num < 0) rew++; /* count only flipped negatives like ABSKV */
      } else if (mode == 5) {
        if (num > 0) { outv = 1; npos++; }
        else if (num < 0) { outv = -1; nneg++; }
        else { outv = 0; nzero++; }
        rew++;
      } else
        outv = num + arg;
      if (mode != 4 && mode != 5)
        rew++;
      else if (mode == 4 && num >= 0)
        ; /* already counted only negatives */
      snprintf(numbuf, sizeof numbuf, "%ld", outv);
      use_raw = numbuf;
    }
    memset(&setr, 0, sizeof setr);
    if (cubalc_host_json_set(cur, key, use_raw, 1, &setr) != 0) {
      snprintf(r->err, sizeof r->err, "%s",
               setr.err[0] ? setr.err : "jsonvalmap: set fail");
      return -1;
    }
    snprintf(cur, sizeof cur, "%s", setr.str);
  }
  snprintf(r->str, sizeof r->str, "%s", cur);
  r->n = rew;
  if (mode == 0)
    r->code = (int)total;
  else if (mode == 5) {
    /* pack pos/neg/zero into code; also store full counts via n if needed */
    r->code = (int)(((npos & 0xff) << 16) | ((nneg & 0xff) << 8) | (nzero & 0xff));
  } else
    r->code = (int)nkeys;
  r->ok = 1;
  return 0;
}

