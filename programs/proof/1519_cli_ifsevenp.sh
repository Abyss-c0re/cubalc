#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1519_ifsevenp.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"asserts_fail":0'

cat > "$TMPDIR/when.cubalc" << 'C'
HOLD_FLASH 1
LET seven = "{\"a\":1,\"b\":2,\"c\":3,\"d\":4,\"e\":5,\"f\":6,\"g\":7}"
LET six = "{\"x\":1,\"y\":2,\"z\":3,\"w\":4,\"v\":5,\"u\":6}"
WHENSEVENKEYP seven
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 1
WHENSEVENP six
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/when.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1519_cli_ifsevenp PASS"
