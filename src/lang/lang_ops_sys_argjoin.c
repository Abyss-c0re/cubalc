/* CubalC lang — lang_ops_sys_argjoin.c
 * SYS JOIN/PATH · ARGC/ARGS/ARG
 * Peel from core_sys residual (B_core_sys). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_sys_argjoin.h"
#include "lang/cubalc_lang_internal.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cubalc_lang_ops_sys_try_argjoin(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. SYS already consumed. */

  /* SYS JOIN|PATH a b → LAST = a/b (portable path join, no double slash).
   * Usability: build plate paths from STATE/TMP/LIST names without shell. */
  if (kw(&L->cur,"JOIN") || kw(&L->cur,"PATH") || kw(&L->cur,"PATHJOIN") ||
      kw(&L->cur,"JOINPATH")){
    lex_next(L);
    char a[512]="", b[512]="";
    cubalc_host_result hr;
    if (resolve_str_arg(vm, L, a, sizeof a) != 0){
      fail(vm,"SYS JOIN a b"); return -1;
    }
    if (resolve_str_arg(vm, L, b, sizeof b) != 0){
      fail(vm,"SYS JOIN a b"); return -1;
    }
    if (cubalc_host_join(a,b,&hr)!=0){ fail(vm, hr.err[0]?hr.err:"JOIN"); return -1; }
    snprintf(vm->last_str,sizeof vm->last_str,"%s",hr.str);
    vm->last_n = hr.n;
    var_set_str(vm,"LAST",hr.str);
    var_set_str(vm,"JOIN",hr.str);
    var_set_num(vm,"LAST_N",hr.n);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }

  /* SYS ARGC|ARGCOUNT — number of program args (CUBALC_ARGC or scan ARG0..) */
  if (kw(&L->cur,"ARGC") || kw(&L->cur,"ARGCOUNT") || kw(&L->cur,"NARGS")){
    long n = 0;
    const char *ac = getenv("CUBALC_ARGC");
    lex_next(L);
    if (ac && ac[0]) {
      n = strtol(ac, NULL, 10);
      if (n < 0) n = 0;
      if (n > 32) n = 32;
    } else {
      int k;
      for (k = 0; k < 32; k++) {
        char nm[32];
        const char *v;
        snprintf(nm, sizeof nm, "CUBALC_ARG%d", k);
        v = getenv(nm);
        if (!v) break;
        n++;
      }
    }
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "ARGC", n);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }

  /* SYS ARGS|ARGVALL|ALLARGS — newline bag of CUBALC_ARG0.. · LAST_N=count.
   * Usability: walk program args without SYS ARG 0/1/2 glue. */
  if (kw(&L->cur,"ARGS") || kw(&L->cur,"ARGVALL") || kw(&L->cur,"ALLARGS") ||
      kw(&L->cur,"ARGBAG")){
    char bag[CUBALC_HOST_STR_MAX];
    size_t o = 0;
    long n = 0;
    int k;
    lex_next(L);
    bag[0] = 0;
    for (k = 0; k < 32; k++) {
      char nm[32];
      const char *v;
      size_t vl;
      snprintf(nm, sizeof nm, "CUBALC_ARG%d", k);
      v = getenv(nm);
      if (!v) break;
      vl = strlen(v);
      if (n && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + vl >= sizeof bag) {
        if (o + 1 < sizeof bag) {
          vl = sizeof bag - o - 1;
          memcpy(bag + o, v, vl);
          o += vl;
          bag[o] = 0;
        }
        n++;
        break;
      }
      memcpy(bag + o, v, vl + 1);
      o += vl;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "ARGS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "ARGC", n);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }

  /* SYS ARG n|name [OR "fallback"] — CUBALC_ARGn / named env with default */
  if (kw(&L->cur,"ARG") || kw(&L->cur,"ARGV")){
    lex_next(L);
    char name[64];
    if (L->cur.kind==TK_NUM){
      snprintf(name,sizeof name,"CUBALC_ARG%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT){
      if (strcmp(L->cur.text,"MSG")==0) snprintf(name,sizeof name,"GROKIUM_MSG");
      else if (strcmp(L->cur.text,"BACKEND")==0) snprintf(name,sizeof name,"GROKIUM_BACKEND");
      else if (strcmp(L->cur.text,"MODEL")==0) snprintf(name,sizeof name,"GROKIUM_MODEL");
      else snprintf(name,sizeof name,"%s", L->cur.text);
      lex_next(L);
    } else { fail(vm,"SYS ARG n|name [OR fallback]"); return -1; }
    cubalc_host_result hr;
    cubalc_host_env(name, &hr);
    if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
        kw(&L->cur,"FALLBACK")){
      lex_next(L);
      char fb[512];
      if (resolve_str_arg(vm, L, fb, sizeof fb) != 0){
        fail(vm,"SYS ARG n|name OR \"fallback\""); return -1;
      }
      if (!hr.str[0] || hr.n <= 0){
        snprintf(hr.str, sizeof hr.str, "%s", fb);
        hr.n = (long)strlen(hr.str);
        hr.ok = 1;
      }
    }
    snprintf(vm->last_str,sizeof vm->last_str,"%s",hr.str);
    vm->last_n = hr.n;
    var_set_str(vm,"LAST",hr.str);
    var_set_num(vm,"LAST_N",hr.n);
    var_set_num(vm,"OK", hr.n > 0 ? 1 : 0);
    bump(vm); return 1;
  }

  return 0;
}
