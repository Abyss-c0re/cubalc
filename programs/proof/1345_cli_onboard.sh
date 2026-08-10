#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" onboard 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.start.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"topics_n":8'
printf '%s\n' "$OUT" | grep -q '"general"'
printf '%s\n' "$OUT" | grep -q 'cubalc doctor'
printf '%s\n' "$OUT" | grep -q 'agent_boot'

OUT=$("$CUBALC" welcome 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.start.v1"'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'onboard'

OUT=$("$CUBALC" forms START 2>&1)
printf '%s\n' "$OUT" | grep -qi 'START'

OUT=$("$CUBALC" run -q -e 'START
ASSERT OK == 1
ASSERT START_TOPICS_N == 8
ASSERT START_NEXT_N >= 8
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# cookbook start still works (legacy)
OUT=$("$CUBALC" start 2>&1)
printf '%s\n' "$OUT" | grep -qi 'cookbook\|COOKBOOK\|onboard'

echo "1345_cli_onboard: PASS"
