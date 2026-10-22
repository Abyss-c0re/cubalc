/* CubalC lang — lang_ops_core.c thin dispatcher (modular SoT)
 * DO_NOT_AMALGAMATE_CORE — Law: never re-amalgamate; shards live in lang_ops_core_*.c
 */
#include "lang/cubalc_lang_internal.h"
#include "lang/cubalc_lang_ops_core_async.h"
#include "lang/cubalc_lang_ops_core_sys.h"
#include "lang/cubalc_lang_ops_core_forms.h"
#include "lang/cubalc_lang_ops_core_print.h"
#include "lang/cubalc_lang_ops_core_agent.h"
#include "lang/cubalc_lang_ops_core_assert.h"
#include "lang/cubalc_lang_ops_core_clearerr.h"
#include "lang/cubalc_lang_ops_core_let.h"
#include "lang/cubalc_lang_ops_core_cop.h"
#include "lang/cubalc_lang_ops_core_ctrl.h"
#include "lang/cubalc_lang_ops_core_discover.h"
#include "lang/cubalc_lang_ops_core_plate.h"
#include "lang/cubalc_lang_ops_core_plate_ext.h"
#include "lang/cubalc_lang_ops_core_plate_nest.h"
#include "lang/cubalc_lang_ops_core_flat.h"
#include "cubalc_law.h"
#include <string.h>
#include <stdio.h>

int cubalc_lang_ops_core(VM *vm, Lex *L) {
  int r;
  r = cubalc_lang_ops_core_try_async(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_clearerr(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_let(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_sys(vm, L); if (r) return r;

  /* CREED / HOLD_FLASH / SHARE / GENESIS — small law surface kept here */
  if (kw(&L->cur, "CREED")) {
    lex_next(L);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", CUBALC_CREED);
    var_set_str(vm, "LAST", vm->last_str);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }
  if (kw(&L->cur, "HOLD_FLASH")) {
    lex_next(L);
    long v = 1;
    if (L->cur.kind == TK_NUM) {
      v = L->cur.num;
      lex_next(L);
    }
    vm->hold_flash = v ? 1 : 0;
    var_set_num(vm, "HOLD_FLASH", vm->hold_flash);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }
  if (kw(&L->cur, "SHARE") || kw(&L->cur, "SHARE_LEVEL")) {
    lex_next(L);
    /* optional tail (SHARE state_matrix_only) */
    while (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR || L->cur.kind == TK_NUM)
      lex_next(L);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", CUBALC_SHARE);
    var_set_str(vm, "LAST", vm->last_str);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }
  /* BUDGET n — mesh/session energy budget knob (law surface; hold_seed). */
  if (kw(&L->cur, "BUDGET")) {
    long v = 0;
    lex_next(L);
    if (L->cur.kind == TK_NUM) {
      v = L->cur.num;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && !vv->is_str) v = vv->val;
      lex_next(L);
    } else {
      fail(vm, "BUDGET needs number");
      return -1;
    }
    var_set_num(vm, "BUDGET", v);
    var_set_num(vm, "LAST_N", v);
    vm->last_n = v;
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }
  if (kw(&L->cur, "GENESIS")) {
    lex_next(L);
    char plate[512] = "NEXUS_COORD v1 | from=play | hold_flash=1 |";
    if (L->cur.kind == TK_STR) {
      snprintf(plate, sizeof plate, "%s", L->cur.text);
      lex_next(L);
    }
    var_set_str(vm, "GENESIS", plate);
    var_set_str(vm, "LAST", plate);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  r = cubalc_lang_ops_core_try_cop(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_forms(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_print(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_agent(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_assert(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_ctrl(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_discover(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_plate(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_plate_ext(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_plate_nest(vm, L); if (r) return r;
  r = cubalc_lang_ops_core_try_flat(vm, L); if (r) return r;


  return 0;
}
