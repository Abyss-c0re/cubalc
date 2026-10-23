/* flow fstr peel from flow_class */
#include "lang/cubalc_lang_ops_flow_fstr_b.h"
#include "lang/cubalc_lang_ops_flow_fstr_hasfield.h"
#include "lang/cubalc_lang_ops_flow_fstr_hasmeth.h"
#include "lang/cubalc_lang_ops_flow_fstr_kinship.h"
#include "lang/cubalc_lang_ops_flow_fstr_cousin.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_fstr_b(VM *vm, Lex *L) {
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

  /* HASCLASS|KNOWNCLASS|CLASS? name â soft 0|1 probe if CLASS is defined.
   * Complements REQUIRE CLASS (fail-fast) and LISTCLASSES+HASLINE glue.
   * String-var class names expand. LAST = name when hit, else "".
   * Usability: IF before ENSURENEW/TRYNEW without bag materialize. */
  if (kw(&L->cur, "HASCLASS") || kw(&L->cur, "KNOWNCLASS") ||
      kw(&L->cur, "CLASS?") || kw(&L->cur, "HAS_CLASS") ||
      kw(&L->cur, "ISDEFINEDCLASS") || kw(&L->cur, "CLASSDEF?") ||
      kw(&L->cur, "TYPEDEFINED") || kw(&L->cur, "HASTYPE")) {
    char cname[48];
    int hit = 0;
    lex_next(L);
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "HASCLASS name"); return -1;
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
    hit = oop_find_class(vm, cname) ? 1 : 0;
    if (hit) {
      var_set_str(vm, "LAST", cname);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", cname);
      var_set_str(vm, "CLASS", cname);
    } else {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
    }
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    var_set_num(vm, "HASCLASS_N", hit);
    var_set_num(vm, "OK", 1);
    if (vm->trace) fprintf(vm->trace, "# HASCLASS %s -> %d\n", cname, hit);
    bump(vm);
    return 1;
  }

  /* HASCLASSES|NEEDCLASSES nameâ¦ â multi CLASS capability gate (all-or-CLASSMISS).
   * Soft: LAST_N 0|1 Â· CLASSMISS bag Â· NEEDCLASSES fail-fast first miss.
   * Usability: agent boot contracts without NÃ HASCLASS glue Â· bag/CSV/idents.
   * Twin of HASFORMS for OOP plane Â· dual REQUIRE CLASS multi. */
  if (kw(&L->cur, "HASCLASSES") || kw(&L->cur, "HAS_CLASSES") ||
      kw(&L->cur, "ALLCLASSES") || kw(&L->cur, "CLASSESOK") ||
      kw(&L->cur, "NEEDCLASSES") || kw(&L->cur, "NEED_CLASSES") ||
      kw(&L->cur, "REQUIRECLASSES") || kw(&L->cur, "MUSTCLASSES")) {
    int hard = kw(&L->cur, "NEEDCLASSES") || kw(&L->cur, "NEED_CLASSES") ||
               kw(&L->cur, "REQUIRECLASSES") || kw(&L->cur, "MUSTCLASSES");
    char names[32][48];
    char miss[1024];
    int nname = 0, nmiss = 0, i, aln = L->cur.line;
    size_t mo = 0;
    lex_next(L);
    miss[0] = 0;
    while (nname < 32 &&
           (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT)) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur, "ASSERT") || kw(&L->cur, "LET") || kw(&L->cur, "PRINT") ||
           kw(&L->cur, "REQUIRE") || kw(&L->cur, "END") ||
           kw(&L->cur, "IF") || kw(&L->cur, "ELSE") || kw(&L->cur, "ELIF") ||
           kw(&L->cur, "PASS") || kw(&L->cur, "FAIL") || kw(&L->cur, "INCLUDE") ||
           kw(&L->cur, "SYS") || kw(&L->cur, "HELP") || kw(&L->cur, "CLEAR_ERR") ||
           kw(&L->cur, "NOTE") || kw(&L->cur, "EXIT") || kw(&L->cur, "DEFAULT") ||
           kw(&L->cur, "VERSION") || kw(&L->cur, "STATUS") || kw(&L->cur, "FOR") ||
           kw(&L->cur, "WHILE") || kw(&L->cur, "LOOP") || kw(&L->cur, "EACH") ||
           kw(&L->cur, "CUBE") || kw(&L->cur, "PLUG") || kw(&L->cur, "HOLD_FLASH") ||
           kw(&L->cur, "CLASS") || kw(&L->cur, "NEW") || kw(&L->cur, "HASCLASS") ||
           kw(&L->cur, "LISTCLASSES") || kw(&L->cur, "HASFORM") ||
           oop_stmt_kw(L)))
        break;
      if (L->cur.kind == TK_STR) {
        const char *s = L->cur.text;
        if (strchr(s, '\n') || strchr(s, ',') || strchr(s, ' ')) {
          const char *p = s;
          while (*p && nname < 32) {
            char tok[48];
            size_t tl = 0;
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' || *p == ':')
              p++;
            if (!*p) break;
            while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                   p[tl] != '\t' && p[tl] != ':' && tl + 1 < sizeof tok) {
              tok[tl] = p[tl];
              tl++;
            }
            tok[tl] = 0;
            p += tl;
            if (tok[0])
              snprintf(names[nname++], sizeof names[0], "%s", tok);
          }
          lex_next(L);
          continue;
        }
        snprintf(names[nname++], sizeof names[0], "%s", s);
        lex_next(L);
      } else {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (strchr(vv->sval, '\n') || strchr(vv->sval, ',') ||
             strchr(vv->sval, ' '))) {
          const char *p = vv->sval;
          while (*p && nname < 32) {
            char tok[48];
            size_t tl = 0;
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' || *p == ':')
              p++;
            if (!*p) break;
            while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                   p[tl] != '\t' && p[tl] != ':' && tl + 1 < sizeof tok) {
              tok[tl] = p[tl];
              tl++;
            }
            tok[tl] = 0;
            p += tl;
            if (tok[0])
              snprintf(names[nname++], sizeof names[0], "%s", tok);
          }
          lex_next(L);
        } else if (vv && vv->is_str && vv->sval[0] && !oop_find_class(vm, L->cur.text)) {
          snprintf(names[nname++], sizeof names[0], "%s", vv->sval);
          lex_next(L);
        } else if (strcmp(L->cur.text, "LAST") == 0) {
          snprintf(names[nname++], sizeof names[0], "%s", vm->last_str);
          lex_next(L);
        } else {
          snprintf(names[nname++], sizeof names[0], "%s", L->cur.text);
          lex_next(L);
        }
      }
    }
    if (nname == 0) {
      if (hard) {
        fail_at(vm, L, "NEEDCLASSES nameâ¦ â NEEDCLASSES Ticket Cell Â· HASCLASSES bag");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASCLASSES_N", 0);
      var_set_num(vm, "CLASSMISS_N", 0);
      var_set_str(vm, "CLASSMISS", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASCLASSES: need names â HASCLASSES Ticket Cell");
      var_set_str(vm, "ERR", "HASCLASSES: need names â HASCLASSES Ticket Cell");
      bump(vm);
      return 1;
    }
    for (i = 0; i < nname; i++) {
      if (oop_find_class(vm, names[i]))
        continue;
      nmiss++;
      if (mo && mo + 1 < sizeof miss) miss[mo++] = '\n';
      {
        size_t ln = strlen(names[i]);
        if (mo + ln < sizeof miss) {
          memcpy(miss + mo, names[i], ln);
          mo += ln;
          miss[mo] = 0;
        }
      }
      if (hard) {
        char em[192];
        snprintf(em, sizeof em,
                 "NEEDCLASSES miss line %d: '%s' (+%d more?) â CLASS/INCLUDE Â· HASCLASS Â· LISTCLASSES",
                 aln, names[i], nname - i - 1);
        fail_at(vm, L, em);
        return -1;
      }
    }
    var_set_str(vm, "CLASSMISS", miss);
    var_set_num(vm, "CLASSMISS_N", nmiss);
    var_set_num(vm, "HASCLASSES_N", nname - nmiss);
    var_set_num(vm, "NEEDCLASSES_N", nname);
    var_set_num(vm, "CLASSES_N", nname);
    if (nmiss == 0) {
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST", "1");
      snprintf(vm->last_str, sizeof vm->last_str, "1");
      var_set_num(vm, "OK", 1);
    } else {
      char em[200];
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      snprintf(em, sizeof em,
               "HASCLASSES miss (%d/%d): %s â CLASS/INCLUDE Â· NEEDCLASSES Â· LISTCLASSES",
               nmiss, nname, miss);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%d miss=%d\n",
              hard ? "NEEDCLASSES" : "HASCLASSES", nname, nmiss);
    bump(vm);
    return 1;
  }

  /* HASCLASSANY|NEEDCLASSANY nameâ¦ â multi CLASS any-of gate (HASCLASSES twin).
   * Soft: LAST_N 1 if any CLASS defined Â· CLASSHAVE bag Â· CLASSMISS bag.
   * NEEDCLASSANY fail-fast if none of the listed classes exist.
   * Usability: optional plugin class OR without NÃ HASCLASS IF glue. */
  if (kw(&L->cur, "HASCLASSANY") || kw(&L->cur, "HAS_CLASS_ANY") ||
      kw(&L->cur, "ANYCLASS") || kw(&L->cur, "CLASSANY") ||
      kw(&L->cur, "HASANYCLASS") || kw(&L->cur, "NEEDCLASSANY") ||
      kw(&L->cur, "NEED_CLASS_ANY") || kw(&L->cur, "REQUIRECLASSANY") ||
      kw(&L->cur, "MUSTCLASSANY")) {
    int hard = kw(&L->cur, "NEEDCLASSANY") || kw(&L->cur, "NEED_CLASS_ANY") ||
               kw(&L->cur, "REQUIRECLASSANY") || kw(&L->cur, "MUSTCLASSANY");
    char names[32][48];
    char miss[1024], havebag[1024];
    int nname = 0, nmiss = 0, nhave = 0, i, aln = L->cur.line;
    size_t mo = 0, ho = 0;
    lex_next(L);
    miss[0] = 0;
    havebag[0] = 0;
    while (nname < 32 &&
           (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT)) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur, "ASSERT") || kw(&L->cur, "LET") || kw(&L->cur, "PRINT") ||
           kw(&L->cur, "REQUIRE") || kw(&L->cur, "END") ||
           kw(&L->cur, "IF") || kw(&L->cur, "ELSE") || kw(&L->cur, "ELIF") ||
           kw(&L->cur, "PASS") || kw(&L->cur, "FAIL") || kw(&L->cur, "INCLUDE") ||
           kw(&L->cur, "SYS") || kw(&L->cur, "HELP") || kw(&L->cur, "CLEAR_ERR") ||
           kw(&L->cur, "NOTE") || kw(&L->cur, "EXIT") || kw(&L->cur, "DEFAULT") ||
           kw(&L->cur, "VERSION") || kw(&L->cur, "STATUS") || kw(&L->cur, "FOR") ||
           kw(&L->cur, "WHILE") || kw(&L->cur, "LOOP") || kw(&L->cur, "EACH") ||
           kw(&L->cur, "CUBE") || kw(&L->cur, "PLUG") || kw(&L->cur, "HOLD_FLASH") ||
           kw(&L->cur, "CLASS") || kw(&L->cur, "NEW") || kw(&L->cur, "HASCLASS") ||
           kw(&L->cur, "HASCLASSES") || kw(&L->cur, "LISTCLASSES") ||
           kw(&L->cur, "HASFORM") || oop_stmt_kw(L)))
        break;
      if (L->cur.kind == TK_STR) {
        const char *s = L->cur.text;
        if (strchr(s, '\n') || strchr(s, ',') || strchr(s, ' ')) {
          const char *pp = s;
          while (*pp && nname < 32) {
            char tok[48];
            size_t tl = 0;
            while (*pp == ' ' || *pp == '\t' || *pp == '\n' || *pp == ',' || *pp == ':')
              pp++;
            if (!*pp) break;
            while (pp[tl] && pp[tl] != '\n' && pp[tl] != ',' && pp[tl] != ' ' &&
                   pp[tl] != '\t' && pp[tl] != ':' && tl + 1 < sizeof tok) {
              tok[tl] = pp[tl];
              tl++;
            }
            tok[tl] = 0;
            pp += tl;
            if (tok[0])
              snprintf(names[nname++], sizeof names[0], "%s", tok);
          }
          lex_next(L);
          continue;
        }
        snprintf(names[nname++], sizeof names[0], "%s", s);
        lex_next(L);
      } else {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (strchr(vv->sval, '\n') || strchr(vv->sval, ',') ||
             strchr(vv->sval, ' '))) {
          const char *pp = vv->sval;
          while (*pp && nname < 32) {
            char tok[48];
            size_t tl = 0;
            while (*pp == ' ' || *pp == '\t' || *pp == '\n' || *pp == ',' || *pp == ':')
              pp++;
            if (!*pp) break;
            while (pp[tl] && pp[tl] != '\n' && pp[tl] != ',' && pp[tl] != ' ' &&
                   pp[tl] != '\t' && pp[tl] != ':' && tl + 1 < sizeof tok) {
              tok[tl] = pp[tl];
              tl++;
            }
            tok[tl] = 0;
            pp += tl;
            if (tok[0])
              snprintf(names[nname++], sizeof names[0], "%s", tok);
          }
          lex_next(L);
        } else if (vv && vv->is_str && vv->sval[0] && !oop_find_class(vm, L->cur.text)) {
          snprintf(names[nname++], sizeof names[0], "%s", vv->sval);
          lex_next(L);
        } else if (strcmp(L->cur.text, "LAST") == 0) {
          snprintf(names[nname++], sizeof names[0], "%s", vm->last_str);
          lex_next(L);
        } else {
          snprintf(names[nname++], sizeof names[0], "%s", L->cur.text);
          lex_next(L);
        }
      }
    }
    if (nname == 0) {
      if (hard) {
        fail_at(vm, L, "NEEDCLASSANY nameâ¦ â NEEDCLASSANY Ticket Cell Â· HASCLASSANY bag");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASCLASSANY_N", 0);
      var_set_num(vm, "CLASSMISS_N", 0);
      var_set_num(vm, "CLASSHAVE_N", 0);
      var_set_str(vm, "CLASSMISS", "");
      var_set_str(vm, "CLASSHAVE", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASCLASSANY: need names â HASCLASSANY Ticket Ghost");
      var_set_str(vm, "ERR", "HASCLASSANY: need names â HASCLASSANY Ticket Ghost");
      bump(vm);
      return 1;
    }
    for (i = 0; i < nname; i++) {
      if (oop_find_class(vm, names[i])) {
        nhave++;
        if (ho && ho + 1 < sizeof havebag) havebag[ho++] = '\n';
        {
          size_t ln = strlen(names[i]);
          if (ho + ln < sizeof havebag) {
            memcpy(havebag + ho, names[i], ln);
            ho += ln;
            havebag[ho] = 0;
          }
        }
      } else {
        nmiss++;
        if (mo && mo + 1 < sizeof miss) miss[mo++] = '\n';
        {
          size_t ln = strlen(names[i]);
          if (mo + ln < sizeof miss) {
            memcpy(miss + mo, names[i], ln);
            mo += ln;
            miss[mo] = 0;
          }
        }
      }
    }
    var_set_str(vm, "CLASSMISS", miss);
    var_set_str(vm, "CLASSHAVE", havebag);
    var_set_num(vm, "CLASSMISS_N", nmiss);
    var_set_num(vm, "CLASSHAVE_N", nhave);
    var_set_num(vm, "HASCLASSANY_N", nhave);
    var_set_num(vm, "NEEDCLASSANY_N", nname);
    var_set_num(vm, "CLASSES_ANY_N", nname);
    if (nhave > 0) {
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST", "1");
      snprintf(vm->last_str, sizeof vm->last_str, "1");
      var_set_num(vm, "OK", 1);
    } else if (hard) {
      char em[240];
      snprintf(em, sizeof em,
               "NEEDCLASSANY miss line %d: need one of [%s] â CLASS/INCLUDE Â· HASCLASSANY Â· LISTCLASSES",
               aln, miss[0] ? miss : "?");
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      fail_at(vm, L, em);
      return -1;
    } else {
      char em[240];
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      snprintf(em, sizeof em,
               "HASCLASSANY miss (0/%d): need one of %s â NEEDCLASSANY Â· INCLUDE",
               nname, miss[0] ? miss : "?");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%d have=%d miss=%d\n",
              hard ? "NEEDCLASSANY" : "HASCLASSANY", nname, nhave, nmiss);
    bump(vm);
    return 1;
  }

  /* LISTCLASSES|CLASSES â newline bag of defined class names.
   * LAST = bag; LAST_N / NCLASSES = count. Agent discovery after INCLUDE. */
  if (kw(&L->cur, "LISTCLASSES") || kw(&L->cur, "CLASSES") ||
      kw(&L->cur, "CLASSLIST") || kw(&L->cur, "LISTCLASS")) {
    char bag[2048];
    size_t o = 0;
    int i, n = 0;
    lex_next(L);
    bag[0] = 0;
    for (i = 0; i < vm->n_classes; i++) {
      size_t ln = strlen(vm->classes[i].name);
      if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, vm->classes[i].name, ln);
        o += ln;
      }
      bag[o] = 0;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LISTCLASSES", bag);
    var_set_str(vm, "CLASSES", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "NCLASSES", n);
    var_set_num(vm, "LISTCLASSES_N", n);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LISTOBJS|OBJECTS [ClassName] â live object names (optional class filter).
   * LAST = newline bag; LAST_N / NOBJS = count. Optional: only that class. */
  if (kw(&L->cur, "LISTOBJS") || kw(&L->cur, "OBJECTS") ||
      kw(&L->cur, "OBJLIST") || kw(&L->cur, "LISTOBJECTS") ||
      kw(&L->cur, "LISTINST")) {
    char bag[2048], filt[48];
    size_t o = 0;
    int i, n = 0, has_filt = 0;
    lex_next(L);
    filt[0] = 0;
    if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
        !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") && !kw(&L->cur, "SYS") &&
        !kw(&L->cur, "PRINT") && !kw(&L->cur, "CUBE") && !kw(&L->cur, "END")) {
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_STR) {
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    }
    bag[0] = 0;
    for (i = 0; i < vm->n_objs; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      size_t ln;
      if (!ob->live) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
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
    var_set_str(vm, "LISTOBJS", bag);
    var_set_str(vm, "OBJECTS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "NOBJS", n);
    var_set_num(vm, "LISTOBJS_N", n);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* PICKOBJ|RANDOBJ|SAMPLEOBJ [Class]
   * â random live object name (optional class filter). Soft empty â "" / -1.
   * LAST=name; LAST_N/PICKOBJ_I = 0-based index among candidates; PICKOBJ_TOTAL=pool.
   * Uses vm->rng (SEED / CUBALC_SEED). Usability: peer/work sample without LISTOBJS+SYS PICK. */
  if (kw(&L->cur, "PICKOBJ") || kw(&L->cur, "RANDOBJ") ||
      kw(&L->cur, "SAMPLEOBJ") || kw(&L->cur, "ANYOBJ") ||
      kw(&L->cur, "DRAWOBJ") || kw(&L->cur, "LOTTOBJ") ||
      kw(&L->cur, "CHOICEOBJ") || kw(&L->cur, "RANDINST") ||
      kw(&L->cur, "PICKINST") || kw(&L->cur, "SAMPLEINST")) {
    char filt[48], bag_names[CUBALC_MAX_OBJS][48];
    int has_filt = 0, i, n = 0, idx = -1;
    uint32_t x;
    lex_next(L);
    filt[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "FROM")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "PICKOBJ OF Class"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
               !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
               !kw(&L->cur, "END") && !kw(&L->cur, "NEW") &&
               !kw(&L->cur, "CUBE") && oop_find_class(vm, L->cur.text)) {
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_STR) {
      /* string class name even if not yet defined â empty pool */
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    }
    for (i = 0; i < vm->n_objs && n < CUBALC_MAX_OBJS; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      snprintf(bag_names[n], sizeof bag_names[n], "%s", ob->name);
      n++;
    }
    if (n <= 0) {
      var_set_str(vm, "LAST", "");
      var_set_str(vm, "PICKOBJ", "");
      var_set_str(vm, "RANDOBJ", "");
      var_set_str(vm, "OBJECT", "");
      vm->last_str[0] = 0;
      vm->last_n = -1;
      var_set_num(vm, "LAST_N", -1);
      var_set_num(vm, "PICKOBJ_I", -1);
      var_set_num(vm, "PICKOBJ_N", -1);
      var_set_num(vm, "PICKOBJ_TOTAL", 0);
      var_set_num(vm, "PICKOBJ_HIT", 0);
      if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
      var_set_num(vm, "OK", 1);
      bump(vm);
      return 1;
    }
    x = vm->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    if (!x) x = 1;
    vm->rng = x;
    idx = (int)(x % (uint32_t)n);
    var_set_str(vm, "LAST", bag_names[idx]);
    var_set_str(vm, "PICKOBJ", bag_names[idx]);
    var_set_str(vm, "RANDOBJ", bag_names[idx]);
    var_set_str(vm, "SAMPLEOBJ", bag_names[idx]);
    var_set_str(vm, "OBJECT", bag_names[idx]);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag_names[idx]);
    vm->last_n = idx;
    var_set_num(vm, "LAST_N", idx);
    var_set_num(vm, "PICKOBJ_I", idx);
    var_set_num(vm, "PICKOBJ_N", idx);
    var_set_num(vm, "PICKOBJ_TOTAL", n);
    var_set_num(vm, "PICKOBJ_HIT", 1);
    var_set_num(vm, "NOBJS", n);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* DRAWNOBJ|SAMPLEKOBJ|PICKNOBJ [Class] k
   * â sample k unique live object names without replacement (partial FisherâYates).
   * k<=0 â empty; k>=pool â all shuffled. LAST bag; LAST_N=count; DRAWNOBJ_TOTAL=pool.
   * Uses vm->rng (SEED/CUBALC_SEED). Usability: multi-peer sample without LISTOBJS+DRAWN. */
  if (kw(&L->cur, "DRAWNOBJ") || kw(&L->cur, "SAMPLEKOBJ") ||
      kw(&L->cur, "PICKNOBJ") || kw(&L->cur, "NPICKOBJ") ||
      kw(&L->cur, "TAKERANDOBJ") || kw(&L->cur, "DRAWNINST") ||
      kw(&L->cur, "SAMPLEKINST") || kw(&L->cur, "DRAWKOBJ") ||
      kw(&L->cur, "RSAMPLEOBJ") || kw(&L->cur, "MULTIDRAWOBJ")) {
    char filt[48], bag_names[CUBALC_MAX_OBJS][48], bag[4096], tmp[48];
    int has_filt = 0, i, n = 0, take = 0, out_n = 0;
    long karg = 0;
    uint32_t x;
    size_t o = 0;
    lex_next(L);
    filt[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "FROM")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "DRAWNOBJ OF Class k"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if ((L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) &&
               L->cur.kind != TK_NUM) {
      /* Class name only if known class and next looks like k */
      if (L->cur.kind == TK_STR) {
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      } else if (oop_find_class(vm, L->cur.text) &&
                 !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
                 !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
                 !kw(&L->cur, "END") && !kw(&L->cur, "NEW")) {
        char maybe[48];
        snprintf(maybe, sizeof maybe, "%s", L->cur.text);
        lex_next(L);
        /* if next is k (num/ident/TAKE/K) treat maybe as class; else fail later as k */
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
            L->cur.kind == TK_LPAREN || L->cur.kind == TK_STR ||
            kw(&L->cur, "K") || kw(&L->cur, "TAKE") || kw(&L->cur, "COUNT") ||
            kw(&L->cur, "LIMIT") ||
            (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
             !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET"))) {
          snprintf(filt, sizeof filt, "%s", maybe);
          has_filt = 1;
        } else {
          /* put back: treat maybe as k via re-parse impossible; use as k string */
          fail(vm, "DRAWNOBJ [Class] k"); return -1;
        }
      }
    }
    /* optional K|TAKE|COUNT|LIMIT sugar â avoid bare N (steals var n) */
    if (kw(&L->cur, "TAKE") || kw(&L->cur, "COUNT") ||
        kw(&L->cur, "LIMIT") || kw(&L->cur, "SIZE") || kw(&L->cur, "DRAW"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      karg = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
      karg = vm->last_n;
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
               !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
               !kw(&L->cur, "END") && !kw(&L->cur, "NEW")) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str)
        karg = atol(sv->sval);
      else
        karg = parse_expr(vm, L);
      if (sv && sv->is_str) lex_next(L);
    } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
               L->cur.kind == TK_LPAREN) {
      karg = parse_expr(vm, L);
    } else {
      fail(vm, "DRAWNOBJ [Class] k"); return -1;
    }
    if (karg < 0) karg = 0;
    take = (int)karg;
    if (take > CUBALC_MAX_OBJS) take = CUBALC_MAX_OBJS;
    for (i = 0; i < vm->n_objs && n < CUBALC_MAX_OBJS; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      snprintf(bag_names[n], sizeof bag_names[n], "%s", ob->name);
      n++;
    }
    out_n = take < n ? take : n;
    /* partial FisherâYates: shuffle first out_n into front */
    for (i = 0; i < out_n; i++) {
      int r;
      x = vm->rng;
      x ^= x << 13;
      x ^= x >> 17;
      x ^= x << 5;
      if (!x) x = 1;
      vm->rng = x;
      r = i + (int)(x % (uint32_t)(n - i));
      if (r != i) {
        snprintf(tmp, sizeof tmp, "%s", bag_names[i]);
        snprintf(bag_names[i], sizeof bag_names[i], "%s", bag_names[r]);
        snprintf(bag_names[r], sizeof bag_names[r], "%s", tmp);
      }
    }
    for (i = 0; i < out_n; i++) {
      size_t ln = strlen(bag_names[i]);
      if (i > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, bag_names[i], ln);
        o += ln;
      }
      bag[o] = 0;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "DRAWNOBJ", bag);
    var_set_str(vm, "SAMPLEKOBJ", bag);
    var_set_str(vm, "PICKNOBJ", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = out_n;
    var_set_num(vm, "LAST_N", out_n);
    var_set_num(vm, "DRAWNOBJ_N", out_n);
    var_set_num(vm, "SAMPLEKOBJ_N", out_n);
    var_set_num(vm, "PICKNOBJ_N", out_n);
    var_set_num(vm, "DRAWNOBJ_TOTAL", n);
    var_set_num(vm, "DRAWNOBJ_REQ", karg);
    var_set_num(vm, "NOBJS", n);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SHUFFLEOBJS|SHUFOBJS|PERMUTEOBJS [Class]
   * â FisherâYates shuffle of live object names â bag (optional class filter).
   * Soft empty â "". LAST bag; LAST_N/SHUFFLEOBJS_N = count. Uses vm->rng (SEED).
   * Usability: randomize fleet order without LISTOBJS + SYS SHUFFLE glue. */
  if (kw(&L->cur, "SHUFFLEOBJS") || kw(&L->cur, "SHUFOBJS") ||
      kw(&L->cur, "PERMUTEOBJS") || kw(&L->cur, "SHUFFLEOBJ") ||
      kw(&L->cur, "MIXOBJS") || kw(&L->cur, "SCRAMBLEOBJS") ||
      kw(&L->cur, "RANDOBJORDER") || kw(&L->cur, "SHUFINST") ||
      kw(&L->cur, "SHUFFLEINST") || kw(&L->cur, "OBJSHUFFLE")) {
    char filt[48], bag_names[CUBALC_MAX_OBJS][48], bag[4096], tmp[48];
    int has_filt = 0, i, n = 0;
    uint32_t x;
    size_t o = 0;
    lex_next(L);
    filt[0] = 0;
    bag[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE") ||
        kw(&L->cur, "FROM")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
        fail(vm, "SHUFFLEOBJS OF Class"); return -1;
      }
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
               !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") &&
               !kw(&L->cur, "END") && !kw(&L->cur, "NEW") &&
               !kw(&L->cur, "CUBE") && oop_find_class(vm, L->cur.text)) {
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_STR) {
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    }
    for (i = 0; i < vm->n_objs && n < CUBALC_MAX_OBJS; i++) {
      ObjInst *ob = &vm->objs[i];
      ClassDef *cd;
      if (!ob->live) continue;
      if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) continue;
      cd = &vm->classes[ob->class_idx];
      if (has_filt && filt[0] && strcmp(cd->name, filt) != 0) continue;
      snprintf(bag_names[n], sizeof bag_names[n], "%s", ob->name);
      n++;
    }
    /* FisherâYates */
    for (i = n - 1; i > 0; i--) {
      int r;
      x = vm->rng;
      x ^= x << 13;
      x ^= x >> 17;
      x ^= x << 5;
      if (!x) x = 1;
      vm->rng = x;
      r = (int)(x % (uint32_t)(i + 1));
      if (r != i) {
        snprintf(tmp, sizeof tmp, "%s", bag_names[i]);
        snprintf(bag_names[i], sizeof bag_names[i], "%s", bag_names[r]);
        snprintf(bag_names[r], sizeof bag_names[r], "%s", tmp);
      }
    }
    for (i = 0; i < n; i++) {
      size_t ln = strlen(bag_names[i]);
      if (i > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, bag_names[i], ln);
        o += ln;
      }
      bag[o] = 0;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "SHUFFLEOBJS", bag);
    var_set_str(vm, "SHUFOBJS", bag);
    var_set_str(vm, "PERMUTEOBJS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "SHUFFLEOBJS_N", n);
    var_set_num(vm, "SHUFOBJS_N", n);
    var_set_num(vm, "PERMUTEOBJS_N", n);
    var_set_num(vm, "NOBJS", n);
    if (has_filt && filt[0]) var_set_str(vm, "CLASS", filt);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
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

/* HASOBJ|LIVES|ISOBJ|ALIVEOBJ name â soft probe if live object exists.
   * LAST_N 1|0. Usability: IF before SEND/GETF without fatal. */
  if (kw(&L->cur, "HASOBJ") || kw(&L->cur, "LIVES") || kw(&L->cur, "ISOBJ") ||
      kw(&L->cur, "ALIVEOBJ") || kw(&L->cur, "OBJ?") || kw(&L->cur, "HASOBJECT") ||
      kw(&L->cur, "OBJEXISTS") || kw(&L->cur, "EXISTS_OBJ")) {
    char oname[48];
    ObjInst *ob;
    int hit = 0;
    lex_next(L);
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "HASOBJ name"); return -1;
    }
    ob = oop_find_obj(vm, oname);
    hit = ob ? 1 : 0;
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    {
      char nb[8];
      snprintf(nb, sizeof nb, "%d", hit);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "HASOBJ_N", hit);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASOBJS|NEEDOBJS nameâ¦ â multi live-object capability gate (all-or-OBJMISS).
   * Soft: LAST_N 0|1 Â· OBJMISS bag Â· NEEDOBJS fail-fast first miss.
   * Usability: agent live_session contracts without NÃ HASOBJ glue Â· bag/CSV/idents.
   * Twin of HASCLASSES for live instances Â· dual of obj_guard ALL path. */
  if (kw(&L->cur, "HASOBJS") || kw(&L->cur, "HAS_OBJS") ||
      kw(&L->cur, "ALLOBJS") || kw(&L->cur, "OBJSOK") ||
      kw(&L->cur, "NEEDOBJS") || kw(&L->cur, "NEED_OBJS") ||
      kw(&L->cur, "REQUIREOBJS") || kw(&L->cur, "MUSTOBJS")) {
    int hard = kw(&L->cur, "NEEDOBJS") || kw(&L->cur, "NEED_OBJS") ||
               kw(&L->cur, "REQUIREOBJS") || kw(&L->cur, "MUSTOBJS");
    char names[32][48];
    char miss[1024];
    int nname = 0, nmiss = 0, i, aln = L->cur.line;
    size_t mo = 0;
    lex_next(L);
    miss[0] = 0;
    while (nname < 32 &&
           (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT)) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur, "ASSERT") || kw(&L->cur, "LET") || kw(&L->cur, "PRINT") ||
           kw(&L->cur, "REQUIRE") || kw(&L->cur, "END") ||
           kw(&L->cur, "IF") || kw(&L->cur, "ELSE") || kw(&L->cur, "ELIF") ||
           kw(&L->cur, "PASS") || kw(&L->cur, "FAIL") || kw(&L->cur, "INCLUDE") ||
           kw(&L->cur, "SYS") || kw(&L->cur, "HELP") || kw(&L->cur, "CLEAR_ERR") ||
           kw(&L->cur, "NOTE") || kw(&L->cur, "EXIT") || kw(&L->cur, "DEFAULT") ||
           kw(&L->cur, "VERSION") || kw(&L->cur, "STATUS") || kw(&L->cur, "FOR") ||
           kw(&L->cur, "WHILE") || kw(&L->cur, "LOOP") || kw(&L->cur, "EACH") ||
           kw(&L->cur, "CUBE") || kw(&L->cur, "PLUG") || kw(&L->cur, "HOLD_FLASH") ||
           kw(&L->cur, "CLASS") || kw(&L->cur, "NEW") || kw(&L->cur, "HASOBJ") ||
           kw(&L->cur, "HASOBJS") || kw(&L->cur, "NEEDOBJS") ||
           kw(&L->cur, "HASOBJANY") || kw(&L->cur, "NEEDOBJANY") ||
           kw(&L->cur, "LISTOBJS") || kw(&L->cur, "HASCLASS") ||
           kw(&L->cur, "HASFORM") || kw(&L->cur, "SEND") || kw(&L->cur, "GETF") ||
           kw(&L->cur, "SETF") || kw(&L->cur, "DELETEOBJ") ||
           kw(&L->cur, "TRYDELETE")))
        break;
      if (L->cur.kind == TK_STR) {
        const char *s = L->cur.text;
        if (strchr(s, '\n') || strchr(s, ',') || strchr(s, ' ')) {
          const char *p = s;
          while (*p && nname < 32) {
            char tok[48];
            size_t tl = 0;
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' || *p == ':')
              p++;
            if (!*p) break;
            while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                   p[tl] != '\t' && p[tl] != ':' && tl + 1 < sizeof tok) {
              tok[tl] = p[tl];
              tl++;
            }
            tok[tl] = 0;
            p += tl;
            if (tok[0])
              snprintf(names[nname++], sizeof names[0], "%s", tok);
          }
          lex_next(L);
          continue;
        }
        snprintf(names[nname++], sizeof names[0], "%s", s);
        lex_next(L);
      } else {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (strchr(vv->sval, '\n') || strchr(vv->sval, ',') ||
             strchr(vv->sval, ' '))) {
          const char *p = vv->sval;
          while (*p && nname < 32) {
            char tok[48];
            size_t tl = 0;
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' || *p == ':')
              p++;
            if (!*p) break;
            while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                   p[tl] != '\t' && p[tl] != ':' && tl + 1 < sizeof tok) {
              tok[tl] = p[tl];
              tl++;
            }
            tok[tl] = 0;
            p += tl;
            if (tok[0])
              snprintf(names[nname++], sizeof names[0], "%s", tok);
          }
          lex_next(L);
        } else if (vv && vv->is_str && vv->sval[0] && !oop_find_obj(vm, L->cur.text)) {
          /* expand string-var object name when ident is not a live obj */
          snprintf(names[nname++], sizeof names[0], "%s", vv->sval);
          lex_next(L);
        } else if (strcmp(L->cur.text, "LAST") == 0) {
          snprintf(names[nname++], sizeof names[0], "%s", vm->last_str);
          lex_next(L);
        } else {
          snprintf(names[nname++], sizeof names[0], "%s", L->cur.text);
          lex_next(L);
        }
      }
    }
    if (nname == 0) {
      if (hard) {
        fail_at(vm, L, "NEEDOBJS nameâ¦ â NEEDOBJS a b Â· HASOBJS bag");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASOBJS_N", 0);
      var_set_num(vm, "OBJMISS_N", 0);
      var_set_str(vm, "OBJMISS", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASOBJS: need names â HASOBJS a b");
      var_set_str(vm, "ERR", "HASOBJS: need names â HASOBJS a b");
      bump(vm);
      return 1;
    }
    for (i = 0; i < nname; i++) {
      if (oop_find_obj(vm, names[i]))
        continue;
      nmiss++;
      if (mo && mo + 1 < sizeof miss) miss[mo++] = '\n';
      {
        size_t ln = strlen(names[i]);
        if (mo + ln < sizeof miss) {
          memcpy(miss + mo, names[i], ln);
          mo += ln;
          miss[mo] = 0;
        }
      }
      if (hard) {
        char em[220];
        snprintf(em, sizeof em,
                 "NEEDOBJS miss line %d: '%s' (+%d more?) â NEW/ENSURENEW Â· HASOBJ Â· LISTOBJS",
                 aln, names[i], nname - i - 1);
        fail_at(vm, L, em);
        return -1;
      }
    }
    var_set_str(vm, "OBJMISS", miss);
    var_set_num(vm, "OBJMISS_N", nmiss);
    var_set_num(vm, "HASOBJS_N", nname - nmiss);
    var_set_num(vm, "NEEDOBJS_N", nname);
    var_set_num(vm, "OBJS_N", nname);
    if (nmiss == 0) {
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST", "1");
      snprintf(vm->last_str, sizeof vm->last_str, "1");
      var_set_num(vm, "OK", 1);
    } else {
      char em[220];
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      snprintf(em, sizeof em,
               "HASOBJS miss (%d/%d): %s â NEW/ENSURENEW Â· NEEDOBJS Â· LISTOBJS",
               nmiss, nname, miss);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%d miss=%d\n",
              hard ? "NEEDOBJS" : "HASOBJS", nname, nmiss);
    bump(vm);
    return 1;
  }

  /* HASOBJANY|NEEDOBJANY nameâ¦ â multi live-object any-of gate (HASOBJS twin).
   * Soft: LAST_N 1 if any live Â· OBJHAVE bag Â· OBJMISS bag.
   * NEEDOBJANY fail-fast if none of the listed objects exist.
   * Usability: optional peer/session OR without NÃ HASOBJ IF glue. */
  if (kw(&L->cur, "HASOBJANY") || kw(&L->cur, "HAS_OBJ_ANY") ||
      kw(&L->cur, "OBJANY") || kw(&L->cur, "HASANYOBJ") ||
      kw(&L->cur, "LIVESANY") || kw(&L->cur, "NEEDOBJANY") ||
      kw(&L->cur, "NEED_OBJ_ANY") || kw(&L->cur, "REQUIREOBJANY") ||
      kw(&L->cur, "MUSTOBJANY")) {
    int hard = kw(&L->cur, "NEEDOBJANY") || kw(&L->cur, "NEED_OBJ_ANY") ||
               kw(&L->cur, "REQUIREOBJANY") || kw(&L->cur, "MUSTOBJANY");
    char names[32][48];
    char miss[1024], havebag[1024];
    int nname = 0, nmiss = 0, nhave = 0, i, aln = L->cur.line;
    size_t mo = 0, ho = 0;
    lex_next(L);
    miss[0] = 0;
    havebag[0] = 0;
    while (nname < 32 &&
           (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT)) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur, "ASSERT") || kw(&L->cur, "LET") || kw(&L->cur, "PRINT") ||
           kw(&L->cur, "REQUIRE") || kw(&L->cur, "END") ||
           kw(&L->cur, "IF") || kw(&L->cur, "ELSE") || kw(&L->cur, "ELIF") ||
           kw(&L->cur, "PASS") || kw(&L->cur, "FAIL") || kw(&L->cur, "INCLUDE") ||
           kw(&L->cur, "SYS") || kw(&L->cur, "HELP") || kw(&L->cur, "CLEAR_ERR") ||
           kw(&L->cur, "NOTE") || kw(&L->cur, "EXIT") || kw(&L->cur, "DEFAULT") ||
           kw(&L->cur, "VERSION") || kw(&L->cur, "STATUS") || kw(&L->cur, "FOR") ||
           kw(&L->cur, "WHILE") || kw(&L->cur, "LOOP") || kw(&L->cur, "EACH") ||
           kw(&L->cur, "CUBE") || kw(&L->cur, "PLUG") || kw(&L->cur, "HOLD_FLASH") ||
           kw(&L->cur, "CLASS") || kw(&L->cur, "NEW") || kw(&L->cur, "HASOBJ") ||
           kw(&L->cur, "HASOBJS") || kw(&L->cur, "NEEDOBJS") ||
           kw(&L->cur, "HASOBJANY") || kw(&L->cur, "NEEDOBJANY") ||
           kw(&L->cur, "LISTOBJS") || kw(&L->cur, "HASCLASS") ||
           kw(&L->cur, "HASFORM") || kw(&L->cur, "SEND") || kw(&L->cur, "GETF") ||
           kw(&L->cur, "SETF") || kw(&L->cur, "DELETEOBJ") ||
           kw(&L->cur, "TRYDELETE")))
        break;
      if (L->cur.kind == TK_STR) {
        const char *s = L->cur.text;
        if (strchr(s, '\n') || strchr(s, ',') || strchr(s, ' ')) {
          const char *p = s;
          while (*p && nname < 32) {
            char tok[48];
            size_t tl = 0;
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' || *p == ':')
              p++;
            if (!*p) break;
            while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                   p[tl] != '\t' && p[tl] != ':' && tl + 1 < sizeof tok) {
              tok[tl] = p[tl];
              tl++;
            }
            tok[tl] = 0;
            p += tl;
            if (tok[0])
              snprintf(names[nname++], sizeof names[0], "%s", tok);
          }
          lex_next(L);
          continue;
        }
        snprintf(names[nname++], sizeof names[0], "%s", s);
        lex_next(L);
      } else {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (strchr(vv->sval, '\n') || strchr(vv->sval, ',') ||
             strchr(vv->sval, ' '))) {
          const char *p = vv->sval;
          while (*p && nname < 32) {
            char tok[48];
            size_t tl = 0;
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' || *p == ':')
              p++;
            if (!*p) break;
            while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                   p[tl] != '\t' && p[tl] != ':' && tl + 1 < sizeof tok) {
              tok[tl] = p[tl];
              tl++;
            }
            tok[tl] = 0;
            p += tl;
            if (tok[0])
              snprintf(names[nname++], sizeof names[0], "%s", tok);
          }
          lex_next(L);
        } else if (vv && vv->is_str && vv->sval[0] && !oop_find_obj(vm, L->cur.text)) {
          snprintf(names[nname++], sizeof names[0], "%s", vv->sval);
          lex_next(L);
        } else if (strcmp(L->cur.text, "LAST") == 0) {
          snprintf(names[nname++], sizeof names[0], "%s", vm->last_str);
          lex_next(L);
        } else {
          snprintf(names[nname++], sizeof names[0], "%s", L->cur.text);
          lex_next(L);
        }
      }
    }
    if (nname == 0) {
      if (hard) {
        fail_at(vm, L, "NEEDOBJANY nameâ¦ â NEEDOBJANY a b Â· HASOBJANY bag");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASOBJANY_N", 0);
      var_set_num(vm, "OBJMISS_N", 0);
      var_set_num(vm, "OBJHAVE_N", 0);
      var_set_str(vm, "OBJMISS", "");
      var_set_str(vm, "OBJHAVE", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASOBJANY: need names â HASOBJANY a b");
      var_set_str(vm, "ERR", "HASOBJANY: need names â HASOBJANY a b");
      bump(vm);
      return 1;
    }
    for (i = 0; i < nname; i++) {
      if (oop_find_obj(vm, names[i])) {
        nhave++;
        if (ho && ho + 1 < sizeof havebag) havebag[ho++] = '\n';
        {
          size_t ln = strlen(names[i]);
          if (ho + ln < sizeof havebag) {
            memcpy(havebag + ho, names[i], ln);
            ho += ln;
            havebag[ho] = 0;
          }
        }
      } else {
        nmiss++;
        if (mo && mo + 1 < sizeof miss) miss[mo++] = '\n';
        {
          size_t ln = strlen(names[i]);
          if (mo + ln < sizeof miss) {
            memcpy(miss + mo, names[i], ln);
            mo += ln;
            miss[mo] = 0;
          }
        }
      }
    }
    var_set_str(vm, "OBJMISS", miss);
    var_set_str(vm, "OBJHAVE", havebag);
    var_set_num(vm, "OBJMISS_N", nmiss);
    var_set_num(vm, "OBJHAVE_N", nhave);
    var_set_num(vm, "HASOBJANY_N", nhave);
    var_set_num(vm, "NEEDOBJANY_N", nname);
    var_set_num(vm, "OBJS_ANY_N", nname);
    if (nhave > 0) {
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST", "1");
      snprintf(vm->last_str, sizeof vm->last_str, "1");
      var_set_num(vm, "OK", 1);
    } else if (hard) {
      char em[240];
      snprintf(em, sizeof em,
               "NEEDOBJANY miss line %d: need one of [%s] â NEW/ENSURENEW Â· HASOBJANY Â· LISTOBJS",
               aln, miss[0] ? miss : "?");
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      fail_at(vm, L, em);
      return -1;
    } else {
      char em[240];
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      snprintf(em, sizeof em,
               "HASOBJANY miss (0/%d): need one of %s â NEEDOBJANY Â· NEW/ENSURENEW",
               nname, miss[0] ? miss : "?");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%d have=%d miss=%d\n",
              hard ? "NEEDOBJANY" : "HASOBJANY", nname, nhave, nmiss);
    bump(vm);
    return 1;
  }

  /* DELETEOBJ|FREEOBJ|TRYDELETE|SOFTDELETE name â mark live object dead (free slot).
   * Note: DESTROY is cube DECONSTRUCT (ops_cell) â not this form.
   * Soft OK=0 if missing (always soft). NEW reuses same name. Usability: pool recycle.
   * TRYDELETE|DELETEOBJ SOFT|SOFTDELETE â explicit TRY* dual of DELETEOBJ (twin TRYCLONE/
   * TRYRENAME/TRYSWAP) Â· sets TRYDELETE_N Â· agent GC without HASOBJ+IF glue. */
  if (kw(&L->cur, "TRYDELETE") || kw(&L->cur, "SOFTDELETE") ||
      kw(&L->cur, "DELETEOBJSOFT") || kw(&L->cur, "TRYFREEOBJ") ||
      kw(&L->cur, "TRYDELOBJ") || kw(&L->cur, "SOFTFREE") ||
      kw(&L->cur, "TRYDISPOSE") || kw(&L->cur, "SOFTDISPOSE") ||
      kw(&L->cur, "DELETEOBJ") || kw(&L->cur, "FREEOBJ") ||
      kw(&L->cur, "DELOBJ") || kw(&L->cur, "RELEASE") ||
      kw(&L->cur, "KILLOBJ") || kw(&L->cur, "DISPOSE") ||
      kw(&L->cur, "UNOBJ") || kw(&L->cur, "OBJDEL") ||
      kw(&L->cur, "DEL_OBJ") || kw(&L->cur, "FREESLOT")) {
    char oname[48], op0[32];
    ObjInst *ob;
    int soft_try = 0;
    snprintf(op0, sizeof op0, "%s", L->cur.text);
    soft_try = (strcmp(op0, "TRYDELETE") == 0 || strcmp(op0, "SOFTDELETE") == 0 ||
                strcmp(op0, "DELETEOBJSOFT") == 0 || strcmp(op0, "TRYFREEOBJ") == 0 ||
                strcmp(op0, "TRYDELOBJ") == 0 || strcmp(op0, "SOFTFREE") == 0 ||
                strcmp(op0, "TRYDISPOSE") == 0 || strcmp(op0, "SOFTDISPOSE") == 0);
    lex_next(L);
    /* DELETEOBJ SOFT name Â· FREE SOFT name */
    if (!soft_try && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY"))) {
      soft_try = 1;
      lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(oname, sizeof oname, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      if (strcmp(L->cur.text, "LAST") == 0) {
        snprintf(oname, sizeof oname, "%s", vm->last_str);
        lex_next(L);
      } else {
        snprintf(oname, sizeof oname, "%s", L->cur.text);
        lex_next(L);
      }
    } else {
      fail(vm, soft_try ? "TRYDELETE name" : "DELETEOBJ name"); return -1;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "OK", 0);
      var_set_num(vm, "DELETEOBJ_N", 0);
      var_set_num(vm, "FREEOBJ_N", 0);
      var_set_num(vm, "TRYDELETE_N", 0);
      var_set_str(vm, "LAST_ERR", "DELETEOBJ: unknown object");
      var_set_str(vm, "ERR", "DELETEOBJ: unknown object");
      if (vm->trace) fprintf(vm->trace, "# TRYDELETE soft miss %s\n", oname);
      bump(vm);
      return 1;
    }
    ob->live = 0;
    /* clear this if receiver destroyed */
    if (strcmp(vm->this_obj, oname) == 0)
      vm->this_obj[0] = 0;
    var_set_str(vm, "LAST", oname);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", oname);
    var_set_num(vm, "LAST_N", 1);
    vm->last_n = 1;
    var_set_num(vm, "DELETEOBJ_N", 1);
    var_set_num(vm, "FREEOBJ_N", 1);
    if (soft_try) var_set_num(vm, "TRYDELETE_N", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace) fprintf(vm->trace, "# %s %s\n", soft_try ? "TRYDELETE" : "DELETEOBJ", oname);
    bump(vm);
    return 1;
  }

  /* DELETEALL|FREEALL|CLEAROBJS [Class] â free every live object (optional class).
   * Soft always; LAST_N/DELETEALL_N = count freed. Complements DELETEOBJ + SENDALL.
   * Usability: pool wipe without EACH OBJ + DELETEOBJ glue. */
  if (kw(&L->cur, "DELETEALL") || kw(&L->cur, "FREEALL") ||
      kw(&L->cur, "CLEAROBJS") || kw(&L->cur, "CLEAROBJECTS") ||
      kw(&L->cur, "PURGEOBJS") || kw(&L->cur, "KILLALL") ||
      kw(&L->cur, "DELOBJALL") || kw(&L->cur, "OBJCLEAR")) {
    char filt[48];
    int has_filt = 0, i, n = 0;
    lex_next(L);
    filt[0] = 0;
    if (kw(&L->cur, "OF") || kw(&L->cur, "CLASS") || kw(&L->cur, "TYPE")) {
      lex_next(L);
      if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
        snprintf(filt, sizeof filt, "%s", L->cur.text);
        lex_next(L);
        has_filt = 1;
      }
    } else if (L->cur.kind == TK_STR) {
      snprintf(filt, sizeof filt, "%s", L->cur.text);
      lex_next(L);
      has_filt = 1;
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L) &&
               !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
               !kw(&L->cur, "SYS") && !kw(&L->cur, "PRINT") &&
               !kw(&L->cur, "END") && !kw(&L->cur, "CUBE") &&
               !kw(&L->cur, "NEW") && !kw(&L->cur, "OF")) {
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
      ob->live = 0;
      if (strcmp(vm->this_obj, ob->name) == 0)
        vm->this_obj[0] = 0;
      n++;
    }
    var_set_num(vm, "LAST_N", n);
    vm->last_n = n;
    var_set_num(vm, "DELETEALL_N", n);
    var_set_num(vm, "FREEALL_N", n);
    var_set_num(vm, "CLEAROBJS_N", n);
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

  /* CLONEOBJ|COPYOBJ|DUPLOBJ|TRYCLONE|ENSURECLONE src dst â shallow copy live
   * object fields into new instance of same class. Does not re-run init.
   * Soft miss src always OK=0. TRYCLONE|CLONE SOFT: dst redefine/pool soft.
   * ENSURECLONE|CLONE OR: if dst already live â keep (ENSURECLONE_N=0);
   * if missing â soft clone (ENSURECLONE_N=1). Twin of ENSURENEW for protos.
   * Hard CLONEOBJ: dst live already â fatal redefine.
   * Usability: template spawn without HASOBJ+IF+CLONE glue. */
  if (kw(&L->cur, "ENSURECLONE") || kw(&L->cur, "GETORCLONE") ||
      kw(&L->cur, "CLONEIFMISS") || kw(&L->cur, "ENSURECOPY") ||
      kw(&L->cur, "TRYCLONE") || kw(&L->cur, "SOFTCLONE") ||
      kw(&L->cur, "CLONEOBJSOFT") || kw(&L->cur, "TRYCOPYOBJ") ||
      kw(&L->cur, "CLONEOBJ") || kw(&L->cur, "COPYOBJ") ||
      kw(&L->cur, "DUPLOBJ") || kw(&L->cur, "CLONE") ||
      kw(&L->cur, "OBJCLONE") || kw(&L->cur, "OBJCOPY") ||
      kw(&L->cur, "DUP_OBJ") || kw(&L->cur, "COPY_OBJ")) {
    char sname[48], dname[48];
    ObjInst *src, *dst;
    ClassDef *cd;
    int fi, soft = 0, ensure = 0, rc;
    char op0[32];
    snprintf(op0, sizeof op0, "%s", L->cur.text);
    if (strcmp(op0, "ENSURECLONE") == 0 || strcmp(op0, "GETORCLONE") == 0 ||
        strcmp(op0, "CLONEIFMISS") == 0 || strcmp(op0, "ENSURECOPY") == 0) {
      ensure = 1;
      soft = 1;
    } else if (strcmp(op0, "TRYCLONE") == 0 || strcmp(op0, "SOFTCLONE") == 0 ||
        strcmp(op0, "CLONEOBJSOFT") == 0 || strcmp(op0, "TRYCOPYOBJ") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && !ensure && (kw(&L->cur, "OR") || kw(&L->cur, "ENSURE") ||
                  kw(&L->cur, "IFMISS") || kw(&L->cur, "UNLESS"))) {
      /* CLONE OR src dst â ensure dual of ENSURECLONE */
      ensure = 1;
      soft = 1;
      lex_next(L);
    } else if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPTIONAL") || kw(&L->cur, "MAYBE") ||
                  kw(&L->cur, "OPT"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_read_name(vm, L, sname, sizeof sname,
                      ensure ? "ENSURECLONE src" : (soft ? "TRYCLONE src" : "CLONEOBJ src")) < 0)
      return -1;
    if (kw(&L->cur, "AS") || kw(&L->cur, "TO") || kw(&L->cur, "INTO"))
      lex_next(L);
    if (oop_read_name(vm, L, dname, sizeof dname,
                      ensure ? "ENSURECLONE dst" : (soft ? "TRYCLONE dst" : "CLONEOBJ dst")) < 0)
      return -1;
    /* ENSURECLONE: dst already live â keep without re-clone */
    if (ensure && oop_find_obj(vm, dname)) {
      var_set_str(vm, "LAST", dname);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", dname);
      var_set_str(vm, "OBJECT", dname);
      var_set_str(vm, "CLONEOBJ", dname);
      var_set_str(vm, "CLONE_SRC", sname);
      var_set_num(vm, "LAST_N", 0); /* 0 = already present */
      vm->last_n = 0;
      var_set_num(vm, "CLONEOBJ_N", 0);
      var_set_num(vm, "ENSURECLONE_N", 0);
      var_set_num(vm, "ENSURED", 1);
      var_set_num(vm, "OK", 1);
      var_set_str(vm, "LAST_ERR", "");
      var_set_str(vm, "ERR", "");
      if (vm->trace) fprintf(vm->trace, "# ENSURECLONE already %s\n", dname);
      bump(vm);
      return 1;
    }
    src = oop_find_obj(vm, sname);
    if (!src) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "CLONEOBJ_N", 0);
      var_set_num(vm, "TRYCLONE_N", 0);
      if (ensure) {
        var_set_num(vm, "ENSURECLONE_N", 0);
        var_set_num(vm, "ENSURED", 0);
      }
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "CLONEOBJ: unknown source");
      var_set_str(vm, "ERR", "CLONEOBJ: unknown source");
      bump(vm);
      return 1;
    }
    if (src->class_idx < 0 || src->class_idx >= vm->n_classes) {
      fail(vm, "CLONEOBJ bad class"); return -1;
    }
    cd = &vm->classes[src->class_idx];
    /* allocate dst without ctor (field defaults then overwrite from src) */
    rc = oop_new_instance(vm, cd->name, dname, -1, NULL, 0, soft);
    if (rc == -1) return -1;
    if (rc == -2) {
      /* soft redefine / pool full â sticky already set by oop_new_instance */
      var_set_num(vm, "CLONEOBJ_N", 0);
      var_set_num(vm, "TRYCLONE_N", 0);
      if (ensure) {
        var_set_num(vm, "ENSURECLONE_N", 0);
        var_set_num(vm, "ENSURED", 0);
      }
      if (vm->trace) fprintf(vm->trace, "# TRYCLONE soft miss %s -> %s\n", sname, dname);
      bump(vm);
      return 1;
    }
    dst = oop_find_obj(vm, dname);
    if (!dst) {
      fail(vm, "CLONEOBJ alloc fail"); return -1;
    }
    for (fi = 0; fi < cd->n_fields && fi < CUBALC_MAX_FIELDS; fi++) {
      dst->fis_str[fi] = src->fis_str[fi];
      if (src->fis_str[fi])
        snprintf(dst->fstr[fi], sizeof dst->fstr[fi], "%s", src->fstr[fi]);
      else
        dst->fnum[fi] = src->fnum[fi];
    }
    /* pure object clone: not bound to src cube */
    dst->cube_idx = -1;
    var_set_str(vm, "LAST", dname);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", dname);
    var_set_str(vm, "OBJECT", dname);
    var_set_str(vm, "CLASS", cd->name);
    var_set_str(vm, "CLONEOBJ", dname);
    var_set_str(vm, "CLONE_SRC", sname);
    var_set_num(vm, "LAST_N", cd->n_fields);
    vm->last_n = cd->n_fields;
    var_set_num(vm, "CLONEOBJ_N", 1);
    if (soft && !ensure) var_set_num(vm, "TRYCLONE_N", 1);
    if (ensure) {
      var_set_num(vm, "ENSURECLONE_N", 1);
      var_set_num(vm, "ENSURED", 1);
      var_set_str(vm, "LAST_ERR", "");
      var_set_str(vm, "ERR", "");
    }
    var_set_num(vm, "NFIELDS", cd->n_fields);
    var_set_num(vm, "OK", 1);
    if (vm->trace) fprintf(vm->trace, "# %s %s -> %s\n",
                           ensure ? "ENSURECLONE" : (soft ? "TRYCLONE" : "CLONEOBJ"),
                           sname, dname);
    bump(vm);
    return 1;
  }

  /* RENAMEOBJ|MOVEOBJ|TRYRENAME old [AS|TO] new â rename live object slot in place.
   * Soft miss old always OK=0. Hard RENAMEOBJ: live destination â fatal redefine.
   * TRYRENAME|RENAMEOBJ SOFT|MOVE SOFT: redefine also soft (OK=0 LAST_ERR) â
   * twin of TRYCLONE for promote-after-clone pipelines without HASOBJ+IF glue.
   * Dead destination name cleared so pool recycle stays unique. Updates THIS.
   * Usability: promote temp/proto names after CLONE without re-NEW. */
  if (kw(&L->cur, "TRYRENAME") || kw(&L->cur, "SOFTRENAME") ||
      kw(&L->cur, "RENAMEOBJSOFT") || kw(&L->cur, "TRYMOVEOBJ") ||
      kw(&L->cur, "SOFTMOVEOBJ") || kw(&L->cur, "TRYRENOBJ") ||
      kw(&L->cur, "RENAMEOBJ") || kw(&L->cur, "MOVEOBJ") ||
      kw(&L->cur, "RENOBJ") || kw(&L->cur, "OBJRENAME") ||
      kw(&L->cur, "RENAME_OBJ") || kw(&L->cur, "MVOBJ") ||
      kw(&L->cur, "OBJMOVE") || kw(&L->cur, "ALIASOBJ")) {
    char oname[48], nname[48], oldn[48];
    ObjInst *ob;
    ClassDef *cd;
    int i, soft = 0;
    char op0[32];
    snprintf(op0, sizeof op0, "%s", L->cur.text);
    if (strcmp(op0, "TRYRENAME") == 0 || strcmp(op0, "SOFTRENAME") == 0 ||
        strcmp(op0, "RENAMEOBJSOFT") == 0 || strcmp(op0, "TRYMOVEOBJ") == 0 ||
        strcmp(op0, "SOFTMOVEOBJ") == 0 || strcmp(op0, "TRYRENOBJ") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPTIONAL") || kw(&L->cur, "MAYBE") ||
                  kw(&L->cur, "OPT"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_read_name(vm, L, oname, sizeof oname, soft ? "TRYRENAME old" : "RENAMEOBJ old") < 0)
      return -1;
    if (kw(&L->cur, "AS") || kw(&L->cur, "TO") || kw(&L->cur, "INTO"))
      lex_next(L);
    if (oop_read_name(vm, L, nname, sizeof nname, soft ? "TRYRENAME new" : "RENAMEOBJ new") < 0)
      return -1;
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "RENAMEOBJ_N", 0);
      var_set_num(vm, "TRYRENAME_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "RENAMEOBJ: unknown object");
      var_set_str(vm, "ERR", "RENAMEOBJ: unknown object");
      if (vm->trace) fprintf(vm->trace, "# TRYRENAME soft miss %s\n", oname);
      bump(vm);
      return 1;
    }
    /* no-op same name */
    if (strcmp(oname, nname) == 0) {
      var_set_str(vm, "LAST", nname);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nname);
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_num(vm, "RENAMEOBJ_N", 1);
      if (soft) var_set_num(vm, "TRYRENAME_N", 1);
      var_set_num(vm, "OK", 1);
      bump(vm);
      return 1;
    }
    /* destination live already? */
    if (oop_find_obj(vm, nname)) {
      if (soft) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        var_set_num(vm, "LAST_N", 0);
        vm->last_n = 0;
        var_set_num(vm, "RENAMEOBJ_N", 0);
        var_set_num(vm, "TRYRENAME_N", 0);
        var_set_num(vm, "OK", 0);
        {
          char msg[96];
          snprintf(msg, sizeof msg, "RENAMEOBJ: redefine %s", nname);
          var_set_str(vm, "LAST_ERR", msg);
          var_set_str(vm, "ERR", msg);
        }
        if (vm->trace) fprintf(vm->trace, "# TRYRENAME soft redefine %s -> %s\n", oname, nname);
        bump(vm);
        return 1;
      }
      snprintf(vm->err, sizeof vm->err, "RENAMEOBJ destination live %s", nname);
      fail(vm, vm->err);
      return -1;
    }
    /* clear dead slots that hold destination name (pool uniqueness) */
    for (i = 0; i < vm->n_objs; i++) {
      if (&vm->objs[i] == ob) continue;
      if (!vm->objs[i].live && strcmp(vm->objs[i].name, nname) == 0)
        vm->objs[i].name[0] = 0;
    }
    snprintf(oldn, sizeof oldn, "%s", ob->name);
    snprintf(ob->name, sizeof ob->name, "%s", nname);
    if (strcmp(vm->this_obj, oldn) == 0) {
      snprintf(vm->this_obj, sizeof vm->this_obj, "%s", nname);
      var_set_str(vm, "THIS", nname);
      var_set_str(vm, "SELF", nname);
    }
    if (ob->class_idx >= 0 && ob->class_idx < vm->n_classes) {
      cd = &vm->classes[ob->class_idx];
      var_set_str(vm, nname, cd->name);
      var_set_str(vm, "CLASS", cd->name);
    }
    var_set_str(vm, "LAST", nname);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", nname);
    var_set_str(vm, "OBJECT", nname);
    var_set_str(vm, "RENAMEOBJ", nname);
    var_set_str(vm, "RENAME_FROM", oldn);
    var_set_num(vm, "LAST_N", 1);
    vm->last_n = 1;
    var_set_num(vm, "RENAMEOBJ_N", 1);
    if (soft) var_set_num(vm, "TRYRENAME_N", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace) fprintf(vm->trace, "# %s %s -> %s\n", soft ? "TRYRENAME" : "RENAMEOBJ", oname, nname);
    bump(vm);
    return 1;
  }

  /* SWAPOBJ|EXCHANGEOBJ|XCHOBJ a [WITH|AND] b â swap live object slot names.
   * Soft miss either side always OK=0 LAST_ERR. TRYSWAP|SWAPOBJ SOFT same soft path.
   * No-op same name OK=1. Dead slots holding either name cleared for pool uniqueness.
   * Updates THIS if receiver is either side. Usability: role flip / primary-backup
   * without temp+RENAMEOBJ+RENAMEOBJ glue (agent promote/demote). */
  if (kw(&L->cur, "TRYSWAP") || kw(&L->cur, "SOFTSWAP") ||
      kw(&L->cur, "TRYSWAPOBJ") || kw(&L->cur, "SOFTSWAPOBJ") ||
      kw(&L->cur, "SWAPOBJSOFT") || kw(&L->cur, "TRYXCHOBJ") ||
      kw(&L->cur, "SWAPOBJ") || kw(&L->cur, "EXCHANGEOBJ") ||
      kw(&L->cur, "XCHOBJ") || kw(&L->cur, "OBJSWAP") ||
      kw(&L->cur, "SWAP_OBJ") || kw(&L->cur, "XCHGOBJ") ||
      kw(&L->cur, "SWAPNAMES") || kw(&L->cur, "NAMESWAP")) {
    char aname[48], bname[48], tmpn[48];
    ObjInst *oa, *ob;
    ClassDef *cd;
    int i, soft = 0;
    char op0[32];
    snprintf(op0, sizeof op0, "%s", L->cur.text);
    if (strcmp(op0, "TRYSWAP") == 0 || strcmp(op0, "SOFTSWAP") == 0 ||
        strcmp(op0, "TRYSWAPOBJ") == 0 || strcmp(op0, "SOFTSWAPOBJ") == 0 ||
        strcmp(op0, "SWAPOBJSOFT") == 0 || strcmp(op0, "TRYXCHOBJ") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPTIONAL") || kw(&L->cur, "MAYBE") ||
                  kw(&L->cur, "OPT"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_read_name(vm, L, aname, sizeof aname, soft ? "TRYSWAP a" : "SWAPOBJ a") < 0)
      return -1;
    if (kw(&L->cur, "WITH") || kw(&L->cur, "AND") || kw(&L->cur, "TO") ||
        kw(&L->cur, "AS") || kw(&L->cur, "FOR"))
      lex_next(L);
    if (oop_read_name(vm, L, bname, sizeof bname, soft ? "TRYSWAP b" : "SWAPOBJ b") < 0)
      return -1;
    /* no-op same name */
    if (strcmp(aname, bname) == 0) {
      oa = oop_find_obj(vm, aname);
      if (!oa) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        var_set_num(vm, "LAST_N", 0);
        vm->last_n = 0;
        var_set_num(vm, "SWAPOBJ_N", 0);
        var_set_num(vm, "TRYSWAP_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR", "SWAPOBJ: unknown object");
        var_set_str(vm, "ERR", "SWAPOBJ: unknown object");
        bump(vm);
        return 1;
      }
      var_set_str(vm, "LAST", aname);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", aname);
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_num(vm, "SWAPOBJ_N", 1);
      if (soft) var_set_num(vm, "TRYSWAP_N", 1);
      var_set_num(vm, "OK", 1);
      bump(vm);
      return 1;
    }
    oa = oop_find_obj(vm, aname);
    ob = oop_find_obj(vm, bname);
    if (!oa || !ob) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "SWAPOBJ_N", 0);
      var_set_num(vm, "TRYSWAP_N", 0);
      var_set_num(vm, "OK", 0);
      {
        char msg[96];
        if (!oa && !ob)
          snprintf(msg, sizeof msg, "SWAPOBJ: unknown objects %s and %s", aname, bname);
        else if (!oa)
          snprintf(msg, sizeof msg, "SWAPOBJ: unknown object %s", aname);
        else
          snprintf(msg, sizeof msg, "SWAPOBJ: unknown object %s", bname);
        var_set_str(vm, "LAST_ERR", msg);
        var_set_str(vm, "ERR", msg);
      }
      if (vm->trace) fprintf(vm->trace, "# TRYSWAP soft miss %s <-> %s\n", aname, bname);
      bump(vm);
      return 1;
    }
    /* clear dead slots that hold either name (pool uniqueness) */
    for (i = 0; i < vm->n_objs; i++) {
      if (&vm->objs[i] == oa || &vm->objs[i] == ob) continue;
      if (!vm->objs[i].live &&
          (strcmp(vm->objs[i].name, aname) == 0 ||
           strcmp(vm->objs[i].name, bname) == 0))
        vm->objs[i].name[0] = 0;
    }
    /* swap names via temp (no third live slot needed) */
    snprintf(tmpn, sizeof tmpn, "%s", oa->name);
    snprintf(oa->name, sizeof oa->name, "%s", bname);
    snprintf(ob->name, sizeof ob->name, "%s", tmpn);
    /* THIS follows the receiver instance (by pointer), rename both sides */
    if (strcmp(vm->this_obj, aname) == 0) {
      snprintf(vm->this_obj, sizeof vm->this_obj, "%s", bname);
      var_set_str(vm, "THIS", bname);
      var_set_str(vm, "SELF", bname);
    } else if (strcmp(vm->this_obj, bname) == 0) {
      snprintf(vm->this_obj, sizeof vm->this_obj, "%s", aname);
      var_set_str(vm, "THIS", aname);
      var_set_str(vm, "SELF", aname);
    }
    /* rebind class-name vars for both new slot names */
    if (oa->class_idx >= 0 && oa->class_idx < vm->n_classes) {
      cd = &vm->classes[oa->class_idx];
      var_set_str(vm, oa->name, cd->name);
    }
    if (ob->class_idx >= 0 && ob->class_idx < vm->n_classes) {
      cd = &vm->classes[ob->class_idx];
      var_set_str(vm, ob->name, cd->name);
      var_set_str(vm, "CLASS", cd->name);
    }
    var_set_str(vm, "LAST", bname);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bname);
    var_set_str(vm, "OBJECT", bname);
    var_set_str(vm, "SWAPOBJ", bname);
    var_set_str(vm, "SWAP_A", aname);
    var_set_str(vm, "SWAP_B", bname);
    var_set_str(vm, "SWAP_FROM", aname);
    var_set_str(vm, "SWAP_TO", bname);
    var_set_num(vm, "LAST_N", 1);
    vm->last_n = 1;
    var_set_num(vm, "SWAPOBJ_N", 1);
    if (soft) var_set_num(vm, "TRYSWAP_N", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace) fprintf(vm->trace, "# %s %s <-> %s\n", soft ? "TRYSWAP" : "SWAPOBJ", aname, bname);
    bump(vm);
    return 1;
  }


  /* CLASSINFO|DUMPCLASS|DESCRIBECLASS [JSON] Class|obj
   * â one-shot class schema plate for agents (fields + methods + live count).
   * Default: key:value bag (LOOKUP-ready). JSON|ASJSON â cubalc.class.v1.
   * Soft OK=0 if unknown. Usability: no LISTFIELDS+LISTMETHODS+COUNTOBJ glue. */
  if (kw(&L->cur, "CLASSINFO") || kw(&L->cur, "DUMPCLASS") ||
      kw(&L->cur, "DESCRIBECLASS") || kw(&L->cur, "CLASSSCHEMA") ||
      kw(&L->cur, "INSPECTCLASS") || kw(&L->cur, "SCHEMACLASS") ||
      kw(&L->cur, "CLASSMETA") || kw(&L->cur, "TYPEINFO") ||
      kw(&L->cur, "DESCRIBE") || kw(&L->cur, "CLASSDEF")) {
    char a[48], bag[4096], fbag[1024], mbag[1024];
    ClassDef *cd = NULL;
    ObjInst *ob;
    size_t o = 0, fo = 0, mo = 0;
    int i, n_live = 0, as_json = 0;
    /* DESCRIBE alone may be too broad â keep as alias */
    lex_next(L);
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "CLASSINFO [JSON] Class|obj"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(a, sizeof a, "%s", L->cur.text);
      lex_next(L);
    } else {
      if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(a, sizeof a, "%s", vm->last_str);
      else
        snprintf(a, sizeof a, "%s", L->cur.text);
      lex_next(L);
    }
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    ob = oop_find_obj(vm, a);
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes)
      cd = &vm->classes[ob->class_idx];
    else
      cd = oop_find_class(vm, a);
    bag[0] = 0;
    fbag[0] = 0;
    mbag[0] = 0;
    if (!cd) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "NFIELDS", 0);
      var_set_num(vm, "NMETHODS", 0);
      var_set_num(vm, "CLASSINFO_N", 0);
      var_set_num(vm, "CLASSINFO_LIVE", 0);
      var_set_str(vm, "CLASSINFO", "");
      var_set_str(vm, "DUMPCLASS", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "CLASSINFO: unknown class/obj");
      var_set_str(vm, "ERR", "CLASSINFO: unknown class/obj");
      bump(vm);
      return 1;
    }
    for (i = 0; i < vm->n_objs; i++) {
      if (vm->objs[i].live && vm->objs[i].class_idx >= 0 &&
          vm->objs[i].class_idx < vm->n_classes &&
          &vm->classes[vm->objs[i].class_idx] == cd)
        n_live++;
    }
    /* field/method name bags (comma-joined for compact plate) */
    for (i = 0; i < cd->n_fields; i++) {
      size_t ln = strlen(cd->fields[i].name);
      if (i > 0 && fo + 1 < sizeof fbag) fbag[fo++] = ',';
      if (fo + ln < sizeof fbag) {
        memcpy(fbag + fo, cd->fields[i].name, ln);
        fo += ln;
      }
      fbag[fo] = 0;
    }
    for (i = 0; i < cd->n_methods; i++) {
      size_t ln = strlen(cd->methods[i].name);
      if (i > 0 && mo + 1 < sizeof mbag) mbag[mo++] = ',';
      if (mo + ln < sizeof mbag) {
        memcpy(mbag + mo, cd->methods[i].name, ln);
        mo += ln;
      }
      mbag[mo] = 0;
    }
    if (as_json) {
      {
        const char *parent_name = "";
        if (cd->parent_idx >= 0 && cd->parent_idx < vm->n_classes)
          parent_name = vm->classes[cd->parent_idx].name;
        o = (size_t)snprintf(
            bag, sizeof bag,
            "{\"schema\":\"cubalc.class.v1\",\"name\":\"%s\",\"role\":\"%s\","
            "\"parent\":\"%s\",\"n_fields\":%d,\"n_methods\":%d,\"live\":%d,\"fields\":[",
            cd->name, cd->role[0] ? cd->role : "", parent_name,
            cd->n_fields, cd->n_methods, n_live);
      }
      for (i = 0; i < cd->n_fields && o + 8 < sizeof bag; i++) {
        if (i > 0 && o + 1 < sizeof bag) bag[o++] = ',';
        o += (size_t)snprintf(bag + o, sizeof bag - o, "\"%s\"",
                              cd->fields[i].name);
      }
      if (o + 16 < sizeof bag)
        o += (size_t)snprintf(bag + o, sizeof bag - o, "],\"methods\":[");
      for (i = 0; i < cd->n_methods && o + 8 < sizeof bag; i++) {
        if (i > 0 && o + 1 < sizeof bag) bag[o++] = ',';
        o += (size_t)snprintf(bag + o, sizeof bag - o, "\"%s\"",
                              cd->methods[i].name);
      }
      if (o + 3 < sizeof bag) {
        bag[o++] = ']';
        bag[o++] = '}';
        bag[o] = 0;
      }
    } else {
      {
        const char *parent_name = "";
        if (cd->parent_idx >= 0 && cd->parent_idx < vm->n_classes)
          parent_name = vm->classes[cd->parent_idx].name;
        o = (size_t)snprintf(
            bag, sizeof bag,
            "name:%s\nrole:%s\nparent:%s\nn_fields:%d\nn_methods:%d\nlive:%d\n"
            "fields:%s\nmethods:%s",
            cd->name, cd->role[0] ? cd->role : "", parent_name,
            cd->n_fields, cd->n_methods, n_live, fbag, mbag);
        if (o >= sizeof bag) bag[sizeof bag - 1] = 0;
        var_set_str(vm, "BASE", parent_name);
        var_set_str(vm, "PARENTCLASS", parent_name);
        var_set_num(vm, "HASPARENT_N", parent_name[0] ? 1 : 0);
      }
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "CLASSINFO", bag);
    var_set_str(vm, "DUMPCLASS", bag);
    var_set_str(vm, "DESCRIBECLASS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = cd->n_fields;
    var_set_num(vm, "LAST_N", cd->n_fields);
    var_set_num(vm, "NFIELDS", cd->n_fields);
    var_set_num(vm, "NMETHODS", cd->n_methods);
    var_set_num(vm, "CLASSINFO_N", cd->n_fields);
    var_set_num(vm, "CLASSINFO_LIVE", n_live);
    var_set_num(vm, "CLASSINFO_METHODS", cd->n_methods);
    var_set_str(vm, "CLASS", cd->name);
    var_set_str(vm, "FIELDS", fbag);
    var_set_str(vm, "METHODS", mbag);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* METHODINFO|DUMPMETHOD|DESCRIBEMETHOD [JSON] Class|obj method
   * â method arity/params plate for agent SEND prep.
   * Default bag: name/class/n_params/params. JSON â cubalc.method.v1.
   * Soft OK=0 if class or method missing. Complements HASMETHOD + CLASSINFO. */
  if (kw(&L->cur, "METHODINFO") || kw(&L->cur, "DUMPMETHOD") ||
      kw(&L->cur, "DESCRIBEMETHOD") || kw(&L->cur, "METHINFO") ||
      kw(&L->cur, "INSPECTMETHOD") || kw(&L->cur, "METHODMETA") ||
      kw(&L->cur, "ARITY") || kw(&L->cur, "METHODDEF") ||
      kw(&L->cur, "METHODSCHEMA") || kw(&L->cur, "PARAMINFO")) {
    char a[48], mname[48], bag[2048], pbag[512];
    ClassDef *cd = NULL;
    MethodDef *md = NULL;
    ObjInst *ob;
    size_t o = 0, po = 0;
    int i, as_json = 0;
    lex_next(L);
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    if (L->cur.kind != TK_IDENT && L->cur.kind != TK_STR) {
      fail(vm, "METHODINFO [JSON] Class|obj method"); return -1;
    }
    if (L->cur.kind == TK_STR) {
      snprintf(a, sizeof a, "%s", L->cur.text);
      lex_next(L);
    } else {
      if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(a, sizeof a, "%s", vm->last_str);
      else
        snprintf(a, sizeof a, "%s", L->cur.text);
      lex_next(L);
    }
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    /* optional METHOD keyword before name */
    if (kw(&L->cur, "METHOD") || kw(&L->cur, "METH") || kw(&L->cur, "OF"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(mname, sizeof mname, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(mname, sizeof mname, "%s", vm->last_str);
      else
        snprintf(mname, sizeof mname, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "METHODINFO [JSON] Class|obj method"); return -1;
    }
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    ob = oop_find_obj(vm, a);
    if (ob && ob->class_idx >= 0 && ob->class_idx < vm->n_classes)
      cd = &vm->classes[ob->class_idx];
    else
      cd = oop_find_class(vm, a);
    if (cd) md = oop_find_method(cd, mname);
    bag[0] = 0;
    pbag[0] = 0;
    if (!cd || !md) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "NPARAMS", 0);
      var_set_num(vm, "METHODINFO_N", 0);
      var_set_num(vm, "ARITY", 0);
      var_set_str(vm, "METHODINFO", "");
      var_set_str(vm, "PARAMS", "");
      var_set_str(vm, "METHOD", mname);
      if (cd) var_set_str(vm, "CLASS", cd->name);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  !cd ? "METHODINFO: unknown class/obj"
                      : "METHODINFO: unknown method");
      var_set_str(vm, "ERR",
                  !cd ? "METHODINFO: unknown class/obj"
                      : "METHODINFO: unknown method");
      bump(vm);
      return 1;
    }
    for (i = 0; i < md->n_params; i++) {
      size_t ln = strlen(md->params[i]);
      if (i > 0 && po + 1 < sizeof pbag) pbag[po++] = ',';
      if (po + ln < sizeof pbag) {
        memcpy(pbag + po, md->params[i], ln);
        po += ln;
      }
      pbag[po] = 0;
    }
    if (as_json) {
      o = (size_t)snprintf(
          bag, sizeof bag,
          "{\"schema\":\"cubalc.method.v1\",\"class\":\"%s\",\"name\":\"%s\","
          "\"n_params\":%d,\"params\":[",
          cd->name, md->name, md->n_params);
      for (i = 0; i < md->n_params && o + 8 < sizeof bag; i++) {
        if (i > 0 && o + 1 < sizeof bag) bag[o++] = ',';
        o += (size_t)snprintf(bag + o, sizeof bag - o, "\"%s\"", md->params[i]);
      }
      if (o + 3 < sizeof bag) {
        bag[o++] = ']';
        bag[o++] = '}';
        bag[o] = 0;
      }
    } else {
      o = (size_t)snprintf(
          bag, sizeof bag,
          "name:%s\nclass:%s\nn_params:%d\nparams:%s",
          md->name, cd->name, md->n_params, pbag);
      if (o >= sizeof bag) bag[sizeof bag - 1] = 0;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "METHODINFO", bag);
    var_set_str(vm, "DUMPMETHOD", bag);
    var_set_str(vm, "DESCRIBEMETHOD", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = md->n_params;
    var_set_num(vm, "LAST_N", md->n_params);
    var_set_num(vm, "NPARAMS", md->n_params);
    var_set_num(vm, "METHODINFO_N", md->n_params);
    var_set_num(vm, "ARITY", md->n_params);
    var_set_str(vm, "METHOD", md->name);
    var_set_str(vm, "CLASS", cd->name);
    var_set_str(vm, "PARAMS", pbag);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* FNINFO|DUMPFN|DESCRIBEFN [JSON] name
   * â FN arity/params plate for agent CALL prep (twin of METHODINFO for SEND).
   * Default bag: name/n_params/params. JSON â cubalc.fn.v1.
   * Soft OK=0 if FN missing. Sets NPARAMS/ARITY/FNINFO_N, PARAMS, FN. */
  if (kw(&L->cur, "FNINFO") || kw(&L->cur, "DUMPFN") ||
      kw(&L->cur, "DESCRIBEFN") || kw(&L->cur, "FUNCINFO") ||
      kw(&L->cur, "INSPECTFN") || kw(&L->cur, "FNMETA") ||
      kw(&L->cur, "FNDEF") || kw(&L->cur, "FNSCHEMA") ||
      kw(&L->cur, "FUNCTIONINFO") || kw(&L->cur, "FNARITY") ||
      kw(&L->cur, "DESCRIBEFUNC") || kw(&L->cur, "DUMPFUNC")) {
    char fname[48], bag[2048], pbag[512];
    FnDef *fn = NULL;
    size_t o = 0, po = 0;
    int i, as_json = 0;
    lex_next(L);
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    /* optional FN|FUNC keyword before name (not NAME â conflicts with var id) */
    if (kw(&L->cur, "FN") || kw(&L->cur, "FUNC") || kw(&L->cur, "FUNCTION") ||
        kw(&L->cur, "OF"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(fname, sizeof fname, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(fname, sizeof fname, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(fname, sizeof fname, "%s", vm->last_str);
      else
        snprintf(fname, sizeof fname, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "FNINFO [JSON] name"); return -1;
    }
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    for (i = 0; i < vm->n_fns; i++) {
      if (strcmp(vm->fns[i].name, fname) == 0) {
        fn = &vm->fns[i];
        break;
      }
    }
    bag[0] = 0;
    pbag[0] = 0;
    if (!fn) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "NPARAMS", 0);
      var_set_num(vm, "FNINFO_N", 0);
      var_set_num(vm, "ARITY", 0);
      var_set_str(vm, "FNINFO", "");
      var_set_str(vm, "PARAMS", "");
      var_set_str(vm, "FN", fname);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "FNINFO: unknown FN");
      var_set_str(vm, "ERR", "FNINFO: unknown FN");
      bump(vm);
      return 1;
    }
    for (i = 0; i < fn->n_params; i++) {
      size_t ln = strlen(fn->params[i]);
      if (i > 0 && po + 1 < sizeof pbag) pbag[po++] = ',';
      if (po + ln < sizeof pbag) {
        memcpy(pbag + po, fn->params[i], ln);
        po += ln;
      }
      pbag[po] = 0;
    }
    if (as_json) {
      o = (size_t)snprintf(
          bag, sizeof bag,
          "{\"schema\":\"cubalc.fn.v1\",\"name\":\"%s\","
          "\"n_params\":%d,\"params\":[",
          fn->name, fn->n_params);
      for (i = 0; i < fn->n_params && o + 8 < sizeof bag; i++) {
        if (i > 0 && o + 1 < sizeof bag) bag[o++] = ',';
        o += (size_t)snprintf(bag + o, sizeof bag - o, "\"%s\"", fn->params[i]);
      }
      if (o + 3 < sizeof bag) {
        bag[o++] = ']';
        bag[o++] = '}';
        bag[o] = 0;
      }
    } else {
      o = (size_t)snprintf(
          bag, sizeof bag,
          "name:%s\nn_params:%d\nparams:%s",
          fn->name, fn->n_params, pbag);
      if (o >= sizeof bag) bag[sizeof bag - 1] = 0;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "FNINFO", bag);
    var_set_str(vm, "DUMPFN", bag);
    var_set_str(vm, "DESCRIBEFN", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = fn->n_params;
    var_set_num(vm, "LAST_N", fn->n_params);
    var_set_num(vm, "NPARAMS", fn->n_params);
    var_set_num(vm, "FNINFO_N", fn->n_params);
    var_set_num(vm, "ARITY", fn->n_params);
    var_set_str(vm, "FN", fn->name);
    var_set_str(vm, "PARAMS", pbag);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LISTFNS|FUNCTIONS â newline bag of defined FN names for agent discovery.
   * LAST_N / NFNS = count. Complements FNINFO (per-FN arity) and LISTMETHODS. */
  if (kw(&L->cur, "LISTFNS") || kw(&L->cur, "FUNCTIONS") ||
      kw(&L->cur, "LISTFUNCS") || kw(&L->cur, "FUNCS") ||
      kw(&L->cur, "FNLIST") || kw(&L->cur, "LISTFUNCTIONS") ||
      kw(&L->cur, "FNS") || kw(&L->cur, "DEFS")) {
    char bag[4096];
    size_t o = 0;
    int i, n = 0;
    lex_next(L);
    bag[0] = 0;
    for (i = 0; i < vm->n_fns; i++) {
      size_t ln = strlen(vm->fns[i].name);
      if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, vm->fns[i].name, ln);
        o += ln;
      }
      bag[o] = 0;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LISTFNS", bag);
    var_set_str(vm, "FUNCTIONS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "NFNS", n);
    var_set_num(vm, "LISTFNS_N", n);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASFN|HASFUNC name â soft 0|1 probe before CALL.
   * Name may be IDENT, "string", or string-var. Usability: IF without fatal. */
  if (kw(&L->cur, "HASFN") || kw(&L->cur, "HASFUNC") ||
      kw(&L->cur, "HASFUNCTION") || kw(&L->cur, "FN?") ||
      kw(&L->cur, "FUNC?") || kw(&L->cur, "FNEXISTS") ||
      kw(&L->cur, "FUNCEXISTS") || kw(&L->cur, "DEFINEDFN") ||
      kw(&L->cur, "ISFN") || kw(&L->cur, "CANCALLFN")) {
    char fname[48];
    int i, hit = 0;
    lex_next(L);
    /* optional FN|FUNC keyword before name (not NAME â conflicts with var id) */
    if (kw(&L->cur, "FN") || kw(&L->cur, "FUNC") || kw(&L->cur, "FUNCTION") ||
        kw(&L->cur, "OF"))
      lex_next(L);
    if (L->cur.kind == TK_STR) {
      snprintf(fname, sizeof fname, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(fname, sizeof fname, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(fname, sizeof fname, "%s", vm->last_str);
      else
        snprintf(fname, sizeof fname, "%s", L->cur.text);
      lex_next(L);
    } else {
      fail(vm, "HASFN name"); return -1;
    }
    for (i = 0; i < vm->n_fns; i++) {
      if (strcmp(vm->fns[i].name, fname) == 0) {
        hit = 1;
        break;
      }
    }
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    {
      char nb[8];
      snprintf(nb, sizeof nb, "%d", hit);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "HASFN_N", hit);
    var_set_str(vm, "FN", fname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }
  /* HASFNS|NEEDFNS nameâ¦ â multi FN capability gate (all-or-FNMISS).
   * Soft: LAST_N 0|1 Â· FNMISS bag Â· NEEDFNS fail-fast first miss.
   * Usability: agent CALL contracts without NÃ HASFN glue Â· bag/CSV/idents.
   * Twin of HASOBJS for FN plane Â· dual of single HASFN. */
  if (kw(&L->cur, "HASFNS") || kw(&L->cur, "HAS_FNS") ||
      kw(&L->cur, "ALLFNS") || kw(&L->cur, "FNSOK") ||
      kw(&L->cur, "HASFUNCS") || kw(&L->cur, "ALLFUNCS") ||
      kw(&L->cur, "NEEDFNS") || kw(&L->cur, "NEED_FNS") ||
      kw(&L->cur, "REQUIREFNS") || kw(&L->cur, "MUSTFNS") ||
      kw(&L->cur, "NEEDFUNCS") || kw(&L->cur, "REQUIREFUNCS")) {
    int hard = kw(&L->cur, "NEEDFNS") || kw(&L->cur, "NEED_FNS") ||
               kw(&L->cur, "REQUIREFNS") || kw(&L->cur, "MUSTFNS") ||
               kw(&L->cur, "NEEDFUNCS") || kw(&L->cur, "REQUIREFUNCS");
    char names[32][48];
    char miss[1024];
    int nname = 0, nmiss = 0, i, j, aln = L->cur.line;
    size_t mo = 0;
    lex_next(L);
    miss[0] = 0;
    if (kw(&L->cur, "FN") || kw(&L->cur, "FUNC") || kw(&L->cur, "FUNCTION") ||
        kw(&L->cur, "OF"))
      lex_next(L);
    while (nname < 32 &&
           (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT)) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur, "ASSERT") || kw(&L->cur, "LET") || kw(&L->cur, "PRINT") ||
           kw(&L->cur, "REQUIRE") || kw(&L->cur, "END") ||
           kw(&L->cur, "IF") || kw(&L->cur, "ELSE") || kw(&L->cur, "ELIF") ||
           kw(&L->cur, "PASS") || kw(&L->cur, "FAIL") || kw(&L->cur, "INCLUDE") ||
           kw(&L->cur, "SYS") || kw(&L->cur, "HELP") || kw(&L->cur, "CLEAR_ERR") ||
           kw(&L->cur, "NOTE") || kw(&L->cur, "EXIT") || kw(&L->cur, "DEFAULT") ||
           kw(&L->cur, "VERSION") || kw(&L->cur, "STATUS") || kw(&L->cur, "FOR") ||
           kw(&L->cur, "WHILE") || kw(&L->cur, "LOOP") || kw(&L->cur, "EACH") ||
           kw(&L->cur, "CUBE") || kw(&L->cur, "PLUG") || kw(&L->cur, "HOLD_FLASH") ||
           kw(&L->cur, "CLASS") || kw(&L->cur, "NEW") || kw(&L->cur, "FN") ||
           kw(&L->cur, "HASFN") || kw(&L->cur, "HASFNS") || kw(&L->cur, "NEEDFNS") ||
           kw(&L->cur, "HASFNANY") || kw(&L->cur, "NEEDFNANY") ||
           kw(&L->cur, "LISTFNS") || kw(&L->cur, "CALL") || kw(&L->cur, "FNINFO") ||
           kw(&L->cur, "SEND") || kw(&L->cur, "GETF") || kw(&L->cur, "SETF")))
        break;
      if (L->cur.kind == TK_STR) {
        const char *s = L->cur.text;
        if (strchr(s, '\n') || strchr(s, ',') || strchr(s, ' ')) {
          const char *p = s;
          while (*p && nname < 32) {
            char tok[48];
            size_t tl = 0;
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' || *p == ':')
              p++;
            if (!*p) break;
            while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                   p[tl] != '\t' && p[tl] != ':' && tl + 1 < sizeof tok) {
              tok[tl] = p[tl];
              tl++;
            }
            tok[tl] = 0;
            p += tl;
            if (tok[0])
              snprintf(names[nname++], sizeof names[0], "%s", tok);
          }
          lex_next(L);
          continue;
        }
        snprintf(names[nname++], sizeof names[0], "%s", s);
        lex_next(L);
      } else {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (strchr(vv->sval, '\n') || strchr(vv->sval, ',') ||
             strchr(vv->sval, ' '))) {
          const char *p = vv->sval;
          while (*p && nname < 32) {
            char tok[48];
            size_t tl = 0;
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' || *p == ':')
              p++;
            if (!*p) break;
            while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                   p[tl] != '\t' && p[tl] != ':' && tl + 1 < sizeof tok) {
              tok[tl] = p[tl];
              tl++;
            }
            tok[tl] = 0;
            p += tl;
            if (tok[0])
              snprintf(names[nname++], sizeof names[0], "%s", tok);
          }
          lex_next(L);
        } else if (vv && vv->is_str && vv->sval[0]) {
          int already = 0;
          for (j = 0; j < vm->n_fns; j++) {
            if (strcmp(vm->fns[j].name, L->cur.text) == 0) {
              already = 1;
              break;
            }
          }
          if (!already) {
            snprintf(names[nname++], sizeof names[0], "%s", vv->sval);
            lex_next(L);
          } else {
            snprintf(names[nname++], sizeof names[0], "%s", L->cur.text);
            lex_next(L);
          }
        } else if (strcmp(L->cur.text, "LAST") == 0) {
          snprintf(names[nname++], sizeof names[0], "%s", vm->last_str);
          lex_next(L);
        } else {
          snprintf(names[nname++], sizeof names[0], "%s", L->cur.text);
          lex_next(L);
        }
      }
    }
    if (nname == 0) {
      if (hard) {
        fail_at(vm, L, "NEEDFNS nameâ¦ â NEEDFNS add greet Â· HASFNS bag");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASFNS_N", 0);
      var_set_num(vm, "FNMISS_N", 0);
      var_set_str(vm, "FNMISS", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASFNS: need names â HASFNS add greet");
      var_set_str(vm, "ERR", "HASFNS: need names â HASFNS add greet");
      bump(vm);
      return 1;
    }
    for (i = 0; i < nname; i++) {
      int hit = 0;
      for (j = 0; j < vm->n_fns; j++) {
        if (strcmp(vm->fns[j].name, names[i]) == 0) {
          hit = 1;
          break;
        }
      }
      if (hit)
        continue;
      nmiss++;
      if (mo && mo + 1 < sizeof miss) miss[mo++] = '\n';
      {
        size_t ln = strlen(names[i]);
        if (mo + ln < sizeof miss) {
          memcpy(miss + mo, names[i], ln);
          mo += ln;
          miss[mo] = 0;
        }
      }
      if (hard) {
        char em[220];
        snprintf(em, sizeof em,
                 "NEEDFNS miss line %d: '%s' (+%d more?) â DEFINE/INCLUDE Â· HASFN Â· LISTFNS",
                 aln, names[i], nname - i - 1);
        fail_at(vm, L, em);
        return -1;
      }
    }
    var_set_str(vm, "FNMISS", miss);
    var_set_num(vm, "FNMISS_N", nmiss);
    var_set_num(vm, "HASFNS_N", nname - nmiss);
    var_set_num(vm, "NEEDFNS_N", nname);
    var_set_num(vm, "FNS_N", nname);
    if (nmiss == 0) {
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST", "1");
      snprintf(vm->last_str, sizeof vm->last_str, "1");
      var_set_num(vm, "OK", 1);
    } else {
      char em[220];
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      snprintf(em, sizeof em,
               "HASFNS miss (%d/%d): %s â DEFINE/INCLUDE Â· NEEDFNS Â· LISTFNS",
               nmiss, nname, miss);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%d miss=%d\n",
              hard ? "NEEDFNS" : "HASFNS", nname, nmiss);
    bump(vm);
    return 1;
  }

  /* HASFNANY|NEEDFNANY nameâ¦ â multi FN any-of gate (HASFNS twin).
   * Soft: LAST_N 1 if any listed FN defined Â· FNHAVE bag Â· FNMISS bag.
   * NEEDFNANY fail-fast if none of the listed FNs exist.
   * Usability: optional CALL plugin FN OR without NÃ HASFN IF glue. */
  if (kw(&L->cur, "HASFNANY") || kw(&L->cur, "HAS_FN_ANY") ||
      kw(&L->cur, "ANYFN") || kw(&L->cur, "FNANY") ||
      kw(&L->cur, "HASANYFN") || kw(&L->cur, "ANYFUNC") ||
      kw(&L->cur, "NEEDFNANY") || kw(&L->cur, "NEED_FN_ANY") ||
      kw(&L->cur, "REQUIREFNANY") || kw(&L->cur, "MUSTFNANY") ||
      kw(&L->cur, "NEEDFUNCANY") || kw(&L->cur, "REQUIREFUNCANY")) {
    int hard = kw(&L->cur, "NEEDFNANY") || kw(&L->cur, "NEED_FN_ANY") ||
               kw(&L->cur, "REQUIREFNANY") || kw(&L->cur, "MUSTFNANY") ||
               kw(&L->cur, "NEEDFUNCANY") || kw(&L->cur, "REQUIREFUNCANY");
    char names[32][48];
    char miss[1024], havebag[1024];
    int nname = 0, nmiss = 0, nhave = 0, i, j, aln = L->cur.line;
    size_t mo = 0, ho = 0;
    lex_next(L);
    miss[0] = 0;
    havebag[0] = 0;
    if (kw(&L->cur, "FN") || kw(&L->cur, "FUNC") || kw(&L->cur, "FUNCTION") ||
        kw(&L->cur, "OF"))
      lex_next(L);
    while (nname < 32 &&
           (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT)) {
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur, "ASSERT") || kw(&L->cur, "LET") || kw(&L->cur, "PRINT") ||
           kw(&L->cur, "REQUIRE") || kw(&L->cur, "END") ||
           kw(&L->cur, "IF") || kw(&L->cur, "ELSE") || kw(&L->cur, "ELIF") ||
           kw(&L->cur, "PASS") || kw(&L->cur, "FAIL") || kw(&L->cur, "INCLUDE") ||
           kw(&L->cur, "SYS") || kw(&L->cur, "HELP") || kw(&L->cur, "CLEAR_ERR") ||
           kw(&L->cur, "NOTE") || kw(&L->cur, "EXIT") || kw(&L->cur, "DEFAULT") ||
           kw(&L->cur, "VERSION") || kw(&L->cur, "STATUS") || kw(&L->cur, "FOR") ||
           kw(&L->cur, "WHILE") || kw(&L->cur, "LOOP") || kw(&L->cur, "EACH") ||
           kw(&L->cur, "CUBE") || kw(&L->cur, "PLUG") || kw(&L->cur, "HOLD_FLASH") ||
           kw(&L->cur, "CLASS") || kw(&L->cur, "NEW") || kw(&L->cur, "FN") ||
           kw(&L->cur, "HASFN") || kw(&L->cur, "HASFNS") || kw(&L->cur, "NEEDFNS") ||
           kw(&L->cur, "HASFNANY") || kw(&L->cur, "NEEDFNANY") ||
           kw(&L->cur, "LISTFNS") || kw(&L->cur, "CALL") || kw(&L->cur, "FNINFO") ||
           kw(&L->cur, "SEND") || kw(&L->cur, "GETF") || kw(&L->cur, "SETF")))
        break;
      if (L->cur.kind == TK_STR) {
        const char *s = L->cur.text;
        if (strchr(s, '\n') || strchr(s, ',') || strchr(s, ' ')) {
          const char *p = s;
          while (*p && nname < 32) {
            char tok[48];
            size_t tl = 0;
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' || *p == ':')
              p++;
            if (!*p) break;
            while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                   p[tl] != '\t' && p[tl] != ':' && tl + 1 < sizeof tok) {
              tok[tl] = p[tl];
              tl++;
            }
            tok[tl] = 0;
            p += tl;
            if (tok[0])
              snprintf(names[nname++], sizeof names[0], "%s", tok);
          }
          lex_next(L);
          continue;
        }
        snprintf(names[nname++], sizeof names[0], "%s", s);
        lex_next(L);
      } else {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0] &&
            (strchr(vv->sval, '\n') || strchr(vv->sval, ',') ||
             strchr(vv->sval, ' '))) {
          const char *p = vv->sval;
          while (*p && nname < 32) {
            char tok[48];
            size_t tl = 0;
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',' || *p == ':')
              p++;
            if (!*p) break;
            while (p[tl] && p[tl] != '\n' && p[tl] != ',' && p[tl] != ' ' &&
                   p[tl] != '\t' && p[tl] != ':' && tl + 1 < sizeof tok) {
              tok[tl] = p[tl];
              tl++;
            }
            tok[tl] = 0;
            p += tl;
            if (tok[0])
              snprintf(names[nname++], sizeof names[0], "%s", tok);
          }
          lex_next(L);
        } else if (vv && vv->is_str && vv->sval[0]) {
          int already = 0;
          for (j = 0; j < vm->n_fns; j++) {
            if (strcmp(vm->fns[j].name, L->cur.text) == 0) {
              already = 1;
              break;
            }
          }
          if (!already) {
            snprintf(names[nname++], sizeof names[0], "%s", vv->sval);
            lex_next(L);
          } else {
            snprintf(names[nname++], sizeof names[0], "%s", L->cur.text);
            lex_next(L);
          }
        } else if (strcmp(L->cur.text, "LAST") == 0) {
          snprintf(names[nname++], sizeof names[0], "%s", vm->last_str);
          lex_next(L);
        } else {
          snprintf(names[nname++], sizeof names[0], "%s", L->cur.text);
          lex_next(L);
        }
      }
    }
    if (nname == 0) {
      if (hard) {
        fail_at(vm, L, "NEEDFNANY nameâ¦ â NEEDFNANY add ghost Â· HASFNANY bag");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASFNANY_N", 0);
      var_set_num(vm, "FNMISS_N", 0);
      var_set_num(vm, "FNHAVE_N", 0);
      var_set_str(vm, "FNMISS", "");
      var_set_str(vm, "FNHAVE", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASFNANY: need names â HASFNANY add ghost");
      var_set_str(vm, "ERR", "HASFNANY: need names â HASFNANY add ghost");
      bump(vm);
      return 1;
    }
    for (i = 0; i < nname; i++) {
      int hit = 0;
      for (j = 0; j < vm->n_fns; j++) {
        if (strcmp(vm->fns[j].name, names[i]) == 0) {
          hit = 1;
          break;
        }
      }
      if (hit) {
        nhave++;
        if (ho && ho + 1 < sizeof havebag) havebag[ho++] = '\n';
        {
          size_t ln = strlen(names[i]);
          if (ho + ln < sizeof havebag) {
            memcpy(havebag + ho, names[i], ln);
            ho += ln;
            havebag[ho] = 0;
          }
        }
      } else {
        nmiss++;
        if (mo && mo + 1 < sizeof miss) miss[mo++] = '\n';
        {
          size_t ln = strlen(names[i]);
          if (mo + ln < sizeof miss) {
            memcpy(miss + mo, names[i], ln);
            mo += ln;
            miss[mo] = 0;
          }
        }
      }
    }
    var_set_str(vm, "FNMISS", miss);
    var_set_str(vm, "FNHAVE", havebag);
    var_set_num(vm, "FNMISS_N", nmiss);
    var_set_num(vm, "FNHAVE_N", nhave);
    var_set_num(vm, "HASFNANY_N", nhave);
    var_set_num(vm, "NEEDFNANY_N", nname);
    var_set_num(vm, "FNS_ANY_N", nname);
    if (nhave > 0) {
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST", "1");
      snprintf(vm->last_str, sizeof vm->last_str, "1");
      var_set_num(vm, "OK", 1);
    } else if (hard) {
      char em[240];
      snprintf(em, sizeof em,
               "NEEDFNANY miss line %d: need one of [%s] â DEFINE/INCLUDE Â· HASFNANY Â· LISTFNS",
               aln, miss[0] ? miss : "?");
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      fail_at(vm, L, em);
      return -1;
    } else {
      char em[240];
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      snprintf(em, sizeof em,
               "HASFNANY miss (0/%d): need one of %s â NEEDFNANY Â· DEFINE",
               nname, miss[0] ? miss : "?");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    if (vm->trace)
      fprintf(vm->trace, "# %s n=%d have=%d miss=%d\n",
              hard ? "NEEDFNANY" : "HASFNANY", nname, nhave, nmiss);
    bump(vm);
    return 1;
  }


  /* DUMPOBJ|INSPECT|OBJDUMP|DUMPF obj â field:value bag of live object state.
   * Complements LISTFIELDS (names only): one-shot snapshot for agents without
   * EACH+GETF. LAST = newline bag field:val (LOOKUP/KVGET ready); LAST_N =
   * NFIELDS count. Soft OK=0 if missing. Optional JSON flag â cubalc.obj.v1. */
  if (kw(&L->cur, "DUMPOBJ") || kw(&L->cur, "INSPECT") ||
      kw(&L->cur, "OBJDUMP") || kw(&L->cur, "DUMPF") ||
      kw(&L->cur, "INSPECTOBJ") || kw(&L->cur, "OBJINSPECT") ||
      kw(&L->cur, "SHOWOBJ") || kw(&L->cur, "DUMPFIELDS")) {
    char oname[48], bag[4096];
    ObjInst *ob;
    ClassDef *cd;
    size_t o = 0;
    int i, n = 0, as_json = 0;
    lex_next(L);
    /* optional JSON|ASJSON before or after name */
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    if (L->cur.kind == TK_STR) {
      snprintf(oname, sizeof oname, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      if (strcmp(L->cur.text, "LAST") == 0) {
        snprintf(oname, sizeof oname, "%s", vm->last_str);
        lex_next(L);
      } else if (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
                 kw(&L->cur, "TOJSON") || kw(&L->cur, "J")) {
        as_json = 1;
        lex_next(L);
        if (L->cur.kind == TK_STR) {
          snprintf(oname, sizeof oname, "%s", L->cur.text);
          lex_next(L);
        } else if (L->cur.kind == TK_IDENT) {
          if (strcmp(L->cur.text, "LAST") == 0)
            snprintf(oname, sizeof oname, "%s", vm->last_str);
          else
            snprintf(oname, sizeof oname, "%s", L->cur.text);
          lex_next(L);
        } else {
          fail(vm, "DUMPOBJ name"); return -1;
        }
      } else {
        snprintf(oname, sizeof oname, "%s", L->cur.text);
        lex_next(L);
      }
    } else {
      fail(vm, "DUMPOBJ name"); return -1;
    }
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "NFIELDS", 0);
      var_set_num(vm, "DUMPOBJ_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "DUMPOBJ: unknown object");
      var_set_str(vm, "ERR", "DUMPOBJ: unknown object");
      bump(vm);
      return 1;
    }
    if (ob->class_idx < 0 || ob->class_idx >= vm->n_classes) {
      fail(vm, "DUMPOBJ bad class"); return -1;
    }
    cd = &vm->classes[ob->class_idx];
    bag[0] = 0;
    if (as_json) {
      /* compact cubalc.obj.v1 â fields as object; numbers bare, strings quoted */
      o = (size_t)snprintf(bag, sizeof bag,
                           "{\"schema\":\"cubalc.obj.v1\",\"name\":\"%s\","
                           "\"class\":\"%s\",\"n\":%d,\"fields\":{",
                           oname, cd->name, cd->n_fields);
      for (i = 0; i < cd->n_fields && o + 8 < sizeof bag; i++) {
        FieldDef *fd = &cd->fields[i];
        char vb[160];
        size_t need;
        if (i > 0 && o + 1 < sizeof bag) bag[o++] = ',';
        if (ob->fis_str[i]) {
          /* escape " \ and control minimally for plate safety */
          size_t vi, vo = 0;
          vb[vo++] = '"';
          for (vi = 0; ob->fstr[i][vi] && vo + 3 < sizeof vb; vi++) {
            char c = ob->fstr[i][vi];
            if (c == '"' || c == '\\') { vb[vo++] = '\\'; vb[vo++] = c; }
            else if ((unsigned char)c < 0x20) { /* skip controls */ }
            else vb[vo++] = c;
          }
          vb[vo++] = '"';
          vb[vo] = 0;
        } else {
          snprintf(vb, sizeof vb, "%ld", ob->fnum[i]);
        }
        need = strlen(fd->name) + 3 + strlen(vb);
        if (o + need >= sizeof bag) break;
        o += (size_t)snprintf(bag + o, sizeof bag - o, "\"%s\":%s", fd->name,
                              vb);
        n++;
      }
      if (o + 3 < sizeof bag) {
        bag[o++] = '}';
        bag[o++] = '}';
        bag[o] = 0;
      }
    } else {
      /* field:value bag â LOOKUP/KVGET/TOPKEY ready */
      for (i = 0; i < cd->n_fields; i++) {
        FieldDef *fd = &cd->fields[i];
        char line[192];
        size_t ln;
        if (ob->fis_str[i])
          snprintf(line, sizeof line, "%s:%s", fd->name, ob->fstr[i]);
        else
          snprintf(line, sizeof line, "%s:%ld", fd->name, ob->fnum[i]);
        ln = strlen(line);
        if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + ln < sizeof bag) {
          memcpy(bag + o, line, ln);
          o += ln;
        }
        bag[o] = 0;
        n++;
      }
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "DUMPOBJ", bag);
    var_set_str(vm, "INSPECT", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "NFIELDS", n);
    var_set_num(vm, "DUMPOBJ_N", n);
    var_set_str(vm, "OBJECT", oname);
    var_set_str(vm, "CLASS", cd->name);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* OBJINFO|DESCRIBEOBJ|OBJMETA [JSON] obj
   * â live object plate: name/class/methods + field values in one shot.
   * Complements DUMPOBJ (fields only) + CLASSINFO/LISTMETHODS (schema only).
   * Soft OK=0 if missing. JSON â cubalc.objinfo.v1. */
  if (kw(&L->cur, "OBJINFO") || kw(&L->cur, "DESCRIBEOBJ") ||
      kw(&L->cur, "OBJMETA") || kw(&L->cur, "OBJSCHEMA") ||
      kw(&L->cur, "OBJECTINFO") || kw(&L->cur, "INFOOBJ") ||
      kw(&L->cur, "SUMMARYOBJ") || kw(&L->cur, "OBJSUMMARY") ||
      kw(&L->cur, "SHOWOBJINFO") || kw(&L->cur, "EXPLAINOBJ")) {
    char oname[48], bag[4096], mbag[1024];
    ObjInst *ob;
    ClassDef *cd;
    size_t o = 0, mo = 0;
    int i, n = 0, as_json = 0;
    lex_next(L);
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    /* Use oop_resolve_obj_name: NEW sets var(name)=Class â do not expand that. */
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "OBJINFO [JSON] name"); return -1;
    }
    if (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "JSON") || kw(&L->cur, "ASJSON") ||
         kw(&L->cur, "TOJSON") || kw(&L->cur, "J"))) {
      as_json = 1;
      lex_next(L);
    }
    ob = oop_find_obj(vm, oname);
    if (!ob || ob->class_idx < 0 || ob->class_idx >= vm->n_classes) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "NFIELDS", 0);
      var_set_num(vm, "NMETHODS", 0);
      var_set_num(vm, "OBJINFO_N", 0);
      var_set_str(vm, "OBJINFO", "");
      var_set_str(vm, "METHODS", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "OBJINFO: unknown object");
      var_set_str(vm, "ERR", "OBJINFO: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    bag[0] = 0;
    mbag[0] = 0;
    for (i = 0; i < cd->n_methods; i++) {
      size_t ln = strlen(cd->methods[i].name);
      if (i > 0 && mo + 1 < sizeof mbag) mbag[mo++] = ',';
      if (mo + ln < sizeof mbag) {
        memcpy(mbag + mo, cd->methods[i].name, ln);
        mo += ln;
      }
      mbag[mo] = 0;
    }
    if (as_json) {
      o = (size_t)snprintf(
          bag, sizeof bag,
          "{\"schema\":\"cubalc.objinfo.v1\",\"name\":\"%s\",\"class\":\"%s\","
          "\"n_fields\":%d,\"n_methods\":%d,\"methods\":[",
          oname, cd->name, cd->n_fields, cd->n_methods);
      for (i = 0; i < cd->n_methods && o + 8 < sizeof bag; i++) {
        if (i > 0 && o + 1 < sizeof bag) bag[o++] = ',';
        o += (size_t)snprintf(bag + o, sizeof bag - o, "\"%s\"",
                              cd->methods[i].name);
      }
      if (o + 16 < sizeof bag)
        o += (size_t)snprintf(bag + o, sizeof bag - o, "],\"fields\":{");
      for (i = 0; i < cd->n_fields && o + 8 < sizeof bag; i++) {
        FieldDef *fd = &cd->fields[i];
        char vb[160];
        size_t need;
        if (i > 0 && o + 1 < sizeof bag) bag[o++] = ',';
        if (ob->fis_str[i]) {
          size_t vi, vo = 0;
          vb[vo++] = '"';
          for (vi = 0; ob->fstr[i][vi] && vo + 3 < sizeof vb; vi++) {
            char c = ob->fstr[i][vi];
            if (c == '"' || c == '\\') { vb[vo++] = '\\'; vb[vo++] = c; }
            else if ((unsigned char)c < 0x20) { /* skip */ }
            else vb[vo++] = c;
          }
          vb[vo++] = '"';
          vb[vo] = 0;
        } else {
          snprintf(vb, sizeof vb, "%ld", ob->fnum[i]);
        }
        need = strlen(fd->name) + 3 + strlen(vb);
        if (o + need >= sizeof bag) break;
        o += (size_t)snprintf(bag + o, sizeof bag - o, "\"%s\":%s", fd->name,
                              vb);
        n++;
      }
      if (o + 3 < sizeof bag) {
        bag[o++] = '}';
        bag[o++] = '}';
        bag[o] = 0;
      }
    } else {
      o = (size_t)snprintf(
          bag, sizeof bag,
          "name:%s\nclass:%s\nn_fields:%d\nn_methods:%d\nmethods:%s",
          oname, cd->name, cd->n_fields, cd->n_methods, mbag);
      if (o >= sizeof bag) o = sizeof bag - 1;
      for (i = 0; i < cd->n_fields; i++) {
        FieldDef *fd = &cd->fields[i];
        char line[192];
        size_t ln;
        if (ob->fis_str[i])
          snprintf(line, sizeof line, "\n%s:%s", fd->name, ob->fstr[i]);
        else
          snprintf(line, sizeof line, "\n%s:%ld", fd->name, ob->fnum[i]);
        ln = strlen(line);
        if (o + ln < sizeof bag) {
          memcpy(bag + o, line, ln);
          o += ln;
          bag[o] = 0;
        }
        n++;
      }
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "OBJINFO", bag);
    var_set_str(vm, "DESCRIBEOBJ", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "NFIELDS", cd->n_fields);
    var_set_num(vm, "NMETHODS", cd->n_methods);
    var_set_num(vm, "OBJINFO_N", n);
    var_set_str(vm, "OBJECT", oname);
    var_set_str(vm, "CLASS", cd->name);
    var_set_str(vm, "METHODS", mbag);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* plane ops_flow */
  return 0;
  return 0;
}
