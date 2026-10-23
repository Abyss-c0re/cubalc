/* CubalC lang — lang_ops_sys_files_line.c
 * SYS file line plane: HEADFILE/TAILFILE · LINECOUNT · MIDFILE · LINEAT ·
 *   SET/INSERT/DROP FILELINE · FIND/COUNT FILELINE
 * Peel from sys_files (B_sys_files_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_sys_files_line.h"
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
#if !defined(CUBALC_OS_WINDOWS)
#  include <unistd.h>
#  include <fcntl.h>
#  include <sys/stat.h>
#endif

int cubalc_lang_ops_sys_try_files_line(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. SYS already consumed. */
    if (kw(&L->cur,"HEADFILE") || kw(&L->cur,"FILEHEAD") || kw(&L->cur,"HEADF") ||
        kw(&L->cur,"HEADLINES") || kw(&L->cur,"TOPLINES") || kw(&L->cur,"FILETOP") ||
        kw(&L->cur,"TAILFILE") || kw(&L->cur,"FILETAIL") || kw(&L->cur,"TAILF") ||
        kw(&L->cur,"TAILLINES") || kw(&L->cur,"BOTTOMLINES") || kw(&L->cur,"FILEBOTTOM")){
      char path[512], out[CUBALC_HOST_STR_MAX];
      char a[CUBALC_HOST_STR_MAX];
      const char *lp, *ls;
      size_t olen = 0, llen;
      long nwant = 10, total = 0, kept = 0;
      int want_tail = 0;
      /* ring for tail: store up to 256 lines of up to 256 chars each is heavy;
       * simpler: two-pass for tail, or collect all line starts in one pass on hr.str */
      cubalc_host_result hr;
      if (kw(&L->cur,"TAILFILE") || kw(&L->cur,"FILETAIL") || kw(&L->cur,"TAILF") ||
          kw(&L->cur,"TAILLINES") || kw(&L->cur,"BOTTOMLINES") || kw(&L->cur,"FILEBOTTOM"))
        want_tail = 1;
      lex_next(L);
      path[0] = 0; out[0] = 0; a[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        /* zero-arg: path = LAST, n = 10 */
        snprintf(path, sizeof path, "%s", vm->last_str);
      } else {
        /* if next is num, a is path; if only one arg and looks like number... */
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          snprintf(path, sizeof path, "%s", a);
          nwant = parse_expr(vm, L);
        } else {
          char b[CUBALC_HOST_STR_MAX];
          b[0] = 0;
          if (resolve_str_arg(vm, L, b, sizeof b) == 0) {
            /* path n as string number, or n path? Prefer path then n */
            snprintf(path, sizeof path, "%s", a);
            nwant = atol(b);
          } else {
            /* only one str arg: could be path (n default) */
            snprintf(path, sizeof path, "%s", a);
          }
        }
      }
      if (nwant < 0) nwant = 0;
      if (nwant > 256) nwant = 256;
      if (!path[0] || cubalc_host_read(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, want_tail ? "TAILFILE_N" : "HEADFILE_N", 0);
        var_set_num(vm, want_tail ? "TAILFILE_TOTAL" : "HEADFILE_TOTAL", 0);
        var_set_num(vm, "OK", 0);
        if (!path[0]) {
          var_set_str(vm, "LAST_ERR", want_tail ? "TAILFILE: empty path" : "HEADFILE: empty path");
          var_set_str(vm, "ERR", want_tail ? "TAILFILE: empty path" : "HEADFILE: empty path");
        } else {
          var_set_str(vm, "LAST_ERR", want_tail ? "TAILFILE: read fail" : "HEADFILE: read fail");
          var_set_str(vm, "ERR", want_tail ? "TAILFILE: read fail" : "HEADFILE: read fail");
        }
        bump(vm); return 1;
      }
      /* count total lines + optionally collect */
      {
        /* line start offsets for tail (cap 4096 lines tracked) */
        const char *starts[4096];
        long nstarts = 0;
        lp = hr.str;
        while (*lp && nstarts < 4096) {
          starts[nstarts++] = lp;
          while (*lp && *lp != '\n') lp++;
          if (*lp == '\n') lp++;
        }
        /* if more content after cap, recount remaining for total only */
        total = nstarts;
        while (*lp) {
          if (*lp == '\n') total++;
          /* count lines: if trailing non-empty after last \n */
          lp++;
        }
        /* recount properly: scan full for total */
        total = 0;
        lp = hr.str;
        while (*lp) {
          while (*lp && *lp != '\n') lp++;
          total++;
          if (*lp == '\n') lp++;
        }
        if (want_tail) {
          long start_i = total > nwant ? total - nwant : 0;
          long i;
          /* rebuild starts if total > 4096 — fall back to full scan collect */
          if (total <= 4096 && nstarts == total) {
            for (i = start_i; i < total; i++) {
              ls = starts[i];
              lp = ls;
              while (*lp && *lp != '\n') lp++;
              llen = (size_t)(lp - ls);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + llen < sizeof out) {
                memcpy(out + olen, ls, llen);
                olen += llen;
              } else if (olen < sizeof out - 1) {
                size_t room = sizeof out - 1 - olen;
                memcpy(out + olen, ls, room);
                olen += room;
              }
              out[olen] = 0;
              kept++;
            }
          } else {
            /* full scan: skip first (total-nwant) lines */
            long skip = total > nwant ? total - nwant : 0;
            long seen = 0;
            lp = hr.str;
            while (*lp) {
              ls = lp;
              while (*lp && *lp != '\n') lp++;
              llen = (size_t)(lp - ls);
              if (seen >= skip) {
                if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
                if (olen + llen < sizeof out) {
                  memcpy(out + olen, ls, llen);
                  olen += llen;
                }
                out[olen] = 0;
                kept++;
              }
              seen++;
              if (*lp == '\n') lp++;
            }
          }
        } else {
          /* head: first nwant lines */
          lp = hr.str;
          while (*lp && kept < nwant) {
            ls = lp;
            while (*lp && *lp != '\n') lp++;
            llen = (size_t)(lp - ls);
            if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + llen < sizeof out) {
              memcpy(out + olen, ls, llen);
              olen += llen;
            } else if (olen < sizeof out - 1) {
              size_t room = sizeof out - 1 - olen;
              memcpy(out + olen, ls, room);
              olen += room;
            }
            out[olen] = 0;
            kept++;
            if (*lp == '\n') lp++;
          }
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      if (want_tail) {
        var_set_str(vm, "TAILFILE", out);
        var_set_str(vm, "FILETAIL", out);
        var_set_num(vm, "TAILFILE_N", kept);
        var_set_num(vm, "TAILFILE_TOTAL", total);
        var_set_num(vm, "TAILFILE_WANT", nwant);
      } else {
        var_set_str(vm, "HEADFILE", out);
        var_set_str(vm, "FILEHEAD", out);
        var_set_num(vm, "HEADFILE_N", kept);
        var_set_num(vm, "HEADFILE_TOTAL", total);
        var_set_num(vm, "HEADFILE_WANT", nwant);
      }
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS LINECOUNTALL|WCALL|MAPLINES [bag]
     * — map each path field to decimal line count (newline segments).
     * Miss/unreadable → "0". LAST = count bag aligned with input;
     * LINECOUNTALL_SUM total lines; LINECOUNTALL_FILES readable hits;
     * LINECOUNTALL_MISS unreadable. Dual of SIZEALL for log inventory.
     * Usability: PATHGLOB line inventory without EACH+READ+LINES glue. */
    if (kw(&L->cur,"LINECOUNTALL") || kw(&L->cur,"WCALL") || kw(&L->cur,"MAPLINES") ||
        kw(&L->cur,"LINESALL") || kw(&L->cur,"WCLINESALL") || kw(&L->cur,"FILELINES") ||
        kw(&L->cur,"COUNTLINESALL") || kw(&L->cur,"MAPLINECOUNT") || kw(&L->cur,"LCALL")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], nbuf[32];
      const char *p, *start, *lp;
      size_t flen, olen = 0;
      long kept = 0, sum = 0, files = 0, miss = 0;
      cubalc_host_result hr;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      p = bag;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen > 0) {
          char path[512];
          size_t take = flen, nlen;
          long lines = 0;
          if (take >= sizeof path) take = sizeof path - 1;
          memcpy(path, start, take);
          path[take] = 0;
          if (cubalc_host_read(path, &hr) != 0) {
            miss++;
            lines = 0;
          } else {
            files++;
            lp = hr.str;
            if (*lp) {
              while (*lp) {
                while (*lp && *lp != '\n') lp++;
                lines++;
                if (*lp == '\n') lp++;
              }
            }
            sum += lines;
          }
          snprintf(nbuf, sizeof nbuf, "%ld", lines);
          nlen = strlen(nbuf);
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + nlen < sizeof out) {
            memcpy(out + olen, nbuf, nlen);
            olen += nlen;
          }
          out[olen] = 0;
          kept++;
        }
        if (*p == '\n') p++;
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "LINECOUNTALL", out);
      var_set_str(vm, "WCALL", out);
      var_set_str(vm, "MAPLINES", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "LINECOUNTALL_N", kept);
      var_set_num(vm, "WCALL_N", kept);
      var_set_num(vm, "LINECOUNTALL_SUM", sum);
      var_set_num(vm, "LINECOUNTALL_FILES", files);
      var_set_num(vm, "LINECOUNTALL_MISS", miss);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MIDFILE|FILEMID|LINESLICE path start [count]
     * — extract file lines [start .. start+count) 0-based → LAST bag.
     * count omitted or <0 → rest of file. Cap 256 lines. Soft miss empty.
     * MIDFILE_N · MIDFILE_TOTAL · MIDFILE_START · MIDFILE_WANT.
     * Completes HEADFILE/TAILFILE for arbitrary windows.
     * Usability: page plate/log slices without READ+DROP+TAKE glue. */
    if (kw(&L->cur,"MIDFILE") || kw(&L->cur,"FILEMID") || kw(&L->cur,"LINESLICE") ||
        kw(&L->cur,"SLICEFILE") || kw(&L->cur,"FILESLICE") || kw(&L->cur,"MIDLINESF") ||
        kw(&L->cur,"LINERANGE") || kw(&L->cur,"FILEWINDOW") || kw(&L->cur,"WINDOWFILE")){
      char path[512], out[CUBALC_HOST_STR_MAX], a[CUBALC_HOST_STR_MAX];
      const char *lp, *ls;
      size_t olen = 0, llen;
      long start_i = 0, count = -1, total = 0, kept = 0, seen = 0;
      cubalc_host_result hr;
      lex_next(L);
      path[0] = 0; out[0] = 0; a[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS MIDFILE path start [count]");
        return -1;
      }
      snprintf(path, sizeof path, "%s", a);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        start_i = parse_expr(vm, L);
      } else {
        char b[CUBALC_HOST_STR_MAX];
        b[0] = 0;
        if (resolve_str_arg(vm, L, b, sizeof b) == 0)
          start_i = atol(b);
        else
          start_i = 0;
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        count = parse_expr(vm, L);
      } else {
        char c[CUBALC_HOST_STR_MAX];
        c[0] = 0;
        if (resolve_str_arg(vm, L, c, sizeof c) == 0)
          count = atol(c);
      }
      if (start_i < 0) start_i = 0;
      if (count < 0) count = 256; /* rest, but cap collect */
      if (count > 256) count = 256;
      if (!path[0] || cubalc_host_read(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "MIDFILE_N", 0);
        var_set_num(vm, "MIDFILE_TOTAL", 0);
        var_set_num(vm, "MIDFILE_START", start_i);
        var_set_num(vm, "MIDFILE_WANT", count);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", path[0] ? "MIDFILE: read fail" : "MIDFILE: empty path");
        var_set_str(vm, "ERR", path[0] ? "MIDFILE: read fail" : "MIDFILE: empty path");
        bump(vm); return 1;
      }
      /* total lines */
      lp = hr.str;
      total = 0;
      if (*lp) {
        while (*lp) {
          while (*lp && *lp != '\n') lp++;
          total++;
          if (*lp == '\n') lp++;
        }
      }
      lp = hr.str;
      seen = 0;
      while (*lp && kept < count) {
        ls = lp;
        while (*lp && *lp != '\n') lp++;
        llen = (size_t)(lp - ls);
        if (seen >= start_i) {
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + llen < sizeof out) {
            memcpy(out + olen, ls, llen);
            olen += llen;
          } else if (olen < sizeof out - 1) {
            size_t room = sizeof out - 1 - olen;
            memcpy(out + olen, ls, room);
            olen += room;
          }
          out[olen] = 0;
          kept++;
        }
        seen++;
        if (*lp == '\n') lp++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_str(vm, "MIDFILE", out);
      var_set_str(vm, "FILEMID", out);
      var_set_str(vm, "LINESLICE", out);
      var_set_num(vm, "MIDFILE_N", kept);
      var_set_num(vm, "MIDFILE_TOTAL", total);
      var_set_num(vm, "MIDFILE_START", start_i);
      var_set_num(vm, "MIDFILE_WANT", count);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS LINEAT|FILELINE|ATLINE path n
     * — extract single 0-based file line → LAST (plain string, not bag).
     * Soft miss / OOB → empty LAST · OK=0 on read fail · OK=1 on OOB empty.
     * LINEAT_N 0|1 · LINEAT_I · LINEAT_TOTAL.
     * Completes HEADFILE/TAILFILE/MIDFILE unit case without MIDFILE+HEAD.
     * Usability: fetch config/log line by index (err_line, plate row). */
    if (kw(&L->cur,"LINEAT") || kw(&L->cur,"FILELINE") || kw(&L->cur,"ATLINE") ||
        kw(&L->cur,"NTHFILELINE") || kw(&L->cur,"LINEBYINDEX") || kw(&L->cur,"FILEGETLINE") ||
        kw(&L->cur,"GETFILELINE") || kw(&L->cur,"FILEAT") || kw(&L->cur,"ATFILELINE")){
      char path[512], out[CUBALC_HOST_STR_MAX], a[CUBALC_HOST_STR_MAX];
      const char *lp, *ls;
      size_t llen;
      long idx = 0, total = 0, seen = 0, found = 0;
      cubalc_host_result hr;
      lex_next(L);
      path[0] = 0; out[0] = 0; a[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS LINEAT path n");
        return -1;
      }
      snprintf(path, sizeof path, "%s", a);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        idx = parse_expr(vm, L);
      } else {
        char b[CUBALC_HOST_STR_MAX];
        b[0] = 0;
        if (resolve_str_arg(vm, L, b, sizeof b) == 0)
          idx = atol(b);
        else
          idx = 0;
      }
      if (idx < 0) idx = 0;
      if (!path[0] || cubalc_host_read(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "LINEAT_N", 0);
        var_set_num(vm, "LINEAT_I", idx);
        var_set_num(vm, "LINEAT_TOTAL", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", path[0] ? "LINEAT: read fail" : "LINEAT: empty path");
        var_set_str(vm, "ERR", path[0] ? "LINEAT: read fail" : "LINEAT: empty path");
        bump(vm); return 1;
      }
      /* total lines */
      lp = hr.str;
      total = 0;
      if (*lp) {
        while (*lp) {
          while (*lp && *lp != '\n') lp++;
          total++;
          if (*lp == '\n') lp++;
        }
      }
      lp = hr.str;
      seen = 0;
      while (*lp) {
        ls = lp;
        while (*lp && *lp != '\n') lp++;
        llen = (size_t)(lp - ls);
        if (seen == idx) {
          if (llen >= sizeof out) llen = sizeof out - 1;
          memcpy(out, ls, llen);
          out[llen] = 0;
          found = 1;
          break;
        }
        seen++;
        if (*lp == '\n') lp++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      var_set_str(vm, "LINEAT", out);
      var_set_str(vm, "FILELINE", out);
      var_set_num(vm, "LINEAT_N", found);
      var_set_num(vm, "LINEAT_I", idx);
      var_set_num(vm, "LINEAT_TOTAL", total);
      var_set_num(vm, "OK", 1);
      /* OOB is soft empty, not sticky ERR */
      bump(vm); return 1;
    }
    /* SYS SETFILELINE|FILESETLINE|PUTFILELINE path n value
     * — replace 0-based file line in place (read-modify-write).
     * Dual of LINEAT. Soft OOB → no write, SETFILELINE_N=0, OK=1.
     * Soft miss/write fail → OK=0 sticky LAST_ERR.
     * LAST = new line text when hit; SETFILELINE_I · SETFILELINE_TOTAL.
     * Value is first line only (no embedded newlines). Cap rebuild HOST_STR_MAX.
     * Usability: edit plate/config row without READ+SETLINE+WRITE glue. */
    if (kw(&L->cur,"SETFILELINE") || kw(&L->cur,"FILESETLINE") || kw(&L->cur,"PUTFILELINE") ||
        kw(&L->cur,"SETLINEAT") || kw(&L->cur,"WRITELINEF") || kw(&L->cur,"LINESETF") ||
        kw(&L->cur,"REPLACEFILELINE") || kw(&L->cur,"FILEPUTLINE") || kw(&L->cur,"SETATLINE")){
      char path[512], val[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      char a[CUBALC_HOST_STR_MAX], clean[CUBALC_HOST_STR_MAX];
      const char *lp, *ls, *vp;
      size_t llen, vlen, o = 0;
      long idx = 0, total = 0, seen = 0, hit = 0;
      int first = 1;
      cubalc_host_result hr, wr;
      lex_next(L);
      path[0] = 0; val[0] = 0; out[0] = 0; a[0] = 0; clean[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS SETFILELINE path n value");
        return -1;
      }
      snprintf(path, sizeof path, "%s", a);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        idx = parse_expr(vm, L);
      } else {
        char b[CUBALC_HOST_STR_MAX];
        b[0] = 0;
        if (resolve_str_arg(vm, L, b, sizeof b) == 0)
          idx = atol(b);
        else
          idx = 0;
      }
      if (resolve_str_arg(vm, L, val, sizeof val) != 0) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long n = parse_expr(vm, L);
          snprintf(val, sizeof val, "%ld", n);
        } else {
          val[0] = 0;
        }
      }
      /* first line only — keep file line discipline */
      vp = val;
      {
        size_t i = 0;
        while (vp[i] && vp[i] != '\n' && vp[i] != '\r' && i + 1 < sizeof clean) {
          clean[i] = vp[i];
          i++;
        }
        clean[i] = 0;
      }
      vlen = strlen(clean);
      if (idx < 0) idx = 0;
      if (!path[0] || cubalc_host_read(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SETFILELINE_N", 0);
        var_set_num(vm, "SETFILELINE_I", idx);
        var_set_num(vm, "SETFILELINE_TOTAL", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", path[0] ? "SETFILELINE: read fail" : "SETFILELINE: empty path");
        var_set_str(vm, "ERR", path[0] ? "SETFILELINE: read fail" : "SETFILELINE: empty path");
        bump(vm); return 1;
      }
      /* count total lines */
      lp = hr.str;
      total = 0;
      if (*lp) {
        while (*lp) {
          while (*lp && *lp != '\n') lp++;
          total++;
          if (*lp == '\n') lp++;
        }
      }
      if (idx >= total) {
        /* soft OOB — leave file unchanged */
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SETFILELINE_N", 0);
        var_set_num(vm, "SETFILELINE_I", idx);
        var_set_num(vm, "SETFILELINE_TOTAL", total);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      /* rebuild with replacement */
      lp = hr.str;
      seen = 0;
      while (*lp || seen < total) {
        if (!*lp && seen >= total) break;
        ls = lp;
        while (*lp && *lp != '\n') lp++;
        llen = (size_t)(lp - ls);
        if (!first && o + 1 < sizeof out) out[o++] = '\n';
        first = 0;
        if (seen == idx) {
          hit = 1;
          if (o + vlen < sizeof out) {
            memcpy(out + o, clean, vlen);
            o += vlen;
          } else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, clean, take);
            o += take;
          }
        } else {
          if (o + llen < sizeof out) {
            memcpy(out + o, ls, llen);
            o += llen;
          } else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, ls, take);
            o += take;
          }
        }
        out[o] = 0;
        seen++;
        if (*lp == '\n') lp++;
        else break;
      }
      if (cubalc_host_write(path, out, &wr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SETFILELINE_N", 0);
        var_set_num(vm, "SETFILELINE_I", idx);
        var_set_num(vm, "SETFILELINE_TOTAL", total);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "SETFILELINE: write fail");
        var_set_str(vm, "ERR", wr.err[0] ? wr.err : "SETFILELINE: write fail");
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", clean);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", clean);
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_str(vm, "SETFILELINE", clean);
      var_set_str(vm, "FILESETLINE", clean);
      var_set_num(vm, "SETFILELINE_N", hit);
      var_set_num(vm, "SETFILELINE_I", idx);
      var_set_num(vm, "SETFILELINE_TOTAL", total);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS INSERTFILELINE|FILEINSERTLINE|INSFILELINE path n value
     * — insert line before 0-based index (read-modify-write).
     * n >= total → append. Empty file / n==0 → front. Soft miss → OK=0.
     * LAST = inserted line; INSERTFILELINE_N=1 · I · TOTAL (after).
     * First line of value only. Completes LINEAT/SETFILELINE edit family.
     * Usability: add plate/config row without READ+INSERTLINE+WRITE glue. */
    if (kw(&L->cur,"INSERTFILELINE") || kw(&L->cur,"FILEINSERTLINE") ||
        kw(&L->cur,"INSFILELINE") || kw(&L->cur,"INSLINEF") || kw(&L->cur,"FILEINSLINE") ||
        kw(&L->cur,"INSERTLINEF") || kw(&L->cur,"ADDLINEF") || kw(&L->cur,"SPLICEFILELINE") ||
        kw(&L->cur,"FILEADDAT")){
      char path[512], val[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      char a[CUBALC_HOST_STR_MAX], clean[CUBALC_HOST_STR_MAX];
      const char *lp, *ls, *vp;
      size_t llen, vlen, o = 0;
      long idx = 0, total = 0, seen = 0, final_i = 0;
      int first = 1, inserted = 0;
      cubalc_host_result hr, wr;
      lex_next(L);
      path[0] = 0; val[0] = 0; out[0] = 0; a[0] = 0; clean[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS INSERTFILELINE path n value");
        return -1;
      }
      snprintf(path, sizeof path, "%s", a);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        idx = parse_expr(vm, L);
      } else {
        char b[CUBALC_HOST_STR_MAX];
        b[0] = 0;
        if (resolve_str_arg(vm, L, b, sizeof b) == 0)
          idx = atol(b);
        else
          idx = 0;
      }
      if (resolve_str_arg(vm, L, val, sizeof val) != 0) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long n = parse_expr(vm, L);
          snprintf(val, sizeof val, "%ld", n);
        } else {
          val[0] = 0;
        }
      }
      vp = val;
      {
        size_t i = 0;
        while (vp[i] && vp[i] != '\n' && vp[i] != '\r' && i + 1 < sizeof clean) {
          clean[i] = vp[i];
          i++;
        }
        clean[i] = 0;
      }
      vlen = strlen(clean);
      if (idx < 0) idx = 0;
      if (!path[0] || cubalc_host_read(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "INSERTFILELINE_N", 0);
        var_set_num(vm, "INSERTFILELINE_I", idx);
        var_set_num(vm, "INSERTFILELINE_TOTAL", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", path[0] ? "INSERTFILELINE: read fail" : "INSERTFILELINE: empty path");
        var_set_str(vm, "ERR", path[0] ? "INSERTFILELINE: read fail" : "INSERTFILELINE: empty path");
        bump(vm); return 1;
      }
      lp = hr.str;
      total = 0;
      if (*lp) {
        while (*lp) {
          while (*lp && *lp != '\n') lp++;
          total++;
          if (*lp == '\n') lp++;
        }
      }
      if (idx > total) idx = total; /* append past end */
      final_i = idx;
      /* empty file → single line */
      if (total == 0) {
        if (vlen < sizeof out) {
          memcpy(out, clean, vlen);
          o = vlen;
        } else {
          size_t take = sizeof out - 1;
          memcpy(out, clean, take);
          o = take;
        }
        out[o] = 0;
        inserted = 1;
        final_i = 0;
      } else {
        lp = hr.str;
        seen = 0;
        while (seen < total) {
          ls = lp;
          while (*lp && *lp != '\n') lp++;
          llen = (size_t)(lp - ls);
          if (seen == idx && !inserted) {
            if (!first && o + 1 < sizeof out) out[o++] = '\n';
            first = 0;
            if (o + vlen < sizeof out) {
              memcpy(out + o, clean, vlen);
              o += vlen;
            } else if (o < sizeof out - 1) {
              size_t take = sizeof out - 1 - o;
              memcpy(out + o, clean, take);
              o += take;
            }
            out[o] = 0;
            inserted = 1;
          }
          if (!first && o + 1 < sizeof out) out[o++] = '\n';
          first = 0;
          if (o + llen < sizeof out) {
            memcpy(out + o, ls, llen);
            o += llen;
          } else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, ls, take);
            o += take;
          }
          out[o] = 0;
          seen++;
          if (*lp == '\n') lp++;
          else break;
        }
        if (!inserted) {
          /* append */
          if (!first && o + 1 < sizeof out) out[o++] = '\n';
          if (o + vlen < sizeof out) {
            memcpy(out + o, clean, vlen);
            o += vlen;
          } else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, clean, take);
            o += take;
          }
          out[o] = 0;
          inserted = 1;
          final_i = total;
        }
      }
      if (cubalc_host_write(path, out, &wr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "INSERTFILELINE_N", 0);
        var_set_num(vm, "INSERTFILELINE_I", final_i);
        var_set_num(vm, "INSERTFILELINE_TOTAL", total);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "INSERTFILELINE: write fail");
        var_set_str(vm, "ERR", wr.err[0] ? wr.err : "INSERTFILELINE: write fail");
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", clean);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", clean);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_str(vm, "INSERTFILELINE", clean);
      var_set_str(vm, "FILEINSERTLINE", clean);
      var_set_num(vm, "INSERTFILELINE_N", 1);
      var_set_num(vm, "INSERTFILELINE_I", final_i);
      var_set_num(vm, "INSERTFILELINE_TOTAL", total + 1);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS DROPFILELINE|FILEDROPLINE|DELFILELINE path n
     * — drop 0-based file line in place (read-modify-write).
     * Dual of INSERTFILELINE / file DROPNTH. Soft OOB → no write, N=0, OK=1.
     * Soft miss/write fail → OK=0 sticky LAST_ERR.
     * LAST = dropped line text when hit; DROPFILELINE_N · I · TOTAL(after).
     * Completes LINEAT/SET/INSERT file-line edit family.
     * Usability: remove plate/config row without READ+DROPNTH+WRITE glue. */
    if (kw(&L->cur,"DROPFILELINE") || kw(&L->cur,"FILEDROPLINE") || kw(&L->cur,"DELFILELINE") ||
        kw(&L->cur,"DELETEFILELINE") || kw(&L->cur,"DROPLINEF") || kw(&L->cur,"FILEDROPAT") ||
        kw(&L->cur,"DROPATFILE") || kw(&L->cur,"OMITFILELINE") || kw(&L->cur,"ERASEFILELINE")){
      char path[512], out[CUBALC_HOST_STR_MAX], dropped[CUBALC_HOST_STR_MAX];
      char a[CUBALC_HOST_STR_MAX];
      const char *lp, *ls;
      size_t llen, o = 0;
      long idx = 0, total = 0, seen = 0, hit = 0;
      int first_kept = 1;
      cubalc_host_result hr, wr;
      lex_next(L);
      path[0] = 0; out[0] = 0; dropped[0] = 0; a[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS DROPFILELINE path n");
        return -1;
      }
      snprintf(path, sizeof path, "%s", a);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
        idx = parse_expr(vm, L);
      } else {
        char b[CUBALC_HOST_STR_MAX];
        b[0] = 0;
        if (resolve_str_arg(vm, L, b, sizeof b) == 0)
          idx = atol(b);
        else
          idx = 0;
      }
      if (idx < 0) idx = 0;
      if (!path[0] || cubalc_host_read(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DROPFILELINE_N", 0);
        var_set_num(vm, "DROPFILELINE_I", idx);
        var_set_num(vm, "DROPFILELINE_TOTAL", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", path[0] ? "DROPFILELINE: read fail" : "DROPFILELINE: empty path");
        var_set_str(vm, "ERR", path[0] ? "DROPFILELINE: read fail" : "DROPFILELINE: empty path");
        bump(vm); return 1;
      }
      lp = hr.str;
      total = 0;
      if (*lp) {
        while (*lp) {
          while (*lp && *lp != '\n') lp++;
          total++;
          if (*lp == '\n') lp++;
        }
      }
      if (idx >= total) {
        /* soft OOB — leave file unchanged */
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DROPFILELINE_N", 0);
        var_set_num(vm, "DROPFILELINE_I", idx);
        var_set_num(vm, "DROPFILELINE_TOTAL", total);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      lp = hr.str;
      seen = 0;
      while (seen < total) {
        ls = lp;
        while (*lp && *lp != '\n') lp++;
        llen = (size_t)(lp - ls);
        if (seen == idx) {
          hit = 1;
          if (llen >= sizeof dropped) llen = sizeof dropped - 1;
          memcpy(dropped, ls, llen);
          dropped[llen] = 0;
          /* skip writing this line */
        } else {
          if (!first_kept && o + 1 < sizeof out) out[o++] = '\n';
          first_kept = 0;
          if (o + llen < sizeof out) {
            memcpy(out + o, ls, llen);
            o += llen;
          } else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, ls, take);
            o += take;
          }
          out[o] = 0;
        }
        seen++;
        if (*lp == '\n') lp++;
        else break;
      }
      if (cubalc_host_write(path, out, &wr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DROPFILELINE_N", 0);
        var_set_num(vm, "DROPFILELINE_I", idx);
        var_set_num(vm, "DROPFILELINE_TOTAL", total);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "DROPFILELINE: write fail");
        var_set_str(vm, "ERR", wr.err[0] ? wr.err : "DROPFILELINE: write fail");
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", dropped);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", dropped);
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_str(vm, "DROPFILELINE", dropped);
      var_set_str(vm, "FILEDROPLINE", dropped);
      var_set_num(vm, "DROPFILELINE_N", hit);
      var_set_num(vm, "DROPFILELINE_I", idx);
      var_set_num(vm, "DROPFILELINE_TOTAL", total > 0 ? total - 1 : 0);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS FINDFILELINE|FILEFINDLINE|LINEINDEXF path needle
     * — 0-based index of first file line containing needle (substring).
     * FINDFILELINEI case-insensitive. LAST = matched line text (empty miss).
     * LAST_N / FINDFILELINE_N = 0|1; FINDFILELINE_I = index or -1;
     * FINDFILELINE_TOTAL lines. Soft miss path → OK=0 I=-1.
     * Empty needle → first line (index 0) if any.
     * Usability: locate then SET/DROP/LINEAT without READ+FINDLINE/FIRSTMATCH glue. */
    if (kw(&L->cur,"FINDFILELINE") || kw(&L->cur,"FILEFINDLINE") || kw(&L->cur,"LINEINDEXF") ||
        kw(&L->cur,"FINDLINEF") || kw(&L->cur,"FILELINEINDEX") || kw(&L->cur,"INDEXFILELINE") ||
        kw(&L->cur,"FINDFILELINEI") || kw(&L->cur,"FILEFINDLINEI") || kw(&L->cur,"LINEINDEXFI") ||
        kw(&L->cur,"FINDLINEFI") || kw(&L->cur,"FILELINEINDEXI")){
      char path[512], needle[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      char a[CUBALC_HOST_STR_MAX];
      const char *lp, *ls;
      size_t llen, nlen;
      long total = 0, seen = 0, found_i = -1, found = 0;
      int icase = 0;
      cubalc_host_result hr;
      if (kw(&L->cur,"FINDFILELINEI") || kw(&L->cur,"FILEFINDLINEI") ||
          kw(&L->cur,"LINEINDEXFI") || kw(&L->cur,"FINDLINEFI") ||
          kw(&L->cur,"FILELINEINDEXI"))
        icase = 1;
      lex_next(L);
      path[0] = 0; needle[0] = 0; out[0] = 0; a[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS FINDFILELINE path needle");
        return -1;
      }
      snprintf(path, sizeof path, "%s", a);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
      nlen = strlen(needle);
      if (!path[0] || cubalc_host_read(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "FINDFILELINE_N", 0);
        var_set_num(vm, "FINDFILELINE_I", -1);
        var_set_num(vm, "FINDFILELINE_TOTAL", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", path[0] ? "FINDFILELINE: read fail" : "FINDFILELINE: empty path");
        var_set_str(vm, "ERR", path[0] ? "FINDFILELINE: read fail" : "FINDFILELINE: empty path");
        bump(vm); return 1;
      }
      lp = hr.str;
      total = 0;
      if (*lp) {
        while (*lp) {
          while (*lp && *lp != '\n') lp++;
          total++;
          if (*lp == '\n') lp++;
        }
      }
      lp = hr.str;
      seen = 0;
      while (*lp) {
        int match = 0;
        ls = lp;
        while (*lp && *lp != '\n') lp++;
        llen = (size_t)(lp - ls);
        if (nlen == 0) {
          match = 1; /* empty needle → first line */
        } else if (llen >= nlen) {
          if (!icase) {
            size_t i;
            for (i = 0; i + nlen <= llen; i++) {
              if (memcmp(ls + i, needle, nlen) == 0) { match = 1; break; }
            }
          } else {
            size_t i, j;
            for (i = 0; i + nlen <= llen; i++) {
              int ok = 1;
              for (j = 0; j < nlen; j++) {
                char a2 = ls[i + j], b2 = needle[j];
                if (a2 >= 'A' && a2 <= 'Z') a2 = (char)(a2 - 'A' + 'a');
                if (b2 >= 'A' && b2 <= 'Z') b2 = (char)(b2 - 'A' + 'a');
                if (a2 != b2) { ok = 0; break; }
              }
              if (ok) { match = 1; break; }
            }
          }
        }
        if (match) {
          size_t take = llen;
          if (take >= sizeof out) take = sizeof out - 1;
          memcpy(out, ls, take);
          out[take] = 0;
          found = 1;
          found_i = seen;
          break;
        }
        seen++;
        if (*lp == '\n') lp++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      var_set_str(vm, "FINDFILELINE", out);
      var_set_str(vm, "FILEFINDLINE", out);
      var_set_num(vm, "FINDFILELINE_N", found);
      var_set_num(vm, "FINDFILELINE_I", found_i);
      var_set_num(vm, "FINDFILELINE_TOTAL", total);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS LASTFINDFILELINE|FILELASTFIND|LINEINDEXFL path needle
     * — 0-based index of *last* file line containing needle (substring).
     * Dual of FINDFILELINE. LASTFINDFILELINEI case-insensitive.
     * LAST = matched line (empty miss); LAST_N 0|1; LASTFINDFILELINE_I or -1;
     * LASTFINDFILELINE_TOTAL. Soft path miss → OK=0 I=-1.
     * Empty needle → last line if any.
     * Usability: latest log/error line without REVL+FINDFILELINE glue. */
    if (kw(&L->cur,"LASTFINDFILELINE") || kw(&L->cur,"FILELASTFIND") ||
        kw(&L->cur,"LINEINDEXFL") || kw(&L->cur,"LASTFINDLINEF") || kw(&L->cur,"FILEFINDLAST") ||
        kw(&L->cur,"FINDFILELINEL") || kw(&L->cur,"GREP1FILEL") ||
        kw(&L->cur,"LASTFINDFILELINEI") || kw(&L->cur,"FILELASTFINDI") ||
        kw(&L->cur,"LINEINDEXFLI") || kw(&L->cur,"LASTFINDLINEFI") ||
        kw(&L->cur,"FILEFINDLASTI") || kw(&L->cur,"FINDFILELINELI")){
      char path[512], needle[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      char a[CUBALC_HOST_STR_MAX];
      const char *lp, *ls;
      size_t llen, nlen;
      long total = 0, seen = 0, found_i = -1, found = 0;
      int icase = 0;
      cubalc_host_result hr;
      if (kw(&L->cur,"LASTFINDFILELINEI") || kw(&L->cur,"FILELASTFINDI") ||
          kw(&L->cur,"LINEINDEXFLI") || kw(&L->cur,"LASTFINDLINEFI") ||
          kw(&L->cur,"FILEFINDLASTI") || kw(&L->cur,"FINDFILELINELI"))
        icase = 1;
      lex_next(L);
      path[0] = 0; needle[0] = 0; out[0] = 0; a[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS LASTFINDFILELINE path needle");
        return -1;
      }
      snprintf(path, sizeof path, "%s", a);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
      nlen = strlen(needle);
      if (!path[0] || cubalc_host_read(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "LASTFINDFILELINE_N", 0);
        var_set_num(vm, "LASTFINDFILELINE_I", -1);
        var_set_num(vm, "LASTFINDFILELINE_TOTAL", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", path[0] ? "LASTFINDFILELINE: read fail" : "LASTFINDFILELINE: empty path");
        var_set_str(vm, "ERR", path[0] ? "LASTFINDFILELINE: read fail" : "LASTFINDFILELINE: empty path");
        bump(vm); return 1;
      }
      lp = hr.str;
      total = 0;
      if (*lp) {
        while (*lp) {
          while (*lp && *lp != '\n') lp++;
          total++;
          if (*lp == '\n') lp++;
        }
      }
      lp = hr.str;
      seen = 0;
      while (*lp) {
        int match = 0;
        ls = lp;
        while (*lp && *lp != '\n') lp++;
        llen = (size_t)(lp - ls);
        if (nlen == 0) {
          match = 1; /* empty → keep updating to last line */
        } else if (llen >= nlen) {
          if (!icase) {
            size_t i;
            for (i = 0; i + nlen <= llen; i++) {
              if (memcmp(ls + i, needle, nlen) == 0) { match = 1; break; }
            }
          } else {
            size_t i, j;
            for (i = 0; i + nlen <= llen; i++) {
              int okm = 1;
              for (j = 0; j < nlen; j++) {
                char a2 = ls[i + j], b2 = needle[j];
                if (a2 >= 'A' && a2 <= 'Z') a2 = (char)(a2 - 'A' + 'a');
                if (b2 >= 'A' && b2 <= 'Z') b2 = (char)(b2 - 'A' + 'a');
                if (a2 != b2) { okm = 0; break; }
              }
              if (okm) { match = 1; break; }
            }
          }
        }
        if (match) {
          size_t take = llen;
          if (take >= sizeof out) take = sizeof out - 1;
          memcpy(out, ls, take);
          out[take] = 0;
          found = 1;
          found_i = seen;
        }
        seen++;
        if (*lp == '\n') lp++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      var_set_str(vm, "LASTFINDFILELINE", out);
      var_set_str(vm, "FILELASTFIND", out);
      var_set_num(vm, "LASTFINDFILELINE_N", found);
      var_set_num(vm, "LASTFINDFILELINE_I", found_i);
      var_set_num(vm, "LASTFINDFILELINE_TOTAL", total);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS COUNTFILELINE|FILECOUNTLINE|COUNTMATCHF path needle
     * — count file lines containing needle (substring) → LAST_N.
     * COUNTFILELINEI case-insensitive. Empty needle → 0 (not every line).
     * LAST = decimal count string; COUNTFILELINE_N · COUNTFILELINE_TOTAL.
     * Soft path miss → OK=0 N=0. Complements FINDFILELINE (locate) / COUNTINFILES (multi-file).
     * Usability: "how many error lines?" without GREPLINES bag rebuild. */
    if (kw(&L->cur,"COUNTFILELINE") || kw(&L->cur,"FILECOUNTLINE") || kw(&L->cur,"COUNTMATCHF") ||
        kw(&L->cur,"GREPCOUNTF") || kw(&L->cur,"COUNTLINESF") || kw(&L->cur,"LINECOUNTMATCH") ||
        kw(&L->cur,"COUNTFILELINEI") || kw(&L->cur,"FILECOUNTLINEI") || kw(&L->cur,"COUNTMATCHFI") ||
        kw(&L->cur,"GREPCOUNTFI") || kw(&L->cur,"COUNTLINESFI")){
      char path[512], needle[CUBALC_HOST_STR_MAX], nbuf[32];
      char a[CUBALC_HOST_STR_MAX];
      const char *lp, *ls;
      size_t llen, nlen;
      long total = 0, hits = 0;
      int icase = 0;
      cubalc_host_result hr;
      if (kw(&L->cur,"COUNTFILELINEI") || kw(&L->cur,"FILECOUNTLINEI") ||
          kw(&L->cur,"COUNTMATCHFI") || kw(&L->cur,"GREPCOUNTFI") ||
          kw(&L->cur,"COUNTLINESFI"))
        icase = 1;
      lex_next(L);
      path[0] = 0; needle[0] = 0; nbuf[0] = 0; a[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS COUNTFILELINE path needle");
        return -1;
      }
      snprintf(path, sizeof path, "%s", a);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        needle[0] = 0;
      nlen = strlen(needle);
      if (!path[0] || cubalc_host_read(path, &hr) != 0) {
        var_set_str(vm, "LAST", "0");
        snprintf(vm->last_str, sizeof vm->last_str, "0");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "COUNTFILELINE_N", 0);
        var_set_num(vm, "COUNTFILELINE_TOTAL", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", path[0] ? "COUNTFILELINE: read fail" : "COUNTFILELINE: empty path");
        var_set_str(vm, "ERR", path[0] ? "COUNTFILELINE: read fail" : "COUNTFILELINE: empty path");
        bump(vm); return 1;
      }
      lp = hr.str;
      total = 0;
      hits = 0;
      if (*lp) {
        while (*lp) {
          int match = 0;
          ls = lp;
          while (*lp && *lp != '\n') lp++;
          llen = (size_t)(lp - ls);
          total++;
          if (nlen > 0 && llen >= nlen) {
            if (!icase) {
              size_t i;
              for (i = 0; i + nlen <= llen; i++) {
                if (memcmp(ls + i, needle, nlen) == 0) { match = 1; break; }
              }
            } else {
              size_t i, j;
              for (i = 0; i + nlen <= llen; i++) {
                int okm = 1;
                for (j = 0; j < nlen; j++) {
                  char a2 = ls[i + j], b2 = needle[j];
                  if (a2 >= 'A' && a2 <= 'Z') a2 = (char)(a2 - 'A' + 'a');
                  if (b2 >= 'A' && b2 <= 'Z') b2 = (char)(b2 - 'A' + 'a');
                  if (a2 != b2) { okm = 0; break; }
                }
                if (okm) { match = 1; break; }
              }
            }
          }
          if (match) hits++;
          if (*lp == '\n') lp++;
        }
      }
      snprintf(nbuf, sizeof nbuf, "%ld", hits);
      var_set_str(vm, "LAST", nbuf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nbuf);
      vm->last_n = hits;
      var_set_num(vm, "LAST_N", hits);
      var_set_str(vm, "COUNTFILELINE", nbuf);
      var_set_str(vm, "FILECOUNTLINE", nbuf);
      var_set_num(vm, "COUNTFILELINE_N", hits);
      var_set_num(vm, "COUNTFILELINE_TOTAL", total);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }

  return 0;
}
