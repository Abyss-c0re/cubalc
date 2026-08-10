#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" errguide INCLUDE missing foo 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.errguide.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"topic":"lib"'
printf '%s\n' "$OUT" | grep -q 'LISTLIBS\|RECIPE\|INCLUDE'
printf '%s\n' "$OUT" | grep -q '"related"'

OUT=$("$CUBALC" errguide "unknown form Z" 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"cap"'
printf '%s\n' "$OUT" | grep -q 'HASFORM\|NEEDFORMS'

OUT=$("$CUBALC" recoverguide "SMX DIAL timeout" 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"p2p"'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'errguide'

OUT=$("$CUBALC" forms ERRGUIDE 2>&1)
printf '%s\n' "$OUT" | grep -qi 'ERRGUIDE'

OUT=$("$CUBALC" run -q -e 'FAIL "INCLUDE x"
ERRGUIDE
ASSERT ERRGUIDE_TOPIC == "lib"
ASSERT GUIDE_RELATED_N >= 3
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1343_cli_errguide: PASS"
