#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1447_retry.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms RETRY 2>&1)
printf '%s\n' "$OUT" | grep -qi RETRY

# missing END
TMP=$(mktemp)
trap 'rm -f "$TMP"' EXIT
cat > "$TMP" << 'C'
HOLD_FLASH 1
RETRY 2
  PASS
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMP" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'RETRY without END|without END'

# regression TIMEIT
OUT=$("$CUBALC" run -q programs/proof/1446_timeit.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1447_cli_retry: PASS"
