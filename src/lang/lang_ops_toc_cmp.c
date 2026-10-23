/* CubalC lang — lang_ops_toc_cmp.c
 * TOC compare plane: SEQ/SLT/SGT · SNE/SLE/SGE · unsigned · 3-way
 *                    SIGN/EQZ/NEZ · zero-rel · parity (stack + imm)
 * Peel from toc (B_toc_peel). Cube is SoT. Zero bloat.
 */
#include "lang/cubalc_lang_ops_toc_cmp.h"
#include "lang/cubalc_lang_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int cubalc_lang_ops_toc_try_cmp(VM *vm, Lex *L) {
  /* Returns 1 handled, -1 fail, 0 not this plane. */
  /* digit-4 stack↔cell compare plane: SEQTOC · SLTTOC · SGTTOC (predicate dual of SEQ/SLT/SGT) */
  if (kw(&L->cur,"SEQTOC")||kw(&L->cur,"SEQTOCELL")||kw(&L->cur,"STACKEQTOC")||
      kw(&L->cur,"CMPEQTOC")||kw(&L->cur,"SEQAT")){
    /* i v → cells[i] = (cells[i]==v)?1:0, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)i] == v) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SLTTOC")||kw(&L->cur,"SLTTOCELL")||kw(&L->cur,"STACKLTTOC")||
      kw(&L->cur,"CMPLTTOC")||kw(&L->cur,"SLTAT")){
    /* i v → cells[i] = (cells[i]<v)?1:0, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)i] < v) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGTTOC")||kw(&L->cur,"SGTTOCELL")||kw(&L->cur,"STACKGTTOC")||
      kw(&L->cur,"CMPGTTOC")||kw(&L->cur,"SGTAT")){
    /* i v → cells[i] = (cells[i]>v)?1:0, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)i] > v) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 energy compare bounds TOC: SNETOC · SLETOC · SGETOC (complete SEQ/SLT/SGT plane) */
  if (kw(&L->cur,"SNETOC")||kw(&L->cur,"SNETOCELL")||kw(&L->cur,"STACKNETOC")||
      kw(&L->cur,"CMPNETOC")||kw(&L->cur,"SNEAT")){
    /* i v → cells[i] = (cells[i]!=v)?1:0, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)i] != v) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SLETOC")||kw(&L->cur,"SLETOCELL")||kw(&L->cur,"STACKLETOC")||
      kw(&L->cur,"CMPLETOC")||kw(&L->cur,"SLEAT")){
    /* i v → cells[i] = (cells[i]<=v)?1:0, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)i] <= v) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGETOC")||kw(&L->cur,"SGETOCELL")||kw(&L->cur,"STACKGETOC")||
      kw(&L->cur,"CMPGETOC")||kw(&L->cur,"SGEAT")){
    /* i v → cells[i] = (cells[i]>=v)?1:0, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)i] >= v) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 data-path unsigned compare TOC: SULTTOC · SUGTTOC · SULETOC · SUGETOC
   * (unsigned dual of SEQ/SLT signed plane into cell; EQ/NE bit-identical so omitted) */
  if (kw(&L->cur,"SULTTOC")||kw(&L->cur,"SULTTOCELL")||kw(&L->cur,"STACKULTTOC")||
      kw(&L->cur,"CMPULTTOC")||kw(&L->cur,"SULTAT")){
    /* i v → cells[i] = ((unsigned)cells[i] < (unsigned)v)?1:0, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ((unsigned long)vm->cells[(int)i] < (unsigned long)v) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUGTTOC")||kw(&L->cur,"SUGTTOCELL")||kw(&L->cur,"STACKUGTTOC")||
      kw(&L->cur,"CMPUGTTOC")||kw(&L->cur,"SUGTAT")){
    /* i v → cells[i] = ((unsigned)cells[i] > (unsigned)v)?1:0, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ((unsigned long)vm->cells[(int)i] > (unsigned long)v) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SULETOC")||kw(&L->cur,"SULETOCELL")||kw(&L->cur,"STACKULETOC")||
      kw(&L->cur,"CMPULETOC")||kw(&L->cur,"SULEAT")){
    /* i v → cells[i] = ((unsigned)cells[i] <= (unsigned)v)?1:0, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ((unsigned long)vm->cells[(int)i] <= (unsigned long)v) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUGETOC")||kw(&L->cur,"SUGETOCELL")||kw(&L->cur,"STACKUGETOC")||
      kw(&L->cur,"CMPUGETOC")||kw(&L->cur,"SUGEAT")){
    /* i v → cells[i] = ((unsigned)cells[i] >= (unsigned)v)?1:0, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ((unsigned long)vm->cells[(int)i] >= (unsigned long)v) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack↔cell 3-way compare TOC: SCMPTOC · SUCMPTOC (dual of SCMP/SUCMP into cell) */
  if (kw(&L->cur,"SCMPTOC")||kw(&L->cur,"SCMPTOCELL")||kw(&L->cur,"STACKCMPTOC")||
      kw(&L->cur,"SICMPTOC")||kw(&L->cur,"SCMP3TOC")||kw(&L->cur,"CMP3TOC")||
      kw(&L->cur,"SCMPAT")){
    /* i v → cells[i] = signed 3-way (cells[i] ? v) as −1/0/+1, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = (a < v) ? -1 : ((a > v) ? 1 : 0);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUCMPTOC")||kw(&L->cur,"SUCMPTOCELL")||kw(&L->cur,"STACKUCMPTOC")||
      kw(&L->cur,"SUCMP3TOC")||kw(&L->cur,"UCMP3TOC")||kw(&L->cur,"CMPU3TOC")||
      kw(&L->cur,"SUCMPAT")){
    /* i v → cells[i] = unsigned 3-way as −1/0/+1, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long ua = (unsigned long)vm->cells[(int)i];
    unsigned long ub = (unsigned long)v;
    long r = (ua < ub) ? -1 : ((ua > ub) ? 1 : 0);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 unary control TOC: SSIGNTOC · SEQZTOC · SNEZTOC (dual of SSIGN/SEQZ/SNEZ after 3-way TOC) */
  if (kw(&L->cur,"SSIGNTOC")||kw(&L->cur,"SSIGNTOCELL")||kw(&L->cur,"STACKSIGNTOC")||
      kw(&L->cur,"SGNTOC")||kw(&L->cur,"SCELLSIGN")||kw(&L->cur,"SSIGNAT")||
      kw(&L->cur,"SIGNTOC")){
    /* i → cells[i] = sign(cells[i]) as −1/0/+1, leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = (a > 0) ? 1 : ((a < 0) ? -1 : 0);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SEQZTOC")||kw(&L->cur,"SEQZTOCELL")||kw(&L->cur,"STACKEQZTOC")||
      kw(&L->cur,"SISZEROTOC")||kw(&L->cur,"S0EQTOC")||kw(&L->cur,"SEQZAT")||
      kw(&L->cur,"EQZTOC")){
    /* i → cells[i] = (cells[i]==0)?1:0, leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)i] == 0) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNEZTOC")||kw(&L->cur,"SNEZTOCELL")||kw(&L->cur,"STACKNEZTOC")||
      kw(&L->cur,"SISNZTOC")||kw(&L->cur,"S0NETOC")||kw(&L->cur,"SNEZAT")||
      kw(&L->cur,"SNONZEROTOC")||kw(&L->cur,"NEZTOC")){
    /* i → cells[i] = (cells[i]!=0)?1:0, leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)i] != 0) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 zero-relative compare TOC: SLTZTOC · SGTZTOC · SLEZTOC · SGEZTOC
   * (complete S0* / DLTZ plane into cell after SEQZ/SNEZ TOC) */
  if (kw(&L->cur,"SLTZTOC")||kw(&L->cur,"SLTZTOCELL")||kw(&L->cur,"STACKLTZTOC")||
      kw(&L->cur,"S0LTTOC")||kw(&L->cur,"S0LTTAT")||kw(&L->cur,"SLTZAT")||
      kw(&L->cur,"LTZTOC")){
    /* i → cells[i] = (cells[i]<0)?1:0, leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)i] < 0) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGTZTOC")||kw(&L->cur,"SGTZTOCELL")||kw(&L->cur,"STACKGTZTOC")||
      kw(&L->cur,"S0GTTOC")||kw(&L->cur,"S0GTTAT")||kw(&L->cur,"SGTZAT")||
      kw(&L->cur,"GTZTOC")){
    /* i → cells[i] = (cells[i]>0)?1:0, leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)i] > 0) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SLEZTOC")||kw(&L->cur,"SLEZTOCELL")||kw(&L->cur,"STACKLEZTOC")||
      kw(&L->cur,"S0LETOC")||kw(&L->cur,"S0LETAT")||kw(&L->cur,"SLEZAT")||
      kw(&L->cur,"SNONPOSTOC")||kw(&L->cur,"LEZTOC")){
    /* i → cells[i] = (cells[i]<=0)?1:0, leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)i] <= 0) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGEZTOC")||kw(&L->cur,"SGEZTOCELL")||kw(&L->cur,"STACKGEZTOC")||
      kw(&L->cur,"S0GEZTOC")||kw(&L->cur,"S0GEZTAT")||kw(&L->cur,"SGEZAT")||
      kw(&L->cur,"SNONNEGTOC")||kw(&L->cur,"GEZTOC")){
    /* i → cells[i] = (cells[i]>=0)?1:0, leave result
     * note: not S0GETOC (collides mentally with SGETOC two-arg GE) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)i] >= 0) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 pure-imm cell zero-rel LE0/GE0 TOC: SLEZTOCN · SGEZTOCN
   * (imm cell-index dual of SLEZTOC/SGEZTOC after complete field S0LE/S0GE plane) */
  if (kw(&L->cur,"SLEZTOCN")||kw(&L->cur,"S0LETOCN")||kw(&L->cur,"STACKLEZTOCN")||
      kw(&L->cur,"SLEZTOCIMM")||kw(&L->cur,"SNONPOSTOCN")||kw(&L->cur,"S0LEATN")||
      kw(&L->cur,"LEZTOCN")||kw(&L->cur,"SCELLLEZN")){
    /* n → cells[n]=(cells[n]<=0)?1:0; push result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (n >= CUBALC_CELL_N) n = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)n] <= 0) ? 1 : 0;
    vm->cells[(int)n] = r;
    if (vm->sp >= CUBALC_STACK_N){
      var_set_num(vm,"OK",0); bump(vm); return 1;
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGEZTOCN")||kw(&L->cur,"S0GEZTOCN")||kw(&L->cur,"STACKGEZTOCN")||
      kw(&L->cur,"SGEZTOCIMM")||kw(&L->cur,"SNONNEGTOCN")||kw(&L->cur,"S0GEZATN")||
      kw(&L->cur,"GEZTOCN")||kw(&L->cur,"SCELLGEZN")){
    /* n → cells[n]=(cells[n]>=0)?1:0; push result
     * note: not SGETOCN (signed imm GE compare) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (n >= CUBALC_CELL_N) n = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)n] >= 0) ? 1 : 0;
    vm->cells[(int)n] = r;
    if (vm->sp >= CUBALC_STACK_N){
      var_set_num(vm,"OK",0); bump(vm); return 1;
    }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 pure-imm cell zero-rel LT0/GT0 TOC: SLTZTOCN · SGTZTOCN
   * (imm cell-index dual of SLTZTOC/SGTZTOC; complete signed zero-rel TOCN after SLEZTOCN/SGEZTOCN) */
  if (kw(&L->cur,"SLTZTOCN")||kw(&L->cur,"S0LTTOCN")||kw(&L->cur,"STACKLTZTOCN")||
      kw(&L->cur,"SLTZTOCIMM")||kw(&L->cur,"S0LTATN")||kw(&L->cur,"LTZTOCN")||
      kw(&L->cur,"SCELLLTZN")||kw(&L->cur,"SISNEGTOCN")){
    /* n → cells[n]=(cells[n]<0)?1:0; push result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (n >= CUBALC_CELL_N) n = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)n] < 0) ? 1 : 0;
    vm->cells[(int)n] = r;
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGTZTOCN")||kw(&L->cur,"S0GTTOCN")||kw(&L->cur,"STACKGTZTOCN")||
      kw(&L->cur,"SGTZTOCIMM")||kw(&L->cur,"S0GTATN")||kw(&L->cur,"GTZTOCN")||
      kw(&L->cur,"SCELLGTZN")||kw(&L->cur,"SISPOSTOCN")){
    /* n → cells[n]=(cells[n]>0)?1:0; push result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (n >= CUBALC_CELL_N) n = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)n] > 0) ? 1 : 0;
    vm->cells[(int)n] = r;
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
    /* digit-3 pure-imm cell zero-eq/ne TOC: SEQZTOCN · SNEZTOCN
   * (imm cell-index dual of SEQZTOC/SNEZTOC; complete zero-rel TOCN after SLTZTOCN/SGTZTOCN/SLEZTOCN/SGEZTOCN) */
  if (kw(&L->cur,"SEQZTOCN")||kw(&L->cur,"S0EQTOCN")||kw(&L->cur,"STACKEQZTOCN")||
      kw(&L->cur,"SEQZTOCIMM")||kw(&L->cur,"S0EQATN")||kw(&L->cur,"EQZTOCN")||
      kw(&L->cur,"SCELLEQZN")||kw(&L->cur,"SISZEROTOCN")||kw(&L->cur,"SZTOCN")){
    /* n → cells[n]=(cells[n]==0)?1:0; push result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (n >= CUBALC_CELL_N) n = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)n] == 0) ? 1 : 0;
    vm->cells[(int)n] = r;
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNEZTOCN")||kw(&L->cur,"S0NETOCN")||kw(&L->cur,"STACKNEZTOCN")||
      kw(&L->cur,"SNEZTOCIMM")||kw(&L->cur,"S0NEATN")||kw(&L->cur,"NEZTOCN")||
      kw(&L->cur,"SCELLNEZN")||kw(&L->cur,"SISNZTOCN")||kw(&L->cur,"SNONZEROTOCN")||
      kw(&L->cur,"SNZTOCN")){
    /* n → cells[n]=(cells[n]!=0)?1:0; push result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (n >= CUBALC_CELL_N) n = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)n] != 0) ? 1 : 0;
    vm->cells[(int)n] = r;
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 pure-imm cell parity TOC: SODDTOCN · SEVENTOCN
   * (imm cell-index dual of SODDTOC/SEVENTOC after complete zero-rel TOCN plane) */
  if (kw(&L->cur,"SODDTOCN")||kw(&L->cur,"STACKEODDTOCN")||kw(&L->cur,"SODDTOCIMM")||
      kw(&L->cur,"SODDATN")||kw(&L->cur,"ODDTOCN")||kw(&L->cur,"SCELLODDN")||
      kw(&L->cur,"SISODDTOCN")||kw(&L->cur,"SPARITY1TOCN")){
    /* n → cells[n]=(cells[n]&1)?1:0; push result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (n >= CUBALC_CELL_N) n = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)n] & 1L) ? 1 : 0;
    vm->cells[(int)n] = r;
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SEVENTOCN")||kw(&L->cur,"STACKEVENTOCN")||kw(&L->cur,"SEVENTOCIMM")||
      kw(&L->cur,"SEVENATN")||kw(&L->cur,"EVENTOCN")||kw(&L->cur,"SCELLEVENN")||
      kw(&L->cur,"SISEVENTOCN")||kw(&L->cur,"SPARITY0TOCN")){
    /* n → cells[n]=((cells[n]&1)==0)?1:0; push result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (n >= CUBALC_CELL_N) n = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)n] & 1L) ? 0 : 1;
    vm->cells[(int)n] = r;
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 unary parity TOC: SODDTOC · SEVENTOC (dual of SODD/SEVEN after zero-rel TOC) */
  if (kw(&L->cur,"SODDTOC")||kw(&L->cur,"SODDTOCELL")||kw(&L->cur,"STACKODDTOC")||
      kw(&L->cur,"SODDAT")||kw(&L->cur,"SCELLODD")||kw(&L->cur,"ODDTOC")||
      kw(&L->cur,"SISODDTOC")){
    /* i → cells[i] = (cells[i] & 1)?1:0, leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)i] & 1L) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SEVENTOC")||kw(&L->cur,"SEVENTOCELL")||kw(&L->cur,"STACKEVENTOC")||
      kw(&L->cur,"SEVENAT")||kw(&L->cur,"SCELLEVEN")||kw(&L->cur,"EVENTOC")||
      kw(&L->cur,"SISEVENTOC")){
    /* i → cells[i] = ((cells[i] & 1)==0)?1:0, leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)i] & 1L) ? 0 : 1;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }

  return 0;
}
