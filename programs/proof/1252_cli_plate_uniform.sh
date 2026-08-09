#!/bin/sh
# cubalc plate uniform — UNIFORMFLAT dual (read-only one-shot)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_uniform"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"meta":{"role":"worker"},"cfg":{"role":"worker","host":"h1"},"other":{"host":"h2","role":"leader"}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate uniform "$F" role)
printf '%s\n' "$OUT" | grep -q '"op":"uniform"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"eq":false'
printf '%s\n' "$OUT" | grep -q '"n":3'
printf '%s\n' "$OUT" | grep -q '"div":1'
printf '%s\n' "$OUT" | grep -q '"path_first":"other.role"'
printf '%s\n' "$OUT" | grep -q '"ref":"worker"'
printf '%s\n' "$OUT" | grep -q '"value":"leader"'
printf '%s\n' "$OUT" | grep -q other.role

"$CUBALC" plate setbyval "$F" leader worker | grep -q '"ok":true'
OUT=$("$CUBALC" plate uniform "$F" role)
printf '%s\n' "$OUT" | grep -q '"eq":true'
printf '%s\n' "$OUT" | grep -q '"div":0'
printf '%s\n' "$OUT" | grep -q '"common":"worker"'
printf '%s\n' "$OUT" | grep -q worker

OUT=$("$CUBALC" plate uniform "$F" host)
printf '%s\n' "$OUT" | grep -q '"eq":false'
printf '%s\n' "$OUT" | grep -q '"div":1'

OUT=$("$CUBALC" plate uniform "$F" nope)
printf '%s\n' "$OUT" | grep -q '"eq":false'
printf '%s\n' "$OUT" | grep -q '"n":0'

# alias
OUT=$("$CUBALC" plate checkflat "$F" role)
printf '%s\n' "$OUT" | grep -q '"op":"uniform"'
printf '%s\n' "$OUT" | grep -q '"eq":true'

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'uniform'

echo "1252_cli_plate_uniform: PASS"
