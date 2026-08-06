/* CubalC host ops — C runtime under CubalC surface (Grokium = CubalC + C).
 * Allowlisted, fail-closed. HOLD_FLASH never flashes devices.
 */
#ifndef CUBALC_HOSTOPS_H
#define CUBALC_HOSTOPS_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define CUBALC_HOST_STR_MAX 8192
#define CUBALC_HOST_ERR_MAX 160

typedef struct cubalc_host_result {
  int ok;
  int code;          /* HTTP status or process exit */
  long n;            /* length / numeric */
  char str[CUBALC_HOST_STR_MAX];
  char err[CUBALC_HOST_ERR_MAX];
} cubalc_host_result;

/* file */
int cubalc_host_read(const char *path, cubalc_host_result *r);
int cubalc_host_write(const char *path, const char *data, cubalc_host_result *r);
int cubalc_host_exists(const char *path); /* 1/0 */
/* path kind: returns 0 missing, 1 regular file, 2 directory, 3 other; sets r->n size for files */
int cubalc_host_path_kind(const char *path, cubalc_host_result *r);
/* mtime: r->n = st_mtime epoch seconds; soft miss ok=0 code=0; exists ok=1 code=1|2|3 */
int cubalc_host_mtime(const char *path, cubalc_host_result *r);
/* atime: r->n = st_atime epoch seconds; soft miss ok=0 */
int cubalc_host_atime(const char *path, cubalc_host_result *r);
/* ctime: r->n = st_ctime epoch seconds (inode status change); soft miss ok=0 */
int cubalc_host_ctime(const char *path, cubalc_host_result *r);
/* mkdir -p style: create path and parents; OK if already a directory */
int cubalc_host_mkdir(const char *path, cubalc_host_result *r);
/* unlink regular file only; missing → ok with n=0; dir → error */
int cubalc_host_rm(const char *path, cubalc_host_result *r);
/* rmdir empty directory only; missing → ok with n=0; file/nonempty → error */
int cubalc_host_rmdir(const char *path, cubalc_host_result *r);
/* rename/move path; soft miss if from missing */
int cubalc_host_rename(const char *from, const char *to, cubalc_host_result *r);
/* copy regular file src → dst; r->n = bytes written */
int cubalc_host_copy(const char *src, const char *dst, cubalc_host_result *r);
/* create symlink linkpath → target; r->str = linkpath; soft fail if linkpath exists */
int cubalc_host_symlink(const char *target, const char *linkpath, cubalc_host_result *r);
/* create hard link newpath → existing (same inode); soft fail if new exists or src missing */
int cubalc_host_hardlink(const char *existing, const char *newpath, cubalc_host_result *r);
/* link count: r->n = st_nlink; soft miss if path missing */
int cubalc_host_nlink(const char *path, cubalc_host_result *r);
/* same inode+device: r->n = 1 if a and b are same file; soft miss either missing */
int cubalc_host_sameinode(const char *a, const char *b, cubalc_host_result *r);
/* inode number: r->n = st_ino; soft miss missing path */
int cubalc_host_inode(const char *path, cubalc_host_result *r);
/* owner uid: r->n = st_uid; soft miss missing path */
int cubalc_host_fileuid(const char *path, cubalc_host_result *r);
/* group gid: r->n = st_gid; soft miss missing path */
int cubalc_host_filegid(const char *path, cubalc_host_result *r);
/* access probe: mode is R_OK/W_OK/X_OK/F_OK; r->n = 1 if allowed, 0 if not; always ok if path non-empty */
int cubalc_host_can_access(const char *path, int mode, cubalc_host_result *r);
/* create/overwrite probe: existing file W_OK, else parent dir W_OK|X_OK; r->n = 0|1 */
int cubalc_host_can_create(const char *path, cubalc_host_result *r);
/* process umask: get current (r->n decimal, r->str 4-digit octal) */
int cubalc_host_umask_get(cubalc_host_result *r);
/* set umask to mode&0777; r->n/str = previous mask (decimal / 4-digit octal) */
int cubalc_host_umask_set(long mode, cubalc_host_result *r);
/* push cwd then chdir path; r->str = new cwd; r->n = stack depth */
int cubalc_host_pushd(const char *path, cubalc_host_result *r);
/* pop saved cwd and chdir; r->str = restored cwd; r->n = remaining depth */
int cubalc_host_popd(cubalc_host_result *r);
/* list dirstack as newline bag (bottom→top); r->n = depth */
int cubalc_host_dirstack(cubalc_host_result *r);
/* path kind label via lstat: r->str = missing|file|dir|link|fifo|sock|chr|blk|other;
 * r->n = 0..8 code; missing is probe ok=1 */
int cubalc_host_kindstr(const char *path, cubalc_host_result *r);
/* path relative to base (abspath both). Under base → relative; same → ".";
 * outside → absolute path. r->n = strlen; r->code = 1 if under base, 0 else */
int cubalc_host_relpath(const char *base, const char *path, cubalc_host_result *r);
/* process groups as newline names (egid+supplementary); r->n = count */
int cubalc_host_groups(cubalc_host_result *r);
/* membership: name or decimal gid; r->n = 0|1 */
int cubalc_host_ingroup(const char *name_or_gid, cubalc_host_result *r);
/* owner login name from st_uid via getpwuid; fallback decimal uid string; r->n = uid */
int cubalc_host_ownername(const char *path, cubalc_host_result *r);
/* group name from st_gid via getgrgid; fallback decimal gid string; r->n = gid */
int cubalc_host_groupname(const char *path, cubalc_host_result *r);
/* read symlink target into r->str; soft fail if not a symlink / missing */
int cubalc_host_readlink(const char *path, cubalc_host_result *r);
/* probe: r->n = 1 if path is a symlink, 0 if missing or not a link; always ok */
int cubalc_host_islink(const char *path, cubalc_host_result *r);
/* permission bits: r->n = st_mode & 07777; r->str = 4-digit octal "0644"; soft miss */
int cubalc_host_mode(const char *path, cubalc_host_result *r);
/* set permission bits (mode & 07777); soft miss if path missing */
int cubalc_host_chmod(const char *path, long mode, cubalc_host_result *r);
/* absolute path: realpath if exists, else cwd-join relative; always fills r->str */
int cubalc_host_abspath(const char *path, cubalc_host_result *r);
/* create empty file or update mtime; r->n=1 created, 0 updated */
int cubalc_host_touch(const char *path, cubalc_host_result *r);
/* list directory entries (skip . ..) → r->str newline-joined names, r->n=count */
int cubalc_host_listdir(const char *path, cubalc_host_result *r);

/* env */
int cubalc_host_env(const char *name, cubalc_host_result *r);
/* set process env (overwrite); val may be empty string */
int cubalc_host_env_set(const char *name, const char *val, cubalc_host_result *r);
/* unset process env; r->n = 1 if was set, 0 if already absent */
int cubalc_host_env_unset(const char *name, cubalc_host_result *r);

/* HTTP — loopback + allowlisted api hosts; auth via CUBALC_HTTP_AUTH / XAI_API_KEY */
int cubalc_host_http(const char *method, const char *url, const char *body,
                     cubalc_host_result *r);

/* process — allowlist only */
int cubalc_host_spawn(const char *bin, char *const argv[], cubalc_host_result *r);

/* which — resolve binary on PATH, then CubalC lib/program (readable) */
int cubalc_host_which(const char *name, cubalc_host_result *r);

/* which_bin — PATH / absolute / ./out / ~/.local/bin only (X_OK). No lib fallback.
 * Usability: REQUIRE BIN / SYS WHICHBIN host tool gates. */
int cubalc_host_which_bin(const char *name, cubalc_host_result *r);

/* find CubalC resource only: lib short name · programs/ · proof/ · path.
 * Prefer over PATH when callers want INCLUDE-style resolution (not bins). */
int cubalc_host_find_cubalc(const char *name, cubalc_host_result *r);

/* path join: a + "/" + b (no double slash) → r->str */
int cubalc_host_join(const char *a, const char *b, cubalc_host_result *r);

/* extract last non-empty JSON string field "key" from json blob → r->str */
int cubalc_host_json_get(const char *json, const char *key, cubalc_host_result *r);

/* chat: backend "local"|"grok", model may be ""/"local", msg required.
 * Reply text in r->str. Uses loopback :1212 or api.x.ai. */
int cubalc_host_chat(const char *backend, const char *model, const char *msg,
                     const char *state_dir, cubalc_host_result *r);

#ifdef __cplusplus
}
#endif
#endif
