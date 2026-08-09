#!/bin/sh
# cubalc plate setflat — SETFLAT dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_setflat"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"host":"cubeA","cfg":{"meta":{"role":"worker","debug":1},"port":8080,"debug":1},"n":3}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate setflat "$F" debug 0)
printf '%s\n' "$OUT" | grep -q '"op":"setflat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":2'
printf '%s\n' "$OUT" | grep -q '"needle":"debug"'
printf '%s\n' "$OUT" | grep -q '"value":"0"'

OUT=$("$CUBALC" plate get "$F" cfg.meta.debug)
printf '%s\n' "$OUT" | grep -q 0
OUT=$("$CUBALC" plate get "$F" cfg.debug)
printf '%s\n' "$OUT" | grep -q 0
OUT=$("$CUBALC" plate get "$F" cfg.meta.role)
printf '%s\n' "$OUT" | grep -q worker

OUT=$("$CUBALC" plate setflat "$F" cfg.port 9090)
printf '%s\n' "$OUT" | grep -q '"n":1'
OUT=$("$CUBALC" plate get "$F" cfg.port)
printf '%s\n' "$OUT" | grep -q 9090

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'setflat'

echo "1218_cli_plate_setflat: PASS"
