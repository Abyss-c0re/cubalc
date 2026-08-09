#!/bin/sh
# cubalc plate leaves — PATHKEYS dual (dotted leaf paths)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_leaves"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"host":"cubeA","cfg":{"meta":{"x":1,"role":"worker","zone":"A"},"port":8080},"empty":{},"n":3}' | grep -q '"ok":true'

# full plate leaf paths — bag lines above plate
OUT=$("$CUBALC" plate leaves "$F")
printf '%s\n' "$OUT" | grep -q '^host$'
printf '%s\n' "$OUT" | grep -q 'cfg.meta.role'
printf '%s\n' "$OUT" | grep -q 'cfg.port'
printf '%s\n' "$OUT" | grep -q '^empty$'
printf '%s\n' "$OUT" | grep -q '"op":"leaves"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":7'

# relative under nest
OUT=$("$CUBALC" plate leaves "$F" cfg)
printf '%s\n' "$OUT" | grep -q 'meta.role'
printf '%s\n' "$OUT" | grep -q '^port$'
printf '%s\n' "$OUT" | grep -q '"nest":"cfg"'
printf '%s\n' "$OUT" | grep -q '"n":4'

# deeper
OUT=$("$CUBALC" plate pathkeys "$F" cfg.meta)
printf '%s\n' "$OUT" | grep -q '^role$'
printf '%s\n' "$OUT" | grep -q '"op":"leaves"'
printf '%s\n' "$OUT" | grep -q '"n":3'

# soft miss nest
OUT=$("$CUBALC" plate leaves "$F" nope)
printf '%s\n' "$OUT" | grep -q '"n":0'

# help lists leaves
OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'leaves'

echo "1210_cli_plate_leaves: PASS"
