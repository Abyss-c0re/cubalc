/* CubalC lang — SYS CLI / pathline / did-you-mean plane.
 * Split from lang_ops_core (mesh vote A_split_lang_ops_core).
 */
#ifndef CUBALC_LANG_OPS_SYS_CLI_H
#define CUBALC_LANG_OPS_SYS_CLI_H

#include "lang/cubalc_lang_internal.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int cubalc_peel_pathline(const char *s, char *path, size_t pathn, long *lineno, char *text, size_t textn);
void sys_suggest_op(const char *typo, char *out, size_t outn);
int cubalc_scan_cli_flag(const char *name, char *out, size_t outn);
int cubalc_read_flag_aliases(Lex *L, char *bag, size_t bagn, char *first, size_t firstn);
int cubalc_scan_cli_flag_any(const char *names_bag, char *out, size_t outn, char *hit_name, size_t hitn);
int cubalc_scan_cli_flag_all(const char *names_bag, char *out, size_t outn, char *first_hit, size_t hitn);
int cubalc_csv_expand_bag(const char *in, char *out, size_t outn);
int cubalc_str_truthy(const char *s);
int cubalc_read_str_atom(VM *vm, Lex *L, char *out, size_t outn);
int cubalc_match_oneof_list(VM *vm, Lex *L, const char *subject, int icase, char *hit_out, size_t hit_n);
int cubalc_read_num_atom(VM *vm, Lex *L, long *out);
long cubalc_collect_restargs(char *out, size_t outn);
long cubalc_collect_restpos(char *out, size_t outn);
long cubalc_collect_listflags(char *out, size_t outn);
long cubalc_collect_flagmap(char *out, size_t outn);
long cubalc_collect_argmap(char *out, size_t outn);
void cubalc_append_usage_tip(VM *vm, char *msg, size_t n);

#ifdef __cplusplus
}
#endif
#endif
