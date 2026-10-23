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
