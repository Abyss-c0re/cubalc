/* Schedule plate. Bands and notes are data a host scans later.
 * SLEEP executes time. This file does not.
 */
#include "lang/cubalc_lang_internal.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define SCHED_NAME_MAX 15
#define SCHED_BANDS_MAX 8
#define SCHED_NOTES_MAX 16
#define SCHED_KIND_BAND 1
#define SCHED_KIND_NOTE 2

typedef struct {
  int kind;
  char name[SCHED_NAME_MAX + 1];
  long sus0, sus1, duty0, duty1, gap0, gap1, twin, sheet;
  long hz, ms;
} SchedItem;

typedef struct {
  int open;
  char name[SCHED_NAME_MAX + 1];
  int n_bands;
  int n_notes;
  int n;
  SchedItem items[SCHED_BANDS_MAX + SCHED_NOTES_MAX];
} Sched;

static Sched g_sched;

static void publish_n(VM *vm, long n) {
  var_set_num(vm, "LAST_N", n);
  vm->last_n = n;
  var_set_num(vm, "OK", 1);
  bump(vm);
}

static long sched_count(void) {
  return (long)g_sched.n_bands + (long)g_sched.n_notes;
}

static void publish_count(VM *vm) {
  long n = sched_count();
  var_set_num(vm, "SCHED_N", n);
  publish_n(vm, n);
}

/* NOTE hz ms is a schedule note. NOTE text / NOTE ident stays a breadcrumb. */
int cubalc_lang_sched_note_ahead(Lex *L) {
  Lex saved;
  if (!L || !kw(&L->cur, "NOTE")) return 0;
  saved = *L;
  lex_next(L);
  if (L->cur.kind == TK_NL || L->cur.kind == TK_EOF || L->cur.kind == TK_STR) {
    *L = saved;
    return 0;
  }
  if (L->cur.kind == TK_IDENT) {
    lex_next(L);
    if (L->cur.kind == TK_NL || L->cur.kind == TK_EOF) {
      *L = saved;
      return 0;
    }
  }
  *L = saved;
  return 1;
}

static long arg_expr(VM *vm, Lex *L, const char *what) {
  if (L->cur.kind == TK_NL || L->cur.kind == TK_EOF) {
    fail_at(vm, L, what);
    return 0;
  }
  return parse_expr(vm, L);
}

static int take_name(VM *vm, Lex *L, char *id, const char *what) {
  if (L->cur.kind != TK_IDENT || !L->cur.text[0] ||
      strlen(L->cur.text) > SCHED_NAME_MAX) {
    fail_at(vm, L, what);
    return -1;
  }
  snprintf(id, SCHED_NAME_MAX + 1, "%s", L->cur.text);
  lex_next(L);
  return 0;
}

static int need_between(VM *vm, Lex *L, long v, long lo, long hi, const char *what) {
  if (v < lo || v > hi) {
    fail_at(vm, L, what);
    return -1;
  }
  return 0;
}

/* Quoted path and bare ident go through resolve_str_arg.
 * A slash-split path (/tmp/a.txt, state/a.txt) is glued back. One level only.
 */
static int take_path(VM *vm, Lex *L, char *out, size_t outn) {
  size_t o = 0;
  if (L->cur.kind == TK_STR)
    return resolve_str_arg(vm, L, out, outn);
  if (L->cur.kind == TK_IDENT) {
    Lex saved = *L;
    int slash;
    lex_next(L);
    slash = (L->cur.kind == TK_SLASH);
    *L = saved;
    if (!slash)
      return resolve_str_arg(vm, L, out, outn);
  } else if (L->cur.kind != TK_SLASH) {
    return -1;
  }
  out[0] = 0;
  while (L->cur.kind == TK_SLASH || L->cur.kind == TK_IDENT ||
         L->cur.kind == TK_NUM) {
    const char *piece = (L->cur.kind == TK_SLASH) ? "/" : L->cur.text;
    size_t n = strlen(piece);
    if (o + n >= outn) return -1;
    memcpy(out + o, piece, n);
    o += n;
    out[o] = 0;
    lex_next(L);
  }
  return o ? 0 : -1;
}

static int ensure_parent(const char *path) {
  char buf[1024];
  char *slash;
  struct stat st;
  size_t n;
  if (!path || !path[0]) return -1;
  n = strlen(path);
  if (n >= sizeof buf) return -1;
  memcpy(buf, path, n + 1);
  slash = strrchr(buf, '/');
  if (!slash || slash == buf) return 0;
  *slash = 0;
  if (mkdir(buf, 0755) == 0) return 0;
  if (errno == EEXIST && stat(buf, &st) == 0 && S_ISDIR(st.st_mode)) return 0;
  return -1;
}

static int write_plate(const char *path) {
  FILE *f;
  int i;
  if (ensure_parent(path) != 0) return -1;
  f = fopen(path, "w");
  if (!f) return -1;
  if (fprintf(f, "MSCHED 1\n") < 0) goto bad;
  if (fprintf(f, "NAME %s\n", g_sched.name) < 0) goto bad;
  for (i = 0; i < g_sched.n; i++) {
    const SchedItem *it = &g_sched.items[i];
    int n;
    if (it->kind == SCHED_KIND_BAND)
      n = fprintf(f, "BAND %s %ld %ld %ld %ld %ld %ld %ld %ld\n",
                  it->name, it->sus0, it->sus1, it->duty0, it->duty1,
                  it->gap0, it->gap1, it->twin, it->sheet);
    else
      n = fprintf(f, "NOTE %ld %ld\n", it->hz, it->ms);
    if (n < 0) goto bad;
  }
  if (fprintf(f, "END\n") < 0) goto bad;
  if (fclose(f) != 0) {
    remove(path);
    return -1;
  }
  return 0;
bad:
  fclose(f);
  remove(path);
  return -1;
}

int cubalc_lang_ops_sched(VM *vm, Lex *L) {
  skip_nl(L);
  if (L->cur.kind == TK_EOF) return 0;

  if (kw(&L->cur, "SCHED")) {
    char name[SCHED_NAME_MAX + 1];
    lex_next(L);
    if (take_name(vm, L, name, "SCHED name") < 0) return -1;
    memset(&g_sched, 0, sizeof g_sched);
    snprintf(g_sched.name, sizeof g_sched.name, "%s", name);
    g_sched.open = 1;
    var_set_str(vm, "SCHED_NAME", g_sched.name);
    var_set_num(vm, "SCHED_N", 0);
    publish_n(vm, 0);
    return 1;
  }

  if (kw(&L->cur, "BAND")) {
    static const char *what =
        "BAND name sus0 sus1 duty0 duty1 gap0 gap1 twin sheet";
    SchedItem it;
    long v[8];
    int i;
    lex_next(L);
    memset(&it, 0, sizeof it);
    if (take_name(vm, L, it.name, what) < 0) return -1;
    for (i = 0; i < 8; i++) {
      v[i] = arg_expr(vm, L, what);
      if (vm->fatal) return -1;
    }
    it.kind = SCHED_KIND_BAND;
    it.sus0 = v[0];
    it.sus1 = v[1];
    it.duty0 = v[2];
    it.duty1 = v[3];
    it.gap0 = v[4];
    it.gap1 = v[5];
    it.twin = v[6];
    it.sheet = v[7];
    if (need_between(vm, L, it.sus0, 0, 60000, "BAND time 0..60000") < 0)
      return -1;
    if (need_between(vm, L, it.sus1, 0, 60000, "BAND time 0..60000") < 0)
      return -1;
    if (need_between(vm, L, it.gap0, 0, 60000, "BAND time 0..60000") < 0)
      return -1;
    if (need_between(vm, L, it.gap1, 0, 60000, "BAND time 0..60000") < 0)
      return -1;
    if (need_between(vm, L, it.duty0, 0, 255, "BAND duty 0..255") < 0)
      return -1;
    if (need_between(vm, L, it.duty1, 0, 255, "BAND duty 0..255") < 0)
      return -1;
    if (need_between(vm, L, it.twin, 0, 100, "BAND twin 0..100") < 0)
      return -1;
    if (need_between(vm, L, it.sheet, 0, 100, "BAND sheet 0..100") < 0)
      return -1;
    if (it.sus0 > it.sus1) {
      fail_at(vm, L, "BAND sus0<=sus1");
      return -1;
    }
    if (it.duty0 > it.duty1) {
      fail_at(vm, L, "BAND duty0<=duty1");
      return -1;
    }
    if (it.gap0 > it.gap1) {
      fail_at(vm, L, "BAND gap0<=gap1");
      return -1;
    }
    if (!g_sched.open) {
      fail_at(vm, L, "BAND needs SCHED");
      return -1;
    }
    if (g_sched.n_bands >= SCHED_BANDS_MAX) {
      fail_at(vm, L, "BAND full");
      return -1;
    }
    g_sched.items[g_sched.n++] = it;
    g_sched.n_bands++;
    publish_count(vm);
    return 1;
  }

  if (kw(&L->cur, "NOTE")) {
    long hz, ms;
    SchedItem it;
    if (!cubalc_lang_sched_note_ahead(L)) return 0;
    lex_next(L);
    hz = arg_expr(vm, L, "NOTE hz ms");
    if (vm->fatal) return -1;
    ms = arg_expr(vm, L, "NOTE hz ms");
    if (vm->fatal) return -1;
    if (hz < 0 || hz > 20000) {
      fail_at(vm, L, "NOTE hz 0..20000");
      return -1;
    }
    if (ms < 1 || ms > 60000) {
      fail_at(vm, L, "NOTE ms 1..60000");
      return -1;
    }
    if (!g_sched.open) {
      fail_at(vm, L, "NOTE needs SCHED");
      return -1;
    }
    if (g_sched.n_notes >= SCHED_NOTES_MAX) {
      fail_at(vm, L, "NOTE full");
      return -1;
    }
    memset(&it, 0, sizeof it);
    it.kind = SCHED_KIND_NOTE;
    it.hz = hz;
    it.ms = ms;
    g_sched.items[g_sched.n++] = it;
    g_sched.n_notes++;
    publish_count(vm);
    return 1;
  }

  if (kw(&L->cur, "SCHEDOUT")) {
    char path[1024];
    long n;
    const char *st;
    int nw;
    lex_next(L);
    path[0] = 0;
    if (!g_sched.open) {
      fail_at(vm, L, "SCHEDOUT needs SCHED");
      return -1;
    }
    if (L->cur.kind == TK_NL || L->cur.kind == TK_EOF) {
      st = getenv("CUBALC_STATE");
      if (!st || !st[0]) st = "state";
      nw = snprintf(path, sizeof path, "%s/cubalc_schedule.txt", st);
      if (nw < 0 || (size_t)nw >= sizeof path) {
        fail_at(vm, L, "SCHEDOUT path");
        return -1;
      }
    } else if (take_path(vm, L, path, sizeof path) != 0 || !path[0]) {
      fail_at(vm, L, "SCHEDOUT path");
      return -1;
    }
    if (write_plate(path) != 0) {
      fail(vm, "SCHEDOUT could not write plate");
      return -1;
    }
    n = sched_count();
    var_set_str(vm, "LAST", path);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
    var_set_num(vm, "SCHED_N", n);
    publish_n(vm, n);
    return 1;
  }

  return 0;
}
