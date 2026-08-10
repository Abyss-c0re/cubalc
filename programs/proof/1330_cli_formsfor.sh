#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" formsfor 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.formsfor.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"topic":"general"'
printf '%s\n' "$OUT" | grep -qE '"n":[1-9]'
printf '%s\n' "$OUT" | grep -q 'TIPS\|FORMSFOR\|VERSION'

OUT=$("$CUBALC" formsfor cap 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"cap"'
printf '%s\n' "$OUT" | grep -q 'HASFORM'
printf '%s\n' "$OUT" | grep -q 'NEEDFORMS\|FORMHINT\|LISTFORMS'

OUT=$("$CUBALC" topicforms fat 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"fat"'
printf '%s\n' "$OUT" | grep -q 'VARROOM\|REMAIN_MS'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'formsfor'

OUT=$("$CUBALC" forms FORMSFOR 2>&1)
printf '%s\n' "$OUT" | grep -qi 'FORMSFOR'

OUT=$("$CUBALC" run -q -e 'FORMSFOR cap
ASSERT FORMSFOR_N >= 5
SYS HASI "HASFORM"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1330_cli_formsfor: PASS"
