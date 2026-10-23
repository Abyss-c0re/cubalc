/* CubalC — hostops JSON query/presence: to_kv·len·first·coalesce·filter·has·eq·subset
 * Peel from cubalc_hostops_json_delta.c (B_hostops_json_query). Cube is SoT. Zero bloat.
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

/* Usability: SYS JSONTOKV plate — object → key:val bag (dual of JSONFROMKV).
 * Uses ':' so default LOOKUP/FREQ work without sep glue. r->n = pairs. */
int cubalc_host_json_to_kv(const char *json, cubalc_host_result *r) {
  cubalc_host_result keys, gr, raw;
  const char *p, *line;
  size_t olen = 0;
  long kept = 0;
  r_clear(r);
  memset(&keys, 0, sizeof keys);
  if (cubalc_host_json_keys(json, &keys) != 0) {
    r->str[0] = 0;
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  r->str[0] = 0;
  p = keys.str;
  while (*p) {
    char key[256];
    char linebuf[CUBALC_HOST_STR_MAX];
    char valbuf[CUBALC_HOST_STR_MAX];
    size_t kn = 0;
    const char *valtext = "";
    const char *v;
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
    if (cubalc_host_json_get_raw(json, key, &raw) != 0)
      continue;
    v = raw.str;
    while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
    memset(&gr, 0, sizeof gr);
    valbuf[0] = 0;
    if (*v == '"') {
      if (cubalc_host_json_get(json, key, &gr) == 0) {
        valtext = gr.str;
      } else {
        size_t rl = strlen(v);
        if (rl >= 2 && v[0] == '"' && v[rl - 1] == '"') {
          size_t n = rl - 2;
          if (n >= sizeof valbuf) n = sizeof valbuf - 1;
          memcpy(valbuf, v + 1, n);
          valbuf[n] = 0;
          valtext = valbuf;
        } else
          valtext = v;
      }
    } else if (strncmp(v, "true", 4) == 0 &&
               (v[4] == 0 || v[4] == ',' || v[4] == '}' || v[4] == ' ' ||
                v[4] == '\n' || v[4] == '\r' || v[4] == '\t')) {
      valtext = "true";
    } else if (strncmp(v, "false", 5) == 0 &&
               (v[5] == 0 || v[5] == ',' || v[5] == '}' || v[5] == ' ' ||
                v[5] == '\n' || v[5] == '\r' || v[5] == '\t')) {
      valtext = "false";
    } else if (strncmp(v, "null", 4) == 0 &&
               (v[4] == 0 || v[4] == ',' || v[4] == '}' || v[4] == ' ' ||
                v[4] == '\n' || v[4] == '\r' || v[4] == '\t')) {
      valtext = "null";
    } else {
      /* num, obj, arr — raw (single-line best effort) */
      size_t i = 0;
      while (v[i] && v[i] != '\n' && v[i] != '\r' && i + 1 < sizeof valbuf) {
        valbuf[i] = v[i];
        i++;
      }
      valbuf[i] = 0;
      valtext = valbuf;
    }
    /* key:val — FREQ/LOOKUP default sep; empty string value → key: */
    if (snprintf(linebuf, sizeof linebuf, "%s:%s", key, valtext ? valtext : "") < 0)
      continue;
    cubalc_bag_push(r->str, sizeof r->str, &olen, &kept, linebuf);
  }
  r->n = kept;
  r->ok = 1;
  return 0;
}


/* Peeled from cubalc_hostops.c (G_hostops_margin mesh vote). */
int cubalc_host_json_len(const char *json, cubalc_host_result *r) {
  cubalc_host_result keys;
  r_clear(r);
  memset(&keys, 0, sizeof keys);
  if (cubalc_host_json_keys(json, &keys) != 0) {
    r->n = 0;
    r->ok = 1;
    snprintf(r->str, sizeof r->str, "0");
    return 0;
  }
  r->n = keys.n;
  r->ok = 1;
  snprintf(r->str, sizeof r->str, "%ld", keys.n);
  return 0;
}

/* Peeled from cubalc_hostops.c (G_hostops_margin mesh vote). */
/* Usability: SYS JSONHITKEY/JSONFIRST — first present candidate key (JSONHASANY twin that names hit). */
int cubalc_host_json_first_key(const char *json, const char *keys_nl,
                               cubalc_host_result *r) {
  cubalc_host_result gr;
  const char *p, *line;
  long idx = 0;
  r_clear(r);
  r->ok = 1;
  r->str[0] = 0;
  r->n = 0;
  r->code = 0;
  if (!keys_nl || !keys_nl[0]) {
    return 0;
  }
  p = keys_nl;
  while (*p) {
    char key[256];
    size_t kn = 0;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    kn = (size_t)(p - line);
    if (kn >= sizeof key) kn = sizeof key - 1;
    memcpy(key, line, kn);
    key[kn] = 0;
    if (!key[0]) continue;
    idx++;
    memset(&gr, 0, sizeof gr);
    /* path_get_raw: dotted/slash paths + null/false/0 still count as present */
    if (cubalc_host_json_path_get_raw(json, key, &gr) == 0) {
      snprintf(r->str, sizeof r->str, "%s", key);
      r->n = 1;
      r->code = (int)idx;
      return 0;
    }
  }
  return 0;
}

/* Peeled from cubalc_hostops.c (G_hostops_margin mesh vote). */
int cubalc_host_json_first_val(const char *json, const char *keys_nl,
                               cubalc_host_result *r) {
  cubalc_host_result gr;
  const char *p, *line;
  long idx = 0;
  r_clear(r);
  r->ok = 1;
  r->str[0] = 0;
  r->err[0] = 0;
  r->n = 0;
  r->code = 0;
  if (!keys_nl || !keys_nl[0]) {
    return 0;
  }
  p = keys_nl;
  while (*p) {
    char key[256];
    size_t kn = 0;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    kn = (size_t)(p - line);
    if (kn >= sizeof key) kn = sizeof key - 1;
    memcpy(key, line, kn);
    key[kn] = 0;
    if (!key[0]) continue;
    idx++;
    memset(&gr, 0, sizeof gr);
    /* path_get: shallow + nest; null → empty string still counts present via path_get_raw first */
    if (cubalc_host_json_path_get_raw(json, key, &gr) == 0) {
      /* re-get decoded form for agent use (true→1, numbers as str, null→"") */
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_path_get(json, key, &gr) != 0) {
        /* raw present but decode miss — treat as empty value hit */
        r->str[0] = 0;
      } else {
        snprintf(r->str, sizeof r->str, "%s", gr.str);
      }
      snprintf(r->err, sizeof r->err, "%s", key);
      r->n = 1;
      r->code = (int)idx;
      return 0;
    }
  }
  return 0;
}

/* Peeled from cubalc_hostops.c (G_hostops_margin mesh vote). */
int cubalc_host_json_coalesce(const char *json, const char *keys_nl,
                              const char *deflt, cubalc_host_result *r) {
  cubalc_host_result gr;
  const char *p, *line;
  long idx = 0;
  r_clear(r);
  r->ok = 1;
  r->str[0] = 0;
  r->err[0] = 0;
  r->n = 0;
  r->code = 0;
  if (keys_nl && keys_nl[0]) {
    p = keys_nl;
    while (*p) {
      char key[256];
      size_t kn = 0;
      while (*p == '\n' || *p == '\r') p++;
      if (!*p) break;
      line = p;
      while (*p && *p != '\n' && *p != '\r') p++;
      kn = (size_t)(p - line);
      if (kn >= sizeof key) kn = sizeof key - 1;
      memcpy(key, line, kn);
      key[kn] = 0;
      if (!key[0]) continue;
      idx++;
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_path_get_raw(json, key, &gr) != 0)
        continue;
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_path_get(json, key, &gr) != 0)
        continue;
      /* skip empty / blank-only values — coalesce wants substance */
      {
        const char *s = gr.str;
        while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
        if (!*s) continue;
      }
      snprintf(r->str, sizeof r->str, "%s", gr.str);
      snprintf(r->err, sizeof r->err, "%s", key);
      r->n = 1;
      r->code = (int)idx;
      return 0;
    }
  }
  if (deflt && deflt[0]) {
    snprintf(r->str, sizeof r->str, "%s", deflt);
    r->n = 0;
    r->code = 0;
    r->err[0] = 0;
  }
  return 0;
}

/* filter_req/plate_keys peel from hostops.c — G_hostops_margin mesh vote */
/* Usability: SYS JSONMISS/JSONPRESENT — bag of missing/present required keys. */
int cubalc_host_json_filter_req_keys(const char *json, const char *keys_nl,
                                     int want_present, cubalc_host_result *r) {
  cubalc_host_result gr;
  const char *p, *line;
  size_t olen = 0;
  long count = 0;
  r_clear(r);
  r->ok = 1;
  r->str[0] = 0;
  r->n = 0;
  if (!keys_nl || !keys_nl[0]) {
    snprintf(r->str, sizeof r->str, "");
    return 0;
  }
  p = keys_nl;
  while (*p) {
    char key[256];
    size_t kn = 0;
    int present;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    kn = (size_t)(p - line);
    if (kn >= sizeof key) kn = sizeof key - 1;
    memcpy(key, line, kn);
    key[kn] = 0;
    if (!key[0]) continue;
    memset(&gr, 0, sizeof gr);
    /* path_get_raw: dotted nest contracts · null/false/0 still present */
    present = (cubalc_host_json_path_get_raw(json, key, &gr) == 0) ? 1 : 0;
    if ((want_present && present) || (!want_present && !present)) {
      size_t al = strlen(key);
      if (olen > 0) {
        if (olen + 1 >= sizeof r->str) break;
        r->str[olen++] = '\n';
        r->str[olen] = 0;
      }
      if (olen + al + 1 >= sizeof r->str) break;
      memcpy(r->str + olen, key, al + 1);
      olen += al;
      count++;
    }
  }
  r->n = count;
  return 0;
}

/* Usability: SYS JSONEXTRA/JSONKNOWN — plate keys outside/inside allow-list. */
int cubalc_host_json_filter_plate_keys(const char *json, const char *allowed_nl,
                                       int want_extra, cubalc_host_result *r) {
  cubalc_host_result keys;
  const char *p, *line;
  size_t olen = 0;
  long count = 0;
  r_clear(r);
  r->ok = 1;
  r->str[0] = 0;
  r->n = 0;
  memset(&keys, 0, sizeof keys);
  if (cubalc_host_json_keys(json, &keys) != 0) {
    /* non-object: no plate keys */
    return 0;
  }
  p = keys.str;
  while (*p) {
    char key[256];
    size_t kn = 0;
    int in_allowed = 0;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    kn = (size_t)(p - line);
    if (kn >= sizeof key) kn = sizeof key - 1;
    memcpy(key, line, kn);
    key[kn] = 0;
    if (!key[0]) continue;
    /* exact field membership in allow-list bag */
    if (allowed_nl && allowed_nl[0]) {
      const char *q = allowed_nl;
      size_t klen = strlen(key);
      while (*q) {
        const char *start;
        size_t flen;
        while (*q == '\n' || *q == '\r') q++;
        if (!*q) break;
        start = q;
        while (*q && *q != '\n' && *q != '\r') q++;
        flen = (size_t)(q - start);
        if (flen == klen && (klen == 0 || memcmp(start, key, klen) == 0)) {
          in_allowed = 1;
          break;
        }
      }
    }
    if ((want_extra && !in_allowed) || (!want_extra && in_allowed)) {
      size_t al = strlen(key);
      if (olen > 0) {
        if (olen + 1 >= sizeof r->str) break;
        r->str[olen++] = '\n';
        r->str[olen] = 0;
      }
      if (olen + al + 1 >= sizeof r->str) break;
      memcpy(r->str + olen, key, al + 1);
      olen += al;
      count++;
    }
  }
  r->n = count;
  return 0;
}


/* subset_bad_keys + has_keys peel from hostops.c — G_hostops_margin mesh vote */
/* Usability: bag of keys in sub that fail subset match (missing in super or raw ≠).
 * Companion of REQUIRE JSONSUBSET error listing · soft always OK. */
int cubalc_host_json_subset_bad_keys(const char *sub, const char *super,
                                     cubalc_host_result *r) {
  cubalc_host_result ksub, ra, rb;
  const char *p, *line;
  size_t olen = 0;
  long count = 0;
  r_clear(r);
  r->ok = 1;
  r->str[0] = 0;
  r->n = 0;
  memset(&ksub, 0, sizeof ksub);
  if (cubalc_host_json_keys(sub, &ksub) != 0) {
    /* non-object sub: no key list — report as whole miss via empty bag */
    snprintf(r->str, sizeof r->str, "");
    return 0;
  }
  p = ksub.str;
  while (*p) {
    char key[256];
    size_t kn = 0;
    int bad = 0;
    const char *va, *vb;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    kn = (size_t)(p - line);
    if (kn >= sizeof key) kn = sizeof key - 1;
    memcpy(key, line, kn);
    key[kn] = 0;
    if (!key[0]) continue;
    memset(&ra, 0, sizeof ra);
    memset(&rb, 0, sizeof rb);
    if (cubalc_host_json_get_raw(sub, key, &ra) != 0) {
      bad = 1;
    } else if (cubalc_host_json_get_raw(super, key, &rb) != 0) {
      bad = 1;
    } else {
      va = ra.str;
      vb = rb.str;
      while (*va == ' ' || *va == '\t' || *va == '\n' || *va == '\r') va++;
      while (*vb == ' ' || *vb == '\t' || *vb == '\n' || *vb == '\r') vb++;
      if (strcmp(va, vb) != 0) bad = 1;
    }
    if (bad) {
      size_t need = kn + (olen ? 1 : 0);
      if (olen + need + 1 < sizeof r->str) {
        if (olen) r->str[olen++] = '\n';
        memcpy(r->str + olen, key, kn);
        olen += kn;
        r->str[olen] = 0;
        count++;
      }
    }
  }
  r->n = count;
  return 0;
}

/* Usability: SYS JSONHASALL/JSONHASANY — multi-key presence without multi JSONHAS. */
int cubalc_host_json_has_keys(const char *json, const char *keys_nl, int want_all,
                              cubalc_host_result *r) {
  cubalc_host_result gr;
  const char *p, *line;
  long found = 0, listed = 0;
  r_clear(r);
  r->ok = 1;
  if (!keys_nl || !keys_nl[0]) {
    /* empty key list: all-of-empty is true; any-of-empty is false */
    r->n = want_all ? 1 : 0;
    r->code = 0;
    snprintf(r->str, sizeof r->str, "%ld", r->n);
    return 0;
  }
  p = keys_nl;
  while (*p) {
    char key[256];
    size_t kn = 0;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    kn = (size_t)(p - line);
    if (kn >= sizeof key) kn = sizeof key - 1;
    memcpy(key, line, kn);
    key[kn] = 0;
    if (!key[0]) continue;
    listed++;
    memset(&gr, 0, sizeof gr);
    /* path_get_raw: dotted/slash paths + null/false/0 still count as present */
    if (cubalc_host_json_path_get_raw(json, key, &gr) == 0)
      found++;
    else if (want_all) {
      r->n = 0;
      r->code = (int)found;
      snprintf(r->str, sizeof r->str, "0");
      return 0;
    }
  }
  if (listed == 0) {
    r->n = want_all ? 1 : 0;
    r->code = 0;
  } else if (want_all) {
    r->n = (found == listed) ? 1 : 0;
    r->code = (int)found;
  } else {
    r->n = (found > 0) ? 1 : 0;
    r->code = (int)found;
  }
  snprintf(r->str, sizeof r->str, "%ld", r->n);
  return 0;
}

/* ---- G_hostops_margin peel: JSONEQ + JSONSUBSET from hostops.c ---- */
/* Usability: SYS JSONEQ a b — order-independent top-level plate equality. */
int cubalc_host_json_eq(const char *a, const char *b, cubalc_host_result *r) {
  cubalc_host_result ka, kb, ra, rb;
  const char *p, *line;
  r_clear(r);
  r->ok = 1;
  memset(&ka, 0, sizeof ka);
  memset(&kb, 0, sizeof kb);
  /* both must be objects; non-object soft unequal (even if identical text) */
  if (cubalc_host_json_keys(a, &ka) != 0 || cubalc_host_json_keys(b, &kb) != 0) {
    r->n = 0;
    snprintf(r->str, sizeof r->str, "0");
    return 0;
  }
  /* identical object text fast path after object check */
  if (a && b && strcmp(a, b) == 0) {
    r->n = 1;
    snprintf(r->str, sizeof r->str, "1");
    return 0;
  }
  if (ka.n != kb.n) {
    r->n = 0;
    snprintf(r->str, sizeof r->str, "0");
    return 0;
  }
  /* every key in a must exist in b with same raw value */
  p = ka.str;
  while (*p) {
    char key[256];
    size_t kn = 0;
    const char *va, *vb;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    kn = (size_t)(p - line);
    if (kn >= sizeof key) kn = sizeof key - 1;
    memcpy(key, line, kn);
    key[kn] = 0;
    if (!key[0]) continue;
    memset(&ra, 0, sizeof ra);
    memset(&rb, 0, sizeof rb);
    if (cubalc_host_json_get_raw(a, key, &ra) != 0 ||
        cubalc_host_json_get_raw(b, key, &rb) != 0) {
      r->n = 0;
      snprintf(r->str, sizeof r->str, "0");
      return 0;
    }
    va = ra.str;
    vb = rb.str;
    while (*va == ' ' || *va == '\t' || *va == '\n' || *va == '\r') va++;
    while (*vb == ' ' || *vb == '\t' || *vb == '\n' || *vb == '\r') vb++;
    if (strcmp(va, vb) != 0) {
      r->n = 0;
      snprintf(r->str, sizeof r->str, "0");
      return 0;
    }
  }
  r->n = 1;
  snprintf(r->str, sizeof r->str, "1");
  return 0;
}

/* Usability: SYS JSONSUBSET sub super — every key in sub matches super raw value. */
int cubalc_host_json_subset(const char *sub, const char *super, cubalc_host_result *r) {
  cubalc_host_result ksub, ksup, ra, rb;
  const char *p, *line;
  r_clear(r);
  r->ok = 1;
  memset(&ksub, 0, sizeof ksub);
  memset(&ksup, 0, sizeof ksup);
  /* super must be object; sub must be object (empty {} ok) */
  if (cubalc_host_json_keys(super, &ksup) != 0) {
    r->n = 0;
    snprintf(r->str, sizeof r->str, "0");
    return 0;
  }
  if (cubalc_host_json_keys(sub, &ksub) != 0) {
    r->n = 0;
    snprintf(r->str, sizeof r->str, "0");
    return 0;
  }
  /* empty sub is subset of any object */
  if (ksub.n == 0) {
    r->n = 1;
    snprintf(r->str, sizeof r->str, "1");
    return 0;
  }
  p = ksub.str;
  while (*p) {
    char key[256];
    size_t kn = 0;
    const char *va, *vb;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    kn = (size_t)(p - line);
    if (kn >= sizeof key) kn = sizeof key - 1;
    memcpy(key, line, kn);
    key[kn] = 0;
    if (!key[0]) continue;
    memset(&ra, 0, sizeof ra);
    memset(&rb, 0, sizeof rb);
    if (cubalc_host_json_get_raw(sub, key, &ra) != 0 ||
        cubalc_host_json_get_raw(super, key, &rb) != 0) {
      r->n = 0;
      snprintf(r->str, sizeof r->str, "0");
      return 0;
    }
    va = ra.str;
    vb = rb.str;
    while (*va == ' ' || *va == '\t' || *va == '\n' || *va == '\r') va++;
    while (*vb == ' ' || *vb == '\t' || *vb == '\n' || *vb == '\r') vb++;
    if (strcmp(va, vb) != 0) {
      r->n = 0;
      snprintf(r->str, sizeof r->str, "0");
      return 0;
    }
  }
  r->n = 1;
  snprintf(r->str, sizeof r->str, "1");
  return 0;
}
