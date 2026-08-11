#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1460_coalesceto_nthin.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms COALESCETO 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"COALESCETO"|COALESCETO\|NVLTO'

OUT=$("$CUBALC" forms NTHIN 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"NTHIN"|NTHIN name'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
COALESCETO
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'COALESCETO needs|needs name'

# regression MUL
OUT=$("$CUBALC" run -q programs/proof/1459_mul_divby_abs.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1460_cli_coalesceto_nthin: PASS"
