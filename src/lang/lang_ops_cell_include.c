/* CubalC lang — lang_ops_cell_include.c
 * CELL INCLUDE/IMPORT/USE monolit peel (G_cell_margin).
 * Mesh vote G_cell · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_cell_include.h"
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

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

int cubalc_lang_ops_cell_try_include(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* INCLUDE "path"|name — practical modules (same VM / world)
   * Source buffer is retained until run end so FN/CLASS METHOD bodies
   * from lib files remain valid (pointers into the included text).
   * Resolve: absolute · include_base/rel · rel · programs/rel ·
   * programs/lib/<name>[.cubalc] short form · CUBALC_ROOT · fail with tried paths.
   * Usability: INCLUDE hold_seed  or  INCLUDE "hold_seed" → programs/lib/…
   * String-var / LAST: INCLUDE lib  after PICKLIB / LET lib = "agent_boot".
   * Filter: INCLUDE MATCH|FIRST|NTH|LASTMATCH|ALL MATCH needle [DEFAULT stem].
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
    char path[768];
    char orig[512];
    orig[0] = 0;
    path[0] = 0;
    /* INCLUDE ALL|EVERY MATCH needle [DEFAULT stem]
     * Usability: bulk-include every matching lib · no EACH LIB + INCLUDE glue.
     * Always ONCE-ish per stem in loop (skip already loaded). Sorted order. */
    if (kw(&L->cur,"ALL") || kw(&L->cur,"EVERY") || kw(&L->cur,"MATCHALL") ||
        kw(&L->cur,"ALLMATCH") || kw(&L->cur,"INCLUDEALL") ||
        kw(&L->cur,"ALLLIBS") || kw(&L->cur,"BULKMATCH")) {
      char needle[96], fup[96], fb[96];
      char stems[96][96];
      char bag[4096];
      int nstem = 0, i, j, have_fb = 0, n_ok = 0, n_skip = 0;
      size_t a, o = 0;
      DIR *d;
      struct dirent *ent;
      const char *ip;
      if (kw(&L->cur,"ALL") || kw(&L->cur,"EVERY") || kw(&L->cur,"ALLLIBS"))
        lex_next(L);
      else
        lex_next(L); /* MATCHALL / ALLMATCH / … */
      if (kw(&L->cur,"MATCH") || kw(&L->cur,"FILTER") || kw(&L->cur,"LIBS") ||
          kw(&L->cur,"OF") || kw(&L->cur,"WITH"))
        lex_next(L);
      needle[0] = 0;
      fb[0] = 0;
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
      if (kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") || kw(&L->cur,"FALLBACK") ||
          kw(&L->cur,"ORDEFAULT")) {
        lex_next(L);
        if (L->cur.kind == TK_STR) {
          snprintf(fb, sizeof fb, "%s", L->cur.text);
          have_fb = 1;
          lex_next(L);
        } else if (L->cur.kind == TK_IDENT) {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
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
        fail_at(vm, L,
                "INCLUDE ALL MATCH needs needle — INCLUDE ALL MATCH guard · DEFAULT hold_seed");
        return -1;
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
      if (nstem == 0 && have_fb) {
        snprintf(stems[0], sizeof stems[0], "%s", fb);
        nstem = 1;
        var_set_num(vm, "INCLUDE_ALL_FALLBACK", 1);
      }
      if (nstem == 0) {
        char msg[192];
        snprintf(msg, sizeof msg,
                 "INCLUDE ALL MATCH miss line %d: '%s' — MATCHLIBS · cubalc libs %s · DEFAULT stem",
                 aln, needle, needle);
        if (soft) {
          var_set_str(vm, "ERR", msg);
          var_set_str(vm, "LAST_ERR", msg);
          var_set_str(vm, "INCLUDE_MISS", needle);
          var_set_str(vm, "INCLUDE_ALL", "");
          var_set_num(vm, "INCLUDE_ALL_N", 0);
          var_set_str(vm, "LAST", needle);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", needle);
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "OK", 0);
          var_set_num(vm, "INCLUDE_OK", 0);
          bump(vm);
          return 1;
        }
        var_set_str(vm, "ERR", msg);
        var_set_str(vm, "LAST_ERR", msg);
        fail(vm, msg);
        return -1;
      }
      /* bulk load each stem (short-name resolve · skip already loaded) */
      for (i = 0; i < nstem && !vm->fatal && !vm->halt; i++) {
        char pth[768];
        FILE *f = NULL;
        int k, hit = 0;
        snprintf(pth, sizeof pth, "programs/lib/%s.cubalc", stems[i]);
        f = fopen(pth, "rb");
        if (!f) {
          snprintf(pth, sizeof pth, "programs/lib/%s", stems[i]);
          f = fopen(pth, "rb");
        }
        if (!f) {
          const char *ip2 = getenv("CUBALC_INCLUDE_PATH");
          if (ip2 && ip2[0]) {
            const char *seg = ip2;
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
              snprintf(p3, sizeof p3, "%s/%s.cubalc", dir, stems[i]);
              f = fopen(p3, "rb");
              if (f) { snprintf(pth, sizeof pth, "%s", p3); break; }
            }
          }
        }
        if (!f) {
          if (soft) continue;
          {
            char msg[160];
            snprintf(msg, sizeof msg,
                     "INCLUDE ALL MATCH cannot open '%s' line %d", stems[i], aln);
            fail(vm, msg);
            return -1;
          }
        }
        for (k = 0; k < vm->n_included; k++) {
          if (strcmp(vm->included[k], pth) == 0) { hit = 1; break; }
        }
        if (hit || once) {
          /* always skip reloads in bulk (ONCE semantics) */
          if (hit) {
            fclose(f);
            n_skip++;
            continue;
          }
        }
        {
          long sz;
          char *buf;
          size_t nr;
          char save_base[512];
          Lex Li;
          int rc;
          fseek(f, 0, SEEK_END);
          sz = ftell(f);
          fseek(f, 0, SEEK_SET);
          if (sz < 0 || sz > CUBALC_MAX_SRC) {
            fclose(f);
            fail(vm, "INCLUDE ALL too large");
            return -1;
          }
          buf = malloc((size_t)sz + 1);
          if (!buf) { fclose(f); fail(vm, "oom"); return -1; }
          nr = fread(buf, 1, (size_t)sz, f);
          fclose(f);
          buf[nr] = 0;
          snprintf(save_base, sizeof save_base, "%s", vm->include_base);
          {
            char *sl = strrchr(pth, '/');
            if (sl) {
              size_t n = (size_t)(sl - pth);
              if (n >= sizeof vm->include_base) n = sizeof vm->include_base - 1;
              memcpy(vm->include_base, pth, n);
              vm->include_base[n] = 0;
            }
          }
          if (vm->n_included < 24) {
            snprintf(vm->included[vm->n_included], sizeof vm->included[0], "%s", pth);
            vm->n_included++;
          }
          lex_init(&Li, buf, nr);
          rc = exec_stmts_until(vm, &Li, NULL, NULL);
          snprintf(vm->include_base, sizeof vm->include_base, "%s", save_base);
          if (vm->n_include_bufs < 24)
            vm->include_bufs[vm->n_include_bufs++] = buf;
          else
            free(buf);
          if (rc < 0) return -1;
          n_ok++;
          {
            size_t ln = strlen(stems[i]);
            if (o > 0 && o + 1 < sizeof bag) bag[o++] = '\n';
            if (o + ln < sizeof bag) {
              memcpy(bag + o, stems[i], ln);
              o += ln;
            }
            bag[o] = 0;
          }
        }
      }
      var_set_str(vm, "INCLUDE_ALL", bag);
      var_set_str(vm, "INCLUDE_MATCH", bag[0] ? bag : "");
      var_set_str(vm, "MATCHLIBS_FILTER", needle);
      var_set_str(vm, "INCLUDE_MATCH_FILTER", needle);
      var_set_num(vm, "INCLUDE_ALL_N", n_ok);
      var_set_num(vm, "INCLUDE_ALL_SKIP", n_skip);
      var_set_num(vm, "MATCHLIBS_N", nstem);
      var_set_str(vm, "INCLUDE_MISS", "");
      var_set_str(vm, "INCLUDE_SUGGEST", "");
      var_set_str(vm, "LAST", bag);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
      vm->last_n = n_ok;
      var_set_num(vm, "LAST_N", n_ok);
      var_set_num(vm, "OK", 1);
      var_set_num(vm, "INCLUDE_OK", 1);
      var_set_num(vm, "INCLUDE_SKIPPED", n_skip > 0 ? 1 : 0);
      bump(vm);
      return 1;
    }
    /* INCLUDE MATCH|FIRST|PICK | NTH idx | LASTMATCH|ENDMATCH | NEWEST|OLDEST needle [DEFAULT stem]
     * Usability: filter pick family → INCLUDE without PICKLIB/NTHLIB/NEWESTLIB+LAST glue. */
    if (kw(&L->cur,"MATCH") || kw(&L->cur,"FIRST") || kw(&L->cur,"PICK") ||
        kw(&L->cur,"MATCHFIRST") || kw(&L->cur,"FIRSTMATCH") ||
        kw(&L->cur,"PICKMATCH") || kw(&L->cur,"MATCHLIB") ||
        kw(&L->cur,"NTH") || kw(&L->cur,"NTHMATCH") || kw(&L->cur,"MATCHNTH") ||
        kw(&L->cur,"INCLUDENTH") ||
        kw(&L->cur,"LASTMATCH") || kw(&L->cur,"ENDMATCH") ||
        kw(&L->cur,"MATCHLAST") || kw(&L->cur,"LASTLIBINC") ||
        kw(&L->cur,"INCLASTMATCH") ||
        kw(&L->cur,"NEWEST") || kw(&L->cur,"LATEST") || kw(&L->cur,"FRESH") ||
        kw(&L->cur,"INCLUDENEWEST") || kw(&L->cur,"MATCHNEWEST") ||
        kw(&L->cur,"OLDEST") || kw(&L->cur,"EARLIEST") || kw(&L->cur,"STALE") ||
        kw(&L->cur,"INCLUDEOLDEST") || kw(&L->cur,"MATCHOLDEST")) {
      char needle[96], fup[96], fb[96];
      char stems[96][96];
      char stem_paths[96][768];
      int nstem = 0, i, j, have_fb = 0;
      int mode = 1; /* 1=first 2=last 3=nth 4=newest 5=oldest */
      int want_idx = 0, have_idx = 0;
      size_t a;
      DIR *d;
      struct dirent *ent;
      const char *ip;
      if (kw(&L->cur,"LASTMATCH") || kw(&L->cur,"ENDMATCH") ||
          kw(&L->cur,"MATCHLAST") || kw(&L->cur,"LASTLIBINC") ||
          kw(&L->cur,"INCLASTMATCH"))
        mode = 2;
      else if (kw(&L->cur,"NTH") || kw(&L->cur,"NTHMATCH") ||
               kw(&L->cur,"MATCHNTH") || kw(&L->cur,"INCLUDENTH"))
        mode = 3;
      else if (kw(&L->cur,"NEWEST") || kw(&L->cur,"LATEST") || kw(&L->cur,"FRESH") ||
               kw(&L->cur,"INCLUDENEWEST") || kw(&L->cur,"MATCHNEWEST"))
        mode = 4;
      else if (kw(&L->cur,"OLDEST") || kw(&L->cur,"EARLIEST") || kw(&L->cur,"STALE") ||
               kw(&L->cur,"INCLUDEOLDEST") || kw(&L->cur,"MATCHOLDEST"))
        mode = 5;
      lex_next(L);
      needle[0] = 0;
      fb[0] = 0;
      /* NTH: index then needle */
      if (mode == 3) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
            L->cur.kind == TK_MINUS) {
          want_idx = (int)parse_expr(vm, L);
          have_idx = 1;
        } else if (L->cur.kind == TK_IDENT) {
          Var *vi = var_get(vm, L->cur.text, 0);
          if (vi && !vi->is_str) {
            want_idx = (int)vi->val;
            have_idx = 1;
            lex_next(L);
          }
        }
        if (!have_idx) {
          fail_at(vm, L,
                  "INCLUDE NTH needs index — INCLUDE NTH 1 plate · DEFAULT agent_boot");
          return -1;
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
      if (kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") || kw(&L->cur,"FALLBACK") ||
          kw(&L->cur,"ORDEFAULT")) {
        lex_next(L);
        if (L->cur.kind == TK_STR) {
          snprintf(fb, sizeof fb, "%s", L->cur.text);
          have_fb = 1;
          lex_next(L);
        } else if (L->cur.kind == TK_IDENT) {
          Var *vv = var_get(vm, L->cur.text, 0);
          if (vv && vv->is_str && vv->sval[0])
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
        fail_at(vm, L,
                "INCLUDE MATCH needs needle — INCLUDE MATCH plate · INCLUDE NTH 1 plate · INCLUDE LASTMATCH plate");
        return -1;
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
            snprintf(stems[nstem], sizeof stems[0], "%s", stem);
            snprintf(stem_paths[nstem], sizeof stem_paths[0], "%s/%s",
                     dirs[di], ent->d_name);
            nstem++;
          }
          closedir(d);
        }
      }
      /* alpha sort keeps path parallel for first/last/nth modes */
      for (i = 1; i < nstem; i++) {
        char tmp[96], tp[768];
        snprintf(tmp, sizeof tmp, "%s", stems[i]);
        snprintf(tp, sizeof tp, "%s", stem_paths[i]);
        j = i;
        while (j > 0 && strcmp(stems[j - 1], tmp) > 0) {
          snprintf(stems[j], sizeof stems[0], "%s", stems[j - 1]);
          snprintf(stem_paths[j], sizeof stem_paths[0], "%s", stem_paths[j - 1]);
          j--;
        }
        snprintf(stems[j], sizeof stems[0], "%s", tmp);
        snprintf(stem_paths[j], sizeof stem_paths[0], "%s", tp);
      }
      {
        int pick_i = 0;
        int ok_pick = 0;
        if (mode == 4 || mode == 5) {
          /* mtime pick among matches */
          long best_mt = 0;
          int have_best = 0;
          for (i = 0; i < nstem; i++) {
            struct stat st;
            if (stat(stem_paths[i], &st) != 0) continue;
            if (!have_best ||
                (mode == 5 ? (st.st_mtime < best_mt) : (st.st_mtime > best_mt)) ||
                (st.st_mtime == best_mt && strcmp(stems[i], stems[pick_i]) < 0)) {
              best_mt = (long)st.st_mtime;
              have_best = 1;
              pick_i = i;
            }
          }
          ok_pick = have_best;
          if (ok_pick) {
            var_set_num(vm, "NEWESTLIB_MTIME", best_mt);
            var_set_num(vm, "LIB_MTIME", best_mt);
          }
        } else if (mode == 2)
          pick_i = nstem > 0 ? nstem - 1 : 0;
        else if (mode == 3)
          pick_i = want_idx;
        else
          pick_i = 0;
        if (!(mode == 4 || mode == 5) && nstem > 0 && pick_i >= 0 && pick_i < nstem)
          ok_pick = 1;
        var_set_num(vm, "MATCHLIBS_N", nstem);
        var_set_num(vm, "PICKLIB_N", nstem);
        var_set_num(vm, "NTHLIB_N", nstem);
        var_set_num(vm, "INCLUDE_MATCH_I", ok_pick ? pick_i : -1);
        var_set_str(vm, "MATCHLIBS_FILTER", needle);
        var_set_str(vm, "INCLUDE_MATCH_FILTER", needle);
        if (ok_pick) {
          snprintf(orig, sizeof orig, "%s", stems[pick_i]);
          var_set_str(vm, "INCLUDE_MATCH", orig);
          var_set_str(vm, "PICKLIB", orig);
          if (mode == 3)
            var_set_str(vm, "NTHLIB", orig);
          if (mode == 2)
            var_set_str(vm, "LASTLIB", orig);
          if (mode == 4)
            var_set_str(vm, "NEWESTLIB", orig);
          if (mode == 5)
            var_set_str(vm, "OLDESTLIB", orig);
        } else if (have_fb) {
          snprintf(orig, sizeof orig, "%s", fb);
          var_set_str(vm, "INCLUDE_MATCH", orig);
          var_set_num(vm, "INCLUDE_MATCH_FALLBACK", 1);
        } else {
          char msg[200];
          if (mode == 3)
            snprintf(msg, sizeof msg,
                     "INCLUDE NTH miss line %d: idx=%d '%s' — MATCHLIBS · DEFAULT stem",
                     aln, want_idx, needle);
          else if (mode == 2)
            snprintf(msg, sizeof msg,
                     "INCLUDE LASTMATCH miss line %d: '%s' — MATCHLIBS · cubalc libs %s · DEFAULT stem",
                     aln, needle, needle);
          else if (mode == 4)
            snprintf(msg, sizeof msg,
                     "INCLUDE NEWEST miss line %d: '%s' — NEWESTLIB · cubalc libs %s · DEFAULT stem",
                     aln, needle, needle);
          else if (mode == 5)
            snprintf(msg, sizeof msg,
                     "INCLUDE OLDEST miss line %d: '%s' — OLDESTLIB · cubalc libs %s · DEFAULT stem",
                     aln, needle, needle);
          else
            snprintf(msg, sizeof msg,
                     "INCLUDE MATCH miss line %d: '%s' — MATCHLIBS · cubalc libs %s · DEFAULT stem",
                     aln, needle, needle);
          if (soft) {
            var_set_str(vm, "ERR", msg);
            var_set_str(vm, "LAST_ERR", msg);
            var_set_str(vm, "INCLUDE_MISS", needle);
            var_set_str(vm, "INCLUDE_MATCH", "");
            var_set_str(vm, "INCLUDE_PATH", "");
            var_set_str(vm, "LAST", needle);
            snprintf(vm->last_str, sizeof vm->last_str, "%s", needle);
            vm->last_n = 0;
            var_set_num(vm, "LAST_N", 0);
            var_set_num(vm, "OK", 0);
            var_set_num(vm, "INCLUDE_OK", 0);
            bump(vm);
            return 1;
          }
          var_set_str(vm, "ERR", msg);
          var_set_str(vm, "LAST_ERR", msg);
          fail(vm, msg);
          return -1;
        }
      }
    } else {
      if (L->cur.kind!=TK_STR && L->cur.kind!=TK_IDENT){
        fail_at(vm,L,"INCLUDE needs path|libname|MATCH|NTH|LASTMATCH|NEWEST|OLDEST — INCLUDE hold_seed"); return -1;
      }
      /* Resolve IDENT as string var / LAST when set — else bare short name (lib stem). */
      if (L->cur.kind == TK_STR) {
        snprintf(orig, sizeof orig, "%s", L->cur.text);
      } else if (strcmp(L->cur.text, "LAST") == 0 && vm->last_str[0]) {
        snprintf(orig, sizeof orig, "%s", vm->last_str);
      } else {
        Var *vv = var_get(vm, L->cur.text, 0);
        if (vv && vv->is_str && vv->sval[0])
          snprintf(orig, sizeof orig, "%s", vv->sval);
        else
          snprintf(orig, sizeof orig, "%s", L->cur.text);
      }
      lex_next(L);
    }
    if (orig[0]=='/' || (vm->include_base[0]==0))
      snprintf(path,sizeof path,"%s",orig);
    else
      snprintf(path,sizeof path,"%s/%s", vm->include_base, orig);
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
  return 0;
}
