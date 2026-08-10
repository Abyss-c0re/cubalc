#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'INCLUDE ONCE NEWEST fat
ASSERT INCLUDE_MATCH == "fat_session"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'INCLUDE ONCE OLDEST fat
ASSERT INCLUDE_MATCH == "fat_boot"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'INCLUDE ONCE NEWEST miss DEFAULT hold_seed
ASSERT INCLUDE_MATCH == "hold_seed"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms include 2>&1) || OUT=$("$CUBALC" forms INCLUDE 2>&1)
printf '%s\n' "$OUT" | grep -qi 'NEWEST\|OLDEST\|MATCH\|INCLUDE'

OUT=$("$CUBALC" run -q programs/proof/1318_include_newest.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1318_cli_include_newest: PASS"
