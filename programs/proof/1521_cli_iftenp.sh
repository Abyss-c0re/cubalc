#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1521_iftenp.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"asserts_fail":0'

cat > "$TMPDIR/when.cubalc" << 'C'
HOLD_FLASH 1
LET ten = "{\"a\":1,\"b\":2,\"c\":3,\"d\":4,\"e\":5,\"f\":6,\"g\":7,\"h\":8,\"i\":9,\"j\":10}"
LET nine = "{\"x\":1,\"y\":2,\"z\":3,\"w\":4,\"v\":5,\"u\":6,\"t\":7,\"s\":8,\"r\":9}"
WHENTENKEYP ten
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 1
WHENTENP nine
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/when.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1521_cli_iftenp PASS"
