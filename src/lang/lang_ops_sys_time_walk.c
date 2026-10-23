/* CubalC lang — lang_ops_sys_time_walk.c
 * PARSEMS / PARSEISO / WALK / optional timeout helpers.
 * Mesh vote A_split_lang_ops_core · modular extract from lang_ops_core.
 */
#include "lang/cubalc_lang_ops_sys_time_walk.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <fnmatch.h>
#  include <dirent.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

/* Basename shell match for WALK (fnmatch when available). */
int cubalc_walk_match(const char *pat, const char *name) {
  if (!pat || !pat[0] || (pat[0] == '*' && pat[1] == 0)) return 1;
  if (!name) return 0;
#if !defined(CUBALC_OS_WINDOWS)
  return fnmatch(pat, name, 0) == 0;
#else
  return strstr(name, pat) != NULL;
#endif
}

/* Parse human duration → milliseconds (SYS PARSEMS rules).
 * Single: "250"/"250ms"/"5s"/"2m"/"1h"/"1d"/"1w".
 * Compound: "1h30m", "2m 15s", "1s250ms" (spaces ok). Bare number = ms.
 * Optional leading + or - (whole duration sign). Rejects strtoull wrap on "-".
 * Returns 0 on success; -1 on empty/bad. Optional segs_out / err_out. */
int cubalc_parse_duration_ms(const char *src, long *ms_out, int *segs_out,
                                    const char **err_out) {
  const char *p;
  char *end = NULL;
  char unit[16];
  unsigned long long total = 0, n = 0, mul = 1, add = 0;
  int ui = 0, segs = 0, bad = 0, sign = 1;
  const char *err = NULL;
  if (ms_out) *ms_out = 0;
  if (segs_out) *segs_out = 0;
  if (err_out) *err_out = NULL;
  p = src ? src : "";
  while (*p == ' ' || *p == '\t') p++;
  if (!*p) {
    err = "PARSEMS: empty";
    bad = 1;
  }
  /* leading sign applies to whole duration (ADDISO "-30s"); avoid strtoull wrap */
  if (!bad && *p == '+') {
    p++;
    while (*p == ' ' || *p == '\t') p++;
  } else if (!bad && *p == '-') {
    sign = -1;
    p++;
    while (*p == ' ' || *p == '\t') p++;
  }
  if (!bad && !*p) {
    err = "PARSEMS: empty";
    bad = 1;
  }
  while (!bad && *p) {
    while (*p == ' ' || *p == '\t') p++;
    if (!*p) break;
    n = strtoull(p, &end, 10);
    if (end == p) {
      err = "PARSEMS: bad number";
      bad = 1;
      break;
    }
    p = end;
    while (*p == ' ' || *p == '\t') p++;
    ui = 0;
    while (ui < (int)sizeof(unit) - 1 &&
           ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z'))) {
      char c = *p++;
      if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
      unit[ui++] = c;
    }
    unit[ui] = 0;
    if (unit[0] == 0 || strcmp(unit, "ms") == 0 || strcmp(unit, "msec") == 0 ||
        strcmp(unit, "msecs") == 0 || strcmp(unit, "millisecond") == 0 ||
        strcmp(unit, "milliseconds") == 0) {
      mul = 1;
    } else if (strcmp(unit, "s") == 0 || strcmp(unit, "sec") == 0 ||
               strcmp(unit, "secs") == 0 || strcmp(unit, "second") == 0 ||
               strcmp(unit, "seconds") == 0) {
      mul = 1000ULL;
    } else if (strcmp(unit, "m") == 0 || strcmp(unit, "min") == 0 ||
               strcmp(unit, "mins") == 0 || strcmp(unit, "minute") == 0 ||
               strcmp(unit, "minutes") == 0) {
      mul = 60ULL * 1000ULL;
    } else if (strcmp(unit, "h") == 0 || strcmp(unit, "hr") == 0 ||
               strcmp(unit, "hrs") == 0 || strcmp(unit, "hour") == 0 ||
               strcmp(unit, "hours") == 0) {
      mul = 60ULL * 60ULL * 1000ULL;
    } else if (strcmp(unit, "d") == 0 || strcmp(unit, "day") == 0 ||
               strcmp(unit, "days") == 0) {
      mul = 24ULL * 60ULL * 60ULL * 1000ULL;
    } else if (strcmp(unit, "w") == 0 || strcmp(unit, "wk") == 0 ||
               strcmp(unit, "week") == 0 || strcmp(unit, "weeks") == 0) {
      mul = 7ULL * 24ULL * 60ULL * 60ULL * 1000ULL;
    } else {
      err = "PARSEMS: bad unit";
      bad = 1;
      break;
    }
    /* bare unit-less number only allowed as sole segment (whole string ms) */
    if (unit[0] == 0 && (segs > 0 || *p != 0)) {
      err = "PARSEMS: bare number only alone";
      bad = 1;
      break;
    }
    if (mul > 1 && n > (unsigned long long)LONG_MAX / mul)
      add = (unsigned long long)LONG_MAX;
    else
      add = n * mul;
    if (total > (unsigned long long)LONG_MAX - add)
      total = (unsigned long long)LONG_MAX;
    else
      total += add;
    segs++;
  }
  if (!bad && segs == 0) {
    err = "PARSEMS: empty";
    bad = 1;
  }
  if (bad) {
    if (err_out) *err_out = err ? err : "PARSEMS: fail";
    return -1;
  }
  if (total > (unsigned long long)LONG_MAX) total = (unsigned long long)LONG_MAX;
  if (ms_out) {
    if (sign < 0) {
      /* clamp -LONG_MAX..0 (avoid LONG_MIN edge) */
      if (total >= (unsigned long long)LONG_MAX)
        *ms_out = -LONG_MAX;
      else
        *ms_out = -(long)total;
    } else {
      *ms_out = (long)total;
    }
  }
  if (segs_out) *segs_out = segs;
  return 0;
}

/* Optional timeout after path (WAITFILE/WAITGONE/WAITMATCH/…):
 * number expr = ms; string / LAST / str-var = PARSEMS duration ("2s","250ms").
 * Leaves *timeout_ms unchanged when no arg present.
 * Returns 0 ok; -1 on bad duration (*err_out set). Optional MS/TIMEOUT keyword. */
int cubalc_parse_optional_timeout_ms(VM *vm, Lex *L, long *timeout_ms,
                                            const char **err_out) {
  char src[256];
  int from_dur = 0;
  long t = 0;
  const char *derr = NULL;
  if (err_out) *err_out = NULL;
  if (!timeout_ms || !L) return -1;
  if (kw(&L->cur, "MS") || kw(&L->cur, "MILLIS") || kw(&L->cur, "MILLISECONDS") ||
      kw(&L->cur, "TIMEOUT") || kw(&L->cur, "FOR"))
    lex_next(L);
  src[0] = 0;
  if (L->cur.kind == TK_STR) {
    if (resolve_str_arg(vm, L, src, sizeof src) != 0)
      return 0;
    from_dur = 1;
  } else if (L->cur.kind == TK_IDENT) {
    if (strcmp(L->cur.text, "LAST") == 0) {
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        return 0;
      from_dur = 1;
    } else {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        if (resolve_str_arg(vm, L, src, sizeof src) != 0)
          return 0;
        from_dur = 1;
      } else {
        t = parse_expr(vm, L);
      }
    }
  } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN) {
    t = parse_expr(vm, L);
  } else {
    return 0; /* no timeout argument */
  }
  if (from_dur) {
    if (cubalc_parse_duration_ms(src, &t, NULL, &derr) != 0) {
      if (err_out) *err_out = derr ? derr : "timeout: bad duration";
      return -1;
    }
  }
  if (kw(&L->cur, "MS") || kw(&L->cur, "MILLIS") || kw(&L->cur, "MILLISECONDS") ||
      kw(&L->cur, "TIMEOUT"))
    lex_next(L);
  *timeout_ms = t;
  return 0;
}

/* Parse UTC ISO stamp → Unix epoch (PARSEISO rules). 0 ok, -1 fail. */
int cubalc_parse_iso_epoch(const char *src, long *epoch_out, const char **err_out) {
  const char *p;
  int y = 0, mo = 0, d = 0, H = 0, M = 0, S = 0, nscan;
  long era, yoe, doy, doe, days, epoch;
  if (err_out) *err_out = NULL;
  if (epoch_out) *epoch_out = 0;
  p = src ? src : "";
  while (*p == ' ' || *p == '\t') p++;
  if (!*p) {
    if (err_out) *err_out = "PARSEISO: empty";
    return -1;
  }
  nscan = sscanf(p, "%d-%d-%d", &y, &mo, &d);
  if (nscan != 3 || y < 1970 || y > 9999 || mo < 1 || mo > 12 || d < 1 || d > 31) {
    if (err_out) *err_out = "PARSEISO: bad date";
    return -1;
  }
  {
    const char *t = p;
    while (*t && *t != 'T' && *t != ' ' && *t != 't') t++;
    if (*t == 'T' || *t == 't' || *t == ' ') {
      t++;
      while (*t == ' ' || *t == '\t') t++;
      if (*t) {
        int hs = 0, ms = 0, ss = 0;
        int nt = sscanf(t, "%d:%d:%d", &hs, &ms, &ss);
        if (nt < 2) {
          if (err_out) *err_out = "PARSEISO: bad time";
          return -1;
        }
        if (nt == 2) ss = 0;
        if (hs < 0 || hs > 23 || ms < 0 || ms > 59 || ss < 0 || ss > 60) {
          if (err_out) *err_out = "PARSEISO: bad time";
          return -1;
        }
        H = hs; M = ms; S = ss;
        if (S == 60) S = 59;
      }
    }
  }
  {
    long Y = (long)y;
    long mp = (long)mo;
    long dy = (long)d;
    Y -= mp <= 2;
    era = (Y >= 0 ? Y : Y - 399) / 400;
    yoe = Y - era * 400;
    doy = (153 * (mp + (mp > 2 ? -3 : 9)) + 2) / 5 + dy - 1;
    doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    days = era * 146097 + doe - 719468;
  }
  epoch = days * 86400L + (long)H * 3600L + (long)M * 60L + (long)S;
  if (epoch < 0) epoch = 0;
  if (epoch_out) *epoch_out = epoch;
  return 0;
}

/* Format epoch seconds as UTC ISO "YYYY-MM-DDTHH:MM:SSZ". */
void cubalc_format_epoch_iso(long n, char *iso, size_t cap) {
  time_t t;
  struct tm tm_utc;
  if (!iso || cap < 2) return;
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
  snprintf(iso, cap, "%04d-%02d-%02dT%02d:%02d:%02dZ",
           tm_utc.tm_year + 1900, tm_utc.tm_mon + 1, tm_utc.tm_mday,
           tm_utc.tm_hour, tm_utc.tm_min, tm_utc.tm_sec);
}

/* Recursive find-style walk: list full paths whose basename matches pat.
 * Always recurses into directories (even if name does not match). Depth cap 40. */
void cubalc_sys_walk_rec(const char *dir, const char *pat,
                                char *out, size_t outsz, size_t *olen, long *kept,
                                int depth, long *ndirs, long *nfiles) {
  cubalc_host_result hr, jr, kr;
  const char *p, *start;
  size_t flen;
  if (depth > 40 || !dir || !dir[0] || !out || !olen || !kept) return;
  if (*olen + 1 >= outsz) return;
  if (cubalc_host_listdir(dir, &hr) != 0) return;
  p = hr.str;
  while (*p) {
    start = p;
    while (*p && *p != '\n') p++;
    flen = (size_t)(p - start);
    if (flen > 0) {
      char name[256], full[512];
      size_t take = flen;
      int isdir = 0;
      if (take >= sizeof name) take = sizeof name - 1;
      memcpy(name, start, take);
      name[take] = 0;
      if (!(name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0)))) {
        if (cubalc_host_join(dir, name, &jr) == 0) {
          size_t dlen;
          snprintf(full, sizeof full, "%s", jr.str);
          if (cubalc_host_path_kind(full, &kr) == 0 && kr.code == 2)
            isdir = 1;
          if (cubalc_walk_match(pat, name)) {
            dlen = strlen(full);
            if (*kept > 0 && *olen + 1 < outsz) out[(*olen)++] = '\n';
            if (*olen + dlen < outsz) {
              memcpy(out + *olen, full, dlen);
              *olen += dlen;
            }
            out[*olen] = 0;
            (*kept)++;
            if (isdir) { if (ndirs) (*ndirs)++; }
            else { if (nfiles) (*nfiles)++; }
          }
          if (isdir)
            cubalc_sys_walk_rec(full, pat, out, outsz, olen, kept, depth + 1,
                                ndirs, nfiles);
        }
      }
    }
    if (*p == '\n') p++;
  }
}

