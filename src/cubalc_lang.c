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
typedef struct { const char *s; size_t n, i; int line; Tok cur; } Lex;
typedef struct { char name[48]; long val; char sval[512]; int is_str; } Var;

typedef struct {
  cubalc_chain ch;
  Var vars[64];
  int n_vars;
  cubalc_run_result *res;
  FILE *trace;
  int hold_flash;
  int fatal;
  char err[160];
  char creed[80];
  /* last placed cube ids in current chunk (for ring) */
  char chunk[40][48];
  int n_chunk;
  char last_str[CUBALC_HOST_STR_MAX];
  int last_code;
  long last_n;
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
    while (L->i<L->n && isdigit((unsigned char)L->s[L->i])){
      if (k+1<sizeof b) b[k++]=L->s[L->i]; L->i++;
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
    "NEXUS_COORD v1 | from=play | type=world | hold_flash=1 | visual=cubes |", &gen);
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
  if (!create || vm->n_vars >= 64) return NULL;
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
    fail(vm, "world full — budget of cubes");
    return -1;
  }
  chunk_push(vm, id);
  return find_cube(vm, id);
}
static void do_plug(VM *vm, const char *a, const char *b){
  int ia=find_cube(vm,a), ib=find_cube(vm,b);
  if (ia<0){ place_cube(vm,a,a,1); ia=find_cube(vm,a); }
  if (ib<0){ place_cube(vm,b,b,1); ib=find_cube(vm,b); }
  if (ia<0||ib<0){ fail(vm,"plug missing cube"); return; }
  cubalc_cube_plug(&vm->ch, ia, ib);
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
  /* [fleet] — Grokium nanobot roles as cubes */
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
    if (vm->trace) fprintf(vm->trace, "# fleet cubes placed\n");
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
    if (strcmp(name,"SET")==0 || strcmp(name,"POPCOUNT")==0 ||
        strcmp(name,"ENERGY")==0 || strcmp(name,"DIGIT")==0 ||
        strcmp(name,"BIT")==0){
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
static long parse_expr(VM *vm, Lex *L){
  long v=parse_add(vm,L);
  if (L->cur.kind==TK_EQEQ){ lex_next(L); return v==parse_add(vm,L); }
  if (L->cur.kind==TK_NE){ lex_next(L); return v!=parse_add(vm,L); }
  if (L->cur.kind==TK_LT){ lex_next(L); return v<parse_add(vm,L); }
  if (L->cur.kind==TK_LE){ lex_next(L); return v<=parse_add(vm,L); }
  if (L->cur.kind==TK_GT){ lex_next(L); return v>parse_add(vm,L); }
  if (L->cur.kind==TK_GE){ lex_next(L); return v>=parse_add(vm,L); }
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
    fail(vm, "SYS: READ|WRITE|ENV|EXIST|WHICH|HTTP|SPAWN|JOIN|JSON|CHAT|ARG|NUM");
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
  if (kw(&L->cur,"PLUG")){
    lex_next(L);
    if (kw(&L->cur,"RING")){ lex_next(L); do_ring(vm); bump(vm); return 1; }
    if (L->cur.kind!=TK_IDENT){ fail(vm,"PLUG"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"PLUG b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    do_plug(vm,a,b); bump(vm); return 1;
  }
  if (kw(&L->cur,"IMPULSE")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"IMPULSE"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int p=1; if (L->cur.kind==TK_NUM){ p=L->cur.num?1:0; lex_next(L); }
    cubalc_chain_impulse(&vm->ch,id,(uint8_t)p); bump(vm); return 1;
  }
  if (kw(&L->cur,"FLOW")||kw(&L->cur,"TICK")){
    lex_next(L);
    int n=8; if (L->cur.kind==TK_NUM){ n=(int)L->cur.num; lex_next(L); }
    if (L->cur.kind==TK_NUM) lex_next(L);
    do_flow(vm,n); bump(vm); return 1;
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
    if (L->cur.kind==TK_STR){
      var_set_str(vm, name, L->cur.text);
      lex_next(L);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"LAST") || (L->cur.kind==TK_IDENT && strcmp(L->cur.text,"LAST")==0)){
      var_set_str(vm, name, vm->last_str);
      lex_next(L);
      bump(vm); return 1;
    }
    long v=parse_expr(vm,L);
    var_set_num(vm, name, v);
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
    if (vm->trace) fprintf(vm->trace,"# POSE mode=%s cubes=%d\n", mode, vm->ch.n_cubes);
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
    place_cube(vm,"lizard","quest_lizard",1);
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
  if (kw(&L->cur,"VIZ")||kw(&L->cur,"SPIN")||kw(&L->cur,"SHOW")||kw(&L->cur,"HELLO")||
      kw(&L->cur,"WAIT")||kw(&L->cur,"SLEEP")){
    while (L->cur.kind!=TK_NL && L->cur.kind!=TK_EOF) lex_next(L);
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
    while (L->cur.kind!=TK_EOF && depth>0){
      if (kw(&L->cur,"LOOP")||kw(&L->cur,"IF")||kw(&L->cur,"WHILE")) depth++;
      else if (kw(&L->cur,"END")){ depth--; if (depth==0) break; }
      lex_next(L);
    }
    if (depth!=0){ fail(vm,"LOOP without END"); return -1; }
    for (long t=0;t<times && !vm->fatal;t++){
      long *it=var_slot(vm,"IT",1); if (it) *it=t;
      Lex body=save;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
    }
    if (kw(&L->cur,"END")) lex_next(L);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"WHILE")){
    lex_next(L);
    Lex cond_start=*L;
    long cond=parse_expr(vm,L);
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF && depth>0){
      if (kw(&L->cur,"LOOP")||kw(&L->cur,"IF")||kw(&L->cur,"WHILE")) depth++;
      else if (kw(&L->cur,"END")){ depth--; if (depth==0) break; }
      lex_next(L);
    }
    if (depth!=0){ fail(vm,"WHILE without END"); return -1; }
    Lex end_tok=*L;
    long guard=0;
    while (cond && !vm->fatal && guard++<100000){
      Lex body=body_start;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      Lex clex=cond_start;
      cond=parse_expr(vm,&clex);
    }
    *L=end_tok;
    if (kw(&L->cur,"END")) lex_next(L);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"IF")){
    lex_next(L);
    long cond=parse_expr(vm,L);
    if (!kw(&L->cur,"THEN")){ fail(vm,"IF expr THEN"); return -1; }
    lex_next(L); skip_nl(L);
    Lex then_start=*L;
    int depth=1; int has_else=0; Lex else_start; memset(&else_start,0,sizeof else_start);
    while (L->cur.kind!=TK_EOF && depth>0){
      if (kw(&L->cur,"IF")||kw(&L->cur,"LOOP")||kw(&L->cur,"WHILE")) depth++;
      else if (kw(&L->cur,"ELSE") && depth==1){
        else_start=*L; lex_next(&else_start); skip_nl(&else_start);
        has_else=1; lex_next(L); continue;
      } else if (kw(&L->cur,"END")){
        depth--; if (depth==0) break;
      }
      lex_next(L);
    }
    if (depth!=0){ fail(vm,"IF without END"); return -1; }
    if (cond){
      Lex body=then_start;
      if (exec_stmts_until(vm,&body,"END","ELSE")<0) return -1;
    } else if (has_else){
      Lex body=else_start;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
    }
    if (kw(&L->cur,"END")) lex_next(L);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"END")||kw(&L->cur,"ELSE")||kw(&L->cur,"THEN")){
    return 0; /* stop marker for nested bodies */
  }

  snprintf(vm->err,sizeof vm->err,"unknown form '%s' line %d — place a cube with [name]",
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
    int r=parse_form(vm,L);
    if (r<0) return -1;
    if (r==0) break;
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
  vm.ch.hold_flash=1;
  snprintf(vm.ch.creed,sizeof vm.ch.creed,"%s",CUBALC_CREED);
  if (out){ memset(out,0,sizeof*out); out->ok=1; }
  (void)name;

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
