/* CubalC lang — lang_ops_core.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"
#if !defined(CUBALC_OS_WINDOWS)
#  include <pwd.h>
#endif

int cubalc_lang_ops_core(VM *vm, Lex *L){
  /* plane ops_core: L3495-4641 */
  skip_nl(L);
  if (L->cur.kind==TK_EOF) return 0;

  /* free-standing look */
  if (L->cur.kind==TK_QMARK){
    lex_next(L);
    do_show(vm, NULL); bump(vm); return 1;
  }
  /* free-standing flow dots: ... or ~~~ */
  if (L->cur.kind==TK_TILDE){
    int n=0;
    while (L->cur.kind==TK_TILDE){ n++; lex_next(L); }
    if (L->cur.kind==TK_NUM){ n=(int)L->cur.num; lex_next(L); }
    if (n<1) n=1;
    do_flow(vm,n); bump(vm); return 1;
  }

  /* primary: cube form */
  if (L->cur.kind==TK_LBRACK)
    return parse_cube(vm, L);

  /* bare hold */
  if (kw(&L->cur,"hold")){
    lex_next(L);
    vm->hold_flash=1; vm->ch.hold_flash=1; bump(vm); return 1;
  }


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

  /* SYS host ops — C runtime (Grokium without Python) */
  if (kw(&L->cur,"SYS") || kw(&L->cur,"HOST")){
    lex_next(L);
    skip_nl(L);
    if (kw(&L->cur,"READ")){
      /* SYS READ [OR|SOFT|TRY] path [OR "fallback"]
       * Soft: optional plate — miss → OK=0 sticky LAST_ERR, no fatal.
       * Fallback: miss → LAST = fallback string, OK=1 (like ENV OR).
       * Hard (default): miss is fatal. */
      int soft = 0;
      char path[512];
      cubalc_host_result hr;
      lex_next(L);
      if (kw(&L->cur,"OR") || kw(&L->cur,"SOFT") || kw(&L->cur,"TRY") ||
          kw(&L->cur,"OPTIONAL") || kw(&L->cur,"MAYBE")){
        soft = 1;
        lex_next(L);
      }
      if (resolve_str_arg(vm, L, path, sizeof path)!=0){
        fail(vm, soft ? "SYS READ OR \"path\"|LAST" : "SYS READ \"path\"|LAST");
        return -1;
      }
      if (cubalc_host_read(path, &hr)!=0){
        char fb[CUBALC_HOST_STR_MAX];
        int have_fb = 0;
        if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
            kw(&L->cur,"FALLBACK")){
          lex_next(L);
          if (resolve_str_arg(vm, L, fb, sizeof fb) == 0)
            have_fb = 1;
          else {
            fail(vm, "SYS READ path OR \"fallback\"");
            return -1;
          }
        }
        if (have_fb) {
          snprintf(vm->last_str, sizeof vm->last_str, "%s", fb);
          vm->last_n = (long)strlen(fb);
          vm->last_code = 0;
          var_set_str(vm, "LAST", fb);
          var_set_num(vm, "LAST_N", vm->last_n);
          var_set_num(vm, "OK", 1);
          var_set_num(vm, "READ_OK", 0); /* content from fallback, not file */
          bump(vm); return 1;
        }
        if (soft) {
          const char *em = hr.err[0] ? hr.err : "SYS READ soft miss";
          var_set_str(vm, "LAST_ERR", em);
          var_set_str(vm, "ERR", em);
          var_set_str(vm, "LAST", "");
          snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
          vm->last_n = 0;
          vm->last_code = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "OK", 0);
          var_set_num(vm, "READ_OK", 0);
          bump(vm); return 1;
        }
        fail(vm, hr.err[0]?hr.err:"SYS READ fail");
        return -1;
      }
      /* success: optional trailing OR ignored (file won) */
      if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
          kw(&L->cur,"FALLBACK")){
        lex_next(L);
        char skip[CUBALC_HOST_STR_MAX];
        (void)resolve_str_arg(vm, L, skip, sizeof skip);
      }
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n; vm->last_code = 0;
      var_set_str(vm, "LAST", hr.str);
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "OK", 1);
      var_set_num(vm, "READ_OK", 1);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"WRITE")){
      lex_next(L);
      char path[512]; path[0]=0;
      if (L->cur.kind==TK_STR){ snprintf(path,sizeof path,"%s",L->cur.text); lex_next(L); }
      else if (L->cur.kind==TK_IDENT){
        if (strcmp(L->cur.text,"LAST")==0) snprintf(path,sizeof path,"%s",vm->last_str);
        else {
          Var *v = var_get(vm, L->cur.text, 0);
          if (v && v->is_str) snprintf(path,sizeof path,"%s",v->sval);
          else { fail(vm,"SYS WRITE path"); return -1; }
        }
        lex_next(L);
      } else { fail(vm,"SYS WRITE \"path\" \"data\""); return -1; }
      const char *data = "";
      char dbuf[CUBALC_HOST_STR_MAX];
      if (L->cur.kind==TK_STR){ snprintf(dbuf,sizeof dbuf,"%s",L->cur.text); data=dbuf; lex_next(L); }
      else if (L->cur.kind==TK_IDENT){
        Var *v = var_get(vm, L->cur.text, 0);
        if (v && v->is_str) data = v->sval;
        else if (strcmp(L->cur.text,"LAST")==0) data = vm->last_str;
        else data = "";
        lex_next(L);
      }
      cubalc_host_result hr;
      if (cubalc_host_write(path, data, &hr)!=0){ fail(vm, hr.err[0]?hr.err:"SYS WRITE fail"); return -1; }
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS RM|UNLINK|DELETE path — remove regular file; missing soft OK (LAST_N=0)
     * Usability: agents clean STATE/TMP plates without shell rm. */
    if (kw(&L->cur,"RM") || kw(&L->cur,"UNLINK") || kw(&L->cur,"DELETE") ||
        kw(&L->cur,"REMOVE") || kw(&L->cur,"DEL")){
      char path[512];
      cubalc_host_result hr;
      lex_next(L);
      if (resolve_str_arg(vm, L, path, sizeof path)!=0){
        fail(vm,"SYS RM \"path\"|LAST"); return -1;
      }
      if (cubalc_host_rm(path, &hr)!=0){
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "RM_N", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
          var_set_str(vm, "LAST", hr.err);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.err);
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", path);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "RM_N", hr.n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS RENAME|MV|MOVE from to — move plate path without shell. */
    if (kw(&L->cur,"RENAME") || kw(&L->cur,"MV") || kw(&L->cur,"MOVE")){
      char from[512], to[512];
      cubalc_host_result hr;
      lex_next(L);
      if (resolve_str_arg(vm, L, from, sizeof from)!=0){
        fail(vm,"SYS RENAME \"from\" \"to\""); return -1;
      }
      if (resolve_str_arg(vm, L, to, sizeof to)!=0){
        fail(vm,"SYS RENAME \"from\" \"to\""); return -1;
      }
      if (cubalc_host_rename(from, to, &hr)!=0){
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
          var_set_str(vm, "LAST", hr.err);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.err);
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", to);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", to);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS COPY|CP|CLONE src dst — duplicate regular file plate.
     * Usability: agents snapshot plates without shell cp. Soft miss on src. */
    if (kw(&L->cur,"COPY") || kw(&L->cur,"CP") || kw(&L->cur,"CLONE") ||
        kw(&L->cur,"FILECOPY") || kw(&L->cur,"FCOPY")){
      char src[512], dst[512];
      cubalc_host_result hr;
      lex_next(L);
      if (resolve_str_arg(vm, L, src, sizeof src)!=0){
        fail(vm,"SYS COPY \"src\" \"dst\""); return -1;
      }
      if (resolve_str_arg(vm, L, dst, sizeof dst)!=0){
        fail(vm,"SYS COPY \"src\" \"dst\""); return -1;
      }
      if (cubalc_host_copy(src, dst, &hr)!=0){
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "COPY_N", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
          var_set_str(vm, "LAST", hr.err);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.err);
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", dst);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", dst);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "COPY_N", hr.n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS REALPATH|ABSPATH|ABS [path|LAST] — absolute path for portable plates.
     * Existing → realpath; missing relative → cwd/path; empty → cwd. */
    if (kw(&L->cur,"REALPATH") || kw(&L->cur,"ABSPATH") || kw(&L->cur,"ABS") ||
        kw(&L->cur,"ABSOLUTE") || kw(&L->cur,"CANON") || kw(&L->cur,"CANONICAL")){
      char path[512];
      cubalc_host_result hr;
      lex_next(L);
      path[0] = 0;
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, path, sizeof path) != 0)
          path[0] = 0;
      }
      /* no arg → LAST if set, else cwd via empty path */
      if (!path[0] && vm->last_str[0])
        snprintf(path, sizeof path, "%s", vm->last_str);
      if (cubalc_host_abspath(path, &hr) != 0) {
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
          var_set_str(vm, "LAST", hr.err);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.err);
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      var_set_str(vm, "REALPATH", hr.str);
      var_set_str(vm, "ABSPATH", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS TOUCH path — create empty file or refresh mtime (plate markers).
     * LAST_N/TOUCH_N: 1 if newly created, 0 if updated existing. Soft fail on dir. */
    if (kw(&L->cur,"TOUCH") || kw(&L->cur,"ENSURE_FILE") || kw(&L->cur,"CREATE") ||
        kw(&L->cur,"MAKEFILE")){
      char path[512];
      cubalc_host_result hr;
      lex_next(L);
      if (resolve_str_arg(vm, L, path, sizeof path)!=0){
        fail(vm,"SYS TOUCH \"path\"|LAST"); return -1;
      }
      if (cubalc_host_touch(path, &hr)!=0){
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "TOUCH_N", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
          var_set_str(vm, "LAST", hr.err);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.err);
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", path);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "TOUCH_N", hr.n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS LIST|LS|LISTDIR path — directory basenames → LAST (newline-joined).
     * LAST_N/LIST_N = count. Soft miss / non-dir. Usability: scan STATE plates. */
    if (kw(&L->cur,"LIST") || kw(&L->cur,"LS") || kw(&L->cur,"LISTDIR") ||
        kw(&L->cur,"DIRLIST") || kw(&L->cur,"SCANDIR") || kw(&L->cur,"READDIR")){
      char path[512];
      cubalc_host_result hr;
      lex_next(L);
      if (resolve_str_arg(vm, L, path, sizeof path)!=0){
        fail(vm,"SYS LIST \"path\"|LAST"); return -1;
      }
      if (cubalc_host_listdir(path, &hr)!=0){
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "LIST_N", 0);
        var_set_str(vm, "LAST", "");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str);
      var_set_str(vm, "LIST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "LIST_N", hr.n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"ENV") || kw(&L->cur,"SETENV") || kw(&L->cur,"EXPORT") ||
        kw(&L->cur,"UNSETENV") || kw(&L->cur,"ENVUNSET")){
      /* SYS ENV name [OR fallback] — get.
       * SYS ENV SET|PUT name value | SYS SETENV|EXPORT — process setenv (agent CUBALC_*).
       * SYS ENV UNSET|CLEAR name | SYS UNSETENV — unsetenv · LAST_N 1 if was set. */
      char op[16]; snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *p = op; *p; p++)
        if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 'a' + 'A');
      int is_set = (strcmp(op, "SETENV") == 0 || strcmp(op, "EXPORT") == 0);
      int is_unset = (strcmp(op, "UNSETENV") == 0 || strcmp(op, "ENVUNSET") == 0);
      lex_next(L);
      if (!is_set && !is_unset && strcmp(op, "ENV") == 0) {
        if (kw(&L->cur,"SET") || kw(&L->cur,"PUT") || kw(&L->cur,"EXPORT") ||
            kw(&L->cur,"WRITE") || kw(&L->cur,"ASSIGN")){
          is_set = 1;
          lex_next(L);
        } else if (kw(&L->cur,"UNSET") || kw(&L->cur,"CLEAR") || kw(&L->cur,"DELETE") ||
                   kw(&L->cur,"RM") || kw(&L->cur,"DROP")){
          is_unset = 1;
          lex_next(L);
        }
      }
      if (is_set) {
        char name[256] = "", val[CUBALC_HOST_STR_MAX];
        cubalc_host_result hr;
        if (resolve_str_arg(vm, L, name, sizeof name) != 0) {
          if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
            snprintf(name, sizeof name, "%s", L->cur.text);
            lex_next(L);
          } else {
            fail(vm, "SYS ENV SET name value"); return -1;
          }
        }
        val[0] = 0;
        if (resolve_str_arg(vm, L, val, sizeof val) != 0) {
          /* bare number → string; else empty */
          if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
            long n = parse_expr(vm, L);
            snprintf(val, sizeof val, "%ld", n);
          } else if (L->cur.kind == TK_IDENT) {
            Var *v = var_get(vm, L->cur.text, 0);
            if (v && !v->is_str) {
              snprintf(val, sizeof val, "%ld", v->val);
              lex_next(L);
            } else {
              fail(vm, "SYS ENV SET name value"); return -1;
            }
          }
          /* else empty value */
        }
        if (cubalc_host_env_set(name, val, &hr) != 0) {
          fail(vm, hr.err[0] ? hr.err : "SYS ENV SET failed"); return -1;
        }
        snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
        vm->last_n = hr.n;
        var_set_str(vm, "LAST", hr.str);
        var_set_num(vm, "LAST_N", hr.n);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      if (is_unset) {
        char name[256] = "";
        cubalc_host_result hr;
        if (resolve_str_arg(vm, L, name, sizeof name) != 0) {
          if (L->cur.kind == TK_IDENT || L->cur.kind == TK_STR) {
            snprintf(name, sizeof name, "%s", L->cur.text);
            lex_next(L);
          } else {
            fail(vm, "SYS ENV UNSET name"); return -1;
          }
        }
        if (cubalc_host_env_unset(name, &hr) != 0) {
          fail(vm, hr.err[0] ? hr.err : "SYS ENV UNSET failed"); return -1;
        }
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        vm->last_n = hr.n;
        var_set_str(vm, "LAST", "");
        var_set_num(vm, "LAST_N", hr.n);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      /* get: SYS ENV name [OR fallback] */
      if (L->cur.kind!=TK_STR && L->cur.kind!=TK_IDENT){ fail(vm,"SYS ENV name"); return -1; }
      cubalc_host_result hr;
      cubalc_host_env(L->cur.text, &hr);
      lex_next(L);
      if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
          kw(&L->cur,"FALLBACK")){
        lex_next(L);
        char fb[512];
        if (resolve_str_arg(vm, L, fb, sizeof fb) != 0){
          fail(vm,"SYS ENV name OR \"fallback\""); return -1;
        }
        if (!hr.str[0] || hr.n <= 0){
          snprintf(hr.str, sizeof hr.str, "%s", fb);
          hr.n = (long)strlen(hr.str);
          hr.ok = 1;
        }
      }
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = hr.n;
      var_set_str(vm, "LAST", hr.str);
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"EXIST") || kw(&L->cur,"EXISTS")){
      lex_next(L);
      char path[512];
      if (resolve_str_arg(vm, L, path, sizeof path)!=0){
        fail(vm,"SYS EXIST \"path\"|LAST"); return -1;
      }
      int e = cubalc_host_exists(path);
      var_set_num(vm, "LAST_N", e);
      var_set_num(vm, "EXIST", e);
      bump(vm); return 1;
    }
    /* SYS SIZE|FSIZE path — regular-file bytes → LAST_N/SIZE; soft miss OK=0
     * SYS ISDIR path — LAST_N 1 if directory
     * SYS ISFILE path — LAST_N 1 if regular file
     * Usability: plate probes without full READ / shell stat. */
    if (kw(&L->cur,"SIZE") || kw(&L->cur,"FSIZE") || kw(&L->cur,"FILESIZE") ||
        kw(&L->cur,"BYTES") || kw(&L->cur,"ISDIR") || kw(&L->cur,"IS_DIR") ||
        kw(&L->cur,"ISFILE") || kw(&L->cur,"IS_FILE") || kw(&L->cur,"ISREG")){
      int want_size = (kw(&L->cur,"SIZE") || kw(&L->cur,"FSIZE") ||
                      kw(&L->cur,"FILESIZE") || kw(&L->cur,"BYTES"));
      int want_dir = (kw(&L->cur,"ISDIR") || kw(&L->cur,"IS_DIR"));
      int want_file = (kw(&L->cur,"ISFILE") || kw(&L->cur,"IS_FILE") ||
                      kw(&L->cur,"ISREG"));
      char path[512];
      cubalc_host_result hr;
      lex_next(L);
      if (resolve_str_arg(vm, L, path, sizeof path)!=0){
        fail(vm, want_size ? "SYS SIZE \"path\"|LAST"
                           : (want_dir ? "SYS ISDIR \"path\"|LAST"
                                       : "SYS ISFILE \"path\"|LAST"));
        return -1;
      }
      cubalc_host_path_kind(path, &hr);
      if (want_size) {
        if (hr.code == 0) {
          var_set_num(vm, "OK", 0);
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "SIZE", 0);
          if (hr.err[0]) {
            var_set_str(vm, "LAST_ERR", hr.err);
            var_set_str(vm, "ERR", hr.err);
            var_set_str(vm, "LAST", hr.err);
            snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.err);
          }
        } else if (hr.code == 1) {
          /* regular file — report bytes */
          vm->last_n = hr.n;
          var_set_num(vm, "LAST_N", hr.n);
          var_set_num(vm, "SIZE", hr.n);
          var_set_num(vm, "OK", 1);
          var_set_str(vm, "LAST", path);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
        } else if (hr.code == 2) {
          /* directory — size 0, still OK (exists as dir) */
          vm->last_n = 0;
          var_set_num(vm, "LAST_N", 0);
          var_set_num(vm, "SIZE", 0);
          var_set_num(vm, "OK", 1);
          var_set_str(vm, "LAST", path);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
        } else {
          vm->last_n = hr.n;
          var_set_num(vm, "LAST_N", hr.n);
          var_set_num(vm, "SIZE", hr.n);
          var_set_num(vm, "OK", 1);
          var_set_str(vm, "LAST", path);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
        }
        bump(vm); return 1;
      }
      if (want_dir) {
        long v = (hr.code == 2) ? 1 : 0;
        var_set_num(vm, "LAST_N", v);
        var_set_num(vm, "ISDIR", v);
        var_set_num(vm, "OK", 1);
        vm->last_n = v;
        bump(vm); return 1;
      }
      if (want_file) {
        long v = (hr.code == 1) ? 1 : 0;
        var_set_num(vm, "LAST_N", v);
        var_set_num(vm, "ISFILE", v);
        var_set_num(vm, "OK", 1);
        vm->last_n = v;
        bump(vm); return 1;
      }
      bump(vm); return 1;
    }
    /* SYS MTIME|MODTIME path — st_mtime epoch seconds → LAST_N/MTIME; soft miss OK=0.
     * SYS AGE|FILEAGE path — now - mtime seconds → LAST_N/AGE (0 if miss or future).
     * Usability: lease/plate freshness without shell stat. */
    if (kw(&L->cur,"MTIME") || kw(&L->cur,"MODTIME") || kw(&L->cur,"MODIFIED") ||
        kw(&L->cur,"FILETIME") || kw(&L->cur,"AGE") || kw(&L->cur,"FILEAGE") ||
        kw(&L->cur,"AGESEC") || kw(&L->cur,"STALE")){
      int want_age = (kw(&L->cur,"AGE") || kw(&L->cur,"FILEAGE") ||
                      kw(&L->cur,"AGESEC") || kw(&L->cur,"STALE"));
      char path[512];
      cubalc_host_result hr;
      lex_next(L);
      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, want_age ? "SYS AGE \"path\"|LAST" : "SYS MTIME \"path\"|LAST");
        return -1;
      }
      if (cubalc_host_mtime(path, &hr) != 0 || !hr.ok) {
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "MTIME", 0);
        var_set_num(vm, "AGE", 0);
        vm->last_n = 0;
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
          var_set_str(vm, "LAST", "");
          snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        }
        bump(vm); return 1;
      }
      if (want_age) {
        long now = (long)time(NULL);
        long age = now - hr.n;
        if (age < 0) age = 0;
        vm->last_n = age;
        var_set_num(vm, "LAST_N", age);
        var_set_num(vm, "AGE", age);
        var_set_num(vm, "MTIME", hr.n);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", path);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
      } else {
        char buf[40];
        snprintf(buf, sizeof buf, "%ld", hr.n);
        vm->last_n = hr.n;
        var_set_num(vm, "LAST_N", hr.n);
        var_set_num(vm, "MTIME", hr.n);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", buf);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      }
      bump(vm); return 1;
    }
    /* SYS MKDIR|MAKEDIR path — mkdir -p; OK if dir already exists.
     * Usability: agents create STATE/TMP plate trees without shell. */
    if (kw(&L->cur,"MKDIR") || kw(&L->cur,"MAKEDIR") || kw(&L->cur,"MAKE_DIR") ||
        kw(&L->cur,"MD") || kw(&L->cur,"ENSURE_DIR")){
      char path[512];
      cubalc_host_result hr;
      lex_next(L);
      if (resolve_str_arg(vm, L, path, sizeof path)!=0){
        fail(vm,"SYS MKDIR \"path\"|LAST"); return -1;
      }
      if (cubalc_host_mkdir(path, &hr)!=0){
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
          var_set_str(vm, "LAST", hr.err);
          snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.err);
        }
        bump(vm); return 1;
      }
      var_set_str(vm, "LAST", hr.str[0] ? hr.str : path);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str[0] ? hr.str : path);
      vm->last_n = hr.n;
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "MKDIR_N", hr.n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS BASENAME|LEAF path — final path component → LAST/BASENAME
     * SYS DIRNAME|PARENT path — parent directory → LAST/DIRNAME
     * Usability: split JOIN/TMP/WHICH paths without shell basename(1). */
    if (kw(&L->cur,"BASENAME") || kw(&L->cur,"LEAF") || kw(&L->cur,"FILENAME") ||
        kw(&L->cur,"DIRNAME") || kw(&L->cur,"PARENT") || kw(&L->cur,"DIR")){
      int want_base = (kw(&L->cur,"BASENAME") || kw(&L->cur,"LEAF") ||
                      kw(&L->cur,"FILENAME"));
      char path[512], out[512];
      const char *slash;
      size_t n;
      lex_next(L);
      if (resolve_str_arg(vm, L, path, sizeof path)!=0){
        fail(vm, want_base ? "SYS BASENAME \"path\"|LAST"
                           : "SYS DIRNAME \"path\"|LAST");
        return -1;
      }
      /* strip trailing separators (keep single root) */
      n = strlen(path);
      while (n > 1 && (path[n - 1] == '/' || path[n - 1] == '\\')) {
        path[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(path);
      if (want_base) {
        if (slash && slash[1])
          snprintf(out, sizeof out, "%s", slash + 1);
        else if (slash && (slash[0] == '/' || slash[0] == '\\') && !slash[1])
          snprintf(out, sizeof out, "%s", path); /* "/" → "/" */
        else
          snprintf(out, sizeof out, "%s", path[0] ? path : ".");
        var_set_str(vm, "BASENAME", out);
      } else {
        if (slash && slash != path) {
          size_t dn = (size_t)(slash - path);
          if (dn >= sizeof out) dn = sizeof out - 1;
          memcpy(out, path, dn);
          out[dn] = 0;
        } else if (slash && slash == path) {
          /* absolute at root */
          snprintf(out, sizeof out, "%c", path[0] == '\\' ? '\\' : '/');
        } else {
          snprintf(out, sizeof out, ".");
        }
        var_set_str(vm, "DIRNAME", out);
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)strlen(out);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS EXTNAME|EXT|SUFFIX path — final extension with leading '.' → LAST/EXT
     * SYS STEM path — basename without extension → LAST/STEM
     * Usability: peel JOIN/WHICH paths for plate stems and type tags. */
    if (kw(&L->cur,"EXTNAME") || kw(&L->cur,"EXT") || kw(&L->cur,"SUFFIX") ||
        kw(&L->cur,"EXTENSION") || kw(&L->cur,"STEM") || kw(&L->cur,"ROOTNAME")){
      int want_ext = (kw(&L->cur,"EXTNAME") || kw(&L->cur,"EXT") ||
                     kw(&L->cur,"SUFFIX") || kw(&L->cur,"EXTENSION"));
      char path[512], leaf[512], out[512];
      const char *slash, *dot, *base;
      size_t n;
      lex_next(L);
      if (resolve_str_arg(vm, L, path, sizeof path)!=0){
        fail(vm, want_ext ? "SYS EXTNAME \"path\"|LAST"
                          : "SYS STEM \"path\"|LAST");
        return -1;
      }
      /* strip trailing separators */
      n = strlen(path);
      while (n > 1 && (path[n - 1] == '/' || path[n - 1] == '\\')) {
        path[n - 1] = 0;
        n--;
      }
      slash = cubalc_path_slash(path);
      if (slash && slash[1])
        snprintf(leaf, sizeof leaf, "%s", slash + 1);
      else if (slash && (slash[0] == '/' || slash[0] == '\\') && !slash[1])
        snprintf(leaf, sizeof leaf, "%s", path);
      else
        snprintf(leaf, sizeof leaf, "%s", path[0] ? path : ".");
      base = leaf;
      /* last '.' that is not the first char (dotfiles have no extension) */
      dot = strrchr(leaf, '.');
      if (dot && dot != leaf) {
        if (want_ext)
          snprintf(out, sizeof out, "%s", dot);
        else {
          size_t sn = (size_t)(dot - leaf);
          if (sn >= sizeof out) sn = sizeof out - 1;
          memcpy(out, leaf, sn);
          out[sn] = 0;
        }
      } else {
        if (want_ext)
          out[0] = 0;
        else
          snprintf(out, sizeof out, "%s", base);
      }
      if (want_ext) {
        var_set_str(vm, "EXTNAME", out);
        var_set_str(vm, "EXT", out);
      } else {
        var_set_str(vm, "STEM", out);
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)strlen(out);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"WHICH")){
      lex_next(L);
      if (L->cur.kind!=TK_STR && L->cur.kind!=TK_IDENT){ fail(vm,"SYS WHICH name"); return -1; }
      cubalc_host_result hr;
      /* Usability: PATH bins first, then CubalC lib/program (host_which). */
      if (cubalc_host_which(L->cur.text, &hr)!=0){
        vm->last_str[0] = 0;
        vm->last_n = 0;
        var_set_str(vm, "LAST", "");
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
      } else {
        snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
        vm->last_n = 1;
        var_set_str(vm, "LAST", hr.str);
        var_set_num(vm, "LAST_N", 1);
        var_set_num(vm, "OK", 1);
      }
      lex_next(L);
      bump(vm); return 1;
    }
    /* SYS CWD|PWD — process working directory → LAST
     * SYS STATE — CUBALC_STATE plate dir (default state) → LAST
     * SYS ROOT — CUBALC_ROOT or cwd (INCLUDE root) → LAST
     * Usability: portable scripts locate layout without shell glue. */
    if (kw(&L->cur,"CWD") || kw(&L->cur,"PWD") || kw(&L->cur,"WORKDIR")){
      char cwd[512];
      lex_next(L);
      if (!getcwd(cwd, sizeof cwd)) {
        snprintf(cwd, sizeof cwd, ".");
        var_set_num(vm, "OK", 0);
      } else {
        var_set_num(vm, "OK", 1);
      }
      snprintf(vm->last_str, sizeof vm->last_str, "%s", cwd);
      vm->last_n = (long)strlen(cwd);
      var_set_str(vm, "LAST", cwd);
      var_set_str(vm, "CWD", cwd);
      var_set_num(vm, "LAST_N", vm->last_n);
      bump(vm); return 1;
    }
    /* SYS CHDIR|CD path — change process working directory → LAST = new cwd.
     * LAST_N = 1 success, 0 soft miss (cwd unchanged). Updates CWD var.
     * Usability: relative READ/WRITE/LIST under STATE/TMP without shell cd. */
    if (kw(&L->cur,"CHDIR") || kw(&L->cur,"CD") || kw(&L->cur,"CHDIRTO") ||
        kw(&L->cur,"SETCWD") || kw(&L->cur,"CHANGE_DIR") || kw(&L->cur,"CHANGEDIR")){
      char path[512], cwd[512];
      int soft = 0, ok = 0;
      lex_next(L);
      if (kw(&L->cur,"OR") || kw(&L->cur,"SOFT") || kw(&L->cur,"OPTIONAL") ||
          kw(&L->cur,"TRY") || kw(&L->cur,"MAYBE")){
        soft = 1;
        lex_next(L);
      }
      path[0] = 0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0)
        snprintf(path, sizeof path, "%s", vm->last_str);
      if (!path[0]) {
        ok = 0;
      } else {
#if defined(CUBALC_OS_WINDOWS)
        ok = (_chdir(path) == 0) ? 1 : 0;
#else
        ok = (chdir(path) == 0) ? 1 : 0;
#endif
      }
      if (!getcwd(cwd, sizeof cwd))
        snprintf(cwd, sizeof cwd, ok ? path : ".");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", cwd);
      var_set_str(vm, "LAST", cwd);
      var_set_str(vm, "CWD", cwd);
      vm->last_n = ok ? 1 : 0;
      var_set_num(vm, "LAST_N", ok ? 1 : 0);
      var_set_num(vm, "CHDIR_N", ok ? 1 : 0);
      var_set_num(vm, "OK", ok ? 1 : 0);
      if (!ok && !soft) {
        /* sticky soft err for agents; not fatal (plate continues) */
        char em[180];
        snprintf(em, sizeof em, "CHDIR miss: %s", path[0] ? path : "(empty)");
        var_set_str(vm, "LAST_ERR", em);
        var_set_str(vm, "ERR", em);
      }
      bump(vm); return 1;
    }
    if (kw(&L->cur,"STATE") || kw(&L->cur,"STATEDIR") || kw(&L->cur,"STATE_DIR")){
      char sdir[512];
      const char *e;
      lex_next(L);
      e = getenv("CUBALC_STATE");
      if (e && e[0]) snprintf(sdir, sizeof sdir, "%s", e);
      else snprintf(sdir, sizeof sdir, "state");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", sdir);
      vm->last_n = (long)strlen(sdir);
      var_set_str(vm, "LAST", sdir);
      var_set_str(vm, "STATE_DIR", sdir);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"ROOT") || kw(&L->cur,"CUBALC_ROOT") || kw(&L->cur,"INSTALL_ROOT")){
      char root[512];
      const char *e;
      lex_next(L);
      e = getenv("CUBALC_ROOT");
      if (e && e[0]) {
        snprintf(root, sizeof root, "%s", e);
      } else if (!getcwd(root, sizeof root)) {
        snprintf(root, sizeof root, ".");
      }
      snprintf(vm->last_str, sizeof vm->last_str, "%s", root);
      vm->last_n = (long)strlen(root);
      var_set_str(vm, "LAST", root);
      var_set_str(vm, "ROOT", root);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS TMP|TEMP|TMPDIR — portable temp directory for agent plate writes.
     * Prefer TMPDIR / TMP / TEMP env, else /tmp (POSIX) or "." (fallback).
     * Usability: SYS JOIN TMP "plate.json" without shell $TMPDIR glue. */
    if (kw(&L->cur,"TMP") || kw(&L->cur,"TEMP") || kw(&L->cur,"TMPDIR") ||
        kw(&L->cur,"TEMPDIR") || kw(&L->cur,"TEMP_DIR") || kw(&L->cur,"TMP_DIR")){
      char tdir[512];
      const char *e;
      lex_next(L);
      tdir[0] = 0;
      e = getenv("TMPDIR");
      if (!e || !e[0]) e = getenv("TMP");
      if (!e || !e[0]) e = getenv("TEMP");
      if (!e || !e[0]) e = getenv("CUBALC_TMP");
      if (e && e[0]) snprintf(tdir, sizeof tdir, "%s", e);
#if defined(CUBALC_OS_WINDOWS)
      if (!tdir[0]) {
        e = getenv("LOCALAPPDATA");
        if (e && e[0]) snprintf(tdir, sizeof tdir, "%s\\Temp", e);
        else snprintf(tdir, sizeof tdir, ".");
      }
#else
      if (!tdir[0]) snprintf(tdir, sizeof tdir, "/tmp");
#endif
      /* strip trailing slash (keep root /) */
      {
        size_t n = strlen(tdir);
        while (n > 1 && (tdir[n - 1] == '/' || tdir[n - 1] == '\\')) {
          tdir[n - 1] = 0;
          n--;
        }
      }
      snprintf(vm->last_str, sizeof vm->last_str, "%s", tdir);
      vm->last_n = (long)strlen(tdir);
      var_set_str(vm, "LAST", tdir);
      var_set_str(vm, "TMP", tdir);
      var_set_str(vm, "TMPDIR", tdir);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"HTTP") || kw(&L->cur,"GET") || kw(&L->cur,"POST")){
      char method[8] = "GET";
      if (kw(&L->cur,"POST")) snprintf(method,sizeof method,"POST");
      if (kw(&L->cur,"HTTP")){ lex_next(L); if (L->cur.kind==TK_IDENT||L->cur.kind==TK_STR){ snprintf(method,sizeof method,"%s",L->cur.text); lex_next(L);} }
      else lex_next(L);
      if (L->cur.kind!=TK_STR){ fail(vm,"SYS HTTP method \"url\" [\"body\"|FILE \"path\"]"); return -1; }
      char url[512]; snprintf(url,sizeof url,"%s",L->cur.text); lex_next(L);
      char body[CUBALC_HOST_STR_MAX]; body[0]=0;
      if (kw(&L->cur,"FILE") || kw(&L->cur,"FROM")){
        lex_next(L);
        if (L->cur.kind!=TK_STR){ fail(vm,"SYS HTTP ... FILE \"path\""); return -1; }
        cubalc_host_result fr;
        if (cubalc_host_read(L->cur.text, &fr)!=0){ fail(vm, fr.err[0]?fr.err:"body file"); return -1; }
        snprintf(body,sizeof body,"%s", fr.str);
        lex_next(L);
      } else if (L->cur.kind==TK_STR){
        snprintf(body,sizeof body,"%s",L->cur.text); lex_next(L);
      }
      cubalc_host_result hr;
      int rc = cubalc_host_http(method, url, body, &hr);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_code = hr.code;
      vm->last_n = hr.n;
      var_set_str(vm, "LAST", hr.str);
      var_set_num(vm, "HTTP_CODE", hr.code);
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "OK", hr.ok ? 1 : 0);
      if (rc!=0 && !hr.ok){
        /* soft-fail for probe: OK=0 not fatal */
        if (vm->trace) fprintf(vm->trace, "# http soft-fail %s\n", hr.err);
      }
      bump(vm); return 1;
    }
    if (kw(&L->cur,"SPAWN")){
      lex_next(L);
      if (L->cur.kind!=TK_STR && L->cur.kind!=TK_IDENT){ fail(vm,"SYS SPAWN bin [args…]"); return -1; }
      char bin[512];
      if (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT){
        /* if not path, which */
        if (strchr(L->cur.text, '/')) snprintf(bin,sizeof bin,"%s",L->cur.text);
        else {
          cubalc_host_result wh;
          if (cubalc_host_which(L->cur.text, &wh)==0) snprintf(bin,sizeof bin,"%s",wh.str);
          else snprintf(bin,sizeof bin,"%s",L->cur.text);
        }
        lex_next(L);
      }
      char *av[16]; char abuf[16][256]; int ac=0;
      av[ac++] = bin;
      while (ac < 15 && (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT || L->cur.kind==TK_NUM)){
        snprintf(abuf[ac], sizeof abuf[ac], "%s", L->cur.text);
        av[ac] = abuf[ac];
        ac++; lex_next(L);
      }
      av[ac] = NULL;
      cubalc_host_result hr;
      cubalc_host_spawn(bin, av, &hr);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      var_set_str(vm, "LAST", hr.str);
      var_set_num(vm, "LAST_N", hr.n);
      var_set_num(vm, "EXIT", hr.code);
      var_set_num(vm, "OK", hr.ok ? 1 : 0);
      bump(vm); return 1;
    }
    /* SYS JOIN|PATH a b → LAST = a/b (portable path join, no double slash).
     * Usability: build plate paths from STATE/TMP/LIST names without shell. */
    if (kw(&L->cur,"JOIN") || kw(&L->cur,"PATH") || kw(&L->cur,"PATHJOIN") ||
        kw(&L->cur,"JOINPATH")){
      lex_next(L);
      char a[512]="", b[512]="";
      cubalc_host_result hr;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0){
        fail(vm,"SYS JOIN a b"); return -1;
      }
      if (resolve_str_arg(vm, L, b, sizeof b) != 0){
        fail(vm,"SYS JOIN a b"); return -1;
      }
      if (cubalc_host_join(a,b,&hr)!=0){ fail(vm, hr.err[0]?hr.err:"JOIN"); return -1; }
      snprintf(vm->last_str,sizeof vm->last_str,"%s",hr.str);
      vm->last_n = hr.n;
      var_set_str(vm,"LAST",hr.str);
      var_set_str(vm,"JOIN",hr.str);
      var_set_num(vm,"LAST_N",hr.n);
      var_set_num(vm,"OK",1);
      bump(vm); return 1;
    }
    /* SYS JSON "key" [from LAST] → LAST = field */
    if (kw(&L->cur,"JSON") || kw(&L->cur,"JGET")){
      lex_next(L);
      char key[96]="content";
      if (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT){
        snprintf(key,sizeof key,"%s",L->cur.text); lex_next(L);
      }
      const char *src = vm->last_str;
      cubalc_host_result hr;
      if (cubalc_host_json_get(src, key, &hr)!=0){
        var_set_str(vm,"LAST","");
        var_set_num(vm,"LAST_N",0);
        var_set_num(vm,"OK",0);
        bump(vm); return 1;
      }
      snprintf(vm->last_str,sizeof vm->last_str,"%s",hr.str);
      vm->last_n = hr.n;
      var_set_str(vm,"LAST",hr.str);
      var_set_num(vm,"LAST_N",hr.n);
      var_set_num(vm,"OK",1);
      bump(vm); return 1;
    }
    /* SYS CHAT "local"|"grok" ["model"] — msg from GROKIUM_MSG / CUBALC_MSG env */
    if (kw(&L->cur,"CHAT") || kw(&L->cur,"ASK")){
      lex_next(L);
      char be[32]="local", model[128]="";
      if (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT){
        snprintf(be,sizeof be,"%s",L->cur.text); lex_next(L);
      }
      if (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT){
        /* model or message string */
        if (strcmp(L->cur.text,"local")==0 || strcmp(L->cur.text,"grok")==0 ||
            strncmp(L->cur.text,"grok",4)==0 || strstr(L->cur.text,"/") ||
            strstr(L->cur.text,".gguf")) {
          snprintf(model,sizeof model,"%s",L->cur.text); lex_next(L);
        }
      }
      char msg[2000]; msg[0]=0;
      if (L->cur.kind==TK_STR){
        snprintf(msg,sizeof msg,"%s",L->cur.text); lex_next(L);
      } else {
        const char *e = getenv("GROKIUM_MSG");
        if (!e || !e[0]) e = getenv("CUBALC_MSG");
        if (e) snprintf(msg,sizeof msg,"%s",e);
      }
      if (!msg[0]){ fail(vm,"SYS CHAT needs msg or GROKIUM_MSG"); return -1; }
      if (!model[0]){
        const char *em = getenv("GROKIUM_MODEL");
        if (em && em[0]) snprintf(model,sizeof model,"%s",em);
      }
      const char *st = getenv("CUBALC_STATE");
      cubalc_host_result hr;
      if (cubalc_host_chat(be, model, msg, st, &hr)!=0){
        var_set_str(vm,"LAST", hr.err[0]?hr.err:"chat fail");
        var_set_num(vm,"OK",0);
        var_set_num(vm,"HTTP_CODE", hr.code);
        if (vm->trace) fprintf(vm->trace, "# chat fail %s\n", hr.err);
        bump(vm); return 1; /* soft */
      }
      snprintf(vm->last_str,sizeof vm->last_str,"%s",hr.str);
      vm->last_n = hr.n;
      vm->last_code = hr.code;
      var_set_str(vm,"LAST",hr.str);
      var_set_num(vm,"LAST_N",hr.n);
      var_set_num(vm,"HTTP_CODE",hr.code);
      var_set_num(vm,"OK",1);
      bump(vm); return 1;
    }
    /* SYS ARG n|name [OR "fallback"] — CUBALC_ARGn / named env with default */
    if (kw(&L->cur,"ARG") || kw(&L->cur,"ARGV")){
      lex_next(L);
      char name[64];
      if (L->cur.kind==TK_NUM){
        snprintf(name,sizeof name,"CUBALC_ARG%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT){
        if (strcmp(L->cur.text,"MSG")==0) snprintf(name,sizeof name,"GROKIUM_MSG");
        else if (strcmp(L->cur.text,"BACKEND")==0) snprintf(name,sizeof name,"GROKIUM_BACKEND");
        else if (strcmp(L->cur.text,"MODEL")==0) snprintf(name,sizeof name,"GROKIUM_MODEL");
        else snprintf(name,sizeof name,"%s", L->cur.text);
        lex_next(L);
      } else { fail(vm,"SYS ARG n|name [OR fallback]"); return -1; }
      cubalc_host_result hr;
      cubalc_host_env(name, &hr);
      if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
          kw(&L->cur,"FALLBACK")){
        lex_next(L);
        char fb[512];
        if (resolve_str_arg(vm, L, fb, sizeof fb) != 0){
          fail(vm,"SYS ARG n|name OR \"fallback\""); return -1;
        }
        if (!hr.str[0] || hr.n <= 0){
          snprintf(hr.str, sizeof hr.str, "%s", fb);
          hr.n = (long)strlen(hr.str);
          hr.ok = 1;
        }
      }
      snprintf(vm->last_str,sizeof vm->last_str,"%s",hr.str);
      vm->last_n = hr.n;
      var_set_str(vm,"LAST",hr.str);
      var_set_num(vm,"LAST_N",hr.n);
      var_set_num(vm,"OK", hr.n > 0 ? 1 : 0);
      bump(vm); return 1;
    }
    /* SYS NUM|INT — parse LAST as integer → LAST_N (CubeBrain digit fold) */
    if (kw(&L->cur,"NUM") || kw(&L->cur,"INT") || kw(&L->cur,"ATOI")){
      lex_next(L);
      const char *s = vm->last_str;
      if (L->cur.kind==TK_STR){ s = L->cur.text; lex_next(L); }
      else if (L->cur.kind==TK_IDENT && strcmp(L->cur.text,"LAST")!=0){
        Var *v = var_get(vm, L->cur.text, 0);
        if (v && v->is_str) s = v->sval;
        else if (v) { var_set_num(vm,"LAST_N", v->val); vm->last_n = v->val; bump(vm); return 1; }
        lex_next(L);
      } else if (L->cur.kind==TK_IDENT && strcmp(L->cur.text,"LAST")==0){
        lex_next(L);
      }
      long n = 0;
      if (s && s[0]) n = strtol(s, NULL, 10);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS STR|ITOA|NUMSTR|TOSTR [expr|LAST_N] — integer → decimal string LAST.
     * Usability: dual of SYS NUM; fill REPLACEALL templates {{COUNT}} without shell. */
    if (kw(&L->cur,"STR") || kw(&L->cur,"ITOA") || kw(&L->cur,"NUMSTR") ||
        kw(&L->cur,"TOSTR") || kw(&L->cur,"STRNUM") || kw(&L->cur,"DECIMAL")){
      lex_next(L);
      long n = vm->last_n;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE"))){
        n = parse_expr(vm, L);
      }
      char buf[40];
      snprintf(buf, sizeof buf, "%ld", n);
      var_set_str(vm, "LAST", buf);
      var_set_str(vm, "STR", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"LEN") || kw(&L->cur,"LENGTH") || kw(&L->cur,"STRLEN")){
      lex_next(L);
      long n = 0;
      if (L->cur.kind==TK_STR){ n = (long)strlen(L->cur.text); lex_next(L); }
      else if (L->cur.kind==TK_IDENT){
        if (strcmp(L->cur.text,"LAST")==0){ n = (long)strlen(vm->last_str); lex_next(L); }
        else {
          Var *v = var_get(vm, L->cur.text, 0);
          if (v && v->is_str) n = (long)strlen(v->sval);
          else if (v) n = v->val;
          lex_next(L);
        }
      } else n = (long)strlen(vm->last_str);
      vm->last_n = n; var_set_num(vm, "LAST_N", n); var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS EMPTY|ISEMPTY [str|LAST] — LAST_N 1 if zero-length string.
     * SYS BLANK|ISBLANK|WS — LAST_N 1 if empty or only space/tab/CR/LF.
     * SYS NONEMPTY|NONEMPTY — invert of EMPTY (1 if any char).
     * Usability: soft plate / optional config IF without LEN + compare glue. */
    if (kw(&L->cur,"EMPTY") || kw(&L->cur,"ISEMPTY") || kw(&L->cur,"ISEMPTYSTR") ||
        kw(&L->cur,"BLANK") || kw(&L->cur,"ISBLANK") || kw(&L->cur,"WS") ||
        kw(&L->cur,"ISWS") || kw(&L->cur,"WHITESPACE") ||
        kw(&L->cur,"NONEMPTY") || kw(&L->cur,"NOTEMPTY") || kw(&L->cur,"HASCHARS")){
      char op[16]; snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      int want_blank = (strcmp(op, "BLANK") == 0 || strcmp(op, "ISBLANK") == 0 ||
                        strcmp(op, "WS") == 0 || strcmp(op, "ISWS") == 0 ||
                        strcmp(op, "WHITESPACE") == 0);
      int want_non = (strcmp(op, "NONEMPTY") == 0 || strcmp(op, "NOTEMPTY") == 0 ||
                      strcmp(op, "HASCHARS") == 0);
      char s[CUBALC_HOST_STR_MAX];
      long hit = 0;
      lex_next(L);
      s[0] = 0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      if (want_blank) {
        const char *p = s;
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        hit = (*p == 0) ? 1 : 0;
      } else if (want_non) {
        hit = (s[0] != 0) ? 1 : 0;
      } else {
        hit = (s[0] == 0) ? 1 : 0;
      }
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "EMPTY_N", hit);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"TIME") || kw(&L->cur,"NOW") || kw(&L->cur,"EPOCH")){
      lex_next(L);
      long n = (long)time(NULL);
      vm->last_n = n; var_set_num(vm, "LAST_N", n); var_set_num(vm, "TIME", n); var_set_num(vm, "OK", 1);
      char buf[32]; snprintf(buf, sizeof buf, "%ld", n);
      var_set_str(vm, "LAST", buf); snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS MS|MILLIS|TIME_MS|EPOCH_MS — wall clock milliseconds since epoch.
     * Usability: agents need finer stamps than SYS TIME (seconds) for plates/timing. */
    if (kw(&L->cur,"MS") || kw(&L->cur,"MILLIS") || kw(&L->cur,"MILLISECONDS") ||
        kw(&L->cur,"TIME_MS") || kw(&L->cur,"EPOCH_MS") || kw(&L->cur,"NOW_MS")){
      struct timespec ts;
      long n;
      char buf[40];
      lex_next(L);
      if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        n = (long)time(NULL) * 1000L;
      } else {
        n = (long)ts.tv_sec * 1000L + (long)(ts.tv_nsec / 1000000L);
      }
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "MS", n);
      var_set_num(vm, "TIME_MS", n);
      var_set_num(vm, "OK", 1);
      snprintf(buf, sizeof buf, "%ld", n);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS SLEEP|MSLEEP|DELAY [MS] n — pause n milliseconds (cap 60s).
     * Usability: agent poll/backoff without shell sleep; pairs with SYS MS. */
    if (kw(&L->cur,"SLEEP") || kw(&L->cur,"MSLEEP") || kw(&L->cur,"DELAY") ||
        kw(&L->cur,"PAUSE") || kw(&L->cur,"WAIT_MS")){
      long ms = 0;
      char buf[40];
      lex_next(L);
      /* optional unit keyword MS|MILLIS before or after value is ignored as unit */
      if (kw(&L->cur,"MS") || kw(&L->cur,"MILLIS") || kw(&L->cur,"MILLISECONDS"))
        lex_next(L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
          L->cur.kind==TK_MINUS)
        ms = parse_expr(vm, L);
      else
        ms = 0;
      if (kw(&L->cur,"MS") || kw(&L->cur,"MILLIS") || kw(&L->cur,"MILLISECONDS"))
        lex_next(L);
      if (ms < 0) ms = 0;
      if (ms > 60000) ms = 60000; /* hard cap — avoid hung agents */
      if (ms > 0) {
        struct timespec ts;
        ts.tv_sec = ms / 1000;
        ts.tv_nsec = (ms % 1000) * 1000000L;
        nanosleep(&ts, NULL);
      }
      vm->last_n = ms;
      var_set_num(vm, "LAST_N", ms);
      var_set_num(vm, "SLEEP_MS", ms);
      var_set_num(vm, "OK", 1);
      snprintf(buf, sizeof buf, "%ld", ms);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS RAND|RANDOM [n] | [lo hi] — uniform integer for agent jitter/sampling.
     * No args → 0..999999; one arg n>0 → 0..n-1; two args → inclusive [lo,hi].
     * Seed: once per process via time^pid (CUBALC_SEED override if set numeric).
     * Distinct from cube-plane RAND (stack/cell). Usability: backoff jitter without shell. */
    if (kw(&L->cur,"RAND") || kw(&L->cur,"RANDOM") || kw(&L->cur,"RND") ||
        kw(&L->cur,"IRAND") || kw(&L->cur,"RANDINT") || kw(&L->cur,"URAND")){
      static int seeded = 0;
      long a = -1, b = -1, out = 0, span;
      int has_a = 0, has_b = 0;
      char buf[40];
      const char *se;
      lex_next(L);
      /* Track presence separately: single negative arg must not look like "no args"
       * (old a<0&&b<0 sentinel treated SYS RAND -3 as default 0..999999). */
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
          L->cur.kind==TK_MINUS) {
        a = parse_expr(vm, L);
        has_a = 1;
      }
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
          L->cur.kind==TK_MINUS) {
        b = parse_expr(vm, L);
        has_b = 1;
      }
      if (!seeded) {
        unsigned long s = (unsigned long)time(NULL);
#if !defined(CUBALC_OS_WINDOWS)
        s ^= (unsigned long)getpid() << 16;
#endif
        se = getenv("CUBALC_SEED");
        if (se && se[0]) {
          char *end = 0;
          unsigned long v = strtoul(se, &end, 0);
          if (end && end != se) s = v;
        }
        srand((unsigned)(s & 0xffffffffu));
        seeded = 1;
      }
      if (!has_a) {
        /* default wide range */
        out = (long)(rand() % 1000000);
      } else if (!has_b) {
        /* single arg: 0..a-1; non-positive → 0 */
        if (a <= 0) out = 0;
        else out = (long)(rand() % (int)(a > 0x7fffffffL ? 0x7fffffffL : a));
      } else {
        long lo = a, hi = b;
        if (lo > hi) { long t = lo; lo = hi; hi = t; }
        span = hi - lo + 1;
        if (span <= 0) out = lo;
        else if (span > 0x7fffffffL) {
          out = lo + (long)(rand() % 0x7fffffff);
        } else {
          out = lo + (long)(rand() % (int)span);
        }
      }
      vm->last_n = out;
      var_set_num(vm, "LAST_N", out);
      var_set_num(vm, "RAND", out);
      var_set_num(vm, "RANDOM", out);
      var_set_num(vm, "OK", 1);
      snprintf(buf, sizeof buf, "%ld", out);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS DATE|ISO|DATETIME|UTC — human-readable UTC stamp for plates/logs.
     * Usability: agents stamp plates without shell date(1).
     * LAST/DATE/ISO = "YYYY-MM-DDTHH:MM:SSZ"; LAST_N = strlen; also TIME epoch. */
    if (kw(&L->cur,"DATE") || kw(&L->cur,"ISO") || kw(&L->cur,"DATETIME") ||
        kw(&L->cur,"UTC") || kw(&L->cur,"ISO8601") || kw(&L->cur,"TIMESTAMP_ISO")){
      time_t now;
      struct tm tm_utc;
      char iso[40];
      long epoch;
      lex_next(L);
      now = time(NULL);
      epoch = (long)now;
#if defined(CUBALC_OS_WINDOWS)
      {
        struct tm *tp = gmtime(&now);
        if (tp) tm_utc = *tp;
        else memset(&tm_utc, 0, sizeof tm_utc);
      }
#else
      if (!gmtime_r(&now, &tm_utc))
        memset(&tm_utc, 0, sizeof tm_utc);
#endif
      snprintf(iso, sizeof iso, "%04d-%02d-%02dT%02d:%02d:%02dZ",
               tm_utc.tm_year + 1900, tm_utc.tm_mon + 1, tm_utc.tm_mday,
               tm_utc.tm_hour, tm_utc.tm_min, tm_utc.tm_sec);
      var_set_str(vm, "DATE", iso);
      var_set_str(vm, "ISO", iso);
      var_set_str(vm, "DATETIME", iso);
      var_set_str(vm, "LAST", iso);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", iso);
      vm->last_n = (long)strlen(iso);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "TIME", epoch);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS PID — process id → LAST_N/PID (peer identity without shell) */
    if (kw(&L->cur,"PID") || kw(&L->cur,"GETPID") || kw(&L->cur,"PROCESS_ID")){
      long n;
      char buf[32];
      lex_next(L);
#if defined(CUBALC_OS_WINDOWS)
      n = (long)_getpid();
#else
      n = (long)getpid();
#endif
      if (n < 0) n = 0;
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "PID", n);
      var_set_num(vm, "OK", 1);
      snprintf(buf, sizeof buf, "%ld", n);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS HOSTNAME|HOST — machine hostname → LAST/HOSTNAME */
    if (kw(&L->cur,"HOSTNAME") || kw(&L->cur,"HOST") || kw(&L->cur,"NODENAME")){
      char host[256];
      lex_next(L);
      host[0] = 0;
#if defined(CUBALC_OS_WINDOWS)
      {
        const char *e = getenv("COMPUTERNAME");
        if (e && e[0]) snprintf(host, sizeof host, "%s", e);
        else snprintf(host, sizeof host, "localhost");
      }
#else
      if (gethostname(host, sizeof host) != 0 || !host[0])
        snprintf(host, sizeof host, "localhost");
      host[sizeof host - 1] = 0;
#endif
      var_set_str(vm, "HOSTNAME", host);
      var_set_str(vm, "LAST", host);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", host);
      vm->last_n = (long)strlen(host);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS USER|USERNAME|LOGNAME — login name → LAST/USER (identity without shell)
     * Prefer USER/LOGNAME/USERNAME env, then getpwuid; fallback "user". */
    if (kw(&L->cur,"USER") || kw(&L->cur,"USERNAME") || kw(&L->cur,"LOGNAME") ||
        kw(&L->cur,"WHOAMI")){
      char user[128];
      const char *e;
      lex_next(L);
      user[0] = 0;
      e = getenv("USER");
      if (!e || !e[0]) e = getenv("LOGNAME");
      if (!e || !e[0]) e = getenv("USERNAME");
      if (e && e[0]) snprintf(user, sizeof user, "%s", e);
#if !defined(CUBALC_OS_WINDOWS)
      if (!user[0]) {
        struct passwd *pw = getpwuid(getuid());
        if (pw && pw->pw_name && pw->pw_name[0])
          snprintf(user, sizeof user, "%s", pw->pw_name);
      }
#endif
      if (!user[0]) snprintf(user, sizeof user, "user");
      var_set_str(vm, "USER", user);
      var_set_str(vm, "USERNAME", user);
      var_set_str(vm, "LAST", user);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", user);
      vm->last_n = (long)strlen(user);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS UID|USER_ID|EUID — numeric user id → LAST_N/UID */
    if (kw(&L->cur,"UID") || kw(&L->cur,"USER_ID") || kw(&L->cur,"USERID") ||
        kw(&L->cur,"EUID") || kw(&L->cur,"GETUID")){
      long n = 0;
      char buf[32];
      lex_next(L);
#if defined(CUBALC_OS_WINDOWS)
      {
        const char *e = getenv("UID");
        if (e && e[0]) n = strtol(e, NULL, 10);
      }
#else
      n = (long)getuid();
      if (n < 0) n = 0;
#endif
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "UID", n);
      var_set_num(vm, "OK", 1);
      snprintf(buf, sizeof buf, "%ld", n);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS HOME|HOMEDIR|USER_HOME — home directory → LAST/HOME */
    if (kw(&L->cur,"HOME") || kw(&L->cur,"HOMEDIR") || kw(&L->cur,"USER_HOME") ||
        kw(&L->cur,"HOMEPATH")){
      char home[512];
      const char *e;
      lex_next(L);
      home[0] = 0;
      e = getenv("HOME");
#if defined(CUBALC_OS_WINDOWS)
      if (!e || !e[0]) e = getenv("USERPROFILE");
#endif
      if (e && e[0]) snprintf(home, sizeof home, "%s", e);
#if !defined(CUBALC_OS_WINDOWS)
      if (!home[0]) {
        struct passwd *pw = getpwuid(getuid());
        if (pw && pw->pw_dir && pw->pw_dir[0])
          snprintf(home, sizeof home, "%s", pw->pw_dir);
      }
#endif
      if (!home[0]) {
        if (getcwd(home, sizeof home) == NULL)
          snprintf(home, sizeof home, ".");
      }
      var_set_str(vm, "HOME", home);
      var_set_str(vm, "LAST", home);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", home);
      vm->last_n = (long)strlen(home);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS APPEND|LOG path data — append line to file (creates if missing).
     * Soft fail OK=0 + sticky LAST_ERR. LAST_N = bytes written (data+newline).
     * Usability: agent history/audit logs without shell >> . */
    if (kw(&L->cur,"APPEND") || kw(&L->cur,"LOG") || kw(&L->cur,"APPENDLN") ||
        kw(&L->cur,"LOGAPPEND")){
      lex_next(L);
      char path[512]="", data[CUBALC_HOST_STR_MAX];
      long nbytes = 0;
      data[0]=0;
      if (resolve_str_arg(vm, L, path, sizeof path) != 0){
        fail(vm,"SYS APPEND path data"); return -1;
      }
      if (resolve_str_arg(vm, L, data, sizeof data) != 0){
        /* allow bare number as data */
        if (L->cur.kind==TK_NUM || L->cur.kind==TK_MINUS || L->cur.kind==TK_LPAREN){
          long n = parse_expr(vm, L);
          snprintf(data, sizeof data, "%ld", n);
        } else if (L->cur.kind==TK_IDENT){
          Var *v = var_get(vm, L->cur.text, 0);
          if (v && !v->is_str){ snprintf(data, sizeof data, "%ld", v->val); lex_next(L); }
          else { fail(vm,"SYS APPEND path data"); return -1; }
        } else {
          fail(vm,"SYS APPEND path data"); return -1;
        }
      }
      {
        FILE *af = fopen(path, "a");
        if (!af){
          var_set_num(vm,"OK",0);
          var_set_num(vm,"LAST_N",0);
          var_set_str(vm,"LAST_ERR","APPEND: open fail");
          var_set_str(vm,"ERR","APPEND: open fail");
          bump(vm); return 1;
        }
        fputs(data, af);
        fputc('\n', af);
        fclose(af);
        nbytes = (long)strlen(data) + 1;
      }
      var_set_str(vm,"LAST",path);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
      vm->last_n = nbytes;
      var_set_num(vm,"LAST_N",nbytes);
      var_set_num(vm,"OK",1);
      bump(vm); return 1;
    }
    /* SYS HEX|FROMHEX — parse hex string (LAST default) → LAST_N  (I/O codec) */
    if (kw(&L->cur,"HEX") || kw(&L->cur,"FROMHEX") || kw(&L->cur,"XTOI")){
      lex_next(L);
      char s[512]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      /* skip optional 0x / 0X prefix */
      const char *p = s;
      if (p[0]=='0' && (p[1]=='x' || p[1]=='X')) p += 2;
      long n = 0;
      if (p[0]) n = strtol(p, NULL, 16);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS TOHEX [expr] — format int as lowercase hex → LAST (I/O codec) */
    if (kw(&L->cur,"TOHEX") || kw(&L->cur,"ITOH") || kw(&L->cur,"HEXOUT")){
      lex_next(L);
      long n = vm->last_n;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE"))){
        n = parse_expr(vm, L);
      }
      char buf[40];
      snprintf(buf, sizeof buf, "%lx", (unsigned long)n);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS ORD [str|LAST] — first byte code → LAST_N  (I/O codec) */
    if (kw(&L->cur,"ORD") || kw(&L->cur,"CODE") || kw(&L->cur,"BYTE")){
      lex_next(L);
      char s[512]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      long n = s[0] ? (long)(unsigned char)s[0] : 0;
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS CHR [expr] — integer → single-byte string LAST  (I/O codec) */
    if (kw(&L->cur,"CHR") || kw(&L->cur,"CHAR")){
      lex_next(L);
      long n = vm->last_n;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          (L->cur.kind==TK_IDENT && !kw(&L->cur,"ASSERT") && !kw(&L->cur,"LET") &&
           !kw(&L->cur,"SYS") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"CUBE"))){
        n = parse_expr(vm, L);
      }
      n &= 0xFF;
      char buf[4];
      buf[0] = (char)(unsigned char)n;
      buf[1] = 0;
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MID|SUBSTR|SLICE str start [len] — substring → LAST  (I/O codec) */
    if (kw(&L->cur,"MID") || kw(&L->cur,"SUBSTR") || kw(&L->cur,"SLICE")){
      lex_next(L);
      char s[512]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      long start = 0, len = -1;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          L->cur.kind==TK_IDENT){
        start = parse_expr(vm, L);
        if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
            L->cur.kind==TK_IDENT)
          len = parse_expr(vm, L);
      }
      size_t slen = strlen(s);
      if (start < 0) start = 0;
      if ((size_t)start > slen) start = (long)slen;
      size_t remain = slen - (size_t)start;
      size_t take = (len < 0) ? remain : (size_t)len;
      if (take > remain) take = remain;
      char out[512];
      if (take >= sizeof out) take = sizeof out - 1;
      memcpy(out, s + (size_t)start, take);
      out[take] = 0;
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)take;
      var_set_num(vm, "LAST_N", (long)take);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS CAT|STRCAT a b — concatenate strings → LAST (digit-3 string plane) */
    if (kw(&L->cur,"CAT") || kw(&L->cur,"STRCAT") || kw(&L->cur,"CONCAT")){
      lex_next(L);
      char a[512]="", b[512]="";
      if (resolve_str_arg(vm, L, a, sizeof a) != 0)
        snprintf(a, sizeof a, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, b, sizeof b) != 0) b[0]=0;
      char out[1024];
      snprintf(out, sizeof out, "%s%s", a, b);
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)strlen(out);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS FIND|INDEX hay needle — first index of needle in hay → LAST_N (-1 miss).
     * SYS FINDI|INDEXI|FIND I — case-insensitive (log/severity locate without UPPER).
     * Completes EQSI/HASI/GREPI/STARTSI family for position → MID peel. */
    if (kw(&L->cur,"FIND") || kw(&L->cur,"INDEX") || kw(&L->cur,"STRFIND") ||
        kw(&L->cur,"FINDI") || kw(&L->cur,"IFIND") || kw(&L->cur,"INDEXI") ||
        kw(&L->cur,"IINDEX") || kw(&L->cur,"STRFINDI") || kw(&L->cur,"FINDCI")){
      char op[16]; snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *p = op; *p; p++)
        if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 'a' + 'A');
      int icase = (strcmp(op, "FINDI") == 0 || strcmp(op, "IFIND") == 0 ||
                   strcmp(op, "INDEXI") == 0 || strcmp(op, "IINDEX") == 0 ||
                   strcmp(op, "STRFINDI") == 0 || strcmp(op, "FINDCI") == 0);
      lex_next(L);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      char hay[512]="", needle[256]="";
      if (resolve_str_arg(vm, L, hay, sizeof hay) != 0)
        snprintf(hay, sizeof hay, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0]=0;
      long idx = -1;
      if (needle[0] == 0) {
        if (hay[0]) idx = 0;
      } else if (!icase) {
        const char *p = strstr(hay, needle);
        if (p) idx = (long)(p - hay);
      } else {
        /* ASCII case-fold scan */
        size_t hn = strlen(hay), nn = strlen(needle), i, j;
        for (i = 0; i + nn <= hn; i++) {
          int hit = 1;
          for (j = 0; j < nn; j++) {
            char a = hay[i + j], b = needle[j];
            if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
            if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
            if (a != b) { hit = 0; break; }
          }
          if (hit) { idx = (long)i; break; }
        }
      }
      vm->last_n = idx;
      var_set_num(vm, "LAST_N", idx);
      var_set_num(vm, "OK", idx >= 0 ? 1 : 0);
      bump(vm); return 1;
    }
    /* SYS NTH n [str|LAST] — 0-based newline field → LAST (pairs with SYS LIST).
     * SYS LINE n [str] — 1-based alias.
     * SYS HEAD [str] — first field; SYS TAIL [str] — last field.
     * Soft out-of-range: OK=0 LAST="" LAST_N=0. */
    if (kw(&L->cur,"NTH") || kw(&L->cur,"LINE") || kw(&L->cur,"FIELD") ||
        kw(&L->cur,"GETLINE") || kw(&L->cur,"HEAD") || kw(&L->cur,"FIRST") ||
        kw(&L->cur,"TAIL") || kw(&L->cur,"LASTLINE")){
      int is_head = (kw(&L->cur,"HEAD") || kw(&L->cur,"FIRST"));
      int is_tail = (kw(&L->cur,"TAIL") || kw(&L->cur,"LASTLINE"));
      int one_based = (kw(&L->cur,"LINE") || kw(&L->cur,"GETLINE"));
      long want = 0, nlines = 0, i;
      char src[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      char out[512];
      size_t len;
      lex_next(L);
      if (!is_head && !is_tail) {
        if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
            L->cur.kind==TK_MINUS)
          want = parse_expr(vm, L);
        else
          want = 0;
        if (one_based) want -= 1;
      }
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      /* count fields; trailing newline does not create an empty last field */
      if (src[0]) {
        nlines = 1;
        for (p = src; *p; p++) {
          if (*p == '\n' && p[1]) nlines++;
        }
      }
      if (is_head) want = 0;
      if (is_tail) want = nlines > 0 ? nlines - 1 : -1;
      if (want < 0 || want >= nlines || !src[0]) {
        var_set_str(vm, "LAST", "");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        vm->last_n = 0;
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      p = src;
      for (i = 0; i < want; i++) {
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
      }
      start = p;
      while (*p && *p != '\n') p++;
      len = (size_t)(p - start);
      if (len >= sizeof out) len = sizeof out - 1;
      memcpy(out, start, len);
      out[len] = 0;
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)len;
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS GREP|FILTER needle [str|LAST] — keep newline fields containing needle.
     * SYS GREPV|VGREP — invert. SYS GREPI|IGREP — case-insensitive.
     * SYS GREP I|ICASE|-I needle — same as GREPI. GREPVI for invert+icase.
     * LAST = kept lines; LAST_N/GREP_N = count. Empty needle: GREP keeps all.
     * Usability: filter LIST/logs without shell grep -i. */
    if (kw(&L->cur,"GREP") || kw(&L->cur,"FILTER") || kw(&L->cur,"MATCHLINES") ||
        kw(&L->cur,"GREPV") || kw(&L->cur,"VGREP") || kw(&L->cur,"FILTERV") ||
        kw(&L->cur,"NOMATCH") ||
        kw(&L->cur,"GREPI") || kw(&L->cur,"IGREP") || kw(&L->cur,"FILTERI") ||
        kw(&L->cur,"GREPVI") || kw(&L->cur,"VGREPI") || kw(&L->cur,"IFILTERV")){
      int invert = (kw(&L->cur,"GREPV") || kw(&L->cur,"VGREP") ||
                    kw(&L->cur,"FILTERV") || kw(&L->cur,"NOMATCH") ||
                    kw(&L->cur,"GREPVI") || kw(&L->cur,"VGREPI") ||
                    kw(&L->cur,"IFILTERV"));
      int icase = (kw(&L->cur,"GREPI") || kw(&L->cur,"IGREP") ||
                   kw(&L->cur,"FILTERI") || kw(&L->cur,"GREPVI") ||
                   kw(&L->cur,"VGREPI") || kw(&L->cur,"IFILTERV"));
      char needle[256]="", src[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t olen = 0;
      long kept = 0;
      lex_next(L);
      /* optional I / ICASE / -I after plain GREP/FILTER */
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") || kw(&L->cur,"IGNORECASE") ||
                     kw(&L->cur,"-I") || kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0]=0;
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      out[0] = 0;
      if (src[0]) {
        p = src;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          {
            size_t flen = (size_t)(p - start);
            char field[512];
            int hit;
            if (flen >= sizeof field) flen = sizeof field - 1;
            memcpy(field, start, flen);
            field[flen] = 0;
            if (needle[0] == 0) {
              hit = 1;
            } else if (!icase) {
              hit = (strstr(field, needle) != NULL) ? 1 : 0;
            } else {
              /* case-insensitive substring */
              size_t nl = strlen(needle), fi;
              hit = 0;
              for (fi = 0; field[fi] && !hit; fi++) {
                size_t j;
                for (j = 0; j < nl; j++) {
                  char a = field[fi + j], b = needle[j];
                  if (!a) break;
                  if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                  if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                  if (a != b) break;
                }
                if (j == nl) hit = 1;
              }
            }
            if (invert) hit = !hit;
            if (hit) {
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + flen < sizeof out) {
                memcpy(out + olen, field, flen);
                olen += flen;
              } else if (olen < sizeof out - 1) {
                size_t take = sizeof out - 1 - olen;
                memcpy(out + olen, field, take);
                olen += take;
              }
              out[olen] = 0;
              kept++;
            }
          }
          if (*p == '\n') p++;
        }
      }
      out[olen] = 0;
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "GREP", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "GREP_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS TAKE|FIRSTN n [str|LAST] — first n newline fields → LAST.
     * SYS DROP|SKIP|REST n [str] — drop first n fields (keep the rest).
     * LAST_N/TAKE_N = kept count. n<=0: TAKE empty / DROP all.
     * Usability: window LIST/GREP text without index math before EACH LINE. */
    if (kw(&L->cur,"TAKE") || kw(&L->cur,"FIRSTN") || kw(&L->cur,"HEADN") ||
        kw(&L->cur,"DROP") || kw(&L->cur,"SKIP") || kw(&L->cur,"REST") ||
        kw(&L->cur,"TAILN") || kw(&L->cur,"DROPN")){
      int is_drop = (kw(&L->cur,"DROP") || kw(&L->cur,"SKIP") || kw(&L->cur,"REST") ||
                     kw(&L->cur,"TAILN") || kw(&L->cur,"DROPN"));
      long nwant = 0, idx = 0, kept = 0;
      char src[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t olen = 0;
      lex_next(L);
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
          L->cur.kind==TK_MINUS)
        nwant = parse_expr(vm, L);
      else
        nwant = 0;
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      out[0] = 0;
      if (src[0] && (is_drop || nwant > 0)) {
        p = src;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          if (start == p && *p == 0 && start > src && start[-1] == '\n')
            break;
          {
            size_t flen = (size_t)(p - start);
            int keep = 0;
            if (is_drop) keep = (idx >= nwant);
            else keep = (idx < nwant);
            if (keep) {
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + flen < sizeof out) {
                memcpy(out + olen, start, flen);
                olen += flen;
              } else if (olen < sizeof out - 1) {
                size_t take = sizeof out - 1 - olen;
                memcpy(out + olen, start, take);
                olen += take;
              }
              out[olen] = 0;
              kept++;
            }
            idx++;
          }
          if (*p == '\n') p++;
        }
      }
      out[olen] = 0;
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "TAKE_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SPLIT|SPLITBY|FIELDS sep [str|LAST] — split on sep → newline fields.
     * LAST_N/SPLIT_N = field count. Empty sep → whole string one field.
     * Usability: PATH/CSV/env lists → EACH LINE/GREP/TAKE without shell. */
    if (kw(&L->cur,"SPLIT") || kw(&L->cur,"SPLITBY") || kw(&L->cur,"FIELDS") ||
        kw(&L->cur,"STRSPLIT") || kw(&L->cur,"SEPARATE") || kw(&L->cur,"CUTSEP")){
      char sep[64]="", src[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      const char *p, *hit;
      size_t sepn, olen = 0, flen;
      long nfields = 0;
      lex_next(L);
      if (resolve_str_arg(vm, L, sep, sizeof sep) != 0) sep[0]=0;
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      out[0] = 0;
      sepn = strlen(sep);
      if (!src[0]) {
        /* empty source → zero fields */
      } else if (sepn == 0) {
        /* no separator → single field */
        snprintf(out, sizeof out, "%s", src);
        nfields = 1;
        olen = strlen(out);
      } else {
        p = src;
        for (;;) {
          hit = strstr(p, sep);
          if (hit) flen = (size_t)(hit - p);
          else flen = strlen(p);
          if (nfields > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + flen < sizeof out) {
            memcpy(out + olen, p, flen);
            olen += flen;
          } else if (olen < sizeof out - 1) {
            size_t take = sizeof out - 1 - olen;
            memcpy(out + olen, p, take);
            olen += take;
          }
          out[olen] = 0;
          nfields++;
          if (!hit) break;
          p = hit + sepn;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "SPLIT", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = nfields;
      var_set_num(vm, "LAST_N", nfields);
      var_set_num(vm, "SPLIT_N", nfields);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS WORDS|TOKENIZE|FIELDSW [str|LAST] — whitespace tokenize → newline fields.
     * Collapses runs of space/tab/CR/LF; trims ends. LAST_N/WORDS_N = token count.
     * Usability: free text / "a b  c" → EACH LINE/GREP/SORT without SPLIT " " glue. */
    if (kw(&L->cur,"WORDS") || kw(&L->cur,"TOKENIZE") || kw(&L->cur,"TOKENS") ||
        kw(&L->cur,"FIELDSW") || kw(&L->cur,"WSSPLIT") || kw(&L->cur,"SPLITWS") ||
        kw(&L->cur,"WORDLIST")){
      char src[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      const char *p;
      size_t olen = 0;
      long ntok = 0;
      lex_next(L);
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      out[0] = 0;
      p = src;
      while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        if (!*p) break;
        {
          const char *start = p;
          size_t flen;
          while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
          flen = (size_t)(p - start);
          if (ntok > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + flen < sizeof out) {
            memcpy(out + olen, start, flen);
            olen += flen;
          } else if (olen < sizeof out - 1) {
            size_t take = sizeof out - 1 - olen;
            memcpy(out + olen, start, take);
            olen += take;
          }
          out[olen] = 0;
          ntok++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "WORDS", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = ntok;
      var_set_num(vm, "LAST_N", ntok);
      var_set_num(vm, "WORDS_N", ntok);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS CUT|COLUMN|FIELDN hay sep n — peel Nth field by separator → LAST.
     * CUT/FIELDN: 0-based index. COLUMN/COL/COLN: 1-based (like SYS LINE).
     * LAST_N = 1 if field exists, 0 if miss/out-of-range. Empty sep → whole as field 0.
     * Usability: CSV/path/kv columns without SPLIT + NTH glue. */
    if (kw(&L->cur,"CUT") || kw(&L->cur,"COLUMN") || kw(&L->cur,"COL") ||
        kw(&L->cur,"COLN") || kw(&L->cur,"FIELDN") || kw(&L->cur,"NTHFIELD") ||
        kw(&L->cur,"GETFIELD") || kw(&L->cur,"CSVFIELD")){
      char op[16]; snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      int one_based = (strcmp(op, "COLUMN") == 0 || strcmp(op, "COL") == 0 ||
                       strcmp(op, "COLN") == 0);
      char hay[CUBALC_HOST_STR_MAX], sep[64], out[512];
      long want = 0, idx = 0, found = 0;
      const char *p, *hit, *start;
      size_t sepn, flen;
      lex_next(L);
      if (resolve_str_arg(vm, L, hay, sizeof hay) != 0)
        snprintf(hay, sizeof hay, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, sep, sizeof sep) != 0) sep[0] = 0;
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
        want = parse_expr(vm, L);
      else
        want = 0;
      if (one_based) {
        if (want < 1) want = 1;
        want = want - 1; /* convert to 0-based walk */
      } else {
        if (want < 0) want = 0;
      }
      out[0] = 0;
      sepn = strlen(sep);
      p = hay;
      if (!hay[0]) {
        found = 0;
      } else if (sepn == 0) {
        /* empty sep: only field 0 is the whole string */
        if (want == 0) {
          snprintf(out, sizeof out, "%s", hay);
          found = 1;
        }
      } else {
        while (*p) {
          start = p;
          hit = strstr(p, sep);
          if (hit) {
            flen = (size_t)(hit - p);
            p = hit + sepn;
          } else {
            flen = strlen(p);
            p = p + flen;
          }
          if (idx == want) {
            if (flen >= sizeof out) flen = sizeof out - 1;
            memcpy(out, start, flen);
            out[flen] = 0;
            found = 1;
            break;
          }
          idx++;
          if (!hit) break;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "CUT", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      var_set_num(vm, "CUT_N", found);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SORT [str|LAST] — lexicographic sort of newline fields → LAST.
     * SYS UNIQ [str|LAST] — drop adjacent duplicate fields (sort first for full unique).
     * LAST_N/SORT_N = kept count. Cap 512 fields.
     * Usability: stable LIST walks — order is not guaranteed by SYS LIST. */
    if (kw(&L->cur,"SORT") || kw(&L->cur,"SORTLINES") || kw(&L->cur,"SORTL") ||
        kw(&L->cur,"UNIQ") || kw(&L->cur,"UNIQUE") || kw(&L->cur,"DEDUP")){
      int is_uniq = (kw(&L->cur,"UNIQ") || kw(&L->cur,"UNIQUE") || kw(&L->cur,"DEDUP"));
      char src[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      /* field copies for stable strcmp (cap 512 × 256) */
      enum { SORT_MAX = 256, SORT_FLEN = 192 };
      char fields[SORT_MAX][SORT_FLEN];
      int order[SORT_MAX];
      int n = 0, i;
      const char *p, *start;
      size_t olen = 0;
      long kept = 0;
      lex_next(L);
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      out[0] = 0;
      if (src[0]) {
        p = src;
        while (*p && n < SORT_MAX) {
          start = p;
          while (*p && *p != '\n') p++;
          if (start == p && *p == 0 && start > src && start[-1] == '\n')
            break;
          {
            size_t flen = (size_t)(p - start);
            if (flen >= SORT_FLEN) flen = SORT_FLEN - 1;
            memcpy(fields[n], start, flen);
            fields[n][flen] = 0;
            order[n] = n;
            n++;
          }
          if (*p == '\n') p++;
        }
      }
      if (!is_uniq && n > 1) {
        /* insertion sort on order[] by fields — small n, pure C, no nested fn needed */
        for (i = 1; i < n; i++) {
          int key = order[i], j = i - 1;
          while (j >= 0 && strcmp(fields[order[j]], fields[key]) > 0) {
            order[j + 1] = order[j];
            j--;
          }
          order[j + 1] = key;
        }
      }
      for (i = 0; i < n; i++) {
        int idx = is_uniq ? i : order[i];
        if (is_uniq && i > 0 && strcmp(fields[i], fields[i - 1]) == 0)
          continue;
        if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        {
          size_t flen = strlen(fields[idx]);
          if (olen + flen < sizeof out) {
            memcpy(out + olen, fields[idx], flen);
            olen += flen;
          } else if (olen < sizeof out - 1) {
            size_t take = sizeof out - 1 - olen;
            memcpy(out + olen, fields[idx], take);
            olen += take;
          }
          out[olen] = 0;
        }
        kept++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "SORT_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS REVL|REVLINES|FLIPLINES [str|LAST] — reverse newline field order → LAST.
     * LAST_N/REVL_N = field count. Cap 256 fields (same as SORT).
     * Usability: newest-first logs / LIFO work bags with POP without shell tac. */
    if (kw(&L->cur,"REVL") || kw(&L->cur,"REVLINES") || kw(&L->cur,"FLIPLINES") ||
        kw(&L->cur,"REVERSELINES") || kw(&L->cur,"LINEREV") || kw(&L->cur,"TAC") ||
        kw(&L->cur,"REVLINE")){
      char src[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      enum { REVL_MAX = 256, REVL_FLEN = 192 };
      char fields[REVL_MAX][REVL_FLEN];
      int n = 0, i;
      const char *p, *start;
      size_t olen = 0;
      lex_next(L);
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      out[0] = 0;
      if (src[0]) {
        p = src;
        while (*p && n < REVL_MAX) {
          start = p;
          while (*p && *p != '\n') p++;
          if (start == p && *p == 0 && start > src && start[-1] == '\n')
            break;
          {
            size_t flen = (size_t)(p - start);
            if (flen >= REVL_FLEN) flen = REVL_FLEN - 1;
            memcpy(fields[n], start, flen);
            fields[n][flen] = 0;
            n++;
          }
          if (*p == '\n') p++;
        }
      }
      for (i = n - 1; i >= 0; i--) {
        size_t flen = strlen(fields[i]);
        if (olen > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        if (olen + flen < sizeof out) {
          memcpy(out + olen, fields[i], flen);
          olen += flen;
        } else if (olen < sizeof out - 1) {
          size_t take = sizeof out - 1 - olen;
          memcpy(out + olen, fields[i], take);
          olen += take;
        }
        out[olen] = 0;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)n;
      var_set_num(vm, "LAST_N", (long)n);
      var_set_num(vm, "REVL_N", (long)n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS JOINLINES|PASTE|MERGE sep [str|LAST] — join newline fields with sep.
     * Inverse of SYS SPLIT. LAST = joined string; LAST_N/JOIN_N = field count.
     * Empty sep concatenates with no delimiter.
     * Usability: LIST/GREP/SORT results → CSV/report line without shell paste. */
    if (kw(&L->cur,"JOINLINES") || kw(&L->cur,"PASTE") || kw(&L->cur,"MERGE") ||
        kw(&L->cur,"JOINSEP") || kw(&L->cur,"IMPLADE") || kw(&L->cur,"LINESJOIN")){
      char sep[64]="", src[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t sepn, olen = 0, flen;
      long nfields = 0;
      lex_next(L);
      if (resolve_str_arg(vm, L, sep, sizeof sep) != 0) sep[0]=0;
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      out[0] = 0;
      sepn = strlen(sep);
      if (src[0]) {
        p = src;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          if (start == p && *p == 0 && start > src && start[-1] == '\n')
            break;
          flen = (size_t)(p - start);
          if (nfields > 0 && sepn > 0) {
            if (olen + sepn < sizeof out) {
              memcpy(out + olen, sep, sepn);
              olen += sepn;
            } else if (olen < sizeof out - 1) {
              size_t take = sizeof out - 1 - olen;
              memcpy(out + olen, sep, take);
              olen += take;
            }
          }
          if (olen + flen < sizeof out) {
            memcpy(out + olen, start, flen);
            olen += flen;
          } else if (olen < sizeof out - 1) {
            size_t take = sizeof out - 1 - olen;
            memcpy(out + olen, start, take);
            olen += take;
          }
          out[olen] = 0;
          nfields++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "JOINED", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = nfields;
      var_set_num(vm, "LAST_N", nfields);
      var_set_num(vm, "JOIN_N", nfields);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS PUSH|ADDLINE|PUSHLINE bag [line|LAST] — append a newline field to bag.
     * Empty bag → line only; empty line still adds a blank field after bag.
     * LAST = bag; LAST_N/PUSH_N = field count after push.
     * Usability: multi-file hit accumulate without CAT + "\\n" glue. */
    if (kw(&L->cur,"PUSH") || kw(&L->cur,"ADDLINE") || kw(&L->cur,"PUSHLINE") ||
        kw(&L->cur,"LINEPUSH") || kw(&L->cur,"BAGPUSH") || kw(&L->cur,"ACCUM")){
      char bag[CUBALC_HOST_STR_MAX], line[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      const char *p;
      long nfields = 0;
      size_t blen, llen, o;
      lex_next(L);
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0) bag[0] = 0;
      if (resolve_str_arg(vm, L, line, sizeof line) != 0)
        snprintf(line, sizeof line, "%s", vm->last_str);
      blen = strlen(bag);
      llen = strlen(line);
      out[0] = 0;
      o = 0;
      if (blen == 0) {
        if (llen < sizeof out) {
          memcpy(out, line, llen);
          o = llen;
        } else {
          o = sizeof out - 1;
          memcpy(out, line, o);
        }
        out[o] = 0;
        /* first fill: count fields inside line (may itself be multi-line) */
        if (out[0]) {
          p = out;
          while (*p) {
            while (*p && *p != '\n') p++;
            nfields++;
            if (*p == '\n') p++;
          }
        }
      } else {
        long bag_n = 0;
        p = bag;
        while (*p) {
          while (*p && *p != '\n') p++;
          bag_n++;
          if (*p == '\n') p++;
        }
        if (blen < sizeof out) {
          memcpy(out, bag, blen);
          o = blen;
        } else {
          o = sizeof out - 1;
          memcpy(out, bag, o);
        }
        if (o + 1 < sizeof out) out[o++] = '\n';
        if (o + llen < sizeof out) {
          memcpy(out + o, line, llen);
          o += llen;
        } else if (o < sizeof out - 1) {
          size_t take = sizeof out - 1 - o;
          memcpy(out + o, line, take);
          o += take;
        }
        out[o] = 0;
        /* always +1 field even if line is empty (trailing blank field) */
        nfields = bag_n + 1;
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "PUSH", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = nfields;
      var_set_num(vm, "LAST_N", nfields);
      var_set_num(vm, "PUSH_N", nfields);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS PREPEND|UNSHIFT|PUSHFRONT bag [line|LAST] — insert field at front of bag.
     * Dual of PUSH (append). LAST = bag; LAST_N/PREPEND_N = field count.
     * Usability: priority / FIFO enqueue front without REVL+PUSH glue. */
    if (kw(&L->cur,"PREPEND") || kw(&L->cur,"UNSHIFT") || kw(&L->cur,"PUSHFRONT") ||
        kw(&L->cur,"PREPENDLINE") || kw(&L->cur,"LINEPREPEND") || kw(&L->cur,"BAGPREPEND") ||
        kw(&L->cur,"INSERTFRONT")){
      char bag[CUBALC_HOST_STR_MAX], line[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      const char *p;
      long nfields = 0;
      size_t blen, llen, o;
      lex_next(L);
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0) bag[0] = 0;
      if (resolve_str_arg(vm, L, line, sizeof line) != 0)
        snprintf(line, sizeof line, "%s", vm->last_str);
      blen = strlen(bag);
      llen = strlen(line);
      out[0] = 0;
      o = 0;
      /* front field */
      if (llen < sizeof out) {
        memcpy(out, line, llen);
        o = llen;
      } else {
        o = sizeof out - 1;
        memcpy(out, line, o);
      }
      out[o] = 0;
      if (blen > 0) {
        if (o + 1 < sizeof out) out[o++] = '\n';
        if (o + blen < sizeof out) {
          memcpy(out + o, bag, blen);
          o += blen;
        } else if (o < sizeof out - 1) {
          size_t take = sizeof out - 1 - o;
          memcpy(out + o, bag, take);
          o += take;
        }
        out[o] = 0;
        nfields = 1; /* new front */
        p = bag;
        while (*p) {
          while (*p && *p != '\n') p++;
          nfields++;
          if (*p == '\n') p++;
        }
      } else {
        nfields = 1; /* only the new front field (even if empty) */
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "PREPEND", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = nfields;
      var_set_num(vm, "LAST_N", nfields);
      var_set_num(vm, "PREPEND_N", nfields);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS POP|POPLINE|SHIFT [bag|LAST] — remove last newline field from bag.
     * LAST = popped field; POP_REST/REST = remaining bag; LAST_N = 1 if popped, 0 if empty.
     * POP_N = remaining field count. Empty bag → LAST "", REST "", LAST_N=0.
     * Usability: process PUSH bags one field at a time without NTH index glue.
     * Note: not stack POP (that is bare POP outside SYS). */
    if (kw(&L->cur,"POP") || kw(&L->cur,"POPLINE") || kw(&L->cur,"LINEPOP") ||
        kw(&L->cur,"BAGPOP") || kw(&L->cur,"SHIFT") || kw(&L->cur,"UNPUSH")){
      char bag[CUBALC_HOST_STR_MAX], lastf[512], rest[CUBALC_HOST_STR_MAX];
      const char *p, *last_nl = NULL;
      long rest_n = 0, found = 0;
      size_t flen;
      lex_next(L);
      bag[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      lastf[0] = 0;
      rest[0] = 0;
      if (!bag[0]) {
        found = 0;
      } else {
        /* find last newline; if none, whole bag is the only field */
        for (p = bag; *p; p++)
          if (*p == '\n') last_nl = p;
        if (!last_nl) {
          snprintf(lastf, sizeof lastf, "%s", bag);
          rest[0] = 0;
          found = 1;
          rest_n = 0;
        } else {
          flen = strlen(last_nl + 1);
          if (flen >= sizeof lastf) flen = sizeof lastf - 1;
          memcpy(lastf, last_nl + 1, flen);
          lastf[flen] = 0;
          {
            size_t rlen = (size_t)(last_nl - bag);
            if (rlen >= sizeof rest) rlen = sizeof rest - 1;
            memcpy(rest, bag, rlen);
            rest[rlen] = 0;
          }
          found = 1;
          if (rest[0]) {
            p = rest;
            while (*p) {
              while (*p && *p != '\n') p++;
              rest_n++;
              if (*p == '\n') p++;
            }
          }
        }
      }
      var_set_str(vm, "LAST", lastf);
      var_set_str(vm, "POP", lastf);
      var_set_str(vm, "POP_REST", rest);
      var_set_str(vm, "REST", rest);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", lastf);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      var_set_num(vm, "POP_N", rest_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS POPHEAD|HEADPOP|SHIFTF [bag|LAST] — peel first newline field (FIFO).
     * LAST = first field; POP_REST/REST = remaining; LAST_N = 1 if peeled, 0 empty.
     * Complements POP (last/LIFO). Note: SYS SHIFT remains POP alias (compat).
     * Usability: FIFO work queue without NTH 0 + DROP 1 glue. */
    if (kw(&L->cur,"POPHEAD") || kw(&L->cur,"HEADPOP") || kw(&L->cur,"SHIFTF") ||
        kw(&L->cur,"SHIFTFRONT") || kw(&L->cur,"DEQUEUE") || kw(&L->cur,"POPFIRST") ||
        kw(&L->cur,"FIRSTPOP") || kw(&L->cur,"UNSHIFT_POP")){
      char bag[CUBALC_HOST_STR_MAX], first[512], rest[CUBALC_HOST_STR_MAX];
      const char *p;
      long rest_n = 0, found = 0;
      size_t flen;
      lex_next(L);
      bag[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      first[0] = 0;
      rest[0] = 0;
      if (!bag[0]) {
        found = 0;
      } else {
        p = bag;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - bag);
        if (flen >= sizeof first) flen = sizeof first - 1;
        memcpy(first, bag, flen);
        first[flen] = 0;
        found = 1;
        if (*p == '\n') {
          snprintf(rest, sizeof rest, "%s", p + 1);
          if (rest[0]) {
            const char *q = rest;
            while (*q) {
              while (*q && *q != '\n') q++;
              rest_n++;
              if (*q == '\n') q++;
            }
          }
        } else {
          rest[0] = 0;
          rest_n = 0;
        }
      }
      var_set_str(vm, "LAST", first);
      var_set_str(vm, "POPHEAD", first);
      var_set_str(vm, "POP_REST", rest);
      var_set_str(vm, "REST", rest);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", first);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      var_set_num(vm, "POPHEAD_N", rest_n);
      var_set_num(vm, "POP_N", rest_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS LINES|NLINES|WC|COUNTLINES [str|LAST] — count newline fields → LAST_N.
     * LAST kept as source text; LINES_N/WC_N = count. Empty → 0.
     * Usability: bag/LIST size after other ops overwrite LAST_N. */
    if (kw(&L->cur,"LINES") || kw(&L->cur,"NLINES") || kw(&L->cur,"WC") ||
        kw(&L->cur,"COUNTLINES") || kw(&L->cur,"LINECOUNT") ||
        kw(&L->cur,"NF") || kw(&L->cur,"NFIELDS")){
      char src[CUBALC_HOST_STR_MAX];
      const char *p;
      long nfields = 0;
      lex_next(L);
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      if (src[0]) {
        p = src;
        while (*p) {
          while (*p && *p != '\n') p++;
          nfields++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", src);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", src);
      vm->last_n = nfields;
      var_set_num(vm, "LAST_N", nfields);
      var_set_num(vm, "LINES_N", nfields);
      var_set_num(vm, "WC_N", nfields);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS HASLINE|HASFIELD bag needle — exact newline-field membership → LAST_N 0|1.
     * SYS HASLINEI|ILINEIN — case-insensitive exact field (not substring GREP).
     * Usability: work-bag membership after LIST/PUSH without EACH LINE + EQS glue. */
    if (kw(&L->cur,"HASLINE") || kw(&L->cur,"HASFIELD") || kw(&L->cur,"CONTAINSLINE") ||
        kw(&L->cur,"INLINES") || kw(&L->cur,"LINEIN") || kw(&L->cur,"MEMBER") ||
        kw(&L->cur,"HASLINEI") || kw(&L->cur,"HASFIELDI") || kw(&L->cur,"ILINEIN") ||
        kw(&L->cur,"LINEINI") || kw(&L->cur,"MEMBERI")){
      char op[20]; snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      int icase = (strcmp(op, "HASLINEI") == 0 || strcmp(op, "HASFIELDI") == 0 ||
                   strcmp(op, "ILINEIN") == 0 || strcmp(op, "LINEINI") == 0 ||
                   strcmp(op, "MEMBERI") == 0);
      char bag[CUBALC_HOST_STR_MAX], needle[512];
      const char *p, *start;
      long hit = 0;
      size_t nn, flen;
      lex_next(L);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      bag[0] = 0; needle[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0] = 0;
      nn = strlen(needle);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen == nn) {
            if (!icase) {
              if (nn == 0 || memcmp(start, needle, nn) == 0) { hit = 1; break; }
            } else {
              size_t i; int ok = 1;
              for (i = 0; i < nn; i++) {
                char a = start[i], b = needle[i];
                if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                if (a != b) { ok = 0; break; }
              }
              if (ok) { hit = 1; break; }
            }
          }
          if (*p == '\n') p++;
        }
      } else if (nn == 0) {
        /* empty bag: empty needle is not a field unless bag has empty field */
        hit = 0;
      }
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "HASLINE_N", hit);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS COUNTLINE|COUNTFIELD bag needle — count exact field matches → LAST_N.
     * SYS COUNTLINEI — case-insensitive exact field. Distinct from SYS LINES/COUNTLINES
     * (total fields) and HASLINE (0|1). Usability: how many idle peers / ERROR lines. */
    if (kw(&L->cur,"COUNTLINE") || kw(&L->cur,"COUNTFIELD") || kw(&L->cur,"NMATCH") ||
        kw(&L->cur,"OCCUR") || kw(&L->cur,"OCCURS") || kw(&L->cur,"TALLY") ||
        kw(&L->cur,"COUNTLINEI") || kw(&L->cur,"COUNTFIELDI") || kw(&L->cur,"NMATCHI") ||
        kw(&L->cur,"TALLYI") || kw(&L->cur,"OCCURI")){
      char op[20]; snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      int icase = (strcmp(op, "COUNTLINEI") == 0 || strcmp(op, "COUNTFIELDI") == 0 ||
                   strcmp(op, "NMATCHI") == 0 || strcmp(op, "TALLYI") == 0 ||
                   strcmp(op, "OCCURI") == 0);
      char bag[CUBALC_HOST_STR_MAX], needle[512];
      const char *p, *start;
      long hit = 0;
      size_t nn, flen;
      lex_next(L);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      bag[0] = 0; needle[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0] = 0;
      nn = strlen(needle);
      if (bag[0]) {
        p = bag;
        while (*p) {
          int match = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen == nn) {
            if (!icase) {
              match = (nn == 0 || memcmp(start, needle, nn) == 0) ? 1 : 0;
            } else {
              size_t i; match = 1;
              for (i = 0; i < nn; i++) {
                char a = start[i], b = needle[i];
                if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                if (a != b) { match = 0; break; }
              }
            }
          }
          if (match) hit++;
          if (*p == '\n') p++;
        }
      }
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "COUNTLINE_N", hit);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS FINDLINE|LINEINDEX bag needle — 0-based index of first exact field → LAST_N.
     * Miss → LAST_N=-1, OK=0. SYS FINDLINEI — case-insensitive exact field.
     * Completes HASLINE/REMOVELINE for locate-then-NTH without EACH glue. */
    if (kw(&L->cur,"FINDLINE") || kw(&L->cur,"LINEINDEX") || kw(&L->cur,"INDEXLINE") ||
        kw(&L->cur,"FIELDINDEX") || kw(&L->cur,"WHICHLINE") || kw(&L->cur,"LINEOF") ||
        kw(&L->cur,"FINDLINEI") || kw(&L->cur,"LINEINDEXI") || kw(&L->cur,"INDEXLINEI") ||
        kw(&L->cur,"FIELDINDEXI") || kw(&L->cur,"WHICHLINEI")){
      char op[20]; snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      int icase = (strcmp(op, "FINDLINEI") == 0 || strcmp(op, "LINEINDEXI") == 0 ||
                   strcmp(op, "INDEXLINEI") == 0 || strcmp(op, "FIELDINDEXI") == 0 ||
                   strcmp(op, "WHICHLINEI") == 0);
      char bag[CUBALC_HOST_STR_MAX], needle[512];
      const char *p, *start;
      long idx = -1, cur = 0;
      size_t nn, flen;
      lex_next(L);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      bag[0] = 0; needle[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0] = 0;
      nn = strlen(needle);
      if (bag[0]) {
        p = bag;
        while (*p) {
          int match = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen == nn) {
            if (!icase) {
              match = (nn == 0 || memcmp(start, needle, nn) == 0) ? 1 : 0;
            } else {
              size_t i; match = 1;
              for (i = 0; i < nn; i++) {
                char a = start[i], b = needle[i];
                if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                if (a != b) { match = 0; break; }
              }
            }
          }
          if (match) { idx = cur; break; }
          cur++;
          if (*p == '\n') p++;
        }
      }
      vm->last_n = idx;
      var_set_num(vm, "LAST_N", idx);
      var_set_num(vm, "FINDLINE_N", idx);
      var_set_num(vm, "OK", idx >= 0 ? 1 : 0);
      bump(vm); return 1;
    }
    /* SYS SETLINE|REPLACELINE bag n value — set 0-based field → LAST bag.
     * LAST_N = 1 if index in range, 0 soft miss (bag unchanged).
     * Usability: FINDLINE then update status in place without EACH rebuild. */
    if (kw(&L->cur,"SETLINE") || kw(&L->cur,"REPLACELINE") || kw(&L->cur,"PUTLINE") ||
        kw(&L->cur,"SETFIELD") || kw(&L->cur,"PUTFIELD") || kw(&L->cur,"REPLACEFIELD") ||
        kw(&L->cur,"LINESET") || kw(&L->cur,"FIELDSET")){
      char bag[CUBALC_HOST_STR_MAX], val[512], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      long want = 0, cur = 0, hit = 0;
      size_t flen, vlen, o = 0;
      int first = 1;
      lex_next(L);
      bag[0] = 0; val[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
        want = parse_expr(vm, L);
      else
        want = 0;
      if (resolve_str_arg(vm, L, val, sizeof val) != 0) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long n = parse_expr(vm, L);
          snprintf(val, sizeof val, "%ld", n);
        } else {
          val[0] = 0;
        }
      }
      vlen = strlen(val);
      if (want < 0 || !bag[0]) {
        snprintf(out, sizeof out, "%s", bag);
        hit = 0;
      } else {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (!first && o + 1 < sizeof out) out[o++] = '\n';
          first = 0;
          if (cur == want) {
            hit = 1;
            if (o + vlen < sizeof out) {
              memcpy(out + o, val, vlen);
              o += vlen;
            } else if (o < sizeof out - 1) {
              size_t take = sizeof out - 1 - o;
              memcpy(out + o, val, take);
              o += take;
            }
          } else {
            if (o + flen < sizeof out) {
              memcpy(out + o, start, flen);
              o += flen;
            } else if (o < sizeof out - 1) {
              size_t take = sizeof out - 1 - o;
              memcpy(out + o, start, take);
              o += take;
            }
          }
          out[o] = 0;
          cur++;
          if (*p == '\n') p++;
        }
        if (!hit) {
          /* out-of-range: restore original */
          snprintf(out, sizeof out, "%s", bag);
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "SETLINE_N", hit);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SETMATCH|REPLACEMATCH bag old new — replace first exact field value → LAST bag.
     * LAST_N = 1 if replaced, 0 soft miss (bag unchanged). SYS SETMATCHI — icase on old.
     * Distinct from SETLINE (by index) and REPLACE (substring). Usability: status by name. */
    if (kw(&L->cur,"SETMATCH") || kw(&L->cur,"REPLACEMATCH") || kw(&L->cur,"CHANGEFIELD") ||
        kw(&L->cur,"UPDATEFIELD") || kw(&L->cur,"SUBFIELD") || kw(&L->cur,"MAPFIELD") ||
        kw(&L->cur,"REWRITEFIELD") || kw(&L->cur,"SETMATCHI") || kw(&L->cur,"REPLACEMATCHI") ||
        kw(&L->cur,"CHANGEFIELDI") || kw(&L->cur,"UPDATEFIELDI") || kw(&L->cur,"MAPFIELDI")){
      char op[24]; snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      int icase = (strcmp(op, "SETMATCHI") == 0 || strcmp(op, "REPLACEMATCHI") == 0 ||
                   strcmp(op, "CHANGEFIELDI") == 0 || strcmp(op, "UPDATEFIELDI") == 0 ||
                   strcmp(op, "MAPFIELDI") == 0);
      char bag[CUBALC_HOST_STR_MAX], oldv[512], newv[512], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      long hit = 0;
      size_t nn, flen, vlen, o = 0;
      int first = 1;
      lex_next(L);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      bag[0] = 0; oldv[0] = 0; newv[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, oldv, sizeof oldv) != 0) oldv[0] = 0;
      if (resolve_str_arg(vm, L, newv, sizeof newv) != 0) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long n = parse_expr(vm, L);
          snprintf(newv, sizeof newv, "%ld", n);
        } else {
          newv[0] = 0;
        }
      }
      nn = strlen(oldv);
      vlen = strlen(newv);
      if (!bag[0]) {
        hit = 0;
      } else {
        p = bag;
        while (*p) {
          int match = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (!hit && flen == nn) {
            if (!icase) {
              match = (nn == 0 || memcmp(start, oldv, nn) == 0) ? 1 : 0;
            } else {
              size_t i; match = 1;
              for (i = 0; i < nn; i++) {
                char a = start[i], b = oldv[i];
                if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                if (a != b) { match = 0; break; }
              }
            }
          }
          if (!first && o + 1 < sizeof out) out[o++] = '\n';
          first = 0;
          if (match) {
            hit = 1;
            if (o + vlen < sizeof out) {
              memcpy(out + o, newv, vlen);
              o += vlen;
            } else if (o < sizeof out - 1) {
              size_t take = sizeof out - 1 - o;
              memcpy(out + o, newv, take);
              o += take;
            }
          } else {
            if (o + flen < sizeof out) {
              memcpy(out + o, start, flen);
              o += flen;
            } else if (o < sizeof out - 1) {
              size_t take = sizeof out - 1 - o;
              memcpy(out + o, start, take);
              o += take;
            }
          }
          out[o] = 0;
          if (*p == '\n') p++;
        }
        if (!hit) {
          snprintf(out, sizeof out, "%s", bag);
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "SETMATCH_N", hit);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS INSERTLINE|INSLINE bag n value — insert field before 0-based index → LAST bag.
     * n < 0 soft miss (bag unchanged, LAST_N=0). n >= field count → append (like PUSH).
     * n == 0 → front (like PREPEND). LAST_N = field count after insert.
     * Usability: priority/ordered work bags without PREPEND+TAKE+DROP glue. */
    if (kw(&L->cur,"INSERTLINE") || kw(&L->cur,"INSLINE") || kw(&L->cur,"INSERTFIELD") ||
        kw(&L->cur,"LINEINSERT") || kw(&L->cur,"ADDAT") || kw(&L->cur,"INSERTAT") ||
        kw(&L->cur,"PUTAT") || kw(&L->cur,"SPLICEIN")){
      char bag[CUBALC_HOST_STR_MAX], val[512], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      long want = 0, cur = 0, nfields = 0, ok_ins = 0;
      size_t flen, vlen, o = 0;
      int first = 1, inserted = 0;
      lex_next(L);
      bag[0] = 0; val[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
        want = parse_expr(vm, L);
      else
        want = 0;
      if (resolve_str_arg(vm, L, val, sizeof val) != 0) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS || L->cur.kind == TK_LPAREN) {
          long n = parse_expr(vm, L);
          snprintf(val, sizeof val, "%ld", n);
        } else {
          val[0] = 0;
        }
      }
      vlen = strlen(val);
      if (want < 0) {
        snprintf(out, sizeof out, "%s", bag);
        ok_ins = 0;
        if (bag[0]) {
          p = bag;
          while (*p) {
            while (*p && *p != '\n') p++;
            nfields++;
            if (*p == '\n') p++;
          }
        }
      } else {
        ok_ins = 1;
        p = bag;
        /* empty bag: just the value */
        if (!bag[0]) {
          if (vlen < sizeof out) {
            memcpy(out, val, vlen);
            o = vlen;
          } else {
            size_t take = sizeof out - 1;
            memcpy(out, val, take);
            o = take;
          }
          out[o] = 0;
          inserted = 1;
          nfields = 1;
        } else {
          while (*p) {
            start = p;
            while (*p && *p != '\n') p++;
            flen = (size_t)(p - start);
            if (cur == want && !inserted) {
              if (!first && o + 1 < sizeof out) out[o++] = '\n';
              first = 0;
              if (o + vlen < sizeof out) {
                memcpy(out + o, val, vlen);
                o += vlen;
              } else if (o < sizeof out - 1) {
                size_t take = sizeof out - 1 - o;
                memcpy(out + o, val, take);
                o += take;
              }
              out[o] = 0;
              inserted = 1;
              nfields++;
            }
            if (!first && o + 1 < sizeof out) out[o++] = '\n';
            first = 0;
            if (o + flen < sizeof out) {
              memcpy(out + o, start, flen);
              o += flen;
            } else if (o < sizeof out - 1) {
              size_t take = sizeof out - 1 - o;
              memcpy(out + o, start, take);
              o += take;
            }
            out[o] = 0;
            nfields++;
            cur++;
            if (*p == '\n') p++;
          }
          if (!inserted) {
            /* n past end → append */
            if (!first && o + 1 < sizeof out) out[o++] = '\n';
            if (o + vlen < sizeof out) {
              memcpy(out + o, val, vlen);
              o += vlen;
            } else if (o < sizeof out - 1) {
              size_t take = sizeof out - 1 - o;
              memcpy(out + o, val, take);
              o += take;
            }
            out[o] = 0;
            inserted = 1;
            nfields++;
          }
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = ok_ins ? nfields : 0;
      var_set_num(vm, "LAST_N", ok_ins ? nfields : 0);
      var_set_num(vm, "INSERTLINE_N", ok_ins ? nfields : 0);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS DROPNTH|DROPAT bag n — drop 0-based field by index → LAST bag.
     * LAST_N = 1 if dropped, 0 soft miss (OOR/empty/neg; bag unchanged).
     * Distinct from SYS DROP n (drop first-n window) and REMOVELINE (by value).
     * Usability: FINDLINE then delete without needing exact needle after mutate. */
    if (kw(&L->cur,"DROPNTH") || kw(&L->cur,"DROPAT") || kw(&L->cur,"REMOVEAT") ||
        kw(&L->cur,"DELETEAT") || kw(&L->cur,"DELAT") || kw(&L->cur,"DROPINDEX") ||
        kw(&L->cur,"REMOVEINDEX") || kw(&L->cur,"DELETEINDEX") || kw(&L->cur,"ERASEN") ||
        kw(&L->cur,"LINEDEL") || kw(&L->cur,"DELLINEAT") || kw(&L->cur,"OMITN")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      long want = 0, cur = 0, hit = 0;
      size_t flen, o = 0;
      int first_kept = 1;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
        want = parse_expr(vm, L);
      else
        want = 0;
      if (want < 0 || !bag[0]) {
        snprintf(out, sizeof out, "%s", bag);
        hit = 0;
      } else {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (cur == want) {
            hit = 1;
            /* skip this field */
          } else {
            if (!first_kept && o + 1 < sizeof out) out[o++] = '\n';
            first_kept = 0;
            if (o + flen < sizeof out) {
              memcpy(out + o, start, flen);
              o += flen;
            } else if (o < sizeof out - 1) {
              size_t take = sizeof out - 1 - o;
              memcpy(out + o, start, take);
              o += take;
            }
            out[o] = 0;
          }
          cur++;
          if (*p == '\n') p++;
        }
        if (!hit) {
          snprintf(out, sizeof out, "%s", bag);
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "DROPNTH_N", hit);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MOVELINE|MOVEAT bag from to — move 0-based field to final index → LAST bag.
     * LAST_N = 1 success, 0 soft miss (from OOR / to<0 / empty). to >= n → end.
     * Usability: promote/demote priority work without EACH rebuild (FINDLINE then MOVE). */
    if (kw(&L->cur,"MOVELINE") || kw(&L->cur,"MOVEAT") || kw(&L->cur,"MOVEFIELD") ||
        kw(&L->cur,"LINEMOVE") || kw(&L->cur,"REORDER") || kw(&L->cur,"SHIFTTO") ||
        kw(&L->cur,"MOVEN") || kw(&L->cur,"RELOCATE")){
      enum { MV_MAX = 256, MV_FLEN = 192 };
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      char fields[MV_MAX][MV_FLEN];
      char held[MV_FLEN];
      const char *p, *start;
      long from_i = 0, to_i = 0, n = 0, hit = 0;
      size_t flen, o = 0;
      int i, first = 1;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
        from_i = parse_expr(vm, L);
      else
        from_i = 0;
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
        to_i = parse_expr(vm, L);
      else
        to_i = 0;
      if (bag[0]) {
        p = bag;
        while (*p && n < MV_MAX) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen >= MV_FLEN) flen = MV_FLEN - 1;
          memcpy(fields[n], start, flen);
          fields[n][flen] = 0;
          n++;
          if (*p == '\n') p++;
        }
      }
      if (n <= 0 || from_i < 0 || from_i >= n || to_i < 0) {
        snprintf(out, sizeof out, "%s", bag);
        hit = 0;
      } else {
        hit = 1;
        if (to_i >= n) to_i = n - 1;
        if (from_i != to_i) {
          snprintf(held, sizeof held, "%s", fields[from_i]);
          if (from_i < to_i) {
            for (i = (int)from_i; i < (int)to_i; i++)
              memcpy(fields[i], fields[i + 1], MV_FLEN);
            memcpy(fields[to_i], held, MV_FLEN);
          } else {
            for (i = (int)from_i; i > (int)to_i; i--)
              memcpy(fields[i], fields[i - 1], MV_FLEN);
            memcpy(fields[to_i], held, MV_FLEN);
          }
        }
        o = 0; first = 1;
        for (i = 0; i < (int)n; i++) {
          flen = strlen(fields[i]);
          if (!first && o + 1 < sizeof out) out[o++] = '\n';
          first = 0;
          if (o + flen < sizeof out) {
            memcpy(out + o, fields[i], flen);
            o += flen;
          } else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, fields[i], take);
            o += take;
          }
          out[o] = 0;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "MOVELINE_N", hit);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS REMOVELINE|DROPLINE bag needle — drop first exact field match → LAST bag.
     * LAST_N = 1 if removed, 0 if miss (bag unchanged). REST not used.
     * SYS REMOVELINEI — case-insensitive exact field.
     * Usability: dequeue/ack work items after HASLINE without EACH rebuild. */
    if (kw(&L->cur,"REMOVELINE") || kw(&L->cur,"DROPLINE") || kw(&L->cur,"DELETELINE") ||
        kw(&L->cur,"REMOVEFIELD") || kw(&L->cur,"DROPFIELD") || kw(&L->cur,"UNLINE") ||
        kw(&L->cur,"REMOVELINEI") || kw(&L->cur,"DROPLINEI") || kw(&L->cur,"DELETELINEI") ||
        kw(&L->cur,"DROPFIELDI")){
      char op[20]; snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      int icase = (strcmp(op, "REMOVELINEI") == 0 || strcmp(op, "DROPLINEI") == 0 ||
                   strcmp(op, "DELETELINEI") == 0 || strcmp(op, "DROPFIELDI") == 0);
      char bag[CUBALC_HOST_STR_MAX], needle[512], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      long removed = 0;
      size_t nn, flen, o = 0;
      int first_kept = 1;
      lex_next(L);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      bag[0] = 0; needle[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0] = 0;
      nn = strlen(needle);
      p = bag;
      while (*p) {
        int match = 0;
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (!removed && flen == nn) {
          if (!icase) {
            match = (nn == 0 || memcmp(start, needle, nn) == 0) ? 1 : 0;
          } else {
            size_t i; match = 1;
            for (i = 0; i < nn; i++) {
              char a = start[i], b = needle[i];
              if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
              if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
              if (a != b) { match = 0; break; }
            }
          }
        }
        if (match) {
          removed = 1;
        } else {
          if (!first_kept && o + 1 < sizeof out) out[o++] = '\n';
          first_kept = 0;
          if (o + flen < sizeof out) {
            memcpy(out + o, start, flen);
            o += flen;
          } else if (o < sizeof out - 1) {
            size_t take = sizeof out - 1 - o;
            memcpy(out + o, start, take);
            o += take;
          }
          out[o] = 0;
        }
        if (*p == '\n') p++;
      }
      if (!removed) {
        /* keep original bag */
        snprintf(out, sizeof out, "%s", bag);
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = removed;
      var_set_num(vm, "LAST_N", removed);
      var_set_num(vm, "REMOVE_N", removed);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS EQS|STREQ a b — string equality → LAST_N 1/0
     * SYS EQSI|IEQS|STREQI — case-insensitive equality (IF after GREPI).
     * SYS EQS I|ICASE a b — same as EQSI. */
    if (kw(&L->cur,"EQS") || kw(&L->cur,"STREQ") || kw(&L->cur,"SEQ") ||
        kw(&L->cur,"EQSI") || kw(&L->cur,"IEQS") || kw(&L->cur,"STREQI") ||
        kw(&L->cur,"SEQI") || kw(&L->cur,"CIEQ")){
      int icase = (kw(&L->cur,"EQSI") || kw(&L->cur,"IEQS") ||
                   kw(&L->cur,"STREQI") || kw(&L->cur,"SEQI") ||
                   kw(&L->cur,"CIEQ"));
      lex_next(L);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      char a[512]="", b[512]="";
      if (resolve_str_arg(vm, L, a, sizeof a) != 0)
        snprintf(a, sizeof a, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, b, sizeof b) != 0) b[0]=0;
      long eq;
      if (!icase) {
        eq = (strcmp(a, b) == 0) ? 1 : 0;
      } else {
        size_t i;
        eq = 1;
        for (i = 0; ; i++) {
          char ca = a[i], cb = b[i];
          if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
          if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
          if (ca != cb) { eq = 0; break; }
          if (!a[i]) break;
        }
      }
      vm->last_n = eq;
      var_set_num(vm, "LAST_N", eq);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS HAS|CONTAINS hay needle — 1 if needle in hay
     * SYS HASI|ICONTAINS|CONTAINSI — case-insensitive substring.
     * SYS HAS I|ICASE hay needle — same as HASI. */
    if (kw(&L->cur,"HAS") || kw(&L->cur,"CONTAINS") || kw(&L->cur,"INSTR") ||
        kw(&L->cur,"HASI") || kw(&L->cur,"ICONTAINS") || kw(&L->cur,"CONTAINSI") ||
        kw(&L->cur,"IHAS") || kw(&L->cur,"INSTRI")){
      int icase = (kw(&L->cur,"HASI") || kw(&L->cur,"ICONTAINS") ||
                   kw(&L->cur,"CONTAINSI") || kw(&L->cur,"IHAS") ||
                   kw(&L->cur,"INSTRI"));
      lex_next(L);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      char hay[512]="", needle[256]="";
      if (resolve_str_arg(vm, L, hay, sizeof hay) != 0)
        snprintf(hay, sizeof hay, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0]=0;
      long hit = 0;
      if (!needle[0]) {
        hit = 1;
      } else if (!icase) {
        hit = (strstr(hay, needle) != NULL) ? 1 : 0;
      } else {
        size_t nl = strlen(needle), fi;
        for (fi = 0; hay[fi] && !hit; fi++) {
          size_t j;
          for (j = 0; j < nl; j++) {
            char ca = hay[fi + j], cb = needle[j];
            if (!ca) break;
            if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
            if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
            if (ca != cb) break;
          }
          if (j == nl) hit = 1;
        }
      }
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS BEFORE|AFTER hay needle — peel at first needle (log/kv without FIND+MID).
     * BEFORE → text left of first match (whole hay if miss).
     * AFTER  → text right of first match (empty if miss).
     * LAST_N = 1 if needle found, 0 if miss; empty needle → found at 0. */
    /* Note: do not alias POST — SYS POST is HTTP POST already. */
    if (kw(&L->cur,"BEFORE") || kw(&L->cur,"LEFT_OF") || kw(&L->cur,"LEFTOF") ||
        kw(&L->cur,"PRE") || kw(&L->cur,"HEADOF") || kw(&L->cur,"SPLITLEFT") ||
        kw(&L->cur,"AFTER") || kw(&L->cur,"RIGHT_OF") || kw(&L->cur,"RIGHTOF") ||
        kw(&L->cur,"TAILOF") || kw(&L->cur,"SPLITRIGHT")){
      char op[16]; snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *p = op; *p; p++)
        if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 'a' + 'A');
      int want_after = (strcmp(op, "AFTER") == 0 || strcmp(op, "RIGHT_OF") == 0 ||
                        strcmp(op, "RIGHTOF") == 0 ||
                        strcmp(op, "TAILOF") == 0 || strcmp(op, "SPLITRIGHT") == 0);
      lex_next(L);
      char hay[512] = "", needle[256] = "";
      if (resolve_str_arg(vm, L, hay, sizeof hay) != 0)
        snprintf(hay, sizeof hay, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0] = 0;
      char out[512];
      long found = 0;
      if (needle[0] == 0) {
        /* empty needle matches at start */
        found = 1;
        if (want_after) snprintf(out, sizeof out, "%s", hay);
        else out[0] = 0;
      } else {
        const char *p = strstr(hay, needle);
        if (!p) {
          found = 0;
          if (want_after) out[0] = 0;
          else snprintf(out, sizeof out, "%s", hay);
        } else {
          found = 1;
          if (want_after) {
            snprintf(out, sizeof out, "%s", p + strlen(needle));
          } else {
            size_t pre = (size_t)(p - hay);
            if (pre >= sizeof out) pre = sizeof out - 1;
            memcpy(out, hay, pre);
            out[pre] = 0;
          }
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS BETWEEN|MIDOF|EXTRACT open close [hay] — peel text between delimiters.
     * Completes BEFORE/AFTER for plate/kv/log/JSON-ish extract without FIND+MID.
     * LAST = interior; LAST_N = 1 if pair found, 0 soft miss (OK stays 1).
     * Empty open → match at start; empty close → rest after open. */
    if (kw(&L->cur,"BETWEEN") || kw(&L->cur,"MIDOF") || kw(&L->cur,"EXTRACT") ||
        kw(&L->cur,"INSIDE") || kw(&L->cur,"STRBETWEEN") || kw(&L->cur,"BETWEENSTR") ||
        kw(&L->cur,"INNER")){
      lex_next(L);
      char open[256] = "", close[256] = "", hay[512] = "";
      if (resolve_str_arg(vm, L, open, sizeof open) != 0) open[0] = 0;
      if (resolve_str_arg(vm, L, close, sizeof close) != 0) close[0] = 0;
      if (resolve_str_arg(vm, L, hay, sizeof hay) != 0)
        snprintf(hay, sizeof hay, "%s", vm->last_str);
      char out[512];
      long found = 0;
      out[0] = 0;
      {
        const char *po, *start, *pc;
        size_t n;
        if (open[0] == 0) {
          po = hay;
          start = hay;
        } else {
          po = strstr(hay, open);
          if (!po) {
            var_set_str(vm, "LAST", "");
            snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
            vm->last_n = 0;
            var_set_num(vm, "LAST_N", 0);
            var_set_num(vm, "OK", 1);
            bump(vm); return 1;
          }
          start = po + strlen(open);
        }
        if (close[0] == 0) {
          pc = start + strlen(start);
          found = 1;
        } else {
          pc = strstr(start, close);
          if (!pc) {
            var_set_str(vm, "LAST", "");
            snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
            vm->last_n = 0;
            var_set_num(vm, "LAST_N", 0);
            var_set_num(vm, "OK", 1);
            bump(vm); return 1;
          }
          found = 1;
        }
        n = (size_t)(pc - start);
        if (n >= sizeof out) n = sizeof out - 1;
        memcpy(out, start, n);
        out[n] = 0;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS REVS|STRREV [str] — reverse string → LAST (not cube REVERSE) */
    if (kw(&L->cur,"REVS") || kw(&L->cur,"STRREV") || kw(&L->cur,"SREV")){
      lex_next(L);
      char s[512]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      size_t n = strlen(s);
      char out[512];
      if (n >= sizeof out) n = sizeof out - 1;
      for (size_t i=0;i<n;i++) out[i] = s[n-1-i];
      out[n] = 0;
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)n;
      var_set_num(vm, "LAST_N", (long)n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS UPPER|UCASE [str] — ASCII upper → LAST */
    if (kw(&L->cur,"UPPER") || kw(&L->cur,"UCASE") || kw(&L->cur,"TOUPPER")){
      lex_next(L);
      char s[512]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      for (char *p=s; *p; p++)
        if (*p >= 'a' && *p <= 'z') *p = (char)(*p - 'a' + 'A');
      var_set_str(vm, "LAST", s);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", s);
      vm->last_n = (long)strlen(s);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS LOWER|LCASE [str] — ASCII lower → LAST */
    if (kw(&L->cur,"LOWER") || kw(&L->cur,"LCASE") || kw(&L->cur,"TOLOWER")){
      lex_next(L);
      char s[512]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      for (char *p=s; *p; p++)
        if (*p >= 'A' && *p <= 'Z') *p = (char)(*p - 'A' + 'a');
      var_set_str(vm, "LAST", s);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", s);
      vm->last_n = (long)strlen(s);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* digit-7 string plane: TRIM / STARTS / ENDS / REPLACE */
    if (kw(&L->cur,"TRIM") || kw(&L->cur,"STRIP") ||
        kw(&L->cur,"LTRIM") || kw(&L->cur,"RTRIM")){
      char op[12]; snprintf(op,sizeof op,"%s",L->cur.text);
      for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
      lex_next(L);
      char s[512]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      char *a = s, *b = s + strlen(s);
      int do_l = (strcmp(op,"TRIM")==0 || strcmp(op,"STRIP")==0 || strcmp(op,"LTRIM")==0);
      int do_r = (strcmp(op,"TRIM")==0 || strcmp(op,"STRIP")==0 || strcmp(op,"RTRIM")==0);
      if (do_l) while (*a==' '||*a=='\t'||*a=='\n'||*a=='\r') a++;
      if (do_r){
        while (b > a && (b[-1]==' '||b[-1]=='\t'||b[-1]=='\n'||b[-1]=='\r')) b--;
      }
      char out[512];
      size_t n = (size_t)(b - a);
      if (n >= sizeof out) n = sizeof out - 1;
      memcpy(out, a, n); out[n] = 0;
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)n;
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS STARTS|ENDS hay needle — prefix/suffix probe → LAST_N 0|1
     * SYS STARTSI|ENDSI — case-insensitive (path/ext/severity without UPPER glue).
     * SYS STARTS I / ENDS ICASE forms. Note: SYS SUFFIX is EXTNAME (path plane). */
    if (kw(&L->cur,"STARTS") || kw(&L->cur,"STARTSWITH") || kw(&L->cur,"HASPREFIX") ||
        kw(&L->cur,"PREFIX") ||
        kw(&L->cur,"STARTSI") || kw(&L->cur,"ISTARTS") || kw(&L->cur,"STARTSWITHI") ||
        kw(&L->cur,"HASPREFIXI") || kw(&L->cur,"IPREFIX") ||
        kw(&L->cur,"ENDS") || kw(&L->cur,"ENDSWITH") || kw(&L->cur,"HASSUFFIX") ||
        kw(&L->cur,"ENDSI") || kw(&L->cur,"IENDS") || kw(&L->cur,"ENDSWITHI") ||
        kw(&L->cur,"HASSUFFIXI") || kw(&L->cur,"ISUFFIX")){
      char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
      for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
      int is_end = (strcmp(op,"ENDS")==0 || strcmp(op,"ENDSWITH")==0 ||
                    strcmp(op,"HASSUFFIX")==0 ||
                    strcmp(op,"ENDSI")==0 || strcmp(op,"IENDS")==0 ||
                    strcmp(op,"ENDSWITHI")==0 || strcmp(op,"HASSUFFIXI")==0 ||
                    strcmp(op,"ISUFFIX")==0);
      int icase = (strcmp(op,"STARTSI")==0 || strcmp(op,"ISTARTS")==0 ||
                   strcmp(op,"STARTSWITHI")==0 || strcmp(op,"HASPREFIXI")==0 ||
                   strcmp(op,"IPREFIX")==0 ||
                   strcmp(op,"ENDSI")==0 || strcmp(op,"IENDS")==0 ||
                   strcmp(op,"ENDSWITHI")==0 || strcmp(op,"HASSUFFIXI")==0 ||
                   strcmp(op,"ISUFFIX")==0);
      lex_next(L);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      char hay[512]="", pref[256]="";
      if (resolve_str_arg(vm, L, hay, sizeof hay) != 0)
        snprintf(hay, sizeof hay, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, pref, sizeof pref) != 0) pref[0]=0;
      long hit = 0;
      size_t hn = strlen(hay), pn = strlen(pref);
      if (pn == 0) hit = 1;
      else if (pn <= hn){
        if (!icase) {
          if (!is_end) hit = (strncmp(hay, pref, pn) == 0) ? 1 : 0;
          else hit = (strcmp(hay + (hn - pn), pref) == 0) ? 1 : 0;
        } else {
          size_t i, base = is_end ? (hn - pn) : 0;
          hit = 1;
          for (i = 0; i < pn; i++) {
            char a = hay[base + i], b = pref[i];
            if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
            if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
            if (a != b) { hit = 0; break; }
          }
        }
      }
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"REPLACE") || kw(&L->cur,"SUBST") || kw(&L->cur,"STRREP") ||
        kw(&L->cur,"REPLACEALL") || kw(&L->cur,"GSUB") || kw(&L->cur,"SUBSTALL") ||
        kw(&L->cur,"STRREPALL") || kw(&L->cur,"REPALL")){
      /* SYS REPLACE hay old new — first occurrence.
       * SYS REPLACE ALL hay old new | SYS REPLACEALL|GSUB — all occurrences.
       * LAST = result; LAST_N = number of replacements (0 if none).
       * Usability: plate templates {{DATE}}/{{HOST}} without shell sed. */
      int do_all = (kw(&L->cur,"REPLACEALL") || kw(&L->cur,"GSUB") ||
                    kw(&L->cur,"SUBSTALL") || kw(&L->cur,"STRREPALL") ||
                    kw(&L->cur,"REPALL"));
      lex_next(L);
      if (!do_all && (kw(&L->cur,"ALL") || kw(&L->cur,"GLOBAL") ||
                      kw(&L->cur,"EVERY") || kw(&L->cur,"G"))){
        do_all = 1;
        lex_next(L);
      }
      char hay[512]="", olds[256]="", news[256]="";
      if (resolve_str_arg(vm, L, hay, sizeof hay) != 0)
        snprintf(hay, sizeof hay, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, olds, sizeof olds) != 0) olds[0]=0;
      if (resolve_str_arg(vm, L, news, sizeof news) != 0) news[0]=0;
      char out[1024];
      long did = 0;
      size_t oldn = strlen(olds), newn = strlen(news);
      if (olds[0] == 0){
        snprintf(out, sizeof out, "%s", hay);
      } else {
        const char *src = hay;
        size_t o = 0;
        out[0] = 0;
        for (;;) {
          const char *p = strstr(src, olds);
          if (!p) {
            size_t rest = strlen(src);
            if (o + rest >= sizeof out) rest = sizeof out - 1 - o;
            memcpy(out + o, src, rest);
            o += rest;
            out[o] = 0;
            break;
          }
          {
            size_t pre = (size_t)(p - src);
            if (o + pre >= sizeof out) pre = sizeof out - 1 - o;
            memcpy(out + o, src, pre); o += pre;
            if (o + newn < sizeof out) {
              memcpy(out + o, news, newn); o += newn;
            } else if (o < sizeof out - 1) {
              size_t take = sizeof out - 1 - o;
              memcpy(out + o, news, take); o += take;
            }
            out[o] = 0;
            did++;
            src = p + oldn;
            /* empty old would infinite-loop; already guarded by olds[0] */
            if (!do_all) {
              size_t rest = strlen(src);
              if (o + rest >= sizeof out) rest = sizeof out - 1 - o;
              memcpy(out + o, src, rest);
              o += rest;
              out[o] = 0;
              break;
            }
            if (o >= sizeof out - 1) break;
          }
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = did;
      var_set_num(vm, "LAST_N", did);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* digit-3 string pad/repeat: LPAD RPAD STREPEAT (SYS REPEAT alias) */
    if (kw(&L->cur,"LPAD") || kw(&L->cur,"RPAD") || kw(&L->cur,"PADLEFT") ||
        kw(&L->cur,"PADRIGHT") || kw(&L->cur,"STRPAD") ||
        kw(&L->cur,"STREPEAT") || kw(&L->cur,"STRREPEAT") ||
        kw(&L->cur,"REPEATSTR") || kw(&L->cur,"SREPEAT")){
      char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
      for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
      int is_rep = (strcmp(op,"STREPEAT")==0 || strcmp(op,"STRREPEAT")==0 ||
                    strcmp(op,"REPEATSTR")==0 || strcmp(op,"SREPEAT")==0);
      int is_left = (strcmp(op,"LPAD")==0 || strcmp(op,"PADLEFT")==0 ||
                     strcmp(op,"STRPAD")==0);
      lex_next(L);
      char s[512]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      if (is_rep){
        long times = 0;
        if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
            L->cur.kind==TK_IDENT)
          times = parse_expr(vm, L);
        if (times < 0) times = 0;
        if (times > 512) times = 512;
        char out[512]; size_t o = 0;
        size_t sn = strlen(s);
        if (sn == 0){ out[0]=0; }
        else {
          for (long t=0; t<times && o+1 < sizeof out; t++){
            size_t take = sn;
            if (o + take >= sizeof out) take = sizeof out - 1 - o;
            memcpy(out + o, s, take); o += take;
          }
          out[o] = 0;
        }
        var_set_str(vm, "LAST", out);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
        vm->last_n = (long)o;
        var_set_num(vm, "LAST_N", vm->last_n);
        var_set_num(vm, "OK", 1);
        bump(vm); return 1;
      }
      /* LPAD/RPAD str width [padchar] */
      long width = 0;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          L->cur.kind==TK_IDENT)
        width = parse_expr(vm, L);
      if (width < 0) width = 0;
      if (width > 511) width = 511;
      char padc = ' ';
      char pad[16]; pad[0]=0;
      if (resolve_str_arg(vm, L, pad, sizeof pad) == 0 && pad[0])
        padc = pad[0];
      size_t sn = strlen(s);
      char out[512];
      if ((long)sn >= width){
        size_t take = (size_t)width;
        if (take >= sizeof out) take = sizeof out - 1;
        memcpy(out, s, take); out[take] = 0;
      } else {
        size_t need = (size_t)width - sn;
        if (is_left){
          size_t o = 0;
          for (size_t i=0; i<need && o+1 < sizeof out; i++) out[o++] = padc;
          size_t take = sn;
          if (o + take >= sizeof out) take = sizeof out - 1 - o;
          memcpy(out + o, s, take); o += take; out[o] = 0;
        } else {
          size_t o = 0;
          size_t take = sn;
          if (take >= sizeof out) take = sizeof out - 1;
          memcpy(out, s, take); o = take;
          for (size_t i=0; i<need && o+1 < sizeof out; i++) out[o++] = padc;
          out[o] = 0;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)strlen(out);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* digit-3 string duals: LEFT/RIGHT slice + COUNT occurrences */
    if (kw(&L->cur,"LEFT") || kw(&L->cur,"STRLEFT") || kw(&L->cur,"TAKELEFT") ||
        kw(&L->cur,"PREFIXN") ||
        kw(&L->cur,"RIGHT") || kw(&L->cur,"STRRIGHT") || kw(&L->cur,"TAKERIGHT") ||
        kw(&L->cur,"SUFFIXN")){
      char op[16]; snprintf(op,sizeof op,"%s",L->cur.text);
      for (char *p=op;*p;p++) if (*p>='a'&&*p<='z') *p=(char)(*p-'a'+'A');
      int is_right = (strcmp(op,"RIGHT")==0 || strcmp(op,"STRRIGHT")==0 ||
                      strcmp(op,"TAKERIGHT")==0 || strcmp(op,"SUFFIXN")==0);
      lex_next(L);
      char s[512]; s[0]=0;
      if (resolve_str_arg(vm, L, s, sizeof s) != 0)
        snprintf(s, sizeof s, "%s", vm->last_str);
      long n = 0;
      if (L->cur.kind==TK_NUM || L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS ||
          L->cur.kind==TK_IDENT)
        n = parse_expr(vm, L);
      if (n < 0) n = 0;
      size_t slen = strlen(s);
      char out[512];
      if (!is_right){
        size_t take = (size_t)n;
        if (take > slen) take = slen;
        if (take >= sizeof out) take = sizeof out - 1;
        memcpy(out, s, take); out[take] = 0;
      } else {
        size_t take = (size_t)n;
        if (take > slen) take = slen;
        if (take >= sizeof out) take = sizeof out - 1;
        size_t start = slen - take;
        memcpy(out, s + start, take); out[take] = 0;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)strlen(out);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    if (kw(&L->cur,"COUNT") || kw(&L->cur,"STRCOUNT") || kw(&L->cur,"COUNTSTR") ||
        kw(&L->cur,"OCCURS") || kw(&L->cur,"COUNTOCC")){
      lex_next(L);
      char hay[512]="", needle[256]="";
      if (resolve_str_arg(vm, L, hay, sizeof hay) != 0)
        snprintf(hay, sizeof hay, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0]=0;
      long cnt = 0;
      if (needle[0] == 0){
        cnt = (long)strlen(hay);
      } else {
        size_t nn = strlen(needle);
        const char *p = hay;
        while ((p = strstr(p, needle)) != NULL){
          cnt++;
          p += nn;
        }
      }
      vm->last_n = cnt;
      var_set_num(vm, "LAST_N", cnt);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    fail(vm, "SYS: READ|WRITE|RM|RENAME|COPY|REALPATH|TOUCH|LIST|NTH|GREP|TAKE|DROP|SPLIT|WORDS|CUT|COLUMN|SORT|UNIQ|REVL|JOINLINES|PUSH|PREPEND|POP|POPHEAD|LINES|HASLINE|COUNTLINE|FINDLINE|SETLINE|SETMATCH|INSERTLINE|DROPNTH|MOVELINE|REMOVELINE|ENV|SETENV|UNSETENV|EXIST|SIZE|ISDIR|ISFILE|MTIME|AGE|MKDIR|BASENAME|DIRNAME|EXTNAME|STEM|WHICH|CWD|CHDIR|STATE|ROOT|TMP|HTTP|SPAWN|JOIN|JSON|CHAT|ARG|NUM|STR|ITOA|LEN|EMPTY|BLANK|TIME|MS|SLEEP|RAND|DATE|PID|HOSTNAME|USER|UID|HOME|APPEND|HEX|TOHEX|ORD|CHR|MID|CAT|FIND|FINDI|NTH|EQS|EQSI|HAS|HASI|BEFORE|AFTER|BETWEEN|REVS|UPPER|LOWER|TRIM|STARTS|STARTSI|ENDS|ENDSI|REPLACE|REPLACEALL|LPAD|RPAD|STREPEAT");
    return -1;
  }

  /* ---- legacy verbose (compat) ---- */
  if (kw(&L->cur,"CREED")){
    lex_next(L);
    if (L->cur.kind==TK_STR){
      snprintf(vm->creed,sizeof vm->creed,"%s",L->cur.text);
      snprintf(vm->ch.creed,sizeof vm->ch.creed,"%s",L->cur.text);
      lex_next(L);
    }
    bump(vm); return 1;
  }
  if (kw(&L->cur,"HOLD_FLASH")){
    lex_next(L);
    if (L->cur.kind==TK_NUM){ vm->hold_flash=L->cur.num?1:0; vm->ch.hold_flash=(uint8_t)vm->hold_flash; lex_next(L); }
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SHARE")||kw(&L->cur,"BUDGET")){
    lex_next(L); while (L->cur.kind!=TK_NL && L->cur.kind!=TK_EOF) lex_next(L);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"GENESIS")){
    lex_next(L);
    char plate[512]="NEXUS_COORD v1 | from=play | hold_flash=1 |";
    if (kw(&L->cur,"FROM")){ lex_next(L); if (L->cur.kind==TK_IDENT) lex_next(L); }
    else if (L->cur.kind==TK_STR){ snprintf(plate,sizeof plate,"%s",L->cur.text); lex_next(L); }
    cubalc_matrix gen; cubalc_coord_to_matrix(plate,&gen);
    cubalc_chain_from_initial(&vm->ch,&gen,1);
    vm->ch.hold_flash=(uint8_t)vm->hold_flash;
    bump(vm); return 1;
  }
  /* SCIENCE [LOAD] — inject pure-science constants (public domain; language design) */
  if (kw(&L->cur,"SCIENCE")||kw(&L->cur,"PURESCIENCE")){
    lex_next(L);
    if (kw(&L->cur,"LOAD")||kw(&L->cur,"CONST")||kw(&L->cur,"CONSTANTS")) lex_next(L);
    var_set_num(vm, "PI100", CUBALC_SCI_PI100);
    var_set_num(vm, "E100", CUBALC_SCI_E100);
    var_set_num(vm, "G_EARTH", CUBALC_SCI_G_EARTH10);
    var_set_num(vm, "C_LIGHT", CUBALC_SCI_C_LIGHT);
    var_set_num(vm, "ATM_KPA", CUBALC_SCI_ATM_KPA);
    var_set_num(vm, "WATER_K", CUBALC_SCI_WATER_K);
    var_set_num(vm, "H2O_BP", CUBALC_SCI_H2O_BP_C);
    var_set_num(vm, "R_GAS", CUBALC_SCI_R_J);
    var_set_num(vm, "NA_ORDER", CUBALC_SCI_AVOGADRO_E23);
    var_set_num(vm, "EARTH_R", CUBALC_SCI_EARTH_R_KM);
    var_set_num(vm, "AU_KM", CUBALC_SCI_AU_KM);
    var_set_num(vm, "YEAR_D", CUBALC_SCI_YEAR_D);
    var_set_num(vm, "MOON_D", CUBALC_SCI_MOON_D);
    var_set_num(vm, "SOLAR_C", CUBALC_SCI_SOLAR_C);
    var_set_num(vm, "ATM_O2", CUBALC_SCI_ATM_O2_PCT);
    var_set_num(vm, "ATM_N2", CUBALC_SCI_ATM_N2_PCT);
    var_set_num(vm, "OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"CUBE")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"CUBE id"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    char role[48]; snprintf(role,sizeof role,"%s",id); int proton=1;
    while (L->cur.kind==TK_IDENT){
      if (kw(&L->cur,"ROLE")){ lex_next(L); if (L->cur.kind==TK_IDENT){ snprintf(role,sizeof role,"%s",L->cur.text); lex_next(L);} }
      else if (kw(&L->cur,"PROTON")){ lex_next(L); if (L->cur.kind==TK_NUM){ proton=L->cur.num?1:0; lex_next(L);} }
      else break;
    }
    place_cube(vm,id,role,proton); bump(vm); return 1;
  }
  /* PLUG — only cubes; pluggable I/O wire (matrix-compatible) */
  if (kw(&L->cur,"PLUG")||kw(&L->cur,"WIRE")||kw(&L->cur,"IO_PLUG")){
    lex_next(L);
    if (kw(&L->cur,"RING")){ lex_next(L); do_ring(vm); bump(vm); return 1; }
    if (L->cur.kind!=TK_IDENT){ fail(vm,"PLUG cube_a cube_b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"PLUG cube_b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    do_plug(vm,a,b); bump(vm); return 1;
  }
  /* UNPLUG a b — detach pluggable I/O */
  if (kw(&L->cur,"UNPLUG")||kw(&L->cur,"DETACH")||kw(&L->cur,"DISCONNECT")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"UNPLUG a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"UNPLUG a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    do_unplug(vm,a,b); bump(vm); return 1;
  }
  /* REVERSE a b — reverse pluggable I/O direction (IN↔OUT) */
  if (kw(&L->cur,"REVERSE")||kw(&L->cur,"REV")||kw(&L->cur,"FLIP_IO")||kw(&L->cur,"IOR")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"REVERSE a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"REVERSE a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    do_reverse(vm,a,b); bump(vm); return 1;
  }
  /* IO cube IN|OUT [face] — declare pluggable port direction on a cube */
  if (kw(&L->cur,"IO")||kw(&L->cur,"PORT")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"IO cube IN|OUT [face]"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int is_out = 1;
    if (kw(&L->cur,"IN")||kw(&L->cur,"RECV")||kw(&L->cur,"RX")){ is_out=0; lex_next(L); }
    else if (kw(&L->cur,"OUT")||kw(&L->cur,"EMIT")||kw(&L->cur,"TX")||kw(&L->cur,"SEND")){ is_out=1; lex_next(L); }
    int face = 0;
    if (L->cur.kind==TK_NUM){ face=(int)L->cur.num; lex_next(L); }
    do_io(vm, id, face, is_out); bump(vm); return 1;
  }
  /* NEST parent child — cubes may nest */
  if (kw(&L->cur,"NEST")||kw(&L->cur,"INSIDE")||kw(&L->cur,"CONTAIN")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"NEST parent child"); return -1; }
    char p[48]; snprintf(p,sizeof p,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"NEST parent child"); return -1; }
    char c[48]; snprintf(c,sizeof c,"%s",L->cur.text); lex_next(L);
    do_nest(vm, p, c); bump(vm); return 1;
  }
  /* UNNEST child — detach from parent */
  if (kw(&L->cur,"UNNEST")||kw(&L->cur,"EJECT")||kw(&L->cur,"DETACH_NEST")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"UNNEST child"); return -1; }
    char c[48]; snprintf(c,sizeof c,"%s",L->cur.text); lex_next(L);
    do_unnest(vm, c); bump(vm); return 1;
  }
  /* COMPILE cube | COMPILE ALL — each cube → matrix; no flow → no compile */
  if (kw(&L->cur,"COMPILE")||kw(&L->cur,"TOMATRIX")||kw(&L->cur,"MATERIALIZE")){
    lex_next(L);
    if (kw(&L->cur,"ALL")||kw(&L->cur,"CHAIN")||kw(&L->cur,"WORLD")){
      lex_next(L);
      do_compile_all(vm); bump(vm); return 1;
    }
    if (L->cur.kind!=TK_IDENT){ fail(vm,"COMPILE cube|ALL"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    do_compile_cube(vm, id); bump(vm); return 1;
  }
  if (kw(&L->cur,"IMPULSE")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"IMPULSE"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    int p=1; if (L->cur.kind==TK_NUM){ p=L->cur.num?1:0; lex_next(L); }
    cubalc_chain_impulse(&vm->ch,id,(uint8_t)p); bump(vm); return 1;
  }
  /* FLOW [DIR|IO] n — free-flow energy; DIR respects OUT→IN only (pluggable I/O) */
  if (kw(&L->cur,"FLOW")||kw(&L->cur,"TICK")){
    lex_next(L);
    int directed = 0;
    if (kw(&L->cur,"DIR")||kw(&L->cur,"DIRECTED")||kw(&L->cur,"IO")||kw(&L->cur,"OUT")){
      directed = 1; lex_next(L);
    }
    int n=8;
    if (L->cur.kind==TK_NUM){ n=(int)L->cur.num; lex_next(L); }
    if (n < 1) n = 1;
    if (n > 1000) n = 1000;
    ensure_world(vm);
    if (directed) {
      for (int i = 0; i < n; i++) cubalc_chain_flow_directed(&vm->ch);
    } else {
      do_flow(vm, n);
    }
    bump(vm); return 1;
  }
  if (kw(&L->cur,"OS_ASPECTS")||kw(&L->cur,"SPAWN_OS")){
    lex_next(L); ensure_world(vm); cubalc_chain_os_aspects(&vm->ch); bump(vm); return 1;
  }
  /* HELP [prefix|form] — in-program form discovery for agents/humans
   * Sets LAST to first match "NAME: hint", LAST_N=len, OK=1 if found.
   * Bare HELP → catalog tip pointing at cubalc forms CLI. */
  if (kw(&L->cur,"HELP")||kw(&L->cur,"MAN")||kw(&L->cur,"DOC")){
    static const struct { const char *name; const char *hint; } help[] = {
      {"HOLD_FLASH", "HOLD_FLASH 1 — user permission BEFORE PLUG (not auto-flash)"},
      {"CUBE", "CUBE name ROLE host|body PROTON 0|1"},
      {"PLUG", "PLUG a b — wire cubes (needs HOLD_FLASH 1)"},
      {"IMPULSE", "IMPULSE cube [0|1] — pulse proton"},
      {"FLOW", "FLOW n — board ticks"},
      {"SETBIT", "SETBIT cube idx 0|1"},
      {"SETDIGIT", "SETDIGIT cube 0..9"},
      {"FOLDBITS", "FOLDBITS cube bits — fold 01 stream into matrix"},
      {"DECIDE", "DECIDE [cube] — matrix → algocube digit 0..9"},
      {"ASSERT", "ASSERT expr [\"why\"] — fail with line + reason"},
      {"EXPECT", "EXPECT expr [\"why\"] — soft check; OK/LAST_ERR, no fatal"},
      {"FAIL", "FAIL [\"why\"] — soft status OK=0 sticky LAST_ERR, no fatal"},
      {"PASS", "PASS [\"why\"] — soft status OK=1 optional LAST note"},
      {"NOTE", "NOTE [\"text\"] — agent breadcrumb · LAST/NOTE · no OK/ERR change"},
      {"EXIT", "EXIT [code] [\"why\"] — halt program; non-zero fails plate + process rc"},
      {"CLEAR_ERR", "CLEAR_ERR [note] — wipe sticky ERR/LAST_ERR after soft recovery"},
      {"VERSION", "VERSION — set LAST/VERSION to language version string"},
      {"REQUIRE", "REQUIRE VERSION x.y | REQUIRE LIB name — fail-fast gates"},
      {"PRINT", "PRINT str|expr…"},
      {"PRINT_JSON", "PRINT_JSON [idents] — one JSON line for agents"},
      {"DUMP", "DUMP — alias of PRINT_JSON"},
      {"VARS", "VARS — dump all program vars as cubalc.vars.v1 JSON"},
      {"STATUS", "STATUS — cubalc.status.v1 health plate (ok/last_err/version/time)"},
      {"IDENTITY", "IDENTITY — cubalc.identity.v1 plate (user@host:pid + vars)"},
      {"INCLUDE", "INCLUDE [ONCE] [OR|SOFT] path|libname — ONCE skips reload"},
      {"LET", "LET name = expr|string"},
      {"DEFAULT", "DEFAULT name = expr|str — set only if unset (INCLUDE-safe)"},
      {"DEFINED", "DEFINED name — LAST_N 1 if var exists, 0 if missing"},
      {"TYPEOF", "TYPEOF name — LAST undef|num|str · LAST_N 0|1|2"},
      {"UNSET", "UNSET name — remove var · LAST_N 1 if removed (DEFAULT re-apply)"},
      {"SYS", "SYS ENV|ARG|READ|WRITE|CWD|STATE|ROOT|TIME|MS … · ENV/ARG OR fallback"},
      {"SYS ENV", "SYS ENV NAME [OR fallback] · ENV SET name val · ENV UNSET name"},
      {"SYS SETENV", "SYS SETENV|ENV SET name value — process setenv (CUBALC_* without shell)"},
      {"SYS UNSETENV", "SYS UNSETENV|ENV UNSET name — process unsetenv · LAST_N 1 if was set"},
      {"SYS ARG", "SYS ARG n|name [OR fallback] via CUBALC_ARGn"},
      {"SYS CWD", "SYS CWD — process working directory → LAST/CWD"},
      {"SYS CHDIR", "SYS CHDIR|CD path — change process cwd · LAST_N 0|1 soft miss"},
      {"SYS CD", "SYS CD path — alias of SYS CHDIR"},
      {"SYS STATE", "SYS STATE — CUBALC_STATE plate dir → LAST"},
      {"SYS ROOT", "SYS ROOT — CUBALC_ROOT or cwd → LAST"},
      {"SYS TMP", "SYS TMP|TEMP|TMPDIR — portable temp dir → LAST/TMP"},
      {"SYS MKDIR", "SYS MKDIR path — mkdir -p · OK if dir exists"},
      {"SYS JOIN", "SYS JOIN|PATH a b — portable path join a/b → LAST (plate paths)"},
      {"SYS PATH", "SYS PATH a b — alias of SYS JOIN"},
      {"SYS BASENAME", "SYS BASENAME|LEAF path — final component → LAST"},
      {"SYS DIRNAME", "SYS DIRNAME|PARENT path — parent directory → LAST"},
      {"SYS EXTNAME", "SYS EXTNAME|EXT|SUFFIX path — final .ext → LAST/EXT"},
      {"SYS STEM", "SYS STEM|ROOTNAME path — basename without extension → LAST"},
      {"SYS SIZE", "SYS SIZE|FSIZE path — file bytes → LAST_N/SIZE · soft miss"},
      {"SYS ISDIR", "SYS ISDIR path — LAST_N 1 if directory"},
      {"SYS ISFILE", "SYS ISFILE path — LAST_N 1 if regular file"},
      {"SYS READ", "SYS READ [OR|SOFT] path [OR fallback] — soft/optional plate"},
      {"SYS RM", "SYS RM|UNLINK|DELETE path — remove file · missing soft OK"},
      {"SYS RENAME", "SYS RENAME|MV|MOVE from to — move plate path"},
      {"SYS COPY", "SYS COPY|CP src dst — duplicate file · LAST_N=bytes"},
      {"SYS REALPATH", "SYS REALPATH|ABSPATH path — absolute path → LAST"},
      {"SYS TOUCH", "SYS TOUCH path — create empty / refresh mtime · LAST_N 0|1"},
      {"SYS LIST", "SYS LIST|LS path — dir basenames → LAST · LAST_N=count"},
      {"SYS NTH", "SYS NTH n [str] — 0-based newline field · pairs with LIST"},
      {"SYS LINE", "SYS LINE n [str] — 1-based newline field"},
      {"SYS HEAD", "SYS HEAD [str] — first newline field"},
      {"SYS TAIL", "SYS TAIL [str] — last newline field"},
      {"SYS GREP", "SYS GREP|FILTER needle [str] — keep newline fields containing needle"},
      {"SYS GREPV", "SYS GREPV|VGREP needle [str] — drop newline fields containing needle"},
      {"SYS GREPI", "SYS GREPI|IGREP|GREP I needle [str] — case-insensitive GREP"},
      {"SYS GREPVI", "SYS GREPVI|VGREPI needle [str] — case-insensitive invert GREP"},
      {"SYS TAKE", "SYS TAKE|FIRSTN n [str] — first n newline fields · LIST window"},
      {"SYS DROP", "SYS DROP|SKIP n [str] — drop first n newline fields · keep rest"},
      {"SYS SPLIT", "SYS SPLIT|FIELDS sep [str] — sep-split → newline fields · PATH/CSV"},
      {"SYS WORDS", "SYS WORDS|TOKENIZE [str] — whitespace → newline fields · collapse runs"},
      {"SYS TOKENIZE", "SYS TOKENIZE [str] — alias of SYS WORDS"},
      {"SYS SORT", "SYS SORT [str] — lexicographic sort of newline fields · stable LIST"},
      {"SYS UNIQ", "SYS UNIQ [str] — drop adjacent duplicate fields (sort first)"},
      {"SYS REVL", "SYS REVL|REVLINES|TAC [str] — reverse newline field order · LIFO bags"},
      {"SYS REVLINES", "SYS REVLINES [str] — alias of SYS REVL"},
      {"SYS TAC", "SYS TAC [str] — alias of SYS REVL (shell tac)"},
      {"SYS JOINLINES", "SYS JOINLINES|PASTE sep [str] — join newline fields with sep (anti-SPLIT)"},
      {"SYS APPEND", "SYS APPEND|LOG path data — append line to file · history/audit log"},
      {"SYS LOG", "SYS LOG path data — alias of SYS APPEND"},
      {"SYS REPLACE", "SYS REPLACE hay old new — first occurrence · LAST_N=1 if replaced"},
      {"SYS REPLACEALL", "SYS REPLACEALL|GSUB hay old new — all occurrences · LAST_N=count"},
      {"SYS STR", "SYS STR|ITOA|NUMSTR [n|LAST_N] — integer → decimal string LAST · template {{COUNT}}"},
      {"SYS ITOA", "SYS ITOA [n] — alias of SYS STR · dual of SYS NUM/ATOI"},
      {"SYS BEFORE", "SYS BEFORE|LEFT_OF hay needle — text left of first needle · LAST_N=found"},
      {"SYS AFTER", "SYS AFTER|RIGHT_OF hay needle — text right of first needle · LAST_N=found"},
      {"SYS BETWEEN", "SYS BETWEEN|MIDOF|EXTRACT open close [hay] — peel between delimiters · LAST_N=found"},
      {"SYS MIDOF", "SYS MIDOF open close [hay] — alias of SYS BETWEEN"},
      {"SYS EXTRACT", "SYS EXTRACT open close [hay] — alias of SYS BETWEEN"},
      {"SYS PUSH", "SYS PUSH|ADDLINE bag [line] — append newline field · multi-file accumulate"},
      {"SYS ADDLINE", "SYS ADDLINE bag line — alias of SYS PUSH · LAST_N/PUSH_N=count"},
      {"SYS PREPEND", "SYS PREPEND|UNSHIFT bag [line] — insert field at front · FIFO/priority"},
      {"SYS UNSHIFT", "SYS UNSHIFT bag line — alias of SYS PREPEND"},
      {"SYS POP", "SYS POP|POPLINE [bag] — last field → LAST; rest → POP_REST · process bags"},
      {"SYS POPLINE", "SYS POPLINE bag — alias of SYS POP (not stack POP)"},
      {"SYS POPHEAD", "SYS POPHEAD|DEQUEUE [bag] — first field → LAST; rest → POP_REST · FIFO"},
      {"SYS DEQUEUE", "SYS DEQUEUE bag — alias of SYS POPHEAD"},
      {"SYS EQSI", "SYS EQSI|IEQS|EQS I a b — case-insensitive string equality · LAST_N 0|1"},
      {"SYS HASI", "SYS HASI|ICONTAINS|HAS I hay needle — case-insensitive substring · LAST_N 0|1"},
      {"SYS LINES", "SYS LINES|NLINES|WC [str] — count newline fields → LAST_N/LINES_N"},
      {"SYS WC", "SYS WC [str] — alias of SYS LINES · field count without shell"},
      {"SYS HASLINE", "SYS HASLINE|HASFIELD bag needle — exact field membership · LAST_N 0|1"},
      {"SYS HASLINEI", "SYS HASLINEI|ILINEIN bag needle — case-insensitive exact field"},
      {"SYS COUNTLINE", "SYS COUNTLINE|COUNTFIELD bag needle — count exact field matches → LAST_N"},
      {"SYS COUNTLINEI", "SYS COUNTLINEI bag needle — case-insensitive field match count"},
      {"SYS FINDLINE", "SYS FINDLINE|LINEINDEX bag needle — 0-based exact field index · -1 miss"},
      {"SYS LINEINDEX", "SYS LINEINDEX bag needle — alias of SYS FINDLINE"},
      {"SYS FINDLINEI", "SYS FINDLINEI bag needle — case-insensitive field index"},
      {"SYS SETLINE", "SYS SETLINE|REPLACELINE bag n value — set 0-based field · LAST=bag"},
      {"SYS REPLACELINE", "SYS REPLACELINE bag n value — alias of SYS SETLINE"},
      {"SYS SETMATCH", "SYS SETMATCH|REPLACEMATCH bag old new — replace first exact field · LAST_N 0|1"},
      {"SYS REPLACEMATCH", "SYS REPLACEMATCH bag old new — alias of SYS SETMATCH"},
      {"SYS SETMATCHI", "SYS SETMATCHI bag old new — case-insensitive match on old field"},
      {"SYS INSERTLINE", "SYS INSERTLINE|INSLINE bag n value — insert field at 0-based index · append if past end"},
      {"SYS INSLINE", "SYS INSLINE bag n value — alias of SYS INSERTLINE"},
      {"SYS DROPNTH", "SYS DROPNTH|DROPAT bag n — drop 0-based field by index · LAST_N 0|1 soft OOR"},
      {"SYS DROPAT", "SYS DROPAT bag n — alias of SYS DROPNTH"},
      {"SYS MOVELINE", "SYS MOVELINE|MOVEAT bag from to — move field to final index · promote/demote"},
      {"SYS MOVEAT", "SYS MOVEAT bag from to — alias of SYS MOVELINE"},
      {"SYS REMOVELINE", "SYS REMOVELINE|DROPLINE bag needle — drop first exact field · LAST=bag"},
      {"SYS DROPLINE", "SYS DROPLINE bag needle — alias of SYS REMOVELINE"},
      {"SYS REMOVELINEI", "SYS REMOVELINEI bag needle — case-insensitive drop first field"},
      {"SYS CUT", "SYS CUT|FIELDN hay sep n — 0-based field by sep · LAST_N=found"},
      {"SYS COLUMN", "SYS COLUMN|COL hay sep n — 1-based field by sep (CSV/path)"},
      {"SYS EMPTY", "SYS EMPTY|ISEMPTY [str] — LAST_N 1 if zero-length · soft plate IF"},
      {"SYS BLANK", "SYS BLANK|ISBLANK [str] — LAST_N 1 if empty or whitespace only"},
      {"SYS NONEMPTY", "SYS NONEMPTY|NOTEMPTY [str] — LAST_N 1 if any character"},
      {"SYS MTIME", "SYS MTIME|MODTIME path — file mtime epoch → LAST_N · soft miss"},
      {"SYS AGE", "SYS AGE|FILEAGE path — seconds since mtime → LAST_N · plate freshness"},
      {"SYS STARTSI", "SYS STARTSI|ISTARTS|STARTS I hay pref — case-insensitive prefix · LAST_N"},
      {"SYS ENDSI", "SYS ENDSI|IENDS|ENDS I hay suf — case-insensitive suffix · LAST_N"},
      {"SYS FINDI", "SYS FINDI|INDEXI|FIND I hay needle — case-insensitive index → LAST_N (-1 miss)"},
      {"SYS INDEXI", "SYS INDEXI hay needle — alias of SYS FINDI"},
      {"EACH LINE", "EACH LINE [as name] [IN str] … END — walk newline fields (LIST/GREP)"},
      {"EACH", "EACH CUBE|CELL|LINE … END — iterate cubes, cells, or text lines"},
      {"SYS TIME", "SYS TIME|NOW|EPOCH — wall seconds → LAST_N/TIME"},
      {"SYS MS", "SYS MS|MILLIS|TIME_MS — wall milliseconds → LAST_N/MS"},
      {"SYS SLEEP", "SYS SLEEP|MSLEEP|DELAY n — pause n ms (cap 60s)"},
      {"SYS RAND", "SYS RAND|RANDOM [n]|[lo hi] — uniform int · jitter/sample without shell"},
      {"SYS RANDOM", "SYS RANDOM [n]|[lo hi] — alias of SYS RAND"},
      {"SYS DATE", "SYS DATE|ISO|UTC — UTC stamp YYYY-MM-DDTHH:MM:SSZ → LAST/DATE"},
      {"SYS PID", "SYS PID — process id → LAST_N/PID"},
      {"SYS HOSTNAME", "SYS HOSTNAME|HOST — machine name → LAST/HOSTNAME"},
      {"SYS USER", "SYS USER|USERNAME — login name → LAST/USER"},
      {"SYS UID", "SYS UID|USER_ID — numeric user id → LAST_N/UID"},
      {"SYS HOME", "SYS HOME|HOMEDIR — home directory → LAST/HOME"},
      {"SMX", "SMX KEY|TALK|EXCHANGE|SERVE|DIAL — binary mesh, no HTTP"},
      {"SMX KEY", "SMX KEY — load CUBALC_SMX_KEY / demo key"},
      {"SMX EXCHANGE", "SMX EXCHANGE a b — bidirectional TALK"},
      {"SMX SERVE", "SMX SERVE local remote host:port · CUBALC_P2P_TIMEOUT"},
      {"SMX DIAL", "SMX DIAL local remote host:port · CUBALC_P2P_SOFT soft-fail"},
      {"HARMONY", "HARMONY [target] — hive consensus + unity"},
      {"RESOLVE", "RESOLVE [target] — harmony + decide + energy"},
      {"COMPARE", "COMPARE a b — Hamming / unity"},
      {"NEST", "NEST parent child — cube nesting"},
    };
    int nhelp = (int)(sizeof help / sizeof help[0]);
    char q[64]; q[0]=0;
    lex_next(L);
    /* Topic may be a keyword (ASSERT, SMX, PLUG…) — always consume as help query. */
    if (L->cur.kind==TK_STR || L->cur.kind==TK_IDENT){
      snprintf(q, sizeof q, "%s", L->cur.text);
      lex_next(L);
    }
    /* uppercase query for match */
    {
      size_t i;
      for (i = 0; q[i]; i++)
        if (q[i] >= 'a' && q[i] <= 'z') q[i] = (char)(q[i] - 'a' + 'A');
    }
    char out[CUBALC_HOST_STR_MAX]; out[0]=0;
    int found = 0, hits = 0;
    if (!q[0]){
      snprintf(out, sizeof out,
               "HELP [form] · cubalc forms [prefix] · cookbook: docs/COOKBOOK.md · "
               "HOLD_FLASH 1 before PLUG · SMX binary mesh");
      found = 1; hits = nhelp;
    } else {
      size_t o = 0;
      int i;
      for (i = 0; i < nhelp; i++){
        char nm[48]; size_t k;
        for (k = 0; help[i].name[k] && k + 1 < sizeof nm; k++)
          nm[k] = (char)((help[i].name[k] >= 'a' && help[i].name[k] <= 'z')
                         ? help[i].name[k] - 'a' + 'A' : help[i].name[k]);
        nm[k] = 0;
        if (strstr(nm, q) || strstr(help[i].hint, q)){
          int n = snprintf(out + o, sizeof out - o, "%s%s: %s",
                           o ? " | " : "", help[i].name, help[i].hint);
          if (n > 0) o += (size_t)n;
          hits++; found = 1;
          if (o + 32 >= sizeof out) break;
        }
      }
      if (!found)
        snprintf(out, sizeof out, "no form match for '%s' — try cubalc forms %s", q, q);
    }
    snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
    vm->last_n = (long)strlen(out);
    var_set_str(vm, "LAST", out);
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "OK", found ? 1 : 0);
    var_set_num(vm, "HELP_N", (long)hits);
    if (vm->trace) fprintf(vm->trace, "%s\n", out);
    if (vm->res) snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", out);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"PRINT")){
    lex_next(L);
    char line[256]; size_t o=0; line[0]=0;
    if (L->cur.kind==TK_STR){ snprintf(line,sizeof line,"%s",L->cur.text); o=strlen(line); lex_next(L); }
    while (L->cur.kind!=TK_NL && L->cur.kind!=TK_EOF && L->cur.kind!=TK_LBRACK){
      if (L->cur.kind==TK_IDENT && (kw(&L->cur,"ASSERT")||kw(&L->cur,"LET")||kw(&L->cur,"CUBE")||
            kw(&L->cur,"PRINT")||kw(&L->cur,"PRINT_JSON")||kw(&L->cur,"DUMP")||kw(&L->cur,"PRINTJSON"))) break;
      long v=parse_expr(vm,L);
      int n=snprintf(line+o,sizeof line-o,"%s%ld",o?" ":"",v); if(n>0)o+=(size_t)n;
      if (o>=sizeof line) break;
    }
    if (vm->trace) fprintf(vm->trace,"%s\n",line);
    if (vm->res) snprintf(vm->res->last_print,sizeof vm->res->last_print,"%s",line);
    bump(vm); return 1;
  }
  /* PRINT_JSON / DUMP [ident…] — one stable JSON line for agents
   * bare: runtime snapshot; with idents: {"name":value,…} (str vars quoted) */
  if (kw(&L->cur,"PRINT_JSON")||kw(&L->cur,"DUMP")||kw(&L->cur,"PRINTJSON")){
    lex_next(L);
    char line[512]; size_t o=0; line[0]=0;
    int first=1;
    o += (size_t)snprintf(line+o, sizeof line-o, "{");
    if (L->cur.kind==TK_NL || L->cur.kind==TK_EOF || L->cur.kind==TK_LBRACK){
      /* bare snapshot of host-facing scalars */
      long okv=0, dec=0, smx=0, talks=0;
      Var *vo=var_get(vm,"OK",0); if (vo && !vo->is_str) okv=vo->val;
      Var *vd=var_get(vm,"DECIDE",0); if (vd && !vd->is_str) dec=vd->val;
      Var *vs=var_get(vm,"SMX_OK",0); smx = vs ? vs->val : (long)vm->smx_ok;
      Var *vt=var_get(vm,"SMX_TALKS",0); talks = vt ? vt->val : (long)vm->smx_talks;
      o += (size_t)snprintf(line+o, sizeof line-o,
        "\"schema\":\"cubalc.print_json.v1\",\"CUBES\":%d,\"LAST_N\":%ld,"
        "\"OK\":%ld,\"SP\":%d,\"UNITY\":%ld,\"DECIDE\":%ld,\"SMX_OK\":%ld,"
        "\"SMX_TALKS\":%ld,\"hold_flash\":%d",
        vm->ch.n_cubes, vm->last_n, okv, vm->sp,
        (long)lround(vm->ch.unity * 100.0), dec, smx, talks, vm->hold_flash);
      first=0;
    } else {
      while (L->cur.kind!=TK_NL && L->cur.kind!=TK_EOF && L->cur.kind!=TK_LBRACK){
        if (L->cur.kind!=TK_IDENT) break;
        if (kw(&L->cur,"ASSERT")||kw(&L->cur,"LET")||kw(&L->cur,"CUBE")||
            kw(&L->cur,"PRINT")||kw(&L->cur,"PRINT_JSON")||kw(&L->cur,"DUMP")||
            kw(&L->cur,"PRINTJSON")||kw(&L->cur,"SYS")||kw(&L->cur,"IF")||
            kw(&L->cur,"LOOP")||kw(&L->cur,"WHILE")||kw(&L->cur,"END")||
            kw(&L->cur,"RET")||kw(&L->cur,"ELSE")) break;
        char name[48];
        snprintf(name, sizeof name, "%s", L->cur.text);
        lex_next(L);
        if (o + 64 >= sizeof line) break;
        if (!first) o += (size_t)snprintf(line+o, sizeof line-o, ",");
        first=0;
        /* string var or LAST */
        Var *vv = var_get(vm, name, 0);
        if ((vv && vv->is_str) || strcmp(name,"LAST")==0){
          const char *sv = (vv && vv->is_str) ? vv->sval : vm->last_str;
          char esc[240]; size_t eo=0;
          for (const char *p=sv?sv:""; *p && eo+2<sizeof esc; p++){
            if (*p=='"' || *p=='\\'){ esc[eo++]='\\'; esc[eo++]=*p; }
            else if ((unsigned char)*p < 0x20) continue;
            else esc[eo++]=*p;
          }
          esc[eo]=0;
          o += (size_t)snprintf(line+o, sizeof line-o, "\"%s\":\"%s\"", name, esc);
        } else {
          long val=0;
          if (vv) val=vv->val;
          else if (strcmp(name,"CUBES")==0) val=(long)vm->ch.n_cubes;
          else if (strcmp(name,"LAST_N")==0) val=vm->last_n;
          else if (strcmp(name,"SP")==0 || strcmp(name,"STACKLEN")==0) val=(long)vm->sp;
          else if (strcmp(name,"UNITY")==0) val=(long)lround(vm->ch.unity*100.0);
          else if (strcmp(name,"SMX_OK")==0) val=(long)vm->smx_ok;
          else if (strcmp(name,"SMX_TALKS")==0) val=(long)vm->smx_talks;
          else if (strcmp(name,"HTTP_CODE")==0) val=(long)vm->last_code;
          else if (strcmp(name,"CELLS")==0) val=(long)CUBALC_CELL_N;
          else if (strcmp(name,"HOLD_FLASH")==0) val=(long)vm->hold_flash;
          o += (size_t)snprintf(line+o, sizeof line-o, "\"%s\":%ld", name, val);
        }
      }
    }
    if (o + 2 < sizeof line) snprintf(line+o, sizeof line-o, "}");
    else line[sizeof line - 2] = '}';
    if (vm->trace) fprintf(vm->trace,"%s\n",line);
    if (vm->res) snprintf(vm->res->last_print,sizeof vm->res->last_print,"%s",line);
    bump(vm); return 1;
  }
  /* VARS / LOCALS — dump all program variables as one JSON line for agents.
   * Complements PRINT_JSON (named/snapshot): full LET/SYS table without guessing names. */
  if (kw(&L->cur,"VARS")||kw(&L->cur,"LOCALS")||kw(&L->cur,"SHOW_VARS")||
      kw(&L->cur,"VARS_JSON")||kw(&L->cur,"DUMP_VARS")){
    lex_next(L);
    char line[CUBALC_HOST_STR_MAX];
    size_t o = 0;
    int i, first = 1, n = 0;
    o += (size_t)snprintf(line + o, sizeof line - o,
      "{\"schema\":\"cubalc.vars.v1\",\"n\":0,\"vars\":{");
    /* rewrite n after count — fill vars first into temp then wrap is heavy;
     * emit and patch is awkward; emit n at end via second pass count. */
    o = 0;
    o += (size_t)snprintf(line + o, sizeof line - o,
      "{\"schema\":\"cubalc.vars.v1\",\"ok\":true,\"vars\":{");
    for (i = 0; i < vm->n_vars; i++) {
      Var *v = &vm->vars[i];
      char esc[280];
      size_t eo = 0;
      int add;
      if (!v->name[0]) continue;
      if (o + 80 >= sizeof line) break;
      if (!first) o += (size_t)snprintf(line + o, sizeof line - o, ",");
      first = 0;
      n++;
      if (v->is_str) {
        const char *p;
        for (p = v->sval; *p && eo + 2 < sizeof esc; p++) {
          if (*p == '"' || *p == '\\') { esc[eo++] = '\\'; esc[eo++] = *p; }
          else if ((unsigned char)*p < 0x20) continue;
          else esc[eo++] = *p;
        }
        esc[eo] = 0;
        add = snprintf(line + o, sizeof line - o, "\"%s\":\"%s\"", v->name, esc);
      } else {
        add = snprintf(line + o, sizeof line - o, "\"%s\":%ld", v->name, v->val);
      }
      if (add > 0) o += (size_t)add;
    }
    /* also surface LAST if not already a var */
    if (o + 64 < sizeof line) {
      int has_last = 0;
      for (i = 0; i < vm->n_vars; i++)
        if (strcmp(vm->vars[i].name, "LAST") == 0) { has_last = 1; break; }
      if (!has_last && vm->last_str[0]) {
        char esc[280];
        size_t eo = 0;
        const char *p;
        for (p = vm->last_str; *p && eo + 2 < sizeof esc; p++) {
          if (*p == '"' || *p == '\\') { esc[eo++] = '\\'; esc[eo++] = *p; }
          else if ((unsigned char)*p < 0x20) continue;
          else esc[eo++] = *p;
        }
        esc[eo] = 0;
        o += (size_t)snprintf(line + o, sizeof line - o, "%s\"LAST\":\"%s\"",
                              first ? "" : ",", esc);
        first = 0;
        n++;
      }
    }
    o += (size_t)snprintf(line + o, sizeof line - o,
                          "},\"n\":%d,\"version\":\"%s\"}", n, CUBALC_LANG_VERSION);
    if (vm->trace) fprintf(vm->trace, "%s\n", line);
    if (vm->res) snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", line);
    var_set_num(vm, "VARS_N", (long)n);
    var_set_num(vm, "LAST_N", (long)n);
    var_set_num(vm, "OK", 1);
    vm->last_n = (long)n;
    /* keep LAST as count note for agents that only read LAST string */
    {
      char note[48];
      snprintf(note, sizeof note, "vars:%d", n);
      var_set_str(vm, "LAST", note);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", note);
    }
    bump(vm); return 1;
  }
  /* STATUS / HEALTH / AGENT_STATUS — one compact runtime health plate for agents.
   * Complements VARS (full table) and PRINT_JSON (named/snapshot): surfaces OK,
   * sticky LAST_ERR, version, wall time, hold, cubes — without guessing names.
   * Does not rewrite OK (reports current soft status). */
  if (kw(&L->cur,"STATUS")||kw(&L->cur,"HEALTH")||kw(&L->cur,"AGENT_STATUS")||
      kw(&L->cur,"STATUS_JSON")||kw(&L->cur,"PROBE_STATUS")){
    long okv = 1, exp_ok = 1, smx = 0, talks = 0;
    long tsec;
    const char *err = "";
    char esc[240];
    size_t eo = 0;
    char line[CUBALC_HOST_STR_MAX];
    char note[64];
    Var *vo, *ve, *vs, *vt, *vle;
    lex_next(L);
    vo = var_get(vm, "OK", 0);
    if (vo && !vo->is_str) okv = vo->val;
    ve = var_get(vm, "EXPECT_OK", 0);
    if (ve && !ve->is_str) exp_ok = ve->val;
    vs = var_get(vm, "SMX_OK", 0);
    smx = vs ? vs->val : (long)vm->smx_ok;
    vt = var_get(vm, "SMX_TALKS", 0);
    talks = vt ? vt->val : (long)vm->smx_talks;
    vle = var_get(vm, "LAST_ERR", 0);
    if (vle && vle->is_str && vle->sval[0]) err = vle->sval;
    else if (vm->err[0]) err = vm->err;
    for (const char *p = err; *p && eo + 2 < sizeof esc; p++) {
      if (*p == '"' || *p == '\\') { esc[eo++] = '\\'; esc[eo++] = *p; }
      else if ((unsigned char)*p < 0x20) continue;
      else esc[eo++] = *p;
    }
    esc[eo] = 0;
    tsec = (long)time(NULL);
    snprintf(line, sizeof line,
      "{\"schema\":\"cubalc.status.v1\",\"ok\":%s,\"last_err\":\"%s\","
      "\"version\":\"%s\",\"time\":%ld,\"n\":%d,\"hold\":%d,\"unity\":%ld,"
      "\"last_n\":%ld,\"expect_ok\":%ld,\"smx_ok\":%ld,\"smx_talks\":%ld,"
      "\"sp\":%d,\"stmts\":%d}",
      okv ? "true" : "false", esc, CUBALC_LANG_VERSION, tsec,
      vm->ch.n_cubes, vm->hold_flash,
      (long)lround(vm->ch.unity * 100.0), vm->last_n, exp_ok, smx, talks,
      vm->sp, vm->res ? vm->res->stmts : 0);
    if (vm->trace) fprintf(vm->trace, "%s\n", line);
    if (vm->res) snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", line);
    var_set_num(vm, "STATUS_OK", okv);
    var_set_num(vm, "TIME", tsec);
    /* short LAST summary — full plate is last_print / trace JSON */
    if (okv)
      snprintf(note, sizeof note, "status:ok");
    else if (esc[0])
      snprintf(note, sizeof note, "status:err");
    else
      snprintf(note, sizeof note, "status:soft");
    var_set_str(vm, "LAST", note);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", note);
    vm->last_n = okv;
    var_set_num(vm, "LAST_N", okv);
    /* report-only: leave OK/EXPECT_OK/LAST_ERR as they were */
    if (vm->trace) fprintf(vm->trace, "# status ok=%ld err=%s\n", okv, esc[0] ? esc : "-");
    bump(vm); return 1;
  }
  /* IDENTITY / WHOAMI_PLATE — one host/process identity plate for agents.
   * Fills PID HOSTNAME USER UID HOME CWD VERSION without chaining SYS forms.
   * Full JSON → last_print; short LAST "user@host:pid". */
  if (kw(&L->cur,"IDENTITY")||kw(&L->cur,"WHO")||kw(&L->cur,"HOST_IDENTITY")||
      kw(&L->cur,"ID_PLATE")||kw(&L->cur,"PEER_ID")||kw(&L->cur,"WHO_AM_I")){
    long pid = 0, uid = 0;
    char host[256], user[128], home[512], cwd[512];
    char eh[280], eu[160], ehome[560], ecwd[560];
    char line[CUBALC_HOST_STR_MAX];
    char note[200];
    const char *e;
    size_t eo;
    const char *p;
    lex_next(L);
    host[0] = user[0] = home[0] = cwd[0] = 0;
#if defined(CUBALC_OS_WINDOWS)
    pid = (long)_getpid();
    {
      const char *ce = getenv("COMPUTERNAME");
      if (ce && ce[0]) snprintf(host, sizeof host, "%s", ce);
      else snprintf(host, sizeof host, "localhost");
    }
    e = getenv("UID");
    if (e && e[0]) uid = strtol(e, NULL, 10);
#else
    pid = (long)getpid();
    if (pid < 0) pid = 0;
    uid = (long)getuid();
    if (uid < 0) uid = 0;
    if (gethostname(host, sizeof host) != 0 || !host[0])
      snprintf(host, sizeof host, "localhost");
    host[sizeof host - 1] = 0;
#endif
    e = getenv("USER");
    if (!e || !e[0]) e = getenv("LOGNAME");
    if (!e || !e[0]) e = getenv("USERNAME");
    if (e && e[0]) snprintf(user, sizeof user, "%s", e);
#if !defined(CUBALC_OS_WINDOWS)
    if (!user[0]) {
      struct passwd *pw = getpwuid(getuid());
      if (pw && pw->pw_name && pw->pw_name[0])
        snprintf(user, sizeof user, "%s", pw->pw_name);
    }
#endif
    if (!user[0]) snprintf(user, sizeof user, "user");
    e = getenv("HOME");
#if defined(CUBALC_OS_WINDOWS)
    if (!e || !e[0]) e = getenv("USERPROFILE");
#endif
    if (e && e[0]) snprintf(home, sizeof home, "%s", e);
#if !defined(CUBALC_OS_WINDOWS)
    if (!home[0]) {
      struct passwd *pw = getpwuid(getuid());
      if (pw && pw->pw_dir && pw->pw_dir[0])
        snprintf(home, sizeof home, "%s", pw->pw_dir);
    }
#endif
    if (!home[0]) {
      if (getcwd(home, sizeof home) == NULL)
        snprintf(home, sizeof home, ".");
    }
    if (getcwd(cwd, sizeof cwd) == NULL)
      snprintf(cwd, sizeof cwd, ".");
    /* JSON-escape helper fields */
#define CUBALC_ID_ESC(dst, src) do { \
      eo = 0; \
      for (p = (src); *p && eo + 2 < sizeof(dst); p++) { \
        if (*p == '"' || *p == '\\') { (dst)[eo++] = '\\'; (dst)[eo++] = *p; } \
        else if ((unsigned char)*p < 0x20) continue; \
        else (dst)[eo++] = *p; \
      } \
      (dst)[eo] = 0; \
    } while (0)
    CUBALC_ID_ESC(eh, host);
    CUBALC_ID_ESC(eu, user);
    CUBALC_ID_ESC(ehome, home);
    CUBALC_ID_ESC(ecwd, cwd);
#undef CUBALC_ID_ESC
    var_set_num(vm, "PID", pid);
    var_set_num(vm, "UID", uid);
    var_set_str(vm, "HOSTNAME", host);
    var_set_str(vm, "USER", user);
    var_set_str(vm, "USERNAME", user);
    var_set_str(vm, "HOME", home);
    var_set_str(vm, "CWD", cwd);
    var_set_str(vm, "VERSION", CUBALC_LANG_VERSION);
    snprintf(line, sizeof line,
      "{\"schema\":\"cubalc.identity.v1\",\"ok\":true,"
      "\"pid\":%ld,\"uid\":%ld,\"user\":\"%s\",\"hostname\":\"%s\","
      "\"home\":\"%s\",\"cwd\":\"%s\",\"version\":\"%s\"}",
      pid, uid, eu, eh, ehome, ecwd, CUBALC_LANG_VERSION);
    if (vm->trace) fprintf(vm->trace, "%s\n", line);
    if (vm->res) snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", line);
    snprintf(note, sizeof note, "%s@%s:%ld", user, host, pid);
    var_set_str(vm, "IDENTITY", note);
    var_set_str(vm, "LAST", note);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", note);
    vm->last_n = pid;
    var_set_num(vm, "LAST_N", pid);
    var_set_num(vm, "OK", 1);
    if (vm->trace) fprintf(vm->trace, "# identity %s\n", note);
    bump(vm); return 1;
  }
  /* ASSERT expr ["why"] — optional message for agent/human-readable failures */
  if (kw(&L->cur,"ASSERT")){
    int aln = L->cur.line;
    lex_next(L);
    long v=parse_expr(vm,L);
    char why[120]; why[0]=0;
    if (L->cur.kind==TK_STR){
      snprintf(why, sizeof why, "%s", L->cur.text);
      lex_next(L);
    }
    if (v){
      if (vm->res) vm->res->asserts_ok++;
      if (vm->trace) fprintf(vm->trace,"# ok\n");
    } else {
      if (vm->res) vm->res->asserts_fail++;
      char msg[160];
      if (why[0])
        snprintf(msg, sizeof msg, "ASSERT failed line %d: %s", aln, why);
      else
        snprintf(msg, sizeof msg, "ASSERT failed line %d", aln);
      fail(vm, msg);
      return -1;
    }
    bump(vm); return 1;
  }
  /* EXPECT|CHECK|SOFTASSERT expr ["why"] — soft check: no fatal, program continues.
   * Sets OK/EXPECT_OK 0|1 and sticky LAST_ERR on fail. Does not fail the run plate
   * (use ASSERT for fail-closed). Bumps asserts_ok only on success. */
  if (kw(&L->cur,"EXPECT")||kw(&L->cur,"CHECK")||kw(&L->cur,"SOFTASSERT")||
      kw(&L->cur,"ASSERT_SOFT")||kw(&L->cur,"SOFT_ASSERT")){
    int aln = L->cur.line;
    lex_next(L);
    long v = parse_expr(vm, L);
    char why[120]; why[0] = 0;
    if (L->cur.kind == TK_STR){
      snprintf(why, sizeof why, "%s", L->cur.text);
      lex_next(L);
    }
    if (v){
      if (vm->res) vm->res->asserts_ok++;
      var_set_num(vm, "OK", 1);
      var_set_num(vm, "EXPECT_OK", 1);
      if (vm->trace) fprintf(vm->trace, "# expect ok\n");
    } else {
      char msg[160];
      if (why[0])
        snprintf(msg, sizeof msg, "EXPECT failed line %d: %s", aln, why);
      else
        snprintf(msg, sizeof msg, "EXPECT failed line %d", aln);
      /* sticky agent-readable — not fatal, not asserts_fail */
      var_set_str(vm, "ERR", msg);
      var_set_str(vm, "LAST_ERR", msg);
      var_set_str(vm, "LAST", msg);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", msg);
      vm->last_n = (long)strlen(msg);
      var_set_num(vm, "LAST_N", vm->last_n);
      var_set_num(vm, "OK", 0);
      var_set_num(vm, "EXPECT_OK", 0);
      if (vm->trace) fprintf(vm->trace, "# expect fail: %s\n", msg);
    }
    bump(vm); return 1;
  }
  /* FAIL ["why"] — intentional soft fail (no expr, no fatal). Sticky LAST_ERR/OK=0.
   * PASS ["why"] — intentional soft pass; OK=1 and optional LAST note. */
  if (kw(&L->cur,"FAIL")||kw(&L->cur,"SOFTFAIL")||kw(&L->cur,"MARK_FAIL")){
    int aln = L->cur.line;
    lex_next(L);
    char why[120]; why[0] = 0;
    if (L->cur.kind == TK_STR){
      snprintf(why, sizeof why, "%s", L->cur.text);
      lex_next(L);
    }
    char msg[160];
    if (why[0])
      snprintf(msg, sizeof msg, "FAIL line %d: %s", aln, why);
    else
      snprintf(msg, sizeof msg, "FAIL line %d", aln);
    var_set_str(vm, "ERR", msg);
    var_set_str(vm, "LAST_ERR", msg);
    var_set_str(vm, "LAST", msg);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", msg);
    vm->last_n = (long)strlen(msg);
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "OK", 0);
    var_set_num(vm, "EXPECT_OK", 0);
    if (vm->trace) fprintf(vm->trace, "# fail: %s\n", msg);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"PASS")||kw(&L->cur,"MARK_PASS")||kw(&L->cur,"OKAY")){
    int aln = L->cur.line;
    lex_next(L);
    char why[120]; why[0] = 0;
    if (L->cur.kind == TK_STR){
      snprintf(why, sizeof why, "%s", L->cur.text);
      lex_next(L);
    }
    char msg[160];
    if (why[0])
      snprintf(msg, sizeof msg, "%s", why);
    else
      snprintf(msg, sizeof msg, "PASS line %d", aln);
    var_set_str(vm, "LAST", msg);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", msg);
    vm->last_n = (long)strlen(msg);
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "OK", 1);
    var_set_num(vm, "EXPECT_OK", 1);
    if (vm->trace) fprintf(vm->trace, "# pass: %s\n", msg);
    bump(vm); return 1;
  }
  /* EXIT [code] ["why"] — intentional early program stop for agents/CI.
   * EXIT / EXIT 0 → clean halt (ok if no asserts_fail). EXIT n → fail plate + rc.
   * Stops further statements (including outer loops). */
  if (kw(&L->cur,"EXIT")||kw(&L->cur,"HALT")||kw(&L->cur,"QUIT")||
      kw(&L->cur,"STOP_PROG")||kw(&L->cur,"DIE")){
    int aln = L->cur.line;
    long code = 0;
    char why[120];
    lex_next(L);
    why[0] = 0;
    if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
        (L->cur.kind == TK_IDENT && (strcmp(L->cur.text,"OK")==0 ||
         strcmp(L->cur.text,"LAST_N")==0 || strcmp(L->cur.text,"EXIT")==0))){
      code = parse_expr(vm, L);
    }
    if (L->cur.kind == TK_STR){
      snprintf(why, sizeof why, "%s", L->cur.text);
      lex_next(L);
    }
    if (code < 0) code = 1;
    if (code > 125) code = 1;
    vm->halt = 1;
    vm->exit_code = (int)code;
    vm->break_loop = 1;
    vm->return_fn = 1;
    var_set_num(vm, "EXIT", code);
    var_set_num(vm, "LAST_N", code);
    vm->last_n = code;
    if (code != 0) {
      char msg[160];
      if (why[0])
        snprintf(msg, sizeof msg, "EXIT %ld line %d: %s", code, aln, why);
      else
        snprintf(msg, sizeof msg, "EXIT %ld line %d", code, aln);
      var_set_str(vm, "ERR", msg);
      var_set_str(vm, "LAST_ERR", msg);
      var_set_str(vm, "LAST", msg);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", msg);
      var_set_num(vm, "OK", 0);
      var_set_num(vm, "EXPECT_OK", 0);
      if (vm->trace) fprintf(vm->trace, "# exit %ld: %s\n", code, why[0]?why:"");
    } else {
      if (why[0]) {
        var_set_str(vm, "LAST", why);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", why);
      } else {
        var_set_str(vm, "LAST", "exit:0");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "exit:0");
      }
      var_set_num(vm, "OK", 1);
      if (vm->trace) fprintf(vm->trace, "# exit 0\n");
    }
    bump(vm); return 1;
  }
  /* NOTE ["text"] — agent breadcrumb / step log. Sets LAST + NOTE, does not
   * change OK, EXPECT_OK, or sticky LAST_ERR (unlike PASS/FAIL). Trace: # note: */
  if (kw(&L->cur,"NOTE")||kw(&L->cur,"REMARK")||kw(&L->cur,"LOG_NOTE")||
      kw(&L->cur,"BREADCRUMB")||kw(&L->cur,"STEP_NOTE")){
    int aln = L->cur.line;
    char msg[160];
    lex_next(L);
    msg[0] = 0;
    if (L->cur.kind == TK_STR){
      snprintf(msg, sizeof msg, "%s", L->cur.text);
      lex_next(L);
    } else if (L->cur.kind == TK_IDENT && !kw(&L->cur,"ASSERT") &&
               !kw(&L->cur,"LET") && !kw(&L->cur,"PRINT") && !kw(&L->cur,"SYS") &&
               !kw(&L->cur,"IF") && !kw(&L->cur,"END") && !kw(&L->cur,"STATUS")){
      /* NOTE LAST or NOTE varname */
      if (strcmp(L->cur.text, "LAST") == 0)
        snprintf(msg, sizeof msg, "%s", vm->last_str);
      else {
        Var *v = var_get(vm, L->cur.text, 0);
        if (v && v->is_str) snprintf(msg, sizeof msg, "%s", v->sval);
        else if (v) snprintf(msg, sizeof msg, "%ld", v->val);
        else snprintf(msg, sizeof msg, "%s", L->cur.text);
      }
      lex_next(L);
    }
    if (!msg[0])
      snprintf(msg, sizeof msg, "note line %d", aln);
    var_set_str(vm, "NOTE", msg);
    var_set_str(vm, "LAST", msg);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", msg);
    vm->last_n = (long)strlen(msg);
    var_set_num(vm, "LAST_N", vm->last_n);
    /* preserve OK — do not rewrite soft status */
    if (vm->trace) fprintf(vm->trace, "# note: %s\n", msg);
    if (vm->res) snprintf(vm->res->last_print, sizeof vm->res->last_print, "%s", msg);
    bump(vm); return 1;
  }
  /* CLEAR_ERR [note] — wipe sticky ERR/LAST_ERR after soft recovery.
   * PASS restores OK but leaves LAST_ERR (agents read plate last_err); this
   * intentionally clears so recovered paths show a clean plate. Does not set OK. */
  if (kw(&L->cur,"CLEAR_ERR")||kw(&L->cur,"CLEARERR")||kw(&L->cur,"ERR_CLEAR")||
      kw(&L->cur,"WIPE_ERR")||kw(&L->cur,"RESET_ERR")||kw(&L->cur,"CLRERR")){
    char note[120];
    lex_next(L);
    note[0] = 0;
    if (L->cur.kind == TK_STR){
      snprintf(note, sizeof note, "%s", L->cur.text);
      lex_next(L);
    }
    var_set_str(vm, "ERR", "");
    var_set_str(vm, "LAST_ERR", "");
    vm->err[0] = 0;
    if (note[0]) {
      var_set_str(vm, "LAST", note);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", note);
      vm->last_n = (long)strlen(note);
    } else {
      var_set_str(vm, "LAST", "cleared");
      snprintf(vm->last_str, sizeof vm->last_str, "%s", "cleared");
      vm->last_n = 7;
    }
    var_set_num(vm, "LAST_N", vm->last_n);
    if (vm->trace) fprintf(vm->trace, "# clear_err %s\n", note[0] ? note : "cleared");
    bump(vm); return 1;
  }
  /* VERSION — agent/human plate: language version string → LAST / VERSION / OK */
  if (kw(&L->cur,"VERSION")||kw(&L->cur,"LANG_VERSION")||kw(&L->cur,"CUBALC_VERSION")){
    lex_next(L);
    const char *ver = CUBALC_LANG_VERSION;
    var_set_str(vm, "VERSION", ver);
    var_set_str(vm, "LAST", ver);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", ver);
    vm->last_n = (long)strlen(ver);
    var_set_num(vm, "LAST_N", vm->last_n);
    var_set_num(vm, "OK", 1);
    if (vm->trace) fprintf(vm->trace, "# version %s\n", ver);
    bump(vm); return 1;
  }
  /* REQUIRE VERSION "x.y[.z]" — fail-fast if runtime older than need.
   * REQUIRE LIB|MODULE name — fail-fast if INCLUDE-style path not found.
   * Usability: agents refuse missing stdlib / old runtime without shell glue. */
  if (kw(&L->cur,"REQUIRE")||kw(&L->cur,"NEED")||kw(&L->cur,"REQUIRES")){
    int aln = L->cur.line;
    lex_next(L);
    /* REQUIRE LIB|MODULE|INCLUDE|FILE name — resolve like INCLUDE / cubalc which */
    if (kw(&L->cur,"LIB")||kw(&L->cur,"MODULE")||kw(&L->cur,"INCLUDE")||
        kw(&L->cur,"FILE")||kw(&L->cur,"STDLIB")||kw(&L->cur,"SRC")){
      char name[160];
      cubalc_host_result hr;
      lex_next(L);
      if (L->cur.kind != TK_STR && L->cur.kind != TK_IDENT){
        fail(vm, "REQUIRE LIB name|path");
        return -1;
      }
      snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
      if (cubalc_host_find_cubalc(name, &hr) != 0){
        char msg[160];
        snprintf(msg, sizeof msg,
                 "REQUIRE LIB '%s' missing line %d — tried programs/lib · cubalc libs",
                 name, aln);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
      var_set_str(vm, "LAST", hr.str);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", hr.str);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "OK", 1);
      var_set_str(vm, "REQUIRE_LIB", hr.str);
      if (vm->trace)
        fprintf(vm->trace, "# require lib %s → %s\n", name, hr.str);
      if (vm->res) vm->res->asserts_ok++;
      bump(vm); return 1;
    }
    if (!kw(&L->cur,"VERSION") && !kw(&L->cur,"VER") && !kw(&L->cur,"LANG") &&
        !kw(&L->cur,"CUBALC") && L->cur.kind != TK_STR){
      fail(vm, "REQUIRE VERSION \"x.y[.z]\" | REQUIRE LIB name");
      return -1;
    }
    if (kw(&L->cur,"VERSION")||kw(&L->cur,"VER")||kw(&L->cur,"LANG")||
        kw(&L->cur,"CUBALC"))
      lex_next(L);
    if (L->cur.kind != TK_STR && L->cur.kind != TK_IDENT && L->cur.kind != TK_NUM){
      fail(vm, "REQUIRE VERSION \"x.y[.z]\"");
      return -1;
    }
    {
      char need[64];
      const char *have = CUBALC_LANG_VERSION;
      long hn[3] = {0, 0, 0}, nn[3] = {0, 0, 0};
      int hi = 0, ni = 0;
      char *p;
      if (L->cur.kind == TK_NUM)
        snprintf(need, sizeof need, "%ld", L->cur.num);
      else
        snprintf(need, sizeof need, "%s", L->cur.text);
      lex_next(L);
      /* parse have */
      p = (char *)have;
      while (*p && hi < 3) {
        while (*p && (*p < '0' || *p > '9')) p++;
        if (!*p) break;
        hn[hi++] = strtol(p, &p, 10);
        if (*p == '.') p++;
        else break;
      }
      /* parse need */
      p = need;
      while (*p && ni < 3) {
        while (*p && (*p < '0' || *p > '9')) p++;
        if (!*p) break;
        nn[ni++] = strtol(p, &p, 10);
        if (*p == '.') p++;
        else break;
      }
      if (ni == 0) {
        char msg[160];
        snprintf(msg, sizeof msg,
                 "REQUIRE VERSION bad need '%s' line %d", need, aln);
        fail(vm, msg);
        return -1;
      }
      {
        int okv = 1, c;
        for (c = 0; c < 3; c++) {
          if (hn[c] > nn[c]) break;
          if (hn[c] < nn[c]) { okv = 0; break; }
        }
        var_set_str(vm, "VERSION", have);
        var_set_str(vm, "LAST", have);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", have);
        if (!okv) {
          char msg[160];
          snprintf(msg, sizeof msg,
                   "REQUIRE VERSION %s failed line %d: have %s",
                   need, aln, have);
          if (vm->res) vm->res->asserts_fail++;
          fail(vm, msg);
          return -1;
        }
        var_set_num(vm, "OK", 1);
        var_set_num(vm, "LAST_N", 1);
        vm->last_n = 1;
        if (vm->trace)
          fprintf(vm->trace, "# require version %s ok (have %s)\n", need, have);
        if (vm->res) vm->res->asserts_ok++;
        bump(vm); return 1;
      }
    }
  }
  /* DEFINED name — does a program var exist? (pair with DEFAULT)
   * Usability: agents branch without guessing; EXPECT DEFINED before use.
   * LAST_N/DEFINED = 1 if present, 0 if missing; OK always 1. */
  if (kw(&L->cur,"DEFINED")||kw(&L->cur,"ISDEF")||kw(&L->cur,"ISDEFINED")||
      kw(&L->cur,"ISSET")||kw(&L->cur,"HAS_VAR")||kw(&L->cur,"VAR_EXISTS")){
    char name[48];
    long n;
    char buf[8];
    lex_next(L);
    if (L->cur.kind!=TK_IDENT && L->cur.kind!=TK_STR){
      fail(vm,"DEFINED name"); return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    n = (var_get(vm, name, 0) != NULL) ? 1L : 0L;
    var_set_num(vm, "DEFINED", n);
    var_set_num(vm, "LAST_N", n);
    var_set_num(vm, "OK", 1);
    vm->last_n = n;
    snprintf(buf, sizeof buf, "%ld", n);
    var_set_str(vm, "LAST", buf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
    if (vm->trace)
      fprintf(vm->trace, "# defined %s → %ld\n", name, n);
    bump(vm); return 1;
  }
  /* UNSET name — remove a program var so DEFAULT can re-apply.
   * Usability: agents clear knobs after use / reset INCLUDE defaults.
   * LAST_N/UNSET_N = 1 if removed, 0 if was missing; OK=1. */
  if (kw(&L->cur,"UNSET")||kw(&L->cur,"UNLET")||kw(&L->cur,"CLEAR_VAR")||
      kw(&L->cur,"DELETE_VAR")||kw(&L->cur,"DROP_VAR")||kw(&L->cur,"VAR_CLEAR")){
    char name[48];
    long removed = 0;
    char buf[8];
    int i;
    lex_next(L);
    if (L->cur.kind!=TK_IDENT && L->cur.kind!=TK_STR){
      fail(vm,"UNSET name"); return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    for (i = 0; i < vm->n_vars; i++) {
      if (strcmp(vm->vars[i].name, name) == 0) {
        int j;
        for (j = i; j < vm->n_vars - 1; j++)
          vm->vars[j] = vm->vars[j + 1];
        vm->n_vars--;
        if (vm->n_vars >= 0 && vm->n_vars < 128)
          memset(&vm->vars[vm->n_vars], 0, sizeof(Var));
        removed = 1;
        break;
      }
    }
    var_set_num(vm, "UNSET_N", removed);
    var_set_num(vm, "LAST_N", removed);
    var_set_num(vm, "OK", 1);
    vm->last_n = removed;
    snprintf(buf, sizeof buf, "%ld", removed);
    var_set_str(vm, "LAST", buf);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
    if (vm->trace)
      fprintf(vm->trace, "# unset %s → %ld\n", name, removed);
    bump(vm); return 1;
  }
  /* TYPEOF name — kind of program var for agents (pair with DEFINED).
   * LAST/TYPE = "undef"|"num"|"str"; LAST_N/TYPE_N = 0|1|2; OK=1. */
  if (kw(&L->cur,"TYPEOF")||kw(&L->cur,"TYPE")||kw(&L->cur,"VARTYPE")||
      kw(&L->cur,"KIND")||kw(&L->cur,"VAR_KIND")||kw(&L->cur,"TYPE_OF")){
    char name[48];
    const char *kind = "undef";
    long code = 0;
    Var *v;
    lex_next(L);
    if (L->cur.kind!=TK_IDENT && L->cur.kind!=TK_STR){
      fail(vm,"TYPEOF name"); return -1;
    }
    snprintf(name, sizeof name, "%s", L->cur.text);
    lex_next(L);
    v = var_get(vm, name, 0);
    if (v) {
      if (v->is_str) { kind = "str"; code = 2; }
      else { kind = "num"; code = 1; }
    }
    var_set_str(vm, "TYPE", kind);
    var_set_str(vm, "LAST", kind);
    snprintf(vm->last_str, sizeof vm->last_str, "%s", kind);
    var_set_num(vm, "TYPE_N", code);
    var_set_num(vm, "LAST_N", code);
    var_set_num(vm, "OK", 1);
    vm->last_n = code;
    if (vm->trace)
      fprintf(vm->trace, "# typeof %s → %s (%ld)\n", name, kind, code);
    bump(vm); return 1;
  }
  /* DEFAULT name = expr|str — assign only if name is not yet defined.
   * Usability: INCLUDE libs set knobs without clobbering caller LET.
   * LAST_N/DEFAULT_SET = 1 if applied, 0 if skipped; OK always 1. */
  if (kw(&L->cur,"DEFAULT")||kw(&L->cur,"SETDEFAULT")||kw(&L->cur,"DEFAULT_LET")||
      kw(&L->cur,"LET_DEFAULT")||kw(&L->cur,"ORLET")){
    int applied = 0;
    int exists;
    char name[48];
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"DEFAULT name = value"); return -1; }
    snprintf(name,sizeof name,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_EQ){ fail(vm,"DEFAULT name = value"); return -1; }
    lex_next(L);
    exists = (var_get(vm, name, 0) != NULL);
    if (L->cur.kind==TK_STR || (L->cur.kind==TK_IDENT && (
          strcmp(L->cur.text,"LAST")==0 ||
          (var_get(vm,L->cur.text,0) && var_get(vm,L->cur.text,0)->is_str)))){
      char buf[CUBALC_HOST_STR_MAX]; buf[0]=0;
      for(;;){
        if (L->cur.kind==TK_STR){
          size_t bl=strlen(buf), al=strlen(L->cur.text);
          if (bl+al+1 < sizeof buf) memcpy(buf+bl, L->cur.text, al+1);
          lex_next(L);
        } else if (L->cur.kind==TK_IDENT){
          if (strcmp(L->cur.text,"LAST")==0){
            size_t bl=strlen(buf), al=strlen(vm->last_str);
            if (bl+al+1 < sizeof buf) memcpy(buf+bl, vm->last_str, al+1);
            lex_next(L);
          } else {
            Var *v = var_get(vm, L->cur.text, 0);
            if (v && v->is_str){
              size_t bl=strlen(buf), al=strlen(v->sval);
              if (bl+al+1 < sizeof buf) memcpy(buf+bl, v->sval, al+1);
            } else if (v){
              char nb[32]; snprintf(nb,sizeof nb,"%ld", v->val);
              size_t bl=strlen(buf), al=strlen(nb);
              if (bl+al+1 < sizeof buf) memcpy(buf+bl, nb, al+1);
            } else break;
            lex_next(L);
          }
        } else break;
        if (L->cur.kind==TK_PLUS){ lex_next(L); continue; }
        break;
      }
      if (!exists){
        var_set_str(vm, name, buf);
        applied = 1;
      }
    } else {
      long v=parse_expr(vm,L);
      if (!exists){
        var_set_num(vm, name, v);
        applied = 1;
      }
    }
    var_set_num(vm, "DEFAULT_SET", (long)applied);
    var_set_num(vm, "LAST_N", (long)applied);
    var_set_num(vm, "OK", 1);
    vm->last_n = (long)applied;
    {
      char nb[8];
      snprintf(nb, sizeof nb, "%d", applied);
      var_set_str(vm, "LAST", nb);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", nb);
    }
    if (vm->trace)
      fprintf(vm->trace, "# default %s %s\n", name, applied ? "set" : "skip");
    bump(vm); return 1;
  }
  if (kw(&L->cur,"LET")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"LET"); return -1; }
    char name[48]; snprintf(name,sizeof name,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_EQ){ fail(vm,"LET ="); return -1; }
    lex_next(L);
    if (L->cur.kind==TK_STR || (L->cur.kind==TK_IDENT && (
          strcmp(L->cur.text,"LAST")==0 ||
          (var_get(vm,L->cur.text,0) && var_get(vm,L->cur.text,0)->is_str)))){
      char buf[CUBALC_HOST_STR_MAX]; buf[0]=0;
      for(;;){
        if (L->cur.kind==TK_STR){
          size_t bl=strlen(buf), al=strlen(L->cur.text);
          if (bl+al+1 < sizeof buf) memcpy(buf+bl, L->cur.text, al+1);
          lex_next(L);
        } else if (L->cur.kind==TK_IDENT){
          if (strcmp(L->cur.text,"LAST")==0){
            size_t bl=strlen(buf), al=strlen(vm->last_str);
            if (bl+al+1 < sizeof buf) memcpy(buf+bl, vm->last_str, al+1);
            lex_next(L);
          } else {
            Var *v = var_get(vm, L->cur.text, 0);
            if (v && v->is_str){
              size_t bl=strlen(buf), al=strlen(v->sval);
              if (bl+al+1 < sizeof buf) memcpy(buf+bl, v->sval, al+1);
            } else if (v){
              char nb[32]; snprintf(nb,sizeof nb,"%ld", v->val);
              size_t bl=strlen(buf), al=strlen(nb);
              if (bl+al+1 < sizeof buf) memcpy(buf+bl, nb, al+1);
            } else break;
            lex_next(L);
          }
        } else break;
        if (L->cur.kind==TK_PLUS){ lex_next(L); continue; }
        break;
      }
      var_set_str(vm, name, buf);
      bump(vm); return 1;
    }
    long v=parse_expr(vm,L);
    var_set_num(vm, name, v);
    bump(vm); return 1;
  }
  return 0;
}
