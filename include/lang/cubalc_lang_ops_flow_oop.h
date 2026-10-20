/* CubalC lang — flow OOP/CLI helper plane (A3).
 * Split from lang_ops_flow.c
 */
#ifndef CUBALC_LANG_OPS_FLOW_OOP_H
#define CUBALC_LANG_OPS_FLOW_OOP_H

#include "lang/cubalc_lang_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

ClassDef *oop_find_class(VM *vm, const char *name);
ObjInst *oop_find_obj(VM *vm, const char *name);
MethodDef *oop_find_method(ClassDef *cd, const char *mname);

int mi_read_flag_aliases(Lex *L, char *bag, size_t bagn, char *first, size_t firstn);
int mi_scan_cli_flag_any(const char *names_bag, char *out, size_t outn, char *hit_name, size_t hitn);
int oop_find_class_idx(VM *vm, const char *name);
int oop_field_idx(ClassDef *cd, const char *fname);
int oop_field_def_same(const FieldDef *a, const FieldDef *b);
int oop_inherit_flatten(VM *vm, ClassDef *cd, ClassDef *base);
int oop_propagate_extend(VM *vm, int src_idx);
int oop_isof_chain(VM *vm, int class_idx, const char *cname);
int oop_parse_class_body(VM *vm, Lex *L, ClassDef *cd, const char *cname);
void oop_suggest_class(VM *vm, const char *typo, char *out, size_t outn);
void oop_suggest_obj(VM *vm, const char *typo, char *out, size_t outn);
void oop_suggest_method(ClassDef *cd, const char *typo, char *out, size_t outn);
void oop_suggest_field(ClassDef *cd, const char *typo, char *out, size_t outn);
void oop_suggest_fn(VM *vm, const char *typo, char *out, size_t outn);
void oop_err_suggest(char *buf, size_t n, const char *base, const char *sug);
int oop_resolve_obj_name(VM *vm, Lex *L, char *out, size_t outn);
int oop_stmt_kw(Lex *L);
void call_apply_or_last(VM *vm, const char *fb);
int call_parse_or_fallback(VM *vm, Lex *L, char *fb, size_t fbn);
int oop_bind_args(VM *vm, Lex *L, char params[][32], int n_params);
int oop_arity_gate(VM *vm, const char *tag, const char *name, char params[][32], int n_params, int got, int soft);
int oop_read_name(VM *vm, Lex *L, char *out, size_t outn, const char *why);
int oop_run_method(VM *vm, ObjInst *ob, MethodDef *md);
int oop_new_instance(VM *vm, const char *cname, const char *oname, int cube_idx, Lex *L, int bind_ctor, int soft);

long case_when_num_atom(VM *vm, Lex *L);

#ifdef __cplusplus
}
#endif
#endif /* CUBALC_LANG_OPS_FLOW_OOP_H */
