#ifndef CUBALC_RUN_UTIL_H
#define CUBALC_RUN_UTIL_H
#include <stddef.h>
int run_version_ge(const char *have, const char *need);
int run_safe_include_token(const char *s);
void run_preload_add(char preload[][96], int *n_preload, const char *name);
int run_includes_has_stem(const char *includes, const char *want);
int run_includes_stems_json(const char *includes, char *outj, size_t outn);
int run_preload_miss_json(char preload[][96], int n_preload,
                          const char *includes, char *outj, size_t outn);
void run_preload_parse_env(char preload[][96], int *n_preload, const char *env);
void run_dotenv_add(char paths[][512], int *n, const char *path);
void run_dotenv_parse_env(char paths[][512], int *n, const char *env);
int run_mkdir_p(const char *path, char *err, size_t errn);
int run_setenv_one(const char *kv, char *err, size_t errn);
int run_dotenv_load_one(const char *path, long *keys_out, char *err, size_t errn);
char *run_build_with_preload(char preload[][96], int n_preload,
                             const char *body, size_t body_len, size_t *out_len);
#endif
