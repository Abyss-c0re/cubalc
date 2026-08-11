#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1429_hasfns.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft miss continues
cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
FN add a b
  RET a + b
END
HASFNS add ghost
ASSERT LAST_N == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# NEEDFNS hard on miss
cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
FN add a b
  RET a + b
END
NEEDFNS add ghost
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qi NEEDFNS

# NEEDFNS ok when all present
cat > "$TMPDIR/ok.cubalc" << 'C'
HOLD_FLASH 1
FN add a b
  RET a + b
END
FN greet name
  RET name
END
NEEDFNS add greet
ASSERT OK == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/ok.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms HASFNS 2>&1)
printf '%s\n' "$OUT" | grep -qi HASFNS

echo "1429_cli_hasfns: PASS"
