/* Pure assembly matrix lanes — lower than C */
#ifndef CUBALC_ASM_H
#define CUBALC_ASM_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
uint64_t cubalc_asm_pop64(uint64_t x);
void cubalc_asm_xor_lanes(uint64_t *dst, const uint64_t *a, const uint64_t *b, size_t n);
#ifdef __cplusplus
}
#endif
#endif
