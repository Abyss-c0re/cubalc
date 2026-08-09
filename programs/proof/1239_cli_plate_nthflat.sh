#!/bin/sh
# cubalc plate nthflat — NTHFLAT dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_nthflat"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"meta":{"role":"worker"},"cfg":{"role":"leader","host":"h1"},"other":{"host":"h2"}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate nthflat "$F" role 0)
printf '%s\n' "$OUT" | grep -q '"op":"nthflat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"hit":true'
printf '%s\n' "$OUT" | grep -q '"i":0'
printf '%s\n' "$OUT" | grep -q '"leaf":"meta.role"'
printf '%s\n' "$OUT" | grep -q worker

OUT=$("$CUBALC" plate nthflat "$F" role 1)
printf '%s\n' "$OUT" | grep -q leader
printf '%s\n' "$OUT" | grep -q '"leaf":"cfg.role"'

OUT=$("$CUBALC" plate nthflat "$F" role 2 OR fallback)
printf '%s\n' "$OUT" | grep -q '"hit":false'
printf '%s\n' "$OUT" | grep -q '"fallback":true'
printf '%s\n' "$OUT" | grep -q fallback

OUT=$("$CUBALC" plate nthflat "$F" host 1)
printf '%s\n' "$OUT" | grep -q h2

# plate not mutated
OUT=$("$CUBALC" plate get "$F" meta.role)
printf '%s\n' "$OUT" | grep -q worker

OUT=$("$CUBALC" plate indexflat "$F" role 0)
printf '%s\n' "$OUT" | grep -q '"op":"nthflat"'

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'nthflat'

echo "1239_cli_plate_nthflat: PASS"
