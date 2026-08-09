#!/bin/sh
# cubalc plate flat — FLATKV dual (path:value bag)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_flat"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"host":"cubeA","cfg":{"meta":{"x":1,"role":"worker","zone":"A"},"port":8080},"empty":{},"n":3,"ok":true}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate flat "$F")
printf '%s\n' "$OUT" | grep -q '^host:cubeA$'
printf '%s\n' "$OUT" | grep -q 'cfg.meta.role:worker'
printf '%s\n' "$OUT" | grep -q 'cfg.port:8080'
printf '%s\n' "$OUT" | grep -q '^empty:$'
printf '%s\n' "$OUT" | grep -q 'ok:true'
printf '%s\n' "$OUT" | grep -q '"op":"flat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":8'

OUT=$("$CUBALC" plate flat "$F" cfg)
printf '%s\n' "$OUT" | grep -q 'meta.role:worker'
printf '%s\n' "$OUT" | grep -q '^port:8080$'
printf '%s\n' "$OUT" | grep -q '"nest":"cfg"'
printf '%s\n' "$OUT" | grep -q '"n":4'

OUT=$("$CUBALC" plate flatkv "$F" cfg.meta)
printf '%s\n' "$OUT" | grep -q '^role:worker$'
printf '%s\n' "$OUT" | grep -q '"op":"flat"'
printf '%s\n' "$OUT" | grep -q '"n":3'

OUT=$("$CUBALC" plate flat "$F" nope)
printf '%s\n' "$OUT" | grep -q '"n":0'

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'flat'

echo "1211_cli_plate_flat: PASS"
