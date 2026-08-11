#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1463_stripprefixto.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms STRIPPREFIXTO 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"STRIPPREFIXTO"|STRIPPREFIXTO name'

OUT=$("$CUBALC" forms STRIPSUFFIXTO 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"STRIPSUFFIXTO"|STRIPSUFFIXTO name'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
STRIPPREFIXTO
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'STRIPPREFIXTO needs|needs name'

# regression LEFTTO peels
OUT=$("$CUBALC" run -q programs/proof/1462_leftto_beforein.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1463_cli_stripprefixto: PASS"
