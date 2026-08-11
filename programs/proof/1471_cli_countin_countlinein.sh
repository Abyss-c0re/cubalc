#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1471_countin_countlinein.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms COUNTIN 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"COUNTIN"|COUNTIN name'

OUT=$("$CUBALC" forms COUNTLINEIN 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"COUNTLINEIN"|COUNTLINEIN name'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
COUNTIN
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'COUNTIN needs|needs name'

# regression movelinein
OUT=$("$CUBALC" run -q programs/proof/1470_movelinein.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1471_cli_countin_countlinein: PASS"
