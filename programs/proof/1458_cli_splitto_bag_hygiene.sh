#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1458_splitto_bag_hygiene.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms SPLITTO 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"SPLITTO"|SPLITTO name'

OUT=$("$CUBALC" forms GREPIN 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"GREPIN"|GREPIN\|KEEPIN'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
SPLITTO
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'SPLITTO needs|needs name'

# regression IFEMPTY
OUT=$("$CUBALC" run -q programs/proof/1457_ifempty_ifdefined.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# regression PUSHTO
OUT=$("$CUBALC" run -q programs/proof/1456_pushto_bag.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1458_cli_splitto_bag_hygiene: PASS"
