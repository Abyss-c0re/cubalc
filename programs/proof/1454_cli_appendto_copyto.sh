#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1454_appendto_copyto.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms APPENDTO 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"APPENDTO"|APPENDTO\|CATTO'

OUT=$("$CUBALC" forms COPYTO 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"COPYTO"|COPYTO\|COPYV'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
APPENDTO
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'APPENDTO needs|needs name'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
COPYTO only
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'COPYTO needs|needs dst'

# regression TOGGLE family
OUT=$("$CUBALC" run -q programs/proof/1453_toggle_clamp_setminmax.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1454_cli_appendto_copyto: PASS"
