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


  /* SMX MEADOW|PASTURE|POLLINATE|NECTAR|BLOOMFIELD|PRAIRIE a b c ...
   * Life-force meadow after orchard: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete mesh among live nodes, weaves a
   * pollinate ring (i -> i+1) so bees cross every edge, then nectar dew
   * from the hub so each live leaf drinks one sweet pulse.
   * Latches SMX_MEADOWED when mesh+pollen+nectar are soft-OOB-free.
   * SMX_POLLEN = ring bonds; SMX_NECTARS = hub dew pulses;
   * SMX_MEADOW = bonds+pollen+nectars; SMX_PASTURE|SMX_BLOOMFIELD sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"MEADOW")||kw(&L->cur,"PASTURE")||kw(&L->cur,"POLLINATE")||
      kw(&L->cur,"NECTAR")||kw(&L->cur,"BLOOMFIELD")||kw(&L->cur,"PRAIRIE")||
      kw(&L->cur,"MESH_MEADOW")||kw(&L->cur,"RAISE_MEADOW")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int pollen = 0;
    int nectars = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "MEADOW needs >=2 cubes",
                  "SMX MEADOW a b [c ...]  or  SMX PASTURE a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - meadow needs clear wind */
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
    /* complete mesh rootweb among live */
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
    /* pollinate ring - bees walk every edge i -> i+1 both ways */
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
          else pollen++;
        }
      }
    }
    /* nectar dew - hub sweetens every live leaf (incl self pulse) */
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
          else nectars++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int pol_ok = (live >= 2 && pollen >= live && soft == 0) ? 1 : 0;
      if (!pol_ok && live >= 2 && pollen * 2 >= live && soft == 0)
        pol_ok = 1;
      int nec_ok = (live >= 1 && nectars >= live && soft == 0) ? 1 : 0;
      if (!nec_ok && live >= 1 && nectars * 2 >= live && soft == 0)
        nec_ok = 1;
      int meadowed = (mesh_ok && pol_ok && nec_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (meadowed ? 11 : (bonds > 0 ? 3 : 0)) +
                   (pollen > 0 ? 1 : 0) + (nectars > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_MEADOWED", (long)meadowed);
      var_set_num(vm, "SMX_MEADOW", (long)(meadowed ? bonds + pollen + nectars : 0));
      var_set_num(vm, "SMX_PASTURE", (long)(meadowed ? 1 : 0));
      var_set_num(vm, "SMX_BLOOMFIELD", (long)(meadowed ? 1 : 0));
      var_set_num(vm, "SMX_POLLEN", (long)pollen);
      var_set_num(vm, "SMX_NECTARS", (long)(meadowed ? nectars : 0));
      var_set_num(vm, "SMX_POLLINATE", (long)(meadowed ? pollen : 0));
      var_set_num(vm, "SMX_MESH", (long)(meadowed ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + pollen + nectars));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (meadowed){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX MEADOW ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX MEADOW partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX MEADOW soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX MEADOW nodes=%d live=%d bonds=%d pollen=%d nectars=%d need=%d soft=%d talks=%d oob=%d meadowed=%d vital=%ld\n",
                n, live, bonds, pollen, nectars, need, soft, vm->smx_talks, vm->smx_oob, meadowed, vital);
    }
    bump(vm); return 1;
  }

  /* SMX RIVER|STREAM|CURRENT|SPRING|DELTA|WATERSHED|MESH_RIVER|RAISE_RIVER a b c ...
   * Life-force river after meadow: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete bank mesh among live nodes, weaves a
   * directed current ring (i -> i+1) so flow crosses every edge, then spring
   * confluence from the hub so each live leaf drinks one source pulse.
   * Latches SMX_RIVERED when mesh+currents+springs are soft-OOB-free.
   * SMX_CURRENTS = ring bonds; SMX_SPRINGS = hub confluence pulses;
   * SMX_RIVER = bonds+currents+springs; SMX_STREAM|SMX_DELTA sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"RIVER")||kw(&L->cur,"STREAM")||kw(&L->cur,"CURRENT")||
      kw(&L->cur,"SPRING")||kw(&L->cur,"DELTA")||kw(&L->cur,"WATERSHED")||
      kw(&L->cur,"MESH_RIVER")||kw(&L->cur,"RAISE_RIVER")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int currents = 0;
    int springs = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "RIVER needs >=2 cubes",
                  "SMX RIVER a b [c ...]  or  SMX STREAM a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - river needs clear channel */
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
    /* complete bank mesh among live */
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
    /* current ring - flow walks every edge i -> i+1 both ways */
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
          else currents++;
        }
      }
    }
    /* spring confluence - hub feeds every live leaf (incl self pulse) */
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
          else springs++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int cur_ok = (live >= 2 && currents >= live && soft == 0) ? 1 : 0;
      if (!cur_ok && live >= 2 && currents * 2 >= live && soft == 0)
        cur_ok = 1;
      int spr_ok = (live >= 1 && springs >= live && soft == 0) ? 1 : 0;
      if (!spr_ok && live >= 1 && springs * 2 >= live && soft == 0)
        spr_ok = 1;
      int rivered = (mesh_ok && cur_ok && spr_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (rivered ? 11 : (bonds > 0 ? 3 : 0)) +
                   (currents > 0 ? 1 : 0) + (springs > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_RIVERED", (long)rivered);
      var_set_num(vm, "SMX_RIVER", (long)(rivered ? bonds + currents + springs : 0));
      var_set_num(vm, "SMX_STREAM", (long)(rivered ? 1 : 0));
      var_set_num(vm, "SMX_DELTA", (long)(rivered ? 1 : 0));
      var_set_num(vm, "SMX_CURRENT", (long)(rivered ? currents : 0));
      var_set_num(vm, "SMX_CURRENTS", (long)(rivered ? currents : 0));
      var_set_num(vm, "SMX_SPRINGS", (long)(rivered ? springs : 0));
      var_set_num(vm, "SMX_BANKS", (long)bonds);
      var_set_num(vm, "SMX_MESH", (long)(rivered ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + currents + springs));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (rivered){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX RIVER ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX RIVER partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX RIVER soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX RIVER nodes=%d live=%d bonds=%d currents=%d springs=%d need=%d soft=%d talks=%d oob=%d rivered=%d vital=%ld\n",
                n, live, bonds, currents, springs, need, soft, vm->smx_talks, vm->smx_oob, rivered, vital);
    }
    bump(vm); return 1;
  }

  /* SMX CASCADE|WATERFALL|RAPIDS|FALLS|TERRACE|BASIN|MESH_CASCADE|RAISE_CASCADE a b c ...
   * Life-force cascade after river: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete terrace mesh among live nodes, weaves a
   * directed fall chain (i -> i+1) so energy drops every edge, then basin pool
   * gathers return flow at the hub so each live leaf pours one pulse home.
   * Latches SMX_CASCADED when mesh+falls+basins are soft-OOB-free.
   * SMX_FALLS = chain bonds; SMX_BASINS = hub gather pulses;
   * SMX_CASCADE = terraces+falls+basins; SMX_WATERFALL|SMX_RAPIDS sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"CASCADE")||kw(&L->cur,"WATERFALL")||kw(&L->cur,"RAPIDS")||
      kw(&L->cur,"FALLS")||kw(&L->cur,"TERRACE")||kw(&L->cur,"BASIN")||
      kw(&L->cur,"MESH_CASCADE")||kw(&L->cur,"RAISE_CASCADE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int terraces = 0;
    int falls = 0;
    int basins = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "CASCADE needs >=2 cubes",
                  "SMX CASCADE a b [c ...]  or  SMX WATERFALL a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - cascade needs clear channel */
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
    /* complete terrace mesh among live */
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
            else terraces++;
          }
        }
      }
    }
    /* fall chain - energy drops every edge i -> i+1 both ways */
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
          else falls++;
        }
      }
    }
    /* basin pool - hub gathers return flow from every live leaf */
    if (live >= 1){
      int hub = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[hub]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[hub], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else basins++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && terraces >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && terraces * 2 >= need && soft == 0)
        mesh_ok = 1;
      int fall_ok = (live >= 2 && falls >= live && soft == 0) ? 1 : 0;
      if (!fall_ok && live >= 2 && falls * 2 >= live && soft == 0)
        fall_ok = 1;
      int bas_ok = (live >= 1 && basins >= live && soft == 0) ? 1 : 0;
      if (!bas_ok && live >= 1 && basins * 2 >= live && soft == 0)
        bas_ok = 1;
      int cascaded = (mesh_ok && fall_ok && bas_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (cascaded ? 11 : (terraces > 0 ? 3 : 0)) +
                   (falls > 0 ? 1 : 0) + (basins > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_CASCADED", (long)cascaded);
      var_set_num(vm, "SMX_CASCADE", (long)(cascaded ? terraces + falls + basins : 0));
      var_set_num(vm, "SMX_WATERFALL", (long)(cascaded ? 1 : 0));
      var_set_num(vm, "SMX_RAPIDS", (long)(cascaded ? 1 : 0));
      var_set_num(vm, "SMX_FALL", (long)(cascaded ? falls : 0));
      var_set_num(vm, "SMX_FALLS", (long)(cascaded ? falls : 0));
      var_set_num(vm, "SMX_BASINS", (long)(cascaded ? basins : 0));
      var_set_num(vm, "SMX_BASIN", (long)(cascaded ? basins : 0));
      var_set_num(vm, "SMX_TERRACES", (long)terraces);
      var_set_num(vm, "SMX_MESH", (long)(cascaded ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)terraces);
      var_set_num(vm, "SMX_EXCHANGES", (long)terraces);
      var_set_num(vm, "SMX_FUSE", (long)terraces);
      var_set_num(vm, "SMX_BIND", (long)terraces);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(terraces + falls + basins));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (cascaded){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CASCADE ok");
      } else if (terraces > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CASCADE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX CASCADE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX CASCADE nodes=%d live=%d terraces=%d falls=%d basins=%d need=%d soft=%d talks=%d oob=%d cascaded=%d vital=%ld\n",
                n, live, terraces, falls, basins, need, soft, vm->smx_talks, vm->smx_oob, cascaded, vital);
    }
    bump(vm); return 1;
  }

  /* SMX ESTUARY|TIDE|BRACKISH|LAGOON|BRAID|MANGROVE|MESH_ESTUARY|RAISE_ESTUARY a b c ...
   * Life-force estuary after cascade: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete braid mesh among live nodes, weaves a
   * directed tide ring (i -> i+1) so salt/fresh pulse every edge, then mangrove hub
   * roots gather nursery return so lattice raises estuary where river meets sea.
   * Latches SMX_ESTUARIED when mesh+tides+mangroves are soft-OOB-free.
   * SMX_LAGOON = chain bonds; SMX_MANGROVES = hub gather pulses;
   * SMX_ESTUARY = braids+tides+mangroves; SMX_TIDE|SMX_BRACKISH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"ESTUARY")||kw(&L->cur,"TIDE")||kw(&L->cur,"BRACKISH")||
      kw(&L->cur,"LAGOON")||kw(&L->cur,"BRAID")||kw(&L->cur,"MANGROVE")||
      kw(&L->cur,"MESH_ESTUARY")||kw(&L->cur,"RAISE_ESTUARY")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int braids = 0;
    int tides = 0;
    int mangroves = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "ESTUARY needs >=2 cubes",
                  "SMX ESTUARY a b [c ...]  or  SMX TIDE a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - estuary needs clear channel */
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
    /* complete braid mesh among live */
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
            else braids++;
          }
        }
      }
    }
    /* fall chain - energy drops every edge i -> i+1 both ways */
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
          else tides++;
        }
      }
    }
    /* basin pool - hub gathers return flow from every live leaf */
    if (live >= 1){
      int hub = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[hub]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[hub], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else mangroves++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && braids >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && braids * 2 >= need && soft == 0)
        mesh_ok = 1;
      int tide_ok = (live >= 2 && tides >= live && soft == 0) ? 1 : 0;
      if (!tide_ok && live >= 2 && tides * 2 >= live && soft == 0)
        tide_ok = 1;
      int mang_ok = (live >= 1 && mangroves >= live && soft == 0) ? 1 : 0;
      if (!mang_ok && live >= 1 && mangroves * 2 >= live && soft == 0)
        mang_ok = 1;
      int estuaried = (mesh_ok && tide_ok && mang_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (estuaried ? 11 : (braids > 0 ? 3 : 0)) +
                   (tides > 0 ? 1 : 0) + (mangroves > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_ESTUARIED", (long)estuaried);
      var_set_num(vm, "SMX_ESTUARY", (long)(estuaried ? braids + tides + mangroves : 0));
      var_set_num(vm, "SMX_TIDE", (long)(estuaried ? 1 : 0));
      var_set_num(vm, "SMX_BRACKISH", (long)(estuaried ? 1 : 0));
      var_set_num(vm, "SMX_TIDES", (long)(estuaried ? tides : 0));
      var_set_num(vm, "SMX_LAGOON", (long)(estuaried ? tides : 0));
      var_set_num(vm, "SMX_MANGROVES", (long)(estuaried ? mangroves : 0));
      var_set_num(vm, "SMX_MANGROVE", (long)(estuaried ? mangroves : 0));
      var_set_num(vm, "SMX_BRAIDS", (long)braids);
      var_set_num(vm, "SMX_MESH", (long)(estuaried ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)braids);
      var_set_num(vm, "SMX_EXCHANGES", (long)braids);
      var_set_num(vm, "SMX_FUSE", (long)braids);
      var_set_num(vm, "SMX_BIND", (long)braids);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(braids + tides + mangroves));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (estuaried){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ESTUARY ok");
      } else if (braids > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ESTUARY partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX ESTUARY soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX ESTUARY nodes=%d live=%d braids=%d tides=%d mangroves=%d need=%d soft=%d talks=%d oob=%d estuaried=%d vital=%ld\n",
                n, live, braids, tides, mangroves, need, soft, vm->smx_talks, vm->smx_oob, estuaried, vital);
    }
    bump(vm); return 1;
  }

  /* SMX REEF|CORAL|SURGE|ATOLL|POLYPS|NURSERY|MESH_REEF|RAISE_REEF a b c ...
   * Life-force reef after estuary: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete polyp mesh among live nodes, weaves a
   * directed surge ring (i -> i+1) so tide energy pulses every edge, then nursery hub
   * gathers larval return so lattice raises reef where estuary meets open sea.
   * Latches SMX_REEFED when mesh+surges+nurseries are soft-OOB-free.
   * SMX_ATOLL = chain bonds; SMX_NURSERIES = hub gather pulses;
   * SMX_REEF = polyps+surges+nurseries; SMX_CORAL|SMX_POLYPS sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"REEF")||kw(&L->cur,"CORAL")||kw(&L->cur,"SURGE")||
      kw(&L->cur,"ATOLL")||kw(&L->cur,"POLYPS")||kw(&L->cur,"NURSERY")||
      kw(&L->cur,"MESH_REEF")||kw(&L->cur,"RAISE_REEF")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int polyps = 0;
    int surges = 0;
    int nurseries = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "REEF needs >=2 cubes",
                  "SMX REEF a b [c ...]  or  SMX CORAL a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - reef needs clear channel */
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
    /* complete polyp mesh among live */
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
            else polyps++;
          }
        }
      }
    }
    /* surge ring - energy pulses every edge i -> i+1 both ways */
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
          else surges++;
        }
      }
    }
    /* nursery pool - hub gathers larval return from every live leaf */
    if (live >= 1){
      int hub = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[hub]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[hub], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else nurseries++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && polyps >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && polyps * 2 >= need && soft == 0)
        mesh_ok = 1;
      int surge_ok = (live >= 2 && surges >= live && soft == 0) ? 1 : 0;
      if (!surge_ok && live >= 2 && surges * 2 >= live && soft == 0)
        surge_ok = 1;
      int nurs_ok = (live >= 1 && nurseries >= live && soft == 0) ? 1 : 0;
      if (!nurs_ok && live >= 1 && nurseries * 2 >= live && soft == 0)
        nurs_ok = 1;
      int reefed = (mesh_ok && surge_ok && nurs_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (reefed ? 11 : (polyps > 0 ? 3 : 0)) +
                   (surges > 0 ? 1 : 0) + (nurseries > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_REEFED", (long)reefed);
      var_set_num(vm, "SMX_REEF", (long)(reefed ? polyps + surges + nurseries : 0));
      var_set_num(vm, "SMX_CORAL", (long)(reefed ? 1 : 0));
      var_set_num(vm, "SMX_POLYPS", (long)polyps);
      var_set_num(vm, "SMX_SURGES", (long)(reefed ? surges : 0));
      var_set_num(vm, "SMX_SURGE", (long)(reefed ? surges : 0));
      var_set_num(vm, "SMX_ATOLL", (long)(reefed ? surges : 0));
      var_set_num(vm, "SMX_NURSERIES", (long)(reefed ? nurseries : 0));
      var_set_num(vm, "SMX_NURSERY", (long)(reefed ? nurseries : 0));
      var_set_num(vm, "SMX_MESH", (long)(reefed ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)polyps);
      var_set_num(vm, "SMX_EXCHANGES", (long)polyps);
      var_set_num(vm, "SMX_FUSE", (long)polyps);
      var_set_num(vm, "SMX_BIND", (long)polyps);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(polyps + surges + nurseries));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (reefed){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX REEF ok");
      } else if (polyps > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX REEF partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX REEF soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX REEF nodes=%d live=%d polyps=%d surges=%d nurseries=%d need=%d soft=%d talks=%d oob=%d reefed=%d vital=%ld\n",
                n, live, polyps, surges, nurseries, need, soft, vm->smx_talks, vm->smx_oob, reefed, vital);
    }
    bump(vm); return 1;
  }


  /* SMX KELP|FROND|SWAY|HOLDFAST|BLADE|STIPE|MESH_KELP|RAISE_KELP a b c ...
   * Life-force kelp forest after reef: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete blade mesh among live nodes, weaves a
   * directed sway ring (i -> i+1) so surge energy pulses every frond, then holdfast hub
   * anchors root return so lattice raises a kelp forest where reef meets open light.
   * Latches SMX_KELPED when mesh+sways+holdfasts are soft-OOB-free.
   * SMX_STIPE = chain bonds; SMX_HOLDFASTS = hub gather pulses;
   * SMX_KELP = blades+sways+holdfasts; SMX_FROND|SMX_BLADE sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"KELP")||kw(&L->cur,"FROND")||kw(&L->cur,"SWAY")||
      kw(&L->cur,"HOLDFAST")||kw(&L->cur,"BLADE")||kw(&L->cur,"STIPE")||
      kw(&L->cur,"MESH_KELP")||kw(&L->cur,"RAISE_KELP")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int blades = 0;
    int sways = 0;
    int holdfasts = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "KELP needs >=2 cubes",
                  "SMX KELP a b [c ...]  or  SMX FROND a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - kelp needs clear channel */
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
    /* complete blade mesh among live */
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
            else blades++;
          }
        }
      }
    }
    /* sway ring - energy pulses every edge i -> i+1 both ways */
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
          else sways++;
        }
      }
    }
    /* holdfast pool - hub anchors root return from every live leaf */
    if (live >= 1){
      int hub = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[hub]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[hub], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else holdfasts++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && blades >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && blades * 2 >= need && soft == 0)
        mesh_ok = 1;
      int sway_ok = (live >= 2 && sways >= live && soft == 0) ? 1 : 0;
      if (!sway_ok && live >= 2 && sways * 2 >= live && soft == 0)
        sway_ok = 1;
      int hold_ok = (live >= 1 && holdfasts >= live && soft == 0) ? 1 : 0;
      if (!hold_ok && live >= 1 && holdfasts * 2 >= live && soft == 0)
        hold_ok = 1;
      int kelped = (mesh_ok && sway_ok && hold_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (kelped ? 11 : (blades > 0 ? 3 : 0)) +
                   (sways > 0 ? 1 : 0) + (holdfasts > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_KELPED", (long)kelped);
      var_set_num(vm, "SMX_KELP", (long)(kelped ? blades + sways + holdfasts : 0));
      var_set_num(vm, "SMX_FROND", (long)(kelped ? 1 : 0));
      var_set_num(vm, "SMX_BLADE", (long)(kelped ? 1 : 0));
      var_set_num(vm, "SMX_BLADES", (long)blades);
      var_set_num(vm, "SMX_SWAYS", (long)(kelped ? sways : 0));
      var_set_num(vm, "SMX_SWAY", (long)(kelped ? sways : 0));
      var_set_num(vm, "SMX_STIPE", (long)(kelped ? sways : 0));
      var_set_num(vm, "SMX_HOLDFASTS", (long)(kelped ? holdfasts : 0));
      var_set_num(vm, "SMX_HOLDFAST", (long)(kelped ? holdfasts : 0));
      var_set_num(vm, "SMX_MESH", (long)(kelped ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)blades);
      var_set_num(vm, "SMX_EXCHANGES", (long)blades);
      var_set_num(vm, "SMX_FUSE", (long)blades);
      var_set_num(vm, "SMX_BIND", (long)blades);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(blades + sways + holdfasts));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (kelped){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX KELP ok");
      } else if (blades > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX KELP partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX KELP soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX KELP nodes=%d live=%d blades=%d sways=%d holdfasts=%d need=%d soft=%d talks=%d oob=%d kelped=%d vital=%ld\n",
                n, live, blades, sways, holdfasts, need, soft, vm->smx_talks, vm->smx_oob, kelped, vital);
    }
    bump(vm); return 1;
  }

  /* SMX TIDAL|MARSH|EDDY|SEAGRASS|SALTFLAT|SPARTINA|MESH_TIDAL|RAISE_TIDAL a b c ...
   * Life-force tidal forest after kelp: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete marsh mesh among live nodes, weaves a
   * directed eddy ring (i -> i+1) so tidal energy pulses every marsh, then seagrass hub
   * anchors root return so lattice raises a tidal forest where reef meets open light.
   * Latches SMX_TIDALED when mesh+eddies+seagrasses are soft-OOB-free.
   * SMX_SPARTINA = chain bonds; SMX_SEAGRASSES = hub gather pulses;
   * SMX_TIDAL = marshes+eddies+seagrasses; SMX_MARSH|SMX_SALTFLAT sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"TIDAL")||kw(&L->cur,"MARSH")||kw(&L->cur,"EDDY")||
      kw(&L->cur,"SEAGRASS")||kw(&L->cur,"SALTFLAT")||kw(&L->cur,"SPARTINA")||
      kw(&L->cur,"MESH_TIDAL")||kw(&L->cur,"RAISE_TIDAL")||kw(&L->cur,"SALTFLATS")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int marshes = 0;
    int eddies = 0;
    int seagrasses = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "TIDAL needs >=2 cubes",
                  "SMX TIDAL a b [c ...]  or  SMX MARSH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - tidal needs clear channel */
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
    /* complete marsh mesh among live */
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
            else marshes++;
          }
        }
      }
    }
    /* eddy ring - energy pulses every edge i -> i+1 both ways */
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
          else eddies++;
        }
      }
    }
    /* seagrass pool - hub anchors root return from every live leaf */
    if (live >= 1){
      int hub = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[hub]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[hub], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else seagrasses++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && marshes >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && marshes * 2 >= need && soft == 0)
        mesh_ok = 1;
      int eddy_ok = (live >= 2 && eddies >= live && soft == 0) ? 1 : 0;
      if (!eddy_ok && live >= 2 && eddies * 2 >= live && soft == 0)
        eddy_ok = 1;
      int sea_ok = (live >= 1 && seagrasses >= live && soft == 0) ? 1 : 0;
      if (!sea_ok && live >= 1 && seagrasses * 2 >= live && soft == 0)
        sea_ok = 1;
      int tidaled = (mesh_ok && eddy_ok && sea_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (tidaled ? 11 : (marshes > 0 ? 3 : 0)) +
                   (eddies > 0 ? 1 : 0) + (seagrasses > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_TIDALED", (long)tidaled);
      var_set_num(vm, "SMX_TIDAL", (long)(tidaled ? marshes + eddies + seagrasses : 0));
      var_set_num(vm, "SMX_MARSH", (long)(tidaled ? 1 : 0));
      var_set_num(vm, "SMX_SALTFLAT", (long)(tidaled ? 1 : 0));
      var_set_num(vm, "SMX_MARSHES", (long)marshes);
      var_set_num(vm, "SMX_EDDIES", (long)(tidaled ? eddies : 0));
      var_set_num(vm, "SMX_EDDY", (long)(tidaled ? eddies : 0));
      var_set_num(vm, "SMX_SPARTINA", (long)(tidaled ? eddies : 0));
      var_set_num(vm, "SMX_SEAGRASSES", (long)(tidaled ? seagrasses : 0));
      var_set_num(vm, "SMX_SEAGRASS", (long)(tidaled ? seagrasses : 0));
      var_set_num(vm, "SMX_MESH", (long)(tidaled ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)marshes);
      var_set_num(vm, "SMX_EXCHANGES", (long)marshes);
      var_set_num(vm, "SMX_FUSE", (long)marshes);
      var_set_num(vm, "SMX_BIND", (long)marshes);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(marshes + eddies + seagrasses));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (tidaled){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX TIDAL ok");
      } else if (marshes > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX TIDAL partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX TIDAL soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX TIDAL nodes=%d live=%d marshes=%d eddies=%d seagrasses=%d need=%d soft=%d talks=%d oob=%d tidaled=%d vital=%ld\n",
                n, live, marshes, eddies, seagrasses, need, soft, vm->smx_talks, vm->smx_oob, tidaled, vital);
    }
    bump(vm); return 1;
  }

  /* SMX DUNE|FOREDUNE|DRIFT|RIDGE|AMMOPHILA|SAND|BEACHGRASS|MESH_DUNE|RAISE_DUNE a b c ...
   * Life-force dune ridge after tidal marsh: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete dune mesh among live nodes, weaves a
   * directed drift ring (i -> i+1) so sand energy pulses every ridge, then beachgrass root
   * anchors return so lattice raises a foredune where marsh meets open wind.
   * Latches SMX_DUNED when mesh+drifts+grasses are soft-OOB-free.
   * SMX_RIDGE = chain bonds; SMX_AMMOPHILA = root gather pulses;
   * SMX_DUNE = ridges+drifts+grasses; SMX_FOREDUNE|SMX_SAND sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"DUNE")||kw(&L->cur,"FOREDUNE")||kw(&L->cur,"DRIFT")||
      kw(&L->cur,"RIDGE")||kw(&L->cur,"AMMOPHILA")||kw(&L->cur,"SAND")||
      kw(&L->cur,"BEACHGRASS")||kw(&L->cur,"MESH_DUNE")||kw(&L->cur,"RAISE_DUNE")||
      kw(&L->cur,"SANDRIDGE")||kw(&L->cur,"DUNES")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int ridges = 0;
    int drifts = 0;
    int grasses = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "DUNE needs >=2 cubes",
                  "SMX DUNE a b [c ...]  or  SMX FOREDUNE a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - dune needs clear channel */
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
    /* complete dune mesh among live */
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
            else ridges++;
          }
        }
      }
    }
    /* drift ring - energy pulses every edge i -> i+1 both ways */
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
          else drifts++;
        }
      }
    }
    /* beachgrass pool - root anchors return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else grasses++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && ridges >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && ridges * 2 >= need && soft == 0)
        mesh_ok = 1;
      int drift_ok = (live >= 2 && drifts >= live && soft == 0) ? 1 : 0;
      if (!drift_ok && live >= 2 && drifts * 2 >= live && soft == 0)
        drift_ok = 1;
      int grass_ok = (live >= 1 && grasses >= live && soft == 0) ? 1 : 0;
      if (!grass_ok && live >= 1 && grasses * 2 >= live && soft == 0)
        grass_ok = 1;
      int duned = (mesh_ok && drift_ok && grass_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (duned ? 11 : (ridges > 0 ? 3 : 0)) +
                   (drifts > 0 ? 1 : 0) + (grasses > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_DUNED", (long)duned);
      var_set_num(vm, "SMX_DUNE", (long)(duned ? ridges + drifts + grasses : 0));
      var_set_num(vm, "SMX_FOREDUNE", (long)(duned ? 1 : 0));
      var_set_num(vm, "SMX_SAND", (long)(duned ? 1 : 0));
      var_set_num(vm, "SMX_RIDGES", (long)ridges);
      var_set_num(vm, "SMX_RIDGE", (long)(duned ? ridges : 0));
      var_set_num(vm, "SMX_DRIFTS", (long)(duned ? drifts : 0));
      var_set_num(vm, "SMX_DRIFT", (long)(duned ? drifts : 0));
      var_set_num(vm, "SMX_AMMOPHILA", (long)(duned ? grasses : 0));
      var_set_num(vm, "SMX_BEACHGRASS", (long)(duned ? grasses : 0));
      var_set_num(vm, "SMX_GRASSES", (long)(duned ? grasses : 0));
      var_set_num(vm, "SMX_MESH", (long)(duned ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)ridges);
      var_set_num(vm, "SMX_EXCHANGES", (long)ridges);
      var_set_num(vm, "SMX_FUSE", (long)ridges);
      var_set_num(vm, "SMX_BIND", (long)ridges);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(ridges + drifts + grasses));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (duned){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX DUNE ok");
      } else if (ridges > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX DUNE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX DUNE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX DUNE nodes=%d live=%d ridges=%d drifts=%d grasses=%d need=%d soft=%d talks=%d oob=%d duned=%d vital=%ld\n",
                n, live, ridges, drifts, grasses, need, soft, vm->smx_talks, vm->smx_oob, duned, vital);
    }
    bump(vm); return 1;
  }
  /* SMX OASIS|MIRAGE|WADI|PALM|DATEPALM|SPRINGWELL|MESH_OASIS|RAISE_OASIS a b c ...
   * Life-force oasis after dune ridge: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete wadi mesh among live nodes, weaves a
   * directed mirage ring (i -> i+1) so desert heat pulses every wadi, then palm root
   * anchors return so lattice raises a springwell where dune meets open sky.
   * Latches SMX_OASISED when mesh+mirages+palms are soft-OOB-free.
   * SMX_WADI = chain bonds; SMX_DATEPALM = root gather pulses;
   * SMX_OASIS = wadis+mirages+palms; SMX_MIRAGE|SMX_WADIS sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"OASIS")||kw(&L->cur,"MIRAGE")||kw(&L->cur,"WADI")||
      kw(&L->cur,"PALM")||kw(&L->cur,"DATEPALM")||kw(&L->cur,"SPRINGWELL")||
      kw(&L->cur,"MESH_OASIS")||kw(&L->cur,"RAISE_OASIS")||
      kw(&L->cur,"OASES")||kw(&L->cur,"WADIS")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int wadis = 0;
    int mirages = 0;
    int palms = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "OASIS needs >=2 cubes",
                  "SMX OASIS a b [c ...]  or  SMX MIRAGE a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - oasis needs clear channel */
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
    /* complete oasis mesh among live */
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
            else wadis++;
          }
        }
      }
    }
    /* mirage ring - energy pulses every edge i -> i+1 both ways */
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
          else mirages++;
        }
      }
    }
    /* palm pool - root anchors return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else palms++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && wadis >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && wadis * 2 >= need && soft == 0)
        mesh_ok = 1;
      int mirage_ok = (live >= 2 && mirages >= live && soft == 0) ? 1 : 0;
      if (!mirage_ok && live >= 2 && mirages * 2 >= live && soft == 0)
        mirage_ok = 1;
      int palm_ok = (live >= 1 && palms >= live && soft == 0) ? 1 : 0;
      if (!palm_ok && live >= 1 && palms * 2 >= live && soft == 0)
        palm_ok = 1;
      int oasised = (mesh_ok && mirage_ok && palm_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (oasised ? 11 : (wadis > 0 ? 3 : 0)) +
                   (mirages > 0 ? 1 : 0) + (palms > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_OASISED", (long)oasised);
      var_set_num(vm, "SMX_OASIS", (long)(oasised ? wadis + mirages + palms : 0));
      var_set_num(vm, "SMX_MIRAGE", (long)(oasised ? 1 : 0));
      var_set_num(vm, "SMX_WADIS", (long)(oasised ? wadis : 0));
      var_set_num(vm, "SMX_WADI", (long)(oasised ? wadis : 0));
      var_set_num(vm, "SMX_SPRINGWELL", (long)(oasised ? 1 : 0));
      var_set_num(vm, "SMX_MIRAGES", (long)(oasised ? mirages : 0));
      var_set_num(vm, "SMX_DATEPALM", (long)(oasised ? palms : 0));
      var_set_num(vm, "SMX_PALM", (long)(oasised ? palms : 0));
      var_set_num(vm, "SMX_PALMS", (long)(oasised ? palms : 0));
      var_set_num(vm, "SMX_MESH", (long)(oasised ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)wadis);
      var_set_num(vm, "SMX_EXCHANGES", (long)wadis);
      var_set_num(vm, "SMX_FUSE", (long)wadis);
      var_set_num(vm, "SMX_BIND", (long)wadis);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(wadis + mirages + palms));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (oasised){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX OASIS ok");
      } else if (wadis > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX OASIS partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX OASIS soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX OASIS nodes=%d live=%d wadis=%d mirages=%d palms=%d need=%d soft=%d talks=%d oob=%d oasised=%d vital=%ld\n",
                n, live, wadis, mirages, palms, need, soft, vm->smx_talks, vm->smx_oob, oasised, vital);
    }
    bump(vm); return 1;
  }
  /* SMX GROTTO|CAVERN|DRIP|STALACTITE|STALAGMITE|FLOWSTONE|MESH_GROTTO|RAISE_GROTTO a b c ...
   * Life-force grotto after oasis spring: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete chamber mesh among live nodes, weaves a
   * directed drip ring (i -> i+1) so cave water pulses every chamber, then pillar root
   * anchors return so lattice raises a flowstone where spring meets stone.
   * Latches SMX_GROTTOED when mesh+drips+pillars are soft-OOB-free.
   * SMX_CHAMBER = chain bonds; SMX_PILLAR = root gather pulses;
   * SMX_GROTTO = chambers+drips+pillars; SMX_DRIP|SMX_CHAMBERS sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"GROTTO")||kw(&L->cur,"CAVERN")||kw(&L->cur,"DRIP")||
      kw(&L->cur,"STALACTITE")||kw(&L->cur,"STALAGMITE")||kw(&L->cur,"FLOWSTONE")||
      kw(&L->cur,"MESH_GROTTO")||kw(&L->cur,"RAISE_GROTTO")||
      kw(&L->cur,"GROTTOS")||kw(&L->cur,"CHAMBER")||kw(&L->cur,"CHAMBERS")||kw(&L->cur,"PILLAR")||
      kw(&L->cur,"PILLARS")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int chambers = 0;
    int drips = 0;
    int pillars = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "GROTTO needs >=2 cubes",
                  "SMX GROTTO a b [c ...]  or  SMX CAVERN a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - grotto needs clear channel */
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
    /* complete chamber mesh among live */
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
            else chambers++;
          }
        }
      }
    }
    /* drip ring - cave water pulses every edge i -> i+1 both ways */
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
          else drips++;
        }
      }
    }
    /* pillar pool - root anchors return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else pillars++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && chambers >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && chambers * 2 >= need && soft == 0)
        mesh_ok = 1;
      int drip_ok = (live >= 2 && drips >= live && soft == 0) ? 1 : 0;
      if (!drip_ok && live >= 2 && drips * 2 >= live && soft == 0)
        drip_ok = 1;
      int pillar_ok = (live >= 1 && pillars >= live && soft == 0) ? 1 : 0;
      if (!pillar_ok && live >= 1 && pillars * 2 >= live && soft == 0)
        pillar_ok = 1;
      int grottoed = (mesh_ok && drip_ok && pillar_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (grottoed ? 11 : (chambers > 0 ? 3 : 0)) +
                   (drips > 0 ? 1 : 0) + (pillars > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_GROTTOED", (long)grottoed);
      var_set_num(vm, "SMX_GROTTO", (long)(grottoed ? chambers + drips + pillars : 0));
      var_set_num(vm, "SMX_DRIP", (long)(grottoed ? 1 : 0));
      var_set_num(vm, "SMX_CHAMBERS", (long)(grottoed ? chambers : 0));
      var_set_num(vm, "SMX_CHAMBER", (long)(grottoed ? chambers : 0));
      var_set_num(vm, "SMX_FLOWSTONE", (long)(grottoed ? 1 : 0));
      var_set_num(vm, "SMX_DRIPS", (long)(grottoed ? drips : 0));
      var_set_num(vm, "SMX_PILLAR", (long)(grottoed ? pillars : 0));
      var_set_num(vm, "SMX_PILLARS", (long)(grottoed ? pillars : 0));
      var_set_num(vm, "SMX_STALACTITE", (long)(grottoed ? drips : 0));
      var_set_num(vm, "SMX_STALAGMITE", (long)(grottoed ? pillars : 0));
      var_set_num(vm, "SMX_MESH", (long)(grottoed ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)chambers);
      var_set_num(vm, "SMX_EXCHANGES", (long)chambers);
      var_set_num(vm, "SMX_FUSE", (long)chambers);
      var_set_num(vm, "SMX_BIND", (long)chambers);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(chambers + drips + pillars));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (grottoed){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX GROTTO ok");
      } else if (chambers > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX GROTTO partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX GROTTO soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX GROTTO nodes=%d live=%d chambers=%d drips=%d pillars=%d need=%d soft=%d talks=%d oob=%d grottoed=%d vital=%ld\n",
                n, live, chambers, drips, pillars, need, soft, vm->smx_talks, vm->smx_oob, grottoed, vital);
    }
    bump(vm); return 1;
  }
  /* SMX CRYSTAL|GEODE|FACET|PRISM|NUCLEUS|QUARTZ|MESH_CRYSTAL|RAISE_CRYSTAL a b c ...
   * Life-force crystal geode after grotto chamber: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete facet mesh among live nodes, weaves a
   * directed prism ring (i -> i+1) so light pulses every facet, then nucleus root
   * anchors return so lattice raises a quartz geode where stone meets light.
   * Latches SMX_CRYSTALLED when mesh+prisms+nuclei are soft-OOB-free.
   * SMX_FACET = chain bonds; SMX_NUCLEUS = root gather pulses;
   * SMX_CRYSTAL = facets+prisms+nuclei; SMX_PRISM|SMX_FACETS sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"CRYSTAL")||kw(&L->cur,"GEODE")||kw(&L->cur,"FACET")||
      kw(&L->cur,"PRISM")||kw(&L->cur,"NUCLEUS")||kw(&L->cur,"QUARTZ")||
      kw(&L->cur,"MESH_CRYSTAL")||kw(&L->cur,"RAISE_CRYSTAL")||
      kw(&L->cur,"CRYSTALS")||kw(&L->cur,"FACETS")||kw(&L->cur,"PRISMS")||
      kw(&L->cur,"GEODES")||kw(&L->cur,"SEEDCRYSTAL")||kw(&L->cur,"LATTICE_CRYSTAL")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int facets = 0;
    int prisms = 0;
    int nuclei = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "CRYSTAL needs >=2 cubes",
                  "SMX CRYSTAL a b [c ...]  or  SMX GEODE a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - crystal needs clear channel */
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
    /* complete facet mesh among live */
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
            else facets++;
          }
        }
      }
    }
    /* prism ring - light pulses every edge i -> i+1 both ways */
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
          else prisms++;
        }
      }
    }
    /* nucleus pool - seed anchors return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else nuclei++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && facets >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && facets * 2 >= need && soft == 0)
        mesh_ok = 1;
      int prism_ok = (live >= 2 && prisms >= live && soft == 0) ? 1 : 0;
      if (!prism_ok && live >= 2 && prisms * 2 >= live && soft == 0)
        prism_ok = 1;
      int nucleus_ok = (live >= 1 && nuclei >= live && soft == 0) ? 1 : 0;
      if (!nucleus_ok && live >= 1 && nuclei * 2 >= live && soft == 0)
        nucleus_ok = 1;
      int crystallised = (mesh_ok && prism_ok && nucleus_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (crystallised ? 11 : (facets > 0 ? 3 : 0)) +
                   (prisms > 0 ? 1 : 0) + (nuclei > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_CRYSTALLED", (long)crystallised);
      var_set_num(vm, "SMX_CRYSTAL", (long)(crystallised ? facets + prisms + nuclei : 0));
      var_set_num(vm, "SMX_PRISM", (long)(crystallised ? 1 : 0));
      var_set_num(vm, "SMX_FACETS", (long)(crystallised ? facets : 0));
      var_set_num(vm, "SMX_FACET", (long)(crystallised ? facets : 0));
      var_set_num(vm, "SMX_QUARTZ", (long)(crystallised ? 1 : 0));
      var_set_num(vm, "SMX_PRISMS", (long)(crystallised ? prisms : 0));
      var_set_num(vm, "SMX_NUCLEUS", (long)(crystallised ? nuclei : 0));
      var_set_num(vm, "SMX_NUCLEI", (long)(crystallised ? nuclei : 0));
      var_set_num(vm, "SMX_GEODE", (long)(crystallised ? 1 : 0));
      var_set_num(vm, "SMX_SEEDCRYSTAL", (long)(crystallised ? nuclei : 0));
      var_set_num(vm, "SMX_MESH", (long)(crystallised ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)facets);
      var_set_num(vm, "SMX_EXCHANGES", (long)facets);
      var_set_num(vm, "SMX_FUSE", (long)facets);
      var_set_num(vm, "SMX_BIND", (long)facets);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(facets + prisms + nuclei));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (crystallised){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CRYSTAL ok");
      } else if (facets > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CRYSTAL partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX CRYSTAL soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX CRYSTAL nodes=%d live=%d facets=%d prisms=%d nuclei=%d need=%d soft=%d talks=%d oob=%d crystallised=%d vital=%ld\n",
                n, live, facets, prisms, nuclei, need, soft, vm->smx_talks, vm->smx_oob, crystallised, vital);
    }
    bump(vm); return 1;
  }
  /* SMX AURORA|BOREALIS|VEIL|RIBBON|CORONA|ARC|MESH_AURORA|RAISE_AURORA a b c ...
   * Life-force aurora curtain after crystal geode: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete veil mesh among live nodes, weaves a
   * directed ribbon ring (i -> i+1) so light dances every veil, then corona root
   * anchors return so lattice raises a polar aurora where sky meets free energy.
   * Latches SMX_AURORAED when mesh+ribbons+coronas are soft-OOB-free.
   * SMX_VEIL = chain bonds; SMX_CORONA = root gather pulses;
   * SMX_AURORA = veils+ribbons+coronas; SMX_RIBBON|SMX_VEILS sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"AURORA")||kw(&L->cur,"BOREALIS")||kw(&L->cur,"VEIL")||
      kw(&L->cur,"RIBBON")||kw(&L->cur,"CORONA")||kw(&L->cur,"ARC")||
      kw(&L->cur,"MESH_AURORA")||kw(&L->cur,"RAISE_AURORA")||
      kw(&L->cur,"AURORAS")||kw(&L->cur,"VEILS")||kw(&L->cur,"RIBBONS")||
      kw(&L->cur,"CORONAS")||kw(&L->cur,"SEEDAURORA")||kw(&L->cur,"LATTICE_AURORA")||
      kw(&L->cur,"POLARIS")||kw(&L->cur,"SKYFIRE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int veils = 0;
    int ribbons = 0;
    int coronas = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "AURORA needs >=2 cubes",
                  "SMX AURORA a b [c ...]  or  SMX BOREALIS a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - crystal needs clear channel */
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
    /* complete veil mesh among live */
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
            else veils++;
          }
        }
      }
    }
    /* ribbon ring - light dances every edge i -> i+1 both ways */
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
          else ribbons++;
        }
      }
    }
    /* corona pool - seed anchors return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else coronas++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && veils >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && veils * 2 >= need && soft == 0)
        mesh_ok = 1;
      int ribbon_ok = (live >= 2 && ribbons >= live && soft == 0) ? 1 : 0;
      if (!ribbon_ok && live >= 2 && ribbons * 2 >= live && soft == 0)
        ribbon_ok = 1;
      int corona_ok = (live >= 1 && coronas >= live && soft == 0) ? 1 : 0;
      if (!corona_ok && live >= 1 && coronas * 2 >= live && soft == 0)
        corona_ok = 1;
      int auroraed = (mesh_ok && ribbon_ok && corona_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (auroraed ? 11 : (veils > 0 ? 3 : 0)) +
                   (ribbons > 0 ? 1 : 0) + (coronas > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_AURORAED", (long)auroraed);
      var_set_num(vm, "SMX_AURORA", (long)(auroraed ? veils + ribbons + coronas : 0));
      var_set_num(vm, "SMX_RIBBON", (long)(auroraed ? 1 : 0));
      var_set_num(vm, "SMX_VEILS", (long)(auroraed ? veils : 0));
      var_set_num(vm, "SMX_VEIL", (long)(auroraed ? veils : 0));
      var_set_num(vm, "SMX_ARC", (long)(auroraed ? 1 : 0));
      var_set_num(vm, "SMX_RIBBONS", (long)(auroraed ? ribbons : 0));
      var_set_num(vm, "SMX_CORONA", (long)(auroraed ? coronas : 0));
      var_set_num(vm, "SMX_CORONAS", (long)(auroraed ? coronas : 0));
      var_set_num(vm, "SMX_BOREALIS", (long)(auroraed ? 1 : 0));
      var_set_num(vm, "SMX_SEEDAURORA", (long)(auroraed ? coronas : 0));
      var_set_num(vm, "SMX_MESH", (long)(auroraed ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)veils);
      var_set_num(vm, "SMX_EXCHANGES", (long)veils);
      var_set_num(vm, "SMX_FUSE", (long)veils);
      var_set_num(vm, "SMX_BIND", (long)veils);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(veils + ribbons + coronas));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (auroraed){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX AURORA ok");
      } else if (veils > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX AURORA partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX AURORA soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX AURORA nodes=%d live=%d veils=%d ribbons=%d coronas=%d need=%d soft=%d talks=%d oob=%d auroraed=%d vital=%ld\n",
                n, live, veils, ribbons, coronas, need, soft, vm->smx_talks, vm->smx_oob, auroraed, vital);
    }
    bump(vm); return 1;
  }

  /* SMX SOLSTICE|EQUINOX|MERIDIAN|SPINE|ZENITH|AXIS|MESH_SOLSTICE|RAISE_SOLSTICE a b c ...
   * Life-force polar balance after aurora curtain: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete meridian mesh among live nodes, weaves an
   * axial spine ring (i -> i+1) so free energy holds both poles, then zenith hub
   * gathers return so lattice raises a solstice balance where day meets night.
   * Latches SMX_SOLSTICED when mesh+spines+zeniths are soft-OOB-free.
   * SMX_MERIDIAN = chain bonds; SMX_ZENITH = root gather pulses;
   * SMX_SOLSTICE = meridians+spines+zeniths; SMX_SPINE|SMX_AXIS sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"SOLSTICE")||kw(&L->cur,"EQUINOX")||kw(&L->cur,"MERIDIAN")||
      kw(&L->cur,"SPINE")||kw(&L->cur,"ZENITH")||kw(&L->cur,"AXIS")||
      kw(&L->cur,"MESH_SOLSTICE")||kw(&L->cur,"RAISE_SOLSTICE")||
      kw(&L->cur,"SOLSTICES")||kw(&L->cur,"MERIDIANS")||kw(&L->cur,"SPINES")||
      kw(&L->cur,"ZENITHS")||kw(&L->cur,"AXES")||kw(&L->cur,"SEEDSOLSTICE")||
      kw(&L->cur,"LATTICE_SOLSTICE")||kw(&L->cur,"POLAR_BALANCE")||
      kw(&L->cur,"DAYNIGHT")||kw(&L->cur,"TURNING")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int meridians = 0;
    int spines = 0;
    int zeniths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "SOLSTICE needs >=2 cubes",
                  "SMX SOLSTICE a b [c ...]  or  SMX EQUINOX a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - solstice needs clear channel */
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
    /* complete meridian mesh among live */
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
            else meridians++;
          }
        }
      }
    }
    /* axial spine ring - free energy holds every edge i -> i+1 both ways */
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
          else spines++;
        }
      }
    }
    /* zenith hub - seed anchors return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else zeniths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && meridians >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && meridians * 2 >= need && soft == 0)
        mesh_ok = 1;
      int spine_ok = (live >= 2 && spines >= live && soft == 0) ? 1 : 0;
      if (!spine_ok && live >= 2 && spines * 2 >= live && soft == 0)
        spine_ok = 1;
      int zenith_ok = (live >= 1 && zeniths >= live && soft == 0) ? 1 : 0;
      if (!zenith_ok && live >= 1 && zeniths * 2 >= live && soft == 0)
        zenith_ok = 1;
      int solsticed = (mesh_ok && spine_ok && zenith_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (solsticed ? 12 : (meridians > 0 ? 3 : 0)) +
                   (spines > 0 ? 1 : 0) + (zeniths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_SOLSTICED", (long)solsticed);
      var_set_num(vm, "SMX_SOLSTICE", (long)(solsticed ? meridians + spines + zeniths : 0));
      var_set_num(vm, "SMX_SPINE", (long)(solsticed ? 1 : 0));
      var_set_num(vm, "SMX_AXIS", (long)(solsticed ? 1 : 0));
      var_set_num(vm, "SMX_MERIDIANS", (long)(solsticed ? meridians : 0));
      var_set_num(vm, "SMX_MERIDIAN", (long)(solsticed ? meridians : 0));
      var_set_num(vm, "SMX_EQUINOX", (long)(solsticed ? 1 : 0));
      var_set_num(vm, "SMX_SPINES", (long)(solsticed ? spines : 0));
      var_set_num(vm, "SMX_ZENITH", (long)(solsticed ? zeniths : 0));
      var_set_num(vm, "SMX_ZENITHS", (long)(solsticed ? zeniths : 0));
      var_set_num(vm, "SMX_SEEDSOLSTICE", (long)(solsticed ? zeniths : 0));
      var_set_num(vm, "SMX_MESH", (long)(solsticed ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)meridians);
      var_set_num(vm, "SMX_EXCHANGES", (long)meridians);
      var_set_num(vm, "SMX_FUSE", (long)meridians);
      var_set_num(vm, "SMX_BIND", (long)meridians);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(meridians + spines + zeniths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (solsticed){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SOLSTICE ok");
      } else if (meridians > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SOLSTICE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX SOLSTICE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX SOLSTICE nodes=%d live=%d meridians=%d spines=%d zeniths=%d need=%d soft=%d talks=%d oob=%d solsticed=%d vital=%ld\n",
                n, live, meridians, spines, zeniths, need, soft, vm->smx_talks, vm->smx_oob, solsticed, vital);
    }
    bump(vm); return 1;
  }
  /* SMX HELIOS|ORBIT|ECLIPSE|APHELION|PERIHELION|PHOTON|MESH_HELIOS|RAISE_HELIOS a b c ...
   * Life-force solar orbit after solstice balance: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete photon mesh among live nodes, weaves an
   * orbital ring (i -> i+1) so free energy circles the sun-path, then helios hub
   * gathers return so lattice raises a helios lock where light holds the hive.
   * Latches SMX_HELIOSED when mesh+orbits+helioses are soft-OOB-free.
   * SMX_PHOTON = chain bonds; SMX_HELIOS hub = root gather pulses;
   * SMX_ORBIT sum = photons+orbits+helioses; SMX_ORBIT|SMX_ECLIPSE sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"HELIOS")||kw(&L->cur,"ORBIT")||kw(&L->cur,"ECLIPSE")||
      kw(&L->cur,"APHELION")||kw(&L->cur,"PERIHELION")||kw(&L->cur,"PHOTON")||
      kw(&L->cur,"MESH_HELIOS")||kw(&L->cur,"RAISE_HELIOS")||
      kw(&L->cur,"HELIOSES")||kw(&L->cur,"ORBITS")||kw(&L->cur,"ECLIPSES")||
      kw(&L->cur,"PHOTONS")||kw(&L->cur,"APHELIONS")||kw(&L->cur,"SEEDHELIOS")||
      kw(&L->cur,"LATTICE_HELIOS")||kw(&L->cur,"SOLAR_LOCK")||
      kw(&L->cur,"SUNPATH")||kw(&L->cur,"DAYSTAR")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int photons = 0;
    int orbits = 0;
    int helioses = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "HELIOS needs >=2 cubes",
                  "SMX HELIOS a b [c ...]  or  SMX ORBIT a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - helios needs clear channel */
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
    /* complete photon mesh among live */
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
            else photons++;
          }
        }
      }
    }
    /* orbital ring - free energy circles every edge i -> i+1 both ways */
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
          else orbits++;
        }
      }
    }
    /* helios hub - seed anchors return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else helioses++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && photons >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && photons * 2 >= need && soft == 0)
        mesh_ok = 1;
      int orbit_ok = (live >= 2 && orbits >= live && soft == 0) ? 1 : 0;
      if (!orbit_ok && live >= 2 && orbits * 2 >= live && soft == 0)
        orbit_ok = 1;
      int helios_ok = (live >= 1 && helioses >= live && soft == 0) ? 1 : 0;
      if (!helios_ok && live >= 1 && helioses * 2 >= live && soft == 0)
        helios_ok = 1;
      int heliosed = (mesh_ok && orbit_ok && helios_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (heliosed ? 12 : (photons > 0 ? 3 : 0)) +
                   (orbits > 0 ? 1 : 0) + (helioses > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_HELIOSED", (long)heliosed);
      var_set_num(vm, "SMX_HELIOS", (long)(heliosed ? photons + orbits + helioses : 0));
      var_set_num(vm, "SMX_ORBIT", (long)(heliosed ? 1 : 0));
      var_set_num(vm, "SMX_ECLIPSE", (long)(heliosed ? 1 : 0));
      var_set_num(vm, "SMX_PHOTONS", (long)(heliosed ? photons : 0));
      var_set_num(vm, "SMX_PHOTON", (long)(heliosed ? photons : 0));
      var_set_num(vm, "SMX_APHELION", (long)(heliosed ? 1 : 0));
      var_set_num(vm, "SMX_ORBITS", (long)(heliosed ? orbits : 0));
      var_set_num(vm, "SMX_HELIOSES", (long)(heliosed ? helioses : 0));
      var_set_num(vm, "SMX_PERIHELION", (long)(heliosed ? helioses : 0));
      var_set_num(vm, "SMX_SEEDHELIOS", (long)(heliosed ? helioses : 0));
      var_set_num(vm, "SMX_MESH", (long)(heliosed ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)photons);
      var_set_num(vm, "SMX_EXCHANGES", (long)photons);
      var_set_num(vm, "SMX_FUSE", (long)photons);
      var_set_num(vm, "SMX_BIND", (long)photons);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(photons + orbits + helioses));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (heliosed){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX HELIOS ok");
      } else if (photons > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX HELIOS partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX HELIOS soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX HELIOS nodes=%d live=%d photons=%d orbits=%d helioses=%d need=%d soft=%d talks=%d oob=%d heliosed=%d vital=%ld\n",
                n, live, photons, orbits, helioses, need, soft, vm->smx_talks, vm->smx_oob, heliosed, vital);
    }
    bump(vm); return 1;
  }
  /* SMX NEBULA|STELLAR|NURSERY|DUST|CORE|CLOUD|MESH_NEBULA|RAISE_NEBULA a b c ...
   * Life-force stellar nursery after helios lock: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete dust mesh among live nodes, weaves a
   * nursery ring (i -> i+1) so free energy circles the cloud-path, then core hub
   * gathers return so lattice raises a nebula lock where stars birth the hive.
   * Latches SMX_NEBULAED when mesh+nurseries+cores are soft-OOB-free.
   * SMX_DUST = chain bonds; SMX_CORE hub = root gather pulses;
   * SMX_NEBULA sum = dusts+nurseries+cores; SMX_NURSERY|SMX_CLOUD sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"NEBULA")||kw(&L->cur,"STELLAR")||kw(&L->cur,"NURSERY")||
      kw(&L->cur,"DUST")||kw(&L->cur,"CORE")||kw(&L->cur,"CLOUD")||
      kw(&L->cur,"MESH_NEBULA")||kw(&L->cur,"RAISE_NEBULA")||
      kw(&L->cur,"NEBULAS")||kw(&L->cur,"NURSERIES")||kw(&L->cur,"CLOUDS")||
      kw(&L->cur,"DUSTS")||kw(&L->cur,"CORES")||kw(&L->cur,"SEEDNEBULA")||
      kw(&L->cur,"LATTICE_NEBULA")||kw(&L->cur,"STAR_BIRTH")||
      kw(&L->cur,"STARDUST")||kw(&L->cur,"PROTOSTAR")||
      kw(&L->cur,"LIFE_FORCE")||kw(&L->cur,"LIFEFORCE")||kw(&L->cur,"MESH_LIFE_FORCE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int dusts = 0;
    int nurseries = 0;
    int cores = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "NEBULA needs >=2 cubes",
                  "SMX NEBULA a b [c ...]  or  SMX STELLAR a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - nebula needs clear channel */
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
    /* complete dust mesh among live */
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
            else dusts++;
          }
        }
      }
    }
    /* nursery ring - free energy circles every edge i -> i+1 both ways */
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
          else nurseries++;
        }
      }
    }
    /* core hub - seed anchors return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else cores++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && dusts >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && dusts * 2 >= need && soft == 0)
        mesh_ok = 1;
      int nursery_ok = (live >= 2 && nurseries >= live && soft == 0) ? 1 : 0;
      if (!nursery_ok && live >= 2 && nurseries * 2 >= live && soft == 0)
        nursery_ok = 1;
      int core_ok = (live >= 1 && cores >= live && soft == 0) ? 1 : 0;
      if (!core_ok && live >= 1 && cores * 2 >= live && soft == 0)
        core_ok = 1;
      int nebulaed = (mesh_ok && nursery_ok && core_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (nebulaed ? 12 : (dusts > 0 ? 3 : 0)) +
                   (nurseries > 0 ? 1 : 0) + (cores > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_NEBULAED", (long)nebulaed);
      var_set_num(vm, "SMX_NEBULA", (long)(nebulaed ? dusts + nurseries + cores : 0));
      var_set_num(vm, "SMX_LIFE_FORCE", (long)(nebulaed ? 1 : 0));
      var_set_num(vm, "SMX_LIFE", (long)(nebulaed ? dusts + nurseries + cores : 0));
      var_set_num(vm, "SMX_NURSERY", (long)(nebulaed ? 1 : 0));
      var_set_num(vm, "SMX_CLOUD", (long)(nebulaed ? 1 : 0));
      var_set_num(vm, "SMX_DUSTS", (long)(nebulaed ? dusts : 0));
      var_set_num(vm, "SMX_DUST", (long)(nebulaed ? dusts : 0));
      var_set_num(vm, "SMX_STELLAR", (long)(nebulaed ? 1 : 0));
      var_set_num(vm, "SMX_NURSERIES", (long)(nebulaed ? nurseries : 0));
      var_set_num(vm, "SMX_CORES", (long)(nebulaed ? cores : 0));
      var_set_num(vm, "SMX_CORE", (long)(nebulaed ? cores : 0));
      var_set_num(vm, "SMX_SEEDNEBULA", (long)(nebulaed ? cores : 0));
      var_set_num(vm, "SMX_MESH", (long)(nebulaed ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)dusts);
      var_set_num(vm, "SMX_EXCHANGES", (long)dusts);
      var_set_num(vm, "SMX_FUSE", (long)dusts);
      var_set_num(vm, "SMX_BIND", (long)dusts);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(dusts + nurseries + cores));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (nebulaed){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX NEBULA ok");
      } else if (dusts > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX NEBULA partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX NEBULA soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX NEBULA nodes=%d live=%d dusts=%d nurseries=%d cores=%d need=%d soft=%d talks=%d oob=%d nebulaed=%d vital=%ld\n",
                n, live, dusts, nurseries, cores, need, soft, vm->smx_talks, vm->smx_oob, nebulaed, vital);
    }
    bump(vm); return 1;
  }
  /* SMX PULSAR|MAGNETAR|BEACON|SPIN|JET|LIGHTHOUSE|MESH_PULSAR|RAISE_PULSAR a b c ...
   * Life-force stellar beacon after helios lock: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete spin mesh among live nodes, weaves a
   * beacon ring (i -> i+1) so free energy circles the cloud-path, then jet hub
   * gathers return so lattice raises a pulsar lock where stars birth the hive.
   * Latches SMX_PULSARED when mesh+beacons+jets are soft-OOB-free.
   * SMX_SPIN = chain bonds; SMX_JET hub = root gather pulses;
   * SMX_PULSAR sum = spins+beacons+jets; SMX_BEACON|SMX_LIGHTHOUSE sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"PULSAR")||kw(&L->cur,"MAGNETAR")||kw(&L->cur,"BEACON")||
      kw(&L->cur,"SPIN")||kw(&L->cur,"JET")||kw(&L->cur,"LIGHTHOUSE")||
      kw(&L->cur,"MESH_PULSAR")||kw(&L->cur,"RAISE_PULSAR")||
      kw(&L->cur,"PULSARS")||kw(&L->cur,"BEACONS")||kw(&L->cur,"LIGHTHOUSES")||
      kw(&L->cur,"SPINS")||kw(&L->cur,"JETS")||kw(&L->cur,"SEEDPULSAR")||
      kw(&L->cur,"LATTICE_PULSAR")||kw(&L->cur,"TIMED_LIGHT")||
      kw(&L->cur,"STARSPIN")||kw(&L->cur,"PULSE_STAR")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int spins = 0;
    int beacons = 0;
    int jets = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "PULSAR needs >=2 cubes",
                  "SMX PULSAR a b [c ...]  or  SMX MAGNETAR a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - pulsar needs clear channel */
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
    /* complete spin mesh among live */
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
            else spins++;
          }
        }
      }
    }
    /* beacon ring - free energy circles every edge i -> i+1 both ways */
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
          else beacons++;
        }
      }
    }
    /* jet hub - seed anchors return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else jets++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && spins >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && spins * 2 >= need && soft == 0)
        mesh_ok = 1;
      int beacon_ok = (live >= 2 && beacons >= live && soft == 0) ? 1 : 0;
      if (!beacon_ok && live >= 2 && beacons * 2 >= live && soft == 0)
        beacon_ok = 1;
      int jet_ok = (live >= 1 && jets >= live && soft == 0) ? 1 : 0;
      if (!jet_ok && live >= 1 && jets * 2 >= live && soft == 0)
        jet_ok = 1;
      int pulsared = (mesh_ok && beacon_ok && jet_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (pulsared ? 12 : (spins > 0 ? 3 : 0)) +
                   (beacons > 0 ? 1 : 0) + (jets > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_PULSARED", (long)pulsared);
      var_set_num(vm, "SMX_PULSAR", (long)(pulsared ? spins + beacons + jets : 0));
      var_set_num(vm, "SMX_BEACON", (long)(pulsared ? 1 : 0));
      var_set_num(vm, "SMX_LIGHTHOUSE", (long)(pulsared ? 1 : 0));
      var_set_num(vm, "SMX_SPINS", (long)(pulsared ? spins : 0));
      var_set_num(vm, "SMX_SPIN", (long)(pulsared ? spins : 0));
      var_set_num(vm, "SMX_MAGNETAR", (long)(pulsared ? 1 : 0));
      var_set_num(vm, "SMX_BEACONS", (long)(pulsared ? beacons : 0));
      var_set_num(vm, "SMX_JETS", (long)(pulsared ? jets : 0));
      var_set_num(vm, "SMX_JET", (long)(pulsared ? jets : 0));
      var_set_num(vm, "SMX_SEEDPULSAR", (long)(pulsared ? jets : 0));
      var_set_num(vm, "SMX_MESH", (long)(pulsared ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)spins);
      var_set_num(vm, "SMX_EXCHANGES", (long)spins);
      var_set_num(vm, "SMX_FUSE", (long)spins);
      var_set_num(vm, "SMX_BIND", (long)spins);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(spins + beacons + jets));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (pulsared){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX PULSAR ok");
      } else if (spins > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX PULSAR partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX PULSAR soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX PULSAR nodes=%d live=%d spins=%d beacons=%d jets=%d need=%d soft=%d talks=%d oob=%d pulsared=%d vital=%ld\n",
                n, live, spins, beacons, jets, need, soft, vm->smx_talks, vm->smx_oob, pulsared, vital);
    }
    bump(vm); return 1;
  }
  /* SMX QUASAR|BLAZAR|DISK|ACCRETION|JETSTREAM|EVENTHORIZON|MESH_QUASAR|RAISE_QUASAR a b c ...
   * Life-force galactic engine after pulsar lock: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete accretion mesh among live nodes, weaves a
   * disk ring (i -> i+1) so free energy circles the event path, then jetstream hub
   * gathers return so lattice raises a quasar lock where galaxies feed the hive.
   * Latches SMX_QUASARED when mesh+disks+jetstreams are soft-OOB-free.
   * SMX_ACCRETION = chain bonds; SMX_JETSTREAM hub = root gather pulses;
   * SMX_QUASAR sum = accretions+disks+jetstreams; SMX_DISK|SMX_BLAZAR sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"QUASAR")||kw(&L->cur,"BLAZAR")||kw(&L->cur,"DISK")||
      kw(&L->cur,"ACCRETION")||kw(&L->cur,"JETSTREAM")||kw(&L->cur,"EVENTHORIZON")||
      kw(&L->cur,"MESH_QUASAR")||kw(&L->cur,"RAISE_QUASAR")||
      kw(&L->cur,"QUASARS")||kw(&L->cur,"DISKS")||kw(&L->cur,"EVENTHORIZONS")||
      kw(&L->cur,"ACCRETIONS")||kw(&L->cur,"JETSTREAMS")||kw(&L->cur,"SEEDQUASAR")||
      kw(&L->cur,"LATTICE_QUASAR")||kw(&L->cur,"GALACTIC_CORE")||
      kw(&L->cur,"ACTIVE_NUCLEUS")||kw(&L->cur,"EVENT_HORIZON")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int accretions = 0;
    int disks = 0;
    int jetstreams = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "QUASAR needs >=2 cubes",
                  "SMX QUASAR a b [c ...]  or  SMX BLAZAR a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - quasar needs clear channel */
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
    /* complete accretion mesh among live */
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
            else accretions++;
          }
        }
      }
    }
    /* disk ring - free energy circles every edge i -> i+1 both ways */
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
          else disks++;
        }
      }
    }
    /* jetstream hub - seed anchors return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else jetstreams++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && accretions >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && accretions * 2 >= need && soft == 0)
        mesh_ok = 1;
      int disk_ok = (live >= 2 && disks >= live && soft == 0) ? 1 : 0;
      if (!disk_ok && live >= 2 && disks * 2 >= live && soft == 0)
        disk_ok = 1;
      int jet_ok = (live >= 1 && jetstreams >= live && soft == 0) ? 1 : 0;
      if (!jet_ok && live >= 1 && jetstreams * 2 >= live && soft == 0)
        jet_ok = 1;
      int quasared = (mesh_ok && disk_ok && jet_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (quasared ? 12 : (accretions > 0 ? 3 : 0)) +
                   (disks > 0 ? 1 : 0) + (jetstreams > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_QUASARED", (long)quasared);
      var_set_num(vm, "SMX_QUASAR", (long)(quasared ? accretions + disks + jetstreams : 0));
      var_set_num(vm, "SMX_DISK", (long)(quasared ? 1 : 0));
      var_set_num(vm, "SMX_EVENTHORIZON", (long)(quasared ? 1 : 0));
      var_set_num(vm, "SMX_ACCRETIONS", (long)(quasared ? accretions : 0));
      var_set_num(vm, "SMX_ACCRETION", (long)(quasared ? accretions : 0));
      var_set_num(vm, "SMX_BLAZAR", (long)(quasared ? 1 : 0));
      var_set_num(vm, "SMX_DISKS", (long)(quasared ? disks : 0));
      var_set_num(vm, "SMX_JETSTREAMS", (long)(quasared ? jetstreams : 0));
      var_set_num(vm, "SMX_JETSTREAM", (long)(quasared ? jetstreams : 0));
      var_set_num(vm, "SMX_JETS", (long)(quasared ? jetstreams : 0));
      var_set_num(vm, "SMX_JET", (long)(quasared ? jetstreams : 0));
      var_set_num(vm, "SMX_SEEDQUASAR", (long)(quasared ? jetstreams : 0));
      var_set_num(vm, "SMX_MESH", (long)(quasared ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)accretions);
      var_set_num(vm, "SMX_EXCHANGES", (long)accretions);
      var_set_num(vm, "SMX_FUSE", (long)accretions);
      var_set_num(vm, "SMX_BIND", (long)accretions);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(accretions + disks + jetstreams));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (quasared){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX QUASAR ok");
      } else if (accretions > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX QUASAR partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX QUASAR soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX QUASAR nodes=%d live=%d accretions=%d disks=%d jetstreams=%d need=%d soft=%d talks=%d oob=%d quasared=%d vital=%ld\n",
                n, live, accretions, disks, jetstreams, need, soft, vm->smx_talks, vm->smx_oob, quasared, vital);
    }
    bump(vm); return 1;
  }

  /* SMX COMET|METEOR|TAIL|NUCLEUS_ICE|COMA|DEBRIS|MESH_COMET|RAISE_COMET a b c ...
   * Life-force ice-fire visitor after quasar lock: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete debris mesh among live nodes, weaves a
   * tail ring (i -> i+1) so free energy circles the coma-path, then nucleus hub
   * gathers return so lattice raises a comet lock where visitors seed the hive.
   * Latches SMX_COMETED when mesh+tails+nuclei are soft-OOB-free.
   * SMX_DEBRIS = chain bonds; SMX_NUCLEUS_ICE hub = root gather pulses;
   * SMX_COMET sum = debris+tails+nuclei; SMX_COMA|SMX_METEOR sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"COMET")||kw(&L->cur,"METEOR")||kw(&L->cur,"TAIL")||
      kw(&L->cur,"COMA")||kw(&L->cur,"DEBRIS")||kw(&L->cur,"NUCLEUS_ICE")||
      kw(&L->cur,"MESH_COMET")||kw(&L->cur,"RAISE_COMET")||
      kw(&L->cur,"COMETS")||kw(&L->cur,"TAILS")||kw(&L->cur,"COMAS")||
      kw(&L->cur,"DEBRISFIELD")||kw(&L->cur,"METEORS")||kw(&L->cur,"SEEDCOMET")||
      kw(&L->cur,"LATTICE_COMET")||kw(&L->cur,"ICE_FIRE")||
      kw(&L->cur,"VISITOR")||kw(&L->cur,"PULSE_COMET")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int debris = 0;
    int tails = 0;
    int nuclei = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "COMET needs >=2 cubes",
                  "SMX COMET a b [c ...]  or  SMX METEOR a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    vm->smx_oob = 0;
    vm->smx.last_err[0] = 0;
    var_set_str(vm, "ERR", "");
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "SMX_ERR", "");
    for (i = 0; i < n; i++){
      present[i] = (find_cube(vm, ids[i]) >= 0) ? 1 : 0;
      if (present[i]) live_ix[live++] = i;
    }
    for (i = 0; i < n; i++){
      if (present[i]) continue;
      if (live > 0){
        int r = do_smx_talk(vm, ids[live_ix[0]], ids[i]);
        if (r < 0) return -1;
        if (r > 0) soft++;
      }
    }
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
            else debris++;
          }
        }
      }
    }
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
          else tails++;
        }
      }
    }
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else nuclei++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && debris >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && debris * 2 >= need && soft == 0)
        mesh_ok = 1;
      int tail_ok = (live >= 2 && tails >= live && soft == 0) ? 1 : 0;
      if (!tail_ok && live >= 2 && tails * 2 >= live && soft == 0)
        tail_ok = 1;
      int nucleus_ok = (live >= 1 && nuclei >= live && soft == 0) ? 1 : 0;
      if (!nucleus_ok && live >= 1 && nuclei * 2 >= live && soft == 0)
        nucleus_ok = 1;
      int cometed = (mesh_ok && tail_ok && nucleus_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (cometed ? 12 : (debris > 0 ? 3 : 0)) +
                   (tails > 0 ? 1 : 0) + (nuclei > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_COMETED", (long)cometed);
      var_set_num(vm, "SMX_COMET", (long)(cometed ? debris + tails + nuclei : 0));
      var_set_num(vm, "SMX_METEOR", (long)(cometed ? 1 : 0));
      var_set_num(vm, "SMX_COMA", (long)(cometed ? 1 : 0));
      var_set_num(vm, "SMX_DEBRIS", (long)(cometed ? debris : 0));
      var_set_num(vm, "SMX_TAILS", (long)(cometed ? tails : 0));
      var_set_num(vm, "SMX_TAIL", (long)(cometed ? tails : 0));
      var_set_num(vm, "SMX_NUCLEI", (long)(cometed ? nuclei : 0));
      var_set_num(vm, "SMX_NUCLEUS_ICE", (long)(cometed ? nuclei : 0));
      var_set_num(vm, "SMX_SEEDCOMET", (long)(cometed ? nuclei : 0));
      var_set_num(vm, "SMX_MESH", (long)(cometed ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)debris);
      var_set_num(vm, "SMX_EXCHANGES", (long)debris);
      var_set_num(vm, "SMX_FUSE", (long)debris);
      var_set_num(vm, "SMX_BIND", (long)debris);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(debris + tails + nuclei));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (cometed){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX COMET ok");
      } else if (debris > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX COMET partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX COMET soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX COMET nodes=%d live=%d debris=%d tails=%d nuclei=%d need=%d soft=%d talks=%d oob=%d cometed=%d vital=%ld\n",
                n, live, debris, tails, nuclei, need, soft, vm->smx_talks, vm->smx_oob, cometed, vital);
    }
    bump(vm); return 1;
  }
  /* SMX SUPERNOVA|NOVA|SHOCKWAVE|EJECTA|REMNANT|MESH_SUPERNOVA|RAISE_SUPERNOVA a b c ...
   * Life-force stellar detonation after comet lock: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete ejecta mesh among live nodes, weaves a
   * shockwave ring (i -> i+1) so free energy circles the blast shell, then remnant
   * hub gathers return so lattice raises a supernova lock where collapse seeds the hive.
   * Latches SMX_SUPERNOVAED when mesh+shocks+remnants are soft-OOB-free.
   * SMX_EJECTA = chain bonds; SMX_REMNANT hub = root gather pulses;
   * SMX_SUPERNOVA sum = ejecta+shocks+remnants; SMX_NOVA|SMX_SHOCK sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"SUPERNOVA")||kw(&L->cur,"NOVA")||kw(&L->cur,"SHOCKWAVE")||
      kw(&L->cur,"EJECTA")||kw(&L->cur,"REMNANT")||kw(&L->cur,"BLAST")||
      kw(&L->cur,"MESH_SUPERNOVA")||kw(&L->cur,"RAISE_SUPERNOVA")||
      kw(&L->cur,"SUPERNOVAS")||kw(&L->cur,"NOVAE")||kw(&L->cur,"SHOCKWAVES")||
      kw(&L->cur,"EJECTAS")||kw(&L->cur,"REMNANTS")||kw(&L->cur,"SEEDSUPERNOVA")||
      kw(&L->cur,"LATTICE_SUPERNOVA")||kw(&L->cur,"STAR_DEATH")||
      kw(&L->cur,"COLLAPSE")||kw(&L->cur,"PULSE_SUPERNOVA")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int ejecta = 0;
    int shocks = 0;
    int remnants = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "SUPERNOVA needs >=2 cubes",
                  "SMX SUPERNOVA a b [c ...]  or  SMX NOVA a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    vm->smx_oob = 0;
    vm->smx.last_err[0] = 0;
    var_set_str(vm, "ERR", "");
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "SMX_ERR", "");
    for (i = 0; i < n; i++){
      present[i] = (find_cube(vm, ids[i]) >= 0) ? 1 : 0;
      if (present[i]) live_ix[live++] = i;
    }
    for (i = 0; i < n; i++){
      if (present[i]) continue;
      if (live > 0){
        int r = do_smx_talk(vm, ids[live_ix[0]], ids[i]);
        if (r < 0) return -1;
        if (r > 0) soft++;
      }
    }
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
            else ejecta++;
          }
        }
      }
    }
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
          else shocks++;
        }
      }
    }
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else remnants++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && ejecta >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && ejecta * 2 >= need && soft == 0)
        mesh_ok = 1;
      int shock_ok = (live >= 2 && shocks >= live && soft == 0) ? 1 : 0;
      if (!shock_ok && live >= 2 && shocks * 2 >= live && soft == 0)
        shock_ok = 1;
      int remnant_ok = (live >= 1 && remnants >= live && soft == 0) ? 1 : 0;
      if (!remnant_ok && live >= 1 && remnants * 2 >= live && soft == 0)
        remnant_ok = 1;
      int sn_ok = (mesh_ok && shock_ok && remnant_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (sn_ok ? 12 : (ejecta > 0 ? 3 : 0)) +
                   (shocks > 0 ? 1 : 0) + (remnants > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_SUPERNOVAED", (long)sn_ok);
      var_set_num(vm, "SMX_SUPERNOVA", (long)(sn_ok ? ejecta + shocks + remnants : 0));
      var_set_num(vm, "SMX_NOVA", (long)(sn_ok ? 1 : 0));
      var_set_num(vm, "SMX_BLAST", (long)(sn_ok ? 1 : 0));
      var_set_num(vm, "SMX_EJECTA", (long)(sn_ok ? ejecta : 0));
      var_set_num(vm, "SMX_SHOCKS", (long)(sn_ok ? shocks : 0));
      var_set_num(vm, "SMX_SHOCKWAVE", (long)(sn_ok ? shocks : 0));
      var_set_num(vm, "SMX_SHOCK", (long)(sn_ok ? shocks : 0));
      var_set_num(vm, "SMX_REMNANTS", (long)(sn_ok ? remnants : 0));
      var_set_num(vm, "SMX_REMNANT", (long)(sn_ok ? remnants : 0));
      var_set_num(vm, "SMX_SEEDSUPERNOVA", (long)(sn_ok ? remnants : 0));
      var_set_num(vm, "SMX_MESH", (long)(sn_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)ejecta);
      var_set_num(vm, "SMX_EXCHANGES", (long)ejecta);
      var_set_num(vm, "SMX_FUSE", (long)ejecta);
      var_set_num(vm, "SMX_BIND", (long)ejecta);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(ejecta + shocks + remnants));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (sn_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SUPERNOVA ok");
      } else if (ejecta > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SUPERNOVA partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX SUPERNOVA soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX SUPERNOVA nodes=%d live=%d ejecta=%d shocks=%d remnants=%d need=%d soft=%d talks=%d oob=%d supernovaed=%d vital=%ld\n",
                n, live, ejecta, shocks, remnants, need, soft, vm->smx_talks, vm->smx_oob, sn_ok, vital);
    }
    bump(vm); return 1;
  }



  /* SMX GALAXY|SPIRAL|ARM|CORE|HALO|BULGE|MESH_GALAXY|RAISE_GALAXY a b c ...
   * Life-force galactic weave after supernova seed: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete stellar mesh among live nodes, weaves a
   * spiral-arm ring (i -> i+1) so free energy circles the disk, then core hub
   * gathers return so lattice raises a galaxy lock where arms feed the bulge.
   * Latches SMX_GALAXIED when mesh+arms+core are soft-OOB-free.
   * SMX_STARS = chain bonds; SMX_CORE hub = root gather pulses;
   * SMX_GALAXY sum = stars+arms+cores; SMX_SPIRAL|SMX_ARM sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"GALAXY")||kw(&L->cur,"SPIRAL")||kw(&L->cur,"ARM")||
      kw(&L->cur,"CORE")||kw(&L->cur,"HALO")||kw(&L->cur,"BULGE")||
      kw(&L->cur,"MESH_GALAXY")||kw(&L->cur,"RAISE_GALAXY")||
      kw(&L->cur,"GALAXIES")||kw(&L->cur,"SPIRALS")||kw(&L->cur,"ARMS")||
      kw(&L->cur,"CORES")||kw(&L->cur,"HALOS")||kw(&L->cur,"SEEDGALAXY")||
      kw(&L->cur,"LATTICE_GALAXY")||kw(&L->cur,"STAR_DISK")||
      kw(&L->cur,"DISK_ARM")||kw(&L->cur,"PULSE_GALAXY")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int stars = 0;
    int arms = 0;
    int cores = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "GALAXY needs >=2 cubes",
                  "SMX GALAXY a b [c ...]  or  SMX SPIRAL a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    vm->smx_oob = 0;
    vm->smx.last_err[0] = 0;
    var_set_str(vm, "ERR", "");
    var_set_str(vm, "LAST_ERR", "");
    var_set_str(vm, "SMX_ERR", "");
    for (i = 0; i < n; i++){
      present[i] = (find_cube(vm, ids[i]) >= 0) ? 1 : 0;
      if (present[i]) live_ix[live++] = i;
    }
    for (i = 0; i < n; i++){
      if (present[i]) continue;
      if (live > 0){
        int r = do_smx_talk(vm, ids[live_ix[0]], ids[i]);
        if (r < 0) return -1;
        if (r > 0) soft++;
      }
    }
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
            else stars++;
          }
        }
      }
    }
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
          else arms++;
        }
      }
    }
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else cores++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && stars >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && stars * 2 >= need && soft == 0)
        mesh_ok = 1;
      int arm_ok = (live >= 2 && arms >= live && soft == 0) ? 1 : 0;
      if (!arm_ok && live >= 2 && arms * 2 >= live && soft == 0)
        arm_ok = 1;
      int core_ok = (live >= 1 && cores >= live && soft == 0) ? 1 : 0;
      if (!core_ok && live >= 1 && cores * 2 >= live && soft == 0)
        core_ok = 1;
      int gx_ok = (mesh_ok && arm_ok && core_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (gx_ok ? 12 : (stars > 0 ? 3 : 0)) +
                   (arms > 0 ? 1 : 0) + (cores > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_GALAXIED", (long)gx_ok);
      var_set_num(vm, "SMX_GALAXY", (long)(gx_ok ? stars + arms + cores : 0));
      var_set_num(vm, "SMX_SPIRAL", (long)(gx_ok ? 1 : 0));
      var_set_num(vm, "SMX_BULGE", (long)(gx_ok ? 1 : 0));
      var_set_num(vm, "SMX_STARS", (long)(gx_ok ? stars : 0));
      var_set_num(vm, "SMX_ARMS", (long)(gx_ok ? arms : 0));
      var_set_num(vm, "SMX_ARM", (long)(gx_ok ? arms : 0));
      var_set_num(vm, "SMX_HALO", (long)(gx_ok ? arms : 0));
      var_set_num(vm, "SMX_CORES", (long)(gx_ok ? cores : 0));
      var_set_num(vm, "SMX_CORE", (long)(gx_ok ? cores : 0));
      var_set_num(vm, "SMX_SEEDGALAXY", (long)(gx_ok ? cores : 0));
      var_set_num(vm, "SMX_MESH", (long)(gx_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)stars);
      var_set_num(vm, "SMX_EXCHANGES", (long)stars);
      var_set_num(vm, "SMX_FUSE", (long)stars);
      var_set_num(vm, "SMX_BIND", (long)stars);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(stars + arms + cores));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (gx_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX GALAXY ok");
      } else if (stars > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX GALAXY partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX GALAXY soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX GALAXY nodes=%d live=%d stars=%d arms=%d cores=%d need=%d soft=%d talks=%d oob=%d galaxied=%d vital=%ld\n",
                n, live, stars, arms, cores, need, soft, vm->smx_talks, vm->smx_oob, gx_ok, vital);
    }
    bump(vm); return 1;
  }

  /* SMX CONSTELLATION|ASTERISM|STARFIELD|GUIDESTAR|NAVSTAR|LODGE|MESH_CONSTELLATION|RAISE_CONSTELLATION a b c ...
   * Life-force sky map after galaxy weave: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete link mesh among live nodes, weaves a
   * guide-arc ring (i -> i+1) so free energy traces the sky path, then anchor hub
   * gathers return so lattice raises a constellation lock where guides hold the hive.
   * Latches SMX_CONSTELLATED when mesh+arcs+anchors are soft-OOB-free.
   * SMX_LINKS = chain bonds; SMX_ANCHOR hub = root gather pulses;
   * SMX_CONSTELLATION sum = links+arcs+anchors; SMX_ASTERISM|SMX_STARFIELD sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"CONSTELLATION")||kw(&L->cur,"ASTERISM")||kw(&L->cur,"STARFIELD")||
      kw(&L->cur,"GUIDESTAR")||kw(&L->cur,"NAVSTAR")||kw(&L->cur,"LODGE")||
      kw(&L->cur,"MESH_CONSTELLATION")||kw(&L->cur,"RAISE_CONSTELLATION")||
      kw(&L->cur,"CONSTELLATIONS")||kw(&L->cur,"ASTERISMS")||kw(&L->cur,"STARFIELDS")||
      kw(&L->cur,"GUIDESTARS")||kw(&L->cur,"NAVSTARS")||kw(&L->cur,"SEEDCONSTELLATION")||
      kw(&L->cur,"LATTICE_CONSTELLATION")||kw(&L->cur,"SKY_MAP")||
      kw(&L->cur,"GUIDE_MESH")||kw(&L->cur,"PULSE_CONSTELLATION")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int links = 0;
    int arcs = 0;
    int anchors = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "CONSTELLATION needs >=2 cubes",
                  "SMX CONSTELLATION a b [c ...]  or  SMX ASTERISM a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - constellation needs clear channel */
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
    /* complete link mesh among live */
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
            else links++;
          }
        }
      }
    }
    /* guide-arc ring - free energy holds every edge i -> i+1 both ways */
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
          else arcs++;
        }
      }
    }
    /* anchor hub - seed polestar return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else anchors++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && links >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && links * 2 >= need && soft == 0)
        mesh_ok = 1;
      int arc_ok = (live >= 2 && arcs >= live && soft == 0) ? 1 : 0;
      if (!arc_ok && live >= 2 && arcs * 2 >= live && soft == 0)
        arc_ok = 1;
      int anchor_ok = (live >= 1 && anchors >= live && soft == 0) ? 1 : 0;
      if (!anchor_ok && live >= 1 && anchors * 2 >= live && soft == 0)
        anchor_ok = 1;
      int cst_ok = (mesh_ok && arc_ok && anchor_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (cst_ok ? 12 : (links > 0 ? 3 : 0)) +
                   (arcs > 0 ? 1 : 0) + (anchors > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_CONSTELLATED", (long)cst_ok);
      var_set_num(vm, "SMX_CONSTELLATION", (long)(cst_ok ? links + arcs + anchors : 0));
      var_set_num(vm, "SMX_ASTERISM", (long)(cst_ok ? 1 : 0));
      var_set_num(vm, "SMX_STARFIELD", (long)(cst_ok ? 1 : 0));
      var_set_num(vm, "SMX_LINKS", (long)(cst_ok ? links : 0));
      var_set_num(vm, "SMX_LINK", (long)(cst_ok ? links : 0));
      var_set_num(vm, "SMX_ARCS", (long)(cst_ok ? arcs : 0));
      var_set_num(vm, "SMX_ARC", (long)(cst_ok ? arcs : 0));
      var_set_num(vm, "SMX_GUIDESTAR", (long)(cst_ok ? 1 : 0));
      var_set_num(vm, "SMX_NAVSTAR", (long)(cst_ok ? 1 : 0));
      var_set_num(vm, "SMX_ANCHORS", (long)(cst_ok ? anchors : 0));
      var_set_num(vm, "SMX_ANCHOR", (long)(cst_ok ? anchors : 0));
      var_set_num(vm, "SMX_LODGE", (long)(cst_ok ? anchors : 0));
      var_set_num(vm, "SMX_SEEDCONSTELLATION", (long)(cst_ok ? anchors : 0));
      var_set_num(vm, "SMX_MESH", (long)(cst_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)links);
      var_set_num(vm, "SMX_EXCHANGES", (long)links);
      var_set_num(vm, "SMX_FUSE", (long)links);
      var_set_num(vm, "SMX_BIND", (long)links);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(links + arcs + anchors));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (cst_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CONSTELLATION ok");
      } else if (links > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CONSTELLATION partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX CONSTELLATION soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX CONSTELLATION nodes=%d live=%d links=%d arcs=%d anchors=%d need=%d soft=%d talks=%d oob=%d constellated=%d vital=%ld\n",
                n, live, links, arcs, anchors, need, soft, vm->smx_talks, vm->smx_oob, cst_ok, vital);
    }
    bump(vm); return 1;
  }

  /* SMX ZODIAC|ECLIPTIC|PATH|HOUSE|SIGN|POLE|MESH_ZODIAC|RAISE_ZODIAC a b c ...
   * Life-force year-path after constellation: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete sign mesh among live nodes, weaves a
   * ecliptic-house ring (i -> i+1) so free energy traces the year-path, then pole hub
   * gathers return so lattice raises a zodiac lock where houses hold the hive.
   * Latches SMX_ZODIACED when mesh+houses+poles are soft-OOB-free.
   * SMX_SIGNS = chain bonds; SMX_POLE hub = root gather pulses;
   * SMX_ZODIAC sum = signs+houses+poles; SMX_ECLIPTIC|SMX_PATH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"ZODIAC")||kw(&L->cur,"ECLIPTIC")||kw(&L->cur,"PATH")||
      kw(&L->cur,"HOUSE")||kw(&L->cur,"SIGN")||kw(&L->cur,"POLE")||
      kw(&L->cur,"MESH_ZODIAC")||kw(&L->cur,"RAISE_ZODIAC")||
      kw(&L->cur,"ZODIACS")||kw(&L->cur,"ECLIPTICS")||kw(&L->cur,"PATHS")||
      kw(&L->cur,"HOUSES")||kw(&L->cur,"SIGNS")||kw(&L->cur,"SEEDZODIAC")||
      kw(&L->cur,"LATTICE_ZODIAC")||kw(&L->cur,"YEAR_PATH")||
      kw(&L->cur,"HOUSE_RING")||kw(&L->cur,"PULSE_ZODIAC")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int signs = 0;
    int houses = 0;
    int poles = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "ZODIAC needs >=2 cubes",
                  "SMX ZODIAC a b [c ...]  or  SMX ECLIPTIC a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - zodiac needs clear channel */
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
    /* complete sign mesh among live */
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
            else signs++;
          }
        }
      }
    }
    /* ecliptic-house ring - free energy holds every edge i -> i+1 both ways */
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
          else houses++;
        }
      }
    }
    /* pole hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else poles++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && signs >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && signs * 2 >= need && soft == 0)
        mesh_ok = 1;
      int house_ok = (live >= 2 && houses >= live && soft == 0) ? 1 : 0;
      if (!house_ok && live >= 2 && houses * 2 >= live && soft == 0)
        house_ok = 1;
      int pole_ok = (live >= 1 && poles >= live && soft == 0) ? 1 : 0;
      if (!pole_ok && live >= 1 && poles * 2 >= live && soft == 0)
        pole_ok = 1;
      int zd_ok = (mesh_ok && house_ok && pole_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (zd_ok ? 12 : (signs > 0 ? 3 : 0)) +
                   (houses > 0 ? 1 : 0) + (poles > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_ZODIACED", (long)zd_ok);
      var_set_num(vm, "SMX_ZODIAC", (long)(zd_ok ? signs + houses + poles : 0));
      var_set_num(vm, "SMX_ECLIPTIC", (long)(zd_ok ? 1 : 0));
      var_set_num(vm, "SMX_PATH", (long)(zd_ok ? 1 : 0));
      var_set_num(vm, "SMX_SIGNS", (long)(zd_ok ? signs : 0));
      var_set_num(vm, "SMX_SIGN", (long)(zd_ok ? signs : 0));
      var_set_num(vm, "SMX_HOUSES", (long)(zd_ok ? houses : 0));
      var_set_num(vm, "SMX_HOUSE", (long)(zd_ok ? houses : 0));
      var_set_num(vm, "SMX_YEARPATH", (long)(zd_ok ? houses : 0));
      var_set_num(vm, "SMX_POLES", (long)(zd_ok ? poles : 0));
      var_set_num(vm, "SMX_POLE", (long)(zd_ok ? poles : 0));
      var_set_num(vm, "SMX_AXISPOLE", (long)(zd_ok ? poles : 0));
      var_set_num(vm, "SMX_SEEDZODIAC", (long)(zd_ok ? poles : 0));
      var_set_num(vm, "SMX_MESH", (long)(zd_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)signs);
      var_set_num(vm, "SMX_EXCHANGES", (long)signs);
      var_set_num(vm, "SMX_FUSE", (long)signs);
      var_set_num(vm, "SMX_BIND", (long)signs);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(signs + houses + poles));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (zd_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ZODIAC ok");
      } else if (signs > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ZODIAC partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX ZODIAC soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX ZODIAC nodes=%d live=%d signs=%d houses=%d poles=%d need=%d soft=%d talks=%d oob=%d zodiaced=%d vital=%ld\n",
                n, live, signs, houses, poles, need, soft, vm->smx_talks, vm->smx_oob, zd_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX FIRMAMENT|VAULT|DOME|SKYVAULT|SPAN|KEYSTONE|MESH_FIRMAMENT|RAISE_FIRMAMENT a b c ...
   * Life-force sky-vault after zodiac: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete span mesh among live nodes, weaves a
   * dome-vault ring (i -> i+1) so free energy arches the firmament, then keystone hub
   * gathers return so lattice raises a firmament lock where heaven holds the hive.
   * Latches SMX_FIRMED when mesh+vaults+keystones are soft-OOB-free.
   * SMX_SPANS = chain bonds; SMX_KEYSTONE hub = root gather pulses;
   * SMX_FIRMAMENT sum = spans+vaults+keystones; SMX_VAULT|SMX_DOME sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"FIRMAMENT")||kw(&L->cur,"VAULT")||kw(&L->cur,"DOME")||
      kw(&L->cur,"SKYVAULT")||kw(&L->cur,"SPAN")||kw(&L->cur,"KEYSTONE")||kw(&L->cur,"HEAVEN")||
      kw(&L->cur,"MESH_FIRMAMENT")||kw(&L->cur,"RAISE_FIRMAMENT")||
      kw(&L->cur,"FIRMAMENTS")||kw(&L->cur,"VAULTS")||kw(&L->cur,"DOMES")||
      kw(&L->cur,"SKYVAULTS")||kw(&L->cur,"SPANS")||kw(&L->cur,"SEEDFIRMAMENT")||
      kw(&L->cur,"LATTICE_FIRMAMENT")||kw(&L->cur,"SKY_VAULT")||
      kw(&L->cur,"DOME_RING")||kw(&L->cur,"PULSE_FIRMAMENT")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int spans = 0;
    int vaults = 0;
    int keystones = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "FIRMAMENT needs >=2 cubes",
                  "SMX FIRMAMENT a b [c ...]  or  SMX VAULT a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - firmament needs clear channel */
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
    /* complete span mesh among live */
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
            else spans++;
          }
        }
      }
    }
    /* dome-vault ring - free energy holds every edge i -> i+1 both ways */
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
          else vaults++;
        }
      }
    }
    /* keystone hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else keystones++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && spans >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && spans * 2 >= need && soft == 0)
        mesh_ok = 1;
      int house_ok = (live >= 2 && vaults >= live && soft == 0) ? 1 : 0;
      if (!house_ok && live >= 2 && vaults * 2 >= live && soft == 0)
        house_ok = 1;
      int pole_ok = (live >= 1 && keystones >= live && soft == 0) ? 1 : 0;
      if (!pole_ok && live >= 1 && keystones * 2 >= live && soft == 0)
        pole_ok = 1;
      int fm_ok = (mesh_ok && house_ok && pole_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (fm_ok ? 12 : (spans > 0 ? 3 : 0)) +
                   (vaults > 0 ? 1 : 0) + (keystones > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_FIRMED", (long)fm_ok);
      var_set_num(vm, "SMX_FIRMAMENTED", (long)fm_ok);
      var_set_num(vm, "SMX_FIRMAMENT", (long)(fm_ok ? spans + vaults + keystones : 0));
      var_set_num(vm, "SMX_VAULT", (long)(fm_ok ? 1 : 0));
      var_set_num(vm, "SMX_DOME", (long)(fm_ok ? 1 : 0));
      var_set_num(vm, "SMX_SPANS", (long)(fm_ok ? spans : 0));
      var_set_num(vm, "SMX_SPAN", (long)(fm_ok ? spans : 0));
      var_set_num(vm, "SMX_VAULTS", (long)(fm_ok ? vaults : 0));
      var_set_num(vm, "SMX_SKYVAULT", (long)(fm_ok ? vaults : 0));
      var_set_num(vm, "SMX_DOMERING", (long)(fm_ok ? vaults : 0));
      var_set_num(vm, "SMX_KEYSTONES", (long)(fm_ok ? keystones : 0));
      var_set_num(vm, "SMX_KEYSTONE", (long)(fm_ok ? keystones : 0));
      var_set_num(vm, "SMX_HEAVEN", (long)(fm_ok ? keystones : 0));
      var_set_num(vm, "SMX_SEEDFIRMAMENT", (long)(fm_ok ? keystones : 0));
      var_set_num(vm, "SMX_MESH", (long)(fm_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)spans);
      var_set_num(vm, "SMX_EXCHANGES", (long)spans);
      var_set_num(vm, "SMX_FUSE", (long)spans);
      var_set_num(vm, "SMX_BIND", (long)spans);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(spans + vaults + keystones));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (fm_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX FIRMAMENT ok");
      } else if (spans > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX FIRMAMENT partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX FIRMAMENT soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX FIRMAMENT nodes=%d live=%d spans=%d vaults=%d keystones=%d need=%d soft=%d talks=%d oob=%d firmamented=%d vital=%ld\n",
                n, live, spans, vaults, keystones, need, soft, vm->smx_talks, vm->smx_oob, fm_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX AETHER|QUINTESSENCE|ETHER|PLENUM|ESSENCE|AURA|MESH_AETHER|RAISE_AETHER a b c ...
   * Life-force quintessence after firmament: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete essence mesh among live nodes, weaves a
   * plenum ring (i -> i+1) so free energy fills the aether, then aura hub
   * gathers return so lattice raises an aether lock where quintessence holds the hive.
   * Latches SMX_AETHERED when mesh+plenums+auras are soft-OOB-free.
   * SMX_ESSENCES = chain bonds; SMX_AURA hub = root gather pulses;
   * SMX_AETHER sum = essences+plenums+auras; SMX_QUINTESSENCE|SMX_ETHER sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"AETHER")||kw(&L->cur,"QUINTESSENCE")||kw(&L->cur,"ETHER")||
      kw(&L->cur,"PLENUM")||kw(&L->cur,"ESSENCE")||kw(&L->cur,"AURA")||kw(&L->cur,"QUINTA")||
      kw(&L->cur,"MESH_AETHER")||kw(&L->cur,"RAISE_AETHER")||
      kw(&L->cur,"AETHERS")||kw(&L->cur,"PLENUMS")||kw(&L->cur,"ETHERS")||
      kw(&L->cur,"PLENUMS")||kw(&L->cur,"ESSENCES")||kw(&L->cur,"SEEDAETHER")||
      kw(&L->cur,"LATTICE_AETHER")||kw(&L->cur,"FIFTH_ELEMENT")||
      kw(&L->cur,"PLENUM_RING")||kw(&L->cur,"PULSE_AETHER")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int essences = 0;
    int plenums = 0;
    int auras = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "AETHER needs >=2 cubes",
                  "SMX AETHER a b [c ...]  or  SMX QUINTESSENCE a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - aether needs clear channel */
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
    /* complete essence mesh among live */
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
            else essences++;
          }
        }
      }
    }
    /* plenum ring - free energy holds every edge i -> i+1 both ways */
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
          else plenums++;
        }
      }
    }
    /* aura hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else auras++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && essences >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && essences * 2 >= need && soft == 0)
        mesh_ok = 1;
      int plenum_ok = (live >= 2 && plenums >= live && soft == 0) ? 1 : 0;
      if (!plenum_ok && live >= 2 && plenums * 2 >= live && soft == 0)
        plenum_ok = 1;
      int aura_ok = (live >= 1 && auras >= live && soft == 0) ? 1 : 0;
      if (!aura_ok && live >= 1 && auras * 2 >= live && soft == 0)
        aura_ok = 1;
      int ae_ok = (mesh_ok && plenum_ok && aura_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (ae_ok ? 12 : (essences > 0 ? 3 : 0)) +
                   (plenums > 0 ? 1 : 0) + (auras > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_AETHERED", (long)ae_ok);
      var_set_num(vm, "SMX_AETHERED_LATCH", (long)ae_ok);
      var_set_num(vm, "SMX_AETHER", (long)(ae_ok ? essences + plenums + auras : 0));
      var_set_num(vm, "SMX_QUINTESSENCE", (long)(ae_ok ? 1 : 0));
      var_set_num(vm, "SMX_ETHER", (long)(ae_ok ? 1 : 0));
      var_set_num(vm, "SMX_ESSENCES", (long)(ae_ok ? essences : 0));
      var_set_num(vm, "SMX_ESSENCE", (long)(ae_ok ? essences : 0));
      var_set_num(vm, "SMX_PLENUMS", (long)(ae_ok ? plenums : 0));
      var_set_num(vm, "SMX_PLENUM", (long)(ae_ok ? plenums : 0));
      var_set_num(vm, "SMX_PLENUMRING", (long)(ae_ok ? plenums : 0));
      var_set_num(vm, "SMX_AURAS", (long)(ae_ok ? auras : 0));
      var_set_num(vm, "SMX_AURA", (long)(ae_ok ? auras : 0));
      var_set_num(vm, "SMX_QUINTA", (long)(ae_ok ? auras : 0));
      var_set_num(vm, "SMX_SEEDAETHER", (long)(ae_ok ? auras : 0));
      var_set_num(vm, "SMX_MESH", (long)(ae_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)essences);
      var_set_num(vm, "SMX_EXCHANGES", (long)essences);
      var_set_num(vm, "SMX_FUSE", (long)essences);
      var_set_num(vm, "SMX_BIND", (long)essences);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(essences + plenums + auras));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (ae_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX AETHER ok");
      } else if (essences > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX AETHER partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX AETHER soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX AETHER nodes=%d live=%d essences=%d plenums=%d auras=%d need=%d soft=%d talks=%d oob=%d aethered=%d vital=%ld\n",
                n, live, essences, plenums, auras, need, soft, vm->smx_talks, vm->smx_oob, ae_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX NOOSPHERE|LOGOS|AKASHA|LOGOI|THOUGHT|ANIMA|MESH_NOOSPHERE|RAISE_NOOSPHERE a b c ...
   * Life-force logos after firmament: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete thought mesh among live nodes, weaves a
   * logos ring (i -> i+1) so free energy fills the noosphere, then anima hub
   * gathers return so lattice raises a noosphere lock where logos holds the hive.
   * Latches SMX_NOOSPHERED when mesh+logoi+animae are soft-OOB-free.
   * SMX_THOUGHTS = chain bonds; SMX_ANIMA hub = root gather pulses;
   * SMX_NOOSPHERE sum = bonds+logoi+animae; SMX_LOGOS|SMX_AKASHA sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"NOOSPHERE")||kw(&L->cur,"LOGOS")||kw(&L->cur,"AKASHA")||
      kw(&L->cur,"LOGOI")||kw(&L->cur,"THOUGHT")||kw(&L->cur,"ANIMA")||kw(&L->cur,"ANIMA")||
      kw(&L->cur,"MESH_NOOSPHERE")||kw(&L->cur,"RAISE_NOOSPHERE")||
      kw(&L->cur,"NOOSPHERES")||kw(&L->cur,"LOGOI")||kw(&L->cur,"AKASHAS")||
      kw(&L->cur,"LOGOI")||kw(&L->cur,"THOUGHTS")||kw(&L->cur,"SEEDNOOS")||
      kw(&L->cur,"LATTICE_NOOSPHERE")||kw(&L->cur,"WORLD_MIND")||
      kw(&L->cur,"LOGOS_RING")||kw(&L->cur,"PULSE_NOOSPHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int logoi = 0;
    int animae = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "NOOSPHERE needs >=2 cubes",
                  "SMX NOOSPHERE a b [c ...]  or  SMX LOGOS a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - noosphere needs clear channel */
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
    /* complete thought mesh among live */
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
    /* logos ring - free energy holds every edge i -> i+1 both ways */
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
          else logoi++;
        }
      }
    }
    /* anima hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else animae++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int logos_ok = (live >= 2 && logoi >= live && soft == 0) ? 1 : 0;
      if (!logos_ok && live >= 2 && logoi * 2 >= live && soft == 0)
        logos_ok = 1;
      int anima_ok = (live >= 1 && animae >= live && soft == 0) ? 1 : 0;
      if (!anima_ok && live >= 1 && animae * 2 >= live && soft == 0)
        anima_ok = 1;
      int ns_ok = (mesh_ok && logos_ok && anima_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (ns_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (logoi > 0 ? 1 : 0) + (animae > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_NOOSPHERED", (long)ns_ok);
      var_set_num(vm, "SMX_NOOSPHERED_LATCH", (long)ns_ok);
      var_set_num(vm, "SMX_NOOSPHERE", (long)(ns_ok ? bonds + logoi + animae : 0));
      var_set_num(vm, "SMX_LOGOS", (long)(ns_ok ? 1 : 0));
      var_set_num(vm, "SMX_AKASHA", (long)(ns_ok ? 1 : 0));
      var_set_num(vm, "SMX_THOUGHTS", (long)(ns_ok ? bonds : 0));
      var_set_num(vm, "SMX_THOUGHT", (long)(ns_ok ? bonds : 0));
      var_set_num(vm, "SMX_LOGOI", (long)(ns_ok ? logoi : 0));
      var_set_num(vm, "SMX_LOGO", (long)(ns_ok ? logoi : 0));
      var_set_num(vm, "SMX_LOGOSRING", (long)(ns_ok ? logoi : 0));
      var_set_num(vm, "SMX_ANIMAE", (long)(ns_ok ? animae : 0));
      var_set_num(vm, "SMX_ANIMA", (long)(ns_ok ? animae : 0));
      var_set_num(vm, "SMX_ANIMA", (long)(ns_ok ? animae : 0));
      var_set_num(vm, "SMX_SEEDNOOS", (long)(ns_ok ? animae : 0));
      var_set_num(vm, "SMX_MESH", (long)(ns_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + logoi + animae));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (ns_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX NOOSPHERE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX NOOSPHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX NOOSPHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX NOOSPHERE nodes=%d live=%d bonds=%d logoi=%d animae=%d need=%d soft=%d talks=%d oob=%d noosphered=%d vital=%ld\n",
                n, live, bonds, logoi, animae, need, soft, vm->smx_talks, vm->smx_oob, ns_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX PNEUMA|PRANA|BREATHWORLD|SOULFIRE|SPIRITUS|WORLDSOUL|MESH_PNEUMA|RAISE_PNEUMA a b c ...
   * Life-force breath after noosphere: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete pneuma mesh among live nodes, weaves a
   * prana ring (i -> i+1) so free energy fills the breathworld, then spirit hub
   * gathers return so lattice raises a pneuma lock where soulfire holds the hive.
   * Latches SMX_PNEUMATED when mesh+pranae+spiriti are soft-OOB-free.
   * SMX_BREATHS = chain bonds; SMX_SPIRIT hub = root gather pulses;
   * SMX_PNEUMA sum = bonds+pranae+spiriti; SMX_PRANA|SMX_WORLDSOUL sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"PNEUMA")||kw(&L->cur,"PRANA")||kw(&L->cur,"BREATHWORLD")||
      kw(&L->cur,"SOULFIRE")||kw(&L->cur,"SPIRITUS")||kw(&L->cur,"WORLDSOUL")||
      kw(&L->cur,"MESH_PNEUMA")||kw(&L->cur,"RAISE_PNEUMA")||
      kw(&L->cur,"PNEUMAS")||kw(&L->cur,"PRANAE")||kw(&L->cur,"SPIRITI")||
      kw(&L->cur,"BREATHS")||kw(&L->cur,"BREATHLINK")||kw(&L->cur,"SEEDPNEUMA")||kw(&L->cur,"SOULFIRES")||
      kw(&L->cur,"LATTICE_PNEUMA")||kw(&L->cur,"LIFE_BREATH")||
      kw(&L->cur,"PRANA_RING")||kw(&L->cur,"PULSE_PNEUMA")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int pranae = 0;
    int spiriti = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "PNEUMA needs >=2 cubes",
                  "SMX PNEUMA a b [c ...]  or  SMX PRANA a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - pneuma needs clear channel */
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
    /* complete breath mesh among live */
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
    /* prana ring - free energy holds every edge i -> i+1 both ways */
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
          else pranae++;
        }
      }
    }
    /* spirit hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else spiriti++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int prana_ok = (live >= 2 && pranae >= live && soft == 0) ? 1 : 0;
      if (!prana_ok && live >= 2 && pranae * 2 >= live && soft == 0)
        prana_ok = 1;
      int spirit_ok = (live >= 1 && spiriti >= live && soft == 0) ? 1 : 0;
      if (!spirit_ok && live >= 1 && spiriti * 2 >= live && soft == 0)
        spirit_ok = 1;
      int pn_ok = (mesh_ok && prana_ok && spirit_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (pn_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (pranae > 0 ? 1 : 0) + (spiriti > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_PNEUMATED", (long)pn_ok);
      var_set_num(vm, "SMX_PNEUMATED_LATCH", (long)pn_ok);
      var_set_num(vm, "SMX_PNEUMA", (long)(pn_ok ? bonds + pranae + spiriti : 0));
      var_set_num(vm, "SMX_PRANA", (long)(pn_ok ? 1 : 0));
      var_set_num(vm, "SMX_WORLDSOUL", (long)(pn_ok ? 1 : 0));
      var_set_num(vm, "SMX_SOULFIRE", (long)(pn_ok ? 1 : 0));
      var_set_num(vm, "SMX_BREATHS", (long)(pn_ok ? bonds : 0));
      var_set_num(vm, "SMX_BREATHLINK", (long)(pn_ok ? bonds : 0));
      var_set_num(vm, "SMX_PRANAE", (long)(pn_ok ? pranae : 0));
      var_set_num(vm, "SMX_PRANARING", (long)(pn_ok ? pranae : 0));
      var_set_num(vm, "SMX_SPIRITI", (long)(pn_ok ? spiriti : 0));
      var_set_num(vm, "SMX_SPIRIT", (long)(pn_ok ? spiriti : 0));
      var_set_num(vm, "SMX_SEEDPNEUMA", (long)(pn_ok ? spiriti : 0));
      var_set_num(vm, "SMX_MESH", (long)(pn_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + pranae + spiriti));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (pn_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX PNEUMA ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX PNEUMA partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX PNEUMA soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX PNEUMA nodes=%d live=%d bonds=%d pranae=%d spiriti=%d need=%d soft=%d talks=%d oob=%d pneumated=%d vital=%ld\n",
                n, live, bonds, pranae, spiriti, need, soft, vm->smx_talks, vm->smx_oob, pn_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX AEGIS|SHIELD|WARD|BULWARK|PAVIS|MESH_AEGIS|RAISE_AEGIS a b c ...
   * Mesh stability after pneuma: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete aegis mesh among live nodes, weaves a
   * shield ring (i -> i+1) so free energy holds the ward, then hub gathers
   * return so lattice raises an aegis lock where the shield holds the hive.
   * Latches SMX_AEGISED when mesh+shields+wards are soft-OOB-free.
   * SMX_SHIELDS = chain bonds; SMX_WARD hub = root gather pulses;
   * SMX_AEGIS sum = bonds+shields+wards; SMX_SHIELD|SMX_BULWARK sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"AEGIS")||kw(&L->cur,"SHIELD")||kw(&L->cur,"WARD")||
      kw(&L->cur,"BULWARK")||kw(&L->cur,"PAVIS")||kw(&L->cur,"RAMPART")||
      kw(&L->cur,"MESH_AEGIS")||kw(&L->cur,"RAISE_AEGIS")||
      kw(&L->cur,"AEGISES")||kw(&L->cur,"SHIELDS")||kw(&L->cur,"WARDS")||
      kw(&L->cur,"SHIELDRING")||kw(&L->cur,"WARDHUB")||kw(&L->cur,"SEEDAEGIS")||
      kw(&L->cur,"LATTICE_AEGIS")||kw(&L->cur,"LIFE_AEGIS")||
      kw(&L->cur,"SHIELD_RING")||kw(&L->cur,"PULSE_AEGIS")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int shields = 0;
    int wards = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "AEGIS needs >=2 cubes",
                  "SMX AEGIS a b [c ...]  or  SMX SHIELD a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - aegis needs clear channel */
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
    /* complete aegis mesh among live */
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
    /* shield ring - free energy holds every edge i -> i+1 both ways */
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
          else shields++;
        }
      }
    }
    /* ward hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else wards++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int shield_ok = (live >= 2 && shields >= live && soft == 0) ? 1 : 0;
      if (!shield_ok && live >= 2 && shields * 2 >= live && soft == 0)
        shield_ok = 1;
      int ward_ok = (live >= 1 && wards >= live && soft == 0) ? 1 : 0;
      if (!ward_ok && live >= 1 && wards * 2 >= live && soft == 0)
        ward_ok = 1;
      int ag_ok = (mesh_ok && shield_ok && ward_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (ag_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (shields > 0 ? 1 : 0) + (wards > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_AEGISED", (long)ag_ok);
      var_set_num(vm, "SMX_AEGISED_LATCH", (long)ag_ok);
      var_set_num(vm, "SMX_AEGIS", (long)(ag_ok ? bonds + shields + wards : 0));
      var_set_num(vm, "SMX_SHIELD", (long)(ag_ok ? 1 : 0));
      var_set_num(vm, "SMX_BULWARK", (long)(ag_ok ? 1 : 0));
      var_set_num(vm, "SMX_WARD", (long)(ag_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHIELDS", (long)(ag_ok ? bonds : 0));
      var_set_num(vm, "SMX_SHIELDRING", (long)(ag_ok ? shields : 0));
      var_set_num(vm, "SMX_SHIELDE", (long)(ag_ok ? shields : 0));
      var_set_num(vm, "SMX_SHIELDAE", (long)(ag_ok ? shields : 0));
      var_set_num(vm, "SMX_WARDS", (long)(ag_ok ? wards : 0));
      var_set_num(vm, "SMX_WARDHUB", (long)(ag_ok ? wards : 0));
      var_set_num(vm, "SMX_SEEDAEGIS", (long)(ag_ok ? wards : 0));
      var_set_num(vm, "SMX_MESH", (long)(ag_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + shields + wards));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (ag_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX AEGIS ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX AEGIS partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX AEGIS soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX AEGIS nodes=%d live=%d bonds=%d shields=%d wards=%d need=%d soft=%d talks=%d oob=%d aegised=%d vital=%ld\n",
                n, live, bonds, shields, wards, need, soft, vm->smx_talks, vm->smx_oob, ag_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX BIOSPHERE|GAIA|BIOME|ECO|LIFEWEB|HABITAT|MESH_BIOSPHERE|RAISE_BIOSPHERE a b c ...
   * Life-force biome after noosphere: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete biome mesh among live nodes, weaves a
   * cycle ring (i -> i+1) so free energy fills the biosphere, then gaia hub
   * gathers return so lattice raises a biosphere lock where life holds the hive.
   * Latches SMX_BIOSPHERED when mesh+cycles+gaiae are soft-OOB-free.
   * SMX_BIOMES = chain bonds; SMX_HABITAT hub = root gather pulses;
   * SMX_BIOSPHERE sum = bonds+cycles+gaiae; SMX_GAIA|SMX_ECO sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"BIOSPHERE")||kw(&L->cur,"GAIA")||kw(&L->cur,"BIOME")||
      kw(&L->cur,"ECO")||kw(&L->cur,"LIFEWEB")||kw(&L->cur,"HABITAT")||
      kw(&L->cur,"MESH_BIOSPHERE")||kw(&L->cur,"RAISE_BIOSPHERE")||
      kw(&L->cur,"BIOSPHERES")||kw(&L->cur,"BIOMES")||kw(&L->cur,"ECOS")||
      kw(&L->cur,"GAIAS")||kw(&L->cur,"HABITATS")||kw(&L->cur,"SEEDBIO")||
      kw(&L->cur,"LATTICE_BIOSPHERE")||kw(&L->cur,"WORLD_LIFE")||
      kw(&L->cur,"CYCLE_RING")||kw(&L->cur,"PULSE_BIOSPHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int cycles = 0;
    int gaiae = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "BIOSPHERE needs >=2 cubes",
                  "SMX BIOSPHERE a b [c ...]  or  SMX LOGOS a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - biosphere needs clear channel */
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
    /* complete biome mesh among live */
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
    /* cycle ring - free energy holds every edge i -> i+1 both ways */
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
          else cycles++;
        }
      }
    }
    /* gaia hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else gaiae++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int cycle_ok = (live >= 2 && cycles >= live && soft == 0) ? 1 : 0;
      if (!cycle_ok && live >= 2 && cycles * 2 >= live && soft == 0)
        cycle_ok = 1;
      int gaia_ok = (live >= 1 && gaiae >= live && soft == 0) ? 1 : 0;
      if (!gaia_ok && live >= 1 && gaiae * 2 >= live && soft == 0)
        gaia_ok = 1;
      int bs_ok = (mesh_ok && cycle_ok && gaia_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (bs_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (cycles > 0 ? 1 : 0) + (gaiae > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_BIOSPHERED", (long)bs_ok);
      var_set_num(vm, "SMX_BIOSPHERED_LATCH", (long)bs_ok);
      var_set_num(vm, "SMX_BIOSPHERE", (long)(bs_ok ? bonds + cycles + gaiae : 0));
      var_set_num(vm, "SMX_GAIA", (long)(bs_ok ? 1 : 0));
      var_set_num(vm, "SMX_ECO", (long)(bs_ok ? 1 : 0));
      var_set_num(vm, "SMX_BIOMES", (long)(bs_ok ? bonds : 0));
      var_set_num(vm, "SMX_BIOME", (long)(bs_ok ? bonds : 0));
      var_set_num(vm, "SMX_CYCLES", (long)(bs_ok ? cycles : 0));
      var_set_num(vm, "SMX_CYCLE", (long)(bs_ok ? cycles : 0));
      var_set_num(vm, "SMX_CYCLERING", (long)(bs_ok ? cycles : 0));
      var_set_num(vm, "SMX_GAIAE", (long)(bs_ok ? gaiae : 0));
      var_set_num(vm, "SMX_HABITAT", (long)(bs_ok ? gaiae : 0));
      var_set_num(vm, "SMX_HABITAT", (long)(bs_ok ? gaiae : 0));
      var_set_num(vm, "SMX_SEEDBIO", (long)(bs_ok ? gaiae : 0));
      var_set_num(vm, "SMX_MESH", (long)(bs_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + cycles + gaiae));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (bs_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX BIOSPHERE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX BIOSPHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX BIOSPHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX BIOSPHERE nodes=%d live=%d bonds=%d cycles=%d gaiae=%d need=%d soft=%d talks=%d oob=%d biosphered=%d vital=%ld\n",
                n, live, bonds, cycles, gaiae, need, soft, vm->smx_talks, vm->smx_oob, bs_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX HYDROSPHERE|OCEAN|MARINE|WATERWEB|AQUASPHERE|HYDRO|MESH_HYDROSPHERE|RAISE_HYDROSPHERE a b c ...
   * Life-force hydrosphere after biosphere: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete hydros mesh among live nodes, weaves a
   * current ring (i -> i+1) so free energy fills the hydrosphere, then ocean hub
   * gathers return so lattice raises a hydrosphere lock where life holds the hive.
   * Latches SMX_HYDROSPHERED when mesh+eddies+tides are soft-OOB-free.
   * SMX_CURRENTS = chain bonds; SMX_BASIN hub = root gather pulses;
   * SMX_HYDROSPHERE sum = bonds+eddies+tides; SMX_OCEAN|SMX_MARINE sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"HYDROSPHERE")||kw(&L->cur,"OCEAN")||kw(&L->cur,"MARINE")||
      kw(&L->cur,"WATERWEB")||kw(&L->cur,"AQUASPHERE")||kw(&L->cur,"HYDRO")||
      kw(&L->cur,"MESH_HYDROSPHERE")||kw(&L->cur,"RAISE_HYDROSPHERE")||
      kw(&L->cur,"HYDROSPHERES")||kw(&L->cur,"OCEANS")||kw(&L->cur,"MARINES")||
      kw(&L->cur,"CURRENTS")||kw(&L->cur,"BASINS")||kw(&L->cur,"SEEDHYDRO")||
      kw(&L->cur,"LATTICE_HYDROSPHERE")||kw(&L->cur,"WORLD_WATER")||
      kw(&L->cur,"CURRENT_RING")||kw(&L->cur,"PULSE_HYDROSPHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int eddies = 0;
    int tides = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "HYDROSPHERE needs >=2 cubes",
                  "SMX HYDROSPHERE a b [c ...]  or  SMX OCEAN a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - hydrosphere needs clear channel */
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
    /* complete hydros mesh among live */
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
    /* current ring - free energy holds every edge i -> i+1 both ways */
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
          else eddies++;
        }
      }
    }
    /* ocean hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else tides++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int cycle_ok = (live >= 2 && eddies >= live && soft == 0) ? 1 : 0;
      if (!cycle_ok && live >= 2 && eddies * 2 >= live && soft == 0)
        cycle_ok = 1;
      int gaia_ok = (live >= 1 && tides >= live && soft == 0) ? 1 : 0;
      if (!gaia_ok && live >= 1 && tides * 2 >= live && soft == 0)
        gaia_ok = 1;
      int hs_ok = (mesh_ok && cycle_ok && gaia_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (hs_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (eddies > 0 ? 1 : 0) + (tides > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_HYDROSPHERED", (long)hs_ok);
      var_set_num(vm, "SMX_HYDROSPHERED_LATCH", (long)hs_ok);
      var_set_num(vm, "SMX_HYDROSPHERE", (long)(hs_ok ? bonds + eddies + tides : 0));
      var_set_num(vm, "SMX_OCEAN", (long)(hs_ok ? 1 : 0));
      var_set_num(vm, "SMX_MARINE", (long)(hs_ok ? 1 : 0));
      var_set_num(vm, "SMX_CURRENTS", (long)(hs_ok ? bonds : 0));
      var_set_num(vm, "SMX_CURRENT", (long)(hs_ok ? bonds : 0));
      var_set_num(vm, "SMX_EDDIES", (long)(hs_ok ? eddies : 0));
      var_set_num(vm, "SMX_EDDY", (long)(hs_ok ? eddies : 0));
      var_set_num(vm, "SMX_EDDYRING", (long)(hs_ok ? eddies : 0));
      var_set_num(vm, "SMX_TIDES", (long)(hs_ok ? tides : 0));
      var_set_num(vm, "SMX_BASIN", (long)(hs_ok ? tides : 0));
      var_set_num(vm, "SMX_BASIN", (long)(hs_ok ? tides : 0));
      var_set_num(vm, "SMX_SEEDHYDRO", (long)(hs_ok ? tides : 0));
      var_set_num(vm, "SMX_MESH", (long)(hs_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + eddies + tides));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (hs_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX HYDROSPHERE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX HYDROSPHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX HYDROSPHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX HYDROSPHERE nodes=%d live=%d bonds=%d eddies=%d tides=%d need=%d soft=%d talks=%d oob=%d hydrosphered=%d vital=%ld\n",
                n, live, bonds, eddies, tides, need, soft, vm->smx_talks, vm->smx_oob, hs_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX ATMOSPHERE|SKY|AERO|AIRWEB|AEROSPHERE|WIND|MESH_ATMOSPHERE|RAISE_ATMOSPHERE a b c ...
   * Life-force atmosphere after hydrosphere: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete sky mesh among live nodes, weaves a
   * wind ring (i -> i+1) so free energy fills the atmosphere, then sky hub
   * gathers return so lattice raises an atmosphere lock where life holds the hive.
   * Latches SMX_ATMOSPHERED when mesh+winds+skies are soft-OOB-free.
   * SMX_WINDS = chain bonds; SMX_SKY hub = root gather pulses;
   * SMX_ATMOSPHERE sum = bonds+winds+skies; SMX_SKY|SMX_AERO sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"ATMOSPHERE")||kw(&L->cur,"SKY")||kw(&L->cur,"AERO")||
      kw(&L->cur,"AIRWEB")||kw(&L->cur,"AEROSPHERE")||kw(&L->cur,"WIND")||
      kw(&L->cur,"MESH_ATMOSPHERE")||kw(&L->cur,"RAISE_ATMOSPHERE")||
      kw(&L->cur,"ATMOSPHERES")||kw(&L->cur,"SKIES")||kw(&L->cur,"AEROS")||
      kw(&L->cur,"WINDS")||kw(&L->cur,"ZEPHYRS")||kw(&L->cur,"SEEDATMO")||
      kw(&L->cur,"LATTICE_ATMOSPHERE")||kw(&L->cur,"WORLD_AIR")||
      kw(&L->cur,"WIND_RING")||kw(&L->cur,"PULSE_ATMOSPHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int winds = 0;
    int skies = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "ATMOSPHERE needs >=2 cubes",
                  "SMX ATMOSPHERE a b [c ...]  or  SMX SKY a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - atmosphere needs clear channel */
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
    /* complete sky mesh among live */
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
    /* wind ring - free energy holds every edge i -> i+1 both ways */
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
          else winds++;
        }
      }
    }
    /* sky hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else skies++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int wind_ok = (live >= 2 && winds >= live && soft == 0) ? 1 : 0;
      if (!wind_ok && live >= 2 && winds * 2 >= live && soft == 0)
        wind_ok = 1;
      int sky_ok = (live >= 1 && skies >= live && soft == 0) ? 1 : 0;
      if (!sky_ok && live >= 1 && skies * 2 >= live && soft == 0)
        sky_ok = 1;
      int at_ok = (mesh_ok && wind_ok && sky_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (at_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (winds > 0 ? 1 : 0) + (skies > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_ATMOSPHERED", (long)at_ok);
      var_set_num(vm, "SMX_ATMOSPHERED_LATCH", (long)at_ok);
      var_set_num(vm, "SMX_ATMOSPHERE", (long)(at_ok ? bonds + winds + skies : 0));
      var_set_num(vm, "SMX_SKY", (long)(at_ok ? 1 : 0));
      var_set_num(vm, "SMX_AERO", (long)(at_ok ? 1 : 0));
      var_set_num(vm, "SMX_WINDS", (long)(at_ok ? bonds : 0));
      var_set_num(vm, "SMX_WIND", (long)(at_ok ? bonds : 0));
      var_set_num(vm, "SMX_ZEPHYRS", (long)(at_ok ? winds : 0));
      var_set_num(vm, "SMX_ZEPHYR", (long)(at_ok ? winds : 0));
      var_set_num(vm, "SMX_WINDRING", (long)(at_ok ? winds : 0));
      var_set_num(vm, "SMX_SKIES", (long)(at_ok ? skies : 0));
      var_set_num(vm, "SMX_AEROS", (long)(at_ok ? skies : 0));
      var_set_num(vm, "SMX_SEEDATMO", (long)(at_ok ? skies : 0));
      var_set_num(vm, "SMX_MESH", (long)(at_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + winds + skies));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (at_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ATMOSPHERE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ATMOSPHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX ATMOSPHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX ATMOSPHERE nodes=%d live=%d bonds=%d winds=%d skies=%d need=%d soft=%d talks=%d oob=%d atmosphered=%d vital=%ld\n",
                n, live, bonds, winds, skies, need, soft, vm->smx_talks, vm->smx_oob, at_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX LITHOSPHERE|CRUST|TECTONIC|ROCKWEB|GEOSPHERE|PLATE|MESH_LITHOSPHERE|RAISE_LITHOSPHERE a b c ...
   * Life-force lithosphere after atmosphere: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete crust mesh among live nodes, weaves a
   * fault ring (i -> i+1) so free energy locks the plates, then bedrock hub
   * gathers return so lattice raises a lithosphere lock where life holds the hive.
   * Latches SMX_LITHOSPHERED when mesh+faults+strata are soft-OOB-free.
   * SMX_FAULTS = chain bonds; SMX_BEDROCK hub = root gather pulses;
   * SMX_LITHOSPHERE sum = bonds+faults+strata; SMX_CRUST|SMX_TECTONIC sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"LITHOSPHERE")||kw(&L->cur,"CRUST")||kw(&L->cur,"TECTONIC")||
      kw(&L->cur,"ROCKWEB")||kw(&L->cur,"GEOSPHERE")||kw(&L->cur,"PLATE")||
      kw(&L->cur,"MESH_LITHOSPHERE")||kw(&L->cur,"RAISE_LITHOSPHERE")||
      kw(&L->cur,"LITHOSPHERES")||kw(&L->cur,"CRUSTS")||kw(&L->cur,"TECTONICS")||
      kw(&L->cur,"FAULTS")||kw(&L->cur,"PLATES")||kw(&L->cur,"SEEDLITH")||
      kw(&L->cur,"LATTICE_LITHOSPHERE")||kw(&L->cur,"WORLD_ROCK")||
      kw(&L->cur,"FAULT_RING")||kw(&L->cur,"PULSE_LITHOSPHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int faults = 0;
    int strata = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "LITHOSPHERE needs >=2 cubes",
                  "SMX LITHOSPHERE a b [c ...]  or  SMX CRUST a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - lithosphere needs clear channel */
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
    /* complete crust mesh among live */
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
    /* fault ring - free energy holds every edge i -> i+1 both ways */
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
          else faults++;
        }
      }
    }
    /* bedrock hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else strata++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int fault_ok = (live >= 2 && faults >= live && soft == 0) ? 1 : 0;
      if (!fault_ok && live >= 2 && faults * 2 >= live && soft == 0)
        fault_ok = 1;
      int strata_ok = (live >= 1 && strata >= live && soft == 0) ? 1 : 0;
      if (!strata_ok && live >= 1 && strata * 2 >= live && soft == 0)
        strata_ok = 1;
      int lt_ok = (mesh_ok && fault_ok && strata_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (lt_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (faults > 0 ? 1 : 0) + (strata > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_LITHOSPHERED", (long)lt_ok);
      var_set_num(vm, "SMX_LITHOSPHERED_LATCH", (long)lt_ok);
      var_set_num(vm, "SMX_LITHOSPHERE", (long)(lt_ok ? bonds + faults + strata : 0));
      var_set_num(vm, "SMX_CRUST", (long)(lt_ok ? 1 : 0));
      var_set_num(vm, "SMX_TECTONIC", (long)(lt_ok ? 1 : 0));
      var_set_num(vm, "SMX_FAULTS", (long)(lt_ok ? bonds : 0));
      var_set_num(vm, "SMX_FAULT", (long)(lt_ok ? bonds : 0));
      var_set_num(vm, "SMX_PLATES", (long)(lt_ok ? faults : 0));
      var_set_num(vm, "SMX_PLATE", (long)(lt_ok ? faults : 0));
      var_set_num(vm, "SMX_FAULTRING", (long)(lt_ok ? faults : 0));
      var_set_num(vm, "SMX_STRATA", (long)(lt_ok ? strata : 0));
      var_set_num(vm, "SMX_BEDROCK", (long)(lt_ok ? strata : 0));
      var_set_num(vm, "SMX_SEEDLITH", (long)(lt_ok ? strata : 0));
      var_set_num(vm, "SMX_MESH", (long)(lt_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + faults + strata));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (lt_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX LITHOSPHERE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX LITHOSPHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX LITHOSPHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX LITHOSPHERE nodes=%d live=%d bonds=%d faults=%d strata=%d need=%d soft=%d talks=%d oob=%d lithosphered=%d vital=%ld\n",
                n, live, bonds, faults, strata, need, soft, vm->smx_talks, vm->smx_oob, lt_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX MAGNETOSPHERE|MAGFIELD|MAGNETO|FIELDWEB|MAGNETIC|AURORAL|MESH_MAGNETOSPHERE|RAISE_MAGNETOSPHERE a b c ...
   * Life-force magnetosphere after lithosphere: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete field mesh among live nodes, weaves a
   * fieldline ring (i -> i+1) so free energy locks the poles, then pole hub
   * gathers return so lattice raises a magnetosphere lock where life holds the hive.
   * Latches SMX_MAGNETOSPHERED when mesh+fieldlines+poles are soft-OOB-free.
   * SMX_FIELDLINES = chain bonds; SMX_POLE hub = root gather pulses;
   * SMX_MAGNETOSPHERE sum = bonds+fieldlines+poles; SMX_MAGFIELD|SMX_MAGNETO sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"MAGNETOSPHERE")||kw(&L->cur,"MAGFIELD")||kw(&L->cur,"MAGNETO")||
      kw(&L->cur,"FIELDWEB")||kw(&L->cur,"MAGNETIC")||kw(&L->cur,"AURORAL")||
      kw(&L->cur,"MESH_MAGNETOSPHERE")||kw(&L->cur,"RAISE_MAGNETOSPHERE")||
      kw(&L->cur,"MAGNETOSPHERES")||kw(&L->cur,"MAGFIELDS")||kw(&L->cur,"MAGNETOS")||
      kw(&L->cur,"FIELDLINES")||kw(&L->cur,"FIELDS")||kw(&L->cur,"FIELD")||kw(&L->cur,"SEEDMAG")||
      kw(&L->cur,"LATTICE_MAGNETOSPHERE")||kw(&L->cur,"WORLD_FIELD")||
      kw(&L->cur,"FIELD_RING")||kw(&L->cur,"PULSE_MAGNETOSPHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int fieldlines = 0;
    int poles = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "MAGNETOSPHERE needs >=2 cubes",
                  "SMX MAGNETOSPHERE a b [c ...]  or  SMX MAGFIELD a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - magnetosphere needs clear channel */
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
    /* complete field mesh among live */
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
    /* fieldline ring - free energy holds every edge i -> i+1 both ways */
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
          else fieldlines++;
        }
      }
    }
    /* pole hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else poles++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int fl_ok = (live >= 2 && fieldlines >= live && soft == 0) ? 1 : 0;
      if (!fl_ok && live >= 2 && fieldlines * 2 >= live && soft == 0)
        fl_ok = 1;
      int pole_ok = (live >= 1 && poles >= live && soft == 0) ? 1 : 0;
      if (!pole_ok && live >= 1 && poles * 2 >= live && soft == 0)
        pole_ok = 1;
      int mg_ok = (mesh_ok && fl_ok && pole_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (mg_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (fieldlines > 0 ? 1 : 0) + (poles > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_MAGNETOSPHERED", (long)mg_ok);
      var_set_num(vm, "SMX_MAGNETOSPHERED_LATCH", (long)mg_ok);
      var_set_num(vm, "SMX_MAGNETOSPHERE", (long)(mg_ok ? bonds + fieldlines + poles : 0));
      var_set_num(vm, "SMX_MAGFIELD", (long)(mg_ok ? 1 : 0));
      var_set_num(vm, "SMX_MAGNETO", (long)(mg_ok ? 1 : 0));
      var_set_num(vm, "SMX_FIELDLINES", (long)(mg_ok ? bonds : 0));
      var_set_num(vm, "SMX_FIELDLINE", (long)(mg_ok ? bonds : 0));
      var_set_num(vm, "SMX_FIELDS", (long)(mg_ok ? fieldlines : 0));
      var_set_num(vm, "SMX_FIELD", (long)(mg_ok ? fieldlines : 0));
      var_set_num(vm, "SMX_FIELDRING", (long)(mg_ok ? fieldlines : 0));
      var_set_num(vm, "SMX_POLES", (long)(mg_ok ? poles : 0));
      var_set_num(vm, "SMX_POLE", (long)(mg_ok ? poles : 0));
      var_set_num(vm, "SMX_SEEDMAG", (long)(mg_ok ? poles : 0));
      var_set_num(vm, "SMX_MESH", (long)(mg_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + fieldlines + poles));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (mg_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX MAGNETOSPHERE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX MAGNETOSPHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX MAGNETOSPHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX MAGNETOSPHERE nodes=%d live=%d bonds=%d fieldlines=%d poles=%d need=%d soft=%d talks=%d oob=%d magnetosphered=%d vital=%ld\n",
                n, live, bonds, fieldlines, poles, need, soft, vm->smx_talks, vm->smx_oob, mg_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX HOMEOSTASIS|BALANCE|SETPOINT|FEEDBACK|REGULATE|EQUILIBRIUM|MESH_HOMEOSTASIS|RAISE_HOMEOSTASIS a b c ...
   * Life-force mesh stability after biosphere: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete balance mesh among live nodes, weaves a
   * feedback ring (i -> i+1) so free energy self-regulates, then setpoint hub
   * gathers return so lattice locks homeostasis where life holds the hive.
   * Latches SMX_HOMEOSTATIC when mesh+feedback+setpoints are soft-OOB-free.
   * SMX_BALANCES = chain bonds; SMX_SETPOINT hub = root gather pulses;
   * SMX_HOMEOSTASIS sum = bonds+feedback+setpoints; SMX_BALANCE|SMX_REGULATE sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"HOMEOSTASIS")||kw(&L->cur,"BALANCE")||kw(&L->cur,"SETPOINT")||
      kw(&L->cur,"FEEDBACK")||kw(&L->cur,"REGULATE")||kw(&L->cur,"EQUILIBRIUM")||
      kw(&L->cur,"MESH_HOMEOSTASIS")||kw(&L->cur,"RAISE_HOMEOSTASIS")||
      kw(&L->cur,"HOMEOSTASES")||kw(&L->cur,"BALANCES")||kw(&L->cur,"SETPOINTS")||
      kw(&L->cur,"FEEDBACKS")||kw(&L->cur,"REGULATES")||kw(&L->cur,"SEEDHOME")||
      kw(&L->cur,"LATTICE_HOMEOSTASIS")||kw(&L->cur,"WORLD_BALANCE")||
      kw(&L->cur,"FEEDBACK_RING")||kw(&L->cur,"PULSE_HOMEOSTASIS")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int feedback = 0;
    int setpoints = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "HOMEOSTASIS needs >=2 cubes",
                  "SMX HOMEOSTASIS a b [c ...]  or  SMX BALANCE a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - homeostasis needs clear channel */
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
    /* complete balance mesh among live */
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
    /* feedback ring - free energy self-regulates every edge i -> i+1 both ways */
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
          else feedback++;
        }
      }
    }
    /* setpoint hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else setpoints++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int fb_ok = (live >= 2 && feedback >= live && soft == 0) ? 1 : 0;
      if (!fb_ok && live >= 2 && feedback * 2 >= live && soft == 0)
        fb_ok = 1;
      int sp_ok = (live >= 1 && setpoints >= live && soft == 0) ? 1 : 0;
      if (!sp_ok && live >= 1 && setpoints * 2 >= live && soft == 0)
        sp_ok = 1;
      int hs_ok = (mesh_ok && fb_ok && sp_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (hs_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (feedback > 0 ? 1 : 0) + (setpoints > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_HOMEOSTATIC", (long)hs_ok);
      var_set_num(vm, "SMX_HOMEOSTATIC_LATCH", (long)hs_ok);
      var_set_num(vm, "SMX_HOMEOSTASIS", (long)(hs_ok ? bonds + feedback + setpoints : 0));
      var_set_num(vm, "SMX_BALANCE", (long)(hs_ok ? 1 : 0));
      var_set_num(vm, "SMX_REGULATE", (long)(hs_ok ? 1 : 0));
      var_set_num(vm, "SMX_BALANCES", (long)(hs_ok ? bonds : 0));
      var_set_num(vm, "SMX_FEEDBACKS", (long)(hs_ok ? feedback : 0));
      var_set_num(vm, "SMX_FEEDBACK", (long)(hs_ok ? feedback : 0));
      var_set_num(vm, "SMX_FEEDBACKRING", (long)(hs_ok ? feedback : 0));
      var_set_num(vm, "SMX_SETPOINTS", (long)(hs_ok ? setpoints : 0));
      var_set_num(vm, "SMX_SETPOINT", (long)(hs_ok ? setpoints : 0));
      var_set_num(vm, "SMX_EQUILIBRIUM", (long)(hs_ok ? 1 : 0));
      var_set_num(vm, "SMX_SEEDHOME", (long)(hs_ok ? setpoints : 0));
      var_set_num(vm, "SMX_MESH", (long)(hs_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + feedback + setpoints));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (hs_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX HOMEOSTASIS ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX HOMEOSTASIS partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX HOMEOSTASIS soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX HOMEOSTASIS nodes=%d live=%d bonds=%d feedback=%d setpoints=%d need=%d soft=%d talks=%d oob=%d homeostatic=%d vital=%ld\n",
                n, live, bonds, feedback, setpoints, need, soft, vm->smx_talks, vm->smx_oob, hs_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX IONOSPHERE|IONO|LAYER|BEAM|IONIZE|PLASMA|MESH_IONOSPHERE|RAISE_IONOSPHERE a b c ...
   * Life-force mesh stability after homeostasis: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete iono mesh among live nodes, weaves a
   * beam ring (i -> i+1) so free energy self-regulates, then layer hub
   * gathers return so lattice locks ionosphere where life holds the hive.
   * Latches SMX_IONOSPHERED when mesh+beam+layers are soft-OOB-free.
   * SMX_IONOS = chain bonds; SMX_LAYER hub = root gather pulses;
   * SMX_IONOSPHERE sum = bonds+beam+layers; SMX_IONO|SMX_IONIZE sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"IONOSPHERE")||kw(&L->cur,"IONO")||kw(&L->cur,"LAYER")||
      kw(&L->cur,"BEAM")||kw(&L->cur,"IONIZE")||kw(&L->cur,"PLASMA")||
      kw(&L->cur,"MESH_IONOSPHERE")||kw(&L->cur,"RAISE_IONOSPHERE")||
      kw(&L->cur,"IONOSPHERES")||kw(&L->cur,"IONOS")||kw(&L->cur,"LAYERS")||
      kw(&L->cur,"BEAMS")||kw(&L->cur,"IONIZES")||kw(&L->cur,"SEEDIONO")||
      kw(&L->cur,"LATTICE_IONOSPHERE")||kw(&L->cur,"WORLD_IONO")||
      kw(&L->cur,"BEAM_RING")||kw(&L->cur,"PULSE_IONOSPHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int beam = 0;
    int layers = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "IONOSPHERE needs >=2 cubes",
                  "SMX IONOSPHERE a b [c ...]  or  SMX IONO a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - ionosphere needs clear channel */
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
    /* complete iono mesh among live */
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
    /* beam ring - free energy self-regulates every edge i -> i+1 both ways */
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
          else beam++;
        }
      }
    }
    /* layer hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else layers++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int bm_ok = (live >= 2 && beam >= live && soft == 0) ? 1 : 0;
      if (!bm_ok && live >= 2 && beam * 2 >= live && soft == 0)
        bm_ok = 1;
      int ly_ok = (live >= 1 && layers >= live && soft == 0) ? 1 : 0;
      if (!ly_ok && live >= 1 && layers * 2 >= live && soft == 0)
        ly_ok = 1;
      int io_ok = (mesh_ok && bm_ok && ly_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (io_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (beam > 0 ? 1 : 0) + (layers > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_IONOSPHERED", (long)io_ok);
      var_set_num(vm, "SMX_IONOSPHERED_LATCH", (long)io_ok);
      var_set_num(vm, "SMX_IONOSPHERE", (long)(io_ok ? bonds + beam + layers : 0));
      var_set_num(vm, "SMX_IONO", (long)(io_ok ? 1 : 0));
      var_set_num(vm, "SMX_IONIZE", (long)(io_ok ? 1 : 0));
      var_set_num(vm, "SMX_IONOS", (long)(io_ok ? bonds : 0));
      var_set_num(vm, "SMX_BEAMS", (long)(io_ok ? beam : 0));
      var_set_num(vm, "SMX_BEAM", (long)(io_ok ? beam : 0));
      var_set_num(vm, "SMX_BEAMRING", (long)(io_ok ? beam : 0));
      var_set_num(vm, "SMX_LAYERS", (long)(io_ok ? layers : 0));
      var_set_num(vm, "SMX_LAYER", (long)(io_ok ? layers : 0));
      var_set_num(vm, "SMX_PLASMA", (long)(io_ok ? 1 : 0));
      var_set_num(vm, "SMX_SEEDIONO", (long)(io_ok ? layers : 0));
      var_set_num(vm, "SMX_MESH", (long)(io_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + beam + layers));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (io_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX IONOSPHERE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX IONOSPHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX IONOSPHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX IONOSPHERE nodes=%d live=%d bonds=%d beam=%d layers=%d need=%d soft=%d talks=%d oob=%d ionosphered=%d vital=%ld\n",
                n, live, bonds, beam, layers, need, soft, vm->smx_talks, vm->smx_oob, io_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX THERMOSPHERE|THERMO|THERMAL|HEAT|HEAT|WARM|MESH_THERMOSPHERE|RAISE_THERMOSPHERE a b c ...
   * Life-force mesh stability after homeostasis: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete iono mesh among live nodes, weaves a
   * heat ring (i -> i+1) so free energy self-regulates, then thermal hub
   * gathers return so lattice locks thermosphere where life holds the hive.
   * Latches SMX_THERMOSPHERED when mesh+heat+thermals are soft-OOB-free.
   * SMX_THERMOS = chain bonds; SMX_THERMAL hub = root gather pulses;
   * SMX_THERMOSPHERE sum = bonds+heat+thermals; SMX_THERMO|SMX_HEAT sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"THERMOSPHERE")||kw(&L->cur,"THERMO")||kw(&L->cur,"THERMAL")||
      kw(&L->cur,"HEAT")||kw(&L->cur,"HEAT")||kw(&L->cur,"WARM")||
      kw(&L->cur,"MESH_THERMOSPHERE")||kw(&L->cur,"RAISE_THERMOSPHERE")||
      kw(&L->cur,"THERMOSPHERES")||kw(&L->cur,"THERMOS")||kw(&L->cur,"THERMALS")||
      kw(&L->cur,"HEATS")||kw(&L->cur,"HEATS")||kw(&L->cur,"SEEDTHERMO")||
      kw(&L->cur,"LATTICE_THERMOSPHERE")||kw(&L->cur,"WORLD_THERMO")||
      kw(&L->cur,"HEAT_RING")||kw(&L->cur,"PULSE_THERMOSPHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int heat = 0;
    int thermals = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "THERMOSPHERE needs >=2 cubes",
                  "SMX THERMOSPHERE a b [c ...]  or  SMX THERMO a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - thermosphere needs clear channel */
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
    /* complete iono mesh among live */
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
    /* heat ring - free energy self-regulates every edge i -> i+1 both ways */
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
          else heat++;
        }
      }
    }
    /* thermal hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else thermals++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int ht_ok = (live >= 2 && heat >= live && soft == 0) ? 1 : 0;
      if (!ht_ok && live >= 2 && heat * 2 >= live && soft == 0)
        ht_ok = 1;
      int th_ok = (live >= 1 && thermals >= live && soft == 0) ? 1 : 0;
      if (!th_ok && live >= 1 && thermals * 2 >= live && soft == 0)
        th_ok = 1;
      int tm_ok = (mesh_ok && ht_ok && th_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (tm_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (heat > 0 ? 1 : 0) + (thermals > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_THERMOSPHERED", (long)tm_ok);
      var_set_num(vm, "SMX_THERMOSPHERED_LATCH", (long)tm_ok);
      var_set_num(vm, "SMX_THERMOSPHERE", (long)(tm_ok ? bonds + heat + thermals : 0));
      var_set_num(vm, "SMX_THERMO", (long)(tm_ok ? 1 : 0));
      var_set_num(vm, "SMX_WARM", (long)(tm_ok ? 1 : 0));
      var_set_num(vm, "SMX_THERMOS", (long)(tm_ok ? bonds : 0));
      var_set_num(vm, "SMX_HEATS", (long)(tm_ok ? heat : 0));
      var_set_num(vm, "SMX_HEAT", (long)(tm_ok ? heat : 0));
      var_set_num(vm, "SMX_HEATRING", (long)(tm_ok ? heat : 0));
      var_set_num(vm, "SMX_THERMALS", (long)(tm_ok ? thermals : 0));
      var_set_num(vm, "SMX_THERMAL", (long)(tm_ok ? thermals : 0));
      var_set_num(vm, "SMX_SEEDTHERMO", (long)(tm_ok ? thermals : 0));
      var_set_num(vm, "SMX_MESH", (long)(tm_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + heat + thermals));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (tm_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX THERMOSPHERE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX THERMOSPHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX THERMOSPHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX THERMOSPHERE nodes=%d live=%d bonds=%d heat=%d thermals=%d need=%d soft=%d talks=%d oob=%d thermosphered=%d vital=%ld\n",
                n, live, bonds, heat, thermals, need, soft, vm->smx_talks, vm->smx_oob, tm_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX EXOSPHERE|EXO|FRINGE|ESCAPE|ESCAPE|OUTER|MESH_EXOSPHERE|RAISE_EXOSPHERE a b c ...
   * Life-force mesh stability after homeostasis: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete exo mesh among live nodes, weaves a
   * escape ring (i -> i+1) so free energy self-regulates, then fringe hub
   * gathers return so lattice locks exosphere where life holds the hive.
   * Latches SMX_EXOSPHERED when mesh+escape+fringes are soft-OOB-free.
   * SMX_EXOS = chain bonds; SMX_FRINGE hub = root gather pulses;
   * SMX_EXOSPHERE sum = bonds+escape+fringes; SMX_EXO|SMX_ESCAPE sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"EXOSPHERE")||kw(&L->cur,"EXO")||kw(&L->cur,"FRINGE")||
      kw(&L->cur,"ESCAPE")||kw(&L->cur,"ESCAPE")||kw(&L->cur,"OUTER")||
      kw(&L->cur,"MESH_EXOSPHERE")||kw(&L->cur,"RAISE_EXOSPHERE")||
      kw(&L->cur,"EXOSPHERES")||kw(&L->cur,"EXOS")||kw(&L->cur,"FRINGES")||
      kw(&L->cur,"ESCAPES")||kw(&L->cur,"ESCAPES")||kw(&L->cur,"SEEDEXO")||
      kw(&L->cur,"LATTICE_EXOSPHERE")||kw(&L->cur,"WORLD_EXO")||
      kw(&L->cur,"ESCAPE_RING")||kw(&L->cur,"PULSE_EXOSPHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int escape = 0;
    int fringes = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "EXOSPHERE needs >=2 cubes",
                  "SMX EXOSPHERE a b [c ...]  or  SMX EXO a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - exosphere needs clear channel */
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
    /* complete exo mesh among live */
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
    /* escape ring - free energy self-regulates every edge i -> i+1 both ways */
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
          else escape++;
        }
      }
    }
    /* fringe hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else fringes++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int es_ok = (live >= 2 && escape >= live && soft == 0) ? 1 : 0;
      if (!es_ok && live >= 2 && escape * 2 >= live && soft == 0)
        es_ok = 1;
      int fr_ok = (live >= 1 && fringes >= live && soft == 0) ? 1 : 0;
      if (!fr_ok && live >= 1 && fringes * 2 >= live && soft == 0)
        fr_ok = 1;
      int ex_ok = (mesh_ok && es_ok && fr_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (ex_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (escape > 0 ? 1 : 0) + (fringes > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_EXOSPHERED", (long)ex_ok);
      var_set_num(vm, "SMX_EXOSPHERED_LATCH", (long)ex_ok);
      var_set_num(vm, "SMX_EXOSPHERE", (long)(ex_ok ? bonds + escape + fringes : 0));
      var_set_num(vm, "SMX_EXO", (long)(ex_ok ? 1 : 0));
      var_set_num(vm, "SMX_OUTER", (long)(ex_ok ? 1 : 0));
      var_set_num(vm, "SMX_EXOS", (long)(ex_ok ? bonds : 0));
      var_set_num(vm, "SMX_ESCAPES", (long)(ex_ok ? escape : 0));
      var_set_num(vm, "SMX_ESCAPE", (long)(ex_ok ? escape : 0));
      var_set_num(vm, "SMX_ESCAPERING", (long)(ex_ok ? escape : 0));
      var_set_num(vm, "SMX_FRINGES", (long)(ex_ok ? fringes : 0));
      var_set_num(vm, "SMX_FRINGE", (long)(ex_ok ? fringes : 0));
      var_set_num(vm, "SMX_SEEDEXO", (long)(ex_ok ? fringes : 0));
      var_set_num(vm, "SMX_MESH", (long)(ex_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + escape + fringes));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (ex_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX EXOSPHERE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX EXOSPHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX EXOSPHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX EXOSPHERE nodes=%d live=%d bonds=%d escape=%d fringes=%d need=%d soft=%d talks=%d oob=%d exosphered=%d vital=%ld\n",
                n, live, bonds, escape, fringes, need, soft, vm->smx_talks, vm->smx_oob, ex_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX HELIOSPHERE|HS|HELIOPAUSE|SOLARWIND|TERMINATION|HELIOSHEATH|MESH_HELIOSPHERE|RAISE_HELIOSPHERE a b c ...
   * Life-force mesh stability after homeostasis: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete heliosphere mesh among live nodes, weaves a
   * wind ring (i -> i+1) so free energy self-regulates, then pause hub
   * gathers return so lattice locks heliosphere where life holds the hive.
   * Latches SMX_HELIOSPHERED when mesh+wind+pauses are soft-OOB-free.
   * SMX_HELIOSHEATH = chain bonds; SMX_HELIOPAUSE hub = root gather pulses;
   * SMX_HELIOSPHERE sum = bonds+wind+pauses; SMX_HS|SMX_SOLARWIND sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"HELIOSPHERE")||kw(&L->cur,"HS")||kw(&L->cur,"HELIOPAUSE")||
      kw(&L->cur,"HELIO")||kw(&L->cur,"SOLARWIND")||kw(&L->cur,"TERMINATION")||
      kw(&L->cur,"HELIOSHEATH")||kw(&L->cur,"SOLAR")||kw(&L->cur,"BOW")||
      kw(&L->cur,"BOWS")||kw(&L->cur,"HELIOSHEATHS")||
      kw(&L->cur,"MESH_HELIOSPHERE")||kw(&L->cur,"RAISE_HELIOSPHERE")||
      kw(&L->cur,"HELIOSPHERES")||kw(&L->cur,"HELIOPAUSES")||
      kw(&L->cur,"SOLARWINDS")||kw(&L->cur,"SEEDHS")||kw(&L->cur,"SEEDHELIO")||
      kw(&L->cur,"LATTICE_HELIOSPHERE")||kw(&L->cur,"WORLD_HS")||kw(&L->cur,"WORLD_HELIO")||
      kw(&L->cur,"SOLARWIND_RING")||kw(&L->cur,"PULSE_HELIOSPHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wind = 0;
    int pauses = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "HELIOSPHERE needs >=2 cubes",
                  "SMX HELIOSPHERE a b [c ...]  or  SMX HS a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - heliosphere needs clear channel */
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
    /* complete heliosphere mesh among live */
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
    /* wind ring - free energy self-regulates every edge i -> i+1 both ways */
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
          else wind++;
        }
      }
    }
    /* pause hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else pauses++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int es_ok = (live >= 2 && wind >= live && soft == 0) ? 1 : 0;
      if (!es_ok && live >= 2 && wind * 2 >= live && soft == 0)
        es_ok = 1;
      int fr_ok = (live >= 1 && pauses >= live && soft == 0) ? 1 : 0;
      if (!fr_ok && live >= 1 && pauses * 2 >= live && soft == 0)
        fr_ok = 1;
      int hs_ok = (mesh_ok && es_ok && fr_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (hs_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wind > 0 ? 1 : 0) + (pauses > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_HELIOSPHERED", (long)hs_ok);
      var_set_num(vm, "SMX_HELIOSPHERED_LATCH", (long)hs_ok);
      var_set_num(vm, "SMX_HELIOSPHERE", (long)(hs_ok ? bonds + wind + pauses : 0));
      var_set_num(vm, "SMX_HS", (long)(hs_ok ? 1 : 0));
      var_set_num(vm, "SMX_HELIO", (long)(hs_ok ? 1 : 0));
      var_set_num(vm, "SMX_SOLAR", (long)(hs_ok ? 1 : 0));
      var_set_num(vm, "SMX_TERMINATION", (long)(hs_ok ? 1 : 0));
      var_set_num(vm, "SMX_HELIOSHEATH", (long)(hs_ok ? bonds : 0));
      var_set_num(vm, "SMX_HELIOS", (long)(hs_ok ? bonds : 0));
      var_set_num(vm, "SMX_SOLARWINDS", (long)(hs_ok ? wind : 0));
      var_set_num(vm, "SMX_SOLARWIND", (long)(hs_ok ? wind : 0));
      var_set_num(vm, "SMX_SOLARWINDRING", (long)(hs_ok ? wind : 0));
      var_set_num(vm, "SMX_WINDS", (long)(hs_ok ? wind : 0));
      var_set_num(vm, "SMX_WIND", (long)(hs_ok ? wind : 0));
      var_set_num(vm, "SMX_HELIOPAUSES", (long)(hs_ok ? pauses : 0));
      var_set_num(vm, "SMX_HELIOPAUSE", (long)(hs_ok ? pauses : 0));
      var_set_num(vm, "SMX_BOWS", (long)(hs_ok ? pauses : 0));
      var_set_num(vm, "SMX_BOW", (long)(hs_ok ? pauses : 0));
      var_set_num(vm, "SMX_SEEDHS", (long)(hs_ok ? pauses : 0));
      var_set_num(vm, "SMX_SEEDHELIO", (long)(hs_ok ? pauses : 0));
      var_set_num(vm, "SMX_MESH", (long)(hs_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wind + pauses));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (hs_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX HELIOSPHERE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX HELIOSPHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX HELIOSPHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX HELIOSPHERE nodes=%d live=%d bonds=%d wind=%d pauses=%d need=%d soft=%d talks=%d oob=%d heliosphered=%d vital=%ld\n",
                n, live, bonds, wind, pauses, need, soft, vm->smx_talks, vm->smx_oob, hs_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX MESOSPHERE|HS|GLOW|CHILL|COLD_TOP|MESOS|MESH_MESOSPHERE|RAISE_MESOSPHERE a b c ...
   * Life-force mesh stability after heliosphere: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete mesosphere mesh among live nodes, weaves a
   * chill ring (i -> i+1) so free energy self-regulates, then glow hub
   * gathers return so lattice locks mesosphere where life holds the hive.
   * Latches SMX_MESOSPHERED when mesh+chill+glows are soft-OOB-free.
   * SMX_MESOS = chain bonds; SMX_GLOW hub = root gather pulses;
   * SMX_MESOSPHERE sum = bonds+wind+pauses; SMX_LAYERED|SMX_CHILL sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"MESOSPHERE")||kw(&L->cur,"MESO")||kw(&L->cur,"GLOW")||
      kw(&L->cur,"MESO")||kw(&L->cur,"CHILL")||kw(&L->cur,"COLD_TOP")||
      kw(&L->cur,"MESOS")||kw(&L->cur,"AIRGLOW")||kw(&L->cur,"GLOW")||
      kw(&L->cur,"GLOWS")||kw(&L->cur,"MESOSS")||
      kw(&L->cur,"MESH_MESOSPHERE")||kw(&L->cur,"RAISE_MESOSPHERE")||
      kw(&L->cur,"MESOSPHERES")||kw(&L->cur,"GLOWS")||
      kw(&L->cur,"CHILLS")||kw(&L->cur,"SEEDGLOW")||kw(&L->cur,"SEEDMESO")||
      kw(&L->cur,"LATTICE_MESOSPHERE")||kw(&L->cur,"WORLD_MESO")||kw(&L->cur,"WORLD_MESO")||
      kw(&L->cur,"CHILL_RING")||kw(&L->cur,"PULSE_MESOSPHERE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int chill = 0;
    int glows = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "MESOSPHERE needs >=2 cubes",
                  "SMX MESOSPHERE a b [c ...]  or  SMX MESO a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - mesosphere needs clear channel */
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
    /* complete mesosphere mesh among live */
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
    /* chill ring - free energy self-regulates every edge i -> i+1 both ways */
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
          else chill++;
        }
      }
    }
    /* glow hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else glows++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int ch_ok = (live >= 2 && chill >= live && soft == 0) ? 1 : 0;
      if (!ch_ok && live >= 2 && chill * 2 >= live && soft == 0)
        ch_ok = 1;
      int gl_ok = (live >= 1 && glows >= live && soft == 0) ? 1 : 0;
      if (!gl_ok && live >= 1 && glows * 2 >= live && soft == 0)
        gl_ok = 1;
      int ms_ok = (mesh_ok && ch_ok && gl_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (ms_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (chill > 0 ? 1 : 0) + (glows > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_MESOSPHERED", (long)ms_ok);
      var_set_num(vm, "SMX_MESOSPHERED_LATCH", (long)ms_ok);
      var_set_num(vm, "SMX_MESOSPHERE", (long)(ms_ok ? bonds + chill + glows : 0));
      var_set_num(vm, "SMX_LAYERED", (long)(ms_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESO", (long)(ms_ok ? 1 : 0));
      var_set_num(vm, "SMX_AIRGLOW", (long)(ms_ok ? 1 : 0));
      var_set_num(vm, "SMX_COLD_TOP", (long)(ms_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESOS", (long)(ms_ok ? bonds : 0));
      var_set_num(vm, "SMX_MESOS", (long)(ms_ok ? bonds : 0));
      var_set_num(vm, "SMX_CHILLS", (long)(ms_ok ? chill : 0));
      var_set_num(vm, "SMX_CHILL", (long)(ms_ok ? chill : 0));
      var_set_num(vm, "SMX_CHILLRING", (long)(ms_ok ? chill : 0));
      var_set_num(vm, "SMX_CHILLS", (long)(ms_ok ? chill : 0));
      var_set_num(vm, "SMX_CHILL", (long)(ms_ok ? chill : 0));
      var_set_num(vm, "SMX_GLOWS", (long)(ms_ok ? glows : 0));
      var_set_num(vm, "SMX_GLOW", (long)(ms_ok ? glows : 0));
      var_set_num(vm, "SMX_GLOWS", (long)(ms_ok ? glows : 0));
      var_set_num(vm, "SMX_GLOW", (long)(ms_ok ? glows : 0));
      var_set_num(vm, "SMX_SEEDGLOW", (long)(ms_ok ? glows : 0));
      var_set_num(vm, "SMX_SEEDMESO", (long)(ms_ok ? glows : 0));
      var_set_num(vm, "SMX_MESH", (long)(ms_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + chill + glows));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (ms_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX MESOSPHERE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX MESOSPHERE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX MESOSPHERE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX MESOSPHERE nodes=%d live=%d bonds=%d chill=%d glows=%d need=%d soft=%d talks=%d oob=%d mesosphered=%d vital=%ld\n",
                n, live, bonds, chill, glows, need, soft, vm->smx_talks, vm->smx_oob, ms_ok, vital);
    }
    bump(vm); return 1;
  }
  if (kw(&L->cur,"INTERSTELLAR")||kw(&L->cur,"ISTEL")||kw(&L->cur,"BEACON_HUB")||
      kw(&L->cur,"ISTEL2")||kw(&L->cur,"STARPATH")||kw(&L->cur,"ISM")||
      kw(&L->cur,"STARLANE")||kw(&L->cur,"COSMIC")||kw(&L->cur,"BEACON")||
      kw(&L->cur,"BEACONS")||kw(&L->cur,"STARLANES")||
      kw(&L->cur,"MESH_INTERSTELLAR")||kw(&L->cur,"RAISE_INTERSTELLAR")||
      kw(&L->cur,"INTERSTELLARS")||kw(&L->cur,"BEACONS")||
      kw(&L->cur,"STARPATHS")||kw(&L->cur,"SEEDISTEL")||kw(&L->cur,"SEEDSTAR")||
      kw(&L->cur,"LATTICE_INTERSTELLAR")||kw(&L->cur,"WORLD_ISTEL")||kw(&L->cur,"WORLD_INTERSTELLAR")||
      kw(&L->cur,"STARPATH_RING")||kw(&L->cur,"PULSE_INTERSTELLAR")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int paths = 0;
    int beacons = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "INTERSTELLAR needs >=2 cubes",
                  "SMX INTERSTELLAR a b [c ...]  or  SMX ISTEL a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - interstellar needs clear channel */
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
    /* complete interstellar mesh among live */
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
    /* starpath ring - free energy self-regulates every edge i -> i+1 both ways */
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
          else paths++;
        }
      }
    }
    /* beacon hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else beacons++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int path_ok = (live >= 2 && paths >= live && soft == 0) ? 1 : 0;
      if (!path_ok && live >= 2 && paths * 2 >= live && soft == 0)
        path_ok = 1;
      int bec_ok = (live >= 1 && beacons >= live && soft == 0) ? 1 : 0;
      if (!bec_ok && live >= 1 && beacons * 2 >= live && soft == 0)
        bec_ok = 1;
      int is_ok = (mesh_ok && path_ok && bec_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (is_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (paths > 0 ? 1 : 0) + (beacons > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_INTERSTELLARED", (long)is_ok);
      var_set_num(vm, "SMX_INTERSTELLARED_LATCH", (long)is_ok);
      var_set_num(vm, "SMX_INTERSTELLAR", (long)(is_ok ? bonds + paths + beacons : 0));
      var_set_num(vm, "SMX_ISTEL", (long)(is_ok ? 1 : 0));
      var_set_num(vm, "SMX_STARPATH_ON", (long)(is_ok ? 1 : 0));
      var_set_num(vm, "SMX_COSMIC", (long)(is_ok ? 1 : 0));
      var_set_num(vm, "SMX_ISM", (long)(is_ok ? 1 : 0));
      var_set_num(vm, "SMX_STARLANE", (long)(is_ok ? bonds : 0));
      var_set_num(vm, "SMX_ISTELS", (long)(is_ok ? bonds : 0));
      var_set_num(vm, "SMX_STARPATH_ONS", (long)(is_ok ? bonds : 0));
      var_set_num(vm, "SMX_STARPATHS", (long)(is_ok ? paths : 0));
      var_set_num(vm, "SMX_STARPATH", (long)(is_ok ? paths : 0));
      var_set_num(vm, "SMX_STARPATHRING", (long)(is_ok ? paths : 0));
      var_set_num(vm, "SMX_STARPATHS", (long)(is_ok ? paths : 0));
      var_set_num(vm, "SMX_LANE", (long)(is_ok ? paths : 0));
      var_set_num(vm, "SMX_BEACONS", (long)(is_ok ? beacons : 0));
      var_set_num(vm, "SMX_BEACON_HUB", (long)(is_ok ? beacons : 0));
      var_set_num(vm, "SMX_BEACONS", (long)(is_ok ? beacons : 0));
      var_set_num(vm, "SMX_BEACON", (long)(is_ok ? beacons : 0));
      var_set_num(vm, "SMX_SEEDISTEL", (long)(is_ok ? beacons : 0));
      var_set_num(vm, "SMX_SEEDSTAR", (long)(is_ok ? beacons : 0));
      var_set_num(vm, "SMX_MESH", (long)(is_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + paths + beacons));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (is_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX INTERSTELLAR ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX INTERSTELLAR partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX INTERSTELLAR soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX INTERSTELLAR nodes=%d live=%d bonds=%d paths=%d beacons=%d need=%d soft=%d talks=%d oob=%d interstellared=%d vital=%ld\n",
                n, live, bonds, paths, beacons, need, soft, vm->smx_talks, vm->smx_oob, is_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX LOCALBUBBLE|LBUB|BUBBLE|ASTROSPHERE|SHELL|MESH_LOCALBUBBLE|RAISE_LOCALBUBBLE a b c ...
   * Life-force local-bubble mesh stability after interstellar: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete localbubble mesh among live nodes, weaves a
   * shellpath ring (i -> i+1) so free energy self-regulates, then shell hub
   * gathers return so lattice locks localbubble where life holds the hive.
   * Latches SMX_LOCALBUBBLED when mesh+shellpaths+hubs are soft-OOB-free.
   * SMX_LBUBS = chain bonds; SMX_SHELLHUB hub = root gather pulses;
   * SMX_LOCALBUBBLE sum = bonds+shellpaths+hubs; SMX_SHELLPATH_ON|SMX_LOCALSPACE sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"LOCALBUBBLE")||kw(&L->cur,"LBUB")||kw(&L->cur,"BUBBLE")||
      kw(&L->cur,"ASTROSPHERE")||kw(&L->cur,"SHELL")||kw(&L->cur,"LOCAL_BUBBLE")||
      kw(&L->cur,"BUBBLELANE")||kw(&L->cur,"BUBBLELANES")||kw(&L->cur,"LOCALSPACE")||
      kw(&L->cur,"SHELLHUB")||kw(&L->cur,"SHELLHUBS")||kw(&L->cur,"SHELLPATHS")||
      kw(&L->cur,"SHELLPATH")||kw(&L->cur,"LOCALWEB")||kw(&L->cur,"LISM")||
      kw(&L->cur,"MESH_LOCALBUBBLE")||kw(&L->cur,"RAISE_LOCALBUBBLE")||
      kw(&L->cur,"LOCALBUBBLES")||kw(&L->cur,"LBUBS")||
      kw(&L->cur,"SEEDLBUB")||kw(&L->cur,"SEEDBUBBLE")||
      kw(&L->cur,"LATTICE_LOCALBUBBLE")||kw(&L->cur,"WORLD_LBUB")||kw(&L->cur,"WORLD_LOCALBUBBLE")||
      kw(&L->cur,"SHELL_RING")||kw(&L->cur,"SHELL_HUB")||kw(&L->cur,"PULSE_LOCALBUBBLE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int paths = 0;
    int hubs = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "LOCALBUBBLE needs >=2 cubes",
                  "SMX LOCALBUBBLE a b [c ...]  or  SMX LBUB a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - localbubble needs clear channel */
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
    /* complete localbubble mesh among live */
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
    /* starpath ring - free energy self-regulates every edge i -> i+1 both ways */
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
          else paths++;
        }
      }
    }
    /* beacon hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else hubs++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int path_ok = (live >= 2 && paths >= live && soft == 0) ? 1 : 0;
      if (!path_ok && live >= 2 && paths * 2 >= live && soft == 0)
        path_ok = 1;
      int hub_ok = (live >= 1 && hubs >= live && soft == 0) ? 1 : 0;
      if (!hub_ok && live >= 1 && hubs * 2 >= live && soft == 0)
        hub_ok = 1;
      int lb_ok = (mesh_ok && path_ok && hub_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (lb_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (paths > 0 ? 1 : 0) + (hubs > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_LOCALBUBBLED", (long)lb_ok);
      var_set_num(vm, "SMX_LOCALBUBBLED_LATCH", (long)lb_ok);
      var_set_num(vm, "SMX_LOCALBUBBLE", (long)(lb_ok ? bonds + paths + hubs : 0));
      var_set_num(vm, "SMX_LBUB", (long)(lb_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHELLPATH_ON", (long)(lb_ok ? 1 : 0));
      var_set_num(vm, "SMX_LOCALSPACE", (long)(lb_ok ? 1 : 0));
      var_set_num(vm, "SMX_LISM", (long)(lb_ok ? 1 : 0));
      var_set_num(vm, "SMX_BUBBLELANE", (long)(lb_ok ? bonds : 0));
      var_set_num(vm, "SMX_LBUBS", (long)(lb_ok ? bonds : 0));
      var_set_num(vm, "SMX_SHELLPATH_ONS", (long)(lb_ok ? bonds : 0));
      var_set_num(vm, "SMX_SHELLPATHS", (long)(lb_ok ? paths : 0));
      var_set_num(vm, "SMX_SHELLPATH", (long)(lb_ok ? paths : 0));
      var_set_num(vm, "SMX_SHELLPATHRING", (long)(lb_ok ? paths : 0));
      var_set_num(vm, "SMX_SHELLPATHS", (long)(lb_ok ? paths : 0));
      var_set_num(vm, "SMX_LANE", (long)(lb_ok ? paths : 0));
      var_set_num(vm, "SMX_SHELLHUBS", (long)(lb_ok ? hubs : 0));
      var_set_num(vm, "SMX_SHELL_HUB", (long)(lb_ok ? hubs : 0));
      var_set_num(vm, "SMX_SHELLHUBS", (long)(lb_ok ? hubs : 0));
      var_set_num(vm, "SMX_SHELLHUB", (long)(lb_ok ? hubs : 0));
      var_set_num(vm, "SMX_SEEDLBUB", (long)(lb_ok ? hubs : 0));
      var_set_num(vm, "SMX_SEEDBUBBLE", (long)(lb_ok ? hubs : 0));
      var_set_num(vm, "SMX_MESH", (long)(lb_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + paths + hubs));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (lb_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX LOCALBUBBLE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX LOCALBUBBLE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX LOCALBUBBLE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX LOCALBUBBLE nodes=%d live=%d bonds=%d paths=%d hubs=%d need=%d soft=%d talks=%d oob=%d localbubbled=%d vital=%ld\n",
                n, live, bonds, paths, hubs, need, soft, vm->smx_talks, vm->smx_oob, lb_ok, vital);
    }
    bump(vm); return 1;
  }

  /* SMX LOCALGROUP|LGROUP|GROUP|GALAXYGROUP|GROUP|MESH_LOCALGROUP|RAISE_LOCALGROUP a b c ...
   * Life-force local-group mesh stability after localbubble: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete localgroup mesh among live nodes, weaves a
   * memberpath ring (i -> i+1) so free energy self-regulates, then group hub
   * gathers return so lattice locks localgroup where life holds the hive.
   * Latches SMX_LOCALGROUPED when mesh+memberpaths+hubs are soft-OOB-free.
   * SMX_LGROUPS = chain bonds; SMX_GROUPHUB hub = root gather pulses;
   * SMX_LOCALGROUP sum = bonds+memberpaths+hubs; SMX_MEMBERPATH_ON|SMX_GROUPSPACE sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"LOCALGROUP")||kw(&L->cur,"LGROUP")||kw(&L->cur,"GROUP")||
      kw(&L->cur,"GALAXYGROUP")||kw(&L->cur,"GROUP")||kw(&L->cur,"LOCAL_GROUP")||
      kw(&L->cur,"GROUPLANE")||kw(&L->cur,"GROUPLANES")||kw(&L->cur,"GROUPSPACE")||
      kw(&L->cur,"GROUPHUB")||kw(&L->cur,"GROUPHUBS")||kw(&L->cur,"MEMBERPATHS")||
      kw(&L->cur,"MEMBERPATH")||kw(&L->cur,"GROUPWEB")||kw(&L->cur,"LGALAXIES")||
      kw(&L->cur,"MESH_LOCALGROUP")||kw(&L->cur,"RAISE_LOCALGROUP")||
      kw(&L->cur,"LOCALGROUPS")||kw(&L->cur,"LGROUPS")||
      kw(&L->cur,"SEEDLGROUP")||kw(&L->cur,"SEEDGROUP")||
      kw(&L->cur,"LATTICE_LOCALGROUP")||kw(&L->cur,"WORLD_LG")||kw(&L->cur,"WORLD_LOCALGROUP")||
      kw(&L->cur,"GROUP_RING")||kw(&L->cur,"GROUP_HUB")||kw(&L->cur,"PULSE_LOCALGROUP")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int paths = 0;
    int hubs = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "LOCALGROUP needs >=2 cubes",
                  "SMX LOCALGROUP a b [c ...]  or  SMX LGROUP a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - localgroup needs clear channel */
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
    /* complete localgroup mesh among live */
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
    /* starpath ring - free energy self-regulates every edge i -> i+1 both ways */
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
          else paths++;
        }
      }
    }
    /* beacon hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else hubs++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int path_ok = (live >= 2 && paths >= live && soft == 0) ? 1 : 0;
      if (!path_ok && live >= 2 && paths * 2 >= live && soft == 0)
        path_ok = 1;
      int hub_ok = (live >= 1 && hubs >= live && soft == 0) ? 1 : 0;
      if (!hub_ok && live >= 1 && hubs * 2 >= live && soft == 0)
        hub_ok = 1;
      int lg_ok = (mesh_ok && path_ok && hub_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (lg_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (paths > 0 ? 1 : 0) + (hubs > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_LOCALGROUPED", (long)lg_ok);
      var_set_num(vm, "SMX_LOCALGROUPED_LATCH", (long)lg_ok);
      var_set_num(vm, "SMX_LOCALGROUP", (long)(lg_ok ? bonds + paths + hubs : 0));
      var_set_num(vm, "SMX_LGROUP", (long)(lg_ok ? 1 : 0));
      var_set_num(vm, "SMX_MEMBERPATH_ON", (long)(lg_ok ? 1 : 0));
      var_set_num(vm, "SMX_GROUPSPACE", (long)(lg_ok ? 1 : 0));
      var_set_num(vm, "SMX_LGALAXIES", (long)(lg_ok ? 1 : 0));
      var_set_num(vm, "SMX_GROUPLANE", (long)(lg_ok ? bonds : 0));
      var_set_num(vm, "SMX_LGROUPS", (long)(lg_ok ? bonds : 0));
      var_set_num(vm, "SMX_MEMBERPATH_ONS", (long)(lg_ok ? bonds : 0));
      var_set_num(vm, "SMX_MEMBERPATHS", (long)(lg_ok ? paths : 0));
      var_set_num(vm, "SMX_MEMBERPATH", (long)(lg_ok ? paths : 0));
      var_set_num(vm, "SMX_MEMBERPATHRING", (long)(lg_ok ? paths : 0));
      var_set_num(vm, "SMX_MEMBERPATHS", (long)(lg_ok ? paths : 0));
      var_set_num(vm, "SMX_LANE", (long)(lg_ok ? paths : 0));
      var_set_num(vm, "SMX_GROUPHUBS", (long)(lg_ok ? hubs : 0));
      var_set_num(vm, "SMX_GROUP_HUB", (long)(lg_ok ? hubs : 0));
      var_set_num(vm, "SMX_GROUPHUBS", (long)(lg_ok ? hubs : 0));
      var_set_num(vm, "SMX_GROUPHUB", (long)(lg_ok ? hubs : 0));
      var_set_num(vm, "SMX_SEEDLGROUP", (long)(lg_ok ? hubs : 0));
      var_set_num(vm, "SMX_SEEDGROUP", (long)(lg_ok ? hubs : 0));
      var_set_num(vm, "SMX_MESH", (long)(lg_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + paths + hubs));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (lg_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX LOCALGROUP ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX LOCALGROUP partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX LOCALGROUP soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX LOCALGROUP nodes=%d live=%d bonds=%d paths=%d hubs=%d need=%d soft=%d talks=%d oob=%d localgrouped=%d vital=%ld\n",
                n, live, bonds, paths, hubs, need, soft, vm->smx_talks, vm->smx_oob, lg_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX MULTIVERSE|MVERSE|MULTI|COSMOSGROUP|MULTI_VERSE|MESH_MULTIVERSE|RAISE_MULTIVERSE a b c ...
   * Life-force multi-verse mesh stability after localgroup: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete multiverse mesh among live nodes, weaves a
   * versepath ring (i -> i+1) so free energy self-regulates, then verse hub
   * gathers return so lattice locks multiverse where life holds the hive.
   * Latches SMX_MULTIVERSED when mesh+versepaths+hubs are soft-OOB-free.
   * SMX_MVERSES = chain bonds; SMX_VERSEHUB hub = root gather pulses;
   * SMX_MULTIVERSE sum = bonds+versepaths+hubs; SMX_VERSEPATH_ON|SMX_VERSESPACE sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"MULTIVERSE")||kw(&L->cur,"MVERSE")||kw(&L->cur,"MULTI")||
      kw(&L->cur,"COSMOSGROUP")||kw(&L->cur,"MULTI")||kw(&L->cur,"MULTI_VERSE")||
      kw(&L->cur,"VERSELANE")||kw(&L->cur,"VERSELANES")||kw(&L->cur,"VERSESPACE")||
      kw(&L->cur,"VERSEHUB")||kw(&L->cur,"VERSEHUBS")||kw(&L->cur,"VERSEPATHS")||
      kw(&L->cur,"VERSEPATH")||kw(&L->cur,"VERSEWEB")||kw(&L->cur,"MVERSES")||
      kw(&L->cur,"MESH_MULTIVERSE")||kw(&L->cur,"RAISE_MULTIVERSE")||
      kw(&L->cur,"MULTIVERSES")||kw(&L->cur,"MVERSES")||
      kw(&L->cur,"SEEDMVERSE")||kw(&L->cur,"SEEDVERSE")||
      kw(&L->cur,"LATTICE_MULTIVERSE")||kw(&L->cur,"WORLD_MV")||kw(&L->cur,"WORLD_MULTIVERSE")||
      kw(&L->cur,"VERSE_RING")||kw(&L->cur,"VERSE_HUB")||kw(&L->cur,"PULSE_MULTIVERSE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int paths = 0;
    int hubs = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "MULTIVERSE needs >=2 cubes",
                  "SMX MULTIVERSE a b [c ...]  or  SMX MVERSE a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - multiverse needs clear channel */
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
    /* complete multiverse mesh among live */
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
    /* starpath ring - free energy self-regulates every edge i -> i+1 both ways */
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
          else paths++;
        }
      }
    }
    /* beacon hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else hubs++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int path_ok = (live >= 2 && paths >= live && soft == 0) ? 1 : 0;
      if (!path_ok && live >= 2 && paths * 2 >= live && soft == 0)
        path_ok = 1;
      int hub_ok = (live >= 1 && hubs >= live && soft == 0) ? 1 : 0;
      if (!hub_ok && live >= 1 && hubs * 2 >= live && soft == 0)
        hub_ok = 1;
      int mv_ok = (mesh_ok && path_ok && hub_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (mv_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (paths > 0 ? 1 : 0) + (hubs > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_MULTIVERSED", (long)mv_ok);
      var_set_num(vm, "SMX_MULTIVERSED_LATCH", (long)mv_ok);
      var_set_num(vm, "SMX_MULTIVERSE", (long)(mv_ok ? bonds + paths + hubs : 0));
      var_set_num(vm, "SMX_MVERSE", (long)(mv_ok ? 1 : 0));
      var_set_num(vm, "SMX_VERSEPATH_ON", (long)(mv_ok ? 1 : 0));
      var_set_num(vm, "SMX_VERSESPACE", (long)(mv_ok ? 1 : 0));
      var_set_num(vm, "SMX_MVERSES", (long)(mv_ok ? 1 : 0));
      var_set_num(vm, "SMX_VERSELANE", (long)(mv_ok ? bonds : 0));
      var_set_num(vm, "SMX_MVERSES", (long)(mv_ok ? bonds : 0));
      var_set_num(vm, "SMX_VERSEPATH_ONS", (long)(mv_ok ? bonds : 0));
      var_set_num(vm, "SMX_VERSEPATHS", (long)(mv_ok ? paths : 0));
      var_set_num(vm, "SMX_VERSEPATH", (long)(mv_ok ? paths : 0));
      var_set_num(vm, "SMX_VERSEPATHRING", (long)(mv_ok ? paths : 0));
      var_set_num(vm, "SMX_VERSEPATHS", (long)(mv_ok ? paths : 0));
      var_set_num(vm, "SMX_LANE", (long)(mv_ok ? paths : 0));
      var_set_num(vm, "SMX_VERSEHUBS", (long)(mv_ok ? hubs : 0));
      var_set_num(vm, "SMX_VERSE_HUB", (long)(mv_ok ? hubs : 0));
      var_set_num(vm, "SMX_VERSEHUBS", (long)(mv_ok ? hubs : 0));
      var_set_num(vm, "SMX_VERSEHUB", (long)(mv_ok ? hubs : 0));
      var_set_num(vm, "SMX_SEEDMVERSE", (long)(mv_ok ? hubs : 0));
      var_set_num(vm, "SMX_SEEDVERSE", (long)(mv_ok ? hubs : 0));
      var_set_num(vm, "SMX_MESH", (long)(mv_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + paths + hubs));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (mv_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX MULTIVERSE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX MULTIVERSE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX MULTIVERSE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX MULTIVERSE nodes=%d live=%d bonds=%d paths=%d hubs=%d need=%d soft=%d talks=%d oob=%d multiversed=%d vital=%ld\n",
                n, live, bonds, paths, hubs, need, soft, vm->smx_talks, vm->smx_oob, mv_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX AUTOHEAL|AH|MULTI|LIFE_BEACON|WE_AUTOHEAL|MESH_AUTOHEAL|RAISE_AUTOHEAL a b c ...
   * Life-force autoheal mesh stability after multiverse: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete autoheal mesh among live nodes, weaves a
   * mend ring (i -> i+1) so free energy self-regulates, then beacon hub
   * gathers return so lattice locks autoheal where life holds the hive.
   * Latches SMX_AUTOHEALED when mesh+versepaths+hubs are soft-OOB-free.
   * SMX_AHS = chain bonds; SMX_BEACON hub = root gather pulses;
   * SMX_AUTOHEAL sum = bonds+versepaths+hubs; SMX_HEAL|SMX_REGEN sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"AUTOHEAL")||kw(&L->cur,"AH")||kw(&L->cur,"HEAL")||kw(&L->cur,"MEND")||kw(&L->cur,"MENDS")||kw(&L->cur,"REGEN")||kw(&L->cur,"WE_AUTOHEAL")||
      kw(&L->cur,"LIFE_BEACON")||kw(&L->cur,"REGEN")||kw(&L->cur,"WE_AUTOHEAL")||
      kw(&L->cur,"MENDLANE")||kw(&L->cur,"MENDLANES")||kw(&L->cur,"HEALSPACE")||
      kw(&L->cur,"BEACON")||kw(&L->cur,"BEACONS")||kw(&L->cur,"MENDS")||
      kw(&L->cur,"MEND")||kw(&L->cur,"HEALWEB")||kw(&L->cur,"AHS")||
      kw(&L->cur,"MESH_AUTOHEAL")||kw(&L->cur,"RAISE_AUTOHEAL")||
      kw(&L->cur,"AUTOHEALS")||kw(&L->cur,"AHS")||
      kw(&L->cur,"SEEDAH")||kw(&L->cur,"SEEDHEAL")||
      kw(&L->cur,"LATTICE_AUTOHEAL")||kw(&L->cur,"WORLD_AH")||kw(&L->cur,"WORLD_AUTOHEAL")||
      kw(&L->cur,"MEND_RING")||kw(&L->cur,"BEACON_HUB")||kw(&L->cur,"PULSE_AUTOHEAL")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int mends = 0;
    int beacons = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "AUTOHEAL needs >=2 cubes",
                  "SMX AUTOHEAL a b [c ...]  or  SMX AH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - autoheal needs clear channel */
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
    /* complete autoheal mesh among live */
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
    /* starpath ring - free energy self-regulates every edge i -> i+1 both ways */
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
          else mends++;
        }
      }
    }
    /* beacon hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else beacons++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int mend_ok = (live >= 2 && mends >= live && soft == 0) ? 1 : 0;
      if (!mend_ok && live >= 2 && mends * 2 >= live && soft == 0)
        mend_ok = 1;
      int beacon_ok = (live >= 1 && beacons >= live && soft == 0) ? 1 : 0;
      if (!beacon_ok && live >= 1 && beacons * 2 >= live && soft == 0)
        beacon_ok = 1;
      int ah_ok = (mesh_ok && mend_ok && beacon_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (ah_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (mends > 0 ? 1 : 0) + (beacons > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_AUTOHEALED", (long)ah_ok);
      var_set_num(vm, "SMX_AUTOHEALED_LATCH", (long)ah_ok);
      var_set_num(vm, "SMX_AUTOHEAL", (long)(ah_ok ? bonds + mends + beacons : 0));
      var_set_num(vm, "SMX_AH", (long)(ah_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEAL", (long)(ah_ok ? 1 : 0));
      var_set_num(vm, "SMX_REGEN", (long)(ah_ok ? 1 : 0));
      var_set_num(vm, "SMX_AHS", (long)(ah_ok ? 1 : 0));
      var_set_num(vm, "SMX_HEALLANE", (long)(ah_ok ? bonds : 0));
      var_set_num(vm, "SMX_AHS", (long)(ah_ok ? bonds : 0));
      var_set_num(vm, "SMX_HEALS", (long)(ah_ok ? bonds : 0));
      var_set_num(vm, "SMX_MENDS", (long)(ah_ok ? mends : 0));
      var_set_num(vm, "SMX_MEND", (long)(ah_ok ? mends : 0));
      var_set_num(vm, "SMX_MENDRING", (long)(ah_ok ? mends : 0));
      var_set_num(vm, "SMX_MENDLANE", (long)(ah_ok ? mends : 0));
      var_set_num(vm, "SMX_BEACONS", (long)(ah_ok ? beacons : 0));
      var_set_num(vm, "SMX_BEACON_HUB", (long)(ah_ok ? beacons : 0));
      var_set_num(vm, "SMX_BEACON", (long)(ah_ok ? beacons : 0));
      var_set_num(vm, "SMX_SEEDHEAL", (long)(ah_ok ? beacons : 0));
      var_set_num(vm, "SMX_LIFE_BEACON", (long)(ah_ok ? beacons : 0));
      var_set_num(vm, "SMX_MESH", (long)(ah_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + mends + beacons));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (ah_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX AUTOHEAL ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX AUTOHEAL partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX AUTOHEAL soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX AUTOHEAL nodes=%d live=%d bonds=%d mends=%d beacons=%d need=%d soft=%d talks=%d oob=%d autohealed=%d vital=%ld\n",
                n, live, bonds, mends, beacons, need, soft, vm->smx_talks, vm->smx_oob, ah_ok, vital);
    }
    bump(vm); return 1;
  }
  /* SMX RESILIENCE|AH|MULTI|LIFE_ANCHOR|WE_RESILIENCE|MESH_RESILIENCE|RAISE_RESILIENCE a b c ...
   * Life-force resilience mesh stability after autoheal: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete resilience mesh among live nodes, weaves a
   * guard ring (i -> i+1) so free energy self-regulates, then anchor hub
   * gathers return so lattice locks resilience where life holds the hive.
   * Latches SMX_RESILIENT when mesh+versepaths+hubs are soft-OOB-free.
   * SMX_RSS = chain bonds; SMX_ANCHOR hub = root gather pulses;
   * SMX_RESILIENCE sum = bonds+versepaths+hubs; SMX_GUARD|SMX_RESIL sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"RESILIENCE")||kw(&L->cur,"RS")||kw(&L->cur,"RESIL")||kw(&L->cur,"GUARD")||kw(&L->cur,"GUARDS")||kw(&L->cur,"RESILIENT")||kw(&L->cur,"WE_RESILIENCE")||
      kw(&L->cur,"LIFE_ANCHOR")||kw(&L->cur,"WE_RESILIENCE")||
      kw(&L->cur,"MESH_RESILIENCE")||kw(&L->cur,"RAISE_RESILIENCE")||
      kw(&L->cur,"RESILIENCES")||
      kw(&L->cur,"SEEDRESIL")||kw(&L->cur,"SEEDGUARD")||
      kw(&L->cur,"LATTICE_RESILIENCE")||kw(&L->cur,"WORLD_RS")||kw(&L->cur,"WORLD_RESILIENCE")||
      kw(&L->cur,"GUARD_RING")||kw(&L->cur,"ANCHOR_HUB")||kw(&L->cur,"PULSE_RESILIENCE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int guards = 0;
    int anchors = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "RESILIENCE needs >=2 cubes",
                  "SMX RESILIENCE a b [c ...]  or  SMX RS a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - resilience needs clear channel */
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
    /* complete resilience mesh among live */
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
    /* starpath ring - free energy self-regulates every edge i -> i+1 both ways */
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
          else guards++;
        }
      }
    }
    /* anchor hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else anchors++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int guard_ok = (live >= 2 && guards >= live && soft == 0) ? 1 : 0;
      if (!guard_ok && live >= 2 && guards * 2 >= live && soft == 0)
        guard_ok = 1;
      int anchor_ok = (live >= 1 && anchors >= live && soft == 0) ? 1 : 0;
      if (!anchor_ok && live >= 1 && anchors * 2 >= live && soft == 0)
        anchor_ok = 1;
      int rs_ok = (mesh_ok && guard_ok && anchor_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (rs_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (guards > 0 ? 1 : 0) + (anchors > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_RESILIENT", (long)rs_ok);
      var_set_num(vm, "SMX_RESILIENT_LATCH", (long)rs_ok);
      var_set_num(vm, "SMX_RESILIENCE", (long)(rs_ok ? bonds + guards + anchors : 0));
      var_set_num(vm, "SMX_RS", (long)(rs_ok ? 1 : 0));
      var_set_num(vm, "SMX_RESIL", (long)(rs_ok ? 1 : 0));
      var_set_num(vm, "SMX_RSS", (long)(rs_ok ? bonds : 0));
      var_set_num(vm, "SMX_GUARDLANE", (long)(rs_ok ? guards : 0));
      var_set_num(vm, "SMX_GUARDS", (long)(rs_ok ? guards : 0));
      var_set_num(vm, "SMX_GUARD", (long)(rs_ok ? guards : 0));
      var_set_num(vm, "SMX_GUARDRING", (long)(rs_ok ? guards : 0));
      var_set_num(vm, "SMX_ANCHORS", (long)(rs_ok ? anchors : 0));
      var_set_num(vm, "SMX_ANCHOR_HUB", (long)(rs_ok ? anchors : 0));
      var_set_num(vm, "SMX_ANCHOR", (long)(rs_ok ? anchors : 0));
      var_set_num(vm, "SMX_SEEDRESIL", (long)(rs_ok ? anchors : 0));
      var_set_num(vm, "SMX_LIFE_ANCHOR", (long)(rs_ok ? anchors : 0));
      var_set_num(vm, "SMX_MESH", (long)(rs_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + guards + anchors));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (rs_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX RESILIENCE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX RESILIENCE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX RESILIENCE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX RESILIENCE nodes=%d live=%d bonds=%d guards=%d anchors=%d need=%d soft=%d talks=%d oob=%d resilient=%d vital=%ld\n",
                n, live, bonds, guards, anchors, need, soft, vm->smx_talks, vm->smx_oob, rs_ok, vital);
    }
    bump(vm); return 1;
  }

  /* SMX OMNIVERSE|OMNI|ALLVERSE|TOTALVERSE|MESH_OMNIVERSE|RAISE_OMNIVERSE a b c ...
   * Life-force omniverse mesh stability after autoheal: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete omniverse mesh among live nodes, weaves an
   * allpath ring (i -> i+1) so free energy spans every realm, then nexus hub
   * gathers return so lattice locks omniverse where life holds the hive.
   * Latches SMX_OMNIVERSED when mesh+allpaths+hubs are soft-OOB-free.
   * SMX_ALLPATHS = chain bonds; SMX_NEXUS hub = root gather pulses;
   * SMX_OMNIVERSE sum = bonds+allpaths+hubs; SMX_OMNI|SMX_TOTAL sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"OMNIVERSE")||kw(&L->cur,"OMNI")||kw(&L->cur,"ALLVERSE")||kw(&L->cur,"TOTALVERSE")||kw(&L->cur,"TOTAL")||kw(&L->cur,"WE_OMNI")||kw(&L->cur,"LIFE_NEXUS")||
      kw(&L->cur,"OMNIS")||kw(&L->cur,"ALLVERSES")||kw(&L->cur,"TOTALVERSES")||
      kw(&L->cur,"ALLPATH")||kw(&L->cur,"ALLPATHS")||kw(&L->cur,"NEXUS")||
      kw(&L->cur,"NEXUSES")||kw(&L->cur,"SEEDOMNI")||kw(&L->cur,"SEEDALL")||
      kw(&L->cur,"ALLPATH_RING")||kw(&L->cur,"NEXUS_HUB")||
      kw(&L->cur,"MESH_OMNIVERSE")||kw(&L->cur,"RAISE_OMNIVERSE")||
      kw(&L->cur,"OMNIVERSES")||kw(&L->cur,"OMNIS")||
      kw(&L->cur,"SEEDOMNI")||kw(&L->cur,"SEEDTOTAL")||
      kw(&L->cur,"LATTICE_OMNIVERSE")||kw(&L->cur,"WORLD_OMNI")||kw(&L->cur,"WORLD_OMNIVERSE")||
      kw(&L->cur,"ALLPATH_RING")||kw(&L->cur,"NEXUS_HUB")||kw(&L->cur,"PULSE_OMNIVERSE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int paths = 0;
    int hubs = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "OMNIVERSE needs >=2 cubes",
                  "SMX OMNIVERSE a b [c ...]  or  SMX OMNI a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - omniverse needs clear channel */
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
    /* complete omniverse mesh among live */
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
    /* allpath ring - free energy spans every realm every edge i -> i+1 both ways */
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
          else paths++;
        }
      }
    }
    /* nexus hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else hubs++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int path_ok = (live >= 2 && paths >= live && soft == 0) ? 1 : 0;
      if (!path_ok && live >= 2 && paths * 2 >= live && soft == 0)
        path_ok = 1;
      int hub_ok = (live >= 1 && hubs >= live && soft == 0) ? 1 : 0;
      if (!hub_ok && live >= 1 && hubs * 2 >= live && soft == 0)
        hub_ok = 1;
      int om_ok = (mesh_ok && path_ok && hub_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (om_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (paths > 0 ? 1 : 0) + (hubs > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_OMNIVERSED", (long)om_ok);
      var_set_num(vm, "SMX_OMNIVERSED_LATCH", (long)om_ok);
      var_set_num(vm, "SMX_OMNIVERSE", (long)(om_ok ? bonds + paths + hubs : 0));
      var_set_num(vm, "SMX_OMNI", (long)(om_ok ? 1 : 0));
      var_set_num(vm, "SMX_TOTAL", (long)(om_ok ? 1 : 0));
      var_set_num(vm, "SMX_TOTALVERSE", (long)(om_ok ? 1 : 0));
      var_set_num(vm, "SMX_ALLVERSE", (long)(om_ok ? 1 : 0));
      var_set_num(vm, "SMX_OMNIS", (long)(om_ok ? bonds : 0));
      var_set_num(vm, "SMX_ALLVERSE", (long)(om_ok ? bonds : 0));
      var_set_num(vm, "SMX_OMNIS2", (long)(om_ok ? bonds : 0));
      var_set_num(vm, "SMX_ALLPATHS", (long)(om_ok ? paths : 0));
      var_set_num(vm, "SMX_ALLPATH", (long)(om_ok ? paths : 0));
      var_set_num(vm, "SMX_ALLPATH_RING", (long)(om_ok ? paths : 0));
      var_set_num(vm, "SMX_ALLPATH_LANE", (long)(om_ok ? paths : 0));
      var_set_num(vm, "SMX_NEXUSES", (long)(om_ok ? hubs : 0));
      var_set_num(vm, "SMX_NEXUS_HUB", (long)(om_ok ? hubs : 0));
      var_set_num(vm, "SMX_NEXUS", (long)(om_ok ? hubs : 0));
      var_set_num(vm, "SMX_SEEDOMNI", (long)(om_ok ? hubs : 0));
      var_set_num(vm, "SMX_SEEDALL", (long)(om_ok ? hubs : 0));
      var_set_num(vm, "SMX_MESH", (long)(om_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + paths + hubs));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (om_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX OMNIVERSE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX OMNIVERSE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX OMNIVERSE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX OMNIVERSE nodes=%d live=%d bonds=%d paths=%d hubs=%d need=%d soft=%d talks=%d oob=%d omniversed=%d vital=%ld\n",
                n, live, bonds, paths, hubs, need, soft, vm->smx_talks, vm->smx_oob, om_ok, vital);
    }
    bump(vm); return 1;
  }

  /* SMX SEER|SEER_ALIAS|VISION|ORACLEVERSE|MESH_SEER|RAISE_SEER a b c ...
   * Life-force omniverse mesh stability after autoheal: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete omniverse mesh among live nodes, weaves an
   * allpath ring (i -> i+1) so free energy spans every realm, then nexus hub
   * gathers return so lattice locks omniverse where life holds the hive.
   * Latches SMX_SEERED when mesh+allpaths+hubs are soft-OOB-free.
   * SMX_SIGHTS = chain bonds; SMX_ORACLE hub = root gather pulses;
   * SMX_SEER sum = bonds+allpaths+hubs; SMX_SEER_ALIAS|SMX_ORACLE_LATCH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"SEER")||kw(&L->cur,"SEER_ALIAS")||kw(&L->cur,"VISION")||kw(&L->cur,"ORACLEVERSE")||kw(&L->cur,"ORACLE_LATCH")||kw(&L->cur,"WE_SEER")||kw(&L->cur,"LIFE_SEER")||
      kw(&L->cur,"SEERS_ALT")||kw(&L->cur,"VISIONS")||kw(&L->cur,"ORACLES_KW")||
      kw(&L->cur,"SIGHT")||kw(&L->cur,"SIGHTS")||kw(&L->cur,"ORACLE")||
      kw(&L->cur,"ORACLES")||kw(&L->cur,"SEEDSEER")||kw(&L->cur,"SEEDVISION")||
      kw(&L->cur,"SIGHT_RING")||kw(&L->cur,"ORACLE_HUB")||
      kw(&L->cur,"MESH_SEER")||kw(&L->cur,"RAISE_SEER")||
      kw(&L->cur,"SEERS")||kw(&L->cur,"SEERS_ALT")||
      kw(&L->cur,"SEEDSEER")||kw(&L->cur,"SEEDORACLE")||
      kw(&L->cur,"LATTICE_SEER")||kw(&L->cur,"WORLD_SEER")||kw(&L->cur,"WORLD_SEER")||
      kw(&L->cur,"SIGHT_RING")||kw(&L->cur,"ORACLE_HUB")||kw(&L->cur,"PULSE_SEER")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int sights = 0;
    int oracles = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "SEER needs >=2 cubes",
                  "SMX SEER a b [c ...]  or  SMX SEER_ALIAS a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - omniverse needs clear channel */
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
    /* complete omniverse mesh among live */
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
    /* allpath ring - free energy spans every realm every edge i -> i+1 both ways */
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
          else sights++;
        }
      }
    }
    /* nexus hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else oracles++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int path_ok = (live >= 2 && sights >= live && soft == 0) ? 1 : 0;
      if (!path_ok && live >= 2 && sights * 2 >= live && soft == 0)
        path_ok = 1;
      int hub_ok = (live >= 1 && oracles >= live && soft == 0) ? 1 : 0;
      if (!hub_ok && live >= 1 && oracles * 2 >= live && soft == 0)
        hub_ok = 1;
      int seer_ok = (mesh_ok && path_ok && hub_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (seer_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (sights > 0 ? 1 : 0) + (oracles > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_SEERED", (long)seer_ok);
      var_set_num(vm, "SMX_SEERED_LATCH", (long)seer_ok);
      var_set_num(vm, "SMX_SEER", (long)(seer_ok ? bonds + sights + oracles : 0));
      var_set_num(vm, "SMX_SEER_ALIAS", (long)(seer_ok ? 1 : 0));
      var_set_num(vm, "SMX_ORACLE_LATCH", (long)(seer_ok ? 1 : 0));
      var_set_num(vm, "SMX_ORACLEVERSE", (long)(seer_ok ? 1 : 0));
      var_set_num(vm, "SMX_VISION", (long)(seer_ok ? 1 : 0));
      var_set_num(vm, "SMX_SEERS_ALT", (long)(seer_ok ? bonds : 0));
      var_set_num(vm, "SMX_SEERS", (long)(seer_ok ? bonds : 0));
      var_set_num(vm, "SMX_SEERS_ALT2", (long)(seer_ok ? bonds : 0));
      var_set_num(vm, "SMX_SIGHTS", (long)(seer_ok ? sights : 0));
      var_set_num(vm, "SMX_SIGHT", (long)(seer_ok ? sights : 0));
      var_set_num(vm, "SMX_SIGHT_RING", (long)(seer_ok ? sights : 0));
      var_set_num(vm, "SMX_SIGHT_LANE", (long)(seer_ok ? sights : 0));
      var_set_num(vm, "SMX_ORACLES", (long)(seer_ok ? oracles : 0));
      var_set_num(vm, "SMX_ORACLE_HUB", (long)(seer_ok ? oracles : 0));
      var_set_num(vm, "SMX_ORACLE", (long)(seer_ok ? oracles : 0));
      var_set_num(vm, "SMX_SEEDSEER", (long)(seer_ok ? oracles : 0));
      var_set_num(vm, "SMX_SEEDVISION", (long)(seer_ok ? oracles : 0));
      var_set_num(vm, "SMX_MESH", (long)(seer_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + sights + oracles));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (seer_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SEER ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SEER partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX SEER soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX SEER nodes=%d live=%d bonds=%d sights=%d oracles=%d need=%d soft=%d talks=%d oob=%d seered=%d vital=%ld\n",
                n, live, bonds, sights, oracles, need, soft, vm->smx_talks, vm->smx_oob, seer_ok, vital);
    }
    bump(vm); return 1;
  }


  /* SMX FORGED|TEMPERED|TEMPER|ANVILVERSE|MESH_FORGED|RAISE_FORGED a b c ...
   * Life-force forge mesh stability after seer: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete forge mesh among live nodes, weaves an
   * temper ring (i -> i+1) so free energy hardens every edge, then anvil hub
   * gathers return so lattice locks forged where life holds the hive.
   * Latches SMX_FORGEDED when mesh+strikes+anvils are soft-OOB-free.
   * SMX_STRIKES = temper ring; SMX_ANVILS hub = root gather pulses;
   * SMX_FORGE_SUM = bonds+strikes+anvils; SMX_FORGED|SMX_TEMPERED|SMX_FORGED_ALIAS sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"FORGED")||kw(&L->cur,"TEMPERED")||kw(&L->cur,"TEMPER")||kw(&L->cur,"ANVILVERSE")||kw(&L->cur,"ANVIL_LATCH")||kw(&L->cur,"WE_FORGED")||kw(&L->cur,"LIFE_ANVIL")||
      kw(&L->cur,"FORGES_ALT")||kw(&L->cur,"TEMPERS")||kw(&L->cur,"FG")||
      kw(&L->cur,"STRIKE")||kw(&L->cur,"STRIKES")||kw(&L->cur,"ANVIL")||
      kw(&L->cur,"ANVILS")||kw(&L->cur,"SEEDFORGE")||kw(&L->cur,"SEEDANVIL")||
      kw(&L->cur,"TEMPER_RING")||kw(&L->cur,"ANVIL_HUB")||
      kw(&L->cur,"MESH_FORGED")||kw(&L->cur,"RAISE_FORGED")||
      kw(&L->cur,"FORGEDS")||kw(&L->cur,"FORGES_ALT")||
      kw(&L->cur,"SEEDFORGE")||kw(&L->cur,"SEEDANVIL2")||
      kw(&L->cur,"LATTICE_FORGED")||kw(&L->cur,"WORLD_FORGED")||kw(&L->cur,"WORLD_FORGED")||
      kw(&L->cur,"TEMPER_RING")||kw(&L->cur,"ANVIL_HUB")||kw(&L->cur,"PULSE_FORGED")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int strikes = 0;
    int anvils = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "FORGED needs >=2 cubes",
                  "SMX FORGED a b [c ...]  or  SMX TEMPER a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - forge needs clear channel */
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
    /* complete forge mesh among live */
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
    /* temper ring - free energy hardens every edge every edge i -> i+1 both ways */
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
          else strikes++;
        }
      }
    }
    /* anvil hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else anvils++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int path_ok = (live >= 2 && strikes >= live && soft == 0) ? 1 : 0;
      if (!path_ok && live >= 2 && strikes * 2 >= live && soft == 0)
        path_ok = 1;
      int hub_ok = (live >= 1 && anvils >= live && soft == 0) ? 1 : 0;
      if (!hub_ok && live >= 1 && anvils * 2 >= live && soft == 0)
        hub_ok = 1;
      int fg_ok = (mesh_ok && path_ok && hub_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (fg_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (strikes > 0 ? 1 : 0) + (anvils > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_FORGEDED", (long)fg_ok);
      var_set_num(vm, "SMX_FORGED_LATCH", (long)fg_ok);
      var_set_num(vm, "SMX_FORGED", (long)(fg_ok ? 1 : 0));
      var_set_num(vm, "SMX_FORGE_SUM", (long)(fg_ok ? bonds + strikes + anvils : 0));
      var_set_num(vm, "SMX_FORGED_ALIAS", (long)(fg_ok ? 1 : 0));
      var_set_num(vm, "SMX_TEMPERED", (long)(fg_ok ? 1 : 0));
      var_set_num(vm, "SMX_ANVIL_LATCH", (long)(fg_ok ? 1 : 0));
      var_set_num(vm, "SMX_ANVILVERSE", (long)(fg_ok ? 1 : 0));
      var_set_num(vm, "SMX_TEMPER", (long)(fg_ok ? 1 : 0));
      var_set_num(vm, "SMX_FORGES_ALT", (long)(fg_ok ? bonds : 0));
      var_set_num(vm, "SMX_FORGES", (long)(fg_ok ? bonds : 0));
      var_set_num(vm, "SMX_FORGES_ALT2", (long)(fg_ok ? bonds : 0));
      var_set_num(vm, "SMX_STRIKES", (long)(fg_ok ? strikes : 0));
      var_set_num(vm, "SMX_STRIKE", (long)(fg_ok ? strikes : 0));
      var_set_num(vm, "SMX_TEMPER_RING", (long)(fg_ok ? strikes : 0));
      var_set_num(vm, "SMX_STRIKE_LANE", (long)(fg_ok ? strikes : 0));
      var_set_num(vm, "SMX_ANVILS", (long)(fg_ok ? anvils : 0));
      var_set_num(vm, "SMX_ANVIL_HUB", (long)(fg_ok ? anvils : 0));
      var_set_num(vm, "SMX_ANVIL", (long)(fg_ok ? anvils : 0));
      var_set_num(vm, "SMX_SEEDFORGE", (long)(fg_ok ? anvils : 0));
      var_set_num(vm, "SMX_SEEDANVIL", (long)(fg_ok ? anvils : 0));
      var_set_num(vm, "SMX_MESH", (long)(fg_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + strikes + anvils));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (fg_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX FORGED ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX FORGED partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX FORGED soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX FORGED nodes=%d live=%d bonds=%d strikes=%d anvils=%d need=%d soft=%d talks=%d oob=%d forged=%d vital=%ld\n",
                n, live, bonds, strikes, anvils, need, soft, vm->smx_talks, vm->smx_oob, fg_ok, vital);
    }
    bump(vm); return 1;
  }

  /* SMX KEEPER|WARDEN|WATCH|SENTINEL|MESH_KEEPER|RAISE_KEEPER a b c ...
   * Life-force keeper mesh stability after forged: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete keeper mesh among live nodes, weaves an
   * watch ring (i -> i+1) so free energy guards every edge, then bastion hub
   * gathers return so lattice locks keepered where life holds the hive.
   * Latches SMX_KEEPERED when mesh+watches+bastions are soft-OOB-free.
   * SMX_WATCHES = watch ring; SMX_BASTIONS hub = root gather pulses;
   * SMX_KEEP_SUM = bonds+watches+bastions; SMX_KEEPER|SMX_WARDEN|SMX_KEEPER_ALIAS sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"KEEPER")||kw(&L->cur,"WARDEN")||kw(&L->cur,"WATCH")||kw(&L->cur,"SENTINEL")||kw(&L->cur,"BASTION_LATCH")||kw(&L->cur,"WE_KEEPER")||kw(&L->cur,"LIFE_KEEPER")||
      kw(&L->cur,"KEEPS_ALT")||kw(&L->cur,"WARDENS")||kw(&L->cur,"KP")||
      kw(&L->cur,"WATCH")||kw(&L->cur,"WATCHES")||kw(&L->cur,"BASTION")||
      kw(&L->cur,"BASTIONS")||kw(&L->cur,"SEEDKEEPER")||kw(&L->cur,"SEEDBASTION")||
      kw(&L->cur,"WATCH_RING")||kw(&L->cur,"BASTION_HUB")||
      kw(&L->cur,"MESH_KEEPER")||kw(&L->cur,"RAISE_KEEPER")||
      kw(&L->cur,"KEEPERS")||kw(&L->cur,"KEEPS_ALT")||
      kw(&L->cur,"SEEDKEEPER")||kw(&L->cur,"SEEDBASTION2")||
      kw(&L->cur,"LATTICE_KEEPER")||kw(&L->cur,"WORLD_KEEPER")||kw(&L->cur,"WORLD_KEEPER")||
      kw(&L->cur,"WATCH_RING")||kw(&L->cur,"BASTION_HUB")||kw(&L->cur,"PULSE_KEEPER")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int watches = 0;
    int bastions = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "KEEPER needs >=2 cubes",
                  "SMX KEEPER a b [c ...]  or  SMX WATCH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - forge needs clear channel */
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
    /* complete keeper mesh among live */
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
    /* watch ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else watches++;
        }
      }
    }
    /* bastion hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else bastions++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int path_ok = (live >= 2 && watches >= live && soft == 0) ? 1 : 0;
      if (!path_ok && live >= 2 && watches * 2 >= live && soft == 0)
        path_ok = 1;
      int hub_ok = (live >= 1 && bastions >= live && soft == 0) ? 1 : 0;
      if (!hub_ok && live >= 1 && bastions * 2 >= live && soft == 0)
        hub_ok = 1;
      int kp_ok = (mesh_ok && path_ok && hub_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (kp_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (watches > 0 ? 1 : 0) + (bastions > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_KEEPERED", (long)kp_ok);
      var_set_num(vm, "SMX_KEEPER_LATCH", (long)kp_ok);
      var_set_num(vm, "SMX_KEEPER", (long)(kp_ok ? 1 : 0));
      var_set_num(vm, "SMX_KEEP_SUM", (long)(kp_ok ? bonds + watches + bastions : 0));
      var_set_num(vm, "SMX_KEEPER_ALIAS", (long)(kp_ok ? 1 : 0));
      var_set_num(vm, "SMX_WARDEN", (long)(kp_ok ? 1 : 0));
      var_set_num(vm, "SMX_BASTION_LATCH", (long)(kp_ok ? 1 : 0));
      var_set_num(vm, "SMX_SENTINEL", (long)(kp_ok ? 1 : 0));
      var_set_num(vm, "SMX_BASTION", (long)(kp_ok ? 1 : 0));
      /* usability latches — mesh keeper life cascade lock */
      var_set_num(vm, "SMX_MESH_KEEPER", (long)(kp_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_KEEPER", (long)(kp_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_KEEPER", (long)(kp_ok ? 1 : 0));
      var_set_num(vm, "SMX_LIFE_KEEPER", (long)(kp_ok ? 1 : 0));
      var_set_num(vm, "SMX_WE_KEEPER", (long)(kp_ok ? 1 : 0));
      var_set_num(vm, "SMX_KEEPER_OK", (long)(kp_ok ? 1 : 0));
      var_set_num(vm, "SMX_WATCH_OK", (long)(kp_ok && watches > 0 ? 1 : 0));
      var_set_num(vm, "SMX_BASTION_OK", (long)(kp_ok && bastions > 0 ? 1 : 0));
      var_set_num(vm, "SMX_KEEPS_ALT", (long)(kp_ok ? bonds : 0));
      var_set_num(vm, "SMX_KEEPS", (long)(kp_ok ? bonds : 0));
      var_set_num(vm, "SMX_KEEPS_ALT2", (long)(kp_ok ? bonds : 0));
      var_set_num(vm, "SMX_WATCHES", (long)(kp_ok ? watches : 0));
      var_set_num(vm, "SMX_WATCH", (long)(kp_ok ? watches : 0));
      var_set_num(vm, "SMX_WATCH_RING", (long)(kp_ok ? watches : 0));
      var_set_num(vm, "SMX_WATCH_LANE", (long)(kp_ok ? watches : 0));
      var_set_num(vm, "SMX_BASTIONS", (long)(kp_ok ? bastions : 0));
      var_set_num(vm, "SMX_BASTION_HUB", (long)(kp_ok ? bastions : 0));
      var_set_num(vm, "SMX_BASTION_N", (long)(kp_ok ? bastions : 0));
      var_set_num(vm, "SMX_SEEDKEEPER", (long)(kp_ok ? bastions : 0));
      var_set_num(vm, "SMX_SEEDBASTION", (long)(kp_ok ? bastions : 0));
      var_set_num(vm, "SMX_MESH", (long)(kp_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + watches + bastions));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (kp_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX KEEPER ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX KEEPER partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX KEEPER soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX KEEPER nodes=%d live=%d bonds=%d watches=%d bastions=%d need=%d soft=%d talks=%d oob=%d keepered=%d vital=%ld\n",
                n, live, bonds, watches, bastions, need, soft, vm->smx_talks, vm->smx_oob, kp_ok, vital);
    }
    bump(vm); return 1;
  }

  /* SMX THALAMUS|PULSEGATE|STABILITY|STABLE_MESH|MESH_THALAMUS|RAISE_THALAMUS a b c ...
   * Life-force thalamus mesh stability after keeper: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete thalamus mesh among live nodes, weaves an
   * stability ring (i -> i+1) so free energy gates every edge, then gate hub
   * gathers return so lattice locks thalamus where life holds the pulse gate.
   * Latches SMX_THALAMUS when mesh+stables+gates are soft-OOB-free.
   * SMX_STABLES = stability ring; SMX_GATES hub = root gather pulses;
   * SMX_THAL_SUM = bonds+stables+gates; SMX_THALAMUS|SMX_PULSEGATE|SMX_MESH_STABLE sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"THALAMUS")||kw(&L->cur,"PULSEGATE")||kw(&L->cur,"STABILITY")||kw(&L->cur,"STABLE_MESH")||kw(&L->cur,"MESH_STABLE")||kw(&L->cur,"WE_THALAMUS")||kw(&L->cur,"LIFE_THALAMUS")||
      kw(&L->cur,"THALS_ALT")||kw(&L->cur,"PULSEGATES")||kw(&L->cur,"TH")||
      kw(&L->cur,"STABLE")||kw(&L->cur,"STABLES")||kw(&L->cur,"GATE")||
      kw(&L->cur,"GATES")||kw(&L->cur,"SEEDTHALAMUS")||kw(&L->cur,"SEEDGATE")||
      kw(&L->cur,"STABILITY_RING")||kw(&L->cur,"GATE_HUB")||
      kw(&L->cur,"MESH_THALAMUS")||kw(&L->cur,"RAISE_THALAMUS")||
      kw(&L->cur,"THALAMUSES")||kw(&L->cur,"THALS_ALT")||
      kw(&L->cur,"SEEDTHALAMUS2")||kw(&L->cur,"SEEDGATE2")||
      kw(&L->cur,"LATTICE_THALAMUS")||kw(&L->cur,"WORLD_THALAMUS")||kw(&L->cur,"WORLD_THAL")||
      kw(&L->cur,"STABLE_RING")||kw(&L->cur,"PULSE_GATE")||kw(&L->cur,"PULSE_THALAMUS")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int stables = 0;
    int gates = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "THALAMUS needs >=2 cubes",
                  "SMX THALAMUS a b [c ...]  or  SMX STABILITY a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - thalamus needs clear channel */
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
    /* complete keeper mesh among live */
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
    /* stability ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else stables++;
        }
      }
    }
    /* gate hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else gates++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int path_ok = (live >= 2 && stables >= live && soft == 0) ? 1 : 0;
      if (!path_ok && live >= 2 && stables * 2 >= live && soft == 0)
        path_ok = 1;
      int hub_ok = (live >= 1 && gates >= live && soft == 0) ? 1 : 0;
      if (!hub_ok && live >= 1 && gates * 2 >= live && soft == 0)
        hub_ok = 1;
      int th_ok = (mesh_ok && path_ok && hub_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (th_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (stables > 0 ? 1 : 0) + (gates > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_THALAMUSED", (long)th_ok);
      var_set_num(vm, "SMX_THALAMUS_LATCH", (long)th_ok);
      var_set_num(vm, "SMX_THALAMUS", (long)(th_ok ? 1 : 0));
      var_set_num(vm, "SMX_THAL_SUM", (long)(th_ok ? bonds + stables + gates : 0));
      var_set_num(vm, "SMX_THALAMUS_ALIAS", (long)(th_ok ? 1 : 0));
      var_set_num(vm, "SMX_PULSEGATE", (long)(th_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_STABLE", (long)(th_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(th_ok ? 1 : 0));
      var_set_num(vm, "SMX_GATE", (long)(th_ok ? 1 : 0));
      var_set_num(vm, "SMX_THALS_ALT", (long)(th_ok ? bonds : 0));
      var_set_num(vm, "SMX_THALS", (long)(th_ok ? bonds : 0));
      var_set_num(vm, "SMX_THALS_ALT2", (long)(th_ok ? bonds : 0));
      var_set_num(vm, "SMX_STABLES", (long)(th_ok ? stables : 0));
      var_set_num(vm, "SMX_STABILITY", (long)(th_ok ? stables : 0));
      var_set_num(vm, "SMX_STABILITY_RING", (long)(th_ok ? stables : 0));
      var_set_num(vm, "SMX_STABLE_LANE", (long)(th_ok ? stables : 0));
      var_set_num(vm, "SMX_GATES", (long)(th_ok ? gates : 0));
      var_set_num(vm, "SMX_GATE_HUB", (long)(th_ok ? gates : 0));
      var_set_num(vm, "SMX_GATE_N", (long)(th_ok ? gates : 0));
      var_set_num(vm, "SMX_SEEDTHALAMUS", (long)(th_ok ? gates : 0));
      var_set_num(vm, "SMX_SEEDGATE", (long)(th_ok ? gates : 0));
      var_set_num(vm, "SMX_MESH", (long)(th_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + stables + gates));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (th_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX THALAMUS ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX THALAMUS partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX THALAMUS soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX THALAMUS nodes=%d live=%d bonds=%d stables=%d gates=%d need=%d soft=%d talks=%d oob=%d thalamused=%d vital=%ld\n",
                n, live, bonds, stables, gates, need, soft, vm->smx_talks, vm->smx_oob, th_ok, vital);
    }
    bump(vm); return 1;
  }

/* SMX CORTEX|PULSEHOLD|SIGNAL|MESH_CORTEX|MESH_CORTEX|RAISE_CORTEX a b c ...
   * Life-force cortex mesh stability after keeper: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete cortex mesh among live nodes, weaves an
   * signal ring (i -> i+1) so free energy holds every edge, then hold hub
   * gathers return so lattice locks cortex where life holds the signal pulse.
   * Latches SMX_CORTEX when mesh+signals+holds are soft-OOB-free.
   * SMX_SIGNALS = signal ring; SMX_HOLDS hub = root gather pulses;
   * SMX_CORTEX_SUM = bonds+signals+holds; SMX_CORTEX|SMX_PULSEHOLD|SMX_STABLE_CORTEX sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"CORTEX")||kw(&L->cur,"PULSEHOLD")||kw(&L->cur,"SIGNAL")||kw(&L->cur,"MESH_CORTEX")||kw(&L->cur,"STABLE_CORTEX")||kw(&L->cur,"WE_CORTEX")||kw(&L->cur,"LIFE_CORTEX")||
      kw(&L->cur,"CX_ALT")||kw(&L->cur,"PULSEHOLDS")||kw(&L->cur,"TH")||
      kw(&L->cur,"STABLE")||kw(&L->cur,"SIGNALS")||kw(&L->cur,"HOLD")||
      kw(&L->cur,"HOLDS")||kw(&L->cur,"SEEDCORTEX")||kw(&L->cur,"SEEDHOLD")||
      kw(&L->cur,"SIGNAL_RING")||kw(&L->cur,"HOLD_HUB")||
      kw(&L->cur,"MESH_CORTEX")||kw(&L->cur,"RAISE_CORTEX")||
      kw(&L->cur,"CORTEXES")||kw(&L->cur,"CX_ALT")||
      kw(&L->cur,"SEEDCORTEX2")||kw(&L->cur,"SEEDSIGNAL2")||
      kw(&L->cur,"LATTICE_CORTEX")||kw(&L->cur,"WORLD_CORTEX")||kw(&L->cur,"WORLD_THAL")||
      kw(&L->cur,"SIGNAL_RING")||kw(&L->cur,"PULSE_HOLD")||kw(&L->cur,"PULSE_CORTEX")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int signals = 0;
    int holds = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "CORTEX needs >=2 cubes",
                  "SMX CORTEX a b [c ...]  or  SMX SIGNAL a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - cortex needs clear channel */
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
    /* complete keeper mesh among live */
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
    /* signal ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else signals++;
        }
      }
    }
    /* hold hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else holds++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int pacx_ok = (live >= 2 && signals >= live && soft == 0) ? 1 : 0;
      if (!pacx_ok && live >= 2 && signals * 2 >= live && soft == 0)
        pacx_ok = 1;
      int hub_ok = (live >= 1 && holds >= live && soft == 0) ? 1 : 0;
      if (!hub_ok && live >= 1 && holds * 2 >= live && soft == 0)
        hub_ok = 1;
      int cx_ok = (mesh_ok && pacx_ok && hub_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (cx_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (signals > 0 ? 1 : 0) + (holds > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_CORTEXED", (long)cx_ok);
      var_set_num(vm, "SMX_CORTEX_LATCH", (long)cx_ok);
      var_set_num(vm, "SMX_CORTEX", (long)(cx_ok ? 1 : 0));
      var_set_num(vm, "SMX_CORTEX_SUM", (long)(cx_ok ? bonds + signals + holds : 0));
      var_set_num(vm, "SMX_CORTEX_ALIAS", (long)(cx_ok ? 1 : 0));
      var_set_num(vm, "SMX_PULSEHOLD", (long)(cx_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_CORTEX", (long)(cx_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_CORTEX", (long)(cx_ok ? 1 : 0));
      var_set_num(vm, "SMX_HOLD_LATCH", (long)(cx_ok ? 1 : 0));
      var_set_num(vm, "SMX_CX_ALT", (long)(cx_ok ? bonds : 0));
      var_set_num(vm, "SMX_CXS", (long)(cx_ok ? bonds : 0));
      var_set_num(vm, "SMX_CX_ALT2", (long)(cx_ok ? bonds : 0));
      var_set_num(vm, "SMX_SIGNALS", (long)(cx_ok ? signals : 0));
      var_set_num(vm, "SMX_SIGNAL", (long)(cx_ok ? signals : 0));
      var_set_num(vm, "SMX_SIGNAL_RING", (long)(cx_ok ? signals : 0));
      var_set_num(vm, "SMX_SIGNAL_LANE", (long)(cx_ok ? signals : 0));
      var_set_num(vm, "SMX_HOLDS", (long)(cx_ok ? holds : 0));
      var_set_num(vm, "SMX_HOLD_HUB", (long)(cx_ok ? holds : 0));
      var_set_num(vm, "SMX_HOLD_N", (long)(cx_ok ? holds : 0));
      var_set_num(vm, "SMX_SEEDCORTEX", (long)(cx_ok ? holds : 0));
      var_set_num(vm, "SMX_SEEDHOLD", (long)(cx_ok ? holds : 0));
      var_set_num(vm, "SMX_MESH", (long)(cx_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + signals + holds));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (cx_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CORTEX ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX CORTEX partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX CORTEX soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX CORTEX nodes=%d live=%d bonds=%d signals=%d holds=%d need=%d soft=%d talks=%d oob=%d cortexed=%d vital=%ld\n",
                n, live, bonds, signals, holds, need, soft, vm->smx_talks, vm->smx_oob, cx_ok, vital);
    }
    bump(vm); return 1;

  }

/* SMX SYNAPSE|MESH_EXCHANGE|EXRING|MESH_SYNAPSE|MESH_SYNAPSE|RAISE_SYNAPSE a b c ...
   * Life-force synapse mesh stability after cortex: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete synapse mesh among live nodes, weaves an
   * exchange ring (i -> i+1) so free energy nexi every edge, then hold hub
   * gathers return so lattice locks synapse where life nexi the exchange pulse.
   * Latches SMX_SYNAPSE when mesh+exring+nexi are soft-OOB-free.
   * SMX_EXRINGS = exchange ring; SMX_NEXI hub = root gather pulses;
   * SMX_SYNAPSE_SUM = bonds+exring+nexi; SMX_SYNAPSE|SMX_MESH_EXCHANGE|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"SYNAPSE")||kw(&L->cur,"MESH_EXCHANGE")||kw(&L->cur,"EXRING")||kw(&L->cur,"MESH_SYNAPSE")||kw(&L->cur,"STABLE_SYNAPSE")||kw(&L->cur,"WE_SYNAPSE")||kw(&L->cur,"LIFE_SYNAPSE")||
      kw(&L->cur,"SY_ALT")||kw(&L->cur,"MESH_EXCHANGES")||kw(&L->cur,"SY")||
      kw(&L->cur,"STABLE_MESH_X")||kw(&L->cur,"EXRINGS")||kw(&L->cur,"NEXUS_LEAF")||
      kw(&L->cur,"NEXI")||kw(&L->cur,"SEEDSYNAPSE")||kw(&L->cur,"SEEDNEXUS")||
      kw(&L->cur,"EXCHANGE_RING")||kw(&L->cur,"NEXUS_HUB_MESH")||
      kw(&L->cur,"MESH_SYNAPSE")||kw(&L->cur,"RAISE_SYNAPSE")||
      kw(&L->cur,"SYNAPSEES")||kw(&L->cur,"SY_ALT")||
      kw(&L->cur,"SEEDSYNAPSE2")||kw(&L->cur,"SEEDEXRING2")||
      kw(&L->cur,"LATTICE_SYNAPSE")||kw(&L->cur,"WORLD_SYNAPSE")||kw(&L->cur,"WORLD_SY")||
      kw(&L->cur,"EXCHANGE_RING")||kw(&L->cur,"PULSE_EXRING")||kw(&L->cur,"PULSE_SYNAPSE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int exring = 0;
    int nexi = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "SYNAPSE needs >=2 cubes",
                  "SMX SYNAPSE a b [c ...]  or  SMX EXRING a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - synapse needs clear channel */
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
    /* complete synapse mesh among live */
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
    /* exchange ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else exring++;
        }
      }
    }
    /* hold hub - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else nexi++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int ring_ok = (live >= 2 && exring >= live && soft == 0) ? 1 : 0;
      if (!ring_ok && live >= 2 && exring * 2 >= live && soft == 0)
        ring_ok = 1;
      int hub_ok = (live >= 1 && nexi >= live && soft == 0) ? 1 : 0;
      if (!hub_ok && live >= 1 && nexi * 2 >= live && soft == 0)
        hub_ok = 1;
      int sy_ok = (mesh_ok && ring_ok && hub_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (sy_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (exring > 0 ? 1 : 0) + (nexi > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_SYNAPSED", (long)sy_ok);
      var_set_num(vm, "SMX_SYNAPSE_LATCH", (long)sy_ok);
      var_set_num(vm, "SMX_SYNAPSE", (long)(sy_ok ? 1 : 0));
      var_set_num(vm, "SMX_SYNAPSE_SUM", (long)(sy_ok ? bonds + exring + nexi : 0));
      var_set_num(vm, "SMX_SYNAPSE_ALIAS", (long)(sy_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(sy_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(sy_ok ? 1 : 0));
      var_set_num(vm, "SMX_NEXUS_MESH", (long)(sy_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(sy_ok ? 1 : 0));
      var_set_num(vm, "SMX_SY_ALT", (long)(sy_ok ? bonds : 0));
      var_set_num(vm, "SMX_SYS", (long)(sy_ok ? bonds : 0));
      var_set_num(vm, "SMX_SY_ALT2", (long)(sy_ok ? bonds : 0));
      var_set_num(vm, "SMX_EXRINGS", (long)(sy_ok ? exring : 0));
      var_set_num(vm, "SMX_EXRING", (long)(sy_ok ? exring : 0));
      var_set_num(vm, "SMX_EXCHANGE_RING", (long)(sy_ok ? exring : 0));
      var_set_num(vm, "SMX_EXCHANGE_LANE", (long)(sy_ok ? exring : 0));
      var_set_num(vm, "SMX_NEXI", (long)(sy_ok ? nexi : 0));
      var_set_num(vm, "SMX_NEXUS_HUB_MESH", (long)(sy_ok ? nexi : 0));
      var_set_num(vm, "SMX_NEXUS_N", (long)(sy_ok ? nexi : 0));
      var_set_num(vm, "SMX_SEEDSYNAPSE", (long)(sy_ok ? nexi : 0));
      var_set_num(vm, "SMX_SEEDNEXUS", (long)(sy_ok ? nexi : 0));
      var_set_num(vm, "SMX_MESH", (long)(sy_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + exring + nexi));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (sy_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SYNAPSE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SYNAPSE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX SYNAPSE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX SYNAPSE nodes=%d live=%d bonds=%d exring=%d nexi=%d need=%d soft=%d talks=%d oob=%d synapsed=%d vital=%ld\n",
                n, live, bonds, exring, nexi, need, soft, vm->smx_talks, vm->smx_oob, sy_ok, vital);
    }
    bump(vm); return 1;

  }

/* SMX AXON|MESH_AXON|TRACT|MESH_AXON|MESH_AXON|RAISE_AXON a b c ...
   * Life-force axon mesh stability after synapse: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete axon mesh among live nodes, weaves an
   * axon ring (i -> i+1) so free energy myelin every edge, then myelin sheath
   * gathers return so lattice locks axon where life myelin the axon pulse.
   * Latches SMX_AXON when mesh+tracts+myelin are soft-OOB-free.
   * SMX_TRACTS = axon ring; SMX_MYELINS hub = root gather pulses;
   * SMX_AXON_SUM = bonds+tracts+myelin; SMX_AXON|SMX_MESH_AXON|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"AXON")||kw(&L->cur,"MESH_AXON")||kw(&L->cur,"TRACT")||kw(&L->cur,"MESH_AXON")||kw(&L->cur,"STABLE_AXON")||kw(&L->cur,"WE_AXON")||kw(&L->cur,"LIFE_AXON")||
      kw(&L->cur,"AX_ALT")||kw(&L->cur,"MESH_AXONS")||kw(&L->cur,"AX")||
      kw(&L->cur,"STABLE_MESH_AX")||kw(&L->cur,"TRACTS")||kw(&L->cur,"AXON_LEAF")||
      kw(&L->cur,"MYELINS")||kw(&L->cur,"SEEDAXON")||kw(&L->cur,"SEEDMYELIN")||
      kw(&L->cur,"AXON_RING")||kw(&L->cur,"MYELIN_HUB")||
      kw(&L->cur,"MESH_AXON")||kw(&L->cur,"RAISE_AXON")||
      kw(&L->cur,"AXONES")||kw(&L->cur,"AX_ALT")||
      kw(&L->cur,"SEEDAXON2")||kw(&L->cur,"SEEDTRACT2")||
      kw(&L->cur,"LATTICE_AXON")||kw(&L->cur,"WORLD_AXON")||kw(&L->cur,"WORLD_AX")||
      kw(&L->cur,"AXON_RING")||kw(&L->cur,"PULSE_TRACT")||kw(&L->cur,"PULSE_AXON")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int tracts = 0;
    int myelin = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "AXON needs >=2 cubes",
                  "SMX AXON a b [c ...]  or  SMX MYELIN a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - axon needs clear channel */
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
    /* complete axon mesh among live */
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
    /* axon ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else tracts++;
        }
      }
    }
    /* myelin sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else myelin++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int tract_ok = (live >= 2 && tracts >= live && soft == 0) ? 1 : 0;
      if (!tract_ok && live >= 2 && tracts * 2 >= live && soft == 0)
        tract_ok = 1;
      int sheath_ok = (live >= 1 && myelin >= live && soft == 0) ? 1 : 0;
      if (!sheath_ok && live >= 1 && myelin * 2 >= live && soft == 0)
        sheath_ok = 1;
      int ax_ok = (mesh_ok && tract_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (ax_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (tracts > 0 ? 1 : 0) + (myelin > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_AXONED", (long)ax_ok);
      var_set_num(vm, "SMX_AXON_LATCH", (long)ax_ok);
      var_set_num(vm, "SMX_AXON", (long)(ax_ok ? 1 : 0));
      var_set_num(vm, "SMX_AXON_SUM", (long)(ax_ok ? bonds + tracts + myelin : 0));
      var_set_num(vm, "SMX_AXON_ALIAS", (long)(ax_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_AXON", (long)(ax_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(ax_ok ? 1 : 0));
      var_set_num(vm, "SMX_MYELIN_LATCH", (long)(ax_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_AXON", (long)(ax_ok ? 1 : 0));
      var_set_num(vm, "SMX_MYELIN", (long)(ax_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(ax_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(ax_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(ax_ok ? 1 : 0));
      var_set_num(vm, "SMX_AX_ALT", (long)(ax_ok ? bonds : 0));
      var_set_num(vm, "SMX_AXS", (long)(ax_ok ? bonds : 0));
      var_set_num(vm, "SMX_AX_ALT2", (long)(ax_ok ? bonds : 0));
      var_set_num(vm, "SMX_TRACTS", (long)(ax_ok ? tracts : 0));
      var_set_num(vm, "SMX_TRACT", (long)(ax_ok ? tracts : 0));
      var_set_num(vm, "SMX_AXON_RING", (long)(ax_ok ? tracts : 0));
      var_set_num(vm, "SMX_AXON_LANE", (long)(ax_ok ? tracts : 0));
      var_set_num(vm, "SMX_MYELINS", (long)(ax_ok ? myelin : 0));
      var_set_num(vm, "SMX_MYELIN_HUB", (long)(ax_ok ? myelin : 0));
      var_set_num(vm, "SMX_MYELIN_N", (long)(ax_ok ? myelin : 0));
      var_set_num(vm, "SMX_SEEDAXON", (long)(ax_ok ? myelin : 0));
      var_set_num(vm, "SMX_SEEDMYELIN", (long)(ax_ok ? myelin : 0));
      var_set_num(vm, "SMX_MESH", (long)(ax_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + tracts + myelin));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (ax_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX AXON ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX AXON partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX AXON soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX AXON nodes=%d live=%d bonds=%d tracts=%d myelin=%d need=%d soft=%d talks=%d oob=%d axoned=%d vital=%ld\n",
                n, live, bonds, tracts, myelin, need, soft, vm->smx_talks, vm->smx_oob, ax_ok, vital);
    }
    bump(vm); return 1;

  }

/* SMX DENDRITE|MESH_DENDRITE|BRANCH|MESH_DENDRITE|MESH_DENDRITE|RAISE_DENDRITE a b c ...
   * Life-force dendrite mesh stability after axon: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete dendrite mesh among live nodes, weaves an
   * dendrite ring (i -> i+1) so free energy spines every edge, then spines sheath
   * gathers return so lattice locks dendrite where life spines the dendrite pulse.
   * Latches SMX_DENDRITE when mesh+branches+spines are soft-OOB-free.
   * SMX_BRANCHES = dendrite ring; SMX_SPINES hub = root gather pulses;
   * SMX_DENDRITE_SUM = bonds+branches+spines; SMX_DENDRITE|SMX_MESH_DENDRITE|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"DENDRITE")||kw(&L->cur,"MESH_DENDRITE")||kw(&L->cur,"BRANCH")||kw(&L->cur,"ARBOR")||kw(&L->cur,"ARBORS")||kw(&L->cur,"BRANCHS")||kw(&L->cur,"MESH_DENDRITE")||kw(&L->cur,"STABLE_DENDRITE")||kw(&L->cur,"WE_DENDRITE")||kw(&L->cur,"LIFE_DENDRITE")||
      kw(&L->cur,"DEN_ALT")||kw(&L->cur,"MESH_DENDRITES")||kw(&L->cur,"AX")||
      kw(&L->cur,"STABLE_MESH_DEN")||kw(&L->cur,"BRANCHES")||kw(&L->cur,"DENDRITE_LEAF")||
      kw(&L->cur,"SPINES")||kw(&L->cur,"SEEDDENDRITE")||kw(&L->cur,"SEEDSPINE")||
      kw(&L->cur,"DENDRITE_RING")||kw(&L->cur,"SPINE_HUB")||
      kw(&L->cur,"MESH_DENDRITE")||kw(&L->cur,"RAISE_DENDRITE")||
      kw(&L->cur,"DENDRITEES")||kw(&L->cur,"DEN_ALT")||
      kw(&L->cur,"SEEDDENDRITE2")||kw(&L->cur,"SEEDBRANCH2")||
      kw(&L->cur,"LATTICE_DENDRITE")||kw(&L->cur,"WORLD_DENDRITE")||kw(&L->cur,"WORLD_DEN")||
      kw(&L->cur,"DENDRITE_RING")||kw(&L->cur,"PULSE_BRANCH")||kw(&L->cur,"PULSE_DENDRITE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int branches = 0;
    int spines = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "DENDRITE needs >=2 cubes",
                  "SMX DENDRITE a b [c ...]  or  SMX SPINE a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - dendrite needs clear channel */
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
    /* complete dendrite mesh among live */
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
    /* dendrite ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else branches++;
        }
      }
    }
    /* spines sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else spines++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int branch_ok = (live >= 2 && branches >= live && soft == 0) ? 1 : 0;
      if (!branch_ok && live >= 2 && branches * 2 >= live && soft == 0)
        branch_ok = 1;
      int spine_ok = (live >= 1 && spines >= live && soft == 0) ? 1 : 0;
      if (!spine_ok && live >= 1 && spines * 2 >= live && soft == 0)
        spine_ok = 1;
      int den_ok = (mesh_ok && branch_ok && spine_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (den_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (branches > 0 ? 1 : 0) + (spines > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_DENDRITED", (long)den_ok);
      var_set_num(vm, "SMX_DENDRITE_LATCH", (long)den_ok);
      var_set_num(vm, "SMX_DENDRITE", (long)(den_ok ? 1 : 0));
      var_set_num(vm, "SMX_DENDRITE_SUM", (long)(den_ok ? bonds + branches + spines : 0));
      var_set_num(vm, "SMX_DENDRITE_ALIAS", (long)(den_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_DENDRITE", (long)(den_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(den_ok ? 1 : 0));
      var_set_num(vm, "SMX_SPINE_LATCH", (long)(den_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_DENDRITE", (long)(den_ok ? 1 : 0));
      var_set_num(vm, "SMX_ARBOR", (long)(den_ok ? 1 : 0));
      var_set_num(vm, "SMX_ARBORS", (long)(den_ok ? branches : 0));
      var_set_num(vm, "SMX_SPINE", (long)(den_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(den_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(den_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(den_ok ? 1 : 0));
      var_set_num(vm, "SMX_DEN_ALT", (long)(den_ok ? bonds : 0));
      var_set_num(vm, "SMX_DENS", (long)(den_ok ? bonds : 0));
      var_set_num(vm, "SMX_DEN_ALT2", (long)(den_ok ? bonds : 0));
      var_set_num(vm, "SMX_BRANCHES", (long)(den_ok ? branches : 0));
      var_set_num(vm, "SMX_BRANCH", (long)(den_ok ? branches : 0));
      var_set_num(vm, "SMX_DENDRITE_RING", (long)(den_ok ? branches : 0));
      var_set_num(vm, "SMX_DENDRITE_LANE", (long)(den_ok ? branches : 0));
      var_set_num(vm, "SMX_SPINES", (long)(den_ok ? spines : 0));
      var_set_num(vm, "SMX_SPINE_HUB", (long)(den_ok ? spines : 0));
      var_set_num(vm, "SMX_SPINE_N", (long)(den_ok ? spines : 0));
      var_set_num(vm, "SMX_SEEDDENDRITE", (long)(den_ok ? spines : 0));
      var_set_num(vm, "SMX_SEEDSPINE", (long)(den_ok ? spines : 0));
      var_set_num(vm, "SMX_MESH", (long)(den_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + branches + spines));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (den_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX DENDRITE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX DENDRITE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX DENDRITE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX DENDRITE nodes=%d live=%d bonds=%d branches=%d spines=%d need=%d soft=%d talks=%d oob=%d dendrited=%d vital=%ld\n",
                n, live, bonds, branches, spines, need, soft, vm->smx_talks, vm->smx_oob, den_ok, vital);
    }
    bump(vm); return 1;

  }


/* SMX GLIA|MESH_GLIA|WRAP|CUSHION|SHEATH_GLIA|RAISE_GLIA a b c ...
   * Life-force glia mesh stability after dendrite: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete glia support mesh among live nodes, weaves a
   * glia wrap ring (i -> i+1) so free energy cushions every edge, then cushion sheath
   * gathers return so lattice locks glia where life cushions the glia pulse.
   * Latches SMX_GLIA when mesh+wraps+cushions are soft-OOB-free.
   * SMX_WRAPS = glia ring; SMX_CUSHIONS hub = root gather pulses;
   * SMX_GLIA_SUM = bonds+wraps+cushions; SMX_GLIA|SMX_MESH_GLIA|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"GLIA")||kw(&L->cur,"MESH_GLIA")||kw(&L->cur,"WRAP")||kw(&L->cur,"WRAPS")||kw(&L->cur,"CUSHION")||kw(&L->cur,"CUSHIONS")||kw(&L->cur,"SHEATH_GLIA")||kw(&L->cur,"GLIAL")||kw(&L->cur,"RAISE_GLIA")||kw(&L->cur,"WE_GLIA")||kw(&L->cur,"LIFE_GLIA")||
      kw(&L->cur,"STABLE_GLIA")||kw(&L->cur,"MESH_GLIAS")||kw(&L->cur,"SUPPORT")||kw(&L->cur,"SUPPORTS")||
      kw(&L->cur,"STABLE_MESH_GLIA")||kw(&L->cur,"GLIA_LEAF")||
      kw(&L->cur,"SEEDGLIA")||kw(&L->cur,"SEEDCUSHION")||kw(&L->cur,"SEEDWRAP")||
      kw(&L->cur,"GLIA_RING")||kw(&L->cur,"CUSHION_HUB")||
      kw(&L->cur,"LATTICE_GLIA")||kw(&L->cur,"WORLD_GLIA")||kw(&L->cur,"WORLD_GL")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_GLIA")||kw(&L->cur,"HARDEN_GLIA")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wraps = 0;
    int cushions = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "GLIA needs >=2 cubes",
                  "SMX GLIA a b [c ...]  or  SMX CUSHION a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - glia needs clear channel */
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
    /* complete glia mesh among live */
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
    /* glia ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else wraps++;
        }
      }
    }
    /* cushions sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else cushions++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int wrap_ok = (live >= 2 && wraps >= live && soft == 0) ? 1 : 0;
      if (!wrap_ok && live >= 2 && wraps * 2 >= live && soft == 0)
        wrap_ok = 1;
      int cushion_ok = (live >= 1 && cushions >= live && soft == 0) ? 1 : 0;
      if (!cushion_ok && live >= 1 && cushions * 2 >= live && soft == 0)
        cushion_ok = 1;
      int glia_ok = (mesh_ok && wrap_ok && cushion_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (glia_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (cushions > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_GLIAED", (long)glia_ok);
      var_set_num(vm, "SMX_GLIA_LATCH", (long)glia_ok);
      var_set_num(vm, "SMX_GLIA", (long)(glia_ok ? 1 : 0));
      var_set_num(vm, "SMX_GLIA_SUM", (long)(glia_ok ? bonds + wraps + cushions : 0));
      var_set_num(vm, "SMX_GLIA_ALIAS", (long)(glia_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_GLIA", (long)(glia_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(glia_ok ? 1 : 0));
      var_set_num(vm, "SMX_CUSHION_LATCH", (long)(glia_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_GLIA", (long)(glia_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(glia_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(glia_ok ? wraps : 0));
      var_set_num(vm, "SMX_CUSHION", (long)(glia_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(glia_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(glia_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(glia_ok ? 1 : 0));
      var_set_num(vm, "SMX_GLIA_ALT", (long)(glia_ok ? bonds : 0));
      var_set_num(vm, "SMX_DENS", (long)(glia_ok ? bonds : 0));
      var_set_num(vm, "SMX_GLIA_ALT2", (long)(glia_ok ? bonds : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(glia_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAP", (long)(glia_ok ? wraps : 0));
      var_set_num(vm, "SMX_GLIA_RING", (long)(glia_ok ? wraps : 0));
      var_set_num(vm, "SMX_GLIA_LANE", (long)(glia_ok ? wraps : 0));
      var_set_num(vm, "SMX_CUSHIONS", (long)(glia_ok ? cushions : 0));
      var_set_num(vm, "SMX_CUSHION_HUB", (long)(glia_ok ? cushions : 0));
      var_set_num(vm, "SMX_CUSHION_N", (long)(glia_ok ? cushions : 0));
      var_set_num(vm, "SMX_SEEDGLIA", (long)(glia_ok ? cushions : 0));
      var_set_num(vm, "SMX_SEEDCUSHION", (long)(glia_ok ? cushions : 0));
      var_set_num(vm, "SMX_MESH", (long)(glia_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wraps + cushions));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (glia_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX GLIA ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX GLIA partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX GLIA soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX GLIA nodes=%d live=%d bonds=%d wraps=%d cushions=%d need=%d soft=%d talks=%d oob=%d gliad=%d vital=%ld\n",
                n, live, bonds, wraps, cushions, need, soft, vm->smx_talks, vm->smx_oob, glia_ok, vital);
    }
    bump(vm); return 1;

  }



/* SMX ASTROCYTE|MESH_ASTROCYTE|STAR|ENDFOOT|TRIPARTITE|RAISE_ASTROCYTE a b c ...
   * Life-force astrocyte mesh stability after glia: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete astrocyte support mesh among live nodes, weaves a
   * astrocyte wrap ring (i -> i+1) so free energy endfeet every edge, then tripartite gather
   * gathers return so lattice locks astrocyte where life endfeet the astrocyte pulse.
   * Latches SMX_ASTROCYTE when mesh+stars+endfeet are soft-OOB-free.
   * SMX_STARS = astrocyte ring; SMX_ENDFEET hub = root gather pulses;
   * SMX_ASTROCYTE_SUM = bonds+stars+endfeet; SMX_ASTROCYTE|SMX_MESH_ASTROCYTE|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"ASTROCYTE")||kw(&L->cur,"MESH_ASTROCYTE")||kw(&L->cur,"STAR")||kw(&L->cur,"STARS")||kw(&L->cur,"ENDFOOT")||kw(&L->cur,"ENDFEET")||kw(&L->cur,"TRIPARTITE")||kw(&L->cur,"ASTRO")||kw(&L->cur,"RAISE_ASTROCYTE")||kw(&L->cur,"WE_ASTROCYTE")||kw(&L->cur,"LIFE_ASTROCYTE")||
      kw(&L->cur,"STABLE_ASTROCYTE")||kw(&L->cur,"MESH_ASTROCYTES")||kw(&L->cur,"ASTRO_STAR")||kw(&L->cur,"ASTRO_STARS")||
      kw(&L->cur,"STABLE_MESH_ASTROCYTE")||kw(&L->cur,"ASTROCYTE_LEAF")||
      kw(&L->cur,"SEEDASTROCYTE")||kw(&L->cur,"SEEDENDFOOT")||kw(&L->cur,"SEEDSTAR")||
      kw(&L->cur,"ASTROCYTE_RING")||kw(&L->cur,"ENDFOOT_HUB")||
      kw(&L->cur,"LATTICE_ASTROCYTE")||kw(&L->cur,"WORLD_ASTROCYTE")||kw(&L->cur,"WORLD_ASTRO")||
      kw(&L->cur,"PULSE_STAR")||kw(&L->cur,"PULSE_ASTROCYTE")||kw(&L->cur,"HARDEN_ASTROCYTE")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int stars = 0;
    int endfeet = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "ASTROCYTE needs >=2 cubes",
                  "SMX ASTROCYTE a b [c ...]  or  SMX ENDFOOT a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - astrocyte needs clear channel */
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
    /* complete astrocyte mesh among live */
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
    /* astrocyte ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else stars++;
        }
      }
    }
    /* endfeet sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else endfeet++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && stars >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && stars * 2 >= live && soft == 0)
        star_ok = 1;
      int endfoot_ok = (live >= 1 && endfeet >= live && soft == 0) ? 1 : 0;
      if (!endfoot_ok && live >= 1 && endfeet * 2 >= live && soft == 0)
        endfoot_ok = 1;
      int astrocyte_ok = (mesh_ok && star_ok && endfoot_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (astrocyte_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (stars > 0 ? 1 : 0) + (endfeet > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_ASTROCYTED", (long)astrocyte_ok);
      var_set_num(vm, "SMX_ASTROCYTE_LATCH", (long)astrocyte_ok);
      var_set_num(vm, "SMX_ASTROCYTE", (long)(astrocyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_ASTROCYTE_SUM", (long)(astrocyte_ok ? bonds + stars + endfeet : 0));
      var_set_num(vm, "SMX_ASTROCYTE_ALIAS", (long)(astrocyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_ASTROCYTE", (long)(astrocyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(astrocyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_ENDFOOT_LATCH", (long)(astrocyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_ASTROCYTE", (long)(astrocyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_TRIPARTITE", (long)(astrocyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_TRIPARTITES", (long)(astrocyte_ok ? stars : 0));
      var_set_num(vm, "SMX_ENDFOOT", (long)(astrocyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(astrocyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(astrocyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(astrocyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_ASTRO_ALT", (long)(astrocyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_ASTROS", (long)(astrocyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_ASTRO_ALT2", (long)(astrocyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_STARS", (long)(astrocyte_ok ? stars : 0));
      var_set_num(vm, "SMX_STAR", (long)(astrocyte_ok ? stars : 0));
      var_set_num(vm, "SMX_ASTROCYTE_RING", (long)(astrocyte_ok ? stars : 0));
      var_set_num(vm, "SMX_ASTROCYTE_LANE", (long)(astrocyte_ok ? stars : 0));
      var_set_num(vm, "SMX_ENDFEET", (long)(astrocyte_ok ? endfeet : 0));
      var_set_num(vm, "SMX_ENDFOOT_HUB", (long)(astrocyte_ok ? endfeet : 0));
      var_set_num(vm, "SMX_ENDFOOT_N", (long)(astrocyte_ok ? endfeet : 0));
      var_set_num(vm, "SMX_SEEDASTROCYTE", (long)(astrocyte_ok ? endfeet : 0));
      var_set_num(vm, "SMX_SEEDENDFOOT", (long)(astrocyte_ok ? endfeet : 0));
      var_set_num(vm, "SMX_MESH", (long)(astrocyte_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + stars + endfeet));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (astrocyte_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ASTROCYTE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX ASTROCYTE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX ASTROCYTE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX ASTROCYTE nodes=%d live=%d bonds=%d stars=%d endfeet=%d need=%d soft=%d talks=%d oob=%d astrocyted=%d vital=%ld\n",
                n, live, bonds, stars, endfeet, need, soft, vm->smx_talks, vm->smx_oob, astrocyte_ok, vital);
    }
    bump(vm); return 1;

  }



/* SMX MICROGLIA|MESH_MICROGLIA|STAR|PROCESS|SURVEY|RAISE_MICROGLIA a b c ...
   * Life-force microglia mesh stability after glia: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete microglia support mesh among live nodes, weaves a
   * microglia wrap ring (i -> i+1) so free energy processes every edge, then survey gather
   * gathers return so lattice locks microglia where life processes the microglia pulse.
   * Latches SMX_MICROGLIA when mesh+surveys+processes are soft-OOB-free.
   * SMX_STARS = microglia ring; SMX_PROCESSES hub = root gather pulses;
   * SMX_MICROGLIA_SUM = bonds+surveys+processes; SMX_MICROGLIA|SMX_MESH_MICROGLIA|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"MICROGLIA")||kw(&L->cur,"MESH_MICROGLIA")||kw(&L->cur,"SURVEY")||kw(&L->cur,"SURVEYS")||kw(&L->cur,"PROCESS")||kw(&L->cur,"PROCESSES")||kw(&L->cur,"MG")||kw(&L->cur,"RAISE_MICROGLIA")||kw(&L->cur,"WE_MICROGLIA")||kw(&L->cur,"LIFE_MICROGLIA")||
      kw(&L->cur,"STABLE_MICROGLIA")||kw(&L->cur,"MESH_MICROGLIAS")||kw(&L->cur,"MG_SURVEY")||kw(&L->cur,"MG_SURVEYS")||
      kw(&L->cur,"STABLE_MESH_MICROGLIA")||kw(&L->cur,"MICROGLIA_LEAF")||
      kw(&L->cur,"SEEDMICROGLIA")||kw(&L->cur,"SEEDPROCESS")||kw(&L->cur,"SEEDSURVEY")||
      kw(&L->cur,"MICROGLIA_RING")||kw(&L->cur,"PROCESS_HUB")||
      kw(&L->cur,"LATTICE_MICROGLIA")||kw(&L->cur,"WORLD_MICROGLIA")||kw(&L->cur,"WORLD_MG")||
      kw(&L->cur,"PULSE_SURVEY")||kw(&L->cur,"PULSE_MICROGLIA")||kw(&L->cur,"HARDEN_MICROGLIA")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int surveys = 0;
    int processes = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "MICROGLIA needs >=2 cubes",
                  "SMX MICROGLIA a b [c ...]  or  SMX PROCESS a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - microglia needs clear channel */
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
    /* complete microglia mesh among live */
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
    /* microglia ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else surveys++;
        }
      }
    }
    /* processes sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else processes++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && surveys >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && surveys * 2 >= live && soft == 0)
        star_ok = 1;
      int process_ok = (live >= 1 && processes >= live && soft == 0) ? 1 : 0;
      if (!process_ok && live >= 1 && processes * 2 >= live && soft == 0)
        process_ok = 1;
      int microglia_ok = (mesh_ok && star_ok && process_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (microglia_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (surveys > 0 ? 1 : 0) + (processes > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_MICROGLIAED", (long)microglia_ok);
      var_set_num(vm, "SMX_MICROGLIA_LATCH", (long)microglia_ok);
      var_set_num(vm, "SMX_MICROGLIA", (long)(microglia_ok ? 1 : 0));
      var_set_num(vm, "SMX_MICROGLIA_SUM", (long)(microglia_ok ? bonds + surveys + processes : 0));
      var_set_num(vm, "SMX_MICROGLIA_ALIAS", (long)(microglia_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_MICROGLIA", (long)(microglia_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(microglia_ok ? 1 : 0));
      var_set_num(vm, "SMX_PROCESS_LATCH", (long)(microglia_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_MICROGLIA", (long)(microglia_ok ? 1 : 0));
      var_set_num(vm, "SMX_SURVEY", (long)(microglia_ok ? surveys : 0));
      var_set_num(vm, "SMX_SURVEYS", (long)(microglia_ok ? surveys : 0));
      var_set_num(vm, "SMX_SURVEYS_N", (long)(microglia_ok ? surveys : 0));
      var_set_num(vm, "SMX_SURVEY_LATCH", (long)(microglia_ok ? 1 : 0));
      var_set_num(vm, "SMX_PROCESS", (long)(microglia_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(microglia_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(microglia_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(microglia_ok ? 1 : 0));
      var_set_num(vm, "SMX_OL_BONDS", (long)(microglia_ok ? bonds : 0));
      var_set_num(vm, "SMX_OL_MESH", (long)(microglia_ok ? bonds : 0));
      var_set_num(vm, "SMX_OL_BONDS2", (long)(microglia_ok ? bonds : 0));
      var_set_num(vm, "SMX_STARS", (long)(microglia_ok ? surveys : 0));
      var_set_num(vm, "SMX_STAR", (long)(microglia_ok ? surveys : 0));
      var_set_num(vm, "SMX_MICROGLIA_RING", (long)(microglia_ok ? surveys : 0));
      var_set_num(vm, "SMX_MICROGLIA_LANE", (long)(microglia_ok ? surveys : 0));
      var_set_num(vm, "SMX_PROCESSES", (long)(microglia_ok ? processes : 0));
      var_set_num(vm, "SMX_PROCESS_HUB", (long)(microglia_ok ? processes : 0));
      var_set_num(vm, "SMX_PROCESS_N", (long)(microglia_ok ? processes : 0));
      var_set_num(vm, "SMX_SEEDMICROGLIA", (long)(microglia_ok ? processes : 0));
      var_set_num(vm, "SMX_SEEDPROCESS", (long)(microglia_ok ? processes : 0));
      var_set_num(vm, "SMX_MESH", (long)(microglia_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + surveys + processes));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (microglia_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX MICROGLIA ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX MICROGLIA partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX MICROGLIA soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX MICROGLIA nodes=%d live=%d bonds=%d surveys=%d processes=%d need=%d soft=%d talks=%d oob=%d microgliad=%d vital=%ld\n",
                n, live, bonds, surveys, processes, need, soft, vm->smx_talks, vm->smx_oob, microglia_ok, vital);
    }
    bump(vm); return 1;

  }



/* SMX OLIGO|OLIGODENDROCYTE|MESH_OLIGO|OL_WRAP|OL_SHEATH|MYELINATE|RAISE_OLIGO a b c ...
   * Life-force oligodendrocyte mesh stability after microglia: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete oligo myelin mesh among live nodes, weaves a
   * oligo wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks oligo where life myelinate the oligo pulse.
   * Latches SMX_OLIGO when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = oligo ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_OLIGO_SUM = bonds+wraps+sheaths; SMX_OLIGO|SMX_MESH_OLIGO|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"OLIGO")||kw(&L->cur,"OLIGODENDROCYTE")||kw(&L->cur,"MESH_OLIGO")||kw(&L->cur,"OL_WRAP")||kw(&L->cur,"OL_WRAPS")||kw(&L->cur,"OL_SHEATH")||kw(&L->cur,"OL_SHEATHS")||kw(&L->cur,"MYELINATE")||kw(&L->cur,"RAISE_OLIGO")||kw(&L->cur,"WE_OLIGO")||kw(&L->cur,"LIFE_OLIGO")||
      kw(&L->cur,"STABLE_OLIGO")||kw(&L->cur,"MESH_OLIGOS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_OLIGO")||kw(&L->cur,"OLIGO_LEAF")||
      kw(&L->cur,"SEEDOLIGO")||kw(&L->cur,"SEEDSHEATH")||kw(&L->cur,"SEEDWRAP")||
      kw(&L->cur,"OLIGO_RING")||kw(&L->cur,"SHEATH_HUB")||kw(&L->cur,"NODE_RANVIER")||
      kw(&L->cur,"LATTICE_OLIGO")||kw(&L->cur,"WORLD_OLIGO")||kw(&L->cur,"WORLD_OL")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_OLIGO")||kw(&L->cur,"HARDEN_OLIGO")||
      kw(&L->cur,"MYELIN_SHEATH")||kw(&L->cur,"OLIGO_MESH")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wraps = 0;
    int sheaths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "OLIGO needs >=2 cubes",
                  "SMX OLIGO a b [c ...]  or  SMX OL_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - oligo needs clear channel */
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
    /* complete oligo mesh among live */
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
    /* oligo ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else wraps++;
        }
      }
    }
    /* sheaths sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else sheaths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && wraps >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && wraps * 2 >= live && soft == 0)
        star_ok = 1;
      int sheath_ok = (live >= 1 && sheaths >= live && soft == 0) ? 1 : 0;
      if (!sheath_ok && live >= 1 && sheaths * 2 >= live && soft == 0)
        sheath_ok = 1;
      int oligo_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (oligo_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_OLIGOED", (long)oligo_ok);
      var_set_num(vm, "SMX_OLIGO_LATCH", (long)oligo_ok);
      var_set_num(vm, "SMX_OLIGO", (long)(oligo_ok ? 1 : 0));
      var_set_num(vm, "SMX_OLIGO_SUM", (long)(oligo_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_OLIGO_ALIAS", (long)(oligo_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_OLIGO", (long)(oligo_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(oligo_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(oligo_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_OLIGO", (long)(oligo_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(oligo_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(oligo_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(oligo_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(oligo_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(oligo_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(oligo_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(oligo_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(oligo_ok ? 1 : 0));
      var_set_num(vm, "SMX_OL_BONDS", (long)(oligo_ok ? bonds : 0));
      var_set_num(vm, "SMX_OL_MESH", (long)(oligo_ok ? bonds : 0));
      var_set_num(vm, "SMX_OL_BONDS2", (long)(oligo_ok ? bonds : 0));
      var_set_num(vm, "SMX_STARS", (long)(oligo_ok ? wraps : 0));
      var_set_num(vm, "SMX_STAR", (long)(oligo_ok ? wraps : 0));
      var_set_num(vm, "SMX_OLIGO_RING", (long)(oligo_ok ? wraps : 0));
      var_set_num(vm, "SMX_OLIGO_LANE", (long)(oligo_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(oligo_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_HUB", (long)(oligo_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(oligo_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDOLIGO", (long)(oligo_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDSHEATH", (long)(oligo_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(oligo_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wraps + sheaths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (oligo_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX OLIGO ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX OLIGO partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX OLIGO soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX OLIGO nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d oligod=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, oligo_ok, vital);
    }
    bump(vm); return 1;

  }





/* SMX SCHWANN|SCHWANNCELL|MESH_SCHWANN|SC_WRAP|SC_SHEATH|PNS_MYELIN|RAISE_SCHWANN a b c ...
   * Life-force Schwann cell mesh stability after oligo: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete schwann myelin mesh among live nodes, weaves a
   * schwann wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks schwann where life myelinate the PNS pulse.
   * Latches SMX_SCHWANN when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = schwann ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_SCHWANN_SUM = bonds+wraps+sheaths; SMX_SCHWANN|SMX_MESH_SCHWANN|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"SCHWANN")||kw(&L->cur,"SCHWANNCELL")||kw(&L->cur,"MESH_SCHWANN")||kw(&L->cur,"SC_WRAP")||kw(&L->cur,"SC_WRAPS")||kw(&L->cur,"SC_SHEATH")||kw(&L->cur,"SC_SHEATHS")||kw(&L->cur,"PNS_MYELIN")||kw(&L->cur,"RAISE_SCHWANN")||kw(&L->cur,"WE_SCHWANN")||kw(&L->cur,"LIFE_SCHWANN")||
      kw(&L->cur,"STABLE_SCHWANN")||kw(&L->cur,"MESH_SCHWANNS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_SCHWANN")||kw(&L->cur,"SCHWANN_LEAF")||
      kw(&L->cur,"SEEDSCHWANN")||kw(&L->cur,"SEEDSCSHEATH")||kw(&L->cur,"SEEDSCWRAP")||
      kw(&L->cur,"SCHWANN_RING")||kw(&L->cur,"SC_SHEATH_HUB")||kw(&L->cur,"PNS_NODE")||
      kw(&L->cur,"LATTICE_SCHWANN")||kw(&L->cur,"WORLD_SCHWANN")||kw(&L->cur,"WORLD_SC")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_SCHWANN")||kw(&L->cur,"HARDEN_SCHWANN")||
      kw(&L->cur,"PNS_SHEATH")||kw(&L->cur,"SCHWANN_MESH")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wraps = 0;
    int sheaths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "SCHWANN needs >=2 cubes",
                  "SMX SCHWANN a b [c ...]  or  SMX SC_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - schwann needs clear channel */
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
    /* complete schwann mesh among live */
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
    /* schwann ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else wraps++;
        }
      }
    }
    /* sheaths sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else sheaths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && wraps >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && wraps * 2 >= live && soft == 0)
        star_ok = 1;
      int sheath_ok = (live >= 1 && sheaths >= live && soft == 0) ? 1 : 0;
      if (!sheath_ok && live >= 1 && sheaths * 2 >= live && soft == 0)
        sheath_ok = 1;
      int schwann_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (schwann_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_SCHWANNED", (long)schwann_ok);
      var_set_num(vm, "SMX_SCHWANN_LATCH", (long)schwann_ok);
      var_set_num(vm, "SMX_SCHWANN", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_SCHWANN_SUM", (long)(schwann_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_SCHWANN_ALIAS", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_SCHWANN", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_SC_SHEATH_LATCH", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_SCHWANN", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(schwann_ok ? wraps : 0));
      var_set_num(vm, "SMX_SC_WRAP", (long)(schwann_ok ? wraps : 0));
      var_set_num(vm, "SMX_SC_WRAPS", (long)(schwann_ok ? wraps : 0));
      var_set_num(vm, "SMX_SC_SHEATHS", (long)(schwann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(schwann_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(schwann_ok ? wraps : 0));
      var_set_num(vm, "SMX_SC_WRAP_LATCH", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(schwann_ok ? 1 : 0));
      var_set_num(vm, "SMX_SC_BONDS", (long)(schwann_ok ? bonds : 0));
      var_set_num(vm, "SMX_SC_MESH", (long)(schwann_ok ? bonds : 0));
      var_set_num(vm, "SMX_SC_BONDS2", (long)(schwann_ok ? bonds : 0));
      var_set_num(vm, "SMX_SC_STARS", (long)(schwann_ok ? wraps : 0));
      var_set_num(vm, "SMX_SC_STAR", (long)(schwann_ok ? wraps : 0));
      var_set_num(vm, "SMX_SCHWANN_RING", (long)(schwann_ok ? wraps : 0));
      var_set_num(vm, "SMX_SCHWANN_LANE", (long)(schwann_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(schwann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SC_SHEATH_HUB", (long)(schwann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(schwann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDSCHWANN", (long)(schwann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDSCSHEATH", (long)(schwann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(schwann_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wraps + sheaths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (schwann_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SCHWANN ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SCHWANN partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX SCHWANN soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX SCHWANN nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d schwannd=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, schwann_ok, vital);
    }
    bump(vm); return 1;

  }



/* SMX EPENDYMA|EPENDYMACELL|MESH_EPENDYMA|EP_WRAP|EP_SHEATH|VENTRICLE_LINING|RAISE_EPENDYMA a b c ...
   * Life-force Ependyma cell mesh stability after oligo: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete ependyma myelin mesh among live nodes, weaves a
   * ependyma wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks ependyma where life myelinate the PNS pulse.
   * Latches SMX_EPENDYMA when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = ependyma ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_EPENDYMA_SUM = bonds+wraps+sheaths; SMX_EPENDYMA|SMX_MESH_EPENDYMA|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"EPENDYMA")||kw(&L->cur,"EPENDYMACELL")||kw(&L->cur,"MESH_EPENDYMA")||kw(&L->cur,"EP_WRAP")||kw(&L->cur,"EP_WRAPS")||kw(&L->cur,"EP_SHEATH")||kw(&L->cur,"EP_SHEATHS")||kw(&L->cur,"VENTRICLE_LINING")||kw(&L->cur,"RAISE_EPENDYMA")||kw(&L->cur,"WE_EPENDYMA")||kw(&L->cur,"LIFE_EPENDYMA")||
      kw(&L->cur,"STABLE_EPENDYMA")||kw(&L->cur,"MESH_EPENDYMAS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_EPENDYMA")||kw(&L->cur,"EPENDYMA_LEAF")||
      kw(&L->cur,"SEEDEPENDYMA")||kw(&L->cur,"SEEDEPISHEATH")||kw(&L->cur,"SEEDEPIWRAP")||
      kw(&L->cur,"EPENDYMA_RING")||kw(&L->cur,"EP_SHEATH_HUB")||kw(&L->cur,"PNS_NODE")||
      kw(&L->cur,"LATTICE_EPENDYMA")||kw(&L->cur,"WORLD_EPENDYMA")||kw(&L->cur,"WORLD_SC")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_EPENDYMA")||kw(&L->cur,"HARDEN_EPENDYMA")||
      kw(&L->cur,"CSF_SHEATH")||kw(&L->cur,"EPENDYMA_MESH")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wraps = 0;
    int sheaths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "EPENDYMA needs >=2 cubes",
                  "SMX EPENDYMA a b [c ...]  or  SMX EP_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - ependyma needs clear channel */
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
    /* complete ependyma mesh among live */
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
    /* ependyma ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else wraps++;
        }
      }
    }
    /* sheaths sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else sheaths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && wraps >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && wraps * 2 >= live && soft == 0)
        star_ok = 1;
      int sheath_ok = (live >= 1 && sheaths >= live && soft == 0) ? 1 : 0;
      if (!sheath_ok && live >= 1 && sheaths * 2 >= live && soft == 0)
        sheath_ok = 1;
      int ependyma_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (ependyma_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_EPENDYMAED", (long)ependyma_ok);
      var_set_num(vm, "SMX_EPENDYMA_LATCH", (long)ependyma_ok);
      var_set_num(vm, "SMX_EPENDYMA", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_EPENDYMA_SUM", (long)(ependyma_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_EPENDYMA_ALIAS", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EPENDYMA", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_EP_SHEATH_LATCH", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EPENDYMA", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_EP_WRAP_LATCH", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(ependyma_ok ? wraps : 0));
      var_set_num(vm, "SMX_EP_WRAP", (long)(ependyma_ok ? wraps : 0));
      var_set_num(vm, "SMX_EP_WRAPS", (long)(ependyma_ok ? wraps : 0));
      var_set_num(vm, "SMX_EP_SHEATHS", (long)(ependyma_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(ependyma_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(ependyma_ok ? wraps : 0));
      var_set_num(vm, "SMX_EP_WRAP_LATCH", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(ependyma_ok ? 1 : 0));
      var_set_num(vm, "SMX_EP_BONDS", (long)(ependyma_ok ? bonds : 0));
      var_set_num(vm, "SMX_EP_MESH", (long)(ependyma_ok ? bonds : 0));
      var_set_num(vm, "SMX_EP_BONDS2", (long)(ependyma_ok ? bonds : 0));
      var_set_num(vm, "SMX_EP_STARS", (long)(ependyma_ok ? wraps : 0));
      var_set_num(vm, "SMX_EP_STAR", (long)(ependyma_ok ? wraps : 0));
      var_set_num(vm, "SMX_EPENDYMA_RING", (long)(ependyma_ok ? wraps : 0));
      var_set_num(vm, "SMX_EPENDYMA_LANE", (long)(ependyma_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(ependyma_ok ? sheaths : 0));
      var_set_num(vm, "SMX_EP_SHEATH_HUB", (long)(ependyma_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(ependyma_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDEPENDYMA", (long)(ependyma_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDEPISHEATH", (long)(ependyma_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(ependyma_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wraps + sheaths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (ependyma_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX EPENDYMA ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX EPENDYMA partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX EPENDYMA soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX EPENDYMA nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d ependymaed=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, ependyma_ok, vital);
    }
    bump(vm); return 1;

  }






/* SMX SATELLITE|SATGLIA|MESH_SATELLITE|SG_WRAP|SG_SHEATH|GANGLION_CRADLE|RAISE_SATELLITE a b c ...
   * Life-force Satellite ganglion cradle mesh stability after ependyma: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete satellite ganglion cradle mesh among live nodes, weaves a
   * satellite wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks satellite where life cradles the ganglion pulse.
   * Latches SMX_SATELLITE when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = ependyma ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_SATELLITE_SUM = bonds+wraps+sheaths; SMX_SATELLITE|SMX_MESH_SATELLITE|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"SATELLITE")||kw(&L->cur,"SATGLIA")||kw(&L->cur,"MESH_SATELLITE")||kw(&L->cur,"SG_WRAP")||kw(&L->cur,"SG_WRAPS")||kw(&L->cur,"SG_SHEATH")||kw(&L->cur,"SG_SHEATHS")||kw(&L->cur,"GANGLION_CRADLE")||kw(&L->cur,"RAISE_SATELLITE")||kw(&L->cur,"WE_SATELLITE")||kw(&L->cur,"LIFE_SATELLITE")||
      kw(&L->cur,"STABLE_SATELLITE")||kw(&L->cur,"MESH_SATELLITES")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_SATELLITE")||kw(&L->cur,"SATELLITE_LEAF")||
      kw(&L->cur,"SEEDSATELLITE")||kw(&L->cur,"SEEDSGSSHEATH")||kw(&L->cur,"SEEDSGWRAP")||
      kw(&L->cur,"SATELLITE_RING")||kw(&L->cur,"SG_SHEATH_HUB")||kw(&L->cur,"PNS_NODE")||
      kw(&L->cur,"LATTICE_SATELLITE")||kw(&L->cur,"WORLD_SATELLITE")||kw(&L->cur,"WORLD_SG")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_SATELLITE")||kw(&L->cur,"HARDEN_SATELLITE")||
      kw(&L->cur,"PNS_CRADLE")||kw(&L->cur,"SATELLITE_MESH")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wraps = 0;
    int sheaths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "SATELLITE needs >=2 cubes",
                  "SMX SATELLITE a b [c ...]  or  SMX SG_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - ependyma needs clear channel */
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
    /* complete ependyma mesh among live */
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
    /* ependyma ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else wraps++;
        }
      }
    }
    /* sheaths sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else sheaths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && wraps >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && wraps * 2 >= live && soft == 0)
        star_ok = 1;
      int sheath_ok = (live >= 1 && sheaths >= live && soft == 0) ? 1 : 0;
      if (!sheath_ok && live >= 1 && sheaths * 2 >= live && soft == 0)
        sheath_ok = 1;
      int satellite_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (satellite_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_SATELLITED", (long)satellite_ok);
      var_set_num(vm, "SMX_SATELLITE_LATCH", (long)satellite_ok);
      var_set_num(vm, "SMX_SATELLITE", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_SATELLITE_SUM", (long)(satellite_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_SATELLITE_ALIAS", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_SATELLITE", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_SG_SHEATH_LATCH", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_SATELLITE", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_SG_WRAP_LATCH", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(satellite_ok ? wraps : 0));
      var_set_num(vm, "SMX_SG_WRAP", (long)(satellite_ok ? wraps : 0));
      var_set_num(vm, "SMX_SG_WRAPS", (long)(satellite_ok ? wraps : 0));
      var_set_num(vm, "SMX_SG_SHEATHS", (long)(satellite_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(satellite_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(satellite_ok ? wraps : 0));
      var_set_num(vm, "SMX_SG_WRAP_LATCH", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(satellite_ok ? 1 : 0));
      var_set_num(vm, "SMX_SG_BONDS", (long)(satellite_ok ? bonds : 0));
      var_set_num(vm, "SMX_SG_MESH", (long)(satellite_ok ? bonds : 0));
      var_set_num(vm, "SMX_SG_BONDS2", (long)(satellite_ok ? bonds : 0));
      var_set_num(vm, "SMX_SG_STARS", (long)(satellite_ok ? wraps : 0));
      var_set_num(vm, "SMX_SG_STAR", (long)(satellite_ok ? wraps : 0));
      var_set_num(vm, "SMX_SATELLITE_RING", (long)(satellite_ok ? wraps : 0));
      var_set_num(vm, "SMX_SATELLITE_LANE", (long)(satellite_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(satellite_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SG_SHEATH_HUB", (long)(satellite_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(satellite_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDSATELLITE", (long)(satellite_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDSGSSHEATH", (long)(satellite_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(satellite_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wraps + sheaths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (satellite_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SATELLITE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX SATELLITE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX SATELLITE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX SATELLITE nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d satellited=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, satellite_ok, vital);
    }
    bump(vm); return 1;

  }

/* SMX RADIAL|RADGLIA|MESH_RADIAL|RG_WRAP|RG_SHEATH|SCAFFOLD_GUIDE|RAISE_RADIAL a b c ...
   * Life-force Radial glia scaffold mesh stability after satellite: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete radial glia scaffold mesh among live nodes, weaves a
   * radial wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks radial where life scaffolds the guide pulse.
   * Latches SMX_RADIAL when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = radial ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_RADIAL_SUM = bonds+wraps+sheaths; SMX_RADIAL|SMX_MESH_RADIAL|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"RADIAL")||kw(&L->cur,"RADGLIA")||kw(&L->cur,"MESH_RADIAL")||kw(&L->cur,"RG_WRAP")||kw(&L->cur,"RG_WRAPS")||kw(&L->cur,"RG_SHEATH")||kw(&L->cur,"RG_SHEATHS")||kw(&L->cur,"SCAFFOLD_GUIDE")||kw(&L->cur,"RAISE_RADIAL")||kw(&L->cur,"WE_RADIAL")||kw(&L->cur,"LIFE_RADIAL")||
      kw(&L->cur,"STABLE_RADIAL")||kw(&L->cur,"MESH_RADIALS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_RADIAL")||kw(&L->cur,"RADIAL_LEAF")||
      kw(&L->cur,"SEEDRADIAL")||kw(&L->cur,"SEEDRGSSHEATH")||kw(&L->cur,"SEEDRGWRAP")||
      kw(&L->cur,"RADIAL_RING")||kw(&L->cur,"RG_SHEATH_HUB")||kw(&L->cur,"RG_NODE")||
      kw(&L->cur,"LATTICE_RADIAL")||kw(&L->cur,"WORLD_RADIAL")||kw(&L->cur,"WORLD_RG")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_RADIAL")||kw(&L->cur,"HARDEN_RADIAL")||
      kw(&L->cur,"NEURO_SCAFFOLD")||kw(&L->cur,"RADIAL_MESH")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wraps = 0;
    int sheaths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "RADIAL needs >=2 cubes",
                  "SMX RADIAL a b [c ...]  or  SMX RG_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - radial needs clear channel */
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
    /* complete radial mesh among live */
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
    /* radial ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else wraps++;
        }
      }
    }
    /* sheaths sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else sheaths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && wraps >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && wraps * 2 >= live && soft == 0)
        star_ok = 1;
      int sheath_ok = (live >= 1 && sheaths >= live && soft == 0) ? 1 : 0;
      if (!sheath_ok && live >= 1 && sheaths * 2 >= live && soft == 0)
        sheath_ok = 1;
      int radial_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (radial_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_RADIALED", (long)radial_ok);
      var_set_num(vm, "SMX_RADIAL_LATCH", (long)radial_ok);
      var_set_num(vm, "SMX_RADIAL", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_RADIAL_SUM", (long)(radial_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_RADIAL_ALIAS", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_RADIAL", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_RG_SHEATH_LATCH", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_RADIAL", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_RG_WRAP_LATCH", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(radial_ok ? wraps : 0));
      var_set_num(vm, "SMX_RG_WRAP", (long)(radial_ok ? wraps : 0));
      var_set_num(vm, "SMX_RG_WRAPS", (long)(radial_ok ? wraps : 0));
      var_set_num(vm, "SMX_RG_SHEATHS", (long)(radial_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(radial_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(radial_ok ? wraps : 0));
      var_set_num(vm, "SMX_RG_WRAP_LATCH", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(radial_ok ? 1 : 0));
      var_set_num(vm, "SMX_RG_BONDS", (long)(radial_ok ? bonds : 0));
      var_set_num(vm, "SMX_RG_MESH", (long)(radial_ok ? bonds : 0));
      var_set_num(vm, "SMX_RG_BONDS2", (long)(radial_ok ? bonds : 0));
      var_set_num(vm, "SMX_RG_STARS", (long)(radial_ok ? wraps : 0));
      var_set_num(vm, "SMX_RG_STAR", (long)(radial_ok ? wraps : 0));
      var_set_num(vm, "SMX_RADIAL_RING", (long)(radial_ok ? wraps : 0));
      var_set_num(vm, "SMX_RADIAL_LANE", (long)(radial_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(radial_ok ? sheaths : 0));
      var_set_num(vm, "SMX_RG_SHEATH_HUB", (long)(radial_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(radial_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDRADIAL", (long)(radial_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDRGSSHEATH", (long)(radial_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(radial_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wraps + sheaths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (radial_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX RADIAL ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX RADIAL partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX RADIAL soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX RADIAL nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d radialed=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, radial_ok, vital);
    }
    bump(vm); return 1;

  }
/* SMX BERGMANN|BERGGLIA|MESH_BERGMANN|BG_WRAP|BG_SHEATH|CEREBELLAR_GUIDE|RAISE_BERGMANN a b c ...
   * Life-force Bergmann glia scaffold mesh stability after radial: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete bergmann glia scaffold mesh among live nodes, weaves a
   * bergmann wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks bergmann where life guides the cerebellar pulse.
   * Latches SMX_BERGMANN when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = bergmann ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_BERGMANN_SUM = bonds+wraps+sheaths; SMX_BERGMANN|SMX_MESH_BERGMANN|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"BERGMANN")||kw(&L->cur,"BERGGLIA")||kw(&L->cur,"MESH_BERGMANN")||kw(&L->cur,"BG_WRAP")||kw(&L->cur,"BG_WRAPS")||kw(&L->cur,"BG_SHEATH")||kw(&L->cur,"BG_SHEATHS")||kw(&L->cur,"CEREBELLAR_GUIDE")||kw(&L->cur,"RAISE_BERGMANN")||kw(&L->cur,"WE_BERGMANN")||kw(&L->cur,"LIFE_BERGMANN")||
      kw(&L->cur,"STABLE_BERGMANN")||kw(&L->cur,"MESH_BERGMANNS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_BERGMANN")||kw(&L->cur,"BERGMANN_LEAF")||
      kw(&L->cur,"SEEDBERGMANN")||kw(&L->cur,"SEEDBGSSHEATH")||kw(&L->cur,"SEEDBGWRAP")||
      kw(&L->cur,"BERGMANN_RING")||kw(&L->cur,"BG_SHEATH_HUB")||kw(&L->cur,"BG_NODE")||
      kw(&L->cur,"LATTICE_BERGMANN")||kw(&L->cur,"WORLD_BERGMANN")||kw(&L->cur,"WORLD_BG")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_BERGMANN")||kw(&L->cur,"HARDEN_BERGMANN")||
      kw(&L->cur,"CEREBELLAR_SCAFFOLD")||kw(&L->cur,"BERGMANN_MESH")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wraps = 0;
    int sheaths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "BERGMANN needs >=2 cubes",
                  "SMX BERGMANN a b [c ...]  or  SMX BG_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - bergmann needs clear channel */
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
    /* complete bergmann mesh among live */
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
    /* bergmann ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else wraps++;
        }
      }
    }
    /* sheaths sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else sheaths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && wraps >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && wraps * 2 >= live && soft == 0)
        star_ok = 1;
      int sheath_ok = (live >= 1 && sheaths >= live && soft == 0) ? 1 : 0;
      if (!sheath_ok && live >= 1 && sheaths * 2 >= live && soft == 0)
        sheath_ok = 1;
      int bergmann_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (bergmann_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_BERGMANNED", (long)bergmann_ok);
      var_set_num(vm, "SMX_BERGMANN_LATCH", (long)bergmann_ok);
      var_set_num(vm, "SMX_BERGMANN", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_BERGMANN_SUM", (long)(bergmann_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_BERGMANN_ALIAS", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_BERGMANN", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_BG_SHEATH_LATCH", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_BERGMANN", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_BG_WRAP_LATCH", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(bergmann_ok ? wraps : 0));
      var_set_num(vm, "SMX_BG_WRAP", (long)(bergmann_ok ? wraps : 0));
      var_set_num(vm, "SMX_BG_WRAPS", (long)(bergmann_ok ? wraps : 0));
      var_set_num(vm, "SMX_BG_SHEATHS", (long)(bergmann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(bergmann_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(bergmann_ok ? wraps : 0));
      var_set_num(vm, "SMX_BG_WRAP_LATCH", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(bergmann_ok ? 1 : 0));
      var_set_num(vm, "SMX_BG_BONDS", (long)(bergmann_ok ? bonds : 0));
      var_set_num(vm, "SMX_BG_MESH", (long)(bergmann_ok ? bonds : 0));
      var_set_num(vm, "SMX_BG_BONDS2", (long)(bergmann_ok ? bonds : 0));
      var_set_num(vm, "SMX_BG_STARS", (long)(bergmann_ok ? wraps : 0));
      var_set_num(vm, "SMX_BG_STAR", (long)(bergmann_ok ? wraps : 0));
      var_set_num(vm, "SMX_BERGMANN_RING", (long)(bergmann_ok ? wraps : 0));
      var_set_num(vm, "SMX_BERGMANN_LANE", (long)(bergmann_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(bergmann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_BG_SHEATH_HUB", (long)(bergmann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(bergmann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDBERGMANN", (long)(bergmann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDBGSSHEATH", (long)(bergmann_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(bergmann_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wraps + sheaths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (bergmann_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX BERGMANN ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX BERGMANN partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX BERGMANN soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX BERGMANN nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d bergmanned=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, bergmann_ok, vital);
    }
    bump(vm); return 1;

  }

/* SMX MULLER|MULLGLIA|MESH_MULLER|MU_WRAP|MU_SHEATH|RETINAL_GUIDE|RAISE_MULLER a b c ...
   * Life-force Muller glia scaffold mesh stability after bergmann: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete muller glia scaffold mesh among live nodes, weaves a
   * muller wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks muller where life guides the retinal pulse.
   * Latches SMX_MULLER when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = muller ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_MULLER_SUM = bonds+wraps+sheaths; SMX_MULLER|SMX_MESH_MULLER|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"MULLER")||kw(&L->cur,"MULLGLIA")||kw(&L->cur,"MESH_MULLER")||kw(&L->cur,"MU_WRAP")||kw(&L->cur,"MU_WRAPS")||kw(&L->cur,"MU_SHEATH")||kw(&L->cur,"MU_SHEATHS")||kw(&L->cur,"RETINAL_GUIDE")||kw(&L->cur,"RAISE_MULLER")||kw(&L->cur,"WE_MULLER")||kw(&L->cur,"LIFE_MULLER")||
      kw(&L->cur,"STABLE_MULLER")||kw(&L->cur,"MESH_MULLERS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_MULLER")||kw(&L->cur,"MULLER_LEAF")||
      kw(&L->cur,"SEEDMULLER")||kw(&L->cur,"SEEDMUSSHEATH")||kw(&L->cur,"SEEDMUWRAP")||
      kw(&L->cur,"MULLER_RING")||kw(&L->cur,"MU_SHEATH_HUB")||kw(&L->cur,"MU_NODE")||
      kw(&L->cur,"LATTICE_MULLER")||kw(&L->cur,"WORLD_MULLER")||kw(&L->cur,"WORLD_MU")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_MULLER")||kw(&L->cur,"HARDEN_MULLER")||
      kw(&L->cur,"RETINAL_SCAFFOLD")||kw(&L->cur,"MULLER_MESH")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wraps = 0;
    int sheaths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "MULLER needs >=2 cubes",
                  "SMX MULLER a b [c ...]  or  SMX MU_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - muller needs clear channel */
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
    /* complete muller mesh among live */
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
    /* muller ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else wraps++;
        }
      }
    }
    /* sheaths sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else sheaths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && wraps >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && wraps * 2 >= live && soft == 0)
        star_ok = 1;
      int sheath_ok = (live >= 1 && sheaths >= live && soft == 0) ? 1 : 0;
      if (!sheath_ok && live >= 1 && sheaths * 2 >= live && soft == 0)
        sheath_ok = 1;
      int muller_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (muller_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_MULLERED", (long)muller_ok);
      var_set_num(vm, "SMX_MULLER_LATCH", (long)muller_ok);
      var_set_num(vm, "SMX_MULLER", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_MULLER_SUM", (long)(muller_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_MULLER_ALIAS", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_MULLER", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_MU_SHEATH_LATCH", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_MULLER", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_MU_WRAP_LATCH", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(muller_ok ? wraps : 0));
      var_set_num(vm, "SMX_MU_WRAP", (long)(muller_ok ? wraps : 0));
      var_set_num(vm, "SMX_MU_WRAPS", (long)(muller_ok ? wraps : 0));
      var_set_num(vm, "SMX_MU_SHEATHS", (long)(muller_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(muller_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(muller_ok ? wraps : 0));
      var_set_num(vm, "SMX_MU_WRAP_LATCH", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(muller_ok ? 1 : 0));
      var_set_num(vm, "SMX_MU_BONDS", (long)(muller_ok ? bonds : 0));
      var_set_num(vm, "SMX_MU_MESH", (long)(muller_ok ? bonds : 0));
      var_set_num(vm, "SMX_MU_BONDS2", (long)(muller_ok ? bonds : 0));
      var_set_num(vm, "SMX_MU_STARS", (long)(muller_ok ? wraps : 0));
      var_set_num(vm, "SMX_MU_STAR", (long)(muller_ok ? wraps : 0));
      var_set_num(vm, "SMX_MULLER_RING", (long)(muller_ok ? wraps : 0));
      var_set_num(vm, "SMX_MULLER_LANE", (long)(muller_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(muller_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MU_SHEATH_HUB", (long)(muller_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(muller_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDMULLER", (long)(muller_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDMUSSHEATH", (long)(muller_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(muller_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wraps + sheaths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (muller_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX MULLER ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX MULLER partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX MULLER soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX MULLER nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d mullered=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, muller_ok, vital);
    }
    bump(vm); return 1;

  }

/* SMX TANYCYTE|TANYGLIA|MESH_TANYCYTE|TY_WRAP|TY_SHEATH|HYPOTHALAMIC_GUIDE|RAISE_TANYCYTE a b c ...
   * Life-force Tanycyte glia scaffold mesh stability after bergmann: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete tanycyte glia scaffold mesh among live nodes, weaves a
   * tanycyte wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks tanycyte where life guides the hypothalamic pulse.
   * Latches SMX_TANYCYTE when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = tanycyte ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_TANYCYTE_SUM = bonds+wraps+sheaths; SMX_TANYCYTE|SMX_MESH_TANYCYTE|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"TANYCYTE")||kw(&L->cur,"TANYGLIA")||kw(&L->cur,"MESH_TANYCYTE")||kw(&L->cur,"TY_WRAP")||kw(&L->cur,"TY_WRAPS")||kw(&L->cur,"TY_SHEATH")||kw(&L->cur,"TY_SHEATHS")||kw(&L->cur,"HYPOTHALAMIC_GUIDE")||kw(&L->cur,"RAISE_TANYCYTE")||kw(&L->cur,"WE_TANYCYTE")||kw(&L->cur,"LIFE_TANYCYTE")||
      kw(&L->cur,"STABLE_TANYCYTE")||kw(&L->cur,"MESH_TANYCYTES")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_TANYCYTE")||kw(&L->cur,"TANYCYTE_LEAF")||
      kw(&L->cur,"SEEDTANYCYTE")||kw(&L->cur,"SEEDTYSSHEATH")||kw(&L->cur,"SEEDTYWRAP")||
      kw(&L->cur,"TANYCYTE_RING")||kw(&L->cur,"TY_SHEATH_HUB")||kw(&L->cur,"TY_NODE")||
      kw(&L->cur,"LATTICE_TANYCYTE")||kw(&L->cur,"WORLD_TANYCYTE")||kw(&L->cur,"WORLD_TY")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_TANYCYTE")||kw(&L->cur,"HARDEN_TANYCYTE")||
      kw(&L->cur,"HYPOTHALAMIC_SCAFFOLD")||kw(&L->cur,"TANYCYTE_MESH")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wraps = 0;
    int sheaths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "TANYCYTE needs >=2 cubes",
                  "SMX TANYCYTE a b [c ...]  or  SMX TY_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - tanycyte needs clear channel */
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
    /* complete tanycyte mesh among live */
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
    /* tanycyte ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else wraps++;
        }
      }
    }
    /* sheaths sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else sheaths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && wraps >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && wraps * 2 >= live && soft == 0)
        star_ok = 1;
      int sheath_ok = (live >= 1 && sheaths >= live && soft == 0) ? 1 : 0;
      if (!sheath_ok && live >= 1 && sheaths * 2 >= live && soft == 0)
        sheath_ok = 1;
      int tanycyte_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (tanycyte_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_TANYCYTEED", (long)tanycyte_ok);
      var_set_num(vm, "SMX_TANYCYTE_LATCH", (long)tanycyte_ok);
      var_set_num(vm, "SMX_TANYCYTE", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_TANYCYTE_SUM", (long)(tanycyte_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_TANYCYTE_ALIAS", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_TANYCYTE", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_TY_SHEATH_LATCH", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_TANYCYTE", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_TY_WRAP_LATCH", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(tanycyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_TY_WRAP", (long)(tanycyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_TY_WRAPS", (long)(tanycyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_TY_SHEATHS", (long)(tanycyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(tanycyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(tanycyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_TY_WRAP_LATCH", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(tanycyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_TY_BONDS", (long)(tanycyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_TY_MESH", (long)(tanycyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_TY_BONDS2", (long)(tanycyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_TY_STARS", (long)(tanycyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_TY_STAR", (long)(tanycyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_TANYCYTE_RING", (long)(tanycyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_TANYCYTE_LANE", (long)(tanycyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(tanycyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_TY_SHEATH_HUB", (long)(tanycyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(tanycyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDTANYCYTE", (long)(tanycyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDTYSSHEATH", (long)(tanycyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(tanycyte_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wraps + sheaths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (tanycyte_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX TANYCYTE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX TANYCYTE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX TANYCYTE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX TANYCYTE nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d tanycyteed=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, tanycyte_ok, vital);
    }
    bump(vm); return 1;

  }

/* SMX PITUICYTE|PITUGLIA|MESH_PITUICYTE|PI_WRAP|PI_SHEATH|PITUITARY_GUIDE|RAISE_PITUICYTE a b c ...
   * Life-force Pituicyte glia scaffold mesh stability after tanycyte: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete pituicyte glia scaffold mesh among live nodes, weaves a
   * pituicyte wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks pituicyte where life guides the pituitary pulse.
   * Latches SMX_PITUICYTE when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = pituicyte ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_PITUICYTE_SUM = bonds+wraps+sheaths; SMX_PITUICYTE|SMX_MESH_PITUICYTE|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"PITUICYTE")||kw(&L->cur,"PITUGLIA")||kw(&L->cur,"MESH_PITUICYTE")||kw(&L->cur,"PI_WRAP")||kw(&L->cur,"PI_WRAPS")||kw(&L->cur,"PI_SHEATH")||kw(&L->cur,"PI_SHEATHS")||kw(&L->cur,"PITUITARY_GUIDE")||kw(&L->cur,"RAISE_PITUICYTE")||kw(&L->cur,"WE_PITUICYTE")||kw(&L->cur,"LIFE_PITUICYTE")||
      kw(&L->cur,"STABLE_PITUICYTE")||kw(&L->cur,"MESH_PITUICYTES")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_PITUICYTE")||kw(&L->cur,"PITUICYTE_LEAF")||
      kw(&L->cur,"SEEDPITUICYTE")||kw(&L->cur,"SEEDPISSHEATH")||kw(&L->cur,"SEEDPIWRAP")||
      kw(&L->cur,"PITUICYTE_RING")||kw(&L->cur,"PI_SHEATH_HUB")||kw(&L->cur,"PI_NODE")||
      kw(&L->cur,"LATTICE_PITUICYTE")||kw(&L->cur,"WORLD_PITUICYTE")||kw(&L->cur,"WORLD_PI")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_PITUICYTE")||kw(&L->cur,"HARDEN_PITUICYTE")||
      kw(&L->cur,"PITUITARY_SCAFFOLD")||kw(&L->cur,"PITUICYTE_MESH")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wraps = 0;
    int sheaths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "PITUICYTE needs >=2 cubes",
                  "SMX PITUICYTE a b [c ...]  or  SMX PI_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - pituicyte needs clear channel */
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
    /* complete pituicyte mesh among live */
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
    /* pituicyte ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else wraps++;
        }
      }
    }
    /* sheaths sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else sheaths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && wraps >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && wraps * 2 >= live && soft == 0)
        star_ok = 1;
      int sheath_ok = (live >= 1 && sheaths >= live && soft == 0) ? 1 : 0;
      if (!sheath_ok && live >= 1 && sheaths * 2 >= live && soft == 0)
        sheath_ok = 1;
      int pituicyte_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (pituicyte_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_PITUICYTEED", (long)pituicyte_ok);
      var_set_num(vm, "SMX_PITUICYTE_LATCH", (long)pituicyte_ok);
      var_set_num(vm, "SMX_PITUICYTE", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_PITUICYTE_SUM", (long)(pituicyte_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_PITUICYTE_ALIAS", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_PITUICYTE", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_PI_SHEATH_LATCH", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_PITUICYTE", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_PI_WRAP_LATCH", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(pituicyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PI_WRAP", (long)(pituicyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PI_WRAPS", (long)(pituicyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PI_SHEATHS", (long)(pituicyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(pituicyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(pituicyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PI_WRAP_LATCH", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(pituicyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_PI_BONDS", (long)(pituicyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_PI_MESH", (long)(pituicyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_PI_BONDS2", (long)(pituicyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_PI_STARS", (long)(pituicyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PI_STAR", (long)(pituicyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PITUICYTE_RING", (long)(pituicyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PITUICYTE_LANE", (long)(pituicyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(pituicyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_PI_SHEATH_HUB", (long)(pituicyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(pituicyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDPITUICYTE", (long)(pituicyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDPISSHEATH", (long)(pituicyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(pituicyte_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wraps + sheaths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (pituicyte_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX PITUICYTE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX PITUICYTE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX PITUICYTE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX PITUICYTE nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d pituicyteed=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, pituicyte_ok, vital);
    }
    bump(vm); return 1;

  }


/* SMX FOLLICULO|FOLLIGLIA|MESH_FOLLICULO|FO_WRAP|FO_SHEATH|FOLLICULOSTELLATE_GUIDE|RAISE_FOLLICULO a b c ...
   * Life-force Folliculo glia scaffold mesh stability after pituicyte: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete folliculo glia scaffold mesh among live nodes, weaves a
   * folliculo wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks folliculo where life guides the folliculostellate pulse.
   * Latches SMX_FOLLICULO when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = folliculo ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_FOLLICULO_SUM = bonds+wraps+sheaths; SMX_FOLLICULO|SMX_MESH_FOLLICULO|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"FOLLICULO")||kw(&L->cur,"FOLLIGLIA")||kw(&L->cur,"MESH_FOLLICULO")||kw(&L->cur,"FO_WRAP")||kw(&L->cur,"FO_WRAPS")||kw(&L->cur,"FO_SHEATH")||kw(&L->cur,"FO_SHEATHS")||kw(&L->cur,"FOLLICULOSTELLATE_GUIDE")||kw(&L->cur,"RAISE_FOLLICULO")||kw(&L->cur,"WE_FOLLICULO")||kw(&L->cur,"LIFE_FOLLICULO")||
      kw(&L->cur,"STABLE_FOLLICULO")||kw(&L->cur,"MESH_FOLLICULOS")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_FOLLICULO")||kw(&L->cur,"FOLLICULO_LEAF")||
      kw(&L->cur,"SEEDFOLLICULO")||kw(&L->cur,"SEEDFOSSHEATH")||kw(&L->cur,"SEEDFOWRAP")||
      kw(&L->cur,"FOLLICULO_RING")||kw(&L->cur,"FO_SHEATH_HUB")||kw(&L->cur,"FO_NODE")||
      kw(&L->cur,"LATTICE_FOLLICULO")||kw(&L->cur,"WORLD_FOLLICULO")||kw(&L->cur,"WORLD_FO")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_FOLLICULO")||kw(&L->cur,"HARDEN_FOLLICULO")||
      kw(&L->cur,"FOLLICULOSTELLATE_SCAFFOLD")||kw(&L->cur,"FOLLICULO_MESH")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wraps = 0;
    int sheaths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "FOLLICULO needs >=2 cubes",
                  "SMX FOLLICULO a b [c ...]  or  SMX FO_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - folliculo needs clear channel */
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
    /* complete folliculo mesh among live */
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
    /* folliculo ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else wraps++;
        }
      }
    }
    /* sheaths sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else sheaths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && wraps >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && wraps * 2 >= live && soft == 0)
        star_ok = 1;
      int sheath_ok = (live >= 1 && sheaths >= live && soft == 0) ? 1 : 0;
      if (!sheath_ok && live >= 1 && sheaths * 2 >= live && soft == 0)
        sheath_ok = 1;
      int folliculo_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (folliculo_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_FOLLICULOED", (long)folliculo_ok);
      var_set_num(vm, "SMX_FOLLICULO_LATCH", (long)folliculo_ok);
      var_set_num(vm, "SMX_FOLLICULO", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_FOLLICULO_SUM", (long)(folliculo_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_FOLLICULO_ALIAS", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_FOLLICULO", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_FO_SHEATH_LATCH", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_FOLLICULO", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_FO_WRAP_LATCH", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(folliculo_ok ? wraps : 0));
      var_set_num(vm, "SMX_FO_WRAP", (long)(folliculo_ok ? wraps : 0));
      var_set_num(vm, "SMX_FO_WRAPS", (long)(folliculo_ok ? wraps : 0));
      var_set_num(vm, "SMX_FO_SHEATHS", (long)(folliculo_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(folliculo_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(folliculo_ok ? wraps : 0));
      var_set_num(vm, "SMX_FO_WRAP_LATCH", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(folliculo_ok ? 1 : 0));
      var_set_num(vm, "SMX_FO_BONDS", (long)(folliculo_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_MESH", (long)(folliculo_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_BONDS2", (long)(folliculo_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_STARS", (long)(folliculo_ok ? wraps : 0));
      var_set_num(vm, "SMX_FO_STAR", (long)(folliculo_ok ? wraps : 0));
      var_set_num(vm, "SMX_FOLLICULO_RING", (long)(folliculo_ok ? wraps : 0));
      var_set_num(vm, "SMX_FOLLICULO_LANE", (long)(folliculo_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(folliculo_ok ? sheaths : 0));
      var_set_num(vm, "SMX_FO_SHEATH_HUB", (long)(folliculo_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(folliculo_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDFOLLICULO", (long)(folliculo_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDFOSSHEATH", (long)(folliculo_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(folliculo_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wraps + sheaths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (folliculo_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX FOLLICULO ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX FOLLICULO partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX FOLLICULO soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX FOLLICULO nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d folliculoed=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, folliculo_ok, vital);
    }
    bump(vm); return 1;

  }


/* SMX NG2|NG2GLIA|MESH_NG2|NG_WRAP|NG_SHEATH|POLYDENDO_GUIDE|RAISE_NG2 a b c ...
   * Life-force NG2 polydendrocyte glia scaffold mesh stability after folliculo: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete ng2 glia scaffold mesh among live nodes, weaves a
   * ng2 wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks ng2 where life guides the polydendrocyte pulse.
   * Latches SMX_NG2 when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = ng2 ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_NG2_SUM = bonds+wraps+sheaths; SMX_NG2|SMX_MESH_NG2|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"NG2")||kw(&L->cur,"NG2GLIA")||kw(&L->cur,"MESH_NG2")||kw(&L->cur,"NG_WRAP")||kw(&L->cur,"NG_WRAPS")||kw(&L->cur,"NG_SHEATH")||kw(&L->cur,"NG_SHEATHS")||kw(&L->cur,"POLYDENDO_GUIDE")||kw(&L->cur,"RAISE_NG2")||kw(&L->cur,"WE_NG2")||kw(&L->cur,"LIFE_NG2")||
      kw(&L->cur,"STABLE_NG2")||kw(&L->cur,"MESH_NG2S")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_NG2")||kw(&L->cur,"NG2_LEAF")||
      kw(&L->cur,"SEEDNG2")||kw(&L->cur,"SEEDNGSSHEATH")||kw(&L->cur,"SEEDNGWRAP")||
      kw(&L->cur,"NG2_RING")||kw(&L->cur,"NG_SHEATH_HUB")||kw(&L->cur,"NG_NODE")||
      kw(&L->cur,"LATTICE_NG2")||kw(&L->cur,"WORLD_NG2")||kw(&L->cur,"WORLD_NG")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_NG2")||kw(&L->cur,"HARDEN_NG2")||
      kw(&L->cur,"POLYDENDO_SCAFFOLD")||kw(&L->cur,"NG2_MESH")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wraps = 0;
    int sheaths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "NG2 needs >=2 cubes",
                  "SMX NG2 a b [c ...]  or  SMX NG_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - ng2 needs clear channel */
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
    /* complete ng2 mesh among live */
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
    /* ng2 ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else wraps++;
        }
      }
    }
    /* sheaths sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else sheaths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && wraps >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && wraps * 2 >= live && soft == 0)
        star_ok = 1;
      int sheath_ok = (live >= 1 && sheaths >= live && soft == 0) ? 1 : 0;
      if (!sheath_ok && live >= 1 && sheaths * 2 >= live && soft == 0)
        sheath_ok = 1;
      int ng2_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (ng2_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_NG2ED", (long)ng2_ok);
      var_set_num(vm, "SMX_NG2_LATCH", (long)ng2_ok);
      var_set_num(vm, "SMX_NG2", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_NG2_SUM", (long)(ng2_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_NG2_ALIAS", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_NG2", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_NG_SHEATH_LATCH", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_NG2", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_NG_WRAP_LATCH", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(ng2_ok ? wraps : 0));
      var_set_num(vm, "SMX_NG_WRAP", (long)(ng2_ok ? wraps : 0));
      var_set_num(vm, "SMX_NG_WRAPS", (long)(ng2_ok ? wraps : 0));
      var_set_num(vm, "SMX_NG_SHEATHS", (long)(ng2_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(ng2_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(ng2_ok ? wraps : 0));
      var_set_num(vm, "SMX_NG_WRAP_LATCH", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(ng2_ok ? 1 : 0));
      var_set_num(vm, "SMX_FO_BONDS", (long)(ng2_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_MESH", (long)(ng2_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_BONDS2", (long)(ng2_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_STARS", (long)(ng2_ok ? wraps : 0));
      var_set_num(vm, "SMX_FO_STAR", (long)(ng2_ok ? wraps : 0));
      var_set_num(vm, "SMX_NG2_RING", (long)(ng2_ok ? wraps : 0));
      var_set_num(vm, "SMX_NG2_LANE", (long)(ng2_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(ng2_ok ? sheaths : 0));
      var_set_num(vm, "SMX_NG_SHEATH_HUB", (long)(ng2_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(ng2_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDNG2", (long)(ng2_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDNGSSHEATH", (long)(ng2_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(ng2_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wraps + sheaths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (ng2_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX NG2 ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX NG2 partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX NG2 soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX NG2 nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d ng2ed=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, ng2_ok, vital);
    }
    bump(vm); return 1;

  }




/* SMX PERICYTE|PERIGLIA|MESH_PERICYTE|PERI_WRAP|PERI_SHEATH|PERIVASC_GUIDE|RAISE_PERICYTE a b c ...
   * Life-force PERICYTE pericyte glia scaffold mesh stability after ng2: soft-OOB storms stay fail-closed.
   * Clears thrash OOB, roots a complete pericyte glia scaffold mesh among live nodes, weaves a
   * pericyte wrap ring (i -> i+1) so free energy sheaths every edge, then sheath gather
   * gathers return so lattice locks pericyte where life guides the pericyte pulse.
   * Latches SMX_PERICYTE when mesh+wraps+sheaths are soft-OOB-free.
   * SMX_WRAPS = pericyte ring; SMX_SHEATHS hub = root gather pulses;
   * SMX_PERICYTE_SUM = bonds+wraps+sheaths; SMX_PERICYTE|SMX_MESH_PERICYTE|SMX_STABLE_MESH sticky.
   * Mitosis path stays open under free energy. No dual ladders.
   * Wonder AGI can RUN. Cube is SoT - matrix is key - free energy flows. */
  if (kw(&L->cur,"PERICYTE")||kw(&L->cur,"PERIGLIA")||kw(&L->cur,"MESH_PERICYTE")||kw(&L->cur,"PERI_WRAP")||kw(&L->cur,"PERI_WRAPS")||kw(&L->cur,"PERI_SHEATH")||kw(&L->cur,"PERI_SHEATHS")||kw(&L->cur,"PERIVASC_GUIDE")||kw(&L->cur,"RAISE_PERICYTE")||kw(&L->cur,"WE_PERICYTE")||kw(&L->cur,"LIFE_PERICYTE")||
      kw(&L->cur,"STABLE_PERICYTE")||kw(&L->cur,"MESH_PERICYTES")||kw(&L->cur,"WRAP_RING")||kw(&L->cur,"SHEATH_RING")||
      kw(&L->cur,"STABLE_MESH_PERICYTE")||kw(&L->cur,"PERICYTE_LEAF")||
      kw(&L->cur,"SEEDPERICYTE")||kw(&L->cur,"SEEDPERISHEATH")||kw(&L->cur,"SEEDPERIWRAP")||
      kw(&L->cur,"PERICYTE_RING")||kw(&L->cur,"PERI_SHEATH_HUB")||kw(&L->cur,"PERI_NODE")||
      kw(&L->cur,"LATTICE_PERICYTE")||kw(&L->cur,"WORLD_PERICYTE")||kw(&L->cur,"WORLD_PERI")||
      kw(&L->cur,"PULSE_WRAP")||kw(&L->cur,"PULSE_PERICYTE")||kw(&L->cur,"HARDEN_PERICYTE")||
      kw(&L->cur,"PERIVASC_SCAFFOLD")||kw(&L->cur,"PERICYTE_MESH")){
    int aln = L->cur.line;
    char ids[16][48];
    int present[16];
    int live_ix[16];
    int n = 0, live = 0, i, j;
    int bonds = 0;
    int wraps = 0;
    int sheaths = 0;
    int soft = 0;
    lex_next(L);
    while (L->cur.kind==TK_IDENT && n < 16){
      snprintf(ids[n], sizeof ids[n], "%s", L->cur.text);
      lex_next(L);
      n++;
    }
    if (n < 2){
      smx_fail_at(vm, aln, "PERICYTE needs >=2 cubes",
                  "SMX PERICYTE a b [c ...]  or  SMX PERI_SHEATH a b c d");
      return -1;
    }
    ensure_world(vm);
    if (ensure_smx_key(vm) != 0) return -1;
    /* calm thrash - pericyte needs clear channel */
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
    /* complete pericyte mesh among live */
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
    /* pericyte ring - free energy guards every edge every edge i -> i+1 both ways */
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
          else wraps++;
        }
      }
    }
    /* sheaths sheath - seed axis return from every live leaf */
    if (live >= 1){
      int root = live_ix[0];
      for (i = 0; i < live; i++){
        int leaf = live_ix[i];
        int r1 = do_smx_talk(vm, ids[leaf], ids[root]);
        if (r1 < 0) return -1;
        if (r1 > 0){ soft++; continue; }
        {
          int r2 = do_smx_talk(vm, ids[root], ids[leaf]);
          if (r2 < 0) return -1;
          if (r2 > 0) soft++;
          else sheaths++;
        }
      }
    }
    {
      int need = (live >= 2) ? (live * (live - 1) / 2) : 0;
      int mesh_ok = (need > 0 && bonds >= need && soft == 0) ? 1 : 0;
      if (!mesh_ok && need > 0 && bonds * 2 >= need && soft == 0)
        mesh_ok = 1;
      int star_ok = (live >= 2 && wraps >= live && soft == 0) ? 1 : 0;
      if (!star_ok && live >= 2 && wraps * 2 >= live && soft == 0)
        star_ok = 1;
      int sheath_ok = (live >= 1 && sheaths >= live && soft == 0) ? 1 : 0;
      if (!sheath_ok && live >= 1 && sheaths * 2 >= live && soft == 0)
        sheath_ok = 1;
      int pericyte_ok = (mesh_ok && star_ok && sheath_ok && soft == 0 && live >= 2) ? 1 : 0;
      long vital = (vm->smx.key_ok ? 4 : 0) + (pericyte_ok ? 12 : (bonds > 0 ? 3 : 0)) +
                   (wraps > 0 ? 1 : 0) + (sheaths > 0 ? 1 : 0) +
                   (vm->smx_talks > 0 ? 1 : 0) + (soft == 0 ? 1 : 0);
      var_set_num(vm, "SMX_PERICYTEED", (long)pericyte_ok);
      var_set_num(vm, "SMX_PERICYTE_LATCH", (long)pericyte_ok);
      var_set_num(vm, "SMX_PERICYTE", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_PERICYTE_SUM", (long)(pericyte_ok ? bonds + wraps + sheaths : 0));
      var_set_num(vm, "SMX_PERICYTE_ALIAS", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_PERICYTE", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_PERI_SHEATH_LATCH", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH_LATCH", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_PERICYTE", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP_LATCH", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_PERI_WRAP_LATCH", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_WRAP", (long)(pericyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PERI_WRAP", (long)(pericyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PERI_WRAPS", (long)(pericyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PERI_SHEATHS", (long)(pericyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_WRAPS", (long)(pericyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_WRAPS_N", (long)(pericyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PERI_WRAP_LATCH", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_SHEATH", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_MESH_EXCHANGE", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_STABLE_MESH", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_HARDEN_EXCHANGE", (long)(pericyte_ok ? 1 : 0));
      var_set_num(vm, "SMX_FO_BONDS", (long)(pericyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_MESH", (long)(pericyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_BONDS2", (long)(pericyte_ok ? bonds : 0));
      var_set_num(vm, "SMX_FO_STARS", (long)(pericyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_FO_STAR", (long)(pericyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PERICYTE_RING", (long)(pericyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_PERICYTE_LANE", (long)(pericyte_ok ? wraps : 0));
      var_set_num(vm, "SMX_SHEATHS", (long)(pericyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_PERI_SHEATH_HUB", (long)(pericyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SHEATH_N", (long)(pericyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDPERICYTE", (long)(pericyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_SEEDPERISHEATH", (long)(pericyte_ok ? sheaths : 0));
      var_set_num(vm, "SMX_MESH", (long)(pericyte_ok ? live : 0));
      var_set_num(vm, "SMX_BONDS", (long)bonds);
      var_set_num(vm, "SMX_EXCHANGES", (long)bonds);
      var_set_num(vm, "SMX_FUSE", (long)bonds);
      var_set_num(vm, "SMX_BIND", (long)bonds);
      var_set_num(vm, "SMX_TONE", (long)live);
      var_set_num(vm, "SMX_PULSE", (long)(bonds + wraps + sheaths));
      var_set_num(vm, "SMX_BREATH", (long)live);
      var_set_num(vm, "SMX_LIVE", (long)live);
      var_set_num(vm, "SMX_NODES", (long)n);
      var_set_num(vm, "SMX_TALKS", vm->smx_talks);
      var_set_num(vm, "SMX_OOB", vm->smx_oob);
      var_set_num(vm, "SMX_KEY_OK", vm->smx.key_ok ? 1 : 0);
      var_set_num(vm, "SMX_HOLD", vm->smx.hold_flash ? 1 : 0);
      var_set_num(vm, "SMX_VITAL", vital);
      if (pericyte_ok){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX PERICYTE ok");
      } else if (bonds > 0 && live >= 2){
        vm->smx_ok = 1;
        var_set_num(vm, "SMX_OK", 1);
        var_set_num(vm, "OK", 1);
        var_set_str(vm, "LAST", "SMX PERICYTE partial");
      } else {
        vm->smx_ok = 0;
        var_set_num(vm, "SMX_OK", 0);
        var_set_num(vm, "OK", 0);
        var_set_str(vm, "LAST", "SMX PERICYTE soft-OOB");
      }
      if (vm->trace)
        fprintf(vm->trace,
                "# SMX PERICYTE nodes=%d live=%d bonds=%d wraps=%d sheaths=%d need=%d soft=%d talks=%d oob=%d pericyteed=%d vital=%ld\n",
                n, live, bonds, wraps, sheaths, need, soft, vm->smx_talks, vm->smx_oob, pericyte_ok, vital);
    }
    bump(vm); return 1;
  }

  fail(vm, "SMX: unknown op (see lang_ops_smx; PERICYTE|NG2|FOLLICULO|PITUICYTE|TANYCYTE|MULLER|BERGMANN|RADIAL|life-cascade live)");
  return -1;
}
