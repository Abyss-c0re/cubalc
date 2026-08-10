#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" errrun INCLUDE missing foo 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.errrun.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"topic":"lib"'

OUT=$("$CUBALC" errrun "unknown form Z" 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"cap"'
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" recoversnip "SMX DIAL timeout" 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"p2p"'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'errrun'

OUT=$("$CUBALC" forms ERRRUN 2>&1)
printf '%s\n' "$OUT" | grep -qi 'ERRRUN'

OUT=$("$CUBALC" run -q -e 'FAIL "INCLUDE x"
ERRRUN
ASSERT ERRRUN_TOPIC == "lib"
ASSERT ERRRUN_OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1337_cli_errrun: PASS"
