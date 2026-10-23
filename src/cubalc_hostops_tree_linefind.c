/* CubalC — hostops tree line-find peel (B_hostops_margin).
 * FIRSTLINEINTREE·LASTLINEINTREE. Mesh vote · zero bloat · cube is SoT.
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

static int cubalc_line_matches(const char *line, const char *needle, int icase) {
  size_t nlen;
  if (!line) return 0;
  if (!needle || !needle[0]) return 1;
  nlen = strlen(needle);
  if (!icase) return strstr(line, needle) != NULL;
  return cubalc_str_has_icase(line, needle);
}

/* FINDINTREE·TAKEINTREE·NTHINTREE·SLICEINTREE·DROPINTREE → cubalc_hostops_tree_find.c (B_hostops_margin) */
/* name_cmp kept for FIRSTLINEINTREE plane below */
static int cubalc_name_cmp(const void *a, const void *b) {
  return strcmp((const char *)a, (const char *)b);
}


/* Peel first/last matching line from one file. Returns 1 if hit.
 * line_out gets line text; *line_idx is 0-based index within file. */
static int cubalc_file_findline(const char *path, const char *needle, int icase,
                                int want_last, char *line_out, size_t linen,
                                int *line_idx) {
  cubalc_host_result hr;
  const char *p, *ls;
  int idx = 0, hit = 0;
  memset(&hr, 0, sizeof hr);
  if (!path || !path[0] || !line_out || linen == 0) return 0;
  if (cubalc_host_read(path, &hr) != 0) return 0;
  line_out[0] = 0;
  if (line_idx) *line_idx = -1;
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
        snprintf(line_out, linen, "%s", line);
        if (line_idx) *line_idx = idx;
        hit = 1;
        if (!want_last) return 1;
      }
    } else if (needle && !needle[0]) {
      line_out[0] = 0;
      if (line_idx) *line_idx = idx;
      hit = 1;
      if (!want_last) return 1;
    }
    if (*p == '\n') p++;
    idx++;
  }
  return hit;
}

/* Sorted DFS: first/last matching line under dir. Updates path/line/idx.
 * Returns 1 if any hit found (for last: keeps walking). */
static int cubalc_findlineintree_rec(const char *dir, const char *needle, int icase,
                                     int want_last, char *path_out, size_t pathn,
                                     char *line_out, size_t linen, int *line_idx,
                                     int depth) {
  DIR *d;
  struct dirent *ent;
  struct stat st;
  int any = 0, nnames = 0, i;
  char names[256][256];
  if (!dir || !dir[0] || depth > 64 || !path_out || !line_out) return 0;
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
    char line[CUBALC_HOST_STR_MAX];
    int lidx = -1;
    if (cubalc_join_child(dir, names[i], full, sizeof full) != 0)
      continue;
    if (lstat(full, &st) != 0) continue;
    if (S_ISDIR(st.st_mode)
#if defined(S_ISLNK)
        && !S_ISLNK(st.st_mode)
#endif
        ) {
      if (cubalc_findlineintree_rec(full, needle, icase, want_last, path_out, pathn,
                                    line_out, linen, line_idx, depth + 1)) {
        any = 1;
        if (!want_last) return 1;
      }
      continue;
    }
    if (!S_ISREG(st.st_mode)) continue;
    if (cubalc_file_findline(full, needle, icase, want_last, line, sizeof line, &lidx)) {
      snprintf(path_out, pathn, "%s", full);
      snprintf(line_out, linen, "%s", line);
      if (line_idx) *line_idx = lidx;
      any = 1;
      if (!want_last) return 1;
    }
  }
  return any;
}

/* Usability: SYS FIRSTLINEINTREE|LASTLINEINTREE root needle — first/last matching line
 * under tree without GREPLINESTREE+TAKE/REVL glue.
 * On hit: r->str = "path\\nline", r->n=1, r->code=0-based line index. Soft miss → -1. */
int cubalc_host_findlineintree(const char *root, const char *needle, int icase,
                               int want_last, cubalc_host_result *r) {
  char abs[CUBALC_HOST_STR_MAX], path[CUBALC_HOST_STR_MAX], line[CUBALC_HOST_STR_MAX];
  struct stat st;
  cubalc_host_result hr;
  int hit = 0, lidx = -1;
  r_clear(r);
  if (!root || !root[0]) {
    snprintf(r->err, sizeof r->err, "findlineintree: empty path");
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
      snprintf(r->err, sizeof r->err, "findlineintree: missing");
      return -1;
    }
    snprintf(r->err, sizeof r->err, "findlineintree: %s", strerror(errno));
    return -1;
  }
  path[0] = 0;
  line[0] = 0;
  if (S_ISREG(st.st_mode)) {
    if (cubalc_file_findline(abs, needle, icase, want_last, line, sizeof line, &lidx)) {
      snprintf(path, sizeof path, "%s", abs);
      hit = 1;
    }
  } else if (S_ISDIR(st.st_mode)
#if defined(S_ISLNK)
             && !S_ISLNK(st.st_mode)
#endif
             ) {
    hit = cubalc_findlineintree_rec(abs, needle, icase, want_last, path, sizeof path,
                                    line, sizeof line, &lidx, 0);
  } else {
    snprintf(r->err, sizeof r->err, "findlineintree: not a file or directory");
    return -1;
  }
  if (hit && path[0]) {
    /* path never contains newline; line is stripped of \\n */
    size_t pl = strlen(path), ll = strlen(line);
    if (pl + 1 + ll + 1 > sizeof r->str) {
      /* truncate line to fit */
      size_t maxl = (sizeof r->str > pl + 2) ? (sizeof r->str - pl - 2) : 0;
      if (ll > maxl) ll = maxl;
    }
    if (pl + 1 + ll + 1 <= sizeof r->str) {
      memcpy(r->str, path, pl);
      r->str[pl] = '\n';
      memcpy(r->str + pl + 1, line, ll);
      r->str[pl + 1 + ll] = 0;
    } else {
      snprintf(r->str, sizeof r->str, "%s", path);
    }
    r->n = 1;
    r->code = lidx;
  } else {
    r->str[0] = 0;
    r->n = 0;
    r->code = -1;
  }
  r->ok = 1;
  return 0;
}

