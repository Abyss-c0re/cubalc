#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1452_inc_dec_swap.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms INC 2>&1)
printf '%s\n' "$OUT" | grep -qi INC

OUT=$("$CUBALC" forms SWAP 2>&1)
printf '%s\n' "$OUT" | grep -qi SWAP

# bad arity
set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
INC
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'INC needs|needs name'

# regression IFERR
OUT=$("$CUBALC" run -q programs/proof/1451_iferr_ifok.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1452_cli_inc_dec_swap: PASS"
