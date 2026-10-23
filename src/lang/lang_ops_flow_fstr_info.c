/* CubalC lang — lang_ops_flow_fstr_info.c
 * FLOW fstr inspect: CLASSINFO · METHODINFO · FNINFO · LISTFNS
 * Peel from fstr_b (B_fstr_b_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_flow_fstr_info.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_fstr_info(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
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
  return 0;
}
