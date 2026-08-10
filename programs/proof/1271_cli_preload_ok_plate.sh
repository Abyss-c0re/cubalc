#!/bin/sh
# run plate preload_ok / preload_miss / preload_miss_n — dual of PRELOADOK/PRELOADMISS
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"

# no -I → preload_ok true, empty miss
OUT=$("$CUBALC" run -q -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"preload_n":0'
printf '%s\n' "$OUT" | grep -q '"preload_ok":true'
printf '%s\n' "$OUT" | grep -q '"preload_miss_n":0'
printf '%s\n' "$OUT" | grep -q '"preload_miss":\[\]'

# successful -I → all loaded → preload_ok
OUT=$("$CUBALC" run -q -I agent_boot -I hold_seed -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"preload_n":2'
printf '%s\n' "$OUT" | grep -q '"preload_ok":true'
printf '%s\n' "$OUT" | grep -q '"preload_miss_n":0'
printf '%s\n' "$OUT" | grep -q '"preload_miss":\[\]'
printf '%s\n' "$OUT" | grep -q 'agent_boot'
printf '%s\n' "$OUT" | grep -q 'hold_seed'
printf '%s\n' "$OUT" | grep -q '"includes_n":2'

# env CUBALC_PRELOAD alone
OUT=$(CUBALC_PRELOAD=hold_seed "$CUBALC" run -q -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"preload_ok":true'
printf '%s\n' "$OUT" | grep -q '"preload_n":1'
printf '%s\n' "$OUT" | grep -q 'hold_seed.cubalc'

# in-lang PRELOADOK agrees with plate when -I succeeds
OUT=$("$CUBALC" run -q -I hold_seed -e 'PRELOADOK
ASSERT LAST_N == 1
PRELOADMISS
ASSERT LAST_N == 0
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"preload_ok":true'

echo "1271_cli_preload_ok_plate: PASS"
