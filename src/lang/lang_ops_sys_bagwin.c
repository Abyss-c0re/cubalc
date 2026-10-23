/* CubalC lang — lang_ops_sys_bagwin.c
 * SYS bag window/reorder: CHUNK/WINDOW/STRIDE/FLATTEN/SORTN/ROTATE/JOINLINES…
 * Peel from core_sys residual (B_core_sys). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_sys_bagwin.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

int cubalc_lang_ops_sys_try_bagwin(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. SYS already consumed. */
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
  return 0;
}
