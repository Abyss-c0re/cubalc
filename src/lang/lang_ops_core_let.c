/* CubalC lang — LET · INC/DEC · SWAP · REQUIRE RESTARGS (core var plane) */
#include "lang/cubalc_lang_ops_core_let.h"
#include "cubalc_hostops.h"
#include "lang/cubalc_lang_ops_sys_cli.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cubalc_law.h"
#include "lang/cubalc_lang_ops_core_help.h"

int cubalc_lang_ops_core_try_let(VM *vm, Lex *L) {
  if (kw(&L->cur,"LET")){
    int aln = L->cur.line;
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){
      char ebuf[160];
      snprintf(ebuf, sizeof ebuf,
               "LET needs name [=] value line %d — LET x = 1", aln);
      fail(vm, ebuf); return -1;
    }
    char name[48]; snprintf(name,sizeof name,"%s",L->cur.text); lex_next(L);
    /* '=' optional when next token is a value — LET x 1 same as LET x = 1 */
    if (L->cur.kind==TK_EQ){
      lex_next(L);
    } else if (!(L->cur.kind==TK_NUM || L->cur.kind==TK_STR ||
                 L->cur.kind==TK_MINUS || L->cur.kind==TK_LPAREN ||
                 L->cur.kind==TK_IDENT)){
      char ebuf[160];
      snprintf(ebuf, sizeof ebuf,
               "LET '%s' needs value line %d — LET %s = 1", name, aln, name);
      fail(vm, ebuf); return -1;
    }
    if (L->cur.kind==TK_STR || (L->cur.kind==TK_IDENT && (
          strcmp(L->cur.text,"LAST")==0 ||
          (var_get(vm,L->cur.text,0) && var_get(vm,L->cur.text,0)->is_str)))){
      char buf[CUBALC_HOST_STR_MAX]; buf[0]=0;
      for(;;){
        if (L->cur.kind==TK_STR){
          size_t bl=strlen(buf), al=strlen(L->cur.text);
          if (bl+al+1 < sizeof buf) memcpy(buf+bl, L->cur.text, al+1);
          lex_next(L);
        } else if (L->cur.kind==TK_IDENT){
          if (strcmp(L->cur.text,"LAST")==0){
            size_t bl=strlen(buf), al=strlen(vm->last_str);
            if (bl+al+1 < sizeof buf) memcpy(buf+bl, vm->last_str, al+1);
            lex_next(L);
          } else {
            Var *v = var_get(vm, L->cur.text, 0);
            if (v && v->is_str){
              size_t bl=strlen(buf), al=strlen(v->sval);
              if (bl+al+1 < sizeof buf) memcpy(buf+bl, v->sval, al+1);
            } else if (v){
              char nb[32]; snprintf(nb,sizeof nb,"%ld", v->val);
              size_t bl=strlen(buf), al=strlen(nb);
              if (bl+al+1 < sizeof buf) memcpy(buf+bl, nb, al+1);
            } else break;
            lex_next(L);
          }
        } else break;
        if (L->cur.kind==TK_PLUS){ lex_next(L); continue; }
        break;
      }
      var_set_str(vm, name, buf);
      bump(vm); return 1;
    }
    long v=parse_expr(vm,L);
    var_set_num(vm, name, v);
    bump(vm); return 1;
  }
  /* INC|BUMP|ADDTO name [delta] — numeric var += delta (default 1).
   * DEC|SUBFROM name [delta] — numeric var -= delta (default 1).
   * Creates var as 0 if missing. Str var promoted via strtol (0 if non-num).
   * Usability: counters/retries without LET x = x + 1 glue. */
  if (kw(&L->cur,"INC")||kw(&L->cur,"BUMP")||kw(&L->cur,"ADDTO")||
      kw(&L->cur,"INCR")||kw(&L->cur,"INCREMENT")||
      kw(&L->cur,"DEC")||kw(&L->cur,"SUBFROM")||kw(&L->cur,"DECR")||
      kw(&L->cur,"DECREMENT")||kw(&L->cur,"SUBTO")){
    char op[24];
    int is_dec = 0;
    char name[48];
    long delta = 1, cur = 0, nv;
    Var *vv;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "DEC") == 0 || strcmp(op, "SUBFROM") == 0 ||
        strcmp(op, "DECR") == 0 || strcmp(op, "DECREMENT") == 0 ||
        strcmp(op, "SUBTO") == 0)
      is_dec = 1;
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, is_dec ? "DEC needs name [n] — DEC retries" : "INC needs name [n] — INC retries");
      return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    if (kw(&L->cur, "BY") || kw(&L->cur, "OF") || kw(&L->cur, "WITH"))
      lex_next(L);
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
        (L->cur.kind == TK_IDENT && !kw(&L->cur, "END") && !kw(&L->cur, "IF") &&
         !kw(&L->cur, "LET") && !kw(&L->cur, "ASSERT") && !kw(&L->cur, "PRINT") &&
         !kw(&L->cur, "PASS") && !kw(&L->cur, "FAIL") && !kw(&L->cur, "INC") &&
         !kw(&L->cur, "DEC") && !kw(&L->cur, "SWAP")))
      delta = parse_expr(vm, L);
    if (delta < 0) {
      /* allow negative delta as opposite direction */
    }
    vv = var_get(vm, name, 0);
    if (vv && !vv->is_str) {
      cur = vv->val;
    } else if (vv && vv->is_str) {
      char *end = NULL;
      cur = strtol(vv->sval, &end, 10);
      if (!(end && end != vv->sval && *end == 0))
        cur = 0;
    } else {
      cur = 0;
    }
    nv = is_dec ? (cur - delta) : (cur + delta);
    var_set_num(vm, name, nv);
    var_set_num(vm, "LAST_N", nv);
    vm->last_n = nv;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", nv);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, is_dec ? "DEC_N" : "INC_N", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s -> %ld\n", is_dec ? "DEC" : "INC", name, nv);
    bump(vm);
    return 1;
  }
  /* SWAP|XCHG a [WITH|AND] b — exchange two vars (num or str).
   * Usability: dual buffers without temp LET t = a glue. */
  if (kw(&L->cur,"SWAP")||kw(&L->cur,"XCHG")||kw(&L->cur,"EXCHANGE")||
      kw(&L->cur,"SWAPVARS")||kw(&L->cur,"SWAPV")){
    char a[48], b[48];
    Var *va, *vb;
    int a_str = 0, b_str = 0;
    long a_n = 0, b_n = 0;
    char a_s[CUBALC_HOST_STR_MAX], b_s[CUBALC_HOST_STR_MAX];
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, "SWAP needs two names — SWAP a b");
      return -1;
    }
    snprintf(a, sizeof a, "%s", L->cur.text);
    lex_next(L);
    if (kw(&L->cur, "WITH") || kw(&L->cur, "AND") || kw(&L->cur, "OR") ||
        kw(&L->cur, ",") || kw(&L->cur, "TO"))
      lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, "SWAP needs two names — SWAP a b");
      return -1;
    }
    snprintf(b, sizeof b, "%s", L->cur.text);
    lex_next(L);
    a_s[0] = b_s[0] = 0;
    va = var_get(vm, a, 0);
    vb = var_get(vm, b, 0);
    if (va) {
      if (va->is_str) {
        a_str = 1;
        snprintf(a_s, sizeof a_s, "%s", va->sval);
      } else {
        a_n = va->val;
      }
    }
    if (vb) {
      if (vb->is_str) {
        b_str = 1;
        snprintf(b_s, sizeof b_s, "%s", vb->sval);
      } else {
        b_n = vb->val;
      }
    }
    /* write b's value into a, a's into b */
    if (b_str)
      var_set_str(vm, a, b_s);
    else
      var_set_num(vm, a, b_n);
    if (a_str)
      var_set_str(vm, b, a_s);
    else
      var_set_num(vm, b, a_n);
    var_set_num(vm, "SWAP_N", 1);
    var_set_num(vm, "OK", 1);
    var_set_num(vm, "LAST_N", 1);
    vm->last_n = 1;
    var_set_str(vm, "LAST", "swapped");
    snprintf(vm->last_str, sizeof vm->last_str, "%s", "swapped");
    if (vm->trace)
      fprintf(vm->trace, "# SWAP %s <-> %s\n", a, b);
    bump(vm);
    return 1;
  }
  /* TOGGLE|FLIP name — 0↔1 flag (missing/non-num → 1).
   * Usability: feature flags without IF name == 0 THEN LET name = 1 ELSE LET name = 0. */
  if (kw(&L->cur,"TOGGLE")||kw(&L->cur,"FLIP")||kw(&L->cur,"TOGGLEV")||
      kw(&L->cur,"FLIPVAR")||kw(&L->cur,"FLIPFLAGV")||kw(&L->cur,"NOTV")){
    char name[48];
    long cur = 0, nv;
    Var *vv;
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, "TOGGLE needs name — TOGGLE verbose");
      return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    vv = var_get(vm, name, 0);
    if (vv && !vv->is_str) {
      cur = vv->val;
    } else if (vv && vv->is_str) {
      char *end = NULL;
      cur = strtol(vv->sval, &end, 10);
      if (!(end && end != vv->sval && *end == 0))
        cur = 0; /* non-numeric str → treat as 0 → flip to 1 */
    } else {
      cur = 0; /* missing → 1 */
    }
    nv = (cur == 0) ? 1 : 0;
    var_set_num(vm, name, nv);
    var_set_num(vm, "LAST_N", nv);
    vm->last_n = nv;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", nv);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "TOGGLE_N", nv);
    var_set_num(vm, "FLIP_N", nv);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# TOGGLE %s -> %ld\n", name, nv);
    bump(vm);
    return 1;
  }
  /* CLAMP|BOUND|CLIP name lo [TO] hi — clamp numeric var into [lo,hi] in place.
   * Missing var treated as 0. LAST_N = result · CLAMP_CHANGED 0|1.
   * Usability: INC retries then CLAMP retries 0 5 without LET retries = CLAMPN … glue. */
  if (kw(&L->cur,"CLAMP")||kw(&L->cur,"BOUND")||kw(&L->cur,"CLIP")||
      kw(&L->cur,"CLAMPV")||kw(&L->cur,"BOUNDV")||kw(&L->cur,"CLIPV")||
      kw(&L->cur,"SATURATE")||kw(&L->cur,"SATV")){
    char name[48];
    long lo, hi, cur = 0, nv;
    int changed;
    Var *vv;
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, "CLAMP needs name lo hi — CLAMP retries 0 5");
      return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    if (!cubalc_read_num_atom(vm, L, &lo)) {
      fail(vm, "CLAMP needs name lo hi — CLAMP retries 0 5");
      return -1;
    }
    if (kw(&L->cur, "TO") || kw(&L->cur, "AND") || kw(&L->cur, ",") ||
        kw(&L->cur, "..") || kw(&L->cur, "THROUGH") || kw(&L->cur, "THRU") ||
        kw(&L->cur, "DOTDOT"))
      lex_next(L);
    if (!cubalc_read_num_atom(vm, L, &hi)) {
      fail(vm, "CLAMP needs hi — CLAMP retries 0 5");
      return -1;
    }
    if (lo > hi) { long t = lo; lo = hi; hi = t; }
    vv = var_get(vm, name, 0);
    if (vv && !vv->is_str) {
      cur = vv->val;
    } else if (vv && vv->is_str) {
      char *end = NULL;
      cur = strtol(vv->sval, &end, 10);
      if (!(end && end != vv->sval && *end == 0))
        cur = 0;
    } else {
      cur = 0;
    }
    nv = cur;
    if (nv < lo) nv = lo;
    if (nv > hi) nv = hi;
    changed = (nv != cur) ? 1 : 0;
    var_set_num(vm, name, nv);
    var_set_num(vm, "LAST_N", nv);
    vm->last_n = nv;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", nv);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "CLAMP_N", 1);
    var_set_num(vm, "CLAMP_LO", lo);
    var_set_num(vm, "CLAMP_HI", hi);
    var_set_num(vm, "CLAMP_CHANGED", (long)changed);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# CLAMP %s [%ld..%ld] -> %ld\n", name, lo, hi, nv);
    bump(vm);
    return 1;
  }
  /* SETMIN|MINTO name val — name = min(name, val); missing name → val.
   * SETMAX|MAXTO name val — name = max(name, val).
   * Usability: running best/worst without IF score > best THEN LET best = score. */
  if (kw(&L->cur,"SETMIN")||kw(&L->cur,"MINTO")||kw(&L->cur,"MINV")||
      kw(&L->cur,"KEEPMIN")||kw(&L->cur,"MINVAR")||
      kw(&L->cur,"SETMAX")||kw(&L->cur,"MAXTO")||kw(&L->cur,"MAXV")||
      kw(&L->cur,"KEEPMAX")||kw(&L->cur,"MAXVAR")){
    char op[24], name[48];
    int is_max = 0;
    long val, cur = 0, nv;
    int have = 0;
    Var *vv;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "SETMAX") == 0 || strcmp(op, "MAXTO") == 0 ||
        strcmp(op, "MAXV") == 0 || strcmp(op, "KEEPMAX") == 0 ||
        strcmp(op, "MAXVAR") == 0)
      is_max = 1;
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, is_max ? "SETMAX needs name val — SETMAX best score"
                      : "SETMIN needs name val — SETMIN worst score");
      return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    if (kw(&L->cur, "TO") || kw(&L->cur, "WITH") || kw(&L->cur, "OF") ||
        kw(&L->cur, "=") || kw(&L->cur, "BY"))
      lex_next(L);
    if (L->cur.kind == TK_EOF || (L->cur.kind == TK_IDENT &&
        (kw(&L->cur, "END") || kw(&L->cur, "IF") || kw(&L->cur, "LET") ||
         kw(&L->cur, "PASS") || kw(&L->cur, "FAIL") || kw(&L->cur, "ASSERT") ||
         kw(&L->cur, "PRINT") || kw(&L->cur, "INC") || kw(&L->cur, "DEC") ||
         kw(&L->cur, "TOGGLE") || kw(&L->cur, "CLAMP") || kw(&L->cur, "SWAP") ||
         kw(&L->cur, "SETMIN") || kw(&L->cur, "SETMAX")))) {
      fail(vm, is_max ? "SETMAX needs value — SETMAX best score"
                      : "SETMIN needs value — SETMIN worst score");
      return -1;
    }
    val = parse_expr(vm, L);
    vv = var_get(vm, name, 0);
    if (vv && !vv->is_str) {
      cur = vv->val;
      have = 1;
    } else if (vv && vv->is_str) {
      char *end = NULL;
      cur = strtol(vv->sval, &end, 10);
      if (end && end != vv->sval && *end == 0)
        have = 1;
      else {
        cur = 0;
        have = 0;
      }
    } else {
      have = 0;
    }
    if (!have)
      nv = val;
    else if (is_max)
      nv = (val > cur) ? val : cur;
    else
      nv = (val < cur) ? val : cur;
    var_set_num(vm, name, nv);
    var_set_num(vm, "LAST_N", nv);
    vm->last_n = nv;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", nv);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, is_max ? "SETMAX_N" : "SETMIN_N", 1);
    var_set_num(vm, is_max ? "MAXTO_N" : "MINTO_N", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s -> %ld\n", is_max ? "SETMAX" : "SETMIN", name, nv);
    bump(vm);
    return 1;
  }
  /* APPENDTO|CATTO name part [part…] — append string parts onto named var.
   * PREPENDTO|PREFIXTO name part [part…] — prepend parts.
   * Missing var starts empty. Numeric vars promoted via itoa.
   * Usability: build logs/messages without LET msg = CAT msg "…" glue. */
  if (kw(&L->cur,"APPENDTO")||kw(&L->cur,"CATTO")||kw(&L->cur,"STRCATTO")||
      kw(&L->cur,"ADDSTR")||kw(&L->cur,"CONCATTO")||
      kw(&L->cur,"PREPENDTO")||kw(&L->cur,"PREFIXTO")||kw(&L->cur,"PRECATTO")||
      kw(&L->cur,"FRONTTO")||kw(&L->cur,"UNSHIFTSTR")){
    char op[24], name[48];
    int is_pre = 0;
    char cur[CUBALC_HOST_STR_MAX];
    char part[CUBALC_HOST_STR_MAX];
    char out[CUBALC_HOST_STR_MAX];
    int nparts = 0;
    Var *vv;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "PREPENDTO") == 0 || strcmp(op, "PREFIXTO") == 0 ||
        strcmp(op, "PRECATTO") == 0 || strcmp(op, "FRONTTO") == 0 ||
        strcmp(op, "UNSHIFTSTR") == 0)
      is_pre = 1;
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, is_pre ? "PREPENDTO needs name part — PREPENDTO msg \"x\""
                      : "APPENDTO needs name part — APPENDTO msg \"x\"");
      return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    if (kw(&L->cur, "WITH") || kw(&L->cur, "BY") || kw(&L->cur, "AND"))
      lex_next(L);
    cur[0] = 0;
    vv = var_get(vm, name, 0);
    if (vv && vv->is_str) {
      snprintf(cur, sizeof cur, "%s", vv->sval);
    } else if (vv && !vv->is_str) {
      snprintf(cur, sizeof cur, "%ld", vv->val);
    }
    out[0] = 0;
    /* accumulate parts first for prepend order (left-to-right as written) */
    {
      char acc[CUBALC_HOST_STR_MAX];
      acc[0] = 0;
      while (1) {
        if (L->cur.kind == TK_EOF || L->cur.kind == TK_NL)
          break;
        if (L->cur.kind == TK_IDENT &&
            (kw(&L->cur, "END") || kw(&L->cur, "IF") || kw(&L->cur, "LET") ||
             kw(&L->cur, "PASS") || kw(&L->cur, "FAIL") || kw(&L->cur, "ASSERT") ||
             kw(&L->cur, "EXPECT") || kw(&L->cur, "PRINT") || kw(&L->cur, "INC") ||
             kw(&L->cur, "DEC") || kw(&L->cur, "SWAP") || kw(&L->cur, "TOGGLE") ||
             kw(&L->cur, "CLAMP") || kw(&L->cur, "SETMIN") || kw(&L->cur, "SETMAX") ||
             kw(&L->cur, "APPENDTO") || kw(&L->cur, "CATTO") ||
             kw(&L->cur, "PREPENDTO") || kw(&L->cur, "PREFIXTO") ||
             kw(&L->cur, "COPYTO") || kw(&L->cur, "COPYV") ||
             kw(&L->cur, "DEFAULT") || kw(&L->cur, "UNSET") ||
             kw(&L->cur, "HASFORM") || kw(&L->cur, "SYS") ||
             kw(&L->cur, "INCLUDE") || kw(&L->cur, "NOTE") ||
             kw(&L->cur, "EXIT") || kw(&L->cur, "RETURN") ||
             kw(&L->cur, "WHILE") || kw(&L->cur, "LOOP") ||
             kw(&L->cur, "FOR") || kw(&L->cur, "EACH") ||
             kw(&L->cur, "CASE") || kw(&L->cur, "TRY") ||
             kw(&L->cur, "IFERR") || kw(&L->cur, "IFOK")))
          break;
        if (L->cur.kind != TK_STR && L->cur.kind != TK_IDENT &&
            L->cur.kind != TK_NUM && L->cur.kind != TK_MINUS)
          break;
        /* numeric atom as decimal string */
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
            (L->cur.kind == TK_IDENT && var_get(vm, L->cur.text, 0) &&
             !var_get(vm, L->cur.text, 0)->is_str &&
             strcmp(L->cur.text, "LAST") != 0)) {
          long n = 0;
          if (cubalc_read_num_atom(vm, L, &n)) {
            char nb[32];
            size_t al, bl;
            snprintf(nb, sizeof nb, "%ld", n);
            bl = strlen(acc);
            al = strlen(nb);
            if (bl + al + 1 < sizeof acc) {
              memcpy(acc + bl, nb, al + 1);
              nparts++;
            }
            continue;
          }
        }
        if (resolve_str_arg(vm, L, part, sizeof part) != 0)
          break;
        {
          size_t al = strlen(part), bl = strlen(acc);
          if (bl + al + 1 < sizeof acc) {
            memcpy(acc + bl, part, al + 1);
            nparts++;
          }
        }
      }
      if (nparts < 1) {
        fail(vm, is_pre ? "PREPENDTO needs part — PREPENDTO msg \"pre\""
                        : "APPENDTO needs part — APPENDTO msg \"x\"");
        return -1;
      }
      if (is_pre) {
        size_t al = strlen(acc), bl = strlen(cur);
        if (al + bl + 1 >= sizeof out) {
          snprintf(out, sizeof out, "%s", acc);
        } else {
          memcpy(out, acc, al);
          memcpy(out + al, cur, bl + 1);
        }
      } else {
        size_t al = strlen(cur), bl = strlen(acc);
        if (al + bl + 1 >= sizeof out) {
          snprintf(out, sizeof out, "%s", cur);
          {
            size_t o = strlen(out), room = sizeof out - 1 - o;
            if (room > 0) {
              size_t take = bl < room ? bl : room;
              memcpy(out + o, acc, take);
              out[o + take] = 0;
            }
          }
        } else {
          memcpy(out, cur, al);
          memcpy(out + al, acc, bl + 1);
        }
      }
    }
    var_set_str(vm, name, out);
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    {
      long ln = (long)strlen(out);
      var_set_num(vm, "LAST_N", ln);
      vm->last_n = ln;
      var_set_num(vm, is_pre ? "PREPENDTO_N" : "APPENDTO_N", (long)nparts);
      var_set_num(vm, is_pre ? "PREFIXTO_N" : "CATTO_N", (long)nparts);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s parts=%d len=%ld\n",
              is_pre ? "PREPENDTO" : "APPENDTO", name, nparts, (long)strlen(out));
    bump(vm);
    return 1;
  }
  /* COPYTO|COPYV|COPYVAR src [TO|INTO] dst — copy one var into another (num or str).
   * Soft: missing src → empty/0. Usability: dual buffer seed without LET dst = src. */
  if (kw(&L->cur,"COPYTO")||kw(&L->cur,"COPYV")||kw(&L->cur,"COPYVAR")||
      kw(&L->cur,"VARCOPY")||kw(&L->cur,"CLONEV")||kw(&L->cur,"DUPVAR")){
    char src[48], dst[48];
    Var *vs;
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, "COPYTO needs src dst — COPYTO a TO b");
      return -1;
    }
    snprintf(src, sizeof src, "%s", L->cur.text);
    lex_next(L);
    if (kw(&L->cur, "TO") || kw(&L->cur, "INTO") || kw(&L->cur, "AS") ||
        kw(&L->cur, "=>") || kw(&L->cur, "AND") || kw(&L->cur, ","))
      lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, "COPYTO needs dst — COPYTO a TO b");
      return -1;
    }
    snprintf(dst, sizeof dst, "%s", L->cur.text);
    lex_next(L);
    vs = var_get(vm, src, 0);
    if (vs && vs->is_str) {
      var_set_str(vm, dst, vs->sval);
      var_set_str(vm, "LAST", vs->sval);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", vs->sval);
      {
        long ln = (long)strlen(vs->sval);
        var_set_num(vm, "LAST_N", ln);
        vm->last_n = ln;
      }
    } else if (vs && !vs->is_str) {
      char nb[32];
      var_set_num(vm, dst, vs->val);
      var_set_num(vm, "LAST_N", vs->val);
      vm->last_n = vs->val;
      snprintf(nb, sizeof nb, "%ld", vs->val);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    } else {
      /* missing src → empty string dest (soft) */
      var_set_str(vm, dst, "");
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
    }
    var_set_num(vm, "COPYTO_N", 1);
    var_set_num(vm, "COPYV_N", 1);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# COPYTO %s -> %s\n", src, dst);
    bump(vm);
    return 1;
  }
  /* REPLACEIN|GSUBTO name needle [WITH] repl — in-place REPLACEALL on var.
   * TRIMTO|STRIPTO name — in-place whitespace trim.
   * UPPERTO|LOWERTO name — in-place ASCII case.
   * Usability: mutate string vars without LET x = REPLACEALL/TRIM/UPPER x … glue. */
  if (kw(&L->cur,"REPLACEIN")||kw(&L->cur,"GSUBTO")||kw(&L->cur,"SUBSTIN")||
      kw(&L->cur,"REPLACEALLIN")||kw(&L->cur,"REPINTO")||kw(&L->cur,"GSUBIN")||
      kw(&L->cur,"TRIMTO")||kw(&L->cur,"STRIPTO")||kw(&L->cur,"TRIMV")||
      kw(&L->cur,"UPPERTO")||kw(&L->cur,"UCASETO")||kw(&L->cur,"TOUPPERV")||
      kw(&L->cur,"LOWERTO")||kw(&L->cur,"LCASETO")||kw(&L->cur,"TOLOWERV")){
    char op[24], name[48];
    int mode = 0; /* 0=replace, 1=trim, 2=upper, 3=lower */
    char cur[CUBALC_HOST_STR_MAX];
    char out[CUBALC_HOST_STR_MAX];
    Var *vv;
    long did = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "TRIMTO") == 0 || strcmp(op, "STRIPTO") == 0 ||
        strcmp(op, "TRIMV") == 0)
      mode = 1;
    else if (strcmp(op, "UPPERTO") == 0 || strcmp(op, "UCASETO") == 0 ||
             strcmp(op, "TOUPPERV") == 0)
      mode = 2;
    else if (strcmp(op, "LOWERTO") == 0 || strcmp(op, "LCASETO") == 0 ||
             strcmp(op, "TOLOWERV") == 0)
      mode = 3;
    else
      mode = 0;
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, mode == 0 ? "REPLACEIN needs name needle repl — REPLACEIN s \"a\" \"b\""
           : mode == 1 ? "TRIMTO needs name — TRIMTO s"
           : mode == 2 ? "UPPERTO needs name — UPPERTO s"
                       : "LOWERTO needs name — LOWERTO s");
      return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    cur[0] = 0;
    vv = var_get(vm, name, 0);
    if (vv && vv->is_str)
      snprintf(cur, sizeof cur, "%s", vv->sval);
    else if (vv && !vv->is_str)
      snprintf(cur, sizeof cur, "%ld", vv->val);

    if (mode == 0) {
      char olds[CUBALC_HOST_STR_MAX], news[CUBALC_HOST_STR_MAX];
      size_t oldn, newn, o;
      const char *srcp;
      olds[0] = news[0] = 0;
      if (kw(&L->cur, "OF") || kw(&L->cur, "FROM"))
        lex_next(L);
      if (resolve_str_arg(vm, L, olds, sizeof olds) != 0) {
        fail(vm, "REPLACEIN needs needle — REPLACEIN s \"old\" \"new\"");
        return -1;
      }
      if (kw(&L->cur, "WITH") || kw(&L->cur, "TO") || kw(&L->cur, "BY") ||
          kw(&L->cur, "=>") || kw(&L->cur, ","))
        lex_next(L);
      if (resolve_str_arg(vm, L, news, sizeof news) != 0) {
        news[0] = 0;
      }
      oldn = strlen(olds);
      newn = strlen(news);
      out[0] = 0;
      o = 0;
      if (oldn == 0) {
        snprintf(out, sizeof out, "%s", cur);
        did = 0;
      } else {
        srcp = cur;
        for (;;) {
          const char *hit = strstr(srcp, olds);
          if (!hit) {
            size_t rest = strlen(srcp);
            if (o + rest >= sizeof out) rest = sizeof out - 1 - o;
            if (rest) memcpy(out + o, srcp, rest);
            o += rest;
            out[o] = 0;
            break;
          }
          {
            size_t pre = (size_t)(hit - srcp);
            if (o + pre >= sizeof out) pre = sizeof out - 1 - o;
            if (pre) { memcpy(out + o, srcp, pre); o += pre; }
            if (o + newn < sizeof out) {
              memcpy(out + o, news, newn); o += newn;
            } else if (o < sizeof out - 1) {
              size_t take = sizeof out - 1 - o;
              memcpy(out + o, news, take); o += take;
            }
            out[o] = 0;
            did++;
            srcp = hit + oldn;
            if (o >= sizeof out - 1) break;
          }
        }
      }
      var_set_num(vm, "REPLACEIN_N", did);
      var_set_num(vm, "GSUBTO_N", did);
    } else if (mode == 1) {
      char *a = cur, *b = cur + strlen(cur);
      size_t n;
      while (*a == ' ' || *a == '\t' || *a == '\n' || *a == '\r') a++;
      while (b > a && (b[-1] == ' ' || b[-1] == '\t' || b[-1] == '\n' || b[-1] == '\r')) b--;
      n = (size_t)(b - a);
      if (n >= sizeof out) n = sizeof out - 1;
      memcpy(out, a, n); out[n] = 0;
      var_set_num(vm, "TRIMTO_N", 1);
    } else if (mode == 2) {
      size_t i, n = strlen(cur);
      if (n >= sizeof out) n = sizeof out - 1;
      for (i = 0; i < n; i++) {
        char c = cur[i];
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        out[i] = c;
      }
      out[n] = 0;
      var_set_num(vm, "UPPERTO_N", 1);
    } else {
      size_t i, n = strlen(cur);
      if (n >= sizeof out) n = sizeof out - 1;
      for (i = 0; i < n; i++) {
        char c = cur[i];
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        out[i] = c;
      }
      out[n] = 0;
      var_set_num(vm, "LOWERTO_N", 1);
    }

    var_set_str(vm, name, out);
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    {
      long ln = (long)strlen(out);
      var_set_num(vm, "LAST_N", mode == 0 ? did : ln);
      vm->last_n = mode == 0 ? did : ln;
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s len=%ld\n", op, name, (long)strlen(out));
    bump(vm);
    return 1;
  }
  /* MUL|MULTO name [BY] n — numeric var *= n (default 2).
   * DIVBY|IDIVTO name [BY] n — integer divide (default 2; n==0 soft → 0).
   * MODBY|REMTO name [BY] n — remainder (n==0 soft → 0).
   * ABS|ABSTO name — absolute value in place.
   * NEG|NEGTO name — negate in place.
   * Usability: scale counters without LET x = x * 2 glue.
   * Bare MUL/DIV/MOD stay stack ALU (core only claims when name IDENT follows,
   * or when the opcode is a clear var alias like MULTO/DIVBY). */
  if (kw(&L->cur,"MUL")||kw(&L->cur,"MULTO")||kw(&L->cur,"MULT")||
      kw(&L->cur,"MULTIPLY")||kw(&L->cur,"MULBY")||kw(&L->cur,"SCALEV")||
      kw(&L->cur,"DIVBY")||kw(&L->cur,"IDIVTO")||kw(&L->cur,"DIVTO")||
      kw(&L->cur,"QUOTTO")||kw(&L->cur,"IDIV")||
      kw(&L->cur,"MODBY")||kw(&L->cur,"REMTO")||kw(&L->cur,"MODTO")||
      kw(&L->cur,"REMAINDER")||
      kw(&L->cur,"ABS")||kw(&L->cur,"ABSTO")||kw(&L->cur,"IABS")||
      kw(&L->cur,"ABSVAL")||kw(&L->cur,"MAG")||
      kw(&L->cur,"NEG")||kw(&L->cur,"NEGTO")||kw(&L->cur,"NEGATE")||
      kw(&L->cur,"FLIPSIGN")||kw(&L->cur,"CHS")){
    char op[24], name[48];
    int mode = 0; /* 0 mul, 1 div, 2 mod, 3 abs, 4 neg */
    long cur = 0, factor = 2, nv;
    Var *vv;
    Lex peek;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "DIVBY") == 0 || strcmp(op, "IDIVTO") == 0 ||
        strcmp(op, "DIVTO") == 0 || strcmp(op, "QUOTTO") == 0 ||
        strcmp(op, "IDIV") == 0)
      mode = 1;
    else if (strcmp(op, "MODBY") == 0 || strcmp(op, "REMTO") == 0 ||
             strcmp(op, "MODTO") == 0 || strcmp(op, "REMAINDER") == 0)
      mode = 2;
    else if (strcmp(op, "ABS") == 0 || strcmp(op, "ABSTO") == 0 ||
             strcmp(op, "IABS") == 0 || strcmp(op, "ABSVAL") == 0 ||
             strcmp(op, "MAG") == 0)
      mode = 3;
    else if (strcmp(op, "NEG") == 0 || strcmp(op, "NEGTO") == 0 ||
             strcmp(op, "NEGATE") == 0 || strcmp(op, "FLIPSIGN") == 0 ||
             strcmp(op, "CHS") == 0)
      mode = 4;
    else
      mode = 0;
    /* Peek: bare MUL must fall through to stack ALU; var form needs name. */
    peek = *L;
    lex_next(&peek);
    if (peek.cur.kind != TK_IDENT) {
      int force_var =
          (strcmp(op, "MULTO") == 0 || strcmp(op, "MULT") == 0 ||
           strcmp(op, "MULTIPLY") == 0 || strcmp(op, "MULBY") == 0 ||
           strcmp(op, "SCALEV") == 0 ||
           mode == 1 || mode == 2 ||
           strcmp(op, "ABSTO") == 0 || strcmp(op, "ABSVAL") == 0 ||
           strcmp(op, "NEGTO") == 0 || strcmp(op, "FLIPSIGN") == 0 ||
           strcmp(op, "CHS") == 0);
      /* Bare MUL with stack depth≥2 → stack ALU. Else hard-fail (CLI arity). */
      if (!force_var && mode == 0 && vm->sp >= 2)
        return 0;
      if (!force_var && mode == 4 && (strcmp(op, "NEGATE") == 0) && vm->sp >= 1)
        return 0; /* stack NEGATE */
      fail(vm, mode == 0 ? "MUL needs name [n] — MUL score BY 2"
           : mode == 1 ? "DIVBY needs name [n] — DIVBY total 3"
           : mode == 2 ? "MODBY needs name [n] — MODBY n 10"
           : mode == 3 ? "ABS needs name — ABS delta"
                       : "NEG needs name — NEG delta");
      return -1;
    }
    lex_next(L); /* consume opcode */
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    if (mode <= 2) {
      if (kw(&L->cur, "BY") || kw(&L->cur, "OF") || kw(&L->cur, "WITH") ||
          kw(&L->cur, "TIMES") || kw(&L->cur, "/"))
        lex_next(L);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN ||
          (L->cur.kind == TK_IDENT && !kw(&L->cur, "END") && !kw(&L->cur, "IF") &&
           !kw(&L->cur, "LET") && !kw(&L->cur, "PASS") && !kw(&L->cur, "FAIL") &&
           !kw(&L->cur, "ASSERT") && !kw(&L->cur, "PRINT") && !kw(&L->cur, "INC") &&
           !kw(&L->cur, "DEC") && !kw(&L->cur, "MUL") && !kw(&L->cur, "DIVBY") &&
           !kw(&L->cur, "MODBY") && !kw(&L->cur, "ABS") && !kw(&L->cur, "NEG") &&
           !kw(&L->cur, "ZERO") && !kw(&L->cur, "CLAMP"))) {
        if (!cubalc_read_num_atom(vm, L, &factor))
          factor = 2;
      } else {
        factor = 2;
      }
    }
    vv = var_get(vm, name, 0);
    if (vv && !vv->is_str) {
      cur = vv->val;
    } else if (vv && vv->is_str) {
      char *end = NULL;
      cur = strtol(vv->sval, &end, 10);
      if (!(end && end != vv->sval && *end == 0))
        cur = 0;
    } else {
      cur = 0;
    }
    if (mode == 0) {
      nv = cur * factor;
      var_set_num(vm, "MUL_N", 1);
      var_set_num(vm, "MULTO_N", 1);
    } else if (mode == 1) {
      nv = (factor == 0) ? 0 : (cur / factor);
      var_set_num(vm, "DIVBY_N", 1);
    } else if (mode == 2) {
      nv = (factor == 0) ? 0 : (cur % factor);
      var_set_num(vm, "MODBY_N", 1);
    } else if (mode == 3) {
      nv = (cur < 0) ? -cur : cur;
      var_set_num(vm, "ABS_N", 1);
    } else {
      nv = -cur;
      var_set_num(vm, "NEG_N", 1);
    }
    var_set_num(vm, name, nv);
    var_set_num(vm, "LAST_N", nv);
    vm->last_n = nv;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", nv);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s -> %ld\n", op, name, nv);
    bump(vm);
    return 1;
  }
  /* SPLITTO name [BY] sep — split string var into newline bag in place.
   * JOINTO name [WITH] sep — join bag fields with sep in place.
   * GREPIN|KEEPIN name needle — keep fields containing needle.
   * GREPVIN|DROPIN name needle — drop fields containing needle.
   * SORTIN name — lex sort bag fields in place.
   * UNIQIN name — adjacent-dedupe after sort (sorts first).
   * Usability: bag hygiene on named vars without LET x = SYS SPLIT/GREP x glue. */
  if (kw(&L->cur,"SPLITTO")||kw(&L->cur,"FIELDSINTO")||kw(&L->cur,"BAGSPLIT")||
      kw(&L->cur,"JOINTO")||kw(&L->cur,"JOINBAG")||kw(&L->cur,"BAGJOIN")||
      kw(&L->cur,"GREPIN")||kw(&L->cur,"KEEPIN")||kw(&L->cur,"FILTERIN")||
      kw(&L->cur,"GREPVIN")||kw(&L->cur,"DROPIN")||kw(&L->cur,"VGREPIN")||
      kw(&L->cur,"SORTIN")||kw(&L->cur,"SORTBAGIN")||kw(&L->cur,"BAGSORT")||
      kw(&L->cur,"UNIQIN")||kw(&L->cur,"DEDUPIN")||kw(&L->cur,"UNIQUEIN")||
      kw(&L->cur,"BAGUNIQ")){
    char op[24], name[48];
    int mode = 0; /* 0 split, 1 join, 2 grepin, 3 grepv, 4 sort, 5 uniq */
    char cur[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
    char arg[CUBALC_HOST_STR_MAX];
    Var *vv;
    long nfields = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "JOINTO") == 0 || strcmp(op, "JOINBAG") == 0 ||
        strcmp(op, "BAGJOIN") == 0)
      mode = 1;
    else if (strcmp(op, "GREPIN") == 0 || strcmp(op, "KEEPIN") == 0 ||
             strcmp(op, "FILTERIN") == 0)
      mode = 2;
    else if (strcmp(op, "GREPVIN") == 0 || strcmp(op, "DROPIN") == 0 ||
             strcmp(op, "VGREPIN") == 0)
      mode = 3;
    else if (strcmp(op, "SORTIN") == 0 || strcmp(op, "SORTBAGIN") == 0 ||
             strcmp(op, "BAGSORT") == 0)
      mode = 4;
    else if (strcmp(op, "UNIQIN") == 0 || strcmp(op, "DEDUPIN") == 0 ||
             strcmp(op, "UNIQUEIN") == 0 || strcmp(op, "BAGUNIQ") == 0)
      mode = 5;
    else
      mode = 0;
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, mode == 0 ? "SPLITTO needs name [BY] sep — SPLITTO csv BY \",\""
           : mode == 1 ? "JOINTO needs name [WITH] sep — JOINTO bag WITH \",\""
           : mode <= 3 ? "GREPIN needs name needle — GREPIN bag \"err\""
                       : "SORTIN needs name — SORTIN bag");
      return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    cur[0] = 0;
    vv = var_get(vm, name, 0);
    if (vv && vv->is_str)
      snprintf(cur, sizeof cur, "%s", vv->sval);
    else if (vv && !vv->is_str)
      snprintf(cur, sizeof cur, "%ld", vv->val);

    if (mode == 0) {
      /* SPLITTO */
      size_t sepn, o = 0;
      const char *p, *hit;
      if (kw(&L->cur, "BY") || kw(&L->cur, "ON") || kw(&L->cur, "WITH") ||
          kw(&L->cur, "SEP"))
        lex_next(L);
      if (resolve_str_arg(vm, L, arg, sizeof arg) != 0) {
        fail(vm, "SPLITTO needs sep — SPLITTO csv BY \",\"");
        return -1;
      }
      sepn = strlen(arg);
      out[0] = 0;
      if (sepn == 0) {
        snprintf(out, sizeof out, "%s", cur);
        nfields = cur[0] ? 1 : 0;
      } else {
        p = cur;
        nfields = 0;
        for (;;) {
          hit = strstr(p, arg);
          if (!hit) {
            size_t rest = strlen(p);
            if (o && o + 1 < sizeof out) out[o++] = '\n';
            if (o + rest >= sizeof out) rest = sizeof out - 1 - o;
            if (rest) memcpy(out + o, p, rest);
            o += rest;
            out[o] = 0;
            nfields++;
            break;
          }
          {
            size_t pre = (size_t)(hit - p);
            if (o && o + 1 < sizeof out) out[o++] = '\n';
            if (o + pre >= sizeof out) pre = sizeof out - 1 - o;
            if (pre) { memcpy(out + o, p, pre); o += pre; }
            out[o] = 0;
            nfields++;
            p = hit + sepn;
            if (o >= sizeof out - 1) break;
            if (!*p) {
              /* trailing sep → empty field */
              if (o + 1 < sizeof out) { out[o++] = '\n'; out[o] = 0; }
              nfields++;
              break;
            }
          }
        }
      }
      var_set_num(vm, "SPLITTO_N", nfields);
    } else if (mode == 1) {
      /* JOINTO */
      size_t sepn, o = 0;
      const char *p, *start;
      int first = 1;
      if (kw(&L->cur, "WITH") || kw(&L->cur, "BY") || kw(&L->cur, "SEP") ||
          kw(&L->cur, "USING"))
        lex_next(L);
      if (resolve_str_arg(vm, L, arg, sizeof arg) != 0) {
        /* default join with empty sep (concat fields) */
        arg[0] = 0;
      }
      sepn = strlen(arg);
      out[0] = 0;
      p = cur;
      nfields = 0;
      if (!cur[0]) {
        nfields = 0;
      } else {
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          {
            size_t fl = (size_t)(p - start);
            if (!first && sepn) {
              if (o + sepn < sizeof out) {
                memcpy(out + o, arg, sepn); o += sepn;
              } else if (o < sizeof out - 1) {
                size_t take = sizeof out - 1 - o;
                memcpy(out + o, arg, take); o += take;
              }
            }
            first = 0;
            if (o + fl < sizeof out) {
              memcpy(out + o, start, fl); o += fl;
            } else if (o < sizeof out - 1) {
              size_t take = sizeof out - 1 - o;
              memcpy(out + o, start, take); o += take;
            }
            out[o] = 0;
            nfields++;
          }
          if (*p == '\n') p++;
        }
      }
      var_set_num(vm, "JOINTO_N", nfields);
    } else if (mode == 2 || mode == 3) {
      /* GREPIN / GREPVIN */
      size_t o = 0;
      const char *p, *start;
      int first = 1;
      if (kw(&L->cur, "FOR") || kw(&L->cur, "WITH") || kw(&L->cur, "MATCHING"))
        lex_next(L);
      if (resolve_str_arg(vm, L, arg, sizeof arg) != 0) {
        fail(vm, "GREPIN needs needle — GREPIN bag \"err\"");
        return -1;
      }
      out[0] = 0;
      p = cur;
      nfields = 0;
      if (cur[0]) {
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          {
            size_t fl = (size_t)(p - start);
            char field[CUBALC_HOST_STR_MAX];
            int match;
            if (fl >= sizeof field) fl = sizeof field - 1;
            memcpy(field, start, fl);
            field[fl] = 0;
            match = (arg[0] == 0) ? 1 : (strstr(field, arg) != NULL);
            if ((mode == 2 && match) || (mode == 3 && !match)) {
              if (!first && o + 1 < sizeof out) out[o++] = '\n';
              first = 0;
              if (o + fl < sizeof out) {
                memcpy(out + o, field, fl); o += fl;
              } else if (o < sizeof out - 1) {
                size_t take = sizeof out - 1 - o;
                memcpy(out + o, field, take); o += take;
              }
              out[o] = 0;
              nfields++;
            }
          }
          if (*p == '\n') p++;
        }
      }
      var_set_num(vm, mode == 2 ? "GREPIN_N" : "GREPVIN_N", nfields);
    } else {
      /* SORTIN / UNIQIN — collect fields, qsort, optional dedupe */
      enum { MAXF = 512, MAXFL = 256 };
      char fields[MAXF][MAXFL];
      int idx[MAXF];
      int nf = 0, i, j;
      const char *p, *start;
      size_t o = 0;
      int first = 1;
      p = cur;
      if (cur[0]) {
        while (*p && nf < MAXF) {
          start = p;
          while (*p && *p != '\n') p++;
          {
            size_t fl = (size_t)(p - start);
            if (fl >= MAXFL) fl = MAXFL - 1;
            memcpy(fields[nf], start, fl);
            fields[nf][fl] = 0;
            idx[nf] = nf;
            nf++;
          }
          if (*p == '\n') p++;
        }
      }
      /* simple insertion sort by field text */
      for (i = 1; i < nf; i++) {
        int key = idx[i];
        j = i - 1;
        while (j >= 0 && strcmp(fields[idx[j]], fields[key]) > 0) {
          idx[j + 1] = idx[j];
          j--;
        }
        idx[j + 1] = key;
      }
      out[0] = 0;
      nfields = 0;
      for (i = 0; i < nf; i++) {
        const char *f = fields[idx[i]];
        size_t fl;
        if (mode == 5 && i > 0 && strcmp(fields[idx[i - 1]], f) == 0)
          continue; /* uniq */
        fl = strlen(f);
        if (!first && o + 1 < sizeof out) out[o++] = '\n';
        first = 0;
        if (o + fl < sizeof out) {
          memcpy(out + o, f, fl); o += fl;
        } else if (o < sizeof out - 1) {
          size_t take = sizeof out - 1 - o;
          memcpy(out + o, f, take); o += take;
        }
        out[o] = 0;
        nfields++;
      }
      var_set_num(vm, mode == 5 ? "UNIQIN_N" : "SORTIN_N", nfields);
    }

    var_set_str(vm, name, out);
    var_set_str(vm, "LAST", out);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    var_set_num(vm, "LAST_N", nfields);
    vm->last_n = nfields;
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s n=%ld\n", op, name, nfields);
    bump(vm);
    return 1;
  }
  /* COALESCETO|NVLTO name a [OR b] [OR c…] — first non-empty into name.
   * COALESCETO BLANK name a OR b… — first non-blank (not only whitespace).
   * All empty → name "" · LAST_N=0 · COALESCETO_I = 0-based pick (−1 if none).
   * Usability: default chains without nested IFEMPTY + LET glue. */
  if (kw(&L->cur,"COALESCETO")||kw(&L->cur,"NVLTO")||kw(&L->cur,"FIRSTTO")||
      kw(&L->cur,"ORTO")||kw(&L->cur,"DEFAULTTO")||kw(&L->cur,"PICKSTRTO")||
      kw(&L->cur,"COALESCEV")||kw(&L->cur,"NVLINTO")){
    char name[48];
    char chosen[CUBALC_HOST_STR_MAX], arg[CUBALC_HOST_STR_MAX];
    int skip_blank = 0, got = 0, parts = 0, pick_i = -1;
    long out_n = 0;
    lex_next(L);
    if (kw(&L->cur,"BLANK") || kw(&L->cur,"NB") || kw(&L->cur,"NONBLANK") ||
        kw(&L->cur,"TRIM") || kw(&L->cur,"WS")) {
      skip_blank = 1;
      lex_next(L);
    }
    if (L->cur.kind != TK_IDENT) {
      fail(vm, "COALESCETO needs name a [OR b]… — COALESCETO path a OR b OR \"def\"");
      return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    if (kw(&L->cur, "FROM") || kw(&L->cur, "=") || kw(&L->cur, "TO") ||
        kw(&L->cur, "WITH"))
      lex_next(L);
    chosen[0] = 0;
    while (1) {
      if (L->cur.kind == TK_EOF || L->cur.kind == TK_NL)
        break;
      if (L->cur.kind == TK_IDENT &&
          (kw(&L->cur, "END") || kw(&L->cur, "IF") || kw(&L->cur, "LET") ||
           kw(&L->cur, "PASS") || kw(&L->cur, "FAIL") || kw(&L->cur, "ASSERT") ||
           kw(&L->cur, "EXPECT") || kw(&L->cur, "PRINT") || kw(&L->cur, "INC") ||
           kw(&L->cur, "DEC") || kw(&L->cur, "MUL") || kw(&L->cur, "DEFAULT") ||
           kw(&L->cur, "COALESCETO") || kw(&L->cur, "NVLTO") ||
           kw(&L->cur, "IFEMPTY") || kw(&L->cur, "IFOK") || kw(&L->cur, "IFERR") ||
           kw(&L->cur, "HASFORM") || kw(&L->cur, "SYS") || kw(&L->cur, "NOTE") ||
           kw(&L->cur, "EXIT") || kw(&L->cur, "WHILE") || kw(&L->cur, "LOOP") ||
           kw(&L->cur, "FOR") || kw(&L->cur, "EACH") || kw(&L->cur, "CASE") ||
           kw(&L->cur, "TRY") || kw(&L->cur, "APPENDTO") || kw(&L->cur, "PUSHTO") ||
           kw(&L->cur, "ZERO") || kw(&L->cur, "CLEARSTR") || kw(&L->cur, "SWAP")))
        break;
      if (kw(&L->cur, "OR") || kw(&L->cur, "ELSE") || kw(&L->cur, "DEFAULT") ||
          kw(&L->cur, ",") || kw(&L->cur, "||")) {
        lex_next(L);
        continue;
      }
      arg[0] = 0;
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS) {
        long n = 0;
        if (!cubalc_read_num_atom(vm, L, &n))
          break;
        snprintf(arg, sizeof arg, "%ld", n);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, arg, sizeof arg) != 0)
          break;
      } else {
        break;
      }
      {
        int empty = (arg[0] == 0);
        if (skip_blank && !empty) {
          const char *p = arg;
          while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
          empty = (*p == 0);
        }
        if (!got && !empty) {
          snprintf(chosen, sizeof chosen, "%s", arg);
          got = 1;
          pick_i = parts;
        }
        parts++;
      }
    }
    if (parts < 1) {
      fail(vm, "COALESCETO needs values — COALESCETO path a OR b OR \"def\"");
      return -1;
    }
    var_set_str(vm, name, chosen);
    var_set_str(vm, "LAST", chosen);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", chosen);
    out_n = (long)strlen(chosen);
    var_set_num(vm, "LAST_N", out_n);
    vm->last_n = out_n;
    var_set_num(vm, "COALESCETO_N", out_n);
    var_set_num(vm, "NVLTO_N", out_n);
    var_set_num(vm, "COALESCETO_I", (long)pick_i);
    var_set_num(vm, "COALESCETO_PARTS", (long)parts);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# COALESCETO %s pick=%d len=%ld\n", name, pick_i, out_n);
    bump(vm);
    return 1;
  }
  /* NTHIN name i [OR fallback] — 0-based bag field peel → LAST (name unchanged).
   * HEADIN name / TAILIN name — first/last field → LAST.
   * Usability: read work bag without LET x = SYS NTH i bag glue. */
  if (kw(&L->cur,"NTHIN")||kw(&L->cur,"BAGNTH")||kw(&L->cur,"FIELDAT")||
      kw(&L->cur,"HEADIN")||kw(&L->cur,"FIRSTIN")||kw(&L->cur,"BAGHEAD")||
      kw(&L->cur,"TAILIN")||kw(&L->cur,"LASTIN")||kw(&L->cur,"BAGTAIL")){
    char op[24], name[48], bag[CUBALC_HOST_STR_MAX], field[CUBALC_HOST_STR_MAX];
    char fb[CUBALC_HOST_STR_MAX];
    int mode = 0; /* 0 nth, 1 head, 2 tail */
    long want = 0, at = 0, found = 0, have_fb = 0;
    const char *p, *start;
    Var *vv;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "HEADIN") == 0 || strcmp(op, "FIRSTIN") == 0 ||
        strcmp(op, "BAGHEAD") == 0)
      mode = 1;
    else if (strcmp(op, "TAILIN") == 0 || strcmp(op, "LASTIN") == 0 ||
             strcmp(op, "BAGTAIL") == 0)
      mode = 2;
    else
      mode = 0;
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, mode == 0 ? "NTHIN needs name i — NTHIN bag 0"
           : mode == 1 ? "HEADIN needs name — HEADIN bag"
                       : "TAILIN needs name — TAILIN bag");
      return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    bag[0] = field[0] = fb[0] = 0;
    vv = var_get(vm, name, 0);
    if (vv && vv->is_str)
      snprintf(bag, sizeof bag, "%s", vv->sval);
    else if (vv && !vv->is_str)
      snprintf(bag, sizeof bag, "%ld", vv->val);
    if (mode == 0) {
      if (kw(&L->cur, "AT") || kw(&L->cur, "INDEX") || kw(&L->cur, "OF"))
        lex_next(L);
      if (!cubalc_read_num_atom(vm, L, &want)) {
        fail(vm, "NTHIN needs index — NTHIN bag 0");
        return -1;
      }
      if (want < 0) want = 0;
    }
    if (kw(&L->cur, "OR") || kw(&L->cur, "DEFAULT") || kw(&L->cur, "ELSE") ||
        kw(&L->cur, "FALLBACK")) {
      lex_next(L);
      if (resolve_str_arg(vm, L, fb, sizeof fb) != 0)
        fb[0] = 0;
      have_fb = 1;
    }
    p = bag;
    if (mode == 1) {
      /* first field */
      start = p;
      while (*p && *p != '\n') p++;
      if (bag[0]) {
        size_t fl = (size_t)(p - start);
        if (fl >= sizeof field) fl = sizeof field - 1;
        memcpy(field, start, fl);
        field[fl] = 0;
        found = 1;
      }
    } else if (mode == 2) {
      const char *last = bag;
      if (bag[0]) {
        for (p = bag; *p; p++)
          if (*p == '\n') last = p + 1;
        snprintf(field, sizeof field, "%s", last);
        found = 1;
      }
    } else {
      at = 0;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        if (at == want) {
          size_t fl = (size_t)(p - start);
          if (fl >= sizeof field) fl = sizeof field - 1;
          memcpy(field, start, fl);
          field[fl] = 0;
          found = 1;
          break;
        }
        at++;
        if (*p == '\n') p++;
      }
    }
    if (!found) {
      if (have_fb)
        snprintf(field, sizeof field, "%s", fb);
      else
        field[0] = 0;
    }
    var_set_str(vm, "LAST", field);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", field);
    {
      long ln = (long)strlen(field);
      var_set_num(vm, "LAST_N", found ? 1 : 0);
      vm->last_n = found ? 1 : 0;
      var_set_num(vm, mode == 1 ? "HEADIN_N" : mode == 2 ? "TAILIN_N" : "NTHIN_N",
                  found);
      var_set_num(vm, "NTHIN_LEN", ln);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s found=%ld\n", op, name, found);
    bump(vm);
    return 1;
  }
  /* LENOF name — string length of var → LAST_N (missing→0).
   * STARTSIN name prefix — 0|1 prefix probe.
   * ENDSIN name suffix — 0|1 suffix probe.
   * CONTAINSIN|HASSTRIN name needle — 0|1 contains probe.
   * EQSIN name other — 0|1 exact string equality.
   * INDEXIN name needle — 0-based index or −1 if missing.
   * Usability: IF guards on named vars without SYS LEN/STARTS/HAS/EQS glue. */
  if (kw(&L->cur,"LENOF")||kw(&L->cur,"STRLENOF")||kw(&L->cur,"LENV")||
      kw(&L->cur,"LENGTHOF")||kw(&L->cur,"BYTELEN")||
      kw(&L->cur,"STARTSIN")||kw(&L->cur,"PREFIXOF")||kw(&L->cur,"HASPREFIXIN")||
      kw(&L->cur,"ENDSIN")||kw(&L->cur,"SUFFIXOF")||kw(&L->cur,"HASSUFFIXIN")||
      kw(&L->cur,"CONTAINSIN")||kw(&L->cur,"HASSTRIN")||kw(&L->cur,"INCLUDESIN")||
      kw(&L->cur,"STRHASIN")||
      kw(&L->cur,"EQSIN")||kw(&L->cur,"STREQIN")||kw(&L->cur,"SAMESTRIN")||
      kw(&L->cur,"INDEXIN")||kw(&L->cur,"FINDAT")||kw(&L->cur,"STRINDEXIN")||
      kw(&L->cur,"POSIN")){
    char op[24], name[48], cur[CUBALC_HOST_STR_MAX], arg[CUBALC_HOST_STR_MAX];
    int mode = 0; /* 0 len, 1 starts, 2 ends, 3 contains, 4 eqs, 5 index */
    long hit = 0;
    Var *vv;
    size_t cn, an;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "STARTSIN") == 0 || strcmp(op, "PREFIXOF") == 0 ||
        strcmp(op, "HASPREFIXIN") == 0)
      mode = 1;
    else if (strcmp(op, "ENDSIN") == 0 || strcmp(op, "SUFFIXOF") == 0 ||
             strcmp(op, "HASSUFFIXIN") == 0)
      mode = 2;
    else if (strcmp(op, "CONTAINSIN") == 0 || strcmp(op, "HASSTRIN") == 0 ||
             strcmp(op, "INCLUDESIN") == 0 || strcmp(op, "STRHASIN") == 0)
      mode = 3;
    else if (strcmp(op, "EQSIN") == 0 || strcmp(op, "STREQIN") == 0 ||
             strcmp(op, "SAMESTRIN") == 0)
      mode = 4;
    else if (strcmp(op, "INDEXIN") == 0 || strcmp(op, "FINDAT") == 0 ||
             strcmp(op, "STRINDEXIN") == 0 || strcmp(op, "POSIN") == 0)
      mode = 5;
    else
      mode = 0;
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, mode == 0 ? "LENOF needs name — LENOF msg"
           : mode == 1 ? "STARTSIN needs name prefix — STARTSIN path \"/tmp\""
           : mode == 2 ? "ENDSIN needs name suffix — ENDSIN file \".log\""
           : mode == 3 ? "CONTAINSIN needs name needle — CONTAINSIN s \"err\""
           : mode == 4 ? "EQSIN needs name other — EQSIN a b"
                       : "INDEXIN needs name needle — INDEXIN s \":\"");
      return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    cur[0] = 0;
    vv = var_get(vm, name, 0);
    if (vv && vv->is_str)
      snprintf(cur, sizeof cur, "%s", vv->sval);
    else if (vv && !vv->is_str)
      snprintf(cur, sizeof cur, "%ld", vv->val);
    cn = strlen(cur);

    if (mode == 0) {
      hit = (long)cn;
      var_set_num(vm, "LENOF_N", hit);
      var_set_num(vm, "LAST_N", hit);
      vm->last_n = hit;
      {
        char nb[32];
        snprintf(nb, sizeof nb, "%ld", hit);
        var_set_str(vm, "LAST", nb);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
      }
    } else {
      if (kw(&L->cur, "WITH") || kw(&L->cur, "OF") || kw(&L->cur, "TO") ||
          kw(&L->cur, "EQ") || kw(&L->cur, "=="))
        lex_next(L);
      arg[0] = 0;
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS) {
        long n = 0;
        if (cubalc_read_num_atom(vm, L, &n))
          snprintf(arg, sizeof arg, "%ld", n);
      } else if (resolve_str_arg(vm, L, arg, sizeof arg) != 0) {
        fail(vm, mode == 1 ? "STARTSIN needs prefix"
             : mode == 2 ? "ENDSIN needs suffix"
             : mode == 3 ? "CONTAINSIN needs needle"
             : mode == 4 ? "EQSIN needs other"
                         : "INDEXIN needs needle");
        return -1;
      }
      an = strlen(arg);
      if (mode == 1) {
        hit = (an == 0) ? 1 : ((cn >= an && memcmp(cur, arg, an) == 0) ? 1 : 0);
        var_set_num(vm, "STARTSIN_N", hit);
      } else if (mode == 2) {
        hit = (an == 0) ? 1 : ((cn >= an && memcmp(cur + (cn - an), arg, an) == 0) ? 1 : 0);
        var_set_num(vm, "ENDSIN_N", hit);
      } else if (mode == 3) {
        hit = (an == 0) ? 1 : (strstr(cur, arg) != NULL ? 1 : 0);
        var_set_num(vm, "CONTAINSIN_N", hit);
      } else if (mode == 4) {
        hit = (strcmp(cur, arg) == 0) ? 1 : 0;
        var_set_num(vm, "EQSIN_N", hit);
      } else {
        if (an == 0) {
          hit = 0;
        } else {
          const char *p = strstr(cur, arg);
          hit = p ? (long)(p - cur) : -1L;
        }
        var_set_num(vm, "INDEXIN_N", hit);
      }
      var_set_num(vm, "LAST_N", hit);
      vm->last_n = hit;
      {
        char nb[32];
        snprintf(nb, sizeof nb, "%ld", hit);
        var_set_str(vm, "LAST", nb);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
      }
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s -> %ld\n", op, name, hit);
    bump(vm);
    return 1;
  }
  /* DEFAULT name = expr|str — assign only if name is not yet defined.
   * Usability: INCLUDE libs set knobs without clobbering caller LET.
   * LAST_N/DEFAULT_SET = 1 if applied, 0 if skipped; OK always 1. */
  if (kw(&L->cur,"DEFAULT")||kw(&L->cur,"SETDEFAULT")||kw(&L->cur,"DEFAULT_LET")||
      kw(&L->cur,"LET_DEFAULT")||kw(&L->cur,"ORLET")){
    int applied = 0;
    int exists;
    char name[48];
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"DEFAULT name = value"); return -1; }
    snprintf(name,sizeof name,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_EQ){ fail(vm,"DEFAULT name = value"); return -1; }
    lex_next(L);
    exists = (var_get(vm, name, 0) != NULL);
    if (L->cur.kind==TK_STR || (L->cur.kind==TK_IDENT && (
          strcmp(L->cur.text,"LAST")==0 ||
          (var_get(vm,L->cur.text,0) && var_get(vm,L->cur.text,0)->is_str)))){
      char buf[CUBALC_HOST_STR_MAX]; buf[0]=0;
      for(;;){
        if (L->cur.kind==TK_STR){
          size_t bl=strlen(buf), al=strlen(L->cur.text);
          if (bl+al+1 < sizeof buf) memcpy(buf+bl, L->cur.text, al+1);
          lex_next(L);
        } else if (L->cur.kind==TK_IDENT){
          if (strcmp(L->cur.text,"LAST")==0){
            size_t bl=strlen(buf), al=strlen(vm->last_str);
            if (bl+al+1 < sizeof buf) memcpy(buf+bl, vm->last_str, al+1);
            lex_next(L);
          } else {
            Var *v = var_get(vm, L->cur.text, 0);
            if (v && v->is_str){
              size_t bl=strlen(buf), al=strlen(v->sval);
              if (bl+al+1 < sizeof buf) memcpy(buf+bl, v->sval, al+1);
            } else if (v){
              char nb[32]; snprintf(nb,sizeof nb,"%ld", v->val);
              size_t bl=strlen(buf), al=strlen(nb);
              if (bl+al+1 < sizeof buf) memcpy(buf+bl, nb, al+1);
            } else break;
            lex_next(L);
          }
        } else break;
        if (L->cur.kind==TK_PLUS){ lex_next(L); continue; }
        break;
      }
      if (!exists){ var_set_str(vm, name, buf); applied = 1; }
    } else {
      long v=parse_expr(vm,L);
      if (!exists){ var_set_num(vm, name, v); applied = 1; }
    }
    var_set_num(vm, "DEFAULT_SET", (long)applied);
    var_set_num(vm, "LAST_N", (long)applied);
    var_set_num(vm, "OK", 1);
    vm->last_n = (long)applied;
    { char nb[8]; snprintf(nb, sizeof nb, "%d", applied);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb); }
    if (vm->trace)
      fprintf(vm->trace, "# default %s %s\n", name, applied ? "set" : "skip");
    bump(vm); return 1;
  }

  /* DEFINED|ISSET name — var exists probe (B_E_science selftest).
   * LAST_N/DEFINED = 1 if present, 0 if missing; OK always 1. */
  if (kw(&L->cur,"DEFINED")||kw(&L->cur,"ISSET")||kw(&L->cur,"HASVAR")||
      kw(&L->cur,"VAREXISTS")||kw(&L->cur,"VAR?")){
    char name[48];
    int exists;
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"DEFINED needs name — DEFINED x"); return -1; }
    snprintf(name,sizeof name,"%s",L->cur.text); lex_next(L);
    exists = (var_get(vm, name, 0) != NULL);
    var_set_num(vm, "DEFINED", (long)exists);
    var_set_num(vm, "ISSET", (long)exists);
    var_set_num(vm, "LAST_N", (long)exists);
    vm->last_n = (long)exists;
    var_set_num(vm, "OK", 1);
    { char nb[8]; snprintf(nb, sizeof nb, "%d", exists);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb); }
    if (vm->trace)
      fprintf(vm->trace, "# defined %s → %d\n", name, exists);
    bump(vm); return 1;
  }

  /* TYPEOF|TYPE name — undef=0 num=1 str=2 (B_E_science selftest).
   * LAST_N/TYPE_N kind code; LAST kind word. OK always 1. */
  if (kw(&L->cur,"TYPEOF")||kw(&L->cur,"TYPE")||kw(&L->cur,"VARTYPE")||
      kw(&L->cur,"KIND OF")||kw(&L->cur,"VARKIND")){
    char name[48];
    Var *v;
    int kind = 0; /* undef */
    const char *kwd = "undef";
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"TYPEOF needs name — TYPEOF x"); return -1; }
    snprintf(name,sizeof name,"%s",L->cur.text); lex_next(L);
    v = var_get(vm, name, 0);
    if (v) {
      if (v->is_str) { kind = 2; kwd = "str"; }
      else { kind = 1; kwd = "num"; }
    }
    var_set_num(vm, "TYPE_N", (long)kind);
    var_set_num(vm, "TYPEOF_N", (long)kind);
    var_set_num(vm, "LAST_N", (long)kind);
    vm->last_n = (long)kind;
    var_set_str(vm, "TYPEOF", kwd);
    var_set_str(vm, "LAST", kwd);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", kwd);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# typeof %s → %s\n", name, kwd);
    bump(vm); return 1;
  }

  /* UNSET name — remove program var so DEFAULT can re-apply (B_E_science).
   * LAST_N/UNSET_N = 1 if removed, 0 if missing; OK always 1. */
  if (kw(&L->cur,"UNSET")||kw(&L->cur,"CLEARVAR")||kw(&L->cur,"DELVAR")||
      kw(&L->cur,"RMVAR")||kw(&L->cur,"VARDEL")){
    char name[48];
    int removed = 0;
    int i;
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"UNSET needs name — UNSET x"); return -1; }
    snprintf(name,sizeof name,"%s",L->cur.text); lex_next(L);
    for (i = 0; i < vm->n_vars; i++) {
      if (strcmp(vm->vars[i].name, name) == 0) {
        if (i + 1 < vm->n_vars)
          memmove(&vm->vars[i], &vm->vars[i + 1],
                  (size_t)(vm->n_vars - i - 1) * sizeof(vm->vars[0]));
        vm->n_vars--;
        removed = 1;
        break;
      }
    }
    var_set_num(vm, "UNSET_N", (long)removed);
    var_set_num(vm, "LAST_N", (long)removed);
    vm->last_n = (long)removed;
    var_set_num(vm, "OK", 1);
    { char nb[8]; snprintf(nb, sizeof nb, "%d", removed);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb); }
    if (vm->trace)
      fprintf(vm->trace, "# unset %s → %d\n", name, removed);
    bump(vm); return 1;
  }

  /* REQUIRE VERSION floor + REQUIRE RESTARGS hard gate (mesh vote B_E). */
  if (kw(&L->cur, "REQUIRE") || kw(&L->cur, "NEED") || kw(&L->cur, "REQUIRES")) {
    int aln = L->cur.line;
    lex_next(L);
    /* REQUIRE OK|SUCCESS ["why"] — soft-status floor (twin of NEEDOK). */
    if (kw(&L->cur, "OK") || kw(&L->cur, "SUCCESS") || kw(&L->cur, "GOOD") ||
        kw(&L->cur, "PASSING") || kw(&L->cur, "CLEAN")) {
      char why[120];
      Var *ov, *ev;
      long okv = 1;
      int bad = 0;
      lex_next(L);
      why[0] = 0;
      if (L->cur.kind == TK_STR) {
        snprintf(why, sizeof why, "%s", L->cur.text);
        lex_next(L);
      }
      ov = var_get(vm, "OK", 0);
      ev = var_get(vm, "LAST_ERR", 0);
      if (ov) okv = ov->val;
      if (okv == 0) bad = 1;
      if (ev && ev->is_str && ev->sval[0]) bad = 1;
      if (bad) {
        char msg[220];
        const char *err = (ev && ev->is_str && ev->sval[0]) ? ev->sval : "OK=0";
        if (why[0])
          snprintf(msg, sizeof msg,
                   "REQUIRE OK failed line %d: %s — %s · CATCH/CLEAR_ERR · WHY",
                   aln, why, err);
        else
          snprintf(msg, sizeof msg,
                   "REQUIRE OK failed line %d: %s — CATCH/CLEAR_ERR · WHY",
                   aln, err);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_num(vm, "REQUIRE_OK", 1);
      var_set_num(vm, "OK", 1);
      var_set_str(vm, "LAST", "ok");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "ok");
      if (vm->res) vm->res->asserts_ok++;
      if (vm->trace) fprintf(vm->trace, "# require ok\n");
      bump(vm);
      return 1;
    }
    /* REQUIRE ARG n|name — CUBALC_ARGn / named env must be non-empty. */
    if (kw(&L->cur, "ARG") || kw(&L->cur, "ARGV") || kw(&L->cur, "ARGN")) {
      char key[96], envn[96], out[CUBALC_HOST_STR_MAX];
      const char *val = NULL;
      int is_num = 0;
      long idx = -1;
      size_t vl;
      lex_next(L);
      if (L->cur.kind == TK_NUM) {
        idx = L->cur.num;
        is_num = 1;
        snprintf(envn, sizeof envn, "CUBALC_ARG%ld", idx);
        snprintf(key, sizeof key, "%ld", idx);
        lex_next(L);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        int all_digit = 1, pi;
        snprintf(key, sizeof key, "%s", L->cur.text);
        for (pi = 0; key[pi]; pi++) {
          if (key[pi] < '0' || key[pi] > '9') {
            all_digit = 0;
            break;
          }
        }
        if (all_digit && key[0]) {
          snprintf(envn, sizeof envn, "CUBALC_ARG%s", key);
          is_num = 1;
          idx = strtol(key, NULL, 10);
        } else if (strncmp(key, "CUBALC_ARG", 10) == 0) {
          snprintf(envn, sizeof envn, "%s", key);
        } else {
          snprintf(envn, sizeof envn, "%s", key);
        }
        lex_next(L);
      } else {
        fail(vm, "REQUIRE ARG n|name");
        return -1;
      }
      val = getenv(envn);
      if (!val || !val[0]) {
        char msg[180];
        if (is_num)
          snprintf(msg, sizeof msg,
                   "REQUIRE ARG %ld missing line %d — cubalc run … -- arg (SYS ARG %ld)",
                   idx, aln, idx);
        else
          snprintf(msg, sizeof msg,
                   "REQUIRE ARG '%s' missing line %d — set env or pass run arg", key, aln);
        cubalc_append_usage_tip(vm, msg, sizeof msg);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
      vl = strlen(val);
      if (vl >= sizeof out) vl = sizeof out - 1;
      memcpy(out, val, vl);
      out[vl] = 0;
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "ARG", out);
      var_set_str(vm, "REQUIRE_ARG", key);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)vl;
      var_set_num(vm, "LAST_N", (long)vl);
      var_set_num(vm, "OK", 1);
      if (vm->res) vm->res->asserts_ok++;
      if (vm->trace)
        fprintf(vm->trace, "# require arg %s ok\n", key);
      bump(vm);
      return 1;
    }

    /* REQUIRE RESTARGS|POS [min] — fail if non-flag count < min (default 1). */
    if (kw(&L->cur, "RESTARGS") || kw(&L->cur, "POSITIONALS") || kw(&L->cur, "POSARGS") ||
        kw(&L->cur, "POS") || kw(&L->cur, "NFILES") || kw(&L->cur, "FILES") ||
        kw(&L->cur, "REST_ARGS") || kw(&L->cur, "NONFLAGS")) {
      long need = 1, have;
      char bag[CUBALC_HOST_STR_MAX], msg[180];
      lex_next(L);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN ||
          (L->cur.kind == TK_IDENT && !kw(&L->cur, "ASSERT") && !kw(&L->cur, "LET") &&
           !kw(&L->cur, "PRINT") && !kw(&L->cur, "SYS") && !kw(&L->cur, "END") &&
           !kw(&L->cur, "REQUIRE") && !kw(&L->cur, "NEED") && !kw(&L->cur, "PATH") &&
           !kw(&L->cur, "FLAG") && !kw(&L->cur, "ARG") && !kw(&L->cur, "ARGC"))) {
        need = parse_expr(vm, L);
      }
      if (need < 0) need = 0;
      if (need > 32) need = 32;
      have = cubalc_collect_restargs(bag, sizeof bag);
      if (have < need) {
        snprintf(msg, sizeof msg,
                 "REQUIRE RESTARGS %ld got %ld line %d — pass files after --flags (RESTARGS)",
                 need, have, aln);
        cubalc_append_usage_tip(vm, msg, sizeof msg);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
      var_set_str(vm, "LAST", bag);
      var_set_str(vm, "RESTARGS", bag);
      var_set_str(vm, "POSITIONALS", bag);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", bag);
      vm->last_n = have;
      var_set_num(vm, "LAST_N", have);
      var_set_num(vm, "RESTARGS_N", have);
      var_set_num(vm, "REQUIRE_RESTARGS", need);
      var_set_num(vm, "OK", 1);
      if (vm->res) vm->res->asserts_ok++;
      if (vm->trace)
        fprintf(vm->trace, "# require restargs >= %ld (have %ld)\n", need, have);
      bump(vm);
      return 1;
    }

    /* REQUIRE FLAGPATH|FLAGFILE|FLAGDIR — peel path flag + ABSPATH + exist/kind.
     * Twin of soft GETFLAGPATH (B_E science). */
    if (kw(&L->cur, "FLAGPATH") || kw(&L->cur, "PATHFLAG") || kw(&L->cur, "REQPATHFLAG") ||
        kw(&L->cur, "REQUIREPATHFLAG") || kw(&L->cur, "FLAG_PATH") ||
        kw(&L->cur, "FLAGFILE") || kw(&L->cur, "REQFLAGFILE") || kw(&L->cur, "PATHFLAGFILE") ||
        kw(&L->cur, "FLAGREG") || kw(&L->cur, "REQFLAGREG") ||
        kw(&L->cur, "FLAGDIR") || kw(&L->cur, "REQFLAGDIR") || kw(&L->cur, "PATHFLAGDIR") ||
        kw(&L->cur, "FLAGFOLDER") || kw(&L->cur, "REQFLAGFOLDER")) {
      char name[96], aliases[384], hitnm[96], val[CUBALC_HOST_STR_MAX], fb[512];
      char env_used[96], src_tag[16], absbuf[CUBALC_HOST_STR_MAX];
      int hit, have_fb = 0, nac, from_env = 0;
      int want_file = 0, want_dir = 0;
      const char *kind = "FLAGPATH";
      cubalc_host_result hr;
      if (kw(&L->cur, "FLAGFILE") || kw(&L->cur, "REQFLAGFILE") ||
          kw(&L->cur, "PATHFLAGFILE") || kw(&L->cur, "FLAGREG") ||
          kw(&L->cur, "REQFLAGREG")) {
        want_file = 1;
        kind = "FLAGFILE";
      } else if (kw(&L->cur, "FLAGDIR") || kw(&L->cur, "REQFLAGDIR") ||
                 kw(&L->cur, "PATHFLAGDIR") || kw(&L->cur, "FLAGFOLDER") ||
                 kw(&L->cur, "REQFLAGFOLDER")) {
        want_dir = 1;
        kind = "FLAGDIR";
      }
      lex_next(L);
      name[0] = 0;
      aliases[0] = 0;
      hitnm[0] = 0;
      val[0] = 0;
      fb[0] = 0;
      env_used[0] = 0;
      absbuf[0] = 0;
      snprintf(src_tag, sizeof src_tag, "%s", "none");
      nac = cubalc_read_flag_aliases(L, aliases, sizeof aliases, name, sizeof name);
      if (nac <= 0 || !name[0]) {
        fail_at(vm, L,
                "REQUIRE FLAGPATH needs name — REQUIRE FLAGPATH conf|c OR ENV CONFIG OR \"cfg.json\"");
        return -1;
      }
      while (kw(&L->cur, "OR") || kw(&L->cur, "DEFAULT") || kw(&L->cur, "ELSE") ||
             kw(&L->cur, "FALLBACK")) {
        lex_next(L);
        if (kw(&L->cur, "ENV") || kw(&L->cur, "FROMENV") || kw(&L->cur, "ENVOR") ||
            kw(&L->cur, "GETENV") || kw(&L->cur, "ENV_GET")) {
          char en[96];
          const char *ev;
          lex_next(L);
          en[0] = 0;
          while (L->cur.kind == TK_MINUS) lex_next(L);
          if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
            snprintf(en, sizeof en, "%s", L->cur.text);
            lex_next(L);
          } else {
            fail_at(vm, L, "REQUIRE FLAGPATH OR ENV NAME");
            return -1;
          }
          if (!have_fb && en[0]) {
            ev = getenv(en);
            if (ev && ev[0]) {
              snprintf(fb, sizeof fb, "%s", ev);
              have_fb = 1;
              from_env = 1;
              snprintf(env_used, sizeof env_used, "%s", en);
            }
          }
          continue;
        }
        {
          char lit[512];
          lit[0] = 0;
          if (resolve_str_arg(vm, L, lit, sizeof lit) != 0) {
            fail_at(vm, L, "REQUIRE FLAGPATH name OR \"fallback\"");
            return -1;
          }
          if (!have_fb) {
            snprintf(fb, sizeof fb, "%s", lit);
            have_fb = 1;
            from_env = 0;
          }
        }
        break;
      }
      hit = cubalc_scan_cli_flag_any(aliases, val, sizeof val, hitnm, sizeof hitnm);
      if (!hit) {
        if (have_fb)
          snprintf(val, sizeof val, "%s", fb);
        else
          val[0] = 0;
      }
      if (hit)
        snprintf(src_tag, sizeof src_tag, "%s", "cli");
      else if (have_fb && from_env)
        snprintf(src_tag, sizeof src_tag, "%s", "env");
      else if (have_fb)
        snprintf(src_tag, sizeof src_tag, "%s", "default");
      else
        snprintf(src_tag, sizeof src_tag, "%s", "none");
      if (!val[0]) {
        char msg[240];
        snprintf(msg, sizeof msg,
                 "REQUIRE %s '%s' missing line %d — pass --%s path or OR ENV/default (GETFLAGPATH soft)",
                 kind, name, aln, name);
        cubalc_append_usage_tip(vm, msg, sizeof msg);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
      memset(&hr, 0, sizeof hr);
      if (cubalc_host_abspath(val, &hr) == 0 && hr.str[0])
        snprintf(absbuf, sizeof absbuf, "%s", hr.str);
      else
        snprintf(absbuf, sizeof absbuf, "%s", val);
      if (want_file || want_dir) {
        memset(&hr, 0, sizeof hr);
        if (cubalc_host_path_kind(absbuf, &hr) != 0 || (want_dir && hr.code != 2) ||
            (want_file && hr.code != 1)) {
          char msg[240];
          const char *why = (!hr.ok && hr.err[0]) ? hr.err
                                                  : (want_dir ? "not a directory" : "not a regular file");
          snprintf(msg, sizeof msg, "REQUIRE %s '%s' failed line %d — %s (%s)", kind, absbuf, aln,
                   why, name);
          cubalc_append_usage_tip(vm, msg, sizeof msg);
          if (vm->res) vm->res->asserts_fail++;
          fail(vm, msg);
          return -1;
        }
      } else {
        if (!cubalc_host_exists(absbuf) && !cubalc_host_exists(val)) {
          char msg[240];
          snprintf(msg, sizeof msg,
                   "REQUIRE FLAGPATH '%s' missing line %d — create path or fix --%s (GETFLAGPATH soft)",
                   absbuf, aln, name);
          cubalc_append_usage_tip(vm, msg, sizeof msg);
          if (vm->res) vm->res->asserts_fail++;
          fail(vm, msg);
          return -1;
        }
      }
      var_set_str(vm, "LAST", absbuf);
      var_set_str(vm, "FLAG", absbuf);
      var_set_str(vm, "FLAGPATH", absbuf);
      var_set_str(vm, "PATH", absbuf);
      var_set_str(vm, "REQUIRE_FLAGPATH", absbuf);
      var_set_str(vm, "REQUIRE_FLAG", name);
      var_set_str(vm, "GETFLAGPATH", name);
      var_set_str(vm, "GETFLAGPATH_RAW", val);
      var_set_str(vm, "FLAG_HIT_NAME", hit ? hitnm : "");
      var_set_str(vm, "FLAG_ALIAS", hit ? hitnm : "");
      var_set_str(vm, "GETFLAGPATH_SRC", src_tag);
      var_set_str(vm, "FLAG_SRC", src_tag);
      var_set_str(vm, "FLAG_ENV", (!hit && from_env) ? env_used : "");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", absbuf);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "GETFLAGPATH_N", (long)strlen(absbuf));
      var_set_num(vm, "GETFLAGPATH_HIT", hit ? 1L : 0L);
      var_set_num(vm, "GETFLAGPATH_EXIST", 1L);
      var_set_num(vm, "PATH_EXIST", 1L);
      var_set_num(vm, "OK", 1);
      if (want_dir) {
        var_set_num(vm, "ISDIR", 1);
        var_set_str(vm, "REQUIRE_FLAGDIR", absbuf);
        var_set_str(vm, "REQUIRE_DIR", absbuf);
      }
      if (want_file) {
        var_set_num(vm, "ISFILE", 1);
        var_set_str(vm, "REQUIRE_FLAGFILE", absbuf);
        var_set_str(vm, "REQUIRE_REG", absbuf);
      }
      if (vm->res) vm->res->asserts_ok++;
      if (vm->trace)
        fprintf(vm->trace, "# require %s %s hit=%s src=%s → %s\n", kind, name, hitnm, src_tag,
                absbuf);
      bump(vm);
      return 1;
    }

    /* REQUIRE ENV|VAR|ENVVAR|HOSTENV name — getenv must be non-empty.
     * On success: LAST = value, LAST_N = len, REQUIRE_ENV = name, OK=1.
     * Usability: fail-fast host config (CUBALC_STATE/SMX key) without SYS ENV + IF glue. */
    if (kw(&L->cur,"ENV")||kw(&L->cur,"VAR")||kw(&L->cur,"ENVVAR")||
        kw(&L->cur,"HOSTENV")||kw(&L->cur,"ENVIRONMENT")){
      char name[96];
      const char *val;
      lex_next(L);
      if (L->cur.kind != TK_STR && L->cur.kind != TK_IDENT){
        fail(vm, "REQUIRE ENV \"NAME\"");
        return -1;
      }
      snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
      if (!name[0]) {
        fail(vm, "REQUIRE ENV empty name");
        return -1;
      }
      val = getenv(name);
      if (!val || !val[0]) {
        char msg[160];
        snprintf(msg, sizeof msg,
                 "REQUIRE ENV '%s' missing line %d — set host env or SYS ENV SET",
                 name, aln);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
      {
        size_t vl = strlen(val);
        char out[CUBALC_HOST_STR_MAX];
        if (vl >= sizeof out) vl = sizeof out - 1;
        memcpy(out, val, vl);
        out[vl] = 0;
        var_set_str(vm, "LAST", out);
        var_set_str(vm, "ENV", out);
        var_set_str(vm, "REQUIRE_ENV", name);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
        vm->last_n = (long)vl;
        var_set_num(vm, "LAST_N", (long)vl);
        var_set_num(vm, "OK", 1);
        if (vm->trace)
          fprintf(vm->trace, "# require env %s ok (len %zu)\n", name, vl);
        if (vm->res) vm->res->asserts_ok++;
        bump(vm); return 1;
      }
    }

    /* REQUIRE PATH|EXIST path — fail if host path missing.
     * REQUIRE DIR|DIRECTORY path — fail if not a directory.
     * REQUIRE REG|REGFILE|ISFILE path — fail if not a regular file.
     * On success: LAST = path, LAST_N=1, REQUIRE_PATH = path, OK=1.
     * Usability: gate plate/config/dir before READ without SYS EXIST + IF + FAIL. */
    if (kw(&L->cur,"PATH")||kw(&L->cur,"EXIST")||kw(&L->cur,"EXISTS")||
        kw(&L->cur,"PRESENT")||kw(&L->cur,"DIR")||kw(&L->cur,"DIRECTORY")||
        kw(&L->cur,"FOLDER")||kw(&L->cur,"ISDIR")||kw(&L->cur,"REG")||
        kw(&L->cur,"REGFILE")||kw(&L->cur,"REGULAR")||kw(&L->cur,"ISFILE")||
        kw(&L->cur,"PLATE")||kw(&L->cur,"HOSTPATH")){
      char path[512];
      cubalc_host_result hr;
      int want_dir = 0, want_file = 0;
      const char *kind = "PATH";
      if (kw(&L->cur,"DIR")||kw(&L->cur,"DIRECTORY")||kw(&L->cur,"FOLDER")||
          kw(&L->cur,"ISDIR")){
        want_dir = 1;
        kind = "DIR";
      } else if (kw(&L->cur,"REG")||kw(&L->cur,"REGFILE")||kw(&L->cur,"REGULAR")||
                 kw(&L->cur,"ISFILE")||kw(&L->cur,"PLATE")){
        want_file = 1;
        kind = "REG";
      }
      lex_next(L);
      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, "REQUIRE PATH|DIR|REG \"path\"|var|LAST");
        return -1;
      }
      if (!path[0]) {
        fail(vm, "REQUIRE PATH empty path");
        return -1;
      }
      if (want_dir || want_file) {
        if (cubalc_host_path_kind(path, &hr) != 0 ||
            (want_dir && hr.code != 2) ||
            (want_file && hr.code != 1)) {
          char msg[200];
          const char *why = (!hr.ok && hr.err[0]) ? hr.err
              : (want_dir ? "not a directory" : "not a regular file");
          snprintf(msg, sizeof msg,
                   "REQUIRE %s '%s' failed line %d — %s",
                   kind, path, aln, why);
          if (vm->res) vm->res->asserts_fail++;
          fail(vm, msg);
          return -1;
        }
      } else {
        if (!cubalc_host_exists(path)) {
          char msg[200];
          snprintf(msg, sizeof msg,
                   "REQUIRE PATH '%s' missing line %d — create path or fix layout",
                   path, aln);
          if (vm->res) vm->res->asserts_fail++;
          fail(vm, msg);
          return -1;
        }
      }
      var_set_str(vm, "LAST", path);
      var_set_str(vm, "REQUIRE_PATH", path);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "OK", 1);
      if (want_dir) {
        var_set_num(vm, "ISDIR", 1);
        var_set_str(vm, "REQUIRE_DIR", path);
      }
      if (want_file) {
        var_set_num(vm, "ISFILE", 1);
        var_set_str(vm, "REQUIRE_REG", path);
      }
      if (vm->trace)
        fprintf(vm->trace, "# require %s %s ok\n", kind, path);
      if (vm->res) vm->res->asserts_ok++;
      bump(vm); return 1;
    }
    /* REQUIRE LIB|MODULE|INCLUDE|FILE name — resolve like INCLUDE / cubalc which.
     * Note: FILE here means cubalc module (legacy), not host path — use REQUIRE PATH|REG. */
    if (kw(&L->cur,"LIB")||kw(&L->cur,"MODULE")||kw(&L->cur,"INCLUDE")||
        kw(&L->cur,"FILE")||kw(&L->cur,"STDLIB")||kw(&L->cur,"SRC")){
      char name[160];
      cubalc_host_result hr;
      lex_next(L);
      name[0] = 0;
      /* Usability: REQUIRE LIB stem_var after EACH LINE / PICKLIB (twin of REQUIRE BIN). */
      if (resolve_str_arg(vm, L, name, sizeof name) != 0) {
        fail(vm, "REQUIRE LIB \"name\"|var|LAST");
        return -1;
      }
      if (!name[0]) {
        fail(vm, "REQUIRE LIB empty name");
        return -1;
      }
      if (cubalc_host_find_cubalc(name, &hr) != 0){
        char msg[160];
        snprintf(msg, sizeof msg,
                 "REQUIRE LIB '%s' missing line %d — tried programs/lib · cubalc libs",
                 name, aln);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "OK", 1);
      var_set_str(vm, "REQUIRE_LIB", hr.str);
      if (vm->trace)
        fprintf(vm->trace, "# require lib %s → %s\n", name, hr.str);
      if (vm->res) vm->res->asserts_ok++;
      bump(vm); return 1;
    }

    /* REQUIRE BIN|CMD|EXE|COMMAND name — fail if not X_OK on PATH (no lib fallback).
     * On success: LAST = resolved path, REQUIRE_BIN = path, LAST_N=1, OK=1.
     * Usability: gate curl/sh/agent tools before SPAWN without shell which. */
    if (kw(&L->cur,"BIN")||kw(&L->cur,"CMD")||kw(&L->cur,"EXE")||
        kw(&L->cur,"COMMAND")||kw(&L->cur,"TOOL")||kw(&L->cur,"PROGRAM")||
        kw(&L->cur,"BINARY")||kw(&L->cur,"EXECUTABLE")){
      char name[160];
      cubalc_host_result hr;
      lex_next(L);
      name[0] = 0;
      if (resolve_str_arg(vm, L, name, sizeof name) != 0) {
        fail(vm, "REQUIRE BIN \"name\"|var|LAST");
        return -1;
      }
      if (!name[0]) {
        fail(vm, "REQUIRE BIN empty name");
        return -1;
      }
      if (cubalc_host_which_bin(name, &hr) != 0){
        char msg[180];
        snprintf(msg, sizeof msg,
                 "REQUIRE BIN '%s' missing line %d — install tool or fix PATH",
                 name, aln);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
      var_set_str(vm, "LAST", hr.str);
      var_set_str(vm, "REQUIRE_BIN", hr.str);
      var_set_str(vm, "BIN", hr.str);
      var_set_str(vm, "WHICHBIN", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# require bin %s → %s\n", name, hr.str);
      if (vm->res) vm->res->asserts_ok++;
      bump(vm); return 1;
    }

    /* REQUIRE FORM|FORMS name — hard form catalog gate (HASFORM/NEEDFORM dual).
     * Multi-word: REQUIRE FORM SORTLIBS · REQUIRE FORMS "SYS ENV". */
    if (kw(&L->cur, "FORM") || kw(&L->cur, "FORMS") || kw(&L->cur, "OP") ||
        kw(&L->cur, "OPCODE") || kw(&L->cur, "KW")) {
      char name[96];
      const char *hint = NULL;
      int hit;
      lex_next(L);
      name[0] = 0;
      if (resolve_str_arg(vm, L, name, sizeof name) != 0) {
        fail(vm, "REQUIRE FORM \"name\"|var|LAST");
        return -1;
      }
      if (!name[0]) {
        fail(vm, "REQUIRE FORM empty name");
        return -1;
      }
      hit = cubalc_form_known(name, &hint) ? 1 : 0;
      if (!hit) {
        char msg[200];
        snprintf(msg, sizeof msg,
                 "REQUIRE FORM '%s' missing line %d — cubalc forms · HELP · HASFORM",
                 name, aln);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
      var_set_str(vm, "LAST", name);
      var_set_str(vm, "FORM", name);
      var_set_str(vm, "REQUIRE_FORM", name);
      if (hint && hint[0]) var_set_str(vm, "FORM_HINT", hint);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", name);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "OK", 1);
      var_set_num(vm, "HASFORM_N", 1);
      if (vm->trace)
        fprintf(vm->trace, "# require form %s ok\n", name);
      if (vm->res) vm->res->asserts_ok++;
      bump(vm); return 1;
    }

    /* REQUIRE JSONHASALL|JSONNEED|JSONKEYS [plate] key [key…]
     * REQUIRE JSONHASANY|NEEDJSONANY [plate] key [key…] — fail-fast any-of (HASALL twin).
     * REQUIRE JSONONLY|NOEXTRA|JSONSTRICT|ONLYP|NOEXTRAP [plate] allow [key…]
     * REQUIRE JSONEXACT|JSONSCHEMA|EXACTP|SCHEMAP [plate] key [key…] — HASALL + ONLY one-shot.
     * Soft twins: SYS JSONHASALL / JSONHASANY / JSONEXTRA / JSONEXACT · ONLYP / EXACTP.
     * Usability: full plate schema gate without multi-probe IF+FAIL glue.
     * ONLYP/EXACTP aliases = fail-fast multi-plate twins of soft ONLYP/EXACTP. */
    if (kw(&L->cur,"JSONHASALL") || kw(&L->cur,"JSONNEED") || kw(&L->cur,"JSONKEYS") ||
        kw(&L->cur,"JHASALL") || kw(&L->cur,"NEEDKEYS") || kw(&L->cur,"PLATEKEYS") ||
        kw(&L->cur,"JSONONLY") || kw(&L->cur,"JONLY") || kw(&L->cur,"JSONSTRICT") ||
        kw(&L->cur,"ONLYP") || kw(&L->cur,"NOEXTRAP") || kw(&L->cur,"JSONONLYP") ||
        kw(&L->cur,"STRICTP") || kw(&L->cur,"ALLOWONLYP") || kw(&L->cur,"PLATEONLYP") ||
        kw(&L->cur,"ONLY_P") || kw(&L->cur,"NOEXTRA_P") || kw(&L->cur,"JSONONLY_P") ||
        kw(&L->cur,"JSONHASANY") || kw(&L->cur,"NEEDJSONANY") || kw(&L->cur,"JHASANY") ||
        kw(&L->cur,"HASANYJSON") || kw(&L->cur,"JSONANYKEY") || kw(&L->cur,"MUSTJSONANY") ||
        kw(&L->cur,"REQUIREJSONANY") || kw(&L->cur,"NEEDANYJSON") || kw(&L->cur,"JSONNEEDANY") ||
        kw(&L->cur,"NOEXTRA") || kw(&L->cur,"ALLOWONLY") || kw(&L->cur,"PLATEONLY") ||
        kw(&L->cur,"JSONALLOW") || kw(&L->cur,"STRICTJSON") ||
        kw(&L->cur,"JSONEXACT") || kw(&L->cur,"JEXACT") || kw(&L->cur,"JSONSCHEMA") ||
        kw(&L->cur,"EXACTKEYS") || kw(&L->cur,"PLATEEXACT") || kw(&L->cur,"SCHEMA") ||
        kw(&L->cur,"EXACTP") || kw(&L->cur,"SCHEMAP") || kw(&L->cur,"PLATEEXACTP") ||
        kw(&L->cur,"KEYSEXACTP") || kw(&L->cur,"SAMEKEYSP") || kw(&L->cur,"PEXACT") ||
        kw(&L->cur,"JSON") || kw(&L->cur,"PLATE")){
      char plate[CUBALC_HOST_STR_MAX], keys_nl[CUBALC_HOST_STR_MAX];
      char arg[CUBALC_HOST_STR_MAX], bag[CUBALC_HOST_STR_MAX];
      int have_plate = 0, want_only = 0, want_exact = 0, want_any = 0;
      size_t olen = 0;
      cubalc_host_result hr, xr;
      if (kw(&L->cur,"JSONEXACT") || kw(&L->cur,"JEXACT") || kw(&L->cur,"JSONSCHEMA") ||
          kw(&L->cur,"EXACTKEYS") || kw(&L->cur,"PLATEEXACT") || kw(&L->cur,"SCHEMA") ||
          kw(&L->cur,"EXACTP") || kw(&L->cur,"SCHEMAP") || kw(&L->cur,"PLATEEXACTP") ||
          kw(&L->cur,"KEYSEXACTP") || kw(&L->cur,"SAMEKEYSP") || kw(&L->cur,"PEXACT")) {
        want_exact = 1;
        lex_next(L);
      } else if (kw(&L->cur,"JSONONLY") || kw(&L->cur,"JONLY") || kw(&L->cur,"JSONSTRICT") ||
          kw(&L->cur,"NOEXTRA") || kw(&L->cur,"ALLOWONLY") || kw(&L->cur,"PLATEONLY") ||
          kw(&L->cur,"JSONALLOW") || kw(&L->cur,"STRICTJSON") ||
          kw(&L->cur,"ONLYP") || kw(&L->cur,"NOEXTRAP") || kw(&L->cur,"JSONONLYP") ||
          kw(&L->cur,"STRICTP") || kw(&L->cur,"ALLOWONLYP") || kw(&L->cur,"PLATEONLYP") ||
          kw(&L->cur,"ONLY_P") || kw(&L->cur,"NOEXTRA_P") || kw(&L->cur,"JSONONLY_P")) {
        want_only = 1;
        lex_next(L);
      } else if (kw(&L->cur,"JSONHASANY") || kw(&L->cur,"NEEDJSONANY") || kw(&L->cur,"JHASANY") ||
                 kw(&L->cur,"HASANYJSON") || kw(&L->cur,"JSONANYKEY") || kw(&L->cur,"MUSTJSONANY") ||
                 kw(&L->cur,"REQUIREJSONANY") || kw(&L->cur,"NEEDANYJSON") || kw(&L->cur,"JSONNEEDANY")) {
        want_any = 1;
        lex_next(L);
      } else if (kw(&L->cur,"JSON") || kw(&L->cur,"PLATE")) {
        lex_next(L);
        /* REQUIRE JSON SUBSET|SUPERSET|COVERS a b — two-token required-field gate */
        if (kw(&L->cur,"SUBSET") || kw(&L->cur,"ISSUBSET") || kw(&L->cur,"IN") ||
            kw(&L->cur,"SUPERSET") || kw(&L->cur,"ISSUPERSET") || kw(&L->cur,"COVERS") ||
            kw(&L->cur,"COVER")) {
          char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
          cubalc_host_result hr, bad;
          int want_super = 0;
          const char *sub, *sup;
          if (kw(&L->cur,"SUPERSET") || kw(&L->cur,"ISSUPERSET") ||
              kw(&L->cur,"COVERS") || kw(&L->cur,"COVER"))
            want_super = 1;
          lex_next(L);
          a[0] = 0;
          b[0] = 0;
          if (resolve_str_arg(vm, L, a, sizeof a) != 0)
            snprintf(a, sizeof a, "%s", vm->last_str);
          if (resolve_str_arg(vm, L, b, sizeof b) != 0)
            b[0] = 0;
          if (!a[0] || !b[0]) {
            fail_at(vm, L,
                    want_super
                        ? "REQUIRE JSON SUPERSET super sub — need two plates"
                        : "REQUIRE JSON SUBSET sub super — need two plates");
            return -1;
          }
          if (want_super) {
            sub = b;
            sup = a;
          } else {
            sub = a;
            sup = b;
          }
          memset(&hr, 0, sizeof hr);
          if (cubalc_host_json_subset(sub, sup, &hr) != 0 || hr.n == 0) {
            char msg[320];
            char flat[160];
            size_t mi = 0;
            const char *mp;
            memset(&bad, 0, sizeof bad);
            cubalc_host_json_subset_bad_keys(sub, sup, &bad);
            flat[0] = 0;
            for (mp = bad.str; *mp && mi + 1 < sizeof flat; mp++) {
              if (*mp == '\n' || *mp == '\r') {
                if (mi > 0 && flat[mi - 1] != ',') {
                  flat[mi++] = ',';
                  if (mi + 1 < sizeof flat) flat[mi++] = ' ';
                }
              } else {
                flat[mi++] = *mp;
              }
            }
            flat[mi] = 0;
            if (flat[0])
              snprintf(msg, sizeof msg,
                       "REQUIRE JSONSUBSET mismatch line %d: %s — soft twin SYS JSONSUBSET",
                       aln, flat);
            else
              snprintf(msg, sizeof msg,
                       "REQUIRE JSONSUBSET mismatch line %d — soft twin SYS JSONSUBSET",
                       aln);
            if (vm->res) vm->res->asserts_fail++;
            fail(vm, msg);
            return -1;
          }
          var_set_str(vm, "LAST", a);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", a);
          vm->last_n = 1;
          var_set_num(vm, "LAST_N", 1);
          var_set_num(vm, "JSONSUBSET_N", 1);
          var_set_num(vm, "JSONSUPERSET_N", 1);
          var_set_num(vm, "REQUIRE_JSONSUBSET", 1);
          var_set_num(vm, "OK", 1);
          if (vm->trace)
            fprintf(vm->trace, "# require json %s ok\n",
                    want_super ? "superset" : "subset");
          if (vm->res) vm->res->asserts_ok++;
          bump(vm); return 1;
        }
        /* REQUIRE JSON EQ a b — two-token equality gate */
        if (kw(&L->cur,"EQ") || kw(&L->cur,"EQUAL") || kw(&L->cur,"SAME") ||
            kw(&L->cur,"EQUALS")) {
          char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
          cubalc_host_result hr, ch;
          lex_next(L);
          a[0] = 0;
          b[0] = 0;
          if (resolve_str_arg(vm, L, a, sizeof a) != 0)
            snprintf(a, sizeof a, "%s", vm->last_str);
          if (resolve_str_arg(vm, L, b, sizeof b) != 0)
            b[0] = 0;
          if (!a[0] || !b[0]) {
            fail_at(vm, L, "REQUIRE JSON EQ a b — need two plates");
            return -1;
          }
          memset(&hr, 0, sizeof hr);
          if (cubalc_host_json_eq(a, b, &hr) != 0 || hr.n == 0) {
            char msg[320];
            char flat[160];
            size_t mi = 0;
            const char *mp;
            memset(&ch, 0, sizeof ch);
            cubalc_host_json_changed_keys(a, b, 0, &ch);
            flat[0] = 0;
            for (mp = ch.str; *mp && mi + 1 < sizeof flat; mp++) {
              if (*mp == '\n' || *mp == '\r') {
                if (mi > 0 && flat[mi - 1] != ',') {
                  flat[mi++] = ',';
                  if (mi + 1 < sizeof flat) flat[mi++] = ' ';
                }
              } else {
                flat[mi++] = *mp;
              }
            }
            flat[mi] = 0;
            if (flat[0])
              snprintf(msg, sizeof msg,
                       "REQUIRE JSONEQ mismatch line %d: %s — soft twin SYS JSONEQ / JSONCHANGED",
                       aln, flat);
            else
              snprintf(msg, sizeof msg,
                       "REQUIRE JSONEQ mismatch line %d — soft twin SYS JSONEQ",
                       aln);
            if (vm->res) vm->res->asserts_fail++;
            fail(vm, msg);
            return -1;
          }
          var_set_str(vm, "LAST", a);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", a);
          vm->last_n = 1;
          var_set_num(vm, "LAST_N", 1);
          var_set_num(vm, "JSONEQ_N", 1);
          var_set_num(vm, "REQUIRE_JSONEQ", 1);
          var_set_num(vm, "OK", 1);
          if (vm->trace)
            fprintf(vm->trace, "# require json eq ok\n");
          if (vm->res) vm->res->asserts_ok++;
          bump(vm); return 1;
        }
        /* REQUIRE JSON NEQ|DIFF|DIFFERENT a b — two-token mutation gate */
        if (kw(&L->cur,"NEQ") || kw(&L->cur,"DIFF") || kw(&L->cur,"DIFFERENT") ||
            kw(&L->cur,"NOTEQ") || kw(&L->cur,"NOTEQUAL") || kw(&L->cur,"UNEQUAL")) {
          char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
          cubalc_host_result hr, ch;
          lex_next(L);
          a[0] = 0;
          b[0] = 0;
          if (resolve_str_arg(vm, L, a, sizeof a) != 0)
            snprintf(a, sizeof a, "%s", vm->last_str);
          if (resolve_str_arg(vm, L, b, sizeof b) != 0)
            b[0] = 0;
          if (!a[0] || !b[0]) {
            fail_at(vm, L, "REQUIRE JSON NEQ a b — need two plates");
            return -1;
          }
          memset(&hr, 0, sizeof hr);
          if (cubalc_host_json_eq(a, b, &hr) != 0) {
            fail_at(vm, L, "REQUIRE JSON NEQ a b — plate compare failed");
            return -1;
          }
          if (hr.n != 0) {
            char msg[320];
            snprintf(msg, sizeof msg,
                     "REQUIRE JSONNEQ still-equal line %d — soft twin SYS JSONNEQ (expect mutation)",
                     aln);
            if (vm->res) vm->res->asserts_fail++;
            fail(vm, msg);
            return -1;
          }
          memset(&ch, 0, sizeof ch);
          cubalc_host_json_changed_keys(a, b, 0, &ch);
          var_set_str(vm, "LAST", a);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", a);
          vm->last_n = 1;
          var_set_num(vm, "LAST_N", 1);
          var_set_num(vm, "JSONNEQ_N", 1);
          var_set_num(vm, "JSONEQ_N", 0);
          var_set_num(vm, "REQUIRE_JSONNEQ", 1);
          var_set_num(vm, "JSONCHANGED_N", ch.n);
          var_set_str(vm, "JSONCHANGED", ch.str);
          var_set_num(vm, "OK", 1);
          if (vm->trace)
            fprintf(vm->trace, "# require json neq ok · changed=%ld\n", ch.n);
          if (vm->res) vm->res->asserts_ok++;
          bump(vm); return 1;
        }
        /* REQUIRE JSON TYPE [plate] key kind — two-token type gate */
        if (kw(&L->cur,"TYPE") || kw(&L->cur,"KIND") || kw(&L->cur,"OFTYPE")) {
          char plate2[CUBALC_HOST_STR_MAX], key2[96], want2[32];
          char a2[CUBALC_HOST_STR_MAX], b2[CUBALC_HOST_STR_MAX];
          const char *got_s = "missing";
          long got_n = 0;
          int n2 = 0, have_w = 0;
          cubalc_host_result gr2;
          lex_next(L);
          plate2[0] = 0; key2[0] = 0; want2[0] = 0;
          a2[0] = 0; b2[0] = 0;
          if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
            if (resolve_str_arg(vm, L, a2, sizeof a2) == 0) n2 = 1;
          }
          if (n2 >= 1 && L->cur.kind == TK_STR) {
            if (resolve_str_arg(vm, L, b2, sizeof b2) == 0) n2 = 2;
          } else if (n2 >= 1 && L->cur.kind == TK_IDENT) {
            const char *id = L->cur.text;
            int looks_kind =
                strcmp(id, "num") == 0 || strcmp(id, "number") == 0 ||
                strcmp(id, "int") == 0 || strcmp(id, "str") == 0 ||
                strcmp(id, "string") == 0 || strcmp(id, "bool") == 0 ||
                strcmp(id, "boolean") == 0 || strcmp(id, "null") == 0 ||
                strcmp(id, "obj") == 0 || strcmp(id, "object") == 0 ||
                strcmp(id, "arr") == 0 || strcmp(id, "array") == 0 ||
                strcmp(id, "list") == 0 || strcmp(id, "missing") == 0;
            if (!looks_kind) {
              if (resolve_str_arg(vm, L, b2, sizeof b2) == 0) n2 = 2;
            }
          }
          if (n2 >= 1 && (kw(&L->cur,"IS") || kw(&L->cur,"AS")))
            lex_next(L);
          if (L->cur.kind == TK_STR) {
            if (resolve_str_arg(vm, L, want2, sizeof want2) == 0) have_w = 1;
          } else if (L->cur.kind == TK_IDENT) {
            snprintf(want2, sizeof want2, "%s", L->cur.text);
            lex_next(L);
            have_w = 1;
          }
          if (n2 == 2 && have_w) {
            snprintf(plate2, sizeof plate2, "%s", a2);
            snprintf(key2, sizeof key2, "%s", b2);
          } else if (n2 == 1 && have_w) {
            snprintf(plate2, sizeof plate2, "%s", vm->last_str);
            snprintf(key2, sizeof key2, "%s", a2);
          } else {
            fail_at(vm, L, "REQUIRE JSON TYPE [plate] key kind — need key and kind");
            return -1;
          }
          {
            char *q;
            for (q = want2; *q; q++)
              if (*q >= 'A' && *q <= 'Z') *q = (char)(*q - 'A' + 'a');
          }
          if (strcmp(want2, "number") == 0 || strcmp(want2, "int") == 0 ||
              strcmp(want2, "integer") == 0)
            snprintf(want2, sizeof want2, "%s", "num");
          else if (strcmp(want2, "string") == 0 || strcmp(want2, "text") == 0)
            snprintf(want2, sizeof want2, "%s", "str");
          else if (strcmp(want2, "boolean") == 0 || strcmp(want2, "flag") == 0)
            snprintf(want2, sizeof want2, "%s", "bool");
          else if (strcmp(want2, "nil") == 0 || strcmp(want2, "none") == 0)
            snprintf(want2, sizeof want2, "%s", "null");
          else if (strcmp(want2, "object") == 0 || strcmp(want2, "map") == 0)
            snprintf(want2, sizeof want2, "%s", "obj");
          else if (strcmp(want2, "array") == 0 || strcmp(want2, "list") == 0)
            snprintf(want2, sizeof want2, "%s", "arr");
          if (!plate2[0]) snprintf(plate2, sizeof plate2, "%s", "{}");
          memset(&gr2, 0, sizeof gr2);
          if (cubalc_host_json_get_raw(plate2, key2, &gr2) != 0) {
            got_s = "missing";
            got_n = 0;
          } else {
            const char *v = gr2.str;
            while (*v == ' ' || *v == '\t' || *v == '\n' || *v == '\r') v++;
            if (*v == '"') { got_s = "str"; got_n = 2; }
            else if (*v == '{') { got_s = "obj"; got_n = 5; }
            else if (*v == '[') { got_s = "arr"; got_n = 6; }
            else if (strncmp(v, "true", 4) == 0 || strncmp(v, "false", 5) == 0) {
              got_s = "bool"; got_n = 3;
            } else if (strncmp(v, "null", 4) == 0) { got_s = "null"; got_n = 4; }
            else if (*v == '-' || (*v >= '0' && *v <= '9')) { got_s = "num"; got_n = 1; }
            else { got_s = "str"; got_n = 2; }
          }
          if (strcmp(got_s, want2) != 0) {
            char msg[320];
            snprintf(msg, sizeof msg,
                     "REQUIRE JSONTYPE line %d: %s is %s, want %s — soft twin SYS JSONTYPE",
                     aln, key2, got_s, want2);
            if (vm->res) vm->res->asserts_fail++;
            fail(vm, msg);
            return -1;
          }
          var_set_str(vm, "LAST", plate2);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", plate2);
          vm->last_n = got_n;
          var_set_num(vm, "LAST_N", got_n);
          var_set_str(vm, "JSONTYPE", got_s);
          var_set_num(vm, "JSONTYPE_N", got_n);
          var_set_str(vm, "JSONTYPE_KEY", key2);
          var_set_str(vm, "JSONTYPE_WANT", want2);
          var_set_num(vm, "REQUIRE_JSONTYPE", 1);
          var_set_num(vm, "OK", 1);
          if (vm->trace)
            fprintf(vm->trace, "# require json type ok · %s is %s\n", key2, got_s);
          if (vm->res) vm->res->asserts_ok++;
          bump(vm); return 1;
        }
        if (kw(&L->cur,"EXACT") || kw(&L->cur,"SCHEMA") || kw(&L->cur,"MATCHKEYS")) {
          want_exact = 1;
          lex_next(L);
        } else if (kw(&L->cur,"ONLY") || kw(&L->cur,"STRICT") || kw(&L->cur,"NOEXTRA") ||
            kw(&L->cur,"ALLOW") || kw(&L->cur,"ALLOWED")) {
          want_only = 1;
          lex_next(L);
        } else if (kw(&L->cur,"HASANY") || kw(&L->cur,"ANY") || kw(&L->cur,"SOME") ||
                   kw(&L->cur,"ANYKEY") || kw(&L->cur,"NEEDANY") || kw(&L->cur,"ANYKEYS")) {
          want_any = 1;
          lex_next(L);
        } else if (kw(&L->cur,"HASALL") || kw(&L->cur,"NEED") || kw(&L->cur,"KEYS") ||
                   kw(&L->cur,"HAS") || kw(&L->cur,"NEEDKEYS")) {
          lex_next(L);
        }
      } else {
        lex_next(L);
      }
      plate[0] = 0;
      keys_nl[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0) {
          const char *s = arg;
          while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
          if (*s == '{') {
            snprintf(plate, sizeof plate, "%s", arg);
            have_plate = 1;
          } else {
            size_t al = strlen(arg);
            if (al + 1 < sizeof keys_nl) {
              memcpy(keys_nl, arg, al + 1);
              olen = al;
            }
          }
        }
      }
      if (have_plate && (kw(&L->cur,"OF") || kw(&L->cur,"KEYS") || kw(&L->cur,"FROM") ||
                         kw(&L->cur,"NEED") || kw(&L->cur,"REQUIRE") ||
                         kw(&L->cur,"ALLOW") || kw(&L->cur,"ALLOWED"))) {
        lex_next(L);
        if (resolve_str_arg(vm, L, arg, sizeof arg) == 0) {
          snprintf(keys_nl, sizeof keys_nl, "%s", arg);
          olen = strlen(keys_nl);
        }
      } else {
        while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          if (L->cur.kind == TK_IDENT &&
              (kw(&L->cur,"END") || kw(&L->cur,"IF") || kw(&L->cur,"ELSE") ||
               kw(&L->cur,"ELIF") || kw(&L->cur,"FOR") || kw(&L->cur,"WHILE") ||
               kw(&L->cur,"LOOP") || kw(&L->cur,"ASSERT") || kw(&L->cur,"PRINT") ||
               kw(&L->cur,"LET") || kw(&L->cur,"SYS") || kw(&L->cur,"JSON") ||
               kw(&L->cur,"REQUIRE") || kw(&L->cur,"PASS") || kw(&L->cur,"FAIL"))) {
            break;
          }
          if (resolve_str_arg(vm, L, arg, sizeof arg) != 0)
            break;
          if (!arg[0]) continue;
          if (olen > 0) {
            if (olen + 1 >= sizeof keys_nl) break;
            keys_nl[olen++] = '\n';
            keys_nl[olen] = 0;
          }
          {
            size_t al = strlen(arg);
            if (olen + al + 1 >= sizeof keys_nl) break;
            memcpy(keys_nl + olen, arg, al + 1);
            olen += al;
          }
        }
      }
      if (!have_plate)
        snprintf(plate, sizeof plate, "%s", vm->last_str);
      if (!plate[0])
        snprintf(plate, sizeof plate, "%s", "{}");
      if (!keys_nl[0]) {
        fail_at(vm, L, want_exact
                ? "REQUIRE EXACTP/JSONEXACT [plate] key… — need schema keys"
                : (want_only
                   ? "REQUIRE ONLYP/JSONONLY [plate] allow… — need allow-list keys"
                   : (want_any
                      ? "REQUIRE JSONHASANY [plate] key… — need at least one candidate key"
                      : "REQUIRE JSONHASALL [plate] key… — need at least one key")));
        return -1;
      }
      if (want_exact) {
        char miss_flat[120], extra_flat[120];
        char msg[320];
        size_t mi;
        const char *mp;
        int bad = 0;
        memset(&hr, 0, sizeof hr);
        memset(&xr, 0, sizeof xr);
        cubalc_host_json_has_keys(plate, keys_nl, 1, &hr);
        cubalc_host_json_filter_plate_keys(plate, keys_nl, 1, &xr);
        miss_flat[0] = 0;
        extra_flat[0] = 0;
        if (hr.n == 0) {
          cubalc_host_result mr;
          memset(&mr, 0, sizeof mr);
          cubalc_host_json_filter_req_keys(plate, keys_nl, 0, &mr);
          mi = 0;
          for (mp = mr.str; *mp && mi + 1 < sizeof miss_flat; mp++) {
            if (*mp == '\n' || *mp == '\r') {
              if (mi > 0 && miss_flat[mi - 1] != ',') {
                miss_flat[mi++] = ',';
                if (mi + 1 < sizeof miss_flat) miss_flat[mi++] = ' ';
              }
            } else {
              miss_flat[mi++] = *mp;
            }
          }
          miss_flat[mi] = 0;
          if (!miss_flat[0]) snprintf(miss_flat, sizeof miss_flat, "?");
          bad = 1;
        }
        if (xr.n > 0) {
          mi = 0;
          for (mp = xr.str; *mp && mi + 1 < sizeof extra_flat; mp++) {
            if (*mp == '\n' || *mp == '\r') {
              if (mi > 0 && extra_flat[mi - 1] != ',') {
                extra_flat[mi++] = ',';
                if (mi + 1 < sizeof extra_flat) extra_flat[mi++] = ' ';
              }
            } else {
              extra_flat[mi++] = *mp;
            }
          }
          extra_flat[mi] = 0;
          if (!extra_flat[0]) snprintf(extra_flat, sizeof extra_flat, "?");
          bad = 1;
        }
        if (bad) {
          if (miss_flat[0] && extra_flat[0])
            snprintf(msg, sizeof msg,
                     "REQUIRE EXACTP/JSONEXACT line %d missing: %s · extra: %s — soft EXACTP / SYS JSONEXACT",
                     aln, miss_flat, extra_flat);
          else if (miss_flat[0])
            snprintf(msg, sizeof msg,
                     "REQUIRE EXACTP/JSONEXACT line %d missing: %s — soft EXACTP / SYS JSONEXACT",
                     aln, miss_flat);
          else
            snprintf(msg, sizeof msg,
                     "REQUIRE EXACTP/JSONEXACT line %d extra: %s — soft EXACTP / SYS JSONEXACT",
                     aln, extra_flat);
          if (vm->res) vm->res->asserts_fail++;
          fail(vm, msg);
          return -1;
        }
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_num(vm, "JSONEXACT_N", 1);
        var_set_num(vm, "JSONEXACT_HIT", (long)hr.code);
        var_set_num(vm, "JSONEXACT_EXTRA", 0);
        var_set_num(vm, "REQUIRE_JSON_HIT", (long)hr.code);
        var_set_num(vm, "REQUIRE_JSON_EXTRA", 0);
        var_set_num(vm, "OK", 1);
        if (vm->trace)
          fprintf(vm->trace, "# require jsonexact hit=%d ok\n", hr.code);
        if (vm->res) vm->res->asserts_ok++;
        bump(vm); return 1;
      }
      if (want_only) {
        memset(&xr, 0, sizeof xr);
        if (cubalc_host_json_filter_plate_keys(plate, keys_nl, 1, &xr) != 0 ||
            xr.n > 0) {
          char msg[280];
          char flat[160];
          size_t mi = 0;
          const char *mp;
          bag[0] = 0;
          if (xr.str[0])
            snprintf(bag, sizeof bag, "%s", xr.str);
          flat[0] = 0;
          for (mp = bag; *mp && mi + 1 < sizeof flat; mp++) {
            if (*mp == '\n' || *mp == '\r') {
              if (mi > 0 && flat[mi - 1] != ',') {
                flat[mi++] = ',';
                if (mi + 1 < sizeof flat) flat[mi++] = ' ';
              }
            } else {
              flat[mi++] = *mp;
            }
          }
          flat[mi] = 0;
          if (!flat[0])
            snprintf(flat, sizeof flat, "?");
          snprintf(msg, sizeof msg,
                   "REQUIRE ONLYP/JSONONLY extra line %d: %s — soft twin ONLYP / SYS JSONEXTRA",
                   aln, flat);
          if (vm->res) vm->res->asserts_fail++;
          fail(vm, msg);
          return -1;
        }
        var_set_str(vm, "LAST", plate);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
        vm->last_n = 1;
        var_set_num(vm, "LAST_N", 1);
        var_set_num(vm, "JSONEXTRA_N", 0);
        var_set_num(vm, "REQUIRE_JSON_EXTRA", 0);
        var_set_num(vm, "REQUIRE_JSON_HIT", 1);
        var_set_num(vm, "OK", 1);
        if (vm->trace)
          fprintf(vm->trace, "# require jsononly ok (no extras)\n");
        if (vm->res) vm->res->asserts_ok++;
        bump(vm); return 1;
      }
      memset(&hr, 0, sizeof hr);
      /* want_any=1 → any-of (JSONHASANY); else all-of (JSONHASALL) */
      if (cubalc_host_json_has_keys(plate, keys_nl, want_any ? 0 : 1, &hr) != 0 || hr.n == 0) {
        char msg[280];
        char miss_flat[160];
        size_t mi = 0;
        const char *mp;
        memset(&xr, 0, sizeof xr);
        cubalc_host_json_filter_req_keys(plate, keys_nl, 0, &xr);
        /* flatten missing bag newlines → comma list for one-line fail */
        bag[0] = 0;
        if (xr.str[0])
          snprintf(bag, sizeof bag, "%s", xr.str);
        miss_flat[0] = 0;
        for (mp = bag; *mp && mi + 1 < sizeof miss_flat; mp++) {
          if (*mp == '\n' || *mp == '\r') {
            if (mi > 0 && miss_flat[mi - 1] != ',') {
              miss_flat[mi++] = ',';
              if (mi + 1 < sizeof miss_flat) miss_flat[mi++] = ' ';
            }
          } else {
            miss_flat[mi++] = *mp;
          }
        }
        miss_flat[mi] = 0;
        if (!miss_flat[0])
          snprintf(miss_flat, sizeof miss_flat, "?");
        if (want_any)
          snprintf(msg, sizeof msg,
                   "REQUIRE JSONHASANY none line %d: need one of %s — soft twin SYS JSONHASANY",
                   aln, miss_flat);
        else
          snprintf(msg, sizeof msg,
                   "REQUIRE JSONHASALL missing line %d: %s — soft twin SYS JSONHASALL / JSONMISS",
                   aln, miss_flat);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
      var_set_str(vm, "LAST", plate);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", plate);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      if (want_any) {
        var_set_num(vm, "JSONHASANY_N", 1);
        var_set_num(vm, "JSONHASANY_HIT", (long)hr.code);
        var_set_num(vm, "REQUIRE_JSONHASANY", 1);
        var_set_num(vm, "REQUIRE_JSON_HIT", (long)hr.code);
      } else {
        var_set_num(vm, "JSONHASALL_N", 1);
        var_set_num(vm, "JSONHASALL_HIT", (long)hr.code);
        var_set_num(vm, "REQUIRE_JSON_HIT", (long)hr.code);
      }
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# require %s hit=%d ok\n",
                want_any ? "jsonhasany" : "jsonhasall", hr.code);
      if (vm->res) vm->res->asserts_ok++;
      bump(vm); return 1;
    }

    if (!kw(&L->cur, "VERSION") && !kw(&L->cur, "VER") && !kw(&L->cur, "LANG") &&
        !kw(&L->cur, "CUBALC") && L->cur.kind != TK_STR) {
      fail(vm, "REQUIRE VERSION|LIB|ENV|ARG|PATH|DIR|BIN|FORM|JSONHASALL|JSONHASANY|ONLYP|EXACTP|…");
      return -1;
    }
    if (kw(&L->cur, "VERSION") || kw(&L->cur, "VER") || kw(&L->cur, "LANG") ||
        kw(&L->cur, "CUBALC"))
      lex_next(L);
    if (L->cur.kind != TK_STR && L->cur.kind != TK_IDENT && L->cur.kind != TK_NUM) {
      fail(vm, "REQUIRE VERSION \"x.y[.z]\" | REQUIRE RESTARGS|LIB|ENV|JSONHASANY|ONLYP|…");
      return -1;
    }
    {
      char need[64];
      const char *have = CUBALC_LANG_VERSION;
      long hn[3] = {0,0,0}, nn[3] = {0,0,0};
      int hi = 0, ni = 0;
      char *p;
      char hbuf[64], nbuf[64];
      if (L->cur.kind == TK_NUM)
        snprintf(need, sizeof need, "%ld", L->cur.num);
      else
        snprintf(need, sizeof need, "%s", L->cur.text);
      lex_next(L);
      snprintf(hbuf, sizeof hbuf, "%s", have);
      snprintf(nbuf, sizeof nbuf, "%s", need);
      for (p = hbuf; *p && hi < 3; ) {
        if (*p >= '0' && *p <= '9') {
          long v = 0;
          while (*p >= '0' && *p <= '9') { v = v * 10 + (*p - '0'); p++; }
          hn[hi++] = v;
          if (*p == '.' || *p == '-') p++;
          else break;
        } else p++;
      }
      for (p = nbuf; *p && ni < 3; ) {
        if (*p >= '0' && *p <= '9') {
          long v = 0;
          while (*p >= '0' && *p <= '9') { v = v * 10 + (*p - '0'); p++; }
          nn[ni++] = v;
          if (*p == '.' || *p == '-') p++;
          else break;
        } else p++;
      }
      {
        int okv = 1, i;
        for (i = 0; i < 3; i++) {
          if (hn[i] > nn[i]) break;
          if (hn[i] < nn[i]) { okv = 0; break; }
        }
        if (!okv) {
          char msg[200];
          snprintf(msg, sizeof msg,
                   "REQUIRE VERSION %s failed: have %s — upgrade runtime · cubalc version",
                   need, have);
          fail(vm, msg);
          return -1;
        }
      }
      var_set_str(vm, "LAST", have);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", have);
      var_set_num(vm, "LAST_N", 1);
      vm->last_n = 1;
      var_set_num(vm, "OK", 1);
      if (vm->res) vm->res->asserts_ok++;
      bump(vm);
      return 1;
    }
  }

  /* ZERO name — numeric 0. CLEARSTR|EMPTYTO name — empty string. */
  if (kw(&L->cur, "ZERO") || kw(&L->cur, "ZEROV") || kw(&L->cur, "ZEROOUT") ||
      kw(&L->cur, "CLEARSTR") || kw(&L->cur, "EMPTYTO") || kw(&L->cur, "CLEARS") ||
      kw(&L->cur, "BLANKTO") || kw(&L->cur, "ERASESTR")) {
    char op[24], name[48];
    int is_zero = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "ZERO") == 0 || strcmp(op, "ZEROV") == 0 || strcmp(op, "ZEROOUT") == 0)
      is_zero = 1;
    lex_next(L);
    if (L->cur.kind != TK_IDENT) {
      fail(vm, is_zero ? "ZERO needs name — ZERO retries" : "CLEARSTR needs name — CLEARSTR msg");
      return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    if (is_zero) {
      var_set_num(vm, name, 0);
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_str(vm, "LAST", "0");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "0");
      var_set_num(vm, "ZERO_N", 1);
    } else {
      var_set_str(vm, name, "");
      var_set_str(vm, "LAST", "");
      vm->last_str[0] = 0;
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      var_set_num(vm, "CLEARSTR_N", 1);
    }
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# %s %s\n", is_zero ? "ZERO" : "CLEARSTR", name);
    bump(vm);
    return 1;
  }
  return 0;
}
