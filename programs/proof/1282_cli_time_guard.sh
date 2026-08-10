#!/bin/sh
# INCLUDE time_guard + doctor lib_time_guard + which/cat/libs
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

# unlimited include
OUT=$("$CUBALC" run -q -e 'INCLUDE time_guard
ASSERT OK == 1
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q 'time_guard'

# under -T: soft miss then hard fail path
OUT=$("$CUBALC" run -q -T 120 -e 'DEFAULT NEED_TIME = 10
INCLUDE time_guard
ASSERT OK == 1
SYS SLEEP 40
LET NEED_TIME = 999999
LET TIME_GUARD_SOFT = 1
INCLUDE time_guard
ASSERT LAST_N == 0
ASSERT OK == 0
CLEAR_ERR
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -T 80 -e 'SYS SLEEP 40
DEFAULT NEED_TIME = 999999
INCLUDE time_guard
PASS' 2>&1) || true
printf '%s\n' "$OUT" | grep -q '"ok":false'
printf '%s\n' "$OUT" | grep -qi 'NEEDTIME\|need .*ms'

# which / cat / libs
OUT=$("$CUBALC" which time_guard 2>&1)
printf '%s\n' "$OUT" | grep -q 'time_guard'
OUT=$("$CUBALC" cat time_guard 2>&1)
printf '%s\n' "$OUT" | grep -q 'NEEDTIME\|NEED_TIME'
OUT=$("$CUBALC" libs 2>&1)
printf '%s\n' "$OUT" | grep -q 'time_guard'

# doctor
OUT=$("$CUBALC" doctor)
printf '%s\n' "$OUT" | grep -q '"lib_time_guard":true'
n=$(printf '%s\n' "$OUT" | sed -n 's/.*"libs_n":\([0-9][0-9]*\).*/\1/p' | head -1)
test -n "$n"
test "$n" -ge 18

# file proof
OUT=$("$CUBALC" run -q programs/proof/1282_time_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1282_cli_time_guard: PASS"
