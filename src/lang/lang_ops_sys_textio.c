/* CubalC lang — lang_ops_sys_textio.c
 * SYS text I/O residual: HEX · ORD/CHR · MID · CAT · FIND · NTH (A2 peel).
 * Mesh vote B_core_sys · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_sys_textio.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

int cubalc_lang_ops_sys_try_textio(VM *vm, Lex *L) {
  /* SYS text I/O residual. Caller consumed SYS|HOST.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
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
  return 0;
}
