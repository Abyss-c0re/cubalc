/* CubalC lang — lang_parse.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_parse_form(VM *vm, Lex *L){
  skip_nl(L);
  if (L->cur.kind==TK_EOF) return 0;

  int r;
  if ((r = cubalc_lang_ops_core(vm, L)) != 0) return r;
  if ((r = cubalc_lang_ops_toc(vm, L)) != 0) return r;
  if ((r = cubalc_lang_ops_stack(vm, L)) != 0) return r;
  if ((r = cubalc_lang_ops_dual(vm, L)) != 0) return r;
  if ((r = cubalc_lang_ops_math(vm, L)) != 0) return r;
  if ((r = cubalc_lang_ops_bit(vm, L)) != 0) return r;
  if ((r = cubalc_lang_ops_cell(vm, L)) != 0) return r;
  if ((r = cubalc_lang_ops_flow(vm, L)) != 0) return r;

  snprintf(vm->err,sizeof vm->err,"unknown form '%s' line %d — place a unit with [name]",
           L->cur.text, L->cur.line);
  fail(vm, vm->err);
  return -1;
}
