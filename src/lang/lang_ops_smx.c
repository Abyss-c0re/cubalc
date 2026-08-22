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

  fail(vm, "SMX: TALK|EXCHANGE|SEAL|OPEN|KEY|SERVE|DIAL|STATUS|RECOVER|RING|CHORUS|WE");
  return -1;
}
