# CubalC language modules

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
