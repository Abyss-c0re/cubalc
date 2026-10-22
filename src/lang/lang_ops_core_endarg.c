/* CubalC — FIRSTARG/LASTARG/NTHARG raw argv peels (rescued post-thin SoT)
 * CLI dual: cubalc firstarg|lastarg (cubalc_cmd_endarg). Cube is SoT.
 */
#include "lang/cubalc_lang_ops_core_endarg.h"
#include "cubalc_law.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int endarg_argc(void) {
  const char *ac = getenv("CUBALC_ARGC");
  char envn[32];
  int argc = 0, k;
  if (ac && ac[0]) {
    argc = (int)strtol(ac, NULL, 10);
    if (argc < 0) argc = 0;
    if (argc > 32) argc = 32;
    return argc;
  }
  for (k = 0; k < 32; k++) {
    snprintf(envn, sizeof envn, "CUBALC_ARG%d", k);
    if (!getenv(envn)) break;
    argc++;
  }
  return argc;
}

int cubalc_lang_ops_core_try_endarg(VM *vm, Lex *L) {
  /* FIRSTARG|HEADARG [OR fallback] · LASTARG|TAILARG [OR fallback]
   * Peel first / last CUBALC_ARGi raw (flags kept). LAST_N 1 hit / 0 miss·fallback.
   * FIRSTARG_N=0 · LASTARG_N=argc-1 when hit. Completes NTHARG ends without ARGC glue. */
  if (kw(&L->cur, "FIRSTARG") || kw(&L->cur, "HEADARG") || kw(&L->cur, "ARGV0") ||
      kw(&L->cur, "ARG0") || kw(&L->cur, "FIRSTARGV") || kw(&L->cur, "PEELFIRST") ||
      kw(&L->cur, "LASTARG") || kw(&L->cur, "TAILARG") || kw(&L->cur, "ARGVLAST") ||
      kw(&L->cur, "ARGLAST") || kw(&L->cur, "LASTARGV") || kw(&L->cur, "PEELLAST")) {
    char op[24], field[CUBALC_HOST_STR_MAX], fb[512], envn[32];
    const char *val;
    int argc, have_fb = 0, want_last = 0;
    long idx = 0, hit = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    for (char *q = op; *q; q++)
      if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    want_last = (strcmp(op, "LASTARG") == 0 || strcmp(op, "TAILARG") == 0 ||
                 strcmp(op, "ARGVLAST") == 0 || strcmp(op, "ARGLAST") == 0 ||
                 strcmp(op, "LASTARGV") == 0 || strcmp(op, "PEELLAST") == 0);
    lex_next(L);
    field[0] = 0;
    fb[0] = 0;
    if (kw(&L->cur, "OR") || kw(&L->cur, "DEFAULT") || kw(&L->cur, "ELSE") ||
        kw(&L->cur, "FALLBACK")) {
      lex_next(L);
      if (resolve_str_arg(vm, L, fb, sizeof fb) != 0) {
        fail_at(vm, L, want_last ? "LASTARG OR \"fallback\""
                                 : "FIRSTARG OR \"fallback\"");
        return -1;
      }
      have_fb = 1;
    }
    argc = endarg_argc();
    if (argc > 0) {
      idx = want_last ? (long)(argc - 1) : 0L;
      snprintf(envn, sizeof envn, "CUBALC_ARG%ld", idx);
      val = getenv(envn);
      if (val) {
        snprintf(field, sizeof field, "%s", val);
        hit = 1;
      }
    }
    if (!hit) {
      if (have_fb)
        snprintf(field, sizeof field, "%s", fb);
      else
        field[0] = 0;
      idx = want_last ? -1L : 0L;
    }
    var_set_str(vm, "LAST", field);
    var_set_str(vm, "ARG", field);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", field);
    vm->last_n = hit ? 1L : 0L;
    var_set_num(vm, "LAST_N", hit ? 1L : 0L);
    if (want_last) {
      var_set_str(vm, "LASTARG", field);
      var_set_str(vm, "TAILARG", field);
      var_set_num(vm, "LASTARG_N", hit ? idx : -1L);
      var_set_num(vm, "LASTARG_HIT", hit ? 1L : 0L);
    } else {
      var_set_str(vm, "FIRSTARG", field);
      var_set_str(vm, "HEADARG", field);
      var_set_num(vm, "FIRSTARG_N", hit ? 0L : -1L);
      var_set_num(vm, "FIRSTARG_HIT", hit ? 1L : 0L);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s hit=%ld idx=%ld\n", want_last ? "lastarg" : "firstarg",
              hit, idx);
    bump(vm);
    return 1;
  }

  /* NTHARG|GETARG index [OR fallback] — peel CUBALC_ARGi raw (flags kept).
   * Twin of NTHPOS (non-flag only). Index not via parse_expr (OR would be boolean). */
  if (kw(&L->cur, "NTHARG") || kw(&L->cur, "GETARG") || kw(&L->cur, "ARGVAT") ||
      kw(&L->cur, "ARGAT") || kw(&L->cur, "NTHARGV") || kw(&L->cur, "PEELARG") ||
      kw(&L->cur, "GETARGV")) {
    long idx = 0, hit = 0;
    char field[CUBALC_HOST_STR_MAX], fb[512], envn[32];
    const char *val;
    int have_fb = 0;
    lex_next(L);
    field[0] = 0;
    fb[0] = 0;
    if (L->cur.kind == TK_NUM) {
      idx = L->cur.num;
      lex_next(L);
    } else if (L->cur.kind == TK_MINUS) {
      lex_next(L);
      if (L->cur.kind == TK_NUM) {
        idx = -L->cur.num;
        lex_next(L);
      } else {
        fail_at(vm, L, "NTHARG needs index — NTHARG 0 [OR \"fallback\"]");
        return -1;
      }
    } else if (L->cur.kind == TK_IDENT && !kw(&L->cur, "OR") && !kw(&L->cur, "DEFAULT") &&
               !kw(&L->cur, "ELSE") && !kw(&L->cur, "FALLBACK")) {
      if (strcmp(L->cur.text, "LAST_N") == 0)
        idx = vm->last_n;
      else {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && !vv->is_str)
          idx = vv->val;
        else if (vv && vv->is_str)
          idx = strtol(vv->sval, NULL, 10);
        else {
          fail_at(vm, L, "NTHARG needs index — NTHARG 0 [OR \"fallback\"]");
          return -1;
        }
      }
      lex_next(L);
    } else if (L->cur.kind == TK_LPAREN) {
      idx = parse_expr(vm, L);
    } else {
      fail_at(vm, L, "NTHARG needs index — NTHARG 0 [OR \"fallback\"]");
      return -1;
    }
    if (idx < 0) idx = 0;
    if (idx > 31) idx = 31;
    if (kw(&L->cur, "OR") || kw(&L->cur, "DEFAULT") || kw(&L->cur, "ELSE") ||
        kw(&L->cur, "FALLBACK")) {
      lex_next(L);
      if (resolve_str_arg(vm, L, fb, sizeof fb) != 0) {
        fail_at(vm, L, "NTHARG n OR \"fallback\"");
        return -1;
      }
      have_fb = 1;
    }
    snprintf(envn, sizeof envn, "CUBALC_ARG%ld", idx);
    val = getenv(envn);
    if (val && val[0]) {
      snprintf(field, sizeof field, "%s", val);
      hit = 1;
    } else if (val) {
      field[0] = 0;
      hit = 1;
    } else if (have_fb) {
      snprintf(field, sizeof field, "%s", fb);
      hit = 0;
    } else {
      field[0] = 0;
      hit = 0;
    }
    var_set_str(vm, "LAST", field);
    var_set_str(vm, "NTHARG", field);
    var_set_str(vm, "GETARG", field);
    var_set_str(vm, "ARG", field);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", field);
    vm->last_n = hit ? 1L : 0L;
    var_set_num(vm, "LAST_N", hit ? 1L : 0L);
    var_set_num(vm, "NTHARG_N", idx);
    var_set_num(vm, "NTHARG_HIT", hit ? 1L : 0L);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# ntharg %ld hit=%d\n", idx, hit ? 1 : 0);
    bump(vm);
    return 1;
  }
  return 0;
}
