/* CubalC lang — lang_ops_flow_fstr_slice.c
 * FLOW fstr slice: LEFTF · BEFOREF · (prefix/left peel)
 * Peel from fstr_a (B_fstr_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_flow_fstr_slice.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_fstr_slice(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* LEFTF|RIGHTF|SLICEF|SUBSTRF|TRUNCF obj field n [count]
   * TRYLEFTF|LEFTF SOFT â soft miss OK=0.
   * In-place string slice on a field (promotes numâstr).
   * LEFTF f n â keep first n; RIGHTF f n â keep last n;
   * SLICEF f start count â window; TRUNCF f max â keep first max (clip).
   * LAST = result; LAST_N = length. Note: MIDF/CLIPF stay fleet median/clamp.
   * Usability: peel/clip note after FINDF without GETF+MID/LEFT+SETF
   * (METHOD/THIS; pairs REPLACEF/TRIMF). */
  if (kw(&L->cur, "LEFTF") || kw(&L->cur, "KEEPLEFTF") ||
      kw(&L->cur, "TAKELEFTF") || kw(&L->cur, "FIELDLEFT") ||
      kw(&L->cur, "RIGHTF") || kw(&L->cur, "KEEPRIGHTF") ||
      kw(&L->cur, "TAKERIGHTF") || kw(&L->cur, "FIELDRIGHT") ||
      kw(&L->cur, "SLICEF") || kw(&L->cur, "SUBSTRF") ||
      kw(&L->cur, "MIDSTRF") || kw(&L->cur, "FIELDMID") ||
      kw(&L->cur, "SLICEFIELD") || kw(&L->cur, "SUBSTRFIELD") ||
      kw(&L->cur, "TRUNCF") || kw(&L->cur, "TRUNCFIELD") ||
      kw(&L->cur, "STRCLIPF") || kw(&L->cur, "CLIPSTRF") ||
      kw(&L->cur, "MAXLENF") ||
      kw(&L->cur, "TRYLEFTF") || kw(&L->cur, "LEFTFSOFT") ||
      kw(&L->cur, "SOFTLEFTF") || kw(&L->cur, "TRYRIGHTF") ||
      kw(&L->cur, "TRYSLICEF") || kw(&L->cur, "TRYTRUNCF")) {
    char oname[48], fname[48], op[24], hay[256], out[256];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, mode = 0; /* 0=left, 1=right, 2=slice, 3=trunc */
    long n1 = 0, n2 = -1;
    size_t hn, start, count, i;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "RIGHTF") == 0 || strcmp(op, "KEEPRIGHTF") == 0 ||
        strcmp(op, "TAKERIGHTF") == 0 || strcmp(op, "FIELDRIGHT") == 0 ||
        strcmp(op, "TRYRIGHTF") == 0)
      mode = 1;
    else if (strcmp(op, "SLICEF") == 0 || strcmp(op, "SUBSTRF") == 0 ||
             strcmp(op, "MIDSTRF") == 0 || strcmp(op, "FIELDMID") == 0 ||
             strcmp(op, "SLICEFIELD") == 0 || strcmp(op, "SUBSTRFIELD") == 0 ||
             strcmp(op, "TRYSLICEF") == 0)
      mode = 2;
    else if (strcmp(op, "TRUNCF") == 0 || strcmp(op, "TRUNCFIELD") == 0 ||
             strcmp(op, "STRCLIPF") == 0 || strcmp(op, "CLIPSTRF") == 0 ||
             strcmp(op, "MAXLENF") == 0 || strcmp(op, "TRYTRUNCF") == 0)
      mode = 3;
    else
      mode = 0; /* LEFTF */
    if (strcmp(op, "TRYLEFTF") == 0 || strcmp(op, "LEFTFSOFT") == 0 ||
        strcmp(op, "SOFTLEFTF") == 0 || strcmp(op, "TRYRIGHTF") == 0 ||
        strcmp(op, "TRYSLICEF") == 0 || strcmp(op, "TRYTRUNCF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "LEFTF object field n"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(fname, sizeof fname, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      vv = var_get(vm, id, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(fname, sizeof fname, "%s", vv->sval);
      else
        snprintf(fname, sizeof fname, "%s", id);
    } else {
      fail(vm, "LEFTF field"); return -1;
    }
    if (kw(&L->cur, "TO") || kw(&L->cur, "OF") || kw(&L->cur, "LEN") ||
        kw(&L->cur, "WIDTH") || kw(&L->cur, "N") || kw(&L->cur, "BY"))
      lex_next(L);
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
        L->cur.kind == TK_LPAREN ||
        (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))) {
      n1 = parse_expr(vm, L);
    } else {
      fail(vm, "LEFTF object field n"); return -1;
    }
    if (mode == 2) {
      if (kw(&L->cur, "FOR") || kw(&L->cur, "COUNT") || kw(&L->cur, "LEN") ||
          kw(&L->cur, "WIDTH") || kw(&L->cur, "TO"))
        lex_next(L);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN ||
          (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
           !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
           !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
           !kw(&L->cur, "END") && !kw(&L->cur, "GETF") &&
           !kw(&L->cur, "SETF"))) {
        n2 = parse_expr(vm, L);
      } else {
        n2 = -1; /* to end */
      }
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "LEFTF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "LEFTF_N", 0);
      var_set_num(vm, "RIGHTF_N", 0);
      var_set_num(vm, "SLICEF_N", 0);
      var_set_num(vm, "TRUNCF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "LEFTF: unknown object");
      var_set_str(vm, "ERR", "LEFTF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "LEFTF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "LEFTF_N", 0);
      var_set_num(vm, "RIGHTF_N", 0);
      var_set_num(vm, "SLICEF_N", 0);
      var_set_num(vm, "TRUNCF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "LEFTF: unknown field");
      var_set_str(vm, "ERR", "LEFTF: unknown field");
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi])
      snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
    else
      snprintf(hay, sizeof hay, "%ld", ob->fnum[fi]);
    hn = strlen(hay);
    out[0] = 0;
    if (mode == 0 || mode == 3) {
      /* left / trunc â first n1 chars */
      if (n1 < 0) n1 = 0;
      count = (size_t)n1;
      if (count > hn) count = hn;
      if (count >= sizeof out) count = sizeof out - 1;
      for (i = 0; i < count; i++) out[i] = hay[i];
      out[count] = 0;
    } else if (mode == 1) {
      /* right â last n1 */
      if (n1 < 0) n1 = 0;
      count = (size_t)n1;
      if (count > hn) count = hn;
      start = hn - count;
      if (count >= sizeof out) count = sizeof out - 1;
      for (i = 0; i < count; i++) out[i] = hay[start + i];
      out[count] = 0;
    } else {
      /* slice start n1, count n2 (-1 = to end) */
      if (n1 < 0) n1 = 0;
      start = (size_t)n1;
      if (start > hn) start = hn;
      if (n2 < 0)
        count = hn - start;
      else
        count = (size_t)n2;
      if (start + count > hn) count = hn - start;
      if (count >= sizeof out) count = sizeof out - 1;
      for (i = 0; i < count; i++) out[i] = hay[start + i];
      out[count] = 0;
    }
    snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", out);
    ob->fis_str[fi] = 1;
    {
      size_t ol = strlen(out);
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)ol;
      var_set_num(vm, "LAST_N", (long)ol);
    }
    var_set_num(vm, "LEFTF_N", (mode == 0) ? 1 : 0);
    var_set_num(vm, "RIGHTF_N", (mode == 1) ? 1 : 0);
    var_set_num(vm, "SLICEF_N", (mode == 2) ? 1 : 0);
    var_set_num(vm, "TRUNCF_N", (mode == 3) ? 1 : 0);
    var_set_num(vm, "SUBSTRF_N", (mode == 2) ? 1 : 0);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* BEFOREF|AFTERF|BETWEENF obj field needle [close]
   * TRYBEFOREF|BEFOREF SOFT â soft miss OK=0.
   * In-place delimiter peel (promotes numâstr).
   * BEFOREF: keep left of first needle (whole if miss).
   * AFTERF: keep right of first needle (empty if miss).
   * BETWEENF open close: keep interior (empty if pair miss).
   * LAST = result; LAST_N = 1 if delimiter found, 0 soft miss peel.
   * Usability: kv/log peel without GETF+SYS BEFORE/AFTER/BETWEEN+SETF
   * (METHOD/THIS; pairs FINDF/LEFTF count slices). */
  if (kw(&L->cur, "BEFOREF") || kw(&L->cur, "LEFTOFF") ||
      kw(&L->cur, "HEADOFF") || kw(&L->cur, "SPLITLEFTF") ||
      kw(&L->cur, "FIELDBEFORE") ||
      kw(&L->cur, "AFTERF") || kw(&L->cur, "RIGHTOFF") ||
      kw(&L->cur, "TAILOFF") || kw(&L->cur, "SPLITRIGHTF") ||
      kw(&L->cur, "FIELDAFTER") ||
      kw(&L->cur, "BETWEENF") || kw(&L->cur, "MIDOFF") ||
      kw(&L->cur, "EXTRACTF") || kw(&L->cur, "INNERF") ||
      kw(&L->cur, "FIELDBETWEEN") ||
      kw(&L->cur, "TRYBEFOREF") || kw(&L->cur, "BEFOREFSOFT") ||
      kw(&L->cur, "SOFTBEFOREF") || kw(&L->cur, "TRYAFTERF") ||
      kw(&L->cur, "TRYBETWEENF")) {
    char oname[48], fname[48], op[24], hay[256], open[256], close[256], out[256];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, mode = 0; /* 0=before, 1=after, 2=between */
    long found = 0;
    const char *po, *start, *pc;
    size_t n;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "AFTERF") == 0 || strcmp(op, "RIGHTOFF") == 0 ||
        strcmp(op, "TAILOFF") == 0 || strcmp(op, "SPLITRIGHTF") == 0 ||
        strcmp(op, "FIELDAFTER") == 0 || strcmp(op, "TRYAFTERF") == 0)
      mode = 1;
    else if (strcmp(op, "BETWEENF") == 0 || strcmp(op, "MIDOFF") == 0 ||
             strcmp(op, "EXTRACTF") == 0 || strcmp(op, "INNERF") == 0 ||
             strcmp(op, "FIELDBETWEEN") == 0 || strcmp(op, "TRYBETWEENF") == 0)
      mode = 2;
    else
      mode = 0; /* BEFORE */
    if (strcmp(op, "TRYBEFOREF") == 0 || strcmp(op, "BEFOREFSOFT") == 0 ||
        strcmp(op, "SOFTBEFOREF") == 0 || strcmp(op, "TRYAFTERF") == 0 ||
        strcmp(op, "TRYBETWEENF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "BEFOREF object field needle"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(fname, sizeof fname, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      vv = var_get(vm, id, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(fname, sizeof fname, "%s", vv->sval);
      else
        snprintf(fname, sizeof fname, "%s", id);
    } else {
      fail(vm, "BEFOREF field"); return -1;
    }
    open[0] = 0;
    close[0] = 0;
    if (resolve_str_arg(vm, L, open, sizeof open) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        long v = parse_expr(vm, L);
        snprintf(open, sizeof open, "%ld", v);
      } else {
        fail(vm, "BEFOREF object field needle"); return -1;
      }
    }
    if (mode == 2) {
      if (kw(&L->cur, "AND") || kw(&L->cur, "TO") || kw(&L->cur, "CLOSE") ||
          kw(&L->cur, "WITH"))
        lex_next(L);
      if (resolve_str_arg(vm, L, close, sizeof close) != 0) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
            L->cur.kind == TK_LPAREN) {
          long v = parse_expr(vm, L);
          snprintf(close, sizeof close, "%ld", v);
        } else {
          fail(vm, "BETWEENF object field open close"); return -1;
        }
      }
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "BEFOREF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "BEFOREF_N", 0);
      var_set_num(vm, "AFTERF_N", 0);
      var_set_num(vm, "BETWEENF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "BEFOREF: unknown object");
      var_set_str(vm, "ERR", "BEFOREF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "BEFOREF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "BEFOREF_N", 0);
      var_set_num(vm, "AFTERF_N", 0);
      var_set_num(vm, "BETWEENF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "BEFOREF: unknown field");
      var_set_str(vm, "ERR", "BEFOREF: unknown field");
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi])
      snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
    else
      snprintf(hay, sizeof hay, "%ld", ob->fnum[fi]);
    out[0] = 0;
    found = 0;
    if (mode == 2) {
      /* BETWEENF */
      if (open[0] == 0) {
        po = hay;
        start = hay;
      } else {
        po = strstr(hay, open);
        if (!po) {
          found = 0;
          out[0] = 0;
          goto between_done;
        }
        start = po + strlen(open);
      }
      if (close[0] == 0) {
        pc = start + strlen(start);
        found = 1;
      } else {
        pc = strstr(start, close);
        if (!pc) {
          found = 0;
          out[0] = 0;
          goto between_done;
        }
        found = 1;
      }
      n = (size_t)(pc - start);
      if (n >= sizeof out) n = sizeof out - 1;
      memcpy(out, start, n);
      out[n] = 0;
    between_done: ;
    } else if (open[0] == 0) {
      found = 1;
      if (mode == 1)
        snprintf(out, sizeof out, "%s", hay);
      else
        out[0] = 0;
    } else {
      po = strstr(hay, open);
      if (!po) {
        found = 0;
        if (mode == 1)
          out[0] = 0;
        else
          snprintf(out, sizeof out, "%s", hay);
      } else {
        found = 1;
        if (mode == 1) {
          snprintf(out, sizeof out, "%s", po + strlen(open));
        } else {
          n = (size_t)(po - hay);
          if (n >= sizeof out) n = sizeof out - 1;
          memcpy(out, hay, n);
          out[n] = 0;
        }
      }
    }
    snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", out);
    ob->fis_str[fi] = 1;
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = found;
    var_set_num(vm, "LAST_N", found);
    var_set_num(vm, "BEFOREF_N", (mode == 0) ? found : 0);
    var_set_num(vm, "AFTERF_N", (mode == 1) ? found : 0);
    var_set_num(vm, "BETWEENF_N", (mode == 2) ? found : 0);
    var_set_num(vm, "BEFOREF_HIT", found);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }
  return 0;
}
