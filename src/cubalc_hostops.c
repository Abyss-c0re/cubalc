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

/* protect_mode·url_allowed·bin_allowed·http·spawn → cubalc_hostops_httpspawn.c (B_hostops_margin) */

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

/* mtime·atime·ctime → cubalc_hostops_stat_time.c (B_hostops_margin) */

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

/* RMTREE·CPTREE·MVTREE·DIRSIZE → cubalc_hostops_tree_mut.c (B_hostops_margin) */
/* EQTREE·GREPTREE → cubalc_hostops_tree_query.c (B_hostops_margin) */

/* Shared tree helpers (also in tree_query; kept for REPLACETREE/COUNT/line planes). */


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

/* REPLACETREE·COUNTINTREE → cubalc_hostops_tree_edit.c (B_hostops_margin) */
/* GREPLINESTREE·PATHLINETREE → cubalc_hostops_tree_lines.c (B_hostops_margin) */
/* FIRSTLINEINTREE·LASTLINEINTREE → cubalc_hostops_tree_linefind.c (B_hostops_margin) */
/* FINDINTREE·TAKEINTREE·NTHINTREE·SLICEINTREE·DROPINTREE → cubalc_hostops_tree_find.c (B_hostops_margin) */
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

/* symlink·hardlink → cubalc_hostops_link.c (B_hostops_margin) */

/* nlink·sameinode·inode·fileuid·filegid → cubalc_hostops_inode.c (B_hostops_margin) */

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

/* groups → cubalc_hostops_groupscopy.c (B_hostops_margin) */


/* ingroup → cubalc_hostops_groupscopy.c (B_hostops_margin) */


/* ownername·groupname → cubalc_hostops_ownermode.c (B_hostops_margin) */

/* readlink·islink → cubalc_hostops_link.c (B_hostops_margin) */

/* mode·chmod → cubalc_hostops_ownermode.c (B_hostops_margin) */

/* copy → cubalc_hostops_groupscopy.c (B_hostops_margin) */


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

/* env·env_set·env_unset → cubalc_hostops_env.c (B_hostops_margin) */

/* find_cubalc·which_bin·which → cubalc_hostops_which.c (B_hostops_margin) */

/* cubalc_host_http · cubalc_host_spawn → cubalc_hostops_httpspawn.c (B_hostops_margin) */

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

/* json_get·json_keys·json_set·json_del (+ esc/skip/fmt)
 * → cubalc_hostops_json_core.c (B_hostops_margin)
 */

/* path_get·path_get_raw·path_has·path_obj (+ path_split/walk)
 * → cubalc_hostops_json_path.c (B_hostops_margin)
 */

/* leaf_paths·leaf_kv·unflat_kv (+ leaf_walk/valtext helpers)
 * → cubalc_hostops_json_flat.c (B_hostops_margin)
 */

/* leaf_diff·leaf_grep·leaf_filter_write·leaf_merge (+ pathbag/flatbag helpers)
 * → cubalc_hostops_json_leafdiff.c (B_hostops_margin)
 */

/* cubalc_host_json_leaf_rename_pfx → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_set → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_inc → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_sum → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_avg → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_median → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_toppath → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_thresh → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_cap → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_scale → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_count → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_countn → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_match_bag → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_match_bagn → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_get → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_get_last → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_get_nth → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_getn → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_getn_last → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_getn_nth → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_type → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_freq → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_mode → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_path_by_val → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_paths_by_val → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_count_by_val → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_uniq → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_all_eq → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_first_uneq → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_leaf_uneq_paths → json_delta peel (G_hostops_margin) */

/* cubalc_host_json_pretty → cubalc_hostops_json_delta peel (G_hostops_margin) */

/* cubalc_host_json_path_set → cubalc_hostops_json_delta peel (G_hostops_margin) */

/* cubalc_host_json_path_del → cubalc_hostops_json_delta peel (G_hostops_margin) */

/* cubalc_host_json_merge → cubalc_hostops_json_delta peel (G_hostops_margin) */

/* cubalc_host_json_defaults → cubalc_hostops_json_delta peel (G_hostops_margin) */

/* cubalc_host_json_get_raw → cubalc_hostops_json_delta peel (G_hostops_margin) */

/* Usability: SYS JSONPICK plate keys — keep only listed top-level keys.
 * keys_nl = newline bag (order preserved). Missing keys skipped. Raw values kept.
 * r->n = number of keys kept. Empty bag / no hits → {}. */
/* cubalc_host_json_pick → cubalc_hostops_json_delta peel (G_hostops_margin) */

/* cubalc_host_json_rename → cubalc_hostops_json_delta peel (G_hostops_margin) */

/* cubalc_host_json_copy_key → cubalc_hostops_json_delta peel (G_hostops_margin) */

/* cubalc_host_json_swap_keys → cubalc_hostops_json_delta peel (G_hostops_margin) */

/* Usability: SYS JSONDROP plate keys — drop listed top-level keys (bulk JSONDEL).
 * keys_nl = newline bag. Missing soft-skipped. r->n = removed count. */
/* cubalc_host_json_drop → cubalc_hostops_json_delta peel (G_hostops; monolit dup stripped) */
