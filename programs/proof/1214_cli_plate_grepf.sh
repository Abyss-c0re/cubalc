#!/bin/sh
# cubalc plate grepf — GREPFLAT dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_grepf"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"host":"cubeA","cfg":{"meta":{"role":"worker","x":1},"port":8080,"tmp":1},"n":3,"ok":true}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate grepf "$F" cfg)
printf '%s\n' "$OUT" | grep -q 'cfg.meta.role:worker'
printf '%s\n' "$OUT" | grep -q 'cfg.port:8080'
printf '%s\n' "$OUT" | grep -q '"op":"grepf"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":4'
printf '%s\n' "$OUT" | grep -q '"needle":"cfg"'
# host not in bag
printf '%s\n' "$OUT" | grep -v 'host:cubeA' | grep -q '"op":"grepf"'

OUT=$("$CUBALC" plate grepvf "$F" cfg)
printf '%s\n' "$OUT" | grep -q 'host:cubeA'
printf '%s\n' "$OUT" | grep -q '"op":"grepvf"'
printf '%s\n' "$OUT" | grep -q '"invert":true'
printf '%s\n' "$OUT" | grep -q '"n":3'

OUT=$("$CUBALC" plate grepfi "$F" CFG.META)
printf '%s\n' "$OUT" | grep -q 'cfg.meta.role:worker'
printf '%s\n' "$OUT" | grep -q '"icase":true'
printf '%s\n' "$OUT" | grep -q '"n":2'

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'grepf'

echo "1214_cli_plate_grepf: PASS"
