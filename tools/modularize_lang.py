#!/usr/bin/env python3
"""CubalC lang modularization — preserve behavior, COP/flow, multiplatform."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC_PATH = ROOT / "src" / "cubalc_lang.c"
text = SRC_PATH.read_text(encoding="utf-8", errors="replace")
lines = text.splitlines(keepends=True)
N = len(lines)
print(f"source lines: {N}")


def ln(a: int, b: int) -> str:
    return "".join(lines[a - 1 : b])


def write_text(path: Path, body: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if not body.endswith("\n"):
        body += "\n"
    path.write_text(body, encoding="utf-8")
    print(f"  wrote {path.relative_to(ROOT)} ({body.count(chr(10))} lines)")


def find_line(pat: str, start: int = 1) -> int:
    rx = re.compile(pat)
    for i in range(start - 1, N):
        if rx.search(lines[i]):
            return i + 1
    raise SystemExit(f"pattern not found: {pat}")


# Detect structure
PF = find_line(r"^static int parse_form\(VM \*vm, Lex \*L\)\{")
EXEC = find_line(r"^static int exec_stmts_until\(VM \*vm, Lex \*L, const char \*stop1")
# last exec_stmts_until is the definition after parse_form
for i in range(N - 1, -1, -1):
    if re.match(r"^static int exec_stmts_until\(VM \*vm, Lex \*L, const char \*stop1", lines[i]):
        EXEC = i + 1
        break
RUN = find_line(r"^int cubalc_run_source\(")
print(f"parse_form@{PF} exec@{EXEC} run_source@{RUN}")

# Plane starts (search comments inside parse_form)
def find_comment(sub: str, start: int) -> int:
    for i in range(start - 1, N):
        if sub in lines[i]:
            return i + 1
    raise SystemExit(f"comment not found from {start}: {sub}")


P_DATA = find_comment("digit-1 data plane: cells + stack", PF)
P_STACK = find_comment("digit-4 data plane: Forth-style stack combinators", P_DATA)
P_DUAL = find_comment("digit-7 dual-stack pair ALU: DADD", P_STACK)
P_MATH = find_comment("digit-2 stack number theory / div modes", P_DUAL)
P_BIT = find_comment("digit-3 stack bitwise ALU: SAND SOR SXOR", P_MATH)
P_CELL = find_comment("digit-5 cell search ext: FINDLASTCELL", P_BIT)
P_FLOW = find_line(r'^\s*if \(kw\(&L->cur,"FN"\)\|\|kw\(&L->cur,"FUNC"\)', P_CELL)
# parse_form ends just before EXEC
PF_END = EXEC - 1  # should be blank or closing brace
# find closing brace of parse_form
for i in range(EXEC - 2, PF, -1):
    if lines[i].strip() == "}":
        PF_END = i  # exclusive end for body is i (closing brace line)
        break
print(f"planes: data@{P_DATA} stack@{P_STACK} dual@{P_DUAL} math@{P_MATH} bit@{P_BIT} cell@{P_CELL} flow@{P_FLOW} pf_end@{PF_END}")

# --- headers ---
platform_h = r"""/* CubalC platform shims — pure C, multiplatform (law: devices free). */
#ifndef CUBALC_PLATFORM_H
#define CUBALC_PLATFORM_H

#if defined(_WIN32) || defined(_WIN64)
#  ifndef CUBALC_OS_WINDOWS
#  define CUBALC_OS_WINDOWS 1
#  endif
#  ifndef _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_WARNINGS
#  endif
#elif defined(__APPLE__)
#  define CUBALC_OS_DARWIN 1
#  ifndef _DARWIN_C_SOURCE
#  define _DARWIN_C_SOURCE 1
#  endif
#else
#  define CUBALC_OS_POSIX 1
#  ifndef _POSIX_C_SOURCE
#  define _POSIX_C_SOURCE 200809L
#  endif
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <errno.h>

#if defined(CUBALC_OS_WINDOWS)
#  include <io.h>
#  include <process.h>
#  ifndef strcasecmp
#    define strcasecmp _stricmp
#  endif
#  ifndef strncasecmp
#    define strncasecmp _strnicmp
#  endif
#else
#  include <strings.h>
#  include <unistd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CUBALC_OS_WINDOWS)
#  define CUBALC_PATH_SEP '\\'
#  define CUBALC_PATH_SEP_STR "\\"
#else
#  define CUBALC_PATH_SEP '/'
#  define CUBALC_PATH_SEP_STR "/"
#endif

static inline const char *cubalc_path_slash(const char *p) {
  if (!p) return NULL;
  const char *a = strrchr(p, '/');
#if defined(CUBALC_OS_WINDOWS)
  const char *b = strrchr(p, '\\');
  if (!a || (b && b > a)) a = b;
#endif
  return a;
}

#ifdef __cplusplus
}
#endif
#endif /* CUBALC_PLATFORM_H */
"""
write_text(ROOT / "include" / "cubalc_platform.h", platform_h)

type_block = ln(19, 65)
internal_h = """/* CubalC lang internal — COP/flow VM (not public product API).
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

#ifdef __cplusplus
extern "C" {
#endif

/* ---- tokens / lexer / VM ---- */
@@TYPE@@

void cubalc_lang_fail(VM *vm, const char *msg);
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

#define fail(vm,msg)          cubalc_lang_fail((vm),(msg))
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
""".replace("@@TYPE@@", type_block)
write_text(ROOT / "include" / "lang" / "cubalc_lang_internal.h", internal_h)


def rename_statics(code: str) -> str:
    pairs = [
        (r"\bstatic void fail\(", "void cubalc_lang_fail("),
        (r"\bstatic void bump\(", "void cubalc_lang_bump("),
        (r"\bstatic int kw\(", "int cubalc_lang_kw("),
        (r"\bstatic void lex_skip\(", "void cubalc_lang_lex_skip("),
        (r"\bstatic void lex_next\(", "void cubalc_lang_lex_next("),
        (r"\bstatic void lex_init\(", "void cubalc_lang_lex_init("),
        (r"\bstatic void skip_nl\(", "void cubalc_lang_skip_nl("),
        (r"\bstatic int find_cube\(", "int cubalc_lang_find_cube("),
        (r"\bstatic void ensure_world\(", "void cubalc_lang_ensure_world("),
        (r"\bstatic void chunk_push\(", "void cubalc_lang_chunk_push("),
        (r"\bstatic Var \*var_get\(", "Var *cubalc_lang_var_get("),
        (r"\bstatic void var_set_num\(", "void cubalc_lang_var_set_num("),
        (r"\bstatic void var_set_str\(", "void cubalc_lang_var_set_str("),
        (r"\bstatic long \*var_slot\(", "long *cubalc_lang_var_slot("),
        (r"\bstatic int place_cube\(", "int cubalc_lang_place_cube("),
        (r"\bstatic void do_plug\(", "void cubalc_lang_do_plug("),
        (r"\bstatic void do_reverse\(", "void cubalc_lang_do_reverse("),
        (r"\bstatic void do_unplug\(", "void cubalc_lang_do_unplug("),
        (r"\bstatic void do_io\(", "void cubalc_lang_do_io("),
        (r"\bstatic void do_nest\(", "void cubalc_lang_do_nest("),
        (r"\bstatic void do_unnest\(", "void cubalc_lang_do_unnest("),
        (r"\bstatic void do_compile_cube\(", "void cubalc_lang_do_compile_cube("),
        (r"\bstatic void do_compile_all\(", "void cubalc_lang_do_compile_all("),
        (r"\bstatic void do_ring\(", "void cubalc_lang_do_ring("),
        (r"\bstatic void do_flow\(", "void cubalc_lang_do_flow("),
        (r"\bstatic void do_show\(", "void cubalc_lang_do_show("),
        (r"\bstatic void do_deconstruct\(", "void cubalc_lang_do_deconstruct("),
        (r"\bstatic void do_reconstruct\(", "void cubalc_lang_do_reconstruct("),
        (r"\bstatic long do_decide\(", "long cubalc_lang_do_decide("),
        (r"\bstatic long do_compare\(", "long cubalc_lang_do_compare("),
        (r"\bstatic long do_harmony\(", "long cubalc_lang_do_harmony("),
        (r"\bstatic long do_resolve\(", "long cubalc_lang_do_resolve("),
        (r"\bstatic void do_setdigit\(", "void cubalc_lang_do_setdigit("),
        (r"\bstatic void do_foldbits\(", "void cubalc_lang_do_foldbits("),
        (r"\bstatic int resolve_str_arg\(", "int cubalc_lang_resolve_str_arg("),
        (r"\bstatic long parse_expr\(", "long cubalc_lang_parse_expr("),
        (r"\bstatic long parse_prim\(", "long cubalc_lang_parse_prim("),
        (r"\bstatic int block_scan_step\(", "int cubalc_lang_block_scan_step("),
        (r"\bstatic int parse_cube\(", "int cubalc_lang_parse_cube("),
        (r"\bstatic int parse_cube_body\(", "int cubalc_lang_parse_cube_body("),
        (r"\bstatic int parse_form\(", "int cubalc_lang_parse_form("),
        (r"\bstatic int exec_stmts_until\(", "int cubalc_lang_exec_stmts_until("),
    ]
    out = code
    for a, b in pairs:
        out = re.sub(a, b, out)
    for pat in [
        r"static int cubalc_lang_parse_form\(VM \*vm, Lex \*L\);\s*",
        r"static int cubalc_lang_parse_cube\(VM \*vm, Lex \*L\);\s*",
        r"static void cubalc_lang_do_deconstruct\(VM \*vm, const char \*id\);\s*",
        r"static void cubalc_lang_do_reconstruct\(VM \*vm, const char \*id\);\s*",
        r"static long cubalc_lang_do_decide\(VM \*vm, const char \*id\);\s*",
        r"static long \*cubalc_lang_var_slot\(VM \*vm, const char \*name, int create\);\s*",
        r"static int cubalc_lang_exec_stmts_until\(VM \*vm, Lex \*L, const char \*stop1, const char \*stop2\);\s*",
        r"static long cubalc_lang_parse_expr\(VM \*vm, Lex \*L\);\s*(/\*[^*]*\*/\s*)?",
    ]:
        out = re.sub(pat, "", out)
    out = re.sub(r"(?m)^#define _POSIX_C_SOURCE.*\n", "", out)
    out = re.sub(r"(?m)^#include .*\n", "", out)
    return out


def write_mod(name: str, body: str) -> None:
    write_text(
        ROOT / "src" / "lang" / name,
        f"/* CubalC lang — {name} (COP/flow · pure C · cube is SoT) */\n"
        f'#include "lang/cubalc_lang_internal.h"\n\n{body}',
    )


# Core before parse_form
core = ln(1, PF - 1)
core = rename_statics(core)
core = re.sub(
    r"enum \{\s*TK_EOF[\s\S]*?\n\} VM;\s*",
    "/* types: include/lang/cubalc_lang_internal.h */\n\n",
    core,
    count=1,
)
write_mod("lang_core.c", core)

# Planes: body lines inside parse_form (after opening, before closing)
# parse_form starts at PF; first content typically PF+1
planes = [
    ("ops_core", PF + 1, P_DATA - 1),
    ("ops_toc", P_DATA, P_STACK - 1),
    ("ops_stack", P_STACK, P_DUAL - 1),
    ("ops_dual", P_DUAL, P_MATH - 1),
    ("ops_math", P_MATH, P_BIT - 1),
    ("ops_bit", P_BIT, P_CELL - 1),
    ("ops_cell", P_CELL, P_FLOW - 1),
    ("ops_flow", P_FLOW, PF_END - 1),  # exclude closing brace of parse_form
]
for name, a, b in planes:
    body = ln(a, b)
    fn = (
        f"int cubalc_lang_{name}(VM *vm, Lex *L){{\n"
        f"  /* plane {name}: L{a}-{b} */\n"
        f"{body}"
        f"  return 0;\n"
        f"}}\n"
    )
    write_mod(f"lang_{name}.c", fn)

parse_c = """int cubalc_lang_parse_form(VM *vm, Lex *L){
  skip_nl(L);
  if (L->cur.kind==TK_EOF) return 0;

  int r;
  if ((r = cubalc_lang_ops_core(vm, L)) != 0) return r;
  if ((r = cubalc_lang_ops_toc(vm, L)) != 0) return r;
  if ((r = cubalc_lang_ops_stack(vm, L)) != 0) return r;
  if ((r = cubalc_lang_ops_dual(vm, L)) != 0) return r;
  if ((r = cubalc_lang_ops_math(vm, L)) != 0) return r;
  if ((r = cubalc_lang_ops_bit(vm, L)) != 0) return r;
  if ((r = cubalc_lang_ops_cell(vm, L)) != 0) return r;
  if ((r = cubalc_lang_ops_flow(vm, L)) != 0) return r;

  snprintf(vm->err,sizeof vm->err,"unknown form '%s' line %d — place a unit with [name]",
           L->cur.text, L->cur.line);
  fail(vm, vm->err);
  return -1;
}
"""
write_mod("lang_parse.c", parse_c)

run_src = ln(EXEC, N)
run_src = rename_statics(run_src)
run_src = run_src.replace("strrchr(name, '/')", "cubalc_path_slash(name)")
write_mod("lang_run.c", run_src)

write_text(
    ROOT / "src" / "cubalc_lang.c",
    "/* Modular language: src/lang/* — see src/lang/README.md */\n"
    '#include "lang/cubalc_lang_internal.h"\n',
)

write_text(
    ROOT / "src" / "lang" / "README.md",
    """# CubalC language modules

| Module | Role |
|--------|------|
| lang_core.c | lexer, VM, expr, cube place/plug |
| lang_ops_core.c | hold, ASYNC, SYS, strings |
| lang_ops_toc.c | stack↔cell TOC plane |
| lang_ops_stack.c | depth + stack ALU |
| lang_ops_dual.c | dual-stack D* |
| lang_ops_math.c | numthy / modular / pack |
| lang_ops_bit.c | bitfield / mask / sat |
| lang_ops_cell.c | *CELL range plane |
| lang_ops_flow.c | FN / LET / control / ASSERT |
| lang_parse.c | parse_form dispatcher |
| lang_run.c | public run_source / run_file |

Headers: `include/lang/cubalc_lang_internal.h`, `include/cubalc_platform.h`  
Public API: `include/cubalc_lang.h`
""",
)

print("modularize complete")
