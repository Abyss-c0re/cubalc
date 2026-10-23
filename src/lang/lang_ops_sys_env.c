/* CubalC lang — lang_ops_sys_env.c
 * SYS ENV / process environment plane (A2 theme peel).
 * Mesh vote A2_theme_peel_core_sys · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_sys_env.h"
#include "lang/cubalc_lang_ops_sys_plate_expand.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <unistd.h>
#endif

int cubalc_lang_ops_sys_try_env(VM *vm, Lex *L) {
  /* ENV/SETENV/DOTENV/SUBSTENV plane.
   * Caller: after SYS keyword + plate/path_fs miss; L at subcommand.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
    if (kw(&L->cur,"ENV") || kw(&L->cur,"SETENV") || kw(&L->cur,"EXPORT") ||
        kw(&L->cur,"UNSETENV") || kw(&L->cur,"ENVUNSET")){
      /* SYS ENV name [OR fallback] — get.
       * SYS ENV SET|PUT name value | SYS SETENV|EXPORT — process setenv (agent CUBALC_*).
       * SYS ENV UNSET|CLEAR name | SYS UNSETENV — unsetenv · LAST_N 1 if was set. */
      char op[16]; snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *p = op; *p; p++)
        if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 'a' + 'A');
      int is_set = (strcmp(op, "SETENV") == 0 || strcmp(op, "EXPORT") == 0);
      int is_unset = (strcmp(op, "UNSETENV") == 0 || strcmp(op, "ENVUNSET") == 0);
      lex_next(L);
      if (!is_set && !is_unset && strcmp(op, "ENV") == 0) {
        if (kw(&L->cur,"SET") || kw(&L->cur,"PUT") || kw(&L->cur,"EXPORT") ||
            kw(&L->cur,"WRITE") || kw(&L->cur,"ASSIGN")){
          is_set = 1;
          lex_next(L);
        } else if (kw(&L->cur,"UNSET") || kw(&L->cur,"CLEAR") || kw(&L->cur,"DELETE") ||
                   kw(&L->cur,"RM") || kw(&L->cur,"DROP")){
          is_unset = 1;
          lex_next(L);
        }
      }
      if (is_set) {
        char name[256] = "", val[CUBALC_HOST_STR_MAX];
        cubalc_host_result hr;
        if (resolve_str_arg(vm, L, name, sizeof name) != 0) {
          if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
            snprintf(name, sizeof name, "%s", L->cur.text);
            lex_next(L);
          } else {
            fail(vm, "SYS ENV SET name value"); return -1;
          }
        }
        val[0] = 0;
        if (resolve_str_arg(vm, L, val, sizeof val) != 0) {
          /* bare number → string; else empty */
          if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
            long n = parse_expr(vm, L);
            snprintf(val, sizeof val, "%ld", n);
          } else if (L->cur.kind == TK_IDENT) {
            Var *v = var_get(vm, L->cur.text, 0);
            if (v && !v->is_str) {
              snprintf(val, sizeof val, "%ld", v->val);
              lex_next(L);
            } else {
              fail(vm, "SYS ENV SET name value"); return -1;
            }
          }
          /* else empty value */
        }
        if (cubalc_host_env_set(name, val, &hr) != 0) {
          fail(vm, hr.err[0] ? hr.err : "SYS ENV SET failed"); return -1;
        }
        snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
        vm->last_n = hr.n;
        var_set_str(vm, "LAST", hr.str);
        var_set_num(vm, "LAST_N", hr.n);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      if (is_unset) {
        char name[256] = "";
        cubalc_host_result hr;
        if (resolve_str_arg(vm, L, name, sizeof name) != 0) {
          if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
            snprintf(name, sizeof name, "%s", L->cur.text);
            lex_next(L);
          } else {
            fail(vm, "SYS ENV UNSET name"); return -1;
          }
        }
        if (cubalc_host_env_unset(name, &hr) != 0) {
          fail(vm, hr.err[0] ? hr.err : "SYS ENV UNSET failed"); return -1;
        }
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        vm->last_n = hr.n;
        var_set_str(vm, "LAST", "");
        var_set_num(vm, "LAST_N", hr.n);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      /* get: SYS ENV name [OR fallback]
       * name: "lit" | LAST | string-var | bare IDENT env key.
       * Usability: SYS ENV k with k="ROLE" must resolve sval, not key "k". */
      char ename[256];
      cubalc_host_result hr;
      ename[0] = 0;
      if (L->cur.kind == TK_STR) {
        snprintf(ename, sizeof ename, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        if (strcmp(L->cur.text, "LAST") == 0) {
          snprintf(ename, sizeof ename, "%s", vm->last_str);
          lex_next(L);
        } else {
          Var *ev = var_get(vm, L->cur.text, 0);
          if (ev && ev->is_str && ev->sval[0]) {
            snprintf(ename, sizeof ename, "%s", ev->sval);
            lex_next(L);
          } else {
            /* bare unquoted env key: SYS ENV CUBALC_STATE */
            snprintf(ename, sizeof ename, "%s", L->cur.text);
            lex_next(L);
          }
        }
      } else {
        fail(vm, "SYS ENV name");
        return -1;
      }
      cubalc_host_env(ename, &hr);
      if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
          kw(&L->cur,"FALLBACK")){
        lex_next(L);
        char fb[512];
        if (resolve_str_arg(vm, L, fb, sizeof fb) != 0){
          fail(vm,"SYS ENV name OR \"fallback\""); return -1;
        }
        if (!hr.str[0] || hr.n <= 0){
          snprintf(hr.str, sizeof hr.str, "%s", fb);
          hr.n = (long)strlen(hr.str);
          hr.ok = 1;
        }
      }
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_str(vm, "LAST", hr.str);
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS ENVDEFAULT|ENSUREENV|DEFAULTENV|SETENVIF name value
     * — set process env only if name missing or empty (agent boot defaults).
     * LAST = final value; LAST_N / ENVDEFAULT_N = 1 if wrote, 0 if kept existing.
     * ENVDEFAULT_SET mirrors LAST_N. Soft fail empty name OK=0.
     * Usability: DEFAULT for host env without IF HASENV + SETENV glue.
     * Distinct from SYS ENV OR (read-only fallback) and SETENV (always overwrite). */
    if (kw(&L->cur,"ENVDEFAULT") || kw(&L->cur,"ENSUREENV") || kw(&L->cur,"DEFAULTENV") ||
        kw(&L->cur,"SETENVIF") || kw(&L->cur,"SETENVDEFAULT") || kw(&L->cur,"ENVENSURE") ||
        kw(&L->cur,"PUTENVIF") || kw(&L->cur,"ENV_DEFAULT") || kw(&L->cur,"DEFAULT_ENV") ||
        kw(&L->cur,"ENSURE_ENV") || kw(&L->cur,"ENVIFUNSET")){
      char name[256] = "", val[CUBALC_HOST_STR_MAX];
      const char *cur;
      int wrote = 0;
      cubalc_host_result hr;
      lex_next(L);
      if (resolve_str_arg(vm, L, name, sizeof name) != 0) {
        if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
          snprintf(name, sizeof name, "%s", L->cur.text);
          lex_next(L);
        } else {
          fail(vm, "SYS ENVDEFAULT name value"); return -1;
        }
      }
      if (!name[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "ENVDEFAULT_N", 0);
        var_set_num(vm, "ENVDEFAULT_SET", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "ENVDEFAULT: empty name");
        var_set_str(vm, "ERR", "ENVDEFAULT: empty name");
        bump(vm); return 1;
      }
      val[0] = 0;
      if (resolve_str_arg(vm, L, val, sizeof val) != 0) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long n = parse_expr(vm, L);
          snprintf(val, sizeof val, "%ld", n);
        } else if (L->cur.kind == TK_IDENT) {
          Var *v = var_get(vm, L->cur.text, 0);
          if (v && !v->is_str) {
            snprintf(val, sizeof val, "%ld", v->val);
            lex_next(L);
          } else if (v && v->is_str) {
            snprintf(val, sizeof val, "%s", v->sval);
            lex_next(L);
          } else {
            fail(vm, "SYS ENVDEFAULT name value"); return -1;
          }
        }
        /* else empty default value */
      }
      cur = getenv(name);
      if (cur && cur[0]) {
        /* keep existing non-empty */
        wrote = 0;
        snprintf(vm->last_str, sizeof vm->last_str, "%s", cur);
        var_set_str(vm, "LAST", cur);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "ENVDEFAULT_N", 0);
        var_set_num(vm, "ENVDEFAULT_SET", 0);
        var_set_str(vm, "ENVDEFAULT", cur);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      if (cubalc_host_env_set(name, val, &hr) != 0) {
        fail(vm, hr.err[0] ? hr.err : "SYS ENVDEFAULT set failed"); return -1;
      }
      wrote = 1;
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str[0] ? hr.str : val);
      var_set_str(vm, "LAST", vm->last_str);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "ENVDEFAULT_N", 1);
      var_set_num(vm, "ENVDEFAULT_SET", 1);
      var_set_str(vm, "ENVDEFAULT", vm->last_str);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS DOTENV|LOADENV|ENVFILE|SOURCEENV path
     * — load dotenv-style KEY=VAL lines from a plate into process env.
     * Skips blank lines and # comments. Optional "export " prefix.
     * Strips matching outer ' or " around values. Overwrites existing.
     * LAST = path; LAST_N / DOTENV_N = keys set; DOTENV_SKIP = skipped lines.
     * Soft miss empty/missing/unreadable → OK=0 sticky LAST_ERR.
     * Usability: agent .env / plate config without shell source or EACH+SETENV. */
    if (kw(&L->cur,"DOTENV") || kw(&L->cur,"LOADENV") || kw(&L->cur,"ENVFILE") ||
        kw(&L->cur,"SOURCEENV") || kw(&L->cur,"ENVLOAD") || kw(&L->cur,"READENV") ||
        kw(&L->cur,"FROMENVFILE") || kw(&L->cur,"IMPORTENV") ||
        kw(&L->cur,"LOADENVFILE") || kw(&L->cur,"DOTENVLOAD")){
      char path[512];
      cubalc_host_result hr, er;
      const char *p, *end;
      long set_n = 0, skip_n = 0;
      lex_next(L);
      path[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0)
        path[0] = 0;
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DOTENV_N", 0);
        var_set_num(vm, "LOADENV_N", 0);
        var_set_num(vm, "DOTENV_SKIP", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "DOTENV: empty path");
        var_set_str(vm, "ERR", "DOTENV: empty path");
        vm->last_n = 0;
        vm->last_str[0] = 0;
        bump(vm); return 1;
      }
      if (cubalc_host_read(path, &hr) != 0) {
        char err[160];
        var_set_str(vm, "LAST", path);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DOTENV_N", 0);
        var_set_num(vm, "LOADENV_N", 0);
        var_set_num(vm, "DOTENV_SKIP", 0);
        var_set_num(vm, "OK", 0);
        snprintf(err, sizeof err, "DOTENV: read fail '%s'", path);
        var_set_str(vm, "LAST_ERR", err);
        var_set_str(vm, "ERR", err);
        bump(vm); return 1;
      }
      p = hr.str;
      end = hr.str + (hr.n > 0 ? (size_t)hr.n : strlen(hr.str));
      while (p < end) {
        const char *line = p, *nl;
        char name[256], val[CUBALC_HOST_STR_MAX];
        size_t llen;
        const char *eq, *np, *vp, *vend;
        while (p < end && *p != '\n') p++;
        nl = p;
        if (p < end && *p == '\n') p++;
        /* trim CR */
        llen = (size_t)(nl - line);
        while (llen > 0 && (line[llen - 1] == '\r' || line[llen - 1] == ' ' ||
                            line[llen - 1] == '\t'))
          llen--;
        /* skip leading ws */
        while (llen > 0 && (*line == ' ' || *line == '\t')) {
          line++;
          llen--;
        }
        if (llen == 0) { skip_n++; continue; }
        if (line[0] == '#') { skip_n++; continue; }
        /* optional export PREFIX */
        if (llen >= 7 &&
            (line[0] == 'e' || line[0] == 'E') &&
            (line[1] == 'x' || line[1] == 'X') &&
            (line[2] == 'p' || line[2] == 'P') &&
            (line[3] == 'o' || line[3] == 'O') &&
            (line[4] == 'r' || line[4] == 'R') &&
            (line[5] == 't' || line[5] == 'T') &&
            (line[6] == ' ' || line[6] == '\t')) {
          line += 7;
          llen -= 7;
          while (llen > 0 && (*line == ' ' || *line == '\t')) {
            line++;
            llen--;
          }
        }
        /* find = */
        eq = NULL;
        for (np = line; np < line + llen; np++) {
          if (*np == '=') { eq = np; break; }
        }
        if (!eq) { skip_n++; continue; }
        /* name */
        {
          size_t nlen = (size_t)(eq - line);
          while (nlen > 0 && (line[nlen - 1] == ' ' || line[nlen - 1] == '\t'))
            nlen--;
          if (nlen == 0 || nlen >= sizeof name) { skip_n++; continue; }
          /* valid env name: [A-Za-z_][A-Za-z0-9_]* */
          if (!( (line[0] >= 'A' && line[0] <= 'Z') ||
                 (line[0] >= 'a' && line[0] <= 'z') || line[0] == '_' )) {
            skip_n++; continue;
          }
          {
            size_t i;
            int bad = 0;
            for (i = 1; i < nlen; i++) {
              char c = line[i];
              if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                    (c >= '0' && c <= '9') || c == '_')) {
                bad = 1; break;
              }
            }
            if (bad) { skip_n++; continue; }
          }
          memcpy(name, line, nlen);
          name[nlen] = 0;
        }
        /* value */
        vp = eq + 1;
        vend = line + llen;
        while (vp < vend && (*vp == ' ' || *vp == '\t')) vp++;
        {
          size_t vlen = (size_t)(vend - vp);
          if (vlen >= sizeof val) vlen = sizeof val - 1;
          memcpy(val, vp, vlen);
          val[vlen] = 0;
          /* strip matching outer quotes */
          if (vlen >= 2 &&
              ((val[0] == '"' && val[vlen - 1] == '"') ||
               (val[0] == '\'' && val[vlen - 1] == '\''))) {
            memmove(val, val + 1, vlen - 2);
            val[vlen - 2] = 0;
          }
          /* light unescape for common \\ \" \' \n \t in remaining body */
          {
            char *w = val, *r = val;
            while (*r) {
              if (*r == '\\' && r[1]) {
                char nch = r[1];
                if (nch == 'n') { *w++ = '\n'; r += 2; continue; }
                if (nch == 't') { *w++ = '\t'; r += 2; continue; }
                if (nch == 'r') { *w++ = '\r'; r += 2; continue; }
                if (nch == '\\' || nch == '"' || nch == '\'') {
                  *w++ = nch; r += 2; continue;
                }
              }
              *w++ = *r++;
            }
            *w = 0;
          }
        }
        if (cubalc_host_env_set(name, val, &er) != 0) {
          skip_n++;
          continue;
        }
        set_n++;
      }
      var_set_str(vm, "LAST", path);
      var_set_str(vm, "DOTENV", path);
      var_set_str(vm, "LOADENV", path);
      var_set_str(vm, "ENVFILE", path);
      var_set_str(vm, "DOTENV_PATH", path);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
      vm->last_n = set_n;
      var_set_num(vm, "LAST_N", set_n);
      var_set_num(vm, "DOTENV_N", set_n);
      var_set_num(vm, "LOADENV_N", set_n);
      var_set_num(vm, "ENVFILE_N", set_n);
      var_set_num(vm, "DOTENV_SKIP", skip_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS DUMPENV|ENVALL|ENVDUMP|LISTENV [prefix]
     * — dump process environ as KEY=VAL newline bag (printenv-style).
     * Optional prefix: keep only names that start with prefix (e.g. CUBALC_).
     * LAST = bag; LAST_N / DUMPENV_N = count; DUMPENV_TRUNC=1 if bag capped.
     * Empty/no-match → LAST="" LAST_N=0 OK=1 (soft empty, not error).
     * Usability: agent env inventory without shell env|grep; dual of DOTENV. */
    if (kw(&L->cur,"DUMPENV") || kw(&L->cur,"ENVALL") || kw(&L->cur,"ENVDUMP") ||
        kw(&L->cur,"LISTENV") || kw(&L->cur,"PRINTENV") || kw(&L->cur,"ENVLIST") ||
        kw(&L->cur,"SHOWENV") || kw(&L->cur,"ENVBAG") || kw(&L->cur,"GETENVALL") ||
        kw(&L->cur,"ENVKEYS")){
      char prefix[256], bag[CUBALC_HOST_STR_MAX];
      long n = 0, trunc = 0;
      size_t bo = 0;
      size_t plen = 0;
#if !defined(CUBALC_OS_WINDOWS)
      extern char **environ;
      char **ep;
#endif
      lex_next(L);
      prefix[0] = 0;
      /* optional prefix: "CUBALC_" | LAST | string-var */
      if (resolve_str_arg(vm, L, prefix, sizeof prefix) == 0)
        plen = strlen(prefix);
      bag[0] = 0;
#if !defined(CUBALC_OS_WINDOWS)
      if (environ) {
        for (ep = environ; *ep; ep++) {
          const char *e = *ep;
          size_t elen;
          const char *eq;
          size_t klen;
          if (!e || !e[0]) continue;
          eq = strchr(e, '=');
          if (!eq) continue;
          klen = (size_t)(eq - e);
          if (plen > 0) {
            if (klen < plen) continue;
            if (strncmp(e, prefix, plen) != 0) continue;
          }
          elen = strlen(e);
          /* room for line + optional \n + NUL */
          if (bo + elen + 2 > sizeof bag) {
            trunc = 1;
            break;
          }
          if (bo > 0)
            bag[bo++] = '\n';
          memcpy(bag + bo, e, elen);
          bo += elen;
          bag[bo] = 0;
          n++;
        }
      }
#else
      /* Windows: soft empty inventory (no environ walk in this slice). */
      (void)plen;
#endif
      var_set_str(vm, "LAST", bag);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "DUMPENV_N", n);
      var_set_num(vm, "ENVALL_N", n);
      var_set_num(vm, "ENVDUMP_N", n);
      var_set_num(vm, "LISTENV_N", n);
      var_set_num(vm, "DUMPENV_TRUNC", trunc);
      var_set_str(vm, "DUMPENV", bag);
      var_set_str(vm, "ENVALL", bag);
      var_set_str(vm, "DUMPENV_PREFIX", prefix);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS WRITEENV|SAVENV|DUMPENVFILE|ENVSAVE path [prefix]
     * — write process environ as KEY=VAL lines into a dotenv plate file.
     * Optional prefix filter (same as DUMPENV). Empty match → empty file OK.
     * LAST = path; LAST_N / WRITEENV_N = keys written; WRITEENV_BYTES = size.
     * Soft miss empty path / write fail → OK=0 sticky LAST_ERR.
     * Usability: persist agent config for peers without shell env > file;
     * round-trip with DOTENV (load) + DUMPENV (inspect). */
    if (kw(&L->cur,"WRITEENV") || kw(&L->cur,"SAVENV") || kw(&L->cur,"DUMPENVFILE") ||
        kw(&L->cur,"ENVSAVE") || kw(&L->cur,"SAVEENV") || kw(&L->cur,"ENVWRITE") ||
        kw(&L->cur,"EXPORTENV") || kw(&L->cur,"ENVEXPORT") || kw(&L->cur,"PUTENVFILE") ||
        kw(&L->cur,"STOREENV")){
      char path[512], prefix[256], bag[CUBALC_HOST_STR_MAX];
      long n = 0;
      size_t bo = 0, plen = 0;
      cubalc_host_result wr;
#if !defined(CUBALC_OS_WINDOWS)
      extern char **environ;
      char **ep;
#endif
      lex_next(L);
      path[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0)
        path[0] = 0;
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "WRITEENV_N", 0);
        var_set_num(vm, "SAVENV_N", 0);
        var_set_num(vm, "WRITEENV_BYTES", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "WRITEENV: empty path");
        var_set_str(vm, "ERR", "WRITEENV: empty path");
        vm->last_n = 0;
        vm->last_str[0] = 0;
        bump(vm); return 1;
      }
      prefix[0] = 0;
      if (resolve_str_arg(vm, L, prefix, sizeof prefix) == 0)
        plen = strlen(prefix);
      bag[0] = 0;
#if !defined(CUBALC_OS_WINDOWS)
      if (environ) {
        for (ep = environ; *ep; ep++) {
          const char *e = *ep;
          size_t elen, klen;
          const char *eq;
          if (!e || !e[0]) continue;
          eq = strchr(e, '=');
          if (!eq) continue;
          klen = (size_t)(eq - e);
          if (plen > 0) {
            if (klen < plen) continue;
            if (strncmp(e, prefix, plen) != 0) continue;
          }
          elen = strlen(e);
          if (bo + elen + 2 > sizeof bag)
            break;
          if (bo > 0)
            bag[bo++] = '\n';
          memcpy(bag + bo, e, elen);
          bo += elen;
          bag[bo] = 0;
          n++;
        }
      }
#else
      (void)plen;
#endif
      /* trailing newline for posix text plates when non-empty */
      if (bo > 0 && bo + 2 <= sizeof bag) {
        bag[bo++] = '\n';
        bag[bo] = 0;
      }
      if (cubalc_host_write(path, bag, &wr) != 0) {
        char err[160];
        var_set_str(vm, "LAST", path);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "WRITEENV_N", 0);
        var_set_num(vm, "SAVENV_N", 0);
        var_set_num(vm, "WRITEENV_BYTES", 0);
        var_set_num(vm, "OK", 0);
        snprintf(err, sizeof err, "WRITEENV: write fail '%s'", path);
        var_set_str(vm, "LAST_ERR", err);
        var_set_str(vm, "ERR", err);
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", path);
      var_set_str(vm, "WRITEENV", path);
      var_set_str(vm, "SAVENV", path);
      var_set_str(vm, "DUMPENVFILE", path);
      var_set_str(vm, "WRITEENV_PATH", path);
      var_set_str(vm, "WRITEENV_PREFIX", prefix);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "WRITEENV_N", n);
      var_set_num(vm, "SAVENV_N", n);
      var_set_num(vm, "DUMPENVFILE_N", n);
      var_set_num(vm, "WRITEENV_BYTES", wr.n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SUBSTENV|ENVSUBST|EXPANDENV [template]
     * — expand $NAME and ${NAME} from process env, then program vars.
     * Unset → empty. Bare $ stops at non [A-Za-z0-9_]. $$ → literal $.
     * LAST = expanded; LAST_N / SUBSTENV_N = replacements; MISS = unset count.
     * Zero-arg: template = prior LAST.
     * Usability: path/config templates without shell envsubst or chained REPLACEALL. */
    if (kw(&L->cur,"SUBSTENV") || kw(&L->cur,"ENVSUBST") ||
        kw(&L->cur,"EXPANDENV") || kw(&L->cur,"ENVEXPAND") ||
        kw(&L->cur,"TEMPLATEENV") || kw(&L->cur,"ENVTEMPLATE") ||
        kw(&L->cur,"INTERPENV") || kw(&L->cur,"DOLLARENV") ||
        kw(&L->cur,"SHELLSUB") || kw(&L->cur,"SUBENV")){
      char tmpl[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      long hits = 0, miss = 0;
      lex_next(L);
      tmpl[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, tmpl, sizeof tmpl) != 0)
        snprintf(tmpl, sizeof tmpl, "%s", vm->last_str);
      hits = cubalc_expand_substenv(vm, tmpl, out, sizeof out, &miss);
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = hits;
      var_set_num(vm, "LAST_N", hits);
      var_set_str(vm, "SUBSTENV", out);
      var_set_str(vm, "ENVSUBST", out);
      var_set_str(vm, "EXPANDENV", out);
      var_set_num(vm, "SUBSTENV_N", hits);
      var_set_num(vm, "ENVSUBST_N", hits);
      var_set_num(vm, "SUBSTENV_MISS", miss);
      var_set_num(vm, "ENVSUBST_MISS", miss);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SUBSTFILE|ENVSUBSTFILE|EXPANDFILE path [out]
     * — read template file, expand $NAME/${NAME} (env then vars), write result.
     * Default out = path (in-place). LAST = out path; LAST_N = replacements;
     * SUBSTFILE_MISS · SUBSTFILE_BYTES. Soft miss read/write → OK=0.
     * Usability: materialize plate/config templates without READ+SUBSTENV+WRITE glue. */
    if (kw(&L->cur,"SUBSTFILE") || kw(&L->cur,"ENVSUBSTFILE") ||
        kw(&L->cur,"EXPANDFILE") || kw(&L->cur,"FILESUBSTENV") ||
        kw(&L->cur,"TEMPLATEFILE") || kw(&L->cur,"FILEEXPAND") ||
        kw(&L->cur,"RENDERFILE") || kw(&L->cur,"SUBSTF") ||
        kw(&L->cur,"ENVSUBFILE") || kw(&L->cur,"MATERIALIZE")){
      char path[512], outpath[512], a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
      char expanded[CUBALC_HOST_STR_MAX];
      long hits = 0, miss = 0;
      cubalc_host_result hr, wr;
      lex_next(L);
      path[0] = 0; outpath[0] = 0; a[0] = 0; b[0] = 0; expanded[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS SUBSTFILE path [out]");
        return -1;
      }
      snprintf(path, sizeof path, "%s", a);
      if (resolve_str_arg(vm, L, b, sizeof b) == 0 && b[0])
        snprintf(outpath, sizeof outpath, "%s", b);
      else
        snprintf(outpath, sizeof outpath, "%s", path);
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SUBSTFILE_N", 0);
        var_set_num(vm, "SUBSTFILE_MISS", 0);
        var_set_num(vm, "SUBSTFILE_BYTES", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "SUBSTFILE: empty path");
        var_set_str(vm, "ERR", "SUBSTFILE: empty path");
        bump(vm); return 1;
      }
      if (cubalc_host_read(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SUBSTFILE_N", 0);
        var_set_num(vm, "SUBSTFILE_MISS", 0);
        var_set_num(vm, "SUBSTFILE_BYTES", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", hr.err[0] ? hr.err : "SUBSTFILE: read fail");
        var_set_str(vm, "ERR", hr.err[0] ? hr.err : "SUBSTFILE: read fail");
        bump(vm); return 1;
      }
      hits = cubalc_expand_substenv(vm, hr.str, expanded, sizeof expanded, &miss);
      if (cubalc_host_write(outpath, expanded, &wr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SUBSTFILE_N", hits);
        var_set_num(vm, "SUBSTFILE_MISS", miss);
        var_set_num(vm, "SUBSTFILE_BYTES", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "SUBSTFILE: write fail");
        var_set_str(vm, "ERR", wr.err[0] ? wr.err : "SUBSTFILE: write fail");
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", outpath);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", outpath);
      vm->last_n = hits;
      var_set_num(vm, "LAST_N", hits);
      var_set_str(vm, "SUBSTFILE", outpath);
      var_set_str(vm, "ENVSUBSTFILE", outpath);
      var_set_str(vm, "EXPANDFILE", outpath);
      var_set_str(vm, "SUBSTFILE_BODY", expanded);
      var_set_num(vm, "SUBSTFILE_N", hits);
      var_set_num(vm, "SUBSTFILE_MISS", miss);
      var_set_num(vm, "SUBSTFILE_BYTES", (long)strlen(expanded));
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
  return 0;
}
