/* CubalC lang — lang_ops_sys_rand.c
 * SYS RAND/PICK/SHUFFLE/DRAWN — sample plane
 * Peel from core_sys residual (B_core_sys). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_sys_rand.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

int cubalc_lang_ops_sys_try_rand(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. SYS already consumed. */
  /* SYS RAND|RANDOM [n] | [lo hi] — uniform integer for agent jitter/sampling.
   * No args → 0..999999; one arg n>0 → 0..n-1; two args → inclusive [lo,hi].
   * Seed: once per process via time^pid (CUBALC_SEED override if set numeric).
   * Distinct from cube-plane RAND (stack/cell). Usability: backoff jitter without shell. */
  if (kw(&L->cur,"RAND") || kw(&L->cur,"RANDOM") || kw(&L->cur,"RND") ||
      kw(&L->cur,"IRAND") || kw(&L->cur,"RANDINT") || kw(&L->cur,"URAND")){
    static int seeded = 0;
    long a = -1, b = -1, out = 0, span;
    int has_a = 0, has_b = 0;
    char buf[40];
    const char *se;
    lex_next(L);
    /* Track presence separately: single negative arg must not look like "no args"
     * (old a<0&&b<0 sentinel treated SYS RAND -3 as default 0..999999). */
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
        L->cur.kind==TK_MINUS) {
      a = parse_expr(vm, L);
      has_a = 1;
    }
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
        L->cur.kind==TK_MINUS) {
      b = parse_expr(vm, L);
      has_b = 1;
    }
    if (!seeded) {
      unsigned long s = (unsigned long)time(NULL);
#if !defined(CUBALC_OS_WINDOWS)
      s ^= (unsigned long)getpid() << 16;
#endif
      se = getenv("CUBALC_SEED");
      if (se && se[0]) {
        char *end = 0;
        unsigned long v = strtoul(se, &end, 0);
        if (end && end != se) s = v;
      }
      srand((unsigned)(s & 0xffffffffu));
      seeded = 1;
    }
    if (!has_a) {
      /* default wide range */
      out = (long)(rand() % 1000000);
    } else if (!has_b) {
      /* single arg: 0..a-1; non-positive → 0 */
      if (a <= 0) out = 0;
      else out = (long)(rand() % (int)(a > 0x7fffffffL ? 0x7fffffffL : a));
    } else {
      long lo = a, hi = b;
      if (lo > hi) { long t = lo; lo = hi; hi = t; }
      span = hi - lo + 1;
      if (span <= 0) out = lo;
      else if (span > 0x7fffffffL) {
        out = lo + (long)(rand() % 0x7fffffff);
      } else {
        out = lo + (long)(rand() % (int)span);
      }
    }
    vm->last_n = out;
    var_set_num(vm, "LAST_N", out);
    var_set_num(vm, "RAND", out);
    var_set_num(vm, "RANDOM", out);
    var_set_num(vm, "OK", 1);
    snprintf(buf, sizeof buf, "%ld", out);
    var_set_str(vm, "LAST", buf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
    bump(vm); return 1;
  }
  /* SYS PICK|CHOICE|SAMPLE [str|LAST] — random newline field → LAST.
   * LAST_N/PICK_N = 0-based index chosen; empty bag → LAST="", LAST_N=-1, OK=0 soft.
   * Shares RAND seed (CUBALC_SEED). Usability: sample LIST/RANGE bag without shell. */
  if (kw(&L->cur,"PICK") || kw(&L->cur,"CHOICE") || kw(&L->cur,"SAMPLE") ||
      kw(&L->cur,"RANDLINE") || kw(&L->cur,"PICKLINE") || kw(&L->cur,"ANYLINE") ||
      kw(&L->cur,"DRAW") || kw(&L->cur,"LOT")){
    static int pick_seeded = 0;
    char src[CUBALC_HOST_STR_MAX];
    enum { PICK_MAX = 256, PICK_FLEN = 192 };
    char fields[PICK_MAX][PICK_FLEN];
    int n = 0, idx = -1;
    const char *p, *start;
    const char *se;
    lex_next(L);
    src[0] = 0;
    if (resolve_str_arg(vm, L, src, sizeof src) != 0)
      snprintf(src, sizeof src, "%s", vm->last_str);
    if (src[0]) {
      p = src;
      while (*p && n < PICK_MAX) {
        start = p;
        while (*p && *p != '\n') p++;
        if (start == p && *p == 0 && start > src && start[-1] == '\n')
          break;
        {
          size_t flen = (size_t)(p - start);
          if (flen >= PICK_FLEN) flen = PICK_FLEN - 1;
          memcpy(fields[n], start, flen);
          fields[n][flen] = 0;
          n++;
        }
        if (*p == '\n') p++;
      }
    }
    if (!pick_seeded) {
      unsigned long s = (unsigned long)time(NULL);
#if !defined(CUBALC_OS_WINDOWS)
      s ^= (unsigned long)getpid() << 16;
#endif
      se = getenv("CUBALC_SEED");
      if (se && se[0]) {
        char *end = 0;
        unsigned long v = strtoul(se, &end, 0);
        if (end && end != se) s = v;
      }
      srand((unsigned)(s & 0xffffffffu));
      pick_seeded = 1;
    }
    if (n <= 0) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = -1;
      var_set_num(vm, "LAST_N", -1);
      var_set_num(vm, "PICK_N", -1);
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    idx = (int)(rand() % n);
    var_set_str(vm, "LAST", fields[idx]);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", fields[idx]);
    vm->last_n = (long)idx;
    var_set_num(vm, "LAST_N", (long)idx);
    var_set_num(vm, "PICK_N", (long)idx);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS SHUFFLE|SHUF|RANDPERM [str|LAST] — Fisher–Yates shuffle of newline fields → LAST.
   * LAST_N/SHUFFLE_N = field count; empty → "" OK=1. Shares RAND/CUBALC_SEED seed.
   * Usability: randomize peer/work bags without shell shuf; pairs with PICK/TAKE. */
  if (kw(&L->cur,"SHUFFLE") || kw(&L->cur,"SHUF") || kw(&L->cur,"RANDPERM") ||
      kw(&L->cur,"SHUFFLEL") || kw(&L->cur,"SHUFFLELINES") || kw(&L->cur,"SCRAMBLE") ||
      kw(&L->cur,"MIXLINES") || kw(&L->cur,"PERMUTE")){
    static int shuf_seeded = 0;
    char src[CUBALC_HOST_STR_MAX];
    char out[CUBALC_HOST_STR_MAX];
    enum { SHUF_MAX = 256, SHUF_FLEN = 192 };
    char fields[SHUF_MAX][SHUF_FLEN];
    int n = 0, i, j;
    const char *p, *start, *se;
    size_t olen = 0;
    lex_next(L);
    src[0] = 0;
    if (resolve_str_arg(vm, L, src, sizeof src) != 0)
      snprintf(src, sizeof src, "%s", vm->last_str);
    if (src[0]) {
      p = src;
      while (*p && n < SHUF_MAX) {
        start = p;
        while (*p && *p != '\n') p++;
        if (start == p && *p == 0 && start > src && start[-1] == '\n')
          break;
        {
          size_t flen = (size_t)(p - start);
          if (flen >= SHUF_FLEN) flen = SHUF_FLEN - 1;
          memcpy(fields[n], start, flen);
          fields[n][flen] = 0;
          n++;
        }
        if (*p == '\n') p++;
      }
    }
    if (!shuf_seeded) {
      unsigned long s = (unsigned long)time(NULL);
#if !defined(CUBALC_OS_WINDOWS)
      s ^= (unsigned long)getpid() << 16;
#endif
      se = getenv("CUBALC_SEED");
      if (se && se[0]) {
        char *end = 0;
        unsigned long v = strtoul(se, &end, 0);
        if (end && end != se) s = v;
      }
      srand((unsigned)(s & 0xffffffffu));
      shuf_seeded = 1;
    }
    /* Fisher–Yates */
    for (i = n - 1; i > 0; i--) {
      j = (int)(rand() % (i + 1));
      if (j != i) {
        char tmp[SHUF_FLEN];
        memcpy(tmp, fields[i], SHUF_FLEN);
        memcpy(fields[i], fields[j], SHUF_FLEN);
        memcpy(fields[j], tmp, SHUF_FLEN);
      }
    }
    out[0] = 0;
    for (i = 0; i < n; i++) {
      size_t flen = strlen(fields[i]);
      if (i > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
      if (olen + flen < sizeof out) {
        memcpy(out + olen, fields[i], flen);
        olen += flen;
      } else if (olen < sizeof out - 1) {
        size_t take = sizeof out - 1 - olen;
        memcpy(out + olen, fields[i], take);
        olen += take;
      }
      out[olen] = 0;
    }
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = (long)n;
    var_set_num(vm, "LAST_N", (long)n);
    var_set_num(vm, "SHUFFLE_N", (long)n);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS DRAWN|SAMPLEK|NPICK|TAKERAND k [bag] — sample k unique bag fields without
   * replacement → LAST bag. Partial Fisher–Yates; k<=0 → empty; k>=n → all shuffled.
   * LAST_N/DRAWN_N = count returned. Distinct from PICK/SAMPLE (single field).
   * Shares RAND/CUBALC_SEED. Usability: multi-peer/work sample without SHUFFLE+TAKE. */
  if (kw(&L->cur,"DRAWN") || kw(&L->cur,"SAMPLEK") || kw(&L->cur,"NPICK") ||
      kw(&L->cur,"TAKERAND") || kw(&L->cur,"RSAMPLE") || kw(&L->cur,"DRAWK") ||
      kw(&L->cur,"SAMPLE_N") || kw(&L->cur,"PICKN") || kw(&L->cur,"CHOICEN") ||
      kw(&L->cur,"RANDTAKE")){
    static int drawn_seeded = 0;
    char src[CUBALC_HOST_STR_MAX];
    char out[CUBALC_HOST_STR_MAX];
    enum { DRAW_MAX = 256, DRAW_FLEN = 192 };
    char fields[DRAW_MAX][DRAW_FLEN];
    int n = 0, i, j, k = 0, take;
    const char *p, *start, *se;
    size_t olen = 0;
    long kwant = 0;
    lex_next(L);
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
        L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
      kwant = parse_expr(vm, L);
    if (kwant < 0) kwant = 0;
    if (kwant > DRAW_MAX) kwant = DRAW_MAX;
    k = (int)kwant;
    src[0] = 0; out[0] = 0;
    if (resolve_str_arg(vm, L, src, sizeof src) != 0)
      snprintf(src, sizeof src, "%s", vm->last_str);
    if (src[0]) {
      p = src;
      while (*p && n < DRAW_MAX) {
        start = p;
        while (*p && *p != '\n') p++;
        if (start == p && *p == 0 && start > src && start[-1] == '\n')
          break;
        {
          size_t flen = (size_t)(p - start);
          if (flen >= DRAW_FLEN) flen = DRAW_FLEN - 1;
          memcpy(fields[n], start, flen);
          fields[n][flen] = 0;
          n++;
        }
        if (*p == '\n') p++;
      }
    }
    if (!drawn_seeded) {
      unsigned long s = (unsigned long)time(NULL);
#if !defined(CUBALC_OS_WINDOWS)
      s ^= (unsigned long)getpid() << 16;
#endif
      se = getenv("CUBALC_SEED");
      if (se && se[0]) {
        char *end = 0;
        unsigned long v = strtoul(se, &end, 0);
        if (end && end != se) s = v;
      }
      srand((unsigned)(s & 0xffffffffu));
      drawn_seeded = 1;
    }
    take = k;
    if (take > n) take = n;
    /* partial Fisher–Yates: randomize first `take` positions */
    for (i = 0; i < take; i++) {
      int r = i + (int)(rand() % (n - i));
      if (r != i) {
        char tmp[DRAW_FLEN];
        memcpy(tmp, fields[i], DRAW_FLEN);
        memcpy(fields[i], fields[r], DRAW_FLEN);
        memcpy(fields[r], tmp, DRAW_FLEN);
      }
    }
    for (i = 0; i < take; i++) {
      size_t flen = strlen(fields[i]);
      if (i > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
      if (olen + flen < sizeof out) {
        memcpy(out + olen, fields[i], flen);
        olen += flen;
      } else if (olen < sizeof out - 1) {
        size_t t = sizeof out - 1 - olen;
        memcpy(out + olen, fields[i], t);
        olen += t;
      }
      out[olen] = 0;
    }
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = (long)take;
    var_set_num(vm, "LAST_N", (long)take);
    var_set_num(vm, "DRAWN_N", (long)take);
    var_set_num(vm, "SAMPLEK_N", (long)take);
    var_set_num(vm, "NPICK_N", (long)take);
    var_set_num(vm, "DRAWN_TOTAL", (long)n);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  return 0;
}
