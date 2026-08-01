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
  char last_print[256];
} cubalc_run_result;

int cubalc_run_file(const char *path, cubalc_run_result *out, FILE *trace);
int cubalc_run_source(const char *src, size_t n, const char *name,
                      cubalc_run_result *out, FILE *trace);

#ifdef __cplusplus
}
#endif
#endif
