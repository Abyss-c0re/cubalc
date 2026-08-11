#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1455_replacein_trimto.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms REPLACEIN 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"REPLACEIN"|REPLACEIN\|GSUBTO'

OUT=$("$CUBALC" forms ZERO 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"ZERO"|ZERO name'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
REPLACEIN
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'REPLACEIN needs|needs name'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
TRIMTO
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'TRIMTO needs|needs name'

# regression APPENDTO family
OUT=$("$CUBALC" run -q programs/proof/1454_appendto_copyto.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1455_cli_replacein_trimto: PASS"
