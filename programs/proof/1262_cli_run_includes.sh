#!/bin/sh
# run plate includes_n / includes — LISTINCLUDES dual for agents (no .cubalc parse)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"

# no INCLUDE → empty array
OUT=$("$CUBALC" run -q -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"includes_n":0'
printf '%s\n' "$OUT" | grep -q '"includes":\[\]'

# -I preload surfaces resolved path on plate
OUT=$("$CUBALC" run -q -I agent_boot -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"includes_n":1'
printf '%s\n' "$OUT" | grep -q 'agent_boot'
printf '%s\n' "$OUT" | grep -q '"preload_n":1'

# multi include
OUT=$("$CUBALC" run -q -I hold_seed -I agent_boot -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"includes_n":2'
printf '%s\n' "$OUT" | grep -q 'hold_seed'
printf '%s\n' "$OUT" | grep -q 'agent_boot'

# in-language INCLUDE also fills plate
OUT=$("$CUBALC" run -q -e 'INCLUDE hold_seed
PASS
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"includes_n":1'
printf '%s\n' "$OUT" | grep -q 'hold_seed'

# INCLUDE ONCE skip does not inflate count
OUT=$("$CUBALC" run -q -e 'INCLUDE ONCE hold_seed
INCLUDE ONCE hold_seed
PASS
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"includes_n":1'

echo "1262_cli_run_includes: PASS"
