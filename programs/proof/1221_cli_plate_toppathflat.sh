#!/bin/sh
# cubalc plate toppath|botpath — TOPPATHFLAT/BOTPATHFLAT dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_toppath"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"scores":{"a":10,"b":30,"c":5},"meta":{"scores":{"z":40}}}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate toppath "$F" scores)
printf '%s\n' "$OUT" | grep -q '"op":"toppath"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"leaf":"meta.scores.z"'
printf '%s\n' "$OUT" | grep -q '"n":40'
printf '%s\n' "$OUT" | grep -q '"count":4'
printf '%s\n' "$OUT" | grep -q '"needle":"scores"'

OUT=$("$CUBALC" plate botpath "$F" scores)
printf '%s\n' "$OUT" | grep -q '"op":"botpath"'
printf '%s\n' "$OUT" | grep -q '"leaf":"scores.c"'
printf '%s\n' "$OUT" | grep -q '"n":5'

OUT=$("$CUBALC" plate toppath "$F")
printf '%s\n' "$OUT" | grep -q '"leaf":"meta.scores.z"'
printf '%s\n' "$OUT" | grep -q '"n":40'

# plate not mutated
OUT=$("$CUBALC" plate get "$F" scores.b)
printf '%s\n' "$OUT" | grep -q 30

OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'toppath'

echo "1221_cli_plate_toppathflat: PASS"
