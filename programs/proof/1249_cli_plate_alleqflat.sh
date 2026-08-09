#!/bin/sh
# cubalc plate alleqflat — ALLEQFLAT dual (read-only)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_alleqflat"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"meta":{"role":"worker"},"cfg":{"role":"worker","host":"h1"},"other":{"host":"h2","role":"leader"}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate alleqflat "$F" role)
printf '%s\n' "$OUT" | grep -q '"op":"alleqflat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"eq":false'
printf '%s\n' "$OUT" | grep -q '"n":3'

"$CUBALC" plate setbyval "$F" leader worker | grep -q '"ok":true'
OUT=$("$CUBALC" plate alleqflat "$F" role)
printf '%s\n' "$OUT" | grep -q '"eq":true'
printf '%s\n' "$OUT" | grep -q '"value":"worker"'
printf '%s\n' "$OUT" | grep -q worker

OUT=$("$CUBALC" plate alleqflat "$F" host)
printf '%s\n' "$OUT" | grep -q '"eq":false'
printf '%s\n' "$OUT" | grep -q '"n":2'

OUT=$("$CUBALC" plate alleqflat "$F" nope)
printf '%s\n' "$OUT" | grep -q '"eq":false'
printf '%s\n' "$OUT" | grep -q '"n":0'

# alias
OUT=$("$CUBALC" plate samevalflat "$F" role)
printf '%s\n' "$OUT" | grep -q '"op":"alleqflat"'
printf '%s\n' "$OUT" | grep -q '"eq":true'

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'alleqflat'

echo "1249_cli_plate_alleqflat: PASS"
