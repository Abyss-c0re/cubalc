#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1472_ifgt_ifeqn.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms IFGT 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"IFGT"|IFGT name'

OUT=$("$CUBALC" forms IFEQN 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"IFEQN"|IFEQN name'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
IFGT
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'IFGT needs|needs name'

# regression IFSTARTS + COUNTIN
OUT=$("$CUBALC" run -q programs/proof/1467_ifstarts_ifcontains.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
OUT=$("$CUBALC" run -q programs/proof/1471_countin_countlinein.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1472_cli_ifgt_ifeqn: PASS"
