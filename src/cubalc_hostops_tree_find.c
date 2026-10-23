/* CubalC — hostops tree find peel (B_hostops_margin).
 * FINDINTREE·TAKEINTREE·NTHINTREE·SLICEINTREE·DROPINTREE. Mesh vote · zero bloat · cube is SoT.
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

static int cubalc_file_has_needle(const char *path, const char *needle, int icase) {
  cubalc_host_result hr;
  size_t nlen;
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_read(path, &hr) != 0) return 0;
  if (!needle || !needle[0]) return 1;
  nlen = strlen(needle);
  (void)nlen;
  if (!icase) return strstr(hr.str, needle) != NULL;
  return cubalc_str_has_icase(hr.str, needle);
}

/* qsort basenames for deterministic FIRST/LAST under tree. */
static int cubalc_name_cmp(const void *a, const void *b) {
  return strcmp((const char *)a, (const char *)b);
}

/* DFS walk with sorted basenames: set found path.
 * First → stop on first hit; last → keep updating found. */
static int cubalc_findintree_rec(const char *dir, const char *needle, int icase,
                                 int want_last, char *found, size_t foundn, int depth) {
  DIR *d;
  struct dirent *ent;
  struct stat st;
  int any = 0, nnames = 0, i;
  char names[256][256];
  if (!dir || !dir[0] || depth > 64 || !found) return 0;
  d = opendir(dir);
  if (!d) return 0;
  while ((ent = readdir(d)) != NULL && nnames < 256) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;
    if (strlen(ent->d_name) >= 256) continue;
    snprintf(names[nnames], sizeof names[nnames], "%s", ent->d_name);
    nnames++;
  }
  closedir(d);
  if (nnames > 1)
    qsort(names, (size_t)nnames, sizeof names[0], cubalc_name_cmp);
  for (i = 0; i < nnames; i++) {
    char full[CUBALC_HOST_STR_MAX];
    if (cubalc_join_child(dir, names[i], full, sizeof full) != 0)
      continue;
    if (lstat(full, &st) != 0) continue;
    if (S_ISDIR(st.st_mode)
#if defined(S_ISLNK)
        && !S_ISLNK(st.st_mode)
#endif
        ) {
      if (cubalc_findintree_rec(full, needle, icase, want_last, found, foundn, depth + 1)) {
        any = 1;
        if (!want_last) return 1;
      }
      continue;
    }
    if (!S_ISREG(st.st_mode)) continue;
    if (cubalc_file_has_needle(full, needle, icase)) {
      snprintf(found, foundn, "%s", full);
      any = 1;
      if (!want_last) return 1;
    }
  }
  return any;
}

/* Usability: SYS FIRSTINTREE|LASTINTREE root needle — first/last content match under tree
 * without GREPTREE+TAKE/REVL glue. r->str=path; r->n=1|0. Soft miss root → -1. */
int cubalc_host_findintree(const char *root, const char *needle, int icase,
                           int want_last, cubalc_host_result *r) {
  char abs[CUBALC_HOST_STR_MAX], found[CUBALC_HOST_STR_MAX];
  struct stat st;
  cubalc_host_result hr;
  int hit = 0;
  r_clear(r);
  if (!root || !root[0]) {
    snprintf(r->err, sizeof r->err, "findintree: empty path");
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
      snprintf(r->err, sizeof r->err, "findintree: missing");
      return -1;
    }
    snprintf(r->err, sizeof r->err, "findintree: %s", strerror(errno));
    return -1;
  }
  found[0] = 0;
  if (S_ISREG(st.st_mode)) {
    if (cubalc_file_has_needle(abs, needle, icase)) {
      snprintf(found, sizeof found, "%s", abs);
      hit = 1;
    }
  } else if (S_ISDIR(st.st_mode)
#if defined(S_ISLNK)
             && !S_ISLNK(st.st_mode)
#endif
             ) {
    hit = cubalc_findintree_rec(abs, needle, icase, want_last, found, sizeof found, 0);
  } else {
    snprintf(r->err, sizeof r->err, "findintree: not a file or directory");
    return -1;
  }
  if (hit && found[0]) {
    snprintf(r->str, sizeof r->str, "%s", found);
    r->n = 1;
  } else {
    r->str[0] = 0;
    r->n = 0;
  }
  r->ok = 1;
  return 0;
}

/* Sorted DFS: collect up to maxn matching file paths into bag. Stops early. */
static void cubalc_takeintree_rec(const char *dir, const char *needle, int icase,
                                 long maxn, char *out, size_t outsz, size_t *olen,
                                 long *kept, int depth) {
  DIR *d;
  struct dirent *ent;
  struct stat st;
  int nnames = 0, i;
  char names[256][256];
  if (!dir || !dir[0] || depth > 64 || !out || !olen || !kept || maxn <= 0) return;
  if (*kept >= maxn) return;
  d = opendir(dir);
  if (!d) return;
  while ((ent = readdir(d)) != NULL && nnames < 256) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;
    if (strlen(ent->d_name) >= 256) continue;
    snprintf(names[nnames], sizeof names[nnames], "%s", ent->d_name);
    nnames++;
  }
  closedir(d);
  if (nnames > 1)
    qsort(names, (size_t)nnames, sizeof names[0], cubalc_name_cmp);
  for (i = 0; i < nnames && *kept < maxn; i++) {
    char full[CUBALC_HOST_STR_MAX];
    if (cubalc_join_child(dir, names[i], full, sizeof full) != 0)
      continue;
    if (lstat(full, &st) != 0) continue;
    if (S_ISDIR(st.st_mode)
#if defined(S_ISLNK)
        && !S_ISLNK(st.st_mode)
#endif
        ) {
      cubalc_takeintree_rec(full, needle, icase, maxn, out, outsz, olen, kept, depth + 1);
      continue;
    }
    if (!S_ISREG(st.st_mode)) continue;
    if (cubalc_file_has_needle(full, needle, icase))
      cubalc_bag_push(out, outsz, olen, kept, full);
  }
}

/* Usability: SYS TAKEINTREE|FIRSTNINTREE root needle n — first n content matches under tree
 * without GREPTREE+TAKE glue. r->str=path bag; r->n=count. Soft miss root → -1. */
int cubalc_host_takeintree(const char *root, const char *needle, int icase,
                           long maxn, cubalc_host_result *r) {
  char abs[CUBALC_HOST_STR_MAX];
  struct stat st;
  cubalc_host_result hr;
  size_t olen = 0;
  long kept = 0;
  r_clear(r);
  if (!root || !root[0]) {
    snprintf(r->err, sizeof r->err, "takeintree: empty path");
    return -1;
  }
  if (!needle) needle = "";
  if (maxn < 0) maxn = 0;
  if (maxn > 100000) maxn = 100000;
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_abspath(root, &hr) == 0 && hr.str[0])
    snprintf(abs, sizeof abs, "%s", hr.str);
  else
    snprintf(abs, sizeof abs, "%s", root);
  if (lstat(abs, &st) != 0) {
    if (errno == ENOENT) {
      snprintf(r->err, sizeof r->err, "takeintree: missing");
      return -1;
    }
    snprintf(r->err, sizeof r->err, "takeintree: %s", strerror(errno));
    return -1;
  }
  r->str[0] = 0;
  if (S_ISREG(st.st_mode)) {
    if (maxn > 0 && cubalc_file_has_needle(abs, needle, icase))
      cubalc_bag_push(r->str, sizeof r->str, &olen, &kept, abs);
  } else if (S_ISDIR(st.st_mode)
#if defined(S_ISLNK)
             && !S_ISLNK(st.st_mode)
#endif
             ) {
    cubalc_takeintree_rec(abs, needle, icase, maxn, r->str, sizeof r->str, &olen, &kept, 0);
  } else {
    snprintf(r->err, sizeof r->err, "takeintree: not a file or directory");
    return -1;
  }
  r->n = kept;
  r->ok = 1;
  return 0;
}

/* Sorted DFS: find 0-based Nth matching file path. Returns 1 if found. */
static int cubalc_nthintree_rec(const char *dir, const char *needle, int icase,
                                long want_i, long *seen, char *found, size_t foundn,
                                int depth) {
  DIR *d;
  struct dirent *ent;
  struct stat st;
  int nnames = 0, i;
  char names[256][256];
  if (!dir || !dir[0] || depth > 64 || !found || !seen || want_i < 0) return 0;
  d = opendir(dir);
  if (!d) return 0;
  while ((ent = readdir(d)) != NULL && nnames < 256) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;
    if (strlen(ent->d_name) >= 256) continue;
    snprintf(names[nnames], sizeof names[nnames], "%s", ent->d_name);
    nnames++;
  }
  closedir(d);
  if (nnames > 1)
    qsort(names, (size_t)nnames, sizeof names[0], cubalc_name_cmp);
  for (i = 0; i < nnames; i++) {
    char full[CUBALC_HOST_STR_MAX];
    if (cubalc_join_child(dir, names[i], full, sizeof full) != 0)
      continue;
    if (lstat(full, &st) != 0) continue;
    if (S_ISDIR(st.st_mode)
#if defined(S_ISLNK)
        && !S_ISLNK(st.st_mode)
#endif
        ) {
      if (cubalc_nthintree_rec(full, needle, icase, want_i, seen, found, foundn, depth + 1))
        return 1;
      continue;
    }
    if (!S_ISREG(st.st_mode)) continue;
    if (cubalc_file_has_needle(full, needle, icase)) {
      if (*seen == want_i) {
        snprintf(found, foundn, "%s", full);
        return 1;
      }
      (*seen)++;
    }
  }
  return 0;
}

/* Usability: SYS NTHINTREE root needle index — 0-based Nth content match under tree
 * without GREPTREE+NTH or TAKEINTREE+NTH glue. Early-stop walk. Soft miss → -1. */
int cubalc_host_nthintree(const char *root, const char *needle, int icase,
                          long index, cubalc_host_result *r) {
  char abs[CUBALC_HOST_STR_MAX], found[CUBALC_HOST_STR_MAX];
  struct stat st;
  cubalc_host_result hr;
  long seen = 0;
  int hit = 0;
  r_clear(r);
  if (!root || !root[0]) {
    snprintf(r->err, sizeof r->err, "nthintree: empty path");
    return -1;
  }
  if (!needle) needle = "";
  if (index < 0) {
    r->str[0] = 0;
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_abspath(root, &hr) == 0 && hr.str[0])
    snprintf(abs, sizeof abs, "%s", hr.str);
  else
    snprintf(abs, sizeof abs, "%s", root);
  if (lstat(abs, &st) != 0) {
    if (errno == ENOENT) {
      snprintf(r->err, sizeof r->err, "nthintree: missing");
      return -1;
    }
    snprintf(r->err, sizeof r->err, "nthintree: %s", strerror(errno));
    return -1;
  }
  found[0] = 0;
  if (S_ISREG(st.st_mode)) {
    if (index == 0 && cubalc_file_has_needle(abs, needle, icase)) {
      snprintf(found, sizeof found, "%s", abs);
      hit = 1;
    }
  } else if (S_ISDIR(st.st_mode)
#if defined(S_ISLNK)
             && !S_ISLNK(st.st_mode)
#endif
             ) {
    hit = cubalc_nthintree_rec(abs, needle, icase, index, &seen, found, sizeof found, 0);
  } else {
    snprintf(r->err, sizeof r->err, "nthintree: not a file or directory");
    return -1;
  }
  if (hit && found[0]) {
    snprintf(r->str, sizeof r->str, "%s", found);
    r->n = 1;
  } else {
    r->str[0] = 0;
    r->n = 0;
  }
  r->ok = 1;
  return 0;
}

/* Sorted DFS: collect matching paths in [start, start+maxn) into bag. Early-stop. */
static void cubalc_sliceintree_rec(const char *dir, const char *needle, int icase,
                                  long start, long maxn, char *out, size_t outsz,
                                  size_t *olen, long *seen, long *kept, int depth) {
  DIR *d;
  struct dirent *ent;
  struct stat st;
  int nnames = 0, i;
  char names[256][256];
  if (!dir || !dir[0] || depth > 64 || !out || !olen || !seen || !kept || maxn <= 0)
    return;
  if (*kept >= maxn) return;
  d = opendir(dir);
  if (!d) return;
  while ((ent = readdir(d)) != NULL && nnames < 256) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;
    if (strlen(ent->d_name) >= 256) continue;
    snprintf(names[nnames], sizeof names[nnames], "%s", ent->d_name);
    nnames++;
  }
  closedir(d);
  if (nnames > 1)
    qsort(names, (size_t)nnames, sizeof names[0], cubalc_name_cmp);
  for (i = 0; i < nnames && *kept < maxn; i++) {
    char full[CUBALC_HOST_STR_MAX];
    if (cubalc_join_child(dir, names[i], full, sizeof full) != 0)
      continue;
    if (lstat(full, &st) != 0) continue;
    if (S_ISDIR(st.st_mode)
#if defined(S_ISLNK)
        && !S_ISLNK(st.st_mode)
#endif
        ) {
      cubalc_sliceintree_rec(full, needle, icase, start, maxn, out, outsz, olen,
                             seen, kept, depth + 1);
      continue;
    }
    if (!S_ISREG(st.st_mode)) continue;
    if (cubalc_file_has_needle(full, needle, icase)) {
      if (*seen >= start && *kept < maxn)
        cubalc_bag_push(out, outsz, olen, kept, full);
      (*seen)++;
    }
  }
}

/* Usability: SYS SLICEINTREE|MIDINTREE root needle start count
 * — path window of content matches under tree without GREPTREE+DROP+TAKE.
 * r->str=path bag; r->n=kept. Soft miss root → -1. */
int cubalc_host_sliceintree(const char *root, const char *needle, int icase,
                            long start, long count, cubalc_host_result *r) {
  char abs[CUBALC_HOST_STR_MAX];
  struct stat st;
  cubalc_host_result hr;
  size_t olen = 0;
  long seen = 0, kept = 0;
  r_clear(r);
  if (!root || !root[0]) {
    snprintf(r->err, sizeof r->err, "sliceintree: empty path");
    return -1;
  }
  if (!needle) needle = "";
  if (start < 0) start = 0;
  if (count < 0) count = 0;
  if (count > 100000) count = 100000;
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_abspath(root, &hr) == 0 && hr.str[0])
    snprintf(abs, sizeof abs, "%s", hr.str);
  else
    snprintf(abs, sizeof abs, "%s", root);
  if (lstat(abs, &st) != 0) {
    if (errno == ENOENT) {
      snprintf(r->err, sizeof r->err, "sliceintree: missing");
      return -1;
    }
    snprintf(r->err, sizeof r->err, "sliceintree: %s", strerror(errno));
    return -1;
  }
  r->str[0] = 0;
  if (S_ISREG(st.st_mode)) {
    if (count > 0 && start == 0 && cubalc_file_has_needle(abs, needle, icase))
      cubalc_bag_push(r->str, sizeof r->str, &olen, &kept, abs);
  } else if (S_ISDIR(st.st_mode)
#if defined(S_ISLNK)
             && !S_ISLNK(st.st_mode)
#endif
             ) {
    cubalc_sliceintree_rec(abs, needle, icase, start, count, r->str, sizeof r->str,
                           &olen, &seen, &kept, 0);
  } else {
    snprintf(r->err, sizeof r->err, "sliceintree: not a file or directory");
    return -1;
  }
  r->n = kept;
  r->ok = 1;
  return 0;
}

/* Usability: SYS DROPINTREE|SKIPINTREE root needle n — skip first n content matches
 * under tree → remaining path bag without GREPTREE+DROP glue. Dual of TAKEINTREE. */
int cubalc_host_dropintree(const char *root, const char *needle, int icase,
                           long skipn, cubalc_host_result *r) {
  char abs[CUBALC_HOST_STR_MAX];
  struct stat st;
  cubalc_host_result hr;
  size_t olen = 0;
  long seen = 0, kept = 0;
  long maxn = 100000;
  r_clear(r);
  if (!root || !root[0]) {
    snprintf(r->err, sizeof r->err, "dropintree: empty path");
    return -1;
  }
  if (!needle) needle = "";
  if (skipn < 0) skipn = 0;
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_abspath(root, &hr) == 0 && hr.str[0])
    snprintf(abs, sizeof abs, "%s", hr.str);
  else
    snprintf(abs, sizeof abs, "%s", root);
  if (lstat(abs, &st) != 0) {
    if (errno == ENOENT) {
      snprintf(r->err, sizeof r->err, "dropintree: missing");
      return -1;
    }
    snprintf(r->err, sizeof r->err, "dropintree: %s", strerror(errno));
    return -1;
  }
  r->str[0] = 0;
  if (S_ISREG(st.st_mode)) {
    /* single file: drop 0 keeps it; drop ≥1 yields empty when match */
    if (cubalc_file_has_needle(abs, needle, icase)) {
      if (skipn == 0)
        cubalc_bag_push(r->str, sizeof r->str, &olen, &kept, abs);
    }
  } else if (S_ISDIR(st.st_mode)
#if defined(S_ISLNK)
             && !S_ISLNK(st.st_mode)
#endif
             ) {
    cubalc_sliceintree_rec(abs, needle, icase, skipn, maxn, r->str, sizeof r->str,
                           &olen, &seen, &kept, 0);
  } else {
    snprintf(r->err, sizeof r->err, "dropintree: not a file or directory");
    return -1;
  }
  r->n = kept;
  r->ok = 1;
  return 0;
}
