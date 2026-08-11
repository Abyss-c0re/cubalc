#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1446_timeit.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# HELP/forms discover
OUT=$("$CUBALC" forms TIMEIT 2>&1)
printf '%s\n' "$OUT" | grep -qi TIMEIT

OUT=$("$CUBALC" run -q -e 'TIMEIT
  SYS SLEEP 15
END
ASSERT TIMEIT_MS >= 10
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# missing END fails
TMP=$(mktemp)
trap 'rm -f "$TMP"' EXIT
cat > "$TMP" << 'C'
HOLD_FLASH 1
TIMEIT
  LET a = 1
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMP" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'TIMEIT without END|without END'

echo "1446_cli_timeit: PASS"
