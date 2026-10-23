/* flow fstr peel from flow_class */
#include "lang/cubalc_lang_ops_flow_fstr_a.h"
#include "lang/cubalc_lang_ops_flow_fstr_oopmeta.h"
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
  /* OOP meta METHODORIGIN..LEAVES → lang_ops_flow_fstr_oopmeta.c */
  {
    int om = cubalc_lang_ops_flow_try_fstr_oopmeta(vm, L);
    if (om != 0) return om;
  }

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



  /* CHILDCOUNT + DESCENDANTCOUNT + SUBTREESIZE (multifile EXTEND)
   * CHILDCOUNT Class|obj — direct children count (EXTEND kids only).
   * DESCENDANTCOUNT Class|obj — proper descendants count (excludes self).
   * SUBTREESIZE Class|obj — 1 + descendants (self included).
   * Complements HASCHILDREN + DESCENDANTS bag + LEAFCOUNT. Cube is SoT. Free energy must flow. */
  return 0;
}
