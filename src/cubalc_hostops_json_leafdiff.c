/* CubalC — hostops JSON leaf diff/grep/filter/merge peel (B_hostops_margin).
 * leaf_diff · leaf_grep · leaf_filter_write · leaf_merge.
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

/* True if path appears as a whole line in a newline bag (no colon required). */
static int cubalc_pathbag_has(const char *bag, const char *path) {
  const char *sp;
  size_t kn;
  if (!bag || !path || !path[0]) return 0;
  kn = strlen(path);
  sp = bag;
  while (*sp) {
    size_t sn = 0;
    while (*sp == '\n' || *sp == '\r') sp++;
    if (!*sp) break;
    while (sp[sn] && sp[sn] != '\n' && sp[sn] != '\r') sn++;
    if (sn == kn && memcmp(sp, path, kn) == 0) return 1;
    sp += sn;
  }
  return 0;
}

/* Lookup path in leaf_kv bag (path:val lines). Returns 1 hit, 0 miss. */
static int cubalc_flatbag_get(const char *bag, const char *path,
                              char *val, size_t vcap) {
  const char *p, *line;
  size_t pl;
  if (!bag || !path || !path[0] || !val || vcap < 1) return 0;
  pl = strlen(path);
  val[0] = 0;
  p = bag;
  while (*p) {
    size_t kn = 0;
    const char *sep;
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
    if (!sep) continue;
    kn = (size_t)(sep - line);
    while (kn > 0 && (line[kn - 1] == ' ' || line[kn - 1] == '\t')) kn--;
    {
      size_t sk = 0;
      while (sk < kn && (line[sk] == ' ' || line[sk] == '\t')) sk++;
      if (sk) {
        line += sk;
        kn -= sk;
      }
    }
    if (kn != pl || memcmp(line, path, pl) != 0) continue;
    {
      const char *vs = sep + 1, *ve = p;
      size_t vn;
      while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
      while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
      vn = (size_t)(ve - vs);
      if (vn >= vcap) vn = vcap - 1;
      memcpy(val, vs, vn);
      val[vn] = 0;
    }
    return 1;
  }
  return 0;
}

/* Deep leaf-path diff / changelog. See header. */
int cubalc_host_json_leaf_diff(const char *a, const char *b, int paths_only,
                               cubalc_host_result *r) {
  cubalc_host_result ka, kb;
  const char *p, *line;
  size_t olen = 0;
  long count = 0;
  char seen[CUBALC_HOST_STR_MAX];
  size_t seen_olen = 0;
  long seen_n = 0;

  r_clear(r);
  r->str[0] = 0;
  r->ok = 1;
  r->n = 0;
  seen[0] = 0;
  memset(&ka, 0, sizeof ka);
  memset(&kb, 0, sizeof kb);
  cubalc_host_json_leaf_kv(a ? a : "{}", NULL, &ka);
  cubalc_host_json_leaf_kv(b ? b : "{}", NULL, &kb);

  p = ka.str;
  while (*p) {
    char path[512], va[CUBALC_HOST_STR_MAX / 4], vb[CUBALC_HOST_STR_MAX / 4];
    char linebuf[CUBALC_HOST_STR_MAX];
    size_t kn = 0;
    int has_b;
    const char *sep;
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
    if (!sep) continue;
    kn = (size_t)(sep - line);
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
    {
      const char *vs = sep + 1, *ve = p;
      size_t vn;
      while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
      while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
      vn = (size_t)(ve - vs);
      if (vn >= sizeof va) vn = sizeof va - 1;
      memcpy(va, vs, vn);
      va[vn] = 0;
    }
    has_b = cubalc_flatbag_get(kb.str, path, vb, sizeof vb);
    cubalc_bag_push(seen, sizeof seen, &seen_olen, &seen_n, path);
    if (has_b && strcmp(va, vb) == 0)
      continue;
    if (paths_only) {
      cubalc_bag_push(r->str, sizeof r->str, &olen, &count, path);
    } else if (has_b) {
      if (snprintf(linebuf, sizeof linebuf, "%s: %s → %s", path, va, vb) >= 0)
        cubalc_bag_push(r->str, sizeof r->str, &olen, &count, linebuf);
    } else {
      if (snprintf(linebuf, sizeof linebuf, "%s: %s → (missing)", path, va) >= 0)
        cubalc_bag_push(r->str, sizeof r->str, &olen, &count, linebuf);
    }
  }

  p = kb.str;
  while (*p) {
    char path[512], vb[CUBALC_HOST_STR_MAX / 4];
    char linebuf[CUBALC_HOST_STR_MAX];
    size_t kn = 0;
    const char *sep;
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
    if (!sep) continue;
    kn = (size_t)(sep - line);
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
    if (cubalc_pathbag_has(seen, path))
      continue;
    {
      const char *vs = sep + 1, *ve = p;
      size_t vn;
      while (vs < ve && (*vs == ' ' || *vs == '\t')) vs++;
      while (ve > vs && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
      vn = (size_t)(ve - vs);
      if (vn >= sizeof vb) vn = sizeof vb - 1;
      memcpy(vb, vs, vn);
      vb[vn] = 0;
    }
    if (paths_only) {
      cubalc_bag_push(r->str, sizeof r->str, &olen, &count, path);
    } else {
      if (snprintf(linebuf, sizeof linebuf, "%s: (missing) → %s", path, vb) >= 0)
        cubalc_bag_push(r->str, sizeof r->str, &olen, &count, linebuf);
    }
  }

  r->n = count;
  r->ok = 1;
  return 0;
}

/* Case-insensitive substring needle in hay[0..hn). Empty needle → hit. */
static int cubalc_str_has_icase_n(const char *hay, size_t hn, const char *needle) {
  size_t nn, i, j;
  if (!needle || !needle[0]) return 1;
  if (!hay) return 0;
  nn = strlen(needle);
  if (nn > hn) return 0;
  for (i = 0; i + nn <= hn; i++) {
    for (j = 0; j < nn; j++) {
      unsigned char a = (unsigned char)hay[i + j];
      unsigned char b = (unsigned char)needle[j];
      if (a >= 'A' && a <= 'Z') a = (unsigned char)(a - 'A' + 'a');
      if (b >= 'A' && b <= 'Z') b = (unsigned char)(b - 'A' + 'a');
      if (a != b) break;
    }
    if (j == nn) return 1;
  }
  return 0;
}

/* Filter leaf path:value bag by path needle. See header. */
int cubalc_host_json_leaf_grep(const char *json, const char *needle, int invert,
                               int icase, cubalc_host_result *r) {
  cubalc_host_result kv;
  const char *p;
  size_t olen = 0;
  long kept = 0;
  r_clear(r);
  r->str[0] = 0;
  r->ok = 1;
  r->n = 0;
  if (!needle) needle = "";
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  p = kv.str;
  while (*p) {
    const char *line, *sep, *path0;
    size_t ln, pn;
    int hit;
    char linebuf[CUBALC_HOST_STR_MAX];
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
    ln = (size_t)(p - line);
    if (ln == 0) continue;
    /* path = trim of prefix before first : or = */
    path0 = line;
    while (path0 < p && (*path0 == ' ' || *path0 == '\t')) path0++;
    sep = path0;
    while (sep < p && *sep != ':' && *sep != '=') sep++;
    pn = (size_t)(sep - path0);
    while (pn > 0 && (path0[pn - 1] == ' ' || path0[pn - 1] == '\t')) pn--;
    if (!needle[0])
      hit = 1;
    else if (icase)
      hit = cubalc_str_has_icase_n(path0, pn, needle);
    else {
      size_t nn = strlen(needle), i;
      hit = 0;
      if (nn <= pn) {
        for (i = 0; i + nn <= pn; i++) {
          if (memcmp(path0 + i, needle, nn) == 0) {
            hit = 1;
            break;
          }
        }
      }
    }
    if (invert) hit = !hit;
    if (!hit) continue;
    if (ln >= sizeof linebuf) ln = sizeof linebuf - 1;
    memcpy(linebuf, line, ln);
    linebuf[ln] = 0;
    cubalc_bag_push(r->str, sizeof r->str, &olen, &kept, linebuf);
  }
  r->n = kept;
  r->ok = 1;
  return 0;
}

/* Write-back prune / keep-only leaf filter. See header. */
int cubalc_host_json_leaf_filter_write(const char *json, const char *needle,
                                       int keep_only, int icase,
                                       cubalc_host_result *r) {
  cubalc_host_result bag, dr;
  const char *p, *line;
  char cur[CUBALC_HOST_STR_MAX];
  long nops = 0;
  r_clear(r);
  if (!needle) needle = "";
  /* empty needle: prune no-op; keep-only = full plate */
  if (!needle[0] && !keep_only) {
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
  memset(&bag, 0, sizeof bag);
  /* keep_only → matching bag; prune → matching bag then delete those paths */
  cubalc_host_json_leaf_grep(json ? json : "{}", needle, 0, icase, &bag);
  if (keep_only) {
    memset(&dr, 0, sizeof dr);
    if (cubalc_host_json_unflat_kv("{}", bag.str, NULL, &dr) != 0) {
      snprintf(r->err, sizeof r->err, "%s",
               dr.err[0] ? dr.err : "keeponly: unflat fail");
      return -1;
    }
    snprintf(r->str, sizeof r->str, "%s", dr.str);
    r->n = bag.n;
    r->ok = 1;
    return 0;
  }
  /* prune: del each matching path */
  snprintf(cur, sizeof cur, "%s", (json && json[0]) ? json : "{}");
  {
    const char *b = cur;
    while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
    if (*b != '{')
      snprintf(cur, sizeof cur, "%s", "{}");
  }
  p = bag.str;
  while (*p) {
    char path[512];
    size_t kn = 0;
    const char *sep;
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
    memset(&dr, 0, sizeof dr);
    if (cubalc_host_json_path_del(cur, path, &dr) == 0) {
      snprintf(cur, sizeof cur, "%s", dr.str);
      if (dr.n > 0)
        nops++;
    }
  }
  snprintf(r->str, sizeof r->str, "%s", cur);
  r->n = nops;
  r->ok = 1;
  return 0;
}

/* Deep leaf overlay merge. See header. */
int cubalc_host_json_leaf_merge(const char *base, const char *overlay,
                                cubalc_host_result *r) {
  cubalc_host_result bag, wr;
  const char *b;
  r_clear(r);
  b = base ? base : "";
  while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
  if (*b != '{')
    b = "{}";
  else
    b = base;
  memset(&bag, 0, sizeof bag);
  cubalc_host_json_leaf_kv(overlay ? overlay : "{}", NULL, &bag);
  if (bag.n == 0 || !bag.str[0]) {
    snprintf(r->str, sizeof r->str, "%s", b && b[0] ? b : "{}");
    {
      const char *t = r->str;
      while (*t == ' ' || *t == '\t' || *t == '\n' || *t == '\r') t++;
      if (*t != '{')
        snprintf(r->str, sizeof r->str, "%s", "{}");
    }
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  memset(&wr, 0, sizeof wr);
  if (cubalc_host_json_unflat_kv(b, bag.str, NULL, &wr) != 0) {
    snprintf(r->err, sizeof r->err, "%s",
             wr.err[0] ? wr.err : "leaf_merge: unflat fail");
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", wr.str);
  r->n = bag.n;
  r->ok = 1;
  return 0;
}

