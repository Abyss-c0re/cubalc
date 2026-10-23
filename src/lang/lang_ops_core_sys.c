/* CubalC lang — lang_ops_core_sys.c
 * SYS/HOST megablock dispatch (mesh vote A_split_lang_ops_core).
 * Peel of ~33k-line SYS plane from lang_ops_core.
 */
#include "lang/cubalc_lang_ops_core_sys.h"
#include "lang/cubalc_lang_ops_sys_plate.h"
#include "lang/cubalc_lang_ops_sys_path_fs.h"
#include "lang/cubalc_lang_ops_sys_env.h"
#include "lang/cubalc_lang_ops_sys_string.h"
#include "lang/cubalc_lang_ops_sys_wait.h"
#include "lang/cubalc_lang_ops_sys_stat.h"
#include "lang/cubalc_lang_ops_sys_bulk.h"
#include "lang/cubalc_lang_ops_sys_bulk_mut.h"
#include "lang/cubalc_lang_ops_sys_net.h"
#include "lang/cubalc_lang_ops_sys_files.h"
#include "lang/cubalc_lang_ops_sys_json.h"
#include "lang/cubalc_lang_ops_sys_streq.h"
#include "lang/cubalc_lang_ops_sys_bagmut.h"
#include "lang/cubalc_lang_ops_sys_rand.h"
#include "lang/cubalc_lang_ops_sys_numagg.h"
#include "lang/cubalc_lang_ops_sys_hostmeta.h"
#include "lang/cubalc_lang_ops_sys_kv.h"
#include "lang/cubalc_lang_ops_sys_kvmath.h"
#include "lang/cubalc_lang_ops_sys_kvfilter.h"
#include "lang/cubalc_lang_ops_sys_grep.h"
#include "lang/cubalc_lang_ops_sys_identity.h"
#include "lang/cubalc_lang_ops_sys_codec.h"
#include "lang/cubalc_lang_ops_sys_time_walk.h"
#include "lang/cubalc_lang_ops_sys_human.h"
#include "lang/cubalc_lang_ops_sys_time_date.h"
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

int cubalc_lang_ops_core_try_sys(VM *vm, Lex *L) {
  /* SYS host ops — C runtime (Grokium without Python) */
  if (kw(&L->cur,"SYS") || kw(&L->cur,"HOST")){
    lex_next(L);
    skip_nl(L);
    /* plate/JSON-file theme → lang_ops_sys_plate.c (mesh vote A2) */
    {
      int pr = cubalc_lang_ops_sys_try_plate(vm, L);
      if (pr != 0) return pr;
    }

    /* path/FS theme → lang_ops_sys_path_fs.c (mesh vote A2) */
    {
      int fr = cubalc_lang_ops_sys_try_path_fs(vm, L);
      if (fr != 0) return fr;
    }

    /* ENV/process theme → lang_ops_sys_env.c (mesh vote A2) */
    {
      int er = cubalc_lang_ops_sys_try_env(vm, L);
      if (er != 0) return er;
    }

    /* string/codec theme → lang_ops_sys_string.c (mesh vote A2) */
    {
      int sr = cubalc_lang_ops_sys_try_string(vm, L);
      if (sr != 0) return sr;
    }

    /* wait/poll theme → lang_ops_sys_wait.c (mesh vote A2) */
    {
      int wr = cubalc_lang_ops_sys_try_wait(vm, L);
      if (wr != 0) return wr;
    }

    /* stat/meta theme → lang_ops_sys_stat.c (mesh vote A2) */
    {
      int tr = cubalc_lang_ops_sys_try_stat(vm, L);
      if (tr != 0) return tr;
    }

    /* bulk bag map/filter → lang_ops_sys_bulk.c (mesh vote A2) */
    {
      int br = cubalc_lang_ops_sys_try_bulk(vm, L);
      if (br != 0) return br;
    }

    /* bulk mutate → lang_ops_sys_bulk_mut.c (mesh vote A2) */
    {
      int mr = cubalc_lang_ops_sys_try_bulk_mut(vm, L);
      if (mr != 0) return mr;
    }

    /* net/TCP peer xfer → lang_ops_sys_net.c (mesh vote A2) */
    {
      int nr = cubalc_lang_ops_sys_try_net(vm, L);
      if (nr != 0) return nr;
    }

    /* files bag/content → lang_ops_sys_files.c */
    {
      int fir = cubalc_lang_ops_sys_try_files(vm, L);
      if (fir != 0) return fir;
    }

    if (kw(&L->cur,"SPAWN")){
      lex_next(L);
      if (L->cur.kind!=TK_STR && L->cur.kind!=TK_IDENT){ fail(vm,"SYS SPAWN bin [args…]"); return -1; }
      char bin[512];
      if (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT){
        /* if not path, which */
        if (strchr(L->cur.text, '/')) snprintf(bin,sizeof bin,"%s",L->cur.text);
        else {
          cubalc_host_result wh;
          if (cubalc_host_which(L->cur.text, &wh)==0) snprintf(bin,sizeof bin,"%s",wh.str);
          else snprintf(bin,sizeof bin,"%s",L->cur.text);
        }
        lex_next(L);
      }
      char *av[16]; char abuf[16][256]; int ac=0;
      av[ac++] = bin;
      while (ac < 15 && (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT || L->cur.kind==TK_NUM)){
        snprintf(abuf[ac], sizeof abuf[ac], "%s", L->cur.text);
        av[ac] = abuf[ac];
        ac++; lex_next(L);
      }
      av[ac] = NULL;
      cubalc_host_result hr;
      cubalc_host_spawn(bin, av, &hr);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      var_set_str(vm, "LAST", hr.str);
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "EXIT", hr.code);
      var_set_num(vm, "OK", hr.ok ? 1 : 0);
      bump(vm); return 1;
    }
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
    /* JSON plate theme → lang_ops_sys_json.c (A2) */
    {
      int jr = cubalc_lang_ops_sys_try_json(vm, L);
      if (jr != 0) return jr;
    }

    /* SYS CHAT "local"|"grok" ["model"] — msg from GROKIUM_MSG / CUBALC_MSG env */
    if (kw(&L->cur,"CHAT") || kw(&L->cur,"ASK")){
      lex_next(L);
      char be[32]="local", model[128]="";
      if (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT){
        snprintf(be,sizeof be,"%s",L->cur.text); lex_next(L);
      }
      if (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT){
        /* model or message string */
        if (strcmp(L->cur.text,"local")==0 || strcmp(L->cur.text,"grok")==0 ||
            strncmp(L->cur.text,"grok",4)==0 || strstr(L->cur.text,"/") ||
            strstr(L->cur.text,".gguf")) {
          snprintf(model,sizeof model,"%s",L->cur.text); lex_next(L);
        }
      }
      char msg[2000]; msg[0]=0;
      if (L->cur.kind==TK_STR){
        snprintf(msg,sizeof msg,"%s",L->cur.text); lex_next(L);
      } else {
        const char *e = getenv("GROKIUM_MSG");
        if (!e || !e[0]) e = getenv("CUBALC_MSG");
        if (e) snprintf(msg,sizeof msg,"%s",e);
      }
      if (!msg[0]){ fail(vm,"SYS CHAT needs msg or GROKIUM_MSG"); return -1; }
      if (!model[0]){
        const char *em = getenv("GROKIUM_MODEL");
        if (em && em[0]) snprintf(model,sizeof model,"%s",em);
      }
      const char *st = getenv("CUBALC_STATE");
      cubalc_host_result hr;
      if (cubalc_host_chat(be, model, msg, st, &hr)!=0){
        var_set_str(vm,"LAST", hr.err[0]?hr.err:"chat fail");
        var_set_num(vm,"OK",0);
        var_set_num(vm,"HTTP_CODE", hr.code);
        if (vm->trace) fprintf(vm->trace, "# chat fail %s\n", hr.err);
        bump(vm); return 1; /* soft */
      }
      snprintf(vm->last_str,sizeof vm->last_str,"%s",hr.str);
      vm->last_n = hr.n;
      vm->last_code = hr.code;
      var_set_str(vm,"LAST",hr.str);
      var_set_num(vm,"LAST_N",hr.n);
      var_set_num(vm,"HTTP_CODE",hr.code);
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
    /* SYS NUM|INT — parse LAST as integer → LAST_N (CubeBrain digit fold) */
    if (kw(&L->cur,"NUM") || kw(&L->cur,"INT") || kw(&L->cur,"ATOI")){
      lex_next(L);
      const char *s = vm->last_str;
      if (L->cur.kind==TK_STR){ s = L->cur.text; lex_next(L); }
      else if (L->cur.kind==TK_IDENT && strcmp(L->cur.text,"LAST")!=0){
        Var *v = var_get(vm, L->cur.text, 0);
        if (v && v->is_str) s = v->sval;
        else if (v) { var_set_num(vm,"LAST_N", v->val); vm->last_n = v->val; bump(vm); return 1; }
        lex_next(L);
      } else if (L->cur.kind==TK_IDENT && strcmp(L->cur.text,"LAST")==0){
        lex_next(L);
      }
      long n = 0;
      if (s && s[0]) n = strtol(s, NULL, 10);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS STR|ITOA|NUMSTR|TOSTR [expr|LAST_N] — integer → decimal string LAST.
     * Usability: dual of SYS NUM; fill REPLACEALL templates {{COUNT}} without shell. */
    if (kw(&L->cur,"STR") || kw(&L->cur,"ITOA") || kw(&L->cur,"NUMSTR") ||
        kw(&L->cur,"TOSTR") || kw(&L->cur,"STRNUM") || kw(&L->cur,"DECIMAL")){
      lex_next(L);
      long n = vm->last_n;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE"))){
        n = parse_expr(vm, L);
      }
      char buf[40];
      snprintf(buf, sizeof buf, "%ld", n);
      var_set_str(vm, "LAST", buf);
      var_set_str(vm, "STR", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* human size/duration → lang_ops_sys_human.c (mesh vote A2) */
    {
      int hu = cubalc_lang_ops_sys_try_human(vm, L);
      if (hu != 0) return hu;
    }

    /* SYS LENALL|MAPLEN|FIELDLENS [bag] — length of every newline field as a
     * decimal string bag → LAST. LAST_N/LENALL_N = field count;
     * LENALL_SUM = sum of lengths (total chars excl. newlines).
     * Usability: size rollups / max field width without EACH+LEN+PUSH glue;
     * pairs with SYS SUM/MAX/AVG/SORTN on the length bag. */
    if (kw(&L->cur,"LENALL") || kw(&L->cur,"MAPLEN") || kw(&L->cur,"FIELDLENS") ||
        kw(&L->cur,"STRLENS") || kw(&L->cur,"LENBAG") || kw(&L->cur,"LENGTHS") ||
        kw(&L->cur,"SIZES") || kw(&L->cur,"BYTELENS") || kw(&L->cur,"WIDTHS")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], nbuf[32];
      const char *p, *start;
      size_t olen = 0, flen, nlen;
      long kept = 0, sum = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          sum += (long)flen;
          snprintf(nbuf, sizeof nbuf, "%ld", (long)flen);
          nlen = strlen(nbuf);
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + nlen < sizeof out) {
            memcpy(out + olen, nbuf, nlen);
            olen += nlen;
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            memcpy(out + olen, nbuf, t);
            olen += t;
          }
          out[olen] = 0;
          kept++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "LENALL_N", kept);
      var_set_num(vm, "LENALL_SUM", sum);
      var_set_num(vm, "MAPLEN_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MAXLEN|MAXWIDTH|FIELDMAX [bag] — max string length among bag fields → LAST_N.
     * SYS MINLEN|MINWIDTH|FIELDMIN [bag] — min length. LAST = decimal of that length.
     * MAXLEN_N/MINLEN_N = field count; MAXLEN_I/MINLEN_I = 0-based index of first extreme
     * (-1 if empty bag). Empty bag → LAST_N 0.
     * Usability: column width for PADALL without LENALL+MAX glue. */
    if (kw(&L->cur,"MAXLEN") || kw(&L->cur,"MAXWIDTH") || kw(&L->cur,"FIELDMAX") ||
        kw(&L->cur,"LONGEST_LEN") || kw(&L->cur,"MAXSTRLEN") || kw(&L->cur,"BAGMAXLEN") ||
        kw(&L->cur,"MINLEN") || kw(&L->cur,"MINWIDTH") || kw(&L->cur,"FIELDMIN") ||
        kw(&L->cur,"SHORTEST_LEN") || kw(&L->cur,"MINSTRLEN") || kw(&L->cur,"BAGMINLEN")){
      char op[20];
      int is_min = 0;
      char bag[CUBALC_HOST_STR_MAX], out[40];
      const char *p, *start;
      size_t flen;
      long kept = 0, best = 0, best_i = -1, idx = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "MINLEN") == 0 || strcmp(op, "MINWIDTH") == 0 ||
          strcmp(op, "FIELDMIN") == 0 || strcmp(op, "SHORTEST_LEN") == 0 ||
          strcmp(op, "MINSTRLEN") == 0 || strcmp(op, "BAGMINLEN") == 0)
        is_min = 1;
      lex_next(L);
      bag[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (kept == 0) {
            best = (long)flen;
            best_i = idx;
          } else if (is_min) {
            if ((long)flen < best) {
              best = (long)flen;
              best_i = idx;
            }
          } else {
            if ((long)flen > best) {
              best = (long)flen;
              best_i = idx;
            }
          }
          kept++;
          idx++;
          if (*p == '\n') p++;
        }
      }
      snprintf(out, sizeof out, "%ld", kept ? best : 0L);
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept ? best : 0;
      var_set_num(vm, "LAST_N", kept ? best : 0);
      if (is_min) {
        var_set_num(vm, "MINLEN", kept ? best : 0);
        var_set_num(vm, "MINLEN_N", kept);
        var_set_num(vm, "MINLEN_I", best_i);
        var_set_num(vm, "MINWIDTH", kept ? best : 0);
      } else {
        var_set_num(vm, "MAXLEN", kept ? best : 0);
        var_set_num(vm, "MAXLEN_N", kept);
        var_set_num(vm, "MAXLEN_I", best_i);
        var_set_num(vm, "MAXWIDTH", kept ? best : 0);
      }
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS LONGEST|LONGESTLINE|MAXFIELD [bag] — first longest bag field text → LAST.
     * SYS SHORTEST|SHORTESTLINE|MINFIELD [bag] — first shortest field.
     * LAST_N 0|1 found; LONGEST_I/SHORTEST_I = index (-1 miss);
     * LONGEST_LEN/SHORTEST_LEN = length of that field; LONGEST_N = field count.
     * Distinct from MAXLEN/MINLEN (numeric length only) and LONGEST_LEN alias of MAXLEN.
     * Usability: pick longest error/payload without MAXLEN_I+NTH glue. */
    if (kw(&L->cur,"LONGEST") || kw(&L->cur,"LONGESTLINE") || kw(&L->cur,"MAXFIELD") ||
        kw(&L->cur,"LONGESTSTR") || kw(&L->cur,"PICKLONGEST") || kw(&L->cur,"BAGLONGEST") ||
        kw(&L->cur,"SHORTEST") || kw(&L->cur,"SHORTESTLINE") || kw(&L->cur,"MINFIELD") ||
        kw(&L->cur,"SHORTESTSTR") || kw(&L->cur,"PICKSHORTEST") || kw(&L->cur,"BAGSHORTEST")){
      char op[20];
      int is_short = 0;
      char bag[CUBALC_HOST_STR_MAX], out[512];
      const char *p, *start;
      size_t flen, best_len = 0;
      long kept = 0, best_i = -1, idx = 0, found = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "SHORTEST") == 0 || strcmp(op, "SHORTESTLINE") == 0 ||
          strcmp(op, "MINFIELD") == 0 || strcmp(op, "SHORTESTSTR") == 0 ||
          strcmp(op, "PICKSHORTEST") == 0 || strcmp(op, "BAGSHORTEST") == 0)
        is_short = 1;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (kept == 0 ||
              (is_short && flen < best_len) ||
              (!is_short && flen > best_len)) {
            best_len = flen;
            best_i = idx;
            {
              size_t take = flen;
              if (take >= sizeof out) take = sizeof out - 1;
              memcpy(out, start, take);
              out[take] = 0;
            }
            found = 1;
          }
          kept++;
          idx++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      if (is_short) {
        var_set_num(vm, "SHORTEST_N", kept);
        var_set_num(vm, "SHORTEST_I", best_i);
        var_set_num(vm, "SHORTEST_LEN", found ? (long)best_len : 0);
        var_set_str(vm, "SHORTEST", out);
      } else {
        var_set_num(vm, "LONGEST_N", kept);
        var_set_num(vm, "LONGEST_I", best_i);
        var_set_num(vm, "LONGEST_LEN", found ? (long)best_len : 0);
        var_set_str(vm, "LONGEST", out);
      }
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS COMMONPREFIX|LCP|SHAREDPREFIX [bag] — longest common prefix of all fields.
     * SYS COMMONSUFFIX|LCS|SHAREDSUFFIX [bag] — longest common suffix.
     * LAST = shared string; LAST_N = its length; COMMONPREFIX_N = field count.
     * Empty bag / no share → LAST empty, LAST_N 0. Single field → whole field.
     * Usability: path/peer dir roots or shared ext without EACH+STARTS glue. */
    if (kw(&L->cur,"COMMONPREFIX") || kw(&L->cur,"LCP") || kw(&L->cur,"SHAREDPREFIX") ||
        kw(&L->cur,"COMMONPRE") || kw(&L->cur,"BAGPREFIX") || kw(&L->cur,"SHAREPRE") ||
        kw(&L->cur,"COMMONSUFFIX") || kw(&L->cur,"LCS") || kw(&L->cur,"SHAREDSUFFIX") ||
        kw(&L->cur,"COMMONSUF") || kw(&L->cur,"BAGSUFFIX") || kw(&L->cur,"SHARESUF")){
      char op[20];
      int is_suf = 0;
      char bag[CUBALC_HOST_STR_MAX], out[512];
      char fields[64][256];
      size_t flens[64];
      const char *p, *start;
      size_t flen, i, j, n = 0, plen = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "COMMONSUFFIX") == 0 || strcmp(op, "LCS") == 0 ||
          strcmp(op, "SHAREDSUFFIX") == 0 || strcmp(op, "COMMONSUF") == 0 ||
          strcmp(op, "BAGSUFFIX") == 0 || strcmp(op, "SHARESUF") == 0)
        is_suf = 1;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (bag[0]) {
        p = bag;
        while (*p && n < 64) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen >= sizeof fields[0]) flen = sizeof fields[0] - 1;
          memcpy(fields[n], start, flen);
          fields[n][flen] = 0;
          flens[n] = flen;
          n++;
          if (*p == '\n') p++;
        }
      }
      if (n == 0) {
        plen = 0;
      } else if (n == 1) {
        plen = flens[0];
        if (plen >= sizeof out) plen = sizeof out - 1;
        memcpy(out, fields[0], plen);
        out[plen] = 0;
      } else if (!is_suf) {
        /* longest common prefix */
        plen = flens[0];
        for (i = 1; i < n; i++) {
          size_t m = flens[i] < plen ? flens[i] : plen;
          j = 0;
          while (j < m && fields[0][j] == fields[i][j]) j++;
          plen = j;
          if (plen == 0) break;
        }
        if (plen >= sizeof out) plen = sizeof out - 1;
        memcpy(out, fields[0], plen);
        out[plen] = 0;
      } else {
        /* longest common suffix */
        plen = flens[0];
        for (i = 1; i < n; i++) {
          size_t m = flens[i] < plen ? flens[i] : plen;
          j = 0;
          while (j < m &&
                 fields[0][flens[0] - 1 - j] == fields[i][flens[i] - 1 - j])
            j++;
          plen = j;
          if (plen == 0) break;
        }
        if (plen >= sizeof out) plen = sizeof out - 1;
        if (plen > 0)
          memcpy(out, fields[0] + (flens[0] - plen), plen);
        out[plen] = 0;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)plen;
      var_set_num(vm, "LAST_N", (long)plen);
      if (is_suf) {
        var_set_num(vm, "COMMONSUFFIX_N", (long)n);
        var_set_num(vm, "COMMONSUFFIX_LEN", (long)plen);
        var_set_str(vm, "COMMONSUFFIX", out);
        var_set_num(vm, "LCS_N", (long)n);
      } else {
        var_set_num(vm, "COMMONPREFIX_N", (long)n);
        var_set_num(vm, "COMMONPREFIX_LEN", (long)plen);
        var_set_str(vm, "COMMONPREFIX", out);
        var_set_num(vm, "LCP_N", (long)n);
      }
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS STRIPPREFIX|MAPSTRIPPRE bag [prefix] — remove leading prefix from each field
     * if present (only when field starts with prefix). Empty prefix → no-op copy.
     * SYS STRIPSUFFIX|MAPSTRIPSUF bag [suffix] — strip trailing suffix when present.
     * SYS STRIPCOMMON|STRIPLCP [bag] — compute LCP then strip it from every field
     * (relative paths in one step). LAST = bag; LAST_N = field count;
     * STRIPPREFIX_HIT / STRIPSUFFIX_HIT / STRIPCOMMON_HIT = fields shortened;
     * STRIPCOMMON_PRE = prefix removed (STRIPCOMMON only).
     * Usability: path relative after COMMONPREFIX without EACH+AFTER glue. */
    if (kw(&L->cur,"STRIPPREFIX") || kw(&L->cur,"MAPSTRIPPRE") || kw(&L->cur,"DROPPREFIX") ||
        kw(&L->cur,"REMOVEPREFIX") || kw(&L->cur,"CHOPPREFIX") || kw(&L->cur,"UNPREFIX") ||
        kw(&L->cur,"STRIPSUFFIX") || kw(&L->cur,"MAPSTRIPSUF") || kw(&L->cur,"DROPSUFFIX") ||
        kw(&L->cur,"REMOVESUFFIX") || kw(&L->cur,"CHOPSUFFIX") || kw(&L->cur,"UNSUFFIX") ||
        kw(&L->cur,"STRIPCOMMON") || kw(&L->cur,"STRIPLCP") || kw(&L->cur,"RELPATHS") ||
        kw(&L->cur,"STRIPROOT") || kw(&L->cur,"COMMONSTRIP")){
      char op[20];
      int is_suf = 0, is_common = 0;
      char bag[CUBALC_HOST_STR_MAX], needle[512], out[CUBALC_HOST_STR_MAX];
      char field[512], clipped[512];
      char fields[64][256];
      size_t flens[64];
      const char *p, *start;
      size_t flen, nlen, olen = 0, sn, i, j, n = 0, plen = 0;
      long kept = 0, hit = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "STRIPSUFFIX") == 0 || strcmp(op, "MAPSTRIPSUF") == 0 ||
          strcmp(op, "DROPSUFFIX") == 0 || strcmp(op, "REMOVESUFFIX") == 0 ||
          strcmp(op, "CHOPSUFFIX") == 0 || strcmp(op, "UNSUFFIX") == 0)
        is_suf = 1;
      if (strcmp(op, "STRIPCOMMON") == 0 || strcmp(op, "STRIPLCP") == 0 ||
          strcmp(op, "RELPATHS") == 0 || strcmp(op, "STRIPROOT") == 0 ||
          strcmp(op, "COMMONSTRIP") == 0)
        is_common = 1;
      lex_next(L);
      bag[0] = 0; needle[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (!is_common) {
        if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0] = 0;
      }
      /* parse fields; for STRIPCOMMON also compute LCP into needle */
      if (bag[0]) {
        p = bag;
        while (*p && n < 64) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen >= sizeof fields[0]) flen = sizeof fields[0] - 1;
          memcpy(fields[n], start, flen);
          fields[n][flen] = 0;
          flens[n] = flen;
          n++;
          if (*p == '\n') p++;
        }
      }
      if (is_common && n > 0) {
        plen = flens[0];
        for (i = 1; i < n; i++) {
          size_t m = flens[i] < plen ? flens[i] : plen;
          j = 0;
          while (j < m && fields[0][j] == fields[i][j]) j++;
          plen = j;
          if (plen == 0) break;
        }
        if (plen >= sizeof needle) plen = sizeof needle - 1;
        memcpy(needle, fields[0], plen);
        needle[plen] = 0;
      }
      nlen = strlen(needle);
      for (i = 0; i < n; i++) {
        flen = flens[i];
        memcpy(field, fields[i], flen + 1);
        sn = flen;
        if (nlen > 0 && flen >= nlen) {
          if (!is_suf) {
            if (memcmp(field, needle, nlen) == 0) {
              sn = flen - nlen;
              memcpy(clipped, field + nlen, sn);
              clipped[sn] = 0;
              hit++;
            } else {
              memcpy(clipped, field, sn);
              clipped[sn] = 0;
            }
          } else {
            if (memcmp(field + (flen - nlen), needle, nlen) == 0) {
              sn = flen - nlen;
              memcpy(clipped, field, sn);
              clipped[sn] = 0;
              hit++;
            } else {
              memcpy(clipped, field, sn);
              clipped[sn] = 0;
            }
          }
        } else {
          memcpy(clipped, field, sn);
          clipped[sn] = 0;
        }
        if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        if (olen + sn < sizeof out) {
          memcpy(out + olen, clipped, sn);
          olen += sn;
        } else if (olen < sizeof out - 1) {
          size_t t = sizeof out - 1 - olen;
          memcpy(out + olen, clipped, t);
          olen += t;
        }
        out[olen] = 0;
        kept++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "STRIPPREFIX_N", kept);
      var_set_num(vm, "STRIPSUFFIX_N", kept);
      var_set_num(vm, "STRIPCOMMON_N", kept);
      if (is_common) {
        var_set_num(vm, "STRIPCOMMON_HIT", hit);
        var_set_str(vm, "STRIPCOMMON_PRE", needle);
        var_set_num(vm, "STRIPCOMMON_LEN", (long)nlen);
      } else if (is_suf) {
        var_set_num(vm, "STRIPSUFFIX_HIT", hit);
      } else {
        var_set_num(vm, "STRIPPREFIX_HIT", hit);
      }
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS EMPTY|ISEMPTY [str|LAST] — LAST_N 1 if zero-length string.
     * SYS BLANK|ISBLANK|WS — LAST_N 1 if empty or only space/tab/CR/LF.
     * SYS NONEMPTY|NONEMPTY — invert of EMPTY (1 if any char).
     * Usability: soft plate / optional config IF without LEN + compare glue. */
    if (kw(&L->cur,"EMPTY") || kw(&L->cur,"ISEMPTY") || kw(&L->cur,"ISEMPTYSTR") ||
        kw(&L->cur,"BLANK") || kw(&L->cur,"ISBLANK") || kw(&L->cur,"WS") ||
        kw(&L->cur,"ISWS") || kw(&L->cur,"WHITESPACE") ||
        kw(&L->cur,"NONEMPTY") || kw(&L->cur,"NOTEMPTY") || kw(&L->cur,"HASCHARS")){
      char op[16]; snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      int want_blank = (strcmp(op, "BLANK") == 0 || strcmp(op, "ISBLANK") == 0 ||
                        strcmp(op, "WS") == 0 || strcmp(op, "ISWS") == 0 ||
                        strcmp(op, "WHITESPACE") == 0);
      int want_non = (strcmp(op, "NONEMPTY") == 0 || strcmp(op, "NOTEMPTY") == 0 ||
                      strcmp(op, "HASCHARS") == 0);
      char s[CUBALC_HOST_STR_MAX];
      long hit = 0;
      lex_next(L);
      s[0] = 0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      if (want_blank) {
        const char *p = s;
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        hit = (*p == 0) ? 1 : 0;
      } else if (want_non) {
        hit = (s[0] != 0) ? 1 : 0;
      } else {
        hit = (s[0] == 0) ? 1 : 0;
      }
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "EMPTY_N", hit);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS COALESCE|FIRSTS|NVL|IFNUL a b [c…] — first non-empty string arg → LAST.
     * SYS COALESCE BLANK|NB a b… — first non-blank (not only whitespace).
     * LAST_N = length of chosen; all empty → "" LAST_N=0 OK=1.
     * Usability: default chain for ENV/ARG plates without nested IF EMPTY. */
    if (kw(&L->cur,"COALESCE") || kw(&L->cur,"FIRSTS") || kw(&L->cur,"NVL") ||
        kw(&L->cur,"IFNUL") || kw(&L->cur,"IFNULL") || kw(&L->cur,"FIRSTNON") ||
        kw(&L->cur,"ORSTR") || kw(&L->cur,"FIRSTNONEMPTY")){
      char chosen[CUBALC_HOST_STR_MAX];
      char arg[CUBALC_HOST_STR_MAX];
      int skip_blank = 0, got = 0;
      long out_n = 0;
      lex_next(L);
      if (kw(&L->cur,"BLANK") || kw(&L->cur,"NB") || kw(&L->cur,"NONBLANK") ||
          kw(&L->cur,"TRIM") || kw(&L->cur,"WS")) {
        skip_blank = 1;
        lex_next(L);
      }
      chosen[0] = 0;
      while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
             L->cur.kind == TK_NUM) {
        arg[0] = 0;
        if (L->cur.kind == TK_NUM) {
          snprintf(arg, sizeof arg, "%ld", L->cur.num);
          lex_next(L);
        } else if (resolve_str_arg(vm, L, arg, sizeof arg) != 0) {
          break;
        }
        if (!got) {
          int empty = (arg[0] == 0);
          if (skip_blank && !empty) {
            const char *p = arg;
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
            empty = (*p == 0);
          }
          if (!empty) {
            snprintf(chosen, sizeof chosen, "%s", arg);
            got = 1;
          }
        }
        /* still consume remaining args even after choice */
      }
      out_n = (long)strlen(chosen);
      var_set_str(vm, "LAST", chosen);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", chosen);
      vm->last_n = out_n;
      var_set_num(vm, "LAST_N", out_n);
      var_set_num(vm, "COALESCE_N", out_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
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
    /* host-meta theme → lang_ops_sys_hostmeta.c (mesh vote A2) */
    {
      int hm = cubalc_lang_ops_sys_try_hostmeta(vm, L);
      if (hm != 0) return hm;
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
    /* RAND/PICK/SHUFFLE → lang_ops_sys_rand.c (B_core_sys) */
    {
      int rd = cubalc_lang_ops_sys_try_rand(vm, L);
      if (rd != 0) return rd;
    }

    /* host numeric MIN..RANGE → lang_ops_sys_numagg.c (mesh vote A2) */
    {
      int na = cubalc_lang_ops_sys_try_numagg(vm, L);
      if (na != 0) return na;
    }

    /* time/date theme → lang_ops_sys_time_date.c (mesh vote A2) */
    {
      int td = cubalc_lang_ops_sys_try_time_date(vm, L);
      if (td != 0) return td;
    }

    /* identity theme → lang_ops_sys_identity.c (mesh vote A2) */
    {
      int id = cubalc_lang_ops_sys_try_identity(vm, L);
      if (id != 0) return id;
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
    /* SYS HEX|FROMHEX — parse hex string (LAST default) → LAST_N  (I/O codec) */
    if (kw(&L->cur,"HEX") || kw(&L->cur,"FROMHEX") || kw(&L->cur,"XTOI")){
      lex_next(L);
      char s[512]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      /* skip optional 0x / 0X prefix */
      const char *p = s;
      if (p[0]=='0' && (p[1]=='x' || p[1]=='X')) p += 2;
      long n = 0;
      if (p[0]) n = strtol(p, NULL, 16);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS TOHEX [expr] — format int as lowercase hex → LAST (I/O codec) */
    if (kw(&L->cur,"TOHEX") || kw(&L->cur,"ITOH") || kw(&L->cur,"HEXOUT")){
      lex_next(L);
      long n = vm->last_n;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE"))){
        n = parse_expr(vm, L);
      }
      char buf[40];
      snprintf(buf, sizeof buf, "%lx", (unsigned long)n);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS ORD [str|LAST] — first byte code → LAST_N  (I/O codec) */
    if (kw(&L->cur,"ORD") || kw(&L->cur,"CODE") || kw(&L->cur,"BYTE")){
      lex_next(L);
      char s[512]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      long n = s[0] ? (long)(unsigned char)s[0] : 0;
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS CHR [expr] — integer → single-byte string LAST  (I/O codec) */
    if (kw(&L->cur,"CHR") || kw(&L->cur,"CHAR")){
      lex_next(L);
      long n = vm->last_n;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE"))){
        n = parse_expr(vm, L);
      }
      n &= 0xFF;
      char buf[4];
      buf[0] = (char)(unsigned char)n;
      buf[1] = 0;
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MID|SUBSTR|SLICE str start [len] — substring → LAST  (I/O codec) */
    /* SYS MID host-wide CUBALC_HOST_STR_MAX — multi-KB plate peels. */
    if (kw(&L->cur,"MID") || kw(&L->cur,"SUBSTR") || kw(&L->cur,"SLICE")){
      lex_next(L);
      char s[CUBALC_HOST_STR_MAX]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      long start = 0, len = -1;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          L->cur.kind==TK_IDENT){
        start = parse_expr(vm, L);
        if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
            L->cur.kind==TK_IDENT)
          len = parse_expr(vm, L);
      }
      size_t slen = strlen(s);
      if (start < 0) start = 0;
      if ((size_t)start > slen) start = (long)slen;
      size_t remain = slen - (size_t)start;
      size_t take = (len < 0) ? remain : (size_t)len;
      if (take > remain) take = remain;
      char out[CUBALC_HOST_STR_MAX];
      if (take >= sizeof out) take = sizeof out - 1;
      memcpy(out, s + (size_t)start, take);
      out[take] = 0;
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)take;
      var_set_num(vm, "LAST_N", (long)take);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS CAT|STRCAT|CONCAT parts… — concatenate 1..N strings → LAST.
     * Host-wide CUBALC_HOST_STR_MAX buffers (no 512/1024 truncate).
     * Multi-arg: SYS CAT "a" "b" "c" → "abc" without nested CAT chains.
     * Numbers / numeric vars accepted as decimal parts. Bare CAT echoes LAST.
     * LAST_N = byte length; CAT_N = part count.
     * Usability: multi-KB agent plate messages / paths in one form
     * (pairs STREPEAT/LPAD wide + FILLP). */
    if (kw(&L->cur,"CAT") || kw(&L->cur,"STRCAT") || kw(&L->cur,"CONCAT") ||
        kw(&L->cur,"STRCATN") || kw(&L->cur,"JOINSTR") || kw(&L->cur,"SCAT") ||
        kw(&L->cur,"STRAPPEND") || kw(&L->cur,"CATN")){
      lex_next(L);
      char out[CUBALC_HOST_STR_MAX], part[CUBALC_HOST_STR_MAX];
      size_t o = 0;
      long parts = 0;
      out[0] = 0;
      while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
             L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN) {
        if (L->cur.kind == TK_IDENT &&
            (kw(&L->cur,"ASSERT") || kw(&L->cur,"LET") || kw(&L->cur,"PRINT") ||
             kw(&L->cur,"SYS") || kw(&L->cur,"IF") || kw(&L->cur,"END") ||
             kw(&L->cur,"ELSE") || kw(&L->cur,"ELIF") || kw(&L->cur,"FOR") ||
             kw(&L->cur,"WHILE") || kw(&L->cur,"LOOP") || kw(&L->cur,"INCLUDE") ||
             kw(&L->cur,"SETP") || kw(&L->cur,"INCP") || kw(&L->cur,"DELP") ||
             kw(&L->cur,"GETP") || kw(&L->cur,"MERGEP") || kw(&L->cur,"FILLP") ||
             kw(&L->cur,"FILLPFILE") || kw(&L->cur,"DUMPP") || kw(&L->cur,"NEEDP") ||
             kw(&L->cur,"HASPANY") || kw(&L->cur,"NEEDPANY") || kw(&L->cur,"REQUIREPANY") ||
             kw(&L->cur,"EQP") || kw(&L->cur,"NEQP") || kw(&L->cur,"DIFFP") ||
             kw(&L->cur,"STABLEP") || kw(&L->cur,"SAMEKEYSP") || kw(&L->cur,"EQKEYSP") ||
             kw(&L->cur,"CHANGELOGP") || kw(&L->cur,"SUBSETP") || kw(&L->cur,"COVERSP") ||
             kw(&L->cur,"PLUCKP") || kw(&L->cur,"GETPALL") || kw(&L->cur,"DELTAP") ||
             kw(&L->cur,"PICKP") || kw(&L->cur,"KEEPP") || kw(&L->cur,"OMITP") ||
             kw(&L->cur,"RENAMEP") || kw(&L->cur,"MOVEKEYP") ||
             kw(&L->cur,"KEYDIFFP") || kw(&L->cur,"KEYCOMMP") ||
             kw(&L->cur,"COPYP") || kw(&L->cur,"SWAPP") ||
             kw(&L->cur,"LENP") || kw(&L->cur,"EMPTYP") || kw(&L->cur,"VALSP") ||
             kw(&L->cur,"TYPEP") || kw(&L->cur,"KINDP") ||
             kw(&L->cur,"GETOBJ") || kw(&L->cur,"SETOBJ") || kw(&L->cur,"PEEKOBJ") ||
             kw(&L->cur,"NESTP") || kw(&L->cur,"PUTOBJ") ||
             kw(&L->cur,"MERGEOBJ") || kw(&L->cur,"DEFAULTOBJ") || kw(&L->cur,"PATCHNEST") ||
             kw(&L->cur,"GETPOBJ") || kw(&L->cur,"SETPOBJ") || kw(&L->cur,"INCOBJ") ||
             kw(&L->cur,"DELPOBJ") ||
             kw(&L->cur,"HASPOBJ") || kw(&L->cur,"TYPEPOBJ") || kw(&L->cur,"KEYSOBJ") ||
             kw(&L->cur,"NEEDPOBJ") ||
             kw(&L->cur,"PLUCKOBJ") || kw(&L->cur,"NESTPLUCK") ||
             kw(&L->cur,"PATHKEYS") || kw(&L->cur,"LEAFKEYS") || kw(&L->cur,"DOTPATHS") ||
             kw(&L->cur,"FLATKV") || kw(&L->cur,"LEAFKV") || kw(&L->cur,"FLATTENP") ||
             kw(&L->cur,"UNFLATKV") || kw(&L->cur,"FROMFLAT") || kw(&L->cur,"UNFLATTENP") ||
             kw(&L->cur,"DIFFFLAT") || kw(&L->cur,"PATHDIFF") || kw(&L->cur,"CHANGELOGFLAT") ||
             kw(&L->cur,"GREPFLAT") || kw(&L->cur,"GREPVFLAT") || kw(&L->cur,"GREPFLATI") ||
             kw(&L->cur,"PRUNEFLAT") || kw(&L->cur,"KEEPONLYFLAT") || kw(&L->cur,"DELFLAT") ||
             kw(&L->cur,"MERGEFLAT") || kw(&L->cur,"OVERLAYFLAT") || kw(&L->cur,"PATCHFLAT") ||
             kw(&L->cur,"RENAMEFLAT") || kw(&L->cur,"MOVEFLAT") || kw(&L->cur,"PREFIXFLAT") ||
             kw(&L->cur,"SETFLAT") || kw(&L->cur,"MAPFLAT") || kw(&L->cur,"SETLEAFP") ||
             kw(&L->cur,"INCFLAT") || kw(&L->cur,"BUMPFLAT") || kw(&L->cur,"ADDFLAT") ||
             kw(&L->cur,"SUMFLAT") || kw(&L->cur,"TOTALFLAT") || kw(&L->cur,"LEAFSUM") ||
             kw(&L->cur,"AVGFLAT") || kw(&L->cur,"MEANFLAT") || kw(&L->cur,"MEDIANFLAT") ||
             kw(&L->cur,"P50FLAT") ||
             kw(&L->cur,"TOPPATHFLAT") || kw(&L->cur,"BOTPATHFLAT") || kw(&L->cur,"MAXPATHFLAT") ||
             kw(&L->cur,"THRESHFLAT") || kw(&L->cur,"KEEPVFLAT") || kw(&L->cur,"DROPZEROFLAT") ||
             kw(&L->cur,"CAPFLAT") || kw(&L->cur,"CLAMPFLAT") || kw(&L->cur,"MAXVFLAT") ||
             kw(&L->cur,"SCALEFLAT") || kw(&L->cur,"MULFLAT") || kw(&L->cur,"TIMESFLAT") ||
             kw(&L->cur,"HASFLAT") || kw(&L->cur,"COUNTFLAT") || kw(&L->cur,"ANYFLAT") ||
             kw(&L->cur,"PATHSFLAT") || kw(&L->cur,"VALSFLAT") || kw(&L->cur,"MATCHPATHS") ||
             kw(&L->cur,"GETFLAT") || kw(&L->cur,"FIRSTFLAT") || kw(&L->cur,"PEELFLAT") ||
             kw(&L->cur,"LASTFLAT") || kw(&L->cur,"ENDFLAT") || kw(&L->cur,"TAILFLAT") ||
             kw(&L->cur,"NTHFLAT") || kw(&L->cur,"INDEXFLAT") || kw(&L->cur,"ATFLAT") ||
             kw(&L->cur,"GETFLATN") || kw(&L->cur,"FIRSTFLATN") || kw(&L->cur,"NUMFLAT") ||
             kw(&L->cur,"LASTFLATN") || kw(&L->cur,"ENDFLATN") || kw(&L->cur,"TAILFLATN") ||
             kw(&L->cur,"NTHFLATN") || kw(&L->cur,"INDEXFLATN") || kw(&L->cur,"ATFLATN") ||
             kw(&L->cur,"TYPEFLAT") || kw(&L->cur,"KINDFLAT") || kw(&L->cur,"LEAFTYPE") ||
             kw(&L->cur,"FREQFLAT") || kw(&L->cur,"HISTFLAT") || kw(&L->cur,"COUNTVALFLAT") ||
             kw(&L->cur,"MODEFLAT") || kw(&L->cur,"TOPVALFLAT") || kw(&L->cur,"MAJORFLAT") ||
             kw(&L->cur,"PATHBYVAL") || kw(&L->cur,"VALPATH") || kw(&L->cur,"FINDVAL") ||
             kw(&L->cur,"PATHSBYVAL") || kw(&L->cur,"ALLPATHSBYVAL") || kw(&L->cur,"FINDVALS") ||
             kw(&L->cur,"COUNTBYVAL") || kw(&L->cur,"NBYVAL") || kw(&L->cur,"HASVAL") ||
             kw(&L->cur,"SETBYVAL") || kw(&L->cur,"REPLACEVAL") || kw(&L->cur,"REWVAL") ||
             kw(&L->cur,"DELBYVAL") || kw(&L->cur,"DROPVAL") || kw(&L->cur,"RMVAL") ||
             kw(&L->cur,"ALLEQFLAT") || kw(&L->cur,"SAMEVALFLAT") || kw(&L->cur,"EQVALSFLAT") ||
             kw(&L->cur,"FIRSTUNEQFLAT") || kw(&L->cur,"PATHUNEQ") || kw(&L->cur,"DIVERGE1") ||
             kw(&L->cur,"UNEQPATHS") || kw(&L->cur,"DIVERGEPATHS") || kw(&L->cur,"ALLUNEQ") ||
             kw(&L->cur,"UNIFORMFLAT") || kw(&L->cur,"CHECKFLAT") || kw(&L->cur,"SAMECHECK") ||
             kw(&L->cur,"PRETTYP") || kw(&L->cur,"JSONPRETTY") || kw(&L->cur,"PRETTYJSON") ||
             kw(&L->cur,"UNIQFLAT") || kw(&L->cur,"DISTINCTFLAT") || kw(&L->cur,"UNIQUEFLAT") ||
             kw(&L->cur,"NEEDFLAT") || kw(&L->cur,"REQUIREFLAT") || kw(&L->cur,"MUSTFLAT") ||
             kw(&L->cur,"PICKOBJ") || kw(&L->cur,"OMITOBJ") ||
             kw(&L->cur,"LENOBJ") || kw(&L->cur,"EMPTYOBJ") || kw(&L->cur,"VALSOBJ") ||
             kw(&L->cur,"RENAMEPOBJ") || kw(&L->cur,"MOVEKEYOBJ") || kw(&L->cur,"NESTRENAME") ||
             kw(&L->cur,"COPYPOBJ") || kw(&L->cur,"SWAPPOBJ") || kw(&L->cur,"NESTCOPY") ||
             kw(&L->cur,"TOKVOBJ") || kw(&L->cur,"FROMKVOBJ") || kw(&L->cur,"NESTTOKV") ||
             kw(&L->cur,"TOKVP") || kw(&L->cur,"FROMKVP") ||
             kw(&L->cur,"SUMNOBJ") || kw(&L->cur,"TOPKEYOBJ") || kw(&L->cur,"MAXNOBJ") ||
             kw(&L->cur,"SUMNP") || kw(&L->cur,"TOPKEYP") || kw(&L->cur,"MAXNP") ||
             kw(&L->cur,"THRESHOBJ") || kw(&L->cur,"DROPZEROOBJ") || kw(&L->cur,"CAPOBJ") ||
             kw(&L->cur,"THRESHP") || kw(&L->cur,"DROPZEROP") || kw(&L->cur,"CAPP") ||
             kw(&L->cur,"PCTOBJ") || kw(&L->cur,"SCALEPOBJ") || kw(&L->cur,"ADDPOBJ") ||
             kw(&L->cur,"PCTP") || kw(&L->cur,"SCALEP") || kw(&L->cur,"ADDP") ||
             kw(&L->cur,"DIVP") || kw(&L->cur,"SUMMERGEP") || kw(&L->cur,"SUBP") ||
             kw(&L->cur,"ABSP") || kw(&L->cur,"SIGNP") ||
             kw(&L->cur,"KEEPKEYOBJ") || kw(&L->cur,"DROPKEYOBJ") ||
             kw(&L->cur,"KEEPKEYP") || kw(&L->cur,"DROPKEYP") ||
             kw(&L->cur,"TOPNOBJ") || kw(&L->cur,"BOTNOBJ") ||
             kw(&L->cur,"TOPNP") || kw(&L->cur,"BOTNP") ||
             kw(&L->cur,"SORTOBJ") || kw(&L->cur,"SORTBAGOBJ") ||
             kw(&L->cur,"SORTP") || kw(&L->cur,"SORTBAGP") ||
             kw(&L->cur,"SUMMERGEOBJ") || kw(&L->cur,"ADDFREQOBJ") ||
             kw(&L->cur,"SUBVALOBJ") || kw(&L->cur,"SUBFREQOBJ") ||
             kw(&L->cur,"REQUIRE") || kw(&L->cur,"FAIL") || kw(&L->cur,"PASS") ||
             kw(&L->cur,"NOTE") || kw(&L->cur,"WHY") || kw(&L->cur,"EXPLAIN") ||
             kw(&L->cur,"EXIT") || kw(&L->cur,"HOLD_FLASH") ||
             kw(&L->cur,"VERSION") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"UNSET") ||
             kw(&L->cur,"CLASS") || kw(&L->cur,"NEW") || kw(&L->cur,"SEND") ||
             kw(&L->cur,"FN") || kw(&L->cur,"RETURN") || kw(&L->cur,"BREAK") ||
             kw(&L->cur,"CONTINUE") || kw(&L->cur,"EACH")))
          break;
        part[0] = 0;
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
            L->cur.kind == TK_LPAREN) {
          long n = parse_expr(vm, L);
          snprintf(part, sizeof part, "%ld", n);
        } else if (resolve_str_arg(vm, L, part, sizeof part) != 0) {
          if (L->cur.kind == TK_IDENT) {
            Var *v = var_get(vm, L->cur.text, 0);
            if (v && !v->is_str) {
              snprintf(part, sizeof part, "%ld", v->val);
              lex_next(L);
            } else {
              /* unknown ident → literal text (path glue helpers) */
              snprintf(part, sizeof part, "%s", L->cur.text);
              lex_next(L);
            }
          } else {
            break;
          }
        }
        {
          size_t pl = strlen(part);
          if (o + pl >= sizeof out) pl = (o < sizeof out - 1) ? (sizeof out - 1 - o) : 0;
          if (pl > 0) {
            memcpy(out + o, part, pl);
            o += pl;
          }
          out[o] = 0;
          parts++;
          if (o + 1 >= sizeof out) break; /* full */
        }
      }
      if (parts == 0) {
        /* bare CAT → echo LAST (agent log mirror) */
        snprintf(out, sizeof out, "%s", vm->last_str);
        o = strlen(out);
        if (out[0]) parts = 1;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)o;
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "CAT_N", parts);
      var_set_num(vm, "STRCAT_N", parts);
      var_set_str(vm, "CAT", out);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS FIND|INDEX hay needle — first index of needle in hay → LAST_N (-1 miss).
     * SYS FINDI|INDEXI|FIND I — case-insensitive (log/severity locate without UPPER).
     * Completes EQSI/HASI/GREPI/STARTSI family for position → MID peel. */
    if (kw(&L->cur,"FIND") || kw(&L->cur,"INDEX") || kw(&L->cur,"STRFIND") ||
        kw(&L->cur,"FINDI") || kw(&L->cur,"IFIND") || kw(&L->cur,"INDEXI") ||
        kw(&L->cur,"IINDEX") || kw(&L->cur,"STRFINDI") || kw(&L->cur,"FINDCI")){
      char op[16]; snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *p = op; *p; p++)
        if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 'a' + 'A');
      int icase = (strcmp(op, "FINDI") == 0 || strcmp(op, "IFIND") == 0 ||
                   strcmp(op, "INDEXI") == 0 || strcmp(op, "IINDEX") == 0 ||
                   strcmp(op, "STRFINDI") == 0 || strcmp(op, "FINDCI") == 0);
      lex_next(L);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      char hay[CUBALC_HOST_STR_MAX]="", needle[CUBALC_HOST_STR_MAX]="";
      if (resolve_str_arg(vm, L, hay, sizeof hay) != 0)
        snprintf(hay, sizeof hay, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0]=0;
      long idx = -1;
      if (needle[0] == 0) {
        if (hay[0]) idx = 0;
      } else if (!icase) {
        const char *p = strstr(hay, needle);
        if (p) idx = (long)(p - hay);
      } else {
        /* ASCII case-fold scan */
        size_t hn = strlen(hay), nn = strlen(needle), i, j;
        for (i = 0; i + nn <= hn; i++) {
          int hit = 1;
          for (j = 0; j < nn; j++) {
            char a = hay[i + j], b = needle[j];
            if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
            if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
            if (a != b) { hit = 0; break; }
          }
          if (hit) { idx = (long)i; break; }
        }
      }
      vm->last_n = idx;
      var_set_num(vm, "LAST_N", idx);
      var_set_num(vm, "OK", idx >= 0 ? 1 : 0);
      bump(vm); return 1;
    }
    /* SYS NTH n [str|LAST] — 0-based newline field → LAST (pairs with SYS LIST).
     * SYS LINE n [str] — 1-based alias.
     * SYS HEAD [str] — first field; SYS TAIL [str] — last field.
     * Soft out-of-range: OK=0 LAST="" LAST_N=0. */
    if (kw(&L->cur,"NTH") || kw(&L->cur,"LINE") || kw(&L->cur,"FIELD") ||
        kw(&L->cur,"GETLINE") || kw(&L->cur,"HEAD") || kw(&L->cur,"FIRST") ||
        kw(&L->cur,"TAIL") || kw(&L->cur,"LASTLINE")){
      int is_head = (kw(&L->cur,"HEAD") || kw(&L->cur,"FIRST"));
      int is_tail = (kw(&L->cur,"TAIL") || kw(&L->cur,"LASTLINE"));
      int one_based = (kw(&L->cur,"LINE") || kw(&L->cur,"GETLINE"));
      long want = 0, nlines = 0, i;
      char src[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      char out[512];
      size_t len;
      lex_next(L);
      if (!is_head && !is_tail) {
        if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
            L->cur.kind==TK_MINUS)
          want = parse_expr(vm, L);
        else
          want = 0;
        if (one_based) want -= 1;
      }
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      /* count fields; trailing newline does not create an empty last field */
      if (src[0]) {
        nlines = 1;
        for (p = src; *p; p++) {
          if (*p == '\n' && p[1]) nlines++;
        }
      }
      if (is_head) want = 0;
      if (is_tail) want = nlines > 0 ? nlines - 1 : -1;
      if (want < 0 || want >= nlines || !src[0]) {
        var_set_str(vm, "LAST", "");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      p = src;
      for (i = 0; i < want; i++) {
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
      }
      start = p;
      while (*p && *p != '\n') p++;
      len = (size_t)(p - start);
      if (len >= sizeof out) len = sizeof out - 1;
      memcpy(out, start, len);
      out[len] = 0;
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)len;
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* bag-GREP theme → lang_ops_sys_grep.c (mesh vote A2) */
    {
      int gr = cubalc_lang_ops_sys_try_grep(vm, L);
      if (gr != 0) return gr;
    }

    /* SYS TAKE|FIRSTN n [str|LAST] — first n newline fields → LAST.
     * SYS DROP|SKIP|REST n [str] — drop first n fields (keep the rest).
     * LAST_N/TAKE_N = kept count. n<=0: TAKE empty / DROP all.
     * Usability: window LIST/GREP text without index math before EACH LINE. */
    if (kw(&L->cur,"TAKE") || kw(&L->cur,"FIRSTN") || kw(&L->cur,"HEADN") ||
        kw(&L->cur,"DROP") || kw(&L->cur,"SKIP") || kw(&L->cur,"REST") ||
        kw(&L->cur,"TAILN") || kw(&L->cur,"DROPN")){
      int is_drop = (kw(&L->cur,"DROP") || kw(&L->cur,"SKIP") || kw(&L->cur,"REST") ||
                     kw(&L->cur,"TAILN") || kw(&L->cur,"DROPN"));
      long nwant = 0, idx = 0, kept = 0;
      char src[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t olen = 0;
      lex_next(L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
          L->cur.kind==TK_MINUS)
        nwant = parse_expr(vm, L);
      else
        nwant = 0;
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      out[0] = 0;
      if (src[0] && (is_drop || nwant > 0)) {
        p = src;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          if (start == p && *p == 0 && start > src && start[-1] == '\n')
            break;
          {
            size_t flen = (size_t)(p - start);
            int keep = 0;
            if (is_drop) keep = (idx >= nwant);
            else keep = (idx < nwant);
            if (keep) {
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + flen < sizeof out) {
                memcpy(out + olen, start, flen);
                olen += flen;
              } else if (olen < sizeof out - 1) {
                size_t take = sizeof out - 1 - olen;
                memcpy(out + olen, start, take);
                olen += take;
              }
              out[olen] = 0;
              kept++;
            }
            idx++;
          }
          if (*p == '\n') p++;
        }
      }
      out[olen] = 0;
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "TAKE_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SPLIT|SPLITBY|FIELDS sep [str|LAST] — split on sep → newline fields.
     * LAST_N/SPLIT_N = field count. Empty sep → whole string one field.
     * Usability: PATH/CSV/env lists → EACH LINE/GREP/TAKE without shell. */
    if (kw(&L->cur,"SPLIT") || kw(&L->cur,"SPLITBY") || kw(&L->cur,"FIELDS") ||
        kw(&L->cur,"STRSPLIT") || kw(&L->cur,"SEPARATE") || kw(&L->cur,"CUTSEP")){
      char sep[64]="", src[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      const char *p, *hit;
      size_t sepn, olen = 0, flen;
      long nfields = 0;
      lex_next(L);
      if (resolve_str_arg(vm, L, sep, sizeof sep) != 0) sep[0]=0;
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      out[0] = 0;
      sepn = strlen(sep);
      if (!src[0]) {
        /* empty source → zero fields */
      } else if (sepn == 0) {
        /* no separator → single field */
        snprintf(out, sizeof out, "%s", src);
        nfields = 1;
        olen = strlen(out);
      } else {
        p = src;
        for (;;) {
          hit = strstr(p, sep);
          if (hit) flen = (size_t)(hit - p);
          else flen = strlen(p);
          if (nfields > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + flen < sizeof out) {
            memcpy(out + olen, p, flen);
            olen += flen;
          } else if (olen < sizeof out - 1) {
            size_t take = sizeof out - 1 - olen;
            memcpy(out + olen, p, take);
            olen += take;
          }
          out[olen] = 0;
          nfields++;
          if (!hit) break;
          p = hit + sepn;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "SPLIT", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = nfields;
      var_set_num(vm, "LAST_N", nfields);
      var_set_num(vm, "SPLIT_N", nfields);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS WORDS|TOKENIZE|FIELDSW [str|LAST] — whitespace tokenize → newline fields.
     * Collapses runs of space/tab/CR/LF; trims ends. LAST_N/WORDS_N = token count.
     * Usability: free text / "a b  c" → EACH LINE/GREP/SORT without SPLIT " " glue. */
    if (kw(&L->cur,"WORDS") || kw(&L->cur,"TOKENIZE") || kw(&L->cur,"TOKENS") ||
        kw(&L->cur,"FIELDSW") || kw(&L->cur,"WSSPLIT") || kw(&L->cur,"SPLITWS") ||
        kw(&L->cur,"WORDLIST")){
      char src[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      const char *p;
      size_t olen = 0;
      long ntok = 0;
      lex_next(L);
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      out[0] = 0;
      p = src;
      while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        if (!*p) break;
        {
          const char *start = p;
          size_t flen;
          while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
          flen = (size_t)(p - start);
          if (ntok > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + flen < sizeof out) {
            memcpy(out + olen, start, flen);
            olen += flen;
          } else if (olen < sizeof out - 1) {
            size_t take = sizeof out - 1 - olen;
            memcpy(out + olen, start, take);
            olen += take;
          }
          out[olen] = 0;
          ntok++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "WORDS", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = ntok;
      var_set_num(vm, "LAST_N", ntok);
      var_set_num(vm, "WORDS_N", ntok);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS CUT|COLUMN|FIELDN hay sep n — peel Nth field by separator → LAST.
     * CUT/FIELDN: 0-based index. COLUMN/COL/COLN: 1-based (like SYS LINE).
     * LAST_N = 1 if field exists, 0 if miss/out-of-range. Empty sep → whole as field 0.
     * Usability: CSV/path/kv columns without SPLIT + NTH glue. */
    if (kw(&L->cur,"CUT") || kw(&L->cur,"COLUMN") || kw(&L->cur,"COL") ||
        kw(&L->cur,"COLN") || kw(&L->cur,"FIELDN") || kw(&L->cur,"NTHFIELD") ||
        kw(&L->cur,"GETFIELD") || kw(&L->cur,"CSVFIELD")){
      char op[16]; snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      int one_based = (strcmp(op, "COLUMN") == 0 || strcmp(op, "COL") == 0 ||
                       strcmp(op, "COLN") == 0);
      char hay[CUBALC_HOST_STR_MAX], sep[64], out[512];
      long want = 0, idx = 0, found = 0;
      const char *p, *hit, *start;
      size_t sepn, flen;
      lex_next(L);
      if (resolve_str_arg(vm, L, hay, sizeof hay) != 0)
        snprintf(hay, sizeof hay, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, sep, sizeof sep) != 0) sep[0] = 0;
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
        want = parse_expr(vm, L);
      else
        want = 0;
      if (one_based) {
        if (want < 1) want = 1;
        want = want - 1; /* convert to 0-based walk */
      } else {
        if (want < 0) want = 0;
      }
      out[0] = 0;
      sepn = strlen(sep);
      p = hay;
      if (!hay[0]) {
        found = 0;
      } else if (sepn == 0) {
        /* empty sep: only field 0 is the whole string */
        if (want == 0) {
          snprintf(out, sizeof out, "%s", hay);
          found = 1;
        }
      } else {
        while (*p) {
          start = p;
          hit = strstr(p, sep);
          if (hit) {
            flen = (size_t)(hit - p);
            p = hit + sepn;
          } else {
            flen = strlen(p);
            p = p + flen;
          }
          if (idx == want) {
            if (flen >= sizeof out) flen = sizeof out - 1;
            memcpy(out, start, flen);
            out[flen] = 0;
            found = 1;
            break;
          }
          idx++;
          if (!hit) break;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "CUT", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      var_set_num(vm, "CUT_N", found);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SORT [str|LAST] — lexicographic sort of newline fields → LAST.
     * SYS UNIQ [str|LAST] — drop adjacent duplicate fields (sort first for full unique).
     * LAST_N/SORT_N = kept count. Cap 512 fields.
     * Usability: stable LIST walks — order is not guaranteed by SYS LIST. */
    if (kw(&L->cur,"SORT") || kw(&L->cur,"SORTLINES") || kw(&L->cur,"SORTL") ||
        kw(&L->cur,"UNIQ") || kw(&L->cur,"UNIQUE") || kw(&L->cur,"DEDUP")){
      int is_uniq = (kw(&L->cur,"UNIQ") || kw(&L->cur,"UNIQUE") || kw(&L->cur,"DEDUP"));
      char src[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      /* field copies for stable strcmp (cap 512 × 256) */
      enum { SORT_MAX = 256, SORT_FLEN = 192 };
      char fields[SORT_MAX][SORT_FLEN];
      int order[SORT_MAX];
      int n = 0, i;
      const char *p, *start;
      size_t olen = 0;
      long kept = 0;
      lex_next(L);
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      out[0] = 0;
      if (src[0]) {
        p = src;
        while (*p && n < SORT_MAX) {
          start = p;
          while (*p && *p != '\n') p++;
          if (start == p && *p == 0 && start > src && start[-1] == '\n')
            break;
          {
            size_t flen = (size_t)(p - start);
            if (flen >= SORT_FLEN) flen = SORT_FLEN - 1;
            memcpy(fields[n], start, flen);
            fields[n][flen] = 0;
            order[n] = n;
            n++;
          }
          if (*p == '\n') p++;
        }
      }
      if (!is_uniq && n > 1) {
        /* insertion sort on order[] by fields — small n, pure C, no nested fn needed */
        for (i = 1; i < n; i++) {
          int key = order[i], j = i - 1;
          while (j >= 0 && strcmp(fields[order[j]], fields[key]) > 0) {
            order[j + 1] = order[j];
            j--;
          }
          order[j + 1] = key;
        }
      }
      for (i = 0; i < n; i++) {
        int idx = is_uniq ? i : order[i];
        if (is_uniq && i > 0 && strcmp(fields[i], fields[i - 1]) == 0)
          continue;
        if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        {
          size_t flen = strlen(fields[idx]);
          if (olen + flen < sizeof out) {
            memcpy(out + olen, fields[idx], flen);
            olen += flen;
          } else if (olen < sizeof out - 1) {
            size_t take = sizeof out - 1 - olen;
            memcpy(out + olen, fields[idx], take);
            olen += take;
          }
          out[olen] = 0;
        }
        kept++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "SORT_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS UNION|ORLINES|SETUNION a [b…] — merge bags · first-seen unique fields → LAST.
     * SYS DISTINCT|UNIQUEALL [bag] — order-preserving full unique (unlike adjacent UNIQ).
     * SYS INTERSECT|ANDLINES|SETAND a b — fields of a also in b (order of a).
     * SYS DIFF|EXCEPT|SETDIFF|MINUSLINES a b — fields of a not in b (order of a).
     * LAST_N = kept count. Cap 256 fields × 192 chars.
     * Usability: work-bag merge/dedup/subtract without EACH+HASLINE rebuild. */
    if (kw(&L->cur,"UNION") || kw(&L->cur,"ORLINES") || kw(&L->cur,"SETUNION") ||
        kw(&L->cur,"SETOR") || kw(&L->cur,"DISTINCT") || kw(&L->cur,"UNIQUEALL") ||
        kw(&L->cur,"UALL") || kw(&L->cur,"DEDUPALL") ||
        kw(&L->cur,"INTERSECT") || kw(&L->cur,"ANDLINES") || kw(&L->cur,"SETAND") ||
        kw(&L->cur,"SETINTERSECT") ||
        kw(&L->cur,"DIFF") || kw(&L->cur,"EXCEPT") || kw(&L->cur,"SETDIFF") ||
        kw(&L->cur,"MINUSLINES") || kw(&L->cur,"LINEDIFF") || kw(&L->cur,"BAGDIFF")){
      char op[24];
      int mode; /* 0=union/distinct 1=intersect 2=diff */
      enum { SET_MAX = 256, SET_FLEN = 192 };
      char fields[SET_MAX][SET_FLEN];
      char bag[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      int n = 0, i;
      size_t olen = 0;
      long kept = 0;
      const char *p, *start;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "INTERSECT") == 0 || strcmp(op, "ANDLINES") == 0 ||
          strcmp(op, "SETAND") == 0 || strcmp(op, "SETINTERSECT") == 0)
        mode = 1;
      else if (strcmp(op, "DIFF") == 0 || strcmp(op, "EXCEPT") == 0 ||
               strcmp(op, "SETDIFF") == 0 || strcmp(op, "MINUSLINES") == 0 ||
               strcmp(op, "LINEDIFF") == 0 || strcmp(op, "BAGDIFF") == 0)
        mode = 2;
      else
        mode = 0; /* UNION / DISTINCT */
      lex_next(L);
      out[0] = 0;
      n = 0;
      if (mode == 0) {
        /* UNION / DISTINCT: absorb all args; keep first-seen order */
        int any = 0;
        while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
               L->cur.kind == TK_NUM) {
          bag[0] = 0;
          if (L->cur.kind == TK_NUM) {
            snprintf(bag, sizeof bag, "%ld", L->cur.num);
            lex_next(L);
          } else if (resolve_str_arg(vm, L, bag, sizeof bag) != 0) {
            break;
          }
          any = 1;
          if (!bag[0]) continue;
          p = bag;
          while (*p && n < SET_MAX) {
            start = p;
            while (*p && *p != '\n') p++;
            if (start == p && *p == 0 && start > bag && start[-1] == '\n')
              break;
            {
              size_t flen = (size_t)(p - start);
              int seen = 0;
              if (flen >= SET_FLEN) flen = SET_FLEN - 1;
              for (i = 0; i < n; i++) {
                if (strlen(fields[i]) == flen &&
                    (flen == 0 || memcmp(fields[i], start, flen) == 0)) {
                  seen = 1;
                  break;
                }
              }
              if (!seen) {
                memcpy(fields[n], start, flen);
                fields[n][flen] = 0;
                n++;
              }
            }
            if (*p == '\n') p++;
          }
        }
        if (!any) {
          /* zero args → LAST bag */
          snprintf(bag, sizeof bag, "%s", vm->last_str);
          if (bag[0]) {
            p = bag;
            while (*p && n < SET_MAX) {
              start = p;
              while (*p && *p != '\n') p++;
              if (start == p && *p == 0 && start > bag && start[-1] == '\n')
                break;
              {
                size_t flen = (size_t)(p - start);
                int seen = 0;
                if (flen >= SET_FLEN) flen = SET_FLEN - 1;
                for (i = 0; i < n; i++) {
                  if (strlen(fields[i]) == flen &&
                      (flen == 0 || memcmp(fields[i], start, flen) == 0)) {
                    seen = 1;
                    break;
                  }
                }
                if (!seen) {
                  memcpy(fields[n], start, flen);
                  fields[n][flen] = 0;
                  n++;
                }
              }
              if (*p == '\n') p++;
            }
          }
        }
        for (i = 0; i < n; i++) {
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          {
            size_t flen = strlen(fields[i]);
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
          kept++;
        }
      } else {
        /* INTERSECT / DIFF: a then b */
        char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
        char bfields[SET_MAX][SET_FLEN];
        int bn = 0;
        a[0] = 0; b[0] = 0;
        if (resolve_str_arg(vm, L, a, sizeof a) != 0)
          snprintf(a, sizeof a, "%s", vm->last_str);
        if (resolve_str_arg(vm, L, b, sizeof b) != 0)
          b[0] = 0;
        /* index fields of b */
        if (b[0]) {
          p = b;
          while (*p && bn < SET_MAX) {
            start = p;
            while (*p && *p != '\n') p++;
            if (start == p && *p == 0 && start > b && start[-1] == '\n')
              break;
            {
              size_t flen = (size_t)(p - start);
              if (flen >= SET_FLEN) flen = SET_FLEN - 1;
              memcpy(bfields[bn], start, flen);
              bfields[bn][flen] = 0;
              bn++;
            }
            if (*p == '\n') p++;
          }
        }
        /* walk a; keep if (intersect && in b) or (diff && not in b); first-seen only */
        if (a[0]) {
          p = a;
          while (*p && n < SET_MAX) {
            start = p;
            while (*p && *p != '\n') p++;
            if (start == p && *p == 0 && start > a && start[-1] == '\n')
              break;
            {
              size_t flen = (size_t)(p - start);
              int in_b = 0, seen = 0;
              if (flen >= SET_FLEN) flen = SET_FLEN - 1;
              for (i = 0; i < bn; i++) {
                if (strlen(bfields[i]) == flen &&
                    (flen == 0 || memcmp(bfields[i], start, flen) == 0)) {
                  in_b = 1;
                  break;
                }
              }
              for (i = 0; i < n; i++) {
                if (strlen(fields[i]) == flen &&
                    (flen == 0 || memcmp(fields[i], start, flen) == 0)) {
                  seen = 1;
                  break;
                }
              }
              if (!seen && ((mode == 1 && in_b) || (mode == 2 && !in_b))) {
                memcpy(fields[n], start, flen);
                fields[n][flen] = 0;
                n++;
              }
            }
            if (*p == '\n') p++;
          }
        }
        for (i = 0; i < n; i++) {
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          {
            size_t flen = strlen(fields[i]);
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
          kept++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "UNION_N", kept);
      var_set_num(vm, "SET_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS ZIP|PAIR|ZIPLINES a b [sep] — pair newline fields by index with sep → LAST.
     * Default sep ":". Shorter bag pads empty partner. LAST_N = pair count.
     * SYS KEYS|COL0|LEFTCOL bag [sep] — peel left of first sep each field → bag.
     * SYS VALS|COL1|RIGHTCOL bag [sep] — peel right of first sep each field → bag.
     * Usability: roster name+status plates without EACH+CAT glue; peel kv bags. */
    if (kw(&L->cur,"ZIP") || kw(&L->cur,"PAIR") || kw(&L->cur,"ZIPLINES") ||
        kw(&L->cur,"ZIPL") || kw(&L->cur,"PAIRS") ||
        kw(&L->cur,"KEYS") || kw(&L->cur,"COL0") || kw(&L->cur,"LEFTCOL") ||
        kw(&L->cur,"KEYCOL") || kw(&L->cur,"KVKEYS") ||
        kw(&L->cur,"VALS") || kw(&L->cur,"COL1") || kw(&L->cur,"RIGHTCOL") ||
        kw(&L->cur,"VALCOL") || kw(&L->cur,"KVVALS") || kw(&L->cur,"VALUES")){
      char op[20];
      int mode; /* 0=zip 1=keys 2=vals */
      enum { ZIP_MAX = 256, ZIP_FLEN = 192 };
      char af[ZIP_MAX][ZIP_FLEN], bf[ZIP_MAX][ZIP_FLEN];
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
      char sep[32], out[CUBALC_HOST_STR_MAX];
      int na = 0, nb = 0, n = 0, i;
      size_t olen = 0, sepn;
      const char *p, *start;
      long kept = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "KEYS") == 0 || strcmp(op, "COL0") == 0 ||
          strcmp(op, "LEFTCOL") == 0 || strcmp(op, "KEYCOL") == 0 ||
          strcmp(op, "KVKEYS") == 0)
        mode = 1;
      else if (strcmp(op, "VALS") == 0 || strcmp(op, "COL1") == 0 ||
               strcmp(op, "RIGHTCOL") == 0 || strcmp(op, "VALCOL") == 0 ||
               strcmp(op, "KVVALS") == 0 || strcmp(op, "VALUES") == 0)
        mode = 2;
      else
        mode = 0;
      lex_next(L);
      a[0] = 0; b[0] = 0; sep[0] = ':'; sep[1] = 0; out[0] = 0;
      if (mode == 0) {
        if (resolve_str_arg(vm, L, a, sizeof a) != 0)
          snprintf(a, sizeof a, "%s", vm->last_str);
        if (resolve_str_arg(vm, L, b, sizeof b) != 0)
          b[0] = 0;
        /* optional sep: string only (avoid eating next form); default ":" */
        if (L->cur.kind == TK_STR) {
          if (resolve_str_arg(vm, L, sep, sizeof sep) != 0)
            { sep[0] = ':'; sep[1] = 0; }
          if (!sep[0]) { sep[0] = ':'; sep[1] = 0; }
        }
        /* split a and b into fields */
        if (a[0]) {
          p = a;
          while (*p && na < ZIP_MAX) {
            start = p;
            while (*p && *p != '\n') p++;
            if (start == p && *p == 0 && start > a && start[-1] == '\n') break;
            {
              size_t flen = (size_t)(p - start);
              if (flen >= ZIP_FLEN) flen = ZIP_FLEN - 1;
              memcpy(af[na], start, flen);
              af[na][flen] = 0;
              na++;
            }
            if (*p == '\n') p++;
          }
        }
        if (b[0]) {
          p = b;
          while (*p && nb < ZIP_MAX) {
            start = p;
            while (*p && *p != '\n') p++;
            if (start == p && *p == 0 && start > b && start[-1] == '\n') break;
            {
              size_t flen = (size_t)(p - start);
              if (flen >= ZIP_FLEN) flen = ZIP_FLEN - 1;
              memcpy(bf[nb], start, flen);
              bf[nb][flen] = 0;
              nb++;
            }
            if (*p == '\n') p++;
          }
        }
        n = na > nb ? na : nb;
        sepn = strlen(sep);
        for (i = 0; i < n; i++) {
          const char *la = (i < na) ? af[i] : "";
          const char *lb = (i < nb) ? bf[i] : "";
          size_t fla = strlen(la), flb = strlen(lb);
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + fla < sizeof out) {
            memcpy(out + olen, la, fla);
            olen += fla;
          } else if (olen < sizeof out - 1) {
            size_t take = sizeof out - 1 - olen;
            memcpy(out + olen, la, take);
            olen += take;
          }
          if (olen + sepn < sizeof out) {
            memcpy(out + olen, sep, sepn);
            olen += sepn;
          }
          if (olen + flb < sizeof out) {
            memcpy(out + olen, lb, flb);
            olen += flb;
          } else if (olen < sizeof out - 1) {
            size_t take = sizeof out - 1 - olen;
            memcpy(out + olen, lb, take);
            olen += take;
          }
          out[olen] = 0;
          kept++;
        }
      } else {
        /* KEYS / VALS: peel each field at first sep */
        if (resolve_str_arg(vm, L, a, sizeof a) != 0)
          snprintf(a, sizeof a, "%s", vm->last_str);
        if (L->cur.kind == TK_STR) {
          if (resolve_str_arg(vm, L, sep, sizeof sep) != 0)
            { sep[0] = ':'; sep[1] = 0; }
          if (!sep[0]) { sep[0] = ':'; sep[1] = 0; }
        }
        sepn = strlen(sep);
        if (a[0]) {
          p = a;
          while (*p) {
            const char *hit = 0;
            size_t flen, take;
            start = p;
            while (*p && *p != '\n') p++;
            flen = (size_t)(p - start);
            /* find first sep in field */
            if (sepn == 0) {
              hit = start; /* empty sep: all left empty right? treat as no sep */
              hit = 0;
            } else if (flen >= sepn) {
              size_t k;
              for (k = 0; k + sepn <= flen; k++) {
                if (memcmp(start + k, sep, sepn) == 0) {
                  hit = start + k;
                  break;
                }
              }
            }
            if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (mode == 1) {
              /* keys: left of sep, or whole field if no sep */
              take = hit ? (size_t)(hit - start) : flen;
              if (olen + take < sizeof out) {
                memcpy(out + olen, start, take);
                olen += take;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, start, t);
                olen += t;
              }
            } else {
              /* vals: right of sep, or empty if no sep */
              if (hit) {
                const char *rs = hit + sepn;
                take = (size_t)((start + flen) - rs);
                if (olen + take < sizeof out) {
                  memcpy(out + olen, rs, take);
                  olen += take;
                } else if (olen < sizeof out - 1) {
                  size_t t = sizeof out - 1 - olen;
                  memcpy(out + olen, rs, t);
                  olen += t;
                }
              }
              /* else empty field */
            }
            out[olen] = 0;
            kept++;
            if (*p == '\n') p++;
          }
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "ZIP_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS PREFIXALL|PREPENDALL|MAPPRE bag prefix — prepend string to every field → LAST.
     * SYS SUFFIXALL|APPENDALL|MAPSUF bag suffix — append string to every field → LAST.
     * LAST_N = field count. Empty bag → "" OK=1.
     * Usability: tag LIST/peer bags (path/, .json) without EACH+CAT rebuild. */
    if (kw(&L->cur,"PREFIXALL") || kw(&L->cur,"PREPENDALL") || kw(&L->cur,"MAPPRE") ||
        kw(&L->cur,"PREALL") || kw(&L->cur,"ADDFRONT") || kw(&L->cur,"TAGPRE") ||
        kw(&L->cur,"SUFFIXALL") || kw(&L->cur,"APPENDALL") || kw(&L->cur,"MAPSUF") ||
        kw(&L->cur,"SUFALL") || kw(&L->cur,"ADDEND") || kw(&L->cur,"TAGSUF") ||
        kw(&L->cur,"POSTFIXALL")){
      char op[20];
      int is_suf;
      char bag[CUBALC_HOST_STR_MAX], affix[512], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t olen = 0, alen, flen;
      long kept = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_suf = (strcmp(op, "SUFFIXALL") == 0 || strcmp(op, "APPENDALL") == 0 ||
                strcmp(op, "MAPSUF") == 0 || strcmp(op, "SUFALL") == 0 ||
                strcmp(op, "ADDEND") == 0 || strcmp(op, "TAGSUF") == 0 ||
                strcmp(op, "POSTFIXALL") == 0);
      lex_next(L);
      bag[0] = 0; affix[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, affix, sizeof affix) != 0)
        affix[0] = 0;
      alen = strlen(affix);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (!is_suf) {
            /* prefix + field */
            if (olen + alen < sizeof out) {
              memcpy(out + olen, affix, alen);
              olen += alen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, affix, t);
              olen += t;
            }
            if (olen + flen < sizeof out) {
              memcpy(out + olen, start, flen);
              olen += flen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, start, t);
              olen += t;
            }
          } else {
            /* field + suffix */
            if (olen + flen < sizeof out) {
              memcpy(out + olen, start, flen);
              olen += flen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, start, t);
              olen += t;
            }
            if (olen + alen < sizeof out) {
              memcpy(out + olen, affix, alen);
              olen += alen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, affix, t);
              olen += t;
            }
          }
          out[olen] = 0;
          kept++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "PREFIXALL_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS FILL|REPEATL|REPEATS|BAGFILL n value — bag of n copies of value → LAST.
     * n capped 0..256; n<=0 → empty. Value may be str/ident/num (num stringified).
     * Usability: default status columns for ZIP / pad peer bags without EACH. */
    if (kw(&L->cur,"FILL") || kw(&L->cur,"REPEATL") || kw(&L->cur,"REPEATS") ||
        kw(&L->cur,"BAGFILL") || kw(&L->cur,"FILLBAG") || kw(&L->cur,"NFILL") ||
        kw(&L->cur,"TIMESLINE") || kw(&L->cur,"DUPFIELD")){
      long n = 0;
      int i;
      char val[512], out[CUBALC_HOST_STR_MAX], nbuf[40];
      size_t olen = 0, vlen;
      long kept = 0;
      lex_next(L);
      out[0] = 0; val[0] = 0;
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS) {
        n = parse_prim(vm, L);
      } else {
        n = vm->last_n;
      }
      if (n < 0) n = 0;
      if (n > 256) n = 256;
      if (L->cur.kind == TK_NUM) {
        snprintf(val, sizeof val, "%ld", L->cur.num);
        lex_next(L);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, val, sizeof val) != 0)
          val[0] = 0;
      } else if (vm->last_str[0]) {
        snprintf(val, sizeof val, "%s", vm->last_str);
      } else {
        snprintf(val, sizeof val, "%ld", vm->last_n);
      }
      vlen = strlen(val);
      for (i = 0; i < (int)n; i++) {
        if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        if (olen + vlen < sizeof out) {
          memcpy(out + olen, val, vlen);
          olen += vlen;
        } else if (olen < sizeof out - 1) {
          size_t t = sizeof out - 1 - olen;
          memcpy(out + olen, val, t);
          olen += t;
        }
        out[olen] = 0;
        kept++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "FILL_N", kept);
      snprintf(nbuf, sizeof nbuf, "%ld", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS ENUMERATE|NUMBER|INDEXLINES|NL|ENUM bag [start] [sep]
     * — prefix each field with start+i and sep (default start=0, sep=":").
     * LAST_N = field count. Empty bag → "" OK=1.
     * Usability: ranked/priority plates without EACH+STR+CAT index glue. */
    /* Note: SYS ENUM is IOTA alias (RANGE plane) — do not claim ENUM here. */
    if (kw(&L->cur,"ENUMERATE") || kw(&L->cur,"NUMBER") || kw(&L->cur,"INDEXLINES") ||
        kw(&L->cur,"NUMBERLINES") || kw(&L->cur,"NLNUM") ||
        kw(&L->cur,"IDXLINES") || kw(&L->cur,"WITHINDEX") || kw(&L->cur,"RANKLINES")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], out[CUBALC_HOST_STR_MAX], ibuf[40];
      const char *p, *start;
      size_t olen = 0, sepn, flen, ilen;
      long kept = 0, base = 0, idx;
      lex_next(L);
      bag[0] = 0; sep[0] = ':'; sep[1] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      /* optional start index (num/ident/paren); then optional sep string */
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS) {
        /* Ambiguity: IDENT could be next form. Only treat as start if num/minus/paren
         * or IDENT that resolves as number var. Prefer: num or minus always start;
         * IDENT only if looks numeric after resolve as number via parse_prim when
         * next is also present or kind is clearly numeric. */
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
            L->cur.kind == TK_LPAREN) {
          base = parse_prim(vm, L);
        } else if (L->cur.kind == TK_IDENT) {
          /* peek: if IDENT is a known numeric-ish we still parse_prim (var num) */
          base = parse_prim(vm, L);
        }
      }
      if (L->cur.kind == TK_STR) {
        if (resolve_str_arg(vm, L, sep, sizeof sep) != 0)
          { sep[0] = ':'; sep[1] = 0; }
        if (!sep[0]) { sep[0] = ':'; sep[1] = 0; }
      }
      sepn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          idx = base + kept;
          snprintf(ibuf, sizeof ibuf, "%ld", idx);
          ilen = strlen(ibuf);
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + ilen < sizeof out) {
            memcpy(out + olen, ibuf, ilen);
            olen += ilen;
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            memcpy(out + olen, ibuf, t);
            olen += t;
          }
          if (olen + sepn < sizeof out) {
            memcpy(out + olen, sep, sepn);
            olen += sepn;
          }
          if (olen + flen < sizeof out) {
            memcpy(out + olen, start, flen);
            olen += flen;
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            memcpy(out + olen, start, t);
            olen += t;
          }
          out[olen] = 0;
          kept++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "ENUM_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SQUEEZE|COMPACT|DROPEMPTY [BLANK|NB] [bag]
     * — drop empty newline fields (and whitespace-only if BLANK/NB).
     * LAST_N = kept count. Usability: clean VALS/SPLIT/LIST bags without EACH. */
    if (kw(&L->cur,"SQUEEZE") || kw(&L->cur,"COMPACT") || kw(&L->cur,"DROPEMPTY") ||
        kw(&L->cur,"DROPBLANK") || kw(&L->cur,"TRIMLINES") || kw(&L->cur,"STRIPLINES") ||
        kw(&L->cur,"NOTEMPTYL") || kw(&L->cur,"KEEPEMPTY0")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t olen = 0, flen;
      long kept = 0;
      int drop_blank = 0;
      char op[20];
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "DROPBLANK") == 0 || strcmp(op, "TRIMLINES") == 0 ||
          strcmp(op, "STRIPLINES") == 0)
        drop_blank = 1;
      lex_next(L);
      if (kw(&L->cur,"BLANK") || kw(&L->cur,"NB") || kw(&L->cur,"WS") ||
          kw(&L->cur,"WHITESPACE") || kw(&L->cur,"TRIM") || kw(&L->cur,"NONBLANK")) {
        drop_blank = 1;
        lex_next(L);
      }
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (bag[0]) {
        p = bag;
        while (*p) {
          int drop = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen == 0) {
            drop = 1;
          } else if (drop_blank) {
            size_t i;
            drop = 1;
            for (i = 0; i < flen; i++) {
              char c = start[i];
              if (c != ' ' && c != '\t' && c != '\r') { drop = 0; break; }
            }
          }
          if (!drop) {
            if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + flen < sizeof out) {
              memcpy(out + olen, start, flen);
              olen += flen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, start, t);
              olen += t;
            }
            out[olen] = 0;
            kept++;
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "SQUEEZE_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS TRIMALL|MAPTRIM|STRIPALL [L|R|BOTH] [bag]
     * — trim leading/trailing whitespace on every newline field → LAST.
     * Default BOTH (LTRIM+RTRIM). LAST_N = field count (empty fields kept).
     * Usability: clean plate lines after READ/SPLIT before SQUEEZE/EQS. */
    if (kw(&L->cur,"TRIMALL") || kw(&L->cur,"MAPTRIM") || kw(&L->cur,"STRIPALL") ||
        kw(&L->cur,"WSALL") || kw(&L->cur,"LTRIMALL") || kw(&L->cur,"RTRIMALL")){
      char op[20];
      int do_l = 1, do_r = 1;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      const char *p, *start, *fe, *fs;
      size_t olen = 0, flen;
      long kept = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "LTRIMALL") == 0) { do_l = 1; do_r = 0; }
      else if (strcmp(op, "RTRIMALL") == 0) { do_l = 0; do_r = 1; }
      lex_next(L);
      if (kw(&L->cur,"L") || kw(&L->cur,"LEFT") || kw(&L->cur,"LTRIM") ||
          kw(&L->cur,"LEADING")) {
        do_l = 1; do_r = 0; lex_next(L);
      } else if (kw(&L->cur,"R") || kw(&L->cur,"RIGHT") || kw(&L->cur,"RTRIM") ||
                 kw(&L->cur,"TRAILING")) {
        do_l = 0; do_r = 1; lex_next(L);
      } else if (kw(&L->cur,"BOTH") || kw(&L->cur,"LR") || kw(&L->cur,"FULL")) {
        do_l = 1; do_r = 1; lex_next(L);
      }
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          fs = start;
          fe = p;
          if (do_l) {
            while (fs < fe && (*fs == ' ' || *fs == '\t' || *fs == '\r')) fs++;
          }
          if (do_r) {
            while (fe > fs && (fe[-1] == ' ' || fe[-1] == '\t' || fe[-1] == '\r'))
              fe--;
          }
          flen = (size_t)(fe - fs);
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + flen < sizeof out) {
            memcpy(out + olen, fs, flen);
            olen += flen;
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            memcpy(out + olen, fs, t);
            olen += t;
          }
          out[olen] = 0;
          kept++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "TRIMALL_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS UPPERALL|MAPUPPER|UCASEALL [bag] — ASCII upper every newline field → LAST.
     * SYS LOWERALL|MAPLOWER|LCASEALL [bag] — ASCII lower every field.
     * LAST_N = field count. Usability: normalize LIST/severity bags before HASLINE/set ops
     * without EACH LINE + UPPER/LOWER glue. */
    if (kw(&L->cur,"UPPERALL") || kw(&L->cur,"MAPUPPER") || kw(&L->cur,"UCASEALL") ||
        kw(&L->cur,"TOUPPERALL") || kw(&L->cur,"UPCASEALL") ||
        kw(&L->cur,"LOWERALL") || kw(&L->cur,"MAPLOWER") || kw(&L->cur,"LCASEALL") ||
        kw(&L->cur,"TOLOWERALL") || kw(&L->cur,"DOWNCASEALL")){
      char op[20];
      int to_upper = 1;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t olen = 0, flen, i;
      long kept = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "LOWERALL") == 0 || strcmp(op, "MAPLOWER") == 0 ||
          strcmp(op, "LCASEALL") == 0 || strcmp(op, "TOLOWERALL") == 0 ||
          strcmp(op, "DOWNCASEALL") == 0)
        to_upper = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + flen < sizeof out) {
            for (i = 0; i < flen; i++) {
              char c = start[i];
              if (to_upper) {
                if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
              } else {
                if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
              }
              out[olen++] = c;
            }
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            for (i = 0; i < t; i++) {
              char c = start[i];
              if (to_upper) {
                if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
              } else {
                if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
              }
              out[olen++] = c;
            }
          }
          out[olen] = 0;
          kept++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "UPPERALL_N", kept);
      var_set_num(vm, "LOWERALL_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MAPREPLACE|GSUBALL|REPLACEBAG bag old new — REPLACEALL on every field.
     * Empty old → no-op (bag unchanged). LAST = bag; LAST_N/MAPREPLACE_N = total
     * replacements; MAPREPLACE_FIELDS = field count.
     * Usability: rewrite path prefixes / tags on LIST bags without EACH+REPLACEALL. */
    if (kw(&L->cur,"MAPREPLACE") || kw(&L->cur,"GSUBALL") || kw(&L->cur,"REPLACEBAG") ||
        kw(&L->cur,"REPLACEALLBAG") || kw(&L->cur,"BAGREPLACE") || kw(&L->cur,"SUBALLBAG") ||
        kw(&L->cur,"MAPGSUB") || kw(&L->cur,"MAPSUBST")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      char olds[256], news[256];
      const char *p, *start, *src;
      size_t olen = 0, flen, oldn, newn, pre, rest, take;
      long fields = 0, did = 0;
      char field[512], fbuf[768];
      size_t fo;
      lex_next(L);
      bag[0] = 0; out[0] = 0; olds[0] = 0; news[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, olds, sizeof olds) != 0) olds[0] = 0;
      if (resolve_str_arg(vm, L, news, sizeof news) != 0) news[0] = 0;
      oldn = strlen(olds);
      newn = strlen(news);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          take = flen;
          if (take >= sizeof field) take = sizeof field - 1;
          memcpy(field, start, take);
          field[take] = 0;
          /* apply REPLACEALL on this field */
          fo = 0;
          fbuf[0] = 0;
          if (oldn == 0) {
            if (take < sizeof fbuf) {
              memcpy(fbuf, field, take);
              fo = take;
              fbuf[fo] = 0;
            }
          } else {
            src = field;
            for (;;) {
              const char *hit = strstr(src, olds);
              if (!hit) {
                rest = strlen(src);
                if (fo + rest >= sizeof fbuf) rest = sizeof fbuf - 1 - fo;
                memcpy(fbuf + fo, src, rest);
                fo += rest;
                fbuf[fo] = 0;
                break;
              }
              pre = (size_t)(hit - src);
              if (fo + pre >= sizeof fbuf) pre = sizeof fbuf - 1 - fo;
              memcpy(fbuf + fo, src, pre);
              fo += pre;
              if (fo + newn < sizeof fbuf) {
                memcpy(fbuf + fo, news, newn);
                fo += newn;
              } else if (fo < sizeof fbuf - 1) {
                size_t nt = sizeof fbuf - 1 - fo;
                memcpy(fbuf + fo, news, nt);
                fo += nt;
              }
              fbuf[fo] = 0;
              did++;
              src = hit + oldn;
              if (fo >= sizeof fbuf - 1) break;
            }
          }
          if (fields > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          {
            size_t n = fo;
            if (olen + n < sizeof out) {
              memcpy(out + olen, fbuf, n);
              olen += n;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, fbuf, t);
              olen += t;
            }
            out[olen] = 0;
          }
          fields++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = did;
      var_set_num(vm, "LAST_N", did);
      var_set_num(vm, "MAPREPLACE_N", did);
      var_set_num(vm, "MAPREPLACE_FIELDS", fields);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS FREQ|HIST|COUNTS|TALLY [bag] [sep] — frequency of each exact field.
     * LAST = "key{sep}count" bag (first-seen order). Default sep ":".
     * LAST_N/FREQ_N = unique keys; FREQ_TOTAL = input field count (cap 64 uniques).
     * Usability: status/severity rollups without EACH LINE + COUNTLINE rebuild. */
    if (kw(&L->cur,"FREQ") || kw(&L->cur,"HIST") || kw(&L->cur,"COUNTS") ||
        kw(&L->cur,"FREQUENCY") || kw(&L->cur,"HISTOGRAM") ||
        kw(&L->cur,"COUNTBAG") || kw(&L->cur,"BAGCOUNT") || kw(&L->cur,"FREQBAG")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], sep[16];
      char keys[64][128];
      long counts[64];
      int nk = 0, k;
      const char *p, *start;
      size_t flen, olen = 0;
      long total = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      /* optional sep if next is a short string (not a form keyword start) */
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      memset(counts, 0, sizeof counts);
      if (bag[0]) {
        p = bag;
        while (*p) {
          char field[128];
          size_t take;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          take = flen;
          if (take >= sizeof field) take = sizeof field - 1;
          memcpy(field, start, take);
          field[take] = 0;
          total++;
          for (k = 0; k < nk; k++) {
            if (strcmp(keys[k], field) == 0) {
              counts[k]++;
              break;
            }
          }
          if (k == nk && nk < 64) {
            snprintf(keys[nk], sizeof keys[0], "%s", field);
            counts[nk] = 1;
            nk++;
          } else if (k == nk) {
            /* overflow uniques: drop further new keys but still count total */
          }
          if (*p == '\n') p++;
        }
      }
      for (k = 0; k < nk; k++) {
        char line[160];
        int n;
        n = snprintf(line, sizeof line, "%s%s%ld", keys[k], sep, counts[k]);
        if (n < 0) n = 0;
        if ((size_t)n >= sizeof line) n = (int)sizeof line - 1;
        if (k > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        if (olen + (size_t)n < sizeof out) {
          memcpy(out + olen, line, (size_t)n);
          olen += (size_t)n;
        } else if (olen < sizeof out - 1) {
          size_t t = sizeof out - 1 - olen;
          memcpy(out + olen, line, t);
          olen += t;
        }
        out[olen] = 0;
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "FREQ", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = nk;
      var_set_num(vm, "LAST_N", nk);
      var_set_num(vm, "FREQ_N", nk);
      var_set_num(vm, "FREQ_TOTAL", total);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SORTFREQ|SORTBYCOUNT|FSORT [bag] [sep] [DESC|ASC]
     * — sort "key{sep}count" FREQ lines by numeric count (default sep ":", DESC).
     * Count is digits after last sep on each line. Stable for equal counts.
     * LAST_N = line count. Usability: top severities after FREQ without shell sort -n. */
    if (kw(&L->cur,"SORTFREQ") || kw(&L->cur,"SORTBYCOUNT") || kw(&L->cur,"FSORT") ||
        kw(&L->cur,"FREQSORT") || kw(&L->cur,"SORTCOUNTS") || kw(&L->cur,"RANKFREQ") ||
        kw(&L->cur,"TOPFREQ") || kw(&L->cur,"HEAVYFIRST")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], sep[16];
      char lines[64][160];
      long counts[64];
      int order[64];
      int n = 0, i, j, desc = 1;
      const char *p, *start;
      size_t sepn, olen = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      /* optional sep (string) and/or DESC|ASC */
      for (;;) {
        if (L->cur.kind == TK_STR) {
          snprintf(sep, sizeof sep, "%s", L->cur.text);
          if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
          lex_next(L);
          continue;
        }
        if (kw(&L->cur,"DESC") || kw(&L->cur,"DOWN") || kw(&L->cur,"REV") ||
            kw(&L->cur,"HEAVY") || kw(&L->cur,"HIGH")) {
          desc = 1; lex_next(L); continue;
        }
        if (kw(&L->cur,"ASC") || kw(&L->cur,"UP") || kw(&L->cur,"LIGHT") ||
            kw(&L->cur,"LOW") || kw(&L->cur,"ASCENDING")) {
          desc = 0; lex_next(L); continue;
        }
        break;
      }
      sepn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p && n < 64) {
          size_t flen, take;
          const char *last;
          char *endc;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          take = flen;
          if (take >= sizeof lines[0]) take = sizeof lines[0] - 1;
          memcpy(lines[n], start, take);
          lines[n][take] = 0;
          /* count = number after last sep */
          counts[n] = 0;
          if (sepn == 0) {
            counts[n] = strtol(lines[n], &endc, 10);
          } else {
            last = NULL;
            {
              const char *q = lines[n];
              while (*q) {
                if (strncmp(q, sep, sepn) == 0) last = q;
                q++;
              }
            }
            if (last) counts[n] = strtol(last + sepn, &endc, 10);
            else counts[n] = strtol(lines[n], &endc, 10);
          }
          order[n] = n;
          n++;
          if (*p == '\n') p++;
        }
      }
      /* stable insertion sort by count */
      for (i = 1; i < n; i++) {
        int oi = order[i];
        long ci = counts[oi];
        j = i - 1;
        while (j >= 0) {
          long cj = counts[order[j]];
          int swap = desc ? (cj < ci) : (cj > ci);
          if (!swap) break;
          order[j + 1] = order[j];
          j--;
        }
        order[j + 1] = oi;
      }
      for (i = 0; i < n; i++) {
        size_t ln = strlen(lines[order[i]]);
        if (i > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        if (olen + ln < sizeof out) {
          memcpy(out + olen, lines[order[i]], ln);
          olen += ln;
        } else if (olen < sizeof out - 1) {
          size_t t = sizeof out - 1 - olen;
          memcpy(out + olen, lines[order[i]], t);
          olen += t;
        }
        out[olen] = 0;
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "FREQ", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "SORTFREQ_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS BEFOREALL|MAPBEFORE bag needle — BEFORE on every bag field → LAST bag.
     * SYS AFTERALL|MAPAFTER bag needle — AFTER on every field.
     * Same miss semantics as BEFORE/AFTER (before→whole field, after→empty).
     * LAST_N = field count; BEFOREALL_HIT = how many fields contained needle.
     * Usability: FREQ key/count peel or kv bags without EACH+BEFORE/AFTER. */
    if (kw(&L->cur,"BEFOREALL") || kw(&L->cur,"MAPBEFORE") || kw(&L->cur,"LEFTALL") ||
        kw(&L->cur,"PREALL") || kw(&L->cur,"BAGBEFORE") ||
        kw(&L->cur,"AFTERALL") || kw(&L->cur,"MAPAFTER") || kw(&L->cur,"RIGHTALL") ||
        kw(&L->cur,"POSTALL") || kw(&L->cur,"BAGAFTER")){
      char op[20];
      int want_after = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], needle[256];
      const char *p, *start;
      size_t olen = 0, flen, nn, take;
      long lines = 0, hits = 0;
      char field[512], cell[512];
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "AFTERALL") == 0 || strcmp(op, "MAPAFTER") == 0 ||
          strcmp(op, "RIGHTALL") == 0 || strcmp(op, "POSTALL") == 0 ||
          strcmp(op, "BAGAFTER") == 0)
        want_after = 1;
      lex_next(L);
      bag[0] = 0; out[0] = 0; needle[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0] = 0;
      nn = strlen(needle);
      if (bag[0]) {
        p = bag;
        while (*p) {
          const char *hit;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          take = flen;
          if (take >= sizeof field) take = sizeof field - 1;
          memcpy(field, start, take);
          field[take] = 0;
          cell[0] = 0;
          if (nn == 0) {
            hits++;
            if (want_after) snprintf(cell, sizeof cell, "%s", field);
            else cell[0] = 0;
          } else {
            hit = strstr(field, needle);
            if (!hit) {
              if (want_after) cell[0] = 0;
              else snprintf(cell, sizeof cell, "%s", field);
            } else {
              hits++;
              if (want_after) {
                snprintf(cell, sizeof cell, "%s", hit + nn);
              } else {
                size_t pre = (size_t)(hit - field);
                if (pre >= sizeof cell) pre = sizeof cell - 1;
                memcpy(cell, field, pre);
                cell[pre] = 0;
              }
            }
          }
          if (lines > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          {
            size_t n = strlen(cell);
            if (olen + n < sizeof out) {
              memcpy(out + olen, cell, n);
              olen += n;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, cell, t);
              olen += t;
            }
            out[olen] = 0;
          }
          lines++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = lines;
      var_set_num(vm, "LAST_N", lines);
      var_set_num(vm, "BEFOREALL_N", lines);
      var_set_num(vm, "AFTERALL_N", lines);
      var_set_num(vm, "BEFOREALL_HIT", hits);
      var_set_num(vm, "AFTERALL_HIT", hits);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS FIRSTMATCH|GREP1|FINDFIELD [I] bag needle — first field containing needle.
     * SYS FIRSTMATCHI — case-insensitive. LAST = field (empty if miss); LAST_N 0|1.
     * FIRSTMATCH_I = 0-based index of hit (-1 miss). Empty needle → first field.
     * Usability: pick one log/peer line without GREP+TAKE 1 glue. */
    if (kw(&L->cur,"FIRSTMATCH") || kw(&L->cur,"GREP1") || kw(&L->cur,"FINDFIELD") ||
        kw(&L->cur,"FIRSTGREP") || kw(&L->cur,"MATCH1") || kw(&L->cur,"FINDLINE_SUB") ||
        kw(&L->cur,"FIRSTMATCHI") || kw(&L->cur,"GREP1I") || kw(&L->cur,"FINDFIELDI")){
      char op[20];
      int icase = 0;
      char bag[CUBALC_HOST_STR_MAX], needle[256], out[512];
      const char *p, *start;
      size_t flen, nn;
      long idx = 0, found = 0, found_i = -1;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "FIRSTMATCHI") == 0 || strcmp(op, "GREP1I") == 0 ||
          strcmp(op, "FINDFIELDI") == 0)
        icase = 1;
      lex_next(L);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      bag[0] = 0; needle[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0] = 0;
      nn = strlen(needle);
      if (bag[0]) {
        p = bag;
        while (*p) {
          int hit = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            char field[512];
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            if (nn == 0) {
              hit = 1;
            } else if (!icase) {
              if (strstr(field, needle) != NULL) hit = 1;
            } else {
              size_t fi, j;
              for (fi = 0; field[fi] && !hit; fi++) {
                for (j = 0; j < nn; j++) {
                  char a = field[fi + j], b = needle[j];
                  if (!a) break;
                  if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                  if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                  if (a != b) break;
                }
                if (j == nn) hit = 1;
              }
            }
            if (hit) {
              if (flen >= sizeof out) flen = sizeof out - 1;
              memcpy(out, start, flen);
              out[flen] = 0;
              found = 1;
              found_i = idx;
              break;
            }
          }
          idx++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "GREP", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      var_set_num(vm, "FIRSTMATCH_N", found);
      var_set_num(vm, "FIRSTMATCH_I", found_i);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* bag-KV theme → lang_ops_sys_kv.c (mesh vote A2) */
    {
      int kr = cubalc_lang_ops_sys_try_kv(vm, L);
      if (kr != 0) return kr;
    }

    /* bag-KV math theme → lang_ops_sys_kvmath.c (mesh vote A2) */
    {
      int km = cubalc_lang_ops_sys_try_kvmath(vm, L);
      if (km != 0) return km;
    }

    /* bag-KV filter theme → lang_ops_sys_kvfilter.c (mesh vote A2) */
    {
      int kf = cubalc_lang_ops_sys_try_kvfilter(vm, L);
      if (kf != 0) return kf;
    }

    /* SYS LASTMATCH|GREP1L|FINDFIELDL [I] bag needle — last field containing needle.
     * SYS LASTMATCHI — case-insensitive. LAST = field (empty if miss); LAST_N 0|1.
     * LASTMATCH_I = 0-based index of hit (-1 miss). Empty needle → last field.
     * Usability: latest log/error line without REVL+FIRSTMATCH or GREP+TAIL glue. */
    if (kw(&L->cur,"LASTMATCH") || kw(&L->cur,"GREP1L") || kw(&L->cur,"FINDFIELDL") ||
        kw(&L->cur,"LASTGREP") || kw(&L->cur,"MATCHLAST") || kw(&L->cur,"GREPLAST") ||
        kw(&L->cur,"LASTMATCHI") || kw(&L->cur,"GREP1LI") || kw(&L->cur,"FINDFIELDLI")){
      char op[20];
      int icase = 0;
      char bag[CUBALC_HOST_STR_MAX], needle[256], out[512];
      const char *p, *start;
      size_t flen, nn;
      long idx = 0, found = 0, found_i = -1;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "LASTMATCHI") == 0 || strcmp(op, "GREP1LI") == 0 ||
          strcmp(op, "FINDFIELDLI") == 0)
        icase = 1;
      lex_next(L);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      bag[0] = 0; needle[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0] = 0;
      nn = strlen(needle);
      if (bag[0]) {
        p = bag;
        while (*p) {
          int hit = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            char field[512];
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            if (nn == 0) {
              hit = 1;
            } else if (!icase) {
              if (strstr(field, needle) != NULL) hit = 1;
            } else {
              size_t fi, j;
              for (fi = 0; field[fi] && !hit; fi++) {
                for (j = 0; j < nn; j++) {
                  char a = field[fi + j], b = needle[j];
                  if (!a) break;
                  if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                  if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                  if (a != b) break;
                }
                if (j == nn) hit = 1;
              }
            }
            if (hit) {
              /* keep scanning — last hit wins */
              if (flen >= sizeof out) flen = sizeof out - 1;
              memcpy(out, start, flen);
              out[flen] = 0;
              found = 1;
              found_i = idx;
            }
          }
          idx++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "GREP", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      var_set_num(vm, "LASTMATCH_N", found);
      var_set_num(vm, "LASTMATCH_I", found_i);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS CHUNK|BATCH|GROUPN n [bag] [join_sep]
     * — group bag fields into batches of n; join each batch with join_sep
     * (default space); batches become newline fields → LAST.
     * n<=0 → empty. LAST_N/CHUNK_N = batch count; CHUNK_TOTAL = input fields.
     * Usability: peer/work list paging without EACH+CAT index glue. */
    if (kw(&L->cur,"CHUNK") || kw(&L->cur,"BATCH") || kw(&L->cur,"GROUPN") ||
        kw(&L->cur,"BUNDLE") || kw(&L->cur,"PARTN") || kw(&L->cur,"SPLITN") ||
        kw(&L->cur,"NCHUNK") || kw(&L->cur,"BATCHLINES")){
      long nwant = 0, total = 0, batches = 0, in_batch = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], jsep[32];
      char batch[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t olen = 0, blen = 0, flen, jsepn;
      lex_next(L);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
        nwant = parse_expr(vm, L);
      else
        nwant = 0;
      bag[0] = 0; out[0] = 0; batch[0] = 0;
      snprintf(jsep, sizeof jsep, "%s", " ");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_STR) {
        snprintf(jsep, sizeof jsep, "%s", L->cur.text);
        lex_next(L);
      }
      jsepn = strlen(jsep);
      if (nwant > 0 && bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (in_batch > 0) {
            if (blen + jsepn < sizeof batch) {
              memcpy(batch + blen, jsep, jsepn);
              blen += jsepn;
            }
          }
          if (blen + flen < sizeof batch) {
            memcpy(batch + blen, start, flen);
            blen += flen;
          } else if (blen < sizeof batch - 1) {
            size_t t = sizeof batch - 1 - blen;
            memcpy(batch + blen, start, t);
            blen += t;
          }
          batch[blen] = 0;
          in_batch++;
          total++;
          if (in_batch >= nwant) {
            if (batches > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + blen < sizeof out) {
              memcpy(out + olen, batch, blen);
              olen += blen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, batch, t);
              olen += t;
            }
            out[olen] = 0;
            batches++;
            in_batch = 0;
            blen = 0;
            batch[0] = 0;
          }
          if (*p == '\n') p++;
        }
        /* flush partial last batch */
        if (in_batch > 0) {
          if (batches > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + blen < sizeof out) {
            memcpy(out + olen, batch, blen);
            olen += blen;
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            memcpy(out + olen, batch, t);
            olen += t;
          }
          out[olen] = 0;
          batches++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = batches;
      var_set_num(vm, "LAST_N", batches);
      var_set_num(vm, "CHUNK_N", batches);
      var_set_num(vm, "CHUNK_TOTAL", total);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS WINDOW|SLIDE|NGRAM n [bag] [join_sep]
     * — overlapping windows of n consecutive bag fields; join within each
     * window with join_sep (default space); windows → newline fields in LAST.
     * n<=0 or fields < n → empty. LAST_N/WINDOW_N = window count;
     * WINDOW_TOTAL = input fields; WINDOW_SIZE = n.
     * Usability: consecutive pairs/ngrams / rolling peer pairs without
     * EACH+NTH+CAT glue. Complements non-overlapping SYS CHUNK. */
    if (kw(&L->cur,"WINDOW") || kw(&L->cur,"SLIDE") || kw(&L->cur,"SLIDING") ||
        kw(&L->cur,"NGRAM") || kw(&L->cur,"WINS") || kw(&L->cur,"ROLLWIN") ||
        kw(&L->cur,"PAIRWISE") || kw(&L->cur,"OVERLAP") || kw(&L->cur,"SWINDOW")){
      long nwant = 0, total = 0, windows = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], jsep[32];
      enum { WIN_MAX = 256 };
      const char *fstart[WIN_MAX];
      size_t flens[WIN_MAX];
      const char *p, *start;
      size_t olen = 0, jsepn, flen;
      long i, j;
      lex_next(L);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
        nwant = parse_expr(vm, L);
      else
        nwant = 0;
      bag[0] = 0; out[0] = 0;
      snprintf(jsep, sizeof jsep, "%s", " ");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_STR) {
        snprintf(jsep, sizeof jsep, "%s", L->cur.text);
        lex_next(L);
      }
      jsepn = strlen(jsep);
      if (bag[0]) {
        p = bag;
        while (*p && total < WIN_MAX) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          fstart[total] = start;
          flens[total] = flen;
          total++;
          if (*p == '\n') p++;
        }
      }
      if (nwant > 0 && total >= nwant) {
        for (i = 0; i <= total - nwant; i++) {
          if (windows > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          for (j = 0; j < nwant; j++) {
            if (j > 0 && jsepn > 0) {
              if (olen + jsepn < sizeof out) {
                memcpy(out + olen, jsep, jsepn);
                olen += jsepn;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, jsep, t);
                olen += t;
              }
            }
            flen = flens[i + j];
            if (olen + flen < sizeof out) {
              memcpy(out + olen, fstart[i + j], flen);
              olen += flen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, fstart[i + j], t);
              olen += t;
            }
          }
          out[olen] = 0;
          windows++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = windows;
      var_set_num(vm, "LAST_N", windows);
      var_set_num(vm, "WINDOW_N", windows);
      var_set_num(vm, "WINDOW_TOTAL", total);
      var_set_num(vm, "WINDOW_SIZE", nwant > 0 ? nwant : 0);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS STRIDE|EVERY|STEP step [offset] [bag]
     * — keep bag fields where index % step == offset (0-based).
     * offset defaults to 0; optional OFF|OFFSET keyword before offset.
     * step<=0 → empty. LAST_N/STRIDE_N = kept; STRIDE_TOTAL = input fields;
     * STRIDE_STEP/STRIDE_OFF = params.
     * Usability: partition work across workers without EACH+MOD index glue.
     * Complements CHUNK (batch size) and WINDOW (overlap). */
    if (kw(&L->cur,"STRIDE") || kw(&L->cur,"EVERY") || kw(&L->cur,"STEP") ||
        kw(&L->cur,"NTHOF") || kw(&L->cur,"STRIDEBAG") || kw(&L->cur,"TAKESTEP") ||
        kw(&L->cur,"EVERYN") || kw(&L->cur,"MODPICK") || kw(&L->cur,"PARTNIDX")){
      long step = 0, offset = 0, total = 0, kept = 0, idx = 0, off_norm = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t olen = 0, flen;
      lex_next(L);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
        step = parse_expr(vm, L);
      else
        step = 0;
      offset = 0;
      if (kw(&L->cur,"OFF") || kw(&L->cur,"OFFSET") || kw(&L->cur,"FROM") ||
          kw(&L->cur,"START") || kw(&L->cur,"AT")){
        lex_next(L);
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
            L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
          offset = parse_expr(vm, L);
      } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
                 L->cur.kind == TK_MINUS) {
        /* bare numeric offset (not IDENT — that is bag name) */
        offset = parse_expr(vm, L);
      }
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (step > 0) {
        off_norm = offset % step;
        if (off_norm < 0) off_norm += step;
      }
      if (step > 0 && bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          total++;
          if ((idx % step) == off_norm) {
            if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + flen < sizeof out) {
              memcpy(out + olen, start, flen);
              olen += flen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, start, t);
              olen += t;
            }
            out[olen] = 0;
            kept++;
          }
          idx++;
          if (*p == '\n') p++;
        }
      } else if (bag[0]) {
        /* step<=0: count total only, keep none */
        p = bag;
        while (*p) {
          while (*p && *p != '\n') p++;
          total++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "STRIDE_N", kept);
      var_set_num(vm, "STRIDE_TOTAL", total);
      var_set_num(vm, "STRIDE_STEP", step > 0 ? step : 0);
      var_set_num(vm, "STRIDE_OFF", step > 0 ? off_norm : 0);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS FLATTEN|UNCHUNK|EXPAND|SPLITALL [bag] [sep]
     * — split every bag field by sep (default space) and emit all tokens as
     * a flat newline bag → LAST. Empty sep concatenates nothing extra (each
     * non-empty field kept as one token). LAST_N/FLATTEN_N = token count;
     * FLATTEN_LINES = input field count.
     * Usability: reverse of SYS CHUNK join — re-expand batched peer/work
     * lists without EACH+SPLIT+PUSH glue. */
    if (kw(&L->cur,"FLATTEN") || kw(&L->cur,"UNCHUNK") || kw(&L->cur,"EXPAND") ||
        kw(&L->cur,"SPLITALL") || kw(&L->cur,"MAPSPLIT") || kw(&L->cur,"FLATSPLIT") ||
        kw(&L->cur,"UNBATCH") || kw(&L->cur,"FLATM") || kw(&L->cur,"BAGFLATTEN")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], sep[64];
      const char *p, *lstart, *lp, *tstart;
      size_t olen = 0, sepn, lflen, tlen;
      long tokens = 0, lines = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", " ");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        lex_next(L);
      }
      sepn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          lstart = p;
          while (*p && *p != '\n') p++;
          lflen = (size_t)(p - lstart);
          lines++;
          if (sepn == 0) {
            /* empty sep: emit whole field as one token (if non-empty) */
            if (lflen > 0) {
              if (tokens > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + lflen < sizeof out) {
                memcpy(out + olen, lstart, lflen);
                olen += lflen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, lstart, t);
                olen += t;
              }
              out[olen] = 0;
              tokens++;
            }
          } else {
            const char *lend = lstart + lflen;
            lp = lstart;
            for (;;) {
              tstart = lp;
              while (lp < lend &&
                     !(sepn > 0 && (size_t)(lend - lp) >= sepn &&
                       memcmp(lp, sep, sepn) == 0))
                lp++;
              tlen = (size_t)(lp - tstart);
              if (tokens > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + tlen < sizeof out) {
                memcpy(out + olen, tstart, tlen);
                olen += tlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, tstart, t);
                olen += t;
              }
              out[olen] = 0;
              tokens++;
              if (lp >= lend) break;
              /* consume sep; if it was trailing, loop emits empty token */
              lp += sepn;
            }
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = tokens;
      var_set_num(vm, "LAST_N", tokens);
      var_set_num(vm, "FLATTEN_N", tokens);
      var_set_num(vm, "FLATTEN_LINES", lines);
      var_set_num(vm, "UNCHUNK_N", tokens);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS CUTALL|MAPCUT|COLALL bag sep n — peel Nth sep-field from every bag line.
     * CUTALL/MAPCUT/FIELDNALL: 0-based. COLALL/COLUMNALL: 1-based.
     * LAST = bag of peeled fields (empty token if miss). LAST_N = line count.
     * CUTALL_HIT = how many lines had the column. Usability: log column → FREQ
     * without EACH LINE + CUT glue. */
    if (kw(&L->cur,"CUTALL") || kw(&L->cur,"MAPCUT") || kw(&L->cur,"FIELDNALL") ||
        kw(&L->cur,"CUTBAG") || kw(&L->cur,"BAGCUT") ||
        kw(&L->cur,"COLALL") || kw(&L->cur,"COLUMNALL") || kw(&L->cur,"MAPCOLUMN") ||
        kw(&L->cur,"COLBAG")){
      char op[20];
      int one_based = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], sep[64];
      long want = 0, lines = 0, hits = 0;
      const char *lp, *lstart;
      size_t olen = 0, sepn, lflen;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "COLALL") == 0 || strcmp(op, "COLUMNALL") == 0 ||
          strcmp(op, "MAPCOLUMN") == 0 || strcmp(op, "COLBAG") == 0)
        one_based = 1;
      lex_next(L);
      bag[0] = 0; out[0] = 0; sep[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, sep, sizeof sep) != 0) sep[0] = 0;
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
        want = parse_expr(vm, L);
      else
        want = 0;
      if (one_based) {
        if (want < 1) want = 1;
        want = want - 1;
      } else {
        if (want < 0) want = 0;
      }
      sepn = strlen(sep);
      if (bag[0]) {
        lp = bag;
        while (*lp) {
          char line[512], cell[512];
          const char *p, *hit, *start;
          long idx = 0, found = 0;
          size_t flen, take;
          lstart = lp;
          while (*lp && *lp != '\n') lp++;
          lflen = (size_t)(lp - lstart);
          take = lflen;
          if (take >= sizeof line) take = sizeof line - 1;
          memcpy(line, lstart, take);
          line[take] = 0;
          cell[0] = 0;
          p = line;
          if (sepn == 0) {
            if (want == 0) {
              snprintf(cell, sizeof cell, "%s", line);
              found = 1;
            }
          } else {
            while (*p) {
              start = p;
              hit = strstr(p, sep);
              if (hit) {
                flen = (size_t)(hit - p);
                p = hit + sepn;
              } else {
                flen = strlen(p);
                p = p + flen;
              }
              if (idx == want) {
                if (flen >= sizeof cell) flen = sizeof cell - 1;
                memcpy(cell, start, flen);
                cell[flen] = 0;
                found = 1;
                break;
              }
              idx++;
              if (!hit) break;
            }
          }
          if (lines > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          {
            size_t n = strlen(cell);
            if (olen + n < sizeof out) {
              memcpy(out + olen, cell, n);
              olen += n;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, cell, t);
              olen += t;
            }
            out[olen] = 0;
          }
          if (found) hits++;
          lines++;
          if (*lp == '\n') lp++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "CUT", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = lines;
      var_set_num(vm, "LAST_N", lines);
      var_set_num(vm, "CUTALL_N", lines);
      var_set_num(vm, "CUTALL_HIT", hits);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MIDLINES|SLICEBAG|FIELDSLICE|LINESLICE bag start [end]
     * — keep newline fields [start..end] inclusive, 0-based.
     * end omitted → through last field. start/end clamp; start>end → empty.
     * Usability: middle bag windows without DROP+TAKE two-step glue.
     * Note: SYS SLICE is string MID — use SLICEBAG/MIDLINES for bags. */
    if (kw(&L->cur,"MIDLINES") || kw(&L->cur,"SLICEBAG") || kw(&L->cur,"FIELDSLICE") ||
        kw(&L->cur,"LINESLICE") || kw(&L->cur,"BAGSLICE") || kw(&L->cur,"SLICEFIELDS") ||
        kw(&L->cur,"SUBLINES") || kw(&L->cur,"WINDOWLINES")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      long start_i = 0, end_i = -1, idx = 0, kept = 0;
      const char *p, *start;
      size_t olen = 0, flen;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)
        start_i = parse_prim(vm, L);
      else
        start_i = 0;
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)
        end_i = parse_prim(vm, L);
      else
        end_i = -1; /* to end */
      if (start_i < 0) start_i = 0;
      if (end_i >= 0 && end_i < start_i) {
        /* empty window */
      } else if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          if (start == p && *p == 0 && start > bag && start[-1] == '\n')
            break;
          flen = (size_t)(p - start);
          if (idx >= start_i && (end_i < 0 || idx <= end_i)) {
            if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + flen < sizeof out) {
              memcpy(out + olen, start, flen);
              olen += flen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, start, t);
              olen += t;
            }
            out[olen] = 0;
            kept++;
          }
          idx++;
          if (*p == '\n') p++;
          /* early stop if past end */
          if (end_i >= 0 && idx > end_i) break;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "MIDLINES_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SORTN|NSORT|NUMSORT [DESC|R] [str|LAST] — numeric sort of newline fields.
     * Lex SORT orders "10" before "2"; SORTN orders by integer value.
     * Non-numeric / blank fields sort as 0; stable on ties. LAST_N/SORT_N = count.
     * Usability: score bags / sizes after LIST without shell sort -n. */
    if (kw(&L->cur,"SORTN") || kw(&L->cur,"NSORT") || kw(&L->cur,"NUMSORT") ||
        kw(&L->cur,"SORTNUM") || kw(&L->cur,"ISORT") || kw(&L->cur,"SORTINT") ||
        kw(&L->cur,"RSORTN") || kw(&L->cur,"SORTND") || kw(&L->cur,"NSORTR")){
      char src[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      enum { SORTN_MAX = 256, SORTN_FLEN = 192 };
      char fields[SORTN_MAX][SORTN_FLEN];
      long nvals[SORTN_MAX];
      int order[SORTN_MAX];
      int n = 0, i, desc = 0;
      const char *p, *start;
      size_t olen = 0;
      long kept = 0;
      char op0[16];
      snprintf(op0, sizeof op0, "%s", L->cur.text);
      for (char *q = op0; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op0, "RSORTN") == 0 || strcmp(op0, "SORTND") == 0 ||
          strcmp(op0, "NSORTR") == 0)
        desc = 1;
      lex_next(L);
      /* optional DESC|ASC|R|REV flag */
      if (kw(&L->cur,"DESC") || kw(&L->cur,"DESCENDING") || kw(&L->cur,"REVERSE") ||
          kw(&L->cur,"REV") || kw(&L->cur,"R") || kw(&L->cur,"-R") ||
          kw(&L->cur,"-N") || kw(&L->cur,"DOWN")) {
        desc = 1;
        lex_next(L);
      } else if (kw(&L->cur,"ASC") || kw(&L->cur,"ASCENDING") || kw(&L->cur,"UP")) {
        desc = 0;
        lex_next(L);
      }
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      out[0] = 0;
      if (src[0]) {
        p = src;
        while (*p && n < SORTN_MAX) {
          start = p;
          while (*p && *p != '\n') p++;
          if (start == p && *p == 0 && start > src && start[-1] == '\n')
            break;
          {
            size_t flen = (size_t)(p - start);
            char *end = NULL;
            long v = 0;
            if (flen >= SORTN_FLEN) flen = SORTN_FLEN - 1;
            memcpy(fields[n], start, flen);
            fields[n][flen] = 0;
            if (fields[n][0]) {
              v = strtol(fields[n], &end, 10);
              if (end == fields[n]) v = 0; /* non-numeric → 0 */
              else {
                while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                  end++;
                if (end && *end != 0) v = 0; /* trailing junk → 0 */
              }
            }
            nvals[n] = v;
            order[n] = n;
            n++;
          }
          if (*p == '\n') p++;
        }
      }
      if (n > 1) {
        for (i = 1; i < n; i++) {
          int key = order[i], j = i - 1;
          while (j >= 0) {
            long a = nvals[order[j]], b = nvals[key];
            int gt = desc ? (a < b) : (a > b);
            if (!gt && a == b) {
              /* stable: keep original order on tie */
              gt = 0;
            }
            if (!gt) break;
            order[j + 1] = order[j];
            j--;
          }
          order[j + 1] = key;
        }
      }
      for (i = 0; i < n; i++) {
        int idx = order[i];
        if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        {
          size_t flen = strlen(fields[idx]);
          if (olen + flen < sizeof out) {
            memcpy(out + olen, fields[idx], flen);
            olen += flen;
          } else if (olen < sizeof out - 1) {
            size_t take = sizeof out - 1 - olen;
            memcpy(out + olen, fields[idx], take);
            olen += take;
          }
          out[olen] = 0;
        }
        kept++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "SORT_N", kept);
      var_set_num(vm, "SORTN_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SORTLEN|LENSORT|SORTBYLEN [DESC|ASC] [bag] — sort fields by string length.
     * Default ASC (shortest first); DESC/R → longest first. Stable on ties.
     * LAST_N/SORTLEN_N = field count. Cap 256×192 like SORT.
     * Usability: longest-first logs / shortest labels without LENALL+SORTN+rebuild. */
    if (kw(&L->cur,"SORTLEN") || kw(&L->cur,"LENSORT") || kw(&L->cur,"SORTBYLEN") ||
        kw(&L->cur,"LSORT") || kw(&L->cur,"BYLEN") || kw(&L->cur,"SORTBYLENGTH") ||
        kw(&L->cur,"RSORTLEN") || kw(&L->cur,"SORTLEND") || kw(&L->cur,"LENSORTR")){
      char src[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      enum { SL_MAX = 256, SL_FLEN = 192 };
      char fields[SL_MAX][SL_FLEN];
      long lens[SL_MAX];
      int order[SL_MAX];
      int n = 0, i, desc = 0;
      const char *p, *start;
      size_t olen = 0;
      long kept = 0;
      char op0[20];
      snprintf(op0, sizeof op0, "%s", L->cur.text);
      for (char *q = op0; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op0, "RSORTLEN") == 0 || strcmp(op0, "SORTLEND") == 0 ||
          strcmp(op0, "LENSORTR") == 0)
        desc = 1;
      lex_next(L);
      if (kw(&L->cur,"DESC") || kw(&L->cur,"DESCENDING") || kw(&L->cur,"REVERSE") ||
          kw(&L->cur,"REV") || kw(&L->cur,"R") || kw(&L->cur,"-R") ||
          kw(&L->cur,"DOWN") || kw(&L->cur,"LONG") || kw(&L->cur,"LONGEST")) {
        desc = 1;
        lex_next(L);
      } else if (kw(&L->cur,"ASC") || kw(&L->cur,"ASCENDING") || kw(&L->cur,"UP") ||
                 kw(&L->cur,"SHORT") || kw(&L->cur,"SHORTEST")) {
        desc = 0;
        lex_next(L);
      }
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      out[0] = 0;
      if (src[0]) {
        p = src;
        while (*p && n < SL_MAX) {
          start = p;
          while (*p && *p != '\n') p++;
          if (start == p && *p == 0 && start > src && start[-1] == '\n')
            break;
          {
            size_t flen = (size_t)(p - start);
            if (flen >= SL_FLEN) flen = SL_FLEN - 1;
            memcpy(fields[n], start, flen);
            fields[n][flen] = 0;
            lens[n] = (long)flen;
            order[n] = n;
            n++;
          }
          if (*p == '\n') p++;
        }
      }
      if (n > 1) {
        for (i = 1; i < n; i++) {
          int key = order[i], j = i - 1;
          while (j >= 0) {
            long a = lens[order[j]], b = lens[key];
            int gt = desc ? (a < b) : (a > b);
            if (!gt) break;
            order[j + 1] = order[j];
            j--;
          }
          order[j + 1] = key;
        }
      }
      for (i = 0; i < n; i++) {
        int idx = order[i];
        if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        {
          size_t flen = strlen(fields[idx]);
          if (olen + flen < sizeof out) {
            memcpy(out + olen, fields[idx], flen);
            olen += flen;
          } else if (olen < sizeof out - 1) {
            size_t take = sizeof out - 1 - olen;
            memcpy(out + olen, fields[idx], take);
            olen += take;
          }
          out[olen] = 0;
        }
        kept++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "SORT_N", kept);
      var_set_num(vm, "SORTLEN_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS REVL|REVLINES|FLIPLINES [str|LAST] — reverse newline field order → LAST.
     * LAST_N/REVL_N = field count. Cap 256 fields (same as SORT).
     * Usability: newest-first logs / LIFO work bags with POP without shell tac. */
    if (kw(&L->cur,"REVL") || kw(&L->cur,"REVLINES") || kw(&L->cur,"FLIPLINES") ||
        kw(&L->cur,"REVERSELINES") || kw(&L->cur,"LINEREV") || kw(&L->cur,"TAC") ||
        kw(&L->cur,"REVLINE")){
      char src[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      enum { REVL_MAX = 256, REVL_FLEN = 192 };
      char fields[REVL_MAX][REVL_FLEN];
      int n = 0, i;
      const char *p, *start;
      size_t olen = 0;
      lex_next(L);
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      out[0] = 0;
      if (src[0]) {
        p = src;
        while (*p && n < REVL_MAX) {
          start = p;
          while (*p && *p != '\n') p++;
          if (start == p && *p == 0 && start > src && start[-1] == '\n')
            break;
          {
            size_t flen = (size_t)(p - start);
            if (flen >= REVL_FLEN) flen = REVL_FLEN - 1;
            memcpy(fields[n], start, flen);
            fields[n][flen] = 0;
            n++;
          }
          if (*p == '\n') p++;
        }
      }
      for (i = n - 1; i >= 0; i--) {
        size_t flen = strlen(fields[i]);
        if (olen > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
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
      var_set_num(vm, "REVL_N", (long)n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS ROTATE|ROTL k [bag] — left-rotate bag fields by k (mod n) → LAST.
     * SYS ROTR k [bag] — right-rotate. Optional LEFT|RIGHT after ROTATE.
     * k may be negative (opposite dir). LAST_N/ROTATE_N = field count;
     * ROTATE_K = normalized left-shift amount [0..n). Empty → "".
     * Usability: round-robin peer/work queues without EACH+NTH rebuild.
     * Complements REVL (full reverse) and STRIDE (partition). */
    if (kw(&L->cur,"ROTATE") || kw(&L->cur,"ROTL") || kw(&L->cur,"ROTR") ||
        kw(&L->cur,"ROTLEFT") || kw(&L->cur,"ROTRIGHT") || kw(&L->cur,"CYCLE") ||
        kw(&L->cur,"ROLL") || kw(&L->cur,"SHIFTBAG") || kw(&L->cur,"BAGROT")){
      char op[20];
      int right = 0;
      long k = 0, total = 0, knorm = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      enum { ROT_MAX = 256 };
      const char *fstart[ROT_MAX];
      size_t flens[ROT_MAX];
      const char *p, *start;
      size_t olen = 0, flen;
      long i, src;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "ROTR") == 0 || strcmp(op, "ROTRIGHT") == 0)
        right = 1;
      lex_next(L);
      if (kw(&L->cur,"LEFT") || kw(&L->cur,"L") || kw(&L->cur,"CCW")) {
        right = 0;
        lex_next(L);
      } else if (kw(&L->cur,"RIGHT") || kw(&L->cur,"R") || kw(&L->cur,"CW")) {
        right = 1;
        lex_next(L);
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
        k = parse_expr(vm, L);
      else
        k = 1; /* default rotate by 1 */
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (bag[0]) {
        p = bag;
        while (*p && total < ROT_MAX) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          fstart[total] = start;
          flens[total] = flen;
          total++;
          if (*p == '\n') p++;
        }
      }
      if (total > 0) {
        long kk = k;
        if (right) kk = -kk;
        knorm = kk % total;
        if (knorm < 0) knorm += total;
        for (i = 0; i < total; i++) {
          src = (i + knorm) % total;
          if (i > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          flen = flens[src];
          if (olen + flen < sizeof out) {
            memcpy(out + olen, fstart[src], flen);
            olen += flen;
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            memcpy(out + olen, fstart[src], t);
            olen += t;
          }
          out[olen] = 0;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = total;
      var_set_num(vm, "LAST_N", total);
      var_set_num(vm, "ROTATE_N", total);
      var_set_num(vm, "ROTATE_K", knorm);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS JOINLINES|PASTE|MERGE sep [str|LAST] — join newline fields with sep.
     * Inverse of SYS SPLIT. LAST = joined string; LAST_N/JOIN_N = field count.
     * Empty sep concatenates with no delimiter.
     * Usability: LIST/GREP/SORT results → CSV/report line without shell paste. */
    if (kw(&L->cur,"JOINLINES") || kw(&L->cur,"PASTE") || kw(&L->cur,"MERGE") ||
        kw(&L->cur,"JOINSEP") || kw(&L->cur,"IMPLADE") || kw(&L->cur,"LINESJOIN")){
      char sep[64]="", src[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t sepn, olen = 0, flen;
      long nfields = 0;
      lex_next(L);
      if (resolve_str_arg(vm, L, sep, sizeof sep) != 0) sep[0]=0;
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      out[0] = 0;
      sepn = strlen(sep);
      if (src[0]) {
        p = src;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          if (start == p && *p == 0 && start > src && start[-1] == '\n')
            break;
          flen = (size_t)(p - start);
          if (nfields > 0 && sepn > 0) {
            if (olen + sepn < sizeof out) {
              memcpy(out + olen, sep, sepn);
              olen += sepn;
            } else if (olen < sizeof out - 1) {
              size_t take = sizeof out - 1 - olen;
              memcpy(out + olen, sep, take);
              olen += take;
            }
          }
          if (olen + flen < sizeof out) {
            memcpy(out + olen, start, flen);
            olen += flen;
          } else if (olen < sizeof out - 1) {
            size_t take = sizeof out - 1 - olen;
            memcpy(out + olen, start, take);
            olen += take;
          }
          out[olen] = 0;
          nfields++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "JOINED", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = nfields;
      var_set_num(vm, "LAST_N", nfields);
      var_set_num(vm, "JOIN_N", nfields);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* bag-line mutators → lang_ops_sys_bagmut.c (B_core_sys) */
    {
      int bm = cubalc_lang_ops_sys_try_bagmut(vm, L);
      if (bm != 0) return bm;
    }

    /* string eq/transform residual → lang_ops_sys_streq.c (A2) */
    {
      int xr = cubalc_lang_ops_sys_try_streq(vm, L);
      if (xr != 0) return xr;
    }

    {
      char sug[48], ebuf[192], typo[64];
      if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR)
        snprintf(typo, sizeof typo, "%s", L->cur.text);
      else
        typo[0] = 0;
      sys_suggest_op(typo, sug, sizeof sug);
      if (sug[0] && typo[0])
        snprintf(ebuf, sizeof ebuf,
                 "SYS unknown '%s' — did you mean %s? (cubalc forms SYS / HELP)",
                 typo, sug);
      else if (typo[0])
        snprintf(ebuf, sizeof ebuf,
                 "SYS unknown '%s' — try cubalc forms SYS or HELP SYS <name>",
                 typo);
      else
        snprintf(ebuf, sizeof ebuf,
                 "SYS needs op — try cubalc forms SYS or HELP SYS ENV");
      fail(vm, ebuf);
    }
    return -1;
  }

  return 0;
}
