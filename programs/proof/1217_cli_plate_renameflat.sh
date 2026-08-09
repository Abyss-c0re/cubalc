#!/bin/sh
# cubalc plate renameflat — RENAMEFLAT dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_renameflat"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"host":"cubeA","cfg":{"meta":{"role":"worker","x":1},"port":8080},"n":3}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate renameflat "$F" cfg. config.)
printf '%s\n' "$OUT" | grep -q '"op":"renameflat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":3'
printf '%s\n' "$OUT" | grep -q '"total":5'
printf '%s\n' "$OUT" | grep -q '"old":"cfg."'
printf '%s\n' "$OUT" | grep -q '"new":"config."'
printf '%s\n' "$OUT" | grep -q '"role":"worker"'
printf '%s\n' "$OUT" | grep -q 'config'

OUT=$("$CUBALC" plate get "$F" config.meta.role)
printf '%s\n' "$OUT" | grep -q worker
OUT=$("$CUBALC" plate get "$F" host)
printf '%s\n' "$OUT" | grep -q cubeA
# old path miss - get may soft fail
OUT=$("$CUBALC" plate get "$F" config.port)
printf '%s\n' "$OUT" | grep -q 8080

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'renameflat'

echo "1217_cli_plate_renameflat: PASS"
