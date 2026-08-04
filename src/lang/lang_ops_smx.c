/* CubalC lang — lang_ops_smx.c
 * P2P / SMX2 plane: KEY · TALK · EXCHANGE · SEAL · OPEN · SERVE · DIAL
 * Law: binary State Matrix only · HOLD_FLASH · fail-closed · no HTTP wire.
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

/* Soft-fail: SMX_OK=0, OK=0, continue program (no fatal). */
static void smx_soft_fail(VM *vm, const char *why){
  vm->smx_ok = 0;
  var_set_num(vm, "SMX_OK", 0);
  var_set_num(vm, "OK", 0);
  var_set_str(vm, "LAST", why ? why : "smx soft fail");
  if (vm->trace)
    fprintf(vm->trace, "# SMX soft-fail: %s\n", why ? why : "");
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
  fail(vm, "SMX key missing (CUBALC_SMX_KEY|CUBALC_SMX_TOKEN)");
  return -1;
}

/* SMX TALK a b — secure State Matrix transfer a→b */
static int do_smx_talk(VM *vm, const char *from_id, const char *to_id){
  ensure_world(vm);
  if (ensure_smx_key(vm) != 0) return -1;
  int ia = find_cube(vm, from_id);
  int ib = find_cube(vm, to_id);
  if (ia < 0){ place_cube(vm, from_id, "host", 1); ia = find_cube(vm, from_id); }
  if (ib < 0){ place_cube(vm, to_id, "body", 1); ib = find_cube(vm, to_id); }
  if (ia < 0 || ib < 0){ fail(vm, "SMX TALK unknown cube"); return -1; }
  cubalc_cube_plug(&vm->ch, ia, ib);
  int rc = cubalc_cube_talk_secure(&vm->ch, &vm->smx, ia, ib);
  if (rc != 0){
    char msg[120];
    snprintf(msg, sizeof msg, "SMX TALK fail %d %s", rc,
             vm->smx.last_err[0] ? vm->smx.last_err : "");
    fail(vm, msg);
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
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"SMX TALK a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"SMX TALK a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    if (do_smx_talk(vm, a, b) != 0) return -1;
    bump(vm); return 1;
  }
  if (kw(&L->cur,"EXCHANGE")||kw(&L->cur,"SWAP")||kw(&L->cur,"PAIR")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"SMX EXCHANGE a b"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"SMX EXCHANGE a b"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    if (do_smx_talk(vm, a, b) != 0) return -1;
    if (do_smx_talk(vm, b, a) != 0) return -1;
    bump(vm); return 1;
  }
  if (kw(&L->cur,"SEAL")||kw(&L->cur,"EMIT")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"SMX SEAL a b path"); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"SMX SEAL a b path"); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    char path[512];
    if (resolve_str_arg(vm, L, path, sizeof path)!=0){
      fail(vm,"SMX SEAL a b \"path\"|LAST"); return -1;
    }
    if (do_smx_seal(vm, a, b, path) != 0) return -1;
    bump(vm); return 1;
  }
  if (kw(&L->cur,"OPEN")||kw(&L->cur,"RECV")||kw(&L->cur,"IMPORT")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"SMX OPEN cube path"); return -1; }
    char id[48]; snprintf(id,sizeof id,"%s",L->cur.text); lex_next(L);
    char path[512];
    if (resolve_str_arg(vm, L, path, sizeof path)!=0){
      fail(vm,"SMX OPEN cube \"path\"|LAST"); return -1;
    }
    if (do_smx_open(vm, id, path) != 0) return -1;
    bump(vm); return 1;
  }

  /* P2P: SMX SERVE local remote "host:port" — listen one binary exchange */
  if (kw(&L->cur,"SERVE")||kw(&L->cur,"LISTEN")||kw(&L->cur,"ACCEPT")){
    lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"SMX SERVE local remote bind"); return -1; }
    char local[48]; snprintf(local,sizeof local,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"SMX SERVE local remote bind"); return -1; }
    char remote[48]; snprintf(remote,sizeof remote,"%s",L->cur.text); lex_next(L);
    char bind[256];
    if (resolve_str_arg(vm, L, bind, sizeof bind)!=0){
      const char *e = getenv("CUBALC_P2P_BIND");
      if (e && e[0]) snprintf(bind,sizeof bind,"%s",e);
      else { fail(vm,"SMX SERVE need \"host:port\"|port or CUBALC_P2P_BIND"); return -1; }
    }
    {
      char host[128]="0.0.0.0";
      int port = 0, lfd, cfd, ia, ib;
      uint8_t frame[512];
      size_t n = 0;
      cubalc_atom recv;
      char fr[CUBALC_ID_LEN], to[CUBALC_ID_LEN];
      if (parse_host_port(bind, host, sizeof host, &port) != 0){
        fail(vm,"SMX SERVE bad port"); return -1;
      }
      ensure_world(vm);
      if (ensure_smx_key(vm) != 0) return -1;
      ia = find_cube(vm, local);
      ib = find_cube(vm, remote);
      if (ia < 0){ place_cube(vm, local, "body", 1); ia = find_cube(vm, local); }
      if (ib < 0){ place_cube(vm, remote, "host", 1); ib = find_cube(vm, remote); }
      lfd = cubalc_smx_tcp_listen(host, port, 4);
      if (lfd < 0){ fail(vm,"SMX SERVE listen fail"); return -1; }
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
            smx_soft_fail(vm, "SMX SERVE timeout");
            bump(vm); return 1;
          }
          fail(vm,"SMX SERVE accept fail"); return -1;
        }
      }
      n = 0;
      if (cubalc_smx_recv_frame(cfd, frame, sizeof frame, &n) != 0){
        close(cfd); fail(vm,"SMX SERVE recv"); return -1;
      }
      if (cubalc_smx_open(&vm->smx, frame, n, &recv, fr, to,
                          &vm->ch.cubes[ia].atom.matrix) != 0){
        close(cfd); fail(vm, vm->smx.last_err[0]?vm->smx.last_err:"SMX SERVE open"); return -1;
      }
      for (int i = 0; i < recv.matrix.n && i < CUBALC_ATOM_BITS; i++)
        if (cubalc_matrix_get(&recv.matrix, i))
          cubalc_matrix_set(&vm->ch.cubes[ia].atom.matrix, i, 1);
      vm->ch.cubes[ia].atom.digit =
        (uint8_t)cubalc_algocube_digit(&vm->ch.cubes[ia].atom.matrix);
      n = 0;
      if (cubalc_smx_seal(&vm->smx, &vm->ch.cubes[ia].atom, local, remote,
                          frame, sizeof frame, &n) != 0){
        close(cfd); fail(vm,"SMX SERVE seal"); return -1;
      }
      if (cubalc_smx_send_frame(cfd, frame, n) != 0){
        close(cfd); fail(vm,"SMX SERVE send"); return -1;
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
    if (kw(&L->cur,"NET")||kw(&L->cur,"TCP")){
      lex_next(L);
      if (!kw(&L->cur,"DIAL") && !kw(&L->cur,"SEND") && !kw(&L->cur,"TALK")){
        fail(vm,"SMX NET DIAL a b \"host:port\""); return -1;
      }
    }
    if (kw(&L->cur,"DIAL")||kw(&L->cur,"SEND")||kw(&L->cur,"TALK"))
      lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"SMX DIAL a b \"host:port\""); return -1; }
    char a[48]; snprintf(a,sizeof a,"%s",L->cur.text); lex_next(L);
    if (L->cur.kind!=TK_IDENT){ fail(vm,"SMX DIAL a b \"host:port\""); return -1; }
    char b[48]; snprintf(b,sizeof b,"%s",L->cur.text); lex_next(L);
    char endpoint[256];
    if (resolve_str_arg(vm, L, endpoint, sizeof endpoint)!=0){
      const char *e = getenv("CUBALC_P2P_PEER");
      if (e && e[0]) snprintf(endpoint,sizeof endpoint,"%s",e);
      else { fail(vm,"SMX DIAL a b \"host:port\" or CUBALC_P2P_PEER"); return -1; }
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
        fail(vm,"SMX DIAL need host:port"); return -1;
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
          smx_soft_fail(vm, "SMX DIAL connect fail");
          bump(vm); return 1;
        }
        fail(vm,"SMX DIAL connect fail"); return -1;
      }
      if (cubalc_smx_seal(&vm->smx, &vm->ch.cubes[ia].atom, a, b,
                          frame, sizeof frame, &n) != 0){
        close(fd);
        if (p2p_soft()){ smx_soft_fail(vm, "SMX DIAL seal"); bump(vm); return 1; }
        fail(vm,"SMX DIAL seal"); return -1;
      }
      if (cubalc_smx_send_frame(fd, frame, n) != 0){
        close(fd);
        if (p2p_soft()){ smx_soft_fail(vm, "SMX DIAL send"); bump(vm); return 1; }
        fail(vm,"SMX DIAL send"); return -1;
      }
      n = 0;
      if (cubalc_smx_recv_frame(fd, frame, sizeof frame, &n) != 0){
        close(fd);
        if (p2p_soft()){ smx_soft_fail(vm, "SMX DIAL recv"); bump(vm); return 1; }
        fail(vm,"SMX DIAL recv"); return -1;
      }
      if (cubalc_smx_open(&vm->smx, frame, n, &recv, fr, to,
                          &vm->ch.cubes[ib].atom.matrix) != 0){
        close(fd);
        if (p2p_soft()){
          smx_soft_fail(vm, vm->smx.last_err[0]?vm->smx.last_err:"SMX DIAL open");
          bump(vm); return 1;
        }
        fail(vm, vm->smx.last_err[0]?vm->smx.last_err:"SMX DIAL open"); return -1;
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

  fail(vm, "SMX: TALK|EXCHANGE|SEAL|OPEN|KEY|SERVE|DIAL");
  return -1;
}
