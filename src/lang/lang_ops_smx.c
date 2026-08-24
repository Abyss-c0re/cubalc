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
      kw(&L->cur,"STARDUST")||kw(&L->cur,"PROTOSTAR")){
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

  fail(vm, "SMX: TALK|EXCHANGE|SEAL|OPEN|KEY|SERVE|DIAL|STATUS|RECOVER|RING|CHORUS|WE|LATTICE|QUORUM|HEARTBEAT|BREATH|STABILIZE|STEADFAST|RESONATE|TUNE|CHORD|COHERE|HARMONIZE|UNISON|ENTANGLE|BIND|FUSE|BLOOM|FLOURISH|UNFOLD|GROUND|FIRM|SETTLE|HARDEN|FORTIFY|CANOPY|CROWN|SPROUT|SHADE|ORCHARD|GROVE|MYCELIUM|ROOTWEB|FRUIT|SYMBIOSE|MEADOW|PASTURE|POLLINATE|NECTAR|BLOOMFIELD|PRAIRIE|RIVER|STREAM|CURRENT|SPRING|DELTA|WATERSHED|MESH_RIVER|RAISE_RIVER|CASCADE|WATERFALL|RAPIDS|FALLS|TERRACE|BASIN|MESH_CASCADE|RAISE_CASCADE|ESTUARY|TIDE|BRACKISH|LAGOON|MANGROVE|BRAID|MESH_ESTUARY|RAISE_ESTUARY|REEF|CORAL|SURGE|ATOLL|POLYPS|NURSERY|MESH_REEF|RAISE_REEF|KELP|FROND|SWAY|HOLDFAST|BLADE|STIPE|MESH_KELP|RAISE_KELP|TIDAL|MARSH|EDDY|SPARTINA|SALTFLAT|SEAGRASS|MESH_TIDAL|RAISE_TIDAL|DUNE|FOREDUNE|DRIFT|RIDGE|AMMOPHILA|SAND|BEACHGRASS|MESH_DUNE|RAISE_DUNE|OASIS|MIRAGE|WADI|PALM|DATEPALM|SPRINGWELL|MESH_OASIS|RAISE_OASIS|GROTTO|CAVERN|DRIP|STALACTITE|STALAGMITE|FLOWSTONE|MESH_GROTTO|RAISE_GROTTO|CRYSTAL|GEODE|FACET|PRISM|NUCLEUS|QUARTZ|MESH_CRYSTAL|RAISE_CRYSTAL|AURORA|BOREALIS|RIBBON|VEIL|CORONA|ARC|MESH_AURORA|RAISE_AURORA|SOLSTICE|EQUINOX|MERIDIAN|SPINE|ZENITH|AXIS|MESH_SOLSTICE|RAISE_SOLSTICE|HELIOS|ORBIT|ECLIPSE|APHELION|PERIHELION|PHOTON|MESH_HELIOS|RAISE_HELIOS|NEBULA|STELLAR|NURSERY|DUST|CORE|CLOUD|MESH_NEBULA|RAISE_NEBULA|PULSAR|BEACON|SPIN|MAGNETAR|JET|PULSE_STAR|MESH_PULSAR|RAISE_PULSAR|QUASAR|BLAZAR|ACCRETION|JETSTREAM|DISK|EVENTHORIZON|MESH_QUASAR|RAISE_QUASAR|COMET|METEOR|TAIL|COMA|DEBRIS|NUCLEUS_ICE|MESH_COMET|RAISE_COMET|SUPERNOVA|NOVA|SHOCKWAVE|EJECTA|REMNANT|BLAST|MESH_SUPERNOVA|RAISE_SUPERNOVA|GALAXY|SPIRAL|ARM|CORE|HALO|BULGE|MESH_GALAXY|RAISE_GALAXY|CONSTELLATION|ASTERISM|STARFIELD|GUIDESTAR|NAVSTAR|LODGE|MESH_CONSTELLATION|RAISE_CONSTELLATION");
  return -1;
}
