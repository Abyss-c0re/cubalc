/* CubalC lang — lang_ops_core_flag.c
 * CLI flag plane: FLAGMAP…GETFLAGMS · GETFLAGALL · GETFLAGCSV
 * Path peels → lang_ops_core_flagpath.c (B_flag_split). Cube is SoT.
 */
#include "lang/cubalc_lang_ops_core_flag.h"
#include "lang/cubalc_lang_ops_core_flagpath.h"
#include "lang/cubalc_lang_ops_sys_cli.h"
#include "lang/cubalc_lang_ops_sys_time_walk.h"
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
int cubalc_lang_ops_core_try_flag(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
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

  /* RESTPATHS|POSPATHS — RESTARGS with each field ABSPATH-resolved → abs bag. */
  if (kw(&L->cur, "RESTPATHS") || kw(&L->cur, "POSPATHS") || kw(&L->cur, "ABSPOS") ||
      kw(&L->cur, "RESTABSPATH") || kw(&L->cur, "FILEPATHS") || kw(&L->cur, "POSPATHBAG") ||
      kw(&L->cur, "ABSREST") || kw(&L->cur, "REST_PATHS") || kw(&L->cur, "PATHARGS")) {
    char raw[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
    char field[CUBALC_HOST_STR_MAX], absbuf[CUBALC_HOST_STR_MAX];
    const char *p, *start;
    long n = 0, exist_n = 0;
    size_t o = 0;
    cubalc_host_result hr;
    lex_next(L);
    n = cubalc_collect_restargs(raw, sizeof raw);
    out[0] = 0;
    p = raw;
    while (*p) {
      size_t fl, room;
      start = p;
      while (*p && *p != '\n') p++;
      fl = (size_t)(p - start);
      if (fl >= sizeof field) fl = sizeof field - 1;
      memcpy(field, start, fl);
      field[fl] = 0;
      absbuf[0] = 0;
      if (field[0]) {
        memset(&hr, 0, sizeof hr);
        if (cubalc_host_abspath(field, &hr) == 0 && hr.str[0])
          snprintf(absbuf, sizeof absbuf, "%s", hr.str);
        else
          snprintf(absbuf, sizeof absbuf, "%s", field);
        if (cubalc_host_exists(absbuf) || cubalc_host_exists(field))
          exist_n++;
      }
      if (absbuf[0] && o + 1 < sizeof out) {
        if (o > 0)
          out[o++] = '\n';
        room = sizeof out - o - 1;
        fl = strlen(absbuf);
        if (fl > room) fl = room;
        if (fl > 0) {
          memcpy(out + o, absbuf, fl);
          o += fl;
        }
        out[o] = 0;
      }
      if (*p == '\n') p++;
    }
    var_set_str(vm, "LAST", out);
    var_set_str(vm, "RESTPATHS", out);
    var_set_str(vm, "POSPATHS", out);
    var_set_str(vm, "FILEPATHS", out);
    var_set_str(vm, "RESTARGS", raw);
    var_set_str(vm, "POSITIONALS", raw);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "RESTPATHS_N", n);
    var_set_num(vm, "POSPATHS_N", n);
    var_set_num(vm, "RESTARGS_N", n);
    var_set_num(vm, "RESTPATHS_EXIST_N", exist_n);
    var_set_num(vm, "PATH_EXIST_N", exist_n);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# restpaths n=%ld exist=%ld\n", n, exist_n);
    bump(vm);
    return 1;
  }

  /* NTHPOSPATH|POSNPATH index [OR fallback] — NTHPOS then ABSPATH. */
  if (kw(&L->cur, "NTHPOSPATH") || kw(&L->cur, "POSNPATH") || kw(&L->cur, "POSPATH") ||
      kw(&L->cur, "NTHPATH") || kw(&L->cur, "ARGPATH") || kw(&L->cur, "FILEPOS") ||
      kw(&L->cur, "PATHPOS") || kw(&L->cur, "NTHRESTPATH") || kw(&L->cur, "GETPOSPATH")) {
    long idx = 0, have, at = 0, hit = 0, exist = 0;
    char bag[CUBALC_HOST_STR_MAX], field[CUBALC_HOST_STR_MAX], fb[512];
    char absbuf[CUBALC_HOST_STR_MAX], raw[CUBALC_HOST_STR_MAX];
    const char *p, *start;
    int have_fb = 0;
    cubalc_host_result hr;
    lex_next(L);
    field[0] = 0;
    fb[0] = 0;
    absbuf[0] = 0;
    raw[0] = 0;
    if (L->cur.kind == TK_NUM) {
      idx = L->cur.num;
      lex_next(L);
    } else if (L->cur.kind == TK_MINUS) {
      lex_next(L);
      if (L->cur.kind == TK_NUM) {
        idx = -L->cur.num;
        lex_next(L);
      } else {
        fail_at(vm, L, "NTHPOSPATH needs index — NTHPOSPATH 0 [OR \"fallback\"]");
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
          fail_at(vm, L, "NTHPOSPATH needs index — NTHPOSPATH 0 [OR \"fallback\"]");
          return -1;
        }
      }
      lex_next(L);
    } else {
      fail_at(vm, L, "NTHPOSPATH needs index — NTHPOSPATH 0 [OR \"fallback\"]");
      return -1;
    }
    if (idx < 0) idx = 0;
    if (idx > 31) idx = 31;
    if (kw(&L->cur, "OR") || kw(&L->cur, "DEFAULT") || kw(&L->cur, "ELSE") ||
        kw(&L->cur, "FALLBACK")) {
      lex_next(L);
      if (resolve_str_arg(vm, L, fb, sizeof fb) != 0) {
        fail_at(vm, L, "NTHPOSPATH n OR \"fallback\"");
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
    snprintf(raw, sizeof raw, "%s", field);
    if (field[0]) {
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_abspath(field, &hr) == 0 && hr.str[0])
        snprintf(absbuf, sizeof absbuf, "%s", hr.str);
      else
        snprintf(absbuf, sizeof absbuf, "%s", field);
      exist = (cubalc_host_exists(absbuf) || cubalc_host_exists(field)) ? 1 : 0;
    } else {
      exist = 0;
      absbuf[0] = 0;
    }
    var_set_str(vm, "LAST", absbuf);
    var_set_str(vm, "NTHPOSPATH", absbuf);
    var_set_str(vm, "POSPATH", absbuf);
    var_set_str(vm, "PATH", absbuf);
    var_set_str(vm, "NTHPOS", raw);
    var_set_str(vm, "POS", raw);
    var_set_str(vm, "NTHPOSPATH_RAW", raw);
    var_set_str(vm, "POSPATH_RAW", raw);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absbuf);
    vm->last_n = (hit || (have_fb && field[0])) ? 1L : 0L;
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "NTHPOSPATH_N", idx);
    var_set_num(vm, "NTHPOS_N", idx);
    var_set_num(vm, "NTHPOSPATH_HIT", hit ? 1L : 0L);
    var_set_num(vm, "NTHPOSPATH_EXIST", (long)exist);
    var_set_num(vm, "PATH_EXIST", (long)exist);
    var_set_num(vm, "RESTARGS_N", have);
    var_set_num(vm, "OK", (hit || (have_fb && field[0])) ? 1 : 0);
    if (vm->trace)
      fprintf(vm->trace, "# nthpospath %ld hit=%ld exist=%ld → %s\n", idx, hit, (long)exist,
              absbuf);
    bump(vm);
    return 1;
  }

  /* HASFLAG/GETFLAG — finish 986/981 CLI surface */
  if (kw(&L->cur,"HASFLAG") || kw(&L->cur,"HASOPT") || kw(&L->cur,"FLAG?") ||
      kw(&L->cur,"HAS_FLAG") || kw(&L->cur,"OPTHAS") || kw(&L->cur,"DEFINEDFLAG")){
    char name[96], aliases[384], hitnm[96], val[CUBALC_HOST_STR_MAX];
    int hit, nac;
    char buf[8];
    lex_next(L);
    name[0] = 0;
    aliases[0] = 0;
    hitnm[0] = 0;
    val[0] = 0;
    nac = cubalc_read_flag_aliases(L, aliases, sizeof aliases, name, sizeof name);
    if (nac <= 0 || !name[0]) {
      fail_at(vm, L, "HASFLAG needs name — HASFLAG verbose|v");
      return -1;
    }
    hit = cubalc_scan_cli_flag_any(aliases, val, sizeof val, hitnm, sizeof hitnm);
    var_set_num(vm, "LAST_N", hit ? 1L : 0L);
    vm->last_n = hit ? 1L : 0L;
    var_set_num(vm, "HASFLAG_N", hit ? 1L : 0L);
    var_set_num(vm, "OK", 1);
    var_set_str(vm, "HASFLAG", name);
    var_set_str(vm, "FLAG_HIT_NAME", hit ? hitnm : "");
    var_set_str(vm, "FLAG_ALIAS", hit ? hitnm : "");
    if (hit && val[0]) {
      var_set_str(vm, "FLAG", val);
      var_set_str(vm, "FLAG_VAL", val);
    }
    snprintf(buf, sizeof buf, "%ld", hit ? 1L : 0L);
    var_set_str(vm, "LAST", buf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
    if (vm->trace)
      fprintf(vm->trace, "# hasflag %s hit=%s → %d\n", name, hitnm, hit ? 1 : 0);
    bump(vm); return 1;
  }

  if (kw(&L->cur,"GETFLAG") || kw(&L->cur,"GETOPT") || kw(&L->cur,"FLAG") ||
      kw(&L->cur,"OPTARG") || kw(&L->cur,"OPT_GET") || kw(&L->cur,"FLAGVAL")){
    char name[96], aliases[384], hitnm[96], val[CUBALC_HOST_STR_MAX], fb[512];
    char env_used[96], src_tag[16];
    int hit, have_fb = 0, nac, from_env = 0;
    lex_next(L);
    name[0] = 0;
    aliases[0] = 0;
    hitnm[0] = 0;
    val[0] = 0;
    fb[0] = 0;
    env_used[0] = 0;
    snprintf(src_tag, sizeof src_tag, "%s", "none");
    nac = cubalc_read_flag_aliases(L, aliases, sizeof aliases, name, sizeof name);
    if (nac <= 0 || !name[0]) {
      fail_at(vm, L, "GETFLAG needs name — GETFLAG out|o OR ENV OUT OR \"a.txt\"");
      return -1;
    }
    while (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
           kw(&L->cur,"FALLBACK")){
      lex_next(L);
      if (kw(&L->cur,"ENV") || kw(&L->cur,"FROMENV") || kw(&L->cur,"ENVOR") ||
          kw(&L->cur,"GETENV") || kw(&L->cur,"ENV_GET")){
        char en[96];
        const char *ev;
        lex_next(L);
        en[0] = 0;
        while (L->cur.kind == TK_MINUS) lex_next(L);
        if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          snprintf(en, sizeof en, "%s", L->cur.text);
          lex_next(L);
        } else {
          fail_at(vm, L, "GETFLAG OR ENV NAME — GETFLAG out OR ENV OUT OR \"-\"");
          return -1;
        }
        if (!have_fb && en[0]) {
          ev = getenv(en);
          if (ev && ev[0]) {
            snprintf(fb, sizeof fb, "%s", ev);
            have_fb = 1;
            from_env = 1;
            snprintf(env_used, sizeof env_used, "%s", en);
          }
        }
        continue;
      }
      {
        char lit[512];
        lit[0] = 0;
        if (resolve_str_arg(vm, L, lit, sizeof lit) != 0) {
          fail_at(vm, L, "GETFLAG name OR \"fallback\"");
          return -1;
        }
        if (!have_fb) {
          snprintf(fb, sizeof fb, "%s", lit);
          have_fb = 1;
          from_env = 0;
        }
      }
      break;
    }
    hit = cubalc_scan_cli_flag_any(aliases, val, sizeof val, hitnm, sizeof hitnm);
    if (!hit) {
      if (have_fb)
        snprintf(val, sizeof val, "%s", fb);
      else
        val[0] = 0;
    }
    if (hit)
      snprintf(src_tag, sizeof src_tag, "%s", "cli");
    else if (have_fb && from_env)
      snprintf(src_tag, sizeof src_tag, "%s", "env");
    else if (have_fb)
      snprintf(src_tag, sizeof src_tag, "%s", "default");
    else
      snprintf(src_tag, sizeof src_tag, "%s", "none");
    var_set_str(vm, "LAST", val);
    var_set_str(vm, "FLAG", val);
    var_set_str(vm, "GETFLAG", name);
    var_set_str(vm, "FLAG_HIT_NAME", hit ? hitnm : "");
    var_set_str(vm, "FLAG_ALIAS", hit ? hitnm : "");
    var_set_str(vm, "GETFLAG_SRC", src_tag);
    var_set_str(vm, "FLAG_SRC", src_tag);
    var_set_str(vm, "FLAG_ENV", (!hit && from_env) ? env_used : "");
    snprintf(vm->last_str, sizeof vm->last_str, "%s", val);
    vm->last_n = hit ? 1L : 0L;
    var_set_num(vm, "LAST_N", hit ? 1L : 0L);
    var_set_num(vm, "GETFLAG_N", hit ? 1L : 0L);
    var_set_num(vm, "OK", (hit || (have_fb && val[0])) ? 1 : (have_fb ? 1 : 0));
    if (vm->trace)
      fprintf(vm->trace, "# getflag %s hit=%s → %s src=%s\n", name, hitnm, val, src_tag);
    bump(vm); return 1;
  }

  /* HASRESTARGS [min] — soft 0|1 if non-flag positional count >= min (default 1). */
  if (kw(&L->cur, "HASRESTARGS") || kw(&L->cur, "HASPOS") || kw(&L->cur, "HASPOSITIONALS") ||
      kw(&L->cur, "RESTARGS?") || kw(&L->cur, "HAS_RESTARGS") || kw(&L->cur, "ENOUGHPOS") ||
      kw(&L->cur, "HASFILES") || kw(&L->cur, "HAS_POS")) {
    long need = 1, have, hit;
    char bag[CUBALC_HOST_STR_MAX], buf[8];
    lex_next(L);
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
        (L->cur.kind == TK_IDENT && !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
         !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") && !kw(&L->cur, "END") &&
         !kw(&L->cur, "IF") && !kw(&L->cur, "HASARG") && !kw(&L->cur, "HASFLAG") &&
         !kw(&L->cur, "REQUIRE") && !kw(&L->cur, "RESTARGS") && !kw(&L->cur, "USAGE"))) {
      need = parse_expr(vm, L);
    }
    if (need < 0) need = 0;
    if (need > 32) need = 32;
    have = cubalc_collect_restargs(bag, sizeof bag);
    hit = (have >= need) ? 1L : 0L;
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    var_set_num(vm, "HASRESTARGS_N", hit);
    var_set_num(vm, "RESTARGS_N", have);
    var_set_num(vm, "OK", 1);
    snprintf(buf, sizeof buf, "%ld", hit);
    var_set_str(vm, "LAST", buf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
    if (have > 0)
      var_set_str(vm, "RESTARGS", bag);
    if (vm->trace)
      fprintf(vm->trace, "# hasrestargs >= %ld (have %ld) → %ld\n", need, have, hit);
    bump(vm);
    return 1;
  }

  /* HASARG n|name — soft 0|1 if CUBALC_ARGn / named env non-empty. */
  if (kw(&L->cur, "HASARG") || kw(&L->cur, "HASARGV") || kw(&L->cur, "ARG?") ||
      kw(&L->cur, "ARG_EXISTS") || kw(&L->cur, "DEFINEDARG")) {
    char key[96], envn[96], buf[8];
    const char *val;
    long hit = 0;
    lex_next(L);
    if (L->cur.kind == TK_NUM) {
      snprintf(envn, sizeof envn, "CUBALC_ARG%ld", L->cur.num);
      snprintf(key, sizeof key, "%ld", L->cur.num);
      lex_next(L);
    } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      int all_digit = 1, pi;
      snprintf(key, sizeof key, "%s", L->cur.text);
      for (pi = 0; key[pi]; pi++) {
        if (key[pi] < '0' || key[pi] > '9') {
          all_digit = 0;
          break;
        }
      }
      if (all_digit && key[0])
        snprintf(envn, sizeof envn, "CUBALC_ARG%s", key);
      else if (strncmp(key, "CUBALC_ARG", 10) == 0)
        snprintf(envn, sizeof envn, "%s", key);
      else
        snprintf(envn, sizeof envn, "%s", key);
      lex_next(L);
    } else {
      fail_at(vm, L, "HASARG needs n|name — HASARG 0");
      return -1;
    }
    val = getenv(envn);
    hit = (val && val[0]) ? 1L : 0L;
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    var_set_num(vm, "HASARG_N", hit);
    var_set_num(vm, "OK", 1);
    snprintf(buf, sizeof buf, "%ld", hit);
    var_set_str(vm, "LAST", buf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
    var_set_str(vm, "HASARG", key);
    if (hit && val)
      var_set_str(vm, "ARG", val);
    if (vm->trace)
      fprintf(vm->trace, "# hasarg %s → %ld\n", key, hit);
    bump(vm);
    return 1;
  }

  /* HASARGC [min] — soft 0|1 if CUBALC_ARGC >= min (default 1). */
  if (kw(&L->cur, "HASARGC") || kw(&L->cur, "HASARGS") || kw(&L->cur, "ARGC?") ||
      kw(&L->cur, "HAS_ARGC") || kw(&L->cur, "ENOUGHARGS")) {
    long need = 1, have = 0, hit;
    char buf[8];
    lex_next(L);
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
        (L->cur.kind == TK_IDENT && !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
         !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") && !kw(&L->cur, "END") &&
         !kw(&L->cur, "IF") && !kw(&L->cur, "HASARG") && !kw(&L->cur, "REQUIRE"))) {
      need = parse_expr(vm, L);
    }
    if (need < 0) need = 0;
    if (need > 32) need = 32;
    have = (long)endarg_argc();
    hit = (have >= need) ? 1L : 0L;
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    var_set_num(vm, "HASARGC_N", hit);
    var_set_num(vm, "ARGC", have);
    var_set_num(vm, "OK", 1);
    snprintf(buf, sizeof buf, "%ld", hit);
    var_set_str(vm, "LAST", buf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
    if (vm->trace)
      fprintf(vm->trace, "# hasargc >= %ld (have %ld) → %ld\n", need, have, hit);
    bump(vm);
    return 1;
  }

  /* GETFLAGN|FLAGN — peel CLI --name as int LAST_N. CLI > ENV > literal.
   * Bare → 1. Non-numeric → soft 0 + sticky LAST_ERR. */
  if (kw(&L->cur, "GETFLAGN") || kw(&L->cur, "FLAGN") || kw(&L->cur, "FLAGNUM") ||
      kw(&L->cur, "GETOPTN") || kw(&L->cur, "OPTN") || kw(&L->cur, "FLAG_INT") ||
      kw(&L->cur, "INTFLAG") || kw(&L->cur, "GETFLAG_N")) {
    char name[96], aliases[384], hitnm[96], val[CUBALC_HOST_STR_MAX], nbuf[32];
    char env_used[96], src_tag[16];
    int hit, have_fb = 0, bad = 0, nac, from_env = 0;
    long num = 0, fb = 0;
    lex_next(L);
    name[0] = 0;
    aliases[0] = 0;
    hitnm[0] = 0;
    val[0] = 0;
    env_used[0] = 0;
    snprintf(src_tag, sizeof src_tag, "%s", "none");
    nac = cubalc_read_flag_aliases(L, aliases, sizeof aliases, name, sizeof name);
    if (nac <= 0 || !name[0]) {
      fail_at(vm, L, "GETFLAGN needs name — GETFLAGN workers|w OR ENV WORKERS OR 4");
      return -1;
    }
    while (kw(&L->cur, "OR") || kw(&L->cur, "DEFAULT") || kw(&L->cur, "ELSE") ||
           kw(&L->cur, "FALLBACK")) {
      lex_next(L);
      if (kw(&L->cur, "ENV") || kw(&L->cur, "FROMENV") || kw(&L->cur, "ENVOR") ||
          kw(&L->cur, "GETENV") || kw(&L->cur, "ENV_GET")) {
        char en[96];
        const char *ev;
        lex_next(L);
        en[0] = 0;
        while (L->cur.kind == TK_MINUS) lex_next(L);
        if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          snprintf(en, sizeof en, "%s", L->cur.text);
          lex_next(L);
        } else {
          fail_at(vm, L, "GETFLAGN OR ENV NAME — GETFLAGN n OR ENV WORKERS OR 4");
          return -1;
        }
        if (!have_fb && en[0]) {
          ev = getenv(en);
          if (ev && ev[0]) {
            char *ep = NULL;
            fb = strtol(ev, &ep, 10);
            if (ev[0] && ep && *ep && *ep != ' ' && *ep != '\t')
              fb = 0;
            have_fb = 1;
            from_env = 1;
            snprintf(env_used, sizeof env_used, "%s", en);
            snprintf(val, sizeof val, "%s", ev);
          }
        }
        continue;
      }
      {
        long lit = 0;
        if (L->cur.kind == TK_NUM) {
          lit = L->cur.num;
          lex_next(L);
        } else if (L->cur.kind == TK_MINUS) {
          lit = parse_expr(vm, L);
        } else if (L->cur.kind == TK_STR) {
          char *ep = NULL;
          lit = strtol(L->cur.text, &ep, 10);
          lex_next(L);
        } else if (L->cur.kind == TK_IDENT) {
          Var *sv = var_get(vm, L->cur.text, 0);
          if (sv && !sv->is_str) {
            lit = sv->val;
            lex_next(L);
          } else if (sv && sv->is_str) {
            lit = strtol(sv->sval, NULL, 10);
            lex_next(L);
          } else {
            lit = parse_expr(vm, L);
          }
        } else {
          lit = parse_expr(vm, L);
        }
        if (!have_fb) {
          fb = lit;
          have_fb = 1;
          from_env = 0;
        }
      }
      break;
    }
    hit = cubalc_scan_cli_flag_any(aliases, val, sizeof val, hitnm, sizeof hitnm);
    if (hit) {
      char *ep = NULL;
      const char *p = val;
      if (p[0] == '+') p++;
      num = strtol(p, &ep, 10);
      if (p[0] == 0 || (ep && *ep != 0 && *ep != ' ' && *ep != '\t')) {
        bad = 1;
        num = 0;
        var_set_str(vm, "LAST_ERR", "GETFLAGN: non-numeric flag value");
        var_set_str(vm, "ERR", "GETFLAGN: non-numeric flag value");
      }
    } else if (have_fb) {
      num = fb;
      snprintf(val, sizeof val, "%ld", num);
    } else {
      num = 0;
      val[0] = 0;
    }
    snprintf(nbuf, sizeof nbuf, "%ld", num);
    var_set_num(vm, "LAST_N", num);
    vm->last_n = num;
    var_set_num(vm, "GETFLAGN_N", num);
    var_set_num(vm, "FLAGN", num);
    var_set_num(vm, "GETFLAGN_HIT", hit ? 1L : 0L);
    var_set_num(vm, "GETFLAGN_OK",
                (hit && !bad) || (!hit && have_fb) ? 1L
                                                   : (hit && bad ? 0L : (have_fb ? 1L : 0L)));
    var_set_num(vm, "OK", 1);
    var_set_str(vm, "GETFLAGN", name);
    var_set_str(vm, "FLAG_HIT_NAME", hit ? hitnm : "");
    var_set_str(vm, "FLAG_ALIAS", hit ? hitnm : "");
    if (hit)
      snprintf(src_tag, sizeof src_tag, "%s", "cli");
    else if (have_fb && from_env)
      snprintf(src_tag, sizeof src_tag, "%s", "env");
    else if (have_fb)
      snprintf(src_tag, sizeof src_tag, "%s", "default");
    else
      snprintf(src_tag, sizeof src_tag, "%s", "none");
    var_set_str(vm, "GETFLAGN_SRC", src_tag);
    var_set_str(vm, "FLAG_SRC", src_tag);
    var_set_str(vm, "FLAG_ENV", (!hit && from_env) ? env_used : "");
    if (hit && !bad) {
      var_set_str(vm, "LAST", nbuf);
      var_set_str(vm, "FLAG", nbuf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nbuf);
    } else if (hit && bad) {
      var_set_str(vm, "LAST", val);
      var_set_str(vm, "FLAG", val);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", val);
    } else if (have_fb) {
      var_set_str(vm, "LAST", nbuf);
      var_set_str(vm, "FLAG", nbuf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nbuf);
    } else {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "FLAG", "");
      vm->last_str[0] = 0;
    }
    if (vm->trace)
      fprintf(vm->trace, "# getflagn %s hit=%s num=%ld bad=%d src=%s\n", name, hitnm, num,
              bad, src_tag);
    bump(vm);
    return 1;
  }

  /* GETFLAGMS|FLAGMS|FLAGDUR — peel duration CLI flag → ms LAST_N. */
  if (kw(&L->cur, "GETFLAGMS") || kw(&L->cur, "FLAGMS") || kw(&L->cur, "FLAGDUR") ||
      kw(&L->cur, "GETOPTMS") || kw(&L->cur, "TIMEOUTFLAG") || kw(&L->cur, "DURFLAG") ||
      kw(&L->cur, "FLAG_MS") || kw(&L->cur, "GETFLAG_MS")) {
    char name[96], aliases[384], hitnm[96], val[CUBALC_HOST_STR_MAX], nbuf[32], fb[512];
    char env_used[96], src_tag[16];
    int hit, have_fb = 0, bad = 0, nac, from_env = 0;
    long ms = 0, fb_ms = 0;
    const char *derr = NULL;
    lex_next(L);
    name[0] = 0;
    aliases[0] = 0;
    hitnm[0] = 0;
    val[0] = 0;
    fb[0] = 0;
    env_used[0] = 0;
    snprintf(src_tag, sizeof src_tag, "%s", "none");
    nac = cubalc_read_flag_aliases(L, aliases, sizeof aliases, name, sizeof name);
    if (nac <= 0 || !name[0]) {
      fail_at(vm, L, "GETFLAGMS needs name — GETFLAGMS timeout|t OR ENV TIMEOUT OR \"30s\"");
      return -1;
    }
    while (kw(&L->cur, "OR") || kw(&L->cur, "DEFAULT") || kw(&L->cur, "ELSE") ||
           kw(&L->cur, "FALLBACK")) {
      lex_next(L);
      if (kw(&L->cur, "ENV") || kw(&L->cur, "FROMENV") || kw(&L->cur, "ENVOR") ||
          kw(&L->cur, "GETENV") || kw(&L->cur, "ENV_GET")) {
        char en[96];
        const char *ev;
        lex_next(L);
        en[0] = 0;
        while (L->cur.kind == TK_MINUS) lex_next(L);
        if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          snprintf(en, sizeof en, "%s", L->cur.text);
          lex_next(L);
        } else {
          fail_at(vm, L, "GETFLAGMS OR ENV NAME");
          return -1;
        }
        if (!have_fb && en[0]) {
          ev = getenv(en);
          if (ev && ev[0]) {
            if (cubalc_parse_duration_ms(ev, &fb_ms, NULL, &derr) != 0) {
              char *ep = NULL;
              fb_ms = strtol(ev, &ep, 10);
              if (!ev[0] || (ep && *ep && *ep != ' ' && *ep != '\t')) fb_ms = 0;
            }
            have_fb = 1;
            from_env = 1;
            snprintf(env_used, sizeof env_used, "%s", en);
            snprintf(fb, sizeof fb, "%s", ev);
          }
        }
        continue;
      }
      {
        long lit = 0;
        char lfb[512];
        lfb[0] = 0;
        if (L->cur.kind == TK_NUM) {
          lit = L->cur.num;
          lex_next(L);
        } else if (L->cur.kind == TK_MINUS) {
          lit = parse_expr(vm, L);
        } else if (L->cur.kind == TK_STR) {
          snprintf(lfb, sizeof lfb, "%s", L->cur.text);
          lex_next(L);
          if (cubalc_parse_duration_ms(lfb, &lit, NULL, &derr) != 0) {
            char *ep = NULL;
            lit = strtol(lfb, &ep, 10);
            if (!lfb[0] || (ep && *ep)) lit = 0;
          }
        } else if (L->cur.kind == TK_IDENT) {
          Var *sv = var_get(vm, L->cur.text, 0);
          if (sv && !sv->is_str) {
            lit = sv->val;
            lex_next(L);
          } else if (sv && sv->is_str) {
            if (cubalc_parse_duration_ms(sv->sval, &lit, NULL, &derr) != 0)
              lit = strtol(sv->sval, NULL, 10);
            lex_next(L);
          } else {
            snprintf(lfb, sizeof lfb, "%s", L->cur.text);
            lex_next(L);
            if (cubalc_parse_duration_ms(lfb, &lit, NULL, &derr) != 0)
              lit = strtol(lfb, NULL, 10);
          }
        } else {
          lit = parse_expr(vm, L);
        }
        if (!have_fb) {
          fb_ms = lit;
          have_fb = 1;
          from_env = 0;
        }
      }
      break;
    }
    hit = cubalc_scan_cli_flag_any(aliases, val, sizeof val, hitnm, sizeof hitnm);
    if (hit) {
      if (cubalc_parse_duration_ms(val, &ms, NULL, &derr) != 0) {
        char *ep = NULL;
        const char *p = val;
        if (p[0] == '+') p++;
        ms = strtol(p, &ep, 10);
        if (p[0] == 0 || (ep && *ep != 0 && *ep != ' ' && *ep != '\t')) {
          bad = 1;
          ms = 0;
          var_set_str(vm, "LAST_ERR", derr ? derr : "GETFLAGMS: bad duration");
          var_set_str(vm, "ERR", derr ? derr : "GETFLAGMS: bad duration");
        }
      }
    } else if (have_fb) {
      ms = fb_ms;
      snprintf(val, sizeof val, "%ld", ms);
    } else {
      ms = 0;
      val[0] = 0;
    }
    snprintf(nbuf, sizeof nbuf, "%ld", ms);
    var_set_num(vm, "LAST_N", ms);
    vm->last_n = ms;
    var_set_num(vm, "GETFLAGMS_N", ms);
    var_set_num(vm, "FLAGMS", ms);
    var_set_num(vm, "GETFLAGMS_HIT", hit ? 1L : 0L);
    var_set_num(vm, "GETFLAGMS_OK",
                (hit && !bad) || (!hit && have_fb) ? 1L
                                                   : (hit && bad ? 0L : (have_fb ? 1L : 0L)));
    var_set_num(vm, "OK", 1);
    var_set_str(vm, "GETFLAGMS", name);
    var_set_str(vm, "FLAG_HIT_NAME", hit ? hitnm : "");
    var_set_str(vm, "FLAG_ALIAS", hit ? hitnm : "");
    if (hit)
      snprintf(src_tag, sizeof src_tag, "%s", "cli");
    else if (have_fb && from_env)
      snprintf(src_tag, sizeof src_tag, "%s", "env");
    else if (have_fb)
      snprintf(src_tag, sizeof src_tag, "%s", "default");
    else
      snprintf(src_tag, sizeof src_tag, "%s", "none");
    var_set_str(vm, "GETFLAGMS_SRC", src_tag);
    var_set_str(vm, "FLAG_SRC", src_tag);
    var_set_str(vm, "FLAG_ENV", (!hit && from_env) ? env_used : "");
    if (hit && !bad) {
      var_set_str(vm, "LAST", nbuf);
      var_set_str(vm, "FLAG", nbuf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nbuf);
    } else if (!hit && have_fb) {
      var_set_str(vm, "LAST", nbuf);
      var_set_str(vm, "FLAG", nbuf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nbuf);
    } else if (hit && bad) {
      var_set_str(vm, "LAST", val);
      var_set_str(vm, "FLAG", val);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", val);
    } else {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "FLAG", "");
      vm->last_str[0] = 0;
    }
    if (vm->trace)
      fprintf(vm->trace, "# getflagms %s hit=%s ms=%ld bad=%d src=%s\n", name, hitnm, ms, bad,
              src_tag);
    bump(vm);
    return 1;
  }

  /* GETFLAGALL|FLAGALL|MULTIFLAG — collect every CLI --name into newline bag. */
  if (kw(&L->cur, "GETFLAGALL") || kw(&L->cur, "FLAGALL") || kw(&L->cur, "MULTIFLAG") ||
      kw(&L->cur, "GETFLAGS") || kw(&L->cur, "COLLECTFLAG") || kw(&L->cur, "GETFLAG_ALL")) {
    char name[96], aliases[384], hitnm[96], bag[CUBALC_HOST_STR_MAX], fb[CUBALC_HOST_STR_MAX];
    int cli_n, count, have_fb = 0, nac;
    lex_next(L);
    name[0] = 0;
    aliases[0] = 0;
    hitnm[0] = 0;
    bag[0] = 0;
    fb[0] = 0;
    nac = cubalc_read_flag_aliases(L, aliases, sizeof aliases, name, sizeof name);
    if (nac <= 0 || !name[0]) {
      fail_at(vm, L, "GETFLAGALL needs name — GETFLAGALL tags|t");
      return -1;
    }
    if (kw(&L->cur, "OR") || kw(&L->cur, "DEFAULT") || kw(&L->cur, "ELSE") ||
        kw(&L->cur, "FALLBACK")) {
      lex_next(L);
      if (resolve_str_arg(vm, L, fb, sizeof fb) != 0) {
        fail_at(vm, L, "GETFLAGALL name OR \"fallback\"");
        return -1;
      }
      have_fb = 1;
    }
    cli_n = cubalc_scan_cli_flag_all(aliases, bag, sizeof bag, hitnm, sizeof hitnm);
    count = cli_n;
    if (cli_n <= 0) {
      if (have_fb) {
        snprintf(bag, sizeof bag, "%s", fb);
        if (fb[0]) {
          const char *s = fb;
          count = 1;
          while (*s) {
            if (*s == '\n') count++;
            s++;
          }
        } else
          count = 0;
      } else {
        bag[0] = 0;
        count = 0;
      }
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "FLAG", bag);
    var_set_str(vm, "GETFLAGALL", name);
    var_set_str(vm, "FLAGALL", bag);
    var_set_str(vm, "FLAG_HIT_NAME", cli_n > 0 ? hitnm : "");
    var_set_str(vm, "FLAG_ALIAS", cli_n > 0 ? hitnm : "");
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = (long)count;
    var_set_num(vm, "LAST_N", (long)count);
    var_set_num(vm, "GETFLAGALL_N", (long)count);
    var_set_num(vm, "FLAGALL_N", (long)count);
    var_set_num(vm, "GETFLAGALL_HIT", cli_n > 0 ? 1L : 0L);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# getflagall %s cli=%d count=%d hit=%s\n", name, cli_n, count,
              hitnm[0] ? hitnm : "");
    bump(vm);
    return 1;
  }

  /* GETFLAGCSV|FLAGCSV — multi --name + comma-split → flat newline bag. */
  if (kw(&L->cur, "GETFLAGCSV") || kw(&L->cur, "FLAGCSV") || kw(&L->cur, "CSVFLAG") ||
      kw(&L->cur, "GETFLAG_CSV") || kw(&L->cur, "SPLITFLAG") || kw(&L->cur, "FLAGSPLIT") ||
      kw(&L->cur, "TAGSFLAG")) {
    char name[96], aliases[384], hitnm[96], raw[CUBALC_HOST_STR_MAX];
    char bag[CUBALC_HOST_STR_MAX], fb[CUBALC_HOST_STR_MAX];
    char env_used[96], src_tag[16];
    int cli_n, count, have_fb = 0, nac, from_env = 0;
    lex_next(L);
    name[0] = 0;
    aliases[0] = 0;
    hitnm[0] = 0;
    raw[0] = 0;
    bag[0] = 0;
    fb[0] = 0;
    env_used[0] = 0;
    snprintf(src_tag, sizeof src_tag, "%s", "none");
    nac = cubalc_read_flag_aliases(L, aliases, sizeof aliases, name, sizeof name);
    if (nac <= 0 || !name[0]) {
      fail_at(vm, L, "GETFLAGCSV needs name — GETFLAGCSV tags|t OR ENV TAGS OR \"\"");
      return -1;
    }
    while (kw(&L->cur, "OR") || kw(&L->cur, "DEFAULT") || kw(&L->cur, "ELSE") ||
           kw(&L->cur, "FALLBACK")) {
      lex_next(L);
      if (kw(&L->cur, "ENV") || kw(&L->cur, "FROMENV") || kw(&L->cur, "ENVOR") ||
          kw(&L->cur, "GETENV") || kw(&L->cur, "ENV_GET")) {
        char en[96];
        const char *ev;
        lex_next(L);
        en[0] = 0;
        while (L->cur.kind == TK_MINUS) lex_next(L);
        if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          snprintf(en, sizeof en, "%s", L->cur.text);
          lex_next(L);
        } else {
          fail_at(vm, L, "GETFLAGCSV OR ENV NAME");
          return -1;
        }
        if (!have_fb && en[0]) {
          ev = getenv(en);
          if (ev && ev[0]) {
            snprintf(fb, sizeof fb, "%s", ev);
            have_fb = 1;
            from_env = 1;
            snprintf(env_used, sizeof env_used, "%s", en);
          }
        }
        continue;
      }
      {
        char lit[CUBALC_HOST_STR_MAX];
        lit[0] = 0;
        if (resolve_str_arg(vm, L, lit, sizeof lit) != 0) {
          fail_at(vm, L, "GETFLAGCSV name OR \"fallback\"");
          return -1;
        }
        if (!have_fb) {
          snprintf(fb, sizeof fb, "%s", lit);
          have_fb = 1;
          from_env = 0;
        }
      }
      break;
    }
    cli_n = cubalc_scan_cli_flag_all(aliases, raw, sizeof raw, hitnm, sizeof hitnm);
    if (cli_n > 0) {
      count = cubalc_csv_expand_bag(raw, bag, sizeof bag);
      snprintf(src_tag, sizeof src_tag, "%s", "cli");
    } else if (have_fb) {
      count = cubalc_csv_expand_bag(fb, bag, sizeof bag);
      snprintf(src_tag, sizeof src_tag, "%s", from_env ? "env" : "default");
    } else {
      bag[0] = 0;
      count = 0;
      snprintf(src_tag, sizeof src_tag, "%s", "none");
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "FLAG", bag);
    var_set_str(vm, "GETFLAGCSV", name);
    var_set_str(vm, "FLAGCSV", bag);
    var_set_str(vm, "FLAG_HIT_NAME", cli_n > 0 ? hitnm : "");
    var_set_str(vm, "FLAG_ALIAS", cli_n > 0 ? hitnm : "");
    var_set_str(vm, "GETFLAGCSV_SRC", src_tag);
    var_set_str(vm, "FLAG_SRC", src_tag);
    var_set_str(vm, "FLAG_ENV", (cli_n <= 0 && from_env) ? env_used : "");
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = (long)count;
    var_set_num(vm, "LAST_N", (long)count);
    var_set_num(vm, "GETFLAGCSV_N", (long)count);
    var_set_num(vm, "FLAGCSV_N", (long)count);
    var_set_num(vm, "GETFLAGCSV_HIT", cli_n > 0 ? 1L : 0L);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# getflagcsv %s cli=%d count=%d src=%s\n", name, cli_n, count,
              src_tag);
    bump(vm);
    return 1;
  }

  /* Path peels → lang_ops_core_flagpath.c (B_flag_split) */
  {
    int fp = cubalc_lang_ops_core_try_flagpath(vm, L);
    if (fp != 0) return fp;
  }

  return 0;
}
