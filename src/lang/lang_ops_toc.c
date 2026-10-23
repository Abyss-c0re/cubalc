/* CubalC lang — lang_ops_toc.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"
#include "lang/cubalc_lang_ops_toc_cell.h"
#include "lang/cubalc_lang_ops_toc_imm.h"
#include "lang/cubalc_lang_ops_toc_field.h"
#include "lang/cubalc_lang_ops_toc_extclip.h"
#include "lang/cubalc_lang_ops_toc_mask.h"
#include "lang/cubalc_lang_ops_toc_bitops.h"
#include "lang/cubalc_lang_ops_toc_alu.h"
#include "lang/cubalc_lang_ops_toc_mod.h"
#include "lang/cubalc_lang_ops_toc_revsat.h"
#include "lang/cubalc_lang_ops_toc_unary.h"
#include "lang/cubalc_lang_ops_toc_bwshift.h"

int cubalc_lang_ops_toc(VM *vm, Lex *L){
  /* plane ops_toc — sequential thin dispatchers */
  {
    int tc = cubalc_lang_ops_toc_try_cell(vm, L);
    if (tc != 0) return tc;
  }
  {
    int ti = cubalc_lang_ops_toc_try_imm(vm, L);
    if (ti != 0) return ti;
  }
  {
    int tf = cubalc_lang_ops_toc_try_field(vm, L);
    if (tf != 0) return tf;
  }
  {
    int te = cubalc_lang_ops_toc_try_extclip(vm, L);
    if (te != 0) return te;
  }
  {
    int tm = cubalc_lang_ops_toc_try_mask(vm, L);
    if (tm != 0) return tm;
  }
  {
    int tb = cubalc_lang_ops_toc_try_bitops(vm, L);
    if (tb != 0) return tb;
  }
  {
    int ta = cubalc_lang_ops_toc_try_alu(vm, L);
    if (ta != 0) return ta;
  }
  {
    int td = cubalc_lang_ops_toc_try_mod(vm, L);
    if (td != 0) return td;
  }
  {
    int tr = cubalc_lang_ops_toc_try_revsat(vm, L);
    if (tr != 0) return tr;
  }
  {
    int tw = cubalc_lang_ops_toc_try_bwshift(vm, L);
    if (tw != 0) return tw;

  /* unary/scale/numthy → lang_ops_toc_unary.c (B_toc_peel) */
  {
    int tu = cubalc_lang_ops_toc_try_unary(vm, L);
    if (tu != 0) return tu;
  }
  }

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
  /* digit-8 stack↔cell bit metrics TOC: SPOPCNTTOC · SCLZTOC · SCTZTOC
   * (dual of SPOPCNT/SCLZ/SCTZ into cell after parity/unary TOC plane) */
  if (kw(&L->cur,"SPOPCNTTOC")||kw(&L->cur,"SPOPCNTTOCELL")||kw(&L->cur,"STACKPOPCNTTOC")||
      kw(&L->cur,"SPCNTTOC")||kw(&L->cur,"SPOPAT")||kw(&L->cur,"POPCNTTOC")||
      kw(&L->cur,"SCELLPOPCNT")){
    /* i → cells[i]=popcount(cells[i]), leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long u = (unsigned long)vm->cells[(int)i];
    long r = 0;
    while (u){ r += (long)(u & 1ul); u >>= 1; }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLZTOC")||kw(&L->cur,"SCLZTOCELL")||kw(&L->cur,"STACKCLZTOC")||
      kw(&L->cur,"SNLZTOC")||kw(&L->cur,"SCLZAT")||kw(&L->cur,"CLZTOC")||
      kw(&L->cur,"SCELLCLZ")){
    /* i → cells[i]=clz64(cells[i]) (0 → 64), leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long u = (unsigned long)vm->cells[(int)i];
    long r = 0;
    if (u == 0) r = 64;
    else {
      for (int b = 63; b >= 0; b--){
        if (u & (1ul << (unsigned)b)) break;
        r++;
      }
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCTZTOC")||kw(&L->cur,"SCTZTOCELL")||kw(&L->cur,"STACKCTZTOC")||
      kw(&L->cur,"SNTZTOC")||kw(&L->cur,"SCTZAT")||kw(&L->cur,"CTZTOC")||
      kw(&L->cur,"SCELLCTZ")){
    /* i → cells[i]=ctz64(cells[i]) (0 → 64), leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long u = (unsigned long)vm->cells[(int)i];
    long r = 0;
    if (u == 0) r = 64;
    else {
      while ((u & 1ul) == 0){ r++; u >>= 1; }
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack↔cell word bit metrics TOC: SPARITYTOC · SBSWAPTOC · SBITREV32TOC
   * (dual of SPARITY/SBSWAP/SBITREV into cell after SPOPCNT/CLZ/CTZ TOC) */
  if (kw(&L->cur,"SPARITYTOC")||kw(&L->cur,"SPARTOC")||kw(&L->cur,"STACKPARITYTOC")||
      kw(&L->cur,"SCELLPARITY")||kw(&L->cur,"PARITYTOC")||kw(&L->cur,"SPARAT")){
    /* i → cells[i]=parity(cells[i]) (xor of all bits), leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long u = (unsigned long)vm->cells[(int)i];
    long r = 0;
    while (u){ r ^= (long)(u & 1ul); u >>= 1; }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SBSWAPTOC")||kw(&L->cur,"SBSWAP32TOC")||kw(&L->cur,"STACKBSWAPTOC")||
      kw(&L->cur,"SCELLBSWAP")||kw(&L->cur,"BSWAPTOC")||kw(&L->cur,"SBSWAPAT")){
    /* i → cells[i]=bswap32(low32 cells[i]), leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned int w = (unsigned int)vm->cells[(int)i];
    w = ((w & 0x000000FFu) << 24) | ((w & 0x0000FF00u) << 8) |
        ((w & 0x00FF0000u) >> 8) | ((w & 0xFF000000u) >> 24);
    long r = (long)w;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SBITREV32TOC")||kw(&L->cur,"SREV32TOC")||kw(&L->cur,"STACKBITREV32TOC")||
      kw(&L->cur,"SCELLBITREV32")||kw(&L->cur,"BITREV32TOC")||kw(&L->cur,"SBITREV32AT")||
      kw(&L->cur,"SREVBITSTOC32")){
    /* i → cells[i]=bitrev32(low32 cells[i]), leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned int w = (unsigned int)vm->cells[(int)i];
    unsigned int rv = 0;
    for (int b = 0; b < 32; b++){
      rv = (rv << 1) | (w & 1u);
      w >>= 1;
    }
    long r = (long)rv;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack↔cell endian/bitrev width ladder TOC: SBSWAP16TOC · SBSWAP64TOC · SBITREV16TOC
   * (complete bswap 16/32/64 + bitrev 16/32 into cell after SPARITY/SBSWAP/SBITREV32 TOC) */
  if (kw(&L->cur,"SBSWAP16TOC")||kw(&L->cur,"STACKBSWAP16TOC")||kw(&L->cur,"SCELLBSWAP16")||
      kw(&L->cur,"BSWAP16TOC")||kw(&L->cur,"SBSWAP16AT")||kw(&L->cur,"SBS16TOC")){
    /* i → cells[i]=bswap16(low16 cells[i]), leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFFFu;
    w = ((w & 0x00FFu) << 8) | ((w & 0xFF00u) >> 8);
    long r = (long)w;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SBSWAP64TOC")||kw(&L->cur,"STACKBSWAP64TOC")||kw(&L->cur,"SCELLBSWAP64")||
      kw(&L->cur,"BSWAP64TOC")||kw(&L->cur,"SBSWAP64AT")||kw(&L->cur,"SBS64TOC")){
    /* i → cells[i]=bswap64(cells[i]), leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long w = (unsigned long)vm->cells[(int)i];
    w = ((w & 0x00000000000000FFul) << 56) | ((w & 0x000000000000FF00ul) << 40) |
        ((w & 0x0000000000FF0000ul) << 24) | ((w & 0x00000000FF000000ul) << 8) |
        ((w & 0x000000FF00000000ul) >> 8) | ((w & 0x0000FF0000000000ul) >> 24) |
        ((w & 0x00FF000000000000ul) >> 40) | ((w & 0xFF00000000000000ul) >> 56);
    long r = (long)w;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SBITREV16TOC")||kw(&L->cur,"SREV16TOC")||kw(&L->cur,"STACKBITREV16TOC")||
      kw(&L->cur,"SCELLBITREV16")||kw(&L->cur,"BITREV16TOC")||kw(&L->cur,"SBITREV16AT")||
      kw(&L->cur,"SREVBITSTOC16")){
    /* i → cells[i]=bitrev16(low16 cells[i]), leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned int w = (unsigned int)vm->cells[(int)i] & 0xFFFFu;
    unsigned int rv = 0;
    for (int b = 0; b < 16; b++){
      rv = (rv << 1) | (w & 1u);
      w >>= 1;
    }
    long r = (long)rv;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack↔cell fixed-width shift32 TOC: SSHL32TOC · SSHR32TOC · SSAR32TOC
   * (cell dual of SSHL32/SSHR32/SSAR32 after dual-stack DSHL32 plane + variable SSHLTOC) */
  if (kw(&L->cur,"SSHL32TOC")||kw(&L->cur,"STACKSHL32TOC")||kw(&L->cur,"SCELLSHL32")||
      kw(&L->cur,"SHL32TOC")||kw(&L->cur,"SSHL32AT")||kw(&L->cur,"S32SHLTOC")){
    /* i k → cells[i] = (uint32)cells[i] << k (k>=32 → 0), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long k = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned int w = (unsigned int)vm->cells[(int)i];
    long r = (kk >= 32) ? 0L : (long)(w << kk);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSHR32TOC")||kw(&L->cur,"STACKSHR32TOC")||kw(&L->cur,"SCELLSHR32")||
      kw(&L->cur,"SHR32TOC")||kw(&L->cur,"SSHR32AT")||kw(&L->cur,"S32SHRTOC")){
    /* i k → cells[i] = (uint32)cells[i] >> k (k>=32 → 0), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long k = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned int w = (unsigned int)vm->cells[(int)i];
    long r = (kk >= 32) ? 0L : (long)(w >> kk);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSAR32TOC")||kw(&L->cur,"SASHR32TOC")||kw(&L->cur,"STACKSAR32TOC")||
      kw(&L->cur,"SAR32TOC")||kw(&L->cur,"SSAR32AT")||kw(&L->cur,"SCELLSAR32")||
      kw(&L->cur,"S32SARTOC")||kw(&L->cur,"ASHR32TOC")){
    /* i k → cells[i] = arithmetic right shift low32 (k>=32 → all sign), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long k = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    long va = (long)(int)(unsigned int)vm->cells[(int)i];
    long r;
    if (kk >= 32) r = (va < 0) ? -1L : 0L;
    else r = va >> kk;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 foundation fixed-width rotate32 TOC: SROTL32TOC · SROTR32TOC · SROTL32TOCN · SROTR32TOCN
   * (complete shift/rotate 32 into cell after SSHL32TOC plane; dual of SROLTOC + SROTL32 stack) */
  if (kw(&L->cur,"SROTL32TOC")||kw(&L->cur,"SROL32TOC")||kw(&L->cur,"STACKROTL32TOC")||
      kw(&L->cur,"ROL32TOC")||kw(&L->cur,"SROTL32AT")||kw(&L->cur,"SCELLROL32")||
      kw(&L->cur,"S32ROLTOC")){
    /* i k → cells[i] = rotl32(low32 cells[i], k&31), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long k = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned int w = (unsigned int)vm->cells[(int)i];
    kk &= 31;
    long r = (kk == 0) ? (long)w : (long)(((w << kk) | (w >> (32 - kk))));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROTR32TOC")||kw(&L->cur,"SROR32TOC")||kw(&L->cur,"STACKROTR32TOC")||
      kw(&L->cur,"ROR32TOC")||kw(&L->cur,"SROTR32AT")||kw(&L->cur,"SCELLROR32")||
      kw(&L->cur,"S32RORTOC")){
    /* i k → cells[i] = rotr32(low32 cells[i], k&31), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long k = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned int w = (unsigned int)vm->cells[(int)i];
    kk &= 31;
    long r = (kk == 0) ? (long)w : (long)(((w >> kk) | (w << (32 - kk))));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROTL32TOCN")||kw(&L->cur,"SROL32TOCN")||kw(&L->cur,"STACKROTL32TOCN")||
      kw(&L->cur,"ROL32TOCN")||kw(&L->cur,"SROTL32TOCIMM")||kw(&L->cur,"SCELLROL32N")||
      kw(&L->cur,"S32ROLTOCN")){
    /* i + k → cells[i] = rotl32(low32 cells[i], k&31), leave result (imm dual) */
    lex_next(L);
    long k = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned int w = (unsigned int)vm->cells[(int)i];
    kk &= 31;
    long r = (kk == 0) ? (long)w : (long)(((w << kk) | (w >> (32 - kk))));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROTR32TOCN")||kw(&L->cur,"SROR32TOCN")||kw(&L->cur,"STACKROTR32TOCN")||
      kw(&L->cur,"ROR32TOCN")||kw(&L->cur,"SROTR32TOCIMM")||kw(&L->cur,"SCELLROR32N")||
      kw(&L->cur,"S32RORTOCN")){
    /* i + k → cells[i] = rotr32(low32 cells[i], k&31), leave result (imm dual) */
    lex_next(L);
    long k = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned int w = (unsigned int)vm->cells[(int)i];
    kk &= 31;
    long r = (kk == 0) ? (long)w : (long)(((w >> kk) | (w << (32 - kk))));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 foundation imm fixed-width shift32 TOC: SSHL32TOCN · SSHR32TOCN · SSAR32TOCN
   * (imm dual of SSHL32TOC/SSHR32TOC/SSAR32TOC; peer of SROTL32TOCN after rotate32 plane) */
  if (kw(&L->cur,"SSHL32TOCN")||kw(&L->cur,"STACKSHL32TOCN")||kw(&L->cur,"SCELLSHL32N")||
      kw(&L->cur,"SHL32TOCN")||kw(&L->cur,"SSHL32TOCIMM")||kw(&L->cur,"S32SHLTOCN")){
    /* i + k → cells[i] = (uint32)cells[i] << k (k>=32 → 0), leave result */
    lex_next(L);
    long k = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned int w = (unsigned int)vm->cells[(int)i];
    long r = (kk >= 32) ? 0L : (long)(w << kk);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSHR32TOCN")||kw(&L->cur,"STACKSHR32TOCN")||kw(&L->cur,"SCELLSHR32N")||
      kw(&L->cur,"SHR32TOCN")||kw(&L->cur,"SSHR32TOCIMM")||kw(&L->cur,"S32SHRTOCN")){
    /* i + k → cells[i] = (uint32)cells[i] >> k (k>=32 → 0), leave result */
    lex_next(L);
    long k = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    unsigned int w = (unsigned int)vm->cells[(int)i];
    long r = (kk >= 32) ? 0L : (long)(w >> kk);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSAR32TOCN")||kw(&L->cur,"SASHR32TOCN")||kw(&L->cur,"STACKSAR32TOCN")||
      kw(&L->cur,"SAR32TOCN")||kw(&L->cur,"SSAR32TOCIMM")||kw(&L->cur,"SCELLSAR32N")||
      kw(&L->cur,"S32SARTOCN")||kw(&L->cur,"ASHR32TOCN")){
    /* i + k → cells[i] = arithmetic right shift low32 (k>=32 → all sign), leave result */
    lex_next(L);
    long k = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    int kk = (int)k;
    if (kk < 0) kk = 0;
    long va = (long)(int)(unsigned int)vm->cells[(int)i];
    long r;
    if (kk >= 32) r = (va < 0) ? -1L : 0L;
    else r = va >> kk;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 imm compare TOC: SEQTOCN · SNETOCN · SLTTOCN · SGTTOCN
   * (imm dual of SEQTOC/SNETOC/SLTTOC/SGTTOC; peer of SEQN/SNEN/SLTN/SGTN) */
  if (kw(&L->cur,"SEQTOCN")||kw(&L->cur,"SEQTOCIMM")||kw(&L->cur,"STACKEQTOCN")||
      kw(&L->cur,"CMPEQTOCN")||kw(&L->cur,"SEQATN")||kw(&L->cur,"EQTOCN")){
    /* i + n → cells[i]=(cells[i]==n)?1:0, leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)i] == n) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNETOCN")||kw(&L->cur,"SNETOCIMM")||kw(&L->cur,"STACKNETOCN")||
      kw(&L->cur,"CMPNETOCN")||kw(&L->cur,"SNEATN")||kw(&L->cur,"NETOCN")){
    /* i + n → cells[i]=(cells[i]!=n)?1:0, leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)i] != n) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SLTTOCN")||kw(&L->cur,"SLTTOCIMM")||kw(&L->cur,"STACKLTTOCN")||
      kw(&L->cur,"CMPLTTOCN")||kw(&L->cur,"SLTATN")||kw(&L->cur,"LTTOCN")){
    /* i + n → cells[i]=(cells[i]<n)?1:0, leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)i] < n) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGTTOCN")||kw(&L->cur,"SGTTOCIMM")||kw(&L->cur,"STACKGTTOCN")||
      kw(&L->cur,"CMPGTTOCN")||kw(&L->cur,"SGTATN")||kw(&L->cur,"GTTOCN")){
    /* i + n → cells[i]=(cells[i]>n)?1:0, leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)i] > n) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 data-path imm compare bounds TOC: SLETOCN · SGETOCN
   * (complete SEQ/NE/LT/GT TOCN plane with LE/GE imm dual of SLETOC/SGETOC) */
  if (kw(&L->cur,"SLETOCN")||kw(&L->cur,"SLETOCIMM")||kw(&L->cur,"STACKLETOCN")||
      kw(&L->cur,"CMPLETOCN")||kw(&L->cur,"SLEATN")||kw(&L->cur,"LETOCN")||
      kw(&L->cur,"SLEQNTOC")){
    /* i + n → cells[i]=(cells[i]<=n)?1:0, leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)i] <= n) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SGETOCN")||kw(&L->cur,"SGETOCIMM")||kw(&L->cur,"STACKGETOCN")||
      kw(&L->cur,"CMPGETOCN")||kw(&L->cur,"SGEATN")||kw(&L->cur,"GETOCN")||
      kw(&L->cur,"SGEQNTOC")){
    /* i + n → cells[i]=(cells[i]>=n)?1:0, leave result
     * note: SGETOCN != SGETOC (two-arg stack GE TOC) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (vm->cells[(int)i] >= n) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack foundation unsigned imm compare TOC:
   * SULTTOCN · SUGTTOCN · SULETOCN · SUGETOCN (unsigned dual of signed TOCN plane) */
  if (kw(&L->cur,"SULTTOCN")||kw(&L->cur,"SULTTOCIMM")||kw(&L->cur,"STACKULTTOCN")||
      kw(&L->cur,"CMPULTTOCN")||kw(&L->cur,"SULTATN")||kw(&L->cur,"ULTTOCN")){
    /* i + n → cells[i]=((u)cells[i]<(u)n)?1:0, leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ((unsigned long)vm->cells[(int)i] < (unsigned long)n) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUGTTOCN")||kw(&L->cur,"SUGTTOCIMM")||kw(&L->cur,"STACKUGTTOCN")||
      kw(&L->cur,"CMPUGTTOCN")||kw(&L->cur,"SUGTATN")||kw(&L->cur,"UGTTOCN")){
    /* i + n → cells[i]=((u)cells[i]>(u)n)?1:0, leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ((unsigned long)vm->cells[(int)i] > (unsigned long)n) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SULETOCN")||kw(&L->cur,"SULETOCIMM")||kw(&L->cur,"STACKULETOCN")||
      kw(&L->cur,"CMPULETOCN")||kw(&L->cur,"SULEATN")||kw(&L->cur,"ULETOCN")){
    /* i + n → cells[i]=((u)cells[i]<=(u)n)?1:0, leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ((unsigned long)vm->cells[(int)i] <= (unsigned long)n) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUGETOCN")||kw(&L->cur,"SUGETOCIMM")||kw(&L->cur,"STACKUGETOCN")||
      kw(&L->cur,"CMPUGETOCN")||kw(&L->cur,"SUGEATN")||kw(&L->cur,"UGETOCN")){
    /* i + n → cells[i]=((u)cells[i]>=(u)n)?1:0, leave result
     * note: SUGETOCN != SGETOCN (signed imm GE) and != SGETOC (two-arg) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ((unsigned long)vm->cells[(int)i] >= (unsigned long)n) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack↔cell 3-way imm compare TOC: SCMPTOCN · SUCMPTOCN
   * (imm dual of SCMPTOC/SUCMPTOC; peer of SEQTOCN plane) */
  if (kw(&L->cur,"SCMPTOCN")||kw(&L->cur,"SCMPTOCIMM")||kw(&L->cur,"STACKCMPTOCN")||
      kw(&L->cur,"SICMPTOCN")||kw(&L->cur,"SCMP3TOCN")||kw(&L->cur,"CMP3TOCN")||
      kw(&L->cur,"SCMPATN")){
    /* i + n → cells[i]=signed 3-way(cells[i],n) as −1/0/+1, leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = (a < n) ? -1 : ((a > n) ? 1 : 0);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUCMPTOCN")||kw(&L->cur,"SUCMPTOCIMM")||kw(&L->cur,"STACKUCMPTOCN")||
      kw(&L->cur,"SUCMP3TOCN")||kw(&L->cur,"UCMP3TOCN")||kw(&L->cur,"CMPU3TOCN")||
      kw(&L->cur,"SUCMPATN")){
    /* i + n → cells[i]=unsigned 3-way as −1/0/+1, leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long ua = (unsigned long)vm->cells[(int)i];
    unsigned long un = (unsigned long)n;
    long r = (ua < un) ? -1 : ((ua > un) ? 1 : 0);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack↔cell range dual: SLOADCELLS · SPOPCELLS · CELLXFER */
  if (kw(&L->cur,"SLOADCELLS")||kw(&L->cur,"SLOADN")||kw(&L->cur,"SPUSHCELLS")||
      kw(&L->cur,"SPUSHRANGE")||kw(&L->cur,"SLOADRANGE")||kw(&L->cur,"STACKLOADCELLS")){
    /* SLOADCELLS lo n — push cells[lo .. lo+n-1] (lo first; TOS = last) */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (lo < 0) lo = 0;
    if (lo >= CUBALC_CELL_N) lo = CUBALC_CELL_N - 1;
    if (lo + n > CUBALC_CELL_N) n = CUBALC_CELL_N - lo;
    if (vm->sp + n > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long last = 0;
    for (long k = 0; k < n; k++){
      last = vm->cells[(int)(lo + k)];
      vm->stack[vm->sp++] = last;
    }
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N", n ? last : 0); vm->last_n = n ? last : 0;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPOPCELLS")||kw(&L->cur,"SSTORECELLS")||kw(&L->cur,"SSTORERANGE")||
      kw(&L->cur,"SPOPRANGE")||kw(&L->cur,"STACKSTORECELLS")||kw(&L->cur,"SCELLSTOREN")){
    /* SPOPCELLS lo n — pop n values into cells[lo..lo+n-1]; first pop → lo+n-1 */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (lo < 0) lo = 0;
    if (lo >= CUBALC_CELL_N) lo = CUBALC_CELL_N - 1;
    if (lo + n > CUBALC_CELL_N) n = CUBALC_CELL_N - lo;
    if (vm->sp < n){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    for (long k = n - 1; k >= 0; k--){
      vm->cells[(int)(lo + k)] = vm->stack[--vm->sp];
    }
    long last = (n > 0) ? vm->cells[(int)(lo + n - 1)] : 0;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CELLXFER")||kw(&L->cur,"XFERCELL")||kw(&L->cur,"TRANSFERCELL")||
      kw(&L->cur,"MOVEAMT")||kw(&L->cur,"CELLMOVEAMT")){
    /* CELLXFER i j amt — move amt from cells[i] to cells[j] (clamp by source) */
    lex_next(L);
    long i = parse_expr(vm,L);
    long j = parse_expr(vm,L);
    long amt = parse_expr(vm,L);
    if (i < 0) i = 0;
    if (j < 0) j = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (j >= CUBALC_CELL_N) j = CUBALC_CELL_N - 1;
    if (amt < 0) amt = 0;
    long src = vm->cells[(int)i];
    long moved = amt;
    if (moved > src) moved = src;
    /* allow negative sources: only transfer positive available */
    if (src < 0) moved = 0;
    vm->cells[(int)i] = src - moved;
    vm->cells[(int)j] += moved;
    var_set_num(vm,"LAST_N",moved); vm->last_n=moved;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"CLEARCELLS")||kw(&L->cur,"CELLSZERO")){
    lex_next(L);
    memset(vm->cells, 0, sizeof vm->cells);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* INC / DEC var | CELL i — loop-friendly mutators (digit-9 data fold) */
  if (kw(&L->cur,"INC")||kw(&L->cur,"INCR")||kw(&L->cur,"++")){
    lex_next(L);
    if (kw(&L->cur,"CELL")||kw(&L->cur,"SLOT")){
      lex_next(L);
      long i = parse_expr(vm,L);
      if (i < 0) i = 0; if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
      long step = 1;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"PRINT") && !kw(&L->cur,"END")))
        step = parse_expr(vm,L);
      vm->cells[(int)i] += step;
      var_set_num(vm, "LAST_N", vm->cells[(int)i]);
      vm->last_n = vm->cells[(int)i];
    } else if (L->cur.kind==TK_IDENT){
      char name[48]; snprintf(name,sizeof name,"%s",L->cur.text); lex_next(L);
      long step = 1;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"PRINT") && !kw(&L->cur,"END")))
        step = parse_expr(vm,L);
      Var *v = var_get(vm, name, 1);
      if (v){ v->is_str=0; v->val += step; }
      long nv = v ? v->val : 0;
      var_set_num(vm, "LAST_N", nv); vm->last_n = nv;
    } else { fail(vm,"INC name|CELL i"); return -1; }
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"DEC")||kw(&L->cur,"DECR")||kw(&L->cur,"--")){
    lex_next(L);
    if (kw(&L->cur,"CELL")||kw(&L->cur,"SLOT")){
      lex_next(L);
      long i = parse_expr(vm,L);
      if (i < 0) i = 0; if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
      long step = 1;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"PRINT") && !kw(&L->cur,"END")))
        step = parse_expr(vm,L);
      vm->cells[(int)i] -= step;
      var_set_num(vm, "LAST_N", vm->cells[(int)i]);
      vm->last_n = vm->cells[(int)i];
    } else if (L->cur.kind==TK_IDENT){
      char name[48]; snprintf(name,sizeof name,"%s",L->cur.text); lex_next(L);
      long step = 1;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"PRINT") && !kw(&L->cur,"END")))
        step = parse_expr(vm,L);
      Var *v = var_get(vm, name, 1);
      if (v){ v->is_str=0; v->val -= step; }
      long nv = v ? v->val : 0;
      var_set_num(vm, "LAST_N", nv); vm->last_n = nv;
    } else { fail(vm,"DEC name|CELL i"); return -1; }
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* SUMCELL [lo [hi]] · MINCELL · MAXCELL — fold over cell bank */
  if (kw(&L->cur,"SUMCELL")||kw(&L->cur,"CELLSUM")||
      kw(&L->cur,"MINCELL")||kw(&L->cur,"CELLMIN")||
      kw(&L->cur,"MAXCELL")||kw(&L->cur,"CELLMAX")||
      kw(&L->cur,"PRODCELL")||kw(&L->cur,"CELLPROD")||kw(&L->cur,"PRODUCTCELL")||
      kw(&L->cur,"MEANCELL")||kw(&L->cur,"AVGCELL")||kw(&L->cur,"CELLMEAN")||
      kw(&L->cur,"CELLAVG")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op; *p; p++) if (*p>='a'&&*p<='z') *p = (char)(*p - 'a' + 'A');
    lex_next(L);
    long lo = 0, hi = CUBALC_CELL_N - 1;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE"))){
      lo = parse_expr(vm,L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"PRINT") && !kw(&L->cur,"END")))
        hi = parse_expr(vm,L);
      else hi = lo;
    }
    if (lo < 0) lo = 0;
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    long acc = 0;
    long prod = 1;
    int first = 1;
    long mn = 0, mx = 0;
    long n = hi - lo + 1;
    for (long i=lo; i<=hi; i++){
      long v = vm->cells[(int)i];
      acc += v;
      prod *= v;
      if (first){ mn = mx = v; first = 0; }
      else { if (v < mn) mn = v; if (v > mx) mx = v; }
    }
    long out = acc;
    if (strcmp(op,"MINCELL")==0 || strcmp(op,"CELLMIN")==0) out = first ? 0 : mn;
    else if (strcmp(op,"MAXCELL")==0 || strcmp(op,"CELLMAX")==0) out = first ? 0 : mx;
    else if (strcmp(op,"PRODCELL")==0 || strcmp(op,"CELLPROD")==0 ||
             strcmp(op,"PRODUCTCELL")==0) out = first ? 0 : prod;
    else if (strcmp(op,"MEANCELL")==0 || strcmp(op,"AVGCELL")==0 ||
             strcmp(op,"CELLMEAN")==0 || strcmp(op,"CELLAVG")==0)
      out = (n > 0) ? (acc / n) : 0;
    var_set_num(vm, "LAST_N", out);
    vm->last_n = out;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"PUSH")){
    lex_next(L);
    long v = parse_expr(vm,L);
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp++] = v;
    var_set_num(vm, "SP", vm->sp);
    var_set_num(vm, "LAST_N", v);
    vm->last_n = v;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"POP")){
    lex_next(L);
    if (vm->sp <= 0){
      var_set_num(vm, "OK", 0);
      var_set_num(vm, "LAST_N", 0);
      vm->last_n = 0;
      bump(vm); return 1;
    }
    long v = vm->stack[--vm->sp];
    var_set_num(vm, "SP", vm->sp);
    var_set_num(vm, "LAST_N", v);
    vm->last_n = v;
    var_set_num(vm, "OK", 1);
    /* optional: POP name → store into var */
    if (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
        !kw(&L->cur,"PUSH") && !kw(&L->cur,"POP") && !kw(&L->cur,"PRINT") &&
        !kw(&L->cur,"END") && !kw(&L->cur,"ELSE")){
      char name[48]; snprintf(name,sizeof name,"%s",L->cur.text); lex_next(L);
      var_set_num(vm, name, v);
    }
    bump(vm); return 1;
  }
  if (kw(&L->cur,"PEEK")){
    lex_next(L);
    long v = (vm->sp > 0) ? vm->stack[vm->sp - 1] : 0;
    var_set_num(vm, "LAST_N", v);
    vm->last_n = v;
    var_set_num(vm, "OK", vm->sp > 0 ? 1 : 0);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"CLEARSTACK")||kw(&L->cur,"STACKCLEAR")||kw(&L->cur,"DROPALL")){
    lex_next(L);
    vm->sp = 0;
    var_set_num(vm, "SP", 0);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  return 0;
}
