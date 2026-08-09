#!/bin/sh
# cubalc plate getn — GETPN dual (numeric peel, paths ok)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_getn"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"host":"cubeA","cfg":{"port":8080,"tls":0},"freq":{"error":5},"n":1}' | grep -q '"ok":true'

# nest path
OUT=$("$CUBALC" plate getn "$F" cfg.port)
printf '%s\n' "$OUT" | grep -q '"op":"getn"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"hit":true'
printf '%s\n' "$OUT" | grep -q '"n":8080'

OUT=$("$CUBALC" plate getn "$F" freq.error)
printf '%s\n' "$OUT" | grep -q '"n":5'

OUT=$("$CUBALC" plate getn "$F" cfg.tls)
printf '%s\n' "$OUT" | grep -q '"n":0'
printf '%s\n' "$OUT" | grep -q '"hit":true'

# shallow
OUT=$("$CUBALC" plate getn "$F" n)
printf '%s\n' "$OUT" | grep -q '"n":1'

# soft OR
OUT=$("$CUBALC" plate getn "$F" missing.path OR 42)
printf '%s\n' "$OUT" | grep -q '"hit":false'
printf '%s\n' "$OUT" | grep -q '"or":true'
printf '%s\n' "$OUT" | grep -q '"n":42'

# hard miss
set +e
OUT=$("$CUBALC" plate getn "$F" nope)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"hit":false'
printf '%s\n' "$OUT" | grep -q 'key miss'
[ "$RC" -ne 0 ]

# aliases
OUT=$("$CUBALC" plate num "$F" cfg.port)
printf '%s\n' "$OUT" | grep -q '"op":"getn"'
printf '%s\n' "$OUT" | grep -q '"n":8080'

# help
OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'getn'

echo "1202_cli_plate_getn: PASS"
