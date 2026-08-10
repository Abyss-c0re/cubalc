#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'INCLUDE ONCE NTH 0 agent
ASSERT INCLUDE_MATCH == "agent_boot"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'INCLUDE ONCE NTH 1 fat
ASSERT INCLUDE_MATCH == "fat_session"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'INCLUDE ONCE LASTMATCH plate
ASSERT INCLUDE_MATCH == "plate_uniform"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'INCLUDE ONCE NTH 50 fat DEFAULT hold_seed
ASSERT INCLUDE_MATCH == "hold_seed"
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms include 2>&1) || OUT=$("$CUBALC" forms INCLUDE 2>&1)
printf '%s\n' "$OUT" | grep -qi 'NTH\|LASTMATCH\|MATCH'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -q 'NTH\|LASTMATCH\|MATCH'

OUT=$("$CUBALC" run -q programs/proof/1312_include_nth_last.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1312_cli_include_nth_last: PASS"
