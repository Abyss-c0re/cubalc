#!/bin/sh
# cubalc plate freqflat — FREQFLAT dual (read-only)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_freqflat"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"meta":{"role":"worker"},"cfg":{"role":"worker","host":"h1"},"other":{"host":"h2","role":"leader"}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate freqflat "$F" role)
printf '%s\n' "$OUT" | grep -q '"op":"freqflat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"total":3'
printf '%s\n' "$OUT" | grep -q '"n":2'
printf '%s\n' "$OUT" | grep -q 'worker:2'
printf '%s\n' "$OUT" | grep -q 'leader:1'
printf '%s\n' "$OUT" | grep -q '"needle":"role"'

# plate not mutated
OUT=$("$CUBALC" plate get "$F" meta.role)
printf '%s\n' "$OUT" | grep -q worker

OUT=$("$CUBALC" plate freqflat "$F" host)
printf '%s\n' "$OUT" | grep -q 'h1:1'
printf '%s\n' "$OUT" | grep -q 'h2:1'
printf '%s\n' "$OUT" | grep -q '"total":2'

# all leaves
OUT=$("$CUBALC" plate freqflat "$F")
printf '%s\n' "$OUT" | grep -q '"total":5'

# alias
OUT=$("$CUBALC" plate histflat "$F" role)
printf '%s\n' "$OUT" | grep -q '"op":"freqflat"'
printf '%s\n' "$OUT" | grep -q worker:2

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'freqflat'

echo "1241_cli_plate_freqflat: PASS"
