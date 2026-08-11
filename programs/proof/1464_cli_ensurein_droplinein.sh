#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1464_ensurein_droplinein.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms ENSUREIN 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"ENSUREIN"|ENSUREIN name'

OUT=$("$CUBALC" forms DROPLINEIN 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"DROPLINEIN"|DROPLINEIN name'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
ENSUREIN
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'ENSUREIN needs|needs name'

# regression strip + bag
OUT=$("$CUBALC" run -q programs/proof/1463_stripprefixto.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
OUT=$("$CUBALC" run -q programs/proof/1456_pushto_bag.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1464_cli_ensurein_droplinein: PASS"
