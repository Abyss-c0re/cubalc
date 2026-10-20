/* CubalC lang — lang_ops_flow_oop.c
 * OOP + flow CLI helpers (mesh vote A3_carve_lang_ops_flow).
 */
#include "lang/cubalc_lang_ops_flow_oop.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/* CLI flag/arg probes for IFHASFLAG/IFHASARG (flow plane).
 * Mirrors lang_ops_core HASFLAG/HASARG without cross-TU static dependency. */
int mi_scan_cli_flag(const char *name, char *out, size_t outn) {
  char envn[32];
  const char *ac;
  int argc = 32, k;
  size_t nlen;
  if (!name || !name[0]) return 0;
  while (name[0] == '-') name++;
  if (!name[0]) return 0;
  nlen = strlen(name);
  ac = getenv("CUBALC_ARGC");
  if (ac && ac[0]) {
    argc = (int)strtol(ac, NULL, 10);
    if (argc < 0) argc = 0;
    if (argc > 32) argc = 32;
  } else {
    argc = 0;
    for (k = 0; k < 32; k++) {
      snprintf(envn, sizeof envn, "CUBALC_ARG%d", k);
      if (!getenv(envn)) break;
      argc++;
    }
  }
  for (k = 0; k < argc; k++) {
    const char *a, *p;
    size_t al;
    snprintf(envn, sizeof envn, "CUBALC_ARG%d", k);
    a = getenv(envn);
    if (!a || a[0] != '-') continue;
    p = a + 1;
    if (*p == '-') p++;
    if (!*p) continue;
    al = strlen(p);
    if (al >= nlen && strncmp(p, name, nlen) == 0) {
      if (p[nlen] == '=') {
        if (out && outn) snprintf(out, outn, "%s", p + nlen + 1);
        return 2;
      }
      if (p[nlen] == 0) {
        if (k + 1 < argc) {
          char env2[32];
          const char *b;
          snprintf(env2, sizeof env2, "CUBALC_ARG%d", k + 1);
          b = getenv(env2);
          if (b && b[0] && b[0] != '-') {
            if (out && outn) snprintf(out, outn, "%s", b);
            return 2;
          }
        }
        if (out && outn) snprintf(out, outn, "1");
        return 1;
      }
    }
  }
  if (out && outn) out[0] = 0;
  return 0;
}

int mi_read_flag_aliases(Lex *L, char *bag, size_t bagn,
                                char *first, size_t firstn) {
  int n = 0;
  if (bag && bagn) bag[0] = 0;
  if (first && firstn) first[0] = 0;
  for (;;) {
    char name[96];
    size_t bl;
    while (L->cur.kind == TK_MINUS) lex_next(L);
    if (L->cur.kind != TK_STR && L->cur.kind != TK_IDENT)
      break;
    if (n > 0 && (kw(&L->cur, "OR") || kw(&L->cur, "DEFAULT") ||
                  kw(&L->cur, "ELSE") || kw(&L->cur, "FALLBACK") ||
                  kw(&L->cur, "THEN") || kw(&L->cur, "END") ||
                  kw(&L->cur, "IF") || kw(&L->cur, "LET")))
      break;
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    while (name[0] == '-')
      memmove(name, name + 1, strlen(name));
    if (!name[0]) {
      if (L->cur.kind == TK_COMMA || L->cur.kind == TK_PIPE) {
        lex_next(L);
        continue;
      }
      break;
    }
    if (n == 0 && first && firstn)
      snprintf(first, firstn, "%s", name);
    if (bag && bagn) {
      bl = strlen(bag);
      if (bl > 0 && bl + 1 < bagn) {
        bag[bl++] = '\n';
        bag[bl] = 0;
      }
      if (bl + strlen(name) < bagn)
        snprintf(bag + bl, bagn - bl, "%s", name);
    }
    n++;
    if (L->cur.kind == TK_COMMA || L->cur.kind == TK_PIPE) {
      lex_next(L);
      continue;
    }
    break;
  }
  return n;
}

int mi_scan_cli_flag_any(const char *names_bag, char *out, size_t outn,
                                char *hit_name, size_t hitn) {
  const char *p = names_bag;
  if (hit_name && hitn) hit_name[0] = 0;
  if (!p || !p[0]) {
    if (out && outn) out[0] = 0;
    return 0;
  }
  while (*p) {
    char name[96];
    size_t i = 0;
    int hit;
    while (*p && *p != '\n' && i + 1 < sizeof name)
      name[i++] = *p++;
    name[i] = 0;
    if (*p == '\n') p++;
    if (!name[0]) continue;
    hit = mi_scan_cli_flag(name, out, outn);
    if (hit) {
      if (hit_name && hitn)
        snprintf(hit_name, hitn, "%s", name);
      return hit;
    }
  }
  if (out && outn) out[0] = 0;
  return 0;
}

ClassDef *oop_find_class(VM *vm, const char *name){
  int i;
  for (i = 0; i < vm->n_classes; i++)
    if (strcmp(vm->classes[i].name, name) == 0) return &vm->classes[i];
  return NULL;
}
int oop_find_class_idx(VM *vm, const char *name){
  int i;
  for (i = 0; i < vm->n_classes; i++)
    if (strcmp(vm->classes[i].name, name) == 0) return i;
  return -1;
}
ObjInst *oop_find_obj(VM *vm, const char *name){
  int i;
  for (i = 0; i < vm->n_objs; i++)
    if (vm->objs[i].live && strcmp(vm->objs[i].name, name) == 0)
      return &vm->objs[i];
  return NULL;
}
int oop_field_idx(ClassDef *cd, const char *fname){
  int i;
  for (i = 0; i < cd->n_fields; i++)
    if (strcmp(cd->fields[i].name, fname) == 0) return i;
  return -1;
}

int oop_field_def_same(const FieldDef *a, const FieldDef *b){
  if (!a || !b) return 0;
  if (a->is_str != b->is_str || a->has_def != b->has_def) return 0;
  if (!a->has_def) return 1;
  if (a->is_str) return strcmp(a->def_str, b->def_str) == 0;
  return a->def_num == b->def_num;
}
MethodDef *oop_find_method(ClassDef *cd, const char *mname){
  int i;
  for (i = 0; i < cd->n_methods; i++)
    if (strcmp(cd->methods[i].name, mname) == 0) return &cd->methods[i];
  return NULL;
}

/* Copy parent fields/methods into child (flatten). Child may override methods
 * by same name later. parent_idx kept for ISOF chain. Returns 0 ok, -1 fail. */
int oop_inherit_flatten(VM *vm, ClassDef *cd, ClassDef *base){
  int i, j;
  if (!cd || !base) return -1;
  for (i = 0; i < base->n_fields; i++) {
    FieldDef *pf = &base->fields[i];
    int hit = 0;
    for (j = 0; j < cd->n_fields; j++) {
      if (strcmp(cd->fields[j].name, pf->name) == 0) { hit = 1; break; }
    }
    if (hit) continue;
    if (cd->n_fields >= CUBALC_MAX_FIELDS) {
      fail(vm, "EXTEND too many FIELD from base");
      return -1;
    }
    cd->fields[cd->n_fields++] = *pf;
  }
  for (i = 0; i < base->n_methods; i++) {
    MethodDef *pm = &base->methods[i];
    int hit = 0;
    for (j = 0; j < cd->n_methods; j++) {
      if (strcmp(cd->methods[j].name, pm->name) == 0) { hit = 1; break; }
    }
    if (hit) continue;
    if (cd->n_methods >= CUBALC_MAX_METHODS) {
      fail(vm, "EXTEND too many METHOD from base");
      return -1;
    }
    cd->methods[cd->n_methods++] = *pm;
  }
  if (base->role[0] && (!cd->role[0] || strcmp(cd->role, "body") == 0))
    snprintf(cd->role, sizeof cd->role, "%s", base->role);
  return 0;
}

/* After EXTEND reopen, push new FIELD/METHOD into CLASS EXTEND descendants.
 * Multi-file: unit C EXTEND Base after Child defined → Child gains methods. */
int oop_propagate_extend(VM *vm, int src_idx){
  int pass, i, gained_classes = 0;
  int touched[CUBALC_MAX_CLASSES];
  (void)src_idx;
  if (!vm) return -1;
  for (i = 0; i < CUBALC_MAX_CLASSES; i++) touched[i] = 0;
  for (pass = 0; pass < CUBALC_MAX_CLASSES; pass++) {
    int any = 0;
    for (i = 0; i < vm->n_classes; i++) {
      ClassDef *d = &vm->classes[i];
      ClassDef *p;
      int bf, bm;
      if (d->parent_idx < 0 || d->parent_idx >= vm->n_classes) continue;
      p = &vm->classes[d->parent_idx];
      bf = d->n_fields;
      bm = d->n_methods;
      if (oop_inherit_flatten(vm, d, p) < 0) return -1;
      if (d->n_fields != bf || d->n_methods != bm) {
        any = 1;
        if (!touched[i]) { touched[i] = 1; gained_classes++; }
      }
    }
    if (!any) break;
  }
  return gained_classes;
}

/* ISOF chain: obj class or any parent matches cname */
int oop_isof_chain(VM *vm, int class_idx, const char *cname){
  int guard = 0;
  while (class_idx >= 0 && class_idx < vm->n_classes && guard++ < CUBALC_MAX_CLASSES) {
    ClassDef *cd = &vm->classes[class_idx];
    if (strcmp(cd->name, cname) == 0) return 1;
    class_idx = cd->parent_idx;
  }
  return 0;
}

/* Shared FIELD/METHOD/ROLE body parser for CLASS and EXTEND reopen.
 * Returns 0 ok, -1 fail. Stops after consuming END. */
int oop_parse_class_body(VM *vm, Lex *L, ClassDef *cd, const char *cname){
  for (;;) {
    skip_nl(L);
    if (L->cur.kind == TK_EOF) {
      snprintf(vm->err, sizeof vm->err, "CLASS/EXTEND %s without END", cname);
      fail_at(vm, L, vm->err);
      return -1;
    }
    if (kw(&L->cur, "END")) { lex_next(L); break; }
    if (kw(&L->cur, "FIELD") || kw(&L->cur, "VAR") || kw(&L->cur, "PROP") ||
        kw(&L->cur, "MEMBER") || kw(&L->cur, "COMPONENT") ||
        kw(&L->cur, "ATTR")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT) { fail(vm, "FIELD name"); return -1; }
      {
        char fname[32];
        int fi = -1, i;
        snprintf(fname, sizeof fname, "%s", L->cur.text);
        lex_next(L);
        for (i = 0; i < cd->n_fields; i++) {
          if (strcmp(cd->fields[i].name, fname) == 0) { fi = i; break; }
        }
        if (fi < 0) {
          if (cd->n_fields >= CUBALC_MAX_FIELDS) {
            fail(vm, "too many FIELD"); return -1;
          }
          fi = cd->n_fields++;
          memset(&cd->fields[fi], 0, sizeof cd->fields[fi]);
          snprintf(cd->fields[fi].name, sizeof cd->fields[fi].name, "%s", fname);
        }
        {
          FieldDef *fd = &cd->fields[fi];
          if (L->cur.kind == TK_EQ) lex_next(L);
          if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
              L->cur.kind == TK_LPAREN) {
            fd->def_num = parse_expr(vm, L);
            fd->has_def = 1;
            fd->is_str = 0;
          } else if (L->cur.kind == TK_STR) {
            snprintf(fd->def_str, sizeof fd->def_str, "%s", L->cur.text);
            fd->is_str = 1;
            fd->has_def = 1;
            lex_next(L);
          } else if (L->cur.kind == TK_IDENT && !kw(&L->cur, "FIELD") &&
                     !kw(&L->cur, "METHOD") && !kw(&L->cur, "END") &&
                     !kw(&L->cur, "ROLE") && !kw(&L->cur, "VAR") &&
                     !kw(&L->cur, "PROP") && !kw(&L->cur, "FN") &&
                     !kw(&L->cur, "ATTR") && !kw(&L->cur, "MEMBER")) {
            snprintf(fd->def_str, sizeof fd->def_str, "%s", L->cur.text);
            fd->is_str = 1;
            fd->has_def = 1;
            lex_next(L);
          }
        }
      }
      continue;
    }
    if (kw(&L->cur, "ROLE")) {
      lex_next(L);
      if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
        snprintf(cd->role, sizeof cd->role, "%s", L->cur.text);
        lex_next(L);
      }
      continue;
    }
    if (kw(&L->cur, "METHOD") || kw(&L->cur, "FN") || kw(&L->cur, "FUNC") ||
        kw(&L->cur, "DEF") || kw(&L->cur, "UPDATE") || kw(&L->cur, "HANDLER")) {
      lex_next(L);
      if (L->cur.kind != TK_IDENT) { fail(vm, "METHOD name"); return -1; }
      {
        char mname[32];
        int mi = -1, i;
        MethodDef *md;
        size_t b0, b1;
        int depth = 1;
        snprintf(mname, sizeof mname, "%s", L->cur.text);
        lex_next(L);
        for (i = 0; i < cd->n_methods; i++) {
          if (strcmp(cd->methods[i].name, mname) == 0) { mi = i; break; }
        }
        if (mi < 0) {
          if (cd->n_methods >= CUBALC_MAX_METHODS) {
            fail(vm, "too many METHOD"); return -1;
          }
          mi = cd->n_methods++;
          memset(&cd->methods[mi], 0, sizeof cd->methods[mi]);
        } else {
          memset(&cd->methods[mi], 0, sizeof cd->methods[mi]);
        }
        md = &cd->methods[mi];
        snprintf(md->name, sizeof md->name, "%s", mname);
        while (md->n_params < 8 && L->cur.kind == TK_IDENT &&
               !kw(&L->cur, "END") && !kw(&L->cur, "THEN")) {
          snprintf(md->params[md->n_params], sizeof md->params[0], "%s",
                   L->cur.text);
          md->n_params++;
          lex_next(L);
        }
        skip_nl(L);
        b0 = L->tok_off;
        while (L->cur.kind != TK_EOF) {
          if (block_scan_step(L, &depth, 0)) break;
        }
        if (depth != 0) { fail(vm, "METHOD without END"); return -1; }
        b1 = L->tok_off;
        if (b1 < b0) b1 = b0;
        md->body = L->s + b0;
        md->len = b1 - b0;
        if (kw(&L->cur, "END")) lex_next(L);
      }
      continue;
    }
    snprintf(vm->err, sizeof vm->err, "CLASS/EXTEND %s unknown form '%s'", cname,
             L->cur.text[0] ? L->cur.text : "?");
    fail(vm, vm->err);
    return -1;
  }
  return 0;
}
/* WHEN multi-alias numeric atom: prim + * / % + - without AND/OR
 * so `WHEN 4 OR 5 OR 6` is three arms, not (4||5||6). */
long case_when_num_atom(VM *vm, Lex *L){
  long v = parse_prim(vm, L);
  for (;;) {
    if (L->cur.kind == TK_STAR) {
      lex_next(L);
      v *= parse_prim(vm, L);
    } else if (L->cur.kind == TK_SLASH) {
      long d;
      lex_next(L);
      d = parse_prim(vm, L);
      v = d ? v / d : 0;
    } else if (L->cur.kind == TK_PERCENT) {
      long d;
      lex_next(L);
      d = parse_prim(vm, L);
      v = d ? v % d : 0;
    } else break;
  }
  for (;;) {
    if (L->cur.kind == TK_PLUS) {
      long t;
      lex_next(L);
      t = parse_prim(vm, L);
      for (;;) {
        if (L->cur.kind == TK_STAR) {
          lex_next(L);
          t *= parse_prim(vm, L);
        } else if (L->cur.kind == TK_SLASH) {
          long d;
          lex_next(L);
          d = parse_prim(vm, L);
          t = d ? t / d : 0;
        } else if (L->cur.kind == TK_PERCENT) {
          long d;
          lex_next(L);
          d = parse_prim(vm, L);
          t = d ? t % d : 0;
        } else break;
      }
      v += t;
    } else if (L->cur.kind == TK_MINUS) {
      long t;
      lex_next(L);
      t = parse_prim(vm, L);
      for (;;) {
        if (L->cur.kind == TK_STAR) {
          lex_next(L);
          t *= parse_prim(vm, L);
        } else if (L->cur.kind == TK_SLASH) {
          long d;
          lex_next(L);
          d = parse_prim(vm, L);
          t = d ? t / d : 0;
        } else if (L->cur.kind == TK_PERCENT) {
          long d;
          lex_next(L);
          d = parse_prim(vm, L);
          t = d ? t % d : 0;
        } else break;
      }
      v -= t;
    } else break;
  }
  return v;
}

/* --- name did-you-mean: agent typo recovery for OOP/FN plane --- */
static void oop_fold(char *dst, size_t n, const char *src){
  size_t i;
  for (i = 0; i + 1 < n && src[i]; i++) {
    char c = src[i];
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    dst[i] = c;
  }
  dst[i] = 0;
}
/* Tiny Levenshtein (capped) â same idea as form_suggest in lang_parse.c */
static int oop_edit_dist(const char *a, const char *b){
  int na = (int)strlen(a), nb = (int)strlen(b);
  int i, j, prev, cur, *row, stack[64];
  if (na > 48) na = 48;
  if (nb > 48) nb = 48;
  if (na + 1 > 64) return 99;
  row = stack;
  for (j = 0; j <= nb; j++) row[j] = j;
  for (i = 1; i <= na; i++) {
    prev = row[0];
    row[0] = i;
    for (j = 1; j <= nb; j++) {
      cur = row[j];
      if (a[i - 1] == b[j - 1])
        row[j] = prev;
      else {
        int ins = row[j - 1] + 1;
        int del = row[j] + 1;
        int sub = prev + 1;
        int m = ins < del ? ins : del;
        row[j] = m < sub ? m : sub;
      }
      prev = cur;
    }
  }
  return row[nb];
}
/* Prefer case-insensitive exact, then prefix (lenâ¥3), then edit dist â¤2. */
static void oop_suggest_from(const char *typo, char *out, size_t outn,
                             const char **names, int n){
  char want[64], cand[64];
  int i, best_d = 99, d;
  const char *best = NULL;
  out[0] = 0;
  if (!typo || !typo[0] || outn < 2 || n <= 0) return;
  oop_fold(want, sizeof want, typo);
  if (!want[0]) return;
  for (i = 0; i < n; i++) {
    if (!names[i] || !names[i][0]) continue;
    oop_fold(cand, sizeof cand, names[i]);
    if (strcmp(want, cand) == 0) {
      snprintf(out, outn, "%s", names[i]);
      return;
    }
  }
  if (strlen(want) >= 3) {
    for (i = 0; i < n; i++) {
      size_t cl;
      if (!names[i] || !names[i][0]) continue;
      oop_fold(cand, sizeof cand, names[i]);
      cl = strlen(cand);
      if (strncmp(want, cand, strlen(want)) == 0 ||
          (cl >= 3 && strncmp(cand, want, cl) == 0)) {
        if (!best || cl < strlen(best))
          best = names[i];
      }
    }
    if (best) {
      snprintf(out, outn, "%s", best);
      return;
    }
  }
  for (i = 0; i < n; i++) {
    if (!names[i] || !names[i][0]) continue;
    oop_fold(cand, sizeof cand, names[i]);
    d = oop_edit_dist(want, cand);
    if (d < best_d) {
      best_d = d;
      best = names[i];
    }
  }
  if (best && best_d <= 2)
    snprintf(out, outn, "%s", best);
}
void oop_suggest_class(VM *vm, const char *typo, char *out, size_t outn){
  const char *names[CUBALC_MAX_CLASSES];
  int i, n = 0;
  for (i = 0; i < vm->n_classes && n < CUBALC_MAX_CLASSES; i++)
    names[n++] = vm->classes[i].name;
  oop_suggest_from(typo, out, outn, names, n);
}
void oop_suggest_obj(VM *vm, const char *typo, char *out, size_t outn){
  const char *names[CUBALC_MAX_OBJS];
  int i, n = 0;
  for (i = 0; i < vm->n_objs && n < CUBALC_MAX_OBJS; i++)
    if (vm->objs[i].live)
      names[n++] = vm->objs[i].name;
  oop_suggest_from(typo, out, outn, names, n);
}
void oop_suggest_method(ClassDef *cd, const char *typo, char *out, size_t outn){
  const char *names[CUBALC_MAX_METHODS];
  int i, n = 0;
  if (!cd) { out[0] = 0; return; }
  for (i = 0; i < cd->n_methods && n < CUBALC_MAX_METHODS; i++)
    names[n++] = cd->methods[i].name;
  oop_suggest_from(typo, out, outn, names, n);
}
void oop_suggest_field(ClassDef *cd, const char *typo, char *out, size_t outn){
  const char *names[CUBALC_MAX_FIELDS];
  int i, n = 0;
  if (!cd) { out[0] = 0; return; }
  for (i = 0; i < cd->n_fields && n < CUBALC_MAX_FIELDS; i++)
    names[n++] = cd->fields[i].name;
  oop_suggest_from(typo, out, outn, names, n);
}
void oop_suggest_fn(VM *vm, const char *typo, char *out, size_t outn){
  const char *names[CUBALC_MAX_FNS];
  int i, n = 0;
  for (i = 0; i < vm->n_fns && n < CUBALC_MAX_FNS; i++)
    names[n++] = vm->fns[i].name;
  oop_suggest_from(typo, out, outn, names, n);
}
/* Build base[+ " â did you mean X?"] into buf (never pass vm->err as msg+dst). */
void oop_err_suggest(char *buf, size_t n, const char *base, const char *sug){
  if (sug && sug[0])
    snprintf(buf, n, "%s â did you mean %s?", base, sug);
  else
    snprintf(buf, n, "%s", base);
}

int oop_resolve_obj_name(VM *vm, Lex *L, char *out, size_t outn){
  if (L->cur.kind == TK_STR) {
    if (!L->cur.text[0]) return -1;
    snprintf(out, outn, "%s", L->cur.text);
    lex_next(L);
    return 0;
  }
  if (L->cur.kind != TK_IDENT) return -1;
  if (strcasecmp(L->cur.text, "THIS") == 0 || strcasecmp(L->cur.text, "SELF") == 0) {
    if (!vm->this_obj[0]) return -1;
    snprintf(out, outn, "%s", vm->this_obj);
    lex_next(L);
    return 0;
  }
  /* LAST / string-var as object name when value is a *live* object.
   * NEW stores var(name)=Class as string â do NOT expand that pollution;
   * only expand when sval names a live instance (EACH OBJ bind, LET peer=â¦). */
  if (strcmp(L->cur.text, "LAST") == 0) {
    if (vm->last_str[0] && oop_find_obj(vm, vm->last_str)) {
      snprintf(out, outn, "%s", vm->last_str);
      lex_next(L);
      return 0;
    }
  } else {
    Var *sv = var_get(vm, L->cur.text, 0);
    if (sv && sv->is_str && sv->sval[0] && oop_find_obj(vm, sv->sval)) {
      snprintf(out, outn, "%s", sv->sval);
      lex_next(L);
      return 0;
    }
  }
  snprintf(out, outn, "%s", L->cur.text);
  lex_next(L);
  return 0;
}
int oop_stmt_kw(Lex *L){
  return L->cur.kind == TK_IDENT &&
         (kw(&L->cur, "END") || kw(&L->cur, "LET") || kw(&L->cur, "ASSERT") ||
          kw(&L->cur, "PRINT") || kw(&L->cur, "IF") || kw(&L->cur, "FOR") ||
          kw(&L->cur, "WHILE") || kw(&L->cur, "CALL") || kw(&L->cur, "SEND") ||
          kw(&L->cur, "RET") || kw(&L->cur, "RETURN") || kw(&L->cur, "CUBE") ||
          kw(&L->cur, "SYS") || kw(&L->cur, "CLASS") || kw(&L->cur, "NEW") ||
          kw(&L->cur, "GETF") || kw(&L->cur, "SETF") || kw(&L->cur, "METHOD") ||
          kw(&L->cur, "FIELD") || kw(&L->cur, "ENTITY") || kw(&L->cur, "SPAWN") ||
          kw(&L->cur, "TICK") || kw(&L->cur, "SCENE") || kw(&L->cur, "FLOW") ||
          kw(&L->cur, "PLUG") || kw(&L->cur, "TYPE") ||
          /* OR/DEFAULT/ELSE/FALLBACK: end of arg lists for CALL/GETF duals */
          kw(&L->cur, "OR") || kw(&L->cur, "DEFAULT") || kw(&L->cur, "ELSE") ||
          kw(&L->cur, "FALLBACK") || kw(&L->cur, "ELIF"));
}
/* Parse optional OR|DEFAULT|ELSE|FALLBACK value after CALL/TRYCALL args.
 * Returns 1 if present (fb filled), 0 if none. Leaves cursor after fallback. */
void call_apply_or_last(VM *vm, const char *fb){
  if (!fb) fb = "";
  if (fb[0] && (fb[0] == '-' || (fb[0] >= '0' && fb[0] <= '9'))) {
    char *end = NULL;
    long v = strtol(fb, &end, 10);
    if (end && end != fb && !*end) {
      var_set_num(vm, "LAST_N", v);
      vm->last_n = v;
      var_set_str(vm, "LAST", fb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", fb);
      return;
    }
  }
  var_set_str(vm, "LAST", fb);
  snprintf(vm->last_str, sizeof vm->last_str, "%s", fb);
  var_set_num(vm, "LAST_N", 0);
  vm->last_n = 0;
}

int call_parse_or_fallback(VM *vm, Lex *L, char *fb, size_t fbn){
  fb[0] = 0;
  if (!(kw(&L->cur, "OR") || kw(&L->cur, "DEFAULT") || kw(&L->cur, "ELSE") ||
        kw(&L->cur, "FALLBACK")))
    return 0;
  lex_next(L);
  if (L->cur.kind == TK_STR) {
    snprintf(fb, fbn, "%s", L->cur.text);
    lex_next(L);
    return 1;
  }
  if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
      L->cur.kind == TK_LPAREN ||
      (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))) {
    long v;
    if (L->cur.kind == TK_IDENT) {
      Var *sv = var_get(vm, L->cur.text, 0);
      if (sv && sv->is_str) {
        snprintf(fb, fbn, "%s", sv->sval);
        lex_next(L);
        return 1;
      }
      if (strcmp(L->cur.text, "LAST") == 0) {
        snprintf(fb, fbn, "%s", vm->last_str);
        lex_next(L);
        return 1;
      }
    }
    v = parse_expr(vm, L);
    snprintf(fb, fbn, "%ld", v);
    return 1;
  }
  fail(vm, "CALL â¦ OR fallback â TRYCALL greeter name OR \"hi\"");
  return -1;
}

int oop_bind_args(VM *vm, Lex *L, char params[][32], int n_params){
  int ai = 0;
  while (ai < 8 &&
         (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT || L->cur.kind == TK_STR ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN)) {
    char an[16];
    long v;
    snprintf(an, sizeof an, "ARG%d", ai);
    if (oop_stmt_kw(L)) break; /* OR/DEFAULT/ELSE end of arg list (CALL â¦ OR fb) */
    if (L->cur.kind == TK_STR) {
      var_set_str(vm, an, L->cur.text);
      if (ai < n_params && params[ai][0])
        var_set_str(vm, params[ai], L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !oop_stmt_kw(L)) {
      /* String formals: bind string vars / LAST by value (not strlen via parse_expr). */
      if (strcmp(L->cur.text, "LAST") == 0) {
        var_set_str(vm, an, vm->last_str);
        if (ai < n_params && params[ai][0])
          var_set_str(vm, params[ai], vm->last_str);
        lex_next(L);
      } else {
        Var *sv = var_get(vm, L->cur.text, 0);
        if (sv && sv->is_str) {
          var_set_str(vm, an, sv->sval);
          if (ai < n_params && params[ai][0])
            var_set_str(vm, params[ai], sv->sval);
          lex_next(L);
        } else {
          /* bare numeric-ish ident / var â single atom, not 4 OR 99 expr */
          v = parse_prim(vm, L);
          var_set_num(vm, an, v);
          if (ai < n_params && params[ai][0])
            var_set_num(vm, params[ai], v);
        }
      }
    } else if (L->cur.kind == TK_NUM) {
      /* single number token â do not parse_expr (would swallow OR fallback) */
      v = strtol(L->cur.text, NULL, 10);
      lex_next(L);
      var_set_num(vm, an, v);
      if (ai < n_params && params[ai][0])
        var_set_num(vm, params[ai], v);
    } else if (L->cur.kind == TK_MINUS) {
      /* unary minus number or parenthesized expr */
      lex_next(L);
      if (L->cur.kind == TK_NUM) {
        v = -strtol(L->cur.text, NULL, 10);
        lex_next(L);
      } else if (L->cur.kind == TK_LPAREN) {
        v = -parse_expr(vm, L);
      } else {
        v = -parse_prim(vm, L);
      }
      var_set_num(vm, an, v);
      if (ai < n_params && params[ai][0])
        var_set_num(vm, params[ai], v);
    } else if (L->cur.kind == TK_LPAREN) {
      v = parse_expr(vm, L);
      var_set_num(vm, an, v);
      if (ai < n_params && params[ai][0])
        var_set_num(vm, params[ai], v);
    } else {
      break;
    }
    ai++;
  }
  var_set_num(vm, "NARGS", ai);
  return ai;
}

/* Under-arity gate for CALL/SEND/NEW â agent-readable needs N got M.
 * Returns 0 ok, 1 soft miss (do not run body), -1 fatal. Clears unbound formals. */
int oop_arity_gate(VM *vm, const char *tag, const char *name,
                          char params[][32], int n_params, int got, int soft){
  int i;
  char ebuf[192], plist[96];
  int o = 0;
  var_set_num(vm, "ARITY_NEED", n_params);
  var_set_num(vm, "ARITY_GOT", got);
  var_set_num(vm, "NARGS", got);
  /* clear unbound formals so stale values do not leak into FN/METHOD */
  for (i = got; i < n_params; i++) {
    if (params[i][0])
      var_set_num(vm, params[i], 0);
  }
  if (got >= n_params || n_params <= 0)
    return 0;
  plist[0] = 0;
  for (i = 0; i < n_params && o + 34 < (int)sizeof plist; i++) {
    int ln;
    if (i) {
      if (o + 1 < (int)sizeof plist) { plist[o++] = ' '; plist[o] = 0; }
    }
    ln = snprintf(plist + o, sizeof plist - (size_t)o, "%s",
                  params[i][0] ? params[i] : "?");
    if (ln > 0) o += ln;
  }
  snprintf(ebuf, sizeof ebuf,
           "%s %s needs %d arg%s, got %d (params: %s)",
           tag, name && name[0] ? name : "?",
           n_params, n_params == 1 ? "" : "s", got,
           plist[0] ? plist : "-");
  if (soft) {
    var_set_str(vm, "LAST_ERR", ebuf);
    var_set_str(vm, "ERR", ebuf);
    var_set_str(vm, "LAST", ebuf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", ebuf);
    vm->last_n = 0;
    var_set_num(vm, "LAST_N", 0);
    var_set_num(vm, "OK", 0);
    return 1;
  }
  fail(vm, ebuf);
  return -1;
}

/* Resolve instance name: bare token, string literal, or string-var value.
 * NEW sets var(name)=Class as string â so bare re-NEW of a free slot must NOT
 * expand that pollution. Prefer literal when an obj slot (live or dead) already
 * uses this exact name (pool recycle). Else expand string-var for dynamic slots
 * (LET slot="dyn1"; NEW Cell slot). */
int oop_read_name(VM *vm, Lex *L, char *out, size_t outn, const char *why){
  if (L->cur.kind == TK_STR) {
    if (!L->cur.text[0]) { fail(vm, why); return -1; }
    snprintf(out, outn, "%s", L->cur.text);
    lex_next(L);
    return 0;
  }
  if (L->cur.kind != TK_IDENT) { fail(vm, why); return -1; }
  if (strcmp(L->cur.text, "LAST") == 0) {
    if (!vm->last_str[0]) { fail(vm, why); return -1; }
    snprintf(out, outn, "%s", vm->last_str);
    lex_next(L);
    return 0;
  }
  {
    int i;
    for (i = 0; i < vm->n_objs; i++) {
      if (strcmp(vm->objs[i].name, L->cur.text) == 0) {
        snprintf(out, outn, "%s", L->cur.text);
        lex_next(L);
        return 0;
      }
    }
  }
  {
    Var *sv = var_get(vm, L->cur.text, 0);
    if (sv && sv->is_str && sv->sval[0]) {
      snprintf(out, outn, "%s", sv->sval);
      lex_next(L);
      return 0;
    }
  }
  /* bare identifier = literal object name token */
  snprintf(out, outn, "%s", L->cur.text);
  lex_next(L);
  return 0;
}
int oop_run_method(VM *vm, ObjInst *ob, MethodDef *md){
  ClassDef *cd;
  char save_this[48];
  if (!ob || !md || ob->class_idx < 0 || ob->class_idx >= vm->n_classes)
    return -1;
  cd = &vm->classes[ob->class_idx];
  snprintf(save_this, sizeof save_this, "%s", vm->this_obj);
  snprintf(vm->this_obj, sizeof vm->this_obj, "%s", ob->name);
  var_set_str(vm, "THIS", ob->name);
  var_set_str(vm, "SELF", ob->name);
  var_set_str(vm, "CLASS", cd->name);
  var_set_num(vm, "CALLED", 1);
  vm->return_fn = 0;
  {
    Lex fl;
    lex_init(&fl, md->body, md->len);
    if (exec_stmts_until(vm, &fl, "END", NULL) < 0) {
      snprintf(vm->this_obj, sizeof vm->this_obj, "%s", save_this);
      return -1;
    }
  }
  vm->return_fn = 0;
  snprintf(vm->this_obj, sizeof vm->this_obj, "%s", save_this);
  if (save_this[0]) {
    var_set_str(vm, "THIS", save_this);
    var_set_str(vm, "SELF", save_this);
  }
  return 0;
}
int oop_new_instance(VM *vm, const char *cname, const char *oname,
                            int cube_idx, Lex *L, int bind_ctor, int soft){
  int ci = oop_find_class_idx(vm, cname);
  ClassDef *cd;
  ObjInst *ob;
  int fi, i, reuse = -1;
  MethodDef *initm;
  /* soft=1 â sticky LAST_ERR/OK=0, return -2 (no fatal). Usability: TRYNEW. */
  if (ci < 0) {
    char sug[48], ebuf[160], base[96];
    oop_suggest_class(vm, cname, sug, sizeof sug);
    snprintf(base, sizeof base, "unknown CLASS %s", cname);
    oop_err_suggest(ebuf, sizeof ebuf, base, sug);
    if (soft) {
      var_set_str(vm, "ERR", ebuf);
      var_set_str(vm, "LAST_ERR", ebuf);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "OK", 0);
      var_set_num(vm, "TRYNEW_N", 0);
      return -2;
    }
    fail(vm, ebuf);
    return -1;
  }
  if (oop_find_obj(vm, oname)) {
    snprintf(vm->err, sizeof vm->err, "object redefine %s", oname);
    if (soft) {
      var_set_str(vm, "ERR", vm->err);
      var_set_str(vm, "LAST_ERR", vm->err);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "OK", 0);
      var_set_num(vm, "TRYNEW_N", 0);
      return -2;
    }
    fail(vm, vm->err);
    return -1;
  }
  /* Reuse DESTROY'd slot with same name (life_engine free slots). */
  for (i = 0; i < vm->n_objs; i++) {
    if (!vm->objs[i].live && strcmp(vm->objs[i].name, oname) == 0) {
      reuse = i;
      break;
    }
  }
  if (reuse < 0 && vm->n_objs >= CUBALC_MAX_OBJS) {
    /* pack: reuse any dead slot */
    for (i = 0; i < vm->n_objs; i++) {
      if (!vm->objs[i].live) { reuse = i; break; }
    }
  }
  if (reuse < 0 && vm->n_objs >= CUBALC_MAX_OBJS) {
    if (soft) {
      var_set_str(vm, "ERR", "too many objects");
      var_set_str(vm, "LAST_ERR", "too many objects");
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "OK", 0);
      var_set_num(vm, "TRYNEW_N", 0);
      return -2;
    }
    fail(vm, "too many objects");
    return -1;
  }
  cd = &vm->classes[ci];
  if (reuse >= 0)
    ob = &vm->objs[reuse];
  else
    ob = &vm->objs[vm->n_objs++];
  memset(ob, 0, sizeof *ob);
  snprintf(ob->name, sizeof ob->name, "%s", oname);
  ob->class_idx = ci;
  ob->live = 1;
  ob->cube_idx = cube_idx;
  for (fi = 0; fi < cd->n_fields; fi++) {
    FieldDef *fd = &cd->fields[fi];
    if (fd->has_def && fd->is_str) {
      snprintf(ob->fstr[fi], sizeof ob->fstr[fi], "%s", fd->def_str);
      ob->fis_str[fi] = 1;
    } else {
      ob->fnum[fi] = fd->has_def ? fd->def_num : 0;
      ob->fis_str[fi] = 0;
    }
  }
  var_set_str(vm, oname, cname);
  var_set_str(vm, "OBJECT", oname);
  var_set_str(vm, "CLASS", cname);
  if (!bind_ctor || !L) return 0;
  initm = oop_find_method(cd, "init");
  if (!initm) initm = oop_find_method(cd, "construct");
  if (!initm) initm = oop_find_method(cd, "spawn");
  if (!initm) initm = oop_find_method(cd, "new");
  if (initm) {
    int got = oop_bind_args(vm, L, initm->params, initm->n_params);
    char qname[96];
    int ag;
    snprintf(qname, sizeof qname, "%s.%s", cname, initm->name);
    ag = oop_arity_gate(vm, "NEW", qname, initm->params, initm->n_params, got, 0);
    if (ag < 0) return -1;
    if (oop_run_method(vm, ob, initm) < 0) return -1;
  } else {
    while (L->cur.kind == TK_NUM || L->cur.kind == TK_STR ||
           L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
           (L->cur.kind == TK_IDENT && !oop_stmt_kw(L))) {
      if (L->cur.kind == TK_STR) lex_next(L);
      else (void)parse_expr(vm, L);
    }
  }
  return 0;
}

