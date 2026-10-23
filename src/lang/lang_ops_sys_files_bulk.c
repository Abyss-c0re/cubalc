/* CubalC lang — lang_ops_sys_files_bulk.c
 * SYS bulk multi-file: LOGALL · WRITEALL · REPLACEFILES · GREPFILES · GREPLINES
 * Peel from sys_files. Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_sys_files_bulk.h"
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
int cubalc_lang_ops_sys_try_files_bulk(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. SYS already consumed. */
  /* SYS LOGALL|APPENDFILES|BULKAPPEND bag data
   * — append data+\n to every path in bag (fopen "a", create if missing).
   * Not APPENDALL (that is string SUFFIXALL). Soft continue on open fail.
   * LAST = paths written; LAST_N / LOGALL_N = count; LOGALL_BYTES · LOGALL_FAIL.
   * One-arg form: data uses LAST bag (unlikely) — prefer bag data or LAST data.
   * Two-arg: bag data. One-arg data: bag = prior LAST.
   * Usability: multi-log stamp without EACH+APPEND glue. */
  if (kw(&L->cur,"LOGALL") || kw(&L->cur,"APPENDFILES") || kw(&L->cur,"BULKAPPEND") ||
      kw(&L->cur,"APPENDTOALL") || kw(&L->cur,"LOGTOALL") || kw(&L->cur,"MULTALOG") ||
      kw(&L->cur,"FILEAPPENDALL") || kw(&L->cur,"APPENDLNALL")){
    char bag[CUBALC_HOST_STR_MAX], data[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
    char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
    const char *p, *start;
    size_t flen, olen = 0;
    long okc = 0, failc = 0, nscan = 0, bytes = 0;
    long dlen;
    lex_next(L);
    bag[0] = 0; data[0] = 0; out[0] = 0; a[0] = 0; b[0] = 0;
    if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
      fail(vm, "SYS LOGALL [bag] data");
      return -1;
    }
    if (resolve_str_arg(vm, L, b, sizeof b) == 0) {
      snprintf(bag, sizeof bag, "%s", a);
      snprintf(data, sizeof data, "%s", b);
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long n = parse_expr(vm, L);
      snprintf(bag, sizeof bag, "%s", a);
      snprintf(data, sizeof data, "%ld", n);
    } else {
      /* one-arg: data = a, bag = LAST */
      snprintf(bag, sizeof bag, "%s", vm->last_str);
      snprintf(data, sizeof data, "%s", a);
    }
    dlen = (long)strlen(data) + 1; /* +newline */
    p = bag;
    while (*p) {
      start = p;
      while (*p && *p != '\n') p++;
      flen = (size_t)(p - start);
      if (flen > 0) {
        char path[512];
        size_t take = flen;
        FILE *af;
        nscan++;
        if (take >= sizeof path) take = sizeof path - 1;
        memcpy(path, start, take);
        path[take] = 0;
        af = fopen(path, "a");
        if (!af) {
          failc++;
          if (failc == 1) {
            var_set_str(vm, "LAST_ERR", "LOGALL: open fail");
            var_set_str(vm, "ERR", "LOGALL: open fail");
          }
        } else {
          fputs(data, af);
          fputc('\n', af);
          fclose(af);
          if (okc > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + take < sizeof out) {
            memcpy(out + olen, path, take);
            olen += take;
          }
          out[olen] = 0;
          okc++;
          bytes += dlen;
        }
      }
      if (*p == '\n') p++;
    }
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = okc;
    var_set_num(vm, "LAST_N", okc);
    var_set_str(vm, "LOGALL", out);
    var_set_str(vm, "APPENDFILES", out);
    var_set_num(vm, "LOGALL_N", okc);
    var_set_num(vm, "APPENDFILES_N", okc);
    var_set_num(vm, "LOGALL_BYTES", bytes);
    var_set_num(vm, "LOGALL_FAIL", failc);
    var_set_num(vm, "LOGALL_SCAN", nscan);
    var_set_num(vm, "OK", failc == 0 ? 1 : 0);
    bump(vm); return 1;
  }
  /* SYS WRITEALL|WRITEFILES|BULKWRITE bag data
   * — overwrite every path in bag with data (fopen "w", create if missing).
   * Dual of LOGALL (append+\n). Soft continue on open/write fail.
   * LAST = paths written; LAST_N / WRITEALL_N; WRITEALL_BYTES · WRITEALL_FAIL.
   * Two-arg: bag data. One-arg data: bag = prior LAST. Numeric data ok.
   * Usability: multi-file plate reset/stamp without EACH+WRITE glue. */
  if (kw(&L->cur,"WRITEALL") || kw(&L->cur,"WRITEFILES") || kw(&L->cur,"BULKWRITE") ||
      kw(&L->cur,"SPLATALL") || kw(&L->cur,"OVERWRITEALL") || kw(&L->cur,"FILEWRITEALL") ||
      kw(&L->cur,"PUTALL") || kw(&L->cur,"SETCONTENTALL") || kw(&L->cur,"WRITEALLFILES")){
    char bag[CUBALC_HOST_STR_MAX], data[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
    char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
    const char *p, *start;
    size_t flen, olen = 0;
    long okc = 0, failc = 0, nscan = 0, bytes = 0;
    long dlen;
    lex_next(L);
    bag[0] = 0; data[0] = 0; out[0] = 0; a[0] = 0; b[0] = 0;
    if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
      fail(vm, "SYS WRITEALL [bag] data");
      return -1;
    }
    if (resolve_str_arg(vm, L, b, sizeof b) == 0) {
      snprintf(bag, sizeof bag, "%s", a);
      snprintf(data, sizeof data, "%s", b);
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      long n = parse_expr(vm, L);
      snprintf(bag, sizeof bag, "%s", a);
      snprintf(data, sizeof data, "%ld", n);
    } else {
      /* one-arg: data = a, bag = LAST */
      snprintf(bag, sizeof bag, "%s", vm->last_str);
      snprintf(data, sizeof data, "%s", a);
    }
    dlen = (long)strlen(data);
    p = bag;
    while (*p) {
      start = p;
      while (*p && *p != '\n') p++;
      flen = (size_t)(p - start);
      if (flen > 0) {
        char path[512];
        size_t take = flen;
        cubalc_host_result hr;
        nscan++;
        if (take >= sizeof path) take = sizeof path - 1;
        memcpy(path, start, take);
        path[take] = 0;
        if (cubalc_host_write(path, data, &hr) != 0) {
          failc++;
          if (failc == 1) {
            const char *em = hr.err[0] ? hr.err : "WRITEALL: write fail";
            var_set_str(vm, "LAST_ERR", em);
            var_set_str(vm, "ERR", em);
          }
        } else {
          if (okc > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + take < sizeof out) {
            memcpy(out + olen, path, take);
            olen += take;
          }
          out[olen] = 0;
          okc++;
          bytes += dlen;
        }
      }
      if (*p == '\n') p++;
    }
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = okc;
    var_set_num(vm, "LAST_N", okc);
    var_set_str(vm, "WRITEALL", out);
    var_set_str(vm, "WRITEFILES", out);
    var_set_num(vm, "WRITEALL_N", okc);
    var_set_num(vm, "WRITEFILES_N", okc);
    var_set_num(vm, "WRITEALL_BYTES", bytes);
    var_set_num(vm, "WRITEALL_FAIL", failc);
    var_set_num(vm, "WRITEALL_SCAN", nscan);
    var_set_num(vm, "OK", failc == 0 ? 1 : 0);
    bump(vm); return 1;
  }
  /* SYS REPLACEFILES|SUBFILES|GSUBFILES bag old new
   * — REPLACEALL old→new inside every path; rewrite only when changed.
   * Soft miss/unreadable skip. Empty old → no-op scan.
   * LAST = paths rewritten; LAST_N / REPLACEFILES_N; REPLACEFILES_SUBS total
   * replacements; REPLACEFILES_MISS · REPLACEFILES_SCAN · REPLACEFILES_SKIP
   * (readable but unchanged).
   * Two-arg after bag: old new. One-arg form not supported (need old+new).
   * Usability: bulk config/plate rewrites without EACH+READ+REPLACEALL+WRITE. */
  if (kw(&L->cur,"REPLACEFILES") || kw(&L->cur,"SUBFILES") || kw(&L->cur,"GSUBFILES") ||
      kw(&L->cur,"FILEGSUB") || kw(&L->cur,"FILESUBST") || kw(&L->cur,"SUBSTFILES") ||
      kw(&L->cur,"SEDALL") || kw(&L->cur,"REWRITES") || kw(&L->cur,"BULKREPLACE")){
    char bag[CUBALC_HOST_STR_MAX], olds[CUBALC_HOST_STR_MAX], news[CUBALC_HOST_STR_MAX];
    char out[CUBALC_HOST_STR_MAX], nbuf[CUBALC_HOST_STR_MAX];
    char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX], c[CUBALC_HOST_STR_MAX];
    const char *p, *start, *src;
    size_t flen, olen = 0, oldn, newn, pre, rest;
    long okc = 0, nscan = 0, nmiss = 0, nskip = 0, nsubs = 0;
    lex_next(L);
    bag[0] = 0; olds[0] = 0; news[0] = 0; out[0] = 0; a[0] = 0; b[0] = 0; c[0] = 0;
    if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
      fail(vm, "SYS REPLACEFILES bag old new");
      return -1;
    }
    if (resolve_str_arg(vm, L, b, sizeof b) != 0) {
      fail(vm, "SYS REPLACEFILES bag old new");
      return -1;
    }
    if (resolve_str_arg(vm, L, c, sizeof c) == 0) {
      snprintf(bag, sizeof bag, "%s", a);
      snprintf(olds, sizeof olds, "%s", b);
      snprintf(news, sizeof news, "%s", c);
    } else {
      /* two-arg: bag=LAST, old=a, new=b */
      snprintf(bag, sizeof bag, "%s", vm->last_str);
      snprintf(olds, sizeof olds, "%s", a);
      snprintf(news, sizeof news, "%s", b);
    }
    oldn = strlen(olds);
    newn = strlen(news);
    p = bag;
    while (*p) {
      start = p;
      while (*p && *p != '\n') p++;
      flen = (size_t)(p - start);
      if (flen > 0) {
        char path[512];
        size_t take = flen, fo;
        cubalc_host_result hr, wr;
        long file_subs = 0;
        nscan++;
        if (take >= sizeof path) take = sizeof path - 1;
        memcpy(path, start, take);
        path[take] = 0;
        if (cubalc_host_read(path, &hr) != 0) {
          nmiss++;
          if (nmiss == 1) {
            var_set_str(vm, "LAST_ERR", "REPLACEFILES: read fail");
            var_set_str(vm, "ERR", "REPLACEFILES: read fail");
          }
        } else if (oldn == 0) {
          nskip++;
        } else {
          /* REPLACEALL into nbuf */
          fo = 0;
          nbuf[0] = 0;
          src = hr.str;
          for (;;) {
            const char *hit = strstr(src, olds);
            if (!hit) {
              rest = strlen(src);
              if (fo + rest >= sizeof nbuf) rest = sizeof nbuf - 1 - fo;
              memcpy(nbuf + fo, src, rest);
              fo += rest;
              nbuf[fo] = 0;
              break;
            }
            pre = (size_t)(hit - src);
            if (fo + pre >= sizeof nbuf) pre = sizeof nbuf - 1 - fo;
            memcpy(nbuf + fo, src, pre);
            fo += pre;
            if (fo + newn < sizeof nbuf) {
              memcpy(nbuf + fo, news, newn);
              fo += newn;
            } else if (fo < sizeof nbuf - 1) {
              size_t nt = sizeof nbuf - 1 - fo;
              memcpy(nbuf + fo, news, nt);
              fo += nt;
            }
            nbuf[fo] = 0;
            file_subs++;
            src = hit + oldn;
            if (fo >= sizeof nbuf - 1) break;
          }
          if (file_subs == 0) {
            nskip++;
          } else if (cubalc_host_write(path, nbuf, &wr) != 0) {
            nmiss++;
            if (nmiss == 1) {
              const char *em = wr.err[0] ? wr.err : "REPLACEFILES: write fail";
              var_set_str(vm, "LAST_ERR", em);
              var_set_str(vm, "ERR", em);
            }
          } else {
            if (okc > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + take < sizeof out) {
              memcpy(out + olen, path, take);
              olen += take;
            }
            out[olen] = 0;
            okc++;
            nsubs += file_subs;
          }
        }
      }
      if (*p == '\n') p++;
    }
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = okc;
    var_set_num(vm, "LAST_N", okc);
    var_set_str(vm, "REPLACEFILES", out);
    var_set_str(vm, "SUBFILES", out);
    var_set_num(vm, "REPLACEFILES_N", okc);
    var_set_num(vm, "SUBFILES_N", okc);
    var_set_num(vm, "REPLACEFILES_SUBS", nsubs);
    var_set_num(vm, "REPLACEFILES_SCAN", nscan);
    var_set_num(vm, "REPLACEFILES_MISS", nmiss);
    var_set_num(vm, "REPLACEFILES_SKIP", nskip);
    var_set_num(vm, "OK", nmiss == 0 ? 1 : 0);
    bump(vm); return 1;
  }
  /* SYS GREPFILES|SEARCHFILES|FILESGREP bag needle
   * — keep paths whose file content contains needle (soft miss skip).
   * GREPFILESI case-insensitive; GREPVFILES invert (no needle in content).
   * Not string GREP (that filters bag fields). LAST = hit paths bag.
   * LAST_N / GREPFILES_N · GREPFILES_SCAN · GREPFILES_MISS · GREPFILES_HIT.
   * One-arg: needle uses LAST bag. Empty needle → keep all readable.
   * Usability: content search over PATHGLOB/WALK without EACH+READ+HAS. */
  if (kw(&L->cur,"GREPFILES") || kw(&L->cur,"SEARCHFILES") || kw(&L->cur,"FILESGREP") ||
      kw(&L->cur,"CONTENTGREP") || kw(&L->cur,"KEEPCONTENT") || kw(&L->cur,"FILECONTAINS") ||
      kw(&L->cur,"GREPFILESI") || kw(&L->cur,"SEARCHFILESI") || kw(&L->cur,"FILESGREPI") ||
      kw(&L->cur,"CONTENTGREPI") || kw(&L->cur,"GREPVFILES") || kw(&L->cur,"VGREPFILES") ||
      kw(&L->cur,"DROPCONTENT") || kw(&L->cur,"GREPFILESV") || kw(&L->cur,"GREPVFILESI") ||
      kw(&L->cur,"VGREPFILESI") || kw(&L->cur,"SEARCHFILESV") || kw(&L->cur,"FILESCONTAIN")){
    char bag[CUBALC_HOST_STR_MAX], needle[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
    char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
    const char *p, *start;
    size_t flen, olen = 0, nlen;
    long kept = 0, nscan = 0, nmiss = 0, nhit = 0;
    int icase = 0, invert = 0;
    if (kw(&L->cur,"GREPFILESI") || kw(&L->cur,"SEARCHFILESI") || kw(&L->cur,"FILESGREPI") ||
        kw(&L->cur,"CONTENTGREPI") || kw(&L->cur,"GREPVFILESI") || kw(&L->cur,"VGREPFILESI"))
      icase = 1;
    if (kw(&L->cur,"GREPVFILES") || kw(&L->cur,"VGREPFILES") || kw(&L->cur,"DROPCONTENT") ||
        kw(&L->cur,"GREPFILESV") || kw(&L->cur,"GREPVFILESI") || kw(&L->cur,"VGREPFILESI") ||
        kw(&L->cur,"SEARCHFILESV"))
      invert = 1;
    lex_next(L);
    bag[0] = 0; needle[0] = 0; out[0] = 0; a[0] = 0; b[0] = 0;
    if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
      fail(vm, "SYS GREPFILES [bag] needle");
      return -1;
    }
    if (resolve_str_arg(vm, L, b, sizeof b) == 0) {
      snprintf(bag, sizeof bag, "%s", a);
      snprintf(needle, sizeof needle, "%s", b);
    } else {
      /* one-arg: needle = a, bag = LAST */
      snprintf(bag, sizeof bag, "%s", vm->last_str);
      snprintf(needle, sizeof needle, "%s", a);
    }
    nlen = strlen(needle);
    p = bag;
    while (*p) {
      start = p;
      while (*p && *p != '\n') p++;
      flen = (size_t)(p - start);
      if (flen > 0) {
        char path[512];
        size_t take = flen;
        cubalc_host_result hr;
        int hit = 0, readable = 0;
        nscan++;
        if (take >= sizeof path) take = sizeof path - 1;
        memcpy(path, start, take);
        path[take] = 0;
        if (cubalc_host_read(path, &hr) != 0) {
          nmiss++;
          /* soft: unreadable never kept (even invert — no content probe) */
          hit = 0;
          readable = 0;
        } else {
          readable = 1;
          if (nlen == 0) {
            hit = 1;
          } else if (!icase) {
            hit = (strstr(hr.str, needle) != NULL) ? 1 : 0;
          } else {
            size_t fi;
            hit = 0;
            for (fi = 0; hr.str[fi] && !hit; fi++) {
              size_t j;
              for (j = 0; j < nlen; j++) {
                char ca = hr.str[fi + j], cb = needle[j];
                if (!ca) break;
                if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
                if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
                if (ca != cb) break;
              }
              if (j == nlen) hit = 1;
            }
          }
          if (invert) hit = !hit;
        }
        (void)readable;
        if (hit) {
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + take < sizeof out) {
            memcpy(out + olen, path, take);
            olen += take;
          }
          out[olen] = 0;
          kept++;
          nhit++;
        }
      }
      if (*p == '\n') p++;
    }
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = kept;
    var_set_num(vm, "LAST_N", kept);
    var_set_str(vm, "GREPFILES", out);
    var_set_str(vm, "SEARCHFILES", out);
    var_set_num(vm, "GREPFILES_N", kept);
    var_set_num(vm, "SEARCHFILES_N", kept);
    var_set_num(vm, "GREPFILES_HIT", nhit);
    var_set_num(vm, "GREPFILES_SCAN", nscan);
    var_set_num(vm, "GREPFILES_MISS", nmiss);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS COUNTINFILES|GREPCOUNTFILES|FILECOUNT bag needle
   * — count total occurrences of needle across path bag contents.
   * COUNTINFILESI case-insensitive. Empty needle → 0 (not every byte).
   * LAST = decimal total; LAST_N / COUNTINFILES_N = total hits;
   * COUNTINFILES_FILES = files with ≥1 hit; COUNTINFILES_SCAN · MISS.
   * One-arg: needle, bag = LAST. Complements GREPFILES (paths vs tally).
   * Usability: multi-file error/severity rollup without EACH+READ+COUNTMATCH. */
  if (kw(&L->cur,"COUNTINFILES") || kw(&L->cur,"GREPCOUNTFILES") || kw(&L->cur,"FILECOUNT") ||
      kw(&L->cur,"COUNTFILES") || kw(&L->cur,"OCCURFILES") || kw(&L->cur,"TOTALINFILES") ||
      kw(&L->cur,"COUNTINFILESI") || kw(&L->cur,"GREPCOUNTFILESI") || kw(&L->cur,"FILECOUNTI") ||
      kw(&L->cur,"COUNTFILESI") || kw(&L->cur,"OCCURFILESI")){
    char bag[CUBALC_HOST_STR_MAX], needle[CUBALC_HOST_STR_MAX];
    char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
    const char *p, *start, *src;
    size_t flen, nlen;
    long total = 0, nscan = 0, nmiss = 0, nfiles = 0;
    int icase = 0;
    if (kw(&L->cur,"COUNTINFILESI") || kw(&L->cur,"GREPCOUNTFILESI") ||
        kw(&L->cur,"FILECOUNTI") || kw(&L->cur,"COUNTFILESI") || kw(&L->cur,"OCCURFILESI"))
      icase = 1;
    lex_next(L);
    bag[0] = 0; needle[0] = 0; a[0] = 0; b[0] = 0;
    if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
      fail(vm, "SYS COUNTINFILES [bag] needle");
      return -1;
    }
    if (resolve_str_arg(vm, L, b, sizeof b) == 0) {
      snprintf(bag, sizeof bag, "%s", a);
      snprintf(needle, sizeof needle, "%s", b);
    } else {
      snprintf(bag, sizeof bag, "%s", vm->last_str);
      snprintf(needle, sizeof needle, "%s", a);
    }
    nlen = strlen(needle);
    p = bag;
    while (*p) {
      start = p;
      while (*p && *p != '\n') p++;
      flen = (size_t)(p - start);
      if (flen > 0) {
        char path[512];
        size_t take = flen;
        cubalc_host_result hr;
        long file_hits = 0;
        nscan++;
        if (take >= sizeof path) take = sizeof path - 1;
        memcpy(path, start, take);
        path[take] = 0;
        if (cubalc_host_read(path, &hr) != 0) {
          nmiss++;
        } else if (nlen > 0) {
          src = hr.str;
          if (!icase) {
            for (;;) {
              const char *hit = strstr(src, needle);
              if (!hit) break;
              file_hits++;
              src = hit + nlen;
            }
          } else {
            size_t fi;
            for (fi = 0; hr.str[fi]; ) {
              size_t j;
              int match = 1;
              for (j = 0; j < nlen; j++) {
                char ca = hr.str[fi + j], cb = needle[j];
                if (!ca) { match = 0; break; }
                if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
                if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
                if (ca != cb) { match = 0; break; }
              }
              if (match) {
                file_hits++;
                fi += nlen ? nlen : 1;
              } else {
                fi++;
              }
            }
          }
          if (file_hits > 0) {
            nfiles++;
            total += file_hits;
          }
        }
      }
      if (*p == '\n') p++;
    }
    {
      char tbuf[32];
      snprintf(tbuf, sizeof tbuf, "%ld", total);
      var_set_str(vm, "LAST", tbuf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", tbuf);
    }
    vm->last_n = total;
    var_set_num(vm, "LAST_N", total);
    var_set_num(vm, "COUNTINFILES", total);
    var_set_num(vm, "GREPCOUNTFILES", total);
    var_set_num(vm, "COUNTINFILES_N", total);
    var_set_num(vm, "COUNTINFILES_FILES", nfiles);
    var_set_num(vm, "COUNTINFILES_SCAN", nscan);
    var_set_num(vm, "COUNTINFILES_MISS", nmiss);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SYS FIRSTFILE|HITFILE|GREP1FILE bag needle
   * — first path whose content contains needle → LAST (empty if none).
   * SYS LASTFILE|HITFILEL|GREP1LFILE — last such path.
   * FIRSTFILEI / LASTFILEI case-insensitive. Soft miss skip unreadable.
   * LAST_N 0|1; FIRSTFILE_SCAN · FIRSTFILE_MISS. Empty needle → first readable.
   * Usability: one-hit path pick without GREPFILES+HEAD/TAIL glue. */
  if (kw(&L->cur,"FIRSTFILE") || kw(&L->cur,"HITFILE") || kw(&L->cur,"GREP1FILE") ||
      kw(&L->cur,"FILEHIT1") || kw(&L->cur,"FIRSTHITFILE") || kw(&L->cur,"FINDFILE") ||
      kw(&L->cur,"FIRSTFILEI") || kw(&L->cur,"HITFILEI") || kw(&L->cur,"GREP1FILEI") ||
      kw(&L->cur,"LASTFILE") || kw(&L->cur,"HITFILEL") || kw(&L->cur,"GREP1LFILE") ||
      kw(&L->cur,"LASTHITFILE") || kw(&L->cur,"FILEHITL") || kw(&L->cur,"LASTFILEI") ||
      kw(&L->cur,"HITFILELI") || kw(&L->cur,"GREP1LFILEI")){
    char bag[CUBALC_HOST_STR_MAX], needle[CUBALC_HOST_STR_MAX], found[512];
    char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
    const char *p, *start;
    size_t flen, nlen;
    long nscan = 0, nmiss = 0, hit = 0;
    int icase = 0, want_last = 0;
    if (kw(&L->cur,"LASTFILE") || kw(&L->cur,"HITFILEL") || kw(&L->cur,"GREP1LFILE") ||
        kw(&L->cur,"LASTHITFILE") || kw(&L->cur,"FILEHITL") || kw(&L->cur,"LASTFILEI") ||
        kw(&L->cur,"HITFILELI") || kw(&L->cur,"GREP1LFILEI"))
      want_last = 1;
    if (kw(&L->cur,"FIRSTFILEI") || kw(&L->cur,"HITFILEI") || kw(&L->cur,"GREP1FILEI") ||
        kw(&L->cur,"LASTFILEI") || kw(&L->cur,"HITFILELI") || kw(&L->cur,"GREP1LFILEI"))
      icase = 1;
    lex_next(L);
    bag[0] = 0; needle[0] = 0; found[0] = 0; a[0] = 0; b[0] = 0;
    if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
      fail(vm, want_last ? "SYS LASTFILE [bag] needle" : "SYS FIRSTFILE [bag] needle");
      return -1;
    }
    if (resolve_str_arg(vm, L, b, sizeof b) == 0) {
      snprintf(bag, sizeof bag, "%s", a);
      snprintf(needle, sizeof needle, "%s", b);
    } else {
      snprintf(bag, sizeof bag, "%s", vm->last_str);
      snprintf(needle, sizeof needle, "%s", a);
    }
    nlen = strlen(needle);
    p = bag;
    while (*p) {
      start = p;
      while (*p && *p != '\n') p++;
      flen = (size_t)(p - start);
      if (flen > 0) {
        char path[512];
        size_t take = flen;
        cubalc_host_result hr;
        int match = 0;
        nscan++;
        if (take >= sizeof path) take = sizeof path - 1;
        memcpy(path, start, take);
        path[take] = 0;
        if (cubalc_host_read(path, &hr) != 0) {
          nmiss++;
        } else {
          if (nlen == 0) {
            match = 1;
          } else if (!icase) {
            match = (strstr(hr.str, needle) != NULL) ? 1 : 0;
          } else {
            size_t fi;
            match = 0;
            for (fi = 0; hr.str[fi] && !match; fi++) {
              size_t j;
              for (j = 0; j < nlen; j++) {
                char ca = hr.str[fi + j], cb = needle[j];
                if (!ca) break;
                if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
                if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
                if (ca != cb) break;
              }
              if (j == nlen) match = 1;
            }
          }
          if (match) {
            snprintf(found, sizeof found, "%s", path);
            hit = 1;
            if (!want_last) break; /* first wins */
          }
        }
      }
      if (*p == '\n') p++;
    }
    var_set_str(vm, "LAST", found);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", found);
    vm->last_n = hit;
    var_set_num(vm, "LAST_N", hit);
    if (want_last) {
      var_set_str(vm, "LASTFILE", found);
      var_set_str(vm, "HITFILEL", found);
      var_set_num(vm, "LASTFILE_N", hit);
      var_set_num(vm, "LASTFILE_SCAN", nscan);
      var_set_num(vm, "LASTFILE_MISS", nmiss);
    } else {
      var_set_str(vm, "FIRSTFILE", found);
      var_set_str(vm, "HITFILE", found);
      var_set_num(vm, "FIRSTFILE_N", hit);
      var_set_num(vm, "FIRSTFILE_SCAN", nscan);
      var_set_num(vm, "FIRSTFILE_MISS", nmiss);
    }
    var_set_num(vm, "OK", hit ? 1 : 0);
    bump(vm); return 1;
  }
  /* SYS GREPLINES|EXTRACTLINES|FILEGREPLINES bag needle
   * — collect every line containing needle from every path → LAST bag.
   * GREPLINESI case-insensitive. Soft miss skips unreadable.
   * LAST_N / GREPLINES_N = matching lines; GREPLINES_FILES = files with hits;
   * GREPLINES_SCAN · GREPLINES_MISS. Empty needle → all lines from readable.
   * Complements GREPFILES (paths) / COUNTINFILES (tally) with line extract.
   * Usability: multi-file log line harvest without EACH+READ+GREP+PUSH. */
  if (kw(&L->cur,"GREPLINES") || kw(&L->cur,"EXTRACTLINES") || kw(&L->cur,"FILEGREPLINES") ||
      kw(&L->cur,"GREPCONTENT") || kw(&L->cur,"LINEMATCH") || kw(&L->cur,"HARVESTLINES") ||
      kw(&L->cur,"GREPLINESI") || kw(&L->cur,"EXTRACTLINESI") || kw(&L->cur,"FILEGREPLINESI") ||
      kw(&L->cur,"GREPCONTENTI") || kw(&L->cur,"LINEMATCHI")){
    char bag[CUBALC_HOST_STR_MAX], needle[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
    char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
    const char *p, *start, *lp, *ls;
    size_t flen, olen = 0, nlen, llen;
    long nlines = 0, nscan = 0, nmiss = 0, nfiles = 0;
    int icase = 0;
    if (kw(&L->cur,"GREPLINESI") || kw(&L->cur,"EXTRACTLINESI") ||
        kw(&L->cur,"FILEGREPLINESI") || kw(&L->cur,"GREPCONTENTI") || kw(&L->cur,"LINEMATCHI"))
      icase = 1;
    lex_next(L);
    bag[0] = 0; needle[0] = 0; out[0] = 0; a[0] = 0; b[0] = 0;
    if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
      fail(vm, "SYS GREPLINES [bag] needle");
      return -1;
    }
    if (resolve_str_arg(vm, L, b, sizeof b) == 0) {
      snprintf(bag, sizeof bag, "%s", a);
      snprintf(needle, sizeof needle, "%s", b);
    } else {
      snprintf(bag, sizeof bag, "%s", vm->last_str);
      snprintf(needle, sizeof needle, "%s", a);
    }
    nlen = strlen(needle);
    p = bag;
    while (*p) {
      start = p;
      while (*p && *p != '\n') p++;
      flen = (size_t)(p - start);
      if (flen > 0) {
        char path[512];
        size_t take = flen;
        cubalc_host_result hr;
        long file_hits = 0;
        nscan++;
        if (take >= sizeof path) take = sizeof path - 1;
        memcpy(path, start, take);
        path[take] = 0;
        if (cubalc_host_read(path, &hr) != 0) {
          nmiss++;
        } else {
          lp = hr.str;
          while (*lp) {
            ls = lp;
            while (*lp && *lp != '\n') lp++;
            llen = (size_t)(lp - ls);
            {
              char line[512];
              size_t lt = llen;
              int match = 0;
              if (lt >= sizeof line) lt = sizeof line - 1;
              memcpy(line, ls, lt);
              line[lt] = 0;
              if (nlen == 0) {
                match = 1;
              } else if (!icase) {
                match = (strstr(line, needle) != NULL) ? 1 : 0;
              } else {
                size_t fi;
                match = 0;
                for (fi = 0; line[fi] && !match; fi++) {
                  size_t j;
                  for (j = 0; j < nlen; j++) {
                    char ca = line[fi + j], cb = needle[j];
                    if (!ca) break;
                    if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
                    if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
                    if (ca != cb) break;
                  }
                  if (j == nlen) match = 1;
                }
              }
              if (match) {
                if (nlines > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
                if (olen + lt < sizeof out) {
                  memcpy(out + olen, line, lt);
                  olen += lt;
                } else if (olen < sizeof out - 1) {
                  size_t room = sizeof out - 1 - olen;
                  memcpy(out + olen, line, room);
                  olen += room;
                }
                out[olen] = 0;
                nlines++;
                file_hits++;
              }
            }
            if (*lp == '\n') lp++;
          }
          if (file_hits > 0) nfiles++;
        }
      }
      if (*p == '\n') p++;
    }
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = nlines;
    var_set_num(vm, "LAST_N", nlines);
    var_set_str(vm, "GREPLINES", out);
    var_set_str(vm, "EXTRACTLINES", out);
    var_set_num(vm, "GREPLINES_N", nlines);
    var_set_num(vm, "EXTRACTLINES_N", nlines);
    var_set_num(vm, "GREPLINES_FILES", nfiles);
    var_set_num(vm, "GREPLINES_SCAN", nscan);
    var_set_num(vm, "GREPLINES_MISS", nmiss);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  return 0;
}
