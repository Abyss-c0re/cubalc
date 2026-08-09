/* CubalC lang — place plug pulse flow look */
#ifndef CUBALC_LANG_H
#define CUBALC_LANG_H
#include "cubalc.h"
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct cubalc_run_result {
  int ok;
  int asserts_ok;
  int asserts_fail;
  int stmts;
  int n_cubes;
  float unity;
  char err[160];
  char last_err[160]; /* sticky LAST_ERR/ERR even when run ok (soft FAIL/EXPECT) */
  char last_print[256];
  /* Usability: source line context for agent-readable failures (from "line N" in err). */
  int err_line;       /* 0 if unknown */
  char err_src[200];  /* trimmed source text at err_line */
  int exit_code;      /* EXIT n — process-oriented; 0 default */
  int halted;         /* 1 if EXIT stopped the program */
  /* Usability: resolved INCLUDE paths this run (dual of LISTINCLUDES for agents). */
  int includes_n;     /* count of modules loaded (INCLUDE / -I preload) */
  char includes[640]; /* newline-joined resolved paths */
} cubalc_run_result;

int cubalc_run_file(const char *path, cubalc_run_result *out, FILE *trace);
int cubalc_run_source(const char *src, size_t n, const char *name,
                      cubalc_run_result *out, FILE *trace);

#ifdef __cplusplus
}
#endif
#endif
