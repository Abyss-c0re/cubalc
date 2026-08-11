#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1469_insertlinein.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms INSERTLINEIN 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"INSERTLINEIN"|INSERTLINEIN name'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
INSERTLINEIN
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'INSERTLINEIN needs|needs name'

# regression findlinein
OUT=$("$CUBALC" run -q programs/proof/1468_findlinein_setlinein.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1469_cli_insertlinein: PASS"
