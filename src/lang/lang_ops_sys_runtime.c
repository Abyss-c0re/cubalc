/* CubalC lang — lang_ops_sys_runtime.c
 * SYS runtime: TIME/MS/MONO/SLEEP · APPEND/LOG
 * Peel from core_sys residual (B_core_sys). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_sys_runtime.h"
#include "lang/cubalc_lang_ops_sys_time_walk.h"
#include "lang/cubalc_lang_internal.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <unistd.h>
#  include <sys/time.h>
#endif

int cubalc_lang_ops_sys_try_runtime(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. SYS already consumed. */
  if (kw(&L->cur,"TIME") || kw(&L->cur,"NOW") || kw(&L->cur,"EPOCH")){
    lex_next(L);
    long n = (long)time(NULL);
    vm->last_n = n; var_set_num(vm, "LAST_N", n); var_set_num(vm, "TIME", n); var_set_num(vm, "OK", 1);
    char buf[32]; snprintf(buf, sizeof buf, "%ld", n);
    var_set_str(vm, "LAST", buf); snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
    bump(vm); return 1;
  }

  /* SYS MS|MILLIS|TIME_MS|EPOCH_MS — wall clock milliseconds since epoch.
   * Usability: agents need finer stamps than SYS TIME (seconds) for plates/timing. */
  if (kw(&L->cur,"MS") || kw(&L->cur,"MILLIS") || kw(&L->cur,"MILLISECONDS") ||
      kw(&L->cur,"TIME_MS") || kw(&L->cur,"EPOCH_MS") || kw(&L->cur,"NOW_MS")){
    struct timespec ts;
    long n;
    char buf[40];
    lex_next(L);
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
      n = (long)time(NULL) * 1000L;
    } else {
      n = (long)ts.tv_sec * 1000L + (long)(ts.tv_nsec / 1000000L);
    }
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "MS", n);
    var_set_num(vm, "TIME_MS", n);
    var_set_num(vm, "OK", 1);
    snprintf(buf, sizeof buf, "%ld", n);
    var_set_str(vm, "LAST", buf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
    bump(vm); return 1;
  }
  /* SYS MONOTONIC|MONO|CLOCK_MONO|STEADY_MS — CLOCK_MONOTONIC ms.
   * LAST_N / MONOTONIC_N = ms (arbitrary epoch, steady for deltas).
   * Usability: elapsed timing that ignores wall-clock jumps; pairs with MS/SLEEP. */
  if (kw(&L->cur,"MONOTONIC") || kw(&L->cur,"MONO") || kw(&L->cur,"CLOCK_MONO") ||
      kw(&L->cur,"STEADY_MS") || kw(&L->cur,"MONO_MS") || kw(&L->cur,"STEADY") ||
      kw(&L->cur,"MONOTONIC_MS") || kw(&L->cur,"CLOCK_MONOTONIC")){
    struct timespec ts;
    long n;
    char buf[40];
    lex_next(L);
#if defined(CLOCK_MONOTONIC)
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
      n = (long)time(NULL) * 1000L;
    } else {
      n = (long)ts.tv_sec * 1000L + (long)(ts.tv_nsec / 1000000L);
    }
#else
    n = (long)time(NULL) * 1000L;
#endif
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "MONOTONIC", n);
    var_set_num(vm, "MONOTONIC_N", n);
    var_set_num(vm, "MONO", n);
    var_set_num(vm, "OK", 1);
    snprintf(buf, sizeof buf, "%ld", n);
    var_set_str(vm, "LAST", buf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
    bump(vm); return 1;
  }

  /* SYS SLEEP|MSLEEP|DELAY [MS] n|"duration"
   * — pause n milliseconds (cap 60s). Numeric still works (ms).
   * String/LAST/str-var: PARSEMS human duration ("250ms","1s","1s500ms").
   * Soft-fail bad duration (OK=0, no sleep). Usability: backoff without
   * SYS PARSEMS + SLEEP glue; pairs with SYS MS / HUMANMS. */
  if (kw(&L->cur,"SLEEP") || kw(&L->cur,"MSLEEP") || kw(&L->cur,"DELAY") ||
      kw(&L->cur,"PAUSE") || kw(&L->cur,"WAIT_MS")){
    long ms = 0;
    char buf[40], src[256];
    int from_dur = 0;
    const char *derr = NULL;
    lex_next(L);
    /* optional unit keyword MS|MILLIS before or after value is ignored as unit */
    if (kw(&L->cur,"MS") || kw(&L->cur,"MILLIS") || kw(&L->cur,"MILLISECONDS"))
      lex_next(L);
    src[0] = 0;
    if (L->cur.kind == TK_STR) {
      if (resolve_str_arg(vm, L, src, sizeof src) == 0)
        from_dur = 1;
    } else if (L->cur.kind == TK_IDENT) {
      /* String vars / LAST hold duration text; numeric idents use parse_expr. */
      if (strcmp(L->cur.text, "LAST") == 0) {
        if (resolve_str_arg(vm, L, src, sizeof src) == 0)
          from_dur = 1;
      } else {
        Var *sv = var_get(vm, L->cur.text, 0);
        if (sv && sv->is_str) {
          if (resolve_str_arg(vm, L, src, sizeof src) == 0)
            from_dur = 1;
        } else if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT ||
                   L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS) {
          ms = parse_expr(vm, L);
        }
      }
    } else if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN ||
               L->cur.kind==TK_MINUS) {
      ms = parse_expr(vm, L);
    }
    if (from_dur) {
      if (cubalc_parse_duration_ms(src, &ms, NULL, &derr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SLEEP_MS", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", derr ? derr : "SLEEP: bad duration");
        var_set_str(vm, "ERR", derr ? derr : "SLEEP: bad duration");
        bump(vm); return 1;
      }
    }
    if (kw(&L->cur,"MS") || kw(&L->cur,"MILLIS") || kw(&L->cur,"MILLISECONDS"))
      lex_next(L);
    if (ms < 0) ms = 0;
    if (ms > 60000) ms = 60000; /* hard cap — avoid hung agents */
    /* Honor CUBALC_RUN_TIMEOUT / -T: clamp or fail before long sleep. */
    if (vm->run_deadline_ms > 0) {
      long rem = cubalc_lang_timeout_remain_ms(vm);
      if (rem <= 0) {
        cubalc_lang_check_timeout(vm, L->cur.line);
        bump(vm); return 1;
      }
      if (ms > rem) ms = rem;
    }
    if (ms > 0) {
      struct timespec ts;
      ts.tv_sec = ms / 1000;
      ts.tv_nsec = (ms % 1000) * 1000000L;
      nanosleep(&ts, NULL);
    }
    /* After sleep, surface budget expiry so loops stop cleanly. */
    if (vm->run_deadline_ms > 0 && cubalc_lang_timeout_remain_ms(vm) <= 0) {
      cubalc_lang_check_timeout(vm, L->cur.line);
      bump(vm); return 1;
    }
    vm->last_n = ms;
    var_set_num(vm, "LAST_N", ms);
    var_set_num(vm, "SLEEP_MS", ms);
    var_set_num(vm, "OK", 1);
    snprintf(buf, sizeof buf, "%ld", ms);
    var_set_str(vm, "LAST", buf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
    bump(vm); return 1;
  }

  /* SYS APPEND|LOG path data — append line to file (creates if missing).
   * Soft fail OK=0 + sticky LAST_ERR. LAST_N = bytes written (data+newline).
   * Usability: agent history/audit logs without shell >> . */
  if (kw(&L->cur,"APPEND") || kw(&L->cur,"LOG") || kw(&L->cur,"APPENDLN") ||
      kw(&L->cur,"LOGAPPEND")){
    lex_next(L);
    char path[512]="", data[CUBALC_HOST_STR_MAX];
    long nbytes = 0;
    data[0]=0;
    if (resolve_str_arg(vm, L, path, sizeof path) != 0){
      fail(vm,"SYS APPEND path data"); return -1;
    }
    if (resolve_str_arg(vm, L, data, sizeof data) != 0){
      /* allow bare number as data */
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_MINUS || L->cur.kind==TK_LPAREN){
        long n = parse_expr(vm, L);
        snprintf(data, sizeof data, "%ld", n);
      } else if (L->cur.kind==TK_IDENT){
        Var *v = var_get(vm, L->cur.text, 0);
        if (v && !v->is_str){ snprintf(data, sizeof data, "%ld", v->val); lex_next(L); }
        else { fail(vm,"SYS APPEND path data"); return -1; }
      } else {
        fail(vm,"SYS APPEND path data"); return -1;
      }
    }
    {
      FILE *af = fopen(path, "a");
      if (!af){
        var_set_num(vm,"OK",0);
        var_set_num(vm,"LAST_N",0);
        var_set_str(vm,"LAST_ERR","APPEND: open fail");
        var_set_str(vm,"ERR","APPEND: open fail");
        bump(vm); return 1;
      }
      fputs(data, af);
      fputc('\n', af);
      fclose(af);
      nbytes = (long)strlen(data) + 1;
    }
    var_set_str(vm,"LAST",path);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
    vm->last_n = nbytes;
    var_set_num(vm,"LAST_N",nbytes);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }

  return 0;
}
