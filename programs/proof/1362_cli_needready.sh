#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" needready 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.ready.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"cmd":"needready"'

OUT=$("$CUBALC" need-ready 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'NEEDREADY
ASSERT READY_OK == 1
ASSERT READY_FAIL_N == 0
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q programs/proof/1362_needready.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms NEEDREADY 2>&1)
printf '%s\n' "$OUT" | grep -qi 'NEEDREADY'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'needready\|ready'

echo "1362_cli_needready: PASS"
