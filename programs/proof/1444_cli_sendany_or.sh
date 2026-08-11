#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1444_sendany_or.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard SENDANY without OR still fatal
TMP=$(mktemp)
trap 'rm -f "$TMP"' EXIT
cat > "$TMP" << 'C'
HOLD_FLASH 1
CLASS C
  FIELD n 0
END
NEW C o
SENDANY o ghost_a ghost_b
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMP" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'SENDANY miss|none of|ghost'

# regression SENDANY base
OUT=$("$CUBALC" run -q programs/proof/1439_sendany.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# regression CALLANY OR
OUT=$("$CUBALC" run -q programs/proof/1443_callany_or.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1444_cli_sendany_or: PASS"
