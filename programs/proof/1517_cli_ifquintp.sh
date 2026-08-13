#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1517_ifquintp.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"asserts_fail":0'

cat > "$TMPDIR/when.cubalc" << 'C'
HOLD_FLASH 1
LET five = "{\"a\":1,\"b\":2,\"c\":3,\"d\":4,\"e\":5}"
LET four = "{\"x\":1,\"y\":2,\"z\":3,\"w\":4}"
WHENFIVEKEYP five
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 1
WHENQUINTP four
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/when.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1517_cli_ifquintp PASS"
