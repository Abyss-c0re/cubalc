#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1465_numto_strto.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms NUMTO 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"NUMTO"|NUMTO name'

OUT=$("$CUBALC" forms STRTO 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"STRTO"|STRTO name'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
NUMTO
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NUMTO needs|needs name'

# regression ensurein + peels
OUT=$("$CUBALC" run -q programs/proof/1464_ensurein_droplinein.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
OUT=$("$CUBALC" run -q programs/proof/1462_leftto_beforein.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1465_cli_numto_strto: PASS"
