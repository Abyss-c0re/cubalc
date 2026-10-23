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

int cubalc_lang_ops_core_try_flagpath(VM *vm, Lex *L) {
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
  return 0;
}
