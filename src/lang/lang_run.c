/* CubalC lang — lang_run.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"

int cubalc_lang_exec_stmts_until(VM *vm, Lex *L, const char *stop1, const char *stop2){
  while (!vm->fatal){
    skip_nl(L);
    if (L->cur.kind==TK_EOF) break;
    if (stop1 && kw(&L->cur,stop1)) break;
    if (stop2 && kw(&L->cur,stop2)) break;
    if (vm->break_loop || vm->continue_loop || vm->return_fn) break;
    int r=parse_form(vm,L);
    if (r<0) return -1;
    if (r==0) break;
    if (vm->break_loop || vm->continue_loop || vm->return_fn) break;
  }
  return vm->fatal ? -1 : 0;
}

static int run_source_inner(const char *src, size_t n, const char *name,
                            cubalc_run_result *out, FILE *trace){
  VM vm; memset(&vm,0,sizeof vm);
  vm.res=out; vm.trace=trace; vm.hold_flash=1;
  snprintf(vm.creed,sizeof vm.creed,"%s",CUBALC_CREED);
  cubalc_async_init(0);
  cubalc_chain_init(&vm.ch);
  vm.last_str[0]=0; vm.last_code=0; vm.last_n=0;
  vm.sp=0;
  {
    const char *se = getenv("CUBALC_SEED");
    if (se && se[0]) vm.rng = (uint32_t)strtoul(se, NULL, 0);
    else vm.rng = (uint32_t)time(NULL) ^ 0xC3C3C3C3u;
    if (!vm.rng) vm.rng = 1;
  }
  vm.ch.hold_flash=1;
  snprintf(vm.ch.creed,sizeof vm.ch.creed,"%s",CUBALC_CREED);
  if (out){ memset(out,0,sizeof*out); out->ok=1; }
  if (name && name[0]){
    const char *sl = cubalc_path_slash(name);
    if (sl){
      size_t nbase = (size_t)(sl - name);
      if (nbase >= sizeof vm.include_base) nbase = sizeof vm.include_base - 1;
      memcpy(vm.include_base, name, nbase);
      vm.include_base[nbase] = 0;
    } else vm.include_base[0]=0;
  }

  Lex L; lex_init(&L, src, n);
  while (!vm.fatal && L.cur.kind != TK_EOF){
    skip_nl(&L);
    if (L.cur.kind==TK_EOF) break;
    if (parse_form(&vm, &L) < 0) break;
  }
  if (vm.ch.n_cubes>0) cubalc_chain_tick(&vm.ch);

  if (out){
    out->ok = !vm.fatal && out->asserts_fail==0;
    out->n_cubes = vm.ch.n_cubes;
    out->unity = vm.ch.unity;
    if (vm.fatal && !out->err[0]) snprintf(out->err,sizeof out->err,"%s",vm.err);
    /* Usability: surface sticky LAST_ERR/ERR on plate even when run ok
     * (soft FAIL/EXPECT probes leave agent-readable reason). */
    {
      Var *le = var_get(&vm, "LAST_ERR", 0);
      if (!le || !le->is_str || !le->sval[0])
        le = var_get(&vm, "ERR", 0);
      if (le && le->is_str && le->sval[0])
        snprintf(out->last_err, sizeof out->last_err, "%s", le->sval);
      else if (out->err[0] && !out->last_err[0])
        snprintf(out->last_err, sizeof out->last_err, "%s", out->err);
    }
  }
  if (vm.ch.n_cubes>0){
    /* Cube Law: share state_matrix only · devices free · united visual faces */
    cubalc_chain_publish_united(&vm.ch);
  }
  return out && out->ok ? 0 : 1;
}

int cubalc_run_source(const char *src, size_t n, const char *name,
                      cubalc_run_result *out, FILE *trace){
  if (!src) return 2;
  return run_source_inner(src, n, name, out, trace);
}

int cubalc_run_file(const char *path, cubalc_run_result *out, FILE *trace){
  FILE *f=fopen(path,"rb");
  if (!f){
    if (out){ memset(out,0,sizeof*out); snprintf(out->err,sizeof out->err,"cannot open %s",path); }
    return 2;
  }
  fseek(f,0,SEEK_END); long sz=ftell(f); fseek(f,0,SEEK_SET);
  if (sz<0 || sz>CUBALC_MAX_SRC){ fclose(f); return 2; }
  char *buf=malloc((size_t)sz+1); if(!buf){ fclose(f); return 2; }
  size_t nr=fread(buf,1,(size_t)sz,f); fclose(f); buf[nr]=0;
  int rc=cubalc_run_source(buf,nr,path,out,trace);
  free(buf); return rc;
}
