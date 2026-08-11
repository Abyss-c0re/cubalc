#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1420_hasclasses.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft multi miss
cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
CLASS A
END
HASCLASSES A Missing
ASSERT LAST_N == 0
ASSERT CLASSMISS == "Missing"
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# NEEDCLASSES hard-fails
cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
CLASS A
END
NEEDCLASSES A Ghost
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qi NEEDCLASSES

# NEEDCLASSES all present
cat > "$TMPDIR/ok.cubalc" << 'C'
HOLD_FLASH 1
CLASS A
END
CLASS B
END
NEEDCLASSES A B
ASSERT OK == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/ok.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms HASCLASSES 2>&1)
printf '%s\n' "$OUT" | grep -qi HASCLASSES
OUT=$("$CUBALC" forms NEEDCLASSES 2>&1)
printf '%s\n' "$OUT" | grep -qi NEEDCLASSES

echo "1420_cli_hasclasses: PASS"
