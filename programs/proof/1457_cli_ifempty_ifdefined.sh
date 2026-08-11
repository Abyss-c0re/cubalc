#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1457_ifempty_ifdefined.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms IFEMPTY 2>&1)
printf '%s\n' "$OUT" | grep -qiE '"name":"IFEMPTY"|IFEMPTY name'

set +e
OUT=$("$CUBALC" run -q -e 'HOLD_FLASH 1
IFEMPTY
  PASS
END
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'IFEMPTY needs|needs name'

# regression PUSHTO bags
OUT=$("$CUBALC" run -q programs/proof/1456_pushto_bag.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# regression IFERR
OUT=$("$CUBALC" run -q programs/proof/1451_iferr_ifok.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1457_cli_ifempty_ifdefined: PASS"
