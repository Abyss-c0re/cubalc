/* CubalC lang — lang_ops_core_sys.c
 * SYS/HOST megablock dispatch (mesh vote A_split_lang_ops_core).
 * Peel of ~33k-line SYS plane from lang_ops_core.
 */
#include "lang/cubalc_lang_ops_sys_plate.h"
#include "lang/cubalc_lang_ops_sys_codec.h"
#include "lang/cubalc_lang_ops_sys_time_walk.h"
#include "lang/cubalc_lang_ops_sys_plate_expand.h"
#include "lang/cubalc_lang_ops_sys_cli.h"
#include "lang/cubalc_lang_ops_core_help.h"
#include "cubalc_hostops.h"
#include "cubalc_async.h"
#include "cubalc_smx.h"
#include "cubalc_law.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <unistd.h>
#  include <dirent.h>
#  include <sys/stat.h>
#  include <sys/statvfs.h>
#  include <fcntl.h>
#  include <poll.h>
#  include <pwd.h>
#  include <grp.h>
#  include <fnmatch.h>
#  include <sys/utsname.h>
#  include <sys/resource.h>
#  include <sys/socket.h>
#  include <sys/wait.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
/* getloadavg is BSD/POSIX extension; declare if libc omits under strict feature macros. */
int getloadavg(double loadavg[], int nelem);
#endif

int cubalc_lang_ops_sys_try_plate(VM *vm, Lex *L) {
  /* Plate/JSON-file SYS plane — READ · *PLATE · JSONFILE* · WRITE.
   * Caller: after SYS/HOST keyword consumed; L at subcommand.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
    if (kw(&L->cur,"READ")){
      /* SYS READ [OR|SOFT|TRY] path [OR "fallback"]
       * Soft: optional plate — miss → OK=0 sticky LAST_ERR, no fatal.
       * Fallback: miss → LAST = fallback string, OK=1 (like ENV OR).
       * Hard (default): miss is fatal. */
      int soft = 0;
      char path[512];
      cubalc_host_result hr;
      lex_next(L);
      if (kw(&L->cur,"OR") || kw(&L->cur,"SOFT") || kw(&L->cur,"TRY") ||
          kw(&L->cur,"OPTIONAL") || kw(&L->cur,"MAYBE")){
        soft = 1;
        lex_next(L);
      }
      if (resolve_str_arg(vm, L, path, sizeof path)!=0){
        fail(vm, soft ? "SYS READ OR \"path\"|LAST" : "SYS READ \"path\"|LAST");
        return -1;
      }
      if (cubalc_host_read(path, &hr)!=0){
        char fb[CUBALC_HOST_STR_MAX];
        int have_fb = 0;
        if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
            kw(&L->cur,"FALLBACK")){
          lex_next(L);
          if (resolve_str_arg(vm, L, fb, sizeof fb) == 0)
            have_fb = 1;
          else {
            fail(vm, "SYS READ path OR \"fallback\"");
            return -1;
          }
        }
        if (have_fb) {
          snprintf(vm->last_str, sizeof vm->last_str, "%s", fb);
          vm->last_n = (long)strlen(fb);
          vm->last_code = 0;
          var_set_str(vm, "LAST", fb);
          var_set_num(vm, "LAST_N", vm->last_n);
          var_set_num(vm, "OK", 1);
          var_set_num(vm, "READ_OK", 0); /* content from fallback, not file */
          bump(vm); return 1;
        }
        if (soft) {
          const char *em = hr.err[0] ? hr.err : "SYS READ soft miss";
          var_set_str(vm, "LAST_ERR", em);
          var_set_str(vm, "ERR", em);
          var_set_str(vm, "LAST", "");
          snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
          vm->last_n = 0;
          vm->last_code = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "OK", 0);
          var_set_num(vm, "READ_OK", 0);
          bump(vm); return 1;
        }
        fail(vm, hr.err[0]?hr.err:"SYS READ fail");
        return -1;
      }
      /* success: optional trailing OR ignored (file won) */
      if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
          kw(&L->cur,"FALLBACK")){
        lex_next(L);
        char skip[CUBALC_HOST_STR_MAX];
        (void)resolve_str_arg(vm, L, skip, sizeof skip);
      }
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n; vm->last_code = 0;
      var_set_str(vm, "LAST", hr.str);
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "OK", 1);
      var_set_num(vm, "READ_OK", 1);
      bump(vm); return 1;
    }
    /* SYS LOADPLATE|READPLATE [INTO name] path [OR|DEFAULT|FROM defaults]
     * Soft agent plate load: miss or non-object → fallback (default "{}"), OK=1.
     * LAST = plate text · LAST_N = 1 file-hit | 0 fallback.
     * INTO name: also write plate into named var (multi-plate boot without LET PLATE=).
     *   SYS LOADPLATE INTO peer "state/peer.json" OR "{}"
     *   SYS LOADPLATE path INTO session
     * LOADPLATE_HIT · LOADPLATE_PATH · LOADPLATE_FALLBACK · LOADPLATE_INTO · READ_OK.
     * Note: FROM still means defaults (legacy); INTO is the multi-plate target.
     * Usability: peer/session disk plates without LET glue / PLATE clobber. */
    if (kw(&L->cur,"LOADPLATE") || kw(&L->cur,"READPLATE") || kw(&L->cur,"PLATEREAD") ||
        kw(&L->cur,"LOADJSON") || kw(&L->cur,"READJSON") || kw(&L->cur,"JSONLOAD") ||
        kw(&L->cur,"PLATELOAD") || kw(&L->cur,"OPENPLATE") || kw(&L->cur,"GETPLATE")){
      char path[512], fb[CUBALC_HOST_STR_MAX], plate[CUBALC_HOST_STR_MAX];
      char into_name[96];
      cubalc_host_result hr, keys;
      int have_fb = 0, hit = 0, have_into = 0;
      const char *body;
      lex_next(L);
      path[0] = 0;
      fb[0] = 0;
      plate[0] = 0;
      into_name[0] = 0;

      /* optional leading INTO name */
      if (kw(&L->cur,"INTO") || kw(&L->cur,"AS") || kw(&L->cur,"TOVAR") ||
          kw(&L->cur,"BIND") || kw(&L->cur,"STOREIN")) {
        lex_next(L);
        have_into = 1;
        if (L->cur.kind == TK_IDENT) {
          snprintf(into_name, sizeof into_name, "%s", L->cur.text);
          lex_next(L);
        } else {
          fail(vm, "SYS LOADPLATE INTO name path — need var name");
          return -1;
        }
      }

      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, "SYS LOADPLATE [INTO name] path [OR defaults] — need plate path");
        return -1;
      }
      if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
          kw(&L->cur,"FALLBACK") || kw(&L->cur,"FROM") || kw(&L->cur,"WITH") ||
          kw(&L->cur,"USING")) {
        lex_next(L);
        if (resolve_str_arg(vm, L, fb, sizeof fb) == 0)
          have_fb = 1;
        else {
          fail(vm, "SYS LOADPLATE path OR defaults — need fallback plate");
          return -1;
        }
      }
      /* trailing INTO name */
      if (!have_into && (kw(&L->cur,"INTO") || kw(&L->cur,"AS") || kw(&L->cur,"TOVAR") ||
                         kw(&L->cur,"BIND") || kw(&L->cur,"STOREIN"))) {
        lex_next(L);
        have_into = 1;
        if (L->cur.kind == TK_IDENT) {
          snprintf(into_name, sizeof into_name, "%s", L->cur.text);
          lex_next(L);
        } else {
          fail(vm, "SYS LOADPLATE path INTO name — need var name");
          return -1;
        }
      }
      if (!have_fb)
        snprintf(fb, sizeof fb, "%s", "{}");
      /* ensure fallback is object-looking; else force {} */
      {
        const char *f = fb;
        while (*f == ' ' || *f == '\t' || *f == '\n' || *f == '\r') f++;
        if (*f != '{')
          snprintf(fb, sizeof fb, "%s", "{}");
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_read(path, &hr) == 0 && hr.str[0]) {
        body = hr.str;
        while (*body == ' ' || *body == '\t' || *body == '\n' || *body == '\r')
          body++;
        if (*body == '{') {
          /* require parseable key set (object) */
          memset(&keys, 0, sizeof keys);
          if (cubalc_host_json_keys(body, &keys) == 0) {
            snprintf(plate, sizeof plate, "%s", body);
            hit = 1;
          }
        }
      }
      if (!hit) {
        snprintf(plate, sizeof plate, "%s", fb);
        var_set_str(vm, "LAST_ERR",
                    hr.err[0] ? hr.err : "LOADPLATE: miss or non-object — using fallback");
        /* sticky soft hint only; OK stays 1 for agent boot continue */
      }
      if (have_into && into_name[0])
        var_set_str(vm, into_name, plate);
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = hit ? 1 : 0;
      var_set_num(vm, "LAST_N", hit ? 1 : 0);
      var_set_str(vm, "LOADPLATE", plate);
      var_set_str(vm, "READPLATE", plate);
      var_set_str(vm, "LOADPLATE_PATH", path);
      var_set_num(vm, "LOADPLATE_HIT", hit);
      var_set_num(vm, "LOADPLATE_FALLBACK", hit ? 0 : 1);
      var_set_num(vm, "LOADPLATE_INTO", have_into ? 1 : 0);
      var_set_str(vm, "LOADPLATE_VAR", into_name);
      var_set_num(vm, "READ_OK", hit);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys loadplate → hit=%d into=%s path=%s\n",
                hit, into_name[0] ? into_name : "-", path);
      bump(vm); return 1;
    }
    /* SYS SAVEPLATE|STOREPLATE path [FROM plate]|[plate]
     * — validate object plate, mkdir parent, write file.
     * 2-arg: path plate · FROM plate · 1-arg: path (PLATE var if object, else LAST).
     * SAVEPLATE_FROM=0|1 · multi-plate: SYS SAVEPLATE path FROM peer
     * LAST = abs path · LAST_N = bytes · SAVEPLATE_PATH · SAVEPLATE_N.
     * Soft fail (OK=0) if non-object or empty path; no fatal for agent recovery.
     * Usability: dual of LOADPLATE — persist agent state without WRITEOUT+type glue.
     * Note: PLATEWRITE remains WRITEOUT alias (any data). */
    if (kw(&L->cur,"SAVEPLATE") || kw(&L->cur,"STOREPLATE") || kw(&L->cur,"PLATESAVE") ||
        kw(&L->cur,"DUMPPLATE") || kw(&L->cur,"PUTPLATE") || kw(&L->cur,"PERSISTPLATE") ||
        kw(&L->cur,"JSONSAVE") || kw(&L->cur,"SAVEJSON") || kw(&L->cur,"WRITEPLATE")){
      char path[CUBALC_HOST_STR_MAX], plate[CUBALC_HOST_STR_MAX];
      char absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
      char from_name[96];
      const char *slash, *body;
      size_t n;
      int created = 0, have_from = 0;
      cubalc_host_result hr, keys;
      Var *pv;
      lex_next(L);
      path[0] = 0;
      plate[0] = 0;
      from_name[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, "SYS SAVEPLATE path [FROM plate] — need path");
        return -1;
      }
      if (kw(&L->cur,"FROM") || kw(&L->cur,"USING") || kw(&L->cur,"OF") ||
          kw(&L->cur,"WITHPLATE") || kw(&L->cur,"PLATEFROM")) {
        lex_next(L);
        have_from = 1;
        if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
          snprintf(plate, sizeof plate, "%s", vm->last_str);
          lex_next(L);
        } else if (L->cur.kind == TK_IDENT) {
          pv = var_get(vm, L->cur.text, 0);
          if (pv && pv->is_str) {
            snprintf(from_name, sizeof from_name, "%s", L->cur.text);
            snprintf(plate, sizeof plate, "%s", pv->sval);
            lex_next(L);
          } else if (pv) {
            snprintf(from_name, sizeof from_name, "%s", L->cur.text);
            snprintf(plate, sizeof plate, "%ld", pv->val);
            lex_next(L);
          } else if (resolve_str_arg(vm, L, plate, sizeof plate) != 0) {
            plate[0] = 0;
          }
        } else if (resolve_str_arg(vm, L, plate, sizeof plate) != 0) {
          snprintf(plate, sizeof plate, "%s", vm->last_str);
        }
      } else if (resolve_str_arg(vm, L, plate, sizeof plate) != 0) {
        /* 1-arg: prefer conventional PLATE var, else LAST (legacy) */
        pv = var_get(vm, "PLATE", 0);
        if (pv && pv->is_str && pv->sval[0]) {
          const char *b = pv->sval;
          while (*b == ' ' || *b == '\t' || *b == '\n' || *b == '\r') b++;
          if (*b == '{')
            snprintf(plate, sizeof plate, "%s", pv->sval);
          else
            snprintf(plate, sizeof plate, "%s", vm->last_str);
        } else {
          snprintf(plate, sizeof plate, "%s", vm->last_str);
        }
      }
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "SAVEPLATE: empty path");
        var_set_str(vm, "ERR", "SAVEPLATE: empty path");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SAVEPLATE_N", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      body = plate;
      while (*body == ' ' || *body == '\t' || *body == '\n' || *body == '\r')
        body++;
      if (*body != '{') {
        var_set_str(vm, "LAST", path);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
        var_set_str(vm, "LAST_ERR", "SAVEPLATE: need object plate (starts with {)");
        var_set_str(vm, "ERR", "SAVEPLATE: need object plate (starts with {)");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SAVEPLATE_N", 0);
        var_set_num(vm, "SAVEPLATE_OK", 0);
        var_set_str(vm, "SAVEPLATE_PATH", path);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      memset(&keys, 0, sizeof keys);
      if (cubalc_host_json_keys(body, &keys) != 0) {
        var_set_str(vm, "LAST", path);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
        var_set_str(vm, "LAST_ERR", "SAVEPLATE: invalid object plate");
        var_set_str(vm, "ERR", "SAVEPLATE: invalid object plate");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SAVEPLATE_N", 0);
        var_set_num(vm, "SAVEPLATE_OK", 0);
        var_set_str(vm, "SAVEPLATE_PATH", path);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
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
        } else {
          parent[0] = '.';
          parent[1] = 0;
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
        const char *err = hr.err[0] ? hr.err : "SAVEPLATE: mkdir parent failed";
        var_set_str(vm, "LAST", absfile);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
        var_set_str(vm, "LAST_ERR", err);
        var_set_str(vm, "ERR", err);
        var_set_str(vm, "SAVEPLATE_PATH", absfile);
        var_set_str(vm, "SAVEPLATE_PARENT", absparent);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SAVEPLATE_N", 0);
        var_set_num(vm, "SAVEPLATE_OK", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      if (hr.str[0])
        snprintf(absparent, sizeof absparent, "%s", hr.str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(absfile, body, &hr) != 0) {
        const char *err = hr.err[0] ? hr.err : "SAVEPLATE: write failed";
        var_set_str(vm, "LAST", absfile);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
        var_set_str(vm, "LAST_ERR", err);
        var_set_str(vm, "ERR", err);
        var_set_str(vm, "SAVEPLATE_PATH", absfile);
        var_set_str(vm, "SAVEPLATE_PARENT", absparent);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SAVEPLATE_N", 0);
        var_set_num(vm, "SAVEPLATE_OK", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", absfile);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absfile);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_str(vm, "SAVEPLATE", absfile);
      var_set_str(vm, "STOREPLATE", absfile);
      var_set_str(vm, "SAVEPLATE_PATH", absfile);
      var_set_str(vm, "SAVEPLATE_PARENT", absparent);
      var_set_num(vm, "SAVEPLATE_N", hr.n);
      var_set_num(vm, "SAVEPLATE_OK", 1);
      var_set_num(vm, "SAVEPLATE_FROM", have_from ? 1 : 0);
      var_set_str(vm, "SAVEPLATE_SRC", from_name[0] ? from_name : (have_from ? "" : ""));
      var_set_num(vm, "SAVEPLATE_CREATED", (long)created);
      var_set_num(vm, "WRITE_OK", 1);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys saveplate → %s n=%ld created=%d\n",
                absfile, hr.n, created);
      bump(vm); return 1;
    }
    /* SYS ENSUREPLATE|TOUCHPLATE [INTO name] path [OR|FROM defaults]
     * Create-or-load agent state plate:
     *   hit  → LAST=plate content · LAST_N=0 · ENSUREPLATE_CREATED=0
     *   miss → write defaults/{} · LAST=plate · LAST_N=1 · CREATED=1
     * INTO name: also bind plate into named var (multi-plate seed without PLATE clobber):
     *   SYS ENSUREPLATE INTO peer "state/peer.json" OR "{\"n\":0}"
     *   SYS ENSUREPLATE path INTO session
     * Always OK=1 when write/load succeeds. Dual of LOAD+SAVE one-shot boot.
     * ENSUREPLATE_INTO · ENSUREPLATE_VAR mirror LOADPLATE INTO flags.
     * Usability: first-run multi-plate seed without IF LOADPLATE_HIT + LET glue. */
    if (kw(&L->cur,"ENSUREPLATE") || kw(&L->cur,"TOUCHPLATE") || kw(&L->cur,"NEEDPLATE") ||
        kw(&L->cur,"PLATEENSURE") || kw(&L->cur,"SEEDPLATE") ||
        kw(&L->cur,"INITPLATE") || kw(&L->cur,"BOOTPLATE") || kw(&L->cur,"HAVEPLATE")){
      char path[CUBALC_HOST_STR_MAX], defs[CUBALC_HOST_STR_MAX], plate[CUBALC_HOST_STR_MAX];
      char absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
      char into_name[96];
      const char *slash, *body;
      size_t n;
      int hit = 0, created = 0, have_into = 0;
      cubalc_host_result hr, keys;
      lex_next(L);
      path[0] = 0;
      defs[0] = 0;
      plate[0] = 0;
      into_name[0] = 0;

      if (kw(&L->cur,"INTO") || kw(&L->cur,"AS") || kw(&L->cur,"TOVAR") ||
          kw(&L->cur,"BIND") || kw(&L->cur,"STOREIN")) {
        lex_next(L);
        have_into = 1;
        if (L->cur.kind == TK_IDENT) {
          snprintf(into_name, sizeof into_name, "%s", L->cur.text);
          lex_next(L);
        } else {
          fail(vm, "SYS ENSUREPLATE INTO name path — need var name");
          return -1;
        }
      }

      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, "SYS ENSUREPLATE [INTO name] path [OR defaults] — need path");
        return -1;
      }
      if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
          kw(&L->cur,"FALLBACK") || kw(&L->cur,"FROM") || kw(&L->cur,"WITH") ||
          kw(&L->cur,"USING") || kw(&L->cur,"SEED")) {
        lex_next(L);
        if (resolve_str_arg(vm, L, defs, sizeof defs) != 0)
          defs[0] = 0;
      }
      if (!have_into && (kw(&L->cur,"INTO") || kw(&L->cur,"AS") || kw(&L->cur,"TOVAR") ||
                         kw(&L->cur,"BIND") || kw(&L->cur,"STOREIN"))) {
        lex_next(L);
        have_into = 1;
        if (L->cur.kind == TK_IDENT) {
          snprintf(into_name, sizeof into_name, "%s", L->cur.text);
          lex_next(L);
        } else {
          fail(vm, "SYS ENSUREPLATE path INTO name — need var name");
          return -1;
        }
      }
      if (!defs[0])
        snprintf(defs, sizeof defs, "%s", "{}");
      {
        const char *f = defs;
        while (*f == ' ' || *f == '\t' || *f == '\n' || *f == '\r') f++;
        if (*f != '{')
          snprintf(defs, sizeof defs, "%s", "{}");
      }
      if (!path[0]) {
        var_set_str(vm, "LAST_ERR", "ENSUREPLATE: empty path");
        var_set_str(vm, "ERR", "ENSUREPLATE: empty path");
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "ENSUREPLATE_INTO", have_into ? 1 : 0);
        bump(vm); return 1;
      }
      /* try load existing object plate */
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_read(path, &hr) == 0 && hr.str[0]) {
        body = hr.str;
        while (*body == ' ' || *body == '\t' || *body == '\n' || *body == '\r')
          body++;
        if (*body == '{') {
          memset(&keys, 0, sizeof keys);
          if (cubalc_host_json_keys(body, &keys) == 0) {
            snprintf(plate, sizeof plate, "%s", body);
            hit = 1;
          }
        }
      }
      if (hit) {
        if (have_into && into_name[0])
          var_set_str(vm, into_name, plate);
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "ENSUREPLATE", plate);
        var_set_str(vm, "TOUCHPLATE", plate);
        var_set_str(vm, "ENSUREPLATE_PATH", path);
        var_set_num(vm, "ENSUREPLATE_CREATED", 0);
        var_set_num(vm, "ENSUREPLATE_HIT", 1);
        var_set_num(vm, "ENSUREPLATE_INTO", have_into ? 1 : 0);
        var_set_str(vm, "ENSUREPLATE_VAR", into_name);
        var_set_num(vm, "LOADPLATE_HIT", 1);
        var_set_num(vm, "LOADPLATE_INTO", have_into ? 1 : 0);
        var_set_num(vm, "READ_OK", 1);
        var_set_num(vm, "OK", 1);
        if (vm->trace)
          fprintf(vm->trace, "# sys ensureplate → hit into=%s path=%s\n",
                  into_name[0] ? into_name : "-", path);
        bump(vm); return 1;
      }
      /* miss/invalid → persist defaults (mkdir parent + write) */
      body = defs;
      while (*body == ' ' || *body == '\t' || *body == '\n' || *body == '\r')
        body++;
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
        } else {
          parent[0] = '.';
          parent[1] = 0;
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
        const char *err = hr.err[0] ? hr.err : "ENSUREPLATE: mkdir parent failed";
        var_set_str(vm, "LAST_ERR", err);
        var_set_str(vm, "ERR", err);
        var_set_str(vm, "ENSUREPLATE_PATH", absfile);
        var_set_num(vm, "ENSUREPLATE_CREATED", 0);
        var_set_num(vm, "ENSUREPLATE_INTO", have_into ? 1 : 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      if (hr.str[0])
        snprintf(absparent, sizeof absparent, "%s", hr.str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(absfile, body, &hr) != 0) {
        const char *err = hr.err[0] ? hr.err : "ENSUREPLATE: write failed";
        var_set_str(vm, "LAST_ERR", err);
        var_set_str(vm, "ERR", err);
        var_set_str(vm, "ENSUREPLATE_PATH", absfile);
        var_set_num(vm, "ENSUREPLATE_CREATED", 0);
        var_set_num(vm, "ENSUREPLATE_INTO", have_into ? 1 : 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      snprintf(plate, sizeof plate, "%s", body);
      if (have_into && into_name[0])
        var_set_str(vm, into_name, plate);
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_str(vm, "ENSUREPLATE", plate);
      var_set_str(vm, "TOUCHPLATE", plate);
      var_set_str(vm, "ENSUREPLATE_PATH", absfile);
      var_set_str(vm, "ENSUREPLATE_PARENT", absparent);
      var_set_num(vm, "ENSUREPLATE_CREATED", 1);
      var_set_num(vm, "ENSUREPLATE_HIT", 0);
      var_set_num(vm, "ENSUREPLATE_INTO", have_into ? 1 : 0);
      var_set_str(vm, "ENSUREPLATE_VAR", into_name);
      var_set_num(vm, "SAVEPLATE_N", hr.n);
      var_set_num(vm, "SAVEPLATE_OK", 1);
      var_set_num(vm, "LOADPLATE_HIT", 0);
      var_set_num(vm, "LOADPLATE_INTO", have_into ? 1 : 0);
      var_set_num(vm, "READ_OK", 0);
      var_set_num(vm, "WRITE_OK", 1);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys ensureplate → created into=%s path=%s\n",
                into_name[0] ? into_name : "-", absfile);
      bump(vm); return 1;
    }

    /* SYS UPDATEPLATE|JSONFILESET|PLATEKEYSET path key value
     * One-shot agent plate mutate: soft-load object plate (miss→{}),
     * JSONSET key value, mkdir parent + write. LAST = new plate content.
     * LAST_N = 1 key-updated | 0 key-inserted (json_set n).
     * UPDATEPLATE_HIT=1 if prior object file · CREATED=1 if file was new.
     * Value rules match JSONSET (num/true/false/null raw; strings quoted; RAW).
     * Usability: persist a plate field without LOAD+JSONSET+SAVE glue.
     * Note: PLATESET stays key=val file (KVFILESET); this is JSON object plates. */
    if (kw(&L->cur,"UPDATEPLATE") || kw(&L->cur,"JSONFILESET") || kw(&L->cur,"PLATEKEYSET") ||
        kw(&L->cur,"SETJSONFILE") || kw(&L->cur,"PUTJSONFILE") || kw(&L->cur,"PATCHPLATE") ||
        kw(&L->cur,"PLATEPUT") || kw(&L->cur,"PUTPLATEKEY") || kw(&L->cur,"SETPLATEKEY") ||
        kw(&L->cur,"JSONFILEPUT") || kw(&L->cur,"FILEJSONSET")){
      char path[CUBALC_HOST_STR_MAX], key[96], val[CUBALC_HOST_STR_MAX];
      char plate[CUBALC_HOST_STR_MAX], absfile[CUBALC_HOST_STR_MAX];
      char parent[512], absparent[CUBALC_HOST_STR_MAX];
      const char *slash, *body;
      size_t n;
      int hit = 0, created_file = 0, val_kind = 0, force_raw = 0;
      cubalc_host_result hr, keys, setr;
      lex_next(L);
      path[0] = 0; key[0] = 0; val[0] = 0; plate[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, "SYS UPDATEPLATE path key value — need plate path");
        return -1;
      }
      /* key */
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(key, sizeof key, "%ld", nv);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, key, sizeof key) != 0) {
          fail(vm, "SYS UPDATEPLATE path key value — need key");
          return -1;
        }
      } else {
        fail(vm, "SYS UPDATEPLATE path key value — need key");
        return -1;
      }
      if (kw(&L->cur,"RAW") || kw(&L->cur,"ASRAW") || kw(&L->cur,"LITERAL")) {
        force_raw = 1;
        lex_next(L);
      }
      /* value */
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(val, sizeof val, "%ld", nv);
        val_kind = 1;
      } else if (L->cur.kind == TK_IDENT &&
                 (strcmp(L->cur.text, "true") == 0 || strcmp(L->cur.text, "false") == 0 ||
                  strcmp(L->cur.text, "null") == 0)) {
        snprintf(val, sizeof val, "%s", L->cur.text);
        val_kind = 1;
        lex_next(L);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, val, sizeof val) != 0) {
          fail(vm, "SYS UPDATEPLATE path key value — need value");
          return -1;
        }
        val_kind = 0;
      } else {
        fail(vm, "SYS UPDATEPLATE path key value — need value");
        return -1;
      }
      if (force_raw) val_kind = 1;
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "UPDATEPLATE: empty path");
        var_set_str(vm, "ERR", "UPDATEPLATE: empty path");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "UPDATEPLATE_N", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      if (!key[0]) {
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "UPDATEPLATE: empty key");
        var_set_str(vm, "ERR", "UPDATEPLATE: empty key");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "UPDATEPLATE_N", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      /* soft load existing object plate */
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_read(path, &hr) == 0 && hr.str[0]) {
        body = hr.str;
        while (*body == ' ' || *body == '\t' || *body == '\n' || *body == '\r')
          body++;
        if (*body == '{') {
          memset(&keys, 0, sizeof keys);
          if (cubalc_host_json_keys(body, &keys) == 0) {
            snprintf(plate, sizeof plate, "%s", body);
            hit = 1;
          }
        }
      }
      if (!hit)
        snprintf(plate, sizeof plate, "%s", "{}");
      created_file = hit ? 0 : 1;
      memset(&setr, 0, sizeof setr);
      if (cubalc_host_json_set(plate, key, val, val_kind, &setr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "UPDATEPLATE_N", 0);
        var_set_num(vm, "OK", 0);
        if (setr.err[0]) {
          var_set_str(vm, "LAST_ERR", setr.err);
          var_set_str(vm, "ERR", setr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "UPDATEPLATE: json set fail");
          var_set_str(vm, "ERR", "UPDATEPLATE: json set fail");
        }
        var_set_str(vm, "UPDATEPLATE_PATH", path);
        var_set_str(vm, "UPDATEPLATE_KEY", key);
        bump(vm); return 1;
      }
      snprintf(plate, sizeof plate, "%s", setr.str);
      body = plate;
      /* resolve abs path + parent (same as SAVEPLATE) */
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
        } else {
          parent[0] = '.';
          parent[1] = 0;
        }
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
        snprintf(absparent, sizeof absparent, "%s", hr.str);
      else
        snprintf(absparent, sizeof absparent, "%s", parent);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_mkdir(absparent, &hr) != 0) {
        const char *err = hr.err[0] ? hr.err : "UPDATEPLATE: mkdir parent failed";
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        var_set_str(vm, "LAST_ERR", err);
        var_set_str(vm, "ERR", err);
        var_set_str(vm, "UPDATEPLATE_PATH", absfile);
        var_set_str(vm, "UPDATEPLATE_KEY", key);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "UPDATEPLATE_N", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      if (hr.str[0])
        snprintf(absparent, sizeof absparent, "%s", hr.str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(absfile, body, &hr) != 0) {
        const char *err = hr.err[0] ? hr.err : "UPDATEPLATE: write failed";
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        var_set_str(vm, "LAST_ERR", err);
        var_set_str(vm, "ERR", err);
        var_set_str(vm, "UPDATEPLATE_PATH", absfile);
        var_set_str(vm, "UPDATEPLATE_KEY", key);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "UPDATEPLATE_N", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = setr.n;
      var_set_num(vm, "LAST_N", setr.n);
      var_set_str(vm, "UPDATEPLATE", plate);
      var_set_str(vm, "JSONFILESET", plate);
      var_set_str(vm, "PLATEKEYSET", plate);
      var_set_str(vm, "UPDATEPLATE_PATH", absfile);
      var_set_str(vm, "UPDATEPLATE_KEY", key);
      var_set_str(vm, "UPDATEPLATE_PARENT", absparent);
      var_set_num(vm, "UPDATEPLATE_N", setr.n);
      var_set_num(vm, "UPDATEPLATE_HIT", hit);
      var_set_num(vm, "UPDATEPLATE_CREATED", created_file);
      var_set_num(vm, "UPDATEPLATE_BYTES", hr.n);
      var_set_num(vm, "JSONSET_N", setr.n);
      var_set_str(vm, "JSONSET_KEY", key);
      var_set_num(vm, "SAVEPLATE_N", hr.n);
      var_set_num(vm, "SAVEPLATE_OK", 1);
      var_set_str(vm, "SAVEPLATE_PATH", absfile);
      var_set_num(vm, "LOADPLATE_HIT", hit);
      var_set_num(vm, "WRITE_OK", 1);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys updateplate → path=%s key=%s hit=%d updated=%ld\n",
                absfile, key, hit, setr.n);
      bump(vm); return 1;
    }
    /* SYS BUMPPLATE|JSONFILEINC|PLATEKEYINC path key [delta]
     * One-shot agent plate counter: soft-load object (miss→{}), bump key by delta
     * (default +1; DECRPLATE/JSONFILEDEC default −1), mkdir parent + write.
     * LAST = new plate · LAST_N = new value · BUMPPLATE_V/DELTA/HIT/CREATED.
     * Missing key treated as 0 (same as JSONINC). Soft empty path/key → OK=0.
     * Usability: tick/retry counters without LOAD+JSONINC+SAVE glue.
     * Note: PLATEINC stays key=val file (KVFILEINC); this is JSON object plates. */
    if (kw(&L->cur,"BUMPPLATE") || kw(&L->cur,"JSONFILEINC") || kw(&L->cur,"PLATEKEYINC") ||
        kw(&L->cur,"INCJSONFILE") || kw(&L->cur,"FILEJSONINC") || kw(&L->cur,"INCPLATEKEY") ||
        kw(&L->cur,"TICKPLATE") || kw(&L->cur,"PLATEBUMP") || kw(&L->cur,"BUMPJSONFILE") ||
        kw(&L->cur,"DECRPLATE") || kw(&L->cur,"JSONFILEDEC") || kw(&L->cur,"PLATEKEYDEC") ||
        kw(&L->cur,"DECJSONFILE") || kw(&L->cur,"FILEJSONDEC") || kw(&L->cur,"DECPLATEKEY")){
      char path[CUBALC_HOST_STR_MAX], key[96], plate[CUBALC_HOST_STR_MAX];
      char raw[32], absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
      const char *slash, *body;
      size_t n;
      int hit = 0, created_file = 0, is_dec = 0;
      long delta = 1, cur = 0, nv = 0;
      cubalc_host_result hr, keys, gr, setr;
      if (kw(&L->cur,"DECRPLATE") || kw(&L->cur,"JSONFILEDEC") || kw(&L->cur,"PLATEKEYDEC") ||
          kw(&L->cur,"DECJSONFILE") || kw(&L->cur,"FILEJSONDEC") || kw(&L->cur,"DECPLATEKEY")) {
        is_dec = 1;
        delta = -1;
      }
      lex_next(L);
      path[0] = 0; key[0] = 0; plate[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, is_dec ? "SYS DECRPLATE path key [delta] — need plate path"
                        : "SYS BUMPPLATE path key [delta] — need plate path");
        return -1;
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long kv = parse_expr(vm, L);
        snprintf(key, sizeof key, "%ld", kv);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, key, sizeof key) != 0) {
          fail(vm, is_dec ? "SYS DECRPLATE path key [delta] — need key"
                          : "SYS BUMPPLATE path key [delta] — need key");
          return -1;
        }
      } else {
        fail(vm, is_dec ? "SYS DECRPLATE path key [delta] — need key"
                        : "SYS BUMPPLATE path key [delta] — need key");
        return -1;
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN)
        delta = parse_expr(vm, L);
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "BUMPPLATE: empty path");
        var_set_str(vm, "ERR", "BUMPPLATE: empty path");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "BUMPPLATE_V", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      if (!key[0]) {
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "BUMPPLATE: empty key");
        var_set_str(vm, "ERR", "BUMPPLATE: empty key");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "BUMPPLATE_V", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      /* soft load */
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_read(path, &hr) == 0 && hr.str[0]) {
        body = hr.str;
        while (*body == ' ' || *body == '\t' || *body == '\n' || *body == '\r')
          body++;
        if (*body == '{') {
          memset(&keys, 0, sizeof keys);
          if (cubalc_host_json_keys(body, &keys) == 0) {
            snprintf(plate, sizeof plate, "%s", body);
            hit = 1;
          }
        }
      }
      if (!hit)
        snprintf(plate, sizeof plate, "%s", "{}");
      created_file = hit ? 0 : 1;
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_get(plate, key, &gr) == 0) {
        char *end = NULL;
        cur = strtol(gr.str, &end, 10);
        if (end == gr.str) cur = 0;
      } else {
        cur = 0;
      }
      nv = cur + delta;
      snprintf(raw, sizeof raw, "%ld", nv);
      memset(&setr, 0, sizeof setr);
      if (cubalc_host_json_set(plate, key, raw, 1 /* raw num */, &setr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "BUMPPLATE_V", 0);
        var_set_num(vm, "OK", 0);
        if (setr.err[0]) {
          var_set_str(vm, "LAST_ERR", setr.err);
          var_set_str(vm, "ERR", setr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "BUMPPLATE: json set fail");
          var_set_str(vm, "ERR", "BUMPPLATE: json set fail");
        }
        var_set_str(vm, "BUMPPLATE_PATH", path);
        var_set_str(vm, "BUMPPLATE_KEY", key);
        bump(vm); return 1;
      }
      snprintf(plate, sizeof plate, "%s", setr.str);
      body = plate;
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
        } else {
          parent[0] = '.';
          parent[1] = 0;
        }
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
        snprintf(absparent, sizeof absparent, "%s", hr.str);
      else
        snprintf(absparent, sizeof absparent, "%s", parent);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_mkdir(absparent, &hr) != 0) {
        const char *err = hr.err[0] ? hr.err : "BUMPPLATE: mkdir parent failed";
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        var_set_str(vm, "LAST_ERR", err);
        var_set_str(vm, "ERR", err);
        var_set_str(vm, "BUMPPLATE_PATH", absfile);
        var_set_str(vm, "BUMPPLATE_KEY", key);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "BUMPPLATE_V", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      if (hr.str[0])
        snprintf(absparent, sizeof absparent, "%s", hr.str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(absfile, body, &hr) != 0) {
        const char *err = hr.err[0] ? hr.err : "BUMPPLATE: write failed";
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        var_set_str(vm, "LAST_ERR", err);
        var_set_str(vm, "ERR", err);
        var_set_str(vm, "BUMPPLATE_PATH", absfile);
        var_set_str(vm, "BUMPPLATE_KEY", key);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "BUMPPLATE_V", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = nv;
      var_set_num(vm, "LAST_N", nv);
      var_set_str(vm, "BUMPPLATE", plate);
      var_set_str(vm, "JSONFILEINC", plate);
      var_set_str(vm, "PLATEKEYINC", plate);
      var_set_str(vm, "DECRPLATE", plate);
      var_set_str(vm, "BUMPPLATE_PATH", absfile);
      var_set_str(vm, "BUMPPLATE_KEY", key);
      var_set_str(vm, "BUMPPLATE_PARENT", absparent);
      var_set_num(vm, "BUMPPLATE_V", nv);
      var_set_num(vm, "BUMPPLATE_DELTA", delta);
      var_set_num(vm, "BUMPPLATE_HIT", hit);
      var_set_num(vm, "BUMPPLATE_CREATED", created_file);
      var_set_num(vm, "BUMPPLATE_BYTES", hr.n);
      var_set_num(vm, "JSONINC_V", nv);
      var_set_num(vm, "JSONINC_DELTA", delta);
      var_set_str(vm, "JSONINC_KEY", key);
      var_set_num(vm, "SAVEPLATE_N", hr.n);
      var_set_num(vm, "SAVEPLATE_OK", 1);
      var_set_str(vm, "SAVEPLATE_PATH", absfile);
      var_set_num(vm, "LOADPLATE_HIT", hit);
      var_set_num(vm, "WRITE_OK", 1);
      var_set_num(vm, "OK", 1);
      (void)is_dec;
      if (vm->trace)
        fprintf(vm->trace, "# sys bumpplate → path=%s key=%s v=%ld delta=%ld hit=%d\n",
                absfile, key, nv, delta, hit);
      bump(vm); return 1;
    }
    /* SYS PEEKPLATE|JSONFILEGET|PLATEKEYGET path key [OR|DEFAULT fallback]
     * One-shot peel field from on-disk JSON plate:
     *   soft-load object (miss/non-object → empty plate) then JSON get key.
     * Hit  → LAST=value · LAST_N=1 · PEEKPLATE_HIT=1 · OK=1
     * Miss + OR → LAST=fallback · LAST_N=0 · HIT=0 · OK=1
     * Miss no OR → LAST="" · LAST_N=0 · HIT=0 · OK=0 sticky LAST_ERR
     * PEEKPLATE_PATH · PEEKPLATE_KEY · LOADPLATE_HIT (file object hit).
     * Usability: read one agent state field without LOADPLATE+JSON glue.
     * Note: PLATEGET stays key=val LOOKUPFILE; this is JSON object plates. */
    if (kw(&L->cur,"PEEKPLATE") || kw(&L->cur,"JSONFILEGET") || kw(&L->cur,"PLATEKEYGET") ||
        kw(&L->cur,"GETJSONFILE") || kw(&L->cur,"FILEJSONGET") || kw(&L->cur,"PLATEPEEK") ||
        kw(&L->cur,"READPLATEKEY") || kw(&L->cur,"GETPLATEKEY") || kw(&L->cur,"PLATEFIELD") ||
        kw(&L->cur,"JSONFILEPEEK") || kw(&L->cur,"PEEKJSONFILE")){
      char path[CUBALC_HOST_STR_MAX], key[96], fb[CUBALC_HOST_STR_MAX];
      char plate[CUBALC_HOST_STR_MAX];
      const char *body;
      int have_fb = 0, file_hit = 0, key_hit = 0;
      cubalc_host_result hr, keys, gr;
      lex_next(L);
      path[0] = 0; key[0] = 0; fb[0] = 0; plate[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, "SYS PEEKPLATE path key [OR fallback] — need plate path");
        return -1;
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long kv = parse_expr(vm, L);
        snprintf(key, sizeof key, "%ld", kv);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, key, sizeof key) != 0) {
          fail(vm, "SYS PEEKPLATE path key [OR fallback] — need key");
          return -1;
        }
      } else {
        fail(vm, "SYS PEEKPLATE path key [OR fallback] — need key");
        return -1;
      }
      if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
          kw(&L->cur,"FALLBACK") || kw(&L->cur,"SOFT")) {
        lex_next(L);
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long nv = parse_expr(vm, L);
          snprintf(fb, sizeof fb, "%ld", nv);
          have_fb = 1;
        } else if (resolve_str_arg(vm, L, fb, sizeof fb) == 0) {
          have_fb = 1;
        } else {
          fail(vm, "SYS PEEKPLATE path key OR fallback — need fallback");
          return -1;
        }
      }
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "PEEKPLATE: empty path");
        var_set_str(vm, "ERR", "PEEKPLATE: empty path");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "PEEKPLATE_HIT", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      if (!key[0]) {
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "PEEKPLATE: empty key");
        var_set_str(vm, "ERR", "PEEKPLATE: empty key");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "PEEKPLATE_HIT", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      /* soft load object plate */
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_read(path, &hr) == 0 && hr.str[0]) {
        body = hr.str;
        while (*body == ' ' || *body == '\t' || *body == '\n' || *body == '\r')
          body++;
        if (*body == '{') {
          memset(&keys, 0, sizeof keys);
          if (cubalc_host_json_keys(body, &keys) == 0) {
            snprintf(plate, sizeof plate, "%s", body);
            file_hit = 1;
          }
        }
      }
      if (!file_hit)
        snprintf(plate, sizeof plate, "%s", "{}");
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_get(plate, key, &gr) == 0) {
        key_hit = 1;
        var_set_str(vm, "LAST", gr.str);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", gr.str);
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_str(vm, "PEEKPLATE", gr.str);
        var_set_str(vm, "JSONFILEGET", gr.str);
        var_set_str(vm, "PLATEKEYGET", gr.str);
        var_set_num(vm, "PEEKPLATE_HIT", 1);
        var_set_num(vm, "PEEKPLATE_OR", 0);
        var_set_num(vm, "JSON_HIT", 1);
        var_set_num(vm, "OK", 1);
      } else if (have_fb) {
        var_set_str(vm, "LAST", fb);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", fb);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "PEEKPLATE", fb);
        var_set_str(vm, "JSONFILEGET", fb);
        var_set_num(vm, "PEEKPLATE_HIT", 0);
        var_set_num(vm, "PEEKPLATE_OR", 1);
        var_set_num(vm, "JSON_HIT", 0);
        var_set_num(vm, "JSON_OR", 1);
        var_set_num(vm, "OK", 1);
      } else {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "PEEKPLATE_HIT", 0);
        var_set_num(vm, "PEEKPLATE_OR", 0);
        var_set_num(vm, "JSON_HIT", 0);
        var_set_num(vm, "OK", 0);
        if (gr.err[0]) {
          var_set_str(vm, "LAST_ERR", gr.err);
          var_set_str(vm, "ERR", gr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "PEEKPLATE: key miss");
          var_set_str(vm, "ERR", "PEEKPLATE: key miss");
        }
      }
      var_set_str(vm, "PEEKPLATE_PATH", path);
      var_set_str(vm, "PEEKPLATE_KEY", key);
      var_set_num(vm, "PEEKPLATE_FILE", file_hit);
      var_set_num(vm, "LOADPLATE_HIT", file_hit);
      var_set_str(vm, "JSON_KEY", key);
      if (vm->trace)
        fprintf(vm->trace, "# sys peekplate → path=%s key=%s file=%d hit=%d\n",
                path, key, file_hit, key_hit);
      (void)key_hit;
      bump(vm); return 1;
    }
    /* SYS DROPPLATE|JSONFILEDEL|PLATEKEYDEL path key
     * One-shot drop field from on-disk JSON plate:
     *   soft-load object (miss→{}), JSONDEL key, mkdir parent + write if needed.
     * LAST = new plate content · LAST_N = 1 removed | 0 key was absent (soft).
     * DROPPLATE_HIT mirrors removed · DROPPLATE_FILE prior object file.
     * Missing file + miss key → write {} (or leave empty object plate).
     * Soft empty path/key → OK=0. Soft del always OK=1 when write succeeds.
     * Usability: ack/remove agent state field without LOAD+JSONDEL+SAVE glue.
     * Note: PLATEDEL stays key=val KVFILEDEL; this is JSON object plates. */
    if (kw(&L->cur,"DROPPLATE") || kw(&L->cur,"JSONFILEDEL") || kw(&L->cur,"PLATEKEYDEL") ||
        kw(&L->cur,"DELJSONFILE") || kw(&L->cur,"FILEJSONDEL") || kw(&L->cur,"PLATEDROP") ||
        kw(&L->cur,"UNSETPLATEKEY") || kw(&L->cur,"RMPLATEKEY") || kw(&L->cur,"DELPLATEKEY") ||
        kw(&L->cur,"JSONFILERM") || kw(&L->cur,"DROPPLATEKEY")){
      char path[CUBALC_HOST_STR_MAX], key[96], plate[CUBALC_HOST_STR_MAX];
      char absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
      const char *slash, *body;
      size_t n;
      int file_hit = 0, created_file = 0;
      cubalc_host_result hr, keys, delr;
      lex_next(L);
      path[0] = 0; key[0] = 0; plate[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, "SYS DROPPLATE path key — need plate path");
        return -1;
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long kv = parse_expr(vm, L);
        snprintf(key, sizeof key, "%ld", kv);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, key, sizeof key) != 0) {
          fail(vm, "SYS DROPPLATE path key — need key");
          return -1;
        }
      } else {
        fail(vm, "SYS DROPPLATE path key — need key");
        return -1;
      }
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "DROPPLATE: empty path");
        var_set_str(vm, "ERR", "DROPPLATE: empty path");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DROPPLATE_HIT", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      if (!key[0]) {
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "DROPPLATE: empty key");
        var_set_str(vm, "ERR", "DROPPLATE: empty key");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DROPPLATE_HIT", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_read(path, &hr) == 0 && hr.str[0]) {
        body = hr.str;
        while (*body == ' ' || *body == '\t' || *body == '\n' || *body == '\r')
          body++;
        if (*body == '{') {
          memset(&keys, 0, sizeof keys);
          if (cubalc_host_json_keys(body, &keys) == 0) {
            snprintf(plate, sizeof plate, "%s", body);
            file_hit = 1;
          }
        }
      }
      if (!file_hit)
        snprintf(plate, sizeof plate, "%s", "{}");
      created_file = file_hit ? 0 : 1;
      memset(&delr, 0, sizeof delr);
      if (cubalc_host_json_del(plate, key, &delr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DROPPLATE_HIT", 0);
        var_set_num(vm, "OK", 0);
        if (delr.err[0]) {
          var_set_str(vm, "LAST_ERR", delr.err);
          var_set_str(vm, "ERR", delr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "DROPPLATE: json del fail");
          var_set_str(vm, "ERR", "DROPPLATE: json del fail");
        }
        var_set_str(vm, "DROPPLATE_PATH", path);
        var_set_str(vm, "DROPPLATE_KEY", key);
        bump(vm); return 1;
      }
      snprintf(plate, sizeof plate, "%s", delr.str);
      body = plate;
      /* if key was absent and file already existed with same plate, still rewrite is fine */
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
        } else {
          parent[0] = '.';
          parent[1] = 0;
        }
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
        snprintf(absparent, sizeof absparent, "%s", hr.str);
      else
        snprintf(absparent, sizeof absparent, "%s", parent);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_mkdir(absparent, &hr) != 0) {
        const char *err = hr.err[0] ? hr.err : "DROPPLATE: mkdir parent failed";
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        var_set_str(vm, "LAST_ERR", err);
        var_set_str(vm, "ERR", err);
        var_set_str(vm, "DROPPLATE_PATH", absfile);
        var_set_str(vm, "DROPPLATE_KEY", key);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DROPPLATE_HIT", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      if (hr.str[0])
        snprintf(absparent, sizeof absparent, "%s", hr.str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(absfile, body, &hr) != 0) {
        const char *err = hr.err[0] ? hr.err : "DROPPLATE: write failed";
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        var_set_str(vm, "LAST_ERR", err);
        var_set_str(vm, "ERR", err);
        var_set_str(vm, "DROPPLATE_PATH", absfile);
        var_set_str(vm, "DROPPLATE_KEY", key);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DROPPLATE_HIT", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = delr.n;
      var_set_num(vm, "LAST_N", delr.n);
      var_set_str(vm, "DROPPLATE", plate);
      var_set_str(vm, "JSONFILEDEL", plate);
      var_set_str(vm, "PLATEKEYDEL", plate);
      var_set_str(vm, "DROPPLATE_PATH", absfile);
      var_set_str(vm, "DROPPLATE_KEY", key);
      var_set_str(vm, "DROPPLATE_PARENT", absparent);
      var_set_num(vm, "DROPPLATE_HIT", delr.n);
      var_set_num(vm, "DROPPLATE_FILE", file_hit);
      var_set_num(vm, "DROPPLATE_CREATED", created_file);
      var_set_num(vm, "DROPPLATE_BYTES", hr.n);
      var_set_num(vm, "JSONDEL_N", delr.n);
      var_set_num(vm, "JSONDEL_HIT", delr.n);
      var_set_str(vm, "JSONDEL_KEY", key);
      var_set_num(vm, "SAVEPLATE_N", hr.n);
      var_set_num(vm, "SAVEPLATE_OK", 1);
      var_set_str(vm, "SAVEPLATE_PATH", absfile);
      var_set_num(vm, "LOADPLATE_HIT", file_hit);
      var_set_num(vm, "WRITE_OK", 1);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys dropplate → path=%s key=%s removed=%ld file=%d\n",
                absfile, key, delr.n, file_hit);
      bump(vm); return 1;
    }
    /* SYS JSONFILEMERGE|OVERLAYPLATE|MERGEPLATEFILE path overlay
     * One-shot multi-key plate file update: soft-load object (miss→{}),
     * JSONMERGE overlay (overlay wins), mkdir parent + write.
     * LAST = new plate · LAST_N = keys applied (JSONMERGE_N).
     * JSONFILEMERGE_FILE = prior object hit · CREATED if file was new.
     * Usability: apply partial agent status without EACH+UPDATEPLATE or LOAD+MERGE+SAVE.
     * Note: MERGEPLATE/PLATEMERGE stay in-memory JSONMERGE aliases. */
    if (kw(&L->cur,"JSONFILEMERGE") || kw(&L->cur,"OVERLAYPLATE") || kw(&L->cur,"MERGEPLATEFILE") ||
        kw(&L->cur,"FILEJSONMERGE") || kw(&L->cur,"PLATEOVERLAY") || kw(&L->cur,"APPLYPLATE") ||
        kw(&L->cur,"PATCHPLATEFILE") || kw(&L->cur,"PLATEFILEMERGE") || kw(&L->cur,"MERGEFILEPLATE") ||
        kw(&L->cur,"JSONOVERLAYFILE") || kw(&L->cur,"OVERLAYJSONFILE")){
      char path[CUBALC_HOST_STR_MAX], over[CUBALC_HOST_STR_MAX], plate[CUBALC_HOST_STR_MAX];
      char absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
      const char *slash, *body;
      size_t n;
      int file_hit = 0, created_file = 0;
      cubalc_host_result hr, keys, mer;
      lex_next(L);
      path[0] = 0; over[0] = 0; plate[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, "SYS JSONFILEMERGE path overlay — need plate path");
        return -1;
      }
      if (resolve_str_arg(vm, L, over, sizeof over) != 0) {
        /* allow overlay = LAST when path was explicit */
        if (vm->last_str[0] == '{')
          snprintf(over, sizeof over, "%s", vm->last_str);
        else {
          fail(vm, "SYS JSONFILEMERGE path overlay — need overlay plate");
          return -1;
        }
      }
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "JSONFILEMERGE: empty path");
        var_set_str(vm, "ERR", "JSONFILEMERGE: empty path");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONFILEMERGE_N", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      /* soft load base */
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_read(path, &hr) == 0 && hr.str[0]) {
        body = hr.str;
        while (*body == ' ' || *body == '\t' || *body == '\n' || *body == '\r')
          body++;
        if (*body == '{') {
          memset(&keys, 0, sizeof keys);
          if (cubalc_host_json_keys(body, &keys) == 0) {
            snprintf(plate, sizeof plate, "%s", body);
            file_hit = 1;
          }
        }
      }
      if (!file_hit)
        snprintf(plate, sizeof plate, "%s", "{}");
      created_file = file_hit ? 0 : 1;
      /* normalize overlay: non-object → {} */
      {
        const char *o = over;
        while (*o == ' ' || *o == '\t' || *o == '\n' || *o == '\r') o++;
        if (*o != '{')
          snprintf(over, sizeof over, "%s", "{}");
      }
      memset(&mer, 0, sizeof mer);
      if (cubalc_host_json_merge(plate, over, &mer) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONFILEMERGE_N", 0);
        var_set_num(vm, "OK", 0);
        if (mer.err[0]) {
          var_set_str(vm, "LAST_ERR", mer.err);
          var_set_str(vm, "ERR", mer.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONFILEMERGE: merge fail");
          var_set_str(vm, "ERR", "JSONFILEMERGE: merge fail");
        }
        var_set_str(vm, "JSONFILEMERGE_PATH", path);
        bump(vm); return 1;
      }
      snprintf(plate, sizeof plate, "%s", mer.str);
      body = plate;
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
        } else {
          parent[0] = '.';
          parent[1] = 0;
        }
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
        snprintf(absparent, sizeof absparent, "%s", hr.str);
      else
        snprintf(absparent, sizeof absparent, "%s", parent);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_mkdir(absparent, &hr) != 0) {
        const char *err = hr.err[0] ? hr.err : "JSONFILEMERGE: mkdir parent failed";
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        var_set_str(vm, "LAST_ERR", err);
        var_set_str(vm, "ERR", err);
        var_set_str(vm, "JSONFILEMERGE_PATH", absfile);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONFILEMERGE_N", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      if (hr.str[0])
        snprintf(absparent, sizeof absparent, "%s", hr.str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(absfile, body, &hr) != 0) {
        const char *err = hr.err[0] ? hr.err : "JSONFILEMERGE: write failed";
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        var_set_str(vm, "LAST_ERR", err);
        var_set_str(vm, "ERR", err);
        var_set_str(vm, "JSONFILEMERGE_PATH", absfile);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONFILEMERGE_N", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = mer.n;
      var_set_num(vm, "LAST_N", mer.n);
      var_set_str(vm, "JSONFILEMERGE", plate);
      var_set_str(vm, "OVERLAYPLATE", plate);
      var_set_str(vm, "MERGEPLATEFILE", plate);
      var_set_str(vm, "APPLYPLATE", plate);
      var_set_str(vm, "JSONFILEMERGE_PATH", absfile);
      var_set_str(vm, "JSONFILEMERGE_PARENT", absparent);
      var_set_num(vm, "JSONFILEMERGE_N", mer.n);
      var_set_num(vm, "JSONFILEMERGE_FILE", file_hit);
      var_set_num(vm, "JSONFILEMERGE_CREATED", created_file);
      var_set_num(vm, "JSONFILEMERGE_BYTES", hr.n);
      var_set_num(vm, "JSONMERGE_N", mer.n);
      var_set_num(vm, "SAVEPLATE_N", hr.n);
      var_set_num(vm, "SAVEPLATE_OK", 1);
      var_set_str(vm, "SAVEPLATE_PATH", absfile);
      var_set_num(vm, "LOADPLATE_HIT", file_hit);
      var_set_num(vm, "WRITE_OK", 1);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonfilemerge → path=%s applied=%ld file=%d\n",
                absfile, mer.n, file_hit);
      bump(vm); return 1;
    }
    /* SYS JSONFILEENSURE|ENSUREPLATEKEY|DEFAULTPLATEKEY path key value
     * One-shot disk plate default: soft-load object (miss→{}), set key only if
     * missing (JSONENSURE), write only when inserted (or create empty file path).
     * LAST = plate content · LAST_N = 1 inserted | 0 already present.
     * JSONFILEENSURE_SET · HAD · FILE · CREATED knobs.
     * Value rules match JSONSET/UPDATEPLATE (num/true/false/null raw; RAW; strings).
     * Usability: seed agent defaults on disk without clobbering live counters.
     * Note: ENSUREKEY/PLATEDEFAULT stay in-memory JSONENSURE; this is file plane. */
    if (kw(&L->cur,"JSONFILEENSURE") || kw(&L->cur,"ENSUREPLATEKEY") ||
        kw(&L->cur,"DEFAULTPLATEKEY") || kw(&L->cur,"FILEJSONENSURE") ||
        kw(&L->cur,"PLATEKEYENSURE") || kw(&L->cur,"ENSUREJSONFILE") ||
        kw(&L->cur,"DEFAULTJSONFILE") || kw(&L->cur,"SEEDPLATEKEY") ||
        kw(&L->cur,"PLATEFILEENSURE") || kw(&L->cur,"ENSUREFILEKEY")){
      char path[CUBALC_HOST_STR_MAX], key[96], val[CUBALC_HOST_STR_MAX];
      char plate[CUBALC_HOST_STR_MAX], absfile[CUBALC_HOST_STR_MAX];
      char parent[512], absparent[CUBALC_HOST_STR_MAX];
      const char *slash, *body;
      size_t n;
      int file_hit = 0, created_file = 0, present = 0, val_kind = 0, force_raw = 0;
      cubalc_host_result hr, keys, gr, setr;
      lex_next(L);
      path[0] = 0; key[0] = 0; val[0] = 0; plate[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, "SYS JSONFILEENSURE path key value — need plate path");
        return -1;
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long kv = parse_expr(vm, L);
        snprintf(key, sizeof key, "%ld", kv);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, key, sizeof key) != 0) {
          fail(vm, "SYS JSONFILEENSURE path key value — need key");
          return -1;
        }
      } else {
        fail(vm, "SYS JSONFILEENSURE path key value — need key");
        return -1;
      }
      if (kw(&L->cur,"RAW") || kw(&L->cur,"ASRAW") || kw(&L->cur,"LITERAL")) {
        force_raw = 1;
        lex_next(L);
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long nv = parse_expr(vm, L);
        snprintf(val, sizeof val, "%ld", nv);
        val_kind = 1;
      } else if (L->cur.kind == TK_IDENT &&
                 (strcmp(L->cur.text, "true") == 0 || strcmp(L->cur.text, "false") == 0 ||
                  strcmp(L->cur.text, "null") == 0)) {
        snprintf(val, sizeof val, "%s", L->cur.text);
        val_kind = 1;
        lex_next(L);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, val, sizeof val) != 0) {
          fail(vm, "SYS JSONFILEENSURE path key value — need value");
          return -1;
        }
        val_kind = 0;
      } else {
        fail(vm, "SYS JSONFILEENSURE path key value — need value");
        return -1;
      }
      if (force_raw) val_kind = 1;
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "JSONFILEENSURE: empty path");
        var_set_str(vm, "ERR", "JSONFILEENSURE: empty path");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONFILEENSURE_SET", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      if (!key[0]) {
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "JSONFILEENSURE: empty key");
        var_set_str(vm, "ERR", "JSONFILEENSURE: empty key");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONFILEENSURE_SET", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_read(path, &hr) == 0 && hr.str[0]) {
        body = hr.str;
        while (*body == ' ' || *body == '\t' || *body == '\n' || *body == '\r')
          body++;
        if (*body == '{') {
          memset(&keys, 0, sizeof keys);
          if (cubalc_host_json_keys(body, &keys) == 0) {
            snprintf(plate, sizeof plate, "%s", body);
            file_hit = 1;
          }
        }
      }
      if (!file_hit)
        snprintf(plate, sizeof plate, "%s", "{}");
      created_file = file_hit ? 0 : 1;
      memset(&gr, 0, sizeof gr);
      present = (cubalc_host_json_get(plate, key, &gr) == 0) ? 1 : 0;
      if (present) {
        /* no write needed — keep live value */
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "JSONFILEENSURE", plate);
        var_set_str(vm, "ENSUREPLATEKEY", plate);
        var_set_str(vm, "JSONFILEENSURE_PATH", path);
        var_set_str(vm, "JSONFILEENSURE_KEY", key);
        var_set_num(vm, "JSONFILEENSURE_SET", 0);
        var_set_num(vm, "JSONFILEENSURE_HAD", 1);
        var_set_num(vm, "JSONFILEENSURE_FILE", file_hit);
        var_set_num(vm, "JSONFILEENSURE_CREATED", 0);
        var_set_num(vm, "JSONENSURE_SET", 0);
        var_set_num(vm, "JSONENSURE_HAD", 1);
        var_set_num(vm, "LOADPLATE_HIT", file_hit);
        var_set_num(vm, "OK", 1);
        if (vm->trace)
          fprintf(vm->trace, "# sys jsonfileensure → key=%s kept path=%s\n", key, path);
        bump(vm); return 1;
      }
      memset(&setr, 0, sizeof setr);
      if (cubalc_host_json_set(plate, key, val, val_kind, &setr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONFILEENSURE_SET", 0);
        var_set_num(vm, "OK", 0);
        if (setr.err[0]) {
          var_set_str(vm, "LAST_ERR", setr.err);
          var_set_str(vm, "ERR", setr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONFILEENSURE: json set fail");
          var_set_str(vm, "ERR", "JSONFILEENSURE: json set fail");
        }
        var_set_str(vm, "JSONFILEENSURE_PATH", path);
        var_set_str(vm, "JSONFILEENSURE_KEY", key);
        bump(vm); return 1;
      }
      snprintf(plate, sizeof plate, "%s", setr.str);
      body = plate;
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
        } else {
          parent[0] = '.';
          parent[1] = 0;
        }
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
        snprintf(absparent, sizeof absparent, "%s", hr.str);
      else
        snprintf(absparent, sizeof absparent, "%s", parent);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_mkdir(absparent, &hr) != 0) {
        const char *err = hr.err[0] ? hr.err : "JSONFILEENSURE: mkdir parent failed";
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        var_set_str(vm, "LAST_ERR", err);
        var_set_str(vm, "ERR", err);
        var_set_str(vm, "JSONFILEENSURE_PATH", absfile);
        var_set_str(vm, "JSONFILEENSURE_KEY", key);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONFILEENSURE_SET", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      if (hr.str[0])
        snprintf(absparent, sizeof absparent, "%s", hr.str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(absfile, body, &hr) != 0) {
        const char *err = hr.err[0] ? hr.err : "JSONFILEENSURE: write failed";
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        var_set_str(vm, "LAST_ERR", err);
        var_set_str(vm, "ERR", err);
        var_set_str(vm, "JSONFILEENSURE_PATH", absfile);
        var_set_str(vm, "JSONFILEENSURE_KEY", key);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONFILEENSURE_SET", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_str(vm, "JSONFILEENSURE", plate);
      var_set_str(vm, "ENSUREPLATEKEY", plate);
      var_set_str(vm, "DEFAULTPLATEKEY", plate);
      var_set_str(vm, "JSONFILEENSURE_PATH", absfile);
      var_set_str(vm, "JSONFILEENSURE_KEY", key);
      var_set_str(vm, "JSONFILEENSURE_PARENT", absparent);
      var_set_num(vm, "JSONFILEENSURE_SET", 1);
      var_set_num(vm, "JSONFILEENSURE_HAD", 0);
      var_set_num(vm, "JSONFILEENSURE_FILE", file_hit);
      var_set_num(vm, "JSONFILEENSURE_CREATED", created_file);
      var_set_num(vm, "JSONFILEENSURE_BYTES", hr.n);
      var_set_num(vm, "JSONENSURE_SET", 1);
      var_set_num(vm, "JSONENSURE_HAD", 0);
      var_set_str(vm, "JSONENSURE_KEY", key);
      var_set_num(vm, "SAVEPLATE_N", hr.n);
      var_set_num(vm, "SAVEPLATE_OK", 1);
      var_set_str(vm, "SAVEPLATE_PATH", absfile);
      var_set_num(vm, "LOADPLATE_HIT", file_hit);
      var_set_num(vm, "WRITE_OK", 1);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonfileensure → key=%s inserted path=%s\n",
                key, absfile);
      bump(vm); return 1;
    }
    /* SYS JSONFILEDEFAULTS|DEFAULTPLATEALL|ENSUREPLATEALL path defaults
     * One-shot multi-key disk defaults: soft-load object (miss→{}),
     * JSONDEFAULTS from defaults plate (existing keys win), write if any key
     * inserted or file was missing.
     * LAST = plate · LAST_N = keys inserted · JSONFILEDEFAULTS_HAD = skipped count.
     * Usability: multi-key agent boot on disk without multi JSONFILEENSURE or clobber.
     * Dual of JSONFILEMERGE (overlay wins). PLATEDEFAULTS stays in-memory. */
    if (kw(&L->cur,"JSONFILEDEFAULTS") || kw(&L->cur,"DEFAULTPLATEALL") ||
        kw(&L->cur,"ENSUREPLATEALL") || kw(&L->cur,"FILEJSONDEFAULTS") ||
        kw(&L->cur,"PLATEFILEDEFAULTS") || kw(&L->cur,"SEEDPLATEALL") ||
        kw(&L->cur,"DEFAULTSFILE") || kw(&L->cur,"BOOTPLATEDEFAULTS") ||
        kw(&L->cur,"JSONENSUREFILEALL") || kw(&L->cur,"FILLPLATEDEFAULTS")){
      char path[CUBALC_HOST_STR_MAX], defs[CUBALC_HOST_STR_MAX], plate[CUBALC_HOST_STR_MAX];
      char absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
      const char *slash, *body;
      size_t n;
      int file_hit = 0, created_file = 0;
      cubalc_host_result hr, keys, defr;
      lex_next(L);
      path[0] = 0; defs[0] = 0; plate[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, "SYS JSONFILEDEFAULTS path defaults — need plate path");
        return -1;
      }
      if (kw(&L->cur,"FROM") || kw(&L->cur,"WITH") || kw(&L->cur,"USING") ||
          kw(&L->cur,"OF") || kw(&L->cur,"DEFAULTS") || kw(&L->cur,"BASE"))
        lex_next(L);
      if (resolve_str_arg(vm, L, defs, sizeof defs) != 0) {
        if (vm->last_str[0] == '{')
          snprintf(defs, sizeof defs, "%s", vm->last_str);
        else {
          fail(vm, "SYS JSONFILEDEFAULTS path defaults — need defaults plate");
          return -1;
        }
      }
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "JSONFILEDEFAULTS: empty path");
        var_set_str(vm, "ERR", "JSONFILEDEFAULTS: empty path");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONFILEDEFAULTS_N", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      {
        const char *d = defs;
        while (*d == ' ' || *d == '\t' || *d == '\n' || *d == '\r') d++;
        if (*d != '{')
          snprintf(defs, sizeof defs, "%s", "{}");
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_read(path, &hr) == 0 && hr.str[0]) {
        body = hr.str;
        while (*body == ' ' || *body == '\t' || *body == '\n' || *body == '\r')
          body++;
        if (*body == '{') {
          memset(&keys, 0, sizeof keys);
          if (cubalc_host_json_keys(body, &keys) == 0) {
            snprintf(plate, sizeof plate, "%s", body);
            file_hit = 1;
          }
        }
      }
      if (!file_hit)
        snprintf(plate, sizeof plate, "%s", "{}");
      created_file = file_hit ? 0 : 1;
      memset(&defr, 0, sizeof defr);
      if (cubalc_host_json_defaults(plate, defs, &defr) != 0) {
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONFILEDEFAULTS_N", 0);
        var_set_num(vm, "OK", 0);
        if (defr.err[0]) {
          var_set_str(vm, "LAST_ERR", defr.err);
          var_set_str(vm, "ERR", defr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONFILEDEFAULTS: fail");
          var_set_str(vm, "ERR", "JSONFILEDEFAULTS: fail");
        }
        var_set_str(vm, "JSONFILEDEFAULTS_PATH", path);
        bump(vm); return 1;
      }
      snprintf(plate, sizeof plate, "%s", defr.str);
      /* skip write if nothing inserted and file already existed */
      if (defr.n == 0 && file_hit) {
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "JSONFILEDEFAULTS", plate);
        var_set_str(vm, "DEFAULTPLATEALL", plate);
        var_set_str(vm, "JSONFILEDEFAULTS_PATH", path);
        var_set_num(vm, "JSONFILEDEFAULTS_N", 0);
        var_set_num(vm, "JSONFILEDEFAULTS_HAD", defr.code);
        var_set_num(vm, "JSONFILEDEFAULTS_SET", 0);
        var_set_num(vm, "JSONFILEDEFAULTS_FILE", file_hit);
        var_set_num(vm, "JSONFILEDEFAULTS_CREATED", 0);
        var_set_num(vm, "JSONDEFAULTS_N", 0);
        var_set_num(vm, "JSONDEFAULTS_HAD", defr.code);
        var_set_num(vm, "LOADPLATE_HIT", file_hit);
        var_set_num(vm, "OK", 1);
        if (vm->trace)
          fprintf(vm->trace, "# sys jsonfiledefaults → no-op path=%s had=%d\n",
                  path, defr.code);
        bump(vm); return 1;
      }
      body = plate;
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
        } else {
          parent[0] = '.';
          parent[1] = 0;
        }
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
        snprintf(absparent, sizeof absparent, "%s", hr.str);
      else
        snprintf(absparent, sizeof absparent, "%s", parent);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_mkdir(absparent, &hr) != 0) {
        const char *err = hr.err[0] ? hr.err : "JSONFILEDEFAULTS: mkdir parent failed";
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        var_set_str(vm, "LAST_ERR", err);
        var_set_str(vm, "ERR", err);
        var_set_str(vm, "JSONFILEDEFAULTS_PATH", absfile);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONFILEDEFAULTS_N", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      if (hr.str[0])
        snprintf(absparent, sizeof absparent, "%s", hr.str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(absfile, body, &hr) != 0) {
        const char *err = hr.err[0] ? hr.err : "JSONFILEDEFAULTS: write failed";
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        var_set_str(vm, "LAST_ERR", err);
        var_set_str(vm, "ERR", err);
        var_set_str(vm, "JSONFILEDEFAULTS_PATH", absfile);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONFILEDEFAULTS_N", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = defr.n;
      var_set_num(vm, "LAST_N", defr.n);
      var_set_str(vm, "JSONFILEDEFAULTS", plate);
      var_set_str(vm, "DEFAULTPLATEALL", plate);
      var_set_str(vm, "ENSUREPLATEALL", plate);
      var_set_str(vm, "JSONFILEDEFAULTS_PATH", absfile);
      var_set_str(vm, "JSONFILEDEFAULTS_PARENT", absparent);
      var_set_num(vm, "JSONFILEDEFAULTS_N", defr.n);
      var_set_num(vm, "JSONFILEDEFAULTS_HAD", defr.code);
      var_set_num(vm, "JSONFILEDEFAULTS_SET", defr.n);
      var_set_num(vm, "JSONFILEDEFAULTS_FILE", file_hit);
      var_set_num(vm, "JSONFILEDEFAULTS_CREATED", created_file);
      var_set_num(vm, "JSONFILEDEFAULTS_BYTES", hr.n);
      var_set_num(vm, "JSONDEFAULTS_N", defr.n);
      var_set_num(vm, "JSONDEFAULTS_HAD", defr.code);
      var_set_num(vm, "SAVEPLATE_N", hr.n);
      var_set_num(vm, "SAVEPLATE_OK", 1);
      var_set_str(vm, "SAVEPLATE_PATH", absfile);
      var_set_num(vm, "LOADPLATE_HIT", file_hit);
      var_set_num(vm, "WRITE_OK", 1);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonfiledefaults → path=%s set=%ld had=%d file=%d\n",
                absfile, defr.n, defr.code, file_hit);
      bump(vm); return 1;
    }
    /* SYS JSONFILETOGGLE|TOGGLEPLATEKEY|FLIPPLATEKEY path key
     * One-shot flip disk plate flag 0↔1: soft-load (miss→{}), JSONTOGGLE key,
     * mkdir parent + write. LAST = plate · LAST_N = new 0|1.
     * Missing/false/0/empty/null → 1; truthy → 0 (same as JSONTOGGLE).
     * JSONFILETOGGLE_V · PREV · FILE · CREATED knobs.
     * Usability: agent feature flags without LOAD+JSONTOGGLE+SAVE glue.
     * Note: PLATETOGGLE/TOGGLEKEY stay in-memory JSONTOGGLE. */
    if (kw(&L->cur,"JSONFILETOGGLE") || kw(&L->cur,"TOGGLEPLATEKEY") ||
        kw(&L->cur,"FLIPPLATEKEY") || kw(&L->cur,"FILEJSONTOGGLE") ||
        kw(&L->cur,"TOGGLEJSONFILE") || kw(&L->cur,"PLATEFILETOGGLE") ||
        kw(&L->cur,"FLIPJSONFILE") || kw(&L->cur,"JSONFILEFLIP") ||
        kw(&L->cur,"TOGGLEFILEKEY") || kw(&L->cur,"FLIPFILEKEY")){
      char path[CUBALC_HOST_STR_MAX], key[96], plate[CUBALC_HOST_STR_MAX];
      char raw[8], absfile[CUBALC_HOST_STR_MAX], parent[512], absparent[CUBALC_HOST_STR_MAX];
      const char *slash, *body;
      size_t n;
      int file_hit = 0, created_file = 0;
      long cur = 0, nv = 0;
      cubalc_host_result hr, keys, gr, setr;
      lex_next(L);
      path[0] = 0; key[0] = 0; plate[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, "SYS JSONFILETOGGLE path key — need plate path");
        return -1;
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        long kv = parse_expr(vm, L);
        snprintf(key, sizeof key, "%ld", kv);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, key, sizeof key) != 0) {
          fail(vm, "SYS JSONFILETOGGLE path key — need key");
          return -1;
        }
      } else {
        fail(vm, "SYS JSONFILETOGGLE path key — need key");
        return -1;
      }
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "JSONFILETOGGLE: empty path");
        var_set_str(vm, "ERR", "JSONFILETOGGLE: empty path");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONFILETOGGLE_V", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      if (!key[0]) {
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "JSONFILETOGGLE: empty key");
        var_set_str(vm, "ERR", "JSONFILETOGGLE: empty key");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONFILETOGGLE_V", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_read(path, &hr) == 0 && hr.str[0]) {
        body = hr.str;
        while (*body == ' ' || *body == '\t' || *body == '\n' || *body == '\r')
          body++;
        if (*body == '{') {
          memset(&keys, 0, sizeof keys);
          if (cubalc_host_json_keys(body, &keys) == 0) {
            snprintf(plate, sizeof plate, "%s", body);
            file_hit = 1;
          }
        }
      }
      if (!file_hit)
        snprintf(plate, sizeof plate, "%s", "{}");
      created_file = file_hit ? 0 : 1;
      memset(&gr, 0, sizeof gr);
      if (cubalc_host_json_get(plate, key, &gr) == 0) {
        char *end = NULL;
        if (!gr.str[0]) {
          cur = 0;
        } else {
          cur = strtol(gr.str, &end, 10);
          if (end == gr.str)
            cur = 1; /* non-numeric non-empty → truthy */
        }
      } else {
        cur = 0; /* missing → off */
      }
      nv = cur ? 0 : 1;
      snprintf(raw, sizeof raw, "%ld", nv);
      memset(&setr, 0, sizeof setr);
      if (cubalc_host_json_set(plate, key, raw, 1, &setr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONFILETOGGLE_V", 0);
        var_set_num(vm, "OK", 0);
        if (setr.err[0]) {
          var_set_str(vm, "LAST_ERR", setr.err);
          var_set_str(vm, "ERR", setr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "JSONFILETOGGLE: json set fail");
          var_set_str(vm, "ERR", "JSONFILETOGGLE: json set fail");
        }
        var_set_str(vm, "JSONFILETOGGLE_PATH", path);
        var_set_str(vm, "JSONFILETOGGLE_KEY", key);
        bump(vm); return 1;
      }
      snprintf(plate, sizeof plate, "%s", setr.str);
      body = plate;
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
        } else {
          parent[0] = '.';
          parent[1] = 0;
        }
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_abspath(parent, &hr) == 0 && hr.str[0])
        snprintf(absparent, sizeof absparent, "%s", hr.str);
      else
        snprintf(absparent, sizeof absparent, "%s", parent);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_mkdir(absparent, &hr) != 0) {
        const char *err = hr.err[0] ? hr.err : "JSONFILETOGGLE: mkdir parent failed";
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        var_set_str(vm, "LAST_ERR", err);
        var_set_str(vm, "ERR", err);
        var_set_str(vm, "JSONFILETOGGLE_PATH", absfile);
        var_set_str(vm, "JSONFILETOGGLE_KEY", key);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONFILETOGGLE_V", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      if (hr.str[0])
        snprintf(absparent, sizeof absparent, "%s", hr.str);
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_write(absfile, body, &hr) != 0) {
        const char *err = hr.err[0] ? hr.err : "JSONFILETOGGLE: write failed";
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        var_set_str(vm, "LAST_ERR", err);
        var_set_str(vm, "ERR", err);
        var_set_str(vm, "JSONFILETOGGLE_PATH", absfile);
        var_set_str(vm, "JSONFILETOGGLE_KEY", key);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "JSONFILETOGGLE_V", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = nv;
      var_set_num(vm, "LAST_N", nv);
      var_set_str(vm, "JSONFILETOGGLE", plate);
      var_set_str(vm, "TOGGLEPLATEKEY", plate);
      var_set_str(vm, "FLIPPLATEKEY", plate);
      var_set_str(vm, "JSONFILETOGGLE_PATH", absfile);
      var_set_str(vm, "JSONFILETOGGLE_KEY", key);
      var_set_str(vm, "JSONFILETOGGLE_PARENT", absparent);
      var_set_num(vm, "JSONFILETOGGLE_V", nv);
      var_set_num(vm, "JSONFILETOGGLE_PREV", cur ? 1 : 0);
      var_set_num(vm, "JSONFILETOGGLE_FILE", file_hit);
      var_set_num(vm, "JSONFILETOGGLE_CREATED", created_file);
      var_set_num(vm, "JSONFILETOGGLE_BYTES", hr.n);
      var_set_num(vm, "JSONTOGGLE_V", nv);
      var_set_num(vm, "JSONTOGGLE_PREV", cur ? 1 : 0);
      var_set_str(vm, "JSONTOGGLE_KEY", key);
      var_set_num(vm, "SAVEPLATE_N", hr.n);
      var_set_num(vm, "SAVEPLATE_OK", 1);
      var_set_str(vm, "SAVEPLATE_PATH", absfile);
      var_set_num(vm, "LOADPLATE_HIT", file_hit);
      var_set_num(vm, "WRITE_OK", 1);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# sys jsonfiletoggle → path=%s key=%s %ld→%ld\n",
                absfile, key, cur ? 1L : 0L, nv);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"WRITE")){
      lex_next(L);
      char path[512]; path[0]=0;
      if (L->cur.kind==TK_STR){ snprintf(path,sizeof path,"%s",L->cur.text); lex_next(L); }
      else if (L->cur.kind==TK_IDENT){
        if (strcmp(L->cur.text,"LAST")==0) snprintf(path,sizeof path,"%s",vm->last_str);
        else {
          Var *v = var_get(vm, L->cur.text, 0);
          if (v && v->is_str) snprintf(path,sizeof path,"%s",v->sval);
          else { fail(vm,"SYS WRITE path"); return -1; }
        }
        lex_next(L);
      } else { fail(vm,"SYS WRITE \"path\" \"data\""); return -1; }
      const char *data = "";
      char dbuf[CUBALC_HOST_STR_MAX];
      if (L->cur.kind==TK_STR){ snprintf(dbuf,sizeof dbuf,"%s",L->cur.text); data=dbuf; lex_next(L); }
      else if (L->cur.kind==TK_IDENT){
        Var *v = var_get(vm, L->cur.text, 0);
        if (v && v->is_str) data = v->sval;
        else if (strcmp(L->cur.text,"LAST")==0) data = vm->last_str;
        else data = "";
        lex_next(L);
      }
      cubalc_host_result hr;
      if (cubalc_host_write(path, data, &hr)!=0){ fail(vm, hr.err[0]?hr.err:"SYS WRITE fail"); return -1; }
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
  return 0;
}
