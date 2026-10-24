/* CubalC — hostops JSON core plane peel (B_hostops_margin).
 * json_get · json_keys · json_set · json_del (+ esc/skip/fmt helpers).
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

int cubalc_host_json_get(const char *json, const char *key, cubalc_host_result *r) {
  r_clear(r);
  if (!json || !key || !key[0]) {
    snprintf(r->err, sizeof r->err, "json: bad args");
    return -1;
  }
  char pat[96];
  snprintf(pat, sizeof pat, "\"%s\"", key);
  const char *p = json;
  int any = 0;
  char best[CUBALC_HOST_STR_MAX];
  best[0] = 0;
  while ((p = strstr(p, pat)) != NULL) {
    p += strlen(pat);
    while (*p == ' ' || *p == '\t') p++;
    if (*p != ':') continue;
    p++;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    char tmp[CUBALC_HOST_STR_MAX];
    size_t k = 0;
    tmp[0] = 0;
    if (*p == '"') {
      /* string value */
      p++;
      while (*p && k + 1 < sizeof tmp) {
        if (*p == '"') break;
        if (*p == '\\' && p[1]) {
          p++;
          if (*p == 'n') { tmp[k++] = '\n'; p++; continue; }
          if (*p == 't') { tmp[k++] = '\t'; p++; continue; }
          if (*p == 'r') { p++; continue; }
          if (*p == 'u') {
            p++;
            for (int i = 0; i < 4 && isxdigit((unsigned char)*p); i++) p++;
            tmp[k++] = '?';
            continue;
          }
          tmp[k++] = *p++;
          continue;
        }
        tmp[k++] = *p++;
      }
      tmp[k] = 0;
      if (*p == '"') p++;
    } else if (*p == '-' || isdigit((unsigned char)*p)) {
      /* number → string so SYS NUM / SETDIGIT can fold peer digits */
      if (*p == '-') tmp[k++] = *p++;
      while (*p && k + 1 < sizeof tmp &&
             (isdigit((unsigned char)*p) || *p == '.' || *p == 'e' || *p == 'E' ||
              *p == '+' || *p == '-')) {
        tmp[k++] = *p++;
      }
      tmp[k] = 0;
    } else if (strncmp(p, "true", 4) == 0) {
      snprintf(tmp, sizeof tmp, "1");
      k = 1;
      p += 4;
    } else if (strncmp(p, "false", 5) == 0) {
      snprintf(tmp, sizeof tmp, "0");
      k = 1;
      p += 5;
    } else if (strncmp(p, "null", 4) == 0) {
      tmp[0] = 0;
      k = 0;
      p += 4;
    } else {
      continue;
    }
    size_t nz = 0;
    for (size_t i = 0; i < k; i++) if ((unsigned char)tmp[i] > ' ') nz++;
    if (nz || k == 0) {
      /* last match wins when key repeats */
      snprintf(best, sizeof best, "%s", tmp);
      any = 1;
    }
  }
  if (!any) {
    snprintf(r->err, sizeof r->err, "json: no field %s", key);
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", best);
  r->n = (long)strlen(r->str);
  r->ok = 1;
  return 0;
}

/* Usability: SYS JSONKEYS [json] — top-level object keys as newline bag for agents
 * walking unknown plates without guessing field names. r->n = key count. */
int cubalc_host_json_keys(const char *json, cubalc_host_result *r) {
  size_t olen = 0;
  long kept = 0;
  int depth = 0, in_str = 0, esc = 0, saw_obj = 0;
  const char *p;
  r_clear(r);
  if (!json) {
    snprintf(r->err, sizeof r->err, "jsonkeys: empty");
    return -1;
  }
  /* skip leading whitespace */
  p = json;
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
  if (*p != '{') {
    snprintf(r->err, sizeof r->err, "jsonkeys: not an object");
    return -1;
  }
  saw_obj = 1;
  r->str[0] = 0;
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
      if (c == '"') {
        in_str = 0;
        /* if we just closed a key string at depth 1, check for : and push */
        /* handled below via key capture path */
      }
      continue;
    }
    if (c == '"') {
      if (depth == 1) {
        /* potential key: capture until closing quote, then require : */
        const char *ks = p + 1;
        const char *q = ks;
        char key[256];
        size_t kn = 0;
        int kesc = 0;
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
        if (*q != '"') {
          /* unclosed string — bail soft empty remainder */
          break;
        }
        key[kn] = 0;
        /* skip whitespace after key string */
        {
          const char *rcolon = q + 1;
          while (*rcolon == ' ' || *rcolon == '\t' || *rcolon == '\n' || *rcolon == '\r')
            rcolon++;
          if (*rcolon == ':') {
            cubalc_bag_push(r->str, sizeof r->str, &olen, &kept, key);
            /* advance p to the closing quote of the key; outer loop p++ then
             * continues into value which may open strings/objects */
            p = q; /* p++ at end of for → first char after key " */
            continue;
          }
        }
        /* not a key (string value etc.) — enter string mode from this " */
        in_str = 1;
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
  (void)saw_obj;
  r->n = kept;
  r->ok = 1;
  return 0;
}

/* Escape JSON string body into out (no outer quotes). Returns 0 ok. */
static int cubalc_json_esc_body(const char *in, char *out, size_t outn) {
  static const char *hx = "0123456789abcdef";
  size_t i, o = 0, n;
  if (!out || outn < 1) return -1;
  if (!in) in = "";
  n = strlen(in);
  for (i = 0; i < n; i++) {
    unsigned char c = (unsigned char)in[i];
    const char *rep = NULL;
    char tmp[8];
    size_t rlen;
    if (c == '"') rep = "\\\"";
    else if (c == '\\') rep = "\\\\";
    else if (c == '/') rep = "\\/";
    else if (c == '\n') rep = "\\n";
    else if (c == '\r') rep = "\\r";
    else if (c == '\t') rep = "\\t";
    else if (c < 0x20) {
      tmp[0] = '\\'; tmp[1] = 'u'; tmp[2] = '0'; tmp[3] = '0';
      tmp[4] = hx[c >> 4]; tmp[5] = hx[c & 0xf]; tmp[6] = 0;
      rep = tmp;
    }
    if (rep) {
      rlen = strlen(rep);
      if (o + rlen >= outn) return -1;
      memcpy(out + o, rep, rlen);
      o += rlen;
    } else {
      if (o + 1 >= outn) return -1;
      out[o++] = (char)c;
    }
  }
  out[o] = 0;
  return 0;
}

/* Skip one JSON value starting at *pp; advance *pp past it. Returns 0 ok. */
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

/* Format encoded value into valout. val_kind 0=string, 1=raw. */
static int cubalc_json_fmt_val(const char *val, int val_kind, char *valout, size_t vn) {
  char body[CUBALC_HOST_STR_MAX];
  if (!valout || vn < 2) return -1;
  if (!val) val = "";
  if (val_kind != 0) {
    if (strlen(val) >= vn) return -1;
    snprintf(valout, vn, "%s", val);
    return 0;
  }
  if (cubalc_json_esc_body(val, body, sizeof body) != 0) return -1;
  if (strlen(body) + 3 >= vn) return -1;
  snprintf(valout, vn, "\"%s\"", body);
  return 0;
}

/* Usability: SYS JSONSET plate key value — set/update top-level key for agent plates
 * without hand string rebuild. r->str=new object; r->n=1 if updated else 0 inserted. */
int cubalc_host_json_set(const char *json, const char *key, const char *val,
                         int val_kind, cubalc_host_result *r) {
  char encoded[CUBALC_HOST_STR_MAX];
  char keypat[320];
  const char *p, *base;
  size_t kn;
  int depth = 0, in_str = 0, esc = 0;
  r_clear(r);
  if (!key || !key[0]) {
    snprintf(r->err, sizeof r->err, "jsonset: empty key");
    return -1;
  }
  if (!val) val = "";
  if (cubalc_json_fmt_val(val, val_kind, encoded, sizeof encoded) != 0) {
    snprintf(r->err, sizeof r->err, "jsonset: value too long");
    return -1;
  }
  kn = strlen(key);
  if (kn + 3 >= sizeof keypat) {
    snprintf(r->err, sizeof r->err, "jsonset: key too long");
    return -1;
  }
  snprintf(keypat, sizeof keypat, "\"%s\"", key);

  base = json ? json : "";
  while (*base == ' ' || *base == '\t' || *base == '\n' || *base == '\r') base++;
  if (!*base || *base != '{') {
    /* fresh object */
    if (snprintf(r->str, sizeof r->str, "{%s:%s}", keypat, encoded) >= (int)sizeof r->str) {
      snprintf(r->err, sizeof r->err, "jsonset: overflow");
      return -1;
    }
    r->n = 0;
    r->ok = 1;
    return 0;
  }

  /* search top-level key */
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
          const char *vstart, *vend, *rest;
          size_t pre_n, mid_n, rest_n;
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
            snprintf(r->err, sizeof r->err, "jsonset: bad value");
            return -1;
          }
          rest = vend;
          pre_n = (size_t)(vstart - base);
          mid_n = strlen(encoded);
          rest_n = strlen(rest);
          if (pre_n + mid_n + rest_n + 1 >= sizeof r->str) {
            snprintf(r->err, sizeof r->err, "jsonset: overflow");
            return -1;
          }
          memcpy(r->str, base, pre_n);
          memcpy(r->str + pre_n, encoded, mid_n);
          memcpy(r->str + pre_n + mid_n, rest, rest_n + 1);
          r->n = 1;
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
      if (depth == 0) {
        /* insert before this closing brace */
        const char *scan = base + 1;
        int nonempty = 0;
        size_t pre_n, mid_n, rest_n;
        char mid[CUBALC_HOST_STR_MAX];
        while (*scan == ' ' || *scan == '\t' || *scan == '\n' || *scan == '\r') scan++;
        if (*scan && *scan != '}') nonempty = 1;
        if (nonempty)
          snprintf(mid, sizeof mid, ",%s:%s", keypat, encoded);
        else
          snprintf(mid, sizeof mid, "%s:%s", keypat, encoded);
        pre_n = (size_t)(p - base);
        mid_n = strlen(mid);
        rest_n = strlen(p);
        if (pre_n + mid_n + rest_n + 1 >= sizeof r->str) {
          snprintf(r->err, sizeof r->err, "jsonset: overflow");
          return -1;
        }
        memcpy(r->str, base, pre_n);
        memcpy(r->str + pre_n, mid, mid_n);
        memcpy(r->str + pre_n + mid_n, p, rest_n + 1);
        r->n = 0;
        r->ok = 1;
        return 0;
      }
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
  snprintf(r->err, sizeof r->err, "jsonset: no closing brace");
  return -1;
}

/* Usability: SYS JSONDEL plate key — drop top-level key without hand rebuild.
 * r->n=1 removed, 0 missing (OK still, LAST=copy). */
int cubalc_host_json_del(const char *json, const char *key, cubalc_host_result *r) {
  char keypat[320];
  const char *base, *p;
  int depth = 0, in_str = 0, esc = 0;
  r_clear(r);
  if (!key || !key[0]) {
    snprintf(r->err, sizeof r->err, "jsondel: empty key");
    return -1;
  }
  if (strlen(key) + 3 >= sizeof keypat) {
    snprintf(r->err, sizeof r->err, "jsondel: key too long");
    return -1;
  }
  snprintf(keypat, sizeof keypat, "\"%s\"", key);
  base = json ? json : "";
  while (*base == ' ' || *base == '\t' || *base == '\n' || *base == '\r') base++;
  if (!*base || *base != '{') {
    /* not an object — return empty object soft */
    snprintf(r->str, sizeof r->str, "%s", "{}");
    r->n = 0;
    r->ok = 1;
    return 0;
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
          const char *del_lo, *del_hi;
          const char *scan;
          size_t pre_n, rest_n;
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
            snprintf(r->err, sizeof r->err, "jsondel: bad value");
            return -1;
          }
          /* default delete ["key": value] */
          del_lo = p;
          del_hi = vend;
          /* prefer include preceding comma */
          scan = p;
          while (scan > base &&
                 (scan[-1] == ' ' || scan[-1] == '\t' || scan[-1] == '\n' ||
                  scan[-1] == '\r'))
            scan--;
          if (scan > base && scan[-1] == ',') {
            del_lo = scan - 1;
          } else {
            /* first field: drop trailing comma after value if present */
            scan = vend;
            while (*scan == ' ' || *scan == '\t' || *scan == '\n' || *scan == '\r')
              scan++;
            if (*scan == ',')
              del_hi = scan + 1;
          }
          pre_n = (size_t)(del_lo - base);
          rest_n = strlen(del_hi);
          if (pre_n + rest_n + 1 >= sizeof r->str) {
            snprintf(r->err, sizeof r->err, "jsondel: overflow");
            return -1;
          }
          memcpy(r->str, base, pre_n);
          memcpy(r->str + pre_n, del_hi, rest_n + 1);
          r->n = 1;
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
  /* key missing — copy original */
  snprintf(r->str, sizeof r->str, "%s", base);
  r->n = 0;
  r->ok = 1;
  return 0;
}

