/* CubalC lang — SYS time/walk plane (PARSEMS PARSEISO WALK timeouts).
 * Split from lang_ops_core (mesh vote A_split_lang_ops_core).
 */
#ifndef CUBALC_LANG_OPS_SYS_TIME_WALK_H
#define CUBALC_LANG_OPS_SYS_TIME_WALK_H

#include "lang/cubalc_lang_internal.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int cubalc_walk_match(const char *pat, const char *name);
int cubalc_parse_duration_ms(const char *src, long *ms_out, int *segs_out,
                             const char **err_out);
int cubalc_parse_optional_timeout_ms(VM *vm, Lex *L, long *timeout_ms,
                                     const char **err_out);
int cubalc_parse_iso_epoch(const char *src, long *epoch_out, const char **err_out);
void cubalc_format_epoch_iso(long n, char *iso, size_t cap);
void cubalc_sys_walk_rec(const char *dir, const char *pat,
                         char *out, size_t outsz, size_t *olen, long *kept,
                         int depth, long *ndirs, long *nfiles);

#ifdef __cplusplus
}
#endif
#endif
