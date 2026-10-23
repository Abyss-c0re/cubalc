/* CubalC lang — lang_ops_flow_fstr_dumpobj.c
 * FLOW fstr dump: DUMPOBJ · INSPECT · OBJINFO
 * Peel from fstr_b (B_fstr_b_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_flow_fstr_dumpobj.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_fstr_dumpobj(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
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
}
