#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1459_mul_divby_abs.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms MUL 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"MUL"|MUL\|MULTO'

OUT=$("$CUBALC" forms DIVBY 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"DIVBY"|DIVBY name'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
MUL
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'MUL needs|needs name'

# regression SPLITTO
OUT=$("$CUBALC" run -q programs/proof/1458_splitto_bag_hygiene.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# regression INC
OUT=$("$CUBALC" run -q programs/proof/1452_inc_dec_swap.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1459_cli_mul_divby_abs: PASS"
