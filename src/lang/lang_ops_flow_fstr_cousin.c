/* CubalC lang — lang_ops_flow_fstr_cousin.c
 * FLOW fstr cousins: COUSINCOUNT · ARECOUSINS · COUSINS · NTHCOUSIN
 * Peel from fstr_b (B_fstr_b_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_flow_fstr_cousin.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_fstr_cousin(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* COUSINCOUNT + HASCOUSIN (multifile EXTEND)
   * COUSINCOUNT Class|obj — count of classes that share the same grandparent
   *   via a different parent (first-cousin peers on EXTEND lattice). Excludes self.
   * HASCOUSIN Class|obj — 1 if COUSINCOUNT > 0, else 0.
   * Soft 0 for roots / only-child-of-root / unknown. Complements ARESIBLINGS +
   * SIBLINGCOUNT + COMMONANCESTOR. Cube is SoT. Free energy must flow. */
  if (kw(&L->cur, "COUSINCOUNT") || kw(&L->cur, "COUNTCOUSINS") ||
      kw(&L->cur, "NUMCOUSINS") || kw(&L->cur, "NCOUSINS") ||
      kw(&L->cur, "COUSIN_COUNT") || kw(&L->cur, "PEERCOUSINCOUNT") ||
      kw(&L->cur, "HASCOUSIN") || kw(&L->cur, "HASCOUSINS") ||
      kw(&L->cur, "HAS_COUSIN") || kw(&L->cur, "HAS_COUSINS") ||
      kw(&L->cur, "HASCOUSIN_P") || kw(&L->cur, "COUSINED_P")) {
    int want_cnt = kw(&L->cur, "COUSINCOUNT") || kw(&L->cur, "COUNTCOUSINS") ||
                   kw(&L->cur, "NUMCOUSINS") || kw(&L->cur, "NCOUSINS") ||
                   kw(&L->cur, "COUSIN_COUNT") || kw(&L->cur, "PEERCOUSINCOUNT");
    char a[48];
    ClassDef *cda = NULL;
    ObjInst *ob;
    int ai = -1, i, ncos = 0, flag = 0;
    int pa = -1, gpa = -1;
    const char *opname = want_cnt ? "COUSINCOUNT" : "HASCOUSIN";
    lex_next(L);
    a[0] = 0;
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, want_cnt ? "COUSINCOUNT Class|obj" : "HASCOUSIN Class|obj");
      return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(a, sizeof a, "%s", L->cur.text);
      lex_next(L);
    } else {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      if (oop_find_obj(vm, id) || oop_find_class(vm, id)) {
        snprintf(a, sizeof a, "%s", id);
      } else {
        vv = var_get(vm, id, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(a, sizeof a, "%s", vv->sval);
        else
          snprintf(a, sizeof a, "%s", id);
      }
    }
    ob = oop_find_obj(vm, a);
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes) {
      cda = &vm->classes[ob->class_idx];
      ai = ob->class_idx;
    } else {
      cda = oop_find_class(vm, a);
      if (cda) ai = (int)(cda - vm->classes);
    }
    ncos = 0;
    if (cda && ai >= 0) {
      pa = cda->parent_idx;
      if (pa >= 0 && pa < vm->n_classes) {
        gpa = vm->classes[pa].parent_idx;
        if (gpa >= 0 && gpa < vm->n_classes) {
          for (i = 0; i < vm->n_classes; i++) {
            int p2;
            if (i == ai) continue;
            p2 = vm->classes[i].parent_idx;
            if (p2 < 0 || p2 >= vm->n_classes) continue;
            if (p2 == pa) continue;
            if (vm->classes[p2].parent_idx != gpa) continue;
            ncos++;
          }
        }
      }
    }
    flag = ncos > 0 ? 1 : 0;
    if (want_cnt) {
      var_set_num(vm, "LAST_N", ncos);
      vm->last_n = ncos;
      snprintf(vm->last_str, sizeof vm->last_str, "%d", ncos);
      var_set_str(vm, "LAST", vm->last_str);
      var_set_num(vm, "COUSINCOUNT", ncos);
      var_set_num(vm, "COUSINCOUNT_N", ncos);
      var_set_num(vm, "COUNTCOUSINS_N", ncos);
      var_set_num(vm, "NUMCOUSINS_N", ncos);
    } else {
      var_set_num(vm, "LAST_N", flag);
      vm->last_n = flag;
      snprintf(vm->last_str, sizeof vm->last_str, "%d", flag);
      var_set_str(vm, "LAST", vm->last_str);
      var_set_num(vm, "HASCOUSIN", flag);
      var_set_num(vm, "HASCOUSIN_N", flag);
      var_set_num(vm, "HASCOUSINS_N", flag);
    }
    var_set_num(vm, "COUSINCOUNT_N", ncos);
    var_set_num(vm, "HASCOUSIN_N", flag);
    var_set_num(vm, "OK", cda ? 1 : 0);
    if (cda) var_set_str(vm, "CLASS", cda->name);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s -> ncos=%d flag=%d\n", opname, a, ncos, flag);
    bump(vm);
    return 1;
  }

  /* ARECOUSINS + ISCOUSIN/COUSINOF (multifile EXTEND)
   * ARECOUSINS|ISCOUSIN|COUSINOF Class|obj Class|obj
   * — soft 0|1 if both resolve, distinct, different parents, and those parents
   * share the same parent_idx (first cousins on EXTEND lattice).
   * Self / siblings / roots / unknown → 0. Complements COUSINCOUNT + HASCOUSIN +
   * ARESIBLINGS + COMMONANCESTOR. Cube is SoT. Free energy must flow. */
  if (kw(&L->cur, "ARECOUSINS") || kw(&L->cur, "ARE_COUSINS") ||
      kw(&L->cur, "ISCOUSIN") || kw(&L->cur, "IS_COUSIN") ||
      kw(&L->cur, "COUSINOF") || kw(&L->cur, "COUSIN_OF") ||
      kw(&L->cur, "ISCOUSINOF") || kw(&L->cur, "IS_COUSIN_OF") ||
      kw(&L->cur, "SHAREGRANDPARENT_P") || kw(&L->cur, "SAMEGRAND_P") ||
      kw(&L->cur, "COCOUSIN") || kw(&L->cur, "CO_COUSIN")) {
    char a[48], b[48];
    ClassDef *cda = NULL, *cdb = NULL;
    ObjInst *ob;
    int ai = -1, bi = -1, hit = 0;
    int pa = -1, pb = -1, ga = -1, gb = -1;
    const char *opname = "ARECOUSINS";
    lex_next(L);
    a[0] = b[0] = 0;
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "ARECOUSINS Class|obj Class|obj");
      return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(a, sizeof a, "%s", L->cur.text);
      lex_next(L);
    } else {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      if (oop_find_obj(vm, id) || oop_find_class(vm, id)) {
        snprintf(a, sizeof a, "%s", id);
      } else {
        vv = var_get(vm, id, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(a, sizeof a, "%s", vv->sval);
        else
          snprintf(a, sizeof a, "%s", id);
      }
    }
    if (kw(&L->cur, "OF") || kw(&L->cur, "AND") || kw(&L->cur, "WITH") ||
        kw(&L->cur, "IS") || kw(&L->cur, "TO") || kw(&L->cur, "VS"))
      lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "ARECOUSINS Class|obj Class|obj");
      return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(b, sizeof b, "%s", L->cur.text);
      lex_next(L);
    } else {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      if (oop_find_obj(vm, id) || oop_find_class(vm, id)) {
        snprintf(b, sizeof b, "%s", id);
      } else {
        vv = var_get(vm, id, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(b, sizeof b, "%s", vv->sval);
        else
          snprintf(b, sizeof b, "%s", id);
      }
    }
    ob = oop_find_obj(vm, a);
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes) {
      cda = &vm->classes[ob->class_idx];
      ai = ob->class_idx;
    } else {
      cda = oop_find_class(vm, a);
      if (cda) ai = (int)(cda - vm->classes);
    }
    ob = oop_find_obj(vm, b);
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes) {
      cdb = &vm->classes[ob->class_idx];
      bi = ob->class_idx;
    } else {
      cdb = oop_find_class(vm, b);
      if (cdb) bi = (int)(cdb - vm->classes);
    }
    hit = 0;
    if (cda && cdb && ai >= 0 && bi >= 0 && ai != bi) {
      pa = cda->parent_idx;
      pb = cdb->parent_idx;
      if (pa >= 0 && pa < vm->n_classes && pb >= 0 && pb < vm->n_classes &&
          pa != pb) {
        ga = vm->classes[pa].parent_idx;
        gb = vm->classes[pb].parent_idx;
        if (ga >= 0 && ga < vm->n_classes && ga == gb)
          hit = 1;
      }
    }
    var_set_num(vm, "LAST_N", hit ? 1 : 0);
    vm->last_n = hit ? 1 : 0;
    snprintf(vm->last_str, sizeof vm->last_str, "%d", hit ? 1 : 0);
    var_set_str(vm, "LAST", vm->last_str);
    var_set_num(vm, "ARECOUSINS_N", hit ? 1 : 0);
    var_set_num(vm, "ARECOUSINS", hit ? 1 : 0);
    var_set_num(vm, "ISCOUSIN_N", hit ? 1 : 0);
    var_set_num(vm, "ISCOUSIN", hit ? 1 : 0);
    var_set_num(vm, "COUSINOF_N", hit ? 1 : 0);
    var_set_num(vm, "OK", (cda && cdb) ? 1 : 0);
    if (cda) var_set_str(vm, "CLASS", cda->name);
    if (cdb) var_set_str(vm, "PEER", cdb->name);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s %s -> %d\n", opname, a, b, hit);
    bump(vm);
    return 1;
  }

  /* COUSINS|COUSINBAG|LISTCOUSINS|PEERCOUSINS Class|obj
   * — newline bag of first cousins (same grandparent, different parents).
   * FIRSTCOUSIN|HEADCOUSIN|ELDESTCOUSIN Class|obj — first cousin name (may empty).
   * LASTCOUSIN|TAILCOUSIN|YOUNGESTCOUSIN Class|obj — last cousin name (may empty).
   * Soft empty OK=1 when root/shallow/unknown/no cousins. LAST=bag|name; LAST_N=count|0|1.
   * Complements ARECOUSINS/COUSINCOUNT/HASCOUSIN + SIBLINGS bag without scrape.
   * Multi-file CLASS/EXTEND usability. Cube Law: free energy must flow. */
  if (kw(&L->cur, "COUSINS") || kw(&L->cur, "COUSINBAG") ||
      kw(&L->cur, "LISTCOUSINS") || kw(&L->cur, "PEERCOUSINS") ||
      kw(&L->cur, "COUSINS_OF") || kw(&L->cur, "LIST_COUSINS") ||
      kw(&L->cur, "COUSINLIST") || kw(&L->cur, "ALLCOUSINS") ||
      kw(&L->cur, "FIRSTCOUSIN") || kw(&L->cur, "HEADCOUSIN") ||
      kw(&L->cur, "ELDESTCOUSIN") || kw(&L->cur, "FIRST_COUSIN") ||
      kw(&L->cur, "HEAD_COUSIN") || kw(&L->cur, "STARTCOUSIN") ||
      kw(&L->cur, "LASTCOUSIN") || kw(&L->cur, "TAILCOUSIN") ||
      kw(&L->cur, "YOUNGESTCOUSIN") || kw(&L->cur, "LAST_COUSIN") ||
      kw(&L->cur, "TAIL_COUSIN") || kw(&L->cur, "ENDCOUSIN")) {
    int want_first =
        kw(&L->cur, "FIRSTCOUSIN") || kw(&L->cur, "HEADCOUSIN") ||
        kw(&L->cur, "ELDESTCOUSIN") || kw(&L->cur, "FIRST_COUSIN") ||
        kw(&L->cur, "HEAD_COUSIN") || kw(&L->cur, "STARTCOUSIN");
    int want_last =
        kw(&L->cur, "LASTCOUSIN") || kw(&L->cur, "TAILCOUSIN") ||
        kw(&L->cur, "YOUNGESTCOUSIN") || kw(&L->cur, "LAST_COUSIN") ||
        kw(&L->cur, "TAIL_COUSIN") || kw(&L->cur, "ENDCOUSIN");
    int want_bag = !want_first && !want_last;
    char a[48], bag[2048];
    char first_nm[48], last_nm[48];
    ClassDef *cd = NULL;
    ObjInst *ob;
    size_t o = 0;
    int n = 0, ci, self_i = -1, parent_i = -1, gparent_i = -1;
    const char *wh = "COUSINS";
    lex_next(L);
    a[0] = 0;
    bag[0] = 0;
    first_nm[0] = 0;
    last_nm[0] = 0;
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, want_first ? "FIRSTCOUSIN Class|obj"
                           : want_last ? "LASTCOUSIN Class|obj"
                                       : "COUSINS Class|obj");
      return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(a, sizeof a, "%s", L->cur.text);
      lex_next(L);
    } else {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      if (oop_find_obj(vm, id) || oop_find_class(vm, id))
        snprintf(a, sizeof a, "%s", id);
      else {
        vv = var_get(vm, id, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(a, sizeof a, "%s", vv->sval);
        else
          snprintf(a, sizeof a, "%s", id);
      }
    }
    ob = oop_find_obj(vm, a);
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes)
      cd = &vm->classes[ob->class_idx];
    else
      cd = oop_find_class(vm, a);
    if (cd) self_i = (int)(cd - vm->classes);
    if (cd && self_i >= 0) parent_i = cd->parent_idx;
    if (parent_i >= 0 && parent_i < vm->n_classes)
      gparent_i = vm->classes[parent_i].parent_idx;
    if (cd && self_i >= 0 && parent_i >= 0 && gparent_i >= 0 &&
        gparent_i < vm->n_classes) {
      for (ci = 0; ci < vm->n_classes; ci++) {
        int p, gp;
        ClassDef *c;
        const char *nm;
        size_t ln;
        if (ci == self_i) continue;
        c = &vm->classes[ci];
        p = c->parent_idx;
        if (p < 0 || p >= vm->n_classes) continue;
        if (p == parent_i) continue; /* sibling, not cousin */
        gp = vm->classes[p].parent_idx;
        if (gp != gparent_i) continue;
        nm = c->name;
        ln = strlen(nm);
        if (!ln) continue;
        if (!first_nm[0]) snprintf(first_nm, sizeof first_nm, "%s", nm);
        snprintf(last_nm, sizeof last_nm, "%s", nm);
        if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + ln < sizeof bag) {
          memcpy(bag + o, nm, ln);
          o += ln;
          bag[o] = 0;
          n++;
        }
      }
    }
    if (want_first) wh = "FIRSTCOUSIN";
    else if (want_last) wh = "LASTCOUSIN";
    else wh = "COUSINS";
    if (want_bag) {
      var_set_str(vm, "LAST", bag);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
      var_set_num(vm, "LAST_N", n);
      vm->last_n = n;
      var_set_str(vm, "COUSINS", bag);
      var_set_str(vm, "COUSINBAG", bag);
      var_set_str(vm, "LISTCOUSINS", bag);
      var_set_str(vm, "PEERCOUSINS", bag);
      var_set_num(vm, "COUSINS_N", n);
      var_set_num(vm, "COUSINBAG_N", n);
      var_set_num(vm, "LISTCOUSINS_N", n);
      var_set_num(vm, "PEERCOUSINS_N", n);
      var_set_num(vm, "COUSINCOUNT", n);
      var_set_num(vm, "NCOUSINS", n);
      var_set_num(vm, "NUMCOUSINS", n);
      var_set_num(vm, "HASCOUSIN", n > 0 ? 1 : 0);
      var_set_str(vm, "FIRSTCOUSIN", first_nm);
      var_set_str(vm, "LASTCOUSIN", last_nm);
      var_set_num(vm, "FIRSTCOUSIN_N", first_nm[0] ? 1 : 0);
      var_set_num(vm, "LASTCOUSIN_N", last_nm[0] ? 1 : 0);
    } else if (want_first) {
      var_set_str(vm, "LAST", first_nm);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", first_nm);
      var_set_num(vm, "LAST_N", first_nm[0] ? 1 : 0);
      vm->last_n = first_nm[0] ? 1 : 0;
      var_set_str(vm, "FIRSTCOUSIN", first_nm);
      var_set_str(vm, "HEADCOUSIN", first_nm);
      var_set_str(vm, "ELDESTCOUSIN", first_nm);
      var_set_num(vm, "FIRSTCOUSIN_N", first_nm[0] ? 1 : 0);
      var_set_str(vm, "COUSINS", bag);
      var_set_num(vm, "COUSINS_N", n);
      var_set_num(vm, "COUSINCOUNT", n);
      var_set_str(vm, "LASTCOUSIN", last_nm);
    } else {
      var_set_str(vm, "LAST", last_nm);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", last_nm);
      var_set_num(vm, "LAST_N", last_nm[0] ? 1 : 0);
      vm->last_n = last_nm[0] ? 1 : 0;
      var_set_str(vm, "LASTCOUSIN", last_nm);
      var_set_str(vm, "TAILCOUSIN", last_nm);
      var_set_str(vm, "YOUNGESTCOUSIN", last_nm);
      var_set_num(vm, "LASTCOUSIN_N", last_nm[0] ? 1 : 0);
      var_set_str(vm, "COUSINS", bag);
      var_set_num(vm, "COUSINS_N", n);
      var_set_num(vm, "COUSINCOUNT", n);
      var_set_str(vm, "FIRSTCOUSIN", first_nm);
    }
    if (cd) var_set_str(vm, "CLASS", cd->name);
    if (parent_i >= 0 && parent_i < vm->n_classes)
      var_set_str(vm, "PARENT", vm->classes[parent_i].name);
    else
      var_set_str(vm, "PARENT", "");
    if (gparent_i >= 0 && gparent_i < vm->n_classes)
      var_set_str(vm, "GRANDPARENT", vm->classes[gparent_i].name);
    else
      var_set_str(vm, "GRANDPARENT", "");
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s -> n=%d first=%s last=%s gp=%d\n", wh, a, n,
              first_nm[0] ? first_nm : "-", last_nm[0] ? last_nm : "-",
              gparent_i);
    bump(vm);
    return 1;
  }
  /* NTHCOUSIN|COUSINAT|COUSIN_AT (multifile EXTEND)
   * NTHCOUSIN Class|obj N — first-cousin name at 0-based index (declaration order).
   * Soft empty LAST="" LAST_N=0 on OOB / root / no cousins. OK=1 always when parse ok.
   * Complements COUSINS bag + FIRSTCOUSIN + LASTCOUSIN + COUSINCOUNT + ARECOUSINS.
   * Cube is SoT. Free energy must flow. */
  if (kw(&L->cur, "NTHCOUSIN") || kw(&L->cur, "COUSINAT") ||
      kw(&L->cur, "NTH_COUSIN") || kw(&L->cur, "COUSIN_AT") ||
      kw(&L->cur, "COUSIN_NTH") || kw(&L->cur, "PEERCOUSINAT") ||
      kw(&L->cur, "NTHPEERCOUSIN") || kw(&L->cur, "COUSININDEX")) {
    char a[48], pick[48];
    ClassDef *cda = NULL;
    ObjInst *ob;
    int ai = -1, i, idx_want = -1, hit = 0;
    int pa = -1, gpa = -1;
    int cous[512];
    int nc = 0;
    const char *opname = "NTHCOUSIN";
    lex_next(L);
    a[0] = 0;
    pick[0] = 0;
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "NTHCOUSIN Class|obj N");
      return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(a, sizeof a, "%s", L->cur.text);
      lex_next(L);
    } else {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      if (oop_find_obj(vm, id) || oop_find_class(vm, id)) {
        snprintf(a, sizeof a, "%s", id);
      } else {
        vv = var_get(vm, id, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(a, sizeof a, "%s", vv->sval);
        else
          snprintf(a, sizeof a, "%s", id);
      }
    }
    if (kw(&L->cur, "AT") || kw(&L->cur, "INDEX") || kw(&L->cur, "N") ||
        kw(&L->cur, "OF") || kw(&L->cur, "POS"))
      lex_next(L);
    if (L->cur.kind == TK_NUM) {
      idx_want = (int)L->cur.num;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && !vv->is_str) idx_want = (int)vv->val;
      else { fail(vm, "NTHCOUSIN Class|obj N"); return -1; }
      lex_next(L);
    } else { fail(vm, "NTHCOUSIN Class|obj N"); return -1; }
    ob = oop_find_obj(vm, a);
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes) {
      cda = &vm->classes[ob->class_idx];
      ai = ob->class_idx;
    } else {
      cda = oop_find_class(vm, a);
      if (cda) ai = (int)(cda - vm->classes);
    }
    nc = 0;
    if (cda && ai >= 0) {
      pa = cda->parent_idx;
      if (pa >= 0 && pa < vm->n_classes) {
        gpa = vm->classes[pa].parent_idx;
        if (gpa >= 0 && gpa < vm->n_classes) {
          for (i = 0; i < vm->n_classes; i++) {
            int p2;
            if (i == ai) continue;
            p2 = vm->classes[i].parent_idx;
            if (p2 < 0 || p2 >= vm->n_classes) continue;
            if (p2 == pa) continue;
            if (vm->classes[p2].parent_idx != gpa) continue;
            if (nc < 512) cous[nc++] = i;
          }
        }
      }
    }
    hit = 0;
    pick[0] = 0;
    if (idx_want >= 0 && idx_want < nc) {
      snprintf(pick, sizeof pick, "%s", vm->classes[cous[idx_want]].name);
      hit = 1;
    }
    var_set_str(vm, "LAST", pick);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", pick);
    var_set_num(vm, "LAST_N", hit ? 1 : 0);
    vm->last_n = hit ? 1 : 0;
    var_set_str(vm, "COUSIN", pick);
    var_set_str(vm, "NTHCOUSIN", pick);
    var_set_str(vm, "COUSINAT", pick);
    var_set_num(vm, "NTHCOUSIN_N", hit ? 1 : 0);
    var_set_num(vm, "COUSINAT_N", hit ? 1 : 0);
    var_set_num(vm, "COUSINS_N", nc);
    var_set_num(vm, "COUSINCOUNT_N", nc);
    if (cda) var_set_str(vm, "CLASS", cda->name);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s %d -> hit=%d cous=%d\n", opname, a, idx_want, hit, nc);
    bump(vm);
    return 1;
  }

  return 0;
}
