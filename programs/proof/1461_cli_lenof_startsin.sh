#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1461_lenof_startsin.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms LENOF 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"LENOF"|LENOF name'

OUT=$("$CUBALC" forms STARTSIN 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"STARTSIN"|STARTSIN name'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
LENOF
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'LENOF needs|needs name'

# regression COALESCETO
OUT=$("$CUBALC" run -q programs/proof/1460_coalesceto_nthin.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1461_cli_lenof_startsin: PASS"
