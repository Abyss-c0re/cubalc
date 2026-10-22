/* CubalC lang — lang_ops_cell_runtime.c
 * CELL runtime: DECIDE·VIZ·SPIN/SHOW/HELLO·WAIT/SLEEP (G peel).
 * Mesh vote G_cell_margin · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_runtime.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>

int cubalc_lang_ops_cell_try_runtime(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  if (kw(&L->cur,"DECIDE")||kw(&L->cur,"ALGOCUBE")){
    lex_next(L);
    char id[48]={0};
    if (L->cur.kind==TK_IDENT && !kw(&L->cur,"END") && !kw(&L->cur,"ELSE")
        && !kw(&L->cur,"LET") && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"PRINT")){
      snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    }
    long d=do_decide(vm, id[0]?id:NULL);
    if (vm->trace) fprintf(vm->trace,"decide %ld\n",d);
    if (vm->res) snprintf(vm->res->last_print,sizeof vm->res->last_print,"decide %ld",d);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"VIZ")||kw(&L->cur,"PUBLISH_VIZ")){
    lex_next(L);
    ensure_world(vm);
    char path[512]="state/cubalc_viz_frame.json";
    if (L->cur.kind==TK_STR){ snprintf(path,sizeof path,"%s",L->cur.text); lex_next(L); }
    else if (L->cur.kind==TK_IDENT){
      Var *v=var_get(vm,L->cur.text,0);
      if (v&&v->is_str) snprintf(path,sizeof path,"%s",v->sval);
      lex_next(L);
    }
    cubalc_chain_write_viz(&vm->ch, path);
    cubalc_chain_publish_united(&vm->ch);
    var_set_str(vm,"LAST", path); var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SPIN")||kw(&L->cur,"SHOW")||kw(&L->cur,"HELLO")){
    lex_next(L);
    char id[48]={0};
    if (L->cur.kind==TK_IDENT){ snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L); }
    do_show(vm, id[0]?id:NULL);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"WAIT")||kw(&L->cur,"SLEEP")){
    lex_next(L);
    long ms = 100;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS)
      ms = parse_expr(vm,L);
    if (ms < 0) ms = 0; if (ms > 60000) ms = 60000;
    if (ms > 0){ struct timespec ts; ts.tv_sec = ms/1000; ts.tv_nsec = (ms%1000)*1000000L; nanosleep(&ts, NULL); }
    bump(vm); return 1;
  }
  return 0;
}
