/* CubalC lang — lang_ops_cell.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"
#include "lang/cubalc_lang_ops_cell_nibble4.h"
#include "lang/cubalc_lang_ops_cell_width8.h"
#include "lang/cubalc_lang_ops_cell_bitwise_w.h"
#include "lang/cubalc_lang_ops_cell_w16rot.h"
#include "lang/cubalc_lang_ops_cell_shift32.h"
#include "lang/cubalc_lang_ops_cell_rot32.h"
#include "lang/cubalc_lang_ops_cell_metrics.h"
#include "lang/cubalc_lang_ops_cell_rng.h"
#include "lang/cubalc_lang_ops_cell_energy.h"
#include "lang/cubalc_lang_ops_cell_matbits.h"
#include "lang/cubalc_lang_ops_cell_matscan.h"
#include "lang/cubalc_lang_ops_cell_matmetric.h"
#include "lang/cubalc_lang_ops_cell_matruns.h"
#include "lang/cubalc_lang_ops_cell_matfind.h"
#include "lang/cubalc_lang_ops_cell_matrange.h"
#include "lang/cubalc_lang_ops_cell_matword.h"
#include "lang/cubalc_lang_ops_cell_matpath.h"
#include "lang/cubalc_lang_ops_cell_matfill.h"
#include "lang/cubalc_lang_ops_cell_matalgebra.h"
#include "lang/cubalc_lang_ops_cell_matrel.h"
#include "lang/cubalc_lang_ops_cell_matmux.h"
#include "lang/cubalc_lang_ops_cell_matbrain.h"
#include "lang/cubalc_lang_ops_cell_energyflow.h"
#include "lang/cubalc_lang_ops_cell_runtime.h"
#include "lang/cubalc_lang_ops_cell_include_query.h"
#include "lang/cubalc_lang_ops_cell_include.h"
#include "lang/cubalc_lang_ops_cell_preload.h"
#include "lang/cubalc_lang_ops_cell_matlogic.h"
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/* INCLUDE lib did-you-mean — scan programs/lib for closest stem. */

/* Stem/path match against vm->included[] (HASINCLUDE / NEEDINCLUDE). */

/* Parse one name|path token for INCLUDE probes (IDENT/STR/string-var/LAST). */

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
  /* nibble4 peel (AND4CELL…CLIPS4CELL) */
  {
    int n4 = cubalc_lang_ops_cell_try_nibble4(vm, L);
    if (n4 != 0) return n4;
  }
  /* width8 peel (SHL8CELL…CLIP8CELL) */
  {
    int w8 = cubalc_lang_ops_cell_try_width8(vm, L);
    if (w8 != 0) return w8;
  }
  /* bitwise_w peel (AND8CELL…NOT32CELL) */
  {
    int bw = cubalc_lang_ops_cell_try_bitwise_w(vm, L);
    if (bw != 0) return bw;
  }
  /* w16rot peel (BSWAP64…SANY/SFIND/SSCAN + fold/logic plane) */
  {
    int w16 = cubalc_lang_ops_cell_try_w16rot(vm, L);
    if (w16 != 0) return w16;
  }
  /* shift32 peel (SHL32CELL…SAR32CELL) */
  {
    int s32 = cubalc_lang_ops_cell_try_shift32(vm, L);
    if (s32 != 0) return s32;
  }
  /* rot32 peel (ROL32CELL…BITREV32CELL) */
  {
    int r32 = cubalc_lang_ops_cell_try_rot32(vm, L);
    if (r32 != 0) return r32;
  }
  /* metrics peel (PARITYCELL…BSWAP16CELL) */
  {
    int met = cubalc_lang_ops_cell_try_metrics(vm, L);
    if (met != 0) return met;
  }
  /* rng peel (RAND·SEED·SRAND·SHUFFLE·RANDBITS) */
  {
    int rng = cubalc_lang_ops_cell_try_rng(vm, L);
    if (rng != 0) return rng;
  }
  /* shift32/rot32/metrics/w16rot monolit bodies removed — try_* handles first (G_cell_margin) */

  /* energy peel (ENERGYGET·SET·ADD·SUB·XFER·CLAMP·SWAP·SHARE·fleet·SEFLOW) */
  {
    int en = cubalc_lang_ops_cell_try_energy(vm, L);
    if (en != 0) return en;
  }
  /* matbits peel (ROTBITS·SHL/SHR·SETBIT·COUNTBITS·GETBIT) */
  {
    int mb = cubalc_lang_ops_cell_try_matbits(vm, L);
    if (mb != 0) return mb;
  }
  /* matscan peel (FINDONE·REVBITS·SWAPBIT·DIFFBITS) */
  {
    int ms = cubalc_lang_ops_cell_try_matscan(vm, L);
    if (ms != 0) return ms;
  }
  /* matmetric peel (DOTBITS·JACCARD·MAJ·GRAY·CLZ/CTZ·ORPOP) */
  {
    int mm = cubalc_lang_ops_cell_try_matmetric(vm, L);
    if (mm != 0) return mm;
  }
  /* matruns peel (TRANSBITS·RUNSBITS·MASKPOP) */
  {
    int mr = cubalc_lang_ops_cell_try_matruns(vm, L);
    if (mr != 0) return mr;
  }
  /* matfind peel (MAXRUN·ZRUNS·FINDZERO) */
  {
    int mf = cubalc_lang_ops_cell_try_matfind(vm, L);
    if (mf != 0) return mf;
  }
  /* matrange peel (PARITYBITS·COPYRANGE·SWAPRANGE·ROTRANGE) */
  {
    int mrg = cubalc_lang_ops_cell_try_matrange(vm, L);
    if (mrg != 0) return mrg;
  }
  /* matword peel (WORDFROM·WORDTO·EXTRACTBITS·DEPOSITBITS) */
  {
    int mw = cubalc_lang_ops_cell_try_matword(vm, L);
    if (mw != 0) return mw;
  }
  /* matpath peel (PEXTBITS·PDEPBITS·ZIPBITS·UNZIPBITS) */
  {
    int mp = cubalc_lang_ops_cell_try_matpath(vm, L);
    if (mp != 0) return mp;
  }
  /* matfill peel (FILLRANGE·CLEARRANGE·FLIPRANGE·COUNTRANGE) */
  {
    int mf = cubalc_lang_ops_cell_try_matfill(vm, L);
    if (mf != 0) return mf;
  }
  /* matalgebra peel (ANDRANGE·ORRANGE·XORRANGE) */
  {
    int ma = cubalc_lang_ops_cell_try_matalgebra(vm, L);
    if (ma != 0) return ma;
  }
  /* matrel peel (EQBITS·NEBITS·SUBSETBITS·SUPERSETBITS·DISJOINTBITS) */
  {
    int mr = cubalc_lang_ops_cell_try_matrel(vm, L);
    if (mr != 0) return mr;
  }
  /* matmux peel (MUXBITS·BLENDBITS·MATCHBITS) */
  {
    int mm = cubalc_lang_ops_cell_try_matmux(vm, L);
    if (mm != 0) return mm;
  }
  /* matbrain peel (SETDIGIT·FOLDBITS·EEG*) */
  {
    int mb = cubalc_lang_ops_cell_try_matbrain(vm, L);
    if (mb != 0) return mb;
  }
  /* energyflow peel (COMPARE·HARMONY·RESOLVE·ENERGYFLOW·FLOWI) */
  {
    int ef = cubalc_lang_ops_cell_try_energyflow(vm, L);
    if (ef != 0) return ef;
  }
  /* runtime peel (DECIDE·VIZ·SPIN·WAIT) */
  {
    int rt = cubalc_lang_ops_cell_try_runtime(vm, L);
    if (rt != 0) return rt;
  }
  /* include query peel (INCLUDESTEMS·HASINCLUDE·NEEDINCLUDE) */
  {
    int iq = cubalc_lang_ops_cell_try_include_query(vm, L);
    if (iq != 0) return iq;
  }
  /* preload query peel (LISTPRELOAD·PRELOADMISS·HASPRELOAD) */
  {
    int pl = cubalc_lang_ops_cell_try_preload(vm, L);
    if (pl != 0) return pl;
  }
  /* matlogic peel (CLEARBITS·NOTBITS·ANDBITS·ORBITS·XORBITS·…) */
  {
    int ml = cubalc_lang_ops_cell_try_matlogic(vm, L);
    if (ml != 0) return ml;
  }
  /* INCLUDE monolit → lang_ops_cell_include.c (G_cell_margin peel) */
  {
    int inc = cubalc_lang_ops_cell_try_include(vm, L);
    if (inc != 0) return inc;
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

  /* MATCHLIBS / FILTERLIBS / LIBMATCH needle — bag of lib stems matching filter.
   * Usability: in-lang dual of cubalc libs [filter] — agents pick INCLUDE targets
   * without shell · case-insensitive stem/path substring · soft empty bag. */
  if (kw(&L->cur, "MATCHLIBS") || kw(&L->cur, "FILTERLIBS") ||
      kw(&L->cur, "LIBMATCH") || kw(&L->cur, "LIBSFILTER") ||
      kw(&L->cur, "GREPSTEMS") || kw(&L->cur, "FINDLIBS") ||
      kw(&L->cur, "MATCH_LIBS") || kw(&L->cur, "FILTER_LIBS")) {
    char needle[96], fup[96], bag[4096];
    char stems[96][96];
    int nstem = 0, nmatch = 0, i, j;
    size_t o = 0, a;
    DIR *d;
    struct dirent *ent;
    const char *ip;
    lex_next(L);
    needle[0] = 0;
    bag[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(needle, sizeof needle, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(needle, sizeof needle, "%s", vm->last_str);
      else
        snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    }
    if (!needle[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "MATCHLIBS_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "MATCHLIBS: need needle — MATCHLIBS plate");
      var_set_str(vm, "ERR", "MATCHLIBS: need needle — MATCHLIBS plate");
      bump(vm);
      return 1;
    }
    for (a = 0; needle[a] && a + 1 < sizeof fup; a++) {
      char c = needle[a];
      if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
      fup[a] = c;
    }
    fup[a] = 0;
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
          char stem[96], hay[192];
          int dup = 0;
          size_t b;
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
          /* case-insensitive stem + full path haystack (dual of cubalc libs filter) */
          snprintf(hay, sizeof hay, "%s/%s", dirs[di], ent->d_name);
          for (b = 0; hay[b]; b++)
            if (hay[b] >= 'A' && hay[b] <= 'Z')
              hay[b] = (char)(hay[b] - 'A' + 'a');
          {
            char stem_l[96];
            for (b = 0; stem[b] && b + 1 < sizeof stem_l; b++) {
              char c = stem[b];
              if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
              stem_l[b] = c;
            }
            stem_l[b] = 0;
            if (!strstr(stem_l, fup) && !strstr(hay, fup))
              continue;
          }
          snprintf(stems[nstem++], sizeof stems[0], "%s", stem);
        }
        closedir(d);
      }
    }
    /* sort */
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
    nmatch = nstem;
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
    var_set_str(vm, "MATCHLIBS", bag);
    var_set_str(vm, "FILTERLIBS", bag);
    var_set_str(vm, "MATCHLIBS_FILTER", needle);
    var_set_str(vm, "LIBS_FILTER", needle);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = nmatch;
    var_set_num(vm, "LAST_N", nmatch);
    var_set_num(vm, "MATCHLIBS_N", nmatch);
    var_set_num(vm, "FILTERLIBS_N", nmatch);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* COUNTMATCHLIBS|COUNTLIBS needle — match count → LAST_N without bag materialize.
   * Usability: IF/gates on how many libs match without MATCHLIBS clobbering LAST.
   * Completes bag · has · need · count quartet. Soft empty needle → OK=0. */
  if (kw(&L->cur, "COUNTMATCHLIBS") || kw(&L->cur, "COUNTLIBS") ||
      kw(&L->cur, "NMATCHLIBS") || kw(&L->cur, "MATCHLIBSCOUNT") ||
      kw(&L->cur, "LIBCOUNT") || kw(&L->cur, "COUNT_MATCHLIBS") ||
      kw(&L->cur, "COUNTFILTERLIBS")) {
    char needle[96], fup[96];
    int nmatch = 0, j;
    size_t a;
    DIR *d;
    struct dirent *ent;
    const char *ip;
    char stems[96][96];
    int nstem = 0;
    lex_next(L);
    needle[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(needle, sizeof needle, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(needle, sizeof needle, "%s", vm->last_str);
      else
        snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    }
    if (!needle[0]) {
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "MATCHLIBS_N", 0);
      var_set_num(vm, "COUNTMATCHLIBS_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "COUNTMATCHLIBS: need needle — COUNTMATCHLIBS plate");
      var_set_str(vm, "ERR", "COUNTMATCHLIBS: need needle — COUNTMATCHLIBS plate");
      bump(vm);
      return 1;
    }
    for (a = 0; needle[a] && a + 1 < sizeof fup; a++) {
      char c = needle[a];
      if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
      fup[a] = c;
    }
    fup[a] = 0;
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
          char stem[96], hay[192];
          int dup = 0;
          size_t b;
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
          snprintf(hay, sizeof hay, "%s/%s", dirs[di], ent->d_name);
          for (b = 0; hay[b]; b++)
            if (hay[b] >= 'A' && hay[b] <= 'Z')
              hay[b] = (char)(hay[b] - 'A' + 'a');
          {
            char stem_l[96];
            for (b = 0; stem[b] && b + 1 < sizeof stem_l; b++) {
              char c = stem[b];
              if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
              stem_l[b] = c;
            }
            stem_l[b] = 0;
            if (!strstr(stem_l, fup) && !strstr(hay, fup))
              continue;
          }
          snprintf(stems[nstem++], sizeof stems[0], "%s", stem);
        }
        closedir(d);
      }
    }
    nmatch = nstem;
    {
      char nb[16];
      snprintf(nb, sizeof nb, "%d", nmatch);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    vm->last_n = nmatch;
    var_set_num(vm, "LAST_N", nmatch);
    var_set_num(vm, "MATCHLIBS_N", nmatch);
    var_set_num(vm, "COUNTMATCHLIBS_N", nmatch);
    var_set_str(vm, "MATCHLIBS_FILTER", needle);
    var_set_str(vm, "COUNTMATCHLIBS_FILTER", needle);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* HASCOUNTMATCHLIBS|NEEDCOUNTMATCHLIBS min needle — soft 0|1 / fail if count < min.
   * Usability: install gates “need ≥2 fat libs” without COUNTMATCHLIBS+IF compare glue.
   * Completes count · soft min · hard min triad. */
  if (kw(&L->cur, "HASCOUNTMATCHLIBS") || kw(&L->cur, "NEEDCOUNTMATCHLIBS") ||
      kw(&L->cur, "HASCOUNTLIBS") || kw(&L->cur, "NEEDCOUNTLIBS") ||
      kw(&L->cur, "REQUIRECOUNTLIBS") || kw(&L->cur, "REQUIRECOUNTMATCHLIBS") ||
      kw(&L->cur, "HAS_COUNTMATCHLIBS") || kw(&L->cur, "NEED_COUNTMATCHLIBS") ||
      kw(&L->cur, "MINMATCHLIBS") || kw(&L->cur, "NEEDMINLIBS")) {
    char needle[96], fup[96];
    int nmatch = 0, j, hard = 0, aln = L->cur.line;
    int min_n = 1, have_min = 0;
    size_t a;
    DIR *d;
    struct dirent *ent;
    const char *ip;
    char stems[96][96];
    int nstem = 0;
    if (kw(&L->cur, "NEEDCOUNTMATCHLIBS") || kw(&L->cur, "NEEDCOUNTLIBS") ||
        kw(&L->cur, "REQUIRECOUNTLIBS") || kw(&L->cur, "REQUIRECOUNTMATCHLIBS") ||
        kw(&L->cur, "NEED_COUNTMATCHLIBS") || kw(&L->cur, "MINMATCHLIBS") ||
        kw(&L->cur, "NEEDMINLIBS"))
      hard = 1;
    lex_next(L);
    needle[0] = 0;
    /* min then needle · min defaults to 1 if omitted before str needle is ambiguous */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS) {
      min_n = (int)parse_expr(vm, L);
      have_min = 1;
    } else if (L->cur.kind == TK_IDENT) {
      Var *vi = var_get(vm, L->cur.text, 0);
      if (vi && !vi->is_str) {
        min_n = (int)vi->val;
        have_min = 1;
        lex_next(L);
      }
    }
    if (L->cur.kind == TK_STR) {
      snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(needle, sizeof needle, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(needle, sizeof needle, "%s", vm->last_str);
      else
        snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    }
    if (!have_min) min_n = 1;
    if (min_n < 0) min_n = 0;
    if (!needle[0]) {
      if (hard) {
        fail_at(vm, L,
                "NEEDCOUNTMATCHLIBS needs min needle — NEEDCOUNTMATCHLIBS 2 fat");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "MATCHLIBS_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASCOUNTMATCHLIBS: need min needle");
      var_set_str(vm, "ERR", "HASCOUNTMATCHLIBS: need min needle");
      bump(vm);
      return 1;
    }
    for (a = 0; needle[a] && a + 1 < sizeof fup; a++) {
      char c = needle[a];
      if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
      fup[a] = c;
    }
    fup[a] = 0;
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
          char stem[96], hay[192];
          int dup = 0;
          size_t b;
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
          snprintf(hay, sizeof hay, "%s/%s", dirs[di], ent->d_name);
          for (b = 0; hay[b]; b++)
            if (hay[b] >= 'A' && hay[b] <= 'Z')
              hay[b] = (char)(hay[b] - 'A' + 'a');
          {
            char stem_l[96];
            for (b = 0; stem[b] && b + 1 < sizeof stem_l; b++) {
              char c = stem[b];
              if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
              stem_l[b] = c;
            }
            stem_l[b] = 0;
            if (!strstr(stem_l, fup) && !strstr(hay, fup))
              continue;
          }
          snprintf(stems[nstem++], sizeof stems[0], "%s", stem);
        }
        closedir(d);
      }
    }
    nmatch = nstem;
    var_set_num(vm, "MATCHLIBS_N", nmatch);
    var_set_num(vm, "COUNTMATCHLIBS_N", nmatch);
    var_set_num(vm, "HASCOUNTMATCHLIBS_MIN", min_n);
    var_set_str(vm, "MATCHLIBS_FILTER", needle);
    var_set_str(vm, "COUNTMATCHLIBS_FILTER", needle);
    {
      int ok = (nmatch >= min_n);
      if (hard) {
        if (ok) {
          var_set_str(vm, "LAST", "1");
          snprintf(vm->last_str, sizeof vm->last_str, "1");
          vm->last_n = 1;
          var_set_num(vm, "LAST_N", 1);
          var_set_num(vm, "OK", 1);
          if (vm->res) vm->res->asserts_ok++;
          bump(vm);
          return 1;
        }
        {
          char msg[200];
          snprintf(msg, sizeof msg,
                   "NEEDCOUNTMATCHLIBS miss line %d: need ≥%d '%s' have %d — MATCHLIBS · cubalc libs %s",
                   aln, min_n, needle, nmatch, needle);
          var_set_str(vm, "ERR", msg);
          var_set_str(vm, "LAST_ERR", msg);
          if (vm->res) vm->res->asserts_fail++;
          fail(vm, msg);
          return -1;
        }
      }
      /* soft HASCOUNTMATCHLIBS */
      {
        char nb[4];
        snprintf(nb, sizeof nb, "%d", ok ? 1 : 0);
        var_set_str(vm, "LAST", nb);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
      }
      vm->last_n = ok ? 1 : 0;
      var_set_num(vm, "LAST_N", ok ? 1 : 0);
      var_set_num(vm, "HASCOUNTMATCHLIBS_N", nmatch);
      if (ok) {
        var_set_num(vm, "OK", 1);
      } else {
        char em[176];
        snprintf(em, sizeof em,
                 "HASCOUNTMATCHLIBS miss: need ≥%d '%s' have %d — COUNTMATCHLIBS",
                 min_n, needle, nmatch);
        var_set_str(vm, "ERR", em);
        var_set_str(vm, "LAST_ERR", em);
        var_set_num(vm, "OK", 0);
      }
      bump(vm);
      return 1;
    }
  }

  /* HASMATCHLIBS / NEEDMATCHLIBS needle — soft 0|1 / fail-fast if any stem matches filter.
   * Usability: agent install/discovery gate without MATCHLIBS bag + LINES compare.
   * Completes MATCHLIBS triad (bag · soft probe · hard gate). */
  if (kw(&L->cur, "HASMATCHLIBS") || kw(&L->cur, "NEEDMATCHLIBS") ||
      kw(&L->cur, "ANYMATCHLIBS") || kw(&L->cur, "HASLIBS") ||
      kw(&L->cur, "NEEDLIBS") || kw(&L->cur, "REQUIREMATCHLIBS") ||
      kw(&L->cur, "HAS_MATCHLIBS") || kw(&L->cur, "NEED_MATCHLIBS")) {
    char needle[96], fup[96];
    int nmatch = 0, hard = 0, aln = L->cur.line;
    size_t a;
    DIR *d;
    struct dirent *ent;
    const char *ip;
    if (kw(&L->cur, "NEEDMATCHLIBS") || kw(&L->cur, "NEEDLIBS") ||
        kw(&L->cur, "REQUIREMATCHLIBS") || kw(&L->cur, "NEED_MATCHLIBS"))
      hard = 1;
    lex_next(L);
    needle[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(needle, sizeof needle, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(needle, sizeof needle, "%s", vm->last_str);
      else
        snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    }
    if (!needle[0]) {
      if (hard) {
        fail_at(vm, L, "NEEDMATCHLIBS needs needle — NEEDMATCHLIBS plate");
        return -1;
      }
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "MATCHLIBS_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "HASMATCHLIBS: need needle");
      var_set_str(vm, "ERR", "HASMATCHLIBS: need needle");
      bump(vm);
      return 1;
    }
    for (a = 0; needle[a] && a + 1 < sizeof fup; a++) {
      char c = needle[a];
      if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
      fup[a] = c;
    }
    fup[a] = 0;
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
      for (di = 0; di < nd && nmatch == 0; di++) {
        d = opendir(dirs[di]);
        if (!d) continue;
        while ((ent = readdir(d)) != NULL) {
          size_t len = strlen(ent->d_name);
          char stem[96], hay[192], stem_l[96];
          size_t b;
          if (len < 8 || strcmp(ent->d_name + len - 7, ".cubalc") != 0)
            continue;
          if (ent->d_name[0] == '.') continue;
          if (len - 7 >= sizeof stem) continue;
          memcpy(stem, ent->d_name, len - 7);
          stem[len - 7] = 0;
          snprintf(hay, sizeof hay, "%s/%s", dirs[di], ent->d_name);
          for (b = 0; hay[b]; b++)
            if (hay[b] >= 'A' && hay[b] <= 'Z')
              hay[b] = (char)(hay[b] - 'A' + 'a');
          for (b = 0; stem[b] && b + 1 < sizeof stem_l; b++) {
            char c = stem[b];
            if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
            stem_l[b] = c;
          }
          stem_l[b] = 0;
          if (strstr(stem_l, fup) || strstr(hay, fup)) {
            nmatch = 1;
            break;
          }
        }
        closedir(d);
      }
    }
    var_set_str(vm, "MATCHLIBS_FILTER", needle);
    var_set_num(vm, "MATCHLIBS_N", nmatch);
    if (hard) {
      if (nmatch) {
        var_set_str(vm, "LAST", "1");
        snprintf(vm->last_str, sizeof vm->last_str, "1");
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_num(vm, "OK", 1);
        if (vm->res) vm->res->asserts_ok++;
        bump(vm);
        return 1;
      }
      {
        char msg[192];
        snprintf(msg, sizeof msg,
                 "NEEDMATCHLIBS miss line %d: '%s' — MATCHLIBS · cubalc libs %s · install libs",
                 aln, needle, needle);
        var_set_str(vm, "ERR", msg);
        var_set_str(vm, "LAST_ERR", msg);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
    }
    /* soft HASMATCHLIBS */
    {
      char nb[4];
      snprintf(nb, sizeof nb, "%d", nmatch);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    vm->last_n = nmatch;
    var_set_num(vm, "LAST_N", nmatch);
    var_set_num(vm, "HASMATCHLIBS_N", nmatch);
    if (nmatch) {
      var_set_num(vm, "OK", 1);
    } else {
      char em[160];
      snprintf(em, sizeof em,
               "HASMATCHLIBS miss: '%s' — MATCHLIBS · cubalc libs %s",
               needle, needle);
      var_set_str(vm, "ERR", em);
      var_set_str(vm, "LAST_ERR", em);
      var_set_num(vm, "OK", 0);
    }
    bump(vm);
    return 1;
  }

  /* PICKLIB / FIRSTLIB needle [OR fallback] — first matching stem (sorted).
   * Usability: INCLUDE without MATCHLIBS+NTH glue · soft miss or OR default.
   * LAST = stem · LAST_N 0|1 · PICKLIB_N = match count · soft. */
  if (kw(&L->cur, "PICKLIB") || kw(&L->cur, "FIRSTLIB") ||
      kw(&L->cur, "LIBPICK") || kw(&L->cur, "CHOOSELIB") ||
      kw(&L->cur, "ONEOFLIBS") || kw(&L->cur, "ANYLIBSTEM") ||
      kw(&L->cur, "PICK_LIB") || kw(&L->cur, "FIRST_LIB")) {
    char needle[96], fup[96], fb[96], pick[96];
    char stems[96][96];
    int nstem = 0, i, j, have_fb = 0;
    size_t a;
    DIR *d;
    struct dirent *ent;
    const char *ip;
    lex_next(L);
    needle[0] = 0;
    fb[0] = 0;
    pick[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(needle, sizeof needle, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(needle, sizeof needle, "%s", vm->last_str);
      else
        snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    }
    if (kw(&L->cur, "OR") || kw(&L->cur, "ELSE") || kw(&L->cur, "DEFAULT") ||
        kw(&L->cur, "FALLBACK")) {
      lex_next(L);
      if (L->cur.kind == TK_STR) {
        snprintf(fb, sizeof fb, "%s", L->cur.text);
        have_fb = 1;
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str)
          snprintf(fb, sizeof fb, "%s", vv->sval);
        else if (strcmp(L->cur.text, "LAST") == 0)
          snprintf(fb, sizeof fb, "%s", vm->last_str);
        else
          snprintf(fb, sizeof fb, "%s", L->cur.text);
        have_fb = 1;
        lex_next(L);
      }
    }
    if (!needle[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "PICKLIB_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "PICKLIB: need needle — PICKLIB plate OR agent_boot");
      var_set_str(vm, "ERR", "PICKLIB: need needle — PICKLIB plate OR agent_boot");
      bump(vm);
      return 1;
    }
    for (a = 0; needle[a] && a + 1 < sizeof fup; a++) {
      char c = needle[a];
      if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
      fup[a] = c;
    }
    fup[a] = 0;
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
          char stem[96], hay[192];
          int dup = 0;
          size_t b;
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
          snprintf(hay, sizeof hay, "%s/%s", dirs[di], ent->d_name);
          for (b = 0; hay[b]; b++)
            if (hay[b] >= 'A' && hay[b] <= 'Z')
              hay[b] = (char)(hay[b] - 'A' + 'a');
          {
            char stem_l[96];
            for (b = 0; stem[b] && b + 1 < sizeof stem_l; b++) {
              char c = stem[b];
              if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
              stem_l[b] = c;
            }
            stem_l[b] = 0;
            if (!strstr(stem_l, fup) && !strstr(hay, fup))
              continue;
          }
          snprintf(stems[nstem++], sizeof stems[0], "%s", stem);
        }
        closedir(d);
      }
    }
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
    if (nstem > 0)
      snprintf(pick, sizeof pick, "%s", stems[0]);
    else if (have_fb)
      snprintf(pick, sizeof pick, "%s", fb);
    var_set_num(vm, "PICKLIB_N", nstem);
    var_set_num(vm, "MATCHLIBS_N", nstem);
    var_set_str(vm, "PICKLIB_FILTER", needle);
    var_set_str(vm, "MATCHLIBS_FILTER", needle);
    if (pick[0]) {
      var_set_str(vm, "LAST", pick);
      var_set_str(vm, "PICKLIB", pick);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", pick);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "OK", 1);
    } else {
      char em[160];
      snprintf(em, sizeof em,
               "PICKLIB miss: '%s' — MATCHLIBS · cubalc libs %s · OR fallback",
               needle, needle);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    bump(vm);
    return 1;
  }

  /* NTHLIB idx needle [OR fallback] · LASTLIB needle [OR fallback]
   * Usability: index/last pick from MATCHLIBS without bag+NTH glue.
   * Completes PICKLIB (first only) — agents INCLUDE the 2nd/last match.
   * LAST = stem · LAST_N 0|1 · NTHLIB_N / MATCHLIBS_N = match count · soft. */
  if (kw(&L->cur, "NTHLIB") || kw(&L->cur, "INDEXLIB") ||
      kw(&L->cur, "ATLIB") || kw(&L->cur, "LIBNTH") ||
      kw(&L->cur, "NTH_LIB") || kw(&L->cur, "INDEX_LIB") ||
      kw(&L->cur, "LASTLIB") || kw(&L->cur, "ENDLIB") ||
      kw(&L->cur, "LIBLAST") || kw(&L->cur, "LAST_LIB") ||
      kw(&L->cur, "END_LIB") || kw(&L->cur, "TAILLIBSTEM")) {
    char needle[96], fup[96], fb[96], pick[96];
    char stems[96][96];
    int nstem = 0, i, j, have_fb = 0, want_last = 0, idx = 0, have_idx = 0;
    size_t a;
    DIR *d;
    struct dirent *ent;
    const char *ip;
    if (kw(&L->cur, "LASTLIB") || kw(&L->cur, "ENDLIB") ||
        kw(&L->cur, "LIBLAST") || kw(&L->cur, "LAST_LIB") ||
        kw(&L->cur, "END_LIB") || kw(&L->cur, "TAILLIBSTEM"))
      want_last = 1;
    lex_next(L);
    needle[0] = 0;
    fb[0] = 0;
    pick[0] = 0;
    /* NTHLIB: index then needle · LASTLIB: needle only */
    if (!want_last) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS) {
        idx = (int)parse_expr(vm, L);
        have_idx = 1;
      } else if (L->cur.kind == TK_IDENT) {
        Var *vi = var_get(vm, L->cur.text, 0);
        if (vi && !vi->is_str) {
          idx = (int)vi->val;
          have_idx = 1;
          lex_next(L);
        }
      }
      if (!have_idx) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "NTHLIB_N", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST_ERR",
                    "NTHLIB: need index — NTHLIB 0 plate · LASTLIB plate");
        var_set_str(vm, "ERR",
                    "NTHLIB: need index — NTHLIB 0 plate · LASTLIB plate");
        bump(vm);
        return 1;
      }
    }
    if (L->cur.kind == TK_STR) {
      snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(needle, sizeof needle, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(needle, sizeof needle, "%s", vm->last_str);
      else
        snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    }
    if (kw(&L->cur, "OR") || kw(&L->cur, "ELSE") || kw(&L->cur, "DEFAULT") ||
        kw(&L->cur, "FALLBACK")) {
      lex_next(L);
      if (L->cur.kind == TK_STR) {
        snprintf(fb, sizeof fb, "%s", L->cur.text);
        have_fb = 1;
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str)
          snprintf(fb, sizeof fb, "%s", vv->sval);
        else if (strcmp(L->cur.text, "LAST") == 0)
          snprintf(fb, sizeof fb, "%s", vm->last_str);
        else
          snprintf(fb, sizeof fb, "%s", L->cur.text);
        have_fb = 1;
        lex_next(L);
      }
    }
    if (!needle[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "NTHLIB_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  "NTHLIB/LASTLIB: need needle — NTHLIB 0 plate · LASTLIB fat");
      var_set_str(vm, "ERR",
                  "NTHLIB/LASTLIB: need needle — NTHLIB 0 plate · LASTLIB fat");
      bump(vm);
      return 1;
    }
    for (a = 0; needle[a] && a + 1 < sizeof fup; a++) {
      char c = needle[a];
      if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
      fup[a] = c;
    }
    fup[a] = 0;
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
          char stem[96], hay[192];
          int dup = 0;
          size_t b;
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
          snprintf(hay, sizeof hay, "%s/%s", dirs[di], ent->d_name);
          for (b = 0; hay[b]; b++)
            if (hay[b] >= 'A' && hay[b] <= 'Z')
              hay[b] = (char)(hay[b] - 'A' + 'a');
          {
            char stem_l[96];
            for (b = 0; stem[b] && b + 1 < sizeof stem_l; b++) {
              char c = stem[b];
              if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
              stem_l[b] = c;
            }
            stem_l[b] = 0;
            if (!strstr(stem_l, fup) && !strstr(hay, fup))
              continue;
          }
          snprintf(stems[nstem++], sizeof stems[0], "%s", stem);
        }
        closedir(d);
      }
    }
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
    if (want_last)
      idx = nstem > 0 ? nstem - 1 : 0;
    if (nstem > 0 && idx >= 0 && idx < nstem)
      snprintf(pick, sizeof pick, "%s", stems[idx]);
    else if (have_fb)
      snprintf(pick, sizeof pick, "%s", fb);
    var_set_num(vm, "NTHLIB_N", nstem);
    var_set_num(vm, "LASTLIB_N", nstem);
    var_set_num(vm, "PICKLIB_N", nstem);
    var_set_num(vm, "MATCHLIBS_N", nstem);
    var_set_num(vm, "NTHLIB_I", want_last ? (nstem > 0 ? nstem - 1 : -1) : idx);
    var_set_str(vm, "NTHLIB_FILTER", needle);
    var_set_str(vm, "LASTLIB_FILTER", needle);
    var_set_str(vm, "MATCHLIBS_FILTER", needle);
    if (pick[0]) {
      var_set_str(vm, "LAST", pick);
      var_set_str(vm, "NTHLIB", pick);
      if (want_last)
        var_set_str(vm, "LASTLIB", pick);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", pick);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "OK", 1);
    } else {
      char em[176];
      if (want_last)
        snprintf(em, sizeof em,
                 "LASTLIB miss: '%s' — MATCHLIBS · cubalc libs %s · OR fallback",
                 needle, needle);
      else
        snprintf(em, sizeof em,
                 "NTHLIB miss: idx=%d '%s' — MATCHLIBS · OR fallback",
                 idx, needle);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    bump(vm);
    return 1;
  }

  /* NEWESTLIB|OLDESTLIB needle [OR fallback] — pick matching stem by mtime.
   * Usability: freshest/oldest recipe among filter matches without shell ls -t.
   * Dual of PICKLIB (alpha first) · LAST = stem · NEWESTLIB_MTIME epoch · soft. */
  if (kw(&L->cur, "NEWESTLIB") || kw(&L->cur, "LATESTLIB") ||
      kw(&L->cur, "FRESHLIB") || kw(&L->cur, "NEWLIB") ||
      kw(&L->cur, "OLDESTLIB") || kw(&L->cur, "EARLIESTLIB") ||
      kw(&L->cur, "STALELIB") || kw(&L->cur, "OLDLIB") ||
      kw(&L->cur, "NEWEST_LIB") || kw(&L->cur, "OLDEST_LIB")) {
    char needle[96], fup[96], fb[96], pick[96], pick_path[768];
    int want_old = 0, have_fb = 0, nstem = 0, j;
    long best_mt = 0;
    int have_best = 0;
    size_t a;
    DIR *d;
    struct dirent *ent;
    const char *ip;
    if (kw(&L->cur, "OLDESTLIB") || kw(&L->cur, "EARLIESTLIB") ||
        kw(&L->cur, "STALELIB") || kw(&L->cur, "OLDLIB") ||
        kw(&L->cur, "OLDEST_LIB"))
      want_old = 1;
    lex_next(L);
    needle[0] = 0;
    fb[0] = 0;
    pick[0] = 0;
    pick_path[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(needle, sizeof needle, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(needle, sizeof needle, "%s", vm->last_str);
      else
        snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    }
    if (kw(&L->cur, "OR") || kw(&L->cur, "ELSE") || kw(&L->cur, "DEFAULT") ||
        kw(&L->cur, "FALLBACK")) {
      lex_next(L);
      if (L->cur.kind == TK_STR) {
        snprintf(fb, sizeof fb, "%s", L->cur.text);
        have_fb = 1;
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str)
          snprintf(fb, sizeof fb, "%s", vv->sval);
        else if (strcmp(L->cur.text, "LAST") == 0)
          snprintf(fb, sizeof fb, "%s", vm->last_str);
        else
          snprintf(fb, sizeof fb, "%s", L->cur.text);
        have_fb = 1;
        lex_next(L);
      }
    }
    if (!needle[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "NEWESTLIB_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  "NEWESTLIB: need needle — NEWESTLIB plate · OLDESTLIB fat");
      var_set_str(vm, "ERR",
                  "NEWESTLIB: need needle — NEWESTLIB plate · OLDESTLIB fat");
      bump(vm);
      return 1;
    }
    for (a = 0; needle[a] && a + 1 < sizeof fup; a++) {
      char c = needle[a];
      if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
      fup[a] = c;
    }
    fup[a] = 0;
    {
      char dirs[10][160];
      int nd = 0, di;
      char seen[96][96];
      int nseen = 0;
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
        while ((ent = readdir(d)) != NULL && nseen < 96) {
          size_t len = strlen(ent->d_name);
          char stem[96], hay[192], full[768];
          int dup = 0;
          size_t b;
          struct stat st;
          if (len < 8 || strcmp(ent->d_name + len - 7, ".cubalc") != 0)
            continue;
          if (ent->d_name[0] == '.') continue;
          if (len - 7 >= sizeof stem) continue;
          memcpy(stem, ent->d_name, len - 7);
          stem[len - 7] = 0;
          for (j = 0; j < nseen; j++) {
            if (strcmp(seen[j], stem) == 0) { dup = 1; break; }
          }
          if (dup) continue;
          snprintf(full, sizeof full, "%s/%s", dirs[di], ent->d_name);
          snprintf(hay, sizeof hay, "%s", full);
          for (b = 0; hay[b]; b++)
            if (hay[b] >= 'A' && hay[b] <= 'Z')
              hay[b] = (char)(hay[b] - 'A' + 'a');
          {
            char stem_l[96];
            for (b = 0; stem[b] && b + 1 < sizeof stem_l; b++) {
              char c = stem[b];
              if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
              stem_l[b] = c;
            }
            stem_l[b] = 0;
            if (!strstr(stem_l, fup) && !strstr(hay, fup))
              continue;
          }
          if (stat(full, &st) != 0) continue;
          snprintf(seen[nseen++], sizeof seen[0], "%s", stem);
          nstem++;
          if (!have_best ||
              (want_old ? (st.st_mtime < best_mt) : (st.st_mtime > best_mt)) ||
              (st.st_mtime == best_mt && strcmp(stem, pick) < 0)) {
            best_mt = (long)st.st_mtime;
            have_best = 1;
            snprintf(pick, sizeof pick, "%s", stem);
            snprintf(pick_path, sizeof pick_path, "%s", full);
          }
        }
        closedir(d);
      }
    }
    var_set_num(vm, "NEWESTLIB_N", nstem);
    var_set_num(vm, "OLDESTLIB_N", nstem);
    var_set_num(vm, "MATCHLIBS_N", nstem);
    var_set_str(vm, "MATCHLIBS_FILTER", needle);
    var_set_str(vm, "NEWESTLIB_FILTER", needle);
    if (have_best) {
      var_set_str(vm, "LAST", pick);
      var_set_str(vm, want_old ? "OLDESTLIB" : "NEWESTLIB", pick);
      var_set_str(vm, "LIB_PATH", pick_path);
      var_set_num(vm, "NEWESTLIB_MTIME", best_mt);
      var_set_num(vm, "OLDESTLIB_MTIME", best_mt);
      var_set_num(vm, "LIB_MTIME", best_mt);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", pick);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "OK", 1);
    } else if (have_fb) {
      var_set_str(vm, "LAST", fb);
      var_set_str(vm, want_old ? "OLDESTLIB" : "NEWESTLIB", fb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", fb);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "OK", 1);
      var_set_num(vm, "NEWESTLIB_FALLBACK", 1);
    } else {
      char em[176];
      snprintf(em, sizeof em,
               "%s miss: '%s' — MATCHLIBS · cubalc libs %s · OR fallback",
               want_old ? "OLDESTLIB" : "NEWESTLIB", needle, needle);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
    }
    bump(vm);
    return 1;
  }

  /* SORTLIBS needle [ASC|DESC|OLDEST|NEWEST] — bag of matching stems ordered by mtime.
   * Usability: walk recipes freshest-first (default DESC/NEWEST) without shell ls -t
   * or NEWESTLIB single-pick. Dual of alpha MATCHLIBS · head of bag == NEWESTLIB.
   * LAST = newline bag · LAST_N = count · LIB_PATH / SORTLIBS_MTIME for first · soft empty. */
  if (kw(&L->cur, "SORTLIBS") || kw(&L->cur, "SORTLIB") ||
      kw(&L->cur, "LIBSORT") || kw(&L->cur, "SORTBYMTIME") ||
      kw(&L->cur, "LIBSBYMTIME") || kw(&L->cur, "MATCHLIBSMT") ||
      kw(&L->cur, "MTIMESORTLIBS") || kw(&L->cur, "SORT_LIBS") ||
      kw(&L->cur, "LIBS_BY_MTIME") || kw(&L->cur, "SORTMTIMELIBS")) {
    char needle[96], fup[96], bag[4096];
    char stems[96][96];
    char stem_paths[96][768];
    long mtimes[96];
    int nstem = 0, i, j, want_old = 0;
    size_t o = 0, a;
    DIR *d;
    struct dirent *ent;
    const char *ip;
    lex_next(L);
    needle[0] = 0;
    bag[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(needle, sizeof needle, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(needle, sizeof needle, "%s", vm->last_str);
      else
        snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    }
    /* optional order: ASC|OLDEST|EARLIEST → oldest first · else newest first */
    if (kw(&L->cur, "ASC") || kw(&L->cur, "OLDEST") || kw(&L->cur, "EARLIEST") ||
        kw(&L->cur, "OLD") || kw(&L->cur, "STALE") || kw(&L->cur, "OLDESTFIRST") ||
        kw(&L->cur, "ASCENDING")) {
      want_old = 1;
      lex_next(L);
    } else if (kw(&L->cur, "DESC") || kw(&L->cur, "NEWEST") ||
               kw(&L->cur, "LATEST") || kw(&L->cur, "NEW") ||
               kw(&L->cur, "FRESH") || kw(&L->cur, "NEWESTFIRST") ||
               kw(&L->cur, "DESCENDING")) {
      want_old = 0;
      lex_next(L);
    }
    if (!needle[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "SORTLIBS_N", 0);
      var_set_num(vm, "MATCHLIBS_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  "SORTLIBS: need needle — SORTLIBS fat · SORTLIBS plate ASC");
      var_set_str(vm, "ERR",
                  "SORTLIBS: need needle — SORTLIBS fat · SORTLIBS plate ASC");
      bump(vm);
      return 1;
    }
    for (a = 0; needle[a] && a + 1 < sizeof fup; a++) {
      char c = needle[a];
      if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
      fup[a] = c;
    }
    fup[a] = 0;
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
          char stem[96], hay[192], full[768];
          int dup = 0;
          size_t b;
          struct stat st;
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
          snprintf(full, sizeof full, "%s/%s", dirs[di], ent->d_name);
          snprintf(hay, sizeof hay, "%s", full);
          for (b = 0; hay[b]; b++)
            if (hay[b] >= 'A' && hay[b] <= 'Z')
              hay[b] = (char)(hay[b] - 'A' + 'a');
          {
            char stem_l[96];
            for (b = 0; stem[b] && b + 1 < sizeof stem_l; b++) {
              char c = stem[b];
              if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
              stem_l[b] = c;
            }
            stem_l[b] = 0;
            if (!strstr(stem_l, fup) && !strstr(hay, fup))
              continue;
          }
          if (stat(full, &st) != 0) continue;
          snprintf(stems[nstem], sizeof stems[0], "%s", stem);
          snprintf(stem_paths[nstem], sizeof stem_paths[0], "%s", full);
          mtimes[nstem] = (long)st.st_mtime;
          nstem++;
        }
        closedir(d);
      }
    }
    /* insertion sort by mtime (want_old: asc · else desc) · stem alpha ties */
    for (i = 1; i < nstem; i++) {
      char tmp[96], tp[768];
      long tm;
      snprintf(tmp, sizeof tmp, "%s", stems[i]);
      snprintf(tp, sizeof tp, "%s", stem_paths[i]);
      tm = mtimes[i];
      j = i;
      while (j > 0) {
        int before = 0;
        if (want_old) {
          if (mtimes[j - 1] > tm ||
              (mtimes[j - 1] == tm && strcmp(stems[j - 1], tmp) > 0))
            before = 1;
        } else {
          if (mtimes[j - 1] < tm ||
              (mtimes[j - 1] == tm && strcmp(stems[j - 1], tmp) > 0))
            before = 1;
        }
        if (!before) break;
        snprintf(stems[j], sizeof stems[0], "%s", stems[j - 1]);
        snprintf(stem_paths[j], sizeof stem_paths[0], "%s", stem_paths[j - 1]);
        mtimes[j] = mtimes[j - 1];
        j--;
      }
      snprintf(stems[j], sizeof stems[0], "%s", tmp);
      snprintf(stem_paths[j], sizeof stem_paths[0], "%s", tp);
      mtimes[j] = tm;
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
    var_set_str(vm, "SORTLIBS", bag);
    var_set_str(vm, "MATCHLIBS", bag);
    var_set_str(vm, "SORTLIBS_FILTER", needle);
    var_set_str(vm, "MATCHLIBS_FILTER", needle);
    var_set_str(vm, "SORTLIBS_ORDER", want_old ? "oldest" : "newest");
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = nstem;
    var_set_num(vm, "LAST_N", nstem);
    var_set_num(vm, "SORTLIBS_N", nstem);
    var_set_num(vm, "MATCHLIBS_N", nstem);
    if (nstem > 0) {
      var_set_str(vm, "LIB_PATH", stem_paths[0]);
      var_set_num(vm, "SORTLIBS_MTIME", mtimes[0]);
      var_set_num(vm, "LIB_MTIME", mtimes[0]);
      var_set_str(vm, "SORTLIBS_HEAD", stems[0]);
    } else {
      var_set_str(vm, "LIB_PATH", "");
      var_set_num(vm, "SORTLIBS_MTIME", 0);
      var_set_num(vm, "LIB_MTIME", 0);
      var_set_str(vm, "SORTLIBS_HEAD", "");
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* FRESHLIBS|STALELIBS needle age_sec — age-filter matching stems (mtime vs now).
   * Usability: keep recent / drop stale recipes without shell find -mtime or EACH+AGE.
   * FRESHLIBS: age <= max_sec (default 86400) · STALELIBS: age >= min_sec (default 86400).
   * Bag sorted newest-first · LAST_N=count · FRESHLIBS_N · soft empty OK=1. */
  if (kw(&L->cur, "FRESHLIBS") || kw(&L->cur, "RECENTLIBS") ||
      kw(&L->cur, "KEEPFRESHLIBS") || kw(&L->cur, "FRESH_LIBS") ||
      kw(&L->cur, "STALELIBS") || kw(&L->cur, "OLDERLIBS") ||
      kw(&L->cur, "KEEPSTALELIBS") || kw(&L->cur, "STALE_LIBS") ||
      kw(&L->cur, "AGEDLIBS") || kw(&L->cur, "OLD_LIBS")) {
    char needle[96], fup[96], bag[4096];
    char stems[96][96];
    char stem_paths[96][768];
    long mtimes[96];
    long ages[96];
    int nstem = 0, i, j, want_stale = 0;
    long age_lim = 86400; /* 1 day default */
    int have_age = 0;
    size_t o = 0, a;
    time_t now = time(NULL);
    DIR *d;
    struct dirent *ent;
    const char *ip;
    if (kw(&L->cur, "STALELIBS") || kw(&L->cur, "OLDERLIBS") ||
        kw(&L->cur, "KEEPSTALELIBS") || kw(&L->cur, "STALE_LIBS") ||
        kw(&L->cur, "AGEDLIBS") || kw(&L->cur, "OLD_LIBS"))
      want_stale = 1;
    lex_next(L);
    needle[0] = 0;
    bag[0] = 0;
    if (L->cur.kind == TK_STR) {
      snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && vv->is_str && vv->sval[0])
        snprintf(needle, sizeof needle, "%s", vv->sval);
      else if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(needle, sizeof needle, "%s", vm->last_str);
      else
        snprintf(needle, sizeof needle, "%s", L->cur.text);
      lex_next(L);
    }
    /* age threshold: number or num var · optional SEC/S/SECONDS suffix keyword skipped */
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
      age_lim = (long)parse_expr(vm, L);
      have_age = 1;
    } else if (L->cur.kind == TK_IDENT) {
      Var *vv = var_get(vm, L->cur.text, 0);
      if (vv && !vv->is_str) {
        age_lim = (long)vv->val;
        have_age = 1;
        lex_next(L);
      }
    }
    if (kw(&L->cur, "SEC") || kw(&L->cur, "SECS") || kw(&L->cur, "SECONDS") ||
        kw(&L->cur, "S") || kw(&L->cur, "AGE"))
      lex_next(L);
    (void)have_age;
    if (age_lim < 0) age_lim = 0;
    if (!needle[0]) {
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "FRESHLIBS_N", 0);
      var_set_num(vm, "STALELIBS_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  "FRESHLIBS: need needle — FRESHLIBS fat 3600 · STALELIBS plate 86400");
      var_set_str(vm, "ERR",
                  "FRESHLIBS: need needle — FRESHLIBS fat 3600 · STALELIBS plate 86400");
      bump(vm);
      return 1;
    }
    for (a = 0; needle[a] && a + 1 < sizeof fup; a++) {
      char c = needle[a];
      if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
      fup[a] = c;
    }
    fup[a] = 0;
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
          char stem[96], hay[192], full[768];
          int dup = 0;
          size_t b;
          struct stat st;
          long age;
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
          snprintf(full, sizeof full, "%s/%s", dirs[di], ent->d_name);
          snprintf(hay, sizeof hay, "%s", full);
          for (b = 0; hay[b]; b++)
            if (hay[b] >= 'A' && hay[b] <= 'Z')
              hay[b] = (char)(hay[b] - 'A' + 'a');
          {
            char stem_l[96];
            for (b = 0; stem[b] && b + 1 < sizeof stem_l; b++) {
              char c = stem[b];
              if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
              stem_l[b] = c;
            }
            stem_l[b] = 0;
            if (!strstr(stem_l, fup) && !strstr(hay, fup))
              continue;
          }
          if (stat(full, &st) != 0) continue;
          age = (long)now - (long)st.st_mtime;
          if (age < 0) age = 0;
          if (want_stale) {
            if (age < age_lim) continue;
          } else {
            if (age > age_lim) continue;
          }
          snprintf(stems[nstem], sizeof stems[0], "%s", stem);
          snprintf(stem_paths[nstem], sizeof stem_paths[0], "%s", full);
          mtimes[nstem] = (long)st.st_mtime;
          ages[nstem] = age;
          nstem++;
        }
        closedir(d);
      }
    }
    /* newest-first (mtime desc) · alpha ties */
    for (i = 1; i < nstem; i++) {
      char tmp[96], tp[768];
      long tm, ag;
      snprintf(tmp, sizeof tmp, "%s", stems[i]);
      snprintf(tp, sizeof tp, "%s", stem_paths[i]);
      tm = mtimes[i];
      ag = ages[i];
      j = i;
      while (j > 0) {
        int before = 0;
        if (mtimes[j - 1] < tm ||
            (mtimes[j - 1] == tm && strcmp(stems[j - 1], tmp) > 0))
          before = 1;
        if (!before) break;
        snprintf(stems[j], sizeof stems[0], "%s", stems[j - 1]);
        snprintf(stem_paths[j], sizeof stem_paths[0], "%s", stem_paths[j - 1]);
        mtimes[j] = mtimes[j - 1];
        ages[j] = ages[j - 1];
        j--;
      }
      snprintf(stems[j], sizeof stems[0], "%s", tmp);
      snprintf(stem_paths[j], sizeof stem_paths[0], "%s", tp);
      mtimes[j] = tm;
      ages[j] = ag;
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
    var_set_str(vm, want_stale ? "STALELIBS" : "FRESHLIBS", bag);
    var_set_str(vm, "MATCHLIBS", bag);
    var_set_str(vm, "FRESHLIBS_FILTER", needle);
    var_set_str(vm, "STALELIBS_FILTER", needle);
    var_set_str(vm, "MATCHLIBS_FILTER", needle);
    var_set_str(vm, "FRESHLIBS_MODE", want_stale ? "stale" : "fresh");
    var_set_num(vm, "FRESHLIBS_AGE", age_lim);
    var_set_num(vm, "STALELIBS_AGE", age_lim);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
    vm->last_n = nstem;
    var_set_num(vm, "LAST_N", nstem);
    var_set_num(vm, "FRESHLIBS_N", nstem);
    var_set_num(vm, "STALELIBS_N", nstem);
    var_set_num(vm, "MATCHLIBS_N", nstem);
    if (nstem > 0) {
      var_set_str(vm, "LIB_PATH", stem_paths[0]);
      var_set_num(vm, "LIB_MTIME", mtimes[0]);
      var_set_num(vm, "LIB_AGE", ages[0]);
      var_set_num(vm, "FRESHLIBS_MTIME", mtimes[0]);
      var_set_str(vm, "FRESHLIBS_HEAD", stems[0]);
      var_set_str(vm, "STALELIBS_HEAD", stems[0]);
    } else {
      var_set_str(vm, "LIB_PATH", "");
      var_set_num(vm, "LIB_MTIME", 0);
      var_set_num(vm, "LIB_AGE", 0);
      var_set_num(vm, "FRESHLIBS_MTIME", 0);
      var_set_str(vm, "FRESHLIBS_HEAD", "");
      var_set_str(vm, "STALELIBS_HEAD", "");
    }
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  /* LIBAGE name · HASFRESH name [max_age] · NEEDFRESH name [max_age]
   * Usability: single-lib freshness contract before INCLUDE — age seconds + soft/hard
   * gates without LIBINFO JSON peel or FRESHLIBS bag. Default max_age 86400.
   * LIBAGE → LAST_N=age · HASFRESH → LAST_N 0|1 · NEEDFRESH fail if missing/stale. */
  if (kw(&L->cur, "LIBAGE") || kw(&L->cur, "AGEOFLIB") ||
      kw(&L->cur, "LIB_AGE") || kw(&L->cur, "MTIMEAGE") ||
      kw(&L->cur, "AGE_LIB") || kw(&L->cur, "STATLIBAGE") ||
      kw(&L->cur, "HASFRESH") || kw(&L->cur, "ISFRESH") ||
      kw(&L->cur, "LIBFRESH") || kw(&L->cur, "HAS_FRESH") ||
      kw(&L->cur, "NEEDFRESH") || kw(&L->cur, "REQUIREFRESH") ||
      kw(&L->cur, "NEED_FRESH") || kw(&L->cur, "REQUIRE_FRESH")) {
    int mode_age = 0, mode_has = 0, mode_need = 0;
    char name[160], path[768], base[160];
    long max_age = 86400, age = -1, mtime = 0;
    int have_max = 0, aln = L->cur.line;
    struct stat st;
    time_t now = time(NULL);
    if (kw(&L->cur, "LIBAGE") || kw(&L->cur, "AGEOFLIB") ||
        kw(&L->cur, "LIB_AGE") || kw(&L->cur, "MTIMEAGE") ||
        kw(&L->cur, "AGE_LIB") || kw(&L->cur, "STATLIBAGE"))
      mode_age = 1;
    else if (kw(&L->cur, "NEEDFRESH") || kw(&L->cur, "REQUIREFRESH") ||
             kw(&L->cur, "NEED_FRESH") || kw(&L->cur, "REQUIRE_FRESH"))
      mode_need = 1;
    else
      mode_has = 1;
    lex_next(L);
    name[0] = 0;
    path[0] = 0;
    base[0] = 0;
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
    if (!mode_age) {
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        max_age = (long)parse_expr(vm, L);
        have_max = 1;
      } else if (L->cur.kind == TK_IDENT) {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && !vv->is_str) {
          max_age = (long)vv->val;
          have_max = 1;
          lex_next(L);
        }
      }
      if (kw(&L->cur, "SEC") || kw(&L->cur, "SECS") || kw(&L->cur, "SECONDS") ||
          kw(&L->cur, "S") || kw(&L->cur, "AGE"))
        lex_next(L);
      (void)have_max;
      if (max_age < 0) max_age = 0;
    }
    if (!name[0]) {
      if (mode_need) {
        fail_at(vm, L, "NEEDFRESH needs name — NEEDFRESH fat_session 86400");
        return -1;
      }
      var_set_str(vm, "LAST", mode_age ? "" : "0");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", mode_age ? "" : "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "LIB_AGE", -1);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR",
                  mode_age ? "LIBAGE: need name — LIBAGE fat_session"
                           : "HASFRESH: need name — HASFRESH fat_session 86400");
      var_set_str(vm, "ERR",
                  mode_age ? "LIBAGE: need name — LIBAGE fat_session"
                           : "HASFRESH: need name — HASFRESH fat_session 86400");
      bump(vm);
      return 1;
    }
    if (!lib_resolve_path(name, path, sizeof path, base, sizeof base) ||
        stat(path, &st) != 0) {
      char em[192];
      snprintf(em, sizeof em,
               "%s miss: '%s' — cubalc which · HASLIB · LIBAGE · install lib",
               mode_need ? "NEEDFRESH" : (mode_age ? "LIBAGE" : "HASFRESH"),
               name);
      if (mode_need) {
        fail_at(vm, L, em);
        return -1;
      }
      var_set_str(vm, "LAST", mode_age ? "" : "0");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", mode_age ? "" : "0");
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "LIB_AGE", -1);
      var_set_num(vm, "LIB_MTIME", 0);
      var_set_str(vm, "LIB_PATH", "");
      var_set_str(vm, "LIB_STEM", base[0] ? base : name);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
    mtime = (long)st.st_mtime;
    age = (long)now - mtime;
    if (age < 0) age = 0;
    var_set_str(vm, "LIB_PATH", path);
    var_set_str(vm, "LIB_STEM", base);
    var_set_num(vm, "LIB_MTIME", mtime);
    var_set_num(vm, "LIB_AGE", age);
    var_set_num(vm, "HASFRESH_AGE", max_age);
    var_set_num(vm, "NEEDFRESH_AGE", max_age);
    if (mode_age) {
      char abuf[32];
      snprintf(abuf, sizeof abuf, "%ld", age);
      var_set_str(vm, "LAST", abuf);
      var_set_str(vm, "LIBAGE", abuf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", abuf);
      vm->last_n = age;
      var_set_num(vm, "LAST_N", age);
      var_set_num(vm, "OK", 1);
      bump(vm);
      return 1;
    }
    {
      int fresh = (age <= max_age) ? 1 : 0;
      char em[200];
      var_set_num(vm, "LAST_N", fresh);
      vm->last_n = fresh;
      {
        char nb[8];
        snprintf(nb, sizeof nb, "%d", fresh);
        var_set_str(vm, "LAST", nb);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
      }
      var_set_num(vm, "HASFRESH_N", fresh);
      if (fresh) {
        var_set_num(vm, "OK", 1);
        bump(vm);
        return 1;
      }
      snprintf(em, sizeof em,
               "%s stale: '%s' age=%ld > %ld — FRESHLIBS · touch/update lib · LIBAGE",
               mode_need ? "NEEDFRESH" : "HASFRESH", base[0] ? base : name,
               age, max_age);
      if (mode_need) {
        fail_at(vm, L, em);
        (void)aln;
        return -1;
      }
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
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

  /* CHECKDEPS / HASDEPS / NEEDDEPS name — install-layout gate for a lib + transitive
   * INCLUDE closure (disk presence, not yet-loaded). Usability: fail-fast before
   * INCLUDE fat_session when a dep is missing, without cascading open errors.
   *   HASDEPS fat_session     → LAST_N 0|1 soft
   *   CHECKDEPS fat_session   → DEPS_MISS bag · LAST_N=miss count · always OK=1 audit
   *   NEEDDEPS fat_session    → fatal if any stem missing (root or tree)
   */
  if (kw(&L->cur, "CHECKDEPS") || kw(&L->cur, "HASDEPS") ||
      kw(&L->cur, "NEEDDEPS") || kw(&L->cur, "DEPSOK") ||
      kw(&L->cur, "VERIFYDEPS") || kw(&L->cur, "DEPSMISS") ||
      kw(&L->cur, "NEEDLIBTREE") || kw(&L->cur, "HASLIBTREE") ||
      kw(&L->cur, "CHECKLIBTREE") || kw(&L->cur, "REQUIREDEPS")) {
    char name[160], root_path[768], root_base[160];
    char stems[64][96];
    char miss[CUBALC_VAR_STR_MAX], okbag[CUBALC_VAR_STR_MAX];
    int nstem = 0, nmiss = 0, nok = 0, qi = 0, i;
    int hard = 0, as_ok = 0, as_miss = 0;
    int aln = L->cur.line;
    size_t mo = 0, oo = 0;
    if (kw(&L->cur, "NEEDDEPS") || kw(&L->cur, "NEEDLIBTREE") ||
        kw(&L->cur, "REQUIREDEPS"))
      hard = 1;
    else if (kw(&L->cur, "HASDEPS") || kw(&L->cur, "DEPSOK") ||
             kw(&L->cur, "HASLIBTREE") || kw(&L->cur, "VERIFYDEPS"))
      as_ok = 1;
    else
      as_miss = 1; /* CHECKDEPS / DEPSMISS / CHECKLIBTREE */
    lex_next(L);
    name[0] = 0;
    root_path[0] = 0;
    miss[0] = 0;
    okbag[0] = 0;
    if (kw(&L->cur, "LIB") || kw(&L->cur, "OF") || kw(&L->cur, "FOR") ||
        kw(&L->cur, "MODULE") || kw(&L->cur, "RECIPE"))
      lex_next(L);
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
      if (hard) {
        fail_at(vm, L, "NEEDDEPS needs lib name — NEEDDEPS fat_session");
        return -1;
      }
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", "CHECKDEPS: need name");
      var_set_str(vm, "ERR", "CHECKDEPS: need name");
      bump(vm);
      return 1;
    }
    /* collect root + transitive deps */
    nstem = 0;
    if (!lib_resolve_path(name, root_path, sizeof root_path, root_base, sizeof root_base)) {
      /* root itself missing */
      snprintf(stems[0], sizeof stems[0], "%s", name);
      {
        const char *slash = strrchr(name, '/');
        const char *leaf = slash ? slash + 1 : name;
        size_t blen;
        snprintf(stems[0], sizeof stems[0], "%s", leaf);
        blen = strlen(stems[0]);
        if (blen > 7 && strcmp(stems[0] + blen - 7, ".cubalc") == 0)
          stems[0][blen - 7] = 0;
      }
      nstem = 1;
      /* cannot walk further */
    } else {
      snprintf(stems[nstem++], sizeof stems[0], "%s", root_base);
      lib_append_deps(root_path, stems, &nstem, 64);
      qi = 1; /* start after root — expand each known stem */
      while (qi < nstem && nstem < 64) {
        char dpath[768], dbase[160];
        if (lib_resolve_path(stems[qi], dpath, sizeof dpath, dbase, sizeof dbase))
          lib_append_deps(dpath, stems, &nstem, 64);
        qi++;
      }
    }
    mo = 0;
    oo = 0;
    nmiss = 0;
    nok = 0;
    for (i = 0; i < nstem; i++) {
      char pth[768], base[160];
      size_t ln = strlen(stems[i]);
      if (!stems[i][0]) continue;
      if (lib_resolve_path(stems[i], pth, sizeof pth, base, sizeof base)) {
        if (nok > 0 && oo + 1 < sizeof okbag) okbag[oo++] = '\n';
        if (oo + ln < sizeof okbag) {
          memcpy(okbag + oo, stems[i], ln);
          oo += ln;
        }
        okbag[oo] = 0;
        nok++;
      } else {
        if (nmiss > 0 && mo + 1 < sizeof miss) miss[mo++] = '\n';
        if (mo + ln < sizeof miss) {
          memcpy(miss + mo, stems[i], ln);
          mo += ln;
        }
        miss[mo] = 0;
        nmiss++;
      }
    }
    var_set_str(vm, "DEPS_MISS", miss);
    var_set_str(vm, "DEPS_OK", okbag);
    var_set_num(vm, "DEPS_MISS_N", nmiss);
    var_set_num(vm, "DEPS_OK_N", nok);
    var_set_num(vm, "DEPS_N", nstem);
    var_set_str(vm, "LIB_STEM", root_base[0] ? root_base : name);
    var_set_str(vm, "LIB_PATH", root_path);
    var_set_num(vm, "CHECKDEPS_N", nstem);

    if (hard) {
      if (nmiss == 0) {
        var_set_num(vm, "LAST_N", 1);
        vm->last_n = 1;
        var_set_str(vm, "LAST", "1");
        snprintf(vm->last_str, sizeof vm->last_str, "1");
        var_set_num(vm, "OK", 1);
        if (vm->res) vm->res->asserts_ok++;
        if (vm->trace)
          fprintf(vm->trace, "# needdeps ok %s n=%d\n", name, nstem);
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
                 "NEEDDEPS missing line %d: %s — cubalc libs · cubalc recipe %s · "
                 "CHECKDEPS / HASDEPS",
                 aln, flat[0] ? flat : "?", name);
        var_set_str(vm, "ERR", msg);
        var_set_str(vm, "LAST_ERR", msg);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
    }

    if (as_ok) {
      int ok = (nmiss == 0 && nstem > 0);
      /* empty name already handled; nstem==0 shouldn't happen */
      if (nstem == 0) ok = 0;
      var_set_num(vm, "LAST_N", ok);
      vm->last_n = ok;
      if (ok) {
        var_set_str(vm, "LAST", "1");
        snprintf(vm->last_str, sizeof vm->last_str, "1");
        var_set_num(vm, "OK", 1);
      } else {
        var_set_str(vm, "LAST", miss[0] ? miss : "0");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", miss[0] ? miss : "0");
        var_set_num(vm, "OK", 0);
        {
          char em[192];
          snprintf(em, sizeof em,
                   "HASDEPS miss line %d: %s — cubalc libs · CHECKDEPS",
                   aln, miss[0] ? miss : name);
          var_set_str(vm, "ERR", em);
          var_set_str(vm, "LAST_ERR", em);
        }
      }
      bump(vm);
      return 1;
    }

    /* CHECKDEPS / DEPSMISS — audit bag; OK=1; LAST_N = miss count */
    var_set_str(vm, "LAST", miss);
    var_set_str(vm, "CHECKDEPS", miss);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", miss);
    vm->last_n = nmiss;
    var_set_num(vm, "LAST_N", nmiss);
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

  /* RECIPE / LIBCARD / CARD name — one-shot lib recipe plate (dual of cubalc recipe).
   * Usability: path + deps + defaults + head without LIBINFO+LIBDEPS+LIBDEFAULTS+HEADLIB glue.
   * LAST = cubalc.recipe.v1 JSON · RECIPE_DEPS/DEFAULTS/HEAD bags · soft miss. */
  if (kw(&L->cur, "RECIPE") || kw(&L->cur, "LIBCARD") ||
      kw(&L->cur, "LIB_CARD") || kw(&L->cur, "LIBRECIPE") ||
      kw(&L->cur, "DESCRIBERECIPE") || kw(&L->cur, "CARD") ||
      kw(&L->cur, "RECIPECARD") || kw(&L->cur, "LIB_RECIPE")) {
    char name[160], path[768], base[160], plate[CUBALC_VAR_STR_MAX];
    char deps_bag[CUBALC_VAR_STR_MAX], knobs_bag[CUBALC_VAR_STR_MAX];
    char head_bag[CUBALC_VAR_STR_MAX];
    char deps[32][96], knobs[32][160], head[8][256];
    char *src = NULL;
    FILE *f = NULL;
    long sz = 0;
    size_t nr = 0, o = 0;
    int nlines = 0, ndep = 0, nknob = 0, nhead = 0, j;
    const char *lp;
    lex_next(L);
    name[0] = 0;
    path[0] = 0;
    plate[0] = 0;
    deps_bag[0] = 0;
    knobs_bag[0] = 0;
    head_bag[0] = 0;
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
      var_set_str(vm, "LAST_ERR", "RECIPE: need name");
      var_set_str(vm, "ERR", "RECIPE: need name");
      bump(vm);
      return 1;
    }
    if (!lib_resolve_path(name, path, sizeof path, base, sizeof base)) {
      char em[192];
      snprintf(em, sizeof em,
               "RECIPE miss: '%s' — programs/lib · CUBALC_INCLUDE_PATH · cubalc recipe",
               name);
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      vm->last_n = 0;
      var_set_num(vm, "LAST_N", 0);
      var_set_str(vm, "LIB_PATH", "");
      var_set_str(vm, "RECIPE_DEPS", "");
      var_set_str(vm, "RECIPE_DEFAULTS", "");
      var_set_str(vm, "RECIPE_HEAD", "");
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST_ERR", em);
      var_set_str(vm, "ERR", em);
      bump(vm);
      return 1;
    }
    f = fopen(path, "rb");
    if (!f) {
      char em[192];
      snprintf(em, sizeof em, "RECIPE open fail: '%s'", path);
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
      var_set_str(vm, "LAST_ERR", "RECIPE: oom");
      var_set_str(vm, "ERR", "RECIPE: oom");
      bump(vm);
      return 1;
    }
    nr = fread(src, 1, (size_t)sz, f);
    fclose(f);
    src[nr] = 0;
    lp = src;
    nlines = 0;
    ndep = 0;
    nknob = 0;
    nhead = 0;
    while (*lp) {
      char raw[512];
      size_t ri = 0;
      const char *p;
      while (*lp && *lp != '\n' && ri + 1 < sizeof raw)
        raw[ri++] = *lp++;
      raw[ri] = 0;
      if (*lp == '\n') lp++;
      nlines++;
      if (nhead < 8) {
        snprintf(head[nhead], sizeof head[0], "%s", raw);
        nhead++;
      }
      p = raw;
      while (*p == ' ' || *p == '\t') p++;
      if (*p == '#' || !*p) continue;
      if (p[0] == 'I' && p[1] == 'N' && p[2] == 'C' && p[3] == 'L' &&
          p[4] == 'U' && p[5] == 'D' && p[6] == 'E' &&
          (p[7] == ' ' || p[7] == '\t' || p[7] == '"' || p[7] == '\'')) {
        char tok[96];
        size_t ti = 0;
        p += 7;
        while (*p == ' ' || *p == '\t') p++;
        for (;;) {
          if (p[0] == 'O' && p[1] == 'N' && p[2] == 'C' && p[3] == 'E' &&
              (p[4] == ' ' || p[4] == '\t' || !p[4])) {
            p += 4; while (*p == ' ' || *p == '\t') p++; continue;
          }
          if (p[0] == 'S' && p[1] == 'O' && p[2] == 'F' && p[3] == 'T' &&
              (p[4] == ' ' || p[4] == '\t' || !p[4])) {
            p += 4; while (*p == ' ' || *p == '\t') p++; continue;
          }
          if (p[0] == 'O' && p[1] == 'R' &&
              (p[2] == ' ' || p[2] == '\t' || !p[2])) {
            p += 2; while (*p == ' ' || *p == '\t') p++; continue;
          }
          break;
        }
        if (*p == '"' || *p == '\'') {
          char q = *p++;
          while (*p && *p != q && ti + 1 < sizeof tok) tok[ti++] = *p++;
          tok[ti] = 0;
        } else {
          while (*p && *p != ' ' && *p != '\t' && *p != '#' && ti + 1 < sizeof tok)
            tok[ti++] = *p++;
          tok[ti] = 0;
        }
        if (tok[0] && ndep < 32) {
          const char *sl = strrchr(tok, '/');
          const char *st = sl ? sl + 1 : tok;
          size_t slen;
          snprintf(deps[ndep], sizeof deps[0], "%s", st);
          slen = strlen(deps[ndep]);
          if (slen > 7 && strcmp(deps[ndep] + slen - 7, ".cubalc") == 0)
            deps[ndep][slen - 7] = 0;
          if (deps[ndep][0]) ndep++;
        }
      } else if (p[0] == 'D' && p[1] == 'E' && p[2] == 'F' && p[3] == 'A' &&
                 p[4] == 'U' && p[5] == 'L' && p[6] == 'T' &&
                 (p[7] == ' ' || p[7] == '\t')) {
        char key[64], val[96], kv[160];
        size_t ki = 0, vi = 0;
        p += 7;
        while (*p == ' ' || *p == '\t') p++;
        while (*p && *p != ' ' && *p != '\t' && *p != '=' && ki + 1 < sizeof key)
          key[ki++] = *p++;
        key[ki] = 0;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '=') { p++; while (*p == ' ' || *p == '\t') p++; }
        if (*p == '"' || *p == '\'') {
          char q = *p++;
          while (*p && *p != q && vi + 1 < sizeof val) val[vi++] = *p++;
          val[vi] = 0;
        } else {
          while (*p && *p != '#' && vi + 1 < sizeof val) val[vi++] = *p++;
          val[vi] = 0;
          while (vi > 0 && (val[vi - 1] == ' ' || val[vi - 1] == '\t'))
            val[--vi] = 0;
        }
        if (key[0] && nknob < 32) {
          snprintf(kv, sizeof kv, "%s=%s", key, val);
          snprintf(knobs[nknob++], sizeof knobs[0], "%s", kv);
        }
      }
    }
    free(src);
    /* bags for in-lang HASLINE / EACH without JSON peel */
    o = 0;
    for (j = 0; j < ndep; j++) {
      size_t ln = strlen(deps[j]);
      if (j > 0 && o + 1 < sizeof deps_bag) deps_bag[o++] = '\n';
      if (o + ln < sizeof deps_bag) {
        memcpy(deps_bag + o, deps[j], ln);
        o += ln;
      }
      deps_bag[o] = 0;
    }
    o = 0;
    for (j = 0; j < nknob; j++) {
      size_t ln = strlen(knobs[j]);
      if (j > 0 && o + 1 < sizeof knobs_bag) knobs_bag[o++] = '\n';
      if (o + ln < sizeof knobs_bag) {
        memcpy(knobs_bag + o, knobs[j], ln);
        o += ln;
      }
      knobs_bag[o] = 0;
    }
    o = 0;
    for (j = 0; j < nhead; j++) {
      size_t ln = strlen(head[j]);
      if (j > 0 && o + 1 < sizeof head_bag) head_bag[o++] = '\n';
      if (o + ln < sizeof head_bag) {
        memcpy(head_bag + o, head[j], ln);
        o += ln;
      }
      head_bag[o] = 0;
    }
    /* cubalc.recipe.v1 plate — dual of CLI cubalc recipe */
    {
      char path_esc[900];
      size_t pi = 0, qi = 0, po = 0;
      for (pi = 0; path[pi] && qi + 2 < sizeof path_esc; pi++) {
        if (path[pi] == '\\' || path[pi] == '"') {
          path_esc[qi++] = '\\';
          path_esc[qi++] = path[pi];
        } else if ((unsigned char)path[pi] >= 32) {
          path_esc[qi++] = path[pi];
        }
      }
      path_esc[qi] = 0;
      po = (size_t)snprintf(plate, sizeof plate,
               "{\"schema\":\"cubalc.recipe.v1\",\"ok\":true,\"cmd\":\"recipe\","
               "\"stem\":\"%s\",\"path\":\"%s\",\"bytes\":%ld,\"lines\":%d,"
               "\"deps_n\":%d,\"defaults_n\":%d,\"version\":\"%s\","
               "\"note\":\"in-lang dual of cubalc recipe · LIBINFO+LIBDEPS+LIBDEFAULTS+HEADLIB\","
               "\"deps\":[",
               base, path_esc, (long)nr, nlines, ndep, nknob, CUBALC_LANG_VERSION);
      if (po >= sizeof plate) po = sizeof plate - 1;
      for (j = 0; j < ndep && po + 4 < sizeof plate; j++) {
        if (j) plate[po++] = ',';
        plate[po++] = '"';
        {
          size_t a;
          for (a = 0; deps[j][a] && po + 2 < sizeof plate; a++) {
            if (deps[j][a] == '\\' || deps[j][a] == '"') {
              plate[po++] = '\\';
              if (po + 1 < sizeof plate) plate[po++] = deps[j][a];
            } else if ((unsigned char)deps[j][a] >= 32) {
              plate[po++] = deps[j][a];
            }
          }
        }
        if (po + 1 < sizeof plate) plate[po++] = '"';
      }
      if (po + 16 < sizeof plate) {
        memcpy(plate + po, "],\"defaults\":[", 14);
        po += 14;
      }
      for (j = 0; j < nknob && po + 4 < sizeof plate; j++) {
        if (j) plate[po++] = ',';
        plate[po++] = '"';
        {
          size_t a;
          for (a = 0; knobs[j][a] && po + 2 < sizeof plate; a++) {
            if (knobs[j][a] == '\\' || knobs[j][a] == '"') {
              plate[po++] = '\\';
              if (po + 1 < sizeof plate) plate[po++] = knobs[j][a];
            } else if ((unsigned char)knobs[j][a] >= 32) {
              plate[po++] = knobs[j][a];
            }
          }
        }
        if (po + 1 < sizeof plate) plate[po++] = '"';
      }
      if (po + 12 < sizeof plate) {
        memcpy(plate + po, "],\"head\":[", 10);
        po += 10;
      }
      for (j = 0; j < nhead && po + 4 < sizeof plate; j++) {
        if (j) plate[po++] = ',';
        plate[po++] = '"';
        {
          size_t a;
          for (a = 0; head[j][a] && po + 2 < sizeof plate; a++) {
            if (head[j][a] == '\\' || head[j][a] == '"') {
              plate[po++] = '\\';
              if (po + 1 < sizeof plate) plate[po++] = head[j][a];
            } else if (head[j][a] == '\t') {
              plate[po++] = ' ';
            } else if ((unsigned char)head[j][a] >= 32) {
              plate[po++] = head[j][a];
            }
          }
        }
        if (po + 1 < sizeof plate) plate[po++] = '"';
      }
      if (po + 2 < sizeof plate) {
        plate[po++] = ']';
        plate[po++] = '}';
      }
      plate[po] = 0;
    }
    var_set_str(vm, "LAST", plate);
    var_set_str(vm, "RECIPE", plate);
    var_set_str(vm, "LIBCARD", plate);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
    vm->last_n = ndep;
    var_set_num(vm, "LAST_N", ndep);
    var_set_num(vm, "LIB_BYTES", (long)nr);
    var_set_num(vm, "LIB_SIZE", (long)nr);
    var_set_num(vm, "LIB_LINES", nlines);
    var_set_num(vm, "LIB_DEPS_N", ndep);
    var_set_num(vm, "RECIPE_DEPS_N", ndep);
    var_set_num(vm, "RECIPE_DEFAULTS_N", nknob);
    var_set_num(vm, "RECIPE_HEAD_N", nhead);
    var_set_str(vm, "RECIPE_DEPS", deps_bag);
    var_set_str(vm, "RECIPE_DEFAULTS", knobs_bag);
    var_set_str(vm, "RECIPE_HEAD", head_bag);
    var_set_str(vm, "LIB_PATH", path);
    var_set_str(vm, "LIB_STEM", base);
    var_set_num(vm, "OK", 1);
    bump(vm);
    return 1;
  }

  return 0;
}
