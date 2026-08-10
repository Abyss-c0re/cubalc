#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'INCLUDE ALL MATCH guard
ASSERT INCLUDE_ALL_N == 2
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q 'time_guard\|var_guard'

OUT=$("$CUBALC" run -q -e 'INCLUDE SOFT ALL MATCH missing_qqq
ASSERT OK == 0
CLEAR_ERR
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'INCLUDE ALL MATCH missing DEFAULT hold_seed
ASSERT INCLUDE_ALL_FALLBACK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms include 2>&1) || OUT=$("$CUBALC" forms INCLUDE 2>&1)
printf '%s\n' "$OUT" | grep -qi 'ALL\|MATCH\|INCLUDE'

OUT=$("$CUBALC" run -q programs/proof/1316_include_all_match.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1316_cli_include_all_match: PASS"
