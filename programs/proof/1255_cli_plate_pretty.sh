#!/bin/sh
# cubalc plate pretty — PRETTYP dual (indented plate, no jq)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_pretty"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"meta":{"role":"worker"},"n":1}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate pretty "$F")
printf '%s\n' "$OUT" | grep -q '"op":"pretty"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"truncated":false'
printf '%s\n' "$OUT" | grep -q meta
printf '%s\n' "$OUT" | grep -q role
# human body has indentation
printf '%s\n' "$OUT" | grep -q '  "meta"'

# alias
OUT=$("$CUBALC" plate prettyjson "$F")
printf '%s\n' "$OUT" | grep -q '"op":"pretty"'

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'pretty'

echo "1255_cli_plate_pretty: PASS"
