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
/* mkdir -p style: create path and parents; OK if already a directory */
int cubalc_host_mkdir(const char *path, cubalc_host_result *r);
/* unlink regular file only; missing → ok with n=0; dir → error */
int cubalc_host_rm(const char *path, cubalc_host_result *r);
/* rename/move path; soft miss if from missing */
int cubalc_host_rename(const char *from, const char *to, cubalc_host_result *r);
/* copy regular file src → dst; r->n = bytes written */
int cubalc_host_copy(const char *src, const char *dst, cubalc_host_result *r);

/* env */
int cubalc_host_env(const char *name, cubalc_host_result *r);

/* HTTP — loopback + allowlisted api hosts; auth via CUBALC_HTTP_AUTH / XAI_API_KEY */
int cubalc_host_http(const char *method, const char *url, const char *body,
                     cubalc_host_result *r);

/* process — allowlist only */
int cubalc_host_spawn(const char *bin, char *const argv[], cubalc_host_result *r);

/* which — resolve binary on PATH, then CubalC lib/program (readable) */
int cubalc_host_which(const char *name, cubalc_host_result *r);

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
