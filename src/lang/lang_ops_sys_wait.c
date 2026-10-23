/* CubalC lang — lang_ops_sys_wait.c
 * SYS WAITFILE/WAITGONE/WAITSTABLE/WAITCHANGED/WAITMATCH plane (A2 theme peel).
 * Mesh vote A2_theme_peel_core_sys · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_sys_wait.h"
#include "lang/cubalc_lang_ops_sys_time_walk.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <unistd.h>
#  include <sys/stat.h>
#endif

int cubalc_lang_ops_sys_try_wait(VM *vm, Lex *L) {
  /* Wait/poll SYS plane.
   * Caller: after SYS + plate/path_fs/env/string miss; L at subcommand.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
    /* SYS WAITFILE|WAITPATH|POLLFILE path [timeout_ms|"duration"]
     * — poll until path exists (agent plate/peer handoff).
     * Default timeout 30000 ms; cap 120s. Poll every 50 ms.
     * Timeout: number ms, or PARSEMS string ("2s","250ms","1s500ms").
     * LAST = path; LAST_N / WAITFILE_HIT = 1 hit | 0 timeout;
     * WAITFILE_MS = elapsed. Soft timeout / empty path / bad duration → OK=0.
     * Usability: peer/flag wait without EXIST+SLEEP or PARSEMS glue. */
    if (kw(&L->cur,"WAITFILE") || kw(&L->cur,"WAITPATH") ||
        kw(&L->cur,"POLLFILE") || kw(&L->cur,"AWAITFILE") ||
        kw(&L->cur,"WAITFORFILE") || kw(&L->cur,"FILEWAIT") ||
        kw(&L->cur,"WAITEXIST") || kw(&L->cur,"POLLPATH") ||
        kw(&L->cur,"UNTILFILE") || kw(&L->cur,"WAITON")){
      char path[512], a[CUBALC_HOST_STR_MAX];
      long timeout_ms = 30000, elapsed = 0, hit = 0;
      const char *toerr = NULL;
      struct timespec t0, t1, sl;
      lex_next(L);
      path[0] = 0; a[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS WAITFILE path [timeout_ms|duration]");
        return -1;
      }
      snprintf(path, sizeof path, "%s", a);
      if (cubalc_parse_optional_timeout_ms(vm, L, &timeout_ms, &toerr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "WAITFILE_HIT", 0);
        var_set_num(vm, "WAITFILE_N", 0);
        var_set_num(vm, "WAITFILE_MS", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", toerr ? toerr : "WAITFILE: bad timeout");
        var_set_str(vm, "ERR", toerr ? toerr : "WAITFILE: bad timeout");
        bump(vm); return 1;
      }
      if (timeout_ms < 0) timeout_ms = 0;
      if (timeout_ms > 120000) timeout_ms = 120000;
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "WAITFILE_HIT", 0);
        var_set_num(vm, "WAITFILE_N", 0);
        var_set_num(vm, "WAITFILE_MS", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "WAITFILE: empty path");
        var_set_str(vm, "ERR", "WAITFILE: empty path");
        bump(vm); return 1;
      }
      clock_gettime(CLOCK_MONOTONIC, &t0);
      for (;;) {
        if (cubalc_host_exists(path)) {
          hit = 1;
          break;
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        elapsed = (t1.tv_sec - t0.tv_sec) * 1000L +
                  (t1.tv_nsec - t0.tv_nsec) / 1000000L;
        if (elapsed >= timeout_ms)
          break;
        {
          long left = timeout_ms - elapsed;
          long step = left < 50 ? left : 50;
          if (step < 1) step = 1;
          sl.tv_sec = step / 1000;
          sl.tv_nsec = (step % 1000) * 1000000L;
          nanosleep(&sl, NULL);
        }
      }
      clock_gettime(CLOCK_MONOTONIC, &t1);
      elapsed = (t1.tv_sec - t0.tv_sec) * 1000L +
                (t1.tv_nsec - t0.tv_nsec) / 1000000L;
      if (elapsed < 0) elapsed = 0;
      if (hit) {
        var_set_str(vm, "LAST", path);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_str(vm, "WAITFILE", path);
        var_set_str(vm, "WAITPATH", path);
        var_set_str(vm, "POLLFILE", path);
        var_set_num(vm, "WAITFILE_HIT", 1);
        var_set_num(vm, "WAITFILE_N", 1);
        var_set_num(vm, "WAITFILE_MS", elapsed);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "WAITFILE", "");
      var_set_num(vm, "WAITFILE_HIT", 0);
      var_set_num(vm, "WAITFILE_N", 0);
      var_set_num(vm, "WAITFILE_MS", elapsed);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "WAITFILE: timeout");
      var_set_str(vm, "ERR", "WAITFILE: timeout");
      bump(vm); return 1;
    }
    /* SYS WAITGONE|WAITMISSING|WAITDELETE path [timeout_ms|"duration"]
     * — poll until path does NOT exist (dual of WAITFILE).
     * Default timeout 30000 ms; cap 120s. Poll every 50 ms.
     * Timeout: number ms or PARSEMS string (same as WAITFILE).
     * LAST = path; LAST_N / WAITGONE_HIT = 1 gone | 0 timeout;
     * WAITGONE_MS = elapsed. Soft timeout / empty path / bad duration → OK=0.
     * Already missing → immediate hit. Usability: unlock/cleanup handoff. */
    if (kw(&L->cur,"WAITGONE") || kw(&L->cur,"WAITMISSING") ||
        kw(&L->cur,"WAITDELETE") || kw(&L->cur,"WAITRM") ||
        kw(&L->cur,"POLLGONE") || kw(&L->cur,"AWAITGONE") ||
        kw(&L->cur,"WAITNOTEXIST") || kw(&L->cur,"WAITREMOVED") ||
        kw(&L->cur,"UNTILGONE") || kw(&L->cur,"WAITABSENT") ||
        kw(&L->cur,"POLLMISSING") || kw(&L->cur,"WAITCLEAR")){
      char path[512], a[CUBALC_HOST_STR_MAX];
      long timeout_ms = 30000, elapsed = 0, hit = 0;
      const char *toerr = NULL;
      struct timespec t0, t1, sl;
      lex_next(L);
      path[0] = 0; a[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS WAITGONE path [timeout_ms|duration]");
        return -1;
      }
      snprintf(path, sizeof path, "%s", a);
      if (cubalc_parse_optional_timeout_ms(vm, L, &timeout_ms, &toerr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "WAITGONE_HIT", 0);
        var_set_num(vm, "WAITGONE_N", 0);
        var_set_num(vm, "WAITGONE_MS", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", toerr ? toerr : "WAITGONE: bad timeout");
        var_set_str(vm, "ERR", toerr ? toerr : "WAITGONE: bad timeout");
        bump(vm); return 1;
      }
      if (timeout_ms < 0) timeout_ms = 0;
      if (timeout_ms > 120000) timeout_ms = 120000;
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "WAITGONE_HIT", 0);
        var_set_num(vm, "WAITGONE_N", 0);
        var_set_num(vm, "WAITGONE_MS", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "WAITGONE: empty path");
        var_set_str(vm, "ERR", "WAITGONE: empty path");
        bump(vm); return 1;
      }
      clock_gettime(CLOCK_MONOTONIC, &t0);
      for (;;) {
        if (!cubalc_host_exists(path)) {
          hit = 1;
          break;
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        elapsed = (t1.tv_sec - t0.tv_sec) * 1000L +
                  (t1.tv_nsec - t0.tv_nsec) / 1000000L;
        if (elapsed >= timeout_ms)
          break;
        {
          long left = timeout_ms - elapsed;
          long step = left < 50 ? left : 50;
          if (step < 1) step = 1;
          sl.tv_sec = step / 1000;
          sl.tv_nsec = (step % 1000) * 1000000L;
          nanosleep(&sl, NULL);
        }
      }
      clock_gettime(CLOCK_MONOTONIC, &t1);
      elapsed = (t1.tv_sec - t0.tv_sec) * 1000L +
                (t1.tv_nsec - t0.tv_nsec) / 1000000L;
      if (elapsed < 0) elapsed = 0;
      if (hit) {
        var_set_str(vm, "LAST", path);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_str(vm, "WAITGONE", path);
        var_set_str(vm, "WAITMISSING", path);
        var_set_str(vm, "WAITDELETE", path);
        var_set_num(vm, "WAITGONE_HIT", 1);
        var_set_num(vm, "WAITGONE_N", 1);
        var_set_num(vm, "WAITGONE_MS", elapsed);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "WAITGONE", "");
      var_set_num(vm, "WAITGONE_HIT", 0);
      var_set_num(vm, "WAITGONE_N", 0);
      var_set_num(vm, "WAITGONE_MS", elapsed);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "WAITGONE: timeout");
      var_set_str(vm, "ERR", "WAITGONE: timeout");
      bump(vm); return 1;
    }
    /* SYS WAITSTABLE|WAITQUIET|STABLEFILE path [timeout_ms]
     *   or path quiet_ms timeout_ms
     * — poll until path exists and size+mtime unchanged for quiet window.
     * Default quiet 200 ms; timeout 30000 (cap 120s); poll 50 ms.
     * One numeric arg = timeout (quiet=200). Two = quiet then timeout.
     * Timeout 0 + exists → immediate hit (no quiet wait).
     * LAST = path; WAITSTABLE_HIT/N/MS/SIZE/MTIME. Soft timeout OK=0.
     * Usability: wait for peer write to settle before READ without torn plate. */
    if (kw(&L->cur,"WAITSTABLE") || kw(&L->cur,"WAITQUIET") ||
        kw(&L->cur,"STABLEFILE") || kw(&L->cur,"WAITSETTLE") ||
        kw(&L->cur,"POLLSTABLE") || kw(&L->cur,"AWAITSTABLE") ||
        kw(&L->cur,"UNTILSTABLE") || kw(&L->cur,"WAITREADYFILE") ||
        kw(&L->cur,"SETTLEFILE") || kw(&L->cur,"WAITSIZESTABLE")){
      char path[512], a[CUBALC_HOST_STR_MAX];
      long timeout_ms = 30000, quiet_ms = 200, elapsed = 0, hit = 0;
      long last_size = -1, last_mtime = -1, cur_size = 0, cur_mtime = 0;
      long stable_since = -1, now_ms;
      int have_sample = 0, n_args = 0;
      long arg1 = 0, arg2 = 0;
      struct timespec t0, t1, sl;
      cubalc_host_result kr, mr;
      lex_next(L);
      path[0] = 0; a[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS WAITSTABLE path [timeout_ms]|quiet timeout");
        return -1;
      }
      snprintf(path, sizeof path, "%s", a);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_IDENT) {
        arg1 = parse_expr(vm, L);
        if (kw(&L->cur,"MS") || kw(&L->cur,"MILLIS") || kw(&L->cur,"MILLISECONDS") ||
            kw(&L->cur,"QUIET") || kw(&L->cur,"SETTLE"))
          lex_next(L);
        n_args = 1;
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
            L->cur.kind == TK_IDENT) {
          arg2 = parse_expr(vm, L);
          if (kw(&L->cur,"MS") || kw(&L->cur,"MILLIS") || kw(&L->cur,"MILLISECONDS") ||
              kw(&L->cur,"TIMEOUT"))
            lex_next(L);
          n_args = 2;
        }
      } else if (kw(&L->cur,"MS") || kw(&L->cur,"MILLIS") || kw(&L->cur,"TIMEOUT")) {
        lex_next(L);
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
            L->cur.kind == TK_IDENT) {
          arg1 = parse_expr(vm, L);
          n_args = 1;
        }
      }
      if (n_args == 2) {
        quiet_ms = arg1;
        timeout_ms = arg2;
      } else if (n_args == 1) {
        timeout_ms = arg1;
      }
      if (quiet_ms < 0) quiet_ms = 0;
      if (quiet_ms > 10000) quiet_ms = 10000;
      if (timeout_ms < 0) timeout_ms = 0;
      if (timeout_ms > 120000) timeout_ms = 120000;
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "WAITSTABLE_HIT", 0);
        var_set_num(vm, "WAITSTABLE_N", 0);
        var_set_num(vm, "WAITSTABLE_MS", 0);
        var_set_num(vm, "WAITSTABLE_SIZE", 0);
        var_set_num(vm, "WAITSTABLE_MTIME", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "WAITSTABLE: empty path");
        var_set_str(vm, "ERR", "WAITSTABLE: empty path");
        bump(vm); return 1;
      }
      clock_gettime(CLOCK_MONOTONIC, &t0);
      for (;;) {
        clock_gettime(CLOCK_MONOTONIC, &t1);
        now_ms = (t1.tv_sec - t0.tv_sec) * 1000L +
                 (t1.tv_nsec - t0.tv_nsec) / 1000000L;
        if (now_ms < 0) now_ms = 0;
        if (cubalc_host_path_kind(path, &kr) == 0 && kr.ok &&
            cubalc_host_mtime(path, &mr) == 0 && mr.ok) {
          cur_size = kr.n;
          cur_mtime = mr.n;
          /* timeout 0: exists → immediate hit (no quiet window) */
          if (timeout_ms == 0) {
            hit = 1;
            break;
          }
          if (!have_sample || cur_size != last_size || cur_mtime != last_mtime) {
            last_size = cur_size;
            last_mtime = cur_mtime;
            have_sample = 1;
            stable_since = now_ms;
          } else if (now_ms - stable_since >= quiet_ms) {
            hit = 1;
            break;
          }
        } else {
          have_sample = 0;
          stable_since = -1;
          last_size = -1;
          last_mtime = -1;
        }
        elapsed = now_ms;
        if (elapsed >= timeout_ms)
          break;
        {
          long left = timeout_ms - elapsed;
          long step = left < 50 ? left : 50;
          if (step < 1) step = 1;
          sl.tv_sec = step / 1000;
          sl.tv_nsec = (step % 1000) * 1000000L;
          nanosleep(&sl, NULL);
        }
      }
      clock_gettime(CLOCK_MONOTONIC, &t1);
      elapsed = (t1.tv_sec - t0.tv_sec) * 1000L +
                (t1.tv_nsec - t0.tv_nsec) / 1000000L;
      if (elapsed < 0) elapsed = 0;
      if (hit) {
        var_set_str(vm, "LAST", path);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_str(vm, "WAITSTABLE", path);
        var_set_str(vm, "WAITQUIET", path);
        var_set_str(vm, "STABLEFILE", path);
        var_set_num(vm, "WAITSTABLE_HIT", 1);
        var_set_num(vm, "WAITSTABLE_N", 1);
        var_set_num(vm, "WAITSTABLE_MS", elapsed);
        var_set_num(vm, "WAITSTABLE_SIZE", cur_size);
        var_set_num(vm, "WAITSTABLE_MTIME", cur_mtime);
        var_set_num(vm, "WAITSTABLE_QUIET", quiet_ms);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "WAITSTABLE", "");
      var_set_num(vm, "WAITSTABLE_HIT", 0);
      var_set_num(vm, "WAITSTABLE_N", 0);
      var_set_num(vm, "WAITSTABLE_MS", elapsed);
      var_set_num(vm, "WAITSTABLE_SIZE", have_sample ? last_size : 0);
      var_set_num(vm, "WAITSTABLE_MTIME", have_sample ? last_mtime : 0);
      var_set_num(vm, "WAITSTABLE_QUIET", quiet_ms);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "WAITSTABLE: timeout");
      var_set_str(vm, "ERR", "WAITSTABLE: timeout");
      bump(vm); return 1;
    }
    /* SYS WAITCHANGED|WAITMODIFIED|POLLCHANGE path [timeout_ms]
     * SYS WAITCHANGED path SINCE|FROM|BASE size mtime [timeout_ms]
     * SYS WAITCHANGED path SINCE MISSING [timeout_ms]
     * — poll until path appears or size/mtime differs from baseline.
     * Default: sample baseline now (peer concurrent update).
     * SINCE size mtime: explicit baseline (agent snapshot then wait/check).
     * SINCE MISSING: baseline = absent (hit when path appears).
     * Default timeout 30000, cap 120s, poll 50 ms. Soft timeout / empty → OK=0.
     * LAST = path; WAITCHANGED_HIT/N/MS/SIZE/MTIME; WAITCHANGED_FROM
     *   0=missing→exist, 1=size/mtime change, 2=exist→gone.
     * Usability: wait for peer status plate update without known content. */
    if (kw(&L->cur,"WAITCHANGED") || kw(&L->cur,"WAITMODIFIED") ||
        kw(&L->cur,"POLLCHANGE") || kw(&L->cur,"AWAITCHANGE") ||
        kw(&L->cur,"WAITUPDATE") || kw(&L->cur,"UNTILCHANGED") ||
        kw(&L->cur,"WAITMTIMECHG") || kw(&L->cur,"POLLMODIFIED") ||
        kw(&L->cur,"WAITNEW") || kw(&L->cur,"WAITDIRTY")){
      char path[512], a[CUBALC_HOST_STR_MAX];
      long timeout_ms = 30000, elapsed = 0, hit = 0, from = 0;
      long base_size = -1, base_mtime = -1, cur_size = 0, cur_mtime = 0;
      int base_exists = 0, explicit_base = 0;
      struct timespec t0, t1, sl;
      cubalc_host_result kr, mr;
      lex_next(L);
      path[0] = 0; a[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS WAITCHANGED path [SINCE size mtime] [timeout_ms]");
        return -1;
      }
      snprintf(path, sizeof path, "%s", a);
      /* optional SINCE|FROM|BASE size mtime | SINCE MISSING */
      if (kw(&L->cur,"SINCE") || kw(&L->cur,"FROM") || kw(&L->cur,"BASE") ||
          kw(&L->cur,"REF") || kw(&L->cur,"BASELINE")){
        lex_next(L);
        explicit_base = 1;
        if (kw(&L->cur,"MISSING") || kw(&L->cur,"ABSENT") || kw(&L->cur,"GONE") ||
            kw(&L->cur,"NONE") || kw(&L->cur,"NIL")){
          lex_next(L);
          base_exists = 0;
          base_size = -1;
          base_mtime = -1;
        } else {
          if (L->cur.kind != TK_NUM && L->cur.kind != TK_MINUS &&
              L->cur.kind != TK_LPAREN && L->cur.kind != TK_IDENT) {
            fail(vm, "SYS WAITCHANGED path SINCE size mtime [timeout_ms]");
            return -1;
          }
          base_size = parse_expr(vm, L);
          if (L->cur.kind != TK_NUM && L->cur.kind != TK_MINUS &&
              L->cur.kind != TK_LPAREN && L->cur.kind != TK_IDENT) {
            fail(vm, "SYS WAITCHANGED path SINCE size mtime [timeout_ms]");
            return -1;
          }
          base_mtime = parse_expr(vm, L);
          base_exists = 1;
        }
      }
      {
        const char *toerr = NULL;
        if (cubalc_parse_optional_timeout_ms(vm, L, &timeout_ms, &toerr) != 0) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "WAITCHANGED_HIT", 0);
          var_set_num(vm, "WAITCHANGED_N", 0);
          var_set_num(vm, "WAITCHANGED_MS", 0);
          var_set_num(vm, "WAITCHANGED_SIZE", 0);
          var_set_num(vm, "WAITCHANGED_MTIME", 0);
          var_set_num(vm, "WAITCHANGED_FROM", 0);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", toerr ? toerr : "WAITCHANGED: bad timeout");
          var_set_str(vm, "ERR", toerr ? toerr : "WAITCHANGED: bad timeout");
          bump(vm); return 1;
        }
      }
      if (timeout_ms < 0) timeout_ms = 0;
      if (timeout_ms > 120000) timeout_ms = 120000;
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "WAITCHANGED_HIT", 0);
        var_set_num(vm, "WAITCHANGED_N", 0);
        var_set_num(vm, "WAITCHANGED_MS", 0);
        var_set_num(vm, "WAITCHANGED_SIZE", 0);
        var_set_num(vm, "WAITCHANGED_MTIME", 0);
        var_set_num(vm, "WAITCHANGED_FROM", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "WAITCHANGED: empty path");
        var_set_str(vm, "ERR", "WAITCHANGED: empty path");
        bump(vm); return 1;
      }
      /* baseline sample unless explicit SINCE */
      if (!explicit_base) {
        if (cubalc_host_path_kind(path, &kr) == 0 && kr.ok &&
            cubalc_host_mtime(path, &mr) == 0 && mr.ok) {
          base_exists = 1;
          base_size = kr.n;
          base_mtime = mr.n;
        } else {
          base_exists = 0;
          base_size = -1;
          base_mtime = -1;
        }
      }
      clock_gettime(CLOCK_MONOTONIC, &t0);
      for (;;) {
        if (cubalc_host_path_kind(path, &kr) == 0 && kr.ok &&
            cubalc_host_mtime(path, &mr) == 0 && mr.ok) {
          cur_size = kr.n;
          cur_mtime = mr.n;
          if (!base_exists) {
            /* missing → now exists */
            hit = 1;
            from = 0;
            break;
          }
          if (cur_size != base_size || cur_mtime != base_mtime) {
            hit = 1;
            from = 1;
            break;
          }
        } else if (base_exists) {
          /* existed → now gone counts as change (peer cleanup / replace) */
          cur_size = 0;
          cur_mtime = 0;
          hit = 1;
          from = 2;
          break;
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        elapsed = (t1.tv_sec - t0.tv_sec) * 1000L +
                  (t1.tv_nsec - t0.tv_nsec) / 1000000L;
        if (elapsed >= timeout_ms)
          break;
        {
          long left = timeout_ms - elapsed;
          long step = left < 50 ? left : 50;
          if (step < 1) step = 1;
          sl.tv_sec = step / 1000;
          sl.tv_nsec = (step % 1000) * 1000000L;
          nanosleep(&sl, NULL);
        }
      }
      clock_gettime(CLOCK_MONOTONIC, &t1);
      elapsed = (t1.tv_sec - t0.tv_sec) * 1000L +
                (t1.tv_nsec - t0.tv_nsec) / 1000000L;
      if (elapsed < 0) elapsed = 0;
      if (hit) {
        var_set_str(vm, "LAST", path);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_str(vm, "WAITCHANGED", path);
        var_set_str(vm, "WAITMODIFIED", path);
        var_set_str(vm, "POLLCHANGE", path);
        var_set_num(vm, "WAITCHANGED_HIT", 1);
        var_set_num(vm, "WAITCHANGED_N", 1);
        var_set_num(vm, "WAITCHANGED_MS", elapsed);
        var_set_num(vm, "WAITCHANGED_SIZE", cur_size);
        var_set_num(vm, "WAITCHANGED_MTIME", cur_mtime);
        var_set_num(vm, "WAITCHANGED_FROM", from);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "WAITCHANGED", "");
      var_set_num(vm, "WAITCHANGED_HIT", 0);
      var_set_num(vm, "WAITCHANGED_N", 0);
      var_set_num(vm, "WAITCHANGED_MS", elapsed);
      var_set_num(vm, "WAITCHANGED_SIZE", base_exists ? base_size : 0);
      var_set_num(vm, "WAITCHANGED_MTIME", base_exists ? base_mtime : 0);
      var_set_num(vm, "WAITCHANGED_FROM", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "WAITCHANGED: timeout");
      var_set_str(vm, "ERR", "WAITCHANGED: timeout");
      bump(vm); return 1;
    }
    /* SYS WAITMATCH|WAITCONTAINS|POLLMATCH path needle [timeout_ms]
     * — poll until file exists and content contains needle (substring).
     * WAITMATCHI icase. Default timeout 30000 ms; cap 120s; poll 50 ms.
     * LAST = path; LAST_N / WAITMATCH_HIT = 1|0; WAITMATCH_MS elapsed.
     * Soft timeout / empty path → OK=0. Empty needle → hit when file readable.
     * Usability: wait for peer plate content (status=ready) without READ+HAS+SLEEP loop. */
    if (kw(&L->cur,"WAITMATCH") || kw(&L->cur,"WAITCONTAINS") ||
        kw(&L->cur,"POLLMATCH") || kw(&L->cur,"AWAITMATCH") ||
        kw(&L->cur,"WAITFOR") || kw(&L->cur,"WAITCONTENT") ||
        kw(&L->cur,"WAITNEEDLE") || kw(&L->cur,"POLLCONTENT") ||
        kw(&L->cur,"UNTILMATCH") || kw(&L->cur,"WAITGREP") ||
        kw(&L->cur,"WAITMATCHI") || kw(&L->cur,"WAITCONTAINSI") ||
        kw(&L->cur,"POLLMATCHI") || kw(&L->cur,"AWAITMATCHI") ||
        kw(&L->cur,"WAITCONTENTI") || kw(&L->cur,"WAITGREPI")){
      char path[512], needle[CUBALC_HOST_STR_MAX], a[CUBALC_HOST_STR_MAX];
      long timeout_ms = 30000, elapsed = 0, hit = 0;
      int icase = 0;
      size_t nlen;
      struct timespec t0, t1, sl;
      cubalc_host_result hr;
      if (kw(&L->cur,"WAITMATCHI") || kw(&L->cur,"WAITCONTAINSI") ||
          kw(&L->cur,"POLLMATCHI") || kw(&L->cur,"AWAITMATCHI") ||
          kw(&L->cur,"WAITCONTENTI") || kw(&L->cur,"WAITGREPI"))
        icase = 1;
      lex_next(L);
      path[0] = 0; needle[0] = 0; a[0] = 0;
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS WAITMATCH path needle [timeout_ms]");
        return -1;
      }
      snprintf(path, sizeof path, "%s", a);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long n = parse_expr(vm, L);
          snprintf(needle, sizeof needle, "%ld", n);
        } else {
          needle[0] = 0;
        }
      }
      nlen = strlen(needle);
      {
        const char *toerr = NULL;
        if (cubalc_parse_optional_timeout_ms(vm, L, &timeout_ms, &toerr) != 0) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "WAITMATCH_HIT", 0);
          var_set_num(vm, "WAITMATCH_N", 0);
          var_set_num(vm, "WAITMATCH_MS", 0);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", toerr ? toerr : "WAITMATCH: bad timeout");
          var_set_str(vm, "ERR", toerr ? toerr : "WAITMATCH: bad timeout");
          bump(vm); return 1;
        }
      }
      if (timeout_ms < 0) timeout_ms = 0;
      if (timeout_ms > 120000) timeout_ms = 120000;
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "WAITMATCH_HIT", 0);
        var_set_num(vm, "WAITMATCH_N", 0);
        var_set_num(vm, "WAITMATCH_MS", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "WAITMATCH: empty path");
        var_set_str(vm, "ERR", "WAITMATCH: empty path");
        bump(vm); return 1;
      }
      clock_gettime(CLOCK_MONOTONIC, &t0);
      for (;;) {
        if (cubalc_host_read(path, &hr) == 0) {
          int found = 0;
          if (nlen == 0) {
            found = 1;
          } else if (!icase) {
            found = (strstr(hr.str, needle) != NULL) ? 1 : 0;
          } else {
            /* case-insensitive substring scan */
            size_t blen = strlen(hr.str), i, j;
            if (blen >= nlen) {
              for (i = 0; i + nlen <= blen; i++) {
                int okm = 1;
                for (j = 0; j < nlen; j++) {
                  char a2 = hr.str[i + j], b2 = needle[j];
                  if (a2 >= 'A' && a2 <= 'Z') a2 = (char)(a2 - 'A' + 'a');
                  if (b2 >= 'A' && b2 <= 'Z') b2 = (char)(b2 - 'A' + 'a');
                  if (a2 != b2) { okm = 0; break; }
                }
                if (okm) { found = 1; break; }
              }
            }
          }
          if (found) {
            hit = 1;
            break;
          }
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        elapsed = (t1.tv_sec - t0.tv_sec) * 1000L +
                  (t1.tv_nsec - t0.tv_nsec) / 1000000L;
        if (elapsed >= timeout_ms)
          break;
        {
          long left = timeout_ms - elapsed;
          long step = left < 50 ? left : 50;
          if (step < 1) step = 1;
          sl.tv_sec = step / 1000;
          sl.tv_nsec = (step % 1000) * 1000000L;
          nanosleep(&sl, NULL);
        }
      }
      clock_gettime(CLOCK_MONOTONIC, &t1);
      elapsed = (t1.tv_sec - t0.tv_sec) * 1000L +
                (t1.tv_nsec - t0.tv_nsec) / 1000000L;
      if (elapsed < 0) elapsed = 0;
      if (hit) {
        var_set_str(vm, "LAST", path);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_str(vm, "WAITMATCH", path);
        var_set_str(vm, "WAITCONTAINS", path);
        var_set_str(vm, "POLLMATCH", path);
        var_set_num(vm, "WAITMATCH_HIT", 1);
        var_set_num(vm, "WAITMATCH_N", 1);
        var_set_num(vm, "WAITMATCH_MS", elapsed);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "WAITMATCH", "");
      var_set_num(vm, "WAITMATCH_HIT", 0);
      var_set_num(vm, "WAITMATCH_N", 0);
      var_set_num(vm, "WAITMATCH_MS", elapsed);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "WAITMATCH: timeout");
      var_set_str(vm, "ERR", "WAITMATCH: timeout");
      bump(vm); return 1;
    }
  return 0;
}
