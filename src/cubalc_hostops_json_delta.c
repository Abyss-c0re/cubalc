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

/* Peeled from cubalc_hostops.c (G_hostops_margin mesh vote) — drop·values·from_kv. */

/* Usability: SYS JSONDROP plate keys — drop listed top-level keys (bulk JSONDEL).
 * keys_nl = newline bag. Missing soft-skipped. r->n = removed count. */
