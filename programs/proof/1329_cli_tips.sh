#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" tips 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.tips.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"topic":"general"'
printf '%s\n' "$OUT" | grep -qE '"n":[1-9]'

OUT=$("$CUBALC" tips cap 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"cap"'
printf '%s\n' "$OUT" | grep -q 'cap_boot\|HASFORM\|REQUIRE_FORMS\|LISTFORMS'

OUT=$("$CUBALC" howto fat 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"fat"'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'tips'

OUT=$("$CUBALC" forms TIPS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'TIPS'

OUT=$("$CUBALC" run -q -e 'TIPS cap
ASSERT TIPS_N >= 3
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1329_cli_tips: PASS"
