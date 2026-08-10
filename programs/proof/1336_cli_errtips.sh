#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" errtips INCLUDE missing foo 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.errtips.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"topic":"lib"'
printf '%s\n' "$OUT" | grep -qE '"n":[1-9]'

OUT=$("$CUBALC" errtips "unknown form ZZZ" 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"cap"'

OUT=$("$CUBALC" fixtips "SMX DIAL timeout" 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"p2p"'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'errtips'

OUT=$("$CUBALC" forms ERRTIPS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'ERRTIPS'

OUT=$("$CUBALC" run -q -e 'FAIL "ASSERT is false"
ERRTIPS
ASSERT ERRTIPS_TOPIC == "run"
ASSERT ERRTIPS_N >= 2
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1336_cli_errtips: PASS"
