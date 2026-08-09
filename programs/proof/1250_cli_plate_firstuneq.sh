#!/bin/sh
# cubalc plate firstuneq — FIRSTUNEQFLAT dual (read-only)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_firstuneq"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"meta":{"role":"worker"},"cfg":{"role":"worker","host":"h1"},"other":{"host":"h2","role":"leader"}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate firstuneq "$F" role)
printf '%s\n' "$OUT" | grep -q '"op":"firstuneq"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"found":true'
printf '%s\n' "$OUT" | grep -q '"leaf":"other.role"'
printf '%s\n' "$OUT" | grep -q '"ref":"worker"'
printf '%s\n' "$OUT" | grep -q '"value":"leader"'
printf '%s\n' "$OUT" | grep -q '"n":3'
printf '%s\n' "$OUT" | grep -q other.role

"$CUBALC" plate setbyval "$F" leader worker | grep -q '"ok":true'
OUT=$("$CUBALC" plate firstuneq "$F" role)
printf '%s\n' "$OUT" | grep -q '"found":false'
printf '%s\n' "$OUT" | grep -q '"n":3'

OUT=$("$CUBALC" plate firstuneq "$F" host)
printf '%s\n' "$OUT" | grep -q '"found":true'
printf '%s\n' "$OUT" | grep -q '"ref":"h1"'
printf '%s\n' "$OUT" | grep -q '"value":"h2"'

OUT=$("$CUBALC" plate firstuneq "$F" nope)
printf '%s\n' "$OUT" | grep -q '"found":false'
printf '%s\n' "$OUT" | grep -q '"n":0'

# aliases
OUT=$("$CUBALC" plate pathuneq "$F" host)
printf '%s\n' "$OUT" | grep -q '"op":"firstuneq"'
printf '%s\n' "$OUT" | grep -q '"found":true'

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'firstuneq'

echo "1250_cli_plate_firstuneq: PASS"
