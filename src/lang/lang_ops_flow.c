/* CubalC lang — lang_ops_flow.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_ops_flow(VM *vm, Lex *L){
  /* plane ops_flow: L30475-31479 */
  if (kw(&L->cur,"FN")||kw(&L->cur,"FUNC")||kw(&L->cur,"FUNCTION")||kw(&L->cur,"DEF")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"FN name"); return -1; }
    char fname[48]; snprintf(fname,sizeof fname,"%s",L->cur.text); lex_next(L);
    skip_nl(L);
    /* capture body from first body token start until matching END */
    size_t b0 = L->tok_off;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"FN without END"); return -1; }
    /* L parked on END; body ends at END's tok_off */
    size_t b1 = L->tok_off;
    if (b1 < b0) b1 = b0;
    size_t blen = b1 - b0;
    if (vm->n_fns >= 32){ fail(vm,"too many FN"); return -1; }
    FnDef *fn = &vm->fns[vm->n_fns++];
    snprintf(fn->name, sizeof fn->name, "%s", fname);
    fn->body = L->s + b0;
    fn->len = blen;
    if (kw(&L->cur,"END")) lex_next(L);
    if (vm->trace) fprintf(vm->trace, "# FN %s len=%zu\n", fname, blen);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"CALL")||kw(&L->cur,"RUNFN")||kw(&L->cur,"DO")||
      kw(&L->cur,"CALLIF")||kw(&L->cur,"CALLNZ")||kw(&L->cur,"CALLZ")||
      kw(&L->cur,"CALLWHEN")||kw(&L->cur,"CALLUNLESS")){
    /* CALL name [args…]
       CALLIF|CALLNZ cond name [args…] — call if cond != 0
       CALLZ cond name [args…] — call if cond == 0
       CALLUNLESS cond name — call if cond == 0 (alias CALLZ) */
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    int mode = 0; /* 0=always, 1=if nz, 2=if z */
    if (strcmp(op,"CALLIF")==0 || strcmp(op,"CALLNZ")==0 || strcmp(op,"CALLWHEN")==0) mode = 1;
    else if (strcmp(op,"CALLZ")==0 || strcmp(op,"CALLUNLESS")==0) mode = 2;
    long cond = 1;
    if (mode){
      cond = parse_expr(vm, L);
    }
    if (L->cur.kind!=TK_IDENT){ fail(vm,"CALL name"); return -1; }
    char fname[48]; snprintf(fname,sizeof fname,"%s",L->cur.text); lex_next(L);
    int do_call = 1;
    if (mode == 1) do_call = (cond != 0);
    else if (mode == 2) do_call = (cond == 0);
    if (!do_call){
      /* still consume optional args so lexer stays aligned */
      int ai=0;
      while (ai<8 && (L->cur.kind==TK_NUM||L->cur.kind==TK_IDENT||L->cur.kind==TK_STR||L->cur.kind==TK_MINUS||L->cur.kind==TK_LPAREN)){
        if (L->cur.kind==TK_STR){ lex_next(L); }
        else { (void)parse_expr(vm,L); }
        ai++;
      }
      var_set_num(vm,"CALLED",0);
      var_set_num(vm,"OK",1);
      bump(vm); return 1;
    }
    FnDef *fn=NULL;
    for (int i=0;i<vm->n_fns;i++) if (strcmp(vm->fns[i].name,fname)==0){ fn=&vm->fns[i]; break; }
    if (!fn){ snprintf(vm->err,sizeof vm->err,"CALL unknown FN %s", fname); fail(vm,vm->err); return -1; }
    /* optional args: CALL name a b c → ARG0 ARG1 */
    int ai=0;
    while (ai<8 && (L->cur.kind==TK_NUM||L->cur.kind==TK_IDENT||L->cur.kind==TK_STR||L->cur.kind==TK_MINUS||L->cur.kind==TK_LPAREN)){
      if (L->cur.kind==TK_STR){
        char an[16]; snprintf(an,sizeof an,"ARG%d",ai);
        var_set_str(vm, an, L->cur.text); lex_next(L);
      } else {
        long v=parse_expr(vm,L);
        char an[16]; snprintf(an,sizeof an,"ARG%d",ai);
        var_set_num(vm, an, v);
      }
      ai++;
    }
    var_set_num(vm, "NARGS", ai);
    var_set_num(vm, "CALLED", 1);
    vm->return_fn = 0;
    Lex fl; lex_init(&fl, fn->body, fn->len);
    if (exec_stmts_until(vm, &fl, "END", NULL)<0) return -1;
    vm->return_fn = 0;
    bump(vm); return 1;
  }
  /* RET [expr] — early return from FN (digit-4 control flow)
     RETIF cond [expr] / RETNZ / RETZ / RETUNLESS — conditional return (digit-1) */
  if (kw(&L->cur,"RET")||kw(&L->cur,"RETURN")||
      kw(&L->cur,"RETIF")||kw(&L->cur,"RETNZ")||kw(&L->cur,"RETZ")||
      kw(&L->cur,"RETUNLESS")||kw(&L->cur,"RETWHEN")){
    char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
    for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
    lex_next(L);
    int mode = 0; /* 0 always, 1 if nz, 2 if z */
    if (strcmp(op,"RETIF")==0 || strcmp(op,"RETNZ")==0 || strcmp(op,"RETWHEN")==0) mode = 1;
    else if (strcmp(op,"RETZ")==0 || strcmp(op,"RETUNLESS")==0) mode = 2;
    int do_ret = 1;
    if (mode){
      long cond = parse_expr(vm, L);
      if (mode == 1) do_ret = (cond != 0);
      else do_ret = (cond == 0);
    }
    /* optional return value when next looks like an expression start */
    int has_val = 0;
    long v = 0;
    if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
        L->cur.kind==TK_STR ||
        (L->cur.kind==TK_IDENT && !kw(&L->cur,"END") && !kw(&L->cur,"ELSE") &&
         !kw(&L->cur,"FN") && !kw(&L->cur,"CALL") && !kw(&L->cur,"LET") &&
         !kw(&L->cur,"ASSERT") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"RET") &&
         !kw(&L->cur,"RETURN") && !kw(&L->cur,"RETIF") && !kw(&L->cur,"RETZ") &&
         !kw(&L->cur,"RETNZ") && !kw(&L->cur,"RETUNLESS") && !kw(&L->cur,"RETWHEN") &&
         !kw(&L->cur,"WHEN") && !kw(&L->cur,"DEFAULT") &&
         !kw(&L->cur,"FOR") && !kw(&L->cur,"WHILE") && !kw(&L->cur,"LOOP") &&
         !kw(&L->cur,"IF") && !kw(&L->cur,"UNLESS") && !kw(&L->cur,"BREAK") &&
         !kw(&L->cur,"CONTINUE") && !kw(&L->cur,"CASE") && !kw(&L->cur,"CUBE") &&
         !kw(&L->cur,"SYS"))){
      v = parse_expr(vm, L);
      has_val = 1;
    }
    if (!do_ret){
      var_set_num(vm,"OK",1);
      bump(vm); return 1;
    }
    if (has_val){
      var_set_num(vm, "LAST_N", v);
      vm->last_n = v;
      var_set_num(vm, "RETVAL", v);
    }
    vm->return_fn = 1;
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* CASE expr ... WHEN n THEN ... [DEFAULT ...] END */
  if (kw(&L->cur,"CASE")||kw(&L->cur,"SWITCH")||kw(&L->cur,"MATCH")){
    lex_next(L);
    long sel = parse_expr(vm, L);
    skip_nl(L);
    int matched = 0;
    int ran = 0;
    for(;;){
      skip_nl(L);
      if (L->cur.kind==TK_EOF){ fail(vm,"CASE without END"); return -1; }
      if (kw(&L->cur,"END")){ lex_next(L); break; }
      if (kw(&L->cur,"WHEN")||kw(&L->cur,"OF")||kw(&L->cur,"CASEIF")){
        lex_next(L);
        long w = parse_expr(vm, L);
        long w_hi = w;
        int is_range = 0;
        /* WHEN lo TO hi THEN — inclusive range arm (digit-1 control_or_branch) */
        if (kw(&L->cur,"TO")||kw(&L->cur,"THROUGH")||kw(&L->cur,"THRU")||
            kw(&L->cur,"DOTDOT")||kw(&L->cur,"RANGE")){
          lex_next(L);
          w_hi = parse_expr(vm, L);
          is_range = 1;
        }
        if (kw(&L->cur,"THEN")) lex_next(L);
        skip_nl(L);
        Lex body_start=*L;
        int depth=1;
        while (L->cur.kind!=TK_EOF && depth>0){
          if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
            lex_next(L); if (kw(&L->cur,"IF")) lex_next(L); continue;
          }
          if (kw(&L->cur,"IF")||kw(&L->cur,"UNLESS")||kw(&L->cur,"LOOP")||kw(&L->cur,"WHILE")||
              kw(&L->cur,"FOR")||kw(&L->cur,"EACH")||kw(&L->cur,"FORCELL")||kw(&L->cur,"EACHCELL")||
              kw(&L->cur,"FORBIT")||kw(&L->cur,"EACHBIT")||
              kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")||
              kw(&L->cur,"CASE")) depth++;
          else if ((kw(&L->cur,"WHEN")||kw(&L->cur,"OF")||kw(&L->cur,"DEFAULT")||kw(&L->cur,"ELSE")) && depth==1) break;
          else if (kw(&L->cur,"END")){ depth--; if (depth==0) break; }
          lex_next(L);
        }
        int arm_hit = 0;
        if (is_range){
          long lo = w, hi = w_hi;
          if (hi < lo){ long t = lo; lo = hi; hi = t; }
          arm_hit = (sel >= lo && sel <= hi);
        } else {
          arm_hit = (w == sel);
        }
        if (!matched && !ran && arm_hit){
          matched = 1; ran = 1;
          Lex body=body_start;
          /* arm body: stop before next WHEN/DEFAULT/END (body copy only) */
          while (!vm->fatal){
            skip_nl(&body);
            if (body.cur.kind==TK_EOF) break;
            if (kw(&body.cur,"END")||kw(&body.cur,"WHEN")||kw(&body.cur,"OF")||
                kw(&body.cur,"DEFAULT")||kw(&body.cur,"ELSE")||kw(&body.cur,"CASEIF")) break;
            if (vm->return_fn || vm->break_loop) break;
            int r=parse_form(vm,&body);
            if (r<0) return -1;
            if (r==0) break;
          }
          /* skip remaining arms to END on outer L (parked on next arm or END) */
          depth=1;
          while (L->cur.kind!=TK_EOF && depth>0){
            if (kw(&L->cur,"CASE")||kw(&L->cur,"IF")||kw(&L->cur,"LOOP")||kw(&L->cur,"WHILE")||
                kw(&L->cur,"FOR")||kw(&L->cur,"FORCELL")||kw(&L->cur,"EACHCELL")||
                kw(&L->cur,"FORBIT")||kw(&L->cur,"EACHBIT")||
                kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")) depth++;
            else if (kw(&L->cur,"END")){ depth--; if(depth==0) break; }
            lex_next(L);
          }
          if (kw(&L->cur,"END")) lex_next(L);
          break;
        }
        continue;
      }
      if (kw(&L->cur,"DEFAULT")||kw(&L->cur,"ELSE")){
        lex_next(L);
        if (kw(&L->cur,"THEN")) lex_next(L);
        skip_nl(L);
        if (!matched && !ran){
          Lex body=*L;
          while (!vm->fatal){
            skip_nl(&body);
            if (body.cur.kind==TK_EOF || kw(&body.cur,"END")) break;
            if (vm->return_fn || vm->break_loop) break;
            int r=parse_form(vm,&body);
            if (r<0) return -1;
            if (r==0) break;
          }
          ran = 1;
        }
        /* always advance outer L to matching END */
        {
          int depth=1;
          while (L->cur.kind!=TK_EOF && depth>0){
            if (kw(&L->cur,"CASE")||kw(&L->cur,"IF")||kw(&L->cur,"LOOP")||kw(&L->cur,"WHILE")||
                kw(&L->cur,"FOR")||kw(&L->cur,"FORCELL")||kw(&L->cur,"EACHCELL")||
                kw(&L->cur,"FORBIT")||kw(&L->cur,"EACHBIT")||
                kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")) depth++;
            else if (kw(&L->cur,"END")){ depth--; if(depth==0) break; }
            lex_next(L);
          }
        }
        if (kw(&L->cur,"END")) lex_next(L);
        break;
      }
      fail(vm,"CASE expects WHEN|DEFAULT|END"); return -1;
    }
    var_set_num(vm, "MATCHED", matched || ran ? 1 : 0);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  /* FOR i = a TO b [STEP s] ... END
     FOR i = a DOWNTO b [STEP s] ... END  (digit-1: default step -1) */
  if (kw(&L->cur,"FOR")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"FOR var = a TO b"); return -1; }
    char vname[48]; snprintf(vname,sizeof vname,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_EQ){ fail(vm,"FOR var ="); return -1; }
    lex_next(L);
    long lo=parse_expr(vm,L);
    if (!kw(&L->cur,"TO") && !kw(&L->cur,"DOWNTO") && !kw(&L->cur,"DOWN") &&
        !kw(&L->cur,"..") && !(L->cur.kind==TK_IDENT && strcmp(L->cur.text,"TO")==0)){
      /* allow FOR i = n as 0..n-1 */
      long hi=lo-1; lo=0;
      long step=1;
      skip_nl(L);
      Lex body_start=*L;
      int depth=1;
      while (L->cur.kind!=TK_EOF){
        if (block_scan_step(L, &depth, 0)) break;
      }
      if (depth!=0){ fail(vm,"FOR without END"); return -1; }
      for (long i=lo;i<=hi && !vm->fatal;i+=step){
        var_set_num(vm,vname,i); var_set_num(vm,"IT",i);
        vm->break_loop=0; vm->continue_loop=0;
        Lex body=body_start;
        if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
        if (vm->break_loop){ vm->break_loop=0; break; }
        vm->continue_loop=0;
      }
      if (kw(&L->cur,"END")) lex_next(L);
      bump(vm); return 1;
    }
    int downto = 0;
    if (kw(&L->cur,"DOWNTO")||kw(&L->cur,"DOWN")){ downto = 1; lex_next(L); }
    else if (kw(&L->cur,"TO")||kw(&L->cur,"..")) lex_next(L);
    long hi=parse_expr(vm,L);
    long step = downto ? -1 : 1;
    if (kw(&L->cur,"STEP")||kw(&L->cur,"BY")){
      lex_next(L);
      step=parse_expr(vm,L);
      if (!step) step = downto ? -1 : 1;
      /* DOWNTO with positive step → force negative direction */
      if (downto && step > 0) step = -step;
    }
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"FOR without END"); return -1; }
    if (step>0){
      for (long i=lo;i<=hi && !vm->fatal;i+=step){
        var_set_num(vm,vname,i); var_set_num(vm,"IT",i);
        vm->break_loop=0; vm->continue_loop=0;
        Lex body=body_start;
        if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
        if (vm->break_loop){ vm->break_loop=0; break; }
        vm->continue_loop=0;
      }
    } else {
      for (long i=lo;i>=hi && !vm->fatal;i+=step){
        var_set_num(vm,vname,i); var_set_num(vm,"IT",i);
        vm->break_loop=0; vm->continue_loop=0;
        Lex body=body_start;
        if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
        if (vm->break_loop){ vm->break_loop=0; break; }
        vm->continue_loop=0;
      }
    }
    if (kw(&L->cur,"END")) lex_next(L);
    bump(vm); return 1;
  }
  /* EACH CUBE as name ... END  |  EACH CELL [as name] [FROM lo TO hi] ... END
   * digit-4 control: cell-range iterator binds value to name, IT=index, VAL=value */
  if (kw(&L->cur,"EACH")||kw(&L->cur,"FOREACH")){
    lex_next(L);
    int is_cell = (kw(&L->cur,"CELL")||kw(&L->cur,"CELLS")||kw(&L->cur,"SLOT")||kw(&L->cur,"SLOTS"));
    int is_cube = (kw(&L->cur,"CUBE")||kw(&L->cur,"CUBES"));
    if (!is_cell && !is_cube){ fail(vm,"EACH CUBE|CELL as name"); return -1; }
    lex_next(L);
    if (is_cube){
      if (kw(&L->cur,"AS")||kw(&L->cur,"->")){ lex_next(L); }
      if (L->cur.kind!=TK_IDENT){ fail(vm,"EACH CUBE as name"); return -1; }
      char cname[48]; snprintf(cname,sizeof cname,"%s",L->cur.text); lex_next(L);
      skip_nl(L);
      Lex body_start=*L;
      int depth=1;
      while (L->cur.kind!=TK_EOF){
        if (block_scan_step(L, &depth, 0)) break;
      }
      if (depth!=0){ fail(vm,"EACH without END"); return -1; }
      ensure_world(vm);
      for (int i=0;i<vm->ch.n_cubes && !vm->fatal;i++){
        var_set_str(vm, cname, vm->ch.cubes[i].id);
        var_set_num(vm, "IT", i);
        var_set_num(vm, "DIGIT", vm->ch.cubes[i].atom.digit);
        var_set_num(vm, "ENERGY", (long)lround(vm->ch.cubes[i].atom.energy*100));
        var_set_num(vm, "SET", cubalc_matrix_popcount(&vm->ch.cubes[i].atom.matrix));
        vm->break_loop=0; vm->continue_loop=0;
        Lex body=body_start;
        if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
        if (vm->break_loop){ vm->break_loop=0; break; }
        vm->continue_loop=0;
      }
      if (kw(&L->cur,"END")) lex_next(L);
      bump(vm); return 1;
    }
    /* EACH CELL [AS name] [FROM lo TO hi | lo TO hi | lo hi] */
    char vname[48]; snprintf(vname,sizeof vname,"VAL");
    if (kw(&L->cur,"AS")||kw(&L->cur,"->")){
      lex_next(L);
      if (L->cur.kind!=TK_IDENT){ fail(vm,"EACH CELL as name"); return -1; }
      snprintf(vname,sizeof vname,"%s",L->cur.text); lex_next(L);
    } else if (L->cur.kind==TK_IDENT &&
               strcmp(L->cur.text,"FROM")!=0 && strcmp(L->cur.text,"TO")!=0 &&
               strcmp(L->cur.text,"FROM")!=0){
      /* EACH CELL name lo hi  (optional bare name) */
      /* only if next tokens look like range — keep default VAL when FROM present */
      /* treat bare IDENT as name when not a keyword */
      if (strcmp(L->cur.text,"FROM")!=0){
        snprintf(vname,sizeof vname,"%s",L->cur.text); lex_next(L);
      }
    }
    long lo = 0, hi = CUBALC_CELL_N - 1;
    if (kw(&L->cur,"FROM")){
      lex_next(L);
      lo = parse_expr(vm,L);
      if (kw(&L->cur,"TO")||kw(&L->cur,"..")||kw(&L->cur,"DOWNTO")) lex_next(L);
      hi = parse_expr(vm,L);
    } else if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
               L->cur.kind==TK_MINUS){
      lo = parse_expr(vm,L);
      if (kw(&L->cur,"TO")||kw(&L->cur,"..")||kw(&L->cur,"DOWNTO")) lex_next(L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
          L->cur.kind==TK_MINUS)
        hi = parse_expr(vm,L);
      else hi = lo;
    }
    if (lo < 0) lo = 0;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"EACH CELL without END"); return -1; }
    for (long i=lo;i<=hi && !vm->fatal;i++){
      long val = vm->cells[(int)i];
      var_set_num(vm, vname, val);
      var_set_num(vm, "VAL", val);
      var_set_num(vm, "IT", i);
      var_set_num(vm, "IDX", i);
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=body_start;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      if (vm->break_loop){ vm->break_loop=0; break; }
      vm->continue_loop=0;
    }
    if (kw(&L->cur,"END")) lex_next(L);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* FORCELL [name] lo hi ... END — compact cell-range loop (digit-4) */
  if (kw(&L->cur,"FORCELL")||kw(&L->cur,"EACHCELL")||kw(&L->cur,"FOREACHCELL")){
    lex_next(L);
    char vname[48]; snprintf(vname,sizeof vname,"VAL");
    /* optional bind name if IDENT followed by another expr token */
    if (L->cur.kind==TK_IDENT){
      /* peek: if only one ident then it's lo as var ref — keep VAL */
      char maybe[48]; snprintf(maybe,sizeof maybe,"%s",L->cur.text);
      Lex save=*L;
      lex_next(L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
          L->cur.kind==TK_MINUS || kw(&L->cur,"TO")||kw(&L->cur,"FROM")){
        snprintf(vname,sizeof vname,"%s",maybe);
      } else {
        *L = save; /* restore — treat as lo expression start */
      }
    }
    long lo = parse_expr(vm,L);
    if (kw(&L->cur,"TO")||kw(&L->cur,"..")||kw(&L->cur,"DOWNTO")||kw(&L->cur,"FROM"))
      lex_next(L);
    long hi = parse_expr(vm,L);
    if (lo < 0) lo = 0;
    if (hi < lo){ long t=lo; lo=hi; hi=t; }
    if (hi >= CUBALC_CELL_N) hi = CUBALC_CELL_N - 1;
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"FORCELL without END"); return -1; }
    for (long i=lo;i<=hi && !vm->fatal;i++){
      long val = vm->cells[(int)i];
      var_set_num(vm, vname, val);
      var_set_num(vm, "VAL", val);
      var_set_num(vm, "IT", i);
      var_set_num(vm, "IDX", i);
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=body_start;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      if (vm->break_loop){ vm->break_loop=0; break; }
      vm->continue_loop=0;
    }
    if (kw(&L->cur,"END")) lex_next(L);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* digit-4 control: FORBIT/EACHBIT cube [AS name] ... END — iterate set-bit indices */
  if (kw(&L->cur,"FORBIT")||kw(&L->cur,"EACHBIT")||kw(&L->cur,"FOREACHBIT")||
      kw(&L->cur,"EACHBITS")||kw(&L->cur,"FORBITS")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"FORBIT cube [AS name]"); return -1; }
    char cid[48]; snprintf(cid,sizeof cid,"%s",L->cur.text); lex_next(L);
    char vname[48]; snprintf(vname,sizeof vname,"IT");
    if (kw(&L->cur,"AS")||kw(&L->cur,"->")){
      lex_next(L);
      if (L->cur.kind!=TK_IDENT){ fail(vm,"FORBIT cube AS name"); return -1; }
      snprintf(vname,sizeof vname,"%s",L->cur.text); lex_next(L);
    }
    skip_nl(L);
    Lex body_start=*L;
    int depth=1;
    while (L->cur.kind!=TK_EOF){
      if (block_scan_step(L, &depth, 0)) break;
    }
    if (depth!=0){ fail(vm,"FORBIT without END"); return -1; }
    int ix=find_cube(vm,cid);
    if (ix<0){
      /* empty iteration */
      if (kw(&L->cur,"END")) lex_next(L);
      var_set_num(vm,"OK",0);
      var_set_num(vm,"LAST_N",0); vm->last_n=0;
      bump(vm); return 1;
    }
    cubalc_matrix *m = &vm->ch.cubes[ix].atom.matrix;
    int n = m->n > 0 ? m->n : CUBALC_ATOM_BITS;
    if (n > CUBALC_ATOM_BITS) n = CUBALC_ATOM_BITS;
    long count = 0;
    for (int bi=0; bi<n && !vm->fatal; bi++){
      if (!cubalc_matrix_get(m, bi)) continue;
      var_set_num(vm, vname, bi);
      var_set_num(vm, "IT", bi);
      var_set_num(vm, "IDX", bi);
      var_set_num(vm, "BIT", bi);
      vm->break_loop=0; vm->continue_loop=0;
      Lex body=body_start;
      if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
      count++;
      if (vm->break_loop){ vm->break_loop=0; break; }
      vm->continue_loop=0;
    }
    if (kw(&L->cur,"END")) lex_next(L);
    var_set_num(vm,"LAST_N",count); vm->last_n=count;
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* PASS / NOP — no-op statement (digit-4 control placeholder) */
  if (kw(&L->cur,"PASS")||kw(&L->cur,"NOP")||kw(&L->cur,"NOOP")||kw(&L->cur,"NOTHING")){
    lex_next(L);
    var_set_num(vm,"OK",1);
    bump(vm); return 1;
  }
  /* BREAKIF expr · CONTINUEIF expr — single-token conditional loop control (digit-4) */
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
  /* BREAK [IF expr] — leave enclosing loop (digit-4 control flow) */
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
  /* CONTINUE [IF expr] — next loop iteration */
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
    /* JUMP [IF expr] — unconditional or conditional break */
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
    /* JZ expr — break if expr == 0 */
    lex_next(L);
    long c = parse_expr(vm, L);
    if (!c) vm->break_loop = 1;
    bump(vm); return 1;
  }
  if (kw(&L->cur,"JNZ")||kw(&L->cur,"JNEZ")||kw(&L->cur,"JTRUE")){
    /* JNZ expr — break if expr != 0 */
    lex_next(L);
    long c = parse_expr(vm, L);
    if (c) vm->break_loop = 1;
    bump(vm); return 1;
  }
  if (kw(&L->cur,"CJZ")||kw(&L->cur,"CJZERO")){
    /* CJZ expr — continue if expr == 0 */
    lex_next(L);
    long c = parse_expr(vm, L);
    if (!c) vm->continue_loop = 1;
    bump(vm); return 1;
  }
  if (kw(&L->cur,"CJNZ")||kw(&L->cur,"CJNEZ")){
    /* CJNZ expr — continue if expr != 0 */
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
    const char *rel = cont ? op+1 : op; /* CBEQ → BEQ */
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
  /* UNLESS cond THEN ... [ELSE ...] END — inverted IF */
  if (kw(&L->cur,"UNLESS")){
    lex_next(L);
    long cond = parse_expr(vm, L);
    if (!kw(&L->cur,"THEN")){ fail(vm,"UNLESS expr THEN"); return -1; }
    lex_next(L); skip_nl(L);
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
    if (depth>1){ fail(vm,"UNLESS without END"); return -1; }
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
    fail(vm,"UNLESS chain broken"); return -1;
  }
  /* digit-1 control: FOREVER / LOOPINF / INFINITE ... END — unbounded until BREAK */
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
    for (; !vm->fatal && guard++<100000;){
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
    for (long t=0;t<times && !vm->fatal;t++){
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
  /* SLOOP — stack count: pop TOS as times, same body semantics as LOOP */
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
    for (long t=0;t<times && !vm->fatal;t++){
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
  /* REPEAT ... UNTIL cond — post-test loop (digit-4 universal control) */
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
      } while (!vm->fatal && guard++<100000);
      *L=after_cond;
    } else {
      /* REPEAT ... END  (same as LOOP 1..∞ with break only — run once as block) */
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
    while (cond && !vm->fatal && guard++<100000){
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
  /* UNTIL cond ... END — pre-test inverse WHILE: run while cond is false (digit-1 loops) */
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
    while (!cond && !vm->fatal && guard++<100000){
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
  if (kw(&L->cur,"IF")){
    lex_next(L);
    /* chain: IF c THEN ... ELIF c THEN ... ELSE ... END */
    for(;;){
      long cond=parse_expr(vm,L);
      if (!kw(&L->cur,"THEN")){ fail(vm,"IF expr THEN"); return -1; }
      lex_next(L); skip_nl(L);
      Lex body_start=*L;
      int depth=1;
      while (L->cur.kind!=TK_EOF && depth>0){
        if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
          lex_next(L);
          if (kw(&L->cur,"IF")) lex_next(L);
          continue;
        }
        if (kw(&L->cur,"IF")||kw(&L->cur,"LOOP")||kw(&L->cur,"SLOOP")||kw(&L->cur,"WHILE")||
            kw(&L->cur,"FOR")||kw(&L->cur,"EACH")||kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")||
            kw(&L->cur,"UNTIL")||kw(&L->cur,"TIMES")) depth++;
        else if ((kw(&L->cur,"ELSE")||kw(&L->cur,"ELIF")||kw(&L->cur,"ELSEIF")) && depth==1) break;
        else if (kw(&L->cur,"END")){ depth--; if (depth==0) break; }
        lex_next(L);
      }
      if (depth>1){ fail(vm,"IF without END"); return -1; }
      if (cond){
        Lex body=body_start;
        if (exec_stmts_until(vm,&body,"END","ELSE")<0) return -1;
        /* also stop at ELIF */
        /* skip to final END */
        depth=1;
        while (L->cur.kind!=TK_EOF && depth>0){
          if (kw(&L->cur,"BREAK")||kw(&L->cur,"CONTINUE")||kw(&L->cur,"NEXT")||kw(&L->cur,"SKIP")){
            lex_next(L);
            if (kw(&L->cur,"IF")) lex_next(L);
            continue;
          }
          if (kw(&L->cur,"IF")||kw(&L->cur,"LOOP")||kw(&L->cur,"SLOOP")||kw(&L->cur,"WHILE")||
              kw(&L->cur,"FOR")||kw(&L->cur,"EACH")||kw(&L->cur,"FN")||kw(&L->cur,"REPEAT")||
              kw(&L->cur,"UNTIL")||kw(&L->cur,"TIMES")) depth++;
          else if (kw(&L->cur,"END")){ depth--; if(depth==0) break; }
          lex_next(L);
        }
        if (kw(&L->cur,"END")) lex_next(L);
        bump(vm); return 1;
      }
      /* not taken */
      if (kw(&L->cur,"ELIF")||kw(&L->cur,"ELSEIF")){ lex_next(L); continue; }
      if (kw(&L->cur,"ELSE")){
        lex_next(L); skip_nl(L);
        Lex body=*L;
        if (exec_stmts_until(vm,&body,"END",NULL)<0) return -1;
        if (kw(&L->cur,"END")) lex_next(L);
        bump(vm); return 1;
      }
      if (kw(&L->cur,"END")){ lex_next(L); bump(vm); return 1; }
      fail(vm,"IF chain broken"); return -1;
    }
  }
  if (kw(&L->cur,"END")||kw(&L->cur,"ELSE")||kw(&L->cur,"ELIF")||kw(&L->cur,"ELSEIF")||kw(&L->cur,"THEN")){
    return 0; /* stop marker for nested bodies */
  }

  snprintf(vm->err,sizeof vm->err,"unknown form '%s' line %d — place a unit with [name]",
           L->cur.text, L->cur.line);
  fail(vm, vm->err);
  return 0;
}
