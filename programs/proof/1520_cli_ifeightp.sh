#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1520_ifeightp.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"asserts_fail":0'

cat > "$TMPDIR/when.cubalc" << 'C'
HOLD_FLASH 1
LET eight = "{\"a\":1,\"b\":2,\"c\":3,\"d\":4,\"e\":5,\"f\":6,\"g\":7,\"h\":8}"
LET seven = "{\"x\":1,\"y\":2,\"z\":3,\"w\":4,\"v\":5,\"u\":6,\"t\":7}"
WHENEIGHTKEYP eight
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 1
WHENEIGHTP seven
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/when.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1520_cli_ifeightp PASS"
