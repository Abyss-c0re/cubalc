#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" topichint cap 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.topichint.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"topic":"cap"'
printf '%s\n' "$OUT" | grep -q 'HASFORM\|NEEDFORMS'

OUT=$("$CUBALC" describetopic plate 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"plate"'
printf '%s\n' "$OUT" | grep -q 'SETP\|NEEDP'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'topichint'

OUT=$("$CUBALC" forms TOPICHINT 2>&1)
printf '%s\n' "$OUT" | grep -qi 'TOPICHINT'

OUT=$("$CUBALC" run -q -e 'TOPICHINT lib
ASSERT OK == 1
SYS HASI "LISTLIBS"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" topichint zzz 2>&1)
rc=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$rc" -ne 0

echo "1339_cli_topichint: PASS"
