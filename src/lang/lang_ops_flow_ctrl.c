/* CubalC lang — lang_ops_flow_ctrl.c
 * FLOW control residual: PASS/BREAK/JUMP/UNLESS/FOREVER/RETRY/TIMEIT/LOOP/WHILE/TRY (A3 peel).
 * Mesh vote A3_carve_lang_ops_flow · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_flow_ctrl.h"
#include "lang/cubalc_lang_ops_flow_oop.h"
#include "lang/cubalc_lang_internal.h"
#include "cubalc_sot.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <unistd.h>
#endif

int cubalc_lang_ops_flow_try_ctrl(VM *vm, Lex *L) {
  /* FLOW control residual. Returns 1 handled, 0 not this plane, -1 fail.
   */
  /* PASS / NOP â no-op statement (digit-4 control placeholder) */
  if (kw(&L->cur,"PASS")||kw(&L->cur,"NOP")||kw(&L->cur,"NOOP")||kw(&L->cur,"NOTHING")){
    lex_next(L);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* BREAKIF expr Â· CONTINUEIF expr â single-token conditional loop control (digit-4) */
  if (kw(&L->cur,"BREAKIF")||kw(&L->cur,"BREAK_IF")){
    lex_next(L);
    long c = parse_expr(vm, L);
    if (c) vm->break_loop = 1;
    bump(vm); return 1;
  }
  if (kw(&L->cur,"CONTINUEIF")||kw(&L->cur,"CONTIF")||kw(&L->cur,"SKIPIF")||
      kw(&L->cur,"NEXTIF")||kw(&L->cur,"CONTINUE_IF")){
    lex_next(L);
    long c = parse_expr(vm, L);
    if (c) vm->continue_loop = 1;
    bump(vm); return 1;
  }
  /* BREAK [IF expr] â leave enclosing loop (digit-4 control flow) */
  if (kw(&L->cur,"BREAK")){
    lex_next(L);
    if (kw(&L->cur,"IF")){
      lex_next(L);
      long c = parse_expr(vm, L);
      if (c) vm->break_loop = 1;
    } else {
      vm->break_loop = 1;
    }
    bump(vm); return 1;
  }
  /* CONTINUE [IF expr] â next loop iteration */
  if (kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
    lex_next(L);
    if (kw(&L->cur,"IF")){
      lex_next(L);
      long c = parse_expr(vm, L);
      if (c) vm->continue_loop = 1;
    } else {
      vm->continue_loop = 1;
    }
    bump(vm); return 1;
  }
  /* digit-1 control: JUMP / JZ / JNZ / CJZ / CJNZ (asm-style loop exits) */
  if (kw(&L->cur,"JUMP")||kw(&L->cur,"JMP")){
    /* JUMP [IF expr] â unconditional or conditional break */
    lex_next(L);
    if (kw(&L->cur,"IF")){
      lex_next(L);
      long c = parse_expr(vm, L);
      if (c) vm->break_loop = 1;
    } else {
      vm->break_loop = 1;
    }
    bump(vm); return 1;
  }
  if (kw(&L->cur,"JZ")||kw(&L->cur,"JZERO")){
    /* JZ expr â break if expr == 0 */
    lex_next(L);
    long c = parse_expr(vm, L);
    if (!c) vm->break_loop = 1;
    bump(vm); return 1;
  }
  if (kw(&L->cur,"JNZ")||kw(&L->cur,"JNEZ")||kw(&L->cur,"JTRUE")){
    /* JNZ expr â break if expr != 0 */
    lex_next(L);
    long c = parse_expr(vm, L);
    if (c) vm->break_loop = 1;
    bump(vm); return 1;
  }
  if (kw(&L->cur,"CJZ")||kw(&L->cur,"CJZERO")){
    /* CJZ expr â continue if expr == 0 */
    lex_next(L);
    long c = parse_expr(vm, L);
    if (!c) vm->continue_loop = 1;
    bump(vm); return 1;
  }
  if (kw(&L->cur,"CJNZ")||kw(&L->cur,"CJNEZ")){
    /* CJNZ expr â continue if expr != 0 */
    lex_next(L);
    long c = parse_expr(vm, L);
    if (c) vm->continue_loop = 1;
    bump(vm); return 1;
  }
  /* stack-driven loop control: SJUMP/SBREAK SJZ SJNZ SCONTINUE SCJZ SCJNZ */
  if (kw(&L->cur,"SJUMP")||kw(&L->cur,"SBREAK")||kw(&L->cur,"STACKJUMP")||
      kw(&L->cur,"STACKBREAK")){
    lex_next(L);
    vm->break_loop = 1;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SCONTINUE")||kw(&L->cur,"SNEXT")||kw(&L->cur,"STACKCONTINUE")){
    lex_next(L);
    vm->continue_loop = 1;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SJZ")||kw(&L->cur,"SJZERO")||kw(&L->cur,"STACKJZ")){
    /* pop TOS; break if zero */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long c = vm->stack[--vm->sp];
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",c); vm->last_n=c;
    if (!c) vm->break_loop = 1;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SJNZ")||kw(&L->cur,"SJNEZ")||kw(&L->cur,"STACKJNZ")){
    /* pop TOS; break if nonzero */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long c = vm->stack[--vm->sp];
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",c); vm->last_n=c;
    if (c) vm->break_loop = 1;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SCJZ")||kw(&L->cur,"STACKCJZ")){
    /* pop TOS; continue if zero */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long c = vm->stack[--vm->sp];
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",c); vm->last_n=c;
    if (!c) vm->continue_loop = 1;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SCJNZ")||kw(&L->cur,"STACKCJNZ")){
    /* pop TOS; continue if nonzero */
    lex_next(L);
    if (vm->sp < 1){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long c = vm->stack[--vm->sp];
    var_set_num(vm,"SP",vm->sp);
    var_set_num(vm,"LAST_N",c); vm->last_n=c;
    if (c) vm->continue_loop = 1;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* digit-1 control_or_branch: BEQ/BNE/BLT/BLE/BGT/BGE + CB* continue */
  if (kw(&L->cur,"BEQ")||kw(&L->cur,"BNE")||kw(&L->cur,"BLT")||kw(&L->cur,"BLE")||
      kw(&L->cur,"BGT")||kw(&L->cur,"BGE")||
      kw(&L->cur,"CBEQ")||kw(&L->cur,"CBNE")||kw(&L->cur,"CBLT")||kw(&L->cur,"CBLE")||
      kw(&L->cur,"CBGT")||kw(&L->cur,"CBGE")){
    char op[12]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    long a = parse_expr(vm, L);
    long b = parse_expr(vm, L);
    int cont = (op[0]=='C');
    const char *rel = cont ? op+1 : op; /* CBEQ â BEQ */
    int hit = 0;
    if (strcmp(rel,"BEQ")==0) hit = (a == b);
    else if (strcmp(rel,"BNE")==0) hit = (a != b);
    else if (strcmp(rel,"BLT")==0) hit = (a < b);
    else if (strcmp(rel,"BLE")==0) hit = (a <= b);
    else if (strcmp(rel,"BGT")==0) hit = (a > b);
    else if (strcmp(rel,"BGE")==0) hit = (a >= b);
    if (hit){
      if (cont) vm->continue_loop = 1;
      else vm->break_loop = 1;
    }
    var_set_num(vm,"LAST_N", hit ? 1 : 0); vm->last_n = hit ? 1 : 0;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* stack compare-branch: SBEQ..SBGE + SCB* continue (pop a,b under/TOS) */
  if (kw(&L->cur,"SBEQ")||kw(&L->cur,"SBNE")||kw(&L->cur,"SBLT")||kw(&L->cur,"SBLE")||
      kw(&L->cur,"SBGT")||kw(&L->cur,"SBGE")||
      kw(&L->cur,"SCBEQ")||kw(&L->cur,"SCBNE")||kw(&L->cur,"SCBLT")||kw(&L->cur,"SCBLE")||
      kw(&L->cur,"SCBGT")||kw(&L->cur,"SCBGE")||
      kw(&L->cur,"STACKBEQ")||kw(&L->cur,"STACKBNE")||kw(&L->cur,"STACKBLT")||
      kw(&L->cur,"STACKBLE")||kw(&L->cur,"STACKBGT")||kw(&L->cur,"STACKBGE")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    if (vm->sp < 2){ var_set_num(vm,"OK",0); bump(vm); return 1; }
    long b = vm->stack[--vm->sp];
    long a = vm->stack[--vm->sp];
    var_set_num(vm,"SP",vm->sp);
    int cont = 0;
    const char *rel = "BEQ";
    if (strncmp(op,"STACK",5)==0) rel = op+5;
    else if (strncmp(op,"SCB",3)==0){ cont = 1; rel = op+2; }
    else if (op[0]=='S') rel = op+1;
    int hit = 0;
    if (strcmp(rel,"BEQ")==0) hit = (a == b);
    else if (strcmp(rel,"BNE")==0) hit = (a != b);
    else if (strcmp(rel,"BLT")==0) hit = (a < b);
    else if (strcmp(rel,"BLE")==0) hit = (a <= b);
    else if (strcmp(rel,"BGT")==0) hit = (a > b);
    else if (strcmp(rel,"BGE")==0) hit = (a >= b);
    if (hit){
      if (cont) vm->continue_loop = 1;
      else vm->break_loop = 1;
    }
    var_set_num(vm,"LAST_N", hit ? 1 : 0); vm->last_n = hit ? 1 : 0;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* UNLESS cond [THEN] ... [ELSE ...] END â inverted IF; THEN optional */
  if (kw(&L->cur,"UNLESS")){
    int aln = L->cur.line;
    lex_next(L);
    long cond = parse_expr(vm, L);
    skip_nl(L);
    if (kw(&L->cur,"THEN")){ lex_next(L); skip_nl(L); }
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF && depth>0){
      if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
        lex_next(L);
        if (kw(&L->cur,"IF")) lex_next(L);
        continue;
      }
      if (kw(&L->cur,"IF")||kw(&L->cur,"UNLESS")||kw(&L->cur,"LOOP")||kw(&L->cur,"SLOOP")||
          kw(&L->cur,"WHILE")||kw(&L->cur,"FOR")||kw(&L->cur,"EACH")||kw(&L->cur,"FORCELL")||
          kw(&L->cur,"EACHCELL")||kw(&L->cur,"FORBIT")||kw(&L->cur,"EACHBIT")||kw(&L->cur,"FN")||
          kw(&L->cur,"REPEAT")||kw(&L->cur,"UNTIL")||kw(&L->cur,"TIMES")||kw(&L->cur,"CASE"))
        depth++;
      else if ((kw(&L->cur,"ELSE")||kw(&L->cur,"ELIF")||kw(&L->cur,"ELSEIF")) && depth==1) break;
      else if (kw(&L->cur,"END")){ depth--; if (depth==0) break; }
      lex_next(L);
    }
    if (depth>1){
      char ebuf[160];
      snprintf(ebuf, sizeof ebuf,
               "UNLESS without END line %d â close with END", aln);
      fail(vm, ebuf); return -1;
    }
    if (!cond){
      Lex body=body_start;
      if (exec_stmts_until(vm,&body,"END","ELSE")<0) return -1;
      depth=1;
      while (L->cur.kind!=TK_EOF && depth>0){
        if (kw(&L->cur,"IF")||kw(&L->cur,"UNLESS")||kw(&L->cur,"LOOP")||kw(&L->cur,"SLOOP")||
            kw(&L->cur,"WHILE")||kw(&L->cur,"FOR")||kw(&L->cur,"EACH")||kw(&L->cur,"FORCELL")||
            kw(&L->cur,"EACHCELL")||kw(&L->cur,"FORBIT")||kw(&L->cur,"EACHBIT")||kw(&L->cur,"FN")||
            kw(&L->cur,"REPEAT")||kw(&L->cur,"UNTIL")||kw(&L->cur,"TIMES")||kw(&L->cur,"CASE"))
          depth++;
        else if (kw(&L->cur,"END")){ depth--; if(depth==0) break; }
        lex_next(L);
      }
      if (kw(&L->cur,"END")) lex_next(L);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"ELSE")){
      lex_next(L); skip_nl(L);
      Lex body=*L;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      /* advance outer L from ELSE-body start to matching END */
      depth=1;
      while (L->cur.kind!=TK_EOF && depth>0){
        if (kw(&L->cur,"IF")||kw(&L->cur,"UNLESS")||kw(&L->cur,"LOOP")||kw(&L->cur,"SLOOP")||
            kw(&L->cur,"WHILE")||kw(&L->cur,"FOR")||kw(&L->cur,"EACH")||kw(&L->cur,"FORCELL")||
            kw(&L->cur,"EACHCELL")||kw(&L->cur,"FORBIT")||kw(&L->cur,"EACHBIT")||kw(&L->cur,"FN")||
            kw(&L->cur,"REPEAT")||kw(&L->cur,"UNTIL")||kw(&L->cur,"TIMES")||kw(&L->cur,"CASE"))
          depth++;
        else if (kw(&L->cur,"END")){ depth--; if(depth==0) break; }
        lex_next(L);
      }
      if (kw(&L->cur,"END")) lex_next(L);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"END")){ lex_next(L); bump(vm); return 1; }
    {
      char ebuf[160];
      snprintf(ebuf, sizeof ebuf,
               "UNLESS chain broken line %d â expected ELSE/END", aln);
      fail(vm, ebuf); return -1;
    }
  }
  /* digit-1 control: FOREVER / LOOPINF / INFINITE ... END â unbounded until BREAK */
  if (kw(&L->cur,"FOREVER")||kw(&L->cur,"LOOPINF")||kw(&L->cur,"INFINITE")||
      kw(&L->cur,"LOOPFOREVER")){
    lex_next(L);
    skip_nl(L);
    Lex save=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"FOREVER without END"); return -1; }
    long guard=0;
    for (; !vm->fatal && !vm->halt && guard++<100000;){
      long *it=var_slot(vm,"IT",1); if (it) *it=guard-1;
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=save;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      if (vm->break_loop){ vm->break_loop=0; break; }
      vm->continue_loop=0;
    }
    if (kw(&L->cur,"END")) lex_next(L);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* RETRY n [EVERY|SLEEP|BACKOFF ms] â¦ END â re-run body until OK=1 or n exhausted.
   * Clears OK before each attempt so soft TRYCALL/EXPECT must re-establish success.
   * Optional EVERY sleeps between failed attempts (capped 60s). RETRY_N attempts used.
   * Usability: agent flaky hooks without LOOP+BREAKIF+SLEEP glue. */
  if (kw(&L->cur,"RETRY")||kw(&L->cur,"ATTEMPT")||kw(&L->cur,"TRIES")||
      kw(&L->cur,"RETRYN")||kw(&L->cur,"NRETRY")){
    long ntry, every_ms = 0, attempt, okv;
    Lex save;
    int depth = 1;
    Var *ov;
    lex_next(L);
    ntry = parse_expr(vm, L);
    if (ntry < 1) ntry = 1;
    if (ntry > 10000) ntry = 10000;
    if (kw(&L->cur,"EVERY")||kw(&L->cur,"SLEEP")||kw(&L->cur,"BACKOFF")||
        kw(&L->cur,"WAIT")||kw(&L->cur,"DELAY")||kw(&L->cur,"PAUSE")){
      lex_next(L);
      every_ms = parse_expr(vm, L);
      if (every_ms < 0) every_ms = 0;
      if (every_ms > 60000) every_ms = 60000;
      if (kw(&L->cur,"MS")||kw(&L->cur,"MILLIS")||kw(&L->cur,"MILLISECONDS"))
        lex_next(L);
    }
    skip_nl(L);
    save = *L;
    while (L->cur.kind != TK_EOF) {
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth != 0) { fail(vm, "RETRY without END"); return -1; }
    okv = 0;
    for (attempt = 1; attempt <= ntry && !vm->fatal && !vm->halt; attempt++) {
      var_set_num(vm, "OK", 0);
      var_set_num(vm, "IT", attempt - 1);
      var_set_num(vm, "RETRY_I", attempt);
      vm->break_loop = 0;
      vm->continue_loop = 0;
      {
        Lex body = save;
        if (exec_stmts_until(vm, &body, "END", NULL) < 0) return -1;
      }
      if (vm->break_loop) { vm->break_loop = 0; break; }
      vm->continue_loop = 0;
      ov = var_get(vm, "OK", 0);
      okv = ov ? ov->val : 0;
      if (okv) break;
      if (attempt < ntry && every_ms > 0) {
        struct timespec ts;
        ts.tv_sec = every_ms / 1000L;
        ts.tv_nsec = (every_ms % 1000L) * 1000000L;
        nanosleep(&ts, NULL);
      }
    }
    if (kw(&L->cur, "END")) lex_next(L);
    var_set_num(vm, "RETRY_N", attempt > ntry ? ntry : attempt);
    var_set_num(vm, "RETRY_OK", okv ? 1 : 0);
    var_set_num(vm, "LAST_N", attempt > ntry ? ntry : attempt);
    vm->last_n = attempt > ntry ? ntry : attempt;
    {
      char nb[32];
      snprintf(nb, sizeof nb, "%ld", vm->last_n);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    /* leave OK as final body OK (0 if all attempts failed) */
    var_set_num(vm, "OK", okv ? 1 : 0);
    if (vm->trace)
      fprintf(vm->trace, "# RETRY attempts=%ld ok=%ld every=%ld\n",
              attempt > ntry ? ntry : attempt, okv, every_ms);
    bump(vm);
    return 1;
  }
  /* TIMEIT|BENCH|ELAPSED|TIMING|MEASURE â¦ END â wall mono ms for body.
   * LAST_N/TIMEIT_MS/BENCH_MS/ELAPSED = elapsed; OK=1.
   * Usability: agent measure hooks without SYS MS + LET delta glue. */
  if (kw(&L->cur,"TIMEIT")||kw(&L->cur,"BENCH")||kw(&L->cur,"ELAPSED")||
      kw(&L->cur,"TIMING")||kw(&L->cur,"MEASURE")){
    long t0, t1, dt;
    Lex save;
    int depth = 1;
    char nb[32];
    lex_next(L);
    skip_nl(L);
    save = *L;
    while (L->cur.kind != TK_EOF) {
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth != 0) { fail(vm, "TIMEIT without END"); return -1; }
    t0 = cubalc_lang_mono_ms();
    {
      Lex body = save;
      if (exec_stmts_until(vm, &body, "END", NULL) < 0) return -1;
    }
    t1 = cubalc_lang_mono_ms();
    dt = t1 - t0;
    if (dt < 0) dt = 0;
    if (kw(&L->cur, "END")) lex_next(L);
    var_set_num(vm, "LAST_N", dt);
    vm->last_n = dt;
    var_set_num(vm, "TIMEIT_MS", dt);
    var_set_num(vm, "BENCH_MS", dt);
    var_set_num(vm, "ELAPSED", dt);
    var_set_num(vm, "TIMEIT_N", 1);
    snprintf(nb, sizeof nb, "%ld", dt);
    var_set_str(vm, "LAST", nb);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    var_set_num(vm, "OK", 1);
    if (vm->trace)
      fprintf(vm->trace, "# TIMEIT %ld ms\n", dt);
    bump(vm);
    return 1;
  }
  if (kw(&L->cur,"LOOP")||kw(&L->cur,"TIMES")){
    lex_next(L);
    long times=parse_expr(vm,L);
    if (times<0) times=0;
    if (times>100000) times=100000;
    skip_nl(L);
    Lex save=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"LOOP without END"); return -1; }
    for (long t=0;t<times && !vm->fatal && !vm->halt;t++){
      long *it=var_slot(vm,"IT",1); if (it) *it=t;
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=save;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      if (vm->break_loop){ vm->break_loop=0; break; }
      /* continue_loop: already stopped body via exec_stmts_until */
      vm->continue_loop=0;
    }
    if (kw(&L->cur,"END")) lex_next(L);
    bump(vm); return 1;
  }
  /* SLOOP â stack count: pop TOS as times, same body semantics as LOOP */
  if (kw(&L->cur,"SLOOP")||kw(&L->cur,"STACKLOOP")){
    lex_next(L);
    long times = 0;
    if (vm->sp >= 1){
      times = vm->stack[--vm->sp];
      var_set_num(vm,"SP",vm->sp);
      var_set_num(vm,"OK",1);
    } else {
      var_set_num(vm,"OK",0);
    }
    if (times<0) times=0;
    if (times>100000) times=100000;
    skip_nl(L);
    Lex save=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"SLOOP without END"); return -1; }
    for (long t=0;t<times && !vm->fatal && !vm->halt;t++){
      long *it=var_slot(vm,"IT",1); if (it) *it=t;
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=save;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      if (vm->break_loop){ vm->break_loop=0; break; }
      vm->continue_loop=0;
    }
    if (kw(&L->cur,"END")) lex_next(L);
    bump(vm); return 1;
  }
  /* REPEAT ... UNTIL cond â post-test loop (digit-4 universal control) */
  if (kw(&L->cur,"REPEAT")){
    lex_next(L);
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 1)) break;
    }
    if (!(kw(&L->cur,"UNTIL") || kw(&L->cur,"END"))){ fail(vm,"REPEAT without UNTIL|END"); return -1; }
    int use_until = kw(&L->cur,"UNTIL") ? 1 : 0;
    if (use_until){
      lex_next(L);
      Lex cond_start=*L;
      (void)parse_expr(vm,L); /* advance over cond for outer scan */
      Lex after_cond=*L;
      long guard=0;
      do {
        vm->break_loop=0; vm->continue_loop=0;
        Lex body=body_start;
        if (exec_stmts_until(vm,&body,"UNTIL",NULL)<0) return -1;
        if (vm->break_loop){ vm->break_loop=0; break; }
        vm->continue_loop=0;
        Lex clex=cond_start;
        long done = parse_expr(vm,&clex);
        if (done) break;
      } while (!vm->fatal && !vm->halt && guard++<100000);
      *L=after_cond;
    } else {
      /* REPEAT ... END  (same as LOOP 1..â with break only â run once as block) */
      if (kw(&L->cur,"END")) lex_next(L);
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=body_start;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      vm->break_loop=0; vm->continue_loop=0;
    }
    bump(vm); return 1;
  }
  if (kw(&L->cur,"WHILE")){
    lex_next(L);
    Lex cond_start=*L;
    long cond=parse_expr(vm,L);
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"WHILE without END"); return -1; }
    Lex end_tok=*L;
    long guard=0;
    while (cond && !vm->fatal && !vm->halt && guard++<100000){
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=body_start;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      if (vm->break_loop){ vm->break_loop=0; break; }
      vm->continue_loop=0;
      Lex clex=cond_start;
      cond=parse_expr(vm,&clex);
    }
    *L=end_tok;
    if (kw(&L->cur,"END")) lex_next(L);
    bump(vm); return 1;
  }
  /* UNTIL cond ... END â pre-test inverse WHILE: run while cond is false (digit-1 loops) */
  if (kw(&L->cur,"UNTIL")){
    lex_next(L);
    Lex cond_start=*L;
    long cond=parse_expr(vm,L);
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"UNTIL without END"); return -1; }
    Lex end_tok=*L;
    long guard=0;
    while (!cond && !vm->fatal && !vm->halt && guard++<100000){
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=body_start;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      if (vm->break_loop){ vm->break_loop=0; break; }
      vm->continue_loop=0;
      Lex clex=cond_start;
      cond=parse_expr(vm,&clex);
    }
    *L=end_tok;
    if (kw(&L->cur,"END")) lex_next(L);
    bump(vm); return 1;
  }
  /* TRY â¦ [CATCH â¦] [FINALLY â¦] END â soft recovery + always cleanup.
   * CATCH|ONERR|EXCEPT runs when body soft-fails (OK==0 or sticky LAST_ERR) and not EXIT/fatal.
   * FINALLY|CLEANUP|ALWAYS always runs (EXIT-safe). Aliases: GUARD|WITHCLEANUP|ENSURE.
   * Usability: recover soft TRYCALL/EXPECT without IF OK glue; still free temps. */
  if (kw(&L->cur,"TRY")||kw(&L->cur,"GUARD")||kw(&L->cur,"WITHCLEANUP")||
      kw(&L->cur,"ENSURE")||kw(&L->cur,"TRYFINALLY")||kw(&L->cur,"PROTECTED")){
    int aln = L->cur.line;
    Lex try_start, catch_start, fin_start;
    int depth = 1;
    int has_catch = 0, has_finally = 0;
    int body_rc = 0, catch_rc = 0, fin_rc = 0;
    int saved_halt = 0, saved_return = 0, saved_break = 0, saved_continue = 0;
    int saved_fatal = 0, saved_exit = 0;
    int soft_fail = 0, ran_catch = 0;
    Var *ov, *ev;
    lex_next(L);
    skip_nl(L);
    try_start = *L;
    while (L->cur.kind != TK_EOF && depth > 0) {
      if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
        lex_next(L);
        if (kw(&L->cur,"IF")) lex_next(L);
        continue;
      }
      if (kw(&L->cur,"IF")||kw(&L->cur,"UNLESS")||kw(&L->cur,"LOOP")||kw(&L->cur,"SLOOP")||
          kw(&L->cur,"WHILE")||kw(&L->cur,"FOR")||kw(&L->cur,"EACH")||kw(&L->cur,"FN")||
          kw(&L->cur,"REPEAT")||kw(&L->cur,"UNTIL")||kw(&L->cur,"TIMES")||
          kw(&L->cur,"CASE")||kw(&L->cur,"TIMEIT")||kw(&L->cur,"BENCH")||
          kw(&L->cur,"RETRY")||kw(&L->cur,"ATTEMPT")||kw(&L->cur,"TRIES")||
          kw(&L->cur,"TRY")||kw(&L->cur,"GUARD")||kw(&L->cur,"WITHCLEANUP")||
          kw(&L->cur,"ENSURE"))
        depth++;
      else if ((kw(&L->cur,"CATCH")||kw(&L->cur,"ONERR")||kw(&L->cur,"ON_ERR")||
                kw(&L->cur,"EXCEPT")||kw(&L->cur,"RECOVER")) && depth == 1) {
        has_catch = 1;
        break;
      } else if ((kw(&L->cur,"FINALLY")||kw(&L->cur,"CLEANUP")||kw(&L->cur,"ALWAYS")||
                  kw(&L->cur,"AFTERALL")) && depth == 1) {
        has_finally = 1;
        break;
      } else if (kw(&L->cur,"END")) {
        depth--;
        if (depth == 0) break;
      }
      lex_next(L);
    }
    if (depth > 1 || (depth == 1 && L->cur.kind == TK_EOF && !has_catch && !has_finally)) {
      char ebuf[160];
      snprintf(ebuf, sizeof ebuf,
               "TRY without END line %d â TRY â¦ [CATCH â¦] [FINALLY â¦] END", aln);
      fail(vm, ebuf); return -1;
    }
    /* run try body until CATCH|FINALLY|END (or halt/fatal) */
    {
      Lex body = try_start;
      body_rc = 0;
      while (!vm->fatal && !vm->halt) {
        if (cubalc_lang_check_timeout(vm, body.cur.line)) { body_rc = -1; break; }
        skip_nl(&body);
        if (body.cur.kind == TK_EOF) break;
        if (kw(&body.cur, "END") || kw(&body.cur, "CATCH") ||
            kw(&body.cur, "ONERR") || kw(&body.cur, "ON_ERR") ||
            kw(&body.cur, "EXCEPT") || kw(&body.cur, "RECOVER") ||
            kw(&body.cur, "FINALLY") || kw(&body.cur, "CLEANUP") ||
            kw(&body.cur, "ALWAYS") || kw(&body.cur, "AFTERALL"))
          break;
        if (vm->break_loop || vm->continue_loop || vm->return_fn || vm->halt) break;
        {
          int r = parse_form(vm, &body);
          if (r < 0) { body_rc = -1; break; }
          if (r == 0) break;
        }
        if (vm->break_loop || vm->continue_loop || vm->return_fn || vm->halt) break;
      }
      if (vm->fatal) body_rc = -1;
    }
    saved_halt = vm->halt;
    saved_exit = vm->exit_code;
    saved_return = vm->return_fn;
    saved_break = vm->break_loop;
    saved_continue = vm->continue_loop;
    saved_fatal = vm->fatal;
    ov = var_get(vm, "OK", 0);
    ev = var_get(vm, "LAST_ERR", 0);
    soft_fail = (!saved_fatal && !saved_halt &&
                 ((ov && ov->val == 0) ||
                  (ev && ev->is_str && ev->sval[0])));

    /* allow CATCH/FINALLY to run even if body halted/fatal */
    vm->halt = 0;
    vm->return_fn = 0;
    vm->break_loop = 0;
    vm->continue_loop = 0;
    if (saved_fatal) vm->fatal = 0;

    /* optional CATCH arm (soft fail only) */
    if (has_catch) {
      lex_next(L); /* CATCH|ONERR|â¦ */
      skip_nl(L);
      catch_start = *L;
      depth = 1;
      while (L->cur.kind != TK_EOF && depth > 0) {
        if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
          lex_next(L);
          if (kw(&L->cur,"IF")) lex_next(L);
          continue;
        }
        if (kw(&L->cur,"IF")||kw(&L->cur,"UNLESS")||kw(&L->cur,"LOOP")||kw(&L->cur,"SLOOP")||
            kw(&L->cur,"WHILE")||kw(&L->cur,"FOR")||kw(&L->cur,"EACH")||kw(&L->cur,"FN")||
            kw(&L->cur,"REPEAT")||kw(&L->cur,"UNTIL")||kw(&L->cur,"TIMES")||
            kw(&L->cur,"CASE")||kw(&L->cur,"TIMEIT")||kw(&L->cur,"BENCH")||
            kw(&L->cur,"RETRY")||kw(&L->cur,"TRY")||kw(&L->cur,"GUARD")||
            kw(&L->cur,"WITHCLEANUP")||kw(&L->cur,"ENSURE"))
          depth++;
        else if ((kw(&L->cur,"FINALLY")||kw(&L->cur,"CLEANUP")||kw(&L->cur,"ALWAYS")||
                  kw(&L->cur,"AFTERALL")) && depth == 1) {
          has_finally = 1;
          break;
        } else if (kw(&L->cur,"END")) {
          depth--;
          if (depth == 0) break;
        }
        lex_next(L);
      }
      if (depth > 1 || (depth == 1 && L->cur.kind == TK_EOF && !has_finally)) {
        char ebuf[160];
        snprintf(ebuf, sizeof ebuf,
                 "CATCH without END line %d â TRY â¦ CATCH â¦ [FINALLY â¦] END", aln);
        fail(vm, ebuf); return -1;
      }
      if (soft_fail) {
        Lex cbody = catch_start;
        catch_rc = 0;
        while (!vm->fatal && !vm->halt) {
          if (cubalc_lang_check_timeout(vm, cbody.cur.line)) { catch_rc = -1; break; }
          skip_nl(&cbody);
          if (cbody.cur.kind == TK_EOF) break;
          if (kw(&cbody.cur, "END") || kw(&cbody.cur, "FINALLY") ||
              kw(&cbody.cur, "CLEANUP") || kw(&cbody.cur, "ALWAYS") ||
              kw(&cbody.cur, "AFTERALL"))
            break;
          if (vm->break_loop || vm->continue_loop || vm->return_fn || vm->halt) break;
          {
            int r = parse_form(vm, &cbody);
            if (r < 0) { catch_rc = -1; break; }
            if (r == 0) break;
          }
          if (vm->break_loop || vm->continue_loop || vm->return_fn || vm->halt) break;
        }
        if (vm->fatal) catch_rc = -1;
        ran_catch = 1;
        /* catch may EXIT â remember for after finally */
        if (vm->halt && !saved_halt) {
          saved_halt = 1;
          saved_exit = vm->exit_code;
        }
        if (vm->return_fn) saved_return = 1;
        if (vm->fatal) saved_fatal = 1;
        vm->halt = 0;
        vm->return_fn = 0;
        vm->break_loop = 0;
        vm->continue_loop = 0;
        if (vm->fatal) vm->fatal = 0;
      }
      /* if catch not taken, L already at FINALLY or END */
    }

    if (has_finally ||
        kw(&L->cur,"FINALLY")||kw(&L->cur,"CLEANUP")||kw(&L->cur,"ALWAYS")||
        kw(&L->cur,"AFTERALL")) {
      if (!has_finally &&
          (kw(&L->cur,"FINALLY")||kw(&L->cur,"CLEANUP")||kw(&L->cur,"ALWAYS")||
           kw(&L->cur,"AFTERALL")))
        has_finally = 1;
      if (has_finally &&
          (kw(&L->cur,"FINALLY")||kw(&L->cur,"CLEANUP")||kw(&L->cur,"ALWAYS")||
           kw(&L->cur,"AFTERALL"))) {
        lex_next(L);
        skip_nl(L);
        fin_start = *L;
        depth = 1;
        while (L->cur.kind != TK_EOF && depth > 0) {
          if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
            lex_next(L);
            if (kw(&L->cur,"IF")) lex_next(L);
            continue;
          }
          if (kw(&L->cur,"IF")||kw(&L->cur,"UNLESS")||kw(&L->cur,"LOOP")||kw(&L->cur,"SLOOP")||
              kw(&L->cur,"WHILE")||kw(&L->cur,"FOR")||kw(&L->cur,"EACH")||kw(&L->cur,"FN")||
              kw(&L->cur,"REPEAT")||kw(&L->cur,"UNTIL")||kw(&L->cur,"TIMES")||
              kw(&L->cur,"CASE")||kw(&L->cur,"TIMEIT")||kw(&L->cur,"BENCH")||
              kw(&L->cur,"RETRY")||kw(&L->cur,"TRY")||kw(&L->cur,"GUARD")||
              kw(&L->cur,"WITHCLEANUP")||kw(&L->cur,"ENSURE"))
            depth++;
          else if (kw(&L->cur,"END")) {
            depth--;
            if (depth == 0) break;
          }
          lex_next(L);
        }
        if (depth != 0) {
          char ebuf[160];
          snprintf(ebuf, sizeof ebuf,
                   "FINALLY without END line %d â TRY â¦ FINALLY â¦ END", aln);
          fail(vm, ebuf); return -1;
        }
        {
          Lex fbody = fin_start;
          fin_rc = exec_stmts_until(vm, &fbody, "END", NULL);
        }
      }
    }
    if (kw(&L->cur,"END")) lex_next(L);

    /* restore body/catch control; keep later fatal if set */
    if (saved_fatal && !vm->fatal) vm->fatal = 1;
    if (saved_halt) {
      vm->halt = 1;
      vm->exit_code = saved_exit;
    }
    if (saved_return) vm->return_fn = 1;
    if (saved_break) vm->break_loop = 1;
    if (saved_continue) vm->continue_loop = 1;

    var_set_num(vm, "CATCH_N", ran_catch ? 1 : 0);
    var_set_num(vm, "FINALLY_N", has_finally ? 1 : 0);
    var_set_num(vm, "TRY_N", 1);
    if (vm->trace)
      fprintf(vm->trace, "# TRY catch=%d finally=%d soft_fail=%d\n",
              ran_catch, has_finally, soft_fail);
    if (vm->fatal) return -1;
    if (body_rc < 0 && saved_fatal) return -1;
    bump(vm);
    return 1;
  }
  /* HASERR|HASOK â soft-status probes â LAST_N 0|1 (no mutate OK/ERR).
   * HASERR: OK==0 or sticky LAST_ERR. HASOK: OK!=0 and no LAST_ERR.
   * Usability: IF HASERR without reading LAST_ERR strings. */
  if (kw(&L->cur,"HASERR")||kw(&L->cur,"HAS_ERR")||kw(&L->cur,"HASERROR")||
      kw(&L->cur,"ISERR")||kw(&L->cur,"ERRED")||kw(&L->cur,"FAILED")||
      kw(&L->cur,"HASOK")||kw(&L->cur,"HAS_OK")||kw(&L->cur,"ISOK")||
      kw(&L->cur,"OKP")||kw(&L->cur,"SUCCEEDED")){
    char op[24];
    int want_ok = 0;
    Var *ov, *ev;
    long okv = 1;
    int has_err = 0, hit = 0;
    snprintf(op, sizeof op, "%s", L->cur.text);
    {
      char *q;
      for (q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
    }
    if (strcmp(op, "HASOK") == 0 || strcmp(op, "HAS_OK") == 0 ||
        strcmp(op, "ISOK") == 0 || strcmp(op, "OKP") == 0 ||
        strcmp(op, "SUCCEEDED") == 0)
      want_ok = 1;
    lex_next(L);
    ov = var_get(vm, "OK", 0);
    ev = var_get(vm, "LAST_ERR", 0);
    if (ov) okv = ov->val;
    if (okv == 0) has_err = 1;
    if (ev && ev->is_str && ev->sval[0]) has_err = 1;
    hit = want_ok ? (!has_err ? 1 : 0) : (has_err ? 1 : 0);
    var_set_num(vm, "LAST_N", hit);
    vm->last_n = hit;
    var_set_num(vm, want_ok ? "HASOK_N" : "HASERR_N", hit);
    {
      char nb[8];
      snprintf(nb, sizeof nb, "%d", hit);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    /* do not change OK */
    bump(vm);
    return 1;
  }

  return 0;
}
