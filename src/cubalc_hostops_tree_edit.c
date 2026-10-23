/* CubalC — hostops tree edit peel (B_hostops_margin).
 * REPLACETREE·COUNTINTREE. Mesh vote · zero bloat · cube is SoT.
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
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <ctype.h>

static void r_clear(cubalc_host_result *r) {
  if (!r) return;
  memset(r, 0, sizeof *r);
}

static int cubalc_join_child(const char *parent, const char *name,
                             char *out, size_t outn) {
  size_t pl, nl;
  if (!parent || !name || !out || outn < 2) return -1;
  pl = strlen(parent);
  nl = strlen(name);
  if (pl + 1 + nl + 1 >= outn) return -1;
  if (pl > 0 && (parent[pl - 1] == '/' || parent[pl - 1] == '\\'))
    snprintf(out, outn, "%s%s", parent, name);
  else
    snprintf(out, outn, "%s/%s", parent, name);
  return 0;
}

static int cubalc_str_has_icase(const char *hay, const char *needle) {
  size_t nlen, fi, j;
  if (!hay || !needle) return 0;
  nlen = strlen(needle);
  if (nlen == 0) return 1;
  for (fi = 0; hay[fi]; fi++) {
    for (j = 0; j < nlen; j++) {
      char ca = hay[fi + j], cb = needle[j];
      if (!ca) break;
      if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
      if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
      if (ca != cb) break;
    }
    if (j == nlen) return 1;
  }
  return 0;
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

/* REPLACEALL olds→news into out (cap outsz). Returns substitution count. */
static long cubalc_replaceall_buf(const char *src, const char *olds, const char *news,
                                  char *out, size_t outsz) {
  size_t oldn, newn, fo = 0, pre, rest;
  long subs = 0;
  const char *p;
  if (!out || outsz < 1) return 0;
  out[0] = 0;
  if (!src) src = "";
  if (!olds) olds = "";
  if (!news) news = "";
  oldn = strlen(olds);
  newn = strlen(news);
  if (oldn == 0) {
    snprintf(out, outsz, "%s", src);
    return 0;
  }
  p = src;
  for (;;) {
    const char *hit = strstr(p, olds);
    if (!hit) {
      rest = strlen(p);
      if (fo + rest >= outsz) rest = outsz - 1 - fo;
      if (rest > 0) memcpy(out + fo, p, rest);
      fo += rest;
      out[fo] = 0;
      break;
    }
    pre = (size_t)(hit - p);
    if (fo + pre >= outsz) pre = outsz - 1 - fo;
    if (pre > 0) memcpy(out + fo, p, pre);
    fo += pre;
    if (fo + newn < outsz) {
      memcpy(out + fo, news, newn);
      fo += newn;
    } else if (fo < outsz - 1) {
      size_t nt = outsz - 1 - fo;
      memcpy(out + fo, news, nt);
      fo += nt;
    }
    out[fo] = 0;
    subs++;
    p = hit + oldn;
    if (fo >= outsz - 1) break;
  }
  return subs;
}

/* Apply replace on one regular file; push path if rewritten. */
static void cubalc_replacetree_file(const char *path, const char *olds, const char *news,
                                    char *out, size_t outsz, size_t *olen, long *rewritten,
                                    long *subs_total, long *scan) {
  cubalc_host_result hr, wr;
  char nbuf[CUBALC_HOST_STR_MAX];
  long file_subs;
  if (scan) (*scan)++;
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_read(path, &hr) != 0) return;
  file_subs = cubalc_replaceall_buf(hr.str, olds, news, nbuf, sizeof nbuf);
  if (file_subs <= 0) return;
  memset(&wr, 0, sizeof wr);
  if (cubalc_host_write(path, nbuf, &wr) != 0) return;
  if (subs_total) *subs_total += file_subs;
  cubalc_bag_push(out, outsz, olen, rewritten, path);
}

static void cubalc_replacetree_rec(const char *dir, const char *olds, const char *news,
                                   char *out, size_t outsz, size_t *olen, long *rewritten,
                                   long *subs_total, long *scan, int depth) {
  DIR *d;
  struct dirent *ent;
  struct stat st;
  if (!dir || !dir[0] || depth > 64 || !out || !olen || !rewritten) return;
  if (*olen + 1 >= outsz) return;
  d = opendir(dir);
  if (!d) return;
  while ((ent = readdir(d)) != NULL) {
    char full[CUBALC_HOST_STR_MAX];
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;
    if (cubalc_join_child(dir, ent->d_name, full, sizeof full) != 0)
      continue;
    if (lstat(full, &st) != 0) continue;
    if (S_ISDIR(st.st_mode)
#if defined(S_ISLNK)
        && !S_ISLNK(st.st_mode)
#endif
        ) {
      cubalc_replacetree_rec(full, olds, news, out, outsz, olen, rewritten,
                             subs_total, scan, depth + 1);
      continue;
    }
    if (!S_ISREG(st.st_mode)) continue;
    cubalc_replacetree_file(full, olds, news, out, outsz, olen, rewritten,
                            subs_total, scan);
  }
  closedir(d);
}

/* Usability: SYS REPLACETREE|SUBTREE root old new — recursive REPLACEALL without
 * WALK+REPLACEFILES or shell find|sed. r->str rewritten paths; r->n files; r->code subs. */
int cubalc_host_replacetree(const char *root, const char *olds, const char *news,
                            cubalc_host_result *r) {
  char abs[CUBALC_HOST_STR_MAX];
  size_t olen = 0;
  long rewritten = 0, subs = 0, scan = 0;
  struct stat st;
  cubalc_host_result hr;
  r_clear(r);
  if (!root || !root[0]) {
    snprintf(r->err, sizeof r->err, "replacetree: empty path");
    return -1;
  }
  if (!olds) olds = "";
  if (!news) news = "";
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_abspath(root, &hr) == 0 && hr.str[0])
    snprintf(abs, sizeof abs, "%s", hr.str);
  else
    snprintf(abs, sizeof abs, "%s", root);
  if (lstat(abs, &st) != 0) {
    if (errno == ENOENT) {
      snprintf(r->err, sizeof r->err, "replacetree: missing");
      return -1;
    }
    snprintf(r->err, sizeof r->err, "replacetree: %s", strerror(errno));
    return -1;
  }
  r->str[0] = 0;
  if (S_ISREG(st.st_mode)) {
    cubalc_replacetree_file(abs, olds, news, r->str, sizeof r->str, &olen,
                            &rewritten, &subs, &scan);
  } else if (S_ISDIR(st.st_mode)
#if defined(S_ISLNK)
             && !S_ISLNK(st.st_mode)
#endif
             ) {
    cubalc_replacetree_rec(abs, olds, news, r->str, sizeof r->str, &olen,
                           &rewritten, &subs, &scan, 0);
  } else {
    snprintf(r->err, sizeof r->err, "replacetree: not a file or directory");
    return -1;
  }
  r->n = rewritten;
  r->code = (int)(subs > 0x7fffffff ? 0x7fffffff : subs);
  (void)scan;
  r->ok = 1;
  return 0;
}

/* Count non-overlapping occurrences of needle in hay (ASCII icase optional). */
static long cubalc_count_hits(const char *hay, const char *needle, int icase) {
  size_t nlen;
  long hits = 0;
  const char *p;
  if (!hay || !needle) return 0;
  nlen = strlen(needle);
  if (nlen == 0) return 0;
  p = hay;
  if (!icase) {
    while ((p = strstr(p, needle)) != NULL) {
      hits++;
      p += nlen;
    }
    return hits;
  }
  /* case-insensitive: scan with cubalc_str_has style advance */
  {
    size_t fi, j, hlen = strlen(hay);
    for (fi = 0; fi < hlen; ) {
      int match = 1;
      for (j = 0; j < nlen; j++) {
        char ca, cb;
        if (fi + j >= hlen) { match = 0; break; }
        ca = hay[fi + j];
        cb = needle[j];
        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
        if (ca != cb) { match = 0; break; }
      }
      if (match) {
        hits++;
        fi += nlen;
      } else {
        fi++;
      }
    }
  }
  return hits;
}

static void cubalc_countintree_file(const char *path, const char *needle, int icase,
                                    long *hits, long *files_hit) {
  cubalc_host_result hr;
  long h;
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_read(path, &hr) != 0) return;
  h = cubalc_count_hits(hr.str, needle, icase);
  if (h > 0) {
    if (hits) *hits += h;
    if (files_hit) (*files_hit)++;
  }
}

static void cubalc_countintree_rec(const char *dir, const char *needle, int icase,
                                   long *hits, long *files_hit, int depth) {
  DIR *d;
  struct dirent *ent;
  struct stat st;
  if (!dir || !dir[0] || depth > 64) return;
  d = opendir(dir);
  if (!d) return;
  while ((ent = readdir(d)) != NULL) {
    char full[CUBALC_HOST_STR_MAX];
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;
    if (cubalc_join_child(dir, ent->d_name, full, sizeof full) != 0)
      continue;
    if (lstat(full, &st) != 0) continue;
    if (S_ISDIR(st.st_mode)
#if defined(S_ISLNK)
        && !S_ISLNK(st.st_mode)
#endif
        ) {
      cubalc_countintree_rec(full, needle, icase, hits, files_hit, depth + 1);
      continue;
    }
    if (!S_ISREG(st.st_mode)) continue;
    cubalc_countintree_file(full, needle, icase, hits, files_hit);
  }
  closedir(d);
}

/* Usability: SYS COUNTINTREE|COUNTTREE root needle — recursive occurrence tally
 * without WALK+COUNTINFILES or shell grep -rc. r->n=hits; r->code=files with hits. */
int cubalc_host_countintree(const char *root, const char *needle, int icase,
                            cubalc_host_result *r) {
  char abs[CUBALC_HOST_STR_MAX];
  long hits = 0, files_hit = 0;
  struct stat st;
  cubalc_host_result hr;
  r_clear(r);
  if (!root || !root[0]) {
    snprintf(r->err, sizeof r->err, "countintree: empty path");
    return -1;
  }
  if (!needle) needle = "";
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_abspath(root, &hr) == 0 && hr.str[0])
    snprintf(abs, sizeof abs, "%s", hr.str);
  else
    snprintf(abs, sizeof abs, "%s", root);
  if (lstat(abs, &st) != 0) {
    if (errno == ENOENT) {
      snprintf(r->err, sizeof r->err, "countintree: missing");
      return -1;
    }
    snprintf(r->err, sizeof r->err, "countintree: %s", strerror(errno));
    return -1;
  }
  if (S_ISREG(st.st_mode)) {
    cubalc_countintree_file(abs, needle, icase, &hits, &files_hit);
  } else if (S_ISDIR(st.st_mode)
#if defined(S_ISLNK)
             && !S_ISLNK(st.st_mode)
#endif
             ) {
    cubalc_countintree_rec(abs, needle, icase, &hits, &files_hit, 0);
  } else {
    snprintf(r->err, sizeof r->err, "countintree: not a file or directory");
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", abs);
  r->n = hits;
  r->code = (int)(files_hit > 0x7fffffff ? 0x7fffffff : files_hit);
  r->ok = 1;
  return 0;
}

/* Line match: needle empty → always; else strstr / icase. */
