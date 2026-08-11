#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1467_ifstarts_ifcontains.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms IFSTARTS 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"IFSTARTS"|IFSTARTS name'

OUT=$("$CUBALC" forms IFCONTAINS 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"IFCONTAINS"|IFCONTAINS name'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
IFSTARTS
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'IFSTARTS needs|needs name'

# regression IFEMPTY + LPADTO
OUT=$("$CUBALC" run -q programs/proof/1457_ifempty_ifdefined.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
OUT=$("$CUBALC" run -q programs/proof/1466_lpadto_rpadto.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1467_cli_ifstarts_ifcontains: PASS"
