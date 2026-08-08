/* CubalC lang internal — COP/flow VM (not public product API).
 * Law: cube is SoT · flow before compile · pure C · HTTP never required.
 */
#ifndef CUBALC_LANG_INTERNAL_H
#define CUBALC_LANG_INTERNAL_H

#include "cubalc_platform.h"
#include "cubalc_lang.h"
#include "cubalc_algocube.h"
#include "cubalc_cubechain.h"
#include "cubalc_async.h"
#include "cubalc_hw.h"
#include "cubalc_hostops.h"
#include "cubalc_smx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- tokens / lexer / VM ---- */
enum {
  TK_EOF=0, TK_IDENT, TK_NUM, TK_STR, TK_NL,
  TK_LBRACK, TK_RBRACK, TK_TILDE, TK_BANG, TK_QMARK, TK_COLON, TK_PIPE,
  TK_PLUS, TK_MINUS, TK_STAR, TK_SLASH, TK_PERCENT,
  TK_EQ, TK_EQEQ, TK_NE, TK_LT, TK_LE, TK_GT, TK_GE,
  TK_LPAREN, TK_RPAREN, TK_COMMA
};

typedef struct { int kind; long num; char text[8192]; int line; } Tok;
typedef struct { const char *s; size_t n, i; int line; size_t tok_off; Tok cur; } Lex;
/* Agent plates (PLATE after plate_boot/SETP) need more than 512 bytes of JSON.
 * 4096 fits multi-key agent state without truncating SAVEPLATE write-back.
 * 128 vars × 4K ≈ 0.5MB of the stack VM — acceptable next to last_str/chain. */
#ifndef CUBALC_VAR_STR_MAX
#define CUBALC_VAR_STR_MAX 4096
#endif
typedef struct { char name[48]; long val; char sval[CUBALC_VAR_STR_MAX]; int is_str; } Var;

typedef struct {
  char name[48];
  char params[8][32]; /* optional named formals (also ARG0..) */
  int n_params;
  const char *body;
  size_t len;
} FnDef;

/* ---- OOP + COP engine plane (beyond C++ class inheritance) ----
 * CLASS/FIELD/METHOD/NEW/SEND compose *reusable cubes* and plain objects.
 * Game engines: ENTITY/SPAWN/TICK/SCENE ride the same plane; FLOW is law. */
#define CUBALC_MAX_FNS      48
#define CUBALC_MAX_CLASSES  24
#define CUBALC_MAX_METHODS  24
#define CUBALC_MAX_FIELDS   24
#define CUBALC_MAX_OBJS     64

typedef struct {
  char name[32];
  long def_num;
  char def_str[96];
  int is_str;
  int has_def;
} FieldDef;

typedef struct {
  char name[32];
  char params[8][32];
  int n_params;
  const char *body;
  size_t len;
} MethodDef;

typedef struct {
  char name[48];
  FieldDef fields[CUBALC_MAX_FIELDS];
  int n_fields;
  MethodDef methods[CUBALC_MAX_METHODS];
  int n_methods;
  char role[24]; /* COP default cube role when CUBE/ENTITY OF class */
} ClassDef;

typedef struct {
  char name[48];
  int class_idx;
  long fnum[CUBALC_MAX_FIELDS];
  char fstr[CUBALC_MAX_FIELDS][128];
  int fis_str[CUBALC_MAX_FIELDS];
  int live;
  int cube_idx; /* -1 pure object; else bound cube slot */
} ObjInst;

#define CUBALC_CELL_N   64
#define CUBALC_STACK_N  32
typedef struct {
  cubalc_chain ch;
  Var vars[128];
  int n_vars;
  FnDef fns[CUBALC_MAX_FNS];
  int n_fns;
  ClassDef classes[CUBALC_MAX_CLASSES];
  int n_classes;
  ObjInst objs[CUBALC_MAX_OBJS];
  int n_objs;
  char this_obj[48];   /* current method receiver (THIS/SELF) */
  char scene[48];      /* optional game SCENE name tag */
  cubalc_run_result *res;
  FILE *trace;
  int hold_flash;
  int fatal;
  int halt;       /* EXIT form: stop program (may be clean exit_code==0) */
  int exit_code;  /* process-oriented code from EXIT n */
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
  /* ASSERT/EXPECT diagnostics from last comparison in parse_cmp */
  int last_cmp_kind;   /* 0 none · 1 numeric · 2 string · 3 bare value */
  char last_cmp_op[4]; /* == != < <= > >= */
  char last_cmp_left[96];
  char last_cmp_right[96];
  char include_base[512];
  /* INCLUDE ONCE — resolved paths already loaded this run (max 24). */
  char included[24][160];
  int n_included;
  /* Retained INCLUDE source so FN/CLASS METHOD bodies stay valid all run. */
  char *include_bufs[24];
  int n_include_bufs;
  /* digit-1 data plane: integer cells + stack */
  long cells[CUBALC_CELL_N];
  long stack[CUBALC_STACK_N];
  int sp; /* stack depth 0..CUBALC_STACK_N */
  /* SMX2 / P2P — Law of Manifestation (binary talk, no HTTP) */
  cubalc_smx_ctx smx;
  int smx_ok;
  int smx_talks;
} VM;


void cubalc_lang_fail(VM *vm, const char *msg);
void cubalc_lang_fail_at(VM *vm, int line, const char *msg);
void cubalc_lang_bump(VM *vm);
int  cubalc_lang_kw(const Tok *t, const char *k);
void cubalc_lang_lex_skip(Lex *L);
void cubalc_lang_lex_next(Lex *L);
void cubalc_lang_lex_init(Lex *L, const char *s, size_t n);
void cubalc_lang_skip_nl(Lex *L);

int  cubalc_lang_find_cube(VM *vm, const char *id);
void cubalc_lang_ensure_world(VM *vm);
void cubalc_lang_chunk_push(VM *vm, const char *id);
Var *cubalc_lang_var_get(VM *vm, const char *name, int create);
void cubalc_lang_var_set_num(VM *vm, const char *name, long val);
void cubalc_lang_var_set_str(VM *vm, const char *name, const char *s);
long *cubalc_lang_var_slot(VM *vm, const char *name, int create);

int  cubalc_lang_place_cube(VM *vm, const char *id, const char *role, int proton);
void cubalc_lang_do_plug(VM *vm, const char *a, const char *b);
void cubalc_lang_do_reverse(VM *vm, const char *a, const char *b);
void cubalc_lang_do_unplug(VM *vm, const char *a, const char *b);
void cubalc_lang_do_io(VM *vm, const char *id, int face, int is_out);
void cubalc_lang_do_nest(VM *vm, const char *parent, const char *child);
void cubalc_lang_do_unnest(VM *vm, const char *child);
void cubalc_lang_do_compile_cube(VM *vm, const char *id);
void cubalc_lang_do_compile_all(VM *vm);
void cubalc_lang_do_ring(VM *vm);
void cubalc_lang_do_flow(VM *vm, int n);
void cubalc_lang_do_show(VM *vm, const char *id);
void cubalc_lang_do_deconstruct(VM *vm, const char *id);
void cubalc_lang_do_reconstruct(VM *vm, const char *id);
long cubalc_lang_do_decide(VM *vm, const char *id);
long cubalc_lang_do_compare(VM *vm, const char *ida, const char *idb);
long cubalc_lang_do_harmony(VM *vm, const char *target);
long cubalc_lang_do_resolve(VM *vm, const char *target);
void cubalc_lang_do_setdigit(VM *vm, const char *id, long d);
void cubalc_lang_do_foldbits(VM *vm, const char *id, const char *bits);
int  cubalc_lang_resolve_str_arg(VM *vm, Lex *L, char *out, size_t outn);

long cubalc_lang_parse_expr(VM *vm, Lex *L);
long cubalc_lang_parse_prim(VM *vm, Lex *L);
int  cubalc_lang_block_scan_step(Lex *L, int *depth, int allow_until);
int  cubalc_lang_parse_cube(VM *vm, Lex *L);
int  cubalc_lang_parse_cube_body(VM *vm, Lex *L);
int  cubalc_lang_parse_form(VM *vm, Lex *L);
int  cubalc_lang_exec_stmts_until(VM *vm, Lex *L, const char *stop1, const char *stop2);

int cubalc_lang_ops_core(VM *vm, Lex *L);
int cubalc_lang_ops_toc(VM *vm, Lex *L);
int cubalc_lang_ops_stack(VM *vm, Lex *L);
int cubalc_lang_ops_dual(VM *vm, Lex *L);
int cubalc_lang_ops_math(VM *vm, Lex *L);
int cubalc_lang_ops_bit(VM *vm, Lex *L);
int cubalc_lang_ops_cell(VM *vm, Lex *L);
int cubalc_lang_ops_flow(VM *vm, Lex *L);
int cubalc_lang_ops_smx(VM *vm, Lex *L);

#define fail(vm,msg)          cubalc_lang_fail((vm),(msg))
#define fail_at(vm,L,msg)     cubalc_lang_fail_at((vm),(L)->cur.line,(msg))
#define bump(vm)              cubalc_lang_bump((vm))
#define kw(t,k)               cubalc_lang_kw((t),(k))
#define lex_skip(L)           cubalc_lang_lex_skip((L))
#define lex_next(L)           cubalc_lang_lex_next((L))
#define lex_init(L,s,n)       cubalc_lang_lex_init((L),(s),(n))
#define skip_nl(L)            cubalc_lang_skip_nl((L))
#define find_cube(vm,id)      cubalc_lang_find_cube((vm),(id))
#define ensure_world(vm)      cubalc_lang_ensure_world((vm))
#define chunk_push(vm,id)     cubalc_lang_chunk_push((vm),(id))
#define var_get(vm,n,c)       cubalc_lang_var_get((vm),(n),(c))
#define var_set_num(vm,n,v)   cubalc_lang_var_set_num((vm),(n),(v))
#define var_set_str(vm,n,s)   cubalc_lang_var_set_str((vm),(n),(s))
#define var_slot(vm,n,c)      cubalc_lang_var_slot((vm),(n),(c))
#define place_cube            cubalc_lang_place_cube
#define do_plug               cubalc_lang_do_plug
#define do_reverse            cubalc_lang_do_reverse
#define do_unplug             cubalc_lang_do_unplug
#define do_io                 cubalc_lang_do_io
#define do_nest               cubalc_lang_do_nest
#define do_unnest             cubalc_lang_do_unnest
#define do_compile_cube       cubalc_lang_do_compile_cube
#define do_compile_all        cubalc_lang_do_compile_all
#define do_ring               cubalc_lang_do_ring
#define do_flow               cubalc_lang_do_flow
#define do_show               cubalc_lang_do_show
#define do_deconstruct        cubalc_lang_do_deconstruct
#define do_reconstruct        cubalc_lang_do_reconstruct
#define do_decide             cubalc_lang_do_decide
#define do_compare            cubalc_lang_do_compare
#define do_harmony            cubalc_lang_do_harmony
#define do_resolve            cubalc_lang_do_resolve
#define do_setdigit           cubalc_lang_do_setdigit
#define do_foldbits           cubalc_lang_do_foldbits
#define resolve_str_arg       cubalc_lang_resolve_str_arg
#define parse_expr            cubalc_lang_parse_expr
#define parse_prim            cubalc_lang_parse_prim
#define block_scan_step       cubalc_lang_block_scan_step
#define parse_cube            cubalc_lang_parse_cube
#define parse_cube_body       cubalc_lang_parse_cube_body
#define parse_form            cubalc_lang_parse_form
#define exec_stmts_until      cubalc_lang_exec_stmts_until

#ifdef __cplusplus
}
#endif
#endif
