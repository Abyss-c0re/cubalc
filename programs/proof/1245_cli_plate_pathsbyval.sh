#!/bin/sh
# cubalc plate pathsbyval — PATHSBYVAL dual (read-only)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_pathsbyval"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"meta":{"role":"worker"},"cfg":{"role":"worker","host":"h1"},"other":{"host":"h2","role":"leader"}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate pathsbyval "$F" worker)
printf '%s\n' "$OUT" | grep -q '"op":"pathsbyval"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":2'
printf '%s\n' "$OUT" | grep -q meta.role
printf '%s\n' "$OUT" | grep -q cfg.role
printf '%s\n' "$OUT" | grep -q '"value":"worker"'

OUT=$("$CUBALC" plate pathsbyval "$F" leader)
printf '%s\n' "$OUT" | grep -q other.role
printf '%s\n' "$OUT" | grep -q '"n":1'

OUT=$("$CUBALC" plate pathsbyval "$F" nope)
printf '%s\n' "$OUT" | grep -q '"n":0'

# plate not mutated
OUT=$("$CUBALC" plate get "$F" meta.role)
printf '%s\n' "$OUT" | grep -q worker

# alias
OUT=$("$CUBALC" plate findvals "$F" worker)
printf '%s\n' "$OUT" | grep -q '"op":"pathsbyval"'
printf '%s\n' "$OUT" | grep -q meta.role

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'pathsbyval'

echo "1245_cli_plate_pathsbyval: PASS"
