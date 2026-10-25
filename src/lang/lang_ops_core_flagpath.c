/* CubalC lang — lang_ops_core_flagpath.c
 * CLI flag-path plane: GETFLAGPATH · GETFLAGDIR · GETFLAGOUT
 * Split from flag at 1.3k (B_flag_split). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_core_flagpath.h"
#include "lang/cubalc_lang_ops_sys_cli.h"
#include "cubalc_law.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <fcntl.h>
#endif

int cubalc_lang_ops_core_try_flagpath(VM *vm, Lex *L) {
  /* RESTPATHS/NTHPOSPATH path-pos peels (B_flag_split from flag). */
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


  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* GETFLAGPATH|FLAGPATH — peel CLI path flag then ABSPATH (realpath or cwd-join).
   * LAST=abs · GETFLAGPATH_EXIST 0|1 · GETFLAGPATH_SRC cli|env|default. */
  if (kw(&L->cur, "GETFLAGPATH") || kw(&L->cur, "FLAGPATH") || kw(&L->cur, "PATHFLAG") ||
      kw(&L->cur, "GETOPTPATH") || kw(&L->cur, "FILEFLAG") || kw(&L->cur, "GETFLAG_PATH") ||
      kw(&L->cur, "CONFFLAG") || kw(&L->cur, "PATHOPT")) {
    char name[96], aliases[384], hitnm[96], val[CUBALC_HOST_STR_MAX], fb[512];
    char env_used[96], src_tag[16], absbuf[CUBALC_HOST_STR_MAX];
    int hit, have_fb = 0, nac, from_env = 0, exist = 0;
    cubalc_host_result hr;
    lex_next(L);
    name[0] = 0;
    aliases[0] = 0;
    hitnm[0] = 0;
    val[0] = 0;
    fb[0] = 0;
    env_used[0] = 0;
    absbuf[0] = 0;
    snprintf(src_tag, sizeof src_tag, "%s", "none");
    nac = cubalc_read_flag_aliases(L, aliases, sizeof aliases, name, sizeof name);
    if (nac <= 0 || !name[0]) {
      fail_at(vm, L, "GETFLAGPATH needs name — GETFLAGPATH conf|c OR ENV CONFIG OR \"cfg.json\"");
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
          fail_at(vm, L, "GETFLAGPATH OR ENV NAME");
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
          fail_at(vm, L, "GETFLAGPATH name OR \"fallback\"");
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
    if (val[0]) {
      exist = cubalc_host_exists(val) ? 1 : 0;
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_abspath(val, &hr) == 0 && hr.str[0])
        snprintf(absbuf, sizeof absbuf, "%s", hr.str);
      else
        snprintf(absbuf, sizeof absbuf, "%s", val);
    } else {
      exist = 0;
      absbuf[0] = 0;
    }
    var_set_str(vm, "LAST", absbuf);
    var_set_str(vm, "FLAG", absbuf);
    var_set_str(vm, "GETFLAGPATH", name);
    var_set_str(vm, "FLAGPATH", absbuf);
    var_set_str(vm, "PATH", absbuf);
    var_set_str(vm, "GETFLAGPATH_RAW", val);
    var_set_str(vm, "FLAG_HIT_NAME", hit ? hitnm : "");
    var_set_str(vm, "FLAG_ALIAS", hit ? hitnm : "");
    var_set_str(vm, "GETFLAGPATH_SRC", src_tag);
    var_set_str(vm, "FLAG_SRC", src_tag);
    var_set_str(vm, "FLAG_ENV", (!hit && from_env) ? env_used : "");
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absbuf);
    vm->last_n = (long)strlen(absbuf);
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "GETFLAGPATH_N", vm->last_n);
    var_set_num(vm, "GETFLAGPATH_HIT", hit ? 1L : 0L);
    var_set_num(vm, "GETFLAGPATH_EXIST", (long)exist);
    var_set_num(vm, "PATH_EXIST", (long)exist);
    var_set_num(vm, "OK", (hit || (have_fb && val[0])) ? 1 : (have_fb ? 1 : 0));
    if (vm->trace)
      fprintf(vm->trace, "# getflagpath %s hit=%s exist=%d → %s src=%s\n", name, hitnm, exist,
              absbuf, src_tag);
    bump(vm);
    return 1;
  }

  /* GETFLAGDIR|MKFLAGDIR — path flag peel + ABSPATH + mkdir -p.
   * LAST=abs dir · GETFLAGDIR_SRC · GETFLAGDIR_CREATED 0|1 · soft empty OK=0. */
  if (kw(&L->cur, "GETFLAGDIR") || kw(&L->cur, "MKFLAGDIR") || kw(&L->cur, "ENSUREFLAGDIR") ||
      kw(&L->cur, "OUTDIRFLAG") || kw(&L->cur, "FLAGOUTDIR") || kw(&L->cur, "DIRFLAG") ||
      kw(&L->cur, "GETOPTDIR") || kw(&L->cur, "FLAG_OUTDIR") || kw(&L->cur, "MKDIRFLAG") ||
      kw(&L->cur, "ENSUREOUTDIR")) {
    char name[96], aliases[384], hitnm[96], val[CUBALC_HOST_STR_MAX], fb[512];
    char env_used[96], src_tag[16], absbuf[CUBALC_HOST_STR_MAX];
    int hit, have_fb = 0, nac, from_env = 0, created = 0, exist_before = 0;
    cubalc_host_result hr;
    lex_next(L);
    name[0] = 0;
    aliases[0] = 0;
    hitnm[0] = 0;
    val[0] = 0;
    fb[0] = 0;
    env_used[0] = 0;
    absbuf[0] = 0;
    snprintf(src_tag, sizeof src_tag, "%s", "none");
    nac = cubalc_read_flag_aliases(L, aliases, sizeof aliases, name, sizeof name);
    if (nac <= 0 || !name[0]) {
      fail_at(vm, L, "GETFLAGDIR needs name — GETFLAGDIR out|o OR ENV OUTDIR OR \"state/out\"");
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
          fail_at(vm, L, "GETFLAGDIR OR ENV NAME");
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
          fail_at(vm, L, "GETFLAGDIR name OR \"fallback\"");
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
    if (!val[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "FLAG", "");
      var_set_str(vm, "GETFLAGDIR", name);
      var_set_str(vm, "GETFLAGDIR_RAW", "");
      var_set_str(vm, "GETFLAGDIR_SRC", src_tag);
      var_set_str(vm, "FLAG_SRC", src_tag);
      var_set_str(vm, "FLAG_HIT_NAME", "");
      var_set_str(vm, "FLAG_ALIAS", "");
      var_set_str(vm, "FLAG_ENV", "");
      var_set_str(vm, "LAST_ERR", "GETFLAGDIR: empty path — pass --name or OR default");
      var_set_str(vm, "ERR", "GETFLAGDIR: empty path — pass --name or OR default");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "GETFLAGDIR_N", 0);
      var_set_num(vm, "GETFLAGDIR_HIT", 0);
      var_set_num(vm, "GETFLAGDIR_CREATED", 0);
      var_set_num(vm, "GETFLAGDIR_EXIST", 0);
      var_set_num(vm, "OK", 0);
      if (vm->trace)
        fprintf(vm->trace, "# getflagdir %s empty src=%s\n", name, src_tag);
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(val, &hr) == 0 && hr.str[0])
      snprintf(absbuf, sizeof absbuf, "%s", hr.str);
    else
      snprintf(absbuf, sizeof absbuf, "%s", val);
    exist_before = cubalc_host_exists(absbuf) || cubalc_host_exists(val) ? 1 : 0;
    created = exist_before ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absbuf, &hr) != 0) {
      const char *err = (hr.err[0]) ? hr.err : "GETFLAGDIR: mkdir failed";
      var_set_str(vm, "LAST", absbuf);
      var_set_str(vm, "FLAG", absbuf);
      var_set_str(vm, "GETFLAGDIR", name);
      var_set_str(vm, "GETFLAGDIR_RAW", val);
      var_set_str(vm, "GETFLAGDIR_SRC", src_tag);
      var_set_str(vm, "FLAG_SRC", src_tag);
      var_set_str(vm, "FLAG_HIT_NAME", hit ? hitnm : "");
      var_set_str(vm, "FLAG_ALIAS", hit ? hitnm : "");
      var_set_str(vm, "FLAG_ENV", (!hit && from_env) ? env_used : "");
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absbuf);
      vm->last_n = (long)strlen(absbuf);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "GETFLAGDIR_N", vm->last_n);
      var_set_num(vm, "GETFLAGDIR_HIT", hit ? 1L : 0L);
      var_set_num(vm, "GETFLAGDIR_CREATED", 0);
      var_set_num(vm, "GETFLAGDIR_EXIST", 0);
      var_set_num(vm, "OK", 0);
      if (vm->trace)
        fprintf(vm->trace, "# getflagdir %s mkdir fail → %s (%s)\n", name, absbuf, err);
      bump(vm);
      return 1;
    }
    if (hr.str[0])
      snprintf(absbuf, sizeof absbuf, "%s", hr.str);
    var_set_str(vm, "LAST", absbuf);
    var_set_str(vm, "FLAG", absbuf);
    var_set_str(vm, "GETFLAGDIR", name);
    var_set_str(vm, "FLAGDIR", absbuf);
    var_set_str(vm, "PATH", absbuf);
    var_set_str(vm, "GETFLAGDIR_RAW", val);
    var_set_str(vm, "FLAG_HIT_NAME", hit ? hitnm : "");
    var_set_str(vm, "FLAG_ALIAS", hit ? hitnm : "");
    var_set_str(vm, "GETFLAGDIR_SRC", src_tag);
    var_set_str(vm, "FLAG_SRC", src_tag);
    var_set_str(vm, "FLAG_ENV", (!hit && from_env) ? env_used : "");
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absbuf);
    vm->last_n = (long)strlen(absbuf);
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "GETFLAGDIR_N", vm->last_n);
    var_set_num(vm, "GETFLAGDIR_HIT", hit ? 1L : 0L);
    var_set_num(vm, "GETFLAGDIR_CREATED", (long)created);
    var_set_num(vm, "GETFLAGDIR_EXIST", 1L);
    var_set_num(vm, "PATH_EXIST", 1L);
    var_set_num(vm, "ISDIR", 1L);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# getflagdir %s hit=%s created=%d → %s src=%s\n", name, hitnm, created,
              absbuf, src_tag);
    bump(vm);
    return 1;
  }

  /* GETFLAGOUT|FLAGOUT — path flag peel + ABSPATH + mkdir parent for WRITE.
   * LAST=abs file · GETFLAGOUT_PARENT · CREATED/EXIST · soft empty OK=0. */
  if (kw(&L->cur, "GETFLAGOUT") || kw(&L->cur, "FLAGOUT") || kw(&L->cur, "OUTPATH") ||
      kw(&L->cur, "OUTFLAG") || kw(&L->cur, "GETOPTOUT") || kw(&L->cur, "WRITEPATH") ||
      kw(&L->cur, "OUTFILE") || kw(&L->cur, "GETFLAG_OUT") || kw(&L->cur, "OUTPUTFLAG")) {
    char name[96], aliases[384], hitnm[96], val[CUBALC_HOST_STR_MAX], fb[512];
    char env_used[96], src_tag[16], absfile[CUBALC_HOST_STR_MAX], parent[512];
    char absparent[CUBALC_HOST_STR_MAX];
    int hit, have_fb = 0, nac, from_env = 0, created = 0, exist = 0;
    const char *slash;
    size_t n;
    cubalc_host_result hr;
    lex_next(L);
    name[0] = 0;
    aliases[0] = 0;
    hitnm[0] = 0;
    val[0] = 0;
    fb[0] = 0;
    env_used[0] = 0;
    absfile[0] = 0;
    parent[0] = 0;
    absparent[0] = 0;
    snprintf(src_tag, sizeof src_tag, "%s", "none");
    nac = cubalc_read_flag_aliases(L, aliases, sizeof aliases, name, sizeof name);
    if (nac <= 0 || !name[0]) {
      fail_at(vm, L, "GETFLAGOUT needs name — GETFLAGOUT out|o OR ENV OUT OR \"state/out.json\"");
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
          fail_at(vm, L, "GETFLAGOUT OR ENV NAME");
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
          fail_at(vm, L, "GETFLAGOUT name OR \"fallback\"");
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
    if (!val[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "FLAG", "");
      var_set_str(vm, "GETFLAGOUT", name);
      var_set_str(vm, "GETFLAGOUT_RAW", "");
      var_set_str(vm, "GETFLAGOUT_PARENT", "");
      var_set_str(vm, "GETFLAGOUT_SRC", src_tag);
      var_set_str(vm, "FLAG_SRC", src_tag);
      var_set_str(vm, "FLAG_HIT_NAME", "");
      var_set_str(vm, "FLAG_ALIAS", "");
      var_set_str(vm, "FLAG_ENV", "");
      var_set_str(vm, "LAST_ERR", "GETFLAGOUT: empty path — pass --name or OR default");
      var_set_str(vm, "ERR", "GETFLAGOUT: empty path — pass --name or OR default");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "GETFLAGOUT_N", 0);
      var_set_num(vm, "GETFLAGOUT_HIT", 0);
      var_set_num(vm, "GETFLAGOUT_CREATED", 0);
      var_set_num(vm, "GETFLAGOUT_EXIST", 0);
      var_set_num(vm, "OK", 0);
      if (vm->trace)
        fprintf(vm->trace, "# getflagout %s empty src=%s\n", name, src_tag);
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(val, &hr) == 0 && hr.str[0])
      snprintf(absfile, sizeof absfile, "%s", hr.str);
    else
      snprintf(absfile, sizeof absfile, "%s", val);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absfile);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "GETFLAGOUT: mkdir parent failed";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "FLAG", absfile);
      var_set_str(vm, "GETFLAGOUT", name);
      var_set_str(vm, "GETFLAGOUT_RAW", val);
      var_set_str(vm, "GETFLAGOUT_PARENT", absparent);
      var_set_str(vm, "GETFLAGOUT_SRC", src_tag);
      var_set_str(vm, "FLAG_SRC", src_tag);
      var_set_str(vm, "FLAG_HIT_NAME", hit ? hitnm : "");
      var_set_str(vm, "FLAG_ALIAS", hit ? hitnm : "");
      var_set_str(vm, "FLAG_ENV", (!hit && from_env) ? env_used : "");
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = (long)strlen(absfile);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "GETFLAGOUT_N", vm->last_n);
      var_set_num(vm, "GETFLAGOUT_HIT", hit ? 1L : 0L);
      var_set_num(vm, "GETFLAGOUT_CREATED", 0);
      var_set_num(vm, "GETFLAGOUT_EXIST", 0);
      var_set_num(vm, "OK", 0);
      if (vm->trace)
        fprintf(vm->trace, "# getflagout %s mkdir fail parent=%s\n", name, absparent);
      bump(vm);
      return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    exist = (cubalc_host_exists(absfile) || cubalc_host_exists(val)) ? 1 : 0;
    var_set_str(vm, "LAST", absfile);
    var_set_str(vm, "FLAG", absfile);
    var_set_str(vm, "PATH", absfile);
    var_set_str(vm, "GETFLAGOUT", name);
    var_set_str(vm, "FLAGOUT", absfile);
    var_set_str(vm, "OUTPATH", absfile);
    var_set_str(vm, "GETFLAGOUT_RAW", val);
    var_set_str(vm, "GETFLAGOUT_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    var_set_str(vm, "FLAG_HIT_NAME", hit ? hitnm : "");
    var_set_str(vm, "FLAG_ALIAS", hit ? hitnm : "");
    var_set_str(vm, "GETFLAGOUT_SRC", src_tag);
    var_set_str(vm, "FLAG_SRC", src_tag);
    var_set_str(vm, "FLAG_ENV", (!hit && from_env) ? env_used : "");
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
    vm->last_n = (long)strlen(absfile);
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "GETFLAGOUT_N", vm->last_n);
    var_set_num(vm, "GETFLAGOUT_HIT", hit ? 1L : 0L);
    var_set_num(vm, "GETFLAGOUT_CREATED", (long)created);
    var_set_num(vm, "GETFLAGOUT_EXIST", (long)exist);
    var_set_num(vm, "PATH_EXIST", (long)exist);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# getflagout %s hit=%s created=%d exist=%d → %s parent=%s src=%s\n",
              name, hitnm, created, exist, absfile, absparent, src_tag);
    bump(vm);
    return 1;
  }

  /* ENSUREPARENT|MKPARENT — dirname + mkdir -p for file WRITE prep (top-level). */
  if (kw(&L->cur, "ENSUREPARENT") || kw(&L->cur, "MKPARENT") || kw(&L->cur, "PARENTMKDIR") ||
      kw(&L->cur, "MKDIRPARENT") || kw(&L->cur, "ENSURE_PARENT") || kw(&L->cur, "PARENTDIRMK") ||
      kw(&L->cur, "MAKEDIRPARENT") || kw(&L->cur, "ENSUREDIRPARENT")) {
    char path[512], parent[512], absbuf[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0;
    cubalc_host_result hr;
    lex_next(L);
    path[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
      fail(vm, "ENSUREPARENT \"path\"|LAST");
      return -1;
    }
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "ENSUREPARENT: empty path");
      var_set_str(vm, "ERR", "ENSUREPARENT: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "ENSUREPARENT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    n = strlen(path);
    while (n > 1 && (path[n - 1] == '/' || path[n - 1] == '\\')) {
      path[n - 1] = 0;
      n--;
    }
    slash = cubalc_path_slash(path);
    if (slash && slash != path) {
      size_t dn = (size_t)(slash - path);
      if (dn >= sizeof parent) dn = sizeof parent - 1;
      memcpy(parent, path, dn);
      parent[dn] = 0;
    } else if (slash && slash == path) {
      snprintf(parent, sizeof parent, "%c", path[0] == '\\' ? '\\' : '/');
    } else {
      snprintf(parent, sizeof parent, ".");
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absbuf, sizeof absbuf, "%s", hr.str);
    else
      snprintf(absbuf, sizeof absbuf, "%s", parent);
    created = (cubalc_host_exists(absbuf) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absbuf, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "ENSUREPARENT: mkdir failed";
      var_set_str(vm, "LAST", absbuf);
      var_set_str(vm, "DIRNAME", absbuf);
      var_set_str(vm, "PARENT", absbuf);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absbuf);
      vm->last_n = (long)strlen(absbuf);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "ENSUREPARENT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    if (hr.str[0])
      snprintf(absbuf, sizeof absbuf, "%s", hr.str);
    var_set_str(vm, "LAST", absbuf);
    var_set_str(vm, "DIRNAME", absbuf);
    var_set_str(vm, "PARENT", absbuf);
    var_set_str(vm, "ENSUREPARENT", absbuf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absbuf);
    vm->last_n = (long)strlen(absbuf);
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "MKDIR_N", hr.n);
    var_set_num(vm, "ISDIR", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# ensureparent → %s created=%d\n", absbuf, created);
    bump(vm);
    return 1;
  }

  /* WRITEOUT|PUTOUT|SAVEOUT path data — ENSUREPARENT + WRITE one-shot.
   * LAST = absolute file path · WRITEOUT_PARENT · WRITEOUT_CREATED · LAST_N=bytes.
   * Soft empty path / mkdir / write fail → OK=0 sticky LAST_ERR.
   * Usability: plate file write without ENSUREPARENT + SYS WRITE glue. */
  if (kw(&L->cur, "WRITEOUT") || kw(&L->cur, "PUTOUT") || kw(&L->cur, "SAVEOUT") ||
      kw(&L->cur, "ENSUREWRITE") || kw(&L->cur, "WRITEPARENT") || kw(&L->cur, "PLATEWRITE") ||
      kw(&L->cur, "SAVEPATH") || kw(&L->cur, "PUTFILE") || kw(&L->cur, "DUMPOUT")) {
    char path[CUBALC_HOST_STR_MAX], data[CUBALC_HOST_STR_MAX];
    char absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0;
    cubalc_host_result hr;
    lex_next(L);
    path[0] = 0;
    data[0] = 0;
    absfile[0] = 0;
    parent[0] = 0;
    absparent[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
      fail_at(vm, L, "WRITEOUT needs path — WRITEOUT \"a/b/c.json\" \"data\"|var");
      return -1;
    }
    if (resolve_str_arg(vm, L, data, sizeof data) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(data, sizeof data, "%ld", nv);
      } else {
        data[0] = 0; /* empty body allowed */
      }
    }
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "WRITEOUT: empty path");
      var_set_str(vm, "ERR", "WRITEOUT: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEOUT_N", 0);
      var_set_num(vm, "WRITEOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(path, &hr) == 0 && hr.str[0])
      snprintf(absfile, sizeof absfile, "%s", hr.str);
    else
      snprintf(absfile, sizeof absfile, "%s", path);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absfile);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "WRITEOUT: mkdir parent failed";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "WRITEOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEOUT_N", 0);
      var_set_num(vm, "WRITEOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_write(absfile, data, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "WRITEOUT: write failed";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "WRITEOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEOUT_N", 0);
      var_set_num(vm, "WRITEOUT_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", absfile);
    var_set_str(vm, "WRITEOUT", absfile);
    var_set_str(vm, "PATH", absfile);
    var_set_str(vm, "WRITEOUT_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "WRITEOUT_N", hr.n);
    var_set_num(vm, "WRITEOUT_CREATED", (long)created);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# writeout → %s bytes=%ld created=%d\n",
              absfile, hr.n, created);
    bump(vm);
    return 1;
  }

  /* LINKOUT|LNOUT target linkpath — ENSUREPARENT link + SYMLINK one-shot.
   * LAST = abs link · LINKOUT_TARGET · LINKOUT_PARENT · CREATED · LAST_N=1.
   * Soft empty/mkdir/symlink fail → OK=0 sticky LAST_ERR.
   * Usability: nested plate aliases without ENSUREPARENT + SYS SYMLINK glue. */
  if (kw(&L->cur, "LINKOUT") || kw(&L->cur, "LNOUT") || kw(&L->cur, "ENSURELINK") ||
      kw(&L->cur, "LINKPARENT") || kw(&L->cur, "SYMLINKOUT") || kw(&L->cur, "ALIASOUT") ||
      kw(&L->cur, "LINKNEST") || kw(&L->cur, "PLATELINK") || kw(&L->cur, "SOFTLINKOUT")) {
    char target[CUBALC_HOST_STR_MAX], linkpath[CUBALC_HOST_STR_MAX];
    char abslink[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0;
    cubalc_host_result hr;
    lex_next(L);
    target[0] = 0;
    linkpath[0] = 0;
    abslink[0] = 0;
    parent[0] = 0;
    absparent[0] = 0;
    if (resolve_str_arg(vm, L, target, sizeof target) != 0) {
      fail_at(vm, L, "LINKOUT needs target linkpath — LINKOUT \"../plate.json\" \"a/b/alias.json\"");
      return -1;
    }
    if (resolve_str_arg(vm, L, linkpath, sizeof linkpath) != 0) {
      fail_at(vm, L, "LINKOUT needs linkpath — LINKOUT \"../plate.json\" \"a/b/alias.json\"");
      return -1;
    }
    if (!target[0] || !linkpath[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "LINKOUT: empty path");
      var_set_str(vm, "ERR", "LINKOUT: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "LINKOUT_N", 0);
      var_set_num(vm, "LINKOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(linkpath, &hr) == 0 && hr.str[0])
      snprintf(abslink, sizeof abslink, "%s", hr.str);
    else
      snprintf(abslink, sizeof abslink, "%s", linkpath);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", abslink);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "LINKOUT: mkdir parent failed";
      var_set_str(vm, "LAST", abslink);
      var_set_str(vm, "LINKOUT_TARGET", target);
      var_set_str(vm, "LINKOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", abslink);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "LINKOUT_N", 0);
      var_set_num(vm, "LINKOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_symlink(target, abslink, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "LINKOUT: symlink failed";
      var_set_str(vm, "LAST", abslink);
      var_set_str(vm, "LINKOUT_TARGET", target);
      var_set_str(vm, "LINKOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", abslink);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "LINKOUT_N", 0);
      var_set_num(vm, "LINKOUT_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", abslink);
    var_set_str(vm, "LINKOUT", abslink);
    var_set_str(vm, "LNOUT", abslink);
    var_set_str(vm, "PATH", abslink);
    var_set_str(vm, "SYMLINK", abslink);
    var_set_str(vm, "LINKOUT_TARGET", target);
    var_set_str(vm, "SYMLINK_TARGET", target);
    var_set_str(vm, "LINKOUT_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", abslink);
    vm->last_n = hr.n > 0 ? hr.n : 1;
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "LINKOUT_N", vm->last_n);
    var_set_num(vm, "SYMLINK_N", vm->last_n);
    var_set_num(vm, "LINKOUT_CREATED", (long)created);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# linkout → %s → %s created=%d\n",
              abslink, target, created);
    bump(vm);
    return 1;
  }

  /* HARDLINKOUT|HLOUT existing newpath — ENSUREPARENT new + HARDLINK one-shot.
   * LAST = abs new · HARDLINKOUT_SRC · HARDLINKOUT_PARENT · CREATED · LAST_N=1.
   * Soft empty/mkdir/link fail → OK=0 sticky LAST_ERR.
   * Usability: nested same-inode plate shares without ENSUREPARENT + HARDLINK glue. */
  if (kw(&L->cur, "HARDLINKOUT") || kw(&L->cur, "HLOUT") || kw(&L->cur, "ENSUREHARDLINK") ||
      kw(&L->cur, "HARDLINKPARENT") || kw(&L->cur, "LINKHARDOUT") || kw(&L->cur, "SHAREOUT") ||
      kw(&L->cur, "HLINKOUT") || kw(&L->cur, "INODEOUT") || kw(&L->cur, "PLATEHLINK")) {
    char existing[CUBALC_HOST_STR_MAX], newpath[CUBALC_HOST_STR_MAX];
    char abssrc[CUBALC_HOST_STR_MAX], absnew[CUBALC_HOST_STR_MAX];
    char parent[512], absparent[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0;
    cubalc_host_result hr;
    lex_next(L);
    existing[0] = 0;
    newpath[0] = 0;
    abssrc[0] = 0;
    absnew[0] = 0;
    parent[0] = 0;
    absparent[0] = 0;
    if (resolve_str_arg(vm, L, existing, sizeof existing) != 0) {
      fail_at(vm, L, "HARDLINKOUT needs existing newpath — HARDLINKOUT \"src.json\" \"a/b/share.json\"");
      return -1;
    }
    if (resolve_str_arg(vm, L, newpath, sizeof newpath) != 0) {
      fail_at(vm, L, "HARDLINKOUT needs newpath — HARDLINKOUT \"src.json\" \"a/b/share.json\"");
      return -1;
    }
    if (!existing[0] || !newpath[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "HARDLINKOUT: empty path");
      var_set_str(vm, "ERR", "HARDLINKOUT: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HARDLINKOUT_N", 0);
      var_set_num(vm, "HARDLINKOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(existing, &hr) == 0 && hr.str[0])
      snprintf(abssrc, sizeof abssrc, "%s", hr.str);
    else
      snprintf(abssrc, sizeof abssrc, "%s", existing);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(newpath, &hr) == 0 && hr.str[0])
      snprintf(absnew, sizeof absnew, "%s", hr.str);
    else
      snprintf(absnew, sizeof absnew, "%s", newpath);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absnew);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "HARDLINKOUT: mkdir parent failed";
      var_set_str(vm, "LAST", absnew);
      var_set_str(vm, "HARDLINKOUT_SRC", abssrc);
      var_set_str(vm, "HARDLINKOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absnew);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HARDLINKOUT_N", 0);
      var_set_num(vm, "HARDLINKOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_hardlink(abssrc, absnew, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "HARDLINKOUT: hardlink failed";
      var_set_str(vm, "LAST", absnew);
      var_set_str(vm, "HARDLINKOUT_SRC", abssrc);
      var_set_str(vm, "HARDLINKOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absnew);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HARDLINKOUT_N", 0);
      var_set_num(vm, "HARDLINKOUT_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", absnew);
    var_set_str(vm, "HARDLINKOUT", absnew);
    var_set_str(vm, "HLOUT", absnew);
    var_set_str(vm, "PATH", absnew);
    var_set_str(vm, "HARDLINK", absnew);
    var_set_str(vm, "HARDLINKOUT_SRC", abssrc);
    var_set_str(vm, "HARDLINK_SRC", abssrc);
    var_set_str(vm, "HARDLINKOUT_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absnew);
    vm->last_n = hr.n > 0 ? hr.n : 1;
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "HARDLINKOUT_N", vm->last_n);
    var_set_num(vm, "HARDLINK_N", vm->last_n);
    var_set_num(vm, "HLINK_N", vm->last_n);
    var_set_num(vm, "HARDLINKOUT_CREATED", (long)created);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# hardlinkout → %s ← %s created=%d\n",
              absnew, abssrc, created);
    bump(vm);
    return 1;
  }

  /* RELINKOUT|RELINK target linkpath — ENSUREPARENT; RM if exists; SYMLINK.
   * LAST = abs link · RELINKOUT_TARGET · PARENT · CREATED · REPLACED 0|1 · LAST_N=1.
   * Soft empty/mkdir/rm/symlink fail → OK=0 sticky LAST_ERR.
   * Usability: update nested plate aliases without RM+LINKOUT IF glue. */
  if (kw(&L->cur, "RELINKOUT") || kw(&L->cur, "RELINK") || kw(&L->cur, "FORCELINKOUT") ||
      kw(&L->cur, "REPLACELINK") || kw(&L->cur, "UPDATELINK") || kw(&L->cur, "RELINKPARENT") ||
      kw(&L->cur, "LINKFORCE") || kw(&L->cur, "OVERWRITELINK") || kw(&L->cur, "RELINKNEST")) {
    char target[CUBALC_HOST_STR_MAX], linkpath[CUBALC_HOST_STR_MAX];
    char abslink[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0, replaced = 0;
    cubalc_host_result hr, trash;
    lex_next(L);
    target[0] = 0;
    linkpath[0] = 0;
    abslink[0] = 0;
    parent[0] = 0;
    absparent[0] = 0;
    if (resolve_str_arg(vm, L, target, sizeof target) != 0) {
      fail_at(vm, L, "RELINKOUT needs target linkpath — RELINKOUT \"src.json\" \"a/b/alias.json\"");
      return -1;
    }
    if (resolve_str_arg(vm, L, linkpath, sizeof linkpath) != 0) {
      fail_at(vm, L, "RELINKOUT needs linkpath — RELINKOUT \"src.json\" \"a/b/alias.json\"");
      return -1;
    }
    if (!target[0] || !linkpath[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "RELINKOUT: empty path");
      var_set_str(vm, "ERR", "RELINKOUT: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "RELINKOUT_N", 0);
      var_set_num(vm, "RELINKOUT_CREATED", 0);
      var_set_num(vm, "RELINKOUT_REPLACED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    /* Never realpath(linkpath): existing symlink would resolve to TARGET and RM it. */
    if (linkpath[0] == '/' || linkpath[0] == '\\') {
      snprintf(abslink, sizeof abslink, "%s", linkpath);
    } else {
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_abspath(".", &hr) == 0 && hr.str[0])
        snprintf(abslink, sizeof abslink, "%s/%s", hr.str, linkpath);
      else
        snprintf(abslink, sizeof abslink, "%s", linkpath);
    }
    n = strlen(abslink);
    while (n > 1 && (abslink[n - 1] == '/' || abslink[n - 1] == '\\')) {
      abslink[n - 1] = 0;
      n--;
    }
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", abslink);
      n = strlen(work);
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "RELINKOUT: mkdir parent failed";
      var_set_str(vm, "LAST", abslink);
      var_set_str(vm, "RELINKOUT_TARGET", target);
      var_set_str(vm, "RELINKOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", abslink);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "RELINKOUT_N", 0);
      var_set_num(vm, "RELINKOUT_CREATED", 0);
      var_set_num(vm, "RELINKOUT_REPLACED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    {
      int present = 0;
      memset(&trash, 0, sizeof trash);
      cubalc_host_islink(abslink, &trash);
      if (trash.n == 1 || cubalc_host_exists(abslink))
        present = 1;
      if (present) {
        memset(&trash, 0, sizeof trash);
        if (unlink(abslink) != 0 && cubalc_host_rm(abslink, &trash) != 0) {
          const char *err = trash.err[0] ? trash.err : "RELINKOUT: rm existing failed";
          var_set_str(vm, "LAST", abslink);
          var_set_str(vm, "RELINKOUT_TARGET", target);
          var_set_str(vm, "RELINKOUT_PARENT", absparent);
          var_set_str(vm, "LAST_ERR", err);
          var_set_str(vm, "ERR", err);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", abslink);
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "RELINKOUT_N", 0);
          var_set_num(vm, "RELINKOUT_CREATED", (long)created);
          var_set_num(vm, "RELINKOUT_REPLACED", 0);
          var_set_num(vm, "OK", 0);
          bump(vm);
          return 1;
        }
        replaced = 1;
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_symlink(target, abslink, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "RELINKOUT: symlink failed";
      var_set_str(vm, "LAST", abslink);
      var_set_str(vm, "RELINKOUT_TARGET", target);
      var_set_str(vm, "RELINKOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", abslink);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "RELINKOUT_N", 0);
      var_set_num(vm, "RELINKOUT_CREATED", (long)created);
      var_set_num(vm, "RELINKOUT_REPLACED", (long)replaced);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", abslink);
    var_set_str(vm, "RELINKOUT", abslink);
    var_set_str(vm, "LINKOUT", abslink);
    var_set_str(vm, "PATH", abslink);
    var_set_str(vm, "SYMLINK", abslink);
    var_set_str(vm, "RELINKOUT_TARGET", target);
    var_set_str(vm, "LINKOUT_TARGET", target);
    var_set_str(vm, "SYMLINK_TARGET", target);
    var_set_str(vm, "RELINKOUT_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", abslink);
    vm->last_n = hr.n > 0 ? hr.n : 1;
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "RELINKOUT_N", vm->last_n);
    var_set_num(vm, "LINKOUT_N", vm->last_n);
    var_set_num(vm, "SYMLINK_N", vm->last_n);
    var_set_num(vm, "RELINKOUT_CREATED", (long)created);
    var_set_num(vm, "RELINKOUT_REPLACED", (long)replaced);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# relinkout → %s → %s replaced=%d created=%d\n",
              abslink, target, replaced, created);
    bump(vm);
    return 1;
  }

  /* TOUCHOUT|MKFILEOUT path — ENSUREPARENT + TOUCH one-shot.
   * LAST = abs path · TOUCHOUT_PARENT · TOUCHOUT_CREATED (parent) · TOUCHOUT_NEW/LAST_N.
   * Soft empty/mkdir/touch fail → OK=0 sticky LAST_ERR.
   * Usability: nested lease/marker files without ENSUREPARENT + SYS TOUCH glue. */
  if (kw(&L->cur, "TOUCHOUT") || kw(&L->cur, "MKFILEOUT") || kw(&L->cur, "ENSURETOUCH") ||
      kw(&L->cur, "TOUCHPARENT") || kw(&L->cur, "STAMPOUT") || kw(&L->cur, "LEASEOUT") ||
      kw(&L->cur, "MARKEROUT") || kw(&L->cur, "TOUCHNEST") || kw(&L->cur, "PLATETOUCH")) {
    char path[CUBALC_HOST_STR_MAX], absfile[CUBALC_HOST_STR_MAX];
    char parent[512], absparent[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0;
    cubalc_host_result hr;
    lex_next(L);
    path[0] = 0;
    absfile[0] = 0;
    parent[0] = 0;
    absparent[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
      fail_at(vm, L, "TOUCHOUT needs path — TOUCHOUT \"a/b/lease.stamp\"");
      return -1;
    }
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "TOUCHOUT: empty path");
      var_set_str(vm, "ERR", "TOUCHOUT: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "TOUCHOUT_N", 0);
      var_set_num(vm, "TOUCHOUT_CREATED", 0);
      var_set_num(vm, "TOUCHOUT_NEW", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(path, &hr) == 0 && hr.str[0])
      snprintf(absfile, sizeof absfile, "%s", hr.str);
    else
      snprintf(absfile, sizeof absfile, "%s", path);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absfile);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "TOUCHOUT: mkdir parent failed";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "TOUCHOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "TOUCHOUT_N", 0);
      var_set_num(vm, "TOUCHOUT_CREATED", 0);
      var_set_num(vm, "TOUCHOUT_NEW", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_touch(absfile, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "TOUCHOUT: touch failed";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "TOUCHOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "TOUCHOUT_N", 0);
      var_set_num(vm, "TOUCHOUT_CREATED", (long)created);
      var_set_num(vm, "TOUCHOUT_NEW", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", absfile);
    var_set_str(vm, "TOUCHOUT", absfile);
    var_set_str(vm, "PATH", absfile);
    var_set_str(vm, "TOUCHOUT_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "TOUCHOUT_N", hr.n);
    var_set_num(vm, "TOUCH_N", hr.n);
    var_set_num(vm, "TOUCHOUT_NEW", hr.n);
    var_set_num(vm, "TOUCHOUT_CREATED", (long)created);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# touchout → %s new=%ld parent_created=%d\n",
              absfile, hr.n, created);
    bump(vm);
    return 1;
  }

  /* APPENDOUT|LOGOUT path data — ENSUREPARENT + APPEND line one-shot.
   * LAST = abs file path · APPENDOUT_PARENT · APPENDOUT_CREATED · LAST_N=data+NL.
   * Soft empty/mkdir/open fail → OK=0 sticky LAST_ERR.
   * Usability: nested agent logs without ENSUREPARENT + SYS APPEND glue. */
  if (kw(&L->cur, "APPENDOUT") || kw(&L->cur, "LOGOUT") || kw(&L->cur, "APPENDLOG") ||
      kw(&L->cur, "ENSUREAPPEND") || kw(&L->cur, "APPENDPATH") || kw(&L->cur, "LOGPATH") ||
      kw(&L->cur, "HISTORYOUT") || kw(&L->cur, "AUDITOUT") || kw(&L->cur, "APPENDPARENT")) {
    char path[CUBALC_HOST_STR_MAX], data[CUBALC_HOST_STR_MAX];
    char absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0;
    long nbytes = 0;
    cubalc_host_result hr;
    FILE *af;
    lex_next(L);
    path[0] = 0;
    data[0] = 0;
    absfile[0] = 0;
    parent[0] = 0;
    absparent[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
      fail_at(vm, L, "APPENDOUT needs path — APPENDOUT \"a/b/log.txt\" \"line\"|var");
      return -1;
    }
    if (resolve_str_arg(vm, L, data, sizeof data) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(data, sizeof data, "%ld", nv);
      } else {
        data[0] = 0;
      }
    }
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "APPENDOUT: empty path");
      var_set_str(vm, "ERR", "APPENDOUT: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "APPENDOUT_N", 0);
      var_set_num(vm, "APPENDOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(path, &hr) == 0 && hr.str[0])
      snprintf(absfile, sizeof absfile, "%s", hr.str);
    else
      snprintf(absfile, sizeof absfile, "%s", path);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absfile);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "APPENDOUT: mkdir parent failed";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "APPENDOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "APPENDOUT_N", 0);
      var_set_num(vm, "APPENDOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    af = fopen(absfile, "a");
    if (!af) {
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "APPENDOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", "APPENDOUT: open fail");
      var_set_str(vm, "ERR", "APPENDOUT: open fail");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "APPENDOUT_N", 0);
      var_set_num(vm, "APPENDOUT_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    fputs(data, af);
    fputc('\n', af);
    fclose(af);
    nbytes = (long)strlen(data) + 1;
    var_set_str(vm, "LAST", absfile);
    var_set_str(vm, "APPENDOUT", absfile);
    var_set_str(vm, "PATH", absfile);
    var_set_str(vm, "APPENDOUT_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
    vm->last_n = nbytes;
    var_set_num(vm, "LAST_N", nbytes);
    var_set_num(vm, "APPENDOUT_N", nbytes);
    var_set_num(vm, "APPENDOUT_CREATED", (long)created);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# appendout → %s bytes=%ld created=%d\n",
              absfile, nbytes, created);
    bump(vm);
    return 1;
  }

  /* COPYOUT|CPOUT src dest — ENSUREPARENT dest + COPY one-shot.
   * LAST = abs dest · COPYOUT_SRC · COPYOUT_PARENT · COPYOUT_CREATED · LAST_N=bytes.
   * Soft empty/mkdir/copy fail → OK=0 sticky LAST_ERR.
   * Usability: nested plate snapshot without ENSUREPARENT + SYS COPY glue. */
  if (kw(&L->cur, "COPYOUT") || kw(&L->cur, "CPOUT") || kw(&L->cur, "ENSURECOPY") ||
      kw(&L->cur, "COPYPARENT") || kw(&L->cur, "PLATECOPY") || kw(&L->cur, "SNAPOUT") ||
      kw(&L->cur, "FILEOUTCOPY") || kw(&L->cur, "COPYNEST") || kw(&L->cur, "STAGECOPY")) {
    char src[CUBALC_HOST_STR_MAX], dst[CUBALC_HOST_STR_MAX];
    char abssrc[CUBALC_HOST_STR_MAX], absdst[CUBALC_HOST_STR_MAX];
    char parent[512], absparent[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0;
    cubalc_host_result hr;
    lex_next(L);
    src[0] = 0;
    dst[0] = 0;
    abssrc[0] = 0;
    absdst[0] = 0;
    parent[0] = 0;
    absparent[0] = 0;
    if (resolve_str_arg(vm, L, src, sizeof src) != 0) {
      fail_at(vm, L, "COPYOUT needs src dest — COPYOUT \"src.json\" \"a/b/copy.json\"");
      return -1;
    }
    if (resolve_str_arg(vm, L, dst, sizeof dst) != 0) {
      fail_at(vm, L, "COPYOUT needs dest — COPYOUT \"src.json\" \"a/b/copy.json\"");
      return -1;
    }
    if (!src[0] || !dst[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "COPYOUT: empty path");
      var_set_str(vm, "ERR", "COPYOUT: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "COPYOUT_N", 0);
      var_set_num(vm, "COPYOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(src, &hr) == 0 && hr.str[0])
      snprintf(abssrc, sizeof abssrc, "%s", hr.str);
    else
      snprintf(abssrc, sizeof abssrc, "%s", src);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(dst, &hr) == 0 && hr.str[0])
      snprintf(absdst, sizeof absdst, "%s", hr.str);
    else
      snprintf(absdst, sizeof absdst, "%s", dst);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absdst);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "COPYOUT: mkdir parent failed";
      var_set_str(vm, "LAST", absdst);
      var_set_str(vm, "COPYOUT_SRC", abssrc);
      var_set_str(vm, "COPYOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absdst);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "COPYOUT_N", 0);
      var_set_num(vm, "COPYOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_copy(abssrc, absdst, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "COPYOUT: copy failed";
      var_set_str(vm, "LAST", absdst);
      var_set_str(vm, "COPYOUT_SRC", abssrc);
      var_set_str(vm, "COPYOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absdst);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "COPYOUT_N", 0);
      var_set_num(vm, "COPYOUT_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", absdst);
    var_set_str(vm, "COPYOUT", absdst);
    var_set_str(vm, "CPOUT", absdst);
    var_set_str(vm, "PATH", absdst);
    var_set_str(vm, "COPYOUT_SRC", abssrc);
    var_set_str(vm, "COPYOUT_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absdst);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "COPYOUT_N", hr.n);
    var_set_num(vm, "COPY_N", hr.n);
    var_set_num(vm, "COPYOUT_CREATED", (long)created);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# copyout → %s ← %s bytes=%ld created=%d\n",
              absdst, abssrc, hr.n, created);
    bump(vm);
    return 1;
  }

  /* MOVEOUT|MVOUT src dest — ENSUREPARENT dest + RENAME one-shot.
   * LAST = abs dest · MOVEOUT_SRC · MOVEOUT_PARENT · MOVEOUT_CREATED · LAST_N=1.
   * Soft empty/mkdir/rename fail → OK=0 sticky LAST_ERR.
   * Usability: promote temp plates into nested paths without ENSUREPARENT + MV glue. */
  if (kw(&L->cur, "MOVEOUT") || kw(&L->cur, "MVOUT") || kw(&L->cur, "ENSUREMOVE") ||
      kw(&L->cur, "MOVEPARENT") || kw(&L->cur, "RENAMEOUT") || kw(&L->cur, "STAGEOUT") ||
      kw(&L->cur, "RELOCATEOUT") || kw(&L->cur, "MOVENEST") || kw(&L->cur, "PLATEMOVE")) {
    char src[CUBALC_HOST_STR_MAX], dst[CUBALC_HOST_STR_MAX];
    char abssrc[CUBALC_HOST_STR_MAX], absdst[CUBALC_HOST_STR_MAX];
    char parent[512], absparent[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0;
    cubalc_host_result hr;
    lex_next(L);
    src[0] = 0;
    dst[0] = 0;
    abssrc[0] = 0;
    absdst[0] = 0;
    parent[0] = 0;
    absparent[0] = 0;
    if (resolve_str_arg(vm, L, src, sizeof src) != 0) {
      fail_at(vm, L, "MOVEOUT needs src dest — MOVEOUT \"tmp.json\" \"a/b/final.json\"");
      return -1;
    }
    if (resolve_str_arg(vm, L, dst, sizeof dst) != 0) {
      fail_at(vm, L, "MOVEOUT needs dest — MOVEOUT \"tmp.json\" \"a/b/final.json\"");
      return -1;
    }
    if (!src[0] || !dst[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "MOVEOUT: empty path");
      var_set_str(vm, "ERR", "MOVEOUT: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "MOVEOUT_N", 0);
      var_set_num(vm, "MOVEOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(src, &hr) == 0 && hr.str[0])
      snprintf(abssrc, sizeof abssrc, "%s", hr.str);
    else
      snprintf(abssrc, sizeof abssrc, "%s", src);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(dst, &hr) == 0 && hr.str[0])
      snprintf(absdst, sizeof absdst, "%s", hr.str);
    else
      snprintf(absdst, sizeof absdst, "%s", dst);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absdst);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "MOVEOUT: mkdir parent failed";
      var_set_str(vm, "LAST", absdst);
      var_set_str(vm, "MOVEOUT_SRC", abssrc);
      var_set_str(vm, "MOVEOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absdst);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "MOVEOUT_N", 0);
      var_set_num(vm, "MOVEOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_rename(abssrc, absdst, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "MOVEOUT: rename failed";
      var_set_str(vm, "LAST", absdst);
      var_set_str(vm, "MOVEOUT_SRC", abssrc);
      var_set_str(vm, "MOVEOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absdst);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "MOVEOUT_N", 0);
      var_set_num(vm, "MOVEOUT_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", absdst);
    var_set_str(vm, "MOVEOUT", absdst);
    var_set_str(vm, "MVOUT", absdst);
    var_set_str(vm, "PATH", absdst);
    var_set_str(vm, "MOVEOUT_SRC", abssrc);
    var_set_str(vm, "MOVEOUT_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absdst);
    vm->last_n = 1;
    var_set_num(vm, "LAST_N", 1);
    var_set_num(vm, "MOVEOUT_N", 1);
    var_set_num(vm, "MOVEOUT_CREATED", (long)created);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# moveout → %s ← %s created=%d\n",
              absdst, abssrc, created);
    bump(vm);
    return 1;
  }

  /* WRITEOUTATOMIC|SAFEWRITEOUT path data — ENSUREPARENT + WRITEATOMIC one-shot.
   * Temp+rename into nested plate path. LAST = abs file · CREATED · LAST_N=bytes.
   * Soft empty/mkdir/write/rename fail → OK=0 sticky LAST_ERR.
   * Usability: multi-agent nested plates without ENSUREPARENT + WRITEATOMIC glue. */
  if (kw(&L->cur, "WRITEOUTATOMIC") || kw(&L->cur, "SAFEWRITEOUT") ||
      kw(&L->cur, "ATOMICWRITEOUT") || kw(&L->cur, "WRITEOUTSAFE") ||
      kw(&L->cur, "COMMITOUT") || kw(&L->cur, "ATOMICOUT") ||
      kw(&L->cur, "ENSUREATOMIC") || kw(&L->cur, "SAFEOUTWRITE") ||
      kw(&L->cur, "WRITEAOUT")) {
    char path[CUBALC_HOST_STR_MAX], data[CUBALC_HOST_STR_MAX];
    char absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
    char tmp[CUBALC_HOST_STR_MAX + 64];
    const char *slash;
    size_t n;
    int created = 0;
    long pid;
    cubalc_host_result hr, rr, trash;
    lex_next(L);
    path[0] = 0;
    data[0] = 0;
    absfile[0] = 0;
    parent[0] = 0;
    absparent[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
      fail_at(vm, L, "WRITEOUTATOMIC needs path — WRITEOUTATOMIC \"a/b/c.json\" \"data\"|var");
      return -1;
    }
    if (resolve_str_arg(vm, L, data, sizeof data) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(data, sizeof data, "%ld", nv);
      } else {
        data[0] = 0;
      }
    }
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "WRITEOUTATOMIC: empty path");
      var_set_str(vm, "ERR", "WRITEOUTATOMIC: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEOUTATOMIC_N", 0);
      var_set_num(vm, "WRITEOUTATOMIC_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(path, &hr) == 0 && hr.str[0])
      snprintf(absfile, sizeof absfile, "%s", hr.str);
    else
      snprintf(absfile, sizeof absfile, "%s", path);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absfile);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "WRITEOUTATOMIC: mkdir parent failed";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "WRITEOUTATOMIC_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEOUTATOMIC_N", 0);
      var_set_num(vm, "WRITEOUTATOMIC_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
#if defined(CUBALC_OS_WINDOWS)
    pid = 0;
#else
    pid = (long)getpid();
#endif
    snprintf(tmp, sizeof tmp, "%s.cubalc-tmp.%ld", absfile, pid);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_write(tmp, data, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "WRITEOUTATOMIC: write temp fail";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "WRITEOUTATOMIC_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEOUTATOMIC_N", 0);
      var_set_num(vm, "WRITEOUTATOMIC_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    memset(&rr, 0, sizeof rr);
    if (cubalc_host_rename(tmp, absfile, &rr) != 0) {
      memset(&trash, 0, sizeof trash);
      cubalc_host_rm(tmp, &trash);
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "WRITEOUTATOMIC_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", rr.err[0] ? rr.err : "WRITEOUTATOMIC: rename fail");
      var_set_str(vm, "ERR", rr.err[0] ? rr.err : "WRITEOUTATOMIC: rename fail");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEOUTATOMIC_N", 0);
      var_set_num(vm, "WRITEOUTATOMIC_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", absfile);
    var_set_str(vm, "WRITEOUTATOMIC", absfile);
    var_set_str(vm, "SAFEWRITEOUT", absfile);
    var_set_str(vm, "PATH", absfile);
    var_set_str(vm, "WRITEOUTATOMIC_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "WRITEOUTATOMIC_N", hr.n);
    var_set_num(vm, "WRITEATOMIC_N", hr.n);
    var_set_num(vm, "WRITEOUTATOMIC_CREATED", (long)created);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# writeoutatomic → %s bytes=%ld created=%d\n",
              absfile, hr.n, created);
    bump(vm);
    return 1;
  }

  /* WRITEOUTIF|PUTIFCHANGED path data — ENSUREPARENT; write only if content differs.
   * Skip when file already equals data (mtime-stable for WAITCHANGED/FRESH).
   * On change: temp+rename atomic write. LAST = abs · CHANGED 0|1 · LAST_N=bytes|0.
   * Soft empty/mkdir/write fail → OK=0 sticky LAST_ERR. */
  if (kw(&L->cur, "WRITEOUTIF") || kw(&L->cur, "PUTIFCHANGED") || kw(&L->cur, "WRITEIFCHANGED") ||
      kw(&L->cur, "SAVEIF") || kw(&L->cur, "PUTIF") || kw(&L->cur, "STABLEWRITE") ||
      kw(&L->cur, "WRITEUNLESSSAME") || kw(&L->cur, "IFWRITEOUT") || kw(&L->cur, "WRITEOUTDIFF")) {
    char path[CUBALC_HOST_STR_MAX], data[CUBALC_HOST_STR_MAX];
    char absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
    char tmp[CUBALC_HOST_STR_MAX + 64];
    const char *slash;
    size_t n;
    int created = 0, changed = 1;
    long pid;
    cubalc_host_result hr, rr, trash;
    lex_next(L);
    path[0] = 0;
    data[0] = 0;
    absfile[0] = 0;
    parent[0] = 0;
    absparent[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
      fail_at(vm, L, "WRITEOUTIF needs path — WRITEOUTIF \"a/b/c.json\" \"data\"|var");
      return -1;
    }
    if (resolve_str_arg(vm, L, data, sizeof data) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(data, sizeof data, "%ld", nv);
      } else {
        data[0] = 0;
      }
    }
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "WRITEOUTIF: empty path");
      var_set_str(vm, "ERR", "WRITEOUTIF: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEOUTIF_N", 0);
      var_set_num(vm, "WRITEOUTIF_CHANGED", 0);
      var_set_num(vm, "WRITEOUTIF_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(path, &hr) == 0 && hr.str[0])
      snprintf(absfile, sizeof absfile, "%s", hr.str);
    else
      snprintf(absfile, sizeof absfile, "%s", path);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absfile);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    if (cubalc_host_exists(absfile)) {
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_read(absfile, &hr) == 0 && strcmp(hr.str, data) == 0) {
        changed = 0;
        var_set_str(vm, "LAST", absfile);
        var_set_str(vm, "WRITEOUTIF", absfile);
        var_set_str(vm, "PATH", absfile);
        var_set_str(vm, "WRITEOUTIF_PARENT", absparent);
        var_set_str(vm, "DIRNAME", absparent);
        var_set_str(vm, "PARENT", absparent);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "WRITEOUTIF_N", 0);
        var_set_num(vm, "WRITEOUTIF_CHANGED", 0);
        var_set_num(vm, "WRITEOUTIF_CREATED", 0);
        var_set_num(vm, "PATH_EXIST", 1);
        var_set_num(vm, "OK", 1);
        if (vm->trace)
          fprintf(vm->trace, "# writeoutif → %s unchanged\n", absfile);
        bump(vm);
        return 1;
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "WRITEOUTIF: mkdir parent failed";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "WRITEOUTIF_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEOUTIF_N", 0);
      var_set_num(vm, "WRITEOUTIF_CHANGED", 0);
      var_set_num(vm, "WRITEOUTIF_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
#if defined(CUBALC_OS_WINDOWS)
    pid = 0;
#else
    pid = (long)getpid();
#endif
    snprintf(tmp, sizeof tmp, "%s.cubalc-tmp.%ld", absfile, pid);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_write(tmp, data, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "WRITEOUTIF: write temp fail";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "WRITEOUTIF_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEOUTIF_N", 0);
      var_set_num(vm, "WRITEOUTIF_CHANGED", 0);
      var_set_num(vm, "WRITEOUTIF_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    memset(&rr, 0, sizeof rr);
    if (cubalc_host_rename(tmp, absfile, &rr) != 0) {
      memset(&trash, 0, sizeof trash);
      cubalc_host_rm(tmp, &trash);
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "WRITEOUTIF_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", rr.err[0] ? rr.err : "WRITEOUTIF: rename fail");
      var_set_str(vm, "ERR", rr.err[0] ? rr.err : "WRITEOUTIF: rename fail");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "WRITEOUTIF_N", 0);
      var_set_num(vm, "WRITEOUTIF_CHANGED", 0);
      var_set_num(vm, "WRITEOUTIF_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", absfile);
    var_set_str(vm, "WRITEOUTIF", absfile);
    var_set_str(vm, "PATH", absfile);
    var_set_str(vm, "WRITEOUTIF_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "WRITEOUTIF_N", hr.n);
    var_set_num(vm, "WRITEOUTIF_CHANGED", (long)changed);
    var_set_num(vm, "WRITEOUTIF_CREATED", (long)created);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# writeoutif → %s bytes=%ld changed=%d created=%d\n",
              absfile, hr.n, changed, created);
    bump(vm);
    return 1;
  }

  /* LOGSTAMP|STAMPLOG path msg — ENSUREPARENT + append "ISO msg" line one-shot.
   * LAST = abs file · LOGSTAMP_LINE · LOGSTAMP_ISO · CREATED · LAST_N = line+NL.
   * Soft empty/mkdir/open fail → OK=0 sticky LAST_ERR.
   * Usability: timestamped agent logs without SYS DATE + CAT + APPENDOUT glue. */
  if (kw(&L->cur, "LOGSTAMP") || kw(&L->cur, "STAMPLOG") || kw(&L->cur, "APPENDOUTISO") ||
      kw(&L->cur, "LOGISO") || kw(&L->cur, "ISOLOG") || kw(&L->cur, "AUDITSTAMP") ||
      kw(&L->cur, "TSLOG") || kw(&L->cur, "LOGTS") || kw(&L->cur, "STAMPAPPEND")) {
    char path[CUBALC_HOST_STR_MAX], msg[CUBALC_HOST_STR_MAX];
    char absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
    char iso[40], line[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0;
    long nbytes = 0;
    time_t now;
    struct tm tm_utc;
    cubalc_host_result hr;
    FILE *af;
    lex_next(L);
    path[0] = 0;
    msg[0] = 0;
    absfile[0] = 0;
    parent[0] = 0;
    absparent[0] = 0;
    iso[0] = 0;
    line[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
      fail_at(vm, L, "LOGSTAMP needs path — LOGSTAMP \"a/b/agent.log\" \"tick ok\"");
      return -1;
    }
    if (resolve_str_arg(vm, L, msg, sizeof msg) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(msg, sizeof msg, "%ld", nv);
      } else {
        msg[0] = 0;
      }
    }
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "LOGSTAMP: empty path");
      var_set_str(vm, "ERR", "LOGSTAMP: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "LOGSTAMP_N", 0);
      var_set_num(vm, "LOGSTAMP_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    now = time(NULL);
#if defined(CUBALC_OS_WINDOWS)
    {
      struct tm *tp = gmtime(&now);
      if (tp) tm_utc = *tp;
      else memset(&tm_utc, 0, sizeof tm_utc);
    }
#else
    if (!gmtime_r(&now, &tm_utc))
      memset(&tm_utc, 0, sizeof tm_utc);
#endif
    snprintf(iso, sizeof iso, "%04d-%02d-%02dT%02d:%02d:%02dZ",
             tm_utc.tm_year + 1900, tm_utc.tm_mon + 1, tm_utc.tm_mday,
             tm_utc.tm_hour, tm_utc.tm_min, tm_utc.tm_sec);
    if (msg[0])
      snprintf(line, sizeof line, "%s %s", iso, msg);
    else
      snprintf(line, sizeof line, "%s", iso);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(path, &hr) == 0 && hr.str[0])
      snprintf(absfile, sizeof absfile, "%s", hr.str);
    else
      snprintf(absfile, sizeof absfile, "%s", path);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absfile);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "LOGSTAMP: mkdir parent failed";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "LOGSTAMP_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "LOGSTAMP_N", 0);
      var_set_num(vm, "LOGSTAMP_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    af = fopen(absfile, "a");
    if (!af) {
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "LOGSTAMP_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", "LOGSTAMP: open fail");
      var_set_str(vm, "ERR", "LOGSTAMP: open fail");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "LOGSTAMP_N", 0);
      var_set_num(vm, "LOGSTAMP_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    fputs(line, af);
    fputc('\n', af);
    fclose(af);
    nbytes = (long)strlen(line) + 1;
    var_set_str(vm, "LAST", absfile);
    var_set_str(vm, "LOGSTAMP", absfile);
    var_set_str(vm, "PATH", absfile);
    var_set_str(vm, "LOGSTAMP_PARENT", absparent);
    var_set_str(vm, "LOGSTAMP_LINE", line);
    var_set_str(vm, "LOGSTAMP_ISO", iso);
    var_set_str(vm, "ISO", iso);
    var_set_str(vm, "DATE", iso);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
    vm->last_n = nbytes;
    var_set_num(vm, "LAST_N", nbytes);
    var_set_num(vm, "LOGSTAMP_N", nbytes);
    var_set_num(vm, "APPENDOUT_N", nbytes);
    var_set_num(vm, "LOGSTAMP_CREATED", (long)created);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# logstamp → %s line=%s created=%d\n",
              absfile, line, created);
    bump(vm);
    return 1;
  }


  /* TEEOUT|TEEWOUT path data — ENSUREPARENT + write keep content in LAST.
   * LAST = data body · TEEOUT_PATH = abs file · TEEOUT_PARENT · CREATED · LAST_N=bytes.
   * Soft empty/mkdir/write fail → OK=0 sticky LAST_ERR (LAST still data when possible).
   * Usability: nested plate write then GREP/HAS without READ back or path clobber. */
  if (kw(&L->cur, "TEEOUT") || kw(&L->cur, "TEEWOUT") || kw(&L->cur, "ENSURETEE") ||
      kw(&L->cur, "TEEPARENT") || kw(&L->cur, "NESTTEE") || kw(&L->cur, "PLATETEE") ||
      kw(&L->cur, "SAVEKEEPOUT") || kw(&L->cur, "WRITEKEEPOUT") || kw(&L->cur, "KEEPWRITEOUT")) {
    char path[CUBALC_HOST_STR_MAX], data[CUBALC_HOST_STR_MAX];
    char absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0;
    cubalc_host_result hr;
    lex_next(L);
    path[0] = 0;
    data[0] = 0;
    absfile[0] = 0;
    parent[0] = 0;
    absparent[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
      fail_at(vm, L, "TEEOUT needs path — TEEOUT \"a/b/c.json\" \"data\"|var");
      return -1;
    }
    if (resolve_str_arg(vm, L, data, sizeof data) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(data, sizeof data, "%ld", nv);
      } else {
        snprintf(data, sizeof data, "%s", vm->last_str);
      }
    }
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "TEEOUT: empty path");
      var_set_str(vm, "ERR", "TEEOUT: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "TEEOUT_N", 0);
      var_set_num(vm, "TEEOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(path, &hr) == 0 && hr.str[0])
      snprintf(absfile, sizeof absfile, "%s", hr.str);
    else
      snprintf(absfile, sizeof absfile, "%s", path);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absfile);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "TEEOUT: mkdir parent failed";
      var_set_str(vm, "LAST", data);
      var_set_str(vm, "TEEOUT_PATH", absfile);
      var_set_str(vm, "TEEOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", data);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "TEEOUT_N", 0);
      var_set_num(vm, "TEEOUT_CREATED", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_write(absfile, data, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "TEEOUT: write failed";
      var_set_str(vm, "LAST", data);
      var_set_str(vm, "TEEOUT_PATH", absfile);
      var_set_str(vm, "TEEOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", data);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "TEEOUT_N", 0);
      var_set_num(vm, "TEEOUT_CREATED", (long)created);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    /* LAST = body (not path) — chain HAS/GREP without READ back */
    var_set_str(vm, "LAST", data);
    var_set_str(vm, "TEE", data);
    var_set_str(vm, "TEEOUT", data);
    var_set_str(vm, "TEEOUT_PATH", absfile);
    var_set_str(vm, "TEE_PATH", absfile);
    var_set_str(vm, "PATH", absfile);
    var_set_str(vm, "TEEOUT_PARENT", absparent);
    var_set_str(vm, "DIRNAME", absparent);
    var_set_str(vm, "PARENT", absparent);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", data);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "TEEOUT_N", hr.n);
    var_set_num(vm, "TEE_N", hr.n);
    var_set_num(vm, "TEEOUT_CREATED", (long)created);
    var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# teeout → %s bytes=%ld created=%d\n",
              absfile, hr.n, created);
    bump(vm);
    return 1;
  }


  /* CLAIMOUT|CLAIMNEST path [body] — ENSUREPARENT + exclusive create (soft).
   * LAST = abs path · CLAIM_HIT 1|0 · CLAIMOUT_CREATED · CLAIM_PID.
   * Soft empty/mkdir fail or exists → OK=0 sticky LAST_ERR.
   * Usability: nested work-item claims without DIRNAME + MKDIR + CLAIM glue. */
  if (kw(&L->cur, "CLAIMOUT") || kw(&L->cur, "CLAIMNEST") || kw(&L->cur, "ENSURECLAIM") ||
      kw(&L->cur, "CLAIMPARENT") || kw(&L->cur, "EXCLOUT") || kw(&L->cur, "TAKECLAIMOUT") ||
      kw(&L->cur, "ONCEOUT") || kw(&L->cur, "WORKCLAIM") || kw(&L->cur, "JOBCLAIM")) {
    char path[CUBALC_HOST_STR_MAX], body[CUBALC_HOST_STR_MAX];
    char absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
    const char *slash;
    size_t n;
    int created = 0, hit = 0;
    long pid = 0;
    cubalc_host_result hr;
    lex_next(L);
    path[0] = 0;
    body[0] = 0;
    absfile[0] = 0;
    parent[0] = 0;
    absparent[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
      fail_at(vm, L, "CLAIMOUT needs path — CLAIMOUT \"a/b/item.claim\" [body]");
      return -1;
    }
    if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      if (resolve_str_arg(vm, L, body, sizeof body) != 0)
        body[0] = 0;
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long nv = parse_expr(vm, L);
      snprintf(body, sizeof body, "%ld", nv);
    }
#if defined(CUBALC_OS_WINDOWS)
    pid = 0;
#else
    pid = (long)getpid();
#endif
    if (!body[0])
      snprintf(body, sizeof body, "%ld\n", pid);
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "LAST_ERR", "CLAIMOUT: empty path");
      var_set_str(vm, "ERR", "CLAIMOUT: empty path");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "CLAIMOUT_N", 0);
      var_set_num(vm, "CLAIM_HIT", 0);
      var_set_num(vm, "CLAIMOUT_CREATED", 0);
      var_set_num(vm, "CLAIM_PID", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(path, &hr) == 0 && hr.str[0])
      snprintf(absfile, sizeof absfile, "%s", hr.str);
    else
      snprintf(absfile, sizeof absfile, "%s", path);
    {
      char work[CUBALC_HOST_STR_MAX];
      snprintf(work, sizeof work, "%s", absfile);
      n = strlen(work);
      while (n > 1 && (work[n - 1] == '/' || work[n - 1] == '\\')) {
        work[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(work);
      if (slash && slash != work) {
        size_t dn = (size_t)(slash - work);
        if (dn >= sizeof parent) dn = sizeof parent - 1;
        memcpy(parent, work, dn);
        parent[dn] = 0;
      } else if (slash && slash == work) {
        snprintf(parent, sizeof parent, "%c", work[0] == '\\' ? '\\' : '/');
      } else {
        snprintf(parent, sizeof parent, ".");
      }
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
    else
      snprintf(absparent, sizeof absparent, "%s", parent);
    created = (cubalc_host_exists(absparent) || cubalc_host_exists(parent)) ? 0 : 1;
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mkdir(absparent, &hr) != 0) {
      const char *err = hr.err[0] ? hr.err : "CLAIMOUT: mkdir parent failed";
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "CLAIMOUT_PARENT", absparent);
      var_set_str(vm, "LAST_ERR", err);
      var_set_str(vm, "ERR", err);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "CLAIMOUT_N", 0);
      var_set_num(vm, "CLAIM_HIT", 0);
      var_set_num(vm, "CLAIMOUT_CREATED", 0);
      var_set_num(vm, "CLAIM_PID", 0);
      var_set_num(vm, "OK", 0);
      bump(vm);
      return 1;
    }
    if (hr.str[0])
      snprintf(absparent, sizeof absparent, "%s", hr.str);
#if defined(CUBALC_OS_WINDOWS)
    {
      FILE *lf = fopen(absfile, "r");
      if (!lf) {
        cubalc_host_result wr;
        memset(&wr, 0, sizeof wr);
        if (cubalc_host_write(absfile, body, &wr) == 0)
          hit = 1;
      } else {
        fclose(lf);
      }
    }
#else
    {
      int fd = open(absfile, O_CREAT | O_EXCL | O_WRONLY, 0644);
      if (fd >= 0) {
        size_t bl = strlen(body);
        ssize_t w = write(fd, body, bl);
        (void)w;
        close(fd);
        hit = 1;
      }
    }
#endif
    if (hit) {
      var_set_str(vm, "LAST", absfile);
      var_set_str(vm, "CLAIMOUT", absfile);
      var_set_str(vm, "CLAIM", absfile);
      var_set_str(vm, "CLAIMFILE", absfile);
      var_set_str(vm, "PATH", absfile);
      var_set_str(vm, "CLAIMOUT_PARENT", absparent);
      var_set_str(vm, "DIRNAME", absparent);
      var_set_str(vm, "PARENT", absparent);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "CLAIMOUT_N", 1);
      var_set_num(vm, "CLAIM_HIT", 1);
      var_set_num(vm, "CLAIM_N", 1);
      var_set_num(vm, "CLAIM_PID", pid);
      var_set_num(vm, "CLAIMOUT_CREATED", (long)created);
      var_set_num(vm, "ENSUREPARENT_CREATED", (long)created);
      var_set_num(vm, "PATH_EXIST", 1);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# claimout → %s hit=1 created=%d\n", absfile, created);
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", absfile);
    var_set_str(vm, "CLAIMOUT", absfile);
    var_set_str(vm, "CLAIMOUT_PARENT", absparent);
    var_set_str(vm, "LAST_ERR", "CLAIMOUT: exists");
    var_set_str(vm, "ERR", "CLAIMOUT: exists");
    snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
    vm->last_n = 0;
    var_set_num(vm, "LAST_N", 0);
    var_set_num(vm, "CLAIMOUT_N", 0);
    var_set_num(vm, "CLAIM_HIT", 0);
    var_set_num(vm, "CLAIM_N", 0);
    var_set_num(vm, "CLAIM_PID", 0);
    var_set_num(vm, "CLAIMOUT_CREATED", (long)created);
    var_set_num(vm, "OK", 0);
    if (vm->trace)
      fprintf(vm->trace, "# claimout → %s hit=0 created=%d\n", absfile, created);
    bump(vm);
    return 1;
  }


  /* RMTREE|RMNEST|RMRF|DELTREE path — recursive remove file or directory tree.
   * Twin of SYS RMTREE. LAST = abs path · LAST_N / RMTREE_N = entries removed.
   * Soft empty/fail · missing soft OK (LAST_N=0). File or symlink → unlink.
   * Usability: clean nested agent workspaces without shell rm -rf or
   * WALK+RMALL+RMDIR glue after MKTEMPDIR/CLAIMOUT/WRITEOUT. */
  if (kw(&L->cur, "RMTREE") || kw(&L->cur, "RMNEST") || kw(&L->cur, "RMRF") ||
      kw(&L->cur, "DELTREE") || kw(&L->cur, "RM_TREE") || kw(&L->cur, "PURGETREE") ||
      kw(&L->cur, "REMOVETREE") || kw(&L->cur, "WIPETREE") || kw(&L->cur, "RMREC") ||
      kw(&L->cur, "RECURSERM")) {
    char path[CUBALC_HOST_STR_MAX], abs[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    lex_next(L);
    path[0] = 0;
    abs[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
      fail_at(vm, L, "RMTREE needs path — RMTREE \"work/tmp/root\"");
      return -1;
    }
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "RMTREE_N", 0);
      var_set_num(vm, "RMNEST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "RMTREE: empty path");
      var_set_str(vm, "ERR", "RMTREE: empty path");
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_abspath(path, &hr) == 0 && hr.str[0])
      snprintf(abs, sizeof abs, "%s", hr.str);
    else
      snprintf(abs, sizeof abs, "%s", path);
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_rmtree(abs, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "RMTREE_N", 0);
      var_set_num(vm, "RMNEST_N", 0);
      vm->last_n = 0;
      var_set_str(vm, "LAST", abs);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", abs);
      if (hr.err[0]) {
        var_set_str(vm, "LAST_ERR", hr.err);
        var_set_str(vm, "ERR", hr.err);
      } else {
        var_set_str(vm, "LAST_ERR", "RMTREE: fail");
        var_set_str(vm, "ERR", "RMTREE: fail");
      }
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", abs);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", abs);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "RMTREE_N", hr.n);
    var_set_num(vm, "RMNEST_N", hr.n);
    var_set_num(vm, "RMRF_N", hr.n);
    var_set_str(vm, "RMTREE", abs);
    var_set_str(vm, "RMNEST", abs);
    var_set_str(vm, "PATH", abs);
    var_set_num(vm, "PATH_EXIST", 0);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# rmtree → %s n=%ld\n", abs, hr.n);
    bump(vm);
    return 1;
  }


  /* CPTREE|COPYTREE|CPDIR src dest — recursive copy file or directory tree.
   * Twin of SYS CPTREE. LAST = abs dest · LAST_N/CPTREE_N = entries · CPTREE_BYTES.
   * Soft empty/missing · dest-inside-src refused.
   * Usability: snapshot nested agent workspaces without shell cp -r. */
  if (kw(&L->cur, "CPTREE") || kw(&L->cur, "COPYTREE") || kw(&L->cur, "CPDIR") ||
      kw(&L->cur, "COPYDIR") || kw(&L->cur, "CP_TREE") || kw(&L->cur, "TREECOPY") ||
      kw(&L->cur, "RCPY") || kw(&L->cur, "RCOPY") || kw(&L->cur, "SNAPSHOTTREE") ||
      kw(&L->cur, "DUPTREE")) {
    char src[CUBALC_HOST_STR_MAX], dst[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    lex_next(L);
    src[0] = 0;
    dst[0] = 0;
    if (resolve_str_arg(vm, L, src, sizeof src) != 0) {
      fail_at(vm, L, "CPTREE needs src dest — CPTREE \"work/a\" \"work/b\"");
      return -1;
    }
    if (resolve_str_arg(vm, L, dst, sizeof dst) != 0) {
      fail_at(vm, L, "CPTREE needs src dest — CPTREE \"work/a\" \"work/b\"");
      return -1;
    }
    if (!src[0] || !dst[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "CPTREE_N", 0);
      var_set_num(vm, "CPTREE_BYTES", 0);
      var_set_num(vm, "COPYTREE_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "CPTREE: empty path");
      var_set_str(vm, "ERR", "CPTREE: empty path");
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_cptree(src, dst, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "CPTREE_N", 0);
      var_set_num(vm, "CPTREE_BYTES", 0);
      var_set_num(vm, "COPYTREE_N", 0);
      vm->last_n = 0;
      var_set_str(vm, "LAST", dst);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", dst);
      if (hr.err[0]) {
        var_set_str(vm, "LAST_ERR", hr.err);
        var_set_str(vm, "ERR", hr.err);
      } else {
        var_set_str(vm, "LAST_ERR", "CPTREE: fail");
        var_set_str(vm, "ERR", "CPTREE: fail");
      }
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", hr.str[0] ? hr.str : dst);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str[0] ? hr.str : dst);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "CPTREE_N", hr.n);
    var_set_num(vm, "COPYTREE_N", hr.n);
    var_set_num(vm, "CPDIR_N", hr.n);
    var_set_num(vm, "CPTREE_BYTES", (long)hr.code);
    var_set_str(vm, "CPTREE", hr.str[0] ? hr.str : dst);
    var_set_str(vm, "COPYTREE", hr.str[0] ? hr.str : dst);
    var_set_str(vm, "PATH", hr.str[0] ? hr.str : dst);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# cptree → %s n=%ld bytes=%d\n",
              hr.str[0] ? hr.str : dst, hr.n, hr.code);
    bump(vm);
    return 1;
  }


  /* MVTREE|MOVETREE|MVDIR src dest — recursive move (rename or cptree+rmtree).
   * Twin of SYS MVTREE. LAST = abs dest · MVTREE_N · MVTREE_RENAMED 1|0.
   * Soft empty/missing · dest-inside-src refused.
   * Usability: promote nested agent workspaces without shell mv or CPTREE+RMTREE glue. */
  if (kw(&L->cur, "MVTREE") || kw(&L->cur, "MOVETREE") || kw(&L->cur, "MVDIR") ||
      kw(&L->cur, "MOVEDIR") || kw(&L->cur, "MV_TREE") || kw(&L->cur, "RELOCATETREE") ||
      kw(&L->cur, "PROMOVETREE") || kw(&L->cur, "TREEMV") || kw(&L->cur, "RMV") ||
      kw(&L->cur, "RENAMETREE")) {
    char src[CUBALC_HOST_STR_MAX], dst[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    lex_next(L);
    src[0] = 0;
    dst[0] = 0;
    if (resolve_str_arg(vm, L, src, sizeof src) != 0) {
      fail_at(vm, L, "MVTREE needs src dest — MVTREE \"work/a\" \"work/b\"");
      return -1;
    }
    if (resolve_str_arg(vm, L, dst, sizeof dst) != 0) {
      fail_at(vm, L, "MVTREE needs src dest — MVTREE \"work/a\" \"work/b\"");
      return -1;
    }
    if (!src[0] || !dst[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "MVTREE_N", 0);
      var_set_num(vm, "MOVETREE_N", 0);
      var_set_num(vm, "MVTREE_RENAMED", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "MVTREE: empty path");
      var_set_str(vm, "ERR", "MVTREE: empty path");
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_mvtree(src, dst, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "MVTREE_N", 0);
      var_set_num(vm, "MOVETREE_N", 0);
      var_set_num(vm, "MVTREE_RENAMED", 0);
      vm->last_n = 0;
      var_set_str(vm, "LAST", dst);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", dst);
      if (hr.err[0]) {
        var_set_str(vm, "LAST_ERR", hr.err);
        var_set_str(vm, "ERR", hr.err);
      } else {
        var_set_str(vm, "LAST_ERR", "MVTREE: fail");
        var_set_str(vm, "ERR", "MVTREE: fail");
      }
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", hr.str[0] ? hr.str : dst);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str[0] ? hr.str : dst);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "MVTREE_N", hr.n);
    var_set_num(vm, "MOVETREE_N", hr.n);
    var_set_num(vm, "MVDIR_N", hr.n);
    var_set_num(vm, "MVTREE_RENAMED", hr.code ? 1 : 0);
    var_set_num(vm, "MOVETREE_RENAMED", hr.code ? 1 : 0);
    var_set_str(vm, "MVTREE", hr.str[0] ? hr.str : dst);
    var_set_str(vm, "MOVETREE", hr.str[0] ? hr.str : dst);
    var_set_str(vm, "PATH", hr.str[0] ? hr.str : dst);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# mvtree → %s n=%ld renamed=%d\n",
              hr.str[0] ? hr.str : dst, hr.n, hr.code);
    bump(vm);
    return 1;
  }


  /* DIRSIZE|DU|TREESIZE path — recursive total file bytes under path.
   * Twin of SYS DIRSIZE. LAST = abs · LAST_N/DIRSIZE = bytes · DIRSIZE_FILES/DIRS.
   * Soft empty/missing. File → own size; dir → sum tree.
   * Usability: agent disk inventory without shell du or WALK+SIZEALL. */
  if (kw(&L->cur, "DIRSIZE") || kw(&L->cur, "DU") || kw(&L->cur, "TREESIZE") ||
      kw(&L->cur, "DISKUSAGE") || kw(&L->cur, "DIR_SIZE") || kw(&L->cur, "TREEBYTES") ||
      kw(&L->cur, "BYTESOF") || kw(&L->cur, "SIZEOFDIR") || kw(&L->cur, "DUSIZE") ||
      kw(&L->cur, "USAGE")) {
    char path[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    long files, dirs;
    lex_next(L);
    path[0] = 0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
      fail_at(vm, L, "DIRSIZE needs path — DIRSIZE \"work/tmp\"");
      return -1;
    }
    if (!path[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "DIRSIZE", 0);
      var_set_num(vm, "DIRSIZE_N", 0);
      var_set_num(vm, "DIRSIZE_FILES", 0);
      var_set_num(vm, "DIRSIZE_DIRS", 0);
      var_set_num(vm, "DU_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "DIRSIZE: empty path");
      var_set_str(vm, "ERR", "DIRSIZE: empty path");
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_dirsize(path, &hr) != 0) {
      var_set_num(vm, "OK", 0);
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "DIRSIZE", 0);
      var_set_num(vm, "DIRSIZE_N", 0);
      var_set_num(vm, "DIRSIZE_FILES", 0);
      var_set_num(vm, "DIRSIZE_DIRS", 0);
      var_set_num(vm, "DU_N", 0);
      vm->last_n = 0;
      var_set_str(vm, "LAST", path);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
      if (hr.err[0]) {
        var_set_str(vm, "LAST_ERR", hr.err);
        var_set_str(vm, "ERR", hr.err);
      } else {
        var_set_str(vm, "LAST_ERR", "DIRSIZE: fail");
        var_set_str(vm, "ERR", "DIRSIZE: fail");
      }
      bump(vm);
      return 1;
    }
    files = (long)((unsigned)hr.code & 0xffffu);
    dirs = (long)(((unsigned)hr.code >> 16) & 0xffffu);
    var_set_str(vm, "LAST", hr.str[0] ? hr.str : path);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str[0] ? hr.str : path);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "DIRSIZE", hr.n);
    var_set_num(vm, "DIRSIZE_N", hr.n);
    var_set_num(vm, "TREESIZE", hr.n);
    var_set_num(vm, "TREESIZE_N", hr.n);
    var_set_num(vm, "DU_N", hr.n);
    var_set_num(vm, "DIRSIZE_FILES", files);
    var_set_num(vm, "DIRSIZE_DIRS", dirs);
    var_set_num(vm, "DU_FILES", files);
    var_set_num(vm, "DU_DIRS", dirs);
    var_set_str(vm, "DIRSIZE_PATH", hr.str[0] ? hr.str : path);
    var_set_str(vm, "PATH", hr.str[0] ? hr.str : path);
    var_set_num(vm, "PATH_EXIST", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# dirsize → %s bytes=%ld files=%ld dirs=%ld\n",
              hr.str[0] ? hr.str : path, hr.n, files, dirs);
    bump(vm);
    return 1;
  }


  /* EQTREE|SAMETREE|CMPTREE a b — recursive structure + content equality.
   * Twin of SYS EQTREE. LAST_N/EQTREE = 1|0 · EQTREE_DIFF · EQTREE_REASON.
   * Soft empty/missing. Usability: verify CPTREE snapshots without shell diff -r. */
  if (kw(&L->cur, "EQTREE") || kw(&L->cur, "SAMETREE") || kw(&L->cur, "CMPTREE") ||
      kw(&L->cur, "TREEEQ") || kw(&L->cur, "EQDIR") || kw(&L->cur, "SAMEDIR") ||
      kw(&L->cur, "DIRSEQ") || kw(&L->cur, "IDENTICALTREE") || kw(&L->cur, "TREE_EQ") ||
      kw(&L->cur, "DIFFTREE")) {
    char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    long eq;
    lex_next(L);
    a[0] = 0;
    b[0] = 0;
    if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
      fail_at(vm, L, "EQTREE needs path_a path_b — EQTREE \"a\" \"b\"");
      return -1;
    }
    if (resolve_str_arg(vm, L, b, sizeof b) != 0) {
      fail_at(vm, L, "EQTREE needs path_a path_b — EQTREE \"a\" \"b\"");
      return -1;
    }
    if (!a[0] || !b[0]) {
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "EQTREE", 0);
      var_set_num(vm, "SAMETREE", 0);
      var_set_num(vm, "EQTREE_N", 0);
      var_set_str(vm, "EQTREE_DIFF", "");
      var_set_str(vm, "EQTREE_REASON", "empty");
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "0");
      vm->last_n = 0;
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "EQTREE: empty path");
      var_set_str(vm, "ERR", "EQTREE: empty path");
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_eqtree(a, b, &hr) != 0) {
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "EQTREE", 0);
      var_set_num(vm, "SAMETREE", 0);
      var_set_num(vm, "EQTREE_N", 0);
      var_set_str(vm, "EQTREE_DIFF", "");
      var_set_str(vm, "EQTREE_REASON", hr.err[0] ? hr.err : "fail");
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "0");
      vm->last_n = 0;
      var_set_num(vm, "OK", 0);
      if (hr.err[0]) {
        var_set_str(vm, "LAST_ERR", hr.err);
        var_set_str(vm, "ERR", hr.err);
      } else {
        var_set_str(vm, "LAST_ERR", "EQTREE: fail");
        var_set_str(vm, "ERR", "EQTREE: fail");
      }
      bump(vm);
      return 1;
    }
    eq = hr.n ? 1 : 0;
    vm->last_n = eq;
    var_set_num(vm, "LAST_N", eq);
    var_set_num(vm, "EQTREE", eq);
    var_set_num(vm, "SAMETREE", eq);
    var_set_num(vm, "CMPTREE", eq);
    var_set_num(vm, "EQTREE_N", eq);
    var_set_str(vm, "EQTREE_DIFF", eq ? "" : (hr.str[0] ? hr.str : "."));
    var_set_str(vm, "EQTREE_REASON", eq ? "" : (hr.err[0] ? hr.err : "differ"));
    {
      char buf[16];
      snprintf(buf, sizeof buf, "%ld", eq);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# eqtree → %ld diff=%s reason=%s\n",
              eq, eq ? "" : hr.str, eq ? "" : hr.err);
    bump(vm);
    return 1;
  }


  /* GREPTREE|SEARCHTREE|RGREP root needle — recursive content search under root.
   * Twin of SYS GREPTREE. LAST = matching paths bag · GREPTREE_N · GREPTREE_SCAN.
   * GREPTREEI case-insensitive. Soft empty/missing. Empty needle → all files.
   * Usability: agent content discovery without WALK+GREPFILES or shell grep -r. */
  if (kw(&L->cur, "GREPTREE") || kw(&L->cur, "SEARCHTREE") || kw(&L->cur, "RGREP") ||
      kw(&L->cur, "TREEGREP") || kw(&L->cur, "GREPDIR") || kw(&L->cur, "CONTENTTREE") ||
      kw(&L->cur, "GREPTREEI") || kw(&L->cur, "SEARCHTREEI") || kw(&L->cur, "RGREPI") ||
      kw(&L->cur, "TREEGREPI") || kw(&L->cur, "GREPDIRI") || kw(&L->cur, "FINDCONTENT") ||
      kw(&L->cur, "FINDIN")) {
    char root[CUBALC_HOST_STR_MAX], needle[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int icase = 0;
    if (kw(&L->cur, "GREPTREEI") || kw(&L->cur, "SEARCHTREEI") || kw(&L->cur, "RGREPI") ||
        kw(&L->cur, "TREEGREPI") || kw(&L->cur, "GREPDIRI"))
      icase = 1;
    lex_next(L);
    root[0] = 0;
    needle[0] = 0;
    if (resolve_str_arg(vm, L, root, sizeof root) != 0) {
      fail_at(vm, L, "GREPTREE needs root needle — GREPTREE \"work\" \"error\"");
      return -1;
    }
    if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
        L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(needle, sizeof needle, "%ld", nv);
      } else if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
        needle[0] = 0;
      }
    }
    if (!root[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "GREPTREE_N", 0);
      var_set_num(vm, "GREPTREE_SCAN", 0);
      var_set_num(vm, "SEARCHTREE_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "GREPTREE: empty path");
      var_set_str(vm, "ERR", "GREPTREE: empty path");
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_greptree(root, needle, icase, 0, &hr) != 0) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "GREPTREE_N", 0);
      var_set_num(vm, "GREPTREE_SCAN", 0);
      var_set_num(vm, "SEARCHTREE_N", 0);
      var_set_num(vm, "OK", 0);
      if (hr.err[0]) {
        var_set_str(vm, "LAST_ERR", hr.err);
        var_set_str(vm, "ERR", hr.err);
      } else {
        var_set_str(vm, "LAST_ERR", "GREPTREE: fail");
        var_set_str(vm, "ERR", "GREPTREE: fail");
      }
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "GREPTREE", hr.str);
    var_set_str(vm, "SEARCHTREE", hr.str);
    var_set_str(vm, "RGREP", hr.str);
    var_set_num(vm, "GREPTREE_N", hr.n);
    var_set_num(vm, "SEARCHTREE_N", hr.n);
    var_set_num(vm, "RGREP_N", hr.n);
    var_set_num(vm, "GREPTREE_SCAN", (long)hr.code);
    var_set_str(vm, "GREPTREE_ROOT", root);
    var_set_str(vm, "GREPTREE_NEEDLE", needle);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# greptree → hits=%ld scan=%d icase=%d\n",
              hr.n, hr.code, icase);
    bump(vm);
    return 1;
  }


  /* REPLACETREE|GSUBTREE|SEDTREE root old new — recursive REPLACEALL under root.
   * Twin of SYS REPLACETREE. LAST = rewritten path bag · REPLACETREE_N · REPLACETREE_SUBS.
   * Soft empty/missing · empty old → no-op. Only writes when content changes.
   * Usability: bulk nested plate rewrites without WALK+REPLACEFILES or shell find|sed. */
  if (kw(&L->cur, "REPLACETREE") || kw(&L->cur, "GSUBTREE") || kw(&L->cur, "SEDTREE") ||
      kw(&L->cur, "TREEREPLACE") || kw(&L->cur, "RPLTREE") || kw(&L->cur, "SUBALLTREE") ||
      kw(&L->cur, "REWRITETREE") || kw(&L->cur, "TREESED") || kw(&L->cur, "BULKTREE") ||
      kw(&L->cur, "SEDALLTREE")) {
    char root[CUBALC_HOST_STR_MAX], olds[CUBALC_HOST_STR_MAX], news[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    lex_next(L);
    root[0] = 0;
    olds[0] = 0;
    news[0] = 0;
    if (resolve_str_arg(vm, L, root, sizeof root) != 0) {
      fail_at(vm, L, "REPLACETREE needs root old new — REPLACETREE \"work\" \"OLD\" \"NEW\"");
      return -1;
    }
    if (resolve_str_arg(vm, L, olds, sizeof olds) != 0) {
      fail_at(vm, L, "REPLACETREE needs root old new — REPLACETREE \"work\" \"OLD\" \"NEW\"");
      return -1;
    }
    if (resolve_str_arg(vm, L, news, sizeof news) != 0)
      news[0] = 0;
    if (!root[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "REPLACETREE_N", 0);
      var_set_num(vm, "REPLACETREE_SUBS", 0);
      var_set_num(vm, "GSUBTREE_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "REPLACETREE: empty path");
      var_set_str(vm, "ERR", "REPLACETREE: empty path");
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_replacetree(root, olds, news, &hr) != 0) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "REPLACETREE_N", 0);
      var_set_num(vm, "REPLACETREE_SUBS", 0);
      var_set_num(vm, "GSUBTREE_N", 0);
      var_set_num(vm, "OK", 0);
      if (hr.err[0]) {
        var_set_str(vm, "LAST_ERR", hr.err);
        var_set_str(vm, "ERR", hr.err);
      } else {
        var_set_str(vm, "LAST_ERR", "REPLACETREE: fail");
        var_set_str(vm, "ERR", "REPLACETREE: fail");
      }
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "REPLACETREE", hr.str);
    var_set_str(vm, "GSUBTREE", hr.str);
    var_set_str(vm, "SEDTREE", hr.str);
    var_set_num(vm, "REPLACETREE_N", hr.n);
    var_set_num(vm, "GSUBTREE_N", hr.n);
    var_set_num(vm, "SEDTREE_N", hr.n);
    var_set_num(vm, "REPLACETREE_SUBS", (long)hr.code);
    var_set_num(vm, "GSUBTREE_SUBS", (long)hr.code);
    var_set_str(vm, "REPLACETREE_ROOT", root);
    var_set_str(vm, "REPLACETREE_OLD", olds);
    var_set_str(vm, "REPLACETREE_NEW", news);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# replacetree → files=%ld subs=%d\n", hr.n, hr.code);
    bump(vm);
    return 1;
  }

  /* COUNTINTREE|COUNTTREE|GREPCOUNTTREE root needle — recursive occurrence tally.
   * Twin of SYS COUNTINTREE. LAST/LAST_N = hits · COUNTINTREE_FILES = files with ≥1.
   * COUNTINTREEI case-insensitive. Soft empty/missing. Empty needle → 0.
   * Usability: severity/error rollups without WALK+COUNTINFILES or shell grep -rc. */
  if (kw(&L->cur, "COUNTINTREE") || kw(&L->cur, "COUNTTREE") || kw(&L->cur, "GREPCOUNTTREE") ||
      kw(&L->cur, "TREECOUNT") || kw(&L->cur, "OCCURTREE") || kw(&L->cur, "TOTALINTREE") ||
      kw(&L->cur, "COUNTINTREEI") || kw(&L->cur, "COUNTTREEI") || kw(&L->cur, "GREPCOUNTTREEI") ||
      kw(&L->cur, "TREECOUNTI") || kw(&L->cur, "OCCURTREEI")) {
    char root[CUBALC_HOST_STR_MAX], needle[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int icase = 0;
    if (kw(&L->cur, "COUNTINTREEI") || kw(&L->cur, "COUNTTREEI") ||
        kw(&L->cur, "GREPCOUNTTREEI") || kw(&L->cur, "TREECOUNTI") ||
        kw(&L->cur, "OCCURTREEI"))
      icase = 1;
    lex_next(L);
    root[0] = 0;
    needle[0] = 0;
    if (resolve_str_arg(vm, L, root, sizeof root) != 0) {
      fail_at(vm, L, "COUNTINTREE needs root needle — COUNTINTREE \"work\" \"error\"");
      return -1;
    }
    if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
        L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(needle, sizeof needle, "%ld", nv);
      } else if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
        needle[0] = 0;
      }
    }
    if (!root[0]) {
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "COUNTINTREE", 0);
      var_set_num(vm, "COUNTINTREE_N", 0);
      var_set_num(vm, "COUNTINTREE_FILES", 0);
      var_set_num(vm, "COUNTTREE_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "COUNTINTREE: empty path");
      var_set_str(vm, "ERR", "COUNTINTREE: empty path");
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_countintree(root, needle, icase, &hr) != 0) {
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "COUNTINTREE", 0);
      var_set_num(vm, "COUNTINTREE_N", 0);
      var_set_num(vm, "COUNTINTREE_FILES", 0);
      var_set_num(vm, "COUNTTREE_N", 0);
      var_set_num(vm, "OK", 0);
      if (hr.err[0]) {
        var_set_str(vm, "LAST_ERR", hr.err);
        var_set_str(vm, "ERR", hr.err);
      } else {
        var_set_str(vm, "LAST_ERR", "COUNTINTREE: fail");
        var_set_str(vm, "ERR", "COUNTINTREE: fail");
      }
      bump(vm);
      return 1;
    }
    {
      char buf[32];
      snprintf(buf, sizeof buf, "%ld", hr.n);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
    }
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "COUNTINTREE", hr.n);
    var_set_num(vm, "COUNTINTREE_N", hr.n);
    var_set_num(vm, "COUNTTREE", hr.n);
    var_set_num(vm, "COUNTTREE_N", hr.n);
    var_set_num(vm, "GREPCOUNTTREE", hr.n);
    var_set_num(vm, "TREECOUNT", hr.n);
    var_set_num(vm, "COUNTINTREE_FILES", (long)hr.code);
    var_set_num(vm, "COUNTTREE_FILES", (long)hr.code);
    var_set_str(vm, "COUNTINTREE_ROOT", hr.str[0] ? hr.str : root);
    var_set_str(vm, "COUNTINTREE_NEEDLE", needle);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# countintree → hits=%ld files=%d icase=%d\n",
              hr.n, hr.code, icase);
    bump(vm);
    return 1;
  }

  /* GREPLINESTREE|EXTRACTLINETREE|HARVESTTREE root needle — recursive line harvest.
   * Twin of SYS GREPLINESTREE. LAST = line bag · GREPLINESTREE_N · GREPLINESTREE_FILES.
   * GREPLINESTREEI case-insensitive. Soft empty/missing. Empty needle → all lines.
   * Usability: log harvest under a tree without WALK+GREPLINES or shell grep -rn. */
  if (kw(&L->cur, "GREPLINESTREE") || kw(&L->cur, "EXTRACTLINETREE") ||
      kw(&L->cur, "TREEGREPLINES") || kw(&L->cur, "HARVESTTREE") ||
      kw(&L->cur, "LINETREE") || kw(&L->cur, "GREPLINEST") ||
      kw(&L->cur, "GREPLINESTREEI") || kw(&L->cur, "EXTRACTLINETREEI") ||
      kw(&L->cur, "TREEGREPLINESI") || kw(&L->cur, "HARVESTTREEI") ||
      kw(&L->cur, "LINETREEI")) {
    char root[CUBALC_HOST_STR_MAX], needle[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int icase = 0;
    if (kw(&L->cur, "GREPLINESTREEI") || kw(&L->cur, "EXTRACTLINETREEI") ||
        kw(&L->cur, "TREEGREPLINESI") || kw(&L->cur, "HARVESTTREEI") ||
        kw(&L->cur, "LINETREEI"))
      icase = 1;
    lex_next(L);
    root[0] = 0;
    needle[0] = 0;
    if (resolve_str_arg(vm, L, root, sizeof root) != 0) {
      fail_at(vm, L, "GREPLINESTREE needs root needle — GREPLINESTREE \"work\" \"error\"");
      return -1;
    }
    if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
        L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(needle, sizeof needle, "%ld", nv);
      } else if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
        needle[0] = 0;
      }
    }
    if (!root[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "GREPLINESTREE_N", 0);
      var_set_num(vm, "GREPLINESTREE_FILES", 0);
      var_set_num(vm, "EXTRACTLINETREE_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "GREPLINESTREE: empty path");
      var_set_str(vm, "ERR", "GREPLINESTREE: empty path");
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_greplines_tree(root, needle, icase, &hr) != 0) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "GREPLINESTREE_N", 0);
      var_set_num(vm, "GREPLINESTREE_FILES", 0);
      var_set_num(vm, "EXTRACTLINETREE_N", 0);
      var_set_num(vm, "OK", 0);
      if (hr.err[0]) {
        var_set_str(vm, "LAST_ERR", hr.err);
        var_set_str(vm, "ERR", hr.err);
      } else {
        var_set_str(vm, "LAST_ERR", "GREPLINESTREE: fail");
        var_set_str(vm, "ERR", "GREPLINESTREE: fail");
      }
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hr.n;
    var_set_num(vm, "LAST_N", hr.n);
    var_set_str(vm, "GREPLINESTREE", hr.str);
    var_set_str(vm, "EXTRACTLINETREE", hr.str);
    var_set_str(vm, "HARVESTTREE", hr.str);
    var_set_num(vm, "GREPLINESTREE_N", hr.n);
    var_set_num(vm, "EXTRACTLINETREE_N", hr.n);
    var_set_num(vm, "HARVESTTREE_N", hr.n);
    var_set_num(vm, "GREPLINESTREE_FILES", (long)hr.code);
    var_set_num(vm, "EXTRACTLINETREE_FILES", (long)hr.code);
    var_set_str(vm, "GREPLINESTREE_ROOT", root);
    var_set_str(vm, "GREPLINESTREE_NEEDLE", needle);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# greplinestree → lines=%ld files=%d icase=%d\n",
              hr.n, hr.code, icase);
    bump(vm);
    return 1;
  }

  /* FIRSTINTREE|GREP1TREE|LASTINTREE root needle — first/last content-matching file.
   * Twin of SYS FIRSTINTREE. LAST = path · LAST_N/HIT 0|1. FIRSTINTREEI/LASTINTREEI icase.
   * Soft empty/missing. Empty needle → first/last readable regular file.
   * Usability: one-shot pick under tree without GREPTREE+TAKE/REVL glue. */
  if (kw(&L->cur, "FIRSTINTREE") || kw(&L->cur, "GREP1TREE") || kw(&L->cur, "HITINTREE") ||
      kw(&L->cur, "FINDINTREE") || kw(&L->cur, "TREEFIRST") ||
      kw(&L->cur, "FIRSTINTREEI") || kw(&L->cur, "GREP1TREEI") || kw(&L->cur, "HITINTREEI") ||
      kw(&L->cur, "LASTINTREE") || kw(&L->cur, "GREP1TREEL") || kw(&L->cur, "HITINTREEL") ||
      kw(&L->cur, "TREELAST") || kw(&L->cur, "LASTINTREEI") || kw(&L->cur, "GREP1TREELI") ||
      kw(&L->cur, "HITINTREELI")) {
    char root[CUBALC_HOST_STR_MAX], needle[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int icase = 0, want_last = 0;
    long hit;
    if (kw(&L->cur, "LASTINTREE") || kw(&L->cur, "GREP1TREEL") || kw(&L->cur, "HITINTREEL") ||
        kw(&L->cur, "TREELAST") || kw(&L->cur, "LASTINTREEI") || kw(&L->cur, "GREP1TREELI") ||
        kw(&L->cur, "HITINTREELI"))
      want_last = 1;
    if (kw(&L->cur, "FIRSTINTREEI") || kw(&L->cur, "GREP1TREEI") || kw(&L->cur, "HITINTREEI") ||
        kw(&L->cur, "LASTINTREEI") || kw(&L->cur, "GREP1TREELI") || kw(&L->cur, "HITINTREELI"))
      icase = 1;
    lex_next(L);
    root[0] = 0;
    needle[0] = 0;
    if (resolve_str_arg(vm, L, root, sizeof root) != 0) {
      fail_at(vm, L, want_last
                        ? "LASTINTREE needs root needle — LASTINTREE \"work\" \"error\""
                        : "FIRSTINTREE needs root needle — FIRSTINTREE \"work\" \"error\"");
      return -1;
    }
    if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
        L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(needle, sizeof needle, "%ld", nv);
      } else if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
        needle[0] = 0;
      }
    }
    if (!root[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "FIRSTINTREE_N", 0);
      var_set_num(vm, "FIRSTINTREE_HIT", 0);
      var_set_num(vm, "LASTINTREE_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", want_last ? "LASTINTREE: empty path" : "FIRSTINTREE: empty path");
      var_set_str(vm, "ERR", want_last ? "LASTINTREE: empty path" : "FIRSTINTREE: empty path");
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_findintree(root, needle, icase, want_last, &hr) != 0) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "FIRSTINTREE_N", 0);
      var_set_num(vm, "FIRSTINTREE_HIT", 0);
      var_set_num(vm, "LASTINTREE_N", 0);
      var_set_num(vm, "OK", 0);
      if (hr.err[0]) {
        var_set_str(vm, "LAST_ERR", hr.err);
        var_set_str(vm, "ERR", hr.err);
      } else {
        var_set_str(vm, "LAST_ERR", want_last ? "LASTINTREE: fail" : "FIRSTINTREE: fail");
        var_set_str(vm, "ERR", want_last ? "LASTINTREE: fail" : "FIRSTINTREE: fail");
      }
      bump(vm);
      return 1;
    }
    hit = hr.n ? 1 : 0;
    var_set_str(vm, "LAST", hr.str);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_n = hit;
    var_set_num(vm, "LAST_N", hit);
    if (want_last) {
      var_set_str(vm, "LASTINTREE", hr.str);
      var_set_str(vm, "GREP1TREEL", hr.str);
      var_set_num(vm, "LASTINTREE_N", hit);
      var_set_num(vm, "LASTINTREE_HIT", hit);
    } else {
      var_set_str(vm, "FIRSTINTREE", hr.str);
      var_set_str(vm, "GREP1TREE", hr.str);
      var_set_num(vm, "FIRSTINTREE_N", hit);
      var_set_num(vm, "FIRSTINTREE_HIT", hit);
    }
    var_set_str(vm, "PATH", hr.str);
    var_set_num(vm, "PATH_EXIST", hit);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %sintree → hit=%ld path=%s icase=%d\n",
              want_last ? "last" : "first", hit, hr.str, icase);
    bump(vm);
    return 1;
  }

  /* HASINTREE|ANYINTREE|CONTAINSINTREE root needle — soft 0|1 content-exists probe.
   * Twin of SYS HASINTREE. LAST_N/HIT 0|1 · PATH on hit. Does NOT clobber LAST str.
   * HASINTREEI icase. Soft empty/missing. Usability: IF guards without FIRSTINTREE+HIT. */
  if (kw(&L->cur, "HASINTREE") || kw(&L->cur, "ANYINTREE") ||
      kw(&L->cur, "CONTAINSINTREE") || kw(&L->cur, "TREEHAS") ||
      kw(&L->cur, "HASTREE") || kw(&L->cur, "ANYINTREECONTENT") ||
      kw(&L->cur, "HASINTREEI") || kw(&L->cur, "ANYINTREEI") ||
      kw(&L->cur, "CONTAINSINTREEI") || kw(&L->cur, "TREEHASI")) {
    char root[CUBALC_HOST_STR_MAX], needle[CUBALC_HOST_STR_MAX];
    cubalc_host_result hr;
    int icase = 0;
    long hit;
    if (kw(&L->cur, "HASINTREEI") || kw(&L->cur, "ANYINTREEI") ||
        kw(&L->cur, "CONTAINSINTREEI") || kw(&L->cur, "TREEHASI"))
      icase = 1;
    lex_next(L);
    root[0] = 0;
    needle[0] = 0;
    if (resolve_str_arg(vm, L, root, sizeof root) != 0) {
      fail_at(vm, L, "HASINTREE needs root needle — HASINTREE \"work\" \"error\"");
      return -1;
    }
    if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
        L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(needle, sizeof needle, "%ld", nv);
      } else if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
        needle[0] = 0;
      }
    }
    if (!root[0]) {
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASINTREE_N", 0);
      var_set_num(vm, "HASINTREE_HIT", 0);
      var_set_str(vm, "PATH", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASINTREE: empty path");
      var_set_str(vm, "ERR", "HASINTREE: empty path");
      bump(vm);
      return 1;
    }
    memset(&hr, 0, sizeof hr);
    if (cubalc_host_findintree(root, needle, icase, 0, &hr) != 0) {
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASINTREE_N", 0);
      var_set_num(vm, "HASINTREE_HIT", 0);
      var_set_str(vm, "PATH", "");
      var_set_num(vm, "OK", 0);
      if (hr.err[0]) {
        var_set_str(vm, "LAST_ERR", hr.err);
        var_set_str(vm, "ERR", hr.err);
      } else {
        var_set_str(vm, "LAST_ERR", "HASINTREE: fail");
        var_set_str(vm, "ERR", "HASINTREE: fail");
      }
      bump(vm);
      return 1;
    }
    hit = hr.n ? 1 : 0;
    vm->last_n = hit;
    var_set_num(vm, "LAST_N", hit);
    var_set_num(vm, "HASINTREE_N", hit);
    var_set_num(vm, "HASINTREE_HIT", hit);
    var_set_num(vm, "ANYINTREE_N", hit);
    var_set_str(vm, "PATH", hit ? hr.str : "");
    var_set_num(vm, "PATH_EXIST", hit);
    if (hit) {
      var_set_str(vm, "HASINTREE_PATH", hr.str);
      var_set_str(vm, "ANYINTREE_PATH", hr.str);
    } else {
      var_set_str(vm, "HASINTREE_PATH", "");
      var_set_str(vm, "ANYINTREE_PATH", "");
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# hasintree → hit=%ld path=%s icase=%d\n",
              hit, hit ? hr.str : "", icase);
    bump(vm);
    return 1;
  }

  return 0;
}
