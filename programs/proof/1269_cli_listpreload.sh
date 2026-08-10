#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"

# -I publishes PRELOAD_ACTIVE for LISTPRELOAD
OUT=$("$CUBALC" run -q -I agent_boot -I hold_seed -e 'LISTPRELOAD
ASSERT LAST_N == 2
SYS HASLINE LAST "agent_boot"
ASSERT LAST_N == 1
SYS HASLINE LAST "hold_seed"
ASSERT LAST_N == 1
INCLUDESTEMS
ASSERT LAST_N == 2
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"preload":\["agent_boot","hold_seed"\]\|"preload":\["hold_seed","agent_boot"\]'

# env PRELOAD alone
OUT=$(CUBALC_PRELOAD=agent_boot "$CUBALC" run -q -e 'LISTPRELOAD
ASSERT LAST_N == 1
SYS HASLINE LAST "agent_boot"
ASSERT LAST_N == 1
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# cubalc env lists PRELOAD_ACTIVE
OUT=$("$CUBALC" env PRELOAD 2>&1)
printf '%s\n' "$OUT" | grep -q 'CUBALC_PRELOAD_ACTIVE'

echo "1269_cli_listpreload: PASS"
