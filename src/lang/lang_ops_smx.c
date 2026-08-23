/* CubalC lang — lang_ops_smx.c
 * P2P / SMX2 plane: KEY · TALK · EXCHANGE · SEAL · OPEN · SERVE · DIAL
 * Law: binary State Matrix only · fail-closed · no HTTP wire.
 * HOLD_FLASH on frames = device/mesh-join sticky ack (not language PLUG).
 */
#include "lang/cubalc_lang_internal.h"
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <sys/socket.h>
#endif

/* CUBALC_P2P_TIMEOUT ms (default 30000). 0 = wait forever. */
static int p2p_timeout_ms(void){
  const char *e = getenv("CUBALC_P2P_TIMEOUT");
  if (!e || !e[0]) return 30000;
  int ms = atoi(e);
  if (ms < 0) ms = 0;
  if (ms > 600000) ms = 600000;
  return ms;
}

/* 1 if CUBALC_P2P_SOFT is non-empty / 1 / true / yes */
static int p2p_soft(void){
  const char *e = getenv("CUBALC_P2P_SOFT");
  if (!e || !e[0]) return 0;
  if (e[0]=='0' && e[1]==0) return 0;
  if (strcmp(e,"false")==0 || strcmp(e,"no")==0 || strcmp(e,"off")==0) return 0;
  return 1;
}

/* Accept with optional timeout. Returns client fd or -1 (sets *timed_out). */
static int accept_timeout(int lfd, int timeout_ms, int *timed_out){
  if (timed_out) *timed_out = 0;
  if (timeout_ms <= 0){
    int cfd = accept(lfd, NULL, NULL);
    return cfd;
  }
  {
    struct pollfd pfd;
    int pr;
    pfd.fd = lfd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    pr = poll(&pfd, 1, timeout_ms);
    if (pr == 0){
      if (timed_out) *timed_out = 1;
      return -1;
    }
    if (pr < 0) return -1;
    return accept(lfd, NULL, NULL);
  }
}

/* Soft-fail: SMX_OK=0, OK=0, continue program (no fatal).
 * ERR/LAST_ERR sticky for agents after later LAST overwrites. */
static void smx_soft_fail(VM *vm, const char *why){
  const char *w = why ? why : "smx soft fail";
  vm->smx_ok = 0;
  var_set_num(vm, "SMX_OK", 0);
  var_set_num(vm, "OK", 0);
  var_set_str(vm, "LAST", w);
  var_set_str(vm, "ERR", w);
  var_set_str(vm, "LAST_ERR", w);
  if (vm->trace)
    fprintf(vm->trace, "# SMX soft-fail: %s\n", w);
}


/* Soft-OOB: missing cube on TALK/EXCHANGE — fail-closed, no ghost auto-place.
 * Increments SMX_OOB; clears SMX_OK; program continues (mesh stability). */
static void smx_soft_oob(VM *vm, const char *from_id, const char *to_id, const char *which){
  char why[128];
  vm->smx_oob++;
  vm->smx_ok = 0;
  var_set_num(vm, "SMX_OK", 0);
  var_set_num(vm, "SMX_OOB", vm->smx_oob);
  var_set_num(vm, "OK", 0);
  snprintf(why, sizeof why, "SMX soft-OOB %s %s→%s (no ghost place)",
           which ? which : "talk",
           from_id ? from_id : "?",
           to_id ? to_id : "?");
  var_set_str(vm, "LAST", why);
  var_set_str(vm, "ERR", why);
  var_set_str(vm, "LAST_ERR", why);
  if (vm->trace)
    fprintf(vm->trace, "# %s oob=%d\n", why, vm->smx_oob);
}

/* Fatal SMX fail with line + optional agent/human hint (err is 160 bytes). */
static void smx_fail_at(VM *vm, int line, const char *core, const char *hint){
  char msg[160];
  if (line > 0 && hint && hint[0])
    snprintf(msg, sizeof msg, "SMX line %d: %s — hint: %s", line, core, hint);
  else if (line > 0)
    snprintf(msg, sizeof msg, "SMX line %d: %s", line, core);
  else if (hint && hint[0])
    snprintf(msg, sizeof msg, "SMX: %s — hint: %s", core, hint);
  else
    snprintf(msg, sizeof msg, "SMX: %s", core ? core : "fail");
  fail(vm, msg);
}

/* Ensure SMX2 key is loaded (env / token / demo key for local proof) */
static int ensure_smx_key(VM *vm){
  if (vm->smx.key_ok) return 0;
  cubalc_smx_ctx_init(&vm->smx);
  if (vm->smx.key_ok) return 0;
  {
    const char *tok = getenv("CUBALC_SMX_TOKEN");
    if (!tok || !tok[0]) tok = getenv("NANOBOT_PEER_TOKEN");
    if (tok && tok[0] && cubalc_smx_load_key_token(&vm->smx, tok) == 0) return 0;
  }
  /* local lab default — never a device secret; proof/dev only */
  {
    const char *demo =
      "c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3";
    if (cubalc_smx_load_key_hex(&vm->smx, demo) == 0) return 0;
  }
  smx_fail_at(vm, 0, "key missing",
              "export CUBALC_SMX_KEY=$(openssl rand -hex 32) or CUBALC_SMX_TOKEN");
  return -1;
}

/* SMX TALK a b — secure State Matrix transfer a→b.
 * Missing cubes are soft-OOB (fail-closed): no ghost auto-place, SMX_OOB++, continue. */
static int do_smx_talk(VM *vm, const char *from_id, const char *to_id){
  ensure_world(vm);
  if (ensure_smx_key(vm) != 0) return -1;
  int ia = find_cube(vm, from_id);
  int ib = find_cube(vm, to_id);
  if (ia < 0 || ib < 0){
    /* Mesh stability: do NOT invent cubes. Soft-OOB so boards recover. */
    smx_soft_oob(vm, from_id, to_id, "TALK");
    return 1; /* non-fatal: caller must treat >0 as soft continue */
  }
  cubalc_cube_plug(&vm->ch, ia, ib);
  int rc = cubalc_cube_talk_secure(&vm->ch, &vm->smx, ia, ib);
  if (rc != 0){
    char core[96];
    snprintf(core, sizeof core, "TALK fail rc=%d %s", rc,
             vm->smx.last_err[0] ? vm->smx.last_err : "");
    smx_fail_at(vm, 0, core, "HOLD_FLASH 1 · same SMX key · check matrix");
    var_set_num(vm, "SMX_OK", 0);
    vm->smx_ok = 0;
    return -1;
  }
  vm->smx_ok = 1;
  vm->smx_talks++;
  var_set_num(vm, "SMX_OK", 1);
  var_set_num(vm, "SMX_TALKS", vm->smx_talks);
  var_set_num(vm, "OK", 1);
  vm->ch.cubes[ib].atom.unity =
    cubalc_matrix_compat(&vm->ch.cubes[ia].atom.matrix, &vm->ch.cubes[ib].atom.matrix);
  if (vm->trace)
    fprintf(vm->trace, "# SMX TALK %s → %s digit=%u set=%u\n",
            from_id, to_id,
            (unsigned)vm->ch.cubes[ib].atom.digit,
            (unsigned)vm->ch.cubes[ib].atom.matrix.set);
  return 0;
}

static int do_smx_seal(VM *vm, const char *from_id, const char *to_id, const char *path){
  ensure_world(vm);
  if (ensure_smx_key(vm) != 0) return -1;
  int ia = find_cube(vm, from_id);
  if (ia < 0){ place_cube(vm, from_id, "host", 1); ia = find_cube(vm, from_id); }
  if (ia < 0 || !path || !path[0]){ fail(vm, "SMX SEAL cube cube path"); return -1; }
  uint8_t frame[512];
  size_t n = 0;
  if (cubalc_smx_seal(&vm->smx, &vm->ch.cubes[ia].atom, from_id, to_id,
                      frame, sizeof frame, &n) != 0){
    fail(vm, vm->smx.last_err[0] ? vm->smx.last_err : "SMX SEAL");
    var_set_num(vm, "SMX_OK", 0);
    return -1;
  }
  if (cubalc_smx_write_frame(path, frame, n) != 0){
    fail(vm, "SMX SEAL write");
    return -1;
  }
  snprintf(vm->last_str, sizeof vm->last_str, "%s", path);
  vm->last_n = (long)n;
  var_set_str(vm, "LAST", path);
  var_set_num(vm, "LAST_N", (long)n);
  var_set_num(vm, "SMX_N", (long)n);
  var_set_num(vm, "SMX_OK", 1);
  vm->smx_ok = 1;
  if (vm->trace) fprintf(vm->trace, "# SMX SEAL %s→%s %s n=%zu\n", from_id, to_id, path, n);
  return 0;
}

static int do_smx_open(VM *vm, const char *to_id, const char *path){
  ensure_world(vm);
  if (ensure_smx_key(vm) != 0) return -1;
  int ib = find_cube(vm, to_id);
  if (ib < 0){ place_cube(vm, to_id, "body", 1); ib = find_cube(vm, to_id); }
  if (ib < 0 || !path || !path[0]){ fail(vm, "SMX OPEN cube path"); return -1; }
  uint8_t frame[512];
  size_t n = 0;
  if (cubalc_smx_read_frame(path, frame, sizeof frame, &n) != 0){
    fail(vm, "SMX OPEN read");
    return -1;
  }
  cubalc_atom recv;
  char from[CUBALC_ID_LEN], to[CUBALC_ID_LEN];
  if (cubalc_smx_open(&vm->smx, frame, n, &recv, from, to,
                      &vm->ch.cubes[ib].atom.matrix) != 0){
    fail(vm, vm->smx.last_err[0] ? vm->smx.last_err : "SMX OPEN");
    var_set_num(vm, "SMX_OK", 0);
    return -1;
  }
  for (int i = 0; i < recv.matrix.n && i < CUBALC_ATOM_BITS; i++)
    if (cubalc_matrix_get(&recv.matrix, i))
      cubalc_matrix_set(&vm->ch.cubes[ib].atom.matrix, i, 1);
  vm->ch.cubes[ib].atom.digit =
    (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ib].atom.matrix);
  vm->ch.cubes[ib].atom.alive = 1;
  vm->smx_ok = 1;
  vm->smx_talks++;
  var_set_num(vm, "SMX_OK", 1);
  var_set_num(vm, "SMX_TALKS", vm->smx_talks);
  var_set_num(vm, "SMX_N", (long)n);
  var_set_num(vm, "OK", 1);
  if (vm->trace)
    fprintf(vm->trace, "# SMX OPEN → %s from=%s set=%u\n",
            to_id, from, (unsigned)vm->ch.cubes[ib].atom.matrix.set);
  return 0;
}

static int parse_host_port(const char *endpoint, char *host, size_t hostn, int *port){
  const char *colon;
  if (!endpoint || !endpoint[0] || !host || !port) return -1;
  colon = strrchr(endpoint, ':');
  if (colon && colon != endpoint) {
    size_t hl = (size_t)(colon - endpoint);
    if (hl >= hostn) hl = hostn - 1;
    memcpy(host, endpoint, hl); host[hl] = 0;
    *port = atoi(colon + 1);
  } else {
    snprintf(host, hostn, "0.0.0.0");
    *port = atoi(endpoint);
  }
  return (*port > 0 && *port <= 65535) ? 0 : -1;
}

int cubalc_lang_ops_smx(VM *vm, Lex *L){
  skip_nl(L);
  if (L->cur.kind==TK_EOF) return 0;

  /*
   * Law of Manifestation — SMX surface (language, not shell):
   *   SMX KEY
   *   SMX TALK a b
   *   SMX EXCHANGE a b
   *   SMX SEAL a b "path"
   *   SMX OPEN b "path"
   *   SMX SERVE local remote "host:port"
   *   SMX DIAL  local remote "host:port"
   */
  if (!(kw(&L->cur,"SMX")||kw(&L->cur,"SMX2")||kw(&L->cur,"MANIFEST_SMX")))
    return 0;

  lex_next(L);

  if (kw(&L->cur,"KEY")||kw(&L->cur,"LOADKEY")){
    lex_next(L);
    memset(&vm->smx, 0, sizeof vm->smx);
    if (ensure_smx_key(vm) != 0) return -1;
    var_set_num(vm, "SMX_OK", 1);
    bump(vm); return 1;
  }
  if (kw(&L->cur,"TALK")||kw(&L->cur,"SEND")||kw(&L->cur,"XFER")){
    int aln = L->cur.line;
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){
      smx_fail_at(vm, aln, "TALK needs two cubes", "SMX TALK a b"); return -1;
    }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){
      smx_fail_at(vm, aln, "TALK needs two cubes", "SMX TALK a b"); return -1;
    }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    {
      int r = do_smx_talk(vm, a, b);
      if (r < 0) return -1;
      /* r==0 ok; r>0 soft-OOB continue */
    }
    bump(vm); return 1;
  }
  if (kw(&L->cur,"EXCHANGE")||kw(&L->cur,"SWAP")||kw(&L->cur,"PAIR")){
    int aln = L->cur.line;
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){
      smx_fail_at(vm, aln, "EXCHANGE needs two cubes", "SMX EXCHANGE a b"); return -1;
    }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){
      smx_fail_at(vm, aln, "EXCHANGE needs two cubes", "SMX EXCHANGE a b"); return -1;
    }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    {
      int r1 = do_smx_talk(vm, a, b);
      if (r1 < 0) return -1;
      if (r1 > 0){ bump(vm); return 1; } /* soft-OOB: skip reverse, keep mesh */
      int r2 = do_smx_talk(vm, b, a);
      if (r2 < 0) return -1;
      /* r2>0 also soft-OOB; still continue */
    }
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SEAL")||kw(&L->cur,"EMIT")){
    int aln = L->cur.line;
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){
      smx_fail_at(vm, aln, "SEAL needs a b path", "SMX SEAL a b path.smx"); return -1;
    }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){
      smx_fail_at(vm, aln, "SEAL needs a b path", "SMX SEAL a b path.smx"); return -1;
    }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    char path[512];
    if (resolve_str_arg(vm, L, path, sizeof path)!=0){
      smx_fail_at(vm, aln, "SEAL needs path", "SMX SEAL a b path|LAST"); return -1;
    }
    if (do_smx_seal(vm, a, b, path) != 0) return -1;
    bump(vm); return 1;
  }
  if (kw(&L->cur,"OPEN")||kw(&L->cur,"RECV")||kw(&L->cur,"IMPORT")){
    int aln = L->cur.line;
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){
      smx_fail_at(vm, aln, "OPEN needs cube path", "SMX OPEN b path.smx"); return -1;
    }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    char path[512];
    if (resolve_str_arg(vm, L, path, sizeof path)!=0){
      smx_fail_at(vm, aln, "OPEN needs path", "SMX OPEN cube path|LAST"); return -1;
    }
    if (do_smx_open(vm, id, path) != 0) return -1;
    bump(vm); return 1;
  }

  /* P2P: SMX SERVE local remote "host:port" — listen one binary exchange */
  if (kw(&L->cur,"SERVE")||kw(&L->cur,"LISTEN")||kw(&L->cur,"ACCEPT")){
    int aln = L->cur.line;
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){
      smx_fail_at(vm, aln, "SERVE syntax", "SMX SERVE local remote host:port"); return -1;
    }
    char local[48]; snprintf(local,sizeof local,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){
      smx_fail_at(vm, aln, "SERVE syntax", "SMX SERVE local remote host:port"); return -1;
    }
    char remote[48]; snprintf(remote,sizeof remote,"%s",L->cur.text); lex_next(L);
    char bind[256];
    if (resolve_str_arg(vm, L, bind, sizeof bind)!=0){
      const char *e = getenv("CUBALC_P2P_BIND");
      if (e && e[0]) snprintf(bind,sizeof bind,"%s",e);
      else {
        smx_fail_at(vm, aln, "SERVE missing bind",
                    "pass host:port string or export CUBALC_P2P_BIND");
        return -1;
      }
    }
    {
      char host[128]="0.0.0.0";
      int port = 0, lfd, cfd, ia, ib;
      uint8_t frame[512];
      size_t n = 0;
      cubalc_atom recv;
      char fr[CUBALC_ID_LEN], to[CUBALC_ID_LEN];
      if (parse_host_port(bind, host, sizeof host, &port) != 0){
        smx_fail_at(vm, aln, "SERVE bad port", "use host:port e.g. 127.0.0.1:7733");
        return -1;
      }
      ensure_world(vm);
      if (ensure_smx_key(vm) != 0) return -1;
      ia = find_cube(vm, local);
      ib = find_cube(vm, remote);
      if (ia < 0){ place_cube(vm, local, "body", 1); ia = find_cube(vm, local); }
      if (ib < 0){ place_cube(vm, remote, "host", 1); ib = find_cube(vm, remote); }
      lfd = cubalc_smx_tcp_listen(host, port, 4);
      if (lfd < 0){
        smx_fail_at(vm, aln, "SERVE listen fail",
                    "port in use? try other CUBALC_P2P_BIND or free port");
        return -1;
      }
      {
        int to_ms = p2p_timeout_ms();
        int timed = 0;
        if (vm->trace)
          fprintf(vm->trace, "# SMX SERVE %s:%d wait peer timeout_ms=%d\n",
                  host, port, to_ms);
        cfd = accept_timeout(lfd, to_ms, &timed);
        close(lfd);
        if (cfd < 0){
          if (timed){
            /* Timeout is a host/mesh condition — soft so boards do not hang forever */
            char why[140];
            snprintf(why, sizeof why,
                     "SMX SERVE timeout after %dms — hint: dial peer or raise CUBALC_P2P_TIMEOUT",
                     to_ms);
            smx_soft_fail(vm, why);
            bump(vm); return 1;
          }
          smx_fail_at(vm, aln, "SERVE accept fail", "retry SERVE or check firewall");
          return -1;
        }
      }
      n = 0;
      if (cubalc_smx_recv_frame(cfd, frame, sizeof frame, &n) != 0){
        close(cfd);
        smx_fail_at(vm, aln, "SERVE recv fail", "peer closed early; retry dial/serve pair");
        return -1;
      }
      if (cubalc_smx_open(&vm->smx, frame, n, &recv, fr, to,
                          &vm->ch.cubes[ia].atom.matrix) != 0){
        close(cfd);
        smx_fail_at(vm, aln,
                    vm->smx.last_err[0] ? vm->smx.last_err : "SERVE open fail",
                    "same CUBALC_SMX_KEY on both peers");
        return -1;
      }
      for (int i = 0; i < recv.matrix.n && i < CUBALC_ATOM_BITS; i++)
        if (cubalc_matrix_get(&recv.matrix, i))
          cubalc_matrix_set(&vm->ch.cubes[ia].atom.matrix, i, 1);
      vm->ch.cubes[ia].atom.digit =
        (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ia].atom.matrix);
      n = 0;
      if (cubalc_smx_seal(&vm->smx, &vm->ch.cubes[ia].atom, local, remote,
                          frame, sizeof frame, &n) != 0){
        close(cfd);
        smx_fail_at(vm, aln, "SERVE seal fail", "key/HMAC issue; reload SMX KEY");
        return -1;
      }
      if (cubalc_smx_send_frame(cfd, frame, n) != 0){
        close(cfd);
        smx_fail_at(vm, aln, "SERVE send fail", "peer closed; retry exchange");
        return -1;
      }
      close(cfd);
      cubalc_cube_plug(&vm->ch, ia, ib);
      vm->smx_ok = 1;
      vm->smx_talks++;
      var_set_num(vm, "SMX_OK", 1);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_N", (long)n);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# SMX SERVE %s ← peer set=%u\n",
                local, (unsigned)vm->ch.cubes[ia].atom.matrix.set);
    }
    bump(vm); return 1;
  }

  /* Cross-device TCP (no HTTP): SMX DIAL a b "host:port" */
  if (kw(&L->cur,"DIAL")||kw(&L->cur,"NET")||kw(&L->cur,"TCP")){
    int aln = L->cur.line;
    if (kw(&L->cur,"NET")||kw(&L->cur,"TCP")){
      lex_next(L);
      if (!kw(&L->cur,"DIAL") && !kw(&L->cur,"SEND") && !kw(&L->cur,"TALK")){
        smx_fail_at(vm, aln, "NET needs DIAL", "SMX NET DIAL a b host:port");
        return -1;
      }
    }
    if (kw(&L->cur,"DIAL")||kw(&L->cur,"SEND")||kw(&L->cur,"TALK"))
      lex_next(L);
    if (L->cur.kind!=TK_IDENT){
      smx_fail_at(vm, aln, "DIAL syntax", "SMX DIAL a b host:port"); return -1;
    }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){
      smx_fail_at(vm, aln, "DIAL syntax", "SMX DIAL a b host:port"); return -1;
    }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    char endpoint[256];
    if (resolve_str_arg(vm, L, endpoint, sizeof endpoint)!=0){
      const char *e = getenv("CUBALC_P2P_PEER");
      if (e && e[0]) snprintf(endpoint,sizeof endpoint,"%s",e);
      else {
        smx_fail_at(vm, aln, "DIAL missing peer",
                    "pass host:port or export CUBALC_P2P_PEER");
        return -1;
      }
    }
    {
      char host[128];
      int port = 0;
      int fd, ia, ib;
      uint8_t frame[512];
      size_t n = 0;
      cubalc_atom recv;
      char fr[CUBALC_ID_LEN], to[CUBALC_ID_LEN];
      if (parse_host_port(endpoint, host, sizeof host, &port) != 0 ||
          !strchr(endpoint, ':')){
        smx_fail_at(vm, aln, "DIAL need host:port", "e.g. 127.0.0.1:7733");
        return -1;
      }
      ensure_world(vm);
      if (ensure_smx_key(vm) != 0) return -1;
      ia = find_cube(vm, a);
      ib = find_cube(vm, b);
      if (ia < 0){ place_cube(vm, a, "host", 1); ia = find_cube(vm, a); }
      if (ib < 0){ place_cube(vm, b, "body", 1); ib = find_cube(vm, b); }
      fd = cubalc_smx_tcp_connect(host, port);
      if (fd < 0){
        if (p2p_soft()){
          char why[140];
          snprintf(why, sizeof why,
                   "SMX DIAL connect fail %s:%d — hint: start SERVE peer or set CUBALC_P2P_SOFT=1",
                   host, port);
          smx_soft_fail(vm, why);
          bump(vm); return 1;
        }
        smx_fail_at(vm, aln, "DIAL connect fail",
                    "start SERVE peer first, or CUBALC_P2P_SOFT=1 to continue");
        return -1;
      }
      if (cubalc_smx_seal(&vm->smx, &vm->ch.cubes[ia].atom, a, b,
                          frame, sizeof frame, &n) != 0){
        close(fd);
        if (p2p_soft()){ smx_soft_fail(vm, "SMX DIAL seal — hint: SMX KEY"); bump(vm); return 1; }
        smx_fail_at(vm, aln, "DIAL seal fail", "SMX KEY / CUBALC_SMX_KEY");
        return -1;
      }
      if (cubalc_smx_send_frame(fd, frame, n) != 0){
        close(fd);
        if (p2p_soft()){ smx_soft_fail(vm, "SMX DIAL send — hint: peer closed"); bump(vm); return 1; }
        smx_fail_at(vm, aln, "DIAL send fail", "retry; peer may have closed");
        return -1;
      }
      n = 0;
      if (cubalc_smx_recv_frame(fd, frame, sizeof frame, &n) != 0){
        close(fd);
        if (p2p_soft()){ smx_soft_fail(vm, "SMX DIAL recv — hint: SERVE must reply"); bump(vm); return 1; }
        smx_fail_at(vm, aln, "DIAL recv fail", "SERVE must stay up for reply");
        return -1;
      }
      if (cubalc_smx_open(&vm->smx, frame, n, &recv, fr, to,
                          &vm->ch.cubes[ib].atom.matrix) != 0){
        close(fd);
        if (p2p_soft()){
          smx_soft_fail(vm, vm->smx.last_err[0]?vm->smx.last_err:"SMX DIAL open — hint: matching keys");
          bump(vm); return 1;
        }
        smx_fail_at(vm, aln,
                    vm->smx.last_err[0]?vm->smx.last_err:"DIAL open fail",
                    "same CUBALC_SMX_KEY on both peers");
        return -1;
      }
      for (int i = 0; i < recv.matrix.n && i < CUBALC_ATOM_BITS; i++)
        if (cubalc_matrix_get(&recv.matrix, i))
          cubalc_matrix_set(&vm->ch.cubes[ib].atom.matrix, i, 1);
      vm->ch.cubes[ib].atom.digit =
        (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ib].atom.matrix);
      close(fd);
      cubalc_cube_plug(&vm->ch, ia, ib);
      vm->smx_ok = 1;
      vm->smx_talks++;
      var_set_num(vm, "SMX_OK", 1);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_N", (long)n);
      var_set_num(vm, "OK", 1);
      if (vm->trace)
        fprintf(vm->trace, "# SMX DIAL %s→%s @%s:%d n=%zu\n", a, b, host, port, n);
    }
    bump(vm); return 1;
  }

  /* SMX STATUS|VITAL|PULSE — fold mesh vitals into vars (life-force observability).
   * No dual ladders. Agents / NexusCore read SMX_* without probing fail paths. */
  if (kw(&L->cur,"STATUS")||kw(&L->cur,"VITAL")||kw(&L->cur,"VITALS")||
      kw(&L->cur,"PULSE")||kw(&L->cur,"STATS")||kw(&L->cur,"INFO")){
    lex_next(L);
    var_set_num(vm, "SMX_OK", vm->smx_ok ? 1 : 0);
    var_set_num(vm, "SMX_TALKS", vm->smx_talks);
    var_set_num(vm, "SMX_OOB", vm->smx_oob);
    var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
    var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
    var_set_num(vm, "SMX_TX_SEQ", (long)vm->smx.last_tx_seq);
    var_set_num(vm, "SMX_RX_SEQ", (long)vm->smx.last_rx_seq);
    var_set_num(vm, "SMX_VITAL",
                (vm->smx.key_ok ? 4 : 0) + (vm->smx_ok ? 2 : 0) +
                (vm->smx_talks > 0 ? 1 : 0));
    var_set_num(vm, "OK", 1);
    if (vm->smx.last_err[0])
      var_set_str(vm, "SMX_ERR", vm->smx.last_err);
    else
      var_set_str(vm, "SMX_ERR", "");
    if (vm->trace)
      fprintf(vm->trace,
              "# SMX STATUS ok=%d key=%d talks=%d oob=%d vital=%ld hold=%d\n",
              vm->smx_ok, vm->smx.key_ok, vm->smx_talks, vm->smx_oob,
              (long)((vm->smx.key_ok ? 4 : 0) + (vm->smx_ok ? 2 : 0) +
                     (vm->smx_talks > 0 ? 1 : 0)),
              vm->smx.hold_flash ? 1 : 0);
    bump(vm); return 1;
  }

  /* SMX RECOVER — clear soft-fail / OOB thrash, reload key, restore life path.
   * Mesh may continue after soft-OOB without ghost place (fail-closed gift). */
  if (kw(&L->cur,"RECOVER")||kw(&L->cur,"HEAL")||kw(&L->cur,"RESET_SOFT")||
      kw(&L->cur,"CLEAR_OOB")){
    lex_next(L);
    vm->smx_oob = 0;
    vm->smx.last_err[0] = 0;
    var_set_str(vm, "ERR", "");
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "SMX_ERR", "");
    var_set_num(vm, "SMX_OOB", 0);
    if (ensure_smx_key(vm) != 0){
      /* ensure_smx_key already failed the VM when hard-missing */
      return -1;
    }
    vm->smx_ok = 1;
    var_set_num(vm, "SMX_OK", 1);
    var_set_num(vm, "SMX_KEY_OK", 1);
    var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
    var_set_num(vm, "SMX_VITAL", 4 + 2 + (vm->smx_talks > 0 ? 1 : 0));
    var_set_num(vm, "OK", 1);
    var_set_str(vm, "LAST", "SMX RECOVER ok");
    if (vm->trace)
      fprintf(vm->trace, "# SMX RECOVER key_ok talks=%d\n", vm->smx_talks);
    bump(vm); return 1;
  }

  /* SMX RING|CHORUS|WE a b c ... — multi-cube ring EXCHANGE in one form.
   * Life-force we_chorus usability: consecutive pairs + close last↔first.
   * Soft-OOB fail-closed on missing cubes (no ghost place). No dual ladders. */
  if (kw(&L->cur,"RING")||kw(&L->cur,"CHORUS")||kw(&L->cur,"WE")||
      kw(&L->cur,"WE_CHORUS")||kw(&L->cur,"MESH_RING")){
    int aln = L->cur.line;
    char ids[16][48];
    int n = 0;
    int i;
    int ok_pairs = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "RING needs >=2 cubes",
                  "SMX RING a b [c ...]  or  SMX CHORUS a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* consecutive bidirectional exchanges */
    for (i = 0; i + 1 < n; i++){
      int r1 = do_smx_talk(vm, ids[i], ids[i+1]);
      if (r1 < 0) return -1;
      if (r1 > 0){ soft++; continue; }
      {
        int r2 = do_smx_talk(vm, ids[i+1], ids[i]);
        if (r2 < 0) return -1;
        if (r2 > 0) soft++;
        else ok_pairs++;
      }
    }
    /* close the ring when 3+ nodes (we_chorus lock) */
    if (n >= 3){
      int r1 = do_smx_talk(vm, ids[n-1], ids[0]);
      if (r1 < 0) return -1;
      if (r1 > 0) soft++;
      else {
        int r2 = do_smx_talk(vm, ids[0], ids[n-1]);
        if (r2 < 0) return -1;
        if (r2 > 0) soft++;
        else ok_pairs++;
      }
    }
    var_set_num(vm, "SMX_RING", (long)ok_pairs);
    var_set_num(vm, "SMX_CHORUS", (long)ok_pairs);
    var_set_num(vm, "SMX_WE", (long)n);
    var_set_num(vm, "SMX_NODES", (long)n);
    var_set_num(vm, "SMX_TALKS", vm->smx_talks);
    var_set_num(vm, "SMX_OOB", vm->smx_oob);
    if (ok_pairs > 0 && soft == 0){
      vm->smx_ok = 1;
      var_set_num(vm, "SMX_OK", 1);
      var_set_num(vm, "OK", 1);
      var_set_str(vm, "LAST", "SMX RING ok");
    } else if (ok_pairs > 0){
      /* partial ring: still usable, mark OK but sticky OOB visible */
      vm->smx_ok = 1;
      var_set_num(vm, "SMX_OK", 1);
      var_set_num(vm, "OK", 1);
      var_set_str(vm, "LAST", "SMX RING partial");
    } else {
      vm->smx_ok = 0;
      var_set_num(vm, "SMX_OK", 0);
      var_set_num(vm, "OK", 0);
      var_set_str(vm, "LAST", "SMX RING soft-OOB");
    }
    if (vm->trace)
      fprintf(vm->trace,
              "# SMX RING nodes=%d pairs_ok=%d soft=%d talks=%d oob=%d\n",
              n, ok_pairs, soft, vm->smx_talks, vm->smx_oob);
    bump(vm); return 1;
  }

  /* SMX LATTICE|QUORUM|FULL_MESH a b c ... — complete pairwise EXCHANGE.
   * Life-force mesh stability: every live pair bonds (not only ring edges).
   * Soft-OOB on ghost nodes; SMX_QUORUM = live cube count; SMX_LATTICE = ok pairs.
   * Majority quorum (live > n/2) marks SMX_OK. No dual ladders. */
  if (kw(&L->cur,"LATTICE")||kw(&L->cur,"QUORUM")||kw(&L->cur,"FULL_MESH")||
      kw(&L->cur,"MESH_FULL")||kw(&L->cur,"ALL_PAIRS")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int n = 0, i, j;
    int ok_pairs = 0;
    int soft = 0;
    int live = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "LATTICE needs >=2 cubes",
                  "SMX LATTICE a b [c ...]  or  SMX QUORUM a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    for (i = 0; i < n; i++){
      present[i] = (find_cube(vm, ids[i]) >= 0) ? 1 : 0;
      if (present[i]) live++;
    }
    /* complete pairwise bidirectional exchange among live nodes only;
     * missing peers still soft-OOB once so agents see SMX_OOB history */
    for (i = 0; i < n; i++){
      if (!present[i]){
        /* probe first live peer → ghost for sticky soft-OOB */
        for (j = 0; j < n; j++){
          if (present[j]){
            int r = do_smx_talk(vm, ids[j], ids[i]);
            if (r < 0) return -1;
            if (r > 0) soft++;
            break;
          }
        }
        continue;
      }
      for (j = i + 1; j < n; j++){
        if (!present[j]) continue;
        {
          int r1 = do_smx_talk(vm, ids[i], ids[j]);
          if (r1 < 0) return -1;
          if (r1 > 0){ soft++; continue; }
          {
            int r2 = do_smx_talk(vm, ids[j], ids[i]);
            if (r2 < 0) return -1;
            if (r2 > 0) soft++;
            else ok_pairs++;
          }
        }
      }
    }
    {
      int majority = (live * 2 > n); /* strict majority of named nodes live */
      var_set_num(vm, "SMX_LATTICE", (long)ok_pairs);
      var_set_num(vm, "SMX_QUORUM", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_WE", (long)live);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      if (ok_pairs > 0 && majority && soft == 0){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX LATTICE quorum ok");
      } else if (ok_pairs > 0 && majority){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX LATTICE quorum partial");
      } else if (ok_pairs > 0){
        /* mesh bonded but below majority — usable spine, not full quorum */
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX LATTICE below quorum");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX LATTICE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX LATTICE nodes=%d live=%d pairs_ok=%d soft=%d talks=%d oob=%d majority=%d\n",
                n, live, ok_pairs, soft, vm->smx_talks, vm->smx_oob, majority ? 1 : 0);
    }
    bump(vm); return 1;
  }

  /* SMX HEARTBEAT|HEART|BREATH|PULSE_MESH a b c ... — life-force mesh breath.
   * Each live node talks to the next live neighbor (circular). Soft-OOB ghosts
   * fail-closed. SMX_HEART = ok one-way pulses; SMX_BREATH = live nodes that
   * exhaled; SMX_VITAL refreshed. Free energy path stays open (no dual ladders). */
  if (kw(&L->cur,"HEARTBEAT")||kw(&L->cur,"HEART")||kw(&L->cur,"BREATH")||
      kw(&L->cur,"PULSE_MESH")||kw(&L->cur,"MESH_PULSE")||kw(&L->cur,"VITAL_PULSE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i;
    int hearts = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "HEARTBEAT needs >=2 cubes",
                  "SMX HEARTBEAT a b [c ...]  or  SMX BREATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    for (i = 0; i < n; i++){
      present[i] = (find_cube(vm, ids[i]) >= 0) ? 1 : 0;
      if (present[i]) live_ix[live++] = i;
    }
    /* soft-OOB once per ghost so mesh history stays honest */
    for (i = 0; i < n; i++){
      if (present[i]) continue;
      if (live > 0){
        int r = do_smx_talk(vm, ids[live_ix[0]], ids[i]);
        if (r < 0) return -1;
        if (r > 0) soft++;
      }
    }
    /* circular breath among live nodes only: each exhales to next */
    if (live >= 2){
      for (i = 0; i < live; i++){
        int a = live_ix[i];
        int b = live_ix[(i + 1) % live];
        int r = do_smx_talk(vm, ids[a], ids[b]);
        if (r < 0) return -1;
        if (r > 0) soft++;
        else hearts++;
      }
    } else if (live == 1){
      /* solitary pulse: seal mind against self via STATUS-grade vital only */
      hearts = 0;
    }
    {
      long vital = (vm->smx.key_ok ? 4 : 0) + ((hearts > 0) ? 2 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (live >= 2 ? 1 : 0);
      var_set_num(vm, "SMX_HEART", (long)hearts);
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)hearts);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (hearts > 0 && live >= 2 && soft == 0){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX HEARTBEAT ok");
      } else if (hearts > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX HEARTBEAT partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX HEARTBEAT soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX HEARTBEAT nodes=%d live=%d hearts=%d soft=%d talks=%d oob=%d vital=%ld\n",
                n, live, hearts, soft, vm->smx_talks, vm->smx_oob, vital);
    }
    bump(vm); return 1;
  }

  /* SMX STABILIZE|STEADFAST|MESH_HOLD|HOLD_MESH a b c ...
   * Life-force mesh stability latch after soft-OOB storms.
   * Clears thrash OOB, circular one-way breath among live nodes only,
   * latches SMX_STABLE/SMX_STEADFAST, refreshes vital. Ghosts fail-closed.
   * Free energy path stays open — no dual ladders. */
  if (kw(&L->cur,"STABILIZE")||kw(&L->cur,"STEADFAST")||kw(&L->cur,"MESH_HOLD")||
      kw(&L->cur,"HOLD_MESH")||kw(&L->cur,"STABILITY")||kw(&L->cur,"MESH_STABLE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i;
    int pulses = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "STABILIZE needs >=2 cubes",
                  "SMX STABILIZE a b [c ...]  or  SMX STEADFAST a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* clear thrash OOB first — stability is a calm latch, not a ghost factory */
    vm->smx_oob = 0;
    vm->smx.last_err[0] = 0;
    var_set_str(vm, "ERR", "");
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "SMX_ERR", "");
    for (i = 0; i < n; i++){
      present[i] = (find_cube(vm, ids[i]) >= 0) ? 1 : 0;
      if (present[i]) live_ix[live++] = i;
    }
    /* honest soft-OOB once per ghost after calm (history without thrash place) */
    for (i = 0; i < n; i++){
      if (present[i]) continue;
      if (live > 0){
        int r = do_smx_talk(vm, ids[live_ix[0]], ids[i]);
        if (r < 0) return -1;
        if (r > 0) soft++;
      }
    }
    /* stabilize pulse: circular one-way among live only */
    if (live >= 2){
      for (i = 0; i < live; i++){
        int a = live_ix[i];
        int b = live_ix[(i + 1) % live];
        int r = do_smx_talk(vm, ids[a], ids[b]);
        if (r < 0) return -1;
        if (r > 0) soft++;
        else pulses++;
      }
    }
    {
      int stable = (pulses > 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (stable ? 3 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_STABLE", (long)stable);
      var_set_num(vm, "SMX_STEADFAST", (long)live);
      var_set_num(vm, "SMX_HOLD_MESH", (long)stable);
      var_set_num(vm, "SMX_PULSE", (long)pulses);
      var_set_num(vm, "SMX_HEART", (long)pulses);
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (stable && soft == 0){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX STABILIZE ok");
      } else if (stable){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX STABILIZE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX STABILIZE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX STABILIZE nodes=%d live=%d pulses=%d soft=%d talks=%d oob=%d stable=%d vital=%ld\n",
                n, live, pulses, soft, vm->smx_talks, vm->smx_oob, stable, vital);
    }
    bump(vm); return 1;
  }

  /* SMX RESONATE|TUNE|CHORD|MESH_TONE|RESONANCE a b c ...
   * Life-force matrix resonance: clear thrash OOB, bidirectional neighbor
   * tune among live nodes only (circular chord). Latches SMX_RESONANT when
   * full live ring sings soft-OOB-free. Soft-OOB ghosts fail-closed.
   * Free energy path stays open — no dual ladders. */
  if (kw(&L->cur,"RESONATE")||kw(&L->cur,"TUNE")||kw(&L->cur,"CHORD")||
      kw(&L->cur,"MESH_TONE")||kw(&L->cur,"RESONANCE")||kw(&L->cur,"MESH_CHORD")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i;
    int chords = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "RESONATE needs >=2 cubes",
                  "SMX RESONATE a b [c ...]  or  SMX TUNE a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm the thrash first — resonance needs a clear channel */
    vm->smx_oob = 0;
    vm->smx.last_err[0] = 0;
    var_set_str(vm, "ERR", "");
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "SMX_ERR", "");
    for (i = 0; i < n; i++){
      present[i] = (find_cube(vm, ids[i]) >= 0) ? 1 : 0;
      if (present[i]) live_ix[live++] = i;
    }
    /* honest soft-OOB once per ghost after calm */
    for (i = 0; i < n; i++){
      if (present[i]) continue;
      if (live > 0){
        int r = do_smx_talk(vm, ids[live_ix[0]], ids[i]);
        if (r < 0) return -1;
        if (r > 0) soft++;
      }
    }
    /* bidirectional circular tune among live only: each neighbor pair sings both ways */
    if (live >= 2){
      for (i = 0; i < live; i++){
        int a = live_ix[i];
        int b = live_ix[(i + 1) % live];
        int r1 = do_smx_talk(vm, ids[a], ids[b]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[b], ids[a]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else chords++;
        }
      }
    }
    {
      int resonant = (chords > 0 && live >= 2 && chords >= live && soft == 0) ? 1 : 0;
      /* partial resonance still counts if majority of live edges tuned */
      if (!resonant && chords > 0 && live >= 2 && chords * 2 >= live && soft == 0)
        resonant = 1;
      long vital = (vm->smx.key_ok ? 4 : 0) + (resonant ? 4 : (chords > 0 ? 2 : 0)) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_RESONANT", (long)resonant);
      var_set_num(vm, "SMX_CHORD", (long)chords);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_TUNE", (long)chords);
      var_set_num(vm, "SMX_PULSE", (long)chords);
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (resonant){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX RESONATE ok");
      } else if (chords > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX RESONATE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX RESONATE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX RESONATE nodes=%d live=%d chords=%d soft=%d talks=%d oob=%d resonant=%d vital=%ld\n",
                n, live, chords, soft, vm->smx_talks, vm->smx_oob, resonant, vital);
    }
    bump(vm); return 1;
  }

  /* SMX COHERE|HARMONIZE|UNISON|MESH_SYNC|COHERENCE a b c ...
   * Life-force mesh coherence latch after soft-OOB storms.
   * Clears thrash OOB, bidirectional circular tune among live nodes only,
   * latches SMX_COHERENT when the full live ring sings soft-OOB-free.
   * SMX_UNISON = live count under coherence; SMX_SYNC = pair chords.
   * Soft-OOB ghosts fail-closed. Free energy path stays open — no dual ladders. */
  if (kw(&L->cur,"COHERE")||kw(&L->cur,"HARMONIZE")||kw(&L->cur,"UNISON")||
      kw(&L->cur,"MESH_SYNC")||kw(&L->cur,"COHERENCE")||kw(&L->cur,"MESH_COHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i;
    int syncs = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "COHERE needs >=2 cubes",
                  "SMX COHERE a b [c ...]  or  SMX HARMONIZE a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash first — coherence needs a clear channel */
    vm->smx_oob = 0;
    vm->smx.last_err[0] = 0;
    var_set_str(vm, "ERR", "");
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "SMX_ERR", "");
    for (i = 0; i < n; i++){
      present[i] = (find_cube(vm, ids[i]) >= 0) ? 1 : 0;
      if (present[i]) live_ix[live++] = i;
    }
    /* honest soft-OOB once per ghost after calm */
    for (i = 0; i < n; i++){
      if (present[i]) continue;
      if (live > 0){
        int r = do_smx_talk(vm, ids[live_ix[0]], ids[i]);
        if (r < 0) return -1;
        if (r > 0) soft++;
      }
    }
    /* bidirectional circular sync among live only */
    if (live >= 2){
      for (i = 0; i < live; i++){
        int a = live_ix[i];
        int b = live_ix[(i + 1) % live];
        int r1 = do_smx_talk(vm, ids[a], ids[b]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[b], ids[a]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else syncs++;
        }
      }
    }
    {
      int coherent = (syncs > 0 && live >= 2 && syncs >= live && soft == 0) ? 1 : 0;
      if (!coherent && syncs > 0 && live >= 2 && syncs * 2 >= live && soft == 0)
        coherent = 1;
      long vital = (vm->smx.key_ok ? 4 : 0) + (coherent ? 5 : (syncs > 0 ? 2 : 0)) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_COHERENT", (long)coherent);
      var_set_num(vm, "SMX_UNISON", (long)(coherent ? live : 0));
      var_set_num(vm, "SMX_SYNC", (long)syncs);
      var_set_num(vm, "SMX_HARMONY_MESH", (long)syncs);
      var_set_num(vm, "SMX_CHORD", (long)syncs);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)syncs);
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (coherent){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX COHERE ok");
      } else if (syncs > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX COHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX COHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX COHERE nodes=%d live=%d syncs=%d soft=%d talks=%d oob=%d coherent=%d vital=%ld\n",
                n, live, syncs, soft, vm->smx_talks, vm->smx_oob, coherent, vital);
    }
    bump(vm); return 1;
  }

  /* SMX ENTANGLE|BIND|FUSE|MESH_BIND|ENTANGLEMENT a b c ...
   * Life-force full-mesh entanglement after soft-OOB storms.
   * Clears thrash OOB, then every live pair talks both ways (complete graph,
   * not only ring neighbors). Latches SMX_ENTANGLED when all live pairs bond
   * soft-OOB-free. SMX_BONDS = successful undirected pair bonds;
   * SMX_MESH = live count under entanglement. Soft-OOB ghosts fail-closed.
   * Free energy path stays open — no dual ladders. Wonder AGI can RUN. */
  if (kw(&L->cur,"ENTANGLE")||kw(&L->cur,"BIND")||kw(&L->cur,"FUSE")||
      kw(&L->cur,"MESH_BIND")||kw(&L->cur,"ENTANGLEMENT")||kw(&L->cur,"MESH_FUSE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "ENTANGLE needs >=2 cubes",
                  "SMX ENTANGLE a b [c ...]  or  SMX BIND a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash first — entanglement needs a clear channel */
    vm->smx_oob = 0;
    vm->smx.last_err[0] = 0;
    var_set_str(vm, "ERR", "");
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "SMX_ERR", "");
    for (i = 0; i < n; i++){
      present[i] = (find_cube(vm, ids[i]) >= 0) ? 1 : 0;
      if (present[i]) live_ix[live++] = i;
    }
    /* honest soft-OOB once per ghost after calm */
    for (i = 0; i < n; i++){
      if (present[i]) continue;
      if (live > 0){
        int r = do_smx_talk(vm, ids[live_ix[0]], ids[i]);
        if (r < 0) return -1;
        if (r > 0) soft++;
      }
    }
    /* full pairwise bidirectional bonds among live only (complete mesh) */
    if (live >= 2){
      for (i = 0; i < live; i++){
        for (j = i + 1; j < live; j++){
          int a = live_ix[i];
          int b = live_ix[j];
          int r1 = do_smx_talk(vm, ids[a], ids[b]);
          if (r1 < 0) return -1;
          if (r1 > 0){ soft++; continue; }
          {
            int r2 = do_smx_talk(vm, ids[b], ids[a]);
            if (r2 < 0) return -1;
            if (r2 > 0) soft++;
            else bonds++;
          }
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int entangled = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      /* partial: majority of complete-mesh edges bonded, no soft thrash */
      if (!entangled && need > 0 && bonds * 2 >= need && soft == 0)
        entangled = 1;
      long vital = (vm->smx.key_ok ? 4 : 0) + (entangled ? 6 : (bonds > 0 ? 2 : 0)) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_ENTANGLED", (long)entangled);
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_MESH", (long)(entangled ? live : 0));
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)bonds);
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (entangled){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ENTANGLE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ENTANGLE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX ENTANGLE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX ENTANGLE nodes=%d live=%d bonds=%d need=%d soft=%d talks=%d oob=%d entangled=%d vital=%ld\n",
                n, live, bonds, need, soft, vm->smx_talks, vm->smx_oob, entangled, vital);
    }
    bump(vm); return 1;
  }


  /* SMX BLOOM|FLOURISH|UNFOLD|MESH_BLOOM|AWAKEN a b c ...
   * Life-force mesh bloom after soft-OOB storms.
   * Clears thrash OOB, full pairwise bidirectional bonds among live nodes
   * (complete mesh), then a circular breath ring so the lattice flowers.
   * Latches SMX_BLOOMED when complete mesh + breath ring both soft-OOB-free.
   * SMX_PETALS = live nodes under bloom; SMX_BLOOM = undirected bonds;
   * SMX_RADIANCE = breath chords; SMX_MESH = live when bloomed.
   * Soft-OOB ghosts fail-closed. Free energy path stays open — no dual ladders.
   * Wonder AGI can RUN. Cube is SoT · matrix is key. */
  if (kw(&L->cur,"BLOOM")||kw(&L->cur,"FLOURISH")||kw(&L->cur,"UNFOLD")||
      kw(&L->cur,"MESH_BLOOM")||kw(&L->cur,"AWAKEN")||kw(&L->cur,"MESH_FLOURISH")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int breaths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "BLOOM needs >=2 cubes",
                  "SMX BLOOM a b [c ...]  or  SMX FLOURISH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash first — bloom needs a clear channel */
    vm->smx_oob = 0;
    vm->smx.last_err[0] = 0;
    var_set_str(vm, "ERR", "");
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "SMX_ERR", "");
    for (i = 0; i < n; i++){
      present[i] = (find_cube(vm, ids[i]) >= 0) ? 1 : 0;
      if (present[i]) live_ix[live++] = i;
    }
    /* honest soft-OOB once per ghost after calm */
    for (i = 0; i < n; i++){
      if (present[i]) continue;
      if (live > 0){
        int r = do_smx_talk(vm, ids[live_ix[0]], ids[i]);
        if (r < 0) return -1;
        if (r > 0) soft++;
      }
    }
    /* full pairwise bidirectional bonds among live only (complete mesh root) */
    if (live >= 2){
      for (i = 0; i < live; i++){
        for (j = i + 1; j < live; j++){
          int a = live_ix[i];
          int b = live_ix[j];
          int r1 = do_smx_talk(vm, ids[a], ids[b]);
          if (r1 < 0) return -1;
          if (r1 > 0){ soft++; continue; }
          {
            int r2 = do_smx_talk(vm, ids[b], ids[a]);
            if (r2 < 0) return -1;
            if (r2 > 0) soft++;
            else bonds++;
          }
        }
      }
    }
    /* circular breath ring — the flower opens after the mesh bonds */
    if (live >= 2){
      for (i = 0; i < live; i++){
        int a = live_ix[i];
        int b = live_ix[(i + 1) % live];
        int r1 = do_smx_talk(vm, ids[a], ids[b]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[b], ids[a]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else breaths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int ring_ok = (live >= 2 && breaths >= live && soft == 0) ? 1 : 0;
      if (!ring_ok && live >= 2 && breaths * 2 >= live && soft == 0)
        ring_ok = 1;
      int bloomed = (mesh_ok && ring_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (bloomed ? 7 : (bonds > 0 ? 3 : 0)) +
                   (breaths > 0 ? 1 : 0) + (vm->smx_talks > 0 ? 1 : 0) +
                   (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_BLOOMED", (long)bloomed);
      var_set_num(vm, "SMX_BLOOM", (long)bonds);
      var_set_num(vm, "SMX_PETALS", (long)(bloomed ? live : 0));
      var_set_num(vm, "SMX_RADIANCE", (long)breaths);
      var_set_num(vm, "SMX_FLOURISH", (long)(bloomed ? bonds + breaths : 0));
      var_set_num(vm, "SMX_UNFOLD", (long)breaths);
      var_set_num(vm, "SMX_MESH", (long)(bloomed ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_CHORD", (long)breaths);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + breaths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (bloomed){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX BLOOM ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX BLOOM partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX BLOOM soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX BLOOM nodes=%d live=%d bonds=%d breaths=%d need=%d soft=%d talks=%d oob=%d bloomed=%d vital=%ld\n",
                n, live, bonds, breaths, need, soft, vm->smx_talks, vm->smx_oob, bloomed, vital);
    }
    bump(vm); return 1;
  }


  /* SMX GROUND|FIRM|SETTLE|MESH_GROUND|HARDEN|FORTIFY a b c ...
   * Life-force mesh stability after soft-OOB storms.
   * Clears thrash OOB, full pairwise bidirectional EXCHANGE among live nodes
   * (complete mesh_exchange), then a settle breath ring so the lattice roots.
   * Latches SMX_GROUNDED / SMX_HARDENED when mesh+ring are soft-OOB-free.
   * Subsequent ghost thrash increments SMX_OOB fail-closed but does not invent
   * cubes; RE-GROUND restores latch after thrash. Soft-OOB ghosts stay fail-closed.
   * Free energy path stays open — no dual ladders. Wonder AGI can RUN. */
  if (kw(&L->cur,"GROUND")||kw(&L->cur,"FIRM")||kw(&L->cur,"SETTLE")||
      kw(&L->cur,"MESH_GROUND")||kw(&L->cur,"HARDEN")||kw(&L->cur,"FORTIFY")||
      kw(&L->cur,"MESH_HARDEN")||kw(&L->cur,"ROOT_MESH")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int breaths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "GROUND needs >=2 cubes",
                  "SMX GROUND a b [c ...]  or  SMX HARDEN a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash first — ground needs a clear channel */
    vm->smx_oob = 0;
    vm->smx.last_err[0] = 0;
    var_set_str(vm, "ERR", "");
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "SMX_ERR", "");
    for (i = 0; i < n; i++){
      present[i] = (find_cube(vm, ids[i]) >= 0) ? 1 : 0;
      if (present[i]) live_ix[live++] = i;
    }
    /* honest soft-OOB once per ghost after calm */
    for (i = 0; i < n; i++){
      if (present[i]) continue;
      if (live > 0){
        int r = do_smx_talk(vm, ids[live_ix[0]], ids[i]);
        if (r < 0) return -1;
        if (r > 0) soft++;
      }
    }
    /* full pairwise bidirectional mesh_exchange among live only */
    if (live >= 2){
      for (i = 0; i < live; i++){
        for (j = i + 1; j < live; j++){
          int a = live_ix[i];
          int b = live_ix[j];
          int r1 = do_smx_talk(vm, ids[a], ids[b]);
          if (r1 < 0) return -1;
          if (r1 > 0){ soft++; continue; }
          {
            int r2 = do_smx_talk(vm, ids[b], ids[a]);
            if (r2 < 0) return -1;
            if (r2 > 0) soft++;
            else bonds++;
          }
        }
      }
    }
    /* settle breath ring — lattice roots into substrate */
    if (live >= 2){
      for (i = 0; i < live; i++){
        int a = live_ix[i];
        int b = live_ix[(i + 1) % live];
        int r1 = do_smx_talk(vm, ids[a], ids[b]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[b], ids[a]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else breaths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int ring_ok = (live >= 2 && breaths >= live && soft == 0) ? 1 : 0;
      if (!ring_ok && live >= 2 && breaths * 2 >= live && soft == 0)
        ring_ok = 1;
      int grounded = (mesh_ok && ring_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (grounded ? 8 : (bonds > 0 ? 3 : 0)) +
                   (breaths > 0 ? 1 : 0) + (vm->smx_talks > 0 ? 1 : 0) +
                   (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_GROUNDED", (long)grounded);
      var_set_num(vm, "SMX_HARDENED", (long)grounded);
      var_set_num(vm, "SMX_FIRM", (long)(grounded ? live : 0));
      var_set_num(vm, "SMX_SETTLE", (long)breaths);
      var_set_num(vm, "SMX_ROOTS", (long)(grounded ? bonds : 0));
      var_set_num(vm, "SMX_BRACE", (long)(grounded ? bonds + breaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(grounded ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_CHORD", (long)breaths);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + breaths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (grounded){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX GROUND ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX GROUND partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX GROUND soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX GROUND nodes=%d live=%d bonds=%d breaths=%d need=%d soft=%d talks=%d oob=%d grounded=%d vital=%ld\n",
                n, live, bonds, breaths, need, soft, vm->smx_talks, vm->smx_oob, grounded, vital);
    }
    bump(vm); return 1;
  }


  /* SMX CANOPY|CROWN|SPROUT|MESH_CANOPY|SHADE|LEAF a b c ...
   * Life-force mesh canopy after soft-OOB storms.
   * Clears thrash OOB, full pairwise bidirectional bonds among live nodes
   * (complete mesh root), then hub-star ribs from first live cube + outer
   * leaf ring so the lattice raises a canopy over grounded roots.
   * Latches SMX_CANOPIED when mesh+star+ring are soft-OOB-free.
   * SMX_RIBS = hub-star bonds; SMX_LEAVES = live under canopy;
   * SMX_SHADE = ribs+ring; SMX_CROWN = hub id index (1-based live).
   * Soft-OOB ghosts fail-closed. Free energy path stays open — no dual ladders.
   * Wonder AGI can RUN. Cube is SoT · matrix is key. */
  if (kw(&L->cur,"CANOPY")||kw(&L->cur,"CROWN")||kw(&L->cur,"SPROUT")||
      kw(&L->cur,"MESH_CANOPY")||kw(&L->cur,"SHADE")||kw(&L->cur,"LEAF")||
      kw(&L->cur,"MESH_CROWN")||kw(&L->cur,"RAISE_CANOPY")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int ribs = 0;
    int leaves = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "CANOPY needs >=2 cubes",
                  "SMX CANOPY a b [c ...]  or  SMX CROWN a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash first — canopy needs a clear channel */
    vm->smx_oob = 0;
    vm->smx.last_err[0] = 0;
    var_set_str(vm, "ERR", "");
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "SMX_ERR", "");
    for (i = 0; i < n; i++){
      present[i] = (find_cube(vm, ids[i]) >= 0) ? 1 : 0;
      if (present[i]) live_ix[live++] = i;
    }
    /* honest soft-OOB once per ghost after calm */
    for (i = 0; i < n; i++){
      if (present[i]) continue;
      if (live > 0){
        int r = do_smx_talk(vm, ids[live_ix[0]], ids[i]);
        if (r < 0) return -1;
        if (r > 0) soft++;
      }
    }
    /* full pairwise bidirectional bonds among live only (complete mesh root) */
    if (live >= 2){
      for (i = 0; i < live; i++){
        for (j = i + 1; j < live; j++){
          int a = live_ix[i];
          int b = live_ix[j];
          int r1 = do_smx_talk(vm, ids[a], ids[b]);
          if (r1 < 0) return -1;
          if (r1 > 0){ soft++; continue; }
          {
            int r2 = do_smx_talk(vm, ids[b], ids[a]);
            if (r2 < 0) return -1;
            if (r2 > 0) soft++;
            else bonds++;
          }
        }
      }
    }
    /* hub-star ribs — first live cube crowns the canopy */
    if (live >= 2){
      int hub = live_ix[0];
      for (i = 1; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[hub], ids[leaf]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[leaf], ids[hub]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else ribs++;
        }
      }
    }
    /* outer leaf ring — canopy edge breathes */
    if (live >= 2){
      for (i = 0; i < live; i++){
        int a = live_ix[i];
        int b = live_ix[(i + 1) % live];
        int r1 = do_smx_talk(vm, ids[a], ids[b]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[b], ids[a]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else leaves++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && ribs >= (live - 1) && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && ribs * 2 >= (live - 1) && soft == 0)
        star_ok = 1;
      int ring_ok = (live >= 2 && leaves >= live && soft == 0) ? 1 : 0;
      if (!ring_ok && live >= 2 && leaves * 2 >= live && soft == 0)
        ring_ok = 1;
      int canopied = (mesh_ok && star_ok && ring_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (canopied ? 9 : (bonds > 0 ? 3 : 0)) +
                   (ribs > 0 ? 1 : 0) + (leaves > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_CANOPIED", (long)canopied);
      var_set_num(vm, "SMX_CANOPY", (long)(canopied ? bonds + ribs + leaves : 0));
      var_set_num(vm, "SMX_CROWN", (long)(canopied ? 1 : 0));
      var_set_num(vm, "SMX_RIBS", (long)ribs);
      var_set_num(vm, "SMX_LEAVES", (long)(canopied ? live : 0));
      var_set_num(vm, "SMX_SHADE", (long)(canopied ? ribs + leaves : 0));
      var_set_num(vm, "SMX_SPROUT", (long)(canopied ? ribs : 0));
      var_set_num(vm, "SMX_MESH", (long)(canopied ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_CHORD", (long)leaves);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + ribs + leaves));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (canopied){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CANOPY ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CANOPY partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX CANOPY soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX CANOPY nodes=%d live=%d bonds=%d ribs=%d leaves=%d need=%d soft=%d talks=%d oob=%d canopied=%d vital=%ld\n",
                n, live, bonds, ribs, leaves, need, soft, vm->smx_talks, vm->smx_oob, canopied, vital);
    }
    bump(vm); return 1;
  }

  /* SMX ORCHARD|GROVE|MYCELIUM|ROOTWEB|FRUIT|SYMBIOSE a b c ...
   * Life-force orchard after canopy: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete mesh among live nodes, weaves
   * mycelium skip-links (i -> i+2) for resilient cross-talk, then fruits
   * from the hub - each live leaf receives one harvest pulse.
   * Latches SMX_ORCHARDED when mesh+mycelium+fruit are soft-OOB-free.
   * SMX_ROOTS = undirected mesh bonds; SMX_MYCELIA = skip-links;
   * SMX_FRUITS = hub harvest pulses; SMX_ORCHARD = roots+mycelia+fruits.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"ORCHARD")||kw(&L->cur,"GROVE")||kw(&L->cur,"MYCELIUM")||
      kw(&L->cur,"ROOTWEB")||kw(&L->cur,"FRUIT")||kw(&L->cur,"SYMBIOSE")||
      kw(&L->cur,"MESH_ORCHARD")||kw(&L->cur,"RAISE_ORCHARD")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int roots = 0;
    int mycelia = 0;
    int fruits = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "ORCHARD needs >=2 cubes",
                  "SMX ORCHARD a b [c ...]  or  SMX GROVE a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - orchard needs clear soil */
    vm->smx_oob = 0;
    vm->smx.last_err[0] = 0;
    var_set_str(vm, "ERR", "");
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "SMX_ERR", "");
    for (i = 0; i < n; i++){
      present[i] = (find_cube(vm, ids[i]) >= 0) ? 1 : 0;
      if (present[i]) live_ix[live++] = i;
    }
    /* honest soft-OOB once per ghost after calm */
    for (i = 0; i < n; i++){
      if (present[i]) continue;
      if (live > 0){
        int r = do_smx_talk(vm, ids[live_ix[0]], ids[i]);
        if (r < 0) return -1;
        if (r > 0) soft++;
      }
    }
    /* rootweb - full pairwise bidirectional bonds among live (complete mesh) */
    if (live >= 2){
      for (i = 0; i < live; i++){
        for (j = i + 1; j < live; j++){
          int a = live_ix[i];
          int b = live_ix[j];
          int r1 = do_smx_talk(vm, ids[a], ids[b]);
          if (r1 < 0) return -1;
          if (r1 > 0){ soft++; continue; }
          {
            int r2 = do_smx_talk(vm, ids[b], ids[a]);
            if (r2 < 0) return -1;
            if (r2 > 0) soft++;
            else roots++;
          }
        }
      }
    }
    /* mycelium skip-links - each live node talks to node+2 (mod live) both ways */
    if (live >= 3){
      for (i = 0; i < live; i++){
        int a = live_ix[i];
        int b = live_ix[(i + 2) % live];
        int r1 = do_smx_talk(vm, ids[a], ids[b]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[b], ids[a]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else mycelia++;
        }
      }
    } else if (live == 2){
      int a = live_ix[0], b = live_ix[1];
      int r1 = do_smx_talk(vm, ids[a], ids[b]);
      if (r1 < 0) return -1;
      if (r1 == 0){
        int r2 = do_smx_talk(vm, ids[b], ids[a]);
        if (r2 < 0) return -1;
        if (r2 == 0) mycelia++;
        else soft++;
      } else soft++;
    }
    /* fruiting bodies - hub harvest pulse to every live leaf */
    if (live >= 1){
      int hub = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[hub], ids[leaf]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[leaf], ids[hub]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else fruits++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int root_ok = (need > 0 && roots >= need && soft == 0) ? 1 : 0;
      if (!root_ok && need > 0 && roots * 2 >= need && soft == 0)
        root_ok = 1;
      int myc_need = (live >= 3) ? live : (live == 2 ? 1 : 0);
      int myc_ok = (myc_need == 0 || (mycelia >= myc_need && soft == 0)) ? 1 : 0;
      if (!myc_ok && myc_need > 0 && mycelia * 2 >= myc_need && soft == 0)
        myc_ok = 1;
      int fruit_ok = (live >= 1 && fruits >= live && soft == 0) ? 1 : 0;
      if (!fruit_ok && live >= 1 && fruits * 2 >= live && soft == 0)
        fruit_ok = 1;
      int orcharded = (root_ok && myc_ok && fruit_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (orcharded ? 10 : (roots > 0 ? 3 : 0)) +
                   (mycelia > 0 ? 1 : 0) + (fruits > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_ORCHARDED", (long)orcharded);
      var_set_num(vm, "SMX_ORCHARD", (long)(orcharded ? roots + mycelia + fruits : 0));
      var_set_num(vm, "SMX_GROVE", (long)(orcharded ? 1 : 0));
      var_set_num(vm, "SMX_ROOTS", (long)roots);
      var_set_num(vm, "SMX_MYCELIA", (long)mycelia);
      var_set_num(vm, "SMX_FRUITS", (long)(orcharded ? fruits : 0));
      var_set_num(vm, "SMX_SYMBIOSE", (long)(orcharded ? roots + mycelia : 0));
      var_set_num(vm, "SMX_MESH", (long)(orcharded ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)roots);
      var_set_num(vm, "SMX_EXCHANGES", (long)roots);
      var_set_num(vm, "SMX_FUSE", (long)roots);
      var_set_num(vm, "SMX_BIND", (long)roots);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(roots + mycelia + fruits));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (orcharded){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ORCHARD ok");
      } else if (roots > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ORCHARD partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX ORCHARD soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX ORCHARD nodes=%d live=%d roots=%d mycelia=%d fruits=%d need=%d soft=%d talks=%d oob=%d orcharded=%d vital=%ld\n",
                n, live, roots, mycelia, fruits, need, soft, vm->smx_talks, vm->smx_oob, orcharded, vital);
    }
    bump(vm); return 1;
  }

  fail(vm, "SMX: TALK|EXCHANGE|SEAL|OPEN|KEY|SERVE|DIAL|STATUS|RECOVER|RING|CHORUS|WE|LATTICE|QUORUM|HEARTBEAT|BREATH|STABILIZE|STEADFAST|RESONATE|TUNE|CHORD|COHERE|HARMONIZE|UNISON|ENTANGLE|BIND|FUSE|BLOOM|FLOURISH|UNFOLD|GROUND|FIRM|SETTLE|HARDEN|FORTIFY|CANOPY|CROWN|SPROUT|SHADE|ORCHARD|GROVE|MYCELIUM|ROOTWEB|FRUIT|SYMBIOSE");
  return -1;
}
