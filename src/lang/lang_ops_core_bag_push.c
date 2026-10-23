/* CubalC lang — lang_ops_core_bag_push.c
 * Work-bag queue: PUSHTO · UNSHIFTTO · POPFROM · SHIFTFROM · LINESIN · HASINBAG
 * Restored from fa4c3a36c (B_E_science). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_core_bag_push.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int cubalc_lang_ops_core_try_bag_push(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* PUSHTO name line — append newline bag field onto named var (work queue).
   * UNSHIFTTO name line — prepend field (FIFO front).
   * POPFROM name — LIFO peel last field → LAST; write rest back to name.
   * SHIFTFROM name — FIFO peel first field → LAST; write rest back.
   * LINESIN name — field count → LAST_N.
   * HASINBAG name line — exact field membership → LAST_N 0|1.
   * Usability: agent work bags without LET x = SYS PUSH x line glue. */
  if (kw(&L->cur,"PUSHTO")||kw(&L->cur,"BAGPUSHTO")||kw(&L->cur,"ENQUEUETO")||
      kw(&L->cur,"LINEPUSHTO")||kw(&L->cur,"PUSHBAGTO")||
      kw(&L->cur,"UNSHIFTTO")||kw(&L->cur,"PREPENDLINE")||kw(&L->cur,"BAGPREPENDTO")||
      kw(&L->cur,"FRONTBAGTO")||
      kw(&L->cur,"POPFROM")||kw(&L->cur,"BAGPOPFROM")||kw(&L->cur,"POPBAGFROM")||
      kw(&L->cur,"STACKPOP")||kw(&L->cur,"POPLASTFROM")||
      kw(&L->cur,"SHIFTFROM")||kw(&L->cur,"DEQUEUEFROM")||kw(&L->cur,"POPHEADFROM")||
      kw(&L->cur,"SHIFTBAGFROM")||kw(&L->cur,"FIFOPOP")||
      kw(&L->cur,"LINESIN")||kw(&L->cur,"NLINES")||kw(&L->cur,"COUNTLINES")||
      kw(&L->cur,"BAGLINES")||kw(&L->cur,"LINECOUNTV")||
      kw(&L->cur,"HASINBAG")||kw(&L->cur,"BAGHAS")||kw(&L->cur,"HASLINEIN")||
      kw(&L->cur,"CONTAINSLINE")||kw(&L->cur,"BAGCONTAINS")){
    char op[24], name[48];
    int mode = 0; /* 0 push, 1 unshift, 2 pop, 3 shift, 4 lines, 5 has */
    char bag[CUBALC_HOST_STR_MAX], line[CUBALC_HOST_STR_MAX];
    char out[CUBALC_HOST_STR_MAX];
    Var *vv;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "UNSHIFTTO") == 0 || strcmp(op, "PREPENDLINE") == 0 ||
        strcmp(op, "BAGPREPENDTO") == 0 || strcmp(op, "FRONTBAGTO") == 0)
      mode = 1;
    else if (strcmp(op, "POPFROM") == 0 || strcmp(op, "BAGPOPFROM") == 0 ||
             strcmp(op, "POPBAGFROM") == 0 || strcmp(op, "STACKPOP") == 0 ||
             strcmp(op, "POPLASTFROM") == 0)
      mode = 2;
    else if (strcmp(op, "SHIFTFROM") == 0 || strcmp(op, "DEQUEUEFROM") == 0 ||
             strcmp(op, "POPHEADFROM") == 0 || strcmp(op, "SHIFTBAGFROM") == 0 ||
             strcmp(op, "FIFOPOP") == 0)
      mode = 3;
    else if (strcmp(op, "LINESIN") == 0 || strcmp(op, "NLINES") == 0 ||
             strcmp(op, "COUNTLINES") == 0 || strcmp(op, "BAGLINES") == 0 ||
             strcmp(op, "LINECOUNTV") == 0)
      mode = 4;
    else if (strcmp(op, "HASINBAG") == 0 || strcmp(op, "BAGHAS") == 0 ||
             strcmp(op, "HASLINEIN") == 0 || strcmp(op, "CONTAINSLINE") == 0 ||
             strcmp(op, "BAGCONTAINS") == 0)
      mode = 5;
    else
      mode = 0; /* PUSHTO */
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, mode <= 1 ? "PUSHTO needs name line — PUSHTO work \"job\""
           : mode <= 3 ? "POPFROM needs name — POPFROM work"
           : mode == 4 ? "LINESIN needs name — LINESIN work"
                       : "HASINBAG needs name line — HASINBAG work \"job\"");
      return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    bag[0] = 0;
    vv = var_get(vm, name, 0);
    if (vv && vv->is_str)
      snprintf(bag, sizeof bag, "%s", vv->sval);
    else if (vv && !vv->is_str)
      snprintf(bag, sizeof bag, "%ld", vv->val);

    if (mode == 0 || mode == 1) {
      size_t blen, llen, o;
      long nfields = 0;
      const char *p;
      if (kw(&L->cur, "WITH") || kw(&L->cur, "BY") || kw(&L->cur, "AND"))
        lex_next(L);
      if (resolve_str_arg(vm, L, line, sizeof line) != 0) {
        fail(vm, mode == 1 ? "UNSHIFTTO needs line — UNSHIFTTO work \"job\""
                           : "PUSHTO needs line — PUSHTO work \"job\"");
        return -1;
      }
      blen = strlen(bag);
      llen = strlen(line);
      out[0] = 0;
      o = 0;
      if (mode == 1) {
        /* line + \n + bag */
        if (llen < sizeof out) { memcpy(out, line, llen); o = llen; }
        else { o = sizeof out - 1; memcpy(out, line, o); }
        out[o] = 0;
        if (blen > 0) {
          if (o + 1 < sizeof out) out[o++] = '\n';
          if (o + blen < sizeof out) { memcpy(out + o, bag, blen); o += blen; }
          else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, bag, take); o += take;
          }
          out[o] = 0;
        }
      } else {
        /* bag + \n + line */
        if (blen == 0) {
          if (llen < sizeof out) { memcpy(out, line, llen); o = llen; }
          else { o = sizeof out - 1; memcpy(out, line, o); }
          out[o] = 0;
        } else {
          if (blen < sizeof out) { memcpy(out, bag, blen); o = blen; }
          else { o = sizeof out - 1; memcpy(out, bag, o); }
          if (o + 1 < sizeof out) out[o++] = '\n';
          if (o + llen < sizeof out) { memcpy(out + o, line, llen); o += llen; }
          else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, line, take); o += take;
          }
          out[o] = 0;
        }
      }
      p = out;
      nfields = 0;
      if (out[0]) {
        while (*p) {
          while (*p && *p != '\n') p++;
          nfields++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, name, out);
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      var_set_num(vm, "LAST_N", nfields);
      vm->last_n = nfields;
      var_set_num(vm, mode == 1 ? "UNSHIFTTO_N" : "PUSHTO_N", nfields);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# %s %s n=%ld\n", mode == 1 ? "UNSHIFTTO" : "PUSHTO",
                name, nfields);
      bump(vm);
      return 1;
    }

    if (mode == 2 || mode == 3) {
      char field[CUBALC_HOST_STR_MAX], rest[CUBALC_HOST_STR_MAX];
      long found = 0, rest_n = 0;
      const char *p;
      field[0] = rest[0] = 0;
      if (!bag[0]) {
        found = 0;
      } else if (mode == 2) {
        /* LIFO: last field */
        const char *last_nl = NULL;
        for (p = bag; *p; p++)
          if (*p == '\n') last_nl = p;
        if (!last_nl) {
          snprintf(field, sizeof field, "%s", bag);
          rest[0] = 0;
          found = 1;
          rest_n = 0;
        } else {
          snprintf(field, sizeof field, "%s", last_nl + 1);
          {
            size_t rlen = (size_t)(last_nl - bag);
            if (rlen >= sizeof rest) rlen = sizeof rest - 1;
            memcpy(rest, bag, rlen);
            rest[rlen] = 0;
          }
          found = 1;
          if (rest[0]) {
            p = rest;
            while (*p) {
              while (*p && *p != '\n') p++;
              rest_n++;
              if (*p == '\n') p++;
            }
          }
        }
      } else {
        /* FIFO: first field */
        size_t flen;
        p = bag;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - bag);
        if (flen >= sizeof field) flen = sizeof field - 1;
        memcpy(field, bag, flen);
        field[flen] = 0;
        found = 1;
        if (*p == '\n') {
          snprintf(rest, sizeof rest, "%s", p + 1);
          if (rest[0]) {
            const char *q = rest;
            while (*q) {
              while (*q && *q != '\n') q++;
              rest_n++;
              if (*q == '\n') q++;
            }
          }
        } else {
          rest[0] = 0;
          rest_n = 0;
        }
      }
      var_set_str(vm, name, rest);
      var_set_str(vm, "LAST", field);
      var_set_str(vm, "POP_REST", rest);
      var_set_str(vm, "REST", rest);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", field);
      var_set_num(vm, "LAST_N", found);
      vm->last_n = found;
      var_set_num(vm, mode == 2 ? "POPFROM_N" : "SHIFTFROM_N", rest_n);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# %s %s found=%ld rest_n=%ld\n",
                mode == 2 ? "POPFROM" : "SHIFTFROM", name, found, rest_n);
      bump(vm);
      return 1;
    }

    if (mode == 4) {
      long n = 0;
      const char *p = bag;
      if (bag[0]) {
        while (*p) {
          while (*p && *p != '\n') p++;
          n++;
          if (*p == '\n') p++;
        }
      }
      var_set_num(vm, "LAST_N", n);
      vm->last_n = n;
      {
        char nb[32];
        snprintf(nb, sizeof nb, "%ld", n);
        var_set_str(vm, "LAST", nb);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
      }
      var_set_num(vm, "LINESIN_N", n);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# LINESIN %s -> %ld\n", name, n);
      bump(vm);
      return 1;
    }

    /* mode 5 HASINBAG */
    {
      long hit = 0;
      const char *p, *start;
      if (kw(&L->cur, "WITH") || kw(&L->cur, "OF") || kw(&L->cur, "FOR"))
        lex_next(L);
      if (resolve_str_arg(vm, L, line, sizeof line) != 0) {
        fail(vm, "HASINBAG needs line — HASINBAG work \"job\"");
        return -1;
      }
      p = bag;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        {
          size_t fl = (size_t)(p - start);
          if (fl == strlen(line) && (fl == 0 || memcmp(start, line, fl) == 0)) {
            hit = 1;
            break;
          }
        }
        if (*p == '\n') p++;
      }
      var_set_num(vm, "LAST_N", hit);
      vm->last_n = hit;
      var_set_str(vm, "LAST", hit ? "1" : "0");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hit ? "1" : "0");
      var_set_num(vm, "HASINBAG_N", hit);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# HASINBAG %s -> %ld\n", name, hit);
      bump(vm);
      return 1;
    }
  }

  return 0;
}
