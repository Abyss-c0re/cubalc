#!/bin/sh
# REMAIN_MS / HAS_TIME / NEEDTIME under -T + forms + doctor
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -T 2000 -e 'REMAIN_MS
ASSERT LAST_N > 0
ASSERT LAST_N <= 2000
ASSERT TIMEOUT_UNLIMITED == 0
LET R0 = LAST_N
SYS SLEEP 50
REMAIN_MS
ASSERT LAST_N < R0
ASSERT LAST_N > 0
HAS_TIME 10
ASSERT LAST_N == 1
ASSERT OK == 1
NEEDTIME 5
ASSERT OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -T 100 -e 'SYS SLEEP 30
HAS_TIME 999999
ASSERT LAST_N == 0
ASSERT OK == 0
CLEAR_ERR
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -T 80 -e 'SYS SLEEP 40
NEEDTIME 999999
PASS' 2>&1) || true
printf '%s\n' "$OUT" | grep -q '"ok":false'
printf '%s\n' "$OUT" | grep -qi 'NEEDTIME'

OUT=$("$CUBALC" run -q -e 'REMAIN_MS
ASSERT LAST_N == -1
HAS_TIME 1
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms remain 2>&1)
printf '%s\n' "$OUT" | grep -qi 'REMAIN_MS'

OUT=$("$CUBALC" doctor)
printf '%s\n' "$OUT" | grep -q '"remain_ms_forms":true'

OUT=$("$CUBALC" run -q programs/proof/1281_remain_ms.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1281_cli_remain_ms: PASS"
