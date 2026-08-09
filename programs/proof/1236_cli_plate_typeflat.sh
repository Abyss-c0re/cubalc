#!/bin/sh
# cubalc plate typeflat — TYPEFLAT dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_typeflat"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"cfg":{"port":9090,"host":"h","on":true},"label":"x"}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate typeflat "$F" port)
printf '%s\n' "$OUT" | grep -q '"op":"typeflat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"hit":true'
printf '%s\n' "$OUT" | grep -q '"kind":"num"'
printf '%s\n' "$OUT" | grep -q '"n":1'
printf '%s\n' "$OUT" | grep -q '"leaf":"cfg.port"'

OUT=$("$CUBALC" plate typeflat "$F" host)
printf '%s\n' "$OUT" | grep -q '"kind":"str"'
printf '%s\n' "$OUT" | grep -q '"n":2'

OUT=$("$CUBALC" plate typeflat "$F" on)
printf '%s\n' "$OUT" | grep -q '"kind":"bool"'
printf '%s\n' "$OUT" | grep -q '"n":3'

OUT=$("$CUBALC" plate typeflat "$F" nope)
printf '%s\n' "$OUT" | grep -q '"hit":false'
printf '%s\n' "$OUT" | grep -q '"kind":"missing"'
printf '%s\n' "$OUT" | grep -q '"n":0'

# plate not mutated
OUT=$("$CUBALC" plate get "$F" cfg.port)
printf '%s\n' "$OUT" | grep -q 9090

OUT=$("$CUBALC" plate kindflat "$F" port)
printf '%s\n' "$OUT" | grep -q '"op":"typeflat"'
printf '%s\n' "$OUT" | grep -q '"kind":"num"'

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'typeflat'

echo "1236_cli_plate_typeflat: PASS"
