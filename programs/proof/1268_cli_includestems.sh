#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"

OUT=$("$CUBALC" run -q -I agent_boot -I hold_seed -e 'INCLUDESTEMS
ASSERT LAST_N == 2
SYS HASLINE LAST "agent_boot"
ASSERT LAST_N == 1
SYS HASLINE LAST "hold_seed"
ASSERT LAST_N == 1
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"preload_n":2'

OUT=$("$CUBALC" forms INCLUDESTEM 2>&1)
printf '%s\n' "$OUT" | grep -qi 'INCLUDESTEM'

echo "1268_cli_includestems: PASS"
