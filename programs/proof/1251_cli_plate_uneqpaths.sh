#!/bin/sh
# cubalc plate uneqpaths — UNEQPATHS dual (read-only)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_uneqpaths"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"meta":{"role":"worker"},"cfg":{"role":"worker","host":"h1"},"other":{"host":"h2","role":"leader"}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate uneqpaths "$F" role)
printf '%s\n' "$OUT" | grep -q '"op":"uneqpaths"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":1'
printf '%s\n' "$OUT" | grep -q '"total":3'
printf '%s\n' "$OUT" | grep -q '"ref":"worker"'
printf '%s\n' "$OUT" | grep -q other.role

"$CUBALC" plate setbyval "$F" leader worker | grep -q '"ok":true'
OUT=$("$CUBALC" plate uneqpaths "$F" role)
printf '%s\n' "$OUT" | grep -q '"n":0'
printf '%s\n' "$OUT" | grep -q '"total":3'

OUT=$("$CUBALC" plate uneqpaths "$F" host)
printf '%s\n' "$OUT" | grep -q '"n":1'
printf '%s\n' "$OUT" | grep -q '"ref":"h1"'

OUT=$("$CUBALC" plate uneqpaths "$F" nope)
printf '%s\n' "$OUT" | grep -q '"n":0'
printf '%s\n' "$OUT" | grep -q '"total":0'

# alias
OUT=$("$CUBALC" plate divergepaths "$F" host)
printf '%s\n' "$OUT" | grep -q '"op":"uneqpaths"'
printf '%s\n' "$OUT" | grep -q '"n":1'

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'uneqpaths'

echo "1251_cli_plate_uneqpaths: PASS"
