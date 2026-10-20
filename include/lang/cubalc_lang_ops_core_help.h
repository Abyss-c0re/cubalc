/* CubalC lang — HELP / HASFORM catalog plane.
 * Split from lang_ops_core (mesh vote A_split_lang_ops_core).
 * Law: zero bloat · never grow lang_ops_core · cube is SoT.
 */
#ifndef CUBALC_LANG_OPS_CORE_HELP_H
#define CUBALC_LANG_OPS_CORE_HELP_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CubalcHelpEnt {
  const char *name;
  const char *hint;
} CubalcHelpEnt;

extern const CubalcHelpEnt cubalc_help_catalog[];
int cubalc_help_catalog_n(void);
int cubalc_form_known(const char *want, const char **hint_out);

#ifdef __cplusplus
}
#endif
#endif /* CUBALC_LANG_OPS_CORE_HELP_H */
