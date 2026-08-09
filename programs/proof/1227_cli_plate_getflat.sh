#!/bin/sh
# cubalc plate getflat — GETFLAT dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_getflat"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"cfg":{"port":9090,"host":"h"},"meta":{"port":1}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate getflat "$F" port)
printf '%s\n' "$OUT" | grep -q '"op":"getflat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"hit":true'
printf '%s\n' "$OUT" | grep -q '"value":"9090"'
printf '%s\n' "$OUT" | grep -q '"leaf":"cfg.port"'
printf '%s\n' "$OUT" | grep -q '"needle":"port"'

OUT=$("$CUBALC" plate getflat "$F" nope OR fallback)
printf '%s\n' "$OUT" | grep -q '"hit":false'
printf '%s\n' "$OUT" | grep -q '"fallback":true'
printf '%s\n' "$OUT" | grep -q '"value":"fallback"'

OUT=$("$CUBALC" plate getflat "$F" host)
printf '%s\n' "$OUT" | grep -q '"value":"h"'

# plate not mutated
OUT=$("$CUBALC" plate get "$F" cfg.port)
printf '%s\n' "$OUT" | grep -q 9090

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'getflat'

echo "1227_cli_plate_getflat: PASS"
