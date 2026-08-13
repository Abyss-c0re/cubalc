#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1513_ifemptyp_ifnonemptyp.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"asserts_fail":0'

# WHEN aliases + dual soft SYS
cat > "$TMPDIR/when.cubalc" << 'C'
HOLD_FLASH 1
LET plate = "{\"a\":1}"
LET empty = "{}"
WHENEMPTYP empty
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 1
WHENNONEMPTYP plate
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 1
WHENISEMPTYP plate
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 0
WHENHASKEYSP empty
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
EMPTYP FROM plate
ASSERT LAST_N == 0
NONEMPTYP FROM plate
ASSERT LAST_N == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"asserts_fail":0'

echo "1513_cli_ifemptyp_ifnonemptyp PASS"
