/* CubalC lang — lang_ops_flow_fstr_cutf.c
 * FLOW fstr column peel: CUTF · NTHFIELDF · COLUMNF
 * Peel from fstr_a (B_fstr_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_flow_fstr_cutf.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_fstr_cutf(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* CUTF|NTHFIELDF|FIELDNF obj field sep index â 0-based column peel into field.
   * COLUMNF|COLF|COLNF obj field sep index â 1-based column peel into field.
   * TRYCUTF|CUTF SOFT â soft miss OK=0 sticky LAST_ERR.
   * On hit: field becomes that column (mutate). On miss: field unchanged, HIT=0.
   * LAST = column (or ""); LAST_N/CUTF_HIT = 0|1; CUTF_I = 0-based index used.
   * Empty sep: only index 0 is whole field. Promotes numâstr for walk.
   * Usability: CSV/path column without GETF+SYS CUT+SETF or SPLITF+NTH
   * (METHOD/THIS; pairs SPLITF bag + COUNTINF depth). */
  if (kw(&L->cur, "CUTF") || kw(&L->cur, "NTHFIELDF") ||
      kw(&L->cur, "FIELDNF") || kw(&L->cur, "GETCOLF") ||
      kw(&L->cur, "CSVFIELDF") || kw(&L->cur, "CUTFIELD") ||
      kw(&L->cur, "COLUMNF") || kw(&L->cur, "COLF") ||
      kw(&L->cur, "COLNF") || kw(&L->cur, "COLUMNFIELD") ||
      kw(&L->cur, "TRYCUTF") || kw(&L->cur, "CUTFSOFT") ||
      kw(&L->cur, "SOFTCUTF") || kw(&L->cur, "TRYCOLUMNF") ||
      kw(&L->cur, "TRYCOLF") || kw(&L->cur, "TRYNTHFIELDF")) {
    char oname[48], fname[48], op[24], hay[256], sep[64], out[256];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, one_based = 0;
    long want = 0, idx = 0, found = 0;
    const char *p, *hitp, *start;
    size_t sepn, flen;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "COLUMNF") == 0 || strcmp(op, "COLF") == 0 ||
        strcmp(op, "COLNF") == 0 || strcmp(op, "COLUMNFIELD") == 0 ||
        strcmp(op, "TRYCOLUMNF") == 0 || strcmp(op, "TRYCOLF") == 0)
      one_based = 1;
    if (strcmp(op, "TRYCUTF") == 0 || strcmp(op, "CUTFSOFT") == 0 ||
        strcmp(op, "SOFTCUTF") == 0 || strcmp(op, "TRYCOLUMNF") == 0 ||
        strcmp(op, "TRYCOLF") == 0 || strcmp(op, "TRYNTHFIELDF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, one_based ? "COLUMNF object field sep index"
                         : "CUTF object field sep index");
      return -1;
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
      fail(vm, one_based ? "COLUMNF field" : "CUTF field"); return -1;
    }
    if (kw(&L->cur, "BY") || kw(&L->cur, "ON") || kw(&L->cur, "SEP") ||
        kw(&L->cur, "WITH") || kw(&L->cur, "AT"))
      lex_next(L);
    sep[0] = 0;
    if (resolve_str_arg(vm, L, sep, sizeof sep) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        /* index without sep is invalid â need sep first */
        fail(vm, one_based ? "COLUMNF object field sep index"
                           : "CUTF object field sep index");
        return -1;
      } else {
        fail(vm, one_based ? "COLUMNF object field sep index"
                           : "CUTF object field sep index");
        return -1;
      }
    }
    if (kw(&L->cur, "AT") || kw(&L->cur, "INDEX") || kw(&L->cur, "N") ||
        kw(&L->cur, "COL") || kw(&L->cur, "COLUMN"))
      lex_next(L);
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
        L->cur.kind == TK_LPAREN ||
        (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        want = parse_expr(vm, L);
      } else {
        Var *dv = var_get(vm, L->cur.text, 0);
        if (dv && !dv->is_str) {
          want = dv->val;
          lex_next(L);
        } else if (strcmp(L->cur.text, "LAST") == 0) {
          want = vm->last_n;
          lex_next(L);
        } else {
          fail(vm, one_based ? "COLUMNF object field sep index"
                             : "CUTF object field sep index");
          return -1;
        }
      }
    } else {
      want = one_based ? 1 : 0;
    }
    if (one_based) {
      if (want < 1) want = 1;
      want = want - 1;
    } else {
      if (want < 0) want = 0;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      const char *tag = one_based ? "COLUMNF" : "CUTF";
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s unknown object %s", tag, oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "CUTF_N", 0);
      var_set_num(vm, "CUTF_HIT", 0);
      var_set_num(vm, "COLUMNF_N", 0);
      var_set_num(vm, "OK", 0);
      {
        char ebuf[48];
        snprintf(ebuf, sizeof ebuf, "%s: unknown object", tag);
        var_set_str(vm, "LAST_ERR", ebuf);
        var_set_str(vm, "ERR", ebuf);
      }
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      const char *tag = one_based ? "COLUMNF" : "CUTF";
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s unknown FIELD %s", tag, fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "CUTF_N", 0);
      var_set_num(vm, "CUTF_HIT", 0);
      var_set_num(vm, "COLUMNF_N", 0);
      var_set_num(vm, "OK", 0);
      {
        char ebuf[48];
        snprintf(ebuf, sizeof ebuf, "%s: unknown field", tag);
        var_set_str(vm, "LAST_ERR", ebuf);
        var_set_str(vm, "ERR", ebuf);
      }
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi])
      snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
    else
      snprintf(hay, sizeof hay, "%ld", ob->fnum[fi]);
    out[0] = 0;
    found = 0;
    sepn = strlen(sep);
    p = hay;
    idx = 0;
    if (!hay[0]) {
      found = 0;
    } else if (sepn == 0) {
      if (want == 0) {
        snprintf(out, sizeof out, "%s", hay);
        found = 1;
      }
    } else {
      while (*p) {
        start = p;
        hitp = strstr(p, sep);
        if (hitp) {
          flen = (size_t)(hitp - p);
          p = hitp + sepn;
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
        if (!hitp) break;
      }
    }
    if (found) {
      snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", out);
      ob->fis_str[fi] = 1;
    }
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = found;
    var_set_num(vm, "LAST_N", found);
    var_set_num(vm, "CUTF_N", found);
    var_set_num(vm, "CUTF_HIT", found);
    var_set_num(vm, "CUTF_I", want);
    var_set_num(vm, "COLUMNF_N", found);
    var_set_num(vm, "COLUMNF_HIT", found);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  return 0;
}
