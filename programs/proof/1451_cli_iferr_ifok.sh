#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1451_iferr_ifok.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms IFERR 2>&1)
printf '%s\n' "$OUT" | grep -qi IFERR

OUT=$("$CUBALC" forms HASOK 2>&1)
printf '%s\n' "$OUT" | grep -qi HASOK

# missing END
set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
FAIL "a"
IFERR
  PASS
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'IFERR without END|without END'

# regression REQUIRE OK
OUT=$("$CUBALC" run -q programs/proof/1450_require_ok.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1451_cli_iferr_ifok: PASS"
