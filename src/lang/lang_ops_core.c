/* CubalC lang — lang_ops_core.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"
#if !defined(CUBALC_OS_WINDOWS)
#  include <pwd.h>
#  include <fnmatch.h>
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
    /* SYS GLOB|MATCHFILES|WILDCARD path pattern
     * — list basenames under path matching shell-style pattern (* ? []).
     * Default pattern "*" when omitted. Soft miss / non-dir: OK=0 empty LAST.
     * LAST = matching names newline-joined; LAST_N/GLOB_N = count.
     * Usability: find plates/proofs without shell ls|grep glue. */
    if (kw(&L->cur,"GLOB") || kw(&L->cur,"MATCHFILES") || kw(&L->cur,"WILDCARD") ||
        kw(&L->cur,"FILEGLOB") || kw(&L->cur,"LSGLOB") || kw(&L->cur,"DIRGLOB") ||
        kw(&L->cur,"MATCHDIR") || kw(&L->cur,"FNMATCH") || kw(&L->cur,"GLOBLS")){
      char path[512], pat[256], out[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr;
      const char *p, *start;
      size_t flen, olen = 0;
      long kept = 0;
      lex_next(L);
      path[0] = 0; pat[0] = 0; out[0] = 0;
      snprintf(pat, sizeof pat, "%s", "*");
      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, "SYS GLOB \"path\" [\"pattern\"]"); return -1;
      }
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, pat, sizeof pat) != 0)
          snprintf(pat, sizeof pat, "%s", "*");
        if (!pat[0]) snprintf(pat, sizeof pat, "%s", "*");
      }
      if (cubalc_host_listdir(path, &hr) != 0) {
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "GLOB_N", 0);
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "GLOB", "");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        }
        bump(vm); return 1;
      }
#if defined(CUBALC_OS_WINDOWS)
      /* no fnmatch: fall back to full list when pattern is "*", else substring * strip */
      if (strcmp(pat, "*") == 0) {
        snprintf(out, sizeof out, "%s", hr.str);
        kept = hr.n;
      } else {
        p = hr.str;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            char name[512];
            size_t take = flen;
            int hit = 0;
            if (take >= sizeof name) take = sizeof name - 1;
            memcpy(name, start, take);
            name[take] = 0;
            /* minimal: *suffix, prefix*, or exact */
            if (pat[0] == '*' && pat[1] && !strchr(pat + 1, '*'))
              hit = (take >= strlen(pat + 1) &&
                     strcmp(name + take - strlen(pat + 1), pat + 1) == 0);
            else if (pat[0] && pat[strlen(pat) - 1] == '*' &&
                     !strchr(pat, '*') /* only trailing */)
              hit = 0; /* handled below */
            else if (pat[0] && pat[strlen(pat) - 1] == '*') {
              size_t pn = strlen(pat) - 1;
              hit = (strncmp(name, pat, pn) == 0);
            } else
              hit = (strcmp(name, pat) == 0);
            if (hit) {
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + take < sizeof out) {
                memcpy(out + olen, name, take);
                olen += take;
              }
              out[olen] = 0;
              kept++;
            }
          }
          if (*p == '\n') p++;
        }
      }
#else
      p = hr.str;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen > 0) {
          char name[512];
          size_t take = flen;
          if (take >= sizeof name) take = sizeof name - 1;
          memcpy(name, start, take);
          name[take] = 0;
          if (fnmatch(pat, name, 0) == 0) {
            if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + take < sizeof out) {
              memcpy(out + olen, name, take);
              olen += take;
            }
            out[olen] = 0;
            kept++;
          }
        }
        if (*p == '\n') p++;
      }
#endif
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "GLOB", out);
      var_set_str(vm, "MATCHFILES", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "GLOB_N", kept);
      var_set_num(vm, "MATCHFILES_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS PATHGLOB|PGLOB|FULLGLOB|GLOBPATH path_or_pattern [pattern]
     * — like GLOB but LAST holds full paths (dir/basename), ready for READ/RM.
     * One-arg shell style: "dir/*.plate" splits on last /; "*.log" uses cwd;
     * bare "dir" (no meta) → all entries under dir. Two-arg: path + pattern.
     * Soft miss / non-dir: OK=0 empty LAST. Usability: one-shot plate discovery. */
    if (kw(&L->cur,"PATHGLOB") || kw(&L->cur,"PGLOB") || kw(&L->cur,"FULLGLOB") ||
        kw(&L->cur,"GLOBPATH") || kw(&L->cur,"PATHMATCH") || kw(&L->cur,"LSPATH") ||
        kw(&L->cur,"MATCHPATHS") || kw(&L->cur,"FILEPATHS") || kw(&L->cur,"GLOBFULL")){
      char arg1[512], path[512], pat[256], out[CUBALC_HOST_STR_MAX];
      cubalc_host_result hr, jr;
      const char *p, *start, *slash;
      size_t flen, olen = 0;
      long kept = 0;
      int has_meta = 0;
      lex_next(L);
      arg1[0] = 0; path[0] = 0; pat[0] = 0; out[0] = 0;
      snprintf(pat, sizeof pat, "%s", "*");
      if (resolve_str_arg(vm, L, arg1, sizeof arg1) != 0) {
        fail(vm, "SYS PATHGLOB \"path|pattern\" [\"pattern\"]"); return -1;
      }
      if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        /* two-arg: path pattern */
        snprintf(path, sizeof path, "%s", arg1);
        if (resolve_str_arg(vm, L, pat, sizeof pat) != 0)
          snprintf(pat, sizeof pat, "%s", "*");
        if (!pat[0]) snprintf(pat, sizeof pat, "%s", "*");
      } else {
        /* one-arg: shell-style. Split dir/pattern only when basename has meta
         * (* ? []) — else whole arg is the directory (avoid /tmp/dir → pat=dir). */
        slash = strrchr(arg1, '/');
#ifdef CUBALC_OS_WINDOWS
        {
          const char *bs = strrchr(arg1, '\\');
          if (bs && (!slash || bs > slash)) slash = bs;
        }
#endif
        {
          const char *base = slash ? slash + 1 : arg1;
          for (p = base; *p; p++) {
            if (*p == '*' || *p == '?' || *p == '[') { has_meta = 1; break; }
          }
          /* trailing slash → all under dir */
          if (slash && !slash[1]) has_meta = 1;
        }
        if (slash && has_meta) {
          size_t dlen = (size_t)(slash - arg1);
          if (dlen == 0) {
            snprintf(path, sizeof path, "%s", "/");
          } else {
            if (dlen >= sizeof path) dlen = sizeof path - 1;
            memcpy(path, arg1, dlen);
            path[dlen] = 0;
          }
          if (slash[1])
            snprintf(pat, sizeof pat, "%s", slash + 1);
          else
            snprintf(pat, sizeof pat, "%s", "*");
        } else if (!slash && has_meta) {
          snprintf(path, sizeof path, "%s", ".");
          snprintf(pat, sizeof pat, "%s", arg1);
        } else {
          /* bare directory path (may contain /) → all entries full paths */
          snprintf(path, sizeof path, "%s", arg1);
          snprintf(pat, sizeof pat, "%s", "*");
        }
      }
      if (cubalc_host_listdir(path, &hr) != 0) {
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "PATHGLOB_N", 0);
        var_set_num(vm, "PGLOB_N", 0);
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "PATHGLOB", "");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        if (hr.err[0]) {
          var_set_str(vm, "LAST_ERR", hr.err);
          var_set_str(vm, "ERR", hr.err);
        }
        bump(vm); return 1;
      }
      p = hr.str;
      while (*p) {
        start = p;
        while (*p && *p != '\n') p++;
        flen = (size_t)(p - start);
        if (flen > 0) {
          char name[512];
          size_t take = flen;
          int hit = 0;
          if (take >= sizeof name) take = sizeof name - 1;
          memcpy(name, start, take);
          name[take] = 0;
#if defined(CUBALC_OS_WINDOWS)
          if (strcmp(pat, "*") == 0)
            hit = 1;
          else if (pat[0] == '*' && pat[1] && !strchr(pat + 1, '*'))
            hit = (take >= strlen(pat + 1) &&
                   strcmp(name + take - strlen(pat + 1), pat + 1) == 0);
          else if (pat[0] && pat[strlen(pat) - 1] == '*') {
            size_t pn = strlen(pat) - 1;
            hit = (strncmp(name, pat, pn) == 0);
          } else
            hit = (strcmp(name, pat) == 0);
#else
          hit = (fnmatch(pat, name, 0) == 0);
#endif
          if (hit) {
            if (cubalc_host_join(path, name, &jr) != 0)
              snprintf(jr.str, sizeof jr.str, "%s/%s", path, name);
            {
              size_t jlen = strlen(jr.str);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + jlen < sizeof out) {
                memcpy(out + olen, jr.str, jlen);
                olen += jlen;
              }
              out[olen] = 0;
              kept++;
            }
          }
        }
        if (*p == '\n') p++;
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "PATHGLOB", out);
      var_set_str(vm, "PGLOB", out);
      var_set_str(vm, "FULLGLOB", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "PATHGLOB_N", kept);
      var_set_num(vm, "PGLOB_N", kept);
      var_set_num(vm, "FULLGLOB_N", kept);
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
    /* SYS LENALL|MAPLEN|FIELDLENS [bag] — length of every newline field as a
     * decimal string bag → LAST. LAST_N/LENALL_N = field count;
     * LENALL_SUM = sum of lengths (total chars excl. newlines).
     * Usability: size rollups / max field width without EACH+LEN+PUSH glue;
     * pairs with SYS SUM/MAX/AVG/SORTN on the length bag. */
    if (kw(&L->cur,"LENALL") || kw(&L->cur,"MAPLEN") || kw(&L->cur,"FIELDLENS") ||
        kw(&L->cur,"STRLENS") || kw(&L->cur,"LENBAG") || kw(&L->cur,"LENGTHS") ||
        kw(&L->cur,"SIZES") || kw(&L->cur,"BYTELENS") || kw(&L->cur,"WIDTHS")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], nbuf[32];
      const char *p, *start;
      size_t olen = 0, flen, nlen;
      long kept = 0, sum = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          sum += (long)flen;
          snprintf(nbuf, sizeof nbuf, "%ld", (long)flen);
          nlen = strlen(nbuf);
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + nlen < sizeof out) {
            memcpy(out + olen, nbuf, nlen);
            olen += nlen;
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            memcpy(out + olen, nbuf, t);
            olen += t;
          }
          out[olen] = 0;
          kept++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "LENALL_N", kept);
      var_set_num(vm, "LENALL_SUM", sum);
      var_set_num(vm, "MAPLEN_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MAXLEN|MAXWIDTH|FIELDMAX [bag] — max string length among bag fields → LAST_N.
     * SYS MINLEN|MINWIDTH|FIELDMIN [bag] — min length. LAST = decimal of that length.
     * MAXLEN_N/MINLEN_N = field count; MAXLEN_I/MINLEN_I = 0-based index of first extreme
     * (-1 if empty bag). Empty bag → LAST_N 0.
     * Usability: column width for PADALL without LENALL+MAX glue. */
    if (kw(&L->cur,"MAXLEN") || kw(&L->cur,"MAXWIDTH") || kw(&L->cur,"FIELDMAX") ||
        kw(&L->cur,"LONGEST_LEN") || kw(&L->cur,"MAXSTRLEN") || kw(&L->cur,"BAGMAXLEN") ||
        kw(&L->cur,"MINLEN") || kw(&L->cur,"MINWIDTH") || kw(&L->cur,"FIELDMIN") ||
        kw(&L->cur,"SHORTEST_LEN") || kw(&L->cur,"MINSTRLEN") || kw(&L->cur,"BAGMINLEN")){
      char op[20];
      int is_min = 0;
      char bag[CUBALC_HOST_STR_MAX], out[40];
      const char *p, *start;
      size_t flen;
      long kept = 0, best = 0, best_i = -1, idx = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "MINLEN") == 0 || strcmp(op, "MINWIDTH") == 0 ||
          strcmp(op, "FIELDMIN") == 0 || strcmp(op, "SHORTEST_LEN") == 0 ||
          strcmp(op, "MINSTRLEN") == 0 || strcmp(op, "BAGMINLEN") == 0)
        is_min = 1;
      lex_next(L);
      bag[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (kept == 0) {
            best = (long)flen;
            best_i = idx;
          } else if (is_min) {
            if ((long)flen < best) {
              best = (long)flen;
              best_i = idx;
            }
          } else {
            if ((long)flen > best) {
              best = (long)flen;
              best_i = idx;
            }
          }
          kept++;
          idx++;
          if (*p == '\n') p++;
        }
      }
      snprintf(out, sizeof out, "%ld", kept ? best : 0L);
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept ? best : 0;
      var_set_num(vm, "LAST_N", kept ? best : 0);
      if (is_min) {
        var_set_num(vm, "MINLEN", kept ? best : 0);
        var_set_num(vm, "MINLEN_N", kept);
        var_set_num(vm, "MINLEN_I", best_i);
        var_set_num(vm, "MINWIDTH", kept ? best : 0);
      } else {
        var_set_num(vm, "MAXLEN", kept ? best : 0);
        var_set_num(vm, "MAXLEN_N", kept);
        var_set_num(vm, "MAXLEN_I", best_i);
        var_set_num(vm, "MAXWIDTH", kept ? best : 0);
      }
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS LONGEST|LONGESTLINE|MAXFIELD [bag] — first longest bag field text → LAST.
     * SYS SHORTEST|SHORTESTLINE|MINFIELD [bag] — first shortest field.
     * LAST_N 0|1 found; LONGEST_I/SHORTEST_I = index (-1 miss);
     * LONGEST_LEN/SHORTEST_LEN = length of that field; LONGEST_N = field count.
     * Distinct from MAXLEN/MINLEN (numeric length only) and LONGEST_LEN alias of MAXLEN.
     * Usability: pick longest error/payload without MAXLEN_I+NTH glue. */
    if (kw(&L->cur,"LONGEST") || kw(&L->cur,"LONGESTLINE") || kw(&L->cur,"MAXFIELD") ||
        kw(&L->cur,"LONGESTSTR") || kw(&L->cur,"PICKLONGEST") || kw(&L->cur,"BAGLONGEST") ||
        kw(&L->cur,"SHORTEST") || kw(&L->cur,"SHORTESTLINE") || kw(&L->cur,"MINFIELD") ||
        kw(&L->cur,"SHORTESTSTR") || kw(&L->cur,"PICKSHORTEST") || kw(&L->cur,"BAGSHORTEST")){
      char op[20];
      int is_short = 0;
      char bag[CUBALC_HOST_STR_MAX], out[512];
      const char *p, *start;
      size_t flen, best_len = 0;
      long kept = 0, best_i = -1, idx = 0, found = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "SHORTEST") == 0 || strcmp(op, "SHORTESTLINE") == 0 ||
          strcmp(op, "MINFIELD") == 0 || strcmp(op, "SHORTESTSTR") == 0 ||
          strcmp(op, "PICKSHORTEST") == 0 || strcmp(op, "BAGSHORTEST") == 0)
        is_short = 1;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (kept == 0 ||
              (is_short && flen < best_len) ||
              (!is_short && flen > best_len)) {
            best_len = flen;
            best_i = idx;
            {
              size_t take = flen;
              if (take >= sizeof out) take = sizeof out - 1;
              memcpy(out, start, take);
              out[take] = 0;
            }
            found = 1;
          }
          kept++;
          idx++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      if (is_short) {
        var_set_num(vm, "SHORTEST_N", kept);
        var_set_num(vm, "SHORTEST_I", best_i);
        var_set_num(vm, "SHORTEST_LEN", found ? (long)best_len : 0);
        var_set_str(vm, "SHORTEST", out);
      } else {
        var_set_num(vm, "LONGEST_N", kept);
        var_set_num(vm, "LONGEST_I", best_i);
        var_set_num(vm, "LONGEST_LEN", found ? (long)best_len : 0);
        var_set_str(vm, "LONGEST", out);
      }
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS COMMONPREFIX|LCP|SHAREDPREFIX [bag] — longest common prefix of all fields.
     * SYS COMMONSUFFIX|LCS|SHAREDSUFFIX [bag] — longest common suffix.
     * LAST = shared string; LAST_N = its length; COMMONPREFIX_N = field count.
     * Empty bag / no share → LAST empty, LAST_N 0. Single field → whole field.
     * Usability: path/peer dir roots or shared ext without EACH+STARTS glue. */
    if (kw(&L->cur,"COMMONPREFIX") || kw(&L->cur,"LCP") || kw(&L->cur,"SHAREDPREFIX") ||
        kw(&L->cur,"COMMONPRE") || kw(&L->cur,"BAGPREFIX") || kw(&L->cur,"SHAREPRE") ||
        kw(&L->cur,"COMMONSUFFIX") || kw(&L->cur,"LCS") || kw(&L->cur,"SHAREDSUFFIX") ||
        kw(&L->cur,"COMMONSUF") || kw(&L->cur,"BAGSUFFIX") || kw(&L->cur,"SHARESUF")){
      char op[20];
      int is_suf = 0;
      char bag[CUBALC_HOST_STR_MAX], out[512];
      char fields[64][256];
      size_t flens[64];
      const char *p, *start;
      size_t flen, i, j, n = 0, plen = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "COMMONSUFFIX") == 0 || strcmp(op, "LCS") == 0 ||
          strcmp(op, "SHAREDSUFFIX") == 0 || strcmp(op, "COMMONSUF") == 0 ||
          strcmp(op, "BAGSUFFIX") == 0 || strcmp(op, "SHARESUF") == 0)
        is_suf = 1;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (bag[0]) {
        p = bag;
        while (*p && n < 64) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen >= sizeof fields[0]) flen = sizeof fields[0] - 1;
          memcpy(fields[n], start, flen);
          fields[n][flen] = 0;
          flens[n] = flen;
          n++;
          if (*p == '\n') p++;
        }
      }
      if (n == 0) {
        plen = 0;
      } else if (n == 1) {
        plen = flens[0];
        if (plen >= sizeof out) plen = sizeof out - 1;
        memcpy(out, fields[0], plen);
        out[plen] = 0;
      } else if (!is_suf) {
        /* longest common prefix */
        plen = flens[0];
        for (i = 1; i < n; i++) {
          size_t m = flens[i] < plen ? flens[i] : plen;
          j = 0;
          while (j < m && fields[0][j] == fields[i][j]) j++;
          plen = j;
          if (plen == 0) break;
        }
        if (plen >= sizeof out) plen = sizeof out - 1;
        memcpy(out, fields[0], plen);
        out[plen] = 0;
      } else {
        /* longest common suffix */
        plen = flens[0];
        for (i = 1; i < n; i++) {
          size_t m = flens[i] < plen ? flens[i] : plen;
          j = 0;
          while (j < m &&
                 fields[0][flens[0] - 1 - j] == fields[i][flens[i] - 1 - j])
            j++;
          plen = j;
          if (plen == 0) break;
        }
        if (plen >= sizeof out) plen = sizeof out - 1;
        if (plen > 0)
          memcpy(out, fields[0] + (flens[0] - plen), plen);
        out[plen] = 0;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)plen;
      var_set_num(vm, "LAST_N", (long)plen);
      if (is_suf) {
        var_set_num(vm, "COMMONSUFFIX_N", (long)n);
        var_set_num(vm, "COMMONSUFFIX_LEN", (long)plen);
        var_set_str(vm, "COMMONSUFFIX", out);
        var_set_num(vm, "LCS_N", (long)n);
      } else {
        var_set_num(vm, "COMMONPREFIX_N", (long)n);
        var_set_num(vm, "COMMONPREFIX_LEN", (long)plen);
        var_set_str(vm, "COMMONPREFIX", out);
        var_set_num(vm, "LCP_N", (long)n);
      }
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS STRIPPREFIX|MAPSTRIPPRE bag [prefix] — remove leading prefix from each field
     * if present (only when field starts with prefix). Empty prefix → no-op copy.
     * SYS STRIPSUFFIX|MAPSTRIPSUF bag [suffix] — strip trailing suffix when present.
     * SYS STRIPCOMMON|STRIPLCP [bag] — compute LCP then strip it from every field
     * (relative paths in one step). LAST = bag; LAST_N = field count;
     * STRIPPREFIX_HIT / STRIPSUFFIX_HIT / STRIPCOMMON_HIT = fields shortened;
     * STRIPCOMMON_PRE = prefix removed (STRIPCOMMON only).
     * Usability: path relative after COMMONPREFIX without EACH+AFTER glue. */
    if (kw(&L->cur,"STRIPPREFIX") || kw(&L->cur,"MAPSTRIPPRE") || kw(&L->cur,"DROPPREFIX") ||
        kw(&L->cur,"REMOVEPREFIX") || kw(&L->cur,"CHOPPREFIX") || kw(&L->cur,"UNPREFIX") ||
        kw(&L->cur,"STRIPSUFFIX") || kw(&L->cur,"MAPSTRIPSUF") || kw(&L->cur,"DROPSUFFIX") ||
        kw(&L->cur,"REMOVESUFFIX") || kw(&L->cur,"CHOPSUFFIX") || kw(&L->cur,"UNSUFFIX") ||
        kw(&L->cur,"STRIPCOMMON") || kw(&L->cur,"STRIPLCP") || kw(&L->cur,"RELPATHS") ||
        kw(&L->cur,"STRIPROOT") || kw(&L->cur,"COMMONSTRIP")){
      char op[20];
      int is_suf = 0, is_common = 0;
      char bag[CUBALC_HOST_STR_MAX], needle[512], out[CUBALC_HOST_STR_MAX];
      char field[512], clipped[512];
      char fields[64][256];
      size_t flens[64];
      const char *p, *start;
      size_t flen, nlen, olen = 0, sn, i, j, n = 0, plen = 0;
      long kept = 0, hit = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "STRIPSUFFIX") == 0 || strcmp(op, "MAPSTRIPSUF") == 0 ||
          strcmp(op, "DROPSUFFIX") == 0 || strcmp(op, "REMOVESUFFIX") == 0 ||
          strcmp(op, "CHOPSUFFIX") == 0 || strcmp(op, "UNSUFFIX") == 0)
        is_suf = 1;
      if (strcmp(op, "STRIPCOMMON") == 0 || strcmp(op, "STRIPLCP") == 0 ||
          strcmp(op, "RELPATHS") == 0 || strcmp(op, "STRIPROOT") == 0 ||
          strcmp(op, "COMMONSTRIP") == 0)
        is_common = 1;
      lex_next(L);
      bag[0] = 0; needle[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (!is_common) {
        if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0] = 0;
      }
      /* parse fields; for STRIPCOMMON also compute LCP into needle */
      if (bag[0]) {
        p = bag;
        while (*p && n < 64) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen >= sizeof fields[0]) flen = sizeof fields[0] - 1;
          memcpy(fields[n], start, flen);
          fields[n][flen] = 0;
          flens[n] = flen;
          n++;
          if (*p == '\n') p++;
        }
      }
      if (is_common && n > 0) {
        plen = flens[0];
        for (i = 1; i < n; i++) {
          size_t m = flens[i] < plen ? flens[i] : plen;
          j = 0;
          while (j < m && fields[0][j] == fields[i][j]) j++;
          plen = j;
          if (plen == 0) break;
        }
        if (plen >= sizeof needle) plen = sizeof needle - 1;
        memcpy(needle, fields[0], plen);
        needle[plen] = 0;
      }
      nlen = strlen(needle);
      for (i = 0; i < n; i++) {
        flen = flens[i];
        memcpy(field, fields[i], flen + 1);
        sn = flen;
        if (nlen > 0 && flen >= nlen) {
          if (!is_suf) {
            if (memcmp(field, needle, nlen) == 0) {
              sn = flen - nlen;
              memcpy(clipped, field + nlen, sn);
              clipped[sn] = 0;
              hit++;
            } else {
              memcpy(clipped, field, sn);
              clipped[sn] = 0;
            }
          } else {
            if (memcmp(field + (flen - nlen), needle, nlen) == 0) {
              sn = flen - nlen;
              memcpy(clipped, field, sn);
              clipped[sn] = 0;
              hit++;
            } else {
              memcpy(clipped, field, sn);
              clipped[sn] = 0;
            }
          }
        } else {
          memcpy(clipped, field, sn);
          clipped[sn] = 0;
        }
        if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        if (olen + sn < sizeof out) {
          memcpy(out + olen, clipped, sn);
          olen += sn;
        } else if (olen < sizeof out - 1) {
          size_t t = sizeof out - 1 - olen;
          memcpy(out + olen, clipped, t);
          olen += t;
        }
        out[olen] = 0;
        kept++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "STRIPPREFIX_N", kept);
      var_set_num(vm, "STRIPSUFFIX_N", kept);
      var_set_num(vm, "STRIPCOMMON_N", kept);
      if (is_common) {
        var_set_num(vm, "STRIPCOMMON_HIT", hit);
        var_set_str(vm, "STRIPCOMMON_PRE", needle);
        var_set_num(vm, "STRIPCOMMON_LEN", (long)nlen);
      } else if (is_suf) {
        var_set_num(vm, "STRIPSUFFIX_HIT", hit);
      } else {
        var_set_num(vm, "STRIPPREFIX_HIT", hit);
      }
      var_set_num(vm, "OK", 1);
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
    /* SYS COALESCE|FIRSTS|NVL|IFNUL a b [c…] — first non-empty string arg → LAST.
     * SYS COALESCE BLANK|NB a b… — first non-blank (not only whitespace).
     * LAST_N = length of chosen; all empty → "" LAST_N=0 OK=1.
     * Usability: default chain for ENV/ARG plates without nested IF EMPTY. */
    if (kw(&L->cur,"COALESCE") || kw(&L->cur,"FIRSTS") || kw(&L->cur,"NVL") ||
        kw(&L->cur,"IFNUL") || kw(&L->cur,"IFNULL") || kw(&L->cur,"FIRSTNON") ||
        kw(&L->cur,"ORSTR") || kw(&L->cur,"FIRSTNONEMPTY")){
      char chosen[CUBALC_HOST_STR_MAX];
      char arg[CUBALC_HOST_STR_MAX];
      int skip_blank = 0, got = 0;
      long out_n = 0;
      lex_next(L);
      if (kw(&L->cur,"BLANK") || kw(&L->cur,"NB") || kw(&L->cur,"NONBLANK") ||
          kw(&L->cur,"TRIM") || kw(&L->cur,"WS")) {
        skip_blank = 1;
        lex_next(L);
      }
      chosen[0] = 0;
      while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
             L->cur.kind == TK_NUM) {
        arg[0] = 0;
        if (L->cur.kind == TK_NUM) {
          snprintf(arg, sizeof arg, "%ld", L->cur.num);
          lex_next(L);
        } else if (resolve_str_arg(vm, L, arg, sizeof arg) != 0) {
          break;
        }
        if (!got) {
          int empty = (arg[0] == 0);
          if (skip_blank && !empty) {
            const char *p = arg;
            while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
            empty = (*p == 0);
          }
          if (!empty) {
            snprintf(chosen, sizeof chosen, "%s", arg);
            got = 1;
          }
        }
        /* still consume remaining args even after choice */
      }
      out_n = (long)strlen(chosen);
      var_set_str(vm, "LAST", chosen);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", chosen);
      vm->last_n = out_n;
      var_set_num(vm, "LAST_N", out_n);
      var_set_num(vm, "COALESCE_N", out_n);
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
    /* SYS PICK|CHOICE|SAMPLE [str|LAST] — random newline field → LAST.
     * LAST_N/PICK_N = 0-based index chosen; empty bag → LAST="", LAST_N=-1, OK=0 soft.
     * Shares RAND seed (CUBALC_SEED). Usability: sample LIST/RANGE bag without shell. */
    if (kw(&L->cur,"PICK") || kw(&L->cur,"CHOICE") || kw(&L->cur,"SAMPLE") ||
        kw(&L->cur,"RANDLINE") || kw(&L->cur,"PICKLINE") || kw(&L->cur,"ANYLINE") ||
        kw(&L->cur,"DRAW") || kw(&L->cur,"LOT")){
      static int pick_seeded = 0;
      char src[CUBALC_HOST_STR_MAX];
      enum { PICK_MAX = 256, PICK_FLEN = 192 };
      char fields[PICK_MAX][PICK_FLEN];
      int n = 0, idx = -1;
      const char *p, *start;
      const char *se;
      lex_next(L);
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      if (src[0]) {
        p = src;
        while (*p && n < PICK_MAX) {
          start = p;
          while (*p && *p != '\n') p++;
          if (start == p && *p == 0 && start > src && start[-1] == '\n')
            break;
          {
            size_t flen = (size_t)(p - start);
            if (flen >= PICK_FLEN) flen = PICK_FLEN - 1;
            memcpy(fields[n], start, flen);
            fields[n][flen] = 0;
            n++;
          }
          if (*p == '\n') p++;
        }
      }
      if (!pick_seeded) {
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
        pick_seeded = 1;
      }
      if (n <= 0) {
        var_set_str(vm, "LAST", "");
        vm->last_str[0] = 0;
        vm->last_n = -1;
        var_set_num(vm, "LAST_N", -1);
        var_set_num(vm, "PICK_N", -1);
        var_set_num(vm, "OK", 0);
        bump(vm); return 1;
      }
      idx = (int)(rand() % n);
      var_set_str(vm, "LAST", fields[idx]);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", fields[idx]);
      vm->last_n = (long)idx;
      var_set_num(vm, "LAST_N", (long)idx);
      var_set_num(vm, "PICK_N", (long)idx);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SHUFFLE|SHUF|RANDPERM [str|LAST] — Fisher–Yates shuffle of newline fields → LAST.
     * LAST_N/SHUFFLE_N = field count; empty → "" OK=1. Shares RAND/CUBALC_SEED seed.
     * Usability: randomize peer/work bags without shell shuf; pairs with PICK/TAKE. */
    if (kw(&L->cur,"SHUFFLE") || kw(&L->cur,"SHUF") || kw(&L->cur,"RANDPERM") ||
        kw(&L->cur,"SHUFFLEL") || kw(&L->cur,"SHUFFLELINES") || kw(&L->cur,"SCRAMBLE") ||
        kw(&L->cur,"MIXLINES") || kw(&L->cur,"PERMUTE")){
      static int shuf_seeded = 0;
      char src[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      enum { SHUF_MAX = 256, SHUF_FLEN = 192 };
      char fields[SHUF_MAX][SHUF_FLEN];
      int n = 0, i, j;
      const char *p, *start, *se;
      size_t olen = 0;
      lex_next(L);
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      if (src[0]) {
        p = src;
        while (*p && n < SHUF_MAX) {
          start = p;
          while (*p && *p != '\n') p++;
          if (start == p && *p == 0 && start > src && start[-1] == '\n')
            break;
          {
            size_t flen = (size_t)(p - start);
            if (flen >= SHUF_FLEN) flen = SHUF_FLEN - 1;
            memcpy(fields[n], start, flen);
            fields[n][flen] = 0;
            n++;
          }
          if (*p == '\n') p++;
        }
      }
      if (!shuf_seeded) {
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
        shuf_seeded = 1;
      }
      /* Fisher–Yates */
      for (i = n - 1; i > 0; i--) {
        j = (int)(rand() % (i + 1));
        if (j != i) {
          char tmp[SHUF_FLEN];
          memcpy(tmp, fields[i], SHUF_FLEN);
          memcpy(fields[i], fields[j], SHUF_FLEN);
          memcpy(fields[j], tmp, SHUF_FLEN);
        }
      }
      out[0] = 0;
      for (i = 0; i < n; i++) {
        size_t flen = strlen(fields[i]);
        if (i > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
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
      var_set_num(vm, "SHUFFLE_N", (long)n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS DRAWN|SAMPLEK|NPICK|TAKERAND k [bag] — sample k unique bag fields without
     * replacement → LAST bag. Partial Fisher–Yates; k<=0 → empty; k>=n → all shuffled.
     * LAST_N/DRAWN_N = count returned. Distinct from PICK/SAMPLE (single field).
     * Shares RAND/CUBALC_SEED. Usability: multi-peer/work sample without SHUFFLE+TAKE. */
    if (kw(&L->cur,"DRAWN") || kw(&L->cur,"SAMPLEK") || kw(&L->cur,"NPICK") ||
        kw(&L->cur,"TAKERAND") || kw(&L->cur,"RSAMPLE") || kw(&L->cur,"DRAWK") ||
        kw(&L->cur,"SAMPLE_N") || kw(&L->cur,"PICKN") || kw(&L->cur,"CHOICEN") ||
        kw(&L->cur,"RANDTAKE")){
      static int drawn_seeded = 0;
      char src[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      enum { DRAW_MAX = 256, DRAW_FLEN = 192 };
      char fields[DRAW_MAX][DRAW_FLEN];
      int n = 0, i, j, k = 0, take;
      const char *p, *start, *se;
      size_t olen = 0;
      long kwant = 0;
      lex_next(L);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
        kwant = parse_expr(vm, L);
      if (kwant < 0) kwant = 0;
      if (kwant > DRAW_MAX) kwant = DRAW_MAX;
      k = (int)kwant;
      src[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      if (src[0]) {
        p = src;
        while (*p && n < DRAW_MAX) {
          start = p;
          while (*p && *p != '\n') p++;
          if (start == p && *p == 0 && start > src && start[-1] == '\n')
            break;
          {
            size_t flen = (size_t)(p - start);
            if (flen >= DRAW_FLEN) flen = DRAW_FLEN - 1;
            memcpy(fields[n], start, flen);
            fields[n][flen] = 0;
            n++;
          }
          if (*p == '\n') p++;
        }
      }
      if (!drawn_seeded) {
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
        drawn_seeded = 1;
      }
      take = k;
      if (take > n) take = n;
      /* partial Fisher–Yates: randomize first `take` positions */
      for (i = 0; i < take; i++) {
        int r = i + (int)(rand() % (n - i));
        if (r != i) {
          char tmp[DRAW_FLEN];
          memcpy(tmp, fields[i], DRAW_FLEN);
          memcpy(fields[i], fields[r], DRAW_FLEN);
          memcpy(fields[r], tmp, DRAW_FLEN);
        }
      }
      for (i = 0; i < take; i++) {
        size_t flen = strlen(fields[i]);
        if (i > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        if (olen + flen < sizeof out) {
          memcpy(out + olen, fields[i], flen);
          olen += flen;
        } else if (olen < sizeof out - 1) {
          size_t t = sizeof out - 1 - olen;
          memcpy(out + olen, fields[i], t);
          olen += t;
        }
        out[olen] = 0;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = (long)take;
      var_set_num(vm, "LAST_N", (long)take);
      var_set_num(vm, "DRAWN_N", (long)take);
      var_set_num(vm, "SAMPLEK_N", (long)take);
      var_set_num(vm, "NPICK_N", (long)take);
      var_set_num(vm, "DRAWN_TOTAL", (long)n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MIN|MAX a b [c…]|bag — host-plane min/max of integer args → LAST_N.
     * Bag mode (like SUM): one string/LAST with newline numeric fields.
     * SYS CLAMP x lo hi — bound x into [lo,hi] (lo/hi swapped if inverted).
     * COUNT = n used in bag mode. Distinct from cube ISA stack MIN/MAX/CLAMP.
     * Usability: cap retries/jitter; after LENALL find max field width without
     * SORTN+TAIL glue. */
    if (kw(&L->cur,"MIN") || kw(&L->cur,"MINIMUM") || kw(&L->cur,"MAX") ||
        kw(&L->cur,"MAXIMUM") || kw(&L->cur,"CLAMP") || kw(&L->cur,"BOUND") ||
        kw(&L->cur,"CLIP") || kw(&L->cur,"SATURATE") ||
        kw(&L->cur,"MINBAG") || kw(&L->cur,"MAXBAG") || kw(&L->cur,"BAGMIN") ||
        kw(&L->cur,"BAGMAX") || kw(&L->cur,"MINALL") || kw(&L->cur,"MAXALL")){
      char op[16];
      long vals[64];
      int n = 0, i, is_min, is_max, is_clamp, bag = 0;
      long out = 0;
      char buf[40];
      char src[CUBALC_HOST_STR_MAX];
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_min = (strcmp(op, "MIN") == 0 || strcmp(op, "MINIMUM") == 0 ||
                strcmp(op, "MINBAG") == 0 || strcmp(op, "BAGMIN") == 0 ||
                strcmp(op, "MINALL") == 0);
      is_max = (strcmp(op, "MAX") == 0 || strcmp(op, "MAXIMUM") == 0 ||
                strcmp(op, "MAXBAG") == 0 || strcmp(op, "BAGMAX") == 0 ||
                strcmp(op, "MAXALL") == 0);
      is_clamp = (strcmp(op, "CLAMP") == 0 || strcmp(op, "BOUND") == 0 ||
                  strcmp(op, "CLIP") == 0 || strcmp(op, "SATURATE") == 0);
      /* force bag mode for *BAG / *ALL aliases */
      if (strcmp(op, "MINBAG") == 0 || strcmp(op, "MAXBAG") == 0 ||
          strcmp(op, "BAGMIN") == 0 || strcmp(op, "BAGMAX") == 0 ||
          strcmp(op, "MINALL") == 0 || strcmp(op, "MAXALL") == 0)
        bag = 1;
      lex_next(L);
      if (is_clamp) {
        /* parse_prim: spaces between args must not become binary minus. */
        while (n < 16 && (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT ||
                          L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS)) {
          vals[n++] = parse_prim(vm, L);
        }
        {
          long x, lo, hi;
          if (n < 1) { x = vm->last_n; lo = 0; hi = 0; }
          else if (n == 1) { x = vals[0]; lo = 0; hi = vals[0]; }
          else if (n == 2) { x = vals[0]; lo = vals[1]; hi = vals[1]; }
          else { x = vals[0]; lo = vals[1]; hi = vals[2]; }
          if (lo > hi) { long t = lo; lo = hi; hi = t; }
          out = x;
          if (out < lo) out = lo;
          if (out > hi) out = hi;
        }
      } else {
        /* bag mode: string literal, string var, or no numeric-looking arg (→ LAST) */
        if (!bag) {
          if (L->cur.kind == TK_STR) {
            bag = 1;
          } else if (L->cur.kind == TK_IDENT) {
            Var *v = var_get(vm, L->cur.text, 0);
            if (v && v->is_str) bag = 1;
          } else if (!(L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
                       L->cur.kind == TK_LPAREN)) {
            bag = 1; /* bare: aggregate LAST bag/text */
          }
        }
        if (bag) {
          src[0] = 0;
          if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
            if (resolve_str_arg(vm, L, src, sizeof src) != 0)
              snprintf(src, sizeof src, "%s", vm->last_str);
          } else {
            snprintf(src, sizeof src, "%s", vm->last_str);
          }
          {
            const char *p = src;
            while (*p && n < 64) {
              const char *start = p;
              char *end = NULL;
              long v;
              while (*p && *p != '\n') p++;
              if (start == p) {
                if (*p == '\n') p++;
                continue;
              }
              {
                char tmp[48];
                size_t flen = (size_t)(p - start);
                if (flen >= sizeof tmp) flen = sizeof tmp - 1;
                memcpy(tmp, start, flen);
                tmp[flen] = 0;
                v = strtol(tmp, &end, 10);
                if (end != tmp) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0)
                    vals[n++] = v;
                }
              }
              if (*p == '\n') p++;
            }
          }
        } else {
          /* parse_prim (not parse_expr): SYS MIN -3 -1 → [-3,-1] not (-3 - 1). */
          while (n < 64 && (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT ||
                            L->cur.kind==TK_LPAREN || L->cur.kind==TK_MINUS)) {
            vals[n++] = parse_prim(vm, L);
          }
        }
        if (n <= 0) {
          out = bag ? 0 : vm->last_n;
        } else {
          out = vals[0];
          for (i = 1; i < n; i++) {
            if (is_min && vals[i] < out) out = vals[i];
            if (is_max && vals[i] > out) out = vals[i];
          }
        }
        var_set_num(vm, "COUNT", (long)n);
      }
      vm->last_n = out;
      var_set_num(vm, "LAST_N", out);
      if (is_min) var_set_num(vm, "MIN", out);
      if (is_max) var_set_num(vm, "MAX", out);
      if (is_clamp) var_set_num(vm, "CLAMP", out);
      var_set_num(vm, "OK", 1);
      snprintf(buf, sizeof buf, "%ld", out);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS ARGMAX|ARGMIN a b [c…]|bag — 0-based index of first extreme → LAST_N.
     * LAST = extreme value as decimal; ARGMAX_V/ARGMIN_V = value; COUNT = n.
     * Empty / no numbers → LAST_N=-1, OK=0. Bag mode like MIN/MAX/SUM.
     * Usability: LENALL → ARGMAX → NTH to pick longest field without EACH. */
    if (kw(&L->cur,"ARGMAX") || kw(&L->cur,"ARGMIN") || kw(&L->cur,"MAXIDX") ||
        kw(&L->cur,"MINIDX") || kw(&L->cur,"WHICHMAX") || kw(&L->cur,"WHICHMIN") ||
        kw(&L->cur,"IMAX") || kw(&L->cur,"IMIN") || kw(&L->cur,"INDEXMAX") ||
        kw(&L->cur,"INDEXMIN") || kw(&L->cur,"MAXI") || kw(&L->cur,"MINI")){
      char op[20];
      long vals[64];
      int n = 0, i, is_min, bag = 0;
      long best_i = -1, best_v = 0;
      char buf[40];
      char src[CUBALC_HOST_STR_MAX];
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_min = (strcmp(op, "ARGMIN") == 0 || strcmp(op, "MINIDX") == 0 ||
                strcmp(op, "WHICHMIN") == 0 || strcmp(op, "IMIN") == 0 ||
                strcmp(op, "INDEXMIN") == 0 || strcmp(op, "MINI") == 0);
      lex_next(L);
      if (L->cur.kind == TK_STR) {
        bag = 1;
      } else if (L->cur.kind == TK_IDENT) {
        Var *v = var_get(vm, L->cur.text, 0);
        if (v && v->is_str) bag = 1;
      } else if (!(L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
                   L->cur.kind == TK_LPAREN)) {
        bag = 1;
      }
      if (bag) {
        src[0] = 0;
        if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          if (resolve_str_arg(vm, L, src, sizeof src) != 0)
            snprintf(src, sizeof src, "%s", vm->last_str);
        } else {
          snprintf(src, sizeof src, "%s", vm->last_str);
        }
        {
          const char *p = src;
          while (*p && n < 64) {
            const char *start = p;
            char *end = NULL;
            long v;
            while (*p && *p != '\n') p++;
            if (start == p) {
              if (*p == '\n') p++;
              continue;
            }
            {
              char tmp[48];
              size_t flen = (size_t)(p - start);
              if (flen >= sizeof tmp) flen = sizeof tmp - 1;
              memcpy(tmp, start, flen);
              tmp[flen] = 0;
              v = strtol(tmp, &end, 10);
              if (end != tmp) {
                while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                  end++;
                if (end && *end == 0)
                  vals[n++] = v;
              }
            }
            if (*p == '\n') p++;
          }
        }
      } else {
        while (n < 64 && (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
                          L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)) {
          vals[n++] = parse_prim(vm, L);
        }
      }
      if (n <= 0) {
        best_i = -1;
        best_v = 0;
        var_set_num(vm, "OK", 0);
      } else {
        best_i = 0;
        best_v = vals[0];
        for (i = 1; i < n; i++) {
          if (is_min) {
            if (vals[i] < best_v) { best_v = vals[i]; best_i = i; }
          } else {
            if (vals[i] > best_v) { best_v = vals[i]; best_i = i; }
          }
        }
        var_set_num(vm, "OK", 1);
      }
      vm->last_n = best_i;
      var_set_num(vm, "LAST_N", best_i);
      if (is_min) {
        var_set_num(vm, "ARGMIN_I", best_i);
        var_set_num(vm, "ARGMIN_V", best_v);
        var_set_num(vm, "MINIDX", best_i);
      } else {
        var_set_num(vm, "ARGMAX_I", best_i);
        var_set_num(vm, "ARGMAX_V", best_v);
        var_set_num(vm, "MAXIDX", best_i);
      }
      var_set_num(vm, "COUNT", (long)n);
      snprintf(buf, sizeof buf, "%ld", best_v);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS IN|WITHIN|INRANGE x lo hi — inclusive numeric range membership → LAST_N 0|1.
     * lo/hi swapped if inverted. Distinct from SYS BETWEEN (string peel).
     * Usability: IF/guard retries and score bands without dual CMP tests. */
    if (kw(&L->cur,"IN") || kw(&L->cur,"WITHIN") || kw(&L->cur,"INRANGE") ||
        kw(&L->cur,"BETWEENN") || kw(&L->cur,"NUMIN") || kw(&L->cur,"ISIN") ||
        kw(&L->cur,"INCL") || kw(&L->cur,"BOUNDS")){
      long vals[8];
      int n = 0;
      long x, lo, hi, out;
      char buf[16];
      lex_next(L);
      while (n < 8 && (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
                        L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)) {
        vals[n++] = parse_prim(vm, L);
      }
      if (n < 1) { x = vm->last_n; lo = 0; hi = 0; }
      else if (n == 1) { x = vals[0]; lo = 0; hi = 0; }
      else if (n == 2) { x = vals[0]; lo = vals[1]; hi = vals[1]; }
      else { x = vals[0]; lo = vals[1]; hi = vals[2]; }
      if (lo > hi) { long t = lo; lo = hi; hi = t; }
      out = (x >= lo && x <= hi) ? 1L : 0L;
      vm->last_n = out;
      var_set_num(vm, "LAST_N", out);
      var_set_num(vm, "IN", out);
      var_set_num(vm, "OK", 1);
      snprintf(buf, sizeof buf, "%ld", out);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS CMP|NCMP a b — three-way numeric compare → LAST_N -1|0|1.
     * SYS SCMP|CMPS|STRCMP a b — lexicographic string compare → LAST_N -1|0|1.
     * SYS SCMPI|CMPSI — case-insensitive string compare.
     * Usability: sort keys / IF without dual LT+GT tests; pairs with MIN/MAX. */
    if (kw(&L->cur,"CMP") || kw(&L->cur,"NCMP") || kw(&L->cur,"ICMP") ||
        kw(&L->cur,"CMP3") || kw(&L->cur,"COMPARE") ||
        kw(&L->cur,"SCMP") || kw(&L->cur,"CMPS") || kw(&L->cur,"STRCMP") ||
        kw(&L->cur,"SCMPI") || kw(&L->cur,"CMPSI") || kw(&L->cur,"STRCMPI") ||
        kw(&L->cur,"IABS") || kw(&L->cur,"ABSVAL") || kw(&L->cur,"ABSNUM") ||
        kw(&L->cur,"NABS")){
      char op[16];
      long out = 0;
      char buf[40];
      int is_str, is_icase, is_abs;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_abs = (strcmp(op, "IABS") == 0 || strcmp(op, "ABSVAL") == 0 ||
                strcmp(op, "ABSNUM") == 0 || strcmp(op, "NABS") == 0);
      is_str = (strcmp(op, "SCMP") == 0 || strcmp(op, "CMPS") == 0 ||
                strcmp(op, "STRCMP") == 0 || strcmp(op, "SCMPI") == 0 ||
                strcmp(op, "CMPSI") == 0 || strcmp(op, "STRCMPI") == 0);
      is_icase = (strcmp(op, "SCMPI") == 0 || strcmp(op, "CMPSI") == 0 ||
                  strcmp(op, "STRCMPI") == 0);
      lex_next(L);
      if (is_abs) {
        long x = 0;
        if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
            L->cur.kind==TK_MINUS)
          x = parse_prim(vm, L);
        else
          x = vm->last_n;
        out = (x < 0) ? -x : x;
        var_set_num(vm, "ABS", out);
      } else if (is_str) {
        char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
        int r;
        a[0] = 0; b[0] = 0;
        if (!is_icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                          kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                          kw(&L->cur,"CI"))) {
          is_icase = 1;
          lex_next(L);
        }
        if (resolve_str_arg(vm, L, a, sizeof a) != 0)
          snprintf(a, sizeof a, "%s", vm->last_str);
        if (resolve_str_arg(vm, L, b, sizeof b) != 0) b[0] = 0;
        if (!is_icase) {
          r = strcmp(a, b);
        } else {
          const char *p = a, *q = b;
          r = 0;
          while (*p && *q) {
            char ca = *p, cb = *q;
            if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
            if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
            if (ca != cb) { r = (ca < cb) ? -1 : 1; break; }
            p++; q++;
          }
          if (r == 0) {
            if (*p) r = 1;
            else if (*q) r = -1;
          }
        }
        out = (r < 0) ? -1L : (r > 0 ? 1L : 0L);
        var_set_num(vm, "SCMP", out);
      } else {
        long a = 0, b = 0;
        if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
            L->cur.kind==TK_MINUS)
          a = parse_prim(vm, L);
        else
          a = vm->last_n;
        if (L->cur.kind==TK_NUM || L->cur.kind==TK_IDENT || L->cur.kind==TK_LPAREN ||
            L->cur.kind==TK_MINUS)
          b = parse_prim(vm, L);
        else
          b = 0;
        out = (a < b) ? -1L : (a > b ? 1L : 0L);
        var_set_num(vm, "CMP", out);
      }
      vm->last_n = out;
      var_set_num(vm, "LAST_N", out);
      var_set_num(vm, "OK", 1);
      snprintf(buf, sizeof buf, "%ld", out);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS SIGN|SGN x — signum → LAST_N -1|0|1 (pairs with IABS/CMP).
     * SYS DIV|IDIV|QUOT a b — trunc-toward-zero integer divide; /0 → 0 soft.
     * SYS MOD|REM|REMAINDER a b — a % b; %0 → 0 soft. Not path ABS/REALPATH.
     * Usability: page index, bucket, parity without cube ISA DIV/MOD soup. */
    if (kw(&L->cur,"SIGN") || kw(&L->cur,"SGN") || kw(&L->cur,"SIGNUM") ||
        kw(&L->cur,"DIV") || kw(&L->cur,"IDIV") || kw(&L->cur,"QUOT") ||
        kw(&L->cur,"QUOTIENT") || kw(&L->cur,"INTDIV") ||
        kw(&L->cur,"MOD") || kw(&L->cur,"REM") || kw(&L->cur,"REMAINDER") ||
        kw(&L->cur,"MODULO") || kw(&L->cur,"IMOD")){
      char op[16];
      long out = 0, a = 0, b = 0;
      char buf[40];
      int is_sign, is_div, is_mod;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_sign = (strcmp(op, "SIGN") == 0 || strcmp(op, "SGN") == 0 ||
                 strcmp(op, "SIGNUM") == 0);
      is_div = (strcmp(op, "DIV") == 0 || strcmp(op, "IDIV") == 0 ||
                strcmp(op, "QUOT") == 0 || strcmp(op, "QUOTIENT") == 0 ||
                strcmp(op, "INTDIV") == 0);
      is_mod = (strcmp(op, "MOD") == 0 || strcmp(op, "REM") == 0 ||
                strcmp(op, "REMAINDER") == 0 || strcmp(op, "MODULO") == 0 ||
                strcmp(op, "IMOD") == 0);
      lex_next(L);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)
        a = parse_prim(vm, L);
      else
        a = vm->last_n;
      if (!is_sign) {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
            L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)
          b = parse_prim(vm, L);
        else
          b = 0;
      }
      if (is_sign) {
        out = (a > 0) ? 1L : (a < 0 ? -1L : 0L);
        var_set_num(vm, "SIGN", out);
      } else if (is_div) {
        if (b == 0) out = 0;
        else if (a == LONG_MIN && b == -1) out = LONG_MAX; /* overflow guard */
        else out = a / b;
        var_set_num(vm, "DIV", out);
        var_set_num(vm, "QUOT", out);
      } else {
        /* mod */
        if (b == 0) out = 0;
        else if (a == LONG_MIN && b == -1) out = 0;
        else out = a % b;
        var_set_num(vm, "MOD", out);
        var_set_num(vm, "REM", out);
      }
      vm->last_n = out;
      var_set_num(vm, "LAST_N", out);
      var_set_num(vm, "OK", 1);
      snprintf(buf, sizeof buf, "%ld", out);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS GCD|HCF a b [c…] — greatest common divisor of integer args → LAST_N.
     * SYS LCM a b [c…] — least common multiple (via |a*b|/gcd, overflow-safe-ish).
     * Abs values used; GCD(0,0)=0; LCM with 0 → 0. Multi-arg reduces left-to-right.
     * Usability: reduce ratios / cycle periods without cube ISA GCD soup. */
    if (kw(&L->cur,"GCD") || kw(&L->cur,"HCF") || kw(&L->cur,"GCF") ||
        kw(&L->cur,"LCM") || kw(&L->cur,"LEASTCM") || kw(&L->cur,"LCMM")){
      char op[16];
      long vals[32];
      int n = 0, i, is_lcm;
      long out = 0;
      char buf[40];
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_lcm = (strcmp(op, "LCM") == 0 || strcmp(op, "LEASTCM") == 0 ||
                strcmp(op, "LCMM") == 0);
      lex_next(L);
      while (n < 32 && (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
                        L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)) {
        vals[n++] = parse_prim(vm, L);
      }
      if (n <= 0) {
        out = is_lcm ? 1 : 0; /* empty LCM=1, empty GCD=0 */
      } else {
        /* abs helper */
        #define CUBALC_IABS_L(x) ((x) < 0 ? -(x) : (x))
        out = CUBALC_IABS_L(vals[0]);
        for (i = 1; i < n; i++) {
          long a = out, b = CUBALC_IABS_L(vals[i]);
          if (is_lcm) {
            long g = a, h = b;
            while (h) { long t = g % h; g = h; h = t; }
            if (a == 0 || b == 0) out = 0;
            else {
              /* out = a / g * b — reduce first to limit overflow */
              out = (a / g) * b;
              if (out < 0) out = -out;
            }
          } else {
            long g = a, h = b;
            while (h) { long t = g % h; g = h; h = t; }
            out = g;
          }
        }
        #undef CUBALC_IABS_L
      }
      vm->last_n = out;
      var_set_num(vm, "LAST_N", out);
      var_set_num(vm, "OK", 1);
      if (is_lcm) {
        var_set_num(vm, "LCM", out);
      } else {
        var_set_num(vm, "GCD", out);
      }
      snprintf(buf, sizeof buf, "%ld", out);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS POW|POWER|IPOW a e — integer a^e (e>=0; e=0 → 1). Overflow soft-wraps in long.
     * SYS ISQRT|SQRT|IROOT2 n — floor integer square root (n<0 → 0).
     * Usability: backoff bases / geometry without cube ISA POW/SQRT soup. */
    if (kw(&L->cur,"POW") || kw(&L->cur,"POWER") || kw(&L->cur,"IPOW") ||
        kw(&L->cur,"EXPT") || kw(&L->cur,"EXPON") ||
        kw(&L->cur,"ISQRT") || kw(&L->cur,"SQRT") || kw(&L->cur,"IROOT2") ||
        kw(&L->cur,"SQRTR") || kw(&L->cur,"FLOORSQRT")){
      char op[16];
      long out = 0, a = 0, e = 0;
      char buf[40];
      int is_sqrt;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_sqrt = (strcmp(op, "ISQRT") == 0 || strcmp(op, "SQRT") == 0 ||
                 strcmp(op, "IROOT2") == 0 || strcmp(op, "SQRTR") == 0 ||
                 strcmp(op, "FLOORSQRT") == 0);
      lex_next(L);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)
        a = parse_prim(vm, L);
      else
        a = vm->last_n;
      if (is_sqrt) {
        if (a < 0) out = 0;
        else {
          /* binary search floor sqrt */
          long lo = 0, hi = a;
          if (hi > 3037000499L) hi = 3037000499L; /* max for hi*hi in signed 64 */
          while (lo <= hi) {
            long mid = lo + (hi - lo) / 2;
            long sq = mid * mid;
            if (sq == a) { out = mid; break; }
            if (sq < a) { out = mid; lo = mid + 1; }
            else hi = mid - 1;
          }
        }
        var_set_num(vm, "SQRT", out);
        var_set_num(vm, "ISQRT", out);
      } else {
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
            L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)
          e = parse_prim(vm, L);
        else
          e = 0;
        if (e < 0) {
          out = 0; /* negative exp not supported on ints */
        } else if (e == 0) {
          out = 1;
        } else {
          long r = 1;
          long base = a;
          long ee = e;
          /* binary exponentiation; soft wrap on overflow */
          while (ee > 0) {
            if (ee & 1) r = r * base;
            ee >>= 1;
            if (ee) base = base * base;
          }
          out = r;
        }
        var_set_num(vm, "POW", out);
      }
      vm->last_n = out;
      var_set_num(vm, "LAST_N", out);
      var_set_num(vm, "OK", 1);
      snprintf(buf, sizeof buf, "%ld", out);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS SUM|TOTAL a b [c…] — sum of integer args → LAST_N.
     * SYS PROD|PRODUCT|MULALL a b [c…] — product of integers.
     * SYS AVG|MEAN|AVERAGE a b [c…] — integer mean (trunc toward 0).
     * SYS MEDIAN|P50 a b [c…]|bag — integer median (sort; even → lower mid).
     * Bag mode: one string (or bare LAST) with newline fields → aggregate numeric
     * lines; blank/non-numeric fields skipped. COUNT = n used.
     * Usability: score bags / LIST sizes without shell awk; pairs with MIN/MAX. */
    if (kw(&L->cur,"SUM") || kw(&L->cur,"TOTAL") || kw(&L->cur,"SUMALL") ||
        kw(&L->cur,"PROD") || kw(&L->cur,"PRODUCT") || kw(&L->cur,"MULALL") ||
        kw(&L->cur,"PRODUCTALL") ||
        kw(&L->cur,"AVG") || kw(&L->cur,"MEAN") || kw(&L->cur,"AVERAGE") ||
        kw(&L->cur,"AVGALL") ||
        kw(&L->cur,"MEDIAN") || kw(&L->cur,"P50") || kw(&L->cur,"MED") ||
        kw(&L->cur,"MIDVAL") || kw(&L->cur,"MEDIANINT")){
      char op[16];
      long vals[64];
      int n = 0, i, is_sum, is_prod, is_avg, is_med, bag = 0;
      long out = 0, count = 0;
      char buf[40];
      char src[CUBALC_HOST_STR_MAX];
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_sum = (strcmp(op, "SUM") == 0 || strcmp(op, "TOTAL") == 0 ||
                strcmp(op, "SUMALL") == 0);
      is_prod = (strcmp(op, "PROD") == 0 || strcmp(op, "PRODUCT") == 0 ||
                 strcmp(op, "MULALL") == 0 || strcmp(op, "PRODUCTALL") == 0);
      is_avg = (strcmp(op, "AVG") == 0 || strcmp(op, "MEAN") == 0 ||
                strcmp(op, "AVERAGE") == 0 || strcmp(op, "AVGALL") == 0);
      is_med = (strcmp(op, "MEDIAN") == 0 || strcmp(op, "P50") == 0 ||
                strcmp(op, "MED") == 0 || strcmp(op, "MIDVAL") == 0 ||
                strcmp(op, "MEDIANINT") == 0);
      lex_next(L);
      /* bag mode: string literal, string var, or no numeric-looking arg (→ LAST) */
      if (L->cur.kind == TK_STR) {
        bag = 1;
      } else if (L->cur.kind == TK_IDENT) {
        Var *v = var_get(vm, L->cur.text, 0);
        if (v && v->is_str) bag = 1;
      } else if (!(L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
                   L->cur.kind == TK_LPAREN)) {
        bag = 1; /* bare: aggregate LAST bag/text */
      }
      if (bag) {
        src[0] = 0;
        if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
          if (resolve_str_arg(vm, L, src, sizeof src) != 0)
            snprintf(src, sizeof src, "%s", vm->last_str);
        } else {
          snprintf(src, sizeof src, "%s", vm->last_str);
        }
        {
          const char *p = src;
          while (*p && n < 64) {
            const char *start = p;
            char *end = NULL;
            long v;
            while (*p && *p != '\n') p++;
            /* empty field between newlines → skip */
            if (start == p) {
              if (*p == '\n') p++;
              continue;
            }
            /* parse field [start,p) as integer */
            {
              char tmp[48];
              size_t flen = (size_t)(p - start);
              if (flen >= sizeof tmp) flen = sizeof tmp - 1;
              memcpy(tmp, start, flen);
              tmp[flen] = 0;
              v = strtol(tmp, &end, 10);
              if (end != tmp) {
                /* allow trailing whitespace only */
                while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                  end++;
                if (end && *end == 0)
                  vals[n++] = v;
              }
            }
            if (*p == '\n') p++;
          }
        }
      } else {
        while (n < 64 && (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
                          L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)) {
          vals[n++] = parse_prim(vm, L);
        }
      }
      count = n;
      if (is_med) {
        /* insertion sort then middle (even n → lower mid, no float) */
        out = 0;
        if (n > 0) {
          for (i = 1; i < n; i++) {
            long key = vals[i];
            int j = i - 1;
            while (j >= 0 && vals[j] > key) {
              vals[j + 1] = vals[j];
              j--;
            }
            vals[j + 1] = key;
          }
          out = vals[(n - 1) / 2];
        }
      } else if (is_prod) {
        out = 1;
        for (i = 0; i < n; i++) out *= vals[i];
        if (n == 0) out = 1; /* empty product */
      } else if (is_avg) {
        out = 0;
        if (n > 0) {
          long s = 0;
          for (i = 0; i < n; i++) s += vals[i];
          out = s / n;
        }
      } else {
        /* sum / total */
        out = 0;
        for (i = 0; i < n; i++) out += vals[i];
      }
      vm->last_n = out;
      var_set_num(vm, "LAST_N", out);
      var_set_num(vm, "COUNT", count);
      var_set_num(vm, "OK", 1);
      if (is_sum) {
        var_set_num(vm, "SUM", out);
        var_set_num(vm, "SUM_N", out);
      }
      if (is_prod) {
        var_set_num(vm, "PROD", out);
        var_set_num(vm, "PROD_N", out);
      }
      if (is_avg) {
        var_set_num(vm, "AVG", out);
        var_set_num(vm, "AVG_N", out);
      }
      if (is_med) {
        var_set_num(vm, "MEDIAN", out);
        var_set_num(vm, "MEDIAN_N", out);
      }
      snprintf(buf, sizeof buf, "%ld", out);
      var_set_str(vm, "LAST", buf);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", buf);
      bump(vm); return 1;
    }
    /* SYS RANGE lo hi [step] — inclusive integer sequence → newline bag LAST.
     * SYS SEQ n — 1..n; SYS IOTA n — 0..n-1. Cap 256 fields.
     * Usability: EACH LINE / SORTN / SUM fixtures without shell seq(1). */
    if (kw(&L->cur,"RANGE") || kw(&L->cur,"SEQ") || kw(&L->cur,"SEQUENCE") ||
        kw(&L->cur,"IOTA") || kw(&L->cur,"ENUM") || kw(&L->cur,"NUMS") ||
        kw(&L->cur,"COUNTTO") || kw(&L->cur,"FROMTO")){
      char op[16];
      long lo = 0, hi = 0, step = 1;
      long n = 0, v, vals[256];
      int i, is_seq, is_iota, empty = 0;
      char out[CUBALC_HOST_STR_MAX];
      size_t olen = 0;
      char num[32];
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_seq = (strcmp(op, "SEQ") == 0 || strcmp(op, "SEQUENCE") == 0 ||
                strcmp(op, "COUNTTO") == 0);
      is_iota = (strcmp(op, "IOTA") == 0 || strcmp(op, "ENUM") == 0);
      lex_next(L);
      /* parse up to 3 prims: a [b [c]] */
      {
        long args[3];
        int na = 0;
        while (na < 3 && (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
                          L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)) {
          args[na++] = parse_prim(vm, L);
        }
        if (is_iota) {
          /* IOTA n → 0..n-1; IOTA lo hi → lo..hi-1 half-open; IOTA lo hi step */
          if (na <= 0 || (na == 1 && args[0] <= 0)) {
            empty = 1;
          } else if (na == 1) {
            lo = 0; hi = args[0] - 1; step = 1;
          } else if (na == 2) {
            if (args[1] <= args[0]) empty = 1;
            else { lo = args[0]; hi = args[1] - 1; step = 1; }
          } else {
            if (args[1] <= args[0] && (!args[2] || args[2] > 0)) empty = 1;
            else {
              lo = args[0]; hi = args[1] - 1; step = args[2] ? args[2] : 1;
            }
          }
        } else if (is_seq) {
          /* SEQ n → 1..n; SEQ lo hi [step] inclusive like RANGE */
          if (na <= 0 || (na == 1 && args[0] <= 0)) {
            empty = 1;
          } else if (na == 1) {
            lo = 1; hi = args[0]; step = 1;
          } else if (na == 2) {
            lo = args[0]; hi = args[1]; step = 1;
          } else {
            lo = args[0]; hi = args[1]; step = args[2] ? args[2] : 1;
          }
        } else {
          /* RANGE lo hi [step] inclusive */
          if (na <= 0) {
            empty = 1;
          } else if (na == 1) {
            if (args[0] <= 0) empty = 1;
            else { lo = 1; hi = args[0]; step = 1; } /* RANGE n ≡ SEQ n */
          } else if (na == 2) {
            lo = args[0]; hi = args[1]; step = 1;
          } else {
            lo = args[0]; hi = args[1]; step = args[2] ? args[2] : 1;
          }
        }
      }
      if (!empty) {
        if (step == 0) step = 1;
        /* auto-flip step when lo/hi inverted (RANGE 5 1 → descending) */
        if (lo > hi && step > 0) step = -step;
        if (lo < hi && step < 0) step = -step;
        v = lo;
        if (step > 0) {
          while (v <= hi && n < 256) { vals[n++] = v; v += step; }
        } else {
          while (v >= hi && n < 256) { vals[n++] = v; v += step; }
        }
      }
      out[0] = 0;
      for (i = 0; i < (int)n; i++) {
        int nn = snprintf(num, sizeof num, "%ld", vals[i]);
        if (nn < 0) nn = 0;
        if (i > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        if (olen + (size_t)nn < sizeof out) {
          memcpy(out + olen, num, (size_t)nn);
          olen += (size_t)nn;
        } else if (olen < sizeof out - 1) {
          size_t take = sizeof out - 1 - olen;
          memcpy(out + olen, num, take);
          olen += take;
        }
        out[olen] = 0;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "RANGE_N", n);
      var_set_num(vm, "SEQ_N", n);
      var_set_num(vm, "COUNT", n);
      var_set_num(vm, "OK", 1);
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
    /* SYS GREPANY|GREPOR|MULTIGREP [I] bag n1 [n2…] — keep fields matching any needle.
     * SYS GREPANYI — case-insensitive. Up to 8 needles. Empty needle list → keep all.
     * LAST_N/GREP_N = kept count.
     * Usability: multi-severity log triage without chained GREP/UNION glue. */
    if (kw(&L->cur,"GREPANY") || kw(&L->cur,"GREPOR") || kw(&L->cur,"MULTIGREP") ||
        kw(&L->cur,"ANYGREP") || kw(&L->cur,"GREPONEOF") || kw(&L->cur,"MATCHANY") ||
        kw(&L->cur,"GREPANYI") || kw(&L->cur,"GREPORI") || kw(&L->cur,"MULTIGREPI")){
      char op[20];
      int icase = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      char needles[8][128];
      int nn = 0, i;
      const char *p, *start;
      size_t olen = 0, flen;
      long kept = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "GREPANYI") == 0 || strcmp(op, "GREPORI") == 0 ||
          strcmp(op, "MULTIGREPI") == 0)
        icase = 1;
      lex_next(L);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      while (nn < 8 && (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
                        L->cur.kind == TK_NUM)) {
        if (L->cur.kind == TK_NUM) {
          snprintf(needles[nn], sizeof needles[0], "%ld", L->cur.num);
          lex_next(L);
          nn++;
        } else {
          if (resolve_str_arg(vm, L, needles[nn], sizeof needles[0]) != 0)
            break;
          nn++;
        }
      }
      if (bag[0]) {
        p = bag;
        while (*p) {
          int hit = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            char field[512];
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            if (nn == 0) {
              hit = 1; /* no needles → keep all */
            } else {
              for (i = 0; i < nn && !hit; i++) {
                if (needles[i][0] == 0) {
                  hit = 1;
                } else if (!icase) {
                  if (strstr(field, needles[i]) != NULL) hit = 1;
                } else {
                  size_t nl = strlen(needles[i]), fi, j;
                  for (fi = 0; field[fi] && !hit; fi++) {
                    for (j = 0; j < nl; j++) {
                      char a = field[fi + j], b = needles[i][j];
                      if (!a) break;
                      if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                      if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                      if (a != b) break;
                    }
                    if (j == nl) hit = 1;
                  }
                }
              }
            }
            if (hit) {
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + flen < sizeof out) {
                memcpy(out + olen, start, flen);
                olen += flen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, start, t);
                olen += t;
              }
              out[olen] = 0;
              kept++;
            }
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "GREP", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "GREP_N", kept);
      var_set_num(vm, "GREPANY_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS GREPALL|GREPAND|MATCHALL [I] bag n1 [n2…] — keep fields matching every needle.
     * SYS GREPALLI — case-insensitive. Up to 8 needles. Empty needle list → keep all.
     * Empty needle is always-match (does not drop fields). LAST_N/GREP_N/GREPALL_N = kept.
     * Usability: multi-tag AND triage without chained GREP (e.g. ERROR+disk without EACH). */
    if (kw(&L->cur,"GREPALL") || kw(&L->cur,"GREPAND") || kw(&L->cur,"MATCHALL") ||
        kw(&L->cur,"ALLGREP") || kw(&L->cur,"ANDGREP") || kw(&L->cur,"MULTIGREPAND") ||
        kw(&L->cur,"GREPALLI") || kw(&L->cur,"GREPANDI") || kw(&L->cur,"MATCHALLI")){
      char op[20];
      int icase = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      char needles[8][128];
      int nn = 0, i;
      const char *p, *start;
      size_t olen = 0, flen;
      long kept = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "GREPALLI") == 0 || strcmp(op, "GREPANDI") == 0 ||
          strcmp(op, "MATCHALLI") == 0)
        icase = 1;
      lex_next(L);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      while (nn < 8 && (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
                        L->cur.kind == TK_NUM)) {
        if (L->cur.kind == TK_NUM) {
          snprintf(needles[nn], sizeof needles[0], "%ld", L->cur.num);
          lex_next(L);
          nn++;
        } else {
          if (resolve_str_arg(vm, L, needles[nn], sizeof needles[0]) != 0)
            break;
          nn++;
        }
      }
      if (bag[0]) {
        p = bag;
        while (*p) {
          int hit = 1;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            char field[512];
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            if (nn > 0) {
              for (i = 0; i < nn && hit; i++) {
                int one = 0;
                if (needles[i][0] == 0) {
                  one = 1; /* empty needle always matches */
                } else if (!icase) {
                  if (strstr(field, needles[i]) != NULL) one = 1;
                } else {
                  size_t nl = strlen(needles[i]), fi, j;
                  for (fi = 0; field[fi] && !one; fi++) {
                    for (j = 0; j < nl; j++) {
                      char a = field[fi + j], b = needles[i][j];
                      if (!a) break;
                      if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                      if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                      if (a != b) break;
                    }
                    if (j == nl) one = 1;
                  }
                }
                if (!one) hit = 0;
              }
            }
            if (hit) {
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + flen < sizeof out) {
                memcpy(out + olen, start, flen);
                olen += flen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, start, t);
                olen += t;
              }
              out[olen] = 0;
              kept++;
            }
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "GREP", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "GREP_N", kept);
      var_set_num(vm, "GREPALL_N", kept);
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
    /* SYS UNION|ORLINES|SETUNION a [b…] — merge bags · first-seen unique fields → LAST.
     * SYS DISTINCT|UNIQUEALL [bag] — order-preserving full unique (unlike adjacent UNIQ).
     * SYS INTERSECT|ANDLINES|SETAND a b — fields of a also in b (order of a).
     * SYS DIFF|EXCEPT|SETDIFF|MINUSLINES a b — fields of a not in b (order of a).
     * LAST_N = kept count. Cap 256 fields × 192 chars.
     * Usability: work-bag merge/dedup/subtract without EACH+HASLINE rebuild. */
    if (kw(&L->cur,"UNION") || kw(&L->cur,"ORLINES") || kw(&L->cur,"SETUNION") ||
        kw(&L->cur,"SETOR") || kw(&L->cur,"DISTINCT") || kw(&L->cur,"UNIQUEALL") ||
        kw(&L->cur,"UALL") || kw(&L->cur,"DEDUPALL") ||
        kw(&L->cur,"INTERSECT") || kw(&L->cur,"ANDLINES") || kw(&L->cur,"SETAND") ||
        kw(&L->cur,"SETINTERSECT") ||
        kw(&L->cur,"DIFF") || kw(&L->cur,"EXCEPT") || kw(&L->cur,"SETDIFF") ||
        kw(&L->cur,"MINUSLINES") || kw(&L->cur,"LINEDIFF") || kw(&L->cur,"BAGDIFF")){
      char op[24];
      int mode; /* 0=union/distinct 1=intersect 2=diff */
      enum { SET_MAX = 256, SET_FLEN = 192 };
      char fields[SET_MAX][SET_FLEN];
      char bag[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      int n = 0, i;
      size_t olen = 0;
      long kept = 0;
      const char *p, *start;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "INTERSECT") == 0 || strcmp(op, "ANDLINES") == 0 ||
          strcmp(op, "SETAND") == 0 || strcmp(op, "SETINTERSECT") == 0)
        mode = 1;
      else if (strcmp(op, "DIFF") == 0 || strcmp(op, "EXCEPT") == 0 ||
               strcmp(op, "SETDIFF") == 0 || strcmp(op, "MINUSLINES") == 0 ||
               strcmp(op, "LINEDIFF") == 0 || strcmp(op, "BAGDIFF") == 0)
        mode = 2;
      else
        mode = 0; /* UNION / DISTINCT */
      lex_next(L);
      out[0] = 0;
      n = 0;
      if (mode == 0) {
        /* UNION / DISTINCT: absorb all args; keep first-seen order */
        int any = 0;
        while (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT ||
               L->cur.kind == TK_NUM) {
          bag[0] = 0;
          if (L->cur.kind == TK_NUM) {
            snprintf(bag, sizeof bag, "%ld", L->cur.num);
            lex_next(L);
          } else if (resolve_str_arg(vm, L, bag, sizeof bag) != 0) {
            break;
          }
          any = 1;
          if (!bag[0]) continue;
          p = bag;
          while (*p && n < SET_MAX) {
            start = p;
            while (*p && *p != '\n') p++;
            if (start == p && *p == 0 && start > bag && start[-1] == '\n')
              break;
            {
              size_t flen = (size_t)(p - start);
              int seen = 0;
              if (flen >= SET_FLEN) flen = SET_FLEN - 1;
              for (i = 0; i < n; i++) {
                if (strlen(fields[i]) == flen &&
                    (flen == 0 || memcmp(fields[i], start, flen) == 0)) {
                  seen = 1;
                  break;
                }
              }
              if (!seen) {
                memcpy(fields[n], start, flen);
                fields[n][flen] = 0;
                n++;
              }
            }
            if (*p == '\n') p++;
          }
        }
        if (!any) {
          /* zero args → LAST bag */
          snprintf(bag, sizeof bag, "%s", vm->last_str);
          if (bag[0]) {
            p = bag;
            while (*p && n < SET_MAX) {
              start = p;
              while (*p && *p != '\n') p++;
              if (start == p && *p == 0 && start > bag && start[-1] == '\n')
                break;
              {
                size_t flen = (size_t)(p - start);
                int seen = 0;
                if (flen >= SET_FLEN) flen = SET_FLEN - 1;
                for (i = 0; i < n; i++) {
                  if (strlen(fields[i]) == flen &&
                      (flen == 0 || memcmp(fields[i], start, flen) == 0)) {
                    seen = 1;
                    break;
                  }
                }
                if (!seen) {
                  memcpy(fields[n], start, flen);
                  fields[n][flen] = 0;
                  n++;
                }
              }
              if (*p == '\n') p++;
            }
          }
        }
        for (i = 0; i < n; i++) {
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          {
            size_t flen = strlen(fields[i]);
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
          kept++;
        }
      } else {
        /* INTERSECT / DIFF: a then b */
        char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
        char bfields[SET_MAX][SET_FLEN];
        int bn = 0;
        a[0] = 0; b[0] = 0;
        if (resolve_str_arg(vm, L, a, sizeof a) != 0)
          snprintf(a, sizeof a, "%s", vm->last_str);
        if (resolve_str_arg(vm, L, b, sizeof b) != 0)
          b[0] = 0;
        /* index fields of b */
        if (b[0]) {
          p = b;
          while (*p && bn < SET_MAX) {
            start = p;
            while (*p && *p != '\n') p++;
            if (start == p && *p == 0 && start > b && start[-1] == '\n')
              break;
            {
              size_t flen = (size_t)(p - start);
              if (flen >= SET_FLEN) flen = SET_FLEN - 1;
              memcpy(bfields[bn], start, flen);
              bfields[bn][flen] = 0;
              bn++;
            }
            if (*p == '\n') p++;
          }
        }
        /* walk a; keep if (intersect && in b) or (diff && not in b); first-seen only */
        if (a[0]) {
          p = a;
          while (*p && n < SET_MAX) {
            start = p;
            while (*p && *p != '\n') p++;
            if (start == p && *p == 0 && start > a && start[-1] == '\n')
              break;
            {
              size_t flen = (size_t)(p - start);
              int in_b = 0, seen = 0;
              if (flen >= SET_FLEN) flen = SET_FLEN - 1;
              for (i = 0; i < bn; i++) {
                if (strlen(bfields[i]) == flen &&
                    (flen == 0 || memcmp(bfields[i], start, flen) == 0)) {
                  in_b = 1;
                  break;
                }
              }
              for (i = 0; i < n; i++) {
                if (strlen(fields[i]) == flen &&
                    (flen == 0 || memcmp(fields[i], start, flen) == 0)) {
                  seen = 1;
                  break;
                }
              }
              if (!seen && ((mode == 1 && in_b) || (mode == 2 && !in_b))) {
                memcpy(fields[n], start, flen);
                fields[n][flen] = 0;
                n++;
              }
            }
            if (*p == '\n') p++;
          }
        }
        for (i = 0; i < n; i++) {
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          {
            size_t flen = strlen(fields[i]);
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
          kept++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "UNION_N", kept);
      var_set_num(vm, "SET_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS ZIP|PAIR|ZIPLINES a b [sep] — pair newline fields by index with sep → LAST.
     * Default sep ":". Shorter bag pads empty partner. LAST_N = pair count.
     * SYS KEYS|COL0|LEFTCOL bag [sep] — peel left of first sep each field → bag.
     * SYS VALS|COL1|RIGHTCOL bag [sep] — peel right of first sep each field → bag.
     * Usability: roster name+status plates without EACH+CAT glue; peel kv bags. */
    if (kw(&L->cur,"ZIP") || kw(&L->cur,"PAIR") || kw(&L->cur,"ZIPLINES") ||
        kw(&L->cur,"ZIPL") || kw(&L->cur,"PAIRS") ||
        kw(&L->cur,"KEYS") || kw(&L->cur,"COL0") || kw(&L->cur,"LEFTCOL") ||
        kw(&L->cur,"KEYCOL") || kw(&L->cur,"KVKEYS") ||
        kw(&L->cur,"VALS") || kw(&L->cur,"COL1") || kw(&L->cur,"RIGHTCOL") ||
        kw(&L->cur,"VALCOL") || kw(&L->cur,"KVVALS") || kw(&L->cur,"VALUES")){
      char op[20];
      int mode; /* 0=zip 1=keys 2=vals */
      enum { ZIP_MAX = 256, ZIP_FLEN = 192 };
      char af[ZIP_MAX][ZIP_FLEN], bf[ZIP_MAX][ZIP_FLEN];
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
      char sep[32], out[CUBALC_HOST_STR_MAX];
      int na = 0, nb = 0, n = 0, i;
      size_t olen = 0, sepn;
      const char *p, *start;
      long kept = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "KEYS") == 0 || strcmp(op, "COL0") == 0 ||
          strcmp(op, "LEFTCOL") == 0 || strcmp(op, "KEYCOL") == 0 ||
          strcmp(op, "KVKEYS") == 0)
        mode = 1;
      else if (strcmp(op, "VALS") == 0 || strcmp(op, "COL1") == 0 ||
               strcmp(op, "RIGHTCOL") == 0 || strcmp(op, "VALCOL") == 0 ||
               strcmp(op, "KVVALS") == 0 || strcmp(op, "VALUES") == 0)
        mode = 2;
      else
        mode = 0;
      lex_next(L);
      a[0] = 0; b[0] = 0; sep[0] = ':'; sep[1] = 0; out[0] = 0;
      if (mode == 0) {
        if (resolve_str_arg(vm, L, a, sizeof a) != 0)
          snprintf(a, sizeof a, "%s", vm->last_str);
        if (resolve_str_arg(vm, L, b, sizeof b) != 0)
          b[0] = 0;
        /* optional sep: string only (avoid eating next form); default ":" */
        if (L->cur.kind == TK_STR) {
          if (resolve_str_arg(vm, L, sep, sizeof sep) != 0)
            { sep[0] = ':'; sep[1] = 0; }
          if (!sep[0]) { sep[0] = ':'; sep[1] = 0; }
        }
        /* split a and b into fields */
        if (a[0]) {
          p = a;
          while (*p && na < ZIP_MAX) {
            start = p;
            while (*p && *p != '\n') p++;
            if (start == p && *p == 0 && start > a && start[-1] == '\n') break;
            {
              size_t flen = (size_t)(p - start);
              if (flen >= ZIP_FLEN) flen = ZIP_FLEN - 1;
              memcpy(af[na], start, flen);
              af[na][flen] = 0;
              na++;
            }
            if (*p == '\n') p++;
          }
        }
        if (b[0]) {
          p = b;
          while (*p && nb < ZIP_MAX) {
            start = p;
            while (*p && *p != '\n') p++;
            if (start == p && *p == 0 && start > b && start[-1] == '\n') break;
            {
              size_t flen = (size_t)(p - start);
              if (flen >= ZIP_FLEN) flen = ZIP_FLEN - 1;
              memcpy(bf[nb], start, flen);
              bf[nb][flen] = 0;
              nb++;
            }
            if (*p == '\n') p++;
          }
        }
        n = na > nb ? na : nb;
        sepn = strlen(sep);
        for (i = 0; i < n; i++) {
          const char *la = (i < na) ? af[i] : "";
          const char *lb = (i < nb) ? bf[i] : "";
          size_t fla = strlen(la), flb = strlen(lb);
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + fla < sizeof out) {
            memcpy(out + olen, la, fla);
            olen += fla;
          } else if (olen < sizeof out - 1) {
            size_t take = sizeof out - 1 - olen;
            memcpy(out + olen, la, take);
            olen += take;
          }
          if (olen + sepn < sizeof out) {
            memcpy(out + olen, sep, sepn);
            olen += sepn;
          }
          if (olen + flb < sizeof out) {
            memcpy(out + olen, lb, flb);
            olen += flb;
          } else if (olen < sizeof out - 1) {
            size_t take = sizeof out - 1 - olen;
            memcpy(out + olen, lb, take);
            olen += take;
          }
          out[olen] = 0;
          kept++;
        }
      } else {
        /* KEYS / VALS: peel each field at first sep */
        if (resolve_str_arg(vm, L, a, sizeof a) != 0)
          snprintf(a, sizeof a, "%s", vm->last_str);
        if (L->cur.kind == TK_STR) {
          if (resolve_str_arg(vm, L, sep, sizeof sep) != 0)
            { sep[0] = ':'; sep[1] = 0; }
          if (!sep[0]) { sep[0] = ':'; sep[1] = 0; }
        }
        sepn = strlen(sep);
        if (a[0]) {
          p = a;
          while (*p) {
            const char *hit = 0;
            size_t flen, take;
            start = p;
            while (*p && *p != '\n') p++;
            flen = (size_t)(p - start);
            /* find first sep in field */
            if (sepn == 0) {
              hit = start; /* empty sep: all left empty right? treat as no sep */
              hit = 0;
            } else if (flen >= sepn) {
              size_t k;
              for (k = 0; k + sepn <= flen; k++) {
                if (memcmp(start + k, sep, sepn) == 0) {
                  hit = start + k;
                  break;
                }
              }
            }
            if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (mode == 1) {
              /* keys: left of sep, or whole field if no sep */
              take = hit ? (size_t)(hit - start) : flen;
              if (olen + take < sizeof out) {
                memcpy(out + olen, start, take);
                olen += take;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, start, t);
                olen += t;
              }
            } else {
              /* vals: right of sep, or empty if no sep */
              if (hit) {
                const char *rs = hit + sepn;
                take = (size_t)((start + flen) - rs);
                if (olen + take < sizeof out) {
                  memcpy(out + olen, rs, take);
                  olen += take;
                } else if (olen < sizeof out - 1) {
                  size_t t = sizeof out - 1 - olen;
                  memcpy(out + olen, rs, t);
                  olen += t;
                }
              }
              /* else empty field */
            }
            out[olen] = 0;
            kept++;
            if (*p == '\n') p++;
          }
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "ZIP_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS PREFIXALL|PREPENDALL|MAPPRE bag prefix — prepend string to every field → LAST.
     * SYS SUFFIXALL|APPENDALL|MAPSUF bag suffix — append string to every field → LAST.
     * LAST_N = field count. Empty bag → "" OK=1.
     * Usability: tag LIST/peer bags (path/, .json) without EACH+CAT rebuild. */
    if (kw(&L->cur,"PREFIXALL") || kw(&L->cur,"PREPENDALL") || kw(&L->cur,"MAPPRE") ||
        kw(&L->cur,"PREALL") || kw(&L->cur,"ADDFRONT") || kw(&L->cur,"TAGPRE") ||
        kw(&L->cur,"SUFFIXALL") || kw(&L->cur,"APPENDALL") || kw(&L->cur,"MAPSUF") ||
        kw(&L->cur,"SUFALL") || kw(&L->cur,"ADDEND") || kw(&L->cur,"TAGSUF") ||
        kw(&L->cur,"POSTFIXALL")){
      char op[20];
      int is_suf;
      char bag[CUBALC_HOST_STR_MAX], affix[512], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t olen = 0, alen, flen;
      long kept = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      is_suf = (strcmp(op, "SUFFIXALL") == 0 || strcmp(op, "APPENDALL") == 0 ||
                strcmp(op, "MAPSUF") == 0 || strcmp(op, "SUFALL") == 0 ||
                strcmp(op, "ADDEND") == 0 || strcmp(op, "TAGSUF") == 0 ||
                strcmp(op, "POSTFIXALL") == 0);
      lex_next(L);
      bag[0] = 0; affix[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, affix, sizeof affix) != 0)
        affix[0] = 0;
      alen = strlen(affix);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (!is_suf) {
            /* prefix + field */
            if (olen + alen < sizeof out) {
              memcpy(out + olen, affix, alen);
              olen += alen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, affix, t);
              olen += t;
            }
            if (olen + flen < sizeof out) {
              memcpy(out + olen, start, flen);
              olen += flen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, start, t);
              olen += t;
            }
          } else {
            /* field + suffix */
            if (olen + flen < sizeof out) {
              memcpy(out + olen, start, flen);
              olen += flen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, start, t);
              olen += t;
            }
            if (olen + alen < sizeof out) {
              memcpy(out + olen, affix, alen);
              olen += alen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, affix, t);
              olen += t;
            }
          }
          out[olen] = 0;
          kept++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "PREFIXALL_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS FILL|REPEATL|REPEATS|BAGFILL n value — bag of n copies of value → LAST.
     * n capped 0..256; n<=0 → empty. Value may be str/ident/num (num stringified).
     * Usability: default status columns for ZIP / pad peer bags without EACH. */
    if (kw(&L->cur,"FILL") || kw(&L->cur,"REPEATL") || kw(&L->cur,"REPEATS") ||
        kw(&L->cur,"BAGFILL") || kw(&L->cur,"FILLBAG") || kw(&L->cur,"NFILL") ||
        kw(&L->cur,"TIMESLINE") || kw(&L->cur,"DUPFIELD")){
      long n = 0;
      int i;
      char val[512], out[CUBALC_HOST_STR_MAX], nbuf[40];
      size_t olen = 0, vlen;
      long kept = 0;
      lex_next(L);
      out[0] = 0; val[0] = 0;
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS) {
        n = parse_prim(vm, L);
      } else {
        n = vm->last_n;
      }
      if (n < 0) n = 0;
      if (n > 256) n = 256;
      if (L->cur.kind == TK_NUM) {
        snprintf(val, sizeof val, "%ld", L->cur.num);
        lex_next(L);
      } else if (L->cur.kind == TK_STR || L->cur.kind == TK_IDENT) {
        if (resolve_str_arg(vm, L, val, sizeof val) != 0)
          val[0] = 0;
      } else if (vm->last_str[0]) {
        snprintf(val, sizeof val, "%s", vm->last_str);
      } else {
        snprintf(val, sizeof val, "%ld", vm->last_n);
      }
      vlen = strlen(val);
      for (i = 0; i < (int)n; i++) {
        if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        if (olen + vlen < sizeof out) {
          memcpy(out + olen, val, vlen);
          olen += vlen;
        } else if (olen < sizeof out - 1) {
          size_t t = sizeof out - 1 - olen;
          memcpy(out + olen, val, t);
          olen += t;
        }
        out[olen] = 0;
        kept++;
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "FILL_N", kept);
      snprintf(nbuf, sizeof nbuf, "%ld", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS ENUMERATE|NUMBER|INDEXLINES|NL|ENUM bag [start] [sep]
     * — prefix each field with start+i and sep (default start=0, sep=":").
     * LAST_N = field count. Empty bag → "" OK=1.
     * Usability: ranked/priority plates without EACH+STR+CAT index glue. */
    /* Note: SYS ENUM is IOTA alias (RANGE plane) — do not claim ENUM here. */
    if (kw(&L->cur,"ENUMERATE") || kw(&L->cur,"NUMBER") || kw(&L->cur,"INDEXLINES") ||
        kw(&L->cur,"NUMBERLINES") || kw(&L->cur,"NLNUM") ||
        kw(&L->cur,"IDXLINES") || kw(&L->cur,"WITHINDEX") || kw(&L->cur,"RANKLINES")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], out[CUBALC_HOST_STR_MAX], ibuf[40];
      const char *p, *start;
      size_t olen = 0, sepn, flen, ilen;
      long kept = 0, base = 0, idx;
      lex_next(L);
      bag[0] = 0; sep[0] = ':'; sep[1] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      /* optional start index (num/ident/paren); then optional sep string */
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS) {
        /* Ambiguity: IDENT could be next form. Only treat as start if num/minus/paren
         * or IDENT that resolves as number var. Prefer: num or minus always start;
         * IDENT only if looks numeric after resolve as number via parse_prim when
         * next is also present or kind is clearly numeric. */
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
            L->cur.kind == TK_LPAREN) {
          base = parse_prim(vm, L);
        } else if (L->cur.kind == TK_IDENT) {
          /* peek: if IDENT is a known numeric-ish we still parse_prim (var num) */
          base = parse_prim(vm, L);
        }
      }
      if (L->cur.kind == TK_STR) {
        if (resolve_str_arg(vm, L, sep, sizeof sep) != 0)
          { sep[0] = ':'; sep[1] = 0; }
        if (!sep[0]) { sep[0] = ':'; sep[1] = 0; }
      }
      sepn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          idx = base + kept;
          snprintf(ibuf, sizeof ibuf, "%ld", idx);
          ilen = strlen(ibuf);
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + ilen < sizeof out) {
            memcpy(out + olen, ibuf, ilen);
            olen += ilen;
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            memcpy(out + olen, ibuf, t);
            olen += t;
          }
          if (olen + sepn < sizeof out) {
            memcpy(out + olen, sep, sepn);
            olen += sepn;
          }
          if (olen + flen < sizeof out) {
            memcpy(out + olen, start, flen);
            olen += flen;
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            memcpy(out + olen, start, t);
            olen += t;
          }
          out[olen] = 0;
          kept++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "ENUM_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SQUEEZE|COMPACT|DROPEMPTY [BLANK|NB] [bag]
     * — drop empty newline fields (and whitespace-only if BLANK/NB).
     * LAST_N = kept count. Usability: clean VALS/SPLIT/LIST bags without EACH. */
    if (kw(&L->cur,"SQUEEZE") || kw(&L->cur,"COMPACT") || kw(&L->cur,"DROPEMPTY") ||
        kw(&L->cur,"DROPBLANK") || kw(&L->cur,"TRIMLINES") || kw(&L->cur,"STRIPLINES") ||
        kw(&L->cur,"NOTEMPTYL") || kw(&L->cur,"KEEPEMPTY0")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t olen = 0, flen;
      long kept = 0;
      int drop_blank = 0;
      char op[20];
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "DROPBLANK") == 0 || strcmp(op, "TRIMLINES") == 0 ||
          strcmp(op, "STRIPLINES") == 0)
        drop_blank = 1;
      lex_next(L);
      if (kw(&L->cur,"BLANK") || kw(&L->cur,"NB") || kw(&L->cur,"WS") ||
          kw(&L->cur,"WHITESPACE") || kw(&L->cur,"TRIM") || kw(&L->cur,"NONBLANK")) {
        drop_blank = 1;
        lex_next(L);
      }
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (bag[0]) {
        p = bag;
        while (*p) {
          int drop = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen == 0) {
            drop = 1;
          } else if (drop_blank) {
            size_t i;
            drop = 1;
            for (i = 0; i < flen; i++) {
              char c = start[i];
              if (c != ' ' && c != '\t' && c != '\r') { drop = 0; break; }
            }
          }
          if (!drop) {
            if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + flen < sizeof out) {
              memcpy(out + olen, start, flen);
              olen += flen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, start, t);
              olen += t;
            }
            out[olen] = 0;
            kept++;
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "SQUEEZE_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS TRIMALL|MAPTRIM|STRIPALL [L|R|BOTH] [bag]
     * — trim leading/trailing whitespace on every newline field → LAST.
     * Default BOTH (LTRIM+RTRIM). LAST_N = field count (empty fields kept).
     * Usability: clean plate lines after READ/SPLIT before SQUEEZE/EQS. */
    if (kw(&L->cur,"TRIMALL") || kw(&L->cur,"MAPTRIM") || kw(&L->cur,"STRIPALL") ||
        kw(&L->cur,"WSALL") || kw(&L->cur,"LTRIMALL") || kw(&L->cur,"RTRIMALL")){
      char op[20];
      int do_l = 1, do_r = 1;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      const char *p, *start, *fe, *fs;
      size_t olen = 0, flen;
      long kept = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "LTRIMALL") == 0) { do_l = 1; do_r = 0; }
      else if (strcmp(op, "RTRIMALL") == 0) { do_l = 0; do_r = 1; }
      lex_next(L);
      if (kw(&L->cur,"L") || kw(&L->cur,"LEFT") || kw(&L->cur,"LTRIM") ||
          kw(&L->cur,"LEADING")) {
        do_l = 1; do_r = 0; lex_next(L);
      } else if (kw(&L->cur,"R") || kw(&L->cur,"RIGHT") || kw(&L->cur,"RTRIM") ||
                 kw(&L->cur,"TRAILING")) {
        do_l = 0; do_r = 1; lex_next(L);
      } else if (kw(&L->cur,"BOTH") || kw(&L->cur,"LR") || kw(&L->cur,"FULL")) {
        do_l = 1; do_r = 1; lex_next(L);
      }
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          fs = start;
          fe = p;
          if (do_l) {
            while (fs < fe && (*fs == ' ' || *fs == '\t' || *fs == '\r')) fs++;
          }
          if (do_r) {
            while (fe > fs && (fe[-1] == ' ' || fe[-1] == '\t' || fe[-1] == '\r'))
              fe--;
          }
          flen = (size_t)(fe - fs);
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + flen < sizeof out) {
            memcpy(out + olen, fs, flen);
            olen += flen;
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            memcpy(out + olen, fs, t);
            olen += t;
          }
          out[olen] = 0;
          kept++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "TRIMALL_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS UPPERALL|MAPUPPER|UCASEALL [bag] — ASCII upper every newline field → LAST.
     * SYS LOWERALL|MAPLOWER|LCASEALL [bag] — ASCII lower every field.
     * LAST_N = field count. Usability: normalize LIST/severity bags before HASLINE/set ops
     * without EACH LINE + UPPER/LOWER glue. */
    if (kw(&L->cur,"UPPERALL") || kw(&L->cur,"MAPUPPER") || kw(&L->cur,"UCASEALL") ||
        kw(&L->cur,"TOUPPERALL") || kw(&L->cur,"UPCASEALL") ||
        kw(&L->cur,"LOWERALL") || kw(&L->cur,"MAPLOWER") || kw(&L->cur,"LCASEALL") ||
        kw(&L->cur,"TOLOWERALL") || kw(&L->cur,"DOWNCASEALL")){
      char op[20];
      int to_upper = 1;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t olen = 0, flen, i;
      long kept = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "LOWERALL") == 0 || strcmp(op, "MAPLOWER") == 0 ||
          strcmp(op, "LCASEALL") == 0 || strcmp(op, "TOLOWERALL") == 0 ||
          strcmp(op, "DOWNCASEALL") == 0)
        to_upper = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + flen < sizeof out) {
            for (i = 0; i < flen; i++) {
              char c = start[i];
              if (to_upper) {
                if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
              } else {
                if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
              }
              out[olen++] = c;
            }
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            for (i = 0; i < t; i++) {
              char c = start[i];
              if (to_upper) {
                if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
              } else {
                if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
              }
              out[olen++] = c;
            }
          }
          out[olen] = 0;
          kept++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "UPPERALL_N", kept);
      var_set_num(vm, "LOWERALL_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MAPREPLACE|GSUBALL|REPLACEBAG bag old new — REPLACEALL on every field.
     * Empty old → no-op (bag unchanged). LAST = bag; LAST_N/MAPREPLACE_N = total
     * replacements; MAPREPLACE_FIELDS = field count.
     * Usability: rewrite path prefixes / tags on LIST bags without EACH+REPLACEALL. */
    if (kw(&L->cur,"MAPREPLACE") || kw(&L->cur,"GSUBALL") || kw(&L->cur,"REPLACEBAG") ||
        kw(&L->cur,"REPLACEALLBAG") || kw(&L->cur,"BAGREPLACE") || kw(&L->cur,"SUBALLBAG") ||
        kw(&L->cur,"MAPGSUB") || kw(&L->cur,"MAPSUBST")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      char olds[256], news[256];
      const char *p, *start, *src;
      size_t olen = 0, flen, oldn, newn, pre, rest, take;
      long fields = 0, did = 0;
      char field[512], fbuf[768];
      size_t fo;
      lex_next(L);
      bag[0] = 0; out[0] = 0; olds[0] = 0; news[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, olds, sizeof olds) != 0) olds[0] = 0;
      if (resolve_str_arg(vm, L, news, sizeof news) != 0) news[0] = 0;
      oldn = strlen(olds);
      newn = strlen(news);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          take = flen;
          if (take >= sizeof field) take = sizeof field - 1;
          memcpy(field, start, take);
          field[take] = 0;
          /* apply REPLACEALL on this field */
          fo = 0;
          fbuf[0] = 0;
          if (oldn == 0) {
            if (take < sizeof fbuf) {
              memcpy(fbuf, field, take);
              fo = take;
              fbuf[fo] = 0;
            }
          } else {
            src = field;
            for (;;) {
              const char *hit = strstr(src, olds);
              if (!hit) {
                rest = strlen(src);
                if (fo + rest >= sizeof fbuf) rest = sizeof fbuf - 1 - fo;
                memcpy(fbuf + fo, src, rest);
                fo += rest;
                fbuf[fo] = 0;
                break;
              }
              pre = (size_t)(hit - src);
              if (fo + pre >= sizeof fbuf) pre = sizeof fbuf - 1 - fo;
              memcpy(fbuf + fo, src, pre);
              fo += pre;
              if (fo + newn < sizeof fbuf) {
                memcpy(fbuf + fo, news, newn);
                fo += newn;
              } else if (fo < sizeof fbuf - 1) {
                size_t nt = sizeof fbuf - 1 - fo;
                memcpy(fbuf + fo, news, nt);
                fo += nt;
              }
              fbuf[fo] = 0;
              did++;
              src = hit + oldn;
              if (fo >= sizeof fbuf - 1) break;
            }
          }
          if (fields > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          {
            size_t n = fo;
            if (olen + n < sizeof out) {
              memcpy(out + olen, fbuf, n);
              olen += n;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, fbuf, t);
              olen += t;
            }
            out[olen] = 0;
          }
          fields++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = did;
      var_set_num(vm, "LAST_N", did);
      var_set_num(vm, "MAPREPLACE_N", did);
      var_set_num(vm, "MAPREPLACE_FIELDS", fields);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS FREQ|HIST|COUNTS|TALLY [bag] [sep] — frequency of each exact field.
     * LAST = "key{sep}count" bag (first-seen order). Default sep ":".
     * LAST_N/FREQ_N = unique keys; FREQ_TOTAL = input field count (cap 64 uniques).
     * Usability: status/severity rollups without EACH LINE + COUNTLINE rebuild. */
    if (kw(&L->cur,"FREQ") || kw(&L->cur,"HIST") || kw(&L->cur,"COUNTS") ||
        kw(&L->cur,"FREQUENCY") || kw(&L->cur,"HISTOGRAM") ||
        kw(&L->cur,"COUNTBAG") || kw(&L->cur,"BAGCOUNT") || kw(&L->cur,"FREQBAG")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], sep[16];
      char keys[64][128];
      long counts[64];
      int nk = 0, k;
      const char *p, *start;
      size_t flen, olen = 0;
      long total = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      /* optional sep if next is a short string (not a form keyword start) */
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      memset(counts, 0, sizeof counts);
      if (bag[0]) {
        p = bag;
        while (*p) {
          char field[128];
          size_t take;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          take = flen;
          if (take >= sizeof field) take = sizeof field - 1;
          memcpy(field, start, take);
          field[take] = 0;
          total++;
          for (k = 0; k < nk; k++) {
            if (strcmp(keys[k], field) == 0) {
              counts[k]++;
              break;
            }
          }
          if (k == nk && nk < 64) {
            snprintf(keys[nk], sizeof keys[0], "%s", field);
            counts[nk] = 1;
            nk++;
          } else if (k == nk) {
            /* overflow uniques: drop further new keys but still count total */
          }
          if (*p == '\n') p++;
        }
      }
      for (k = 0; k < nk; k++) {
        char line[160];
        int n;
        n = snprintf(line, sizeof line, "%s%s%ld", keys[k], sep, counts[k]);
        if (n < 0) n = 0;
        if ((size_t)n >= sizeof line) n = (int)sizeof line - 1;
        if (k > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        if (olen + (size_t)n < sizeof out) {
          memcpy(out + olen, line, (size_t)n);
          olen += (size_t)n;
        } else if (olen < sizeof out - 1) {
          size_t t = sizeof out - 1 - olen;
          memcpy(out + olen, line, t);
          olen += t;
        }
        out[olen] = 0;
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "FREQ", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = nk;
      var_set_num(vm, "LAST_N", nk);
      var_set_num(vm, "FREQ_N", nk);
      var_set_num(vm, "FREQ_TOTAL", total);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SORTFREQ|SORTBYCOUNT|FSORT [bag] [sep] [DESC|ASC]
     * — sort "key{sep}count" FREQ lines by numeric count (default sep ":", DESC).
     * Count is digits after last sep on each line. Stable for equal counts.
     * LAST_N = line count. Usability: top severities after FREQ without shell sort -n. */
    if (kw(&L->cur,"SORTFREQ") || kw(&L->cur,"SORTBYCOUNT") || kw(&L->cur,"FSORT") ||
        kw(&L->cur,"FREQSORT") || kw(&L->cur,"SORTCOUNTS") || kw(&L->cur,"RANKFREQ") ||
        kw(&L->cur,"TOPFREQ") || kw(&L->cur,"HEAVYFIRST")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], sep[16];
      char lines[64][160];
      long counts[64];
      int order[64];
      int n = 0, i, j, desc = 1;
      const char *p, *start;
      size_t sepn, olen = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      /* optional sep (string) and/or DESC|ASC */
      for (;;) {
        if (L->cur.kind == TK_STR) {
          snprintf(sep, sizeof sep, "%s", L->cur.text);
          if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
          lex_next(L);
          continue;
        }
        if (kw(&L->cur,"DESC") || kw(&L->cur,"DOWN") || kw(&L->cur,"REV") ||
            kw(&L->cur,"HEAVY") || kw(&L->cur,"HIGH")) {
          desc = 1; lex_next(L); continue;
        }
        if (kw(&L->cur,"ASC") || kw(&L->cur,"UP") || kw(&L->cur,"LIGHT") ||
            kw(&L->cur,"LOW") || kw(&L->cur,"ASCENDING")) {
          desc = 0; lex_next(L); continue;
        }
        break;
      }
      sepn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p && n < 64) {
          size_t flen, take;
          const char *last;
          char *endc;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          take = flen;
          if (take >= sizeof lines[0]) take = sizeof lines[0] - 1;
          memcpy(lines[n], start, take);
          lines[n][take] = 0;
          /* count = number after last sep */
          counts[n] = 0;
          if (sepn == 0) {
            counts[n] = strtol(lines[n], &endc, 10);
          } else {
            last = NULL;
            {
              const char *q = lines[n];
              while (*q) {
                if (strncmp(q, sep, sepn) == 0) last = q;
                q++;
              }
            }
            if (last) counts[n] = strtol(last + sepn, &endc, 10);
            else counts[n] = strtol(lines[n], &endc, 10);
          }
          order[n] = n;
          n++;
          if (*p == '\n') p++;
        }
      }
      /* stable insertion sort by count */
      for (i = 1; i < n; i++) {
        int oi = order[i];
        long ci = counts[oi];
        j = i - 1;
        while (j >= 0) {
          long cj = counts[order[j]];
          int swap = desc ? (cj < ci) : (cj > ci);
          if (!swap) break;
          order[j + 1] = order[j];
          j--;
        }
        order[j + 1] = oi;
      }
      for (i = 0; i < n; i++) {
        size_t ln = strlen(lines[order[i]]);
        if (i > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
        if (olen + ln < sizeof out) {
          memcpy(out + olen, lines[order[i]], ln);
          olen += ln;
        } else if (olen < sizeof out - 1) {
          size_t t = sizeof out - 1 - olen;
          memcpy(out + olen, lines[order[i]], t);
          olen += t;
        }
        out[olen] = 0;
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "FREQ", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = n;
      var_set_num(vm, "LAST_N", n);
      var_set_num(vm, "SORTFREQ_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS BEFOREALL|MAPBEFORE bag needle — BEFORE on every bag field → LAST bag.
     * SYS AFTERALL|MAPAFTER bag needle — AFTER on every field.
     * Same miss semantics as BEFORE/AFTER (before→whole field, after→empty).
     * LAST_N = field count; BEFOREALL_HIT = how many fields contained needle.
     * Usability: FREQ key/count peel or kv bags without EACH+BEFORE/AFTER. */
    if (kw(&L->cur,"BEFOREALL") || kw(&L->cur,"MAPBEFORE") || kw(&L->cur,"LEFTALL") ||
        kw(&L->cur,"PREALL") || kw(&L->cur,"BAGBEFORE") ||
        kw(&L->cur,"AFTERALL") || kw(&L->cur,"MAPAFTER") || kw(&L->cur,"RIGHTALL") ||
        kw(&L->cur,"POSTALL") || kw(&L->cur,"BAGAFTER")){
      char op[20];
      int want_after = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], needle[256];
      const char *p, *start;
      size_t olen = 0, flen, nn, take;
      long lines = 0, hits = 0;
      char field[512], cell[512];
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "AFTERALL") == 0 || strcmp(op, "MAPAFTER") == 0 ||
          strcmp(op, "RIGHTALL") == 0 || strcmp(op, "POSTALL") == 0 ||
          strcmp(op, "BAGAFTER") == 0)
        want_after = 1;
      lex_next(L);
      bag[0] = 0; out[0] = 0; needle[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0] = 0;
      nn = strlen(needle);
      if (bag[0]) {
        p = bag;
        while (*p) {
          const char *hit;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          take = flen;
          if (take >= sizeof field) take = sizeof field - 1;
          memcpy(field, start, take);
          field[take] = 0;
          cell[0] = 0;
          if (nn == 0) {
            hits++;
            if (want_after) snprintf(cell, sizeof cell, "%s", field);
            else cell[0] = 0;
          } else {
            hit = strstr(field, needle);
            if (!hit) {
              if (want_after) cell[0] = 0;
              else snprintf(cell, sizeof cell, "%s", field);
            } else {
              hits++;
              if (want_after) {
                snprintf(cell, sizeof cell, "%s", hit + nn);
              } else {
                size_t pre = (size_t)(hit - field);
                if (pre >= sizeof cell) pre = sizeof cell - 1;
                memcpy(cell, field, pre);
                cell[pre] = 0;
              }
            }
          }
          if (lines > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          {
            size_t n = strlen(cell);
            if (olen + n < sizeof out) {
              memcpy(out + olen, cell, n);
              olen += n;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, cell, t);
              olen += t;
            }
            out[olen] = 0;
          }
          lines++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = lines;
      var_set_num(vm, "LAST_N", lines);
      var_set_num(vm, "BEFOREALL_N", lines);
      var_set_num(vm, "AFTERALL_N", lines);
      var_set_num(vm, "BEFOREALL_HIT", hits);
      var_set_num(vm, "AFTERALL_HIT", hits);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS FIRSTMATCH|GREP1|FINDFIELD [I] bag needle — first field containing needle.
     * SYS FIRSTMATCHI — case-insensitive. LAST = field (empty if miss); LAST_N 0|1.
     * FIRSTMATCH_I = 0-based index of hit (-1 miss). Empty needle → first field.
     * Usability: pick one log/peer line without GREP+TAKE 1 glue. */
    if (kw(&L->cur,"FIRSTMATCH") || kw(&L->cur,"GREP1") || kw(&L->cur,"FINDFIELD") ||
        kw(&L->cur,"FIRSTGREP") || kw(&L->cur,"MATCH1") || kw(&L->cur,"FINDLINE_SUB") ||
        kw(&L->cur,"FIRSTMATCHI") || kw(&L->cur,"GREP1I") || kw(&L->cur,"FINDFIELDI")){
      char op[20];
      int icase = 0;
      char bag[CUBALC_HOST_STR_MAX], needle[256], out[512];
      const char *p, *start;
      size_t flen, nn;
      long idx = 0, found = 0, found_i = -1;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "FIRSTMATCHI") == 0 || strcmp(op, "GREP1I") == 0 ||
          strcmp(op, "FINDFIELDI") == 0)
        icase = 1;
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
      if (bag[0]) {
        p = bag;
        while (*p) {
          int hit = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            char field[512];
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            if (nn == 0) {
              hit = 1;
            } else if (!icase) {
              if (strstr(field, needle) != NULL) hit = 1;
            } else {
              size_t fi, j;
              for (fi = 0; field[fi] && !hit; fi++) {
                for (j = 0; j < nn; j++) {
                  char a = field[fi + j], b = needle[j];
                  if (!a) break;
                  if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                  if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                  if (a != b) break;
                }
                if (j == nn) hit = 1;
              }
            }
            if (hit) {
              if (flen >= sizeof out) flen = sizeof out - 1;
              memcpy(out, start, flen);
              out[flen] = 0;
              found = 1;
              found_i = idx;
              break;
            }
          }
          idx++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "GREP", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      var_set_num(vm, "FIRSTMATCH_N", found);
      var_set_num(vm, "FIRSTMATCH_I", found_i);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS LOOKUP|KVGET|GETKV [I] bag key [sep] [OR|DEFAULT fallback]
     * — find first bag field whose left side (before sep, default ":") equals key;
     * LAST = right side (value); LOOKUP_LINE = full field; LAST_N 0|1 hit;
     * LOOKUP_I = index (-1 miss). Empty sep → exact field equality → LAST=field.
     * LOOKUPI / KVGETI — case-insensitive key match.
     * OR|DEFAULT|ELSE|FALLBACK: on miss LAST=fallback, OK=1, LOOKUP_N=0, LOOKUP_OR=1
     * (hit ignores trailing OR — same as SYS ENV/READ). Soft miss without empty-value
     * ambiguity for FREQ counts: SYS LOOKUP hist "FATAL" OR "0".
     * Usability: FREQ/plate kv defaults without LOOKUP+IF EMPTY glue. */
    if (kw(&L->cur,"LOOKUP") || kw(&L->cur,"KVGET") || kw(&L->cur,"GETKV") ||
        kw(&L->cur,"DICTGET") || kw(&L->cur,"MAPGET") || kw(&L->cur,"BAGGET") ||
        kw(&L->cur,"LOOKUPI") || kw(&L->cur,"KVGETI") || kw(&L->cur,"GETKVI") ||
        kw(&L->cur,"DICTGETI") || kw(&L->cur,"MAPGETI")){
      char op[20];
      int icase = 0;
      char bag[CUBALC_HOST_STR_MAX], key[256], sep[32], out[512], line[512];
      char fb[512];
      int have_fb = 0, used_or = 0;
      const char *p, *start;
      size_t flen, kn, sn, i;
      long idx = 0, found = 0, found_i = -1;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "LOOKUPI") == 0 || strcmp(op, "KVGETI") == 0 ||
          strcmp(op, "GETKVI") == 0 || strcmp(op, "DICTGETI") == 0 ||
          strcmp(op, "MAPGETI") == 0)
        icase = 1;
      lex_next(L);
      if (!icase && (kw(&L->cur,"I") || kw(&L->cur,"ICASE") ||
                     kw(&L->cur,"IGNORECASE") || kw(&L->cur,"-I") ||
                     kw(&L->cur,"CI"))){
        icase = 1;
        lex_next(L);
      }
      bag[0] = 0; key[0] = 0; out[0] = 0; line[0] = 0; fb[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, key, sizeof key) != 0) key[0] = 0;
      /* optional sep (string or bare token) — never OR/DEFAULT/… */
      if (L->cur.kind == TK_STR) {
        /* string is sep only when not following OR keyword (handled below) */
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT &&
                 (strcmp(L->cur.text, "OR") != 0 &&
                  strcmp(L->cur.text, "SOFT") != 0 &&
                  strcmp(L->cur.text, "DEFAULT") != 0 &&
                  strcmp(L->cur.text, "ELSE") != 0 &&
                  strcmp(L->cur.text, "FALLBACK") != 0)) {
        /* allow bare = or : only if single-char-ish */
        if (strlen(L->cur.text) <= 2) {
          snprintf(sep, sizeof sep, "%s", L->cur.text);
          lex_next(L);
        }
      }
      /* optional OR|DEFAULT fallback (like SYS ENV / ARG / READ) */
      if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
          kw(&L->cur,"FALLBACK") || kw(&L->cur,"SOFT")){
        lex_next(L);
        if (resolve_str_arg(vm, L, fb, sizeof fb) != 0) {
          fail(vm, "SYS LOOKUP bag key [sep] OR \"fallback\"");
          return -1;
        }
        have_fb = 1;
      }
      kn = strlen(key);
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          int hit = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            char field[512];
            char left[256];
            size_t take = flen, left_n = 0;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            if (sn == 0) {
              /* exact field match */
              if (!icase) {
                hit = (strcmp(field, key) == 0);
              } else {
                size_t a = strlen(field), j;
                if (a == kn) {
                  hit = 1;
                  for (j = 0; j < kn; j++) {
                    char ca = field[j], cb = key[j];
                    if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
                    if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
                    if (ca != cb) { hit = 0; break; }
                  }
                }
              }
              if (hit) {
                snprintf(out, sizeof out, "%s", field);
              }
            } else {
              /* key is left of first sep */
              const char *sp = strstr(field, sep);
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
              } else {
                left_n = take;
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
              }
              if (!icase) {
                hit = (strcmp(left, key) == 0);
              } else {
                size_t j;
                if (left_n == kn) {
                  hit = 1;
                  for (j = 0; j < kn; j++) {
                    char ca = left[j], cb = key[j];
                    if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
                    if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
                    if (ca != cb) { hit = 0; break; }
                  }
                }
              }
              if (hit) {
                if (sp) {
                  snprintf(out, sizeof out, "%s", sp + sn);
                } else {
                  out[0] = 0; /* key-only line, empty value */
                }
              }
            }
            if (hit) {
              if (flen >= sizeof line) flen = sizeof line - 1;
              memcpy(line, start, flen);
              line[flen] = 0;
              found = 1;
              found_i = idx;
              break;
            }
          }
          idx++;
          if (*p == '\n') p++;
        }
      }
      (void)i;
      if (!found && have_fb) {
        snprintf(out, sizeof out, "%s", fb);
        used_or = 1;
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "LOOKUP", out);
      var_set_str(vm, "LOOKUP_LINE", line);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      var_set_num(vm, "LOOKUP_N", found);
      var_set_num(vm, "LOOKUP_I", found_i);
      var_set_num(vm, "LOOKUP_OR", used_or);
      var_set_num(vm, "KVGET_N", found);
      var_set_num(vm, "KVGET_OR", used_or);
      /* hit → OK=1; miss+OR → OK=1 (soft default); bare miss → OK=0 */
      var_set_num(vm, "OK", (found || used_or) ? 1 : 0);
      bump(vm); return 1;
    }
    /* SYS LOOKUPN|KVGETN|GETKVN bag key [sep] [OR default]
     * — like LOOKUP but LAST_N = integer parse of the value (not 0|1 hit).
     * LAST still holds value string. On miss+OR: LAST_N = default int, OK=1,
     * LOOKUPN_OR=1. Bare miss: LAST_N=0, OK=0.
     * Usability: FREQ count arithmetic without LOOKUP+SYS NUM glue. */
    if (kw(&L->cur,"LOOKUPN") || kw(&L->cur,"KVGETN") || kw(&L->cur,"GETKVN") ||
        kw(&L->cur,"DICTGETN") || kw(&L->cur,"MAPGETN") || kw(&L->cur,"NUMKV") ||
        kw(&L->cur,"KVNUM") || kw(&L->cur,"ICOUNT") || kw(&L->cur,"NLOOKUP")){
      char bag[CUBALC_HOST_STR_MAX], key[256], sep[32], out[512], line[512];
      char fb[64];
      int have_fb = 0, used_or = 0;
      const char *p, *start;
      size_t flen, kn, sn;
      long idx = 0, found = 0, found_i = -1, nval = 0, defv = 0;
      lex_next(L);
      bag[0] = 0; key[0] = 0; out[0] = 0; line[0] = 0; fb[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, key, sizeof key) != 0) key[0] = 0;
      /* optional sep string (not OR keywords) */
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        lex_next(L);
      } else if (L->cur.kind == TK_IDENT &&
                 strcmp(L->cur.text, "OR") != 0 &&
                 strcmp(L->cur.text, "DEFAULT") != 0 &&
                 strcmp(L->cur.text, "ELSE") != 0 &&
                 strcmp(L->cur.text, "FALLBACK") != 0 &&
                 strcmp(L->cur.text, "SOFT") != 0) {
        if (strlen(L->cur.text) <= 2) {
          snprintf(sep, sizeof sep, "%s", L->cur.text);
          lex_next(L);
        }
      }
      if (kw(&L->cur,"OR") || kw(&L->cur,"DEFAULT") || kw(&L->cur,"ELSE") ||
          kw(&L->cur,"FALLBACK") || kw(&L->cur,"SOFT")){
        lex_next(L);
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
            L->cur.kind == TK_LPAREN) {
          defv = parse_expr(vm, L);
          snprintf(fb, sizeof fb, "%ld", defv);
          have_fb = 1;
        } else {
          if (resolve_str_arg(vm, L, fb, sizeof fb) != 0) {
            fail(vm, "SYS LOOKUPN bag key [sep] OR default");
            return -1;
          }
          {
            char *end = 0;
            defv = strtol(fb, &end, 10);
            if (end == fb) defv = 0;
          }
          have_fb = 1;
        }
      }
      kn = strlen(key);
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          int hit = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            char field[512];
            char left[256];
            size_t take = flen, left_n = 0;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            if (sn == 0) {
              hit = (strcmp(field, key) == 0);
              if (hit) snprintf(out, sizeof out, "%s", field);
            } else {
              const char *sp = strstr(field, sep);
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
              } else {
                left_n = take;
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
              }
              hit = (strcmp(left, key) == 0);
              if (hit) {
                if (sp) snprintf(out, sizeof out, "%s", sp + sn);
                else out[0] = 0;
              }
            }
            if (hit) {
              if (flen >= sizeof line) flen = sizeof line - 1;
              memcpy(line, start, flen);
              line[flen] = 0;
              found = 1;
              found_i = idx;
              break;
            }
          }
          idx++;
          if (*p == '\n') p++;
        }
      }
      (void)kn;
      if (found) {
        char *end = 0;
        nval = strtol(out, &end, 10);
        if (end == out) nval = 0;
      } else if (have_fb) {
        snprintf(out, sizeof out, "%s", fb);
        nval = defv;
        used_or = 1;
      } else {
        out[0] = 0;
        nval = 0;
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "LOOKUPN", out);
      var_set_str(vm, "LOOKUP_LINE", line);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = nval;
      var_set_num(vm, "LAST_N", nval);
      var_set_num(vm, "LOOKUPN_N", found);
      var_set_num(vm, "LOOKUPN_I", found_i);
      var_set_num(vm, "LOOKUPN_OR", used_or);
      var_set_num(vm, "LOOKUPN_V", nval);
      var_set_num(vm, "KVGETN_V", nval);
      var_set_num(vm, "OK", (found || used_or) ? 1 : 0);
      bump(vm); return 1;
    }
    /* SYS KVSET|SETKV|DICTSET bag key value [sep]
     * — set key:value in bag: if a field's left-of-sep equals key, replace value;
     * else append key+sep+value. Default sep ":". LAST = bag; LAST_N = field count;
     * KVSET_I = index of set field; KVSET_HIT 1=replaced 0=appended.
     * Usability: update FREQ/plate kv without FINDLINE+SETLINE glue; dual of LOOKUP. */
    if (kw(&L->cur,"KVSET") || kw(&L->cur,"SETKV") || kw(&L->cur,"DICTSET") ||
        kw(&L->cur,"MAPSET") || kw(&L->cur,"BAGSET") || kw(&L->cur,"PUTKV") ||
        kw(&L->cur,"KVPUT") || kw(&L->cur,"SETKEY")){
      char bag[CUBALC_HOST_STR_MAX], key[256], val[512], sep[32], out[CUBALC_HOST_STR_MAX];
      char field[512], left[256], newline[768];
      const char *p, *start;
      size_t flen, kn, sn, vn, olen = 0, left_n;
      long idx = 0, kept = 0, hit = 0, found_i = -1;
      lex_next(L);
      bag[0] = 0; key[0] = 0; val[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, key, sizeof key) != 0) key[0] = 0;
      if (resolve_str_arg(vm, L, val, sizeof val) != 0) val[0] = 0;
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      kn = strlen(key);
      sn = strlen(sep);
      vn = strlen(val);
      if (bag[0]) {
        p = bag;
        while (*p) {
          int match = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            if (sn == 0) {
              match = (strcmp(field, key) == 0);
            } else {
              const char *sp = strstr(field, sep);
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
              } else {
                left_n = take;
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
              }
              match = (strcmp(left, key) == 0);
            }
            if (match && found_i < 0) {
              /* replace first match */
              if (sn == 0)
                snprintf(newline, sizeof newline, "%s", val);
              else
                snprintf(newline, sizeof newline, "%s%s%s", key, sep, val);
              found_i = idx;
              hit = 1;
            } else {
              snprintf(newline, sizeof newline, "%s", field);
            }
          }
          {
            size_t nlen = strlen(newline);
            if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + nlen < sizeof out) {
              memcpy(out + olen, newline, nlen);
              olen += nlen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, newline, t);
              olen += t;
            }
            out[olen] = 0;
          }
          kept++;
          idx++;
          if (*p == '\n') p++;
        }
      }
      if (!hit) {
        /* append new key:value */
        if (sn == 0)
          snprintf(newline, sizeof newline, "%s", key[0] ? key : val);
        else
          snprintf(newline, sizeof newline, "%s%s%s", key, sep, val);
        {
          size_t nlen = strlen(newline);
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + nlen < sizeof out) {
            memcpy(out + olen, newline, nlen);
            olen += nlen;
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            memcpy(out + olen, newline, t);
            olen += t;
          }
          out[olen] = 0;
        }
        found_i = kept;
        kept++;
      }
      (void)kn; (void)vn;
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "KVSET", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "KVSET_N", kept);
      var_set_num(vm, "KVSET_I", found_i);
      var_set_num(vm, "KVSET_HIT", hit);
      var_set_num(vm, "SETKV_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS KVINC|INCKV|BUMPKV bag key [delta] [sep]
     * — add delta (default 1) to numeric value of key:val field; if missing, insert
     * key:delta. Default sep ":". LAST = bag; LAST_N = field count;
     * KVINC_V = new value; KVINC_I = index; KVINC_HIT 1=existed 0=inserted.
     * Usability: bump FREQ/severity counters without LOOKUP+arith+KVSET glue. */
    if (kw(&L->cur,"KVINC") || kw(&L->cur,"INCKV") || kw(&L->cur,"BUMPKV") ||
        kw(&L->cur,"KVADD") || kw(&L->cur,"INCKEY") ||
        kw(&L->cur,"COUNTUP") || kw(&L->cur,"KVBUMP")){
      char bag[CUBALC_HOST_STR_MAX], key[256], sep[32], out[CUBALC_HOST_STR_MAX];
      char field[512], left[256], newline[768], vbuf[40];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long idx = 0, kept = 0, hit = 0, found_i = -1, delta = 1, newv = 0;
      lex_next(L);
      bag[0] = 0; key[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, key, sizeof key) != 0) key[0] = 0;
      /* optional delta (number) then optional sep string */
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        delta = parse_expr(vm, L);
      } else if (L->cur.kind == TK_IDENT) {
        /* allow numeric var as delta when next is not a form keyword we care about */
        Var *dv = var_get(vm, L->cur.text, 0);
        if (dv && !dv->is_str) {
          delta = (long)dv->val;
          lex_next(L);
        }
      }
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          int match = 0;
          long oldv = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *sp = sn ? strstr(field, sep) : NULL;
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
                match = (strcmp(left, key) == 0);
                if (match) {
                  char *end = 0;
                  oldv = strtol(sp + sn, &end, 10);
                }
              } else if (sn == 0) {
                match = (strcmp(field, key) == 0);
              } else {
                match = (strcmp(field, key) == 0);
                if (match) oldv = 0;
              }
            }
            if (match && found_i < 0) {
              newv = oldv + delta;
              snprintf(vbuf, sizeof vbuf, "%ld", newv);
              if (sn == 0)
                snprintf(newline, sizeof newline, "%s", vbuf);
              else
                snprintf(newline, sizeof newline, "%s%s%s", key, sep, vbuf);
              found_i = idx;
              hit = 1;
            } else {
              snprintf(newline, sizeof newline, "%s", field);
            }
          }
          {
            size_t nlen = strlen(newline);
            if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + nlen < sizeof out) {
              memcpy(out + olen, newline, nlen);
              olen += nlen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, newline, t);
              olen += t;
            }
            out[olen] = 0;
          }
          kept++;
          idx++;
          if (*p == '\n') p++;
        }
      }
      if (!hit) {
        newv = delta;
        snprintf(vbuf, sizeof vbuf, "%ld", newv);
        if (sn == 0)
          snprintf(newline, sizeof newline, "%s", key[0] ? key : vbuf);
        else
          snprintf(newline, sizeof newline, "%s%s%s", key, sep, vbuf);
        {
          size_t nlen = strlen(newline);
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + nlen < sizeof out) {
            memcpy(out + olen, newline, nlen);
            olen += nlen;
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            memcpy(out + olen, newline, t);
            olen += t;
          }
          out[olen] = 0;
        }
        found_i = kept;
        kept++;
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "KVINC", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "KVINC_N", kept);
      var_set_num(vm, "KVINC_I", found_i);
      var_set_num(vm, "KVINC_HIT", hit);
      var_set_num(vm, "KVINC_V", newv);
      var_set_num(vm, "INCKV_V", newv);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS KVDEL|DELKV|RMKV bag key [sep]
     * — drop first bag field whose left-of-sep equals key. Default sep ":".
     * LAST = bag (unchanged on miss); LAST_N = remaining field count;
     * KVDEL_HIT 1=removed 0=miss; KVDEL_I = removed index (-1 miss);
     * KVDEL_V = removed value (empty on miss). Soft: miss does not fail OK.
     * Usability: ack/remove FREQ key after handling without FINDLINE+DROPNTH. */
    if (kw(&L->cur,"KVDEL") || kw(&L->cur,"DELKV") || kw(&L->cur,"RMKV") ||
        kw(&L->cur,"DELKEY") || kw(&L->cur,"UNSETKV") || kw(&L->cur,"KVUNSET") ||
        kw(&L->cur,"KVDROP") || kw(&L->cur,"REMOVEKV")){
      char bag[CUBALC_HOST_STR_MAX], key[256], sep[32], out[CUBALC_HOST_STR_MAX];
      char field[512], left[256], dropped_val[512];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long idx = 0, kept = 0, hit = 0, found_i = -1;
      lex_next(L);
      bag[0] = 0; key[0] = 0; out[0] = 0; dropped_val[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, key, sizeof key) != 0) key[0] = 0;
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          int match = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *sp = sn ? strstr(field, sep) : NULL;
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
                match = (strcmp(left, key) == 0);
                if (match && found_i < 0) {
                  size_t vn = strlen(sp + sn);
                  if (vn >= sizeof dropped_val) vn = sizeof dropped_val - 1;
                  memcpy(dropped_val, sp + sn, vn);
                  dropped_val[vn] = 0;
                }
              } else if (sn == 0) {
                match = (strcmp(field, key) == 0);
                if (match && found_i < 0) {
                  snprintf(dropped_val, sizeof dropped_val, "%s", field);
                }
              } else {
                match = (strcmp(field, key) == 0);
                if (match && found_i < 0) dropped_val[0] = 0;
              }
            }
            if (match && found_i < 0) {
              /* skip first match (delete) */
              found_i = idx;
              hit = 1;
            } else {
              size_t nlen = strlen(field);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, field, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, field, t);
                olen += t;
              }
              out[olen] = 0;
              kept++;
            }
          }
          idx++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "KVDEL", out);
      var_set_str(vm, "KVDEL_V", dropped_val);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "KVDEL_N", kept);
      var_set_num(vm, "KVDEL_I", found_i);
      var_set_num(vm, "KVDEL_HIT", hit);
      var_set_num(vm, "DELKV_N", kept);
      var_set_num(vm, "DELKV_HIT", hit);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MERGEKV|KVADDALL|ADDFREQ bag_a bag_b [sep]
     * — merge two key:val bags by summing numeric values for shared keys;
     * keys only in bag_b are appended. Default sep ":". LAST = merged bag;
     * LAST_N = field count; MERGEKV_N = same; MERGEKV_ADDED = keys appended
     * from bag_b; MERGEKV_HIT = keys that already existed in bag_a and got
     * their values summed.
     * Usability: combine FREQ histograms from multiple logs without EACH+KVINC. */
    if (kw(&L->cur,"MERGEKV") || kw(&L->cur,"KVADDALL") || kw(&L->cur,"ADDFREQ") ||
        kw(&L->cur,"KVUNION") || kw(&L->cur,"MERGEHIST") || kw(&L->cur,"SUMFREQ") ||
        kw(&L->cur,"COMBINEKV") || kw(&L->cur,"KVCOMBINE") || kw(&L->cur,"FUSEKV")){
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX], sep[32];
      char out[CUBALC_HOST_STR_MAX], field[512], left[256], newline[768], vbuf[40];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long kept = 0, added = 0, hit_n = 0;
      lex_next(L);
      a[0] = 0; b[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, a, sizeof a) != 0)
        snprintf(a, sizeof a, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, b, sizeof b) != 0) b[0] = 0;
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      /* seed out from bag_a */
      if (a[0]) {
        p = a;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              size_t nlen = strlen(field);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, field, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, field, t);
                olen += t;
              }
              out[olen] = 0;
            }
            kept++;
          }
          if (*p == '\n') p++;
        }
      }
      /* fold bag_b keys into out (KVINC-style) */
      if (b[0]) {
        p = b;
        while (*p) {
          long delta = 0, found_i = -1, hit = 0;
          char key[256];
          char tmp[CUBALC_HOST_STR_MAX];
          size_t tolen = 0;
          long tkept = 0, idx = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            size_t take = flen;
            const char *sp;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            key[0] = 0;
            if (sn == 0) {
              snprintf(key, sizeof key, "%s", field);
              delta = 1;
            } else {
              sp = strstr(field, sep);
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof key) left_n = sizeof key - 1;
                memcpy(key, field, left_n);
                key[left_n] = 0;
                {
                  char *end = 0;
                  delta = strtol(sp + sn, &end, 10);
                  if (end == sp + sn) delta = 0;
                }
              } else {
                snprintf(key, sizeof key, "%s", field);
                delta = 0;
              }
            }
          }
          if (key[0]) {
            const char *q = out;
            tmp[0] = 0;
            tolen = 0;
            tkept = 0;
            idx = 0;
            found_i = -1;
            hit = 0;
            if (out[0]) {
              while (*q) {
                const char *qs = q;
                size_t qflen;
                int match = 0;
                long oldv = 0;
                while (*q && *q != '\n') q++;
                qflen = (size_t)(q - qs);
                {
                  size_t take = qflen;
                  if (take >= sizeof field) take = sizeof field - 1;
                  memcpy(field, qs, take);
                  field[take] = 0;
                  {
                    const char *sp = sn ? strstr(field, sep) : NULL;
                    if (sp) {
                      left_n = (size_t)(sp - field);
                      if (left_n >= sizeof left) left_n = sizeof left - 1;
                      memcpy(left, field, left_n);
                      left[left_n] = 0;
                      match = (strcmp(left, key) == 0);
                      if (match) {
                        char *end = 0;
                        oldv = strtol(sp + sn, &end, 10);
                      }
                    } else {
                      match = (strcmp(field, key) == 0);
                    }
                  }
                  if (match && found_i < 0) {
                    long newv = oldv + delta;
                    snprintf(vbuf, sizeof vbuf, "%ld", newv);
                    if (sn == 0)
                      snprintf(newline, sizeof newline, "%s", vbuf);
                    else
                      snprintf(newline, sizeof newline, "%s%s%s", key, sep, vbuf);
                    found_i = idx;
                    hit = 1;
                  } else {
                    snprintf(newline, sizeof newline, "%s", field);
                  }
                }
                {
                  size_t nlen = strlen(newline);
                  if (tkept > 0 && tolen + 1 < sizeof tmp) tmp[tolen++] = '\n';
                  if (tolen + nlen < sizeof tmp) {
                    memcpy(tmp + tolen, newline, nlen);
                    tolen += nlen;
                  } else if (tolen < sizeof tmp - 1) {
                    size_t t = sizeof tmp - 1 - tolen;
                    memcpy(tmp + tolen, newline, t);
                    tolen += t;
                  }
                  tmp[tolen] = 0;
                }
                tkept++;
                idx++;
                if (*q == '\n') q++;
              }
            }
            if (!hit) {
              if (sn == 0)
                snprintf(newline, sizeof newline, "%s", key);
              else {
                snprintf(vbuf, sizeof vbuf, "%ld", delta);
                snprintf(newline, sizeof newline, "%s%s%s", key, sep, vbuf);
              }
              {
                size_t nlen = strlen(newline);
                if (tkept > 0 && tolen + 1 < sizeof tmp) tmp[tolen++] = '\n';
                if (tolen + nlen < sizeof tmp) {
                  memcpy(tmp + tolen, newline, nlen);
                  tolen += nlen;
                } else if (tolen < sizeof tmp - 1) {
                  size_t t = sizeof tmp - 1 - tolen;
                  memcpy(tmp + tolen, newline, t);
                  tolen += t;
                }
                tmp[tolen] = 0;
              }
              tkept++;
              added++;
            } else {
              hit_n++;
            }
            snprintf(out, sizeof out, "%s", tmp);
            olen = tolen;
            kept = tkept;
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "MERGEKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "MERGEKV_N", kept);
      var_set_num(vm, "MERGEKV_ADDED", added);
      var_set_num(vm, "MERGEKV_HIT", hit_n);
      var_set_num(vm, "KVADDALL_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS DIFFKV|SUBKV|DELTAKV bag_a bag_b [sep]
     * — subtract bag_b key:val values from bag_a (shared keys: a-b);
     * keys only in bag_b are appended as -b. Default sep ":".
     * LAST = delta bag; LAST_N = field count; DIFFKV_HIT = shared keys
     * subtracted; DIFFKV_ADDED = keys appended from bag_b only.
     * Usability: FREQ delta (now − baseline) without EACH+LOOKUPN+arith+KVSET. */
    if (kw(&L->cur,"DIFFKV") || kw(&L->cur,"SUBKV") || kw(&L->cur,"DELTAKV") ||
        kw(&L->cur,"SUBFREQ") || kw(&L->cur,"KVSUB") || kw(&L->cur,"DELTFREQ") ||
        kw(&L->cur,"DELTAFREQ") || kw(&L->cur,"KVDIFF") || kw(&L->cur,"SUBHIST")){
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX], sep[32];
      char out[CUBALC_HOST_STR_MAX], field[512], left[256], newline[768], vbuf[40];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long kept = 0, added = 0, hit_n = 0;
      lex_next(L);
      a[0] = 0; b[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, a, sizeof a) != 0)
        snprintf(a, sizeof a, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, b, sizeof b) != 0) b[0] = 0;
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      /* seed out from bag_a */
      if (a[0]) {
        p = a;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              size_t nlen = strlen(field);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, field, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, field, t);
                olen += t;
              }
              out[olen] = 0;
            }
            kept++;
          }
          if (*p == '\n') p++;
        }
      }
      /* fold bag_b keys into out (subtract) */
      if (b[0]) {
        p = b;
        while (*p) {
          long delta = 0, found_i = -1, hit = 0;
          char key[256];
          char tmp[CUBALC_HOST_STR_MAX];
          size_t tolen = 0;
          long tkept = 0, idx = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            size_t take = flen;
            const char *sp;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            key[0] = 0;
            if (sn == 0) {
              snprintf(key, sizeof key, "%s", field);
              delta = 1;
            } else {
              sp = strstr(field, sep);
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof key) left_n = sizeof key - 1;
                memcpy(key, field, left_n);
                key[left_n] = 0;
                {
                  char *end = 0;
                  delta = strtol(sp + sn, &end, 10);
                  if (end == sp + sn) delta = 0;
                }
              } else {
                snprintf(key, sizeof key, "%s", field);
                delta = 0;
              }
            }
          }
          if (key[0]) {
            const char *q = out;
            tmp[0] = 0;
            tolen = 0;
            tkept = 0;
            idx = 0;
            found_i = -1;
            hit = 0;
            if (out[0]) {
              while (*q) {
                const char *qs = q;
                size_t qflen;
                int match = 0;
                long oldv = 0;
                while (*q && *q != '\n') q++;
                qflen = (size_t)(q - qs);
                {
                  size_t take = qflen;
                  if (take >= sizeof field) take = sizeof field - 1;
                  memcpy(field, qs, take);
                  field[take] = 0;
                  {
                    const char *sp = sn ? strstr(field, sep) : NULL;
                    if (sp) {
                      left_n = (size_t)(sp - field);
                      if (left_n >= sizeof left) left_n = sizeof left - 1;
                      memcpy(left, field, left_n);
                      left[left_n] = 0;
                      match = (strcmp(left, key) == 0);
                      if (match) {
                        char *end = 0;
                        oldv = strtol(sp + sn, &end, 10);
                      }
                    } else {
                      match = (strcmp(field, key) == 0);
                    }
                  }
                  if (match && found_i < 0) {
                    long newv = oldv - delta;
                    snprintf(vbuf, sizeof vbuf, "%ld", newv);
                    if (sn == 0)
                      snprintf(newline, sizeof newline, "%s", vbuf);
                    else
                      snprintf(newline, sizeof newline, "%s%s%s", key, sep, vbuf);
                    found_i = idx;
                    hit = 1;
                  } else {
                    snprintf(newline, sizeof newline, "%s", field);
                  }
                }
                {
                  size_t nlen = strlen(newline);
                  if (tkept > 0 && tolen + 1 < sizeof tmp) tmp[tolen++] = '\n';
                  if (tolen + nlen < sizeof tmp) {
                    memcpy(tmp + tolen, newline, nlen);
                    tolen += nlen;
                  } else if (tolen < sizeof tmp - 1) {
                    size_t t = sizeof tmp - 1 - tolen;
                    memcpy(tmp + tolen, newline, t);
                    tolen += t;
                  }
                  tmp[tolen] = 0;
                }
                tkept++;
                idx++;
                if (*q == '\n') q++;
              }
            }
            if (!hit) {
              /* key only in bag_b → append as -delta */
              long neg = -delta;
              if (sn == 0)
                snprintf(newline, sizeof newline, "%s", key);
              else {
                snprintf(vbuf, sizeof vbuf, "%ld", neg);
                snprintf(newline, sizeof newline, "%s%s%s", key, sep, vbuf);
              }
              {
                size_t nlen = strlen(newline);
                if (tkept > 0 && tolen + 1 < sizeof tmp) tmp[tolen++] = '\n';
                if (tolen + nlen < sizeof tmp) {
                  memcpy(tmp + tolen, newline, nlen);
                  tolen += nlen;
                } else if (tolen < sizeof tmp - 1) {
                  size_t t = sizeof tmp - 1 - tolen;
                  memcpy(tmp + tolen, newline, t);
                  tolen += t;
                }
                tmp[tolen] = 0;
              }
              tkept++;
              added++;
            } else {
              hit_n++;
            }
            snprintf(out, sizeof out, "%s", tmp);
            olen = tolen;
            kept = tkept;
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "DIFFKV", out);
      var_set_str(vm, "SUBKV", out);
      var_set_str(vm, "DELTAKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "DIFFKV_N", kept);
      var_set_num(vm, "DIFFKV_ADDED", added);
      var_set_num(vm, "DIFFKV_HIT", hit_n);
      var_set_num(vm, "SUBKV_N", kept);
      var_set_num(vm, "DELTAKV_HIT", hit_n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SUMKV|TOTALKV|SUMVALS bag [sep]
     * — sum numeric right-hand sides of key:val bag fields (default sep ":").
     * LAST = decimal string of sum; LAST_N = sum; SUMKV_N = fields used;
     * SUMKV_KEYS = total fields seen (incl. non-numeric skipped).
     * Usability: FREQ total event count without VALS+SUM two-step. */
    if (kw(&L->cur,"SUMKV") || kw(&L->cur,"TOTALKV") || kw(&L->cur,"SUMVALS") ||
        kw(&L->cur,"KVSUM") || kw(&L->cur,"SUMCOUNTS") || kw(&L->cur,"FREQTOTAL") ||
        kw(&L->cur,"TOTALCOUNTS") || kw(&L->cur,"SUMDICT") || kw(&L->cur,"KVTOTAL")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[40];
      const char *p, *start;
      size_t flen, sn;
      long sum = 0, used = 0, keys = 0;
      lex_next(L);
      bag[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            keys++;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *rhs = field;
              char *end = 0;
              long v;
              if (sn > 0) {
                const char *sp = strstr(field, sep);
                if (sp) rhs = sp + sn;
                else rhs = NULL; /* no sep → skip (key-only line) */
              }
              if (rhs) {
                v = strtol(rhs, &end, 10);
                if (end != rhs) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0) {
                    sum += v;
                    used++;
                  }
                }
              }
            }
          }
          if (*p == '\n') p++;
        }
      }
      snprintf(out, sizeof out, "%ld", sum);
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "SUMKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = sum;
      var_set_num(vm, "LAST_N", sum);
      var_set_num(vm, "SUMKV_N", used);
      var_set_num(vm, "SUMKV_KEYS", keys);
      var_set_num(vm, "TOTALKV_N", sum);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS AVGKV|MEANKV|MEANVAL bag [sep]
     * — integer mean of key:val numeric values (sum/used, trunc toward 0).
     * Empty/no numeric fields → 0. Default sep ":".
     * LAST = decimal mean; LAST_N = mean; AVGKV_N = fields used;
     * AVGKV_SUM = sum; AVGKV_KEYS = fields seen.
     * Usability: typical FREQ count without SUMKV+count+DIV glue (≠ DIVKV per-key). */
    if (kw(&L->cur,"AVGKV") || kw(&L->cur,"MEANKV") || kw(&L->cur,"MEANVAL") ||
        kw(&L->cur,"KVAVG") || kw(&L->cur,"AVGVALS") || kw(&L->cur,"MEANCOUNTS") ||
        kw(&L->cur,"FREQAVG") || kw(&L->cur,"AVGHIST") || kw(&L->cur,"KVMEAN")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[40];
      const char *p, *start;
      size_t flen, sn;
      long sum = 0, used = 0, keys = 0, avg = 0;
      lex_next(L);
      bag[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            keys++;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *rhs = field;
              char *end = 0;
              long v;
              if (sn > 0) {
                const char *sp = strstr(field, sep);
                if (sp) rhs = sp + sn;
                else rhs = NULL;
              }
              if (rhs) {
                v = strtol(rhs, &end, 10);
                if (end != rhs) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0) {
                    sum += v;
                    used++;
                  }
                }
              }
            }
          }
          if (*p == '\n') p++;
        }
      }
      if (used > 0) avg = sum / used;
      snprintf(out, sizeof out, "%ld", avg);
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "AVGKV", out);
      var_set_str(vm, "MEANKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = avg;
      var_set_num(vm, "LAST_N", avg);
      var_set_num(vm, "AVGKV_N", used);
      var_set_num(vm, "AVGKV_SUM", sum);
      var_set_num(vm, "AVGKV_KEYS", keys);
      var_set_num(vm, "MEANKV_N", used);
      var_set_num(vm, "MEANKV_SUM", sum);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MEDIANKV|P50KV|MIDKV bag [sep]
     * — integer median of key:val numeric values (sort; even n → lower mid).
     * Cap 256 values. Empty → 0. Default sep ":".
     * LAST = decimal median; LAST_N = median; MEDIANKV_N = fields used.
     * Usability: outlier-robust typical FREQ count (vs AVGKV) without VALS+MEDIAN. */
    if (kw(&L->cur,"MEDIANKV") || kw(&L->cur,"P50KV") || kw(&L->cur,"MIDKV") ||
        kw(&L->cur,"KVMEDIAN") || kw(&L->cur,"MEDVALS") || kw(&L->cur,"MIDCOUNT") ||
        kw(&L->cur,"FREQMED") || kw(&L->cur,"MEDHIST") || kw(&L->cur,"P50HIST")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[40];
      long vals[256];
      const char *p, *start;
      size_t flen, sn;
      long keys = 0, med = 0;
      int n = 0, i;
      lex_next(L);
      bag[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            keys++;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            if (n < 256) {
              const char *rhs = field;
              char *end = 0;
              long v;
              if (sn > 0) {
                const char *sp = strstr(field, sep);
                if (sp) rhs = sp + sn;
                else rhs = NULL;
              }
              if (rhs) {
                v = strtol(rhs, &end, 10);
                if (end != rhs) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0)
                    vals[n++] = v;
                }
              }
            }
          }
          if (*p == '\n') p++;
        }
      }
      if (n > 0) {
        for (i = 1; i < n; i++) {
          long key = vals[i];
          int j = i - 1;
          while (j >= 0 && vals[j] > key) {
            vals[j + 1] = vals[j];
            j--;
          }
          vals[j + 1] = key;
        }
        med = vals[(n - 1) / 2];
      }
      snprintf(out, sizeof out, "%ld", med);
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "MEDIANKV", out);
      var_set_str(vm, "P50KV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = med;
      var_set_num(vm, "LAST_N", med);
      var_set_num(vm, "MEDIANKV_N", n);
      var_set_num(vm, "MEDIANKV_KEYS", keys);
      var_set_num(vm, "P50KV_N", n);
      var_set_num(vm, "MIDKV_N", n);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS TOPKEY|ARGMAXKV|MAXKEY bag [sep]
     * SYS BOTKEY|ARGMINKV|MINKEY bag [sep]
     * — pick first key with max (or min) numeric value in a key:val bag.
     * LAST = winning key; LAST_N = that value; TOPKEY_V/BOTKEY_V = same;
     * TOPKEY_I = 0-based field index (-1 empty). Default sep ":".
     * Usability: dominant FREQ severity without SORTFREQ+TAKE+BEFORE glue. */
    if (kw(&L->cur,"TOPKEY") || kw(&L->cur,"ARGMAXKV") || kw(&L->cur,"MAXKEY") ||
        kw(&L->cur,"KEYMAX") || kw(&L->cur,"MOSTKEY") || kw(&L->cur,"DOMINANT") ||
        kw(&L->cur,"BOTKEY") || kw(&L->cur,"ARGMINKV") || kw(&L->cur,"MINKEY") ||
        kw(&L->cur,"KEYMIN") || kw(&L->cur,"LEASTKEY") || kw(&L->cur,"RAREST")){
      char op[20];
      int want_min = 0;
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], key[256], best_key[256];
      const char *p, *start;
      size_t flen, sn;
      long idx = 0, best_i = -1, best_v = 0, found = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "BOTKEY") == 0 || strcmp(op, "ARGMINKV") == 0 ||
          strcmp(op, "MINKEY") == 0 || strcmp(op, "KEYMIN") == 0 ||
          strcmp(op, "LEASTKEY") == 0 || strcmp(op, "RAREST") == 0)
        want_min = 1;
      lex_next(L);
      bag[0] = 0; best_key[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *sp = sn ? strstr(field, sep) : NULL;
              char *end = 0;
              long v = 0;
              int okv = 0;
              key[0] = 0;
              if (sp) {
                size_t kn = (size_t)(sp - field);
                if (kn >= sizeof key) kn = sizeof key - 1;
                memcpy(key, field, kn);
                key[kn] = 0;
                v = strtol(sp + sn, &end, 10);
                if (end != sp + sn) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0) okv = 1;
                }
              } else if (sn == 0) {
                /* bare fields: key is field, value treated as 0 */
                snprintf(key, sizeof key, "%s", field);
                v = 0;
                okv = 1;
              }
              if (okv && key[0]) {
                if (!found || (want_min ? (v < best_v) : (v > best_v))) {
                  best_v = v;
                  best_i = idx;
                  snprintf(best_key, sizeof best_key, "%s", key);
                  found = 1;
                }
              }
            }
          }
          idx++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", best_key);
      if (want_min) {
        var_set_str(vm, "BOTKEY", best_key);
        var_set_str(vm, "MINKEY", best_key);
        var_set_num(vm, "BOTKEY_V", found ? best_v : 0);
        var_set_num(vm, "BOTKEY_I", best_i);
        var_set_num(vm, "ARGMINKV_V", found ? best_v : 0);
      } else {
        var_set_str(vm, "TOPKEY", best_key);
        var_set_str(vm, "MAXKEY", best_key);
        var_set_num(vm, "TOPKEY_V", found ? best_v : 0);
        var_set_num(vm, "TOPKEY_I", best_i);
        var_set_num(vm, "ARGMAXKV_V", found ? best_v : 0);
      }
      snprintf(vm->last_str, sizeof vm->last_str, "%s", best_key);
      vm->last_n = found ? best_v : 0;
      var_set_num(vm, "LAST_N", found ? best_v : 0);
      var_set_num(vm, "OK", found ? 1 : 0);
      bump(vm); return 1;
    }
    /* SYS THRESHKV|KEEPVAL|MINCOUNT bag min [sep]
     * — keep key:val fields whose numeric value >= min (default sep ":").
     * LAST = filtered bag; LAST_N = remaining field count;
     * THRESHKV_DROP = removed count; THRESHKV_MIN = threshold used.
     * Usability: denoise FREQ (drop rare noise) without EACH+LOOKUPN rebuild. */
    if (kw(&L->cur,"THRESHKV") || kw(&L->cur,"KEEPVAL") || kw(&L->cur,"MINCOUNT") ||
        kw(&L->cur,"KEEPMIN") || kw(&L->cur,"KVTHRESH") || kw(&L->cur,"FILTERVAL") ||
        kw(&L->cur,"MINFREQ") || kw(&L->cur,"DROPRARE") || kw(&L->cur,"PRUNEKV")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t flen, sn, olen = 0;
      long minv = 1, kept = 0, drop = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      /* min threshold: number, expr, or numeric var */
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        minv = parse_expr(vm, L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *dv = var_get(vm, L->cur.text, 0);
        if (dv && !dv->is_str) {
          minv = (long)dv->val;
          lex_next(L);
        }
      }
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            int keep = 0;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *rhs = field;
              char *end = 0;
              long v;
              if (sn > 0) {
                const char *sp = strstr(field, sep);
                if (sp) rhs = sp + sn;
                else rhs = NULL;
              }
              if (rhs) {
                v = strtol(rhs, &end, 10);
                if (end != rhs) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0 && v >= minv) keep = 1;
                }
              }
            }
            if (keep) {
              size_t nlen = strlen(field);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, field, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, field, t);
                olen += t;
              }
              out[olen] = 0;
              kept++;
            } else {
              drop++;
            }
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "THRESHKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "THRESHKV_N", kept);
      var_set_num(vm, "THRESHKV_DROP", drop);
      var_set_num(vm, "THRESHKV_MIN", minv);
      var_set_num(vm, "KEEPVAL_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS DROPZERO|KEEPNZ|NZKV bag [sep]
     * — drop key:val fields whose numeric value is exactly 0; keep negatives and
     * positives. Non-numeric fields kept. Default sep ":".
     * LAST = filtered bag; LAST_N = remaining; DROPZERO_N = dropped count.
     * Usability: clean DIFFKV stable keys (delta 0) without THRESHKV (which drops
     * negatives) or EACH rebuild. */
    if (kw(&L->cur,"DROPZERO") || kw(&L->cur,"KEEPNZ") || kw(&L->cur,"NZKV") ||
        kw(&L->cur,"DROPZ") || kw(&L->cur,"DROP0") || kw(&L->cur,"FILTERZERO") ||
        kw(&L->cur,"NONZERO") || kw(&L->cur,"KEEPNONZERO") || kw(&L->cur,"PRUNEZERO")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t flen, sn, olen = 0;
      long kept = 0, drop = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            int keep = 1;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *rhs = field;
              char *end = 0;
              long v;
              if (sn > 0) {
                const char *sp = strstr(field, sep);
                if (sp) rhs = sp + sn;
                else rhs = NULL;
              }
              if (rhs) {
                v = strtol(rhs, &end, 10);
                if (end != rhs) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0 && v == 0) keep = 0;
                }
              }
            }
            if (keep) {
              size_t nlen = strlen(field);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, field, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, field, t);
                olen += t;
              }
              out[olen] = 0;
              kept++;
            } else {
              drop++;
            }
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "DROPZERO", out);
      var_set_str(vm, "KEEPNZ", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "DROPZERO_N", drop);
      var_set_num(vm, "DROPZERO_KEEP", kept);
      var_set_num(vm, "KEEPNZ_N", kept);
      var_set_num(vm, "NZKV_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS KEEPKEY|GREPKEY bag needle [sep]
     * SYS DROPKEY|GREPVKEY bag needle [sep] — invert (drop matching keys).
     * SYS KEEPKEYI|GREPKEYI — case-insensitive key match.
     * Keep (or drop) key:val fields whose key contains needle. Default sep ":".
     * Empty needle → keep all (or drop all for invert).
     * LAST = filtered bag; LAST_N = remaining; KEEPKEY_DROP = removed count.
     * Usability: filter FREQ/plate by key pattern without EACH+BEFORE+HAS. */
    if (kw(&L->cur,"KEEPKEY") || kw(&L->cur,"GREPKEY") || kw(&L->cur,"FILTERKEY") ||
        kw(&L->cur,"KEYGREP") || kw(&L->cur,"HASKEY") || kw(&L->cur,"MATCHKEY") ||
        kw(&L->cur,"DROPKEY") || kw(&L->cur,"GREPVKEY") || kw(&L->cur,"RMKEY") ||
        kw(&L->cur,"KEEPKEYI") || kw(&L->cur,"GREPKEYI") || kw(&L->cur,"DROPKEYI")){
      char op[24], bag[CUBALC_HOST_STR_MAX], needle[256], sep[32];
      char field[512], key[256], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t flen, sn, nlen, olen = 0;
      long kept = 0, drop = 0;
      int invert = 0, icase = 0;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "DROPKEY") == 0 || strcmp(op, "GREPVKEY") == 0 ||
          strcmp(op, "RMKEY") == 0 || strcmp(op, "DROPKEYI") == 0)
        invert = 1;
      if (strcmp(op, "KEEPKEYI") == 0 || strcmp(op, "GREPKEYI") == 0 ||
          strcmp(op, "DROPKEYI") == 0)
        icase = 1;
      lex_next(L);
      bag[0] = 0; needle[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, needle, sizeof needle) != 0) needle[0] = 0;
      if (L->cur.kind == TK_STR) {
        /* optional sep — only if looks like sep (short) or explicit after needle */
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      nlen = strlen(needle);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            int match = 0, keep;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            key[0] = 0;
            if (sn > 0) {
              const char *sp = strstr(field, sep);
              if (sp) {
                size_t kn = (size_t)(sp - field);
                if (kn >= sizeof key) kn = sizeof key - 1;
                memcpy(key, field, kn);
                key[kn] = 0;
              } else {
                snprintf(key, sizeof key, "%s", field);
              }
            } else {
              snprintf(key, sizeof key, "%s", field);
            }
            if (nlen == 0) {
              match = 1; /* empty needle matches all */
            } else if (!icase) {
              match = (strstr(key, needle) != NULL);
            } else {
              /* ASCII case-insensitive contains */
              char kl[256], nl[256];
              size_t i, kn = strlen(key);
              if (kn >= sizeof kl) kn = sizeof kl - 1;
              for (i = 0; i < kn; i++) {
                char c = key[i];
                kl[i] = (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
              }
              kl[kn] = 0;
              for (i = 0; i < nlen && i < sizeof nl - 1; i++) {
                char c = needle[i];
                nl[i] = (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
              }
              nl[i] = 0;
              match = (strstr(kl, nl) != NULL);
            }
            keep = invert ? !match : match;
            if (keep) {
              size_t fl = strlen(field);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + fl < sizeof out) {
                memcpy(out + olen, field, fl);
                olen += fl;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, field, t);
                olen += t;
              }
              out[olen] = 0;
              kept++;
            } else {
              drop++;
            }
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "KEEPKEY", out);
      var_set_str(vm, "GREPKEY", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "KEEPKEY_N", kept);
      var_set_num(vm, "KEEPKEY_DROP", drop);
      var_set_num(vm, "GREPKEY_N", kept);
      var_set_num(vm, "DROPKEY_N", drop);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS PCTKV|PERCENTKV|SHAREKV bag [sep]
     * — rewrite each numeric key:val value as integer percent of the bag total
     * (v*100)/sum, trunc toward 0. Non-numeric fields kept as-is. Default sep ":".
     * LAST = bag; LAST_N = field count; PCTKV_SUM = original total; PCTKV_N = fields rewritten.
     * Empty/zero total → all rewritten values become 0.
     * Usability: FREQ share-of-total without EACH+SUMKV+arith+KVSET glue. */
    if (kw(&L->cur,"PCTKV") || kw(&L->cur,"PERCENTKV") || kw(&L->cur,"SHAREKV") ||
        kw(&L->cur,"KVPCT") || kw(&L->cur,"PCTFREQ") || kw(&L->cur,"FREQPCT") ||
        kw(&L->cur,"PCTDICT") || kw(&L->cur,"NORMKV") || kw(&L->cur,"KVSHARE")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[CUBALC_HOST_STR_MAX];
      char left[256], newline[768], vbuf[40];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long total = 0, kept = 0, rew = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      /* pass 1: sum numeric values */
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *rhs = field;
              char *end = 0;
              long v;
              if (sn > 0) {
                const char *sp = strstr(field, sep);
                if (sp) rhs = sp + sn;
                else rhs = NULL;
              }
              if (rhs) {
                v = strtol(rhs, &end, 10);
                if (end != rhs) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0) total += v;
                }
              }
            }
          }
          if (*p == '\n') p++;
        }
      }
      /* pass 2: rewrite */
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *sp = sn ? strstr(field, sep) : NULL;
              char *end = 0;
              long v, pct;
              int did = 0;
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
                v = strtol(sp + sn, &end, 10);
                if (end != sp + sn) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0) {
                    pct = (total != 0) ? ((v * 100) / total) : 0;
                    snprintf(vbuf, sizeof vbuf, "%ld", pct);
                    snprintf(newline, sizeof newline, "%s%s%s", left, sep, vbuf);
                    did = 1;
                    rew++;
                  }
                }
              }
              if (!did)
                snprintf(newline, sizeof newline, "%s", field);
            }
            {
              size_t nlen = strlen(newline);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, newline, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, newline, t);
                olen += t;
              }
              out[olen] = 0;
            }
            kept++;
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "PCTKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "PCTKV_N", rew);
      var_set_num(vm, "PCTKV_SUM", total);
      var_set_num(vm, "PCTKV_KEYS", kept);
      var_set_num(vm, "SHAREKV_SUM", total);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS CAPKV|CLAMPKV|MAXVAL bag max [sep]
     * — clamp each numeric key:val value to max (values > max become max).
     * Default sep ":". LAST = bag; LAST_N = field count;
     * CAPKV_N = fields capped; CAPKV_MAX = threshold used.
     * Usability: cap FREQ outliers before PCTKV/TOPKEY without EACH+LOOKUPN+KVSET. */
    if (kw(&L->cur,"CAPKV") || kw(&L->cur,"CLAMPKV") || kw(&L->cur,"MAXVAL") ||
        kw(&L->cur,"KVCAP") || kw(&L->cur,"CAPVAL") || kw(&L->cur,"CEILKV") ||
        kw(&L->cur,"LIMITKV") || kw(&L->cur,"KVCLAMP") || kw(&L->cur,"MAXCOUNT")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[CUBALC_HOST_STR_MAX];
      char left[256], newline[768], vbuf[40];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long maxv = 0, kept = 0, capped = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        maxv = parse_expr(vm, L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *dv = var_get(vm, L->cur.text, 0);
        if (dv && !dv->is_str) {
          maxv = (long)dv->val;
          lex_next(L);
        }
      }
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *sp = sn ? strstr(field, sep) : NULL;
              char *end = 0;
              long v;
              int did = 0;
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
                v = strtol(sp + sn, &end, 10);
                if (end != sp + sn) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0) {
                    if (v > maxv) {
                      v = maxv;
                      capped++;
                    }
                    snprintf(vbuf, sizeof vbuf, "%ld", v);
                    snprintf(newline, sizeof newline, "%s%s%s", left, sep, vbuf);
                    did = 1;
                  }
                }
              }
              if (!did)
                snprintf(newline, sizeof newline, "%s", field);
            }
            {
              size_t nlen = strlen(newline);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, newline, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, newline, t);
                olen += t;
              }
              out[olen] = 0;
            }
            kept++;
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "CAPKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "CAPKV_N", capped);
      var_set_num(vm, "CAPKV_MAX", maxv);
      var_set_num(vm, "CLAMPKV_N", capped);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SCALEKV|MULKV|WEIGHTKV bag factor [sep]
     * — multiply each numeric key:val value by factor (integer). Default sep ":".
     * LAST = bag; LAST_N = field count; SCALEKV_N = fields scaled;
     * SCALEKV_F = factor used.
     * Usability: weight a FREQ source before MERGEKV without EACH+LOOKUPN+KVSET. */
    if (kw(&L->cur,"SCALEKV") || kw(&L->cur,"MULKV") || kw(&L->cur,"WEIGHTKV") ||
        kw(&L->cur,"MULTKV") || kw(&L->cur,"MULVAL") || kw(&L->cur,"KVMUL") ||
        kw(&L->cur,"SCALEVAL") || kw(&L->cur,"TIMSKV") || kw(&L->cur,"FACTORKV")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[CUBALC_HOST_STR_MAX];
      char left[256], newline[768], vbuf[40];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long factor = 1, kept = 0, scaled = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        factor = parse_expr(vm, L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *dv = var_get(vm, L->cur.text, 0);
        if (dv && !dv->is_str) {
          factor = (long)dv->val;
          lex_next(L);
        }
      }
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *sp = sn ? strstr(field, sep) : NULL;
              char *end = 0;
              long v;
              int did = 0;
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
                v = strtol(sp + sn, &end, 10);
                if (end != sp + sn) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0) {
                    v = v * factor;
                    snprintf(vbuf, sizeof vbuf, "%ld", v);
                    snprintf(newline, sizeof newline, "%s%s%s", left, sep, vbuf);
                    did = 1;
                    scaled++;
                  }
                }
              }
              if (!did)
                snprintf(newline, sizeof newline, "%s", field);
            }
            {
              size_t nlen = strlen(newline);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, newline, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, newline, t);
                olen += t;
              }
              out[olen] = 0;
            }
            kept++;
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "SCALEKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "SCALEKV_N", scaled);
      var_set_num(vm, "SCALEKV_F", factor);
      var_set_num(vm, "MULKV_N", scaled);
      var_set_num(vm, "WEIGHTKV_F", factor);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS DIVKV|IDIVKV|QUOTKV bag divisor [sep]
     * — integer-divide each numeric key:val value by divisor (trunc toward 0).
     * Default sep ":". Divisor 0 → all rewritten values become 0 (soft, no fatal).
     * LAST = bag; LAST_N = field count; DIVKV_N = fields divided; DIVKV_D = divisor.
     * Usability: mean of N MERGEKV FREQ sources without EACH+LOOKUPN+DIV+KVSET. */
    if (kw(&L->cur,"DIVKV") || kw(&L->cur,"IDIVKV") || kw(&L->cur,"QUOTKV") ||
        kw(&L->cur,"KVDIV") || kw(&L->cur,"DIVVAL") || kw(&L->cur,"SCALEKVD") ||
        kw(&L->cur,"AVGSCALE") || kw(&L->cur,"MEANKVD") || kw(&L->cur,"DIVALL")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[CUBALC_HOST_STR_MAX];
      char left[256], newline[768], vbuf[40];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long div = 1, kept = 0, divided = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        div = parse_expr(vm, L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *dv = var_get(vm, L->cur.text, 0);
        if (dv && !dv->is_str) {
          div = (long)dv->val;
          lex_next(L);
        }
      }
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *sp = sn ? strstr(field, sep) : NULL;
              char *end = 0;
              long v;
              int did = 0;
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
                v = strtol(sp + sn, &end, 10);
                if (end != sp + sn) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0) {
                    if (div == 0) v = 0;
                    else v = v / div;
                    snprintf(vbuf, sizeof vbuf, "%ld", v);
                    snprintf(newline, sizeof newline, "%s%s%s", left, sep, vbuf);
                    did = 1;
                    divided++;
                  }
                }
              }
              if (!did)
                snprintf(newline, sizeof newline, "%s", field);
            }
            {
              size_t nlen = strlen(newline);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, newline, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, newline, t);
                olen += t;
              }
              out[olen] = 0;
            }
            kept++;
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "DIVKV", out);
      var_set_str(vm, "IDIVKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "DIVKV_N", divided);
      var_set_num(vm, "DIVKV_D", div);
      var_set_num(vm, "IDIVKV_N", divided);
      var_set_num(vm, "QUOTKV_D", div);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS ADDKV|OFFSETKV|ADDVAL bag delta [sep]
     * — add integer delta to each numeric key:val value. Default sep ":".
     * LAST = bag; LAST_N = field count; ADDKV_N = fields adjusted;
     * ADDKV_D = delta used.
     * Usability: Laplace +1 / score offset before PCTKV without EACH+KVINC. */
    if (kw(&L->cur,"ADDKV") || kw(&L->cur,"OFFSETKV") || kw(&L->cur,"ADDVAL") ||
        kw(&L->cur,"BIASKV") || kw(&L->cur,"SHIFTKV") || kw(&L->cur,"ADDFALL") ||
        kw(&L->cur,"INCALL") || kw(&L->cur,"BUMPALL") || kw(&L->cur,"LAPLACE")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[CUBALC_HOST_STR_MAX];
      char left[256], newline[768], vbuf[40];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long delta = 1, kept = 0, adjusted = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_MINUS ||
          L->cur.kind == TK_LPAREN) {
        delta = parse_expr(vm, L);
      } else if (L->cur.kind == TK_IDENT) {
        Var *dv = var_get(vm, L->cur.text, 0);
        if (dv && !dv->is_str) {
          delta = (long)dv->val;
          lex_next(L);
        }
      }
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *sp = sn ? strstr(field, sep) : NULL;
              char *end = 0;
              long v;
              int did = 0;
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
                v = strtol(sp + sn, &end, 10);
                if (end != sp + sn) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0) {
                    v = v + delta;
                    snprintf(vbuf, sizeof vbuf, "%ld", v);
                    snprintf(newline, sizeof newline, "%s%s%s", left, sep, vbuf);
                    did = 1;
                    adjusted++;
                  }
                }
              }
              if (!did)
                snprintf(newline, sizeof newline, "%s", field);
            }
            {
              size_t nlen = strlen(newline);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, newline, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, newline, t);
                olen += t;
              }
              out[olen] = 0;
            }
            kept++;
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "ADDKV", out);
      var_set_str(vm, "OFFSETKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "ADDKV_N", adjusted);
      var_set_num(vm, "ADDKV_D", delta);
      var_set_num(vm, "OFFSETKV_N", adjusted);
      var_set_num(vm, "OFFSETKV_D", delta);
      var_set_num(vm, "ADDVAL_N", adjusted);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS ABSKV|MAGKV|ABSALL bag [sep]
     * — take absolute value of each numeric key:val value. Default sep ":".
     * LAST = bag; LAST_N = field count; ABSKV_N = fields rewritten (negatives).
     * Usability: rank DIFFKV deltas by magnitude with TOPKEY without EACH+IABS. */
    if (kw(&L->cur,"ABSKV") || kw(&L->cur,"MAGKV") || kw(&L->cur,"ABSALL") ||
        kw(&L->cur,"KVMAG") || kw(&L->cur,"ABSHIST") || kw(&L->cur,"MAGNITUDE") ||
        kw(&L->cur,"ABSCOUNT") || kw(&L->cur,"POSMAG") || kw(&L->cur,"MAGNITUDEKV")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[CUBALC_HOST_STR_MAX];
      char left[256], newline[768], vbuf[40];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long kept = 0, rewritten = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *sp = sn ? strstr(field, sep) : NULL;
              char *end = 0;
              long v;
              int did = 0;
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
                v = strtol(sp + sn, &end, 10);
                if (end != sp + sn) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0) {
                    if (v < 0) {
                      v = -v;
                      rewritten++;
                    }
                    snprintf(vbuf, sizeof vbuf, "%ld", v);
                    snprintf(newline, sizeof newline, "%s%s%s", left, sep, vbuf);
                    did = 1;
                  }
                }
              }
              if (!did)
                snprintf(newline, sizeof newline, "%s", field);
            }
            {
              size_t nlen = strlen(newline);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, newline, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, newline, t);
                olen += t;
              }
              out[olen] = 0;
            }
            kept++;
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "ABSKV", out);
      var_set_str(vm, "MAGKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "ABSKV_N", rewritten);
      var_set_num(vm, "ABSKV_KEYS", kept);
      var_set_num(vm, "MAGKV_N", rewritten);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SIGNKV|DIRKV|SGNKV bag [sep]
     * — map each numeric key:val value to signum: -1, 0, or 1. Default sep ":".
     * LAST = bag; LAST_N = field count; SIGNKV_N = fields rewritten;
     * SIGNKV_POS / SIGNKV_NEG / SIGNKV_ZERO = counts per sign class.
     * Usability: DIFFKV direction map without EACH+SIGN+KVSET (pairs with ABSKV). */
    if (kw(&L->cur,"SIGNKV") || kw(&L->cur,"DIRKV") || kw(&L->cur,"SGNKV") ||
        kw(&L->cur,"SIGNUMKV") || kw(&L->cur,"DIRHIST") || kw(&L->cur,"TRENDKV") ||
        kw(&L->cur,"POLKV") || kw(&L->cur,"POLARITY") || kw(&L->cur,"SGNALL")){
      char bag[CUBALC_HOST_STR_MAX], sep[32], field[512], out[CUBALC_HOST_STR_MAX];
      char left[256], newline[768], vbuf[40];
      const char *p, *start;
      size_t flen, sn, olen = 0, left_n;
      long kept = 0, rew = 0, npos = 0, nneg = 0, nzero = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", ":");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        if (!sep[0]) snprintf(sep, sizeof sep, "%s", ":");
        lex_next(L);
      }
      sn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen > 0) {
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            {
              const char *sp = sn ? strstr(field, sep) : NULL;
              char *end = 0;
              long v, s;
              int did = 0;
              if (sp) {
                left_n = (size_t)(sp - field);
                if (left_n >= sizeof left) left_n = sizeof left - 1;
                memcpy(left, field, left_n);
                left[left_n] = 0;
                v = strtol(sp + sn, &end, 10);
                if (end != sp + sn) {
                  while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                    end++;
                  if (end && *end == 0) {
                    if (v > 0) { s = 1; npos++; }
                    else if (v < 0) { s = -1; nneg++; }
                    else { s = 0; nzero++; }
                    snprintf(vbuf, sizeof vbuf, "%ld", s);
                    snprintf(newline, sizeof newline, "%s%s%s", left, sep, vbuf);
                    did = 1;
                    rew++;
                  }
                }
              }
              if (!did)
                snprintf(newline, sizeof newline, "%s", field);
            }
            {
              size_t nlen = strlen(newline);
              if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + nlen < sizeof out) {
                memcpy(out + olen, newline, nlen);
                olen += nlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, newline, t);
                olen += t;
              }
              out[olen] = 0;
            }
            kept++;
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "SIGNKV", out);
      var_set_str(vm, "DIRKV", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "SIGNKV_N", rew);
      var_set_num(vm, "SIGNKV_POS", npos);
      var_set_num(vm, "SIGNKV_NEG", nneg);
      var_set_num(vm, "SIGNKV_ZERO", nzero);
      var_set_num(vm, "DIRKV_POS", npos);
      var_set_num(vm, "DIRKV_NEG", nneg);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS LASTMATCH|GREP1L|FINDFIELDL [I] bag needle — last field containing needle.
     * SYS LASTMATCHI — case-insensitive. LAST = field (empty if miss); LAST_N 0|1.
     * LASTMATCH_I = 0-based index of hit (-1 miss). Empty needle → last field.
     * Usability: latest log/error line without REVL+FIRSTMATCH or GREP+TAIL glue. */
    if (kw(&L->cur,"LASTMATCH") || kw(&L->cur,"GREP1L") || kw(&L->cur,"FINDFIELDL") ||
        kw(&L->cur,"LASTGREP") || kw(&L->cur,"MATCHLAST") || kw(&L->cur,"GREPLAST") ||
        kw(&L->cur,"LASTMATCHI") || kw(&L->cur,"GREP1LI") || kw(&L->cur,"FINDFIELDLI")){
      char op[20];
      int icase = 0;
      char bag[CUBALC_HOST_STR_MAX], needle[256], out[512];
      const char *p, *start;
      size_t flen, nn;
      long idx = 0, found = 0, found_i = -1;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "LASTMATCHI") == 0 || strcmp(op, "GREP1LI") == 0 ||
          strcmp(op, "FINDFIELDLI") == 0)
        icase = 1;
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
      if (bag[0]) {
        p = bag;
        while (*p) {
          int hit = 0;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          {
            char field[512];
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            if (nn == 0) {
              hit = 1;
            } else if (!icase) {
              if (strstr(field, needle) != NULL) hit = 1;
            } else {
              size_t fi, j;
              for (fi = 0; field[fi] && !hit; fi++) {
                for (j = 0; j < nn; j++) {
                  char a = field[fi + j], b = needle[j];
                  if (!a) break;
                  if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                  if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                  if (a != b) break;
                }
                if (j == nn) hit = 1;
              }
            }
            if (hit) {
              /* keep scanning — last hit wins */
              if (flen >= sizeof out) flen = sizeof out - 1;
              memcpy(out, start, flen);
              out[flen] = 0;
              found = 1;
              found_i = idx;
            }
          }
          idx++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "GREP", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = found;
      var_set_num(vm, "LAST_N", found);
      var_set_num(vm, "LASTMATCH_N", found);
      var_set_num(vm, "LASTMATCH_I", found_i);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS CHUNK|BATCH|GROUPN n [bag] [join_sep]
     * — group bag fields into batches of n; join each batch with join_sep
     * (default space); batches become newline fields → LAST.
     * n<=0 → empty. LAST_N/CHUNK_N = batch count; CHUNK_TOTAL = input fields.
     * Usability: peer/work list paging without EACH+CAT index glue. */
    if (kw(&L->cur,"CHUNK") || kw(&L->cur,"BATCH") || kw(&L->cur,"GROUPN") ||
        kw(&L->cur,"BUNDLE") || kw(&L->cur,"PARTN") || kw(&L->cur,"SPLITN") ||
        kw(&L->cur,"NCHUNK") || kw(&L->cur,"BATCHLINES")){
      long nwant = 0, total = 0, batches = 0, in_batch = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], jsep[32];
      char batch[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t olen = 0, blen = 0, flen, jsepn;
      lex_next(L);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
        nwant = parse_expr(vm, L);
      else
        nwant = 0;
      bag[0] = 0; out[0] = 0; batch[0] = 0;
      snprintf(jsep, sizeof jsep, "%s", " ");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_STR) {
        snprintf(jsep, sizeof jsep, "%s", L->cur.text);
        lex_next(L);
      }
      jsepn = strlen(jsep);
      if (nwant > 0 && bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (in_batch > 0) {
            if (blen + jsepn < sizeof batch) {
              memcpy(batch + blen, jsep, jsepn);
              blen += jsepn;
            }
          }
          if (blen + flen < sizeof batch) {
            memcpy(batch + blen, start, flen);
            blen += flen;
          } else if (blen < sizeof batch - 1) {
            size_t t = sizeof batch - 1 - blen;
            memcpy(batch + blen, start, t);
            blen += t;
          }
          batch[blen] = 0;
          in_batch++;
          total++;
          if (in_batch >= nwant) {
            if (batches > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + blen < sizeof out) {
              memcpy(out + olen, batch, blen);
              olen += blen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, batch, t);
              olen += t;
            }
            out[olen] = 0;
            batches++;
            in_batch = 0;
            blen = 0;
            batch[0] = 0;
          }
          if (*p == '\n') p++;
        }
        /* flush partial last batch */
        if (in_batch > 0) {
          if (batches > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + blen < sizeof out) {
            memcpy(out + olen, batch, blen);
            olen += blen;
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            memcpy(out + olen, batch, t);
            olen += t;
          }
          out[olen] = 0;
          batches++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = batches;
      var_set_num(vm, "LAST_N", batches);
      var_set_num(vm, "CHUNK_N", batches);
      var_set_num(vm, "CHUNK_TOTAL", total);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS WINDOW|SLIDE|NGRAM n [bag] [join_sep]
     * — overlapping windows of n consecutive bag fields; join within each
     * window with join_sep (default space); windows → newline fields in LAST.
     * n<=0 or fields < n → empty. LAST_N/WINDOW_N = window count;
     * WINDOW_TOTAL = input fields; WINDOW_SIZE = n.
     * Usability: consecutive pairs/ngrams / rolling peer pairs without
     * EACH+NTH+CAT glue. Complements non-overlapping SYS CHUNK. */
    if (kw(&L->cur,"WINDOW") || kw(&L->cur,"SLIDE") || kw(&L->cur,"SLIDING") ||
        kw(&L->cur,"NGRAM") || kw(&L->cur,"WINS") || kw(&L->cur,"ROLLWIN") ||
        kw(&L->cur,"PAIRWISE") || kw(&L->cur,"OVERLAP") || kw(&L->cur,"SWINDOW")){
      long nwant = 0, total = 0, windows = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], jsep[32];
      enum { WIN_MAX = 256 };
      const char *fstart[WIN_MAX];
      size_t flens[WIN_MAX];
      const char *p, *start;
      size_t olen = 0, jsepn, flen;
      long i, j;
      lex_next(L);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
        nwant = parse_expr(vm, L);
      else
        nwant = 0;
      bag[0] = 0; out[0] = 0;
      snprintf(jsep, sizeof jsep, "%s", " ");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_STR) {
        snprintf(jsep, sizeof jsep, "%s", L->cur.text);
        lex_next(L);
      }
      jsepn = strlen(jsep);
      if (bag[0]) {
        p = bag;
        while (*p && total < WIN_MAX) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          fstart[total] = start;
          flens[total] = flen;
          total++;
          if (*p == '\n') p++;
        }
      }
      if (nwant > 0 && total >= nwant) {
        for (i = 0; i <= total - nwant; i++) {
          if (windows > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          for (j = 0; j < nwant; j++) {
            if (j > 0 && jsepn > 0) {
              if (olen + jsepn < sizeof out) {
                memcpy(out + olen, jsep, jsepn);
                olen += jsepn;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, jsep, t);
                olen += t;
              }
            }
            flen = flens[i + j];
            if (olen + flen < sizeof out) {
              memcpy(out + olen, fstart[i + j], flen);
              olen += flen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, fstart[i + j], t);
              olen += t;
            }
          }
          out[olen] = 0;
          windows++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = windows;
      var_set_num(vm, "LAST_N", windows);
      var_set_num(vm, "WINDOW_N", windows);
      var_set_num(vm, "WINDOW_TOTAL", total);
      var_set_num(vm, "WINDOW_SIZE", nwant > 0 ? nwant : 0);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS STRIDE|EVERY|STEP step [offset] [bag]
     * — keep bag fields where index % step == offset (0-based).
     * offset defaults to 0; optional OFF|OFFSET keyword before offset.
     * step<=0 → empty. LAST_N/STRIDE_N = kept; STRIDE_TOTAL = input fields;
     * STRIDE_STEP/STRIDE_OFF = params.
     * Usability: partition work across workers without EACH+MOD index glue.
     * Complements CHUNK (batch size) and WINDOW (overlap). */
    if (kw(&L->cur,"STRIDE") || kw(&L->cur,"EVERY") || kw(&L->cur,"STEP") ||
        kw(&L->cur,"NTHOF") || kw(&L->cur,"STRIDEBAG") || kw(&L->cur,"TAKESTEP") ||
        kw(&L->cur,"EVERYN") || kw(&L->cur,"MODPICK") || kw(&L->cur,"PARTNIDX")){
      long step = 0, offset = 0, total = 0, kept = 0, idx = 0, off_norm = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      const char *p, *start;
      size_t olen = 0, flen;
      lex_next(L);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
        step = parse_expr(vm, L);
      else
        step = 0;
      offset = 0;
      if (kw(&L->cur,"OFF") || kw(&L->cur,"OFFSET") || kw(&L->cur,"FROM") ||
          kw(&L->cur,"START") || kw(&L->cur,"AT")){
        lex_next(L);
        if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
            L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
          offset = parse_expr(vm, L);
      } else if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
                 L->cur.kind == TK_MINUS) {
        /* bare numeric offset (not IDENT — that is bag name) */
        offset = parse_expr(vm, L);
      }
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (step > 0) {
        off_norm = offset % step;
        if (off_norm < 0) off_norm += step;
      }
      if (step > 0 && bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          total++;
          if ((idx % step) == off_norm) {
            if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + flen < sizeof out) {
              memcpy(out + olen, start, flen);
              olen += flen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, start, t);
              olen += t;
            }
            out[olen] = 0;
            kept++;
          }
          idx++;
          if (*p == '\n') p++;
        }
      } else if (bag[0]) {
        /* step<=0: count total only, keep none */
        p = bag;
        while (*p) {
          while (*p && *p != '\n') p++;
          total++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "STRIDE_N", kept);
      var_set_num(vm, "STRIDE_TOTAL", total);
      var_set_num(vm, "STRIDE_STEP", step > 0 ? step : 0);
      var_set_num(vm, "STRIDE_OFF", step > 0 ? off_norm : 0);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS FLATTEN|UNCHUNK|EXPAND|SPLITALL [bag] [sep]
     * — split every bag field by sep (default space) and emit all tokens as
     * a flat newline bag → LAST. Empty sep concatenates nothing extra (each
     * non-empty field kept as one token). LAST_N/FLATTEN_N = token count;
     * FLATTEN_LINES = input field count.
     * Usability: reverse of SYS CHUNK join — re-expand batched peer/work
     * lists without EACH+SPLIT+PUSH glue. */
    if (kw(&L->cur,"FLATTEN") || kw(&L->cur,"UNCHUNK") || kw(&L->cur,"EXPAND") ||
        kw(&L->cur,"SPLITALL") || kw(&L->cur,"MAPSPLIT") || kw(&L->cur,"FLATSPLIT") ||
        kw(&L->cur,"UNBATCH") || kw(&L->cur,"FLATM") || kw(&L->cur,"BAGFLATTEN")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], sep[64];
      const char *p, *lstart, *lp, *tstart;
      size_t olen = 0, sepn, lflen, tlen;
      long tokens = 0, lines = 0;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      snprintf(sep, sizeof sep, "%s", " ");
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_STR) {
        snprintf(sep, sizeof sep, "%s", L->cur.text);
        lex_next(L);
      }
      sepn = strlen(sep);
      if (bag[0]) {
        p = bag;
        while (*p) {
          lstart = p;
          while (*p && *p != '\n') p++;
          lflen = (size_t)(p - lstart);
          lines++;
          if (sepn == 0) {
            /* empty sep: emit whole field as one token (if non-empty) */
            if (lflen > 0) {
              if (tokens > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + lflen < sizeof out) {
                memcpy(out + olen, lstart, lflen);
                olen += lflen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, lstart, t);
                olen += t;
              }
              out[olen] = 0;
              tokens++;
            }
          } else {
            const char *lend = lstart + lflen;
            lp = lstart;
            for (;;) {
              tstart = lp;
              while (lp < lend &&
                     !(sepn > 0 && (size_t)(lend - lp) >= sepn &&
                       memcmp(lp, sep, sepn) == 0))
                lp++;
              tlen = (size_t)(lp - tstart);
              if (tokens > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
              if (olen + tlen < sizeof out) {
                memcpy(out + olen, tstart, tlen);
                olen += tlen;
              } else if (olen < sizeof out - 1) {
                size_t t = sizeof out - 1 - olen;
                memcpy(out + olen, tstart, t);
                olen += t;
              }
              out[olen] = 0;
              tokens++;
              if (lp >= lend) break;
              /* consume sep; if it was trailing, loop emits empty token */
              lp += sepn;
            }
          }
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = tokens;
      var_set_num(vm, "LAST_N", tokens);
      var_set_num(vm, "FLATTEN_N", tokens);
      var_set_num(vm, "FLATTEN_LINES", lines);
      var_set_num(vm, "UNCHUNK_N", tokens);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS CUTALL|MAPCUT|COLALL bag sep n — peel Nth sep-field from every bag line.
     * CUTALL/MAPCUT/FIELDNALL: 0-based. COLALL/COLUMNALL: 1-based.
     * LAST = bag of peeled fields (empty token if miss). LAST_N = line count.
     * CUTALL_HIT = how many lines had the column. Usability: log column → FREQ
     * without EACH LINE + CUT glue. */
    if (kw(&L->cur,"CUTALL") || kw(&L->cur,"MAPCUT") || kw(&L->cur,"FIELDNALL") ||
        kw(&L->cur,"CUTBAG") || kw(&L->cur,"BAGCUT") ||
        kw(&L->cur,"COLALL") || kw(&L->cur,"COLUMNALL") || kw(&L->cur,"MAPCOLUMN") ||
        kw(&L->cur,"COLBAG")){
      char op[20];
      int one_based = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], sep[64];
      long want = 0, lines = 0, hits = 0;
      const char *lp, *lstart;
      size_t olen = 0, sepn, lflen;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "COLALL") == 0 || strcmp(op, "COLUMNALL") == 0 ||
          strcmp(op, "MAPCOLUMN") == 0 || strcmp(op, "COLBAG") == 0)
        one_based = 1;
      lex_next(L);
      bag[0] = 0; out[0] = 0; sep[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (resolve_str_arg(vm, L, sep, sizeof sep) != 0) sep[0] = 0;
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
        want = parse_expr(vm, L);
      else
        want = 0;
      if (one_based) {
        if (want < 1) want = 1;
        want = want - 1;
      } else {
        if (want < 0) want = 0;
      }
      sepn = strlen(sep);
      if (bag[0]) {
        lp = bag;
        while (*lp) {
          char line[512], cell[512];
          const char *p, *hit, *start;
          long idx = 0, found = 0;
          size_t flen, take;
          lstart = lp;
          while (*lp && *lp != '\n') lp++;
          lflen = (size_t)(lp - lstart);
          take = lflen;
          if (take >= sizeof line) take = sizeof line - 1;
          memcpy(line, lstart, take);
          line[take] = 0;
          cell[0] = 0;
          p = line;
          if (sepn == 0) {
            if (want == 0) {
              snprintf(cell, sizeof cell, "%s", line);
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
                if (flen >= sizeof cell) flen = sizeof cell - 1;
                memcpy(cell, start, flen);
                cell[flen] = 0;
                found = 1;
                break;
              }
              idx++;
              if (!hit) break;
            }
          }
          if (lines > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          {
            size_t n = strlen(cell);
            if (olen + n < sizeof out) {
              memcpy(out + olen, cell, n);
              olen += n;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, cell, t);
              olen += t;
            }
            out[olen] = 0;
          }
          if (found) hits++;
          lines++;
          if (*lp == '\n') lp++;
        }
      }
      var_set_str(vm, "LAST", out);
      var_set_str(vm, "CUT", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = lines;
      var_set_num(vm, "LAST_N", lines);
      var_set_num(vm, "CUTALL_N", lines);
      var_set_num(vm, "CUTALL_HIT", hits);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS MIDLINES|SLICEBAG|FIELDSLICE|LINESLICE bag start [end]
     * — keep newline fields [start..end] inclusive, 0-based.
     * end omitted → through last field. start/end clamp; start>end → empty.
     * Usability: middle bag windows without DROP+TAKE two-step glue.
     * Note: SYS SLICE is string MID — use SLICEBAG/MIDLINES for bags. */
    if (kw(&L->cur,"MIDLINES") || kw(&L->cur,"SLICEBAG") || kw(&L->cur,"FIELDSLICE") ||
        kw(&L->cur,"LINESLICE") || kw(&L->cur,"BAGSLICE") || kw(&L->cur,"SLICEFIELDS") ||
        kw(&L->cur,"SUBLINES") || kw(&L->cur,"WINDOWLINES")){
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      long start_i = 0, end_i = -1, idx = 0, kept = 0;
      const char *p, *start;
      size_t olen = 0, flen;
      lex_next(L);
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)
        start_i = parse_prim(vm, L);
      else
        start_i = 0;
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_IDENT ||
          L->cur.kind == TK_LPAREN || L->cur.kind == TK_MINUS)
        end_i = parse_prim(vm, L);
      else
        end_i = -1; /* to end */
      if (start_i < 0) start_i = 0;
      if (end_i >= 0 && end_i < start_i) {
        /* empty window */
      } else if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          if (start == p && *p == 0 && start > bag && start[-1] == '\n')
            break;
          flen = (size_t)(p - start);
          if (idx >= start_i && (end_i < 0 || idx <= end_i)) {
            if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
            if (olen + flen < sizeof out) {
              memcpy(out + olen, start, flen);
              olen += flen;
            } else if (olen < sizeof out - 1) {
              size_t t = sizeof out - 1 - olen;
              memcpy(out + olen, start, t);
              olen += t;
            }
            out[olen] = 0;
            kept++;
          }
          idx++;
          if (*p == '\n') p++;
          /* early stop if past end */
          if (end_i >= 0 && idx > end_i) break;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "MIDLINES_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SORTN|NSORT|NUMSORT [DESC|R] [str|LAST] — numeric sort of newline fields.
     * Lex SORT orders "10" before "2"; SORTN orders by integer value.
     * Non-numeric / blank fields sort as 0; stable on ties. LAST_N/SORT_N = count.
     * Usability: score bags / sizes after LIST without shell sort -n. */
    if (kw(&L->cur,"SORTN") || kw(&L->cur,"NSORT") || kw(&L->cur,"NUMSORT") ||
        kw(&L->cur,"SORTNUM") || kw(&L->cur,"ISORT") || kw(&L->cur,"SORTINT") ||
        kw(&L->cur,"RSORTN") || kw(&L->cur,"SORTND") || kw(&L->cur,"NSORTR")){
      char src[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      enum { SORTN_MAX = 256, SORTN_FLEN = 192 };
      char fields[SORTN_MAX][SORTN_FLEN];
      long nvals[SORTN_MAX];
      int order[SORTN_MAX];
      int n = 0, i, desc = 0;
      const char *p, *start;
      size_t olen = 0;
      long kept = 0;
      char op0[16];
      snprintf(op0, sizeof op0, "%s", L->cur.text);
      for (char *q = op0; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op0, "RSORTN") == 0 || strcmp(op0, "SORTND") == 0 ||
          strcmp(op0, "NSORTR") == 0)
        desc = 1;
      lex_next(L);
      /* optional DESC|ASC|R|REV flag */
      if (kw(&L->cur,"DESC") || kw(&L->cur,"DESCENDING") || kw(&L->cur,"REVERSE") ||
          kw(&L->cur,"REV") || kw(&L->cur,"R") || kw(&L->cur,"-R") ||
          kw(&L->cur,"-N") || kw(&L->cur,"DOWN")) {
        desc = 1;
        lex_next(L);
      } else if (kw(&L->cur,"ASC") || kw(&L->cur,"ASCENDING") || kw(&L->cur,"UP")) {
        desc = 0;
        lex_next(L);
      }
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      out[0] = 0;
      if (src[0]) {
        p = src;
        while (*p && n < SORTN_MAX) {
          start = p;
          while (*p && *p != '\n') p++;
          if (start == p && *p == 0 && start > src && start[-1] == '\n')
            break;
          {
            size_t flen = (size_t)(p - start);
            char *end = NULL;
            long v = 0;
            if (flen >= SORTN_FLEN) flen = SORTN_FLEN - 1;
            memcpy(fields[n], start, flen);
            fields[n][flen] = 0;
            if (fields[n][0]) {
              v = strtol(fields[n], &end, 10);
              if (end == fields[n]) v = 0; /* non-numeric → 0 */
              else {
                while (end && *end && (*end == ' ' || *end == '\t' || *end == '\r'))
                  end++;
                if (end && *end != 0) v = 0; /* trailing junk → 0 */
              }
            }
            nvals[n] = v;
            order[n] = n;
            n++;
          }
          if (*p == '\n') p++;
        }
      }
      if (n > 1) {
        for (i = 1; i < n; i++) {
          int key = order[i], j = i - 1;
          while (j >= 0) {
            long a = nvals[order[j]], b = nvals[key];
            int gt = desc ? (a < b) : (a > b);
            if (!gt && a == b) {
              /* stable: keep original order on tie */
              gt = 0;
            }
            if (!gt) break;
            order[j + 1] = order[j];
            j--;
          }
          order[j + 1] = key;
        }
      }
      for (i = 0; i < n; i++) {
        int idx = order[i];
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
      var_set_num(vm, "SORTN_N", kept);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS SORTLEN|LENSORT|SORTBYLEN [DESC|ASC] [bag] — sort fields by string length.
     * Default ASC (shortest first); DESC/R → longest first. Stable on ties.
     * LAST_N/SORTLEN_N = field count. Cap 256×192 like SORT.
     * Usability: longest-first logs / shortest labels without LENALL+SORTN+rebuild. */
    if (kw(&L->cur,"SORTLEN") || kw(&L->cur,"LENSORT") || kw(&L->cur,"SORTBYLEN") ||
        kw(&L->cur,"LSORT") || kw(&L->cur,"BYLEN") || kw(&L->cur,"SORTBYLENGTH") ||
        kw(&L->cur,"RSORTLEN") || kw(&L->cur,"SORTLEND") || kw(&L->cur,"LENSORTR")){
      char src[CUBALC_HOST_STR_MAX];
      char out[CUBALC_HOST_STR_MAX];
      enum { SL_MAX = 256, SL_FLEN = 192 };
      char fields[SL_MAX][SL_FLEN];
      long lens[SL_MAX];
      int order[SL_MAX];
      int n = 0, i, desc = 0;
      const char *p, *start;
      size_t olen = 0;
      long kept = 0;
      char op0[20];
      snprintf(op0, sizeof op0, "%s", L->cur.text);
      for (char *q = op0; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op0, "RSORTLEN") == 0 || strcmp(op0, "SORTLEND") == 0 ||
          strcmp(op0, "LENSORTR") == 0)
        desc = 1;
      lex_next(L);
      if (kw(&L->cur,"DESC") || kw(&L->cur,"DESCENDING") || kw(&L->cur,"REVERSE") ||
          kw(&L->cur,"REV") || kw(&L->cur,"R") || kw(&L->cur,"-R") ||
          kw(&L->cur,"DOWN") || kw(&L->cur,"LONG") || kw(&L->cur,"LONGEST")) {
        desc = 1;
        lex_next(L);
      } else if (kw(&L->cur,"ASC") || kw(&L->cur,"ASCENDING") || kw(&L->cur,"UP") ||
                 kw(&L->cur,"SHORT") || kw(&L->cur,"SHORTEST")) {
        desc = 0;
        lex_next(L);
      }
      src[0] = 0;
      if (resolve_str_arg(vm, L, src, sizeof src) != 0)
        snprintf(src, sizeof src, "%s", vm->last_str);
      out[0] = 0;
      if (src[0]) {
        p = src;
        while (*p && n < SL_MAX) {
          start = p;
          while (*p && *p != '\n') p++;
          if (start == p && *p == 0 && start > src && start[-1] == '\n')
            break;
          {
            size_t flen = (size_t)(p - start);
            if (flen >= SL_FLEN) flen = SL_FLEN - 1;
            memcpy(fields[n], start, flen);
            fields[n][flen] = 0;
            lens[n] = (long)flen;
            order[n] = n;
            n++;
          }
          if (*p == '\n') p++;
        }
      }
      if (n > 1) {
        for (i = 1; i < n; i++) {
          int key = order[i], j = i - 1;
          while (j >= 0) {
            long a = lens[order[j]], b = lens[key];
            int gt = desc ? (a < b) : (a > b);
            if (!gt) break;
            order[j + 1] = order[j];
            j--;
          }
          order[j + 1] = key;
        }
      }
      for (i = 0; i < n; i++) {
        int idx = order[i];
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
      var_set_num(vm, "SORTLEN_N", kept);
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
    /* SYS ROTATE|ROTL k [bag] — left-rotate bag fields by k (mod n) → LAST.
     * SYS ROTR k [bag] — right-rotate. Optional LEFT|RIGHT after ROTATE.
     * k may be negative (opposite dir). LAST_N/ROTATE_N = field count;
     * ROTATE_K = normalized left-shift amount [0..n). Empty → "".
     * Usability: round-robin peer/work queues without EACH+NTH rebuild.
     * Complements REVL (full reverse) and STRIDE (partition). */
    if (kw(&L->cur,"ROTATE") || kw(&L->cur,"ROTL") || kw(&L->cur,"ROTR") ||
        kw(&L->cur,"ROTLEFT") || kw(&L->cur,"ROTRIGHT") || kw(&L->cur,"CYCLE") ||
        kw(&L->cur,"ROLL") || kw(&L->cur,"SHIFTBAG") || kw(&L->cur,"BAGROT")){
      char op[20];
      int right = 0;
      long k = 0, total = 0, knorm = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      enum { ROT_MAX = 256 };
      const char *fstart[ROT_MAX];
      size_t flens[ROT_MAX];
      const char *p, *start;
      size_t olen = 0, flen;
      long i, src;
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "ROTR") == 0 || strcmp(op, "ROTRIGHT") == 0)
        right = 1;
      lex_next(L);
      if (kw(&L->cur,"LEFT") || kw(&L->cur,"L") || kw(&L->cur,"CCW")) {
        right = 0;
        lex_next(L);
      } else if (kw(&L->cur,"RIGHT") || kw(&L->cur,"R") || kw(&L->cur,"CW")) {
        right = 1;
        lex_next(L);
      }
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
        k = parse_expr(vm, L);
      else
        k = 1; /* default rotate by 1 */
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (bag[0]) {
        p = bag;
        while (*p && total < ROT_MAX) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          fstart[total] = start;
          flens[total] = flen;
          total++;
          if (*p == '\n') p++;
        }
      }
      if (total > 0) {
        long kk = k;
        if (right) kk = -kk;
        knorm = kk % total;
        if (knorm < 0) knorm += total;
        for (i = 0; i < total; i++) {
          src = (i + knorm) % total;
          if (i > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          flen = flens[src];
          if (olen + flen < sizeof out) {
            memcpy(out + olen, fstart[src], flen);
            olen += flen;
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            memcpy(out + olen, fstart[src], t);
            olen += t;
          }
          out[olen] = 0;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = total;
      var_set_num(vm, "LAST_N", total);
      var_set_num(vm, "ROTATE_N", total);
      var_set_num(vm, "ROTATE_K", knorm);
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
    /* SYS COUNTMATCH|GREPCOUNT bag needle — count fields containing needle (substring).
     * SYS COUNTMATCHI|GREPCOUNTI — case-insensitive. Empty needle counts all fields.
     * LAST_N/COUNTMATCH_N = hits; does not replace LAST with filtered bag (unlike GREP).
     * Distinct from COUNTLINE (exact field) and LINES (total). Usability: log severity
     * tallies without GREP clobber or EACH+HAS glue. */
    if (kw(&L->cur,"COUNTMATCH") || kw(&L->cur,"GREPCOUNT") || kw(&L->cur,"COUNTGREP") ||
        kw(&L->cur,"MATCHCOUNT") || kw(&L->cur,"SUBCOUNT") || kw(&L->cur,"CONTAINSCOUNT") ||
        kw(&L->cur,"COUNTMATCHI") || kw(&L->cur,"GREPCOUNTI") || kw(&L->cur,"COUNTGREPI") ||
        kw(&L->cur,"MATCHCOUNTI") || kw(&L->cur,"SUBCOUNTI")){
      char op[20]; snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      int icase = (strcmp(op, "COUNTMATCHI") == 0 || strcmp(op, "GREPCOUNTI") == 0 ||
                   strcmp(op, "COUNTGREPI") == 0 || strcmp(op, "MATCHCOUNTI") == 0 ||
                   strcmp(op, "SUBCOUNTI") == 0);
      char bag[CUBALC_HOST_STR_MAX], needle[512];
      const char *p, *start;
      long hit = 0, total = 0;
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
          total++;
          {
            char field[512];
            size_t take = flen;
            if (take >= sizeof field) take = sizeof field - 1;
            memcpy(field, start, take);
            field[take] = 0;
            if (nn == 0) {
              match = 1;
            } else if (!icase) {
              if (strstr(field, needle) != NULL) match = 1;
            } else {
              size_t fi, j;
              for (fi = 0; field[fi] && !match; fi++) {
                for (j = 0; j < nn; j++) {
                  char a = field[fi + j], b = needle[j];
                  if (!a) break;
                  if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
                  if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
                  if (a != b) break;
                }
                if (j == nn) match = 1;
              }
            }
          }
          if (match) hit++;
          if (*p == '\n') p++;
        }
      }
      vm->last_n = hit;
      var_set_num(vm, "LAST_N", hit);
      var_set_num(vm, "COUNTMATCH_N", hit);
      var_set_num(vm, "GREPCOUNT_N", hit);
      var_set_num(vm, "COUNTMATCH_TOTAL", total);
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
    /* SYS PADALL|MAPAD|RPADALL bag width [padchar] — pad every field to width.
     * SYS LPADALL|MAPLPAD — left-pad; PADALL/RPADALL — right-pad (default).
     * Fields longer than width truncated to width (like LPAD/RPAD). Default
     * pad char space. LAST_N/PADALL_N = field count; PADALL_W = width.
     * Usability: column-align bags after LENALL+MAX without EACH+LPAD glue. */
    if (kw(&L->cur,"PADALL") || kw(&L->cur,"MAPAD") || kw(&L->cur,"MAPPAD") ||
        kw(&L->cur,"RPADALL") || kw(&L->cur,"MAPRPAD") || kw(&L->cur,"PADRIGHTALL") ||
        kw(&L->cur,"LPADALL") || kw(&L->cur,"MAPLPAD") || kw(&L->cur,"PADLEFTALL") ||
        kw(&L->cur,"PADBAG") || kw(&L->cur,"ALIGNALL")){
      char op[20];
      int is_left = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX], pad[16];
      char field[512], padded[512];
      const char *p, *start;
      size_t olen = 0, flen, sn;
      long width = 0, kept = 0;
      char padc = ' ';
      snprintf(op, sizeof op, "%s", L->cur.text);
      for (char *q = op; *q; q++)
        if (*q >= 'a' && *q <= 'z') *q = (char)(*q - 'a' + 'A');
      if (strcmp(op, "LPADALL") == 0 || strcmp(op, "MAPLPAD") == 0 ||
          strcmp(op, "PADLEFTALL") == 0)
        is_left = 1;
      lex_next(L);
      if (kw(&L->cur,"LEFT") || kw(&L->cur,"L") || kw(&L->cur,"LPAD")) {
        is_left = 1;
        lex_next(L);
      } else if (kw(&L->cur,"RIGHT") || kw(&L->cur,"R") || kw(&L->cur,"RPAD")) {
        is_left = 0;
        lex_next(L);
      }
      bag[0] = 0; out[0] = 0; pad[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
        width = parse_expr(vm, L);
      if (width < 0) width = 0;
      if (width > 511) width = 511;
      if (resolve_str_arg(vm, L, pad, sizeof pad) == 0 && pad[0])
        padc = pad[0];
      if (bag[0]) {
        p = bag;
        while (*p) {
          size_t o, need, take, i;
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen >= sizeof field) flen = sizeof field - 1;
          memcpy(field, start, flen);
          field[flen] = 0;
          sn = flen;
          if ((long)sn >= width) {
            take = (size_t)width;
            if (take >= sizeof padded) take = sizeof padded - 1;
            memcpy(padded, field, take);
            padded[take] = 0;
            sn = take;
          } else {
            need = (size_t)width - sn;
            if (is_left) {
              o = 0;
              for (i = 0; i < need && o + 1 < sizeof padded; i++)
                padded[o++] = padc;
              take = sn;
              if (o + take >= sizeof padded) take = sizeof padded - 1 - o;
              memcpy(padded + o, field, take);
              o += take;
              padded[o] = 0;
              sn = o;
            } else {
              o = 0;
              take = sn;
              if (take >= sizeof padded) take = sizeof padded - 1;
              memcpy(padded, field, take);
              o = take;
              for (i = 0; i < need && o + 1 < sizeof padded; i++)
                padded[o++] = padc;
              padded[o] = 0;
              sn = o;
            }
          }
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + sn < sizeof out) {
            memcpy(out + olen, padded, sn);
            olen += sn;
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            memcpy(out + olen, padded, t);
            olen += t;
          }
          out[olen] = 0;
          kept++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "PADALL_N", kept);
      var_set_num(vm, "PADALL_W", width);
      var_set_num(vm, "OK", 1);
      bump(vm); return 1;
    }
    /* SYS TRUNCALL|CLIPALL|TRUNCATEALL [RIGHT] bag width — truncate every field
     * to at most width chars (no pad fill). Default keep left; RIGHT/TAIL keep right.
     * width<=0 → empty fields. LAST_N/TRUNCALL_N = field count; TRUNCALL_W = width;
     * TRUNCALL_HIT = fields that were shortened.
     * Usability: clip log/payload bags without EACH+LEFT/MID glue; dual of PADALL. */
    if (kw(&L->cur,"TRUNCALL") || kw(&L->cur,"CLIPALL") || kw(&L->cur,"TRUNCATEALL") ||
        kw(&L->cur,"MAPTRUNC") || kw(&L->cur,"MAPCLIP") || kw(&L->cur,"SHORTENALL") ||
        kw(&L->cur,"TRIMLEN") || kw(&L->cur,"LIMITALL") || kw(&L->cur,"MAXLENALL")){
      int keep_right = 0;
      char bag[CUBALC_HOST_STR_MAX], out[CUBALC_HOST_STR_MAX];
      char field[512], clipped[512];
      const char *p, *start;
      size_t olen = 0, flen, sn, take;
      long width = 0, kept = 0, hit = 0;
      lex_next(L);
      if (kw(&L->cur,"RIGHT") || kw(&L->cur,"R") || kw(&L->cur,"TAIL") ||
          kw(&L->cur,"END") || kw(&L->cur,"KEEPRIGHT")) {
        keep_right = 1;
        lex_next(L);
      } else if (kw(&L->cur,"LEFT") || kw(&L->cur,"L") || kw(&L->cur,"HEAD") ||
                 kw(&L->cur,"START") || kw(&L->cur,"KEEPLEFT")) {
        keep_right = 0;
        lex_next(L);
      }
      bag[0] = 0; out[0] = 0;
      if (resolve_str_arg(vm, L, bag, sizeof bag) != 0)
        snprintf(bag, sizeof bag, "%s", vm->last_str);
      if (L->cur.kind == TK_NUM || L->cur.kind == TK_LPAREN ||
          L->cur.kind == TK_MINUS || L->cur.kind == TK_IDENT)
        width = parse_expr(vm, L);
      if (width < 0) width = 0;
      if (width > 511) width = 511;
      if (bag[0]) {
        p = bag;
        while (*p) {
          start = p;
          while (*p && *p != '\n') p++;
          flen = (size_t)(p - start);
          if (flen >= sizeof field) flen = sizeof field - 1;
          memcpy(field, start, flen);
          field[flen] = 0;
          if ((long)flen > width) {
            hit++;
            take = (size_t)width;
            if (take >= sizeof clipped) take = sizeof clipped - 1;
            if (keep_right && flen > take) {
              memcpy(clipped, field + (flen - take), take);
            } else {
              memcpy(clipped, field, take);
            }
            clipped[take] = 0;
            sn = take;
          } else {
            sn = flen;
            memcpy(clipped, field, sn);
            clipped[sn] = 0;
          }
          if (kept > 0 && olen + 1 < sizeof out) out[olen++] = '\n';
          if (olen + sn < sizeof out) {
            memcpy(out + olen, clipped, sn);
            olen += sn;
          } else if (olen < sizeof out - 1) {
            size_t t = sizeof out - 1 - olen;
            memcpy(out + olen, clipped, t);
            olen += t;
          }
          out[olen] = 0;
          kept++;
          if (*p == '\n') p++;
        }
      }
      var_set_str(vm, "LAST", out);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
      vm->last_n = kept;
      var_set_num(vm, "LAST_N", kept);
      var_set_num(vm, "TRUNCALL_N", kept);
      var_set_num(vm, "CLIPALL_N", kept);
      var_set_num(vm, "TRUNCALL_W", width);
      var_set_num(vm, "TRUNCALL_HIT", hit);
      var_set_num(vm, "CLIPALL_HIT", hit);
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
    fail(vm, "SYS: READ|WRITE|RM|RENAME|COPY|REALPATH|TOUCH|LIST|GLOB|MATCHFILES|PATHGLOB|PGLOB|FULLGLOB|NTH|GREP|GREPANY|GREPALL|FIRSTMATCH|GREP1|LASTMATCH|GREP1L|LOOKUP|KVGET|LOOKUPN|KVGETN|KVSET|SETKV|KVINC|INCKV|KVDEL|DELKV|MERGEKV|KVADDALL|DIFFKV|SUBKV|SUMKV|TOTALKV|AVGKV|MEANKV|MEDIANKV|P50KV|TOPKEY|BOTKEY|THRESHKV|KEEPVAL|DROPZERO|KEEPNZ|KEEPKEY|GREPKEY|DROPKEY|PCTKV|SHAREKV|CAPKV|CLAMPKV|SCALEKV|MULKV|DIVKV|IDIVKV|ADDKV|OFFSETKV|ABSKV|MAGKV|SIGNKV|DIRKV|CHUNK|BATCH|WINDOW|SLIDE|STRIDE|EVERY|ROTATE|ROTL|ROTR|FLATTEN|UNCHUNK|TAKE|DROP|SPLIT|WORDS|CUT|CUTALL|COLUMN|SORT|SORTN|SORTLEN|UNIQ|UNION|DISTINCT|INTERSECT|DIFF|ZIP|KEYS|VALS|PREFIXALL|SUFFIXALL|FILL|ENUMERATE|NUMBER|SQUEEZE|COMPACT|TRIMALL|UPPERALL|LOWERALL|MAPREPLACE|GSUBALL|FREQ|HIST|SORTFREQ|BEFOREALL|AFTERALL|MIDLINES|SLICEBAG|REVL|JOINLINES|PUSH|PREPEND|POP|POPHEAD|LINES|HASLINE|COUNTLINE|COUNTMATCH|GREPCOUNT|FINDLINE|SETLINE|SETMATCH|INSERTLINE|DROPNTH|MOVELINE|REMOVELINE|ENV|SETENV|UNSETENV|EXIST|SIZE|ISDIR|ISFILE|MTIME|AGE|MKDIR|BASENAME|DIRNAME|EXTNAME|STEM|WHICH|CWD|CHDIR|STATE|ROOT|TMP|HTTP|SPAWN|JOIN|JSON|CHAT|ARG|NUM|STR|ITOA|LEN|LENALL|MAPLEN|MAXLEN|MINLEN|LONGEST|SHORTEST|COMMONPREFIX|COMMONSUFFIX|STRIPPREFIX|STRIPSUFFIX|STRIPCOMMON|LCP|EMPTY|BLANK|COALESCE|NVL|TIME|MS|SLEEP|RAND|PICK|CHOICE|SHUFFLE|SHUF|DRAWN|SAMPLEK|NPICK|MIN|MAX|ARGMAX|ARGMIN|CLAMP|IN|WITHIN|CMP|SCMP|IABS|SIGN|DIV|MOD|GCD|LCM|POW|ISQRT|SUM|PROD|AVG|MEDIAN|RANGE|SEQ|IOTA|DATE|PID|HOSTNAME|USER|UID|HOME|APPEND|HEX|TOHEX|ORD|CHR|MID|CAT|FIND|FINDI|NTH|EQS|EQSI|HAS|HASI|BEFORE|AFTER|BETWEEN|REVS|UPPER|LOWER|TRIM|STARTS|STARTSI|ENDS|ENDSI|REPLACE|REPLACEALL|LPAD|RPAD|PADALL|LPADALL|RPADALL|TRUNCALL|CLIPALL|STREPEAT");
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
      {"REQUIRE", "REQUIRE VERSION|LIB|ENV|PATH|DIR|REG — fail-fast gates (host path/kind too)"},
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
      {"SYS GLOB", "SYS GLOB|MATCHFILES path [pattern] — basenames matching * ? [] · plate discovery"},
      {"SYS MATCHFILES", "SYS MATCHFILES path pattern — alias of SYS GLOB"},
      {"SYS PATHGLOB", "SYS PATHGLOB|PGLOB path|pattern [pattern] — full paths matching * ? [] · ready for READ/RM"},
      {"SYS PGLOB", "SYS PGLOB alias of SYS PATHGLOB"},
      {"SYS FULLGLOB", "SYS FULLGLOB alias of SYS PATHGLOB"},
      {"SYS NTH", "SYS NTH n [str] — 0-based newline field · pairs with LIST"},
      {"SYS LINE", "SYS LINE n [str] — 1-based newline field"},
      {"SYS HEAD", "SYS HEAD [str] — first newline field"},
      {"SYS TAIL", "SYS TAIL [str] — last newline field"},
      {"SYS GREP", "SYS GREP|FILTER needle [str] — keep newline fields containing needle"},
      {"SYS GREPV", "SYS GREPV|VGREP needle [str] — drop newline fields containing needle"},
      {"SYS GREPI", "SYS GREPI|IGREP|GREP I needle [str] — case-insensitive GREP"},
      {"SYS GREPVI", "SYS GREPVI|VGREPI needle [str] — case-insensitive invert GREP"},
      {"SYS GREPANY", "SYS GREPANY|GREPOR bag n1 [n2…] — keep fields matching any needle"},
      {"SYS GREPOR", "SYS GREPOR bag n1 [n2…] — alias of SYS GREPANY · multi-severity triage"},
      {"SYS GREPANYI", "SYS GREPANYI|GREPORI bag n1 [n2…] — case-insensitive GREPANY"},
      {"SYS GREPALL", "SYS GREPALL|GREPAND bag n1 [n2…] — keep fields matching every needle"},
      {"SYS GREPAND", "SYS GREPAND bag n1 [n2…] — alias of SYS GREPALL · multi-tag AND triage"},
      {"SYS GREPALLI", "SYS GREPALLI|GREPANDI bag n1 [n2…] — case-insensitive GREPALL"},
      {"SYS TAKE", "SYS TAKE|FIRSTN n [str] — first n newline fields · LIST window"},
      {"SYS DROP", "SYS DROP|SKIP n [str] — drop first n newline fields · keep rest"},
      {"SYS SPLIT", "SYS SPLIT|FIELDS sep [str] — sep-split → newline fields · PATH/CSV"},
      {"SYS WORDS", "SYS WORDS|TOKENIZE [str] — whitespace → newline fields · collapse runs"},
      {"SYS TOKENIZE", "SYS TOKENIZE [str] — alias of SYS WORDS"},
      {"SYS SORT", "SYS SORT [str] — lexicographic sort of newline fields · stable LIST"},
      {"SYS SORTN", "SYS SORTN|NSORT [DESC] [str] — numeric sort of newline fields (vs lex SORT)"},
      {"SYS SORTLEN", "SYS SORTLEN|LENSORT [DESC] [bag] — sort fields by string length"},
      {"SYS LENSORT", "SYS LENSORT [DESC] [bag] — alias of SYS SORTLEN"},
      {"SYS NSORT", "SYS NSORT [DESC] [str] — alias of SYS SORTN · score bags / sizes"},
      {"SYS UNIQ", "SYS UNIQ [str] — drop adjacent duplicate fields (sort first)"},
      {"SYS UNION", "SYS UNION|ORLINES a [b…] — merge bags · first-seen unique fields → LAST"},
      {"SYS DISTINCT", "SYS DISTINCT|UNIQUEALL [bag] — order-preserving full unique (vs adjacent UNIQ)"},
      {"SYS INTERSECT", "SYS INTERSECT|ANDLINES a b — fields of a also in b (order of a)"},
      {"SYS DIFF", "SYS DIFF|EXCEPT|SETDIFF a b — fields of a not in b (order of a)"},
      {"SYS ZIP", "SYS ZIP|PAIR a b [sep] — pair bag fields by index with sep (default :)"},
      {"SYS KEYS", "SYS KEYS|COL0 bag [sep] — peel left of first sep each field → bag"},
      {"SYS VALS", "SYS VALS|COL1 bag [sep] — peel right of first sep each field → bag"},
      {"SYS PREFIXALL", "SYS PREFIXALL|MAPPRE bag prefix — prepend string to every field → LAST"},
      {"SYS SUFFIXALL", "SYS SUFFIXALL|MAPSUF bag suffix — append string to every field → LAST"},
      {"SYS FILL", "SYS FILL|REPEATL n value — bag of n copies of value → LAST (cap 256)"},
      {"SYS ENUMERATE", "SYS ENUMERATE|NUMBER bag [start] [sep] — index-prefix each field (default 0:)"},
      {"SYS NUMBER", "SYS NUMBER bag [start] [sep] — alias of SYS ENUMERATE · ranked plates"},
      {"SYS SQUEEZE", "SYS SQUEEZE|COMPACT [BLANK] [bag] — drop empty (or blank) fields → LAST"},
      {"SYS COMPACT", "SYS COMPACT [BLANK] [bag] — alias of SYS SQUEEZE · clean VALS/SPLIT bags"},
      {"SYS TRIMALL", "SYS TRIMALL|MAPTRIM [L|R] [bag] — trim whitespace on every field → LAST"},
      {"SYS MAPTRIM", "SYS MAPTRIM [L|R] [bag] — alias of SYS TRIMALL · clean READ/SPLIT lines"},
      {"SYS UPPERALL", "SYS UPPERALL|MAPUPPER [bag] — ASCII upper every bag field → LAST"},
      {"SYS MAPUPPER", "SYS MAPUPPER [bag] — alias of SYS UPPERALL · normalize before HASLINE"},
      {"SYS LOWERALL", "SYS LOWERALL|MAPLOWER [bag] — ASCII lower every bag field → LAST"},
      {"SYS MAPLOWER", "SYS MAPLOWER [bag] — alias of SYS LOWERALL · case-fold bags"},
      {"SYS MAPREPLACE", "SYS MAPREPLACE|GSUBALL bag old new — REPLACEALL on every field · LAST_N=subs"},
      {"SYS GSUBALL", "SYS GSUBALL bag old new — alias of SYS MAPREPLACE · bag path/tag rewrite"},
      {"SYS REPLACEBAG", "SYS REPLACEBAG bag old new — alias of SYS MAPREPLACE"},
      {"SYS FREQ", "SYS FREQ|HIST [bag] [sep] — field frequency bag key:count · LAST_N=uniques"},
      {"SYS HIST", "SYS HIST [bag] [sep] — alias of SYS FREQ · severity/status rollups"},
      {"SYS COUNTS", "SYS COUNTS [bag] [sep] — alias of SYS FREQ"},
      {"SYS SORTFREQ", "SYS SORTFREQ|SORTBYCOUNT [bag] [sep] [DESC|ASC] — sort FREQ by count"},
      {"SYS SORTBYCOUNT", "SYS SORTBYCOUNT [bag] [sep] [DESC|ASC] — alias of SYS SORTFREQ"},
      {"SYS FSORT", "SYS FSORT [bag] — alias of SYS SORTFREQ · top severities after FREQ"},
      {"SYS BEFOREALL", "SYS BEFOREALL|MAPBEFORE bag needle — BEFORE on every field → bag"},
      {"SYS AFTERALL", "SYS AFTERALL|MAPAFTER bag needle — AFTER on every field → bag"},
      {"SYS MAPBEFORE", "SYS MAPBEFORE bag needle — alias of SYS BEFOREALL · FREQ keys peel"},
      {"SYS MAPAFTER", "SYS MAPAFTER bag needle — alias of SYS AFTERALL · FREQ counts peel"},
      {"SYS FIRSTMATCH", "SYS FIRSTMATCH|GREP1 bag needle — first field containing needle · LAST_N 0|1"},
      {"SYS GREP1", "SYS GREP1 bag needle — alias of SYS FIRSTMATCH · one-line pick without TAKE"},
      {"SYS FIRSTMATCHI", "SYS FIRSTMATCHI|GREP1I bag needle — case-insensitive FIRSTMATCH"},
      {"SYS LOOKUP", "SYS LOOKUP|KVGET bag key [sep] [OR fallback] — peel key:val · miss→fallback OK"},
      {"SYS KVGET", "SYS KVGET bag key [sep] [OR fallback] — alias of SYS LOOKUP · FREQ count peel"},
      {"SYS LOOKUPI", "SYS LOOKUPI|KVGETI bag key [sep] [OR fallback] — case-insensitive LOOKUP"},
      {"SYS GETKV", "SYS GETKV bag key [sep] [OR fallback] — alias of SYS LOOKUP"},
      {"SYS LOOKUPN", "SYS LOOKUPN|KVGETN bag key [sep] [OR n] — peel value as int → LAST_N · FREQ arith"},
      {"SYS KVGETN", "SYS KVGETN bag key [sep] [OR n] — alias of SYS LOOKUPN · count as number"},
      {"SYS GETKVN", "SYS GETKVN bag key [sep] [OR n] — alias of SYS LOOKUPN"},
      {"SYS KVSET", "SYS KVSET|SETKV bag key value [sep] — set/update key:val field · dual of LOOKUP"},
      {"SYS SETKV", "SYS SETKV bag key value [sep] — alias of SYS KVSET · plate kv write"},
      {"SYS DICTSET", "SYS DICTSET bag key value [sep] — alias of SYS KVSET"},
      {"SYS MAPSET", "SYS MAPSET bag key value [sep] — alias of SYS KVSET"},
      {"SYS KVINC", "SYS KVINC|INCKV bag key [delta] [sep] — add delta to key's numeric value"},
      {"SYS INCKV", "SYS INCKV bag key [delta] [sep] — alias of SYS KVINC · counter bump"},
      {"SYS BUMPKV", "SYS BUMPKV bag key [delta] [sep] — alias of SYS KVINC"},
      {"SYS KVADD", "SYS KVADD bag key [delta] [sep] — alias of SYS KVINC"},
      {"SYS KVDEL", "SYS KVDEL|DELKV bag key [sep] — drop first key:val field · dual of KVSET"},
      {"SYS DELKV", "SYS DELKV bag key [sep] — alias of SYS KVDEL · remove FREQ/plate key"},
      {"SYS RMKV", "SYS RMKV bag key [sep] — alias of SYS KVDEL"},
      {"SYS UNSETKV", "SYS UNSETKV bag key [sep] — alias of SYS KVDEL · soft miss OK"},
      {"SYS MERGEKV", "SYS MERGEKV|KVADDALL bag_a bag_b [sep] — sum shared key:val counts · merge FREQ"},
      {"SYS KVADDALL", "SYS KVADDALL bag_a bag_b [sep] — alias of SYS MERGEKV · combine histograms"},
      {"SYS ADDFREQ", "SYS ADDFREQ bag_a bag_b [sep] — alias of SYS MERGEKV"},
      {"SYS SUMKV", "SYS SUMKV|TOTALKV bag [sep] — sum key:val numeric values · FREQ total events"},
      {"SYS TOTALKV", "SYS TOTALKV bag [sep] — alias of SYS SUMKV · histogram grand total"},
      {"SYS SUMVALS", "SYS SUMVALS bag [sep] — alias of SYS SUMKV"},
      {"SYS AVGKV", "SYS AVGKV|MEANKV bag [sep] — integer mean of key:val values · typical FREQ count"},
      {"SYS MEANKV", "SYS MEANKV bag [sep] — alias of SYS AVGKV · mean count across keys"},
      {"SYS MEANVAL", "SYS MEANVAL bag [sep] — alias of SYS AVGKV"},
      {"SYS MEDIANKV", "SYS MEDIANKV|P50KV bag [sep] — integer median of key:val values · robust FREQ mid"},
      {"SYS P50KV", "SYS P50KV bag [sep] — alias of SYS MEDIANKV"},
      {"SYS MIDKV", "SYS MIDKV bag [sep] — alias of SYS MEDIANKV"},
      {"SYS TOPKEY", "SYS TOPKEY|ARGMAXKV bag [sep] — key with max numeric value · dominant FREQ"},
      {"SYS ARGMAXKV", "SYS ARGMAXKV bag [sep] — alias of SYS TOPKEY · LAST=key LAST_N=value"},
      {"SYS BOTKEY", "SYS BOTKEY|ARGMINKV bag [sep] — key with min numeric value · rarest FREQ"},
      {"SYS ARGMINKV", "SYS ARGMINKV bag [sep] — alias of SYS BOTKEY"},
      {"SYS THRESHKV", "SYS THRESHKV|KEEPVAL bag min [sep] — keep key:val with value>=min · denoise FREQ"},
      {"SYS KEEPVAL", "SYS KEEPVAL bag min [sep] — alias of SYS THRESHKV"},
      {"SYS MINCOUNT", "SYS MINCOUNT bag min [sep] — alias of SYS THRESHKV · drop rare counts"},
      {"SYS DROPZERO", "SYS DROPZERO|KEEPNZ bag [sep] — drop value==0 key:val · keep +/− after DIFFKV"},
      {"SYS KEEPNZ", "SYS KEEPNZ bag [sep] — alias of SYS DROPZERO · non-zero deltas only"},
      {"SYS NZKV", "SYS NZKV bag [sep] — alias of SYS DROPZERO"},
      {"SYS KEEPKEY", "SYS KEEPKEY|GREPKEY bag needle [sep] — keep key:val whose key contains needle"},
      {"SYS GREPKEY", "SYS GREPKEY bag needle [sep] — alias of SYS KEEPKEY"},
      {"SYS DROPKEY", "SYS DROPKEY|GREPVKEY bag needle [sep] — drop keys containing needle"},
      {"SYS KEEPKEYI", "SYS KEEPKEYI bag needle [sep] — case-insensitive KEEPKEY"},
      {"SYS PCTKV", "SYS PCTKV|SHAREKV bag [sep] — rewrite values as integer %% of total · FREQ share"},
      {"SYS SHAREKV", "SYS SHAREKV bag [sep] — alias of SYS PCTKV · share-of-total"},
      {"SYS PERCENTKV", "SYS PERCENTKV bag [sep] — alias of SYS PCTKV"},
      {"SYS CAPKV", "SYS CAPKV|CLAMPKV bag max [sep] — clamp key:val values to max · cap FREQ outliers"},
      {"SYS CLAMPKV", "SYS CLAMPKV bag max [sep] — alias of SYS CAPKV"},
      {"SYS MAXVAL", "SYS MAXVAL bag max [sep] — alias of SYS CAPKV · ceiling on counts"},
      {"SYS SCALEKV", "SYS SCALEKV|MULKV bag factor [sep] — multiply key:val values · weight FREQ"},
      {"SYS MULKV", "SYS MULKV bag factor [sep] — alias of SYS SCALEKV"},
      {"SYS WEIGHTKV", "SYS WEIGHTKV bag factor [sep] — alias of SYS SCALEKV · weight before MERGEKV"},
      {"SYS DIVKV", "SYS DIVKV|IDIVKV bag divisor [sep] — integer-divide key:val values · mean after MERGE"},
      {"SYS IDIVKV", "SYS IDIVKV bag divisor [sep] — alias of SYS DIVKV"},
      {"SYS QUOTKV", "SYS QUOTKV bag divisor [sep] — alias of SYS DIVKV"},
      {"SYS DIFFKV", "SYS DIFFKV|SUBKV bag_a bag_b [sep] — subtract key:val values · FREQ delta"},
      {"SYS SUBKV", "SYS SUBKV bag_a bag_b [sep] — alias of SYS DIFFKV · a−b by key"},
      {"SYS DELTAKV", "SYS DELTAKV bag_a bag_b [sep] — alias of SYS DIFFKV · now−baseline"},
      {"SYS ADDKV", "SYS ADDKV|OFFSETKV bag delta [sep] — add delta to key:val values · Laplace/offset"},
      {"SYS OFFSETKV", "SYS OFFSETKV bag delta [sep] — alias of SYS ADDKV · score offset"},
      {"SYS ADDVAL", "SYS ADDVAL bag delta [sep] — alias of SYS ADDKV"},
      {"SYS ABSKV", "SYS ABSKV|MAGKV bag [sep] — absolute key:val values · magnitude after DIFFKV"},
      {"SYS MAGKV", "SYS MAGKV bag [sep] — alias of SYS ABSKV · rank deltas by magnitude"},
      {"SYS ABSALL", "SYS ABSALL bag [sep] — alias of SYS ABSKV"},
      {"SYS SIGNKV", "SYS SIGNKV|DIRKV bag [sep] — map values to −1|0|1 · DIFFKV direction"},
      {"SYS DIRKV", "SYS DIRKV bag [sep] — alias of SYS SIGNKV · trend polarity"},
      {"SYS SGNKV", "SYS SGNKV bag [sep] — alias of SYS SIGNKV"},
      {"SYS LASTMATCH", "SYS LASTMATCH|GREP1L bag needle — last field containing needle · LAST_N 0|1"},
      {"SYS GREP1L", "SYS GREP1L bag needle — alias of SYS LASTMATCH · latest hit without REVL"},
      {"SYS LASTMATCHI", "SYS LASTMATCHI|GREP1LI bag needle — case-insensitive LASTMATCH"},
      {"SYS CHUNK", "SYS CHUNK|BATCH n [bag] [join] — group fields into batches of n → LAST"},
      {"SYS BATCH", "SYS BATCH n [bag] [join] — alias of SYS CHUNK · work-list paging"},
      {"SYS GROUPN", "SYS GROUPN n [bag] [join] — alias of SYS CHUNK"},
      {"SYS WINDOW", "SYS WINDOW|SLIDE n [bag] [join] — overlapping windows of n fields → LAST"},
      {"SYS SLIDE", "SYS SLIDE n [bag] [join] — alias of SYS WINDOW · consecutive pairs/ngrams"},
      {"SYS NGRAM", "SYS NGRAM n [bag] [join] — alias of SYS WINDOW"},
      {"SYS STRIDE", "SYS STRIDE|EVERY step [offset] [bag] — keep index%step==offset fields"},
      {"SYS EVERY", "SYS EVERY step [offset] [bag] — alias of SYS STRIDE · worker partition"},
      {"SYS STEP", "SYS STEP step [offset] [bag] — alias of SYS STRIDE"},
      {"SYS ROTATE", "SYS ROTATE|ROTL k [bag] — left-rotate bag fields by k · round-robin"},
      {"SYS ROTL", "SYS ROTL k [bag] — alias of SYS ROTATE (left)"},
      {"SYS ROTR", "SYS ROTR k [bag] — right-rotate bag fields by k"},
      {"SYS FLATTEN", "SYS FLATTEN|UNCHUNK [bag] [sep] — split every field by sep → flat bag"},
      {"SYS UNCHUNK", "SYS UNCHUNK [bag] [sep] — alias of SYS FLATTEN · reverse CHUNK join"},
      {"SYS SPLITALL", "SYS SPLITALL [bag] [sep] — alias of SYS FLATTEN · map-split bag lines"},
      {"SYS CUTALL", "SYS CUTALL|MAPCUT bag sep n — peel Nth sep-field from every bag line (0-based)"},
      {"SYS MAPCUT", "SYS MAPCUT bag sep n — alias of SYS CUTALL · log columns → FREQ"},
      {"SYS COLALL", "SYS COLALL|COLUMNALL bag sep n — 1-based CUTALL (CSV/path columns)"},
      {"SYS MIDLINES", "SYS MIDLINES|SLICEBAG bag start [end] — keep fields [start..end] 0-based"},
      {"SYS SLICEBAG", "SYS SLICEBAG bag start [end] — alias of SYS MIDLINES · middle bag window"},
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
      {"SYS LENALL", "SYS LENALL|MAPLEN [bag] — length of every field → decimal bag · LENALL_SUM"},
      {"SYS MAPLEN", "SYS MAPLEN [bag] — alias of SYS LENALL · size rollups without EACH+LEN"},
      {"SYS FIELDLENS", "SYS FIELDLENS [bag] — alias of SYS LENALL"},
      {"SYS MAXLEN", "SYS MAXLEN|MAXWIDTH [bag] — max field string length → LAST_N · PADALL width"},
      {"SYS MINLEN", "SYS MINLEN|MINWIDTH [bag] — min field string length → LAST_N"},
      {"SYS MAXWIDTH", "SYS MAXWIDTH [bag] — alias of SYS MAXLEN"},
      {"SYS MINWIDTH", "SYS MINWIDTH [bag] — alias of SYS MINLEN"},
      {"SYS LONGEST", "SYS LONGEST|MAXFIELD [bag] — first longest bag field text → LAST"},
      {"SYS SHORTEST", "SYS SHORTEST|MINFIELD [bag] — first shortest bag field text → LAST"},
      {"SYS MAXFIELD", "SYS MAXFIELD [bag] — alias of SYS LONGEST"},
      {"SYS MINFIELD", "SYS MINFIELD [bag] — alias of SYS SHORTEST"},
      {"SYS COMMONPREFIX", "SYS COMMONPREFIX|LCP [bag] — longest common prefix of fields → LAST"},
      {"SYS LCP", "SYS LCP [bag] — alias of SYS COMMONPREFIX · path/peer roots"},
      {"SYS COMMONSUFFIX", "SYS COMMONSUFFIX|LCS [bag] — longest common suffix of fields → LAST"},
      {"SYS LCS", "SYS LCS [bag] — alias of SYS COMMONSUFFIX · shared ext peel"},
      {"SYS STRIPPREFIX", "SYS STRIPPREFIX bag prefix — drop leading prefix from each field if present"},
      {"SYS STRIPSUFFIX", "SYS STRIPSUFFIX bag suffix — drop trailing suffix from each field if present"},
      {"SYS STRIPCOMMON", "SYS STRIPCOMMON|STRIPLCP [bag] — strip LCP from every field · relative paths"},
      {"SYS STRIPLCP", "SYS STRIPLCP [bag] — alias of SYS STRIPCOMMON"},
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
      {"SYS COUNTMATCH", "SYS COUNTMATCH|GREPCOUNT bag needle — count fields containing needle → LAST_N"},
      {"SYS GREPCOUNT", "SYS GREPCOUNT bag needle — alias of SYS COUNTMATCH · log severity tallies"},
      {"SYS COUNTMATCHI", "SYS COUNTMATCHI|GREPCOUNTI bag needle — case-insensitive COUNTMATCH"},
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
      {"SYS COALESCE", "SYS COALESCE|NVL a b [c…] — first non-empty string → LAST"},
      {"SYS NVL", "SYS NVL a b [c…] — alias of SYS COALESCE · default chain"},
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
      {"SYS PICK", "SYS PICK|CHOICE|SAMPLE [str] — random newline field → LAST · index LAST_N"},
      {"SYS CHOICE", "SYS CHOICE [str] — alias of SYS PICK · sample LIST/RANGE bag"},
      {"SYS SHUFFLE", "SYS SHUFFLE|SHUF [str] — Fisher–Yates shuffle newline fields → LAST"},
      {"SYS SHUF", "SYS SHUF [str] — alias of SYS SHUFFLE · randomize work bags"},
      {"SYS DRAWN", "SYS DRAWN|SAMPLEK k [bag] — sample k unique fields without replacement → bag"},
      {"SYS SAMPLEK", "SYS SAMPLEK k [bag] — alias of SYS DRAWN · multi-peer sample"},
      {"SYS NPICK", "SYS NPICK k [bag] — alias of SYS DRAWN"},
      {"SYS TAKERAND", "SYS TAKERAND k [bag] — alias of SYS DRAWN · random TAKE"},
      {"SYS MIN", "SYS MIN a b [c…]|bag — host-plane minimum → LAST_N · bag like SUM"},
      {"SYS MAX", "SYS MAX a b [c…]|bag — host-plane maximum → LAST_N · after LENALL width"},
      {"SYS MINBAG", "SYS MINBAG [bag] — min of newline numeric fields · alias of bag MIN"},
      {"SYS MAXBAG", "SYS MAXBAG [bag] — max of newline numeric fields · alias of bag MAX"},
      {"SYS ARGMAX", "SYS ARGMAX a b [c…]|bag — 0-based index of first max → LAST_N · LAST=value"},
      {"SYS ARGMIN", "SYS ARGMIN a b [c…]|bag — 0-based index of first min → LAST_N · LAST=value"},
      {"SYS MAXIDX", "SYS MAXIDX [bag] — alias of SYS ARGMAX"},
      {"SYS MINIDX", "SYS MINIDX [bag] — alias of SYS ARGMIN"},
      {"SYS PADALL", "SYS PADALL|RPADALL bag width [pad] — right-pad every field to width"},
      {"SYS LPADALL", "SYS LPADALL bag width [pad] — left-pad every field to width"},
      {"SYS RPADALL", "SYS RPADALL bag width [pad] — alias of right-pad PADALL"},
      {"SYS TRUNCALL", "SYS TRUNCALL|CLIPALL [RIGHT] bag width — truncate every field to width (no pad)"},
      {"SYS CLIPALL", "SYS CLIPALL bag width — alias of SYS TRUNCALL · clip log payloads"},
      {"SYS MAPTRUNC", "SYS MAPTRUNC bag width — alias of SYS TRUNCALL"},
      {"SYS CLAMP", "SYS CLAMP x lo hi — bound x into [lo,hi] → LAST_N"},
      {"SYS IN", "SYS IN|WITHIN x lo hi — inclusive range membership → LAST_N 0|1"},
      {"SYS WITHIN", "SYS WITHIN x lo hi — alias of SYS IN · score/retry bands"},
      {"SYS CMP", "SYS CMP|NCMP a b — three-way numeric compare → LAST_N -1|0|1"},
      {"SYS SCMP", "SYS SCMP|CMPS a b — string compare → LAST_N -1|0|1"},
      {"SYS SCMPI", "SYS SCMPI a b — case-insensitive string compare"},
      {"SYS IABS", "SYS IABS|ABSVAL|NABS x — integer absolute value → LAST_N"},
      {"SYS SIGN", "SYS SIGN|SGN x — signum → LAST_N -1|0|1"},
      {"SYS DIV", "SYS DIV|IDIV|QUOT a b — integer divide (trunc) · /0→0"},
      {"SYS MOD", "SYS MOD|REM a b — remainder · %0→0 soft"},
      {"SYS GCD", "SYS GCD|HCF a b [c…] — greatest common divisor → LAST_N"},
      {"SYS LCM", "SYS LCM a b [c…] — least common multiple → LAST_N"},
      {"SYS POW", "SYS POW|POWER a e — integer a^e (e>=0) → LAST_N"},
      {"SYS ISQRT", "SYS ISQRT|SQRT n — floor integer square root → LAST_N"},
      {"SYS SUM", "SYS SUM|TOTAL a b [c…]|bag — sum ints or newline bag → LAST_N"},
      {"SYS PROD", "SYS PROD|PRODUCT a b [c…]|bag — product of ints → LAST_N"},
      {"SYS AVG", "SYS AVG|MEAN a b [c…]|bag — integer mean (trunc) → LAST_N"},
      {"SYS MEDIAN", "SYS MEDIAN|P50 a b [c…]|bag — integer median (even → lower mid) → LAST_N"},
      {"SYS RANGE", "SYS RANGE lo hi [step] — inclusive int sequence → newline bag"},
      {"SYS SEQ", "SYS SEQ n | lo hi [step] — 1..n or inclusive range → bag"},
      {"SYS IOTA", "SYS IOTA n | lo hi [step] — 0..n-1 (half-open) → bag"},
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
   * REQUIRE ENV|VAR name — fail-fast if host env missing or empty.
   * REQUIRE PATH|DIR|REG path — fail-fast if host path missing / wrong kind.
   * Usability: agents refuse missing stdlib / old runtime / host config / plates without shell glue. */
  if (kw(&L->cur,"REQUIRE")||kw(&L->cur,"NEED")||kw(&L->cur,"REQUIRES")){
    int aln = L->cur.line;
    lex_next(L);
    /* REQUIRE LIB|MODULE|INCLUDE|FILE name — resolve like INCLUDE / cubalc which.
     * Note: FILE here means cubalc module (legacy), not host path — use REQUIRE PATH|REG. */
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
    /* REQUIRE ENV|VAR|ENVVAR|HOSTENV name — getenv must be non-empty.
     * On success: LAST = value, LAST_N = len, REQUIRE_ENV = name, OK=1.
     * Usability: fail-fast host config (CUBALC_STATE/SMX key) without SYS ENV + IF glue. */
    if (kw(&L->cur,"ENV")||kw(&L->cur,"VAR")||kw(&L->cur,"ENVVAR")||
        kw(&L->cur,"HOSTENV")||kw(&L->cur,"ENVIRONMENT")){
      char name[96];
      const char *val;
      lex_next(L);
      if (L->cur.kind != TK_STR && L->cur.kind != TK_IDENT){
        fail(vm, "REQUIRE ENV \"NAME\"");
        return -1;
      }
      snprintf(name, sizeof name, "%s", L->cur.text);
      lex_next(L);
      if (!name[0]) {
        fail(vm, "REQUIRE ENV empty name");
        return -1;
      }
      val = getenv(name);
      if (!val || !val[0]) {
        char msg[160];
        snprintf(msg, sizeof msg,
                 "REQUIRE ENV '%s' missing line %d — set host env or SYS ENV SET",
                 name, aln);
        if (vm->res) vm->res->asserts_fail++;
        fail(vm, msg);
        return -1;
      }
      {
        size_t vl = strlen(val);
        char out[CUBALC_HOST_STR_MAX];
        if (vl >= sizeof out) vl = sizeof out - 1;
        memcpy(out, val, vl);
        out[vl] = 0;
        var_set_str(vm, "LAST", out);
        var_set_str(vm, "ENV", out);
        var_set_str(vm, "REQUIRE_ENV", name);
        snprintf(vm->last_str, sizeof vm->last_str, "%s", out);
        vm->last_n = (long)vl;
        var_set_num(vm, "LAST_N", (long)vl);
        var_set_num(vm, "OK", 1);
        if (vm->trace)
          fprintf(vm->trace, "# require env %s ok (len %zu)\n", name, vl);
        if (vm->res) vm->res->asserts_ok++;
        bump(vm); return 1;
      }
    }
    /* REQUIRE PATH|EXIST path — fail if host path missing.
     * REQUIRE DIR|DIRECTORY path — fail if not a directory.
     * REQUIRE REG|REGFILE|ISFILE path — fail if not a regular file.
     * On success: LAST = path, LAST_N=1, REQUIRE_PATH = path, OK=1.
     * Usability: gate plate/config/dir before READ without SYS EXIST + IF + FAIL. */
    if (kw(&L->cur,"PATH")||kw(&L->cur,"EXIST")||kw(&L->cur,"EXISTS")||
        kw(&L->cur,"PRESENT")||kw(&L->cur,"DIR")||kw(&L->cur,"DIRECTORY")||
        kw(&L->cur,"FOLDER")||kw(&L->cur,"ISDIR")||kw(&L->cur,"REG")||
        kw(&L->cur,"REGFILE")||kw(&L->cur,"REGULAR")||kw(&L->cur,"ISFILE")||
        kw(&L->cur,"PLATE")||kw(&L->cur,"HOSTPATH")){
      char path[512];
      cubalc_host_result hr;
      int want_dir = 0, want_file = 0;
      const char *kind = "PATH";
      if (kw(&L->cur,"DIR")||kw(&L->cur,"DIRECTORY")||kw(&L->cur,"FOLDER")||
          kw(&L->cur,"ISDIR")){
        want_dir = 1;
        kind = "DIR";
      } else if (kw(&L->cur,"REG")||kw(&L->cur,"REGFILE")||kw(&L->cur,"REGULAR")||
                 kw(&L->cur,"ISFILE")||kw(&L->cur,"PLATE")){
        want_file = 1;
        kind = "REG";
      }
      lex_next(L);
      if (resolve_str_arg(vm, L, path, sizeof path) != 0) {
        fail(vm, "REQUIRE PATH|DIR|REG \"path\"|var|LAST");
        return -1;
      }
      if (!path[0]) {
        fail(vm, "REQUIRE PATH empty path");
        return -1;
      }
      if (want_dir || want_file) {
        if (cubalc_host_path_kind(path, &hr) != 0 ||
            (want_dir && hr.code != 2) ||
            (want_file && hr.code != 1)) {
          char msg[200];
          const char *why = (!hr.ok && hr.err[0]) ? hr.err
              : (want_dir ? "not a directory" : "not a regular file");
          snprintf(msg, sizeof msg,
                   "REQUIRE %s '%s' failed line %d — %s",
                   kind, path, aln, why);
          if (vm->res) vm->res->asserts_fail++;
          fail(vm, msg);
          return -1;
        }
      } else {
        if (!cubalc_host_exists(path)) {
          char msg[200];
          snprintf(msg, sizeof msg,
                   "REQUIRE PATH '%s' missing line %d — create path or fix layout",
                   path, aln);
          if (vm->res) vm->res->asserts_fail++;
          fail(vm, msg);
          return -1;
        }
      }
      var_set_str(vm, "LAST", path);
      var_set_str(vm, "REQUIRE_PATH", path);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
      vm->last_n = 1;
      var_set_num(vm, "LAST_N", 1);
      var_set_num(vm, "OK", 1);
      if (want_dir) {
        var_set_num(vm, "ISDIR", 1);
        var_set_str(vm, "REQUIRE_DIR", path);
      }
      if (want_file) {
        var_set_num(vm, "ISFILE", 1);
        var_set_str(vm, "REQUIRE_REG", path);
      }
      if (vm->trace)
        fprintf(vm->trace, "# require %s %s ok\n", kind, path);
      if (vm->res) vm->res->asserts_ok++;
      bump(vm); return 1;
    }
    if (!kw(&L->cur,"VERSION") && !kw(&L->cur,"VER") && !kw(&L->cur,"LANG") &&
        !kw(&L->cur,"CUBALC") && L->cur.kind != TK_STR){
      fail(vm, "REQUIRE VERSION|LIB|ENV|PATH|DIR|REG …");
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
