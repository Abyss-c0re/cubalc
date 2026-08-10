#!/bin/sh
# run plate vars_n / vars_max / vars_full — dual of STATUS pressure fields
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"

# empty-ish program still allocates some specials
OUT=$("$CUBALC" run -q -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"vars_max":256\|"vars_max": 256'
printf '%s\n' "$OUT" | grep -q '"vars_full":false'
printf '%s\n' "$OUT" | grep -q '"vars_n":'

# LETs increase vars_n
OUT=$("$CUBALC" run -q -e 'LET a = 1
LET b = 2
LET c = 3
PASS
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"vars_full":false'
# at least a,b,c present
N=$(printf '%s\n' "$OUT" | sed -n 's/.*"vars_n":\([0-9]*\).*/\1/p' | head -1)
test -n "$N"
test "$N" -ge 3

# agrees with in-lang STATUS
OUT=$("$CUBALC" run -q -e 'LET x = 9
STATUS
ASSERT VARS_MAX >= 256
ASSERT VARS_FULL == 0
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"vars_max":256\|"vars_max": 256'
printf '%s\n' "$OUT" | grep -q '"vars_full":false'

echo "1275_cli_vars_plate: PASS"
