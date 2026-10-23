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
#include "lang/cubalc_lang_ops_cell_libcat.h"
#include "lang/cubalc_lang_ops_cell_libpick.h"
#include "lang/cubalc_lang_ops_cell_libread.h"
#include "lang/cubalc_lang_ops_cell_libmeta.h"
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
  /* lib catalog LISTINCLUDES·LISTLIBS·MATCH* → lang_ops_cell_libcat.c (G_cell_margin) */
  {
    int lc = cubalc_lang_ops_cell_try_libcat(vm, L);
    if (lc != 0) return lc;
  }


  /* lib pick PICKLIB·NTHLIB·NEWEST/OLDEST·SORTLIBS → lang_ops_cell_libpick.c (G_cell_libpick) */
  {
    int lp = cubalc_lang_ops_cell_try_libpick(vm, L);
    if (lp != 0) return lp;
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

  /* lib read CATLIB·GREPLIB·HEAD/TAILLIB → lang_ops_cell_libread.c (G_cell_libread) */
  {
    int lr = cubalc_lang_ops_cell_try_libread(vm, L);
    if (lr != 0) return lr;
  }

  /* lib meta LIBDEPS·INFO·TREE·CHECK·DEFAULTS·RECIPE → lang_ops_cell_libmeta.c (G_cell_libmeta) */
  {
    int lm = cubalc_lang_ops_cell_try_libmeta(vm, L);
    if (lm != 0) return lm;
  }

  return 0;
}
