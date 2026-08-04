/* CubalC lang — lang_ops_stack.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_stack(VM *vm, Lex *L){
  /* plane ops_stack: L10809-12579 */
  /* digit-4 data plane: Forth-style stack combinators + FILLCELL */
  if (kw(&L->cur,"DUP")||kw(&L->cur,"STACKDUP")){
    lex_next(L);
    if (vm->sp <= 0){
      var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0;
      bump(vm); return 1;
    }
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[vm->sp - 1];
    vm->stack[vm->sp++] = v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DROP")||kw(&L->cur,"STACKDROP")){
    lex_next(L);
    if (vm->sp <= 0){
      var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0;
      bump(vm); return 1;
    }
    long v = vm->stack[--vm->sp];
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SWAP")||kw(&L->cur,"STACKSWAP")||kw(&L->cur,"SWAPTOP")){
    /* SWAP only if not already consumed as SWAPCELL (checked earlier) */
    lex_next(L);
    if (vm->sp < 2){
      var_set_num(vm,"OK",0); bump(vm); return 1;
    }
    long t = vm->stack[vm->sp-1];
    vm->stack[vm->sp-1] = vm->stack[vm->sp-2];
    vm->stack[vm->sp-2] = t;
    var_set_num(vm,"LAST_N",vm->stack[vm->sp-1]); vm->last_n=vm->stack[vm->sp-1];
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"OVER")||kw(&L->cur,"STACKOVER")){
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[vm->sp - 2];
    vm->stack[vm->sp++] = v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ROT")||kw(&L->cur,"ROTSTACK")||kw(&L->cur,"STACKROT")){
    /* a b c → b c a  (rotate top 3 left) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-3], b = vm->stack[vm->sp-2], c = vm->stack[vm->sp-1];
    vm->stack[vm->sp-3] = b;
    vm->stack[vm->sp-2] = c;
    vm->stack[vm->sp-1] = a;
    var_set_num(vm,"LAST_N",a); vm->last_n=a;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack foundation: NDROP · SEMPTY/SFULL · SSWAPN */
  if (kw(&L->cur,"NDROP")||kw(&L->cur,"DROPN")||kw(&L->cur,"STACKNDROP")||
      kw(&L->cur,"DROPN")){
    /* NDROP n — drop top n items (n clamped to sp) */
    lex_next(L);
    long n = 0;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE")))
      n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (n > (long)vm->sp) n = (long)vm->sp;
    vm->sp -= (int)n;
    long last = (vm->sp > 0) ? vm->stack[vm->sp - 1] : 0;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SEMPTY")||kw(&L->cur,"STACKEMPTY")||kw(&L->cur,"SEMPTYP")||
      kw(&L->cur,"ISEMPTY")||kw(&L->cur,"EMPTYSTACK")){
    /* SEMPTY → LAST_N = 1 if stack empty */
    lex_next(L);
    long r = (vm->sp == 0) ? 1 : 0;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SFULL")||kw(&L->cur,"STACKFULL")||kw(&L->cur,"SFULLP")||
      kw(&L->cur,"ISFULL")||kw(&L->cur,"FULLSTACK")){
    /* SFULL → LAST_N = 1 if stack at capacity */
    lex_next(L);
    long r = (vm->sp >= CUBALC_STACK_N) ? 1 : 0;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SSWAPN")||kw(&L->cur,"SWAPN")||kw(&L->cur,"STACKSWAPN")||
      kw(&L->cur,"XCHGN")||kw(&L->cur,"SEXCHN")){
    /* SSWAPN n — exchange TOS with item n under top (0=noop self) */
    lex_next(L);
    long n = 0;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE")))
      n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (vm->sp < 1 || n >= (long)vm->sp){
      var_set_num(vm,"OK",0); bump(vm); return 1;
    }
    int i = vm->sp - 1;
    int j = vm->sp - 1 - (int)n;
    long t = vm->stack[i];
    vm->stack[i] = vm->stack[j];
    vm->stack[j] = t;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",vm->stack[i]); vm->last_n=vm->stack[i];
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* digit-1 stack index mutators: SREPLACE · SDROPAT */
  if (kw(&L->cur,"SREPLACE")||kw(&L->cur,"SPUT")||kw(&L->cur,"SSTOREN")||
      kw(&L->cur,"STACKPUT")||kw(&L->cur,"PLACEAT")){
    /* SREPLACE n — write TOS into depth-n slot, then drop TOS (n=0≡DROP) */
    lex_next(L);
    long n = 0;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE")))
      n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (vm->sp < 1 || n >= vm->sp){
      var_set_num(vm,"OK",0); bump(vm); return 1;
    }
    long v = vm->stack[vm->sp - 1];
    vm->stack[vm->sp - 1 - (int)n] = v;
    vm->sp--;
    long last = (vm->sp > 0) ? vm->stack[vm->sp - 1] : v;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SDROPAT")||kw(&L->cur,"NIPN")||kw(&L->cur,"DROPAT")||
      kw(&L->cur,"STACKDROPAT")||kw(&L->cur,"REMOVEAT")){
    /* SDROPAT n — remove item at depth n, close gap (n=0≡DROP, n=1≡NIP) */
    lex_next(L);
    long n = 0;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE")))
      n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (vm->sp < 1 || n >= vm->sp){
      var_set_num(vm,"OK",0); bump(vm); return 1;
    }
    int idx = vm->sp - 1 - (int)n;
    for (int j = idx; j < vm->sp - 1; j++)
      vm->stack[j] = vm->stack[j + 1];
    vm->sp--;
    long last = (vm->sp > 0) ? vm->stack[vm->sp - 1] : 0;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack index insert + bulk push + cell accumulate */
  if (kw(&L->cur,"SINSERT")||kw(&L->cur,"INSERTAT")||kw(&L->cur,"UNROLL")||
      kw(&L->cur,"RROLL")||kw(&L->cur,"STACKINSERT")||kw(&L->cur,"MOVETODEEP")){
    /* SINSERT/UNROLL n — move TOS to depth n (n=0 noop; n=1≡SWAP; n=2≡-ROT) */
    lex_next(L);
    long n = 0;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE")))
      n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (vm->sp < 1 || n >= vm->sp){
      var_set_num(vm,"OK",0); bump(vm); return 1;
    }
    long v = vm->stack[vm->sp - 1];
    for (int j = vm->sp - 1; j > vm->sp - 1 - (int)n; j--)
      vm->stack[j] = vm->stack[j - 1];
    vm->stack[vm->sp - 1 - (int)n] = v;
    long last = vm->stack[vm->sp - 1];
    var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"RROT")||kw(&L->cur,"NROT")||kw(&L->cur,"-ROT")||
      kw(&L->cur,"REVROT")||kw(&L->cur,"STACKRROT")){
    /* RROT / -ROT: a b c → c a b  (≡ SINSERT 2) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-3], b = vm->stack[vm->sp-2], c = vm->stack[vm->sp-1];
    vm->stack[vm->sp-3] = c;
    vm->stack[vm->sp-2] = a;
    vm->stack[vm->sp-1] = b;
    var_set_num(vm,"LAST_N",b); vm->last_n=b;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"NPUSH")||kw(&L->cur,"PUSHN")||kw(&L->cur,"STACKNPUSH")||
      kw(&L->cur,"REPPUSH")){
    /* NPUSH v n — push value v, n times (n clamped; soft-fail on overflow) */
    lex_next(L);
    long v = parse_expr(vm,L);
    long n = 1;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE")))
      n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (n == 0){
      var_set_num(vm,"OK",1); var_set_num(vm,"LAST_N",0); vm->last_n=0;
      var_set_num(vm,"SP",vm->sp); bump(vm); return 1;
    }
    if (vm->sp + n > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    for (long i = 0; i < n; i++)
      vm->stack[vm->sp++] = v;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SKEEP")||kw(&L->cur,"KEEPN")||kw(&L->cur,"STACKKEEP")||
      kw(&L->cur,"KEEP")||kw(&L->cur,"STAYN")){
    /* SKEEP n — keep only top n items; drop everything under (n clamped) */
    lex_next(L);
    long n = 0;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE")))
      n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (n > (long)vm->sp) n = (long)vm->sp;
    if (n < (long)vm->sp){
      int base = vm->sp - (int)n;
      for (int i = 0; i < (int)n; i++)
        vm->stack[i] = vm->stack[base + i];
      vm->sp = (int)n;
    }
    long last = (vm->sp > 0) ? vm->stack[vm->sp - 1] : 0;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"PICK")||kw(&L->cur,"STACKPICK")){
    /* PICK n — copy n-th under top (0=top) onto stack; depth from TOS */
    lex_next(L);
    long n = 0;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE")))
      n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (vm->sp <= 0 || n >= vm->sp){
      var_set_num(vm,"OK",0); var_set_num(vm,"LAST_N",0); vm->last_n=0;
      bump(vm); return 1;
    }
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[vm->sp - 1 - (int)n];
    vm->stack[vm->sp++] = v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack duals (n from TOS): SPICK · SROLL · SNDROP (dual of PICK/ROLL/NDROP imm) */
  if (kw(&L->cur,"SPICK")||kw(&L->cur,"PICKS")||kw(&L->cur,"STACKPICKS")||
      kw(&L->cur,"SPICKST")||kw(&L->cur,"PICKST")){
    /* SPICK — pop n, copy n-th under remaining TOS onto stack */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    if (n < 0) n = 0;
    if (vm->sp <= 0 || n >= vm->sp){
      var_set_num(vm,"OK",0); var_set_num(vm,"SP",vm->sp);
      var_set_num(vm,"LAST_N",0); vm->last_n=0; bump(vm); return 1;
    }
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[vm->sp - 1 - (int)n];
    vm->stack[vm->sp++] = v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SROLL")||kw(&L->cur,"ROLLS")||kw(&L->cur,"STACKROLLS")||
      kw(&L->cur,"SROLLST")||kw(&L->cur,"ROLLST")){
    /* SROLL — pop n, rotate top (n+1) items (dual of ROLL n) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    if (n < 0) n = 0;
    if (n == 0){
      var_set_num(vm,"OK", vm->sp > 0 ? 1 : 0);
      if (vm->sp > 0){ var_set_num(vm,"LAST_N",vm->stack[vm->sp-1]); vm->last_n=vm->stack[vm->sp-1]; }
      var_set_num(vm,"SP",vm->sp); bump(vm); return 1;
    }
    if (vm->sp <= n){ var_set_num(vm,"OK",0); var_set_num(vm,"SP",vm->sp); bump(vm); return 1; }
    long v = vm->stack[vm->sp - 1 - (int)n];
    for (int i = (int)n; i > 0; i--)
      vm->stack[vm->sp - 1 - i] = vm->stack[vm->sp - i];
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNDROP")||kw(&L->cur,"DROPS")||kw(&L->cur,"STACKDROPS")||
      kw(&L->cur,"SNDROPST")||kw(&L->cur,"NDROPS")){
    /* SNDROP — pop n, drop that many remaining top items (dual of NDROP n) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long n = vm->stack[--vm->sp];
    if (n < 0) n = 0;
    if (n > (long)vm->sp) n = (long)vm->sp;
    vm->sp -= (int)n;
    long last = (vm->sp > 0) ? vm->stack[vm->sp - 1] : 0;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack combinators: SFILL · DROPZ · DROPNZ */
  if (kw(&L->cur,"SFILL")||kw(&L->cur,"STACKFILL")||kw(&L->cur,"FILLSTK")||
      kw(&L->cur,"FILLTOP")){
    /* SFILL n v — write value v into top n stack slots (need n items) */
    lex_next(L);
    long n = parse_expr(vm,L);
    long v = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (n == 0){
      var_set_num(vm,"OK",1); var_set_num(vm,"LAST_N",v); vm->last_n=v;
      var_set_num(vm,"SP",vm->sp); bump(vm); return 1;
    }
    if (vm->sp < n){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    for (int i = 0; i < (int)n; i++)
      vm->stack[vm->sp - 1 - i] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DROPZ")||kw(&L->cur,"SDROPZ")||kw(&L->cur,"DROPIF0")||
      kw(&L->cur,"DROPZERO")||kw(&L->cur,"STACKDROPZ")){
    /* DROPZ — drop TOS if it is zero; leave nonzero */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[vm->sp - 1];
    if (v == 0){
      vm->sp--;
      long last = (vm->sp > 0) ? vm->stack[vm->sp - 1] : 0;
      var_set_num(vm,"LAST_N",last); vm->last_n=last;
    } else {
      var_set_num(vm,"LAST_N",v); vm->last_n=v;
    }
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"DROPNZ")||kw(&L->cur,"SDROPNZ")||kw(&L->cur,"DROPIF")||
      kw(&L->cur,"DROPNONZERO")||kw(&L->cur,"STACKDROPNZ")){
    /* DROPNZ — drop TOS if nonzero; leave zero */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[vm->sp - 1];
    if (v != 0){
      vm->sp--;
      long last = (vm->sp > 0) ? vm->stack[vm->sp - 1] : 0;
      var_set_num(vm,"LAST_N",last); vm->last_n=last;
    } else {
      var_set_num(vm,"LAST_N",0); vm->last_n=0;
    }
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack combinators ext: QDUP NDUP SREVERSE */
  if (kw(&L->cur,"QDUP")||kw(&L->cur,"DUPNZ")||kw(&L->cur,"DUPIF")||
      kw(&L->cur,"DUPNONZERO")){
    /* QDUP — duplicate TOS only if nonzero (Forth ?DUP) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[vm->sp - 1];
    if (v != 0){
      if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
      vm->stack[vm->sp++] = v;
    }
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-1 stack conditionals: DUPZ · SSWAPIF · SKEEPIF · SNIPIF */
  if (kw(&L->cur,"DUPZ")||kw(&L->cur,"QDUP0")||kw(&L->cur,"DUPIF0")||
      kw(&L->cur,"DUPZERO")||kw(&L->cur,"STACKDUPZ")){
    /* DUPZ — duplicate TOS only if zero (complement of QDUP) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[vm->sp - 1];
    if (v == 0){
      if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
      vm->stack[vm->sp++] = 0;
    }
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSWAPIF")||kw(&L->cur,"SWAPIF")||kw(&L->cur,"QSWAP")||
      kw(&L->cur,"STACKSWAPIF")||kw(&L->cur,"CSWAP")){
    /* a b f → if f then b a else a b  (conditional swap) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long f = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    if (f){ long t=a; a=b; b=t; }
    vm->stack[vm->sp++] = a;
    vm->stack[vm->sp++] = b;
    var_set_num(vm,"LAST_N",b); vm->last_n=b;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SKEEPIF")||kw(&L->cur,"KEEPIF")||kw(&L->cur,"QKEEP")||
      kw(&L->cur,"STACKKEEPIF")||kw(&L->cur,"KEEPWHEN")){
    /* v f → if f leave v else drop both */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long f = vm->stack[--vm->sp];
    long v = vm->stack[--vm->sp];
    if (f){
      vm->stack[vm->sp++] = v;
      var_set_num(vm,"LAST_N",v); vm->last_n=v;
    } else {
      long last = (vm->sp > 0) ? vm->stack[vm->sp - 1] : 0;
      var_set_num(vm,"LAST_N",last); vm->last_n=last;
    }
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNIPIF")||kw(&L->cur,"NIPIF")||kw(&L->cur,"STACKNIPIF")||
      kw(&L->cur,"CNIP")||kw(&L->cur,"NIPWHEN")){
    /* a b f → if f then b else a  (conditional choose; QNIP reserved for 4NIP) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long f = vm->stack[--vm->sp];
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = f ? b : a;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"NDUP")||kw(&L->cur,"DUPN")||kw(&L->cur,"STACKNDUP")){
    /* NDUP n — duplicate top n items (append copy of top n) */
    lex_next(L);
    long n = 0;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE")))
      n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (n == 0){ var_set_num(vm,"OK",1); bump(vm); return 1; }
    if (vm->sp < n){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + n > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    int base = vm->sp - (int)n;
    long last = 0;
    for (int i = 0; i < (int)n; i++){
      last = vm->stack[base + i];
      vm->stack[vm->sp++] = last;
    }
    var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SREVERSE")||kw(&L->cur,"REVSTACK")||kw(&L->cur,"STACKREV")||
      kw(&L->cur,"SREVST")){
    /* SREVERSE [n] — reverse top n items (omit n → whole stack) */
    lex_next(L);
    int have_n = 0;
    long n = 0;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE") &&
         !kw(&L->cur,"PUSH") && !kw(&L->cur,"POP") && !kw(&L->cur,"CLEARSTACK") &&
         !kw(&L->cur,"PEEK") && !kw(&L->cur,"DROP") && !kw(&L->cur,"DUP"))){
      n = parse_expr(vm,L);
      have_n = 1;
    }
    if (!have_n) n = (long)vm->sp;
    if (n < 0) n = 0;
    if (have_n && n > (long)vm->sp){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n > 1){
      int lo = vm->sp - (int)n;
      int hi = vm->sp - 1;
      while (lo < hi){
        long t = vm->stack[lo];
        vm->stack[lo] = vm->stack[hi];
        vm->stack[hi] = t;
        lo++; hi--;
      }
    }
    long last = (vm->sp > 0) ? vm->stack[vm->sp - 1] : 0;
    var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack depth plane: NIP TUCK 2DUP 2DROP 2SWAP 2OVER 2ROT 2NIP ROLL DEPTH */
  if (kw(&L->cur,"NIP")||kw(&L->cur,"STACKNIP")){
    /* a b → b  (drop under top) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp - 2] = vm->stack[vm->sp - 1];
    vm->sp--;
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",vm->stack[vm->sp-1]); vm->last_n=vm->stack[vm->sp-1];
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"TUCK")||kw(&L->cur,"STACKTUCK")){
    /* a b → b a b */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-2], b = vm->stack[vm->sp-1];
    vm->stack[vm->sp-2] = b;
    vm->stack[vm->sp-1] = a;
    vm->stack[vm->sp++] = b;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",b); vm->last_n=b;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"2DUP")||kw(&L->cur,"DDUP")||kw(&L->cur,"DUP2")){
    /* a b → a b a b */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 2 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-2], b = vm->stack[vm->sp-1];
    vm->stack[vm->sp++] = a;
    vm->stack[vm->sp++] = b;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",b); vm->last_n=b;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"2DROP")||kw(&L->cur,"DDROP")||kw(&L->cur,"DROP2")){
    /* a b → (empty of top 2) */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->sp -= 2;
    var_set_num(vm,"SP",vm->sp);
    if (vm->sp > 0){ var_set_num(vm,"LAST_N",vm->stack[vm->sp-1]); vm->last_n=vm->stack[vm->sp-1]; }
    else { var_set_num(vm,"LAST_N",0); vm->last_n=0; }
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"2SWAP")||kw(&L->cur,"DSWAP")||kw(&L->cur,"SWAP2")){
    /* a b c d → c d a b */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-4], b = vm->stack[vm->sp-3];
    long c = vm->stack[vm->sp-2], d = vm->stack[vm->sp-1];
    vm->stack[vm->sp-4] = c; vm->stack[vm->sp-3] = d;
    vm->stack[vm->sp-2] = a; vm->stack[vm->sp-1] = b;
    var_set_num(vm,"LAST_N",b); vm->last_n=b;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"2OVER")||kw(&L->cur,"DOVER")||kw(&L->cur,"OVER2")){
    /* a b c d → a b c d a b  (copy pair under top pair) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 2 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-4], b = vm->stack[vm->sp-3];
    vm->stack[vm->sp++] = a;
    vm->stack[vm->sp++] = b;
    var_set_num(vm,"LAST_N",b); vm->last_n=b;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"2ROT")||kw(&L->cur,"DROT")||kw(&L->cur,"ROT2")){
    /* a b c d e f → c d e f a b  (rotate three pairs left) */
    lex_next(L);
    if (vm->sp < 6){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-6], b = vm->stack[vm->sp-5];
    long c = vm->stack[vm->sp-4], d = vm->stack[vm->sp-3];
    long e = vm->stack[vm->sp-2], f = vm->stack[vm->sp-1];
    vm->stack[vm->sp-6] = c; vm->stack[vm->sp-5] = d;
    vm->stack[vm->sp-4] = e; vm->stack[vm->sp-3] = f;
    vm->stack[vm->sp-2] = a; vm->stack[vm->sp-1] = b;
    var_set_num(vm,"LAST_N",b); vm->last_n=b;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 pair right-rotate dual of 2ROT */
  if (kw(&L->cur,"2RROT")||kw(&L->cur,"DRROT")||kw(&L->cur,"RROT2")||
      kw(&L->cur,"STACK2RROT")||kw(&L->cur,"PAIRRROT")||kw(&L->cur,"2-ROT")){
    /* a b c d e f → e f a b c d  (rotate three pairs right) */
    lex_next(L);
    if (vm->sp < 6){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-6], b = vm->stack[vm->sp-5];
    long c = vm->stack[vm->sp-4], d = vm->stack[vm->sp-3];
    long e = vm->stack[vm->sp-2], f = vm->stack[vm->sp-1];
    vm->stack[vm->sp-6] = e; vm->stack[vm->sp-5] = f;
    vm->stack[vm->sp-4] = a; vm->stack[vm->sp-3] = b;
    vm->stack[vm->sp-2] = c; vm->stack[vm->sp-1] = d;
    var_set_num(vm,"LAST_N",d); vm->last_n=d;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"2NIP")||kw(&L->cur,"DNIP")||kw(&L->cur,"NIP2")){
    /* a b c d → a b d  (drop third under TOS) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[vm->sp-1];
    vm->stack[vm->sp-2] = d;
    vm->sp--;
    var_set_num(vm,"LAST_N",d); vm->last_n=d;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"ROLL")||kw(&L->cur,"STACKROLL")){
    /* ROLL n — rotate top (n+1) items: n-th under top becomes TOS
     * n=0 no-op; n=1 ≡ SWAP; n=2 ≡ ROT */
    lex_next(L);
    long n = 0;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"PRINT") && !kw(&L->cur,"END") && !kw(&L->cur,"CUBE")))
      n = parse_expr(vm,L);
    if (n < 0) n = 0;
    if (n == 0){
      var_set_num(vm,"OK", vm->sp > 0 ? 1 : 0);
      if (vm->sp > 0){ var_set_num(vm,"LAST_N",vm->stack[vm->sp-1]); vm->last_n=vm->stack[vm->sp-1]; }
      bump(vm); return 1;
    }
    if (vm->sp <= n){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    /* take item at depth n, shift others down, place on top */
    long v = vm->stack[vm->sp - 1 - (int)n];
    for (int i = (int)n; i > 0; i--)
      vm->stack[vm->sp - 1 - i] = vm->stack[vm->sp - i];
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 stack depth ext: 3DUP · 3DROP · 2TUCK · 3SWAP */
  if (kw(&L->cur,"3DUP")||kw(&L->cur,"TDUP")||kw(&L->cur,"DUP3")||
      kw(&L->cur,"STACK3DUP")){
    /* a b c → a b c a b c */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 3 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-3], b = vm->stack[vm->sp-2], c = vm->stack[vm->sp-1];
    vm->stack[vm->sp++] = a;
    vm->stack[vm->sp++] = b;
    vm->stack[vm->sp++] = c;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",c); vm->last_n=c;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"3DROP")||kw(&L->cur,"TDROP")||kw(&L->cur,"DROP3")||
      kw(&L->cur,"STACK3DROP")){
    /* a b c → (empty of top 3) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->sp -= 3;
    var_set_num(vm,"SP",vm->sp);
    if (vm->sp > 0){ var_set_num(vm,"LAST_N",vm->stack[vm->sp-1]); vm->last_n=vm->stack[vm->sp-1]; }
    else { var_set_num(vm,"LAST_N",0); vm->last_n=0; }
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"2TUCK")||kw(&L->cur,"DTUCK")||kw(&L->cur,"TUCK2")||
      kw(&L->cur,"STACK2TUCK")){
    /* a b c d → c d a b c d  (tuck top pair under second pair) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 2 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-4], b = vm->stack[vm->sp-3];
    long c = vm->stack[vm->sp-2], d = vm->stack[vm->sp-1];
    vm->stack[vm->sp-4] = c; vm->stack[vm->sp-3] = d;
    vm->stack[vm->sp-2] = a; vm->stack[vm->sp-1] = b;
    vm->stack[vm->sp++] = c;
    vm->stack[vm->sp++] = d;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",d); vm->last_n=d;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"3SWAP")||kw(&L->cur,"TSWAP")||kw(&L->cur,"SWAP3")||
      kw(&L->cur,"STACK3SWAP")){
    /* a b c → c b a  (reverse top 3) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-3], c = vm->stack[vm->sp-1];
    vm->stack[vm->sp-3] = c;
    vm->stack[vm->sp-1] = a;
    var_set_num(vm,"LAST_N",a); vm->last_n=a;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
    if (kw(&L->cur,"3OVER")||kw(&L->cur,"TOVER")||kw(&L->cur,"OVER3")||
      kw(&L->cur,"TRIPLEOVER")||kw(&L->cur,"STACK3OVER")){
    /* a b c d e f -> a b c d e f a b c */
    lex_next(L);
    if (vm->sp < 6){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 3 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long x = vm->stack[vm->sp-6], y = vm->stack[vm->sp-5], z = vm->stack[vm->sp-4];
    vm->stack[vm->sp++] = x;
    vm->stack[vm->sp++] = y;
    vm->stack[vm->sp++] = z;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",z); vm->last_n=z;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 triple depth duals: 3ROT · 3RROT · 3TUCK (complete after 3DUP/3SWAP) */
  if (kw(&L->cur,"3ROT")||kw(&L->cur,"TROT")||kw(&L->cur,"ROT3")||
      kw(&L->cur,"STACK3ROT")){
    /* a b c → b c a */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-3], b = vm->stack[vm->sp-2], c = vm->stack[vm->sp-1];
    vm->stack[vm->sp-3] = b;
    vm->stack[vm->sp-2] = c;
    vm->stack[vm->sp-1] = a;
    var_set_num(vm,"LAST_N",a); vm->last_n=a;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"3RROT")||kw(&L->cur,"TRROT")||kw(&L->cur,"RROT3")||
      kw(&L->cur,"STACK3RROT")||kw(&L->cur,"3-ROT")){
    /* a b c → c a b  (right rotate top 3) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-3], b = vm->stack[vm->sp-2], c = vm->stack[vm->sp-1];
    vm->stack[vm->sp-3] = c;
    vm->stack[vm->sp-2] = a;
    vm->stack[vm->sp-1] = b;
    var_set_num(vm,"LAST_N",b); vm->last_n=b;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"3TUCK")||kw(&L->cur,"TTUCK")||kw(&L->cur,"TUCK3")||
      kw(&L->cur,"STACK3TUCK")){
    /* a b c → c a b c  (copy TOS under top 2) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 1 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-3], b = vm->stack[vm->sp-2], c = vm->stack[vm->sp-1];
    vm->stack[vm->sp-3] = c;
    vm->stack[vm->sp-2] = a;
    vm->stack[vm->sp-1] = b;
    vm->stack[vm->sp++] = c;
    var_set_num(vm,"LAST_N",c); vm->last_n=c;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 depth duals: 3NIP · 4DUP · 4DROP · 4SWAP */
  if (kw(&L->cur,"3NIP")||kw(&L->cur,"TNIP")||kw(&L->cur,"NIP3")||
      kw(&L->cur,"STACK3NIP")){
    /* a b c → a c  (drop middle of top 3) */
    lex_next(L);
    if (vm->sp < 3){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-3], c = vm->stack[vm->sp-1];
    vm->stack[vm->sp-3] = a;
    vm->stack[vm->sp-2] = c;
    vm->sp--;
    var_set_num(vm,"LAST_N",c); vm->last_n=c;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"4DUP")||kw(&L->cur,"QDUP4")||kw(&L->cur,"DUP4")||
      kw(&L->cur,"STACK4DUP")){
    /* a b c d → a b c d a b c d */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 4 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-4], b = vm->stack[vm->sp-3];
    long c = vm->stack[vm->sp-2], d = vm->stack[vm->sp-1];
    vm->stack[vm->sp++] = a;
    vm->stack[vm->sp++] = b;
    vm->stack[vm->sp++] = c;
    vm->stack[vm->sp++] = d;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",d); vm->last_n=d;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"4DROP")||kw(&L->cur,"QDROP")||kw(&L->cur,"DROP4")||
      kw(&L->cur,"STACK4DROP")){
    /* drop top 4 */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->sp -= 4;
    var_set_num(vm,"SP",vm->sp);
    if (vm->sp > 0){ var_set_num(vm,"LAST_N",vm->stack[vm->sp-1]); vm->last_n=vm->stack[vm->sp-1]; }
    else { var_set_num(vm,"LAST_N",0); vm->last_n=0; }
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"4SWAP")||kw(&L->cur,"QSWAP")||kw(&L->cur,"SWAP4")||
      kw(&L->cur,"STACK4SWAP")){
    /* a b c d → d c b a  (reverse top 4) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-4], b = vm->stack[vm->sp-3];
    long c = vm->stack[vm->sp-2], d = vm->stack[vm->sp-1];
    vm->stack[vm->sp-4] = d;
    vm->stack[vm->sp-3] = c;
    vm->stack[vm->sp-2] = b;
    vm->stack[vm->sp-1] = a;
    var_set_num(vm,"LAST_N",a); vm->last_n=a;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack foundation: 4NIP · 4ROT · 4RROT · 4OVER (complete depth-4 plane) */
  if (kw(&L->cur,"4NIP")||kw(&L->cur,"QNIP")||kw(&L->cur,"NIP4")||
      kw(&L->cur,"STACK4NIP")){
    /* a b c d → a d  (drop middle two of top 4; dual of 3NIP) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-4], d = vm->stack[vm->sp-1];
    vm->stack[vm->sp-4] = a;
    vm->stack[vm->sp-3] = d;
    vm->sp -= 2;
    var_set_num(vm,"LAST_N",d); vm->last_n=d;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"4ROT")||kw(&L->cur,"QROT")||kw(&L->cur,"ROT4")||
      kw(&L->cur,"STACK4ROT")){
    /* a b c d → b c d a */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-4], b = vm->stack[vm->sp-3];
    long c = vm->stack[vm->sp-2], d = vm->stack[vm->sp-1];
    vm->stack[vm->sp-4] = b;
    vm->stack[vm->sp-3] = c;
    vm->stack[vm->sp-2] = d;
    vm->stack[vm->sp-1] = a;
    var_set_num(vm,"LAST_N",a); vm->last_n=a;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"4RROT")||kw(&L->cur,"QRROT")||kw(&L->cur,"RROT4")||
      kw(&L->cur,"STACK4RROT")||kw(&L->cur,"4-ROT")){
    /* a b c d → d a b c  (right rotate top 4) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-4], b = vm->stack[vm->sp-3];
    long c = vm->stack[vm->sp-2], d = vm->stack[vm->sp-1];
    vm->stack[vm->sp-4] = d;
    vm->stack[vm->sp-3] = a;
    vm->stack[vm->sp-2] = b;
    vm->stack[vm->sp-1] = c;
    var_set_num(vm,"LAST_N",c); vm->last_n=c;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"4OVER")||kw(&L->cur,"QOVER")||kw(&L->cur,"OVER4")||
      kw(&L->cur,"STACK4OVER")){
    /* a b c d e f g h → a b c d e f g h a b c d  (copy third-under quartet) */
    lex_next(L);
    if (vm->sp < 8){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 4 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-8], b = vm->stack[vm->sp-7];
    long c = vm->stack[vm->sp-6], d = vm->stack[vm->sp-5];
    vm->stack[vm->sp++] = a;
    vm->stack[vm->sp++] = b;
    vm->stack[vm->sp++] = c;
    vm->stack[vm->sp++] = d;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",d); vm->last_n=d;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 depth duals: 4TUCK · 5DUP · 5DROP · 5SWAP */
  if (kw(&L->cur,"4TUCK")||kw(&L->cur,"QTUCK")||kw(&L->cur,"TUCK4")||
      kw(&L->cur,"STACK4TUCK")){
    /* a b c d → d a b c d  (copy TOS under top 3; dual of 3TUCK) */
    lex_next(L);
    if (vm->sp < 4){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 1 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-4], b = vm->stack[vm->sp-3];
    long c = vm->stack[vm->sp-2], d = vm->stack[vm->sp-1];
    vm->stack[vm->sp-4] = d;
    vm->stack[vm->sp-3] = a;
    vm->stack[vm->sp-2] = b;
    vm->stack[vm->sp-1] = c;
    vm->stack[vm->sp++] = d;
    var_set_num(vm,"LAST_N",d); vm->last_n=d;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"5DUP")||kw(&L->cur,"PDUP")||kw(&L->cur,"DUP5")||
      kw(&L->cur,"STACK5DUP")){
    /* a b c d e → a b c d e a b c d e */
    lex_next(L);
    if (vm->sp < 5){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 5 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-5], b = vm->stack[vm->sp-4];
    long c = vm->stack[vm->sp-3], d = vm->stack[vm->sp-2];
    long e = vm->stack[vm->sp-1];
    vm->stack[vm->sp++] = a;
    vm->stack[vm->sp++] = b;
    vm->stack[vm->sp++] = c;
    vm->stack[vm->sp++] = d;
    vm->stack[vm->sp++] = e;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",e); vm->last_n=e;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"5DROP")||kw(&L->cur,"PDROP")||kw(&L->cur,"DROP5")||
      kw(&L->cur,"STACK5DROP")){
    /* drop top 5 */
    lex_next(L);
    if (vm->sp < 5){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->sp -= 5;
    var_set_num(vm,"SP",vm->sp);
    if (vm->sp > 0){ var_set_num(vm,"LAST_N",vm->stack[vm->sp-1]); vm->last_n=vm->stack[vm->sp-1]; }
    else { var_set_num(vm,"LAST_N",0); vm->last_n=0; }
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"5SWAP")||kw(&L->cur,"PSWAP")||kw(&L->cur,"SWAP5")||
      kw(&L->cur,"STACK5SWAP")){
    /* a b c d e → e d c b a  (reverse top 5) */
    lex_next(L);
    if (vm->sp < 5){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-5], b = vm->stack[vm->sp-4];
    long c = vm->stack[vm->sp-3], d = vm->stack[vm->sp-2];
    long e = vm->stack[vm->sp-1];
    vm->stack[vm->sp-5] = e;
    vm->stack[vm->sp-4] = d;
    vm->stack[vm->sp-3] = c;
    vm->stack[vm->sp-2] = b;
    vm->stack[vm->sp-1] = a;
    var_set_num(vm,"LAST_N",a); vm->last_n=a;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack foundation: 5NIP · 5ROT · 5RROT · 5OVER · 5TUCK (complete depth-5) */
  if (kw(&L->cur,"5NIP")||kw(&L->cur,"PNIP")||kw(&L->cur,"NIP5")||
      kw(&L->cur,"STACK5NIP")){
    /* a b c d e → a e  (drop middle three of top 5) */
    lex_next(L);
    if (vm->sp < 5){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-5], e = vm->stack[vm->sp-1];
    vm->stack[vm->sp-5] = a;
    vm->stack[vm->sp-4] = e;
    vm->sp -= 3;
    var_set_num(vm,"LAST_N",e); vm->last_n=e;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"5ROT")||kw(&L->cur,"PROT")||kw(&L->cur,"ROT5")||
      kw(&L->cur,"STACK5ROT")){
    /* a b c d e → b c d e a */
    lex_next(L);
    if (vm->sp < 5){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-5], b = vm->stack[vm->sp-4];
    long c = vm->stack[vm->sp-3], d = vm->stack[vm->sp-2];
    long e = vm->stack[vm->sp-1];
    vm->stack[vm->sp-5] = b;
    vm->stack[vm->sp-4] = c;
    vm->stack[vm->sp-3] = d;
    vm->stack[vm->sp-2] = e;
    vm->stack[vm->sp-1] = a;
    var_set_num(vm,"LAST_N",a); vm->last_n=a;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"5RROT")||kw(&L->cur,"PRROT")||kw(&L->cur,"RROT5")||
      kw(&L->cur,"STACK5RROT")||kw(&L->cur,"5-ROT")){
    /* a b c d e → e a b c d */
    lex_next(L);
    if (vm->sp < 5){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-5], b = vm->stack[vm->sp-4];
    long c = vm->stack[vm->sp-3], d = vm->stack[vm->sp-2];
    long e = vm->stack[vm->sp-1];
    vm->stack[vm->sp-5] = e;
    vm->stack[vm->sp-4] = a;
    vm->stack[vm->sp-3] = b;
    vm->stack[vm->sp-2] = c;
    vm->stack[vm->sp-1] = d;
    var_set_num(vm,"LAST_N",d); vm->last_n=d;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"5OVER")||kw(&L->cur,"POVER")||kw(&L->cur,"OVER5")||
      kw(&L->cur,"STACK5OVER")){
    /* 10-deep: copy under quintet onto stack */
    lex_next(L);
    if (vm->sp < 10){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 5 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-10], b = vm->stack[vm->sp-9];
    long c = vm->stack[vm->sp-8], d = vm->stack[vm->sp-7];
    long e = vm->stack[vm->sp-6];
    vm->stack[vm->sp++] = a;
    vm->stack[vm->sp++] = b;
    vm->stack[vm->sp++] = c;
    vm->stack[vm->sp++] = d;
    vm->stack[vm->sp++] = e;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",e); vm->last_n=e;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"5TUCK")||kw(&L->cur,"PTUCK")||kw(&L->cur,"TUCK5")||
      kw(&L->cur,"STACK5TUCK")){
    /* a b c d e → e a b c d e  (copy TOS under top 4) */
    lex_next(L);
    if (vm->sp < 5){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 1 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-5], b = vm->stack[vm->sp-4];
    long c = vm->stack[vm->sp-3], d = vm->stack[vm->sp-2];
    long e = vm->stack[vm->sp-1];
    vm->stack[vm->sp-5] = e;
    vm->stack[vm->sp-4] = a;
    vm->stack[vm->sp-3] = b;
    vm->stack[vm->sp-2] = c;
    vm->stack[vm->sp-1] = d;
    vm->stack[vm->sp++] = e;
    var_set_num(vm,"LAST_N",e); vm->last_n=e;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 depth-6 plane: 6DUP · 6DROP */
  if (kw(&L->cur,"6DUP")||kw(&L->cur,"HDUP")||kw(&L->cur,"DUP6")||
      kw(&L->cur,"STACK6DUP")){
    /* a b c d e f → a b c d e f a b c d e f */
    lex_next(L);
    if (vm->sp < 6){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 6 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-6], b = vm->stack[vm->sp-5];
    long c = vm->stack[vm->sp-4], d = vm->stack[vm->sp-3];
    long e = vm->stack[vm->sp-2], f = vm->stack[vm->sp-1];
    vm->stack[vm->sp++] = a;
    vm->stack[vm->sp++] = b;
    vm->stack[vm->sp++] = c;
    vm->stack[vm->sp++] = d;
    vm->stack[vm->sp++] = e;
    vm->stack[vm->sp++] = f;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",f); vm->last_n=f;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"6DROP")||kw(&L->cur,"HDROP")||kw(&L->cur,"DROP6")||
      kw(&L->cur,"STACK6DROP")){
    /* drop top 6 */
    lex_next(L);
    if (vm->sp < 6){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->sp -= 6;
    var_set_num(vm,"SP",vm->sp);
    if (vm->sp > 0){ var_set_num(vm,"LAST_N",vm->stack[vm->sp-1]); vm->last_n=vm->stack[vm->sp-1]; }
    else { var_set_num(vm,"LAST_N",0); vm->last_n=0; }
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 depth-6 ext: 6SWAP · 6NIP (complete after 6DUP/6DROP) */
  if (kw(&L->cur,"6SWAP")||kw(&L->cur,"HSWAP")||kw(&L->cur,"SWAP6")||
      kw(&L->cur,"STACK6SWAP")){
    /* a b c d e f → f e d c b a  (reverse top 6) */
    lex_next(L);
    if (vm->sp < 6){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-6], b = vm->stack[vm->sp-5];
    long c = vm->stack[vm->sp-4], d = vm->stack[vm->sp-3];
    long e = vm->stack[vm->sp-2], f = vm->stack[vm->sp-1];
    vm->stack[vm->sp-6] = f;
    vm->stack[vm->sp-5] = e;
    vm->stack[vm->sp-4] = d;
    vm->stack[vm->sp-3] = c;
    vm->stack[vm->sp-2] = b;
    vm->stack[vm->sp-1] = a;
    var_set_num(vm,"LAST_N",a); vm->last_n=a;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"6NIP")||kw(&L->cur,"HNIP")||kw(&L->cur,"NIP6")||
      kw(&L->cur,"STACK6NIP")){
    /* a b c d e f → a f  (keep ends of top 6; dual of 4NIP) */
    lex_next(L);
    if (vm->sp < 6){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-6], f = vm->stack[vm->sp-1];
    vm->stack[vm->sp-6] = a;
    vm->stack[vm->sp-5] = f;
    vm->sp -= 4;
    var_set_num(vm,"LAST_N",f); vm->last_n=f;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 depth-6 rotate/over: 6ROT · 6RROT · 6OVER (parity with 5-plane) */
  if (kw(&L->cur,"6ROT")||kw(&L->cur,"HROT")||kw(&L->cur,"ROT6")||
      kw(&L->cur,"STACK6ROT")){
    /* a b c d e f → b c d e f a */
    lex_next(L);
    if (vm->sp < 6){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-6], b = vm->stack[vm->sp-5];
    long c = vm->stack[vm->sp-4], d = vm->stack[vm->sp-3];
    long e = vm->stack[vm->sp-2], f = vm->stack[vm->sp-1];
    vm->stack[vm->sp-6] = b;
    vm->stack[vm->sp-5] = c;
    vm->stack[vm->sp-4] = d;
    vm->stack[vm->sp-3] = e;
    vm->stack[vm->sp-2] = f;
    vm->stack[vm->sp-1] = a;
    var_set_num(vm,"LAST_N",a); vm->last_n=a;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"6RROT")||kw(&L->cur,"HRROT")||kw(&L->cur,"RROT6")||
      kw(&L->cur,"STACK6RROT")||kw(&L->cur,"6-ROT")){
    /* a b c d e f → f a b c d e */
    lex_next(L);
    if (vm->sp < 6){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-6], b = vm->stack[vm->sp-5];
    long c = vm->stack[vm->sp-4], d = vm->stack[vm->sp-3];
    long e = vm->stack[vm->sp-2], f = vm->stack[vm->sp-1];
    vm->stack[vm->sp-6] = f;
    vm->stack[vm->sp-5] = a;
    vm->stack[vm->sp-4] = b;
    vm->stack[vm->sp-3] = c;
    vm->stack[vm->sp-2] = d;
    vm->stack[vm->sp-1] = e;
    var_set_num(vm,"LAST_N",e); vm->last_n=e;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"6OVER")||kw(&L->cur,"HOVER")||kw(&L->cur,"OVER6")||
      kw(&L->cur,"STACK6OVER")){
    /* 12-deep: copy under sextet onto stack */
    lex_next(L);
    if (vm->sp < 12){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 6 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-12], b = vm->stack[vm->sp-11];
    long c = vm->stack[vm->sp-10], d = vm->stack[vm->sp-9];
    long e = vm->stack[vm->sp-8], f = vm->stack[vm->sp-7];
    vm->stack[vm->sp++] = a;
    vm->stack[vm->sp++] = b;
    vm->stack[vm->sp++] = c;
    vm->stack[vm->sp++] = d;
    vm->stack[vm->sp++] = e;
    vm->stack[vm->sp++] = f;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",f); vm->last_n=f;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 depth-6 tuck: 6TUCK (complete 5/6 plane after 6OVER) */
  if (kw(&L->cur,"6TUCK")||kw(&L->cur,"HTUCK")||kw(&L->cur,"TUCK6")||
      kw(&L->cur,"STACK6TUCK")){
    /* a b c d e f → f a b c d e f  (copy TOS under top 5) */
    lex_next(L);
    if (vm->sp < 6){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 1 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp-6], b = vm->stack[vm->sp-5];
    long c = vm->stack[vm->sp-4], d = vm->stack[vm->sp-3];
    long e = vm->stack[vm->sp-2], f = vm->stack[vm->sp-1];
    vm->stack[vm->sp-6] = f;
    vm->stack[vm->sp-5] = a;
    vm->stack[vm->sp-4] = b;
    vm->stack[vm->sp-3] = c;
    vm->stack[vm->sp-2] = d;
    vm->stack[vm->sp-1] = e;
    vm->stack[vm->sp++] = f;
    var_set_num(vm,"LAST_N",f); vm->last_n=f;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 depth-7 foundation: 7DUP · 7DROP · 7SWAP (extend after depth-6 plane) */
  if (kw(&L->cur,"7DUP")||kw(&L->cur,"SEPDUP")||kw(&L->cur,"DUP7")||
      kw(&L->cur,"STACK7DUP")){
    /* a..g (7) → a..g a..g */
    lex_next(L);
    if (vm->sp < 7){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 7 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v[7];
    for (int i = 0; i < 7; i++) v[i] = vm->stack[vm->sp - 7 + i];
    for (int i = 0; i < 7; i++) vm->stack[vm->sp++] = v[i];
    long last = v[6];
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"7DROP")||kw(&L->cur,"SEPDROP")||kw(&L->cur,"DROP7")||
      kw(&L->cur,"STACK7DROP")){
    /* drop top 7 */
    lex_next(L);
    if (vm->sp < 7){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->sp -= 7;
    var_set_num(vm,"SP",vm->sp);
    if (vm->sp > 0){ var_set_num(vm,"LAST_N",vm->stack[vm->sp-1]); vm->last_n=vm->stack[vm->sp-1]; }
    else { var_set_num(vm,"LAST_N",0); vm->last_n=0; }
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"7SWAP")||kw(&L->cur,"SEPSWAP")||kw(&L->cur,"SWAP7")||
      kw(&L->cur,"STACK7SWAP")){
    /* reverse top 7 */
    lex_next(L);
    if (vm->sp < 7){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v[7];
    for (int i = 0; i < 7; i++) v[i] = vm->stack[vm->sp - 7 + i];
    for (int i = 0; i < 7; i++) vm->stack[vm->sp - 7 + i] = v[6 - i];
    long last = vm->stack[vm->sp - 1];
    var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 depth-7 ext: 7NIP · 7ROT · 7RROT (parity with 6NIP/6ROT plane) */
  if (kw(&L->cur,"7NIP")||kw(&L->cur,"SEPNIP")||kw(&L->cur,"NIP7")||
      kw(&L->cur,"STACK7NIP")){
    /* a b c d e f g → a g  (keep ends of top 7) */
    lex_next(L);
    if (vm->sp < 7){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 7], g = vm->stack[vm->sp - 1];
    vm->stack[vm->sp - 7] = a;
    vm->stack[vm->sp - 6] = g;
    vm->sp -= 5;
    var_set_num(vm,"LAST_N",g); vm->last_n=g;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"7ROT")||kw(&L->cur,"SEPROT")||kw(&L->cur,"ROT7")||
      kw(&L->cur,"STACK7ROT")){
    /* a b c d e f g → b c d e f g a */
    lex_next(L);
    if (vm->sp < 7){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v[7];
    for (int i = 0; i < 7; i++) v[i] = vm->stack[vm->sp - 7 + i];
    for (int i = 0; i < 6; i++) vm->stack[vm->sp - 7 + i] = v[i + 1];
    vm->stack[vm->sp - 1] = v[0];
    var_set_num(vm,"LAST_N",v[0]); vm->last_n=v[0];
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"7RROT")||kw(&L->cur,"SEPRROT")||kw(&L->cur,"RROT7")||
      kw(&L->cur,"STACK7RROT")||kw(&L->cur,"7-ROT")){
    /* a b c d e f g → g a b c d e f */
    lex_next(L);
    if (vm->sp < 7){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v[7];
    for (int i = 0; i < 7; i++) v[i] = vm->stack[vm->sp - 7 + i];
    vm->stack[vm->sp - 7] = v[6];
    for (int i = 0; i < 6; i++) vm->stack[vm->sp - 6 + i] = v[i];
    long last = vm->stack[vm->sp - 1];
    var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 depth-8 foundation: 8DUP · 8DROP · 8SWAP (extend after complete depth-7 plane) */
  if (kw(&L->cur,"8DUP")||kw(&L->cur,"OCTDUP")||kw(&L->cur,"DUP8")||
      kw(&L->cur,"STACK8DUP")){
    /* a..h (8) → a..h a..h */
    lex_next(L);
    if (vm->sp < 8){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 8 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v[8];
    for (int i = 0; i < 8; i++) v[i] = vm->stack[vm->sp - 8 + i];
    for (int i = 0; i < 8; i++) vm->stack[vm->sp++] = v[i];
    long last = v[7];
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"8DROP")||kw(&L->cur,"OCTDROP")||kw(&L->cur,"DROP8")||
      kw(&L->cur,"STACK8DROP")){
    /* drop top 8 */
    lex_next(L);
    if (vm->sp < 8){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->sp -= 8;
    var_set_num(vm,"SP",vm->sp);
    if (vm->sp > 0){ var_set_num(vm,"LAST_N",vm->stack[vm->sp-1]); vm->last_n=vm->stack[vm->sp-1]; }
    else { var_set_num(vm,"LAST_N",0); vm->last_n=0; }
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"8SWAP")||kw(&L->cur,"OCTSWAP")||kw(&L->cur,"SWAP8")||
      kw(&L->cur,"STACK8SWAP")){
    /* reverse top 8 */
    lex_next(L);
    if (vm->sp < 8){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v[8];
    for (int i = 0; i < 8; i++) v[i] = vm->stack[vm->sp - 8 + i];
    for (int i = 0; i < 8; i++) vm->stack[vm->sp - 8 + i] = v[7 - i];
    long last = vm->stack[vm->sp - 1];
    var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 depth-8 combinator ext: 8NIP · 8ROT · 8RROT (parity with 7NIP/7ROT plane) */
  if (kw(&L->cur,"8NIP")||kw(&L->cur,"OCTNIP")||kw(&L->cur,"NIP8")||
      kw(&L->cur,"STACK8NIP")){
    /* a b c d e f g h → a h  (keep ends of top 8) */
    lex_next(L);
    if (vm->sp < 8){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 8], h = vm->stack[vm->sp - 1];
    vm->stack[vm->sp - 8] = a;
    vm->stack[vm->sp - 7] = h;
    vm->sp -= 6;
    var_set_num(vm,"LAST_N",h); vm->last_n=h;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"8ROT")||kw(&L->cur,"OCTROT")||kw(&L->cur,"ROT8")||
      kw(&L->cur,"STACK8ROT")){
    /* a b c d e f g h → b c d e f g h a */
    lex_next(L);
    if (vm->sp < 8){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v[8];
    for (int i = 0; i < 8; i++) v[i] = vm->stack[vm->sp - 8 + i];
    for (int i = 0; i < 7; i++) vm->stack[vm->sp - 8 + i] = v[i + 1];
    vm->stack[vm->sp - 1] = v[0];
    var_set_num(vm,"LAST_N",v[0]); vm->last_n=v[0];
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"8RROT")||kw(&L->cur,"OCTRROT")||kw(&L->cur,"RROT8")||
      kw(&L->cur,"STACK8RROT")||kw(&L->cur,"8-ROT")){
    /* a b c d e f g h → h a b c d e f g */
    lex_next(L);
    if (vm->sp < 8){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v[8];
    for (int i = 0; i < 8; i++) v[i] = vm->stack[vm->sp - 8 + i];
    vm->stack[vm->sp - 8] = v[7];
    for (int i = 0; i < 7; i++) vm->stack[vm->sp - 7 + i] = v[i];
    long last = vm->stack[vm->sp - 1];
    var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 depth-8 over/tuck: 8OVER · 8TUCK (complete depth-8 plane after 8NIP/8ROT) */
  if (kw(&L->cur,"8OVER")||kw(&L->cur,"OCTOVER")||kw(&L->cur,"OVER8")||
      kw(&L->cur,"STACK8OVER")){
    /* 16-deep: copy under octet onto stack */
    lex_next(L);
    if (vm->sp < 16){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 8 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v[8];
    for (int i = 0; i < 8; i++) v[i] = vm->stack[vm->sp - 16 + i];
    for (int i = 0; i < 8; i++) vm->stack[vm->sp++] = v[i];
    long last = v[7];
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"8TUCK")||kw(&L->cur,"OCTTUCK")||kw(&L->cur,"TUCK8")||
      kw(&L->cur,"STACK8TUCK")){
    /* a..h → h a..g h  (copy TOS under top 7) */
    lex_next(L);
    if (vm->sp < 8){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 1 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v[8];
    for (int i = 0; i < 8; i++) v[i] = vm->stack[vm->sp - 8 + i];
    vm->stack[vm->sp - 8] = v[7];
    for (int i = 0; i < 7; i++) vm->stack[vm->sp - 7 + i] = v[i];
    vm->stack[vm->sp++] = v[7];
    var_set_num(vm,"LAST_N",v[7]); vm->last_n=v[7];
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-8 depth-9 foundation: 9DUP · 9DROP · 9SWAP (extend after complete depth-8 plane) */
  if (kw(&L->cur,"9DUP")||kw(&L->cur,"NONADUP")||kw(&L->cur,"DUP9")||
      kw(&L->cur,"STACK9DUP")){
    /* a..i (9) → a..i a..i */
    lex_next(L);
    if (vm->sp < 9){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 9 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v[9];
    for (int i = 0; i < 9; i++) v[i] = vm->stack[vm->sp - 9 + i];
    for (int i = 0; i < 9; i++) vm->stack[vm->sp++] = v[i];
    long last = v[8];
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"9DROP")||kw(&L->cur,"NONADROP")||kw(&L->cur,"DROP9")||
      kw(&L->cur,"STACK9DROP")){
    /* drop top 9 */
    lex_next(L);
    if (vm->sp < 9){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->sp -= 9;
    var_set_num(vm,"SP",vm->sp);
    if (vm->sp > 0){ var_set_num(vm,"LAST_N",vm->stack[vm->sp-1]); vm->last_n=vm->stack[vm->sp-1]; }
    else { var_set_num(vm,"LAST_N",0); vm->last_n=0; }
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"9SWAP")||kw(&L->cur,"NONASWAP")||kw(&L->cur,"SWAP9")||
      kw(&L->cur,"STACK9SWAP")){
    /* reverse top 9 */
    lex_next(L);
    if (vm->sp < 9){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v[9];
    for (int i = 0; i < 9; i++) v[i] = vm->stack[vm->sp - 9 + i];
    for (int i = 0; i < 9; i++) vm->stack[vm->sp - 9 + i] = v[8 - i];
    long last = vm->stack[vm->sp - 1];
    var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 depth-9 combinator ext: 9NIP · 9ROT · 9RROT (parity with 8NIP/8ROT plane) */
  if (kw(&L->cur,"9NIP")||kw(&L->cur,"NONANIP")||kw(&L->cur,"NIP9")||
      kw(&L->cur,"STACK9NIP")){
    /* a b c d e f g h i → a i  (keep ends of top 9) */
    lex_next(L);
    if (vm->sp < 9){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 9], i9 = vm->stack[vm->sp - 1];
    vm->stack[vm->sp - 9] = a;
    vm->stack[vm->sp - 8] = i9;
    vm->sp -= 7;
    var_set_num(vm,"LAST_N",i9); vm->last_n=i9;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"9ROT")||kw(&L->cur,"NONAROT")||kw(&L->cur,"ROT9")||
      kw(&L->cur,"STACK9ROT")){
    /* a..i → b..i a */
    lex_next(L);
    if (vm->sp < 9){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v[9];
    for (int i = 0; i < 9; i++) v[i] = vm->stack[vm->sp - 9 + i];
    for (int i = 0; i < 8; i++) vm->stack[vm->sp - 9 + i] = v[i + 1];
    vm->stack[vm->sp - 1] = v[0];
    var_set_num(vm,"LAST_N",v[0]); vm->last_n=v[0];
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"9RROT")||kw(&L->cur,"NONARROT")||kw(&L->cur,"RROT9")||
      kw(&L->cur,"STACK9RROT")||kw(&L->cur,"9-ROT")){
    /* a..i → i a..h */
    lex_next(L);
    if (vm->sp < 9){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v[9];
    for (int i = 0; i < 9; i++) v[i] = vm->stack[vm->sp - 9 + i];
    vm->stack[vm->sp - 9] = v[8];
    for (int i = 0; i < 8; i++) vm->stack[vm->sp - 8 + i] = v[i];
    long last = vm->stack[vm->sp - 1];
    var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 depth-9 over/tuck: 9OVER · 9TUCK (complete depth-9 plane after 9NIP/9ROT) */
  if (kw(&L->cur,"9OVER")||kw(&L->cur,"NONAOVER")||kw(&L->cur,"OVER9")||
      kw(&L->cur,"STACK9OVER")){
    /* 18-deep: copy under nonet onto stack */
    lex_next(L);
    if (vm->sp < 18){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 9 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v[9];
    for (int i = 0; i < 9; i++) v[i] = vm->stack[vm->sp - 18 + i];
    for (int i = 0; i < 9; i++) vm->stack[vm->sp++] = v[i];
    long last = v[8];
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"9TUCK")||kw(&L->cur,"NONATUCK")||kw(&L->cur,"TUCK9")||
      kw(&L->cur,"STACK9TUCK")){
    /* a..i → i a..h i  (copy TOS under top 8) */
    lex_next(L);
    if (vm->sp < 9){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 1 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v[9];
    for (int i = 0; i < 9; i++) v[i] = vm->stack[vm->sp - 9 + i];
    vm->stack[vm->sp - 9] = v[8];
    for (int i = 0; i < 8; i++) vm->stack[vm->sp - 8 + i] = v[i];
    vm->stack[vm->sp++] = v[8];
    var_set_num(vm,"LAST_N",v[8]); vm->last_n=v[8];
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack foundation: 7OVER · 7TUCK (complete depth-7 plane after 7NIP/7ROT) */
  if (kw(&L->cur,"7OVER")||kw(&L->cur,"SEPOVER")||kw(&L->cur,"OVER7")||
      kw(&L->cur,"STACK7OVER")){
    /* 14-deep: copy under septet onto stack */
    lex_next(L);
    if (vm->sp < 14){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 7 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v[7];
    for (int i = 0; i < 7; i++) v[i] = vm->stack[vm->sp - 14 + i];
    for (int i = 0; i < 7; i++) vm->stack[vm->sp++] = v[i];
    long last = v[6];
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",last); vm->last_n=last;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"7TUCK")||kw(&L->cur,"SEPTUCK")||kw(&L->cur,"TUCK7")||
      kw(&L->cur,"STACK7TUCK")){
    /* a b c d e f g → g a b c d e f g  (copy TOS under top 6) */
    lex_next(L);
    if (vm->sp < 7){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 1 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v[7];
    for (int i = 0; i < 7; i++) v[i] = vm->stack[vm->sp - 7 + i];
    vm->stack[vm->sp - 7] = v[6];
    for (int i = 0; i < 6; i++) vm->stack[vm->sp - 6 + i] = v[i];
    vm->stack[vm->sp++] = v[6];
    var_set_num(vm,"LAST_N",v[6]); vm->last_n=v[6];
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"UNDER")||kw(&L->cur,"SUNDER")||kw(&L->cur,"DUPUNDER")||
      kw(&L->cur,"STACKUNDER")){
    /* a b -> a a b */
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (vm->sp + 1 > CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long under = vm->stack[vm->sp-2];
    long top = vm->stack[vm->sp-1];
    vm->stack[vm->sp-1] = under;
    vm->stack[vm->sp++] = top;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",top); vm->last_n=top;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
if (kw(&L->cur,"DEPTH")||kw(&L->cur,"STACKDEPTH")){
    /* DEPTH — push current stack depth */
    lex_next(L);
    if (vm->sp >= CUBALC_STACK_N){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = (long)vm->sp;
    vm->stack[vm->sp++] = d;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",d); vm->last_n=d;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack ALU: binary ops a b → r ; unary NEG/ABS on TOS */
  if (kw(&L->cur,"ADD")||kw(&L->cur,"SUB")||kw(&L->cur,"MUL")||
      kw(&L->cur,"DIV")||kw(&L->cur,"MOD")||
      kw(&L->cur,"STACKADD")||kw(&L->cur,"STACKSUB")||kw(&L->cur,"STACKMUL")||
      kw(&L->cur,"STACKDIV")||kw(&L->cur,"STACKMOD")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    long r = 0;
    if (strcmp(op,"ADD")==0 || strcmp(op,"STACKADD")==0) r = a + b;
    else if (strcmp(op,"SUB")==0 || strcmp(op,"STACKSUB")==0) r = a - b;
    else if (strcmp(op,"MUL")==0 || strcmp(op,"STACKMUL")==0) r = a * b;
    else if (strcmp(op,"DIV")==0 || strcmp(op,"STACKDIV")==0) r = b ? (a / b) : 0;
    else if (strcmp(op,"MOD")==0 || strcmp(op,"STACKMOD")==0) r = b ? (a % b) : 0;
    vm->stack[vm->sp++] = r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNEG")||kw(&L->cur,"STACKNEG")||kw(&L->cur,"NEGATE")){
    /* unary negate TOS (NEG alone is expr form elsewhere) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp - 1] = -vm->stack[vm->sp - 1];
    var_set_num(vm,"LAST_N",vm->stack[vm->sp-1]); vm->last_n=vm->stack[vm->sp-1];
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SABS")||kw(&L->cur,"STACKABS")){
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = vm->stack[vm->sp - 1];
    if (v < 0) v = -v;
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack ALU unary: SINC · SDEC · SDBL · SHALF */
  if (kw(&L->cur,"SINC")||kw(&L->cur,"INCSTK")||
      kw(&L->cur,"STACKINC")||kw(&L->cur,"INCS")){
    /* SINC — TOS += 1 */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp - 1] += 1;
    long v = vm->stack[vm->sp - 1];
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SDEC")||kw(&L->cur,"DECSTK")||
      kw(&L->cur,"STACKDEC")||kw(&L->cur,"DECS")){
    /* SDEC — TOS -= 1 */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp - 1] -= 1;
    long v = vm->stack[vm->sp - 1];
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SDBL")||kw(&L->cur,"SDOUBLE")||
      kw(&L->cur,"STACKDBL")||kw(&L->cur,"DOUBLES")){
    /* SDBL — TOS *= 2 */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp - 1] *= 2;
    long v = vm->stack[vm->sp - 1];
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SHALF")||kw(&L->cur,"SHALVE")||
      kw(&L->cur,"STACKHALF")||kw(&L->cur,"HALFS")){
    /* SHALF — TOS /= 2 (toward zero) */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp - 1] /= 2;
    long v = vm->stack[vm->sp - 1];
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-4 stack immediate ALU: SADDN · SSUBN · SMULN */
  if (kw(&L->cur,"SADDN")||kw(&L->cur,"PLUSN")||kw(&L->cur,"ADDN")||
      kw(&L->cur,"STACKADDN")||kw(&L->cur,"SADDIMM")){
    /* SADDN n — TOS += n */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp - 1] += n;
    long v = vm->stack[vm->sp - 1];
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SSUBN")||kw(&L->cur,"MINUSN")||kw(&L->cur,"SUBN")||
      kw(&L->cur,"STACKSUBN")||kw(&L->cur,"SSUBIMM")){
    /* SSUBN n — TOS -= n */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp - 1] -= n;
    long v = vm->stack[vm->sp - 1];
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMULN")||kw(&L->cur,"TIMESN")||kw(&L->cur,"MULN")||
      kw(&L->cur,"STACKMULN")||kw(&L->cur,"SMULIMM")){
    /* SMULN n — TOS *= n */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    vm->stack[vm->sp - 1] *= n;
    long v = vm->stack[vm->sp - 1];
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack immediate ALU: SDIVN · SMODN (complete + − * / % by constant) */
  if (kw(&L->cur,"SDIVN")||kw(&L->cur,"DIVN")||kw(&L->cur,"QUOTN")||
      kw(&L->cur,"STACKDIVN")||kw(&L->cur,"SDIVIMM")){
    /* SDIVN n — TOS /= n (n==0 → 0, soft) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n == 0) vm->stack[vm->sp - 1] = 0;
    else vm->stack[vm->sp - 1] /= n;
    long v = vm->stack[vm->sp - 1];
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMODN")||kw(&L->cur,"MODN")||kw(&L->cur,"REMN")||
      kw(&L->cur,"STACKMODN")||kw(&L->cur,"SMODIMM")){
    /* SMODN n — TOS %= n (n==0 → 0, soft) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    if (n == 0) vm->stack[vm->sp - 1] = 0;
    else vm->stack[vm->sp - 1] %= n;
    long v = vm->stack[vm->sp - 1];
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 stack imm ceil/floor div: SDIVCEILN · SDIVFLOORN (imm dual of SDIVCEIL/SDIVFLOOR) */
  if (kw(&L->cur,"SDIVCEILN")||kw(&L->cur,"SCEILDIVN")||kw(&L->cur,"CEILDIVN")||
      kw(&L->cur,"STACKDIVCEILN")||kw(&L->cur,"SDIVCEILIMM")||kw(&L->cur,"CEILN")){
    /* SDIVCEILN n — TOS = ceil(TOS/n); n==0 → 0 soft (match SDIVCEIL) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (n == 0) r = 0;
    else if (a >= 0 && n > 0) r = (a + n - 1) / n;
    else if (a <= 0 && n < 0){
      long aa = -a, nn = -n;
      r = (aa + nn - 1) / nn;
    } else r = a / n;
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SDIVFLOORN")||kw(&L->cur,"SFLOORDIVN")||kw(&L->cur,"FLOORDIVN")||
      kw(&L->cur,"STACKDIVFLOORN")||kw(&L->cur,"SDIVFLOORIMM")||kw(&L->cur,"FLOORN")){
    /* SDIVFLOORN n — TOS = floor(TOS/n); n==0 → 0 soft (match SDIVFLOOR) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long r = 0;
    if (n == 0) r = 0;
    else {
      long q = a / n, rem = a % n;
      if (rem != 0 && ((a < 0) != (n < 0))) q--;
      r = q;
    }
    vm->stack[vm->sp - 1] = r;
    var_set_num(vm,"LAST_N",r); vm->last_n=r;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-0 stack imm unsigned div/mod: SUDIVN · SUMODN (imm dual of SUDIV/SUMOD) */
  if (kw(&L->cur,"SUDIVN")||kw(&L->cur,"STACKUDIVN")||kw(&L->cur,"UDIVN")||
      kw(&L->cur,"SUDIVIMM")||kw(&L->cur,"UDIVIMM")||kw(&L->cur,"SUQUOTN")){
    /* SUDIVN n — TOS = (unsigned)TOS / (unsigned)n; n==0 → 0 soft */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long v = 0;
    if (n != 0) v = (long)((unsigned long)a / (unsigned long)n);
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUMODN")||kw(&L->cur,"STACKUMODN")||kw(&L->cur,"UMODN")||
      kw(&L->cur,"SUMODIMM")||kw(&L->cur,"UMODIMM")||kw(&L->cur,"SUREMN")||
      kw(&L->cur,"UREMN")){
    /* SUMODN n — TOS = (unsigned)TOS % (unsigned)n; n==0 → 0 soft */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long a = vm->stack[vm->sp - 1];
    long v = 0;
    if (n != 0) v = (long)((unsigned long)a % (unsigned long)n);
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack imm reverse ALU: SSUBFROMN · SDIVFROMN · SMODFROMN (n op TOS after SSUBN/SDIVN/SMODN) */
  if (kw(&L->cur,"SSUBFROMN")||kw(&L->cur,"SRSUBN")||kw(&L->cur,"RSUBN")||
      kw(&L->cur,"STACKSUBFROMN")||kw(&L->cur,"NSUBN")||kw(&L->cur,"SSUBFROMIMM")){
    /* SSUBFROMN n — TOS = n - TOS */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = n - vm->stack[vm->sp - 1];
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SDIVFROMN")||kw(&L->cur,"SRDIVN")||kw(&L->cur,"RDIVN")||
      kw(&L->cur,"STACKDIVFROMN")||kw(&L->cur,"NDIVFROMN")||kw(&L->cur,"SDIVFROMIMM")){
    /* SDIVFROMN n — TOS = n / TOS (TOS==0 → 0, soft) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[vm->sp - 1];
    long v = (d == 0) ? 0 : (n / d);
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SMODFROMN")||kw(&L->cur,"SRMODN")||kw(&L->cur,"RMODN")||
      kw(&L->cur,"STACKMODFROMN")||kw(&L->cur,"NMODFROMN")||kw(&L->cur,"SMODFROMIMM")||
      kw(&L->cur,"REMFROMN")){
    /* SMODFROMN n — TOS = n % TOS (TOS==0 → 0, soft) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[vm->sp - 1];
    long v = (d == 0) ? 0 : (n % d);
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-9 stack imm reverse unsigned: SUDIVFROMN · SUMODFROMN (n op_u TOS after SUDIVN/SUMODN) */
  if (kw(&L->cur,"SUDIVFROMN")||kw(&L->cur,"SRUDIVN")||kw(&L->cur,"RUDIVN")||
      kw(&L->cur,"STACKUDIVFROMN")||kw(&L->cur,"NUDIVFROMN")||kw(&L->cur,"SUDIVFROMIMM")||
      kw(&L->cur,"UDIVFROMN")){
    /* SUDIVFROMN n — TOS = (unsigned)n / (unsigned)TOS; TOS==0 → 0 soft */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[vm->sp - 1];
    long v = 0;
    if (d != 0) v = (long)((unsigned long)n / (unsigned long)d);
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SUMODFROMN")||kw(&L->cur,"SRUMODN")||kw(&L->cur,"RUMODN")||
      kw(&L->cur,"STACKUMODFROMN")||kw(&L->cur,"NUMODFROMN")||kw(&L->cur,"SUMODFROMIMM")||
      kw(&L->cur,"UMODFROMN")||kw(&L->cur,"UREMFROMN")){
    /* SUMODFROMN n — TOS = (unsigned)n % (unsigned)TOS; TOS==0 → 0 soft */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long d = vm->stack[vm->sp - 1];
    long v = 0;
    if (d != 0) v = (long)((unsigned long)n % (unsigned long)d);
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-7 reverse imm ANDN plane: SANDNFROMN · SORNFROMN · SXORNFROMN
   * (TOS = n op ~TOS; reverse dual of SANDNI/SORNI/SXORNI after SUDIVFROMN + ANDN stack) */
  if (kw(&L->cur,"SANDNFROMN")||kw(&L->cur,"STACKANDNFROMN")||kw(&L->cur,"ANDNFROMN")||
      kw(&L->cur,"SBICFROMN")||kw(&L->cur,"BICFROMN")||kw(&L->cur,"SANDNFROMIMM")||
      kw(&L->cur,"SRANDNFROMN")||kw(&L->cur,"RANDNFROMN")||kw(&L->cur,"SANDNOTFROMN")){
    /* SANDNFROMN n — TOS = n & ~TOS */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = n & ~vm->stack[vm->sp - 1];
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SORNFROMN")||kw(&L->cur,"STACKORNFROMN")||kw(&L->cur,"ORNFROMN")||
      kw(&L->cur,"SORNOTFROMN")||kw(&L->cur,"ORNOTFROMN")||kw(&L->cur,"SORNFROMIMM")||
      kw(&L->cur,"SRORNFROMN")||kw(&L->cur,"RORNFROMN")){
    /* SORNFROMN n — TOS = n | ~TOS */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = n | ~vm->stack[vm->sp - 1];
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXORNFROMN")||kw(&L->cur,"STACKXORNFROMN")||kw(&L->cur,"XORNFROMN")||
      kw(&L->cur,"SXORNOTFROMN")||kw(&L->cur,"XORNOTFROMN")||kw(&L->cur,"SXORNFROMIMM")||
      kw(&L->cur,"SRXORNFROMN")||kw(&L->cur,"RXORNFROMN")){
    /* SXORNFROMN n — TOS = n ^ ~TOS */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = n ^ ~vm->stack[vm->sp - 1];
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  /* digit-3 reverse imm inverted ANDN plane: SNANDNFROMN · SNORNFROMN · SXNORNFROMN
   * (TOS = ~(n op ~TOS); reverse dual of SNANDNI/SNORNI/SXNORNI after SANDNFROMN) */
  if (kw(&L->cur,"SNANDNFROMN")||kw(&L->cur,"STACKNANDNFROMN")||kw(&L->cur,"NANDNFROMN")||
      kw(&L->cur,"SINVERTANDNFROMN")||kw(&L->cur,"SNANDNFROMIMM")||kw(&L->cur,"RNANDNFROMN")||
      kw(&L->cur,"NANDNOTFROMN")||kw(&L->cur,"SNANDNOTFROMN")){
    /* SNANDNFROMN n — TOS = ~(n & ~TOS)  (= ~n | TOS) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = ~(n & ~vm->stack[vm->sp - 1]);
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SNORNFROMN")||kw(&L->cur,"STACKNORNFROMN")||kw(&L->cur,"NORNFROMN")||
      kw(&L->cur,"SINVERTORNFROMN")||kw(&L->cur,"SNORNFROMIMM")||kw(&L->cur,"RNORNFROMN")||
      kw(&L->cur,"NORNOTFROMN")||kw(&L->cur,"SNORNOTFROMN")){
    /* SNORNFROMN n — TOS = ~(n | ~TOS)  (= ~n & TOS) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = ~(n | ~vm->stack[vm->sp - 1]);
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  if (kw(&L->cur,"SXNORNFROMN")||kw(&L->cur,"STACKXNORNFROMN")||kw(&L->cur,"XNORNFROMN")||
      kw(&L->cur,"SEQUIVNFROMN")||kw(&L->cur,"SXNORNFROMIMM")||kw(&L->cur,"RXNORNFROMN")||
      kw(&L->cur,"XNORNOTFROMN")||kw(&L->cur,"SXNORNOTFROMN")){
    /* SXNORNFROMN n — TOS = ~(n ^ ~TOS)  (equiv n ^ TOS) */
    lex_next(L);
    long n = parse_expr(vm,L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long v = ~(n ^ ~vm->stack[vm->sp - 1]);
    vm->stack[vm->sp - 1] = v;
    var_set_num(vm,"LAST_N",v); vm->last_n=v;
    var_set_num(vm,"SP",vm->sp); var_set_num(vm,"OK",1); bump(vm); return 1;
  }
  return 0;
}
