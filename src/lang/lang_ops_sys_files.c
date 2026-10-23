/* CubalC lang — lang_ops_sys_files.c
 * SYS/HOST megablock dispatch (mesh vote A_split_lang_ops_core).
 * Peel of ~33k-line SYS plane from lang_ops_core.
 */
#include "lang/cubalc_lang_ops_sys_files.h"
#include "lang/cubalc_lang_ops_sys_files_bulk.h"
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
#  include <dirent.h>
#  include <sys/stat.h>
#  include <sys/statvfs.h>
#  include <fcntl.h>
#  include <poll.h>
#  include <pwd.h>
#  include <grp.h>
#  include <fnmatch.h>
#  include <sys/utsname.h>
#  include <sys/resource.h>
#  include <sys/socket.h>
#  include <sys/wait.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
/* getloadavg is BSD/POSIX extension; declare if libc omits under strict feature macros. */
int getloadavg(double loadavg[], int nelem);
#endif

int cubalc_lang_ops_sys_try_files(VM *vm, Lex *L) {
  /* bulk multi-file → lang_ops_sys_files_bulk.c */
  {
    int fb = cubalc_lang_ops_sys_try_files_bulk(vm, L);
    if (fb != 0) return fb;
  }

  /* line HEADFILE..COUNTFILELINE → lang_ops_sys_files_line.c (B_sys_files_peel) */
  {
    int fl = cubalc_lang_ops_sys_try_files_line(vm, L);
    if (fl != 0) return fl;
  }

    /* SYS GREPFILE|FILEGREP|MATCHFILELINES path needle
     * — keep file lines containing needle → LAST bag (cap 256 lines).
     * GREPFILEI case-insensitive. Empty needle → all lines (capped).
     * LAST_N / GREPFILE_N = kept; GREPFILE_TOTAL source lines; GREPFILE_HIT un-capped hit count.
     * Soft path miss → empty OK=0. Complements COUNTFILELINE (tally) / GREPLINES (path bag).
     * Usability: single-log line harvest without path-bag GREPLINES glue. */
    if (kw(&L->cur,"GREPFILE") || kw(&L->cur,"FILEGREP") || kw(&L->cur,"MATCHFILELINES") ||
        kw(&L->cur,"EXTRACTFILELINES") || kw(&L->cur,"FILTERFILE") || kw(&L->cur,"FILEFILTER") ||
        kw(&L->cur,"GREPFILEI") || kw(&L->cur,"FILEGREPI") || kw(&L->cur,"MATCHFILELINESI") ||
        kw(&L->cur,"EXTRACTFILELINESI") || kw(&L->cur,"FILTERFILEI")){
      char path[512], needle[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      char a[CUBALC_HOST_STR_MAX];
      const char *lp, *ls;
      size_t llen, nlen, olen = 0;
      long total = 0, kept = 0, hits = 0;
      int icase = 0, first = 1;
      cubalc_host_result hr;
      if (kw(&L->cur,"GREPFILEI") || kw(&L->cur,"FILEGREPI") ||
          kw(&L->cur,"MATCHFILELINESI") || kw(&L->cur,"EXTRACTFILELINESI") ||
          kw(&L->cur,"FILTERFILEI"))
        icase = 1;
      lex_next(L);
      path[0] = 0; needle[0] = 0; out[0] = 0; a[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS GREPFILE path needle");
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
        var_set_num(vm, "GREPFILE_N", 0);
        var_set_num(vm, "GREPFILE_TOTAL", 0);
        var_set_num(vm, "GREPFILE_HIT", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", path[0] ? "GREPFILE: read fail" : "GREPFILE: empty path");
        var_set_str(vm, "ERR", path[0] ? "GREPFILE: read fail" : "GREPFILE: empty path");
        bump(vm); return 1;
      }
      lp = hr.str;
      while (*lp) {
        int match = 0;
        ls = lp;
        while (*lp && *lp != '\n') lp++;
        llen = (size_t)(lp - ls);
        total++;
        if (nlen == 0) {
          match = 1;
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
          hits++;
          if (kept < 256) {
            if (!first && olen + 1 < sizeof out) out[olen++] = '\n';
            first = 0;
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
        }
        if (*lp == '\n') lp++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_str(vm, "GREPFILE", out);
      var_set_str(vm, "FILEGREP", out);
      var_set_num(vm, "GREPFILE_N", kept);
      var_set_num(vm, "GREPFILE_TOTAL", total);
      var_set_num(vm, "GREPFILE_HIT", hits);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SETMATCHFILE|FILESETMATCH|REPLACEMATCHF path needle value
     * — replace first file line containing needle with value (read-modify-write).
     * SETMATCHFILEI case-insensitive needle. Value is first line only.
     * LAST = new line when hit; SETMATCHFILE_N 0|1 · I · TOTAL.
     * Soft no-match → N=0 OK=1 file unchanged. Soft path miss → OK=0.
     * Usability: FINDFILELINE+SETFILELINE in one for plate/config patches. */
    if (kw(&L->cur,"SETMATCHFILE") || kw(&L->cur,"FILESETMATCH") || kw(&L->cur,"REPLACEMATCHF") ||
        kw(&L->cur,"SUBMATCHFILE") || kw(&L->cur,"PATCHFILELINE") || kw(&L->cur,"FILEPATCHLINE") ||
        kw(&L->cur,"SETMATCHFILEI") || kw(&L->cur,"FILESETMATCHI") || kw(&L->cur,"REPLACEMATCHFI") ||
        kw(&L->cur,"SUBMATCHFILEI") || kw(&L->cur,"PATCHFILELINEI")){
      char path[512], needle[CUBALC_HOST_STR_MAX], val[CUBALC_HOST_STR_MAX];
      char clean[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], a[CUBALC_HOST_STR_MAX];
      const char *lp, *ls, *vp;
      size_t llen, nlen, vlen, o = 0;
      long total = 0, seen = 0, found_i = -1, hit = 0;
      int icase = 0, first = 1;
      cubalc_host_result hr, wr;
      if (kw(&L->cur,"SETMATCHFILEI") || kw(&L->cur,"FILESETMATCHI") ||
          kw(&L->cur,"REPLACEMATCHFI") || kw(&L->cur,"SUBMATCHFILEI") ||
          kw(&L->cur,"PATCHFILELINEI"))
        icase = 1;
      lex_next(L);
      path[0] = 0; needle[0] = 0; val[0] = 0; clean[0] = 0; out[0] = 0; a[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS SETMATCHFILE path needle value");
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
      nlen = strlen(needle);
      if (!path[0] || cubalc_host_read(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SETMATCHFILE_N", 0);
        var_set_num(vm, "SETMATCHFILE_I", -1);
        var_set_num(vm, "SETMATCHFILE_TOTAL", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", path[0] ? "SETMATCHFILE: read fail" : "SETMATCHFILE: empty path");
        var_set_str(vm, "ERR", path[0] ? "SETMATCHFILE: read fail" : "SETMATCHFILE: empty path");
        bump(vm); return 1;
      }
      /* count total + find first match index */
      lp = hr.str;
      total = 0;
      seen = 0;
      found_i = -1;
      if (*lp) {
        while (*lp) {
          int match = 0;
          ls = lp;
          while (*lp && *lp != '\n') lp++;
          llen = (size_t)(lp - ls);
          if (found_i < 0 && nlen > 0 && llen >= nlen) {
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
            if (match) found_i = seen;
          }
          total++;
          seen++;
          if (*lp == '\n') lp++;
        }
      }
      if (found_i < 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SETMATCHFILE_N", 0);
        var_set_num(vm, "SETMATCHFILE_I", -1);
        var_set_num(vm, "SETMATCHFILE_TOTAL", total);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      /* rebuild with replacement at found_i */
      lp = hr.str;
      seen = 0;
      while (seen < total) {
        ls = lp;
        while (*lp && *lp != '\n') lp++;
        llen = (size_t)(lp - ls);
        if (!first && o + 1 < sizeof out) out[o++] = '\n';
        first = 0;
        if (seen == found_i) {
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
        var_set_num(vm, "SETMATCHFILE_N", 0);
        var_set_num(vm, "SETMATCHFILE_I", found_i);
        var_set_num(vm, "SETMATCHFILE_TOTAL", total);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "SETMATCHFILE: write fail");
        var_set_str(vm, "ERR", wr.err[0] ? wr.err : "SETMATCHFILE: write fail");
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", clean);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", clean);
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_str(vm, "SETMATCHFILE", clean);
      var_set_str(vm, "FILESETMATCH", clean);
      var_set_num(vm, "SETMATCHFILE_N", hit);
      var_set_num(vm, "SETMATCHFILE_I", found_i);
      var_set_num(vm, "SETMATCHFILE_TOTAL", total);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS DROPMATCHFILE|FILEDROPMATCH|DELMATCHFILE path needle
     * — drop first file line containing needle (read-modify-write).
     * Dual of SETMATCHFILE / file DROPNTH-by-content. DROPMATCHFILEI icase.
     * LAST = dropped line when hit; DROPMATCHFILE_N 0|1 · I · TOTAL(after).
     * Soft no-match → N=0 OK=1 file unchanged. Soft path miss → OK=0.
     * Usability: FINDFILELINE+DROPFILELINE in one for plate/config ack-delete. */
    if (kw(&L->cur,"DROPMATCHFILE") || kw(&L->cur,"FILEDROPMATCH") || kw(&L->cur,"DELMATCHFILE") ||
        kw(&L->cur,"DELETEMATCHFILE") || kw(&L->cur,"REMOVEMATCHF") || kw(&L->cur,"FILEDELMATCH") ||
        kw(&L->cur,"DROPMATCHFILEI") || kw(&L->cur,"FILEDROPMATCHI") || kw(&L->cur,"DELMATCHFILEI") ||
        kw(&L->cur,"DELETEMATCHFILEI") || kw(&L->cur,"REMOVEMATCHFI")){
      char path[512], needle[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      char dropped[CUBALC_HOST_STR_MAX], a[CUBALC_HOST_STR_MAX];
      const char *lp, *ls;
      size_t llen, nlen, o = 0;
      long total = 0, seen = 0, found_i = -1, hit = 0;
      int icase = 0, first_kept = 1;
      cubalc_host_result hr, wr;
      if (kw(&L->cur,"DROPMATCHFILEI") || kw(&L->cur,"FILEDROPMATCHI") ||
          kw(&L->cur,"DELMATCHFILEI") || kw(&L->cur,"DELETEMATCHFILEI") ||
          kw(&L->cur,"REMOVEMATCHFI"))
        icase = 1;
      lex_next(L);
      path[0] = 0; needle[0] = 0; out[0] = 0; dropped[0] = 0; a[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS DROPMATCHFILE path needle");
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
        var_set_num(vm, "DROPMATCHFILE_N", 0);
        var_set_num(vm, "DROPMATCHFILE_I", -1);
        var_set_num(vm, "DROPMATCHFILE_TOTAL", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", path[0] ? "DROPMATCHFILE: read fail" : "DROPMATCHFILE: empty path");
        var_set_str(vm, "ERR", path[0] ? "DROPMATCHFILE: read fail" : "DROPMATCHFILE: empty path");
        bump(vm); return 1;
      }
      lp = hr.str;
      total = 0;
      seen = 0;
      found_i = -1;
      if (*lp) {
        while (*lp) {
          int match = 0;
          ls = lp;
          while (*lp && *lp != '\n') lp++;
          llen = (size_t)(lp - ls);
          if (found_i < 0 && nlen > 0 && llen >= nlen) {
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
            if (match) found_i = seen;
          }
          total++;
          seen++;
          if (*lp == '\n') lp++;
        }
      }
      if (found_i < 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DROPMATCHFILE_N", 0);
        var_set_num(vm, "DROPMATCHFILE_I", -1);
        var_set_num(vm, "DROPMATCHFILE_TOTAL", total);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      lp = hr.str;
      seen = 0;
      while (seen < total) {
        ls = lp;
        while (*lp && *lp != '\n') lp++;
        llen = (size_t)(lp - ls);
        if (seen == found_i) {
          hit = 1;
          if (llen >= sizeof dropped) llen = sizeof dropped - 1;
          memcpy(dropped, ls, llen);
          dropped[llen] = 0;
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
        var_set_num(vm, "DROPMATCHFILE_N", 0);
        var_set_num(vm, "DROPMATCHFILE_I", found_i);
        var_set_num(vm, "DROPMATCHFILE_TOTAL", total);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "DROPMATCHFILE: write fail");
        var_set_str(vm, "ERR", wr.err[0] ? wr.err : "DROPMATCHFILE: write fail");
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", dropped);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", dropped);
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_str(vm, "DROPMATCHFILE", dropped);
      var_set_str(vm, "FILEDROPMATCH", dropped);
      var_set_num(vm, "DROPMATCHFILE_N", hit);
      var_set_num(vm, "DROPMATCHFILE_I", found_i);
      var_set_num(vm, "DROPMATCHFILE_TOTAL", total > 0 ? total - 1 : 0);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS DROPALLMATCHFILE|FILEDROPALLMATCH|DELALLMATCHFILE path needle
     * — drop every file line containing needle (read-modify-write).
     * Dual of DROPMATCHFILE for bulk purge; file GREPV write-back.
     * DROPALLMATCHFILEI icase. LAST = bag of dropped lines when hit.
     * DROPALLMATCHFILE_N = count dropped · I = first index · TOTAL(after).
     * Soft no-match → N=0 OK=1 file unchanged. Soft path miss → OK=0.
     * Usability: purge all status/comment rows without FIND+DROP loop. */
    if (kw(&L->cur,"DROPALLMATCHFILE") || kw(&L->cur,"FILEDROPALLMATCH") ||
        kw(&L->cur,"DELALLMATCHFILE") || kw(&L->cur,"DELETEALLMATCHF") ||
        kw(&L->cur,"REMOVEALLMATCHF") || kw(&L->cur,"PURGEMATCHFILE") ||
        kw(&L->cur,"FILEPURGEMATCH") || kw(&L->cur,"DROPALLMATCHF") ||
        kw(&L->cur,"DROPALLMATCHFILEI") || kw(&L->cur,"FILEDROPALLMATCHI") ||
        kw(&L->cur,"DELALLMATCHFILEI") || kw(&L->cur,"DELETEALLMATCHFI") ||
        kw(&L->cur,"REMOVEALLMATCHFI") || kw(&L->cur,"PURGEMATCHFILEI") ||
        kw(&L->cur,"FILEPURGEMATCHI") || kw(&L->cur,"DROPALLMATCHFI")){
      char path[512], needle[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      char dropped[CUBALC_HOST_STR_MAX], a[CUBALC_HOST_STR_MAX];
      const char *lp, *ls;
      size_t llen, nlen, o = 0, doff = 0;
      long total = 0, seen = 0, found_i = -1, hits = 0, kept = 0;
      int icase = 0, first_kept = 1, first_drop = 1;
      cubalc_host_result hr, wr;
      if (kw(&L->cur,"DROPALLMATCHFILEI") || kw(&L->cur,"FILEDROPALLMATCHI") ||
          kw(&L->cur,"DELALLMATCHFILEI") || kw(&L->cur,"DELETEALLMATCHFI") ||
          kw(&L->cur,"REMOVEALLMATCHFI") || kw(&L->cur,"PURGEMATCHFILEI") ||
          kw(&L->cur,"FILEPURGEMATCHI") || kw(&L->cur,"DROPALLMATCHFI"))
        icase = 1;
      lex_next(L);
      path[0] = 0; needle[0] = 0; out[0] = 0; dropped[0] = 0; a[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS DROPALLMATCHFILE path needle");
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
        var_set_num(vm, "DROPALLMATCHFILE_N", 0);
        var_set_num(vm, "DROPALLMATCHFILE_I", -1);
        var_set_num(vm, "DROPALLMATCHFILE_TOTAL", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", path[0] ? "DROPALLMATCHFILE: read fail" : "DROPALLMATCHFILE: empty path");
        var_set_str(vm, "ERR", path[0] ? "DROPALLMATCHFILE: read fail" : "DROPALLMATCHFILE: empty path");
        bump(vm); return 1;
      }
      lp = hr.str;
      total = 0;
      seen = 0;
      found_i = -1;
      hits = 0;
      kept = 0;
      /* single pass: classify each line, build kept out + dropped bag */
      while (*lp) {
        int match = 0;
        ls = lp;
        while (*lp && *lp != '\n') lp++;
        llen = (size_t)(lp - ls);
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
        if (match) {
          if (found_i < 0) found_i = seen;
          hits++;
          if (!first_drop && doff + 1 < sizeof dropped) dropped[doff++] = '\n';
          first_drop = 0;
          if (doff + llen < sizeof dropped) {
            memcpy(dropped + doff, ls, llen);
            doff += llen;
          } else if (doff < sizeof dropped - 1) {
            size_t take = sizeof dropped - 1 - doff;
            memcpy(dropped + doff, ls, take);
            doff += take;
          }
          dropped[doff] = 0;
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
          kept++;
        }
        total++;
        seen++;
        if (*lp == '\n') lp++;
        else break;
      }
      if (hits == 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DROPALLMATCHFILE_N", 0);
        var_set_num(vm, "DROPALLMATCHFILE_I", -1);
        var_set_num(vm, "DROPALLMATCHFILE_TOTAL", total);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      if (cubalc_host_write(path, out, &wr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DROPALLMATCHFILE_N", 0);
        var_set_num(vm, "DROPALLMATCHFILE_I", found_i);
        var_set_num(vm, "DROPALLMATCHFILE_TOTAL", total);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "DROPALLMATCHFILE: write fail");
        var_set_str(vm, "ERR", wr.err[0] ? wr.err : "DROPALLMATCHFILE: write fail");
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", dropped);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", dropped);
      vm->last_n = hits;
      var_set_num(vm, "LAST_N", hits);
      var_set_str(vm, "DROPALLMATCHFILE", dropped);
      var_set_str(vm, "FILEDROPALLMATCH", dropped);
      var_set_num(vm, "DROPALLMATCHFILE_N", hits);
      var_set_num(vm, "DROPALLMATCHFILE_I", found_i);
      var_set_num(vm, "DROPALLMATCHFILE_TOTAL", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SETALLMATCHFILE|FILESETALLMATCH|REPLACEALLMATCHF path needle value
     * — replace every file line containing needle with value (read-modify-write).
     * Bulk dual of SETMATCHFILE / file map-set. SETALLMATCHFILEI icase.
     * Value is first line only. LAST = new line text when any hit.
     * SETALLMATCHFILE_N = count replaced · I = first index · TOTAL lines.
     * Soft no-match → N=0 OK=1 file unchanged. Soft path miss → OK=0.
     * Usability: bulk status rewrite without FIND+SET loop / GREP+WRITE glue. */
    if (kw(&L->cur,"SETALLMATCHFILE") || kw(&L->cur,"FILESETALLMATCH") ||
        kw(&L->cur,"REPLACEALLMATCHF") || kw(&L->cur,"SUBALLMATCHFILE") ||
        kw(&L->cur,"PATCHALLFILELINE") || kw(&L->cur,"FILEPATCHALL") ||
        kw(&L->cur,"SETALLMATCHF") || kw(&L->cur,"REPLALLMATCHF") ||
        kw(&L->cur,"SETALLMATCHFILEI") || kw(&L->cur,"FILESETALLMATCHI") ||
        kw(&L->cur,"REPLACEALLMATCHFI") || kw(&L->cur,"SUBALLMATCHFILEI") ||
        kw(&L->cur,"PATCHALLFILELINEI") || kw(&L->cur,"FILEPATCHALLI") ||
        kw(&L->cur,"SETALLMATCHFI") || kw(&L->cur,"REPLALLMATCHFI")){
      char path[512], needle[CUBALC_HOST_STR_MAX], val[CUBALC_HOST_STR_MAX];
      char clean[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], a[CUBALC_HOST_STR_MAX];
      const char *lp, *ls, *vp;
      size_t llen, nlen, vlen, o = 0;
      long total = 0, seen = 0, found_i = -1, hits = 0;
      int icase = 0, first = 1;
      cubalc_host_result hr, wr;
      if (kw(&L->cur,"SETALLMATCHFILEI") || kw(&L->cur,"FILESETALLMATCHI") ||
          kw(&L->cur,"REPLACEALLMATCHFI") || kw(&L->cur,"SUBALLMATCHFILEI") ||
          kw(&L->cur,"PATCHALLFILELINEI") || kw(&L->cur,"FILEPATCHALLI") ||
          kw(&L->cur,"SETALLMATCHFI") || kw(&L->cur,"REPLALLMATCHFI"))
        icase = 1;
      lex_next(L);
      path[0] = 0; needle[0] = 0; val[0] = 0; clean[0] = 0; out[0] = 0; a[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS SETALLMATCHFILE path needle value");
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
      nlen = strlen(needle);
      if (!path[0] || cubalc_host_read(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SETALLMATCHFILE_N", 0);
        var_set_num(vm, "SETALLMATCHFILE_I", -1);
        var_set_num(vm, "SETALLMATCHFILE_TOTAL", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", path[0] ? "SETALLMATCHFILE: read fail" : "SETALLMATCHFILE: empty path");
        var_set_str(vm, "ERR", path[0] ? "SETALLMATCHFILE: read fail" : "SETALLMATCHFILE: empty path");
        bump(vm); return 1;
      }
      lp = hr.str;
      total = 0;
      seen = 0;
      found_i = -1;
      hits = 0;
      while (*lp) {
        int match = 0;
        ls = lp;
        while (*lp && *lp != '\n') lp++;
        llen = (size_t)(lp - ls);
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
        if (!first && o + 1 < sizeof out) out[o++] = '\n';
        first = 0;
        if (match) {
          if (found_i < 0) found_i = seen;
          hits++;
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
        total++;
        seen++;
        if (*lp == '\n') lp++;
        else break;
      }
      if (hits == 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SETALLMATCHFILE_N", 0);
        var_set_num(vm, "SETALLMATCHFILE_I", -1);
        var_set_num(vm, "SETALLMATCHFILE_TOTAL", total);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      if (cubalc_host_write(path, out, &wr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "SETALLMATCHFILE_N", 0);
        var_set_num(vm, "SETALLMATCHFILE_I", found_i);
        var_set_num(vm, "SETALLMATCHFILE_TOTAL", total);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "SETALLMATCHFILE: write fail");
        var_set_str(vm, "ERR", wr.err[0] ? wr.err : "SETALLMATCHFILE: write fail");
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", clean);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", clean);
      vm->last_n = hits;
      var_set_num(vm, "LAST_N", hits);
      var_set_str(vm, "SETALLMATCHFILE", clean);
      var_set_str(vm, "FILESETALLMATCH", clean);
      var_set_num(vm, "SETALLMATCHFILE_N", hits);
      var_set_num(vm, "SETALLMATCHFILE_I", found_i);
      var_set_num(vm, "SETALLMATCHFILE_TOTAL", total);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS INSERTMATCHFILE|FILEINSERTMATCH|AFTERMATCHFILE path needle value
     * — insert value as a new line AFTER the first file line containing needle.
     * FINDFILELINE+INSERTFILELINE one-shot for section/config patching.
     * INSERTMATCHFILEI icase. Value is first line only.
     * LAST = inserted line; INSERTMATCHFILE_N 0|1 · I (insert idx) · TOTAL(after).
     * Soft no-match → N=0 OK=1 file unchanged. Soft path miss → OK=0. */
    if (kw(&L->cur,"INSERTMATCHFILE") || kw(&L->cur,"FILEINSERTMATCH") ||
        kw(&L->cur,"AFTERMATCHFILE") || kw(&L->cur,"INSMATCHFILE") ||
        kw(&L->cur,"INSERTAFTERMATCH") || kw(&L->cur,"FILEINSAFTER") ||
        kw(&L->cur,"INSERTMATCHFILEI") || kw(&L->cur,"FILEINSERTMATCHI") ||
        kw(&L->cur,"AFTERMATCHFILEI") || kw(&L->cur,"INSMATCHFILEI") ||
        kw(&L->cur,"INSERTAFTERMATCHI") || kw(&L->cur,"FILEINSAFTERI")){
      char path[512], needle[CUBALC_HOST_STR_MAX], val[CUBALC_HOST_STR_MAX];
      char clean[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], a[CUBALC_HOST_STR_MAX];
      const char *lp, *ls, *vp;
      size_t llen, nlen, vlen, o = 0;
      long total = 0, seen = 0, found_i = -1, ins_i = -1, hit = 0;
      int icase = 0, first = 1;
      cubalc_host_result hr, wr;
      if (kw(&L->cur,"INSERTMATCHFILEI") || kw(&L->cur,"FILEINSERTMATCHI") ||
          kw(&L->cur,"AFTERMATCHFILEI") || kw(&L->cur,"INSMATCHFILEI") ||
          kw(&L->cur,"INSERTAFTERMATCHI") || kw(&L->cur,"FILEINSAFTERI"))
        icase = 1;
      lex_next(L);
      path[0] = 0; needle[0] = 0; val[0] = 0; clean[0] = 0; out[0] = 0; a[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS INSERTMATCHFILE path needle value");
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
      nlen = strlen(needle);
      if (!path[0] || cubalc_host_read(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "INSERTMATCHFILE_N", 0);
        var_set_num(vm, "INSERTMATCHFILE_I", -1);
        var_set_num(vm, "INSERTMATCHFILE_TOTAL", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", path[0] ? "INSERTMATCHFILE: read fail" : "INSERTMATCHFILE: empty path");
        var_set_str(vm, "ERR", path[0] ? "INSERTMATCHFILE: read fail" : "INSERTMATCHFILE: empty path");
        bump(vm); return 1;
      }
      /* locate first matching line index */
      lp = hr.str;
      total = 0;
      seen = 0;
      found_i = -1;
      if (*lp) {
        while (*lp) {
          int match = 0;
          ls = lp;
          while (*lp && *lp != '\n') lp++;
          llen = (size_t)(lp - ls);
          if (found_i < 0 && nlen > 0 && llen >= nlen) {
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
            if (match) found_i = seen;
          }
          total++;
          seen++;
          if (*lp == '\n') lp++;
        }
      }
      if (found_i < 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "INSERTMATCHFILE_N", 0);
        var_set_num(vm, "INSERTMATCHFILE_I", -1);
        var_set_num(vm, "INSERTMATCHFILE_TOTAL", total);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      /* rebuild: copy lines; after found_i emit inserted line */
      ins_i = found_i + 1;
      lp = hr.str;
      seen = 0;
      while (seen < total) {
        ls = lp;
        while (*lp && *lp != '\n') lp++;
        llen = (size_t)(lp - ls);
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
        if (seen == found_i) {
          /* insert after this line */
          if (o + 1 < sizeof out) out[o++] = '\n';
          if (o + vlen < sizeof out) {
            memcpy(out + o, clean, vlen);
            o += vlen;
          } else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, clean, take);
            o += take;
          }
          out[o] = 0;
          hit = 1;
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
        var_set_num(vm, "INSERTMATCHFILE_N", 0);
        var_set_num(vm, "INSERTMATCHFILE_I", ins_i);
        var_set_num(vm, "INSERTMATCHFILE_TOTAL", total);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "INSERTMATCHFILE: write fail");
        var_set_str(vm, "ERR", wr.err[0] ? wr.err : "INSERTMATCHFILE: write fail");
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", clean);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", clean);
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_str(vm, "INSERTMATCHFILE", clean);
      var_set_str(vm, "FILEINSERTMATCH", clean);
      var_set_str(vm, "AFTERMATCHFILE", clean);
      var_set_num(vm, "INSERTMATCHFILE_N", hit);
      var_set_num(vm, "INSERTMATCHFILE_I", ins_i);
      var_set_num(vm, "INSERTMATCHFILE_TOTAL", total + 1);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS BEFOREMATCHFILE|INSERTBEFOREMATCH|FILEINSERTBEFORE path needle value
     * — insert value as a new line BEFORE the first file line containing needle.
     * Dual of AFTERMATCHFILE / INSERTMATCHFILE for header/import patches.
     * BEFOREMATCHFILEI icase. Value is first line only.
     * LAST = inserted line; BEFOREMATCHFILE_N 0|1 · I (insert idx) · TOTAL(after).
     * Soft no-match → N=0 OK=1 file unchanged. Soft path miss → OK=0. */
    if (kw(&L->cur,"BEFOREMATCHFILE") || kw(&L->cur,"INSERTBEFOREMATCH") ||
        kw(&L->cur,"FILEINSERTBEFORE") || kw(&L->cur,"INSMATCHBEFORE") ||
        kw(&L->cur,"INSERTBEFOREFILE") || kw(&L->cur,"PREPENDMATCHF") ||
        kw(&L->cur,"BEFOREMATCHFILEI") || kw(&L->cur,"INSERTBEFOREMATCHI") ||
        kw(&L->cur,"FILEINSERTBEFOREI") || kw(&L->cur,"INSMATCHBEFOREI") ||
        kw(&L->cur,"INSERTBEFOREFILEI") || kw(&L->cur,"PREPENDMATCHFI")){
      char path[512], needle[CUBALC_HOST_STR_MAX], val[CUBALC_HOST_STR_MAX];
      char clean[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], a[CUBALC_HOST_STR_MAX];
      const char *lp, *ls, *vp;
      size_t llen, nlen, vlen, o = 0;
      long total = 0, seen = 0, found_i = -1, ins_i = -1, hit = 0;
      int icase = 0, first = 1;
      cubalc_host_result hr, wr;
      if (kw(&L->cur,"BEFOREMATCHFILEI") || kw(&L->cur,"INSERTBEFOREMATCHI") ||
          kw(&L->cur,"FILEINSERTBEFOREI") || kw(&L->cur,"INSMATCHBEFOREI") ||
          kw(&L->cur,"INSERTBEFOREFILEI") || kw(&L->cur,"PREPENDMATCHFI"))
        icase = 1;
      lex_next(L);
      path[0] = 0; needle[0] = 0; val[0] = 0; clean[0] = 0; out[0] = 0; a[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS BEFOREMATCHFILE path needle value");
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
      nlen = strlen(needle);
      if (!path[0] || cubalc_host_read(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "BEFOREMATCHFILE_N", 0);
        var_set_num(vm, "BEFOREMATCHFILE_I", -1);
        var_set_num(vm, "BEFOREMATCHFILE_TOTAL", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", path[0] ? "BEFOREMATCHFILE: read fail" : "BEFOREMATCHFILE: empty path");
        var_set_str(vm, "ERR", path[0] ? "BEFOREMATCHFILE: read fail" : "BEFOREMATCHFILE: empty path");
        bump(vm); return 1;
      }
      lp = hr.str;
      total = 0;
      seen = 0;
      found_i = -1;
      if (*lp) {
        while (*lp) {
          int match = 0;
          ls = lp;
          while (*lp && *lp != '\n') lp++;
          llen = (size_t)(lp - ls);
          if (found_i < 0 && nlen > 0 && llen >= nlen) {
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
            if (match) found_i = seen;
          }
          total++;
          seen++;
          if (*lp == '\n') lp++;
        }
      }
      if (found_i < 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "BEFOREMATCHFILE_N", 0);
        var_set_num(vm, "BEFOREMATCHFILE_I", -1);
        var_set_num(vm, "BEFOREMATCHFILE_TOTAL", total);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      /* rebuild: insert clean before found_i */
      ins_i = found_i;
      lp = hr.str;
      seen = 0;
      while (seen < total) {
        ls = lp;
        while (*lp && *lp != '\n') lp++;
        llen = (size_t)(lp - ls);
        if (seen == found_i) {
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
          hit = 1;
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
      if (cubalc_host_write(path, out, &wr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "BEFOREMATCHFILE_N", 0);
        var_set_num(vm, "BEFOREMATCHFILE_I", ins_i);
        var_set_num(vm, "BEFOREMATCHFILE_TOTAL", total);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "BEFOREMATCHFILE: write fail");
        var_set_str(vm, "ERR", wr.err[0] ? wr.err : "BEFOREMATCHFILE: write fail");
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", clean);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", clean);
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_str(vm, "BEFOREMATCHFILE", clean);
      var_set_str(vm, "INSERTBEFOREMATCH", clean);
      var_set_str(vm, "FILEINSERTBEFORE", clean);
      var_set_num(vm, "BEFOREMATCHFILE_N", hit);
      var_set_num(vm, "BEFOREMATCHFILE_I", ins_i);
      var_set_num(vm, "BEFOREMATCHFILE_TOTAL", total + 1);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS LOOKUPFILE|FILELOOKUP|KVFILEGET path key [sep] [OR|DEFAULT fallback]
     * — peel value from first key:val line in a plate/config file.
     * Default sep "=" (file/config style; pass ":" for FREQ plates).
     * LAST = value; LOOKUPFILE_LINE = full line; N 0|1 · I · OR.
     * LOOKUPFILEI icase key. Soft key miss → OK=0; OR → OK=1 LOOKUPFILE_OR=1.
     * Soft path miss → OK=0 sticky LAST_ERR.
     * Usability: plate status without READ+LOOKUP glue. */
    if (kw(&L->cur,"LOOKUPFILE") || kw(&L->cur,"FILELOOKUP") ||
        kw(&L->cur,"KVFILEGET") || kw(&L->cur,"GETKVFILE") ||
        kw(&L->cur,"FILEKVGET") || kw(&L->cur,"PLATEGET") ||
        kw(&L->cur,"CONFIGGET") || kw(&L->cur,"FILEGETKV") ||
        kw(&L->cur,"LOOKUPFILEI") || kw(&L->cur,"FILELOOKUPI") ||
        kw(&L->cur,"KVFILEGETI") || kw(&L->cur,"GETKVFILEI") ||
        kw(&L->cur,"FILEKVGETI") || kw(&L->cur,"PLATEGETI") ||
        kw(&L->cur,"CONFIGGETI") || kw(&L->cur,"FILEGETKVI")){
      char path[512], key[256], sep[32], out[512], line[512], fb[512], a[CUBALC_HOST_STR_MAX];
      int icase = 0, have_fb = 0, used_or = 0;
      const char *p, *start;
      size_t flen, kn, sn;
      long idx = 0, found = 0, found_i = -1;
      cubalc_host_result hr;
      if (kw(&L->cur,"LOOKUPFILEI") || kw(&L->cur,"FILELOOKUPI") ||
          kw(&L->cur,"KVFILEGETI") || kw(&L->cur,"GETKVFILEI") ||
          kw(&L->cur,"FILEKVGETI") || kw(&L->cur,"PLATEGETI") ||
          kw(&L->cur,"CONFIGGETI") || kw(&L->cur,"FILEGETKVI"))
        icase = 1;
      lex_next(L);
      path[0] = 0; key[0] = 0; out[0] = 0; line[0] = 0; fb[0] = 0; a[0] = 0;
      snprintf(sep, sizeof sep, "%s", "=");
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS LOOKUPFILE path key [sep] [OR fallback]");
        return -1;
      }
      snprintf(path, sizeof path, "%s", a);
      if (resolve_str_arg(vm, L, key, sizeof key) != 0)
        key[0] = 0;
      /* optional sep — not OR/DEFAULT */
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT &&
                 strcmp(L->cur.text, "OR") != 0 &&
                 strcmp(L->cur.text, "SOFT") != 0 &&
                 strcmp(L->cur.text, "DEFAULT") != 0 &&
                 strcmp(L->cur.text, "ELSE") != 0 &&
                 strcmp(L->cur.text, "FALLBACK") != 0) {
        if (strlen(L->cur.text) <= 2) {
          snprintf(sep, sizeof sep, "%s", L->cur.text);
          lex_next(L);
        }
      }
      if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
          kw(&L->cur,"FALLBACK") || kw(&L->cur,"SOFT")){
        lex_next(L);
        if (resolve_str_arg(vm, L, fb, sizeof fb) != 0) {
          fail(vm, "SYS LOOKUPFILE path key [sep] OR \"fallback\"");
          return -1;
        }
        have_fb = 1;
      }
      kn = strlen(key);
      sn = strlen(sep);
      if (!path[0] || cubalc_host_read(path, &hr) != 0) {
        if (have_fb) {
          snprintf(out, sizeof out, "%s", fb);
          used_or = 1;
          var_set_str(vm, "LAST", out);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_str(vm, "LOOKUPFILE", out);
          var_set_str(vm, "FILELOOKUP", out);
          var_set_str(vm, "LOOKUPFILE_LINE", "");
          var_set_num(vm, "LOOKUPFILE_N", 0);
          var_set_num(vm, "LOOKUPFILE_I", -1);
          var_set_num(vm, "LOOKUPFILE_OR", 1);
          var_set_num(vm, "OK", 1);
          bump(vm); return 1;
        }
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "LOOKUPFILE_N", 0);
        var_set_num(vm, "LOOKUPFILE_I", -1);
        var_set_num(vm, "LOOKUPFILE_OR", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", path[0] ? "LOOKUPFILE: read fail" : "LOOKUPFILE: empty path");
        var_set_str(vm, "ERR", path[0] ? "LOOKUPFILE: read fail" : "LOOKUPFILE: empty path");
        bump(vm); return 1;
      }
      p = hr.str;
      while (*p) {
        int hit = 0;
        const char *sp = 0;
        char field[512], left[256];
        size_t take, left_n = 0;
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        take = flen;
        if (take >= sizeof field) take = sizeof field - 1;
        memcpy(field, start, take);
        field[take] = 0;
        if (sn == 0) {
          if (!icase) {
            hit = (strcmp(field, key) == 0);
          } else {
            size_t a2 = strlen(field), j;
            if (a2 == kn) {
              hit = 1;
              for (j = 0; j < kn; j++) {
                char ca = field[j], cb = key[j];
                if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
                if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
                if (ca != cb) { hit = 0; break; }
              }
            }
          }
          if (hit) snprintf(out, sizeof out, "%s", field);
        } else {
          sp = strstr(field, sep);
          if (sp) {
            left_n = (size_t)(sp - field);
            if (left_n >= sizeof left) left_n = sizeof left - 1;
            memcpy(left, field, left_n);
            left[left_n] = 0;
          } else {
            left_n = take;
            if (left_n >= sizeof left) left_n = sizeof left - 1;
            memcpy(left, field, left_n);
            left[left_n] = 0;
          }
          if (!icase) {
            hit = (strcmp(left, key) == 0);
          } else {
            size_t j;
            if (left_n == kn) {
              hit = 1;
              for (j = 0; j < kn; j++) {
                char ca = left[j], cb = key[j];
                if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
                if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
                if (ca != cb) { hit = 0; break; }
              }
            }
          }
          if (hit) {
            if (sp) snprintf(out, sizeof out, "%s", sp + sn);
            else out[0] = 0;
          }
        }
        if (hit) {
          if (flen >= sizeof line) flen = sizeof line - 1;
          memcpy(line, start, flen);
          line[flen] = 0;
          found = 1;
          found_i = idx;
          break;
        }
        idx++;
        if (*p == '\n') p++;
      }
      if (!found && have_fb) {
        snprintf(out, sizeof out, "%s", fb);
        used_or = 1;
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "LOOKUPFILE", out);
      var_set_str(vm, "FILELOOKUP", out);
      var_set_str(vm, "KVFILEGET", out);
      var_set_str(vm, "LOOKUPFILE_LINE", line);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      var_set_num(vm, "LOOKUPFILE_N", found);
      var_set_num(vm, "LOOKUPFILE_I", found_i);
      var_set_num(vm, "LOOKUPFILE_OR", used_or);
      var_set_num(vm, "FILELOOKUP_N", found);
      var_set_num(vm, "OK", (found || used_or) ? 1 : 0);
      bump(vm); return 1;
    }
    /* SYS KVFILESET|FILEKVSET|SETKVFILE path key value [sep]
     * — set/update key=val line in a plate file (read-modify-write).
     * Dual of LOOKUPFILE / file KVSET. Default sep "=".
     * Hit → replace first matching key; miss → append. Missing file → create.
     * LAST = key+sep+value line; KVFILESET_HIT 1|0 · I · TOTAL(after).
     * Soft empty path → OK=0. Soft write fail → OK=0.
     * Usability: plate status write without READ+KVSET+WRITE glue. */
    if (kw(&L->cur,"KVFILESET") || kw(&L->cur,"FILEKVSET") ||
        kw(&L->cur,"SETKVFILE") || kw(&L->cur,"PUTKVFILE") ||
        kw(&L->cur,"PLATESET") || kw(&L->cur,"CONFIGSET") ||
        kw(&L->cur,"FILESETKV") || kw(&L->cur,"KVFILEPUT") ||
        kw(&L->cur,"SETFILEKV") || kw(&L->cur,"WRITEKVFILE")){
      char path[512], key[256], val[512], sep[32], a[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX], newline[768], field[512], left[256];
      const char *p, *start;
      size_t flen, kn, sn, vn, olen = 0, left_n;
      long idx = 0, kept = 0, hit = 0, found_i = -1;
      int first = 1, created = 0;
      cubalc_host_result hr, wr;
      lex_next(L);
      path[0] = 0; key[0] = 0; val[0] = 0; out[0] = 0; a[0] = 0; newline[0] = 0;
      snprintf(sep, sizeof sep, "%s", "=");
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS KVFILESET path key value [sep]");
        return -1;
      }
      snprintf(path, sizeof path, "%s", a);
      if (resolve_str_arg(vm, L, key, sizeof key) != 0) key[0] = 0;
      if (resolve_str_arg(vm, L, val, sizeof val) != 0) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long n = parse_expr(vm, L);
          snprintf(val, sizeof val, "%ld", n);
        } else {
          val[0] = 0;
        }
      }
      /* strip value to first line */
      {
        size_t i = 0;
        while (val[i] && val[i] != '\n' && val[i] != '\r' && i + 1 < sizeof val) i++;
        val[i] = 0;
      }
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", "=");
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && strlen(L->cur.text) <= 2) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        lex_next(L);
      }
      kn = strlen(key);
      sn = strlen(sep);
      vn = strlen(val);
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "KVFILESET_N", 0);
        var_set_num(vm, "KVFILESET_I", -1);
        var_set_num(vm, "KVFILESET_HIT", 0);
        var_set_num(vm, "KVFILESET_TOTAL", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "KVFILESET: empty path");
        var_set_str(vm, "ERR", "KVFILESET: empty path");
        bump(vm); return 1;
      }
      if (cubalc_host_read(path, &hr) != 0) {
        /* create new plate with single key=val */
        created = 1;
        hr.str[0] = 0;
      }
      p = hr.str;
      if (*p) {
        while (*p) {
          int match = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            if (sn == 0) {
              match = (strcmp(field, key) == 0);
            } else {
              const char *sp = strstr(field, sep);
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
              } else {
                left_n = take;
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
              }
              match = (strcmp(left, key) == 0);
            }
            if (match && found_i < 0) {
              if (sn == 0)
                snprintf(newline, sizeof newline, "%s", val);
              else
                snprintf(newline, sizeof newline, "%s%s%s", key, sep, val);
              found_i = idx;
              hit = 1;
            } else {
              snprintf(newline, sizeof newline, "%s", field);
            }
          }
          {
            size_t nlen = strlen(newline);
            if (!first && olen + 1 < sizeof out) out[olen++] = '\n';
            first = 0;
            if (olen + nlen < sizeof out) {
              memcpy(out + olen, newline, nlen);
              olen += nlen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, newline, t);
              olen += t;
            }
            out[olen] = 0;
          }
          kept++;
          idx++;
          if (*p == '\n') p++;
        }
      }
      if (!hit) {
        if (sn == 0)
          snprintf(newline, sizeof newline, "%s", key[0] ? key : val);
        else
          snprintf(newline, sizeof newline, "%s%s%s", key, sep, val);
        {
          size_t nlen = strlen(newline);
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + nlen < sizeof out) {
            memcpy(out + olen, newline, nlen);
            olen += nlen;
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            memcpy(out + olen, newline, t);
            olen += t;
          }
          out[olen] = 0;
        }
        found_i = kept;
        kept++;
      } else {
        /* rebuild newline for LAST reporting */
        if (sn == 0)
          snprintf(newline, sizeof newline, "%s", val);
        else
          snprintf(newline, sizeof newline, "%s%s%s", key, sep, val);
      }
      if (cubalc_host_write(path, out, &wr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "KVFILESET_N", 0);
        var_set_num(vm, "KVFILESET_I", found_i);
        var_set_num(vm, "KVFILESET_HIT", hit);
        var_set_num(vm, "KVFILESET_TOTAL", kept);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "KVFILESET: write fail");
        var_set_str(vm, "ERR", wr.err[0] ? wr.err : "KVFILESET: write fail");
        bump(vm); return 1;
      }
      (void)kn; (void)vn; (void)created;
      var_set_str(vm, "LAST", newline);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", newline);
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_str(vm, "KVFILESET", newline);
      var_set_str(vm, "FILEKVSET", newline);
      var_set_str(vm, "SETKVFILE", newline);
      var_set_num(vm, "KVFILESET_N", kept);
      var_set_num(vm, "KVFILESET_I", found_i);
      var_set_num(vm, "KVFILESET_HIT", hit);
      var_set_num(vm, "KVFILESET_TOTAL", kept);
      var_set_num(vm, "KVFILESET_NEW", hit ? 0 : 1);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS KVFILEENSURE|ENSUREKVFILE|DEFAULTKVFILE path key value [sep]
     * — set key=val in plate file only if key missing (file DEFAULT).
     * Existing key → leave value; NEW=0 HIT=1; LAST = existing line.
     * Missing key / file → write default; NEW=1 HIT=0.
     * LAST_N = NEW; default sep "=". Soft empty path → OK=0.
     * Usability: agent config defaults without LOOKUPFILE+IF+KVFILESET glue. */
    if (kw(&L->cur,"KVFILEENSURE") || kw(&L->cur,"ENSUREKVFILE") ||
        kw(&L->cur,"DEFAULTKVFILE") || kw(&L->cur,"KVFILEDEFAULT") ||
        kw(&L->cur,"PLATEENSURE") || kw(&L->cur,"CONFIGENSURE") ||
        kw(&L->cur,"ENSUREFILEKV") || kw(&L->cur,"FILEENSUREKV") ||
        kw(&L->cur,"PUTKVIF") || kw(&L->cur,"SETKVIFMISS") ||
        kw(&L->cur,"KVFILEIFMISS") || kw(&L->cur,"IFNOTKVFILE")){
      char path[512], key[256], val[512], sep[32], a[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX], newline[768], field[512], left[256];
      char existing[768], exist_val[512];
      const char *p, *start;
      size_t flen, kn, sn, vn, olen = 0, left_n;
      long idx = 0, kept = 0, hit = 0, found_i = -1;
      int first = 1;
      cubalc_host_result hr, wr;
      lex_next(L);
      path[0] = 0; key[0] = 0; val[0] = 0; out[0] = 0; a[0] = 0;
      newline[0] = 0; existing[0] = 0; exist_val[0] = 0;
      snprintf(sep, sizeof sep, "%s", "=");
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS KVFILEENSURE path key value [sep]");
        return -1;
      }
      snprintf(path, sizeof path, "%s", a);
      if (resolve_str_arg(vm, L, key, sizeof key) != 0) key[0] = 0;
      if (resolve_str_arg(vm, L, val, sizeof val) != 0) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long n = parse_expr(vm, L);
          snprintf(val, sizeof val, "%ld", n);
        } else {
          val[0] = 0;
        }
      }
      {
        size_t i = 0;
        while (val[i] && val[i] != '\n' && val[i] != '\r' && i + 1 < sizeof val) i++;
        val[i] = 0;
      }
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", "=");
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && strlen(L->cur.text) <= 2) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        lex_next(L);
      }
      kn = strlen(key);
      sn = strlen(sep);
      vn = strlen(val);
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "KVFILEENSURE_N", 0);
        var_set_num(vm, "KVFILEENSURE_I", -1);
        var_set_num(vm, "KVFILEENSURE_HIT", 0);
        var_set_num(vm, "KVFILEENSURE_NEW", 0);
        var_set_num(vm, "KVFILEENSURE_TOTAL", 0);
        var_set_str(vm, "KVFILEENSURE_V", "");
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "KVFILEENSURE: empty path");
        var_set_str(vm, "ERR", "KVFILEENSURE: empty path");
        bump(vm); return 1;
      }
      if (cubalc_host_read(path, &hr) != 0) {
        /* missing file → create with default */
        if (sn == 0)
          snprintf(newline, sizeof newline, "%s", key[0] ? key : val);
        else
          snprintf(newline, sizeof newline, "%s%s%s", key, sep, val);
        if (cubalc_host_write(path, newline, &wr) != 0) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "KVFILEENSURE_N", 0);
          var_set_num(vm, "KVFILEENSURE_I", -1);
          var_set_num(vm, "KVFILEENSURE_HIT", 0);
          var_set_num(vm, "KVFILEENSURE_NEW", 0);
          var_set_num(vm, "KVFILEENSURE_TOTAL", 0);
          var_set_str(vm, "KVFILEENSURE_V", "");
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "KVFILEENSURE: write fail");
          var_set_str(vm, "ERR", wr.err[0] ? wr.err : "KVFILEENSURE: write fail");
          bump(vm); return 1;
        }
        var_set_str(vm, "LAST", newline);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", newline);
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_str(vm, "KVFILEENSURE", newline);
        var_set_str(vm, "ENSUREKVFILE", newline);
        var_set_str(vm, "DEFAULTKVFILE", newline);
        var_set_str(vm, "KVFILEENSURE_V", val);
        var_set_num(vm, "KVFILEENSURE_N", 1);
        var_set_num(vm, "KVFILEENSURE_I", 0);
        var_set_num(vm, "KVFILEENSURE_HIT", 0);
        var_set_num(vm, "KVFILEENSURE_NEW", 1);
        var_set_num(vm, "KVFILEENSURE_TOTAL", 1);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      p = hr.str;
      if (*p) {
        while (*p) {
          int match = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            if (sn == 0) {
              match = (strcmp(field, key) == 0);
            } else {
              const char *sp = strstr(field, sep);
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
              } else {
                left_n = take;
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
              }
              match = (strcmp(left, key) == 0);
            }
            if (match && found_i < 0) {
              found_i = idx;
              hit = 1;
              snprintf(existing, sizeof existing, "%s", field);
              if (sn == 0) {
                snprintf(exist_val, sizeof exist_val, "%s", field);
              } else {
                const char *sp = strstr(field, sep);
                if (sp) {
                  snprintf(exist_val, sizeof exist_val, "%s", sp + sn);
                } else {
                  exist_val[0] = 0;
                }
              }
            }
            /* always keep original lines (no rewrite of existing keys) */
            {
              size_t nlen = strlen(field);
              if (!first && olen + 1 < sizeof out) out[olen++] = '\n';
              first = 0;
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, field, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, field, t);
                olen += t;
              }
              out[olen] = 0;
            }
          }
          kept++;
          idx++;
          if (*p == '\n') p++;
        }
      }
      if (hit) {
        /* key already present — leave file untouched */
        var_set_str(vm, "LAST", existing);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", existing);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "KVFILEENSURE", existing);
        var_set_str(vm, "ENSUREKVFILE", existing);
        var_set_str(vm, "DEFAULTKVFILE", existing);
        var_set_str(vm, "KVFILEENSURE_V", exist_val);
        var_set_num(vm, "KVFILEENSURE_N", 0);
        var_set_num(vm, "KVFILEENSURE_I", found_i);
        var_set_num(vm, "KVFILEENSURE_HIT", 1);
        var_set_num(vm, "KVFILEENSURE_NEW", 0);
        var_set_num(vm, "KVFILEENSURE_TOTAL", kept);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      /* append default */
      if (sn == 0)
        snprintf(newline, sizeof newline, "%s", key[0] ? key : val);
      else
        snprintf(newline, sizeof newline, "%s%s%s", key, sep, val);
      {
        size_t nlen = strlen(newline);
        if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        if (olen + nlen < sizeof out) {
          memcpy(out + olen, newline, nlen);
          olen += nlen;
        } else if (olen < sizeof out - 1) {
          size_t t = sizeof out - 1 - olen;
          memcpy(out + olen, newline, t);
          olen += t;
        }
        out[olen] = 0;
      }
      found_i = kept;
      kept++;
      if (cubalc_host_write(path, out, &wr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "KVFILEENSURE_N", 0);
        var_set_num(vm, "KVFILEENSURE_I", found_i);
        var_set_num(vm, "KVFILEENSURE_HIT", 0);
        var_set_num(vm, "KVFILEENSURE_NEW", 0);
        var_set_num(vm, "KVFILEENSURE_TOTAL", kept - 1);
        var_set_str(vm, "KVFILEENSURE_V", "");
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "KVFILEENSURE: write fail");
        var_set_str(vm, "ERR", wr.err[0] ? wr.err : "KVFILEENSURE: write fail");
        bump(vm); return 1;
      }
      (void)kn; (void)vn;
      var_set_str(vm, "LAST", newline);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", newline);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_str(vm, "KVFILEENSURE", newline);
      var_set_str(vm, "ENSUREKVFILE", newline);
      var_set_str(vm, "DEFAULTKVFILE", newline);
      var_set_str(vm, "KVFILEENSURE_V", val);
      var_set_num(vm, "KVFILEENSURE_N", 1);
      var_set_num(vm, "KVFILEENSURE_I", found_i);
      var_set_num(vm, "KVFILEENSURE_HIT", 0);
      var_set_num(vm, "KVFILEENSURE_NEW", 1);
      var_set_num(vm, "KVFILEENSURE_TOTAL", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS KVFILEDEL|FILEKVDEL|DELKVFILE path key [sep]
     * — drop first key=val line from a plate file (read-modify-write).
     * Dual of KVFILESET / file KVDEL. Default sep "=".
     * LAST = dropped value when hit; KVFILEDEL_HIT 0|1 · I · TOTAL(after) · V.
     * Soft key miss → HIT=0 OK=1 file unchanged. Soft path miss → OK=0.
     * Usability: ack/remove plate status without READ+KVDEL+WRITE glue. */
    if (kw(&L->cur,"KVFILEDEL") || kw(&L->cur,"FILEKVDEL") ||
        kw(&L->cur,"DELKVFILE") || kw(&L->cur,"RMKVFILE") ||
        kw(&L->cur,"UNSETKVFILE") || kw(&L->cur,"PLATEDEL") ||
        kw(&L->cur,"CONFIGDEL") || kw(&L->cur,"FILEDELKV") ||
        kw(&L->cur,"DROPKVFILE") || kw(&L->cur,"KVFILEDROP")){
      char path[512], key[256], sep[32], a[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX], field[512], left[256], dropped_val[512];
      char dropped_line[512];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long idx = 0, kept = 0, hit = 0, found_i = -1;
      int first_kept = 1;
      cubalc_host_result hr, wr;
      lex_next(L);
      path[0] = 0; key[0] = 0; out[0] = 0; a[0] = 0;
      dropped_val[0] = 0; dropped_line[0] = 0;
      snprintf(sep, sizeof sep, "%s", "=");
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS KVFILEDEL path key [sep]");
        return -1;
      }
      snprintf(path, sizeof path, "%s", a);
      if (resolve_str_arg(vm, L, key, sizeof key) != 0) key[0] = 0;
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", "=");
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && strlen(L->cur.text) <= 2) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        lex_next(L);
      }
      sn = strlen(sep);
      if (!path[0] || cubalc_host_read(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "KVFILEDEL_N", 0);
        var_set_num(vm, "KVFILEDEL_I", -1);
        var_set_num(vm, "KVFILEDEL_HIT", 0);
        var_set_num(vm, "KVFILEDEL_TOTAL", 0);
        var_set_str(vm, "KVFILEDEL_V", "");
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", path[0] ? "KVFILEDEL: read fail" : "KVFILEDEL: empty path");
        var_set_str(vm, "ERR", path[0] ? "KVFILEDEL: read fail" : "KVFILEDEL: empty path");
        bump(vm); return 1;
      }
      p = hr.str;
      if (*p) {
        while (*p) {
          int match = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *sp = sn ? strstr(field, sep) : NULL;
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
                match = (strcmp(left, key) == 0);
                if (match && found_i < 0) {
                  size_t vn = strlen(sp + sn);
                  if (vn >= sizeof dropped_val) vn = sizeof dropped_val - 1;
                  memcpy(dropped_val, sp + sn, vn);
                  dropped_val[vn] = 0;
                }
              } else if (sn == 0) {
                match = (strcmp(field, key) == 0);
                if (match && found_i < 0)
                  snprintf(dropped_val, sizeof dropped_val, "%s", field);
              } else {
                match = (strcmp(field, key) == 0);
                if (match && found_i < 0) dropped_val[0] = 0;
              }
            }
            if (match && found_i < 0) {
              found_i = idx;
              hit = 1;
              snprintf(dropped_line, sizeof dropped_line, "%s", field);
            } else {
              size_t nlen = strlen(field);
              if (!first_kept && olen + 1 < sizeof out) out[olen++] = '\n';
              first_kept = 0;
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, field, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, field, t);
                olen += t;
              }
              out[olen] = 0;
              kept++;
            }
          }
          idx++;
          if (*p == '\n') p++;
        }
      }
      if (!hit) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "KVFILEDEL_N", 0);
        var_set_num(vm, "KVFILEDEL_I", -1);
        var_set_num(vm, "KVFILEDEL_HIT", 0);
        var_set_num(vm, "KVFILEDEL_TOTAL", kept);
        var_set_str(vm, "KVFILEDEL_V", "");
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      if (cubalc_host_write(path, out, &wr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "KVFILEDEL_N", 0);
        var_set_num(vm, "KVFILEDEL_I", found_i);
        var_set_num(vm, "KVFILEDEL_HIT", 0);
        var_set_num(vm, "KVFILEDEL_TOTAL", kept + 1);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "KVFILEDEL: write fail");
        var_set_str(vm, "ERR", wr.err[0] ? wr.err : "KVFILEDEL: write fail");
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", dropped_val);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", dropped_val);
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_str(vm, "KVFILEDEL", dropped_val);
      var_set_str(vm, "FILEKVDEL", dropped_val);
      var_set_str(vm, "KVFILEDEL_V", dropped_val);
      var_set_str(vm, "KVFILEDEL_LINE", dropped_line);
      var_set_num(vm, "KVFILEDEL_N", hit);
      var_set_num(vm, "KVFILEDEL_I", found_i);
      var_set_num(vm, "KVFILEDEL_HIT", hit);
      var_set_num(vm, "KVFILEDEL_TOTAL", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS KVFILEINC|INCKVFILE|FILEKVINC path key [delta] [sep]
     * — add delta (default 1) to numeric key=val in plate file.
     * Dual of bag KVINC for on-disk counters. Default sep "=".
     * Missing key → insert key=delta. Missing file → create plate.
     * LAST = key=newval line; KVFILEINC_V = new value · HIT · I · TOTAL.
     * Usability: retry/severity plate counters without LOOKUPFILE+arith+KVFILESET. */
    if (kw(&L->cur,"KVFILEINC") || kw(&L->cur,"INCKVFILE") ||
        kw(&L->cur,"FILEKVINC") || kw(&L->cur,"BUMPKVFILE") ||
        kw(&L->cur,"PLATEINC") || kw(&L->cur,"CONFIGINC") ||
        kw(&L->cur,"FILEINCKV") || kw(&L->cur,"KVFILEBUMP") ||
        kw(&L->cur,"INCKEYFILE") || kw(&L->cur,"COUNTUPFILE")){
      char path[512], key[256], sep[32], a[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX], field[512], left[256], newline[768], vbuf[40];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long idx = 0, kept = 0, hit = 0, found_i = -1, delta = 1, newv = 0;
      int first = 1, created = 0;
      cubalc_host_result hr, wr;
      lex_next(L);
      path[0] = 0; key[0] = 0; out[0] = 0; a[0] = 0; newline[0] = 0; vbuf[0] = 0;
      snprintf(sep, sizeof sep, "%s", "=");
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS KVFILEINC path key [delta] [sep]");
        return -1;
      }
      snprintf(path, sizeof path, "%s", a);
      if (resolve_str_arg(vm, L, key, sizeof key) != 0) key[0] = 0;
      /* optional delta then optional sep */
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        delta = parse_expr(vm, L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *dv = var_get(vm, L->cur.text, 0);
        if (dv && !dv->is_str) {
          delta = (long)dv->val;
          lex_next(L);
        }
      }
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", "=");
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && strlen(L->cur.text) <= 2) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        lex_next(L);
      }
      sn = strlen(sep);
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "KVFILEINC_N", 0);
        var_set_num(vm, "KVFILEINC_I", -1);
        var_set_num(vm, "KVFILEINC_HIT", 0);
        var_set_num(vm, "KVFILEINC_V", 0);
        var_set_num(vm, "KVFILEINC_TOTAL", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "KVFILEINC: empty path");
        var_set_str(vm, "ERR", "KVFILEINC: empty path");
        bump(vm); return 1;
      }
      if (cubalc_host_read(path, &hr) != 0) {
        created = 1;
        hr.str[0] = 0;
      }
      p = hr.str;
      if (*p) {
        while (*p) {
          int match = 0;
          long oldv = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *sp = sn ? strstr(field, sep) : NULL;
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
                match = (strcmp(left, key) == 0);
                if (match) {
                  char *end = 0;
                  oldv = strtol(sp + sn, &end, 10);
                }
              } else if (sn == 0) {
                match = (strcmp(field, key) == 0);
              } else {
                match = (strcmp(field, key) == 0);
                if (match) oldv = 0;
              }
            }
            if (match && found_i < 0) {
              newv = oldv + delta;
              snprintf(vbuf, sizeof vbuf, "%ld", newv);
              if (sn == 0)
                snprintf(newline, sizeof newline, "%s", vbuf);
              else
                snprintf(newline, sizeof newline, "%s%s%s", key, sep, vbuf);
              found_i = idx;
              hit = 1;
            } else {
              snprintf(newline, sizeof newline, "%s", field);
            }
          }
          {
            size_t nlen = strlen(newline);
            if (!first && olen + 1 < sizeof out) out[olen++] = '\n';
            first = 0;
            if (olen + nlen < sizeof out) {
              memcpy(out + olen, newline, nlen);
              olen += nlen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, newline, t);
              olen += t;
            }
            out[olen] = 0;
          }
          kept++;
          idx++;
          if (*p == '\n') p++;
        }
      }
      if (!hit) {
        newv = delta;
        snprintf(vbuf, sizeof vbuf, "%ld", newv);
        if (sn == 0)
          snprintf(newline, sizeof newline, "%s", key[0] ? key : vbuf);
        else
          snprintf(newline, sizeof newline, "%s%s%s", key, sep, vbuf);
        {
          size_t nlen = strlen(newline);
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + nlen < sizeof out) {
            memcpy(out + olen, newline, nlen);
            olen += nlen;
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            memcpy(out + olen, newline, t);
            olen += t;
          }
          out[olen] = 0;
        }
        found_i = kept;
        kept++;
      } else {
        snprintf(vbuf, sizeof vbuf, "%ld", newv);
        if (sn == 0)
          snprintf(newline, sizeof newline, "%s", vbuf);
        else
          snprintf(newline, sizeof newline, "%s%s%s", key, sep, vbuf);
      }
      if (cubalc_host_write(path, out, &wr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "KVFILEINC_N", 0);
        var_set_num(vm, "KVFILEINC_I", found_i);
        var_set_num(vm, "KVFILEINC_HIT", hit);
        var_set_num(vm, "KVFILEINC_V", newv);
        var_set_num(vm, "KVFILEINC_TOTAL", kept);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "KVFILEINC: write fail");
        var_set_str(vm, "ERR", wr.err[0] ? wr.err : "KVFILEINC: write fail");
        bump(vm); return 1;
      }
      (void)created;
      var_set_str(vm, "LAST", newline);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", newline);
      vm->last_n = newv;
      var_set_num(vm, "LAST_N", newv);
      var_set_str(vm, "KVFILEINC", newline);
      var_set_str(vm, "FILEKVINC", newline);
      var_set_str(vm, "INCKVFILE", newline);
      var_set_num(vm, "KVFILEINC_N", kept);
      var_set_num(vm, "KVFILEINC_I", found_i);
      var_set_num(vm, "KVFILEINC_HIT", hit);
      var_set_num(vm, "KVFILEINC_V", newv);
      var_set_num(vm, "KVFILEINC_TOTAL", kept);
      var_set_num(vm, "KVFILEINC_DELTA", delta);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS ENSURELINE|ENSUREFILELINE|ADDIFMISSING path line
     * — append line if exact file line not already present (idempotent).
     * Strip line to first newline. Missing file → create with that line.
     * LAST = line; ENSURELINE_NEW 1=appended 0=already; I · TOTAL.
     * Soft empty path → OK=0.
     * Usability: config flags/markers/INCLUDE guards without HASLINE+APPEND glue. */
    if (kw(&L->cur,"ENSURELINE") || kw(&L->cur,"ENSUREFILELINE") ||
        kw(&L->cur,"ADDIFMISSING") || kw(&L->cur,"FILEENSURE") ||
        kw(&L->cur,"ENSURELN") || kw(&L->cur,"ADDLINEIF") ||
        kw(&L->cur,"IFNOTLINE") || kw(&L->cur,"FILEADDIF") ||
        kw(&L->cur,"ENSUREEXACT") || kw(&L->cur,"APPENDIFNEW")){
      char path[512], line[CUBALC_HOST_STR_MAX], clean[CUBALC_HOST_STR_MAX];
      char a[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t flen, clen;
      long idx = 0, total = 0, found_i = -1, hit = 0;
      cubalc_host_result hr;
      lex_next(L);
      path[0] = 0; line[0] = 0; clean[0] = 0; a[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS ENSURELINE path line");
        return -1;
      }
      snprintf(path, sizeof path, "%s", a);
      if (resolve_str_arg(vm, L, line, sizeof line) != 0) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long n = parse_expr(vm, L);
          snprintf(line, sizeof line, "%ld", n);
        } else {
          line[0] = 0;
        }
      }
      {
        size_t i = 0;
        while (line[i] && line[i] != '\n' && line[i] != '\r' && i + 1 < sizeof clean) {
          clean[i] = line[i];
          i++;
        }
        clean[i] = 0;
      }
      clen = strlen(clean);
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "ENSURELINE_N", 0);
        var_set_num(vm, "ENSURELINE_NEW", 0);
        var_set_num(vm, "ENSURELINE_I", -1);
        var_set_num(vm, "ENSURELINE_TOTAL", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "ENSURELINE: empty path");
        var_set_str(vm, "ERR", "ENSURELINE: empty path");
        bump(vm); return 1;
      }
      if (cubalc_host_read(path, &hr) != 0) {
        /* create with single line */
        FILE *af = fopen(path, "w");
        if (!af) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "ENSURELINE_N", 0);
          var_set_num(vm, "ENSURELINE_NEW", 0);
          var_set_num(vm, "ENSURELINE_I", -1);
          var_set_num(vm, "ENSURELINE_TOTAL", 0);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", "ENSURELINE: create fail");
          var_set_str(vm, "ERR", "ENSURELINE: create fail");
          bump(vm); return 1;
        }
        fputs(clean, af);
        fputc('\n', af);
        fclose(af);
        var_set_str(vm, "LAST", clean);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", clean);
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_str(vm, "ENSURELINE", clean);
        var_set_str(vm, "ENSUREFILELINE", clean);
        var_set_num(vm, "ENSURELINE_N", 1);
        var_set_num(vm, "ENSURELINE_NEW", 1);
        var_set_num(vm, "ENSURELINE_I", 0);
        var_set_num(vm, "ENSURELINE_TOTAL", 1);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      p = hr.str;
      if (*p) {
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (found_i < 0 && flen == clen &&
              (clen == 0 || memcmp(start, clean, clen) == 0)) {
            found_i = idx;
            hit = 1;
          }
          total++;
          idx++;
          if (*p == '\n') p++;
        }
      }
      if (hit) {
        var_set_str(vm, "LAST", clean);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", clean);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "ENSURELINE", clean);
        var_set_str(vm, "ENSUREFILELINE", clean);
        var_set_num(vm, "ENSURELINE_N", 0);
        var_set_num(vm, "ENSURELINE_NEW", 0);
        var_set_num(vm, "ENSURELINE_I", found_i);
        var_set_num(vm, "ENSURELINE_TOTAL", total);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      {
        FILE *af = fopen(path, "a");
        if (!af) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "ENSURELINE_N", 0);
          var_set_num(vm, "ENSURELINE_NEW", 0);
          var_set_num(vm, "ENSURELINE_I", -1);
          var_set_num(vm, "ENSURELINE_TOTAL", total);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", "ENSURELINE: append fail");
          var_set_str(vm, "ERR", "ENSURELINE: append fail");
          bump(vm); return 1;
        }
        /* if existing body lacks trailing newline, insert one so line stays atomic */
        if (total > 0) {
          size_t blen = strlen(hr.str);
          if (blen > 0 && hr.str[blen - 1] != '\n')
            fputc('\n', af);
        }
        fputs(clean, af);
        fputc('\n', af);
        fclose(af);
      }
      var_set_str(vm, "LAST", clean);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", clean);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_str(vm, "ENSURELINE", clean);
      var_set_str(vm, "ENSUREFILELINE", clean);
      var_set_num(vm, "ENSURELINE_N", 1);
      var_set_num(vm, "ENSURELINE_NEW", 1);
      var_set_num(vm, "ENSURELINE_I", total);
      var_set_num(vm, "ENSURELINE_TOTAL", total + 1);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS HASFILELINE|FILEHASLINE|HASLINEF path line
     * — exact file line membership probe (dual of bag HASLINE).
     * LAST_N / HASFILELINE_N = 0|1; I = first index (-1 miss); TOTAL lines.
     * LAST = line when hit (empty on miss). Soft path miss → N=0 OK=1 (probe).
     * Usability: IF guards before ENSURELINE/DROP without GREPFILE/READ glue. */
    if (kw(&L->cur,"HASFILELINE") || kw(&L->cur,"FILEHASLINE") ||
        kw(&L->cur,"HASLINEF") || kw(&L->cur,"FILEHAS") ||
        kw(&L->cur,"HASLINEFILE") || kw(&L->cur,"CONTAINSLINE") ||
        kw(&L->cur,"FILECONTAINSLINE") || kw(&L->cur,"LINEINFILE") ||
        kw(&L->cur,"HASFILELINEI") || kw(&L->cur,"FILEHASLINEI") ||
        kw(&L->cur,"HASLINEFI") || kw(&L->cur,"CONTAINSLINEI")){
      char path[512], needle[CUBALC_HOST_STR_MAX], clean[CUBALC_HOST_STR_MAX];
      char a[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t flen, clen;
      long idx = 0, total = 0, found_i = -1, hit = 0;
      int icase = 0;
      cubalc_host_result hr;
      if (kw(&L->cur,"HASFILELINEI") || kw(&L->cur,"FILEHASLINEI") ||
          kw(&L->cur,"HASLINEFI") || kw(&L->cur,"CONTAINSLINEI"))
        icase = 1;
      lex_next(L);
      path[0] = 0; needle[0] = 0; clean[0] = 0; a[0] = 0;
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS HASFILELINE path line");
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
      {
        size_t i = 0;
        while (needle[i] && needle[i] != '\n' && needle[i] != '\r' && i + 1 < sizeof clean) {
          clean[i] = needle[i];
          i++;
        }
        clean[i] = 0;
      }
      clen = strlen(clean);
      if (!path[0] || cubalc_host_read(path, &hr) != 0) {
        /* soft probe: missing path → not present, OK=1 */
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "HASFILELINE_N", 0);
        var_set_num(vm, "HASFILELINE_I", -1);
        var_set_num(vm, "HASFILELINE_TOTAL", 0);
        var_set_num(vm, "OK", path[0] ? 1 : 0);
        if (!path[0]) {
          var_set_str(vm, "LAST_ERR", "HASFILELINE: empty path");
          var_set_str(vm, "ERR", "HASFILELINE: empty path");
        }
        bump(vm); return 1;
      }
      p = hr.str;
      if (*p) {
        while (*p) {
          int match = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (found_i < 0) {
            if (!icase) {
              match = (flen == clen && (clen == 0 || memcmp(start, clean, clen) == 0));
            } else if (flen == clen) {
              size_t j;
              match = 1;
              for (j = 0; j < clen; j++) {
                char a2 = start[j], b2 = clean[j];
                if (a2 >= 'A' && a2 <= 'Z') a2 = (char)(a2 - 'A' + 'a');
                if (b2 >= 'A' && b2 <= 'Z') b2 = (char)(b2 - 'A' + 'a');
                if (a2 != b2) { match = 0; break; }
              }
            }
            if (match) {
              found_i = idx;
              hit = 1;
            }
          }
          total++;
          idx++;
          if (*p == '\n') p++;
        }
      }
      if (hit) {
        var_set_str(vm, "LAST", clean);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", clean);
      } else {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
      }
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_str(vm, "HASFILELINE", hit ? clean : "");
      var_set_str(vm, "FILEHASLINE", hit ? clean : "");
      var_set_num(vm, "HASFILELINE_N", hit);
      var_set_num(vm, "HASFILELINE_I", found_i);
      var_set_num(vm, "HASFILELINE_TOTAL", total);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS DROPEXACTFILE|DROPFILEEXACT|REMOVELINEFILE path line
     * — drop first exact file line (dual of bag REMOVELINE / file HASFILELINE).
     * Exact match only (substring DROPMATCHFILE is separate). DROPEXACTFILEI icase.
     * LAST = dropped line when hit; DROPEXACTFILE_N 0|1 · I · TOTAL(after).
     * Soft no-match → N=0 OK=1 file unchanged. Soft path miss → OK=0.
     * Usability: drop config flag/marker without substring false positives or HAS+DROP glue. */
    if (kw(&L->cur,"DROPEXACTFILE") || kw(&L->cur,"DROPFILEEXACT") ||
        kw(&L->cur,"REMOVELINEFILE") || kw(&L->cur,"FILEREMOVELINE") ||
        kw(&L->cur,"DROPFILELINEEXACT") || kw(&L->cur,"FILEDROPEXACT") ||
        kw(&L->cur,"DELEXACTLINE") || kw(&L->cur,"REMOVELINEF") ||
        kw(&L->cur,"DROPEXACTFILEI") || kw(&L->cur,"DROPFILEEXACTI") ||
        kw(&L->cur,"REMOVELINEFILEI") || kw(&L->cur,"FILEREMOVELINEI") ||
        kw(&L->cur,"FILEDROPEXACTI") || kw(&L->cur,"DELEXACTLINEI")){
      char path[512], needle[CUBALC_HOST_STR_MAX], clean[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX], dropped[CUBALC_HOST_STR_MAX], a[CUBALC_HOST_STR_MAX];
      const char *lp, *ls;
      size_t llen, clen, o = 0;
      long total = 0, seen = 0, found_i = -1, hit = 0;
      int icase = 0, first_kept = 1;
      cubalc_host_result hr, wr;
      if (kw(&L->cur,"DROPEXACTFILEI") || kw(&L->cur,"DROPFILEEXACTI") ||
          kw(&L->cur,"REMOVELINEFILEI") || kw(&L->cur,"FILEREMOVELINEI") ||
          kw(&L->cur,"FILEDROPEXACTI") || kw(&L->cur,"DELEXACTLINEI"))
        icase = 1;
      lex_next(L);
      path[0] = 0; needle[0] = 0; clean[0] = 0; out[0] = 0; dropped[0] = 0; a[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS DROPEXACTFILE path line");
        return -1;
      }
      snprintf(path, sizeof path, "%s", a);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long n = parse_expr(vm, L);
          snprintf(needle, sizeof needle, "%ld", n);
        } else {
          needle[0] = 0;
        }
      }
      {
        size_t i = 0;
        while (needle[i] && needle[i] != '\n' && needle[i] != '\r' && i + 1 < sizeof clean) {
          clean[i] = needle[i];
          i++;
        }
        clean[i] = 0;
      }
      clen = strlen(clean);
      if (!path[0] || cubalc_host_read(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DROPEXACTFILE_N", 0);
        var_set_num(vm, "DROPEXACTFILE_I", -1);
        var_set_num(vm, "DROPEXACTFILE_TOTAL", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", path[0] ? "DROPEXACTFILE: read fail" : "DROPEXACTFILE: empty path");
        var_set_str(vm, "ERR", path[0] ? "DROPEXACTFILE: read fail" : "DROPEXACTFILE: empty path");
        bump(vm); return 1;
      }
      lp = hr.str;
      total = 0;
      seen = 0;
      found_i = -1;
      if (*lp) {
        while (*lp) {
          int match = 0;
          ls = lp;
          while (*lp && *lp != '\n') lp++;
          llen = (size_t)(lp - ls);
          if (found_i < 0) {
            if (!icase) {
              match = (llen == clen && (clen == 0 || memcmp(ls, clean, clen) == 0));
            } else if (llen == clen) {
              size_t j;
              match = 1;
              for (j = 0; j < clen; j++) {
                char a2 = ls[j], b2 = clean[j];
                if (a2 >= 'A' && a2 <= 'Z') a2 = (char)(a2 - 'A' + 'a');
                if (b2 >= 'A' && b2 <= 'Z') b2 = (char)(b2 - 'A' + 'a');
                if (a2 != b2) { match = 0; break; }
              }
            }
            if (match) found_i = seen;
          }
          total++;
          seen++;
          if (*lp == '\n') lp++;
        }
      }
      if (found_i < 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "DROPEXACTFILE_N", 0);
        var_set_num(vm, "DROPEXACTFILE_I", -1);
        var_set_num(vm, "DROPEXACTFILE_TOTAL", total);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      lp = hr.str;
      seen = 0;
      while (seen < total) {
        ls = lp;
        while (*lp && *lp != '\n') lp++;
        llen = (size_t)(lp - ls);
        if (seen == found_i) {
          hit = 1;
          if (llen >= sizeof dropped) llen = sizeof dropped - 1;
          memcpy(dropped, ls, llen);
          dropped[llen] = 0;
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
        var_set_num(vm, "DROPEXACTFILE_N", 0);
        var_set_num(vm, "DROPEXACTFILE_I", found_i);
        var_set_num(vm, "DROPEXACTFILE_TOTAL", total);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "DROPEXACTFILE: write fail");
        var_set_str(vm, "ERR", wr.err[0] ? wr.err : "DROPEXACTFILE: write fail");
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", dropped);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", dropped);
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_str(vm, "DROPEXACTFILE", dropped);
      var_set_str(vm, "DROPFILEEXACT", dropped);
      var_set_str(vm, "REMOVELINEFILE", dropped);
      var_set_num(vm, "DROPEXACTFILE_N", hit);
      var_set_num(vm, "DROPEXACTFILE_I", found_i);
      var_set_num(vm, "DROPEXACTFILE_TOTAL", total > 0 ? total - 1 : 0);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS TOGGLELINE|TOGGLEFILELINE|FLIPLINE path line
     * — flip exact file line presence: drop if present, else append.
     * TOGGLELINEI icase. Missing file → create with line (ON=1).
     * LAST = line; LAST_N / TOGGLELINE_ON = present after (0|1);
     * TOGGLELINE_WAS before; N=1 always on flip; I · TOTAL(after).
     * Soft empty path → OK=0.
     * Usability: feature flags/markers one-shot without HAS+ENSURE/DROP IF glue. */
    if (kw(&L->cur,"TOGGLELINE") || kw(&L->cur,"TOGGLEFILELINE") ||
        kw(&L->cur,"FLIPLINE") || kw(&L->cur,"FILETOGGLE") ||
        kw(&L->cur,"TOGGLELN") || kw(&L->cur,"FLIPFILELINE") ||
        kw(&L->cur,"TOGGLEEXACT") || kw(&L->cur,"FLIPFLAG") ||
        kw(&L->cur,"TOGGLELINEI") || kw(&L->cur,"TOGGLEFILELINEI") ||
        kw(&L->cur,"FLIPLINEI") || kw(&L->cur,"FILETOGGLEI") ||
        kw(&L->cur,"TOGGLELNI") || kw(&L->cur,"FLIPFILELINEI")){
      char path[512], needle[CUBALC_HOST_STR_MAX], clean[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX], a[CUBALC_HOST_STR_MAX];
      const char *lp, *ls;
      size_t llen, clen, o = 0;
      long total = 0, seen = 0, found_i = -1;
      int icase = 0, first_kept = 1;
      cubalc_host_result hr, wr;
      if (kw(&L->cur,"TOGGLELINEI") || kw(&L->cur,"TOGGLEFILELINEI") ||
          kw(&L->cur,"FLIPLINEI") || kw(&L->cur,"FILETOGGLEI") ||
          kw(&L->cur,"TOGGLELNI") || kw(&L->cur,"FLIPFILELINEI"))
        icase = 1;
      lex_next(L);
      path[0] = 0; needle[0] = 0; clean[0] = 0; out[0] = 0; a[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS TOGGLELINE path line");
        return -1;
      }
      snprintf(path, sizeof path, "%s", a);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long n = parse_expr(vm, L);
          snprintf(needle, sizeof needle, "%ld", n);
        } else {
          needle[0] = 0;
        }
      }
      {
        size_t i = 0;
        while (needle[i] && needle[i] != '\n' && needle[i] != '\r' && i + 1 < sizeof clean) {
          clean[i] = needle[i];
          i++;
        }
        clean[i] = 0;
      }
      clen = strlen(clean);
      if (!path[0]) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "TOGGLELINE_N", 0);
        var_set_num(vm, "TOGGLELINE_ON", 0);
        var_set_num(vm, "TOGGLELINE_WAS", 0);
        var_set_num(vm, "TOGGLELINE_I", -1);
        var_set_num(vm, "TOGGLELINE_TOTAL", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "TOGGLELINE: empty path");
        var_set_str(vm, "ERR", "TOGGLELINE: empty path");
        bump(vm); return 1;
      }
      if (cubalc_host_read(path, &hr) != 0) {
        /* missing file → create with line (ON) */
        FILE *af = fopen(path, "w");
        if (!af) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "TOGGLELINE_N", 0);
          var_set_num(vm, "TOGGLELINE_ON", 0);
          var_set_num(vm, "TOGGLELINE_WAS", 0);
          var_set_num(vm, "TOGGLELINE_I", -1);
          var_set_num(vm, "TOGGLELINE_TOTAL", 0);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", "TOGGLELINE: create fail");
          var_set_str(vm, "ERR", "TOGGLELINE: create fail");
          bump(vm); return 1;
        }
        fputs(clean, af);
        fputc('\n', af);
        fclose(af);
        var_set_str(vm, "LAST", clean);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", clean);
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_str(vm, "TOGGLELINE", clean);
        var_set_str(vm, "TOGGLEFILELINE", clean);
        var_set_str(vm, "FLIPLINE", clean);
        var_set_num(vm, "TOGGLELINE_N", 1);
        var_set_num(vm, "TOGGLELINE_ON", 1);
        var_set_num(vm, "TOGGLELINE_WAS", 0);
        var_set_num(vm, "TOGGLELINE_I", 0);
        var_set_num(vm, "TOGGLELINE_TOTAL", 1);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      lp = hr.str;
      total = 0;
      seen = 0;
      found_i = -1;
      if (*lp) {
        while (*lp) {
          int match = 0;
          ls = lp;
          while (*lp && *lp != '\n') lp++;
          llen = (size_t)(lp - ls);
          if (found_i < 0) {
            if (!icase) {
              match = (llen == clen && (clen == 0 || memcmp(ls, clean, clen) == 0));
            } else if (llen == clen) {
              size_t j;
              match = 1;
              for (j = 0; j < clen; j++) {
                char a2 = ls[j], b2 = clean[j];
                if (a2 >= 'A' && a2 <= 'Z') a2 = (char)(a2 - 'A' + 'a');
                if (b2 >= 'A' && b2 <= 'Z') b2 = (char)(b2 - 'A' + 'a');
                if (a2 != b2) { match = 0; break; }
              }
            }
            if (match) found_i = seen;
          }
          total++;
          seen++;
          if (*lp == '\n') lp++;
        }
      }
      if (found_i >= 0) {
        /* present → drop; LAST = actual file line (matters for icase) */
        char dropped[CUBALC_HOST_STR_MAX];
        dropped[0] = 0;
        lp = hr.str;
        seen = 0;
        o = 0;
        first_kept = 1;
        while (seen < total) {
          ls = lp;
          while (*lp && *lp != '\n') lp++;
          llen = (size_t)(lp - ls);
          if (seen == found_i) {
            size_t take = llen;
            if (take >= sizeof dropped) take = sizeof dropped - 1;
            memcpy(dropped, ls, take);
            dropped[take] = 0;
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
          var_set_num(vm, "TOGGLELINE_N", 0);
          var_set_num(vm, "TOGGLELINE_ON", 1);
          var_set_num(vm, "TOGGLELINE_WAS", 1);
          var_set_num(vm, "TOGGLELINE_I", found_i);
          var_set_num(vm, "TOGGLELINE_TOTAL", total);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", wr.err[0] ? wr.err : "TOGGLELINE: write fail");
          var_set_str(vm, "ERR", wr.err[0] ? wr.err : "TOGGLELINE: write fail");
          bump(vm); return 1;
        }
        var_set_str(vm, "LAST", dropped);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", dropped);
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "TOGGLELINE", dropped);
        var_set_str(vm, "TOGGLEFILELINE", dropped);
        var_set_str(vm, "FLIPLINE", dropped);
        var_set_num(vm, "TOGGLELINE_N", 1);
        var_set_num(vm, "TOGGLELINE_ON", 0);
        var_set_num(vm, "TOGGLELINE_WAS", 1);
        var_set_num(vm, "TOGGLELINE_I", found_i);
        var_set_num(vm, "TOGGLELINE_TOTAL", total > 0 ? total - 1 : 0);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      /* absent → append */
      {
        FILE *af = fopen(path, "a");
        if (!af) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "TOGGLELINE_N", 0);
          var_set_num(vm, "TOGGLELINE_ON", 0);
          var_set_num(vm, "TOGGLELINE_WAS", 0);
          var_set_num(vm, "TOGGLELINE_I", -1);
          var_set_num(vm, "TOGGLELINE_TOTAL", total);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", "TOGGLELINE: append fail");
          var_set_str(vm, "ERR", "TOGGLELINE: append fail");
          bump(vm); return 1;
        }
        if (total > 0) {
          size_t blen = strlen(hr.str);
          if (blen > 0 && hr.str[blen - 1] != '\n')
            fputc('\n', af);
        }
        fputs(clean, af);
        fputc('\n', af);
        fclose(af);
      }
      var_set_str(vm, "LAST", clean);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", clean);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_str(vm, "TOGGLELINE", clean);
      var_set_str(vm, "TOGGLEFILELINE", clean);
      var_set_str(vm, "FLIPLINE", clean);
      var_set_num(vm, "TOGGLELINE_N", 1);
      var_set_num(vm, "TOGGLELINE_ON", 1);
      var_set_num(vm, "TOGGLELINE_WAS", 0);
      var_set_num(vm, "TOGGLELINE_I", total);
      var_set_num(vm, "TOGGLELINE_TOTAL", total + 1);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
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
    /* SYS CWD|PWD — process working directory → LAST
     * SYS STATE — CUBALC_STATE plate dir (default state) → LAST
     * SYS ROOT — CUBALC_ROOT or cwd (INCLUDE root) → LAST
     * Usability: portable scripts locate layout without shell glue. */
    if (kw(&L->cur,"CWD") || kw(&L->cur,"PWD") || kw(&L->cur,"WORKDIR")){
      char cwd[512];
      lex_next(L);
      if (!getcwd(cwd, sizeof cwd)) {
        snprintf(cwd, sizeof cwd, ".");
        var_set_num(vm, "OK", 0);
      } else {
        var_set_num(vm, "OK", 1);
      }
      snprintf(vm->last_str, sizeof vm->last_str, "%s", cwd);
      vm->last_n = (long)strlen(cwd);
      var_set_str(vm, "LAST", cwd);
      var_set_str(vm, "CWD", cwd);
      var_set_num(vm, "LAST_N", vm->last_n);
      bump(vm); return 1;
    }
    /* SYS PUSHD|PUSHDIR path — save cwd then chdir → LAST = new cwd.
     * LAST_N = stack depth after push. Soft miss OK=0 sticky LAST_ERR.
     * SYS POPD|POPDIR — restore previous cwd → LAST = restored; LAST_N = depth.
     * SYS DIRSTACK|DIRS — newline bag of saved dirs (bottom→top); LAST_N = depth.
     * Usability: temp relative plate I/O without losing base (no shell pushd). */
    if (kw(&L->cur,"PUSHD") || kw(&L->cur,"PUSHDIR") || kw(&L->cur,"ENTERDIR") ||
        kw(&L->cur,"PUSHCWD") || kw(&L->cur,"CDPUSH") ||
        kw(&L->cur,"POPD") || kw(&L->cur,"POPDIR") || kw(&L->cur,"LEAVEDIR") ||
        kw(&L->cur,"POPCWD") || kw(&L->cur,"CDBACK") || kw(&L->cur,"BACKDIR") ||
        kw(&L->cur,"DIRSTACK") || kw(&L->cur,"DIRS") || kw(&L->cur,"DIR_STACK") ||
        kw(&L->cur,"CWDSTACK") || kw(&L->cur,"STACKDIRS")){
      char op[24], path[512];
      cubalc_host_result hr;
      int is_pop, is_list;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_pop = (strcmp(op, "POPD") == 0 || strcmp(op, "POPDIR") == 0 ||
                strcmp(op, "LEAVEDIR") == 0 || strcmp(op, "POPCWD") == 0 ||
                strcmp(op, "CDBACK") == 0 || strcmp(op, "BACKDIR") == 0);
      is_list = (strcmp(op, "DIRSTACK") == 0 || strcmp(op, "DIRS") == 0 ||
                 strcmp(op, "DIR_STACK") == 0 || strcmp(op, "CWDSTACK") == 0 ||
                 strcmp(op, "STACKDIRS") == 0);
      memset(&hr, 0, sizeof hr);
      lex_next(L);
      if (is_list) {
        if (cubalc_host_dirstack(&hr) != 0) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "DIRSTACK_N", 0);
          var_set_num(vm, "OK", 0);
          bump(vm); return 1;
        }
        var_set_str(vm, "LAST", hr.str);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
        vm->last_n = hr.n;
        var_set_num(vm, "LAST_N", hr.n);
        var_set_num(vm, "DIRSTACK_N", hr.n);
        var_set_str(vm, "DIRSTACK", hr.str);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      if (is_pop) {
        if (cubalc_host_popd(&hr) != 0) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "POPD_N", 0);
          var_set_num(vm, "DIRSTACK_N", 0);
          var_set_num(vm, "OK", 0);
          if (hr.err[0]) {
            var_set_str(vm, "LAST_ERR", hr.err);
            var_set_str(vm, "ERR", hr.err);
          } else {
            var_set_str(vm, "LAST_ERR", "POPD: empty stack");
            var_set_str(vm, "ERR", "POPD: empty stack");
          }
          bump(vm); return 1;
        }
        var_set_str(vm, "LAST", hr.str);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
        vm->last_n = hr.n;
        var_set_num(vm, "LAST_N", hr.n);
        var_set_num(vm, "POPD_N", hr.n);
        var_set_num(vm, "DIRSTACK_N", hr.n);
        var_set_str(vm, "CWD", hr.str);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      /* PUSHD path */
      path[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0)
        snprintf(path, sizeof path, "%s", vm->last_str);
      if (!path[0] || cubalc_host_pushd(path, &hr) != 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "PUSHD_N", 0);
        var_set_num(vm, "OK", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        } else {
          var_set_str(vm, "LAST_ERR", "PUSHD: fail");
          var_set_str(vm, "ERR", "PUSHD: fail");
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "PUSHD_N", hr.n);
      var_set_num(vm, "DIRSTACK_N", hr.n);
      var_set_str(vm, "CWD", hr.str);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS CHDIR|CD path — change process working directory → LAST = new cwd.
     * LAST_N = 1 success, 0 soft miss (cwd unchanged). Updates CWD var.
     * Usability: relative READ/WRITE/LIST under STATE/TMP without shell cd. */
    if (kw(&L->cur,"CHDIR") || kw(&L->cur,"CD") || kw(&L->cur,"CHDIRTO") ||
        kw(&L->cur,"SETCWD") || kw(&L->cur,"CHANGE_DIR") || kw(&L->cur,"CHANGEDIR")){
      char path[512], cwd[512];
      int soft = 0, ok = 0;
      lex_next(L);
      if (kw(&L->cur,"OR") || kw(&L->cur,"SOFT") || kw(&L->cur,"OPTIONAL") ||
          kw(&L->cur,"TRY") || kw(&L->cur,"MAYBE")){
        soft = 1;
        lex_next(L);
      }
      path[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0)
        snprintf(path, sizeof path, "%s", vm->last_str);
      if (!path[0]) {
        ok = 0;
      } else {
#if defined(CUBALC_OS_WINDOWS)
        ok = (_chdir(path) == 0) ? 1 : 0;
#else
        ok = (chdir(path) == 0) ? 1 : 0;
#endif
      }
      if (!getcwd(cwd, sizeof cwd))
        snprintf(cwd, sizeof cwd, ok ? path : ".");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", cwd);
      var_set_str(vm, "LAST", cwd);
      var_set_str(vm, "CWD", cwd);
      vm->last_n = ok ? 1 : 0;
      var_set_num(vm, "LAST_N", ok ? 1 : 0);
      var_set_num(vm, "CHDIR_N", ok ? 1 : 0);
      var_set_num(vm, "OK", ok ? 1 : 0);
      if (!ok && !soft) {
        /* sticky soft err for agents; not fatal (plate continues) */
        char em[180];
        snprintf(em, sizeof em, "CHDIR miss: %s", path[0] ? path : "(empty)");
        var_set_str(vm, "LAST_ERR", em);
        var_set_str(vm, "ERR", em);
      }
      bump(vm); return 1;
    }
    if (kw(&L->cur,"STATE") || kw(&L->cur,"STATEDIR") || kw(&L->cur,"STATE_DIR")){
      char sdir[512];
      const char *e;
      lex_next(L);
      e = getenv("CUBALC_STATE");
      if (e && e[0]) snprintf(sdir, sizeof sdir, "%s", e);
      else snprintf(sdir, sizeof sdir, "state");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", sdir);
      vm->last_n = (long)strlen(sdir);
      var_set_str(vm, "LAST", sdir);
      var_set_str(vm, "STATE_DIR", sdir);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"ROOT") || kw(&L->cur,"CUBALC_ROOT") || kw(&L->cur,"INSTALL_ROOT")){
      char root[512];
      const char *e;
      lex_next(L);
      e = getenv("CUBALC_ROOT");
      if (e && e[0]) {
        snprintf(root, sizeof root, "%s", e);
      } else if (!getcwd(root, sizeof root)) {
        snprintf(root, sizeof root, ".");
      }
      snprintf(vm->last_str, sizeof vm->last_str, "%s", root);
      vm->last_n = (long)strlen(root);
      var_set_str(vm, "LAST", root);
      var_set_str(vm, "ROOT", root);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MKTEMP|TEMPFILE|MAKETEMP [prefix]
     * — create unique empty file under TMP (mkstemp template prefix.XXXXXX).
     * Default prefix "cubalc". Prefix sanitized (alnum/_/- only, max 32).
     * LAST = path; LAST_N/MKTEMP_N = 1; soft fail OK=0.
     * Usability: collision-free agent plate paths without shell mktemp. */
    if (kw(&L->cur,"MKTEMP") || kw(&L->cur,"TEMPFILE") || kw(&L->cur,"MAKETEMP") ||
        kw(&L->cur,"MKTEMPFILE") || kw(&L->cur,"NEWTEMP") || kw(&L->cur,"TMPFILE") ||
        kw(&L->cur,"TEMPPATH") || kw(&L->cur,"UNIQUETEMP")){
      char tdir[512], prefix[40], safe[36], tmpl[560], path[560];
      const char *e;
      size_t i, j, pl;
      int fd = -1;
      lex_next(L);
      tdir[0] = 0; prefix[0] = 0; safe[0] = 0; tmpl[0] = 0; path[0] = 0;
      /* optional prefix string */
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, prefix, sizeof prefix) != 0) {
          /* no arg — default */
          prefix[0] = 0;
        }
      }
      /* resolve temp dir (same policy as SYS TMP) */
      e = getenv("TMPDIR");
      if (!e || !e[0]) e = getenv("TMP");
      if (!e || !e[0]) e = getenv("TEMP");
      if (!e || !e[0]) e = getenv("CUBALC_TMP");
      if (e && e[0]) snprintf(tdir, sizeof tdir, "%s", e);
#if defined(CUBALC_OS_WINDOWS)
      if (!tdir[0]) {
        e = getenv("LOCALAPPDATA");
        if (e && e[0]) snprintf(tdir, sizeof tdir, "%s\\Temp", e);
        else snprintf(tdir, sizeof tdir, ".");
      }
#else
      if (!tdir[0]) snprintf(tdir, sizeof tdir, "/tmp");
#endif
      {
        size_t n = strlen(tdir);
        while (n > 1 && (tdir[n - 1] == '/' || tdir[n - 1] == '\\')) {
          tdir[n - 1] = 0;
          n--;
        }
      }
      /* sanitize prefix: strip path seps, keep alnum _ - */
      if (!prefix[0]) snprintf(prefix, sizeof prefix, "cubalc");
      for (i = 0, j = 0; prefix[i] && j + 1 < sizeof safe; i++) {
        char c = prefix[i];
        if (c == '/' || c == '\\' || c == '.') continue;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_' || c == '-')
          safe[j++] = c;
      }
      safe[j] = 0;
      if (!safe[0]) snprintf(safe, sizeof safe, "cubalc");
      /* cap length so template fits */
      pl = strlen(safe);
      if (pl > 32) safe[32] = 0;
      snprintf(tmpl, sizeof tmpl, "%s/%s.XXXXXX", tdir, safe);
#if defined(CUBALC_OS_WINDOWS)
      /* Windows fallback: unique path via pid+ms (no mkstemp) */
      {
        struct timespec ts;
        long ms;
        FILE *wf;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        ms = (long)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
        snprintf(path, sizeof path, "%s/%s.%ld.%ld.tmp", tdir, safe,
                 (long)getpid(), ms);
        wf = fopen(path, "wx");
        if (!wf) {
          snprintf(path, sizeof path, "%s/%s.%ld.%ld.%d.tmp", tdir, safe,
                   (long)getpid(), ms, rand() & 0xffff);
          wf = fopen(path, "w");
        }
        if (!wf) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "MKTEMP_N", 0);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", "MKTEMP: create fail");
          var_set_str(vm, "ERR", "MKTEMP: create fail");
          bump(vm); return 1;
        }
        fclose(wf);
      }
#else
      {
        char tcopy[560];
        snprintf(tcopy, sizeof tcopy, "%s", tmpl);
        fd = mkstemp(tcopy);
        if (fd < 0) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "MKTEMP_N", 0);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", "MKTEMP: mkstemp fail");
          var_set_str(vm, "ERR", "MKTEMP: mkstemp fail");
          bump(vm); return 1;
        }
        close(fd);
        snprintf(path, sizeof path, "%s", tcopy);
      }
#endif
      var_set_str(vm, "LAST", path);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_str(vm, "MKTEMP", path);
      var_set_str(vm, "TEMPFILE", path);
      var_set_str(vm, "MAKETEMP", path);
      var_set_num(vm, "MKTEMP_N", 1);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MKTEMPDIR|MKDTEMP|MAKETEMPDIR [prefix]
     * — create unique empty directory under TMP (mkdtemp prefix.XXXXXX).
     * Default prefix "cubalc". Prefix sanitized (alnum/_/- only, max 32).
     * LAST = path; LAST_N/MKTEMPDIR_N = 1; soft fail OK=0.
     * Usability: collision-free agent work dirs without shell mktemp -d. */
    if (kw(&L->cur,"MKTEMPDIR") || kw(&L->cur,"MKDTEMP") || kw(&L->cur,"MAKETEMPDIR") ||
        kw(&L->cur,"NEWTEMPDIR") || kw(&L->cur,"TEMPMKDIR") || kw(&L->cur,"UNIQUEDIR") ||
        kw(&L->cur,"MKDIRTEMP") || kw(&L->cur,"TEMPWORKDIR") || kw(&L->cur,"WORKDIRTMP")){
      char tdir[512], prefix[40], safe[36], tmpl[560], path[560];
      const char *e;
      size_t i, j, pl;
      lex_next(L);
      tdir[0] = 0; prefix[0] = 0; safe[0] = 0; tmpl[0] = 0; path[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, prefix, sizeof prefix) != 0)
          prefix[0] = 0;
      }
      e = getenv("TMPDIR");
      if (!e || !e[0]) e = getenv("TMP");
      if (!e || !e[0]) e = getenv("TEMP");
      if (!e || !e[0]) e = getenv("CUBALC_TMP");
      if (e && e[0]) snprintf(tdir, sizeof tdir, "%s", e);
#if defined(CUBALC_OS_WINDOWS)
      if (!tdir[0]) {
        e = getenv("LOCALAPPDATA");
        if (e && e[0]) snprintf(tdir, sizeof tdir, "%s\\Temp", e);
        else snprintf(tdir, sizeof tdir, ".");
      }
#else
      if (!tdir[0]) snprintf(tdir, sizeof tdir, "/tmp");
#endif
      {
        size_t n = strlen(tdir);
        while (n > 1 && (tdir[n - 1] == '/' || tdir[n - 1] == '\\')) {
          tdir[n - 1] = 0;
          n--;
        }
      }
      if (!prefix[0]) snprintf(prefix, sizeof prefix, "cubalc");
      for (i = 0, j = 0; prefix[i] && j + 1 < sizeof safe; i++) {
        char c = prefix[i];
        if (c == '/' || c == '\\' || c == '.') continue;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_' || c == '-')
          safe[j++] = c;
      }
      safe[j] = 0;
      if (!safe[0]) snprintf(safe, sizeof safe, "cubalc");
      pl = strlen(safe);
      if (pl > 32) safe[32] = 0;
      snprintf(tmpl, sizeof tmpl, "%s/%s.XXXXXX", tdir, safe);
#if defined(CUBALC_OS_WINDOWS)
      {
        struct timespec ts;
        long ms;
        cubalc_host_result hr;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        ms = (long)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
        snprintf(path, sizeof path, "%s/%s.%ld.%ld.d", tdir, safe,
                 (long)getpid(), ms);
        if (cubalc_host_mkdir(path, &hr) != 0) {
          snprintf(path, sizeof path, "%s/%s.%ld.%ld.%d.d", tdir, safe,
                   (long)getpid(), ms, rand() & 0xffff);
          if (cubalc_host_mkdir(path, &hr) != 0) {
            var_set_str(vm, "LAST", "");
            vm->last_str[0] = 0;
            vm->last_n = 0;
            var_set_num(vm, "LAST_N", 0);
            var_set_num(vm, "MKTEMPDIR_N", 0);
            var_set_num(vm, "OK", 0);
            var_set_str(vm, "LAST_ERR", "MKTEMPDIR: mkdir fail");
            var_set_str(vm, "ERR", "MKTEMPDIR: mkdir fail");
            bump(vm); return 1;
          }
        }
      }
#else
      {
        char tcopy[560];
        char *created;
        snprintf(tcopy, sizeof tcopy, "%s", tmpl);
        created = mkdtemp(tcopy);
        if (!created) {
          var_set_str(vm, "LAST", "");
          vm->last_str[0] = 0;
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "MKTEMPDIR_N", 0);
          var_set_num(vm, "OK", 0);
          var_set_str(vm, "LAST_ERR", "MKTEMPDIR: mkdtemp fail");
          var_set_str(vm, "ERR", "MKTEMPDIR: mkdtemp fail");
          bump(vm); return 1;
        }
        snprintf(path, sizeof path, "%s", created);
      }
#endif
      var_set_str(vm, "LAST", path);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_str(vm, "MKTEMPDIR", path);
      var_set_str(vm, "MKDTEMP", path);
      var_set_str(vm, "MAKETEMPDIR", path);
      var_set_num(vm, "MKTEMPDIR_N", 1);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS TMP|TEMP|TMPDIR — portable temp directory for agent plate writes.
     * Prefer TMPDIR / TMP / TEMP env, else /tmp (POSIX) or "." (fallback).
     * Usability: SYS JOIN TMP "plate.json" without shell $TMPDIR glue. */
    if (kw(&L->cur,"TMP") || kw(&L->cur,"TEMP") || kw(&L->cur,"TMPDIR") ||
        kw(&L->cur,"TEMPDIR") || kw(&L->cur,"TEMP_DIR") || kw(&L->cur,"TMP_DIR")){
      char tdir[512];
      const char *e;
      lex_next(L);
      tdir[0] = 0;
      e = getenv("TMPDIR");
      if (!e || !e[0]) e = getenv("TMP");
      if (!e || !e[0]) e = getenv("TEMP");
      if (!e || !e[0]) e = getenv("CUBALC_TMP");
      if (e && e[0]) snprintf(tdir, sizeof tdir, "%s", e);
#if defined(CUBALC_OS_WINDOWS)
      if (!tdir[0]) {
        e = getenv("LOCALAPPDATA");
        if (e && e[0]) snprintf(tdir, sizeof tdir, "%s\\Temp", e);
        else snprintf(tdir, sizeof tdir, ".");
      }
#else
      if (!tdir[0]) snprintf(tdir, sizeof tdir, "/tmp");
#endif
      /* strip trailing slash (keep root /) */
      {
        size_t n = strlen(tdir);
        while (n > 1 && (tdir[n - 1] == '/' || tdir[n - 1] == '\\')) {
          tdir[n - 1] = 0;
          n--;
        }
      }
      snprintf(vm->last_str, sizeof vm->last_str, "%s", tdir);
      vm->last_n = (long)strlen(tdir);
      var_set_str(vm, "LAST", tdir);
      var_set_str(vm, "TMP", tdir);
      var_set_str(vm, "TMPDIR", tdir);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
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

