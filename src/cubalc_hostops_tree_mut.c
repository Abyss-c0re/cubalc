/* CubalC — hostops tree mut peel (B_hostops_margin).
 * RMTREE·CPTREE·MVTREE·DIRSIZE. Mesh vote · zero bloat · cube is SoT.
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

static void r_clear(cubalc_host_result *r) {
  if (!r) return;
  memset(r, 0, sizeof *r);
}

/* Depth-first recursive remove. Does not follow dir symlinks (unlink link).
 * Caps recursion to avoid cycles. Returns count of removed entries via *count. */
static int cubalc_rmtree_rec(const char *path, long *count, int depth) {
  struct stat st;
  DIR *d;
  struct dirent *ent;
  if (!path || !path[0] || !count) return -1;
  if (depth > 64) {
    errno = ELOOP;
    return -1;
  }
  /* lstat: treat symlink as leaf (unlink, do not descend) */
  if (lstat(path, &st) != 0) {
    if (errno == ENOENT) return 0;
    return -1;
  }
  if (S_ISDIR(st.st_mode)
#if defined(S_ISLNK)
      && !S_ISLNK(st.st_mode)
#endif
      ) {
    d = opendir(path);
    if (!d) return -1;
    while ((ent = readdir(d)) != NULL) {
      char child[CUBALC_HOST_STR_MAX];
      size_t pl, nl;
      if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
        continue;
      pl = strlen(path);
      nl = strlen(ent->d_name);
      if (pl + 1 + nl + 1 >= sizeof child) {
        closedir(d);
        errno = ENAMETOOLONG;
        return -1;
      }
      if (pl > 0 && (path[pl - 1] == '/' || path[pl - 1] == '\\'))
        snprintf(child, sizeof child, "%s%s", path, ent->d_name);
      else
        snprintf(child, sizeof child, "%s/%s", path, ent->d_name);
      if (cubalc_rmtree_rec(child, count, depth + 1) != 0) {
        closedir(d);
        return -1;
      }
    }
    closedir(d);
    if (rmdir(path) != 0) return -1;
    (*count)++;
    return 0;
  }
  /* file, symlink, fifo, etc. */
  if (unlink(path) != 0) return -1;
  (*count)++;
  return 0;
}

/* Usability: SYS RMTREE|RMNEST path — recursive plate tree delete without shell rm -rf.
 * File → unlink; dir → depth-first children then rmdir. Missing → soft ok n=0. */
int cubalc_host_rmtree(const char *path, cubalc_host_result *r) {
  long count = 0;
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "rmtree: empty path");
    return -1;
  }
  if (lstat(path, &st) != 0) {
    if (errno == ENOENT) {
      snprintf(r->str, sizeof r->str, "%s", path);
      r->n = 0;
      r->ok = 1;
      return 0;
    }
    snprintf(r->err, sizeof r->err, "rmtree: %s", strerror(errno));
    return -1;
  }
  if (cubalc_rmtree_rec(path, &count, 0) != 0) {
    snprintf(r->err, sizeof r->err, "rmtree: %s", strerror(errno));
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", path);
  r->n = count;
  r->ok = 1;
  return 0;
}

/* Refuse dest inside src (would recurse forever). Same path ok only for file copy. */
static int cubalc_path_is_under(const char *parent, const char *child) {
  size_t n;
  if (!parent || !child || !parent[0] || !child[0]) return 0;
  n = strlen(parent);
  if (strncmp(child, parent, n) != 0) return 0;
  if (child[n] == 0) return 1; /* equal */
  if (child[n] == '/' || child[n] == '\\') return 1;
  return 0;
}

/* Recursive copy: file → cubalc_host_copy; dir → mkdir dest + children.
 * *count entries, *bytes file payload. Does not follow dir symlinks (copy as file if possible). */
static int cubalc_cptree_rec(const char *src, const char *dst,
                             long *count, long *bytes, int depth) {
  struct stat st;
  DIR *d;
  struct dirent *ent;
  cubalc_host_result hr;
  if (!src || !src[0] || !dst || !dst[0] || !count || !bytes) return -1;
  if (depth > 64) {
    errno = ELOOP;
    return -1;
  }
  if (lstat(src, &st) != 0) {
    errno = errno ? errno : ENOENT;
    return -1;
  }
  if (S_ISDIR(st.st_mode)
#if defined(S_ISLNK)
      && !S_ISLNK(st.st_mode)
#endif
      ) {
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(dst, &hr) != 0) {
      errno = EIO;
      return -1;
    }
    (*count)++;
    d = opendir(src);
    if (!d) return -1;
    while ((ent = readdir(d)) != NULL) {
      char schild[CUBALC_HOST_STR_MAX], dchild[CUBALC_HOST_STR_MAX];
      size_t sl, dl, nl;
      if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
        continue;
      sl = strlen(src);
      dl = strlen(dst);
      nl = strlen(ent->d_name);
      if (sl + 1 + nl + 1 >= sizeof schild || dl + 1 + nl + 1 >= sizeof dchild) {
        closedir(d);
        errno = ENAMETOOLONG;
        return -1;
      }
      if (sl > 0 && (src[sl - 1] == '/' || src[sl - 1] == '\\'))
        snprintf(schild, sizeof schild, "%s%s", src, ent->d_name);
      else
        snprintf(schild, sizeof schild, "%s/%s", src, ent->d_name);
      if (dl > 0 && (dst[dl - 1] == '/' || dst[dl - 1] == '\\'))
        snprintf(dchild, sizeof dchild, "%s%s", dst, ent->d_name);
      else
        snprintf(dchild, sizeof dchild, "%s/%s", dst, ent->d_name);
      if (cubalc_cptree_rec(schild, dchild, count, bytes, depth + 1) != 0) {
        closedir(d);
        return -1;
      }
    }
    closedir(d);
    return 0;
  }
  /* file / symlink / other: copy bytes via host_copy (follows symlink content) */
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_copy(src, dst, &hr) != 0) {
    errno = EIO;
    return -1;
  }
  (*count)++;
  *bytes += hr.n;
  return 0;
}

/* Usability: SYS CPTREE|COPYTREE src dest — recursive plate tree snapshot without shell cp -r.
 * File → COPY; dir → mkdir dest + children. Soft miss src. r->n=entries, r->code=bytes. */
int cubalc_host_cptree(const char *src, const char *dst, cubalc_host_result *r) {
  long count = 0, bytes = 0;
  struct stat st;
  char asrc[CUBALC_HOST_STR_MAX], adst[CUBALC_HOST_STR_MAX];
  cubalc_host_result hr;
  r_clear(r);
  if (!src || !src[0] || !dst || !dst[0]) {
    snprintf(r->err, sizeof r->err, "cptree: empty path");
    return -1;
  }
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_abspath(src, &hr) == 0 && hr.str[0])
    snprintf(asrc, sizeof asrc, "%s", hr.str);
  else
    snprintf(asrc, sizeof asrc, "%s", src);
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_abspath(dst, &hr) == 0 && hr.str[0])
    snprintf(adst, sizeof adst, "%s", hr.str);
  else
    snprintf(adst, sizeof adst, "%s", dst);
  if (lstat(asrc, &st) != 0) {
    if (errno == ENOENT) {
      snprintf(r->err, sizeof r->err, "cptree: missing source");
      return -1;
    }
    snprintf(r->err, sizeof r->err, "cptree: %s", strerror(errno));
    return -1;
  }
  /* dest under src dir would infinite-loop */
  if (S_ISDIR(st.st_mode) && cubalc_path_is_under(asrc, adst) &&
      strcmp(asrc, adst) != 0) {
    snprintf(r->err, sizeof r->err, "cptree: dest inside src");
    return -1;
  }
  if (strcmp(asrc, adst) == 0 && !S_ISDIR(st.st_mode)) {
    /* file self-copy: treat as ok no-op like host_copy */
    snprintf(r->str, sizeof r->str, "%s", adst);
    r->n = 1;
    r->code = (int)st.st_size;
    r->ok = 1;
    return 0;
  }
  if (strcmp(asrc, adst) == 0 && S_ISDIR(st.st_mode)) {
    snprintf(r->err, sizeof r->err, "cptree: dest is src");
    return -1;
  }
  if (cubalc_cptree_rec(asrc, adst, &count, &bytes, 0) != 0) {
    if (r->err[0] == 0)
      snprintf(r->err, sizeof r->err, "cptree: %s",
               errno ? strerror(errno) : "fail");
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", adst);
  r->n = count;
  r->code = (int)(bytes > 0x7fffffff ? 0x7fffffff : bytes);
  r->ok = 1;
  return 0;
}

/* Parent directory of path into out (may be "." or "/"). */
static void cubalc_path_parent(const char *path, char *out, size_t outn) {
  char work[CUBALC_HOST_STR_MAX];
  const char *slash;
  size_t n;
  if (!out || outn == 0) return;
  out[0] = 0;
  if (!path || !path[0]) {
    snprintf(out, outn, "%s", ".");
    return;
  }
  snprintf(work, sizeof work, "%s", path);
  n = strlen(work);
  while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
    work[n - 1] = 0;
    n--;
  }
  slash = strrchr(work, '/');
#if defined(CUBALC_OS_WINDOWS)
  {
    const char *b = strrchr(work, '\\');
    if (b && (!slash || b > slash)) slash = b;
  }
#endif
  if (slash && slash != work) {
    size_t dn = (size_t)(slash - work);
    if (dn >= outn) dn = outn - 1;
    memcpy(out, work, dn);
    out[dn] = 0;
  } else if (slash && slash == work) {
    snprintf(out, outn, "%c", work[0] == '\\' ? '\\' : '/');
  } else {
    snprintf(out, outn, "%s", ".");
  }
}

/* Usability: SYS MVTREE|MOVETREE src dest — promote/relocate tree without shell mv -r.
 * Prefer rename(2); on EXDEV/fail → CPTREE then RMTREE src. Soft miss src.
 * r->n = entries (1 if rename-hit); r->code = 1 renamed / 0 copy+delete. */
int cubalc_host_mvtree(const char *src, const char *dst, cubalc_host_result *r) {
  struct stat st;
  char asrc[CUBALC_HOST_STR_MAX], adst[CUBALC_HOST_STR_MAX], parent[CUBALC_HOST_STR_MAX];
  cubalc_host_result hr;
  r_clear(r);
  if (!src || !src[0] || !dst || !dst[0]) {
    snprintf(r->err, sizeof r->err, "mvtree: empty path");
    return -1;
  }
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_abspath(src, &hr) == 0 && hr.str[0])
    snprintf(asrc, sizeof asrc, "%s", hr.str);
  else
    snprintf(asrc, sizeof asrc, "%s", src);
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_abspath(dst, &hr) == 0 && hr.str[0])
    snprintf(adst, sizeof adst, "%s", hr.str);
  else
    snprintf(adst, sizeof adst, "%s", dst);
  if (lstat(asrc, &st) != 0) {
    if (errno == ENOENT) {
      snprintf(r->err, sizeof r->err, "mvtree: missing source");
      return -1;
    }
    snprintf(r->err, sizeof r->err, "mvtree: %s", strerror(errno));
    return -1;
  }
  if (strcmp(asrc, adst) == 0) {
    snprintf(r->str, sizeof r->str, "%s", adst);
    r->n = 1;
    r->code = 1; /* already in place */
    r->ok = 1;
    return 0;
  }
  if (S_ISDIR(st.st_mode) && cubalc_path_is_under(asrc, adst)) {
    snprintf(r->err, sizeof r->err, "mvtree: dest inside src");
    return -1;
  }
  /* ensure parent of dest exists */
  cubalc_path_parent(adst, parent, sizeof parent);
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_mkdir(parent, &hr) != 0) {
    snprintf(r->err, sizeof r->err, "mvtree: mkdir parent failed");
    return -1;
  }
  /* try atomic rename first */
  if (rename(asrc, adst) == 0) {
    snprintf(r->str, sizeof r->str, "%s", adst);
    r->n = 1;
    r->code = 1;
    r->ok = 1;
    return 0;
  }
  /* cross-device or dest conflict: copy then remove source */
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_cptree(asrc, adst, &hr) != 0) {
    snprintf(r->err, sizeof r->err, "%s",
             hr.err[0] ? hr.err : "mvtree: copy failed");
    return -1;
  }
  {
    long n = hr.n;
    int bytes = hr.code;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_rmtree(asrc, &hr) != 0) {
      snprintf(r->err, sizeof r->err, "%s",
               hr.err[0] ? hr.err : "mvtree: remove src failed");
      return -1;
    }
    snprintf(r->str, sizeof r->str, "%s", adst);
    r->n = n;
    r->code = 0; /* copy+delete path */
    (void)bytes;
    r->ok = 1;
    return 0;
  }
}

/* Depth-first size walk. *bytes / *files / *dirs accumulators. No dir-symlink follow. */
static int cubalc_dirsize_rec(const char *path, long *bytes, long *files, long *dirs,
                              int depth) {
  struct stat st;
  DIR *d;
  struct dirent *ent;
  if (!path || !path[0] || !bytes || !files || !dirs) return -1;
  if (depth > 64) {
    errno = ELOOP;
    return -1;
  }
  if (lstat(path, &st) != 0) return -1;
  if (S_ISDIR(st.st_mode)
#if defined(S_ISLNK)
      && !S_ISLNK(st.st_mode)
#endif
      ) {
    (*dirs)++;
    d = opendir(path);
    if (!d) return -1;
    while ((ent = readdir(d)) != NULL) {
      char child[CUBALC_HOST_STR_MAX];
      size_t pl, nl;
      if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
        continue;
      pl = strlen(path);
      nl = strlen(ent->d_name);
      if (pl + 1 + nl + 1 >= sizeof child) {
        closedir(d);
        errno = ENAMETOOLONG;
        return -1;
      }
      if (pl > 0 && (path[pl - 1] == '/' || path[pl - 1] == '\\'))
        snprintf(child, sizeof child, "%s%s", path, ent->d_name);
      else
        snprintf(child, sizeof child, "%s/%s", path, ent->d_name);
      if (cubalc_dirsize_rec(child, bytes, files, dirs, depth + 1) != 0) {
        closedir(d);
        return -1;
      }
    }
    closedir(d);
    return 0;
  }
  /* regular file / symlink / special: count size of the node (symlink: length of link) */
  (*files)++;
  *bytes += (long)st.st_size;
  return 0;
}

/* Usability: SYS DIRSIZE|DU|TREESIZE path — recursive byte inventory without shell du.
 * r->n = total bytes; r->code = (files & 0xffff) | ((dirs & 0xffff) << 16); soft miss. */
int cubalc_host_dirsize(const char *path, cubalc_host_result *r) {
  long bytes = 0, files = 0, dirs = 0;
  char abs[CUBALC_HOST_STR_MAX];
  struct stat st;
  cubalc_host_result hr;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "dirsize: empty path");
    return -1;
  }
  memset(&hr, 0, sizeof hr);
  if (cubalc_host_abspath(path, &hr) == 0 && hr.str[0])
    snprintf(abs, sizeof abs, "%s", hr.str);
  else
    snprintf(abs, sizeof abs, "%s", path);
  if (lstat(abs, &st) != 0) {
    if (errno == ENOENT) {
      snprintf(r->err, sizeof r->err, "dirsize: missing");
      return -1;
    }
    snprintf(r->err, sizeof r->err, "dirsize: %s", strerror(errno));
    return -1;
  }
  if (cubalc_dirsize_rec(abs, &bytes, &files, &dirs, 0) != 0) {
    snprintf(r->err, sizeof r->err, "dirsize: %s",
             errno ? strerror(errno) : "fail");
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", abs);
  r->n = bytes;
  {
    unsigned fc = files > 0xffff ? 0xffff : (unsigned)files;
    unsigned dc = dirs > 0xffff ? 0xffff : (unsigned)dirs;
    r->code = (int)(fc | (dc << 16));
  }
  r->ok = 1;
  return 0;
}

