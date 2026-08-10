#!/bin/sh
# run plate include_stems / include_stems_n — dual of INCLUDESTEMS
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"

# empty includes
OUT=$("$CUBALC" run -q -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"includes_n":0'
printf '%s\n' "$OUT" | grep -q '"include_stems_n":0'
printf '%s\n' "$OUT" | grep -q '"include_stems":\[\]'

# -I → short stems match preload names
OUT=$("$CUBALC" run -q -I agent_boot -I hold_seed -e 'INCLUDESTEMS
ASSERT LAST_N == 2
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"includes_n":2'
printf '%s\n' "$OUT" | grep -q '"include_stems_n":2'
printf '%s\n' "$OUT" | grep -q '"include_stems":\["agent_boot","hold_seed"\]\|"include_stems":\["hold_seed","agent_boot"\]'
printf '%s\n' "$OUT" | grep -q 'agent_boot.cubalc'
printf '%s\n' "$OUT" | grep -q '"preload":\["agent_boot","hold_seed"\]\|"preload":\["hold_seed","agent_boot"\]'
printf '%s\n' "$OUT" | grep -q '"preload_ok":true'

# in-program INCLUDE only (no -I) still gets stems
OUT=$("$CUBALC" run -q -e 'INCLUDE hold_seed
PASS
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"include_stems_n":1'
printf '%s\n' "$OUT" | grep -q '"include_stems":\["hold_seed"\]'
printf '%s\n' "$OUT" | grep -q '"preload_n":0'

echo "1273_cli_include_stems_plate: PASS"
