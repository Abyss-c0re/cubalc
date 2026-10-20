/* CubalC lang — lang_ops_core_async.c
 * ASYNC / AWAIT / PARALLEL plane (mesh vote A_split_lang_ops_core).
 */
#include "lang/cubalc_lang_ops_core_async.h"
#include "cubalc_async.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

int cubalc_lang_ops_core_try_async(VM *vm, Lex *L) {
  /* ASYNC / AWAIT / PARALLEL — energy must flow (thread pool + GPU-shaped lanes) */
  if (kw(&L->cur,"ASYNC")){
    lex_next(L);
    if (kw(&L->cur,"HTTP") || kw(&L->cur,"GET") || kw(&L->cur,"POST")){
      char method[8]="GET";
      if (kw(&L->cur,"POST")) snprintf(method,sizeof method,"POST");
      if (kw(&L->cur,"HTTP")){
        lex_next(L);
        if (L->cur.kind==TK_IDENT||L->cur.kind==TK_STR){
          snprintf(method,sizeof method,"%s",L->cur.text); lex_next(L);
        }
      } else lex_next(L);
      if (L->cur.kind!=TK_STR){ fail(vm,"ASYNC HTTP method \"url\" …"); return -1; }
      char url[512]; snprintf(url,sizeof url,"%s",L->cur.text); lex_next(L);
      char body[CUBALC_HOST_STR_MAX]; body[0]=0;
      if (kw(&L->cur,"FILE")||kw(&L->cur,"FROM")){
        lex_next(L);
        if (L->cur.kind!=TK_STR){ fail(vm,"ASYNC HTTP FILE \"path\""); return -1; }
        cubalc_host_result fr;
        if (cubalc_host_read(L->cur.text,&fr)!=0){ fail(vm,fr.err[0]?fr.err:"body file"); return -1; }
        snprintf(body,sizeof body,"%s",fr.str); lex_next(L);
      } else if (L->cur.kind==TK_STR){
        snprintf(body,sizeof body,"%s",L->cur.text); lex_next(L);
      }
      int timeout_ms = 120000;
      if (L->cur.kind==TK_NUM){ timeout_ms=(int)L->cur.num; if(timeout_ms<1000) timeout_ms*=1000; lex_next(L); }
      int jid = cubalc_async_http(method, url, body, timeout_ms);
      if (jid < 0){ fail(vm,"ASYNC HTTP submit failed"); return -1; }
      var_set_num(vm, "ASYNC_ID", jid);
      var_set_num(vm, "JOB", jid);
      if (vm->trace) fprintf(vm->trace, "# async job %d submitted (%s)\n", jid, cubalc_async_backend());
      bump(vm); return 1;
    }
    fail(vm,"ASYNC HTTP …"); return -1;
  }
  if (kw(&L->cur,"AWAIT") || kw(&L->cur,"WAIT_JOB")){
    lex_next(L);
    if (kw(&L->cur,"ALL")){
      lex_next(L);
      int ms = 120000;
      if (L->cur.kind==TK_NUM){ ms=(int)L->cur.num; lex_next(L); }
      if (cubalc_async_await_all(ms)!=0){
        if (vm->trace) fprintf(vm->trace,"# await all timeout\n");
        var_set_num(vm,"OK",0);
      } else var_set_num(vm,"OK",1);
      bump(vm); return 1;
    }
    long jid = 0;
    if (L->cur.kind==TK_NUM){ jid=L->cur.num; lex_next(L); }
    else if (L->cur.kind==TK_IDENT){
      /* AWAIT ASYNC_ID or AWAIT JOB */
      for (int i=0;i<vm->n_vars;i++)
        if (strcmp(vm->vars[i].name,L->cur.text)==0){ jid=vm->vars[i].val; break; }
      if (jid==0 && (kw(&L->cur,"ASYNC_ID")||kw(&L->cur,"JOB"))){
        for (int i=0;i<vm->n_vars;i++)
          if (strcmp(vm->vars[i].name,"ASYNC_ID")==0 || strcmp(vm->vars[i].name,"JOB")==0)
            { jid=vm->vars[i].val; break; }
      }
      lex_next(L);
    } else {
      for (int i=0;i<vm->n_vars;i++)
        if (strcmp(vm->vars[i].name,"ASYNC_ID")==0){ jid=vm->vars[i].val; break; }
    }
    int ms = 180000;
    if (L->cur.kind==TK_NUM){ ms=(int)L->cur.num; lex_next(L); }
    cubalc_async_job job;
    memset(&job,0,sizeof job);
    if (cubalc_async_wait((int)jid, ms, &job)!=0){
      var_set_num(vm,"OK",0);
      var_set_num(vm,"HTTP_CODE",0);
      if (vm->trace) fprintf(vm->trace,"# await timeout job %ld\n", jid);
      bump(vm); return 1;
    }
    snprintf(vm->last_str,sizeof vm->last_str,"%s", job.str);
    vm->last_code = job.code;
    vm->last_n = job.n;
    var_set_str(vm,"LAST", job.str);
    var_set_num(vm,"HTTP_CODE", job.code);
    var_set_num(vm,"LAST_N", job.n);
    var_set_num(vm,"OK", job.ok?1:0);
    if (vm->trace) fprintf(vm->trace,"# await job %ld → code %d ok=%d\n", jid, job.code, job.ok);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"PARALLEL") || kw(&L->cur,"PAR")){
    lex_next(L);
    if (kw(&L->cur,"FLOW") || kw(&L->cur,"TICK")){
      lex_next(L);
      int n=8;
      if (L->cur.kind==TK_NUM){ n=(int)L->cur.num; lex_next(L); }
      ensure_world(vm);
      cubalc_async_chain_flow(&vm->ch, n);
      if (vm->trace) fprintf(vm->trace,"# parallel flow %d (%s)\n", n, cubalc_async_backend());
      bump(vm); return 1;
    }
    if (kw(&L->cur,"COMPAT") || kw(&L->cur,"MATRIX")){
      lex_next(L);
      ensure_world(vm);
      float out[CUBALC_MAX_CUBES*CUBALC_MAX_CUBES];
      int n = cubalc_async_compat_batch(&vm->ch, out, CUBALC_MAX_CUBES);
      long avg = 0;
      if (n>0){
        double s=0; int c=0;
        for (int i=0;i<n;i++) for (int j=0;j<n;j++){ s+=out[i*n+j]; c++; }
        avg = c ? (long)lround(100.0*s/c) : 0;
      }
      var_set_num(vm,"COMPAT_AVG", avg);
      var_set_num(vm,"LAST_N", n);
      if (vm->trace) fprintf(vm->trace,"# parallel compat n=%d avg=%ld (%s)\n",
                             n, avg, cubalc_async_backend());
      bump(vm); return 1;
    }
    fail(vm,"PARALLEL FLOW|COMPAT"); return -1;
  }

  return 0; /* not async plane */
}
