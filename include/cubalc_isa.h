/* CubalC ISA — lower than C
 *
 * CubalC is NOT "a language written in C".
 * C is only a bootstrap host (loader / OS glue).
 *
 * Real program form = Cube bytecode (.cblc): fixed words over the State Matrix machine.
 * Human form      = cubical [] text that assembles into that bytecode.
 * Hot bits         = packed matrices · optional asm lanes · future GPU same blob.
 *
 * Law: Cube is SoT · binary talk · HOLD_FLASH · no prose on the wire.
 */
#ifndef CUBALC_ISA_H
#define CUBALC_ISA_H
#include "cubalc.h"
#include "cubalc_lang.h"
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CUBALC_ISA_MAGIC   0x43424C43u /* CBLC */
#define CUBALC_ISA_VER     1
#define CUBALC_ISA_MAX_INS 4096
#define CUBALC_ISA_MAX_STR 64
#define CUBALC_ISA_MAX_STRTAB 32
#define CUBALC_ISA_MAX_REG 32

/* Opcodes — cube machine code (not C statements) */
enum cubalc_op {
  OP_HALT    = 0x00,
  OP_NOP     = 0x01,
  OP_GENESIS = 0x02, /* a = str plate or 0 for default */
  OP_SPAWN   = 0x03, /* a=id str, b=role str, imm=proton */
  OP_PLUG    = 0x04, /* a=id str, b=id str */
  OP_RING    = 0x05,
  OP_IMPULSE = 0x06, /* a=id str, imm=proton */
  OP_FLOW    = 0x07, /* imm = n ticks */
  OP_SETBIT  = 0x08, /* a=id, b=bit, imm=on */
  OP_SPIN    = 0x09, /* a=id, imm=dyaw */
  OP_SHOW    = 0x0A, /* a=id or 0 first */
  OP_PRINT   = 0x0B, /* a=str label, b=reg */
  OP_ASSERT  = 0x0C, /* a=reg must be nonzero */
  OP_LOADI   = 0x0D, /* a=reg, imm=value (signed in imm8 extended) · use imm16 path */
  OP_ADD     = 0x0E, /* a=reg, b=reg */
  OP_SUB     = 0x0F,
  OP_MUL     = 0x10,
  OP_MOV     = 0x11, /* a=dst, b=src */
  OP_GETSET  = 0x12, /* a=reg, b=id str → reg = SET(cube) */
  OP_GETCUBES= 0x13, /* a=reg → reg = CUBES */
  OP_CMP     = 0x14, /* a=reg, imm=op(0eq 1ne 2lt 3le 4gt 5ge), b=reg → a = 0/1 */
  OP_JMP     = 0x15, /* imm = pc delta (signed) */
  OP_JZ      = 0x16, /* a=reg, imm=delta */
  OP_JNZ     = 0x17,
  OP_OS      = 0x18, /* spawn OS aspect cubes */
  OP_HOLD    = 0x19, /* imm = hold_flash */
  OP_VIZ     = 0x1A, /* a = str path optional */
  OP_WAIT    = 0x1B, /* imm = ms */
  OP_POSE    = 0x1C,
  OP_DECIDE  = 0x1D, /* a=id str optional · algocube digit → reg0 */
  OP_ENERGY  = 0x1E, /* a=reg, b=id str · centi-energy */
  OP_MOD     = 0x1F, /* a=reg, b=reg */
  OP_DIV     = 0x20,
  OP_NEG     = 0x21, /* a=reg */
  OP_AND     = 0x22,
  OP_OR      = 0x23,
  OP_XOR     = 0x24,
  OP_CALLH   = 0x25, /* host call imm=fnid */
  OP_FLOW_P  = 0x26, /* parallel flow imm=n */
};


/* One instruction = 8 bytes (matrix-word friendly) */
typedef struct cubalc_ins {
  uint8_t  op;
  uint8_t  a;
  uint8_t  b;
  uint8_t  c;
  int32_t  imm; /* immediates, deltas, proton, n, bit index */
} cubalc_ins;

typedef struct cubalc_image {
  uint32_t magic;
  uint16_t ver;
  uint16_t n_str;
  uint32_t n_ins;
  char     str[CUBALC_ISA_MAX_STRTAB][CUBALC_ISA_MAX_STR];
  cubalc_ins code[CUBALC_ISA_MAX_INS];
} cubalc_image;

typedef struct cubalc_vm {
  cubalc_chain ch;
  int64_t reg[CUBALC_ISA_MAX_REG];
  int pc;
  int hold_flash;
  int halted;
  int fatal;
  char err[160];
  cubalc_run_result *res;
  FILE *trace;
} cubalc_vm;

/* Assemble cubical [] human text → image (below C: product is bytecode) */
int cubalc_isa_assemble(const char *src, size_t n, cubalc_image *img, char *err, size_t errn);

/* Load / save .cblc cube machine images */
int cubalc_isa_save(const cubalc_image *img, const char *path);
int cubalc_isa_load(cubalc_image *img, const char *path);

/* Execute bytecode on the matrix machine */
int cubalc_isa_run(const cubalc_image *img, cubalc_run_result *out, FILE *trace);

/* Disassemble for humans who still need eyes */
int cubalc_isa_disasm(const cubalc_image *img, FILE *out);

#ifdef __cplusplus
}
#endif
#endif
