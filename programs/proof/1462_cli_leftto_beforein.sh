#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1462_leftto_beforein.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms LEFTTO 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"LEFTTO"|LEFTTO name'

OUT=$("$CUBALC" forms BEFOREIN 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"BEFOREIN"|BEFOREIN name'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
LEFTTO
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'LEFTTO needs|needs name'

# regression LENOF
OUT=$("$CUBALC" run -q programs/proof/1461_lenof_startsin.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1462_cli_leftto_beforein: PASS"
