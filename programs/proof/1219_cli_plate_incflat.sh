#!/bin/sh
# cubalc plate incflat — INCFLAT dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_incflat"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"hits_total":1,"cfg":{"meta":{"hits":2,"role":"w"},"port":8080},"errs":3}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate incflat "$F" hits)
printf '%s\n' "$OUT" | grep -q '"op":"incflat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":2'
printf '%s\n' "$OUT" | grep -q '"delta":1'
printf '%s\n' "$OUT" | grep -q '"needle":"hits"'

OUT=$("$CUBALC" plate get "$F" hits_total)
printf '%s\n' "$OUT" | grep -q 2
OUT=$("$CUBALC" plate get "$F" cfg.meta.hits)
printf '%s\n' "$OUT" | grep -q 3

OUT=$("$CUBALC" plate incflat "$F" port 5)
printf '%s\n' "$OUT" | grep -q '"n":1'
printf '%s\n' "$OUT" | grep -q '"delta":5'
OUT=$("$CUBALC" plate get "$F" cfg.port)
printf '%s\n' "$OUT" | grep -q 8085

OUT=$("$CUBALC" plate decflat "$F" errs)
printf '%s\n' "$OUT" | grep -q '"op":"decflat"'
printf '%s\n' "$OUT" | grep -q '"n":1'
OUT=$("$CUBALC" plate get "$F" errs)
printf '%s\n' "$OUT" | grep -q 2

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'incflat'

echo "1219_cli_plate_incflat: PASS"
