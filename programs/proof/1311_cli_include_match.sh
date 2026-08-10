#!/bin/sh
# INCLUDE MATCH plate + DEFAULT + SOFT + forms
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'INCLUDE MATCH agent_boot
ASSERT OK == 1
ASSERT INCLUDE_MATCH == "agent_boot"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q 'agent_boot'

OUT=$("$CUBALC" run -q -e 'INCLUDE ONCE MATCH plate
ASSERT INCLUDE_MATCH == "plate_boot"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'INCLUDE ONCE MATCH missing_qqq DEFAULT hold_seed
ASSERT INCLUDE_MATCH == "hold_seed"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'INCLUDE SOFT MATCH missing_qqq
ASSERT OK == 0
CLEAR_ERR
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

set +e
OUT=$("$CUBALC" run -q -e 'INCLUDE MATCH no_such_lib_xyz_zzz
PASS' 2>&1)
set -e
printf '%s\n' "$OUT" | grep -qi 'INCLUDE MATCH miss\|"ok":false'

OUT=$("$CUBALC" forms include 2>&1) || OUT=$("$CUBALC" forms INCLUDE 2>&1)
printf '%s\n' "$OUT" | grep -qi 'MATCH\|INCLUDE'

OUT=$("$CUBALC" run -q programs/proof/1311_include_match.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1311_cli_include_match: PASS"
