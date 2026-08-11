#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1439_sendany.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard SENDANY miss
cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
CLASS Cell
  FIELD n 0
  METHOD tick
    LET n = 1
  END
END
NEW Cell c
SENDANY c ghost_a ghost_b
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'SENDANY|none of|ghost'

# TRYSENDANY soft plate ok
cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
CLASS Cell
  FIELD n 0
  METHOD tick
    LET n = 1
  END
END
NEW Cell c
TRYSENDANY c ghost_a ghost_b
ASSERT OK == 0
ASSERT SENDANY_N == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# regression TRYSEND + CALLANY
OUT=$("$CUBALC" run -q programs/proof/876_oop_trysend.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q programs/proof/1438_callany.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms SENDANY 2>&1)
printf '%s\n' "$OUT" | grep -qi 'SENDANY'

echo "1439_cli_sendany: PASS"
