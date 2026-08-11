/* CubalC lang — lang_parse.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"
#include <string.h>
#include <stdio.h>

/* Case-fold ASCII for suggestion matching. */
static void form_fold(char *dst, size_t n, const char *src){
  size_t i;
  for (i = 0; i + 1 < n && src[i]; i++) {
    char c = src[i];
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    dst[i] = c;
  }
  dst[i] = 0;
}

/* Tiny Levenshtein (capped lengths) for agent typo recovery. */
static int form_edit_dist(const char *a, const char *b){
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

/* Curated high-frequency forms agents mistype — not full ISA soup. */
static const char *const form_suggest_pool[] = {
  "CLASS", "NEW", "TRYNEW", "ENSURENEW", "SEND", "TRYSEND", "GETF", "SETF", "TRYGETF", "TRYSETF",
  "HASOBJ", "HASOBJS", "NEEDOBJS", "HASOBJANY", "NEEDOBJANY", "HASFN", "HASFNS", "NEEDFNS", "HASCLASS", "HASCLASSES", "NEEDCLASSES", "HASCLASSANY", "NEEDCLASSANY", "BINDTHIS", "HASTHIS", "ISCLASS", "NEEDISA", "HASFIELD", "HASFIELDS", "NEEDFIELDS", "HASFIELDANY", "NEEDFIELDANY", "HASMETHOD", "HASMETHODS", "NEEDMETHODS", "HASMETHODANY", "NEEDMETHODANY", "LISTOBJS", "LISTFIELDS", "LISTMETHODS",
  "LISTCLASSES", "EACH", "INCLUDE", "ASSERT", "EXPECT", "PRINT", "PRINT_JSON",
  "VARS", "STATUS", "IDENTITY", "VERSION", "REQUIRE", "DEFAULT", "DEFINED",
  "TYPEOF", "UNSET", "NOTE", "EXIT", "CLEAR_ERR", "FAIL", "PASS", "HELP",
  "PUSHF", "POPF", "POPHEADF", "UNSHIFTF", "GREPF", "SORTBAGF", "LINESF",
  "HASBAGLINE", "DROPBAGLINE", "ENSUREBAGLINE", "LINEF", "CUTF", "SPLITF",
  "JOINF", "WORDSF", "INCF", "CATF", "REPLACEF", "NUMF", "STRF",
  "HOLD_FLASH", "CUBE", "PLUG", "FLOW", "IMPULSE", "SYS", "SMX", "LET",
  "IF", "LOOP", "WHILE", "FOR", "FN", "CALL", "TRYCALL", "DUMPOBJ",
  "TRYCLONE", "ENSURECLONE", "CLONEOBJ", "TRYRENAME", "RENAMEOBJ", "TRYSWAP", "SWAPOBJ", "TRYDELETE", "DELETEOBJ", "SENDALL", "GETFALL", "SETFALL", "WHEREOBJ",
  NULL
};

/* Fill out with "did you mean NAME" (or empty). Prefer prefix, then edit dist ≤2. */
static void form_suggest(const char *typo, char *out, size_t outn){
  char want[64], cand[64];
  int i, best_d = 99, d;
  const char *best = NULL;
  out[0] = 0;
  if (!typo || !typo[0] || outn < 8) return;
  form_fold(want, sizeof want, typo);
  if (!want[0]) return;
  /* exact case-insensitive match shouldn't happen (unknown form) */
  for (i = 0; form_suggest_pool[i]; i++) {
    form_fold(cand, sizeof cand, form_suggest_pool[i]);
    if (strcmp(want, cand) == 0) {
      snprintf(out, outn, "%s", form_suggest_pool[i]);
      return;
    }
  }
  /* strong prefix: typo is prefix of form or form is prefix of typo (len≥3) */
  if (strlen(want) >= 3) {
    for (i = 0; form_suggest_pool[i]; i++) {
      form_fold(cand, sizeof cand, form_suggest_pool[i]);
      if (strncmp(want, cand, strlen(want)) == 0 ||
          strncmp(cand, want, strlen(cand)) == 0) {
        if (!best || strlen(cand) < strlen(best))
          best = form_suggest_pool[i];
      }
    }
    if (best) {
      snprintf(out, outn, "%s", best);
      return;
    }
  }
  /* edit distance */
  for (i = 0; form_suggest_pool[i]; i++) {
    form_fold(cand, sizeof cand, form_suggest_pool[i]);
    d = form_edit_dist(want, cand);
    if (d < best_d) {
      best_d = d;
      best = form_suggest_pool[i];
    }
  }
  if (best && best_d <= 2)
    snprintf(out, outn, "%s", best);
}

int cubalc_lang_parse_form(VM *vm, Lex *L){
  skip_nl(L);
  if (L->cur.kind==TK_EOF) return 0;

  int r;
  if ((r = cubalc_lang_ops_core(vm, L)) != 0) return r;
  if ((r = cubalc_lang_ops_smx(vm, L)) != 0) return r;
  if ((r = cubalc_lang_ops_toc(vm, L)) != 0) return r;
  if ((r = cubalc_lang_ops_stack(vm, L)) != 0) return r;
  if ((r = cubalc_lang_ops_dual(vm, L)) != 0) return r;
  if ((r = cubalc_lang_ops_math(vm, L)) != 0) return r;
  if ((r = cubalc_lang_ops_bit(vm, L)) != 0) return r;
  if ((r = cubalc_lang_ops_cell(vm, L)) != 0) return r;
  if ((r = cubalc_lang_ops_flow(vm, L)) != 0) return r;

  {
    char sug[48], ebuf[160];
    form_suggest(L->cur.text, sug, sizeof sug);
    /* ebuf separate from vm->err: fail() snprintf(vm->err, msg) must not overlap. */
    if (sug[0]) {
      snprintf(ebuf, sizeof ebuf,
               "unknown form '%s' line %d — did you mean %s? (HELP / cubalc forms)",
               L->cur.text, L->cur.line, sug);
    } else {
      snprintf(ebuf, sizeof ebuf,
               "unknown form '%s' line %d — try HELP or cubalc forms [prefix]",
               L->cur.text, L->cur.line);
    }
    fail(vm, ebuf);
  }
  return -1;
}
