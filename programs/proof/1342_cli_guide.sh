#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" guide cap 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.guide.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"topic":"cap"'
printf '%s\n' "$OUT" | grep -q 'HASFORM\|NEEDFORMS'
printf '%s\n' "$OUT" | grep -q '"related"'
printf '%s\n' "$OUT" | grep -q '"general"'

OUT=$("$CUBALC" playguide plate 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"plate"'
printf '%s\n' "$OUT" | grep -q 'SETP\|NEEDP'

OUT=$("$CUBALC" guide 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"general"'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'guide'

OUT=$("$CUBALC" forms GUIDE 2>&1)
printf '%s\n' "$OUT" | grep -qi 'GUIDE'

OUT=$("$CUBALC" run -q -e 'GUIDE lib
ASSERT OK == 1
ASSERT GUIDE_RELATED_N >= 3
SYS HASI "LISTLIBS"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" guide zzz 2>&1)
rc=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$rc" -ne 0

echo "1342_cli_guide: PASS"
