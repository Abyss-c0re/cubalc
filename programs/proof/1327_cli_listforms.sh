#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" countforms SORT 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.formlist.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -qE '"n":[1-9]'

OUT=$("$CUBALC" listforms LIB 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"prefix":"LIB"'

OUT=$("$CUBALC" forms SORTLIBS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'SORTLIBS'

OUT=$("$CUBALC" run -q -e 'LISTFORMS SORT
ASSERT LAST_N >= 3
SYS HASLINE LAST "SORTLIBS"
ASSERT LAST_N == 1
COUNTFORMS FORM
ASSERT LAST_N >= 5
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'LISTFORMS\|listforms'

echo "1327_cli_listforms: PASS"
