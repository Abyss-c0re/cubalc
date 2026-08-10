#!/bin/sh
# HASVARROOM soft free-slot probe
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"

OUT=$("$CUBALC" run -q -e 'HASVARROOM 20
ASSERT LAST_N == 1
ASSERT OK == 1
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft miss does not fail plate unless -s strict
OUT=$("$CUBALC" run -q -e 'HASVARROOM 9999
ASSERT LAST_N == 0
CLEAR_ERR
PASS
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# forms
OUT=$("$CUBALC" forms HASVARROOM 2>&1)
printf '%s\n' "$OUT" | grep -q 'HASVARROOM'

echo "1277_cli_hasvarroom: PASS"
