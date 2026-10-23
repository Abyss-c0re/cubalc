/* CubalC lang — lang_ops_sys_time_date.c
 * SYS time/date residual: DATE/ISO · PARSEISO · ADDISO · TIMEDIFF · LOCAL (A2 peel).
 * Mesh vote B_core_sys · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_sys_time_date.h"
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
#endif

int cubalc_lang_ops_sys_try_time_date(VM *vm, Lex *L) {
  /* SYS time/date residual. Caller consumed SYS|HOST.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
    /* SYS DATE|ISO|DATETIME|UTC — human-readable UTC stamp for plates/logs.
     * Usability: agents stamp plates without shell date(1).
     * LAST/DATE/ISO = "YYYY-MM-DDTHH:MM:SSZ"; LAST_N = strlen; also TIME epoch. */
    if (kw(&L->cur,"DATE") || kw(&L->cur,"ISO") || kw(&L->cur,"DATETIME") ||
        kw(&L->cur,"UTC") || kw(&L->cur,"ISO8601") || kw(&L->cur,"TIMESTAMP_ISO")){
      time_t now;
      struct tm tm_utc;
      char iso[40];
      long epoch;
      lex_next(L);
      now = time(NULL);
      epoch = (long)now;
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
      var_set_str(vm, "DATE", iso);
      var_set_str(vm, "ISO", iso);
      var_set_str(vm, "DATETIME", iso);
      var_set_str(vm, "LAST", iso);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", iso);
      vm->last_n = (long)strlen(iso);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "TIME", epoch);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS FROMTIME|EPOCHISO|TOISO|ISOFROM [n|LAST_N]
     * — format Unix epoch seconds as UTC ISO "YYYY-MM-DDTHH:MM:SSZ".
     * LAST = stamp; LAST_N / FROMTIME_N / TIME = epoch (input, clamped ≥0).
     * Default n = LAST_N. Usability: MTIME/AGE plate epochs → human stamps
     * without shell date -d @epoch (DATE only does "now"). */
    if (kw(&L->cur,"FROMTIME") || kw(&L->cur,"EPOCHISO") || kw(&L->cur,"TOISO") ||
        kw(&L->cur,"ISOFROM") || kw(&L->cur,"EPOCH2ISO") || kw(&L->cur,"TIMEISO") ||
        kw(&L->cur,"UNIXISO") || kw(&L->cur,"FROMEPOCH") || kw(&L->cur,"EPOCH_TO_ISO") ||
        kw(&L->cur,"TSISO") || kw(&L->cur,"STAMPEPOCH")){
      long n = vm->last_n;
      time_t t;
      struct tm tm_utc;
      char iso[40];
      lex_next(L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE"))){
        n = parse_expr(vm, L);
      }
      if (n < 0) n = 0;
      t = (time_t)n;
#if defined(CUBALC_OS_WINDOWS)
      {
        struct tm *tp = gmtime(&t);
        if (tp) tm_utc = *tp;
        else memset(&tm_utc, 0, sizeof tm_utc);
      }
#else
      if (!gmtime_r(&t, &tm_utc))
        memset(&tm_utc, 0, sizeof tm_utc);
#endif
      snprintf(iso, sizeof iso, "%04d-%02d-%02dT%02d:%02d:%02dZ",
               tm_utc.tm_year + 1900, tm_utc.tm_mon + 1, tm_utc.tm_mday,
               tm_utc.tm_hour, tm_utc.tm_min, tm_utc.tm_sec);
      var_set_str(vm, "LAST", iso);
      var_set_str(vm, "FROMTIME", iso);
      var_set_str(vm, "EPOCHISO", iso);
      var_set_str(vm, "TOISO", iso);
      var_set_str(vm, "ISO", iso);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", iso);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "FROMTIME_N", n);
      var_set_num(vm, "TIME", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS PARSEISO|TOEPOCH|ISO2EPOCH|FROMISO [str]
     * — parse UTC ISO stamp → Unix epoch seconds (dual of FROMTIME).
     * Accepts: YYYY-MM-DDTHH:MM:SSZ | …SS | space sep | date-only (midnight).
     * LAST = decimal epoch; LAST_N / PARSEISO_N / TIME = epoch.
     * Soft fail OK=0 on empty/bad. Usability: plate DATE stamps → age/compare
     * without shell date -d. */
    if (kw(&L->cur,"PARSEISO") || kw(&L->cur,"TOEPOCH") || kw(&L->cur,"ISO2EPOCH") ||
        kw(&L->cur,"FROMISO") || kw(&L->cur,"ISOEPOCH") || kw(&L->cur,"PARSEDATE") ||
        kw(&L->cur,"DATEPARSE") || kw(&L->cur,"EPOCHFROM") || kw(&L->cur,"ISO_TO_EPOCH") ||
        kw(&L->cur,"READISO") || kw(&L->cur,"PARSE_ISO")){
      char src[128], out[40];
      const char *p;
      int y=0, mo=0, d=0, H=0, M=0, S=0, nscan;
      long epoch = 0;
      /* days from civil date (Howard Hinnant algorithm) → Unix epoch */
      long era, yoe, doy, doe, days;
      lex_next(L);
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      p = src;
      while (*p == ' ' || *p == '\t') p++;
      if (!*p) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "PARSEISO_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "PARSEISO: empty");
        var_set_str(vm, "ERR", "PARSEISO: empty");
        bump(vm); return 1;
      }
      /* date-only */
      nscan = sscanf(p, "%d-%d-%d", &y, &mo, &d);
      if (nscan != 3 || y < 1970 || y > 9999 || mo < 1 || mo > 12 || d < 1 || d > 31) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "PARSEISO_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "PARSEISO: bad date");
        var_set_str(vm, "ERR", "PARSEISO: bad date");
        bump(vm); return 1;
      }
      /* optional time after T or space */
      {
        const char *t = p;
        while (*t && *t != 'T' && *t != ' ' && *t != 't') t++;
        if (*t == 'T' || *t == 't' || *t == ' ') {
          t++;
          while (*t == ' ' || *t == '\t') t++;
          if (*t) {
            int hs=0, ms=0, ss=0;
            int nt = sscanf(t, "%d:%d:%d", &hs, &ms, &ss);
            if (nt < 2) {
              var_set_str(vm, "LAST", "");
              vm->last_str[0] = 0;
              vm->last_n = 0;
              var_set_num(vm, "LAST_N", 0);
              var_set_num(vm, "PARSEISO_N", 0);
              var_set_num(vm, "OK", 0);
              var_set_str(vm, "LAST_ERR", "PARSEISO: bad time");
              var_set_str(vm, "ERR", "PARSEISO: bad time");
              bump(vm); return 1;
            }
            if (nt == 2) ss = 0;
            if (hs < 0 || hs > 23 || ms < 0 || ms > 59 || ss < 0 || ss > 60) {
              var_set_str(vm, "LAST", "");
              vm->last_str[0] = 0;
              vm->last_n = 0;
              var_set_num(vm, "LAST_N", 0);
              var_set_num(vm, "PARSEISO_N", 0);
              var_set_num(vm, "OK", 0);
              var_set_str(vm, "LAST_ERR", "PARSEISO: bad time");
              var_set_str(vm, "ERR", "PARSEISO: bad time");
              bump(vm); return 1;
            }
            H = hs; M = ms; S = ss;
            if (S == 60) S = 59; /* leap second clamp */
          }
        }
      }
      /* civil (y, mo, d) → days since 1970-01-01 (UTC), proleptic Gregorian */
      {
        long Y = (long)y;
        long mp = (long)mo;
        long dy = (long)d;
        Y -= mp <= 2;
        era = (Y >= 0 ? Y : Y - 399) / 400;
        yoe = Y - era * 400;
        doy = (153 * (mp + (mp > 2 ? -3 : 9)) + 2) / 5 + dy - 1;
        doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
        days = era * 146097 + doe - 719468; /* days since 1970-01-01 */
      }
      epoch = days * 86400L + (long)H * 3600L + (long)M * 60L + (long)S;
      if (epoch < 0) epoch = 0;
      snprintf(out, sizeof out, "%ld", epoch);
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = epoch;
      var_set_num(vm, "LAST_N", epoch);
      var_set_num(vm, "PARSEISO_N", epoch);
      var_set_num(vm, "TOEPOCH_N", epoch);
      var_set_num(vm, "TIME", epoch);
      var_set_str(vm, "PARSEISO", out);
      var_set_str(vm, "TOEPOCH", out);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS ADDISO|SHIFTISO|ISOADD|PLUSISO [iso] [secs|"duration"]
     * — parse ISO stamp, add seconds (or PARSEMS duration → floor ms/1000),
     * re-format as UTC ISO. LAST = new stamp; LAST_N / ADDISO_N / TIME = new epoch.
     * One arg number/duration: offset only, ISO from prior LAST (DATE chain).
     * One arg ISO string: add 0 (normalize). Soft-fail bad ISO/duration.
     * Usability: schedule plates without PARSEISO + ADDTIME + FROMTIME glue. */
    if (kw(&L->cur,"ADDISO") || kw(&L->cur,"SHIFTISO") || kw(&L->cur,"ISOADD") ||
        kw(&L->cur,"PLUSISO") || kw(&L->cur,"ISO_ADD") || kw(&L->cur,"ADDDATE") ||
        kw(&L->cur,"SHIFTDATE") || kw(&L->cur,"DATEADD") || kw(&L->cur,"MOVEISO") ||
        kw(&L->cur,"OFFSETISO") || kw(&L->cur,"ISOOFFSET")){
      char iso_in[128], iso_out[40], tok[256];
      long epoch = 0, secs = 0, ms = 0, out_ep = 0;
      int has_iso = 0, has_off = 0;
      const char *err = NULL, *derr = NULL;
      lex_next(L);
      iso_in[0] = 0;
      tok[0] = 0;
      /* number-only first arg → offset; ISO from prior LAST string */
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS) {
        secs = parse_expr(vm, L);
        has_off = 1;
        snprintf(iso_in, sizeof iso_in, "%s", vm->last_str);
        has_iso = iso_in[0] ? 1 : 0;
      } else if (L->cur.kind == TK_STR ||
                 (L->cur.kind == TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
                  !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE"))) {
        int got = 0;
        if (L->cur.kind == TK_STR || strcmp(L->cur.text, "LAST") == 0) {
          got = (resolve_str_arg(vm, L, tok, sizeof tok) == 0);
        } else {
          Var *sv = var_get(vm, L->cur.text, 0);
          if (sv && sv->is_str)
            got = (resolve_str_arg(vm, L, tok, sizeof tok) == 0);
          else {
            /* numeric ident → offset, ISO from LAST */
            secs = parse_expr(vm, L);
            has_off = 1;
            snprintf(iso_in, sizeof iso_in, "%s", vm->last_str);
            has_iso = iso_in[0] ? 1 : 0;
            got = 0;
          }
        }
        if (got) {
          /* Prefer ISO parse; if fail, treat as duration offset on prior LAST */
          if (cubalc_parse_iso_epoch(tok, &epoch, &err) == 0) {
            snprintf(iso_in, sizeof iso_in, "%s", tok);
            has_iso = 1;
            /* optional second offset */
            if (L->cur.kind == TK_STR) {
              if (resolve_str_arg(vm, L, tok, sizeof tok) == 0) {
                if (cubalc_parse_duration_ms(tok, &ms, NULL, &derr) != 0) {
                  var_set_str(vm, "LAST", "");
                  vm->last_str[0] = 0;
                  vm->last_n = 0;
                  var_set_num(vm, "LAST_N", 0);
                  var_set_num(vm, "OK", 0);
                  var_set_str(vm, "LAST_ERR", derr ? derr : "ADDISO: bad duration");
                  var_set_str(vm, "ERR", derr ? derr : "ADDISO: bad duration");
                  bump(vm); return 1;
                }
                secs = ms / 1000L;
                has_off = 1;
              }
            } else if (L->cur.kind == TK_IDENT && !kw(&L->cur,"ASSERT") &&
                       !kw(&L->cur,"LET") && !kw(&L->cur,"SYS") &&
                       !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE")) {
              Var *sv2 = (strcmp(L->cur.text, "LAST") == 0) ? NULL :
                         var_get(vm, L->cur.text, 0);
              if (strcmp(L->cur.text, "LAST") == 0 || (sv2 && sv2->is_str)) {
                if (resolve_str_arg(vm, L, tok, sizeof tok) == 0) {
                  if (cubalc_parse_duration_ms(tok, &ms, NULL, &derr) != 0) {
                    var_set_str(vm, "LAST", "");
                    vm->last_str[0] = 0;
                    vm->last_n = 0;
                    var_set_num(vm, "LAST_N", 0);
                    var_set_num(vm, "OK", 0);
                    var_set_str(vm, "LAST_ERR", derr ? derr : "ADDISO: bad duration");
                    var_set_str(vm, "ERR", derr ? derr : "ADDISO: bad duration");
                    bump(vm); return 1;
                  }
                  secs = ms / 1000L;
                  has_off = 1;
                }
              } else {
                secs = parse_expr(vm, L);
                has_off = 1;
              }
            } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
                       L->cur.kind == TK_MINUS) {
              secs = parse_expr(vm, L);
              has_off = 1;
            }
          } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
                     L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
                     (L->cur.kind == TK_IDENT && !kw(&L->cur,"ASSERT") &&
                      !kw(&L->cur,"LET") && !kw(&L->cur,"SYS") &&
                      !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE"))) {
            /* first token was meant as ISO but failed; consume offset junk then soft-fail */
            if (L->cur.kind == TK_STR ||
                (L->cur.kind == TK_IDENT &&
                 (strcmp(L->cur.text, "LAST") == 0 ||
                  (var_get(vm, L->cur.text, 0) && var_get(vm, L->cur.text, 0)->is_str)))) {
              char skip[256];
              (void)resolve_str_arg(vm, L, skip, sizeof skip);
            } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
                       L->cur.kind == TK_LPAREN || L->cur.kind == TK_IDENT) {
              (void)parse_expr(vm, L);
            }
            var_set_str(vm, "LAST", "");
            vm->last_str[0] = 0;
            vm->last_n = 0;
            var_set_num(vm, "LAST_N", 0);
            var_set_num(vm, "OK", 0);
            var_set_str(vm, "LAST_ERR", err ? err : "ADDISO: bad iso");
            var_set_str(vm, "ERR", err ? err : "ADDISO: bad iso");
            bump(vm); return 1;
          } else if (cubalc_parse_duration_ms(tok, &ms, NULL, &derr) == 0) {
            secs = ms / 1000L;
            has_off = 1;
            snprintf(iso_in, sizeof iso_in, "%s", vm->last_str);
            has_iso = iso_in[0] ? 1 : 0;
          } else {
            var_set_str(vm, "LAST", "");
            vm->last_str[0] = 0;
            vm->last_n = 0;
            var_set_num(vm, "LAST_N", 0);
            var_set_num(vm, "OK", 0);
            var_set_str(vm, "LAST_ERR", err ? err : "ADDISO: bad iso/duration");
            var_set_str(vm, "ERR", err ? err : "ADDISO: bad iso/duration");
            bump(vm); return 1;
          }
        }
      }
      if (!has_iso || !iso_in[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "ADDISO: empty iso");
        var_set_str(vm, "ERR", "ADDISO: empty iso");
        bump(vm); return 1;
      }
      if (cubalc_parse_iso_epoch(iso_in, &epoch, &err) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", err ? err : "ADDISO: bad iso");
        var_set_str(vm, "ERR", err ? err : "ADDISO: bad iso");
        bump(vm); return 1;
      }
      if (!has_off) secs = 0;
      if (secs >= 0) {
        if (epoch > LONG_MAX - secs) out_ep = LONG_MAX;
        else out_ep = epoch + secs;
      } else {
        if (epoch < -secs) out_ep = 0;
        else out_ep = epoch + secs;
      }
      cubalc_format_epoch_iso(out_ep, iso_out, sizeof iso_out);
      var_set_str(vm, "LAST", iso_out);
      var_set_str(vm, "ADDISO", iso_out);
      var_set_str(vm, "SHIFTISO", iso_out);
      var_set_str(vm, "ISO", iso_out);
      var_set_str(vm, "FROMTIME", iso_out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", iso_out);
      vm->last_n = out_ep;
      var_set_num(vm, "LAST_N", out_ep);
      var_set_num(vm, "ADDISO_N", out_ep);
      var_set_num(vm, "SHIFTISO_N", out_ep);
      var_set_num(vm, "ADDISO_SECS", secs);
      var_set_num(vm, "TIME", out_ep);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS DIFFISO|ISODIFF|SUBISO|ISOAGE [a] [b]
     * — signed second delta between ISO stamps (dual of ADDISO shift).
     * Two args: LAST_N = epoch(a) - epoch(b). One arg: now - epoch(a) (ISO age).
     * Zero args: soft empty. LAST = decimal secs; DIFFISO_N mirrors.
     * Soft-fail bad ISO. Usability: plate age/SLA without PARSEISO+TIMEDIFF. */
    if (kw(&L->cur,"DIFFISO") || kw(&L->cur,"ISODIFF") || kw(&L->cur,"SUBISO") ||
        kw(&L->cur,"ISOAGE") || kw(&L->cur,"AGEISO") || kw(&L->cur,"ISO_DIFF") ||
        kw(&L->cur,"DIFFDATE") || kw(&L->cur,"DATEDIFF") || kw(&L->cur,"ISODELTA") ||
        kw(&L->cur,"DELTAISO") || kw(&L->cur,"BETWEENISO")){
      char a_s[128], b_s[128];
      long ea = 0, eb = 0, outn = 0, now;
      int has_a = 0, has_b = 0;
      const char *err = NULL;
      char buf[40];
      lex_next(L);
      a_s[0] = 0; b_s[0] = 0;
      /* arg a: ISO string / LAST / str-var */
      if (L->cur.kind == TK_STR ||
          (L->cur.kind == TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE"))) {
        if (L->cur.kind == TK_STR || strcmp(L->cur.text, "LAST") == 0) {
          if (resolve_str_arg(vm, L, a_s, sizeof a_s) == 0)
            has_a = 1;
        } else {
          Var *sv = var_get(vm, L->cur.text, 0);
          if (sv && sv->is_str) {
            if (resolve_str_arg(vm, L, a_s, sizeof a_s) == 0)
              has_a = 1;
          }
        }
      }
      /* arg b optional */
      if (L->cur.kind == TK_STR ||
          (L->cur.kind == TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE"))) {
        if (L->cur.kind == TK_STR || strcmp(L->cur.text, "LAST") == 0) {
          if (resolve_str_arg(vm, L, b_s, sizeof b_s) == 0)
            has_b = 1;
        } else {
          Var *sv = var_get(vm, L->cur.text, 0);
          if (sv && sv->is_str) {
            if (resolve_str_arg(vm, L, b_s, sizeof b_s) == 0)
              has_b = 1;
          }
        }
      }
      if (!has_a || !a_s[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DIFFISO_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "DIFFISO: empty");
        var_set_str(vm, "ERR", "DIFFISO: empty");
        bump(vm); return 1;
      }
      if (cubalc_parse_iso_epoch(a_s, &ea, &err) != 0) {
        /* if second arg was present but first bad — already consumed b_s */
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DIFFISO_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", err ? err : "DIFFISO: bad iso a");
        var_set_str(vm, "ERR", err ? err : "DIFFISO: bad iso a");
        bump(vm); return 1;
      }
      now = (long)time(NULL);
      if (has_b) {
        if (!b_s[0] || cubalc_parse_iso_epoch(b_s, &eb, &err) != 0) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "DIFFISO_N", 0);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", err ? err : "DIFFISO: bad iso b");
          var_set_str(vm, "ERR", err ? err : "DIFFISO: bad iso b");
          bump(vm); return 1;
        }
        outn = ea - eb;
      } else {
        /* one arg: age of stamp vs wall now (dual of TIMEDIFF one-arg) */
        outn = now - ea;
      }
      snprintf(buf, sizeof buf, "%ld", outn);
      var_set_str(vm, "LAST", buf);
      var_set_str(vm, "DIFFISO", buf);
      var_set_str(vm, "ISODIFF", buf);
      var_set_str(vm, "ISOAGE", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      vm->last_n = outn;
      var_set_num(vm, "LAST_N", outn);
      var_set_num(vm, "DIFFISO_N", outn);
      var_set_num(vm, "ISODIFF_N", outn);
      var_set_num(vm, "ISOAGE_N", outn);
      var_set_num(vm, "TIMEDIFF_N", outn);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS TIMEDIFF|ELAPSED|DELTA|DIFFTIME [a] [b]
     * — epoch-second difference for plate age/deadline math.
     * Two args: LAST_N = a - b (signed). One arg: LAST_N = now - a.
     * Zero args: LAST_N = 0. LAST = decimal; TIMEDIFF_N mirrors.
     * Usability: after PARSEISO/FROMTIME/MTIME compare without shell date.
     * Distinct from SYS AGE (file mtime age by path). */
    if (kw(&L->cur,"TIMEDIFF") || kw(&L->cur,"ELAPSED") || kw(&L->cur,"DELTA") ||
        kw(&L->cur,"DIFFTIME") || kw(&L->cur,"SECDIFF") || kw(&L->cur,"EPOCHDIFF") ||
        kw(&L->cur,"TDIFF") || kw(&L->cur,"DIFFSEC") || kw(&L->cur,"SUBTIME") ||
        kw(&L->cur,"TIMEDELTA") || kw(&L->cur,"DTIME")){
      long a = 0, b = 0, outn = 0;
      int has_a = 0, has_b = 0;
      char buf[40];
      long now;
      lex_next(L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE"))){
        a = parse_expr(vm, L);
        has_a = 1;
      }
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE"))){
        b = parse_expr(vm, L);
        has_b = 1;
      }
      now = (long)time(NULL);
      if (has_a && has_b) {
        outn = a - b;
      } else if (has_a) {
        outn = now - a;
      } else {
        /* default: age of prior LAST_N as epoch */
        outn = now - vm->last_n;
      }
      snprintf(buf, sizeof buf, "%ld", outn);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      vm->last_n = outn;
      var_set_num(vm, "LAST_N", outn);
      var_set_num(vm, "TIMEDIFF_N", outn);
      var_set_num(vm, "ELAPSED_N", outn);
      var_set_num(vm, "DELTA_N", outn);
      var_set_str(vm, "TIMEDIFF", buf);
      var_set_str(vm, "ELAPSED", buf);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS ADDTIME|ADDEPOCH|PLUSSECS [a] [b]
     * — epoch + seconds for lease/deadline plates (dual of TIMEDIFF subtract).
     * Two args: LAST_N = a + b. One arg: LAST_N = prior LAST_N + a.
     * Zero args: LAST_N unchanged (no-op add 0). LAST = decimal epoch.
     * Offset args may be PARSEMS duration strings ("5m","30s","1h30m") → floor ms/1000.
     * SYS DEADLINE|EXPIRESAT|UNTIL [secs|"duration"]
     * — wall now + secs → LAST_N epoch (lease expiry stamp).
     * Zero args: now + prior LAST_N. Usability: leases without PARSEMS+MS2SEC glue.
     * Soft-fail bad duration (OK=0). Pair with EXPIRED/REMAINING/FROMTIME. */
    if (kw(&L->cur,"ADDTIME") || kw(&L->cur,"ADDEPOCH") || kw(&L->cur,"PLUSSECS") ||
        kw(&L->cur,"EPOCHADD") || kw(&L->cur,"TIMEADD") || kw(&L->cur,"ADDSECS") ||
        kw(&L->cur,"PLUSTIME") ||
        kw(&L->cur,"DEADLINE") || kw(&L->cur,"EXPIRESAT") || kw(&L->cur,"UNTIL") ||
        kw(&L->cur,"EXPIREAT") || kw(&L->cur,"LEASEEND") || kw(&L->cur,"DUEAT") ||
        kw(&L->cur,"NOWPLUS")){
      char op[24], src[256], buf[40];
      int is_deadline = 0;
      long a = 0, b = 0, outn = 0, now, ms = 0;
      int has_a = 0, has_b = 0;
      const char *derr = NULL;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *p = op; *p; p++)
        if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 'a' + 'A');
      is_deadline = (strcmp(op, "DEADLINE") == 0 || strcmp(op, "EXPIRESAT") == 0 ||
                     strcmp(op, "UNTIL") == 0 || strcmp(op, "EXPIREAT") == 0 ||
                     strcmp(op, "LEASEEND") == 0 || strcmp(op, "DUEAT") == 0 ||
                     strcmp(op, "NOWPLUS") == 0);
      lex_next(L);
      /* arg a: number secs or duration string → floor(ms/1000) secs */
      src[0] = 0;
      if (L->cur.kind == TK_STR) {
        if (resolve_str_arg(vm, L, src, sizeof src) == 0) {
          if (cubalc_parse_duration_ms(src, &ms, NULL, &derr) != 0) {
            var_set_str(vm, "LAST", "");
            vm->last_str[0] = 0;
            vm->last_n = 0;
            var_set_num(vm, "LAST_N", 0);
            var_set_num(vm, "OK", 0);
            var_set_str(vm, "LAST_ERR", derr ? derr : "DEADLINE: bad duration");
            var_set_str(vm, "ERR", derr ? derr : "DEADLINE: bad duration");
            bump(vm); return 1;
          }
          a = ms / 1000L;
          has_a = 1;
        }
      } else if (L->cur.kind == TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
                 !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE")) {
        if (strcmp(L->cur.text, "LAST") == 0) {
          if (resolve_str_arg(vm, L, src, sizeof src) == 0) {
            if (cubalc_parse_duration_ms(src, &ms, NULL, &derr) != 0) {
              var_set_str(vm, "LAST", "");
              vm->last_str[0] = 0;
              vm->last_n = 0;
              var_set_num(vm, "LAST_N", 0);
              var_set_num(vm, "OK", 0);
              var_set_str(vm, "LAST_ERR", derr ? derr : "DEADLINE: bad duration");
              var_set_str(vm, "ERR", derr ? derr : "DEADLINE: bad duration");
              bump(vm); return 1;
            }
            a = ms / 1000L;
            has_a = 1;
          }
        } else {
          Var *sv = var_get(vm, L->cur.text, 0);
          if (sv && sv->is_str) {
            if (resolve_str_arg(vm, L, src, sizeof src) == 0) {
              if (cubalc_parse_duration_ms(src, &ms, NULL, &derr) != 0) {
                var_set_str(vm, "LAST", "");
                vm->last_str[0] = 0;
                vm->last_n = 0;
                var_set_num(vm, "LAST_N", 0);
                var_set_num(vm, "OK", 0);
                var_set_str(vm, "LAST_ERR", derr ? derr : "DEADLINE: bad duration");
                var_set_str(vm, "ERR", derr ? derr : "DEADLINE: bad duration");
                bump(vm); return 1;
              }
              a = ms / 1000L;
              has_a = 1;
            }
          } else {
            a = parse_expr(vm, L);
            has_a = 1;
          }
        }
      } else if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS) {
        a = parse_expr(vm, L);
        has_a = 1;
      }
      /* ADDTIME second arg: same number|duration rules */
      if (!is_deadline) {
        src[0] = 0;
        derr = NULL;
        if (L->cur.kind == TK_STR) {
          if (resolve_str_arg(vm, L, src, sizeof src) == 0) {
            if (cubalc_parse_duration_ms(src, &ms, NULL, &derr) != 0) {
              var_set_str(vm, "LAST", "");
              vm->last_str[0] = 0;
              vm->last_n = 0;
              var_set_num(vm, "LAST_N", 0);
              var_set_num(vm, "OK", 0);
              var_set_str(vm, "LAST_ERR", derr ? derr : "ADDTIME: bad duration");
              var_set_str(vm, "ERR", derr ? derr : "ADDTIME: bad duration");
              bump(vm); return 1;
            }
            b = ms / 1000L;
            has_b = 1;
          }
        } else if (L->cur.kind == TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
                   !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE")) {
          if (strcmp(L->cur.text, "LAST") == 0) {
            if (resolve_str_arg(vm, L, src, sizeof src) == 0) {
              if (cubalc_parse_duration_ms(src, &ms, NULL, &derr) != 0) {
                var_set_str(vm, "LAST", "");
                vm->last_str[0] = 0;
                vm->last_n = 0;
                var_set_num(vm, "LAST_N", 0);
                var_set_num(vm, "OK", 0);
                var_set_str(vm, "LAST_ERR", derr ? derr : "ADDTIME: bad duration");
                var_set_str(vm, "ERR", derr ? derr : "ADDTIME: bad duration");
                bump(vm); return 1;
              }
              b = ms / 1000L;
              has_b = 1;
            }
          } else {
            Var *sv = var_get(vm, L->cur.text, 0);
            if (sv && sv->is_str) {
              if (resolve_str_arg(vm, L, src, sizeof src) == 0) {
                if (cubalc_parse_duration_ms(src, &ms, NULL, &derr) != 0) {
                  var_set_str(vm, "LAST", "");
                  vm->last_str[0] = 0;
                  vm->last_n = 0;
                  var_set_num(vm, "LAST_N", 0);
                  var_set_num(vm, "OK", 0);
                  var_set_str(vm, "LAST_ERR", derr ? derr : "ADDTIME: bad duration");
                  var_set_str(vm, "ERR", derr ? derr : "ADDTIME: bad duration");
                  bump(vm); return 1;
                }
                b = ms / 1000L;
                has_b = 1;
              }
            } else {
              b = parse_expr(vm, L);
              has_b = 1;
            }
          }
        } else if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS) {
          b = parse_expr(vm, L);
          has_b = 1;
        }
      }
      now = (long)time(NULL);
      if (is_deadline) {
        if (has_a)
          outn = now + a;
        else
          outn = now + vm->last_n;
        /* surface offset used for plates (seconds) */
        var_set_num(vm, "DEADLINE_SECS", has_a ? a : vm->last_n);
        var_set_num(vm, "LEASE_SECS", has_a ? a : vm->last_n);
      } else {
        if (has_a && has_b)
          outn = a + b;
        else if (has_a)
          outn = vm->last_n + a;
        else
          outn = vm->last_n;
      }
      snprintf(buf, sizeof buf, "%ld", outn);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      vm->last_n = outn;
      var_set_num(vm, "LAST_N", outn);
      var_set_num(vm, "TIME", outn);
      if (is_deadline) {
        var_set_num(vm, "DEADLINE_N", outn);
        var_set_str(vm, "DEADLINE", buf);
        var_set_num(vm, "EXPIRESAT_N", outn);
      } else {
        var_set_num(vm, "ADDTIME_N", outn);
        var_set_str(vm, "ADDTIME", buf);
      }
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS EXPIRED|ISEXPIRED|PAST|ISPAST [epoch|LAST_N]
     * — LAST_N 1 if wall now >= epoch (lease due), else 0.
     * SYS REMAINING|LEFTSECS|TTL|TIMELEFT [epoch|LAST_N]
     * — LAST_N = max(0, epoch - now) seconds until due (0 if past).
     * Default epoch = prior LAST_N (DEADLINE result).
     * Usability: SLA/lease IF without TIMEDIFF + CMP glue. */
    if (kw(&L->cur,"EXPIRED") || kw(&L->cur,"ISEXPIRED") || kw(&L->cur,"PAST") ||
        kw(&L->cur,"ISPAST") || kw(&L->cur,"IS_EXPIRED") || kw(&L->cur,"DUE") ||
        kw(&L->cur,"ISDUE") || kw(&L->cur,"OVERDUE") ||
        kw(&L->cur,"REMAINING") || kw(&L->cur,"LEFTSECS") || kw(&L->cur,"TTL") ||
        kw(&L->cur,"TIMELEFT") || kw(&L->cur,"SECSLEFT") || kw(&L->cur,"LEFT") ||
        kw(&L->cur,"COUNTDOWN") || kw(&L->cur,"UNTILLEFT")){
      char op[24];
      int is_rem = 0;
      long ep = 0, now, outn = 0;
      int has = 0;
      char buf[40];
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *p = op; *p; p++)
        if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 'a' + 'A');
      is_rem = (strcmp(op, "REMAINING") == 0 || strcmp(op, "LEFTSECS") == 0 ||
                strcmp(op, "TTL") == 0 || strcmp(op, "TIMELEFT") == 0 ||
                strcmp(op, "SECSLEFT") == 0 || strcmp(op, "LEFT") == 0 ||
                strcmp(op, "COUNTDOWN") == 0 || strcmp(op, "UNTILLEFT") == 0);
      lex_next(L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE"))){
        ep = parse_expr(vm, L);
        has = 1;
      }
      if (!has) ep = vm->last_n;
      now = (long)time(NULL);
      if (is_rem) {
        outn = ep - now;
        if (outn < 0) outn = 0;
      } else {
        outn = (now >= ep) ? 1 : 0;
      }
      snprintf(buf, sizeof buf, "%ld", outn);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      vm->last_n = outn;
      var_set_num(vm, "LAST_N", outn);
      if (is_rem) {
        var_set_num(vm, "REMAINING_N", outn);
        var_set_num(vm, "TTL_N", outn);
        var_set_str(vm, "REMAINING", buf);
        var_set_str(vm, "TTL", buf);
      } else {
        var_set_num(vm, "EXPIRED_N", outn);
        var_set_num(vm, "ISEXPIRED", outn);
        var_set_num(vm, "PAST_N", outn);
      }
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS LOCAL|LOCALTIME|LOCALISO|NOW_LOCAL — local wall stamp (no Z).
     * SYS LOCALDATE|DAYLOCAL — date-only YYYY-MM-DD in local TZ.
     * LAST/LOCAL/LOCALTIME = "YYYY-MM-DDTHH:MM:SS"; LOCALDATE = "YYYY-MM-DD".
     * Usability: human log stamps without shell date; complements UTC DATE. */
    if (kw(&L->cur,"LOCAL") || kw(&L->cur,"LOCALTIME") || kw(&L->cur,"LOCALISO") ||
        kw(&L->cur,"NOW_LOCAL") || kw(&L->cur,"LOCAL_NOW") || kw(&L->cur,"WALLTIME") ||
        kw(&L->cur,"LOCALDATE") || kw(&L->cur,"DAYLOCAL") || kw(&L->cur,"LOCAL_DATE") ||
        kw(&L->cur,"TODAY_LOCAL") || kw(&L->cur,"LOCALDAY")){
      time_t now;
      struct tm tm_loc;
      char full[40], day[16];
      long epoch;
      int date_only;
      date_only = (kw(&L->cur,"LOCALDATE") || kw(&L->cur,"DAYLOCAL") ||
                   kw(&L->cur,"LOCAL_DATE") || kw(&L->cur,"TODAY_LOCAL") ||
                   kw(&L->cur,"LOCALDAY"));
      lex_next(L);
      now = time(NULL);
      epoch = (long)now;
#if defined(CUBALC_OS_WINDOWS)
      {
        struct tm *tp = localtime(&now);
        if (tp) tm_loc = *tp;
        else memset(&tm_loc, 0, sizeof tm_loc);
      }
#else
      if (!localtime_r(&now, &tm_loc))
        memset(&tm_loc, 0, sizeof tm_loc);
#endif
      snprintf(day, sizeof day, "%04d-%02d-%02d",
               tm_loc.tm_year + 1900, tm_loc.tm_mon + 1, tm_loc.tm_mday);
      snprintf(full, sizeof full, "%04d-%02d-%02dT%02d:%02d:%02d",
               tm_loc.tm_year + 1900, tm_loc.tm_mon + 1, tm_loc.tm_mday,
               tm_loc.tm_hour, tm_loc.tm_min, tm_loc.tm_sec);
      var_set_str(vm, "LOCAL", full);
      var_set_str(vm, "LOCALTIME", full);
      var_set_str(vm, "LOCALISO", full);
      var_set_str(vm, "LOCALDATE", day);
      var_set_num(vm, "TIME", epoch);
      var_set_num(vm, "OK", 1);
      if (date_only) {
        var_set_str(vm, "LAST", day);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", day);
        vm->last_n = (long)strlen(day);
      } else {
        var_set_str(vm, "LAST", full);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", full);
        vm->last_n = (long)strlen(full);
      }
      var_set_num(vm, "LAST_N", vm->last_n);
      bump(vm); return 1;
    }
  return 0;
}
