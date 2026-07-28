#!/usr/bin/env bash
# Cross-device SMX2 demo (TCP, no HTTP). Shared key required on both hosts.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${CUBALC_BIN:-$ROOT/out/cubalc}"
PORT="${SMX_PORT:-17733}"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state}"
export CUBALC_SMX_KEY="${CUBALC_SMX_KEY:-c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3c3}"

[[ -x "$BIN" ]] || make -C "$ROOT" all

case "${1:-prove}" in
  prove|prove-tcp)
    echo "# local two-process TCP proof port=$PORT"
    "$BIN" smx-bus prove-tcp "$PORT"
    ;;
  serve)
    BIND="${2:-0.0.0.0:$PORT}"
    echo "# serve SMX2 on $BIND (share CUBALC_SMX_KEY with peer)"
    exec "$BIN" smx-bus serve "$BIND"
    ;;
  dial)
    PEER="${2:-127.0.0.1:$PORT}"
    echo "# dial SMX2 $PEER"
    exec "$BIN" smx-bus dial "$PEER"
    ;;
  *)
    echo "usage: $0 prove | serve [host:port] | dial host:port"
    exit 2
    ;;
esac
