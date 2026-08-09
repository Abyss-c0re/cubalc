#!/bin/sh
# cubalc plate uniqflat — UNIQFLAT dual (read-only)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_uniqflat"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"meta":{"role":"worker"},"cfg":{"role":"worker","host":"h1"},"other":{"host":"h2","role":"leader"}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate uniqflat "$F" role)
printf '%s\n' "$OUT" | grep -q '"op":"uniqflat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"total":3'
printf '%s\n' "$OUT" | grep -q '"n":2'
printf '%s\n' "$OUT" | grep -q worker
printf '%s\n' "$OUT" | grep -q leader
printf '%s\n' "$OUT" | grep -q '"needle":"role"'

# plate not mutated
OUT=$("$CUBALC" plate get "$F" meta.role)
printf '%s\n' "$OUT" | grep -q worker

OUT=$("$CUBALC" plate uniqflat "$F" host)
printf '%s\n' "$OUT" | grep -q h1
printf '%s\n' "$OUT" | grep -q h2
printf '%s\n' "$OUT" | grep -q '"total":2'

# all leaves
OUT=$("$CUBALC" plate uniqflat "$F")
printf '%s\n' "$OUT" | grep -q '"total":5'

# alias
OUT=$("$CUBALC" plate distinctflat "$F" role)
printf '%s\n' "$OUT" | grep -q '"op":"uniqflat"'
printf '%s\n' "$OUT" | grep -q worker

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'uniqflat'

echo "1244_cli_plate_uniqflat: PASS"
