/* CubalC — argv/flag peels rescued post-thin (endarg plane)
 * FIRSTARG/LASTARG/NTHARG · NTHPOS/ARGMAP/FINDARG/AFTERARG · FLAGMAP/LISTFLAGS/RESTARGS
 * CLI dual: cubalc firstarg|lastarg. Cube is SoT.
 */
#include "lang/cubalc_lang_ops_core_endarg.h"
#include "lang/cubalc_lang_ops_sys_cli.h"
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

  /* NTHPOS/ARGMAP/FINDARG/AFTERARG — mesh vote B_E_science follow-on */
  if (kw(&L->cur,"NTHPOS") || kw(&L->cur,"POSN") || kw(&L->cur,"POSAT") ||
      kw(&L->cur,"ARGPOS") || kw(&L->cur,"NTHREST") || kw(&L->cur,"POSITH") ||
      kw(&L->cur,"GETPOS")){
    long idx = 0, have, at = 0, hit = 0;
    char bag[CUBALC_HOST_STR_MAX], field[CUBALC_HOST_STR_MAX], fb[512];
    const char *p, *start;
    int have_fb = 0;
    lex_next(L);
    field[0] = 0;
    fb[0] = 0;
    /* do NOT use parse_expr — it swallows OR as boolean */
    if (L->cur.kind == TK_NUM) {
      idx = L->cur.num;
      lex_next(L);
    } else if (L->cur.kind == TK_MINUS) {
      lex_next(L);
      if (L->cur.kind == TK_NUM) {
        idx = -L->cur.num;
        lex_next(L);
      } else {
        fail_at(vm, L, "NTHPOS needs index — NTHPOS 0 [OR \"fallback\"]");
        return -1;
      }
    } else if (L->cur.kind == TK_IDENT && !kw(&L->cur,"OR") && !kw(&L->cur,"DEFAULT") &&
               !kw(&L->cur,"ELSE") && !kw(&L->cur,"FALLBACK")) {
      if (strcmp(L->cur.text, "LAST_N") == 0)
        idx = vm->last_n;
      else {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && !vv->is_str) idx = vv->val;
        else if (vv && vv->is_str) idx = strtol(vv->sval, NULL, 10);
        else {
          fail_at(vm, L, "NTHPOS needs index — NTHPOS 0 [OR \"fallback\"]");
          return -1;
        }
      }
      lex_next(L);
    } else if (L->cur.kind == TK_LPAREN) {
      idx = parse_expr(vm, L);
    } else {
      fail_at(vm, L, "NTHPOS needs index — NTHPOS 0 [OR \"fallback\"]");
      return -1;
    }
    if (idx < 0) idx = 0;
    if (idx > 31) idx = 31;
    if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
        kw(&L->cur,"FALLBACK")){
      lex_next(L);
      if (resolve_str_arg(vm, L, fb, sizeof fb) != 0) {
        fail_at(vm, L, "NTHPOS n OR \"fallback\"");
        return -1;
      }
      have_fb = 1;
    }
    have = cubalc_collect_restargs(bag, sizeof bag);
    p = bag;
    while (*p) {
      start = p;
      while (*p && *p != '\n') p++;
      if (at == idx) {
        size_t fl = (size_t)(p - start);
        if (fl >= sizeof field) fl = sizeof field - 1;
        memcpy(field, start, fl);
        field[fl] = 0;
        hit = 1;
        break;
      }
      at++;
      if (*p == '\n') p++;
    }
    if (!hit) {
      if (have_fb)
        snprintf(field, sizeof field, "%s", fb);
      else
        field[0] = 0;
    }
    var_set_str(vm, "LAST", field);
    var_set_str(vm, "NTHPOS", field);
    var_set_str(vm, "POS", field);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", field);
    vm->last_n = hit ? 1L : 0L;
    var_set_num(vm, "LAST_N", hit ? 1L : 0L);
    var_set_num(vm, "NTHPOS_N", idx);
    var_set_num(vm, "RESTARGS_N", have);
    var_set_num(vm, "OK", (hit || have_fb) ? 1 : 0);
    if (vm->trace)
      fprintf(vm->trace, "# nthpos %ld hit=%ld → %s\n", idx, hit, field);
    bump(vm); return 1;
  }

  if (kw(&L->cur,"ARGMAP") || kw(&L->cur,"ARGKV") || kw(&L->cur,"ARGV_KV") ||
      kw(&L->cur,"ARGVMAP") || kw(&L->cur,"CLI_ARGMAP") || kw(&L->cur,"ARGS_KV") ||
      kw(&L->cur,"KVARGS")){
    char bag[CUBALC_HOST_STR_MAX];
    long n;
    lex_next(L);
    n = cubalc_collect_argmap(bag, sizeof bag);
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "ARGMAP", bag);
    var_set_str(vm, "ARGKV", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "ARGMAP_N", n);
    var_set_num(vm, "ARGC", n);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# argmap n=%ld\n", n);
    bump(vm); return 1;
  }

  if (kw(&L->cur,"FINDARG") || kw(&L->cur,"ARGINDEX") || kw(&L->cur,"INDEXARG") ||
      kw(&L->cur,"WHICHARG") || kw(&L->cur,"ARGVINDEX") || kw(&L->cur,"ARGOF") ||
      kw(&L->cur,"FINDARGV") || kw(&L->cur,"LOCATEARG")){
    char needle[CUBALC_HOST_STR_MAX], field[CUBALC_HOST_STR_MAX], envn[32];
    const char *ac, *a;
    int argc = 32, k;
    long idx = -1, hit = 0;
    lex_next(L);
    needle[0] = 0;
    field[0] = 0;
    if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
      fail_at(vm, L, "FINDARG needs token — FINDARG \"--verbose\" | FINDARG seal");
      return -1;
    }
    ac = getenv("CUBALC_ARGC");
    if (ac && ac[0]) {
      argc = (int)strtol(ac, NULL, 10);
      if (argc < 0) argc = 0;
      if (argc > 32) argc = 32;
    } else {
      argc = 0;
      for (k = 0; k < 32; k++) {
        snprintf(envn, sizeof envn, "CUBALC_ARG%d", k);
        if (!getenv(envn)) break;
        argc++;
      }
    }
    for (k = 0; k < argc; k++) {
      snprintf(envn, sizeof envn, "CUBALC_ARG%d", k);
      a = getenv(envn);
      if (!a) a = "";
      if (strcmp(a, needle) == 0) {
        idx = (long)k;
        hit = 1;
        snprintf(field, sizeof field, "%s", a);
        break;
      }
    }
    var_set_str(vm, "LAST", field);
    var_set_str(vm, "FINDARG", field);
    var_set_str(vm, "ARGINDEX", field);
    var_set_str(vm, "FINDARG_TOKEN", needle);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", field);
    vm->last_n = hit ? idx : -1L;
    var_set_num(vm, "LAST_N", hit ? idx : -1L);
    var_set_num(vm, "FINDARG_N", hit ? idx : -1L);
    var_set_num(vm, "ARGINDEX_N", hit ? idx : -1L);
    var_set_num(vm, "FINDARG_HIT", hit ? 1L : 0L);
    var_set_num(vm, "OK", hit ? 1L : 0L);
    if (vm->trace)
      fprintf(vm->trace, "# findarg hit=%ld idx=%ld token=%s\n", hit, idx, needle);
    bump(vm);
    return 1;
  }

  if (kw(&L->cur,"AFTERARG") || kw(&L->cur,"NEXTARG") || kw(&L->cur,"ARGAFTER") ||
      kw(&L->cur,"FOLLOWARG") || kw(&L->cur,"PEELAFTER") || kw(&L->cur,"VALUEAFTER") ||
      kw(&L->cur,"ARGNEXT") || kw(&L->cur,"GETAFTER")){
    char needle[CUBALC_HOST_STR_MAX], field[CUBALC_HOST_STR_MAX], fb[512], envn[32];
    const char *ac, *a, *val;
    int argc = 32, k, have_fb = 0;
    long from = -1, next_i = -1, hit = 0;
    lex_next(L);
    needle[0] = 0;
    field[0] = 0;
    fb[0] = 0;
    if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
      fail_at(vm, L, "AFTERARG needs token — AFTERARG \"--out\" [OR \"default\"]");
      return -1;
    }
    if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
        kw(&L->cur,"FALLBACK")){
      lex_next(L);
      if (resolve_str_arg(vm, L, fb, sizeof fb) != 0) {
        fail_at(vm, L, "AFTERARG token OR \"fallback\"");
        return -1;
      }
      have_fb = 1;
    }
    ac = getenv("CUBALC_ARGC");
    if (ac && ac[0]) {
      argc = (int)strtol(ac, NULL, 10);
      if (argc < 0) argc = 0;
      if (argc > 32) argc = 32;
    } else {
      argc = 0;
      for (k = 0; k < 32; k++) {
        snprintf(envn, sizeof envn, "CUBALC_ARG%d", k);
        if (!getenv(envn)) break;
        argc++;
      }
    }
    for (k = 0; k < argc; k++) {
      snprintf(envn, sizeof envn, "CUBALC_ARG%d", k);
      a = getenv(envn);
      if (!a) a = "";
      if (strcmp(a, needle) == 0) {
        from = (long)k;
        break;
      }
    }
    if (from >= 0 && from + 1 < (long)argc) {
      next_i = from + 1;
      snprintf(envn, sizeof envn, "CUBALC_ARG%ld", next_i);
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
    }
    var_set_str(vm, "LAST", field);
    var_set_str(vm, "AFTERARG", field);
    var_set_str(vm, "NEXTARG", field);
    var_set_str(vm, "ARG", field);
    var_set_str(vm, "AFTERARG_TOKEN", needle);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", field);
    vm->last_n = hit ? 1L : 0L;
    var_set_num(vm, "LAST_N", hit ? 1L : 0L);
    var_set_num(vm, "AFTERARG_FROM", from);
    var_set_num(vm, "AFTERARG_N", hit ? next_i : -1L);
    var_set_num(vm, "AFTERARG_HIT", hit ? 1L : 0L);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# afterarg from=%ld next=%ld hit=%ld token=%s\n",
              from, next_i, hit, needle);
    bump(vm);
    return 1;
  }

  /* FLAGMAP/LISTFLAGS/RESTARGS — CLI surface twins of ARGMAP/NTHPOS */
  if (kw(&L->cur,"FLAGMAP") || kw(&L->cur,"FLAGKV") || kw(&L->cur,"FLAGPAIRS") ||
      kw(&L->cur,"OPTMAP") || kw(&L->cur,"CLI_FLAGMAP") || kw(&L->cur,"FLAGS_KV") ||
      kw(&L->cur,"KVFLAGS")){
    char bag[CUBALC_HOST_STR_MAX];
    long n;
    lex_next(L);
    n = cubalc_collect_flagmap(bag, sizeof bag);
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "FLAGMAP", bag);
    var_set_str(vm, "FLAGKV", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "FLAGMAP_N", n);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# flagmap n=%ld\n", n);
    bump(vm); return 1;
  }

  if (kw(&L->cur,"LISTFLAGS") || kw(&L->cur,"FLAGS") || kw(&L->cur,"FLAGNAMES") ||
      kw(&L->cur,"OPTFLAGS") || kw(&L->cur,"CLI_FLAGS") || kw(&L->cur,"ALLFLAGS") ||
      kw(&L->cur,"FLAGLIST")){
    char bag[CUBALC_HOST_STR_MAX];
    long n;
    lex_next(L);
    n = cubalc_collect_listflags(bag, sizeof bag);
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LISTFLAGS", bag);
    var_set_str(vm, "FLAGS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "LISTFLAGS_N", n);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# listflags n=%ld\n", n);
    bump(vm); return 1;
  }

  /* RESTARGS|POSITIONALS — newline bag of non-flag CUBALC_ARGn (positionals).
   * Soft peel; REQUIRE RESTARGS hard gate lives under REQUIRE plane. */
  if (kw(&L->cur, "RESTARGS") || kw(&L->cur, "POSITIONALS") || kw(&L->cur, "POSARGS") ||
      kw(&L->cur, "NONFLAGS") || kw(&L->cur, "ARGSREST") || kw(&L->cur, "POSARG") ||
      kw(&L->cur, "REST_ARGS")) {
    char bag[CUBALC_HOST_STR_MAX];
    long n;
    lex_next(L);
    n = cubalc_collect_restargs(bag, sizeof bag);
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "RESTARGS", bag);
    var_set_str(vm, "POSITIONALS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "RESTARGS_N", n);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# restargs n=%ld\n", n);
    bump(vm);
    return 1;
  }
  return 0;
}
