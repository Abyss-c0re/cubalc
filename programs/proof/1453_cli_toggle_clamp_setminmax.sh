#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1453_toggle_clamp_setminmax.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms TOGGLE 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"TOGGLE"|TOGGLE\|FLIP'

OUT=$("$CUBALC" forms SETMAX 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"SETMAX"|SETMAX\|MAXTO'

OUT=$("$CUBALC" forms CLAMP 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"CLAMP"|clamp var in place'

# bad arity
set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
TOGGLE
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'TOGGLE needs|needs name'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
CLAMP x 0
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'CLAMP needs|needs hi'

# regression INC/DEC/SWAP
OUT=$("$CUBALC" run -q programs/proof/1452_inc_dec_swap.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1453_cli_toggle_clamp_setminmax: PASS"
