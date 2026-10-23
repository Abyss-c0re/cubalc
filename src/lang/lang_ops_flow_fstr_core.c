/* CubalC lang — lang_ops_flow_fstr_core.c
 * FLOW fstr field core: REPLACEF · TRIMF · LENF · HASINF · CLEARF
 * Peel from fstr_a (B_fstr_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_flow_fstr_core.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_fstr_core(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* REPLACEF|GSUBF|SUBSTF obj field old new
   * TRYREPLACEF|REPLACEF SOFT â soft miss OK=0.
   * REPLACEF FIRST â first occurrence only (default: all like GSUB).
   * In-place string surgery on a field; promotes numâstr.
   * LAST = result; LAST_N = replace count. Empty old = no-op.
   * Usability: rewrite status/note/templates without GETF+REPLACEALL+SETF
   * (METHOD/THIS; complements CATF concat). */
  if (kw(&L->cur, "REPLACEF") || kw(&L->cur, "GSUBF") ||
      kw(&L->cur, "SUBSTF") || kw(&L->cur, "FIELDREPLACE") ||
      kw(&L->cur, "REPF") || kw(&L->cur, "STRREPF") ||
      kw(&L->cur, "SUBSTFIELD") || kw(&L->cur, "GSUBFIELD") ||
      kw(&L->cur, "TRYREPLACEF") || kw(&L->cur, "REPLACEFSOFT") ||
      kw(&L->cur, "SOFTREPLACEF") || kw(&L->cur, "TRYGSUBF") ||
      kw(&L->cur, "TRYSUBSTF")) {
    char oname[48], fname[48], olds[256], news[256], hay[256], out[256];
    char op[24];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, do_all = 1;
    long did = 0;
    size_t oldn, newn;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "TRYREPLACEF") == 0 || strcmp(op, "REPLACEFSOFT") == 0 ||
        strcmp(op, "SOFTREPLACEF") == 0 || strcmp(op, "TRYGSUBF") == 0 ||
        strcmp(op, "TRYSUBSTF") == 0)
      soft = 1;
    /* GSUBF always all; plain REPLACEF defaults all, FIRST optional */
    if (strcmp(op, "GSUBF") == 0 || strcmp(op, "GSUBFIELD") == 0 ||
        strcmp(op, "TRYGSUBF") == 0)
      do_all = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (kw(&L->cur, "FIRST") || kw(&L->cur, "ONCE") || kw(&L->cur, "ONE")) {
      do_all = 0;
      lex_next(L);
    } else if (kw(&L->cur, "ALL") || kw(&L->cur, "GLOBAL") ||
               kw(&L->cur, "EVERY") || kw(&L->cur, "G")) {
      do_all = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "REPLACEF object field old new"); return -1;
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
      fail(vm, "REPLACEF field"); return -1;
    }
    olds[0] = 0;
    news[0] = 0;
    if (resolve_str_arg(vm, L, olds, sizeof olds) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        long v = parse_expr(vm, L);
        snprintf(olds, sizeof olds, "%ld", v);
      } else {
        fail(vm, "REPLACEF object field old new"); return -1;
      }
    }
    if (kw(&L->cur, "WITH") || kw(&L->cur, "BY") || kw(&L->cur, "TO") ||
        kw(&L->cur, "AS") || kw(&L->cur, "INTO"))
      lex_next(L);
    if (resolve_str_arg(vm, L, news, sizeof news) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        long v = parse_expr(vm, L);
        snprintf(news, sizeof news, "%ld", v);
      } else {
        fail(vm, "REPLACEF object field old new"); return -1;
      }
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "REPLACEF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "REPLACEF_N", 0);
      var_set_num(vm, "GSUBF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "REPLACEF: unknown object");
      var_set_str(vm, "ERR", "REPLACEF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "REPLACEF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "REPLACEF_N", 0);
      var_set_num(vm, "GSUBF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "REPLACEF: unknown field");
      var_set_str(vm, "ERR", "REPLACEF: unknown field");
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi])
      snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
    else
      snprintf(hay, sizeof hay, "%ld", ob->fnum[fi]);
    oldn = strlen(olds);
    newn = strlen(news);
    did = 0;
    if (olds[0] == 0) {
      snprintf(out, sizeof out, "%s", hay);
    } else {
      const char *src = hay;
      size_t o = 0;
      out[0] = 0;
      for (;;) {
        const char *p = strstr(src, olds);
        if (!p) {
          size_t rest = strlen(src);
          if (o + rest >= sizeof out) rest = sizeof out - 1 - o;
          memcpy(out + o, src, rest);
          o += rest;
          out[o] = 0;
          break;
        }
        {
          size_t pre = (size_t)(p - src);
          if (o + pre >= sizeof out) pre = sizeof out - 1 - o;
          memcpy(out + o, src, pre);
          o += pre;
          if (o + newn < sizeof out) {
            memcpy(out + o, news, newn);
            o += newn;
          } else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, news, take);
            o += take;
          }
          out[o] = 0;
          did++;
          src = p + oldn;
          if (!do_all) {
            size_t rest = strlen(src);
            if (o + rest >= sizeof out) rest = sizeof out - 1 - o;
            memcpy(out + o, src, rest);
            o += rest;
            out[o] = 0;
            break;
          }
          if (o >= sizeof out - 1) break;
        }
      }
    }
    snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", out);
    ob->fis_str[fi] = 1;
    var_set_str(vm, "LAST", ob->fstr[fi]);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", ob->fstr[fi]);
    vm->last_n = did;
    var_set_num(vm, "LAST_N", did);
    var_set_num(vm, "REPLACEF_N", 1);
    var_set_num(vm, "GSUBF_N", 1);
    var_set_num(vm, "REPLACEF_COUNT", did);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* TRIMF|LTRIMF|RTRIMF|UPPERF|LOWERF obj field
   * TRYTRIMF|TRIMF SOFT â soft miss OK=0.
   * In-place normalize string field (whitespace trim / ASCII case);
   * promotes numâstr. LAST = result; LAST_N = length.
   * Usability: clean status/note before IF without GETF+TRIM/UPPER/LOWER+SETF
   * (METHOD/THIS; pairs REPLACEF/CATF). */
  if (kw(&L->cur, "TRIMF") || kw(&L->cur, "STRIPF") ||
      kw(&L->cur, "LTRIMF") || kw(&L->cur, "RTRIMF") ||
      kw(&L->cur, "FIELDTRIM") || kw(&L->cur, "TRIMFIELD") ||
      kw(&L->cur, "UPPERF") || kw(&L->cur, "UCASEF") ||
      kw(&L->cur, "TOUPPERF") || kw(&L->cur, "FIELDUPPER") ||
      kw(&L->cur, "LOWERF") || kw(&L->cur, "LCASEF") ||
      kw(&L->cur, "TOLOWERF") || kw(&L->cur, "FIELDLOWER") ||
      kw(&L->cur, "TRYTRIMF") || kw(&L->cur, "TRIMFSOFT") ||
      kw(&L->cur, "SOFTTRIMF") || kw(&L->cur, "TRYUPPERF") ||
      kw(&L->cur, "TRYLOWERF")) {
    char oname[48], fname[48], op[24], hay[256], out[256];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, mode = 0; /* 0=trim both, 1=ltrim, 2=rtrim, 3=upper, 4=lower */
    char *a, *b;
    size_t n;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "LTRIMF") == 0)
      mode = 1;
    else if (strcmp(op, "RTRIMF") == 0)
      mode = 2;
    else if (strcmp(op, "UPPERF") == 0 || strcmp(op, "UCASEF") == 0 ||
             strcmp(op, "TOUPPERF") == 0 || strcmp(op, "FIELDUPPER") == 0 ||
             strcmp(op, "TRYUPPERF") == 0)
      mode = 3;
    else if (strcmp(op, "LOWERF") == 0 || strcmp(op, "LCASEF") == 0 ||
             strcmp(op, "TOLOWERF") == 0 || strcmp(op, "FIELDLOWER") == 0 ||
             strcmp(op, "TRYLOWERF") == 0)
      mode = 4;
    else
      mode = 0; /* TRIMF / STRIPF / FIELDTRIM */
    if (strcmp(op, "TRYTRIMF") == 0 || strcmp(op, "TRIMFSOFT") == 0 ||
        strcmp(op, "SOFTTRIMF") == 0 || strcmp(op, "TRYUPPERF") == 0 ||
        strcmp(op, "TRYLOWERF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    /* TRIMF L|R|LEFT|RIGHT | TRIMF BOTH */
    if (mode == 0) {
      if (kw(&L->cur, "L") || kw(&L->cur, "LEFT") || kw(&L->cur, "LTRIM")) {
        mode = 1;
        lex_next(L);
      } else if (kw(&L->cur, "R") || kw(&L->cur, "RIGHT") ||
                 kw(&L->cur, "RTRIM")) {
        mode = 2;
        lex_next(L);
      } else if (kw(&L->cur, "BOTH") || kw(&L->cur, "ALL") ||
                 kw(&L->cur, "LR")) {
        mode = 0;
        lex_next(L);
      }
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "TRIMF object field"); return -1;
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
      fail(vm, "TRIMF field"); return -1;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "TRIMF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "TRIMF_N", 0);
      var_set_num(vm, "UPPERF_N", 0);
      var_set_num(vm, "LOWERF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "TRIMF: unknown object");
      var_set_str(vm, "ERR", "TRIMF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "TRIMF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "TRIMF_N", 0);
      var_set_num(vm, "UPPERF_N", 0);
      var_set_num(vm, "LOWERF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "TRIMF: unknown field");
      var_set_str(vm, "ERR", "TRIMF: unknown field");
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi])
      snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
    else
      snprintf(hay, sizeof hay, "%ld", ob->fnum[fi]);
    if (mode == 3) {
      /* UPPER */
      {
        char *p;
        for (p = hay; *p; p++)
          if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 'a' + 'A');
      }
      snprintf(out, sizeof out, "%s", hay);
    } else if (mode == 4) {
      /* LOWER */
      {
        char *p;
        for (p = hay; *p; p++)
          if (*p >= 'A' && *p <= 'Z') *p = (char)(*p - 'A' + 'a');
      }
      snprintf(out, sizeof out, "%s", hay);
    } else {
      a = hay;
      b = hay + strlen(hay);
      if (mode == 0 || mode == 1)
        while (*a == ' ' || *a == '\t' || *a == '\n' || *a == '\r') a++;
      if (mode == 0 || mode == 2)
        while (b > a &&
               (b[-1] == ' ' || b[-1] == '\t' || b[-1] == '\n' ||
                b[-1] == '\r'))
          b--;
      n = (size_t)(b - a);
      if (n >= sizeof out) n = sizeof out - 1;
      memcpy(out, a, n);
      out[n] = 0;
    }
    snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", out);
    ob->fis_str[fi] = 1;
    n = strlen(out);
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = (long)n;
    var_set_num(vm, "LAST_N", (long)n);
    var_set_num(vm, "TRIMF_N", (mode <= 2) ? 1 : 0);
    var_set_num(vm, "LTRIMF_N", (mode == 1) ? 1 : 0);
    var_set_num(vm, "RTRIMF_N", (mode == 2) ? 1 : 0);
    var_set_num(vm, "UPPERF_N", (mode == 3) ? 1 : 0);
    var_set_num(vm, "LOWERF_N", (mode == 4) ? 1 : 0);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LENF|STRLENF|EMPTYF|BLANKF|NONEMPTYF obj field
   * TRYLENF|LENF SOFT â soft miss OK=0.
   * Field probes without mutating: length / empty / blank / nonempty.
   * LAST = field text (num promoted); LAST_N = length (LENF) or 0|1 probe.
   * Usability: IF guards on note/status without GETF+SYS LEN/EMPTY glue
   * (METHOD/THIS; complements TRIMF mutators). */
  if (kw(&L->cur, "LENF") || kw(&L->cur, "STRLENF") ||
      kw(&L->cur, "FIELDLEN") || kw(&L->cur, "LENFIELD") ||
      kw(&L->cur, "EMPTYF") || kw(&L->cur, "ISEMPTYF") ||
      kw(&L->cur, "FIELDEMPTY") || kw(&L->cur, "EMPTYFIELD") ||
      kw(&L->cur, "BLANKF") || kw(&L->cur, "ISBLANKF") ||
      kw(&L->cur, "FIELDBLANK") || kw(&L->cur, "BLANKFIELD") ||
      kw(&L->cur, "NONEMPTYF") || kw(&L->cur, "NOTEMPTYF") ||
      kw(&L->cur, "HASCHARSF") || kw(&L->cur, "FIELDFULL") ||
      kw(&L->cur, "TRYLENF") || kw(&L->cur, "LENFSOFT") ||
      kw(&L->cur, "SOFTLENF") || kw(&L->cur, "TRYEMPTYF") ||
      kw(&L->cur, "TRYBLANKF") || kw(&L->cur, "TRYNONEMPTYF")) {
    char oname[48], fname[48], op[24], hay[256];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, mode = 0; /* 0=len, 1=empty, 2=blank, 3=nonempty */
    long hit = 0, len = 0;
    const char *p;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "EMPTYF") == 0 || strcmp(op, "ISEMPTYF") == 0 ||
        strcmp(op, "FIELDEMPTY") == 0 || strcmp(op, "EMPTYFIELD") == 0 ||
        strcmp(op, "TRYEMPTYF") == 0)
      mode = 1;
    else if (strcmp(op, "BLANKF") == 0 || strcmp(op, "ISBLANKF") == 0 ||
             strcmp(op, "FIELDBLANK") == 0 || strcmp(op, "BLANKFIELD") == 0 ||
             strcmp(op, "TRYBLANKF") == 0)
      mode = 2;
    else if (strcmp(op, "NONEMPTYF") == 0 || strcmp(op, "NOTEMPTYF") == 0 ||
             strcmp(op, "HASCHARSF") == 0 || strcmp(op, "FIELDFULL") == 0 ||
             strcmp(op, "TRYNONEMPTYF") == 0)
      mode = 3;
    else
      mode = 0; /* LENF */
    if (strcmp(op, "TRYLENF") == 0 || strcmp(op, "LENFSOFT") == 0 ||
        strcmp(op, "SOFTLENF") == 0 || strcmp(op, "TRYEMPTYF") == 0 ||
        strcmp(op, "TRYBLANKF") == 0 || strcmp(op, "TRYNONEMPTYF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "LENF object field"); return -1;
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
      fail(vm, "LENF field"); return -1;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "LENF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "LENF_N", 0);
      var_set_num(vm, "EMPTYF_N", 0);
      var_set_num(vm, "BLANKF_N", 0);
      var_set_num(vm, "NONEMPTYF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "LENF: unknown object");
      var_set_str(vm, "ERR", "LENF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "LENF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "LENF_N", 0);
      var_set_num(vm, "EMPTYF_N", 0);
      var_set_num(vm, "BLANKF_N", 0);
      var_set_num(vm, "NONEMPTYF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "LENF: unknown field");
      var_set_str(vm, "ERR", "LENF: unknown field");
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi])
      snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
    else
      snprintf(hay, sizeof hay, "%ld", ob->fnum[fi]);
    len = (long)strlen(hay);
    if (mode == 1) {
      hit = (hay[0] == 0) ? 1 : 0;
    } else if (mode == 2) {
      p = hay;
      while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
      hit = (*p == 0) ? 1 : 0;
    } else if (mode == 3) {
      hit = (hay[0] != 0) ? 1 : 0;
    } else {
      hit = len; /* LENF reports length in LAST_N */
    }
    var_set_str(vm, "LAST", hay);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hay);
    vm->last_n = hit;
    var_set_num(vm, "LAST_N", hit);
    var_set_num(vm, "LENF_N", len);
    var_set_num(vm, "EMPTYF_N", (hay[0] == 0) ? 1 : 0);
    {
      p = hay;
      while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
      var_set_num(vm, "BLANKF_N", (*p == 0) ? 1 : 0);
    }
    var_set_num(vm, "NONEMPTYF_N", (hay[0] != 0) ? 1 : 0);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASINF|CONTAINSF|STARTSF|ENDSF obj field needle
   * HASIFI|STARTSIF|ENDSIF â case-insensitive.
   * TRYHASINF|HASINF SOFT â soft miss OK=0.
   * Field membership probes (no mutate). LAST=field text; LAST_N 0|1.
   * Usability: IF on status/note without GETF+SYS HAS/STARTS/ENDS
   * (METHOD/THIS; complements LENF/EMPTYF probes). */
  if (kw(&L->cur, "HASINF") || kw(&L->cur, "CONTAINSF") ||
      kw(&L->cur, "FIELDHAS") || kw(&L->cur, "HASFIELDSTR") ||
      kw(&L->cur, "INSTRF") || kw(&L->cur, "INCLUDESF") ||
      kw(&L->cur, "HASIFI") || kw(&L->cur, "CONTAINSFI") ||
      kw(&L->cur, "IHASINF") || kw(&L->cur, "FIELDHASI") ||
      kw(&L->cur, "STARTSF") || kw(&L->cur, "PREFIXFHAS") ||
      kw(&L->cur, "FIELDSTARTS") || kw(&L->cur, "STARTSWITHF") ||
      kw(&L->cur, "STARTSIF") || kw(&L->cur, "ISTARTSF") ||
      kw(&L->cur, "PREFIXFI") ||
      kw(&L->cur, "ENDSF") || kw(&L->cur, "SUFFIXFHAS") ||
      kw(&L->cur, "FIELDENDS") || kw(&L->cur, "ENDSWITHF") ||
      kw(&L->cur, "ENDSIF") || kw(&L->cur, "IENDSF") ||
      kw(&L->cur, "SUFFIXFI") ||
      kw(&L->cur, "TRYHASINF") || kw(&L->cur, "HASINFSOFT") ||
      kw(&L->cur, "SOFTHASINF") || kw(&L->cur, "TRYSTARTSF") ||
      kw(&L->cur, "TRYENDSF") || kw(&L->cur, "TRYHASIFI")) {
    char oname[48], fname[48], op[24], hay[256], needle[256];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, mode = 0; /* 0=has, 1=starts, 2=ends */
    int icase = 0;
    long hit = 0;
    size_t hn, nn;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "STARTSF") == 0 || strcmp(op, "PREFIXFHAS") == 0 ||
        strcmp(op, "FIELDSTARTS") == 0 || strcmp(op, "STARTSWITHF") == 0 ||
        strcmp(op, "STARTSIF") == 0 || strcmp(op, "ISTARTSF") == 0 ||
        strcmp(op, "PREFIXFI") == 0 || strcmp(op, "TRYSTARTSF") == 0)
      mode = 1;
    else if (strcmp(op, "ENDSF") == 0 || strcmp(op, "SUFFIXFHAS") == 0 ||
             strcmp(op, "FIELDENDS") == 0 || strcmp(op, "ENDSWITHF") == 0 ||
             strcmp(op, "ENDSIF") == 0 || strcmp(op, "IENDSF") == 0 ||
             strcmp(op, "SUFFIXFI") == 0 || strcmp(op, "TRYENDSF") == 0)
      mode = 2;
    else
      mode = 0; /* HAS */
    if (strcmp(op, "HASIFI") == 0 || strcmp(op, "CONTAINSFI") == 0 ||
        strcmp(op, "IHASINF") == 0 || strcmp(op, "FIELDHASI") == 0 ||
        strcmp(op, "STARTSIF") == 0 || strcmp(op, "ISTARTSF") == 0 ||
        strcmp(op, "PREFIXFI") == 0 || strcmp(op, "ENDSIF") == 0 ||
        strcmp(op, "IENDSF") == 0 || strcmp(op, "SUFFIXFI") == 0 ||
        strcmp(op, "TRYHASIFI") == 0)
      icase = 1;
    if (strcmp(op, "TRYHASINF") == 0 || strcmp(op, "HASINFSOFT") == 0 ||
        strcmp(op, "SOFTHASINF") == 0 || strcmp(op, "TRYSTARTSF") == 0 ||
        strcmp(op, "TRYENDSF") == 0 || strcmp(op, "TRYHASIFI") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (!icase && (kw(&L->cur, "I") || kw(&L->cur, "ICASE") ||
                   kw(&L->cur, "IGNORECASE") || kw(&L->cur, "-I") ||
                   kw(&L->cur, "CI"))) {
      icase = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "HASINF object field needle"); return -1;
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
      fail(vm, "HASINF field"); return -1;
    }
    needle[0] = 0;
    if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        long v = parse_expr(vm, L);
        snprintf(needle, sizeof needle, "%ld", v);
      } else {
        fail(vm, "HASINF object field needle"); return -1;
      }
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "HASINF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "HASINF_N", 0);
      var_set_num(vm, "STARTSF_N", 0);
      var_set_num(vm, "ENDSF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASINF: unknown object");
      var_set_str(vm, "ERR", "HASINF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "HASINF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "HASINF_N", 0);
      var_set_num(vm, "STARTSF_N", 0);
      var_set_num(vm, "ENDSF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASINF: unknown field");
      var_set_str(vm, "ERR", "HASINF: unknown field");
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi])
      snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
    else
      snprintf(hay, sizeof hay, "%ld", ob->fnum[fi]);
    hn = strlen(hay);
    nn = strlen(needle);
    hit = 0;
    if (nn == 0) {
      hit = 1; /* empty needle matches (like SYS HAS) */
    } else if (mode == 0) {
      /* contains */
      if (!icase) {
        hit = (strstr(hay, needle) != NULL) ? 1 : 0;
      } else {
        size_t i, j;
        for (i = 0; i + nn <= hn; i++) {
          int ok = 1;
          for (j = 0; j < nn; j++) {
            char ca = hay[i + j], cb = needle[j];
            if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
            if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
            if (ca != cb) { ok = 0; break; }
          }
          if (ok) { hit = 1; break; }
        }
      }
    } else if (mode == 1) {
      /* starts */
      if (nn <= hn) {
        if (!icase) {
          hit = (strncmp(hay, needle, nn) == 0) ? 1 : 0;
        } else {
          size_t j;
          hit = 1;
          for (j = 0; j < nn; j++) {
            char ca = hay[j], cb = needle[j];
            if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
            if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
            if (ca != cb) { hit = 0; break; }
          }
        }
      }
    } else {
      /* ends */
      if (nn <= hn) {
        if (!icase) {
          hit = (strcmp(hay + (hn - nn), needle) == 0) ? 1 : 0;
        } else {
          size_t j, base = hn - nn;
          hit = 1;
          for (j = 0; j < nn; j++) {
            char ca = hay[base + j], cb = needle[j];
            if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
            if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
            if (ca != cb) { hit = 0; break; }
          }
        }
      }
    }
    var_set_str(vm, "LAST", hay);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hay);
    vm->last_n = hit;
    var_set_num(vm, "LAST_N", hit);
    var_set_num(vm, "HASINF_N", (mode == 0) ? hit : 0);
    var_set_num(vm, "STARTSF_N", (mode == 1) ? hit : 0);
    var_set_num(vm, "ENDSF_N", (mode == 2) ? hit : 0);
    var_set_num(vm, "HASIFI_N", (mode == 0 && icase) ? hit : 0);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* CLEARF|ZEROF|RESETF|DEFAULTF obj field [value]
   * TRYCLEARF|CLEARF SOFT â soft miss OK=0.
   * CLEARF â wipe to "" (str class/cur) or 0 (num).
   * RESETF â restore CLASS field default (NEW init value).
   * DEFAULTF field value â set only if empty/blank (str) or 0 (num).
   * LAST = result; LAST_N = 1 if wrote, 0 if DEFAULTF skipped.
   * Usability: clean/reset/fill-if-missing without GETF+IF+SETF (METHOD/THIS). */
  if (kw(&L->cur, "CLEARF") || kw(&L->cur, "ZEROF") ||
      kw(&L->cur, "WIPEF") || kw(&L->cur, "EMPTYSETF") ||
      kw(&L->cur, "CLEAREIELD") || kw(&L->cur, "CLEARFIELD") ||
      kw(&L->cur, "RESETF") || kw(&L->cur, "RESTOREF") ||
      kw(&L->cur, "DEFAULTF") || kw(&L->cur, "DEFAULTFIELD") ||
      kw(&L->cur, "FILLF") || kw(&L->cur, "IFNULLF") ||
      kw(&L->cur, "TRYCLEARF") || kw(&L->cur, "CLEARFSOFT") ||
      kw(&L->cur, "SOFTCLEARF") || kw(&L->cur, "TRYRESETF") ||
      kw(&L->cur, "TRYDEFAULTF")) {
    char oname[48], fname[48], op[24], sval[256];
    ObjInst *ob;
    ClassDef *cd;
    FieldDef *fd;
    int fi, soft = 0, mode = 0; /* 0=clear, 1=reset, 2=default */
    int wrote = 0, want_str = 0;
    long nval = 0;
    const char *p;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "RESETF") == 0 || strcmp(op, "RESTOREF") == 0 ||
        strcmp(op, "TRYRESETF") == 0)
      mode = 1;
    else if (strcmp(op, "DEFAULTF") == 0 || strcmp(op, "DEFAULTFIELD") == 0 ||
             strcmp(op, "FILLF") == 0 || strcmp(op, "IFNULLF") == 0 ||
             strcmp(op, "TRYDEFAULTF") == 0)
      mode = 2;
    else
      mode = 0; /* CLEARF */
    if (strcmp(op, "TRYCLEARF") == 0 || strcmp(op, "CLEARFSOFT") == 0 ||
        strcmp(op, "SOFTCLEARF") == 0 || strcmp(op, "TRYRESETF") == 0 ||
        strcmp(op, "TRYDEFAULTF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "CLEARF object field"); return -1;
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
      fail(vm, "CLEARF field"); return -1;
    }
    sval[0] = 0;
    want_str = 0;
    nval = 0;
    if (mode == 2) {
      /* DEFAULTF requires value */
      if (kw(&L->cur, "WITH") || kw(&L->cur, "TO") || kw(&L->cur, "AS") ||
          kw(&L->cur, "BY") || kw(&L->cur, "=") || L->cur.kind == TK_EQ) {
        if (L->cur.kind == TK_EQ) lex_next(L);
        else lex_next(L);
      }
      if (L->cur.kind == TK_STR) {
        snprintf(sval, sizeof sval, "%s", L->cur.text);
        want_str = 1;
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && strcmp(L->cur.text, "LAST") == 0) {
        snprintf(sval, sizeof sval, "%s", vm->last_str);
        want_str = 1;
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
        Var *sv = var_get(vm, L->cur.text, 0);
        if (sv && sv->is_str) {
          snprintf(sval, sizeof sval, "%s", sv->sval);
          want_str = 1;
          lex_next(L);
        } else {
          nval = parse_expr(vm, L);
          want_str = 0;
        }
      } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
                 L->cur.kind == TK_LPAREN) {
        nval = parse_expr(vm, L);
        want_str = 0;
      } else {
        fail(vm, "DEFAULTF object field value"); return -1;
      }
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "CLEARF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "CLEARF_N", 0);
      var_set_num(vm, "RESETF_N", 0);
      var_set_num(vm, "DEFAULTF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "CLEARF: unknown object");
      var_set_str(vm, "ERR", "CLEARF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "CLEARF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "CLEARF_N", 0);
      var_set_num(vm, "RESETF_N", 0);
      var_set_num(vm, "DEFAULTF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "CLEARF: unknown field");
      var_set_str(vm, "ERR", "CLEARF: unknown field");
      bump(vm);
      return 1;
    }
    fd = &cd->fields[fi];
    wrote = 0;
    if (mode == 1) {
      /* RESETF â class default */
      if (fd->has_def && fd->is_str) {
        snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", fd->def_str);
        ob->fis_str[fi] = 1;
      } else if (fd->has_def) {
        ob->fnum[fi] = fd->def_num;
        ob->fis_str[fi] = 0;
      } else if (fd->is_str) {
        ob->fstr[fi][0] = 0;
        ob->fis_str[fi] = 1;
      } else {
        ob->fnum[fi] = 0;
        ob->fis_str[fi] = 0;
      }
      wrote = 1;
    } else if (mode == 2) {
      /* DEFAULTF â only if empty/blank or zero */
      int is_empty = 0;
      if (ob->fis_str[fi]) {
        p = ob->fstr[fi];
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        is_empty = (*p == 0);
      } else {
        is_empty = (ob->fnum[fi] == 0);
      }
      if (is_empty) {
        if (want_str) {
          snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", sval);
          ob->fis_str[fi] = 1;
        } else {
          ob->fnum[fi] = nval;
          ob->fis_str[fi] = 0;
        }
        wrote = 1;
      }
    } else {
      /* CLEARF â wipe */
      if (fd->is_str || ob->fis_str[fi]) {
        ob->fstr[fi][0] = 0;
        ob->fis_str[fi] = 1;
      } else {
        ob->fnum[fi] = 0;
        ob->fis_str[fi] = 0;
      }
      wrote = 1;
    }
    if (ob->fis_str[fi]) {
      var_set_str(vm, "LAST", ob->fstr[fi]);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", ob->fstr[fi]);
      vm->last_n = wrote ? 1 : 0;
      if (mode != 2)
        vm->last_n = (long)strlen(ob->fstr[fi]);
    } else {
      char nb[24];
      snprintf(nb, sizeof nb, "%ld", ob->fnum[fi]);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
      vm->last_n = (mode == 2) ? (wrote ? 1 : 0) : ob->fnum[fi];
    }
    /* DEFAULTF: LAST_N always 0|1 applied; CLEARF length/value; RESETF too */
    if (mode == 2)
      var_set_num(vm, "LAST_N", wrote ? 1 : 0);
    else if (ob->fis_str[fi])
      var_set_num(vm, "LAST_N", (long)strlen(ob->fstr[fi]));
    else
      var_set_num(vm, "LAST_N", ob->fnum[fi]);
    vm->last_n = (mode == 2) ? (wrote ? 1 : 0)
                             : (ob->fis_str[fi] ? (long)strlen(ob->fstr[fi])
                                                : ob->fnum[fi]);
    var_set_num(vm, "CLEARF_N", (mode == 0 && wrote) ? 1 : 0);
    var_set_num(vm, "RESETF_N", (mode == 1 && wrote) ? 1 : 0);
    var_set_num(vm, "DEFAULTF_N", (mode == 2) ? (wrote ? 1 : 0) : 0);
    var_set_num(vm, "DEFAULTF_APPLIED", (mode == 2 && wrote) ? 1 : 0);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }
  return 0;
}
