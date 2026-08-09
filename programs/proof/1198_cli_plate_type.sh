#!/bin/sh
# cubalc plate type|kind — TYPEP dual (kind probe, dotted paths)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_type"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"host":"cubeA","freq":{"error":5,"warn":2},"meta":{"role":"leader"},"on":true,"z":null,"arr":[1,2]}' | grep -q '"ok":true'

OUT=$("$CUBALC" plate type "$F" host)
printf '%s\n' "$OUT" | grep -q '"op":"type"'
printf '%s\n' "$OUT" | grep -q '"kind":"str"'
printf '%s\n' "$OUT" | grep -q '"hit":true'
printf '%s\n' "$OUT" | grep -q '"n":2'

OUT=$("$CUBALC" plate type "$F" freq.error)
printf '%s\n' "$OUT" | grep -q '"kind":"num"'
printf '%s\n' "$OUT" | grep -q '"n":1'

OUT=$("$CUBALC" plate type "$F" freq)
printf '%s\n' "$OUT" | grep -q '"kind":"obj"'
printf '%s\n' "$OUT" | grep -q '"n":5'

OUT=$("$CUBALC" plate type "$F" on)
printf '%s\n' "$OUT" | grep -q '"kind":"bool"'

OUT=$("$CUBALC" plate type "$F" z)
printf '%s\n' "$OUT" | grep -q '"kind":"null"'

OUT=$("$CUBALC" plate type "$F" arr)
printf '%s\n' "$OUT" | grep -q '"kind":"arr"'

# soft miss
set +e
OUT=$("$CUBALC" plate type "$F" freq.crit)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"kind":"missing"'
printf '%s\n' "$OUT" | grep -q '"hit":false'
test "$RC" -ne 0

# aliases
OUT=$("$CUBALC" plate kind "$F" meta.role)
printf '%s\n' "$OUT" | grep -q '"kind":"str"'
printf '%s\n' "$OUT" | grep -q '"op":"type"'

OUT=$("$CUBALC" plate typep "$F" freq/error)
printf '%s\n' "$OUT" | grep -q '"kind":"num"'

# help lists type
OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q '"type"'

echo "1198_cli_plate_type: PASS"
