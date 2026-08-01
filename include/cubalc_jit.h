/* CubalC JIT — Cube Flow native execution
 * Source → bytecode (.cblc) → x86_64 machine code (mmap RWX) → run.
 * Arithmetic/control: native. Cube ops: host trampoline.
 * Law: HOLD_FLASH · matrix SoT · C is bootstrap only.
 */
#ifndef CUBALC_JIT_H
#define CUBALC_JIT_H
#include "cubalc_isa.h"
#include "cubalc_lang.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct cubalc_jit_blob {
  void *code;           /* mmap executable */
  size_t code_cap;
  size_t code_len;
  int n_ins;
  int ok;
  char err[160];
} cubalc_jit_blob;

/* Compile image → native blob (x86_64 Linux). Returns 0 on success. */
int cubalc_jit_compile(const cubalc_image *img, cubalc_jit_blob *blob);

/* Execute native blob on a fresh chain machine. */
int cubalc_jit_run(const cubalc_jit_blob *blob, const cubalc_image *img,
                   cubalc_run_result *out, FILE *trace);

void cubalc_jit_free(cubalc_jit_blob *blob);

/* One-shot: image → JIT → run (falls back to interpreter if JIT fails). */
int cubalc_jit_exec(const cubalc_image *img, cubalc_run_result *out, FILE *trace);

/* Compile high-level .cubalc source → image (lowerer). */
int cubalc_isa_compile_source(const char *src, size_t n, cubalc_image *img,
                              char *err, size_t errn);

/* Self-manifest: compile src → .cblc path → JIT run. */
int cubalc_flow_manifest(const char *src_path, const char *cblc_path,
                         cubalc_run_result *out, FILE *trace);

const char *cubalc_jit_backend(void); /* "x86_64" or "interp" */

#ifdef __cplusplus
}
#endif
#endif
