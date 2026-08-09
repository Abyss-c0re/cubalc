#!/bin/sh
# cubalc plate mergeflat — MERGEFLAT dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_mergeflat"
A="$ST/base.json"
B="$ST/over.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$A" '{"host":"cubeA","cfg":{"meta":{"role":"worker","x":1},"port":8080},"n":3}' | grep -q '"ok":true'
"$CUBALC" plate ensure "$B" '{"cfg":{"meta":{"role":"leader","zone":"A"},"port":9090},"ok":true}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate mergeflat "$A" "$B")
printf '%s\n' "$OUT" | grep -q '"op":"mergeflat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":4'
printf '%s\n' "$OUT" | grep -q '"role":"leader"'
printf '%s\n' "$OUT" | grep -q '"port":9090'
printf '%s\n' "$OUT" | grep -q '"host":"cubeA"'
printf '%s\n' "$OUT" | grep -q '"zone":"A"'
# x kept
printf '%s\n' "$OUT" | grep -q '"x":1'

OUT=$("$CUBALC" plate get "$A" cfg.meta.role)
printf '%s\n' "$OUT" | grep -q leader
OUT=$("$CUBALC" plate get "$A" host)
printf '%s\n' "$OUT" | grep -q cubeA
OUT=$("$CUBALC" plate get "$A" cfg.meta.x)
printf '%s\n' "$OUT" | grep -q 1

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'mergeflat'

echo "1216_cli_plate_mergeflat: PASS"
