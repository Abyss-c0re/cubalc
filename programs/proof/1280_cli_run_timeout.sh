#!/bin/sh
# cubalc run -T / CUBALC_RUN_TIMEOUT wall kill + timed_out plate
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

# Fast path under budget: ok + timeout_ms on plate
OUT=$("$CUBALC" run -q -T 2000 -e 'ASSERT TIMEOUT_MS == 2000
ASSERT TIMED_OUT == 0
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"timeout_ms":2000'
printf '%s\n' "$OUT" | grep -q '"timed_out":false'

# Exceed budget with sleep loop — must fail timed_out
OUT=$("$CUBALC" run -q -T 80 -e 'LET I = 0
WHILE I < 50
  SYS SLEEP 40
  LET I = I + 1
END
PASS' 2>&1) || true
printf '%s\n' "$OUT" | grep -q '"ok":false'
printf '%s\n' "$OUT" | grep -q '"timed_out":true'
printf '%s\n' "$OUT" | grep -q '"timeout_ms":80'
printf '%s\n' "$OUT" | grep -qi 'TIMEOUT\|exceeded\|RUN_TIMEOUT'

# Env dual
OUT=$(CUBALC_RUN_TIMEOUT=1500 "$CUBALC" run -q -e 'ASSERT TIMEOUT_MS == 1500
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"timeout_ms":1500'

# env catalog
OUT=$("$CUBALC" env RUN_TIMEOUT 2>&1) || OUT=$("$CUBALC" env 2>&1)
printf '%s\n' "$OUT" | grep -q 'CUBALC_RUN_TIMEOUT'

# doctor advertises run_timeout
OUT=$("$CUBALC" doctor)
printf '%s\n' "$OUT" | grep -q '"run_timeout":true'

# file proof under budget
OUT=$(CUBALC_RUN_TIMEOUT=3000 "$CUBALC" run -q programs/proof/1280_run_timeout.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1280_cli_run_timeout: PASS"
