#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1415_isclass.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# NEEDISA fails process on mismatch
cat > "$TMPDIR/need.cubalc" << 'C'
HOLD_FLASH 1
CLASS A
  FIELD n
END
CLASS B
  FIELD n
END
NEW A x
NEEDISA x B
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/need.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDISA|not |ok.:false'

# ISCLASS soft continues
cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
CLASS A
  FIELD n
END
NEW A x
ISCLASS x B
ASSERT LAST_N == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms ISCLASS 2>&1)
printf '%s\n' "$OUT" | grep -qi ISCLASS

echo "1415_cli_isclass: PASS"
