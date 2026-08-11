#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1442_send_or.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard SEND without OR still fatal
TMP=$(mktemp)
trap 'rm -f "$TMP"' EXIT
cat > "$TMP" << 'C'
HOLD_FLASH 1
CLASS Cell
  FIELD n 0
  METHOD tick
    RET 1
  END
END
NEW Cell c
SEND c ghost_only
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMP" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'unknown METHOD|SEND|ghost'

# regressions
OUT=$("$CUBALC" run -q programs/proof/876_oop_trysend.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q programs/proof/1441_call_or.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q programs/proof/1439_sendany.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1442_cli_send_or: PASS"
