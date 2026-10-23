/* CubalC lang — lang_ops_flow_fstr_fieldbag.c
 * FLOW fstr field-bag: EQF · COUNTINF · SPLITF · JOINF · WORDSF · LINEF · ENSUREBAGLINE
 * Peel from fstr_a (B_fstr_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_flow_fstr_fieldbag.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int cubalc_lang_ops_flow_try_fstr_fieldbag(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* EQF|NEQF|EQFI obj field value
   * FINDF|FINDFI obj field needle â index of first needle or -1.
   * TRYEQF|EQF SOFT â soft miss OK=0.
   * Exact equality / inequality / locate without GETF+SYS EQS/FIND.
   * LAST = field text; LAST_N = 0|1 (EQ*) or index|-1 (FIND*).
   * Usability: IF status=="open" / locate error offset (METHOD/THIS;
   * complements HASINF contains probes). */
  if (kw(&L->cur, "EQF") || kw(&L->cur, "EQFIELD") ||
      kw(&L->cur, "FIELDEQ") || kw(&L->cur, "SAMEF") ||
      kw(&L->cur, "NEQF") || kw(&L->cur, "NEFIELD") ||
      kw(&L->cur, "FIELDNE") || kw(&L->cur, "DIFFF") ||
      kw(&L->cur, "EQFI") || kw(&L->cur, "IEQF") ||
      kw(&L->cur, "FIELDEQI") || kw(&L->cur, "SAMEFI") ||
      kw(&L->cur, "FINDF") || kw(&L->cur, "INDEXF") ||
      kw(&L->cur, "FIELDFIND") || kw(&L->cur, "STRINDF") ||
      kw(&L->cur, "FINDFI") || kw(&L->cur, "IFINDF") ||
      kw(&L->cur, "INDEXFI") || kw(&L->cur, "FIELDFINDI") ||
      kw(&L->cur, "TRYEQF") || kw(&L->cur, "EQFSOFT") ||
      kw(&L->cur, "SOFTEQF") || kw(&L->cur, "TRYNEQF") ||
      kw(&L->cur, "TRYFINDF") || kw(&L->cur, "TRYFINDFI")) {
    char oname[48], fname[48], op[24], hay[256], rhs[256];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, mode = 0; /* 0=eq, 1=ne, 2=find */
    int icase = 0;
    long hit = 0;
    size_t hn, nn, i, j;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "NEQF") == 0 || strcmp(op, "NEFIELD") == 0 ||
        strcmp(op, "FIELDNE") == 0 || strcmp(op, "DIFFF") == 0 ||
        strcmp(op, "TRYNEQF") == 0)
      mode = 1;
    else if (strcmp(op, "FINDF") == 0 || strcmp(op, "INDEXF") == 0 ||
             strcmp(op, "FIELDFIND") == 0 || strcmp(op, "STRINDF") == 0 ||
             strcmp(op, "FINDFI") == 0 || strcmp(op, "IFINDF") == 0 ||
             strcmp(op, "INDEXFI") == 0 || strcmp(op, "FIELDFINDI") == 0 ||
             strcmp(op, "TRYFINDF") == 0 || strcmp(op, "TRYFINDFI") == 0)
      mode = 2;
    else
      mode = 0; /* EQF */
    if (strcmp(op, "EQFI") == 0 || strcmp(op, "IEQF") == 0 ||
        strcmp(op, "FIELDEQI") == 0 || strcmp(op, "SAMEFI") == 0 ||
        strcmp(op, "FINDFI") == 0 || strcmp(op, "IFINDF") == 0 ||
        strcmp(op, "INDEXFI") == 0 || strcmp(op, "FIELDFINDI") == 0 ||
        strcmp(op, "TRYFINDFI") == 0)
      icase = 1;
    if (strcmp(op, "TRYEQF") == 0 || strcmp(op, "EQFSOFT") == 0 ||
        strcmp(op, "SOFTEQF") == 0 || strcmp(op, "TRYNEQF") == 0 ||
        strcmp(op, "TRYFINDF") == 0 || strcmp(op, "TRYFINDFI") == 0)
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
      fail(vm, "EQF object field value"); return -1;
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
      fail(vm, "EQF field"); return -1;
    }
    if (kw(&L->cur, "WITH") || kw(&L->cur, "TO") || kw(&L->cur, "AS") ||
        kw(&L->cur, "EQ") || kw(&L->cur, "IS") || L->cur.kind == TK_EQ ||
        L->cur.kind == TK_EQEQ) {
      if (L->cur.kind == TK_EQ || L->cur.kind == TK_EQEQ) lex_next(L);
      else lex_next(L);
    }
    rhs[0] = 0;
    if (resolve_str_arg(vm, L, rhs, sizeof rhs) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        long v = parse_expr(vm, L);
        snprintf(rhs, sizeof rhs, "%ld", v);
      } else {
        fail(vm, "EQF object field value"); return -1;
      }
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "EQF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", (mode == 2) ? -1 : 0);
      vm->last_n = (mode == 2) ? -1 : 0;
      var_set_num(vm, "EQF_N", 0);
      var_set_num(vm, "FINDF_N", -1);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "EQF: unknown object");
      var_set_str(vm, "ERR", "EQF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "EQF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", (mode == 2) ? -1 : 0);
      vm->last_n = (mode == 2) ? -1 : 0;
      var_set_num(vm, "EQF_N", 0);
      var_set_num(vm, "FINDF_N", -1);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "EQF: unknown field");
      var_set_str(vm, "ERR", "EQF: unknown field");
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi])
      snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
    else
      snprintf(hay, sizeof hay, "%ld", ob->fnum[fi]);
    hn = strlen(hay);
    nn = strlen(rhs);
    if (mode == 2) {
      /* FINDF â first index or -1 */
      hit = -1;
      if (nn == 0) {
        hit = 0;
      } else if (!icase) {
        const char *p = strstr(hay, rhs);
        if (p) hit = (long)(p - hay);
      } else {
        for (i = 0; i + nn <= hn; i++) {
          int ok = 1;
          for (j = 0; j < nn; j++) {
            char ca = hay[i + j], cb = rhs[j];
            if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
            if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
            if (ca != cb) { ok = 0; break; }
          }
          if (ok) { hit = (long)i; break; }
        }
      }
    } else {
      /* EQ / NE */
      if (!icase) {
        hit = (strcmp(hay, rhs) == 0) ? 1 : 0;
      } else {
        hit = 1;
        for (i = 0; ; i++) {
          char ca = hay[i], cb = rhs[i];
          if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
          if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
          if (ca != cb) { hit = 0; break; }
          if (!hay[i]) break;
        }
      }
      if (mode == 1) hit = hit ? 0 : 1;
    }
    var_set_str(vm, "LAST", hay);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hay);
    vm->last_n = hit;
    var_set_num(vm, "LAST_N", hit);
    var_set_num(vm, "EQF_N", (mode == 0) ? hit : 0);
    var_set_num(vm, "NEQF_N", (mode == 1) ? hit : 0);
    var_set_num(vm, "EQFI_N", (mode == 0 && icase) ? hit : 0);
    var_set_num(vm, "FINDF_N", (mode == 2) ? hit : -1);
    var_set_num(vm, "FINDF_I", (mode == 2) ? hit : -1);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* COUNTINF|NCOUNTF|OCCURSF obj field needle â count non-overlapping needle hits.
   * COUNTINFI|NCOUNTFI â case-insensitive.
   * TRYCOUNTINF|COUNTINF SOFT â soft miss OK=0 sticky LAST_ERR.
   * No mutate. LAST = field text; LAST_N = count. Empty needle â 0.
   * Usability: path depth / delimiter tally without GETF+loop (METHOD/THIS;
   * complements HASINF 0|1 and FINDF first index). Note: fleet COUNTF is hist. */
  if (kw(&L->cur, "COUNTINF") || kw(&L->cur, "NCOUNTF") ||
      kw(&L->cur, "OCCURSF") || kw(&L->cur, "OCCURF") ||
      kw(&L->cur, "TALLYF") || kw(&L->cur, "NMATCHF") ||
      kw(&L->cur, "COUNTNEEDLEF") || kw(&L->cur, "FIELDCOUNT") ||
      kw(&L->cur, "COUNTINFI") || kw(&L->cur, "NCOUNTFI") ||
      kw(&L->cur, "OCCURSFI") || kw(&L->cur, "TALLYFI") ||
      kw(&L->cur, "ICOUNTINF") || kw(&L->cur, "COUNTINFI") ||
      kw(&L->cur, "TRYCOUNTINF") || kw(&L->cur, "COUNTINFSOFT") ||
      kw(&L->cur, "SOFTCOUNTINF") || kw(&L->cur, "TRYNCOUNTF") ||
      kw(&L->cur, "TRYCOUNTINFI")) {
    char oname[48], fname[48], op[24], hay[256], needle[256];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, icase = 0;
    long hit = 0;
    size_t hn, nn, i, j;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "COUNTINFI") == 0 || strcmp(op, "NCOUNTFI") == 0 ||
        strcmp(op, "OCCURSFI") == 0 || strcmp(op, "TALLYFI") == 0 ||
        strcmp(op, "ICOUNTINF") == 0 || strcmp(op, "TRYCOUNTINFI") == 0)
      icase = 1;
    if (strcmp(op, "TRYCOUNTINF") == 0 || strcmp(op, "COUNTINFSOFT") == 0 ||
        strcmp(op, "SOFTCOUNTINF") == 0 || strcmp(op, "TRYNCOUNTF") == 0 ||
        strcmp(op, "TRYCOUNTINFI") == 0)
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
      fail(vm, "COUNTINF object field needle"); return -1;
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
      fail(vm, "COUNTINF field"); return -1;
    }
    if (kw(&L->cur, "OF") || kw(&L->cur, "WITH") || kw(&L->cur, "FOR") ||
        kw(&L->cur, "NEEDLE") || kw(&L->cur, "AS"))
      lex_next(L);
    needle[0] = 0;
    if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        long v = parse_expr(vm, L);
        snprintf(needle, sizeof needle, "%ld", v);
      } else {
        fail(vm, "COUNTINF object field needle"); return -1;
      }
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "COUNTINF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "COUNTINF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "COUNTINF: unknown object");
      var_set_str(vm, "ERR", "COUNTINF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "COUNTINF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "COUNTINF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "COUNTINF: unknown field");
      var_set_str(vm, "ERR", "COUNTINF: unknown field");
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
    if (nn > 0) {
      if (!icase) {
        const char *p = hay;
        while (p && *p) {
          const char *q = strstr(p, needle);
          if (!q) break;
          hit++;
          p = q + nn; /* non-overlapping */
        }
      } else {
        for (i = 0; i + nn <= hn; ) {
          int ok = 1;
          for (j = 0; j < nn; j++) {
            char ca = hay[i + j], cb = needle[j];
            if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
            if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
            if (ca != cb) { ok = 0; break; }
          }
          if (ok) {
            hit++;
            i += nn;
          } else {
            i++;
          }
        }
      }
    }
    var_set_str(vm, "LAST", hay);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", hay);
    vm->last_n = hit;
    var_set_num(vm, "LAST_N", hit);
    var_set_num(vm, "COUNTINF_N", hit);
    var_set_num(vm, "NCOUNTF_N", hit);
    var_set_num(vm, "OCCURSF_N", hit);
    var_set_num(vm, "COUNTINF_I", icase ? 1 : 0);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* SPLITF|FIELDSF|CUTSEPF obj field sep â split field on sep â newline bag.
   * TRYSPLITF|SPLITF SOFT â soft miss OK=0 sticky LAST_ERR.
   * No mutate on field. LAST = bag; LAST_N/SPLITF_N = part count.
   * Empty sep â whole field as one part; empty field â 0 parts.
   * Usability: path/CSV walk with EACH LINE without GETF+SYS SPLIT
   * (METHOD/THIS; pairs COUNTINF depth + BEFOREF peels). */
  if (kw(&L->cur, "SPLITF") || kw(&L->cur, "FIELDSF") ||
      kw(&L->cur, "CUTSEPF") || kw(&L->cur, "FIELDSPLIT") ||
      kw(&L->cur, "SPLITFIELD") || kw(&L->cur, "PARTSF") ||
      kw(&L->cur, "SEPF") || kw(&L->cur, "STRSPLITF") ||
      kw(&L->cur, "TRYSPLITF") || kw(&L->cur, "SPLITFSOFT") ||
      kw(&L->cur, "SOFTSPLITF") || kw(&L->cur, "TRYFIELDSF")) {
    char oname[48], fname[48], op[24], hay[256], sep[64], out[512];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0;
    const char *p, *hitp;
    size_t sepn, olen = 0, flen;
    long nfields = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "TRYSPLITF") == 0 || strcmp(op, "SPLITFSOFT") == 0 ||
        strcmp(op, "SOFTSPLITF") == 0 || strcmp(op, "TRYFIELDSF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "SPLITF object field sep"); return -1;
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
      fail(vm, "SPLITF field"); return -1;
    }
    if (kw(&L->cur, "BY") || kw(&L->cur, "ON") || kw(&L->cur, "WITH") ||
        kw(&L->cur, "SEP") || kw(&L->cur, "AT"))
      lex_next(L);
    sep[0] = 0;
    if (resolve_str_arg(vm, L, sep, sizeof sep) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        long v = parse_expr(vm, L);
        snprintf(sep, sizeof sep, "%ld", v);
      } else {
        fail(vm, "SPLITF object field sep"); return -1;
      }
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "SPLITF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "SPLITF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "SPLITF: unknown object");
      var_set_str(vm, "ERR", "SPLITF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "SPLITF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "SPLITF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "SPLITF: unknown field");
      var_set_str(vm, "ERR", "SPLITF: unknown field");
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi])
      snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
    else
      snprintf(hay, sizeof hay, "%ld", ob->fnum[fi]);
    out[0] = 0;
    olen = 0;
    nfields = 0;
    sepn = strlen(sep);
    if (!hay[0]) {
      /* empty â zero parts */
    } else if (sepn == 0) {
      snprintf(out, sizeof out, "%s", hay);
      nfields = 1;
      olen = strlen(out);
    } else {
      p = hay;
      for (;;) {
        hitp = strstr(p, sep);
        if (hitp) flen = (size_t)(hitp - p);
        else flen = strlen(p);
        if (nfields > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        if (olen + flen < sizeof out) {
          memcpy(out + olen, p, flen);
          olen += flen;
        } else if (olen < sizeof out - 1) {
          size_t take = sizeof out - 1 - olen;
          memcpy(out + olen, p, take);
          olen += take;
        }
        out[olen] = 0;
        nfields++;
        if (!hitp) break;
        p = hitp + sepn;
      }
    }
    var_set_str(vm, "LAST", out);
    var_set_str(vm, "SPLITF", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = nfields;
    var_set_num(vm, "LAST_N", nfields);
    var_set_num(vm, "SPLITF_N", nfields);
    var_set_num(vm, "FIELDSF_N", nfields);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* JOINF|JOINLINESF|PASTEF obj field sep [bag|LAST]
   * TRYJOINF|JOINF SOFT â soft miss OK=0 sticky LAST_ERR.
   * Join newline bag with sep into field (inverse of SPLITF).
   * Bag defaults to LAST. LAST = joined text; LAST_N/JOINF_N = part count.
   * Empty bag â empty field; empty sep concatenates with no delimiter.
   * Usability: rebuild path/CSV after bag edit without GETF+SYS JOINLINES+SETF
   * (METHOD/THIS; pairs SPLITF). */
  if (kw(&L->cur, "JOINF") || kw(&L->cur, "JOINLINESF") ||
      kw(&L->cur, "PASTEF") || kw(&L->cur, "MERGEF") ||
      kw(&L->cur, "IMPLADEF") || kw(&L->cur, "FIELDJOIN") ||
      kw(&L->cur, "JOINFIELD") || kw(&L->cur, "BAGJOINF") ||
      kw(&L->cur, "TRYJOINF") || kw(&L->cur, "JOINFSOFT") ||
      kw(&L->cur, "SOFTJOINF") || kw(&L->cur, "TRYPASTEF") ||
      kw(&L->cur, "TRYJOINLINESF")) {
    char oname[48], fname[48], op[24], sep[64], bag[512], out[128];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0;
    const char *p, *start;
    size_t sepn, olen = 0, flen;
    long nfields = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "TRYJOINF") == 0 || strcmp(op, "JOINFSOFT") == 0 ||
        strcmp(op, "SOFTJOINF") == 0 || strcmp(op, "TRYPASTEF") == 0 ||
        strcmp(op, "TRYJOINLINESF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "JOINF object field sep [bag]"); return -1;
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
      fail(vm, "JOINF field"); return -1;
    }
    if (kw(&L->cur, "WITH") || kw(&L->cur, "BY") || kw(&L->cur, "SEP") ||
        kw(&L->cur, "ON") || kw(&L->cur, "USING"))
      lex_next(L);
    sep[0] = 0;
    if (resolve_str_arg(vm, L, sep, sizeof sep) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        long v = parse_expr(vm, L);
        snprintf(sep, sizeof sep, "%ld", v);
      } else {
        fail(vm, "JOINF object field sep [bag]"); return -1;
      }
    }
    if (kw(&L->cur, "FROM") || kw(&L->cur, "BAG") || kw(&L->cur, "LINES") ||
        kw(&L->cur, "OF"))
      lex_next(L);
    bag[0] = 0;
    if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
      snprintf(bag, sizeof bag, "%s", vm->last_str);
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "JOINF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "JOINF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "JOINF: unknown object");
      var_set_str(vm, "ERR", "JOINF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "JOINF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "JOINF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "JOINF: unknown field");
      var_set_str(vm, "ERR", "JOINF: unknown field");
      bump(vm);
      return 1;
    }
    out[0] = 0;
    olen = 0;
    nfields = 0;
    sepn = strlen(sep);
    if (bag[0]) {
      p = bag;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        /* trailing bare newline after last field: still count empty? match JOINLINES:
         * if start==p at end after consuming newlines from previous, only break if
         * we already finished content. JOINLINES skips final empty after trailing \n
         * when start==p && *p==0 && start>src && start[-1]=='\n' */
        if (start == p && *p == 0 && start > bag && start[-1] == '\n')
          break;
        flen = (size_t)(p - start);
        if (nfields > 0 && sepn > 0) {
          if (olen + sepn < sizeof out) {
            memcpy(out + olen, sep, sepn);
            olen += sepn;
          } else if (olen < sizeof out - 1) {
            size_t take = sizeof out - 1 - olen;
            memcpy(out + olen, sep, take);
            olen += take;
          }
        }
        if (olen + flen < sizeof out) {
          memcpy(out + olen, start, flen);
          olen += flen;
        } else if (olen < sizeof out - 1) {
          size_t take = sizeof out - 1 - olen;
          memcpy(out + olen, start, take);
          olen += take;
        }
        out[olen] = 0;
        nfields++;
        if (*p == '\n') p++;
      }
    }
    snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", out);
    ob->fis_str[fi] = 1;
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = nfields;
    var_set_num(vm, "LAST_N", nfields);
    var_set_num(vm, "JOINF_N", nfields);
    var_set_num(vm, "JOINLINESF_N", nfields);
    var_set_num(vm, "PASTEF_N", nfields);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* STRIPPREFIXF|DROPPREFIXF|UNPREFIXF obj field prefix
   * STRIPSUFFIXF|DROPSUFFIXF|UNSUFFIXF obj field suffix
   * TRYSTRIPPREFIXF|STRIPPREFIXF SOFT â soft miss OK=0 sticky LAST_ERR.
   * Drop fixed leading/trailing affix when present; no-op if missing.
   * Empty affix â no-op. Promotes numâstr. LAST = result; LAST_N = length;
   * STRIPPREFIXF_HIT / STRIPSUFFIXF_HIT = 1 if shortened.
   * Usability: path roots / file extensions without GETF+STARTS+SLICEF+SETF
   * (METHOD/THIS; pairs STARTSF/ENDSF/BEFOREF). */
  if (kw(&L->cur, "STRIPPREFIXF") || kw(&L->cur, "DROPPREFIXF") ||
      kw(&L->cur, "UNPREFIXF") || kw(&L->cur, "CHOPPREFIXF") ||
      kw(&L->cur, "REMOVEPREFIXF") || kw(&L->cur, "FIELDSTRIPPRE") ||
      kw(&L->cur, "STRIPSUFFIXF") || kw(&L->cur, "DROPSUFFIXF") ||
      kw(&L->cur, "UNSUFFIXF") || kw(&L->cur, "CHOPSUFFIXF") ||
      kw(&L->cur, "REMOVESUFFIXF") || kw(&L->cur, "FIELDSTRIPSUF") ||
      kw(&L->cur, "TRYSTRIPPREFIXF") || kw(&L->cur, "STRIPPREFIXFSOFT") ||
      kw(&L->cur, "SOFTSTRIPPREFIXF") || kw(&L->cur, "TRYSTRIPSUFFIXF") ||
      kw(&L->cur, "STRIPSUFFIXFSOFT") || kw(&L->cur, "SOFTSTRIPSUFFIXF") ||
      kw(&L->cur, "TRYDROPPREFIXF") || kw(&L->cur, "TRYDROPSUFFIXF")) {
    char oname[48], fname[48], op[24], hay[256], affix[256], out[256];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, is_suf = 0, hit = 0;
    size_t hn, an;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "STRIPSUFFIXF") == 0 || strcmp(op, "DROPSUFFIXF") == 0 ||
        strcmp(op, "UNSUFFIXF") == 0 || strcmp(op, "CHOPSUFFIXF") == 0 ||
        strcmp(op, "REMOVESUFFIXF") == 0 || strcmp(op, "FIELDSTRIPSUF") == 0 ||
        strcmp(op, "TRYSTRIPSUFFIXF") == 0 || strcmp(op, "STRIPSUFFIXFSOFT") == 0 ||
        strcmp(op, "SOFTSTRIPSUFFIXF") == 0 || strcmp(op, "TRYDROPSUFFIXF") == 0)
      is_suf = 1;
    if (strcmp(op, "TRYSTRIPPREFIXF") == 0 || strcmp(op, "STRIPPREFIXFSOFT") == 0 ||
        strcmp(op, "SOFTSTRIPPREFIXF") == 0 || strcmp(op, "TRYSTRIPSUFFIXF") == 0 ||
        strcmp(op, "STRIPSUFFIXFSOFT") == 0 || strcmp(op, "SOFTSTRIPSUFFIXF") == 0 ||
        strcmp(op, "TRYDROPPREFIXF") == 0 || strcmp(op, "TRYDROPSUFFIXF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, is_suf ? "STRIPSUFFIXF object field suffix"
                      : "STRIPPREFIXF object field prefix");
      return -1;
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
      fail(vm, is_suf ? "STRIPSUFFIXF field" : "STRIPPREFIXF field");
      return -1;
    }
    if (kw(&L->cur, "OF") || kw(&L->cur, "WITH") || kw(&L->cur, "FROM") ||
        kw(&L->cur, "AS") || kw(&L->cur, "IS"))
      lex_next(L);
    affix[0] = 0;
    if (resolve_str_arg(vm, L, affix, sizeof affix) != 0) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        long v = parse_expr(vm, L);
        snprintf(affix, sizeof affix, "%ld", v);
      } else {
        fail(vm, is_suf ? "STRIPSUFFIXF object field suffix"
                        : "STRIPPREFIXF object field prefix");
        return -1;
      }
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      const char *tag = is_suf ? "STRIPSUFFIXF" : "STRIPPREFIXF";
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s unknown object %s", tag, oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "STRIPPREFIXF_N", 0);
      var_set_num(vm, "STRIPSUFFIXF_N", 0);
      var_set_num(vm, "STRIPPREFIXF_HIT", 0);
      var_set_num(vm, "STRIPSUFFIXF_HIT", 0);
      var_set_num(vm, "OK", 0);
      {
        char ebuf[56];
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
      const char *tag = is_suf ? "STRIPSUFFIXF" : "STRIPPREFIXF";
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s unknown FIELD %s", tag, fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "STRIPPREFIXF_N", 0);
      var_set_num(vm, "STRIPSUFFIXF_N", 0);
      var_set_num(vm, "STRIPPREFIXF_HIT", 0);
      var_set_num(vm, "STRIPSUFFIXF_HIT", 0);
      var_set_num(vm, "OK", 0);
      {
        char ebuf[56];
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
    hn = strlen(hay);
    an = strlen(affix);
    snprintf(out, sizeof out, "%s", hay);
    hit = 0;
    if (an > 0 && an <= hn) {
      if (!is_suf) {
        if (memcmp(hay, affix, an) == 0) {
          snprintf(out, sizeof out, "%s", hay + an);
          hit = 1;
        }
      } else {
        if (memcmp(hay + hn - an, affix, an) == 0) {
          size_t keep = hn - an;
          if (keep >= sizeof out) keep = sizeof out - 1;
          memcpy(out, hay, keep);
          out[keep] = 0;
          hit = 1;
        }
      }
    }
    snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", out);
    ob->fis_str[fi] = 1;
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = (long)strlen(out);
    var_set_num(vm, "LAST_N", vm->last_n);
    if (is_suf) {
      var_set_num(vm, "STRIPSUFFIXF_N", 1);
      var_set_num(vm, "STRIPSUFFIXF_HIT", hit);
      var_set_num(vm, "STRIPPREFIXF_N", 0);
      var_set_num(vm, "STRIPPREFIXF_HIT", 0);
    } else {
      var_set_num(vm, "STRIPPREFIXF_N", 1);
      var_set_num(vm, "STRIPPREFIXF_HIT", hit);
      var_set_num(vm, "STRIPSUFFIXF_N", 0);
      var_set_num(vm, "STRIPSUFFIXF_HIT", 0);
    }
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* WORDSF|TOKENIZEF|FIELDSWF obj field â whitespace tokenize field â bag.
   * TRYWORDSF|WORDSF SOFT â soft miss OK=0 sticky LAST_ERR.
   * Collapses runs of space/tab/CR/LF; trims ends. No mutate on field.
   * LAST = newline bag; LAST_N/WORDSF_N = token count.
   * Usability: free-text notes â EACH LINE without GETF+SYS WORDS
   * (METHOD/THIS; pairs SPLITF for fixed-sep and JOINF rebuild). */
  if (kw(&L->cur, "WORDSF") || kw(&L->cur, "TOKENIZEF") ||
      kw(&L->cur, "FIELDSWF") || kw(&L->cur, "WSSPLITF") ||
      kw(&L->cur, "SPLITWSF") || kw(&L->cur, "WORDLISTF") ||
      kw(&L->cur, "TOKENSF") || kw(&L->cur, "FIELDWORDS") ||
      kw(&L->cur, "TRYWORDSF") || kw(&L->cur, "WORDSFSOFT") ||
      kw(&L->cur, "SOFTWORDSF") || kw(&L->cur, "TRYTOKENIZEF")) {
    char oname[48], fname[48], op[24], hay[256], out[512];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0;
    const char *p;
    size_t olen = 0;
    long ntok = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "TRYWORDSF") == 0 || strcmp(op, "WORDSFSOFT") == 0 ||
        strcmp(op, "SOFTWORDSF") == 0 || strcmp(op, "TRYTOKENIZEF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, "WORDSF object field"); return -1;
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
      fail(vm, "WORDSF field"); return -1;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "WORDSF unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "WORDSF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "WORDSF: unknown object");
      var_set_str(vm, "ERR", "WORDSF: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "WORDSF unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "WORDSF_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "WORDSF: unknown field");
      var_set_str(vm, "ERR", "WORDSF: unknown field");
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi])
      snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
    else
      snprintf(hay, sizeof hay, "%ld", ob->fnum[fi]);
    out[0] = 0;
    olen = 0;
    ntok = 0;
    p = hay;
    while (*p) {
      while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
      if (!*p) break;
      {
        const char *start = p;
        size_t flen;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
        flen = (size_t)(p - start);
        if (ntok > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        if (olen + flen < sizeof out) {
          memcpy(out + olen, start, flen);
          olen += flen;
        } else if (olen < sizeof out - 1) {
          size_t take = sizeof out - 1 - olen;
          memcpy(out + olen, start, take);
          olen += take;
        }
        out[olen] = 0;
        ntok++;
      }
    }
    var_set_str(vm, "LAST", out);
    var_set_str(vm, "WORDSF", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = ntok;
    var_set_num(vm, "LAST_N", ntok);
    var_set_num(vm, "WORDSF_N", ntok);
    var_set_num(vm, "TOKENIZEF_N", ntok);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LINEF|NTHLINEF|BAGLINEF obj field index â 0-based newline bag line peel into field.
   * LINENOF|LINENF|ROWF obj field index â 1-based line peel into field.
   * TRYLINEF|LINEF SOFT â soft miss OK=0 sticky LAST_ERR.
   * On hit: field becomes that line (mutate). On miss: field unchanged, HIT=0.
   * LAST = line (or ""); LAST_N/LINEF_HIT = 0|1; LINEF_I = 0-based index used.
   * Trailing newline does not create an empty last field (matches SYS NTH).
   * Promotes numâstr for walk. Complements CUTF (sep columns) for bag-in-field.
   * Usability: multi-line plate/work bag line without GETF+SYS NTH+SETF
   * (METHOD/THIS; pairs SPLITF/WORDSF/JOINF). */
  if (kw(&L->cur, "LINEF") || kw(&L->cur, "NTHLINEF") ||
      kw(&L->cur, "BAGLINEF") || kw(&L->cur, "GETLINEF") ||
      kw(&L->cur, "FIELDLINE") || kw(&L->cur, "NTHBAGF") ||
      kw(&L->cur, "LINENOF") || kw(&L->cur, "LINENF") ||
      kw(&L->cur, "ROWF") || kw(&L->cur, "ROWLINEF") ||
      kw(&L->cur, "TRYLINEF") || kw(&L->cur, "LINEFSOFT") ||
      kw(&L->cur, "SOFTLINEF") || kw(&L->cur, "TRYLINENOF") ||
      kw(&L->cur, "TRYLINENF") || kw(&L->cur, "TRYNTHLINEF")) {
    char oname[48], fname[48], op[24], hay[256], out[256];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, one_based = 0;
    long want = 0, idx = 0, found = 0, nlines = 0;
    const char *p, *start;
    size_t flen;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "LINENOF") == 0 || strcmp(op, "LINENF") == 0 ||
        strcmp(op, "ROWF") == 0 || strcmp(op, "ROWLINEF") == 0 ||
        strcmp(op, "TRYLINENOF") == 0 || strcmp(op, "TRYLINENF") == 0)
      one_based = 1;
    if (strcmp(op, "TRYLINEF") == 0 || strcmp(op, "LINEFSOFT") == 0 ||
        strcmp(op, "SOFTLINEF") == 0 || strcmp(op, "TRYLINENOF") == 0 ||
        strcmp(op, "TRYLINENF") == 0 || strcmp(op, "TRYNTHLINEF") == 0)
      soft = 1;
    lex_next(L);
    if (!soft && (kw(&L->cur, "SOFT") || kw(&L->cur, "TRY") ||
                  kw(&L->cur, "OPT") || kw(&L->cur, "OPTIONAL"))) {
      soft = 1;
      lex_next(L);
    }
    if (oop_resolve_obj_name(vm, L, oname, sizeof oname) < 0) {
      fail(vm, one_based ? "LINENOF object field index"
                         : "LINEF object field index");
      return -1;
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
      fail(vm, one_based ? "LINENOF field" : "LINEF field"); return -1;
    }
    if (kw(&L->cur, "AT") || kw(&L->cur, "INDEX") || kw(&L->cur, "N") ||
        kw(&L->cur, "LINE") || kw(&L->cur, "ROW"))
      lex_next(L);
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
        L->cur.kind == TK_LPAREN ||
        (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        want = parse_expr(vm, L);
      } else {
        Var *dv = var_get(vm, L->cur.text, 0);
        if (dv && !dv->is_str) {
          want = dv->val;
          lex_next(L);
        } else if (strcmp(L->cur.text, "LAST") == 0) {
          want = vm->last_n;
          lex_next(L);
        } else {
          fail(vm, one_based ? "LINENOF object field index"
                             : "LINEF object field index");
          return -1;
        }
      }
    } else {
      want = one_based ? 1 : 0;
    }
    if (one_based) {
      if (want < 1) want = 1;
      want = want - 1;
    } else {
      if (want < 0) want = 0;
    }
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      const char *tag = one_based ? "LINENOF" : "LINEF";
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s unknown object %s", tag, oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "LINEF_N", 0);
      var_set_num(vm, "LINEF_HIT", 0);
      var_set_num(vm, "LINENOF_N", 0);
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
      const char *tag = one_based ? "LINENOF" : "LINEF";
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "%s unknown FIELD %s", tag, fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "LINEF_N", 0);
      var_set_num(vm, "LINEF_HIT", 0);
      var_set_num(vm, "LINENOF_N", 0);
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
    out[0] = 0;
    found = 0;
    nlines = 0;
    if (hay[0]) {
      nlines = 1;
      for (p = hay; *p; p++) {
        if (*p == '\n' && p[1]) nlines++;
      }
    }
    if (want >= 0 && want < nlines && hay[0]) {
      p = hay;
      for (idx = 0; idx < want; idx++) {
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
      }
      start = p;
      while (*p && *p != '\n') p++;
      flen = (size_t)(p - start);
      if (flen >= sizeof out) flen = sizeof out - 1;
      memcpy(out, start, flen);
      out[flen] = 0;
      found = 1;
    }
    if (found) {
      snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", out);
      ob->fis_str[fi] = 1;
    }
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = found;
    var_set_num(vm, "LAST_N", found);
    var_set_num(vm, "LINEF_N", found);
    var_set_num(vm, "LINEF_HIT", found);
    var_set_num(vm, "LINEF_I", want);
    var_set_num(vm, "LINENOF_N", found);
    var_set_num(vm, "LINENOF_HIT", found);
    var_set_num(vm, "NTHLINEF_N", found);
    var_set_num(vm, "NTHLINEF_HIT", found);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }


  /* ENSUREBAGLINE|BAGENSURE|ADDIFMISSINGBAG obj field line
   * â append exact bag line only if missing (idempotent set membership).
   * ENSUREBAGLINEI â case-insensitive exact membership test.
   * TRYENSUREBAGLINE|ENSUREBAGLINE SOFT â soft miss OK=0 sticky LAST_ERR.
   * LAST = bag after; LAST_N/ENSUREBAGLINE_NEW = 1 if appended, 0 if already present;
   * ENSUREBAGLINE_N = bag line count after. Complements HASBAGLINE/DROPBAGLINE/PUSHF.
   * Not SYS ENSURELINE (file). Usability: flags/work sets without HAS+PUSH IF glue
   * (METHOD/THIS). */
  if (kw(&L->cur, "ENSUREBAGLINE") || kw(&L->cur, "BAGENSURE") ||
      kw(&L->cur, "ADDIFMISSINGBAG") || kw(&L->cur, "ENSUREBAG") ||
      kw(&L->cur, "BAGADDIF") || kw(&L->cur, "ENSURELINEBAG") ||
      kw(&L->cur, "ENSUREBAGLINEI") || kw(&L->cur, "BAGENSUREI") ||
      kw(&L->cur, "TRYENSUREBAGLINE") || kw(&L->cur, "ENSUREBAGLINESOFT") ||
      kw(&L->cur, "SOFTENSUREBAGLINE")) {
    char oname[48], fname[48], op[24], hay[256], needle[128], out[256];
    ObjInst *ob;
    ClassDef *cd;
    int fi, soft = 0, icase = 0;
    long hit = 0, is_new = 0, nfields = 0;
    const char *p, *start;
    size_t nn, flen, blen, llen, o;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "ENSUREBAGLINEI") == 0 || strcmp(op, "BAGENSUREI") == 0)
      icase = 1;
    if (strcmp(op, "TRYENSUREBAGLINE") == 0 || strcmp(op, "ENSUREBAGLINESOFT") == 0 ||
        strcmp(op, "SOFTENSUREBAGLINE") == 0)
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
      fail(vm, "ENSUREBAGLINE object field line"); return -1;
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
      fail(vm, "ENSUREBAGLINE field"); return -1;
    }
    if (kw(&L->cur, "LINE") || kw(&L->cur, "ITEM") || kw(&L->cur, "VALUE") ||
        kw(&L->cur, "WITH") || kw(&L->cur, "IS"))
      lex_next(L);
    needle[0] = 0;
    if (resolve_str_arg(vm, L, needle, sizeof needle) != 0)
      snprintf(needle, sizeof needle, "%s", vm->last_str);
    nn = strlen(needle);
    ob = oop_find_obj(vm, oname);
    if (!ob) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "ENSUREBAGLINE unknown object %s", oname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "ENSUREBAGLINE_NEW", 0);
      var_set_num(vm, "ENSUREBAGLINE_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "ENSUREBAGLINE: unknown object");
      var_set_str(vm, "ERR", "ENSUREBAGLINE: unknown object");
      bump(vm);
      return 1;
    }
    cd = &vm->classes[ob->class_idx];
    fi = oop_field_idx(cd, fname);
    if (fi < 0) {
      if (!soft) {
        snprintf(vm->err, sizeof vm->err, "ENSUREBAGLINE unknown FIELD %s", fname);
        fail(vm, vm->err); return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "ENSUREBAGLINE_NEW", 0);
      var_set_num(vm, "ENSUREBAGLINE_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "ENSUREBAGLINE: unknown field");
      var_set_str(vm, "ERR", "ENSUREBAGLINE: unknown field");
      bump(vm);
      return 1;
    }
    if (ob->fis_str[fi])
      snprintf(hay, sizeof hay, "%s", ob->fstr[fi]);
    else
      snprintf(hay, sizeof hay, "%ld", ob->fnum[fi]);
    hit = 0;
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
    out[0] = 0;
    is_new = 0;
    if (hit) {
      snprintf(out, sizeof out, "%s", hay);
    } else {
      /* append like PUSHF */
      is_new = 1;
      blen = strlen(hay);
      llen = nn;
      o = 0;
      if (blen == 0) {
        if (llen < sizeof out) {
          memcpy(out, needle, llen);
          o = llen;
        } else {
          o = sizeof out - 1;
          memcpy(out, needle, o);
        }
        out[o] = 0;
      } else {
        if (blen < sizeof out) {
          memcpy(out, hay, blen);
          o = blen;
        } else {
          o = sizeof out - 1;
          memcpy(out, hay, o);
        }
        if (o + 1 < sizeof out) out[o++] = '\n';
        if (o + llen < sizeof out) {
          memcpy(out + o, needle, llen);
          o += llen;
        } else if (o < sizeof out - 1) {
          size_t take = sizeof out - 1 - o;
          memcpy(out + o, needle, take);
          o += take;
        }
        out[o] = 0;
      }
    }
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
    nfields = 0;
    if (ob->fstr[fi][0]) {
      p = ob->fstr[fi];
      while (*p) {
        while (*p && *p != '\n') p++;
        nfields++;
        if (*p == '\n') p++;
      }
    }
    var_set_str(vm, "LAST", ob->fstr[fi]);
    var_set_str(vm, "ENSUREBAGLINE", ob->fstr[fi]);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", ob->fstr[fi]);
    vm->last_n = is_new;
    var_set_num(vm, "LAST_N", is_new);
    var_set_num(vm, "ENSUREBAGLINE_NEW", is_new);
    var_set_num(vm, "ENSUREBAGLINE_N", nfields);
    var_set_num(vm, "BAGENSURE_NEW", is_new);
    var_set_str(vm, "FIELD", fname);
    var_set_str(vm, "OBJECT", oname);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }
  return 0;
}
