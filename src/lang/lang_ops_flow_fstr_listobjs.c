/* CubalC lang — lang_ops_flow_fstr_listobjs.c
 * FLOW fstr list/sample: LISTCLASSES · LISTOBJS · PICKOBJ · DRAWNOBJ · SHUFFLEOBJS
 * Peel from fstr_b (B_fstr_b_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_flow_fstr_listobjs.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_fstr_listobjs(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
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

  return 0;
}
