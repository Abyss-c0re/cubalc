#!/bin/sh
# cubalc plate countbyval|hasval — COUNTBYVAL/HASVAL duals (read-only)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_countbyval"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"meta":{"role":"worker"},"cfg":{"role":"worker","host":"h1"},"other":{"host":"h2","role":"leader"}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate countbyval "$F" worker)
printf '%s\n' "$OUT" | grep -q '"op":"countbyval"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":2'
printf '%s\n' "$OUT" | grep -q '"value":"worker"'

OUT=$("$CUBALC" plate countbyval "$F" leader)
printf '%s\n' "$OUT" | grep -q '"n":1'

OUT=$("$CUBALC" plate countbyval "$F" nope)
printf '%s\n' "$OUT" | grep -q '"n":0'

OUT=$("$CUBALC" plate hasval "$F" worker)
printf '%s\n' "$OUT" | grep -q '"op":"hasval"'
printf '%s\n' "$OUT" | grep -q '"hit":true'
printf '%s\n' "$OUT" | grep -q '"n":2'

OUT=$("$CUBALC" plate hasval "$F" nope)
printf '%s\n' "$OUT" | grep -q '"hit":false'
printf '%s\n' "$OUT" | grep -q '"n":0'

# plate not mutated
OUT=$("$CUBALC" plate get "$F" meta.role)
printf '%s\n' "$OUT" | grep -q worker

# aliases
OUT=$("$CUBALC" plate nbyval "$F" worker)
printf '%s\n' "$OUT" | grep -q '"op":"countbyval"'
OUT=$("$CUBALC" plate hasvalflat "$F" leader)
printf '%s\n' "$OUT" | grep -q '"op":"hasval"'

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'countbyval'
printf '%s\n' "$OUT" | grep -q 'hasval'

echo "1246_cli_plate_countbyval: PASS"
