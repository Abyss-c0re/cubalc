#!/bin/sh
# run plate preload JSON array of -I / CUBALC_PRELOAD names
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"

# empty preload
OUT=$("$CUBALC" run -q -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"preload_n":0'
printf '%s\n' "$OUT" | grep -q '"preload":\[\]'

# single -I
OUT=$("$CUBALC" run -q -I agent_boot -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"preload_n":1'
printf '%s\n' "$OUT" | grep -q '"preload":\["agent_boot"\]'
printf '%s\n' "$OUT" | grep -q 'agent_boot.cubalc'

# multi + env
OUT=$(CUBALC_PRELOAD=hold_seed "$CUBALC" run -q -I agent_boot -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"preload_n":2'
printf '%s\n' "$OUT" | grep -q 'hold_seed'
printf '%s\n' "$OUT" | grep -q 'agent_boot'
printf '%s\n' "$OUT" | grep -q '"includes_n":2'

# request vs resolved: agent can compare preload[] to includes[]
OUT=$("$CUBALC" run -q -I agent_boot -e 'NEEDINCLUDE agent_boot
PASS
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"preload":\["agent_boot"\]'

echo "1267_cli_run_preload_plate: PASS"
