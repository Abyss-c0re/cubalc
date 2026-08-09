#!/bin/sh
# cubalc plate setbyval — SETBYVAL dual (write-back)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_setbyval"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"meta":{"role":"worker"},"cfg":{"role":"worker","host":"h1"},"other":{"host":"h2","role":"leader"}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate setbyval "$F" worker standby)
printf '%s\n' "$OUT" | grep -q '"op":"setbyval"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":2'
printf '%s\n' "$OUT" | grep -q '"old":"worker"'
printf '%s\n' "$OUT" | grep -q '"new":"standby"'

OUT=$("$CUBALC" plate get "$F" meta.role)
printf '%s\n' "$OUT" | grep -q standby
OUT=$("$CUBALC" plate get "$F" cfg.role)
printf '%s\n' "$OUT" | grep -q standby
OUT=$("$CUBALC" plate get "$F" other.role)
printf '%s\n' "$OUT" | grep -q leader
OUT=$("$CUBALC" plate get "$F" cfg.host)
printf '%s\n' "$OUT" | grep -q h1

OUT=$("$CUBALC" plate countbyval "$F" standby)
printf '%s\n' "$OUT" | grep -q '"n":2'

OUT=$("$CUBALC" plate setbyval "$F" leader boss)
printf '%s\n' "$OUT" | grep -q '"n":1'
OUT=$("$CUBALC" plate get "$F" other.role)
printf '%s\n' "$OUT" | grep -q boss

# alias
OUT=$("$CUBALC" plate replaceval "$F" standby worker)
printf '%s\n' "$OUT" | grep -q '"op":"setbyval"'
printf '%s\n' "$OUT" | grep -q '"n":2'

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'setbyval'

echo "1247_cli_plate_setbyval: PASS"
