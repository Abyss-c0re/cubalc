#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1466_lpadto_rpadto.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms LPADTO 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"LPADTO"|LPADTO name'

OUT=$("$CUBALC" forms RPADTO 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"RPADTO"|RPADTO name'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
LPADTO
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'LPADTO needs|needs name'

# regression NUMTO
OUT=$("$CUBALC" run -q programs/proof/1465_numto_strto.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1466_cli_lpadto_rpadto: PASS"
