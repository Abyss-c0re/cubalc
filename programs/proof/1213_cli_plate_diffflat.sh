#!/bin/sh
# cubalc plate diffflat / pathdiff — DIFFFLAT dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_diffflat"
A="$ST/a.json"
B="$ST/b.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$A" '{"host":"cubeA","cfg":{"meta":{"role":"worker","x":1},"port":8080},"n":3}' | grep -q '"ok":true'
"$CUBALC" plate ensure "$B" '{"host":"cubeA","cfg":{"meta":{"role":"leader","x":1,"zone":"A"},"port":9090},"n":3,"ok":true}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate diffflat "$A" "$B" || true)
printf '%s\n' "$OUT" | grep -q 'cfg.meta.role: worker → leader'
printf '%s\n' "$OUT" | grep -q 'cfg.port: 8080 → 9090'
printf '%s\n' "$OUT" | grep -q 'cfg.meta.zone: (missing) → A'
printf '%s\n' "$OUT" | grep -q '"op":"diffflat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":4'
printf '%s\n' "$OUT" | grep -q '"equal":false'

OUT=$("$CUBALC" plate pathdiff "$A" "$B" || true)
printf '%s\n' "$OUT" | grep -q '^cfg.meta.role$'
printf '%s\n' "$OUT" | grep -q '^cfg.port$'
printf '%s\n' "$OUT" | grep -q '"op":"pathdiff"'
printf '%s\n' "$OUT" | grep -q '"n":4'

# equal → exit 0 and n=0
OUT=$("$CUBALC" plate diffflat "$A" "$A")
printf '%s\n' "$OUT" | grep -q '"n":0'
printf '%s\n' "$OUT" | grep -q '"equal":true'

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'diffflat'

echo "1213_cli_plate_diffflat: PASS"
