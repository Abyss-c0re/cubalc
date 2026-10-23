/* CubalC lang — lang_ops_flow_fstr_bagf.c
 * FLOW fstr bag-field plane: PUSHF · GREPF · SORTBAGF · LINESF
 * Peel from fstr_a (B_fstr_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_flow_fstr_bagf.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_fstr_bagf(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* PUSHF|BAGPUSHF obj field [line|LAST] â append newline field to bag-in-field.
   * UNSHIFTF|PUSHFRONTF obj field [line|LAST] â insert at front (FIFO enqueue).
   * POPF|BAGPOPF obj field â peel last line â LAST; field = rest (LIFO).
   * POPHEADF|DEQUEUEF obj field â peel first line â LAST; field = rest (FIFO).
   * TRYPUSHF|PUSHF SOFT â soft miss OK=0 sticky LAST_ERR (unknown obj/field).
   * PUSH: LAST = bag after; LAST_N/PUSHF_N = field count.
   * POP/POPHEAD: LAST = peeled line; LAST_N/POPF_HIT = 0|1; POPF_N = remaining count.
   * Empty pop: HIT=0 LAST="" field unchanged. Promotes numâstr.
   * Not PREPENDF/APPENDF (string concat). Complements LINEF/JOINF for work queues.
   * Usability: agent worklist on objects without GETF+SYS PUSH/POP+SETF
   * (METHOD/THIS). */
  if (kw(&L->cur, "PUSHF") || kw(&L->cur, "BAGPUSHF") ||
      kw(&L->cur, "LINEPUSHF") || kw(&L->cur, "ADDBAGF") ||
      kw(&L->cur, "UNSHIFTF") || kw(&L->cur, "PUSHFRONTF") ||
      kw(&L->cur, "BAGPREPENDF") || kw(&L->cur, "ENQUEUEF") ||
      kw(&L->cur, "POPF") || kw(&L->cur, "BAGPOPF") ||
      kw(&L->cur, "POPLINEFIELD") || kw(&L->cur, "LINEPOPF") ||
      kw(&L->cur, "POPHEADF") || kw(&L->cur, "DEQUEUEF") ||
      kw(&L->cur, "HEADPOPF") || kw(&L->cur, "SHIFTFRONTF") ||
      kw(&L->cur, "TRYPUSHF") || kw(&L->cur, "PUSHFSOFT") ||
      kw(&L->cur, "SOFTPUSHF") || kw(&L->cur, "TRYPOPF") ||
      kw(&L->cur, "TRYPOPHEADF") || kw(&L->cur, "TRYUNSHIFTF")) {
    char oname[48], fname[48], op[24], hay[256], line[256], out[256], peeled[256];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, mode = 0; /* 0=push, 1=unshift, 2=pop, 3=pophead */
    long nfields = 0, found = 0, rest_n = 0;
    const char *p, *last_nl;
    size_t blen, llen, o, flen;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "UNSHIFTF") == 0 || strcmp(op, "PUSHFRONTF") == 0 ||
        strcmp(op, "BAGPREPENDF") == 0 || strcmp(op, "ENQUEUEF") == 0 ||
        strcmp(op, "TRYUNSHIFTF") == 0)
      mode = 1;
    else if (strcmp(op, "POPF") == 0 || strcmp(op, "BAGPOPF") == 0 ||
             strcmp(op, "POPLINEFIELD") == 0 || strcmp(op, "LINEPOPF") == 0 ||
             strcmp(op, "TRYPOPF") == 0)
      mode = 2;
    else if (strcmp(op, "POPHEADF") == 0 || strcmp(op, "DEQUEUEF") == 0 ||
             strcmp(op, "HEADPOPF") == 0 || strcmp(op, "SHIFTFRONTF") == 0 ||
             strcmp(op, "TRYPOPHEADF") == 0)
      mode = 3;
    else
      mode = 0;
    if (strcmp(op, "TRYPUSHF") == 0 || strcmp(op, "PUSHFSOFT") == 0 ||
        strcmp(op, "SOFTPUSHF") == 0 || strcmp(op, "TRYPOPF") == 0 ||
        strcmp(op, "TRYPOPHEADF") == 0 || strcmp(op, "TRYUNSHIFTF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    {
      const char *usage =
          (mode == 2) ? "POPF object field"
        : (mode == 3) ? "POPHEADF object field"
        : (mode == 1) ? "UNSHIFTF object field [line]"
                      : "PUSHF object field [line]";
      if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
        fail(vm, usage); return -1;
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
        fail(vm, mode >= 2 ? (mode == 3 ? "POPHEADF field" : "POPF field")
                           : (mode == 1 ? "UNSHIFTF field" : "PUSHF field"));
        return -1;
      }
    }
    line[0] = 0;
    if (mode <= 1) {
      if (kw(&L->cur, "LINE") || kw(&L->cur, "WITH") || kw(&L->cur, "VALUE") ||
          kw(&L->cur, "ITEM"))
        lex_next(L);
      if (resolve_str_arg(vm, L, line, sizeof line) != 0)
        snprintf(line, sizeof line, "%s", vm->last_str);
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      const char *tag =
          (mode == 2) ? "POPF" : (mode == 3) ? "POPHEADF"
          : (mode == 1) ? "UNSHIFTF" : "PUSHF";
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s unknown object %s", tag, oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "PUSHF_N", 0);
      var_set_num(vm, "POPF_N", 0);
      var_set_num(vm, "POPF_HIT", 0);
      var_set_num(vm, "POPHEADF_HIT", 0);
      var_set_num(vm, "OK", 0);
      {
        char ebuf[48];
        snprintf(ebuf, sizeof ebuf, "%s: unknown object", tag);
        var_set_str(vm, "LAST_ERR", ebuf);
        var_set_str(vm, "ERR", ebuf);
      }
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      const char *tag =
          (mode == 2) ? "POPF" : (mode == 3) ? "POPHEADF"
          : (mode == 1) ? "UNSHIFTF" : "PUSHF";
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s unknown FIELD %s", tag, fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "PUSHF_N", 0);
      var_set_num(vm, "POPF_N", 0);
      var_set_num(vm, "POPF_HIT", 0);
      var_set_num(vm, "POPHEADF_HIT", 0);
      var_set_num(vm, "OK", 0);
      {
        char ebuf[48];
        snprintf(ebuf, sizeof ebuf, "%s: unknown field", tag);
        var_set_str(vm, "LAST_ERR", ebuf);
        var_set_str(vm, "ERR", ebuf);
      }
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi])
      snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
    else
      snprintf(hay, sizeof hay, "%ld", ob->fnum[fi]);
    blen = strlen(hay);
    out[0] = 0;
    peeled[0] = 0;
    found = 0;
    nfields = 0;
    rest_n = 0;
    if (mode <= 1) {
      /* push / unshift */
      llen = strlen(line);
      o = 0;
      if (mode == 0) {
        /* append */
        if (blen == 0) {
          if (llen < sizeof out) {
            memcpy(out, line, llen);
            o = llen;
          } else {
            o = sizeof out - 1;
            memcpy(out, line, o);
          }
          out[o] = 0;
          if (out[0]) {
            p = out;
            while (*p) {
              while (*p && *p != '\n') p++;
              nfields++;
              if (*p == '\n') p++;
            }
          } else {
            nfields = 1; /* empty line into empty bag still one field */
          }
        } else {
          long bag_n = 0;
          p = hay;
          while (*p) {
            while (*p && *p != '\n') p++;
            bag_n++;
            if (*p == '\n') p++;
          }
          if (blen < sizeof out) {
            memcpy(out, hay, blen);
            o = blen;
          } else {
            o = sizeof out - 1;
            memcpy(out, hay, o);
          }
          if (o + 1 < sizeof out) out[o++] = '\n';
          if (o + llen < sizeof out) {
            memcpy(out + o, line, llen);
            o += llen;
          } else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, line, take);
            o += take;
          }
          out[o] = 0;
          nfields = bag_n + 1;
        }
      } else {
        /* unshift front */
        if (llen < sizeof out) {
          memcpy(out, line, llen);
          o = llen;
        } else {
          o = sizeof out - 1;
          memcpy(out, line, o);
        }
        out[o] = 0;
        if (blen > 0) {
          if (o + 1 < sizeof out) out[o++] = '\n';
          if (o + blen < sizeof out) {
            memcpy(out + o, hay, blen);
            o += blen;
          } else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, hay, take);
            o += take;
          }
          out[o] = 0;
          nfields = 1;
          p = hay;
          while (*p) {
            while (*p && *p != '\n') p++;
            nfields++;
            if (*p == '\n') p++;
          }
        } else {
          nfields = 1;
        }
      }
      {
        size_t cap = sizeof ob->fstr[fi];
        if (strlen(out) >= cap) {
          memcpy(ob->fstr[fi], out, cap - 1);
          ob->fstr[fi][cap - 1] = 0;
        } else {
          snprintf(ob->fstr[fi], cap, "%s", out);
        }
      }
      ob->fis_str[fi] = 1;
      var_set_str(vm, "LAST", ob->fstr[fi]);
      var_set_str(vm, mode == 1 ? "UNSHIFTF" : "PUSHF", ob->fstr[fi]);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", ob->fstr[fi]);
      vm->last_n = nfields;
      var_set_num(vm, "LAST_N", nfields);
      var_set_num(vm, "PUSHF_N", nfields);
      var_set_num(vm, "UNSHIFTF_N", nfields);
      var_set_num(vm, "POPF_HIT", 0);
      var_set_num(vm, "POPHEADF_HIT", 0);
    } else if (mode == 2) {
      /* pop last */
      if (!hay[0]) {
        found = 0;
      } else {
        last_nl = NULL;
        for (p = hay; *p; p++)
          if (*p == '\n') last_nl = p;
        if (!last_nl) {
          snprintf(peeled, sizeof peeled, "%s", hay);
          out[0] = 0;
          found = 1;
          rest_n = 0;
        } else {
          flen = strlen(last_nl + 1);
          if (flen >= sizeof peeled) flen = sizeof peeled - 1;
          memcpy(peeled, last_nl + 1, flen);
          peeled[flen] = 0;
          {
            size_t rlen = (size_t)(last_nl - hay);
            if (rlen >= sizeof out) rlen = sizeof out - 1;
            memcpy(out, hay, rlen);
            out[rlen] = 0;
          }
          found = 1;
          if (out[0]) {
            p = out;
            while (*p) {
              while (*p && *p != '\n') p++;
              rest_n++;
              if (*p == '\n') p++;
            }
          }
        }
      }
      if (found) {
        snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", out);
        ob->fis_str[fi] = 1;
      }
      var_set_str(vm, "LAST", peeled);
      var_set_str(vm, "POPF", peeled);
      var_set_str(vm, "POPF_REST", out);
      var_set_str(vm, "REST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", peeled);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      var_set_num(vm, "POPF_HIT", found);
      var_set_num(vm, "POPF_N", rest_n);
      var_set_num(vm, "POPHEADF_HIT", 0);
      var_set_num(vm, "PUSHF_N", rest_n);
    } else {
      /* pophead first */
      if (!hay[0]) {
        found = 0;
      } else {
        p = hay;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - hay);
        if (flen >= sizeof peeled) flen = sizeof peeled - 1;
        memcpy(peeled, hay, flen);
        peeled[flen] = 0;
        found = 1;
        if (*p == '\n') {
          snprintf(out, sizeof out, "%s", p + 1);
          if (out[0]) {
            const char *q = out;
            while (*q) {
              while (*q && *q != '\n') q++;
              rest_n++;
              if (*q == '\n') q++;
            }
          }
        } else {
          out[0] = 0;
          rest_n = 0;
        }
      }
      if (found) {
        snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", out);
        ob->fis_str[fi] = 1;
      }
      var_set_str(vm, "LAST", peeled);
      var_set_str(vm, "POPHEADF", peeled);
      var_set_str(vm, "POPF_REST", out);
      var_set_str(vm, "REST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", peeled);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      var_set_num(vm, "POPHEADF_HIT", found);
      var_set_num(vm, "POPHEADF_N", rest_n);
      var_set_num(vm, "POPF_HIT", found);
      var_set_num(vm, "POPF_N", rest_n);
      var_set_num(vm, "PUSHF_N", rest_n);
    }
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* GREPF|FILTERF|KEEPF obj field needle â keep bag lines containing needle (mutate).
   * GREPVF|VGREPF|DROPMATCHF â invert (drop matching lines).
   * GREPFI|FILTERFI|IGREPF â case-insensitive keep; GREPVFI invert+icase.
   * TRYGREPF|GREPF SOFT â soft miss OK=0 sticky LAST_ERR.
   * LAST = filtered bag; LAST_N/GREPF_N = kept count; GREPF_HIT = unfiltered match count;
   * GREPF_TOTAL = source line count. Empty needle â keep all (or drop all if invert).
   * Promotes numâstr. Complements HASINF (probe) / PUSHF (queue) for log triage.
   * Usability: filter multi-line field bags without GETF+SYS GREP+SETF
   * (METHOD/THIS). Not SYS GREPFILE (path content). */
  if (kw(&L->cur, "GREPF") || kw(&L->cur, "FILTERF") ||
      kw(&L->cur, "KEEPF") || kw(&L->cur, "MATCHLINESF") ||
      kw(&L->cur, "BAGGREPF") || kw(&L->cur, "FIELDGREP") ||
      kw(&L->cur, "GREPVF") || kw(&L->cur, "VGREPF") ||
      kw(&L->cur, "DROPMATCHF") || kw(&L->cur, "FILTERVF") ||
      kw(&L->cur, "GREPFI") || kw(&L->cur, "FILTERFI") ||
      kw(&L->cur, "IGREPF") || kw(&L->cur, "KEEPFI") ||
      kw(&L->cur, "GREPVFI") || kw(&L->cur, "VGREPFI") ||
      kw(&L->cur, "IFILTERVF") || kw(&L->cur, "DROPMATCHFI") ||
      kw(&L->cur, "TRYGREPF") || kw(&L->cur, "GREPFSOFT") ||
      kw(&L->cur, "SOFTGREPF") || kw(&L->cur, "TRYFILTERF") ||
      kw(&L->cur, "TRYGREPVF") || kw(&L->cur, "TRYGREPFI")) {
    char oname[48], fname[48], op[24], hay[256], needle[128], out[256];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, invert = 0, icase = 0;
    long kept = 0, hits = 0, total = 0;
    const char *p, *start;
    size_t olen = 0, flen, nn;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "GREPVF") == 0 || strcmp(op, "VGREPF") == 0 ||
        strcmp(op, "DROPMATCHF") == 0 || strcmp(op, "FILTERVF") == 0 ||
        strcmp(op, "GREPVFI") == 0 || strcmp(op, "VGREPFI") == 0 ||
        strcmp(op, "IFILTERVF") == 0 || strcmp(op, "DROPMATCHFI") == 0 ||
        strcmp(op, "TRYGREPVF") == 0)
      invert = 1;
    if (strcmp(op, "GREPFI") == 0 || strcmp(op, "FILTERFI") == 0 ||
        strcmp(op, "IGREPF") == 0 || strcmp(op, "KEEPFI") == 0 ||
        strcmp(op, "GREPVFI") == 0 || strcmp(op, "VGREPFI") == 0 ||
        strcmp(op, "IFILTERVF") == 0 || strcmp(op, "DROPMATCHFI") == 0 ||
        strcmp(op, "TRYGREPFI") == 0)
      icase = 1;
    if (strcmp(op, "TRYGREPF") == 0 || strcmp(op, "GREPFSOFT") == 0 ||
        strcmp(op, "SOFTGREPF") == 0 || strcmp(op, "TRYFILTERF") == 0 ||
        strcmp(op, "TRYGREPVF") == 0 || strcmp(op, "TRYGREPFI") == 0)
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
    if (!invert && (kw(&L->cur, "V") || kw(&L->cur, "INVERT") ||
                    kw(&L->cur, "NOT") || kw(&L->cur, "DROP"))) {
      invert = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "GREPF object field needle"); return -1;
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
      fail(vm, "GREPF field"); return -1;
    }
    if (kw(&L->cur, "FOR") || kw(&L->cur, "MATCH") || kw(&L->cur, "NEEDLE") ||
        kw(&L->cur, "WITH") || kw(&L->cur, "CONTAINS"))
      lex_next(L);
    needle[0] = 0;
    if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
      snprintf(needle, sizeof needle, "%s", vm->last_str);
    nn = strlen(needle);
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "GREPF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "GREPF_N", 0);
      var_set_num(vm, "GREPF_HIT", 0);
      var_set_num(vm, "GREPF_TOTAL", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "GREPF: unknown object");
      var_set_str(vm, "ERR", "GREPF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "GREPF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "GREPF_N", 0);
      var_set_num(vm, "GREPF_HIT", 0);
      var_set_num(vm, "GREPF_TOTAL", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "GREPF: unknown field");
      var_set_str(vm, "ERR", "GREPF: unknown field");
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi])
      snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
    else
      snprintf(hay, sizeof hay, "%ld", ob->fnum[fi]);
    out[0] = 0;
    olen = 0;
    kept = 0;
    hits = 0;
    total = 0;
    if (hay[0]) {
      p = hay;
      while (*p) {
        int hit;
        char field[256];
        size_t take;
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        total++;
        take = flen;
        if (take >= sizeof field) take = sizeof field - 1;
        memcpy(field, start, take);
        field[take] = 0;
        if (nn == 0) {
          hit = 1;
        } else if (!icase) {
          hit = (strstr(field, needle) != NULL) ? 1 : 0;
        } else {
          size_t fi2, j;
          hit = 0;
          for (fi2 = 0; field[fi2] && !hit; fi2++) {
            for (j = 0; j < nn; j++) {
              char a = field[fi2 + j], b = needle[j];
              if (!a) break;
              if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
              if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
              if (a != b) break;
            }
            if (j == nn) hit = 1;
          }
        }
        if (hit) hits++;
        if (invert) hit = !hit;
        if (hit) {
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + flen < sizeof out) {
            memcpy(out + olen, start, flen);
            olen += flen;
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            memcpy(out + olen, start, t);
            olen += t;
          }
          out[olen] = 0;
          kept++;
        }
        if (*p == '\n') p++;
      }
    }
    out[olen] = 0;
    {
      size_t cap = sizeof ob->fstr[fi];
      if (olen >= cap) {
        memcpy(ob->fstr[fi], out, cap - 1);
        ob->fstr[fi][cap - 1] = 0;
      } else {
        memcpy(ob->fstr[fi], out, olen + 1);
      }
    }
    ob->fis_str[fi] = 1;
    var_set_str(vm, "LAST", ob->fstr[fi]);
    var_set_str(vm, "GREPF", ob->fstr[fi]);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", ob->fstr[fi]);
    vm->last_n = kept;
    var_set_num(vm, "LAST_N", kept);
    var_set_num(vm, "GREPF_N", kept);
    var_set_num(vm, "FILTERF_N", kept);
    var_set_num(vm, "GREPF_HIT", hits);
    var_set_num(vm, "GREPF_TOTAL", total);
    var_set_num(vm, "GREPVF_N", kept);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SORTBAGF|SORTLINESF|BAGSORTF|SORTF obj field â lex-sort newline bag in field (mutate).
   * UNIQBAGF|DEDUPBAGF|ADJUNIQF obj field â drop adjacent duplicates (like SYS UNIQ).
   * UNIQUEBAGF|DEDUPALLF|UALLF obj field â first-seen full unique (order-preserving).
   * TRYSORTBAGF|SORTBAGF SOFT â soft miss OK=0 sticky LAST_ERR.
   * LAST = bag after; LAST_N/SORTBAGF_N = field count. Cap 48 lines Ã 128.
   * Not fleet UNIQUF/DISTINCTF/UNIQF (unique field *values* across objects).
   * Usability: worklist hygiene after PUSHF/GREPF without GETF+SYS SORT/UNIQ+SETF
   * (METHOD/THIS). */
  if (kw(&L->cur, "SORTBAGF") || kw(&L->cur, "SORTLINESF") ||
      kw(&L->cur, "BAGSORTF") || kw(&L->cur, "LINESORTF") ||
      kw(&L->cur, "SORTF") ||
      kw(&L->cur, "UNIQBAGF") || kw(&L->cur, "DEDUPBAGF") ||
      kw(&L->cur, "ADJUNIQF") || kw(&L->cur, "UNIQADJF") ||
      kw(&L->cur, "ADJACENTUNIQF") ||
      kw(&L->cur, "UNIQUEBAGF") || kw(&L->cur, "DEDUPALLF") ||
      kw(&L->cur, "UALLF") || kw(&L->cur, "BAGUNIQUEF") ||
      kw(&L->cur, "TRYSORTBAGF") || kw(&L->cur, "SORTBAGFSOFT") ||
      kw(&L->cur, "SOFTSORTBAGF") || kw(&L->cur, "TRYUNIQBAGF") ||
      kw(&L->cur, "TRYUNIQUEBAGF") || kw(&L->cur, "TRYSORTF")) {
    char oname[48], fname[48], op[24], hay[256], out[256];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, mode = 0; /* 0=sort, 1=adj uniq, 2=distinct */
    enum { SB_MAX = 48, SB_FLEN = 128 };
    char fields[SB_MAX][SB_FLEN];
    int order[SB_MAX];
    int n = 0, i;
    long kept = 0;
    const char *p, *start;
    size_t olen = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "UNIQBAGF") == 0 || strcmp(op, "DEDUPBAGF") == 0 ||
        strcmp(op, "ADJUNIQF") == 0 || strcmp(op, "UNIQADJF") == 0 ||
        strcmp(op, "ADJACENTUNIQF") == 0 || strcmp(op, "TRYUNIQBAGF") == 0)
      mode = 1;
    else if (strcmp(op, "UNIQUEBAGF") == 0 || strcmp(op, "DEDUPALLF") == 0 ||
             strcmp(op, "UALLF") == 0 || strcmp(op, "BAGUNIQUEF") == 0 ||
             strcmp(op, "TRYUNIQUEBAGF") == 0)
      mode = 2;
    else
      mode = 0;
    if (strcmp(op, "TRYSORTBAGF") == 0 || strcmp(op, "SORTBAGFSOFT") == 0 ||
        strcmp(op, "SOFTSORTBAGF") == 0 || strcmp(op, "TRYUNIQBAGF") == 0 ||
        strcmp(op, "TRYUNIQUEBAGF") == 0 || strcmp(op, "TRYSORTF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    {
      const char *usage =
          (mode == 1) ? "UNIQBAGF object field"
        : (mode == 2) ? "UNIQUEBAGF object field"
                      : "SORTBAGF object field";
      if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
        fail(vm, usage); return -1;
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
        fail(vm, mode == 1 ? "UNIQBAGF field"
             : mode == 2 ? "UNIQUEBAGF field" : "SORTBAGF field");
        return -1;
      }
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      const char *tag =
          (mode == 1) ? "UNIQBAGF" : (mode == 2) ? "UNIQUEBAGF" : "SORTBAGF";
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s unknown object %s", tag, oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "SORTBAGF_N", 0);
      var_set_num(vm, "UNIQBAGF_N", 0);
      var_set_num(vm, "UNIQUEBAGF_N", 0);
      var_set_num(vm, "OK", 0);
      {
        char ebuf[48];
        snprintf(ebuf, sizeof ebuf, "%s: unknown object", tag);
        var_set_str(vm, "LAST_ERR", ebuf);
        var_set_str(vm, "ERR", ebuf);
      }
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      const char *tag =
          (mode == 1) ? "UNIQBAGF" : (mode == 2) ? "UNIQUEBAGF" : "SORTBAGF";
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s unknown FIELD %s", tag, fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "SORTBAGF_N", 0);
      var_set_num(vm, "UNIQBAGF_N", 0);
      var_set_num(vm, "UNIQUEBAGF_N", 0);
      var_set_num(vm, "OK", 0);
      {
        char ebuf[48];
        snprintf(ebuf, sizeof ebuf, "%s: unknown field", tag);
        var_set_str(vm, "LAST_ERR", ebuf);
        var_set_str(vm, "ERR", ebuf);
      }
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi])
      snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
    else
      snprintf(hay, sizeof hay, "%ld", ob->fnum[fi]);
    n = 0;
    if (hay[0]) {
      p = hay;
      while (*p && n < SB_MAX) {
        start = p;
        while (*p && *p != '\n') p++;
        {
          size_t flen = (size_t)(p - start);
          if (flen >= SB_FLEN) flen = SB_FLEN - 1;
          memcpy(fields[n], start, flen);
          fields[n][flen] = 0;
          order[n] = n;
          n++;
        }
        if (*p == '\n') p++;
      }
    }
    out[0] = 0;
    olen = 0;
    kept = 0;
    if (mode == 0 && n > 1) {
      for (i = 1; i < n; i++) {
        int key = order[i], j = i - 1;
        while (j >= 0 && strcmp(fields[order[j]], fields[key]) > 0) {
          order[j + 1] = order[j];
          j--;
        }
        order[j + 1] = key;
      }
    }
    if (mode == 2) {
      /* first-seen distinct */
      for (i = 0; i < n; i++) {
        int seen = 0, j;
        for (j = 0; j < i && !seen; j++) {
          if (strcmp(fields[i], fields[j]) == 0) seen = 1;
        }
        if (seen) continue;
        if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        {
          size_t flen = strlen(fields[i]);
          if (olen + flen < sizeof out) {
            memcpy(out + olen, fields[i], flen);
            olen += flen;
          } else if (olen < sizeof out - 1) {
            size_t take = sizeof out - 1 - olen;
            memcpy(out + olen, fields[i], take);
            olen += take;
          }
          out[olen] = 0;
        }
        kept++;
      }
    } else {
      /* sort (mode 0) or adjacent uniq (mode 1, walk original order) */
      for (i = 0; i < n; i++) {
        int idx = (mode == 0) ? order[i] : i;
        if (mode == 1 && i > 0 && strcmp(fields[i], fields[i - 1]) == 0)
          continue;
        if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        {
          size_t flen = strlen(fields[idx]);
          if (olen + flen < sizeof out) {
            memcpy(out + olen, fields[idx], flen);
            olen += flen;
          } else if (olen < sizeof out - 1) {
            size_t take = sizeof out - 1 - olen;
            memcpy(out + olen, fields[idx], take);
            olen += take;
          }
          out[olen] = 0;
        }
        kept++;
      }
    }
    out[olen] = 0;
    {
      size_t cap = sizeof ob->fstr[fi];
      if (olen >= cap) {
        memcpy(ob->fstr[fi], out, cap - 1);
        ob->fstr[fi][cap - 1] = 0;
      } else {
        memcpy(ob->fstr[fi], out, olen + 1);
      }
    }
    ob->fis_str[fi] = 1;
    var_set_str(vm, "LAST", ob->fstr[fi]);
    var_set_str(vm, mode == 1 ? "UNIQBAGF" : mode == 2 ? "UNIQUEBAGF" : "SORTBAGF",
                ob->fstr[fi]);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", ob->fstr[fi]);
    vm->last_n = kept;
    var_set_num(vm, "LAST_N", kept);
    var_set_num(vm, "SORTBAGF_N", kept);
    var_set_num(vm, "SORTF_N", kept);
    var_set_num(vm, "UNIQBAGF_N", kept);
    var_set_num(vm, "UNIQUEBAGF_N", kept);
    var_set_num(vm, "DEDUPALLF_N", kept);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LINESF|BAGLINESF|COUNTBAGF obj field â count newline bag lines â LAST_N (no mutate).
   * HASBAGLINE|BAGHASLINE|LINEINBAGF obj field line â exact bag membership â LAST_N 0|1.
   * DROPBAGLINE|BAGDROPLINE|ACKBAGLINE obj field line â drop first exact line (mutate).
   * HASBAGLINEI / DROPBAGLINEI â case-insensitive exact field.
   * TRYLINESF|LINESF SOFT â soft miss OK=0 sticky LAST_ERR.
   * LINESF: LAST = bag text; LAST_N/LINESF_N = count.
   * HAS: LAST_N = 0|1; DROP: LAST = bag after; LAST_N/DROPBAGLINE_HIT = 0|1.
   * Not SYS HASLINEF/REMOVELINEF (file-plane). Complements PUSHF/GREPF for work queues.
   * Usability: IF/ack bag work without GETF+SYS LINES/HASLINE/REMOVELINE+SETF
   * (METHOD/THIS). */
  if (kw(&L->cur, "LINESF") || kw(&L->cur, "BAGLINESF") ||
      kw(&L->cur, "COUNTBAGF") || kw(&L->cur, "NLINESF") ||
      kw(&L->cur, "WCFIELD") || kw(&L->cur, "BAGCOUNTF") ||
      kw(&L->cur, "HASBAGLINE") || kw(&L->cur, "BAGHASLINE") ||
      kw(&L->cur, "LINEINBAGF") || kw(&L->cur, "BAGCONTAINSLINE") ||
      kw(&L->cur, "HASBAGLINEI") || kw(&L->cur, "BAGHASLINEI") ||
      kw(&L->cur, "LINEINBAGFI") ||
      kw(&L->cur, "DROPBAGLINE") || kw(&L->cur, "BAGDROPLINE") ||
      kw(&L->cur, "ACKBAGLINE") || kw(&L->cur, "REMOVELINEBAG") ||
      kw(&L->cur, "DROPBAGLINEI") || kw(&L->cur, "BAGDROPLINEI") ||
      kw(&L->cur, "ACKBAGLINEI") ||
      kw(&L->cur, "TRYLINESF") || kw(&L->cur, "LINESFSOFT") ||
      kw(&L->cur, "SOFTLINESF") || kw(&L->cur, "TRYHASBAGLINE") ||
      kw(&L->cur, "TRYDROPBAGLINE")) {
    char oname[48], fname[48], op[24], hay[256], needle[128], out[256];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, mode = 0; /* 0=lines, 1=has, 2=drop */
    int icase = 0;
    long nfields = 0, hit = 0, removed = 0;
    const char *p, *start;
    size_t nn, flen, o;
    int first_kept;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "HASBAGLINE") == 0 || strcmp(op, "BAGHASLINE") == 0 ||
        strcmp(op, "LINEINBAGF") == 0 || strcmp(op, "BAGCONTAINSLINE") == 0 ||
        strcmp(op, "HASBAGLINEI") == 0 || strcmp(op, "BAGHASLINEI") == 0 ||
        strcmp(op, "LINEINBAGFI") == 0 || strcmp(op, "TRYHASBAGLINE") == 0)
      mode = 1;
    else if (strcmp(op, "DROPBAGLINE") == 0 || strcmp(op, "BAGDROPLINE") == 0 ||
             strcmp(op, "ACKBAGLINE") == 0 || strcmp(op, "REMOVELINEBAG") == 0 ||
             strcmp(op, "DROPBAGLINEI") == 0 || strcmp(op, "BAGDROPLINEI") == 0 ||
             strcmp(op, "ACKBAGLINEI") == 0 || strcmp(op, "TRYDROPBAGLINE") == 0)
      mode = 2;
    else
      mode = 0;
    if (strcmp(op, "HASBAGLINEI") == 0 || strcmp(op, "BAGHASLINEI") == 0 ||
        strcmp(op, "LINEINBAGFI") == 0 || strcmp(op, "DROPBAGLINEI") == 0 ||
        strcmp(op, "BAGDROPLINEI") == 0 || strcmp(op, "ACKBAGLINEI") == 0)
      icase = 1;
    if (strcmp(op, "TRYLINESF") == 0 || strcmp(op, "LINESFSOFT") == 0 ||
        strcmp(op, "SOFTLINESF") == 0 || strcmp(op, "TRYHASBAGLINE") == 0 ||
        strcmp(op, "TRYDROPBAGLINE") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (!icase && mode >= 1 && (kw(&L->cur, "I") || kw(&L->cur, "ICASE") ||
                                kw(&L->cur, "IGNORECASE") || kw(&L->cur, "-I") ||
                                kw(&L->cur, "CI"))) {
      icase = 1;
      lex_next(L);
    }
    {
      const char *usage =
          (mode == 1) ? "HASBAGLINE object field line"
        : (mode == 2) ? "DROPBAGLINE object field line"
                      : "LINESF object field";
      if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
        fail(vm, usage); return -1;
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
        fail(vm, mode == 1 ? "HASBAGLINE field"
             : mode == 2 ? "DROPBAGLINE field" : "LINESF field");
        return -1;
      }
    }
    needle[0] = 0;
    if (mode >= 1) {
      if (kw(&L->cur, "LINE") || kw(&L->cur, "ITEM") || kw(&L->cur, "VALUE") ||
          kw(&L->cur, "OF") || kw(&L->cur, "IS") || kw(&L->cur, "EQ"))
        lex_next(L);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
        snprintf(needle, sizeof needle, "%s", vm->last_str);
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      const char *tag =
          (mode == 1) ? "HASBAGLINE" : (mode == 2) ? "DROPBAGLINE" : "LINESF";
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s unknown object %s", tag, oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "LINESF_N", 0);
      var_set_num(vm, "HASBAGLINE_N", 0);
      var_set_num(vm, "DROPBAGLINE_HIT", 0);
      var_set_num(vm, "OK", 0);
      {
        char ebuf[48];
        snprintf(ebuf, sizeof ebuf, "%s: unknown object", tag);
        var_set_str(vm, "LAST_ERR", ebuf);
        var_set_str(vm, "ERR", ebuf);
      }
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      const char *tag =
          (mode == 1) ? "HASBAGLINE" : (mode == 2) ? "DROPBAGLINE" : "LINESF";
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s unknown FIELD %s", tag, fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "LINESF_N", 0);
      var_set_num(vm, "HASBAGLINE_N", 0);
      var_set_num(vm, "DROPBAGLINE_HIT", 0);
      var_set_num(vm, "OK", 0);
      {
        char ebuf[48];
        snprintf(ebuf, sizeof ebuf, "%s: unknown field", tag);
        var_set_str(vm, "LAST_ERR", ebuf);
        var_set_str(vm, "ERR", ebuf);
      }
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi])
      snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
    else
      snprintf(hay, sizeof hay, "%ld", ob->fnum[fi]);
    nn = strlen(needle);
    nfields = 0;
    hit = 0;
    removed = 0;
    out[0] = 0;
    o = 0;
    first_kept = 1;
    if (mode == 0) {
      if (hay[0]) {
        p = hay;
        while (*p) {
          while (*p && *p != '\n') p++;
          nfields++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", hay);
      var_set_str(vm, "LINESF", hay);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hay);
      vm->last_n = nfields;
      var_set_num(vm, "LAST_N", nfields);
      var_set_num(vm, "LINESF_N", nfields);
      var_set_num(vm, "COUNTBAGF_N", nfields);
      var_set_num(vm, "BAGLINESF_N", nfields);
    } else if (mode == 1) {
      if (hay[0]) {
        p = hay;
        while (*p && !hit) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen == nn) {
            if (!icase) {
              if (nn == 0 || memcmp(start, needle, nn) == 0) hit = 1;
            } else {
              size_t i; int okm = 1;
              for (i = 0; i < nn; i++) {
                char a = start[i], b = needle[i];
                if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                if (a != b) { okm = 0; break; }
              }
              if (okm) hit = 1;
            }
          }
          if (*p == '\n') p++;
        }
      }
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "HASBAGLINE_N", hit);
      var_set_num(vm, "BAGHASLINE_N", hit);
      var_set_str(vm, "LAST", hit ? needle : "");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hit ? needle : "");
    } else {
      /* drop first exact */
      p = hay;
      while (*p) {
        int match = 0;
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (!removed && flen == nn) {
          if (!icase) {
            match = (nn == 0 || memcmp(start, needle, nn) == 0) ? 1 : 0;
          } else {
            size_t i; match = 1;
            for (i = 0; i < nn; i++) {
              char a = start[i], b = needle[i];
              if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
              if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
              if (a != b) { match = 0; break; }
            }
          }
        }
        if (match) {
          removed = 1;
        } else {
          if (!first_kept && o + 1 < sizeof out) out[o++] = '\n';
          first_kept = 0;
          if (o + flen < sizeof out) {
            memcpy(out + o, start, flen);
            o += flen;
          } else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, start, take);
            o += take;
          }
          out[o] = 0;
        }
        if (*p == '\n') p++;
      }
      if (!removed)
        snprintf(out, sizeof out, "%s", hay);
      {
        size_t cap = sizeof ob->fstr[fi];
        size_t olen = strlen(out);
        if (olen >= cap) {
          memcpy(ob->fstr[fi], out, cap - 1);
          ob->fstr[fi][cap - 1] = 0;
        } else {
          memcpy(ob->fstr[fi], out, olen + 1);
        }
      }
      ob->fis_str[fi] = 1;
      var_set_str(vm, "LAST", ob->fstr[fi]);
      var_set_str(vm, "DROPBAGLINE", ob->fstr[fi]);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", ob->fstr[fi]);
      vm->last_n = removed;
      var_set_num(vm, "LAST_N", removed);
      var_set_num(vm, "DROPBAGLINE_HIT", removed);
      var_set_num(vm, "DROPBAGLINE_N", removed);
      var_set_num(vm, "ACKBAGLINE_HIT", removed);
    }
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }
  return 0;
}
