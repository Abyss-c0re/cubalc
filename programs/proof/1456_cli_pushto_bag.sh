#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1456_pushto_bag.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms PUSHTO 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"PUSHTO"|PUSHTO name line'

OUT=$("$CUBALC" forms HASINBAG 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"HASINBAG"|HASINBAG name'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
PUSHTO
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'PUSHTO needs|needs name'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
POPFROM
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'POPFROM needs|needs name'

# regression REPLACEIN family
OUT=$("$CUBALC" run -q programs/proof/1455_replacein_trimto.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1456_cli_pushto_bag: PASS"
