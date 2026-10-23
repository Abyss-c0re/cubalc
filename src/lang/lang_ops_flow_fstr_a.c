/* flow fstr peel from flow_class */
#include "lang/cubalc_lang_ops_flow_fstr_a.h"
#include "lang/cubalc_lang_ops_flow_fstr_fieldbag.h"
#include "lang/cubalc_lang_ops_flow_fstr_slice.h"
#include "lang/cubalc_lang_ops_flow_fstr_core.h"
#include "lang/cubalc_lang_ops_flow_fstr_bagf.h"
#include "lang/cubalc_lang_ops_flow_fstr_catfall.h"
#include "lang/cubalc_lang_ops_flow_fstr_cutf.h"
#include "lang/cubalc_lang_ops_flow_fstr_oopchain.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_fstr_a(VM *vm, Lex *L) {
  /* field-bag EQF..ENSUREBAGLINE → lang_ops_flow_fstr_fieldbag.c */
  {
    int fb = cubalc_lang_ops_flow_try_fstr_fieldbag(vm, L);
    if (fb != 0) return fb;
  }

  /* slice LEFTF/BEFOREF → lang_ops_flow_fstr_slice.c (B_fstr_peel) */
  {
    int sl = cubalc_lang_ops_flow_try_fstr_slice(vm, L);
    if (sl != 0) return sl;
  }

  /* field core REPLACEF..CLEARF → lang_ops_flow_fstr_core.c (B_fstr_peel) */
  {
    int fc = cubalc_lang_ops_flow_try_fstr_core(vm, L);
    if (fc != 0) return fc;
  }

  /* bag-field PUSHF..LINESF → lang_ops_flow_fstr_bagf.c (B_fstr_peel) */
  {
    int bf = cubalc_lang_ops_flow_try_fstr_bagf(vm, L);
    if (bf != 0) return bf;
  }

  /* bulk CATFALL/PREPENDFALL → lang_ops_flow_fstr_catfall.c (B_fstr_peel) */
  {
    int cf = cubalc_lang_ops_flow_try_fstr_catfall(vm, L);
    if (cf != 0) return cf;
  }




  /* column CUTF/COLUMNF → lang_ops_flow_fstr_cutf.c (B_fstr_peel) */
  {
    int cu = cubalc_lang_ops_flow_try_fstr_cutf(vm, L);
    if (cu != 0) return cu;
  }




  /* NUMF|INTF|ATOIF obj field â coerce field to numeric (strtol).
   * STRF|ASSTRF|ITOAF obj field â coerce field to decimal string.
   * TRYNUMF|NUMF SOFT â soft miss OK=0.
   * LAST = decimal text always; LAST_N = numeric value.
   * NUMF_OK = 1 if parse consumed digits (partial "12x"â12 OK).
   * Usability: after AFTERF/BETWEENF peel, INCF without GETF+SYS NUM+SETF;
   * dual STRF for CATF/templates (METHOD/THIS). */
  if (kw(&L->cur, "NUMF") || kw(&L->cur, "INTF") ||
      kw(&L->cur, "ATOIF") || kw(&L->cur, "TONUMF") ||
      kw(&L->cur, "FIELDNUM") || kw(&L->cur, "ASNUMF") ||
      kw(&L->cur, "STRF") || kw(&L->cur, "ASSTRF") ||
      kw(&L->cur, "ITOAF") || kw(&L->cur, "TOSTRF") ||
      kw(&L->cur, "FIELDSTR") || kw(&L->cur, "NUMSTRF") ||
      kw(&L->cur, "TRYNUMF") || kw(&L->cur, "NUMFSOFT") ||
      kw(&L->cur, "SOFTNUMF") || kw(&L->cur, "TRYSTRF") ||
      kw(&L->cur, "TRYINTF")) {
    char oname[48], fname[48], op[24], hay[256], buf[40];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, to_str = 0, parse_ok = 0;
    long nval = 0;
    char *endp = NULL;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "STRF") == 0 || strcmp(op, "ASSTRF") == 0 ||
        strcmp(op, "ITOAF") == 0 || strcmp(op, "TOSTRF") == 0 ||
        strcmp(op, "FIELDSTR") == 0 || strcmp(op, "NUMSTRF") == 0 ||
        strcmp(op, "TRYSTRF") == 0)
      to_str = 1;
    if (strcmp(op, "TRYNUMF") == 0 || strcmp(op, "NUMFSOFT") == 0 ||
        strcmp(op, "SOFTNUMF") == 0 || strcmp(op, "TRYSTRF") == 0 ||
        strcmp(op, "TRYINTF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "NUMF object field"); return -1;
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
      fail(vm, "NUMF field"); return -1;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "NUMF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "NUMF_N", 0);
      var_set_num(vm, "STRF_N", 0);
      var_set_num(vm, "NUMF_OK", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "NUMF: unknown object");
      var_set_str(vm, "ERR", "NUMF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "NUMF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "NUMF_N", 0);
      var_set_num(vm, "STRF_N", 0);
      var_set_num(vm, "NUMF_OK", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "NUMF: unknown field");
      var_set_str(vm, "ERR", "NUMF: unknown field");
      bump(vm);
      return 1;
    }
    if (to_str) {
      /* force decimal string form */
      if (ob->fis_str[fi]) {
        nval = strtol(ob->fstr[fi], &endp, 10);
        parse_ok = (ob->fstr[fi][0] != 0 && endp != ob->fstr[fi]);
        if (parse_ok) {
          /* pure/leading-digit string â normalize decimal */
          snprintf(buf, sizeof buf, "%ld", nval);
          snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", buf);
        } else {
          /* non-numeric: keep text; LAST_N = 0 */
          nval = 0;
        }
      } else {
        nval = ob->fnum[fi];
        parse_ok = 1;
        snprintf(buf, sizeof buf, "%ld", nval);
        snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", buf);
      }
      ob->fis_str[fi] = 1;
      var_set_str(vm, "LAST", ob->fstr[fi]);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", ob->fstr[fi]);
      vm->last_n = nval;
      var_set_num(vm, "LAST_N", nval);
      var_set_num(vm, "STRF_N", 1);
      var_set_num(vm, "NUMF_N", 0);
      var_set_num(vm, "NUMF_OK", parse_ok ? 1 : 0);
    } else {
      /* coerce to numeric */
      if (ob->fis_str[fi]) {
        snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
        endp = NULL;
        nval = strtol(hay, &endp, 10);
        parse_ok = (hay[0] != 0 && endp != hay);
        if (!parse_ok) nval = 0;
      } else {
        nval = ob->fnum[fi];
        parse_ok = 1;
      }
      ob->fnum[fi] = nval;
      ob->fis_str[fi] = 0;
      snprintf(buf, sizeof buf, "%ld", nval);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      vm->last_n = nval;
      var_set_num(vm, "LAST_N", nval);
      var_set_num(vm, "NUMF_N", 1);
      var_set_num(vm, "STRF_N", 0);
      var_set_num(vm, "NUMF_OK", parse_ok ? 1 : 0);
      var_set_num(vm, "INTF_N", 1);
    }
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* OOP chain ISOF..HASANCESTOR → lang_ops_flow_fstr_oopchain.c (B_fstr_peel) */
  {
    int oc = cubalc_lang_ops_flow_try_fstr_oopchain(vm, L);
    if (oc != 0) return oc;
  }


  /* METHODORIGIN|DEFINEDIN|ORIGINOF|METHODOWNER|OWNEROF|METHODCLASS Class|obj method
   * multi-file EXTEND/link usability: nearest class that defines method.
   * LAST = class name. Soft empty if missing.
   * OVERRIDES|ISOVERRIDE|HASOWNMETHOD Class|obj method soft 0|1 own body. */
  if (kw(&L->cur, "METHODORIGIN") || kw(&L->cur, "DEFINEDIN") ||
      kw(&L->cur, "ORIGINOF") || kw(&L->cur, "METHODOWNER") ||
      kw(&L->cur, "OWNEROF") || kw(&L->cur, "METHODCLASS") ||
      kw(&L->cur, "WHEREDEFINED") || kw(&L->cur, "METHODFROM") ||
      kw(&L->cur, "SOURCEMETHOD") || kw(&L->cur, "METHOD_ORIGIN") ||
      kw(&L->cur, "OVERRIDES") || kw(&L->cur, "ISOVERRIDE") ||
      kw(&L->cur, "HASEOWN") || kw(&L->cur, "HASOWNMETHOD") ||
      kw(&L->cur, "OWNMETHOD") || kw(&L->cur, "IS_OVERRIDE") ||
      kw(&L->cur, "DEFINESMETHOD") || kw(&L->cur, "HAS_OWN_METHOD")) {
    int want_flag = kw(&L->cur, "OVERRIDES") || kw(&L->cur, "ISOVERRIDE") ||
                    kw(&L->cur, "HASEOWN") || kw(&L->cur, "HASOWNMETHOD") ||
                    kw(&L->cur, "OWNMETHOD") || kw(&L->cur, "IS_OVERRIDE") ||
                    kw(&L->cur, "DEFINESMETHOD") || kw(&L->cur, "HAS_OWN_METHOD");
    char a[48], mname[48];
    ClassDef *cd = NULL;
    ObjInst *ob;
    const char *origin = "";
    int own = 0, inherited = 0, guard = 0, pi;
    int found = 0;
    lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, want_flag ? "OVERRIDES Class|obj method"
                         : "METHODORIGIN Class|obj method");
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
    if (kw(&L->cur, "METHOD") || kw(&L->cur, "METH") || kw(&L->cur, "OF"))
      lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, want_flag ? "OVERRIDES Class|obj method"
                         : "METHODORIGIN Class|obj method");
      return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    } else {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      vv = var_get(vm, id, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mname, sizeof mname, "%s", vv->sval);
      else if (strcmp(id, "LAST") == 0)
        snprintf(mname, sizeof mname, "%s", vm->last_str);
      else
        snprintf(mname, sizeof mname, "%s", id);
    }
    ob = oop_find_obj(vm, a);
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes)
      cd = &vm->classes[ob->class_idx];
    else
      cd = oop_find_class(vm, a);
    if (cd) {
      MethodDef *md_self = oop_find_method(cd, mname);
      if (md_self) {
        MethodDef *md_par = NULL;
        pi = cd->parent_idx;
        guard = 0;
        while (pi >= 0 && pi < vm->n_classes && guard++ < CUBALC_MAX_CLASSES) {
          ClassDef *pw = &vm->classes[pi];
          MethodDef *pm = oop_find_method(pw, mname);
          if (pm) { md_par = pm; break; }
          pi = pw->parent_idx;
        }
        if (!md_par) {
          own = 1;
          origin = cd->name;
          found = 1;
        } else if (md_self->body != md_par->body || md_self->len != md_par->len) {
          own = 1;
          inherited = 1;
          origin = cd->name;
          found = 1;
        } else {
          inherited = 1;
          pi = cd->parent_idx;
          guard = 0;
          origin = "";
          while (pi >= 0 && pi < vm->n_classes && guard++ < CUBALC_MAX_CLASSES) {
            ClassDef *pw = &vm->classes[pi];
            MethodDef *pm = oop_find_method(pw, mname);
            if (!pm) { pi = pw->parent_idx; continue; }
            if (pm->body == md_self->body && pm->len == md_self->len) {
              origin = pw->name;
              if (pw->parent_idx >= 0 && pw->parent_idx < vm->n_classes) {
                MethodDef *gp = NULL;
                int gpi = pw->parent_idx, gg = 0;
                while (gpi >= 0 && gpi < vm->n_classes && gg++ < CUBALC_MAX_CLASSES) {
                  ClassDef *gw = &vm->classes[gpi];
                  gp = oop_find_method(gw, mname);
                  if (gp) break;
                  gpi = gw->parent_idx;
                }
                if (!gp || gp->body != pm->body || gp->len != pm->len) {
                  found = 1;
                  break;
                }
              } else {
                found = 1;
                break;
              }
              pi = pw->parent_idx;
              continue;
            } else {
              found = 1;
              break;
            }
          }
          if (!found && origin[0]) found = 1;
        }
      } else {
        pi = cd->parent_idx;
        guard = 0;
        while (pi >= 0 && pi < vm->n_classes && guard++ < CUBALC_MAX_CLASSES) {
          ClassDef *pw = &vm->classes[pi];
          MethodDef *pm = oop_find_method(pw, mname);
          if (pm) {
            origin = pw->name;
            inherited = 1;
            found = 1;
            break;
          }
          pi = pw->parent_idx;
        }
      }
    }
    if (want_flag) {
      var_set_num(vm, "LAST_N", own ? 1 : 0);
      vm->last_n = own ? 1 : 0;
      snprintf(vm->last_str, sizeof vm->last_str, "%s", own ? "1" : "0");
      var_set_str(vm, "LAST", vm->last_str);
      var_set_num(vm, "OVERRIDES_N", own ? 1 : 0);
      var_set_num(vm, "HASOWNMETHOD_N", own ? 1 : 0);
      var_set_num(vm, "OWNMETHOD_N", own ? 1 : 0);
    } else {
      var_set_str(vm, "LAST", origin);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", origin);
      var_set_str(vm, "ORIGIN", origin);
      var_set_str(vm, "METHODORIGIN", origin);
      var_set_str(vm, "DEFINEDIN", origin);
      var_set_str(vm, "OWNER", origin);
      var_set_str(vm, "METHODCLASS", origin);
      var_set_num(vm, "LAST_N", found && origin[0] ? 1 : 0);
      vm->last_n = found && origin[0] ? 1 : 0;
      var_set_num(vm, "METHODORIGIN_N", found && origin[0] ? 1 : 0);
      var_set_num(vm, "DEFINEDIN_N", found && origin[0] ? 1 : 0);
    }
    var_set_str(vm, "METHOD", mname);
    if (cd) var_set_str(vm, "CLASS", cd->name);
    var_set_num(vm, "HASMETHOD_N", (found || own) ? 1 : 0);
    if (own && inherited) {
      var_set_num(vm, "INHERITED_N", 1);
      var_set_num(vm, "OVERRIDE_N", 1);
      var_set_num(vm, "INTRODUCED_N", 0);
    } else if (own) {
      var_set_num(vm, "INHERITED_N", 0);
      var_set_num(vm, "OVERRIDE_N", 0);
      var_set_num(vm, "INTRODUCED_N", 1);
    } else if (inherited) {
      var_set_num(vm, "INHERITED_N", 1);
      var_set_num(vm, "OVERRIDE_N", 0);
      var_set_num(vm, "INTRODUCED_N", 0);
    } else {
      var_set_num(vm, "INHERITED_N", 0);
      var_set_num(vm, "OVERRIDE_N", 0);
      var_set_num(vm, "INTRODUCED_N", 0);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s.%s -> origin=%s own=%d\n",
              want_flag ? "OVERRIDES" : "METHODORIGIN", a, mname,
              origin[0] ? origin : "-", own);
    bump(vm);
    return 1;
  }

  /* FIELDORIGIN|FIELDFROM|DEFINEDFIELD|FIELDOWNER|SOURCEFIELD|FIELDCLASS Class|obj field
   * multi-file EXTEND/link usability: nearest class that defines/overrides field default.
   * LAST = class name. Soft empty if missing.
   * HASOWNFIELD|OWNFIELD|DEFINESFIELD|OVERRIDESFIELD Class|obj field soft 0|1 own slot. */
  if (kw(&L->cur, "FIELDORIGIN") || kw(&L->cur, "FIELDFROM") ||
      kw(&L->cur, "DEFINEDFIELD") || kw(&L->cur, "FIELDOWNER") ||
      kw(&L->cur, "SOURCEFIELD") || kw(&L->cur, "FIELDCLASS") ||
      kw(&L->cur, "WHEREFIELD") || kw(&L->cur, "FIELD_ORIGIN") ||
      kw(&L->cur, "ORIGINFIELD") || kw(&L->cur, "FIELDDEFINEDIN") ||
      kw(&L->cur, "HASOWNFIELD") || kw(&L->cur, "OWNFIELD") ||
      kw(&L->cur, "DEFINESFIELD") || kw(&L->cur, "OVERRIDESFIELD") ||
      kw(&L->cur, "HAS_OWN_FIELD") || kw(&L->cur, "ISOWNFIELD") ||
      kw(&L->cur, "OWN_FIELD") || kw(&L->cur, "FIELD_OWN")) {
    int want_flag = kw(&L->cur, "HASOWNFIELD") || kw(&L->cur, "OWNFIELD") ||
                    kw(&L->cur, "DEFINESFIELD") || kw(&L->cur, "OVERRIDESFIELD") ||
                    kw(&L->cur, "HAS_OWN_FIELD") || kw(&L->cur, "ISOWNFIELD") ||
                    kw(&L->cur, "OWN_FIELD") || kw(&L->cur, "FIELD_OWN");
    char a[48], fname[48];
    ClassDef *cd = NULL;
    ObjInst *ob;
    const char *origin = "";
    int own = 0, inherited = 0, introduced = 0, overridden = 0;
    int guard = 0, fi;
    int found = 0;
    FieldDef *fd_self = NULL;
    lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, want_flag ? "HASOWNFIELD Class|obj field"
                         : "FIELDORIGIN Class|obj field");
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
    if (kw(&L->cur, "FIELD") || kw(&L->cur, "PROP") || kw(&L->cur, "OF") ||
        kw(&L->cur, "ATTR") || kw(&L->cur, "MEMBER"))
      lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, want_flag ? "HASOWNFIELD Class|obj field"
                         : "FIELDORIGIN Class|obj field");
      return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(fname, sizeof fname, "%s", L->cur.text);
      lex_next(L);
    } else {
      char id[48];
      Var *vv;
      snprintf(id, sizeof id, "%s", L->cur.text);
      lex_next(L);
      vv = var_get(vm, id, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(fname, sizeof fname, "%s", vv->sval);
      else if (strcmp(id, "LAST") == 0)
        snprintf(fname, sizeof fname, "%s", vm->last_str);
      else
        snprintf(fname, sizeof fname, "%s", id);
    }
    ob = oop_find_obj(vm, a);
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes)
      cd = &vm->classes[ob->class_idx];
    else
      cd = oop_find_class(vm, a);
    if (cd) {
      ClassDef *slot_cd = NULL;
      FieldDef *slot_fd = NULL;
      ClassDef *w = cd;
      guard = 0;
      while (w && guard++ < CUBALC_MAX_CLASSES) {
        fi = oop_field_idx(w, fname);
        if (fi >= 0) {
          slot_cd = w;
          slot_fd = &w->fields[fi];
          break;
        }
        if (w->parent_idx < 0 || w->parent_idx >= vm->n_classes) break;
        w = &vm->classes[w->parent_idx];
      }
      if (slot_cd && slot_fd) {
        ClassDef *origin_cd = slot_cd;
        FieldDef *origin_fd = slot_fd;
        int g2 = 0;
        found = 1;
        fd_self = slot_fd;
        w = slot_cd;
        while (w->parent_idx >= 0 && w->parent_idx < vm->n_classes &&
               g2++ < CUBALC_MAX_CLASSES) {
          ClassDef *par = &vm->classes[w->parent_idx];
          int pfi2 = oop_field_idx(par, fname);
          if (pfi2 < 0) break;
          if (!oop_field_def_same(origin_fd, &par->fields[pfi2])) break;
          origin_cd = par;
          origin_fd = &par->fields[pfi2];
          w = par;
        }
        origin = origin_cd->name;
        if (origin_cd == cd) {
          own = 1;
          if (cd->parent_idx >= 0 && cd->parent_idx < vm->n_classes) {
            ClassDef *walk = &vm->classes[cd->parent_idx];
            int g3 = 0;
            while (walk && g3++ < CUBALC_MAX_CLASSES) {
              if (oop_field_idx(walk, fname) >= 0) {
                inherited = 1;
                overridden = 1;
                break;
              }
              if (walk->parent_idx < 0 || walk->parent_idx >= vm->n_classes) break;
              walk = &vm->classes[walk->parent_idx];
            }
          }
          if (!inherited) introduced = 1;
        } else {
          inherited = 1;
          own = 0;
          if (origin_cd->parent_idx >= 0 && origin_cd->parent_idx < vm->n_classes) {
            ClassDef *op = &vm->classes[origin_cd->parent_idx];
            int pfi = oop_field_idx(op, fname);
            if (pfi >= 0) {
              if (!oop_field_def_same(origin_fd, &op->fields[pfi]))
                overridden = 1;
            } else {
              introduced = 1;
            }
          } else {
            introduced = 1;
          }
        }
      }
    }
    if (want_flag) {
      var_set_num(vm, "LAST_N", own ? 1 : 0);
      vm->last_n = own ? 1 : 0;
      snprintf(vm->last_str, sizeof vm->last_str, "%s", own ? "1" : "0");
      var_set_str(vm, "LAST", vm->last_str);
      var_set_num(vm, "HASOWNFIELD_N", own ? 1 : 0);
      var_set_num(vm, "OWNFIELD_N", own ? 1 : 0);
      var_set_num(vm, "OVERRIDESFIELD_N", (own && overridden) ? 1 : 0);
      var_set_num(vm, "DEFINESFIELD_N", own ? 1 : 0);
    } else {
      var_set_str(vm, "LAST", origin);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", origin);
      var_set_str(vm, "ORIGIN", origin);
      var_set_str(vm, "FIELDORIGIN", origin);
      var_set_str(vm, "DEFINEDFIELD", origin);
      var_set_str(vm, "FIELDOWNER", origin);
      var_set_str(vm, "FIELDCLASS", origin);
      var_set_str(vm, "SOURCEFIELD", origin);
      var_set_str(vm, "WHEREFIELD", origin);
      var_set_num(vm, "LAST_N", found && origin[0] ? 1 : 0);
      vm->last_n = found && origin[0] ? 1 : 0;
      var_set_num(vm, "FIELDORIGIN_N", found && origin[0] ? 1 : 0);
      var_set_num(vm, "DEFINEDFIELD_N", found && origin[0] ? 1 : 0);
    }
    var_set_str(vm, "FIELD", fname);
    if (cd) var_set_str(vm, "CLASS", cd->name);
    var_set_num(vm, "HASFIELD_N", found ? 1 : 0);
    var_set_num(vm, "INHERITED_N", inherited ? 1 : 0);
    var_set_num(vm, "OVERRIDE_N", overridden ? 1 : 0);
    var_set_num(vm, "INTRODUCED_N", introduced ? 1 : 0);
    var_set_num(vm, "OK", 1);
    (void)fd_self;
    if (vm->trace)
      fprintf(vm->trace, "# %s %s.%s -> origin=%s own=%d ovr=%d intro=%d\n",
              want_flag ? "HASOWNFIELD" : "FIELDORIGIN", a, fname,
              origin[0] ? origin : "-", own, overridden, introduced);
    bump(vm);
    return 1;
  }


  /* LISTOWNFIELDS|OWNFIELDS|LOCALFIELDS Class|obj — newline bag of fields whose
   * default is owned on this class (HASOWNFIELD=1), not pure inherit.
   * Multi-file EXTEND pack contribution probe. LAST = bag; LAST_N = count.
   * Complements FIELDORIGIN + HASOWNFIELD + LISTFIELDS. */
  if (kw(&L->cur, "LISTOWNFIELDS") || kw(&L->cur, "OWNFIELDS") ||
      kw(&L->cur, "LOCALFIELDS") || kw(&L->cur, "OWN_FIELDS") ||
      kw(&L->cur, "LISTLOCALFIELDS") || kw(&L->cur, "FIELDS_OWN")) {
    char a[48], bag[2048];
    ClassDef *cd = NULL;
    ObjInst *ob;
    size_t o = 0;
    int n = 0, fi;
    lex_next(L);
    a[0] = bag[0] = 0;
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "LISTOWNFIELDS Class|obj");
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
        int own = 0;
        if (cd->parent_idx < 0 || cd->parent_idx >= vm->n_classes) {
          own = 1;
        } else {
          ClassDef *walk = &vm->classes[cd->parent_idx];
          int g = 0, seen = 0;
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
          if (!seen) own = 1; /* introduced on this class */
        }
        if (!own) continue;
        {
          size_t ln = strlen(fname);
          if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
          if (o + ln < sizeof bag) {
            memcpy(bag + o, fname, ln);
            o += ln;
          }
          bag[o] = 0;
          n++;
        }
      }
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LISTOWNFIELDS", bag);
    var_set_str(vm, "OWNFIELDS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "OWNFIELDS_N", n);
    var_set_num(vm, "LISTOWNFIELDS_N", n);
    if (a[0]) var_set_str(vm, "CLASS", a);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LISTOWNMETHODS|OWNMETHODS|LOCALMETHODS Class|obj — newline bag of methods
   * whose body is owned on this class (OVERRIDES/HASOWNMETHOD=1).
   * Multi-file EXTEND pack contribution probe. LAST = bag; LAST_N = count.
   * Complements METHODORIGIN + OVERRIDES + LISTMETHODS. */
  if (kw(&L->cur, "LISTOWNMETHODS") || kw(&L->cur, "OWNMETHODS") ||
      kw(&L->cur, "LOCALMETHODS") || kw(&L->cur, "OWN_METHODS") ||
      kw(&L->cur, "LISTLOCALMETHODS") || kw(&L->cur, "METHODS_OWN")) {
    char a[48], bag[2048];
    ClassDef *cd = NULL;
    ObjInst *ob;
    size_t o = 0;
    int n = 0, mi;
    lex_next(L);
    a[0] = bag[0] = 0;
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "LISTOWNMETHODS Class|obj");
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
        int own = 1;
        if (cd->parent_idx >= 0 && cd->parent_idx < vm->n_classes) {
          ClassDef *walk = &vm->classes[cd->parent_idx];
          int pguard = 0;
          while (walk && pguard++ < CUBALC_MAX_CLASSES) {
            MethodDef *pm = oop_find_method(walk, mname);
            if (pm) {
              if (pm->body == md->body && pm->len == md->len)
                own = 0; /* pure inherit same body pointer */
              break;
            }
            if (walk->parent_idx < 0 || walk->parent_idx >= vm->n_classes) break;
            walk = &vm->classes[walk->parent_idx];
          }
        }
        if (!own) continue;
        {
          size_t ln = strlen(mname);
          if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
          if (o + ln < sizeof bag) {
            memcpy(bag + o, mname, ln);
            o += ln;
          }
          bag[o] = 0;
          n++;
        }
      }
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LISTOWNMETHODS", bag);
    var_set_str(vm, "OWNMETHODS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "OWNMETHODS_N", n);
    var_set_num(vm, "LISTOWNMETHODS_N", n);
    if (a[0]) var_set_str(vm, "CLASS", a);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LISTINHERITEDFIELDS|INHERITEDFIELDS|PARENTFIELDS Class|obj — newline bag of
   * fields present on class/obj but NOT owned here (pure inherit from ancestors).
   * Multi-file EXTEND pack: what this unit reuses vs contributes.
   * Dual of LISTOWNFIELDS. LAST = bag; LAST_N = count. Soft empty OK=1. */
  if (kw(&L->cur, "LISTINHERITEDFIELDS") || kw(&L->cur, "INHERITEDFIELDS") ||
      kw(&L->cur, "PARENTFIELDS") || kw(&L->cur, "INHERIT_FIELDS") ||
      kw(&L->cur, "LISTPARENTFIELDS") || kw(&L->cur, "FIELDS_INHERITED")) {
    char a[48], bag[2048];
    ClassDef *cd = NULL;
    ObjInst *ob;
    size_t o = 0;
    int n = 0, fi;
    lex_next(L);
    a[0] = bag[0] = 0;
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "LISTINHERITEDFIELDS Class|obj");
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
        int own = 0;
        if (cd->parent_idx < 0 || cd->parent_idx >= vm->n_classes) {
          own = 1; /* root owns all */
        } else {
          ClassDef *walk = &vm->classes[cd->parent_idx];
          int g = 0, seen = 0;
          while (walk && g++ < CUBALC_MAX_CLASSES) {
            int pfi = oop_field_idx(walk, fname);
            if (pfi >= 0) {
              seen = 1;
              if (!oop_field_def_same(fd, &walk->fields[pfi]))
                own = 1; /* override default = own */
              break;
            }
            if (walk->parent_idx < 0 || walk->parent_idx >= vm->n_classes) break;
            walk = &vm->classes[walk->parent_idx];
          }
          if (!seen) own = 1; /* introduced here */
        }
        if (!own) {
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
    var_set_str(vm, "LISTINHERITEDFIELDS", bag);
    var_set_str(vm, "INHERITEDFIELDS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "INHERITEDFIELDS_N", n);
    var_set_num(vm, "LISTINHERITEDFIELDS_N", n);
    if (a[0]) var_set_str(vm, "CLASS", a);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LISTINHERITEDMETHODS|INHERITEDMETHODS|PARENTMETHODS Class|obj — newline bag
   * of methods present but NOT owned here (pure inherit same body).
   * Multi-file EXTEND pack reuse probe. Dual of LISTOWNMETHODS.
   * LAST = bag; LAST_N = count. Soft empty OK=1. */
  if (kw(&L->cur, "LISTINHERITEDMETHODS") || kw(&L->cur, "INHERITEDMETHODS") ||
      kw(&L->cur, "PARENTMETHODS") || kw(&L->cur, "INHERIT_METHODS") ||
      kw(&L->cur, "LISTPARENTMETHODS") || kw(&L->cur, "METHODS_INHERITED")) {
    char a[48], bag[2048];
    ClassDef *cd = NULL;
    ObjInst *ob;
    size_t o = 0;
    int n = 0, mi;
    lex_next(L);
    a[0] = bag[0] = 0;
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "LISTINHERITEDMETHODS Class|obj");
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
        int own = 1;
        if (cd->parent_idx >= 0 && cd->parent_idx < vm->n_classes) {
          ClassDef *walk = &vm->classes[cd->parent_idx];
          int pguard = 0;
          while (walk && pguard++ < CUBALC_MAX_CLASSES) {
            MethodDef *pm = oop_find_method(walk, mname);
            if (pm) {
              if (pm->body == md->body && pm->len == md->len)
                own = 0; /* pure inherit same body pointer */
              break;
            }
            if (walk->parent_idx < 0 || walk->parent_idx >= vm->n_classes) break;
            walk = &vm->classes[walk->parent_idx];
          }
        }
        if (!own) {
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
    var_set_str(vm, "LISTINHERITEDMETHODS", bag);
    var_set_str(vm, "INHERITEDMETHODS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "INHERITEDMETHODS_N", n);
    var_set_num(vm, "LISTINHERITEDMETHODS_N", n);
    if (a[0]) var_set_str(vm, "CLASS", a);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* COMMONANCESTOR|LCA|LOWESTCOMMON|COMMONBASE Class|obj Class|obj
   * — multi-file EXTEND/link usability: lowest common ancestor class name.
   * Walk B ancestors into a set, then walk A up until hit. LAST = LCA name.
   * Soft empty if disjoint / unknown. Complements HASANCESTOR + ROOTOF + PARENTS.
   * SAMECHAIN|SHARECHAIN|COCHAIN|SAMEFAMILY|ISRELATED Class|obj Class|obj — soft 0|1 if
   * same EXTEND family (LCA exists OR either is ancestor of other OR same class).
   * Cube Law: free energy must flow; meaningful growth only. */
  if (kw(&L->cur, "COMMONANCESTOR") || kw(&L->cur, "LCA") ||
      kw(&L->cur, "LOWESTCOMMON") || kw(&L->cur, "COMMONBASE") ||
      kw(&L->cur, "COMMON_ANCESTOR") || kw(&L->cur, "LOWEST_COMMON") ||
      kw(&L->cur, "COMMONPARENT") || kw(&L->cur, "SHAREDBASE") ||
      kw(&L->cur, "COMMONROOT") || kw(&L->cur, "LCACLASS") ||
      kw(&L->cur, "SAMECHAIN") || kw(&L->cur, "SHARECHAIN") ||
      kw(&L->cur, "COCHAIN") || kw(&L->cur, "CHAINMATES") ||
      kw(&L->cur, "SAME_CHAIN") || kw(&L->cur, "SHARE_CHAIN") ||
      kw(&L->cur, "ONCHAIN") || kw(&L->cur, "FAMILYOF") ||
      kw(&L->cur, "SAMEFAMILY") || kw(&L->cur, "ISRELATED")) {
    int want_flag = kw(&L->cur, "SAMECHAIN") || kw(&L->cur, "SHARECHAIN") ||
                    kw(&L->cur, "COCHAIN") || kw(&L->cur, "CHAINMATES") ||
                    kw(&L->cur, "SAME_CHAIN") || kw(&L->cur, "SHARE_CHAIN") ||
                    kw(&L->cur, "ONCHAIN") || kw(&L->cur, "FAMILYOF") ||
                    kw(&L->cur, "SAMEFAMILY") || kw(&L->cur, "ISRELATED");
    char a[48], b[48];
    ClassDef *cda = NULL, *cdb = NULL;
    ObjInst *ob;
    int ai = -1, bi = -1;
    int b_anc[CUBALC_MAX_CLASSES];
    int b_n = 0, guard = 0, pi, i, hit = 0;
    const char *lca = "";
    int same = 0;
    lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, want_flag ? "SAMECHAIN Class|obj Class|obj"
                         : "COMMONANCESTOR Class|obj Class|obj");
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
    if (kw(&L->cur, "AND") || kw(&L->cur, "WITH") || kw(&L->cur, "TO") ||
        kw(&L->cur, "OF") || kw(&L->cur, "VS") || kw(&L->cur, "X"))
      lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, want_flag ? "SAMECHAIN Class|obj Class|obj"
                         : "COMMONANCESTOR Class|obj Class|obj");
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
    if (cda && cdb && ai >= 0 && bi >= 0) {
      if (ai == bi) {
        lca = cda->name;
        same = 1;
        hit = 1;
      } else {
        /* collect B + ancestors of B */
        pi = bi;
        guard = 0;
        while (pi >= 0 && pi < vm->n_classes && guard++ < CUBALC_MAX_CLASSES) {
          if (b_n < CUBALC_MAX_CLASSES) b_anc[b_n++] = pi;
          pi = vm->classes[pi].parent_idx;
        }
        /* walk A + ancestors; first membership in B-set is LCA (nearest to A) */
        pi = ai;
        guard = 0;
        while (pi >= 0 && pi < vm->n_classes && guard++ < CUBALC_MAX_CLASSES) {
          for (i = 0; i < b_n; i++) {
            if (b_anc[i] == pi) {
              lca = vm->classes[pi].name;
              hit = 1;
              same = 1;
              guard = CUBALC_MAX_CLASSES; /* break outer */
              break;
            }
          }
          if (hit) break;
          pi = vm->classes[pi].parent_idx;
        }
      }
    }
    if (want_flag) {
      var_set_num(vm, "LAST_N", same ? 1 : 0);
      vm->last_n = same ? 1 : 0;
      var_set_num(vm, "SAMECHAIN_N", same ? 1 : 0);
      var_set_num(vm, "RELATED_N", same ? 1 : 0);
      var_set_num(vm, "COCHAIN_N", same ? 1 : 0);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", same ? "1" : "0");
      var_set_str(vm, "LAST", vm->last_str);
      if (lca[0]) {
        var_set_str(vm, "LCA", lca);
        var_set_str(vm, "COMMONANCESTOR", lca);
        var_set_str(vm, "COMMON", lca);
      }
    } else {
      var_set_str(vm, "LAST", lca);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", lca);
      var_set_num(vm, "LAST_N", hit && lca[0] ? 1 : 0);
      vm->last_n = hit && lca[0] ? 1 : 0;
      var_set_num(vm, "COMMONANCESTOR_N", hit && lca[0] ? 1 : 0);
      var_set_num(vm, "LCA_N", hit && lca[0] ? 1 : 0);
      if (lca[0]) {
        var_set_str(vm, "LCA", lca);
        var_set_str(vm, "COMMONANCESTOR", lca);
        var_set_str(vm, "COMMON", lca);
        var_set_str(vm, "ORIGIN", lca);
      }
      var_set_num(vm, "SAMECHAIN_N", same ? 1 : 0);
    }
    if (cda) var_set_str(vm, "CLASS", cda->name);
    if (cdb) var_set_str(vm, "OTHER", cdb->name);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s %s -> lca=%s same=%d\n",
              want_flag ? "SAMECHAIN" : "COMMONANCESTOR", a, b,
              lca[0] ? lca : "-", same);
    bump(vm);
    return 1;
  }


  /* CHAINDIST|CLASSDIST|EXTENDDIST|DISTBETWEEN|HOPBETWEEN Class|obj Class|obj
   * multi-file EXTEND/link usability: hop distance on the single-inheritance
   * graph via LCA. Same class -> 0. One ancestor of other -> depth delta.
   * Related via LCA -> depth(A,LCA)+depth(B,LCA). Unrelated soft LAST_N=-1.
   * PATHBETWEEN|CLASSPATH|CHAINPATH|EXTENDPATH|PATHTO Class|obj Class|obj
   * newline bag of class names A ... B (via LCA). Soft empty if unrelated.
   * Complements COMMONANCESTOR/SAMECHAIN + DEPTHOF + PARENTS. Cube is SoT. */
  if (kw(&L->cur, "CHAINDIST") || kw(&L->cur, "CLASSDIST") ||
      kw(&L->cur, "EXTENDDIST") || kw(&L->cur, "DISTBETWEEN") ||
      kw(&L->cur, "HOPBETWEEN") || kw(&L->cur, "CHAIN_DIST") ||
      kw(&L->cur, "CLASS_DIST") || kw(&L->cur, "EXTEND_DIST") ||
      kw(&L->cur, "DIST_BETWEEN") || kw(&L->cur, "HOPSBETWEEN") ||
      kw(&L->cur, "HOPSTO") || kw(&L->cur, "DISTANCEOF") ||
      kw(&L->cur, "PATHBETWEEN") || kw(&L->cur, "CLASSPATH") ||
      kw(&L->cur, "CHAINPATH") || kw(&L->cur, "EXTENDPATH") ||
      kw(&L->cur, "PATHTO") || kw(&L->cur, "PATH_BETWEEN") ||
      kw(&L->cur, "CLASS_PATH") || kw(&L->cur, "CHAIN_PATH") ||
      kw(&L->cur, "ANCESTORPATH") || kw(&L->cur, "PATHFROM")) {
    int want_path = kw(&L->cur, "PATHBETWEEN") || kw(&L->cur, "CLASSPATH") ||
                    kw(&L->cur, "CHAINPATH") || kw(&L->cur, "EXTENDPATH") ||
                    kw(&L->cur, "PATHTO") || kw(&L->cur, "PATH_BETWEEN") ||
                    kw(&L->cur, "CLASS_PATH") || kw(&L->cur, "CHAIN_PATH") ||
                    kw(&L->cur, "ANCESTORPATH") || kw(&L->cur, "PATHFROM");
    char a[48], b[48];
    ClassDef *cda = NULL, *cdb = NULL;
    ObjInst *ob;
    int ai = -1, bi = -1;
    int a_anc[CUBALC_MAX_CLASSES];
    int b_anc[CUBALC_MAX_CLASSES];
    int a_n = 0, b_n = 0, guard = 0, pi, i, j;
    int lca_i = -1, hit = 0;
    int dist = -1;
    int da = 0, db = 0;
    char bag[2048];
    size_t o = 0;
    int pn = 0;
    lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, want_path ? "PATHBETWEEN Class|obj Class|obj"
                         : "CHAINDIST Class|obj Class|obj");
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
    if (kw(&L->cur, "AND") || kw(&L->cur, "WITH") || kw(&L->cur, "TO") ||
        kw(&L->cur, "OF") || kw(&L->cur, "VS") || kw(&L->cur, "X") ||
        kw(&L->cur, "FROM"))
      lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, want_path ? "PATHBETWEEN Class|obj Class|obj"
                         : "CHAINDIST Class|obj Class|obj");
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
    bag[0] = 0;
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
    if (cda && cdb && ai >= 0 && bi >= 0) {
      pi = ai;
      guard = 0;
      while (pi >= 0 && pi < vm->n_classes && guard++ < CUBALC_MAX_CLASSES) {
        if (a_n < CUBALC_MAX_CLASSES) a_anc[a_n++] = pi;
        pi = vm->classes[pi].parent_idx;
      }
      pi = bi;
      guard = 0;
      while (pi >= 0 && pi < vm->n_classes && guard++ < CUBALC_MAX_CLASSES) {
        if (b_n < CUBALC_MAX_CLASSES) b_anc[b_n++] = pi;
        pi = vm->classes[pi].parent_idx;
      }
      for (i = 0; i < a_n && !hit; i++) {
        for (j = 0; j < b_n; j++) {
          if (a_anc[i] == b_anc[j]) {
            lca_i = a_anc[i];
            da = i;
            db = j;
            hit = 1;
            break;
          }
        }
      }
      if (hit) {
        dist = da + db;
        o = 0;
        pn = 0;
        for (i = 0; i <= da; i++) {
          const char *nm = vm->classes[a_anc[i]].name;
          size_t ln = strlen(nm);
          if (pn > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
          if (o + ln < sizeof bag) {
            memcpy(bag + o, nm, ln);
            o += ln;
          }
          pn++;
        }
        for (j = db - 1; j >= 0; j--) {
          const char *nm = vm->classes[b_anc[j]].name;
          size_t ln = strlen(nm);
          if (pn > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
          if (o + ln < sizeof bag) {
            memcpy(bag + o, nm, ln);
            o += ln;
          }
          pn++;
        }
        if (o < sizeof bag) bag[o] = 0;
      }
    }
    if (want_path) {
      var_set_str(vm, "LAST", bag);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
      var_set_num(vm, "LAST_N", hit ? pn : 0);
      vm->last_n = hit ? pn : 0;
      var_set_str(vm, "PATHBETWEEN", bag);
      var_set_str(vm, "CLASSPATH", bag);
      var_set_str(vm, "CHAINPATH", bag);
      var_set_str(vm, "PATH", bag);
      var_set_num(vm, "PATHBETWEEN_N", hit ? pn : 0);
      var_set_num(vm, "CLASSPATH_N", hit ? pn : 0);
      var_set_num(vm, "PATH_N", hit ? pn : 0);
      if (dist >= 0) {
        var_set_num(vm, "CHAINDIST_N", dist);
        var_set_num(vm, "DIST_N", dist);
      } else {
        var_set_num(vm, "CHAINDIST_N", -1);
        var_set_num(vm, "DIST_N", -1);
      }
    } else {
      if (dist >= 0) {
        var_set_num(vm, "LAST_N", dist);
        vm->last_n = dist;
        snprintf(vm->last_str, sizeof vm->last_str, "%d", dist);
        var_set_str(vm, "LAST", vm->last_str);
        var_set_num(vm, "CHAINDIST_N", dist);
        var_set_num(vm, "CLASSDIST_N", dist);
        var_set_num(vm, "DIST_N", dist);
        var_set_num(vm, "HOPS_N", dist);
      } else {
        var_set_num(vm, "LAST_N", -1);
        vm->last_n = -1;
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "-1");
        var_set_str(vm, "LAST", vm->last_str);
        var_set_num(vm, "CHAINDIST_N", -1);
        var_set_num(vm, "CLASSDIST_N", -1);
        var_set_num(vm, "DIST_N", -1);
        var_set_num(vm, "HOPS_N", -1);
      }
      if (bag[0]) {
        var_set_str(vm, "PATHBETWEEN", bag);
        var_set_str(vm, "PATH", bag);
        var_set_num(vm, "PATH_N", pn);
      }
    }
    if (hit && lca_i >= 0 && lca_i < vm->n_classes) {
      var_set_str(vm, "LCA", vm->classes[lca_i].name);
      var_set_str(vm, "COMMONANCESTOR", vm->classes[lca_i].name);
      var_set_str(vm, "COMMON", vm->classes[lca_i].name);
      var_set_num(vm, "LCA_N", 1);
    } else {
      var_set_num(vm, "LCA_N", 0);
    }
    if (cda) var_set_str(vm, "CLASS", cda->name);
    if (cdb) var_set_str(vm, "OTHER", cdb->name);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s %s -> dist=%d path_n=%d lca=%s\n",
              want_path ? "PATHBETWEEN" : "CHAINDIST", a, b, dist, pn,
              (hit && lca_i >= 0) ? vm->classes[lca_i].name : "-");
    bump(vm);
    return 1;
  }


  /* SIBLINGS|COSUBS|PEERCLASSES|SAMEPARENT|LISTSIBLINGS Class|obj
   * multi-file EXTEND/link usability: newline bag of classes that share the
   * same parent_idx (excluding self). Soft empty OK=1 when root/orphan/unknown.
   * HASCHILD|HASDIRECTSUB|HASSUBCLASS|DIRECTCHILD Class|obj Child
   * soft 0|1 if Child parent_idx is Class (direct EXTEND child).
   * HASDESCENDANT|HASSUBTREE|INPROGENY|HASPROGENY Class|obj Desc
   * soft 0|1 if Desc walks parent_idx into Class (strict descendant, not self).
   * CHILDREN|LISTCHILDREN|SUBCLASSES|DIRECTSUBS Class|obj - bag of direct kids.
   * Complements HASANCESTOR/DEPTHOF + COMMONANCESTOR + CHAINDIST. Cube is SoT. */
  if (kw(&L->cur, "SIBLINGS") || kw(&L->cur, "COSUBS") ||
      kw(&L->cur, "PEERCLASSES") || kw(&L->cur, "SAMEPARENT") ||
      kw(&L->cur, "LISTSIBLINGS") || kw(&L->cur, "SIBLINGCLASSES") ||
      kw(&L->cur, "PEER_CLASSES") || kw(&L->cur, "SAME_PARENT") ||
      kw(&L->cur, "COCLASSES") || kw(&L->cur, "SHAREPARENT") ||
      kw(&L->cur, "HASCHILD") || kw(&L->cur, "HASDIRECTSUB") ||
      kw(&L->cur, "HASSUBCLASS") || kw(&L->cur, "DIRECTCHILD") ||
      kw(&L->cur, "HAS_CHILD") || kw(&L->cur, "HAS_SUBCLASS") ||
      kw(&L->cur, "HASDIRECTCHILD") || kw(&L->cur, "ISPARENTOF") ||
      kw(&L->cur, "HASDESCENDANT") || kw(&L->cur, "HASSUBTREE") ||
      kw(&L->cur, "INPROGENY") || kw(&L->cur, "HASPROGENY") ||
      kw(&L->cur, "HAS_DESCENDANT") || kw(&L->cur, "HAS_SUBTREE") ||
      kw(&L->cur, "IN_PROGENY") || kw(&L->cur, "DESCENDSOF") ||
      kw(&L->cur, "CHILDREN") || kw(&L->cur, "LISTCHILDREN") ||
      kw(&L->cur, "SUBCLASSES") || kw(&L->cur, "DIRECTSUBS") ||
      kw(&L->cur, "LISTSUBS") || kw(&L->cur, "CHILDOF") ||
      kw(&L->cur, "LIST_CHILDREN") || kw(&L->cur, "DIRECT_CHILDREN")) {
    int want_sib = kw(&L->cur, "SIBLINGS") || kw(&L->cur, "COSUBS") ||
                   kw(&L->cur, "PEERCLASSES") || kw(&L->cur, "SAMEPARENT") ||
                   kw(&L->cur, "LISTSIBLINGS") || kw(&L->cur, "SIBLINGCLASSES") ||
                   kw(&L->cur, "PEER_CLASSES") || kw(&L->cur, "SAME_PARENT") ||
                   kw(&L->cur, "COCLASSES") || kw(&L->cur, "SHAREPARENT");
    int want_child = kw(&L->cur, "HASCHILD") || kw(&L->cur, "HASDIRECTSUB") ||
                     kw(&L->cur, "HASSUBCLASS") || kw(&L->cur, "DIRECTCHILD") ||
                     kw(&L->cur, "HAS_CHILD") || kw(&L->cur, "HAS_SUBCLASS") ||
                     kw(&L->cur, "HASDIRECTCHILD") || kw(&L->cur, "ISPARENTOF");
    int want_desc = kw(&L->cur, "HASDESCENDANT") || kw(&L->cur, "HASSUBTREE") ||
                    kw(&L->cur, "INPROGENY") || kw(&L->cur, "HASPROGENY") ||
                    kw(&L->cur, "HAS_DESCENDANT") || kw(&L->cur, "HAS_SUBTREE") ||
                    kw(&L->cur, "IN_PROGENY") || kw(&L->cur, "DESCENDSOF");
    int want_kids = kw(&L->cur, "CHILDREN") || kw(&L->cur, "LISTCHILDREN") ||
                    kw(&L->cur, "SUBCLASSES") || kw(&L->cur, "DIRECTSUBS") ||
                    kw(&L->cur, "LISTSUBS") || kw(&L->cur, "CHILDOF") ||
                    kw(&L->cur, "LIST_CHILDREN") || kw(&L->cur, "DIRECT_CHILDREN");
    char a[48], b[48];
    ClassDef *cda = NULL, *cdb = NULL;
    ObjInst *ob;
    int ai = -1, bi = -1;
    int hit = 0, n = 0, i, guard, pi;
    char bag[2048];
    size_t o = 0;
    const char *opname = want_sib ? "SIBLINGS"
                         : want_child ? "HASCHILD"
                         : want_desc ? "HASDESCENDANT"
                         : "CHILDREN";
    (void)want_kids;
    lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, want_child || want_desc
                   ? (want_child ? "HASCHILD Class|obj Child"
                                 : "HASDESCENDANT Class|obj Desc")
                   : (want_sib ? "SIBLINGS Class|obj" : "CHILDREN Class|obj"));
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
    b[0] = 0;
    if (want_child || want_desc) {
      if (kw(&L->cur, "AND") || kw(&L->cur, "WITH") || kw(&L->cur, "OF") ||
          kw(&L->cur, "IS") || kw(&L->cur, "TO") || kw(&L->cur, "HAS"))
        lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, want_child ? "HASCHILD Class|obj Child"
                            : "HASDESCENDANT Class|obj Desc");
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
    }
    bag[0] = 0;
    ob = oop_find_obj(vm, a);
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes) {
      cda = &vm->classes[ob->class_idx];
      ai = ob->class_idx;
    } else {
      cda = oop_find_class(vm, a);
      if (cda) ai = (int)(cda - vm->classes);
    }
    if (want_child || want_desc) {
      ob = oop_find_obj(vm, b);
      if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes) {
        cdb = &vm->classes[ob->class_idx];
        bi = ob->class_idx;
      } else {
        cdb = oop_find_class(vm, b);
        if (cdb) bi = (int)(cdb - vm->classes);
      }
    }
    if (want_child) {
      hit = 0;
      if (cda && cdb && ai >= 0 && bi >= 0 && ai != bi) {
        if (cdb->parent_idx == ai) hit = 1;
      }
      var_set_num(vm, "LAST_N", hit ? 1 : 0);
      vm->last_n = hit ? 1 : 0;
      var_set_num(vm, "HASCHILD_N", hit ? 1 : 0);
      var_set_num(vm, "CHILD_N", hit ? 1 : 0);
      var_set_num(vm, "DIRECT_N", hit ? 1 : 0);
      snprintf(vm->last_str, sizeof vm->last_str, "%d", hit ? 1 : 0);
      var_set_str(vm, "LAST", vm->last_str);
    } else if (want_desc) {
      hit = 0;
      if (cda && cdb && ai >= 0 && bi >= 0 && ai != bi) {
        pi = cdb->parent_idx;
        guard = 0;
        while (pi >= 0 && pi < vm->n_classes && guard++ < CUBALC_MAX_CLASSES) {
          if (pi == ai) { hit = 1; break; }
          pi = vm->classes[pi].parent_idx;
        }
      }
      var_set_num(vm, "LAST_N", hit ? 1 : 0);
      vm->last_n = hit ? 1 : 0;
      var_set_num(vm, "HASDESCENDANT_N", hit ? 1 : 0);
      var_set_num(vm, "DESCENDANT_N", hit ? 1 : 0);
      var_set_num(vm, "SUBTREE_N", hit ? 1 : 0);
      snprintf(vm->last_str, sizeof vm->last_str, "%d", hit ? 1 : 0);
      var_set_str(vm, "LAST", vm->last_str);
    } else if (want_sib) {
      o = 0; n = 0; bag[0] = 0;
      if (cda && ai >= 0) {
        int par = cda->parent_idx;
        if (par >= 0 && par < vm->n_classes) {
          for (i = 0; i < vm->n_classes; i++) {
            if (i == ai) continue;
            if (vm->classes[i].parent_idx == par) {
              const char *nm = vm->classes[i].name;
              size_t ln = strlen(nm);
              if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
              if (o + ln < sizeof bag) {
                memcpy(bag + o, nm, ln);
                o += ln;
              }
              n++;
            }
          }
          if (o < sizeof bag) bag[o] = 0;
        }
      }
      var_set_str(vm, "LAST", bag);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
      var_set_num(vm, "LAST_N", n);
      vm->last_n = n;
      var_set_str(vm, "SIBLINGS", bag);
      var_set_str(vm, "PEERS", bag);
      var_set_str(vm, "COSUBS", bag);
      var_set_num(vm, "SIBLINGS_N", n);
      var_set_num(vm, "PEERS_N", n);
    } else {
      o = 0; n = 0; bag[0] = 0;
      if (cda && ai >= 0) {
        for (i = 0; i < vm->n_classes; i++) {
          if (vm->classes[i].parent_idx == ai) {
            const char *nm = vm->classes[i].name;
            size_t ln = strlen(nm);
            if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
            if (o + ln < sizeof bag) {
              memcpy(bag + o, nm, ln);
              o += ln;
            }
            n++;
          }
        }
        if (o < sizeof bag) bag[o] = 0;
      }
      var_set_str(vm, "LAST", bag);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
      var_set_num(vm, "LAST_N", n);
      vm->last_n = n;
      var_set_str(vm, "CHILDREN", bag);
      var_set_str(vm, "SUBCLASSES", bag);
      var_set_str(vm, "KIDS", bag);
      var_set_num(vm, "CHILDREN_N", n);
      var_set_num(vm, "CHILDCOUNT_N", n);
      var_set_num(vm, "KIDS_N", n);
    }
    if (cda) var_set_str(vm, "CLASS", cda->name);
    if (cdb) var_set_str(vm, "OTHER", cdb->name);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s %s -> n=%d hit=%d\n", opname, a,
              b[0] ? b : "-", n, hit);
    bump(vm);
    return 1;
  }
  /* DESCENDANTS bag + ISLEAF + HASCHILDREN + NTH/FIRST/LAST child (multifile EXTEND) */
  if (kw(&L->cur, "DESCENDANTS") || kw(&L->cur, "LISTDESCENDANTS") ||
      kw(&L->cur, "PROGENY") || kw(&L->cur, "ALLSUBS") ||
      kw(&L->cur, "SUBTREEBAG") || kw(&L->cur, "ALLDESCENDANTS") ||
      kw(&L->cur, "LIST_DESCENDANTS") || kw(&L->cur, "DESCENDANTBAG") ||
      kw(&L->cur, "ISLEAF") || kw(&L->cur, "NOCHILDREN") ||
      kw(&L->cur, "LEAFCLASS") || kw(&L->cur, "HASNOCHILD") ||
      kw(&L->cur, "IS_LEAF") || kw(&L->cur, "NO_CHILDREN") ||
      kw(&L->cur, "HASCHILDREN") || kw(&L->cur, "HASKIDS") ||
      kw(&L->cur, "HASDIRECTKIDS") || kw(&L->cur, "HAS_CHILDREN") ||
      kw(&L->cur, "HAS_KIDS") || kw(&L->cur, "HASDIRECTCHILDREN") ||
      kw(&L->cur, "NTHCHILD") || kw(&L->cur, "CHILDAT") ||
      kw(&L->cur, "SUBAT") || kw(&L->cur, "NTH_CHILD") ||
      kw(&L->cur, "CHILD_AT") || kw(&L->cur, "KIDAT") ||
      kw(&L->cur, "FIRSTCHILD") || kw(&L->cur, "ELDERCHILD") ||
      kw(&L->cur, "OLDESTCHILD") || kw(&L->cur, "FIRST_CHILD") ||
      kw(&L->cur, "LASTCHILD") || kw(&L->cur, "YOUNGCHILD") ||
      kw(&L->cur, "NEWESTCHILD") || kw(&L->cur, "LAST_CHILD") ||
      kw(&L->cur, "YOUNGESTCHILD")) {
    int want_descbag = kw(&L->cur, "DESCENDANTS") || kw(&L->cur, "LISTDESCENDANTS") ||
                       kw(&L->cur, "PROGENY") || kw(&L->cur, "ALLSUBS") ||
                       kw(&L->cur, "SUBTREEBAG") || kw(&L->cur, "ALLDESCENDANTS") ||
                       kw(&L->cur, "LIST_DESCENDANTS") || kw(&L->cur, "DESCENDANTBAG");
    int want_leaf = kw(&L->cur, "ISLEAF") || kw(&L->cur, "NOCHILDREN") ||
                    kw(&L->cur, "LEAFCLASS") || kw(&L->cur, "HASNOCHILD") ||
                    kw(&L->cur, "IS_LEAF") || kw(&L->cur, "NO_CHILDREN");
    int want_haskids = kw(&L->cur, "HASCHILDREN") || kw(&L->cur, "HASKIDS") ||
                       kw(&L->cur, "HASDIRECTKIDS") || kw(&L->cur, "HAS_CHILDREN") ||
                       kw(&L->cur, "HAS_KIDS") || kw(&L->cur, "HASDIRECTCHILDREN");
    int want_nth = kw(&L->cur, "NTHCHILD") || kw(&L->cur, "CHILDAT") ||
                   kw(&L->cur, "SUBAT") || kw(&L->cur, "NTH_CHILD") ||
                   kw(&L->cur, "CHILD_AT") || kw(&L->cur, "KIDAT");
    int want_first = kw(&L->cur, "FIRSTCHILD") || kw(&L->cur, "ELDERCHILD") ||
                     kw(&L->cur, "OLDESTCHILD") || kw(&L->cur, "FIRST_CHILD");
    int want_last = kw(&L->cur, "LASTCHILD") || kw(&L->cur, "YOUNGCHILD") ||
                    kw(&L->cur, "NEWESTCHILD") || kw(&L->cur, "LAST_CHILD") ||
                    kw(&L->cur, "YOUNGESTCHILD");
    char a[48];
    ClassDef *cda = NULL;
    ObjInst *ob;
    int ai = -1;
    int n = 0, i, idx_want = 0;
    int hit = 0;
    char bag[2048];
    size_t o = 0;
    const char *opname = want_descbag ? "DESCENDANTS"
                         : want_leaf ? "ISLEAF"
                         : want_haskids ? "HASCHILDREN"
                         : want_nth ? "NTHCHILD"
                         : want_first ? "FIRSTCHILD"
                         : "LASTCHILD";
    lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, want_nth ? "NTHCHILD Class|obj N"
                        : (want_descbag ? "DESCENDANTS Class|obj"
                                        : (want_leaf ? "ISLEAF Class|obj"
                                                     : (want_haskids ? "HASCHILDREN Class|obj"
                                                                     : (want_first ? "FIRSTCHILD Class|obj"
                                                                                   : "LASTCHILD Class|obj")))));
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
        else { fail(vm, "NTHCHILD Class|obj N"); return -1; }
        lex_next(L);
      } else { fail(vm, "NTHCHILD Class|obj N"); return -1; }
    }
    bag[0] = 0;
    ob = oop_find_obj(vm, a);
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes) {
      cda = &vm->classes[ob->class_idx];
      ai = ob->class_idx;
    } else {
      cda = oop_find_class(vm, a);
      if (cda) ai = (int)(cda - vm->classes);
    }
    {
      int kids[512];
      int nk = 0;
      if (cda && ai >= 0) {
        for (i = 0; i < vm->n_classes; i++) {
          if (vm->classes[i].parent_idx == ai) {
            if (nk < 512) kids[nk++] = i;
          }
        }
      }
      if (want_leaf || want_haskids) {
        hit = want_leaf ? (nk == 0 && cda != NULL) : (nk > 0);
        if (!cda) hit = 0;
        var_set_num(vm, "LAST_N", hit ? 1 : 0);
        vm->last_n = hit ? 1 : 0;
        var_set_num(vm, "ISLEAF_N", (nk == 0 && cda) ? 1 : 0);
        var_set_num(vm, "HASCHILDREN_N", nk > 0 ? 1 : 0);
        var_set_num(vm, "HASKIDS_N", nk > 0 ? 1 : 0);
        var_set_num(vm, "CHILDCOUNT_N", nk);
        var_set_num(vm, "CHILDREN_N", nk);
        snprintf(vm->last_str, sizeof vm->last_str, "%d", hit ? 1 : 0);
        var_set_str(vm, "LAST", vm->last_str);
        n = hit ? 1 : 0;
      } else if (want_first || want_last || want_nth) {
        int pick = -1;
        char namebuf[48];
        namebuf[0] = 0;
        if (want_first && nk > 0) pick = kids[0];
        else if (want_last && nk > 0) pick = kids[nk - 1];
        else if (want_nth && idx_want >= 0 && idx_want < nk) pick = kids[idx_want];
        if (pick >= 0) {
          snprintf(namebuf, sizeof namebuf, "%s", vm->classes[pick].name);
          hit = 1;
        }
        var_set_str(vm, "LAST", namebuf);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", namebuf);
        var_set_num(vm, "LAST_N", hit ? 1 : 0);
        vm->last_n = hit ? 1 : 0;
        var_set_str(vm, "CHILD", namebuf);
        var_set_str(vm, "NTHCHILD", namebuf);
        if (nk > 0) var_set_str(vm, "FIRSTCHILD", vm->classes[kids[0]].name);
        else var_set_str(vm, "FIRSTCHILD", "");
        if (nk > 0) var_set_str(vm, "LASTCHILD", vm->classes[kids[nk - 1]].name);
        else var_set_str(vm, "LASTCHILD", "");
        var_set_num(vm, "CHILDCOUNT_N", nk);
        var_set_num(vm, "NTHCHILD_N", hit ? 1 : 0);
        n = hit ? 1 : 0;
      } else {
        int queue[512];
        int qh = 0, qt = 0;
        int seen[512];
        o = 0; n = 0; bag[0] = 0;
        for (i = 0; i < 512; i++) seen[i] = 0;
        if (cda && ai >= 0) {
          for (i = 0; i < nk; i++) { queue[qt++] = kids[i]; seen[kids[i]] = 1; }
          while (qh < qt) {
            int cur = queue[qh++];
            const char *nm; size_t ln;
            if (cur < 0 || cur >= vm->n_classes) continue;
            nm = vm->classes[cur].name; ln = strlen(nm);
            if (n > 0 && o + 1 < sizeof bag) bag[o++] = "\n"[0];
            if (o + ln < sizeof bag) { memcpy(bag + o, nm, ln); o += ln; }
            n++;
            for (i = 0; i < vm->n_classes; i++) {
              if (vm->classes[i].parent_idx == cur && !seen[i]) {
                seen[i] = 1;
                if (qt < 512) queue[qt++] = i;
              }
            }
          }
          if (o < sizeof bag) bag[o] = 0;
        }
        var_set_str(vm, "LAST", bag);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
        var_set_num(vm, "LAST_N", n);
        vm->last_n = n;
        var_set_str(vm, "DESCENDANTS", bag);
        var_set_str(vm, "PROGENY", bag);
        var_set_str(vm, "SUBTREE", bag);
        var_set_num(vm, "DESCENDANTS_N", n);
        var_set_num(vm, "PROGENY_N", n);
        var_set_num(vm, "SUBTREE_N", n);
        var_set_num(vm, "CHILDCOUNT_N", nk);
      }
    }
    if (cda) var_set_str(vm, "CLASS", cda->name);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s -> n=%d hit=%d\n", opname, a, n, hit);
    bump(vm);
    return 1;
  }


  /* NTHPARENT|PARENTAT + HEIGHTOF + LEAFCOUNT (multifile EXTEND)
   * NTHPARENT Class|obj N — ancestor N steps up (0 = immediate parent). Soft empty OOB.
   * HEIGHTOF Class|obj — longest downward EXTEND path length (leaf height 0).
   * LEAFCOUNT Class|obj — number of leaf classes in subtree (self counts if leaf).
   * Dual of NTHCHILD/DEPTHOF/DESCENDANTS. Cube is SoT. Free energy must flow. */
  if (kw(&L->cur, "NTHPARENT") || kw(&L->cur, "PARENTAT") ||
      kw(&L->cur, "ANCESTORAT") || kw(&L->cur, "NTH_PARENT") ||
      kw(&L->cur, "PARENT_AT") || kw(&L->cur, "SUPERAT") ||
      kw(&L->cur, "NTHSUPER") || kw(&L->cur, "NTHANC") ||
      kw(&L->cur, "HEIGHTOF") || kw(&L->cur, "TREEHEIGHT") ||
      kw(&L->cur, "SUBTREEHEIGHT") || kw(&L->cur, "HEIGHT_OF") ||
      kw(&L->cur, "CLASSHEIGHT") || kw(&L->cur, "HEIGHTCLASS") ||
      kw(&L->cur, "GETHEIGHT") || kw(&L->cur, "TREE_HEIGHT") ||
      kw(&L->cur, "LEAFCOUNT") || kw(&L->cur, "COUNTLEAVES") ||
      kw(&L->cur, "NLEAVES") || kw(&L->cur, "LEAF_COUNT") ||
      kw(&L->cur, "NUMLEAVES") || kw(&L->cur, "COUNT_LEAVES") ||
      kw(&L->cur, "LEAVESN") || kw(&L->cur, "N_LEAVES")) {
    int want_nthp = kw(&L->cur, "NTHPARENT") || kw(&L->cur, "PARENTAT") ||
                    kw(&L->cur, "ANCESTORAT") || kw(&L->cur, "NTH_PARENT") ||
                    kw(&L->cur, "PARENT_AT") || kw(&L->cur, "SUPERAT") ||
                    kw(&L->cur, "NTHSUPER") || kw(&L->cur, "NTHANC");
    int want_height = kw(&L->cur, "HEIGHTOF") || kw(&L->cur, "TREEHEIGHT") ||
                      kw(&L->cur, "SUBTREEHEIGHT") || kw(&L->cur, "HEIGHT_OF") ||
                      kw(&L->cur, "CLASSHEIGHT") || kw(&L->cur, "HEIGHTCLASS") ||
                      kw(&L->cur, "GETHEIGHT") || kw(&L->cur, "TREE_HEIGHT");
    int want_leafc = kw(&L->cur, "LEAFCOUNT") || kw(&L->cur, "COUNTLEAVES") ||
                     kw(&L->cur, "NLEAVES") || kw(&L->cur, "LEAF_COUNT") ||
                     kw(&L->cur, "NUMLEAVES") || kw(&L->cur, "COUNT_LEAVES") ||
                     kw(&L->cur, "LEAVESN") || kw(&L->cur, "N_LEAVES");
    char a[48];
    ClassDef *cda = NULL;
    ObjInst *ob;
    int ai = -1, i, hit = 0, n = 0, idx_want = -1, guard = 0;
    const char *opname = want_nthp ? "NTHPARENT"
                         : want_height ? "HEIGHTOF"
                                       : "LEAFCOUNT";
    lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, want_nthp ? "NTHPARENT Class|obj N"
                         : (want_height ? "HEIGHTOF Class|obj"
                                        : "LEAFCOUNT Class|obj"));
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
    if (want_nthp) {
      if (kw(&L->cur, "AT") || kw(&L->cur, "INDEX") || kw(&L->cur, "N") ||
          kw(&L->cur, "OF") || kw(&L->cur, "POS") || kw(&L->cur, "UP"))
        lex_next(L);
      if (L->cur.kind == TK_NUM) {
        idx_want = (int)L->cur.num;
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && !vv->is_str) idx_want = (int)vv->val;
        else { fail(vm, "NTHPARENT Class|obj N"); return -1; }
        lex_next(L);
      } else { fail(vm, "NTHPARENT Class|obj N"); return -1; }
    }
    ob = oop_find_obj(vm, a);
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes) {
      cda = &vm->classes[ob->class_idx];
      ai = ob->class_idx;
    } else {
      cda = oop_find_class(vm, a);
      if (cda) ai = (int)(cda - vm->classes);
    }
    if (want_nthp) {
      char namebuf[48];
      int pi;
      namebuf[0] = 0;
      hit = 0;
      if (cda && ai >= 0 && idx_want >= 0) {
        pi = cda->parent_idx;
        guard = 0;
        while (pi >= 0 && pi < vm->n_classes && guard++ < CUBALC_MAX_CLASSES) {
          if (idx_want == 0) {
            snprintf(namebuf, sizeof namebuf, "%s", vm->classes[pi].name);
            hit = 1;
            break;
          }
          idx_want--;
          pi = vm->classes[pi].parent_idx;
        }
      }
      var_set_str(vm, "LAST", namebuf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", namebuf);
      var_set_num(vm, "LAST_N", hit ? 1 : 0);
      vm->last_n = hit ? 1 : 0;
      var_set_str(vm, "NTHPARENT", namebuf);
      var_set_str(vm, "PARENTAT", namebuf);
      var_set_str(vm, "ANCESTOR", namebuf);
      var_set_num(vm, "NTHPARENT_N", hit ? 1 : 0);
      var_set_num(vm, "PARENTAT_N", hit ? 1 : 0);
      n = hit ? 1 : 0;
    } else if (want_height) {
      int best = 0;
      if (cda && ai >= 0) {
        int stack_i[512];
        int stack_d[512];
        int sp = 0;
        stack_i[sp] = ai;
        stack_d[sp] = 0;
        sp++;
        guard = 0;
        while (sp > 0 && guard++ < CUBALC_MAX_CLASSES * 4) {
          int cur, depth;
          sp--;
          cur = stack_i[sp];
          depth = stack_d[sp];
          if (depth > best) best = depth;
          for (i = 0; i < vm->n_classes; i++) {
            if (vm->classes[i].parent_idx == cur) {
              if (sp < 512) {
                stack_i[sp] = i;
                stack_d[sp] = depth + 1;
                sp++;
              }
            }
          }
        }
      }
      n = (cda && ai >= 0) ? best : 0;
      var_set_num(vm, "LAST_N", n);
      vm->last_n = n;
      var_set_num(vm, "HEIGHT", n);
      var_set_num(vm, "HEIGHTOF_N", n);
      var_set_num(vm, "TREEHEIGHT_N", n);
      snprintf(vm->last_str, sizeof vm->last_str, "%d", n);
      var_set_str(vm, "LAST", vm->last_str);
    } else {
      int queue[512];
      int qh = 0, qt = 0;
      int seen[512];
      int leaves = 0;
      for (i = 0; i < 512; i++) seen[i] = 0;
      if (cda && ai >= 0) {
        queue[qt++] = ai;
        seen[ai] = 1;
        while (qh < qt) {
          int cur = queue[qh++];
          int nk = 0;
          if (cur < 0 || cur >= vm->n_classes) continue;
          for (i = 0; i < vm->n_classes; i++) {
            if (vm->classes[i].parent_idx == cur) {
              nk++;
              if (!seen[i] && qt < 512) {
                seen[i] = 1;
                queue[qt++] = i;
              }
            }
          }
          if (nk == 0) leaves++;
        }
      }
      n = leaves;
      var_set_num(vm, "LAST_N", n);
      vm->last_n = n;
      var_set_num(vm, "LEAFCOUNT_N", n);
      var_set_num(vm, "LEAVES_N", n);
      var_set_num(vm, "NLEAVES_N", n);
      snprintf(vm->last_str, sizeof vm->last_str, "%d", n);
      var_set_str(vm, "LAST", vm->last_str);
    }
    if (cda) var_set_str(vm, "CLASS", cda->name);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s -> n=%d hit=%d\n", opname, a, n, hit);
    bump(vm);
    return 1;
  }

  /* LISTLEAVES|LEAVESBAG + FIRSTLEAF/LASTLEAF/NTHLEAF (multifile EXTEND)
   * LISTLEAVES Class|obj — newline bag of leaf class names in subtree (self if leaf).
   * FIRSTLEAF / LASTLEAF — first/last leaf name in declaration order.
   * NTHLEAF Class|obj N — leaf at index N (0-based); soft empty OOB.
   * Complements LEAFCOUNT + ISLEAF + DESCENDANTS. Cube is SoT. Free energy must flow. */
  if (kw(&L->cur, "LISTLEAVES") || kw(&L->cur, "LEAVESBAG") ||
      kw(&L->cur, "LEAFNAMES") || kw(&L->cur, "LIST_LEAVES") ||
      kw(&L->cur, "LEAVES_BAG") || kw(&L->cur, "GETLEAVES") ||
      kw(&L->cur, "SUBTREELEAVES") || kw(&L->cur, "LEAFSBAG") ||
      kw(&L->cur, "FIRSTLEAF") || kw(&L->cur, "ELDERLEAF") ||
      kw(&L->cur, "OLDESTLEAF") || kw(&L->cur, "FIRST_LEAF") ||
      kw(&L->cur, "LASTLEAF") || kw(&L->cur, "YOUNGLEAF") ||
      kw(&L->cur, "NEWESTLEAF") || kw(&L->cur, "LAST_LEAF") ||
      kw(&L->cur, "YOUNGESTLEAF") ||
      kw(&L->cur, "NTHLEAF") || kw(&L->cur, "LEAFAT") ||
      kw(&L->cur, "NTH_LEAF") || kw(&L->cur, "LEAF_AT") ||
      kw(&L->cur, "KIDLEAFAT") || kw(&L->cur, "LEAFINDEX")) {
    int want_bag = kw(&L->cur, "LISTLEAVES") || kw(&L->cur, "LEAVESBAG") ||
                   kw(&L->cur, "LEAFNAMES") || kw(&L->cur, "LIST_LEAVES") ||
                   kw(&L->cur, "LEAVES_BAG") || kw(&L->cur, "GETLEAVES") ||
                   kw(&L->cur, "SUBTREELEAVES") || kw(&L->cur, "LEAFSBAG");
    int want_first = kw(&L->cur, "FIRSTLEAF") || kw(&L->cur, "ELDERLEAF") ||
                     kw(&L->cur, "OLDESTLEAF") || kw(&L->cur, "FIRST_LEAF");
    int want_last = kw(&L->cur, "LASTLEAF") || kw(&L->cur, "YOUNGLEAF") ||
                    kw(&L->cur, "NEWESTLEAF") || kw(&L->cur, "LAST_LEAF") ||
                    kw(&L->cur, "YOUNGESTLEAF");
    int want_nth = kw(&L->cur, "NTHLEAF") || kw(&L->cur, "LEAFAT") ||
                   kw(&L->cur, "NTH_LEAF") || kw(&L->cur, "LEAF_AT") ||
                   kw(&L->cur, "KIDLEAFAT") || kw(&L->cur, "LEAFINDEX");
    char a[48], bag[4096], pick[48];
    ClassDef *cda = NULL;
    ObjInst *ob;
    int ai = -1, i, n = 0, idx_want = -1, guard = 0;
    int leaf_idx[512];
    int nleaf = 0;
    const char *opname = want_nth ? "NTHLEAF"
                         : want_first ? "FIRSTLEAF"
                         : want_last ? "LASTLEAF"
                                       : "LISTLEAVES";
    lex_next(L);
    a[0] = bag[0] = pick[0] = 0;
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, want_nth ? "NTHLEAF Class|obj N"
                         : (want_bag ? "LISTLEAVES Class|obj"
                                     : "FIRSTLEAF|LASTLEAF Class|obj"));
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
        else { fail(vm, "NTHLEAF Class|obj N"); return -1; }
        lex_next(L);
      } else { fail(vm, "NTHLEAF Class|obj N"); return -1; }
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
      int queue[512];
      int qh = 0, qt = 0;
      int seen[512];
      for (i = 0; i < 512; i++) seen[i] = 0;
      queue[qt++] = ai;
      seen[ai] = 1;
      guard = 0;
      while (qh < qt && guard++ < CUBALC_MAX_CLASSES * 4) {
        int cur = queue[qh++];
        int nk = 0;
        int kids[256];
        int nkids = 0;
        if (cur < 0 || cur >= vm->n_classes) continue;
        for (i = 0; i < vm->n_classes; i++) {
          if (vm->classes[i].parent_idx == cur) {
            if (nkids < 256) kids[nkids++] = i;
            nk++;
            if (!seen[i] && qt < 512) {
              seen[i] = 1;
              queue[qt++] = i;
            }
          }
        }
        (void)kids;
        if (nk == 0 && nleaf < 512) {
          leaf_idx[nleaf++] = cur;
        }
      }
    }
    n = nleaf;
    if (want_bag) {
      size_t o = 0;
      bag[0] = 0;
      for (i = 0; i < nleaf; i++) {
        const char *nm = vm->classes[leaf_idx[i]].name;
        size_t ln = strlen(nm);
        if (o && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + ln + 1 < sizeof bag) {
          memcpy(bag + o, nm, ln);
          o += ln;
          bag[o] = 0;
        }
      }
      var_set_str(vm, "LAST", bag);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
      var_set_str(vm, "LISTLEAVES", bag);
      var_set_str(vm, "LEAVESBAG", bag);
      var_set_str(vm, "LEAFNAMES", bag);
      var_set_num(vm, "LAST_N", n);
      vm->last_n = n;
      var_set_num(vm, "LISTLEAVES_N", n);
      var_set_num(vm, "LEAVESBAG_N", n);
      var_set_num(vm, "LEAFNAMES_N", n);
      var_set_num(vm, "LEAFCOUNT_N", n);
    } else {
      int hit = 0;
      pick[0] = 0;
      if (want_first && nleaf > 0) {
        snprintf(pick, sizeof pick, "%s", vm->classes[leaf_idx[0]].name);
        hit = 1;
      } else if (want_last && nleaf > 0) {
        snprintf(pick, sizeof pick, "%s", vm->classes[leaf_idx[nleaf - 1]].name);
        hit = 1;
      } else if (want_nth && idx_want >= 0 && idx_want < nleaf) {
        snprintf(pick, sizeof pick, "%s", vm->classes[leaf_idx[idx_want]].name);
        hit = 1;
      }
      var_set_str(vm, "LAST", pick);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", pick);
      var_set_str(vm, "FIRSTLEAF", pick);
      var_set_str(vm, "LASTLEAF", pick);
      var_set_str(vm, "NTHLEAF", pick);
      var_set_str(vm, "LEAFAT", pick);
      var_set_num(vm, "LAST_N", hit ? 1 : 0);
      vm->last_n = hit ? 1 : 0;
      var_set_num(vm, "FIRSTLEAF_N", hit ? 1 : 0);
      var_set_num(vm, "LASTLEAF_N", hit ? 1 : 0);
      var_set_num(vm, "NTHLEAF_N", hit ? 1 : 0);
      n = hit ? 1 : 0;
    }
    if (cda) var_set_str(vm, "CLASS", cda->name);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s -> n=%d bag_or_hit=%d\n", opname, a, nleaf, n);
    bump(vm);
    return 1;
  }

  /* CHILDCOUNT + DESCENDANTCOUNT + SUBTREESIZE (multifile EXTEND)
   * CHILDCOUNT Class|obj — direct children count (EXTEND kids only).
   * DESCENDANTCOUNT Class|obj — proper descendants count (excludes self).
   * SUBTREESIZE Class|obj — 1 + descendants (self included).
   * Complements HASCHILDREN + DESCENDANTS bag + LEAFCOUNT. Cube is SoT. Free energy must flow. */
  return 0;
}
