#!/bin/sh
# NTHLIB / LASTLIB plate + OR fallback + forms
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'NTHLIB 0 plate
ASSERT OK == 1
ASSERT LAST == "plate_boot"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'LASTLIB plate
ASSERT OK == 1
ASSERT LAST == "plate_uniform"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'NTHLIB 1 plate
ASSERT LAST == "plate_both_save"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'NTHLIB 50 fat OR agent_boot
ASSERT LAST == "agent_boot"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'NTHLIB 0 missing_qqq
ASSERT OK == 0
CLEAR_ERR
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms nthlib 2>&1) || OUT=$("$CUBALC" forms NTHLIB 2>&1)
printf '%s\n' "$OUT" | grep -qi 'NTHLIB'

OUT=$("$CUBALC" forms lastlib 2>&1) || OUT=$("$CUBALC" forms LASTLIB 2>&1)
printf '%s\n' "$OUT" | grep -qi 'LASTLIB'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -q 'NTHLIB\|LASTLIB'

OUT=$("$CUBALC" run -q programs/proof/1310_nthlib.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1310_cli_nthlib: PASS"
