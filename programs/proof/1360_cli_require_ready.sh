#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

# host prove floor passes on healthy tree
OUT=$("$CUBALC" run -q -Y -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"require_ready":true'

# long flags
OUT=$("$CUBALC" run -q --need-ready -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q --require-ready -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# env dual
OUT=$(CUBALC_REQUIRE_READY=1 "$CUBALC" run -q -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"require_ready":true'

# compose with -D (preflight both floors; body is clean)
OUT=$("$CUBALC" run -q -D -Y -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"require_doctor":true'
printf '%s\n' "$OUT" | grep -q '"require_ready":true'

# help surfaces -Y
OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -q -- '-Y'

# env catalog
OUT=$("$CUBALC" env 2>&1)
printf '%s\n' "$OUT" | grep -q 'CUBALC_REQUIRE_READY'

# in-lang NEEDREADY still works under floor
OUT=$("$CUBALC" run -q -Y -e 'NEEDREADY
ASSERT READY_OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# without -Y plate shows require_ready false
OUT=$("$CUBALC" run -q -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"require_ready":false'

echo "1360_cli_require_ready: PASS"
