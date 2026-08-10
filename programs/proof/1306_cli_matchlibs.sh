#!/bin/sh
# MATCHLIBS dual of cubalc libs [filter]
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'MATCHLIBS plate
ASSERT OK == 1
ASSERT LAST_N >= 10
SYS HASLINE LAST "plate_tick"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'FILTERLIBS fat
ASSERT LAST_N == 2
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'MATCHLIBS no_such_qqq
ASSERT OK == 1
ASSERT LAST_N == 0
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms matchlibs 2>&1) || OUT=$("$CUBALC" forms MATCHLIBS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'MATCHLIBS'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -q 'MATCHLIBS'

OUT=$("$CUBALC" run -q programs/proof/1306_matchlibs.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1306_cli_matchlibs: PASS"
