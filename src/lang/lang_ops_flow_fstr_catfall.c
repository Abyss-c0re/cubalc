/* CubalC lang — lang_ops_flow_fstr_catfall.c
 * FLOW fstr bulk: CATFALL · PREPENDFALL · MAPCATF
 * Peel from fstr_a (B_fstr_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_flow_fstr_catfall.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_fstr_catfall(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  if (kw(&L->cur, "CATFALL") || kw(&L->cur, "APPENDFALL") ||
      kw(&L->cur, "STRCATFALL") || kw(&L->cur, "SUFFIXFALL") ||
      kw(&L->cur, "CONCATFALL") || kw(&L->cur, "ADDSUFALL") ||
      kw(&L->cur, "PREPENDFALL") || kw(&L->cur, "PREFIXFALL") ||
      kw(&L->cur, "ADDPREFALL") || kw(&L->cur, "CATF_ALL") ||
      kw(&L->cur, "APPENDF_ALL") || kw(&L->cur, "PREPENDF_ALL") ||
      kw(&L->cur, "BULKCATF") || kw(&L->cur, "BULKAPPENDF") ||
      kw(&L->cur, "BULKPREPENDF") || kw(&L->cur, "MAPCATF") ||
      kw(&L->cur, "MAPAPPENDF") || kw(&L->cur, "MAPPREPENDF")) {
    char filt[48], fname[48], tok1[48], piece[256], op[24], out[256];
    int has_filt = 0, is_pre = 0, i, n = 0, n_skip = 0;
    size_t pl, ol;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "PREPENDFALL") == 0 || strcmp(op, "PREFIXFALL") == 0 ||
        strcmp(op, "ADDPREFALL") == 0 || strcmp(op, "PREPENDF_ALL") == 0 ||
        strcmp(op, "BULKPREPENDF") == 0 || strcmp(op, "MAPPREPENDF") == 0)
      is_pre = 1;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    piece[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "CATFALL OF Class field string"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
          !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
          !kw(&L->cur, "END") && !kw(&L->cur, "NEW")) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(fname, sizeof fname, "%s", vv->sval);
          else
            snprintf(fname, sizeof fname, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_STR || L->cur.kind == TK_NUM ||
             L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "CATFALL [Class] field string"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "CATFALL [Class] field string"); return -1;
      }
    }
    if (kw(&L->cur, "WITH") || kw(&L->cur, "PLUS") || kw(&L->cur, "AND") ||
        kw(&L->cur, "APPEND") || kw(&L->cur, "PREPEND") || kw(&L->cur, "BY"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(piece, sizeof piece, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(piece, sizeof piece, "%s", vm->last_str);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(piece, sizeof piece, "%s", sv->sval);
        lex_next(L);
      } else if (sv && !sv->is_str) {
        snprintf(piece, sizeof piece, "%ld", sv->val);
        lex_next(L);
      } else {
        fail(vm, "CATFALL [Class] field string"); return -1;
      }
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN) {
      long v = parse_expr(vm, L);
      snprintf(piece, sizeof piece, "%ld", v);
    } else {
      fail(vm, "CATFALL [Class] field string"); return -1;
    }
    pl = strlen(piece);
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi])
        snprintf(out, sizeof out, "%s", ob->fstr[fi]);
      else
        snprintf(out, sizeof out, "%ld", ob->fnum[fi]);
      ol = strlen(out);
      if (is_pre) {
        size_t puse = pl, ouse = ol;
        if (puse + ouse >= sizeof ob->fstr[fi]) {
          if (puse >= sizeof ob->fstr[fi]) puse = sizeof ob->fstr[fi] - 1;
          if (puse + ouse >= sizeof ob->fstr[fi])
            ouse = sizeof ob->fstr[fi] - 1 - puse;
        }
        {
          char tmp[sizeof ob->fstr[0]];
          size_t j;
          for (j = 0; j < puse; j++) tmp[j] = piece[j];
          for (j = 0; j < ouse && puse + j + 1 < sizeof tmp; j++)
            tmp[puse + j] = out[j];
          tmp[puse + ouse < sizeof tmp ? puse + ouse : sizeof tmp - 1] = 0;
          snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", tmp);
        }
      } else {
        size_t puse = pl, ouse = ol;
        if (ouse + puse >= sizeof ob->fstr[fi]) {
          if (ouse >= sizeof ob->fstr[fi]) ouse = sizeof ob->fstr[fi] - 1;
          if (ouse + puse >= sizeof ob->fstr[fi])
            puse = sizeof ob->fstr[fi] - 1 - ouse;
        }
        {
          size_t j;
          for (j = 0; j < ouse && j + 1 < sizeof ob->fstr[fi]; j++)
            ob->fstr[fi][j] = out[j];
          for (j = 0; j < puse && ouse + j + 1 < sizeof ob->fstr[fi]; j++)
            ob->fstr[fi][ouse + j] = piece[j];
          ob->fstr[fi][ouse + puse < sizeof ob->fstr[fi] ? ouse + puse
                                                         : sizeof ob->fstr[fi] - 1] = 0;
        }
      }
      ob->fis_str[fi] = 1;
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "CATFALL_N", n);
    var_set_num(vm, "APPENDFALL_N", is_pre ? 0 : n);
    var_set_num(vm, "PREPENDFALL_N", is_pre ? n : 0);
    var_set_num(vm, "CATFALL_SKIP", n_skip);
    var_set_num(vm, "APPENDFALL_SKIP", n_skip);
    var_set_num(vm, "PREPENDFALL_SKIP", n_skip);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0])
      var_set_str(vm, "CLASS", filt);
    var_set_str(vm, "LAST", piece);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", piece);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  return 0;
}
