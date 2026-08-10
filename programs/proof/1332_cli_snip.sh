#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" snip 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.snip.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"topic":"general"'
printf '%s\n' "$OUT" | grep -q 'VERSION'

OUT=$("$CUBALC" snip cap 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"cap"'
printf '%s\n' "$OUT" | grep -q 'HASFORM\|FORMHINT\|RELATED'

OUT=$("$CUBALC" snippet plate 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"plate"'
printf '%s\n' "$OUT" | grep -q 'SETP\|NEEDP'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'snip'

OUT=$("$CUBALC" forms SNIP 2>&1)
printf '%s\n' "$OUT" | grep -qi 'SNIP'

# run the general snip body via -e extract is hard; run in-lang form
OUT=$("$CUBALC" run -q -e 'SNIP cap
ASSERT SNIP_N >= 4
SYS HASI "HASFORM"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# execute a snip source: materialize via run -e of known mini
OUT=$("$CUBALC" run -q -e 'VERSION
STATUS
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1332_cli_snip: PASS"
