/* CubalC lang — LET · INC/DEC · SWAP (core var plane; rescued from nest_sortp residual) */
#include "lang/cubalc_lang_ops_core_let.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int cubalc_lang_ops_core_try_let(VM *vm, Lex *L) {
  if (kw(&L->cur,"LET")){
    int aln = L->cur.line;
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){
      char ebuf[160];
      snprintf(ebuf, sizeof ebuf,
               "LET needs name [=] value line %d — LET x = 1", aln);
      fail(vm, ebuf); return -1;
    }
    char name[48]; snprintf(name,sizeof name,"%s",L->cur.text); lex_next(L);
    /* '=' optional when next token is a value — LET x 1 same as LET x = 1 */
    if (L->cur.kind==TK_EQ){
      lex_next(L);
    } else if (!(L->cur.kind==TK_NUM || L->cur.kind==TK_STR ||
                 L->cur.kind==TK_MINUS || L->cur.kind==TK_LPAREN ||
                 L->cur.kind==TK_IDENT)){
      char ebuf[160];
      snprintf(ebuf, sizeof ebuf,
               "LET '%s' needs value line %d — LET %s = 1", name, aln, name);
      fail(vm, ebuf); return -1;
    }
    if (L->cur.kind==TK_STR || (L->cur.kind==TK_IDENT && (
          strcmp(L->cur.text,"LAST")==0 ||
          (var_get(vm,L->cur.text,0) && var_get(vm,L->cur.text,0)->is_str)))){
      char buf[CUBALC_HOST_STR_MAX]; buf[0]=0;
      for(;;){
        if (L->cur.kind==TK_STR){
          size_t bl=strlen(buf), al=strlen(L->cur.text);
          if (bl+al+1 < sizeof buf) memcpy(buf+bl, L->cur.text, al+1);
          lex_next(L);
        } else if (L->cur.kind==TK_IDENT){
          if (strcmp(L->cur.text,"LAST")==0){
            size_t bl=strlen(buf), al=strlen(vm->last_str);
            if (bl+al+1 < sizeof buf) memcpy(buf+bl, vm->last_str, al+1);
            lex_next(L);
          } else {
            Var *v = var_get(vm, L->cur.text, 0);
            if (v && v->is_str){
              size_t bl=strlen(buf), al=strlen(v->sval);
              if (bl+al+1 < sizeof buf) memcpy(buf+bl, v->sval, al+1);
            } else if (v){
              char nb[32]; snprintf(nb,sizeof nb,"%ld", v->val);
              size_t bl=strlen(buf), al=strlen(nb);
              if (bl+al+1 < sizeof buf) memcpy(buf+bl, nb, al+1);
            } else break;
            lex_next(L);
          }
        } else break;
        if (L->cur.kind==TK_PLUS){ lex_next(L); continue; }
        break;
      }
      var_set_str(vm, name, buf);
      bump(vm); return 1;
    }
    long v=parse_expr(vm,L);
    var_set_num(vm, name, v);
    bump(vm); return 1;
  }
  /* INC|BUMP|ADDTO name [delta] — numeric var += delta (default 1).
   * DEC|SUBFROM name [delta] — numeric var -= delta (default 1).
   * Creates var as 0 if missing. Str var promoted via strtol (0 if non-num).
   * Usability: counters/retries without LET x = x + 1 glue. */
  if (kw(&L->cur,"INC")||kw(&L->cur,"BUMP")||kw(&L->cur,"ADDTO")||
      kw(&L->cur,"INCR")||kw(&L->cur,"INCREMENT")||
      kw(&L->cur,"DEC")||kw(&L->cur,"SUBFROM")||kw(&L->cur,"DECR")||
      kw(&L->cur,"DECREMENT")||kw(&L->cur,"SUBTO")){
    char op[24];
    int is_dec = 0;
    char name[48];
    long delta = 1, cur = 0, nv;
    Var *vv;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "DEC") == 0 || strcmp(op, "SUBFROM") == 0 ||
        strcmp(op, "DECR") == 0 || strcmp(op, "DECREMENT") == 0 ||
        strcmp(op, "SUBTO") == 0)
      is_dec = 1;
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, is_dec ? "DEC needs name [n] — DEC retries" : "INC needs name [n] — INC retries");
      return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    if (kw(&L->cur, "BY") || kw(&L->cur, "OF") || kw(&L->cur, "WITH"))
      lex_next(L);
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
        (L->cur.kind == TK_IDENT && !kw(&L->cur, "END") && !kw(&L->cur, "IF") &&
         !kw(&L->cur, "LET") && !kw(&L->cur, "ASSERT") && !kw(&L->cur, "PRINT") &&
         !kw(&L->cur, "PASS") && !kw(&L->cur, "FAIL") && !kw(&L->cur, "INC") &&
         !kw(&L->cur, "DEC") && !kw(&L->cur, "SWAP")))
      delta = parse_expr(vm, L);
    if (delta < 0) {
      /* allow negative delta as opposite direction */
    }
    vv = var_get(vm, name, 0);
    if (vv && !vv->is_str) {
      cur = vv->val;
    } else if (vv && vv->is_str) {
      char *end = NULL;
      cur = strtol(vv->sval, &end, 10);
      if (!(end && end != vv->sval && *end == 0))
        cur = 0;
    } else {
      cur = 0;
    }
    nv = is_dec ? (cur - delta) : (cur + delta);
    var_set_num(vm, name, nv);
    var_set_num(vm, "LAST_N", nv);
    vm->last_n = nv;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", nv);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, is_dec ? "DEC_N" : "INC_N", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s -> %ld\n", is_dec ? "DEC" : "INC", name, nv);
    bump(vm);
    return 1;
  }
  /* SWAP|XCHG a [WITH|AND] b — exchange two vars (num or str).
   * Usability: dual buffers without temp LET t = a glue. */
  if (kw(&L->cur,"SWAP")||kw(&L->cur,"XCHG")||kw(&L->cur,"EXCHANGE")||
      kw(&L->cur,"SWAPVARS")||kw(&L->cur,"SWAPV")){
    char a[48], b[48];
    Var *va, *vb;
    int a_str = 0, b_str = 0;
    long a_n = 0, b_n = 0;
    char a_s[CUBALC_HOST_STR_MAX], b_s[CUBALC_HOST_STR_MAX];
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, "SWAP needs two names — SWAP a b");
      return -1;
    }
    snprintf(a, sizeof a, "%s", L->cur.text);
    lex_next(L);
    if (kw(&L->cur, "WITH") || kw(&L->cur, "AND") || kw(&L->cur, "OR") ||
        kw(&L->cur, ",") || kw(&L->cur, "TO"))
      lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, "SWAP needs two names — SWAP a b");
      return -1;
    }
    snprintf(b, sizeof b, "%s", L->cur.text);
    lex_next(L);
    a_s[0] = b_s[0] = 0;
    va = var_get(vm, a, 0);
    vb = var_get(vm, b, 0);
    if (va) {
      if (va->is_str) {
        a_str = 1;
        snprintf(a_s, sizeof a_s, "%s", va->sval);
      } else {
        a_n = va->val;
      }
    }
    if (vb) {
      if (vb->is_str) {
        b_str = 1;
        snprintf(b_s, sizeof b_s, "%s", vb->sval);
      } else {
        b_n = vb->val;
      }
    }
    /* write b's value into a, a's into b */
    if (b_str)
      var_set_str(vm, a, b_s);
    else
      var_set_num(vm, a, b_n);
    if (a_str)
      var_set_str(vm, b, a_s);
    else
      var_set_num(vm, b, a_n);
    var_set_num(vm, "SWAP_N", 1);
    var_set_num(vm, "OK", 1);
    var_set_num(vm, "LAST_N", 1);
    vm->last_n = 1;
    var_set_str(vm, "LAST", "swapped");
    snprintf(vm->last_str, sizeof vm->last_str, "%s", "swapped");
    if (vm->trace)
      fprintf(vm->trace, "# SWAP %s <-> %s\n", a, b);
    bump(vm);
    return 1;
  }
  return 0;
}
