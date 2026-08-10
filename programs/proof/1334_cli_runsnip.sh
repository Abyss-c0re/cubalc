#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" runsnip 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.runsnip.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"topic":"general"'

OUT=$("$CUBALC" runsnip cap 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"cap"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -qE '"asserts_ok":[0-9]'

OUT=$("$CUBALC" sniprun plate 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"plate"'
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'runsnip'

OUT=$("$CUBALC" forms RUNSNIP 2>&1)
printf '%s\n' "$OUT" | grep -qi 'RUNSNIP'

OUT=$("$CUBALC" run -q -e 'RUNSNIP cap
ASSERT RUNSNIP_OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1334_cli_runsnip: PASS"
