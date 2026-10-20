/* CubalC lang — SYS plate expand plane (SUBSTENV FILLP FILLPKEYS).
 * Split from lang_ops_core (mesh vote A_split_lang_ops_core).
 */
#ifndef CUBALC_LANG_OPS_SYS_PLATE_EXPAND_H
#define CUBALC_LANG_OPS_SYS_PLATE_EXPAND_H

#include "lang/cubalc_lang_internal.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

long cubalc_expand_substenv(VM *vm, const char *tmpl, char *out, size_t outcap,
                            long *miss_out);
long cubalc_expand_fillp(const char *plate, const char *tmpl, char *out,
                         size_t outcap, long *miss_out,
                         char *miss_bag, size_t miss_cap);
long cubalc_fillp_keys(const char *tmpl, char *out, size_t outcap);

#ifdef __cplusplus
}
#endif
#endif
