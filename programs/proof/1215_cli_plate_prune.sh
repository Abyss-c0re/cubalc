#!/bin/sh
# cubalc plate prune / keeponly — PRUNEFLAT duals
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_prune"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"host":"cubeA","cfg":{"meta":{"role":"worker"},"port":8080,"tmp":9},"n":3,"debug":1}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate prune "$F" tmp)
printf '%s\n' "$OUT" | grep -q '"op":"prune"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":1'
printf '%s\n' "$OUT" | grep -q '"keep_only":false'
# tmp gone, host remains
OUT=$("$CUBALC" plate has "$F" cfg.tmp || true)
printf '%s\n' "$OUT" | grep -q '"ok":true'
# has returns soft - check get miss
OUT=$("$CUBALC" plate get "$F" host)
printf '%s\n' "$OUT" | grep -q cubeA

OUT=$("$CUBALC" plate prune "$F" debug)
printf '%s\n' "$OUT" | grep -q '"n":1'

G="$ST/proj.json"
"$CUBALC" plate ensure "$G" '{"host":"cubeA","cfg":{"meta":{"role":"worker"},"port":8080},"n":3}' | grep -q '"ok":true'
OUT=$("$CUBALC" plate keeponly "$G" cfg)
printf '%s\n' "$OUT" | grep -q '"op":"keeponly"'
printf '%s\n' "$OUT" | grep -q '"keep_only":true'
printf '%s\n' "$OUT" | grep -q '"n":2'
printf '%s\n' "$OUT" | grep -q '"role":"worker"'
# host should be gone from plate body
printf '%s\n' "$OUT" | grep -q '"plate":'
# verify host missing via has
OUT=$("$CUBALC" plate get "$G" cfg.port)
printf '%s\n' "$OUT" | grep -q 8080

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'prune'

echo "1215_cli_plate_prune: PASS"
