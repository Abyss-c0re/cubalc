/* CubalC run-path helpers — F_main peel from main · zero bloat · cube is SoT */
#define _POSIX_C_SOURCE 200809L
#include "cubalc_run_util.h"
#include "cubalc.h"
#include "cubalc_lang.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

/* Usability: CUBALC_REQUIRE_VERSION / run -R — have >= need (x.y[.z]). */
int run_version_ge(const char *have, const char *need) {
  long hn[3] = {0, 0, 0}, nn[3] = {0, 0, 0};
  int hi = 0, ni = 0, c;
  char *hp, *np;
  char hbuf[64], nbuf[64];
  if (!need || !need[0]) return 1;
  if (!have || !have[0]) return 0;
  snprintf(hbuf, sizeof hbuf, "%s", have);
  snprintf(nbuf, sizeof nbuf, "%s", need);
  hp = hbuf;
  while (*hp && hi < 3) {
    while (*hp && (*hp < '0' || *hp > '9')) hp++;
    if (!*hp) break;
    hn[hi++] = strtol(hp, &hp, 10);
    if (*hp == '.') hp++;
    else break;
  }
  np = nbuf;
  while (*np && ni < 3) {
    while (*np && (*np < '0' || *np > '9')) np++;
    if (!*np) break;
    nn[ni++] = strtol(np, &np, 10);
    if (*np == '.') np++;
    else break;
  }
  if (ni == 0) return 0;
  for (c = 0; c < 3; c++) {
    if (hn[c] > nn[c]) return 1;
    if (hn[c] < nn[c]) return 0;
  }
  return 1;
}

/* Usability: cubalc run -I / CUBALC_PRELOAD — prepend INCLUDE ONCE lines. */
int run_safe_include_token(const char *s) {
  size_t i;
  if (!s || !s[0]) return 0;
  for (i = 0; s[i]; i++) {
    unsigned char c = (unsigned char)s[i];
    if (c < 32 || c == '"' || c == '\'' || c == ';' || c == '\\') return 0;
  }
  return 1;
}

void run_preload_add(char preload[][96], int *n_preload, const char *name) {
  int i;
  if (!name || !name[0] || !n_preload || *n_preload >= 16) return;
  if (!run_safe_include_token(name)) return;
  for (i = 0; i < *n_preload; i++) {
    if (strcmp(preload[i], name) == 0) return;
  }
  snprintf(preload[*n_preload], 96, "%s", name);
  (*n_preload)++;
}

/* True if includes bag (newline paths) has module stem matching want. */
int run_includes_has_stem(const char *includes, const char *want) {
  char stem[96];
  size_t wlen;
  const char *ip;
  if (!want || !want[0]) return 0;
  {
    const char *slash = strrchr(want, '/');
    snprintf(stem, sizeof stem, "%s", slash ? slash + 1 : want);
  }
  wlen = strlen(stem);
  if (wlen > 7 && strcmp(stem + wlen - 7, ".cubalc") == 0)
    stem[wlen - 7] = 0;
  if (!includes) return 0;
  for (ip = includes; *ip;) {
    const char *seg = ip;
    size_t slen = 0;
    const char *b;
    char pstem[256];
    size_t plen;
    while (ip[slen] && ip[slen] != '\n') slen++;
    b = seg;
    {
      size_t j;
      for (j = 0; j < slen; j++)
        if (seg[j] == '/') b = seg + j + 1;
    }
    plen = (size_t)(seg + slen - b);
    if (plen >= sizeof pstem) plen = sizeof pstem - 1;
    memcpy(pstem, b, plen);
    pstem[plen] = 0;
    if (plen > 7 && strcmp(pstem + plen - 7, ".cubalc") == 0)
      pstem[plen - 7] = 0;
    if (strcmp(pstem, stem) == 0 || strcmp(pstem, want) == 0)
      return 1;
    ip += slen;
    if (*ip == '\n') ip++;
  }
  return 0;
}

/* JSON array of short stems from includes bag · dual of INCLUDESTEMS.
 * Returns stem count; outj = ["hold_seed","agent_boot"] or []. */
int run_includes_stems_json(const char *includes, char *outj, size_t outn) {
  size_t io = 0;
  int n = 0;
  const char *ip;
  if (!outj || outn < 3) return 0;
  outj[io++] = '[';
  if (!includes) includes = "";
  for (ip = includes; *ip;) {
    const char *seg = ip;
    size_t slen = 0;
    const char *b;
    char stem[256];
    size_t plen, j;
    while (ip[slen] && ip[slen] != '\n') slen++;
    b = seg;
    for (j = 0; j < slen; j++)
      if (seg[j] == '/') b = seg + j + 1;
    plen = (size_t)(seg + slen - b);
    if (plen >= sizeof stem) plen = sizeof stem - 1;
    memcpy(stem, b, plen);
    stem[plen] = 0;
    if (plen > 7 && strcmp(stem + plen - 7, ".cubalc") == 0) {
      stem[plen - 7] = 0;
      plen -= 7;
    }
    if (plen > 0) {
      if (n && io + 1 < outn) outj[io++] = ',';
      if (io + 1 < outn) outj[io++] = '"';
      for (j = 0; j < plen && io + 2 < outn; j++) {
        char c = stem[j];
        if (c == '"' || c == '\\') {
          outj[io++] = '\\';
          if (io + 1 < outn) outj[io++] = c;
        } else if ((unsigned char)c < 32) {
          if (io + 1 < outn) outj[io++] = ' ';
        } else {
          outj[io++] = c;
        }
      }
      if (io + 1 < outn) outj[io++] = '"';
      n++;
    }
    ip += slen;
    if (*ip == '\n') ip++;
  }
  if (io + 1 < outn) outj[io++] = ']';
  outj[io] = 0;
  return n;
}

/* JSON array of -I names missing from includes bag · dual of PRELOADMISS.
 * Returns miss count; outj = ["a","b"] or []. Empty request → 0 + []. */
int run_preload_miss_json(char preload[][96], int n_preload,
                                 const char *includes,
                                 char *outj, size_t outn) {
  size_t po = 0;
  int pi, n_miss = 0;
  if (!outj || outn < 3) return 0;
  outj[po++] = '[';
  for (pi = 0; pi < n_preload; pi++) {
    size_t j, sl;
    if (run_includes_has_stem(includes, preload[pi]))
      continue;
    sl = strlen(preload[pi]);
    if (n_miss && po + 1 < outn) outj[po++] = ',';
    if (po + 1 < outn) outj[po++] = '"';
    for (j = 0; j < sl && po + 2 < outn; j++) {
      char c = preload[pi][j];
      if (c == '"' || c == '\\') {
        outj[po++] = '\\';
        if (po + 1 < outn) outj[po++] = c;
      } else if ((unsigned char)c < 32) {
        if (po + 1 < outn) outj[po++] = ' ';
      } else {
        outj[po++] = c;
      }
    }
    if (po + 1 < outn) outj[po++] = '"';
    n_miss++;
  }
  if (po + 1 < outn) outj[po++] = ']';
  outj[po] = 0;
  return n_miss;
}

void run_preload_parse_env(char preload[][96], int *n_preload, const char *env) {
  const char *p = env;
  if (!p || !p[0]) return;
  while (*p && *n_preload < 16) {
    char name[96];
    size_t len = 0;
    while (*p == ':' || *p == ',' || *p == ' ' || *p == '\t') p++;
    if (!*p) break;
    while (p[len] && p[len] != ':' && p[len] != ',' && p[len] != ' ' &&
           p[len] != '\t' && len + 1 < sizeof name) {
      name[len] = p[len];
      len++;
    }
    name[len] = 0;
    p += len;
    if (name[0]) run_preload_add(preload, n_preload, name);
  }
}

/* Usability: cubalc run --dotenv / CUBALC_DOTENV — load KEY=VAL plates before body.
 * Dual of in-lang SYS DOTENV without rewriting every program. Paths colon-separated. */
void run_dotenv_add(char paths[][512], int *n, const char *path) {
  int k;
  if (!path || !path[0] || !n || *n >= 8) return;
  for (k = 0; k < *n; k++)
    if (strcmp(paths[k], path) == 0) return;
  snprintf(paths[*n], 512, "%s", path);
  (*n)++;
}

void run_dotenv_parse_env(char paths[][512], int *n, const char *env) {
  const char *p = env;
  if (!p || !p[0]) return;
  while (*p && *n < 8) {
    char path[512];
    size_t len = 0;
    while (*p == ':' || *p == ',' || *p == ' ' || *p == '\t') p++;
    if (!*p) break;
    while (p[len] && p[len] != ':' && p[len] != ',' && len + 1 < sizeof path) {
      path[len] = p[len];
      len++;
    }
    path[len] = 0;
    p += len;
    while (len > 0 && (path[len - 1] == ' ' || path[len - 1] == '\t'))
      path[--len] = 0;
    if (path[0]) run_dotenv_add(paths, n, path);
  }
}

/* mkdir -p path · 0 ok · -1 fail (err filled). Dual of SYS MKDIR for host run. */
int run_mkdir_p(const char *path, char *err, size_t errn) {
  char buf[1024];
  size_t i, n;
  struct stat st;
  if (!path || !path[0]) {
    if (err && errn) snprintf(err, errn, "MKDIR: empty path");
    return -1;
  }
  if (stat(path, &st) == 0) {
    if (S_ISDIR(st.st_mode)) return 0;
    if (err && errn)
      snprintf(err, errn, "MKDIR: exists but not dir '%s'", path);
    return -1;
  }
  snprintf(buf, sizeof buf, "%s", path);
  n = strlen(buf);
  while (n > 1 && buf[n - 1] == '/') buf[--n] = 0;
  for (i = 1; i < n; i++) {
    if (buf[i] != '/') continue;
    buf[i] = 0;
    if (buf[0] && mkdir(buf, 0755) != 0 && errno != EEXIST) {
      if (err && errn)
        snprintf(err, errn, "MKDIR: fail '%s' (%s)", buf, strerror(errno));
      return -1;
    }
    if (buf[0] && stat(buf, &st) == 0 && !S_ISDIR(st.st_mode)) {
      if (err && errn)
        snprintf(err, errn, "MKDIR: not a dir '%s'", buf);
      return -1;
    }
    buf[i] = '/';
  }
  if (mkdir(buf, 0755) != 0 && errno != EEXIST) {
    if (err && errn)
      snprintf(err, errn, "MKDIR: fail '%s' (%s)", buf, strerror(errno));
    return -1;
  }
  if (stat(buf, &st) != 0 || !S_ISDIR(st.st_mode)) {
    if (err && errn)
      snprintf(err, errn, "MKDIR: not a dir '%s'", buf);
    return -1;
  }
  return 0;
}

/* Parse KEY=VAL into process env. Returns 0 ok · -1 bad form (err filled).
 * Usability: cubalc run -E KEY=VAL dual of SYS ENV SET without rewriting body. */
int run_setenv_one(const char *kv, char *err, size_t errn) {
  char key[192], val[CUBALC_HOST_STR_MAX];
  const char *eq;
  size_t klen;
  if (!kv || !kv[0]) {
    if (err && errn) snprintf(err, errn, "SETENV: empty KEY=VAL");
    return -1;
  }
  eq = strchr(kv, '=');
  if (!eq || eq == kv) {
    if (err && errn)
      snprintf(err, errn, "SETENV: need KEY=VAL (got '%s')", kv);
    return -1;
  }
  klen = (size_t)(eq - kv);
  if (klen >= sizeof key) klen = sizeof key - 1;
  memcpy(key, kv, klen);
  key[klen] = 0;
  snprintf(val, sizeof val, "%s", eq + 1);
  if (!key[0]) {
    if (err && errn) snprintf(err, errn, "SETENV: empty key");
    return -1;
  }
  setenv(key, val, 1);
  return 0;
}

/* Load one dotenv plate via language SYS DOTENV (shared parser/semantics).
 * Returns 0 ok · -1 miss/fail (err filled). keys_out += DOTENV_N when possible. */
int run_dotenv_load_one(const char *path, long *keys_out, char *err, size_t errn) {
  char lit[600], src[720];
  size_t li = 0, k;
  cubalc_run_result rr;
  long nkeys = 0;
  if (!path || !path[0]) {
    if (err && errn) snprintf(err, errn, "DOTENV: empty path");
    return -1;
  }
  for (k = 0; path[k] && li + 2 < sizeof lit; k++) {
    char c = path[k];
    if (c == '"' || c == '\\') {
      lit[li++] = '\\';
      lit[li++] = c;
    } else if ((unsigned char)c < 32)
      lit[li++] = ' ';
    else
      lit[li++] = c;
  }
  lit[li] = 0;
  snprintf(src, sizeof src,
           "SYS DOTENV \"%s\"\n"
           "PRINT LAST_N\n"
           "PASS\n",
           lit);
  memset(&rr, 0, sizeof rr);
  (void)cubalc_run_source(src, strlen(src), "<run-dotenv>", &rr, NULL);
  if (!(rr.ok && rr.asserts_fail == 0 && !rr.err[0])) {
    if (err && errn)
      snprintf(err, errn, "%s",
               rr.err[0] ? rr.err : (rr.last_err[0] ? rr.last_err : "DOTENV fail"));
    return -1;
  }
  if (rr.last_print[0])
    nkeys = strtol(rr.last_print, NULL, 10);
  if (rr.last_err[0] && nkeys <= 0) {
    if (err && errn) snprintf(err, errn, "%s", rr.last_err);
    return -1;
  }
  if (keys_out) *keys_out += nkeys;
  return 0;
}

/* body may be NULL when only preloads (unlikely). Caller frees return. */
char *run_build_with_preload(char preload[][96], int n_preload,
                                    const char *body, size_t body_len,
                                    size_t *out_len) {
  size_t pre = 0, need, k;
  char *buf;
  if (!body) {
    body = "";
    body_len = 0;
  }
  if (n_preload <= 0) {
    buf = malloc(body_len + 1);
    if (!buf) return NULL;
    if (body_len) memcpy(buf, body, body_len);
    buf[body_len] = 0;
    if (out_len) *out_len = body_len;
    return buf;
  }
  for (k = 0; k < (size_t)n_preload; k++)
    pre += strlen("INCLUDE ONCE ") + strlen(preload[k]) + 1;
  need = pre + body_len + 8;
  if (need > (size_t)CUBALC_MAX_SRC + 2048)
    need = (size_t)CUBALC_MAX_SRC + 2048;
  buf = malloc(need);
  if (!buf) return NULL;
  pre = 0;
  for (k = 0; k < (size_t)n_preload; k++) {
    int n = snprintf(buf + pre, need - pre, "INCLUDE ONCE %s\n", preload[k]);
    if (n > 0 && (size_t)n < need - pre) pre += (size_t)n;
  }
  if (pre + body_len >= need) body_len = need - pre - 1;
  if (body_len) memcpy(buf + pre, body, body_len);
  buf[pre + body_len] = 0;
  if (out_len) *out_len = pre + body_len;
  return buf;
}
