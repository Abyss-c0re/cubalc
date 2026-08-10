#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" ready 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.ready.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" proveready 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.ready.v1"'

OUT=$("$CUBALC" run -q -e 'READY
ASSERT READY_OK == 1
ASSERT READY_FAIL_N == 0
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms READY 2>&1)
printf '%s\n' "$OUT" | grep -qi 'READY'

OUT=$("$CUBALC" forms NEEDREADY 2>&1)
printf '%s\n' "$OUT" | grep -qi 'NEEDREADY'

OUT=$("$CUBALC" run -q programs/proof/1359_ready.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'ready'

echo "1359_cli_ready: PASS"
