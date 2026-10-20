/* CubalC lang — lang_ops_core_assert.c
 * ASSERT / EXPECT / FAIL / HARDFAIL / PASS plane (A_core peel).
 * Mesh vote A_core_forms_agent_peel · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_core_assert.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void assert_fail_msg(VM *vm, char *msg, size_t n, const char *tag, int aln,
                            const char *why){
  char body[140];
  body[0] = 0;
  if (vm->last_cmp_kind == 1 || vm->last_cmp_kind == 2) {
    if (vm->last_cmp_kind == 2)
      snprintf(body, sizeof body, "\"%s\" %s \"%s\" is false",
               vm->last_cmp_left, vm->last_cmp_op, vm->last_cmp_right);
    else
      snprintf(body, sizeof body, "%s %s %s is false",
               vm->last_cmp_left, vm->last_cmp_op, vm->last_cmp_right);
  } else if (vm->last_cmp_kind == 3) {
    snprintf(body, sizeof body, "got %s (falsey)", vm->last_cmp_left);
  }
  if (body[0] && why && why[0])
    snprintf(msg, n, "%s failed line %d: %s — %s", tag, aln, body, why);
  else if (body[0])
    snprintf(msg, n, "%s failed line %d: %s", tag, aln, body);
  else if (why && why[0])
    snprintf(msg, n, "%s failed line %d: %s", tag, aln, why);
  else
    snprintf(msg, n, "%s failed line %d", tag, aln);
  /* agent plate knobs */
  if (vm->last_cmp_kind == 1 || vm->last_cmp_kind == 2) {
    var_set_str(vm, "ASSERT_GOT", vm->last_cmp_left);
    var_set_str(vm, "ASSERT_EXPECTED", vm->last_cmp_right);
    var_set_str(vm, "ASSERT_OP", vm->last_cmp_op);
  } else if (vm->last_cmp_kind == 3) {
    var_set_str(vm, "ASSERT_GOT", vm->last_cmp_left);
    var_set_str(vm, "ASSERT_EXPECTED", "truthy");
    var_set_str(vm, "ASSERT_OP", "");
  }
}

int cubalc_lang_ops_core_try_assert(VM *vm, Lex *L) {
  /* Assert/expect/fail/pass plane.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
  /* ASSERT expr ["why"] — optional message; fail shows got/expected from cmp. */
  if (kw(&L->cur,"ASSERT")){
    int aln = L->cur.line;
    lex_next(L);
    long v=parse_expr(vm,L);
    char why[120]; why[0]=0;
    if (L->cur.kind==TK_STR){
      snprintf(why, sizeof why, "%s", L->cur.text);
      lex_next(L);
    }
    if (v){
      if (vm->res) vm->res->asserts_ok++;
      if (vm->trace) fprintf(vm->trace,"# ok\n");
    } else {
      if (vm->res) vm->res->asserts_fail++;
      char msg[192];
      assert_fail_msg(vm, msg, sizeof msg, "ASSERT", aln, why);
      fail(vm, msg);
      return -1;
    }
    bump(vm); return 1;
  }
  /* EXPECT|CHECK|SOFTASSERT expr ["why"] — soft check: no fatal, program continues.
   * Sets OK/EXPECT_OK 0|1 and sticky LAST_ERR on fail. Does not fail the run plate
   * (use ASSERT for fail-closed). Bumps asserts_ok only on success. */
  if (kw(&L->cur,"EXPECT")||kw(&L->cur,"CHECK")||kw(&L->cur,"SOFTASSERT")||
      kw(&L->cur,"ASSERT_SOFT")||kw(&L->cur,"SOFT_ASSERT")){
    int aln = L->cur.line;
    lex_next(L);
    long v = parse_expr(vm, L);
    char why[120]; why[0] = 0;
    if (L->cur.kind == TK_STR){
      snprintf(why, sizeof why, "%s", L->cur.text);
      lex_next(L);
    }
    if (v){
      if (vm->res) vm->res->asserts_ok++;
      var_set_num(vm, "OK", 1);
      var_set_num(vm, "EXPECT_OK", 1);
      if (vm->trace) fprintf(vm->trace, "# expect ok\n");
    } else {
      char msg[192];
      assert_fail_msg(vm, msg, sizeof msg, "EXPECT", aln, why);
      /* sticky agent-readable — not fatal, not asserts_fail */
      var_set_str(vm, "ERR", msg);
      var_set_str(vm, "LAST_ERR", msg);
      var_set_str(vm, "LAST", msg);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", msg);
      vm->last_n = (long)strlen(msg);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 0);
      var_set_num(vm, "EXPECT_OK", 0);
      if (vm->trace) fprintf(vm->trace, "# expect fail: %s\n", msg);
    }
    bump(vm); return 1;
  }
  /* FAIL ["why"] — intentional soft fail (no expr, no fatal). Sticky LAST_ERR/OK=0.
   * PASS ["why"] — intentional soft pass; OK=1 and optional LAST note. */
  if (kw(&L->cur,"FAIL")||kw(&L->cur,"SOFTFAIL")||kw(&L->cur,"MARK_FAIL")){
    int aln = L->cur.line;
    lex_next(L);
    char why[120]; why[0] = 0;
    if (L->cur.kind == TK_STR){
      snprintf(why, sizeof why, "%s", L->cur.text);
      lex_next(L);
    }
    char msg[160];
    if (why[0])
      snprintf(msg, sizeof msg, "FAIL line %d: %s", aln, why);
    else
      snprintf(msg, sizeof msg, "FAIL line %d", aln);
    var_set_str(vm, "ERR", msg);
    var_set_str(vm, "LAST_ERR", msg);
    var_set_str(vm, "LAST", msg);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", msg);
    vm->last_n = (long)strlen(msg);
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "OK", 0);
    var_set_num(vm, "EXPECT_OK", 0);
    if (vm->trace) fprintf(vm->trace, "# fail: %s\n", msg);
    bump(vm); return 1;
  }
  /* HARDFAIL|FATAL|FAILHARD ["why"] — soft FAIL sticky + halt exit 1.
   * Usability: guard hard paths stop the program without cubalc run -s / STRICT.
   * Soft dual remains FAIL/SOFTFAIL. Twin of EXIT 1 with FAIL-shaped messages. */
  if (kw(&L->cur,"HARDFAIL")||kw(&L->cur,"FATAL")||kw(&L->cur,"FAILHARD")||
      kw(&L->cur,"HARD_FAIL")||kw(&L->cur,"ABORTFAIL")||kw(&L->cur,"DIEFAIL")){
    int aln = L->cur.line;
    lex_next(L);
    char why[CUBALC_HOST_STR_MAX];
    why[0] = 0;
    /* STR, string-var, or LAST — agent: HARDFAIL LAST_ERR after LET ERR=… */
    if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (resolve_str_arg(vm, L, why, sizeof why) != 0)
        why[0] = 0;
      /* peel trailing CAT fragments: "a" + VAR already unresolved — use simple msg */
      while (L->cur.kind == TK_PLUS || (L->cur.kind == TK_IDENT &&
             !strcmp(L->cur.text, "+"))) {
        /* skip broken concat tails left by old FAIL "x" + VAR style */
        if (L->cur.kind == TK_PLUS) lex_next(L);
        else lex_next(L);
        char skip[96];
        (void)resolve_str_arg(vm, L, skip, sizeof skip);
      }
    }
    char msg[200];
    if (why[0])
      snprintf(msg, sizeof msg, "HARDFAIL line %d: %s", aln, why);
    else
      snprintf(msg, sizeof msg, "HARDFAIL line %d", aln);
    var_set_str(vm, "ERR", msg);
    var_set_str(vm, "LAST_ERR", msg);
    var_set_str(vm, "LAST", msg);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", msg);
    vm->last_n = (long)strlen(msg);
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "OK", 0);
    var_set_num(vm, "EXPECT_OK", 0);
    var_set_num(vm, "HARDFAIL", 1);
    vm->halt = 1;
    vm->exit_code = 1;
    vm->break_loop = 1;
    vm->return_fn = 1;
    var_set_num(vm, "EXIT", 1);
    if (vm->trace) fprintf(vm->trace, "# hardfail: %s\n", msg);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"PASS")||kw(&L->cur,"MARK_PASS")||kw(&L->cur,"OKAY")){
    int aln = L->cur.line;
    lex_next(L);
    char why[120]; why[0] = 0;
    if (L->cur.kind == TK_STR){
      snprintf(why, sizeof why, "%s", L->cur.text);
      lex_next(L);
    }
    char msg[160];
    if (why[0])
      snprintf(msg, sizeof msg, "%s", why);
    else
      snprintf(msg, sizeof msg, "PASS line %d", aln);
    var_set_str(vm, "LAST", msg);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", msg);
    vm->last_n = (long)strlen(msg);
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "OK", 1);
    var_set_num(vm, "EXPECT_OK", 1);
    if (vm->trace) fprintf(vm->trace, "# pass: %s\n", msg);
    bump(vm); return 1;
  }
  return 0;
}
