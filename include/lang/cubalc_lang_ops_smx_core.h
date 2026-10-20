#ifndef CUBALC_LANG_OPS_SMX_CORE_H
#define CUBALC_LANG_OPS_SMX_CORE_H
#include "lang/cubalc_lang_internal.h"
#ifdef __cplusplus
extern "C" {
#endif

int p2p_timeout_ms(void);
int p2p_soft(void);
int accept_timeout(int lfd, int timeout_ms, int *timed_out);
int ensure_smx_key(VM *vm);
int do_smx_talk(VM *vm, const char *from_id, const char *to_id);
int do_smx_seal(VM *vm, const char *from_id, const char *to_id, const char *path);
int do_smx_open(VM *vm, const char *to_id, const char *path);
int parse_host_port(const char *endpoint, char *host, size_t hostn, int *port);

void smx_fail_at(VM *vm, int line, const char *core, const char *hint);
int cubalc_lang_ops_smx_try_core(VM *vm, Lex *L);
#ifdef __cplusplus
}
#endif
#endif
