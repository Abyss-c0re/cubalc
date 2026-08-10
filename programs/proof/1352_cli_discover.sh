#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" discover 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.discover.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"first":"general"'
printf '%s\n' "$OUT" | grep -q '"n":8'
printf '%s\n' "$OUT" | grep -q '"cap"'

OUT=$("$CUBALC" discover p 2>&1)
printf '%s\n' "$OUT" | grep -q '"first":"cap"'
printf '%s\n' "$OUT" | grep -q '"filter":"p"'
printf '%s\n' "$OUT" | grep -q '"n":4'
printf '%s\n' "$OUT" | grep -q 'HASFORM\|capability'
printf '%s\n' "$OUT" | grep -q 'GUIDE cap\|RUNSNIP'

OUT=$("$CUBALC" explore plate 2>&1)
printf '%s\n' "$OUT" | grep -q '"first":"plate"'

OUT=$("$CUBALC" discover mesh 2>&1)
printf '%s\n' "$OUT" | grep -q '"first":"p2p"'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'discover'

OUT=$("$CUBALC" forms DISCOVER 2>&1)
printf '%s\n' "$OUT" | grep -qi 'DISCOVER'

OUT=$("$CUBALC" run -q -e 'DISCOVER p
ASSERT OK == 1
ASSERT DISCOVER_FIRST == "cap"
ASSERT DISCOVER_N == 4
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" discover zzz 2>&1)
rc=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$rc" -ne 0

echo "1352_cli_discover: PASS"
