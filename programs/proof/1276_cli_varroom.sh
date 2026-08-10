#!/bin/sh
# VARROOM / NEEDVARROOM CLI smoke
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"

OUT=$("$CUBALC" run -q -e 'VARROOM
ASSERT LAST_N > 100
NEEDVARROOM 50
ASSERT OK == 1
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# NEEDVARROOM fails when asking for more than max
OUT=$("$CUBALC" run -q -e 'NEEDVARROOM 9999' 2>&1 || true)
printf '%s\n' "$OUT" | grep -qi 'NEEDVARROOM\|need '
printf '%s\n' "$OUT" | grep -q '"ok":false\|"ok": false' || printf '%s\n' "$OUT" | grep -qi 'fail\|error\|NEEDVARROOM'

# forms catalog
OUT=$("$CUBALC" forms VARROOM 2>&1)
printf '%s\n' "$OUT" | grep -q 'VARROOM'
OUT=$("$CUBALC" forms NEEDVARROOM 2>&1)
printf '%s\n' "$OUT" | grep -q 'NEEDVARROOM'

echo "1276_cli_varroom: PASS"
