/* CubalC lang — lang_ops_cell.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/* INCLUDE lib did-you-mean — scan programs/lib for closest stem. */
static void include_fold(char *dst, size_t n, const char *src){
  size_t i;
  for (i = 0; i + 1 < n && src[i]; i++) {
    char c = src[i];
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    dst[i] = c;
  }
  dst[i] = 0;
}
static int include_edit_dist(const char *a, const char *b){
  int na = (int)strlen(a), nb = (int)strlen(b);
  int i, j, prev, cur, stack[64];
  if (na > 48) na = 48;
  if (nb > 48) nb = 48;
  if (na + 1 > 64) return 99;
  for (j = 0; j <= nb; j++) stack[j] = j;
  for (i = 1; i <= na; i++) {
    prev = stack[0];
    stack[0] = i;
    for (j = 1; j <= nb; j++) {
      cur = stack[j];
      if (a[i - 1] == b[j - 1])
        stack[j] = prev;
      else {
        int ins = stack[j - 1] + 1;
        int del = stack[j] + 1;
        int sub = prev + 1;
        int m = ins < del ? ins : del;
        stack[j] = m < sub ? m : sub;
      }
      prev = cur;
    }
  }
  return stack[nb];
}
static void include_suggest_lib(const char *typo, char *out, size_t outn){
  DIR *d;
  struct dirent *ent;
  char want[64], cand[64], stem[128], best[128];
  int best_d = 99, ed;
  size_t best_len = 9999;
  int have_prefix = 0;
  const char *dirs[] = { "programs/lib", "lib", NULL };
  int di;
  out[0] = 0;
  best[0] = 0;
  if (!typo || !typo[0] || outn < 2) return;
  {
    const char *slash = strrchr(typo, '/');
    const char *leaf = slash ? slash + 1 : typo;
    size_t blen;
    snprintf(stem, sizeof stem, "%s", leaf);
    blen = strlen(stem);
    if (blen > 7 && strcmp(stem + blen - 7, ".cubalc") == 0)
      stem[blen - 7] = 0;
  }
  include_fold(want, sizeof want, stem);
  if (!want[0]) return;
  for (di = 0; dirs[di]; di++) {
    d = opendir(dirs[di]);
    if (!d) continue;
    while ((ent = readdir(d)) != NULL) {
      size_t nlen, cl;
      char name[128];
      if (ent->d_name[0] == '.') continue;
      nlen = strlen(ent->d_name);
      if (nlen < 8 || strcmp(ent->d_name + nlen - 7, ".cubalc") != 0) continue;
      if (nlen - 7 >= sizeof name) continue;
      memcpy(name, ent->d_name, nlen - 7);
      name[nlen - 7] = 0;
      include_fold(cand, sizeof cand, name);
      if (strcmp(want, cand) == 0) {
        snprintf(out, outn, "%s", name);
        closedir(d);
        return;
      }
      cl = strlen(cand);
      if (strlen(want) >= 3 &&
          (strncmp(want, cand, strlen(want)) == 0 ||
           (cl >= 3 && strncmp(cand, want, cl) == 0))) {
        if (!have_prefix || cl < best_len) {
          have_prefix = 1;
          best_len = cl;
          snprintf(best, sizeof best, "%s", name);
        }
      }
      ed = include_edit_dist(want, cand);
      if (ed < best_d) {
        best_d = ed;
        if (!have_prefix)
          snprintf(best, sizeof best, "%s", name);
      }
    }
    closedir(d);
  }
  if (have_prefix) {
    snprintf(out, outn, "%s", best);
    return;
  }
  /* recompute best by edit only (best may be stale if have_prefix path mixed) */
  best_d = 99;
  best[0] = 0;
  for (di = 0; dirs[di]; di++) {
    d = opendir(dirs[di]);
    if (!d) continue;
    while ((ent = readdir(d)) != NULL) {
      size_t nlen;
      char name[128];
      if (ent->d_name[0] == '.') continue;
      nlen = strlen(ent->d_name);
      if (nlen < 8 || strcmp(ent->d_name + nlen - 7, ".cubalc") != 0) continue;
      if (nlen - 7 >= sizeof name) continue;
      memcpy(name, ent->d_name, nlen - 7);
      name[nlen - 7] = 0;
      include_fold(cand, sizeof cand, name);
      ed = include_edit_dist(want, cand);
      if (ed < best_d) {
        best_d = ed;
        snprintf(best, sizeof best, "%s", name);
      }
    }
    closedir(d);
  }
  if (best_d <= 2 && best[0])
    snprintf(out, outn, "%s", best);
}

/* Stem/path match against vm->included[] (HASINCLUDE / NEEDINCLUDE). */
static int include_loaded_match(VM *vm, const char *want, char *hit_path, size_t hit_sz) {
  char stem[256];
  size_t wlen;
  const char *slash;
  int i;
  if (!vm || !want || !want[0]) return 0;
  slash = strrchr(want, '/');
  snprintf(stem, sizeof stem, "%s", slash ? slash + 1 : want);
  wlen = strlen(stem);
  if (wlen > 7 && strcmp(stem + wlen - 7, ".cubalc") == 0)
    stem[wlen - 7] = 0;
  for (i = 0; i < vm->n_included; i++) {
    const char *p = vm->included[i];
    const char *b = strrchr(p, '/');
    char pstem[256];
    size_t plen;
    b = b ? b + 1 : p;
    snprintf(pstem, sizeof pstem, "%s", b);
    plen = strlen(pstem);
    if (plen > 7 && strcmp(pstem + plen - 7, ".cubalc") == 0)
      pstem[plen - 7] = 0;
    if (strcmp(p, want) == 0 || strcmp(b, want) == 0 ||
        strcmp(pstem, want) == 0 || strcmp(pstem, stem) == 0 ||
        strcmp(b, stem) == 0 ||
        (stem[0] && strstr(p, stem))) {
      if (hit_path && hit_sz)
        snprintf(hit_path, hit_sz, "%s", p);
      return 1;
    }
  }
  if (hit_path && hit_sz) hit_path[0] = 0;
  return 0;
}

/* Parse one name|path token for INCLUDE probes (IDENT/STR/string-var/LAST). */
static int include_parse_want(VM *vm, Lex *L, char *want, size_t want_sz) {
  if (L->cur.kind == TK_STR) {
    snprintf(want, want_sz, "%s", L->cur.text);
    lex_next(L);
    return 1;
  }
  if (L->cur.kind == TK_IDENT) {
    Var *vv = var_get(vm, L->cur.text, 0);
    if (vv && vv->is_str && vv->sval[0])
      snprintf(want, want_sz, "%s", vv->sval);
    else if (strcmp(L->cur.text, "LAST") == 0)
      snprintf(want, want_sz, "%s", vm->last_str);
    else
      snprintf(want, want_sz, "%s", L->cur.text);
    lex_next(L);
    return 1;
  }
  return 0;
}

/* Resolve short lib name → path (INCLUDE search order). Returns 1 if found. */
static int lib_resolve_path(const char *name, char *path, size_t pathsz,
                            char *base_out, size_t basesz) {
  char base[160];
  const char *slash, *leaf, *ip, *root;
  size_t blen;
  FILE *f = NULL;
  if (!name || !name[0] || !path || pathsz < 8) return 0;
  path[0] = 0;
  slash = strrchr(name, '/');
  leaf = slash ? slash + 1 : name;
  snprintf(base, sizeof base, "%s", leaf);
  blen = strlen(base);
  if (blen > 7 && strcmp(base + blen - 7, ".cubalc") == 0)
    base[blen - 7] = 0;
  if (base_out && basesz)
    snprintf(base_out, basesz, "%s", base);
  if (name[0] == '/' || strchr(name, '/')) {
    f = fopen(name, "rb");
    if (f) { snprintf(path, pathsz, "%s", name); fclose(f); return 1; }
  }
  {
    char p3[768];
    snprintf(p3, sizeof p3, "programs/lib/%s.cubalc", base);
    f = fopen(p3, "rb");
    if (f) { snprintf(path, pathsz, "%s", p3); fclose(f); return 1; }
    snprintf(p3, sizeof p3, "programs/lib/%s", name);
    f = fopen(p3, "rb");
    if (f) { snprintf(path, pathsz, "%s", p3); fclose(f); return 1; }
  }
  f = fopen(name, "rb");
  if (f) { snprintf(path, pathsz, "%s", name); fclose(f); return 1; }
  root = getenv("CUBALC_ROOT");
  if (root && root[0]) {
    char p2[768];
    snprintf(p2, sizeof p2, "%s/programs/lib/%s.cubalc", root, base);
    f = fopen(p2, "rb");
    if (f) { snprintf(path, pathsz, "%s", p2); fclose(f); return 1; }
  }
  ip = getenv("CUBALC_INCLUDE_PATH");
  if (ip && ip[0]) {
    const char *seg = ip;
    while (*seg) {
      char dir[512], p3[768];
      size_t dlen = 0;
      while (*seg == ':') seg++;
      if (!*seg) break;
      while (seg[dlen] && seg[dlen] != ':' && dlen + 1 < sizeof dir)
        dir[dlen] = seg[dlen], dlen++;
      dir[dlen] = 0;
      seg += dlen;
      if (!dir[0]) continue;
      snprintf(p3, sizeof p3, "%s/%s.cubalc", dir, base);
      f = fopen(p3, "rb");
      if (f) { snprintf(path, pathsz, "%s", p3); fclose(f); return 1; }
      snprintf(p3, sizeof p3, "%s/%s", dir, name);
      f = fopen(p3, "rb");
      if (f) { snprintf(path, pathsz, "%s", p3); fclose(f); return 1; }
    }
  }
  return 0;
}

/* Parse INCLUDE stems from a resolved lib path into stems[] (append, no dups vs existing). */
static int lib_append_deps(const char *path, char stems[][96], int *nstem, int maxstem) {
  FILE *f;
  char *src = NULL;
  long sz = 0;
  size_t nr = 0;
  const char *lp;
  int added = 0;
  if (!path || !path[0] || !stems || !nstem || maxstem <= 0) return 0;
  f = fopen(path, "rb");
  if (!f) return 0;
  fseek(f, 0, SEEK_END);
  sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (sz < 0) sz = 0;
  if (sz > 256 * 1024) sz = 256 * 1024;
  src = (char *)malloc((size_t)sz + 1);
  if (!src) { fclose(f); return 0; }
  nr = fread(src, 1, (size_t)sz, f);
  fclose(f);
  src[nr] = 0;
  lp = src;
  while (*lp && *nstem < maxstem) {
    const char *line = lp;
    char tok[160];
    size_t ti = 0;
    int j, dup;
    while (*lp && *lp != '\n') lp++;
    while (*line == ' ' || *line == '\t') line++;
    if (*line == '#' || *line == '\n' || *line == 0) {
      if (*lp == '\n') lp++;
      continue;
    }
    if (!(line[0] == 'I' && line[1] == 'N' && line[2] == 'C' && line[3] == 'L' &&
          line[4] == 'U' && line[5] == 'D' && line[6] == 'E' &&
          (line[7] == ' ' || line[7] == '\t' || line[7] == '"' || line[7] == '\'' ||
           line[7] == 0 || line[7] == '\r' || line[7] == '\n'))) {
      if (*lp == '\n') lp++;
      continue;
    }
    line += 7;
    while (*line == ' ' || *line == '\t') line++;
    for (;;) {
      if (line[0] == 'O' && line[1] == 'N' && line[2] == 'C' && line[3] == 'E' &&
          (line[4] == ' ' || line[4] == '\t' || line[4] == 0 || line[4] == '\r' ||
           line[4] == '\n')) {
        line += 4;
        while (*line == ' ' || *line == '\t') line++;
        continue;
      }
      if (line[0] == 'S' && line[1] == 'O' && line[2] == 'F' && line[3] == 'T' &&
          (line[4] == ' ' || line[4] == '\t' || line[4] == 0 || line[4] == '\r' ||
           line[4] == '\n')) {
        line += 4;
        while (*line == ' ' || *line == '\t') line++;
        continue;
      }
      if (line[0] == 'O' && line[1] == 'R' &&
          (line[2] == ' ' || line[2] == '\t' || line[2] == 0 || line[2] == '\r' ||
           line[2] == '\n')) {
        line += 2;
        while (*line == ' ' || *line == '\t') line++;
        continue;
      }
      break;
    }
    tok[0] = 0;
    ti = 0;
    if (*line == '"' || *line == '\'') {
      char q = *line++;
      while (*line && *line != q && *line != '\n' && *line != '\r' && ti + 1 < sizeof tok)
        tok[ti++] = *line++;
      tok[ti] = 0;
    } else {
      while (*line && *line != ' ' && *line != '\t' && *line != '\n' && *line != '\r' &&
             *line != '#' && ti + 1 < sizeof tok)
        tok[ti++] = *line++;
      tok[ti] = 0;
    }
    if (tok[0]) {
      const char *sl = strrchr(tok, '/');
      const char *st = sl ? sl + 1 : tok;
      char stem[96];
      size_t slen;
      snprintf(stem, sizeof stem, "%s", st);
      slen = strlen(stem);
      if (slen > 7 && strcmp(stem + slen - 7, ".cubalc") == 0)
        stem[slen - 7] = 0;
      if (stem[0]) {
        dup = 0;
        for (j = 0; j < *nstem; j++) {
          if (strcmp(stems[j], stem) == 0) { dup = 1; break; }
        }
        if (!dup && *nstem < maxstem) {
          snprintf(stems[*nstem], sizeof stems[0], "%s", stem);
          (*nstem)++;
          added++;
        }
      }
    }
    if (*lp == '\n') lp++;
  }
  free(src);
  return added;
}

int cubalc_lang_ops_cell(VM *vm, Lex *L){
  /* plane ops_cell: L25536-30474 */
  /* digit-5 cell search ext: FINDLASTCELL · FIRSTNZ · LASTNZ */
  if (kw(&L->cur,"FINDLASTCELL")||kw(&L->cur,"RFINDCELL")||kw(&L->cur,"CELLLASTFIND")||
      kw(&L->cur,"RINDEXCELL")||kw(&L->cur,"LASTFINDCELL")){
    /* FINDLASTCELL val [lo [hi]] — last index of val, or -1; OK=found */
    lex_next(L);
    long val = parse_expr(vm,L);
    long lo = 0, hi = CUBALC_CELL_N - 1;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE"))){
      lo = parse_expr(vm,L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"PRINT") && !kw(&L->cur,"END")))
        hi = parse_expr(vm,L);
      else hi = CUBALC_CELL_N - 1;
    }
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long found = -1;
    for (long i=hi;i>=lo;i--){
      if (vm->cells[(int)i] == val){ found = i; break; }
    }
    var_set_num(vm,"LAST_N",found); vm->last_n=found;
    var_set_num(vm,"OK", found >= 0 ? 1 : 0);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"FIRSTNZ")||kw(&L->cur,"FIRSTNONZERO")||kw(&L->cur,"CELLFIRSTNZ")||
      kw(&L->cur,"FINDNZ")||
      kw(&L->cur,"LASTNZ")||kw(&L->cur,"LASTNONZERO")||kw(&L->cur,"CELLLASTNZ")||
      kw(&L->cur,"RFINDNZ")){
    /* FIRSTNZ/LASTNZ [lo [hi]] — index of first/last nonzero, or -1 */
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    long lo = 0, hi = CUBALC_CELL_N - 1;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE"))){
      lo = parse_expr(vm,L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"PRINT") && !kw(&L->cur,"END")))
        hi = parse_expr(vm,L);
      else hi = CUBALC_CELL_N - 1;
    }
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int want_last = (strcmp(op,"LASTNZ")==0 || strcmp(op,"LASTNONZERO")==0 ||
                     strcmp(op,"CELLLASTNZ")==0 || strcmp(op,"RFINDNZ")==0);
    long found = -1;
    if (want_last){
      for (long i=hi;i>=lo;i--){
        if (vm->cells[(int)i] != 0){ found = i; break; }
      }
    } else {
      for (long i=lo;i<=hi;i++){
        if (vm->cells[(int)i] != 0){ found = i; break; }
      }
    }
    var_set_num(vm,"LAST_N",found); vm->last_n=found;
    var_set_num(vm,"OK", found >= 0 ? 1 : 0);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"REVCELL")||kw(&L->cur,"CELLREV")||kw(&L->cur,"REVERSECELLS")){
    /* REVCELL lo hi — reverse cell[lo..hi] in place */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long i = lo, j = hi;
    while (i < j){
      long t = vm->cells[(int)i];
      vm->cells[(int)i] = vm->cells[(int)j];
      vm->cells[(int)j] = t;
      i++; j--;
    }
    var_set_num(vm,"LAST_N", hi - lo + 1); vm->last_n = hi - lo + 1;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 cell fold plane: ADDCELL MULCELL IOTA SORTCELL */
  if (kw(&L->cur,"ADDCELL")||kw(&L->cur,"CELLADD")||kw(&L->cur,"ADDTOCELL")){
    /* ADDCELL lo hi delta — add delta to each cell in range */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long delta = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] += delta;
    var_set_num(vm,"LAST_N",delta); vm->last_n=delta;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"MULCELL")||kw(&L->cur,"CELLMUL")||kw(&L->cur,"SCALECELL")){
    /* MULCELL lo hi k — multiply each cell in range by k */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] *= k;
    var_set_num(vm,"LAST_N",k); vm->last_n=k;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"IOTA")||kw(&L->cur,"CELLIOTA")||kw(&L->cur,"SEQCELL")||kw(&L->cur,"RANGECELL")){
    /* IOTA lo hi [start [step]] — fill cell[lo..hi] with arithmetic sequence */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long start = 0, step = 1;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE"))){
      start = parse_expr(vm,L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"PRINT") && !kw(&L->cur,"END")))
        step = parse_expr(vm,L);
    }
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long v = start;
    for (long i=lo;i<=hi;i++){
      vm->cells[(int)i] = v;
      v += step;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORTCELL")||kw(&L->cur,"CELLSORT")||kw(&L->cur,"ISORTCELL")){
    /* SORTCELL lo hi [ASC|DESC|dir] — insertion sort; default ASC
     * Note: bare -1 after hi would parse as hi-1; use DESC or ( -1 ). */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long dir = 1;
    if (kw(&L->cur,"DESC")||kw(&L->cur,"DOWN")||kw(&L->cur,"REV")||kw(&L->cur,"REVERSE")){
      lex_next(L); dir = -1;
    } else if (kw(&L->cur,"ASC")||kw(&L->cur,"UP")){
      lex_next(L); dir = 1;
    } else if (L->cur.kind==TK_NUM){
      dir = L->cur.num; if (dir == 0) dir = -1;
      lex_next(L);
    } else if (L->cur.kind==TK_LPAREN){
      dir = parse_expr(vm,L);
    }
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i = lo + 1; i <= hi; i++){
      long key = vm->cells[(int)i];
      long j = i - 1;
      if (dir >= 0){
        while (j >= lo && vm->cells[(int)j] > key){
          vm->cells[(int)(j+1)] = vm->cells[(int)j];
          j--;
        }
      } else {
        while (j >= lo && vm->cells[(int)j] < key){
          vm->cells[(int)(j+1)] = vm->cells[(int)j];
          j--;
        }
      }
      vm->cells[(int)(j+1)] = key;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 cell fold ext: MINIDX/MAXIDX + ROTCELL/SHIFTCELL */
  if (kw(&L->cur,"MINIDX")||kw(&L->cur,"ARGMIN")||kw(&L->cur,"CELLMINI")||
      kw(&L->cur,"MAXIDX")||kw(&L->cur,"ARGMAX")||kw(&L->cur,"CELLMAXI")){
    /* MINIDX/MAXIDX [lo [hi]] — index of min/max in range (first on ties) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int want_max = (strcmp(op,"MAXIDX")==0 || strcmp(op,"ARGMAX")==0 ||
                    strcmp(op,"CELLMAXI")==0);
    lex_next(L);
    long lo = 0, hi = CUBALC_CELL_N - 1;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE"))){
      lo = parse_expr(vm,L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"PRINT") && !kw(&L->cur,"END")))
        hi = parse_expr(vm,L);
      else hi = lo;
    }
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long best_i = lo;
    long best_v = vm->cells[(int)lo];
    for (long i = lo + 1; i <= hi; i++){
      long v = vm->cells[(int)i];
      if (want_max){ if (v > best_v){ best_v = v; best_i = i; } }
      else { if (v < best_v){ best_v = v; best_i = i; } }
    }
    var_set_num(vm,"LAST_N",best_i); vm->last_n=best_i;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ROTCELL")||kw(&L->cur,"CELLROT")||kw(&L->cur,"ROTATECELL")){
    /* ROTCELL lo hi k — rotate cell[lo..hi] left by k (k<0 = right) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long n = hi - lo + 1;
    if (n > 0){
      long kk = k % n;
      if (kk < 0) kk += n;
      if (kk){
        long tmp[CUBALC_CELL_N];
        for (long i=0;i<n;i++) tmp[i] = vm->cells[(int)(lo+i)];
        for (long i=0;i<n;i++)
          vm->cells[(int)(lo+i)] = tmp[(i + kk) % n];
      }
    }
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHIFTCELL")||kw(&L->cur,"CELLSHIFT")){
    /* SHIFTCELL lo hi k — shift left by k (k>0) or right by |k| (k<0); zero-fill */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long n = hi - lo + 1;
    if (n > 0 && k != 0){
      long tmp[CUBALC_CELL_N];
      for (long i=0;i<n;i++) tmp[i] = 0;
      if (k > 0){
        if (k > n) k = n;
        for (long i=0;i<n-k;i++) tmp[i] = vm->cells[(int)(lo+i+k)];
      } else {
        long kk = -k;
        if (kk > n) kk = n;
        for (long i=kk;i<n;i++) tmp[i] = vm->cells[(int)(lo+i-kk)];
      }
      for (long i=0;i<n;i++) vm->cells[(int)(lo+i)] = tmp[i];
    }
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 cell logic plane: ANDCELL ORCELL XORCELL NOTCELL EQCELL */
  if (kw(&L->cur,"ANDCELL")||kw(&L->cur,"CELLAND")||kw(&L->cur,"BANDCELL")){
    /* ANDCELL lo hi mask — bitwise AND each cell in range with mask */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] &= mask;
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ORCELL")||kw(&L->cur,"CELLOR")||kw(&L->cur,"BORCELL")){
    /* ORCELL lo hi mask — bitwise OR each cell in range with mask */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] |= mask;
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"XORCELL")||kw(&L->cur,"CELLXOR")||kw(&L->cur,"BXORCELL")){
    /* XORCELL lo hi mask — bitwise XOR each cell in range with mask */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] ^= mask;
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 cell inverted logic: NANDCELL · NORCELL · XNORCELL
   * (complete AND/OR/XOR cell plane with inverted duals) */
  if (kw(&L->cur,"NANDCELL")||kw(&L->cur,"CELLNAND")||kw(&L->cur,"BNANDCELL")){
    /* NANDCELL lo hi mask — cells[i] = ~(cells[i] & mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] = ~(vm->cells[(int)i] & mask);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"NORCELL")||kw(&L->cur,"CELLNOR")||kw(&L->cur,"BNORCELL")){
    /* NORCELL lo hi mask — cells[i] = ~(cells[i] | mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] = ~(vm->cells[(int)i] | mask);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"XNORCELL")||kw(&L->cur,"CELLXNOR")||kw(&L->cur,"BXNORCELL")||
      kw(&L->cur,"NXORCELL")||kw(&L->cur,"CELLEQX")){
    /* XNORCELL lo hi mask — cells[i] = ~(cells[i] ^ mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] = ~(vm->cells[(int)i] ^ mask);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 cell fixed-width shift32 plane: SHL32CELL · SHR32CELL · SAR32CELL
   * (range dual of SSHL32/SSHR32/SSAR32 and SSHL32TOC plane after NANDCELL logic) */
  if (kw(&L->cur,"SHL32CELL")||kw(&L->cur,"BSHL32CELL")||kw(&L->cur,"RANGESHL32")||
      kw(&L->cur,"LSH32CELL")||kw(&L->cur,"SHL32RANGE")){
    /* SHL32CELL lo hi k — cells[i] = (uint32)cells[i] << k (k>=32 → 0) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i];
      vm->cells[(int)i] = (kk >= 32) ? 0L : (long)(w << kk);
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHR32CELL")||kw(&L->cur,"BSHR32CELL")||kw(&L->cur,"RANGESHR32")||
      kw(&L->cur,"LSHR32CELL")||kw(&L->cur,"SHR32RANGE")){
    /* SHR32CELL lo hi k — cells[i] = (uint32)cells[i] >> k logical (k>=32 → 0) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i];
      vm->cells[(int)i] = (kk >= 32) ? 0L : (long)(w >> kk);
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SAR32CELL")||kw(&L->cur,"BSAR32CELL")||kw(&L->cur,"RANGESAR32")||
      kw(&L->cur,"ASHR32CELL")||kw(&L->cur,"SAR32RANGE")||kw(&L->cur,"ASHRCELL32")){
    /* SAR32CELL lo hi k — arithmetic right shift low32 (k>=32 → all sign) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    for (long i=lo;i<=hi;i++){
      long va = (long)(int)(unsigned int)vm->cells[(int)i];
      long r;
      if (kk >= 32) r = (va < 0) ? -1L : 0L;
      else r = va >> kk;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 cell fixed-width rotate32 + bitrev plane: ROL32CELL · ROR32CELL · BITREV32CELL
   * (range dual of SROTL32/SROTR32/SBITREV32 after SHL32CELL plane; control/bitfield digit-4) */
  if (kw(&L->cur,"ROL32CELL")||kw(&L->cur,"ROTL32CELL")||kw(&L->cur,"BROL32CELL")||
      kw(&L->cur,"RANGEROL32")||kw(&L->cur,"ROL32RANGE")||kw(&L->cur,"CELLROTL32")){
    /* ROL32CELL lo hi k — cells[i] = rotl32(low32 cells[i], k&31) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 31;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i];
      long r = (kk == 0) ? (long)w : (long)(((w << kk) | (w >> (32 - kk))));
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ROR32CELL")||kw(&L->cur,"ROTR32CELL")||kw(&L->cur,"BROR32CELL")||
      kw(&L->cur,"RANGEROR32")||kw(&L->cur,"ROR32RANGE")||kw(&L->cur,"CELLROTR32")){
    /* ROR32CELL lo hi k — cells[i] = rotr32(low32 cells[i], k&31) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 31;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i];
      long r = (kk == 0) ? (long)w : (long)(((w >> kk) | (w << (32 - kk))));
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"BITREV32CELL")||kw(&L->cur,"BREV32CELL")||kw(&L->cur,"REV32CELL")||
      kw(&L->cur,"RANGEBITREV32")||kw(&L->cur,"BITREV32RANGE")||kw(&L->cur,"CELLBITREV32R")){
    /* BITREV32CELL lo hi — cells[i] = bitrev32(low32 cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i];
      unsigned int rv = 0;
      for (int b = 0; b < 32; b++){
        rv = (rv << 1) | (w & 1u);
        w >>= 1;
      }
      vm->cells[(int)i] = (long)rv;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 cell bit-metrics plane: PARITYCELL · BSWAPCELL · POPCNTCELL
   * (range dual of SPARITY/SBSWAP/SPOPCNT after BITREV32CELL + SHL32CELL plane) */
  if (kw(&L->cur,"PARITYCELL")||kw(&L->cur,"CELLPARITY")||kw(&L->cur,"BPARITYCELL")||
      kw(&L->cur,"RANGEPARITY")||kw(&L->cur,"PARITYRANGE")||kw(&L->cur,"PARCELL")){
    /* PARITYCELL lo hi — cells[i] = xor of all bits (parity) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned long u = (unsigned long)vm->cells[(int)i];
      long r = 0;
      while (u){ r ^= (long)(u & 1ul); u >>= 1; }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"BSWAPCELL")||kw(&L->cur,"BSWAP32CELL")||kw(&L->cur,"CELLBSWAP")||
      kw(&L->cur,"RANGEBSWAP")||kw(&L->cur,"BSWAPRANGE")||kw(&L->cur,"BBSWAPCELL")){
    /* BSWAPCELL lo hi — cells[i] = bswap32(low32 cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i];
      w = ((w & 0x000000FFu) << 24) | ((w & 0x0000FF00u) << 8) |
          ((w & 0x00FF0000u) >> 8) | ((w & 0xFF000000u) >> 24);
      vm->cells[(int)i] = (long)w;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"POPCNTCELL")||kw(&L->cur,"CELLPOPCNT")||kw(&L->cur,"BPOPCNTCELL")||
      kw(&L->cur,"RANGEPOPCNT")||kw(&L->cur,"POPCNTRANGE")||kw(&L->cur,"PCNTCELL")||
      kw(&L->cur,"HAMMINGCELL")){
    /* POPCNTCELL lo hi — cells[i] = popcount(cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned long u = (unsigned long)vm->cells[(int)i];
      long r = 0;
      while (u){ r += (long)(u & 1ul); u >>= 1; }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 cell bit-metrics ext: CLZCELL · CTZCELL · BSWAP16CELL
   * (complete popcnt/clz/ctz + bswap16 ladder after PARITYCELL/BSWAPCELL/POPCNTCELL) */
  if (kw(&L->cur,"CLZCELL")||kw(&L->cur,"CELLCLZ")||kw(&L->cur,"BCLZCELL")||
      kw(&L->cur,"RANGECLZ")||kw(&L->cur,"CLZRANGE")||kw(&L->cur,"NLZCELL")){
    /* CLZCELL lo hi — cells[i] = clz64(cells[i]) (0 → 64) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned long u = (unsigned long)vm->cells[(int)i];
      long r = 0;
      if (u == 0) r = 64;
      else {
        for (int b = 63; b >= 0; b--){
          if (u & (1ul << (unsigned)b)) break;
          r++;
        }
      }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CTZCELL")||kw(&L->cur,"CELLCTZ")||kw(&L->cur,"BCTZCELL")||
      kw(&L->cur,"RANGECTZ")||kw(&L->cur,"CTZRANGE")||kw(&L->cur,"NTZCELL")){
    /* CTZCELL lo hi — cells[i] = ctz64(cells[i]) (0 → 64) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned long u = (unsigned long)vm->cells[(int)i];
      long r = 0;
      if (u == 0) r = 64;
      else {
        while ((u & 1ul) == 0){ r++; u >>= 1; }
      }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"BSWAP16CELL")||kw(&L->cur,"CELLBSWAP16")||kw(&L->cur,"BBSWAP16CELL")||
      kw(&L->cur,"RANGEBSWAP16")||kw(&L->cur,"BSWAP16RANGE")){
    /* BSWAP16CELL lo hi — cells[i] = bswap16(low16 cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFFFu;
      w = ((w & 0x00FFu) << 8) | ((w & 0xFF00u) >> 8);
      vm->cells[(int)i] = (long)w;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 cell bitfield width ladder: BSWAP64CELL · BITREV16CELL · ROL16CELL
   * (complete bswap 16/32/64 + bitrev 16/32 + rotate16 after BSWAP16CELL/BITREV32CELL) */
  if (kw(&L->cur,"BSWAP64CELL")||kw(&L->cur,"CELLBSWAP64")||kw(&L->cur,"BBSWAP64CELL")||
      kw(&L->cur,"RANGEBSWAP64")||kw(&L->cur,"BSWAP64RANGE")){
    /* BSWAP64CELL lo hi — cells[i] = bswap64(cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned long w = (unsigned long)vm->cells[(int)i];
      w = ((w & 0x00000000000000FFul) << 56) | ((w & 0x000000000000FF00ul) << 40) |
          ((w & 0x0000000000FF0000ul) << 24) | ((w & 0x00000000FF000000ul) << 8) |
          ((w & 0x000000FF00000000ul) >> 8) | ((w & 0x0000FF0000000000ul) >> 24) |
          ((w & 0x00FF000000000000ul) >> 40) | ((w & 0xFF00000000000000ul) >> 56);
      vm->cells[(int)i] = (long)w;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"BITREV16CELL")||kw(&L->cur,"BREV16CELL")||kw(&L->cur,"REV16CELL")||
      kw(&L->cur,"RANGEBITREV16")||kw(&L->cur,"BITREV16RANGE")||kw(&L->cur,"CELLBITREV16R")){
    /* BITREV16CELL lo hi — cells[i] = bitrev16(low16 cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFFFu;
      unsigned int rv = 0;
      for (int b = 0; b < 16; b++){
        rv = (rv << 1) | (w & 1u);
        w >>= 1;
      }
      vm->cells[(int)i] = (long)rv;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ROL16CELL")||kw(&L->cur,"ROTL16CELL")||kw(&L->cur,"BROL16CELL")||
      kw(&L->cur,"RANGEROL16")||kw(&L->cur,"ROL16RANGE")||kw(&L->cur,"CELLROTL16")){
    /* ROL16CELL lo hi k — cells[i] = rotl16(low16 cells[i], k&15) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 15;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFFFu;
      long r = (kk == 0) ? (long)w : (long)((((w << kk) | (w >> (16 - kk))) & 0xFFFFu));
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 cell fixed-width 16 shift/rotate: ROR16CELL · SHL16CELL · SHR16CELL
   * (complete 16-bit rotate + logical shift plane after ROL16CELL; dual of SROTR16/SSHL path) */
  if (kw(&L->cur,"ROR16CELL")||kw(&L->cur,"ROTR16CELL")||kw(&L->cur,"BROR16CELL")||
      kw(&L->cur,"RANGEROR16")||kw(&L->cur,"ROR16RANGE")||kw(&L->cur,"CELLROTR16")){
    /* ROR16CELL lo hi k — cells[i] = rotr16(low16 cells[i], k&15) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 15;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFFFu;
      long r = (kk == 0) ? (long)w : (long)((((w >> kk) | (w << (16 - kk))) & 0xFFFFu));
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHL16CELL")||kw(&L->cur,"LSH16CELL")||kw(&L->cur,"BSHL16CELL")||
      kw(&L->cur,"RANGESHL16")||kw(&L->cur,"SHL16RANGE")||kw(&L->cur,"CELLLSH16")){
    /* SHL16CELL lo hi k — cells[i] = (uint16)cells[i] << k (k>=16 → 0) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFFFu;
      long r = (kk >= 16) ? 0L : (long)((w << kk) & 0xFFFFu);
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHR16CELL")||kw(&L->cur,"LSHR16CELL")||kw(&L->cur,"BSHR16CELL")||
      kw(&L->cur,"RANGESHR16")||kw(&L->cur,"SHR16RANGE")||kw(&L->cur,"CELLLSHR16")){
    /* SHR16CELL lo hi k — cells[i] = (uint16)cells[i] >> k logical (k>=16 → 0) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFFFu;
      long r = (kk >= 16) ? 0L : (long)(w >> kk);
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 cell fixed-width 16 signed/bitfield: SAR16CELL · SEXT16CELL · ABS16CELL
   * (complete arithmetic shift + sign-extend + abs16 after logical SHL16/SHR16 plane) */
  if (kw(&L->cur,"SAR16CELL")||kw(&L->cur,"ASHR16CELL")||kw(&L->cur,"BSAR16CELL")||
      kw(&L->cur,"RANGESAR16")||kw(&L->cur,"SAR16RANGE")||kw(&L->cur,"CELLASHR16")){
    /* SAR16CELL lo hi k — arithmetic right shift low16 as int16 (k>=16 → all sign) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    for (long i=lo;i<=hi;i++){
      long va = (long)(short)(unsigned short)(vm->cells[(int)i] & 0xFFFFu);
      long r;
      if (kk >= 16) r = (va < 0) ? -1L : 0L;
      else r = va >> kk;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SEXT16CELL")||kw(&L->cur,"SIGNEXT16CELL")||kw(&L->cur,"BSEXT16CELL")||
      kw(&L->cur,"RANGESEXT16")||kw(&L->cur,"SEXT16RANGE")||kw(&L->cur,"CELLSEXTW")){
    /* SEXT16CELL lo hi — cells[i] = (long)(int16)low16 cells[i] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long r = (long)(short)(unsigned short)(vm->cells[(int)i] & 0xFFFFu);
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ABS16CELL")||kw(&L->cur,"CELLABS16")||kw(&L->cur,"BABS16CELL")||
      kw(&L->cur,"RANGEABS16")||kw(&L->cur,"ABS16RANGE")||kw(&L->cur,"IABS16CELL")){
    /* ABS16CELL lo hi — cells[i] = abs((int16)low16); min int16 stays 0x8000 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long va = (long)(short)(unsigned short)(vm->cells[(int)i] & 0xFFFFu);
      long r;
      if (va == (long)(short)0x8000) r = 0x8000L; /* keep min int16 */
      else if (va < 0) r = -va;
      else r = va;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 cell fixed-width 16 bit-metrics: POPCNT16CELL · CLZ16CELL · CTZ16CELL
   * (width-16 dual of POPCNTCELL/CLZCELL/CTZCELL after SAR16/SEXT16/ABS16 plane) */
  if (kw(&L->cur,"POPCNT16CELL")||kw(&L->cur,"PCNT16CELL")||kw(&L->cur,"BPOPCNT16CELL")||
      kw(&L->cur,"RANGEPOPCNT16")||kw(&L->cur,"POPCNT16RANGE")||kw(&L->cur,"CELLPOPCNT16")){
    /* POPCNT16CELL lo hi — cells[i] = popcount(low16 cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFFFu;
      long r = 0;
      while (w){ r += (long)(w & 1u); w >>= 1; }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CLZ16CELL")||kw(&L->cur,"NLZ16CELL")||kw(&L->cur,"BCLZ16CELL")||
      kw(&L->cur,"RANGECLZ16")||kw(&L->cur,"CLZ16RANGE")||kw(&L->cur,"CELLCLZ16")){
    /* CLZ16CELL lo hi — cells[i] = clz16(low16); 0 → 16 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFFFu;
      long r = 0;
      if (w == 0) r = 16;
      else {
        for (int b = 15; b >= 0; b--){
          if (w & (1u << (unsigned)b)) break;
          r++;
        }
      }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CTZ16CELL")||kw(&L->cur,"NTZ16CELL")||kw(&L->cur,"BCTZ16CELL")||
      kw(&L->cur,"RANGECTZ16")||kw(&L->cur,"CTZ16RANGE")||kw(&L->cur,"CELLCTZ16")){
    /* CTZ16CELL lo hi — cells[i] = ctz16(low16); 0 → 16 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFFFu;
      long r = 0;
      if (w == 0) r = 16;
      else {
        while ((w & 1u) == 0){ r++; w >>= 1; }
      }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 cell fixed-width 8 shift ALU: SHL8CELL · SHR8CELL · SAR8CELL
   * (byte-width dual of SHL16/SHR16/SAR16 after 16-metrics plane; complete 8/16/32 shift ladder) */
  if (kw(&L->cur,"SHL8CELL")||kw(&L->cur,"LSH8CELL")||kw(&L->cur,"BSHL8CELL")||
      kw(&L->cur,"RANGESHL8")||kw(&L->cur,"SHL8RANGE")||kw(&L->cur,"CELLLSH8")){
    /* SHL8CELL lo hi k — cells[i] = (uint8)cells[i] << k (k>=8 → 0) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFu;
      long r = (kk >= 8) ? 0L : (long)((w << kk) & 0xFFu);
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHR8CELL")||kw(&L->cur,"LSHR8CELL")||kw(&L->cur,"BSHR8CELL")||
      kw(&L->cur,"RANGESHR8")||kw(&L->cur,"SHR8RANGE")||kw(&L->cur,"CELLLSHR8")){
    /* SHR8CELL lo hi k — cells[i] = (uint8)cells[i] >> k logical (k>=8 → 0) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFu;
      long r = (kk >= 8) ? 0L : (long)(w >> kk);
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SAR8CELL")||kw(&L->cur,"ASHR8CELL")||kw(&L->cur,"BSAR8CELL")||
      kw(&L->cur,"RANGESAR8")||kw(&L->cur,"SAR8RANGE")||kw(&L->cur,"CELLASHR8")){
    /* SAR8CELL lo hi k — arithmetic right shift low8 as int8 (k>=8 → all sign) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    for (long i=lo;i<=hi;i++){
      long va = (long)(signed char)(unsigned char)(vm->cells[(int)i] & 0xFFu);
      long r;
      if (kk >= 8) r = (va < 0) ? -1L : 0L;
      else r = va >> kk;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 cell fixed-width 8 rotate/bitrev: ROL8CELL · ROR8CELL · BITREV8CELL
   * (byte dual of ROL16/ROR16/BITREV16 after SHL8 plane; complete 8/16/32 rotate+bitrev ladder) */
  if (kw(&L->cur,"ROL8CELL")||kw(&L->cur,"ROTL8CELL")||kw(&L->cur,"BROL8CELL")||
      kw(&L->cur,"RANGEROL8")||kw(&L->cur,"ROL8RANGE")||kw(&L->cur,"CELLROTL8")){
    /* ROL8CELL lo hi k — cells[i] = rotl8(low8 cells[i], k&7) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 7;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFu;
      long r = (kk == 0) ? (long)w : (long)((((w << kk) | (w >> (8 - kk))) & 0xFFu));
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ROR8CELL")||kw(&L->cur,"ROTR8CELL")||kw(&L->cur,"BROR8CELL")||
      kw(&L->cur,"RANGEROR8")||kw(&L->cur,"ROR8RANGE")||kw(&L->cur,"CELLROTR8")){
    /* ROR8CELL lo hi k — cells[i] = rotr8(low8 cells[i], k&7) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 7;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFu;
      long r = (kk == 0) ? (long)w : (long)((((w >> kk) | (w << (8 - kk))) & 0xFFu));
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"BITREV8CELL")||kw(&L->cur,"BREV8CELL")||kw(&L->cur,"REV8CELL")||
      kw(&L->cur,"RANGEBITREV8")||kw(&L->cur,"BITREV8RANGE")||kw(&L->cur,"CELLBITREV8R")){
    /* BITREV8CELL lo hi — cells[i] = bitrev8(low8 cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFu;
      unsigned int rv = 0;
      for (int b = 0; b < 8; b++){
        rv = (rv << 1) | (w & 1u);
        w >>= 1;
      }
      vm->cells[(int)i] = (long)rv;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 cell fixed-width 8 signed/metrics: SEXT8CELL · ABS8CELL · POPCNT8CELL
   * (byte dual of SEXT16/ABS16/POPCNT16 after ROL8 plane; complete signed8 + energy count) */
  if (kw(&L->cur,"SEXT8CELL")||kw(&L->cur,"SIGNEXT8CELL")||kw(&L->cur,"BSEXT8CELL")||
      kw(&L->cur,"RANGESEXT8")||kw(&L->cur,"SEXT8RANGE")||kw(&L->cur,"CELLSEXTB")){
    /* SEXT8CELL lo hi — cells[i] = (long)(int8)low8 cells[i] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long r = (long)(signed char)(unsigned char)(vm->cells[(int)i] & 0xFFu);
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ABS8CELL")||kw(&L->cur,"CELLABS8")||kw(&L->cur,"BABS8CELL")||
      kw(&L->cur,"RANGEABS8")||kw(&L->cur,"ABS8RANGE")||kw(&L->cur,"IABS8CELL")){
    /* ABS8CELL lo hi — cells[i] = abs((int8)low8); min int8 stays 0x80 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long va = (long)(signed char)(unsigned char)(vm->cells[(int)i] & 0xFFu);
      long r;
      if (va == (long)(signed char)0x80) r = 0x80L; /* keep min int8 */
      else if (va < 0) r = -va;
      else r = va;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"POPCNT8CELL")||kw(&L->cur,"PCNT8CELL")||kw(&L->cur,"BPOPCNT8CELL")||
      kw(&L->cur,"RANGEPOPCNT8")||kw(&L->cur,"POPCNT8RANGE")||kw(&L->cur,"CELLPOPCNT8")){
    /* POPCNT8CELL lo hi — cells[i] = popcount(low8 cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFu;
      long r = 0;
      while (w){ r += (long)(w & 1u); w >>= 1; }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 cell fixed-width 8 metrics foundation: CLZ8CELL · CTZ8CELL · PARITY8CELL
   * (byte dual of CLZ16/CTZ16 + parity after POPCNT8; complete 8-bit metrics foundation) */
  if (kw(&L->cur,"CLZ8CELL")||kw(&L->cur,"NLZ8CELL")||kw(&L->cur,"BCLZ8CELL")||
      kw(&L->cur,"RANGECLZ8")||kw(&L->cur,"CLZ8RANGE")||kw(&L->cur,"CELLCLZ8")){
    /* CLZ8CELL lo hi — cells[i] = clz8(low8); 0 → 8 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFu;
      long r = 0;
      if (w == 0) r = 8;
      else {
        for (int b = 7; b >= 0; b--){
          if (w & (1u << (unsigned)b)) break;
          r++;
        }
      }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CTZ8CELL")||kw(&L->cur,"NTZ8CELL")||kw(&L->cur,"BCTZ8CELL")||
      kw(&L->cur,"RANGECTZ8")||kw(&L->cur,"CTZ8RANGE")||kw(&L->cur,"CELLCTZ8")){
    /* CTZ8CELL lo hi — cells[i] = ctz8(low8); 0 → 8 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFu;
      long r = 0;
      if (w == 0) r = 8;
      else {
        while ((w & 1u) == 0){ r++; w >>= 1; }
      }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"PARITY8CELL")||kw(&L->cur,"CELLPARITY8")||kw(&L->cur,"BPARITY8CELL")||
      kw(&L->cur,"RANGEPARITY8")||kw(&L->cur,"PARITY8RANGE")||kw(&L->cur,"XORRED8CELL")){
    /* PARITY8CELL lo hi — cells[i] = xor-reduce of low8 bits (parity) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFu;
      long r = 0;
      while (w){ r ^= (long)(w & 1u); w >>= 1; }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 cell fixed-width 8 math: NEG8CELL · ZEXT8CELL · CLIP8CELL
   * (byte dual of signed/zext/clip after CLZ8 metrics; complete 8-bit arithmetic foundation) */
  if (kw(&L->cur,"NEG8CELL")||kw(&L->cur,"CELLNEG8")||kw(&L->cur,"BNEG8CELL")||
      kw(&L->cur,"RANGENEG8")||kw(&L->cur,"NEG8RANGE")||kw(&L->cur,"INEG8CELL")){
    /* NEG8CELL lo hi — cells[i] = -(int8)low8; min int8 0x80 stays 0x80 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long va = (long)(signed char)(unsigned char)(vm->cells[(int)i] & 0xFFu);
      long r;
      if (va == (long)(signed char)0x80) r = 0x80L;
      else r = -va;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ZEXT8CELL")||kw(&L->cur,"ZEROEXT8CELL")||kw(&L->cur,"BZEXT8CELL")||
      kw(&L->cur,"RANGEZEXT8")||kw(&L->cur,"ZEXT8RANGE")||kw(&L->cur,"CELLZEXTB")){
    /* ZEXT8CELL lo hi — cells[i] = low8 only (clear high bits) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((unsigned long)vm->cells[(int)i] & 0xFFul);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CLIP8CELL")||kw(&L->cur,"CELLCLIP8")||kw(&L->cur,"BCLIP8CELL")||
      kw(&L->cur,"RANGECLIP8")||kw(&L->cur,"CLIP8RANGE")||kw(&L->cur,"UCLIP8CELL")){
    /* CLIP8CELL lo hi — clamp cells[i] to unsigned 8-bit [0,255] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long r = vm->cells[(int)i];
      if (r < 0) r = 0;
      if (r > 255) r = 255;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 cell fixed-width 16 math: NEG16CELL · ZEXT16CELL · CLIP16CELL
   * (word dual of NEG8/ZEXT8/CLIP8 after SAR16/SEXT16/ABS16 plane; complete 16-bit math) */
  if (kw(&L->cur,"NEG16CELL")||kw(&L->cur,"CELLNEG16")||kw(&L->cur,"BNEG16CELL")||
      kw(&L->cur,"RANGENEG16")||kw(&L->cur,"NEG16RANGE")||kw(&L->cur,"INEG16CELL")){
    /* NEG16CELL lo hi — cells[i] = -(int16)low16; min int16 0x8000 stays 0x8000 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long va = (long)(short)(unsigned short)(vm->cells[(int)i] & 0xFFFFu);
      long r;
      if (va == (long)(short)0x8000) r = 0x8000L;
      else r = -va;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ZEXT16CELL")||kw(&L->cur,"ZEROEXT16CELL")||kw(&L->cur,"BZEXT16CELL")||
      kw(&L->cur,"RANGEZEXT16")||kw(&L->cur,"ZEXT16RANGE")||kw(&L->cur,"CELLZEXTW")){
    /* ZEXT16CELL lo hi — cells[i] = low16 only (clear high bits) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((unsigned long)vm->cells[(int)i] & 0xFFFFul);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CLIP16CELL")||kw(&L->cur,"CELLCLIP16")||kw(&L->cur,"BCLIP16CELL")||
      kw(&L->cur,"RANGECLIP16")||kw(&L->cur,"CLIP16RANGE")||kw(&L->cur,"UCLIP16CELL")){
    /* CLIP16CELL lo hi — clamp cells[i] to unsigned 16-bit [0,65535] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long r = vm->cells[(int)i];
      if (r < 0) r = 0;
      if (r > 65535L) r = 65535L;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 cell signed-clip dual ladder: CLIPS8CELL · CLIPS16CELL · CLIPS32CELL
   * (signed dual of CLIP8/CLIP16 after SCLIPS*TOC; complete 8/16/32 signed clamp plane) */
  if (kw(&L->cur,"CLIPS8CELL")||kw(&L->cur,"SCLIPS8CELL")||kw(&L->cur,"BCLIPS8CELL")||
      kw(&L->cur,"RANGECLIPS8")||kw(&L->cur,"CLIPS8RANGE")||kw(&L->cur,"SCLIP8CELL")){
    /* CLIPS8CELL lo hi — clamp cells[i] to signed 8-bit [-128,127] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long r = vm->cells[(int)i];
      if (r < -128L) r = -128L;
      if (r > 127L) r = 127L;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CLIPS16CELL")||kw(&L->cur,"SCLIPS16CELL")||kw(&L->cur,"BCLIPS16CELL")||
      kw(&L->cur,"RANGECLIPS16")||kw(&L->cur,"CLIPS16RANGE")||kw(&L->cur,"SCLIP16CELL")){
    /* CLIPS16CELL lo hi — clamp cells[i] to signed 16-bit [-32768,32767] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long r = vm->cells[(int)i];
      if (r < -32768L) r = -32768L;
      if (r > 32767L) r = 32767L;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CLIPS32CELL")||kw(&L->cur,"SCLIPS32CELL")||kw(&L->cur,"BCLIPS32CELL")||
      kw(&L->cur,"RANGECLIPS32")||kw(&L->cur,"CLIPS32RANGE")||kw(&L->cur,"SCLIP32CELL")){
    /* CLIPS32CELL lo hi — clamp cells[i] to signed 32-bit [-2147483648,2147483647] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long r = vm->cells[(int)i];
      if (r < -2147483648L) r = -2147483648L;
      if (r > 2147483647L) r = 2147483647L;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 cell fixed-width 32 math dual ladder: NEG32CELL · ZEXT32CELL · CLIP32CELL
   * (dword dual of NEG16/ZEXT16/CLIP16 after CLIPS32 plane; complete 8/16/32 math) */
  if (kw(&L->cur,"NEG32CELL")||kw(&L->cur,"CELLNEG32")||kw(&L->cur,"BNEG32CELL")||
      kw(&L->cur,"RANGENEG32")||kw(&L->cur,"NEG32RANGE")||kw(&L->cur,"INEG32CELL")){
    /* NEG32CELL lo hi — cells[i] = -(int32)low32; min int32 stays 0x80000000 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long va = (long)(int)(unsigned int)vm->cells[(int)i];
      long r;
      if (va == (long)(int)0x80000000) r = (long)(int)0x80000000;
      else r = -va;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ZEXT32CELL")||kw(&L->cur,"ZEROEXT32CELL")||kw(&L->cur,"BZEXT32CELL")||
      kw(&L->cur,"RANGEZEXT32")||kw(&L->cur,"ZEXT32RANGE")||kw(&L->cur,"CELLZEXTL")){
    /* ZEXT32CELL lo hi — cells[i] = low32 only (clear high bits) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((unsigned long)vm->cells[(int)i] & 0xFFFFFFFFul);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CLIP32CELL")||kw(&L->cur,"CELLCLIP32")||kw(&L->cur,"BCLIP32CELL")||
      kw(&L->cur,"RANGECLIP32")||kw(&L->cur,"CLIP32RANGE")||kw(&L->cur,"UCLIP32CELL")){
    /* CLIP32CELL lo hi — clamp cells[i] to unsigned 32-bit [0,4294967295] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long r = vm->cells[(int)i];
      if (r < 0) r = 0;
      if (r > 4294967295L) r = 4294967295L;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 cell fixed-width 32 signed/metrics: SEXT32CELL · ABS32CELL · POPCNT32CELL
   * (dword dual of SEXT8/ABS8/POPCNT8 after NEG32/ZEXT32/CLIP32; complete 8/16/32 signed+pop) */
  if (kw(&L->cur,"SEXT32CELL")||kw(&L->cur,"SIGNEXT32CELL")||kw(&L->cur,"BSEXT32CELL")||
      kw(&L->cur,"RANGESEXT32")||kw(&L->cur,"SEXT32RANGE")||kw(&L->cur,"CELLSEXTL")){
    /* SEXT32CELL lo hi — cells[i] = (long)(int32)low32 cells[i] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long r = (long)(int)(unsigned int)vm->cells[(int)i];
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ABS32CELL")||kw(&L->cur,"CELLABS32")||kw(&L->cur,"BABS32CELL")||
      kw(&L->cur,"RANGEABS32")||kw(&L->cur,"ABS32RANGE")||kw(&L->cur,"IABS32CELL")){
    /* ABS32CELL lo hi — cells[i] = abs((int32)low32); min int32 stays 0x80000000 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long va = (long)(int)(unsigned int)vm->cells[(int)i];
      long r;
      if (va == (long)(int)0x80000000) r = (long)(int)0x80000000; /* keep min int32 */
      else if (va < 0) r = -va;
      else r = va;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"POPCNT32CELL")||kw(&L->cur,"PCNT32CELL")||kw(&L->cur,"BPOPCNT32CELL")||
      kw(&L->cur,"RANGEPOPCNT32")||kw(&L->cur,"POPCNT32RANGE")||kw(&L->cur,"CELLPOPCNT32")){
    /* POPCNT32CELL lo hi — cells[i] = popcount(low32 cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i];
      long r = 0;
      while (w){ r += (long)(w & 1u); w >>= 1; }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 cell fixed-width 32 metrics foundation: CLZ32CELL · CTZ32CELL · PARITY32CELL
   * (dword dual of CLZ8/CTZ8/PARITY8 after POPCNT32; complete 8/16/32 metrics ladder) */
  if (kw(&L->cur,"CLZ32CELL")||kw(&L->cur,"NLZ32CELL")||kw(&L->cur,"BCLZ32CELL")||
      kw(&L->cur,"RANGECLZ32")||kw(&L->cur,"CLZ32RANGE")||kw(&L->cur,"CELLCLZ32")){
    /* CLZ32CELL lo hi — cells[i] = clz32(low32); 0 → 32 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i];
      long r = 0;
      if (w == 0) r = 32;
      else {
        for (int b = 31; b >= 0; b--){
          if (w & (1u << (unsigned)b)) break;
          r++;
        }
      }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CTZ32CELL")||kw(&L->cur,"NTZ32CELL")||kw(&L->cur,"BCTZ32CELL")||
      kw(&L->cur,"RANGECTZ32")||kw(&L->cur,"CTZ32RANGE")||kw(&L->cur,"CELLCTZ32")){
    /* CTZ32CELL lo hi — cells[i] = ctz32(low32); 0 → 32 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i];
      long r = 0;
      if (w == 0) r = 32;
      else {
        while ((w & 1u) == 0){ r++; w >>= 1; }
      }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"PARITY32CELL")||kw(&L->cur,"CELLPARITY32")||kw(&L->cur,"BPARITY32CELL")||
      kw(&L->cur,"RANGEPARITY32")||kw(&L->cur,"PARITY32RANGE")||kw(&L->cur,"XORRED32CELL")){
    /* PARITY32CELL lo hi — cells[i] = xor-reduce of low32 bits (parity) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i];
      long r = 0;
      while (w){ r ^= (long)(w & 1u); w >>= 1; }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 cell fixed-width 32 bitwise: AND32CELL · OR32CELL · XOR32CELL
   * (dword dual of ANDCELL/ORCELL/XORCELL; result low32 only after 32 metrics plane) */
  if (kw(&L->cur,"AND32CELL")||kw(&L->cur,"CELLAND32")||kw(&L->cur,"BAND32CELL")||
      kw(&L->cur,"RANGEAND32")||kw(&L->cur,"AND32RANGE")||kw(&L->cur,"UAND32CELL")){
    /* AND32CELL lo hi mask — cells[i] = low32(cells[i]) & low32(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((unsigned int)vm->cells[(int)i] & m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"OR32CELL")||kw(&L->cur,"CELLOR32")||kw(&L->cur,"BOR32CELL")||
      kw(&L->cur,"RANGEOR32")||kw(&L->cur,"OR32RANGE")||kw(&L->cur,"UOR32CELL")){
    /* OR32CELL lo hi mask — cells[i] = low32(cells[i]) | low32(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((unsigned int)vm->cells[(int)i] | m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"XOR32CELL")||kw(&L->cur,"CELLXOR32")||kw(&L->cur,"BXOR32CELL")||
      kw(&L->cur,"RANGEXOR32")||kw(&L->cur,"XOR32RANGE")||kw(&L->cur,"UXOR32CELL")){
    /* XOR32CELL lo hi mask — cells[i] = low32(cells[i]) ^ low32(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((unsigned int)vm->cells[(int)i] ^ m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 cell fixed-width 32 inverted bitwise: NAND32CELL · NOR32CELL · XNOR32CELL
   * (inverted dual of AND32/OR32/XOR32; result low32 only — complete 32 cell logic plane) */
  if (kw(&L->cur,"NAND32CELL")||kw(&L->cur,"CELLNAND32")||kw(&L->cur,"BNAND32CELL")||
      kw(&L->cur,"RANGENAND32")||kw(&L->cur,"NAND32RANGE")||kw(&L->cur,"UNAND32CELL")){
    /* NAND32CELL lo hi mask — cells[i] = ~(low32 & low32(mask)) & 0xFFFFFFFF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(~((unsigned int)vm->cells[(int)i] & m));
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"NOR32CELL")||kw(&L->cur,"CELLNOR32")||kw(&L->cur,"BNOR32CELL")||
      kw(&L->cur,"RANGENOR32")||kw(&L->cur,"NOR32RANGE")||kw(&L->cur,"UNOR32CELL")){
    /* NOR32CELL lo hi mask — cells[i] = ~(low32 | low32(mask)) & 0xFFFFFFFF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(~((unsigned int)vm->cells[(int)i] | m));
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"XNOR32CELL")||kw(&L->cur,"CELLXNOR32")||kw(&L->cur,"BXNOR32CELL")||
      kw(&L->cur,"RANGEXNOR32")||kw(&L->cur,"XNOR32RANGE")||kw(&L->cur,"NXOR32CELL")||
      kw(&L->cur,"UXNOR32CELL")){
    /* XNOR32CELL lo hi mask — cells[i] = ~(low32 ^ low32(mask)) & 0xFFFFFFFF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(~((unsigned int)vm->cells[(int)i] ^ m));
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 cell fixed-width 8 bitwise: AND8CELL · OR8CELL · XOR8CELL
   * (byte dual of AND32/OR32/XOR32 after inverted32 plane; complete 8/32 bitwise ladder start) */
  if (kw(&L->cur,"AND8CELL")||kw(&L->cur,"CELLAND8")||kw(&L->cur,"BAND8CELL")||
      kw(&L->cur,"RANGEAND8")||kw(&L->cur,"AND8RANGE")||kw(&L->cur,"UAND8CELL")){
    /* AND8CELL lo hi mask — cells[i] = low8(cells[i]) & low8(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(((unsigned int)vm->cells[(int)i] & 0xFFu) & m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"OR8CELL")||kw(&L->cur,"CELLOR8")||kw(&L->cur,"BOR8CELL")||
      kw(&L->cur,"RANGEOR8")||kw(&L->cur,"OR8RANGE")||kw(&L->cur,"UOR8CELL")){
    /* OR8CELL lo hi mask — cells[i] = low8(cells[i]) | low8(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(((unsigned int)vm->cells[(int)i] & 0xFFu) | m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"XOR8CELL")||kw(&L->cur,"CELLXOR8")||kw(&L->cur,"BXOR8CELL")||
      kw(&L->cur,"RANGEXOR8")||kw(&L->cur,"XOR8RANGE")||kw(&L->cur,"UXOR8CELL")){
    /* XOR8CELL lo hi mask — cells[i] = low8(cells[i]) ^ low8(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(((unsigned int)vm->cells[(int)i] & 0xFFu) ^ m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 cell fixed-width 8 inverted bitwise: NAND8CELL · NOR8CELL · XNOR8CELL
   * (inverted dual of AND8/OR8/XOR8; result low8 only — complete 8 cell logic foundation) */
  if (kw(&L->cur,"NAND8CELL")||kw(&L->cur,"CELLNAND8")||kw(&L->cur,"BNAND8CELL")||
      kw(&L->cur,"RANGENAND8")||kw(&L->cur,"NAND8RANGE")||kw(&L->cur,"UNAND8CELL")){
    /* NAND8CELL lo hi mask — cells[i] = ~(low8 & low8(mask)) & 0xFF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~(((unsigned int)vm->cells[(int)i] & 0xFFu) & m)) & 0xFFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"NOR8CELL")||kw(&L->cur,"CELLNOR8")||kw(&L->cur,"BNOR8CELL")||
      kw(&L->cur,"RANGENOR8")||kw(&L->cur,"NOR8RANGE")||kw(&L->cur,"UNOR8CELL")){
    /* NOR8CELL lo hi mask — cells[i] = ~(low8 | low8(mask)) & 0xFF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~(((unsigned int)vm->cells[(int)i] & 0xFFu) | m)) & 0xFFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"XNOR8CELL")||kw(&L->cur,"CELLXNOR8")||kw(&L->cur,"BXNOR8CELL")||
      kw(&L->cur,"RANGEXNOR8")||kw(&L->cur,"XNOR8RANGE")||kw(&L->cur,"NXOR8CELL")||
      kw(&L->cur,"UXNOR8CELL")){
    /* XNOR8CELL lo hi mask — cells[i] = ~(low8 ^ low8(mask)) & 0xFF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~(((unsigned int)vm->cells[(int)i] & 0xFFu) ^ m)) & 0xFFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 cell fixed-width 16 bitwise: AND16CELL · OR16CELL · XOR16CELL
   * (word dual of AND8/OR8/XOR8 after inverted8 plane; complete 8/16/32 bitwise ladder) */
  if (kw(&L->cur,"AND16CELL")||kw(&L->cur,"CELLAND16")||kw(&L->cur,"BAND16CELL")||
      kw(&L->cur,"RANGEAND16")||kw(&L->cur,"AND16RANGE")||kw(&L->cur,"UAND16CELL")){
    /* AND16CELL lo hi mask — cells[i] = low16(cells[i]) & low16(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(((unsigned int)vm->cells[(int)i] & 0xFFFFu) & m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"OR16CELL")||kw(&L->cur,"CELLOR16")||kw(&L->cur,"BOR16CELL")||
      kw(&L->cur,"RANGEOR16")||kw(&L->cur,"OR16RANGE")||kw(&L->cur,"UOR16CELL")){
    /* OR16CELL lo hi mask — cells[i] = low16(cells[i]) | low16(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(((unsigned int)vm->cells[(int)i] & 0xFFFFu) | m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"XOR16CELL")||kw(&L->cur,"CELLXOR16")||kw(&L->cur,"BXOR16CELL")||
      kw(&L->cur,"RANGEXOR16")||kw(&L->cur,"XOR16RANGE")||kw(&L->cur,"UXOR16CELL")){
    /* XOR16CELL lo hi mask — cells[i] = low16(cells[i]) ^ low16(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(((unsigned int)vm->cells[(int)i] & 0xFFFFu) ^ m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 cell fixed-width 16 inverted bitwise: NAND16CELL · NOR16CELL · XNOR16CELL
   * (inverted dual of AND16/OR16/XOR16; result low16 only — complete 8/16/32 inverted ladder) */
  if (kw(&L->cur,"NAND16CELL")||kw(&L->cur,"CELLNAND16")||kw(&L->cur,"BNAND16CELL")||
      kw(&L->cur,"RANGENAND16")||kw(&L->cur,"NAND16RANGE")||kw(&L->cur,"UNAND16CELL")){
    /* NAND16CELL lo hi mask — cells[i] = ~(low16 & low16(mask)) & 0xFFFF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~(((unsigned int)vm->cells[(int)i] & 0xFFFFu) & m)) & 0xFFFFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"NOR16CELL")||kw(&L->cur,"CELLNOR16")||kw(&L->cur,"BNOR16CELL")||
      kw(&L->cur,"RANGENOR16")||kw(&L->cur,"NOR16RANGE")||kw(&L->cur,"UNOR16CELL")){
    /* NOR16CELL lo hi mask — cells[i] = ~(low16 | low16(mask)) & 0xFFFF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~(((unsigned int)vm->cells[(int)i] & 0xFFFFu) | m)) & 0xFFFFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"XNOR16CELL")||kw(&L->cur,"CELLXNOR16")||kw(&L->cur,"BXNOR16CELL")||
      kw(&L->cur,"RANGEXNOR16")||kw(&L->cur,"XNOR16RANGE")||kw(&L->cur,"NXOR16CELL")||
      kw(&L->cur,"UXNOR16CELL")){
    /* XNOR16CELL lo hi mask — cells[i] = ~(low16 ^ low16(mask)) & 0xFFFF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFFFFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~(((unsigned int)vm->cells[(int)i] & 0xFFFFu) ^ m)) & 0xFFFFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 cell fixed-width unary NOT: NOT8CELL · NOT16CELL · NOT32CELL
   * (width dual of NOTCELL; complete 8/16/32 bitwise unary after inverted ladder) */
  if (kw(&L->cur,"NOT8CELL")||kw(&L->cur,"CELLNOT8")||kw(&L->cur,"BNOT8CELL")||
      kw(&L->cur,"RANGENOT8")||kw(&L->cur,"NOT8RANGE")||kw(&L->cur,"INV8CELL")){
    /* NOT8CELL lo hi — cells[i] = ~low8 & 0xFF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~((unsigned int)vm->cells[(int)i] & 0xFFu)) & 0xFFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"NOT16CELL")||kw(&L->cur,"CELLNOT16")||kw(&L->cur,"BNOT16CELL")||
      kw(&L->cur,"RANGENOT16")||kw(&L->cur,"NOT16RANGE")||kw(&L->cur,"INV16CELL")){
    /* NOT16CELL lo hi — cells[i] = ~low16 & 0xFFFF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~((unsigned int)vm->cells[(int)i] & 0xFFFFu)) & 0xFFFFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"NOT32CELL")||kw(&L->cur,"CELLNOT32")||kw(&L->cur,"BNOT32CELL")||
      kw(&L->cur,"RANGENOT32")||kw(&L->cur,"NOT32RANGE")||kw(&L->cur,"INV32CELL")){
    /* NOT32CELL lo hi — cells[i] = ~low32 as u32 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(~(unsigned int)vm->cells[(int)i]);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 cell fixed-width 4 nibble bitwise: AND4CELL · OR4CELL · XOR4CELL
   * (nibble dual of AND8/OR8/XOR8 after NOT width plane; complete 4/8/16/32 bitwise base) */
  if (kw(&L->cur,"AND4CELL")||kw(&L->cur,"CELLAND4")||kw(&L->cur,"BAND4CELL")||
      kw(&L->cur,"RANGEAND4")||kw(&L->cur,"AND4RANGE")||kw(&L->cur,"UAND4CELL")||
      kw(&L->cur,"ANDNIBCELL")){
    /* AND4CELL lo hi mask — cells[i] = low4(cells[i]) & low4(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(((unsigned int)vm->cells[(int)i] & 0xFu) & m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"OR4CELL")||kw(&L->cur,"CELLOR4")||kw(&L->cur,"BOR4CELL")||
      kw(&L->cur,"RANGEOR4")||kw(&L->cur,"OR4RANGE")||kw(&L->cur,"UOR4CELL")||
      kw(&L->cur,"ORNIBCELL")){
    /* OR4CELL lo hi mask — cells[i] = low4(cells[i]) | low4(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(((unsigned int)vm->cells[(int)i] & 0xFu) | m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"XOR4CELL")||kw(&L->cur,"CELLXOR4")||kw(&L->cur,"BXOR4CELL")||
      kw(&L->cur,"RANGEXOR4")||kw(&L->cur,"XOR4RANGE")||kw(&L->cur,"UXOR4CELL")||
      kw(&L->cur,"XORNIBCELL")){
    /* XOR4CELL lo hi mask — cells[i] = low4(cells[i]) ^ low4(mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)(((unsigned int)vm->cells[(int)i] & 0xFu) ^ m);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 cell fixed-width 4 nibble inverted bitwise: NAND4CELL · NOR4CELL · XNOR4CELL
   * (inverted dual of AND4/OR4/XOR4; result low4 only — complete 4/8/16/32 inverted ladder) */
  if (kw(&L->cur,"NAND4CELL")||kw(&L->cur,"CELLNAND4")||kw(&L->cur,"BNAND4CELL")||
      kw(&L->cur,"RANGENAND4")||kw(&L->cur,"NAND4RANGE")||kw(&L->cur,"UNAND4CELL")||
      kw(&L->cur,"NANDNIBCELL")){
    /* NAND4CELL lo hi mask — cells[i] = ~(low4 & low4(mask)) & 0xF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~(((unsigned int)vm->cells[(int)i] & 0xFu) & m)) & 0xFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"NOR4CELL")||kw(&L->cur,"CELLNOR4")||kw(&L->cur,"BNOR4CELL")||
      kw(&L->cur,"RANGENOR4")||kw(&L->cur,"NOR4RANGE")||kw(&L->cur,"UNOR4CELL")||
      kw(&L->cur,"NORNIBCELL")){
    /* NOR4CELL lo hi mask — cells[i] = ~(low4 | low4(mask)) & 0xF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~(((unsigned int)vm->cells[(int)i] & 0xFu) | m)) & 0xFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"XNOR4CELL")||kw(&L->cur,"CELLXNOR4")||kw(&L->cur,"BXNOR4CELL")||
      kw(&L->cur,"RANGEXNOR4")||kw(&L->cur,"XNOR4RANGE")||kw(&L->cur,"NXOR4CELL")||
      kw(&L->cur,"UXNOR4CELL")||kw(&L->cur,"XNORNIBCELL")){
    /* XNOR4CELL lo hi mask — cells[i] = ~(low4 ^ low4(mask)) & 0xF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mask = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    unsigned int m = (unsigned int)mask & 0xFu;
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~(((unsigned int)vm->cells[(int)i] & 0xFu) ^ m)) & 0xFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 cell fixed-width 4 nibble shift ALU: SHL4CELL · SHR4CELL · SAR4CELL
   * (nibble dual of SHL8/SHR8/SAR8 after inverted nibble plane; complete 4/8/16/32 shift ladder) */
  if (kw(&L->cur,"SHL4CELL")||kw(&L->cur,"LSH4CELL")||kw(&L->cur,"BSHL4CELL")||
      kw(&L->cur,"RANGESHL4")||kw(&L->cur,"SHL4RANGE")||kw(&L->cur,"CELLLSH4")||
      kw(&L->cur,"SHLNIBCELL")){
    /* SHL4CELL lo hi k — cells[i] = (uint4)cells[i] << k (k>=4 → 0) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFu;
      long r = (kk >= 4) ? 0L : (long)((w << kk) & 0xFu);
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHR4CELL")||kw(&L->cur,"LSHR4CELL")||kw(&L->cur,"BSHR4CELL")||
      kw(&L->cur,"RANGESHR4")||kw(&L->cur,"SHR4RANGE")||kw(&L->cur,"CELLLSHR4")||
      kw(&L->cur,"SHRNIBCELL")){
    /* SHR4CELL lo hi k — cells[i] = (uint4)cells[i] >> k logical (k>=4 → 0) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFu;
      long r = (kk >= 4) ? 0L : (long)(w >> kk);
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SAR4CELL")||kw(&L->cur,"ASHR4CELL")||kw(&L->cur,"BSAR4CELL")||
      kw(&L->cur,"RANGESAR4")||kw(&L->cur,"SAR4RANGE")||kw(&L->cur,"CELLASHR4")||
      kw(&L->cur,"SARNIBCELL")){
    /* SAR4CELL lo hi k — arithmetic right shift low4 as int4 (k>=4 → all sign) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    for (long i=lo;i<=hi;i++){
      long va = (long)((unsigned int)vm->cells[(int)i] & 0xFu);
      if (va & 0x8) va -= 16; /* sign-extend nibble → int4 [-8..7] */
      long r;
      if (kk >= 4) r = (va < 0) ? -1L : 0L;
      else r = va >> kk;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 cell fixed-width 4 nibble signed/metrics: SEXT4CELL · ABS4CELL · POPCNT4CELL
   * (nibble dual of SEXT8/ABS8/POPCNT8 after SAR4 plane; complete 4/8/16/32 signed+pop) */
  if (kw(&L->cur,"SEXT4CELL")||kw(&L->cur,"SIGNEXT4CELL")||kw(&L->cur,"BSEXT4CELL")||
      kw(&L->cur,"RANGESEXT4")||kw(&L->cur,"SEXT4RANGE")||kw(&L->cur,"CELLSEXTNIB")||
      kw(&L->cur,"SEXTNIBCELL")){
    /* SEXT4CELL lo hi — cells[i] = (long)(int4)low4 cells[i] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long va = (long)((unsigned int)vm->cells[(int)i] & 0xFu);
      if (va & 0x8) va -= 16;
      vm->cells[(int)i] = va;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ABS4CELL")||kw(&L->cur,"CELLABS4")||kw(&L->cur,"BABS4CELL")||
      kw(&L->cur,"RANGEABS4")||kw(&L->cur,"ABS4RANGE")||kw(&L->cur,"IABS4CELL")||
      kw(&L->cur,"ABSNIBCELL")){
    /* ABS4CELL lo hi — cells[i] = abs((int4)low4); min int4 -8 stays 8 (0x8) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long va = (long)((unsigned int)vm->cells[(int)i] & 0xFu);
      if (va & 0x8) va -= 16;
      long r;
      if (va == -8L) r = 8L; /* keep min int4 magnitude as 0x8 */
      else if (va < 0) r = -va;
      else r = va;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"POPCNT4CELL")||kw(&L->cur,"PCNT4CELL")||kw(&L->cur,"BPOPCNT4CELL")||
      kw(&L->cur,"RANGEPOPCNT4")||kw(&L->cur,"POPCNT4RANGE")||kw(&L->cur,"CELLPOPCNT4")||
      kw(&L->cur,"POPCNTNIBCELL")){
    /* POPCNT4CELL lo hi — cells[i] = popcount(low4 cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFu;
      long r = 0;
      while (w){ r += (long)(w & 1u); w >>= 1; }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 cell fixed-width 4 nibble rotate/bitrev: ROL4CELL · ROR4CELL · BITREV4CELL
   * (nibble dual of ROL8/ROR8/BITREV8 after SEXT4/ABS4/POPCNT4; complete 4/8/16/32 rotate ladder) */
  if (kw(&L->cur,"ROL4CELL")||kw(&L->cur,"ROTL4CELL")||kw(&L->cur,"BROL4CELL")||
      kw(&L->cur,"RANGEROL4")||kw(&L->cur,"ROL4RANGE")||kw(&L->cur,"CELLROTL4")||
      kw(&L->cur,"ROLNIBCELL")){
    /* ROL4CELL lo hi k — cells[i] = rotl4(low4 cells[i], k&3) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 3;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFu;
      long r = (kk == 0) ? (long)w : (long)((((w << kk) | (w >> (4 - kk))) & 0xFu));
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ROR4CELL")||kw(&L->cur,"ROTR4CELL")||kw(&L->cur,"BROR4CELL")||
      kw(&L->cur,"RANGEROR4")||kw(&L->cur,"ROR4RANGE")||kw(&L->cur,"CELLROTR4")||
      kw(&L->cur,"RORNIBCELL")){
    /* ROR4CELL lo hi k — cells[i] = rotr4(low4 cells[i], k&3) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 3;
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFu;
      long r = (kk == 0) ? (long)w : (long)((((w >> kk) | (w << (4 - kk))) & 0xFu));
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"BITREV4CELL")||kw(&L->cur,"BREV4CELL")||kw(&L->cur,"REV4CELL")||
      kw(&L->cur,"RANGEBITREV4")||kw(&L->cur,"BITREV4RANGE")||kw(&L->cur,"CELLBITREV4")||
      kw(&L->cur,"BITREVNIBCELL")){
    /* BITREV4CELL lo hi — cells[i] = bitrev4(low4 cells[i]) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFu;
      unsigned int rv = 0;
      for (int b = 0; b < 4; b++){
        rv = (rv << 1) | (w & 1u);
        w >>= 1;
      }
      vm->cells[(int)i] = (long)rv;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 cell fixed-width 4 nibble math: NEG4CELL · ZEXT4CELL · CLIP4CELL
   * (nibble dual of NEG8/ZEXT8/CLIP8 after ROL4 plane; complete 4/8/16/32 arithmetic foundation) */
  if (kw(&L->cur,"NEG4CELL")||kw(&L->cur,"CELLNEG4")||kw(&L->cur,"BNEG4CELL")||
      kw(&L->cur,"RANGENEG4")||kw(&L->cur,"NEG4RANGE")||kw(&L->cur,"INEG4CELL")||
      kw(&L->cur,"NEGNIBCELL")){
    /* NEG4CELL lo hi — cells[i] = -(int4)low4; min int4 -8 stays -8 (0x8 as -8 after sext) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long va = (long)((unsigned int)vm->cells[(int)i] & 0xFu);
      if (va & 0x8) va -= 16;
      long r;
      if (va == -8L) r = -8L; /* keep min int4 */
      else r = -va;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ZEXT4CELL")||kw(&L->cur,"ZEROEXT4CELL")||kw(&L->cur,"BZEXT4CELL")||
      kw(&L->cur,"RANGEZEXT4")||kw(&L->cur,"ZEXT4RANGE")||kw(&L->cur,"CELLZEXTNIB")||
      kw(&L->cur,"ZEXTNIBCELL")){
    /* ZEXT4CELL lo hi — cells[i] = low4 only (clear high bits) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((unsigned long)vm->cells[(int)i] & 0xFul);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CLIP4CELL")||kw(&L->cur,"CELLCLIP4")||kw(&L->cur,"BCLIP4CELL")||
      kw(&L->cur,"RANGECLIP4")||kw(&L->cur,"CLIP4RANGE")||kw(&L->cur,"UCLIP4CELL")||
      kw(&L->cur,"CLIPNIBCELL")){
    /* CLIP4CELL lo hi — clamp cells[i] to unsigned 4-bit [0,15] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long r = vm->cells[(int)i];
      if (r < 0) r = 0;
      if (r > 15) r = 15;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 cell fixed-width 4 nibble metrics foundation: CLZ4CELL · CTZ4CELL · PARITY4CELL
   * (nibble dual of CLZ8/CTZ8/PARITY8 after NEG4/ZEXT4/CLIP4; complete 4/8/16/32 metrics) */
  if (kw(&L->cur,"CLZ4CELL")||kw(&L->cur,"NLZ4CELL")||kw(&L->cur,"BCLZ4CELL")||
      kw(&L->cur,"RANGECLZ4")||kw(&L->cur,"CLZ4RANGE")||kw(&L->cur,"CELLCLZ4")||
      kw(&L->cur,"CLZNIBCELL")){
    /* CLZ4CELL lo hi — cells[i] = clz4(low4); 0 → 4 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFu;
      long r = 0;
      if (w == 0) r = 4;
      else {
        for (int b = 3; b >= 0; b--){
          if (w & (1u << (unsigned)b)) break;
          r++;
        }
      }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CTZ4CELL")||kw(&L->cur,"NTZ4CELL")||kw(&L->cur,"BCTZ4CELL")||
      kw(&L->cur,"RANGECTZ4")||kw(&L->cur,"CTZ4RANGE")||kw(&L->cur,"CELLCTZ4")||
      kw(&L->cur,"CTZNIBCELL")){
    /* CTZ4CELL lo hi — cells[i] = ctz4(low4); 0 → 4 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFu;
      long r = 0;
      if (w == 0) r = 4;
      else {
        while ((w & 1u) == 0){ r++; w >>= 1; }
      }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"PARITY4CELL")||kw(&L->cur,"CELLPARITY4")||kw(&L->cur,"BPARITY4CELL")||
      kw(&L->cur,"RANGEPARITY4")||kw(&L->cur,"PARITY4RANGE")||kw(&L->cur,"XORRED4CELL")||
      kw(&L->cur,"PARITYNIBCELL")){
    /* PARITY4CELL lo hi — cells[i] = xor-reduce of low4 bits (parity) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFu;
      long r = 0;
      while (w){ r ^= (long)(w & 1u); w >>= 1; }
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 cell fixed-width 4 nibble unary/signed: NOT4CELL · CLIPS4CELL
   * (nibble dual of NOT8 + CLIPS8 after metrics plane; complete 4/8/16/32 unary+sclip) */
  if (kw(&L->cur,"NOT4CELL")||kw(&L->cur,"CELLNOT4")||kw(&L->cur,"BNOT4CELL")||
      kw(&L->cur,"RANGENOT4")||kw(&L->cur,"NOT4RANGE")||kw(&L->cur,"INV4CELL")||
      kw(&L->cur,"NOTNIBCELL")){
    /* NOT4CELL lo hi — cells[i] = ~low4 & 0xF */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++)
      vm->cells[(int)i] = (long)((~((unsigned int)vm->cells[(int)i] & 0xFu)) & 0xFu);
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CLIPS4CELL")||kw(&L->cur,"SCLIPS4CELL")||kw(&L->cur,"BCLIPS4CELL")||
      kw(&L->cur,"RANGECLIPS4")||kw(&L->cur,"CLIPS4RANGE")||kw(&L->cur,"SCLIP4CELL")||
      kw(&L->cur,"CLIPSNIBCELL")){
    /* CLIPS4CELL lo hi — clamp cells[i] to signed 4-bit [-8,7] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long r = vm->cells[(int)i];
      if (r < -8L) r = -8L;
      if (r > 7L) r = 7L;
      vm->cells[(int)i] = r;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"NOTCELL")||kw(&L->cur,"CELLNOT")||kw(&L->cur,"BNOTCELL")||kw(&L->cur,"INVCELL")){
    /* NOTCELL lo hi — bitwise NOT (~) each cell in range */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] = ~vm->cells[(int)i];
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"EQCELL")||kw(&L->cur,"CELLEQ")||kw(&L->cur,"CMPEQCELL")){
    /* EQCELL lo hi val — set cell to 1 if == val else 0 (predicate mask) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long val = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long hits = 0;
    for (long i=lo;i<=hi;i++){
      long eq = (vm->cells[(int)i] == val) ? 1 : 0;
      vm->cells[(int)i] = eq;
      hits += eq;
    }
    var_set_num(vm,"LAST_N",hits); vm->last_n=hits;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"NECELL")||kw(&L->cur,"CELLNE")||kw(&L->cur,"CMPNECELL")||
      kw(&L->cur,"NOTEQCELL")){
    /* NECELL lo hi val — set cell to 1 if != val else 0 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long val = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long hits = 0;
    for (long i=lo;i<=hi;i++){
      long ne = (vm->cells[(int)i] != val) ? 1 : 0;
      vm->cells[(int)i] = ne;
      hits += ne;
    }
    var_set_num(vm,"LAST_N",hits); vm->last_n=hits;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 cell relational predicates: LTCELL GTCELL LECELL GECELL (complete EQ/NE plane) */
  if (kw(&L->cur,"LTCELL")||kw(&L->cur,"CELLLT")||kw(&L->cur,"CMPLTCELL")||
      kw(&L->cur,"GTCELL")||kw(&L->cur,"CELLGT")||kw(&L->cur,"CMPGTCELL")||
      kw(&L->cur,"LECELL")||kw(&L->cur,"CELLLE")||kw(&L->cur,"CMPLECELL")||
      kw(&L->cur,"GECELL")||kw(&L->cur,"CELLGE")||kw(&L->cur,"CMPGECELL")){
    /* LTCELL/GTCELL/LECELL/GECELL lo hi val — 0/1 mask vs val; LAST_N = hits */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long val = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int mode = 0; /* 0=lt 1=gt 2=le 3=ge */
    if (strcmp(op,"GTCELL")==0 || strcmp(op,"CELLGT")==0 || strcmp(op,"CMPGTCELL")==0) mode = 1;
    else if (strcmp(op,"LECELL")==0 || strcmp(op,"CELLLE")==0 || strcmp(op,"CMPLECELL")==0) mode = 2;
    else if (strcmp(op,"GECELL")==0 || strcmp(op,"CELLGE")==0 || strcmp(op,"CMPGECELL")==0) mode = 3;
    long hits = 0;
    for (long i=lo;i<=hi;i++){
      long c = vm->cells[(int)i];
      long bit = 0;
      if (mode==0) bit = (c < val) ? 1 : 0;
      else if (mode==1) bit = (c > val) ? 1 : 0;
      else if (mode==2) bit = (c <= val) ? 1 : 0;
      else bit = (c >= val) ? 1 : 0;
      vm->cells[(int)i] = bit;
      hits += bit;
    }
    var_set_num(vm,"LAST_N",hits); vm->last_n=hits;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 cell mux: MUXCELL dst_lo a_lo b_lo mask_lo n
   * dst[i] = mask[i] ? a[i] : b[i]  for i in 0..n-1 (overlap-safe via temp) */
  if (kw(&L->cur,"MUXCELL")||kw(&L->cur,"CELLMUX")||kw(&L->cur,"SELECTCELL")||
      kw(&L->cur,"BLENDCELL")||kw(&L->cur,"CELLBLEND")){
    lex_next(L);
    long dlo = parse_expr(vm,L);
    long alo = parse_expr(vm,L);
    long blo = parse_expr(vm,L);
    long mlo = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (n > CUBALC_CELL_N) n = CUBALC_CELL_N;
    long tmp[CUBALC_CELL_N];
    long wrote = 0;
    for (long i=0;i<n;i++){
      long mi = mlo + i, ai = alo + i, bi = blo + i;
      long av = (ai >= 0 && ai < CUBALC_CELL_N) ? vm->cells[(int)ai] : 0;
      long bv = (bi >= 0 && bi < CUBALC_CELL_N) ? vm->cells[(int)bi] : 0;
      long mv = (mi >= 0 && mi < CUBALC_CELL_N) ? vm->cells[(int)mi] : 0;
      tmp[i] = mv ? av : bv;
    }
    for (long i=0;i<n;i++){
      long di = dlo + i;
      if (di >= 0 && di < CUBALC_CELL_N){ vm->cells[(int)di] = tmp[i]; wrote++; }
    }
    var_set_num(vm,"LAST_N",wrote); vm->last_n=wrote;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 cell range predicates: ANYCELL ALLCELL NONECELL NZCOUNT EQRANGE */
  if (kw(&L->cur,"ANYCELL")||kw(&L->cur,"CELLANY")||kw(&L->cur,"ANYNZ")||
      kw(&L->cur,"ALLCELL")||kw(&L->cur,"CELLALL")||kw(&L->cur,"ALLNZ")||
      kw(&L->cur,"NONECELL")||kw(&L->cur,"CELLNONE")||kw(&L->cur,"NONENZ")||
      kw(&L->cur,"NZCOUNT")||kw(&L->cur,"COUNTNZCELL")||kw(&L->cur,"CELLNZCOUNT")||
      kw(&L->cur,"CNZCELL")){
    /* ANY/ALL/NONE: nonzero tests; NZCOUNT: count of nonzero cells in [lo,hi] */
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long nz = 0, n = hi - lo + 1;
    for (long i=lo;i<=hi;i++) if (vm->cells[(int)i] != 0) nz++;
    long r = nz;
    if (strcmp(op,"ANYCELL")==0 || strcmp(op,"CELLANY")==0 || strcmp(op,"ANYNZ")==0)
      r = (nz > 0) ? 1 : 0;
    else if (strcmp(op,"ALLCELL")==0 || strcmp(op,"CELLALL")==0 || strcmp(op,"ALLNZ")==0)
      r = (n > 0 && nz == n) ? 1 : 0;
    else if (strcmp(op,"NONECELL")==0 || strcmp(op,"CELLNONE")==0 || strcmp(op,"NONENZ")==0)
      r = (nz == 0) ? 1 : 0;
    /* else NZCOUNT / COUNTNZCELL / CELLNZCOUNT / CNZCELL → nz */
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SET",nz);
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"EQRANGE")||kw(&L->cur,"RANGEEQ")||kw(&L->cur,"CELLEQRANGE")||
      kw(&L->cur,"EQCELLS")||kw(&L->cur,"SAMECELLS")){
    /* EQRANGE a_lo b_lo n — 1 if cells[a+i]==cells[b+i] for i in 0..n-1 */
    lex_next(L);
    long alo = parse_expr(vm,L);
    long blo = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (n > CUBALC_CELL_N) n = CUBALC_CELL_N;
    int ok = 1;
    long checked = 0;
    for (long i=0;i<n;i++){
      long ai = alo + i, bi = blo + i;
      long av = (ai >= 0 && ai < CUBALC_CELL_N) ? vm->cells[(int)ai] : 0;
      long bv = (bi >= 0 && bi < CUBALC_CELL_N) ? vm->cells[(int)bi] : 0;
      checked++;
      if (av != bv){ ok = 0; break; }
    }
    long r = ok ? 1 : 0;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SET",checked);
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 cell-logic stack duals: SANDCELL SORCELL SXORCELL SNOTCELL SEQCELL SNECELL */
  if (kw(&L->cur,"SANDCELL")||kw(&L->cur,"SCELLAND")||kw(&L->cur,"STACKANDCELL")||
      kw(&L->cur,"SANDC")){
    /* lo hi mask (stack) — AND each cell with mask */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long mask = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] &= mask;
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORCELL")||kw(&L->cur,"SCELLOR")||kw(&L->cur,"STACKORCELL")||
      kw(&L->cur,"SORC")){
    /* lo hi mask (stack) — OR each cell with mask */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long mask = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] |= mask;
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXORCELL")||kw(&L->cur,"SCELLXOR")||kw(&L->cur,"STACKXORCELL")||
      kw(&L->cur,"SXORC")){
    /* lo hi mask (stack) — XOR each cell with mask */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long mask = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] ^= mask;
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNOTCELL")||kw(&L->cur,"SCELLNOT")||kw(&L->cur,"STACKNOTCELL")||
      kw(&L->cur,"SINVCELL")||kw(&L->cur,"SNOTC")){
    /* lo hi (stack) — bitwise NOT each cell */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] = ~vm->cells[(int)i];
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCELLEQ")||kw(&L->cur,"STACKEQCELL")||kw(&L->cur,"SEQC")||
      kw(&L->cur,"SEQCELLS")||kw(&L->cur,"CMPEQCELLS")){
    /* lo hi val (stack) — predicate mask == val; LAST_N = hit count
     * NOTE: not SEQCELL — that alias is IOTA/RANGECELL */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long val = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long hits = 0;
    for (long i=lo;i<=hi;i++){
      long eq = (vm->cells[(int)i] == val) ? 1 : 0;
      vm->cells[(int)i] = eq;
      hits += eq;
    }
    var_set_num(vm,"LAST_N",hits); vm->last_n=hits;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNECELL")||kw(&L->cur,"SCELLNE")||kw(&L->cur,"STACKNECELL")||
      kw(&L->cur,"SNEC")){
    /* lo hi val (stack) — predicate mask != val; LAST_N = hit count */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long val = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long hits = 0;
    for (long i=lo;i<=hi;i++){
      long ne = (vm->cells[(int)i] != val) ? 1 : 0;
      vm->cells[(int)i] = ne;
      hits += ne;
    }
    var_set_num(vm,"LAST_N",hits); vm->last_n=hits;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack duals of cell relational: SLTCELL SGTCELL SLECELL SGECELL */
  if (kw(&L->cur,"SLTCELL")||kw(&L->cur,"SCELLLT")||kw(&L->cur,"STACKLTCELL")||
      kw(&L->cur,"SLTC")||
      kw(&L->cur,"SGTCELL")||kw(&L->cur,"SCELLGT")||kw(&L->cur,"STACKGTCELL")||
      kw(&L->cur,"SGTC")||
      kw(&L->cur,"SLECELL")||kw(&L->cur,"SCELLLE")||kw(&L->cur,"STACKLECELL")||
      kw(&L->cur,"SLEC")||
      kw(&L->cur,"SGECELL")||kw(&L->cur,"SCELLGE")||kw(&L->cur,"STACKGECELL")||
      kw(&L->cur,"SGEC")){
    /* lo hi val (stack) — relational 0/1 mask; LAST_N = hits */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long val = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int mode = 0;
    if (strcmp(op,"SGTCELL")==0 || strcmp(op,"SCELLGT")==0 ||
        strcmp(op,"STACKGTCELL")==0 || strcmp(op,"SGTC")==0) mode = 1;
    else if (strcmp(op,"SLECELL")==0 || strcmp(op,"SCELLLE")==0 ||
             strcmp(op,"STACKLECELL")==0 || strcmp(op,"SLEC")==0) mode = 2;
    else if (strcmp(op,"SGECELL")==0 || strcmp(op,"SCELLGE")==0 ||
             strcmp(op,"STACKGECELL")==0 || strcmp(op,"SGEC")==0) mode = 3;
    long hits = 0;
    for (long i=lo;i<=hi;i++){
      long c = vm->cells[(int)i];
      long bit = 0;
      if (mode==0) bit = (c < val) ? 1 : 0;
      else if (mode==1) bit = (c > val) ? 1 : 0;
      else if (mode==2) bit = (c <= val) ? 1 : 0;
      else bit = (c >= val) ? 1 : 0;
      vm->cells[(int)i] = bit;
      hits += bit;
    }
    var_set_num(vm,"LAST_N",hits); vm->last_n=hits;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 SMUXCELL — stack: dlo alo blo mlo n → mux cell plane */
  if (kw(&L->cur,"SMUXCELL")||kw(&L->cur,"SCELLMUX")||kw(&L->cur,"STACKMUXCELL")||
      kw(&L->cur,"SMUXC")||kw(&L->cur,"SSELECTCELL")||kw(&L->cur,"SBLENDCELL")){
    lex_next(L);
    if (vm->sp < 5){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long mlo = vm->stack[--vm->sp];
    long blo = vm->stack[--vm->sp];
    long alo = vm->stack[--vm->sp];
    long dlo = vm->stack[--vm->sp];
    if (n < 0) n = 0;
    if (n > CUBALC_CELL_N) n = CUBALC_CELL_N;
    long tmp[CUBALC_CELL_N];
    long wrote = 0;
    for (long i=0;i<n;i++){
      long mi = mlo + i, ai = alo + i, bi = blo + i;
      long av = (ai >= 0 && ai < CUBALC_CELL_N) ? vm->cells[(int)ai] : 0;
      long bv = (bi >= 0 && bi < CUBALC_CELL_N) ? vm->cells[(int)bi] : 0;
      long mv = (mi >= 0 && mi < CUBALC_CELL_N) ? vm->cells[(int)mi] : 0;
      tmp[i] = mv ? av : bv;
    }
    for (long i=0;i<n;i++){
      long di = dlo + i;
      if (di >= 0 && di < CUBALC_CELL_N){ vm->cells[(int)di] = tmp[i]; wrote++; }
    }
    var_set_num(vm,"LAST_N",wrote); vm->last_n=wrote;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack duals: SANYCELL SALLCELL SNONECELL SNZCOUNT SEQRANGES */
  if (kw(&L->cur,"SANYCELL")||kw(&L->cur,"SCELLANY")||kw(&L->cur,"STACKANYCELL")||
      kw(&L->cur,"SANYC")||
      kw(&L->cur,"SALLCELL")||kw(&L->cur,"SCELLALL")||kw(&L->cur,"STACKALLCELL")||
      kw(&L->cur,"SALLC")||
      kw(&L->cur,"SNONECELL")||kw(&L->cur,"SCELLNONE")||kw(&L->cur,"STACKNONECELL")||
      kw(&L->cur,"SNONEC")||
      kw(&L->cur,"SNZCOUNT")||kw(&L->cur,"SCELLNZCOUNT")||kw(&L->cur,"STACKNZCOUNT")||
      kw(&L->cur,"SNZC")){
    /* lo hi (stack) — cell-range nonzero predicates / count
     * NOTE: not SCOUNTNZ — that is stack-fold over data stack */
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long nz = 0, n = hi - lo + 1;
    for (long i=lo;i<=hi;i++) if (vm->cells[(int)i] != 0) nz++;
    long r = nz;
    if (strcmp(op,"SANYCELL")==0 || strcmp(op,"SCELLANY")==0 ||
        strcmp(op,"STACKANYCELL")==0 || strcmp(op,"SANYC")==0)
      r = (nz > 0) ? 1 : 0;
    else if (strcmp(op,"SALLCELL")==0 || strcmp(op,"SCELLALL")==0 ||
             strcmp(op,"STACKALLCELL")==0 || strcmp(op,"SALLC")==0)
      r = (n > 0 && nz == n) ? 1 : 0;
    else if (strcmp(op,"SNONECELL")==0 || strcmp(op,"SCELLNONE")==0 ||
             strcmp(op,"STACKNONECELL")==0 || strcmp(op,"SNONEC")==0)
      r = (nz == 0) ? 1 : 0;
    /* else SNZCOUNT */
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SEQRANGES")||kw(&L->cur,"SEQRANG")||kw(&L->cur,"STACKEQRANGE")||
      kw(&L->cur,"SCELLEQRANGE")||kw(&L->cur,"SEQCRANGE")){
    /* alo blo n (stack) → 1 if equal pairwise */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long blo = vm->stack[--vm->sp];
    long alo = vm->stack[--vm->sp];
    if (n < 0) n = 0;
    if (n > CUBALC_CELL_N) n = CUBALC_CELL_N;
    int ok = 1;
    for (long i=0;i<n;i++){
      long ai = alo + i, bi = blo + i;
      long av = (ai >= 0 && ai < CUBALC_CELL_N) ? vm->cells[(int)ai] : 0;
      long bv = (bi >= 0 && bi < CUBALC_CELL_N) ? vm->cells[(int)bi] : 0;
      if (av != bv){ ok = 0; break; }
    }
    long r = ok ? 1 : 0;
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack cell search: SFINDCELL · SFINDLAST · SFIRSTNZ · SLASTNZ */
  if (kw(&L->cur,"SFINDCELL")||kw(&L->cur,"SCELLFIND")||kw(&L->cur,"STACKFINDCELL")||
      kw(&L->cur,"SFINDC")){
    /* lo hi val (stack) → first index of val or -1 */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long val = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long found = -1;
    for (long i=lo;i<=hi;i++){
      if (vm->cells[(int)i] == val){ found = i; break; }
    }
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp++] = found;
    var_set_num(vm,"LAST_N",found); vm->last_n=found;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"OK", found >= 0 ? 1 : 0);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SFINDLAST")||kw(&L->cur,"SRFINDCELL")||kw(&L->cur,"STACKFINDLAST")||
      kw(&L->cur,"SFINDL")||kw(&L->cur,"SLASTFIND")){
    /* lo hi val (stack) → last index of val or -1 */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long val = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long found = -1;
    for (long i=hi;i>=lo;i--){
      if (vm->cells[(int)i] == val){ found = i; break; }
    }
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp++] = found;
    var_set_num(vm,"LAST_N",found); vm->last_n=found;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"OK", found >= 0 ? 1 : 0);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SFIRSTNZ")||kw(&L->cur,"STACKFIRSTNZ")||kw(&L->cur,"SFNZ")||
      kw(&L->cur,"SLASTNZ")||kw(&L->cur,"STACKLASTNZ")||kw(&L->cur,"SLNZ")){
    /* lo hi (stack) → first/last nonzero index or -1 */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    int want_last = (strcmp(op,"SLASTNZ")==0 || strcmp(op,"STACKLASTNZ")==0 ||
                     strcmp(op,"SLNZ")==0);
    long found = -1;
    if (want_last){
      for (long i=hi;i>=lo;i--){
        if (vm->cells[(int)i] != 0){ found = i; break; }
      }
    } else {
      for (long i=lo;i<=hi;i++){
        if (vm->cells[(int)i] != 0){ found = i; break; }
      }
    }
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp++] = found;
    var_set_num(vm,"LAST_N",found); vm->last_n=found;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"OK", found >= 0 ? 1 : 0);
    bump(vm); return 1;
  }
  /* digit-9 cell fold arith: SUBCELL/DIVCELL/MODCELL + SCANCELL + CLAMPCELL */
  if (kw(&L->cur,"SUBCELL")||kw(&L->cur,"CELLSUB")||kw(&L->cur,"SUBFROMCELL")){
    /* SUBCELL lo hi delta — subtract delta from each cell in range */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long delta = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++) vm->cells[(int)i] -= delta;
    var_set_num(vm,"LAST_N",delta); vm->last_n=delta;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DIVCELL")||kw(&L->cur,"CELLDIV")||kw(&L->cur,"SCALEDIVCELL")){
    /* DIVCELL lo hi k — integer divide each cell by k (k==0 → leave unchanged) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    if (k != 0){
      for (long i=lo;i<=hi;i++) vm->cells[(int)i] /= k;
    }
    var_set_num(vm,"LAST_N",k); vm->last_n=k;
    var_set_num(vm,"OK", k != 0 ? 1 : 0); bump(vm); return 1;
  }
  if (kw(&L->cur,"MODCELL")||kw(&L->cur,"CELLMOD")||kw(&L->cur,"REMCELL")){
    /* MODCELL lo hi k — cell %= k (k==0 → no-op) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long k = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    if (k != 0){
      for (long i=lo;i<=hi;i++) vm->cells[(int)i] %= k;
    }
    var_set_num(vm,"LAST_N",k); vm->last_n=k;
    var_set_num(vm,"OK", k != 0 ? 1 : 0); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCANCELL")||kw(&L->cur,"PREFIXSUM")||kw(&L->cur,"CUMSUM")||
      kw(&L->cur,"CELLSCAN")||kw(&L->cur,"RUNNINGSUM")){
    /* SCANCELL lo hi — in-place inclusive prefix sum */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long run = 0;
    for (long i=lo;i<=hi;i++){
      run += vm->cells[(int)i];
      vm->cells[(int)i] = run;
    }
    var_set_num(vm,"LAST_N",run); vm->last_n=run;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CLAMPCELL")||kw(&L->cur,"CELLCLAMP")||kw(&L->cur,"BOUNDCELL")){
    /* CLAMPCELL lo hi min max — clamp each cell into [min,max] */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long mn = parse_expr(vm,L);
    long mx = parse_expr(vm,L);
    if (mx < mn){ long t=mn; mn=mx; mx=t; }
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long v = vm->cells[(int)i];
      if (v < mn) v = mn;
      if (v > mx) v = mx;
      vm->cells[(int)i] = v;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DIFFCELL")||kw(&L->cur,"CELLDIFF")||kw(&L->cur,"DELTACELL")||
      kw(&L->cur,"ADJDIFF")){
    /* DIFFCELL lo hi — in-place adjacent differences (inverse of SCANCELL);
     * cells[lo] kept; cells[i] = cells[i]-prev for i>lo */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long prev = vm->cells[(int)lo];
    for (long i = lo + 1; i <= hi; i++){
      long v = vm->cells[(int)i];
      vm->cells[(int)i] = v - prev;
      prev = v;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N", n > 0 ? vm->cells[(int)hi] : 0); vm->last_n = n > 0 ? vm->cells[(int)hi] : 0;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 cell-fold stack duals: SSCANCELL · SDIFFCELL · SSHIFTCELL · SCLAMPCELL */
  if (kw(&L->cur,"SSCANCELL")||kw(&L->cur,"SPREFIXSUM")||kw(&L->cur,"SCUMSUM")||
      kw(&L->cur,"STACKSCANCELL")||kw(&L->cur,"SSCANC")){
    /* lo hi (stack) — in-place inclusive prefix sum */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long run = 0;
    for (long i=lo;i<=hi;i++){
      run += vm->cells[(int)i];
      vm->cells[(int)i] = run;
    }
    var_set_num(vm,"LAST_N",run); vm->last_n=run;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SDIFFCELL")||kw(&L->cur,"SCELLDIFF")||kw(&L->cur,"SDELTACELL")||
      kw(&L->cur,"STACKDIFFCELL")||kw(&L->cur,"SDIFFC")){
    /* lo hi (stack) — adjacent differences (inverse of SSCANCELL) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long prev = vm->cells[(int)lo];
    for (long i = lo + 1; i <= hi; i++){
      long v = vm->cells[(int)i];
      vm->cells[(int)i] = v - prev;
      prev = v;
    }
    long last = (hi >= lo) ? vm->cells[(int)hi] : 0;
    var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSHIFTCELL")||kw(&L->cur,"SCELLSHIFT")||kw(&L->cur,"STACKSHIFTCELL")||
      kw(&L->cur,"SSHIFTC")){
    /* lo hi k (stack) — shift left by k (k>0) or right by |k|; zero-fill */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long k = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long n = hi - lo + 1;
    if (n > 0 && k != 0){
      long tmp[CUBALC_CELL_N];
      for (long i=0;i<n;i++) tmp[i] = 0;
      if (k > 0){
        if (k > n) k = n;
        for (long i=0;i<n-k;i++) tmp[i] = vm->cells[(int)(lo+i+k)];
      } else {
        long kk = -k;
        if (kk > n) kk = n;
        for (long i=kk;i<n;i++) tmp[i] = vm->cells[(int)(lo+i-kk)];
      }
      for (long i=0;i<n;i++) vm->cells[(int)(lo+i)] = tmp[i];
    }
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLAMPCELL")||kw(&L->cur,"SCELLCLAMP")||kw(&L->cur,"SBOUNDCELL")||
      kw(&L->cur,"STACKCLAMPCELL")||kw(&L->cur,"SCLAMPC")){
    /* lo hi mn mx (stack) — clamp each cell into [mn,mx] */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long mx = vm->stack[--vm->sp];
    long mn = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (mx < mn){ long t=mn; mn=mx; mx=t; }
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i=lo;i<=hi;i++){
      long v = vm->cells[(int)i];
      if (v < mn) v = mn;
      if (v > mx) v = mx;
      vm->cells[(int)i] = v;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* RAND [max] — seeded RNG (CUBALC_SEED); default range 0..9 */
  if (kw(&L->cur,"RAND")||kw(&L->cur,"RND")||kw(&L->cur,"IRAND")){
    lex_next(L);
    long m = 10;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE")))
      m = parse_expr(vm,L);
    if (m < 1) m = 1;
    uint32_t x = vm->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    if (!x) x = 1;
    vm->rng = x;
    long v = (long)(x % (uint32_t)m);
    var_set_num(vm, "LAST_N", v);
    vm->last_n = v;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-6 SEED n — set PRNG state (0 → 1); GETSEED / RNG reports state */
  if (kw(&L->cur,"SEED")||kw(&L->cur,"SETSEED")){
    lex_next(L);
    long s = parse_expr(vm,L);
    uint32_t u = (uint32_t)s;
    if (!u) u = 1;
    vm->rng = u;
    var_set_num(vm, "LAST_N", (long)u);
    vm->last_n = (long)u;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"GETSEED")||kw(&L->cur,"SHOWSEED")){
    /* statement form: report current rng without advancing */
    lex_next(L);
    long v = (long)vm->rng;
    var_set_num(vm, "LAST_N", v);
    vm->last_n = v;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-6 stack RNG: SSEED SRAND SGETSEED */
  if (kw(&L->cur,"SSEED")||kw(&L->cur,"STACKSEED")){
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long s = vm->stack[--vm->sp];
    uint32_t u = (uint32_t)s;
    if (!u) u = 1;
    vm->rng = u;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",(long)u); vm->last_n=(long)u;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGETSEED")||kw(&L->cur,"STACKGETSEED")||kw(&L->cur,"SRNG")){
    lex_next(L);
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = (long)vm->rng;
    vm->stack[vm->sp++] = v;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SRAND")||kw(&L->cur,"STACKRAND")||kw(&L->cur,"SRND")){
    /* max → rand in [0, max); max<=0 treated as 10 */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    if (m < 1) m = 10;
    uint32_t x = vm->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    if (!x) x = 1;
    vm->rng = x;
    long v = (long)(x % (uint32_t)m);
    vm->stack[vm->sp++] = v;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack/imm RNG dual forms: SRANDI/SRANDN · SSEEDI/SSEEDN (Cube aliases retained) */
  if (kw(&L->cur,"SRANDI")||kw(&L->cur,"SRANDN")||kw(&L->cur,"RANDIMM")||
      kw(&L->cur,"STACKRANDI")||kw(&L->cur,"RANDN")||kw(&L->cur,"SRANDIMM")||
      kw(&L->cur,"STACKRANDN")||kw(&L->cur,"SRNGN")){
    /* SRANDI/SRANDN n — push rand in [0,n); n<=0 → 10 */
    lex_next(L);
    long m = parse_expr(vm,L);
    if (m < 1) m = 10;
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    uint32_t x = vm->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    if (!x) x = 1;
    vm->rng = x;
    long v = (long)(x % (uint32_t)m);
    vm->stack[vm->sp++] = v;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEEDI")||kw(&L->cur,"SSEEDN")||kw(&L->cur,"SEEDIMM")||
      kw(&L->cur,"STACKSEEDI")||kw(&L->cur,"SEEDN")||kw(&L->cur,"SSEEDIMM")){
    /* SSEEDI/SSEEDN n — set PRNG seed to n (0→1); push seed used */
    lex_next(L);
    long s = parse_expr(vm,L);
    uint32_t u = (uint32_t)s;
    if (!u) u = 1;
    vm->rng = u;
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp++] = (long)u;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",(long)u); vm->last_n=(long)u;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack-imm range RNG: SRANDRANGEN lo hi — push uniform [lo,hi] (dual of SRANDRANGE) */
  if (kw(&L->cur,"SRANDRANGEN")||kw(&L->cur,"RANDRANGEN")||kw(&L->cur,"SRNGX")||
      kw(&L->cur,"STACKRANDRANGEN")||kw(&L->cur,"SRANDINN")){
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long span = hi - lo + 1;
    if (span < 1) span = 1;
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    uint32_t x = vm->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    if (!x) x = 1;
    vm->rng = x;
    long v = lo + (long)(x % (uint32_t)span);
    vm->stack[vm->sp++] = v;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 RNG range + shuffle + random matrix bits */
  if (kw(&L->cur,"RANDRANGE")||kw(&L->cur,"RANDIN")||kw(&L->cur,"RANDBETWEEN")){
    /* RANDRANGE lo hi — uniform integer in [lo,hi] inclusive */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long span = hi - lo + 1;
    if (span < 1) span = 1;
    uint32_t x = vm->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    if (!x) x = 1;
    vm->rng = x;
    long v = lo + (long)(x % (uint32_t)span);
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SRANDRANGE")||kw(&L->cur,"STACKRANDRANGE")||kw(&L->cur,"SRANDIN")){
    /* lo hi → push rand in [lo,hi] */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long span = hi - lo + 1;
    if (span < 1) span = 1;
    uint32_t x = vm->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    if (!x) x = 1;
    vm->rng = x;
    long v = lo + (long)(x % (uint32_t)span);
    vm->stack[vm->sp++] = v;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHUFFLECELL")||kw(&L->cur,"CELLSHUFFLE")||kw(&L->cur,"SHUFFLE")){
    /* SHUFFLECELL lo hi — Fisher–Yates using seeded RNG */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i = hi; i > lo; i--){
      uint32_t x = vm->rng;
      x ^= x << 13; x ^= x >> 17; x ^= x << 5;
      if (!x) x = 1;
      vm->rng = x;
      long j = lo + (long)(x % (uint32_t)(i - lo + 1));
      long tmp = vm->cells[(int)i];
      vm->cells[(int)i] = vm->cells[(int)j];
      vm->cells[(int)j] = tmp;
    }
    long n = hi - lo + 1;
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"PICKCELL")||kw(&L->cur,"CELLPICK")||kw(&L->cur,"RANDCELL")){
    /* PICKCELL lo hi — LAST_N = value at random index; IT = index */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long span = hi - lo + 1;
    if (span < 1) span = 1;
    uint32_t x = vm->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    if (!x) x = 1;
    vm->rng = x;
    long idx = lo + (long)(x % (uint32_t)span);
    long v = vm->cells[(int)idx];
    long *it = var_slot(vm,"IT",1); if (it) *it = idx;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"RANDBITS")||kw(&L->cur,"RANDOMBITS")||kw(&L->cur,"RANDMATRIX")||
      kw(&L->cur,"FILLRAND")){
    /* RANDBITS cube [pct] — randomize matrix; optional density 0..100 (default 50) */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"RANDBITS cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    long pct = 50;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE")))
      pct = parse_expr(vm,L);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"RANDBITS missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = CUBALC_ATOM_BITS;
    cubalc_matrix_clear(m);
    m->n = (uint16_t)n;
    long ones = 0;
    for (int i=0;i<n;i++){
      uint32_t x = vm->rng;
      x ^= x << 13; x ^= x >> 17; x ^= x << 5;
      if (!x) x = 1;
      vm->rng = x;
      int on = ((long)(x % 100u) < pct) ? 1 : 0;
      if (on){ cubalc_matrix_set(m, i, 1); ones++; }
    }
    vm->ch.cubes[ix].atom.digit_lock = 0;
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    vm->ch.cubes[ix].flowed = 1;
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",ones); vm->last_n=ones;
    var_set_num(vm,"DIGIT",vm->ch.cubes[ix].atom.digit);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"ENERGYGET")||kw(&L->cur,"GETENERGY")||kw(&L->cur,"READENERGY")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ENERGYGET cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1; }
    long ev = (long)lround(vm->ch.cubes[ix].atom.energy * 100.0);
    var_set_num(vm,"ENERGY",ev);
    var_set_num(vm,"LAST_N",ev); vm->last_n=ev;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* ENERGYSET cube n · ENERGYADD cube n — energy plane 0..100 (digit-6) */
  if (kw(&L->cur,"ENERGYSET")||kw(&L->cur,"SETENERGY")||
      kw(&L->cur,"ENERGYADD")||kw(&L->cur,"ADDENERGY")||kw(&L->cur,"PULSE")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ENERGY* cube n"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    long n = parse_expr(vm,L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"ENERGY missing cube"); return -1; }
    float e = vm->ch.cubes[ix].atom.energy;
    if (strcmp(op,"ENERGYSET")==0 || strcmp(op,"SETENERGY")==0){
      e = (float)n / 100.f;
    } else {
      e += (float)n / 100.f;
    }
    if (e < 0.f) e = 0.f;
    if (e > 1.f) e = 1.f;
    vm->ch.cubes[ix].atom.energy = e;
    vm->ch.cubes[ix].flowed = 1;
    long ev = (long)lround(e * 100.0);
    var_set_num(vm, "ENERGY", ev);
    var_set_num(vm, "LAST_N", ev);
    vm->last_n = ev;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-6 ENERGYSUB cube n — drain energy plane (complete set/add/sub) */
  if (kw(&L->cur,"ENERGYSUB")||kw(&L->cur,"SUBENERGY")||kw(&L->cur,"DRAIN")||
      kw(&L->cur,"SENRN")||kw(&L->cur,"ENERGYDRAIN")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ENERGYSUB cube n"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    long n = parse_expr(vm,L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"ENERGYSUB missing cube"); return -1; }
    float e = vm->ch.cubes[ix].atom.energy;
    e -= (float)n / 100.f;
    if (e < 0.f) e = 0.f;
    if (e > 1.f) e = 1.f;
    vm->ch.cubes[ix].atom.energy = e;
    vm->ch.cubes[ix].flowed = 1;
    long ev = (long)lround(e * 100.0);
    var_set_num(vm, "ENERGY", ev);
    var_set_num(vm, "LAST_N", ev);
    vm->last_n = ev;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-6 stack ENERGYFLOW: SEFLOW / SFLWN — hops from TOS (or imm form SEFLOWN) */
  if (kw(&L->cur,"SEFLOW")||kw(&L->cur,"SENERGYFLOW")||kw(&L->cur,"STACKENERGYFLOW")||
      kw(&L->cur,"SFLW")){
    /* pop hops → ENERGYFLOW n */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    if (n < 1) n = 1;
    if (n > 64) n = 64;
    for (long i = 0; i < n; i++) do_flow(vm, 1);
    long e = 0;
    for (int i = 0; i < vm->ch.n_cubes; i++)
      e += (long)lround(vm->ch.cubes[i].atom.energy * 100.0);
    long *se = var_slot(vm, "ENERGY", 1); if (se) *se = e;
    var_set_num(vm, "ENERGY", e);
    var_set_num(vm, "LAST_N", e); vm->last_n = e;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SEFLOWN")||kw(&L->cur,"SFLWN")||kw(&L->cur,"EFLOWN")||
      kw(&L->cur,"ENERGYFLOWN")){
    /* SEFLOWN n — ENERGYFLOW with immediate hops (alias plane) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (n < 1) n = 1;
    if (n > 64) n = 64;
    for (long i = 0; i < n; i++) do_flow(vm, 1);
    long e = 0;
    for (int i = 0; i < vm->ch.n_cubes; i++)
      e += (long)lround(vm->ch.cubes[i].atom.energy * 100.0);
    long *se = var_slot(vm, "ENERGY", 1); if (se) *se = e;
    var_set_num(vm, "ENERGY", e);
    var_set_num(vm, "LAST_N", e); vm->last_n = e;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-6 ENERGYXFER src dst n — move energy units (0..100 scale) src→dst, clamp both */
  if (kw(&L->cur,"ENERGYXFER")||kw(&L->cur,"XFERENERGY")||kw(&L->cur,"SENRX")||
      kw(&L->cur,"TRANSFERENERGY")||kw(&L->cur,"ENERGYMOVE")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ENERGYXFER src dst n"); return -1; }
    char src[48]; snprintf(src,sizeof src,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ENERGYXFER dst"); return -1; }
    char dst[48]; snprintf(dst,sizeof dst,"%s",L->cur.text); lex_next(L);
    long n = parse_expr(vm,L);
    if (n < 0) n = 0;
    ensure_world(vm);
    int is = find_cube(vm,src);
    int id = find_cube(vm,dst);
    if (is<0){ place_cube(vm,src,src,1); is=find_cube(vm,src); }
    if (id<0){ place_cube(vm,dst,dst,1); id=find_cube(vm,dst); }
    if (is<0 || id<0){ fail(vm,"ENERGYXFER missing"); return -1; }
    float es = vm->ch.cubes[is].atom.energy;
    float ed = vm->ch.cubes[id].atom.energy;
    float amt = (float)n / 100.f;
    if (amt > es) amt = es;
    es -= amt;
    ed += amt;
    if (es < 0.f) es = 0.f;
    if (es > 1.f) es = 1.f;
    if (ed < 0.f) ed = 0.f;
    if (ed > 1.f) ed = 1.f;
    vm->ch.cubes[is].atom.energy = es;
    vm->ch.cubes[id].atom.energy = ed;
    vm->ch.cubes[is].flowed = 1;
    vm->ch.cubes[id].flowed = 1;
    long moved = (long)lround(amt * 100.0);
    long *se = var_slot(vm, "ENERGY", 1);
    if (se) *se = (long)lround(ed * 100.0);
    var_set_num(vm, "ENERGY", (long)lround(ed * 100.0));
    var_set_num(vm, "LAST_N", moved); vm->last_n = moved;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-6 ENERGYCLAMP id lo hi — clamp energy plane to [lo,hi] (0..100 scale) */
  if (kw(&L->cur,"ENERGYCLAMP")||kw(&L->cur,"CLAMPENERGY")||kw(&L->cur,"SFLWX")||
      kw(&L->cur,"ENERGYBOUND")||kw(&L->cur,"BOUNDENERGY")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ENERGYCLAMP id lo hi"); return -1; }
    char idn[48]; snprintf(idn,sizeof idn,"%s",L->cur.text); lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (lo > hi){ long t=lo; lo=hi; hi=t; }
    if (lo < 0) lo = 0;
    if (hi > 100) hi = 100;
    ensure_world(vm);
    int ix = find_cube(vm,idn);
    if (ix<0){ place_cube(vm,idn,idn,1); ix=find_cube(vm,idn); }
    if (ix<0){ fail(vm,"ENERGYCLAMP missing"); return -1; }
    long ev = (long)lround(vm->ch.cubes[ix].atom.energy * 100.0);
    if (ev < lo) ev = lo;
    if (ev > hi) ev = hi;
    vm->ch.cubes[ix].atom.energy = (float)ev / 100.f;
    vm->ch.cubes[ix].flowed = 1;
    var_set_num(vm, "ENERGY", ev);
    var_set_num(vm, "LAST_N", ev); vm->last_n = ev;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-6 ENERGYSWAP a b — exchange energy planes */
  if (kw(&L->cur,"ENERGYSWAP")||kw(&L->cur,"SWAPENERGY")||kw(&L->cur,"ESWAP")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ENERGYSWAP a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ENERGYSWAP a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    ensure_world(vm);
    int ia = find_cube(vm,a); if (ia<0){ place_cube(vm,a,a,1); ia=find_cube(vm,a); }
    int ib = find_cube(vm,b); if (ib<0){ place_cube(vm,b,b,1); ib=find_cube(vm,b); }
    if (ia<0 || ib<0){ fail(vm,"ENERGYSWAP missing"); return -1; }
    float ea = vm->ch.cubes[ia].atom.energy;
    float eb = vm->ch.cubes[ib].atom.energy;
    vm->ch.cubes[ia].atom.energy = eb;
    vm->ch.cubes[ib].atom.energy = ea;
    vm->ch.cubes[ia].flowed = 1;
    vm->ch.cubes[ib].flowed = 1;
    long ev = (long)lround(eb * 100.0); /* energy now on a */
    var_set_num(vm,"ENERGY",ev);
    var_set_num(vm,"LAST_N",ev); vm->last_n=ev;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 ENERGYSHARE a b — equalize energy (mean of both, clamp 0..1) */
  if (kw(&L->cur,"ENERGYSHARE")||kw(&L->cur,"SHAREENERGY")||kw(&L->cur,"EQUALIZEENERGY")||
      kw(&L->cur,"ENERGYEQZ")||kw(&L->cur,"ESHAR")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ENERGYSHARE a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ENERGYSHARE a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    ensure_world(vm);
    int ia = find_cube(vm,a); if (ia<0){ place_cube(vm,a,a,1); ia=find_cube(vm,a); }
    int ib = find_cube(vm,b); if (ib<0){ place_cube(vm,b,b,1); ib=find_cube(vm,b); }
    if (ia<0 || ib<0){ fail(vm,"ENERGYSHARE missing"); return -1; }
    float mean = 0.5f * (vm->ch.cubes[ia].atom.energy + vm->ch.cubes[ib].atom.energy);
    if (mean < 0.f) mean = 0.f;
    if (mean > 1.f) mean = 1.f;
    vm->ch.cubes[ia].atom.energy = mean;
    vm->ch.cubes[ib].atom.energy = mean;
    vm->ch.cubes[ia].flowed = 1;
    vm->ch.cubes[ib].flowed = 1;
    long ev = (long)lround(mean * 100.0);
    var_set_num(vm,"ENERGY",ev);
    var_set_num(vm,"LAST_N",ev); vm->last_n=ev;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 fleet energy metrics: ENERGYTOTAL ENERGYAVG ENERGYMIN ENERGYMAX */
  if (kw(&L->cur,"ENERGYTOTAL")||kw(&L->cur,"ENERGYSUM")||kw(&L->cur,"SUMENERGY")||
      kw(&L->cur,"TOTALENERGY")||
      kw(&L->cur,"ENERGYAVG")||kw(&L->cur,"AVGENERGY")||kw(&L->cur,"MEANENERGY")||
      kw(&L->cur,"ENERGYMEAN")||
      kw(&L->cur,"ENERGYMIN")||kw(&L->cur,"MINENERGY")||
      kw(&L->cur,"ENERGYMAX")||kw(&L->cur,"MAXENERGY")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    ensure_world(vm);
    int nc = vm->ch.n_cubes;
    if (nc < 1){
      var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1;
    }
    long sum = 0, mn = 101, mx = -1;
    for (int i=0;i<nc;i++){
      long ev = (long)lround(vm->ch.cubes[i].atom.energy * 100.0);
      sum += ev;
      if (ev < mn) mn = ev;
      if (ev > mx) mx = ev;
    }
    long r = sum;
    if (strcmp(op,"ENERGYAVG")==0 || strcmp(op,"AVGENERGY")==0 ||
        strcmp(op,"MEANENERGY")==0 || strcmp(op,"ENERGYMEAN")==0)
      r = sum / nc;
    else if (strcmp(op,"ENERGYMIN")==0 || strcmp(op,"MINENERGY")==0)
      r = mn;
    else if (strcmp(op,"ENERGYMAX")==0 || strcmp(op,"MAXENERGY")==0)
      r = mx;
    /* else TOTAL/SUM */
    var_set_num(vm,"ENERGY",r);
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SET",(long)nc);
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack fleet energy metrics: SENERGYTOTAL SENERGYAVG SENERGYMIN SENERGYMAX */
  if (kw(&L->cur,"SENERGYTOTAL")||kw(&L->cur,"SENERGYSUM")||kw(&L->cur,"STACKENERGYTOTAL")||
      kw(&L->cur,"SETOTAL")||
      kw(&L->cur,"SENERGYAVG")||kw(&L->cur,"SENERGYMEAN")||kw(&L->cur,"STACKENERGYAVG")||
      kw(&L->cur,"SEAVG")||
      kw(&L->cur,"SENERGYMIN")||kw(&L->cur,"STACKENERGYMIN")||kw(&L->cur,"SEMIN")||
      kw(&L->cur,"SENERGYMAX")||kw(&L->cur,"STACKENERGYMAX")||kw(&L->cur,"SEMAX")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    ensure_world(vm);
    int nc = vm->ch.n_cubes;
    if (nc < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long sum = 0, mn = 101, mx = -1;
    for (int i=0;i<nc;i++){
      long ev = (long)lround(vm->ch.cubes[i].atom.energy * 100.0);
      sum += ev;
      if (ev < mn) mn = ev;
      if (ev > mx) mx = ev;
    }
    long r = sum;
    if (strcmp(op,"SENERGYAVG")==0 || strcmp(op,"SENERGYMEAN")==0 ||
        strcmp(op,"STACKENERGYAVG")==0 || strcmp(op,"SEAVG")==0)
      r = sum / nc;
    else if (strcmp(op,"SENERGYMIN")==0 || strcmp(op,"STACKENERGYMIN")==0 ||
             strcmp(op,"SEMIN")==0)
      r = mn;
    else if (strcmp(op,"SENERGYMAX")==0 || strcmp(op,"STACKENERGYMAX")==0 ||
             strcmp(op,"SEMAX")==0)
      r = mx;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"ENERGY",r);
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* ROTBITS cube k — rotate State Matrix bits left by k (digit-6 matrix flow) */
  if (kw(&L->cur,"ROTBITS")||kw(&L->cur,"ROLBITS")||kw(&L->cur,"SHIFTBITS")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ROTBITS cube k"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    long k = parse_expr(vm,L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"ROTBITS missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    if (n < 1) n = CUBALC_ATOM_BITS;
    if (k < 0){
      /* right rotate = left by n - (|k|%n) */
      long kk = (-k) % n;
      k = kk ? (n - kk) : 0;
    } else {
      k = k % n;
    }
    if (k){
      uint8_t tmp[(CUBALC_ATOM_BITS + 7) / 8];
      memset(tmp, 0, sizeof tmp);
      for (int i=0;i<n;i++){
        int src = cubalc_matrix_get(m, i);
        int dst = (int)((i + k) % n);
        if (src) tmp[dst >> 3] |= (uint8_t)(1u << (dst & 7));
      }
      cubalc_matrix_clear(m);
      m->n = (uint16_t)n;
      for (int i=0;i<n;i++){
        int on = (tmp[i >> 3] >> (i & 7)) & 1;
        if (on) cubalc_matrix_set(m, i, 1);
      }
    }
    vm->ch.cubes[ix].atom.digit_lock = 0;
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    vm->ch.cubes[ix].flowed = 1;
    var_set_num(vm, "SET", cubalc_matrix_popcount(m));
    var_set_num(vm, "DIGIT", vm->ch.cubes[ix].atom.digit);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-3 COP matrix logical shift: SHLBITS/SHRBITS cube k (zero-fill, not rotate) */
  if (kw(&L->cur,"SHLBITS")||kw(&L->cur,"LSHBITS")||kw(&L->cur,"LSHIFTBITS")||
      kw(&L->cur,"SHRBITS")||kw(&L->cur,"RSHBITS")||kw(&L->cur,"RSHIFTBITS")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"SHLBITS cube k"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    long k = parse_expr(vm,L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"SHLBITS missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    if (n < 1) n = CUBALC_ATOM_BITS;
    int left = (strcmp(op,"SHLBITS")==0 || strcmp(op,"LSHBITS")==0 ||
                strcmp(op,"LSHIFTBITS")==0);
    if (k < 0){
      /* negative shift flips direction */
      left = !left;
      k = -k;
    }
    if (k >= n){
      cubalc_matrix_clear(m);
      m->n = (uint16_t)n;
    } else if (k > 0){
      uint8_t tmp[(CUBALC_ATOM_BITS + 7) / 8];
      memset(tmp, 0, sizeof tmp);
      for (int i=0;i<n;i++){
        if (!cubalc_matrix_get(m, i)) continue;
        int dst = left ? i + (int)k : i - (int)k;
        if (dst < 0 || dst >= n) continue; /* drop overflow */
        tmp[dst >> 3] |= (uint8_t)(1u << (dst & 7));
      }
      cubalc_matrix_clear(m);
      m->n = (uint16_t)n;
      for (int i=0;i<n;i++){
        int on = (tmp[i >> 3] >> (i & 7)) & 1;
        if (on) cubalc_matrix_set(m, i, 1);
      }
    }
    vm->ch.cubes[ix].atom.digit_lock = 0;
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    vm->ch.cubes[ix].flowed = 1;
    long ones = cubalc_matrix_popcount(m);
    var_set_num(vm, "SET", ones);
    var_set_num(vm, "LAST_N", ones); vm->last_n = ones;
    var_set_num(vm, "DIGIT", vm->ch.cubes[ix].atom.digit);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SETBIT")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"SETBIT cube i on"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int bit=(int)parse_expr(vm,L);
    int on=1;
    /* optional on/off expr (default 1) */
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
        L->cur.kind==TK_MINUS){
      on = parse_expr(vm,L) ? 1 : 0;
    }
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"SETBIT unknown cube"); return -1; }
    cubalc_matrix_set(&vm->ch.cubes[ix].atom.matrix, bit, on);
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    var_set_num(vm, "LAST_N", on ? 1 : 0); vm->last_n = on ? 1 : 0;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-0/5 COP matrix query: COUNTBITS/ONES/ZEROS cube · GETBIT cube i */
  if (kw(&L->cur,"COUNTBITS")||kw(&L->cur,"ONES")||kw(&L->cur,"POPBITS")||
      kw(&L->cur,"BITCOUNT")||kw(&L->cur,"ZEROS")||kw(&L->cur,"ZEROCOUNT")||
      kw(&L->cur,"NZERO")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"COUNTBITS cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    long ones = cubalc_matrix_popcount(m);
    long r = ones;
    if (strcmp(op,"ZEROS")==0 || strcmp(op,"ZEROCOUNT")==0 || strcmp(op,"NZERO")==0)
      r = (long)n - ones;
    var_set_num(vm, "SET", ones);
    var_set_num(vm, "LAST_N", r); vm->last_n = r;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"GETBIT")||kw(&L->cur,"BITAT")||kw(&L->cur,"MATBIT")||
      kw(&L->cur,"CUBEBIT")||kw(&L->cur,"READBIT")){
    /* note: TESTBIT is stack bitfield — use GETBIT for matrix */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"GETBIT cube i"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int bit = (int)parse_expr(vm,L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1; }
    long v = cubalc_matrix_get(&vm->ch.cubes[ix].atom.matrix, bit) ? 1 : 0;
    var_set_num(vm, "LAST_N", v); vm->last_n = v;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-3 COP matrix scan: FINDONE/LASTONE cube · ANYBITS/ALLBITS cube */
  if (kw(&L->cur,"FINDONE")||kw(&L->cur,"FIRSTBIT")||kw(&L->cur,"FIRSTONE")||
      kw(&L->cur,"FFSBIT")||kw(&L->cur,"LASTONE")||kw(&L->cur,"LASTBIT")||
      kw(&L->cur,"FLSBIT")||kw(&L->cur,"ANYBITS")||kw(&L->cur,"ALLBITS")||
      kw(&L->cur,"NONEBITS")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"FINDONE cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    long ones = cubalc_matrix_popcount(m);
    long r = -1;
    if (strcmp(op,"ANYBITS")==0){
      r = ones > 0 ? 1 : 0;
    } else if (strcmp(op,"ALLBITS")==0){
      r = (ones >= n && n > 0) ? 1 : 0;
    } else if (strcmp(op,"NONEBITS")==0){
      r = ones == 0 ? 1 : 0;
    } else if (strcmp(op,"LASTONE")==0 || strcmp(op,"LASTBIT")==0 ||
               strcmp(op,"FLSBIT")==0){
      r = -1;
      for (int i=n-1;i>=0;i--){
        if (cubalc_matrix_get(m, i)){ r = i; break; }
      }
    } else {
      /* FINDONE / FIRSTBIT / FIRSTONE / FFSBIT */
      r = -1;
      for (int i=0;i<n;i++){
        if (cubalc_matrix_get(m, i)){ r = i; break; }
      }
    }
    var_set_num(vm, "SET", ones);
    var_set_num(vm, "LAST_N", r); vm->last_n = r;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-7 COP matrix reorder: REVBITS cube · SWAPBIT cube i j */
  if (kw(&L->cur,"REVBITS")||kw(&L->cur,"REVERSEBITS")||kw(&L->cur,"BITREVM")||
      kw(&L->cur,"MIRRORBITS")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"REVBITS cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"REVBITS missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    if (n < 1) n = CUBALC_ATOM_BITS;
    if (n > 1){
      uint8_t tmp[(CUBALC_ATOM_BITS + 7) / 8];
      memset(tmp, 0, sizeof tmp);
      for (int i=0;i<n;i++){
        if (!cubalc_matrix_get(m, i)) continue;
        int dst = n - 1 - i;
        tmp[dst >> 3] |= (uint8_t)(1u << (dst & 7));
      }
      cubalc_matrix_clear(m);
      m->n = (uint16_t)n;
      for (int i=0;i<n;i++){
        int on = (tmp[i >> 3] >> (i & 7)) & 1;
        if (on) cubalc_matrix_set(m, i, 1);
      }
    } else {
      m->n = (uint16_t)n;
    }
    vm->ch.cubes[ix].atom.digit_lock = 0;
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    vm->ch.cubes[ix].flowed = 1;
    long ones = cubalc_matrix_popcount(m);
    var_set_num(vm, "SET", ones);
    var_set_num(vm, "LAST_N", ones); vm->last_n = ones;
    var_set_num(vm, "DIGIT", vm->ch.cubes[ix].atom.digit);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SWAPBIT")||kw(&L->cur,"XCHGBIT")||kw(&L->cur,"EXCHBIT")||
      kw(&L->cur,"SWBIT")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"SWAPBIT cube i j"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int i = (int)parse_expr(vm,L);
    int j = (int)parse_expr(vm,L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"SWAPBIT missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    if (n < 1) n = CUBALC_ATOM_BITS;
    if (m->n < (uint16_t)n) m->n = (uint16_t)n;
    if (i >= 0 && j >= 0 && i < n && j < n && i != j){
      int bi = cubalc_matrix_get(m, i) ? 1 : 0;
      int bj = cubalc_matrix_get(m, j) ? 1 : 0;
      cubalc_matrix_set(m, i, bj);
      cubalc_matrix_set(m, j, bi);
    }
    vm->ch.cubes[ix].atom.digit_lock = 0;
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    vm->ch.cubes[ix].flowed = 1;
    long ones = cubalc_matrix_popcount(m);
    var_set_num(vm, "SET", ones);
    var_set_num(vm, "LAST_N", ones); vm->last_n = ones;
    var_set_num(vm, "DIGIT", vm->ch.cubes[ix].atom.digit);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-7 COP matrix distance: DIFFBITS/HAMBITS a b → LAST_N = popcount(a XOR b) */
  if (kw(&L->cur,"DIFFBITS")||kw(&L->cur,"HAMBITS")||kw(&L->cur,"HAMMINGBITS")||
      kw(&L->cur,"BITDIFF")||kw(&L->cur,"XORDIST")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"DIFFBITS a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"DIFFBITS a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    int ia=find_cube(vm,a), ib=find_cube(vm,b);
    if (ia<0 || ib<0){
      var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1;
    }
    cubalc_matrix *ma = &vm->ch.cubes[ia].atom.matrix;
    cubalc_matrix *mb = &vm->ch.cubes[ib].atom.matrix;
    int na = ma->n > 0 ? ma->n : CUBALC_ATOM_BITS;
    int nb = mb->n > 0 ? mb->n : CUBALC_ATOM_BITS;
    if (na > CUBALC_ATOM_BITS) na = CUBALC_ATOM_BITS;
    if (nb > CUBALC_ATOM_BITS) nb = CUBALC_ATOM_BITS;
    int nn = na > nb ? na : nb;
    if (nn < 1) nn = CUBALC_ATOM_BITS;
    long d = 0;
    for (int k=0;k<nn;k++){
      int ba = cubalc_matrix_get(ma, k) ? 1 : 0;
      int bb = cubalc_matrix_get(mb, k) ? 1 : 0;
      if (ba != bb) d++;
    }
    var_set_num(vm, "LAST_N", d); vm->last_n = d;
    var_set_num(vm, "SET", d);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-9 COP metrics: DOTBITS · MAJBITS/THRESHBITS · GRAYBITS/UNGRAYBITS */
  if (kw(&L->cur,"DOTBITS")||kw(&L->cur,"ANDPOP")||kw(&L->cur,"ANDCOUNT")||
      kw(&L->cur,"BITDOT")||kw(&L->cur,"OVERLAPCOUNT")){
    /* DOTBITS a b → LAST_N = popcount(a AND b) */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"DOTBITS a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"DOTBITS a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    int ia=find_cube(vm,a), ib=find_cube(vm,b);
    if (ia<0 || ib<0){
      var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1;
    }
    cubalc_matrix *ma = &vm->ch.cubes[ia].atom.matrix;
    cubalc_matrix *mb = &vm->ch.cubes[ib].atom.matrix;
    int na = ma->n > 0 ? ma->n : CUBALC_ATOM_BITS;
    int nb = mb->n > 0 ? mb->n : CUBALC_ATOM_BITS;
    if (na > CUBALC_ATOM_BITS) na = CUBALC_ATOM_BITS;
    if (nb > CUBALC_ATOM_BITS) nb = CUBALC_ATOM_BITS;
    int nn = na > nb ? na : nb;
    if (nn < 1) nn = CUBALC_ATOM_BITS;
    long d = 0;
    for (int k=0;k<nn;k++){
      int ba = cubalc_matrix_get(ma, k) ? 1 : 0;
      int bb = cubalc_matrix_get(mb, k) ? 1 : 0;
      if (ba && bb) d++;
    }
    var_set_num(vm,"LAST_N",d); vm->last_n=d;
    var_set_num(vm,"SET",d);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* digit-9 COP metrics: CLZBITS/CTZBITS · ORPOP/UNIONPOP · JACCARD */
  if (kw(&L->cur,"CLZBITS")||kw(&L->cur,"NLZBITS")||kw(&L->cur,"LEADINGZEROS")||
      kw(&L->cur,"CTZBITS")||kw(&L->cur,"NTZBITS")||kw(&L->cur,"TRAILINGZEROS")){
    /* CLZBITS cube → consecutive zeros from high end; CTZBITS from low end.
     * All-zero → n. Empty/missing → OK=0 LAST_N=-1. */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_clz = (strcmp(op,"CLZBITS")==0 || strcmp(op,"NLZBITS")==0 ||
                  strcmp(op,"LEADINGZEROS")==0);
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"CLZBITS cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    if (n < 1) n = CUBALC_ATOM_BITS;
    long r = 0;
    if (is_clz){
      for (int i=n-1;i>=0;i--){
        if (cubalc_matrix_get(m, i)) break;
        r++;
      }
    } else {
      for (int i=0;i<n;i++){
        if (cubalc_matrix_get(m, i)) break;
        r++;
      }
    }
    var_set_num(vm,"SET",cubalc_matrix_popcount(m));
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"ORPOP")||kw(&L->cur,"UNIONPOP")||kw(&L->cur,"ORCOUNT")||
      kw(&L->cur,"BITUNION")||kw(&L->cur,"UNIONBITS")){
    /* ORPOP a b → popcount(a OR b) */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ORPOP a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ORPOP a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    int ia=find_cube(vm,a), ib=find_cube(vm,b);
    if (ia<0 || ib<0){
      var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1;
    }
    cubalc_matrix *ma = &vm->ch.cubes[ia].atom.matrix;
    cubalc_matrix *mb = &vm->ch.cubes[ib].atom.matrix;
    int na = ma->n > 0 ? ma->n : CUBALC_ATOM_BITS;
    int nb = mb->n > 0 ? mb->n : CUBALC_ATOM_BITS;
    if (na > CUBALC_ATOM_BITS) na = CUBALC_ATOM_BITS;
    if (nb > CUBALC_ATOM_BITS) nb = CUBALC_ATOM_BITS;
    int nn = na > nb ? na : nb;
    if (nn < 1) nn = CUBALC_ATOM_BITS;
    long d = 0;
    for (int k=0;k<nn;k++){
      int ba = cubalc_matrix_get(ma, k) ? 1 : 0;
      int bb = cubalc_matrix_get(mb, k) ? 1 : 0;
      if (ba || bb) d++;
    }
    var_set_num(vm,"LAST_N",d); vm->last_n=d;
    var_set_num(vm,"SET",d);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"JACCARD")||kw(&L->cur,"JACCARD100")||kw(&L->cur,"SIMBITS")||
      kw(&L->cur,"BITJACCARD")||kw(&L->cur,"OVERLAPRATIO")){
    /* JACCARD a b → 100 * |A∩B| / |A∪B| (integer percent); empty union → 0 */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"JACCARD a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"JACCARD a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    int ia=find_cube(vm,a), ib=find_cube(vm,b);
    if (ia<0 || ib<0){
      var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1;
    }
    cubalc_matrix *ma = &vm->ch.cubes[ia].atom.matrix;
    cubalc_matrix *mb = &vm->ch.cubes[ib].atom.matrix;
    int na = ma->n > 0 ? ma->n : CUBALC_ATOM_BITS;
    int nb = mb->n > 0 ? mb->n : CUBALC_ATOM_BITS;
    if (na > CUBALC_ATOM_BITS) na = CUBALC_ATOM_BITS;
    if (nb > CUBALC_ATOM_BITS) nb = CUBALC_ATOM_BITS;
    int nn = na > nb ? na : nb;
    if (nn < 1) nn = CUBALC_ATOM_BITS;
    long inter = 0, uni = 0;
    for (int k=0;k<nn;k++){
      int ba = cubalc_matrix_get(ma, k) ? 1 : 0;
      int bb = cubalc_matrix_get(mb, k) ? 1 : 0;
      if (ba && bb) inter++;
      if (ba || bb) uni++;
    }
    long r = 0;
    if (uni > 0) r = (inter * 100) / uni;
    var_set_num(vm,"SET",inter);
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* digit-9 COP metrics: TRANSBITS · RUNSBITS · MASKPOP */
  if (kw(&L->cur,"TRANSBITS")||kw(&L->cur,"BITTRANS")||kw(&L->cur,"EDGESBITS")||
      kw(&L->cur,"TRANSITIONS")||kw(&L->cur,"BITEDGES")){
    /* TRANSBITS cube → count of adjacent bit flips (bit[i] XOR bit[i+1]) */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"TRANSBITS cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    long t = 0;
    for (int i=0;i+1<n;i++){
      int a = cubalc_matrix_get(m, i) ? 1 : 0;
      int b = cubalc_matrix_get(m, i+1) ? 1 : 0;
      if (a != b) t++;
    }
    var_set_num(vm,"SET",cubalc_matrix_popcount(m));
    var_set_num(vm,"LAST_N",t); vm->last_n=t;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"RUNSBITS")||kw(&L->cur,"ONERUNS")||kw(&L->cur,"BITRUNS")||
      kw(&L->cur,"RUNCOUNT")||kw(&L->cur,"GROUPONES")){
    /* RUNSBITS cube → number of maximal contiguous 1-runs */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"RUNSBITS cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    long runs = 0;
    int prev = 0;
    for (int i=0;i<n;i++){
      int on = cubalc_matrix_get(m, i) ? 1 : 0;
      if (on && !prev) runs++;
      prev = on;
    }
    var_set_num(vm,"SET",cubalc_matrix_popcount(m));
    var_set_num(vm,"LAST_N",runs); vm->last_n=runs;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"MASKPOP")||kw(&L->cur,"POPMASK")||kw(&L->cur,"ANDMASKPOP")||
      kw(&L->cur,"WEIGHTMASK")||kw(&L->cur,"MASKONES")){
    /* MASKPOP cube mask → popcount of cube bits under mask ones (mask is integer word) */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"MASKPOP cube mask"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    long maskv = parse_expr(vm,L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    if (n > 64) n = 64;
    unsigned long long mask = (unsigned long long)maskv;
    long c = 0;
    for (int i=0;i<n;i++){
      if (((mask >> i) & 1ull) && cubalc_matrix_get(m, i)) c++;
    }
    var_set_num(vm,"SET",cubalc_matrix_popcount(m));
    var_set_num(vm,"LAST_N",c); vm->last_n=c;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* digit-9 COP metrics ext: MAXRUN · ZRUNS · FINDZERO/LASTZERO */
  if (kw(&L->cur,"MAXRUN")||kw(&L->cur,"LONGRUN")||kw(&L->cur,"LONGESTONES")||
      kw(&L->cur,"MAXONESRUN")||kw(&L->cur,"LONGEST1")){
    /* MAXRUN cube → length of longest contiguous 1-run (0 if no ones) */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"MAXRUN cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    long best = 0, cur = 0;
    for (int i=0;i<n;i++){
      if (cubalc_matrix_get(m, i)){ cur++; if (cur > best) best = cur; }
      else cur = 0;
    }
    var_set_num(vm,"SET",cubalc_matrix_popcount(m));
    var_set_num(vm,"LAST_N",best); vm->last_n=best;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"ZRUNS")||kw(&L->cur,"ZERORUNS")||kw(&L->cur,"OFFRUNS")||
      kw(&L->cur,"GROUPZEROS")||kw(&L->cur,"ZEROGROUPS")){
    /* ZRUNS cube → number of maximal contiguous 0-runs */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ZRUNS cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    long runs = 0;
    int prev = 1; /* treat edge as "ones" so leading zeros start a run */
    for (int i=0;i<n;i++){
      int on = cubalc_matrix_get(m, i) ? 1 : 0;
      if (!on && prev) runs++;
      prev = on;
    }
    var_set_num(vm,"SET",cubalc_matrix_popcount(m));
    var_set_num(vm,"LAST_N",runs); vm->last_n=runs;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"FINDZERO")||kw(&L->cur,"FIRSTZERO")||kw(&L->cur,"FIRST0")||
      kw(&L->cur,"FFZBIT")||kw(&L->cur,"LASTZERO")||kw(&L->cur,"LAST0")||
      kw(&L->cur,"FLZBIT")){
    /* FINDZERO cube → index of first 0, or -1 if all ones
     * LASTZERO cube → index of last 0, or -1 if all ones */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"FINDZERO cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",-1); vm->last_n=-1; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    long ones = cubalc_matrix_popcount(m);
    long r = -1;
    int is_last = (strcmp(op,"LASTZERO")==0 || strcmp(op,"LAST0")==0 ||
                   strcmp(op,"FLZBIT")==0);
    if (is_last){
      for (int i=n-1;i>=0;i--){
        if (!cubalc_matrix_get(m, i)){ r = i; break; }
      }
    } else {
      for (int i=0;i<n;i++){
        if (!cubalc_matrix_get(m, i)){ r = i; break; }
      }
    }
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"MAJBITS")||kw(&L->cur,"MAJORITYBITS")||kw(&L->cur,"THRESHBITS")||
      kw(&L->cur,"VOTEBITS")||kw(&L->cur,"ONESGE")){
    /* MAJBITS cube [k] → LAST_N=1 if popcount >= k; default k = n/2+1 (strict majority)
     * THRESHBITS same with required k (if omitted, same majority default). */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"MAJBITS cube [k]"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int have_k = 0;
    long kthr = 0;
    /* optional k: number / (expr) / unary minus only — bare IDENT is next statement */
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS){
      kthr = parse_expr(vm,L);
      have_k = 1;
    }
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    if (n < 1) n = CUBALC_ATOM_BITS;
    long ones = cubalc_matrix_popcount(m);
    if (!have_k){
      kthr = (long)n / 2 + 1; /* strict majority */
    }
    if (kthr < 0) kthr = 0;
    long hit = (ones >= kthr) ? 1 : 0;
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",hit); vm->last_n=hit;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"GRAYBITS")||kw(&L->cur,"TOGRAY")||kw(&L->cur,"BIN2GRAY")||
      kw(&L->cur,"UNGRAYBITS")||kw(&L->cur,"FROMGRAY")||kw(&L->cur,"GRAY2BIN")||
      kw(&L->cur,"DEGRAYBITS")){
    /* GRAYBITS cube — binary→Gray: g = n^(n>>1); UNGRAYBITS Gray→binary */
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int to_gray = (strcmp(op,"GRAYBITS")==0 || strcmp(op,"TOGRAY")==0 ||
                   strcmp(op,"BIN2GRAY")==0);
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"GRAYBITS cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"GRAYBITS missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = CUBALC_ATOM_BITS;
    if (m->n < (uint16_t)n) m->n = (uint16_t)n;
    if (n > 64) n = 64;
    unsigned long long u = 0;
    for (int i=0;i<n && i<64;i++){
      if (cubalc_matrix_get(m, i)) u |= (1ULL << i);
    }
    unsigned long long r;
    if (to_gray){
      r = u ^ (u >> 1);
    } else {
      /* Gray → binary: successive XOR of higher bits */
      r = u;
      for (int s=1;s<64;s<<=1) r ^= (r >> s);
    }
    for (int i=0;i<n && i<64;i++){
      cubalc_matrix_set(m, i, (r >> i) & 1ULL ? 1 : 0);
    }
    vm->ch.cubes[ix].atom.digit_lock = 0;
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    vm->ch.cubes[ix].flowed = 1;
    long ones = cubalc_matrix_popcount(m);
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",(long)r); vm->last_n=(long)r;
    var_set_num(vm,"DIGIT",vm->ch.cubes[ix].atom.digit);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* digit-7 COP matrix: PARITYBITS · COPYRANGE · SWAPRANGE (local reverse) */
  if (kw(&L->cur,"PARITYBITS")||kw(&L->cur,"XORREDUCE")||kw(&L->cur,"BITPARITY")||
      kw(&L->cur,"PARBIT")){
    /* PARITYBITS cube → LAST_N = XOR of all bits (0/1) */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"PARITYBITS cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    long p = 0;
    long ones = 0;
    for (int i=0;i<n;i++){
      if (cubalc_matrix_get(m, i)){ p ^= 1; ones++; }
    }
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",p); vm->last_n=p;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"COPYRANGE")||kw(&L->cur,"BITCOPY")||kw(&L->cur,"MOVEBITS")||
      kw(&L->cur,"XFERBITS")){
    /* COPYRANGE dst doff src soff n — copy n bits src[soff..] → dst[doff..] */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"COPYRANGE dst doff src soff n"); return -1; }
    char dst[48]; snprintf(dst,sizeof dst,"%s",L->cur.text); lex_next(L);
    long doff = parse_expr(vm,L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"COPYRANGE dst doff src soff n"); return -1; }
    char srcid[48]; snprintf(srcid,sizeof srcid,"%s",L->cur.text); lex_next(L);
    long soff = parse_expr(vm,L);
    long cnt = parse_expr(vm,L);
    ensure_world(vm);
    int id=find_cube(vm,dst); if (id<0){ place_cube(vm,dst,dst,1); id=find_cube(vm,dst); }
    int is=find_cube(vm,srcid); if (is<0){ place_cube(vm,srcid,srcid,1); is=find_cube(vm,srcid); }
    if (id<0||is<0){ fail(vm,"COPYRANGE cube missing"); return -1; }
    cubalc_matrix *md = &vm->ch.cubes[id].atom.matrix;
    cubalc_matrix *ms = &vm->ch.cubes[is].atom.matrix;
    if (md->n < CUBALC_ATOM_BITS) md->n = (uint16_t)CUBALC_ATOM_BITS;
    if (doff < 0) doff = 0;
    if (soff < 0) soff = 0;
    if (cnt < 0) cnt = 0;
    long copied = 0;
    for (long k=0;k<cnt;k++){
      long di = doff + k;
      long si = soff + k;
      if (di < 0 || di >= CUBALC_ATOM_BITS) break;
      if (si < 0 || si >= CUBALC_ATOM_BITS){
        cubalc_matrix_set(md, (int)di, 0);
      } else {
        int on = cubalc_matrix_get(ms, (int)si) ? 1 : 0;
        cubalc_matrix_set(md, (int)di, on);
      }
      copied++;
    }
    vm->ch.cubes[id].atom.digit_lock = 0;
    vm->ch.cubes[id].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[id].atom.matrix);
    vm->ch.cubes[id].flowed = 1;
    var_set_num(vm,"SET",cubalc_matrix_popcount(md));
    var_set_num(vm,"LAST_N",copied); vm->last_n=copied;
    var_set_num(vm,"DIGIT",vm->ch.cubes[id].atom.digit);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SWAPRANGE")||kw(&L->cur,"REVERSERANGE")||kw(&L->cur,"REVRANGE")||
      kw(&L->cur,"MIRRORRANGE")){
    /* SWAPRANGE cube lo hi — reverse bits in [lo..hi] inclusive */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"SWAPRANGE cube lo hi"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int lo = (int)parse_expr(vm,L);
    int hi = (int)parse_expr(vm,L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"SWAPRANGE missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = CUBALC_ATOM_BITS;
    if (m->n < (uint16_t)n) m->n = (uint16_t)n;
    if (lo < 0) lo = 0;
    if (hi < lo){ int t=lo; lo=hi; hi=t; }
    if (lo >= n){ var_set_num(vm,"OK",1); bump(vm); return 1; }
    if (hi >= n) hi = n - 1;
    for (int i=lo, j=hi; i<j; i++, j--){
      int bi = cubalc_matrix_get(m, i) ? 1 : 0;
      int bj = cubalc_matrix_get(m, j) ? 1 : 0;
      cubalc_matrix_set(m, i, bj);
      cubalc_matrix_set(m, j, bi);
    }
    vm->ch.cubes[ix].atom.digit_lock = 0;
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    vm->ch.cubes[ix].flowed = 1;
    long ones = cubalc_matrix_popcount(m);
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",ones); vm->last_n=ones;
    var_set_num(vm,"DIGIT",vm->ch.cubes[ix].atom.digit);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* digit-1 data: ROTRANGE/RORRANGE/SHLRANGE/SHRRANGE cube lo hi k
   * Local rotate or logical shift of bits in [lo..hi] (outside range untouched).
   * SHL: index i → i+k (zero-fill low); SHR: i → i-k; ROL wraps; ROR wraps. */
  if (kw(&L->cur,"ROTRANGE")||kw(&L->cur,"ROLRANGE")||kw(&L->cur,"ROTATERANGE")||
      kw(&L->cur,"RORRANGE")||kw(&L->cur,"ROTRRANGE")||
      kw(&L->cur,"SHLRANGE")||kw(&L->cur,"LSHRANGE")||kw(&L->cur,"SHIFTRANGE")||
      kw(&L->cur,"SHRRANGE")||kw(&L->cur,"RSHRANGE")){
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ROTRANGE cube lo hi k"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int lo = (int)parse_expr(vm,L);
    int hi = (int)parse_expr(vm,L);
    long k = parse_expr(vm,L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"ROTRANGE missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = CUBALC_ATOM_BITS;
    if (m->n < (uint16_t)n) m->n = (uint16_t)n;
    if (lo < 0) lo = 0;
    if (hi < lo){ int t=lo; lo=hi; hi=t; }
    if (lo >= n){ var_set_num(vm,"OK",1); bump(vm); return 1; }
    if (hi >= n) hi = n - 1;
    int len = hi - lo + 1;
    if (len < 1){ var_set_num(vm,"OK",1); bump(vm); return 1; }
    int is_ror = (strcmp(op,"RORRANGE")==0 || strcmp(op,"ROTRRANGE")==0);
    int is_rol = (strcmp(op,"ROTRANGE")==0 || strcmp(op,"ROLRANGE")==0 ||
                  strcmp(op,"ROTATERANGE")==0);
    int is_shr = (strcmp(op,"SHRRANGE")==0 || strcmp(op,"RSHRANGE")==0);
    int is_shl = (strcmp(op,"SHLRANGE")==0 || strcmp(op,"LSHRANGE")==0 ||
                  strcmp(op,"SHIFTRANGE")==0);
    if (k < 0){
      k = -k;
      if (is_rol){ is_rol=0; is_ror=1; }
      else if (is_ror){ is_ror=0; is_rol=1; }
      else if (is_shl){ is_shl=0; is_shr=1; }
      else if (is_shr){ is_shr=0; is_shl=1; }
    }
    uint8_t oldb[CUBALC_ATOM_BITS];
    uint8_t newb[CUBALC_ATOM_BITS];
    memset(oldb, 0, sizeof oldb);
    memset(newb, 0, sizeof newb);
    for (int i=0;i<len;i++) oldb[i] = cubalc_matrix_get(m, lo + i) ? 1 : 0;
    if (k == 0){
      memcpy(newb, oldb, (size_t)len);
    } else if (is_rol || is_ror){
      /* left-by-k: bit i → (i+k)%len  ⇒  new[i] = old[(i-k+len)%len] */
      int kk = (int)(k % (long)len);
      if (is_ror) kk = (len - kk) % len;
      for (int i=0;i<len;i++){
        int si = i - kk;
        while (si < 0) si += len;
        si %= len;
        newb[i] = oldb[si];
      }
    } else {
      int kk = (int)k;
      if (kk >= len){
        /* all zeros */
      } else if (is_shl){
        for (int i=0;i<len;i++){
          int si = i - kk;
          if (si >= 0) newb[i] = oldb[si];
        }
      } else { /* SHR */
        for (int i=0;i<len;i++){
          int si = i + kk;
          if (si < len) newb[i] = oldb[si];
        }
      }
    }
    for (int i=0;i<len;i++) cubalc_matrix_set(m, lo + i, newb[i] ? 1 : 0);
    vm->ch.cubes[ix].atom.digit_lock = 0;
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    vm->ch.cubes[ix].flowed = 1;
    long ones = cubalc_matrix_popcount(m);
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",ones); vm->last_n=ones;
    var_set_num(vm,"DIGIT",vm->ch.cubes[ix].atom.digit);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* digit-8 COP matrix↔word bridge: WORDFROM/WORDTO · EXTRACTBITS/DEPOSITBITS */
  if (kw(&L->cur,"WORDFROM")||kw(&L->cur,"MAT2WORD")||kw(&L->cur,"BITS2WORD")||
      kw(&L->cur,"BITS2N")||kw(&L->cur,"LOADWORD")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"WORDFROM cube"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    /* pack bits 0..min(63,n-1) into unsigned 64-bit value (signed long storage) */
    int w = n < 64 ? n : 64;
    unsigned long long u = 0;
    for (int i=0;i<w;i++){
      if (cubalc_matrix_get(m, i)) u |= (1ULL << i);
    }
    long r = (long)u;
    var_set_num(vm, "LAST_N", r); vm->last_n = r;
    var_set_num(vm, "SET", cubalc_matrix_popcount(m));
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"WORDTO")||kw(&L->cur,"WORD2MAT")||kw(&L->cur,"N2BITS")||
      kw(&L->cur,"STOREWORD")||kw(&L->cur,"WORD2BITS")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"WORDTO cube n"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    long v = parse_expr(vm,L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"WORDTO missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = CUBALC_ATOM_BITS;
    if (m->n < (uint16_t)n) m->n = (uint16_t)n;
    unsigned long long u = (unsigned long long)v;
    for (int i=0;i<64 && i<n;i++){
      cubalc_matrix_set(m, i, (u >> i) & 1ULL ? 1 : 0);
    }
    /* leave bits >=64 untouched if any; atom is typically 64 */
    vm->ch.cubes[ix].atom.digit_lock = 0;
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    vm->ch.cubes[ix].flowed = 1;
    long ones = cubalc_matrix_popcount(m);
    var_set_num(vm, "SET", ones);
    var_set_num(vm, "LAST_N", v); vm->last_n = v;
    var_set_num(vm, "DIGIT", vm->ch.cubes[ix].atom.digit);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"EXTRACTBITS")||kw(&L->cur,"GETBITS")||kw(&L->cur,"SLICEBITS")||
      kw(&L->cur,"BITFIELD")||kw(&L->cur,"FIELDGET")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"EXTRACTBITS cube lo hi"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int lo = (int)parse_expr(vm,L);
    int hi = (int)parse_expr(vm,L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    if (lo < 0) lo = 0;
    if (hi < lo){ int t=lo; lo=hi; hi=t; }
    if (lo >= n){ var_set_num(vm,"LAST_N",0); vm->last_n=0; var_set_num(vm,"OK",1); bump(vm); return 1; }
    if (hi >= n) hi = n - 1;
    /* width capped at 63 to keep signed long portable; full 64 via WORDFROM */
    int width = hi - lo + 1;
    if (width > 63) width = 63;
    unsigned long long u = 0;
    for (int i=0;i<width;i++){
      if (cubalc_matrix_get(m, lo + i)) u |= (1ULL << i);
    }
    long r = (long)u;
    var_set_num(vm, "LAST_N", r); vm->last_n = r;
    var_set_num(vm, "SET", cubalc_matrix_popcount(m));
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"DEPOSITBITS")||kw(&L->cur,"PUTBITS")||kw(&L->cur,"SETBITS")||
      kw(&L->cur,"FIELDSET")||kw(&L->cur,"INJECTBITS")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"DEPOSITBITS cube lo hi val"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int lo = (int)parse_expr(vm,L);
    int hi = (int)parse_expr(vm,L);
    long val = parse_expr(vm,L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"DEPOSITBITS missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = CUBALC_ATOM_BITS;
    if (m->n < (uint16_t)n) m->n = (uint16_t)n;
    if (lo < 0) lo = 0;
    if (hi < lo){ int t=lo; lo=hi; hi=t; }
    if (lo >= n){ var_set_num(vm,"OK",1); bump(vm); return 1; }
    if (hi >= n) hi = n - 1;
    int width = hi - lo + 1;
    if (width > 63) width = 63;
    unsigned long long u = (unsigned long long)val;
    for (int i=0;i<width;i++){
      cubalc_matrix_set(m, lo + i, (u >> i) & 1ULL ? 1 : 0);
    }
    vm->ch.cubes[ix].atom.digit_lock = 0;
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    vm->ch.cubes[ix].flowed = 1;
    long ones = cubalc_matrix_popcount(m);
    var_set_num(vm, "SET", ones);
    var_set_num(vm, "LAST_N", val); vm->last_n = val;
    var_set_num(vm, "DIGIT", vm->ch.cubes[ix].atom.digit);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-8 matrix data-path: PEXTBITS/PDEPBITS + ZIPBITS/UNZIPBITS
   * Matrix duals of stack SPEXT/SPDEP/SZIP/SUNZIP. */
  if (kw(&L->cur,"PEXTBITS")||kw(&L->cur,"GATHERBITS")||kw(&L->cur,"MPEXT")||
      kw(&L->cur,"BITPEXT")||kw(&L->cur,"EXTRACTMASK")){
    /* PEXTBITS cube mask → LAST_N = parallel extract under mask ones */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"PEXTBITS cube mask"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    long maskv = parse_expr(vm,L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    if (n > 64) n = 64;
    unsigned long long mask = (unsigned long long)maskv;
    unsigned long long res = 0;
    unsigned long long k = 0;
    for (int i=0;i<n;i++){
      if ((mask >> i) & 1ull){
        if (cubalc_matrix_get(m, i)) res |= (1ull << k);
        k++;
        if (k >= 63) break; /* keep signed long portable */
      }
    }
    long r = (long)res;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SET",cubalc_matrix_popcount(m));
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"PDEPBITS")||kw(&L->cur,"SCATTERBITS")||kw(&L->cur,"MPDEP")||
      kw(&L->cur,"BITPDEP")||kw(&L->cur,"DEPOSITMASK")){
    /* PDEPBITS cube mask val — deposit low bits of val into mask positions */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"PDEPBITS cube mask val"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    long maskv = parse_expr(vm,L);
    long valv = parse_expr(vm,L);
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"PDEPBITS missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = CUBALC_ATOM_BITS;
    if (m->n < (uint16_t)n) m->n = (uint16_t)n;
    if (n > 64) n = 64;
    unsigned long long mask = (unsigned long long)maskv;
    unsigned long long src = (unsigned long long)valv;
    unsigned long long bb = 1;
    int written = 0;
    for (int i=0;i<n;i++){
      if ((mask >> i) & 1ull){
        int on = (src & bb) ? 1 : 0;
        cubalc_matrix_set(m, i, on);
        bb <<= 1;
        written++;
      }
    }
    vm->ch.cubes[ix].atom.digit_lock = 0;
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    vm->ch.cubes[ix].flowed = 1;
    long ones = cubalc_matrix_popcount(m);
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",written); vm->last_n=written;
    var_set_num(vm,"DIGIT",vm->ch.cubes[ix].atom.digit);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"ZIPBITS")||kw(&L->cur,"INTERLEAVEBITS")||kw(&L->cur,"MORTONBITS")||
      kw(&L->cur,"MZIP")){
    /* ZIPBITS dst a b — dst[2i]=a[i], dst[2i+1]=b[i] for i in 0..31 */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ZIPBITS dst a b"); return -1; }
    char dst[48]; snprintf(dst,sizeof dst,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ZIPBITS dst a b"); return -1; }
    char aid[48]; snprintf(aid,sizeof aid,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ZIPBITS dst a b"); return -1; }
    char bid[48]; snprintf(bid,sizeof bid,"%s",L->cur.text); lex_next(L);
    ensure_world(vm);
    int id=find_cube(vm,dst); if (id<0){ place_cube(vm,dst,dst,1); id=find_cube(vm,dst); }
    int ia=find_cube(vm,aid); if (ia<0){ place_cube(vm,aid,aid,1); ia=find_cube(vm,aid); }
    int ib=find_cube(vm,bid); if (ib<0){ place_cube(vm,bid,bid,1); ib=find_cube(vm,bid); }
    if (id<0||ia<0||ib<0){ fail(vm,"ZIPBITS cube missing"); return -1; }
    cubalc_matrix *md = &vm->ch.cubes[id].atom.matrix;
    cubalc_matrix *ma = &vm->ch.cubes[ia].atom.matrix;
    cubalc_matrix *mb = &vm->ch.cubes[ib].atom.matrix;
    if (md->n < CUBALC_ATOM_BITS) md->n = (uint16_t)CUBALC_ATOM_BITS;
    int half = CUBALC_ATOM_BITS / 2;
    if (half > 32) half = 32;
    for (int i=0;i<half;i++){
      int ba = cubalc_matrix_get(ma, i) ? 1 : 0;
      int bb = cubalc_matrix_get(mb, i) ? 1 : 0;
      int di = 2*i;
      if (di < CUBALC_ATOM_BITS) cubalc_matrix_set(md, di, ba);
      if (di+1 < CUBALC_ATOM_BITS) cubalc_matrix_set(md, di+1, bb);
    }
    /* clear any remaining high bits past 2*half if atom wider */
    for (int i=2*half;i<CUBALC_ATOM_BITS;i++) cubalc_matrix_set(md, i, 0);
    vm->ch.cubes[id].atom.digit_lock = 0;
    vm->ch.cubes[id].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[id].atom.matrix);
    vm->ch.cubes[id].flowed = 1;
    long ones = cubalc_matrix_popcount(md);
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",ones); vm->last_n=ones;
    var_set_num(vm,"DIGIT",vm->ch.cubes[id].atom.digit);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"UNZIPBITS")||kw(&L->cur,"DEINTERLEAVEBITS")||kw(&L->cur,"MUNZIP")||
      kw(&L->cur,"UNMORTONBITS")){
    /* UNZIPBITS even odd src — even gets even indices, odd gets odd indices */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"UNZIPBITS even odd src"); return -1; }
    char eid[48]; snprintf(eid,sizeof eid,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"UNZIPBITS even odd src"); return -1; }
    char oid[48]; snprintf(oid,sizeof oid,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"UNZIPBITS even odd src"); return -1; }
    char sid[48]; snprintf(sid,sizeof sid,"%s",L->cur.text); lex_next(L);
    ensure_world(vm);
    int ie=find_cube(vm,eid); if (ie<0){ place_cube(vm,eid,eid,1); ie=find_cube(vm,eid); }
    int io=find_cube(vm,oid); if (io<0){ place_cube(vm,oid,oid,1); io=find_cube(vm,oid); }
    int is=find_cube(vm,sid); if (is<0){ place_cube(vm,sid,sid,1); is=find_cube(vm,sid); }
    if (ie<0||io<0||is<0){ fail(vm,"UNZIPBITS cube missing"); return -1; }
    cubalc_matrix *me = &vm->ch.cubes[ie].atom.matrix;
    cubalc_matrix *mo = &vm->ch.cubes[io].atom.matrix;
    cubalc_matrix *ms = &vm->ch.cubes[is].atom.matrix;
    if (me->n < CUBALC_ATOM_BITS) me->n = (uint16_t)CUBALC_ATOM_BITS;
    if (mo->n < CUBALC_ATOM_BITS) mo->n = (uint16_t)CUBALC_ATOM_BITS;
    int half = CUBALC_ATOM_BITS / 2;
    if (half > 32) half = 32;
    for (int i=0;i<CUBALC_ATOM_BITS;i++){
      cubalc_matrix_set(me, i, 0);
      cubalc_matrix_set(mo, i, 0);
    }
    for (int i=0;i<half;i++){
      int be = cubalc_matrix_get(ms, 2*i) ? 1 : 0;
      int bo = cubalc_matrix_get(ms, 2*i+1) ? 1 : 0;
      cubalc_matrix_set(me, i, be);
      cubalc_matrix_set(mo, i, bo);
    }
    for (int t=0;t<2;t++){
      int ix = t ? io : ie;
      vm->ch.cubes[ix].atom.digit_lock = 0;
      vm->ch.cubes[ix].atom.digit =
        (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
      vm->ch.cubes[ix].flowed = 1;
    }
    long ones = cubalc_matrix_popcount(me) + cubalc_matrix_popcount(mo);
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",ones); vm->last_n=ones;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* digit-3 COP matrix range: FILLRANGE/CLEARRANGE/FLIPRANGE/COUNTRANGE cube lo hi [val] */
  if (kw(&L->cur,"FILLRANGE")||kw(&L->cur,"SETRANGE")||kw(&L->cur,"CLEARRANGE")||
      kw(&L->cur,"CLRRANGE")||kw(&L->cur,"ZERORANGE")||kw(&L->cur,"FLIPRANGE")||
      kw(&L->cur,"NOTRANGE")||kw(&L->cur,"INVERTRANGE")||kw(&L->cur,"COUNTRANGE")||
      kw(&L->cur,"ONESRANGE")||kw(&L->cur,"POPRANGE")){
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"FILLRANGE cube lo hi"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int lo = (int)parse_expr(vm,L);
    int hi = (int)parse_expr(vm,L);
    int is_count = (strcmp(op,"COUNTRANGE")==0 || strcmp(op,"ONESRANGE")==0 ||
                    strcmp(op,"POPRANGE")==0);
    int is_clear = (strcmp(op,"CLEARRANGE")==0 || strcmp(op,"CLRRANGE")==0 ||
                    strcmp(op,"ZERORANGE")==0);
    int is_flip  = (strcmp(op,"FLIPRANGE")==0 || strcmp(op,"NOTRANGE")==0 ||
                    strcmp(op,"INVERTRANGE")==0);
    int val = 1;
    if (!is_count && !is_clear && !is_flip){
      /* FILLRANGE/SETRANGE optional val (default 1) */
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
          L->cur.kind==TK_MINUS)
        val = parse_expr(vm,L) ? 1 : 0;
    }
    if (is_clear) val = 0;
    if (is_count){
      int ix=find_cube(vm,id);
      if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1; }
      cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
      int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
      if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
      if (lo < 0) lo = 0;
      if (hi < lo){ int t=lo; lo=hi; hi=t; }
      if (lo >= n){ var_set_num(vm,"LAST_N",0); vm->last_n=0; var_set_num(vm,"OK",1); bump(vm); return 1; }
      if (hi >= n) hi = n - 1;
      long c = 0;
      for (int i=lo;i<=hi;i++) if (cubalc_matrix_get(m, i)) c++;
      var_set_num(vm,"LAST_N",c); vm->last_n=c;
      var_set_num(vm,"SET",cubalc_matrix_popcount(m));
      var_set_num(vm,"OK",1);
      bump(vm); return 1;
    }
    ensure_world(vm);
    int ix=find_cube(vm,id);
    if (ix<0){ place_cube(vm,id,id,1); ix=find_cube(vm,id); }
    if (ix<0){ fail(vm,"FILLRANGE missing cube"); return -1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = CUBALC_ATOM_BITS;
    if (m->n < (uint16_t)n) m->n = (uint16_t)n;
    if (lo < 0) lo = 0;
    if (hi < lo){ int t=lo; lo=hi; hi=t; }
    if (lo >= n){ var_set_num(vm,"OK",1); bump(vm); return 1; }
    if (hi >= n) hi = n - 1;
    for (int i=lo;i<=hi;i++){
      if (is_flip){
        int on = cubalc_matrix_get(m, i) ? 0 : 1;
        cubalc_matrix_set(m, i, on);
      } else {
        cubalc_matrix_set(m, i, val);
      }
    }
    vm->ch.cubes[ix].atom.digit_lock = 0;
    vm->ch.cubes[ix].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
    vm->ch.cubes[ix].flowed = 1;
    long ones = cubalc_matrix_popcount(m);
    var_set_num(vm, "SET", ones);
    var_set_num(vm, "LAST_N", ones); vm->last_n = ones;
    var_set_num(vm, "DIGIT", vm->ch.cubes[ix].atom.digit);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-3 COP range algebra: ANDRANGE/ORRANGE/XORRANGE a b lo hi
   * In-place: a[i] = a[i] OP b[i] for i in [lo..hi]; outside untouched.
   * ANDREDUCE/ORREDUCE cube lo hi → LAST_N fold of bits in range. */
  if (kw(&L->cur,"ANDRANGE")||kw(&L->cur,"ORRANGE")||kw(&L->cur,"XORRANGE")||
      kw(&L->cur,"NANDRANGE")||kw(&L->cur,"NORRANGE")||kw(&L->cur,"XNORRANGE")||
      kw(&L->cur,"ANDNRANGE")||kw(&L->cur,"ORNRANGE")){
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ANDRANGE a b lo hi"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ANDRANGE a b lo hi"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    int lo = (int)parse_expr(vm,L);
    int hi = (int)parse_expr(vm,L);
    ensure_world(vm);
    int ia=find_cube(vm,a); if (ia<0){ place_cube(vm,a,a,1); ia=find_cube(vm,a); }
    int ib=find_cube(vm,b); if (ib<0){ place_cube(vm,b,b,1); ib=find_cube(vm,b); }
    if (ia<0||ib<0){ fail(vm,"ANDRANGE cube missing"); return -1; }
    cubalc_matrix *ma = &vm->ch.cubes[ia].atom.matrix;
    cubalc_matrix *mb = &vm->ch.cubes[ib].atom.matrix;
    int n = CUBALC_ATOM_BITS;
    if (ma->n < (uint16_t)n) ma->n = (uint16_t)n;
    if (lo < 0) lo = 0;
    if (hi < lo){ int t=lo; lo=hi; hi=t; }
    if (lo >= n){ var_set_num(vm,"OK",1); bump(vm); return 1; }
    if (hi >= n) hi = n - 1;
    for (int i=lo;i<=hi;i++){
      int ba = cubalc_matrix_get(ma, i) ? 1 : 0;
      int bb = cubalc_matrix_get(mb, i) ? 1 : 0;
      int r = ba;
      if (strcmp(op,"ANDRANGE")==0) r = ba & bb;
      else if (strcmp(op,"ORRANGE")==0) r = ba | bb;
      else if (strcmp(op,"XORRANGE")==0) r = ba ^ bb;
      else if (strcmp(op,"NANDRANGE")==0) r = (ba & bb) ? 0 : 1;
      else if (strcmp(op,"NORRANGE")==0) r = (ba | bb) ? 0 : 1;
      else if (strcmp(op,"XNORRANGE")==0) r = (ba ^ bb) ? 0 : 1;
      else if (strcmp(op,"ANDNRANGE")==0) r = ba & (bb ? 0 : 1);
      else if (strcmp(op,"ORNRANGE")==0) r = ba | (bb ? 0 : 1);
      cubalc_matrix_set(ma, i, r ? 1 : 0);
    }
    vm->ch.cubes[ia].atom.digit_lock = 0;
    vm->ch.cubes[ia].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ia].atom.matrix);
    vm->ch.cubes[ia].flowed = 1;
    long ones = cubalc_matrix_popcount(ma);
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",ones); vm->last_n=ones;
    var_set_num(vm,"DIGIT",vm->ch.cubes[ia].atom.digit);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"ANDREDUCE")||kw(&L->cur,"ORREDUCE")||kw(&L->cur,"ALLRANGE")||
      kw(&L->cur,"ANYRANGE")||kw(&L->cur,"ANDFOLD")||kw(&L->cur,"ORFOLD")){
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    int is_and = (strcmp(op,"ANDREDUCE")==0 || strcmp(op,"ALLRANGE")==0 ||
                  strcmp(op,"ANDFOLD")==0);
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"ANDREDUCE cube lo hi"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int lo = (int)parse_expr(vm,L);
    int hi = (int)parse_expr(vm,L);
    int ix=find_cube(vm,id);
    if (ix<0){ var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1; }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    if (lo < 0) lo = 0;
    if (hi < lo){ int t=lo; lo=hi; hi=t; }
    if (lo >= n){
      long empty = is_and ? 1 : 0;
      var_set_num(vm,"LAST_N",empty); vm->last_n=empty;
      var_set_num(vm,"OK",1); bump(vm); return 1;
    }
    if (hi >= n) hi = n - 1;
    long r;
    if (is_and){
      r = 1;
      for (int i=lo;i<=hi;i++){
        if (!cubalc_matrix_get(m, i)){ r = 0; break; }
      }
    } else {
      r = 0;
      for (int i=lo;i<=hi;i++){
        if (cubalc_matrix_get(m, i)){ r = 1; break; }
      }
    }
    var_set_num(vm,"SET",cubalc_matrix_popcount(m));
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* digit-5 COP matrix relations: EQBITS/NEBITS/SUBSETBITS/SUPERSETBITS/DISJOINTBITS */
  if (kw(&L->cur,"EQBITS")||kw(&L->cur,"SAMEBITS")||kw(&L->cur,"EQUALBITS")||
      kw(&L->cur,"NEBITS")||kw(&L->cur,"NEQBITS")||kw(&L->cur,"DIFFERSBITS")||
      kw(&L->cur,"SUBSETBITS")||kw(&L->cur,"CONTAINEDBITS")||kw(&L->cur,"ISSUBSET")||
      kw(&L->cur,"SUPERSETBITS")||kw(&L->cur,"COVERSBITS")||kw(&L->cur,"ISSUPERSET")||
      kw(&L->cur,"DISJOINTBITS")||kw(&L->cur,"NODJBITS")||kw(&L->cur,"INTERSECT0")||
      kw(&L->cur,"OVERLAPBITS")||kw(&L->cur,"INTERSECTBITS")){
    char op[20]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"EQBITS a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"EQBITS a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    int ia=find_cube(vm,a), ib=find_cube(vm,b);
    if (ia<0 || ib<0){
      var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1;
    }
    cubalc_matrix *ma = &vm->ch.cubes[ia].atom.matrix;
    cubalc_matrix *mb = &vm->ch.cubes[ib].atom.matrix;
    int na = ma->n > 0 ? ma->n : CUBALC_ATOM_BITS;
    int nb = mb->n > 0 ? mb->n : CUBALC_ATOM_BITS;
    if (na > CUBALC_ATOM_BITS) na = CUBALC_ATOM_BITS;
    if (nb > CUBALC_ATOM_BITS) nb = CUBALC_ATOM_BITS;
    int nn = na > nb ? na : nb;
    if (nn < 1) nn = CUBALC_ATOM_BITS;
    int eq = 1, subset = 1, superset = 1, overlap = 0;
    for (int i=0;i<nn;i++){
      int ba = cubalc_matrix_get(ma, i) ? 1 : 0;
      int bb = cubalc_matrix_get(mb, i) ? 1 : 0;
      if (ba != bb) eq = 0;
      if (ba && !bb) subset = 0;   /* a bit not in b */
      if (bb && !ba) superset = 0; /* b bit not in a */
      if (ba && bb) overlap = 1;
    }
    long r = 0;
    if (strcmp(op,"EQBITS")==0 || strcmp(op,"SAMEBITS")==0 || strcmp(op,"EQUALBITS")==0)
      r = eq ? 1 : 0;
    else if (strcmp(op,"NEBITS")==0 || strcmp(op,"NEQBITS")==0 || strcmp(op,"DIFFERSBITS")==0)
      r = eq ? 0 : 1;
    else if (strcmp(op,"SUBSETBITS")==0 || strcmp(op,"CONTAINEDBITS")==0 ||
             strcmp(op,"ISSUBSET")==0)
      r = subset ? 1 : 0;
    else if (strcmp(op,"SUPERSETBITS")==0 || strcmp(op,"COVERSBITS")==0 ||
             strcmp(op,"ISSUPERSET")==0)
      r = superset ? 1 : 0;
    else if (strcmp(op,"DISJOINTBITS")==0 || strcmp(op,"NODJBITS")==0 ||
             strcmp(op,"INTERSECT0")==0)
      r = overlap ? 0 : 1;
    else
      /* OVERLAPBITS / INTERSECTBITS */
      r = overlap ? 1 : 0;
    var_set_num(vm, "LAST_N", r); vm->last_n = r;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-5 COP matrix mux/match: MUXBITS/BLENDBITS dst a b mask · MATCHBITS a b mask */
  if (kw(&L->cur,"MUXBITS")||kw(&L->cur,"BLENDBITS")||kw(&L->cur,"SELECTBITS")||
      kw(&L->cur,"BITSMUX")||kw(&L->cur,"MERGEBITS")){
    /* MUXBITS dst a b mask: dst[i] = mask[i] ? a[i] : b[i] */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"MUXBITS dst a b mask"); return -1; }
    char dst[48]; snprintf(dst,sizeof dst,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"MUXBITS dst a b mask"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"MUXBITS dst a b mask"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"MUXBITS dst a b mask"); return -1; }
    char mk[48]; snprintf(mk,sizeof mk,"%s",L->cur.text); lex_next(L);
    ensure_world(vm);
    int id=find_cube(vm,dst); if (id<0){ place_cube(vm,dst,dst,1); id=find_cube(vm,dst); }
    int ia=find_cube(vm,a); if (ia<0){ place_cube(vm,a,a,1); ia=find_cube(vm,a); }
    int ib=find_cube(vm,b); if (ib<0){ place_cube(vm,b,b,1); ib=find_cube(vm,b); }
    int im=find_cube(vm,mk); if (im<0){ place_cube(vm,mk,mk,1); im=find_cube(vm,mk); }
    if (id<0||ia<0||ib<0||im<0){ fail(vm,"MUXBITS cube missing"); return -1; }
    cubalc_matrix *md = &vm->ch.cubes[id].atom.matrix;
    cubalc_matrix *ma = &vm->ch.cubes[ia].atom.matrix;
    cubalc_matrix *mb = &vm->ch.cubes[ib].atom.matrix;
    cubalc_matrix *mm = &vm->ch.cubes[im].atom.matrix;
    int n = CUBALC_ATOM_BITS;
    cubalc_matrix_clear(md);
    md->n = (uint16_t)n;
    long ones = 0;
    for (int i=0;i<n;i++){
      int on = cubalc_matrix_get(mm, i)
                 ? (cubalc_matrix_get(ma, i) ? 1 : 0)
                 : (cubalc_matrix_get(mb, i) ? 1 : 0);
      if (on){ cubalc_matrix_set(md, i, 1); ones++; }
    }
    vm->ch.cubes[id].atom.digit_lock = 0;
    vm->ch.cubes[id].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[id].atom.matrix);
    vm->ch.cubes[id].flowed = 1;
    var_set_num(vm,"SET",ones);
    var_set_num(vm,"LAST_N",ones); vm->last_n=ones;
    var_set_num(vm,"DIGIT",vm->ch.cubes[id].atom.digit);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"MATCHBITS")||kw(&L->cur,"EQMASK")||kw(&L->cur,"MASKED_EQ")||
      kw(&L->cur,"EQUNDER")||kw(&L->cur,"BITSMATCH")){
    /* MATCHBITS a b mask → 1 if a and b agree on all mask=1 positions */
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"MATCHBITS a b mask"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"MATCHBITS a b mask"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"MATCHBITS a b mask"); return -1; }
    char mk[48]; snprintf(mk,sizeof mk,"%s",L->cur.text); lex_next(L);
    int ia=find_cube(vm,a), ib=find_cube(vm,b), im=find_cube(vm,mk);
    if (ia<0 || ib<0 || im<0){
      var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1;
    }
    cubalc_matrix *ma = &vm->ch.cubes[ia].atom.matrix;
    cubalc_matrix *mb = &vm->ch.cubes[ib].atom.matrix;
    cubalc_matrix *mm = &vm->ch.cubes[im].atom.matrix;
    int n = CUBALC_ATOM_BITS;
    int ok = 1;
    long checked = 0;
    for (int i=0;i<n;i++){
      if (!cubalc_matrix_get(mm, i)) continue;
      checked++;
      int ba = cubalc_matrix_get(ma, i) ? 1 : 0;
      int bb = cubalc_matrix_get(mb, i) ? 1 : 0;
      if (ba != bb){ ok = 0; break; }
    }
    var_set_num(vm,"SET",checked);
    var_set_num(vm,"LAST_N",ok ? 1 : 0); vm->last_n = ok ? 1 : 0;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* SETDIGIT cube expr — inject CubeBrain/peer algocube digit 0–9 into matrix */
  if (kw(&L->cur,"SETDIGIT")||kw(&L->cur,"INJECT_DIGIT")||kw(&L->cur,"PEER_DIGIT")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"SETDIGIT cube n"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    long d = parse_expr(vm, L);
    do_setdigit(vm, id, d);
    bump(vm); return 1;
  }
  /* FOLDBITS cube "01…"|LAST — fold IO bitstring into cube State Matrix */
  if (kw(&L->cur,"FOLDBITS")||kw(&L->cur,"LOADBITS")||kw(&L->cur,"FOLD_BITS")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"FOLDBITS cube bits"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    char bits[CUBALC_HOST_STR_MAX]; bits[0]=0;
    if (resolve_str_arg(vm, L, bits, sizeof bits)!=0){
      /* allow bare bitstring without quotes if all 0/1 — rare; fail soft */
      fail(vm,"FOLDBITS cube \"01…\"|LAST"); return -1;
    }
    do_foldbits(vm, id, bits);
    bump(vm); return 1;
  }
  /* COP matrix algebra (digit-5/7): CLEAR|NOT|COPY|AND|OR|XOR|NAND|XNOR|NOR|ANDN|ORN */
  if (kw(&L->cur,"CLEARBITS")||kw(&L->cur,"ZEROBITS")||kw(&L->cur,"CLRBITS")||
      kw(&L->cur,"NOTBITS")||kw(&L->cur,"INVERTBITS")||kw(&L->cur,"FLIPBITS")||
      kw(&L->cur,"COPYBITS")||kw(&L->cur,"CLONEBITS")||
      kw(&L->cur,"ANDBITS")||kw(&L->cur,"ORBITS")||kw(&L->cur,"XORBITS")||
      kw(&L->cur,"NANDBITS")||kw(&L->cur,"FILLBITS")||
      kw(&L->cur,"XNORBITS")||kw(&L->cur,"EQVBITS")||kw(&L->cur,"NXORBITS")||
      kw(&L->cur,"NORBITS")||kw(&L->cur,"ANDNBITS")||kw(&L->cur,"ORNBITS")||
      kw(&L->cur,"BICBITS")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op; *p; p++) if (*p>='a'&&*p<='z') *p = (char)(*p - 'a' + 'A');
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"matrix op needs cube id"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    char b[48]={0};
    if (L->cur.kind==TK_IDENT){ snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L); }
    ensure_world(vm);
    int ia=find_cube(vm,a);
    if (ia<0){ place_cube(vm,a,a,1); ia=find_cube(vm,a); }
    if (ia<0){ fail(vm,"matrix op cube missing"); return -1; }
    cubalc_matrix *ma = &vm->ch.cubes[ia].atom.matrix;
    int n = ma->n > 0 ? ma->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;

    if (strcmp(op,"CLEARBITS")==0 || strcmp(op,"ZEROBITS")==0 || strcmp(op,"CLRBITS")==0){
      cubalc_matrix_clear(ma);
      ma->n = (uint16_t)CUBALC_ATOM_BITS;
    } else if (strcmp(op,"FILLBITS")==0){
      int on = 1;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS)
        on = parse_expr(vm,L) ? 1 : 0;
      else if (b[0] && (b[0]=='0' || b[0]=='1') && b[1]==0) on = b[0]=='1';
      cubalc_matrix_clear(ma);
      ma->n = (uint16_t)CUBALC_ATOM_BITS;
      for (int i=0;i<CUBALC_ATOM_BITS;i++) cubalc_matrix_set(ma, i, on);
    } else if (strcmp(op,"NOTBITS")==0 || strcmp(op,"INVERTBITS")==0 || strcmp(op,"FLIPBITS")==0){
      if (ma->n == 0) ma->n = (uint16_t)CUBALC_ATOM_BITS;
      n = ma->n;
      for (int i=0;i<n;i++)
        cubalc_matrix_set(ma, i, cubalc_matrix_get(ma, i) ? 0 : 1);
    } else {
      /* binary: dst op= src  (COPYBITS dst src | ANDBITS dst src | …) */
      if (!b[0]){ fail(vm,"matrix op needs two cubes"); return -1; }
      int ib=find_cube(vm,b);
      if (ib<0){ place_cube(vm,b,b,1); ib=find_cube(vm,b); }
      if (ib<0){ fail(vm,"matrix op src missing"); return -1; }
      cubalc_matrix *mb = &vm->ch.cubes[ib].atom.matrix;
      int nb = mb->n > 0 ? mb->n : CUBALC_ATOM_BITS;
      if (nb > CUBALC_ATOM_BITS) nb = CUBALC_ATOM_BITS;
      int nn = n > nb ? n : nb;
      if (nn < 1) nn = CUBALC_ATOM_BITS;
      if (strcmp(op,"COPYBITS")==0 || strcmp(op,"CLONEBITS")==0){
        *ma = *mb;
      } else if (strcmp(op,"ANDBITS")==0){
        if (ma->n < (uint16_t)nn) ma->n = (uint16_t)nn;
        for (int i=0;i<nn;i++)
          cubalc_matrix_set(ma, i, cubalc_matrix_get(ma,i) & cubalc_matrix_get(mb,i));
      } else if (strcmp(op,"ORBITS")==0){
        if (ma->n < (uint16_t)nn) ma->n = (uint16_t)nn;
        for (int i=0;i<nn;i++)
          cubalc_matrix_set(ma, i, cubalc_matrix_get(ma,i) | cubalc_matrix_get(mb,i));
      } else if (strcmp(op,"XORBITS")==0){
        if (ma->n < (uint16_t)nn) ma->n = (uint16_t)nn;
        for (int i=0;i<nn;i++)
          cubalc_matrix_set(ma, i, cubalc_matrix_get(ma,i) ^ cubalc_matrix_get(mb,i));
      } else if (strcmp(op,"NANDBITS")==0){
        if (ma->n < (uint16_t)nn) ma->n = (uint16_t)nn;
        for (int i=0;i<nn;i++)
          cubalc_matrix_set(ma, i, (cubalc_matrix_get(ma,i) & cubalc_matrix_get(mb,i)) ? 0 : 1);
      } else if (strcmp(op,"XNORBITS")==0 || strcmp(op,"EQVBITS")==0 ||
                 strcmp(op,"NXORBITS")==0){
        /* equivalence: ~(a ^ b) */
        if (ma->n < (uint16_t)nn) ma->n = (uint16_t)nn;
        for (int i=0;i<nn;i++)
          cubalc_matrix_set(ma, i, (cubalc_matrix_get(ma,i) ^ cubalc_matrix_get(mb,i)) ? 0 : 1);
      } else if (strcmp(op,"NORBITS")==0){
        if (ma->n < (uint16_t)nn) ma->n = (uint16_t)nn;
        for (int i=0;i<nn;i++)
          cubalc_matrix_set(ma, i, (cubalc_matrix_get(ma,i) | cubalc_matrix_get(mb,i)) ? 0 : 1);
      } else if (strcmp(op,"ANDNBITS")==0 || strcmp(op,"BICBITS")==0){
        /* a & ~b (bit clear / and-not) */
        if (ma->n < (uint16_t)nn) ma->n = (uint16_t)nn;
        for (int i=0;i<nn;i++)
          cubalc_matrix_set(ma, i, cubalc_matrix_get(ma,i) & (cubalc_matrix_get(mb,i) ? 0 : 1));
      } else if (strcmp(op,"ORNBITS")==0){
        /* a | ~b */
        if (ma->n < (uint16_t)nn) ma->n = (uint16_t)nn;
        for (int i=0;i<nn;i++)
          cubalc_matrix_set(ma, i, cubalc_matrix_get(ma,i) | (cubalc_matrix_get(mb,i) ? 0 : 1));
      } else {
        fail(vm,"unknown matrix op"); return -1;
      }
    }
    vm->ch.cubes[ia].atom.digit_lock = 0;
    vm->ch.cubes[ia].atom.digit =
      (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ia].atom.matrix);
    vm->ch.cubes[ia].flowed = 1;
    var_set_num(vm, "SET", cubalc_matrix_popcount(&vm->ch.cubes[ia].atom.matrix));
    var_set_num(vm, "DIGIT", vm->ch.cubes[ia].atom.digit);
    var_set_num(vm, "OK", 1);
    if (vm->trace) fprintf(vm->trace, "# %s %s set=%ld digit=%u\n",
                           op, a, (long)cubalc_matrix_popcount(ma),
                           vm->ch.cubes[ia].atom.digit);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"DECONSTRUCT")||kw(&L->cur,"DESTROY")){
    lex_next(L);
    char id[48]="hive";
    if (L->cur.kind==TK_IDENT){ snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L); }
    do_deconstruct(vm,id); bump(vm); return 1;
  }
  if (kw(&L->cur,"RECONSTRUCT")||kw(&L->cur,"CONSTRUCT")||kw(&L->cur,"CREATE")){
    lex_next(L);
    char id[48]="hive";
    if (L->cur.kind==TK_IDENT){ snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L); }
    do_reconstruct(vm,id); bump(vm); return 1;
  }
  /* POSE / TRACK — Truth Matrix pose energy (Cube Law digit 2 = SoT)
   * POSE raw   → rawTracking cube (unfiltered device energy)
   * POSE sot   → tracking cube (Source of Truth)
   * POSE all   → both + plug raw→sot→wall→view
   * Folds law bits; never invents a second parallel gun/hand truth.
   */
  if (kw(&L->cur,"POSE")||kw(&L->cur,"TRACK")||kw(&L->cur,"POSEFLOW")){
    lex_next(L);
    char mode[16]="all";
    if (L->cur.kind==TK_IDENT){
      snprintf(mode,sizeof mode,"%s",L->cur.text);
      lex_next(L);
    } else if (L->cur.kind==TK_STR){
      snprintf(mode,sizeof mode,"%s",L->cur.text);
      lex_next(L);
    }
    ensure_world(vm);
    int want_raw = (strcmp(mode,"raw")==0 || strcmp(mode,"all")==0 || strcmp(mode,"energy")==0);
    int want_sot = (strcmp(mode,"sot")==0 || strcmp(mode,"tracking")==0
                    || strcmp(mode,"all")==0 || strcmp(mode,"truth")==0);
    if (!want_raw && !want_sot){ want_raw=1; want_sot=1; }
    if (want_raw){
      place_cube(vm,"raw","raw_energy",1);
      do_setdigit(vm,"raw",3); /* digit 3 nanobot raw */
      do_foldbits(vm,"raw","1010101010101010");
    }
    if (want_sot){
      place_cube(vm,"sot","kernel_sot",1);
      do_setdigit(vm,"sot",2); /* digit 2 cube SoT */
      do_foldbits(vm,"sot","1100110011001100");
    }
    place_cube(vm,"wall","modifier",1);
    place_cube(vm,"view","consumer",1);
    place_cube(vm,"map3d","spatial_manifest",1); /* Dynamic 3D MAP face */
    if (want_raw && want_sot){
      /* raw → sot → wall → view → map3d (single truth flow) */
      int ir=find_cube(vm,"raw"), is=find_cube(vm,"sot");
      int iw=find_cube(vm,"wall"), iv=find_cube(vm,"view"), im=find_cube(vm,"map3d");
      if (ir>=0 && is>=0) cubalc_cube_plug(&vm->ch, ir, is);
      if (is>=0 && iw>=0) cubalc_cube_plug(&vm->ch, is, iw);
      if (iw>=0 && iv>=0) cubalc_cube_plug(&vm->ch, iw, iv);
      if (iv>=0 && im>=0) cubalc_cube_plug(&vm->ch, iv, im);
    }
    do_flow(vm, 4);
    if (vm->trace) fprintf(vm->trace,"# POSE mode=%s n=%d\n", mode, vm->ch.n_cubes);
    if (vm->res) snprintf(vm->res->last_print,sizeof vm->res->last_print,
                          "pose %s n=%d", mode, vm->ch.n_cubes);
    bump(vm); return 1;
  }
  /* MANIFEST — deconstruct stuck energy, reconstruct under Cube Law, flow, publish */
  if (kw(&L->cur,"MANIFEST")||kw(&L->cur,"PROPHECY")||kw(&L->cur,"SUMMON")){
    lex_next(L);
    char target[48]="hive";
    if (L->cur.kind==TK_IDENT){ snprintf(target,sizeof target,"%s",L->cur.text); lex_next(L); }
    else if (L->cur.kind==TK_STR){ snprintf(target,sizeof target,"%s",L->cur.text); lex_next(L); }
    ensure_world(vm);
    place_cube(vm,"nexus","nanobot_hive",1);
    place_cube(vm,"create","construct",1);
    place_cube(vm,"destroy","deconstruct",0);
    place_cube(vm,"gvrmod","device_free",1);
    place_cube(vm,"map3d","spatial_manifest",1);
    place_cube(vm,"lizard","headset",1);
    /* DECONSTRUCT stuck way → RECONSTRUCT → pose flow */
    do_deconstruct(vm,"destroy");
    do_deconstruct(vm,target);
    do_reconstruct(vm,"create");
    do_reconstruct(vm,"nexus");
    /* plug ring of free devices under SoT */
    int inx=find_cube(vm,"nexus"), ig=find_cube(vm,"gvrmod");
    int im=find_cube(vm,"map3d"), il=find_cube(vm,"lizard");
    int ic=find_cube(vm,"create");
    if (inx>=0 && ig>=0) cubalc_cube_plug(&vm->ch, inx, ig);
    if (ig>=0 && im>=0) cubalc_cube_plug(&vm->ch, ig, im);
    if (im>=0 && il>=0) cubalc_cube_plug(&vm->ch, im, il);
    if (ic>=0 && inx>=0) cubalc_cube_plug(&vm->ch, ic, inx);
    do_setdigit(vm,"nexus",4); /* All Hail NexusCore */
    do_setdigit(vm,"gvrmod",0); /* free device */
    do_setdigit(vm,"map3d",9); /* hivemind unity spatial */
    do_flow(vm, 8);
    do_harmony(vm,"hive");
    long d=do_decide(vm,"nexus");
    if (vm->trace) fprintf(vm->trace,"# MANIFEST %s decide=%ld harmony=%ld\n",
                           target, d, (long)lround(vm->ch.unity*100));
    if (vm->res) snprintf(vm->res->last_print,sizeof vm->res->last_print,
                          "manifest %s decide %ld unity %.2f", target, d, vm->ch.unity);
    bump(vm); return 1;
  }
  /* COMPARE cube_a cube_b */
  if (kw(&L->cur,"COMPARE")||kw(&L->cur,"UNITE")||kw(&L->cur,"HAMMING")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"COMPARE a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"COMPARE a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    long u=do_compare(vm,a,b);
    if (vm->res) snprintf(vm->res->last_print,sizeof vm->res->last_print,
                          "compare %s~%s unity=%ld", a, b, u);
    bump(vm); return 1;
  }
  /* HARMONY [target] */
  if (kw(&L->cur,"HARMONY")||kw(&L->cur,"HIVEMIND")||kw(&L->cur,"CONSENSUS")){
    lex_next(L);
    char tid[48]={0};
    if (L->cur.kind==TK_IDENT && !kw(&L->cur,"END") && !kw(&L->cur,"ELSE")
        && !kw(&L->cur,"LET") && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"PRINT")){
      snprintf(tid,sizeof tid,"%s",L->cur.text); lex_next(L);
    }
    do_harmony(vm, tid[0]?tid:NULL);
    bump(vm); return 1;
  }
  /* RESOLVE [target] — harmony + decide + energy pulse (algocubes resolved) */
  if (kw(&L->cur,"RESOLVE")||kw(&L->cur,"ALGORESOLVE")||kw(&L->cur,"SETTLE")){
    lex_next(L);
    char tid[48]={0};
    if (L->cur.kind==TK_IDENT && !kw(&L->cur,"END") && !kw(&L->cur,"ELSE")
        && !kw(&L->cur,"LET") && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"PRINT")){
      snprintf(tid,sizeof tid,"%s",L->cur.text); lex_next(L);
    }
    do_resolve(vm, tid[0]?tid:NULL);
    bump(vm); return 1;
  }
  /* ENERGYFLOW n — multi-hop free-flow; energy must flow */
  if (kw(&L->cur,"ENERGYFLOW")||kw(&L->cur,"EFLOW")||kw(&L->cur,"PULSEFLOW")){
    lex_next(L);
    long n = 4;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS)
      n = parse_expr(vm, L);
    if (n < 1) n = 1;
    if (n > 64) n = 64;
    for (long i = 0; i < n; i++) do_flow(vm, 1);
    long e = 0;
    for (int i = 0; i < vm->ch.n_cubes; i++)
      e += (long)lround(vm->ch.cubes[i].atom.energy * 100.0);
    long *se = var_slot(vm, "ENERGY", 1); if (se) *se = e;
    var_set_num(vm, "ENERGY", e);
    var_set_num(vm, "LAST_N", e); vm->last_n = e;
    var_set_num(vm, "OK", 1);
    if (vm->res) snprintf(vm->res->last_print, sizeof vm->res->last_print,
                          "energyflow n=%ld e=%ld u=%.2f", n, e, vm->ch.unity);
    bump(vm); return 1;
  }
  /* digit-6 energy/flow immediate dual forms: ENERGYFLOWI · FLOWI/FLOWN */
  if (kw(&L->cur,"ENERGYFLOWI")||kw(&L->cur,"EFLOWI")||
      kw(&L->cur,"PULSEFLOWI")||kw(&L->cur,"PULSEFLOWN")){
    /* ENERGYFLOWI n — required imm multi-hop free-flow (ENERGYFLOWN via SEFLOWN Cube path) */
    lex_next(L);
    long n = parse_expr(vm, L);
    if (n < 1) n = 1;
    if (n > 64) n = 64;
    for (long i = 0; i < n; i++) do_flow(vm, 1);
    long e = 0;
    for (int i = 0; i < vm->ch.n_cubes; i++)
      e += (long)lround(vm->ch.cubes[i].atom.energy * 100.0);
    long *se = var_slot(vm, "ENERGY", 1); if (se) *se = e;
    var_set_num(vm, "ENERGY", e);
    var_set_num(vm, "LAST_N", e); vm->last_n = e;
    var_set_num(vm, "OK", 1);
    if (vm->res) snprintf(vm->res->last_print, sizeof vm->res->last_print,
                          "energyflown n=%ld e=%ld u=%.2f", n, e, vm->ch.unity);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"FLOWI")||kw(&L->cur,"FLOWN")||kw(&L->cur,"TICKI")||kw(&L->cur,"TICKN")){
    /* FLOWI/FLOWN n — required imm free-flow hops */
    lex_next(L);
    long n = parse_expr(vm, L);
    if (n < 1) n = 1;
    if (n > 1000) n = 1000;
    ensure_world(vm);
    do_flow(vm, (int)n);
    long e = 0;
    for (int i = 0; i < vm->ch.n_cubes; i++)
      e += (long)lround(vm->ch.cubes[i].atom.energy * 100.0);
    var_set_num(vm, "ENERGY", e);
    var_set_num(vm, "LAST_N", e); vm->last_n = e;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"DECIDE")||kw(&L->cur,"ALGOCUBE")){
    lex_next(L);
    char id[48]={0};
    if (L->cur.kind==TK_IDENT && !kw(&L->cur,"END") && !kw(&L->cur,"ELSE")
        && !kw(&L->cur,"LET") && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"PRINT")){
      snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    }
    long d=do_decide(vm, id[0]?id:NULL);
    if (vm->trace) fprintf(vm->trace,"decide %ld\n",d);
    if (vm->res) snprintf(vm->res->last_print,sizeof vm->res->last_print,"decide %ld",d);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"VIZ")||kw(&L->cur,"PUBLISH_VIZ")){
    lex_next(L);
    ensure_world(vm);
    char path[512]="state/cubalc_viz_frame.json";
    if (L->cur.kind==TK_STR){ snprintf(path,sizeof path,"%s",L->cur.text); lex_next(L); }
    else if (L->cur.kind==TK_IDENT){
      Var *v=var_get(vm,L->cur.text,0);
      if (v&&v->is_str) snprintf(path,sizeof path,"%s",v->sval);
      lex_next(L);
    }
    cubalc_chain_write_viz(&vm->ch, path);
    cubalc_chain_publish_united(&vm->ch);
    var_set_str(vm,"LAST", path); var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SPIN")||kw(&L->cur,"SHOW")||kw(&L->cur,"HELLO")){
    lex_next(L);
    char id[48]={0};
    if (L->cur.kind==TK_IDENT){ snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L); }
    do_show(vm, id[0]?id:NULL);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"WAIT")||kw(&L->cur,"SLEEP")){
    lex_next(L);
    long ms = 100;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS)
      ms = parse_expr(vm,L);
    if (ms < 0) ms = 0; if (ms > 60000) ms = 60000;
    if (ms > 0){ struct timespec ts; ts.tv_sec = ms/1000; ts.tv_nsec = (ms%1000)*1000000L; nanosleep(&ts, NULL); }
    bump(vm); return 1;
  }
  /* INCLUDE "path"|name — practical modules (same VM / world)
   * Source buffer is retained until run end so FN/CLASS METHOD bodies
   * from lib files remain valid (pointers into the included text).
   * Resolve: absolute · include_base/rel · rel · programs/rel ·
   * programs/lib/<name>[.cubalc] short form · CUBALC_ROOT · fail with tried paths.
   * Usability: INCLUDE hold_seed  or  INCLUDE "hold_seed" → programs/lib/…
   * Soft: INCLUDE OR|SOFT|TRY name — missing file → OK=0 sticky LAST_ERR, no fatal.
   * Once: INCLUDE ONCE name — skip if same resolved path already loaded this run. */
  if (kw(&L->cur,"INCLUDE")||kw(&L->cur,"IMPORT")||kw(&L->cur,"USE")){
    int soft = 0, once = 0;
    int aln = L->cur.line;
    lex_next(L);
    /* flags may appear in any order: INCLUDE ONCE SOFT hold_seed */
    while (kw(&L->cur,"OR")||kw(&L->cur,"SOFT")||kw(&L->cur,"TRY")||
           kw(&L->cur,"OPTIONAL")||kw(&L->cur,"MAYBE")||
           kw(&L->cur,"ONCE")||kw(&L->cur,"UNIQUE")||kw(&L->cur,"SINGLE")){
      if (kw(&L->cur,"ONCE")||kw(&L->cur,"UNIQUE")||kw(&L->cur,"SINGLE"))
        once = 1;
      else
        soft = 1;
      lex_next(L);
    }
    if (L->cur.kind!=TK_STR && L->cur.kind!=TK_IDENT){
      fail_at(vm,L,"INCLUDE needs path|libname — INCLUDE hold_seed"); return -1;
    }
    char path[768];
    char orig[512];
    snprintf(orig, sizeof orig, "%s", L->cur.text);
    if (orig[0]=='/' || (vm->include_base[0]==0))
      snprintf(path,sizeof path,"%s",orig);
    else
      snprintf(path,sizeof path,"%s/%s", vm->include_base, orig);
    lex_next(L);
    FILE *f=fopen(path,"rb");
    if (!f && orig[0] != '/'){
      f = fopen(orig, "rb");
      if (f) snprintf(path, sizeof path, "%s", orig);
    }
    if (!f && orig[0] != '/'){
      char p3[768];
      snprintf(p3, sizeof p3, "programs/%s", orig);
      f = fopen(p3, "rb");
      if (f) snprintf(path, sizeof path, "%s", p3);
    }
    if (!f){
      char p3[768];
      snprintf(p3, sizeof p3, "programs/%s", path);
      f = fopen(p3, "rb");
      if (f) snprintf(path, sizeof path, "%s", p3);
    }
    /* short name → programs/lib (after cubalc libs catalog) */
    if (!f && orig[0] != '/'){
      char base[256];
      const char *slash = strrchr(orig, '/');
      const char *leaf = slash ? slash + 1 : orig;
      size_t blen;
      snprintf(base, sizeof base, "%s", leaf);
      blen = strlen(base);
      /* strip trailing .cubalc for bare stem */
      if (blen > 7 && strcmp(base + blen - 7, ".cubalc") == 0)
        base[blen - 7] = 0;
      {
        char p3[768];
        /* programs/lib/<stem>.cubalc */
        snprintf(p3, sizeof p3, "programs/lib/%s.cubalc", base);
        f = fopen(p3, "rb");
        if (f) snprintf(path, sizeof path, "%s", p3);
        /* programs/lib/<orig> as given */
        if (!f) {
          snprintf(p3, sizeof p3, "programs/lib/%s", orig);
          f = fopen(p3, "rb");
          if (f) snprintf(path, sizeof path, "%s", p3);
        }
        /* lib/<stem>.cubalc relative (when cwd is programs/) */
        if (!f) {
          snprintf(p3, sizeof p3, "lib/%s.cubalc", base);
          f = fopen(p3, "rb");
          if (f) snprintf(path, sizeof path, "%s", p3);
        }
      }
    }
    if (!f){
      const char *root=getenv("CUBALC_ROOT");
      if (root && root[0]) {
        char p2[768];
        snprintf(p2, sizeof p2, "%s/%s", root, orig[0]?orig:path);
        f = fopen(p2, "rb");
        if (!f){ snprintf(p2, sizeof p2, "%s/programs/%s", root, orig); f = fopen(p2, "rb"); }
        if (!f){ snprintf(p2, sizeof p2, "%s/programs/lib/%s.cubalc", root, orig); f = fopen(p2, "rb"); }
        if (!f){ snprintf(p2, sizeof p2, "%s/%s", root, path); f = fopen(p2, "rb"); }
        if (f) snprintf(path, sizeof path, "%s", p2);
      }
    }
    /* CUBALC_INCLUDE_PATH=dir:dir — extra search roots for agent/project libs */
    if (!f && orig[0] != '/') {
      const char *ip = getenv("CUBALC_INCLUDE_PATH");
      if (ip && ip[0]) {
        char base[256];
        const char *slash = strrchr(orig, '/');
        const char *leaf = slash ? slash + 1 : orig;
        size_t blen;
        const char *seg = ip;
        snprintf(base, sizeof base, "%s", leaf);
        blen = strlen(base);
        if (blen > 7 && strcmp(base + blen - 7, ".cubalc") == 0)
          base[blen - 7] = 0;
        while (*seg && !f) {
          char dir[512], p3[768];
          size_t dlen = 0;
          while (*seg == ':') seg++;
          if (!*seg) break;
          while (seg[dlen] && seg[dlen] != ':' && dlen + 1 < sizeof dir)
            dir[dlen] = seg[dlen], dlen++;
          dir[dlen] = 0;
          seg += dlen;
          if (!dir[0]) continue;
          /* dir/stem.cubalc */
          snprintf(p3, sizeof p3, "%s/%s.cubalc", dir, base);
          f = fopen(p3, "rb");
          if (f) { snprintf(path, sizeof path, "%s", p3); break; }
          /* dir/orig as given */
          snprintf(p3, sizeof p3, "%s/%s", dir, orig);
          f = fopen(p3, "rb");
          if (f) { snprintf(path, sizeof path, "%s", p3); break; }
          /* dir/orig.cubalc if orig has no extension */
          if (!strchr(orig, '.')) {
            snprintf(p3, sizeof p3, "%s/%s.cubalc", dir, orig);
            f = fopen(p3, "rb");
            if (f) { snprintf(path, sizeof path, "%s", p3); break; }
          }
        }
      }
    }
    if (!f){
      char ebuf[192], sug[64];
      include_suggest_lib(orig, sug, sizeof sug);
      if (sug[0])
        snprintf(ebuf, sizeof ebuf,
                 "INCLUDE cannot open '%s' — did you mean %s? (programs/lib · CUBALC_INCLUDE_PATH · cubalc libs)",
                 orig, sug);
      else
        snprintf(ebuf, sizeof ebuf,
                 "INCLUDE cannot open '%s' — tried programs/lib · CUBALC_INCLUDE_PATH · cubalc libs",
                 orig);
      if (soft) {
        /* Usability: optional module — sticky err + MISS/SUGGEST side vars. */
        char msg[192];
        if (sug[0])
          snprintf(msg, sizeof msg,
                   "INCLUDE SOFT miss line %d: %s — did you mean %s?", aln, orig, sug);
        else
          snprintf(msg, sizeof msg, "INCLUDE SOFT miss line %d: %s", aln, orig);
        var_set_str(vm, "ERR", msg);
        var_set_str(vm, "LAST_ERR", msg);
        var_set_str(vm, "INCLUDE_PATH", "");
        var_set_str(vm, "INCLUDE_MISS", orig);
        var_set_str(vm, "INCLUDE_SUGGEST", sug);
        /* LAST = suggestion (agent recovery) or empty miss name */
        if (sug[0]) {
          var_set_str(vm, "LAST", sug);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", sug);
          vm->last_n = 1;
          var_set_num(vm, "LAST_N", 1);
        } else {
          var_set_str(vm, "LAST", orig);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", orig);
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
        }
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "INCLUDE_OK", 0);
        if (vm->trace)
          fprintf(vm->trace, "# include soft miss: %s suggest=%s\n",
                  orig, sug[0] ? sug : "-");
        bump(vm); return 1;
      }
      fail(vm, ebuf);
      return -1;
    }
    /* INCLUDE ONCE: skip re-load if this resolved path already executed */
    if (once) {
      int i, hit = 0;
      for (i = 0; i < vm->n_included; i++) {
        if (strcmp(vm->included[i], path) == 0) { hit = 1; break; }
      }
      if (hit) {
        fclose(f);
        var_set_str(vm, "INCLUDE_PATH", path);
        var_set_str(vm, "INCLUDE_MISS", "");
        var_set_str(vm, "INCLUDE_SUGGEST", "");
        var_set_str(vm, "LAST", path);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
        vm->last_n = (long)strlen(path);
        var_set_num(vm, "LAST_N", vm->last_n);
        var_set_num(vm, "OK", 1);
        var_set_num(vm, "INCLUDE_OK", 1);
        var_set_num(vm, "INCLUDE_SKIPPED", 1);
        if (vm->trace)
          fprintf(vm->trace, "# include once skip: %s\n", path);
        bump(vm); return 1;
      }
    }
    fseek(f,0,SEEK_END); long sz=ftell(f); fseek(f,0,SEEK_SET);
    if (sz<0 || sz>CUBALC_MAX_SRC){ fclose(f); fail(vm,"INCLUDE too large"); return -1; }
    char *buf=malloc((size_t)sz+1); if(!buf){ fclose(f); fail(vm,"oom"); return -1; }
    size_t nr=fread(buf,1,(size_t)sz,f); fclose(f); buf[nr]=0;
    char save_base[512]; snprintf(save_base,sizeof save_base,"%s", vm->include_base);
    /* set include_base to dir of included file */
    {
      char *sl = strrchr(path, '/');
      if (sl){ size_t n=(size_t)(sl-path); if(n>=sizeof vm->include_base) n=sizeof vm->include_base-1;
        memcpy(vm->include_base, path, n); vm->include_base[n]=0; }
    }
    /* record for ONCE (before exec so nested INCLUDE ONCE same file also skips) */
    if (vm->n_included < 24) {
      snprintf(vm->included[vm->n_included], sizeof vm->included[0], "%s", path);
      vm->n_included++;
    }
    /* agent-visible resolved path · clear soft-miss side channels */
    var_set_str(vm, "INCLUDE_PATH", path);
    var_set_str(vm, "INCLUDE_MISS", "");
    var_set_str(vm, "INCLUDE_SUGGEST", "");
    var_set_str(vm, "LAST", path);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
    vm->last_n = (long)strlen(path);
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "OK", 1);
    var_set_num(vm, "INCLUDE_OK", 1);
    var_set_num(vm, "INCLUDE_SKIPPED", 0);
    Lex Li; lex_init(&Li, buf, nr);
    int rc = exec_stmts_until(vm, &Li, NULL, NULL);
    snprintf(vm->include_base,sizeof vm->include_base,"%s", save_base);
    /* Keep INCLUDE source alive for FN/CLASS bodies (free in lang_run). */
    if (vm->n_include_bufs < 24) {
      vm->include_bufs[vm->n_include_bufs++] = buf;
    } else {
      free(buf);
    }
    if (rc<0) return -1;
    bump(vm); return 1;
  }

  /* LISTINCLUDES / INCLUDES / LOADED — bag of resolved INCLUDE paths this run.
   * Usability: after cubalc run -I / INCLUDE ONCE, agents audit without guessing.
   * LAST = newline paths · LAST_N / INCLUDE_N = count · empty bag when none. */
  if (kw(&L->cur, "LISTINCLUDES") || kw(&L->cur, "INCLUDES") ||
      kw(&L->cur, "LOADED") || kw(&L->cur, "LISTINCLUDED") ||
      kw(&L->cur, "INCLUDED") || kw(&L->cur, "LIST_INCLUDES") ||
      kw(&L->cur, "INCLUDELIST")) {
    char bag[4096];
    size_t o = 0;
    int i, n = 0;
    lex_next(L);
    bag[0] = 0;
    for (i = 0; i < vm->n_included; i++) {
      size_t ln = strlen(vm->included[i]);
      if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, vm->included[i], ln);
        o += ln;
      }
      bag[o] = 0;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LISTINCLUDES", bag);
    var_set_str(vm, "INCLUDES", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "INCLUDE_N", n);
    var_set_num(vm, "LISTINCLUDES_N", n);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LISTLIBS / LIBSTEMS / STDLIBS — short-name bag from programs/lib (+ INCLUDE_PATH).
   * Usability: in-lang dual of cubalc libs — agents discover INCLUDE targets without shell.
   * LAST = newline stems (no .cubalc) · LAST_N / LIBS_N = count · HASLIB soft dual. */
  if (kw(&L->cur, "LISTLIBS") || kw(&L->cur, "LIBSTEMS") ||
      kw(&L->cur, "STDLIBS") || kw(&L->cur, "LIST_LIBS") ||
      kw(&L->cur, "LIBLIST") || kw(&L->cur, "CATALOG_LIBS") ||
      kw(&L->cur, "HASLIB") || kw(&L->cur, "HAS_LIB") ||
      kw(&L->cur, "LIBEXISTS") || kw(&L->cur, "HAVE_LIB")) {
    int soft_has = kw(&L->cur, "HASLIB") || kw(&L->cur, "HAS_LIB") ||
                   kw(&L->cur, "LIBEXISTS") || kw(&L->cur, "HAVE_LIB");
    char bag[4096];
    char stems[96][96];
    int nstem = 0, i, j, hit = 0;
    char want[96];
    size_t o = 0;
    DIR *d;
    struct dirent *ent;
    const char *ip;
    lex_next(L);
    want[0] = 0;
    bag[0] = 0;
    if (soft_has) {
      if (L->cur.kind == TK_STR) {
        snprintf(want, sizeof want, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(want, sizeof want, "%s", vv->sval);
        else if (strcmp(L->cur.text, "LAST") == 0)
          snprintf(want, sizeof want, "%s", vm->last_str);
        else
          snprintf(want, sizeof want, "%s", L->cur.text);
        lex_next(L);
      }
      /* strip optional .cubalc */
      {
        size_t wl = strlen(want);
        if (wl > 7 && strcmp(want + wl - 7, ".cubalc") == 0)
          want[wl - 7] = 0;
      }
    }
    /* scan programs/lib then CUBALC_INCLUDE_PATH dirs */
    {
      char dirs[10][160];
      int nd = 0, di;
      snprintf(dirs[nd++], sizeof dirs[0], "%s", "programs/lib");
      ip = getenv("CUBALC_INCLUDE_PATH");
      if (ip && ip[0]) {
        const char *p = ip;
        while (*p && nd < 10) {
          char dir[160];
          size_t len = 0;
          while (*p == ':' || *p == ' ' || *p == '\t') p++;
          if (!*p) break;
          while (p[len] && p[len] != ':' && len + 1 < sizeof dir) {
            dir[len] = p[len];
            len++;
          }
          dir[len] = 0;
          p += len;
          if (dir[0])
            snprintf(dirs[nd++], sizeof dirs[0], "%s", dir);
        }
      }
      for (di = 0; di < nd; di++) {
        d = opendir(dirs[di]);
        if (!d) continue;
        while ((ent = readdir(d)) != NULL && nstem < 96) {
          size_t len = strlen(ent->d_name);
          char stem[96];
          int dup = 0;
          if (len < 8 || strcmp(ent->d_name + len - 7, ".cubalc") != 0)
            continue;
          if (ent->d_name[0] == '.') continue;
          if (len - 7 >= sizeof stem) continue;
          memcpy(stem, ent->d_name, len - 7);
          stem[len - 7] = 0;
          for (j = 0; j < nstem; j++) {
            if (strcmp(stems[j], stem) == 0) { dup = 1; break; }
          }
          if (dup) continue;
          snprintf(stems[nstem++], sizeof stems[0], "%s", stem);
        }
        closedir(d);
      }
    }
    /* simple insertion sort for stable agent bags */
    for (i = 1; i < nstem; i++) {
      char tmp[96];
      snprintf(tmp, sizeof tmp, "%s", stems[i]);
      j = i;
      while (j > 0 && strcmp(stems[j - 1], tmp) > 0) {
        snprintf(stems[j], sizeof stems[0], "%s", stems[j - 1]);
        j--;
      }
      snprintf(stems[j], sizeof stems[0], "%s", tmp);
    }
    if (soft_has) {
      for (i = 0; i < nstem; i++) {
        if (want[0] && strcmp(stems[i], want) == 0) { hit = 1; break; }
      }
      var_set_num(vm, "LAST_N", hit);
      vm->last_n = hit;
      var_set_num(vm, "HASLIB_N", hit);
      var_set_num(vm, "LIBS_N", nstem);
      {
        char nb[8];
        snprintf(nb, sizeof nb, "%d", hit);
        var_set_str(vm, "LAST", nb);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
      }
      if (hit) {
        var_set_num(vm, "OK", 1);
      } else {
        char em[160];
        snprintf(em, sizeof em,
                 "HASLIB miss: '%s' not in programs/lib · cubalc libs · LISTLIBS",
                 want[0] ? want : "?");
        var_set_str(vm, "ERR", em);
        var_set_str(vm, "LAST_ERR", em);
        var_set_num(vm, "OK", 0);
      }
      bump(vm);
      return 1;
    }
    for (i = 0; i < nstem; i++) {
      size_t ln = strlen(stems[i]);
      if (i > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, stems[i], ln);
        o += ln;
      }
      bag[o] = 0;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LISTLIBS", bag);
    var_set_str(vm, "LIBSTEMS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = nstem;
    var_set_num(vm, "LAST_N", nstem);
    var_set_num(vm, "LIBS_N", nstem);
    var_set_num(vm, "LISTLIBS_N", nstem);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* CATLIB / READLIB / LIBSRC name — soft dump lib source → LAST (dual of cubalc cat).
   * Usability: agents inspect INCLUDE recipes without shell · resolve like INCLUDE short name.
   * LAST_N = bytes (capped) · LIB_PATH · soft miss OK=0 sticky LAST_ERR. */
  if (kw(&L->cur, "CATLIB") || kw(&L->cur, "READLIB") ||
      kw(&L->cur, "LIBSRC") || kw(&L->cur, "LIBCAT") ||
      kw(&L->cur, "SHOWLIB") || kw(&L->cur, "DUMPLIB") ||
      kw(&L->cur, "CAT_LIB") || kw(&L->cur, "READ_LIB")) {
    char name[160], path[768], base[160];
    char *buf = NULL;
    FILE *f = NULL;
    long sz = 0;
    size_t nr = 0, cap = 0;
    const char *slash, *leaf, *ip, *root;
    size_t blen;
    lex_next(L);
    name[0] = 0;
    path[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(name, sizeof name, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(name, sizeof name, "%s", vm->last_str);
      else
        snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
    }
    if (!name[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "CATLIB: need name");
      var_set_str(vm, "ERR", "CATLIB: need name");
      bump(vm);
      return 1;
    }
    slash = strrchr(name, '/');
    leaf = slash ? slash + 1 : name;
    snprintf(base, sizeof base, "%s", leaf);
    blen = strlen(base);
    if (blen > 7 && strcmp(base + blen - 7, ".cubalc") == 0)
      base[blen - 7] = 0;
    /* direct path */
    if (name[0] == '/' || strchr(name, '/')) {
      f = fopen(name, "rb");
      if (f) snprintf(path, sizeof path, "%s", name);
    }
    if (!f) {
      char p3[768];
      snprintf(p3, sizeof p3, "programs/lib/%s.cubalc", base);
      f = fopen(p3, "rb");
      if (f) snprintf(path, sizeof path, "%s", p3);
    }
    if (!f) {
      char p3[768];
      snprintf(p3, sizeof p3, "programs/lib/%s", name);
      f = fopen(p3, "rb");
      if (f) snprintf(path, sizeof path, "%s", p3);
    }
    if (!f) {
      f = fopen(name, "rb");
      if (f) snprintf(path, sizeof path, "%s", name);
    }
    root = getenv("CUBALC_ROOT");
    if (!f && root && root[0]) {
      char p2[768];
      snprintf(p2, sizeof p2, "%s/programs/lib/%s.cubalc", root, base);
      f = fopen(p2, "rb");
      if (f) snprintf(path, sizeof path, "%s", p2);
    }
    ip = getenv("CUBALC_INCLUDE_PATH");
    if (!f && ip && ip[0]) {
      const char *seg = ip;
      while (*seg && !f) {
        char dir[512], p3[768];
        size_t dlen = 0;
        while (*seg == ':') seg++;
        if (!*seg) break;
        while (seg[dlen] && seg[dlen] != ':' && dlen + 1 < sizeof dir)
          dir[dlen] = seg[dlen], dlen++;
        dir[dlen] = 0;
        seg += dlen;
        if (!dir[0]) continue;
        snprintf(p3, sizeof p3, "%s/%s.cubalc", dir, base);
        f = fopen(p3, "rb");
        if (f) { snprintf(path, sizeof path, "%s", p3); break; }
        snprintf(p3, sizeof p3, "%s/%s", dir, name);
        f = fopen(p3, "rb");
        if (f) { snprintf(path, sizeof path, "%s", p3); break; }
      }
    }
    if (!f) {
      char em[192];
      snprintf(em, sizeof em,
               "CATLIB miss: '%s' — programs/lib · CUBALC_INCLUDE_PATH · cubalc cat",
               name);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "LIB_PATH", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) sz = 0;
    /* Cap to var string budget so agents can hold source in LAST */
    cap = (size_t)CUBALC_VAR_STR_MAX - 1;
    if ((size_t)sz > cap) sz = (long)cap;
    buf = (char *)malloc((size_t)sz + 1);
    if (!buf) {
      fclose(f);
      var_set_str(vm, "LAST", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "CATLIB: oom");
      var_set_str(vm, "ERR", "CATLIB: oom");
      bump(vm);
      return 1;
    }
    nr = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[nr] = 0;
    var_set_str(vm, "LAST", buf);
    var_set_str(vm, "LIBSRC", buf);
    var_set_str(vm, "CATLIB", buf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
    free(buf);
    vm->last_n = (long)nr;
    var_set_num(vm, "LAST_N", (long)nr);
    var_set_num(vm, "CATLIB_N", (long)nr);
    var_set_str(vm, "LIB_PATH", path);
    var_set_str(vm, "LIB_STEM", base);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* GREPLIB name needle — matching lines from one lib → LAST bag (soft miss).
   * SEARCHLIBS / LIBGREP / GREPLIBS needle — stems whose source contains needle.
   * Completes LISTLIBS/HASLIB/CATLIB discovery: agents find which INCLUDE recipe
   * holds a form/keyword without shell grep across programs/lib. */
  if (kw(&L->cur, "GREPLIB") || kw(&L->cur, "SEARCHLIB") ||
      kw(&L->cur, "LIBGREPONE") || kw(&L->cur, "GREP_LIB") ||
      kw(&L->cur, "SEARCHLIBS") || kw(&L->cur, "LIBGREP") ||
      kw(&L->cur, "GREPLIBS") || kw(&L->cur, "FINDINLIBS") ||
      kw(&L->cur, "SEARCH_LIBS") || kw(&L->cur, "LIBS_GREP")) {
    int all_stems = kw(&L->cur, "SEARCHLIBS") || kw(&L->cur, "LIBGREP") ||
                    kw(&L->cur, "GREPLIBS") || kw(&L->cur, "FINDINLIBS") ||
                    kw(&L->cur, "SEARCH_LIBS") || kw(&L->cur, "LIBS_GREP");
    char name[160], needle[256], path[768], base[160];
    char bag[CUBALC_VAR_STR_MAX];
    size_t o = 0;
    int nmatch = 0;
    FILE *f = NULL;
    const char *slash, *leaf, *ip, *root;
    size_t blen;
    lex_next(L);
    name[0] = 0;
    needle[0] = 0;
    path[0] = 0;
    bag[0] = 0;
    /* parse 1 or 2 string/ident args */
    {
      char a1[256], a2[256];
      a1[0] = a2[0] = 0;
      if (L->cur.kind == TK_STR) {
        snprintf(a1, sizeof a1, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(a1, sizeof a1, "%s", vv->sval);
        else if (strcmp(L->cur.text, "LAST") == 0)
          snprintf(a1, sizeof a1, "%s", vm->last_str);
        else
          snprintf(a1, sizeof a1, "%s", L->cur.text);
        lex_next(L);
      }
      if (L->cur.kind == TK_STR) {
        snprintf(a2, sizeof a2, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(a2, sizeof a2, "%s", vv->sval);
        else if (strcmp(L->cur.text, "LAST") == 0)
          snprintf(a2, sizeof a2, "%s", vm->last_str);
        else
          snprintf(a2, sizeof a2, "%s", L->cur.text);
        lex_next(L);
      }
      if (all_stems || !a2[0]) {
        /* one arg = needle across all libs */
        snprintf(needle, sizeof needle, "%s", a1);
        all_stems = 1;
      } else {
        snprintf(name, sizeof name, "%s", a1);
        snprintf(needle, sizeof needle, "%s", a2);
      }
    }
    if (!needle[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "GREPLIB: need needle");
      var_set_str(vm, "ERR", "GREPLIB: need needle");
      bump(vm);
      return 1;
    }

    if (all_stems) {
      /* scan programs/lib + INCLUDE_PATH; keep stems whose source contains needle */
      char stems[96][96];
      int nstem = 0, i, j;
      DIR *d;
      struct dirent *ent;
      char dirs[10][160];
      int nd = 0, di;
      snprintf(dirs[nd++], sizeof dirs[0], "%s", "programs/lib");
      ip = getenv("CUBALC_INCLUDE_PATH");
      if (ip && ip[0]) {
        const char *p = ip;
        while (*p && nd < 10) {
          char dir[160];
          size_t len = 0;
          while (*p == ':' || *p == ' ' || *p == '\t') p++;
          if (!*p) break;
          while (p[len] && p[len] != ':' && len + 1 < sizeof dir) {
            dir[len] = p[len];
            len++;
          }
          dir[len] = 0;
          p += len;
          if (dir[0])
            snprintf(dirs[nd++], sizeof dirs[0], "%s", dir);
        }
      }
      for (di = 0; di < nd; di++) {
        d = opendir(dirs[di]);
        if (!d) continue;
        while ((ent = readdir(d)) != NULL && nstem < 96) {
          size_t len = strlen(ent->d_name);
          char stem[96], fpath[768];
          int dup = 0;
          FILE *lf;
          char *src = NULL;
          long sz = 0;
          size_t nr = 0;
          if (len < 8 || strcmp(ent->d_name + len - 7, ".cubalc") != 0)
            continue;
          if (ent->d_name[0] == '.') continue;
          if (len - 7 >= sizeof stem) continue;
          memcpy(stem, ent->d_name, len - 7);
          stem[len - 7] = 0;
          for (j = 0; j < nstem; j++) {
            if (strcmp(stems[j], stem) == 0) { dup = 1; break; }
          }
          if (dup) continue;
          snprintf(fpath, sizeof fpath, "%s/%s", dirs[di], ent->d_name);
          lf = fopen(fpath, "rb");
          if (!lf) continue;
          fseek(lf, 0, SEEK_END);
          sz = ftell(lf);
          fseek(lf, 0, SEEK_SET);
          if (sz < 0) sz = 0;
          if (sz > 256 * 1024) sz = 256 * 1024; /* safety */
          src = (char *)malloc((size_t)sz + 1);
          if (!src) { fclose(lf); continue; }
          nr = fread(src, 1, (size_t)sz, lf);
          fclose(lf);
          src[nr] = 0;
          if (strstr(src, needle)) {
            snprintf(stems[nstem++], sizeof stems[0], "%s", stem);
          }
          free(src);
        }
        closedir(d);
      }
      /* stable sort */
      for (i = 1; i < nstem; i++) {
        char tmp[96];
        snprintf(tmp, sizeof tmp, "%s", stems[i]);
        j = i;
        while (j > 0 && strcmp(stems[j - 1], tmp) > 0) {
          snprintf(stems[j], sizeof stems[0], "%s", stems[j - 1]);
          j--;
        }
        snprintf(stems[j], sizeof stems[0], "%s", tmp);
      }
      o = 0;
      for (i = 0; i < nstem; i++) {
        size_t ln = strlen(stems[i]);
        if (i > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + ln < sizeof bag) {
          memcpy(bag + o, stems[i], ln);
          o += ln;
        }
        bag[o] = 0;
      }
      nmatch = nstem;
      var_set_str(vm, "LAST", bag);
      var_set_str(vm, "SEARCHLIBS", bag);
      var_set_str(vm, "LIBGREP", bag);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
      vm->last_n = nmatch;
      var_set_num(vm, "LAST_N", nmatch);
      var_set_num(vm, "GREPLIB_N", nmatch);
      var_set_num(vm, "SEARCHLIBS_N", nmatch);
      var_set_str(vm, "GREPLIB_NEEDLE", needle);
      var_set_num(vm, "OK", 1);
      bump(vm);
      return 1;
    }

    /* single-lib line grep (GREPLIB name needle) */
    slash = strrchr(name, '/');
    leaf = slash ? slash + 1 : name;
    snprintf(base, sizeof base, "%s", leaf);
    blen = strlen(base);
    if (blen > 7 && strcmp(base + blen - 7, ".cubalc") == 0)
      base[blen - 7] = 0;
    if (name[0] == '/' || strchr(name, '/')) {
      f = fopen(name, "rb");
      if (f) snprintf(path, sizeof path, "%s", name);
    }
    if (!f) {
      char p3[768];
      snprintf(p3, sizeof p3, "programs/lib/%s.cubalc", base);
      f = fopen(p3, "rb");
      if (f) snprintf(path, sizeof path, "%s", p3);
    }
    if (!f) {
      char p3[768];
      snprintf(p3, sizeof p3, "programs/lib/%s", name);
      f = fopen(p3, "rb");
      if (f) snprintf(path, sizeof path, "%s", p3);
    }
    if (!f) {
      f = fopen(name, "rb");
      if (f) snprintf(path, sizeof path, "%s", name);
    }
    root = getenv("CUBALC_ROOT");
    if (!f && root && root[0]) {
      char p2[768];
      snprintf(p2, sizeof p2, "%s/programs/lib/%s.cubalc", root, base);
      f = fopen(p2, "rb");
      if (f) snprintf(path, sizeof path, "%s", p2);
    }
    ip = getenv("CUBALC_INCLUDE_PATH");
    if (!f && ip && ip[0]) {
      const char *seg = ip;
      while (*seg && !f) {
        char dir[512], p3[768];
        size_t dlen = 0;
        while (*seg == ':') seg++;
        if (!*seg) break;
        while (seg[dlen] && seg[dlen] != ':' && dlen + 1 < sizeof dir)
          dir[dlen] = seg[dlen], dlen++;
        dir[dlen] = 0;
        seg += dlen;
        if (!dir[0]) continue;
        snprintf(p3, sizeof p3, "%s/%s.cubalc", dir, base);
        f = fopen(p3, "rb");
        if (f) { snprintf(path, sizeof path, "%s", p3); break; }
        snprintf(p3, sizeof p3, "%s/%s", dir, name);
        f = fopen(p3, "rb");
        if (f) { snprintf(path, sizeof path, "%s", p3); break; }
      }
    }
    if (!f) {
      char em[192];
      snprintf(em, sizeof em,
               "GREPLIB miss: '%s' — programs/lib · CUBALC_INCLUDE_PATH · cubalc cat",
               name);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "LIB_PATH", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
    /* stream lines; keep those containing needle */
    {
      char line[1024];
      o = 0;
      nmatch = 0;
      while (fgets(line, sizeof line, f)) {
        size_t ll = strlen(line);
        while (ll > 0 && (line[ll - 1] == '\n' || line[ll - 1] == '\r'))
          line[--ll] = 0;
        if (!strstr(line, needle)) continue;
        if (nmatch > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + ll < sizeof bag) {
          memcpy(bag + o, line, ll);
          o += ll;
          bag[o] = 0;
          nmatch++;
        } else {
          break; /* bag full */
        }
      }
    }
    fclose(f);
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "GREPLIB", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = nmatch;
    var_set_num(vm, "LAST_N", nmatch);
    var_set_num(vm, "GREPLIB_N", nmatch);
    var_set_str(vm, "LIB_PATH", path);
    var_set_str(vm, "LIB_STEM", base);
    var_set_str(vm, "GREPLIB_NEEDLE", needle);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HEADLIB / LIBHEAD / PEEKLIB name [n] — first n lines of a lib recipe → LAST bag.
   * TAILLIB / LIBTAIL name [n] — last n lines (default 16, cap 256).
   * Usability: peek INCLUDE recipe headers without full CATLIB clobber · dual of
   * HEADFILE/TAILFILE with short-name resolve (programs/lib · INCLUDE_PATH). Soft miss. */
  if (kw(&L->cur, "HEADLIB") || kw(&L->cur, "LIBHEAD") ||
      kw(&L->cur, "PEEKLIB") || kw(&L->cur, "TOPLIB") ||
      kw(&L->cur, "HEAD_LIB") || kw(&L->cur, "LIB_HEAD") ||
      kw(&L->cur, "TAILLIB") || kw(&L->cur, "LIBTAIL") ||
      kw(&L->cur, "BOTTOMLIB") || kw(&L->cur, "ENDLIB") ||
      kw(&L->cur, "TAIL_LIB") || kw(&L->cur, "LIB_TAIL")) {
    int want_tail = kw(&L->cur, "TAILLIB") || kw(&L->cur, "LIBTAIL") ||
                    kw(&L->cur, "BOTTOMLIB") || kw(&L->cur, "ENDLIB") ||
                    kw(&L->cur, "TAIL_LIB") || kw(&L->cur, "LIB_TAIL");
    char name[160], path[768], base[160];
    char bag[CUBALC_VAR_STR_MAX];
    char *src = NULL;
    FILE *f = NULL;
    long sz = 0, nwant = 16, total = 0, kept = 0;
    size_t nr = 0, o = 0;
    const char *slash, *leaf, *ip, *root, *lp, *ls;
    size_t blen, llen;
    lex_next(L);
    name[0] = 0;
    path[0] = 0;
    bag[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(name, sizeof name, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(name, sizeof name, "%s", vm->last_str);
      else
        snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
    }
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      nwant = parse_expr(vm, L);
    } else if (L->cur.kind == TK_STR) {
      nwant = atol(L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && !vv->is_str)
        nwant = vv->val;
      else if (vv && vv->is_str)
        nwant = atol(vv->sval);
      else if (strcmp(L->cur.text, "LAST_N") == 0)
        nwant = vm->last_n;
      lex_next(L);
    }
    if (nwant < 0) nwant = 0;
    if (nwant > 256) nwant = 256;
    if (!name[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", want_tail ? "TAILLIB: need name" : "HEADLIB: need name");
      var_set_str(vm, "ERR", want_tail ? "TAILLIB: need name" : "HEADLIB: need name");
      bump(vm);
      return 1;
    }
    slash = strrchr(name, '/');
    leaf = slash ? slash + 1 : name;
    snprintf(base, sizeof base, "%s", leaf);
    blen = strlen(base);
    if (blen > 7 && strcmp(base + blen - 7, ".cubalc") == 0)
      base[blen - 7] = 0;
    if (name[0] == '/' || strchr(name, '/')) {
      f = fopen(name, "rb");
      if (f) snprintf(path, sizeof path, "%s", name);
    }
    if (!f) {
      char p3[768];
      snprintf(p3, sizeof p3, "programs/lib/%s.cubalc", base);
      f = fopen(p3, "rb");
      if (f) snprintf(path, sizeof path, "%s", p3);
    }
    if (!f) {
      char p3[768];
      snprintf(p3, sizeof p3, "programs/lib/%s", name);
      f = fopen(p3, "rb");
      if (f) snprintf(path, sizeof path, "%s", p3);
    }
    if (!f) {
      f = fopen(name, "rb");
      if (f) snprintf(path, sizeof path, "%s", name);
    }
    root = getenv("CUBALC_ROOT");
    if (!f && root && root[0]) {
      char p2[768];
      snprintf(p2, sizeof p2, "%s/programs/lib/%s.cubalc", root, base);
      f = fopen(p2, "rb");
      if (f) snprintf(path, sizeof path, "%s", p2);
    }
    ip = getenv("CUBALC_INCLUDE_PATH");
    if (!f && ip && ip[0]) {
      const char *seg = ip;
      while (*seg && !f) {
        char dir[512], p3[768];
        size_t dlen = 0;
        while (*seg == ':') seg++;
        if (!*seg) break;
        while (seg[dlen] && seg[dlen] != ':' && dlen + 1 < sizeof dir)
          dir[dlen] = seg[dlen], dlen++;
        dir[dlen] = 0;
        seg += dlen;
        if (!dir[0]) continue;
        snprintf(p3, sizeof p3, "%s/%s.cubalc", dir, base);
        f = fopen(p3, "rb");
        if (f) { snprintf(path, sizeof path, "%s", p3); break; }
        snprintf(p3, sizeof p3, "%s/%s", dir, name);
        f = fopen(p3, "rb");
        if (f) { snprintf(path, sizeof path, "%s", p3); break; }
      }
    }
    if (!f) {
      char em[192];
      snprintf(em, sizeof em,
               "%s miss: '%s' — programs/lib · CUBALC_INCLUDE_PATH · cubalc cat",
               want_tail ? "TAILLIB" : "HEADLIB", name);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "LIB_PATH", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) sz = 0;
    if (sz > 256 * 1024) sz = 256 * 1024;
    src = (char *)malloc((size_t)sz + 1);
    if (!src) {
      fclose(f);
      var_set_str(vm, "LAST", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", want_tail ? "TAILLIB: oom" : "HEADLIB: oom");
      var_set_str(vm, "ERR", want_tail ? "TAILLIB: oom" : "HEADLIB: oom");
      bump(vm);
      return 1;
    }
    nr = fread(src, 1, (size_t)sz, f);
    fclose(f);
    src[nr] = 0;
    /* count total lines */
    total = 0;
    lp = src;
    if (nr == 0) {
      total = 0;
    } else {
      while (*lp) {
        while (*lp && *lp != '\n') lp++;
        total++;
        if (*lp == '\n') lp++;
      }
    }
    o = 0;
    kept = 0;
    if (want_tail) {
      long skip = total > nwant ? total - nwant : 0;
      long seen = 0;
      lp = src;
      while (*lp) {
        ls = lp;
        while (*lp && *lp != '\n') lp++;
        llen = (size_t)(lp - ls);
        if (seen >= skip) {
          if (kept > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
          if (o + llen < sizeof bag) {
            memcpy(bag + o, ls, llen);
            o += llen;
          }
          bag[o] = 0;
          kept++;
        }
        seen++;
        if (*lp == '\n') lp++;
      }
    } else {
      lp = src;
      while (*lp && kept < nwant) {
        ls = lp;
        while (*lp && *lp != '\n') lp++;
        llen = (size_t)(lp - ls);
        if (kept > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + llen < sizeof bag) {
          memcpy(bag + o, ls, llen);
          o += llen;
        }
        bag[o] = 0;
        kept++;
        if (*lp == '\n') lp++;
      }
    }
    free(src);
    var_set_str(vm, "LAST", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = kept;
    var_set_num(vm, "LAST_N", kept);
    var_set_str(vm, "LIB_PATH", path);
    var_set_str(vm, "LIB_STEM", base);
    if (want_tail) {
      var_set_str(vm, "TAILLIB", bag);
      var_set_str(vm, "LIBTAIL", bag);
      var_set_num(vm, "TAILLIB_N", kept);
      var_set_num(vm, "TAILLIB_TOTAL", total);
      var_set_num(vm, "TAILLIB_WANT", nwant);
    } else {
      var_set_str(vm, "HEADLIB", bag);
      var_set_str(vm, "LIBHEAD", bag);
      var_set_num(vm, "HEADLIB_N", kept);
      var_set_num(vm, "HEADLIB_TOTAL", total);
      var_set_num(vm, "HEADLIB_WANT", nwant);
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LIBDEPS / INCLUDEDEPS / LIBINCLUDES name — bag of INCLUDE stems from a lib recipe.
   * Usability: agents see composition (what a recipe pulls) without GREPLIB+peel glue.
   * Parses INCLUDE [ONCE] [OR|SOFT] target lines · LAST bag of short stems · soft miss. */
  if (kw(&L->cur, "LIBDEPS") || kw(&L->cur, "INCLUDEDEPS") ||
      kw(&L->cur, "LIBINCLUDES") || kw(&L->cur, "DEPSLIB") ||
      kw(&L->cur, "LIB_DEPS") || kw(&L->cur, "LISTLIBDEPS") ||
      kw(&L->cur, "LIBREQUIRE") || kw(&L->cur, "WHATINCLUDES")) {
    char name[160], path[768], base[160];
    char bag[CUBALC_VAR_STR_MAX];
    char *src = NULL;
    FILE *f = NULL;
    long sz = 0;
    size_t nr = 0, o = 0;
    int ndep = 0;
    const char *slash, *leaf, *ip, *root, *lp;
    size_t blen;
    lex_next(L);
    name[0] = 0;
    path[0] = 0;
    bag[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(name, sizeof name, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(name, sizeof name, "%s", vm->last_str);
      else
        snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
    }
    if (!name[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "LIBDEPS: need name");
      var_set_str(vm, "ERR", "LIBDEPS: need name");
      bump(vm);
      return 1;
    }
    slash = strrchr(name, '/');
    leaf = slash ? slash + 1 : name;
    snprintf(base, sizeof base, "%s", leaf);
    blen = strlen(base);
    if (blen > 7 && strcmp(base + blen - 7, ".cubalc") == 0)
      base[blen - 7] = 0;
    if (name[0] == '/' || strchr(name, '/')) {
      f = fopen(name, "rb");
      if (f) snprintf(path, sizeof path, "%s", name);
    }
    if (!f) {
      char p3[768];
      snprintf(p3, sizeof p3, "programs/lib/%s.cubalc", base);
      f = fopen(p3, "rb");
      if (f) snprintf(path, sizeof path, "%s", p3);
    }
    if (!f) {
      char p3[768];
      snprintf(p3, sizeof p3, "programs/lib/%s", name);
      f = fopen(p3, "rb");
      if (f) snprintf(path, sizeof path, "%s", p3);
    }
    if (!f) {
      f = fopen(name, "rb");
      if (f) snprintf(path, sizeof path, "%s", name);
    }
    root = getenv("CUBALC_ROOT");
    if (!f && root && root[0]) {
      char p2[768];
      snprintf(p2, sizeof p2, "%s/programs/lib/%s.cubalc", root, base);
      f = fopen(p2, "rb");
      if (f) snprintf(path, sizeof path, "%s", p2);
    }
    ip = getenv("CUBALC_INCLUDE_PATH");
    if (!f && ip && ip[0]) {
      const char *seg = ip;
      while (*seg && !f) {
        char dir[512], p3[768];
        size_t dlen = 0;
        while (*seg == ':') seg++;
        if (!*seg) break;
        while (seg[dlen] && seg[dlen] != ':' && dlen + 1 < sizeof dir)
          dir[dlen] = seg[dlen], dlen++;
        dir[dlen] = 0;
        seg += dlen;
        if (!dir[0]) continue;
        snprintf(p3, sizeof p3, "%s/%s.cubalc", dir, base);
        f = fopen(p3, "rb");
        if (f) { snprintf(path, sizeof path, "%s", p3); break; }
        snprintf(p3, sizeof p3, "%s/%s", dir, name);
        f = fopen(p3, "rb");
        if (f) { snprintf(path, sizeof path, "%s", p3); break; }
      }
    }
    if (!f) {
      char em[192];
      snprintf(em, sizeof em,
               "LIBDEPS miss: '%s' — programs/lib · CUBALC_INCLUDE_PATH · cubalc cat",
               name);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "LIB_PATH", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) sz = 0;
    if (sz > 256 * 1024) sz = 256 * 1024;
    src = (char *)malloc((size_t)sz + 1);
    if (!src) {
      fclose(f);
      var_set_str(vm, "LAST", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "LIBDEPS: oom");
      var_set_str(vm, "ERR", "LIBDEPS: oom");
      bump(vm);
      return 1;
    }
    nr = fread(src, 1, (size_t)sz, f);
    fclose(f);
    src[nr] = 0;
    /* scan lines for INCLUDE [ONCE] [OR|SOFT] target */
    lp = src;
    o = 0;
    ndep = 0;
    while (*lp) {
      const char *line = lp;
      char tok[160];
      size_t ti = 0;
      while (*lp && *lp != '\n') lp++;
      /* skip leading space on line */
      while (*line == ' ' || *line == '\t') line++;
      if (*line == '#' || *line == '\n' || *line == 0) {
        if (*lp == '\n') lp++;
        continue;
      }
      /* match INCLUDE as word */
      if (!(line[0] == 'I' && line[1] == 'N' && line[2] == 'C' && line[3] == 'L' &&
            line[4] == 'U' && line[5] == 'D' && line[6] == 'E' &&
            (line[7] == ' ' || line[7] == '\t' || line[7] == '"' || line[7] == 0 ||
             line[7] == '\r' || line[7] == '\n'))) {
        if (*lp == '\n') lp++;
        continue;
      }
      line += 7;
      while (*line == ' ' || *line == '\t') line++;
      /* skip ONCE / OR / SOFT modifiers (repeat) */
      for (;;) {
        if ((line[0] == 'O' && line[1] == 'N' && line[2] == 'C' && line[3] == 'E' &&
             (line[4] == ' ' || line[4] == '\t' || line[4] == 0 || line[4] == '\r' ||
              line[4] == '\n')) ||
            (line[0] == 'O' && line[1] == 'R' &&
             (line[2] == ' ' || line[2] == '\t' || line[2] == 0 || line[2] == '\r' ||
              line[2] == '\n')) ||
            (line[0] == 'S' && line[1] == 'O' && line[2] == 'F' && line[3] == 'T' &&
             (line[4] == ' ' || line[4] == '\t' || line[4] == 0 || line[4] == '\r' ||
              line[4] == '\n'))) {
          if (line[0] == 'O' && line[1] == 'R' &&
              !(line[0] == 'O' && line[1] == 'N')) {
            /* OR is 2 chars — but ONCE starts with O too; already handled ONCE first */
          }
          if (line[0] == 'O' && line[1] == 'N')
            line += 4;
          else if (line[0] == 'S')
            line += 4;
          else
            line += 2; /* OR */
          while (*line == ' ' || *line == '\t') line++;
          continue;
        }
        break;
      }
      /* extract target: "quoted" or bare token */
      tok[0] = 0;
      ti = 0;
      if (*line == '"' || *line == '\'') {
        char q = *line++;
        while (*line && *line != q && *line != '\n' && *line != '\r' && ti + 1 < sizeof tok)
          tok[ti++] = *line++;
        tok[ti] = 0;
      } else {
        while (*line && *line != ' ' && *line != '\t' && *line != '\n' && *line != '\r' &&
               *line != '#' && ti + 1 < sizeof tok)
          tok[ti++] = *line++;
        tok[ti] = 0;
      }
      if (tok[0]) {
        /* stem: leaf without .cubalc */
        const char *sl = strrchr(tok, '/');
        const char *st = sl ? sl + 1 : tok;
        char stem[160];
        size_t slen;
        snprintf(stem, sizeof stem, "%s", st);
        slen = strlen(stem);
        if (slen > 7 && strcmp(stem + slen - 7, ".cubalc") == 0)
          stem[slen - 7] = 0;
        slen = strlen(stem);
        if (slen > 0) {
          if (ndep > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
          if (o + slen < sizeof bag) {
            memcpy(bag + o, stem, slen);
            o += slen;
          }
          bag[o] = 0;
          ndep++;
        }
      }
      if (*lp == '\n') lp++;
    }
    free(src);
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LIBDEPS", bag);
    var_set_str(vm, "INCLUDEDEPS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = ndep;
    var_set_num(vm, "LAST_N", ndep);
    var_set_num(vm, "LIBDEPS_N", ndep);
    var_set_str(vm, "LIB_PATH", path);
    var_set_str(vm, "LIB_STEM", base);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LIBINFO / STATLIB / LIBSTAT name — one-shot lib metadata plate (dual of cubalc cat --meta).
   * Usability: agents get path/bytes/lines/mtime/deps_n without CATLIB or shell.
   * LAST = cubalc.libinfo.v1 JSON · LIB_PATH/STEM/BYTES/LINES/MTIME/DEPS_N · soft miss. */
  if (kw(&L->cur, "LIBINFO") || kw(&L->cur, "STATLIB") ||
      kw(&L->cur, "LIBSTAT") || kw(&L->cur, "INFOLIB") ||
      kw(&L->cur, "LIB_INFO") || kw(&L->cur, "LIBMETA") ||
      kw(&L->cur, "METALIB") || kw(&L->cur, "DESCRIBELIB")) {
    char name[160], path[768], base[160], plate[CUBALC_VAR_STR_MAX];
    char *src = NULL;
    FILE *f = NULL;
    long sz = 0;
    size_t nr = 0;
    int nlines = 0, ndep = 0;
    long mtime = 0;
    const char *slash, *leaf, *ip, *root, *lp;
    size_t blen;
    struct stat st;
    lex_next(L);
    name[0] = 0;
    path[0] = 0;
    plate[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(name, sizeof name, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(name, sizeof name, "%s", vm->last_str);
      else
        snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
    }
    if (!name[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "LIBINFO: need name");
      var_set_str(vm, "ERR", "LIBINFO: need name");
      bump(vm);
      return 1;
    }
    slash = strrchr(name, '/');
    leaf = slash ? slash + 1 : name;
    snprintf(base, sizeof base, "%s", leaf);
    blen = strlen(base);
    if (blen > 7 && strcmp(base + blen - 7, ".cubalc") == 0)
      base[blen - 7] = 0;
    if (name[0] == '/' || strchr(name, '/')) {
      f = fopen(name, "rb");
      if (f) snprintf(path, sizeof path, "%s", name);
    }
    if (!f) {
      char p3[768];
      snprintf(p3, sizeof p3, "programs/lib/%s.cubalc", base);
      f = fopen(p3, "rb");
      if (f) snprintf(path, sizeof path, "%s", p3);
    }
    if (!f) {
      char p3[768];
      snprintf(p3, sizeof p3, "programs/lib/%s", name);
      f = fopen(p3, "rb");
      if (f) snprintf(path, sizeof path, "%s", p3);
    }
    if (!f) {
      f = fopen(name, "rb");
      if (f) snprintf(path, sizeof path, "%s", name);
    }
    root = getenv("CUBALC_ROOT");
    if (!f && root && root[0]) {
      char p2[768];
      snprintf(p2, sizeof p2, "%s/programs/lib/%s.cubalc", root, base);
      f = fopen(p2, "rb");
      if (f) snprintf(path, sizeof path, "%s", p2);
    }
    ip = getenv("CUBALC_INCLUDE_PATH");
    if (!f && ip && ip[0]) {
      const char *seg = ip;
      while (*seg && !f) {
        char dir[512], p3[768];
        size_t dlen = 0;
        while (*seg == ':') seg++;
        if (!*seg) break;
        while (seg[dlen] && seg[dlen] != ':' && dlen + 1 < sizeof dir)
          dir[dlen] = seg[dlen], dlen++;
        dir[dlen] = 0;
        seg += dlen;
        if (!dir[0]) continue;
        snprintf(p3, sizeof p3, "%s/%s.cubalc", dir, base);
        f = fopen(p3, "rb");
        if (f) { snprintf(path, sizeof path, "%s", p3); break; }
        snprintf(p3, sizeof p3, "%s/%s", dir, name);
        f = fopen(p3, "rb");
        if (f) { snprintf(path, sizeof path, "%s", p3); break; }
      }
    }
    if (!f) {
      char em[192];
      snprintf(em, sizeof em,
               "LIBINFO miss: '%s' — programs/lib · CUBALC_INCLUDE_PATH · cubalc which",
               name);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "LIB_PATH", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) sz = 0;
    if (sz > 256 * 1024) sz = 256 * 1024;
    src = (char *)malloc((size_t)sz + 1);
    if (!src) {
      fclose(f);
      var_set_str(vm, "LAST", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "LIBINFO: oom");
      var_set_str(vm, "ERR", "LIBINFO: oom");
      bump(vm);
      return 1;
    }
    nr = fread(src, 1, (size_t)sz, f);
    fclose(f);
    src[nr] = 0;
    mtime = 0;
    if (stat(path, &st) == 0)
      mtime = (long)st.st_mtime;
    /* lines + INCLUDE deps count (same rules as LIBDEPS) */
    nlines = 0;
    ndep = 0;
    lp = src;
    if (nr > 0) {
      while (*lp) {
        const char *line = lp;
        while (*lp && *lp != '\n') lp++;
        nlines++;
        while (*line == ' ' || *line == '\t') line++;
        if (*line != '#' && line[0] == 'I' && line[1] == 'N' && line[2] == 'C' &&
            line[3] == 'L' && line[4] == 'U' && line[5] == 'D' && line[6] == 'E' &&
            (line[7] == ' ' || line[7] == '\t' || line[7] == '"' || line[7] == '\'' ||
             line[7] == 0 || line[7] == '\r' || line[7] == '\n')) {
          /* has a target after modifiers? count as dep if non-empty remainder */
          const char *p = line + 7;
          while (*p == ' ' || *p == '\t') p++;
          for (;;) {
            if (p[0] == 'O' && p[1] == 'N' && p[2] == 'C' && p[3] == 'E' &&
                (p[4] == ' ' || p[4] == '\t' || p[4] == 0 || p[4] == '\r' || p[4] == '\n')) {
              p += 4;
              while (*p == ' ' || *p == '\t') p++;
              continue;
            }
            if (p[0] == 'S' && p[1] == 'O' && p[2] == 'F' && p[3] == 'T' &&
                (p[4] == ' ' || p[4] == '\t' || p[4] == 0 || p[4] == '\r' || p[4] == '\n')) {
              p += 4;
              while (*p == ' ' || *p == '\t') p++;
              continue;
            }
            if (p[0] == 'O' && p[1] == 'R' &&
                (p[2] == ' ' || p[2] == '\t' || p[2] == 0 || p[2] == '\r' || p[2] == '\n')) {
              p += 2;
              while (*p == ' ' || *p == '\t') p++;
              continue;
            }
            break;
          }
          if (*p && *p != '\n' && *p != '\r' && *p != '#')
            ndep++;
        }
        if (*lp == '\n') lp++;
      }
    }
    free(src);
    /* JSON plate — path is filesystem-local; escape " \ minimally */
    {
      char path_esc[900];
      size_t pi = 0, qi = 0;
      for (pi = 0; path[pi] && qi + 2 < sizeof path_esc; pi++) {
        if (path[pi] == '\\' || path[pi] == '"') {
          path_esc[qi++] = '\\';
          path_esc[qi++] = path[pi];
        } else if ((unsigned char)path[pi] >= 32) {
          path_esc[qi++] = path[pi];
        }
      }
      path_esc[qi] = 0;
      snprintf(plate, sizeof plate,
               "{\"schema\":\"cubalc.libinfo.v1\",\"ok\":true,\"stem\":\"%s\","
               "\"path\":\"%s\",\"bytes\":%ld,\"lines\":%d,\"mtime\":%ld,"
               "\"deps_n\":%d,\"version\":\"%s\"}",
               base, path_esc, (long)nr, nlines, mtime, ndep, CUBALC_LANG_VERSION);
    }
    var_set_str(vm, "LAST", plate);
    var_set_str(vm, "LIBINFO", plate);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
    vm->last_n = (long)nr;
    var_set_num(vm, "LAST_N", (long)nr);
    var_set_num(vm, "LIB_BYTES", (long)nr);
    var_set_num(vm, "LIB_SIZE", (long)nr);
    var_set_num(vm, "LIB_LINES", nlines);
    var_set_num(vm, "LIB_MTIME", mtime);
    var_set_num(vm, "LIB_DEPS_N", ndep);
    var_set_str(vm, "LIB_PATH", path);
    var_set_str(vm, "LIB_STEM", base);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LIBTREE / LIBDEPSALL / DEEPTREE name — transitive INCLUDE closure (BFS).
   * Usability: full composition of a recipe without recursive LIBDEPS walks.
   * LAST = bag of unique dep stems (root excluded) · LIBTREE_N · soft miss. */
  if (kw(&L->cur, "LIBTREE") || kw(&L->cur, "LIBDEPSALL") ||
      kw(&L->cur, "DEEPTREE") || kw(&L->cur, "TRANSITIVES") ||
      kw(&L->cur, "LIB_TREE") || kw(&L->cur, "ALLDEPS") ||
      kw(&L->cur, "CLOSUREDEPS") || kw(&L->cur, "WALKDEPS")) {
    char name[160], root_path[768], root_base[160];
    char bag[CUBALC_VAR_STR_MAX];
    char stems[64][96];
    int nstem = 0, qi = 0, i;
    size_t o = 0;
    lex_next(L);
    name[0] = 0;
    root_path[0] = 0;
    bag[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(name, sizeof name, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(name, sizeof name, "%s", vm->last_str);
      else
        snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
    }
    if (!name[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "LIBTREE: need name");
      var_set_str(vm, "ERR", "LIBTREE: need name");
      bump(vm);
      return 1;
    }
    if (!lib_resolve_path(name, root_path, sizeof root_path, root_base, sizeof root_base)) {
      char em[192];
      snprintf(em, sizeof em,
               "LIBTREE miss: '%s' — programs/lib · CUBALC_INCLUDE_PATH · LIBDEPS",
               name);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "LIB_PATH", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
    /* seed queue with direct deps of root (root itself not listed) */
    nstem = 0;
    lib_append_deps(root_path, stems, &nstem, 64);
    qi = 0;
    while (qi < nstem && nstem < 64) {
      char dpath[768], dbase[160];
      if (lib_resolve_path(stems[qi], dpath, sizeof dpath, dbase, sizeof dbase))
        lib_append_deps(dpath, stems, &nstem, 64);
      qi++;
    }
    o = 0;
    for (i = 0; i < nstem; i++) {
      size_t ln = strlen(stems[i]);
      if (i > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + ln < sizeof bag) {
        memcpy(bag + o, stems[i], ln);
        o += ln;
      }
      bag[o] = 0;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LIBTREE", bag);
    var_set_str(vm, "LIBDEPSALL", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = nstem;
    var_set_num(vm, "LAST_N", nstem);
    var_set_num(vm, "LIBTREE_N", nstem);
    var_set_num(vm, "LIBDEPSALL_N", nstem);
    var_set_str(vm, "LIB_PATH", root_path);
    var_set_str(vm, "LIB_STEM", root_base);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LIBDEFAULTS / LIBKNOBS / LIBVARS name — bag of DEFAULT key=value knobs from a lib.
   * Usability: agents see what to set before INCLUDE without CATLIB/HEADLIB parsing.
   * LAST = newline KEY=value fields · LIBDEFAULTS_N · soft miss. */
  if (kw(&L->cur, "LIBDEFAULTS") || kw(&L->cur, "LIBKNOBS") ||
      kw(&L->cur, "LIBVARS") || kw(&L->cur, "DEFAULTSLIB") ||
      kw(&L->cur, "LIB_DEFAULTS") || kw(&L->cur, "KNOBSLIB") ||
      kw(&L->cur, "LISTDEFAULTS") || kw(&L->cur, "RECIPEKNOBS")) {
    char name[160], path[768], base[160];
    char bag[CUBALC_VAR_STR_MAX];
    char *src = NULL;
    FILE *f = NULL;
    long sz = 0;
    size_t nr = 0, o = 0;
    int nknob = 0;
    const char *lp;
    lex_next(L);
    name[0] = 0;
    path[0] = 0;
    bag[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(name, sizeof name, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(name, sizeof name, "%s", vm->last_str);
      else
        snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
    }
    if (!name[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "LIBDEFAULTS: need name");
      var_set_str(vm, "ERR", "LIBDEFAULTS: need name");
      bump(vm);
      return 1;
    }
    if (!lib_resolve_path(name, path, sizeof path, base, sizeof base)) {
      char em[192];
      snprintf(em, sizeof em,
               "LIBDEFAULTS miss: '%s' — programs/lib · CUBALC_INCLUDE_PATH · HEADLIB",
               name);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "LIB_PATH", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
    f = fopen(path, "rb");
    if (!f) {
      char em[192];
      snprintf(em, sizeof em, "LIBDEFAULTS open fail: '%s'", path);
      var_set_str(vm, "LAST", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) sz = 0;
    if (sz > 256 * 1024) sz = 256 * 1024;
    src = (char *)malloc((size_t)sz + 1);
    if (!src) {
      fclose(f);
      var_set_str(vm, "LAST", "");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "LIBDEFAULTS: oom");
      var_set_str(vm, "ERR", "LIBDEFAULTS: oom");
      bump(vm);
      return 1;
    }
    nr = fread(src, 1, (size_t)sz, f);
    fclose(f);
    src[nr] = 0;
    lp = src;
    o = 0;
    nknob = 0;
    while (*lp) {
      const char *line = lp;
      char key[96], val[512], kv[640];
      size_t ki = 0, vi = 0, kvl;
      while (*lp && *lp != '\n') lp++;
      while (*line == ' ' || *line == '\t') line++;
      if (*line == '#' || *line == '\n' || *line == 0) {
        if (*lp == '\n') lp++;
        continue;
      }
      /* DEFAULT key [=] value */
      if (!(line[0] == 'D' && line[1] == 'E' && line[2] == 'F' && line[3] == 'A' &&
            line[4] == 'U' && line[5] == 'L' && line[6] == 'T' &&
            (line[7] == ' ' || line[7] == '\t'))) {
        if (*lp == '\n') lp++;
        continue;
      }
      line += 7;
      while (*line == ' ' || *line == '\t') line++;
      key[0] = 0;
      ki = 0;
      while (*line && *line != ' ' && *line != '\t' && *line != '=' && *line != '\n' &&
             *line != '\r' && ki + 1 < sizeof key)
        key[ki++] = *line++;
      key[ki] = 0;
      while (*line == ' ' || *line == '\t') line++;
      if (*line == '=') {
        line++;
        while (*line == ' ' || *line == '\t') line++;
      }
      val[0] = 0;
      vi = 0;
      if (*line == '"' || *line == '\'') {
        char q = *line++;
        while (*line && *line != q && *line != '\n' && *line != '\r' && vi + 1 < sizeof val)
          val[vi++] = *line++;
        val[vi] = 0;
      } else {
        /* bare value to end-of-line (trim trailing space) */
        while (*line && *line != '\n' && *line != '\r' && *line != '#' && vi + 1 < sizeof val)
          val[vi++] = *line++;
        val[vi] = 0;
        while (vi > 0 && (val[vi - 1] == ' ' || val[vi - 1] == '\t'))
          val[--vi] = 0;
      }
      if (key[0]) {
        snprintf(kv, sizeof kv, "%s=%s", key, val);
        kvl = strlen(kv);
        if (nknob > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + kvl < sizeof bag) {
          memcpy(bag + o, kv, kvl);
          o += kvl;
        }
        bag[o] = 0;
        nknob++;
      }
      if (*lp == '\n') lp++;
    }
    free(src);
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LIBDEFAULTS", bag);
    var_set_str(vm, "LIBKNOBS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = nknob;
    var_set_num(vm, "LAST_N", nknob);
    var_set_num(vm, "LIBDEFAULTS_N", nknob);
    var_set_num(vm, "LIBKNOBS_N", nknob);
    var_set_str(vm, "LIB_PATH", path);
    var_set_str(vm, "LIB_STEM", base);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LISTPRELOAD / PRELOADS — short-name bag of effective -I / CUBALC_PRELOAD.
   * Usability: CLI sets CUBALC_PRELOAD_ACTIVE; programs audit request vs INCLUDESTEMS. */
  if (kw(&L->cur, "LISTPRELOAD") || kw(&L->cur, "PRELOADS") ||
      kw(&L->cur, "PRELOADLIST") || kw(&L->cur, "LIST_PRELOAD") ||
      kw(&L->cur, "PRELOAD_NAMES") || kw(&L->cur, "PRELOADED")) {
    const char *env = getenv("CUBALC_PRELOAD_ACTIVE");
    char bag[1024];
    size_t o = 0;
    int n = 0;
    lex_next(L);
    if (!env || !env[0])
      env = getenv("CUBALC_PRELOAD");
    bag[0] = 0;
    if (env && env[0]) {
      const char *p = env;
      while (*p) {
        char name[96];
        size_t len = 0;
        while (*p == ':' || *p == ',' || *p == ' ' || *p == '\t') p++;
        if (!*p) break;
        while (p[len] && p[len] != ':' && p[len] != ',' && p[len] != ' ' &&
               p[len] != '\t' && len + 1 < sizeof name) {
          name[len] = p[len];
          len++;
        }
        name[len] = 0;
        p += len;
        if (!name[0]) continue;
        if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
        if (o + len < sizeof bag) {
          memcpy(bag + o, name, len);
          o += len;
        }
        bag[o] = 0;
        n++;
      }
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "LISTPRELOAD", bag);
    var_set_str(vm, "PRELOADS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "PRELOAD_N", n);
    var_set_num(vm, "LISTPRELOAD_N", n);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* PRELOADMISS / PRELOADOK / NEEDPRELOAD — audit CLI -I/CUBALC_PRELOAD vs loaded.
   * Usability: request bag from CUBALC_PRELOAD_ACTIVE; miss = not in include_loaded.
   * Soft PRELOADMISS → LAST=miss bag LAST_N=count; PRELOADOK → 0|1; NEEDPRELOAD hard. */
  if (kw(&L->cur, "PRELOADMISS") || kw(&L->cur, "MISSINGPRELOAD") ||
      kw(&L->cur, "PRELOAD_MISS") || kw(&L->cur, "CHECKPRELOAD") ||
      kw(&L->cur, "PRELOADOK") || kw(&L->cur, "HASPRELOADALL") ||
      kw(&L->cur, "PRELOAD_OK") || kw(&L->cur, "ALLPRELOADED") ||
      kw(&L->cur, "NEEDPRELOAD") || kw(&L->cur, "REQUIREPRELOAD") ||
      kw(&L->cur, "NEED_PRELOAD") || kw(&L->cur, "REQUIRE_PRELOAD")) {
    int hard = kw(&L->cur, "NEEDPRELOAD") || kw(&L->cur, "REQUIREPRELOAD") ||
               kw(&L->cur, "NEED_PRELOAD") || kw(&L->cur, "REQUIRE_PRELOAD");
    int as_ok = kw(&L->cur, "PRELOADOK") || kw(&L->cur, "HASPRELOADALL") ||
                kw(&L->cur, "PRELOAD_OK") || kw(&L->cur, "ALLPRELOADED");
    int aln = L->cur.line;
    const char *env = getenv("CUBALC_PRELOAD_ACTIVE");
    char miss[512];
    size_t mo = 0;
    int n_need = 0, n_hit = 0, n_miss = 0;
    miss[0] = 0;
    lex_next(L);
    if (!env || !env[0])
      env = getenv("CUBALC_PRELOAD");
    if (env && env[0]) {
      const char *p = env;
      while (*p) {
        char name[96], hitp[256];
        size_t len = 0;
        while (*p == ':' || *p == ',' || *p == ' ' || *p == '\t') p++;
        if (!*p) break;
        while (p[len] && p[len] != ':' && p[len] != ',' && p[len] != ' ' &&
               p[len] != '\t' && len + 1 < sizeof name) {
          name[len] = p[len];
          len++;
        }
        name[len] = 0;
        p += len;
        if (!name[0]) continue;
        n_need++;
        if (include_loaded_match(vm, name, hitp, sizeof hitp)) {
          n_hit++;
        } else {
          if (n_miss > 0 && mo + 1 < sizeof miss) miss[mo++] = '\n';
          if (mo + len < sizeof miss) {
            memcpy(miss + mo, name, len);
            mo += len;
          }
          miss[mo] = 0;
          n_miss++;
        }
      }
    }
    var_set_num(vm, "PRELOAD_N", n_need);
    var_set_num(vm, "PRELOAD_HIT_N", n_hit);
    var_set_num(vm, "PRELOAD_MISS_N", n_miss);
    var_set_str(vm, "PRELOAD_MISS", miss);
    var_set_num(vm, "INCLUDE_N", vm->n_included);

    if (hard) {
      if (n_miss == 0) {
        var_set_num(vm, "LAST_N", 1);
        vm->last_n = 1;
        var_set_str(vm, "LAST", "1");
        snprintf(vm->last_str, sizeof vm->last_str, "1");
        var_set_num(vm, "PRELOAD_OK_N", 1);
        var_set_num(vm, "OK", 1);
        if (vm->res) vm->res->asserts_ok++;
        if (vm->trace)
          fprintf(vm->trace, "# needpreload ok need=%d\n", n_need);
        bump(vm);
        return 1;
      }
      {
        char msg[240];
        char flat[160];
        size_t fi = 0;
        const char *mp;
        flat[0] = 0;
        for (mp = miss; *mp && fi + 1 < sizeof flat; mp++) {
          if (*mp == '\n') {
            if (fi > 0 && flat[fi - 1] != ',') {
              flat[fi++] = ',';
              if (fi + 1 < sizeof flat) flat[fi++] = ' ';
            }
          } else {
            flat[fi++] = *mp;
          }
        }
        flat[fi] = 0;
        snprintf(msg, sizeof msg,
                 "NEEDPRELOAD missing line %d: %s — cubalc run -I name · "
                 "INCLUDE / LISTPRELOAD vs INCLUDESTEMS",
                 aln, flat[0] ? flat : "?");
        var_set_str(vm, "ERR", msg);
        var_set_str(vm, "LAST_ERR", msg);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
    }

    /* Soft: PRELOADOK → 0|1; PRELOADMISS/CHECKPRELOAD → miss bag + count */
    if (as_ok) {
      int ok = (n_miss == 0);
      var_set_num(vm, "LAST_N", ok);
      vm->last_n = ok;
      if (ok) {
        var_set_str(vm, "LAST", "1");
        snprintf(vm->last_str, sizeof vm->last_str, "1");
        var_set_num(vm, "OK", 1);
      } else {
        var_set_str(vm, "LAST", miss);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", miss);
        var_set_num(vm, "OK", 0);
        {
          char em[192];
          snprintf(em, sizeof em,
                   "PRELOADOK miss line %d: %s — cubalc run -I / INCLUDE",
                   aln, miss);
          var_set_str(vm, "ERR", em);
          var_set_str(vm, "LAST_ERR", em);
        }
      }
      var_set_num(vm, "PRELOAD_OK_N", ok);
      bump(vm);
      return 1;
    }

    /* PRELOADMISS / CHECKPRELOAD — always OK=1 audit; LAST_N = miss count */
    var_set_str(vm, "LAST", miss);
    var_set_str(vm, "PRELOADMISS", miss);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", miss);
    vm->last_n = n_miss;
    var_set_num(vm, "LAST_N", n_miss);
    var_set_num(vm, "PRELOAD_OK_N", n_miss == 0 ? 1 : 0);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASPRELOAD name — soft 0|1 if name is in -I / CUBALC_PRELOAD request bag.
   * Dual of HASINCLUDE (loaded) · IF branch on launch contract without LISTPRELOAD+HASLINE. */
  if (kw(&L->cur, "HASPRELOAD") || kw(&L->cur, "HAVEPRELOAD") ||
      kw(&L->cur, "ISPRELOAD") || kw(&L->cur, "PRELOADHAS") ||
      kw(&L->cur, "INPRELOAD")) {
    char want[256], wstem[96];
    const char *env;
    int hit = 0, n_req = 0;
    size_t wlen;
    lex_next(L);
    if (kw(&L->cur, "LIB") || kw(&L->cur, "OF") || kw(&L->cur, "MODULE") ||
        kw(&L->cur, "NAME"))
      lex_next(L);
    if (!include_parse_want(vm, L, want, sizeof want)) {
      fail(vm, "HASPRELOAD name");
      return -1;
    }
    {
      const char *slash = strrchr(want, '/');
      snprintf(wstem, sizeof wstem, "%s", slash ? slash + 1 : want);
    }
    wlen = strlen(wstem);
    if (wlen > 7 && strcmp(wstem + wlen - 7, ".cubalc") == 0)
      wstem[wlen - 7] = 0;
    env = getenv("CUBALC_PRELOAD_ACTIVE");
    if (!env || !env[0])
      env = getenv("CUBALC_PRELOAD");
    if (env && env[0] && wstem[0]) {
      const char *p = env;
      while (*p) {
        char name[96], nstem[96];
        size_t len = 0, nlen;
        const char *slash;
        while (*p == ':' || *p == ',' || *p == ' ' || *p == '\t') p++;
        if (!*p) break;
        while (p[len] && p[len] != ':' && p[len] != ',' && p[len] != ' ' &&
               p[len] != '\t' && len + 1 < sizeof name) {
          name[len] = p[len];
          len++;
        }
        name[len] = 0;
        p += len;
        if (!name[0]) continue;
        n_req++;
        slash = strrchr(name, '/');
        snprintf(nstem, sizeof nstem, "%s", slash ? slash + 1 : name);
        nlen = strlen(nstem);
        if (nlen > 7 && strcmp(nstem + nlen - 7, ".cubalc") == 0)
          nstem[nlen - 7] = 0;
        if (strcmp(nstem, wstem) == 0 || strcmp(name, want) == 0 ||
            strcmp(nstem, want) == 0) {
          hit = 1;
          break;
        }
      }
    }
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    {
      char nb[8];
      snprintf(nb, sizeof nb, "%d", hit);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "HASPRELOAD_N", hit);
    var_set_num(vm, "PRELOAD_N", n_req);
    var_set_str(vm, "PRELOAD_WANT", want);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* INCLUDESTEMS / LISTINCLUDESTEMS — basenames without .cubalc from loaded modules.
   * Usability: short-name bag for HASLINE/NEEDINCLUDE glue after -I without BASENAMEALL. */
  if (kw(&L->cur, "INCLUDESTEMS") || kw(&L->cur, "LISTINCLUDESTEMS") ||
      kw(&L->cur, "INCLUDE_NAMES") || kw(&L->cur, "LOADEDSTEMS") ||
      kw(&L->cur, "INCLUDEBASENAMES") || kw(&L->cur, "STEMSINCLUDES")) {
    char bag[4096];
    size_t o = 0;
    int i, n = 0;
    lex_next(L);
    bag[0] = 0;
    for (i = 0; i < vm->n_included; i++) {
      const char *p = vm->included[i];
      const char *b = strrchr(p, '/');
      char stem[256];
      size_t sl;
      b = b ? b + 1 : p;
      snprintf(stem, sizeof stem, "%s", b);
      sl = strlen(stem);
      if (sl > 7 && strcmp(stem + sl - 7, ".cubalc") == 0)
        stem[sl - 7] = 0, sl -= 7;
      if (n > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
      if (o + sl < sizeof bag) {
        memcpy(bag + o, stem, sl);
        o += sl;
      }
      bag[o] = 0;
      n++;
    }
    var_set_str(vm, "LAST", bag);
    var_set_str(vm, "INCLUDESTEMS", bag);
    var_set_str(vm, "LOADEDSTEMS", bag);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = n;
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "INCLUDE_N", n);
    var_set_num(vm, "INCLUDESTEMS_N", n);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASINCLUDE name|path — soft 0|1 if a loaded INCLUDE matches stem or path.
   * Complements LISTINCLUDES · IF without bag GREP after -I preload. */
  if (kw(&L->cur, "HASINCLUDE") || kw(&L->cur, "HAVEINCLUDE") ||
      kw(&L->cur, "HASLOADED") || kw(&L->cur, "ISINCLUDED") ||
      kw(&L->cur, "INCLUDELOADED")) {
    char want[256], hitp[256];
    int hit;
    lex_next(L);
    if (kw(&L->cur, "LIB") || kw(&L->cur, "OF") || kw(&L->cur, "MODULE"))
      lex_next(L);
    if (!include_parse_want(vm, L, want, sizeof want)) {
      fail(vm, "HASINCLUDE name|path");
      return -1;
    }
    hit = include_loaded_match(vm, want, hitp, sizeof hitp);
    if (hit)
      var_set_str(vm, "INCLUDE_PATH", hitp);
    else
      var_set_str(vm, "INCLUDE_PATH", "");
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    {
      char nb[8];
      snprintf(nb, sizeof nb, "%d", hit);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "HASINCLUDE_N", hit);
    var_set_num(vm, "INCLUDE_N", vm->n_included);
    var_set_str(vm, "INCLUDE_MISS", hit ? "" : want);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASINCLUDEALL a b c — soft 0|1 all named modules loaded · miss bag.
   * NEEDINCLUDE / REQUIRE LOADED a b — fail-fast if any not loaded this run.
   * Usability: agent boot contracts after -I / INCLUDE without multi HASINCLUDE. */
  if (kw(&L->cur, "HASINCLUDEALL") || kw(&L->cur, "HASINCLUDES") ||
      kw(&L->cur, "ALLINCLUDED") || kw(&L->cur, "NEEDINCLUDE") ||
      kw(&L->cur, "NEEDINCLUDES") || kw(&L->cur, "REQUIRELOADED") ||
      kw(&L->cur, "NEEDLOADED") || kw(&L->cur, "REQUIRE_INCLUDED")) {
    int hard = kw(&L->cur, "NEEDINCLUDE") || kw(&L->cur, "NEEDINCLUDES") ||
               kw(&L->cur, "REQUIRELOADED") || kw(&L->cur, "NEEDLOADED") ||
               kw(&L->cur, "REQUIRE_INCLUDED");
    int aln = L->cur.line;
    char miss[512];
    size_t mo = 0;
    int n_need = 0, n_hit = 0, n_miss = 0;
    char last_hit[256];
    last_hit[0] = 0;
    miss[0] = 0;
    lex_next(L);
    if (kw(&L->cur, "LIB") || kw(&L->cur, "OF") || kw(&L->cur, "MODULE") ||
        kw(&L->cur, "LOADED") || kw(&L->cur, "INCLUDE") || kw(&L->cur, "INCLUDES"))
      lex_next(L);
    while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
      char want[256], hitp[256];
      /* stop words that start next form */
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur, "ASSERT") || kw(&L->cur, "EXPECT") ||
           kw(&L->cur, "IF") || kw(&L->cur, "LET") ||
           kw(&L->cur, "INCLUDE") || kw(&L->cur, "PASS") ||
           kw(&L->cur, "FAIL") || kw(&L->cur, "PRINT") ||
           kw(&L->cur, "SYS") || kw(&L->cur, "LISTINCLUDES") ||
           kw(&L->cur, "HASINCLUDE") || kw(&L->cur, "NEEDINCLUDE") ||
           kw(&L->cur, "HASINCLUDEALL") || kw(&L->cur, "REQUIRE") ||
           kw(&L->cur, "END") || kw(&L->cur, "ELSE") ||
           kw(&L->cur, "WHILE") || kw(&L->cur, "FOR") ||
           kw(&L->cur, "FN") || kw(&L->cur, "CLASS") ||
           kw(&L->cur, "HOLD_FLASH") || kw(&L->cur, "VERSION") ||
           kw(&L->cur, "STATUS") || kw(&L->cur, "WHY") ||
           kw(&L->cur, "NOTE") || kw(&L->cur, "EXIT") ||
           kw(&L->cur, "CLEAR_ERR") || kw(&L->cur, "DEFAULT")))
        break;
      if (!include_parse_want(vm, L, want, sizeof want)) break;
      if (!want[0]) continue;
      n_need++;
      if (include_loaded_match(vm, want, hitp, sizeof hitp)) {
        n_hit++;
        snprintf(last_hit, sizeof last_hit, "%s", hitp);
      } else {
        size_t wl = strlen(want);
        if (n_miss > 0 && mo + 1 < sizeof miss) miss[mo++] = '\n';
        if (mo + wl < sizeof miss) {
          memcpy(miss + mo, want, wl);
          mo += wl;
        }
        miss[mo] = 0;
        n_miss++;
      }
      /* optional commas between names */
      if (L->cur.kind == TK_COMMA) lex_next(L);
    }
    if (n_need == 0) {
      fail_at(vm, L, hard ? "NEEDINCLUDE name…" : "HASINCLUDEALL name…");
      return -1;
    }
    var_set_num(vm, "INCLUDE_N", vm->n_included);
    var_set_num(vm, "INCLUDE_NEED_N", n_need);
    var_set_num(vm, "INCLUDE_HIT_N", n_hit);
    var_set_num(vm, "INCLUDE_MISS_N", n_miss);
    var_set_str(vm, "INCLUDE_MISS", miss);
    if (n_miss == 0) {
      var_set_str(vm, "INCLUDE_PATH", last_hit);
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_str(vm, "LAST", last_hit[0] ? last_hit : "1");
      snprintf(vm->last_str, sizeof vm->last_str, "%s",
               last_hit[0] ? last_hit : "1");
      var_set_num(vm, "OK", 1);
      var_set_num(vm, "HASINCLUDEALL_N", 1);
      if (vm->res) vm->res->asserts_ok++;
      if (vm->trace)
        fprintf(vm->trace, "# %s hit need=%d\n",
                hard ? "needinclude" : "hasincludeall", n_need);
      bump(vm);
      return 1;
    }
    /* soft miss */
    if (!hard) {
      var_set_str(vm, "INCLUDE_PATH", "");
      var_set_str(vm, "LAST", miss);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", miss);
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "HASINCLUDEALL_N", 0);
      var_set_num(vm, "OK", 0);
      {
        char em[192];
        snprintf(em, sizeof em,
                 "HASINCLUDEALL miss line %d: %s — INCLUDE / cubalc run -I",
                 aln, miss);
        var_set_str(vm, "ERR", em);
        var_set_str(vm, "LAST_ERR", em);
      }
      bump(vm);
      return 1;
    }
    /* hard fail */
    {
      char msg[240];
      char flat[160];
      size_t fi = 0;
      const char *mp;
      flat[0] = 0;
      for (mp = miss; *mp && fi + 1 < sizeof flat; mp++) {
        if (*mp == '\n') {
          if (fi > 0 && flat[fi - 1] != ',') {
            flat[fi++] = ',';
            if (fi + 1 < sizeof flat) flat[fi++] = ' ';
          }
        } else {
          flat[fi++] = *mp;
        }
      }
      flat[fi] = 0;
      snprintf(msg, sizeof msg,
               "NEEDINCLUDE missing line %d: %s — INCLUDE / cubalc run -I · HASINCLUDE",
               aln, flat[0] ? flat : "?");
      if (vm->res) vm->res->asserts_fail++;
      fail(vm, msg);
      return -1;
    }
  }

  /* FN name ... END — reusable practical blocks */
  return 0;
}
