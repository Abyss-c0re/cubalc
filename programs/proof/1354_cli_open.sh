#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" open 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.open.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"first":"general"'
printf '%s\n' "$OUT" | grep -q '"n":8'

OUT=$("$CUBALC" open p 2>&1)
printf '%s\n' "$OUT" | grep -q '"first":"cap"'
printf '%s\n' "$OUT" | grep -q '"filter":"p"'
printf '%s\n' "$OUT" | grep -q '"n":4'
printf '%s\n' "$OUT" | grep -q 'HASFORM\|capability'

OUT=$("$CUBALC" openplay plate 2>&1)
printf '%s\n' "$OUT" | grep -q '"first":"plate"'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'open'

OUT=$("$CUBALC" forms OPEN 2>&1)
printf '%s\n' "$OUT" | grep -qi 'OPEN'

OUT=$("$CUBALC" run -q -e 'OPEN p
ASSERT OK == 1
ASSERT OPEN_FIRST == "cap"
ASSERT GUIDE_RELATED_N >= 3
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" open zzz 2>&1)
rc=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$rc" -ne 0

echo "1354_cli_open: PASS"
