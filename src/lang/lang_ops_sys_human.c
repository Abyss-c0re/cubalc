/* CubalC lang — lang_ops_sys_human.c
 * SYS human format residual: HUMANSIZE · PARSESIZE · HUMANMS · HUMANSECS · MS2SEC (A2 peel).
 * Mesh vote B_core_sys · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_sys_human.h"
#include "lang/cubalc_lang_ops_sys_time_walk.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>

int cubalc_lang_ops_sys_try_human(VM *vm, Lex *L) {
  /* SYS human-size/duration residual. Caller consumed SYS|HOST.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
    /* SYS HUMANSIZE|BYTESFMT|HUMANSZ|PRETTYBYTES [n|LAST_N]
     * — integer B/K/M/G/T label for byte counts (1024-base floor).
     * LAST = e.g. "1K"; LAST_N = scaled value; HUMANSIZE_UNIT; HUMANSIZE_BYTES.
     * Usability: SIZE/SIZEALL inventory plates without shell numfmt. */
    if (kw(&L->cur,"HUMANSIZE") || kw(&L->cur,"BYTESFMT") || kw(&L->cur,"HUMANSZ") ||
        kw(&L->cur,"PRETTYBYTES") || kw(&L->cur,"HSIZE") || kw(&L->cur,"BYTEHUMAN") ||
        kw(&L->cur,"FMTBYTES") || kw(&L->cur,"HUMANBYTES") || kw(&L->cur,"SIZEFMT")){
      long n = vm->last_n;
      unsigned long long u, scaled;
      const char *unit = "B";
      char buf[48];
      lex_next(L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE"))){
        n = parse_expr(vm, L);
      }
      if (n < 0) n = 0;
      u = (unsigned long long)n;
      if (u >= 1024ULL * 1024ULL * 1024ULL * 1024ULL) {
        scaled = u / (1024ULL * 1024ULL * 1024ULL * 1024ULL);
        unit = "T";
      } else if (u >= 1024ULL * 1024ULL * 1024ULL) {
        scaled = u / (1024ULL * 1024ULL * 1024ULL);
        unit = "G";
      } else if (u >= 1024ULL * 1024ULL) {
        scaled = u / (1024ULL * 1024ULL);
        unit = "M";
      } else if (u >= 1024ULL) {
        scaled = u / 1024ULL;
        unit = "K";
      } else {
        scaled = u;
        unit = "B";
      }
      snprintf(buf, sizeof buf, "%llu%s", scaled, unit);
      var_set_str(vm, "LAST", buf);
      var_set_str(vm, "HUMANSIZE", buf);
      var_set_str(vm, "BYTESFMT", buf);
      var_set_str(vm, "HUMANSIZE_UNIT", unit);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      vm->last_n = (long)scaled;
      var_set_num(vm, "LAST_N", (long)scaled);
      var_set_num(vm, "HUMANSIZE_N", (long)scaled);
      var_set_num(vm, "HUMANSIZE_BYTES", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS PARSESIZE|FROMHUMAN|PARSEBYTES|UNHUMAN [str]
     * — parse "512" / "1K" / "2M" / "1G" / "1T" (1024-base) → bytes.
     * LAST = decimal string of bytes; LAST_N / PARSESIZE_N = bytes.
     * Soft fail OK=0 on empty/bad unit. Usability: agent size configs reverse of HUMANSIZE. */
    if (kw(&L->cur,"PARSESIZE") || kw(&L->cur,"FROMHUMAN") || kw(&L->cur,"PARSEBYTES") ||
        kw(&L->cur,"UNHUMAN") || kw(&L->cur,"PARSEHUMAN") || kw(&L->cur,"HUMANPARSE") ||
        kw(&L->cur,"TOBYTES") || kw(&L->cur,"SIZEPARSE") || kw(&L->cur,"BYTESPARSE")){
      char src[256], out[48], unitc = 0;
      const char *p;
      unsigned long long n = 0, mul = 1;
      char *end = NULL;
      lex_next(L);
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      /* trim leading/trailing whitespace */
      p = src;
      while (*p == ' ' || *p == '\t') p++;
      if (!*p) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "PARSESIZE_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "PARSESIZE: empty");
        var_set_str(vm, "ERR", "PARSESIZE: empty");
        bump(vm); return 1;
      }
      n = strtoull(p, &end, 10);
      if (end == p) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "PARSESIZE_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "PARSESIZE: bad number");
        var_set_str(vm, "ERR", "PARSESIZE: bad number");
        bump(vm); return 1;
      }
      while (*end == ' ' || *end == '\t') end++;
      unitc = *end;
      if (unitc >= 'a' && unitc <= 'z') unitc = (char)(unitc - 'a' + 'A');
      if (unitc == 0 || unitc == 'B') {
        mul = 1;
      } else if (unitc == 'K') {
        mul = 1024ULL;
      } else if (unitc == 'M') {
        mul = 1024ULL * 1024ULL;
      } else if (unitc == 'G') {
        mul = 1024ULL * 1024ULL * 1024ULL;
      } else if (unitc == 'T') {
        mul = 1024ULL * 1024ULL * 1024ULL * 1024ULL;
      } else {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "PARSESIZE_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "PARSESIZE: bad unit");
        var_set_str(vm, "ERR", "PARSESIZE: bad unit");
        bump(vm); return 1;
      }
      /* optional trailing iB/B after unit letter (KiB, MB) — skip one more letter if B/I */
      if (unitc && end[1]) {
        char c2 = end[1];
        if (c2 >= 'a' && c2 <= 'z') c2 = (char)(c2 - 'a' + 'A');
        if (!(c2 == 'B' || c2 == 'I' || c2 == 0 || c2 == ' ' || c2 == '\t')) {
          /* e.g. "1XX" — only allow B/i after unit */
          if (unitc != 'B') {
            /* already consumed unit; if junk after, soft fail unless only B/iB */
            var_set_str(vm, "LAST", "");
            vm->last_str[0] = 0;
            vm->last_n = 0;
            var_set_num(vm, "LAST_N", 0);
            var_set_num(vm, "PARSESIZE_N", 0);
            var_set_num(vm, "OK", 0);
            var_set_str(vm, "LAST_ERR", "PARSESIZE: bad unit");
            var_set_str(vm, "ERR", "PARSESIZE: bad unit");
            bump(vm); return 1;
          }
        }
      }
      n = n * mul;
      if (n > (unsigned long long)LONG_MAX) n = (unsigned long long)LONG_MAX;
      snprintf(out, sizeof out, "%llu", n);
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)n;
      var_set_num(vm, "LAST_N", (long)n);
      var_set_num(vm, "PARSESIZE_N", (long)n);
      var_set_str(vm, "PARSESIZE", out);
      var_set_str(vm, "FROMHUMAN", out);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS PARSEMS|DURATION|TOMS|PARSEDUR [str]
     * — parse duration → milliseconds. Single: "250"/"250ms"/"5s"/"2m"/"1h"/"1d"/"1w".
     * Compound (HUMANMS dual): "1h30m", "2m15s", "1d12h", "1s250ms" (spaces ok).
     * Bare number = ms. LAST_N / PARSEMS_N = total ms. Soft fail empty/bad.
     * Usability: config timeouts + round-trip HUMANMS without shell. Cap LONG_MAX. */
    if (kw(&L->cur,"PARSEMS") || kw(&L->cur,"DURATION") || kw(&L->cur,"TOMS") ||
        kw(&L->cur,"PARSEDUR") || kw(&L->cur,"DURMS") || kw(&L->cur,"MSDUR") ||
        kw(&L->cur,"PARSE_MS") || kw(&L->cur,"TO_MS") || kw(&L->cur,"ASMS") ||
        kw(&L->cur,"HUMANMS_PARSE") || kw(&L->cur,"PARSE_DURATION")){
      char src[256], out[48];
      long total = 0;
      int segs = 0;
      const char *err = NULL;
      lex_next(L);
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      if (cubalc_parse_duration_ms(src, &total, &segs, &err) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "PARSEMS_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", err ? err : "PARSEMS: fail");
        var_set_str(vm, "ERR", err ? err : "PARSEMS: fail");
        bump(vm); return 1;
      }
      snprintf(out, sizeof out, "%ld", total);
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = total;
      var_set_num(vm, "LAST_N", total);
      var_set_num(vm, "PARSEMS_N", total);
      var_set_num(vm, "DURATION_N", total);
      var_set_num(vm, "PARSEMS_SEGS", (long)segs);
      var_set_str(vm, "PARSEMS", out);
      var_set_str(vm, "DURATION", out);
      var_set_str(vm, "TOMS", out);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS HUMANMS|FMTMS|HUMANDUR|PRETTYMS [n|LAST_N]
     * — format milliseconds as compact human duration (reverse of PARSEMS).
     * LAST = e.g. "5s", "1m30s", "1h2m", "250ms"; LAST_N / HUMANMS_MS = original ms.
     * Units: w d h m s ms (compound, omit zero parts). Usability: plate labels for
     * WAITFILE_MS / SLEEP_MS / elapsed without shell. */
    if (kw(&L->cur,"HUMANMS") || kw(&L->cur,"FMTMS") || kw(&L->cur,"HUMANDUR") ||
        kw(&L->cur,"PRETTYMS") || kw(&L->cur,"MSFMT") || kw(&L->cur,"FMTTIME") ||
        kw(&L->cur,"DURSTR") || kw(&L->cur,"MSHUMAN") || kw(&L->cur,"PRETTYDUR") ||
        kw(&L->cur,"FORMATMS") || kw(&L->cur,"MS_HUMAN")){
      long n = vm->last_n;
      unsigned long long u, w, d, h, m, s, ms;
      char buf[96];
      size_t o = 0;
      int parts = 0;
      lex_next(L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE"))){
        n = parse_expr(vm, L);
      }
      if (n < 0) n = 0;
      u = (unsigned long long)n;
      w = u / (7ULL * 24ULL * 60ULL * 60ULL * 1000ULL);
      u %= (7ULL * 24ULL * 60ULL * 60ULL * 1000ULL);
      d = u / (24ULL * 60ULL * 60ULL * 1000ULL);
      u %= (24ULL * 60ULL * 60ULL * 1000ULL);
      h = u / (60ULL * 60ULL * 1000ULL);
      u %= (60ULL * 60ULL * 1000ULL);
      m = u / (60ULL * 1000ULL);
      u %= (60ULL * 1000ULL);
      s = u / 1000ULL;
      ms = u % 1000ULL;
      buf[0] = 0;
      o = 0;
#define CUBALC_HMS_PART(val, suf) do { \
        if ((val) > 0) { \
          int wrote = snprintf(buf + o, sizeof(buf) - o, "%llu%s", \
                               (unsigned long long)(val), (suf)); \
          if (wrote < 0 || (size_t)wrote >= sizeof(buf) - o) wrote = 0; \
          o += (size_t)wrote; \
          parts++; \
        } \
      } while (0)
      CUBALC_HMS_PART(w, "w");
      CUBALC_HMS_PART(d, "d");
      CUBALC_HMS_PART(h, "h");
      CUBALC_HMS_PART(m, "m");
      CUBALC_HMS_PART(s, "s");
      /* include ms when non-zero, or when whole duration is zero, or only sub-second */
      if (ms > 0 || parts == 0) {
        int wrote = snprintf(buf + o, sizeof(buf) - o, "%llums",
                             (unsigned long long)ms);
        if (wrote < 0 || (size_t)wrote >= sizeof(buf) - o) wrote = 0;
        o += (size_t)wrote;
        parts++;
      }
#undef CUBALC_HMS_PART
      if (o >= sizeof(buf)) o = sizeof(buf) - 1;
      buf[o] = 0;
      var_set_str(vm, "LAST", buf);
      var_set_str(vm, "HUMANMS", buf);
      var_set_str(vm, "FMTMS", buf);
      var_set_str(vm, "HUMANDUR", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "HUMANMS_N", n);
      var_set_num(vm, "HUMANMS_MS", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS HUMANSECS|HUMANAGE|FMTSECS|SECSFMT|PRETTYSECS [n|LAST_N]
     * — format non-negative seconds as compact human duration (no ms unit).
     * LAST e.g. "0s", "5s", "1m30s", "2h", "1d". LAST_N / HUMANSECS_N = input secs.
     * Signed input: leading "-" on label, magnitude formatted (DIFFISO ages).
     * Usability: AGE / REMAINING / TIMEDIFF / DIFFISO plate labels without
     * SEC2MS + HUMANMS glue. Dual of HUMANMS for host second-plane values. */
    if (kw(&L->cur,"HUMANSECS") || kw(&L->cur,"HUMANAGE") || kw(&L->cur,"FMTSECS") ||
        kw(&L->cur,"SECSFMT") || kw(&L->cur,"PRETTYSECS") || kw(&L->cur,"SECSHUMAN") ||
        kw(&L->cur,"HUMANS") || kw(&L->cur,"AGESTR") || kw(&L->cur,"FMTAGE") ||
        kw(&L->cur,"SECHUMAN") || kw(&L->cur,"PRETTYAGE") || kw(&L->cur,"DURSECS")){
      long n = vm->last_n, mag;
      int neg = 0;
      unsigned long long u, w, d, h, m, s;
      char buf[96];
      size_t o = 0;
      int parts = 0;
      lex_next(L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE"))){
        n = parse_expr(vm, L);
      }
      mag = n;
      if (mag < 0) {
        neg = 1;
        if (mag <= -LONG_MAX) mag = LONG_MAX;
        else mag = -mag;
      }
      u = (unsigned long long)mag;
      w = u / (7ULL * 24ULL * 60ULL * 60ULL);
      u %= (7ULL * 24ULL * 60ULL * 60ULL);
      d = u / (24ULL * 60ULL * 60ULL);
      u %= (24ULL * 60ULL * 60ULL);
      h = u / (60ULL * 60ULL);
      u %= (60ULL * 60ULL);
      m = u / 60ULL;
      s = u % 60ULL;
      buf[0] = 0;
      o = 0;
      if (neg) {
        buf[o++] = '-';
        buf[o] = 0;
      }
#define CUBALC_HSEC_PART(val, suf) do { \
        if ((val) > 0) { \
          int wrote = snprintf(buf + o, sizeof(buf) - o, "%llu%s", \
                               (unsigned long long)(val), (suf)); \
          if (wrote < 0 || (size_t)wrote >= sizeof(buf) - o) wrote = 0; \
          o += (size_t)wrote; \
          parts++; \
        } \
      } while (0)
      CUBALC_HSEC_PART(w, "w");
      CUBALC_HSEC_PART(d, "d");
      CUBALC_HSEC_PART(h, "h");
      CUBALC_HSEC_PART(m, "m");
      /* always include seconds when zero whole duration or residual secs */
      if (s > 0 || parts == 0) {
        int wrote = snprintf(buf + o, sizeof(buf) - o, "%llus",
                             (unsigned long long)s);
        if (wrote < 0 || (size_t)wrote >= sizeof(buf) - o) wrote = 0;
        o += (size_t)wrote;
        parts++;
      }
#undef CUBALC_HSEC_PART
      if (o >= sizeof(buf)) o = sizeof(buf) - 1;
      buf[o] = 0;
      var_set_str(vm, "LAST", buf);
      var_set_str(vm, "HUMANSECS", buf);
      var_set_str(vm, "HUMANAGE", buf);
      var_set_str(vm, "FMTSECS", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "HUMANSECS_N", n);
      var_set_num(vm, "HUMANAGE_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MS2SEC|TOSECS|FROMMS|MS_TO_SEC [n|LAST_N]
     * — floor(ms / 1000) → seconds for DEADLINE/ADDTIME after PARSEMS.
     * SYS SEC2MS|MSECFROM|SECS2MS|SEC_TO_MS [n|LAST_N]
     * — secs * 1000 → ms for SLEEP/WAIT after REMAINING/TTL.
     * LAST_N = result; clamp n>=0. Usability: unit bridge without shell $(( )). */
    if (kw(&L->cur,"MS2SEC") || kw(&L->cur,"TOSECS") || kw(&L->cur,"FROMMS") ||
        kw(&L->cur,"MS_TO_SEC") || kw(&L->cur,"MSTOSEC") || kw(&L->cur,"MSSECS") ||
        kw(&L->cur,"MILLI2SEC") || kw(&L->cur,"MSDIV") ||
        kw(&L->cur,"SEC2MS") || kw(&L->cur,"MSECFROM") || kw(&L->cur,"SECS2MS") ||
        kw(&L->cur,"SEC_TO_MS") || kw(&L->cur,"SECTOMS") || kw(&L->cur,"S2MS") ||
        kw(&L->cur,"SECMS") || kw(&L->cur,"TOMILLISEC")){
      char op[24];
      int to_ms = 0;
      long n = vm->last_n, outn = 0;
      char buf[40];
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *p = op; *p; p++)
        if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 'a' + 'A');
      to_ms = (strcmp(op, "SEC2MS") == 0 || strcmp(op, "MSECFROM") == 0 ||
               strcmp(op, "SECS2MS") == 0 || strcmp(op, "SEC_TO_MS") == 0 ||
               strcmp(op, "SECTOMS") == 0 || strcmp(op, "S2MS") == 0 ||
               strcmp(op, "SECMS") == 0 || strcmp(op, "TOMILLISEC") == 0);
      lex_next(L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE"))){
        n = parse_expr(vm, L);
      }
      if (n < 0) n = 0;
      if (to_ms) {
        /* overflow guard: LONG_MAX/1000 */
        if (n > (LONG_MAX / 1000L))
          outn = LONG_MAX;
        else
          outn = n * 1000L;
        var_set_num(vm, "SEC2MS_N", outn);
        var_set_num(vm, "SEC2MS_SECS", n);
      } else {
        outn = n / 1000L;
        var_set_num(vm, "MS2SEC_N", outn);
        var_set_num(vm, "MS2SEC_MS", n);
      }
      snprintf(buf, sizeof buf, "%ld", outn);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      vm->last_n = outn;
      var_set_num(vm, "LAST_N", outn);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"LEN") || kw(&L->cur,"LENGTH") || kw(&L->cur,"STRLEN")){
      lex_next(L);
      long n = 0;
      if (L->cur.kind==TK_STR){ n = (long)strlen(L->cur.text); lex_next(L); }
      else if (L->cur.kind==TK_IDENT){
        if (strcmp(L->cur.text,"LAST")==0){ n = (long)strlen(vm->last_str); lex_next(L); }
        else {
          Var *v = var_get(vm, L->cur.text, 0);
          if (v && v->is_str) n = (long)strlen(v->sval);
          else if (v) n = v->val;
          lex_next(L);
        }
      } else n = (long)strlen(vm->last_str);
      vm->last_n = n; var_set_num(vm, "LAST_N", n); var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
  return 0;
}
