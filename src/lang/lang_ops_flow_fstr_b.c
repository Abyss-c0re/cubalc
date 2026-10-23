/* flow fstr peel from flow_class */
#include "lang/cubalc_lang_ops_flow_fstr_b.h"
#include "lang/cubalc_lang_ops_flow_fstr_lifecycle.h"
#include "lang/cubalc_lang_ops_flow_fstr_info.h"
#include "lang/cubalc_lang_ops_flow_fstr_hasexist.h"
#include "lang/cubalc_lang_ops_flow_fstr_hasfield.h"
#include "lang/cubalc_lang_ops_flow_fstr_hasmeth.h"
#include "lang/cubalc_lang_ops_flow_fstr_kinship.h"
#include "lang/cubalc_lang_ops_flow_fstr_cousin.h"
#include "lang/cubalc_lang_ops_flow_fstr_dumpobj.h"
#include "lang/cubalc_lang_ops_flow_fstr_listobjs.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_fstr_b(VM *vm, Lex *L) {
  /* lifecycle DELETE/CLONE/RENAME → lang_ops_flow_fstr_lifecycle.c */
  {
    int lc = cubalc_lang_ops_flow_try_fstr_lifecycle(vm, L);
    if (lc != 0) return lc;
  }

  /* CLASSINFO/METHODINFO/FNINFO → lang_ops_flow_fstr_info.c */
  {
    int info = cubalc_lang_ops_flow_try_fstr_info(vm, L);
    if (info != 0) return info;
  }

  /* HASCLASS/HASOBJ/HASFN → lang_ops_flow_fstr_hasexist.c */
  {
    int he = cubalc_lang_ops_flow_try_fstr_hasexist(vm, L);
    if (he != 0) return he;
  }

  /* HASFIELD cluster → lang_ops_flow_fstr_hasfield.c (B_fstr_b_peel) */
  {
    int hf = cubalc_lang_ops_flow_try_fstr_hasfield(vm, L);
    if (hf != 0) return hf;
  }

  /* HASMETHOD cluster → lang_ops_flow_fstr_hasmeth.c (B_fstr_b_peel) */
  {
    int hm = cubalc_lang_ops_flow_try_fstr_hasmeth(vm, L);
    if (hm != 0) return hm;
  }

  /* kinship CHILDCOUNT..ARESIBLINGS → lang_ops_flow_fstr_kinship.c (B_fstr_b_peel) */
  {
    int kn = cubalc_lang_ops_flow_try_fstr_kinship(vm, L);
    if (kn != 0) return kn;
  }



  /* cousin COUSINCOUNT..NTHCOUSIN → lang_ops_flow_fstr_cousin.c (B_fstr_b_peel) */
  {
    int cz = cubalc_lang_ops_flow_try_fstr_cousin(vm, L);
    if (cz != 0) return cz;
  }




  /* LISTOVERRIDEFIELDS|OVERRIDEFIELDS|REDEFINEDFIELDS Class|obj — newline bag of
   * fields owned here that also exist on an ancestor (default override / redefine).
   * Multi-file EXTEND pack: contribution that shadows parent slots.
   * Partition: LISTOWN = introduced+override; LISTINHERITED = pure inherit;
   * LISTOVERRIDE = own∩ancestor. LAST = bag; LAST_N = count. Soft empty OK=1. */
  if (kw(&L->cur, "LISTOVERRIDEFIELDS") || kw(&L->cur, "OVERRIDEFIELDS") ||
      kw(&L->cur, "REDEFINEDFIELDS") || kw(&L->cur, "OVERRIDE_FIELDS") ||
      kw(&L->cur, "LISTREDEFINEDFIELDS") || kw(&L->cur, "FIELDS_OVERRIDE")) {
    char a[48], bag[2048];
    ClassDef *cd = NULL;
    ObjInst *ob;
    size_t o = 0;
    int n = 0, fi;
    lex_next(L);
    a[0] = bag[0] = 0;
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "LISTOVERRIDEFIELDS Class|obj");
      return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(a, sizeof a, "%s", L->cur.text); lex_next(L);
    } else {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text); lex_next(L);
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
    cd = oop_find_class(vm, a);
    if (!cd) {
      ob = oop_find_obj(vm, a);
      if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes)
        cd = &vm->classes[ob->class_idx];
    }
    if (cd) {
      for (fi = 0; fi < cd->n_fields; fi++) {
        FieldDef *fd = &cd->fields[fi];
        const char *fname = fd->name;
        int own = 0, seen = 0;
        if (cd->parent_idx < 0 || cd->parent_idx >= vm->n_classes) {
          own = 1; /* root: no ancestor → not override */
        } else {
          ClassDef *walk = &vm->classes[cd->parent_idx];
          int g = 0;
          while (walk && g++ < CUBALC_MAX_CLASSES) {
            int pfi = oop_field_idx(walk, fname);
            if (pfi >= 0) {
              seen = 1;
              if (!oop_field_def_same(fd, &walk->fields[pfi]))
                own = 1; /* override default */
              break;
            }
            if (walk->parent_idx < 0 || walk->parent_idx >= vm->n_classes) break;
            walk = &vm->classes[walk->parent_idx];
          }
          if (!seen) own = 1; /* introduced here — not override */
        }
        /* override = owned AND seen on ancestor with different def */
        if (own && seen) {
          size_t ln = strlen(fname);
          if (ln && o + ln + 2 < sizeof bag) {
            if (o) bag[o++] = '\n';
            memcpy(bag + o, fname, ln);
            o += ln;
          }
          bag[o] = 0;
          n++;
        }
      }
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LISTOVERRIDEFIELDS", bag);
    var_set_str(vm, "OVERRIDEFIELDS", bag);
    var_set_str(vm, "REDEFINEDFIELDS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "OVERRIDEFIELDS_N", n);
    var_set_num(vm, "LISTOVERRIDEFIELDS_N", n);
    var_set_num(vm, "REDEFINEDFIELDS_N", n);
    if (a[0]) var_set_str(vm, "CLASS", a);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LISTOVERRIDEMETHODS|OVERRIDEMETHODS|REDEFINEDMETHODS Class|obj — newline bag
   * of methods owned here that also exist on an ancestor (body override).
   * Multi-file EXTEND pack shadow probe. Dual of LISTOVERRIDEFIELDS.
   * LAST = bag; LAST_N = count. Soft empty OK=1. */
  if (kw(&L->cur, "LISTOVERRIDEMETHODS") || kw(&L->cur, "OVERRIDEMETHODS") ||
      kw(&L->cur, "REDEFINEDMETHODS") || kw(&L->cur, "OVERRIDE_METHODS") ||
      kw(&L->cur, "LISTREDEFINEDMETHODS") || kw(&L->cur, "METHODS_OVERRIDE")) {
    char a[48], bag[2048];
    ClassDef *cd = NULL;
    ObjInst *ob;
    size_t o = 0;
    int n = 0, mi;
    lex_next(L);
    a[0] = bag[0] = 0;
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "LISTOVERRIDEMETHODS Class|obj");
      return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(a, sizeof a, "%s", L->cur.text); lex_next(L);
    } else {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text); lex_next(L);
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
    cd = oop_find_class(vm, a);
    if (!cd) {
      ob = oop_find_obj(vm, a);
      if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes)
        cd = &vm->classes[ob->class_idx];
    }
    if (cd) {
      for (mi = 0; mi < cd->n_methods; mi++) {
        MethodDef *md = &cd->methods[mi];
        const char *mname = md->name;
        int own = 1, seen = 0;
        if (cd->parent_idx >= 0 && cd->parent_idx < vm->n_classes) {
          ClassDef *walk = &vm->classes[cd->parent_idx];
          int pguard = 0;
          while (walk && pguard++ < CUBALC_MAX_CLASSES) {
            MethodDef *pm = oop_find_method(walk, mname);
            if (pm) {
              seen = 1;
              if (pm->body == md->body && pm->len == md->len)
                own = 0; /* pure inherit same body pointer */
              break;
            }
            if (walk->parent_idx < 0 || walk->parent_idx >= vm->n_classes) break;
            walk = &vm->classes[walk->parent_idx];
          }
        }
        /* override = owned body AND ancestor had same name */
        if (own && seen) {
          size_t ln = strlen(mname);
          if (ln && o + ln + 2 < sizeof bag) {
            if (o) bag[o++] = '\n';
            memcpy(bag + o, mname, ln);
            o += ln;
          }
          bag[o] = 0;
          n++;
        }
      }
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LISTOVERRIDEMETHODS", bag);
    var_set_str(vm, "OVERRIDEMETHODS", bag);
    var_set_str(vm, "REDEFINEDMETHODS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "OVERRIDEMETHODS_N", n);
    var_set_num(vm, "LISTOVERRIDEMETHODS_N", n);
    var_set_num(vm, "REDEFINEDMETHODS_N", n);
    if (a[0]) var_set_str(vm, "CLASS", a);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* ISCLASS|ISA|OFCLASS|INSTANCEOF obj Class â soft 0|1 probe if live obj
   * is an instance of Class. Complements CLASSNAME + EQS without string glue.
   * Miss obj / wrong class / unknown Class â LAST_N=0 OK=1 (probe, not fail).
   * LAST = actual class name when live, else "". Usability: agent IF after
   * ENSURENEW/TRYNEW without CLASSNAME+EQS dual. */
  if (kw(&L->cur, "ISCLASS") || kw(&L->cur, "ISA") ||
      kw(&L->cur, "OFCLASS") || kw(&L->cur, "INSTANCEOF") ||
      kw(&L->cur, "IS_A") || kw(&L->cur, "ISINSTANCE") ||
      kw(&L->cur, "OBJISA") || kw(&L->cur, "TYPEIS")) {
    char oname[48], cname[48];
    ObjInst *ob;
    ClassDef *cd;
    int hit = 0;
    lex_next(L);
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "ISCLASS object Class"); return -1;
    }
    if (kw(&L->cur, "OF") || kw(&L->cur, "AS") || kw(&L->cur, "IS"))
      lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "ISCLASS object Class"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(cname, sizeof cname, "%s", L->cur.text);
      lex_next(L);
    } else {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      if (oop_find_class(vm, id))
        snprintf(cname, sizeof cname, "%s", id);
      else {
        vv = var_get(vm, id, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(cname, sizeof cname, "%s", vv->sval);
        else
          snprintf(cname, sizeof cname, "%s", id);
      }
    }
    ob = oop_find_obj(vm, oname);
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes) {
      cd = &vm->classes[ob->class_idx];
      var_set_str(vm, "LAST", cd->name);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", cd->name);
      var_set_str(vm, "CLASS", cd->name);
      var_set_str(vm, "OBJECT", oname);
      if (cname[0] && oop_isof_chain(vm, ob->class_idx, cname))
        hit = 1;
    } else {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
    }
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    var_set_num(vm, "ISCLASS_N", hit);
    var_set_num(vm, "ISA_N", hit);
    var_set_num(vm, "OK", 1);
    if (vm->trace) fprintf(vm->trace, "# ISCLASS %s %s -> %d\n", oname, cname, hit);
    bump(vm);
    return 1;
  }

  /* NEEDISA|REQUIRE ISA|NEEDCLASS_OF obj Class â fail-fast if not instance.
   * Soft dual is ISCLASS. Usability: agent contract after ENSURENEW. */
  if (kw(&L->cur, "NEEDISA") || kw(&L->cur, "NEEDISCLASS") ||
      kw(&L->cur, "REQUIREISA") || kw(&L->cur, "NEEDINSTANCEOF")) {
    char oname[48], cname[48];
    ObjInst *ob;
    ClassDef *cd;
    int hit = 0;
    lex_next(L);
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "NEEDISA object Class"); return -1;
    }
    if (kw(&L->cur, "OF") || kw(&L->cur, "AS") || kw(&L->cur, "IS"))
      lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "NEEDISA object Class"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(cname, sizeof cname, "%s", L->cur.text);
      lex_next(L);
    } else {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      if (oop_find_class(vm, id))
        snprintf(cname, sizeof cname, "%s", id);
      else {
        vv = var_get(vm, id, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(cname, sizeof cname, "%s", vv->sval);
        else
          snprintf(cname, sizeof cname, "%s", id);
      }
    }
    ob = oop_find_obj(vm, oname);
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes) {
      cd = &vm->classes[ob->class_idx];
      if (cname[0] && strcmp(cd->name, cname) == 0)
        hit = 1;
      else {
        snprintf(vm->err, sizeof vm->err,
                 "NEEDISA: %s is %s not %s", oname, cd->name, cname);
      }
    } else {
      snprintf(vm->err, sizeof vm->err, "NEEDISA: unknown object %s", oname);
    }
    if (!hit) {
      fail(vm, vm->err);
      return -1;
    }
    var_set_str(vm, "LAST", oname);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", oname);
    var_set_str(vm, "OBJECT", oname);
    var_set_str(vm, "CLASS", cname);
    var_set_num(vm, "LAST_N", 1);
    vm->last_n = 1;
    var_set_num(vm, "NEEDISA_N", 1);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }


  /* LISTCLASSES/LISTOBJS/PICK/SHUFFLE → lang_ops_flow_fstr_listobjs.c (B_fstr_b_peel) */
  {
    int lo = cubalc_lang_ops_flow_try_fstr_listobjs(vm, L);
    if (lo != 0) return lo;
  }



  /* LISTMETHODS Class|obj â newline bag of method names on class/object.
   * LAST_N / NMETHODS = count. Soft empty if unknown. String-var expands (METHOD_ON). */
  if (kw(&L->cur, "LISTMETHODS") || kw(&L->cur, "METHODS") ||
      kw(&L->cur, "METHODLIST") || kw(&L->cur, "LISTMETH")) {
    char a[48], bag[2048];
    ClassDef *cd = NULL;
    ObjInst *ob;
    size_t o = 0;
    int i, n = 0;
    lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "LISTMETHODS Class|obj"); return -1;
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
    if (ob)
      cd = &vm->classes[ob->class_idx];
    else
      cd = oop_find_class(vm, a);
    bag[0] = 0;
    if (cd) {
      for (i = 0; i < cd->n_methods; i++) {
        size_t ln = strlen(cd->methods[i].name);
        if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + ln < sizeof bag) {
          memcpy(bag + o, cd->methods[i].name, ln);
          o += ln;
        }
        bag[o] = 0;
        n++;
      }
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LISTMETHODS", bag);
    var_set_str(vm, "METHODS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "NMETHODS", n);
    var_set_num(vm, "LISTMETHODS_N", n);
    var_set_num(vm, "OK", cd ? 1 : 0);
    if (!cd) {
      var_set_str(vm, "LAST_ERR", "LISTMETHODS: unknown class/obj");
      var_set_str(vm, "ERR", "LISTMETHODS: unknown class/obj");
    }
    bump(vm);
    return 1;
  }

    /* BINDTHIS|USEOBJ|ASTHIS|SETHIS name â bind THIS/SELF to live object without SEND.
   * Soft miss OK=0 sticky LAST_ERR. Usability: multi-step GETF/SETF THIS after
   * ENSURENEW without inventing a method or hard-coding the slot name each line.
   * CLEARTHIS|UNBINDTHIS clears the binding. */
  if (kw(&L->cur, "BINDTHIS") || kw(&L->cur, "USEOBJ") ||
      kw(&L->cur, "ASTHIS") || kw(&L->cur, "SETHIS") ||
      kw(&L->cur, "USETHIS") || kw(&L->cur, "ASOBJ") ||
      kw(&L->cur, "FOCUSOBJ") || kw(&L->cur, "SELECTOBJ") ||
      kw(&L->cur, "TRYBINDTHIS") || kw(&L->cur, "TRYUSEOBJ")) {
    char oname[48];
    ObjInst *ob;
    ClassDef *cd;
    int soft = 1; /* always soft â bind is never fatal */
    lex_next(L);
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "BINDTHIS name"); return -1;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "BINDTHIS_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "BINDTHIS: unknown object");
      var_set_str(vm, "ERR", "BINDTHIS: unknown object");
      if (vm->trace) fprintf(vm->trace, "# BINDTHIS soft miss %s\n", oname);
      bump(vm);
      return 1;
    }
    snprintf(vm->this_obj, sizeof vm->this_obj, "%s", oname);
    var_set_str(vm, "THIS", oname);
    var_set_str(vm, "SELF", oname);
    var_set_str(vm, "OBJECT", oname);
    if (ob->class_idx >= 0 && ob->class_idx < vm->n_classes) {
      cd = &vm->classes[ob->class_idx];
      var_set_str(vm, "CLASS", cd->name);
    }
    var_set_str(vm, "LAST", oname);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", oname);
    var_set_num(vm, "LAST_N", 1);
    vm->last_n = 1;
    var_set_num(vm, "BINDTHIS_N", 1);
    var_set_num(vm, "OK", 1);
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "ERR", "");
    if (vm->trace) fprintf(vm->trace, "# BINDTHIS %s\n", oname);
    bump(vm);
    return 1;
  }

  /* CLEARTHIS|UNBINDTHIS|NOTHIS|DROPTHIS â clear THIS/SELF binding.
   * Always soft OK=1. LAST_N=1 if had binding, 0 if already empty. */
  if (kw(&L->cur, "CLEARTHIS") || kw(&L->cur, "UNBINDTHIS") ||
      kw(&L->cur, "NOTHIS") || kw(&L->cur, "DROPTHIS") ||
      kw(&L->cur, "RELEASETTHIS") || kw(&L->cur, "UNFOCUS") ||
      kw(&L->cur, "CLEAR_THIS")) {
    int had = vm->this_obj[0] ? 1 : 0;
    lex_next(L);
    vm->this_obj[0] = 0;
    var_set_str(vm, "THIS", "");
    var_set_str(vm, "SELF", "");
    var_set_num(vm, "LAST_N", had);
    vm->last_n = had;
    {
      char nb[8];
      snprintf(nb, sizeof nb, "%d", had);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "CLEARTHIS_N", had);
    var_set_num(vm, "OK", 1);
    if (vm->trace) fprintf(vm->trace, "# CLEARTHIS had=%d\n", had);
    bump(vm);
    return 1;
  }

  /* HASTHIS|BOUNDTHIS â soft 0|1 probe if THIS is bound to a live object.
   * LAST = bound name when live, else "". Usability: IF before SETF THIS. */
  if (kw(&L->cur, "HASTHIS") || kw(&L->cur, "BOUNDTHIS") ||
      kw(&L->cur, "THIS?") || kw(&L->cur, "HASTHISOBJ") ||
      kw(&L->cur, "THISBOUND")) {
    int hit = 0;
    lex_next(L);
    if (vm->this_obj[0] && oop_find_obj(vm, vm->this_obj)) {
      hit = 1;
      var_set_str(vm, "LAST", vm->this_obj);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", vm->this_obj);
    } else {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
    }
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    var_set_num(vm, "HASTHIS_N", hit);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }







  /* DUMPOBJ/OBJINFO → lang_ops_flow_fstr_dumpobj.c (B_fstr_b_peel) */
  {
    int dmp = cubalc_lang_ops_flow_try_fstr_dumpobj(vm, L);
    if (dmp != 0) return dmp;
  }

  return 0;
}
