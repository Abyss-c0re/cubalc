#define _POSIX_C_SOURCE 200809L
#include "cubalc_lang.h"
#include "cubalc_algocube.h"
#include "cubalc_cubechain.h"
#include "cubalc_async.h"
#include "cubalc_hw.h"
#include "cubalc_hostops.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

/* CubalC lang — place/plug/pulse/flow/look. Grammar = ops, not prose. */

enum {
  TK_EOF=0, TK_IDENT, TK_NUM, TK_STR, TK_NL,
  TK_LBRACK, TK_RBRACK, TK_TILDE, TK_BANG, TK_QMARK, TK_COLON, TK_PIPE,
  TK_PLUS, TK_MINUS, TK_STAR, TK_SLASH, TK_PERCENT,
  TK_EQ, TK_EQEQ, TK_NE, TK_LT, TK_LE, TK_GT, TK_GE,
  TK_LPAREN, TK_RPAREN, TK_COMMA
};

typedef struct { int kind; long num; char text[8192]; int line; } Tok;
typedef struct { const char *s; size_t n, i; int line; size_t tok_off; Tok cur; } Lex;
typedef struct { char name[48]; long val; char sval[512]; int is_str; } Var;

typedef struct {
  char name[48];
  const char *body;
  size_t len;
} FnDef;

#define CUBALC_CELL_N   64
#define CUBALC_STACK_N  32
typedef struct {
  cubalc_chain ch;
  Var vars[128];
  int n_vars;
  FnDef fns[32];
  int n_fns;
  cubalc_run_result *res;
  FILE *trace;
  int hold_flash;
  int fatal;
  int break_loop;
  int continue_loop;
  int return_fn; /* digit-4: RET from FN body */
  uint32_t rng;  /* digit-6: seeded RNG state */
  char err[160];
  char creed[80];
  char chunk[40][48];
  int n_chunk;
  char last_str[CUBALC_HOST_STR_MAX];
  int last_code;
  long last_n;
  char include_base[512];
  /* digit-1 data plane: integer cells + stack */
  long cells[CUBALC_CELL_N];
  long stack[CUBALC_STACK_N];
  int sp; /* stack depth 0..CUBALC_STACK_N */
} VM;

static void fail(VM *vm, const char *msg) {
  if (vm->fatal) return;
  vm->fatal = 1;
  snprintf(vm->err, sizeof vm->err, "%s", msg);
  if (vm->res) {
    vm->res->ok = 0;
    snprintf(vm->res->err, sizeof vm->res->err, "%s", msg);
  }
}
static void bump(VM *vm) { if (vm->res) vm->res->stmts++; }
static int kw(const Tok *t, const char *k) {
  return t->kind == TK_IDENT && strcasecmp(t->text, k) == 0;
}

static void lex_skip(Lex *L) {
  while (L->i < L->n) {
    char c = L->s[L->i];
    if (c==' '||c=='\t'||c=='\r') { L->i++; continue; }
    if (c=='#' || (c=='/' && L->i+1<L->n && L->s[L->i+1]=='/')) {
      while (L->i < L->n && L->s[L->i] != '\n') L->i++;
      continue;
    }
    break;
  }
}
static int is_id0(int c){ return isalpha(c)||c=='_'; }
static int is_id(int c){ return isalnum(c)||c=='_'||c=='-'||c=='.'; }

static void lex_next(Lex *L) {
  lex_skip(L);
  L->tok_off = L->i; /* start of current token (for FN body capture) */
  L->cur.line = L->line;
  L->cur.text[0]=0; L->cur.num=0;
  if (L->i >= L->n) { L->cur.kind = TK_EOF; return; }
  char c = L->s[L->i];
  if (c=='\n'){ L->i++; L->line++; L->cur.kind=TK_NL; return; }
  if (c=='['){ L->i++; L->cur.kind=TK_LBRACK; return; }
  if (c==']'){ L->i++; L->cur.kind=TK_RBRACK; return; }
  if (c=='~'){ L->i++; L->cur.kind=TK_TILDE; return; }
  if (c=='?'){ L->i++; L->cur.kind=TK_QMARK; return; }
  if (c==':'){ L->i++; L->cur.kind=TK_COLON; return; }
  if (c=='|'){ L->i++; L->cur.kind=TK_PIPE; return; }
  if (c=='!'){
    L->i++;
    if (L->i<L->n && L->s[L->i]=='='){ L->i++; L->cur.kind=TK_NE; return; }
    L->cur.kind=TK_BANG; return;
  }
  if (c=='"'){
    L->i++; size_t k=0;
    while (L->i<L->n && L->s[L->i]!='"'){
      char ch = L->s[L->i];
      if (ch=='\\' && L->i+1<L->n){
        char e = L->s[L->i+1];
        L->i += 2;
        if (e=='n') ch='\n';
        else if (e=='t') ch='\t';
        else if (e=='r') ch='\r';
        else if (e=='"') ch='"';
        else if (e=='\\') ch='\\';
        else ch=e;
        if (k+1<sizeof L->cur.text) L->cur.text[k++]=ch;
        continue;
      }
      if (ch=='\n') L->line++;
      if (k+1<sizeof L->cur.text) L->cur.text[k++]=ch;
      L->i++;
    }
    L->cur.text[k]=0; if (L->i<L->n) L->i++;
    L->cur.kind=TK_STR; return;
  }
  if (isdigit((unsigned char)c)){
    char b[64]; size_t k=0;
    /* 0x… / 0X… hex integer literals (universal data-path) */
    if (c=='0' && L->i+1<L->n && (L->s[L->i+1]=='x' || L->s[L->i+1]=='X')){
      b[k++]=L->s[L->i++]; /* 0 */
      b[k++]=L->s[L->i++]; /* x */
      while (L->i<L->n && isxdigit((unsigned char)L->s[L->i])){
        if (k+1<sizeof b) b[k++]=L->s[L->i];
        L->i++;
      }
      b[k]=0;
      L->cur.num = strtoul(b, NULL, 16);
      L->cur.kind = TK_NUM;
      snprintf(L->cur.text, sizeof L->cur.text, "%s", b);
      return;
    }
    while (L->i<L->n && isdigit((unsigned char)L->s[L->i])){
      if (k+1<sizeof b) b[k++]=L->s[L->i]; L->i++;
    }
    /* 2DUP / 2DROP / 2SWAP / 2OVER / 2ROT / 2NIP — Forth double ops (digit-8) */
    if (k==1 && b[0]=='2' && L->i<L->n && isalpha((unsigned char)L->s[L->i])){
      size_t j = L->i;
      char tail[16]; size_t t=0;
      while (j<L->n && isalpha((unsigned char)L->s[j]) && t+1<sizeof tail)
        tail[t++]=L->s[j++];
      tail[t]=0;
      if (strcasecmp(tail,"DUP")==0 || strcasecmp(tail,"DROP")==0 ||
          strcasecmp(tail,"SWAP")==0 || strcasecmp(tail,"OVER")==0 ||
          strcasecmp(tail,"ROT")==0 || strcasecmp(tail,"NIP")==0){
        L->i = j;
        snprintf(L->cur.text, sizeof L->cur.text, "2%s", tail);
        /* normalize to upper for kw() which is case-insensitive anyway */
        for (char *p=L->cur.text; *p; p++)
          if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
        L->cur.kind = TK_IDENT;
        return;
      }
    }
    b[k]=0; L->cur.num=strtol(b,NULL,10); L->cur.kind=TK_NUM;
    snprintf(L->cur.text,sizeof L->cur.text,"%s",b); return;
  }
  if (c=='+'){ L->i++; L->cur.kind=TK_PLUS; return; }
  if (c=='-'){ L->i++; L->cur.kind=TK_MINUS; return; }
  if (c=='*'){ L->i++; L->cur.kind=TK_STAR; return; }
  if (c=='/'){ L->i++; L->cur.kind=TK_SLASH; return; }
  if (c=='%'){ L->i++; L->cur.kind=TK_PERCENT; return; }
  if (c=='('){ L->i++; L->cur.kind=TK_LPAREN; return; }
  if (c==')'){ L->i++; L->cur.kind=TK_RPAREN; return; }
  if (c==','){ L->i++; L->cur.kind=TK_COMMA; return; }
  if (c=='='){
    L->i++;
    if (L->i<L->n && L->s[L->i]=='='){ L->i++; L->cur.kind=TK_EQEQ; return; }
    L->cur.kind=TK_EQ; return;
  }
  if (c=='<'){
    L->i++;
    if (L->i<L->n && L->s[L->i]=='='){ L->i++; L->cur.kind=TK_LE; return; }
    L->cur.kind=TK_LT; return;
  }
  if (c=='>'){
    L->i++;
    if (L->i<L->n && L->s[L->i]=='='){ L->i++; L->cur.kind=TK_GE; return; }
    L->cur.kind=TK_GT; return;
  }
  if (is_id0((unsigned char)c)){
    size_t k=0;
    while (L->i<L->n && is_id((unsigned char)L->s[L->i])){
      if (k+1<sizeof L->cur.text) L->cur.text[k++]=L->s[L->i];
      L->i++;
    }
    L->cur.text[k]=0; L->cur.kind=TK_IDENT; return;
  }
  /* skip unknown */
  L->i++; L->cur.kind=TK_NL;
}

static void lex_init(Lex *L, const char *s, size_t n){
  L->s=s; L->n=n; L->i=0; L->line=1; lex_next(L);
}
static void skip_nl(Lex *L){ while (L->cur.kind==TK_NL) lex_next(L); }

static int find_cube(VM *vm, const char *id){
  for (int i=0;i<vm->ch.n_cubes;i++)
    if (strcmp(vm->ch.cubes[i].id, id)==0) return i;
  return -1;
}
static void ensure_world(VM *vm){
  if (vm->ch.n_cubes > 0 || vm->ch.initial.n > 0) return;
  cubalc_matrix gen;
  cubalc_coord_to_matrix(
    "NEXUS_COORD v1 | from=play | type=world | hold_flash=1 | visual=units |", &gen);
  cubalc_chain_from_initial(&vm->ch, &gen, 1);
  vm->ch.hold_flash = (uint8_t)vm->hold_flash;
  snprintf(vm->ch.creed, sizeof vm->ch.creed, "%s",
           vm->creed[0]?vm->creed:CUBALC_CREED);
}
static void chunk_push(VM *vm, const char *id){
  if (vm->n_chunk >= 40) return;
  snprintf(vm->chunk[vm->n_chunk], sizeof vm->chunk[0], "%s", id);
  vm->n_chunk++;
}

static Var *var_get(VM *vm, const char *name, int create) {
  for (int i=0;i<vm->n_vars;i++)
    if (strcmp(vm->vars[i].name, name)==0) return &vm->vars[i];
  if (!create || vm->n_vars >= 128) return NULL;
  Var *v = &vm->vars[vm->n_vars++];
  memset(v, 0, sizeof *v);
  snprintf(v->name, sizeof v->name, "%s", name);
  return v;
}
static void var_set_num(VM *vm, const char *name, long val) {
  Var *v = var_get(vm, name, 1);
  if (v) { v->val = val; v->is_str = 0; v->sval[0]=0; }
}
static void var_set_str(VM *vm, const char *name, const char *s) {
  Var *v = var_get(vm, name, 1);
  if (v) {
    v->is_str = 1;
    snprintf(v->sval, sizeof v->sval, "%s", s ? s : "");
    v->val = (long)strlen(v->sval);
  }
}

static int place_cube(VM *vm, const char *id, const char *role, int proton){
  ensure_world(vm);
  if (find_cube(vm, id) >= 0) return find_cube(vm, id); /* already placed */
  float x = (float)(vm->ch.n_cubes % 5) * 0.28f;
  float z = (float)(vm->ch.n_cubes / 5) * 0.28f;
  if (cubalc_cube_spawn(&vm->ch, id, role && role[0]?role:id,
                        (uint8_t)(proton?1:0), x, 0.f, z) < 0) {
    fail(vm, "world full — budget");
    return -1;
  }
  chunk_push(vm, id);
  return find_cube(vm, id);
}
static void do_plug(VM *vm, const char *a, const char *b){
  int ia=find_cube(vm,a), ib=find_cube(vm,b);
  if (ia<0){ place_cube(vm,a,a,1); ia=find_cube(vm,a); }
  if (ib<0){ place_cube(vm,b,b,1); ib=find_cube(vm,b); }
  if (ia<0||ib<0){ fail(vm,"plug missing unit"); return; }
  cubalc_cube_plug(&vm->ch, ia, ib);
}
/* Only CUBE is defined — I/O is pluggable; reverse flips IN/OUT on the wire. */
static void do_reverse(VM *vm, const char *a, const char *b){
  int ia=find_cube(vm,a), ib=find_cube(vm,b);
  if (ia<0||ib<0){ fail(vm,"REVERSE needs two units"); return; }
  int rc = cubalc_cube_reverse(&vm->ch, ia, ib);
  if (rc < 0){ fail(vm,"REVERSE: no plug between units (pluggable I/O only)"); return; }
  var_set_num(vm, "REVERSED", rc);
  var_set_num(vm, "OK", 1);
}
static void do_unplug(VM *vm, const char *a, const char *b){
  int ia=find_cube(vm,a), ib=find_cube(vm,b);
  if (ia<0||ib<0){ fail(vm,"UNPLUG needs two units"); return; }
  cubalc_cube_unplug(&vm->ch, ia, ib);
  var_set_num(vm, "OK", 1);
}
static void do_io(VM *vm, const char *id, int face, int is_out){
  int ix=find_cube(vm,id);
  if (ix<0){ place_cube(vm,id,"io",1); ix=find_cube(vm,id); }
  if (ix<0){ fail(vm,"IO unit missing"); return; }
  int rc = cubalc_cube_io(&vm->ch, ix, face,
    is_out ? CUBALC_PORT_OUT : CUBALC_PORT_IN);
  if (rc < 0){ fail(vm,"IO port full"); return; }
  var_set_num(vm, "OK", 1);
  var_set_num(vm, "PORT", rc);
}
/* Nest child inside parent — units may nest. */
static void do_nest(VM *vm, const char *parent, const char *child){
  int ip=find_cube(vm,parent), ic=find_cube(vm,child);
  if (ip<0){ place_cube(vm,parent,"shell",1); ip=find_cube(vm,parent); }
  if (ic<0){ place_cube(vm,child,"inner",1); ic=find_cube(vm,child); }
  if (ip<0||ic<0){ fail(vm,"NEST parent child — missing unit"); return; }
  int rc = cubalc_cube_nest(&vm->ch, ip, ic);
  if (rc == -2){ fail(vm,"NEST cycle or depth limit"); return; }
  if (rc < 0){ fail(vm,"NEST failed"); return; }
  var_set_num(vm, "OK", 1);
  var_set_num(vm, "NESTED", 1);
  var_set_num(vm, "PARENT", ip);
}
static void do_unnest(VM *vm, const char *child){
  int ic=find_cube(vm,child);
  if (ic<0){ fail(vm,"UNNEST needs unit"); return; }
  cubalc_cube_unnest(&vm->ch, ic);
  var_set_num(vm, "OK", 1);
}
/* Law: each cube compiles into a matrix. No flow — no compiling. */
static void do_compile_cube(VM *vm, const char *id){
  int ix=find_cube(vm,id);
  if (ix<0){ fail(vm,"COMPILE needs unit"); return; }
  int rc = cubalc_cube_compile(&vm->ch, ix);
  var_set_num(vm, "COMPILE_RC", rc);
  if (rc == -2){
    var_set_num(vm, "OK", 0);
    var_set_num(vm, "COMPILED", 0);
    /* soft fail: law gate, not fatal — program may ASSERT no-compile */
    if (vm->trace) fprintf(vm->trace, "# COMPILE blocked: no flow on %s\n", id);
    return;
  }
  if (rc == -3){
    var_set_num(vm, "OK", 0);
    var_set_num(vm, "COMPILED", 0);
    if (vm->trace) fprintf(vm->trace, "# COMPILE blocked: nested child not compiled for %s\n", id);
    return;
  }
  if (rc < 0){ fail(vm,"COMPILE failed"); return; }
  var_set_num(vm, "OK", 1);
  var_set_num(vm, "COMPILED", 1);
  var_set_num(vm, "SET", (long)vm->ch.cubes[ix].compiled_matrix.set);
}
static void do_compile_all(VM *vm){
  ensure_world(vm);
  int failed = -1;
  int rc = cubalc_chain_compile(&vm->ch, &failed);
  var_set_num(vm, "COMPILE_RC", rc);
  var_set_num(vm, "FAILED", failed);
  if (rc == -2){
    var_set_num(vm, "OK", 0);
    var_set_num(vm, "COMPILED", 0);
    if (vm->trace) fprintf(vm->trace, "# COMPILE ALL blocked: no flow (ix=%d)\n", failed);
    return;
  }
  if (rc < 0){
    var_set_num(vm, "OK", 0);
    if (vm->trace) fprintf(vm->trace, "# COMPILE ALL rc=%d ix=%d\n", rc, failed);
    return;
  }
  int ncomp = 0;
  for (int i = 0; i < vm->ch.n_cubes; i++)
    if (vm->ch.cubes[i].compiled) ncomp++;
  var_set_num(vm, "OK", 1);
  var_set_num(vm, "COMPILED", ncomp);
}
static void do_ring(VM *vm){
  int n = vm->ch.n_cubes;
  if (n < 2) return;
  for (int i=0;i<n;i++) cubalc_cube_plug(&vm->ch, i, (i+1)%n);
}
static void do_flow(VM *vm, int n){
  if (n < 1) n = 1;
  if (n > 1000) n = 1000;
  ensure_world(vm);
  /* async parallel energy flow — energy must flow (CPU workers; GPU-shaped path ready) */
  if (cubalc_async_chain_flow(&vm->ch, n) != 0) {
    for (int i=0;i<n;i++) cubalc_chain_flow(&vm->ch);
  }
}
static void do_show(VM *vm, const char *id){
  FILE *o = vm->trace ? vm->trace : stdout;
  ensure_world(vm);
  if (id && id[0] && find_cube(vm,id)>=0)
    cubalc_cube_print_spin(&vm->ch, id, o);
  else
    cubalc_chain_print_cubes(&vm->ch, o);
}

/* forward */
static int parse_form(VM *vm, Lex *L);
static int parse_cube(VM *vm, Lex *L);
static void do_deconstruct(VM *vm, const char *id);
static void do_reconstruct(VM *vm, const char *id);
static long do_decide(VM *vm, const char *id);
static long *var_slot(VM *vm, const char *name, int create);
static int exec_stmts_until(VM *vm, Lex *L, const char *stop1, const char *stop2);
static long parse_expr(VM *vm, Lex *L);

/* Parse inside [ ... ] already consumed '[' */
static int parse_cube_body(VM *vm, Lex *L){
  skip_nl(L);
  if (L->cur.kind == TK_RBRACK){ lex_next(L); bump(vm); return 1; }

  /* nested chunk: [ [a] [b] [c] ]  → place children, ring them */
  if (L->cur.kind == TK_LBRACK){
    int save = vm->n_chunk;
    vm->n_chunk = 0;
    char local[40][48]; int nloc=0;
    while (L->cur.kind != TK_RBRACK && L->cur.kind != TK_EOF && !vm->fatal){
      skip_nl(L);
      if (L->cur.kind == TK_RBRACK) break;
      if (L->cur.kind == TK_LBRACK){
        int before = vm->ch.n_cubes;
        if (parse_cube(vm, L) < 0) return -1;
        /* collect ids placed in this nested form if single place */
        (void)before;
      } else if (L->cur.kind == TK_TILDE){
        lex_next(L);
        int n = 1;
        if (L->cur.kind==TK_NUM){ n=(int)L->cur.num; lex_next(L); }
        do_flow(vm, n); bump(vm);
      } else if (L->cur.kind == TK_QMARK){
        lex_next(L);
        char id[48]={0};
        if (L->cur.kind==TK_IDENT){ snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L); }
        do_show(vm, id[0]?id:NULL); bump(vm);
      } else if (L->cur.kind == TK_IDENT || L->cur.kind == TK_NUM){
        /* bare word inside chunk treated as place */
        char id[48]; snprintf(id,sizeof id,"%s", L->cur.kind==TK_NUM?L->cur.text:L->cur.text);
        lex_next(L);
        char role[48]; snprintf(role,sizeof role,"%s",id);
        int proton=1;
        if (L->cur.kind==TK_COLON){
          lex_next(L);
          if (L->cur.kind==TK_IDENT||L->cur.kind==TK_NUM){
            snprintf(role,sizeof role,"%s",L->cur.text); lex_next(L);
          }
        }
        if (L->cur.kind==TK_BANG){
          lex_next(L); proton=1;
          if (L->cur.kind==TK_NUM){ proton=L->cur.num?1:0; lex_next(L); }
          place_cube(vm,id,role,proton);
          cubalc_chain_impulse(&vm->ch, id, (uint8_t)proton);
        } else {
          place_cube(vm,id,role,proton);
        }
        if (nloc<40){ snprintf(local[nloc],sizeof local[0],"%s",id); nloc++; }
        bump(vm);
      } else {
        lex_next(L);
      }
    }
    if (L->cur.kind != TK_RBRACK){ fail(vm,"chunk missing ]"); return -1; }
    lex_next(L);
    /* ring cubes placed in this chunk session */
    if (vm->n_chunk >= 2){
      for (int i=0;i<vm->n_chunk;i++){
        int a=find_cube(vm, vm->chunk[i]);
        int b=find_cube(vm, vm->chunk[(i+1)%vm->n_chunk]);
        if (a>=0&&b>=0) cubalc_cube_plug(&vm->ch,a,b);
      }
    } else if (nloc>=2){
      for (int i=0;i<nloc;i++){
        int a=find_cube(vm, local[i]);
        int b=find_cube(vm, local[(i+1)%nloc]);
        if (a>=0&&b>=0) cubalc_cube_plug(&vm->ch,a,b);
      }
    }
    vm->n_chunk = save;
    bump(vm);
    return 1;
  }

  /* [~n] flow */
  if (L->cur.kind == TK_TILDE){
    lex_next(L);
    int n=1;
    if (L->cur.kind==TK_NUM){ n=(int)L->cur.num; lex_next(L); }
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[~n] needs ]"); return -1; }
    lex_next(L);
    do_flow(vm,n); bump(vm); return 1;
  }

  /* [?] or [?name] */
  if (L->cur.kind == TK_QMARK){
    lex_next(L);
    char id[48]={0};
    if (L->cur.kind==TK_IDENT){ snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L); }
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[?] needs ]"); return -1; }
    lex_next(L);
    do_show(vm, id[0]?id:NULL); bump(vm); return 1;
  }

  /* keywords inside cube: hold, ring, os, genesis, share */
  if (kw(&L->cur,"hold") || kw(&L->cur,"HOLD_FLASH")){
    lex_next(L);
    int v=1;
    if (L->cur.kind==TK_NUM){ v=L->cur.num?1:0; lex_next(L); }
    vm->hold_flash=v; vm->ch.hold_flash=(uint8_t)v;
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[hold] needs ]"); return -1; }
    lex_next(L); bump(vm); return 1;
  }
  if (kw(&L->cur,"ring")){
    lex_next(L);
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[ring]"); return -1; }
    lex_next(L); do_ring(vm); bump(vm); return 1;
  }
  if (kw(&L->cur,"os") || kw(&L->cur,"OS_ASPECTS")){
    lex_next(L);
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[os]"); return -1; }
    lex_next(L); ensure_world(vm); cubalc_chain_os_aspects(&vm->ch); bump(vm); return 1;
  }
  /* [sync] — hive lattice as abstract roles (no product/device names) */
  if (kw(&L->cur,"sync") || kw(&L->cur,"HIVE_SYNC")){
    lex_next(L);
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[sync]"); return -1; }
    lex_next(L);
    ensure_world(vm);
    place_cube(vm, "construct", "construct", 1);
    place_cube(vm, "architect", "architect", 1);
    place_cube(vm, "peer_body", "body", 1);
    place_cube(vm, "peer_host", "host", 0); /* optional offline peer still a cube */
    place_cube(vm, "side", "SIDE_organ", 1);
    place_cube(vm, "hive", "atom", 1);
    do_plug(vm, "side", "construct");
    do_plug(vm, "construct", "architect");
    do_plug(vm, "architect", "peer_body");
    do_plug(vm, "peer_body", "hive");
    do_plug(vm, "hive", "side");
    do_plug(vm, "side", "peer_host");
    cubalc_chain_impulse(&vm->ch, "hive", 1);
    cubalc_chain_impulse(&vm->ch, "construct", 1);
    do_flow(vm, 4);
    if (vm->trace) fprintf(vm->trace, "# sync: construct architect peer_body peer_host side hive\n");
    bump(vm); return 1;
  }
  /* [export "path"] — dump board JSON for host (Grokium way) */
  if (kw(&L->cur,"export") || kw(&L->cur,"dump")){
    lex_next(L);
    char path[256];
    snprintf(path,sizeof path,"state/cubalc_export.json");
    if (L->cur.kind==TK_STR){ snprintf(path,sizeof path,"%s",L->cur.text); lex_next(L); }
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[export]"); return -1; }
    lex_next(L);
    ensure_world(vm);
    {
      FILE *f = fopen(path, "w");
      if (!f){ fail(vm,"export open failed"); return -1; }
      fprintf(f, "{\"schema\":\"cubalc.export.v1\",\"lang\":\"CubalC\",\"version\":\"%s\","
                 "\"hold_flash\":%d,\"n_cubes\":%d,\"unity\":%.4f,\"seq\":%u,"
                 "\"creed\":\"%s\",\"talk\":\"binary_CBLC\",\"store\":\"cubechain\","
                 "\"share\":\"%s\",\"cubes\":[",
              CUBALC_LANG_VERSION, (int)vm->ch.hold_flash, vm->ch.n_cubes,
              vm->ch.unity, (unsigned)vm->ch.seq, vm->ch.creed, CUBALC_SHARE);
      for (int i=0;i<vm->ch.n_cubes;i++){
        cubalc_cube *c=&vm->ch.cubes[i];
        if (i) fputc(',',f);
        fprintf(f, "{\"id\":\"%s\",\"role\":\"%s\",\"proton\":%u,\"energy\":%.3f,"
                   "\"set\":%u,\"digit\":%u,\"plugged\":%u}",
                c->id, c->role, (unsigned)c->atom.proton, c->atom.energy,
                (unsigned)c->atom.matrix.set, (unsigned)c->atom.digit,
                (unsigned)c->plugged);
      }
      fprintf(f, "]}\n");
      fclose(f);
    }
    if (vm->trace) fprintf(vm->trace, "# export %s\n", path);
    bump(vm); return 1;
  }
  /* [fleet] — Grokium nanobot roles as units */
  if (kw(&L->cur,"fleet") || kw(&L->cur,"nanobots")){
    lex_next(L);
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[fleet]"); return -1; }
    lex_next(L);
    ensure_world(vm);
    place_cube(vm,"nb-integrity","integrity_no_leak",1);
    place_cube(vm,"nb-matrix-eval","evaluate_sot_smx",1);
    place_cube(vm,"nb-construct","construct_edge",1);
    place_cube(vm,"nb-observer","observe_unity",1);
    place_cube(vm,"nb-host","station_liaison",1);
    place_cube(vm,"hive","nanobot",1);
    do_plug(vm,"nb-integrity","nb-matrix-eval");
    do_plug(vm,"nb-matrix-eval","nb-construct");
    do_plug(vm,"nb-construct","nb-observer");
    do_plug(vm,"nb-observer","nb-host");
    do_plug(vm,"nb-host","hive");
    do_plug(vm,"hive","nb-integrity");
    cubalc_chain_impulse(&vm->ch,"hive",1);
    do_flow(vm,3);
    if (vm->trace) fprintf(vm->trace, "# fleet units placed\n");
    bump(vm); return 1;
  }
  /* [status] — short board line for hosts */
  if (kw(&L->cur,"status")){
    lex_next(L);
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[status]"); return -1; }
    lex_next(L);
    ensure_world(vm);
    if (vm->trace)
      fprintf(vm->trace,
        "{\"schema\":\"cubalc.status.v1\",\"ok\":true,\"n_cubes\":%d,\"unity\":%.3f,"
        "\"hold_flash\":%d,\"seq\":%u,\"version\":\"%s\"}\n",
        vm->ch.n_cubes, vm->ch.unity, (int)vm->ch.hold_flash,
        (unsigned)vm->ch.seq, CUBALC_LANG_VERSION);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"deconstruct")||kw(&L->cur,"destroy")){
    lex_next(L);
    char id[48]="hive";
    if (L->cur.kind==TK_IDENT){ snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L); }
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[deconstruct]"); return -1; }
    lex_next(L); do_deconstruct(vm,id); bump(vm); return 1;
  }
  if (kw(&L->cur,"reconstruct")||kw(&L->cur,"construct")){
    lex_next(L);
    char id[48]="hive";
    if (L->cur.kind==TK_IDENT){ snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L); }
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[reconstruct]"); return -1; }
    lex_next(L); do_reconstruct(vm,id); bump(vm); return 1;
  }
  if (kw(&L->cur,"decide")||kw(&L->cur,"algocube")){
    lex_next(L);
    char id[48]={0};
    if (L->cur.kind==TK_IDENT){ snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L); }
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[decide]"); return -1; }
    lex_next(L); do_decide(vm,id[0]?id:NULL); bump(vm); return 1;
  }
  if (kw(&L->cur,"share")){
    lex_next(L);
    while (L->cur.kind!=TK_RBRACK && L->cur.kind!=TK_EOF) lex_next(L);
    if (L->cur.kind==TK_RBRACK) lex_next(L);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"genesis") || kw(&L->cur,"world")){
    lex_next(L);
    char plate[512];
    if (L->cur.kind==TK_STR){
      snprintf(plate,sizeof plate,"%s",L->cur.text); lex_next(L);
    } else {
      snprintf(plate,sizeof plate,
        "NEXUS_COORD v1 | from=play | type=world | hold_flash=%d |", vm->hold_flash);
    }
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[genesis] needs ]"); return -1; }
    lex_next(L);
    cubalc_matrix gen; cubalc_coord_to_matrix(plate,&gen);
    cubalc_chain_from_initial(&vm->ch,&gen,1);
    vm->ch.hold_flash=(uint8_t)vm->hold_flash;
    bump(vm); return 1;
  }
  if (kw(&L->cur,"creed")){
    lex_next(L);
    if (L->cur.kind==TK_STR){
      snprintf(vm->creed,sizeof vm->creed,"%s",L->cur.text);
      snprintf(vm->ch.creed,sizeof vm->ch.creed,"%s",L->cur.text);
      lex_next(L);
    }
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"[creed]"); return -1; }
    lex_next(L); bump(vm); return 1;
  }

  /* place / plug / pulse:
   * [name]
   * [name:role]
   * [name!]
   * [name!0]
   * [a~b]
   * [a~b~c]
   * [a|b]
   */
  if (L->cur.kind != TK_IDENT && L->cur.kind != TK_NUM){
    fail(vm, "empty or unknown cube body");
    return -1;
  }
  char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);

  /* plug chain a~b~c or a|b */
  if (L->cur.kind==TK_TILDE || L->cur.kind==TK_PIPE){
    char prev[48]; snprintf(prev,sizeof prev,"%s",a);
    place_cube(vm, prev, prev, 1);
    while (L->cur.kind==TK_TILDE || L->cur.kind==TK_PIPE){
      lex_next(L);
      skip_nl(L);
      if (L->cur.kind!=TK_IDENT && L->cur.kind!=TK_NUM){ fail(vm,"plug needs name"); return -1; }
      char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
      place_cube(vm, b, b, 1);
      do_plug(vm, prev, b);
      snprintf(prev,sizeof prev,"%s",b);
    }
    if (L->cur.kind!=TK_RBRACK){ fail(vm,"plug chain needs ]"); return -1; }
    lex_next(L); bump(vm); return 1;
  }

  char role[48]; snprintf(role,sizeof role,"%s",a);
  int proton=1;
  int do_impulse=0;
  if (L->cur.kind==TK_COLON){
    lex_next(L);
    if (L->cur.kind==TK_IDENT||L->cur.kind==TK_NUM){
      snprintf(role,sizeof role,"%s",L->cur.text); lex_next(L);
    }
  }
  if (L->cur.kind==TK_BANG){
    do_impulse=1; lex_next(L);
    if (L->cur.kind==TK_NUM){ proton=L->cur.num?1:0; lex_next(L); }
  }
  if (L->cur.kind!=TK_RBRACK){ fail(vm,"cube needs ]"); return -1; }
  lex_next(L);
  place_cube(vm, a, role, proton);
  if (do_impulse) cubalc_chain_impulse(&vm->ch, a, (uint8_t)proton);
  bump(vm);
  return 1;
}

static int parse_cube(VM *vm, Lex *L){
  if (L->cur.kind != TK_LBRACK){ fail(vm,"expected ["); return -1; }
  lex_next(L);
  return parse_cube_body(vm, L);
}


static long *var_slot(VM *vm, const char *name, int create){
  for (int i=0;i<vm->n_vars;i++)
    if (strcmp(vm->vars[i].name,name)==0) return &vm->vars[i].val;
  if (!create || vm->n_vars>=64) return NULL;
  snprintf(vm->vars[vm->n_vars].name,sizeof vm->vars[0].name,"%s",name);
  vm->vars[vm->n_vars].val=0;
  return &vm->vars[vm->n_vars++].val;
}

/* Flow law: if stuck, deconstruct then reconstruct the way out. */
static void do_deconstruct(VM *vm, const char *id){
  ensure_world(vm);
  int ix=find_cube(vm,id);
  if (ix<0){ place_cube(vm,id,"deconstruct",0); ix=find_cube(vm,id); }
  if (ix<0) return;
  cubalc_chain_impulse(&vm->ch, id, 0);
  /* clear half plugs by draining energy on peers */
  cubalc_cube *c=&vm->ch.cubes[ix];
  for (int p=0;p<c->n_ports;p++){
    int peer=c->ports[p].peer;
    if (peer>=0 && peer<vm->ch.n_cubes)
      cubalc_chain_impulse(&vm->ch, vm->ch.cubes[peer].id, 0);
  }
  if (vm->trace) fprintf(vm->trace,"# DECONSTRUCT %s\n",id);
}

static void do_reconstruct(VM *vm, const char *id){
  ensure_world(vm);
  int ix=find_cube(vm,id);
  if (ix<0){ place_cube(vm,id,"construct",1); ix=find_cube(vm,id); }
  if (ix<0) return;
  cubalc_chain_impulse(&vm->ch, id, 1);
  /* re-open ports + ring-adjacent plugs for flow */
  for (int i=0;i<vm->ch.n_cubes;i++){
    if (i==ix) continue;
    float c=cubalc_matrix_compat(&vm->ch.cubes[ix].atom.matrix,&vm->ch.cubes[i].atom.matrix);
    if (c>=0.35f) cubalc_cube_plug(&vm->ch, ix, i);
  }
  do_flow(vm, 2);
  if (vm->trace) fprintf(vm->trace,"# RECONSTRUCT %s\n",id);
}

/* Resolve path/string arg: "lit" | LAST | string-var */
static int resolve_str_arg(VM *vm, Lex *L, char *out, size_t outn){
  if (L->cur.kind==TK_STR){
    snprintf(out, outn, "%s", L->cur.text);
    lex_next(L);
    return 0;
  }
  if (L->cur.kind==TK_IDENT){
    if (strcmp(L->cur.text,"LAST")==0){
      snprintf(out, outn, "%s", vm->last_str);
      lex_next(L);
      return 0;
    }
    Var *v = var_get(vm, L->cur.text, 0);
    if (v && v->is_str){
      snprintf(out, outn, "%s", v->sval);
      lex_next(L);
      return 0;
    }
  }
  return -1;
}

/* Peer digit inject: SETDIGIT cube n — CubeBrain algocube 0–9 into matrix SoT */
static void do_setdigit(VM *vm, const char *id, long d){
  ensure_world(vm);
  if (d < 0) d = 0;
  if (d > 9) d = d % 10;
  int ix = find_cube(vm, id);
  if (ix < 0){ place_cube(vm, id, "peer", 1); ix = find_cube(vm, id); }
  if (ix < 0) return;
  cubalc_cube *c = &vm->ch.cubes[ix];
  c->atom.digit = (uint8_t)d;
  c->atom.digit_lock = 1; /* sticky through tick/impulse — peer digit inject */
  c->atom.alive = 1;
  c->atom.energy = fminf(1.f, c->atom.energy + 0.20f);
  /* encode digit pulse into State Matrix — matrix remains SoT */
  for (int i = 0; i < 8; i++)
    cubalc_matrix_set(&c->atom.matrix, (int)((d * 3 + i) % CUBALC_ATOM_BITS), 1);
  cubalc_matrix_set(&c->atom.matrix, (int)d, 1);
  cubalc_matrix_set(&c->atom.matrix, (int)d + 10, 1);
  long *slot = var_slot(vm, "DIGIT", 1);
  if (slot) *slot = d;
  if (vm->trace) fprintf(vm->trace, "# SETDIGIT %s → %ld (%s)\n",
    c->id, d, CUBALC_DIGIT_TAG[d % 10]);
}

/* FOLDBITS cube bits — fold 0/1 stream (newlines ok) into cube matrix + recompute digit */
static void do_foldbits(VM *vm, const char *id, const char *bits){
  ensure_world(vm);
  int ix = find_cube(vm, id);
  if (ix < 0){ place_cube(vm, id, "io", 1); ix = find_cube(vm, id); }
  if (ix < 0 || !bits) return;
  char compact[CUBALC_ATOM_BITS + 1];
  int n = 0;
  for (const char *p = bits; *p && n < CUBALC_ATOM_BITS; p++){
    if (*p == '0' || *p == '1') compact[n++] = *p;
  }
  compact[n] = 0;
  if (n <= 0) return;
  cubalc_matrix_from_ascii(&vm->ch.cubes[ix].atom.matrix, compact, n);
  /* keep full atom width for plugs */
  if (vm->ch.cubes[ix].atom.matrix.n < CUBALC_ATOM_BITS)
    vm->ch.cubes[ix].atom.matrix.n = CUBALC_ATOM_BITS;
  /* FOLDBITS owns matrix → unlock peer digit so algocube can recompute */
  vm->ch.cubes[ix].atom.digit_lock = 0;
  vm->ch.cubes[ix].atom.digit =
    (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ix].atom.matrix);
  vm->ch.cubes[ix].atom.alive = 1;
  if (vm->trace) fprintf(vm->trace, "# FOLDBITS %s n=%d digit=%u\n",
    id, n, (unsigned)vm->ch.cubes[ix].atom.digit);
}

/* Braincube decide: State Matrix → algocube digit 0..9 into var DECIDE + cube digit */
static long do_decide(VM *vm, const char *id){
  ensure_world(vm);
  int ix = id && id[0] ? find_cube(vm,id) : -1;
  if (ix<0){
    /* prefer brain / braincube / algocube / hive */
    const char *cands[]={"brain","braincube","algo","algocube","hive","sot",NULL};
    for (int i=0;cands[i];i++){ ix=find_cube(vm,cands[i]); if(ix>=0) break; }
    if (ix<0 && vm->ch.n_cubes>0) ix=0;
  }
  if (ix<0) return 4; /* hail nexus default digit */
  cubalc_cube *c=&vm->ch.cubes[ix];
  /* matrix is SoT — recompute digit; lock result as braincube decision */
  c->atom.digit = (uint8_t)cubalc_algocube_digit(&c->atom.matrix);
  c->atom.digit_lock = 1;
  long d = c->atom.digit;
  long *slot = var_slot(vm, "DECIDE", 1);
  if (slot) *slot = d;
  long *slot2 = var_slot(vm, "decide", 1);
  if (slot2) *slot2 = d;
  if (vm->trace) fprintf(vm->trace,"# DECIDE %s → %ld (%s)\n",
    c->id, d, CUBALC_DIGIT_TAG[d%10]);
  long *slot3 = var_slot(vm, "DIGIT", 1);
  if (slot3) *slot3 = d;
  return d;
}

/* COMPARE a b — Hamming / unity / XOR digit (free-flow law). */
static long do_compare(VM *vm, const char *ida, const char *idb){
  ensure_world(vm);
  int ia = find_cube(vm, ida), ib = find_cube(vm, idb);
  if (ia < 0 || ib < 0) return 0;
  cubalc_algo_cmp cmp;
  if (cubalc_algocube_compare(&vm->ch.cubes[ia].atom.matrix,
                              &vm->ch.cubes[ib].atom.matrix, &cmp) != 0)
    return 0;
  vm->ch.cubes[ia].atom.unity = cmp.unity;
  vm->ch.cubes[ib].atom.unity = cmp.unity;
  vm->ch.unity = cmp.unity;
  long u = (long)lround(cmp.unity * 100.0);
  long *su = var_slot(vm, "UNITY", 1); if (su) *su = u;
  long *sh = var_slot(vm, "HAMMING", 1); if (sh) *sh = cmp.hamming;
  long *sa = var_slot(vm, "AGREE", 1); if (sa) *sa = cmp.agree;
  long *sc = var_slot(vm, "COMPAT", 1); if (sc) *sc = u;
  long *sd = var_slot(vm, "DIGIT", 1); if (sd) *sd = cmp.digit;
  long *sx = var_slot(vm, "COMPARE", 1); if (sx) *sx = u;
  if (vm->trace) fprintf(vm->trace,
    "# COMPARE %s~%s hamming=%d unity=%.4f digit=%d\n",
    ida, idb, cmp.hamming, cmp.unity, cmp.digit);
  return u;
}

/* HARMONY [target] — majority-vote hive consensus; inject into target cube. */
static long do_harmony(VM *vm, const char *target){
  ensure_world(vm);
  cubalc_algo_harm h;
  if (cubalc_algocube_chain_harmony(&vm->ch, &h) != 0 || !h.ok) {
    long *so = var_slot(vm, "HARMONY", 1); if (so) *so = 0;
    return 0;
  }
  long u = (long)lround(h.unity * 100.0);
  long *su = var_slot(vm, "UNITY", 1); if (su) *su = u;
  long *sh = var_slot(vm, "HARMONY", 1); if (sh) *sh = u;
  long *sd = var_slot(vm, "DIGIT", 1); if (sd) *sd = h.digit;
  long *sc = var_slot(vm, "CONSENSUS", 1); if (sc) *sc = h.consensus.set;
  long *sn = var_slot(vm, "HIVE_N", 1); if (sn) *sn = h.n;
  const char *tid = (target && target[0]) ? target : "hive";
  int ix = find_cube(vm, tid);
  if (ix < 0) { place_cube(vm, tid, "algocube_harmony", 1); ix = find_cube(vm, tid); }
  if (ix >= 0)
    cubalc_algocube_inject(&vm->ch.cubes[ix], &h.consensus, h.digit);
  const char *st = getenv("CUBALC_STATE");
  if (st && st[0]) {
    char path[512];
    snprintf(path, sizeof path, "%s/harmony.json", st);
    FILE *f = fopen(path, "w");
    if (f) { cubalc_algocube_harmony_json(&h, f); fclose(f); }
  }
  if (vm->trace) fprintf(vm->trace,
    "# HARMONY n=%d unity=%.4f digit=%d → %s\n", h.n, h.unity, h.digit, tid);
  if (vm->res) snprintf(vm->res->last_print, sizeof vm->res->last_print,
                        "harmony n=%d unity=%ld digit=%d", h.n, u, h.digit);
  return u;
}

/* RESOLVE [target] — The Cube demands: harmony + decide + energy pulse */
static long do_resolve(VM *vm, const char *target){
  ensure_world(vm);
  long u = do_harmony(vm, target);
  long d = do_decide(vm, target && target[0] ? target : "brain");
  /* energy must flow: pulse create-protons after resolve */
  for (int i = 0; i < vm->ch.n_cubes; i++) {
    if (vm->ch.cubes[i].atom.proton && vm->ch.cubes[i].atom.alive) {
      vm->ch.cubes[i].atom.energy = fminf(1.f, vm->ch.cubes[i].atom.energy + 0.12f);
    }
  }
  do_flow(vm, 2);
  long e = 0;
  for (int i = 0; i < vm->ch.n_cubes; i++)
    e += (long)lround(vm->ch.cubes[i].atom.energy * 100.0);
  long *se = var_slot(vm, "ENERGY", 1); if (se) *se = e;
  long *sr = var_slot(vm, "RESOLVE", 1); if (sr) *sr = d;
  if (vm->trace) fprintf(vm->trace, "# RESOLVE unity=%ld decide=%ld energy=%ld\n", u, d, e);
  if (vm->res) snprintf(vm->res->last_print, sizeof vm->res->last_print,
                        "resolve d=%ld u=%ld e=%ld", d, u, e);
  return d;
}

static int exec_stmts_until(VM *vm, Lex *L, const char *stop1, const char *stop2);

/* Scan one token toward block end. Parks on matching END (depth→0) or UNTIL (if allow_until).
 * Skips BREAK IF / CONTINUE IF so guarded IF does not nest the block. */
static int block_scan_step(Lex *L, int *depth, int allow_until){
  if (L->cur.kind==TK_EOF) return 1;
  if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
    lex_next(L);
    if (kw(&L->cur,"IF")) lex_next(L);
    return 0;
  }
  if (allow_until && *depth==1 && kw(&L->cur,"UNTIL")) return 1;
  if (kw(&L->cur,"FN")||kw(&L->cur,"FUNC")||kw(&L->cur,"FUNCTION")||kw(&L->cur,"DEF")||
      kw(&L->cur,"LOOP")||kw(&L->cur,"IF")||kw(&L->cur,"WHILE")||kw(&L->cur,"FOR")||
      kw(&L->cur,"EACH")||kw(&L->cur,"FOREACH")||kw(&L->cur,"REPEAT")){
    (*depth)++;
    lex_next(L);
    return 0;
  }
  if (kw(&L->cur,"END")){
    (*depth)--;
    if (*depth==0) return 1; /* leave END for caller */
    lex_next(L);
    return 0;
  }
  lex_next(L);
  return 0;
}

/* legacy verbose still works (CREED, CUBE, …) so old plates run */
static long parse_expr(VM *vm, Lex *L); /* minimal for ASSERT */
static long parse_prim(VM *vm, Lex *L){
  skip_nl(L);
  if (L->cur.kind==TK_NUM){ long v=L->cur.num; lex_next(L); return v; }
  if (L->cur.kind==TK_MINUS){ lex_next(L); return -parse_prim(vm,L); }
  if (L->cur.kind==TK_LPAREN){
    lex_next(L); long v=parse_expr(vm,L);
    if (L->cur.kind==TK_RPAREN) lex_next(L);
    return v;
  }
  if (L->cur.kind==TK_IDENT){
    char name[96]; snprintf(name,sizeof name,"%s",L->cur.text); lex_next(L);
    if (strcmp(name,"CUBES")==0) return vm->ch.n_cubes;
    if (strcmp(name,"WORKERS")==0) return cubalc_async_workers();
    if (strcmp(name,"GPU")==0) return cubalc_async_gpu_ok();
    if (strcmp(name,"HTTP_CODE")==0) return vm->last_code;
    if (strcmp(name,"OK")==0){ Var *vv=var_get(vm,"OK",0); return vv?vv->val:0; }
    if (strcmp(name,"EXIT")==0){ Var *vv=var_get(vm,"EXIT",0); return vv?vv->val:0; }
    if (strcmp(name,"LAST_N")==0){ Var *vn=var_get(vm,"LAST_N",0); if(vn) return vn->val; return vm->last_n; }
    if (strcmp(name,"SP")==0 || strcmp(name,"STACKLEN")==0) return (long)vm->sp;
    if (strcmp(name,"CELLS")==0) return (long)CUBALC_CELL_N;
    if (strcmp(name,"STRLEN")==0){
      if (L->cur.kind==TK_LPAREN){ lex_next(L);
        long ln=0;
        if (L->cur.kind==TK_STR) ln=(long)strlen(L->cur.text);
        else if (L->cur.kind==TK_IDENT){ Var *sv=var_get(vm,L->cur.text,0); ln=sv?(sv->is_str?(long)strlen(sv->sval):sv->val):0; }
        if (L->cur.kind==TK_STR||L->cur.kind==TK_IDENT) lex_next(L);
        if (L->cur.kind==TK_RPAREN) lex_next(L);
        return ln;
      }
      return vm->last_n;
    }

    if (strcmp(name,"UNITY")==0) return (long)lround(vm->ch.unity*100);
    if (strcmp(name,"SEQ")==0) return (long)vm->ch.seq;
    /* Pure-science public-domain constants (integer scales) */
    if (strcmp(name,"PI100")==0 || strcmp(name,"PI")==0) return CUBALC_SCI_PI100;
    if (strcmp(name,"E100")==0 || strcmp(name,"EULER")==0) return CUBALC_SCI_E100;
    if (strcmp(name,"G_EARTH")==0 || strcmp(name,"GEARTH")==0) return CUBALC_SCI_G_EARTH10;
    if (strcmp(name,"C_LIGHT")==0 || strcmp(name,"CLIGHT")==0) return CUBALC_SCI_C_LIGHT;
    if (strcmp(name,"ATM_KPA")==0) return CUBALC_SCI_ATM_KPA;
    if (strcmp(name,"WATER_K")==0 || strcmp(name,"ZERO_C_K")==0) return CUBALC_SCI_WATER_K;
    if (strcmp(name,"H2O_BP")==0) return CUBALC_SCI_H2O_BP_C;
    if (strcmp(name,"R_GAS")==0) return CUBALC_SCI_R_J;
    if (strcmp(name,"NA_ORDER")==0) return CUBALC_SCI_AVOGADRO_E23;
    if (strcmp(name,"EARTH_R")==0 || strcmp(name,"EARTH_R_KM")==0) return CUBALC_SCI_EARTH_R_KM;
    if (strcmp(name,"AU_KM")==0 || strcmp(name,"AU")==0) return CUBALC_SCI_AU_KM;
    if (strcmp(name,"YEAR_D")==0) return CUBALC_SCI_YEAR_D;
    if (strcmp(name,"MOON_D")==0) return CUBALC_SCI_MOON_D;
    if (strcmp(name,"SOLAR_C")==0) return CUBALC_SCI_SOLAR_C;
    if (strcmp(name,"ATM_O2")==0) return CUBALC_SCI_ATM_O2_PCT;
    if (strcmp(name,"ATM_N2")==0) return CUBALC_SCI_ATM_N2_PCT;
    /* Math / science pure functions — pure-science language plane */
    if (strcmp(name,"ABS")==0 || strcmp(name,"SIGN")==0 ||
        strcmp(name,"SQRT")==0 || strcmp(name,"ISQRT")==0 ||
        strcmp(name,"MIN")==0 || strcmp(name,"MAX")==0 ||
        strcmp(name,"POW")==0 || strcmp(name,"GCD")==0 ||
        strcmp(name,"LCM")==0 || strcmp(name,"FACT")==0 ||
        strcmp(name,"FORCE")==0 || strcmp(name,"WORK")==0 ||
        strcmp(name,"KE")==0 || strcmp(name,"PE")==0 ||
        strcmp(name,"DENSITY")==0 || strcmp(name,"MOLAR")==0 ||
        strcmp(name,"CELSIUS_K")==0 || strcmp(name,"KELVIN_C")==0 ||
        strcmp(name,"PH_H")==0 || strcmp(name,"CLAMP")==0 ||
        strcmp(name,"AVG")==0 || strcmp(name,"PCT")==0 ||
        strcmp(name,"CIRC")==0 || strcmp(name,"AREA_CIRCLE")==0 ||
        strcmp(name,"HYP")==0 || strcmp(name,"WAVE_V")==0 ||
        strcmp(name,"LIGHT_T")==0 || strcmp(name,"BOYLE_P2")==0 ||
        strcmp(name,"ORBIT_PERIOD")==0 ||
        /* universal bit ops (word forms — | used by play dialect) */
        strcmp(name,"BAND")==0 || strcmp(name,"BOR")==0 ||
        strcmp(name,"BXOR")==0 || strcmp(name,"BNOT")==0 ||
        strcmp(name,"SHL")==0 || strcmp(name,"SHR")==0 ||
        strcmp(name,"BITCOUNT")==0 || strcmp(name,"HAMMING32")==0 ||
        /* digit-9 universal data-path: rotate · pack · select */
        strcmp(name,"ROTL")==0 || strcmp(name,"ROTR")==0 ||
        strcmp(name,"ROL")==0 || strcmp(name,"ROR")==0 ||
        strcmp(name,"PACK16")==0 || strcmp(name,"PACK")==0 ||
        strcmp(name,"HI16")==0 || strcmp(name,"LO16")==0 ||
        strcmp(name,"HIWORD")==0 || strcmp(name,"LOWORD")==0 ||
        strcmp(name,"ISEL")==0 || strcmp(name,"SELECT")==0 ||
        strcmp(name,"NEG")==0 ||
        strcmp(name,"CELL")==0 || strcmp(name,"SLOT")==0 ||
        strcmp(name,"PEEK")==0 || strcmp(name,"STACKLEN")==0 ||
        strcmp(name,"SP")==0 ||
        strcmp(name,"SUMCELL")==0 || strcmp(name,"MINCELL")==0 ||
        strcmp(name,"MAXCELL")==0 ||
        strcmp(name,"FINDCELL")==0 || strcmp(name,"COUNTCELL")==0 ||
        strcmp(name,"MINIDX")==0 || strcmp(name,"ARGMIN")==0 ||
        strcmp(name,"MAXIDX")==0 || strcmp(name,"ARGMAX")==0 ||
        strcmp(name,"RAND")==0 || strcmp(name,"RND")==0 ||
        strcmp(name,"IRAND")==0 ||
        strcmp(name,"SEED")==0 || strcmp(name,"SETSEED")==0 ||
        strcmp(name,"RNG")==0 || strcmp(name,"GETSEED")==0 ||
        /* digit-2 math: modular + number theory + integer bit/log */
        strcmp(name,"ADDMOD")==0 || strcmp(name,"SUBMOD")==0 ||
        strcmp(name,"MULMOD")==0 || strcmp(name,"POWMOD")==0 ||
        strcmp(name,"FIB")==0 || strcmp(name,"FIBONACCI")==0 ||
        strcmp(name,"ISPRIME")==0 || strcmp(name,"PRIMEP")==0 ||
        strcmp(name,"IDIV")==0 || strcmp(name,"IMOD")==0 ||
        strcmp(name,"ILOG2")==0 || strcmp(name,"LOG2")==0 ||
        strcmp(name,"ILOG10")==0 || strcmp(name,"LOG10")==0 ||
        strcmp(name,"ODD")==0 || strcmp(name,"EVEN")==0 ||
        strcmp(name,"CTZ")==0 || strcmp(name,"CLZ")==0 ||
        strcmp(name,"ISPOW2")==0 || strcmp(name,"POW2")==0 ||
        strcmp(name,"POW10")==0 || strcmp(name,"TENPOW")==0 ||
        strcmp(name,"NDIGITS")==0 || strcmp(name,"DIGSUM")==0 ||
        strcmp(name,"MODINV")==0 || strcmp(name,"INVMOD")==0 ||
        /* digit-0 foundation: bitfield extract/deposit + ceil div + mask */
        strcmp(name,"BEXT")==0 || strcmp(name,"BITEXT")==0 ||
        strcmp(name,"BDEP")==0 || strcmp(name,"BITDEP")==0 ||
        strcmp(name,"BYTE")==0 || strcmp(name,"HIBYTE")==0 ||
        strcmp(name,"LOBYTE")==0 ||
        strcmp(name,"MASK")==0 || strcmp(name,"BITMASK")==0 ||
        strcmp(name,"ISDIV")==0 || strcmp(name,"DIVISIBLE")==0 ||
        strcmp(name,"DIVCEIL")==0 || strcmp(name,"CEILDIV")==0 ||
        /* digit-1 data path: word reverse / parity / nibble */
        strcmp(name,"BSWAP")==0 || strcmp(name,"BSWAP32")==0 ||
        strcmp(name,"BITREV")==0 || strcmp(name,"REVBITS")==0 ||
        strcmp(name,"PARITY")==0 ||
        strcmp(name,"NIBBLE")==0 || strcmp(name,"NIB")==0 ||
        strcmp(name,"DIST")==0 || strcmp(name,"ABSDIFF")==0 ||
        /* digit-8 sign/zero extend data path */
        strcmp(name,"SEXT")==0 || strcmp(name,"SIGNEXT")==0 ||
        strcmp(name,"ZEXT")==0 || strcmp(name,"ZEROEXT")==0 ||
        strcmp(name,"SEXT8")==0 || strcmp(name,"SEXTB")==0 ||
        strcmp(name,"SEXT16")==0 || strcmp(name,"SEXTW")==0 ||
        /* digit-8 pack byte/nibble + set nibble */
        strcmp(name,"PACK8")==0 || strcmp(name,"PACKB")==0 ||
        strcmp(name,"PACKNIB")==0 || strcmp(name,"PACK4")==0 ||
        strcmp(name,"SETNIB")==0 || strcmp(name,"SETNIBBLE")==0 ||
        strcmp(name,"SETBYTE")==0 || strcmp(name,"SETB")==0 ||
        /* digit-5 align / round-to-multiple */
        strcmp(name,"ALIGN")==0 || strcmp(name,"ROUNDUP")==0 ||
        strcmp(name,"ALIGNDN")==0 || strcmp(name,"ROUNDDN")==0 ||
        /* digit-2 math ext: combinatorics + square + floor div */
        strcmp(name,"SQR")==0 || strcmp(name,"SQUARE")==0 ||
        strcmp(name,"BINOM")==0 || strcmp(name,"CHOOSE")==0 ||
        strcmp(name,"PERM")==0 || strcmp(name,"PNR")==0 ||
        strcmp(name,"DIVFLOOR")==0 || strcmp(name,"FLOORDIV")==0){
      if (L->cur.kind==TK_LPAREN){
        lex_next(L);
        long a = parse_expr(vm,L);
        long b = 0, c = 0;
        if (L->cur.kind==TK_COMMA){ lex_next(L); b = parse_expr(vm,L); }
        if (L->cur.kind==TK_COMMA){ lex_next(L); c = parse_expr(vm,L); }
        if (L->cur.kind==TK_RPAREN) lex_next(L);
        if (strcmp(name,"ABS")==0) return a < 0 ? -a : a;
        if (strcmp(name,"SIGN")==0) return a > 0 ? 1 : (a < 0 ? -1 : 0);
        if (strcmp(name,"SQRT")==0 || strcmp(name,"ISQRT")==0){
          if (a < 0) return 0;
          long r = 0;
          while ((r+1)*(r+1) <= a) r++;
          return r;
        }
        if (strcmp(name,"MIN")==0) return a < b ? a : b;
        if (strcmp(name,"MAX")==0) return a > b ? a : b;
        if (strcmp(name,"POW")==0){
          long e = b; if (e < 0) return 0;
          long r = 1;
          while (e-- > 0) r *= a;
          return r;
        }
        if (strcmp(name,"GCD")==0){
          long x = a < 0 ? -a : a, y = b < 0 ? -b : b;
          while (y){ long t = x % y; x = y; y = t; }
          return x;
        }
        if (strcmp(name,"LCM")==0){
          long x = a < 0 ? -a : a, y = b < 0 ? -b : b;
          if (!x || !y) return 0;
          long g = x, h = y;
          while (h){ long t = g % h; g = h; h = t; }
          return (x / g) * y;
        }
        if (strcmp(name,"FACT")==0){
          if (a < 0) return 0;
          if (a > 20) a = 20; /* stay in long */
          long r = 1;
          for (long i = 2; i <= a; i++) r *= i;
          return r;
        }
        /* Physics (integer): F=ma, W=Fd, KE=mv²/2, PE=mgh (g scaled ×10 → divide 10) */
        if (strcmp(name,"FORCE")==0) return a * b;           /* m * a */
        if (strcmp(name,"WORK")==0) return a * b;            /* F * d */
        if (strcmp(name,"KE")==0) return (a * b * b) / 2;    /* m v v / 2 */
        if (strcmp(name,"PE")==0){
          /* PE(m,h) → m·g·h with g×10; PE(m,g10,h) if third arg present (c!=0 or b used as g) */
          long g10 = CUBALC_SCI_G_EARTH10;
          long h = b;
          /* 3-arg form: PE(m, g10, h) when user passes three numbers; detect via c!=0 OR
             convention: if only two args, b is height. Third arg always sets c from parse. */
          if (c) { g10 = b; h = c; }
          return (a * g10 * h) / 10;
        }
        if (strcmp(name,"DENSITY")==0) return b ? (a / b) : 0; /* m/V */
        if (strcmp(name,"MOLAR")==0) return b ? (a / b) : 0;   /* n = N/NA_order or mass/M */
        if (strcmp(name,"CELSIUS_K")==0) return a + CUBALC_SCI_WATER_K;
        if (strcmp(name,"KELVIN_C")==0) return a - CUBALC_SCI_WATER_K;
        /* pH proxy: pH = -log10[H+]; integer H_scaled e.g. 1e-3 → use H_pow = 3 → pH 3 */
        if (strcmp(name,"PH_H")==0) return a; /* pass-through: user supplies exponent as pH law */
        if (strcmp(name,"CLAMP")==0){
          /* CLAMP(x, lo, hi) */
          long lo = b, hi = c;
          if (hi < lo) { long t = lo; lo = hi; hi = t; }
          if (a < lo) return lo;
          if (a > hi) return hi;
          return a;
        }
        if (strcmp(name,"AVG")==0) return (a + b) / 2;
        if (strcmp(name,"PCT")==0) return b ? (a * 100 / b) : 0; /* a is what % of b */
        if (strcmp(name,"CIRC")==0) return 2 * CUBALC_SCI_PI100 * a / 100; /* 2πr scaled */
        if (strcmp(name,"AREA_CIRCLE")==0) return CUBALC_SCI_PI100 * a * a / 100; /* πr² scaled */
        if (strcmp(name,"HYP")==0){ /* integer hypotenuse √(a²+b²) */
          long s = a*a + b*b;
          if (s < 0) return 0;
          long r = 0;
          while ((r+1)*(r+1) <= s) r++;
          return r;
        }
        if (strcmp(name,"WAVE_V")==0) return a * b; /* f * λ */
        if (strcmp(name,"LIGHT_T")==0) return b ? (a / b) : 0; /* dist / c → seconds if SI */
        if (strcmp(name,"BOYLE_P2")==0) return c ? (a * b / c) : 0; /* P1*V1/V2 */
        if (strcmp(name,"ORBIT_PERIOD")==0){
          /* Kepler-ish scale: T² ∝ a³ → T ~ a * sqrt(a) / k ; use T = a for unit AU demo */
          if (a <= 0) return 0;
          long aa = a * a * a;
          long r = 0;
          while ((r+1)*(r+1) <= aa) r++;
          return r; /* rough √(a³) for integer AU */
        }
        /* Universal integer bit algebra */
        if (strcmp(name,"BAND")==0) return a & b;
        if (strcmp(name,"BOR")==0) return a | b;
        if (strcmp(name,"BXOR")==0) return a ^ b;
        if (strcmp(name,"BNOT")==0) return ~a;
        if (strcmp(name,"SHL")==0){
          if (b < 0) b = 0; if (b > 62) b = 62;
          return a << b;
        }
        if (strcmp(name,"SHR")==0){
          if (b < 0) b = 0; if (b > 62) b = 62;
          return (long)((unsigned long)a >> (unsigned)b);
        }
        if (strcmp(name,"BITCOUNT")==0){
          unsigned long u = (unsigned long)a; int n = 0;
          while (u) { n += (int)(u & 1u); u >>= 1; }
          return n;
        }
        if (strcmp(name,"HAMMING32")==0){
          unsigned long u = (unsigned long)(a ^ b); int n = 0;
          while (u) { n += (int)(u & 1u); u >>= 1; }
          return n;
        }
        /* Rotate (32-bit width — portable universal word) */
        if (strcmp(name,"ROTL")==0 || strcmp(name,"ROL")==0){
          unsigned int w = (unsigned int)a;
          int k = (int)b;
          if (k < 0) k = 0;
          k &= 31;
          if (k == 0) return (long)w;
          return (long)((w << k) | (w >> (32 - k)));
        }
        if (strcmp(name,"ROTR")==0 || strcmp(name,"ROR")==0){
          unsigned int w = (unsigned int)a;
          int k = (int)b;
          if (k < 0) k = 0;
          k &= 31;
          if (k == 0) return (long)w;
          return (long)((w >> k) | (w << (32 - k)));
        }
        /* Pack / unpack 16-bit halves → 32-bit word */
        if (strcmp(name,"PACK16")==0 || strcmp(name,"PACK")==0){
          unsigned int hi = (unsigned int)a & 0xFFFFu;
          unsigned int lo = (unsigned int)b & 0xFFFFu;
          return (long)((hi << 16) | lo);
        }
        if (strcmp(name,"HI16")==0 || strcmp(name,"HIWORD")==0)
          return (long)(((unsigned int)a >> 16) & 0xFFFFu);
        if (strcmp(name,"LO16")==0 || strcmp(name,"LOWORD")==0)
          return (long)((unsigned int)a & 0xFFFFu);
        /* ISEL(cond, then, else) — expression ternary (universal control in expr) */
        if (strcmp(name,"ISEL")==0 || strcmp(name,"SELECT")==0)
          return a ? b : c;
        if (strcmp(name,"NEG")==0) return -a;
        if (strcmp(name,"CELL")==0 || strcmp(name,"SLOT")==0){
          if (a < 0) a = 0;
          if (a >= CUBALC_CELL_N) a = CUBALC_CELL_N - 1;
          return vm->cells[(int)a];
        }
        if (strcmp(name,"SUMCELL")==0 || strcmp(name,"MINCELL")==0 ||
            strcmp(name,"MAXCELL")==0){
          long lo = a, hi = (b != 0 || L->cur.kind==TK_RPAREN) ? b : a;
          /* if only one arg, hi=a already; if two, b set */
          if (b == 0 && a == 0){ lo = 0; hi = CUBALC_CELL_N - 1; }
          else if (b == 0) hi = a;
          if (lo < 0) lo = 0;
          if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
          if (hi < lo){ long t=lo; lo=hi; hi=t; }
          long acc=0, mn=0, mx=0; int first=1;
          for (long i=lo;i<=hi;i++){
            long v=vm->cells[(int)i];
            acc += v;
            if (first){ mn=mx=v; first=0; }
            else { if (v<mn) mn=v; if (v>mx) mx=v; }
          }
          if (strcmp(name,"MINCELL")==0) return first?0:mn;
          if (strcmp(name,"MAXCELL")==0) return first?0:mx;
          return acc;
        }
        /* digit-5 cell memory: FINDCELL(val[,lo[,hi]]) · COUNTCELL(val[,lo[,hi]]) */
        if (strcmp(name,"FINDCELL")==0 || strcmp(name,"COUNTCELL")==0){
          long val = a;
          long lo = 0, hi = CUBALC_CELL_N - 1;
          if (b != 0 || c != 0){
            lo = b;
            hi = (c != 0) ? c : CUBALC_CELL_N - 1;
          }
          if (lo < 0) lo = 0;
          if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
          if (hi < lo){ long t=lo; lo=hi; hi=t; }
          if (strcmp(name,"FINDCELL")==0){
            for (long i=lo;i<=hi;i++)
              if (vm->cells[(int)i] == val) return i;
            return -1;
          }
          long cnt = 0;
          for (long i=lo;i<=hi;i++)
            if (vm->cells[(int)i] == val) cnt++;
          return cnt;
        }
        /* digit-9: MINIDX/ARGMIN(lo[,hi]) · MAXIDX/ARGMAX(lo[,hi]) */
        if (strcmp(name,"MINIDX")==0 || strcmp(name,"ARGMIN")==0 ||
            strcmp(name,"MAXIDX")==0 || strcmp(name,"ARGMAX")==0){
          long lo = a, hi = (b != 0) ? b : a;
          if (b == 0 && a == 0){ lo = 0; hi = CUBALC_CELL_N - 1; }
          else if (b == 0) hi = a;
          if (lo < 0) lo = 0;
          if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
          if (hi < lo){ long t=lo; lo=hi; hi=t; }
          int want_max = (strcmp(name,"MAXIDX")==0 || strcmp(name,"ARGMAX")==0);
          long best_i = lo, best_v = vm->cells[(int)lo];
          for (long i = lo + 1; i <= hi; i++){
            long v = vm->cells[(int)i];
            if (want_max){ if (v > best_v){ best_v = v; best_i = i; } }
            else { if (v < best_v){ best_v = v; best_i = i; } }
          }
          return best_i;
        }
        if (strcmp(name,"PEEK")==0){
          if (vm->sp <= 0) return 0;
          return vm->stack[vm->sp - 1];
        }
        if (strcmp(name,"STACKLEN")==0 || strcmp(name,"SP")==0)
          return (long)vm->sp;
        if (strcmp(name,"RAND")==0 || strcmp(name,"RND")==0 || strcmp(name,"IRAND")==0){
          /* xorshift32 → [0, a) if a>0 else 0..9 */
          uint32_t x = vm->rng;
          x ^= x << 13; x ^= x >> 17; x ^= x << 5;
          if (!x) x = 1;
          vm->rng = x;
          long m = a > 0 ? a : 10;
          return (long)(x % (uint32_t)m);
        }
        if (strcmp(name,"SEED")==0 || strcmp(name,"SETSEED")==0){
          /* SEED(n) — set RNG state; 0 maps to 1; return the seed used */
          uint32_t s = (uint32_t)a;
          if (!s) s = 1;
          vm->rng = s;
          return (long)s;
        }
        if (strcmp(name,"RNG")==0 || strcmp(name,"GETSEED")==0){
          /* current PRNG state (no advance); arg ignored */
          return (long)vm->rng;
        }
        /* Modular arithmetic (digit-2 math plane) */
        if (strcmp(name,"IDIV")==0) return b ? (a / b) : 0;
        if (strcmp(name,"IMOD")==0) return b ? (a % b) : 0;
        if (strcmp(name,"ADDMOD")==0){
          long m = c; if (m <= 0) return 0;
          long x = a % m; if (x < 0) x += m;
          long y = b % m; if (y < 0) y += m;
          return (x + y) % m;
        }
        if (strcmp(name,"SUBMOD")==0){
          long m = c; if (m <= 0) return 0;
          long x = a % m; if (x < 0) x += m;
          long y = b % m; if (y < 0) y += m;
          return (x - y + m) % m;
        }
        if (strcmp(name,"MULMOD")==0){
          long m = c; if (m <= 0) return 0;
          long x = a % m; if (x < 0) x += m;
          long y = b % m; if (y < 0) y += m;
          /* careful multiply via binary for large values */
          long r = 0;
          while (y > 0){
            if (y & 1) r = (r + x) % m;
            x = (x + x) % m;
            y >>= 1;
          }
          return r;
        }
        if (strcmp(name,"POWMOD")==0){
          long m = c; if (m <= 0) return 0;
          long base = a % m; if (base < 0) base += m;
          long exp = b; if (exp < 0) return 0;
          long r = 1 % m;
          while (exp > 0){
            if (exp & 1){
              long y = r, x = base, acc = 0;
              while (y > 0){
                if (y & 1) acc = (acc + x) % m;
                x = (x + x) % m;
                y >>= 1;
              }
              r = acc;
            }
            {
              long x = base, acc = 0, y = base;
              while (y > 0){
                if (y & 1) acc = (acc + x) % m;
                x = (x + x) % m;
                y >>= 1;
              }
              base = acc;
            }
            exp >>= 1;
          }
          return r;
        }
        if (strcmp(name,"FIB")==0 || strcmp(name,"FIBONACCI")==0){
          if (a <= 0) return 0;
          if (a == 1 || a == 2) return 1;
          if (a > 92) a = 92; /* stay in signed 64-bit */
          long f0 = 0, f1 = 1;
          for (long i = 2; i <= a; i++){
            long f2 = f0 + f1;
            f0 = f1; f1 = f2;
          }
          return f1;
        }
        if (strcmp(name,"ISPRIME")==0 || strcmp(name,"PRIMEP")==0){
          if (a <= 1) return 0;
          if (a <= 3) return 1;
          if ((a % 2) == 0 || (a % 3) == 0) return 0;
          for (long i = 5; i * i <= a; i += 6){
            if ((a % i) == 0 || (a % (i + 2)) == 0) return 0;
          }
          return 1;
        }
        /* digit-2/6 extended math: log2 / log10 / odd-even / bit counts / pow2 */
        if (strcmp(name,"ILOG2")==0 || strcmp(name,"LOG2")==0){
          if (a <= 0) return -1;
          unsigned long u = (unsigned long)a;
          long r = -1;
          while (u){ r++; u >>= 1; }
          return r;
        }
        if (strcmp(name,"ILOG10")==0 || strcmp(name,"LOG10")==0){
          /* floor(log10(a)); a<=0 → -1 */
          if (a <= 0) return -1;
          long r = 0;
          long x = a;
          while (x >= 10){ r++; x /= 10; }
          return r;
        }
        if (strcmp(name,"ODD")==0) return (a & 1L) ? 1 : 0;
        if (strcmp(name,"EVEN")==0) return (a & 1L) ? 0 : 1;
        if (strcmp(name,"CTZ")==0){
          /* count trailing zeros (0 → 64) */
          if (a == 0) return 64;
          unsigned long u = (unsigned long)a;
          long n = 0;
          while ((u & 1ul) == 0){ n++; u >>= 1; }
          return n;
        }
        if (strcmp(name,"CLZ")==0){
          /* count leading zeros in 64-bit word (0 → 64) */
          if (a == 0) return 64;
          unsigned long u = (unsigned long)a;
          long n = 0;
          for (int i = 63; i >= 0; i--){
            if (u & (1ul << i)) break;
            n++;
          }
          return n;
        }
        if (strcmp(name,"ISPOW2")==0){
          if (a <= 0) return 0;
          unsigned long u = (unsigned long)a;
          return (u & (u - 1ul)) == 0ul ? 1 : 0;
        }
        if (strcmp(name,"POW2")==0){
          if (a < 0 || a > 62) return 0;
          return 1L << a;
        }
        if (strcmp(name,"POW10")==0 || strcmp(name,"TENPOW")==0){
          /* 10^a for a in 0..18; out of range → 0 */
          if (a < 0 || a > 18) return 0;
          long r = 1;
          for (long i = 0; i < a; i++) r *= 10;
          return r;
        }
        if (strcmp(name,"NDIGITS")==0){
          long x = a < 0 ? -a : a;
          if (x == 0) return 1;
          long n = 0;
          while (x){ n++; x /= 10; }
          return n;
        }
        if (strcmp(name,"DIGSUM")==0){
          long x = a < 0 ? -a : a;
          long s = 0;
          if (x == 0) return 0;
          while (x){ s += x % 10; x /= 10; }
          return s;
        }
        if (strcmp(name,"MODINV")==0 || strcmp(name,"INVMOD")==0){
          /* modular inverse a^{-1} mod b via extended Euclid; 0 if none */
          long m = b;
          if (m <= 1) return 0;
          long aa = a % m; if (aa < 0) aa += m;
          if (aa == 0) return 0;
          long t = 0, nt = 1;
          long r = m, nr = aa;
          while (nr != 0){
            long q = r / nr;
            long tmp = nt; nt = t - q * nt; t = tmp;
            tmp = nr; nr = r - q * nr; r = tmp;
          }
          if (r > 1) return 0; /* not invertible */
          if (t < 0) t += m;
          return t;
        }
        /* digit-0 foundation bitfields */
        if (strcmp(name,"BEXT")==0 || strcmp(name,"BITEXT")==0){
          /* BEXT(val, pos, width) — extract width bits starting at bit pos */
          long pos = b, width = c;
          if (pos < 0) pos = 0;
          if (pos > 62) return 0;
          if (width < 1) return 0;
          if (width > 63 - pos) width = 63 - pos;
          if (width <= 0) return 0;
          unsigned long mask = (width >= 63) ? ~0ul : ((1ul << width) - 1ul);
          return (long)(((unsigned long)a >> (unsigned)pos) & mask);
        }
        if (strcmp(name,"BDEP")==0 || strcmp(name,"BITDEP")==0){
          /* BDEP(base, field, pos) — deposit low 8 bits of field at bit pos */
          long pos = c;
          long width = 8;
          if (pos < 0) pos = 0;
          if (pos > 62) return a;
          if (width > 63 - pos) width = 63 - pos;
          if (width <= 0) return a;
          unsigned long mask = (width >= 63) ? ~0ul : ((1ul << width) - 1ul);
          unsigned long base = (unsigned long)a;
          unsigned long field = (unsigned long)b & mask;
          base = (base & ~(mask << (unsigned)pos)) | (field << (unsigned)pos);
          return (long)base;
        }
        if (strcmp(name,"BYTE")==0){
          /* BYTE(val, i) — i-th byte little-endian (0=LSB) */
          long i = b;
          if (i < 0) i = 0;
          if (i > 7) i = 7;
          return (long)(((unsigned long)a >> (unsigned)(i * 8)) & 0xFFul);
        }
        if (strcmp(name,"LOBYTE")==0)
          return (long)((unsigned long)a & 0xFFul);
        if (strcmp(name,"HIBYTE")==0)
          return (long)(((unsigned long)a >> 8) & 0xFFul);
        if (strcmp(name,"MASK")==0 || strcmp(name,"BITMASK")==0){
          /* MASK(n) — low n bits set; n<=0 → 0; n>=63 → all ones (signed long) */
          if (a <= 0) return 0;
          if (a >= 63) return (long)~0ul;
          return (long)((1ul << (unsigned)a) - 1ul);
        }
        if (strcmp(name,"ISDIV")==0 || strcmp(name,"DIVISIBLE")==0){
          /* ISDIV(a,b) → 1 if b!=0 and a is multiple of b */
          if (b == 0) return 0;
          return (a % b) == 0 ? 1 : 0;
        }
        /* digit-1 word data path: BSWAP BITREV PARITY NIBBLE */
        if (strcmp(name,"BSWAP")==0 || strcmp(name,"BSWAP32")==0){
          unsigned int w = (unsigned int)a;
          w = ((w & 0x000000FFu) << 24) | ((w & 0x0000FF00u) << 8) |
              ((w & 0x00FF0000u) >> 8) | ((w & 0xFF000000u) >> 24);
          return (long)w;
        }
        if (strcmp(name,"BITREV")==0 || strcmp(name,"REVBITS")==0){
          unsigned int w = (unsigned int)a;
          unsigned int r = 0;
          for (int i = 0; i < 32; i++){
            r = (r << 1) | (w & 1u);
            w >>= 1;
          }
          return (long)r;
        }
        if (strcmp(name,"PARITY")==0){
          unsigned long u = (unsigned long)a;
          int n = 0;
          while (u){ n ^= (int)(u & 1u); u >>= 1; }
          return (long)n;
        }
        if (strcmp(name,"NIBBLE")==0 || strcmp(name,"NIB")==0){
          /* NIBBLE(val, i) — i-th 4-bit nibble little-endian (0=LSB) */
          long i = b;
          if (i < 0) i = 0;
          if (i > 15) i = 15;
          return (long)(((unsigned long)a >> (unsigned)(i * 4)) & 0xFul);
        }
        if (strcmp(name,"DIST")==0 || strcmp(name,"ABSDIFF")==0){
          long d = a - b;
          return d < 0 ? -d : d;
        }
        /* digit-8: SEXT/ZEXT — extend bottom width bits; SEXT8/16 fixed width */
        if (strcmp(name,"SEXT8")==0 || strcmp(name,"SEXTB")==0){
          long v = a & 0xFFL;
          if (v & 0x80L) v |= ~0xFFL;
          return v;
        }
        if (strcmp(name,"SEXT16")==0 || strcmp(name,"SEXTW")==0){
          long v = a & 0xFFFFL;
          if (v & 0x8000L) v |= ~0xFFFFL;
          return v;
        }
        if (strcmp(name,"ZEXT")==0 || strcmp(name,"ZEROEXT")==0){
          long w = b;
          if (w <= 0) return 0;
          if (w >= 63) return a;
          unsigned long mask = (1ul << (unsigned)w) - 1ul;
          return (long)((unsigned long)a & mask);
        }
        if (strcmp(name,"SEXT")==0 || strcmp(name,"SIGNEXT")==0){
          long w = b;
          if (w <= 0) return 0;
          if (w >= 63) return a;
          unsigned long mask = (1ul << (unsigned)w) - 1ul;
          unsigned long v = (unsigned long)a & mask;
          unsigned long sign = 1ul << (unsigned)(w - 1);
          if (v & sign) v |= ~mask;
          return (long)v;
        }
        if (strcmp(name,"PACK8")==0 || strcmp(name,"PACKB")==0){
          /* PACK8(hi, lo) — two bytes → 16-bit word */
          unsigned int h = (unsigned int)a & 0xFFu;
          unsigned int l = (unsigned int)b & 0xFFu;
          return (long)((h << 8) | l);
        }
        if (strcmp(name,"PACKNIB")==0 || strcmp(name,"PACK4")==0){
          /* PACKNIB(hi, lo) — two nibbles → byte */
          unsigned int h = (unsigned int)a & 0xFu;
          unsigned int l = (unsigned int)b & 0xFu;
          return (long)((h << 4) | l);
        }
        if (strcmp(name,"SETNIB")==0 || strcmp(name,"SETNIBBLE")==0){
          /* SETNIB(val, field, i) — deposit 4-bit field at nibble index i (LE) */
          long i = c;
          if (i < 0) i = 0;
          if (i > 15) i = 15;
          unsigned long base = (unsigned long)a;
          unsigned long field = (unsigned long)b & 0xFul;
          unsigned long shift = (unsigned long)(i * 4);
          base = (base & ~(0xFul << shift)) | (field << shift);
          return (long)base;
        }
        if (strcmp(name,"SETBYTE")==0 || strcmp(name,"SETB")==0){
          /* SETBYTE(val, byte, i) — deposit 8-bit at byte index i (LE) */
          long i = c;
          if (i < 0) i = 0;
          if (i > 7) i = 7;
          unsigned long base = (unsigned long)a;
          unsigned long field = (unsigned long)b & 0xFFul;
          unsigned long shift = (unsigned long)(i * 8);
          base = (base & ~(0xFFul << shift)) | (field << shift);
          return (long)base;
        }
        if (strcmp(name,"ALIGN")==0 || strcmp(name,"ROUNDUP")==0){
          /* ALIGN(val, a) — smallest multiple of a that is >= val; a<=0 → val */
          long al = b;
          if (al <= 0) return a;
          long q = a / al, r = a % al;
          if (r == 0) return a;
          if (a > 0) return (q + 1) * al;
          return q * al; /* a<0: C trunc toward 0 → q*al is ceil toward +inf */
        }
        if (strcmp(name,"ALIGNDN")==0 || strcmp(name,"ROUNDDN")==0){
          /* ALIGNDN(val, a) — largest multiple of a that is <= val */
          long al = b;
          if (al <= 0) return a;
          long q = a / al, r = a % al;
          if (r == 0) return a;
          if (a > 0) return q * al;
          return (q - 1) * al; /* a<0: floor toward -inf */
        }
        if (strcmp(name,"DIVCEIL")==0 || strcmp(name,"CEILDIV")==0){
          /* ceil(a/b); 0 if b==0. Non-neg exact; mixed → C trunc (ok for ceil when <0). */
          if (b == 0) return 0;
          if (a >= 0 && b > 0) return (a + b - 1) / b;
          if (a <= 0 && b < 0){
            long aa = -a, bb = -b;
            return (aa + bb - 1) / bb;
          }
          return a / b;
        }
        if (strcmp(name,"SQR")==0 || strcmp(name,"SQUARE")==0)
          return a * a;
        if (strcmp(name,"DIVFLOOR")==0 || strcmp(name,"FLOORDIV")==0){
          /* floor(a/b); 0 if b==0 */
          if (b == 0) return 0;
          long q = a / b, r = a % b;
          if (r != 0 && ((a < 0) != (b < 0))) q--; /* toward -inf when signs differ */
          return q;
        }
        if (strcmp(name,"BINOM")==0 || strcmp(name,"CHOOSE")==0){
          /* C(n,k) multiplicative formula; 0 if invalid */
          long n = a, k = b;
          if (n < 0 || k < 0 || k > n) return 0;
          if (k > n - k) k = n - k;
          long r = 1;
          for (long i = 1; i <= k; i++){
            /* keep intermediate exact: multiply then divide by i */
            r = r * (n - k + i) / i;
          }
          return r;
        }
        if (strcmp(name,"PERM")==0 || strcmp(name,"PNR")==0){
          /* P(n,k) = n!/(n-k)! */
          long n = a, k = b;
          if (n < 0 || k < 0 || k > n) return 0;
          long r = 1;
          for (long i = 0; i < k; i++) r *= (n - i);
          return r;
        }
        return 0;
      }
      /* zero-arg: PEEK() STACKLEN() SP() */
      if (strcmp(name,"PEEK")==0){
        if (vm->sp <= 0) return 0;
        return vm->stack[vm->sp - 1];
      }
      if (strcmp(name,"STACKLEN")==0 || strcmp(name,"SP")==0)
        return (long)vm->sp;
    }
    if (strcmp(name,"SET")==0 || strcmp(name,"POPCOUNT")==0 ||
        strcmp(name,"ENERGY")==0 || strcmp(name,"DIGIT")==0 ||
        strcmp(name,"BIT")==0 || strcmp(name,"FLOWED")==0 ||
        strcmp(name,"COMPILED")==0 || strcmp(name,"PARENT")==0 ||
        strcmp(name,"NESTED")==0 || strcmp(name,"PORTS")==0 ||
        strcmp(name,"NPORTS")==0 || strcmp(name,"PLUGGED")==0 ||
        strcmp(name,"BITS")==0 || strcmp(name,"WIDTH")==0){
      if (L->cur.kind==TK_LPAREN){
        lex_next(L);
        char id[48]={0};
        if (L->cur.kind==TK_IDENT){ snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L); }
        int bit=-1;
        if (strcmp(name,"BIT")==0 && L->cur.kind==TK_COMMA){
          lex_next(L);
          if (L->cur.kind==TK_NUM){ bit=(int)L->cur.num; lex_next(L); }
        }
        if (L->cur.kind==TK_RPAREN) lex_next(L);
        int ix=find_cube(vm,id);
        if (ix<0) return 0;
        cubalc_cube *c=&vm->ch.cubes[ix];
        if (strcmp(name,"SET")==0 || strcmp(name,"POPCOUNT")==0)
          return cubalc_matrix_popcount(&c->atom.matrix);
        if (strcmp(name,"ENERGY")==0) return (long)lround(c->atom.energy*100.0);
        if (strcmp(name,"DIGIT")==0) return (long)c->atom.digit;
        if (strcmp(name,"BIT")==0 && bit>=0)
          return cubalc_matrix_get(&c->atom.matrix, bit)?1:0;
        if (strcmp(name,"FLOWED")==0) return c->flowed ? 1 : 0;
        if (strcmp(name,"COMPILED")==0) return c->compiled ? 1 : 0;
        if (strcmp(name,"PARENT")==0) return (long)c->parent;
        if (strcmp(name,"NESTED")==0) return c->parent >= 0 ? 1 : 0;
        if (strcmp(name,"PORTS")==0 || strcmp(name,"NPORTS")==0) return (long)c->n_ports;
        if (strcmp(name,"PLUGGED")==0) return (long)c->plugged;
        if (strcmp(name,"BITS")==0 || strcmp(name,"WIDTH")==0)
          return c->atom.matrix.n ? (long)c->atom.matrix.n : (long)CUBALC_ATOM_BITS;
        return 0;
      }
    }
    if (strcmp(name,"COMPAT")==0){
      if (L->cur.kind==TK_LPAREN){
        lex_next(L);
        char a[48]={0},b[48]={0};
        if (L->cur.kind==TK_IDENT){ snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L); }
        if (L->cur.kind==TK_COMMA) lex_next(L);
        if (L->cur.kind==TK_IDENT){ snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L); }
        if (L->cur.kind==TK_RPAREN) lex_next(L);
        int ia=find_cube(vm,a), ib=find_cube(vm,b);
        if (ia<0||ib<0) return 0;
        return (long)lround(cubalc_matrix_compat(
          &vm->ch.cubes[ia].atom.matrix,&vm->ch.cubes[ib].atom.matrix)*100.0);
      }
    }
    for (int i=0;i<vm->n_vars;i++) if (strcmp(vm->vars[i].name,name)==0) return vm->vars[i].val;
    int ix=find_cube(vm,name);
    if (ix>=0) return cubalc_matrix_popcount(&vm->ch.cubes[ix].atom.matrix);
    return 0;
  }
  return 0;
}
static long parse_term(VM *vm, Lex *L){
  long v=parse_prim(vm,L);
  for(;;){
    if (L->cur.kind==TK_STAR){ lex_next(L); v*=parse_prim(vm,L); }
    else if (L->cur.kind==TK_SLASH){ lex_next(L); long d=parse_prim(vm,L); v=d?v/d:0; }
    else if (L->cur.kind==TK_PERCENT){ lex_next(L); long d=parse_prim(vm,L); v=d?v%d:0; }
    else break;
  }
  return v;
}
static long parse_add(VM *vm, Lex *L){
  long v=parse_term(vm,L);
  for(;;){
    if (L->cur.kind==TK_PLUS){ lex_next(L); v+=parse_term(vm,L); }
    else if (L->cur.kind==TK_MINUS){ lex_next(L); v-=parse_term(vm,L); }
    else break;
  }
  return v;
}
static long parse_cmp(VM *vm, Lex *L){
  long v=parse_add(vm,L);
  if (L->cur.kind==TK_EQEQ){ lex_next(L); return v==parse_add(vm,L); }
  if (L->cur.kind==TK_NE){ lex_next(L); return v!=parse_add(vm,L); }
  if (L->cur.kind==TK_LT){ lex_next(L); return v<parse_add(vm,L); }
  if (L->cur.kind==TK_LE){ lex_next(L); return v<=parse_add(vm,L); }
  if (L->cur.kind==TK_GT){ lex_next(L); return v>parse_add(vm,L); }
  if (L->cur.kind==TK_GE){ lex_next(L); return v>=parse_add(vm,L); }
  return v;
}
static long parse_expr(VM *vm, Lex *L){
  long v = parse_cmp(vm, L);
  for(;;){
    if (kw(&L->cur,"AND")){
      lex_next(L);
      long r = parse_cmp(vm, L);
      v = (v && r) ? 1 : 0;
    } else if (kw(&L->cur,"OR")){
      lex_next(L);
      long r = parse_cmp(vm, L);
      v = (v || r) ? 1 : 0;
    } else break;
  }
  return v;
}

static int parse_form(VM *vm, Lex *L){
  skip_nl(L);
  if (L->cur.kind==TK_EOF) return 0;

  /* free-standing look */
  if (L->cur.kind==TK_QMARK){
    lex_next(L);
    do_show(vm, NULL); bump(vm); return 1;
  }
  /* free-standing flow dots: ... or ~~~ */
  if (L->cur.kind==TK_TILDE){
    int n=0;
    while (L->cur.kind==TK_TILDE){ n++; lex_next(L); }
    if (L->cur.kind==TK_NUM){ n=(int)L->cur.num; lex_next(L); }
    if (n<1) n=1;
    do_flow(vm,n); bump(vm); return 1;
  }

  /* primary: cube form */
  if (L->cur.kind==TK_LBRACK)
    return parse_cube(vm, L);

  /* bare hold */
  if (kw(&L->cur,"hold")){
    lex_next(L);
    vm->hold_flash=1; vm->ch.hold_flash=1; bump(vm); return 1;
  }


  /* ASYNC / AWAIT / PARALLEL — energy must flow (thread pool + GPU-shaped lanes) */
  if (kw(&L->cur,"ASYNC")){
    lex_next(L);
    if (kw(&L->cur,"HTTP") || kw(&L->cur,"GET") || kw(&L->cur,"POST")){
      char method[8]="GET";
      if (kw(&L->cur,"POST")) snprintf(method,sizeof method,"POST");
      if (kw(&L->cur,"HTTP")){
        lex_next(L);
        if (L->cur.kind==TK_IDENT||L->cur.kind==TK_STR){
          snprintf(method,sizeof method,"%s",L->cur.text); lex_next(L);
        }
      } else lex_next(L);
      if (L->cur.kind!=TK_STR){ fail(vm,"ASYNC HTTP method \"url\" …"); return -1; }
      char url[512]; snprintf(url,sizeof url,"%s",L->cur.text); lex_next(L);
      char body[CUBALC_HOST_STR_MAX]; body[0]=0;
      if (kw(&L->cur,"FILE")||kw(&L->cur,"FROM")){
        lex_next(L);
        if (L->cur.kind!=TK_STR){ fail(vm,"ASYNC HTTP FILE \"path\""); return -1; }
        cubalc_host_result fr;
        if (cubalc_host_read(L->cur.text,&fr)!=0){ fail(vm,fr.err[0]?fr.err:"body file"); return -1; }
        snprintf(body,sizeof body,"%s",fr.str); lex_next(L);
      } else if (L->cur.kind==TK_STR){
        snprintf(body,sizeof body,"%s",L->cur.text); lex_next(L);
      }
      int timeout_ms = 120000;
      if (L->cur.kind==TK_NUM){ timeout_ms=(int)L->cur.num; if(timeout_ms<1000) timeout_ms*=1000; lex_next(L); }
      int jid = cubalc_async_http(method, url, body, timeout_ms);
      if (jid < 0){ fail(vm,"ASYNC HTTP submit failed"); return -1; }
      var_set_num(vm, "ASYNC_ID", jid);
      var_set_num(vm, "JOB", jid);
      if (vm->trace) fprintf(vm->trace, "# async job %d submitted (%s)\n", jid, cubalc_async_backend());
      bump(vm); return 1;
    }
    fail(vm,"ASYNC HTTP …"); return -1;
  }
  if (kw(&L->cur,"AWAIT") || kw(&L->cur,"WAIT_JOB")){
    lex_next(L);
    if (kw(&L->cur,"ALL")){
      lex_next(L);
      int ms = 120000;
      if (L->cur.kind==TK_NUM){ ms=(int)L->cur.num; lex_next(L); }
      if (cubalc_async_await_all(ms)!=0){
        if (vm->trace) fprintf(vm->trace,"# await all timeout\n");
        var_set_num(vm,"OK",0);
      } else var_set_num(vm,"OK",1);
      bump(vm); return 1;
    }
    long jid = 0;
    if (L->cur.kind==TK_NUM){ jid=L->cur.num; lex_next(L); }
    else if (L->cur.kind==TK_IDENT){
      /* AWAIT ASYNC_ID or AWAIT JOB */
      for (int i=0;i<vm->n_vars;i++)
        if (strcmp(vm->vars[i].name,L->cur.text)==0){ jid=vm->vars[i].val; break; }
      if (jid==0 && (kw(&L->cur,"ASYNC_ID")||kw(&L->cur,"JOB"))){
        for (int i=0;i<vm->n_vars;i++)
          if (strcmp(vm->vars[i].name,"ASYNC_ID")==0 || strcmp(vm->vars[i].name,"JOB")==0)
            { jid=vm->vars[i].val; break; }
      }
      lex_next(L);
    } else {
      for (int i=0;i<vm->n_vars;i++)
        if (strcmp(vm->vars[i].name,"ASYNC_ID")==0){ jid=vm->vars[i].val; break; }
    }
    int ms = 180000;
    if (L->cur.kind==TK_NUM){ ms=(int)L->cur.num; lex_next(L); }
    cubalc_async_job job;
    memset(&job,0,sizeof job);
    if (cubalc_async_wait((int)jid, ms, &job)!=0){
      var_set_num(vm,"OK",0);
      var_set_num(vm,"HTTP_CODE",0);
      if (vm->trace) fprintf(vm->trace,"# await timeout job %ld\n", jid);
      bump(vm); return 1;
    }
    snprintf(vm->last_str,sizeof vm->last_str,"%s", job.str);
    vm->last_code = job.code;
    vm->last_n = job.n;
    var_set_str(vm,"LAST", job.str);
    var_set_num(vm,"HTTP_CODE", job.code);
    var_set_num(vm,"LAST_N", job.n);
    var_set_num(vm,"OK", job.ok?1:0);
    if (vm->trace) fprintf(vm->trace,"# await job %ld → code %d ok=%d\n", jid, job.code, job.ok);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"PARALLEL") || kw(&L->cur,"PAR")){
    lex_next(L);
    if (kw(&L->cur,"FLOW") || kw(&L->cur,"TICK")){
      lex_next(L);
      int n=8;
      if (L->cur.kind==TK_NUM){ n=(int)L->cur.num; lex_next(L); }
      ensure_world(vm);
      cubalc_async_chain_flow(&vm->ch, n);
      if (vm->trace) fprintf(vm->trace,"# parallel flow %d (%s)\n", n, cubalc_async_backend());
      bump(vm); return 1;
    }
    if (kw(&L->cur,"COMPAT") || kw(&L->cur,"MATRIX")){
      lex_next(L);
      ensure_world(vm);
      float out[CUBALC_MAX_CUBES*CUBALC_MAX_CUBES];
      int n = cubalc_async_compat_batch(&vm->ch, out, CUBALC_MAX_CUBES);
      long avg = 0;
      if (n>0){
        double s=0; int c=0;
        for (int i=0;i<n;i++) for (int j=0;j<n;j++){ s+=out[i*n+j]; c++; }
        avg = c ? (long)lround(100.0*s/c) : 0;
      }
      var_set_num(vm,"COMPAT_AVG", avg);
      var_set_num(vm,"LAST_N", n);
      if (vm->trace) fprintf(vm->trace,"# parallel compat n=%d avg=%ld (%s)\n",
                             n, avg, cubalc_async_backend());
      bump(vm); return 1;
    }
    fail(vm,"PARALLEL FLOW|COMPAT"); return -1;
  }

  /* SYS host ops — C runtime (Grokium without Python) */
  if (kw(&L->cur,"SYS") || kw(&L->cur,"HOST")){
    lex_next(L);
    skip_nl(L);
    if (kw(&L->cur,"READ")){
      lex_next(L);
      char path[512];
      if (resolve_str_arg(vm, L, path, sizeof path)!=0){
        fail(vm,"SYS READ \"path\"|LAST"); return -1;
      }
      cubalc_host_result hr;
      if (cubalc_host_read(path, &hr)!=0){ fail(vm, hr.err[0]?hr.err:"SYS READ fail"); return -1; }
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n; vm->last_code = 0;
      var_set_str(vm, "LAST", hr.str);
      var_set_num(vm, "LAST_N", hr.n);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"WRITE")){
      lex_next(L);
      char path[512]; path[0]=0;
      if (L->cur.kind==TK_STR){ snprintf(path,sizeof path,"%s",L->cur.text); lex_next(L); }
      else if (L->cur.kind==TK_IDENT){
        if (strcmp(L->cur.text,"LAST")==0) snprintf(path,sizeof path,"%s",vm->last_str);
        else {
          Var *v = var_get(vm, L->cur.text, 0);
          if (v && v->is_str) snprintf(path,sizeof path,"%s",v->sval);
          else { fail(vm,"SYS WRITE path"); return -1; }
        }
        lex_next(L);
      } else { fail(vm,"SYS WRITE \"path\" \"data\""); return -1; }
      const char *data = "";
      char dbuf[CUBALC_HOST_STR_MAX];
      if (L->cur.kind==TK_STR){ snprintf(dbuf,sizeof dbuf,"%s",L->cur.text); data=dbuf; lex_next(L); }
      else if (L->cur.kind==TK_IDENT){
        Var *v = var_get(vm, L->cur.text, 0);
        if (v && v->is_str) data = v->sval;
        else if (strcmp(L->cur.text,"LAST")==0) data = vm->last_str;
        else data = "";
        lex_next(L);
      }
      cubalc_host_result hr;
      if (cubalc_host_write(path, data, &hr)!=0){ fail(vm, hr.err[0]?hr.err:"SYS WRITE fail"); return -1; }
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"ENV")){
      lex_next(L);
      if (L->cur.kind!=TK_STR && L->cur.kind!=TK_IDENT){ fail(vm,"SYS ENV name"); return -1; }
      cubalc_host_result hr;
      cubalc_host_env(L->cur.text, &hr);
      lex_next(L);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_str(vm, "LAST", hr.str);
      var_set_num(vm, "LAST_N", hr.n);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"EXIST") || kw(&L->cur,"EXISTS")){
      lex_next(L);
      char path[512];
      if (resolve_str_arg(vm, L, path, sizeof path)!=0){
        fail(vm,"SYS EXIST \"path\"|LAST"); return -1;
      }
      int e = cubalc_host_exists(path);
      var_set_num(vm, "LAST_N", e);
      var_set_num(vm, "EXIST", e);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"WHICH")){
      lex_next(L);
      if (L->cur.kind!=TK_STR && L->cur.kind!=TK_IDENT){ fail(vm,"SYS WHICH name"); return -1; }
      cubalc_host_result hr;
      if (cubalc_host_which(L->cur.text, &hr)!=0){
        var_set_str(vm, "LAST", "");
        var_set_num(vm, "LAST_N", 0);
        vm->last_n = 0;
      } else {
        snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
        vm->last_n = 1;
        var_set_str(vm, "LAST", hr.str);
        var_set_num(vm, "LAST_N", 1);
      }
      lex_next(L);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"HTTP") || kw(&L->cur,"GET") || kw(&L->cur,"POST")){
      char method[8] = "GET";
      if (kw(&L->cur,"POST")) snprintf(method,sizeof method,"POST");
      if (kw(&L->cur,"HTTP")){ lex_next(L); if (L->cur.kind==TK_IDENT||L->cur.kind==TK_STR){ snprintf(method,sizeof method,"%s",L->cur.text); lex_next(L);} }
      else lex_next(L);
      if (L->cur.kind!=TK_STR){ fail(vm,"SYS HTTP method \"url\" [\"body\"|FILE \"path\"]"); return -1; }
      char url[512]; snprintf(url,sizeof url,"%s",L->cur.text); lex_next(L);
      char body[CUBALC_HOST_STR_MAX]; body[0]=0;
      if (kw(&L->cur,"FILE") || kw(&L->cur,"FROM")){
        lex_next(L);
        if (L->cur.kind!=TK_STR){ fail(vm,"SYS HTTP ... FILE \"path\""); return -1; }
        cubalc_host_result fr;
        if (cubalc_host_read(L->cur.text, &fr)!=0){ fail(vm, fr.err[0]?fr.err:"body file"); return -1; }
        snprintf(body,sizeof body,"%s", fr.str);
        lex_next(L);
      } else if (L->cur.kind==TK_STR){
        snprintf(body,sizeof body,"%s",L->cur.text); lex_next(L);
      }
      cubalc_host_result hr;
      int rc = cubalc_host_http(method, url, body, &hr);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_code = hr.code;
      vm->last_n = hr.n;
      var_set_str(vm, "LAST", hr.str);
      var_set_num(vm, "HTTP_CODE", hr.code);
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "OK", hr.ok ? 1 : 0);
      if (rc!=0 && !hr.ok){
        /* soft-fail for probe: OK=0 not fatal */
        if (vm->trace) fprintf(vm->trace, "# http soft-fail %s\n", hr.err);
      }
      bump(vm); return 1;
    }
    if (kw(&L->cur,"SPAWN")){
      lex_next(L);
      if (L->cur.kind!=TK_STR && L->cur.kind!=TK_IDENT){ fail(vm,"SYS SPAWN bin [args…]"); return -1; }
      char bin[512];
      if (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT){
        /* if not path, which */
        if (strchr(L->cur.text, '/')) snprintf(bin,sizeof bin,"%s",L->cur.text);
        else {
          cubalc_host_result wh;
          if (cubalc_host_which(L->cur.text, &wh)==0) snprintf(bin,sizeof bin,"%s",wh.str);
          else snprintf(bin,sizeof bin,"%s",L->cur.text);
        }
        lex_next(L);
      }
      char *av[16]; char abuf[16][256]; int ac=0;
      av[ac++] = bin;
      while (ac < 15 && (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT || L->cur.kind==TK_NUM)){
        snprintf(abuf[ac], sizeof abuf[ac], "%s", L->cur.text);
        av[ac] = abuf[ac];
        ac++; lex_next(L);
      }
      av[ac] = NULL;
      cubalc_host_result hr;
      cubalc_host_spawn(bin, av, &hr);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      var_set_str(vm, "LAST", hr.str);
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "EXIT", hr.code);
      var_set_num(vm, "OK", hr.ok ? 1 : 0);
      bump(vm); return 1;
    }
    /* SYS JOIN a b → LAST = a/b */
    if (kw(&L->cur,"JOIN") || kw(&L->cur,"PATH")){
      lex_next(L);
      char a[512]="", b[512]="";
      if (L->cur.kind==TK_STR){ snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L); }
      else if (L->cur.kind==TK_IDENT){
        if (strcmp(L->cur.text,"LAST")==0) snprintf(a,sizeof a,"%s",vm->last_str);
        else { Var *v=var_get(vm,L->cur.text,0); if (v&&v->is_str) snprintf(a,sizeof a,"%s",v->sval); }
        lex_next(L);
      }
      if (L->cur.kind==TK_STR){ snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L); }
      else if (L->cur.kind==TK_IDENT){
        if (strcmp(L->cur.text,"LAST")==0) snprintf(b,sizeof b,"%s",vm->last_str);
        else { Var *v=var_get(vm,L->cur.text,0); if (v&&v->is_str) snprintf(b,sizeof b,"%s",v->sval); }
        lex_next(L);
      }
      cubalc_host_result hr;
      if (cubalc_host_join(a,b,&hr)!=0){ fail(vm, hr.err[0]?hr.err:"JOIN"); return -1; }
      snprintf(vm->last_str,sizeof vm->last_str,"%s",hr.str);
      vm->last_n = hr.n;
      var_set_str(vm,"LAST",hr.str);
      var_set_num(vm,"LAST_N",hr.n);
      bump(vm); return 1;
    }
    /* SYS JSON "key" [from LAST] → LAST = field */
    if (kw(&L->cur,"JSON") || kw(&L->cur,"JGET")){
      lex_next(L);
      char key[96]="content";
      if (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT){
        snprintf(key,sizeof key,"%s",L->cur.text); lex_next(L);
      }
      const char *src = vm->last_str;
      cubalc_host_result hr;
      if (cubalc_host_json_get(src, key, &hr)!=0){
        var_set_str(vm,"LAST","");
        var_set_num(vm,"LAST_N",0);
        var_set_num(vm,"OK",0);
        bump(vm); return 1;
      }
      snprintf(vm->last_str,sizeof vm->last_str,"%s",hr.str);
      vm->last_n = hr.n;
      var_set_str(vm,"LAST",hr.str);
      var_set_num(vm,"LAST_N",hr.n);
      var_set_num(vm,"OK",1);
      bump(vm); return 1;
    }
    /* SYS CHAT "local"|"grok" ["model"] — msg from GROKIUM_MSG / CUBALC_MSG env */
    if (kw(&L->cur,"CHAT") || kw(&L->cur,"ASK")){
      lex_next(L);
      char be[32]="local", model[128]="";
      if (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT){
        snprintf(be,sizeof be,"%s",L->cur.text); lex_next(L);
      }
      if (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT){
        /* model or message string */
        if (strcmp(L->cur.text,"local")==0 || strcmp(L->cur.text,"grok")==0 ||
            strncmp(L->cur.text,"grok",4)==0 || strstr(L->cur.text,"/") ||
            strstr(L->cur.text,".gguf")) {
          snprintf(model,sizeof model,"%s",L->cur.text); lex_next(L);
        }
      }
      char msg[2000]; msg[0]=0;
      if (L->cur.kind==TK_STR){
        snprintf(msg,sizeof msg,"%s",L->cur.text); lex_next(L);
      } else {
        const char *e = getenv("GROKIUM_MSG");
        if (!e || !e[0]) e = getenv("CUBALC_MSG");
        if (e) snprintf(msg,sizeof msg,"%s",e);
      }
      if (!msg[0]){ fail(vm,"SYS CHAT needs msg or GROKIUM_MSG"); return -1; }
      if (!model[0]){
        const char *em = getenv("GROKIUM_MODEL");
        if (em && em[0]) snprintf(model,sizeof model,"%s",em);
      }
      const char *st = getenv("CUBALC_STATE");
      cubalc_host_result hr;
      if (cubalc_host_chat(be, model, msg, st, &hr)!=0){
        var_set_str(vm,"LAST", hr.err[0]?hr.err:"chat fail");
        var_set_num(vm,"OK",0);
        var_set_num(vm,"HTTP_CODE", hr.code);
        if (vm->trace) fprintf(vm->trace, "# chat fail %s\n", hr.err);
        bump(vm); return 1; /* soft */
      }
      snprintf(vm->last_str,sizeof vm->last_str,"%s",hr.str);
      vm->last_n = hr.n;
      vm->last_code = hr.code;
      var_set_str(vm,"LAST",hr.str);
      var_set_num(vm,"LAST_N",hr.n);
      var_set_num(vm,"HTTP_CODE",hr.code);
      var_set_num(vm,"OK",1);
      bump(vm); return 1;
    }
    /* SYS ARG n | SYS ARG "NAME" — CUBALC_ARG0… or named env */
    if (kw(&L->cur,"ARG") || kw(&L->cur,"ARGV")){
      lex_next(L);
      char name[64];
      if (L->cur.kind==TK_NUM){
        snprintf(name,sizeof name,"CUBALC_ARG%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT){
        if (strcmp(L->cur.text,"MSG")==0) snprintf(name,sizeof name,"GROKIUM_MSG");
        else if (strcmp(L->cur.text,"BACKEND")==0) snprintf(name,sizeof name,"GROKIUM_BACKEND");
        else if (strcmp(L->cur.text,"MODEL")==0) snprintf(name,sizeof name,"GROKIUM_MODEL");
        else snprintf(name,sizeof name,"%s", L->cur.text);
        lex_next(L);
      } else { fail(vm,"SYS ARG n|name"); return -1; }
      cubalc_host_result hr;
      cubalc_host_env(name, &hr);
      snprintf(vm->last_str,sizeof vm->last_str,"%s",hr.str);
      vm->last_n = hr.n;
      var_set_str(vm,"LAST",hr.str);
      var_set_num(vm,"LAST_N",hr.n);
      bump(vm); return 1;
    }
    /* SYS NUM|INT — parse LAST as integer → LAST_N (CubeBrain digit fold) */
    if (kw(&L->cur,"NUM") || kw(&L->cur,"INT") || kw(&L->cur,"ATOI")){
      lex_next(L);
      const char *s = vm->last_str;
      if (L->cur.kind==TK_STR){ s = L->cur.text; lex_next(L); }
      else if (L->cur.kind==TK_IDENT && strcmp(L->cur.text,"LAST")!=0){
        Var *v = var_get(vm, L->cur.text, 0);
        if (v && v->is_str) s = v->sval;
        else if (v) { var_set_num(vm,"LAST_N", v->val); vm->last_n = v->val; bump(vm); return 1; }
        lex_next(L);
      } else if (L->cur.kind==TK_IDENT && strcmp(L->cur.text,"LAST")==0){
        lex_next(L);
      }
      long n = 0;
      if (s && s[0]) n = strtol(s, NULL, 10);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"LEN") || kw(&L->cur,"LENGTH") || kw(&L->cur,"STRLEN")){
      lex_next(L);
      long n = 0;
      if (L->cur.kind==TK_STR){ n = (long)strlen(L->cur.text); lex_next(L); }
      else if (L->cur.kind==TK_IDENT){
        if (strcmp(L->cur.text,"LAST")==0){ n = (long)strlen(vm->last_str); lex_next(L); }
        else {
          Var *v = var_get(vm, L->cur.text, 0);
          if (v && v->is_str) n = (long)strlen(v->sval);
          else if (v) n = v->val;
          lex_next(L);
        }
      } else n = (long)strlen(vm->last_str);
      vm->last_n = n; var_set_num(vm, "LAST_N", n); var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"TIME") || kw(&L->cur,"NOW") || kw(&L->cur,"EPOCH")){
      lex_next(L);
      long n = (long)time(NULL);
      vm->last_n = n; var_set_num(vm, "LAST_N", n); var_set_num(vm, "TIME", n); var_set_num(vm, "OK", 1);
      char buf[32]; snprintf(buf, sizeof buf, "%ld", n);
      var_set_str(vm, "LAST", buf); snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"APPEND") || kw(&L->cur,"LOG")){
      lex_next(L);
      char path[512]="", data[4096]; data[0]=0;
      if (L->cur.kind==TK_STR){ snprintf(path,sizeof path,"%s",L->cur.text); lex_next(L); }
      else if (L->cur.kind==TK_IDENT){
        if (strcmp(L->cur.text,"LAST")==0) snprintf(path,sizeof path,"%s",vm->last_str);
        else { Var *v=var_get(vm,L->cur.text,0); if(v&&v->is_str) snprintf(path,sizeof path,"%s",v->sval); }
        lex_next(L);
      }
      if (L->cur.kind==TK_STR){ snprintf(data,sizeof data,"%s",L->cur.text); lex_next(L); }
      else if (L->cur.kind==TK_IDENT){
        if (strcmp(L->cur.text,"LAST")==0) snprintf(data,sizeof data,"%s",vm->last_str);
        else { Var *v=var_get(vm,L->cur.text,0); if(v&&v->is_str) snprintf(data,sizeof data,"%s",v->sval);
               else if(v) snprintf(data,sizeof data,"%ld",v->val); }
        lex_next(L);
      }
      FILE *af = fopen(path, "a");
      if (!af){ var_set_num(vm,"OK",0); bump(vm); return 1; }
      fputs(data, af); fputc('\n', af); fclose(af);
      var_set_num(vm,"OK",1); bump(vm); return 1;
    }
    /* SYS HEX|FROMHEX — parse hex string (LAST default) → LAST_N  (I/O codec) */
    if (kw(&L->cur,"HEX") || kw(&L->cur,"FROMHEX") || kw(&L->cur,"XTOI")){
      lex_next(L);
      char s[512]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      /* skip optional 0x / 0X prefix */
      const char *p = s;
      if (p[0]=='0' && (p[1]=='x' || p[1]=='X')) p += 2;
      long n = 0;
      if (p[0]) n = strtol(p, NULL, 16);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS TOHEX [expr] — format int as lowercase hex → LAST (I/O codec) */
    if (kw(&L->cur,"TOHEX") || kw(&L->cur,"ITOH") || kw(&L->cur,"HEXOUT")){
      lex_next(L);
      long n = vm->last_n;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE"))){
        n = parse_expr(vm, L);
      }
      char buf[40];
      snprintf(buf, sizeof buf, "%lx", (unsigned long)n);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS ORD [str|LAST] — first byte code → LAST_N  (I/O codec) */
    if (kw(&L->cur,"ORD") || kw(&L->cur,"CODE") || kw(&L->cur,"BYTE")){
      lex_next(L);
      char s[512]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      long n = s[0] ? (long)(unsigned char)s[0] : 0;
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS CHR [expr] — integer → single-byte string LAST  (I/O codec) */
    if (kw(&L->cur,"CHR") || kw(&L->cur,"CHAR")){
      lex_next(L);
      long n = vm->last_n;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE"))){
        n = parse_expr(vm, L);
      }
      n &= 0xFF;
      char buf[4];
      buf[0] = (char)(unsigned char)n;
      buf[1] = 0;
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MID|SUBSTR|SLICE str start [len] — substring → LAST  (I/O codec) */
    if (kw(&L->cur,"MID") || kw(&L->cur,"SUBSTR") || kw(&L->cur,"SLICE")){
      lex_next(L);
      char s[512]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      long start = 0, len = -1;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          L->cur.kind==TK_IDENT){
        start = parse_expr(vm, L);
        if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
            L->cur.kind==TK_IDENT)
          len = parse_expr(vm, L);
      }
      size_t slen = strlen(s);
      if (start < 0) start = 0;
      if ((size_t)start > slen) start = (long)slen;
      size_t remain = slen - (size_t)start;
      size_t take = (len < 0) ? remain : (size_t)len;
      if (take > remain) take = remain;
      char out[512];
      if (take >= sizeof out) take = sizeof out - 1;
      memcpy(out, s + (size_t)start, take);
      out[take] = 0;
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)take;
      var_set_num(vm, "LAST_N", (long)take);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS CAT|STRCAT a b — concatenate strings → LAST (digit-3 string plane) */
    if (kw(&L->cur,"CAT") || kw(&L->cur,"STRCAT") || kw(&L->cur,"CONCAT")){
      lex_next(L);
      char a[512]="", b[512]="";
      if (resolve_str_arg(vm, L, a, sizeof a) != 0)
        snprintf(a, sizeof a, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, b, sizeof b) != 0) b[0]=0;
      char out[1024];
      snprintf(out, sizeof out, "%s%s", a, b);
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)strlen(out);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS FIND|INDEX hay needle — first index of needle in hay → LAST_N (-1 miss) */
    if (kw(&L->cur,"FIND") || kw(&L->cur,"INDEX") || kw(&L->cur,"STRFIND")){
      lex_next(L);
      char hay[512]="", needle[256]="";
      if (resolve_str_arg(vm, L, hay, sizeof hay) != 0)
        snprintf(hay, sizeof hay, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0]=0;
      long idx = -1;
      if (needle[0]){
        const char *p = strstr(hay, needle);
        if (p) idx = (long)(p - hay);
      } else if (hay[0]) idx = 0;
      vm->last_n = idx;
      var_set_num(vm, "LAST_N", idx);
      var_set_num(vm, "OK", idx >= 0 ? 1 : 0);
      bump(vm); return 1;
    }
    /* SYS EQS|STREQ a b — string equality → LAST_N 1/0 */
    if (kw(&L->cur,"EQS") || kw(&L->cur,"STREQ") || kw(&L->cur,"SEQ")){
      lex_next(L);
      char a[512]="", b[512]="";
      if (resolve_str_arg(vm, L, a, sizeof a) != 0)
        snprintf(a, sizeof a, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, b, sizeof b) != 0) b[0]=0;
      long eq = (strcmp(a, b) == 0) ? 1 : 0;
      vm->last_n = eq;
      var_set_num(vm, "LAST_N", eq);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS HAS|CONTAINS hay needle — 1 if needle in hay */
    if (kw(&L->cur,"HAS") || kw(&L->cur,"CONTAINS") || kw(&L->cur,"INSTR")){
      lex_next(L);
      char hay[512]="", needle[256]="";
      if (resolve_str_arg(vm, L, hay, sizeof hay) != 0)
        snprintf(hay, sizeof hay, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0]=0;
      long hit = (needle[0] && strstr(hay, needle)) ? 1 : 0;
      if (!needle[0]) hit = 1;
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS REVS|STRREV [str] — reverse string → LAST (not cube REVERSE) */
    if (kw(&L->cur,"REVS") || kw(&L->cur,"STRREV") || kw(&L->cur,"SREV")){
      lex_next(L);
      char s[512]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      size_t n = strlen(s);
      char out[512];
      if (n >= sizeof out) n = sizeof out - 1;
      for (size_t i=0;i<n;i++) out[i] = s[n-1-i];
      out[n] = 0;
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)n;
      var_set_num(vm, "LAST_N", (long)n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS UPPER|UCASE [str] — ASCII upper → LAST */
    if (kw(&L->cur,"UPPER") || kw(&L->cur,"UCASE") || kw(&L->cur,"TOUPPER")){
      lex_next(L);
      char s[512]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      for (char *p=s; *p; p++)
        if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 'a' + 'A');
      var_set_str(vm, "LAST", s);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", s);
      vm->last_n = (long)strlen(s);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS LOWER|LCASE [str] — ASCII lower → LAST */
    if (kw(&L->cur,"LOWER") || kw(&L->cur,"LCASE") || kw(&L->cur,"TOLOWER")){
      lex_next(L);
      char s[512]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      for (char *p=s; *p; p++)
        if (*p >= 'A' && *p <= 'Z') *p = (char)(*p - 'A' + 'a');
      var_set_str(vm, "LAST", s);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", s);
      vm->last_n = (long)strlen(s);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    fail(vm, "SYS: READ|WRITE|ENV|EXIST|WHICH|HTTP|SPAWN|JOIN|JSON|CHAT|ARG|NUM|LEN|TIME|APPEND|HEX|TOHEX|ORD|CHR|MID|CAT|FIND|EQS|HAS|REVS|UPPER|LOWER");
    return -1;
  }

  /* ---- legacy verbose (compat) ---- */
  if (kw(&L->cur,"CREED")){
    lex_next(L);
    if (L->cur.kind==TK_STR){
      snprintf(vm->creed,sizeof vm->creed,"%s",L->cur.text);
      snprintf(vm->ch.creed,sizeof vm->ch.creed,"%s",L->cur.text);
      lex_next(L);
    }
    bump(vm); return 1;
  }
  if (kw(&L->cur,"HOLD_FLASH")){
    lex_next(L);
    if (L->cur.kind==TK_NUM){ vm->hold_flash=L->cur.num?1:0; vm->ch.hold_flash=(uint8_t)vm->hold_flash; lex_next(L); }
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SHARE")||kw(&L->cur,"BUDGET")){
    lex_next(L); while (L->cur.kind!=TK_NL && L->cur.kind!=TK_EOF) lex_next(L);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"GENESIS")){
    lex_next(L);
    char plate[512]="NEXUS_COORD v1 | from=play | hold_flash=1 |";
    if (kw(&L->cur,"FROM")){ lex_next(L); if (L->cur.kind==TK_IDENT) lex_next(L); }
    else if (L->cur.kind==TK_STR){ snprintf(plate,sizeof plate,"%s",L->cur.text); lex_next(L); }
    cubalc_matrix gen; cubalc_coord_to_matrix(plate,&gen);
    cubalc_chain_from_initial(&vm->ch,&gen,1);
    vm->ch.hold_flash=(uint8_t)vm->hold_flash;
    bump(vm); return 1;
  }
  /* SCIENCE [LOAD] — inject pure-science constants (public domain; language design) */
  if (kw(&L->cur,"SCIENCE")||kw(&L->cur,"PURESCIENCE")){
    lex_next(L);
    if (kw(&L->cur,"LOAD")||kw(&L->cur,"CONST")||kw(&L->cur,"CONSTANTS")) lex_next(L);
    var_set_num(vm, "PI100", CUBALC_SCI_PI100);
    var_set_num(vm, "E100", CUBALC_SCI_E100);
    var_set_num(vm, "G_EARTH", CUBALC_SCI_G_EARTH10);
    var_set_num(vm, "C_LIGHT", CUBALC_SCI_C_LIGHT);
    var_set_num(vm, "ATM_KPA", CUBALC_SCI_ATM_KPA);
    var_set_num(vm, "WATER_K", CUBALC_SCI_WATER_K);
    var_set_num(vm, "H2O_BP", CUBALC_SCI_H2O_BP_C);
    var_set_num(vm, "R_GAS", CUBALC_SCI_R_J);
    var_set_num(vm, "NA_ORDER", CUBALC_SCI_AVOGADRO_E23);
    var_set_num(vm, "EARTH_R", CUBALC_SCI_EARTH_R_KM);
    var_set_num(vm, "AU_KM", CUBALC_SCI_AU_KM);
    var_set_num(vm, "YEAR_D", CUBALC_SCI_YEAR_D);
    var_set_num(vm, "MOON_D", CUBALC_SCI_MOON_D);
    var_set_num(vm, "SOLAR_C", CUBALC_SCI_SOLAR_C);
    var_set_num(vm, "ATM_O2", CUBALC_SCI_ATM_O2_PCT);
    var_set_num(vm, "ATM_N2", CUBALC_SCI_ATM_N2_PCT);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"CUBE")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"CUBE id"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    char role[48]; snprintf(role,sizeof role,"%s",id); int proton=1;
    while (L->cur.kind==TK_IDENT){
      if (kw(&L->cur,"ROLE")){ lex_next(L); if (L->cur.kind==TK_IDENT){ snprintf(role,sizeof role,"%s",L->cur.text); lex_next(L);} }
      else if (kw(&L->cur,"PROTON")){ lex_next(L); if (L->cur.kind==TK_NUM){ proton=L->cur.num?1:0; lex_next(L);} }
      else break;
    }
    place_cube(vm,id,role,proton); bump(vm); return 1;
  }
  /* PLUG — only cubes; pluggable I/O wire (matrix-compatible) */
  if (kw(&L->cur,"PLUG")||kw(&L->cur,"WIRE")||kw(&L->cur,"IO_PLUG")){
    lex_next(L);
    if (kw(&L->cur,"RING")){ lex_next(L); do_ring(vm); bump(vm); return 1; }
    if (L->cur.kind!=TK_IDENT){ fail(vm,"PLUG cube_a cube_b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"PLUG cube_b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    do_plug(vm,a,b); bump(vm); return 1;
  }
  /* UNPLUG a b — detach pluggable I/O */
  if (kw(&L->cur,"UNPLUG")||kw(&L->cur,"DETACH")||kw(&L->cur,"DISCONNECT")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"UNPLUG a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"UNPLUG a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    do_unplug(vm,a,b); bump(vm); return 1;
  }
  /* REVERSE a b — reverse pluggable I/O direction (IN↔OUT) */
  if (kw(&L->cur,"REVERSE")||kw(&L->cur,"REV")||kw(&L->cur,"FLIP_IO")||kw(&L->cur,"IOR")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"REVERSE a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"REVERSE a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    do_reverse(vm,a,b); bump(vm); return 1;
  }
  /* IO cube IN|OUT [face] — declare pluggable port direction on a cube */
  if (kw(&L->cur,"IO")||kw(&L->cur,"PORT")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"IO cube IN|OUT [face]"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int is_out = 1;
    if (kw(&L->cur,"IN")||kw(&L->cur,"RECV")||kw(&L->cur,"RX")){ is_out=0; lex_next(L); }
    else if (kw(&L->cur,"OUT")||kw(&L->cur,"EMIT")||kw(&L->cur,"TX")||kw(&L->cur,"SEND")){ is_out=1; lex_next(L); }
    int face = 0;
    if (L->cur.kind==TK_NUM){ face=(int)L->cur.num; lex_next(L); }
    do_io(vm, id, face, is_out); bump(vm); return 1;
  }
  /* NEST parent child — cubes may nest */
  if (kw(&L->cur,"NEST")||kw(&L->cur,"INSIDE")||kw(&L->cur,"CONTAIN")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"NEST parent child"); return -1; }
    char p[48]; snprintf(p,sizeof p,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"NEST parent child"); return -1; }
    char c[48]; snprintf(c,sizeof c,"%s",L->cur.text); lex_next(L);
    do_nest(vm, p, c); bump(vm); return 1;
  }
  /* UNNEST child — detach from parent */
  if (kw(&L->cur,"UNNEST")||kw(&L->cur,"EJECT")||kw(&L->cur,"DETACH_NEST")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"UNNEST child"); return -1; }
    char c[48]; snprintf(c,sizeof c,"%s",L->cur.text); lex_next(L);
    do_unnest(vm, c); bump(vm); return 1;
  }
  /* COMPILE cube | COMPILE ALL — each cube → matrix; no flow → no compile */
  if (kw(&L->cur,"COMPILE")||kw(&L->cur,"TOMATRIX")||kw(&L->cur,"MATERIALIZE")){
    lex_next(L);
    if (kw(&L->cur,"ALL")||kw(&L->cur,"CHAIN")||kw(&L->cur,"WORLD")){
      lex_next(L);
      do_compile_all(vm); bump(vm); return 1;
    }
    if (L->cur.kind!=TK_IDENT){ fail(vm,"COMPILE cube|ALL"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    do_compile_cube(vm, id); bump(vm); return 1;
  }
  if (kw(&L->cur,"IMPULSE")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"IMPULSE"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int p=1; if (L->cur.kind==TK_NUM){ p=L->cur.num?1:0; lex_next(L); }
    cubalc_chain_impulse(&vm->ch,id,(uint8_t)p); bump(vm); return 1;
  }
  /* FLOW [DIR|IO] n — free-flow energy; DIR respects OUT→IN only (pluggable I/O) */
  if (kw(&L->cur,"FLOW")||kw(&L->cur,"TICK")){
    lex_next(L);
    int directed = 0;
    if (kw(&L->cur,"DIR")||kw(&L->cur,"DIRECTED")||kw(&L->cur,"IO")||kw(&L->cur,"OUT")){
      directed = 1; lex_next(L);
    }
    int n=8;
    if (L->cur.kind==TK_NUM){ n=(int)L->cur.num; lex_next(L); }
    if (n < 1) n = 1;
    if (n > 1000) n = 1000;
    ensure_world(vm);
    if (directed) {
      for (int i = 0; i < n; i++) cubalc_chain_flow_directed(&vm->ch);
    } else {
      do_flow(vm, n);
    }
    bump(vm); return 1;
  }
  if (kw(&L->cur,"OS_ASPECTS")||kw(&L->cur,"SPAWN_OS")){
    lex_next(L); ensure_world(vm); cubalc_chain_os_aspects(&vm->ch); bump(vm); return 1;
  }
  if (kw(&L->cur,"PRINT")){
    lex_next(L);
    char line[256]; size_t o=0; line[0]=0;
    if (L->cur.kind==TK_STR){ snprintf(line,sizeof line,"%s",L->cur.text); o=strlen(line); lex_next(L); }
    while (L->cur.kind!=TK_NL && L->cur.kind!=TK_EOF && L->cur.kind!=TK_LBRACK){
      if (L->cur.kind==TK_IDENT && (kw(&L->cur,"ASSERT")||kw(&L->cur,"LET")||kw(&L->cur,"CUBE")||kw(&L->cur,"PRINT"))) break;
      long v=parse_expr(vm,L);
      int n=snprintf(line+o,sizeof line-o,"%s%ld",o?" ":"",v); if(n>0)o+=(size_t)n;
      if (o>=sizeof line) break;
    }
    if (vm->trace) fprintf(vm->trace,"%s\n",line);
    if (vm->res) snprintf(vm->res->last_print,sizeof vm->res->last_print,"%s",line);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"ASSERT")){
    lex_next(L);
    long v=parse_expr(vm,L);
    if (v){ if (vm->res) vm->res->asserts_ok++; if (vm->trace) fprintf(vm->trace,"# ok\n"); }
    else { if (vm->res) vm->res->asserts_fail++; fail(vm,"ASSERT failed"); return -1; }
    bump(vm); return 1;
  }
  if (kw(&L->cur,"LET")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"LET"); return -1; }
    char name[48]; snprintf(name,sizeof name,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_EQ){ fail(vm,"LET ="); return -1; }
    lex_next(L);
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
  /* ---- digit-1 data plane: cells + stack ---- */
  if (kw(&L->cur,"CELLSET")||kw(&L->cur,"SLOTSET")||kw(&L->cur,"STORE")){
    lex_next(L);
    long i = parse_expr(vm,L);
    long v = 0;
    if (L->cur.kind==TK_EQ){ lex_next(L); v = parse_expr(vm,L); }
    else if (L->cur.kind!=TK_NL && L->cur.kind!=TK_EOF)
      v = parse_expr(vm,L);
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    vm->cells[(int)i] = v;
    var_set_num(vm, "LAST_N", v);
    vm->last_n = v;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"CELLGET")||kw(&L->cur,"SLOTGET")||kw(&L->cur,"LOAD")){
    lex_next(L);
    long i = parse_expr(vm,L);
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long v = vm->cells[(int)i];
    var_set_num(vm, "LAST_N", v);
    vm->last_n = v;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SWAPCELL")||kw(&L->cur,"CELLSWAP")||kw(&L->cur,"SWAPSLOT")){
    lex_next(L);
    long i = parse_expr(vm,L);
    long j = parse_expr(vm,L);
    if (i < 0) i = 0; if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (j < 0) j = 0; if (j >= CUBALC_CELL_N) j = CUBALC_CELL_N - 1;
    long t = vm->cells[(int)i];
    vm->cells[(int)i] = vm->cells[(int)j];
    vm->cells[(int)j] = t;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"CLEARCELLS")||kw(&L->cur,"CELLSZERO")){
    lex_next(L);
    memset(vm->cells, 0, sizeof vm->cells);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* INC / DEC var | CELL i — loop-friendly mutators (digit-9 data fold) */
  if (kw(&L->cur,"INC")||kw(&L->cur,"INCR")||kw(&L->cur,"++")){
    lex_next(L);
    if (kw(&L->cur,"CELL")||kw(&L->cur,"SLOT")){
      lex_next(L);
      long i = parse_expr(vm,L);
      if (i < 0) i = 0; if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
      long step = 1;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"PRINT") && !kw(&L->cur,"END")))
        step = parse_expr(vm,L);
      vm->cells[(int)i] += step;
      var_set_num(vm, "LAST_N", vm->cells[(int)i]);
      vm->last_n = vm->cells[(int)i];
    } else if (L->cur.kind==TK_IDENT){
      char name[48]; snprintf(name,sizeof name,"%s",L->cur.text); lex_next(L);
      long step = 1;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"PRINT") && !kw(&L->cur,"END")))
        step = parse_expr(vm,L);
      Var *v = var_get(vm, name, 1);
      if (v){ v->is_str=0; v->val += step; }
      long nv = v ? v->val : 0;
      var_set_num(vm, "LAST_N", nv); vm->last_n = nv;
    } else { fail(vm,"INC name|CELL i"); return -1; }
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"DEC")||kw(&L->cur,"DECR")||kw(&L->cur,"--")){
    lex_next(L);
    if (kw(&L->cur,"CELL")||kw(&L->cur,"SLOT")){
      lex_next(L);
      long i = parse_expr(vm,L);
      if (i < 0) i = 0; if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
      long step = 1;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"PRINT") && !kw(&L->cur,"END")))
        step = parse_expr(vm,L);
      vm->cells[(int)i] -= step;
      var_set_num(vm, "LAST_N", vm->cells[(int)i]);
      vm->last_n = vm->cells[(int)i];
    } else if (L->cur.kind==TK_IDENT){
      char name[48]; snprintf(name,sizeof name,"%s",L->cur.text); lex_next(L);
      long step = 1;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"PRINT") && !kw(&L->cur,"END")))
        step = parse_expr(vm,L);
      Var *v = var_get(vm, name, 1);
      if (v){ v->is_str=0; v->val -= step; }
      long nv = v ? v->val : 0;
      var_set_num(vm, "LAST_N", nv); vm->last_n = nv;
    } else { fail(vm,"DEC name|CELL i"); return -1; }
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SUMCELL [lo [hi]] · MINCELL · MAXCELL — fold over cell bank */
  if (kw(&L->cur,"SUMCELL")||kw(&L->cur,"CELLSUM")||
      kw(&L->cur,"MINCELL")||kw(&L->cur,"CELLMIN")||
      kw(&L->cur,"MAXCELL")||kw(&L->cur,"CELLMAX")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op; *p; p++) if (*p>='a'&&*p<='z') *p = (char)(*p - 'a' + 'A');
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
    long acc = 0;
    int first = 1;
    long mn = 0, mx = 0;
    for (long i=lo; i<=hi; i++){
      long v = vm->cells[(int)i];
      acc += v;
      if (first){ mn = mx = v; first = 0; }
      else { if (v < mn) mn = v; if (v > mx) mx = v; }
    }
    long out = acc;
    if (strcmp(op,"MINCELL")==0 || strcmp(op,"CELLMIN")==0) out = first ? 0 : mn;
    else if (strcmp(op,"MAXCELL")==0 || strcmp(op,"CELLMAX")==0) out = first ? 0 : mx;
    var_set_num(vm, "LAST_N", out);
    vm->last_n = out;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"PUSH")){
    lex_next(L);
    long v = parse_expr(vm,L);
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp++] = v;
    var_set_num(vm, "SP", vm->sp);
    var_set_num(vm, "LAST_N", v);
    vm->last_n = v;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"POP")){
    lex_next(L);
    if (vm->sp <= 0){
      var_set_num(vm, "OK", 0);
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      bump(vm); return 1;
    }
    long v = vm->stack[--vm->sp];
    var_set_num(vm, "SP", vm->sp);
    var_set_num(vm, "LAST_N", v);
    vm->last_n = v;
    var_set_num(vm, "OK", 1);
    /* optional: POP name → store into var */
    if (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
        !kw(&L->cur,"PUSH") && !kw(&L->cur,"POP") && !kw(&L->cur,"PRINT") &&
        !kw(&L->cur,"END") && !kw(&L->cur,"ELSE")){
      char name[48]; snprintf(name,sizeof name,"%s",L->cur.text); lex_next(L);
      var_set_num(vm, name, v);
    }
    bump(vm); return 1;
  }
  if (kw(&L->cur,"PEEK")){
    lex_next(L);
    long v = (vm->sp > 0) ? vm->stack[vm->sp - 1] : 0;
    var_set_num(vm, "LAST_N", v);
    vm->last_n = v;
    var_set_num(vm, "OK", vm->sp > 0 ? 1 : 0);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"CLEARSTACK")||kw(&L->cur,"STACKCLEAR")||kw(&L->cur,"DROPALL")){
    lex_next(L);
    vm->sp = 0;
    var_set_num(vm, "SP", 0);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-4 data plane: Forth-style stack combinators + FILLCELL */
  if (kw(&L->cur,"DUP")||kw(&L->cur,"STACKDUP")){
    lex_next(L);
    if (vm->sp <= 0){
      var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0;
      bump(vm); return 1;
    }
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[vm->sp - 1];
    vm->stack[vm->sp++] = v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DROP")||kw(&L->cur,"STACKDROP")){
    lex_next(L);
    if (vm->sp <= 0){
      var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0;
      bump(vm); return 1;
    }
    long v = vm->stack[--vm->sp];
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SWAP")||kw(&L->cur,"STACKSWAP")||kw(&L->cur,"SWAPTOP")){
    /* SWAP only if not already consumed as SWAPCELL (checked earlier) */
    lex_next(L);
    if (vm->sp < 2){
      var_set_num(vm,"OK",0); bump(vm); return 1;
    }
    long t = vm->stack[vm->sp-1];
    vm->stack[vm->sp-1] = vm->stack[vm->sp-2];
    vm->stack[vm->sp-2] = t;
    var_set_num(vm,"LAST_N",vm->stack[vm->sp-1]); vm->last_n=vm->stack[vm->sp-1];
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"OVER")||kw(&L->cur,"STACKOVER")){
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[vm->sp - 2];
    vm->stack[vm->sp++] = v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ROT")||kw(&L->cur,"ROTSTACK")||kw(&L->cur,"STACKROT")){
    /* a b c → b c a  (rotate top 3 left) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-3], b = vm->stack[vm->sp-2], c = vm->stack[vm->sp-1];
    vm->stack[vm->sp-3] = b;
    vm->stack[vm->sp-2] = c;
    vm->stack[vm->sp-1] = a;
    var_set_num(vm,"LAST_N",a); vm->last_n=a;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"PICK")||kw(&L->cur,"STACKPICK")){
    /* PICK n — copy n-th under top (0=top) onto stack; depth from TOS */
    lex_next(L);
    long n = 0;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE")))
      n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (vm->sp <= 0 || n >= vm->sp){
      var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0;
      bump(vm); return 1;
    }
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[vm->sp - 1 - (int)n];
    vm->stack[vm->sp++] = v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack combinators ext: QDUP NDUP SREVERSE */
  if (kw(&L->cur,"QDUP")||kw(&L->cur,"DUPNZ")||kw(&L->cur,"DUPIF")||
      kw(&L->cur,"DUPNONZERO")){
    /* QDUP — duplicate TOS only if nonzero (Forth ?DUP) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[vm->sp - 1];
    if (v != 0){
      if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
      vm->stack[vm->sp++] = v;
    }
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"NDUP")||kw(&L->cur,"DUPN")||kw(&L->cur,"STACKNDUP")){
    /* NDUP n — duplicate top n items (append copy of top n) */
    lex_next(L);
    long n = 0;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE")))
      n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (n == 0){ var_set_num(vm,"OK",1); bump(vm); return 1; }
    if (vm->sp < n){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + n > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    int base = vm->sp - (int)n;
    long last = 0;
    for (int i = 0; i < (int)n; i++){
      last = vm->stack[base + i];
      vm->stack[vm->sp++] = last;
    }
    var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SREVERSE")||kw(&L->cur,"REVSTACK")||kw(&L->cur,"STACKREV")||
      kw(&L->cur,"SREVST")){
    /* SREVERSE [n] — reverse top n items (omit n → whole stack) */
    lex_next(L);
    int have_n = 0;
    long n = 0;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE") &&
         !kw(&L->cur,"PUSH") && !kw(&L->cur,"POP") && !kw(&L->cur,"CLEARSTACK") &&
         !kw(&L->cur,"PEEK") && !kw(&L->cur,"DROP") && !kw(&L->cur,"DUP"))){
      n = parse_expr(vm,L);
      have_n = 1;
    }
    if (!have_n) n = (long)vm->sp;
    if (n < 0) n = 0;
    if (have_n && n > (long)vm->sp){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n > 1){
      int lo = vm->sp - (int)n;
      int hi = vm->sp - 1;
      while (lo < hi){
        long t = vm->stack[lo];
        vm->stack[lo] = vm->stack[hi];
        vm->stack[hi] = t;
        lo++; hi--;
      }
    }
    long last = (vm->sp > 0) ? vm->stack[vm->sp - 1] : 0;
    var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack depth plane: NIP TUCK 2DUP 2DROP 2SWAP 2OVER 2ROT 2NIP ROLL DEPTH */
  if (kw(&L->cur,"NIP")||kw(&L->cur,"STACKNIP")){
    /* a b → b  (drop under top) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp - 2] = vm->stack[vm->sp - 1];
    vm->sp--;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",vm->stack[vm->sp-1]); vm->last_n=vm->stack[vm->sp-1];
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"TUCK")||kw(&L->cur,"STACKTUCK")){
    /* a b → b a b */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-2], b = vm->stack[vm->sp-1];
    vm->stack[vm->sp-2] = b;
    vm->stack[vm->sp-1] = a;
    vm->stack[vm->sp++] = b;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",b); vm->last_n=b;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"2DUP")||kw(&L->cur,"DDUP")||kw(&L->cur,"DUP2")){
    /* a b → a b a b */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 2 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-2], b = vm->stack[vm->sp-1];
    vm->stack[vm->sp++] = a;
    vm->stack[vm->sp++] = b;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",b); vm->last_n=b;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"2DROP")||kw(&L->cur,"DDROP")||kw(&L->cur,"DROP2")){
    /* a b → (empty of top 2) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->sp -= 2;
    var_set_num(vm,"SP",vm->sp);
    if (vm->sp > 0){ var_set_num(vm,"LAST_N",vm->stack[vm->sp-1]); vm->last_n=vm->stack[vm->sp-1]; }
    else { var_set_num(vm,"LAST_N",0); vm->last_n=0; }
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"2SWAP")||kw(&L->cur,"DSWAP")||kw(&L->cur,"SWAP2")){
    /* a b c d → c d a b */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-4], b = vm->stack[vm->sp-3];
    long c = vm->stack[vm->sp-2], d = vm->stack[vm->sp-1];
    vm->stack[vm->sp-4] = c; vm->stack[vm->sp-3] = d;
    vm->stack[vm->sp-2] = a; vm->stack[vm->sp-1] = b;
    var_set_num(vm,"LAST_N",b); vm->last_n=b;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"2OVER")||kw(&L->cur,"DOVER")||kw(&L->cur,"OVER2")){
    /* a b c d → a b c d a b  (copy pair under top pair) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 2 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-4], b = vm->stack[vm->sp-3];
    vm->stack[vm->sp++] = a;
    vm->stack[vm->sp++] = b;
    var_set_num(vm,"LAST_N",b); vm->last_n=b;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"2ROT")||kw(&L->cur,"DROT")||kw(&L->cur,"ROT2")){
    /* a b c d e f → c d e f a b  (rotate three pairs left) */
    lex_next(L);
    if (vm->sp < 6){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-6], b = vm->stack[vm->sp-5];
    long c = vm->stack[vm->sp-4], d = vm->stack[vm->sp-3];
    long e = vm->stack[vm->sp-2], f = vm->stack[vm->sp-1];
    vm->stack[vm->sp-6] = c; vm->stack[vm->sp-5] = d;
    vm->stack[vm->sp-4] = e; vm->stack[vm->sp-3] = f;
    vm->stack[vm->sp-2] = a; vm->stack[vm->sp-1] = b;
    var_set_num(vm,"LAST_N",b); vm->last_n=b;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"2NIP")||kw(&L->cur,"DNIP")||kw(&L->cur,"NIP2")){
    /* a b c d → a b d  (drop third under TOS) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[vm->sp-1];
    vm->stack[vm->sp-2] = d;
    vm->sp--;
    var_set_num(vm,"LAST_N",d); vm->last_n=d;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ROLL")||kw(&L->cur,"STACKROLL")){
    /* ROLL n — rotate top (n+1) items: n-th under top becomes TOS
     * n=0 no-op; n=1 ≡ SWAP; n=2 ≡ ROT */
    lex_next(L);
    long n = 0;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE")))
      n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (n == 0){
      var_set_num(vm,"OK", vm->sp > 0 ? 1 : 0);
      if (vm->sp > 0){ var_set_num(vm,"LAST_N",vm->stack[vm->sp-1]); vm->last_n=vm->stack[vm->sp-1]; }
      bump(vm); return 1;
    }
    if (vm->sp <= n){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    /* take item at depth n, shift others down, place on top */
    long v = vm->stack[vm->sp - 1 - (int)n];
    for (int i = (int)n; i > 0; i--)
      vm->stack[vm->sp - 1 - i] = vm->stack[vm->sp - i];
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DEPTH")||kw(&L->cur,"STACKDEPTH")){
    /* DEPTH — push current stack depth */
    lex_next(L);
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = (long)vm->sp;
    vm->stack[vm->sp++] = d;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",d); vm->last_n=d;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack ALU: binary ops a b → r ; unary NEG/ABS on TOS */
  if (kw(&L->cur,"ADD")||kw(&L->cur,"SUB")||kw(&L->cur,"MUL")||
      kw(&L->cur,"DIV")||kw(&L->cur,"MOD")||
      kw(&L->cur,"STACKADD")||kw(&L->cur,"STACKSUB")||kw(&L->cur,"STACKMUL")||
      kw(&L->cur,"STACKDIV")||kw(&L->cur,"STACKMOD")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (strcmp(op,"ADD")==0 || strcmp(op,"STACKADD")==0) r = a + b;
    else if (strcmp(op,"SUB")==0 || strcmp(op,"STACKSUB")==0) r = a - b;
    else if (strcmp(op,"MUL")==0 || strcmp(op,"STACKMUL")==0) r = a * b;
    else if (strcmp(op,"DIV")==0 || strcmp(op,"STACKDIV")==0) r = b ? (a / b) : 0;
    else if (strcmp(op,"MOD")==0 || strcmp(op,"STACKMOD")==0) r = b ? (a % b) : 0;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNEG")||kw(&L->cur,"STACKNEG")||kw(&L->cur,"NEGATE")){
    /* unary negate TOS (NEG alone is expr form elsewhere) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp - 1] = -vm->stack[vm->sp - 1];
    var_set_num(vm,"LAST_N",vm->stack[vm->sp-1]); vm->last_n=vm->stack[vm->sp-1];
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SABS")||kw(&L->cur,"STACKABS")){
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[vm->sp - 1];
    if (v < 0) v = -v;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 stack number theory / div modes: SPOW SGCD SLCM SSQR SISQRT SDIVCEIL SDIVFLOOR */
  if (kw(&L->cur,"SSQR")||kw(&L->cur,"STACKSQR")||kw(&L->cur,"SSQUARE")||
      kw(&L->cur,"SISQRT")||kw(&L->cur,"SSQRT")||kw(&L->cur,"STACKISQRT")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (strcmp(op,"SSQR")==0 || strcmp(op,"STACKSQR")==0 || strcmp(op,"SSQUARE")==0)
      r = a * a;
    else {
      /* SISQRT / SSQRT / STACKISQRT — integer square root; neg → 0 */
      if (a < 0) r = 0;
      else {
        long t = 0;
        while ((t + 1) * (t + 1) <= a) t++;
        r = t;
      }
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPOW")||kw(&L->cur,"STACKPOW")||kw(&L->cur,"SPOWER")||
      kw(&L->cur,"SGCD")||kw(&L->cur,"STACKGCD")||
      kw(&L->cur,"SLCM")||kw(&L->cur,"STACKLCM")||
      kw(&L->cur,"SDIVCEIL")||kw(&L->cur,"SCEILDIV")||kw(&L->cur,"STACKDIVCEIL")||
      kw(&L->cur,"SDIVFLOOR")||kw(&L->cur,"SFLOORDIV")||kw(&L->cur,"STACKDIVFLOOR")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (strcmp(op,"SPOW")==0 || strcmp(op,"STACKPOW")==0 || strcmp(op,"SPOWER")==0){
      long e = b;
      if (e < 0) r = 0;
      else {
        r = 1;
        while (e-- > 0) r *= a;
      }
    } else if (strcmp(op,"SGCD")==0 || strcmp(op,"STACKGCD")==0){
      long x = a < 0 ? -a : a, y = b < 0 ? -b : b;
      while (y){ long t = x % y; x = y; y = t; }
      r = x;
    } else if (strcmp(op,"SLCM")==0 || strcmp(op,"STACKLCM")==0){
      long x = a < 0 ? -a : a, y = b < 0 ? -b : b;
      if (!x || !y) r = 0;
      else {
        long g = x, h = y;
        while (h){ long t = g % h; g = h; h = t; }
        r = (x / g) * y;
      }
    } else if (strcmp(op,"SDIVCEIL")==0 || strcmp(op,"SCEILDIV")==0 ||
               strcmp(op,"STACKDIVCEIL")==0){
      if (b == 0) r = 0;
      else if (a >= 0 && b > 0) r = (a + b - 1) / b;
      else if (a <= 0 && b < 0){
        long aa = -a, bb = -b;
        r = (aa + bb - 1) / bb;
      } else r = a / b;
    } else {
      /* SDIVFLOOR / SFLOORDIV / STACKDIVFLOOR */
      if (b == 0) r = 0;
      else {
        long q = a / b, rem = a % b;
        if (rem != 0 && ((a < 0) != (b < 0))) q--;
        r = q;
      }
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2/6 stack unary math: SFACT SILOG2 SILOG10 SODD SEVEN SFIB ... */
  if (kw(&L->cur,"SFACT")||kw(&L->cur,"STACKFACT")||kw(&L->cur,"SFACTORIAL")||
      kw(&L->cur,"SILOG2")||kw(&L->cur,"SLOG2")||kw(&L->cur,"STACKILOG2")||
      kw(&L->cur,"SILOG10")||kw(&L->cur,"SLOG10")||kw(&L->cur,"STACKILOG10")||
      kw(&L->cur,"SODD")||kw(&L->cur,"STACKODD")||
      kw(&L->cur,"SEVEN")||kw(&L->cur,"STACKEVEN")||
      kw(&L->cur,"SFIB")||kw(&L->cur,"SFIBONACCI")||kw(&L->cur,"STACKFIB")||
      kw(&L->cur,"SISPRIME")||kw(&L->cur,"SPRIME")||kw(&L->cur,"SPRIMEP")||
      kw(&L->cur,"SISPOW2")||kw(&L->cur,"SISPOWER2")||kw(&L->cur,"SPOW2P")||
      kw(&L->cur,"SPOW2")||kw(&L->cur,"STACKPOW2")||
      kw(&L->cur,"SPOW10")||kw(&L->cur,"STENPOW")||kw(&L->cur,"STACKPOW10")||
      kw(&L->cur,"SNDIGITS")||kw(&L->cur,"SNDIG")||kw(&L->cur,"STACKNDIGITS")||
      kw(&L->cur,"SDIGSUM")||kw(&L->cur,"SDIGITSUM")||kw(&L->cur,"STACKDIGSUM")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (strcmp(op,"SFACT")==0 || strcmp(op,"STACKFACT")==0 || strcmp(op,"SFACTORIAL")==0){
      if (a < 0) r = 0;
      else {
        if (a > 20) a = 20;
        r = 1;
        for (long i = 2; i <= a; i++) r *= i;
      }
    } else if (strcmp(op,"SILOG2")==0 || strcmp(op,"SLOG2")==0 || strcmp(op,"STACKILOG2")==0){
      if (a <= 0) r = -1;
      else {
        unsigned long u = (unsigned long)a;
        r = -1;
        while (u){ r++; u >>= 1; }
      }
    } else if (strcmp(op,"SILOG10")==0 || strcmp(op,"SLOG10")==0 || strcmp(op,"STACKILOG10")==0){
      if (a <= 0) r = -1;
      else {
        r = 0;
        long x = a;
        while (x >= 10){ r++; x /= 10; }
      }
    } else if (strcmp(op,"SODD")==0 || strcmp(op,"STACKODD")==0){
      r = (a & 1L) ? 1 : 0;
    } else if (strcmp(op,"SEVEN")==0 || strcmp(op,"STACKEVEN")==0){
      r = (a & 1L) ? 0 : 1;
    } else if (strcmp(op,"SFIB")==0 || strcmp(op,"SFIBONACCI")==0 || strcmp(op,"STACKFIB")==0){
      if (a <= 0) r = 0;
      else if (a == 1 || a == 2) r = 1;
      else {
        if (a > 92) a = 92;
        long f0 = 0, f1 = 1;
        for (long i = 2; i <= a; i++){
          long f2 = f0 + f1;
          f0 = f1; f1 = f2;
        }
        r = f1;
      }
    } else if (strcmp(op,"SISPRIME")==0 || strcmp(op,"SPRIME")==0 || strcmp(op,"SPRIMEP")==0){
      if (a <= 1) r = 0;
      else if (a <= 3) r = 1;
      else if ((a % 2) == 0 || (a % 3) == 0) r = 0;
      else {
        r = 1;
        for (long i = 5; i * i <= a; i += 6){
          if ((a % i) == 0 || (a % (i + 2)) == 0){ r = 0; break; }
        }
      }
    } else if (strcmp(op,"SISPOW2")==0 || strcmp(op,"SISPOWER2")==0 || strcmp(op,"SPOW2P")==0){
      if (a <= 0) r = 0;
      else {
        unsigned long u = (unsigned long)a;
        r = ((u & (u - 1ul)) == 0ul) ? 1 : 0;
      }
    } else if (strcmp(op,"SPOW2")==0 || strcmp(op,"STACKPOW2")==0){
      if (a < 0 || a > 62) r = 0;
      else r = 1L << a;
    } else if (strcmp(op,"SPOW10")==0 || strcmp(op,"STENPOW")==0 || strcmp(op,"STACKPOW10")==0){
      if (a < 0 || a > 18) r = 0;
      else {
        r = 1;
        for (long i = 0; i < a; i++) r *= 10;
      }
    } else if (strcmp(op,"SNDIGITS")==0 || strcmp(op,"SNDIG")==0 || strcmp(op,"STACKNDIGITS")==0){
      long x = a < 0 ? -a : a;
      if (x == 0) r = 1;
      else {
        r = 0;
        while (x){ r++; x /= 10; }
      }
    } else {
      /* SDIGSUM / SDIGITSUM / STACKDIGSUM */
      long x = a < 0 ? -a : a;
      r = 0;
      if (x == 0) r = 0;
      else while (x){ r += x % 10; x /= 10; }
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMODINV")||kw(&L->cur,"SINVMOD")||kw(&L->cur,"STACKMODINV")){
    /* a m → a^{-1} mod m (0 if none) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (m > 1){
      long aa = a % m; if (aa < 0) aa += m;
      if (aa != 0){
        long t = 0, nt = 1;
        long rr = m, nr = aa;
        while (nr != 0){
          long q = rr / nr;
          long tmp = nt; nt = t - q * nt; t = tmp;
          tmp = nr; nr = rr - q * nr; rr = tmp;
        }
        if (rr == 1){
          if (t < 0) t += m;
          r = t;
        }
      }
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMULMOD")||kw(&L->cur,"STACKMULMOD")||
      kw(&L->cur,"SPOWMOD")||kw(&L->cur,"STACKPOWMOD")){
    /* a b m → (a*b)%m or pow(a,b)%m */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (m <= 0) r = 0;
    else if (strcmp(op,"SMULMOD")==0 || strcmp(op,"STACKMULMOD")==0){
      long x = a % m; if (x < 0) x += m;
      long y = b % m; if (y < 0) y += m;
      r = 0;
      while (y > 0){
        if (y & 1) r = (r + x) % m;
        x = (x + x) % m;
        y >>= 1;
      }
    } else {
      /* SPOWMOD / STACKPOWMOD */
      long base = a % m; if (base < 0) base += m;
      long exp = b;
      if (exp < 0) r = 0;
      else {
        r = 1 % m;
        while (exp > 0){
          if (exp & 1){
            long y = r, x = base, acc = 0;
            while (y > 0){
              if (y & 1) acc = (acc + x) % m;
              x = (x + x) % m;
              y >>= 1;
            }
            r = acc;
          }
          {
            long x = base, acc = 0, y = base;
            while (y > 0){
              if (y & 1) acc = (acc + x) % m;
              x = (x + x) % m;
              y >>= 1;
            }
            base = acc;
          }
          exp >>= 1;
        }
      }
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 stack combinatorics + add/sub mod: SBINOM SPERM SADDMOD SSUBMOD */
  if (kw(&L->cur,"SBINOM")||kw(&L->cur,"SCHOOSE")||kw(&L->cur,"STACKBINOM")||
      kw(&L->cur,"SPERM")||kw(&L->cur,"SPNR")||kw(&L->cur,"STACKPERM")){
    /* n k → C(n,k) or P(n,k) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long k = vm->stack[--vm->sp];
    long n = vm->stack[--vm->sp];
    long r = 0;
    if (n < 0 || k < 0 || k > n) r = 0;
    else if (strcmp(op,"SPERM")==0 || strcmp(op,"SPNR")==0 || strcmp(op,"STACKPERM")==0){
      r = 1;
      for (long i = 0; i < k; i++) r *= (n - i);
    } else {
      /* SBINOM / SCHOOSE / STACKBINOM */
      long kk = k;
      if (kk > n - kk) kk = n - kk;
      r = 1;
      for (long i = 1; i <= kk; i++)
        r = r * (n - kk + i) / i;
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SADDMOD")||kw(&L->cur,"STACKADDMOD")||
      kw(&L->cur,"SSUBMOD")||kw(&L->cur,"STACKSUBMOD")){
    /* a b m → (a±b) mod m */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (m <= 0) r = 0;
    else {
      long x = a % m; if (x < 0) x += m;
      long y = b % m; if (y < 0) y += m;
      if (strcmp(op,"SADDMOD")==0 || strcmp(op,"STACKADDMOD")==0)
        r = (x + y) % m;
      else
        r = (x - y + m) % m;
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3/0 stack pack+byte: SPACK16 SHI16 SLO16 SBYTE SLOBYTE SHIBYTE */
  if (kw(&L->cur,"SLOBYTE")||kw(&L->cur,"STACKLOBYTE")||
      kw(&L->cur,"SHIBYTE")||kw(&L->cur,"STACKHIBYTE")||
      kw(&L->cur,"SHI16")||kw(&L->cur,"SHIWORD")||kw(&L->cur,"STACKHI16")||
      kw(&L->cur,"SLO16")||kw(&L->cur,"SLOWORD")||kw(&L->cur,"STACKLO16")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (strcmp(op,"SLOBYTE")==0 || strcmp(op,"STACKLOBYTE")==0)
      r = (long)((unsigned long)a & 0xFFul);
    else if (strcmp(op,"SHIBYTE")==0 || strcmp(op,"STACKHIBYTE")==0)
      r = (long)(((unsigned long)a >> 8) & 0xFFul);
    else if (strcmp(op,"SHI16")==0 || strcmp(op,"SHIWORD")==0 || strcmp(op,"STACKHI16")==0)
      r = (long)(((unsigned int)a >> 16) & 0xFFFFu);
    else
      r = (long)((unsigned int)a & 0xFFFFu);
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SBYTE")||kw(&L->cur,"STACKBYTE")||kw(&L->cur,"SGETBYTE")){
    /* a i → i-th little-endian byte of a (i clamped 0..7) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i > 7) i = 7;
    long r = (long)(((unsigned long)a >> (unsigned)(i * 8)) & 0xFFul);
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 SETBYTE + ALIGN: SSETBYTE SALIGN SALIGNDN */
  if (kw(&L->cur,"SSETBYTE")||kw(&L->cur,"STACKSETBYTE")||kw(&L->cur,"SSETBY")){
    /* a field i → deposit 8-bit at byte index i (SSETB reserved for bit set) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    long field = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i > 7) i = 7;
    unsigned long base = (unsigned long)a;
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long shift = (unsigned long)(i * 8);
    long r = (long)((base & ~(0xFFul << shift)) | (f << shift));
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SALIGN")||kw(&L->cur,"SROUNDUP")||kw(&L->cur,"STACKALIGN")||
      kw(&L->cur,"SALIGNDN")||kw(&L->cur,"SROUNDDN")||kw(&L->cur,"STACKALIGNDN")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long al = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = a;
    int is_dn = (strcmp(op,"SALIGNDN")==0 || strcmp(op,"SROUNDDN")==0 ||
                 strcmp(op,"STACKALIGNDN")==0);
    if (al > 0){
      long q = a / al, rem = a % al;
      if (rem != 0){
        if (is_dn) r = (a > 0) ? q * al : (q - 1) * al;
        else r = (a > 0) ? (q + 1) * al : q * al;
      }
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPACK16")||kw(&L->cur,"SPACK")||kw(&L->cur,"STACKPACK")||
      kw(&L->cur,"STACKPACK16")){
    /* hi lo → (hi<<16)|lo  (16-bit halves) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long lo = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    unsigned int h = (unsigned int)hi & 0xFFFFu;
    unsigned int l = (unsigned int)lo & 0xFFFFu;
    long r = (long)((h << 16) | l);
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 pack8/nibble + set nibble: SPACK8 SPACKNIB SSETNIB */
  if (kw(&L->cur,"SPACK8")||kw(&L->cur,"SPACKB")||kw(&L->cur,"STACKPACK8")||
      kw(&L->cur,"SPACKNIB")||kw(&L->cur,"SPACK4")||kw(&L->cur,"STACKPACKNIB")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long lo = vm->stack[--vm->sp];
    long hi = vm->stack[--vm->sp];
    long r;
    if (strcmp(op,"SPACKNIB")==0 || strcmp(op,"SPACK4")==0 || strcmp(op,"STACKPACKNIB")==0)
      r = (long)((((unsigned int)hi & 0xFu) << 4) | ((unsigned int)lo & 0xFu));
    else
      r = (long)((((unsigned int)hi & 0xFFu) << 8) | ((unsigned int)lo & 0xFFu));
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSETNIB")||kw(&L->cur,"SSETNIBBLE")||kw(&L->cur,"STACKSETNIB")){
    /* a field i → deposit 4-bit field at nibble index i */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    long field = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i > 15) i = 15;
    unsigned long base = (unsigned long)a;
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long shift = (unsigned long)(i * 4);
    long r = (long)((base & ~(0xFul << shift)) | (f << shift));
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 word data path: SBSWAP SBITREV SPARITY SNIBBLE */
  if (kw(&L->cur,"SBSWAP")||kw(&L->cur,"SBSWAP32")||kw(&L->cur,"STACKBSWAP")||
      kw(&L->cur,"SBITREV")||kw(&L->cur,"SREVBITS")||kw(&L->cur,"STACKBITREV")||
      kw(&L->cur,"SPARITY")||kw(&L->cur,"STACKPARITY")||kw(&L->cur,"SPAR")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (strcmp(op,"SBSWAP")==0 || strcmp(op,"SBSWAP32")==0 || strcmp(op,"STACKBSWAP")==0){
      unsigned int w = (unsigned int)a;
      w = ((w & 0x000000FFu) << 24) | ((w & 0x0000FF00u) << 8) |
          ((w & 0x00FF0000u) >> 8) | ((w & 0xFF000000u) >> 24);
      r = (long)w;
    } else if (strcmp(op,"SBITREV")==0 || strcmp(op,"SREVBITS")==0 ||
               strcmp(op,"STACKBITREV")==0){
      unsigned int w = (unsigned int)a;
      unsigned int rv = 0;
      for (int i = 0; i < 32; i++){
        rv = (rv << 1) | (w & 1u);
        w >>= 1;
      }
      r = (long)rv;
    } else {
      /* SPARITY / STACKPARITY / SPAR */
      unsigned long u = (unsigned long)a;
      int n = 0;
      while (u){ n ^= (int)(u & 1u); u >>= 1; }
      r = (long)n;
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNIB")||kw(&L->cur,"SNIBBLE")||kw(&L->cur,"STACKNIBBLE")||
      kw(&L->cur,"SGETNIB")){
    /* a i → i-th little-endian nibble of a (i clamped 0..15) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i > 15) i = 15;
    long r = (long)(((unsigned long)a >> (unsigned)(i * 4)) & 0xFul);
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 sign/zero extend: SSEXT SZEXT SSEXT8 SSEXT16 */
  if (kw(&L->cur,"SSEXT8")||kw(&L->cur,"SSEXTB")||kw(&L->cur,"STACKSEXT8")||
      kw(&L->cur,"SSEXT16")||kw(&L->cur,"SSEXTW")||kw(&L->cur,"STACKSEXT16")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r;
    if (strcmp(op,"SSEXT16")==0 || strcmp(op,"SSEXTW")==0 || strcmp(op,"STACKSEXT16")==0){
      r = a & 0xFFFFL;
      if (r & 0x8000L) r |= ~0xFFFFL;
    } else {
      r = a & 0xFFL;
      if (r & 0x80L) r |= ~0xFFL;
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEXT")||kw(&L->cur,"STACKSEXT")||kw(&L->cur,"SSIGNEXT")||
      kw(&L->cur,"SZEXT")||kw(&L->cur,"STACKZEXT")||kw(&L->cur,"SZEROEXT")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long w = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    int is_z = (strcmp(op,"SZEXT")==0 || strcmp(op,"STACKZEXT")==0 ||
                strcmp(op,"SZEROEXT")==0);
    if (w <= 0) r = 0;
    else if (w >= 63) r = a;
    else {
      unsigned long mask = (1ul << (unsigned)w) - 1ul;
      unsigned long v = (unsigned long)a & mask;
      if (!is_z){
        unsigned long sign = 1ul << (unsigned)(w - 1);
        if (v & sign) v |= ~mask;
      }
      r = (long)v;
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack science/math duals: SAVG SPCT SHYP SHAM SDIST */
  if (kw(&L->cur,"SAVG")||kw(&L->cur,"STACKAVG")||
      kw(&L->cur,"SPCT")||kw(&L->cur,"STACKPCT")||kw(&L->cur,"SPERCENT")||
      kw(&L->cur,"SHYP")||kw(&L->cur,"STACKHYP")||kw(&L->cur,"SHYPOT")||
      kw(&L->cur,"SHAM")||kw(&L->cur,"SHAMMING")||kw(&L->cur,"STACKHAMMING")||
      kw(&L->cur,"SDIST")||kw(&L->cur,"SABSDIFF")||kw(&L->cur,"STACKDIST")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (strcmp(op,"SAVG")==0 || strcmp(op,"STACKAVG")==0)
      r = (a + b) / 2;
    else if (strcmp(op,"SPCT")==0 || strcmp(op,"STACKPCT")==0 || strcmp(op,"SPERCENT")==0)
      r = b ? (a * 100 / b) : 0;
    else if (strcmp(op,"SHYP")==0 || strcmp(op,"STACKHYP")==0 || strcmp(op,"SHYPOT")==0){
      long s = a * a + b * b;
      if (s < 0) r = 0;
      else {
        long t = 0;
        while ((t + 1) * (t + 1) <= s) t++;
        r = t;
      }
    } else if (strcmp(op,"SHAM")==0 || strcmp(op,"SHAMMING")==0 ||
               strcmp(op,"STACKHAMMING")==0){
      unsigned long u = (unsigned long)(a ^ b);
      int n = 0;
      while (u){ n += (int)(u & 1u); u >>= 1; }
      r = (long)n;
    } else {
      /* SDIST / SABSDIFF / STACKDIST — |a-b| */
      long d = a - b;
      r = d < 0 ? -d : d;
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 foundation stack: SMASK SISDIV */
  if (kw(&L->cur,"SMASK")||kw(&L->cur,"SBITMASK")||kw(&L->cur,"STACKMASK")){
    /* n → low n bits set */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r;
    if (a <= 0) r = 0;
    else if (a >= 63) r = (long)~0ul;
    else r = (long)((1ul << (unsigned)a) - 1ul);
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SISDIV")||kw(&L->cur,"SDIVISIBLE")||kw(&L->cur,"STACKISDIV")){
    /* a b → 1 if b!=0 and a multiple of b */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = (b != 0 && (a % b) == 0) ? 1 : 0;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 stack bitwise ALU: SAND SOR SXOR SNOT SSHL SSHR */
  if (kw(&L->cur,"SAND")||kw(&L->cur,"STACKAND")||kw(&L->cur,"BANDST")||
      kw(&L->cur,"SOR")||kw(&L->cur,"STACKOR")||kw(&L->cur,"BORST")||
      kw(&L->cur,"SXOR")||kw(&L->cur,"STACKXOR")||kw(&L->cur,"BXORST")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (strcmp(op,"SAND")==0 || strcmp(op,"STACKAND")==0 || strcmp(op,"BANDST")==0)
      r = a & b;
    else if (strcmp(op,"SOR")==0 || strcmp(op,"STACKOR")==0 || strcmp(op,"BORST")==0)
      r = a | b;
    else if (strcmp(op,"SXOR")==0 || strcmp(op,"STACKXOR")==0 || strcmp(op,"BXORST")==0)
      r = a ^ b;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNOT")||kw(&L->cur,"STACKNOT")||kw(&L->cur,"BNOTST")||kw(&L->cur,"SINVERT")){
    /* bitwise invert TOS */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = ~vm->stack[vm->sp - 1];
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSHL")||kw(&L->cur,"STACKSHL")||kw(&L->cur,"SLSHL")||
      kw(&L->cur,"SSHR")||kw(&L->cur,"STACKSHR")||kw(&L->cur,"SLSHR")){
    /* a b → a<<b or a>>b (logical-ish on unsigned cast for SHR) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (b < 0) b = 0;
    if (b > 63) b = 63;
    if (strcmp(op,"SSHL")==0 || strcmp(op,"STACKSHL")==0 || strcmp(op,"SLSHL")==0)
      r = (long)((unsigned long)a << (unsigned)b);
    else
      r = (long)((unsigned long)a >> (unsigned)b);
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 stack bit metrics + rotate/SAR: SPOPCNT SCLZ SCTZ SROL SROR SSAR */
  if (kw(&L->cur,"SPOPCNT")||kw(&L->cur,"SPCOUNT")||kw(&L->cur,"STACKPOPCNT")||
      kw(&L->cur,"SPOPCOUNT")||kw(&L->cur,"SPCNT")||
      kw(&L->cur,"SCLZ")||kw(&L->cur,"STACKCLZ")||
      kw(&L->cur,"SCTZ")||kw(&L->cur,"STACKCTZ")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    unsigned long u = (unsigned long)a;
    long r = 0;
    if (strcmp(op,"SPOPCNT")==0 || strcmp(op,"SPCOUNT")==0 ||
        strcmp(op,"STACKPOPCNT")==0 || strcmp(op,"SPOPCOUNT")==0 ||
        strcmp(op,"SPCNT")==0){
      while (u){ r += (long)(u & 1ul); u >>= 1; }
    } else if (strcmp(op,"SCLZ")==0 || strcmp(op,"STACKCLZ")==0){
      if (u == 0) r = 64;
      else {
        for (int i = 63; i >= 0; i--){
          if (u & (1ul << i)) break;
          r++;
        }
      }
    } else {
      /* SCTZ / STACKCTZ */
      if (u == 0) r = 64;
      else {
        while ((u & 1ul) == 0){ r++; u >>= 1; }
      }
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROL")||kw(&L->cur,"STACKROL")||kw(&L->cur,"SROTL")||
      kw(&L->cur,"SROR")||kw(&L->cur,"STACKROR")||kw(&L->cur,"SROTR")||
      kw(&L->cur,"SSAR")||kw(&L->cur,"STACKSAR")||kw(&L->cur,"SASHR")){
    /* a k → rotate/arithmetic-shift a by k (k mod 64; k<0 → 0) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long k = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (k < 0) k = 0;
    unsigned uk = (unsigned)(k & 63);
    if (strcmp(op,"SSAR")==0 || strcmp(op,"STACKSAR")==0 || strcmp(op,"SASHR")==0){
      /* arithmetic right shift (sign-preserving) */
      if (k > 63) k = 63;
      r = a >> k;
    } else if (strcmp(op,"SROL")==0 || strcmp(op,"STACKROL")==0 || strcmp(op,"SROTL")==0){
      unsigned long u = (unsigned long)a;
      if (uk == 0) r = a;
      else r = (long)((u << uk) | (u >> (64u - uk)));
    } else {
      /* SROR / STACKROR / SROTR */
      unsigned long u = (unsigned long)a;
      if (uk == 0) r = a;
      else r = (long)((u >> uk) | (u << (64u - uk)));
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack bitfield data path: SBTEST SSETB SCLRB SFLPB SBEXT SBDEP */
  if (kw(&L->cur,"SBTEST")||kw(&L->cur,"SBITT")||kw(&L->cur,"STACKBIT")||
      kw(&L->cur,"SBTST")||kw(&L->cur,"TESTBIT")||
      kw(&L->cur,"SSETB")||kw(&L->cur,"SSETBIT")||kw(&L->cur,"STACKSETB")||
      kw(&L->cur,"SCLRB")||kw(&L->cur,"SCLRBIT")||kw(&L->cur,"STACKCLRB")||
      kw(&L->cur,"SFLPB")||kw(&L->cur,"SFLIPB")||kw(&L->cur,"STGLB")||
      kw(&L->cur,"STGLBIT")||kw(&L->cur,"STACKFLIPB")){
    /* a k → test/set/clear/flip bit k of a (k clamped 0..63) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long k = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (k < 0) k = 0;
    if (k > 63) k = 63;
    unsigned long uk = (unsigned long)k;
    unsigned long bit = 1ul << uk;
    unsigned long u = (unsigned long)a;
    long r = 0;
    if (strcmp(op,"SBTEST")==0 || strcmp(op,"SBITT")==0 || strcmp(op,"STACKBIT")==0 ||
        strcmp(op,"SBTST")==0 || strcmp(op,"TESTBIT")==0)
      r = (u & bit) ? 1 : 0;
    else if (strcmp(op,"SSETB")==0 || strcmp(op,"SSETBIT")==0 || strcmp(op,"STACKSETB")==0)
      r = (long)(u | bit);
    else if (strcmp(op,"SCLRB")==0 || strcmp(op,"SCLRBIT")==0 || strcmp(op,"STACKCLRB")==0)
      r = (long)(u & ~bit);
    else
      /* SFLPB / SFLIPB / STGLB / STGLBIT / STACKFLIPB */
      r = (long)(u ^ bit);
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SBEXT")||kw(&L->cur,"STACKBEXT")||kw(&L->cur,"SEXTR")||
      kw(&L->cur,"SBITEXT")){
    /* a pos width → extract width bits at pos (dual of BEXT) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long width = vm->stack[--vm->sp];
    long pos = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (pos < 0) pos = 0;
    if (pos > 62){ vm->stack[vm->sp++] = 0; var_set_num(vm,"LAST_N",0); vm->last_n=0;
      var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1; }
    if (width < 1) width = 0;
    if (width > 63 - pos) width = 63 - pos;
    long r = 0;
    if (width > 0){
      unsigned long mask = (width >= 63) ? ~0ul : ((1ul << (unsigned)width) - 1ul);
      r = (long)(((unsigned long)a >> (unsigned)pos) & mask);
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SBDEP")||kw(&L->cur,"STACKBDEP")||kw(&L->cur,"SDEP")||
      kw(&L->cur,"SBITDEP")){
    /* base field pos → deposit low 8 bits of field at pos (dual of BDEP) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long pos = vm->stack[--vm->sp];
    long field = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long width = 8;
    if (pos < 0) pos = 0;
    if (pos > 62){
      vm->stack[vm->sp++] = a;
      var_set_num(vm,"LAST_N",a); vm->last_n=a;
      var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
    }
    if (width > 63 - pos) width = 63 - pos;
    long r = a;
    if (width > 0){
      unsigned long mask = (width >= 63) ? ~0ul : ((1ul << (unsigned)width) - 1ul);
      unsigned long base = (unsigned long)a;
      unsigned long f = (unsigned long)field & mask;
      base = (base & ~(mask << (unsigned)pos)) | (f << (unsigned)pos);
      r = (long)base;
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack compare + min/max: predicate 0/1 or ordered select */
  if (kw(&L->cur,"SMIN")||kw(&L->cur,"SMAX")||
      kw(&L->cur,"STACKMIN")||kw(&L->cur,"STACKMAX")||
      kw(&L->cur,"SEQ")||kw(&L->cur,"SNE")||kw(&L->cur,"SLT")||
      kw(&L->cur,"SGT")||kw(&L->cur,"SLE")||kw(&L->cur,"SGE")||
      kw(&L->cur,"STACKEQ")||kw(&L->cur,"STACKNE")||kw(&L->cur,"STACKLT")||
      kw(&L->cur,"STACKGT")||kw(&L->cur,"STACKLE")||kw(&L->cur,"STACKGE")||
      kw(&L->cur,"CMPEQ")||kw(&L->cur,"CMPNE")||kw(&L->cur,"CMPLT")||
      kw(&L->cur,"CMPGT")||kw(&L->cur,"CMPLE")||kw(&L->cur,"CMPGE")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (strcmp(op,"SMIN")==0 || strcmp(op,"STACKMIN")==0) r = a < b ? a : b;
    else if (strcmp(op,"SMAX")==0 || strcmp(op,"STACKMAX")==0) r = a > b ? a : b;
    else if (strcmp(op,"SEQ")==0 || strcmp(op,"STACKEQ")==0 || strcmp(op,"CMPEQ")==0)
      r = (a == b) ? 1 : 0;
    else if (strcmp(op,"SNE")==0 || strcmp(op,"STACKNE")==0 || strcmp(op,"CMPNE")==0)
      r = (a != b) ? 1 : 0;
    else if (strcmp(op,"SLT")==0 || strcmp(op,"STACKLT")==0 || strcmp(op,"CMPLT")==0)
      r = (a < b) ? 1 : 0;
    else if (strcmp(op,"SGT")==0 || strcmp(op,"STACKGT")==0 || strcmp(op,"CMPGT")==0)
      r = (a > b) ? 1 : 0;
    else if (strcmp(op,"SLE")==0 || strcmp(op,"STACKLE")==0 || strcmp(op,"CMPLE")==0)
      r = (a <= b) ? 1 : 0;
    else if (strcmp(op,"SGE")==0 || strcmp(op,"STACKGE")==0 || strcmp(op,"CMPGE")==0)
      r = (a >= b) ? 1 : 0;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack select/within/clamp + zero-tests/sign */
  if (kw(&L->cur,"SZ")||kw(&L->cur,"S0EQ")||kw(&L->cur,"STACK0EQ")||kw(&L->cur,"S0=")||
      kw(&L->cur,"SNZ")||kw(&L->cur,"S0NE")||kw(&L->cur,"STACK0NE")||kw(&L->cur,"S0<>")||
      kw(&L->cur,"S0LT")||kw(&L->cur,"STACK0LT")||kw(&L->cur,"S0<")||
      kw(&L->cur,"S0GT")||kw(&L->cur,"STACK0GT")||kw(&L->cur,"S0>")||
      kw(&L->cur,"SSIGN")||kw(&L->cur,"STACKSIGN")||kw(&L->cur,"SGN")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (strcmp(op,"SZ")==0 || strcmp(op,"S0EQ")==0 || strcmp(op,"STACK0EQ")==0 || strcmp(op,"S0=")==0)
      r = (a == 0) ? 1 : 0;
    else if (strcmp(op,"SNZ")==0 || strcmp(op,"S0NE")==0 || strcmp(op,"STACK0NE")==0 || strcmp(op,"S0<>")==0)
      r = (a != 0) ? 1 : 0;
    else if (strcmp(op,"S0LT")==0 || strcmp(op,"STACK0LT")==0 || strcmp(op,"S0<")==0)
      r = (a < 0) ? 1 : 0;
    else if (strcmp(op,"S0GT")==0 || strcmp(op,"STACK0GT")==0 || strcmp(op,"S0>")==0)
      r = (a > 0) ? 1 : 0;
    else {
      /* SSIGN / STACKSIGN / SGN */
      r = (a > 0) ? 1 : ((a < 0) ? -1 : 0);
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEL")||kw(&L->cur,"STACKSEL")||kw(&L->cur,"SSELECT")||kw(&L->cur,"STACKSELECT")){
    /* f t cond → (cond ? t : f)  — cond on TOS */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long cond = vm->stack[--vm->sp];
    long t = vm->stack[--vm->sp];
    long f = vm->stack[--vm->sp];
    long r = cond ? t : f;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SWITHIN")||kw(&L->cur,"WITHIN")||kw(&L->cur,"STACKWITHIN")){
    /* n lo hi → 1 if lo <= n < hi else 0 (Forth WITHIN) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    long n = vm->stack[--vm->sp];
    long r = (n >= lo && n < hi) ? 1 : 0;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLAMP")||kw(&L->cur,"STACKCLAMP")||kw(&L->cur,"SCLMP")){
    /* n lo hi → clamp n into [lo,hi] (if lo>hi swap) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    long n = vm->stack[--vm->sp];
    if (lo > hi){ long t=lo; lo=hi; hi=t; }
    long r = n;
    if (r < lo) r = lo;
    if (r > hi) r = hi;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack fold/reduce: SSUM SPROD SFAND SFOR SFXOR FOLDMIN FOLDMAX
   * SMEAN SCOUNTNZ — optional trailing n = fold only top n; omit n → whole stack.
   * empty identities: sum/or/xor/countnz=0, prod=1, and=-1; min/max/mean empty → OK=0. */
  if (kw(&L->cur,"SSUM")||kw(&L->cur,"STACKSUM")||kw(&L->cur,"SFOLDADD")||
      kw(&L->cur,"SPROD")||kw(&L->cur,"STACKPROD")||kw(&L->cur,"SFOLDMUL")||
      kw(&L->cur,"SFAND")||kw(&L->cur,"FOLDAND")||kw(&L->cur,"SFOLDAND")||
      kw(&L->cur,"SFOR")||kw(&L->cur,"FOLDOR")||kw(&L->cur,"SFOLDOR")||
      kw(&L->cur,"SFXOR")||kw(&L->cur,"FOLDXOR")||kw(&L->cur,"SFOLDXOR")||
      kw(&L->cur,"FOLDMIN")||kw(&L->cur,"SFOLDMIN")||
      kw(&L->cur,"FOLDMAX")||kw(&L->cur,"SFOLDMAX")||
      kw(&L->cur,"SMEAN")||kw(&L->cur,"SFOLDAVG")||kw(&L->cur,"STACKMEAN")||
      kw(&L->cur,"SAVGALL")||
      kw(&L->cur,"SCOUNTNZ")||kw(&L->cur,"SCNTNZ")||kw(&L->cur,"SFOLDCOUNTNZ")||
      kw(&L->cur,"STACKCOUNTNZ")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    int have_n = 0;
    long n = 0;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE") &&
         !kw(&L->cur,"PUSH") && !kw(&L->cur,"POP") && !kw(&L->cur,"CLEARSTACK") &&
         !kw(&L->cur,"PEEK") && !kw(&L->cur,"DROP") && !kw(&L->cur,"DUP"))){
      n = parse_expr(vm,L);
      have_n = 1;
    }
    if (!have_n) n = (long)vm->sp;
    if (n < 0) n = 0;
    if (have_n && n > (long)vm->sp){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    int is_min = (strcmp(op,"FOLDMIN")==0 || strcmp(op,"SFOLDMIN")==0);
    int is_max = (strcmp(op,"FOLDMAX")==0 || strcmp(op,"SFOLDMAX")==0);
    int is_sum = (strcmp(op,"SSUM")==0 || strcmp(op,"STACKSUM")==0 || strcmp(op,"SFOLDADD")==0);
    int is_prod = (strcmp(op,"SPROD")==0 || strcmp(op,"STACKPROD")==0 || strcmp(op,"SFOLDMUL")==0);
    int is_and = (strcmp(op,"SFAND")==0 || strcmp(op,"FOLDAND")==0 || strcmp(op,"SFOLDAND")==0);
    int is_or = (strcmp(op,"SFOR")==0 || strcmp(op,"FOLDOR")==0 || strcmp(op,"SFOLDOR")==0);
    int is_xor = (strcmp(op,"SFXOR")==0 || strcmp(op,"FOLDXOR")==0 || strcmp(op,"SFOLDXOR")==0);
    int is_mean = (strcmp(op,"SMEAN")==0 || strcmp(op,"SFOLDAVG")==0 ||
                   strcmp(op,"STACKMEAN")==0 || strcmp(op,"SAVGALL")==0);
    int is_cntnz = (strcmp(op,"SCOUNTNZ")==0 || strcmp(op,"SCNTNZ")==0 ||
                    strcmp(op,"SFOLDCOUNTNZ")==0 || strcmp(op,"STACKCOUNTNZ")==0);
    long r = 0;
    if (n == 0){
      if (is_min || is_max || is_mean){ var_set_num(vm,"OK",0); bump(vm); return 1; }
      if (is_sum || is_or || is_xor || is_cntnz) r = 0;
      else if (is_prod) r = 1;
      else if (is_and) r = -1;
      else r = 0;
      if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
      vm->stack[vm->sp++] = r;
      var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
      var_set_num(vm,"OK",1); bump(vm); return 1;
    }
    /* left-fold top n items (bottom of window first) */
    int base = vm->sp - (int)n;
    if (is_mean){
      long acc = 0;
      for (int i = 0; i < (int)n; i++) acc += vm->stack[base + i];
      r = acc / n;
    } else if (is_cntnz){
      long c = 0;
      for (int i = 0; i < (int)n; i++)
        if (vm->stack[base + i] != 0) c++;
      r = c;
    } else {
      r = vm->stack[base];
      for (int i = 1; i < (int)n; i++){
        long v = vm->stack[base + i];
        if (is_sum) r = r + v;
        else if (is_prod) r = r * v;
        else if (is_and) r = r & v;
        else if (is_or) r = r | v;
        else if (is_xor) r = r ^ v;
        else if (is_min) r = (r < v) ? r : v;
        else if (is_max) r = (r > v) ? r : v;
      }
    }
    vm->sp = base;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack↔cell bridge: TOCELL / FROMCELL */
  if (kw(&L->cur,"TOCELL")||kw(&L->cur,"STACKTOCELL")||kw(&L->cur,">CELL")){
    /* TOCELL dst [n] — pop n values into cells[dst..dst+n-1] (TOS → highest index) */
    lex_next(L);
    long dst = parse_expr(vm,L);
    long n = 1;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE")))
      n = parse_expr(vm,L);
    if (n < 1) n = 1;
    if (dst < 0) dst = 0;
    if (dst >= CUBALC_CELL_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (dst + n > CUBALC_CELL_N) n = CUBALC_CELL_N - dst;
    if (vm->sp < n){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    /* pop n times: first pop goes to dst+n-1 (TOS last index) */
    for (long i = n - 1; i >= 0; i--){
      long v = vm->stack[--vm->sp];
      vm->cells[(int)(dst + i)] = v;
    }
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"FROMCELL")||kw(&L->cur,"CELLTOSTACK")||kw(&L->cur,"CELL>")||
      kw(&L->cur,"PUSHCELL")){
    /* FROMCELL src [n] — push cells[src..src+n-1] onto stack (src first, then up) */
    lex_next(L);
    long src = parse_expr(vm,L);
    long n = 1;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE")))
      n = parse_expr(vm,L);
    if (n < 1) n = 1;
    if (src < 0) src = 0;
    if (src >= CUBALC_CELL_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (src + n > CUBALC_CELL_N) n = CUBALC_CELL_N - src;
    if (vm->sp + n > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long last = 0;
    for (long i = 0; i < n; i++){
      last = vm->cells[(int)(src + i)];
      vm->stack[vm->sp++] = last;
    }
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"FILLCELL")||kw(&L->cur,"CELLFILL")||kw(&L->cur,"FILL")){
    /* FILLCELL lo hi val — fill cell[lo..hi] with val */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long val = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    for (long i = lo; i <= hi; i++) vm->cells[(int)i] = val;
    var_set_num(vm,"LAST_N",val); vm->last_n=val;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 data/memory plane: block copy/move + search + reverse */
  if (kw(&L->cur,"COPYCELL")||kw(&L->cur,"CELLCOPY")||kw(&L->cur,"CMOVE")){
    /* COPYCELL src dst n — copy n cells src.. → dst.. (overlap-safe) */
    lex_next(L);
    long src = parse_expr(vm,L);
    long dst = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (src < 0) src = 0;
    if (dst < 0) dst = 0;
    if (src >= CUBALC_CELL_N || dst >= CUBALC_CELL_N || n == 0){
      var_set_num(vm,"OK", n==0 ? 1 : 0);
      var_set_num(vm,"LAST_N",0); vm->last_n=0;
      bump(vm); return 1;
    }
    if (src + n > CUBALC_CELL_N) n = CUBALC_CELL_N - src;
    if (dst + n > CUBALC_CELL_N) n = CUBALC_CELL_N - dst;
    if (n > 0){
      long tmp[CUBALC_CELL_N];
      for (long i=0;i<n;i++) tmp[i] = vm->cells[(int)(src+i)];
      for (long i=0;i<n;i++) vm->cells[(int)(dst+i)] = tmp[i];
    }
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"MOVECELL")||kw(&L->cur,"CELLMOVE")){
    /* MOVECELL src dst n — copy then zero source (non-overlapping src clear) */
    lex_next(L);
    long src = parse_expr(vm,L);
    long dst = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (src < 0) src = 0;
    if (dst < 0) dst = 0;
    if (src >= CUBALC_CELL_N || dst >= CUBALC_CELL_N || n == 0){
      var_set_num(vm,"OK", n==0 ? 1 : 0);
      var_set_num(vm,"LAST_N",0); vm->last_n=0;
      bump(vm); return 1;
    }
    if (src + n > CUBALC_CELL_N) n = CUBALC_CELL_N - src;
    if (dst + n > CUBALC_CELL_N) n = CUBALC_CELL_N - dst;
    if (n > 0){
      long tmp[CUBALC_CELL_N];
      for (long i=0;i<n;i++) tmp[i] = vm->cells[(int)(src+i)];
      for (long i=0;i<n;i++) vm->cells[(int)(dst+i)] = tmp[i];
      /* clear source cells that are not inside the destination range */
      for (long i=0;i<n;i++){
        long si = src + i;
        if (si < dst || si >= dst + n)
          vm->cells[(int)si] = 0;
      }
    }
    var_set_num(vm,"LAST_N",n); vm->last_n=n;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"FINDCELL")||kw(&L->cur,"CELLFIND")||kw(&L->cur,"INDEXCELL")){
    /* FINDCELL val [lo [hi]] — first index of val, or -1; OK=found */
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
    for (long i=lo;i<=hi;i++){
      if (vm->cells[(int)i] == val){ found = i; break; }
    }
    var_set_num(vm,"LAST_N",found); vm->last_n=found;
    var_set_num(vm,"OK", found >= 0 ? 1 : 0);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"COUNTCELL")||kw(&L->cur,"CELLCOUNT")||kw(&L->cur,"COUNTVAL")){
    /* COUNTCELL val [lo [hi]] — how many cells equal val */
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
    long cnt = 0;
    for (long i=lo;i<=hi;i++)
      if (vm->cells[(int)i] == val) cnt++;
    var_set_num(vm,"LAST_N",cnt); vm->last_n=cnt;
    var_set_num(vm,"OK",1); bump(vm); return 1;
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
  /* COP matrix algebra (digit-5): CLEARBITS|NOTBITS|COPYBITS|ANDBITS|ORBITS|XORBITS */
  if (kw(&L->cur,"CLEARBITS")||kw(&L->cur,"ZEROBITS")||kw(&L->cur,"CLRBITS")||
      kw(&L->cur,"NOTBITS")||kw(&L->cur,"INVERTBITS")||kw(&L->cur,"FLIPBITS")||
      kw(&L->cur,"COPYBITS")||kw(&L->cur,"CLONEBITS")||
      kw(&L->cur,"ANDBITS")||kw(&L->cur,"ORBITS")||kw(&L->cur,"XORBITS")||
      kw(&L->cur,"NANDBITS")||kw(&L->cur,"FILLBITS")){
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
  /* INCLUDE "path.cubalc" — practical modules (same VM / world) */
  if (kw(&L->cur,"INCLUDE")||kw(&L->cur,"IMPORT")||kw(&L->cur,"USE")){
    lex_next(L);
    if (L->cur.kind!=TK_STR){ fail(vm,"INCLUDE \"path.cubalc\""); return -1; }
    char path[768];
    if (L->cur.text[0]=='/' || (vm->include_base[0]==0))
      snprintf(path,sizeof path,"%s",L->cur.text);
    else
      snprintf(path,sizeof path,"%s/%s", vm->include_base, L->cur.text);
    lex_next(L);
    FILE *f=fopen(path,"rb");
    if (!f){
      const char *root=getenv("CUBALC_ROOT");
      if (root && root[0]) {
        char p2[768];
        snprintf(p2, sizeof p2, "%s/%s", root, path);
        f = fopen(p2, "rb");
        if (f) snprintf(path, sizeof path, "%s", p2);
      }
    }
    if (!f){
      char p3[768];
      snprintf(p3, sizeof p3, "programs/%s", path);
      f = fopen(p3, "rb");
      if (f) snprintf(path, sizeof path, "%s", p3);
    }
    if (!f){ snprintf(vm->err,sizeof vm->err,"INCLUDE cannot open %s", path); fail(vm,vm->err); return -1; }
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
    Lex Li; lex_init(&Li, buf, nr);
    int rc = exec_stmts_until(vm, &Li, NULL, NULL);
    snprintf(vm->include_base,sizeof vm->include_base,"%s", save_base);
    free(buf);
    if (rc<0) return -1;
    bump(vm); return 1;
  }
  /* FN name ... END — reusable practical blocks */
  if (kw(&L->cur,"FN")||kw(&L->cur,"FUNC")||kw(&L->cur,"FUNCTION")||kw(&L->cur,"DEF")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"FN name"); return -1; }
    char fname[48]; snprintf(fname,sizeof fname,"%s",L->cur.text); lex_next(L);
    skip_nl(L);
    /* capture body from first body token start until matching END */
    size_t b0 = L->tok_off;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"FN without END"); return -1; }
    /* L parked on END; body ends at END's tok_off */
    size_t b1 = L->tok_off;
    if (b1 < b0) b1 = b0;
    size_t blen = b1 - b0;
    if (vm->n_fns >= 32){ fail(vm,"too many FN"); return -1; }
    FnDef *fn = &vm->fns[vm->n_fns++];
    snprintf(fn->name, sizeof fn->name, "%s", fname);
    fn->body = L->s + b0;
    fn->len = blen;
    if (kw(&L->cur,"END")) lex_next(L);
    if (vm->trace) fprintf(vm->trace, "# FN %s len=%zu\n", fname, blen);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"CALL")||kw(&L->cur,"RUNFN")||kw(&L->cur,"DO")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"CALL name"); return -1; }
    char fname[48]; snprintf(fname,sizeof fname,"%s",L->cur.text); lex_next(L);
    FnDef *fn=NULL;
    for (int i=0;i<vm->n_fns;i++) if (strcmp(vm->fns[i].name,fname)==0){ fn=&vm->fns[i]; break; }
    if (!fn){ snprintf(vm->err,sizeof vm->err,"CALL unknown FN %s", fname); fail(vm,vm->err); return -1; }
    /* optional args: CALL name a b c → ARG0 ARG1 */
    int ai=0;
    while (ai<8 && (L->cur.kind==TK_NUM||L->cur.kind==TK_IDENT||L->cur.kind==TK_STR||L->cur.kind==TK_MINUS||L->cur.kind==TK_LPAREN)){
      if (L->cur.kind==TK_STR){
        char an[16]; snprintf(an,sizeof an,"ARG%d",ai);
        var_set_str(vm, an, L->cur.text); lex_next(L);
      } else {
        long v=parse_expr(vm,L);
        char an[16]; snprintf(an,sizeof an,"ARG%d",ai);
        var_set_num(vm, an, v);
      }
      ai++;
    }
    var_set_num(vm, "NARGS", ai);
    vm->return_fn = 0;
    Lex fl; lex_init(&fl, fn->body, fn->len);
    if (exec_stmts_until(vm, &fl, "END", NULL)<0) return -1;
    vm->return_fn = 0;
    bump(vm); return 1;
  }
  /* RET [expr] — early return from FN (digit-4 control flow) */
  if (kw(&L->cur,"RET")||kw(&L->cur,"RETURN")){
    lex_next(L);
    /* optional return value when next looks like an expression start */
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        L->cur.kind==TK_STR ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"END") && !kw(&L->cur,"ELSE") &&
         !kw(&L->cur,"FN") && !kw(&L->cur,"CALL") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"ASSERT") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"RET") &&
         !kw(&L->cur,"RETURN") && !kw(&L->cur,"WHEN") && !kw(&L->cur,"DEFAULT") &&
         !kw(&L->cur,"FOR") && !kw(&L->cur,"WHILE") && !kw(&L->cur,"LOOP") &&
         !kw(&L->cur,"IF") && !kw(&L->cur,"BREAK") && !kw(&L->cur,"CONTINUE") &&
         !kw(&L->cur,"CASE") && !kw(&L->cur,"CUBE") && !kw(&L->cur,"SYS"))){
      long v = parse_expr(vm, L);
      var_set_num(vm, "LAST_N", v);
      vm->last_n = v;
      var_set_num(vm, "RETVAL", v);
    }
    vm->return_fn = 1;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* CASE expr ... WHEN n THEN ... [DEFAULT ...] END */
  if (kw(&L->cur,"CASE")||kw(&L->cur,"SWITCH")||kw(&L->cur,"MATCH")){
    lex_next(L);
    long sel = parse_expr(vm, L);
    skip_nl(L);
    int matched = 0;
    int ran = 0;
    for(;;){
      skip_nl(L);
      if (L->cur.kind==TK_EOF){ fail(vm,"CASE without END"); return -1; }
      if (kw(&L->cur,"END")){ lex_next(L); break; }
      if (kw(&L->cur,"WHEN")||kw(&L->cur,"OF")||kw(&L->cur,"CASEIF")){
        lex_next(L);
        long w = parse_expr(vm, L);
        if (kw(&L->cur,"THEN")) lex_next(L);
        skip_nl(L);
        Lex body_start=*L;
        int depth=1;
        while (L->cur.kind!=TK_EOF && depth>0){
          if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
            lex_next(L); if (kw(&L->cur,"IF")) lex_next(L); continue;
          }
          if (kw(&L->cur,"IF")||kw(&L->cur,"LOOP")||kw(&L->cur,"WHILE")||kw(&L->cur,"FOR")||
              kw(&L->cur,"EACH")||kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")||kw(&L->cur,"CASE")) depth++;
          else if ((kw(&L->cur,"WHEN")||kw(&L->cur,"OF")||kw(&L->cur,"DEFAULT")||kw(&L->cur,"ELSE")) && depth==1) break;
          else if (kw(&L->cur,"END")){ depth--; if (depth==0) break; }
          lex_next(L);
        }
        if (!matched && !ran && w == sel){
          matched = 1; ran = 1;
          Lex body=body_start;
          /* arm body: stop before next WHEN/DEFAULT/END (body copy only) */
          while (!vm->fatal){
            skip_nl(&body);
            if (body.cur.kind==TK_EOF) break;
            if (kw(&body.cur,"END")||kw(&body.cur,"WHEN")||kw(&body.cur,"OF")||
                kw(&body.cur,"DEFAULT")||kw(&body.cur,"ELSE")||kw(&body.cur,"CASEIF")) break;
            if (vm->return_fn || vm->break_loop) break;
            int r=parse_form(vm,&body);
            if (r<0) return -1;
            if (r==0) break;
          }
          /* skip remaining arms to END on outer L (parked on next arm or END) */
          depth=1;
          while (L->cur.kind!=TK_EOF && depth>0){
            if (kw(&L->cur,"CASE")||kw(&L->cur,"IF")||kw(&L->cur,"LOOP")||kw(&L->cur,"WHILE")||
                kw(&L->cur,"FOR")||kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")) depth++;
            else if (kw(&L->cur,"END")){ depth--; if(depth==0) break; }
            lex_next(L);
          }
          if (kw(&L->cur,"END")) lex_next(L);
          break;
        }
        continue;
      }
      if (kw(&L->cur,"DEFAULT")||kw(&L->cur,"ELSE")){
        lex_next(L);
        if (kw(&L->cur,"THEN")) lex_next(L);
        skip_nl(L);
        if (!matched && !ran){
          Lex body=*L;
          while (!vm->fatal){
            skip_nl(&body);
            if (body.cur.kind==TK_EOF || kw(&body.cur,"END")) break;
            if (vm->return_fn || vm->break_loop) break;
            int r=parse_form(vm,&body);
            if (r<0) return -1;
            if (r==0) break;
          }
          ran = 1;
        }
        /* always advance outer L to matching END */
        {
          int depth=1;
          while (L->cur.kind!=TK_EOF && depth>0){
            if (kw(&L->cur,"CASE")||kw(&L->cur,"IF")||kw(&L->cur,"LOOP")||kw(&L->cur,"WHILE")||
                kw(&L->cur,"FOR")||kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")) depth++;
            else if (kw(&L->cur,"END")){ depth--; if(depth==0) break; }
            lex_next(L);
          }
        }
        if (kw(&L->cur,"END")) lex_next(L);
        break;
      }
      fail(vm,"CASE expects WHEN|DEFAULT|END"); return -1;
    }
    var_set_num(vm, "MATCHED", matched || ran ? 1 : 0);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* FOR i = a TO b [STEP s] ... END */
  if (kw(&L->cur,"FOR")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"FOR var = a TO b"); return -1; }
    char vname[48]; snprintf(vname,sizeof vname,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_EQ){ fail(vm,"FOR var ="); return -1; }
    lex_next(L);
    long lo=parse_expr(vm,L);
    if (!kw(&L->cur,"TO") && !kw(&L->cur,"..") && !(L->cur.kind==TK_IDENT && strcmp(L->cur.text,"TO")==0)){
      /* allow FOR i = n as 0..n-1 */
      long hi=lo-1; lo=0;
      long step=1;
      skip_nl(L);
      Lex body_start=*L;
      int depth=1;
      while (L->cur.kind!=TK_EOF){
        if (block_scan_step(L, &depth, 0)) break;
      }
      if (depth!=0){ fail(vm,"FOR without END"); return -1; }
      for (long i=lo;i<=hi && !vm->fatal;i+=step){
        var_set_num(vm,vname,i); var_set_num(vm,"IT",i);
        vm->break_loop=0; vm->continue_loop=0;
        Lex body=body_start;
        if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
        if (vm->break_loop){ vm->break_loop=0; break; }
        vm->continue_loop=0;
      }
      if (kw(&L->cur,"END")) lex_next(L);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"TO")||kw(&L->cur,"..")) lex_next(L);
    long hi=parse_expr(vm,L);
    long step=1;
    if (kw(&L->cur,"STEP")||kw(&L->cur,"BY")){ lex_next(L); step=parse_expr(vm,L); if(!step) step=1; }
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"FOR without END"); return -1; }
    if (step>0){
      for (long i=lo;i<=hi && !vm->fatal;i+=step){
        var_set_num(vm,vname,i); var_set_num(vm,"IT",i);
        vm->break_loop=0; vm->continue_loop=0;
        Lex body=body_start;
        if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
        if (vm->break_loop){ vm->break_loop=0; break; }
      }
    } else {
      for (long i=lo;i>=hi && !vm->fatal;i+=step){
        var_set_num(vm,vname,i); var_set_num(vm,"IT",i);
        vm->break_loop=0; vm->continue_loop=0;
        Lex body=body_start;
        if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
        if (vm->break_loop){ vm->break_loop=0; break; }
      }
    }
    if (kw(&L->cur,"END")) lex_next(L);
    bump(vm); return 1;
  }
  /* EACH CUBE as name ... END */
  if (kw(&L->cur,"EACH")||kw(&L->cur,"FOREACH")){
    lex_next(L);
    if (!kw(&L->cur,"CUBE") && !kw(&L->cur,"CUBES")){ fail(vm,"EACH CUBE as name"); return -1; }
    lex_next(L);
    if (kw(&L->cur,"AS")||kw(&L->cur,"->")){ lex_next(L); }
    if (L->cur.kind!=TK_IDENT){ fail(vm,"EACH CUBE as name"); return -1; }
    char cname[48]; snprintf(cname,sizeof cname,"%s",L->cur.text); lex_next(L);
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"EACH without END"); return -1; }
    ensure_world(vm);
    for (int i=0;i<vm->ch.n_cubes && !vm->fatal;i++){
      var_set_str(vm, cname, vm->ch.cubes[i].id);
      var_set_num(vm, "IT", i);
      var_set_num(vm, "DIGIT", vm->ch.cubes[i].atom.digit);
      var_set_num(vm, "ENERGY", (long)lround(vm->ch.cubes[i].atom.energy*100));
      var_set_num(vm, "SET", cubalc_matrix_popcount(&vm->ch.cubes[i].atom.matrix));
      vm->break_loop=0;
      Lex body=body_start;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      if (vm->break_loop){ vm->break_loop=0; break; }
    }
    if (kw(&L->cur,"END")) lex_next(L);
    bump(vm); return 1;
  }
  /* BREAK [IF expr] — leave enclosing loop (digit-4 control flow) */
  if (kw(&L->cur,"BREAK")){
    lex_next(L);
    if (kw(&L->cur,"IF")){
      lex_next(L);
      long c = parse_expr(vm, L);
      if (c) vm->break_loop = 1;
    } else {
      vm->break_loop = 1;
    }
    bump(vm); return 1;
  }
  /* CONTINUE [IF expr] — next loop iteration */
  if (kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
    lex_next(L);
    if (kw(&L->cur,"IF")){
      lex_next(L);
      long c = parse_expr(vm, L);
      if (c) vm->continue_loop = 1;
    } else {
      vm->continue_loop = 1;
    }
    bump(vm); return 1;
  }
  if (kw(&L->cur,"LOOP")){
    lex_next(L);
    long times=parse_expr(vm,L);
    if (times<0) times=0;
    if (times>100000) times=100000;
    skip_nl(L);
    Lex save=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"LOOP without END"); return -1; }
    for (long t=0;t<times && !vm->fatal;t++){
      long *it=var_slot(vm,"IT",1); if (it) *it=t;
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=save;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      if (vm->break_loop){ vm->break_loop=0; break; }
      /* continue_loop: already stopped body via exec_stmts_until */
      vm->continue_loop=0;
    }
    if (kw(&L->cur,"END")) lex_next(L);
    bump(vm); return 1;
  }
  /* REPEAT ... UNTIL cond — post-test loop (digit-4 universal control) */
  if (kw(&L->cur,"REPEAT")){
    lex_next(L);
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 1)) break;
    }
    if (!(kw(&L->cur,"UNTIL") || kw(&L->cur,"END"))){ fail(vm,"REPEAT without UNTIL|END"); return -1; }
    int use_until = kw(&L->cur,"UNTIL") ? 1 : 0;
    if (use_until){
      lex_next(L);
      Lex cond_start=*L;
      (void)parse_expr(vm,L); /* advance over cond for outer scan */
      Lex after_cond=*L;
      long guard=0;
      do {
        vm->break_loop=0; vm->continue_loop=0;
        Lex body=body_start;
        if (exec_stmts_until(vm,&body,"UNTIL",NULL)<0) return -1;
        if (vm->break_loop){ vm->break_loop=0; break; }
        vm->continue_loop=0;
        Lex clex=cond_start;
        long done = parse_expr(vm,&clex);
        if (done) break;
      } while (!vm->fatal && guard++<100000);
      *L=after_cond;
    } else {
      /* REPEAT ... END  (same as LOOP 1..∞ with break only — run once as block) */
      if (kw(&L->cur,"END")) lex_next(L);
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=body_start;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      vm->break_loop=0; vm->continue_loop=0;
    }
    bump(vm); return 1;
  }
  if (kw(&L->cur,"WHILE")){
    lex_next(L);
    Lex cond_start=*L;
    long cond=parse_expr(vm,L);
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"WHILE without END"); return -1; }
    Lex end_tok=*L;
    long guard=0;
    while (cond && !vm->fatal && guard++<100000){
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=body_start;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      if (vm->break_loop){ vm->break_loop=0; break; }
      vm->continue_loop=0;
      Lex clex=cond_start;
      cond=parse_expr(vm,&clex);
    }
    *L=end_tok;
    if (kw(&L->cur,"END")) lex_next(L);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"IF")){
    lex_next(L);
    /* chain: IF c THEN ... ELIF c THEN ... ELSE ... END */
    for(;;){
      long cond=parse_expr(vm,L);
      if (!kw(&L->cur,"THEN")){ fail(vm,"IF expr THEN"); return -1; }
      lex_next(L); skip_nl(L);
      Lex body_start=*L;
      int depth=1;
      while (L->cur.kind!=TK_EOF && depth>0){
        if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
          lex_next(L);
          if (kw(&L->cur,"IF")) lex_next(L);
          continue;
        }
        if (kw(&L->cur,"IF")||kw(&L->cur,"LOOP")||kw(&L->cur,"WHILE")||kw(&L->cur,"FOR")||
            kw(&L->cur,"EACH")||kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")) depth++;
        else if ((kw(&L->cur,"ELSE")||kw(&L->cur,"ELIF")||kw(&L->cur,"ELSEIF")) && depth==1) break;
        else if (kw(&L->cur,"END")){ depth--; if (depth==0) break; }
        lex_next(L);
      }
      if (depth>1){ fail(vm,"IF without END"); return -1; }
      if (cond){
        Lex body=body_start;
        if (exec_stmts_until(vm,&body,"END","ELSE")<0) return -1;
        /* also stop at ELIF */
        /* skip to final END */
        depth=1;
        while (L->cur.kind!=TK_EOF && depth>0){
          if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
            lex_next(L);
            if (kw(&L->cur,"IF")) lex_next(L);
            continue;
          }
          if (kw(&L->cur,"IF")||kw(&L->cur,"LOOP")||kw(&L->cur,"WHILE")||kw(&L->cur,"FOR")||
              kw(&L->cur,"EACH")||kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")) depth++;
          else if (kw(&L->cur,"END")){ depth--; if(depth==0) break; }
          lex_next(L);
        }
        if (kw(&L->cur,"END")) lex_next(L);
        bump(vm); return 1;
      }
      /* not taken */
      if (kw(&L->cur,"ELIF")||kw(&L->cur,"ELSEIF")){ lex_next(L); continue; }
      if (kw(&L->cur,"ELSE")){
        lex_next(L); skip_nl(L);
        Lex body=*L;
        if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
        if (kw(&L->cur,"END")) lex_next(L);
        bump(vm); return 1;
      }
      if (kw(&L->cur,"END")){ lex_next(L); bump(vm); return 1; }
      fail(vm,"IF chain broken"); return -1;
    }
  }
  if (kw(&L->cur,"END")||kw(&L->cur,"ELSE")||kw(&L->cur,"ELIF")||kw(&L->cur,"ELSEIF")||kw(&L->cur,"THEN")){
    return 0; /* stop marker for nested bodies */
  }

  snprintf(vm->err,sizeof vm->err,"unknown form '%s' line %d — place a unit with [name]",
           L->cur.text, L->cur.line);
  fail(vm, vm->err);
  return -1;
}


static int exec_stmts_until(VM *vm, Lex *L, const char *stop1, const char *stop2){
  while (!vm->fatal){
    skip_nl(L);
    if (L->cur.kind==TK_EOF) break;
    if (stop1 && kw(&L->cur,stop1)) break;
    if (stop2 && kw(&L->cur,stop2)) break;
    if (vm->break_loop || vm->continue_loop || vm->return_fn) break;
    int r=parse_form(vm,L);
    if (r<0) return -1;
    if (r==0) break;
    if (vm->break_loop || vm->continue_loop || vm->return_fn) break;
  }
  return vm->fatal ? -1 : 0;
}

static int run_source_inner(const char *src, size_t n, const char *name,
                            cubalc_run_result *out, FILE *trace){
  VM vm; memset(&vm,0,sizeof vm);
  vm.res=out; vm.trace=trace; vm.hold_flash=1;
  snprintf(vm.creed,sizeof vm.creed,"%s",CUBALC_CREED);
  cubalc_async_init(0);
  cubalc_chain_init(&vm.ch);
  vm.last_str[0]=0; vm.last_code=0; vm.last_n=0;
  vm.sp=0;
  {
    const char *se = getenv("CUBALC_SEED");
    if (se && se[0]) vm.rng = (uint32_t)strtoul(se, NULL, 0);
    else vm.rng = (uint32_t)time(NULL) ^ 0xC3C3C3C3u;
    if (!vm.rng) vm.rng = 1;
  }
  vm.ch.hold_flash=1;
  snprintf(vm.ch.creed,sizeof vm.ch.creed,"%s",CUBALC_CREED);
  if (out){ memset(out,0,sizeof*out); out->ok=1; }
  if (name && name[0]){
    const char *sl = strrchr(name, '/');
    if (sl){
      size_t nbase = (size_t)(sl - name);
      if (nbase >= sizeof vm.include_base) nbase = sizeof vm.include_base - 1;
      memcpy(vm.include_base, name, nbase);
      vm.include_base[nbase] = 0;
    } else vm.include_base[0]=0;
  }

  Lex L; lex_init(&L, src, n);
  while (!vm.fatal && L.cur.kind != TK_EOF){
    skip_nl(&L);
    if (L.cur.kind==TK_EOF) break;
    if (parse_form(&vm, &L) < 0) break;
  }
  if (vm.ch.n_cubes>0) cubalc_chain_tick(&vm.ch);

  if (out){
    out->ok = !vm.fatal && out->asserts_fail==0;
    out->n_cubes = vm.ch.n_cubes;
    out->unity = vm.ch.unity;
    if (vm.fatal && !out->err[0]) snprintf(out->err,sizeof out->err,"%s",vm.err);
  }
  if (vm.ch.n_cubes>0){
    /* Cube Law: share state_matrix only · devices free · united visual faces */
    cubalc_chain_publish_united(&vm.ch);
  }
  return out && out->ok ? 0 : 1;
}

int cubalc_run_source(const char *src, size_t n, const char *name,
                      cubalc_run_result *out, FILE *trace){
  if (!src) return 2;
  return run_source_inner(src, n, name, out, trace);
}

int cubalc_run_file(const char *path, cubalc_run_result *out, FILE *trace){
  FILE *f=fopen(path,"rb");
  if (!f){
    if (out){ memset(out,0,sizeof*out); snprintf(out->err,sizeof out->err,"cannot open %s",path); }
    return 2;
  }
  fseek(f,0,SEEK_END); long sz=ftell(f); fseek(f,0,SEEK_SET);
  if (sz<0 || sz>CUBALC_MAX_SRC){ fclose(f); return 2; }
  char *buf=malloc((size_t)sz+1); if(!buf){ fclose(f); return 2; }
  size_t nr=fread(buf,1,(size_t)sz,f); fclose(f); buf[nr]=0;
  int rc=cubalc_run_source(buf,nr,path,out,trace);
  free(buf); return rc;
}
