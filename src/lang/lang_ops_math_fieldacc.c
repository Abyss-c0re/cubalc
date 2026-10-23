/* CubalC lang — lang_ops_math_fieldacc.c
 * MATH field access/merge: pack/byte/half/word get-set · AND/OR/XOR/N* field merge
 * Peel from math residual (B_math_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_math_fieldacc.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_math_try_fieldacc(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* digit-3/0 stack pack+byte: SPACK16 SHI16 SLO16 SBYTE SLOBYTE SHIBYTE */
  if (kw(&L->cur,"SLOBYTE")||kw(&L->cur,"STACKLOBYTE")||
      kw(&L->cur,"SHIBYTE")||kw(&L->cur,"STACKHIBYTE")||
      kw(&L->cur,"SHI16")||kw(&L->cur,"SHIWORD")||kw(&L->cur,"STACKHI16")||
      kw(&L->cur,"SLO16")||kw(&L->cur,"SLOWORD")||kw(&L->cur,"STACKLO16")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (strcmp(op,"SLOBYTE")==0 || strcmp(op,"STACKLOBYTE")==0)
      r = (long)((unsigned long)a & 0xFFul);
    else if (strcmp(op,"SHIBYTE")==0 || strcmp(op,"STACKHIBYTE")==0)
      r = (long)(((unsigned long)a >> 8) & 0xFFul);
    else if (strcmp(op,"SHI16")==0 || strcmp(op,"SHIWORD")==0 || strcmp(op,"STACKHI16")==0)
      r = (long)(((unsigned int)a >> 16) & 0xFFFFu);
    else
      r = (long)((unsigned int)a & 0xFFFFu);
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SBYTE")||kw(&L->cur,"STACKBYTE")||kw(&L->cur,"SGETBYTE")){
    /* a i → i-th little-endian byte of a (i clamped 0..7) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i > 7) i = 7;
    long r = (long)(((unsigned long)a >> (unsigned)(i * 8)) & 0xFFul);
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 SETBYTE + ALIGN: SSETBYTE SALIGN SALIGNDN */
  if (kw(&L->cur,"SSETBYTE")||kw(&L->cur,"STACKSETBYTE")||kw(&L->cur,"SSETBY")){
    /* a field i → deposit 8-bit at byte index i (SSETB reserved for bit set) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    long field = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i > 7) i = 7;
    unsigned long base = (unsigned long)a;
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long shift = (unsigned long)(i * 8);
    long r = (long)((base & ~(0xFFul << shift)) | (f << shift));
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack imm byte field: SBYTEN · SSETBYTEN · SCLRBYTEN (imm dual of SBYTE/SSETBYTE) */
  if (kw(&L->cur,"SBYTEN")||kw(&L->cur,"STACKBYTEN")||kw(&L->cur,"SGETBYTEN")||
      kw(&L->cur,"BYTEN")||kw(&L->cur,"GETBYTEN")){
    /* SBYTEN n — TOS = little-endian byte n of TOS; n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    long r = (long)(((unsigned long)vm->stack[vm->sp - 1] >> (unsigned)(n * 8)) & 0xFFul);
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSETBYTEN")||kw(&L->cur,"STACKSETBYTEN")||kw(&L->cur,"SSETBYIMM")||
      kw(&L->cur,"SETBYTEN")||kw(&L->cur,"PUTBYTEN")){
    /* SSETBYTEN field n — deposit low 8 bits of field into byte n of TOS; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long shift = (unsigned long)(n * 8);
    long r = (long)((base & ~(0xFFul << shift)) | (f << shift));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLRBYTEN")||kw(&L->cur,"STACKCLRBYTEN")||kw(&L->cur,"SCLRBYIMM")||
      kw(&L->cur,"CLRBYTEN")||kw(&L->cur,"ZAPBYTEN")){
    /* SCLRBYTEN n — clear little-endian byte n of TOS; n clamped 0..7 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long shift = (unsigned long)(n * 8);
    long r = (long)(base & ~(0xFFul << shift));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack imm 16-bit halfword field: SWORDN · SSET16N · SCLR16N (complete nibble/byte ladder) */
  if (kw(&L->cur,"SWORDN")||kw(&L->cur,"STACKWORDN")||kw(&L->cur,"SGET16N")||
      kw(&L->cur,"SHALFN")||kw(&L->cur,"GET16N")||kw(&L->cur,"WORDN")||
      kw(&L->cur,"SGETHALF")){
    /* SWORDN n — TOS = little-endian 16-bit halfword n; n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    long r = (long)(((unsigned long)vm->stack[vm->sp - 1] >> (unsigned)(n * 16)) & 0xFFFFul);
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSET16N")||kw(&L->cur,"STACKSET16N")||kw(&L->cur,"SSETWORDN")||
      kw(&L->cur,"SET16N")||kw(&L->cur,"PUT16N")||kw(&L->cur,"SSETHALFN")||
      kw(&L->cur,"SSETWIMM")){
    /* SSET16N field n — deposit low 16 bits of field into halfword n of TOS; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long shift = (unsigned long)(n * 16);
    long r = (long)((base & ~(0xFFFFul << shift)) | (f << shift));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLR16N")||kw(&L->cur,"STACKCLR16N")||kw(&L->cur,"SCLRWORDN")||
      kw(&L->cur,"CLR16N")||kw(&L->cur,"ZAP16N")||kw(&L->cur,"SCLRHAFN")||
      kw(&L->cur,"SCLRWIMM")){
    /* SCLR16N n — clear little-endian halfword n of TOS; n clamped 0..3 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long shift = (unsigned long)(n * 16);
    long r = (long)(base & ~(0xFFFFul << shift));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack imm 32-bit field: SGET32N · SSET32N · SCLR32N (complete nibble/byte/half/word ladder) */
  if (kw(&L->cur,"SGET32N")||kw(&L->cur,"STACKGET32N")||kw(&L->cur,"SWORD32N")||
      kw(&L->cur,"GET32N")||kw(&L->cur,"SLOW32N")||kw(&L->cur,"SDWORDN")||
      kw(&L->cur,"SGETDWN")||kw(&L->cur,"WORD32N")){
    /* SGET32N n — TOS = little-endian 32-bit word n; n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    long r = (long)(((unsigned long)vm->stack[vm->sp - 1] >> (unsigned)(n * 32)) & 0xFFFFFFFFul);
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSET32N")||kw(&L->cur,"STACKSET32N")||kw(&L->cur,"SSETWORD32N")||
      kw(&L->cur,"SET32N")||kw(&L->cur,"PUT32N")||kw(&L->cur,"SSETDWN")||
      kw(&L->cur,"SSETW32IMM")){
    /* SSET32N field n — deposit low 32 bits of field into word n of TOS; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long shift = (unsigned long)(n * 32);
    long r = (long)((base & ~(0xFFFFFFFFul << shift)) | (f << shift));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLR32N")||kw(&L->cur,"STACKCLR32N")||kw(&L->cur,"SCLRWORD32N")||
      kw(&L->cur,"CLR32N")||kw(&L->cur,"ZAP32N")||kw(&L->cur,"SCLRDWN")||
      kw(&L->cur,"SCLRW32IMM")){
    /* SCLR32N n — clear little-endian 32-bit word n of TOS; n clamped 0..1 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long shift = (unsigned long)(n * 32);
    long r = (long)(base & ~(0xFFFFFFFFul << shift));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack imm 32-bit field bitwise: SAND32N · SOR32N · SXOR32N (merge into word n; after SSET32N) */
  if (kw(&L->cur,"SAND32N")||kw(&L->cur,"STACKAND32N")||kw(&L->cur,"AND32N")||
      kw(&L->cur,"SANDWN")||kw(&L->cur,"SANDWIMM")||kw(&L->cur,"SKEEP32N")){
    /* SAND32N field n — word n of TOS &= field; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = ((base >> sh) & 0xFFFFFFFFul) & f;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SOR32N")||kw(&L->cur,"STACKOR32N")||kw(&L->cur,"OR32N")||
      kw(&L->cur,"SORWN")||kw(&L->cur,"SORWIMM")||kw(&L->cur,"SSETOR32N")){
    /* SOR32N field n — word n of TOS |= field; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = ((base >> sh) & 0xFFFFFFFFul) | f;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXOR32N")||kw(&L->cur,"STACKXOR32N")||kw(&L->cur,"XOR32N")||
      kw(&L->cur,"SXORWN")||kw(&L->cur,"SXORWIMM")||kw(&L->cur,"SFLIP32N")){
    /* SXOR32N field n — word n of TOS ^= field; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = ((base >> sh) & 0xFFFFFFFFul) ^ f;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack imm inverted 32-bit field: SNAND32N · SNOR32N · SXNOR32N (complete after SAND32N; ladder after SNAND16N) */
  if (kw(&L->cur,"SNAND32N")||kw(&L->cur,"STACKNAND32N")||kw(&L->cur,"NAND32N")||
      kw(&L->cur,"SNANDWN")||kw(&L->cur,"SNANDWIMM")||kw(&L->cur,"SINVERTAND32N")){
    /* SNAND32N field n — word n of TOS = ~(word & field) & 0xFFFFFFFF; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = (~(((base >> sh) & 0xFFFFFFFFul) & f)) & 0xFFFFFFFFul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNOR32N")||kw(&L->cur,"STACKNOR32N")||kw(&L->cur,"NOR32N")||
      kw(&L->cur,"SNORWN")||kw(&L->cur,"SNORWIMM")||kw(&L->cur,"SINVERTOR32N")){
    /* SNOR32N field n — word n of TOS = ~(word | field) & 0xFFFFFFFF; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = (~(((base >> sh) & 0xFFFFFFFFul) | f)) & 0xFFFFFFFFul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNOR32N")||kw(&L->cur,"STACKXNOR32N")||kw(&L->cur,"XNOR32N")||
      kw(&L->cur,"SXNORWN")||kw(&L->cur,"SXNORWIMM")||kw(&L->cur,"SEQUIV32N")){
    /* SXNOR32N field n — word n of TOS = ~(word ^ field) & 0xFFFFFFFF; n clamped 0..1 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long sh = (unsigned long)(n * 32);
    unsigned long w = (~(((base >> sh) & 0xFFFFFFFFul) ^ f)) & 0xFFFFFFFFul;
    long r = (long)((base & ~(0xFFFFFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack imm 16-bit field bitwise: SAND16N · SOR16N · SXOR16N (merge into halfword n; after SSET16N/SAND32N) */
  if (kw(&L->cur,"SAND16N")||kw(&L->cur,"STACKAND16N")||kw(&L->cur,"AND16N")||
      kw(&L->cur,"SANDHN16")||kw(&L->cur,"SANDHIMM")||kw(&L->cur,"SKEEPH16N")){
    /* SAND16N field n — halfword n of TOS &= field; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = ((base >> sh) & 0xFFFFul) & f;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SOR16N")||kw(&L->cur,"STACKOR16N")||kw(&L->cur,"OR16N")||
      kw(&L->cur,"SORHN16")||kw(&L->cur,"SORHIMM")||kw(&L->cur,"SSETOR16N")){
    /* SOR16N field n — halfword n of TOS |= field; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = ((base >> sh) & 0xFFFFul) | f;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXOR16N")||kw(&L->cur,"STACKXOR16N")||kw(&L->cur,"XOR16N")||
      kw(&L->cur,"SXORHN16")||kw(&L->cur,"SXORHIMM")||kw(&L->cur,"SFLIP16N")){
    /* SXOR16N field n — halfword n of TOS ^= field; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = ((base >> sh) & 0xFFFFul) ^ f;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack imm inverted 16-bit field: SNAND16N · SNOR16N · SXNOR16N (complete after SAND16N; ladder after SNAND8N) */
  if (kw(&L->cur,"SNAND16N")||kw(&L->cur,"STACKNAND16N")||kw(&L->cur,"NAND16N")||
      kw(&L->cur,"SNANDHN16")||kw(&L->cur,"SNANDHIMM")||kw(&L->cur,"SINVERTAND16N")){
    /* SNAND16N field n — halfword n of TOS = ~(half & field) & 0xFFFF; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = (~(((base >> sh) & 0xFFFFul) & f)) & 0xFFFFul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNOR16N")||kw(&L->cur,"STACKNOR16N")||kw(&L->cur,"NOR16N")||
      kw(&L->cur,"SNORHN16")||kw(&L->cur,"SNORHIMM")||kw(&L->cur,"SINVERTOR16N")){
    /* SNOR16N field n — halfword n of TOS = ~(half | field) & 0xFFFF; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = (~(((base >> sh) & 0xFFFFul) | f)) & 0xFFFFul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNOR16N")||kw(&L->cur,"STACKXNOR16N")||kw(&L->cur,"XNOR16N")||
      kw(&L->cur,"SXNORHN16")||kw(&L->cur,"SXNORHIMM")||kw(&L->cur,"SEQUIV16N")){
    /* SXNOR16N field n — halfword n of TOS = ~(half ^ field) & 0xFFFF; n clamped 0..3 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long sh = (unsigned long)(n * 16);
    unsigned long w = (~(((base >> sh) & 0xFFFFul) ^ f)) & 0xFFFFul;
    long r = (long)((base & ~(0xFFFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack imm 8-bit field bitwise: SAND8N · SOR8N · SXOR8N (merge into byte n; complete after SAND16N/SAND32N) */
  if (kw(&L->cur,"SAND8N")||kw(&L->cur,"STACKAND8N")||kw(&L->cur,"AND8N")||
      kw(&L->cur,"SANDBN")||kw(&L->cur,"SANDBIMM")||kw(&L->cur,"SKEEP8N")){
    /* SAND8N field n — byte n of TOS &= field; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = ((base >> sh) & 0xFFul) & f;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SOR8N")||kw(&L->cur,"STACKOR8N")||kw(&L->cur,"OR8N")||
      kw(&L->cur,"SORBN")||kw(&L->cur,"SORBIMM")||kw(&L->cur,"SSETOR8N")){
    /* SOR8N field n — byte n of TOS |= field; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = ((base >> sh) & 0xFFul) | f;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXOR8N")||kw(&L->cur,"STACKXOR8N")||kw(&L->cur,"XOR8N")||
      kw(&L->cur,"SXORBN")||kw(&L->cur,"SXORBIMM")||kw(&L->cur,"SFLIP8N")){
    /* SXOR8N field n — byte n of TOS ^= field; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = ((base >> sh) & 0xFFul) ^ f;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack imm inverted 8-bit field: SNAND8N · SNOR8N · SXNOR8N (complete after SAND8N/SOR8N/SXOR8N; ladder after SNAND4N) */
  if (kw(&L->cur,"SNAND8N")||kw(&L->cur,"STACKNAND8N")||kw(&L->cur,"NAND8N")||
      kw(&L->cur,"SNANDBN")||kw(&L->cur,"SNANDBIMM")||kw(&L->cur,"SINVERTAND8N")){
    /* SNAND8N field n — byte n of TOS = ~(byte & field) & 0xFF; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = (~(((base >> sh) & 0xFFul) & f)) & 0xFFul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNOR8N")||kw(&L->cur,"STACKNOR8N")||kw(&L->cur,"NOR8N")||
      kw(&L->cur,"SNORBN")||kw(&L->cur,"SNORBIMM")||kw(&L->cur,"SINVERTOR8N")){
    /* SNOR8N field n — byte n of TOS = ~(byte | field) & 0xFF; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = (~(((base >> sh) & 0xFFul) | f)) & 0xFFul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNOR8N")||kw(&L->cur,"STACKXNOR8N")||kw(&L->cur,"XNOR8N")||
      kw(&L->cur,"SXNORBN")||kw(&L->cur,"SXNORBIMM")||kw(&L->cur,"SEQUIV8N")){
    /* SXNOR8N field n — byte n of TOS = ~(byte ^ field) & 0xFF; n clamped 0..7 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long sh = (unsigned long)(n * 8);
    unsigned long w = (~(((base >> sh) & 0xFFul) ^ f)) & 0xFFul;
    long r = (long)((base & ~(0xFFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
