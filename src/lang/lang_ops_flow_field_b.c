/* CubalC lang — lang_ops_flow_field_b.c
 * SUMF·WHERE·COUNT·DELETE field agg plane (A3 peel from flow_field).
 * Mesh vote · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_flow_field_b.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_field_b(VM *vm, Lex *L) {
  /* Returns 1 handled, 0 not this plane. */
  if (kw(&L->cur, "SUMF") || kw(&L->cur, "SUMFALL") ||
      kw(&L->cur, "TOTALF") || kw(&L->cur, "FIELDSUM") ||
      kw(&L->cur, "SUMFIELD") || kw(&L->cur, "OBJSSUM") ||
      kw(&L->cur, "AVGF") || kw(&L->cur, "AVGFALL") ||
      kw(&L->cur, "MEANF") || kw(&L->cur, "FIELDAVG") ||
      kw(&L->cur, "AVGFIELD") || kw(&L->cur, "OBJSAVG")) {
    char filt[48], fname[48], tok1[48], op[24];
    int has_filt = 0, want_avg = 0, i, n = 0, n_skip = 0;
    long sum = 0, outv = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "AVGF") == 0 || strcmp(op, "AVGFALL") == 0 ||
        strcmp(op, "MEANF") == 0 || strcmp(op, "FIELDAVG") == 0 ||
        strcmp(op, "AVGFIELD") == 0 || strcmp(op, "OBJSAVG") == 0)
      want_avg = 1;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "BY")) {
      if (kw(&L->cur, "BY")) {
        lex_next(L);
      } else {
        lex_next(L);
        if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
          fail(vm, "SUMF OF Class field"); return -1;
        }
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      }
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
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
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "SUMF [Class] field"); return -1;
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
        fail(vm, "SUMF [Class] field"); return -1;
      }
    }
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
      if (ob->fis_str[fi]) { n_skip++; continue; }
      sum += ob->fnum[fi];
      n++;
    }
    if (want_avg)
      outv = (n > 0) ? (sum / n) : 0;
    else
      outv = sum;
    var_set_num(vm, "LAST_N", outv);
    vm->last_n = outv;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", outv);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "SUMF_N", n);
    var_set_num(vm, "AVGF_N", n);
    var_set_num(vm, "SUMFALL_N", n);
    var_set_num(vm, "SUMF_SKIP", n_skip);
    var_set_num(vm, "SUMF_SUM", sum);
    var_set_num(vm, "AVGF_SUM", sum);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SUMWHERE|TOTALWHERE|SUMFWHERE|AVGWHERE|MEANWHERE [Class] sumfield matchfield matchvalue
   * â sum or integer mean of numeric sumfield where matchfield == matchvalue.
   * WHERE sugar: SUMWHERE Cell energy WHERE age 1.
   * LAST_N = result; SUMWHERE_N = sample count; soft empty â 0.
   * Usability: filtered SUMF without GETFWHERE+SYS SUM glue. */
  if (kw(&L->cur, "SUMWHERE") || kw(&L->cur, "TOTALWHERE") ||
      kw(&L->cur, "SUMFWHERE") || kw(&L->cur, "WHERESUM") ||
      kw(&L->cur, "SUMIF") || kw(&L->cur, "FIELDSUMWHERE") ||
      kw(&L->cur, "AVGWHERE") || kw(&L->cur, "MEANWHERE") ||
      kw(&L->cur, "AVGFWHERE") || kw(&L->cur, "WHEREAVG") ||
      kw(&L->cur, "AVGIF") || kw(&L->cur, "MEANIF")) {
    char filt[48], sfield[48], mfield[48], tok1[48], op[24];
    char m_sval[512];
    int has_filt = 0, m_is_str = 0, want_avg = 0, i, n = 0, n_skip = 0;
    long m_nval = 0, sum = 0, outv = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "AVGWHERE") == 0 || strcmp(op, "MEANWHERE") == 0 ||
        strcmp(op, "AVGFWHERE") == 0 || strcmp(op, "WHEREAVG") == 0 ||
        strcmp(op, "AVGIF") == 0 || strcmp(op, "MEANIF") == 0)
      want_avg = 1;
    lex_next(L);
    filt[0] = 0;
    sfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "SUMWHERE OF Class sumfield matchfield matchvalue");
        return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN")) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(sfield, sizeof sfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(sfield, sizeof sfield, "%s", vv->sval);
          else
            snprintf(sfield, sizeof sfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
             kw(&L->cur, "WHERE")))
          snprintf(sfield, sizeof sfield, "%s", vv->sval);
        else
          snprintf(sfield, sizeof sfield, "%s", tok1);
      }
    } else {
      fail(vm, "SUMWHERE [Class] sumfield matchfield matchvalue");
      return -1;
    }
    if (!sfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(sfield, sizeof sfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(sfield, sizeof sfield, "%s", vv->sval);
        else
          snprintf(sfield, sizeof sfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "SUMWHERE [Class] sumfield matchfield matchvalue");
        return -1;
      }
    }
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "SUMWHERE [Class] sumfield matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int sfi, mfi, hit = 0;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      sfi = oop_field_idx(cd, sfield);
      if (mfi < 0 || sfi < 0) { n_skip++; continue; }
      if (ob->fis_str[sfi]) { n_skip++; continue; } /* sum numeric only */
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
      sum += ob->fnum[sfi];
      n++;
    }
    if (want_avg)
      outv = (n > 0) ? (sum / n) : 0;
    else
      outv = sum;
    var_set_num(vm, "LAST_N", outv);
    vm->last_n = outv;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", outv);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "SUMWHERE_N", n);
    var_set_num(vm, "AVGWHERE_N", n);
    var_set_num(vm, "TOTALWHERE_N", n);
    var_set_num(vm, "SUMFWHERE_N", n);
    var_set_num(vm, "WHERESUM_N", n);
    var_set_num(vm, "SUMWHERE_SKIP", n_skip);
    var_set_num(vm, "SUMWHERE_SUM", sum);
    var_set_num(vm, "AVGWHERE_SUM", sum);
    var_set_str(vm, "FIELD", sfield);
    var_set_str(vm, "SRC", mfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* MINWHERE|MAXWHERE|MINFWHERE|MAXFWHERE [Class] valfield matchfield matchvalue
   * â min or max of numeric valfield where matchfield == matchvalue.
   * WHERE sugar: MINWHERE Cell energy WHERE age 1.
   * LAST_N = extreme; MINWHERE_N = sample count; soft empty â 0.
   * Usability: filtered MIN/MAX without GETFWHERE+SYS MIN/MAX glue. */
  if (kw(&L->cur, "MINWHERE") || kw(&L->cur, "MAXWHERE") ||
      kw(&L->cur, "MINFWHERE") || kw(&L->cur, "MAXFWHERE") ||
      kw(&L->cur, "WHEREMIN") || kw(&L->cur, "WHEREMAX") ||
      kw(&L->cur, "MINIF") || kw(&L->cur, "MAXIF") ||
      kw(&L->cur, "LOWESTWHERE") || kw(&L->cur, "HIGHESTWHERE") ||
      kw(&L->cur, "BOTWHERE") || kw(&L->cur, "TOPWHERE")) {
    char filt[48], vfield[48], mfield[48], tok1[48], op[24];
    char m_sval[512];
    int has_filt = 0, m_is_str = 0, want_max = 0, i, n = 0, n_skip = 0, have = 0;
    long m_nval = 0, best = 0, outv = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "MAXWHERE") == 0 || strcmp(op, "MAXFWHERE") == 0 ||
        strcmp(op, "WHEREMAX") == 0 || strcmp(op, "MAXIF") == 0 ||
        strcmp(op, "HIGHESTWHERE") == 0 || strcmp(op, "TOPWHERE") == 0)
      want_max = 1;
    lex_next(L);
    filt[0] = 0;
    vfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "MINWHERE OF Class valfield matchfield matchvalue");
        return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN")) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(vfield, sizeof vfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(vfield, sizeof vfield, "%s", vv->sval);
          else
            snprintf(vfield, sizeof vfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
             kw(&L->cur, "WHERE")))
          snprintf(vfield, sizeof vfield, "%s", vv->sval);
        else
          snprintf(vfield, sizeof vfield, "%s", tok1);
      }
    } else {
      fail(vm, "MINWHERE [Class] valfield matchfield matchvalue");
      return -1;
    }
    if (!vfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(vfield, sizeof vfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(vfield, sizeof vfield, "%s", vv->sval);
        else
          snprintf(vfield, sizeof vfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "MINWHERE [Class] valfield matchfield matchvalue");
        return -1;
      }
    }
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "MINWHERE [Class] valfield matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int vfi, mfi, hit = 0;
      long v;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      vfi = oop_field_idx(cd, vfield);
      if (mfi < 0 || vfi < 0) { n_skip++; continue; }
      if (ob->fis_str[vfi]) { n_skip++; continue; }
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
      v = ob->fnum[vfi];
      if (!have) {
        best = v;
        have = 1;
      } else if (want_max) {
        if (v > best) best = v;
      } else {
        if (v < best) best = v;
      }
      n++;
    }
    outv = have ? best : 0;
    var_set_num(vm, "LAST_N", outv);
    vm->last_n = outv;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", outv);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "MINWHERE_N", n);
    var_set_num(vm, "MAXWHERE_N", n);
    var_set_num(vm, "MINFWHERE_N", n);
    var_set_num(vm, "MAXFWHERE_N", n);
    var_set_num(vm, "WHEREMIN_N", n);
    var_set_num(vm, "WHEREMAX_N", n);
    var_set_num(vm, "MINWHERE_SKIP", n_skip);
    var_set_num(vm, "MAXWHERE_SKIP", n_skip);
    var_set_num(vm, "MINWHERE_VAL", outv);
    var_set_num(vm, "MAXWHERE_VAL", outv);
    var_set_str(vm, "FIELD", vfield);
    var_set_str(vm, "SRC", mfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* MEDIANWHERE|P50WHERE|MIDWHERE|MEDIANFWHERE [Class] valfield matchfield matchvalue
   * â integer median of numeric valfield where matchfield == matchvalue.
   * WHERE sugar: MEDIANWHERE Cell energy WHERE age 1.
   * Sort ascending; odd â middle; even â lower mid (matches MEDIANF/SYS MEDIAN).
   * LAST_N = median; MEDIANWHERE_N = sample count; soft empty â 0.
   * Usability: filtered MEDIANF without GETFWHERE+SYS MEDIAN glue. */
  if (kw(&L->cur, "MEDIANWHERE") || kw(&L->cur, "P50WHERE") ||
      kw(&L->cur, "MIDWHERE") || kw(&L->cur, "MEDIANFWHERE") ||
      kw(&L->cur, "WHEREMEDIAN") || kw(&L->cur, "MEDIANIF") ||
      kw(&L->cur, "P50IF") || kw(&L->cur, "MIDIF") ||
      kw(&L->cur, "FIELDMEDIANWHERE") || kw(&L->cur, "WHEREP50")) {
    char filt[48], vfield[48], mfield[48], tok1[48];
    char m_sval[512];
    long vals[CUBALC_MAX_OBJS];
    int has_filt = 0, m_is_str = 0, i, j, n = 0, n_skip = 0;
    long m_nval = 0, outv = 0;
    lex_next(L);
    filt[0] = 0;
    vfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "MEDIANWHERE OF Class valfield matchfield matchvalue");
        return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN")) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(vfield, sizeof vfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(vfield, sizeof vfield, "%s", vv->sval);
          else
            snprintf(vfield, sizeof vfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
             kw(&L->cur, "WHERE")))
          snprintf(vfield, sizeof vfield, "%s", vv->sval);
        else
          snprintf(vfield, sizeof vfield, "%s", tok1);
      }
    } else {
      fail(vm, "MEDIANWHERE [Class] valfield matchfield matchvalue");
      return -1;
    }
    if (!vfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(vfield, sizeof vfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(vfield, sizeof vfield, "%s", vv->sval);
        else
          snprintf(vfield, sizeof vfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "MEDIANWHERE [Class] valfield matchfield matchvalue");
        return -1;
      }
    }
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "MEDIANWHERE [Class] valfield matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
    }
    for (i = 0; i < vm->n_objs && n < CUBALC_MAX_OBJS; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int vfi, mfi, hit = 0;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      vfi = oop_field_idx(cd, vfield);
      if (mfi < 0 || vfi < 0) { n_skip++; continue; }
      if (ob->fis_str[vfi]) { n_skip++; continue; }
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
      vals[n++] = ob->fnum[vfi];
    }
    /* insertion sort ascending */
    for (i = 1; i < n; i++) {
      long key = vals[i];
      j = i - 1;
      while (j >= 0 && vals[j] > key) {
        vals[j + 1] = vals[j];
        j--;
      }
      vals[j + 1] = key;
    }
    if (n == 0)
      outv = 0;
    else if (n & 1)
      outv = vals[n / 2];
    else
      outv = vals[n / 2 - 1]; /* lower mid for even â match MEDIANF/SYS MEDIAN */
    var_set_num(vm, "LAST_N", outv);
    vm->last_n = outv;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", outv);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "MEDIANWHERE", outv);
    var_set_num(vm, "MEDIANWHERE_N", n);
    var_set_num(vm, "P50WHERE_N", n);
    var_set_num(vm, "MIDWHERE_N", n);
    var_set_num(vm, "MEDIANFWHERE_N", n);
    var_set_num(vm, "WHEREMEDIAN_N", n);
    var_set_num(vm, "MEDIANWHERE_SKIP", n_skip);
    var_set_num(vm, "MEDIANWHERE_VAL", outv);
    var_set_str(vm, "FIELD", vfield);
    var_set_str(vm, "SRC", mfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* MEDIANF|P50F|MIDF|MEDIANFALL [Class] field
   * â integer median of numeric field over live objects (optional class).
   * Sort ascending; odd â middle; even â lower mid (matches SYS MEDIAN).
   * LAST_N = median; MEDIANF_N = sample count; soft empty â 0.
   * Usability: robust fleet mid vs AVGF outliers without GETFALL+SYS MEDIAN. */
  if (kw(&L->cur, "MEDIANF") || kw(&L->cur, "P50F") ||
      kw(&L->cur, "MIDF") || kw(&L->cur, "MEDIANFALL") ||
      kw(&L->cur, "FIELDMEDIAN") || kw(&L->cur, "OBJSMEDIAN") ||
      kw(&L->cur, "MEDF") || kw(&L->cur, "MIDFALL")) {
    char filt[48], fname[48], tok1[48];
    long vals[CUBALC_MAX_OBJS];
    int has_filt = 0, i, j, n = 0, n_skip = 0;
    long outv = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "BY")) {
      if (kw(&L->cur, "BY")) {
        lex_next(L);
      } else {
        lex_next(L);
        if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
          fail(vm, "MEDIANF OF Class field"); return -1;
        }
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      }
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
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
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "MEDIANF [Class] field"); return -1;
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
        fail(vm, "MEDIANF [Class] field"); return -1;
      }
    }
    for (i = 0; i < vm->n_objs && n < CUBALC_MAX_OBJS; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      vals[n++] = ob->fnum[fi];
    }
    /* insertion sort ascending */
    for (i = 1; i < n; i++) {
      long key = vals[i];
      j = i - 1;
      while (j >= 0 && vals[j] > key) {
        vals[j + 1] = vals[j];
        j--;
      }
      vals[j + 1] = key;
    }
    if (n == 0)
      outv = 0;
    else if (n & 1)
      outv = vals[n / 2];
    else
      outv = vals[n / 2 - 1]; /* lower mid for even â match SYS MEDIAN */
    var_set_num(vm, "LAST_N", outv);
    vm->last_n = outv;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", outv);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "MEDIANF", outv);
    var_set_num(vm, "MEDIANF_N", n);
    var_set_num(vm, "P50F_N", n);
    var_set_num(vm, "MIDF_N", n);
    var_set_num(vm, "MEDIANFALL_N", n);
    var_set_num(vm, "MEDIANF_SKIP", n_skip);
    var_set_num(vm, "MEDIANF_VAL", outv);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* FREQF|HISTF|COUNTF|FREQFALL [Class] field
   * â frequency histogram of field values over live objects (num or str).
   * LAST = "val:count" bag (first-seen order, sep ":"); LAST_N = distinct keys.
   * FREQF_TOTAL = sample count; soft empty â empty bag / 0.
   * Usability: fleet status rollups without GETFALL+SYS FREQ / EACH+KVINC. */
  if (kw(&L->cur, "FREQF") || kw(&L->cur, "HISTF") ||
      kw(&L->cur, "COUNTF") || kw(&L->cur, "FREQFALL") ||
      kw(&L->cur, "HISTFALL") || kw(&L->cur, "FIELDFREQ") ||
      kw(&L->cur, "OBJSFREQ") || kw(&L->cur, "FREQFIELD") ||
      kw(&L->cur, "HISTFIELD") || kw(&L->cur, "VALUEFREQ") ||
      kw(&L->cur, "ROLLUPF")) {
    char filt[48], fname[48], tok1[48];
    char keys[64][128];
    long counts[64];
    char out[CUBALC_HOST_STR_MAX];
    int has_filt = 0, i, k, nk = 0, n_skip = 0, total = 0;
    size_t olen = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    out[0] = 0;
    memset(counts, 0, sizeof counts);
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "BY")) {
      if (kw(&L->cur, "BY")) {
        lex_next(L);
      } else {
        lex_next(L);
        if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
          fail(vm, "FREQF OF Class field"); return -1;
        }
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      }
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
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
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "FREQF [Class] field"); return -1;
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
        fail(vm, "FREQF [Class] field"); return -1;
      }
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      char field[128];
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi])
        snprintf(field, sizeof field, "%s", ob->fstr[fi]);
      else
        snprintf(field, sizeof field, "%ld", ob->fnum[fi]);
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
      }
    }
    for (k = 0; k < nk; k++) {
      char line[160];
      int n;
      n = snprintf(line, sizeof line, "%s:%ld", keys[k], counts[k]);
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
    var_set_str(vm, "FREQF", out);
    var_set_str(vm, "HISTF", out);
    var_set_str(vm, "FREQFALL", out);
    var_set_str(vm, "FREQ", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = nk;
    var_set_num(vm, "LAST_N", nk);
    var_set_num(vm, "FREQF_N", nk);
    var_set_num(vm, "HISTF_N", nk);
    var_set_num(vm, "COUNTF_N", nk);
    var_set_num(vm, "FREQFALL_N", nk);
    var_set_num(vm, "FREQF_TOTAL", total);
    var_set_num(vm, "HISTF_TOTAL", total);
    var_set_num(vm, "FREQF_SKIP", n_skip);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* FREQWHERE|HISTWHERE|FREQFWHERE [Class] histfield matchfield matchvalue
   * â frequency histogram of histfield where matchfield == matchvalue.
   * WHERE sugar: FREQWHERE Ticket severity WHERE status "open".
   * LAST = "val:count" bag (first-seen order); LAST_N = distinct keys.
   * FREQWHERE_TOTAL = matched samples; soft empty â empty bag / 0.
   * Usability: filtered FREQF without GETFWHERE+SYS FREQ glue. */
  if (kw(&L->cur, "FREQWHERE") || kw(&L->cur, "HISTWHERE") ||
      kw(&L->cur, "FREQFWHERE") || kw(&L->cur, "WHEREFREQ") ||
      kw(&L->cur, "FREQIF") || kw(&L->cur, "HISTIF") ||
      kw(&L->cur, "HISTFWHERE") || kw(&L->cur, "FIELDFREQWHERE") ||
      kw(&L->cur, "WHEREHIST") || kw(&L->cur, "ROLLUPWHERE")) {
    char filt[48], hfield[48], mfield[48], tok1[48];
    char m_sval[512];
    char keys[64][128];
    long counts[64];
    char out[CUBALC_HOST_STR_MAX];
    int has_filt = 0, m_is_str = 0, i, k, nk = 0, n_skip = 0, total = 0;
    long m_nval = 0;
    size_t olen = 0;
    lex_next(L);
    filt[0] = 0;
    hfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    out[0] = 0;
    memset(counts, 0, sizeof counts);
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "FREQWHERE OF Class histfield matchfield matchvalue");
        return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN")) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(hfield, sizeof hfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(hfield, sizeof hfield, "%s", vv->sval);
          else
            snprintf(hfield, sizeof hfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
             kw(&L->cur, "WHERE")))
          snprintf(hfield, sizeof hfield, "%s", vv->sval);
        else
          snprintf(hfield, sizeof hfield, "%s", tok1);
      }
    } else {
      fail(vm, "FREQWHERE [Class] histfield matchfield matchvalue");
      return -1;
    }
    if (!hfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(hfield, sizeof hfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(hfield, sizeof hfield, "%s", vv->sval);
        else
          snprintf(hfield, sizeof hfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "FREQWHERE [Class] histfield matchfield matchvalue");
        return -1;
      }
    }
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "FREQWHERE [Class] histfield matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int hfi, mfi, hit = 0;
      char field[128];
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      hfi = oop_field_idx(cd, hfield);
      if (mfi < 0 || hfi < 0) { n_skip++; continue; }
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
      if (ob->fis_str[hfi])
        snprintf(field, sizeof field, "%s", ob->fstr[hfi]);
      else
        snprintf(field, sizeof field, "%ld", ob->fnum[hfi]);
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
      }
    }
    for (k = 0; k < nk; k++) {
      char line[160];
      int n;
      n = snprintf(line, sizeof line, "%s:%ld", keys[k], counts[k]);
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
    var_set_str(vm, "FREQWHERE", out);
    var_set_str(vm, "HISTWHERE", out);
    var_set_str(vm, "FREQFWHERE", out);
    var_set_str(vm, "FREQ", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = nk;
    var_set_num(vm, "LAST_N", nk);
    var_set_num(vm, "FREQWHERE_N", nk);
    var_set_num(vm, "HISTWHERE_N", nk);
    var_set_num(vm, "FREQFWHERE_N", nk);
    var_set_num(vm, "WHEREFREQ_N", nk);
    var_set_num(vm, "FREQWHERE_TOTAL", total);
    var_set_num(vm, "HISTWHERE_TOTAL", total);
    var_set_num(vm, "FREQWHERE_SKIP", n_skip);
    var_set_str(vm, "FIELD", hfield);
    var_set_str(vm, "SRC", mfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* MODEF|TOPF|MODEFALL|DOMINANTF [Class] field
   * â most frequent field value over live objects (num or str).
   * LAST = mode value (first-seen on ties); LAST_N = mode count.
   * MODEF_TOTAL = samples; MODEF_DISTINCT = unique keys; soft empty â "".
   * Usability: dominant status/energy without FREQF+SYS TOPKEY glue. */
  if (kw(&L->cur, "MODEF") || kw(&L->cur, "TOPF") ||
      kw(&L->cur, "MODEFALL") || kw(&L->cur, "DOMINANTF") ||
      kw(&L->cur, "MOSTF") || kw(&L->cur, "COMMONF") ||
      kw(&L->cur, "FIELDMODE") || kw(&L->cur, "OBJSMODE") ||
      kw(&L->cur, "TOPVALUE") || kw(&L->cur, "MODEFIELD")) {
    char filt[48], fname[48], tok1[48];
    char keys[64][128];
    long counts[64];
    char best[128];
    int has_filt = 0, i, k, nk = 0, n_skip = 0, total = 0, best_i = -1;
    long best_c = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    best[0] = 0;
    memset(counts, 0, sizeof counts);
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "BY")) {
      if (kw(&L->cur, "BY")) {
        lex_next(L);
      } else {
        lex_next(L);
        if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
          fail(vm, "MODEF OF Class field"); return -1;
        }
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      }
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
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
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "MODEF [Class] field"); return -1;
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
        fail(vm, "MODEF [Class] field"); return -1;
      }
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      char field[128];
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi])
        snprintf(field, sizeof field, "%s", ob->fstr[fi]);
      else
        snprintf(field, sizeof field, "%ld", ob->fnum[fi]);
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
      }
    }
    for (k = 0; k < nk; k++) {
      if (best_i < 0 || counts[k] > best_c) {
        best_c = counts[k];
        best_i = k;
      }
    }
    if (best_i >= 0)
      snprintf(best, sizeof best, "%s", keys[best_i]);
    var_set_str(vm, "LAST", best);
    var_set_str(vm, "MODEF", best);
    var_set_str(vm, "TOPF", best);
    var_set_str(vm, "MODEFALL", best);
    var_set_str(vm, "DOMINANTF", best);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", best);
    vm->last_n = best_c;
    var_set_num(vm, "LAST_N", best_c);
    var_set_num(vm, "MODEF_COUNT", best_c);
    var_set_num(vm, "TOPF_COUNT", best_c);
    var_set_num(vm, "MODEF_N", total);
    var_set_num(vm, "MODEFALL_N", total);
    var_set_num(vm, "TOPF_N", total);
    var_set_num(vm, "MODEF_TOTAL", total);
    var_set_num(vm, "MODEF_DISTINCT", nk);
    var_set_num(vm, "MODEF_SKIP", n_skip);
    /* numeric peel when mode key is integer text */
    {
      char *end = 0;
      long nv;
      if (best[0]) {
        nv = strtol(best, &end, 10);
        if (end && end != best && *end == 0)
          var_set_num(vm, "MODEF_NUM", nv);
        else
          var_set_num(vm, "MODEF_NUM", 0);
      } else {
        var_set_num(vm, "MODEF_NUM", 0);
      }
    }
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* MODEWHERE|DOMINANTWHERE|MODEFWHERE [Class] valfield matchfield matchvalue
   * â most frequent valfield where matchfield == matchvalue.
   * WHERE sugar: MODEWHERE Ticket severity WHERE status "open".
   * LAST = mode value (first-seen on ties); LAST_N = mode count.
   * MODEWHERE_TOTAL = matched samples; soft empty â "" / 0.
   * Usability: filtered MODEF without FREQWHERE+TOPKEY glue. */
  if (kw(&L->cur, "MODEWHERE") || kw(&L->cur, "DOMINANTWHERE") ||
      kw(&L->cur, "MODEFWHERE") || kw(&L->cur, "WHEREMODE") ||
      kw(&L->cur, "MODEIF") || kw(&L->cur, "MOSTWHERE") ||
      kw(&L->cur, "COMMONWHERE") || kw(&L->cur, "FIELDMODEWHERE") ||
      kw(&L->cur, "WHEREDOMINANT") || kw(&L->cur, "MODEMATCH")) {
    char filt[48], vfield[48], mfield[48], tok1[48];
    char m_sval[512];
    char keys[64][128];
    long counts[64];
    char best[128];
    int has_filt = 0, m_is_str = 0, i, k, nk = 0, n_skip = 0, total = 0, best_i = -1;
    long m_nval = 0, best_c = 0;
    lex_next(L);
    filt[0] = 0;
    vfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    best[0] = 0;
    memset(counts, 0, sizeof counts);
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "MODEWHERE OF Class valfield matchfield matchvalue");
        return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN")) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(vfield, sizeof vfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(vfield, sizeof vfield, "%s", vv->sval);
          else
            snprintf(vfield, sizeof vfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
             kw(&L->cur, "WHERE")))
          snprintf(vfield, sizeof vfield, "%s", vv->sval);
        else
          snprintf(vfield, sizeof vfield, "%s", tok1);
      }
    } else {
      fail(vm, "MODEWHERE [Class] valfield matchfield matchvalue");
      return -1;
    }
    if (!vfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(vfield, sizeof vfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(vfield, sizeof vfield, "%s", vv->sval);
        else
          snprintf(vfield, sizeof vfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "MODEWHERE [Class] valfield matchfield matchvalue");
        return -1;
      }
    }
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "MODEWHERE [Class] valfield matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int vfi, mfi, hit = 0;
      char field[128];
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      vfi = oop_field_idx(cd, vfield);
      if (mfi < 0 || vfi < 0) { n_skip++; continue; }
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
      if (ob->fis_str[vfi])
        snprintf(field, sizeof field, "%s", ob->fstr[vfi]);
      else
        snprintf(field, sizeof field, "%ld", ob->fnum[vfi]);
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
      }
    }
    for (k = 0; k < nk; k++) {
      if (best_i < 0 || counts[k] > best_c) {
        best_c = counts[k];
        best_i = k;
      }
    }
    if (best_i >= 0)
      snprintf(best, sizeof best, "%s", keys[best_i]);
    var_set_str(vm, "LAST", best);
    var_set_str(vm, "MODEWHERE", best);
    var_set_str(vm, "DOMINANTWHERE", best);
    var_set_str(vm, "MODEFWHERE", best);
    var_set_str(vm, "MODE", best);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", best);
    vm->last_n = best_c;
    var_set_num(vm, "LAST_N", best_c);
    var_set_num(vm, "MODEWHERE_COUNT", best_c);
    var_set_num(vm, "DOMINANTWHERE_COUNT", best_c);
    var_set_num(vm, "MODEWHERE_N", total);
    var_set_num(vm, "MODEFWHERE_N", total);
    var_set_num(vm, "MODEWHERE_TOTAL", total);
    var_set_num(vm, "MODEWHERE_DISTINCT", nk);
    var_set_num(vm, "MODEWHERE_SKIP", n_skip);
    {
      char *end = 0;
      long nv;
      if (best[0]) {
        nv = strtol(best, &end, 10);
        if (end && end != best && *end == 0)
          var_set_num(vm, "MODEWHERE_NUM", nv);
        else
          var_set_num(vm, "MODEWHERE_NUM", 0);
      } else {
        var_set_num(vm, "MODEWHERE_NUM", 0);
      }
    }
    var_set_str(vm, "FIELD", vfield);
    var_set_str(vm, "SRC", mfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* UNIQUF|DISTINCTF|UNIQUEFALL|UNIQF [Class] field
   * â unique field values over live objects as newline bag (first-seen order).
   * LAST = bag; LAST_N = distinct count; UNIQUF_TOTAL = samples.
   * Num or str fields. Soft empty â "" / 0.
   * Usability: "what statuses exist?" without FREQF+KEYS or EACH+HASLINE. */
  if (kw(&L->cur, "UNIQUF") || kw(&L->cur, "DISTINCTF") ||
      kw(&L->cur, "UNIQUEFALL") || kw(&L->cur, "UNIQF") ||
      kw(&L->cur, "UNIQUEF") || kw(&L->cur, "FIELDUNIQ") ||
      kw(&L->cur, "OBJSUNIQ") || kw(&L->cur, "DISTINCTFALL") ||
      kw(&L->cur, "UNIQVALUES") || kw(&L->cur, "UNIQUEVALUES")) {
    char filt[48], fname[48], tok1[48];
    char keys[64][128];
    char out[CUBALC_HOST_STR_MAX];
    int has_filt = 0, i, k, nk = 0, n_skip = 0, total = 0;
    size_t olen = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    out[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "BY")) {
      if (kw(&L->cur, "BY")) {
        lex_next(L);
      } else {
        lex_next(L);
        if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
          fail(vm, "UNIQUF OF Class field"); return -1;
        }
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      }
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
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
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "UNIQUF [Class] field"); return -1;
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
        fail(vm, "UNIQUF [Class] field"); return -1;
      }
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      char field[128];
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi])
        snprintf(field, sizeof field, "%s", ob->fstr[fi]);
      else
        snprintf(field, sizeof field, "%ld", ob->fnum[fi]);
      total++;
      for (k = 0; k < nk; k++) {
        if (strcmp(keys[k], field) == 0)
          break;
      }
      if (k == nk && nk < 64) {
        snprintf(keys[nk], sizeof keys[0], "%s", field);
        nk++;
      }
    }
    for (k = 0; k < nk; k++) {
      size_t ln = strlen(keys[k]);
      if (k > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
      if (olen + ln < sizeof out) {
        memcpy(out + olen, keys[k], ln);
        olen += ln;
      } else if (olen < sizeof out - 1) {
        size_t t = sizeof out - 1 - olen;
        memcpy(out + olen, keys[k], t);
        olen += t;
      }
      out[olen] = 0;
    }
    var_set_str(vm, "LAST", out);
    var_set_str(vm, "UNIQUF", out);
    var_set_str(vm, "DISTINCTF", out);
    var_set_str(vm, "UNIQUEFALL", out);
    var_set_str(vm, "UNIQF", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = nk;
    var_set_num(vm, "LAST_N", nk);
    var_set_num(vm, "UNIQUF_N", nk);
    var_set_num(vm, "DISTINCTF_N", nk);
    var_set_num(vm, "UNIQUEFALL_N", nk);
    var_set_num(vm, "UNIQF_N", nk);
    var_set_num(vm, "UNIQUF_TOTAL", total);
    var_set_num(vm, "DISTINCTF_TOTAL", total);
    var_set_num(vm, "UNIQUF_SKIP", n_skip);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* UNIQUWHERE|DISTINCTWHERE|UNIQUEFWHERE [Class] valfield matchfield matchvalue
   * â unique valfield values where matchfield == matchvalue (first-seen bag).
   * WHERE sugar: UNIQUWHERE Ticket severity WHERE status "open".
   * LAST = newline bag; LAST_N = distinct; UNIQUWHERE_TOTAL = matched samples.
   * Soft empty â "" / 0.
   * Usability: filtered UNIQUF without GETFWHERE+UNIQ glue. */
  if (kw(&L->cur, "UNIQUWHERE") || kw(&L->cur, "DISTINCTWHERE") ||
      kw(&L->cur, "UNIQUEFWHERE") || kw(&L->cur, "WHEREUNIQ") ||
      kw(&L->cur, "UNIQWHERE") || kw(&L->cur, "UNIQUEWHERE") ||
      kw(&L->cur, "UNIQUIF") || kw(&L->cur, "DISTINCTIF") ||
      kw(&L->cur, "WHEREDISTINCT") || kw(&L->cur, "FIELDUNIQUWHERE")) {
    char filt[48], vfield[48], mfield[48], tok1[48];
    char m_sval[512];
    char keys[64][128];
    char out[CUBALC_HOST_STR_MAX];
    int has_filt = 0, m_is_str = 0, i, k, nk = 0, n_skip = 0, total = 0;
    long m_nval = 0;
    size_t olen = 0;
    lex_next(L);
    filt[0] = 0;
    vfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    out[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "UNIQUWHERE OF Class valfield matchfield matchvalue");
        return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN")) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(vfield, sizeof vfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(vfield, sizeof vfield, "%s", vv->sval);
          else
            snprintf(vfield, sizeof vfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
             kw(&L->cur, "WHERE")))
          snprintf(vfield, sizeof vfield, "%s", vv->sval);
        else
          snprintf(vfield, sizeof vfield, "%s", tok1);
      }
    } else {
      fail(vm, "UNIQUWHERE [Class] valfield matchfield matchvalue");
      return -1;
    }
    if (!vfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(vfield, sizeof vfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(vfield, sizeof vfield, "%s", vv->sval);
        else
          snprintf(vfield, sizeof vfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "UNIQUWHERE [Class] valfield matchfield matchvalue");
        return -1;
      }
    }
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "UNIQUWHERE [Class] valfield matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(m_sval, sizeof m_sval, "%s", sv->sval);
        m_is_str = 1;
        lex_next(L);
      } else {
        m_nval = parse_expr(vm, L);
        m_is_str = 0;
      }
    } else {
      m_nval = parse_expr(vm, L);
      m_is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int vfi, mfi, hit = 0;
      char field[128];
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      vfi = oop_field_idx(cd, vfield);
      if (mfi < 0 || vfi < 0) { n_skip++; continue; }
      if (ob->fis_str[mfi]) {
        if (m_is_str)
          hit = (strcmp(ob->fstr[mfi], m_sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", m_nval);
          hit = (strcmp(ob->fstr[mfi], nb) == 0);
        }
      } else {
        if (m_is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[mfi]);
          hit = (strcmp(nb, m_sval) == 0);
        } else {
          hit = (ob->fnum[mfi] == m_nval);
        }
      }
      if (!hit) continue;
      if (ob->fis_str[vfi])
        snprintf(field, sizeof field, "%s", ob->fstr[vfi]);
      else
        snprintf(field, sizeof field, "%ld", ob->fnum[vfi]);
      total++;
      for (k = 0; k < nk; k++) {
        if (strcmp(keys[k], field) == 0)
          break;
      }
      if (k == nk && nk < 64) {
        snprintf(keys[nk], sizeof keys[0], "%s", field);
        nk++;
      }
    }
    for (k = 0; k < nk; k++) {
      size_t ln = strlen(keys[k]);
      if (k > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
      if (olen + ln < sizeof out) {
        memcpy(out + olen, keys[k], ln);
        olen += ln;
      } else if (olen < sizeof out - 1) {
        size_t t = sizeof out - 1 - olen;
        memcpy(out + olen, keys[k], t);
        olen += t;
      }
      out[olen] = 0;
    }
    var_set_str(vm, "LAST", out);
    var_set_str(vm, "UNIQUWHERE", out);
    var_set_str(vm, "DISTINCTWHERE", out);
    var_set_str(vm, "UNIQUEFWHERE", out);
    var_set_str(vm, "UNIQWHERE", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = nk;
    var_set_num(vm, "LAST_N", nk);
    var_set_num(vm, "UNIQUWHERE_N", nk);
    var_set_num(vm, "DISTINCTWHERE_N", nk);
    var_set_num(vm, "UNIQUEFWHERE_N", nk);
    var_set_num(vm, "WHEREUNIQ_N", nk);
    var_set_num(vm, "UNIQUWHERE_TOTAL", total);
    var_set_num(vm, "DISTINCTWHERE_TOTAL", total);
    var_set_num(vm, "UNIQUWHERE_SKIP", n_skip);
    var_set_str(vm, "FIELD", vfield);
    var_set_str(vm, "SRC", mfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* WHEREGE|WHEREGT|WHERELE|WHERELT [Class] field value
   * â bag of live object names where numeric field meets threshold.
   * Soft always; string/missing fields skipped. LAST_N = count.
   * Usability: range/threshold select beyond WHEREOBJ equality Â· no EACH+GETF+IF. */
  if (kw(&L->cur, "WHEREGE") || kw(&L->cur, "WHEREGTE") ||
      kw(&L->cur, "ATLEAST") || kw(&L->cur, "MINWHERE") ||
      kw(&L->cur, "WHEREGT") || kw(&L->cur, "ABOVE") ||
      kw(&L->cur, "WHERELE") || kw(&L->cur, "WHERELTE") ||
      kw(&L->cur, "ATMOST") || kw(&L->cur, "MAXWHERE") ||
      kw(&L->cur, "WHERELT") || kw(&L->cur, "BELOW") ||
      kw(&L->cur, "THRESHOBJS") || kw(&L->cur, "FILTERGE") ||
      kw(&L->cur, "FILTERLE")) {
    char filt[48], fname[48], tok1[48], bag[4096], op[24];
    int has_filt = 0, mode = 0, i, n = 0, n_skip = 0;
    long thresh = 0;
    size_t o = 0;
    /* mode: 0=GE 1=GT 2=LE 3=LT */
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "WHEREGT") == 0 || strcmp(op, "ABOVE") == 0)
      mode = 1;
    else if (strcmp(op, "WHERELE") == 0 || strcmp(op, "WHERELTE") == 0 ||
             strcmp(op, "ATMOST") == 0 || strcmp(op, "MAXWHERE") == 0 ||
             strcmp(op, "FILTERLE") == 0)
      mode = 2;
    else if (strcmp(op, "WHERELT") == 0 || strcmp(op, "BELOW") == 0)
      mode = 3;
    else
      mode = 0; /* GE / ATLEAST / THRESHOBJS / FILTERGE */
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "WHEREGE OF Class field value"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
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
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "WHEREGE [Class] field value"); return -1;
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
        fail(vm, "WHEREGE [Class] field value"); return -1;
      }
    }
    if (kw(&L->cur, "GE") || kw(&L->cur, "GTE") || kw(&L->cur, "GT") ||
        kw(&L->cur, "LE") || kw(&L->cur, "LTE") || kw(&L->cur, "LT") ||
        kw(&L->cur, "MIN") || kw(&L->cur, "MAX"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      thresh = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      thresh = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        thresh = atol(sv->sval);
      else
        thresh = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      thresh = parse_expr(vm, L);
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi, hit = 0;
      size_t ln;
      long v;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      v = ob->fnum[fi];
      if (mode == 0) hit = (v >= thresh);
      else if (mode == 1) hit = (v > thresh);
      else if (mode == 2) hit = (v <= thresh);
      else hit = (v < thresh);
      if (!hit) continue;
      ln = strlen(ob->name);
      if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, ob->name, ln);
        o += ln;
      }
      bag[o] = 0;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "WHEREGE", bag);
    var_set_str(vm, "WHERELE", bag);
    var_set_str(vm, "WHEREGT", bag);
    var_set_str(vm, "WHERELT", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "WHEREGE_N", n);
    var_set_num(vm, "WHERELE_N", n);
    var_set_num(vm, "WHEREGT_N", n);
    var_set_num(vm, "WHERELT_N", n);
    var_set_num(vm, "WHEREGE_SKIP", n_skip);
    var_set_num(vm, "WHEREGE_THRESH", thresh);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* WHEREBETWEEN|WHERERANGE|INRANGEOBJS [Class] field lo [TO|..] hi
   * COUNTBETWEEN|COUNTRANGE â count only (no bag).
   * HASBETWEEN|ANYBETWEEN â soft 0|1 + first name (early-exit).
   * Closed interval: lo <= numeric field <= hi (swaps if lo>hi).
   * Usability: band/SLO select without dual WHEREGE+WHERELE glue. */
  if (kw(&L->cur, "WHEREBETWEEN") || kw(&L->cur, "WHERERANGE") ||
      kw(&L->cur, "INRANGEOBJS") || kw(&L->cur, "BETWEENOBJS") ||
      kw(&L->cur, "RANGEOBJS") || kw(&L->cur, "WHERERANGEOF") ||
      kw(&L->cur, "COUNTBETWEEN") || kw(&L->cur, "COUNTBAND") ||
      kw(&L->cur, "COUNTINRANGE") || kw(&L->cur, "NBETWEEN") ||
      kw(&L->cur, "TALLYBETWEEN") || kw(&L->cur, "TALLYRANGE") ||
      kw(&L->cur, "HASBETWEEN") || kw(&L->cur, "ANYBETWEEN") ||
      kw(&L->cur, "INRANGE") || kw(&L->cur, "HASRANGE") ||
      kw(&L->cur, "ANYRANGE")) {
    char filt[48], fname[48], tok1[48], bag[4096], found[48], op[32];
    int has_filt = 0, mode = 0; /* 0=bag 1=count 2=has */
    int i, n = 0, n_skip = 0;
    long lo = 0, hi = 0, t;
    size_t o = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "COUNTBETWEEN") == 0 || strcmp(op, "COUNTBAND") == 0 ||
        strcmp(op, "COUNTINRANGE") == 0 || strcmp(op, "NBETWEEN") == 0 ||
        strcmp(op, "TALLYBETWEEN") == 0 || strcmp(op, "TALLYRANGE") == 0)
      mode = 1;
    else if (strcmp(op, "HASBETWEEN") == 0 || strcmp(op, "ANYBETWEEN") == 0 ||
             strcmp(op, "HASRANGE") == 0 ||
             strcmp(op, "ANYRANGE") == 0)
      /* note: free INRANGE is numeric soft range (996); OOP uses HASBETWEEN */
      mode = 2;
    else
      mode = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    bag[0] = 0;
    found[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "WHEREBETWEEN OF Class field lo hi"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
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
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "WHEREBETWEEN [Class] field lo hi"); return -1;
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
        fail(vm, "WHEREBETWEEN [Class] field lo hi"); return -1;
      }
    }
    /* optional IN | BETWEEN before lo */
    if (kw(&L->cur, "IN") || kw(&L->cur, "BETWEEN") || kw(&L->cur, "RANGE"))
      lex_next(L);
    /* lo */
    if (L->cur.kind == TK_STR) {
      lo = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      lo = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        lo = atol(sv->sval);
      else
        lo = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      lo = parse_expr(vm, L);
    }
    /* optional TO | .. | - | AND | , between lo and hi */
    if (kw(&L->cur, "TO") || kw(&L->cur, "AND") || kw(&L->cur, "THRU") ||
        kw(&L->cur, "THROUGH") || kw(&L->cur, "UNTIL") ||
        (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "..") == 0))
      lex_next(L);
    else if (L->cur.kind == TK_COMMA)
      lex_next(L);
    /* hi */
    if (L->cur.kind == TK_STR) {
      hi = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      hi = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        hi = atol(sv->sval);
      else
        hi = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      hi = parse_expr(vm, L);
    }
    if (lo > hi) { t = lo; lo = hi; hi = t; }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      long v;
      size_t ln;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      v = ob->fnum[fi];
      if (v < lo || v > hi) continue;
      if (mode == 2) {
        /* has: first hit */
        snprintf(found, sizeof found, "%s", ob->name);
        n = 1;
        break;
      }
      if (mode == 0) {
        ln = strlen(ob->name);
        if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + ln < sizeof bag) {
          memcpy(bag + o, ob->name, ln);
          o += ln;
        }
        bag[o] = 0;
      }
      n++;
    }
    if (mode == 2) {
      var_set_str(vm, "LAST", found);
      var_set_str(vm, "HASBETWEEN", found);
      var_set_str(vm, "ANYBETWEEN", found);
      var_set_str(vm, "FIRST", found);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", found);
      vm->last_n = n ? 1 : 0;
      var_set_num(vm, "LAST_N", n ? 1 : 0);
      var_set_num(vm, "HASBETWEEN_N", n ? 1 : 0);
      var_set_num(vm, "ANYBETWEEN_N", n ? 1 : 0);
      var_set_num(vm, "INRANGE_N", n ? 1 : 0);
    } else if (mode == 1) {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "COUNTBETWEEN", n);
      var_set_num(vm, "COUNTBETWEEN_N", n);
      var_set_num(vm, "COUNTBAND_N", n);
      var_set_num(vm, "COUNTINRANGE_N", n);
      var_set_num(vm, "NBETWEEN_N", n);
    } else {
      var_set_str(vm, "LAST", bag);
      var_set_str(vm, "WHEREBETWEEN", bag);
      var_set_str(vm, "WHERERANGE", bag);
      var_set_str(vm, "INRANGEOBJS", bag);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "WHEREBETWEEN_N", n);
      var_set_num(vm, "WHERERANGE_N", n);
      var_set_num(vm, "INRANGEOBJS_N", n);
    }
    var_set_num(vm, "WHEREBETWEEN_SKIP", n_skip);
    var_set_num(vm, "WHEREBETWEEN_LO", lo);
    var_set_num(vm, "WHEREBETWEEN_HI", hi);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* WHEREOBJ|FILTEROBJS|KEEPOBJS [Class] field value
   * FINDOBJ|FIRSTOBJ â first match only (LAST=name, LAST_N 0|1).
   * Soft always. Equality on num or string fields.
   * Usability: select live objects by field without EACH OBJ + GETF + IF glue. */
  if (kw(&L->cur, "WHEREOBJ") || kw(&L->cur, "FILTEROBJS") ||
      kw(&L->cur, "KEEPOBJS") || kw(&L->cur, "SELECTOBJS") ||
      kw(&L->cur, "OBJSWHERE") || kw(&L->cur, "MATCHOBJS") ||
      kw(&L->cur, "FINDOBJ") || kw(&L->cur, "FIRSTOBJ") ||
      kw(&L->cur, "FINDINST") || kw(&L->cur, "OBJFIND") ||
      kw(&L->cur, "FINDWHERE")) {
    char filt[48], fname[48], tok1[48], sval[512], bag[4096], op[24];
    int has_filt = 0, is_str = 0, first_only = 0, i, n = 0, n_skip = 0;
    long nval = 0;
    size_t o = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "FINDOBJ") == 0 || strcmp(op, "FIRSTOBJ") == 0 ||
        strcmp(op, "FINDINST") == 0 || strcmp(op, "OBJFIND") == 0 ||
        strcmp(op, "FINDWHERE") == 0)
      first_only = 1;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    sval[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "WHEREOBJ OF Class field value"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
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
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "WHEREOBJ [Class] field value"); return -1;
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
        fail(vm, "WHEREOBJ [Class] field value"); return -1;
      }
    }
    /* optional == | EQ | IS before value */
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS") ||
        (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "==") == 0)) {
      lex_next(L);
    } else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L); /* == as two EQ tokens if any */
    }
    if (L->cur.kind == TK_STR) {
      snprintf(sval, sizeof sval, "%s", L->cur.text);
      is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(sval, sizeof sval, "%s", vm->last_str);
      is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(sval, sizeof sval, "%s", sv->sval);
        is_str = 1;
        lex_next(L);
      } else {
        nval = parse_expr(vm, L);
        is_str = 0;
      }
    } else {
      nval = parse_expr(vm, L);
      is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi, hit = 0;
      size_t ln;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) {
        if (is_str)
          hit = (strcmp(ob->fstr[fi], sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", nval);
          hit = (strcmp(ob->fstr[fi], nb) == 0);
        }
      } else {
        if (is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[fi]);
          hit = (strcmp(nb, sval) == 0);
        } else {
          hit = (ob->fnum[fi] == nval);
        }
      }
      if (!hit) continue;
      if (first_only) {
        var_set_str(vm, "LAST", ob->name);
        var_set_str(vm, "FINDOBJ", ob->name);
        var_set_str(vm, "WHEREOBJ", ob->name);
        var_set_str(vm, "OBJECT", ob->name);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", ob->name);
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_num(vm, "FINDOBJ_N", 1);
        var_set_num(vm, "WHEREOBJ_N", 1);
        var_set_num(vm, "WHEREOBJ_SKIP", n_skip);
        var_set_str(vm, "FIELD", fname);
        if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
        var_set_num(vm, "OK", 1);
        bump(vm);
        return 1;
      }
      ln = strlen(ob->name);
      if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, ob->name, ln);
        o += ln;
      }
      bag[o] = 0;
      n++;
    }
    if (first_only) {
      /* no match */
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_str(vm, "FINDOBJ", "");
      var_set_str(vm, "WHEREOBJ", "");
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "FINDOBJ_N", 0);
      var_set_num(vm, "WHEREOBJ_N", 0);
      var_set_num(vm, "WHEREOBJ_SKIP", n_skip);
      var_set_str(vm, "FIELD", fname);
      if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
      var_set_num(vm, "OK", 1);
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "WHEREOBJ", bag);
    var_set_str(vm, "FILTEROBJS", bag);
    var_set_str(vm, "KEEPOBJS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "WHEREOBJ_N", n);
    var_set_num(vm, "FILTEROBJS_N", n);
    var_set_num(vm, "KEEPOBJS_N", n);
    var_set_num(vm, "WHEREOBJ_SKIP", n_skip);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* COUNTOBJ|NOBJS|COUNTOBJS [Class]
   * â count live objects (optional class filter). LAST_N = count; LAST = decimal.
   * Usability: fleet size without LISTOBJS bag materialize. */
  if (kw(&L->cur, "COUNTOBJ") || kw(&L->cur, "COUNTOBJS") ||
      kw(&L->cur, "NUMOBJS") || kw(&L->cur, "OBJCOUNT") ||
      kw(&L->cur, "COUNTINST") || kw(&L->cur, "NINST") ||
      kw(&L->cur, "NOBJS")) {
    /* NOBJS as stmt = count-only; LISTOBJS also sets NOBJS after listing. */
    char filt[48];
    int has_filt = 0, i, n = 0;
    lex_next(L);
    filt[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "COUNTOBJ [Class]"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
               !kw(&L->cur, "SYS") && !kw(&L->cur, "PRINT") &&
               !kw(&L->cur, "CUBE") && !kw(&L->cur, "END") &&
               !kw(&L->cur, "NEW") && !kw(&L->cur, "CLASS")) {
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_STR) {
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      n++;
    }
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "COUNTOBJ", n);
    var_set_num(vm, "COUNTOBJ_N", n);
    var_set_num(vm, "NOBJS", n);
    var_set_num(vm, "NUMOBJS", n);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* COUNTWHERE [Class] field value
   * â count live objs with field==value (num or string). Soft; LAST_N=count.
   * Usability: fleet tally without WHEREOBJ bag Â· IF/gates for pool size. */
  if (kw(&L->cur, "COUNTWHERE") || kw(&L->cur, "COUNTMATCH") ||
      kw(&L->cur, "COUNTEQ") || kw(&L->cur, "NWHERE") ||
      kw(&L->cur, "HOWMANY") || kw(&L->cur, "TALLYWHERE")) {
    char filt[48], fname[48], tok1[48], sval[512];
    int has_filt = 0, is_str = 0, i, n = 0, n_skip = 0;
    long nval = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    sval[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "COUNTWHERE OF Class field value"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
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
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "COUNTWHERE [Class] field value"); return -1;
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
        fail(vm, "COUNTWHERE [Class] field value"); return -1;
      }
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(sval, sizeof sval, "%s", L->cur.text);
      is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(sval, sizeof sval, "%s", vm->last_str);
      is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(sval, sizeof sval, "%s", sv->sval);
        is_str = 1;
        lex_next(L);
      } else {
        nval = parse_expr(vm, L);
        is_str = 0;
      }
    } else {
      nval = parse_expr(vm, L);
      is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi, hit = 0;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) {
        if (is_str)
          hit = (strcmp(ob->fstr[fi], sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", nval);
          hit = (strcmp(ob->fstr[fi], nb) == 0);
        }
      } else {
        if (is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[fi]);
          hit = (strcmp(nb, sval) == 0);
        } else {
          hit = (ob->fnum[fi] == nval);
        }
      }
      if (hit) n++;
    }
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "COUNTWHERE", n);
    var_set_num(vm, "COUNTWHERE_N", n);
    var_set_num(vm, "COUNTMATCH_N", n);
    var_set_num(vm, "HOWMANY_N", n);
    var_set_num(vm, "COUNTWHERE_SKIP", n_skip);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* COUNTWHEREGE|COUNTWHERELE|COUNTWHEREGT|COUNTWHERELT [Class] field value
   * COUNTATLEAST|COUNTATMOST|COUNTABOVE|COUNTBELOW aliases.
   * â count live objs whose numeric field meets threshold. Soft; LAST_N=count.
   * Usability: SLO / capacity gates without WHEREGE bag materialize. */
  if (kw(&L->cur, "COUNTWHEREGE") || kw(&L->cur, "COUNTWHEREGTE") ||
      kw(&L->cur, "COUNTATLEAST") || kw(&L->cur, "NGE") ||
      kw(&L->cur, "COUNTWHEREGT") || kw(&L->cur, "COUNTABOVE") ||
      kw(&L->cur, "NGT") ||
      kw(&L->cur, "COUNTWHERELE") || kw(&L->cur, "COUNTWHERELTE") ||
      kw(&L->cur, "COUNTATMOST") || kw(&L->cur, "NLE") ||
      kw(&L->cur, "COUNTWHERELT") || kw(&L->cur, "COUNTBELOW") ||
      kw(&L->cur, "NLT") ||
      kw(&L->cur, "THRESHCOUNT") || kw(&L->cur, "COUNTTHRESH")) {
    char filt[48], fname[48], tok1[48], op[32];
    int has_filt = 0, mode = 0, i, n = 0, n_skip = 0;
    long thresh = 0;
    /* mode: 0=GE 1=GT 2=LE 3=LT */
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "COUNTWHEREGT") == 0 || strcmp(op, "COUNTABOVE") == 0 ||
        strcmp(op, "NGT") == 0)
      mode = 1;
    else if (strcmp(op, "COUNTWHERELE") == 0 ||
             strcmp(op, "COUNTWHERELTE") == 0 ||
             strcmp(op, "COUNTATMOST") == 0 || strcmp(op, "NLE") == 0)
      mode = 2;
    else if (strcmp(op, "COUNTWHERELT") == 0 || strcmp(op, "COUNTBELOW") == 0 ||
             strcmp(op, "NLT") == 0)
      mode = 3;
    else
      mode = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "COUNTWHEREGE OF Class field value"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
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
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "COUNTWHEREGE [Class] field value"); return -1;
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
        fail(vm, "COUNTWHEREGE [Class] field value"); return -1;
      }
    }
    if (kw(&L->cur, "GE") || kw(&L->cur, "GTE") || kw(&L->cur, "GT") ||
        kw(&L->cur, "LE") || kw(&L->cur, "LTE") || kw(&L->cur, "LT") ||
        kw(&L->cur, "MIN") || kw(&L->cur, "MAX"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      thresh = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      thresh = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        thresh = atol(sv->sval);
      else
        thresh = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      thresh = parse_expr(vm, L);
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi, hit = 0;
      long v;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      v = ob->fnum[fi];
      if (mode == 0) hit = (v >= thresh);
      else if (mode == 1) hit = (v > thresh);
      else if (mode == 2) hit = (v <= thresh);
      else hit = (v < thresh);
      if (hit) n++;
    }
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "COUNTWHEREGE", n);
    var_set_num(vm, "COUNTWHEREGE_N", n);
    var_set_num(vm, "COUNTWHERELE_N", n);
    var_set_num(vm, "COUNTWHEREGT_N", n);
    var_set_num(vm, "COUNTWHERELT_N", n);
    var_set_num(vm, "COUNTATLEAST_N", n);
    var_set_num(vm, "COUNTATMOST_N", n);
    var_set_num(vm, "COUNTABOVE_N", n);
    var_set_num(vm, "COUNTBELOW_N", n);
    var_set_num(vm, "COUNTWHEREGE_SKIP", n_skip);
    var_set_num(vm, "COUNTWHEREGE_THRESH", thresh);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASWHERE|ANYWHERE|ANYOBJWHERE [Class] field value
   * â soft 0|1 existence probe: any live obj with field==value?
   * Early-exit on first hit. LAST_N 0|1; optional FIRST name in LAST/HASWHERE.
   * Usability: IF gates without COUNTWHERE compare Â· mirrors HASOBJ/HASFIELD. */
  if (kw(&L->cur, "HASWHERE") || kw(&L->cur, "ANYWHERE") ||
      kw(&L->cur, "ANYOBJWHERE") || kw(&L->cur, "EXISTSWHERE") ||
      kw(&L->cur, "SOMEWHERE") || kw(&L->cur, "HASMATCH") ||
      kw(&L->cur, "ANYMATCH")) {
    char filt[48], fname[48], tok1[48], sval[512], found[48];
    int has_filt = 0, is_str = 0, i, hit_any = 0, n_skip = 0;
    long nval = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    sval[0] = 0;
    found[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "HASWHERE OF Class field value"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
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
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "HASWHERE [Class] field value"); return -1;
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
        fail(vm, "HASWHERE [Class] field value"); return -1;
      }
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(sval, sizeof sval, "%s", L->cur.text);
      is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(sval, sizeof sval, "%s", vm->last_str);
      is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(sval, sizeof sval, "%s", sv->sval);
        is_str = 1;
        lex_next(L);
      } else {
        nval = parse_expr(vm, L);
        is_str = 0;
      }
    } else {
      nval = parse_expr(vm, L);
      is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi, hit = 0;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) {
        if (is_str)
          hit = (strcmp(ob->fstr[fi], sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", nval);
          hit = (strcmp(ob->fstr[fi], nb) == 0);
        }
      } else {
        if (is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[fi]);
          hit = (strcmp(nb, sval) == 0);
        } else {
          hit = (ob->fnum[fi] == nval);
        }
      }
      if (hit) {
        hit_any = 1;
        snprintf(found, sizeof found, "%s", ob->name);
        break;
      }
    }
    var_set_str(vm, "LAST", found);
    var_set_str(vm, "HASWHERE", found);
    var_set_str(vm, "ANYWHERE", found);
    var_set_str(vm, "FIRST", found);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", found);
    vm->last_n = hit_any ? 1 : 0;
    var_set_num(vm, "LAST_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASWHERE_N", hit_any ? 1 : 0);
    var_set_num(vm, "ANYWHERE_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASMATCH_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASWHERE_SKIP", n_skip);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASWHEREGE|HASWHERELE|HASWHEREGT|HASWHERELT [Class] field value
   * ANYATLEAST|ANYABOVE|ANYBELOW|HASATLEAST aliases.
   * â soft 0|1: any live obj whose numeric field meets threshold?
   * Early-exit; LAST = first matching name. Completes WHERE/COUNT/HAS triad. */
  if (kw(&L->cur, "HASWHEREGE") || kw(&L->cur, "HASWHEREGTE") ||
      kw(&L->cur, "HASATLEAST") || kw(&L->cur, "ANYATLEAST") ||
      kw(&L->cur, "ANYWHEREGE") ||
      kw(&L->cur, "HASWHEREGT") || kw(&L->cur, "HASABOVE") ||
      kw(&L->cur, "ANYABOVE") || kw(&L->cur, "ANYWHEREGT") ||
      kw(&L->cur, "HASWHERELE") || kw(&L->cur, "HASWHERELTE") ||
      kw(&L->cur, "HASATMOST") || kw(&L->cur, "ANYATMOST") ||
      kw(&L->cur, "ANYWHERELE") ||
      kw(&L->cur, "HASWHERELT") || kw(&L->cur, "HASBELOW") ||
      kw(&L->cur, "ANYBELOW") || kw(&L->cur, "ANYWHERELT") ||
      kw(&L->cur, "THRESHHAS") || kw(&L->cur, "HASTHRESH")) {
    char filt[48], fname[48], tok1[48], found[48], op[32];
    int has_filt = 0, mode = 0, i, hit_any = 0, n_skip = 0;
    long thresh = 0;
    /* mode: 0=GE 1=GT 2=LE 3=LT */
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "HASWHEREGT") == 0 || strcmp(op, "HASABOVE") == 0 ||
        strcmp(op, "ANYABOVE") == 0 || strcmp(op, "ANYWHEREGT") == 0)
      mode = 1;
    else if (strcmp(op, "HASWHERELE") == 0 || strcmp(op, "HASWHERELTE") == 0 ||
             strcmp(op, "HASATMOST") == 0 || strcmp(op, "ANYATMOST") == 0 ||
             strcmp(op, "ANYWHERELE") == 0)
      mode = 2;
    else if (strcmp(op, "HASWHERELT") == 0 || strcmp(op, "HASBELOW") == 0 ||
             strcmp(op, "ANYBELOW") == 0 || strcmp(op, "ANYWHERELT") == 0)
      mode = 3;
    else
      mode = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    found[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "HASWHEREGE OF Class field value"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
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
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "HASWHEREGE [Class] field value"); return -1;
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
        fail(vm, "HASWHEREGE [Class] field value"); return -1;
      }
    }
    if (kw(&L->cur, "GE") || kw(&L->cur, "GTE") || kw(&L->cur, "GT") ||
        kw(&L->cur, "LE") || kw(&L->cur, "LTE") || kw(&L->cur, "LT") ||
        kw(&L->cur, "MIN") || kw(&L->cur, "MAX"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      thresh = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      thresh = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        thresh = atol(sv->sval);
      else
        thresh = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      thresh = parse_expr(vm, L);
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi, hit = 0;
      long v;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      v = ob->fnum[fi];
      if (mode == 0) hit = (v >= thresh);
      else if (mode == 1) hit = (v > thresh);
      else if (mode == 2) hit = (v <= thresh);
      else hit = (v < thresh);
      if (hit) {
        hit_any = 1;
        snprintf(found, sizeof found, "%s", ob->name);
        break;
      }
    }
    var_set_str(vm, "LAST", found);
    var_set_str(vm, "HASWHEREGE", found);
    var_set_str(vm, "HASWHERELE", found);
    var_set_str(vm, "HASATLEAST", found);
    var_set_str(vm, "FIRST", found);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", found);
    vm->last_n = hit_any ? 1 : 0;
    var_set_num(vm, "LAST_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASWHEREGE_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASWHERELE_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASWHEREGT_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASWHERELT_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASATLEAST_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASATMOST_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASABOVE_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASBELOW_N", hit_any ? 1 : 0);
    var_set_num(vm, "ANYATLEAST_N", hit_any ? 1 : 0);
    var_set_num(vm, "ANYBELOW_N", hit_any ? 1 : 0);
    var_set_num(vm, "HASWHEREGE_SKIP", n_skip);
    var_set_num(vm, "HASWHEREGE_THRESH", thresh);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* DELETEWHERE|FREEWHERE|PURGEWHERE [Class] field value
   * â free every live object whose field equals value (optional class).
   * Soft always; LAST_N/DELETEWHERE_N = count freed. Optional bag of names freed.
   * Usability: WHEREOBJ + DELETEALL one-shot Â· pool GC without EACH+GETF+IF+DELETEOBJ. */
  if (kw(&L->cur, "DELETEWHERE") || kw(&L->cur, "FREEWHERE") ||
      kw(&L->cur, "PURGEWHERE") || kw(&L->cur, "DROPWHERE") ||
      kw(&L->cur, "KILLWHERE") || kw(&L->cur, "DELOBJWHERE") ||
      kw(&L->cur, "REMOVEWHERE") || kw(&L->cur, "CLEARWHERE")) {
    char filt[48], fname[48], tok1[48], sval[512], bag[4096];
    int has_filt = 0, is_str = 0, i, n = 0, n_skip = 0;
    long nval = 0;
    size_t o = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    sval[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "DELETEWHERE OF Class field value"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
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
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "DELETEWHERE [Class] field value"); return -1;
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
        fail(vm, "DELETEWHERE [Class] field value"); return -1;
      }
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(sval, sizeof sval, "%s", L->cur.text);
      is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(sval, sizeof sval, "%s", vm->last_str);
      is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(sval, sizeof sval, "%s", sv->sval);
        is_str = 1;
        lex_next(L);
      } else {
        nval = parse_expr(vm, L);
        is_str = 0;
      }
    } else {
      nval = parse_expr(vm, L);
      is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi, hit = 0;
      size_t ln;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) {
        if (is_str)
          hit = (strcmp(ob->fstr[fi], sval) == 0);
        else {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", nval);
          hit = (strcmp(ob->fstr[fi], nb) == 0);
        }
      } else {
        if (is_str) {
          char nb[32];
          snprintf(nb, sizeof nb, "%ld", ob->fnum[fi]);
          hit = (strcmp(nb, sval) == 0);
        } else {
          hit = (ob->fnum[fi] == nval);
        }
      }
      if (!hit) continue;
      ln = strlen(ob->name);
      if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, ob->name, ln);
        o += ln;
      }
      bag[o] = 0;
      ob->live = 0;
      if (strcmp(vm->this_obj, ob->name) == 0)
        vm->this_obj[0] = 0;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "DELETEWHERE", bag);
    var_set_str(vm, "FREEWHERE", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "DELETEWHERE_N", n);
    var_set_num(vm, "FREEWHERE_N", n);
    var_set_num(vm, "PURGEWHERE_N", n);
    var_set_num(vm, "DELETEWHERE_SKIP", n_skip);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* DELETEWHEREGE|DELETEWHERELE|DELETEWHEREGT|DELETEWHERELT [Class] field value
   * FREEWHEREGE|FREEWHERELE|â¦|PURGEBELOW|FREEABOVE aliases.
   * â free every live object whose numeric field meets threshold.
   * Soft always; string/missing fields skipped. LAST_N = count freed.
   * Usability: WHEREGE + DELETEWHERE one-shot Â· pool GC by range without
   * EACH+GETF+IF+DELETEOBJ (dead cells energy<=0, expired retries>=max). */
  if (kw(&L->cur, "DELETEWHEREGE") || kw(&L->cur, "DELETEWHEREGTE") ||
      kw(&L->cur, "FREEWHEREGE") || kw(&L->cur, "PURGEWHEREGE") ||
      kw(&L->cur, "KILLWHEREGE") || kw(&L->cur, "DROPWHEREGE") ||
      kw(&L->cur, "FREEATLEAST") || kw(&L->cur, "PURGEATLEAST") ||
      kw(&L->cur, "DELETEWHEREGT") || kw(&L->cur, "FREEWHEREGT") ||
      kw(&L->cur, "PURGEWHEREGT") || kw(&L->cur, "FREEABOVE") ||
      kw(&L->cur, "PURGEABOVE") || kw(&L->cur, "KILLABOVE") ||
      kw(&L->cur, "DELETEWHERELE") || kw(&L->cur, "DELETEWHERELTE") ||
      kw(&L->cur, "FREEWHERELE") || kw(&L->cur, "PURGEWHERELE") ||
      kw(&L->cur, "KILLWHERELE") || kw(&L->cur, "DROPWHERELE") ||
      kw(&L->cur, "FREEATMOST") || kw(&L->cur, "PURGEATMOST") ||
      kw(&L->cur, "DELETEWHERELT") || kw(&L->cur, "FREEWHERELT") ||
      kw(&L->cur, "PURGEWHERELT") || kw(&L->cur, "FREEBELOW") ||
      kw(&L->cur, "PURGEBELOW") || kw(&L->cur, "KILLBELOW") ||
      kw(&L->cur, "THRESHFREE") || kw(&L->cur, "FREETHRESH")) {
    char filt[48], fname[48], tok1[48], bag[4096], op[32];
    int has_filt = 0, mode = 0, i, n = 0, n_skip = 0;
    long thresh = 0;
    size_t o = 0;
    /* mode: 0=GE 1=GT 2=LE 3=LT */
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "DELETEWHEREGT") == 0 || strcmp(op, "FREEWHEREGT") == 0 ||
        strcmp(op, "PURGEWHEREGT") == 0 || strcmp(op, "FREEABOVE") == 0 ||
        strcmp(op, "PURGEABOVE") == 0 || strcmp(op, "KILLABOVE") == 0)
      mode = 1;
    else if (strcmp(op, "DELETEWHERELE") == 0 ||
             strcmp(op, "DELETEWHERELTE") == 0 ||
             strcmp(op, "FREEWHERELE") == 0 || strcmp(op, "PURGEWHERELE") == 0 ||
             strcmp(op, "KILLWHERELE") == 0 || strcmp(op, "DROPWHERELE") == 0 ||
             strcmp(op, "FREEATMOST") == 0 || strcmp(op, "PURGEATMOST") == 0)
      mode = 2;
    else if (strcmp(op, "DELETEWHERELT") == 0 || strcmp(op, "FREEWHERELT") == 0 ||
             strcmp(op, "PURGEWHERELT") == 0 || strcmp(op, "FREEBELOW") == 0 ||
             strcmp(op, "PURGEBELOW") == 0 || strcmp(op, "KILLBELOW") == 0)
      mode = 3;
    else
      mode = 0; /* GE / ATLEAST / THRESHFREE */
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "DELETEWHEREGE OF Class field value"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
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
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "DELETEWHEREGE [Class] field value"); return -1;
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
        fail(vm, "DELETEWHEREGE [Class] field value"); return -1;
      }
    }
    if (kw(&L->cur, "GE") || kw(&L->cur, "GTE") || kw(&L->cur, "GT") ||
        kw(&L->cur, "LE") || kw(&L->cur, "LTE") || kw(&L->cur, "LT") ||
        kw(&L->cur, "MIN") || kw(&L->cur, "MAX"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      thresh = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      thresh = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        thresh = atol(sv->sval);
      else
        thresh = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      thresh = parse_expr(vm, L);
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi, hit = 0;
      size_t ln;
      long v;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      v = ob->fnum[fi];
      if (mode == 0) hit = (v >= thresh);
      else if (mode == 1) hit = (v > thresh);
      else if (mode == 2) hit = (v <= thresh);
      else hit = (v < thresh);
      if (!hit) continue;
      ln = strlen(ob->name);
      if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, ob->name, ln);
        o += ln;
      }
      bag[o] = 0;
      ob->live = 0;
      if (strcmp(vm->this_obj, ob->name) == 0)
        vm->this_obj[0] = 0;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "DELETEWHEREGE", bag);
    var_set_str(vm, "DELETEWHERELE", bag);
    var_set_str(vm, "DELETEWHEREGT", bag);
    var_set_str(vm, "DELETEWHERELT", bag);
    var_set_str(vm, "FREEWHEREGE", bag);
    var_set_str(vm, "FREEWHERELE", bag);
    var_set_str(vm, "FREEBELOW", bag);
    var_set_str(vm, "FREEABOVE", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "DELETEWHEREGE_N", n);
    var_set_num(vm, "DELETEWHERELE_N", n);
    var_set_num(vm, "DELETEWHEREGT_N", n);
    var_set_num(vm, "DELETEWHERELT_N", n);
    var_set_num(vm, "FREEWHEREGE_N", n);
    var_set_num(vm, "FREEWHERELE_N", n);
    var_set_num(vm, "FREEBELOW_N", n);
    var_set_num(vm, "FREEABOVE_N", n);
    var_set_num(vm, "PURGEBELOW_N", n);
    var_set_num(vm, "PURGEABOVE_N", n);
    var_set_num(vm, "PURGEWHEREGE_N", n);
    var_set_num(vm, "PURGEWHERELE_N", n);
    var_set_num(vm, "DELETEWHEREGE_SKIP", n_skip);
    var_set_num(vm, "DELETEWHEREGE_THRESH", thresh);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* DELETEBETWEEN|FREEBETWEEN|PURGEBETWEEN|FREEBAND [Class] field lo [TO] hi
   * â free live objs with lo <= numeric field <= hi (auto-swap lo/hi).
   * Soft always; LAST = bag of freed names; LAST_N = count.
   * Usability: WHEREBETWEEN + DELETEWHERE one-shot Â· band GC without
   * dual DELETEWHEREGE+DELETEWHERELE or EACH+GETF+IF+DELETEOBJ. */
  if (kw(&L->cur, "DELETEBETWEEN") || kw(&L->cur, "FREEBETWEEN") ||
      kw(&L->cur, "PURGEBETWEEN") || kw(&L->cur, "DROPBETWEEN") ||
      kw(&L->cur, "KILLBETWEEN") || kw(&L->cur, "FREEBAND") ||
      kw(&L->cur, "PURGEBAND") || kw(&L->cur, "DELETEBAND") ||
      kw(&L->cur, "FREERANGEOBJS") || kw(&L->cur, "PURGERANGEOBJS")) {
    char filt[48], fname[48], tok1[48], bag[4096];
    int has_filt = 0, i, n = 0, n_skip = 0;
    long lo = 0, hi = 0, t;
    size_t o = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "DELETEBETWEEN OF Class field lo hi"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1)) {
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
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "DELETEBETWEEN [Class] field lo hi"); return -1;
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
        fail(vm, "DELETEBETWEEN [Class] field lo hi"); return -1;
      }
    }
    if (kw(&L->cur, "IN") || kw(&L->cur, "BETWEEN") || kw(&L->cur, "RANGE"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      lo = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      lo = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        lo = atol(sv->sval);
      else
        lo = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      lo = parse_expr(vm, L);
    }
    if (kw(&L->cur, "TO") || kw(&L->cur, "AND") || kw(&L->cur, "THRU") ||
        kw(&L->cur, "THROUGH") || kw(&L->cur, "UNTIL") ||
        (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "..") == 0))
      lex_next(L);
    else if (L->cur.kind == TK_COMMA)
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      hi = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      hi = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        hi = atol(sv->sval);
      else
        hi = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else {
      hi = parse_expr(vm, L);
    }
    if (lo > hi) { t = lo; lo = hi; hi = t; }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      long v;
      size_t ln;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      v = ob->fnum[fi];
      if (v < lo || v > hi) continue;
      ln = strlen(ob->name);
      if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, ob->name, ln);
        o += ln;
      }
      bag[o] = 0;
      ob->live = 0;
      if (strcmp(vm->this_obj, ob->name) == 0)
        vm->this_obj[0] = 0;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "DELETEBETWEEN", bag);
    var_set_str(vm, "FREEBETWEEN", bag);
    var_set_str(vm, "PURGEBETWEEN", bag);
    var_set_str(vm, "FREEBAND", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "DELETEBETWEEN_N", n);
    var_set_num(vm, "FREEBETWEEN_N", n);
    var_set_num(vm, "PURGEBETWEEN_N", n);
    var_set_num(vm, "FREEBAND_N", n);
    var_set_num(vm, "DELETEBAND_N", n);
    var_set_num(vm, "DELETEBETWEEN_SKIP", n_skip);
    var_set_num(vm, "DELETEBETWEEN_LO", lo);
    var_set_num(vm, "DELETEBETWEEN_HI", hi);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  return 0;
}
