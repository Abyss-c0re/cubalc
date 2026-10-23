/* CubalC — hostops JSON pluck/delta/topn/sort/sum/minmax/avg/median/keygrep peel (G_hostops).
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


int cubalc_host_json_pluck(const char *json, const char *keys_nl,
                           cubalc_host_result *r) {
  cubalc_host_result gr;
  const char *p, *line;
  size_t olen = 0;
  long listed = 0, found = 0;
  int started = 0; /* allow leading empty fields (null / miss) */
  int last_empty = 0;
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
    char field[CUBALC_HOST_STR_MAX];
    size_t flen = 0;
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
    field[0] = 0;
    memset(&gr, 0, sizeof gr);
    /* path_get: shallow keys + nested freq.error / meta/role */
    if (cubalc_host_json_path_get(json, key, &gr) == 0) {
      found++;
      /* bag fields cannot embed newlines — flatten decoded value */
      {
        const char *s = gr.str;
        while (*s && flen + 1 < sizeof field) {
          char c = *s++;
          if (c == '\n' || c == '\r') c = ' ';
          field[flen++] = c;
        }
        field[flen] = 0;
      }
    }
    last_empty = (field[0] == 0) ? 1 : 0;
    if (started) {
      if (olen + 1 >= sizeof r->str) break;
      r->str[olen++] = '\n';
      r->str[olen] = 0;
    }
    started = 1;
    {
      size_t al = strlen(field);
      if (olen + al + 1 >= sizeof r->str) break;
      memcpy(r->str + olen, field, al + 1);
      olen += al;
    }
  }
  /* trailing empty among 2+ fields: NTH/LINES need a final \n to observe it.
   * Sole empty field stays "" so SYS EMPTY LAST works (LAST_N still = listed). */
  if (listed > 1 && last_empty && olen + 1 < sizeof r->str) {
    r->str[olen++] = '\n';
    r->str[olen] = 0;
  }
  r->n = listed;
  r->code = (int)found;
  return 0;
}

/* Usability: SYS JSONKEYDIFF/JSONKEYCOMM — key-set diff/inter without JSONKEYS+DIFF. */
int cubalc_host_json_key_set_op(const char *a, const char *b, int want_inter,
                                cubalc_host_result *r) {
  cubalc_host_result ka, kb;
  const char *p, *line;
  size_t olen = 0;
  long count = 0;
  r_clear(r);
  r->ok = 1;
  r->str[0] = 0;
  r->n = 0;
  memset(&ka, 0, sizeof ka);
  memset(&kb, 0, sizeof kb);
  if (cubalc_host_json_keys(a, &ka) != 0) {
    /* non-object a → empty result */
    return 0;
  }
  if (cubalc_host_json_keys(b, &kb) != 0)
    kb.str[0] = 0;
  p = ka.str;
  while (*p) {
    char key[256];
    size_t kn = 0;
    int in_b = 0;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    kn = (size_t)(p - line);
    if (kn >= sizeof key) kn = sizeof key - 1;
    memcpy(key, line, kn);
    key[kn] = 0;
    if (!key[0]) continue;
    if (kb.str[0]) {
      const char *q = kb.str;
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
          in_b = 1;
          break;
        }
      }
    }
    if ((want_inter && in_b) || (!want_inter && !in_b)) {
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

/* Usability: SYS JSONCHANGED/JSONSTABLE — value+structure key delta between plates. */
int cubalc_host_json_changed_keys(const char *a, const char *b, int want_same,
                                  cubalc_host_result *r) {
  cubalc_host_result ka, kb, ra, rb;
  const char *p, *line;
  size_t olen = 0;
  long count = 0;
  r_clear(r);
  r->ok = 1;
  r->str[0] = 0;
  r->n = 0;
  memset(&ka, 0, sizeof ka);
  memset(&kb, 0, sizeof kb);
  if (cubalc_host_json_keys(a, &ka) != 0)
    ka.str[0] = 0;
  if (cubalc_host_json_keys(b, &kb) != 0)
    kb.str[0] = 0;
  /* walk keys of a */
  p = ka.str;
  while (*p) {
    char key[256];
    size_t kn = 0;
    int has_a, has_b, same = 0, keep = 0;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    kn = (size_t)(p - line);
    if (kn >= sizeof key) kn = sizeof key - 1;
    memcpy(key, line, kn);
    key[kn] = 0;
    if (!key[0]) continue;
    has_a = 1;
    memset(&ra, 0, sizeof ra);
    memset(&rb, 0, sizeof rb);
    has_b = (cubalc_host_json_get_raw(b, key, &rb) == 0) ? 1 : 0;
    if (cubalc_host_json_get_raw(a, key, &ra) != 0)
      has_a = 0;
    if (has_a && has_b) {
      const char *va = ra.str, *vb = rb.str;
      while (*va == ' ' || *va == '\t' || *va == '\n' || *va == '\r') va++;
      while (*vb == ' ' || *vb == '\t' || *vb == '\n' || *vb == '\r') vb++;
      same = (strcmp(va, vb) == 0) ? 1 : 0;
    }
    if (want_same)
      keep = (has_a && has_b && same) ? 1 : 0;
    else
      keep = (!has_b || !same) ? 1 : 0; /* only-a or value differ */
    if (keep) {
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
  /* keys only in b (changed); skip for want_same */
  if (!want_same) {
    p = kb.str;
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
      memset(&ra, 0, sizeof ra);
      if (cubalc_host_json_get_raw(a, key, &ra) == 0)
        continue; /* already handled as common/diff */
      {
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
  }
  r->n = count;
  return 0;
}

/* Usability: SYS JSONDELTA — changed keys as plate object (values from new/right). */
int cubalc_host_json_delta_obj(const char *a, const char *b, int prefer_b,
                               cubalc_host_result *r) {
  cubalc_host_result ch, gr, setr;
  const char *p, *line;
  char cur[CUBALC_HOST_STR_MAX];
  long count = 0;
  r_clear(r);
  r->ok = 1;
  snprintf(cur, sizeof cur, "%s", "{}");
  r->str[0] = 0;
  r->n = 0;
  memset(&ch, 0, sizeof ch);
  if (cubalc_host_json_changed_keys(a, b, 0, &ch) != 0 || !ch.str[0]) {
    snprintf(r->str, sizeof r->str, "%s", "{}");
    r->n = 0;
    return 0;
  }
  p = ch.str;
  while (*p) {
    char key[256];
    size_t kn = 0;
    const char *src_plate;
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
    src_plate = prefer_b ? b : a;
    if (cubalc_host_json_get_raw(src_plate, key, &gr) != 0) {
      /* key only on the other side */
      src_plate = prefer_b ? a : b;
      if (cubalc_host_json_get_raw(src_plate, key, &gr) != 0)
        continue;
    }
    memset(&setr, 0, sizeof setr);
    if (cubalc_host_json_set(cur, key, gr.str, 1, &setr) != 0)
      continue;
    snprintf(cur, sizeof cur, "%s", setr.str);
    count++;
  }
  snprintf(r->str, sizeof r->str, "%s", cur);
  r->n = count;
  return 0;
}

/* Usability: SYS JSONCHANGELOG — "key: old → new" bag for agent/log plate sync. */
int cubalc_host_json_changelog(const char *a, const char *b, cubalc_host_result *r) {
  cubalc_host_result ch, ra, rb;
  const char *p, *line;
  size_t olen = 0;
  long count = 0;
  r_clear(r);
  r->ok = 1;
  r->str[0] = 0;
  r->n = 0;
  memset(&ch, 0, sizeof ch);
  if (cubalc_host_json_changed_keys(a, b, 0, &ch) != 0 || !ch.str[0]) {
    snprintf(r->str, sizeof r->str, "%s", "");
    return 0;
  }
  p = ch.str;
  while (*p) {
    char key[256];
    char linebuf[CUBALC_HOST_STR_MAX];
    char oldv[CUBALC_HOST_STR_MAX / 4];
    char newv[CUBALC_HOST_STR_MAX / 4];
    size_t kn = 0, li;
    int has_a, has_b;
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
    has_a = (cubalc_host_json_get_raw(a, key, &ra) == 0) ? 1 : 0;
    has_b = (cubalc_host_json_get_raw(b, key, &rb) == 0) ? 1 : 0;
    /* flatten raw values to single-line tokens */
    oldv[0] = 0;
    newv[0] = 0;
    if (has_a) {
      size_t oi = 0;
      for (va = ra.str; *va && oi + 1 < sizeof oldv; va++) {
        if (*va == '\n' || *va == '\r') {
          if (oi > 0 && oldv[oi - 1] != ' ') oldv[oi++] = ' ';
        } else {
          oldv[oi++] = *va;
        }
      }
      oldv[oi] = 0;
      /* trim trailing space */
      while (oi > 0 && (oldv[oi - 1] == ' ' || oldv[oi - 1] == '\t'))
        oldv[--oi] = 0;
    }
    if (has_b) {
      size_t ni = 0;
      for (vb = rb.str; *vb && ni + 1 < sizeof newv; vb++) {
        if (*vb == '\n' || *vb == '\r') {
          if (ni > 0 && newv[ni - 1] != ' ') newv[ni++] = ' ';
        } else {
          newv[ni++] = *vb;
        }
      }
      newv[ni] = 0;
      while (ni > 0 && (newv[ni - 1] == ' ' || newv[ni - 1] == '\t'))
        newv[--ni] = 0;
    }
    /* present empty string stays ""; only truly absent sides use (missing) */
    if (has_a && has_b)
      snprintf(linebuf, sizeof linebuf, "%s: %s → %s", key, oldv, newv);
    else if (has_a)
      snprintf(linebuf, sizeof linebuf, "%s: %s → (missing)", key, oldv);
    else
      snprintf(linebuf, sizeof linebuf, "%s: (missing) → %s", key, newv);
    li = strlen(linebuf);
    if (olen > 0) {
      if (olen + 1 >= sizeof r->str) break;
      r->str[olen++] = '\n';
      r->str[olen] = 0;
    }
    if (olen + li + 1 >= sizeof r->str) break;
    memcpy(r->str + olen, linebuf, li + 1);
    olen += li;
    count++;
  }
  r->n = count;
  return 0;
}

/* Usability: SYS JSONTOPKEY/JSONBOTKEY — dominant/min numeric key without TOKV+TOPKEY. */
int cubalc_host_json_topkey(const char *json, int want_min, cubalc_host_result *r) {
  cubalc_host_result keys, raw;
  const char *p, *line;
  char best_key[256];
  long best_v = 0;
  int found = 0;
  r_clear(r);
  best_key[0] = 0;
  memset(&keys, 0, sizeof keys);
  if (cubalc_host_json_keys(json, &keys) != 0) {
    r->str[0] = 0;
    r->n = 0;
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
    memset(&raw, 0, sizeof raw);
    if (cubalc_host_json_get_raw(json, key, &raw) != 0)
      continue;
    v = raw.str;
    while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
    if (*v == '"') {
      /* string — skip (not pure int) */
      continue;
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
    } else if (strncmp(v, "null", 4) == 0) {
      continue;
    } else if (*v == '{' || *v == '[') {
      continue;
    } else {
      num = strtol(v, &end, 10);
      if (end && end != v && *end == 0)
        is_num = 1;
    }
    if (!is_num) continue;
    if (!found || (want_min ? (num < best_v) : (num > best_v))) {
      snprintf(best_key, sizeof best_key, "%s", key);
      best_v = num;
      found = 1;
    }
  }
  if (!found) {
    r->str[0] = 0;
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  snprintf(r->str, sizeof r->str, "%s", best_key);
  r->n = best_v;
  r->ok = 1;
  return 0;
}
/* --- dotted path helpers (GETP "freq.error" / SETP "a.b" …) --- */
#define CUBALC_JSON_PATH_MAXSEG 8



