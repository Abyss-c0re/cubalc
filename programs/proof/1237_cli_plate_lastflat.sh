#!/bin/sh
# cubalc plate lastflat — LASTFLAT dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_lastflat"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"meta":{"role":"worker","port":1},"cfg":{"role":"leader","port":9090}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate lastflat "$F" role)
printf '%s\n' "$OUT" | grep -q '"op":"lastflat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"hit":true'
printf '%s\n' "$OUT" | grep -q '"leaf":"cfg.role"'
printf '%s\n' "$OUT" | grep -q leader

OUT=$("$CUBALC" plate getflat "$F" role)
printf '%s\n' "$OUT" | grep -q worker

OUT=$("$CUBALC" plate lastflat "$F" port)
printf '%s\n' "$OUT" | grep -q 9090
printf '%s\n' "$OUT" | grep -q '"leaf":"cfg.port"'

OUT=$("$CUBALC" plate lastflat "$F" nope OR fallback)
printf '%s\n' "$OUT" | grep -q '"hit":false'
printf '%s\n' "$OUT" | grep -q '"fallback":true'
printf '%s\n' "$OUT" | grep -q fallback

# plate not mutated
OUT=$("$CUBALC" plate get "$F" meta.role)
printf '%s\n' "$OUT" | grep -q worker

OUT=$("$CUBALC" plate endflat "$F" role)
printf '%s\n' "$OUT" | grep -q '"op":"lastflat"'
printf '%s\n' "$OUT" | grep -q leader

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'lastflat'

echo "1237_cli_plate_lastflat: PASS"
