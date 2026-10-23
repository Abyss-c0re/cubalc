/* CubalC lang — lang_ops_flow_fstr_oopchain.c
 * FLOW fstr OOP chain: ISOF · CLASSNAME · BASEOF · PARENTS · HASANCESTOR
 * Peel from fstr_a (B_fstr_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_flow_fstr_oopchain.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_fstr_oopchain(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* ISOF obj ClassName */
  if (kw(&L->cur, "ISOF") || kw(&L->cur, "ISINSTANCE") || kw(&L->cur, "ISA") ||
      kw(&L->cur, "INSTANCEOF")) {
    char oname[48], cname[48];
    ObjInst *ob;
    int hit = 0;
    lex_next(L);
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "ISOF object"); return -1;
    }
    if (L->cur.kind != TK_IDENT) { fail(vm, "ISOF ClassName"); return -1; }
    snprintf(cname, sizeof cname, "%s", L->cur.text);
    lex_next(L);
    ob = oop_find_obj(vm, oname);
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes)
      hit = oop_isof_chain(vm, ob->class_idx, cname);
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* CLASSNAME obj */
  if (kw(&L->cur, "CLASSNAME") || kw(&L->cur, "TYPEOF_OBJ") ||
      kw(&L->cur, "OBJCLASS")) {
    char oname[48];
    ObjInst *ob;
    lex_next(L);
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "CLASSNAME object"); return -1;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      var_set_str(vm, "LAST", "");
      var_set_num(vm, "OK", 0);
      bump(vm); return 1;
    }
    {
      ClassDef *cd = &vm->classes[ob->class_idx];
      var_set_str(vm, "LAST", cd->name);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", cd->name);
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* BASEOF|GETBASE|PARENTCLASS|BASECLASS Class|obj
   * — multi-file EXTEND/link usability: resolve immediate parent CLASS name.
   * Soft: unknown / no parent → LAST="" LAST_N=0 OK=1 (probe, not fail).
   * HASPARENT|HASBASE → LAST_N 0|1 only. Complements ISOF chain + CLASSINFO.
   * Usability: agent link checks without CLASSINFO bag scrape after INCLUDE units. */
  if (kw(&L->cur, "BASEOF") || kw(&L->cur, "GETBASE") ||
      kw(&L->cur, "PARENTCLASS") || kw(&L->cur, "BASECLASS") ||
      kw(&L->cur, "CLASSBASE") || kw(&L->cur, "GETPARENT") ||
      kw(&L->cur, "PARENT_OF") || kw(&L->cur, "BASE_OF") ||
      kw(&L->cur, "HASPARENT") || kw(&L->cur, "HASBASE") ||
      kw(&L->cur, "HAS_PARENT") || kw(&L->cur, "HAS_BASE")) {
    int want_flag = kw(&L->cur, "HASPARENT") || kw(&L->cur, "HASBASE") ||
                    kw(&L->cur, "HAS_PARENT") || kw(&L->cur, "HAS_BASE");
    char a[48];
    ClassDef *cd = NULL;
    ObjInst *ob;
    const char *pname = "";
    int has = 0;
    lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, want_flag ? "HASPARENT Class|obj" : "BASEOF Class|obj");
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
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes)
      cd = &vm->classes[ob->class_idx];
    else
      cd = oop_find_class(vm, a);
    if (cd && cd->parent_idx >= 0 && cd->parent_idx < vm->n_classes) {
      pname = vm->classes[cd->parent_idx].name;
      has = 1;
    }
    if (want_flag) {
      var_set_num(vm, "LAST_N", has);
      vm->last_n = has;
      var_set_str(vm, "LAST", has ? "1" : "0");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", has ? "1" : "0");
      if (has) {
        var_set_str(vm, "BASE", pname);
        var_set_str(vm, "PARENTCLASS", pname);
      } else {
        var_set_str(vm, "BASE", "");
        var_set_str(vm, "PARENTCLASS", "");
      }
    } else {
      var_set_str(vm, "LAST", pname);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", pname);
      var_set_num(vm, "LAST_N", has);
      vm->last_n = has;
      var_set_str(vm, "BASE", pname);
      var_set_str(vm, "PARENTCLASS", pname);
      if (cd)
        var_set_str(vm, "CLASS", cd->name);
    }
    var_set_num(vm, "HASPARENT_N", has);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s -> %s has=%d\n",
              want_flag ? "HASPARENT" : "BASEOF", a, pname[0] ? pname : "-", has);
    bump(vm);
    return 1;
  }

  /* PARENTS|SUPERCHAIN|ANCESTORS|BASECHAIN Class|obj
   * — multi-file EXTEND/link usability: full parent chain as newline bag.
   * Walk parent_idx from Class|obj up to root (skip self). LAST = bag,
   * PARENTS_N / CHAIN_N / LAST_N = depth. Soft empty bag if no parent.
   * ROOTOF|GETROOT|ROOTCLASS Class|obj — topmost ancestor name (or self).
   * Complements BASEOF (immediate) + ISOF + SENDSUPER without bag scrape. */
  if (kw(&L->cur, "PARENTS") || kw(&L->cur, "SUPERCHAIN") ||
      kw(&L->cur, "ANCESTORS") || kw(&L->cur, "BASECHAIN") ||
      kw(&L->cur, "PARENTCHAIN") || kw(&L->cur, "EXTENDCHAIN") ||
      kw(&L->cur, "CHAINOF") || kw(&L->cur, "SUPERPATH") ||
      kw(&L->cur, "ROOTOF") || kw(&L->cur, "GETROOT") ||
      kw(&L->cur, "ROOTCLASS") || kw(&L->cur, "CLASSROOT") ||
      kw(&L->cur, "BASEROOT") || kw(&L->cur, "TOPBASE") ||
      kw(&L->cur, "ROOT_OF") || kw(&L->cur, "GET_ROOT")) {
    int want_root = kw(&L->cur, "ROOTOF") || kw(&L->cur, "GETROOT") ||
                    kw(&L->cur, "ROOTCLASS") || kw(&L->cur, "CLASSROOT") ||
                    kw(&L->cur, "BASEROOT") || kw(&L->cur, "TOPBASE") ||
                    kw(&L->cur, "ROOT_OF") || kw(&L->cur, "GET_ROOT");
    char a[48];
    ClassDef *cd = NULL;
    ObjInst *ob;
    char bag[1024];
    size_t o = 0;
    int n = 0, guard = 0, pi;
    const char *root = "";
    lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, want_root ? "ROOTOF Class|obj" : "PARENTS Class|obj");
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
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes)
      cd = &vm->classes[ob->class_idx];
    else
      cd = oop_find_class(vm, a);
    bag[0] = 0;
    if (cd) {
      root = cd->name;
      pi = cd->parent_idx;
      while (pi >= 0 && pi < vm->n_classes && guard++ < CUBALC_MAX_CLASSES) {
        ClassDef *pw = &vm->classes[pi];
        size_t ln = strlen(pw->name);
        if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + ln < sizeof bag) {
          memcpy(bag + o, pw->name, ln);
          o += ln;
        }
        bag[o] = 0;
        n++;
        root = pw->name;
        pi = pw->parent_idx;
      }
    }
    if (want_root) {
      var_set_str(vm, "LAST", root);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", root);
      var_set_str(vm, "ROOT", root);
      var_set_str(vm, "ROOTCLASS", root);
      var_set_str(vm, "BASE", root);
      var_set_num(vm, "LAST_N", root[0] ? 1 : 0);
      vm->last_n = root[0] ? 1 : 0;
      var_set_num(vm, "ROOTOF_N", root[0] ? 1 : 0);
      if (cd) var_set_str(vm, "CLASS", cd->name);
    } else {
      var_set_str(vm, "LAST", bag);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
      var_set_str(vm, "PARENTS", bag);
      var_set_str(vm, "SUPERCHAIN", bag);
      var_set_str(vm, "ANCESTORS", bag);
      var_set_str(vm, "BASECHAIN", bag);
      var_set_num(vm, "LAST_N", n);
      vm->last_n = n;
      var_set_num(vm, "PARENTS_N", n);
      var_set_num(vm, "CHAIN_N", n);
      var_set_num(vm, "SUPERCHAIN_N", n);
      var_set_str(vm, "ROOT", root);
      var_set_str(vm, "ROOTCLASS", root);
      if (cd) var_set_str(vm, "CLASS", cd->name);
    }
    var_set_num(vm, "HASPARENT_N", n > 0 ? 1 : 0);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s -> root=%s depth=%d\n",
              want_root ? "ROOTOF" : "PARENTS", a, root[0] ? root : "-", n);
    bump(vm);
    return 1;
  }

  /* HASANCESTOR|INCHAIN|EXTENDSFROM|DERIVESFROM|ISAEXTEND Class|obj Ancestor
   * — multi-file EXTEND/link usability: soft 0|1 if Ancestor appears on the
   * parent_idx walk (not self). Complements PARENTS bag + ROOTOF + ISOF.
   * DEPTHOF|CHAINDEPTH|PARENTDEPTH Class|obj — numeric EXTEND depth (PARENTS_N).
   * Cube Law: free energy must flow; meaningful growth only. */
  if (kw(&L->cur, "HASANCESTOR") || kw(&L->cur, "INCHAIN") ||
      kw(&L->cur, "EXTENDSFROM") || kw(&L->cur, "DERIVESFROM") ||
      kw(&L->cur, "ISAEXTEND") || kw(&L->cur, "HAS_ANCESTOR") ||
      kw(&L->cur, "DEPTHOF") || kw(&L->cur, "CHAINDEPTH") ||
      kw(&L->cur, "PARENTDEPTH") || kw(&L->cur, "EXTENDDEPTH") ||
      kw(&L->cur, "DEPTH_OF") || kw(&L->cur, "CHAIN_DEPTH")) {
    int want_depth = kw(&L->cur, "DEPTHOF") || kw(&L->cur, "CHAINDEPTH") ||
                     kw(&L->cur, "PARENTDEPTH") || kw(&L->cur, "EXTENDDEPTH") ||
                     kw(&L->cur, "DEPTH_OF") || kw(&L->cur, "CHAIN_DEPTH");
    char a[48], anc[48];
    ClassDef *cd = NULL;
    ObjInst *ob;
    int n = 0, guard = 0, pi, hit = 0;
    lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, want_depth ? "DEPTHOF Class|obj" : "HASANCESTOR Class|obj Ancestor");
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
    anc[0] = 0;
    if (!want_depth) {
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "HASANCESTOR Class|obj Ancestor");
        return -1;
      }
      if (L->cur.kind == TK_STR) {
        snprintf(anc, sizeof anc, "%s", L->cur.text);
        lex_next(L);
      } else {
        char id[48];
        Var *vv;
        snprintf(id, sizeof id, "%s", L->cur.text);
        lex_next(L);
        if (oop_find_class(vm, id)) {
          snprintf(anc, sizeof anc, "%s", id);
        } else {
          vv = var_get(vm, id, 0);
          if (vv && vv->is_str && vv->sval[0])
            snprintf(anc, sizeof anc, "%s", vv->sval);
          else
            snprintf(anc, sizeof anc, "%s", id);
        }
      }
    }
    ob = oop_find_obj(vm, a);
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes)
      cd = &vm->classes[ob->class_idx];
    else
      cd = oop_find_class(vm, a);
    if (cd) {
      pi = cd->parent_idx;
      while (pi >= 0 && pi < vm->n_classes && guard++ < CUBALC_MAX_CLASSES) {
        ClassDef *pw = &vm->classes[pi];
        n++;
        if (!want_depth && anc[0] && strcmp(pw->name, anc) == 0) hit = 1;
        pi = pw->parent_idx;
      }
    }
    if (want_depth) {
      var_set_num(vm, "LAST_N", n);
      vm->last_n = n;
      var_set_num(vm, "DEPTH", n);
      var_set_num(vm, "DEPTHOF_N", n);
      var_set_num(vm, "CHAIN_N", n);
      var_set_num(vm, "PARENTS_N", n);
      snprintf(vm->last_str, sizeof vm->last_str, "%d", n);
      var_set_str(vm, "LAST", vm->last_str);
      if (cd) var_set_str(vm, "CLASS", cd->name);
    } else {
      var_set_num(vm, "LAST_N", hit ? 1 : 0);
      vm->last_n = hit ? 1 : 0;
      var_set_num(vm, "HASANCESTOR_N", hit ? 1 : 0);
      var_set_num(vm, "INCHAIN_N", hit ? 1 : 0);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hit ? "1" : "0");
      var_set_str(vm, "LAST", vm->last_str);
      if (anc[0]) var_set_str(vm, "ANCESTOR", anc);
      if (cd) var_set_str(vm, "CLASS", cd->name);
      var_set_num(vm, "PARENTS_N", n);
      var_set_num(vm, "CHAIN_N", n);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s %s -> hit=%d depth=%d\n",
              want_depth ? "DEPTHOF" : "HASANCESTOR", a,
              want_depth ? "-" : anc, hit, n);
    bump(vm);
    return 1;
  }

  return 0;
}
