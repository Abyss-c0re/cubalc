/* CubalC — hostops tree query peel (B_hostops_margin).
 * EQTREE·GREPTREE. Mesh vote · zero bloat · cube is SoT.
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

/* Streamed content equality for two regular files (or non-dir nodes). */
static int cubalc_files_content_eq(const char *a, const char *b) {
  FILE *fa, *fb;
  char ba[4096], bb[4096];
  size_t na, nb;
  struct stat sa, sb;
  if (stat(a, &sa) != 0 || stat(b, &sb) != 0) return 0;
  if (sa.st_size != sb.st_size) return 0;
  if (sa.st_size == 0) return 1;
  fa = fopen(a, "rb");
  fb = fopen(b, "rb");
  if (!fa || !fb) {
    if (fa) fclose(fa);
    if (fb) fclose(fb);
    return 0;
  }
  for (;;) {
    na = fread(ba, 1, sizeof ba, fa);
    nb = fread(bb, 1, sizeof bb, fb);
    if (na != nb) { fclose(fa); fclose(fb); return 0; }
    if (na == 0) break;
    if (memcmp(ba, bb, na) != 0) { fclose(fa); fclose(fb); return 0; }
  }
  fclose(fa);
  fclose(fb);
  return 1;
}

/* Join parent/name → child path. */
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

/* Recursive structural+content compare.
 * Returns 1 equal, 0 differ (fills diff/reason), -1 hard error. */
static int cubalc_eqtree_rec(const char *a, const char *b, const char *rel,
                             char *diff, size_t diffn,
                             char *reason, size_t reasonn, int depth) {
  struct stat sa, sb;
  int da, db;
  DIR *d;
  struct dirent *ent;
  if (depth > 64) {
    snprintf(reason, reasonn, "%s", "depth");
    snprintf(diff, diffn, "%s", rel && rel[0] ? rel : ".");
    return 0;
  }
  if (lstat(a, &sa) != 0) {
    snprintf(reason, reasonn, "%s", "missing_a");
    snprintf(diff, diffn, "%s", rel && rel[0] ? rel : ".");
    return 0;
  }
  if (lstat(b, &sb) != 0) {
    snprintf(reason, reasonn, "%s", "missing_b");
    snprintf(diff, diffn, "%s", rel && rel[0] ? rel : ".");
    return 0;
  }
  da = S_ISDIR(sa.st_mode)
#if defined(S_ISLNK)
       && !S_ISLNK(sa.st_mode)
#endif
       ? 1 : 0;
  db = S_ISDIR(sb.st_mode)
#if defined(S_ISLNK)
       && !S_ISLNK(sb.st_mode)
#endif
       ? 1 : 0;
  if (da != db) {
    snprintf(reason, reasonn, "%s", "type");
    snprintf(diff, diffn, "%s", rel && rel[0] ? rel : ".");
    return 0;
  }
  if (!da) {
    /* file / symlink / special: content compare via size+bytes */
    if (!cubalc_files_content_eq(a, b)) {
      snprintf(reason, reasonn, "%s", "content");
      snprintf(diff, diffn, "%s", rel && rel[0] ? rel : ".");
      return 0;
    }
    return 1;
  }
  /* both directories: every entry in a must match in b, and b has no extras */
  d = opendir(a);
  if (!d) return -1;
  while ((ent = readdir(d)) != NULL) {
    char ca[CUBALC_HOST_STR_MAX], cb[CUBALC_HOST_STR_MAX], rchild[CUBALC_HOST_STR_MAX];
    int rc;
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;
    if (cubalc_join_child(a, ent->d_name, ca, sizeof ca) != 0 ||
        cubalc_join_child(b, ent->d_name, cb, sizeof cb) != 0) {
      closedir(d);
      return -1;
    }
    if (rel && rel[0] && strcmp(rel, ".") != 0)
      snprintf(rchild, sizeof rchild, "%s/%s", rel, ent->d_name);
    else
      snprintf(rchild, sizeof rchild, "%s", ent->d_name);
    rc = cubalc_eqtree_rec(ca, cb, rchild, diff, diffn, reason, reasonn, depth + 1);
    if (rc != 1) {
      closedir(d);
      return rc;
    }
  }
  closedir(d);
  /* extras in b */
  d = opendir(b);
  if (!d) return -1;
  while ((ent = readdir(d)) != NULL) {
    char ca[CUBALC_HOST_STR_MAX];
    struct stat st;
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;
    if (cubalc_join_child(a, ent->d_name, ca, sizeof ca) != 0) {
      closedir(d);
      return -1;
    }
    if (lstat(ca, &st) != 0) {
      if (rel && rel[0] && strcmp(rel, ".") != 0)
        snprintf(diff, diffn, "%s/%s", rel, ent->d_name);
      else
        snprintf(diff, diffn, "%s", ent->d_name);
      snprintf(reason, reasonn, "%s", "extra_b");
      closedir(d);
      return 0;
    }
  }
  closedir(d);
  return 1;
}

/* Usability: SYS EQTREE|SAMETREE a b — recursive structure+content equality without shell diff -r.
 * r->n = 1|0; r->str = first relative diff path; r->err = reason when differ.
 * Soft miss either root → -1. */
int cubalc_host_eqtree(const char *a, const char *b, cubalc_host_result *r) {
  char aa[CUBALC_HOST_STR_MAX], bb[CUBALC_HOST_STR_MAX];
  char diff[CUBALC_HOST_STR_MAX], reason[64];
  cubalc_host_result hr;
  struct stat st;
  int rc;
  r_clear(r);
  if (!a || !a[0] || !b || !b[0]) {
    snprintf(r->err, sizeof r->err, "eqtree: empty path");
    return -1;
  }
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_abspath(a, &hr) == 0 && hr.str[0])
    snprintf(aa, sizeof aa, "%s", hr.str);
  else
    snprintf(aa, sizeof aa, "%s", a);
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_abspath(b, &hr) == 0 && hr.str[0])
    snprintf(bb, sizeof bb, "%s", hr.str);
  else
    snprintf(bb, sizeof bb, "%s", b);
  if (lstat(aa, &st) != 0) {
    snprintf(r->err, sizeof r->err, "eqtree: missing_a");
    return -1;
  }
  if (lstat(bb, &st) != 0) {
    snprintf(r->err, sizeof r->err, "eqtree: missing_b");
    return -1;
  }
  if (strcmp(aa, bb) == 0) {
    r->n = 1;
    r->str[0] = 0;
    r->ok = 1;
    return 0;
  }
  diff[0] = 0;
  reason[0] = 0;
  rc = cubalc_eqtree_rec(aa, bb, "", diff, sizeof diff, reason, sizeof reason, 0);
  if (rc < 0) {
    snprintf(r->err, sizeof r->err, "eqtree: walk fail");
    return -1;
  }
  r->n = rc ? 1 : 0;
  r->ok = 1;
  if (rc) {
    r->str[0] = 0;
  } else {
    snprintf(r->str, sizeof r->str, "%s", diff[0] ? diff : ".");
    snprintf(r->err, sizeof r->err, "%s", reason[0] ? reason : "differ");
  }
  return 0;
}

/* Case-insensitive substring (ASCII). */
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

/* Append path to bag with newline separator; soft truncate when full. */
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

static void cubalc_greptree_rec(const char *dir, const char *needle, int icase,
                                int invert, char *out, size_t outsz, size_t *olen,
                                long *kept, long *scan, long *miss, int depth) {
  DIR *d;
  struct dirent *ent;
  struct stat st;
  size_t nlen;
  if (!dir || !dir[0] || depth > 64 || !out || !olen || !kept) return;
  if (*olen + 1 >= outsz) return;
  nlen = needle ? strlen(needle) : 0;
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
      cubalc_greptree_rec(full, needle, icase, invert, out, outsz, olen, kept,
                          scan, miss, depth + 1);
      continue;
    }
    /* only regular files for content probe */
    if (!S_ISREG(st.st_mode)) continue;
    if (scan) (*scan)++;
    {
      cubalc_host_result hr;
      int hit = 0;
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_read(full, &hr) != 0) {
        if (miss) (*miss)++;
        continue;
      }
      if (nlen == 0) {
        hit = 1;
      } else if (!icase) {
        hit = (strstr(hr.str, needle) != NULL) ? 1 : 0;
      } else {
        hit = cubalc_str_has_icase(hr.str, needle);
      }
      if (invert) hit = !hit;
      if (hit)
        cubalc_bag_push(out, outsz, olen, kept, full);
    }
  }
  closedir(d);
}

/* Usability: SYS GREPTREE|SEARCHTREE root needle — recursive content search without
 * WALK+GREPFILES glue or shell grep -r. r->str bag of paths; r->n hits; r->code scanned. */
int cubalc_host_greptree(const char *root, const char *needle, int icase, int invert,
                         cubalc_host_result *r) {
  char abs[CUBALC_HOST_STR_MAX];
  size_t olen = 0;
  long kept = 0, scan = 0, miss = 0;
  struct stat st;
  cubalc_host_result hr;
  r_clear(r);
  if (!root || !root[0]) {
    snprintf(r->err, sizeof r->err, "greptree: empty path");
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
      snprintf(r->err, sizeof r->err, "greptree: missing");
      return -1;
    }
    snprintf(r->err, sizeof r->err, "greptree: %s", strerror(errno));
    return -1;
  }
  r->str[0] = 0;
  if (S_ISREG(st.st_mode)) {
    /* single file root */
    int hit = 0;
    scan = 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_read(abs, &hr) != 0) {
      miss = 1;
    } else {
      size_t nlen = strlen(needle);
      if (nlen == 0) hit = 1;
      else if (!icase) hit = (strstr(hr.str, needle) != NULL) ? 1 : 0;
      else hit = cubalc_str_has_icase(hr.str, needle);
      if (invert) hit = !hit;
      if (hit) cubalc_bag_push(r->str, sizeof r->str, &olen, &kept, abs);
    }
  } else if (S_ISDIR(st.st_mode)
#if defined(S_ISLNK)
             && !S_ISLNK(st.st_mode)
#endif
             ) {
    cubalc_greptree_rec(abs, needle, icase, invert, r->str, sizeof r->str, &olen,
                        &kept, &scan, &miss, 0);
  } else {
    snprintf(r->err, sizeof r->err, "greptree: not a file or directory");
    return -1;
  }
  r->n = kept;
  r->code = (int)(scan > 0x7fffffff ? 0x7fffffff : scan);
  /* stash miss count in high... not available; language can re-scan. OK. */
  (void)miss;
  r->ok = 1;
  return 0;
}

