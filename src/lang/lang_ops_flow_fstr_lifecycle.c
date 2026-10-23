/* CubalC lang — lang_ops_flow_fstr_lifecycle.c
 * FLOW fstr lifecycle: TRYDELETE · DELETEALL · ENSURECLONE · TRYRENAME · TRYSWAP
 * Peel from fstr_b (B_fstr_b_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_flow_fstr_lifecycle.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_fstr_lifecycle(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
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
  return 0;
}
