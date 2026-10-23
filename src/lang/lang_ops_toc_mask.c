/* CubalC lang — lang_ops_toc_mask.c
 * TOC mask plane: SAND/OR/XORMN · POP/ANY/ALLMN · NAND/NOR/XNOR MN+HN
 *                 SCLRMN/HN · SHMASK · low/high-n mask fill
 * Peel from toc (B_toc_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_toc_mask.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_toc_try_mask(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* digit-8 imm low-n mask TOC: SANDMNTOCN · SORMNTOCN · SXORMNTOCN
   * (imm dual of SANDMN/SORMN/SXORMN into cell; low-n keep/set/flip bit fill on cells[i]) */
  if (kw(&L->cur,"SANDMNTOCN")||kw(&L->cur,"SANDMNTOCIMM")||kw(&L->cur,"STACKANDMNTOCN")||
      kw(&L->cur,"SKEEPLNTOCN")||kw(&L->cur,"ANDMNTOCN")||kw(&L->cur,"SCELLANDMN")||
      kw(&L->cur,"SLOWANDTOCN")||kw(&L->cur,"KEEPLNTOCN")){
    /* i + n → cells[i] &= low-n mask; keep low n bits; n clamped 0..64; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long r = (long)((unsigned long)vm->cells[(int)i] & m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORMNTOCN")||kw(&L->cur,"SORMNTOCIMM")||kw(&L->cur,"STACKORMNTOCN")||
      kw(&L->cur,"SSETLNTOCN")||kw(&L->cur,"ORMNTOCN")||kw(&L->cur,"SCELLORMN")||
      kw(&L->cur,"SLOWORTOCN")||kw(&L->cur,"SETLNTOCN")){
    /* i + n → cells[i] |= low-n mask; set low n bits; n clamped 0..64; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long r = (long)((unsigned long)vm->cells[(int)i] | m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXORMNTOCN")||kw(&L->cur,"SXORMNTOCIMM")||kw(&L->cur,"STACKXORMNTOCN")||
      kw(&L->cur,"SFLIPLNTOCN")||kw(&L->cur,"XORMNTOCN")||kw(&L->cur,"SCELLXORMN")||
      kw(&L->cur,"SLOWXORTOCN")||kw(&L->cur,"FLIPLNTOCN")){
    /* i + n → cells[i] ^= low-n mask; toggle low n bits; n clamped 0..64; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long r = (long)((unsigned long)vm->cells[(int)i] ^ m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 imm low-n metric TOC: SPOPMNTOCN · SANYMNTOCN · SALLMNTOCN
   * (imm dual of SPOPMN/SANYMN/SALLMN into cell; metrics after SANDMNTOCN plane) */
  if (kw(&L->cur,"SPOPMNTOCN")||kw(&L->cur,"SPOPMNTOCIMM")||kw(&L->cur,"STACKPOPMNTOCN")||
      kw(&L->cur,"SONESMNTOCN")||kw(&L->cur,"POPMNTOCN")||kw(&L->cur,"SCELLPOPMN")||
      kw(&L->cur,"SLOWPOPTOCN")||kw(&L->cur,"SPCNTMNTOCN")){
    /* i + n → cells[i] = popcount(cells[i] & low-n mask); n 0..64; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    unsigned long ua = (unsigned long)vm->cells[(int)i] & m;
    long r = 0;
    while (ua){ r += (long)(ua & 1ul); ua >>= 1; }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SANYMNTOCN")||kw(&L->cur,"SANYMNTOCIMM")||kw(&L->cur,"STACKANYMNTOCN")||
      kw(&L->cur,"SLOWANYTOCN")||kw(&L->cur,"ANYMNTOCN")||kw(&L->cur,"SCELLANYMN")||
      kw(&L->cur,"STESTANYTOCN")){
    /* i + n → cells[i] = ((cells[i] & low-n mask) != 0) ? 1 : 0; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long r = (((unsigned long)vm->cells[(int)i] & m) != 0) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SALLMNTOCN")||kw(&L->cur,"SALLMNTOCIMM")||kw(&L->cur,"STACKALLMNTOCN")||
      kw(&L->cur,"SLOWALLTOCN")||kw(&L->cur,"ALLMNTOCN")||kw(&L->cur,"SCELLALLMN")||
      kw(&L->cur,"STESTALLTOCN")){
    /* i + n → cells[i] = ((cells[i] & low-n mask) == mask) ? 1 : 0; n=0 → 1 vacuous */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long r = (((unsigned long)vm->cells[(int)i] & m) == m) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 imm inverted low-n mask TOC: SNANDMNTOCN · SNORMNTOCN · SXNORMNTOCN
   * (imm dual of SNANDMN/SNORMN/SXNORMN into cell after SANDMNTOCN plane) */
  if (kw(&L->cur,"SNANDMNTOCN")||kw(&L->cur,"SNANDMNTOCIMM")||kw(&L->cur,"STACKNANDMNTOCN")||
      kw(&L->cur,"SLOWNANDTOCN")||kw(&L->cur,"NANDMNTOCN")||kw(&L->cur,"SCELLNANDMN")||
      kw(&L->cur,"SNANDMASKTOCN")||kw(&L->cur,"MASKNANDTOCN")){
    /* i + n → cells[i] = ~(cells[i] & low-n mask); n clamped 0..64; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long r = (long)~((unsigned long)vm->cells[(int)i] & m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNORMNTOCN")||kw(&L->cur,"SNORMNTOCIMM")||kw(&L->cur,"STACKNORMNTOCN")||
      kw(&L->cur,"SLOWNORTOCN")||kw(&L->cur,"NORMNTOCN")||kw(&L->cur,"SCELLNORMN")||
      kw(&L->cur,"SNORMASKTOCN")||kw(&L->cur,"MASKNORTOCN")){
    /* i + n → cells[i] = ~(cells[i] | low-n mask); n clamped 0..64; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long r = (long)~((unsigned long)vm->cells[(int)i] | m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNORMNTOCN")||kw(&L->cur,"SXNORMNTOCIMM")||kw(&L->cur,"STACKXNORMNTOCN")||
      kw(&L->cur,"SLOWXNORTOCN")||kw(&L->cur,"XNORMNTOCN")||kw(&L->cur,"SCELLXNORMN")||
      kw(&L->cur,"SXNORMASKTOCN")||kw(&L->cur,"MASKXNORTOCN")||
      kw(&L->cur,"SEQUIVMNTOCN")||kw(&L->cur,"EQUIVMNTOCN")){
    /* i + n → cells[i] = ~(cells[i] ^ low-n mask); n clamped 0..64; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long r = (long)~((unsigned long)vm->cells[(int)i] ^ m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 imm high-n mask TOC: SANDHNTOCN · SORHNTOCN · SXORHNTOCN
   * (imm dual of SANDHN/SORHN/SXORHN into cell; high-n dual of SANDMNTOCN plane) */
  if (kw(&L->cur,"SANDHNTOCN")||kw(&L->cur,"SANDHNTOCIMM")||kw(&L->cur,"STACKANDHNTOCN")||
      kw(&L->cur,"SKEEPHNTOCN")||kw(&L->cur,"ANDHNTOCN")||kw(&L->cur,"SCELLANDHN")||
      kw(&L->cur,"SHIGHANDTOCN")||kw(&L->cur,"KEEPHNTOCN")){
    /* i + n → cells[i] &= high-n mask; keep high n bits; n clamped 0..64; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long r = (long)((unsigned long)vm->cells[(int)i] & m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORHNTOCN")||kw(&L->cur,"SORHNTOCIMM")||kw(&L->cur,"STACKORHNTOCN")||
      kw(&L->cur,"SSETHNTOCN")||kw(&L->cur,"ORHNTOCN")||kw(&L->cur,"SCELLORHN")||
      kw(&L->cur,"SHIGHORTOCN")||kw(&L->cur,"SETHNTOCN")){
    /* i + n → cells[i] |= high-n mask; set high n bits; n clamped 0..64; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long r = (long)((unsigned long)vm->cells[(int)i] | m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXORHNTOCN")||kw(&L->cur,"SXORHNTOCIMM")||kw(&L->cur,"STACKXORHNTOCN")||
      kw(&L->cur,"SFLIPHNTOCN")||kw(&L->cur,"XORHNTOCN")||kw(&L->cur,"SCELLXORHN")||
      kw(&L->cur,"SHIGHXORTOCN")||kw(&L->cur,"FLIPHNTOCN")){
    /* i + n → cells[i] ^= high-n mask; toggle high n bits; n clamped 0..64; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long r = (long)((unsigned long)vm->cells[(int)i] ^ m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 imm inverted high-n mask TOC: SNANDHNTOCN · SNORHNTOCN · SXNORHNTOCN
   * (imm dual of SNANDHN/SNORHN/SXNORHN into cell; high dual of SNANDMNTOCN after SANDHNTOCN) */
  if (kw(&L->cur,"SNANDHNTOCN")||kw(&L->cur,"SNANDHNTOCIMM")||kw(&L->cur,"STACKNANDHNTOCN")||
      kw(&L->cur,"SHIGHNANDTOCN")||kw(&L->cur,"NANDHNTOCN")||kw(&L->cur,"SCELLNANDHN")||
      kw(&L->cur,"SNANDMASKHTOCN")||kw(&L->cur,"MASKNANDHTOCN")){
    /* i + n → cells[i] = ~(cells[i] & high-n mask); n clamped 0..64; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long r = (long)~((unsigned long)vm->cells[(int)i] & m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNORHNTOCN")||kw(&L->cur,"SNORHNTOCIMM")||kw(&L->cur,"STACKNORHNTOCN")||
      kw(&L->cur,"SHIGHNORTOCN")||kw(&L->cur,"NORHNTOCN")||kw(&L->cur,"SCELLNORHN")||
      kw(&L->cur,"SNORMASKHTOCN")||kw(&L->cur,"MASKNORHTOCN")){
    /* i + n → cells[i] = ~(cells[i] | high-n mask); n clamped 0..64; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long r = (long)~((unsigned long)vm->cells[(int)i] | m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNORHNTOCN")||kw(&L->cur,"SXNORHNTOCIMM")||kw(&L->cur,"STACKXNORHNTOCN")||
      kw(&L->cur,"SHIGHXNORTOCN")||kw(&L->cur,"XNORHNTOCN")||kw(&L->cur,"SCELLXNORHN")||
      kw(&L->cur,"SXNORMASKHTOCN")||kw(&L->cur,"MASKXNORHTOCN")||
      kw(&L->cur,"SEQUIVHNTOCN")||kw(&L->cur,"EQUIVHNTOCN")){
    /* i + n → cells[i] = ~(cells[i] ^ high-n mask); n clamped 0..64; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long r = (long)~((unsigned long)vm->cells[(int)i] ^ m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 imm clear + high-mask TOC: SCLRMNTOCN · SCLRHNTOCN · SHMASKTOCN
   * (imm dual of SCLRMN/SCLRHN/SHMASKN into cell after SANDMNTOCN/SANDHNTOCN plane) */
  if (kw(&L->cur,"SCLRMNTOCN")||kw(&L->cur,"SCLRMNTOCIMM")||kw(&L->cur,"STACKCLRMNTOCN")||
      kw(&L->cur,"SCLEARLNTOCN")||kw(&L->cur,"CLRMNTOCN")||kw(&L->cur,"SCELLCLRMN")||
      kw(&L->cur,"SLOWCLRTOCN")||kw(&L->cur,"CLEARLNTOCN")){
    /* i + n → cells[i] &= ~low-n mask; clear low n bits; n clamped 0..64; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = (1ul << (unsigned)n) - 1ul;
    long r = (long)((unsigned long)vm->cells[(int)i] & ~m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLRHNTOCN")||kw(&L->cur,"SCLRHNTOCIMM")||kw(&L->cur,"STACKCLRHNTOCN")||
      kw(&L->cur,"SCLEARHNTOCN")||kw(&L->cur,"CLRHNTOCN")||kw(&L->cur,"SCELLCLRHN")||
      kw(&L->cur,"SHIGHCLRTOCN")||kw(&L->cur,"CLEARHNTOCN")){
    /* i + n → cells[i] &= ~high-n mask; clear high n bits; n clamped 0..64; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long r = (long)((unsigned long)vm->cells[(int)i] & ~m);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHMASKTOCN")||kw(&L->cur,"SHMASKTOCIMM")||kw(&L->cur,"STACKHMASKTOCN")||
      kw(&L->cur,"SHIMASKTOCN")||kw(&L->cur,"HMASKTOCN")||kw(&L->cur,"SCELLHMASK")||
      kw(&L->cur,"SHIGHMASKTOCN")||kw(&L->cur,"SETHMASKTOCN")){
    /* i + n → cells[i] = high-n-bit mask; n clamped 0..64; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long r = (long)m;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  return 0;
}
