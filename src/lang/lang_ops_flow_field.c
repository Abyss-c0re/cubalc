/* CubalC lang — lang_ops_flow_field.c
 * GETFALL · SETFALL · WHERE field bulk ops (A3 peel from flow_class).
 */
#include "lang/cubalc_lang_ops_flow_field.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_field(VM *vm, Lex *L) {
  /* GETFALL|COLLECTF|MAPGETF [Class] field [AS KV|WITH NAMES]
   * â collect field values from every live object (optional class filter).
   * Soft always; objects missing the field are skipped (GETFALL_SKIP).
   * LAST = newline bag of values (or name:value with AS KV); LAST_N/GETFALL_N = count.
   * Usability: fleet field harvest without EACH OBJ + GETF + PUSH glue (SUM/AVG/FREQ ready). */
  if (kw(&L->cur, "GETFALL") || kw(&L->cur, "COLLECTF") ||
      kw(&L->cur, "MAPGETF") || kw(&L->cur, "GETF_ALL") ||
      kw(&L->cur, "FIELDALL") || kw(&L->cur, "COLLECTFIELD") ||
      kw(&L->cur, "PLUCKALL") || kw(&L->cur, "MAPFIELD")) {
    char filt[48], fname[48], tok1[48], bag[4096];
    int has_filt = 0, as_kv = 0, i, n = 0, n_skip = 0;
    size_t o = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    bag[0] = 0;
    /* OF|CLASS|TYPE Class field */
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "GETFALL OF Class field"); return -1;
      }
      if (L->cur.kind == TK_STR)
        snprintf(filt, sizeof filt, "%s", L->cur.text);
      else
        snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      /* Class field when tok1 names a known CLASS and next is field */
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "AS") && !kw(&L->cur, "WITH") && !kw(&L->cur, "KV") &&
          !kw(&L->cur, "NAMES") && !kw(&L->cur, "ASNAMES")) {
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
        /* bare field (or class-less); resolve string-var */
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] && L->cur.kind != TK_STR &&
            !(L->cur.kind == TK_IDENT && oop_find_class(vm, tok1)))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else if (tok1[0] == 0) {
          fail(vm, "GETFALL [Class] field"); return -1;
        } else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "GETFALL [Class] field"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT &&
                 !kw(&L->cur, "AS") && !kw(&L->cur, "WITH") &&
                 !kw(&L->cur, "KV") && !kw(&L->cur, "NAMES")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "GETFALL [Class] field"); return -1;
      }
    }
    /* optional AS KV | WITH NAMES | KV â name:value bag for LOOKUP */
    if (kw(&L->cur, "AS") || kw(&L->cur, "WITH")) {
      lex_next(L);
      if (kw(&L->cur, "KV") || kw(&L->cur, "NAMES") || kw(&L->cur, "NAME") ||
          kw(&L->cur, "PAIRS") || kw(&L->cur, "MAP") || kw(&L->cur, "OBJECTS")) {
        as_kv = 1;
        lex_next(L);
      }
    } else if (kw(&L->cur, "KV") || kw(&L->cur, "NAMES") ||
               kw(&L->cur, "ASNAMES") || kw(&L->cur, "WITHNAMES") ||
               kw(&L->cur, "PAIRS")) {
      as_kv = 1;
      lex_next(L);
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      char line[256];
      size_t ln;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (as_kv) {
        if (ob->fis_str[fi])
          snprintf(line, sizeof line, "%s:%s", ob->name, ob->fstr[fi]);
        else
          snprintf(line, sizeof line, "%s:%ld", ob->name, ob->fnum[fi]);
      } else {
        if (ob->fis_str[fi])
          snprintf(line, sizeof line, "%s", ob->fstr[fi]);
        else
          snprintf(line, sizeof line, "%ld", ob->fnum[fi]);
      }
      ln = strlen(line);
      if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, line, ln);
        o += ln;
      }
      bag[o] = 0;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "GETFALL", bag);
    var_set_str(vm, "COLLECTF", bag);
    var_set_str(vm, "MAPGETF", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "GETFALL_N", n);
    var_set_num(vm, "COLLECTF_N", n);
    var_set_num(vm, "MAPGETF_N", n);
    var_set_num(vm, "GETFALL_SKIP", n_skip);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0])
      var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* GETFWHERE|COLLECTWHERE|PLUCKWHERE|MAPGETWHERE [Class] getfield matchfield matchvalue
   * â bag of getfield values from live objs where matchfield == matchvalue.
   * Optional AS KV â name:value. Soft skip missing fields.
   * LAST_N = count. WHERE sugar: GETFWHERE Cell energy WHERE ready 1.
   * Usability: filtered GETFALL without WHEREOBJ bag + EACH+GETF. */
  if (kw(&L->cur, "GETFWHERE") || kw(&L->cur, "COLLECTWHERE") ||
      kw(&L->cur, "PLUCKWHERE") || kw(&L->cur, "MAPGETWHERE") ||
      kw(&L->cur, "GETWHERE") || kw(&L->cur, "WHEREGETF") ||
      kw(&L->cur, "HARVESTWHERE") || kw(&L->cur, "FIELDWHERE") ||
      kw(&L->cur, "COLLECTFWHERE") || kw(&L->cur, "GETFALLWHERE")) {
    char filt[48], gfield[48], mfield[48], tok1[48];
    char m_sval[512], bag[4096];
    int has_filt = 0, m_is_str = 0, as_kv = 0, i, n = 0, n_skip = 0;
    long m_nval = 0;
    size_t o = 0;
    lex_next(L);
    filt[0] = 0;
    gfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "GETFWHERE OF Class getfield matchfield matchvalue");
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
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") &&
          !kw(&L->cur, "AS") && !kw(&L->cur, "WITH") && !kw(&L->cur, "KV")) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(gfield, sizeof gfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(gfield, sizeof gfield, "%s", vv->sval);
          else
            snprintf(gfield, sizeof gfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
             kw(&L->cur, "WHERE")))
          snprintf(gfield, sizeof gfield, "%s", vv->sval);
        else
          snprintf(gfield, sizeof gfield, "%s", tok1);
      }
    } else {
      fail(vm, "GETFWHERE [Class] getfield matchfield matchvalue");
      return -1;
    }
    if (!gfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(gfield, sizeof gfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "AS")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(gfield, sizeof gfield, "%s", vv->sval);
        else
          snprintf(gfield, sizeof gfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "GETFWHERE [Class] getfield matchfield matchvalue");
        return -1;
      }
    }
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    /* match field */
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "AS") && !kw(&L->cur, "KV")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "GETFWHERE [Class] getfield matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS") ||
        kw(&L->cur, "=="))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    /* match value */
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "AS") && !kw(&L->cur, "WITH") && !kw(&L->cur, "KV") &&
               !kw(&L->cur, "NAMES")) {
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
    /* optional AS KV after match */
    if (kw(&L->cur, "AS") || kw(&L->cur, "WITH")) {
      lex_next(L);
      if (kw(&L->cur, "KV") || kw(&L->cur, "NAMES") || kw(&L->cur, "NAME") ||
          kw(&L->cur, "PAIRS") || kw(&L->cur, "MAP") || kw(&L->cur, "OBJECTS")) {
        as_kv = 1;
        lex_next(L);
      }
    } else if (kw(&L->cur, "KV") || kw(&L->cur, "NAMES") ||
               kw(&L->cur, "ASNAMES") || kw(&L->cur, "PAIRS")) {
      as_kv = 1;
      lex_next(L);
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int gfi, mfi, hit = 0;
      char line[256];
      size_t ln;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      gfi = oop_field_idx(cd, gfield);
      if (mfi < 0 || gfi < 0) { n_skip++; continue; }
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
      if (as_kv) {
        if (ob->fis_str[gfi])
          snprintf(line, sizeof line, "%s:%s", ob->name, ob->fstr[gfi]);
        else
          snprintf(line, sizeof line, "%s:%ld", ob->name, ob->fnum[gfi]);
      } else {
        if (ob->fis_str[gfi])
          snprintf(line, sizeof line, "%s", ob->fstr[gfi]);
        else
          snprintf(line, sizeof line, "%ld", ob->fnum[gfi]);
      }
      ln = strlen(line);
      if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, line, ln);
        o += ln;
      }
      bag[o] = 0;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "GETFWHERE", bag);
    var_set_str(vm, "COLLECTWHERE", bag);
    var_set_str(vm, "GETWHERE", bag);
    var_set_str(vm, "PLUCKWHERE", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "GETFWHERE_N", n);
    var_set_num(vm, "COLLECTWHERE_N", n);
    var_set_num(vm, "GETWHERE_N", n);
    var_set_num(vm, "PLUCKWHERE_N", n);
    var_set_num(vm, "GETFWHERE_SKIP", n_skip);
    var_set_str(vm, "FIELD", gfield);
    var_set_str(vm, "SRC", mfield);
    var_set_str(vm, "DST", gfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SETFALL|MAPSETF|PUTFALL [Class] field value
   * â write same field value on every live object (optional class filter).
   * Soft always; objects missing the field are skipped (SETFALL_SKIP).
   * LAST_N/SETFALL_N = write count. Value: string | LAST | string-var | expr.
   * Usability: fleet field stamp without EACH OBJ + SETF glue (dual of GETFALL). */
  if (kw(&L->cur, "SETFALL") || kw(&L->cur, "MAPSETF") ||
      kw(&L->cur, "PUTFALL") || kw(&L->cur, "SETF_ALL") ||
      kw(&L->cur, "WRITEFALL") || kw(&L->cur, "FIELDSETALL") ||
      kw(&L->cur, "BULKSETF") || kw(&L->cur, "SETFIELDALL")) {
    char filt[48], fname[48], tok1[48], sval[512];
    int has_filt = 0, is_str = 0, i, n = 0, n_skip = 0;
    long nval = 0;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    sval[0] = 0;
    /* OF|CLASS|TYPE Class field value */
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "SETFALL OF Class field value"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      /* Class field value when tok1 is known CLASS and next looks like field */
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
      fail(vm, "SETFALL [Class] field value"); return -1;
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
        fail(vm, "SETFALL [Class] field value"); return -1;
      }
    }
    if (L->cur.kind == TK_EQ) lex_next(L);
    /* parse value once, apply to all matching objs */
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
      int fi;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (is_str) {
        snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", sval);
        ob->fis_str[fi] = 1;
      } else {
        ob->fnum[fi] = nval;
        ob->fis_str[fi] = 0;
      }
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "SETFALL_N", n);
    var_set_num(vm, "MAPSETF_N", n);
    var_set_num(vm, "PUTFALL_N", n);
    var_set_num(vm, "SETFALL_SKIP", n_skip);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0])
      var_set_str(vm, "CLASS", filt);
    if (is_str) {
      var_set_str(vm, "LAST", sval);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", sval);
    } else {
      char nb[24];
      snprintf(nb, sizeof nb, "%ld", nval);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SETFWHERE|WHERESETF|PUTWHERE|MAPSETWHERE [Class] matchfield matchvalue setfield setvalue
   * â write setfield=setvalue on live objs where matchfield == matchvalue.
   * Optional SET/TO before setfield. Soft skip missing fields.
   * LAST_N = update count. Usability: WHEREOBJ + SETF without EACH+GETF+IF.
   * Same-field rewrite: SETFWHERE Ticket status "open" status "done". */
  if (kw(&L->cur, "SETFWHERE") || kw(&L->cur, "WHERESETF") ||
      kw(&L->cur, "PUTWHERE") || kw(&L->cur, "MAPSETWHERE") ||
      kw(&L->cur, "SETWHERE") || kw(&L->cur, "ASSIGNWHERE") ||
      kw(&L->cur, "UPDATEWHERE") || kw(&L->cur, "WRITEWHERE") ||
      kw(&L->cur, "SETF_WHERE") || kw(&L->cur, "WHERESET")) {
    char filt[48], mfield[48], sfield[48], tok1[48];
    char m_sval[512], s_sval[512];
    int has_filt = 0, m_is_str = 0, s_is_str = 0, i, n = 0, n_skip = 0;
    long m_nval = 0, s_nval = 0;
    lex_next(L);
    filt[0] = 0;
    mfield[0] = 0;
    sfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    s_sval[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "SETFWHERE OF Class matchfield matchvalue setfield setvalue");
        return -1;
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
          snprintf(mfield, sizeof mfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(mfield, sizeof mfield, "%s", vv->sval);
          else
            snprintf(mfield, sizeof mfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(mfield, sizeof mfield, "%s", vv->sval);
        else
          snprintf(mfield, sizeof mfield, "%s", tok1);
      }
    } else {
      fail(vm, "SETFWHERE [Class] matchfield matchvalue setfield setvalue");
      return -1;
    }
    if (!mfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(mfield, sizeof mfield, "%s", vv->sval);
        else
          snprintf(mfield, sizeof mfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "SETFWHERE [Class] matchfield matchvalue setfield setvalue");
        return -1;
      }
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    /* match value */
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "SET") && !kw(&L->cur, "TO") &&
               !kw(&L->cur, "PUT") && !kw(&L->cur, "AS")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      /* If next looks like setfield, this bare ident is match value only when
       * it's a known string var; else try parse_expr for numbers. If next is
       * SET/TO keyword, same. Heuristic: if pure string var use it; else num. */
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
    if (kw(&L->cur, "SET") || kw(&L->cur, "TO") || kw(&L->cur, "PUT") ||
        kw(&L->cur, "AS") || kw(&L->cur, "WRITE"))
      lex_next(L);
    /* setfield */
    if (L->cur.kind == TK_STR) {
      snprintf(sfield, sizeof sfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(sfield, sizeof sfield, "%s", vv->sval);
      else
        snprintf(sfield, sizeof sfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "SETFWHERE [Class] matchfield matchvalue setfield setvalue");
      return -1;
    }
    if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    /* set value */
    if (L->cur.kind == TK_STR) {
      snprintf(s_sval, sizeof s_sval, "%s", L->cur.text);
      s_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(s_sval, sizeof s_sval, "%s", vm->last_str);
      s_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(s_sval, sizeof s_sval, "%s", sv->sval);
        s_is_str = 1;
        lex_next(L);
      } else {
        s_nval = parse_expr(vm, L);
        s_is_str = 0;
      }
    } else {
      s_nval = parse_expr(vm, L);
      s_is_str = 0;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int mfi, sfi, hit = 0;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      sfi = oop_field_idx(cd, sfield);
      if (mfi < 0 || sfi < 0) { n_skip++; continue; }
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
      if (s_is_str) {
        snprintf(ob->fstr[sfi], sizeof ob->fstr[sfi], "%s", s_sval);
        ob->fis_str[sfi] = 1;
      } else {
        ob->fnum[sfi] = s_nval;
        ob->fis_str[sfi] = 0;
      }
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "SETFWHERE_N", n);
    var_set_num(vm, "WHERESETF_N", n);
    var_set_num(vm, "PUTWHERE_N", n);
    var_set_num(vm, "SETFWHERE_SKIP", n_skip);
    var_set_str(vm, "FIELD", mfield);
    var_set_str(vm, "SRC", mfield);
    var_set_str(vm, "DST", sfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* INCFWHERE|ADDWHERE|BUMPWHERE|DECFWHERE [Class] matchfield matchvalue targetfield [delta]
   * â add delta (default +1; DECFWHERE default â1) to numeric targetfield on objs
   * where matchfield == matchvalue. Soft skip missing/string target.
   * BY sugar before delta. LAST_N = update count.
   * Usability: selective counter/retry bump without EACH+GETF+IF+arith+SETF. */
  if (kw(&L->cur, "INCFWHERE") || kw(&L->cur, "ADDWHERE") ||
      kw(&L->cur, "BUMPWHERE") || kw(&L->cur, "DECFWHERE") ||
      kw(&L->cur, "SUBWHERE") || kw(&L->cur, "INCALLWHERE") ||
      kw(&L->cur, "ADDFIELDWHERE") || kw(&L->cur, "WHEREINC") ||
      kw(&L->cur, "WHEREADD") || kw(&L->cur, "WHEREBUMP") ||
      kw(&L->cur, "WHEREDEC") || kw(&L->cur, "DECWHERE")) {
    char filt[48], mfield[48], tfield[48], tok1[48], op[24];
    char m_sval[512];
    int has_filt = 0, m_is_str = 0, i, n = 0, n_skip = 0, dec_mode = 0;
    long m_nval = 0, delta = 1;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "DECFWHERE") == 0 || strcmp(op, "SUBWHERE") == 0 ||
        strcmp(op, "WHEREDEC") == 0 || strcmp(op, "DECWHERE") == 0) {
      dec_mode = 1;
      delta = -1;
    }
    lex_next(L);
    filt[0] = 0;
    mfield[0] = 0;
    tfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "INCFWHERE OF Class matchfield matchvalue target [delta]");
        return -1;
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
          snprintf(mfield, sizeof mfield, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(mfield, sizeof mfield, "%s", vv->sval);
          else
            snprintf(mfield, sizeof mfield, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(mfield, sizeof mfield, "%s", vv->sval);
        else
          snprintf(mfield, sizeof mfield, "%s", tok1);
      }
    } else {
      fail(vm, "INCFWHERE [Class] matchfield matchvalue target [delta]");
      return -1;
    }
    if (!mfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(mfield, sizeof mfield, "%s", vv->sval);
        else
          snprintf(mfield, sizeof mfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "INCFWHERE [Class] matchfield matchvalue target [delta]");
        return -1;
      }
    }
    if (kw(&L->cur, "EQ") || kw(&L->cur, "IS") || kw(&L->cur, "EQUALS"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      if (L->cur.kind == TK_EQ) lex_next(L);
    }
    /* match value */
    if (L->cur.kind == TK_STR) {
      snprintf(m_sval, sizeof m_sval, "%s", L->cur.text);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      snprintf(m_sval, sizeof m_sval, "%s", vm->last_str);
      m_is_str = 1;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "BY") && !kw(&L->cur, "SET") &&
               !kw(&L->cur, "ADD") && !kw(&L->cur, "INC")) {
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
    if (kw(&L->cur, "SET") || kw(&L->cur, "ADD") || kw(&L->cur, "INC") ||
        kw(&L->cur, "ON") || kw(&L->cur, "FIELD"))
      lex_next(L);
    /* target field */
    if (L->cur.kind == TK_STR) {
      snprintf(tfield, sizeof tfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "BY")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(tfield, sizeof tfield, "%s", vv->sval);
      else
        snprintf(tfield, sizeof tfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "INCFWHERE [Class] matchfield matchvalue target [delta]");
      return -1;
    }
    if (kw(&L->cur, "BY") || kw(&L->cur, "DELTA") || kw(&L->cur, "PLUS"))
      lex_next(L);
    /* optional delta */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
        L->cur.kind == TK_LPAREN ||
        (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
         !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
         !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
         !kw(&L->cur, "END") && !kw(&L->cur, "NEW") &&
         !kw(&L->cur, "CLASS") && !kw(&L->cur, "GETF") &&
         !kw(&L->cur, "SETF") && !kw(&L->cur, "INCFWHERE") &&
         !kw(&L->cur, "SETFWHERE") && !kw(&L->cur, "COPYF"))) {
      long d = parse_expr(vm, L);
      if (dec_mode && d > 0)
        delta = -d;
      else if (dec_mode && d < 0)
        delta = d; /* already negative */
      else
        delta = d;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int mfi, tfi, hit = 0;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      tfi = oop_field_idx(cd, tfield);
      if (mfi < 0 || tfi < 0) { n_skip++; continue; }
      if (ob->fis_str[tfi]) { n_skip++; continue; } /* target numeric only */
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
      ob->fnum[tfi] += delta;
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "INCFWHERE_N", n);
    var_set_num(vm, "ADDWHERE_N", n);
    var_set_num(vm, "BUMPWHERE_N", n);
    var_set_num(vm, "DECFWHERE_N", n);
    var_set_num(vm, "WHEREINC_N", n);
    var_set_num(vm, "INCFWHERE_SKIP", n_skip);
    var_set_num(vm, "INCFWHERE_DELTA", delta);
    var_set_str(vm, "FIELD", tfield);
    var_set_str(vm, "SRC", mfield);
    var_set_str(vm, "DST", tfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* COPYF|COPYFALL|COPYFIELD|MAPCOPYF [Class] src dst
   * â copy src field â dst field on every live object (optional class).
   * Preserves num/str kind per object. Soft skip if either field missing.
   * LAST_N = copy count. TO sugar: COPYF Cell energy TO prev.
   * Usability: snapshot fields before tick/mutation without EACH+GETF+SETF. */
  if (kw(&L->cur, "COPYF") || kw(&L->cur, "COPYFALL") ||
      kw(&L->cur, "COPYFIELD") || kw(&L->cur, "MAPCOPYF") ||
      kw(&L->cur, "COPYFIELDALL") || kw(&L->cur, "DUPFIELD") ||
      kw(&L->cur, "CLONEFIELD") || kw(&L->cur, "FIELDCOPY") ||
      kw(&L->cur, "SNAPSHOTF") || kw(&L->cur, "BACKUPF")) {
    char filt[48], srcf[48], dstf[48], tok1[48];
    int has_filt = 0, i, n = 0, n_skip = 0;
    lex_next(L);
    filt[0] = 0;
    srcf[0] = 0;
    dstf[0] = 0;
    tok1[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "COPYF OF Class src dst"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      /* Class src dst when tok1 is known CLASS */
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "TO") && !kw(&L->cur, "INTO") &&
          !kw(&L->cur, "AS") && !oop_stmt_kw(L)) {
        /* peek: Class src [TO] dst needs a third token; if only one more field name
         * without third, tok1 might be src. Prefer class when known + next is field. */
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(srcf, sizeof srcf, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(srcf, sizeof srcf, "%s", vv->sval);
          else
            snprintf(srcf, sizeof srcf, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(srcf, sizeof srcf, "%s", vv->sval);
        else
          snprintf(srcf, sizeof srcf, "%s", tok1);
      }
    } else {
      fail(vm, "COPYF [Class] src dst"); return -1;
    }
    if (!srcf[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(srcf, sizeof srcf, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "TO") && !kw(&L->cur, "INTO") && !kw(&L->cur, "AS")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(srcf, sizeof srcf, "%s", vv->sval);
        else
          snprintf(srcf, sizeof srcf, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "COPYF [Class] src dst"); return -1;
      }
    }
    if (kw(&L->cur, "TO") || kw(&L->cur, "INTO") || kw(&L->cur, "AS") ||
        kw(&L->cur, "->") || kw(&L->cur, "=")) {
      lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(dstf, sizeof dstf, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(dstf, sizeof dstf, "%s", vv->sval);
      else
        snprintf(dstf, sizeof dstf, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "COPYF [Class] src dst"); return -1;
    }
    if (!srcf[0] || !dstf[0]) {
      fail(vm, "COPYF [Class] src dst"); return -1;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int si, di;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      si = oop_field_idx(cd, srcf);
      di = oop_field_idx(cd, dstf);
      if (si < 0 || di < 0) { n_skip++; continue; }
      if (ob->fis_str[si]) {
        snprintf(ob->fstr[di], sizeof ob->fstr[di], "%s", ob->fstr[si]);
        ob->fis_str[di] = 1;
      } else {
        ob->fnum[di] = ob->fnum[si];
        ob->fis_str[di] = 0;
      }
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "COPYF_N", n);
    var_set_num(vm, "COPYFALL_N", n);
    var_set_num(vm, "MAPCOPYF_N", n);
    var_set_num(vm, "COPYF_SKIP", n_skip);
    var_set_str(vm, "FIELD", srcf);
    var_set_str(vm, "SRC", srcf);
    var_set_str(vm, "DST", dstf);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SWAPF|SWAPFALL|SWAPFIELD|EXCHANGEF [Class] a b
   * â exchange fields a and b on every live object (optional class).
   * Preserves num/str kinds per field. Soft skip if either missing.
   * WITH/AND sugar: SWAPF Cell energy WITH age.
   * Usability: rotate dual buffers without third temp field + EACH+GETF+SETF. */
  if (kw(&L->cur, "SWAPF") || kw(&L->cur, "SWAPFALL") ||
      kw(&L->cur, "SWAPFIELD") || kw(&L->cur, "EXCHANGEF") ||
      kw(&L->cur, "SWAPFIELDS") || kw(&L->cur, "XCHGF") ||
      kw(&L->cur, "FLIPFIELDS") || kw(&L->cur, "FIELDSWAP") ||
      kw(&L->cur, "SWAPF_ALL") || kw(&L->cur, "EXCHFALL")) {
    char filt[48], af[48], bf[48], tok1[48];
    int has_filt = 0, i, n = 0, n_skip = 0;
    lex_next(L);
    filt[0] = 0;
    af[0] = 0;
    bf[0] = 0;
    tok1[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "SWAPF OF Class a b"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "WITH") && !kw(&L->cur, "AND") &&
          !kw(&L->cur, "TO") && !oop_stmt_kw(L)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(af, sizeof af, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(af, sizeof af, "%s", vv->sval);
          else
            snprintf(af, sizeof af, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(af, sizeof af, "%s", vv->sval);
        else
          snprintf(af, sizeof af, "%s", tok1);
      }
    } else {
      fail(vm, "SWAPF [Class] a b"); return -1;
    }
    if (!af[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(af, sizeof af, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WITH") && !kw(&L->cur, "AND") &&
                 !kw(&L->cur, "TO")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(af, sizeof af, "%s", vv->sval);
        else
          snprintf(af, sizeof af, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "SWAPF [Class] a b"); return -1;
      }
    }
    if (kw(&L->cur, "WITH") || kw(&L->cur, "AND") || kw(&L->cur, "TO") ||
        kw(&L->cur, "AND") || kw(&L->cur, ",")) {
      lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(bf, sizeof bf, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(bf, sizeof bf, "%s", vv->sval);
      else
        snprintf(bf, sizeof bf, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "SWAPF [Class] a b"); return -1;
    }
    if (!af[0] || !bf[0]) {
      fail(vm, "SWAPF [Class] a b"); return -1;
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int ia, ib;
      long tnum;
      char tstr[128];
      int t_is_str;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      ia = oop_field_idx(cd, af);
      ib = oop_field_idx(cd, bf);
      if (ia < 0 || ib < 0) { n_skip++; continue; }
      /* swap including kind bits */
      t_is_str = ob->fis_str[ia];
      tnum = ob->fnum[ia];
      snprintf(tstr, sizeof tstr, "%s", ob->fstr[ia]);
      if (ob->fis_str[ib]) {
        snprintf(ob->fstr[ia], sizeof ob->fstr[ia], "%s", ob->fstr[ib]);
        ob->fis_str[ia] = 1;
      } else {
        ob->fnum[ia] = ob->fnum[ib];
        ob->fis_str[ia] = 0;
      }
      if (t_is_str) {
        snprintf(ob->fstr[ib], sizeof ob->fstr[ib], "%s", tstr);
        ob->fis_str[ib] = 1;
      } else {
        ob->fnum[ib] = tnum;
        ob->fis_str[ib] = 0;
      }
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "SWAPF_N", n);
    var_set_num(vm, "SWAPFALL_N", n);
    var_set_num(vm, "EXCHANGEF_N", n);
    var_set_num(vm, "SWAPF_SKIP", n_skip);
    var_set_str(vm, "FIELD", af);
    var_set_str(vm, "SRC", af);
    var_set_str(vm, "DST", bf);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* INCFALL|ADDFALL|DECFALL [Class] field [delta]
   * â add delta (default +1; DECFALL default â1) to numeric field on every live obj.
   * Soft always; missing or string fields skipped (INCFALL_SKIP). LAST_N = update count.
   * Usability: fleet counter/age bump without EACH OBJ + GETF + arith + SETF glue. */
  if (kw(&L->cur, "INCFALL") || kw(&L->cur, "ADDFALL") ||
      kw(&L->cur, "DECFALL") || kw(&L->cur, "SUBFALL") ||
      kw(&L->cur, "INCALL") || kw(&L->cur, "ADDFIELDALL") ||
      kw(&L->cur, "BUMPALL") || kw(&L->cur, "INCFIELDALL")) {
    char filt[48], fname[48], tok1[48], op[24];
    int has_filt = 0, i, n = 0, n_skip = 0, dec_mode = 0, have_delta = 0;
    long delta = 1;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "DECFALL") == 0 || strcmp(op, "SUBFALL") == 0) {
      dec_mode = 1;
      delta = -1;
    }
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "INCFALL OF Class field [delta]"); return -1;
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
        /* Class field [delta] â next is field if not a bare numeric-only line */
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
             L->cur.kind == TK_LPAREN ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "INCFALL [Class] field [delta]"); return -1;
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
        fail(vm, "INCFALL [Class] field [delta]"); return -1;
      }
    }
    /* optional delta: BY n | = n | bare expr (not stmt kw).
     * DECFALL/SUBFALL: magnitude is subtracted (DECFALL f 5 â â5). */
    if (kw(&L->cur, "BY") || kw(&L->cur, "PLUS") || kw(&L->cur, "DELTA")) {
      lex_next(L);
      delta = parse_expr(vm, L);
      have_delta = 1;
    } else if (L->cur.kind == TK_EQ) {
      lex_next(L);
      delta = parse_expr(vm, L);
      have_delta = 1;
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN ||
               (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
                !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
                !kw(&L->cur, "END") && !kw(&L->cur, "NEW") &&
                !kw(&L->cur, "GETF") && !kw(&L->cur, "SETF") &&
                !kw(&L->cur, "INCFALL") && !kw(&L->cur, "ADDFALL") &&
                !kw(&L->cur, "DECFALL") && !kw(&L->cur, "CLASS") &&
                !kw(&L->cur, "WHEREOBJ") && !kw(&L->cur, "SENDWHERE") &&
                !kw(&L->cur, "DELETEWHERE") && !kw(&L->cur, "SETFALL") &&
                !kw(&L->cur, "GETFALL") && !kw(&L->cur, "SENDALL") &&
                !kw(&L->cur, "LISTOBJS") && !kw(&L->cur, "HASOBJ"))) {
      delta = parse_expr(vm, L);
      have_delta = 1;
    }
    if (dec_mode && have_delta) {
      if (delta < 0) delta = -delta; /* normalize then flip */
      delta = -delta;
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
      if (ob->fis_str[fi]) { n_skip++; continue; } /* numeric only */
      ob->fnum[fi] += delta;
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "INCFALL_N", n);
    var_set_num(vm, "ADDFALL_N", n);
    var_set_num(vm, "DECFALL_N", n);
    var_set_num(vm, "INCFALL_SKIP", n_skip);
    var_set_num(vm, "INCFALL_DELTA", delta);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0])
      var_set_str(vm, "CLASS", filt);
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* MULFALL|SCALEFALL|MULF|TIMESFALL [Class] field factor
   * â multiply every live object's numeric field by factor (integer).
   * Soft always; missing/string fields skipped. LAST_N = update count.
   * Usability: scale scores/energy (double, zero, decay) without EACH+GETF+*+SETF.
   * Complements INCFALL (add) with product update. */
  if (kw(&L->cur, "MULFALL") || kw(&L->cur, "SCALEFALL") ||
      kw(&L->cur, "MULF") || kw(&L->cur, "TIMESFALL") ||
      kw(&L->cur, "MULALL") || kw(&L->cur, "SCALEF") ||
      kw(&L->cur, "MULTFALL") || kw(&L->cur, "PRODUCTFALL") ||
      kw(&L->cur, "SCALEFIELDALL") || kw(&L->cur, "MULFIELDALL")) {
    char filt[48], fname[48], tok1[48];
    int has_filt = 0, i, n = 0, n_skip = 0;
    long factor = 1;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "MULFALL OF Class field factor"); return -1;
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
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "MULFALL [Class] field factor"); return -1;
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
        fail(vm, "MULFALL [Class] field factor"); return -1;
      }
    }
    /* factor: BY n | * n | = n | bare expr (required for scale) */
    if (kw(&L->cur, "BY") || kw(&L->cur, "TIMES") || kw(&L->cur, "FACTOR") ||
        kw(&L->cur, "MUL") || kw(&L->cur, "SCALE"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ)
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      factor = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      factor = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
               !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
               !kw(&L->cur, "END") && !kw(&L->cur, "NEW") &&
               !kw(&L->cur, "GETF") && !kw(&L->cur, "SETF") &&
               !kw(&L->cur, "INCFALL") && !kw(&L->cur, "MULFALL") &&
               !kw(&L->cur, "CLAMPFALL") && !kw(&L->cur, "CLASS")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        factor = atol(sv->sval);
      else
        factor = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN) {
      factor = parse_expr(vm, L);
    } else {
      fail(vm, "MULFALL [Class] field factor"); return -1;
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
      ob->fnum[fi] *= factor;
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "MULFALL_N", n);
    var_set_num(vm, "SCALEFALL_N", n);
    var_set_num(vm, "MULF_N", n);
    var_set_num(vm, "TIMESFALL_N", n);
    var_set_num(vm, "MULFALL_SKIP", n_skip);
    var_set_num(vm, "MULFALL_FACTOR", factor);
    var_set_num(vm, "SCALEFALL_FACTOR", factor);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* CLAMPFALL|CLAMPF|BOUNDALL [Class] field lo [TO] hi
   * â clamp every live object's numeric field into [lo,hi] (auto-swap lo/hi).
   * Soft always; string/missing fields skipped. LAST_N = objs touched (any clamp
   * or already-in-range still counts as visited with field present).
   * CLAMPFALL_CHANGED = how many values actually moved.
   * Usability: bound energy/retries/scores without EACH+GETF+IF+SETF glue. */
  if (kw(&L->cur, "CLAMPFALL") || kw(&L->cur, "CLAMPF") ||
      kw(&L->cur, "BOUNDALL") || kw(&L->cur, "CLAMPFIELD") ||
      kw(&L->cur, "CLIPFALL") || kw(&L->cur, "CLIPF") ||
      kw(&L->cur, "LIMITFALL") || kw(&L->cur, "BOUNDFALL") ||
      kw(&L->cur, "SATFALL") || kw(&L->cur, "SATF")) {
    char filt[48], fname[48], tok1[48];
    int has_filt = 0, i, n = 0, n_skip = 0, n_chg = 0;
    long lo = 0, hi = 0, t;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "CLAMPFALL OF Class field lo hi"); return -1;
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
      fail(vm, "CLAMPFALL [Class] field lo hi"); return -1;
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
        fail(vm, "CLAMPFALL [Class] field lo hi"); return -1;
      }
    }
    /* optional glue before lo */
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
      long v, nv;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) { n_skip++; continue; }
      if (ob->fis_str[fi]) { n_skip++; continue; }
      v = ob->fnum[fi];
      nv = v;
      if (nv < lo) nv = lo;
      if (nv > hi) nv = hi;
      if (nv != v) {
        ob->fnum[fi] = nv;
        n_chg++;
      }
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "CLAMPFALL_N", n);
    var_set_num(vm, "CLAMPF_N", n);
    var_set_num(vm, "CLIPFALL_N", n);
    var_set_num(vm, "BOUNDALL_N", n);
    var_set_num(vm, "CLAMPFALL_SKIP", n_skip);
    var_set_num(vm, "CLAMPFALL_CHANGED", n_chg);
    var_set_num(vm, "CLAMPFALL_LO", lo);
    var_set_num(vm, "CLAMPFALL_HI", hi);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* CLAMPWHERE|CLIPWHERE|BOUNDWHERE [Class] valfield matchfield matchvalue lo [TO] hi
   * â clamp numeric valfield into [lo,hi] where matchfield == matchvalue.
   * WHERE sugar: CLAMPWHERE Cell energy WHERE age 1 0 TO 100.
   * LAST_N = matched objs with numeric field; CLAMPWHERE_CHANGED = values moved.
   * Soft empty â 0. Usability: selective clamp without EACH+GETF+IF+SETF. */
  if (kw(&L->cur, "CLAMPWHERE") || kw(&L->cur, "CLIPWHERE") ||
      kw(&L->cur, "BOUNDWHERE") || kw(&L->cur, "CLAMPFWHERE") ||
      kw(&L->cur, "WHERECLAMP") || kw(&L->cur, "CLAMPIF") ||
      kw(&L->cur, "CLIPIF") || kw(&L->cur, "BOUNDIF") ||
      kw(&L->cur, "LIMITWHERE") || kw(&L->cur, "SATWHERE")) {
    char filt[48], vfield[48], mfield[48], tok1[48];
    char m_sval[512];
    int has_filt = 0, m_is_str = 0, i, n = 0, n_skip = 0, n_chg = 0;
    long m_nval = 0, lo = 0, hi = 0, t;
    lex_next(L);
    filt[0] = 0;
    vfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "CLAMPWHERE OF Class valfield matchfield matchvalue lo hi");
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
      fail(vm, "CLAMPWHERE [Class] valfield matchfield matchvalue lo hi");
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
        fail(vm, "CLAMPWHERE [Class] valfield matchfield matchvalue lo hi");
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
      fail(vm, "CLAMPWHERE [Class] valfield matchfield matchvalue lo hi");
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
    /* optional glue before lo */
    if (kw(&L->cur, "IN") || kw(&L->cur, "BETWEEN") || kw(&L->cur, "RANGE") ||
        kw(&L->cur, "TO") || kw(&L->cur, "CLAMP"))
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
      int vfi, mfi, hit = 0;
      long v, nv;
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
      nv = v;
      if (nv < lo) nv = lo;
      if (nv > hi) nv = hi;
      if (nv != v) {
        ob->fnum[vfi] = nv;
        n_chg++;
      }
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "CLAMPWHERE_N", n);
    var_set_num(vm, "CLIPWHERE_N", n);
    var_set_num(vm, "BOUNDWHERE_N", n);
    var_set_num(vm, "CLAMPFWHERE_N", n);
    var_set_num(vm, "CLAMPWHERE_SKIP", n_skip);
    var_set_num(vm, "CLAMPWHERE_CHANGED", n_chg);
    var_set_num(vm, "CLAMPWHERE_LO", lo);
    var_set_num(vm, "CLAMPWHERE_HI", hi);
    var_set_str(vm, "FIELD", vfield);
    var_set_str(vm, "SRC", mfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* MULWHERE|SCALEWHERE|MULFWHERE [Class] valfield matchfield matchvalue factor
   * â multiply numeric valfield by factor where matchfield == matchvalue.
   * WHERE sugar: MULWHERE Cell energy WHERE age 1 BY 2.
   * LAST_N = matched updates; MULWHERE_FACTOR = factor; soft empty â 0.
   * Usability: selective scale without EACH+GETF+IF+SETF (pairs with CLAMPWHERE). */
  if (kw(&L->cur, "MULWHERE") || kw(&L->cur, "SCALEWHERE") ||
      kw(&L->cur, "MULFWHERE") || kw(&L->cur, "WHEREMUL") ||
      kw(&L->cur, "MULIF") || kw(&L->cur, "SCALEIF") ||
      kw(&L->cur, "TIMESWHERE") || kw(&L->cur, "WHERESCALE") ||
      kw(&L->cur, "PRODUCTWHERE") || kw(&L->cur, "SCALEFWHERE")) {
    char filt[48], vfield[48], mfield[48], tok1[48];
    char m_sval[512];
    int has_filt = 0, m_is_str = 0, i, n = 0, n_skip = 0;
    long m_nval = 0, factor = 1;
    lex_next(L);
    filt[0] = 0;
    vfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "MULWHERE OF Class valfield matchfield matchvalue factor");
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
      fail(vm, "MULWHERE [Class] valfield matchfield matchvalue factor");
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
        fail(vm, "MULWHERE [Class] valfield matchfield matchvalue factor");
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
      fail(vm, "MULWHERE [Class] valfield matchfield matchvalue factor");
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
    /* factor: BY n | TIMES n | bare */
    if (kw(&L->cur, "BY") || kw(&L->cur, "TIMES") || kw(&L->cur, "FACTOR") ||
        kw(&L->cur, "MUL") || kw(&L->cur, "SCALE") || kw(&L->cur, "WITH"))
      lex_next(L);
    else if (L->cur.kind == TK_EQ)
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      factor = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      factor = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
               !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
               !kw(&L->cur, "END") && !kw(&L->cur, "NEW") &&
               !kw(&L->cur, "GETF") && !kw(&L->cur, "SETF")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        factor = atol(sv->sval);
      else
        factor = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN) {
      factor = parse_expr(vm, L);
    } else {
      fail(vm, "MULWHERE [Class] valfield matchfield matchvalue factor");
      return -1;
    }
    for (i = 0; i < vm->n_objs; i++) {
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
      ob->fnum[vfi] *= factor;
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "MULWHERE_N", n);
    var_set_num(vm, "SCALEWHERE_N", n);
    var_set_num(vm, "MULFWHERE_N", n);
    var_set_num(vm, "WHEREMUL_N", n);
    var_set_num(vm, "MULWHERE_SKIP", n_skip);
    var_set_num(vm, "MULWHERE_FACTOR", factor);
    var_set_num(vm, "SCALEWHERE_FACTOR", factor);
    var_set_str(vm, "FIELD", vfield);
    var_set_str(vm, "SRC", mfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* COPYFWHERE|SNAPSHOTWHERE|BACKUPWHERE [Class] src [TO] dst matchfield matchvalue
   * â copy srcâdst field where matchfield == matchvalue.
   * WHERE sugar: COPYFWHERE Cell energy TO prev WHERE age 1.
   * LAST_N = copy count; soft empty â 0.
   * Usability: selective snapshot without EACH+GETF+IF+SETF. */
  if (kw(&L->cur, "COPYFWHERE") || kw(&L->cur, "SNAPSHOTWHERE") ||
      kw(&L->cur, "BACKUPWHERE") || kw(&L->cur, "WHERECOPYF") ||
      kw(&L->cur, "COPYIF") || kw(&L->cur, "SNAPSHOTIF") ||
      kw(&L->cur, "BACKUPIF") || kw(&L->cur, "DUPWHERE") ||
      kw(&L->cur, "CLONEWHERE") || kw(&L->cur, "FIELDCOPYWHERE")) {
    char filt[48], srcf[48], dstf[48], mfield[48], tok1[48];
    char m_sval[512];
    int has_filt = 0, m_is_str = 0, i, n = 0, n_skip = 0;
    long m_nval = 0;
    lex_next(L);
    filt[0] = 0;
    srcf[0] = 0;
    dstf[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "COPYFWHERE OF Class src dst matchfield matchvalue");
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
          !kw(&L->cur, "TO") && !kw(&L->cur, "INTO") && !kw(&L->cur, "AS") &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !oop_stmt_kw(L)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(srcf, sizeof srcf, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(srcf, sizeof srcf, "%s", vv->sval);
          else
            snprintf(srcf, sizeof srcf, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
             kw(&L->cur, "TO") || kw(&L->cur, "WHERE")))
          snprintf(srcf, sizeof srcf, "%s", vv->sval);
        else
          snprintf(srcf, sizeof srcf, "%s", tok1);
      }
    } else {
      fail(vm, "COPYFWHERE [Class] src dst matchfield matchvalue");
      return -1;
    }
    if (!srcf[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(srcf, sizeof srcf, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "TO") && !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(srcf, sizeof srcf, "%s", vv->sval);
        else
          snprintf(srcf, sizeof srcf, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "COPYFWHERE [Class] src dst matchfield matchvalue");
        return -1;
      }
    }
    if (kw(&L->cur, "TO") || kw(&L->cur, "INTO") || kw(&L->cur, "AS") ||
        kw(&L->cur, "->") || (L->cur.kind == TK_EQ))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(dstf, sizeof dstf, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(dstf, sizeof dstf, "%s", vv->sval);
      else
        snprintf(dstf, sizeof dstf, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "COPYFWHERE [Class] src dst matchfield matchvalue");
      return -1;
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
      fail(vm, "COPYFWHERE [Class] src dst matchfield matchvalue");
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
      int si, di, mfi, hit = 0;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      si = oop_field_idx(cd, srcf);
      di = oop_field_idx(cd, dstf);
      if (mfi < 0 || si < 0 || di < 0) { n_skip++; continue; }
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
      if (ob->fis_str[si]) {
        snprintf(ob->fstr[di], sizeof ob->fstr[di], "%s", ob->fstr[si]);
        ob->fis_str[di] = 1;
      } else {
        ob->fnum[di] = ob->fnum[si];
        ob->fis_str[di] = 0;
      }
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "COPYFWHERE_N", n);
    var_set_num(vm, "SNAPSHOTWHERE_N", n);
    var_set_num(vm, "BACKUPWHERE_N", n);
    var_set_num(vm, "WHERECOPYF_N", n);
    var_set_num(vm, "COPYFWHERE_SKIP", n_skip);
    var_set_str(vm, "FIELD", srcf);
    var_set_str(vm, "SRC", srcf);
    var_set_str(vm, "DST", dstf);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SWAPFWHERE|EXCHANGEWHERE|XCHFWHERE [Class] a [WITH] b matchfield matchvalue
   * â exchange fields a and b where matchfield == matchvalue.
   * WHERE sugar: SWAPFWHERE Cell energy WITH age WHERE ready 1.
   * LAST_N = swap count; soft empty â 0.
   * Usability: selective dual-buffer rotate without EACH+GETF+IF+temp. */
  /* Note: SWAPIF is stack-plane (lang_ops_stack) â do not alias here. */
  if (kw(&L->cur, "SWAPFWHERE") || kw(&L->cur, "EXCHANGEWHERE") ||
      kw(&L->cur, "XCHFWHERE") || kw(&L->cur, "WHERESWAPF") ||
      kw(&L->cur, "EXCHANGEIF") || kw(&L->cur, "FLIPWHERE") ||
      kw(&L->cur, "FIELDSWAPWHERE") || kw(&L->cur, "WHEREEXCHANGE") ||
      kw(&L->cur, "XCHGIF") || kw(&L->cur, "SWAPFIELDWHERE")) {
    char filt[48], af[48], bf[48], mfield[48], tok1[48];
    char m_sval[512];
    int has_filt = 0, m_is_str = 0, i, n = 0, n_skip = 0;
    long m_nval = 0;
    lex_next(L);
    filt[0] = 0;
    af[0] = 0;
    bf[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "SWAPFWHERE OF Class a b matchfield matchvalue");
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
          !kw(&L->cur, "WITH") && !kw(&L->cur, "AND") && !kw(&L->cur, "TO") &&
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !oop_stmt_kw(L)) {
        snprintf(filt, sizeof filt, "%s", tok1);
        has_filt = 1;
        if (L->cur.kind == TK_STR) {
          snprintf(af, sizeof af, "%s", L->cur.text);
          lex_next(L);
        } else {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(af, sizeof af, "%s", vv->sval);
          else
            snprintf(af, sizeof af, "%s", L->cur.text);
          lex_next(L);
        }
      } else {
        Var *vv = var_get(vm, tok1, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
             kw(&L->cur, "WITH") || kw(&L->cur, "WHERE")))
          snprintf(af, sizeof af, "%s", vv->sval);
        else
          snprintf(af, sizeof af, "%s", tok1);
      }
    } else {
      fail(vm, "SWAPFWHERE [Class] a b matchfield matchvalue");
      return -1;
    }
    if (!af[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(af, sizeof af, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WITH") && !kw(&L->cur, "AND") &&
                 !kw(&L->cur, "TO") && !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(af, sizeof af, "%s", vv->sval);
        else
          snprintf(af, sizeof af, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "SWAPFWHERE [Class] a b matchfield matchvalue");
        return -1;
      }
    }
    if (kw(&L->cur, "WITH") || kw(&L->cur, "AND") || kw(&L->cur, "TO") ||
        (L->cur.kind == TK_IDENT && strcmp(L->cur.text, ",") == 0))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(bf, sizeof bf, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(bf, sizeof bf, "%s", vv->sval);
      else
        snprintf(bf, sizeof bf, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "SWAPFWHERE [Class] a b matchfield matchvalue");
      return -1;
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
      fail(vm, "SWAPFWHERE [Class] a b matchfield matchvalue");
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
      int ia, ib, mfi, hit = 0;
      long tnum;
      char tstr[128];
      int t_is_str;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      mfi = oop_field_idx(cd, mfield);
      ia = oop_field_idx(cd, af);
      ib = oop_field_idx(cd, bf);
      if (mfi < 0 || ia < 0 || ib < 0) { n_skip++; continue; }
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
      t_is_str = ob->fis_str[ia];
      tnum = ob->fnum[ia];
      snprintf(tstr, sizeof tstr, "%s", ob->fstr[ia]);
      if (ob->fis_str[ib]) {
        snprintf(ob->fstr[ia], sizeof ob->fstr[ia], "%s", ob->fstr[ib]);
        ob->fis_str[ia] = 1;
      } else {
        ob->fnum[ia] = ob->fnum[ib];
        ob->fis_str[ia] = 0;
      }
      if (t_is_str) {
        snprintf(ob->fstr[ib], sizeof ob->fstr[ib], "%s", tstr);
        ob->fis_str[ib] = 1;
      } else {
        ob->fnum[ib] = tnum;
        ob->fis_str[ib] = 0;
      }
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%d", n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "SWAPFWHERE_N", n);
    var_set_num(vm, "EXCHANGEWHERE_N", n);
    var_set_num(vm, "XCHFWHERE_N", n);
    var_set_num(vm, "WHERESWAPF_N", n);
    var_set_num(vm, "SWAPFWHERE_SKIP", n_skip);
    var_set_str(vm, "FIELD", af);
    var_set_str(vm, "SRC", af);
    var_set_str(vm, "DST", bf);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* MAXOBJ|HIGHEST|ARGMAXF [Class] field
   * MINOBJ|LOWEST|ARGMINF [Class] field
   * â name of first live object with max/min numeric field (optional class).
   * LAST=name, LAST_N=field value, OBJECT=name. Soft empty â OK=0 LAST="".
   * Usability: pick extreme without GETFALL + ARGMAX + NTH / ZIP glue. */
  if (kw(&L->cur, "MAXOBJ") || kw(&L->cur, "HIGHEST") ||
      kw(&L->cur, "ARGMAXF") || kw(&L->cur, "TOPOBJ") ||
      kw(&L->cur, "BESTOBJ") || kw(&L->cur, "MAXINST") ||
      kw(&L->cur, "MINOBJ") || kw(&L->cur, "LOWEST") ||
      kw(&L->cur, "ARGMINF") || kw(&L->cur, "BOTTOMOBJ") ||
      kw(&L->cur, "WORSTOBJ") || kw(&L->cur, "MININST")) {
    char filt[48], fname[48], tok1[48], op[24], best_name[48];
    int has_filt = 0, want_min = 0, i, n = 0, found = 0;
    long best = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "MINOBJ") == 0 || strcmp(op, "LOWEST") == 0 ||
        strcmp(op, "ARGMINF") == 0 || strcmp(op, "BOTTOMOBJ") == 0 ||
        strcmp(op, "WORSTOBJ") == 0 || strcmp(op, "MININST") == 0)
      want_min = 1;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    best_name[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "BY")) {
      if (kw(&L->cur, "BY")) {
        /* MAXOBJ BY field â no class; next is field */
        lex_next(L);
      } else {
        lex_next(L);
        if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
          fail(vm, "MAXOBJ OF Class field"); return -1;
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
      fail(vm, "MAXOBJ [Class] field"); return -1;
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
        fail(vm, "MAXOBJ [Class] field"); return -1;
      }
    }
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      int fi;
      long v;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      fi = oop_field_idx(cd, fname);
      if (fi < 0) continue;
      if (ob->fis_str[fi]) continue; /* numeric field only */
      v = ob->fnum[fi];
      n++;
      if (!found || (want_min ? (v < best) : (v > best))) {
        best = v;
        snprintf(best_name, sizeof best_name, "%s", ob->name);
        found = 1;
      }
    }
    if (!found) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_str(vm, "MAXOBJ", "");
      var_set_str(vm, "MINOBJ", "");
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "MAXOBJ_N", 0);
      var_set_num(vm, "MINOBJ_N", 0);
      var_set_num(vm, "MAXOBJ_VAL", 0);
      var_set_str(vm, "FIELD", fname);
      if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "MAXOBJ: no matching objects");
      var_set_str(vm, "ERR", "MAXOBJ: no matching objects");
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", best_name);
    var_set_str(vm, "MAXOBJ", best_name);
    var_set_str(vm, "MINOBJ", best_name);
    var_set_str(vm, "OBJECT", best_name);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", best_name);
    var_set_num(vm, "LAST_N", best);
    vm->last_n = best;
    var_set_num(vm, "MAXOBJ_N", 1);
    var_set_num(vm, "MINOBJ_N", 1);
    var_set_num(vm, "MAXOBJ_VAL", best);
    var_set_num(vm, "MINOBJ_VAL", best);
    var_set_num(vm, "NOBJS", n);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* MAXOBJWHERE|MINOBJWHERE|BESTWHERE|WORSTWHERE [Class] valfield matchfield matchvalue
   * â name of first live object with max/min numeric valfield among matchfield==matchvalue.
   * WHERE sugar: MAXOBJWHERE Cell energy WHERE age 1.
   * LAST=name, LAST_N=extreme value, OBJECT=name; soft empty â OK=0 LAST="".
   * Usability: pick extreme in subset without WHEREOBJ+MAXOBJ or EACH+GETF glue. */
  /* Note: HIGHESTWHERE/LOWESTWHERE are MAXWHERE/MINWHERE value extremes â not here. */
  if (kw(&L->cur, "MAXOBJWHERE") || kw(&L->cur, "MINOBJWHERE") ||
      kw(&L->cur, "BESTWHERE") || kw(&L->cur, "WORSTWHERE") ||
      kw(&L->cur, "ARGMAXWHERE") || kw(&L->cur, "ARGMINWHERE") ||
      kw(&L->cur, "TOPOBJWHERE") || kw(&L->cur, "BOTOBJWHERE") ||
      kw(&L->cur, "MAXIFOBJ") || kw(&L->cur, "MINIFOBJ") ||
      kw(&L->cur, "BESTOBJWHERE") || kw(&L->cur, "WORSTOBJWHERE")) {
    char filt[48], vfield[48], mfield[48], tok1[48], op[24], best_name[48];
    char m_sval[512];
    int has_filt = 0, m_is_str = 0, want_min = 0, i, n = 0, n_skip = 0, found = 0;
    long m_nval = 0, best = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "MINOBJWHERE") == 0 || strcmp(op, "WORSTWHERE") == 0 ||
        strcmp(op, "ARGMINWHERE") == 0 || strcmp(op, "BOTOBJWHERE") == 0 ||
        strcmp(op, "MINIFOBJ") == 0 || strcmp(op, "WORSTOBJWHERE") == 0)
      want_min = 1;
    lex_next(L);
    filt[0] = 0;
    vfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    best_name[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "MAXOBJWHERE OF Class valfield matchfield matchvalue");
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
      fail(vm, "MAXOBJWHERE [Class] valfield matchfield matchvalue");
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
        fail(vm, "MAXOBJWHERE [Class] valfield matchfield matchvalue");
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
      fail(vm, "MAXOBJWHERE [Class] valfield matchfield matchvalue");
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
      n++;
      if (!found || (want_min ? (v < best) : (v > best))) {
        best = v;
        snprintf(best_name, sizeof best_name, "%s", ob->name);
        found = 1;
      }
    }
    if (!found) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_str(vm, "MAXOBJWHERE", "");
      var_set_str(vm, "MINOBJWHERE", "");
      var_set_str(vm, "OBJECT", "");
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "MAXOBJWHERE_N", 0);
      var_set_num(vm, "MINOBJWHERE_N", 0);
      var_set_num(vm, "MAXOBJWHERE_VAL", 0);
      var_set_num(vm, "MINOBJWHERE_VAL", 0);
      var_set_num(vm, "MAXOBJWHERE_SKIP", n_skip);
      var_set_num(vm, "MAXOBJWHERE_MATCH", 0);
      var_set_str(vm, "FIELD", vfield);
      var_set_str(vm, "SRC", mfield);
      if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "MAXOBJWHERE: no matching objects");
      var_set_str(vm, "ERR", "MAXOBJWHERE: no matching objects");
      bump(vm);
      return 1;
    }
    var_set_str(vm, "LAST", best_name);
    var_set_str(vm, "MAXOBJWHERE", best_name);
    var_set_str(vm, "MINOBJWHERE", best_name);
    var_set_str(vm, "BESTWHERE", best_name);
    var_set_str(vm, "OBJECT", best_name);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", best_name);
    var_set_num(vm, "LAST_N", best);
    vm->last_n = best;
    var_set_num(vm, "MAXOBJWHERE_N", 1);
    var_set_num(vm, "MINOBJWHERE_N", 1);
    var_set_num(vm, "MAXOBJWHERE_VAL", best);
    var_set_num(vm, "MINOBJWHERE_VAL", best);
    var_set_num(vm, "MAXOBJWHERE_MATCH", n);
    var_set_num(vm, "MINOBJWHERE_MATCH", n);
    var_set_num(vm, "MAXOBJWHERE_SKIP", n_skip);
    var_set_num(vm, "NOBJS", n);
    var_set_str(vm, "FIELD", vfield);
    var_set_str(vm, "SRC", mfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SORTBYF|SORTOBJS|ORDERBYF [Class] field [ASC|DESC]
   * â bag of live object names ordered by numeric field (stable first-seen on ties).
   * Soft always; string/missing fields skipped. LAST_N = count. Default ASC.
   * Usability: ranked fleet without GETFALL+ZIP+SORTN+KEYS glue. */
  if (kw(&L->cur, "SORTBYF") || kw(&L->cur, "SORTOBJS") ||
      kw(&L->cur, "ORDERBYF") || kw(&L->cur, "RANKBYF") ||
      kw(&L->cur, "ORDEROBJS") || kw(&L->cur, "SORTBYFIELD") ||
      kw(&L->cur, "OBJSSORT") || kw(&L->cur, "RANKOBJS")) {
    char filt[48], fname[48], tok1[48], bag[4096];
    int has_filt = 0, desc = 0, i, n = 0, n_skip = 0;
    size_t o = 0;
    typedef struct { char name[48]; long v; int idx; } SortByFRow;
    SortByFRow rows[CUBALC_MAX_OBJS];
    SortByFRow key;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "BY")) {
      if (kw(&L->cur, "BY")) {
        lex_next(L);
      } else {
        lex_next(L);
        if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
          fail(vm, "SORTBYF OF Class field [ASC|DESC]"); return -1;
        }
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      }
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
          !kw(&L->cur, "UP") && !kw(&L->cur, "DOWN")) {
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
      fail(vm, "SORTBYF [Class] field [ASC|DESC]"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "SORTBYF [Class] field [ASC|DESC]"); return -1;
      }
    }
    if (kw(&L->cur, "DESC") || kw(&L->cur, "DOWN") || kw(&L->cur, "REV") ||
        kw(&L->cur, "REVERSE") || kw(&L->cur, "HIGHFIRST")) {
      desc = 1;
      lex_next(L);
    } else if (kw(&L->cur, "ASC") || kw(&L->cur, "UP") ||
               kw(&L->cur, "LOWFIRST")) {
      desc = 0;
      lex_next(L);
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
      snprintf(rows[n].name, sizeof rows[n].name, "%s", ob->name);
      rows[n].v = ob->fnum[fi];
      rows[n].idx = n;
      n++;
    }
    /* stable-ish insertion sort (n<=64) */
    {
      int a, b;
      for (a = 1; a < n; a++) {
        key = rows[a];
        b = a - 1;
        while (b >= 0) {
          int less;
          if (desc)
            less = (key.v > rows[b].v) ||
                   (key.v == rows[b].v && key.idx < rows[b].idx);
          else
            less = (key.v < rows[b].v) ||
                   (key.v == rows[b].v && key.idx < rows[b].idx);
          if (!less) break;
          rows[b + 1] = rows[b];
          b--;
        }
        rows[b + 1] = key;
      }
    }
    for (i = 0; i < n; i++) {
      size_t ln = strlen(rows[i].name);
      if (i > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, rows[i].name, ln);
        o += ln;
      }
      bag[o] = 0;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "SORTBYF", bag);
    var_set_str(vm, "SORTOBJS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "SORTBYF_N", n);
    var_set_num(vm, "SORTOBJS_N", n);
    var_set_num(vm, "SORTBYF_SKIP", n_skip);
    var_set_num(vm, "SORTBYF_DESC", desc);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SORTBYFWHERE|ORDERWHERE|RANKWHERE [Class] sortfield matchfield matchvalue [ASC|DESC]
   * â bag of matching object names ordered by numeric sortfield.
   * WHERE sugar: SORTBYFWHERE Cell energy WHERE age 1 DESC.
   * Soft empty â "" / 0. Usability: ranked subset without WHEREOBJ+SORTBYF glue. */
  if (kw(&L->cur, "SORTBYFWHERE") || kw(&L->cur, "ORDERWHERE") ||
      kw(&L->cur, "RANKWHERE") || kw(&L->cur, "WHERESORTBYF") ||
      kw(&L->cur, "SORTOBJSWHERE") || kw(&L->cur, "ORDERBYFWHERE") ||
      kw(&L->cur, "RANKBYFWHERE") || kw(&L->cur, "SORTIF") ||
      kw(&L->cur, "ORDERIF") || kw(&L->cur, "RANKIF")) {
    char filt[48], sfield[48], mfield[48], tok1[48], bag[4096];
    char m_sval[512];
    int has_filt = 0, m_is_str = 0, desc = 0, i, n = 0, n_skip = 0;
    long m_nval = 0;
    size_t o = 0;
    typedef struct { char name[48]; long v; int idx; } SortByFWRow;
    SortByFWRow rows[CUBALC_MAX_OBJS];
    SortByFWRow key;
    lex_next(L);
    filt[0] = 0;
    sfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "SORTBYFWHERE OF Class sortfield matchfield matchvalue");
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
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN") &&
          !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC")) {
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
      fail(vm, "SORTBYFWHERE [Class] sortfield matchfield matchvalue");
      return -1;
    }
    if (!sfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(sfield, sizeof sfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") &&
                 !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC")) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(sfield, sizeof sfield, "%s", vv->sval);
        else
          snprintf(sfield, sizeof sfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "SORTBYFWHERE [Class] sortfield matchfield matchvalue");
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
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS") &&
               !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "SORTBYFWHERE [Class] sortfield matchfield matchvalue");
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
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
               !kw(&L->cur, "UP") && !kw(&L->cur, "DOWN")) {
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
    if (kw(&L->cur, "DESC") || kw(&L->cur, "DOWN") || kw(&L->cur, "REV") ||
        kw(&L->cur, "REVERSE") || kw(&L->cur, "HIGHFIRST")) {
      desc = 1;
      lex_next(L);
    } else if (kw(&L->cur, "ASC") || kw(&L->cur, "UP") ||
               kw(&L->cur, "LOWFIRST")) {
      desc = 0;
      lex_next(L);
    }
    for (i = 0; i < vm->n_objs && n < CUBALC_MAX_OBJS; i++) {
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
      if (ob->fis_str[sfi]) { n_skip++; continue; }
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
      snprintf(rows[n].name, sizeof rows[n].name, "%s", ob->name);
      rows[n].v = ob->fnum[sfi];
      rows[n].idx = n;
      n++;
    }
    {
      int a, b;
      for (a = 1; a < n; a++) {
        key = rows[a];
        b = a - 1;
        while (b >= 0) {
          int less;
          if (desc)
            less = (key.v > rows[b].v) ||
                   (key.v == rows[b].v && key.idx < rows[b].idx);
          else
            less = (key.v < rows[b].v) ||
                   (key.v == rows[b].v && key.idx < rows[b].idx);
          if (!less) break;
          rows[b + 1] = rows[b];
          b--;
        }
        rows[b + 1] = key;
      }
    }
    for (i = 0; i < n; i++) {
      size_t ln = strlen(rows[i].name);
      if (i > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, rows[i].name, ln);
        o += ln;
      }
      bag[o] = 0;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "SORTBYFWHERE", bag);
    var_set_str(vm, "ORDERWHERE", bag);
    var_set_str(vm, "RANKWHERE", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "SORTBYFWHERE_N", n);
    var_set_num(vm, "ORDERWHERE_N", n);
    var_set_num(vm, "RANKWHERE_N", n);
    var_set_num(vm, "SORTBYFWHERE_SKIP", n_skip);
    var_set_num(vm, "SORTBYFWHERE_DESC", desc);
    var_set_str(vm, "FIELD", sfield);
    var_set_str(vm, "SRC", mfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* TOPNBYF|HEADBYF|TAKEBYF [Class] field n [ASC|DESC]
   * â first n live object names ordered by numeric field (stable ties).
   * Soft always; string/missing fields skipped. n<=0 â empty bag.
   * LAST = name bag; LAST_N/TOPNBYF_N = returned count; TOPNBYF_TOTAL = ranked pool.
   * Usability: top-k fleet without SORTBYF + SYS TAKE glue (peer pick, leaderboard). */
  if (kw(&L->cur, "TOPNBYF") || kw(&L->cur, "HEADBYF") ||
      kw(&L->cur, "TAKEBYF") || kw(&L->cur, "FIRSTNBYF") ||
      kw(&L->cur, "LIMITBYF") || kw(&L->cur, "TOPNOBJS") ||
      kw(&L->cur, "HEADOBJS") || kw(&L->cur, "TAKEOBJS") ||
      kw(&L->cur, "TOPNFIELD") || kw(&L->cur, "HEADNBYF")) {
    char filt[48], fname[48], tok1[48], bag[4096];
    int has_filt = 0, desc = 0, i, n = 0, n_skip = 0, take = 0, out_n = 0;
    long narg = 0;
    size_t o = 0;
    typedef struct { char name[48]; long v; int idx; } TopNByFRow;
    TopNByFRow rows[CUBALC_MAX_OBJS];
    TopNByFRow key;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "BY")) {
      if (kw(&L->cur, "BY")) {
        lex_next(L);
      } else {
        lex_next(L);
        if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
          fail(vm, "TOPNBYF OF Class field n [ASC|DESC]"); return -1;
        }
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      }
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
          !kw(&L->cur, "UP") && !kw(&L->cur, "DOWN") &&
          L->cur.kind != TK_NUM && L->cur.kind != TK_MINUS &&
          L->cur.kind != TK_LPAREN) {
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
             L->cur.kind == TK_LPAREN ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
              !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC"))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "TOPNBYF [Class] field n [ASC|DESC]"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
                 L->cur.kind != TK_NUM) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "TOPNBYF [Class] field n [ASC|DESC]"); return -1;
      }
    }
    /* optional sugar: TOP n | FIRST n | LIMIT n | TAKE n */
    if (kw(&L->cur, "TOP") || kw(&L->cur, "FIRST") || kw(&L->cur, "LIMIT") ||
        kw(&L->cur, "TAKE") || kw(&L->cur, "N") || kw(&L->cur, "COUNT") ||
        kw(&L->cur, "HEAD"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      narg = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      narg = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
               !kw(&L->cur, "UP") && !kw(&L->cur, "DOWN") &&
               !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
               !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
               !kw(&L->cur, "END") && !kw(&L->cur, "NEW")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        narg = atol(sv->sval);
      else
        narg = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN) {
      narg = parse_expr(vm, L);
    } else {
      fail(vm, "TOPNBYF [Class] field n [ASC|DESC]"); return -1;
    }
    if (narg < 0) narg = 0;
    take = (int)narg;
    if (take > CUBALC_MAX_OBJS) take = CUBALC_MAX_OBJS;
    if (kw(&L->cur, "DESC") || kw(&L->cur, "DOWN") || kw(&L->cur, "REV") ||
        kw(&L->cur, "REVERSE") || kw(&L->cur, "HIGHFIRST") ||
        kw(&L->cur, "TOP") || kw(&L->cur, "BEST")) {
      desc = 1;
      lex_next(L);
    } else if (kw(&L->cur, "ASC") || kw(&L->cur, "UP") ||
               kw(&L->cur, "LOWFIRST") || kw(&L->cur, "WORST") ||
               kw(&L->cur, "BOTTOM")) {
      desc = 0;
      lex_next(L);
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
      snprintf(rows[n].name, sizeof rows[n].name, "%s", ob->name);
      rows[n].v = ob->fnum[fi];
      rows[n].idx = n;
      n++;
    }
    {
      int a, b;
      for (a = 1; a < n; a++) {
        key = rows[a];
        b = a - 1;
        while (b >= 0) {
          int less;
          if (desc)
            less = (key.v > rows[b].v) ||
                   (key.v == rows[b].v && key.idx < rows[b].idx);
          else
            less = (key.v < rows[b].v) ||
                   (key.v == rows[b].v && key.idx < rows[b].idx);
          if (!less) break;
          rows[b + 1] = rows[b];
          b--;
        }
        rows[b + 1] = key;
      }
    }
    out_n = n < take ? n : take;
    for (i = 0; i < out_n; i++) {
      size_t ln = strlen(rows[i].name);
      if (i > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, rows[i].name, ln);
        o += ln;
      }
      bag[o] = 0;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "TOPNBYF", bag);
    var_set_str(vm, "HEADBYF", bag);
    var_set_str(vm, "TAKEBYF", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = out_n;
    var_set_num(vm, "LAST_N", out_n);
    var_set_num(vm, "TOPNBYF_N", out_n);
    var_set_num(vm, "HEADBYF_N", out_n);
    var_set_num(vm, "TAKEBYF_N", out_n);
    var_set_num(vm, "TOPNBYF_TOTAL", n);
    var_set_num(vm, "TOPNBYF_REQ", narg);
    var_set_num(vm, "TOPNBYF_SKIP", n_skip);
    var_set_num(vm, "TOPNBYF_DESC", desc);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* TOPNBYFWHERE|HEADWHERE|TAKEWHERE [Class] sortfield n [WHERE] matchfield matchvalue [ASC|DESC]
   * â top-n matching object names ordered by numeric sortfield.
   * WHERE sugar: TOPNBYFWHERE Cell energy 2 WHERE age 1 DESC.
   * Soft empty â "" / 0. Usability: filtered leaderboard without WHERE+TOPNBYF or SORTBYFWHERE+TAKE. */
  if (kw(&L->cur, "TOPNBYFWHERE") || kw(&L->cur, "HEADWHERE") ||
      kw(&L->cur, "TAKEWHERE") || kw(&L->cur, "FIRSTNWHERE") ||
      kw(&L->cur, "LIMITWHERE") || kw(&L->cur, "TOPNIF") ||
      kw(&L->cur, "HEADIF") || kw(&L->cur, "TAKEIF") ||
      kw(&L->cur, "WHERETOPN") || kw(&L->cur, "TOPNOBJSWHERE") ||
      kw(&L->cur, "HEADBYFWHERE") || kw(&L->cur, "TAKEBYFWHERE")) {
    char filt[48], sfield[48], mfield[48], tok1[48], bag[4096];
    char m_sval[512];
    int has_filt = 0, m_is_str = 0, desc = 0, i, n = 0, n_skip = 0, take = 0, out_n = 0;
    long m_nval = 0, narg = 0;
    size_t o = 0;
    typedef struct { char name[48]; long v; int idx; } TopNByFWRow;
    TopNByFWRow rows[CUBALC_MAX_OBJS];
    TopNByFWRow key;
    lex_next(L);
    filt[0] = 0;
    sfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "TOPNBYFWHERE OF Class sortfield n matchfield matchvalue");
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
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN") &&
          !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
          L->cur.kind != TK_NUM && L->cur.kind != TK_MINUS &&
          L->cur.kind != TK_LPAREN) {
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
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_IDENT ||
             L->cur.kind == TK_STR || kw(&L->cur, "WHERE") ||
             kw(&L->cur, "TOP") || kw(&L->cur, "TAKE") || kw(&L->cur, "N")))
          snprintf(sfield, sizeof sfield, "%s", vv->sval);
        else
          snprintf(sfield, sizeof sfield, "%s", tok1);
      }
    } else {
      fail(vm, "TOPNBYFWHERE [Class] sortfield n matchfield matchvalue");
      return -1;
    }
    if (!sfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(sfield, sizeof sfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") &&
                 !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
                 !kw(&L->cur, "TOP") && !kw(&L->cur, "TAKE") &&
                 !kw(&L->cur, "N") && L->cur.kind != TK_NUM) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(sfield, sizeof sfield, "%s", vv->sval);
        else
          snprintf(sfield, sizeof sfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "TOPNBYFWHERE [Class] sortfield n matchfield matchvalue");
        return -1;
      }
    }
    /* n: optional TOP|TAKE|FIRST|LIMIT|N sugar then count */
    if (kw(&L->cur, "TOP") || kw(&L->cur, "FIRST") || kw(&L->cur, "LIMIT") ||
        kw(&L->cur, "TAKE") || kw(&L->cur, "N") || kw(&L->cur, "COUNT") ||
        kw(&L->cur, "HEAD"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      narg = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      narg = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") &&
               !kw(&L->cur, "WHEN") && !kw(&L->cur, "MATCH") &&
               !kw(&L->cur, "ON") && !kw(&L->cur, "ASC") &&
               !kw(&L->cur, "DESC") && !kw(&L->cur, "ASSERT") &&
               !kw(&L->cur, "LET") && !kw(&L->cur, "PRINT") &&
               !kw(&L->cur, "SYS") && !kw(&L->cur, "END") &&
               !kw(&L->cur, "NEW")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        narg = atol(sv->sval);
      else
        narg = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN) {
      narg = parse_expr(vm, L);
    } else {
      fail(vm, "TOPNBYFWHERE [Class] sortfield n matchfield matchvalue");
      return -1;
    }
    if (narg < 0) narg = 0;
    take = (int)narg;
    if (take > CUBALC_MAX_OBJS) take = CUBALC_MAX_OBJS;
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS") &&
               !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "TOPNBYFWHERE [Class] sortfield n matchfield matchvalue");
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
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
               !kw(&L->cur, "UP") && !kw(&L->cur, "DOWN") &&
               !kw(&L->cur, "HIGHFIRST") && !kw(&L->cur, "LOWFIRST") &&
               !kw(&L->cur, "TOP") && !kw(&L->cur, "BEST") &&
               !kw(&L->cur, "BOTTOM") && !kw(&L->cur, "WORST")) {
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
    if (kw(&L->cur, "DESC") || kw(&L->cur, "DOWN") || kw(&L->cur, "REV") ||
        kw(&L->cur, "REVERSE") || kw(&L->cur, "HIGHFIRST") ||
        kw(&L->cur, "TOP") || kw(&L->cur, "BEST")) {
      desc = 1;
      lex_next(L);
    } else if (kw(&L->cur, "ASC") || kw(&L->cur, "UP") ||
               kw(&L->cur, "LOWFIRST") || kw(&L->cur, "WORST") ||
               kw(&L->cur, "BOTTOM")) {
      desc = 0;
      lex_next(L);
    }
    for (i = 0; i < vm->n_objs && n < CUBALC_MAX_OBJS; i++) {
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
      if (ob->fis_str[sfi]) { n_skip++; continue; }
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
      snprintf(rows[n].name, sizeof rows[n].name, "%s", ob->name);
      rows[n].v = ob->fnum[sfi];
      rows[n].idx = n;
      n++;
    }
    {
      int a, b;
      for (a = 1; a < n; a++) {
        key = rows[a];
        b = a - 1;
        while (b >= 0) {
          int less;
          if (desc)
            less = (key.v > rows[b].v) ||
                   (key.v == rows[b].v && key.idx < rows[b].idx);
          else
            less = (key.v < rows[b].v) ||
                   (key.v == rows[b].v && key.idx < rows[b].idx);
          if (!less) break;
          rows[b + 1] = rows[b];
          b--;
        }
        rows[b + 1] = key;
      }
    }
    out_n = n < take ? n : take;
    for (i = 0; i < out_n; i++) {
      size_t ln = strlen(rows[i].name);
      if (i > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, rows[i].name, ln);
        o += ln;
      }
      bag[o] = 0;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "TOPNBYFWHERE", bag);
    var_set_str(vm, "HEADWHERE", bag);
    var_set_str(vm, "TAKEWHERE", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = out_n;
    var_set_num(vm, "LAST_N", out_n);
    var_set_num(vm, "TOPNBYFWHERE_N", out_n);
    var_set_num(vm, "HEADWHERE_N", out_n);
    var_set_num(vm, "TAKEWHERE_N", out_n);
    var_set_num(vm, "TOPNBYFWHERE_TOTAL", n);
    var_set_num(vm, "TOPNBYFWHERE_REQ", narg);
    var_set_num(vm, "TOPNBYFWHERE_SKIP", n_skip);
    var_set_num(vm, "TOPNBYFWHERE_DESC", desc);
    var_set_str(vm, "FIELD", sfield);
    var_set_str(vm, "SRC", mfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* NTHBYF|ATBYF|INDEXBYF [Class] field index [ASC|DESC]
   * â 0-based rank pick: object name at index in numeric-field order (stable ties).
   * Soft OOB/empty â "" / 0. LAST=name; LAST_N=field value when hit else 0;
   * NTHBYF_TOTAL = ranked pool; NTHBYF_I = requested index; NTHBYF_DESC = 0|1.
   * Usability: 2nd-best / runner-up without SORTBYF + SYS NTH glue. */
  if (kw(&L->cur, "NTHBYF") || kw(&L->cur, "ATBYF") ||
      kw(&L->cur, "INDEXBYF") || kw(&L->cur, "OBJAT") ||
      kw(&L->cur, "NTHOBJ") || kw(&L->cur, "PICKNBYF") ||
      kw(&L->cur, "ATRANK") || kw(&L->cur, "RANKAT") ||
      kw(&L->cur, "ITHBYF") || kw(&L->cur, "KTHBYF")) {
    char filt[48], fname[48], tok1[48], pick[48];
    int has_filt = 0, desc = 0, i, n = 0, n_skip = 0;
    long idx_arg = 0;
    int idx = 0;
    typedef struct { char name[48]; long v; int idx; } NthByFRow;
    NthByFRow rows[CUBALC_MAX_OBJS];
    NthByFRow key;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    pick[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "BY")) {
      if (kw(&L->cur, "BY")) {
        lex_next(L);
      } else {
        lex_next(L);
        if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
          fail(vm, "NTHBYF OF Class field index [ASC|DESC]"); return -1;
        }
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      }
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
          !kw(&L->cur, "UP") && !kw(&L->cur, "DOWN") &&
          L->cur.kind != TK_NUM && L->cur.kind != TK_MINUS &&
          L->cur.kind != TK_LPAREN) {
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
             L->cur.kind == TK_LPAREN ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
              !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC"))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "NTHBYF [Class] field index [ASC|DESC]"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
                 L->cur.kind != TK_NUM) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "NTHBYF [Class] field index [ASC|DESC]"); return -1;
      }
    }
    /* optional sugar: AT i | INDEX i | RANK i | # i */
    if (kw(&L->cur, "AT") || kw(&L->cur, "INDEX") || kw(&L->cur, "RANK") ||
        kw(&L->cur, "POS") || kw(&L->cur, "OFFSET") || kw(&L->cur, "ITH"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      idx_arg = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      idx_arg = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
               !kw(&L->cur, "UP") && !kw(&L->cur, "DOWN") &&
               !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
               !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
               !kw(&L->cur, "END") && !kw(&L->cur, "NEW")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        idx_arg = atol(sv->sval);
      else
        idx_arg = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN) {
      idx_arg = parse_expr(vm, L);
    } else {
      fail(vm, "NTHBYF [Class] field index [ASC|DESC]"); return -1;
    }
    if (kw(&L->cur, "DESC") || kw(&L->cur, "DOWN") || kw(&L->cur, "REV") ||
        kw(&L->cur, "REVERSE") || kw(&L->cur, "HIGHFIRST") ||
        kw(&L->cur, "TOP") || kw(&L->cur, "BEST")) {
      desc = 1;
      lex_next(L);
    } else if (kw(&L->cur, "ASC") || kw(&L->cur, "UP") ||
               kw(&L->cur, "LOWFIRST") || kw(&L->cur, "WORST") ||
               kw(&L->cur, "BOTTOM")) {
      desc = 0;
      lex_next(L);
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
      snprintf(rows[n].name, sizeof rows[n].name, "%s", ob->name);
      rows[n].v = ob->fnum[fi];
      rows[n].idx = n;
      n++;
    }
    {
      int a, b;
      for (a = 1; a < n; a++) {
        key = rows[a];
        b = a - 1;
        while (b >= 0) {
          int less;
          if (desc)
            less = (key.v > rows[b].v) ||
                   (key.v == rows[b].v && key.idx < rows[b].idx);
          else
            less = (key.v < rows[b].v) ||
                   (key.v == rows[b].v && key.idx < rows[b].idx);
          if (!less) break;
          rows[b + 1] = rows[b];
          b--;
        }
        rows[b + 1] = key;
      }
    }
    idx = (int)idx_arg;
    if (idx_arg < 0 || idx_arg >= n) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "NTHBYF", "");
      var_set_str(vm, "ATBYF", "");
      var_set_str(vm, "INDEXBYF", "");
      var_set_str(vm, "OBJECT", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "NTHBYF_N", 0);
      var_set_num(vm, "NTHBYF_VAL", 0);
      var_set_num(vm, "NTHBYF_TOTAL", n);
      var_set_num(vm, "NTHBYF_I", idx_arg);
      var_set_num(vm, "NTHBYF_SKIP", n_skip);
      var_set_num(vm, "NTHBYF_DESC", desc);
      var_set_num(vm, "NTHBYF_HIT", 0);
      var_set_str(vm, "FIELD", fname);
      if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
      var_set_num(vm, "OK", 1);
      bump(vm);
      return 1;
    }
    snprintf(pick, sizeof pick, "%s", rows[idx].name);
    var_set_str(vm, "LAST", pick);
    var_set_str(vm, "NTHBYF", pick);
    var_set_str(vm, "ATBYF", pick);
    var_set_str(vm, "INDEXBYF", pick);
    var_set_str(vm, "OBJECT", pick);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", pick);
    vm->last_n = rows[idx].v;
    var_set_num(vm, "LAST_N", rows[idx].v);
    var_set_num(vm, "NTHBYF_N", 1);
    var_set_num(vm, "NTHBYF_VAL", rows[idx].v);
    var_set_num(vm, "NTHBYF_TOTAL", n);
    var_set_num(vm, "NTHBYF_I", idx_arg);
    var_set_num(vm, "NTHBYF_SKIP", n_skip);
    var_set_num(vm, "NTHBYF_DESC", desc);
    var_set_num(vm, "NTHBYF_HIT", 1);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* NTHBYFWHERE|ATWHERE|INDEXWHERE [Class] sortfield index [WHERE] matchfield matchvalue [ASC|DESC]
   * â 0-based rank pick among field==value matches (stable ties).
   * WHERE sugar: NTHBYFWHERE Cell energy 1 WHERE age 1 DESC.
   * Soft OOB/empty â "" / 0. LAST=name; LAST_N=val when hit; NTHBYFWHERE_TOTAL=match pool.
   * Usability: kth-best in subset without WHERE+NTHBYF or SORTBYFWHERE+NTH glue. */
  if (kw(&L->cur, "NTHBYFWHERE") || kw(&L->cur, "ATWHERE") ||
      kw(&L->cur, "INDEXWHERE") || kw(&L->cur, "NTHWHERE") ||
      kw(&L->cur, "OBJATWHERE") || kw(&L->cur, "NTHOBJWHERE") ||
      kw(&L->cur, "PICKNWHERE") || kw(&L->cur, "ATRANKWHERE") ||
      kw(&L->cur, "RANKATWHERE") || kw(&L->cur, "ITHWHERE") ||
      kw(&L->cur, "KTHWHERE") || kw(&L->cur, "WHERENTH")) {
    char filt[48], sfield[48], mfield[48], tok1[48], pick[48];
    char m_sval[512];
    int has_filt = 0, m_is_str = 0, desc = 0, i, n = 0, n_skip = 0;
    long m_nval = 0, idx_arg = 0;
    int idx = 0;
    typedef struct { char name[48]; long v; int idx; } NthByFWRow;
    NthByFWRow rows[CUBALC_MAX_OBJS];
    NthByFWRow key;
    lex_next(L);
    filt[0] = 0;
    sfield[0] = 0;
    mfield[0] = 0;
    tok1[0] = 0;
    m_sval[0] = 0;
    pick[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "NTHBYFWHERE OF Class sortfield index matchfield matchvalue");
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
          !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") && !kw(&L->cur, "WHEN") &&
          !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
          L->cur.kind != TK_NUM && L->cur.kind != TK_MINUS &&
          L->cur.kind != TK_LPAREN) {
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
            (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
             L->cur.kind == TK_LPAREN || L->cur.kind == TK_IDENT ||
             L->cur.kind == TK_STR || kw(&L->cur, "WHERE") ||
             kw(&L->cur, "AT") || kw(&L->cur, "INDEX") || kw(&L->cur, "RANK")))
          snprintf(sfield, sizeof sfield, "%s", vv->sval);
        else
          snprintf(sfield, sizeof sfield, "%s", tok1);
      }
    } else {
      fail(vm, "NTHBYFWHERE [Class] sortfield index matchfield matchvalue");
      return -1;
    }
    if (!sfield[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(sfield, sizeof sfield, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") &&
                 !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
                 !kw(&L->cur, "AT") && !kw(&L->cur, "INDEX") &&
                 !kw(&L->cur, "RANK") && L->cur.kind != TK_NUM) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(sfield, sizeof sfield, "%s", vv->sval);
        else
          snprintf(sfield, sizeof sfield, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "NTHBYFWHERE [Class] sortfield index matchfield matchvalue");
        return -1;
      }
    }
    /* index: optional AT|INDEX|RANK|POS|OFFSET|ITH (not I/N â steals vars) */
    if (kw(&L->cur, "AT") || kw(&L->cur, "INDEX") || kw(&L->cur, "RANK") ||
        kw(&L->cur, "POS") || kw(&L->cur, "OFFSET") || kw(&L->cur, "ITH"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      idx_arg = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      idx_arg = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "WHERE") && !kw(&L->cur, "IF") &&
               !kw(&L->cur, "WHEN") && !kw(&L->cur, "MATCH") &&
               !kw(&L->cur, "ON") && !kw(&L->cur, "ASC") &&
               !kw(&L->cur, "DESC") && !kw(&L->cur, "ASSERT") &&
               !kw(&L->cur, "LET") && !kw(&L->cur, "PRINT") &&
               !kw(&L->cur, "SYS") && !kw(&L->cur, "END") &&
               !kw(&L->cur, "NEW")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        idx_arg = atol(sv->sval);
      else
        idx_arg = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN) {
      idx_arg = parse_expr(vm, L);
    } else {
      fail(vm, "NTHBYFWHERE [Class] sortfield index matchfield matchvalue");
      return -1;
    }
    if (kw(&L->cur, "WHERE") || kw(&L->cur, "IF") || kw(&L->cur, "WHEN") ||
        kw(&L->cur, "MATCH") || kw(&L->cur, "ON"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "EQ") && !kw(&L->cur, "IS") &&
               !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC")) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mfield, sizeof mfield, "%s", vv->sval);
      else
        snprintf(mfield, sizeof mfield, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "NTHBYFWHERE [Class] sortfield index matchfield matchvalue");
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
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
               !kw(&L->cur, "UP") && !kw(&L->cur, "DOWN") &&
               !kw(&L->cur, "HIGHFIRST") && !kw(&L->cur, "LOWFIRST") &&
               !kw(&L->cur, "TOP") && !kw(&L->cur, "BEST") &&
               !kw(&L->cur, "BOTTOM") && !kw(&L->cur, "WORST")) {
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
    if (kw(&L->cur, "DESC") || kw(&L->cur, "DOWN") || kw(&L->cur, "REV") ||
        kw(&L->cur, "REVERSE") || kw(&L->cur, "HIGHFIRST") ||
        kw(&L->cur, "TOP") || kw(&L->cur, "BEST")) {
      desc = 1;
      lex_next(L);
    } else if (kw(&L->cur, "ASC") || kw(&L->cur, "UP") ||
               kw(&L->cur, "LOWFIRST") || kw(&L->cur, "WORST") ||
               kw(&L->cur, "BOTTOM")) {
      desc = 0;
      lex_next(L);
    }
    for (i = 0; i < vm->n_objs && n < CUBALC_MAX_OBJS; i++) {
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
      if (ob->fis_str[sfi]) { n_skip++; continue; }
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
      snprintf(rows[n].name, sizeof rows[n].name, "%s", ob->name);
      rows[n].v = ob->fnum[sfi];
      rows[n].idx = n;
      n++;
    }
    {
      int a, b;
      for (a = 1; a < n; a++) {
        key = rows[a];
        b = a - 1;
        while (b >= 0) {
          int less;
          if (desc)
            less = (key.v > rows[b].v) ||
                   (key.v == rows[b].v && key.idx < rows[b].idx);
          else
            less = (key.v < rows[b].v) ||
                   (key.v == rows[b].v && key.idx < rows[b].idx);
          if (!less) break;
          rows[b + 1] = rows[b];
          b--;
        }
        rows[b + 1] = key;
      }
    }
    idx = (int)idx_arg;
    if (idx_arg < 0 || idx_arg >= n) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "NTHBYFWHERE", "");
      var_set_str(vm, "ATWHERE", "");
      var_set_str(vm, "INDEXWHERE", "");
      var_set_str(vm, "OBJECT", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "NTHBYFWHERE_N", 0);
      var_set_num(vm, "NTHBYFWHERE_VAL", 0);
      var_set_num(vm, "NTHBYFWHERE_TOTAL", n);
      var_set_num(vm, "NTHBYFWHERE_I", idx_arg);
      var_set_num(vm, "NTHBYFWHERE_SKIP", n_skip);
      var_set_num(vm, "NTHBYFWHERE_DESC", desc);
      var_set_num(vm, "NTHBYFWHERE_HIT", 0);
      var_set_str(vm, "FIELD", sfield);
      var_set_str(vm, "SRC", mfield);
      if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
      var_set_num(vm, "OK", 1);
      bump(vm);
      return 1;
    }
    snprintf(pick, sizeof pick, "%s", rows[idx].name);
    var_set_str(vm, "LAST", pick);
    var_set_str(vm, "NTHBYFWHERE", pick);
    var_set_str(vm, "ATWHERE", pick);
    var_set_str(vm, "INDEXWHERE", pick);
    var_set_str(vm, "OBJECT", pick);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", pick);
    vm->last_n = rows[idx].v;
    var_set_num(vm, "LAST_N", rows[idx].v);
    var_set_num(vm, "NTHBYFWHERE_N", 1);
    var_set_num(vm, "NTHBYFWHERE_VAL", rows[idx].v);
    var_set_num(vm, "NTHBYFWHERE_TOTAL", n);
    var_set_num(vm, "NTHBYFWHERE_I", idx_arg);
    var_set_num(vm, "NTHBYFWHERE_SKIP", n_skip);
    var_set_num(vm, "NTHBYFWHERE_DESC", desc);
    var_set_num(vm, "NTHBYFWHERE_HIT", 1);
    var_set_str(vm, "FIELD", sfield);
    var_set_str(vm, "SRC", mfield);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SLICEBYF|MIDBYF|WINDOWBYF [Class] field start count [ASC|DESC]
   * â ranked name bag window [start, start+count) by numeric field (stable ties).
   * Soft always; start OOB or count<=0 â empty. LAST bag; LAST_N returned count.
   * SLICEBYF_TOTAL = ranked pool; SLICEBYF_START / SLICEBYF_REQ / SLICEBYF_DESC.
   * Usability: page/skip ranked fleet without SORTBYF + DROP + TAKE glue. */
  if (kw(&L->cur, "SLICEBYF") || kw(&L->cur, "MIDBYF") ||
      kw(&L->cur, "WINDOWBYF") || kw(&L->cur, "RANKSLICE") ||
      kw(&L->cur, "SLICEOBJS") || kw(&L->cur, "SKIPTAKEBYF") ||
      kw(&L->cur, "PAGEBYF") || kw(&L->cur, "RANGEBYF") ||
      kw(&L->cur, "SUBRANK") || kw(&L->cur, "SLICEFIELD")) {
    char filt[48], fname[48], tok1[48], bag[4096];
    int has_filt = 0, desc = 0, i, n = 0, n_skip = 0, out_n = 0;
    long start_arg = 0, count_arg = 0;
    int start = 0, take = 0, end = 0;
    size_t o = 0;
    typedef struct { char name[48]; long v; int idx; } SliceByFRow;
    SliceByFRow rows[CUBALC_MAX_OBJS];
    SliceByFRow key;
    lex_next(L);
    filt[0] = 0;
    fname[0] = 0;
    tok1[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "BY")) {
      if (kw(&L->cur, "BY")) {
        lex_next(L);
      } else {
        lex_next(L);
        if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
          fail(vm, "SLICEBYF OF Class field start count [ASC|DESC]"); return -1;
        }
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      }
    } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
      snprintf(tok1, sizeof tok1, "%s", L->cur.text);
      lex_next(L);
      if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
          oop_find_class(vm, tok1) &&
          !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
          !kw(&L->cur, "UP") && !kw(&L->cur, "DOWN") &&
          L->cur.kind != TK_NUM && L->cur.kind != TK_MINUS &&
          L->cur.kind != TK_LPAREN) {
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
             L->cur.kind == TK_LPAREN ||
             (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
              !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC"))))
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", tok1);
      }
    } else {
      fail(vm, "SLICEBYF [Class] field start count [ASC|DESC]"); return -1;
    }
    if (!fname[0]) {
      if (L->cur.kind == TK_STR) {
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
                 !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
                 L->cur.kind != TK_NUM) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(fname, sizeof fname, "%s", vv->sval);
        else
          snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
      } else {
        fail(vm, "SLICEBYF [Class] field start count [ASC|DESC]"); return -1;
      }
    }
    /* optional FROM|START|OFFSET then start */
    if (kw(&L->cur, "FROM") || kw(&L->cur, "START") || kw(&L->cur, "OFFSET") ||
        kw(&L->cur, "SKIP") || kw(&L->cur, "DROP") || kw(&L->cur, "AT"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      start_arg = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      start_arg = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
               !kw(&L->cur, "FOR") && !kw(&L->cur, "COUNT") &&
               !kw(&L->cur, "TAKE") && !kw(&L->cur, "LIMIT") &&
               !kw(&L->cur, "LEN") && !kw(&L->cur, "SIZE") &&
               !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
               !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
               !kw(&L->cur, "END") && !kw(&L->cur, "NEW")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        start_arg = atol(sv->sval);
      else
        start_arg = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN) {
      start_arg = parse_expr(vm, L);
    } else {
      fail(vm, "SLICEBYF [Class] field start count [ASC|DESC]"); return -1;
    }
    /* optional FOR|COUNT|TAKE|LIMIT|LEN then count */
    if (kw(&L->cur, "FOR") || kw(&L->cur, "COUNT") || kw(&L->cur, "TAKE") ||
        kw(&L->cur, "LIMIT") || kw(&L->cur, "LEN") || kw(&L->cur, "SIZE"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      count_arg = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      count_arg = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASC") && !kw(&L->cur, "DESC") &&
               !kw(&L->cur, "UP") && !kw(&L->cur, "DOWN") &&
               !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
               !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
               !kw(&L->cur, "END") && !kw(&L->cur, "NEW")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        count_arg = atol(sv->sval);
      else
        count_arg = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN) {
      count_arg = parse_expr(vm, L);
    } else {
      fail(vm, "SLICEBYF [Class] field start count [ASC|DESC]"); return -1;
    }
    if (kw(&L->cur, "DESC") || kw(&L->cur, "DOWN") || kw(&L->cur, "REV") ||
        kw(&L->cur, "REVERSE") || kw(&L->cur, "HIGHFIRST") ||
        kw(&L->cur, "TOP") || kw(&L->cur, "BEST")) {
      desc = 1;
      lex_next(L);
    } else if (kw(&L->cur, "ASC") || kw(&L->cur, "UP") ||
               kw(&L->cur, "LOWFIRST") || kw(&L->cur, "WORST") ||
               kw(&L->cur, "BOTTOM")) {
      desc = 0;
      lex_next(L);
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
      snprintf(rows[n].name, sizeof rows[n].name, "%s", ob->name);
      rows[n].v = ob->fnum[fi];
      rows[n].idx = n;
      n++;
    }
    {
      int a, b;
      for (a = 1; a < n; a++) {
        key = rows[a];
        b = a - 1;
        while (b >= 0) {
          int less;
          if (desc)
            less = (key.v > rows[b].v) ||
                   (key.v == rows[b].v && key.idx < rows[b].idx);
          else
            less = (key.v < rows[b].v) ||
                   (key.v == rows[b].v && key.idx < rows[b].idx);
          if (!less) break;
          rows[b + 1] = rows[b];
          b--;
        }
        rows[b + 1] = key;
      }
    }
    if (start_arg < 0) start_arg = 0;
    if (count_arg < 0) count_arg = 0;
    start = (int)start_arg;
    take = (int)count_arg;
    if (start > n) start = n;
    end = start + take;
    if (end > n) end = n;
    if (end < start) end = start;
    out_n = end - start;
    for (i = start; i < end; i++) {
      size_t ln = strlen(rows[i].name);
      if (o > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, rows[i].name, ln);
        o += ln;
      }
      bag[o] = 0;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "SLICEBYF", bag);
    var_set_str(vm, "MIDBYF", bag);
    var_set_str(vm, "WINDOWBYF", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = out_n;
    var_set_num(vm, "LAST_N", out_n);
    var_set_num(vm, "SLICEBYF_N", out_n);
    var_set_num(vm, "MIDBYF_N", out_n);
    var_set_num(vm, "WINDOWBYF_N", out_n);
    var_set_num(vm, "SLICEBYF_TOTAL", n);
    var_set_num(vm, "SLICEBYF_START", start_arg);
    var_set_num(vm, "SLICEBYF_REQ", count_arg);
    var_set_num(vm, "SLICEBYF_SKIP", n_skip);
    var_set_num(vm, "SLICEBYF_DESC", desc);
    var_set_str(vm, "FIELD", fname);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SUMF|SUMFALL|TOTALF [Class] field
   * AVGF|AVGFALL|MEANF [Class] field
   * â sum or integer mean of numeric field over live objects (optional class).
   * LAST_N = result; SUMF_N/AVGF_N = sample count; soft empty â 0.
   * Usability: fleet health without GETFALL + SYS SUM/AVG glue. */
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
