/* CubalC lang — lang_ops_sys_files_net.c
 * SYS READALL · WHICH · HTTP/GET/POST
 * Peel from sys_files. Leaves pure dispatcher. Cube is SoT.
 */
#include "lang/cubalc_lang_ops_sys_files_net.h"
#include "lang/cubalc_lang_ops_sys_files_bulk.h"
#include "lang/cubalc_lang_ops_sys_files_match.h"
#include "lang/cubalc_lang_ops_sys_files_line.h"
#include "lang/cubalc_lang_ops_sys_files_cwd.h"
#include "lang/cubalc_lang_ops_sys_plate.h"
#include "lang/cubalc_lang_ops_sys_path_fs.h"
#include "lang/cubalc_lang_ops_sys_env.h"
#include "lang/cubalc_lang_ops_sys_string.h"
#include "lang/cubalc_lang_ops_sys_wait.h"
#include "lang/cubalc_lang_ops_sys_stat.h"
#include "lang/cubalc_lang_ops_sys_bulk.h"
#include "lang/cubalc_lang_ops_sys_bulk_mut.h"
#include "lang/cubalc_lang_ops_sys_net.h"
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
int cubalc_lang_ops_sys_try_files_net(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. SYS already consumed. */
  /* SYS READALL|CATFILES|SLURPALL bag [sep]
   * — concatenate contents of every path in bag → LAST.
   * Optional sep between files (default empty). Soft miss skips unreadable.
   * LAST_N / READALL_N = files read; READALL_BYTES · READALL_MISS · READALL_SCAN.
   * READALL_PATHS = bag of successfully read paths.
   * Zero-arg: bag = prior LAST. One-arg: bag. Two-arg: bag sep.
   * Usability: multi-file plate/log join after PATHGLOB/GREPFILES without EACH+READ+CAT. */
  if (kw(&L->cur,"READALL") || kw(&L->cur,"CATFILES") || kw(&L->cur,"SLURPALL") ||
      kw(&L->cur,"CATALL") || kw(&L->cur,"CONCATFILES") || kw(&L->cur,"READFILES") ||
      kw(&L->cur,"BULKREAD") || kw(&L->cur,"FILECAT") || kw(&L->cur,"JOINFILES")){
    char bag[CUBALC_HOST_STR_MAX], sep[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
    char paths[CUBALC_HOST_STR_MAX], a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
    const char *p, *start;
    size_t flen, olen = 0, plen = 0, seplen;
    long okc = 0, nscan = 0, nmiss = 0, bytes = 0;
    int first = 1;
    lex_next(L);
    bag[0] = 0; sep[0] = 0; out[0] = 0; paths[0] = 0; a[0] = 0; b[0] = 0;
    if (resolve_str_arg(vm, L, a, sizeof a) == 0) {
      if (resolve_str_arg(vm, L, b, sizeof b) == 0) {
        snprintf(bag, sizeof bag, "%s", a);
        snprintf(sep, sizeof sep, "%s", b);
      } else {
        snprintf(bag, sizeof bag, "%s", a);
      }
    } else {
      snprintf(bag, sizeof bag, "%s", vm->last_str);
    }
    seplen = strlen(sep);
    p = bag;
    while (*p) {
      start = p;
      while (*p && *p != '\n') p++;
      flen = (size_t)(p - start);
      if (flen > 0) {
        char path[512];
        size_t take = flen, clen;
        cubalc_host_result hr;
        nscan++;
        if (take >= sizeof path) take = sizeof path - 1;
        memcpy(path, start, take);
        path[take] = 0;
        if (cubalc_host_read(path, &hr) != 0) {
          nmiss++;
          if (nmiss == 1) {
            var_set_str(vm, "LAST_ERR", "READALL: read fail");
            var_set_str(vm, "ERR", "READALL: read fail");
          }
        } else {
          clen = strlen(hr.str);
          if (!first && seplen > 0) {
            if (olen + seplen < sizeof out) {
              memcpy(out + olen, sep, seplen);
              olen += seplen;
            }
          }
          first = 0;
          if (olen + clen < sizeof out) {
            memcpy(out + olen, hr.str, clen);
            olen += clen;
          } else if (olen < sizeof out - 1) {
            size_t room = sizeof out - 1 - olen;
            memcpy(out + olen, hr.str, room);
            olen += room;
          }
          out[olen] = 0;
          if (okc > 0 && plen + 1 < sizeof paths) paths[plen++] = '\n';
          if (plen + take < sizeof paths) {
            memcpy(paths + plen, path, take);
            plen += take;
          }
          paths[plen] = 0;
          okc++;
          bytes += (long)clen;
        }
      }
      if (*p == '\n') p++;
    }
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = okc;
    var_set_num(vm, "LAST_N", okc);
    var_set_str(vm, "READALL", out);
    var_set_str(vm, "CATFILES", out);
    var_set_str(vm, "SLURPALL", out);
    var_set_str(vm, "READALL_PATHS", paths);
    var_set_num(vm, "READALL_N", okc);
    var_set_num(vm, "CATFILES_N", okc);
    var_set_num(vm, "READALL_BYTES", bytes);
    var_set_num(vm, "READALL_MISS", nmiss);
    var_set_num(vm, "READALL_SCAN", nscan);
    var_set_num(vm, "OK", nmiss == 0 ? 1 : 0);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"WHICH") || kw(&L->cur,"WHICHBIN") || kw(&L->cur,"PATHWHICH") ||
      kw(&L->cur,"BIN") || kw(&L->cur,"WHICH_BIN") || kw(&L->cur,"COMMAND") ||
      kw(&L->cur,"WHEREIS")){
    int bin_only = 0;
    cubalc_host_result hr;
    if (kw(&L->cur,"WHICHBIN") || kw(&L->cur,"PATHWHICH") || kw(&L->cur,"BIN") ||
        kw(&L->cur,"WHICH_BIN") || kw(&L->cur,"COMMAND") || kw(&L->cur,"WHEREIS"))
      bin_only = 1;
    /* SYS WHICH BIN name — PATH-only (also via WHICHBIN alias) */
    lex_next(L);
    if (!bin_only && (kw(&L->cur,"BIN") || kw(&L->cur,"CMD") || kw(&L->cur,"EXE") ||
                      kw(&L->cur,"PATH") || kw(&L->cur,"COMMAND"))){
      bin_only = 1;
      lex_next(L);
    }
    /* name: "lit" | LAST | string-var | bare IDENT tool token.
     * Usability: SYS WHICHBIN tool with tool="ls" must resolve sval, not
     * the identifier text "tool" (agent tool bags / EACH LINE). */
    char name[256];
    name[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      if (strcmp(L->cur.text, "LAST") == 0) {
        snprintf(name, sizeof name, "%s", vm->last_str);
        lex_next(L);
      } else {
        Var *nv = var_get(vm, L->cur.text, 0);
        if (nv && nv->is_str && nv->sval[0]) {
          snprintf(name, sizeof name, "%s", nv->sval);
          lex_next(L);
        } else {
          /* bare unquoted tool name: WHICHBIN ls */
          snprintf(name, sizeof name, "%s", L->cur.text);
          lex_next(L);
        }
      }
    } else {
      fail(vm, bin_only ? "SYS WHICHBIN name" : "SYS WHICH name");
      return -1;
    }
    if (!name[0]) {
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_str(vm, "LAST", "");
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      if (bin_only) {
        var_set_str(vm, "LAST_ERR", "WHICHBIN: empty name");
        var_set_str(vm, "ERR", "WHICHBIN: empty name");
      }
      bump(vm); return 1;
    }
    /* Usability: WHICHBIN = host executable only; WHICH also finds libs. */
    if ((bin_only ? cubalc_host_which_bin(name, &hr)
                  : cubalc_host_which(name, &hr)) != 0){
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_str(vm, "LAST", "");
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      if (bin_only) {
        char err[120];
        snprintf(err, sizeof err, "WHICHBIN '%s' not found", name);
        var_set_str(vm, "ERR", err);
        var_set_str(vm, "LAST_ERR", err);
      }
    } else {
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = 1;
      var_set_str(vm, "LAST", hr.str);
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "OK", 1);
      if (bin_only) {
        var_set_str(vm, "WHICHBIN", hr.str);
        var_set_str(vm, "BIN", hr.str);
      }
    }
    bump(vm); return 1;
  }

  if (kw(&L->cur,"HTTP") || kw(&L->cur,"GET") || kw(&L->cur,"POST")){
    char method[8] = "GET";
    if (kw(&L->cur,"POST")) snprintf(method,sizeof method,"POST");
    if (kw(&L->cur,"HTTP")){ lex_next(L); if (L->cur.kind==TK_IDENT||L->cur.kind==TK_STR){ snprintf(method,sizeof method,"%s",L->cur.text); lex_next(L);} }
    else lex_next(L);
    if (L->cur.kind!=TK_STR){ fail(vm,"SYS HTTP method \"url\" [\"body\"|FILE \"path\"]"); return -1; }
    char url[512]; snprintf(url,sizeof url,"%s",L->cur.text); lex_next(L);
    char body[CUBALC_HOST_STR_MAX]; body[0]=0;
    if (kw(&L->cur,"FILE") || kw(&L->cur,"FROM")){
      lex_next(L);
      if (L->cur.kind!=TK_STR){ fail(vm,"SYS HTTP ... FILE \"path\""); return -1; }
      cubalc_host_result fr;
      if (cubalc_host_read(L->cur.text, &fr)!=0){ fail(vm, fr.err[0]?fr.err:"body file"); return -1; }
      snprintf(body,sizeof body,"%s", fr.str);
      lex_next(L);
    } else if (L->cur.kind==TK_STR){
      snprintf(body,sizeof body,"%s",L->cur.text); lex_next(L);
    }
    cubalc_host_result hr;
    int rc = cubalc_host_http(method, url, body, &hr);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
    vm->last_code = hr.code;
    vm->last_n = hr.n;
    var_set_str(vm, "LAST", hr.str);
    var_set_num(vm, "HTTP_CODE", hr.code);
    var_set_num(vm, "LAST_N", hr.n);
    var_set_num(vm, "OK", hr.ok ? 1 : 0);
    if (rc!=0 && !hr.ok){
      /* soft-fail for probe: OK=0 not fatal */
      if (vm->trace) fprintf(vm->trace, "# http soft-fail %s\n", hr.err);
    }
    bump(vm); return 1;
  }
  return 0;
}
