#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1512_ifmissp_ifextrap.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"asserts_fail":0'

# WHEN aliases + dual soft SYS
cat > "$TMPDIR/when.cubalc" << 'C'
HOLD_FLASH 1
LET plate = "{\"a\":1}"
LET noisy = "{\"a\":1,\"noise\":9}"
WHENMISSP plate "a" "b"
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 1
WHENEXTRAP noisy "a"
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 1
WHENABSENTP plate "a"
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 0
WHENUNKNOWNKEYSP plate "a"
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/when.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft SYS dual with FROM plate
cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "x" 1
LET plate = LAST
MISSP FROM plate "x" "y"
ASSERT LAST_N == 1
EXTRAP FROM plate "x"
ASSERT LAST_N == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"asserts_fail":0'

echo "1512_cli_ifmissp_ifextrap PASS"
