#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1443_callany_or.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard CALLANY without OR still fatal
TMP=$(mktemp)
trap 'rm -f "$TMP"' EXIT
cat > "$TMP" << 'C'
HOLD_FLASH 1
CALLANY ghost_only_a ghost_only_b
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMP" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'CALLANY miss|none of|ghost'

# regression CALLANY base
OUT=$("$CUBALC" run -q programs/proof/1438_callany.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# regression CALL OR
OUT=$("$CUBALC" run -q programs/proof/1441_call_or.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1443_cli_callany_or: PASS"
