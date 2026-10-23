/* CubalC — hostops tree lines peel (B_hostops_margin).
 * GREPLINESTREE·PATHLINETREE. Mesh vote · zero bloat · cube is SoT.
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

static int cubalc_line_matches(const char *line, const char *needle, int icase) {
  size_t nlen;
  if (!line) return 0;
  if (!needle || !needle[0]) return 1;
  nlen = strlen(needle);
  if (!icase) return strstr(line, needle) != NULL;
  return cubalc_str_has_icase(line, needle);
}

/* Extract matching lines from one file into bag. */
static void cubalc_greplinestree_file(const char *path, const char *needle, int icase,
                                     char *out, size_t outsz, size_t *olen, long *nlines,
                                     long *files_hit) {
  cubalc_host_result hr;
  const char *p, *ls;
  long file_hits = 0;
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_read(path, &hr) != 0) return;
  p = hr.str;
  while (*p) {
    size_t llen;
    char line[CUBALC_HOST_STR_MAX];
    ls = p;
    while (*p && *p != '\n') p++;
    llen = (size_t)(p - ls);
    if (llen >= sizeof line) llen = sizeof line - 1;
    if (llen > 0) {
      memcpy(line, ls, llen);
      line[llen] = 0;
      if (cubalc_line_matches(line, needle, icase)) {
        cubalc_bag_push(out, outsz, olen, nlines, line);
        file_hits++;
      }
    } else if (needle && !needle[0]) {
      /* empty line counts for empty needle */
      cubalc_bag_push(out, outsz, olen, nlines, "");
      file_hits++;
    }
    if (*p == '\n') p++;
  }
  if (file_hits > 0 && files_hit) (*files_hit)++;
}

static void cubalc_greplinestree_rec(const char *dir, const char *needle, int icase,
                                    char *out, size_t outsz, size_t *olen, long *nlines,
                                    long *files_hit, int depth) {
  DIR *d;
  struct dirent *ent;
  struct stat st;
  if (!dir || !dir[0] || depth > 64 || !out || !olen || !nlines) return;
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
      cubalc_greplinestree_rec(full, needle, icase, out, outsz, olen, nlines,
                               files_hit, depth + 1);
      continue;
    }
    if (!S_ISREG(st.st_mode)) continue;
    cubalc_greplinestree_file(full, needle, icase, out, outsz, olen, nlines, files_hit);
  }
  closedir(d);
}

/* Usability: SYS GREPLINESTREE|EXTRACTLINETREE root needle — recursive line harvest
 * without WALK+GREPLINES or shell grep -rn. r->str lines bag; r->n lines; r->code files. */
int cubalc_host_greplines_tree(const char *root, const char *needle, int icase,
                               cubalc_host_result *r) {
  char abs[CUBALC_HOST_STR_MAX];
  size_t olen = 0;
  long nlines = 0, files_hit = 0;
  struct stat st;
  cubalc_host_result hr;
  r_clear(r);
  if (!root || !root[0]) {
    snprintf(r->err, sizeof r->err, "greplinestree: empty path");
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
      snprintf(r->err, sizeof r->err, "greplinestree: missing");
      return -1;
    }
    snprintf(r->err, sizeof r->err, "greplinestree: %s", strerror(errno));
    return -1;
  }
  r->str[0] = 0;
  if (S_ISREG(st.st_mode)) {
    cubalc_greplinestree_file(abs, needle, icase, r->str, sizeof r->str, &olen,
                              &nlines, &files_hit);
  } else if (S_ISDIR(st.st_mode)
#if defined(S_ISLNK)
             && !S_ISLNK(st.st_mode)
#endif
             ) {
    cubalc_greplinestree_rec(abs, needle, icase, r->str, sizeof r->str, &olen,
                             &nlines, &files_hit, 0);
  } else {
    snprintf(r->err, sizeof r->err, "greplinestree: not a file or directory");
    return -1;
  }
  r->n = nlines;
  r->code = (int)(files_hit > 0x7fffffff ? 0x7fffffff : files_hit);
  r->ok = 1;
  return 0;
}

/* Extract matching lines as path:lineno:line (1-based lineno, grep -rn style). */
static void cubalc_pathlinestree_file(const char *path, const char *needle, int icase,
                                     char *out, size_t outsz, size_t *olen, long *nlines,
                                     long *files_hit) {
  cubalc_host_result hr;
  const char *p, *ls;
  long file_hits = 0;
  int lineno = 0;
  memset(&hr, 0, sizeof hr);
  if (!path || !path[0] || !out || !olen || !nlines) return;
  if (cubalc_host_read(path, &hr) != 0) return;
  p = hr.str;
  while (*p) {
    size_t llen;
    char line[CUBALC_HOST_STR_MAX];
    char entry[CUBALC_HOST_STR_MAX];
    int match = 0;
    ls = p;
    while (*p && *p != '\n') p++;
    llen = (size_t)(p - ls);
    if (llen >= sizeof line) llen = sizeof line - 1;
    lineno++;
    if (llen > 0) {
      memcpy(line, ls, llen);
      line[llen] = 0;
      match = cubalc_line_matches(line, needle, icase);
    } else if (needle && !needle[0]) {
      line[0] = 0;
      match = 1;
    } else {
      line[0] = 0;
      match = 0;
    }
    if (match) {
      /* path:lineno:line — path is absolute and has no newlines */
      snprintf(entry, sizeof entry, "%s:%d:%s", path, lineno, line);
      cubalc_bag_push(out, outsz, olen, nlines, entry);
      file_hits++;
    }
    if (*p == '\n') p++;
  }
  if (file_hits > 0 && files_hit) (*files_hit)++;
}

static void cubalc_pathlinestree_rec(const char *dir, const char *needle, int icase,
                                    char *out, size_t outsz, size_t *olen, long *nlines,
                                    long *files_hit, int depth) {
  DIR *d;
  struct dirent *ent;
  struct stat st;
  if (!dir || !dir[0] || depth > 64 || !out || !olen || !nlines) return;
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
      cubalc_pathlinestree_rec(full, needle, icase, out, outsz, olen, nlines,
                               files_hit, depth + 1);
      continue;
    }
    if (!S_ISREG(st.st_mode)) continue;
    cubalc_pathlinestree_file(full, needle, icase, out, outsz, olen, nlines, files_hit);
  }
  closedir(d);
}

/* Usability: SYS PATHLINETREE|GREPNTREE root needle — recursive path:line:text harvest
 * (agent grep -rn) without WALK+EACH+READ. r->str bag; r->n lines; r->code files. */
int cubalc_host_pathlines_tree(const char *root, const char *needle, int icase,
                               cubalc_host_result *r) {
  char abs[CUBALC_HOST_STR_MAX];
  size_t olen = 0;
  long nlines = 0, files_hit = 0;
  struct stat st;
  cubalc_host_result hr;
  r_clear(r);
  if (!root || !root[0]) {
    snprintf(r->err, sizeof r->err, "pathlinestree: empty path");
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
      snprintf(r->err, sizeof r->err, "pathlinestree: missing");
      return -1;
    }
    snprintf(r->err, sizeof r->err, "pathlinestree: %s", strerror(errno));
    return -1;
  }
  r->str[0] = 0;
  if (S_ISREG(st.st_mode)) {
    cubalc_pathlinestree_file(abs, needle, icase, r->str, sizeof r->str, &olen,
                              &nlines, &files_hit);
  } else if (S_ISDIR(st.st_mode)
#if defined(S_ISLNK)
             && !S_ISLNK(st.st_mode)
#endif
             ) {
    cubalc_pathlinestree_rec(abs, needle, icase, r->str, sizeof r->str, &olen,
                             &nlines, &files_hit, 0);
  } else {
    snprintf(r->err, sizeof r->err, "pathlinestree: not a file or directory");
    return -1;
  }
  r->n = nlines;
  r->code = (int)(files_hit > 0x7fffffff ? 0x7fffffff : files_hit);
  r->ok = 1;
  return 0;
}

