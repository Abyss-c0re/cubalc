/* CubalC — CLEAR_ERR · VERSION · NEEDOK (rescued from residual monolit)
 * Dispatches endarg peels (FIRSTARG/LASTARG/NTHARG) without touching thin core.
 */
#include "lang/cubalc_lang_ops_core_clearerr.h"
#include "lang/cubalc_lang_ops_core_endarg.h"
#include "cubalc_law.h"
#include <stdio.h>
#include <string.h>

int cubalc_lang_ops_core_try_clearerr(VM *vm, Lex *L) {
  /* CLEAR_ERR [note] — wipe sticky ERR/LAST_ERR after soft recovery.
   * PASS restores OK but leaves LAST_ERR (agents read plate last_err); this
   * intentionally clears so recovered paths show a clean plate. Does not set OK. */
  if (kw(&L->cur,"CLEAR_ERR")||kw(&L->cur,"CLEARERR")||kw(&L->cur,"ERR_CLEAR")||
      kw(&L->cur,"WIPE_ERR")||kw(&L->cur,"RESET_ERR")||kw(&L->cur,"CLRERR")){
    char note[120];
    lex_next(L);
    note[0] = 0;
    if (L->cur.kind == TK_STR){
      snprintf(note, sizeof note, "%s", L->cur.text);
      lex_next(L);
    }
    var_set_str(vm, "ERR", "");
    var_set_str(vm, "LAST_ERR", "");
    vm->err[0] = 0;
    if (note[0]) {
      var_set_str(vm, "LAST", note);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", note);
      vm->last_n = (long)strlen(note);
    } else {
      var_set_str(vm, "LAST", "cleared");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "cleared");
      vm->last_n = 7;
    }
    var_set_num(vm, "LAST_N", vm->last_n);
    if (vm->trace) fprintf(vm->trace, "# clear_err %s\n", note[0] ? note : "cleared");
    bump(vm); return 1;
  }
  /* VERSION — agent/human plate: language version string → LAST / VERSION / OK */
  if (kw(&L->cur,"VERSION")||kw(&L->cur,"LANG_VERSION")||kw(&L->cur,"CUBALC_VERSION")){
    lex_next(L);
    const char *ver = CUBALC_LANG_VERSION;
    var_set_str(vm, "VERSION", ver);
    var_set_str(vm, "LAST", ver);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", ver);
    vm->last_n = (long)strlen(ver);
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "OK", 1);
    if (vm->trace) fprintf(vm->trace, "# version %s\n", ver);
    bump(vm); return 1;
  }
  /* NEEDOK|MUSTOK|ASSERTOK|CHECKOK ["why"] — standalone REQUIRE OK (no REQUIRE prefix).
   * Usability: one-token soft-status floor after TRYCALL/EXPECT/FAIL chains. */
  if (kw(&L->cur,"NEEDOK")||kw(&L->cur,"MUSTOK")||kw(&L->cur,"ASSERTOK")||
      kw(&L->cur,"CHECKOK")||kw(&L->cur,"OKCHECK")||kw(&L->cur,"ENSUREOK")||
      kw(&L->cur,"NEED_OK")||kw(&L->cur,"MUST_OK")||kw(&L->cur,"ASSERT_OK")){
    int aln = L->cur.line;
    char why[120];
    Var *ov, *ev;
    long okv = 1;
    int bad = 0;
    lex_next(L);
    why[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(why, sizeof why, "%s", L->cur.text);
      lex_next(L);
    }
    ov = var_get(vm, "OK", 0);
    ev = var_get(vm, "LAST_ERR", 0);
    if (ov) okv = ov->val;
    if (okv == 0) bad = 1;
    if (ev && ev->is_str && ev->sval[0]) bad = 1;
    if (bad) {
      char msg[220];
      const char *err = (ev && ev->is_str && ev->sval[0]) ? ev->sval : "OK=0";
      if (why[0])
        snprintf(msg, sizeof msg,
                 "NEEDOK failed line %d: %s — %s · CATCH/CLEAR_ERR · WHY",
                 aln, why, err);
      else
        snprintf(msg, sizeof msg,
                 "NEEDOK failed line %d: %s — CATCH/CLEAR_ERR · WHY",
                 aln, err);
      if (vm->res) vm->res->asserts_fail++;
      fail(vm, msg);
      return -1;
    }
    var_set_num(vm, "LAST_N", 1);
    vm->last_n = 1;
    var_set_num(vm, "REQUIRE_OK", 1);
    var_set_num(vm, "OK", 1);
    var_set_str(vm, "LAST", "ok");
    snprintf(vm->last_str, sizeof vm->last_str, "%s", "ok");
    if (vm->res) vm->res->asserts_ok++;
    bump(vm); return 1;
  }
  {
    int r = cubalc_lang_ops_core_try_endarg(vm, L);
    if (r) return r;
  }
  return 0;
}
