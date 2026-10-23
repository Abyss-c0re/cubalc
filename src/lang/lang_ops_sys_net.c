/* CubalC lang — lang_ops_sys_net.c
 * SYS TCPXFER / TCPLISTEN peer xfer plane (A2 theme peel).
 * Mesh vote A2_theme_peel_core_sys · zero bloat · cube is SoT.
 */
#include "lang/cubalc_lang_ops_sys_net.h"
#include "lang/cubalc_lang_ops_sys_codec.h"
#include "cubalc_hostops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#if !defined(CUBALC_OS_WINDOWS)
#  include <unistd.h>
#  include <fcntl.h>
#  include <poll.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#endif

int cubalc_lang_ops_sys_try_net(VM *vm, Lex *L) {
  /* TCP peer xfer SYS plane.
   * Caller: after bulk_mut miss; L at subcommand.
   * Returns 1 handled, 0 not this plane, -1 fail.
   */
#if !defined(CUBALC_OS_WINDOWS)
    /* SYS TCPXFER|TCPBIDIR|NETXFER host:port [payload]
     * — connect, send CBXF-framed payload, recv peer reply → LAST.
     * Soft fail on connect/protocol (OK=0). Timeout CUBALC_P2P_TIMEOUT ms (def 30s).
     * Two-arg payload, or one-arg endpoint with payload=LAST.
     * Usability: bidirectional any-string peer flow (not matrix-only SMX). */
    if (kw(&L->cur,"TCPXFER") || kw(&L->cur,"TCPBIDIR") || kw(&L->cur,"NETXFER") ||
        kw(&L->cur,"CBXF") || kw(&L->cur,"DATAXFER") || kw(&L->cur,"PEERXFER")){
      char ep[256], payload[CUBALC_HOST_STR_MAX], host[128], reply[CUBALC_HOST_STR_MAX];
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
      int port = 0, fd = -1, to_ms = 30000;
      size_t nrx = 0;
      const char *te;
      lex_next(L);
      ep[0] = 0; payload[0] = 0; a[0] = 0; b[0] = 0; reply[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS TCPXFER host:port [payload]");
        return -1;
      }
      if (resolve_str_arg(vm, L, b, sizeof b) == 0) {
        snprintf(ep, sizeof ep, "%s", a);
        snprintf(payload, sizeof payload, "%s", b);
      } else {
        snprintf(ep, sizeof ep, "%s", a);
        snprintf(payload, sizeof payload, "%s", vm->last_str);
      }
      te = getenv("CUBALC_P2P_TIMEOUT");
      if (te && te[0]) {
        to_ms = atoi(te);
        if (to_ms < 0) to_ms = 0;
        if (to_ms > 600000) to_ms = 600000;
      }
      if (cubalc_parse_host_port_simple(ep, host, sizeof host, &port) != 0) {
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "TCPXFER_TX", 0);
        var_set_num(vm, "TCPXFER_RX", 0);
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "TCPXFER: bad host:port");
        var_set_str(vm, "ERR", "TCPXFER: bad host:port");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        vm->last_n = 0;
        bump(vm); return 1;
      }
      fd = cubalc_smx_tcp_connect(host, port);
      if (fd < 0) {
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "TCPXFER_TX", 0);
        var_set_num(vm, "TCPXFER_RX", 0);
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "TCPXFER: connect fail");
        var_set_str(vm, "ERR", "TCPXFER: connect fail — start TCPLISTEN peer");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        vm->last_n = 0;
        bump(vm); return 1;
      }
      if (cubalc_cbxf_send(fd, payload, strlen(payload)) != 0) {
        close(fd);
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "TCPXFER_TX", 0);
        var_set_num(vm, "TCPXFER_RX", 0);
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "TCPXFER: send fail");
        var_set_str(vm, "ERR", "TCPXFER: send fail");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        vm->last_n = 0;
        bump(vm); return 1;
      }
      if (cubalc_cbxf_recv(fd, reply, sizeof reply, &nrx, to_ms > 0 ? to_ms : 30000) != 0) {
        close(fd);
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_num(vm, "TCPXFER_TX", (long)strlen(payload));
        var_set_num(vm, "TCPXFER_RX", 0);
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "TCPXFER: recv fail");
        var_set_str(vm, "ERR", "TCPXFER: recv fail — peer TCPLISTEN must reply");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        vm->last_n = 0;
        bump(vm); return 1;
      }
      close(fd);
      var_set_num(vm, "OK", 1);
      var_set_num(vm, "LAST_N", (long)nrx);
      var_set_num(vm, "TCPXFER_TX", (long)strlen(payload));
      var_set_num(vm, "TCPXFER_RX", (long)nrx);
      var_set_num(vm, "TCPBIDIR_TX", (long)strlen(payload));
      var_set_num(vm, "TCPBIDIR_RX", (long)nrx);
      var_set_str(vm, "LAST", reply);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", reply);
      vm->last_n = (long)nrx;
      bump(vm); return 1;
    }
    /* SYS TCPLISTEN|TCPSERVE|DATASERVE host:port [reply]
     * — one-shot accept, CBXF recv → LAST, send reply (or echo if omit).
     * Soft timeout (CUBALC_P2P_TIMEOUT). Bidirectional any-payload peer serve.
     * Two-arg: fixed reply. One-arg: echo received payload. */
    if (kw(&L->cur,"TCPLISTEN") || kw(&L->cur,"TCPSERVE") || kw(&L->cur,"DATASERVE") ||
        kw(&L->cur,"CBXFLISTEN") || kw(&L->cur,"PEERLISTEN") || kw(&L->cur,"BIDIRLISTEN")){
      char ep[256], reply[CUBALC_HOST_STR_MAX], host[128], got[CUBALC_HOST_STR_MAX];
      char a[CUBALC_HOST_STR_MAX], b[CUBALC_HOST_STR_MAX];
      int port = 0, lfd = -1, cfd = -1, to_ms = 30000, timed = 0, echo = 0;
      size_t nrx = 0;
      const char *te;
      struct pollfd pfd;
      lex_next(L);
      ep[0] = 0; reply[0] = 0; a[0] = 0; b[0] = 0; got[0] = 0;
      if (resolve_str_arg(vm, L, a, sizeof a) != 0) {
        fail(vm, "SYS TCPLISTEN host:port [reply]");
        return -1;
      }
      if (resolve_str_arg(vm, L, b, sizeof b) == 0) {
        snprintf(ep, sizeof ep, "%s", a);
        snprintf(reply, sizeof reply, "%s", b);
        echo = 0;
      } else {
        snprintf(ep, sizeof ep, "%s", a);
        echo = 1;
      }
      te = getenv("CUBALC_P2P_TIMEOUT");
      if (te && te[0]) {
        to_ms = atoi(te);
        if (to_ms < 0) to_ms = 0;
        if (to_ms > 600000) to_ms = 600000;
      }
      if (cubalc_parse_host_port_simple(ep, host, sizeof host, &port) != 0) {
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "TCPLISTEN: bad host:port");
        var_set_str(vm, "ERR", "TCPLISTEN: bad host:port");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        vm->last_n = 0;
        bump(vm); return 1;
      }
      lfd = cubalc_smx_tcp_listen(host, port, 2);
      if (lfd < 0) {
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "TCPLISTEN: bind/listen fail");
        var_set_str(vm, "ERR", "TCPLISTEN: bind/listen fail");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        vm->last_n = 0;
        bump(vm); return 1;
      }
      pfd.fd = lfd; pfd.events = POLLIN; pfd.revents = 0;
      if (to_ms > 0) {
        if (poll(&pfd, 1, to_ms) <= 0) {
          close(lfd);
          var_set_num(vm, "OK", 0);
          var_set_num(vm, "LAST_N", 0);
          var_set_str(vm, "LAST", "");
          var_set_str(vm, "LAST_ERR", "TCPLISTEN: accept timeout");
          var_set_str(vm, "ERR", "TCPLISTEN: accept timeout");
          snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
          vm->last_n = 0;
          bump(vm); return 1;
        }
      }
      cfd = accept(lfd, NULL, NULL);
      close(lfd);
      if (cfd < 0) {
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "TCPLISTEN: accept fail");
        var_set_str(vm, "ERR", "TCPLISTEN: accept fail");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        vm->last_n = 0;
        bump(vm); return 1;
      }
      (void)timed;
      if (cubalc_cbxf_recv(cfd, got, sizeof got, &nrx, to_ms > 0 ? to_ms : 30000) != 0) {
        close(cfd);
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", 0);
        var_set_str(vm, "LAST", "");
        var_set_str(vm, "LAST_ERR", "TCPLISTEN: recv fail");
        var_set_str(vm, "ERR", "TCPLISTEN: recv fail");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", "");
        vm->last_n = 0;
        bump(vm); return 1;
      }
      if (echo) snprintf(reply, sizeof reply, "%s", got);
      if (cubalc_cbxf_send(cfd, reply, strlen(reply)) != 0) {
        close(cfd);
        var_set_num(vm, "OK", 0);
        var_set_num(vm, "LAST_N", (long)nrx);
        var_set_str(vm, "LAST", got);
        var_set_str(vm, "LAST_ERR", "TCPLISTEN: send fail");
        var_set_str(vm, "ERR", "TCPLISTEN: send fail");
        snprintf(vm->last_str, sizeof vm->last_str, "%s", got);
        vm->last_n = (long)nrx;
        bump(vm); return 1;
      }
      close(cfd);
      var_set_num(vm, "OK", 1);
      var_set_num(vm, "LAST_N", (long)nrx);
      var_set_num(vm, "TCPLISTEN_RX", (long)nrx);
      var_set_num(vm, "TCPLISTEN_TX", (long)strlen(reply));
      var_set_str(vm, "LAST", got);
      snprintf(vm->last_str, sizeof vm->last_str, "%s", got);
      vm->last_n = (long)nrx;
      bump(vm); return 1;
    }
#endif
  return 0;
}
