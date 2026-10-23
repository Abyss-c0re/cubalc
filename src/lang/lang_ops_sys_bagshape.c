/* CubalC lang — lang_ops_sys_bagshape.c
 * SYS bag shape: TAKE/SPLIT/SORT/UNION/ZIP…
 * Peel from core_sys residual (B_core_sys). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_sys_bagshape.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

int cubalc_lang_ops_sys_try_bagshape(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. SYS already consumed. */
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
  return 0;
}
