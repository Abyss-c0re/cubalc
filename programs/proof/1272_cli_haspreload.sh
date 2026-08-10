#!/bin/sh
# HASPRELOAD after cubalc run -I
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"

OUT=$("$CUBALC" run -q -I agent_boot -I hold_seed -e 'HASPRELOAD agent_boot
ASSERT LAST_N == 1
HASPRELOAD hold_seed
ASSERT LAST_N == 1
HASPRELOAD no_such_zzz
ASSERT LAST_N == 0
HASINCLUDE agent_boot
ASSERT LAST_N == 1
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"preload_ok":true'

# forms catalog
OUT=$("$CUBALC" forms HASPRELOAD 2>&1)
printf '%s\n' "$OUT" | grep -q 'HASPRELOAD'

# no -I → miss
OUT=$("$CUBALC" run -q -e 'HASPRELOAD agent_boot
ASSERT LAST_N == 0
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1272_cli_haspreload: PASS"
