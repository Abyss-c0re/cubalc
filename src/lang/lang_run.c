/* CubalC lang — lang_run.c (COP/flow · pure C · cube is SoT) */
#include "lang/cubalc_lang_internal.h"
#include <ctype.h>

/* Parse "line N" from err/last_err and copy that source line into out->err_src. */
static void fill_err_src(cubalc_run_result *out, const char *src, size_t n) {
  const char *msg;
  const char *p;
  int line = 0, cur;
  size_t i, start, len, lead;
  if (!out || !src) return;
  msg = out->err[0] ? out->err : out->last_err;
  if (!msg || !msg[0]) return;
  p = strstr(msg, "line ");
  if (!p) p = strstr(msg, "Line ");
  if (!p) return;
  p += 5;
  while (*p == ' ') p++;
  if (!isdigit((unsigned char)*p)) return;
  line = 0;
  while (isdigit((unsigned char)*p)) {
    line = line * 10 + (*p - '0');
    p++;
  }
  if (line < 1) return;
  out->err_line = line;
  cur = 1;
  start = 0;
  for (i = 0; i < n && cur < line; i++) {
    if (src[i] == '\n') {
      cur++;
      start = i + 1;
    }
  }
  if (cur != line) return;
  len = 0;
  while (start + len < n && src[start + len] != '\n' && src[start + len] != '\r' &&
         len + 1 < sizeof out->err_src)
    len++;
  /* trim leading whitespace for plate readability */
  lead = 0;
  while (lead < len && (src[start + lead] == ' ' || src[start + lead] == '\t'))
    lead++;
  len -= lead;
  start += lead;
  if (len >= sizeof out->err_src) len = sizeof out->err_src - 1;
  if (len > 0) memcpy(out->err_src, src + start, len);
  out->err_src[len] = 0;
  /* strip trailing spaces */
  while (len > 0 && (out->err_src[len - 1] == ' ' || out->err_src[len - 1] == '\t')) {
    out->err_src[--len] = 0;
  }
}

int cubalc_lang_exec_stmts_until(VM *vm, Lex *L, const char *stop1, const char *stop2){
  while (!vm->fatal && !vm->halt){
    skip_nl(L);
    if (L->cur.kind==TK_EOF) break;
    if (stop1 && kw(&L->cur,stop1)) break;
    if (stop2 && kw(&L->cur,stop2)) break;
    if (vm->break_loop || vm->continue_loop || vm->return_fn || vm->halt) break;
    int r=parse_form(vm,L);
    if (r<0) return -1;
    if (r==0) break;
    if (vm->break_loop || vm->continue_loop || vm->return_fn || vm->halt) break;
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
  while (!vm.fatal && !vm.halt && L.cur.kind != TK_EOF){
    skip_nl(&L);
    if (L.cur.kind==TK_EOF) break;
    if (parse_form(&vm, &L) < 0) break;
  }
  if (vm.ch.n_cubes>0) cubalc_chain_tick(&vm.ch);

  if (out){
    out->halted = vm.halt ? 1 : 0;
    out->exit_code = vm.exit_code;
    /* EXIT n: non-zero fails plate; clean EXIT 0 stays ok if no asserts_fail/fatal */
    out->ok = !vm.fatal && out->asserts_fail==0 && !(vm.halt && vm.exit_code != 0);
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
    /* Usability: attach source line text when err mentions "line N". */
    if (out->err[0] || out->last_err[0])
      fill_err_src(out, src, n);
  }
  if (vm.ch.n_cubes>0){
    /* Cube Law: share state_matrix only · devices free · united visual faces */
    cubalc_chain_publish_united(&vm.ch);
  }
  /* Prefer EXIT code for process rc when halted with non-zero. */
  if (vm.halt && vm.exit_code != 0) {
    int ec = vm.exit_code;
    if (ec < 0) ec = 1;
    if (ec > 125) ec = 1;
    return ec;
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
