#!/bin/sh
# cubalc plate pathbyval — PATHBYVAL dual (read-only)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_pathbyval"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"meta":{"role":"worker"},"cfg":{"role":"leader","host":"h1"},"other":{"host":"h2"}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate pathbyval "$F" worker)
printf '%s\n' "$OUT" | grep -q '"op":"pathbyval"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"hit":true'
printf '%s\n' "$OUT" | grep -q '"leaf":"meta.role"'
printf '%s\n' "$OUT" | grep -q meta.role
printf '%s\n' "$OUT" | grep -q '"value":"worker"'

OUT=$("$CUBALC" plate pathbyval "$F" leader)
printf '%s\n' "$OUT" | grep -q cfg.role

OUT=$("$CUBALC" plate pathbyval "$F" h2)
printf '%s\n' "$OUT" | grep -q other.host

# miss + OR
OUT=$("$CUBALC" plate pathbyval "$F" nope OR default.path)
printf '%s\n' "$OUT" | grep -q '"hit":false'
printf '%s\n' "$OUT" | grep -q '"fallback":true'
printf '%s\n' "$OUT" | grep -q default.path

# plate not mutated
OUT=$("$CUBALC" plate get "$F" meta.role)
printf '%s\n' "$OUT" | grep -q worker

# alias
OUT=$("$CUBALC" plate valpath "$F" worker)
printf '%s\n' "$OUT" | grep -q '"op":"pathbyval"'
printf '%s\n' "$OUT" | grep -q meta.role

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'pathbyval'

echo "1243_cli_plate_pathbyval: PASS"
