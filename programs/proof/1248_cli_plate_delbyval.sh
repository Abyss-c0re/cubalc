#!/bin/sh
# cubalc plate delbyval — DELBYVAL dual (write-back)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_delbyval"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"meta":{"role":"worker"},"cfg":{"role":"worker","host":"h1"},"other":{"host":"h2","role":"leader"}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate delbyval "$F" worker)
printf '%s\n' "$OUT" | grep -q '"op":"delbyval"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":2'
printf '%s\n' "$OUT" | grep -q '"kept":3'
printf '%s\n' "$OUT" | grep -q '"value":"worker"'

OUT=$("$CUBALC" plate hasval "$F" worker)
printf '%s\n' "$OUT" | grep -q '"hit":false'

OUT=$("$CUBALC" plate get "$F" other.role)
printf '%s\n' "$OUT" | grep -q leader
OUT=$("$CUBALC" plate get "$F" cfg.host)
printf '%s\n' "$OUT" | grep -q h1

OUT=$("$CUBALC" plate delbyval "$F" leader)
printf '%s\n' "$OUT" | grep -q '"n":1'
printf '%s\n' "$OUT" | grep -q '"kept":2'

OUT=$("$CUBALC" plate hasval "$F" leader)
printf '%s\n' "$OUT" | grep -q '"hit":false'

# alias on fresh plate
F2="$ST/nums.json"
rm -f "$F2"
"$CUBALC" plate ensure "$F2" '{"a":1,"b":1,"c":2}' | grep -q '"ok":true'
OUT=$("$CUBALC" plate dropval "$F2" 1)
printf '%s\n' "$OUT" | grep -q '"op":"delbyval"'
printf '%s\n' "$OUT" | grep -q '"n":2'

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'delbyval'

echo "1248_cli_plate_delbyval: PASS"
