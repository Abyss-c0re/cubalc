#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1510_ifjsonhasall_any.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"asserts_fail":0'

# WHEN aliases + ELSE dual
cat > "$TMPDIR/when.cubalc" << 'C'
HOLD_FLASH 1
LET plate = "{\"a\":1,\"b\":2}"
WHENJSONHASALL plate "a" "b"
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 1
WHENJSONHASANY plate "z" "a"
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 1
IFHASJSONALL plate "a" "missing"
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/when.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft SYS JSONHASALL dual still works
cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "x" 1 "y" 2
LET plate = LAST
SYS JSONHASALL plate "x" "y"
ASSERT LAST_N == 1
SYS JSONHASANY plate "nosuch" "x"
ASSERT LAST_N == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1510_cli_ifjsonhasall_any PASS"
