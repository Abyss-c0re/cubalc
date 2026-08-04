/* CubalC lang — lang_ops_toc.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_toc(VM *vm, Lex *L){
  /* plane ops_toc: L4642-10808 */
  /* ---- digit-1 data plane: cells + stack ---- */
  if (kw(&L->cur,"CELLSET")||kw(&L->cur,"SLOTSET")||kw(&L->cur,"STORE")||
      kw(&L->cur,"POKE")||kw(&L->cur,"PUTCELL")||kw(&L->cur,"SETCELL")){
    lex_next(L);
    /* SETCELL(i,v) paren form — expr-style mutator as statement */
    if (L->cur.kind==TK_LPAREN){
      lex_next(L);
      long i = parse_expr(vm,L);
      long v = 0;
      if (L->cur.kind==TK_COMMA){ lex_next(L); v = parse_expr(vm,L); }
      if (L->cur.kind==TK_RPAREN) lex_next(L);
      if (i < 0) i = 0;
      if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
      vm->cells[(int)i] = v;
      var_set_num(vm, "LAST_N", v);
      vm->last_n = v;
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    long i = parse_expr(vm,L);
    long v = 0;
    if (L->cur.kind==TK_EQ){ lex_next(L); v = parse_expr(vm,L); }
    else if (L->cur.kind!=TK_NL && L->cur.kind!=TK_EOF)
      v = parse_expr(vm,L);
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    vm->cells[(int)i] = v;
    var_set_num(vm, "LAST_N", v);
    vm->last_n = v;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"CELLGET")||kw(&L->cur,"SLOTGET")||kw(&L->cur,"LOAD")){
    lex_next(L);
    long i = parse_expr(vm,L);
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long v = vm->cells[(int)i];
    var_set_num(vm, "LAST_N", v);
    vm->last_n = v;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SWAPCELL")||kw(&L->cur,"CELLSWAP")||kw(&L->cur,"SWAPSLOT")){
    lex_next(L);
    long i = parse_expr(vm,L);
    long j = parse_expr(vm,L);
    if (i < 0) i = 0; if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (j < 0) j = 0; if (j >= CUBALC_CELL_N) j = CUBALC_CELL_N - 1;
    long t = vm->cells[(int)i];
    vm->cells[(int)i] = vm->cells[(int)j];
    vm->cells[(int)j] = t;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* digit-1 stack↔cell: SSETCELL SINCCELL SDECCELL SXCHGCELL */
  if (kw(&L->cur,"SSETCELL")||kw(&L->cur,"SSTORE")||kw(&L->cur,"SPUTCELL")||
      kw(&L->cur,"STACKSETCELL")||kw(&L->cur,"SPOKE")){
    /* i v → cells[i]=v, leave v */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    vm->cells[(int)i] = v;
    vm->stack[vm->sp++] = v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SINCCELL")||kw(&L->cur,"SINCELL")||kw(&L->cur,"STACKINCCELL")||
      kw(&L->cur,"SDECCELL")||kw(&L->cur,"SDECELL")||kw(&L->cur,"STACKDECCELL")){
    char op[24]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    int is_dec = (strcmp(op,"SDECCELL")==0 || strcmp(op,"SDECELL")==0 ||
                  strcmp(op,"STACKDECCELL")==0);
    if (is_dec) vm->cells[(int)i] -= 1;
    else vm->cells[(int)i] += 1;
    long r = vm->cells[(int)i];
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXCHGCELL")||kw(&L->cur,"SEXCHCELL")||kw(&L->cur,"SSWAPC")||
      kw(&L->cur,"STACKXCHGCELL")){
    /* i v → swap cells[i] with v, leave previous cell value */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long old = vm->cells[(int)i];
    vm->cells[(int)i] = v;
    vm->stack[vm->sp++] = old;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",old); vm->last_n=old;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack↔cell fetch: SGETCELL (complement of SSETCELL) */
  if (kw(&L->cur,"SGETCELL")||kw(&L->cur,"SLOAD")||kw(&L->cur,"SFETCH")||
      kw(&L->cur,"STACKGETCELL")||kw(&L->cur,"SPEEKCELL")){
    /* i → cells[i]  (stack index → cell value) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long v = vm->cells[(int)i];
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack↔cell accumulate: SADDTOC (i v → cells[i]+=v leave sum) */
  if (kw(&L->cur,"SADDTOC")||kw(&L->cur,"SCELLADD")||kw(&L->cur,"SACCUMCELL")||
      kw(&L->cur,"STACKADDCELL")||kw(&L->cur,"SADDTOCELL")){
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] + v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack↔cell accumulate ext: SSUBTOC · SMULTOC · SDIVTOC */
  if (kw(&L->cur,"SSUBTOC")||kw(&L->cur,"SCELLSUB")||kw(&L->cur,"SSUBFROMCELL")||
      kw(&L->cur,"STACKSUBCELL")||kw(&L->cur,"SSUBCELL")){
    /* i v → cells[i]-=v, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] - v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMULTOC")||kw(&L->cur,"SCELLMUL")||kw(&L->cur,"SMULCELL")||
      kw(&L->cur,"STACKMULCELL")||kw(&L->cur,"SMULTOCELL")){
    /* i v → cells[i]*=v, leave product */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] * v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SDIVTOC")||kw(&L->cur,"SCELLDIV")||kw(&L->cur,"SDIVCELL")||
      kw(&L->cur,"STACKDIVCELL")||kw(&L->cur,"SDIVTOCELL")){
    /* i v → cells[i]/=v (0 if v==0), leave quotient */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = v ? (vm->cells[(int)i] / v) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack↔cell accumulate bound: SMODTOC · SMINTOC · SMAXTOC */
  if (kw(&L->cur,"SMODTOC")||kw(&L->cur,"SCELLMOD")||kw(&L->cur,"SMODCELL")||
      kw(&L->cur,"STACKMODCELL")||kw(&L->cur,"SMODTOCELL")){
    /* i v → cells[i]%=v (0 if v==0), leave remainder */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = v ? (vm->cells[(int)i] % v) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMINTOC")||kw(&L->cur,"SCELLMIN")||kw(&L->cur,"SMINCELL")||
      kw(&L->cur,"STACKMINCELL")||kw(&L->cur,"SMINTOCELL")){
    /* i v → cells[i]=min(cells[i],v), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long c = vm->cells[(int)i];
    long r = (c < v) ? c : v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMAXTOC")||kw(&L->cur,"SCELLMAX")||kw(&L->cur,"SMAXCELL")||
      kw(&L->cur,"STACKMAXCELL")||kw(&L->cur,"SMAXTOCELL")){
    /* i v → cells[i]=max(cells[i],v), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long c = vm->cells[(int)i];
    long r = (c > v) ? c : v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
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
  /* digit-1 imm byte field TOC: SBYTETOCN · SSETBYTETOCN · SCLRBYTETOCN
   * (imm dual of SBYTEN/SSETBYTEN/SCLRBYTEN into cell; complete bitfield TOC with LE bytes) */
  if (kw(&L->cur,"SBYTETOCN")||kw(&L->cur,"SGETBYTETOCN")||kw(&L->cur,"STACKBYTETOCN")||
      kw(&L->cur,"SBYTETOCIMM")||kw(&L->cur,"BYTETOCN")||kw(&L->cur,"SCELLBYTEN")||
      kw(&L->cur,"GETBYTETOCN")){
    /* i + n → cells[i] = LE byte n of cells[i]; n clamped 0..7; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    long r = (long)(((unsigned long)vm->cells[(int)i] >> (unsigned)(n * 8)) & 0xFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSETBYTETOCN")||kw(&L->cur,"SSETBYTOCIMM")||kw(&L->cur,"STACKSETBYTETOCN")||
      kw(&L->cur,"SETBYTETOCN")||kw(&L->cur,"SCELLSETBYTEN")||kw(&L->cur,"PUTBYTETOCN")||
      kw(&L->cur,"SSETBYATN")){
    /* i + field + n → deposit low 8 bits of field into LE byte n of cells[i]; leave result */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long shift = (unsigned long)(n * 8);
    long r = (long)((base & ~(0xFFul << shift)) | (f << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLRBYTETOCN")||kw(&L->cur,"SCLRBYTOCIMM")||kw(&L->cur,"STACKCLRBYTETOCN")||
      kw(&L->cur,"CLRBYTETOCN")||kw(&L->cur,"SCELLCLRBYTEN")||kw(&L->cur,"ZAPBYTETOCN")||
      kw(&L->cur,"SCLRBYATN")){
    /* i + n → clear LE byte n of cells[i]; n clamped 0..7; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long shift = (unsigned long)(n * 8);
    long r = (long)(base & ~(0xFFul << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack dual byte field TOC: SBYTETOC · SSETBYTETOC · SCLRBYTETOC
   * (stack dual of SBYTETOCN/SSETBYTETOCN/SCLRBYTETOCN; LE bytes from stack) */
  if (kw(&L->cur,"SBYTETOC")||kw(&L->cur,"SGETBYTETOC")||kw(&L->cur,"STACKBYTETOC")||
      kw(&L->cur,"BYTETOC")||kw(&L->cur,"SCELLBYTES")||kw(&L->cur,"GETBYTETOC")||
      kw(&L->cur,"SBYTEAT")){
    /* i n → cells[i] = LE byte n of cells[i]; n clamped 0..7; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    long r = (long)(((unsigned long)vm->cells[(int)i] >> (unsigned)(n * 8)) & 0xFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSETBYTETOC")||kw(&L->cur,"STACKSETBYTETOC")||kw(&L->cur,"SETBYTETOC")||
      kw(&L->cur,"SCELLSETBYTES")||kw(&L->cur,"PUTBYTETOC")||kw(&L->cur,"SSETBYAT")||
      kw(&L->cur,"SDEPOSITBYTE")){
    /* i field n → deposit low 8 bits of field into LE byte n of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long field = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long f = (unsigned long)field & 0xFFul;
    unsigned long shift = (unsigned long)(n * 8);
    long r = (long)((base & ~(0xFFul << shift)) | (f << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLRBYTETOC")||kw(&L->cur,"STACKCLRBYTETOC")||kw(&L->cur,"CLRBYTETOC")||
      kw(&L->cur,"SCELLCLRBYTES")||kw(&L->cur,"ZAPBYTETOC")||kw(&L->cur,"SCLRBYAT")||
      kw(&L->cur,"SZAPBYTE")){
    /* i n → clear LE byte n of cells[i]; n clamped 0..7; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 7) n = 7;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long shift = (unsigned long)(n * 8);
    long r = (long)(base & ~(0xFFul << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 imm halfword field TOC: SWORDTOCN · SSET16TOCN · SCLR16TOCN
   * (imm dual of SWORDN/SSET16N/SCLR16N into cell; complete byte TOC ladder with 16-bit) */
  if (kw(&L->cur,"SWORDTOCN")||kw(&L->cur,"SGET16TOCN")||kw(&L->cur,"STACKWORDTOCN")||
      kw(&L->cur,"SWORDTOCIMM")||kw(&L->cur,"WORDTOCN")||kw(&L->cur,"SCELLWORDN")||
      kw(&L->cur,"SHALFTOCN")||kw(&L->cur,"GET16TOCN")){
    /* i + n → cells[i] = LE 16-bit halfword n of cells[i]; n clamped 0..3; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    long r = (long)(((unsigned long)vm->cells[(int)i] >> (unsigned)(n * 16)) & 0xFFFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSET16TOCN")||kw(&L->cur,"SSET16TOCIMM")||kw(&L->cur,"STACKSET16TOCN")||
      kw(&L->cur,"SET16TOCN")||kw(&L->cur,"SCELLSET16N")||kw(&L->cur,"SSETWORDTOCN")||
      kw(&L->cur,"PUT16TOCN")||kw(&L->cur,"SSETHALFTOCN")){
    /* i + field + n → deposit low 16 bits of field into LE halfword n of cells[i] */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long shift = (unsigned long)(n * 16);
    long r = (long)((base & ~(0xFFFFul << shift)) | (f << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLR16TOCN")||kw(&L->cur,"SCLR16TOCIMM")||kw(&L->cur,"STACKCLR16TOCN")||
      kw(&L->cur,"CLR16TOCN")||kw(&L->cur,"SCELLCLR16N")||kw(&L->cur,"SCLRWORDTOCN")||
      kw(&L->cur,"ZAP16TOCN")||kw(&L->cur,"SCLRHAFTOCN")){
    /* i + n → clear LE halfword n of cells[i]; n clamped 0..3; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long shift = (unsigned long)(n * 16);
    long r = (long)(base & ~(0xFFFFul << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack dual halfword field TOC: SWORDTOC · SSET16TOC · SCLR16TOC
   * (stack dual of SWORDTOCN/SSET16TOCN/SCLR16TOCN; LE 16-bit after SBYTETOC plane) */
  if (kw(&L->cur,"SWORDTOC")||kw(&L->cur,"SGET16TOC")||kw(&L->cur,"STACKWORDTOC")||
      kw(&L->cur,"WORDTOC")||kw(&L->cur,"SCELLWORDS")||kw(&L->cur,"SHALFWORDTOC")||
      kw(&L->cur,"GET16TOC")||kw(&L->cur,"SWORDAT")){
    /* i n → cells[i] = LE 16-bit halfword n of cells[i]; n clamped 0..3; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    long r = (long)(((unsigned long)vm->cells[(int)i] >> (unsigned)(n * 16)) & 0xFFFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSET16TOC")||kw(&L->cur,"STACKSET16TOC")||kw(&L->cur,"SET16TOC")||
      kw(&L->cur,"SCELLSET16S")||kw(&L->cur,"SSETWORDTOC")||kw(&L->cur,"PUT16TOC")||
      kw(&L->cur,"SSETHALFTOC")||kw(&L->cur,"SSET16AT")){
    /* i field n → deposit low 16 bits of field into LE halfword n of cells[i] */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long field = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long f = (unsigned long)field & 0xFFFFul;
    unsigned long shift = (unsigned long)(n * 16);
    long r = (long)((base & ~(0xFFFFul << shift)) | (f << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLR16TOC")||kw(&L->cur,"STACKCLR16TOC")||kw(&L->cur,"CLR16TOC")||
      kw(&L->cur,"SCELLCLR16S")||kw(&L->cur,"SCLRWORDTOC")||kw(&L->cur,"ZAP16TOC")||
      kw(&L->cur,"SCLRHAFTOC")||kw(&L->cur,"SCLR16AT")){
    /* i n → clear LE halfword n of cells[i]; n clamped 0..3; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 3) n = 3;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long shift = (unsigned long)(n * 16);
    long r = (long)(base & ~(0xFFFFul << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 imm ceil/floor div TOC: SDIVCEILTOCN · SDIVFLOORTOCN
   * (imm dual of SDIVCEILN/SDIVFLOORN into cell; complete SDIVTOCN plane with rounding modes) */
  if (kw(&L->cur,"SDIVCEILTOCN")||kw(&L->cur,"SCEILDIVTOCN")||kw(&L->cur,"STACKDIVCEILTOCN")||
      kw(&L->cur,"SDIVCEILTOCIMM")||kw(&L->cur,"CEILDIVTOCN")||kw(&L->cur,"SCELLDIVCEILN")||
      kw(&L->cur,"CEILTOCN")){
    /* i + n → cells[i] = ceil(cells[i]/n); n==0 → 0 soft; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (n == 0) r = 0;
    else if (a >= 0 && n > 0) r = (a + n - 1) / n;
    else if (a <= 0 && n < 0){
      long aa = -a, nn = -n;
      r = (aa + nn - 1) / nn;
    } else r = a / n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SDIVFLOORTOCN")||kw(&L->cur,"SFLOORDIVTOCN")||kw(&L->cur,"STACKDIVFLOORTOCN")||
      kw(&L->cur,"SDIVFLOORTOCIMM")||kw(&L->cur,"FLOORDIVTOCN")||kw(&L->cur,"SCELLDIVFLOORN")||
      kw(&L->cur,"FLOORTOCN")){
    /* i + n → cells[i] = floor(cells[i]/n); n==0 → 0 soft; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (n == 0) r = 0;
    else {
      long q = a / n, rem = a % n;
      if (rem != 0 && ((a < 0) != (n < 0))) q--;
      r = q;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 imm nibble field TOC: SNIBTOCN · SSETNIBTOCN · SCLRNIBTOCN
   * (imm dual of SNIBN/SSETNIBN/SCLRNIBN into cell; complete field ladder 4/8/16) */
  if (kw(&L->cur,"SNIBTOCN")||kw(&L->cur,"SGETNIBTOCN")||kw(&L->cur,"STACKNIBTOCN")||
      kw(&L->cur,"SNIBTOCIMM")||kw(&L->cur,"NIBTOCN")||kw(&L->cur,"SCELLNIBN")||
      kw(&L->cur,"GETNIBTOCN")||kw(&L->cur,"SNIBBLETOCN")){
    /* i + n → cells[i] = LE nibble n of cells[i]; n clamped 0..15; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    long r = (long)(((unsigned long)vm->cells[(int)i] >> (unsigned)(n * 4)) & 0xFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSETNIBTOCN")||kw(&L->cur,"SSETNIBTOCIMM")||kw(&L->cur,"STACKSETNIBTOCN")||
      kw(&L->cur,"SETNIBTOCN")||kw(&L->cur,"SCELLSETNIBN")||kw(&L->cur,"PUTNIBTOCN")||
      kw(&L->cur,"SSETNIBATN")){
    /* i + field + n → deposit low 4 bits of field into LE nibble n of cells[i] */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long shift = (unsigned long)(n * 4);
    long r = (long)((base & ~(0xFul << shift)) | (f << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLRNIBTOCN")||kw(&L->cur,"SCLRNIBTOCIMM")||kw(&L->cur,"STACKCLRNIBTOCN")||
      kw(&L->cur,"CLRNIBTOCN")||kw(&L->cur,"SCELLCLRNIBN")||kw(&L->cur,"ZAPNIBTOCN")||
      kw(&L->cur,"SCLRNIBATN")){
    /* i + n → clear LE nibble n of cells[i]; n clamped 0..15; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long shift = (unsigned long)(n * 4);
    long r = (long)(base & ~(0xFul << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack dual nibble field TOC: SNIBTOC · SSETNIBTOC · SCLRNIBTOC
   * (stack dual of SNIBTOCN/SSETNIBTOCN/SCLRNIBTOCN; foundation completes 4/8/16/32 stack ladder) */
  if (kw(&L->cur,"SNIBTOC")||kw(&L->cur,"SGETNIBTOC")||kw(&L->cur,"STACKNIBTOC")||
      kw(&L->cur,"NIBTOC")||kw(&L->cur,"SCELLNIBS")||kw(&L->cur,"GETNIBTOC")||
      kw(&L->cur,"SNIBBLETOC")||kw(&L->cur,"SNIBAT")){
    /* i n → cells[i] = LE nibble n of cells[i]; n clamped 0..15; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    long r = (long)(((unsigned long)vm->cells[(int)i] >> (unsigned)(n * 4)) & 0xFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSETNIBTOC")||kw(&L->cur,"STACKSETNIBTOC")||kw(&L->cur,"SETNIBTOC")||
      kw(&L->cur,"SCELLSETNIBS")||kw(&L->cur,"PUTNIBTOC")||kw(&L->cur,"SSETNIBAT")||
      kw(&L->cur,"SDEPOSITNIB")){
    /* i field n → deposit low 4 bits of field into LE nibble n of cells[i] */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long field = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long f = (unsigned long)field & 0xFul;
    unsigned long shift = (unsigned long)(n * 4);
    long r = (long)((base & ~(0xFul << shift)) | (f << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLRNIBTOC")||kw(&L->cur,"STACKCLRNIBTOC")||kw(&L->cur,"CLRNIBTOC")||
      kw(&L->cur,"SCELLCLRNIBS")||kw(&L->cur,"ZAPNIBTOC")||kw(&L->cur,"SCLRNIBAT")||
      kw(&L->cur,"SZAPNIB")){
    /* i n → clear LE nibble n of cells[i]; n clamped 0..15; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 15) n = 15;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long shift = (unsigned long)(n * 4);
    long r = (long)(base & ~(0xFul << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack dual sign/zero extend TOC: SSEXTTOC · SZEXTTOC · SSEXT8TOC
   * (stack dual of SSEXT/SZEXT/SSEXT8 into cell; width from stack after field ladder) */
  if (kw(&L->cur,"SSEXTTOC")||kw(&L->cur,"SEXTTOC")||kw(&L->cur,"SSIGNEXTTOC")||
      kw(&L->cur,"STACKSEXTTOC")||kw(&L->cur,"SCELLSEXT")||kw(&L->cur,"SIGNEXTTOC")||
      kw(&L->cur,"SSEXTAT")){
    /* i w → cells[i] = sign-extend low w bits of cells[i]; w 0..63; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long w = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (w <= 0) r = 0;
    else if (w >= 63) r = a;
    else {
      unsigned long mask = (1ul << (unsigned)w) - 1ul;
      unsigned long v = (unsigned long)a & mask;
      unsigned long sign = 1ul << (unsigned)(w - 1);
      if (v & sign) v |= ~mask;
      r = (long)v;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SZEXTTOC")||kw(&L->cur,"ZEXTTOC")||kw(&L->cur,"SZEROEXTTOC")||
      kw(&L->cur,"STACKZEXTTOC")||kw(&L->cur,"SCELLZEXT")||kw(&L->cur,"ZEROEXTTOC")||
      kw(&L->cur,"SZEXTAT")){
    /* i w → cells[i] = zero-extend low w bits of cells[i]; w 0..63; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long w = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (w <= 0) r = 0;
    else if (w >= 63) r = a;
    else {
      unsigned long mask = (1ul << (unsigned)w) - 1ul;
      r = (long)((unsigned long)a & mask);
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEXT8TOC")||kw(&L->cur,"SEXT8TOC")||kw(&L->cur,"SSEXTBTOC")||
      kw(&L->cur,"STACKSEXT8TOC")||kw(&L->cur,"SCELLSEXT8")||kw(&L->cur,"SIGNEXT8TOC")||
      kw(&L->cur,"SSEXT8AT")){
    /* i → cells[i] = sign-extend low 8 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = a & 0xFFL;
    if (r & 0x80L) r |= ~0xFFL;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 imm dual sign/zero extend TOC: SSEXTTOCN · SZEXTTOCN · SSEXT16TOCN
   * (imm dual of SSEXTTOC/SZEXTTOC; fixed 16-bit path after SSEXT8TOC plane) */
  if (kw(&L->cur,"SSEXTTOCN")||kw(&L->cur,"SEXTTOCN")||kw(&L->cur,"SSIGNEXTTOCN")||
      kw(&L->cur,"SSEXTTOCIMM")||kw(&L->cur,"STACKSEXTTOCN")||kw(&L->cur,"SCELLSEXTN")||
      kw(&L->cur,"SIGNEXTTOCN")){
    /* i + w → cells[i] = sign-extend low w bits of cells[i]; w 0..63; leave result */
    lex_next(L);
    long w = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (w <= 0) r = 0;
    else if (w >= 63) r = a;
    else {
      unsigned long mask = (1ul << (unsigned)w) - 1ul;
      unsigned long v = (unsigned long)a & mask;
      unsigned long sign = 1ul << (unsigned)(w - 1);
      if (v & sign) v |= ~mask;
      r = (long)v;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SZEXTTOCN")||kw(&L->cur,"ZEXTTOCN")||kw(&L->cur,"SZEROEXTTOCN")||
      kw(&L->cur,"SZEXTTOCIMM")||kw(&L->cur,"STACKZEXTTOCN")||kw(&L->cur,"SCELLZEXTN")||
      kw(&L->cur,"ZEROEXTTOCN")){
    /* i + w → cells[i] = zero-extend low w bits of cells[i]; w 0..63; leave result */
    lex_next(L);
    long w = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (w <= 0) r = 0;
    else if (w >= 63) r = a;
    else {
      unsigned long mask = (1ul << (unsigned)w) - 1ul;
      r = (long)((unsigned long)a & mask);
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEXT16TOCN")||kw(&L->cur,"SEXT16TOCN")||kw(&L->cur,"SSEXTWTOCN")||
      kw(&L->cur,"STACKSEXT16TOCN")||kw(&L->cur,"SCELLSEXT16N")||kw(&L->cur,"SIGNEXT16TOCN")||
      kw(&L->cur,"SSEXT16TOCIMM")){
    /* i → cells[i] = sign-extend low 16 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = a & 0xFFFFL;
    if (r & 0x8000L) r |= ~0xFFFFL;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack dual fixed-width extend TOC: SSEXT16TOC · SSEXT32TOC · SZEXT8TOC
   * (complete 8/16/32 fixed extend plane after SSEXT8TOC/SSEXT16TOCN) */
  if (kw(&L->cur,"SSEXT16TOC")||kw(&L->cur,"SEXT16TOC")||kw(&L->cur,"SSEXTWTOC")||
      kw(&L->cur,"STACKSEXT16TOC")||kw(&L->cur,"SCELLSEXT16")||kw(&L->cur,"SIGNEXT16TOC")||
      kw(&L->cur,"SSEXT16AT")){
    /* i → cells[i] = sign-extend low 16 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = a & 0xFFFFL;
    if (r & 0x8000L) r |= ~0xFFFFL;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEXT32TOC")||kw(&L->cur,"SEXT32TOC")||kw(&L->cur,"SSEXTLTOC")||
      kw(&L->cur,"STACKSEXT32TOC")||kw(&L->cur,"SCELLSEXT32")||kw(&L->cur,"SIGNEXT32TOC")||
      kw(&L->cur,"SSEXT32AT")||kw(&L->cur,"SSEXTDTOC")){
    /* i → cells[i] = sign-extend low 32 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = (long)(int)(unsigned int)a;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SZEXT8TOC")||kw(&L->cur,"ZEXT8TOC")||kw(&L->cur,"SZEXTBTOC")||
      kw(&L->cur,"STACKZEXT8TOC")||kw(&L->cur,"SCELLZEXT8")||kw(&L->cur,"ZEROEXT8TOC")||
      kw(&L->cur,"SZEXT8AT")){
    /* i → cells[i] = zero-extend low 8 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (long)((unsigned long)vm->cells[(int)i] & 0xFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack dual fixed zext + nibble sext TOC: SZEXT16TOC · SZEXT32TOC · SSEXT4TOC
   * (complete zext 8/16/32 plane after SZEXT8TOC; nibble sext closes 4-bit extend) */
  if (kw(&L->cur,"SZEXT16TOC")||kw(&L->cur,"ZEXT16TOC")||kw(&L->cur,"SZEXTWTOC")||
      kw(&L->cur,"STACKZEXT16TOC")||kw(&L->cur,"SCELLZEXT16")||kw(&L->cur,"ZEROEXT16TOC")||
      kw(&L->cur,"SZEXT16AT")){
    /* i → cells[i] = zero-extend low 16 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (long)((unsigned long)vm->cells[(int)i] & 0xFFFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SZEXT32TOC")||kw(&L->cur,"ZEXT32TOC")||kw(&L->cur,"SZEXTLTOC")||
      kw(&L->cur,"STACKZEXT32TOC")||kw(&L->cur,"SCELLZEXT32")||kw(&L->cur,"ZEROEXT32TOC")||
      kw(&L->cur,"SZEXT32AT")||kw(&L->cur,"SZEXTDTOC")){
    /* i → cells[i] = zero-extend low 32 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (long)((unsigned long)vm->cells[(int)i] & 0xFFFFFFFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEXT4TOC")||kw(&L->cur,"SEXT4TOC")||kw(&L->cur,"SSEXTNTOC")||
      kw(&L->cur,"STACKSEXT4TOC")||kw(&L->cur,"SCELLSEXT4")||kw(&L->cur,"SIGNEXT4TOC")||
      kw(&L->cur,"SSEXT4AT")||kw(&L->cur,"SNIBSEXTTOC")){
    /* i → cells[i] = sign-extend low 4 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = a & 0xFL;
    if (r & 0x8L) r |= ~0xFL;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 imm dual fixed nibble/byte extend TOC: SSEXT8TOCN · SSEXT4TOCN · SZEXT4TOCN
   * (imm dual of SSEXT8TOC/SSEXT4TOC; nibble zext closes 4-bit zext plane) */
  if (kw(&L->cur,"SSEXT8TOCN")||kw(&L->cur,"SEXT8TOCN")||kw(&L->cur,"SSEXTBTOCN")||
      kw(&L->cur,"STACKSEXT8TOCN")||kw(&L->cur,"SCELLSEXT8N")||kw(&L->cur,"SIGNEXT8TOCN")||
      kw(&L->cur,"SSEXT8TOCIMM")){
    /* i → cells[i] = sign-extend low 8 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = a & 0xFFL;
    if (r & 0x80L) r |= ~0xFFL;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEXT4TOCN")||kw(&L->cur,"SEXT4TOCN")||kw(&L->cur,"SSEXTNTOCN")||
      kw(&L->cur,"STACKSEXT4TOCN")||kw(&L->cur,"SCELLSEXT4N")||kw(&L->cur,"SIGNEXT4TOCN")||
      kw(&L->cur,"SSEXT4TOCIMM")||kw(&L->cur,"SNIBSEXTTOCN")){
    /* i → cells[i] = sign-extend low 4 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = a & 0xFL;
    if (r & 0x8L) r |= ~0xFL;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SZEXT4TOCN")||kw(&L->cur,"ZEXT4TOCN")||kw(&L->cur,"SZEXTNTOCN")||
      kw(&L->cur,"STACKZEXT4TOCN")||kw(&L->cur,"SCELLZEXT4N")||kw(&L->cur,"ZEROEXT4TOCN")||
      kw(&L->cur,"SZEXT4TOCIMM")||kw(&L->cur,"SNIBZEXTTOCN")){
    /* i → cells[i] = zero-extend low 4 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (long)((unsigned long)vm->cells[(int)i] & 0xFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack/imm zext dual ladder: SZEXT4TOC · SZEXT8TOCN · SZEXT16TOCN
   * (stack dual of SZEXT4TOCN; imm dual of SZEXT8TOC/SZEXT16TOC after nibble plane) */
  if (kw(&L->cur,"SZEXT4TOC")||kw(&L->cur,"ZEXT4TOC")||kw(&L->cur,"SZEXTNTOC")||
      kw(&L->cur,"STACKZEXT4TOC")||kw(&L->cur,"SCELLZEXT4")||kw(&L->cur,"ZEROEXT4TOC")||
      kw(&L->cur,"SZEXT4AT")||kw(&L->cur,"SNIBZEXTTOC")){
    /* i → cells[i] = zero-extend low 4 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (long)((unsigned long)vm->cells[(int)i] & 0xFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SZEXT8TOCN")||kw(&L->cur,"ZEXT8TOCN")||kw(&L->cur,"SZEXTBTOCN")||
      kw(&L->cur,"STACKZEXT8TOCN")||kw(&L->cur,"SCELLZEXT8N")||kw(&L->cur,"ZEROEXT8TOCN")||
      kw(&L->cur,"SZEXT8TOCIMM")){
    /* i → cells[i] = zero-extend low 8 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (long)((unsigned long)vm->cells[(int)i] & 0xFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SZEXT16TOCN")||kw(&L->cur,"ZEXT16TOCN")||kw(&L->cur,"SZEXTWTOCN")||
      kw(&L->cur,"STACKZEXT16TOCN")||kw(&L->cur,"SCELLZEXT16N")||kw(&L->cur,"ZEROEXT16TOCN")||
      kw(&L->cur,"SZEXT16TOCIMM")){
    /* i → cells[i] = zero-extend low 16 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (long)((unsigned long)vm->cells[(int)i] & 0xFFFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 imm dual 32-bit extend + clip8 TOC: SZEXT32TOCN · SSEXT32TOCN · SCLIP8TOC
   * (complete zext/sext imm 4/8/16/32 ladder; clip dual of CLIP8 into cell) */
  if (kw(&L->cur,"SZEXT32TOCN")||kw(&L->cur,"ZEXT32TOCN")||kw(&L->cur,"SZEXTLTOCN")||
      kw(&L->cur,"STACKZEXT32TOCN")||kw(&L->cur,"SCELLZEXT32N")||kw(&L->cur,"ZEROEXT32TOCN")||
      kw(&L->cur,"SZEXT32TOCIMM")||kw(&L->cur,"SZEXTDTOCN")){
    /* i → cells[i] = zero-extend low 32 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = (long)((unsigned long)vm->cells[(int)i] & 0xFFFFFFFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSEXT32TOCN")||kw(&L->cur,"SEXT32TOCN")||kw(&L->cur,"SSEXTLTOCN")||
      kw(&L->cur,"STACKSEXT32TOCN")||kw(&L->cur,"SCELLSEXT32N")||kw(&L->cur,"SIGNEXT32TOCN")||
      kw(&L->cur,"SSEXT32TOCIMM")||kw(&L->cur,"SSEXTDTOCN")){
    /* i → cells[i] = sign-extend low 32 bits of cells[i]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = (long)(int)(unsigned int)a;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIP8TOC")||kw(&L->cur,"CLIP8TOC")||kw(&L->cur,"SCLIPBTOC")||
      kw(&L->cur,"STACKCLIP8TOC")||kw(&L->cur,"SCELLCLIP8")||kw(&L->cur,"UCLIP8TOC")||
      kw(&L->cur,"SCLIP8AT")){
    /* i → cells[i] = clamp cells[i] to unsigned 8-bit [0,255]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = a;
    if (r < 0) r = 0;
    if (r > 255) r = 255;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 stack dual fixed clip TOC: SCLIP4TOC · SCLIP16TOC · SCLIP32TOC
   * (complete unsigned clip 4/8/16/32 plane after SCLIP8TOC; dual of CLIP4/16/32) */
  if (kw(&L->cur,"SCLIP4TOC")||kw(&L->cur,"CLIP4TOC")||kw(&L->cur,"SCLIPNTOC")||
      kw(&L->cur,"STACKCLIP4TOC")||kw(&L->cur,"SCELLCLIP4")||kw(&L->cur,"UCLIP4TOC")||
      kw(&L->cur,"SCLIP4AT")){
    /* i → cells[i] = clamp cells[i] to unsigned nibble [0,15]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < 0) r = 0;
    if (r > 15) r = 15;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIP16TOC")||kw(&L->cur,"CLIP16TOC")||kw(&L->cur,"SCLIPWTOC")||
      kw(&L->cur,"STACKCLIP16TOC")||kw(&L->cur,"SCELLCLIP16")||kw(&L->cur,"UCLIP16TOC")||
      kw(&L->cur,"SCLIP16AT")){
    /* i → cells[i] = clamp cells[i] to unsigned 16-bit [0,65535]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < 0) r = 0;
    if (r > 65535L) r = 65535L;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIP32TOC")||kw(&L->cur,"CLIP32TOC")||kw(&L->cur,"SCLIPLTOC")||
      kw(&L->cur,"STACKCLIP32TOC")||kw(&L->cur,"SCELLCLIP32")||kw(&L->cur,"UCLIP32TOC")||
      kw(&L->cur,"SCLIP32AT")||kw(&L->cur,"SCLIPDTOC")){
    /* i → cells[i] = clamp cells[i] to unsigned 32-bit [0,0xFFFFFFFF]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < 0) r = 0;
    if ((unsigned long)r > 0xFFFFFFFFul) r = (long)0xFFFFFFFFul;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 imm dual fixed clip TOC: SCLIP4TOCN · SCLIP8TOCN · SCLIP16TOCN
   * (imm dual of SCLIP4TOC/SCLIP8TOC/SCLIP16TOC; keep i on stack) */
  if (kw(&L->cur,"SCLIP4TOCN")||kw(&L->cur,"CLIP4TOCN")||kw(&L->cur,"SCLIPNTOCN")||
      kw(&L->cur,"STACKCLIP4TOCN")||kw(&L->cur,"SCELLCLIP4N")||kw(&L->cur,"UCLIP4TOCN")||
      kw(&L->cur,"SCLIP4TOCIMM")){
    /* i → cells[i] = clamp to [0,15]; leave result on stack (i replaced) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < 0) r = 0;
    if (r > 15) r = 15;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIP8TOCN")||kw(&L->cur,"CLIP8TOCN")||kw(&L->cur,"SCLIPBTOCN")||
      kw(&L->cur,"STACKCLIP8TOCN")||kw(&L->cur,"SCELLCLIP8N")||kw(&L->cur,"UCLIP8TOCN")||
      kw(&L->cur,"SCLIP8TOCIMM")){
    /* i → cells[i] = clamp to [0,255]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < 0) r = 0;
    if (r > 255) r = 255;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIP16TOCN")||kw(&L->cur,"CLIP16TOCN")||kw(&L->cur,"SCLIPWTOCN")||
      kw(&L->cur,"STACKCLIP16TOCN")||kw(&L->cur,"SCELLCLIP16N")||kw(&L->cur,"UCLIP16TOCN")||
      kw(&L->cur,"SCLIP16TOCIMM")){
    /* i → cells[i] = clamp to [0,65535]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < 0) r = 0;
    if (r > 65535L) r = 65535L;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 imm clip32 + signed clip TOC: SCLIP32TOCN · SCLIPS8TOC · SCLIPS16TOC
   * (complete uclip imm 4/8/16/32; signed clamp duals of CLIP into cell) */
  if (kw(&L->cur,"SCLIP32TOCN")||kw(&L->cur,"CLIP32TOCN")||kw(&L->cur,"SCLIPLTOCN")||
      kw(&L->cur,"STACKCLIP32TOCN")||kw(&L->cur,"SCELLCLIP32N")||kw(&L->cur,"UCLIP32TOCN")||
      kw(&L->cur,"SCLIP32TOCIMM")||kw(&L->cur,"SCLIPDTOCN")){
    /* i → cells[i] = clamp to [0,0xFFFFFFFF]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < 0) r = 0;
    if ((unsigned long)r > 0xFFFFFFFFul) r = (long)0xFFFFFFFFul;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIPS8TOC")||kw(&L->cur,"CLIPS8TOC")||kw(&L->cur,"SSCLIP8TOC")||
      kw(&L->cur,"STACKSCLIPS8TOC")||kw(&L->cur,"SCELLSCLIP8")||kw(&L->cur,"SCLIPBSTO")||
      kw(&L->cur,"SCLIPS8AT")){
    /* i → cells[i] = clamp to signed 8-bit [-128,127]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < -128L) r = -128L;
    if (r > 127L) r = 127L;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIPS16TOC")||kw(&L->cur,"CLIPS16TOC")||kw(&L->cur,"SSCLIP16TOC")||
      kw(&L->cur,"STACKSCLIPS16TOC")||kw(&L->cur,"SCELLSCLIP16")||kw(&L->cur,"SCLIPWSTO")||
      kw(&L->cur,"SCLIPS16AT")){
    /* i → cells[i] = clamp to signed 16-bit [-32768,32767]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < -32768L) r = -32768L;
    if (r > 32767L) r = 32767L;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 signed clip 4/32 + imm signed8 TOC: SCLIPS4TOC · SCLIPS32TOC · SCLIPS8TOCN
   * (complete signed clip 4/8/16/32 plane; imm dual of SCLIPS8TOC) */
  if (kw(&L->cur,"SCLIPS4TOC")||kw(&L->cur,"CLIPS4TOC")||kw(&L->cur,"SSCLIP4TOC")||
      kw(&L->cur,"STACKSCLIPS4TOC")||kw(&L->cur,"SCELLSCLIP4")||kw(&L->cur,"SCLIPNSTO")||
      kw(&L->cur,"SCLIPS4AT")){
    /* i → cells[i] = clamp to signed 4-bit [-8,7]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < -8L) r = -8L;
    if (r > 7L) r = 7L;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIPS32TOC")||kw(&L->cur,"CLIPS32TOC")||kw(&L->cur,"SSCLIP32TOC")||
      kw(&L->cur,"STACKSCLIPS32TOC")||kw(&L->cur,"SCELLSCLIP32")||kw(&L->cur,"SCLIPLSTO")||
      kw(&L->cur,"SCLIPS32AT")||kw(&L->cur,"SCLIPSDTOC")){
    /* i → cells[i] = clamp to signed 32-bit [INT32_MIN,INT32_MAX]; leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < (-2147483647L - 1)) r = (-2147483647L - 1);
    if (r > 2147483647L) r = 2147483647L;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIPS8TOCN")||kw(&L->cur,"CLIPS8TOCN")||kw(&L->cur,"SSCLIP8TOCN")||
      kw(&L->cur,"STACKSCLIPS8TOCN")||kw(&L->cur,"SCELLSCLIP8N")||kw(&L->cur,"SCLIPBSTON")||
      kw(&L->cur,"SCLIPS8TOCIMM")){
    /* i → cells[i] = clamp to signed 8-bit [-128,127]; leave result (i replaced) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < -128L) r = -128L;
    if (r > 127L) r = 127L;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 imm dual signed clip TOC: SCLIPS4TOCN · SCLIPS16TOCN · SCLIPS32TOCN
   * (imm dual of SCLIPS4/16/32TOC; complete signed clip imm plane with SCLIPS8TOCN) */
  if (kw(&L->cur,"SCLIPS4TOCN")||kw(&L->cur,"CLIPS4TOCN")||kw(&L->cur,"SSCLIP4TOCN")||
      kw(&L->cur,"STACKSCLIPS4TOCN")||kw(&L->cur,"SCELLSCLIP4N")||kw(&L->cur,"SCLIPNSTON")||
      kw(&L->cur,"SCLIPS4TOCIMM")){
    /* i → cells[i] = clamp to signed 4-bit [-8,7]; leave result (i replaced) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < -8L) r = -8L;
    if (r > 7L) r = 7L;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIPS16TOCN")||kw(&L->cur,"CLIPS16TOCN")||kw(&L->cur,"SSCLIP16TOCN")||
      kw(&L->cur,"STACKSCLIPS16TOCN")||kw(&L->cur,"SCELLSCLIP16N")||kw(&L->cur,"SCLIPWSTON")||
      kw(&L->cur,"SCLIPS16TOCIMM")){
    /* i → cells[i] = clamp to signed 16-bit [-32768,32767]; leave result (i replaced) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < -32768L) r = -32768L;
    if (r > 32767L) r = 32767L;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLIPS32TOCN")||kw(&L->cur,"CLIPS32TOCN")||kw(&L->cur,"SSCLIP32TOCN")||
      kw(&L->cur,"STACKSCLIPS32TOCN")||kw(&L->cur,"SCELLSCLIP32N")||kw(&L->cur,"SCLIPLSTON")||
      kw(&L->cur,"SCLIPS32TOCIMM")||kw(&L->cur,"SCLIPSDTOCN")){
    /* i → cells[i] = clamp to signed 32-bit [INT32_MIN,INT32_MAX]; leave result (i replaced) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i];
    if (r < (-2147483647L - 1)) r = (-2147483647L - 1);
    if (r > 2147483647L) r = 2147483647L;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 imm 32-bit field TOC: SGET32TOCN · SSET32TOCN · SCLR32TOCN
   * (imm dual of SGET32N/SSET32N/SCLR32N into cell; complete 4/8/16/32 field ladder) */
  if (kw(&L->cur,"SGET32TOCN")||kw(&L->cur,"SWORD32TOCN")||kw(&L->cur,"STACKGET32TOCN")||
      kw(&L->cur,"SGET32TOCIMM")||kw(&L->cur,"GET32TOCN")||kw(&L->cur,"SCELLGET32N")||
      kw(&L->cur,"SDWORDTOCN")||kw(&L->cur,"WORD32TOCN")){
    /* i + n → cells[i] = LE 32-bit word n of cells[i]; n clamped 0..1; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    long r = (long)(((unsigned long)vm->cells[(int)i] >> (unsigned)(n * 32)) & 0xFFFFFFFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSET32TOCN")||kw(&L->cur,"SSET32TOCIMM")||kw(&L->cur,"STACKSET32TOCN")||
      kw(&L->cur,"SET32TOCN")||kw(&L->cur,"SCELLSET32N")||kw(&L->cur,"SSETDWORDTOCN")||
      kw(&L->cur,"PUT32TOCN")||kw(&L->cur,"SSETW32TOCN")){
    /* i + field + n → deposit low 32 bits of field into LE word n of cells[i] */
    lex_next(L);
    long field = parse_expr(vm,L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long shift = (unsigned long)(n * 32);
    long r = (long)((base & ~(0xFFFFFFFFul << shift)) | (f << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLR32TOCN")||kw(&L->cur,"SCLR32TOCIMM")||kw(&L->cur,"STACKCLR32TOCN")||
      kw(&L->cur,"CLR32TOCN")||kw(&L->cur,"SCELLCLR32N")||kw(&L->cur,"SCLRDWORDTOCN")||
      kw(&L->cur,"ZAP32TOCN")||kw(&L->cur,"SCLRW32TOCN")){
    /* i + n → clear LE 32-bit word n of cells[i]; n clamped 0..1; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long shift = (unsigned long)(n * 32);
    long r = (long)(base & ~(0xFFFFFFFFul << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack dual 32-bit field TOC: SGET32TOC · SSET32TOC · SCLR32TOC
   * (stack dual of SGET32TOCN/SSET32TOCN/SCLR32TOCN; foundation complete field ladder) */
  if (kw(&L->cur,"SGET32TOC")||kw(&L->cur,"SWORD32TOC")||kw(&L->cur,"STACKGET32TOC")||
      kw(&L->cur,"GET32TOC")||kw(&L->cur,"SCELLGET32S")||kw(&L->cur,"SDWORDTOC")||
      kw(&L->cur,"WORD32TOC")||kw(&L->cur,"SGET32AT")){
    /* i n → cells[i] = LE 32-bit word n of cells[i]; n clamped 0..1; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    long r = (long)(((unsigned long)vm->cells[(int)i] >> (unsigned)(n * 32)) & 0xFFFFFFFFul);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSET32TOC")||kw(&L->cur,"STACKSET32TOC")||kw(&L->cur,"SET32TOC")||
      kw(&L->cur,"SCELLSET32S")||kw(&L->cur,"SSETDWORDTOC")||kw(&L->cur,"PUT32TOC")||
      kw(&L->cur,"SSETW32TOC")||kw(&L->cur,"SSET32AT")){
    /* i field n → deposit low 32 bits of field into LE word n of cells[i] */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long field = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long f = (unsigned long)field & 0xFFFFFFFFul;
    unsigned long shift = (unsigned long)(n * 32);
    long r = (long)((base & ~(0xFFFFFFFFul << shift)) | (f << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLR32TOC")||kw(&L->cur,"STACKCLR32TOC")||kw(&L->cur,"CLR32TOC")||
      kw(&L->cur,"SCELLCLR32S")||kw(&L->cur,"SCLRDWORDTOC")||kw(&L->cur,"ZAP32TOC")||
      kw(&L->cur,"SCLRW32TOC")||kw(&L->cur,"SCLR32AT")){
    /* i n → clear LE 32-bit word n of cells[i]; n clamped 0..1; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 1) n = 1;
    unsigned long base = (unsigned long)vm->cells[(int)i];
    unsigned long shift = (unsigned long)(n * 32);
    long r = (long)(base & ~(0xFFFFFFFFul << shift));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
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
  /* digit-3 imm low-n reverse/rotate TOC: SBREVTOCN · SROLBTOCN · SRORBTOCN
   * (imm dual of SBREVN/SROLBN/SRORBN into cell after SCLRMNTOCN plane) */
  if (kw(&L->cur,"SBREVTOCN")||kw(&L->cur,"SBREVTOCIMM")||kw(&L->cur,"STACKBREVTOCN")||
      kw(&L->cur,"SREVLOWTOCN")||kw(&L->cur,"BREVTOCN")||kw(&L->cur,"SCELLBREVN")||
      kw(&L->cur,"SBITREVTOCN")||kw(&L->cur,"BREVNTOCN")){
    /* i + n → reverse low n bits of cells[i]; high kept; n 0..64; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n > 0 && n < 64){
      unsigned long m = (1ul << (unsigned)n) - 1ul;
      unsigned long la = (unsigned long)a & m;
      unsigned long ra = 0;
      for (long k = 0; k < n; k++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
      }
      r = (long)(((unsigned long)a & ~m) | (ra & m));
    } else if (n >= 64){
      unsigned long la = (unsigned long)a;
      unsigned long ra = 0;
      for (int k = 0; k < 64; k++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
      }
      r = (long)ra;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROLBTOCN")||kw(&L->cur,"SROLBTOCIMM")||kw(&L->cur,"STACKROLBTOCN")||
      kw(&L->cur,"SROTLBTOCN")||kw(&L->cur,"ROLBTOCN")||kw(&L->cur,"SCELLROLBN")||
      kw(&L->cur,"SLOWROLTOCN")||kw(&L->cur,"ROLBNTOCN")){
    /* i + n → rotate left by 1 within low n bits of cells[i]; high kept; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n >= 2 && n < 64){
      unsigned long m = (1ul << (unsigned)n) - 1ul;
      unsigned long la = (unsigned long)a & m;
      la = ((la << 1) | (la >> (unsigned)(n - 1))) & m;
      r = (long)(((unsigned long)a & ~m) | la);
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a;
      r = (long)((ua << 1) | (ua >> 63));
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SRORBTOCN")||kw(&L->cur,"SRORBTOCIMM")||kw(&L->cur,"STACKRORBTOCN")||
      kw(&L->cur,"SROTRBTOCN")||kw(&L->cur,"RORBTOCN")||kw(&L->cur,"SCELLRORBN")||
      kw(&L->cur,"SLOWRORTOCN")||kw(&L->cur,"RORBNTOCN")){
    /* i + n → rotate right by 1 within low n bits of cells[i]; high kept; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n >= 2 && n < 64){
      unsigned long m = (1ul << (unsigned)n) - 1ul;
      unsigned long la = (unsigned long)a & m;
      la = ((la >> 1) | (la << (unsigned)(n - 1))) & m;
      r = (long)(((unsigned long)a & ~m) | la);
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a;
      r = (long)((ua >> 1) | (ua << 63));
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 imm high-n reverse/rotate TOC: SBREVHNTOCN · SROLHNTOCN · SRORHNTOCN
   * (imm dual of SBREVHN/SROLHN/SRORHN into cell; high dual of SBREVTOCN plane) */
  if (kw(&L->cur,"SBREVHNTOCN")||kw(&L->cur,"SBREVHNTOCIMM")||kw(&L->cur,"STACKBREVHNTOCN")||
      kw(&L->cur,"SREVHIGHTOCN")||kw(&L->cur,"BREVHNTOCN")||kw(&L->cur,"SCELLBREVHN")||
      kw(&L->cur,"SBITREVHNTOCN")||kw(&L->cur,"BREVHNTOC")){
    /* i + n → reverse high n bits of cells[i]; low kept; n 0..64; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n > 0 && n < 64){
      unsigned long m = ~0ul << (unsigned)(64 - n);
      unsigned sh = (unsigned)(64 - n);
      unsigned long la = ((unsigned long)a & m) >> sh;
      unsigned long ra = 0;
      for (long k = 0; k < n; k++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
      }
      r = (long)(((unsigned long)a & ~m) | ((ra << sh) & m));
    } else if (n >= 64){
      unsigned long la = (unsigned long)a;
      unsigned long ra = 0;
      for (int k = 0; k < 64; k++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
      }
      r = (long)ra;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROLHNTOCN")||kw(&L->cur,"SROLHNTOCIMM")||kw(&L->cur,"STACKROLHNTOCN")||
      kw(&L->cur,"SROTLHNTOCN")||kw(&L->cur,"ROLHNTOCN")||kw(&L->cur,"SCELLROLHN")||
      kw(&L->cur,"SHIGHROLTOCN")||kw(&L->cur,"ROLHNTOC")){
    /* i + n → rotate left by 1 within high n bits of cells[i]; low kept; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n >= 2 && n < 64){
      unsigned long m = ~0ul << (unsigned)(64 - n);
      unsigned sh = (unsigned)(64 - n);
      unsigned long la = ((unsigned long)a & m) >> sh;
      unsigned long fm = (1ul << (unsigned)n) - 1ul;
      la = ((la << 1) | (la >> (unsigned)(n - 1))) & fm;
      r = (long)(((unsigned long)a & ~m) | ((la << sh) & m));
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a;
      r = (long)((ua << 1) | (ua >> 63));
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SRORHNTOCN")||kw(&L->cur,"SRORHNTOCIMM")||kw(&L->cur,"STACKRORHNTOCN")||
      kw(&L->cur,"SROTRHNTOCN")||kw(&L->cur,"RORHNTOCN")||kw(&L->cur,"SCELLRORHN")||
      kw(&L->cur,"SHIGHRORTOCN")||kw(&L->cur,"RORHNTOC")){
    /* i + n → rotate right by 1 within high n bits of cells[i]; low kept; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n >= 2 && n < 64){
      unsigned long m = ~0ul << (unsigned)(64 - n);
      unsigned sh = (unsigned)(64 - n);
      unsigned long la = ((unsigned long)a & m) >> sh;
      unsigned long fm = (1ul << (unsigned)n) - 1ul;
      la = ((la >> 1) | (la << (unsigned)(n - 1))) & fm;
      r = (long)(((unsigned long)a & ~m) | ((la << sh) & m));
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a;
      r = (long)((ua >> 1) | (ua << 63));
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 imm high-n metric TOC: SPOPHNTOCN · SANYHNTOCN · SALLHNTOCN
   * (imm dual of SPOPHN/SANYHN/SALLHN into cell; high dual of SPOPMNTOCN plane) */
  if (kw(&L->cur,"SPOPHNTOCN")||kw(&L->cur,"SPOPHNTOCIMM")||kw(&L->cur,"STACKPOPHNTOCN")||
      kw(&L->cur,"SONESHNTOCN")||kw(&L->cur,"POPHNTOCN")||kw(&L->cur,"SCELLPOPHN")||
      kw(&L->cur,"SHIGHPOPTOCN")||kw(&L->cur,"SPCNTHNTOCN")){
    /* i + n → cells[i] = popcount(cells[i] & high-n mask); n 0..64; leave result */
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
    unsigned long ua = (unsigned long)vm->cells[(int)i] & m;
    long r = 0;
    while (ua){ r += (long)(ua & 1ul); ua >>= 1; }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SANYHNTOCN")||kw(&L->cur,"SANYHNTOCIMM")||kw(&L->cur,"STACKANYHNTOCN")||
      kw(&L->cur,"SHIGHANYTOCN")||kw(&L->cur,"ANYHNTOCN")||kw(&L->cur,"SCELLANYHN")||
      kw(&L->cur,"STESTANYHNTOCN")){
    /* i + n → cells[i] = ((cells[i] & high-n mask) != 0) ? 1 : 0; leave result */
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
    long r = (((unsigned long)vm->cells[(int)i] & m) != 0) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SALLHNTOCN")||kw(&L->cur,"SALLHNTOCIMM")||kw(&L->cur,"STACKALLHNTOCN")||
      kw(&L->cur,"SHIGHALLTOCN")||kw(&L->cur,"ALLHNTOCN")||kw(&L->cur,"SCELLALLHN")||
      kw(&L->cur,"STESTALLHNTOCN")){
    /* i + n → cells[i] = ((cells[i] & high-n mask) == mask) ? 1 : 0; n=0 → 1 vacuous */
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
    long r = (((unsigned long)vm->cells[(int)i] & m) == m) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack dual low-n metric TOC: SPOPMNTOC · SANYMNTOC · SALLMNTOC
   * (stack dual of SPOPMNTOCN/SANYMNTOCN/SALLMNTOCN; i n from stack → metric into cell) */
  if (kw(&L->cur,"SPOPMNTOC")||kw(&L->cur,"STACKPOPMNTOC")||kw(&L->cur,"SONESMNTOC")||
      kw(&L->cur,"POPMNTOC")||kw(&L->cur,"SCELLPOPMNS")||kw(&L->cur,"SLOWPOPTOC")||
      kw(&L->cur,"SPCNTMNTOC")||kw(&L->cur,"SPOPMNAT")){
    /* i n → cells[i] = popcount(cells[i] & low-n mask); n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
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
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SANYMNTOC")||kw(&L->cur,"STACKANYMNTOC")||kw(&L->cur,"SLOWANYTOC")||
      kw(&L->cur,"ANYMNTOC")||kw(&L->cur,"SCELLANYMNS")||kw(&L->cur,"STESTANYTOC")||
      kw(&L->cur,"SANYMNAT")){
    /* i n → cells[i] = ((cells[i] & low-n mask) != 0) ? 1 : 0; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
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
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SALLMNTOC")||kw(&L->cur,"STACKALLMNTOC")||kw(&L->cur,"SLOWALLTOC")||
      kw(&L->cur,"ALLMNTOC")||kw(&L->cur,"SCELLALLMNS")||kw(&L->cur,"STESTALLTOC")||
      kw(&L->cur,"SALLMNAT")){
    /* i n → cells[i] = ((cells[i] & low-n mask) == mask) ? 1 : 0; n=0 → 1 vacuous */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
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
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack dual high-n metric TOC: SPOPHNTOC · SANYHNTOC · SALLHNTOC
   * (stack dual of SPOPHNTOCN/SANYHNTOCN/SALLHNTOCN; high dual of SPOPMNTOC plane) */
  if (kw(&L->cur,"SPOPHNTOC")||kw(&L->cur,"STACKPOPHNTOC")||kw(&L->cur,"SONESHNTOC")||
      kw(&L->cur,"POPHNTOC")||kw(&L->cur,"SCELLPOPHNS")||kw(&L->cur,"SHIGHPOPTOC")||
      kw(&L->cur,"SPCNTHNTOC")||kw(&L->cur,"SPOPHNAT")){
    /* i n → cells[i] = popcount(cells[i] & high-n mask); n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    unsigned long ua = (unsigned long)vm->cells[(int)i] & m;
    long r = 0;
    while (ua){ r += (long)(ua & 1ul); ua >>= 1; }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SANYHNTOC")||kw(&L->cur,"STACKANYHNTOC")||kw(&L->cur,"SHIGHANYTOC")||
      kw(&L->cur,"ANYHNTOC")||kw(&L->cur,"SCELLANYHNS")||kw(&L->cur,"STESTANYHNTOC")||
      kw(&L->cur,"SANYHNAT")){
    /* i n → cells[i] = ((cells[i] & high-n mask) != 0) ? 1 : 0; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long r = (((unsigned long)vm->cells[(int)i] & m) != 0) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SALLHNTOC")||kw(&L->cur,"STACKALLHNTOC")||kw(&L->cur,"SHIGHALLTOC")||
      kw(&L->cur,"ALLHNTOC")||kw(&L->cur,"SCELLALLHNS")||kw(&L->cur,"STESTALLHNTOC")||
      kw(&L->cur,"SALLHNAT")){
    /* i n → cells[i] = ((cells[i] & high-n mask) == mask) ? 1 : 0; n=0 → 1 vacuous */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    unsigned long m = 0;
    if (n == 0) m = 0;
    else if (n >= 64) m = ~0ul;
    else m = ~0ul << (unsigned)(64 - n);
    long r = (((unsigned long)vm->cells[(int)i] & m) == m) ? 1 : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-5 stack dual low-n mask TOC: SANDMNTOC · SORMNTOC · SXORMNTOC
   * (stack dual of SANDMNTOCN/SORMNTOCN/SXORMNTOCN; i n from stack → mask op into cell) */
  if (kw(&L->cur,"SANDMNTOC")||kw(&L->cur,"STACKANDMNTOC")||kw(&L->cur,"SKEEPLNTOC")||
      kw(&L->cur,"ANDMNTOC")||kw(&L->cur,"SCELLANDMNS")||kw(&L->cur,"SLOWANDTOC")||
      kw(&L->cur,"KEEPLNTOC")||kw(&L->cur,"SANDMNAT")){
    /* i n → cells[i] &= low-n mask; keep low n bits; n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
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
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORMNTOC")||kw(&L->cur,"STACKORMNTOC")||kw(&L->cur,"SSETLNTOC")||
      kw(&L->cur,"ORMNTOC")||kw(&L->cur,"SCELLORMNS")||kw(&L->cur,"SLOWORTOC")||
      kw(&L->cur,"SETLNTOC")||kw(&L->cur,"SORMNAT")){
    /* i n → cells[i] |= low-n mask; set low n bits; n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
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
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXORMNTOC")||kw(&L->cur,"STACKXORMNTOC")||kw(&L->cur,"SFLIPLNTOC")||
      kw(&L->cur,"XORMNTOC")||kw(&L->cur,"SCELLXORMNS")||kw(&L->cur,"SLOWXORTOC")||
      kw(&L->cur,"FLIPLNTOC")||kw(&L->cur,"SXORMNAT")){
    /* i n → cells[i] ^= low-n mask; toggle low n bits; n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
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
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack dual inverted low-n mask TOC: SNANDMNTOC · SNORMNTOC · SXNORMNTOC
   * (stack dual of SNANDMNTOCN/SNORMNTOCN/SXNORMNTOCN; i n from stack after SANDMNTOC plane) */
  if (kw(&L->cur,"SNANDMNTOC")||kw(&L->cur,"STACKNANDMNTOC")||kw(&L->cur,"SLOWNANDTOC")||
      kw(&L->cur,"NANDMNTOC")||kw(&L->cur,"SCELLNANDMNS")||kw(&L->cur,"SNANDMASKTOC")||
      kw(&L->cur,"MASKNANDTOC")||kw(&L->cur,"SNANDMNAT")){
    /* i n → cells[i] = ~(cells[i] & low-n mask); n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
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
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNORMNTOC")||kw(&L->cur,"STACKNORMNTOC")||kw(&L->cur,"SLOWNORTOC")||
      kw(&L->cur,"NORMNTOC")||kw(&L->cur,"SCELLNORMNS")||kw(&L->cur,"SNORMASKTOC")||
      kw(&L->cur,"MASKNORTOC")||kw(&L->cur,"SNORMNAT")){
    /* i n → cells[i] = ~(cells[i] | low-n mask); n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
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
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNORMNTOC")||kw(&L->cur,"STACKXNORMNTOC")||kw(&L->cur,"SLOWXNORTOC")||
      kw(&L->cur,"XNORMNTOC")||kw(&L->cur,"SCELLXNORMNS")||kw(&L->cur,"SXNORMASKTOC")||
      kw(&L->cur,"MASKXNORTOC")||kw(&L->cur,"SEQUIVMNTOC")||kw(&L->cur,"EQUIVMNTOC")||
      kw(&L->cur,"SXNORMNAT")){
    /* i n → cells[i] = ~(cells[i] ^ low-n mask); n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
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
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 stack dual high-n mask TOC: SANDHNTOC · SORHNTOC · SXORHNTOC
   * (stack dual of SANDHNTOCN/SORHNTOCN/SXORHNTOCN; high-n dual of SANDMNTOC plane) */
  if (kw(&L->cur,"SANDHNTOC")||kw(&L->cur,"STACKANDHNTOC")||kw(&L->cur,"SKEEPHNTOC")||
      kw(&L->cur,"ANDHNTOC")||kw(&L->cur,"SCELLANDHNS")||kw(&L->cur,"SHIGHANDTOC")||
      kw(&L->cur,"KEEPHNTOC")||kw(&L->cur,"SANDHNAT")){
    /* i n → cells[i] &= high-n mask; keep high n bits; n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
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
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORHNTOC")||kw(&L->cur,"STACKORHNTOC")||kw(&L->cur,"SSETHNTOC")||
      kw(&L->cur,"ORHNTOC")||kw(&L->cur,"SCELLORHNS")||kw(&L->cur,"SHIGHORTOC")||
      kw(&L->cur,"SETHNTOC")||kw(&L->cur,"SORHNAT")){
    /* i n → cells[i] |= high-n mask; set high n bits; n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
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
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXORHNTOC")||kw(&L->cur,"STACKXORHNTOC")||kw(&L->cur,"SFLIPHNTOC")||
      kw(&L->cur,"XORHNTOC")||kw(&L->cur,"SCELLXORHNS")||kw(&L->cur,"SHIGHXORTOC")||
      kw(&L->cur,"FLIPHNTOC")||kw(&L->cur,"SXORHNAT")){
    /* i n → cells[i] ^= high-n mask; toggle high n bits; n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
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
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack dual inverted high-n mask TOC: SNANDHNTOC · SNORHNTOC · SXNORHNTOC
   * (stack dual of SNANDHNTOCN/SNORHNTOCN/SXNORHNTOCN; high dual of SNANDMNTOC after SANDHNTOC) */
  if (kw(&L->cur,"SNANDHNTOC")||kw(&L->cur,"STACKNANDHNTOC")||kw(&L->cur,"SHIGHNANDTOC")||
      kw(&L->cur,"NANDHNTOC")||kw(&L->cur,"SCELLNANDHNS")||kw(&L->cur,"SNANDMASKHTOC")||
      kw(&L->cur,"MASKNANDHTOC")||kw(&L->cur,"SNANDHNAT")){
    /* i n → cells[i] = ~(cells[i] & high-n mask); n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
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
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNORHNTOC")||kw(&L->cur,"STACKNORHNTOC")||kw(&L->cur,"SHIGHNORTOC")||
      kw(&L->cur,"NORHNTOC")||kw(&L->cur,"SCELLNORHNS")||kw(&L->cur,"SNORMASKHTOC")||
      kw(&L->cur,"MASKNORHTOC")||kw(&L->cur,"SNORHNAT")){
    /* i n → cells[i] = ~(cells[i] | high-n mask); n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
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
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNORHNTOC")||kw(&L->cur,"STACKXNORHNTOC")||kw(&L->cur,"SHIGHXNORTOC")||
      kw(&L->cur,"XNORHNTOC")||kw(&L->cur,"SCELLXNORHNS")||kw(&L->cur,"SXNORMASKHTOC")||
      kw(&L->cur,"MASKXNORHTOC")||kw(&L->cur,"SEQUIVHNTOC")||kw(&L->cur,"EQUIVHNTOC")||
      kw(&L->cur,"SXNORHNAT")){
    /* i n → cells[i] = ~(cells[i] ^ high-n mask); n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
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
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack dual clear + high-mask TOC: SCLRMNTOC · SCLRHNTOC · SHMASKTOC
   * (stack dual of SCLRMNTOCN/SCLRHNTOCN/SHMASKTOCN after inverted high-n plane) */
  if (kw(&L->cur,"SCLRMNTOC")||kw(&L->cur,"STACKCLRMNTOC")||kw(&L->cur,"SCLEARLNTOC")||
      kw(&L->cur,"CLRMNTOC")||kw(&L->cur,"SCELLCLRMNS")||kw(&L->cur,"SLOWCLRTOC")||
      kw(&L->cur,"CLEARLNTOC")||kw(&L->cur,"SCLRMNAT")){
    /* i n → cells[i] &= ~low-n mask; clear low n bits; n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
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
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLRHNTOC")||kw(&L->cur,"STACKCLRHNTOC")||kw(&L->cur,"SCLEARHNTOC")||
      kw(&L->cur,"CLRHNTOC")||kw(&L->cur,"SCELLCLRHNS")||kw(&L->cur,"SHIGHCLRTOC")||
      kw(&L->cur,"CLEARHNTOC")||kw(&L->cur,"SCLRHNAT")){
    /* i n → cells[i] &= ~high-n mask; clear high n bits; n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
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
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHMASKTOC")||kw(&L->cur,"STACKHMASKTOC")||kw(&L->cur,"SHIMASKTOC")||
      kw(&L->cur,"HMASKTOC")||kw(&L->cur,"SCELLHMASKS")||kw(&L->cur,"SHIGHMASKTOC")||
      kw(&L->cur,"SETHMASKTOC")||kw(&L->cur,"SHMASKAT")){
    /* i n → cells[i] = high-n-bit mask; n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
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
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack dual low-n reverse/rotate TOC: SBREVTOC · SROLBTOC · SRORBTOC
   * (stack dual of SBREVTOCN/SROLBTOCN/SRORBTOCN after SCLRMNTOC plane) */
  if (kw(&L->cur,"SBREVTOC")||kw(&L->cur,"STACKBREVTOC")||kw(&L->cur,"SREVLOWTOC")||
      kw(&L->cur,"BREVTOC")||kw(&L->cur,"SCELLBREVNS")||kw(&L->cur,"SBITREVTOC")||
      kw(&L->cur,"BREVNTOC")||kw(&L->cur,"SBREVAT")){
    /* i n → reverse low n bits of cells[i]; high kept; n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n > 0 && n < 64){
      unsigned long m = (1ul << (unsigned)n) - 1ul;
      unsigned long la = (unsigned long)a & m;
      unsigned long ra = 0;
      for (long k = 0; k < n; k++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
      }
      r = (long)(((unsigned long)a & ~m) | (ra & m));
    } else if (n >= 64){
      unsigned long la = (unsigned long)a;
      unsigned long ra = 0;
      for (int k = 0; k < 64; k++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
      }
      r = (long)ra;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROLBTOC")||kw(&L->cur,"STACKROLBTOC")||kw(&L->cur,"SROTLBTOC")||
      kw(&L->cur,"ROLBTOC")||kw(&L->cur,"SCELLROLBNS")||kw(&L->cur,"SLOWROLTOC")||
      kw(&L->cur,"ROLBNTOC")||kw(&L->cur,"SROLBAT")){
    /* i n → rotate left by 1 within low n bits of cells[i]; high kept; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n >= 2 && n < 64){
      unsigned long m = (1ul << (unsigned)n) - 1ul;
      unsigned long la = (unsigned long)a & m;
      la = ((la << 1) | (la >> (unsigned)(n - 1))) & m;
      r = (long)(((unsigned long)a & ~m) | la);
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a;
      r = (long)((ua << 1) | (ua >> 63));
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SRORBTOC")||kw(&L->cur,"STACKRORBTOC")||kw(&L->cur,"SROTRBTOC")||
      kw(&L->cur,"RORBTOC")||kw(&L->cur,"SCELLRORBNS")||kw(&L->cur,"SLOWRORTOC")||
      kw(&L->cur,"RORBNTOC")||kw(&L->cur,"SRORBAT")){
    /* i n → rotate right by 1 within low n bits of cells[i]; high kept; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n >= 2 && n < 64){
      unsigned long m = (1ul << (unsigned)n) - 1ul;
      unsigned long la = (unsigned long)a & m;
      la = ((la >> 1) | (la << (unsigned)(n - 1))) & m;
      r = (long)(((unsigned long)a & ~m) | la);
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a;
      r = (long)((ua >> 1) | (ua << 63));
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 stack dual high-n reverse/rotate TOC: SBREVHNTOC · SROLHNTOC · SRORHNTOC
   * (stack dual of SBREVHNTOCN/SROLHNTOCN/SRORHNTOCN; high dual of SBREVTOC plane) */
  if (kw(&L->cur,"SBREVHNTOC")||kw(&L->cur,"STACKBREVHNTOC")||kw(&L->cur,"SREVHIGHTOC")||
      kw(&L->cur,"BREVHNAT")||kw(&L->cur,"SCELLBREVHNS")||kw(&L->cur,"SBITREVHNTOC")||
      kw(&L->cur,"BREVHNTOCS")||kw(&L->cur,"SHIGHBREVTOC")){
    /* i n → reverse high n bits of cells[i]; low kept; n 0..64; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n > 0 && n < 64){
      unsigned long m = ~0ul << (unsigned)(64 - n);
      unsigned sh = (unsigned)(64 - n);
      unsigned long la = ((unsigned long)a & m) >> sh;
      unsigned long ra = 0;
      for (long k = 0; k < n; k++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
      }
      r = (long)(((unsigned long)a & ~m) | ((ra << sh) & m));
    } else if (n >= 64){
      unsigned long la = (unsigned long)a;
      unsigned long ra = 0;
      for (int k = 0; k < 64; k++){
        ra = (ra << 1) | (la & 1ul); la >>= 1;
      }
      r = (long)ra;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROLHNTOC")||kw(&L->cur,"STACKROLHNTOC")||kw(&L->cur,"SROTLHNTOC")||
      kw(&L->cur,"ROLHNAT")||kw(&L->cur,"SCELLROLHNS")||kw(&L->cur,"SHIGHROLTOC")||
      kw(&L->cur,"ROLHNTOCS")||kw(&L->cur,"SROTLHNTOCS")){
    /* i n → rotate left by 1 within high n bits of cells[i]; low kept; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n >= 2 && n < 64){
      unsigned long m = ~0ul << (unsigned)(64 - n);
      unsigned sh = (unsigned)(64 - n);
      unsigned long la = ((unsigned long)a & m) >> sh;
      unsigned long fm = (1ul << (unsigned)n) - 1ul;
      la = ((la << 1) | (la >> (unsigned)(n - 1))) & fm;
      r = (long)(((unsigned long)a & ~m) | ((la << sh) & m));
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a;
      r = (long)((ua << 1) | (ua >> 63));
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SRORHNTOC")||kw(&L->cur,"STACKRORHNTOC")||kw(&L->cur,"SROTRHNTOC")||
      kw(&L->cur,"RORHNAT")||kw(&L->cur,"SCELLRORHNS")||kw(&L->cur,"SHIGHRORTOC")||
      kw(&L->cur,"RORHNTOCS")||kw(&L->cur,"SROTRHNTOCS")){
    /* i n → rotate right by 1 within high n bits of cells[i]; low kept; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 64) n = 64;
    long a = vm->cells[(int)i];
    long r = a;
    if (n >= 2 && n < 64){
      unsigned long m = ~0ul << (unsigned)(64 - n);
      unsigned sh = (unsigned)(64 - n);
      unsigned long la = ((unsigned long)a & m) >> sh;
      unsigned long fm = (1ul << (unsigned)n) - 1ul;
      la = ((la >> 1) | (la << (unsigned)(n - 1))) & fm;
      r = (long)(((unsigned long)a & ~m) | ((la << sh) & m));
    } else if (n >= 64){
      unsigned long ua = (unsigned long)a;
      r = (long)((ua >> 1) | (ua << 63));
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 foundation imm inverted bitwise TOC: SNANDTOCN · SNORTOCN · SXNORTOCN
   * (imm dual of SNANDTOC/SNORTOC/SXNORTOC after SANDTOCN plane) */
  if (kw(&L->cur,"SNANDTOCN")||kw(&L->cur,"SNANDTOCIMM")||kw(&L->cur,"STACKNANDTOCN")||
      kw(&L->cur,"SNANDATN")||kw(&L->cur,"NANDTOCN")||kw(&L->cur,"SCELLNANDN")||
      kw(&L->cur,"BNANDTOCN")){
    /* i + n → cells[i] = ~(cells[i] & n), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ~(vm->cells[(int)i] & n);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNORTOCN")||kw(&L->cur,"SNORTOCIMM")||kw(&L->cur,"STACKNORTOCN")||
      kw(&L->cur,"SNORATN")||kw(&L->cur,"NORTOCN")||kw(&L->cur,"SCELLNORN")||
      kw(&L->cur,"BNORTOCN")){
    /* i + n → cells[i] = ~(cells[i] | n), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ~(vm->cells[(int)i] | n);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNORTOCN")||kw(&L->cur,"SXNORTOCIMM")||kw(&L->cur,"STACKXNORTOCN")||
      kw(&L->cur,"SXNORATN")||kw(&L->cur,"XNORTOCN")||kw(&L->cur,"SCELLXNORN")||
      kw(&L->cur,"BXNORTOCN")||kw(&L->cur,"SEQUIVTOCN")){
    /* i + n → cells[i] = ~(cells[i] ^ n), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ~(vm->cells[(int)i] ^ n);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 imm ANDN-plane TOC: SANDNTOCN · SORNTOCN · SXORNTOCN
   * (imm dual of SANDNTOC/SORNTOC/SXORNTOC after SNANDTOCN; BIC/ORNOT/XORNOT into cell) */
  if (kw(&L->cur,"SANDNTOCN")||kw(&L->cur,"SANDNTOCIMM")||kw(&L->cur,"STACKANDNTOCN")||
      kw(&L->cur,"SANDNATN")||kw(&L->cur,"ANDNTOCN")||kw(&L->cur,"SBICTOCN")||
      kw(&L->cur,"BICTOCN")||kw(&L->cur,"SCELLANDNN")){
    /* i + n → cells[i] &= ~n, leave result (clear bits set in n) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] & ~n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORNTOCN")||kw(&L->cur,"SORNTOCIMM")||kw(&L->cur,"STACKORNTOCN")||
      kw(&L->cur,"SORNATN")||kw(&L->cur,"ORNTOCN")||kw(&L->cur,"SORNOTTOCN")||
      kw(&L->cur,"SCELLORNN")){
    /* i + n → cells[i] |= ~n, leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] | ~n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXORNTOCN")||kw(&L->cur,"SXORNTOCIMM")||kw(&L->cur,"STACKXORNTOCN")||
      kw(&L->cur,"SXORNATN")||kw(&L->cur,"XORNTOCN")||kw(&L->cur,"SXORNOTTOCN")||
      kw(&L->cur,"SCELLXORNN")){
    /* i + n → cells[i] ^= ~n, leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] ^ ~n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 imm accumulate bound TOC: SDIVTOCN · SMODTOCN · SMINTOCN · SMAXTOCN
   * (complete SADDTOCN/SMULTOCN with / % min max; imm dual of SDIVTOC/SMODTOC/SMINTOC/SMAXTOC) */
  if (kw(&L->cur,"SDIVTOCN")||kw(&L->cur,"SDIVTOCIMM")||kw(&L->cur,"STACKDIVTOCN")||
      kw(&L->cur,"SDIVATN")||kw(&L->cur,"DIVTOCN")||kw(&L->cur,"SCELLDIVN")||
      kw(&L->cur,"QUOTTOCN")){
    /* i + n → cells[i]/=n (n==0 → 0 soft), leave quotient */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = n ? (vm->cells[(int)i] / n) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMODTOCN")||kw(&L->cur,"SMODTOCIMM")||kw(&L->cur,"STACKMODTOCN")||
      kw(&L->cur,"SMODATN")||kw(&L->cur,"MODTOCN")||kw(&L->cur,"SCELLMODN")||
      kw(&L->cur,"REMTOCN")){
    /* i + n → cells[i]%=n (n==0 → 0 soft), leave remainder */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = n ? (vm->cells[(int)i] % n) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMINTOCN")||kw(&L->cur,"SMINTOCIMM")||kw(&L->cur,"STACKMINTOCN")||
      kw(&L->cur,"SMINATN")||kw(&L->cur,"MINTOCN")||kw(&L->cur,"SCELLMINN")){
    /* i + n → cells[i]=min(cells[i],n), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long c = vm->cells[(int)i];
    long r = (c < n) ? c : n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMAXTOCN")||kw(&L->cur,"SMAXTOCIMM")||kw(&L->cur,"STACKMAXTOCN")||
      kw(&L->cur,"SMAXATN")||kw(&L->cur,"MAXTOCN")||kw(&L->cur,"SCELLMAXN")){
    /* i + n → cells[i]=max(cells[i],n), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long c = vm->cells[(int)i];
    long r = (c > n) ? c : n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 foundation forward unsigned imm TOC: SUDIVTOCN · SUMODTOCN · SUMINTOCN · SUMAXTOCN
   * (unsigned peer of SDIVTOCN/SMODTOCN/SMINTOCN/SMAXTOCN; imm dual of SUDIVN/SUMINN after reverse unsigned TOC) */
  if (kw(&L->cur,"SUDIVTOCN")||kw(&L->cur,"SUDIVTOCIMM")||kw(&L->cur,"STACKUDIVTOCN")||
      kw(&L->cur,"SUDIVATN")||kw(&L->cur,"UDIVTOCN")||kw(&L->cur,"SCELLUDIVN")||
      kw(&L->cur,"UQUOTTOCN")){
    /* i + n → cells[i] = (u)cells[i] / (u)n; n==0 → 0 soft; leave quotient */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = 0;
    if (n != 0) r = (long)((unsigned long)vm->cells[(int)i] / (unsigned long)n);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUMODTOCN")||kw(&L->cur,"SUMODTOCIMM")||kw(&L->cur,"STACKUMODTOCN")||
      kw(&L->cur,"SUMODATN")||kw(&L->cur,"UMODTOCN")||kw(&L->cur,"SCELLUMODN")||
      kw(&L->cur,"UREMTOCN")||kw(&L->cur,"SUREMTOCN")){
    /* i + n → cells[i] = (u)cells[i] % (u)n; n==0 → 0 soft; leave remainder */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = 0;
    if (n != 0) r = (long)((unsigned long)vm->cells[(int)i] % (unsigned long)n);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUMINTOCN")||kw(&L->cur,"SUMINTOCIMM")||kw(&L->cur,"STACKUMINTOCN")||
      kw(&L->cur,"SUMINATN")||kw(&L->cur,"UMINTOCN")||kw(&L->cur,"SCELLUMINN")||
      kw(&L->cur,"UMINTOCN")){
    /* i + n → cells[i]=umin(cells[i],n), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long c = (unsigned long)vm->cells[(int)i];
    unsigned long un = (unsigned long)n;
    long r = (long)((c < un) ? c : un);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUMAXTOCN")||kw(&L->cur,"SUMAXTOCIMM")||kw(&L->cur,"STACKUMAXTOCN")||
      kw(&L->cur,"SUMAXATN")||kw(&L->cur,"UMAXTOCN")||kw(&L->cur,"SCELLUMAXN")||
      kw(&L->cur,"UMAXTOCN")){
    /* i + n → cells[i]=umax(cells[i],n), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long c = (unsigned long)vm->cells[(int)i];
    unsigned long un = (unsigned long)n;
    long r = (long)((c > un) ? c : un);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 foundation stack↔cell unsigned TOC: SUDIVTOC · SUMODTOC · SUMINTOC · SUMAXTOC
   * (stack dual of SUDIVTOCN/SUMODTOCN/SUMINTOCN/SUMAXTOCN; unsigned peer of SDIVTOC/SMODTOC/SMINTOC/SMAXTOC) */
  if (kw(&L->cur,"SUDIVTOC")||kw(&L->cur,"SCELLUDIV")||kw(&L->cur,"STACKUDIVTOC")||
      kw(&L->cur,"SUDIVTOCELL")||kw(&L->cur,"UDIVTOC")||kw(&L->cur,"SUQUOTTOC")){
    /* i v → cells[i] = (u)cells[i] / (u)v; v==0 → 0 soft; leave quotient */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = 0;
    if (v != 0) r = (long)((unsigned long)vm->cells[(int)i] / (unsigned long)v);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUMODTOC")||kw(&L->cur,"SCELLUMOD")||kw(&L->cur,"STACKUMODTOC")||
      kw(&L->cur,"SUMODTOCELL")||kw(&L->cur,"UMODTOC")||kw(&L->cur,"SUREMTOC")||
      kw(&L->cur,"UREMTOC")){
    /* i v → cells[i] = (u)cells[i] % (u)v; v==0 → 0 soft; leave remainder */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = 0;
    if (v != 0) r = (long)((unsigned long)vm->cells[(int)i] % (unsigned long)v);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUMINTOC")||kw(&L->cur,"SCELLUMIN")||kw(&L->cur,"STACKUMINTOC")||
      kw(&L->cur,"SUMINTOCELL")||kw(&L->cur,"UMINTOC")||kw(&L->cur,"SUMINTO")){
    /* i v → cells[i]=umin(cells[i],v), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long c = (unsigned long)vm->cells[(int)i];
    unsigned long uv = (unsigned long)v;
    long r = (long)((c < uv) ? c : uv);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUMAXTOC")||kw(&L->cur,"SCELLUMAX")||kw(&L->cur,"STACKUMAXTOC")||
      kw(&L->cur,"SUMAXTOCELL")||kw(&L->cur,"UMAXTOC")||kw(&L->cur,"SUMAXTO")){
    /* i v → cells[i]=umax(cells[i],v), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long c = (unsigned long)vm->cells[(int)i];
    unsigned long uv = (unsigned long)v;
    long r = (long)((c > uv) ? c : uv);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 forward imm modular TOC: SADDMODTOCN · SSUBMODTOCN · SMULMODTOCN
   * (i + k m → cells[i] = (cells[i] op k) mod m; imm dual of SADDMODN after reverse modular TOC) */
  if (kw(&L->cur,"SADDMODTOCN")||kw(&L->cur,"SADDMODTOCIMM")||kw(&L->cur,"STACKADDMODTOCN")||
      kw(&L->cur,"SADDMODATN")||kw(&L->cur,"ADDMODTOCN")||kw(&L->cur,"SCELLADDMODN")){
    /* i + k m → cells[i] = (cells[i]+k) mod m; m<=0 → 0; leave result */
    lex_next(L);
    long k = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (m > 0){
      long kk = k % m; if (kk < 0) kk += m;
      long aa = a % m; if (aa < 0) aa += m;
      r = (aa + kk) % m;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSUBMODTOCN")||kw(&L->cur,"SSUBMODTOCIMM")||kw(&L->cur,"STACKSUBMODTOCN")||
      kw(&L->cur,"SSUBMODATN")||kw(&L->cur,"SUBMODTOCN")||kw(&L->cur,"SCELLSUBMODN")){
    /* i + k m → cells[i] = (cells[i]-k) mod m; m<=0 → 0; leave result */
    lex_next(L);
    long k = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (m > 0){
      long kk = k % m; if (kk < 0) kk += m;
      long aa = a % m; if (aa < 0) aa += m;
      r = (aa - kk + m) % m;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMULMODTOCN")||kw(&L->cur,"SMULMODTOCIMM")||kw(&L->cur,"STACKMULMODTOCN")||
      kw(&L->cur,"SMULMODATN")||kw(&L->cur,"MULMODTOCN")||kw(&L->cur,"SCELLMULMODN")){
    /* i + k m → cells[i] = (cells[i]*k) mod m; m<=0 → 0; leave result */
    lex_next(L);
    long k = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (m > 0){
      long kk = k % m; if (kk < 0) kk += m;
      long aa = a % m; if (aa < 0) aa += m;
      long acc = 0, xx = aa, kk2 = kk;
      while (kk2 > 0){
        if (kk2 & 1) acc = (acc + xx) % m;
        xx = (xx + xx) % m;
        kk2 >>= 1;
      }
      r = acc;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack↔cell modular accumulate TOC: SADDMODTOC · SSUBMODTOC · SMULMODTOC
   * (stack dual of SADDMODTOCN/SSUBMODTOCN/SMULMODTOCN; modular peer of SADDTOC after imm plane) */
  if (kw(&L->cur,"SADDMODTOC")||kw(&L->cur,"SCELLADDMOD")||kw(&L->cur,"STACKADDMODTOC")||
      kw(&L->cur,"SADDMODTOCELL")||kw(&L->cur,"ADDMODTOC")||kw(&L->cur,"SACCUMMODTOC")){
    /* i k m → cells[i] = (cells[i]+k) mod m; m<=0 → 0; leave result */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long k = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (m > 0){
      long kk = k % m; if (kk < 0) kk += m;
      long aa = a % m; if (aa < 0) aa += m;
      r = (aa + kk) % m;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSUBMODTOC")||kw(&L->cur,"SCELLSUBMOD")||kw(&L->cur,"STACKSUBMODTOC")||
      kw(&L->cur,"SSUBMODTOCELL")||kw(&L->cur,"SUBMODTOC")){
    /* i k m → cells[i] = (cells[i]-k) mod m; m<=0 → 0; leave result */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long k = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (m > 0){
      long kk = k % m; if (kk < 0) kk += m;
      long aa = a % m; if (aa < 0) aa += m;
      r = (aa - kk + m) % m;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMULMODTOC")||kw(&L->cur,"SCELLMULMOD")||kw(&L->cur,"STACKMULMODTOC")||
      kw(&L->cur,"SMULMODTOCELL")||kw(&L->cur,"MULMODTOC")){
    /* i k m → cells[i] = (cells[i]*k) mod m; m<=0 → 0; leave result */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long k = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (m > 0){
      long kk = k % m; if (kk < 0) kk += m;
      long aa = a % m; if (aa < 0) aa += m;
      long acc = 0, xx = aa, kk2 = kk;
      while (kk2 > 0){
        if (kk2 & 1) acc = (acc + xx) % m;
        xx = (xx + xx) % m;
        kk2 >>= 1;
      }
      r = acc;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 forward imm modular inv/pow/div TOC: SPOWMODTOCN · SMODDIVTOCN · SMODINVTOCN
   * (i + args → cells[i] = inv/pow/div-mod; imm dual of SPOWMODN after SADDMODTOCN plane) */
  if (kw(&L->cur,"SPOWMODTOCN")||kw(&L->cur,"SPOWMODTOCIMM")||kw(&L->cur,"STACKPOWMODTOCN")||
      kw(&L->cur,"SPOWMODATN")||kw(&L->cur,"POWMODTOCN")||kw(&L->cur,"SCELLPOWMODN")||
      kw(&L->cur,"SEXPMMODTOCN")){
    /* i + exp m → cells[i] = cells[i]^exp mod m; m<=0 or exp<0 → 0; leave result */
    lex_next(L);
    long exp = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (m > 0 && exp >= 0){
      long base = a % m; if (base < 0) base += m;
      r = 1 % m;
      long e = exp;
      while (e > 0){
        if (e & 1){
          long y = r, x = base, acc = 0;
          while (y > 0){
            if (y & 1) acc = (acc + x) % m;
            x = (x + x) % m;
            y >>= 1;
          }
          r = acc;
        }
        {
          long x = base, acc = 0, y = base;
          while (y > 0){
            if (y & 1) acc = (acc + x) % m;
            x = (x + x) % m;
            y >>= 1;
          }
          base = acc;
        }
        e >>= 1;
      }
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMODDIVTOCN")||kw(&L->cur,"SMODDIVTOCIMM")||kw(&L->cur,"STACKMODDIVTOCN")||
      kw(&L->cur,"SMODDIVATN")||kw(&L->cur,"MODDIVTOCN")||kw(&L->cur,"SCELLMODDIVN")||
      kw(&L->cur,"SDIVMODTOCN")||kw(&L->cur,"DIVMODMTOCN")){
    /* i + b m → cells[i] = cells[i] * b^{-1} mod m; 0 if none / m<=0; leave result */
    lex_next(L);
    long b = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (m > 0){
      long bb = b % m; if (bb < 0) bb += m;
      if (bb != 0){
        long t = 0, nt = 1;
        long rr = m, nr = bb;
        while (nr != 0){
          long q = rr / nr;
          long tmp = nt; nt = t - q * nt; t = tmp;
          tmp = nr; nr = rr - q * nr; rr = tmp;
        }
        if (rr == 1){
          if (t < 0) t += m;
          long x = a % m; if (x < 0) x += m;
          long y = t, acc = 0;
          while (y > 0){
            if (y & 1) acc = (acc + x) % m;
            x = (x + x) % m;
            y >>= 1;
          }
          r = acc;
        }
      }
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMODINVTOCN")||kw(&L->cur,"SMODINVTOCIMM")||kw(&L->cur,"STACKMODINVTOCN")||
      kw(&L->cur,"SMODINVATN")||kw(&L->cur,"MODINVTOCN")||kw(&L->cur,"SCELLMODINVN")||
      kw(&L->cur,"SINVMODTOCN")||kw(&L->cur,"INVMODTOCN")){
    /* i + m → cells[i] = cells[i]^{-1} mod m; 0 if none / m<=1; leave result */
    lex_next(L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (m > 1){
      long aa = a % m; if (aa < 0) aa += m;
      if (aa != 0){
        long t = 0, nt = 1;
        long rr = m, nr = aa;
        while (nr != 0){
          long q = rr / nr;
          long tmp = nt; nt = t - q * nt; t = tmp;
          tmp = nr; nr = rr - q * nr; rr = tmp;
        }
        if (rr == 1){ if (t < 0) t += m; r = t; }
      }
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 stack↔cell modular inv/pow/div TOC: SPOWMODTOC · SMODDIVTOC · SMODINVTOC
   * (stack dual of SPOWMODTOCN/SMODDIVTOCN/SMODINVTOCN; complete modular TOC after SADDMODTOCN) */
  if (kw(&L->cur,"SPOWMODTOC")||kw(&L->cur,"SCELLPOWMOD")||kw(&L->cur,"STACKPOWMODTOC")||
      kw(&L->cur,"SPOWMODTOCELL")||kw(&L->cur,"POWMODTOC")||kw(&L->cur,"SEXPMMODTOC")){
    /* i exp m → cells[i] = cells[i]^exp mod m; m<=0 or exp<0 → 0; leave result */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long exp = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (m > 0 && exp >= 0){
      long base = a % m; if (base < 0) base += m;
      r = 1 % m;
      long e = exp;
      while (e > 0){
        if (e & 1){
          long y = r, x = base, acc = 0;
          while (y > 0){
            if (y & 1) acc = (acc + x) % m;
            x = (x + x) % m;
            y >>= 1;
          }
          r = acc;
        }
        {
          long x = base, acc = 0, y = base;
          while (y > 0){
            if (y & 1) acc = (acc + x) % m;
            x = (x + x) % m;
            y >>= 1;
          }
          base = acc;
        }
        e >>= 1;
      }
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMODDIVTOC")||kw(&L->cur,"SCELLMODDIV")||kw(&L->cur,"STACKMODDIVTOC")||
      kw(&L->cur,"SMODDIVTOCELL")||kw(&L->cur,"MODDIVTOC")||kw(&L->cur,"SDIVMODMTOC")||
      kw(&L->cur,"DIVMODMTOC")){
    /* i b m → cells[i] = cells[i] * b^{-1} mod m; 0 if none / m<=0; leave result */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (m > 0){
      long bb = b % m; if (bb < 0) bb += m;
      if (bb != 0){
        long t = 0, nt = 1;
        long rr = m, nr = bb;
        while (nr != 0){
          long q = rr / nr;
          long tmp = nt; nt = t - q * nt; t = tmp;
          tmp = nr; nr = rr - q * nr; rr = tmp;
        }
        if (rr == 1){
          if (t < 0) t += m;
          long x = a % m; if (x < 0) x += m;
          long y = t, acc = 0;
          while (y > 0){
            if (y & 1) acc = (acc + x) % m;
            x = (x + x) % m;
            y >>= 1;
          }
          r = acc;
        }
      }
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMODINVTOC")||kw(&L->cur,"SCELLMODINV")||kw(&L->cur,"STACKMODINVTOC")||
      kw(&L->cur,"SMODINVTOCELL")||kw(&L->cur,"MODINVTOC")||kw(&L->cur,"SINVMODTOC")||
      kw(&L->cur,"INVMODTOC")){
    /* i m → cells[i] = cells[i]^{-1} mod m; 0 if none / m<=1; leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (m > 1){
      long aa = a % m; if (aa < 0) aa += m;
      if (aa != 0){
        long t = 0, nt = 1;
        long rr = m, nr = aa;
        while (nr != 0){
          long q = rr / nr;
          long tmp = nt; nt = t - q * nt; t = tmp;
          tmp = nr; nr = rr - q * nr; rr = tmp;
        }
        if (rr == 1){ if (t < 0) t += m; r = t; }
      }
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack↔cell reverse accumulate: SSUBFROMTOC · SDIVFROMTOC · SMODFROMTOC */
  if (kw(&L->cur,"SSUBFROMTOC")||kw(&L->cur,"SCELLSUBFROM")||kw(&L->cur,"SRSUBCELL")||
      kw(&L->cur,"STACKSUBFROMCELL")||kw(&L->cur,"SSUBFROMCELL")){
    /* i v → cells[i] = v - cells[i], leave result (reverse of SSUBTOC) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = v - vm->cells[(int)i];
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SDIVFROMTOC")||kw(&L->cur,"SCELLDIVFROM")||kw(&L->cur,"SRDIVCELL")||
      kw(&L->cur,"STACKDIVFROMCELL")||kw(&L->cur,"SDIVFROMCELL")){
    /* i v → cells[i] = v / cells[i] (cells[i]==0 → 0), leave quotient */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long c = vm->cells[(int)i];
    long r = c ? (v / c) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMODFROMTOC")||kw(&L->cur,"SCELLMODFROM")||kw(&L->cur,"SRMODCELL")||
      kw(&L->cur,"STACKMODFROMCELL")||kw(&L->cur,"SMODFROMCELL")||kw(&L->cur,"SREMFROMCELL")){
    /* i v → cells[i] = v % cells[i] (cells[i]==0 → 0), leave remainder */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long c = vm->cells[(int)i];
    long r = c ? (v % c) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 reverse imm accumulate TOC: SSUBFROMTOCN · SDIVFROMTOCN · SMODFROMTOCN
   * (imm dual of SSUBFROMTOC/SDIVFROMTOC/SMODFROMTOC; peer of SSUBFROMN into cell) */
  if (kw(&L->cur,"SSUBFROMTOCN")||kw(&L->cur,"SSUBFROMTOCIMM")||kw(&L->cur,"STACKSUBFROMTOCN")||
      kw(&L->cur,"SSUBFROMATN")||kw(&L->cur,"SRSUBTOCN")||kw(&L->cur,"RSUBTOCN")||
      kw(&L->cur,"SCELLSUBFROMN")){
    /* i + n → cells[i] = n - cells[i], leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = n - vm->cells[(int)i];
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SDIVFROMTOCN")||kw(&L->cur,"SDIVFROMTOCIMM")||kw(&L->cur,"STACKDIVFROMTOCN")||
      kw(&L->cur,"SDIVFROMATN")||kw(&L->cur,"SRDIVTOCN")||kw(&L->cur,"RDIVTOCN")||
      kw(&L->cur,"SCELLDIVFROMN")){
    /* i + n → cells[i] = n / cells[i] (cells[i]==0 → 0 soft), leave quotient */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long c = vm->cells[(int)i];
    long r = c ? (n / c) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMODFROMTOCN")||kw(&L->cur,"SMODFROMTOCIMM")||kw(&L->cur,"STACKMODFROMTOCN")||
      kw(&L->cur,"SMODFROMATN")||kw(&L->cur,"SRMODTOCN")||kw(&L->cur,"RMODTOCN")||
      kw(&L->cur,"SCELLMODFROMN")||kw(&L->cur,"SREMFROMTOCN")){
    /* i + n → cells[i] = n % cells[i] (cells[i]==0 → 0 soft), leave remainder */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long c = vm->cells[(int)i];
    long r = c ? (n % c) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 reverse unsigned imm TOC: SUDIVFROMTOCN · SUMODFROMTOCN
   * (imm dual of reverse unsigned after SUDIVFROMN; peer of SDIVFROMTOCN unsigned) */
  if (kw(&L->cur,"SUDIVFROMTOCN")||kw(&L->cur,"SUDIVFROMTOCIMM")||kw(&L->cur,"STACKUDIVFROMTOCN")||
      kw(&L->cur,"SUDIVFROMATN")||kw(&L->cur,"SRUDIVTOCN")||kw(&L->cur,"RUDIVTOCN")||
      kw(&L->cur,"SCELLUDIVFROMN")||kw(&L->cur,"UDIVFROMTOCN")){
    /* i + n → cells[i] = (u)n / (u)cells[i] (cell0 → 0 soft), leave quotient */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long c = (unsigned long)vm->cells[(int)i];
    long r = c ? (long)((unsigned long)n / c) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUMODFROMTOCN")||kw(&L->cur,"SUMODFROMTOCIMM")||kw(&L->cur,"STACKUMODFROMTOCN")||
      kw(&L->cur,"SUMODFROMATN")||kw(&L->cur,"SRUMODTOCN")||kw(&L->cur,"RUMODTOCN")||
      kw(&L->cur,"SCELLUMODFROMN")||kw(&L->cur,"UMODFROMTOCN")||kw(&L->cur,"SUREMFROMTOCN")){
    /* i + n → cells[i] = (u)n % (u)cells[i] (cell0 → 0 soft), leave remainder */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long c = (unsigned long)vm->cells[(int)i];
    long r = c ? (long)((unsigned long)n % c) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 reverse unsigned stack↔cell TOC: SUDIVFROMTOC · SUMODFROMTOC
   * (stack dual of SUDIVFROMTOCN/SUMODFROMTOCN; reverse of SUDIVTOC/SUMODTOC unsigned plane) */
  if (kw(&L->cur,"SUDIVFROMTOC")||kw(&L->cur,"SCELLUDIVFROM")||kw(&L->cur,"STACKUDIVFROMTOC")||
      kw(&L->cur,"SUDIVFROMCELL")||kw(&L->cur,"UDIVFROMTOC")||kw(&L->cur,"SRUDIVTOC")||
      kw(&L->cur,"RUDIVTOC")){
    /* i v → cells[i] = (u)v / (u)cells[i]; cell0 → 0 soft; leave quotient */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long c = (unsigned long)vm->cells[(int)i];
    long r = c ? (long)((unsigned long)v / c) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUMODFROMTOC")||kw(&L->cur,"SCELLUMODFROM")||kw(&L->cur,"STACKUMODFROMTOC")||
      kw(&L->cur,"SUMODFROMCELL")||kw(&L->cur,"UMODFROMTOC")||kw(&L->cur,"SRUMODTOC")||
      kw(&L->cur,"RUMODTOC")||kw(&L->cur,"SUREMFROMTOC")||kw(&L->cur,"UREMFROMTOC")){
    /* i v → cells[i] = (u)v % (u)cells[i]; cell0 → 0 soft; leave remainder */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    unsigned long c = (unsigned long)vm->cells[(int)i];
    long r = c ? (long)((unsigned long)v % c) : 0;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 reverse imm modular TOC: SSUBMODFROMTOCN · SPOWMODFROMTOCN · SMODDIVFROMTOCN
   * (i + args → cells[i] = reverse-mod; imm dual of SSUBMODFROMN after SMODFROMTOCN plane) */
  if (kw(&L->cur,"SSUBMODFROMTOCN")||kw(&L->cur,"SSUBMODFROMTOCIMM")||kw(&L->cur,"STACKSUBMODFROMTOCN")||
      kw(&L->cur,"SSUBMODFROMATN")||kw(&L->cur,"SUBMODFROMTOCN")||kw(&L->cur,"SRSUBMODTOCN")||
      kw(&L->cur,"RSUBMODTOCN")||kw(&L->cur,"SCELLSUBMODFROMN")){
    /* i + k m → cells[i] = (k - cells[i]) mod m; m<=0 → 0; leave result */
    lex_next(L);
    long k = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (m > 0){
      long kk = k % m; if (kk < 0) kk += m;
      long aa = a % m; if (aa < 0) aa += m;
      r = (kk - aa + m) % m;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPOWMODFROMTOCN")||kw(&L->cur,"SPOWMODFROMTOCIMM")||kw(&L->cur,"STACKPOWMODFROMTOCN")||
      kw(&L->cur,"SPOWMODFROMATN")||kw(&L->cur,"POWMODFROMTOCN")||kw(&L->cur,"SRPOWMODTOCN")||
      kw(&L->cur,"RPOWMODTOCN")||kw(&L->cur,"SCELLPOWMODFROMN")||kw(&L->cur,"SBASEPOWMODTOCN")){
    /* i + base m → cells[i] = base^cells[i] mod m; m<=0 or exp<0 → 0; leave result */
    lex_next(L);
    long base_in = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long exp = vm->cells[(int)i];
    long r = 0;
    if (m > 0 && exp >= 0){
      long base = base_in % m; if (base < 0) base += m;
      r = 1 % m;
      long e = exp;
      while (e > 0){
        if (e & 1){
          long y = r, x = base, acc = 0;
          while (y > 0){
            if (y & 1) acc = (acc + x) % m;
            x = (x + x) % m;
            y >>= 1;
          }
          r = acc;
        }
        {
          long x = base, acc = 0, y = base;
          while (y > 0){
            if (y & 1) acc = (acc + x) % m;
            x = (x + x) % m;
            y >>= 1;
          }
          base = acc;
        }
        e >>= 1;
      }
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMODDIVFROMTOCN")||kw(&L->cur,"SMODDIVFROMTOCIMM")||kw(&L->cur,"STACKMODDIVFROMTOCN")||
      kw(&L->cur,"SMODDIVFROMATN")||kw(&L->cur,"MODDIVFROMTOCN")||kw(&L->cur,"SRMODDIVTOCN")||
      kw(&L->cur,"RMODDIVTOCN")||kw(&L->cur,"SCELLMODDIVFROMN")||kw(&L->cur,"SDIVMODFROMTOCN")){
    /* i + a m → cells[i] = a * cells[i]^{-1} mod m; 0 if none / m<=0; leave result */
    lex_next(L);
    long a = parse_expr(vm,L);
    long m = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long b = vm->cells[(int)i];
    long r = 0;
    if (m > 0){
      long bb = b % m; if (bb < 0) bb += m;
      if (bb != 0){
        long t = 0, nt = 1;
        long rr = m, nr = bb;
        while (nr != 0){
          long q = rr / nr;
          long tmp = nt; nt = t - q * nt; t = tmp;
          tmp = nr; nr = rr - q * nr; rr = tmp;
        }
        if (rr == 1){
          if (t < 0) t += m;
          long x = a % m; if (x < 0) x += m;
          long y = t, acc = 0;
          while (y > 0){
            if (y & 1) acc = (acc + x) % m;
            x = (x + x) % m;
            y >>= 1;
          }
          r = acc;
        }
      }
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 reverse modular stack↔cell TOC: SSUBMODFROMTOC · SPOWMODFROMTOC · SMODDIVFROMTOC
   * (stack dual of SSUBMODFROMTOCN/SPOWMODFROMTOCN/SMODDIVFROMTOCN; reverse of SPOWMODTOC plane) */
  if (kw(&L->cur,"SSUBMODFROMTOC")||kw(&L->cur,"SCELLSUBMODFROM")||kw(&L->cur,"STACKSUBMODFROMTOC")||
      kw(&L->cur,"SSUBMODFROMCELL")||kw(&L->cur,"SUBMODFROMTOC")||kw(&L->cur,"SRSUBMODTOC")||
      kw(&L->cur,"RSUBMODTOC")){
    /* i k m → cells[i] = (k - cells[i]) mod m; m<=0 → 0; leave result */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long k = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r = 0;
    if (m > 0){
      long kk = k % m; if (kk < 0) kk += m;
      long aa = a % m; if (aa < 0) aa += m;
      r = (kk - aa + m) % m;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SPOWMODFROMTOC")||kw(&L->cur,"SCELLPOWMODFROM")||kw(&L->cur,"STACKPOWMODFROMTOC")||
      kw(&L->cur,"SPOWMODFROMCELL")||kw(&L->cur,"POWMODFROMTOC")||kw(&L->cur,"SRPOWMODTOC")||
      kw(&L->cur,"RPOWMODTOC")||kw(&L->cur,"SBASEPOWMODTOC")){
    /* i base m → cells[i] = base^cells[i] mod m; m<=0 or exp<0 → 0; leave result */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long base_in = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long exp = vm->cells[(int)i];
    long r = 0;
    if (m > 0 && exp >= 0){
      long base = base_in % m; if (base < 0) base += m;
      r = 1 % m;
      long e = exp;
      while (e > 0){
        if (e & 1){
          long y = r, x = base, acc = 0;
          while (y > 0){
            if (y & 1) acc = (acc + x) % m;
            x = (x + x) % m;
            y >>= 1;
          }
          r = acc;
        }
        {
          long x = base, acc = 0, y = base;
          while (y > 0){
            if (y & 1) acc = (acc + x) % m;
            x = (x + x) % m;
            y >>= 1;
          }
          base = acc;
        }
        e >>= 1;
      }
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMODDIVFROMTOC")||kw(&L->cur,"SCELLMODDIVFROM")||kw(&L->cur,"STACKMODDIVFROMTOC")||
      kw(&L->cur,"SMODDIVFROMCELL")||kw(&L->cur,"MODDIVFROMTOC")||kw(&L->cur,"SRMODDIVTOC")||
      kw(&L->cur,"RMODDIVTOC")||kw(&L->cur,"SDIVMODFROMTOC")){
    /* i a m → cells[i] = a * cells[i]^{-1} mod m; 0 if none / m<=0; leave result */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long m = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long b = vm->cells[(int)i];
    long r = 0;
    if (m > 0){
      long bb = b % m; if (bb < 0) bb += m;
      if (bb != 0){
        long t = 0, nt = 1;
        long rr = m, nr = bb;
        while (nr != 0){
          long q = rr / nr;
          long tmp = nt; nt = t - q * nt; t = tmp;
          tmp = nr; nr = rr - q * nr; rr = tmp;
        }
        if (rr == 1){
          if (t < 0) t += m;
          long x = a % m; if (x < 0) x += m;
          long y = t, acc = 0;
          while (y > 0){
            if (y & 1) acc = (acc + x) % m;
            x = (x + x) % m;
            y >>= 1;
          }
          r = acc;
        }
      }
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 reverse imm ANDN-plane TOC: SANDNFROMTOCN · SORNFROMTOCN · SXORNFROMTOCN
   * (imm reverse of SANDNTOCN/SORNTOCN/SXORNTOCN; n op ~cells[i] after ANDN energy plane) */
  if (kw(&L->cur,"SANDNFROMTOCN")||kw(&L->cur,"SANDNFROMTOCIMM")||kw(&L->cur,"STACKANDNFROMTOCN")||
      kw(&L->cur,"SANDNFROMATN")||kw(&L->cur,"ANDNFROMTOCN")||kw(&L->cur,"SBICFROMTOCN")||
      kw(&L->cur,"BICFROMTOCN")||kw(&L->cur,"SCELLANDNFROMN")||kw(&L->cur,"SRANDNTOCN")||
      kw(&L->cur,"RANDNTOCN")){
    /* i + n → cells[i] = n & ~cells[i], leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = n & ~vm->cells[(int)i];
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORNFROMTOCN")||kw(&L->cur,"SORNFROMTOCIMM")||kw(&L->cur,"STACKORNFROMTOCN")||
      kw(&L->cur,"SORNFROMATN")||kw(&L->cur,"ORNFROMTOCN")||kw(&L->cur,"SORNOTFROMTOCN")||
      kw(&L->cur,"SCELLORNFROMN")||kw(&L->cur,"SRORNTOCN")||kw(&L->cur,"RORNTOCN")){
    /* i + n → cells[i] = n | ~cells[i], leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = n | ~vm->cells[(int)i];
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXORNFROMTOCN")||kw(&L->cur,"SXORNFROMTOCIMM")||kw(&L->cur,"STACKXORNFROMTOCN")||
      kw(&L->cur,"SXORNFROMATN")||kw(&L->cur,"XORNFROMTOCN")||kw(&L->cur,"SXORNOTFROMTOCN")||
      kw(&L->cur,"SCELLXORNFROMN")||kw(&L->cur,"SRXORNTOCN")||kw(&L->cur,"RXORNTOCN")){
    /* i + n → cells[i] = n ^ ~cells[i], leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = n ^ ~vm->cells[(int)i];
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 reverse imm inverted ANDN TOC: SNANDNFROMTOCN · SNORNFROMTOCN · SXNORNFROMTOCN
   * (i + n → cells[i] = ~(n op ~cells[i]); imm dual of SNANDNFROMN after SANDNFROMTOCN) */
  if (kw(&L->cur,"SNANDNFROMTOCN")||kw(&L->cur,"SNANDNFROMTOCIMM")||kw(&L->cur,"STACKNANDNFROMTOCN")||
      kw(&L->cur,"SNANDNFROMATN")||kw(&L->cur,"NANDNFROMTOCN")||kw(&L->cur,"SCELLNANDNFROMN")||
      kw(&L->cur,"RNANDNFROMTOCN")||kw(&L->cur,"SINVERTANDNFROMTOCN")){
    /* i + n → cells[i] = ~(n & ~cells[i])  (= ~n | cells[i]), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ~(n & ~vm->cells[(int)i]);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNORNFROMTOCN")||kw(&L->cur,"SNORNFROMTOCIMM")||kw(&L->cur,"STACKNORNFROMTOCN")||
      kw(&L->cur,"SNORNFROMATN")||kw(&L->cur,"NORNFROMTOCN")||kw(&L->cur,"SCELLNORNFROMN")||
      kw(&L->cur,"RNORNFROMTOCN")||kw(&L->cur,"SINVERTORNFROMTOCN")){
    /* i + n → cells[i] = ~(n | ~cells[i])  (= ~n & cells[i]), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ~(n | ~vm->cells[(int)i]);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNORNFROMTOCN")||kw(&L->cur,"SXNORNFROMTOCIMM")||kw(&L->cur,"STACKXNORNFROMTOCN")||
      kw(&L->cur,"SXNORNFROMATN")||kw(&L->cur,"XNORNFROMTOCN")||kw(&L->cur,"SCELLXNORNFROMN")||
      kw(&L->cur,"RXNORNFROMTOCN")||kw(&L->cur,"SEQUIVNFROMTOCN")){
    /* i + n → cells[i] = ~(n ^ ~cells[i])  (equiv n ^ cells[i]), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ~(n ^ ~vm->cells[(int)i]);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 reverse sat stack↔cell: SSATSUBFROMTOC · SSATDIVFROMTOC (sat dual of reverse TOC) */
  if (kw(&L->cur,"SSATSUBFROMTOC")||kw(&L->cur,"SCELLSATSUBFROM")||kw(&L->cur,"SSATRSUBCELL")||
      kw(&L->cur,"STACKSATSUBFROMCELL")||kw(&L->cur,"SSATSUBFROMCELL")||kw(&L->cur,"SATSUBFROMTOC")){
    /* i v → cells[i] = sat(v - cells[i]) to LONG_MIN..LONG_MAX, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (a > 0 && v < LONG_MIN + a) r = LONG_MIN;
    else if (a < 0 && v > LONG_MAX + a) r = LONG_MAX;
    else r = v - a;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSATDIVFROMTOC")||kw(&L->cur,"SCELLSATDIVFROM")||kw(&L->cur,"SSATRDIVCELL")||
      kw(&L->cur,"STACKSATDIVFROMCELL")||kw(&L->cur,"SSATDIVFROMCELL")||kw(&L->cur,"SATDIVFROMTOC")){
    /* i v → cells[i] = sat(v / cells[i]); cell==0 → 0; LONG_MIN/-1 → LONG_MAX */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (a == 0) r = 0;
    else if (v == LONG_MIN && a == -1) r = LONG_MAX;
    else r = v / a;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 forward sat stack↔cell energy: SSATADDTOC · SSATSUBTOC · SSATDIVTOC · SSATMULTOC · SCLAMPTOC */
  if (kw(&L->cur,"SSATADDTOC")||kw(&L->cur,"SCELLSATADD")||kw(&L->cur,"SSATADDCELL")||
      kw(&L->cur,"STACKSATADDCELL")||kw(&L->cur,"SATADDTOC")){
    /* i v → cells[i] = sat(cells[i] + v), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (v > 0 && a > LONG_MAX - v) r = LONG_MAX;
    else if (v < 0 && a < LONG_MIN - v) r = LONG_MIN;
    else r = a + v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSATSUBTOC")||kw(&L->cur,"SCELLSATSUB")||kw(&L->cur,"SSATSUBCELL")||
      kw(&L->cur,"STACKSATSUBCELL")||kw(&L->cur,"SATSUBTOC")){
    /* i v → cells[i] = sat(cells[i] - v), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (v > 0 && a < LONG_MIN + v) r = LONG_MIN;
    else if (v < 0 && a > LONG_MAX + v) r = LONG_MAX;
    else r = a - v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSATDIVTOC")||kw(&L->cur,"SCELLSATDIV")||kw(&L->cur,"SSATDIVCELL")||
      kw(&L->cur,"STACKSATDIVCELL")||kw(&L->cur,"SATDIVTOC")){
    /* i v → cells[i] = sat(cells[i] / v); v==0 → 0; LONG_MIN/-1 → LONG_MAX */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (v == 0) r = 0;
    else if (a == LONG_MIN && v == -1) r = LONG_MAX;
    else r = a / v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 sat energy ext: SSATMULTOC · SCLAMPTOC (complete sat TOC + bound) */
  if (kw(&L->cur,"SSATMULTOC")||kw(&L->cur,"SCELLSATMUL")||kw(&L->cur,"SSATMULCELL")||
      kw(&L->cur,"STACKSATMULCELL")||kw(&L->cur,"SATMULTOC")){
    /* i v → cells[i] = sat(cells[i] * v), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (a == 0 || v == 0) r = 0;
    else {
      __int128 p = (__int128)a * (__int128)v;
      if (p > (__int128)LONG_MAX) r = LONG_MAX;
      else if (p < (__int128)LONG_MIN) r = LONG_MIN;
      else r = (long)p;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLAMPTOC")||kw(&L->cur,"SCELLCLAMP")||kw(&L->cur,"SBOUNDTOC")||
      kw(&L->cur,"STACKCLAMPTOC")||kw(&L->cur,"CLAMPTOC")||kw(&L->cur,"SCLIPTOC")){
    /* i lo hi → cells[i] = clamp(cells[i], lo, hi), leave result */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long hi = vm->stack[--vm->sp];
    long lo = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (lo > hi){ long t = lo; lo = hi; hi = t; }
    long a = vm->cells[(int)i];
    long r = a;
    if (r < lo) r = lo;
    if (r > hi) r = hi;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 imm sat TOC: SSATADDTOCN · SSATSUBTOCN · SSATMULTOCN · SCLAMPTOCN
   * (imm dual of SSATADDTOC/SSATSUBTOC/SSATMULTOC/SCLAMPTOC; peer of SSATADDN into cell) */
  if (kw(&L->cur,"SSATADDTOCN")||kw(&L->cur,"SSATADDTOCIMM")||kw(&L->cur,"STACKSATADDTOCN")||
      kw(&L->cur,"SSATADDATN")||kw(&L->cur,"SATADDTOCN")||kw(&L->cur,"SCELLSATADDN")||
      kw(&L->cur,"SADDSATTOCN")){
    /* i + n → cells[i] = sat(cells[i]+n), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (n > 0 && a > LONG_MAX - n) r = LONG_MAX;
    else if (n < 0 && a < LONG_MIN - n) r = LONG_MIN;
    else r = a + n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSATSUBTOCN")||kw(&L->cur,"SSATSUBTOCIMM")||kw(&L->cur,"STACKSATSUBTOCN")||
      kw(&L->cur,"SSATSUBATN")||kw(&L->cur,"SATSUBTOCN")||kw(&L->cur,"SCELLSATSUBN")||
      kw(&L->cur,"SSUBSATTOCN")){
    /* i + n → cells[i] = sat(cells[i]-n), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (n > 0 && a < LONG_MIN + n) r = LONG_MIN;
    else if (n < 0 && a > LONG_MAX + n) r = LONG_MAX;
    else r = a - n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSATMULTOCN")||kw(&L->cur,"SSATMULTOCIMM")||kw(&L->cur,"STACKSATMULTOCN")||
      kw(&L->cur,"SSATMULATN")||kw(&L->cur,"SATMULTOCN")||kw(&L->cur,"SCELLSATMULN")||
      kw(&L->cur,"SMULSATTOCN")){
    /* i + n → cells[i] = sat(cells[i]*n), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (a == 0 || n == 0) r = 0;
    else {
      __int128 p = (__int128)a * (__int128)n;
      if (p > (__int128)LONG_MAX) r = LONG_MAX;
      else if (p < (__int128)LONG_MIN) r = LONG_MIN;
      else r = (long)p;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SCLAMPTOCN")||kw(&L->cur,"SCLAMPTOCIMM")||kw(&L->cur,"STACKCLAMPTOCN")||
      kw(&L->cur,"SCLAMPATN")||kw(&L->cur,"CLAMPTOCN")||kw(&L->cur,"SCELLCLAMPN")||
      kw(&L->cur,"SBOUNDTOCN")||kw(&L->cur,"BOUNDTOCN")){
    /* i + lo hi → cells[i] = clamp(cells[i], lo, hi), leave result */
    lex_next(L);
    long lo = parse_expr(vm,L);
    long hi = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (lo > hi){ long t = lo; lo = hi; hi = t; }
    long a = vm->cells[(int)i];
    long r = a;
    if (r < lo) r = lo;
    if (r > hi) r = hi;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 reverse imm sat TOC: SSATDIVTOCN · SSATSUBFROMTOCN · SSATDIVFROMTOCN
   * (complete SSATADDTOCN plane with /; reverse sat dual of SSATSUBFROMTOC/SSATDIVFROMTOC) */
  if (kw(&L->cur,"SSATDIVTOCN")||kw(&L->cur,"SSATDIVTOCIMM")||kw(&L->cur,"STACKSATDIVTOCN")||
      kw(&L->cur,"SSATDIVATN")||kw(&L->cur,"SATDIVTOCN")||kw(&L->cur,"SCELLSATDIVN")||
      kw(&L->cur,"SDIVSATTOCN")){
    /* i + n → cells[i] = sat(cells[i]/n); n==0 → 0; LONG_MIN/-1 → LONG_MAX; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (n == 0) r = 0;
    else if (a == LONG_MIN && n == -1) r = LONG_MAX;
    else r = a / n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSATSUBFROMTOCN")||kw(&L->cur,"SSATSUBFROMTOCIMM")||kw(&L->cur,"STACKSATSUBFROMTOCN")||
      kw(&L->cur,"SSATSUBFROMATN")||kw(&L->cur,"SATSUBFROMTOCN")||kw(&L->cur,"SCELLSATSUBFROMN")||
      kw(&L->cur,"SSATRSUBTOCN")||kw(&L->cur,"SRSATSUBTOCN")||kw(&L->cur,"RSATSUBTOCN")){
    /* i + n → cells[i] = sat(n - cells[i]), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (a > 0 && n < LONG_MIN + a) r = LONG_MIN;
    else if (a < 0 && n > LONG_MAX + a) r = LONG_MAX;
    else r = n - a;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSATDIVFROMTOCN")||kw(&L->cur,"SSATDIVFROMTOCIMM")||kw(&L->cur,"STACKSATDIVFROMTOCN")||
      kw(&L->cur,"SSATDIVFROMATN")||kw(&L->cur,"SATDIVFROMTOCN")||kw(&L->cur,"SCELLSATDIVFROMN")||
      kw(&L->cur,"SSATRDIVTOCN")||kw(&L->cur,"SRSATDIVTOCN")||kw(&L->cur,"RSATDIVTOCN")){
    /* i + n → cells[i] = sat(n / cells[i]); cell0 → 0; LONG_MIN/-1 → LONG_MAX; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long r;
    if (a == 0) r = 0;
    else if (n == LONG_MIN && a == -1) r = LONG_MAX;
    else r = n / a;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack↔cell bitwise accumulate: SANDTOC · SORTOC · SXORTOC
   * (single-index dual of range SANDCELL; names avoid SCELLAND/SANDCELL) */
  if (kw(&L->cur,"SANDTOC")||kw(&L->cur,"SANDTOCELL")||kw(&L->cur,"STACKANDTOC")||
      kw(&L->cur,"BANDTOC")||kw(&L->cur,"SANDAT")){
    /* i v → cells[i] &= v, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] & v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORTOC")||kw(&L->cur,"SORTOCELL")||kw(&L->cur,"STACKORTOC")||
      kw(&L->cur,"BORTOC")||kw(&L->cur,"SORAT")){
    /* i v → cells[i] |= v, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] | v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXORTOC")||kw(&L->cur,"SXORTOCELL")||kw(&L->cur,"STACKXORTOC")||
      kw(&L->cur,"BXORTOC")||kw(&L->cur,"SXORAT")){
    /* i v → cells[i] ^= v, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] ^ v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 energy inverted bitwise TOC: SNANDTOC · SNORTOC · SXNORTOC (after SAND/OR/XOR TOC) */
  if (kw(&L->cur,"SNANDTOC")||kw(&L->cur,"SNANDTOCELL")||kw(&L->cur,"STACKNANDTOC")||
      kw(&L->cur,"BNANDTOC")||kw(&L->cur,"SNANDAT")){
    /* i v → cells[i] = ~(cells[i] & v), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ~(vm->cells[(int)i] & v);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNORTOC")||kw(&L->cur,"SNORTOCELL")||kw(&L->cur,"STACKNORTOC")||
      kw(&L->cur,"BNORTOC")||kw(&L->cur,"SNORAT")){
    /* i v → cells[i] = ~(cells[i] | v), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ~(vm->cells[(int)i] | v);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNORTOC")||kw(&L->cur,"SXNORTOCELL")||kw(&L->cur,"STACKXNORTOC")||
      kw(&L->cur,"BXNORTOC")||kw(&L->cur,"SEQUIVTOC")||kw(&L->cur,"SXNORAT")){
    /* i v → cells[i] = ~(cells[i] ^ v), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = ~(vm->cells[(int)i] ^ v);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-6 energy ANDN plane TOC: SANDNTOC · SORNTOC · SXORNTOC (cells[i] op ~v after SNAND/NOR/XNOR TOC) */
  if (kw(&L->cur,"SANDNTOC")||kw(&L->cur,"SANDNTOCELL")||kw(&L->cur,"STACKANDNTOC")||
      kw(&L->cur,"SBICTOC")||kw(&L->cur,"SANDNOTTOC")||kw(&L->cur,"SANDNAT")||
      kw(&L->cur,"BICTOC")){
    /* i v → cells[i] &= ~v, leave result (clear bits set in v) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] & ~v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORNTOC")||kw(&L->cur,"SORNTOCELL")||kw(&L->cur,"STACKORNTOC")||
      kw(&L->cur,"SORNOTTOC")||kw(&L->cur,"SORNAT")||kw(&L->cur,"ORNOTTOC")){
    /* i v → cells[i] |= ~v, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] | ~v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXORNTOC")||kw(&L->cur,"SXORNTOCELL")||kw(&L->cur,"STACKXORNTOC")||
      kw(&L->cur,"SXORNOTTOC")||kw(&L->cur,"SXORNAT")||kw(&L->cur,"XORNOTTOC")){
    /* i v → cells[i] ^= ~v, leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long r = vm->cells[(int)i] ^ ~v;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack↔cell shift foundation: SSHLTOC · SSHRTOC · SSARTOC (after SSHL/SSHR/SSAR stack + bitwise TOC) */
  if (kw(&L->cur,"SSHLTOC")||kw(&L->cur,"SSHLTOCELL")||kw(&L->cur,"STACKSHLTOC")||
      kw(&L->cur,"SHLTOC")||kw(&L->cur,"SCELLSHL")||kw(&L->cur,"SSHLAT")){
    /* i n → cells[i] <<= n (n clamped 0..63), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    long r = (long)((unsigned long)vm->cells[(int)i] << (unsigned)n);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSHRTOC")||kw(&L->cur,"SSHRTOCELL")||kw(&L->cur,"STACKSHRTOC")||
      kw(&L->cur,"SHRTOC")||kw(&L->cur,"SCELLSHR")||kw(&L->cur,"SSHRAT")){
    /* i n → cells[i] = logical cells[i] >> n (n clamped 0..63), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    long r = (long)((unsigned long)vm->cells[(int)i] >> (unsigned)n);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSARTOC")||kw(&L->cur,"SSARTOCELL")||kw(&L->cur,"STACKSARTOC")||
      kw(&L->cur,"SARTOC")||kw(&L->cur,"SCELLSAR")||kw(&L->cur,"SASHRTOC")||
      kw(&L->cur,"SSARAT")||kw(&L->cur,"ASHRTOC")){
    /* i n → cells[i] = arithmetic cells[i] >> n (sign-preserving; n clamped 0..63), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    long r = vm->cells[(int)i] >> n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack foundation imm shift TOC: SSHLTOCN · SSHRTOCN · SSARTOCN
   * (imm dual of SSHLTOC/SSHRTOC/SSARTOC; peer of SSHLN/SSHRN/SSARN into cell) */
  if (kw(&L->cur,"SSHLTOCN")||kw(&L->cur,"SSHLTOCIMM")||kw(&L->cur,"STACKSHLTOCN")||
      kw(&L->cur,"SSHLATN")||kw(&L->cur,"SHLTOCN")||kw(&L->cur,"SCELLSHLN")){
    /* i + n → cells[i] <<= n (n clamped 0..63), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    long r = (long)((unsigned long)vm->cells[(int)i] << (unsigned)n);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSHRTOCN")||kw(&L->cur,"SSHRTOCIMM")||kw(&L->cur,"STACKSHRTOCN")||
      kw(&L->cur,"SSHRATN")||kw(&L->cur,"SHRTOCN")||kw(&L->cur,"SCELLSHRN")){
    /* i + n → cells[i] logical >>= n (n clamped 0..63), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    long r = (long)((unsigned long)vm->cells[(int)i] >> (unsigned)n);
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSARTOCN")||kw(&L->cur,"SSARTOCIMM")||kw(&L->cur,"STACKSARTOCN")||
      kw(&L->cur,"SSARATN")||kw(&L->cur,"SARTOCN")||kw(&L->cur,"SCELLSARN")||
      kw(&L->cur,"SASHRTOCN")||kw(&L->cur,"ASHRTOCN")){
    /* i + n → cells[i] arithmetic >>= n (n clamped 0..63), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    if (n > 63) n = 63;
    long r = vm->cells[(int)i] >> n;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack↔cell rotate dual: SROLTOC · SRORTOC (complete shift/rotate TOC after SSHL/SHR/SAR TOC) */
  if (kw(&L->cur,"SROLTOC")||kw(&L->cur,"SROLTOCELL")||kw(&L->cur,"STACKROLTOC")||
      kw(&L->cur,"ROLTOC")||kw(&L->cur,"SCELLROL")||kw(&L->cur,"SROTLTOC")||
      kw(&L->cur,"SROLAT")){
    /* i n → cells[i] = rotl(cells[i], n mod 64), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    unsigned uk = (unsigned)(n & 63);
    unsigned long u = (unsigned long)vm->cells[(int)i];
    long r = (uk == 0) ? (long)u : (long)((u << uk) | (u >> (64u - uk)));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SRORTOC")||kw(&L->cur,"SRORTOCELL")||kw(&L->cur,"STACKRORTOC")||
      kw(&L->cur,"RORTOC")||kw(&L->cur,"SCELLROR")||kw(&L->cur,"SROTRTOC")||
      kw(&L->cur,"SRORAT")){
    /* i n → cells[i] = rotr(cells[i], n mod 64), leave result */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    long i = vm->stack[--vm->sp];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    unsigned uk = (unsigned)(n & 63);
    unsigned long u = (unsigned long)vm->cells[(int)i];
    long r = (uk == 0) ? (long)u : (long)((u >> uk) | (u << (64u - uk)));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 imm rotate TOC: SROLTOCN · SRORTOCN
   * (imm dual of SROLTOC/SRORTOC; complete shift/rotate TOCN after SSHLTOCN plane) */
  if (kw(&L->cur,"SROLTOCN")||kw(&L->cur,"SROLTOCIMM")||kw(&L->cur,"STACKROLTOCN")||
      kw(&L->cur,"SROLATN")||kw(&L->cur,"ROLTOCN")||kw(&L->cur,"SCELLROLN")||
      kw(&L->cur,"SROTLTOCN")){
    /* i + n → cells[i] = rotl(cells[i], n mod 64), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    unsigned uk = (unsigned)(n & 63);
    unsigned long u = (unsigned long)vm->cells[(int)i];
    long r = (uk == 0) ? (long)u : (long)((u << uk) | (u >> (64u - uk)));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SRORTOCN")||kw(&L->cur,"SRORTOCIMM")||kw(&L->cur,"STACKRORTOCN")||
      kw(&L->cur,"SRORATN")||kw(&L->cur,"RORTOCN")||kw(&L->cur,"SCELLRORN")||
      kw(&L->cur,"SROTRTOCN")){
    /* i + n → cells[i] = rotr(cells[i], n mod 64), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    if (n < 0) n = 0;
    unsigned uk = (unsigned)(n & 63);
    unsigned long u = (unsigned long)vm->cells[(int)i];
    long r = (uk == 0) ? (long)u : (long)((u >> uk) | (u << (64u - uk)));
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack↔cell unary ALU: SABSTOC · SNEGTOC · SNOTTOC (dual of SABS/SNEG/SNOT) */
  if (kw(&L->cur,"SABSTOC")||kw(&L->cur,"SABSCELL")||kw(&L->cur,"STACKABSTOC")||
      kw(&L->cur,"SCELLABS")||kw(&L->cur,"ABSAT")){
    /* i → cells[i] = |cells[i]|, leave abs (LONG_MIN stays LONG_MIN on wrap) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long v = vm->cells[(int)i];
    if (v < 0) v = -v;
    vm->cells[(int)i] = v;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNEGTOC")||kw(&L->cur,"SNEGCELL")||kw(&L->cur,"STACKNEGTOC")||
      kw(&L->cur,"SCELLNEG")||kw(&L->cur,"NEGAT")){
    /* i → cells[i] = -cells[i], leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long v = -vm->cells[(int)i];
    vm->cells[(int)i] = v;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNOTTOC")||kw(&L->cur,"SNOTTOCELL")||kw(&L->cur,"STACKNOTTOC")||
      kw(&L->cur,"BINVTOC")||kw(&L->cur,"SBNOTAT")){
    /* i → cells[i] = ~cells[i], leave result (avoid SNOTCELL range alias) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long v = ~vm->cells[(int)i];
    vm->cells[(int)i] = v;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack↔cell unary scale: SDBLTOC · SHALFTOC (dual of SDBL/SHALF after ABS/NEG/NOT TOC) */
  if (kw(&L->cur,"SDBLTOC")||kw(&L->cur,"SDBLTOCELL")||kw(&L->cur,"STACKDBLTOC")||
      kw(&L->cur,"SDOUBLETOC")||kw(&L->cur,"SCELLDBL")||kw(&L->cur,"SDBLAT")||
      kw(&L->cur,"DBLTOC")){
    /* i → cells[i] *= 2, leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long v = vm->cells[(int)i] * 2;
    vm->cells[(int)i] = v;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHALFTOC")||kw(&L->cur,"SHALFTOCELL")||kw(&L->cur,"STACKHALFTOC")||
      kw(&L->cur,"SHALVETOC")||kw(&L->cur,"SCELLHALF")||kw(&L->cur,"SHALFAT")||
      kw(&L->cur,"HALFTOC")){
    /* i → cells[i] /= 2 (toward zero), leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long v = vm->cells[(int)i] / 2;
    vm->cells[(int)i] = v;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack↔cell unary mutator: SINCTOC · SDECTOC (dual of SINC/SDEC after SDBL/SHALF TOC) */
  if (kw(&L->cur,"SINCTOC")||kw(&L->cur,"SINCTOCELL")||kw(&L->cur,"STACKINCTOC")||
      kw(&L->cur,"SINCELLTOC")||kw(&L->cur,"SINCAT")||kw(&L->cur,"INCTOC")||
      kw(&L->cur,"SCELLINCTOC")){
    /* i → cells[i] += 1, leave result (TOC dual of SINC; peer of SINCCELL) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long v = vm->cells[(int)i] + 1;
    vm->cells[(int)i] = v;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SDECTOC")||kw(&L->cur,"SDECTOCELL")||kw(&L->cur,"STACKDECTOC")||
      kw(&L->cur,"SDECELLTOC")||kw(&L->cur,"SDECAT")||kw(&L->cur,"DECTOC")||
      kw(&L->cur,"SCELLDECTOC")){
    /* i → cells[i] -= 1, leave result (TOC dual of SDEC; peer of SDECCELL) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long v = vm->cells[(int)i] - 1;
    vm->cells[(int)i] = v;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 stack↔cell math dual: SSQRTOC · SISQRTTOC (dual of SSQR/SISQRT after scale TOC) */
  if (kw(&L->cur,"SSQRTOC")||kw(&L->cur,"SSQRTOCELL")||kw(&L->cur,"STACKSQRTOC")||
      kw(&L->cur,"SSQUARETOC")||kw(&L->cur,"SCELLSQR")||kw(&L->cur,"SQRTOC")||
      kw(&L->cur,"SSQRAT")){
    /* i → cells[i] = cells[i]*cells[i], leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long v = a * a;
    vm->cells[(int)i] = v;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SISQRTTOC")||kw(&L->cur,"SISQRTTOCELL")||kw(&L->cur,"STACKISQRTTOC")||
      kw(&L->cur,"SSQRTTOC")||kw(&L->cur,"SCELLISQRT")||kw(&L->cur,"ISQRTTOC")||
      kw(&L->cur,"SISQRTAT")){
    /* i → cells[i] = isqrt(cells[i]); neg → 0, leave result */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long v = 0;
    if (a < 0) v = 0;
    else {
      long t = 0;
      while ((t + 1) * (t + 1) <= a) t++;
      v = t;
    }
    vm->cells[(int)i] = v;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-2 imm numthy TOC: SGCDTOCN · SLCMTOCN
   * (imm dual of SGCDN/SLCMN into cell; complete SSQRTOC plane with gcd/lcm) */
  if (kw(&L->cur,"SGCDTOCN")||kw(&L->cur,"SGCDTOCIMM")||kw(&L->cur,"STACKGCDTOCN")||
      kw(&L->cur,"SGCDATN")||kw(&L->cur,"GCDTOCN")||kw(&L->cur,"SCELLGCDN")||
      kw(&L->cur,"GCDINTOCN")){
    /* i + n → cells[i] = gcd(|cells[i]|,|n|), leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long x = a < 0 ? -a : a, y = n < 0 ? -n : n;
    while (y){ long t = x % y; x = y; y = t; }
    long r = x;
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SLCMTOCN")||kw(&L->cur,"SLCMTOCIMM")||kw(&L->cur,"STACKLCMTOCN")||
      kw(&L->cur,"SLCMATN")||kw(&L->cur,"LCMTOCN")||kw(&L->cur,"SCELLLCMN")||
      kw(&L->cur,"LCMINTOCN")){
    /* i + n → cells[i] = lcm(|cells[i]|,|n|); 0 if either side 0; leave result */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long i = vm->stack[vm->sp - 1];
    if (i < 0) i = 0;
    if (i >= CUBALC_CELL_N) i = CUBALC_CELL_N - 1;
    long a = vm->cells[(int)i];
    long x = a < 0 ? -a : a, y = n < 0 ? -n : n;
    long r = 0;
    if (x && y){
      long g = x, h = y;
      while (h){ long t = g % h; g = h; h = t; }
      r = (x / g) * y;
    }
    vm->cells[(int)i] = r;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
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
