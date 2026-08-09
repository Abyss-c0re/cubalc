/* glibc: realpath + POSIX APIs without macro fights */
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
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <dirent.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <pwd.h>
#  include <grp.h>
#endif

static void r_clear(cubalc_host_result *r) {
  if (!r) return;
  memset(r, 0, sizeof *r);
}

/* Core protect: CUBALC_PROTECT=1 tightens host edge (fail-closed for Core stability). */
static int protect_mode(void) {
  const char *p = getenv("CUBALC_PROTECT");
  return (p && p[0] && p[0] != '0') ? 1 : 0;
}

static int url_allowed(const char *url) {
  if (!url) return 0;
  if (strncmp(url, "http://127.0.0.1", 16) == 0) return 1;
  if (strncmp(url, "http://localhost", 16) == 0) return 1;
  if (strncmp(url, "http://[::1]", 12) == 0) return 1;
  /* protect mode: loopback only unless explicit protect-http override */
  if (protect_mode()) {
    const char *ph = getenv("CUBALC_PROTECT_HTTP");
    if (!(ph && ph[0] && ph[0] != '0')) return 0;
  }
  if (strncmp(url, "https://api.x.ai/", 17) == 0) return 1;
  const char *extra = getenv("CUBALC_HTTP_ALLOW");
  if (extra && extra[0] && strstr(url, extra)) return 1;
  return 0;
}

static int base_in_allowlist(const char *base, const char *list) {
  /* colon-separated basenames, e.g. "nanobot:extra:tool" */
  if (!base || !list || !list[0]) return 0;
  const char *p = list;
  size_t bl = strlen(base);
  while (*p) {
    while (*p == ':' || *p == ',' || *p == ' ') p++;
    if (!*p) break;
    const char *start = p;
    while (*p && *p != ':' && *p != ',' && *p != ' ') p++;
    size_t n = (size_t)(p - start);
    if (n == bl && strncmp(start, base, bl) == 0) return 1;
  }
  return 0;
}

static int bin_allowed(const char *bin) {
  if (!bin || !bin[0]) return 0;
  const char *base = strrchr(bin, '/');
  base = base ? base + 1 : bin;
  /* core hive tools only — extend via CUBALC_SPAWN_ALLOW (no device hardcode) */
  /* protect mode: nanobot + cubalc only (curl needs CUBALC_PROTECT_HTTP=1) */
  if (strcmp(base, "nanobot") == 0) return 1;
  if (strcmp(base, "cubalc") == 0) return 1;
  if (strcmp(base, "curl") == 0) {
    if (protect_mode()) {
      const char *ph = getenv("CUBALC_PROTECT_HTTP");
      if (!(ph && ph[0] && ph[0] != '0')) return 0;
    }
    return 1; /* internal HTTP helper */
  }
  if (protect_mode()) {
    /* fail-closed: no extra SPAWN basenames under Core protect */
    return 0;
  }
  const char *extra = getenv("CUBALC_SPAWN_ALLOW");
  if (base_in_allowlist(base, extra)) return 1;
  /* absolute path only if basename already allowed */
  if (bin[0] == '/' &&
      (strcmp(base, "nanobot") == 0 || strcmp(base, "cubalc") == 0 ||
       strcmp(base, "curl") == 0 || base_in_allowlist(base, extra)))
    return 1;
  return 0;
}

int cubalc_host_exists(const char *path) {
  struct stat st;
  if (!path || !path[0]) return 0;
  return stat(path, &st) == 0 ? 1 : 0;
}

/* Usability: SYS SIZE / ISDIR / ISFILE — path metadata without full READ.
 * r->code: 0 missing, 1 regular file, 2 directory, 3 other
 * r->n: byte size for regular files (0 for dirs/other/missing) */
int cubalc_host_path_kind(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "path: empty");
    r->code = 0;
    r->n = 0;
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "path: missing");
    r->code = 0;
    r->n = 0;
    r->ok = 0;
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", path);
  if (S_ISREG(st.st_mode)) {
    r->code = 1;
    r->n = (long)st.st_size;
    r->ok = 1;
    return 0;
  }
  if (S_ISDIR(st.st_mode)) {
    r->code = 2;
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  r->code = 3;
  r->n = (long)st.st_size;
  r->ok = 1;
  return 0;
}

/* Usability: SYS MTIME / AGE — plate freshness without shell stat.
 * r->n = st_mtime (epoch seconds). Soft miss: ok=0 code=0. */
int cubalc_host_mtime(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "mtime: empty path");
    r->code = 0;
    r->n = 0;
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "mtime: missing");
    r->code = 0;
    r->n = 0;
    r->ok = 0;
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", path);
  r->n = (long)st.st_mtime;
  r->ok = 1;
  if (S_ISREG(st.st_mode)) r->code = 1;
  else if (S_ISDIR(st.st_mode)) r->code = 2;
  else r->code = 3;
  return 0;
}

/* Usability: SYS ATIME path — last access epoch without shell stat -c %X. */
int cubalc_host_atime(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "atime: empty path");
    r->code = 0;
    r->n = 0;
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "atime: missing");
    r->code = 0;
    r->n = 0;
    r->ok = 0;
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", path);
  r->n = (long)st.st_atime;
  r->ok = 1;
  if (S_ISREG(st.st_mode)) r->code = 1;
  else if (S_ISDIR(st.st_mode)) r->code = 2;
  else r->code = 3;
  return 0;
}

/* Usability: SYS CTIME path — inode status-change epoch without shell stat -c %Z. */
int cubalc_host_ctime(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "ctime: empty path");
    r->code = 0;
    r->n = 0;
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "ctime: missing");
    r->code = 0;
    r->n = 0;
    r->ok = 0;
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", path);
  r->n = (long)st.st_ctime;
  r->ok = 1;
  if (S_ISREG(st.st_mode)) r->code = 1;
  else if (S_ISDIR(st.st_mode)) r->code = 2;
  else r->code = 3;
  return 0;
}

int cubalc_host_read(const char *path, cubalc_host_result *r) {
  r_clear(r);
  if (!path || !path[0]) { snprintf(r->err, sizeof r->err, "read: empty path"); return -1; }
  FILE *f = fopen(path, "rb");
  if (!f) { snprintf(r->err, sizeof r->err, "read: %s", strerror(errno)); return -1; }
  size_t n = fread(r->str, 1, sizeof r->str - 1, f);
  r->str[n] = 0;
  fclose(f);
  r->n = (long)n;
  r->ok = 1;
  r->code = 0;
  return 0;
}

int cubalc_host_write(const char *path, const char *data, cubalc_host_result *r) {
  r_clear(r);
  if (!path || !path[0]) { snprintf(r->err, sizeof r->err, "write: empty path"); return -1; }
  FILE *f = fopen(path, "wb");
  if (!f) { snprintf(r->err, sizeof r->err, "write: %s", strerror(errno)); return -1; }
  const char *d = data ? data : "";
  size_t n = fwrite(d, 1, strlen(d), f);
  fclose(f);
  r->n = (long)n;
  r->ok = 1;
  return 0;
}

/* Usability: SYS RM path — delete regular file plate; missing is soft OK. */
int cubalc_host_rm(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "rm: empty path");
    return -1;
  }
  if (stat(path, &st) != 0) {
    /* idempotent: already gone */
    snprintf(r->str, sizeof r->str, "%s", path);
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  if (S_ISDIR(st.st_mode)) {
    snprintf(r->err, sizeof r->err, "rm: is a directory");
    return -1;
  }
  if (unlink(path) != 0) {
    snprintf(r->err, sizeof r->err, "rm: %s", strerror(errno));
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", path);
  r->n = 1; /* removed */
  r->ok = 1;
  return 0;
}

/* Usability: SYS RMDIR path — remove empty directory; missing soft OK.
 * Non-directory or non-empty → error (no recursive delete). */
int cubalc_host_rmdir(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "rmdir: empty path");
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->str, sizeof r->str, "%s", path);
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  if (!S_ISDIR(st.st_mode)) {
    snprintf(r->err, sizeof r->err, "rmdir: not a directory");
    return -1;
  }
  if (rmdir(path) != 0) {
    snprintf(r->err, sizeof r->err, "rmdir: %s", strerror(errno));
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", path);
  r->n = 1;
  r->ok = 1;
  return 0;
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

/* File content contains needle? Empty needle → yes if readable. */
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

/* Usability: SYS RENAME|MV from to — move plate without shell. */
int cubalc_host_rename(const char *from, const char *to, cubalc_host_result *r) {
  r_clear(r);
  if (!from || !from[0] || !to || !to[0]) {
    snprintf(r->err, sizeof r->err, "rename: empty path");
    return -1;
  }
  if (access(from, F_OK) != 0) {
    snprintf(r->err, sizeof r->err, "rename: missing source");
    return -1;
  }
  if (rename(from, to) != 0) {
    snprintf(r->err, sizeof r->err, "rename: %s", strerror(errno));
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", to);
  r->n = 1;
  r->ok = 1;
  return 0;
}

/* Usability: SYS SYMLINK|LN target linkpath — plate alias without shell ln -s. */
int cubalc_host_symlink(const char *target, const char *linkpath, cubalc_host_result *r) {
  r_clear(r);
  if (!target || !target[0] || !linkpath || !linkpath[0]) {
    snprintf(r->err, sizeof r->err, "symlink: empty path");
    return -1;
  }
  if (access(linkpath, F_OK) == 0) {
    snprintf(r->err, sizeof r->err, "symlink: link exists");
    return -1;
  }
  if (symlink(target, linkpath) != 0) {
    snprintf(r->err, sizeof r->err, "symlink: %s", strerror(errno));
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", linkpath);
  r->n = 1;
  r->ok = 1;
  return 0;
}

/* Usability: SYS HARDLINK existing newpath — shared plate inode without copy/shell ln. */
int cubalc_host_hardlink(const char *existing, const char *newpath, cubalc_host_result *r) {
  r_clear(r);
  if (!existing || !existing[0] || !newpath || !newpath[0]) {
    snprintf(r->err, sizeof r->err, "hardlink: empty path");
    return -1;
  }
  if (access(existing, F_OK) != 0) {
    snprintf(r->err, sizeof r->err, "hardlink: source missing");
    return -1;
  }
  if (access(newpath, F_OK) == 0) {
    snprintf(r->err, sizeof r->err, "hardlink: dest exists");
    return -1;
  }
  if (link(existing, newpath) != 0) {
    snprintf(r->err, sizeof r->err, "hardlink: %s", strerror(errno));
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", newpath);
  r->n = 1;
  r->ok = 1;
  return 0;
}

/* Usability: SYS NLINK path — hard-link count for shared-plate IF guards. */
int cubalc_host_nlink(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "nlink: empty path");
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "nlink: missing");
    return -1;
  }
  r->n = (long)st.st_nlink;
  snprintf(r->str, sizeof r->str, "%ld", r->n);
  r->ok = 1;
  return 0;
}

/* Usability: SYS SAMEINODE a b — true if HARDLINK pair (dev+ino), not content EQFILE. */
int cubalc_host_sameinode(const char *a, const char *b, cubalc_host_result *r) {
  struct stat sa, sb;
  r_clear(r);
  if (!a || !a[0] || !b || !b[0]) {
    snprintf(r->err, sizeof r->err, "sameinode: empty path");
    return -1;
  }
  if (stat(a, &sa) != 0 || stat(b, &sb) != 0) {
    snprintf(r->err, sizeof r->err, "sameinode: missing");
    return -1;
  }
  r->n = (sa.st_dev == sb.st_dev && sa.st_ino == sb.st_ino) ? 1 : 0;
  snprintf(r->str, sizeof r->str, "%ld", r->n);
  r->ok = 1;
  return 0;
}

/* Usability: SYS INODE path — plate identity stamp without shell stat. */
int cubalc_host_inode(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "inode: empty path");
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "inode: missing");
    return -1;
  }
  r->n = (long)st.st_ino;
  snprintf(r->str, sizeof r->str, "%ld", r->n);
  r->ok = 1;
  return 0;
}

/* Usability: SYS FILEUID path — plate owner uid without shell stat. */
int cubalc_host_fileuid(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "fileuid: empty path");
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "fileuid: missing");
    return -1;
  }
  r->n = (long)st.st_uid;
  snprintf(r->str, sizeof r->str, "%ld", r->n);
  r->ok = 1;
  return 0;
}

/* Usability: SYS FILEGID path — plate group gid without shell stat. */
int cubalc_host_filegid(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "filegid: empty path");
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "filegid: missing");
    return -1;
  }
  r->n = (long)st.st_gid;
  snprintf(r->str, sizeof r->str, "%ld", r->n);
  r->ok = 1;
  return 0;
}

/* Usability: SYS CANREAD|CANWRITE|CANEXEC path — access(2) probe without soft-fail glue. */
int cubalc_host_can_access(const char *path, int mode, cubalc_host_result *r) {
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "access: empty path");
    return -1;
  }
  r->n = (access(path, mode) == 0) ? 1 : 0;
  snprintf(r->str, sizeof r->str, "%ld", r->n);
  r->ok = 1;
  return 0;
}

/* Usability: SYS CANCREATE path — pre-flight create/overwrite without soft WRITE fail.
 * Existing path: W_OK on the path itself (file or dir).
 * Missing path: W_OK|X_OK on parent directory (basename strip). */
int cubalc_host_can_create(const char *path, cubalc_host_result *r) {
  char parent[CUBALC_HOST_STR_MAX];
  const char *slash;
  size_t n;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "cancreate: empty path");
    return -1;
  }
  if (access(path, F_OK) == 0) {
    /* path exists — can we overwrite / write into it? */
    r->n = (access(path, W_OK) == 0) ? 1 : 0;
    snprintf(r->str, sizeof r->str, "%ld", r->n);
    r->ok = 1;
    return 0;
  }
  /* missing: parent must allow create (write+search) */
  slash = strrchr(path, '/');
  if (!slash) {
    /* bare basename → cwd */
    snprintf(parent, sizeof parent, "%s", ".");
  } else if (slash == path) {
    /* "/name" → "/" */
    snprintf(parent, sizeof parent, "%s", "/");
  } else {
    n = (size_t)(slash - path);
    if (n >= sizeof parent) n = sizeof parent - 1;
    memcpy(parent, path, n);
    parent[n] = 0;
  }
  r->n = (access(parent, W_OK | X_OK) == 0) ? 1 : 0;
  snprintf(r->str, sizeof r->str, "%ld", r->n);
  r->ok = 1;
  return 0;
}

/* Usability: SYS UMASK — get process file-creation mask without shell umask. */
int cubalc_host_umask_get(cubalc_host_result *r) {
  mode_t cur;
  r_clear(r);
  /* umask returns previous; set 0 then restore to sample without lasting change */
  cur = umask((mode_t)0);
  (void)umask(cur);
  r->n = (long)(cur & (mode_t)0777);
  snprintf(r->str, sizeof r->str, "%04lo", (unsigned long)r->n);
  r->ok = 1;
  return 0;
}

/* Usability: SYS UMASK mode — set creation mask; report previous like shell umask. */
int cubalc_host_umask_set(long mode, cubalc_host_result *r) {
  mode_t prev;
  mode_t m;
  r_clear(r);
  if (mode < 0) {
    snprintf(r->err, sizeof r->err, "umask: bad mode");
    return -1;
  }
  m = (mode_t)(mode & 0777);
  prev = umask(m);
  r->n = (long)(prev & (mode_t)0777);
  snprintf(r->str, sizeof r->str, "%04lo", (unsigned long)r->n);
  r->ok = 1;
  return 0;
}

/* Process cwd stack for SYS PUSHD/POPD — agent temp chdir without shell. */
#define CUBALC_DIRSTACK_MAX 32
#define CUBALC_DIRSTACK_PATH 512
static char cubalc_dirstack[CUBALC_DIRSTACK_MAX][CUBALC_DIRSTACK_PATH];
static int cubalc_dirstack_n = 0;

/* Usability: SYS PUSHD path — save cwd then chdir. */
int cubalc_host_pushd(const char *path, cubalc_host_result *r) {
  char cur[CUBALC_DIRSTACK_PATH];
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "pushd: empty path");
    return -1;
  }
  if (cubalc_dirstack_n >= CUBALC_DIRSTACK_MAX) {
    snprintf(r->err, sizeof r->err, "pushd: stack full");
    return -1;
  }
  if (!getcwd(cur, sizeof cur)) {
    snprintf(r->err, sizeof r->err, "pushd: getcwd fail");
    return -1;
  }
#if defined(CUBALC_OS_WINDOWS)
  if (_chdir(path) != 0) {
#else
  if (chdir(path) != 0) {
#endif
    snprintf(r->err, sizeof r->err, "pushd: chdir fail");
    return -1;
  }
  snprintf(cubalc_dirstack[cubalc_dirstack_n], CUBALC_DIRSTACK_PATH, "%s", cur);
  cubalc_dirstack_n++;
  if (!getcwd(r->str, sizeof r->str))
    snprintf(r->str, sizeof r->str, "%s", path);
  r->n = (long)cubalc_dirstack_n;
  r->ok = 1;
  return 0;
}

/* Usability: SYS POPD — restore previous cwd from stack. */
int cubalc_host_popd(cubalc_host_result *r) {
  r_clear(r);
  if (cubalc_dirstack_n <= 0) {
    snprintf(r->err, sizeof r->err, "popd: empty stack");
    return -1;
  }
  cubalc_dirstack_n--;
#if defined(CUBALC_OS_WINDOWS)
  if (_chdir(cubalc_dirstack[cubalc_dirstack_n]) != 0) {
#else
  if (chdir(cubalc_dirstack[cubalc_dirstack_n]) != 0) {
#endif
    /* leave depth consumed; report fail so agent knows cwd may be wrong */
    snprintf(r->err, sizeof r->err, "popd: chdir fail");
    return -1;
  }
  if (!getcwd(r->str, sizeof r->str))
    snprintf(r->str, sizeof r->str, "%s", cubalc_dirstack[cubalc_dirstack_n]);
  r->n = (long)cubalc_dirstack_n;
  r->ok = 1;
  return 0;
}

/* Usability: SYS DIRSTACK — bag of saved directories (bottom→top). */
int cubalc_host_dirstack(cubalc_host_result *r) {
  size_t o = 0;
  int i;
  r_clear(r);
  r->str[0] = 0;
  for (i = 0; i < cubalc_dirstack_n; i++) {
    size_t L = strlen(cubalc_dirstack[i]);
    if (o + L + 2 >= sizeof r->str) break;
    if (o > 0) r->str[o++] = '\n';
    memcpy(r->str + o, cubalc_dirstack[i], L);
    o += L;
    r->str[o] = 0;
  }
  r->n = (long)cubalc_dirstack_n;
  r->ok = 1;
  return 0;
}

/* Usability: SYS RELPATH base path — portable relative plate paths without shell. */
int cubalc_host_relpath(const char *base, const char *path, cubalc_host_result *r) {
  cubalc_host_result ba, pa;
  size_t bl, pl;
  r_clear(r);
  if (!base || !base[0] || !path || !path[0]) {
    snprintf(r->err, sizeof r->err, "relpath: empty base or path");
    return -1;
  }
  memset(&ba, 0, sizeof ba);
  memset(&pa, 0, sizeof pa);
  if (cubalc_host_abspath(base, &ba) != 0 || !ba.str[0]) {
    snprintf(r->err, sizeof r->err, "relpath: bad base");
    return -1;
  }
  if (cubalc_host_abspath(path, &pa) != 0 || !pa.str[0]) {
    snprintf(r->err, sizeof r->err, "relpath: bad path");
    return -1;
  }
  bl = strlen(ba.str);
  pl = strlen(pa.str);
  while (bl > 1 && ba.str[bl - 1] == '/') {
    ba.str[bl - 1] = 0;
    bl--;
  }
  while (pl > 1 && pa.str[pl - 1] == '/') {
    pa.str[pl - 1] = 0;
    pl--;
  }
  if (strcmp(ba.str, pa.str) == 0) {
    snprintf(r->str, sizeof r->str, "%s", ".");
    r->n = 1;
    r->code = 1; /* under / same */
    r->ok = 1;
    return 0;
  }
  if (pl > bl && strncmp(pa.str, ba.str, bl) == 0 && pa.str[bl] == '/') {
    snprintf(r->str, sizeof r->str, "%s", pa.str + bl + 1);
    r->n = (long)strlen(r->str);
    r->code = 1;
    r->ok = 1;
    return 0;
  }
  /* outside base — keep absolute */
  snprintf(r->str, sizeof r->str, "%s", pa.str);
  r->n = (long)strlen(r->str);
  r->code = 0;
  r->ok = 1;
  return 0;
}

/* Usability: SYS KINDSTR path — human path kind without multi ISDIR/ISFILE/ISLINK glue. */
int cubalc_host_kindstr(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "kindstr: empty path");
    return -1;
  }
  if (lstat(path, &st) != 0) {
    snprintf(r->str, sizeof r->str, "%s", "missing");
    r->n = 0;
    r->code = 0;
    r->ok = 1; /* soft probe — not a hard fail */
    return 0;
  }
  if (S_ISLNK(st.st_mode)) {
    snprintf(r->str, sizeof r->str, "%s", "link");
    r->n = 3;
  } else if (S_ISREG(st.st_mode)) {
    snprintf(r->str, sizeof r->str, "%s", "file");
    r->n = 1;
  } else if (S_ISDIR(st.st_mode)) {
    snprintf(r->str, sizeof r->str, "%s", "dir");
    r->n = 2;
  } else if (S_ISFIFO(st.st_mode)) {
    snprintf(r->str, sizeof r->str, "%s", "fifo");
    r->n = 4;
  } else if (S_ISSOCK(st.st_mode)) {
    snprintf(r->str, sizeof r->str, "%s", "sock");
    r->n = 5;
  } else if (S_ISCHR(st.st_mode)) {
    snprintf(r->str, sizeof r->str, "%s", "chr");
    r->n = 6;
  } else if (S_ISBLK(st.st_mode)) {
    snprintf(r->str, sizeof r->str, "%s", "blk");
    r->n = 7;
  } else {
    snprintf(r->str, sizeof r->str, "%s", "other");
    r->n = 8;
  }
  r->code = (int)r->n;
  r->ok = 1;
  return 0;
}

/* Usability: SYS GROUPS — process group names (egid + supplementary) without shell id -Gn. */
int cubalc_host_groups(cubalc_host_result *r) {
  gid_t list[64];
  int n = 0, i, j, count = 0;
  size_t o = 0;
  gid_t eg;
  r_clear(r);
  r->str[0] = 0;
#if defined(CUBALC_OS_WINDOWS)
  snprintf(r->str, sizeof r->str, "%s", "group");
  r->n = 1;
  r->ok = 1;
  return 0;
#else
  eg = getegid();
  n = getgroups((int)(sizeof list / sizeof list[0]), list);
  if (n < 0) n = 0;
  /* ensure egid present first */
  {
    int has_eg = 0;
    for (i = 0; i < n; i++)
      if (list[i] == eg) { has_eg = 1; break; }
    if (!has_eg && n < (int)(sizeof list / sizeof list[0])) {
      /* shift and insert eg at front for stable primary-first order */
      for (i = n; i > 0; i--) list[i] = list[i - 1];
      list[0] = eg;
      n++;
    }
  }
  for (i = 0; i < n; i++) {
    /* dedupe */
    int dup = 0;
    char name[128];
    size_t L;
    for (j = 0; j < i; j++)
      if (list[j] == list[i]) { dup = 1; break; }
    if (dup) continue;
    name[0] = 0;
    {
      struct group *gr = getgrgid(list[i]);
      if (gr && gr->gr_name && gr->gr_name[0])
        snprintf(name, sizeof name, "%s", gr->gr_name);
      else
        snprintf(name, sizeof name, "%ld", (long)list[i]);
    }
    L = strlen(name);
    if (o + L + 2 >= sizeof r->str) break;
    if (o > 0) r->str[o++] = '\n';
    memcpy(r->str + o, name, L);
    o += L;
    r->str[o] = 0;
    count++;
  }
  r->n = (long)count;
  r->ok = 1;
  return 0;
#endif
}

/* Usability: SYS INGROUP name|gid — process membership probe without shell id -nG. */
int cubalc_host_ingroup(const char *name_or_gid, cubalc_host_result *r) {
  gid_t target = (gid_t)-1;
  gid_t list[64];
  int n, i;
  char *end = NULL;
  unsigned long v;
  r_clear(r);
  if (!name_or_gid || !name_or_gid[0]) {
    snprintf(r->err, sizeof r->err, "ingroup: empty name");
    return -1;
  }
#if defined(CUBALC_OS_WINDOWS)
  r->n = 0;
  snprintf(r->str, sizeof r->str, "0");
  r->ok = 1;
  return 0;
#else
  /* decimal gid? */
  v = strtoul(name_or_gid, &end, 10);
  if (end && end != name_or_gid && *end == 0)
    target = (gid_t)v;
  else {
    struct group *gr = getgrnam(name_or_gid);
    if (!gr) {
      r->n = 0;
      snprintf(r->str, sizeof r->str, "0");
      r->ok = 1;
      return 0;
    }
    target = gr->gr_gid;
  }
  if (getegid() == target || getgid() == target) {
    r->n = 1;
    snprintf(r->str, sizeof r->str, "1");
    r->ok = 1;
    return 0;
  }
  n = getgroups((int)(sizeof list / sizeof list[0]), list);
  if (n < 0) n = 0;
  for (i = 0; i < n; i++) {
    if (list[i] == target) {
      r->n = 1;
      snprintf(r->str, sizeof r->str, "1");
      r->ok = 1;
      return 0;
    }
  }
  r->n = 0;
  snprintf(r->str, sizeof r->str, "0");
  r->ok = 1;
  return 0;
#endif
}

/* Usability: SYS OWNERNAME path — login name for st_uid without shell stat -c %U. */
int cubalc_host_ownername(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "ownername: empty path");
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "ownername: missing");
    return -1;
  }
  r->n = (long)st.st_uid;
#if !defined(CUBALC_OS_WINDOWS)
  {
    struct passwd *pw = getpwuid(st.st_uid);
    if (pw && pw->pw_name && pw->pw_name[0]) {
      snprintf(r->str, sizeof r->str, "%s", pw->pw_name);
      r->ok = 1;
      return 0;
    }
  }
#endif
  /* numeric fallback when passwd has no entry (containers, deleted users) */
  snprintf(r->str, sizeof r->str, "%ld", r->n);
  r->ok = 1;
  return 0;
}

/* Usability: SYS GROUPNAME path — group name for st_gid without shell stat -c %G. */
int cubalc_host_groupname(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "groupname: empty path");
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "groupname: missing");
    return -1;
  }
  r->n = (long)st.st_gid;
#if !defined(CUBALC_OS_WINDOWS)
  {
    struct group *gr = getgrgid(st.st_gid);
    if (gr && gr->gr_name && gr->gr_name[0]) {
      snprintf(r->str, sizeof r->str, "%s", gr->gr_name);
      r->ok = 1;
      return 0;
    }
  }
#endif
  snprintf(r->str, sizeof r->str, "%ld", r->n);
  r->ok = 1;
  return 0;
}

/* Usability: SYS READLINK path — peel symlink target without shell. */
int cubalc_host_readlink(const char *path, cubalc_host_result *r) {
  char buf[CUBALC_HOST_STR_MAX];
  ssize_t n;
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "readlink: empty path");
    return -1;
  }
  if (lstat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "readlink: missing");
    return -1;
  }
  if (!S_ISLNK(st.st_mode)) {
    snprintf(r->err, sizeof r->err, "readlink: not a symlink");
    return -1;
  }
  n = readlink(path, buf, sizeof buf - 1);
  if (n < 0) {
    snprintf(r->err, sizeof r->err, "readlink: %s", strerror(errno));
    return -1;
  }
  buf[n] = 0;
  snprintf(r->str, sizeof r->str, "%s", buf);
  r->n = (long)n;
  r->ok = 1;
  return 0;
}

/* Usability: SYS ISLINK path — probe without READLINK soft-fail glue. */
int cubalc_host_islink(const char *path, cubalc_host_result *r) {
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  if (lstat(path, &st) != 0) {
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  r->n = S_ISLNK(st.st_mode) ? 1 : 0;
  snprintf(r->str, sizeof r->str, "%s", path);
  r->ok = 1;
  return 0;
}

/* Usability: SYS MODE path — octal permission stamp without shell stat -c %a. */
int cubalc_host_mode(const char *path, cubalc_host_result *r) {
  struct stat st;
  unsigned m;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "mode: empty path");
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "mode: missing");
    return -1;
  }
  m = (unsigned)(st.st_mode & 07777);
  r->n = (long)m;
  snprintf(r->str, sizeof r->str, "%04o", m);
  r->ok = 1;
  return 0;
}

/* Usability: SYS CHMOD path mode — set plate perms without shell chmod. */
int cubalc_host_chmod(const char *path, long mode, cubalc_host_result *r) {
  mode_t m;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "chmod: empty path");
    return -1;
  }
  if (mode < 0) mode = 0;
  m = (mode_t)(mode & 07777);
  if (chmod(path, m) != 0) {
    snprintf(r->err, sizeof r->err, "chmod: %s", strerror(errno));
    return -1;
  }
  r->n = (long)m;
  snprintf(r->str, sizeof r->str, "%04o", (unsigned)m);
  r->ok = 1;
  return 0;
}

/* Usability: SYS COPY|CP src dst — duplicate plate without shell. */
int cubalc_host_copy(const char *src, const char *dst, cubalc_host_result *r) {
  FILE *in = NULL, *out = NULL;
  char buf[4096];
  size_t nr, nw;
  long total = 0;
  struct stat st;
  r_clear(r);
  if (!src || !src[0] || !dst || !dst[0]) {
    snprintf(r->err, sizeof r->err, "copy: empty path");
    return -1;
  }
  if (stat(src, &st) != 0) {
    snprintf(r->err, sizeof r->err, "copy: missing source");
    return -1;
  }
  if (S_ISDIR(st.st_mode)) {
    snprintf(r->err, sizeof r->err, "copy: source is a directory");
    return -1;
  }
  /* refuse copy onto self */
  if (strcmp(src, dst) == 0) {
    snprintf(r->str, sizeof r->str, "%s", dst);
    r->n = (long)st.st_size;
    r->ok = 1;
    return 0;
  }
  in = fopen(src, "rb");
  if (!in) {
    snprintf(r->err, sizeof r->err, "copy: open src %s", strerror(errno));
    return -1;
  }
  out = fopen(dst, "wb");
  if (!out) {
    fclose(in);
    snprintf(r->err, sizeof r->err, "copy: open dst %s", strerror(errno));
    return -1;
  }
  while ((nr = fread(buf, 1, sizeof buf, in)) > 0) {
    nw = fwrite(buf, 1, nr, out);
    if (nw != nr) {
      fclose(in);
      fclose(out);
      snprintf(r->err, sizeof r->err, "copy: write short");
      return -1;
    }
    total += (long)nw;
  }
  if (ferror(in)) {
    fclose(in);
    fclose(out);
    snprintf(r->err, sizeof r->err, "copy: read error");
    return -1;
  }
  fclose(in);
  if (fclose(out) != 0) {
    snprintf(r->err, sizeof r->err, "copy: close dst %s", strerror(errno));
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", dst);
  r->n = total;
  r->ok = 1;
  return 0;
}

/* Usability: SYS REALPATH|ABSPATH path — absolute path without shell.
 * Existing paths: realpath(3) (resolves . .. symlinks).
 * Missing/relative: cwd + "/" + path (no fatal). Empty path → cwd. */
int cubalc_host_abspath(const char *path, cubalc_host_result *r) {
  char cwd[512], joined[1024], resolved[4096];
  const char *p = path ? path : "";
  size_t n;
  r_clear(r);
  /* empty → cwd */
  if (!p[0] || strcmp(p, ".") == 0) {
    if (!getcwd(r->str, sizeof r->str)) {
      snprintf(r->err, sizeof r->err, "abspath: getcwd fail");
      return -1;
    }
    r->n = (long)strlen(r->str);
    r->ok = 1;
    return 0;
  }
  /* try realpath when node exists (or intermediate for symlink) */
  if (realpath(p, resolved) != NULL) {
    snprintf(r->str, sizeof r->str, "%s", resolved);
    r->n = (long)strlen(r->str);
    r->ok = 1;
    return 0;
  }
  /* absolute path that does not exist yet — return as-is (strip trailing /) */
  if (p[0] == '/') {
    snprintf(r->str, sizeof r->str, "%s", p);
    n = strlen(r->str);
    while (n > 1 && r->str[n - 1] == '/') {
      r->str[n - 1] = 0;
      n--;
    }
    r->n = (long)n;
    r->ok = 1;
    return 0;
  }
  /* relative missing — join with cwd */
  if (!getcwd(cwd, sizeof cwd)) {
    snprintf(r->err, sizeof r->err, "abspath: getcwd fail");
    return -1;
  }
  n = strlen(cwd);
  while (n > 1 && cwd[n - 1] == '/') {
    cwd[n - 1] = 0;
    n--;
  }
  /* strip leading ./ from relative */
  if (p[0] == '.' && p[1] == '/')
    p += 2;
  snprintf(joined, sizeof joined, "%s/%s", cwd, p);
  /* light normalize: remove trailing slash */
  n = strlen(joined);
  while (n > 1 && joined[n - 1] == '/') {
    joined[n - 1] = 0;
    n--;
  }
  snprintf(r->str, sizeof r->str, "%s", joined);
  r->n = (long)strlen(r->str);
  r->ok = 1;
  return 0;
}

/* Usability: SYS TOUCH path — create empty plate marker or refresh mtime. */
int cubalc_host_touch(const char *path, cubalc_host_result *r) {
  struct stat st;
  int created = 0;
  int fd;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "touch: empty path");
    return -1;
  }
  if (stat(path, &st) != 0) {
    fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd < 0) {
      /* race: created by another process */
      fd = open(path, O_WRONLY | O_CREAT, 0644);
      if (fd < 0) {
        snprintf(r->err, sizeof r->err, "touch: %s", strerror(errno));
        return -1;
      }
      created = 0;
    } else {
      created = 1;
    }
    close(fd);
  } else {
    if (S_ISDIR(st.st_mode)) {
      snprintf(r->err, sizeof r->err, "touch: is a directory");
      return -1;
    }
    /* update mtime to now */
    if (utimensat(AT_FDCWD, path, NULL, 0) != 0) {
      /* fallback: open/close */
      fd = open(path, O_WRONLY);
      if (fd < 0) {
        snprintf(r->err, sizeof r->err, "touch: %s", strerror(errno));
        return -1;
      }
      close(fd);
    }
    created = 0;
  }
  snprintf(r->str, sizeof r->str, "%s", path);
  r->n = created ? 1 : 0;
  r->ok = 1;
  return 0;
}

/* Usability: SYS LIST|LS path — directory names for agent plate scans.
 * r->str = newline-joined basenames (skip . and ..); r->n = entry count.
 * Truncates safely if buffer fills (still returns partial + count so far). */
int cubalc_host_listdir(const char *path, cubalc_host_result *r) {
  DIR *d;
  struct dirent *ent;
  struct stat st;
  size_t used = 0;
  long count = 0;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "list: empty path");
    return -1;
  }
  if (stat(path, &st) != 0) {
    snprintf(r->err, sizeof r->err, "list: missing");
    return -1;
  }
  if (!S_ISDIR(st.st_mode)) {
    snprintf(r->err, sizeof r->err, "list: not a directory");
    return -1;
  }
  d = opendir(path);
  if (!d) {
    snprintf(r->err, sizeof r->err, "list: %s", strerror(errno));
    return -1;
  }
  r->str[0] = 0;
  while ((ent = readdir(d)) != NULL) {
    size_t nlen;
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
      continue;
    nlen = strlen(ent->d_name);
    if (nlen == 0) continue;
    /* need name + optional newline + NUL */
    if (used + nlen + (used ? 1 : 0) + 1 >= sizeof r->str)
      break; /* buffer full — stop, keep partial */
    if (used > 0)
      r->str[used++] = '\n';
    memcpy(r->str + used, ent->d_name, nlen);
    used += nlen;
    r->str[used] = 0;
    count++;
  }
  closedir(d);
  r->n = count;
  r->ok = 1;
  return 0;
}

/* Usability: SYS MKDIR path — mkdir -p for agent plate dirs under STATE/TMP. */
int cubalc_host_mkdir(const char *path, cubalc_host_result *r) {
  char buf[512];
  size_t i, n;
  struct stat st;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "mkdir: empty path");
    return -1;
  }
  if (stat(path, &st) == 0) {
    if (S_ISDIR(st.st_mode)) {
      snprintf(r->str, sizeof r->str, "%s", path);
      r->n = 0; /* already existed */
      r->ok = 1;
      return 0;
    }
    snprintf(r->err, sizeof r->err, "mkdir: not a directory");
    return -1;
  }
  n = strlen(path);
  if (n >= sizeof buf) {
    snprintf(r->err, sizeof r->err, "mkdir: path too long");
    return -1;
  }
  memcpy(buf, path, n + 1);
  /* walk components; skip drive letters on windows-ish "C:..." */
  for (i = 1; i < n; i++) {
    if (buf[i] == '/' || buf[i] == '\\') {
      char save = buf[i];
      buf[i] = 0;
      if (buf[0] && !(buf[0] == '/' && buf[1] == 0)) {
        if (mkdir(buf, 0755) != 0 && errno != EEXIST) {
          snprintf(r->err, sizeof r->err, "mkdir: %s (%s)", buf, strerror(errno));
          return -1;
        }
      }
      buf[i] = save;
    }
  }
  if (mkdir(buf, 0755) != 0 && errno != EEXIST) {
    snprintf(r->err, sizeof r->err, "mkdir: %s", strerror(errno));
    return -1;
  }
  if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
    snprintf(r->err, sizeof r->err, "mkdir: failed to create dir");
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", path);
  r->n = 1; /* newly ensured */
  r->ok = 1;
  return 0;
}

int cubalc_host_env(const char *name, cubalc_host_result *r) {
  r_clear(r);
  if (!name) { snprintf(r->err, sizeof r->err, "env: empty"); return -1; }
  const char *v = getenv(name);
  if (!v) { r->ok = 1; r->str[0] = 0; r->n = 0; return 0; }
  snprintf(r->str, sizeof r->str, "%s", v);
  r->n = (long)strlen(r->str);
  r->ok = 1;
  return 0;
}

/* Usability: in-language CUBALC_* / agent config without shell export.
 * Process-local only (setenv overwrite). Empty val → set empty string. */
int cubalc_host_env_set(const char *name, const char *val, cubalc_host_result *r) {
  r_clear(r);
  if (!name || !name[0]) {
    snprintf(r->err, sizeof r->err, "env set: empty name");
    return -1;
  }
  if (!val) val = "";
  if (setenv(name, val, 1) != 0) {
    snprintf(r->err, sizeof r->err, "env set: %s", strerror(errno));
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", val);
  r->n = (long)strlen(r->str);
  r->ok = 1;
  return 0;
}

/* Soft: already-absent is OK with n=0; removed → n=1. */
int cubalc_host_env_unset(const char *name, cubalc_host_result *r) {
  int was;
  r_clear(r);
  if (!name || !name[0]) {
    snprintf(r->err, sizeof r->err, "env unset: empty name");
    return -1;
  }
  was = (getenv(name) != NULL) ? 1 : 0;
  if (unsetenv(name) != 0) {
    snprintf(r->err, sizeof r->err, "env unset: %s", strerror(errno));
    return -1;
  }
  r->str[0] = 0;
  r->n = was;
  r->ok = 1;
  return 0;
}

/* Usability: INCLUDE-style resolve — short lib name → programs/lib/<stem>.cubalc
 * Also programs/, programs/proof|p2p|protect, CUBALC_ROOT, readable path as-is. */
int cubalc_host_find_cubalc(const char *name, cubalc_host_result *r) {
  char stem[256], try[768];
  const char *root, *slash, *leaf;
  size_t blen;
  int i;
  static const char *dirs[] = {
    "programs/lib", "programs", "programs/proof", "programs/p2p",
    "programs/protect", "docs", NULL
  };
  r_clear(r);
  if (!name || !name[0]) {
    snprintf(r->err, sizeof r->err, "find: empty name");
    return -1;
  }
  /* path as given (readable) */
  if (access(name, R_OK) == 0) {
    snprintf(r->str, sizeof r->str, "%s", name);
    r->ok = 1; r->n = 1; return 0;
  }
  slash = strrchr(name, '/');
  leaf = slash ? slash + 1 : name;
  snprintf(stem, sizeof stem, "%s", leaf);
  blen = strlen(stem);
  if (blen > 7 && strcmp(stem + blen - 7, ".cubalc") == 0)
    stem[blen - 7] = 0;
  for (i = 0; dirs[i]; i++) {
    snprintf(try, sizeof try, "%s/%s.cubalc", dirs[i], stem);
    if (access(try, R_OK) == 0) {
      snprintf(r->str, sizeof r->str, "%s", try);
      r->ok = 1; r->n = 1; return 0;
    }
    snprintf(try, sizeof try, "%s/%s", dirs[i], stem);
    if (access(try, R_OK) == 0) {
      snprintf(r->str, sizeof r->str, "%s", try);
      r->ok = 1; r->n = 1; return 0;
    }
    /* docs: COOKBOOK.md style */
    snprintf(try, sizeof try, "%s/%s.md", dirs[i], stem);
    if (access(try, R_OK) == 0) {
      snprintf(r->str, sizeof r->str, "%s", try);
      r->ok = 1; r->n = 1; return 0;
    }
  }
  root = getenv("CUBALC_ROOT");
  if (root && root[0]) {
    for (i = 0; dirs[i]; i++) {
      snprintf(try, sizeof try, "%s/%s/%s.cubalc", root, dirs[i], stem);
      if (access(try, R_OK) == 0) {
        snprintf(r->str, sizeof r->str, "%s", try);
        r->ok = 1; r->n = 1; return 0;
      }
      snprintf(try, sizeof try, "%s/%s/%s", root, dirs[i], stem);
      if (access(try, R_OK) == 0) {
        snprintf(r->str, sizeof r->str, "%s", try);
        r->ok = 1; r->n = 1; return 0;
      }
    }
  }
  snprintf(r->err, sizeof r->err, "find: not found %s", name);
  return -1;
}

/* PATH/abs/out/local-bin only — shared by which_bin and which. */
static int cubalc_host_which_bin_inner(const char *name, cubalc_host_result *r) {
  char try[512];
  const char *home = getenv("HOME");
  const char *cbin = getenv("CUBALC_BIN");
  if (!name || !name[0]) return -1;
  if (cbin && strstr(name, "cubalc") && access(cbin, X_OK) == 0) {
    snprintf(r->str, sizeof r->str, "%s", cbin); r->ok=1; r->n=1; return 0;
  }
  if (name[0]=='/' && access(name, X_OK)==0) {
    snprintf(r->str, sizeof r->str, "%s", name); r->ok=1; r->n=1; return 0;
  }
  snprintf(try, sizeof try, "./out/%s", name);
  if (access(try, X_OK) == 0) { snprintf(r->str, sizeof r->str, "%s", try); r->ok=1; r->n=1; return 0; }
  if (home) {
    snprintf(try, sizeof try, "%s/.local/bin/%s", home, name);
    if (access(try, X_OK) == 0) { snprintf(r->str, sizeof r->str, "%s", try); r->ok=1; r->n=1; return 0; }
  }
  /* PATH search */
  {
    const char *path = getenv("PATH");
    if (path) {
      char pbuf[4096]; snprintf(pbuf, sizeof pbuf, "%s", path);
      for (char *tok = strtok(pbuf, ":"); tok; tok = strtok(NULL, ":")) {
        snprintf(try, sizeof try, "%s/%s", tok, name);
        if (access(try, X_OK) == 0) {
          snprintf(r->str, sizeof r->str, "%s", try); r->ok=1; r->n=1; return 0;
        }
      }
    }
  }
  return -1;
}

int cubalc_host_which_bin(const char *name, cubalc_host_result *r) {
  r_clear(r);
  if (!name) return -1;
  if (cubalc_host_which_bin_inner(name, r) == 0)
    return 0;
  snprintf(r->err, sizeof r->err, "which_bin: not found %s", name);
  r->ok = 0;
  r->str[0] = 0;
  return -1;
}

int cubalc_host_which(const char *name, cubalc_host_result *r) {
  r_clear(r);
  if (!name) return -1;
  if (cubalc_host_which_bin_inner(name, r) == 0)
    return 0;
  /* Usability: fall back to CubalC lib/program resolve (SYS WHICH hold_seed). */
  if (cubalc_host_find_cubalc(name, r) == 0)
    return 0;
  snprintf(r->err, sizeof r->err, "which: not found %s", name);
  r->ok = 0;
  r->str[0] = 0;
  return -1;
}

/* minimal HTTP/1.0 via TCP using /dev/tcp equivalent: use curl if allowlisted path */
int cubalc_host_http(const char *method, const char *url, const char *body,
                     cubalc_host_result *r) {
  r_clear(r);
  if (!url_allowed(url)) {
    snprintf(r->err, sizeof r->err, "http: url not allowlisted");
    return -1;
  }
  const char *m = method && method[0] ? method : "GET";
  char tmpout[] = "/tmp/cubalc_http_XXXXXX";
  int fd = mkstemp(tmpout);
  if (fd < 0) { snprintf(r->err, sizeof r->err, "http: tmp"); return -1; }
  close(fd);

  pid_t pid = fork();
  if (pid < 0) { unlink(tmpout); return -1; }
  if (pid == 0) {
    int out = open(tmpout, O_WRONLY | O_TRUNC);
    if (out >= 0) { dup2(out, 1); dup2(out, 2); close(out); }
    {
      /* POST chat can be long; override with CUBALC_HTTP_TIMEOUT seconds */
      const char *tmax =
        (body && body[0] && (strcmp(m, "POST") == 0 || strcmp(m, "PUT") == 0))
          ? "120" : "15";
      const char *envt = getenv("CUBALC_HTTP_TIMEOUT");
      if (envt && envt[0]) tmax = envt;
      if (body && body[0] && (strcmp(m, "POST") == 0 || strcmp(m, "PUT") == 0)) {
        execlp("curl", "curl", "-sS", "-m", tmax, "-X", m,
               "-H", "Content-Type: application/json",
               "-d", body, "-w", "\n__HTTP_CODE__%{http_code}",
               url, (char *)NULL);
      } else {
        execlp("curl", "curl", "-sS", "-m", tmax, "-X", m,
               "-w", "\n__HTTP_CODE__%{http_code}",
               url, (char *)NULL);
      }
    }
    _exit(127);
  }
  int st = 0;
  waitpid(pid, &st, 0);
  FILE *f = fopen(tmpout, "rb");
  if (!f) { unlink(tmpout); snprintf(r->err, sizeof r->err, "http: no out"); return -1; }
  size_t n = fread(r->str, 1, sizeof r->str - 1, f);
  r->str[n] = 0;
  fclose(f);
  unlink(tmpout);
  /* parse code trailer */
  char *p = strstr(r->str, "__HTTP_CODE__");
  if (p) {
    r->code = atoi(p + 13);
    *p = 0;
    r->n = (long)strlen(r->str);
  } else {
    r->code = WIFEXITED(st) && WEXITSTATUS(st) == 0 ? 200 : 0;
    r->n = (long)n;
  }
  r->ok = (r->code >= 200 && r->code < 400) ? 1 : 0;
  if (!r->ok && !r->err[0])
    snprintf(r->err, sizeof r->err, "http: code %d", r->code);
  return r->ok ? 0 : -1;
}

int cubalc_host_spawn(const char *bin, char *const argv[], cubalc_host_result *r) {
  r_clear(r);
  if (!bin_allowed(bin)) {
    snprintf(r->err, sizeof r->err, "spawn: binary not allowlisted");
    return -1;
  }
  if (access(bin, X_OK) != 0) {
    snprintf(r->err, sizeof r->err, "spawn: not executable %s", bin);
    return -1;
  }
  int pipefd[2];
  if (pipe(pipefd) != 0) return -1;
  pid_t pid = fork();
  if (pid < 0) { close(pipefd[0]); close(pipefd[1]); return -1; }
  if (pid == 0) {
    close(pipefd[0]);
    dup2(pipefd[1], 1);
    dup2(pipefd[1], 2);
    close(pipefd[1]);
    execv(bin, argv);
    _exit(127);
  }
  close(pipefd[1]);
  size_t o = 0;
  char buf[512];
  ssize_t nr;
  while ((nr = read(pipefd[0], buf, sizeof buf)) > 0) {
    if (o + (size_t)nr >= sizeof r->str) nr = (ssize_t)(sizeof r->str - 1 - o);
    if (nr <= 0) break;
    memcpy(r->str + o, buf, (size_t)nr);
    o += (size_t)nr;
  }
  r->str[o] = 0;
  close(pipefd[0]);
  int st = 0;
  waitpid(pid, &st, 0);
  r->code = WIFEXITED(st) ? WEXITSTATUS(st) : 1;
  r->n = (long)o;
  r->ok = (r->code == 0) ? 1 : 0;
  if (!r->ok) snprintf(r->err, sizeof r->err, "spawn exit %d", r->code);
  return r->ok ? 0 : -1;
}

/* ---- Grokium CubalC surface: join / json / chat ---- */

int cubalc_host_join(const char *a, const char *b, cubalc_host_result *r) {
  r_clear(r);
  if (!a) a = "";
  if (!b) b = "";
  size_t la = strlen(a), lb = strlen(b);
  int need_slash = 0;
  if (la > 0 && lb > 0 && a[la - 1] != '/' && b[0] != '/') need_slash = 1;
  if (la + lb + need_slash + 1 >= sizeof r->str) {
    snprintf(r->err, sizeof r->err, "join: too long");
    return -1;
  }
  if (need_slash)
    snprintf(r->str, sizeof r->str, "%s/%s", a, b);
  else
    snprintf(r->str, sizeof r->str, "%s%s", a, b);
  r->n = (long)strlen(r->str);
  r->ok = 1;
  return 0;
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

/* --- dotted path helpers (GETP "freq.error" / SETP "a.b" …) --- */
#define CUBALC_JSON_PATH_MAXSEG 8

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

static int cubalc_json_path_is_deep(const char *path) {
  if (!path) return 0;
  for (; *path; path++)
    if (*path == '.' || *path == '/') return 1;
  return 0;
}

/* Walk intermediates to parent object of leaf; leave parent in cur. n segs out. */
static int cubalc_json_path_walk_parent(const char *json, const char *path,
                                        char segs[][96], int *nsegs,
                                        char *cur, size_t curcap,
                                        cubalc_host_result *err) {
  cubalc_host_result gr;
  int n, i;
  const char *v;
  n = cubalc_json_path_split(path, segs, CUBALC_JSON_PATH_MAXSEG);
  if (n < 1) {
    if (err) snprintf(err->err, sizeof err->err, "jsonpath: bad path");
    return -1;
  }
  *nsegs = n;
  snprintf(cur, curcap, "%s", json && json[0] ? json : "{}");
  for (i = 0; i < n - 1; i++) {
    memset(&gr, 0, sizeof gr);
    if (cubalc_host_json_get_raw(cur, segs[i], &gr) != 0) {
      if (err) snprintf(err->err, sizeof err->err, "jsonpath: miss %s", segs[i]);
      return -1;
    }
    v = gr.str;
    while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
    if (*v != '{') {
      if (err) snprintf(err->err, sizeof err->err, "jsonpath: not object at %s", segs[i]);
      return -1;
    }
    if (v != gr.str) {
      size_t m = strlen(v);
      memmove(gr.str, v, m + 1);
    }
    snprintf(cur, curcap, "%s", gr.str);
  }
  return 0;
}

/* Peel path leaf as decoded scalar (string unquoted / number text). Soft miss -1. */
int cubalc_host_json_path_get(const char *json, const char *path, cubalc_host_result *r) {
  char segs[CUBALC_JSON_PATH_MAXSEG][96];
  char cur[CUBALC_HOST_STR_MAX];
  int n;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "jsonpath: empty path");
    return -1;
  }
  if (!cubalc_json_path_is_deep(path))
    return cubalc_host_json_get(json, path, r);
  if (cubalc_json_path_walk_parent(json, path, segs, &n, cur, sizeof cur, r) != 0)
    return -1;
  return cubalc_host_json_get(cur, segs[n - 1], r);
}

/* Raw leaf peel along path. Soft miss -1. */
int cubalc_host_json_path_get_raw(const char *json, const char *path,
                                  cubalc_host_result *r) {
  char segs[CUBALC_JSON_PATH_MAXSEG][96];
  char cur[CUBALC_HOST_STR_MAX];
  int n;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "jsonpath: empty path");
    return -1;
  }
  if (!cubalc_json_path_is_deep(path))
    return cubalc_host_json_get_raw(json, path, r);
  if (cubalc_json_path_walk_parent(json, path, segs, &n, cur, sizeof cur, r) != 0)
    return -1;
  return cubalc_host_json_get_raw(cur, segs[n - 1], r);
}

/* Soft presence along dotted path. r->n = 0|1. Always r->ok=1 when args valid. */
int cubalc_host_json_path_has(const char *json, const char *path, cubalc_host_result *r) {
  char segs[CUBALC_JSON_PATH_MAXSEG][96];
  char cur[CUBALC_HOST_STR_MAX];
  cubalc_host_result gr;
  int n, i;
  const char *v;
  r_clear(r);
  if (!path || !path[0]) {
    snprintf(r->err, sizeof r->err, "jsonpath: empty path");
    return -1;
  }
  if (!cubalc_json_path_is_deep(path)) {
    if (cubalc_host_json_get_raw(json, path, &gr) == 0) {
      r->n = 1;
      r->ok = 1;
      snprintf(r->str, sizeof r->str, "%s", "1");
      return 0;
    }
    r->n = 0;
    r->ok = 1;
    snprintf(r->str, sizeof r->str, "%s", "0");
    return 0;
  }
  n = cubalc_json_path_split(path, segs, CUBALC_JSON_PATH_MAXSEG);
  if (n < 1) {
    snprintf(r->err, sizeof r->err, "jsonpath: bad path");
    return -1;
  }
  snprintf(cur, sizeof cur, "%s", json && json[0] ? json : "{}");
  for (i = 0; i < n - 1; i++) {
    memset(&gr, 0, sizeof gr);
    if (cubalc_host_json_get_raw(cur, segs[i], &gr) != 0) {
      r->n = 0;
      r->ok = 1;
      snprintf(r->str, sizeof r->str, "%s", "0");
      return 0;
    }
    v = gr.str;
    while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
    if (*v != '{') {
      r->n = 0;
      r->ok = 1;
      snprintf(r->str, sizeof r->str, "%s", "0");
      return 0;
    }
    if (v != gr.str) {
      size_t m = strlen(v);
      memmove(gr.str, v, m + 1);
    }
    snprintf(cur, sizeof cur, "%s", gr.str);
  }
  memset(&gr, 0, sizeof gr);
  if (cubalc_host_json_get_raw(cur, segs[n - 1], &gr) == 0) {
    r->n = 1;
    r->ok = 1;
    snprintf(r->str, sizeof r->str, "%s", "1");
  } else {
    r->n = 0;
    r->ok = 1;
    snprintf(r->str, sizeof r->str, "%s", "0");
  }
  return 0;
}

/* Peel object at path (or whole plate) for KEYSP/LENP/VALSP nest probes.
 * Soft miss/non-object → "{}" n=0. path empty → root object (or "{}"). */
int cubalc_host_json_path_obj(const char *json, const char *path, cubalc_host_result *r) {
  cubalc_host_result gr;
  const char *v, *j;
  r_clear(r);
  j = json ? json : "";
  while (*j == ' ' || *j == '\t' || *j == '\n' || *j == '\r') j++;
  if (!path || !path[0]) {
    if (*j == '{') {
      snprintf(r->str, sizeof r->str, "%s", j);
      /* count is not required for callers that re-run keys/len; n=1 means object hit */
      r->n = 1;
      r->ok = 1;
      return 0;
    }
    snprintf(r->str, sizeof r->str, "%s", "{}");
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  memset(&gr, 0, sizeof gr);
  if (cubalc_host_json_path_get_raw(j && *j ? j : "{}", path, &gr) != 0) {
    snprintf(r->str, sizeof r->str, "%s", "{}");
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  v = gr.str;
  while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
  if (*v != '{') {
    snprintf(r->str, sizeof r->str, "%s", "{}");
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  if (v != gr.str) {
    size_t m = strlen(v);
    memmove(gr.str, v, m + 1);
  }
  snprintf(r->str, sizeof r->str, "%s", gr.str);
  r->n = 1;
  r->ok = 1;
  return 0;
}

/* DFS walk: emit dotted leaf paths under obj with optional prefix. */
static void cubalc_json_leaf_walk(const char *obj, const char *pfx,
                                  char *out, size_t outcap, size_t *olen,
                                  long *kept, int depth) {
  cubalc_host_result keys, raw;
  const char *p, *v;
  char key[256], child[512];

  if (!obj || !out || !olen || !kept || depth > 8) return;
  memset(&keys, 0, sizeof keys);
  if (cubalc_host_json_keys(obj, &keys) != 0) return;
  if (keys.n == 0) {
    /* empty object terminal: emit path when nested under a key */
    if (pfx && pfx[0])
      cubalc_bag_push(out, outcap, olen, kept, pfx);
    return;
  }
  p = keys.str;
  while (*p) {
    size_t kn = 0;
    while (p[kn] && p[kn] != '\n' && p[kn] != '\r') kn++;
    if (kn >= sizeof key) kn = sizeof key - 1;
    if (kn > 0) {
      memcpy(key, p, kn);
      key[kn] = 0;
      memset(&raw, 0, sizeof raw);
      if (cubalc_host_json_get_raw(obj, key, &raw) == 0) {
        if (pfx && pfx[0])
          snprintf(child, sizeof child, "%s.%s", pfx, key);
        else
          snprintf(child, sizeof child, "%s", key);
        v = raw.str;
        while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
        if (*v == '{') {
          cubalc_json_leaf_walk(v, child, out, outcap, olen, kept, depth + 1);
        } else {
          cubalc_bag_push(out, outcap, olen, kept, child);
        }
      }
    }
    p += kn;
    while (*p == '\n' || *p == '\r') p++;
  }
}

/* Recursively collect dotted leaf paths · optional nest path root.
 * Paths relative to nest root. Soft always OK. See header. */
int cubalc_host_json_leaf_paths(const char *json, const char *path,
                                cubalc_host_result *r) {
  cubalc_host_result obj;
  size_t olen = 0;
  long kept = 0;
  r_clear(r);
  r->str[0] = 0;
  memset(&obj, 0, sizeof obj);
  if (cubalc_host_json_path_obj(json ? json : "{}", path, &obj) != 0) {
    r->ok = 1;
    r->n = 0;
    return 0;
  }
  cubalc_json_leaf_walk(obj.str, "", r->str, sizeof r->str, &olen, &kept, 0);
  r->n = kept;
  r->ok = 1;
  return 0;
}

/* Decode a leaf raw JSON value into valbuf (string unquoted; tokens as-is). */
static void cubalc_json_leaf_valtext(const char *obj, const char *key,
                                     const char *rawv, char *valbuf, size_t vcap,
                                     const char **valtext_out) {
  cubalc_host_result gr;
  const char *v = rawv ? rawv : "";
  while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
  valbuf[0] = 0;
  *valtext_out = "";
  if (*v == '"') {
    memset(&gr, 0, sizeof gr);
    if (cubalc_host_json_get(obj, key, &gr) == 0) {
      *valtext_out = gr.str;
      /* copy into valbuf so gr can go out of scope for caller — caller must use
       * immediately; copy for safety */
      snprintf(valbuf, vcap, "%s", gr.str);
      *valtext_out = valbuf;
    } else {
      size_t rl = strlen(v);
      if (rl >= 2 && v[0] == '"' && v[rl - 1] == '"') {
        size_t n = rl - 2;
        if (n >= vcap) n = vcap - 1;
        memcpy(valbuf, v + 1, n);
        valbuf[n] = 0;
        *valtext_out = valbuf;
      } else {
        snprintf(valbuf, vcap, "%s", v);
        *valtext_out = valbuf;
      }
    }
  } else if (strncmp(v, "true", 4) == 0 &&
             (v[4] == 0 || v[4] == ',' || v[4] == '}' || v[4] == ' ' ||
              v[4] == '\n' || v[4] == '\r' || v[4] == '\t')) {
    *valtext_out = "true";
  } else if (strncmp(v, "false", 5) == 0 &&
             (v[5] == 0 || v[5] == ',' || v[5] == '}' || v[5] == ' ' ||
              v[5] == '\n' || v[5] == '\r' || v[5] == '\t')) {
    *valtext_out = "false";
  } else if (strncmp(v, "null", 4) == 0 &&
             (v[4] == 0 || v[4] == ',' || v[4] == '}' || v[4] == ' ' ||
              v[4] == '\n' || v[4] == '\r' || v[4] == '\t')) {
    *valtext_out = "null";
  } else {
    size_t i = 0;
    while (v[i] && v[i] != '\n' && v[i] != '\r' && i + 1 < vcap) {
      valbuf[i] = v[i];
      i++;
    }
    valbuf[i] = 0;
    *valtext_out = valbuf;
  }
}

/* DFS: emit path:val lines under obj. */
static void cubalc_json_leaf_kv_walk(const char *obj, const char *pfx,
                                    char *out, size_t outcap, size_t *olen,
                                    long *kept, int depth) {
  cubalc_host_result keys, raw;
  const char *p, *v, *valtext;
  char key[256], child[512], linebuf[CUBALC_HOST_STR_MAX], valbuf[CUBALC_HOST_STR_MAX];

  if (!obj || !out || !olen || !kept || depth > 8) return;
  memset(&keys, 0, sizeof keys);
  if (cubalc_host_json_keys(obj, &keys) != 0) return;
  if (keys.n == 0) {
    if (pfx && pfx[0]) {
      if (snprintf(linebuf, sizeof linebuf, "%s:", pfx) >= 0)
        cubalc_bag_push(out, outcap, olen, kept, linebuf);
    }
    return;
  }
  p = keys.str;
  while (*p) {
    size_t kn = 0;
    while (p[kn] && p[kn] != '\n' && p[kn] != '\r') kn++;
    if (kn >= sizeof key) kn = sizeof key - 1;
    if (kn > 0) {
      memcpy(key, p, kn);
      key[kn] = 0;
      memset(&raw, 0, sizeof raw);
      if (cubalc_host_json_get_raw(obj, key, &raw) == 0) {
        if (pfx && pfx[0])
          snprintf(child, sizeof child, "%s.%s", pfx, key);
        else
          snprintf(child, sizeof child, "%s", key);
        v = raw.str;
        while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
        if (*v == '{') {
          cubalc_json_leaf_kv_walk(v, child, out, outcap, olen, kept, depth + 1);
        } else {
          cubalc_json_leaf_valtext(obj, key, raw.str, valbuf, sizeof valbuf, &valtext);
          if (snprintf(linebuf, sizeof linebuf, "%s:%s", child,
                       valtext ? valtext : "") >= 0)
            cubalc_bag_push(out, outcap, olen, kept, linebuf);
        }
      }
    }
    p += kn;
    while (*p == '\n' || *p == '\r') p++;
  }
}

/* Recursive path:value bag · optional nest path root. Soft always OK. */
int cubalc_host_json_leaf_kv(const char *json, const char *path,
                             cubalc_host_result *r) {
  cubalc_host_result obj;
  size_t olen = 0;
  long kept = 0;
  r_clear(r);
  r->str[0] = 0;
  memset(&obj, 0, sizeof obj);
  if (cubalc_host_json_path_obj(json ? json : "{}", path, &obj) != 0) {
    r->ok = 1;
    r->n = 0;
    return 0;
  }
  cubalc_json_leaf_kv_walk(obj.str, "", r->str, sizeof r->str, &olen, &kept, 0);
  r->n = kept;
  r->ok = 1;
  return 0;
}

/* Apply path:value bag onto base (optional under prefix). See header. */
int cubalc_host_json_unflat_kv(const char *base, const char *bag, const char *under,
                               cubalc_host_result *r) {
  char cur[CUBALC_HOST_STR_MAX];
  char full[640];
  const char *p, *line, *j;
  long applied = 0;
  cubalc_host_result hr;
  r_clear(r);
  j = base ? base : "";
  while (*j == ' ' || *j == '\t' || *j == '\n' || *j == '\r') j++;
  if (*j == '{')
    snprintf(cur, sizeof cur, "%s", j);
  else
    snprintf(cur, sizeof cur, "%s", "{}");
  if (!bag || !bag[0]) {
    snprintf(r->str, sizeof r->str, "%s", cur);
    r->n = 0;
    r->ok = 1;
    return 0;
  }
  p = bag;
  while (*p) {
    char key[512], val[CUBALC_HOST_STR_MAX];
    size_t kn = 0, vn = 0;
    const char *sep, *eq, *col, *vstart, *vend;
    int val_kind = 0;
    char *end = NULL;
    while (*p == '\n' || *p == '\r') p++;
    if (!*p) break;
    line = p;
    while (*p && *p != '\n' && *p != '\r') p++;
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
      continue;
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
    if (under && under[0])
      snprintf(full, sizeof full, "%s.%s", under, key);
    else
      snprintf(full, sizeof full, "%s", key);
    /* empty value → raw {} (leaf_kv empty-object terminal) */
    if (!val[0]) {
      snprintf(val, sizeof val, "%s", "{}");
      val_kind = 1;
    } else if (strcmp(val, "true") == 0 || strcmp(val, "false") == 0 ||
               strcmp(val, "null") == 0) {
      val_kind = 1;
    } else if (val[0] == '{' || val[0] == '[') {
      val_kind = 1; /* auto-raw JSON-shaped */
    } else {
      (void)strtol(val, &end, 10);
      if (end && end != val && *end == 0)
        val_kind = 1;
      else {
        char *ed = NULL;
        (void)strtod(val, &ed);
        if (ed && ed != val && *ed == 0)
          val_kind = 1;
        else
          val_kind = 0;
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_json_path_set(cur, full, val, val_kind, &hr) != 0) {
      snprintf(r->err, sizeof r->err, "%s",
               hr.err[0] ? hr.err : "unflat: path set fail");
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

/* Rename leaf path prefixes. See header. */
int cubalc_host_json_leaf_rename_pfx(const char *json, const char *old_pfx,
                                     const char *new_pfx, cubalc_host_result *r) {
  cubalc_host_result kv, wr;
  const char *p, *line;
  char bag[CUBALC_HOST_STR_MAX];
  size_t olen = 0;
  long renamed = 0, kept = 0;
  size_t opx;

  r_clear(r);
  if (!old_pfx) old_pfx = "";
  if (!new_pfx) new_pfx = "";
  opx = strlen(old_pfx);
  memset(&kv, 0, sizeof kv);
  cubalc_host_json_leaf_kv(json ? json : "{}", NULL, &kv);
  bag[0] = 0;
  if (!old_pfx[0]) {
    /* no-op: return original plate */
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
  p = kv.str;
  while (*p) {
    char path[512], val[CUBALC_HOST_STR_MAX / 2], npath[640], linebuf[CUBALC_HOST_STR_MAX];
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
    if (vn >= sizeof val) vn = sizeof val - 1;
    memcpy(val, vs, vn);
    val[vn] = 0;
    if (kn >= opx && memcmp(path, old_pfx, opx) == 0) {
      snprintf(npath, sizeof npath, "%s%.*s", new_pfx, (int)(kn - opx), path + opx);
      renamed++;
    } else {
      snprintf(npath, sizeof npath, "%s", path);
    }
    if (snprintf(linebuf, sizeof linebuf, "%s:%s", npath, val) >= 0)
      cubalc_bag_push(bag, sizeof bag, &olen, &kept, linebuf);
  }
  memset(&wr, 0, sizeof wr);
  if (cubalc_host_json_unflat_kv("{}", bag, NULL, &wr) != 0) {
    snprintf(r->err, sizeof r->err, "%s",
             wr.err[0] ? wr.err : "renameflat: unflat fail");
    return -1;
  }
  snprintf(r->str, sizeof r->str, "%s", wr.str);
  r->n = renamed;
  r->ok = 1;
  r->code = kept; /* total leaves after rename */
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

/* Usability: SYS JSONPICK plate keys — keep only listed top-level keys.
 * keys_nl = newline bag (order preserved). Missing keys skipped. Raw values kept.
 * r->n = number of keys kept. Empty bag / no hits → {}. */
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

/* Usability: SYS JSONLEN plate — top-level key count without JSONKEYS bag. */
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

/* Parse pure-int JSON raw value (bool as 0/1). Returns 1 if numeric. */
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

/* Usability: multi-plate TOPNP/BOTNP — top/bottom N pure-int keys by value → plate. */
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

/* Usability: SYS JSONPLUCK / PLUCKP — multi-key peel → value bag without multi GETP+PUSH.
 * Keys may be dotted/slash nest paths (same plane as GETP "freq.error"). */
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

static int load_token(char *out, size_t outn) {
  out[0] = 0;
  const char *e = getenv("XAI_API_KEY");
  if (!e || !e[0]) e = getenv("GROK_API_KEY");
  if (!e || !e[0]) e = getenv("CUBALC_HTTP_AUTH");
  if (e && e[0]) { snprintf(out, outn, "%s", e); return 0; }
  const char *home = getenv("HOME");
  if (!home) return -1;
  char path[512];
  snprintf(path, sizeof path, "%s/.grok/auth.json", home);
  FILE *f = fopen(path, "r");
  if (!f) return -1;
  char *buf = malloc(512 * 1024);
  if (!buf) { fclose(f); return -1; }
  size_t n = fread(buf, 1, 512 * 1024 - 1, f);
  buf[n] = 0;
  fclose(f);
  cubalc_host_result jr;
  if (cubalc_host_json_get(buf, "key", &jr) == 0 && strncmp(jr.str, "eyJ", 3) == 0) {
    snprintf(out, outn, "%s", jr.str);
    free(buf);
    return 0;
  }
  if (cubalc_host_json_get(buf, "access_token", &jr) == 0) {
    snprintf(out, outn, "%s", jr.str);
    free(buf);
    return 0;
  }
  free(buf);
  return -1;
}

static void json_esc(FILE *f, const char *s) {
  if (!s) return;
  for (const char *p = s; *p; p++) {
    if (*p == '"' || *p == '\\') fputc('\\', f);
    else if (*p == '\n') fputs("\\n", f);
    else if ((unsigned char)*p < 32) continue;
    else fputc(*p, f);
  }
}

static int resolve_local_model_id(char *model, size_t n) {
  if (model[0] && strcmp(model, "local") != 0)
    return 0;
  FILE *mf = popen("curl -sS -m 3 http://127.0.0.1:1212/v1/models 2>/dev/null", "r");
  if (!mf) { if (!model[0]) snprintf(model, n, "local"); return 0; }
  char buf[8192];
  size_t nr = fread(buf, 1, sizeof buf - 1, mf);
  buf[nr] = 0;
  pclose(mf);
  cubalc_host_result jr;
  if (cubalc_host_json_get(buf, "id", &jr) == 0 && jr.str[0]) {
    snprintf(model, n, "%s", jr.str);
    return 0;
  }
  if (!model[0]) snprintf(model, n, "local");
  return 0;
}

int cubalc_host_chat(const char *backend, const char *model_in, const char *msg,
                     const char *state_dir, cubalc_host_result *r) {
  r_clear(r);
  if (!msg || !msg[0]) {
    snprintf(r->err, sizeof r->err, "chat: empty msg");
    return -1;
  }
  int is_grok = backend && (strcmp(backend, "grok") == 0 || strcmp(backend, "xai") == 0);
  char token[8192];
  token[0] = 0;
  if (is_grok && load_token(token, sizeof token) != 0) {
    snprintf(r->err, sizeof r->err, "chat: no token");
    return -1;
  }
  char model[512];
  snprintf(model, sizeof model, "%s",
           model_in && model_in[0] ? model_in : (is_grok ? "grok-4" : "local"));
  if (!is_grok) {
    if (strncmp(model, "grok", 4) == 0) snprintf(model, sizeof model, "local");
    resolve_local_model_id(model, sizeof model);
  } else if (strcmp(model, "local") == 0) {
    snprintf(model, sizeof model, "grok-4");
  }

  const char *url = is_grok
    ? "https://api.x.ai/v1/chat/completions"
    : "http://127.0.0.1:1212/v1/chat/completions";
  if (is_grok) {
    const char *u = getenv("GROKIUM_GROK_URL");
    if (u && u[0]) url = u;
  } else {
    const char *u = getenv("GROKIUM_LOCAL_URL");
    if (u && u[0]) url = u;
  }

  char bodypath[512];
  const char *sd = state_dir && state_dir[0] ? state_dir : "/tmp";
  snprintf(bodypath, sizeof bodypath, "%s/_cblc_chat_body.json", sd);
  mkdir(sd, 0755);
  FILE *bf = fopen(bodypath, "w");
  if (!bf) {
    snprintf(r->err, sizeof r->err, "chat: body write");
    return -1;
  }
  fputs("{\"model\":\"", bf);
  json_esc(bf, model);
  fputs("\",\"messages\":[{\"role\":\"system\",\"content\":\"", bf);
  if (is_grok)
    json_esc(bf, "Concise helpful assistant. Use tools via host when offered.");
  else
    json_esc(bf, "Reply one short plain sentence. No chain-of-thought.");
  fputs("\"},{\"role\":\"user\",\"content\":\"", bf);
  json_esc(bf, msg);
  if (is_grok)
    fputs("\"}],\"max_tokens\":512,\"temperature\":0.7,\"stream\":false}", bf);
  else
    fputs("\"}],\"max_tokens\":256,\"temperature\":0.7,\"stream\":false,"
          "\"chat_template_kwargs\":{\"enable_thinking\":false}}", bf);
  fclose(bf);

  /* read body back for http */
  cubalc_host_result br;
  if (cubalc_host_read(bodypath, &br) != 0) {
    snprintf(r->err, sizeof r->err, "chat: body read");
    return -1;
  }

  /* HTTP with optional bearer — inline curl for auth */
  char tmpout[] = "/tmp/cubalc_chat_XXXXXX";
  int fd = mkstemp(tmpout);
  if (fd < 0) { snprintf(r->err, sizeof r->err, "chat: tmp"); return -1; }
  close(fd);
  pid_t pid = fork();
  if (pid < 0) { unlink(tmpout); return -1; }
  if (pid == 0) {
    int out = open(tmpout, O_WRONLY | O_TRUNC);
    if (out >= 0) { dup2(out, 1); dup2(out, 2); close(out); }
    char dflag[600];
    snprintf(dflag, sizeof dflag, "@%s", bodypath);
    if (is_grok) {
      char auth[8500];
      snprintf(auth, sizeof auth, "Authorization: Bearer %s", token);
      execlp("curl", "curl", "-sS", "-m", "120",
             "-H", "Content-Type: application/json",
             "-H", "User-Agent: grokium/c3 (cubalc)",
             "-H", auth,
             "-d", dflag,
             "-w", "\n__HTTP_CODE__%{http_code}",
             url, (char *)NULL);
    } else {
      execlp("curl", "curl", "-sS", "-m", "90",
             "-H", "Content-Type: application/json",
             "-d", dflag,
             "-w", "\n__HTTP_CODE__%{http_code}",
             url, (char *)NULL);
    }
    _exit(127);
  }
  int st = 0;
  waitpid(pid, &st, 0);
  FILE *f = fopen(tmpout, "rb");
  if (!f) { unlink(tmpout); snprintf(r->err, sizeof r->err, "chat: no out"); return -1; }
  size_t n = fread(r->str, 1, sizeof r->str - 1, f);
  r->str[n] = 0;
  fclose(f);
  unlink(tmpout);
  char *p = strstr(r->str, "__HTTP_CODE__");
  if (p) {
    r->code = atoi(p + 13);
    *p = 0;
  } else {
    r->code = WIFEXITED(st) && WEXITSTATUS(st) == 0 ? 200 : 0;
  }
  /* cache raw */
  {
    char lastp[512];
    snprintf(lastp, sizeof lastp, "%s/_chat_last.json", sd);
    FILE *lf = fopen(lastp, "w");
    if (lf) { fputs(r->str, lf); fclose(lf); }
  }
  cubalc_host_result jr;
  if (cubalc_host_json_get(r->str, "content", &jr) != 0 || !jr.str[0]) {
    if (cubalc_host_json_get(r->str, "reasoning_content", &jr) != 0) {
      snprintf(r->err, sizeof r->err, "chat: no content http=%d", r->code);
      r->ok = 0;
      return -1;
    }
  }
  /* squash ws lightly */
  {
    char buf[CUBALC_HOST_STR_MAX];
    size_t o = 0;
    int sp = 1;
    for (char *q = jr.str; *q && o + 1 < sizeof buf; q++) {
      unsigned char c = (unsigned char)*q;
      if (c == '\n' || c == '\r' || c == '\t' || c == ' ') {
        if (!sp) { buf[o++] = ' '; sp = 1; }
        continue;
      }
      sp = 0;
      buf[o++] = (char)c;
    }
    while (o > 0 && buf[o - 1] == ' ') o--;
    buf[o] = 0;
    snprintf(r->str, sizeof r->str, "%s", buf);
  }
  r->n = (long)strlen(r->str);
  r->ok = 1;
  return 0;
}


/* Expand {{key}} from plate object — CLI + agents without .cubalc FILLP. */
int cubalc_host_expand_fillp(const char *plate, const char *tmpl,
                             char *out, size_t outcap,
                             long *hits_out, long *miss_out,
                             char *miss_bag, size_t miss_cap) {
  const char *src = tmpl ? tmpl : "";
  const char *pl = (plate && plate[0]) ? plate : "{}";
  size_t o = 0, mo = 0;
  long hits = 0, miss = 0;
  char name[96];
  cubalc_host_result gr;
  if (miss_bag && miss_cap) miss_bag[0] = 0;
  if (!out || outcap == 0) {
    if (hits_out) *hits_out = 0;
    if (miss_out) *miss_out = 0;
    return -1;
  }
  out[0] = 0;
  while (*src && o + 1 < outcap) {
    if (src[0] == '{' && src[1] == '{') {
      const char *p = src + 2;
      size_t ni = 0;
      name[0] = 0;
      while (*p && !(p[0] == '}' && p[1] == '}') && ni + 1 < sizeof name) {
        char ch = *p;
        /* dotted/slash nest paths: {{freq.error}} {{meta/role}} */
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
            (ch >= '0' && ch <= '9') || ch == '_' || ch == '-' || ch == '.' ||
            ch == '/')
          name[ni++] = ch;
        else
          break;
        p++;
      }
      name[ni] = 0;
      if (name[0] && p[0] == '}' && p[1] == '}') {
        src = p + 2;
        hits++;
        memset(&gr, 0, sizeof gr);
        if (cubalc_host_json_path_get(pl, name, &gr) == 0) {
          size_t vn = strlen(gr.str);
          if (o + vn >= outcap) vn = outcap - 1 - o;
          if (vn > 0) {
            memcpy(out + o, gr.str, vn);
            o += vn;
          }
          out[o] = 0;
        } else {
          miss++;
          if (miss_bag && miss_cap > 1) {
            size_t nl = strlen(name);
            if (mo > 0 && mo + 1 < miss_cap) miss_bag[mo++] = '\n';
            if (mo + nl < miss_cap) {
              memcpy(miss_bag + mo, name, nl);
              mo += nl;
              miss_bag[mo] = 0;
            }
          }
        }
        continue;
      }
      out[o++] = *src++;
      continue;
    }
    out[o++] = *src++;
  }
  out[o] = 0;
  if (hits_out) *hits_out = hits;
  if (miss_out) *miss_out = miss;
  return 0;
}

int cubalc_host_fillp_keys(const char *tmpl, char *out, size_t outcap, long *nkeys) {
  const char *src = tmpl ? tmpl : "";
  size_t o = 0;
  long n = 0;
  char name[96];
  if (!out || outcap == 0) {
    if (nkeys) *nkeys = 0;
    return -1;
  }
  out[0] = 0;
  while (*src) {
    if (src[0] == '{' && src[1] == '{') {
      const char *p = src + 2;
      size_t ni = 0;
      name[0] = 0;
      while (*p && !(p[0] == '}' && p[1] == '}') && ni + 1 < sizeof name) {
        char ch = *p;
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
            (ch >= '0' && ch <= '9') || ch == '_' || ch == '-' || ch == '.' ||
            ch == '/')
          name[ni++] = ch;
        else
          break;
        p++;
      }
      name[ni] = 0;
      if (name[0] && p[0] == '}' && p[1] == '}') {
        size_t nl;
        int seen = 0;
        const char *q;
        src = p + 2;
        nl = strlen(name);
        q = out;
        while (*q) {
          size_t ll = 0;
          while (q[ll] && q[ll] != '\n') ll++;
          if (ll == nl && memcmp(q, name, nl) == 0) { seen = 1; break; }
          q += ll;
          if (*q == '\n') q++;
        }
        if (!seen) {
          if (o > 0 && o + 1 < outcap) out[o++] = '\n';
          if (o + nl < outcap) {
            memcpy(out + o, name, nl);
            o += nl;
            out[o] = 0;
            n++;
          }
        }
        continue;
      }
      src++;
      continue;
    }
    src++;
  }
  if (nkeys) *nkeys = n;
  return 0;
}

