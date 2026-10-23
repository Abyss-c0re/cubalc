/* CubalC lang — lang_ops_sys_streq.c
 * SYS EQS/HAS/TRIM/UPPER/REPLACE/PAD/COUNT string residual (A2 peel).
 * Mesh vote A2_theme_peel_core_sys · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_sys_streq.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int cubalc_lang_ops_sys_try_streq(VM *vm, Lex *L) {
  /* SYS string eq/transform residual. Caller consumed SYS|HOST.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
    /* SYS EQS|STREQ a b — string equality → LAST_N 1/0
     * SYS EQSI|IEQS|STREQI — case-insensitive equality (IF after GREPI).
     * SYS EQS I|ICASE a b — same as EQSI. */
    if (kw(&L->cur,"EQS") || kw(&L->cur,"STREQ") || kw(&L->cur,"SEQ") ||
        kw(&L->cur,"EQSI") || kw(&L->cur,"IEQS") || kw(&L->cur,"STREQI") ||
        kw(&L->cur,"SEQI") || kw(&L->cur,"CIEQ")){
      int icase = (kw(&L->cur,"EQSI") || kw(&L->cur,"IEQS") ||
                   kw(&L->cur,"STREQI") || kw(&L->cur,"SEQI") ||
                   kw(&L->cur,"CIEQ"));
      lex_next(L);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      char a[CUBALC_HOST_STR_MAX]="", b[CUBALC_HOST_STR_MAX]="";
      if (resolve_str_arg(vm, L, a, sizeof a) != 0)
        snprintf(a, sizeof a, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, b, sizeof b) != 0) b[0]=0;
      long eq;
      if (!icase) {
        eq = (strcmp(a, b) == 0) ? 1 : 0;
      } else {
        size_t i;
        eq = 1;
        for (i = 0; ; i++) {
          char ca = a[i], cb = b[i];
          if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
          if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
          if (ca != cb) { eq = 0; break; }
          if (!a[i]) break;
        }
      }
      vm->last_n = eq;
      var_set_num(vm, "LAST_N", eq);
      var_set_num(vm, "OK", 1);
      { FILE *df=fopen("/tmp/eqs_debug.txt","w"); if(df){fprintf(df,"a=[%s] b=[%s] eq=%ld\n",a,b,eq);fclose(df);} }
      bump(vm); return 1;
    }
    /* SYS HAS|CONTAINS hay needle — 1 if needle in hay
     * SYS HASI|ICONTAINS|CONTAINSI — case-insensitive substring.
     * SYS HAS I|ICASE hay needle — same as HASI. */
    if (kw(&L->cur,"HAS") || kw(&L->cur,"CONTAINS") || kw(&L->cur,"INSTR") ||
        kw(&L->cur,"HASI") || kw(&L->cur,"ICONTAINS") || kw(&L->cur,"CONTAINSI") ||
        kw(&L->cur,"IHAS") || kw(&L->cur,"INSTRI")){
      int icase = (kw(&L->cur,"HASI") || kw(&L->cur,"ICONTAINS") ||
                   kw(&L->cur,"CONTAINSI") || kw(&L->cur,"IHAS") ||
                   kw(&L->cur,"INSTRI"));
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
      long hit = 0;
      if (!needle[0]) {
        hit = 1;
      } else if (!icase) {
        hit = (strstr(hay, needle) != NULL) ? 1 : 0;
      } else {
        size_t nl = strlen(needle), fi;
        for (fi = 0; hay[fi] && !hit; fi++) {
          size_t j;
          for (j = 0; j < nl; j++) {
            char ca = hay[fi + j], cb = needle[j];
            if (!ca) break;
            if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
            if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
            if (ca != cb) break;
          }
          if (j == nl) hit = 1;
        }
      }
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS BEFORE|AFTER hay needle — peel at first needle (log/kv without FIND+MID).
     * BEFORE → text left of first match (whole hay if miss).
     * AFTER  → text right of first match (empty if miss).
     * LAST_N = 1 if needle found, 0 if miss; empty needle → found at 0. */
    /* Note: do not alias POST — SYS POST is HTTP POST already. */
    if (kw(&L->cur,"BEFORE") || kw(&L->cur,"LEFT_OF") || kw(&L->cur,"LEFTOF") ||
        kw(&L->cur,"PRE") || kw(&L->cur,"HEADOF") || kw(&L->cur,"SPLITLEFT") ||
        kw(&L->cur,"AFTER") || kw(&L->cur,"RIGHT_OF") || kw(&L->cur,"RIGHTOF") ||
        kw(&L->cur,"TAILOF") || kw(&L->cur,"SPLITRIGHT")){
      char op[16]; snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *p = op; *p; p++)
        if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 'a' + 'A');
      int want_after = (strcmp(op, "AFTER") == 0 || strcmp(op, "RIGHT_OF") == 0 ||
                        strcmp(op, "RIGHTOF") == 0 ||
                        strcmp(op, "TAILOF") == 0 || strcmp(op, "SPLITRIGHT") == 0);
      lex_next(L);
      char hay[CUBALC_HOST_STR_MAX] = "", needle[CUBALC_HOST_STR_MAX] = "";
      if (resolve_str_arg(vm, L, hay, sizeof hay) != 0)
        snprintf(hay, sizeof hay, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0] = 0;
      char out[CUBALC_HOST_STR_MAX];
      long found = 0;
      if (needle[0] == 0) {
        /* empty needle matches at start */
        found = 1;
        if (want_after) snprintf(out, sizeof out, "%s", hay);
        else out[0] = 0;
      } else {
        const char *p = strstr(hay, needle);
        if (!p) {
          found = 0;
          if (want_after) out[0] = 0;
          else snprintf(out, sizeof out, "%s", hay);
        } else {
          found = 1;
          if (want_after) {
            snprintf(out, sizeof out, "%s", p + strlen(needle));
          } else {
            size_t pre = (size_t)(p - hay);
            if (pre >= sizeof out) pre = sizeof out - 1;
            memcpy(out, hay, pre);
            out[pre] = 0;
          }
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS BETWEEN|MIDOF|EXTRACT open close [hay] — peel text between delimiters.
     * Completes BEFORE/AFTER for plate/kv/log/JSON-ish extract without FIND+MID.
     * LAST = interior; LAST_N = 1 if pair found, 0 soft miss (OK stays 1).
     * Empty open → match at start; empty close → rest after open. */
    if (kw(&L->cur,"BETWEEN") || kw(&L->cur,"MIDOF") || kw(&L->cur,"EXTRACT") ||
        kw(&L->cur,"INSIDE") || kw(&L->cur,"STRBETWEEN") || kw(&L->cur,"BETWEENSTR") ||
        kw(&L->cur,"INNER")){
      lex_next(L);
      char open[CUBALC_HOST_STR_MAX] = "", close[CUBALC_HOST_STR_MAX] = "", hay[CUBALC_HOST_STR_MAX] = "";
      if (resolve_str_arg(vm, L, open, sizeof open) != 0) open[0] = 0;
      if (resolve_str_arg(vm, L, close, sizeof close) != 0) close[0] = 0;
      if (resolve_str_arg(vm, L, hay, sizeof hay) != 0)
        snprintf(hay, sizeof hay, "%s", vm->last_str);
      char out[CUBALC_HOST_STR_MAX];
      long found = 0;
      out[0] = 0;
      {
        const char *po, *start, *pc;
        size_t n;
        if (open[0] == 0) {
          po = hay;
          start = hay;
        } else {
          po = strstr(hay, open);
          if (!po) {
            var_set_str(vm, "LAST", "");
            snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
            vm->last_n = 0;
            var_set_num(vm, "LAST_N", 0);
            var_set_num(vm, "OK", 1);
            bump(vm); return 1;
          }
          start = po + strlen(open);
        }
        if (close[0] == 0) {
          pc = start + strlen(start);
          found = 1;
        } else {
          pc = strstr(start, close);
          if (!pc) {
            var_set_str(vm, "LAST", "");
            snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
            vm->last_n = 0;
            var_set_num(vm, "LAST_N", 0);
            var_set_num(vm, "OK", 1);
            bump(vm); return 1;
          }
          found = 1;
        }
        n = (size_t)(pc - start);
        if (n >= sizeof out) n = sizeof out - 1;
        memcpy(out, start, n);
        out[n] = 0;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS REVS|STRREV [str] — reverse string → LAST (not cube REVERSE).
     * Host-wide CUBALC_HOST_STR_MAX — multi-KB after CAT/STREPEAT. */
    if (kw(&L->cur,"REVS") || kw(&L->cur,"STRREV") || kw(&L->cur,"SREV")){
      lex_next(L);
      char s[CUBALC_HOST_STR_MAX]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      size_t n = strlen(s);
      char out[CUBALC_HOST_STR_MAX];
      if (n >= sizeof out) n = sizeof out - 1;
      for (size_t i=0;i<n;i++) out[i] = s[n-1-i];
      out[n] = 0;
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)n;
      var_set_num(vm, "LAST_N", (long)n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS UPPER|UCASE [str] — ASCII upper → LAST (host-wide multi-KB). */
    if (kw(&L->cur,"UPPER") || kw(&L->cur,"UCASE") || kw(&L->cur,"TOUPPER")){
      lex_next(L);
      char s[CUBALC_HOST_STR_MAX]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      for (char *p=s; *p; p++)
        if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 'a' + 'A');
      var_set_str(vm, "LAST", s);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", s);
      vm->last_n = (long)strlen(s);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS LOWER|LCASE [str] — ASCII lower → LAST (host-wide multi-KB). */
    if (kw(&L->cur,"LOWER") || kw(&L->cur,"LCASE") || kw(&L->cur,"TOLOWER")){
      lex_next(L);
      char s[CUBALC_HOST_STR_MAX]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      for (char *p=s; *p; p++)
        if (*p >= 'A' && *p <= 'Z') *p = (char)(*p - 'A' + 'a');
      var_set_str(vm, "LAST", s);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", s);
      vm->last_n = (long)strlen(s);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS TRIM|STRIP|LTRIM|RTRIM [str] — host-wide multi-KB whitespace peel.
     * Usability: clean plate payloads without 512 clip before HAS/EQS. */
    if (kw(&L->cur,"TRIM") || kw(&L->cur,"STRIP") ||
        kw(&L->cur,"LTRIM") || kw(&L->cur,"RTRIM")){
      char op[12]; snprintf(op,sizeof op,"%s",L->cur.text);
      for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
      lex_next(L);
      char s[CUBALC_HOST_STR_MAX]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      char *a = s, *b = s + strlen(s);
      int do_l = (strcmp(op,"TRIM")==0 || strcmp(op,"STRIP")==0 || strcmp(op,"LTRIM")==0);
      int do_r = (strcmp(op,"TRIM")==0 || strcmp(op,"STRIP")==0 || strcmp(op,"RTRIM")==0);
      if (do_l) while (*a==' '||*a=='\t'||*a=='\n'||*a=='\r') a++;
      if (do_r){
        while (b > a && (b[-1]==' '||b[-1]=='\t'||b[-1]=='\n'||b[-1]=='\r')) b--;
      }
      char out[CUBALC_HOST_STR_MAX];
      size_t n = (size_t)(b - a);
      if (n >= sizeof out) n = sizeof out - 1;
      memcpy(out, a, n); out[n] = 0;
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)n;
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS STARTS|ENDS hay needle — prefix/suffix probe → LAST_N 0|1
     * SYS STARTSI|ENDSI — case-insensitive (path/ext/severity without UPPER glue).
     * SYS STARTS I / ENDS ICASE forms. Note: SYS SUFFIX is EXTNAME (path plane). */
    if (kw(&L->cur,"STARTS") || kw(&L->cur,"STARTSWITH") || kw(&L->cur,"HASPREFIX") ||
        kw(&L->cur,"PREFIX") ||
        kw(&L->cur,"STARTSI") || kw(&L->cur,"ISTARTS") || kw(&L->cur,"STARTSWITHI") ||
        kw(&L->cur,"HASPREFIXI") || kw(&L->cur,"IPREFIX") ||
        kw(&L->cur,"ENDS") || kw(&L->cur,"ENDSWITH") || kw(&L->cur,"HASSUFFIX") ||
        kw(&L->cur,"ENDSI") || kw(&L->cur,"IENDS") || kw(&L->cur,"ENDSWITHI") ||
        kw(&L->cur,"HASSUFFIXI") || kw(&L->cur,"ISUFFIX")){
      char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
      for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
      int is_end = (strcmp(op,"ENDS")==0 || strcmp(op,"ENDSWITH")==0 ||
                    strcmp(op,"HASSUFFIX")==0 ||
                    strcmp(op,"ENDSI")==0 || strcmp(op,"IENDS")==0 ||
                    strcmp(op,"ENDSWITHI")==0 || strcmp(op,"HASSUFFIXI")==0 ||
                    strcmp(op,"ISUFFIX")==0);
      int icase = (strcmp(op,"STARTSI")==0 || strcmp(op,"ISTARTS")==0 ||
                   strcmp(op,"STARTSWITHI")==0 || strcmp(op,"HASPREFIXI")==0 ||
                   strcmp(op,"IPREFIX")==0 ||
                   strcmp(op,"ENDSI")==0 || strcmp(op,"IENDS")==0 ||
                   strcmp(op,"ENDSWITHI")==0 || strcmp(op,"HASSUFFIXI")==0 ||
                   strcmp(op,"ISUFFIX")==0);
      lex_next(L);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      char hay[CUBALC_HOST_STR_MAX]="", pref[CUBALC_HOST_STR_MAX]="";
      if (resolve_str_arg(vm, L, hay, sizeof hay) != 0)
        snprintf(hay, sizeof hay, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, pref, sizeof pref) != 0) pref[0]=0;
      long hit = 0;
      size_t hn = strlen(hay), pn = strlen(pref);
      if (pn == 0) hit = 1;
      else if (pn <= hn){
        if (!icase) {
          if (!is_end) hit = (strncmp(hay, pref, pn) == 0) ? 1 : 0;
          else hit = (strcmp(hay + (hn - pn), pref) == 0) ? 1 : 0;
        } else {
          size_t i, base = is_end ? (hn - pn) : 0;
          hit = 1;
          for (i = 0; i < pn; i++) {
            char a = hay[base + i], b = pref[i];
            if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
            if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
            if (a != b) { hit = 0; break; }
          }
        }
      }
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"REPLACE") || kw(&L->cur,"SUBST") || kw(&L->cur,"STRREP") ||
        kw(&L->cur,"REPLACEALL") || kw(&L->cur,"GSUB") || kw(&L->cur,"SUBSTALL") ||
        kw(&L->cur,"STRREPALL") || kw(&L->cur,"REPALL")){
      /* SYS REPLACE hay old new — first occurrence.
       * SYS REPLACE ALL hay old new | SYS REPLACEALL|GSUB — all occurrences.
       * LAST = result; LAST_N = number of replacements (0 if none).
       * Host-wide CUBALC_HOST_STR_MAX (no 512/1024 truncate on multi-KB plates).
       * Usability: plate templates {{DATE}}/{{HOST}} without shell sed. */
      int do_all = (kw(&L->cur,"REPLACEALL") || kw(&L->cur,"GSUB") ||
                    kw(&L->cur,"SUBSTALL") || kw(&L->cur,"STRREPALL") ||
                    kw(&L->cur,"REPALL"));
      lex_next(L);
      if (!do_all && (kw(&L->cur,"ALL") || kw(&L->cur,"GLOBAL") ||
                      kw(&L->cur,"EVERY") || kw(&L->cur,"G"))){
        do_all = 1;
        lex_next(L);
      }
      char hay[CUBALC_HOST_STR_MAX]="", olds[CUBALC_HOST_STR_MAX]="", news[CUBALC_HOST_STR_MAX]="";
      if (resolve_str_arg(vm, L, hay, sizeof hay) != 0)
        snprintf(hay, sizeof hay, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, olds, sizeof olds) != 0) olds[0]=0;
      if (resolve_str_arg(vm, L, news, sizeof news) != 0) news[0]=0;
      char out[CUBALC_HOST_STR_MAX];
      long did = 0;
      size_t oldn = strlen(olds), newn = strlen(news);
      if (olds[0] == 0){
        snprintf(out, sizeof out, "%s", hay);
      } else {
        const char *src = hay;
        size_t o = 0;
        out[0] = 0;
        for (;;) {
          const char *p = strstr(src, olds);
          if (!p) {
            size_t rest = strlen(src);
            if (o + rest >= sizeof out) rest = sizeof out - 1 - o;
            memcpy(out + o, src, rest);
            o += rest;
            out[o] = 0;
            break;
          }
          {
            size_t pre = (size_t)(p - src);
            if (o + pre >= sizeof out) pre = sizeof out - 1 - o;
            memcpy(out + o, src, pre); o += pre;
            if (o + newn < sizeof out) {
              memcpy(out + o, news, newn); o += newn;
            } else if (o < sizeof out - 1) {
              size_t take = sizeof out - 1 - o;
              memcpy(out + o, news, take); o += take;
            }
            out[o] = 0;
            did++;
            src = p + oldn;
            /* empty old would infinite-loop; already guarded by olds[0] */
            if (!do_all) {
              size_t rest = strlen(src);
              if (o + rest >= sizeof out) rest = sizeof out - 1 - o;
              memcpy(out + o, src, rest);
              o += rest;
              out[o] = 0;
              break;
            }
            if (o >= sizeof out - 1) break;
          }
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = did;
      var_set_num(vm, "LAST_N", did);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* digit-3 string pad/repeat: LPAD RPAD STREPEAT (SYS REPEAT alias)
     * Usability: buffers use CUBALC_HOST_STR_MAX so agent plate templates
     * (long STREPEAT blobs) are not truncated at 512 bytes. */
    if (kw(&L->cur,"LPAD") || kw(&L->cur,"RPAD") || kw(&L->cur,"PADLEFT") ||
        kw(&L->cur,"PADRIGHT") || kw(&L->cur,"STRPAD") ||
        kw(&L->cur,"STREPEAT") || kw(&L->cur,"STRREPEAT") ||
        kw(&L->cur,"REPEATSTR") || kw(&L->cur,"SREPEAT")){
      char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
      for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
      int is_rep = (strcmp(op,"STREPEAT")==0 || strcmp(op,"STRREPEAT")==0 ||
                    strcmp(op,"REPEATSTR")==0 || strcmp(op,"SREPEAT")==0);
      int is_left = (strcmp(op,"LPAD")==0 || strcmp(op,"PADLEFT")==0 ||
                     strcmp(op,"STRPAD")==0);
      lex_next(L);
      char s[CUBALC_HOST_STR_MAX]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      if (is_rep){
        long times = 0;
        long max_times = (long)(CUBALC_HOST_STR_MAX - 1);
        if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
            L->cur.kind==TK_IDENT)
          times = parse_expr(vm, L);
        if (times < 0) times = 0;
        if (times > max_times) times = max_times;
        char out[CUBALC_HOST_STR_MAX]; size_t o = 0;
        size_t sn = strlen(s);
        if (sn == 0){ out[0]=0; }
        else {
          for (long t=0; t<times && o+1 < sizeof out; t++){
            size_t take = sn;
            if (o + take >= sizeof out) take = sizeof out - 1 - o;
            if (take == 0) break;
            memcpy(out + o, s, take); o += take;
          }
          out[o] = 0;
        }
        var_set_str(vm, "LAST", out);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
        vm->last_n = (long)o;
        var_set_num(vm, "LAST_N", vm->last_n);
        var_set_num(vm, "STREPEAT_N", vm->last_n);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      /* LPAD/RPAD str width [padchar] */
      long width = 0;
      long max_w = (long)(CUBALC_HOST_STR_MAX - 1);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          L->cur.kind==TK_IDENT)
        width = parse_expr(vm, L);
      if (width < 0) width = 0;
      if (width > max_w) width = max_w;
      char padc = ' ';
      char pad[16]; pad[0]=0;
      if (resolve_str_arg(vm, L, pad, sizeof pad) == 0 && pad[0])
        padc = pad[0];
      size_t sn = strlen(s);
      char out[CUBALC_HOST_STR_MAX];
      if ((long)sn >= width){
        size_t take = (size_t)width;
        if (take >= sizeof out) take = sizeof out - 1;
        memcpy(out, s, take); out[take] = 0;
      } else {
        size_t need = (size_t)width - sn;
        if (is_left){
          size_t o = 0;
          for (size_t i=0; i<need && o+1 < sizeof out; i++) out[o++] = padc;
          size_t take = sn;
          if (o + take >= sizeof out) take = sizeof out - 1 - o;
          memcpy(out + o, s, take); o += take; out[o] = 0;
        } else {
          size_t o = 0;
          size_t take = sn;
          if (take >= sizeof out) take = sizeof out - 1;
          memcpy(out, s, take); o = take;
          for (size_t i=0; i<need && o+1 < sizeof out; i++) out[o++] = padc;
          out[o] = 0;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)strlen(out);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS PADALL|MAPAD|RPADALL bag width [padchar] — pad every field to width.
     * SYS LPADALL|MAPLPAD — left-pad; PADALL/RPADALL — right-pad (default).
     * Fields longer than width truncated to width (like LPAD/RPAD). Default
     * pad char space. LAST_N/PADALL_N = field count; PADALL_W = width.
     * Usability: column-align bags after LENALL+MAX without EACH+LPAD glue. */
    if (kw(&L->cur,"PADALL") || kw(&L->cur,"MAPAD") || kw(&L->cur,"MAPPAD") ||
        kw(&L->cur,"RPADALL") || kw(&L->cur,"MAPRPAD") || kw(&L->cur,"PADRIGHTALL") ||
        kw(&L->cur,"LPADALL") || kw(&L->cur,"MAPLPAD") || kw(&L->cur,"PADLEFTALL") ||
        kw(&L->cur,"PADBAG") || kw(&L->cur,"ALIGNALL")){
      char op[20];
      int is_left = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], pad[16];
      char field[512], padded[512];
      const char *p, *start;
      size_t olen = 0, flen, sn;
      long width = 0, kept = 0;
      char padc = ' ';
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "LPADALL") == 0 || strcmp(op, "MAPLPAD") == 0 ||
          strcmp(op, "PADLEFTALL") == 0)
        is_left = 1;
      lex_next(L);
      if (kw(&L->cur,"LEFT") || kw(&L->cur,"L") || kw(&L->cur,"LPAD")) {
        is_left = 1;
        lex_next(L);
      } else if (kw(&L->cur,"RIGHT") || kw(&L->cur,"R") || kw(&L->cur,"RPAD")) {
        is_left = 0;
        lex_next(L);
      }
      bag[0] = 0; out[0] = 0; pad[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
        width = parse_expr(vm, L);
      if (width < 0) width = 0;
      if (width > 511) width = 511;
      if (resolve_str_arg(vm, L, pad, sizeof pad) == 0 && pad[0])
        padc = pad[0];
      if (bag[0]) {
        p = bag;
        while (*p) {
          size_t o, need, take, i;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen >= sizeof field) flen = sizeof field - 1;
          memcpy(field, start, flen);
          field[flen] = 0;
          sn = flen;
          if ((long)sn >= width) {
            take = (size_t)width;
            if (take >= sizeof padded) take = sizeof padded - 1;
            memcpy(padded, field, take);
            padded[take] = 0;
            sn = take;
          } else {
            need = (size_t)width - sn;
            if (is_left) {
              o = 0;
              for (i = 0; i < need && o + 1 < sizeof padded; i++)
                padded[o++] = padc;
              take = sn;
              if (o + take >= sizeof padded) take = sizeof padded - 1 - o;
              memcpy(padded + o, field, take);
              o += take;
              padded[o] = 0;
              sn = o;
            } else {
              o = 0;
              take = sn;
              if (take >= sizeof padded) take = sizeof padded - 1;
              memcpy(padded, field, take);
              o = take;
              for (i = 0; i < need && o + 1 < sizeof padded; i++)
                padded[o++] = padc;
              padded[o] = 0;
              sn = o;
            }
          }
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + sn < sizeof out) {
            memcpy(out + olen, padded, sn);
            olen += sn;
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            memcpy(out + olen, padded, t);
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
      var_set_num(vm, "PADALL_N", kept);
      var_set_num(vm, "PADALL_W", width);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS TRUNCALL|CLIPALL|TRUNCATEALL [RIGHT] bag width — truncate every field
     * to at most width chars (no pad fill). Default keep left; RIGHT/TAIL keep right.
     * width<=0 → empty fields. LAST_N/TRUNCALL_N = field count; TRUNCALL_W = width;
     * TRUNCALL_HIT = fields that were shortened.
     * Usability: clip log/payload bags without EACH+LEFT/MID glue; dual of PADALL. */
    if (kw(&L->cur,"TRUNCALL") || kw(&L->cur,"CLIPALL") || kw(&L->cur,"TRUNCATEALL") ||
        kw(&L->cur,"MAPTRUNC") || kw(&L->cur,"MAPCLIP") || kw(&L->cur,"SHORTENALL") ||
        kw(&L->cur,"TRIMLEN") || kw(&L->cur,"LIMITALL") || kw(&L->cur,"MAXLENALL")){
      int keep_right = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      char field[512], clipped[512];
      const char *p, *start;
      size_t olen = 0, flen, sn, take;
      long width = 0, kept = 0, hit = 0;
      lex_next(L);
      if (kw(&L->cur,"RIGHT") || kw(&L->cur,"R") || kw(&L->cur,"TAIL") ||
          kw(&L->cur,"END") || kw(&L->cur,"KEEPRIGHT")) {
        keep_right = 1;
        lex_next(L);
      } else if (kw(&L->cur,"LEFT") || kw(&L->cur,"L") || kw(&L->cur,"HEAD") ||
                 kw(&L->cur,"START") || kw(&L->cur,"KEEPLEFT")) {
        keep_right = 0;
        lex_next(L);
      }
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
        width = parse_expr(vm, L);
      if (width < 0) width = 0;
      if (width > 511) width = 511;
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen >= sizeof field) flen = sizeof field - 1;
          memcpy(field, start, flen);
          field[flen] = 0;
          if ((long)flen > width) {
            hit++;
            take = (size_t)width;
            if (take >= sizeof clipped) take = sizeof clipped - 1;
            if (keep_right && flen > take) {
              memcpy(clipped, field + (flen - take), take);
            } else {
              memcpy(clipped, field, take);
            }
            clipped[take] = 0;
            sn = take;
          } else {
            sn = flen;
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
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "TRUNCALL_N", kept);
      var_set_num(vm, "CLIPALL_N", kept);
      var_set_num(vm, "TRUNCALL_W", width);
      var_set_num(vm, "TRUNCALL_HIT", hit);
      var_set_num(vm, "CLIPALL_HIT", hit);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* digit-3 string duals: LEFT/RIGHT slice + COUNT occurrences */
    if (kw(&L->cur,"LEFT") || kw(&L->cur,"STRLEFT") || kw(&L->cur,"TAKELEFT") ||
        kw(&L->cur,"PREFIXN") ||
        kw(&L->cur,"RIGHT") || kw(&L->cur,"STRRIGHT") || kw(&L->cur,"TAKERIGHT") ||
        kw(&L->cur,"SUFFIXN")){
      char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
      for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
      int is_right = (strcmp(op,"RIGHT")==0 || strcmp(op,"STRRIGHT")==0 ||
                      strcmp(op,"TAKERIGHT")==0 || strcmp(op,"SUFFIXN")==0);
      lex_next(L);
      char s[512]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      long n = 0;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          L->cur.kind==TK_IDENT)
        n = parse_expr(vm, L);
      if (n < 0) n = 0;
      size_t slen = strlen(s);
      char out[512];
      if (!is_right){
        size_t take = (size_t)n;
        if (take > slen) take = slen;
        if (take >= sizeof out) take = sizeof out - 1;
        memcpy(out, s, take); out[take] = 0;
      } else {
        size_t take = (size_t)n;
        if (take > slen) take = slen;
        if (take >= sizeof out) take = sizeof out - 1;
        size_t start = slen - take;
        memcpy(out, s + start, take); out[take] = 0;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)strlen(out);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"COUNT") || kw(&L->cur,"STRCOUNT") || kw(&L->cur,"COUNTSTR") ||
        kw(&L->cur,"OCCURS") || kw(&L->cur,"COUNTOCC")){
      lex_next(L);
      char hay[512]="", needle[256]="";
      if (resolve_str_arg(vm, L, hay, sizeof hay) != 0)
        snprintf(hay, sizeof hay, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0]=0;
      long cnt = 0;
      if (needle[0] == 0){
        cnt = (long)strlen(hay);
      } else {
        size_t nn = strlen(needle);
        const char *p = hay;
        while ((p = strstr(p, needle)) != NULL){
          cnt++;
          p += nn;
        }
      }
      vm->last_n = cnt;
      var_set_num(vm, "LAST_N", cnt);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }

  return 0;
}
