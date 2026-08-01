/* Anything → CubalC translator.
 * Braincube runs State Matrix · algocube digit · DECIDE.
 * Law: HOLD_FLASH · matrix SoT · binary talk · no prose on the wire.
 */
#ifndef CUBALC_TRANSLATE_H
#define CUBALC_TRANSLATE_H
#include <stddef.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Translate free-form input (plate, bits, prose, JSON crumbs, pseudo-C)
 * into a lawful .cubalc program that ends with DECIDE on braincube.
 * Returns 0 on success. Writes source into out (NUL-terminated).
 */
int cubalc_translate(const char *in, size_t n,
                     char *out, size_t out_cap,
                     char *err, size_t err_cap);

/* Detect already-CubalC (returns 1 if looks like source). */
int cubalc_looks_like_cubalc(const char *in, size_t n);

#ifdef __cplusplus
}
#endif
#endif
