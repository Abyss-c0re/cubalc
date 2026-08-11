#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1441_call_or.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard CALL without OR still fatal
TMP=$(mktemp)
trap 'rm -f "$TMP"' EXIT
cat > "$TMP" << 'C'
HOLD_FLASH 1
CALL ghost_only_fn
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMP" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'unknown FN|CALL|ghost'

# regression TRYCALL base
OUT=$("$CUBALC" run -q programs/proof/932_fn_trycall.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# regression CALLANY
OUT=$("$CUBALC" run -q programs/proof/1438_callany.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1441_cli_call_or: PASS"
