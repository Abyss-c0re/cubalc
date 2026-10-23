/* CubalC lang — lang_ops_toc_imm.c
 * TOC imm plane: SADDTOCN..SMULTOCN · SADDCTOCN/SSUBBTOCN · SAND/OR/XORTOCN
 *                SSETB/CLRB/FLPB TOCN+TOC bitfield
 * Peel from toc (B_toc_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_toc_imm.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_toc_try_imm(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* digit-4 imm accumulate TOC: SADDTOCN · SSUBTOCN · SMULTOCN
   * (imm dual of SADDTOC/SSUBTOC/SMULTOC; peer of SADDN/SSUBN/SMULN into cell) */
  if (kw(&L->cur,"SADDTOCN")||kw(&L->cur,"SADDTOCIMM")||kw(&L->cur,"STACKADDTOCN")||
      kw(&L->cur,"SADDATN")||kw(&L->cur,"ADDTOCN")||kw(&L->cur,"SCELLADDN")){
    /* i + n → cells[i]+=n, leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] + n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSUBTOCN")||kw(&L->cur,"SSUBTOCIMM")||kw(&L->cur,"STACKSUBTOCN")||
      kw(&L->cur,"SSUBATN")||kw(&L->cur,"SUBTOCN")||kw(&L->cur,"SCELLSUBN")){
    /* i + n → cells[i]-=n, leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] - n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMULTOCN")||kw(&L->cur,"SMULTOCIMM")||kw(&L->cur,"STACKMULTOCN")||
      kw(&L->cur,"SMULATN")||kw(&L->cur,"MULTOCN")||kw(&L->cur,"SCELLMULN")){
    /* i + n → cells[i]*=n, leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] * n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 multiword carry/borrow imm TOC: SADDCTOCN · SSUBBTOCN
   * (imm dual of SADDCN/SSUBBN into cell; complete SADDTOCN plane with carry chain) */
  if (kw(&L->cur,"SADDCTOCN")||kw(&L->cur,"SADDCTOCIMM")||kw(&L->cur,"STACKADDCTOCN")||
      kw(&L->cur,"SADDCATN")||kw(&L->cur,"ADDCTOCN")||kw(&L->cur,"SCELLADDCN")||
      kw(&L->cur,"SADDCINTOCN")||kw(&L->cur,"SADCINTOCN")){
    /* i + n → cells[i] = cells[i] + n + cin(CARRY); update CARRY/CY; leave sum */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long cin = 0;
    {
      Var *vc = var_get(vm, "CARRY", 0);
      if (vc && vc->val) cin = 1;
    }
    unsigned long ua = (unsigned long)a;
    unsigned long ub = (unsigned long)n;
    unsigned long uc = (unsigned long)cin;
    unsigned long s = ua + ub;
    int c1 = (s < ua) ? 1 : 0;
    unsigned long sum = s + uc;
    int c2 = (sum < s) ? 1 : 0;
    int carry = c1 | c2;
    long r = (long)sum;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"CARRY",carry); var_set_num(vm,"CY",carry);
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSUBBTOCN")||kw(&L->cur,"SSUBBTOCIMM")||kw(&L->cur,"STACKSUBBTOCN")||
      kw(&L->cur,"SSUBBATN")||kw(&L->cur,"SUBBTOCN")||kw(&L->cur,"SCELLSUBBN")||
      kw(&L->cur,"SSBBTOCN")||kw(&L->cur,"SSUBBINTOCN")){
    /* i + n → cells[i] = cells[i] - n - bin(BORROW|CARRY); update BORROW/BW/CARRY; leave diff */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long cin = 0;
    {
      Var *vc = var_get(vm, "BORROW", 0);
      if (!vc) vc = var_get(vm, "CARRY", 0);
      if (vc && vc->val) cin = 1;
    }
    unsigned long ua = (unsigned long)a;
    unsigned long ub = (unsigned long)n;
    unsigned long uc = (unsigned long)cin;
    int b1 = (ua < ub) ? 1 : 0;
    unsigned long d = ua - ub;
    int b2 = (d < uc) ? 1 : 0;
    unsigned long diff = d - uc;
    int borrow = b1 | b2;
    long r = (long)diff;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"BORROW",borrow); var_set_num(vm,"BW",borrow);
    var_set_num(vm,"CARRY",borrow);
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 imm bitwise TOC: SANDTOCN · SORTOCN · SXORTOCN
   * (imm dual of SANDTOC/SORTOC/SXORTOC; bitfield peer of SADDTOCN after SANDIMM plane) */
  if (kw(&L->cur,"SANDTOCN")||kw(&L->cur,"SANDTOCIMM")||kw(&L->cur,"STACKANDTOCN")||
      kw(&L->cur,"SANDATN")||kw(&L->cur,"ANDTOCN")||kw(&L->cur,"SCELLANDN")||
      kw(&L->cur,"BANDTOCN")){
    /* i + n → cells[i] &= n, leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] & n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORTOCN")||kw(&L->cur,"SORTOCIMM")||kw(&L->cur,"STACKORTOCN")||
      kw(&L->cur,"SORATN")||kw(&L->cur,"ORTOCN")||kw(&L->cur,"SCELLORN")||
      kw(&L->cur,"BORTOCN")){
    /* i + n → cells[i] |= n, leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] | n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXORTOCN")||kw(&L->cur,"SXORTOCIMM")||kw(&L->cur,"STACKXORTOCN")||
      kw(&L->cur,"SXORATN")||kw(&L->cur,"XORTOCN")||kw(&L->cur,"SCELLXORN")||
      kw(&L->cur,"BXORTOCN")){
    /* i + n → cells[i] ^= n, leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] ^ n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 bitfield imm TOC: SSETBTOCN · SCLRBTOCN · SFLPBTOCN
   * (imm dual of SSETBN/SCLRBN/SFLIPBN into cell; complete SANDTOCN plane with single-bit ops) */
  if (kw(&L->cur,"SSETBTOCN")||kw(&L->cur,"SSETBTOCIMM")||kw(&L->cur,"STACKSETBTOCN")||
      kw(&L->cur,"SSETBITTOCN")||kw(&L->cur,"SETBTOCN")||kw(&L->cur,"SCELLSETBN")||
      kw(&L->cur,"SBSETTOCN")||kw(&L->cur,"BSETTOCN")){
    /* i + n → cells[i] |= (1<<n), leave result; n clamped 0..63 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    unsigned long u = (unsigned long)vm->cells[(int)i];
    u |= (1ul << (unsigned)n);
    long r = (long)u;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLRBTOCN")||kw(&L->cur,"SCLRBTOCIMM")||kw(&L->cur,"STACKCLRBTOCN")||
      kw(&L->cur,"SCLRBITTOCN")||kw(&L->cur,"CLRBTOCN")||kw(&L->cur,"SCELLCLRBN")||
      kw(&L->cur,"SBCLRTOCN")||kw(&L->cur,"BCLRTOCN")){
    /* i + n → cells[i] &= ~(1<<n), leave result; n clamped 0..63 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    unsigned long u = (unsigned long)vm->cells[(int)i];
    u &= ~(1ul << (unsigned)n);
    long r = (long)u;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SFLPBTOCN")||kw(&L->cur,"SFLIPBTOCN")||kw(&L->cur,"STACKFLPBTOCN")||
      kw(&L->cur,"SFLIPBITTOCN")||kw(&L->cur,"FLPBTOCN")||kw(&L->cur,"SCELLFLIPBN")||
      kw(&L->cur,"STGLBTOCN")||kw(&L->cur,"SBFLIPTOCN")||kw(&L->cur,"BFLIPTOCN")){
    /* i + n → cells[i] ^= (1<<n), leave result; n clamped 0..63 */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    unsigned long u = (unsigned long)vm->cells[(int)i];
    u ^= (1ul << (unsigned)n);
    long r = (long)u;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack bitfield TOC: SSETBTOC · SCLRBTOC · SFLPBTOC
   * (stack dual of SSETBTOCN/SCLRBTOCN/SFLPBTOCN; i bit → set/clear/flip bit in cell) */
  if (kw(&L->cur,"SSETBTOC")||kw(&L->cur,"SSETBITTOC")||kw(&L->cur,"STACKSETBTOC")||
      kw(&L->cur,"SETBTOC")||kw(&L->cur,"SCELLSETB")||kw(&L->cur,"SBSETTOC")||
      kw(&L->cur,"BSETTOC")||kw(&L->cur,"SSETBAT")){
    /* i bit → cells[i] |= (1<<bit), leave result; bit clamped 0..63 */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long bit = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (bit < 0) bit = 0;
    if (bit > 63) bit = 63;
    unsigned long u = (unsigned long)vm->cells[(int)i];
    u |= (1ul << (unsigned)bit);
    long r = (long)u;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLRBTOC")||kw(&L->cur,"SCLRBITTOC")||kw(&L->cur,"STACKCLRBTOC")||
      kw(&L->cur,"CLRBTOC")||kw(&L->cur,"SCELLCLRB")||kw(&L->cur,"SBCLRTOC")||
      kw(&L->cur,"BCLRTOC")||kw(&L->cur,"SCLRBAT")){
    /* i bit → cells[i] &= ~(1<<bit), leave result; bit clamped 0..63 */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long bit = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (bit < 0) bit = 0;
    if (bit > 63) bit = 63;
    unsigned long u = (unsigned long)vm->cells[(int)i];
    u &= ~(1ul << (unsigned)bit);
    long r = (long)u;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SFLPBTOC")||kw(&L->cur,"SFLIPBTOC")||kw(&L->cur,"STACKFLPBTOC")||
      kw(&L->cur,"SFLIPBITTOC")||kw(&L->cur,"FLPBTOC")||kw(&L->cur,"SCELLFLIPB")||
      kw(&L->cur,"STGLBTOC")||kw(&L->cur,"SBFLIPTOC")||kw(&L->cur,"BFLIPTOC")||
      kw(&L->cur,"SFLPBAT")){
    /* i bit → cells[i] ^= (1<<bit), leave result; bit clamped 0..63 */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long bit = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (bit < 0) bit = 0;
    if (bit > 63) bit = 63;
    unsigned long u = (unsigned long)vm->cells[(int)i];
    u ^= (1ul << (unsigned)bit);
    long r = (long)u;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  return 0;
}
