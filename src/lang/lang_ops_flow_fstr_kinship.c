/* CubalC lang — lang_ops_flow_fstr_kinship.c
 * FLOW fstr kinship: CHILDCOUNT · NTHSIBLING · NEXTSIBLING · ISROOT · SIBLINGCOUNT · ARESIBLINGS
 * Peel from fstr_b (B_fstr_b_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_flow_fstr_kinship.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_fstr_kinship(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  if (kw(&L->cur, "CHILDCOUNT") || kw(&L->cur, "COUNTCHILDREN") ||
      kw(&L->cur, "NUMCHILDREN") || kw(&L->cur, "NCHILDREN") ||
      kw(&L->cur, "KIDCOUNT") || kw(&L->cur, "DIRECTKIDS") ||
      kw(&L->cur, "CHILD_COUNT") || kw(&L->cur, "COUNT_CHILDREN") ||
      kw(&L->cur, "DESCENDANTCOUNT") || kw(&L->cur, "COUNTDESCENDANTS") ||
      kw(&L->cur, "NUMDESCENDANTS") || kw(&L->cur, "NDESCENDANTS") ||
      kw(&L->cur, "PROGENYCOUNT") || kw(&L->cur, "DESCENDANT_COUNT") ||
      kw(&L->cur, "COUNT_DESCENDANTS") || kw(&L->cur, "ALLSUBSCOUNT") ||
      kw(&L->cur, "SUBTREESIZE") || kw(&L->cur, "TREESIZE") ||
      kw(&L->cur, "NODESIN") || kw(&L->cur, "SUBTREE_SIZE") ||
      kw(&L->cur, "TREE_SIZE") || kw(&L->cur, "COUNTNODES") ||
      kw(&L->cur, "SUBTREECARD") || kw(&L->cur, "CLASSTREE_N")) {
    int want_childc = kw(&L->cur, "CHILDCOUNT") || kw(&L->cur, "COUNTCHILDREN") ||
                      kw(&L->cur, "NUMCHILDREN") || kw(&L->cur, "NCHILDREN") ||
                      kw(&L->cur, "KIDCOUNT") || kw(&L->cur, "DIRECTKIDS") ||
                      kw(&L->cur, "CHILD_COUNT") || kw(&L->cur, "COUNT_CHILDREN");
    int want_descc = kw(&L->cur, "DESCENDANTCOUNT") || kw(&L->cur, "COUNTDESCENDANTS") ||
                     kw(&L->cur, "NUMDESCENDANTS") || kw(&L->cur, "NDESCENDANTS") ||
                     kw(&L->cur, "PROGENYCOUNT") || kw(&L->cur, "DESCENDANT_COUNT") ||
                     kw(&L->cur, "COUNT_DESCENDANTS") || kw(&L->cur, "ALLSUBSCOUNT");
    int want_sub = kw(&L->cur, "SUBTREESIZE") || kw(&L->cur, "TREESIZE") ||
                   kw(&L->cur, "NODESIN") || kw(&L->cur, "SUBTREE_SIZE") ||
                   kw(&L->cur, "TREE_SIZE") || kw(&L->cur, "COUNTNODES") ||
                   kw(&L->cur, "SUBTREECARD") || kw(&L->cur, "CLASSTREE_N");
    char a[48];
    ClassDef *cda = NULL;
    ObjInst *ob;
    int ai = -1, i, n = 0, guard = 0;
    const char *opname = want_childc ? "CHILDCOUNT"
                         : want_descc ? "DESCENDANTCOUNT"
                                        : "SUBTREESIZE";
    lex_next(L);
    a[0] = 0;
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, want_childc ? "CHILDCOUNT Class|obj"
                         : (want_descc ? "DESCENDANTCOUNT Class|obj"
                                       : "SUBTREESIZE Class|obj"));
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
    if (cda && ai >= 0) {
      if (want_childc) {
        for (i = 0; i < vm->n_classes; i++) {
          if (vm->classes[i].parent_idx == ai) n++;
        }
      } else {
        /* BFS descendants; subtree = 1 + descendants */
        int queue[512];
        int qh = 0, qt = 0;
        int seen[512];
        int ndesc = 0;
        for (i = 0; i < 512; i++) seen[i] = 0;
        queue[qt++] = ai;
        seen[ai] = 1;
        guard = 0;
        while (qh < qt && guard++ < CUBALC_MAX_CLASSES * 4) {
          int cur = queue[qh++];
          if (cur < 0 || cur >= vm->n_classes) continue;
          for (i = 0; i < vm->n_classes; i++) {
            if (vm->classes[i].parent_idx == cur && !seen[i] && qt < 512) {
              seen[i] = 1;
              queue[qt++] = i;
              ndesc++;
            }
          }
        }
        n = want_descc ? ndesc : (ndesc + 1);
      }
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    if (want_childc) {
      var_set_num(vm, "CHILDCOUNT_N", n);
      var_set_num(vm, "COUNTCHILDREN_N", n);
      var_set_num(vm, "NUMCHILDREN_N", n);
      var_set_num(vm, "NCHILDREN_N", n);
      var_set_num(vm, "KIDCOUNT_N", n);
    } else if (want_descc) {
      var_set_num(vm, "DESCENDANTCOUNT_N", n);
      var_set_num(vm, "COUNTDESCENDANTS_N", n);
      var_set_num(vm, "NUMDESCENDANTS_N", n);
      var_set_num(vm, "NDESCENDANTS_N", n);
      var_set_num(vm, "PROGENYCOUNT_N", n);
      var_set_num(vm, "DESCENDANTCOUNT", n);
    } else {
      var_set_num(vm, "SUBTREESIZE_N", n);
      var_set_num(vm, "TREESIZE_N", n);
      var_set_num(vm, "NODESIN_N", n);
      var_set_num(vm, "SUBTREESIZE", n);
      var_set_num(vm, "TREESIZE", n);
    }
    /* also mirror common count aliases for bag duals */
    if (cda) var_set_str(vm, "CLASS", cda->name);
    var_set_num(vm, "OK", cda ? 1 : 0);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s -> n=%d\n", opname, a, n);
    bump(vm);
    return 1;
  }


  /* NTHSIBLING|FIRSTSIBLING|LASTSIBLING (multifile EXTEND)
   * FIRSTSIBLING / LASTSIBLING Class|obj — first/last peer sharing parent_idx (excl self).
   * NTHSIBLING Class|obj N — sibling at index N (0-based, declaration order); soft empty OOB.
   * Complements SIBLINGS bag + NTHCHILD + NTHPARENT. Cube is SoT. Free energy must flow. */
  if (kw(&L->cur, "NTHSIBLING") || kw(&L->cur, "SIBLINGAT") ||
      kw(&L->cur, "NTH_SIBLING") || kw(&L->cur, "SIBLING_AT") ||
      kw(&L->cur, "PEERAT") || kw(&L->cur, "NTHPEER") ||
      kw(&L->cur, "FIRSTSIBLING") || kw(&L->cur, "ELDERSIBLING") ||
      kw(&L->cur, "OLDESTSIBLING") || kw(&L->cur, "FIRST_SIBLING") ||
      kw(&L->cur, "FIRSTPEER") || kw(&L->cur, "ELDERPEER") ||
      kw(&L->cur, "LASTSIBLING") || kw(&L->cur, "YOUNGSIBLING") ||
      kw(&L->cur, "NEWESTSIBLING") || kw(&L->cur, "LAST_SIBLING") ||
      kw(&L->cur, "YOUNGESTSIBLING") || kw(&L->cur, "LASTPEER") ||
      kw(&L->cur, "YOUNGPEER")) {
    int want_nth = kw(&L->cur, "NTHSIBLING") || kw(&L->cur, "SIBLINGAT") ||
                   kw(&L->cur, "NTH_SIBLING") || kw(&L->cur, "SIBLING_AT") ||
                   kw(&L->cur, "PEERAT") || kw(&L->cur, "NTHPEER");
    int want_first = kw(&L->cur, "FIRSTSIBLING") || kw(&L->cur, "ELDERSIBLING") ||
                     kw(&L->cur, "OLDESTSIBLING") || kw(&L->cur, "FIRST_SIBLING") ||
                     kw(&L->cur, "FIRSTPEER") || kw(&L->cur, "ELDERPEER");
    int want_last = kw(&L->cur, "LASTSIBLING") || kw(&L->cur, "YOUNGSIBLING") ||
                    kw(&L->cur, "NEWESTSIBLING") || kw(&L->cur, "LAST_SIBLING") ||
                    kw(&L->cur, "YOUNGESTSIBLING") || kw(&L->cur, "LASTPEER") ||
                    kw(&L->cur, "YOUNGPEER");
    char a[48], pick[48];
    ClassDef *cda = NULL;
    ObjInst *ob;
    int ai = -1, i, n = 0, idx_want = -1, hit = 0;
    int sibs[512];
    int ns = 0;
    const char *opname = want_nth ? "NTHSIBLING"
                         : want_first ? "FIRSTSIBLING"
                                      : "LASTSIBLING";
    lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, want_nth ? "NTHSIBLING Class|obj N"
                        : (want_first ? "FIRSTSIBLING Class|obj"
                                      : "LASTSIBLING Class|obj"));
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
    if (want_nth) {
      if (kw(&L->cur, "AT") || kw(&L->cur, "INDEX") || kw(&L->cur, "N") ||
          kw(&L->cur, "OF") || kw(&L->cur, "POS"))
        lex_next(L);
      if (L->cur.kind == TK_NUM) {
        idx_want = (int)L->cur.num;
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && !vv->is_str) idx_want = (int)vv->val;
        else { fail(vm, "NTHSIBLING Class|obj N"); return -1; }
        lex_next(L);
      } else { fail(vm, "NTHSIBLING Class|obj N"); return -1; }
    }
    pick[0] = 0;
    ob = oop_find_obj(vm, a);
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes) {
      cda = &vm->classes[ob->class_idx];
      ai = ob->class_idx;
    } else {
      cda = oop_find_class(vm, a);
      if (cda) ai = (int)(cda - vm->classes);
    }
    ns = 0;
    if (cda && ai >= 0) {
      int par = cda->parent_idx;
      if (par >= 0 && par < vm->n_classes) {
        for (i = 0; i < vm->n_classes; i++) {
          if (i == ai) continue;
          if (vm->classes[i].parent_idx == par) {
            if (ns < 512) sibs[ns++] = i;
          }
        }
      }
    }
    hit = 0;
    if (want_first && ns > 0) {
      snprintf(pick, sizeof pick, "%s", vm->classes[sibs[0]].name);
      hit = 1;
    } else if (want_last && ns > 0) {
      snprintf(pick, sizeof pick, "%s", vm->classes[sibs[ns - 1]].name);
      hit = 1;
    } else if (want_nth && idx_want >= 0 && idx_want < ns) {
      snprintf(pick, sizeof pick, "%s", vm->classes[sibs[idx_want]].name);
      hit = 1;
    }
    var_set_str(vm, "LAST", pick);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", pick);
    var_set_num(vm, "LAST_N", hit ? 1 : 0);
    vm->last_n = hit ? 1 : 0;
    var_set_str(vm, "SIBLING", pick);
    var_set_str(vm, "NTHSIBLING", pick);
    var_set_str(vm, "FIRSTSIBLING", pick);
    var_set_str(vm, "LASTSIBLING", pick);
    var_set_str(vm, "SIBLINGAT", pick);
    var_set_num(vm, "NTHSIBLING_N", hit ? 1 : 0);
    var_set_num(vm, "FIRSTSIBLING_N", hit ? 1 : 0);
    var_set_num(vm, "LASTSIBLING_N", hit ? 1 : 0);
    var_set_num(vm, "SIBLINGS_N", ns);
    var_set_num(vm, "SIBLINGCOUNT_N", ns);
    n = hit ? 1 : 0;
    if (cda) var_set_str(vm, "CLASS", cda->name);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s -> n=%d hit=%d sibs=%d\n", opname, a, n, hit, ns);
    bump(vm);
    return 1;
  }

  /* NEXTSIBLING + PREVSIBLING + CHILDRANK (multifile EXTEND)
   * NEXTSIBLING Class|obj — next peer after self under same parent (decl order); soft empty.
   * PREVSIBLING Class|obj — previous peer before self; soft empty.
   * CHILDRANK Class|obj — 0-based index among parent's children (self rank); -1/soft 0 if root.
   * Complements NTHSIBLING + FIRST/LASTSIBLING + SIBLINGS bag. Cube is SoT. Free energy must flow. */
  if (kw(&L->cur, "NEXTSIBLING") || kw(&L->cur, "NEXTPEER") ||
      kw(&L->cur, "SIBLINGNEXT") || kw(&L->cur, "NEXT_SIBLING") ||
      kw(&L->cur, "FOLLOWINGSIB") || kw(&L->cur, "RIGHTSIBLING") ||
      kw(&L->cur, "PREVSIBLING") || kw(&L->cur, "PREVPEER") ||
      kw(&L->cur, "SIBLINGPREV") || kw(&L->cur, "PREV_SIBLING") ||
      kw(&L->cur, "PRECEDINGSIB") || kw(&L->cur, "LEFTSIBLING") ||
      kw(&L->cur, "PRIORSIBLING") ||
      kw(&L->cur, "CHILDRANK") || kw(&L->cur, "SIBLINGRANK") ||
      kw(&L->cur, "INDEXOFCHILD") || kw(&L->cur, "CHILD_RANK") ||
      kw(&L->cur, "RANKAMONGSIBS") || kw(&L->cur, "MYCHILDIX") ||
      kw(&L->cur, "CHILDINDEX") || kw(&L->cur, "SIBINDEX")) {
    int want_next = kw(&L->cur, "NEXTSIBLING") || kw(&L->cur, "NEXTPEER") ||
                    kw(&L->cur, "SIBLINGNEXT") || kw(&L->cur, "NEXT_SIBLING") ||
                    kw(&L->cur, "FOLLOWINGSIB") || kw(&L->cur, "RIGHTSIBLING");
    int want_prev = kw(&L->cur, "PREVSIBLING") || kw(&L->cur, "PREVPEER") ||
                    kw(&L->cur, "SIBLINGPREV") || kw(&L->cur, "PREV_SIBLING") ||
                    kw(&L->cur, "PRECEDINGSIB") || kw(&L->cur, "LEFTSIBLING") ||
                    kw(&L->cur, "PRIORSIBLING");
    int want_rank = kw(&L->cur, "CHILDRANK") || kw(&L->cur, "SIBLINGRANK") ||
                    kw(&L->cur, "INDEXOFCHILD") || kw(&L->cur, "CHILD_RANK") ||
                    kw(&L->cur, "RANKAMONGSIBS") || kw(&L->cur, "MYCHILDIX") ||
                    kw(&L->cur, "CHILDINDEX") || kw(&L->cur, "SIBINDEX");
    char a[48], pick[48];
    ClassDef *cda = NULL;
    ObjInst *ob;
    int ai = -1, i, n = 0, hit = 0, rank = -1;
    int kids[512];
    int nk = 0;
    const char *opname = want_next ? "NEXTSIBLING"
                         : want_prev ? "PREVSIBLING"
                                       : "CHILDRANK";
    lex_next(L);
    a[0] = pick[0] = 0;
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, want_rank ? "CHILDRANK Class|obj"
                         : (want_next ? "NEXTSIBLING Class|obj"
                                      : "PREVSIBLING Class|obj"));
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
    if (cda && ai >= 0) {
      int parent = cda->parent_idx;
      if (parent >= 0 && parent < vm->n_classes) {
        for (i = 0; i < vm->n_classes; i++) {
          if (vm->classes[i].parent_idx == parent && nk < 512)
            kids[nk++] = i;
        }
        for (i = 0; i < nk; i++) {
          if (kids[i] == ai) { rank = i; break; }
        }
      } else {
        /* root / orphan: rank 0 alone; no siblings */
        rank = 0;
        nk = 0;
      }
    }
    if (want_rank) {
      n = (rank >= 0) ? rank : 0;
      hit = (cda && rank >= 0) ? 1 : 0;
      /* roots report rank 0 with OK=1 and LAST_N=0 meaning rank value in LAST_N */
      var_set_num(vm, "LAST_N", (cda && rank >= 0) ? rank : 0);
      vm->last_n = (cda && rank >= 0) ? rank : 0;
      var_set_num(vm, "CHILDRANK", (cda && rank >= 0) ? rank : 0);
      var_set_num(vm, "CHILDRANK_N", (cda && rank >= 0) ? rank : 0);
      var_set_num(vm, "SIBLINGRANK_N", (cda && rank >= 0) ? rank : 0);
      var_set_num(vm, "INDEXOFCHILD_N", (cda && rank >= 0) ? rank : 0);
      var_set_num(vm, "CHILDINDEX_N", (cda && rank >= 0) ? rank : 0);
      n = (cda && rank >= 0) ? rank : 0;
    } else {
      pick[0] = 0;
      hit = 0;
      if (rank >= 0 && nk > 0) {
        int j = want_next ? (rank + 1) : (rank - 1);
        if (j >= 0 && j < nk) {
          snprintf(pick, sizeof pick, "%s", vm->classes[kids[j]].name);
          hit = 1;
        }
      }
      var_set_str(vm, "LAST", pick);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", pick);
      var_set_str(vm, "NEXTSIBLING", pick);
      var_set_str(vm, "PREVSIBLING", pick);
      var_set_str(vm, "NEXTPEER", pick);
      var_set_str(vm, "PREVPEER", pick);
      var_set_num(vm, "LAST_N", hit ? 1 : 0);
      vm->last_n = hit ? 1 : 0;
      var_set_num(vm, "NEXTSIBLING_N", hit ? 1 : 0);
      var_set_num(vm, "PREVSIBLING_N", hit ? 1 : 0);
      n = hit ? 1 : 0;
    }
    if (cda) var_set_str(vm, "CLASS", cda->name);
    var_set_num(vm, "OK", cda ? 1 : 0);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s -> rank=%d hit=%d n=%d\n", opname, a, rank, hit, n);
    bump(vm);
    return 1;
  }


  /* ISROOT + ISFIRSTCHILD/ISLASTCHILD + ONLYCHILD (multifile EXTEND)
   * ISROOT Class|obj — 1 if no parent_idx (top of EXTEND chain).
   * ISFIRSTCHILD / ISLASTCHILD — 1 if self is first/last among parent's kids.
   * ONLYCHILD — 1 if parent has exactly one direct child (self alone).
   * Complements CHILDRANK + NEXT/PREVSIBLING + ISLEAF + ROOTOF.
   * LAST_N / form_N = 0|1 soft. Cube is SoT. Free energy must flow. */
  if (kw(&L->cur, "ISROOT") || kw(&L->cur, "IS_ROOT") ||
      kw(&L->cur, "NOROOTPARENT") || kw(&L->cur, "TOPCLASS") ||
      kw(&L->cur, "ISFIRSTCHILD") || kw(&L->cur, "IS_FIRST_CHILD") ||
      kw(&L->cur, "ISELDERCHILD") || kw(&L->cur, "ISOLDESTCHILD") ||
      kw(&L->cur, "ISFIRSTKID") || kw(&L->cur, "ELDESTCHILD_P") ||
      kw(&L->cur, "ISLASTCHILD") || kw(&L->cur, "IS_LAST_CHILD") ||
      kw(&L->cur, "ISYOUNGCHILD") || kw(&L->cur, "ISNEWESTCHILD") ||
      kw(&L->cur, "ISLASTKID") || kw(&L->cur, "YOUNGESTCHILD_P") ||
      kw(&L->cur, "ONLYCHILD") || kw(&L->cur, "ISONLYCHILD") ||
      kw(&L->cur, "ONLY_CHILD") || kw(&L->cur, "IS_ONLY_CHILD") ||
      kw(&L->cur, "SOLECHILD") || kw(&L->cur, "HASNOSIB")) {
    int want_root = kw(&L->cur, "ISROOT") || kw(&L->cur, "IS_ROOT") ||
                    kw(&L->cur, "NOROOTPARENT") || kw(&L->cur, "TOPCLASS");
    int want_first = kw(&L->cur, "ISFIRSTCHILD") || kw(&L->cur, "IS_FIRST_CHILD") ||
                     kw(&L->cur, "ISELDERCHILD") || kw(&L->cur, "ISOLDESTCHILD") ||
                     kw(&L->cur, "ISFIRSTKID") || kw(&L->cur, "ELDESTCHILD_P");
    int want_last = kw(&L->cur, "ISLASTCHILD") || kw(&L->cur, "IS_LAST_CHILD") ||
                    kw(&L->cur, "ISYOUNGCHILD") || kw(&L->cur, "ISNEWESTCHILD") ||
                    kw(&L->cur, "ISLASTKID") || kw(&L->cur, "YOUNGESTCHILD_P");
    int want_only = kw(&L->cur, "ONLYCHILD") || kw(&L->cur, "ISONLYCHILD") ||
                    kw(&L->cur, "ONLY_CHILD") || kw(&L->cur, "IS_ONLY_CHILD") ||
                    kw(&L->cur, "SOLECHILD") || kw(&L->cur, "HASNOSIB");
    char a[48];
    ClassDef *cda = NULL;
    ObjInst *ob;
    int ai = -1, i, n = 0, nk = 0, rank = -1;
    const char *opname = want_root ? "ISROOT"
                         : want_first ? "ISFIRSTCHILD"
                         : want_last ? "ISLASTCHILD"
                                       : "ONLYCHILD";
    lex_next(L);
    a[0] = 0;
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, want_root ? "ISROOT Class|obj"
                         : (want_first ? "ISFIRSTCHILD Class|obj"
                         : (want_last ? "ISLASTCHILD Class|obj"
                                      : "ONLYCHILD Class|obj")));
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
    n = 0;
    nk = 0;
    rank = -1;
    if (cda && ai >= 0) {
      int parent = cda->parent_idx;
      if (want_root) {
        n = (parent < 0 || parent >= vm->n_classes) ? 1 : 0;
      } else if (parent >= 0 && parent < vm->n_classes) {
        for (i = 0; i < vm->n_classes; i++) {
          if (vm->classes[i].parent_idx == parent) {
            if (i == ai) rank = nk;
            nk++;
          }
        }
        if (want_first) n = (rank == 0) ? 1 : 0;
        else if (want_last) n = (rank >= 0 && rank == nk - 1) ? 1 : 0;
        else if (want_only) n = (nk == 1) ? 1 : 0;
      } else {
        /* root: not a child; first/last/only soft 0 */
        n = 0;
      }
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    snprintf(vm->last_str, sizeof vm->last_str, "%d", n);
    var_set_str(vm, "LAST", vm->last_str);
    if (want_root) {
      var_set_num(vm, "ISROOT_N", n);
      var_set_num(vm, "ISROOT", n);
    } else if (want_first) {
      var_set_num(vm, "ISFIRSTCHILD_N", n);
      var_set_num(vm, "ISFIRSTCHILD", n);
    } else if (want_last) {
      var_set_num(vm, "ISLASTCHILD_N", n);
      var_set_num(vm, "ISLASTCHILD", n);
    } else {
      var_set_num(vm, "ONLYCHILD_N", n);
      var_set_num(vm, "ONLYCHILD", n);
      var_set_num(vm, "ISONLYCHILD_N", n);
    }
    if (rank >= 0) {
      var_set_num(vm, "CHILDRANK_N", rank);
      var_set_num(vm, "CHILDCOUNT_N", nk);
    }
    if (cda) var_set_str(vm, "CLASS", cda->name);
    var_set_num(vm, "OK", cda ? 1 : 0);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s -> n=%d rank=%d kids=%d\n", opname, a, n, rank, nk);
    bump(vm);
    return 1;
  }

  /* SIBLINGCOUNT + HASSIBLING (multifile EXTEND)
   * SIBLINGCOUNT Class|obj — peer count under same parent (excludes self).
   * HASSIBLING Class|obj — 1 if at least one peer, else 0.
   * ONLYCHILD lives with ISROOT/ISFIRSTCHILD block (parent-child cardinality).
   * Complements SIBLINGS bag + NTHSIBLING + CHILDRANK. Cube is SoT. Free energy must flow. */
  if (kw(&L->cur, "SIBLINGCOUNT") || kw(&L->cur, "COUNTSIBLINGS") ||
      kw(&L->cur, "NUMSIBLINGS") || kw(&L->cur, "NSIBLINGS") ||
      kw(&L->cur, "PEERCOUNT") || kw(&L->cur, "SIBLING_COUNT") ||
      kw(&L->cur, "COUNT_SIBLINGS") || kw(&L->cur, "NPEERS") ||
      kw(&L->cur, "HASSIBLING") || kw(&L->cur, "HASSIBLINGS") ||
      kw(&L->cur, "HASPEER") || kw(&L->cur, "HASPEERS") ||
      kw(&L->cur, "HAS_SIBLING") || kw(&L->cur, "HAS_SIBLINGS")) {
    int want_cnt = kw(&L->cur, "SIBLINGCOUNT") || kw(&L->cur, "COUNTSIBLINGS") ||
                   kw(&L->cur, "NUMSIBLINGS") || kw(&L->cur, "NSIBLINGS") ||
                   kw(&L->cur, "PEERCOUNT") || kw(&L->cur, "SIBLING_COUNT") ||
                   kw(&L->cur, "COUNT_SIBLINGS") || kw(&L->cur, "NPEERS");
    int want_has = kw(&L->cur, "HASSIBLING") || kw(&L->cur, "HASSIBLINGS") ||
                   kw(&L->cur, "HASPEER") || kw(&L->cur, "HASPEERS") ||
                   kw(&L->cur, "HAS_SIBLING") || kw(&L->cur, "HAS_SIBLINGS");
    char a[48];
    ClassDef *cda = NULL;
    ObjInst *ob;
    int ai = -1, i, nsib = 0, flag = 0;
    const char *opname = want_cnt ? "SIBLINGCOUNT" : "HASSIBLING";
    lex_next(L);
    a[0] = 0;
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, want_cnt ? "SIBLINGCOUNT Class|obj" : "HASSIBLING Class|obj");
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
    nsib = 0;
    if (cda && ai >= 0) {
      int parent = cda->parent_idx;
      if (parent >= 0 && parent < vm->n_classes) {
        for (i = 0; i < vm->n_classes; i++) {
          if (i != ai && vm->classes[i].parent_idx == parent)
            nsib++;
        }
      }
      /* roots: no peers under a parent → 0 siblings */
    }
    flag = nsib > 0 ? 1 : 0;
    var_set_num(vm, "LAST_N", want_cnt ? nsib : flag);
    vm->last_n = want_cnt ? nsib : flag;
    snprintf(vm->last_str, sizeof vm->last_str, "%d", want_cnt ? nsib : flag);
    var_set_str(vm, "LAST", vm->last_str);
    if (want_cnt) {
      var_set_num(vm, "SIBLINGCOUNT", nsib);
      var_set_num(vm, "SIBLINGCOUNT_N", nsib);
      var_set_num(vm, "PEERCOUNT_N", nsib);
      var_set_num(vm, "NSIBLINGS", nsib);
    } else {
      var_set_num(vm, "HASSIBLING", flag);
      var_set_num(vm, "HASSIBLING_N", flag);
      var_set_num(vm, "HASSIBLINGS_N", flag);
      var_set_num(vm, "HASPEER_N", flag);
    }
    /* also mirror peer card for both */
    var_set_num(vm, "SIBLINGCOUNT_N", nsib);
    var_set_num(vm, "HASSIBLING_N", flag);
    if (cda) var_set_str(vm, "CLASS", cda->name);
    var_set_num(vm, "OK", cda ? 1 : 0);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s -> nsib=%d flag=%d\n", opname, a, nsib, flag);
    bump(vm);
    return 1;
  }

  /* ARESIBLINGS + ISSIBLING/SIBLINGOF/PEEROF (multifile EXTEND)
   * ARESIBLINGS|ISSIBLING|SIBLINGOF|PEEROF|ISPEEROF|SHAREPARENT_P Class|obj Class|obj
   * — soft 0|1 if both resolve and share the same parent_idx (distinct classes).
   * Roots / unknown / self-pair → 0. Complements SIBLINGCOUNT + HASSIBLING +
   * SIBLINGS bag + CHILDRANK. Cube is SoT. Free energy must flow. */
  if (kw(&L->cur, "ARESIBLINGS") || kw(&L->cur, "ARE_SIBLINGS") ||
      kw(&L->cur, "ISSIBLING") || kw(&L->cur, "IS_SIBLING") ||
      kw(&L->cur, "SIBLINGOF") || kw(&L->cur, "SIBLING_OF") ||
      kw(&L->cur, "PEEROF") || kw(&L->cur, "PEER_OF") ||
      kw(&L->cur, "ISPEEROF") || kw(&L->cur, "IS_PEER_OF") ||
      kw(&L->cur, "SHAREPARENT_P") || kw(&L->cur, "SAMEPARENT_P") ||
      kw(&L->cur, "COSIBLING") || kw(&L->cur, "CO_SIBLING")) {
    char a[48], b[48];
    ClassDef *cda = NULL, *cdb = NULL;
    ObjInst *ob;
    int ai = -1, bi = -1, hit = 0;
    const char *opname = "ARESIBLINGS";
    lex_next(L);
    a[0] = b[0] = 0;
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "ARESIBLINGS Class|obj Class|obj");
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
      fail(vm, "ARESIBLINGS Class|obj Class|obj");
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
      int pa = cda->parent_idx;
      int pb = cdb->parent_idx;
      if (pa >= 0 && pa < vm->n_classes && pa == pb)
        hit = 1;
    }
    var_set_num(vm, "LAST_N", hit ? 1 : 0);
    vm->last_n = hit ? 1 : 0;
    snprintf(vm->last_str, sizeof vm->last_str, "%d", hit ? 1 : 0);
    var_set_str(vm, "LAST", vm->last_str);
    var_set_num(vm, "ARESIBLINGS_N", hit ? 1 : 0);
    var_set_num(vm, "ARESIBLINGS", hit ? 1 : 0);
    var_set_num(vm, "ISSIBLING_N", hit ? 1 : 0);
    var_set_num(vm, "ISSIBLING", hit ? 1 : 0);
    var_set_num(vm, "SIBLINGOF_N", hit ? 1 : 0);
    var_set_num(vm, "PEEROF_N", hit ? 1 : 0);
    var_set_num(vm, "OK", (cda && cdb) ? 1 : 0);
    if (cda) var_set_str(vm, "CLASS", cda->name);
    if (cdb) var_set_str(vm, "PEER", cdb->name);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s %s -> %d\n", opname, a, b, hit);
    bump(vm);
    return 1;
  }

  return 0;
}
