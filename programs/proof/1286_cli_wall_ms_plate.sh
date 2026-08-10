#!/bin/sh
# run plate wall_ms + WALL_MS form + STATUS
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

# plate reports wall_ms after sleep
OUT=$("$CUBALC" run -q -e 'SYS SLEEP 40
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
w=$(printf '%s\n' "$OUT" | sed -n 's/.*"wall_ms":\([0-9][0-9]*\).*/\1/p' | head -1)
test -n "$w"
test "$w" -ge 30

# form + plate
OUT=$("$CUBALC" run -q -e 'SYS SLEEP 30
WALL_MS
ASSERT LAST_N >= 25
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"wall_ms":'

# STATUS mid-run
OUT=$("$CUBALC" run -q -e 'SYS SLEEP 25
STATUS
ASSERT WALL_MS >= 20
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# forms catalog
OUT=$("$CUBALC" forms wall 2>&1)
printf '%s\n' "$OUT" | grep -qi 'WALL_MS'

# file proof
OUT=$("$CUBALC" run -q programs/proof/1286_wall_ms.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1286_cli_wall_ms_plate: PASS"
