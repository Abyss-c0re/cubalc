#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1468_findlinein_setlinein.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms FINDLINEIN 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"FINDLINEIN"|FINDLINEIN name'

OUT=$("$CUBALC" forms SETLINEIN 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"SETLINEIN"|SETLINEIN name'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
FINDLINEIN
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'FINDLINEIN needs|needs name'

# regression bag set ops
OUT=$("$CUBALC" run -q programs/proof/1464_ensurein_droplinein.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1468_cli_findlinein_setlinein: PASS"
