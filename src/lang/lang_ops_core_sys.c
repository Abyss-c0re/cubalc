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
#include "lang/cubalc_lang_ops_sys_bagshape.h"
#include "lang/cubalc_lang_ops_sys_bagmap.h"
#include "lang/cubalc_lang_ops_sys_bagwin.h"
#include "lang/cubalc_lang_ops_sys_baglen.h"
#include "lang/cubalc_lang_ops_sys_runtime.h"
#include "lang/cubalc_lang_ops_sys_numagg.h"
#include "lang/cubalc_lang_ops_sys_hostmeta.h"
#include "lang/cubalc_lang_ops_sys_kv.h"
#include "lang/cubalc_lang_ops_sys_kvmath.h"
#include "lang/cubalc_lang_ops_sys_kvfilter.h"
#include "lang/cubalc_lang_ops_sys_grep.h"
#include "lang/cubalc_lang_ops_sys_identity.h"
#include "lang/cubalc_lang_ops_sys_codec.h"
#include "lang/cubalc_lang_ops_sys_textio.h"
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

    /* runtime TIME/SLEEP/APPEND → lang_ops_sys_runtime.c (B_core_sys) */
    {
      int rt = cubalc_lang_ops_sys_try_runtime(vm, L);
      if (rt != 0) return rt;
    }

    /* bag field metrics → lang_ops_sys_baglen.c (B_core_sys) */
    {
      int bl = cubalc_lang_ops_sys_try_baglen(vm, L);
      if (bl != 0) return bl;
    }

    /* host-meta theme → lang_ops_sys_hostmeta.c (mesh vote A2) */
    {
      int hm = cubalc_lang_ops_sys_try_hostmeta(vm, L);
      if (hm != 0) return hm;
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

    /* text I/O HEX..NTH → lang_ops_sys_textio.c (mesh vote A2) */
    {
      int tio = cubalc_lang_ops_sys_try_textio(vm, L);
      if (tio != 0) return tio;
    }

    /* bag-GREP theme → lang_ops_sys_grep.c (mesh vote A2) */
    {
      int gr = cubalc_lang_ops_sys_try_grep(vm, L);
      if (gr != 0) return gr;
    }

    /* bag shape TAKE..ZIP → lang_ops_sys_bagshape.c (B_core_sys) */
    {
      int bs = cubalc_lang_ops_sys_try_bagshape(vm, L);
      if (bs != 0) return bs;
    }

    /* bag map PREFIX..FIRSTMATCH → lang_ops_sys_bagmap.c (mesh vote A2) */
    {
      int bmap = cubalc_lang_ops_sys_try_bagmap(vm, L);
      if (bmap != 0) return bmap;
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
    /* bag window/reorder → lang_ops_sys_bagwin.c (B_core_sys) */
    {
      int bw = cubalc_lang_ops_sys_try_bagwin(vm, L);
      if (bw != 0) return bw;
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
