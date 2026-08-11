#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1414_ensurenew.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# ensure is idempotent across run statements
cat > "$TMPDIR/idem.cubalc" << 'C'
HOLD_FLASH 1
CLASS T
  FIELD n
END
ENSURENEW T a
ENSURENEW T a
ASSERT ENSURENEW_N == 0
ASSERT OK == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/idem.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft continues past unknown class
cat > "$TMPDIR/miss.cubalc" << 'C'
HOLD_FLASH 1
ENSURENEW Nope x
ASSERT OK == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/miss.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms ENSURENEW 2>&1)
printf '%s\n' "$OUT" | grep -qi ENSURENEW

echo "1414_cli_ensurenew: PASS"
