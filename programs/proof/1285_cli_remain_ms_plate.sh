#!/bin/sh
# run plate remain_ms dual of REMAIN_MS · STATUS timeout_ms/remain_ms
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

# unlimited: remain_ms = -1
OUT=$("$CUBALC" run -q -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"remain_ms":-1'
printf '%s\n' "$OUT" | grep -q '"timeout_ms":0\|"timeout_ms":0,'

# under -T: remain_ms positive and <= budget after short sleep
OUT=$("$CUBALC" run -q -T 1500 -e 'SYS SLEEP 40
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"timeout_ms":1500'
# remain_ms between 1 and 1500
rem=$(printf '%s\n' "$OUT" | sed -n 's/.*"remain_ms":\(-*[0-9][0-9]*\).*/\1/p' | head -1)
test -n "$rem"
test "$rem" -ge 1
test "$rem" -le 1500

# timed_out → remain_ms 0
OUT=$("$CUBALC" run -q -T 60 -e 'SYS SLEEP 40
SYS SLEEP 40
PASS' 2>&1) || true
printf '%s\n' "$OUT" | grep -q '"timed_out":true'
printf '%s\n' "$OUT" | grep -q '"remain_ms":0'

# STATUS in-lang under budget
OUT=$("$CUBALC" run -q -T 2000 -e 'SYS SLEEP 20
STATUS
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
# last_print or body may embed status — plate remain_ms still present
printf '%s\n' "$OUT" | grep -q '"remain_ms":'

# STATUS unlimited
OUT=$("$CUBALC" run -q -e 'STATUS
ASSERT REMAIN_MS == -1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"remain_ms":-1'

echo "1285_cli_remain_ms_plate: PASS"
