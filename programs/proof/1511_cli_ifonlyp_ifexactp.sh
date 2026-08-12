#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1511_ifonlyp_ifexactp.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"asserts_fail":0'

# WHEN aliases + dual soft SYS
cat > "$TMPDIR/when.cubalc" << 'C'
HOLD_FLASH 1
LET plate = "{\"a\":1,\"b\":2}"
WHENONLYP plate "a" "b"
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 1
WHENEXACTP plate "a" "b"
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 1
WHENSCHEMAP plate "a" "missing"
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 0
WHENNOEXTRAP plate "a"
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/when.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft SYS dual with FROM plate (LAST not sticky after ONLYP)
cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "x" 1 "y" 2
LET plate = LAST
ONLYP FROM plate "x" "y"
ASSERT LAST_N == 1
EXACTP FROM plate "x" "y"
ASSERT LAST_N == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"asserts_fail":0'

echo "1511_cli_ifonlyp_ifexactp PASS"
