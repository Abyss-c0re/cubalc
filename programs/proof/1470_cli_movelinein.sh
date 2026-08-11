#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1470_movelinein.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms MOVELINEIN 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"MOVELINEIN"|MOVELINEIN name'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
MOVELINEIN
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'MOVELINEIN needs|needs name'

# regression insertlinein
OUT=$("$CUBALC" run -q programs/proof/1469_insertlinein.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1470_cli_movelinein: PASS"
