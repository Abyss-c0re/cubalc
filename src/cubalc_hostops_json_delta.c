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

/* Peeled from cubalc_hostops.c (G_hostops_margin mesh vote) — drop·values·from_kv. */

/* Usability: SYS JSONDROP plate keys — drop listed top-level keys (bulk JSONDEL).
 * keys_nl = newline bag. Missing soft-skipped. r->n = removed count. */
int cubalc_host_json_drop(const char *json, const char *keys_nl, cubalc_host_result *r) {
  char cur[CUBALC_HOST_STR_MAX];
  const char *p, *line, *j;
  long removed = 0;
  cubalc_host_result dr;
  r_clear(r);
  j = json ? json : "";
  while (*j == ' ' || *j == '\t' || *j == '\n' || *j == '\r') j++;
  if (!*j || *j != '{')
    snprintf(cur, sizeof cur, "%s", "{}");
  else
    snprintf(cur, sizeof cur, "%s", j);
  if (!keys_nl || !keys_nl[0]) {
    snprintf(r->str, sizeof r->str, "%s", cur);
    r->n = 0;
    r->ok = 1;
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
    while (kn > 0 && (key[kn - 1] == ' ' || key[kn - 1] == '\t'))
      key[--kn] = 0;
    {
      char *s = key;
      while (*s == ' ' || *s == '\t') s++;
      if (s != key) {
        size_t n = strlen(s);
        memmove(key, s, n + 1);
        kn = n;
      }
    }
    if (!key[0]) continue;
    memset(&dr, 0, sizeof dr);
    if (cubalc_host_json_del(cur, key, &dr) != 0) {
      snprintf(r->err, sizeof r->err, "%s",
               dr.err[0] ? dr.err : "jsondrop: del fail");
      return -1;
    }
    if (dr.n > 0)
      removed++;
    snprintf(cur, sizeof cur, "%s", dr.str);
  }
  snprintf(r->str, sizeof r->str, "%s", cur);
  r->n = removed;
  r->ok = 1;
  return 0;
}

/* Usability: SYS JSONVALUES plate — top-level values bag in JSONKEYS order.
 * Scalars decoded like JSON peel; nested obj/arr kept as raw text for agents. */
int cubalc_host_json_values(const char *json, cubalc_host_result *r) {
  cubalc_host_result keys, gr, raw;
  const char *p, *line;
  size_t olen = 0;
  long kept = 0;
  r_clear(r);
  memset(&keys, 0, sizeof keys);
  if (cubalc_host_json_keys(json, &keys) != 0) {
    /* not an object — empty bag soft */
    r->str[0] = 0;
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  r->str[0] = 0;
  p = keys.str;
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
    memset(&raw, 0, sizeof raw);
    if (cubalc_host_json_get_raw(json, key, &raw) != 0)
      continue;
    {
      const char *v = raw.str;
      while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
      if (*v == '"') {
        /* decoded string body via get */
        memset(&gr, 0, sizeof gr);
        if (cubalc_host_json_get(json, key, &gr) == 0)
          cubalc_bag_push(r->str, sizeof r->str, &olen, &kept, gr.str);
        else
          cubalc_bag_push(r->str, sizeof r->str, &olen, &kept, raw.str);
      } else if (strncmp(v, "true", 4) == 0) {
        cubalc_bag_push(r->str, sizeof r->str, &olen, &kept, "1");
      } else if (strncmp(v, "false", 5) == 0) {
        cubalc_bag_push(r->str, sizeof r->str, &olen, &kept, "0");
      } else if (strncmp(v, "null", 4) == 0) {
        /* bag_push skips empty; keep explicit null token for field alignment */
        cubalc_bag_push(r->str, sizeof r->str, &olen, &kept, "null");
      } else {
        /* num, obj, arr — raw */
        cubalc_bag_push(r->str, sizeof r->str, &olen, &kept, v);
      }
    }
  }
  r->n = kept;
  r->ok = 1;
  return 0;
}

/* Usability: SYS JSONFROMKV bag — key=val / key:val lines → JSON object plate.
 * First '=' or ':' per line is sep (prefer earlier). r->n = pairs set. */
int cubalc_host_json_from_kv(const char *bag, cubalc_host_result *r) {
  char cur[CUBALC_HOST_STR_MAX];
  const char *p, *line;
  long applied = 0;
  cubalc_host_result hr;
  r_clear(r);
  snprintf(cur, sizeof cur, "%s", "{}");
  if (!bag || !bag[0]) {
    snprintf(r->str, sizeof r->str, "%s", "{}");
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  p = bag;
  while (*p) {
    char key[256], val[CUBALC_HOST_STR_MAX];
    size_t kn = 0, vn = 0;
    const char *sep, *eq, *col, *vstart, *vend;
    int val_kind = 0;
    char *end = NULL;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    /* find sep */
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
      continue; /* skip bad line */
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
    /* classify value */
    if (!val[0]) {
      val_kind = 0; /* empty string */
    } else if (strcmp(val, "true") == 0 || strcmp(val, "false") == 0 ||
               strcmp(val, "null") == 0) {
      val_kind = 1;
    } else {
      (void)strtol(val, &end, 10);
      if (end && end != val && *end == 0)
        val_kind = 1; /* pure integer */
      else {
        /* allow simple decimals */
        char *ed = NULL;
        (void)strtod(val, &ed);
        if (ed && ed != val && *ed == 0)
          val_kind = 1;
        else
          val_kind = 0;
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_set(cur, key, val, val_kind, &hr) != 0) {
      snprintf(r->err, sizeof r->err, "%s",
               hr.err[0] ? hr.err : "jsonfromkv: set fail");
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


/* PRETTYP: 2-space indented JSON for human/agent plate reads. See header. */
int cubalc_host_json_pretty(const char *json, cubalc_host_result *r) {
  const char *p;
  size_t o = 0;
  int depth = 0, in_str = 0, esc = 0, truncated = 0;
  size_t cap;

  r_clear(r);
  r->ok = 1;
  r->str[0] = 0;
  p = json ? json : "";
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
  if (!*p) {
    snprintf(r->str, sizeof r->str, "{}");
    r->n = 2;
    r->code = 1;
    return 0;
  }
  cap = sizeof r->str - 1;
  while (*p) {
    char c = *p++;
    if (o + 16 >= cap) {
      truncated = 1;
      break;
    }
    if (in_str) {
      r->str[o++] = c;
      if (esc) esc = 0;
      else if (c == '\\') esc = 1;
      else if (c == '"') in_str = 0;
      continue;
    }
    if (c == '"') {
      r->str[o++] = c;
      in_str = 1;
      continue;
    }
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
      continue;
    if (c == '{' || c == '[') {
      const char *q = p;
      r->str[o++] = c;
      while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r') q++;
      if ((c == '{' && *q == '}') || (c == '[' && *q == ']')) {
        r->str[o++] = *q;
        p = q + 1;
        continue;
      }
      depth++;
      r->str[o++] = '\n';
      {
        int i;
        for (i = 0; i < depth && o + 2 < cap; i++) {
          r->str[o++] = ' ';
          r->str[o++] = ' ';
        }
      }
      continue;
    }
    if (c == '}' || c == ']') {
      if (depth > 0) depth--;
      r->str[o++] = '\n';
      {
        int i;
        for (i = 0; i < depth && o + 2 < cap; i++) {
          r->str[o++] = ' ';
          r->str[o++] = ' ';
        }
      }
      if (o < cap) r->str[o++] = c;
      continue;
    }
    if (c == ',') {
      r->str[o++] = c;
      r->str[o++] = '\n';
      {
        int i;
        for (i = 0; i < depth && o + 2 < cap; i++) {
          r->str[o++] = ' ';
          r->str[o++] = ' ';
        }
      }
      continue;
    }
    if (c == ':') {
      r->str[o++] = c;
      r->str[o++] = ' ';
      continue;
    }
    r->str[o++] = c;
  }
  r->str[o] = 0;
  r->n = (long)o;
  r->code = truncated ? 0 : 1;
  if (truncated)
    snprintf(r->err, sizeof r->err, "truncated");
  return 0;
}

/* Usability: swap two keys (paths ok). One-sided miss moves value; both miss n=0. */
int cubalc_host_json_swap_keys(const char *json, const char *a, const char *b,
                               cubalc_host_result *r) {
  char base[CUBALC_HOST_STR_MAX];
  char ra[CUBALC_HOST_STR_MAX], rb[CUBALC_HOST_STR_MAX];
  cubalc_host_result gr, hr, dr;
  const char *j;
  int has_a = 0, has_b = 0;
  r_clear(r);
  if (!a || !a[0] || !b || !b[0]) {
    snprintf(r->err, sizeof r->err, "jsonswap: empty key");
    return -1;
  }
  j = json ? json : "";
  while (*j == ' ' || *j == '\t' || *j == '\n' || *j == '\r') j++;
  if (!*j || *j != '{')
    snprintf(base, sizeof base, "%s", "{}");
  else
    snprintf(base, sizeof base, "%s", j);

  if (strcmp(a, b) == 0) {
    memset(&gr, 0, sizeof gr);
    has_a = (cubalc_host_json_path_get_raw(base, a, &gr) == 0) ? 1 : 0;
    snprintf(r->str, sizeof r->str, "%s", base);
    r->n = has_a ? 1 : 0;
    r->ok = 1;
    return 0;
  }

  ra[0] = 0;
  rb[0] = 0;
  memset(&gr, 0, sizeof gr);
  if (cubalc_host_json_path_get_raw(base, a, &gr) == 0) {
    has_a = 1;
    snprintf(ra, sizeof ra, "%s", gr.str);
  }
  memset(&gr, 0, sizeof gr);
  if (cubalc_host_json_path_get_raw(base, b, &gr) == 0) {
    has_b = 1;
    snprintf(rb, sizeof rb, "%s", gr.str);
  }
  if (!has_a && !has_b) {
    snprintf(r->str, sizeof r->str, "%s", base);
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  if (has_a && has_b) {
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_path_set(base, a, rb, 1, &hr) != 0) {
      snprintf(r->err, sizeof r->err, "%s",
               hr.err[0] ? hr.err : "jsonswap: set a fail");
      return -1;
    }
    snprintf(base, sizeof base, "%s", hr.str);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_path_set(base, b, ra, 1, &hr) != 0) {
      snprintf(r->err, sizeof r->err, "%s",
               hr.err[0] ? hr.err : "jsonswap: set b fail");
      return -1;
    }
    snprintf(r->str, sizeof r->str, "%s", hr.str);
    r->n = 1;
    r->ok = 1;
    return 0;
  }
  if (has_a && !has_b) {
    memset(&dr, 0, sizeof dr);
    if (cubalc_host_json_path_del(base, a, &dr) != 0) {
      snprintf(r->err, sizeof r->err, "%s",
               dr.err[0] ? dr.err : "jsonswap: del a fail");
      return -1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_path_set(dr.str, b, ra, 1, &hr) != 0) {
      snprintf(r->err, sizeof r->err, "%s",
               hr.err[0] ? hr.err : "jsonswap: set b fail");
      return -1;
    }
    snprintf(r->str, sizeof r->str, "%s", hr.str);
    r->n = 1;
    r->ok = 1;
    return 0;
  }
  /* !has_a && has_b */
  memset(&dr, 0, sizeof dr);
  if (cubalc_host_json_path_del(base, b, &dr) != 0) {
    snprintf(r->err, sizeof r->err, "%s",
             dr.err[0] ? dr.err : "jsonswap: del b fail");
    return -1;
  }
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_json_path_set(dr.str, a, rb, 1, &hr) != 0) {
    snprintf(r->err, sizeof r->err, "%s",
             hr.err[0] ? hr.err : "jsonswap: set a fail");
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", hr.str);
  r->n = 1;
  r->ok = 1;
  return 0;
}

/* Usability: copy key src→dst (paths ok) — dual of rename keep-src.
 * Soft miss src → plate unchanged n=0. Dest overwrites. Same path no-op n=1. */
int cubalc_host_json_copy_key(const char *json, const char *src, const char *dst,
                              cubalc_host_result *r) {
  char base[CUBALC_HOST_STR_MAX];
  char raw[CUBALC_HOST_STR_MAX];
  cubalc_host_result gr, hr;
  const char *j;
  r_clear(r);
  if (!src || !src[0] || !dst || !dst[0]) {
    snprintf(r->err, sizeof r->err, "jsoncopy: empty key");
    return -1;
  }
  j = json ? json : "";
  while (*j == ' ' || *j == '\t' || *j == '\n' || *j == '\r') j++;
  if (!*j || *j != '{')
    snprintf(base, sizeof base, "%s", "{}");
  else
    snprintf(base, sizeof base, "%s", j);

  memset(&gr, 0, sizeof gr);
  if (cubalc_host_json_path_get_raw(base, src, &gr) != 0) {
    snprintf(r->str, sizeof r->str, "%s", base);
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  snprintf(raw, sizeof raw, "%s", gr.str);
  if (strcmp(src, dst) == 0) {
    snprintf(r->str, sizeof r->str, "%s", base);
    r->n = 1;
    r->ok = 1;
    return 0;
  }
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_json_path_set(base, dst, raw, 1, &hr) != 0) {
    snprintf(r->err, sizeof r->err, "%s",
             hr.err[0] ? hr.err : "jsoncopy: set fail");
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", hr.str);
  r->n = 1;
  r->ok = 1;
  return 0;
}


int cubalc_host_json_pick(const char *json, const char *keys_nl, cubalc_host_result *r) {
  char cur[CUBALC_HOST_STR_MAX];
  const char *p, *line;
  long kept = 0;
  cubalc_host_result gr, hr;
  r_clear(r);
  snprintf(cur, sizeof cur, "%s", "{}");
  if (!keys_nl || !keys_nl[0]) {
    snprintf(r->str, sizeof r->str, "%s", "{}");
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  p = keys_nl;
  while (*p) {
    char key[256];
    size_t kn = 0;
    /* skip empty lines */
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    kn = (size_t)(p - line);
    if (kn >= sizeof key) kn = sizeof key - 1;
    memcpy(key, line, kn);
    key[kn] = 0;
    /* trim trailing spaces */
    while (kn > 0 && (key[kn - 1] == ' ' || key[kn - 1] == '\t')) {
      key[--kn] = 0;
    }
    /* trim leading spaces */
    {
      char *s = key;
      while (*s == ' ' || *s == '\t') s++;
      if (s != key) {
        size_t n = strlen(s);
        memmove(key, s, n + 1);
        kn = n;
      }
    }
    if (!key[0]) continue;
    memset(&gr, 0, sizeof gr);
    if (cubalc_host_json_get_raw(json, key, &gr) != 0)
      continue; /* missing soft skip */
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_set(cur, key, gr.str, 1, &hr) != 0) {
      snprintf(r->err, sizeof r->err, "%s",
               hr.err[0] ? hr.err : "jsonpick: set fail");
      return -1;
    }
    snprintf(cur, sizeof cur, "%s", hr.str);
    kept++;
  }
  snprintf(r->str, sizeof r->str, "%s", cur);
  r->n = kept;
  r->ok = 1;
  return 0;
}

/* Usability: SYS JSONRENAME plate old new — rename top-level key, keep raw value.
 * Soft miss (old absent) → copy plate, r->n=0. Dest key overwritten if present. */
int cubalc_host_json_rename(const char *json, const char *oldk, const char *newk,
                            cubalc_host_result *r) {
  char base[CUBALC_HOST_STR_MAX];
  char raw[CUBALC_HOST_STR_MAX];
  cubalc_host_result gr, hr, dr;
  const char *j;
  r_clear(r);
  if (!oldk || !oldk[0] || !newk || !newk[0]) {
    snprintf(r->err, sizeof r->err, "jsonrename: empty key");
    return -1;
  }
  j = json ? json : "";
  while (*j == ' ' || *j == '\t' || *j == '\n' || *j == '\r') j++;
  if (!*j || *j != '{')
    snprintf(base, sizeof base, "%s", "{}");
  else
    snprintf(base, sizeof base, "%s", j);

  /* path-aware: dotted/slash keys use path_get/del/set (shallow falls through). */
  memset(&gr, 0, sizeof gr);
  if (cubalc_host_json_path_get_raw(base, oldk, &gr) != 0) {
    /* missing — soft copy */
    snprintf(r->str, sizeof r->str, "%s", base);
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  snprintf(raw, sizeof raw, "%s", gr.str);
  if (strcmp(oldk, newk) == 0) {
    snprintf(r->str, sizeof r->str, "%s", base);
    r->n = 1;
    r->ok = 1;
    return 0;
  }
  /* drop old, then write new with raw value (overwrites dest if any) */
  memset(&dr, 0, sizeof dr);
  if (cubalc_host_json_path_del(base, oldk, &dr) != 0) {
    snprintf(r->err, sizeof r->err, "%s",
             dr.err[0] ? dr.err : "jsonrename: del fail");
    return -1;
  }
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_json_path_set(dr.str, newk, raw, 1, &hr) != 0) {
    snprintf(r->err, sizeof r->err, "%s",
             hr.err[0] ? hr.err : "jsonrename: set fail");
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", hr.str);
  r->n = 1;
  r->ok = 1;
  return 0;
}

/* Usability: SYS JSONDEFAULTS plate defaults — fill missing keys only (no clobber).
 * Dual of JSONMERGE for agent boot: live plate fields win; defaults fill gaps. */
int cubalc_host_json_defaults(const char *plate, const char *defaults,
                              cubalc_host_result *r) {
  cubalc_host_result kd, gr, setr;
  char cur[CUBALC_HOST_STR_MAX];
  const char *p, *line;
  long applied = 0, had = 0;
  r_clear(r);
  r->ok = 1;
  /* start from plate object or empty */
  {
    const char *b = plate ? plate : "";
    while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
    if (*b == '{')
      snprintf(cur, sizeof cur, "%s", b);
    else
      snprintf(cur, sizeof cur, "%s", "{}");
  }
  memset(&kd, 0, sizeof kd);
  if (cubalc_host_json_keys(defaults, &kd) != 0 || kd.n == 0) {
    snprintf(r->str, sizeof r->str, "%s", cur);
    r->n = 0;
    r->code = 0;
    return 0;
  }
  p = kd.str;
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
    memset(&gr, 0, sizeof gr);
    /* present → keep (plate wins) */
    if (cubalc_host_json_get_raw(cur, key, &gr) == 0) {
      had++;
      continue;
    }
    memset(&gr, 0, sizeof gr);
    if (cubalc_host_json_get_raw(defaults, key, &gr) != 0)
      continue;
    memset(&setr, 0, sizeof setr);
    if (cubalc_host_json_set(cur, key, gr.str, 1, &setr) != 0) {
      snprintf(r->err, sizeof r->err, "%s",
               setr.err[0] ? setr.err : "jsondefaults: set fail");
      return -1;
    }
    snprintf(cur, sizeof cur, "%s", setr.str);
    applied++;
  }
  snprintf(r->str, sizeof r->str, "%s", cur);
  r->n = applied;
  r->code = (int)had;
  return 0;
}


/* Usability: SYS JSONMERGE base overlay — overlay top-level keys win.
 * Preserves raw value text (numbers/bools/nested). r->n = applied key count. */
int cubalc_host_json_merge(const char *base, const char *overlay, cubalc_host_result *r) {
  char cur[CUBALC_HOST_STR_MAX];
  const char *ob, *p;
  int depth = 0, in_str = 0, esc = 0;
  long applied = 0;
  cubalc_host_result hr;
  r_clear(r);
  /* start from base object or empty */
  {
    const char *b = base ? base : "";
    while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
    if (*b == '{')
      snprintf(cur, sizeof cur, "%s", b);
    else
      snprintf(cur, sizeof cur, "%s", "{}");
  }
  ob = overlay ? overlay : "";
  while (*ob == ' ' || *ob == '\t' || *ob == '\n' || *ob == '\r') ob++;
  if (*ob != '{') {
    /* no overlay object — return base copy */
    snprintf(r->str, sizeof r->str, "%s", cur);
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  p = ob;
  depth = 0;
  in_str = 0;
  esc = 0;
  for (; *p; p++) {
    unsigned char c = (unsigned char)*p;
    if (in_str) {
      if (esc) {
        esc = 0;
        continue;
      }
      if (c == '\\') {
        esc = 1;
        continue;
      }
      if (c == '"') in_str = 0;
      continue;
    }
    if (c == '"') {
      if (depth == 1) {
        /* capture key */
        const char *ks = p + 1, *q = ks;
        char key[256];
        char rawval[CUBALC_HOST_STR_MAX];
        size_t kn = 0;
        int kesc = 0;
        const char *after, *vstart, *vend;
        size_t vn;
        while (*q) {
          if (kesc) {
            kesc = 0;
            if (kn + 1 < sizeof key) key[kn++] = *q;
            q++;
            continue;
          }
          if (*q == '\\') {
            kesc = 1;
            q++;
            continue;
          }
          if (*q == '"') break;
          if (kn + 1 < sizeof key) key[kn++] = *q;
          q++;
        }
        if (*q != '"') break;
        key[kn] = 0;
        after = q + 1;
        while (*after == ' ' || *after == '\t' || *after == '\n' || *after == '\r')
          after++;
        if (*after != ':') {
          in_str = 1;
          continue;
        }
        after++;
        while (*after == ' ' || *after == '\t' || *after == '\n' || *after == '\r')
          after++;
        vstart = after;
        vend = vstart;
        if (cubalc_json_skip_value(&vend) != 0) {
          snprintf(r->err, sizeof r->err, "jsonmerge: bad value");
          return -1;
        }
        vn = (size_t)(vend - vstart);
        if (vn >= sizeof rawval) vn = sizeof rawval - 1;
        memcpy(rawval, vstart, vn);
        rawval[vn] = 0;
        memset(&hr, 0, sizeof hr);
        if (cubalc_host_json_set(cur, key, rawval, 1, &hr) != 0) {
          snprintf(r->err, sizeof r->err, "%s",
                   hr.err[0] ? hr.err : "jsonmerge: set fail");
          return -1;
        }
        snprintf(cur, sizeof cur, "%s", hr.str);
        applied++;
        p = vend - 1; /* for loop p++ → vend */
        continue;
      }
      in_str = 1;
      continue;
    }
    if (c == '{') {
      depth++;
      continue;
    }
    if (c == '}') {
      if (depth > 0) depth--;
      if (depth == 0) break;
      continue;
    }
    if (c == '[') {
      depth++;
      continue;
    }
    if (c == ']') {
      if (depth > 0) depth--;
      continue;
    }
  }
  snprintf(r->str, sizeof r->str, "%s", cur);
  r->n = applied;
  r->ok = 1;
  return 0;
}

/* Usability: raw top-level value for key (preserves quotes/nested/true/null).
 * Soft miss → -1. Used by JSONPICK / agents that must not re-encode peels. */
int cubalc_host_json_get_raw(const char *json, const char *key, cubalc_host_result *r) {
  char keypat[320];
  const char *base, *p;
  int depth = 0, in_str = 0, esc = 0;
  r_clear(r);
  if (!key || !key[0]) {
    snprintf(r->err, sizeof r->err, "jsonraw: empty key");
    return -1;
  }
  if (strlen(key) + 3 >= sizeof keypat) {
    snprintf(r->err, sizeof r->err, "jsonraw: key too long");
    return -1;
  }
  snprintf(keypat, sizeof keypat, "\"%s\"", key);
  base = json ? json : "";
  while (*base == ' ' || *base == '\t' || *base == '\n' || *base == '\r') base++;
  if (!*base || *base != '{') {
    snprintf(r->err, sizeof r->err, "jsonraw: not an object");
    return -1;
  }
  p = base;
  for (; *p; p++) {
    unsigned char c = (unsigned char)*p;
    if (in_str) {
      if (esc) {
        esc = 0;
        continue;
      }
      if (c == '\\') {
        esc = 1;
        continue;
      }
      if (c == '"') in_str = 0;
      continue;
    }
    if (c == '"') {
      if (depth == 1) {
        size_t plen = strlen(keypat);
        if (strncmp(p, keypat, plen) == 0) {
          const char *after = p + plen;
          const char *vstart, *vend;
          size_t vn;
          while (*after == ' ' || *after == '\t' || *after == '\n' || *after == '\r')
            after++;
          if (*after != ':') {
            in_str = 1;
            continue;
          }
          after++;
          while (*after == ' ' || *after == '\t' || *after == '\n' || *after == '\r')
            after++;
          vstart = after;
          vend = vstart;
          if (cubalc_json_skip_value(&vend) != 0) {
            snprintf(r->err, sizeof r->err, "jsonraw: bad value");
            return -1;
          }
          vn = (size_t)(vend - vstart);
          if (vn + 1 >= sizeof r->str) {
            snprintf(r->err, sizeof r->err, "jsonraw: overflow");
            return -1;
          }
          memcpy(r->str, vstart, vn);
          r->str[vn] = 0;
          r->n = (long)vn;
          r->ok = 1;
          return 0;
        }
      }
      in_str = 1;
      continue;
    }
    if (c == '{') {
      depth++;
      continue;
    }
    if (c == '}') {
      if (depth > 0) depth--;
      if (depth == 0) break;
      continue;
    }
    if (c == '[') {
      depth++;
      continue;
    }
    if (c == ']') {
      if (depth > 0) depth--;
      continue;
    }
  }
  snprintf(r->err, sizeof r->err, "jsonraw: no field %s", key);
  return -1;
}


/* Set leaf along path; create missing intermediate objects as {}.
 * r->str = new root plate · r->n from leaf set · r->code = path depth. */
int cubalc_host_json_path_set(const char *json, const char *path, const char *val,
                              int val_kind, cubalc_host_result *r) {
  char segs[CUBALC_JSON_PATH_MAXSEG][96];
  char objs[CUBALC_JSON_PATH_MAXSEG][CUBALC_HOST_STR_MAX];
  cubalc_host_result gr, wr;
  int n, i;
  const char *v;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "jsonpath: empty path");
    return -1;
  }
  if (!cubalc_json_path_is_deep(path))
    return cubalc_host_json_set(json, path, val, val_kind, r);
  n = cubalc_json_path_split(path, segs, CUBALC_JSON_PATH_MAXSEG);
  if (n < 1) {
    snprintf(r->err, sizeof r->err, "jsonpath: bad path");
    return -1;
  }
  /* objs[0] = root; objs[i+1] = object under segs[i] */
  snprintf(objs[0], sizeof objs[0], "%s", json && json[0] ? json : "{}");
  {
    const char *b = objs[0];
    while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
    if (*b != '{')
      snprintf(objs[0], sizeof objs[0], "%s", "{}");
  }
  for (i = 0; i < n - 1; i++) {
    memset(&gr, 0, sizeof gr);
    if (cubalc_host_json_get_raw(objs[i], segs[i], &gr) == 0) {
      v = gr.str;
      while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
      if (*v == '{') {
        if (v != gr.str) {
          size_t m = strlen(v);
          memmove(gr.str, v, m + 1);
        }
        snprintf(objs[i + 1], sizeof objs[i + 1], "%s", gr.str);
      } else {
        /* non-object intermediate: replace with {} */
        snprintf(objs[i + 1], sizeof objs[i + 1], "%s", "{}");
      }
    } else {
      snprintf(objs[i + 1], sizeof objs[i + 1], "%s", "{}");
    }
  }
  /* set leaf on deepest object */
  memset(&wr, 0, sizeof wr);
  if (cubalc_host_json_set(objs[n - 1], segs[n - 1], val ? val : "", val_kind, &wr) != 0) {
    snprintf(r->err, sizeof r->err, "%s", wr.err[0] ? wr.err : "jsonpath: leaf set fail");
    return -1;
  }
  snprintf(objs[n - 1], sizeof objs[n - 1], "%s", wr.str);
  r->n = wr.n;
  /* bubble raw objects up to root */
  for (i = n - 2; i >= 0; i--) {
    memset(&wr, 0, sizeof wr);
    if (cubalc_host_json_set(objs[i], segs[i], objs[i + 1], 1, &wr) != 0) {
      snprintf(r->err, sizeof r->err, "%s", wr.err[0] ? wr.err : "jsonpath: nest set fail");
      return -1;
    }
    snprintf(objs[i], sizeof objs[i], "%s", wr.str);
  }
  snprintf(r->str, sizeof r->str, "%s", objs[0]);
  r->code = n;
  r->ok = 1;
  return 0;
}

/* Delete leaf along path; soft miss (r->n=0, root copy). Intermediate miss → n=0. */
int cubalc_host_json_path_del(const char *json, const char *path, cubalc_host_result *r) {
  char segs[CUBALC_JSON_PATH_MAXSEG][96];
  char objs[CUBALC_JSON_PATH_MAXSEG][CUBALC_HOST_STR_MAX];
  cubalc_host_result gr, wr;
  int n, i;
  const char *v;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "jsonpath: empty path");
    return -1;
  }
  if (!cubalc_json_path_is_deep(path))
    return cubalc_host_json_del(json, path, r);
  n = cubalc_json_path_split(path, segs, CUBALC_JSON_PATH_MAXSEG);
  if (n < 1) {
    snprintf(r->err, sizeof r->err, "jsonpath: bad path");
    return -1;
  }
  snprintf(objs[0], sizeof objs[0], "%s", json && json[0] ? json : "{}");
  {
    const char *b = objs[0];
    while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
    if (*b != '{') {
      snprintf(r->str, sizeof r->str, "%s", "{}");
      r->n = 0;
      r->ok = 1;
      return 0;
    }
  }
  for (i = 0; i < n - 1; i++) {
    memset(&gr, 0, sizeof gr);
    if (cubalc_host_json_get_raw(objs[i], segs[i], &gr) != 0) {
      snprintf(r->str, sizeof r->str, "%s", objs[0]);
      r->n = 0;
      r->ok = 1;
      return 0;
    }
    v = gr.str;
    while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
    if (*v != '{') {
      snprintf(r->str, sizeof r->str, "%s", objs[0]);
      r->n = 0;
      r->ok = 1;
      return 0;
    }
    if (v != gr.str) {
      size_t m = strlen(v);
      memmove(gr.str, v, m + 1);
    }
    snprintf(objs[i + 1], sizeof objs[i + 1], "%s", gr.str);
  }
  memset(&wr, 0, sizeof wr);
  if (cubalc_host_json_del(objs[n - 1], segs[n - 1], &wr) != 0) {
    snprintf(r->err, sizeof r->err, "%s", wr.err[0] ? wr.err : "jsonpath: leaf del fail");
    return -1;
  }
  if (wr.n == 0) {
    snprintf(r->str, sizeof r->str, "%s", objs[0]);
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  snprintf(objs[n - 1], sizeof objs[n - 1], "%s", wr.str);
  r->n = 1;
  for (i = n - 2; i >= 0; i--) {
    memset(&wr, 0, sizeof wr);
    if (cubalc_host_json_set(objs[i], segs[i], objs[i + 1], 1, &wr) != 0) {
      snprintf(r->err, sizeof r->err, "%s", wr.err[0] ? wr.err : "jsonpath: nest set fail");
      return -1;
    }
    snprintf(objs[i], sizeof objs[i], "%s", wr.str);
  }
  snprintf(r->str, sizeof r->str, "%s", objs[0]);
  r->code = n;
  r->ok = 1;
  return 0;
}

/* FIRSTUNEQFLAT: first matching leaf that diverges from the first match value.
 * Hit:  r->code=1 · r->str=path · r->n=0-based match index · r->err="ref\\nval\\ntotal"
 * Miss: r->code=0 · r->str="" · r->n=0 · r->err="total" decimal (match count).
 * Soft always OK. Read-only. */
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
