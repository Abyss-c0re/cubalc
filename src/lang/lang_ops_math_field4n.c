/* CubalC lang — lang_ops_math_field4n.c
 * MATH 4-bit field ladder: SAND4N…S0GE4N · unary/shift/arith/cmp/signed
 * Peel from math residual (B_math_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_math_field4n.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_math_try_field4n(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* digit-7 stack imm 4-bit field bitwise: SAND4N · SOR4N · SXOR4N (merge into nibble n; complete field bitwise ladder) */
  if (kw(&L->cur,"SAND4N")||kw(&L->cur,"STACKAND4N")||kw(&L->cur,"AND4N")||
      kw(&L->cur,"SANDNIBN")||kw(&L->cur,"SANDNIMM")||kw(&L->cur,"SKEEP4N")){
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = ((base >> sh) & 0xFul) & f;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SOR4N")||kw(&L->cur,"STACKOR4N")||kw(&L->cur,"OR4N")||
      kw(&L->cur,"SORNIBN")||kw(&L->cur,"SORNIMM")||kw(&L->cur,"SSETOR4N")){
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = ((base >> sh) & 0xFul) | f;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXOR4N")||kw(&L->cur,"STACKXOR4N")||kw(&L->cur,"XOR4N")||
      kw(&L->cur,"SXORNIBN")||kw(&L->cur,"SXORNIMM")||kw(&L->cur,"SFLIP4N")){
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = ((base >> sh) & 0xFul) ^ f;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack imm inverted 4-bit field: SNAND4N · SNOR4N · SXNOR4N (complete after SAND4N/SOR4N/SXOR4N) */
  if (kw(&L->cur,"SNAND4N")||kw(&L->cur,"STACKNAND4N")||kw(&L->cur,"NAND4N")||
      kw(&L->cur,"SNANDNIBN")||kw(&L->cur,"SNANDNIMM")||kw(&L->cur,"SINVERTAND4N")){
    /* SNAND4N field n — nibble n of TOS = ~(nibble & field) & 0xF; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (~(((base >> sh) & 0xFul) & f)) & 0xFul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNOR4N")||kw(&L->cur,"STACKNOR4N")||kw(&L->cur,"NOR4N")||
      kw(&L->cur,"SNORNIBN")||kw(&L->cur,"SNORNIMM")||kw(&L->cur,"SINVERTOR4N")){
    /* SNOR4N field n — nibble n of TOS = ~(nibble | field) & 0xF; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (~(((base >> sh) & 0xFul) | f)) & 0xFul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNOR4N")||kw(&L->cur,"STACKXNOR4N")||kw(&L->cur,"XNOR4N")||
      kw(&L->cur,"SXNORNIBN")||kw(&L->cur,"SXNORNIMM")||kw(&L->cur,"SEQUIV4N")){
    /* SXNOR4N field n — nibble n of TOS = ~(nibble ^ field) & 0xF; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (~(((base >> sh) & 0xFul) ^ f)) & 0xFul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack imm 4-bit field unary: SNOT4N · SROL4N · SROR4N
   * (nibble-field dual of NOT4CELL/ROL4CELL after SNAND4N; complete stack nibble unary+rotate) */
  if (kw(&L->cur,"SNOT4N")||kw(&L->cur,"STACKNOT4N")||kw(&L->cur,"NOT4N")||
      kw(&L->cur,"SNOTNIBN")||kw(&L->cur,"SINV4N")||kw(&L->cur,"SINVERT4N")){
    /* SNOT4N n — nibble n of TOS = ~nibble & 0xF; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (~((base >> sh) & 0xFul)) & 0xFul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROL4N")||kw(&L->cur,"STACKROL4N")||kw(&L->cur,"ROL4N")||
      kw(&L->cur,"SROTL4N")||kw(&L->cur,"SROLNIBN")||kw(&L->cur,"SROTNIBL")){
    /* SROL4N k n — rotl4 nibble n of TOS by k&3; n clamped 0..15 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (base >> sh) & 0xFul;
    unsigned long rot = (kk == 0) ? w : (((w << (unsigned)kk) | (w >> (unsigned)(4 - kk))) & 0xFul);
    long r = (long)((base & ~(0xFul << sh)) | (rot << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROR4N")||kw(&L->cur,"STACKROR4N")||kw(&L->cur,"ROR4N")||
      kw(&L->cur,"SROTR4N")||kw(&L->cur,"SRORNIBN")||kw(&L->cur,"SROTNIBR")){
    /* SROR4N k n — rotr4 nibble n of TOS by k&3; n clamped 0..15 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    kk &= 3;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (base >> sh) & 0xFul;
    unsigned long rot = (kk == 0) ? w : (((w >> (unsigned)kk) | (w << (unsigned)(4 - kk))) & 0xFul);
    long r = (long)((base & ~(0xFul << sh)) | (rot << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack imm 4-bit field shift: SSHL4N · SSHR4N · SSAR4N
   * (nibble-field dual of SHL4CELL/SHR4CELL/SAR4CELL after SROL4N; complete stack nibble shift) */
  if (kw(&L->cur,"SSHL4N")||kw(&L->cur,"STACKSHL4N")||kw(&L->cur,"SHL4N")||
      kw(&L->cur,"SLSH4N")||kw(&L->cur,"SSHLNIBN")||kw(&L->cur,"SSHIFTNIBL")){
    /* SSHL4N k n — nibble n of TOS = (uint4)<<k (k>=4 → 0); n clamped 0..15 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (base >> sh) & 0xFul;
    unsigned long nw = (kk >= 4) ? 0ul : ((w << (unsigned)kk) & 0xFul);
    long r = (long)((base & ~(0xFul << sh)) | (nw << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSHR4N")||kw(&L->cur,"STACKSHR4N")||kw(&L->cur,"SHR4N")||
      kw(&L->cur,"SLSHR4N")||kw(&L->cur,"SSHRNIBN")||kw(&L->cur,"SSHIFTNIBR")){
    /* SSHR4N k n — nibble n of TOS = (uint4)>>k logical (k>=4 → 0); n clamped 0..15 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (base >> sh) & 0xFul;
    unsigned long nw = (kk >= 4) ? 0ul : (w >> (unsigned)kk);
    long r = (long)((base & ~(0xFul << sh)) | (nw << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSAR4N")||kw(&L->cur,"STACKSAR4N")||kw(&L->cur,"SAR4N")||
      kw(&L->cur,"SASHR4N")||kw(&L->cur,"SSARNIBN")||kw(&L->cur,"SSARSHIFTNIB")){
    /* SSAR4N k n — nibble n of TOS = (int4)>>k arithmetic (k>=4 → all sign); n clamped 0..15 */
    lex_next(L);
    long k = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long va = (long)((base >> sh) & 0xFul);
    if (va & 0x8) va -= 16; /* sign-extend nibble */
    long shifted;
    if (kk >= 4) shifted = (va < 0) ? -1L : 0L;
    else shifted = va >> kk;
    unsigned long nw = (unsigned long)shifted & 0xFul;
    long r = (long)((base & ~(0xFul << sh)) | (nw << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack imm 4-bit field metrics: SBITREV4N · SPOPCNT4N · SPARITY4N
   * (nibble-field dual of BITREV4CELL/POPCNT4CELL/PARITY4CELL after SSHL4N; complete stack nibble metrics) */
  if (kw(&L->cur,"SBITREV4N")||kw(&L->cur,"STACKBITREV4N")||kw(&L->cur,"BITREV4N")||
      kw(&L->cur,"SBREV4N")||kw(&L->cur,"SREV4N")||kw(&L->cur,"SBITREVNIBN")){
    /* SBITREV4N n — nibble n of TOS = bitrev4(nibble); n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (base >> sh) & 0xFul;
    unsigned long rv = 0;
    for (int b = 0; b < 4; b++){
      rv = (rv << 1) | (w & 1u);
      w >>= 1;
    }
    long r = (long)((base & ~(0xFul << sh)) | (rv << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPOPCNT4N")||kw(&L->cur,"STACKPOPCNT4N")||kw(&L->cur,"POPCNT4N")||
      kw(&L->cur,"SPCNT4N")||kw(&L->cur,"SPOPNIBN")||kw(&L->cur,"SNIBPOPN")){
    /* SPOPCNT4N n — nibble n of TOS = popcount(nibble); n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (base >> sh) & 0xFul;
    unsigned long pc = 0;
    while (w){ pc += (w & 1u); w >>= 1; }
    long r = (long)((base & ~(0xFul << sh)) | (pc << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPARITY4N")||kw(&L->cur,"STACKPARITY4N")||kw(&L->cur,"PARITY4N")||
      kw(&L->cur,"SXORRED4N")||kw(&L->cur,"SPARITYNIBN")||kw(&L->cur,"SNIBPARN")){
    /* SPARITY4N n — nibble n of TOS = xor-reduce(nibble) in low bit; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (base >> sh) & 0xFul;
    unsigned long pr = 0;
    while (w){ pr ^= (w & 1u); w >>= 1; }
    long r = (long)((base & ~(0xFul << sh)) | (pr << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack imm 4-bit field zeros+signed: SCLZ4N · SCTZ4N · SNEG4N
   * (nibble-field dual of CLZ4CELL/CTZ4CELL/NEG4CELL after SPARITY4N; complete stack nibble zeros+negate) */
  if (kw(&L->cur,"SCLZ4N")||kw(&L->cur,"STACKCLZ4N")||kw(&L->cur,"CLZ4N")||
      kw(&L->cur,"SNLZ4N")||kw(&L->cur,"SCLZNIBN")||kw(&L->cur,"SNIBCLZN")){
    /* SCLZ4N n — nibble n of TOS = clz4(nibble); 0 → 4; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    unsigned int w = (unsigned int)((base >> sh) & 0xFul);
    unsigned long cz = 0;
    if (w == 0) cz = 4;
    else {
      for (int b = 3; b >= 0; b--){
        if (w & (1u << (unsigned)b)) break;
        cz++;
      }
    }
    long r = (long)((base & ~(0xFul << sh)) | (cz << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCTZ4N")||kw(&L->cur,"STACKCTZ4N")||kw(&L->cur,"CTZ4N")||
      kw(&L->cur,"SNTZ4N")||kw(&L->cur,"SCTZNIBN")||kw(&L->cur,"SNIBCTZN")){
    /* SCTZ4N n — nibble n of TOS = ctz4(nibble); 0 → 4; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    unsigned int w = (unsigned int)((base >> sh) & 0xFul);
    unsigned long tz = 0;
    if (w == 0) tz = 4;
    else {
      while ((w & 1u) == 0){ tz++; w >>= 1; }
    }
    long r = (long)((base & ~(0xFul << sh)) | (tz << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNEG4N")||kw(&L->cur,"STACKNEG4N")||kw(&L->cur,"NEG4N")||
      kw(&L->cur,"SINEG4N")||kw(&L->cur,"SNEGNIBN")||kw(&L->cur,"SNIBNEGN")){
    /* SNEG4N n — nibble n of TOS = -(int4)nibble as uint4; min int4 -8 stays -8 (0x8) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long va = (long)((base >> sh) & 0xFul);
    if (va & 0x8) va -= 16;
    long nr;
    if (va == -8L) nr = -8L; /* keep min int4 */
    else nr = -va;
    unsigned long nv = (unsigned long)(nr & 0xF);
    long r = (long)((base & ~(0xFul << sh)) | (nv << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack imm 4-bit field abs+extend: SABS4N · SSEXT4N · SZEXT4N
   * (nibble-field dual of ABS4CELL/SEXT4CELL/ZEXT4CELL after SNEG4N; complete stack nibble signed extract) */
  if (kw(&L->cur,"SABS4N")||kw(&L->cur,"STACKABS4N")||kw(&L->cur,"ABS4N")||
      kw(&L->cur,"SIABS4N")||kw(&L->cur,"SABSNIBN")||kw(&L->cur,"SNIBABSN")){
    /* SABS4N n — nibble n of TOS = abs(int4); min int4 -8 → +8 (0x8); n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long va = (long)((base >> sh) & 0xFul);
    if (va & 0x8) va -= 16;
    long ar = (va < 0) ? -va : va; /* -8 → 8 fits in nibble */
    unsigned long nv = (unsigned long)(ar & 0xF);
    long r = (long)((base & ~(0xFul << sh)) | (nv << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEXT4N")||kw(&L->cur,"STACKSEXT4N")||kw(&L->cur,"SEXT4N")||
      kw(&L->cur,"SSIGNEXT4N")||kw(&L->cur,"SSEXTNIBN")||kw(&L->cur,"SNIBSEXTN")){
    /* SSEXT4N n — TOS = sign-extend nibble n of TOS to full width; n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long va = (long)((base >> sh) & 0xFul);
    if (va & 0x8) va -= 16; /* sign-extend int4 */
    long r = va;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SZEXT4N")||kw(&L->cur,"STACKZEXT4N")||kw(&L->cur,"ZEXT4N")||
      kw(&L->cur,"SZEROEXT4N")||kw(&L->cur,"SZEXTNIBN")||kw(&L->cur,"SNIBZEXTN")){
    /* SZEXT4N n — TOS = zero-extend nibble n of TOS (low4 only); n clamped 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long r = (long)((base >> sh) & 0xFul);
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack imm 4-bit field arith merge: SADD4N · SSUB4N · SMUL4N
   * (nibble-field dual of bitwise SAND4N plane; wrap uint4 ALU foundation after SZEXT4N) */
  if (kw(&L->cur,"SADD4N")||kw(&L->cur,"STACKADD4N")||kw(&L->cur,"ADD4N")||
      kw(&L->cur,"SADDNIBN")||kw(&L->cur,"SADDNIMM")||kw(&L->cur,"SINC4N")){
    /* SADD4N field n — nibble n of TOS = (nibble + field) & 0xF; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (((base >> sh) & 0xFul) + f) & 0xFul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSUB4N")||kw(&L->cur,"STACKSUB4N")||kw(&L->cur,"SUB4N")||
      kw(&L->cur,"SSUBNIBN")||kw(&L->cur,"SSUBNIMM")||kw(&L->cur,"SDEC4N")){
    /* SSUB4N field n — nibble n of TOS = (nibble - field) & 0xF; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (((base >> sh) & 0xFul) - f) & 0xFul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMUL4N")||kw(&L->cur,"STACKMUL4N")||kw(&L->cur,"MUL4N")||
      kw(&L->cur,"SMULNIBN")||kw(&L->cur,"SMULNIMM")||kw(&L->cur,"STIMES4N")){
    /* SMUL4N field n — nibble n of TOS = (nibble * field) & 0xF; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long w = (((base >> sh) & 0xFul) * f) & 0xFul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack imm 4-bit field div/mod/min: SDIV4N · SMOD4N · SMIN4N
   * (complete uint4 field ALU after SADD4N/SSUB4N/SMUL4N; field 0 → div/mod 0) */
  if (kw(&L->cur,"SDIV4N")||kw(&L->cur,"STACKDIV4N")||kw(&L->cur,"DIV4N")||
      kw(&L->cur,"SDIVNIBN")||kw(&L->cur,"SDIVNIMM")||kw(&L->cur,"SQUO4N")){
    /* SDIV4N field n — nibble n of TOS = nibble / field (field 0 → 0); n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long v = (base >> sh) & 0xFul;
    unsigned long w = (f == 0) ? 0ul : (v / f);
    long r = (long)((base & ~(0xFul << sh)) | ((w & 0xFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMOD4N")||kw(&L->cur,"STACKMOD4N")||kw(&L->cur,"MOD4N")||
      kw(&L->cur,"SMODNIBN")||kw(&L->cur,"SMODNIMM")||kw(&L->cur,"SREM4N")){
    /* SMOD4N field n — nibble n of TOS = nibble % field (field 0 → 0); n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long v = (base >> sh) & 0xFul;
    unsigned long w = (f == 0) ? 0ul : (v % f);
    long r = (long)((base & ~(0xFul << sh)) | ((w & 0xFul) << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMIN4N")||kw(&L->cur,"STACKMIN4N")||kw(&L->cur,"MIN4N")||
      kw(&L->cur,"SMINNIBN")||kw(&L->cur,"SMINNIMM")||kw(&L->cur,"SLE4N")){
    /* SMIN4N field n — nibble n of TOS = min(nibble, field); n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long v = (base >> sh) & 0xFul;
    unsigned long w = (v < f) ? v : f;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack imm 4-bit field max+eq: SMAX4N · SEQ4N · SNE4N
   * (complete min/max + equality compare plane after SMIN4N; bool writes 0/1 into nibble) */
  if (kw(&L->cur,"SMAX4N")||kw(&L->cur,"STACKMAX4N")||kw(&L->cur,"MAX4N")||
      kw(&L->cur,"SMAXNIBN")||kw(&L->cur,"SMAXNIMM")||kw(&L->cur,"SGE4N")){
    /* SMAX4N field n — nibble n of TOS = max(nibble, field); n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long v = (base >> sh) & 0xFul;
    unsigned long w = (v > f) ? v : f;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SEQ4N")||kw(&L->cur,"STACKEQ4N")||kw(&L->cur,"EQ4N")||
      kw(&L->cur,"SEQNIBN")||kw(&L->cur,"SEQNIMM")||kw(&L->cur,"SCMPEQ4N")){
    /* SEQ4N field n — nibble n of TOS = (nibble == field) ? 1 : 0; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long v = (base >> sh) & 0xFul;
    unsigned long w = (v == f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNE4N")||kw(&L->cur,"STACKNE4N")||kw(&L->cur,"NE4N")||
      kw(&L->cur,"SNENIBN")||kw(&L->cur,"SNENIMM")||kw(&L->cur,"SCMPNE4N")){
    /* SNE4N field n — nibble n of TOS = (nibble != field) ? 1 : 0; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long v = (base >> sh) & 0xFul;
    unsigned long w = (v != f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack imm 4-bit field ordered-cmp: SLT4N · SGT4N · SLTE4N
   * (nibble ladder of SLT8N/SGT8N/SLTE8N; unsigned predicates after SEQ4N/SNE4N) */
  if (kw(&L->cur,"SLT4N")||kw(&L->cur,"STACKLT4N")||kw(&L->cur,"LT4N")||
      kw(&L->cur,"SCMPLT4N")||kw(&L->cur,"SLTNIBN")||kw(&L->cur,"SLT4IMM")){
    /* SLT4N field n — nibble n of TOS = (nib < field) ? 1 : 0; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long v = (base >> sh) & 0xFul;
    unsigned long w = (v < f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGT4N")||kw(&L->cur,"STACKGT4N")||kw(&L->cur,"GT4N")||
      kw(&L->cur,"SCMPGT4N")||kw(&L->cur,"SGTNIBN")||kw(&L->cur,"SGT4IMM")){
    /* SGT4N field n — nibble n of TOS = (nib > field) ? 1 : 0; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long v = (base >> sh) & 0xFul;
    unsigned long w = (v > f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SLTE4N")||kw(&L->cur,"STACKLTE4N")||kw(&L->cur,"LTE4N")||
      kw(&L->cur,"SCMPLE4N")||kw(&L->cur,"SLEQ4N")||kw(&L->cur,"SLTENIBN")||
      kw(&L->cur,"SLTE4IMM")){
    /* SLTE4N field n — nibble n of TOS = (nib <= field) ? 1 : 0; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long v = (base >> sh) & 0xFul;
    unsigned long w = (v <= f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack imm 4-bit field GTE: SGTE4N (nibble ladder after SLTE4N; complete stack GTE plane) */
  if (kw(&L->cur,"SGTE4N")||kw(&L->cur,"STACKGTE4N")||kw(&L->cur,"GTE4N")||
      kw(&L->cur,"SCMPGE4N")||kw(&L->cur,"SGEQ4N")||kw(&L->cur,"SGTENIBN")||
      kw(&L->cur,"SGTE4IMM")){
    /* SGTE4N field n — nibble n of TOS = (nib >= field) ? 1 : 0; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long v = (base >> sh) & 0xFul;
    unsigned long w = (v >= f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack imm 4-bit field signed ordered-cmp: SLTS4N · SGTS4N · SLTES4N · SGTES4N
   * (signed int4 plane after unsigned SLT/SGT/SLTE/SGTE; nibble as -8..7) */
  if (kw(&L->cur,"SLTS4N")||kw(&L->cur,"STACKLTS4N")||kw(&L->cur,"LTS4N")||
      kw(&L->cur,"SCMPLTS4N")||kw(&L->cur,"SLTSSIGN4N")||kw(&L->cur,"SLTS4IMM")){
    /* SLTS4N field n — nibble n = (int4(nib) < int4(field)) ? 1 : 0; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long v = (long)((base >> sh) & 0xFul);
    if (v & 0x8) v -= 16;
    long f = (long)((unsigned long)field & 0xFul);
    if (f & 0x8) f -= 16;
    unsigned long w = (v < f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGTS4N")||kw(&L->cur,"STACKGTS4N")||kw(&L->cur,"GTS4N")||
      kw(&L->cur,"SCMPGTS4N")||kw(&L->cur,"SGTSSIGN4N")||kw(&L->cur,"SGTS4IMM")){
    /* SGTS4N field n — nibble n = (int4(nib) > int4(field)) ? 1 : 0; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long v = (long)((base >> sh) & 0xFul);
    if (v & 0x8) v -= 16;
    long f = (long)((unsigned long)field & 0xFul);
    if (f & 0x8) f -= 16;
    unsigned long w = (v > f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SLTES4N")||kw(&L->cur,"STACKLTES4N")||kw(&L->cur,"LTES4N")||
      kw(&L->cur,"SCMPLES4N")||kw(&L->cur,"SLEQS4N")||kw(&L->cur,"SLTES4IMM")){
    /* SLTES4N field n — nibble n = (int4(nib) <= int4(field)) ? 1 : 0; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long v = (long)((base >> sh) & 0xFul);
    if (v & 0x8) v -= 16;
    long f = (long)((unsigned long)field & 0xFul);
    if (f & 0x8) f -= 16;
    unsigned long w = (v <= f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGTES4N")||kw(&L->cur,"STACKGTES4N")||kw(&L->cur,"GTES4N")||
      kw(&L->cur,"SCMPGES4N")||kw(&L->cur,"SGEQS4N")||kw(&L->cur,"SGTES4IMM")){
    /* SGTES4N field n — nibble n = (int4(nib) >= int4(field)) ? 1 : 0; n clamped 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long v = (long)((base >> sh) & 0xFul);
    if (v & 0x8) v -= 16;
    long f = (long)((unsigned long)field & 0xFul);
    if (f & 0x8) f -= 16;
    unsigned long w = (v >= f) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack imm 4-bit field signed min/max: SMINS4N · SMAXS4N
   * (nibble ladder of SMINS8N plane; signed select after SLTS4N; completes signed min/max all widths) */
  if (kw(&L->cur,"SMINS4N")||kw(&L->cur,"STACKMINS4N")||kw(&L->cur,"MINS4N")||
      kw(&L->cur,"SMINSIGN4N")||kw(&L->cur,"SMINS4IMM")||kw(&L->cur,"SMINSI4N")){
    /* SMINS4N field n — nibble n = min_signed(int4(nib), int4(field)); n 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long v = (long)((base >> sh) & 0xFul);
    if (v & 0x8) v -= 16;
    long f = (long)((unsigned long)field & 0xFul);
    if (f & 0x8) f -= 16;
    long m = (v < f) ? v : f;
    unsigned long w = (unsigned long)m & 0xFul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMAXS4N")||kw(&L->cur,"STACKMAXS4N")||kw(&L->cur,"MAXS4N")||
      kw(&L->cur,"SMAXSIGN4N")||kw(&L->cur,"SMAXS4IMM")||kw(&L->cur,"SMAXSI4N")){
    /* SMAXS4N field n — nibble n = max_signed(int4(nib), int4(field)); n 0..15 */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long v = (long)((base >> sh) & 0xFul);
    if (v & 0x8) v -= 16;
    long f = (long)((unsigned long)field & 0xFul);
    if (f & 0x8) f -= 16;
    long m = (v > f) ? v : f;
    unsigned long w = (unsigned long)m & 0xFul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack imm 4-bit field signed clamp+between: SCLAMPS4N · SBETWEENS4N
   * (nibble ladder of SCLAMPS8N; completes signed range plane all widths) */
  if (kw(&L->cur,"SCLAMPS4N")||kw(&L->cur,"STACKCLAMPS4N")||kw(&L->cur,"CLAMPS4N")||
      kw(&L->cur,"SCLAMPSIGN4N")||kw(&L->cur,"SCLAMPS4IMM")||kw(&L->cur,"SBOUNDS4N")){
    /* SCLAMPS4N lo hi n — nibble n = clamp_signed(int4(nib),[lo,hi]); n 0..15 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    long slo = (long)((unsigned long)lo & 0xFul); if (slo & 0x8) slo -= 16;
    long shi = (long)((unsigned long)hi & 0xFul); if (shi & 0x8) shi -= 16;
    if (slo > shi){ long t = slo; slo = shi; shi = t; }
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long v = (long)((base >> sh) & 0xFul); if (v & 0x8) v -= 16;
    if (v < slo) v = slo;
    if (v > shi) v = shi;
    unsigned long w = (unsigned long)v & 0xFul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SBETWEENS4N")||kw(&L->cur,"STACKBETWEENS4N")||kw(&L->cur,"BETWEENS4N")||
      kw(&L->cur,"SINRANGES4N")||kw(&L->cur,"SBETWEENS4IMM")||kw(&L->cur,"SINRANGESIGN4N")){
    /* SBETWEENS4N lo hi n — nibble n = (lo <= int4(nib) <= hi) ? 1 : 0; n 0..15 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    long slo = (long)((unsigned long)lo & 0xFul); if (slo & 0x8) slo -= 16;
    long shi = (long)((unsigned long)hi & 0xFul); if (shi & 0x8) shi -= 16;
    if (slo > shi){ long t = slo; slo = shi; shi = t; }
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long v = (long)((base >> sh) & 0xFul); if (v & 0x8) v -= 16;
    unsigned long w = (v >= slo && v <= shi) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 stack imm 4-bit field signed WITHIN: SWITHINS4N (nibble ladder; completes WITHIN plane) */
  if (kw(&L->cur,"SWITHINS4N")||kw(&L->cur,"STACKWITHINS4N")||kw(&L->cur,"WITHINS4N")||
      kw(&L->cur,"SWITHINSIGN4N")||kw(&L->cur,"SWITHINS4IMM")||kw(&L->cur,"SINTERVALS4N")){
    /* SWITHINS4N lo hi n — nibble n = (lo <= int4(nib) < hi) ? 1 : 0; n 0..15; no swap */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    long slo = (long)((unsigned long)lo & 0xFul); if (slo & 0x8) slo -= 16;
    long shi = (long)((unsigned long)hi & 0xFul); if (shi & 0x8) shi -= 16;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long v = (long)((base >> sh) & 0xFul); if (v & 0x8) v -= 16;
    unsigned long w = (v >= slo && v < shi) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack imm 4-bit field signed signum: SSIGN4N (nibble ladder; completes field signum all widths) */
  if (kw(&L->cur,"SSIGN4N")||kw(&L->cur,"STACKSIGN4N")||kw(&L->cur,"SIGN4N")||
      kw(&L->cur,"SSIGNUM4N")||kw(&L->cur,"SGN4N")||kw(&L->cur,"SSIGN4IMM")){
    /* SSIGN4N n — nibble n = sgn(int4): +1 / 0 / 0xF(-1); n 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long v = (long)((base >> sh) & 0xFul); if (v & 0x8) v -= 16;
    unsigned long w = (v > 0) ? 1ul : ((v < 0) ? 0xFul : 0ul);
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack imm 4-bit field zero-eq: S0EQ4N (nibble ladder; completes field zero-eq all widths) */
  if (kw(&L->cur,"S0EQ4N")||kw(&L->cur,"STACK0EQ4N")||kw(&L->cur,"SZ4N")||
      kw(&L->cur,"SEQZ4N")||kw(&L->cur,"SISZERO4N")||kw(&L->cur,"S0EQ4IMM")){
    /* S0EQ4N n — nibble n = (nib == 0) ? 1 : 0; n 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long v = (base >> sh) & 0xFul;
    unsigned long w = (v == 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 stack imm 4-bit field nonzero: S0NE4N (nibble ladder; completes field nonzero all widths) */
  if (kw(&L->cur,"S0NE4N")||kw(&L->cur,"STACK0NE4N")||kw(&L->cur,"SNZ4N")||
      kw(&L->cur,"SNEZ4N")||kw(&L->cur,"SISNZ4N")||kw(&L->cur,"S0NE4IMM")){
    /* S0NE4N n — nibble n = (nib != 0) ? 1 : 0; n 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    unsigned long v = (base >> sh) & 0xFul;
    unsigned long w = (v != 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack imm 4-bit field signed LT0: S0LT4N (nibble ladder; completes field signed LT0 all widths) */
  if (kw(&L->cur,"S0LT4N")||kw(&L->cur,"STACK0LT4N")||kw(&L->cur,"SLT04N")||
      kw(&L->cur,"SNEGZ4N")||kw(&L->cur,"SISNEG4N")||kw(&L->cur,"S0LT4IMM")){
    /* S0LT4N n — nibble n = (int4(nib) < 0) ? 1 : 0; n 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long v = (long)((base >> sh) & 0xFul); if (v & 0x8) v -= 16;
    unsigned long w = (v < 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack imm 4-bit field signed GT0: S0GT4N (nibble ladder; completes field signed GT0 all widths) */
  if (kw(&L->cur,"S0GT4N")||kw(&L->cur,"STACK0GT4N")||kw(&L->cur,"SGT04N")||
      kw(&L->cur,"SPOSZ4N")||kw(&L->cur,"SISPOS4N")||kw(&L->cur,"S0GT4IMM")){
    /* S0GT4N n — nibble n = (int4(nib) > 0) ? 1 : 0; n 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long v = (long)((base >> sh) & 0xFul); if (v & 0x8) v -= 16;
    unsigned long w = (v > 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack imm 4-bit field signed LE0: S0LE4N (nibble ladder; completes field signed LE0 all widths) */
  if (kw(&L->cur,"S0LE4N")||kw(&L->cur,"STACK0LE4N")||kw(&L->cur,"SLE04N")||
      kw(&L->cur,"SNONPOS4N")||kw(&L->cur,"SISNONPOS4N")||kw(&L->cur,"S0LE4IMM")){
    /* S0LE4N n — nibble n = (int4(nib) <= 0) ? 1 : 0; n 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long v = (long)((base >> sh) & 0xFul); if (v & 0x8) v -= 16;
    unsigned long w = (v <= 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 stack imm 4-bit field signed GE0: S0GE4N (nibble ladder; completes field signed GE0 all widths) */
  if (kw(&L->cur,"S0GE4N")||kw(&L->cur,"STACK0GE4N")||kw(&L->cur,"SGE04N")||
      kw(&L->cur,"SNONNEG4N")||kw(&L->cur,"SISNONNEG4N")||kw(&L->cur,"S0GE4IMM")){
    /* S0GE4N n — nibble n = (int4(nib) >= 0) ? 1 : 0; n 0..15 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->stack[vm->sp - 1];
    unsigned long sh = (unsigned long)(n * 4);
    long v = (long)((base >> sh) & 0xFul); if (v & 0x8) v -= 16;
    unsigned long w = (v >= 0) ? 1ul : 0ul;
    long r = (long)((base & ~(0xFul << sh)) | (w << sh));
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
