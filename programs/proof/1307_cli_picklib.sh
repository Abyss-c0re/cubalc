#!/bin/sh
# PICKLIB first match + OR fallback
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'PICKLIB plate
ASSERT LAST == "plate_boot"
ASSERT OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'PICKLIB missing_qqq OR hold_seed
ASSERT LAST == "hold_seed"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'PICKLIB missing_qqq
ASSERT OK == 0
CLEAR_ERR
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms picklib 2>&1) || OUT=$("$CUBALC" forms PICKLIB 2>&1)
printf '%s\n' "$OUT" | grep -qi 'PICKLIB'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -q 'PICKLIB'

OUT=$("$CUBALC" run -q programs/proof/1307_picklib.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1307_cli_picklib: PASS"
