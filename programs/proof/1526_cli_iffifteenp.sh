#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1526_iffifteenp.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"asserts_fail":0'

cat > "$TMPDIR/when.cubalc" << 'C'
HOLD_FLASH 1
LET fifteen = "{\"a\":1,\"b\":2,\"c\":3,\"d\":4,\"e\":5,\"f\":6,\"g\":7,\"h\":8,\"i\":9,\"j\":10,\"k\":11,\"l\":12,\"m\":13,\"n\":14,\"o\":15}"
LET fourteen = "{\"a\":1,\"b\":2,\"c\":3,\"d\":4,\"e\":5,\"f\":6,\"g\":7,\"h\":8,\"i\":9,\"j\":10,\"k\":11,\"l\":12,\"m\":13,\"n\":14}"
WHENFIFTEENKEYP fifteen
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 1
WHENFIFTEENP fourteen
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 0
# boundary: sixteen must fail
LET sixteen = "{\"a\":1,\"b\":2,\"c\":3,\"d\":4,\"e\":5,\"f\":6,\"g\":7,\"h\":8,\"i\":9,\"j\":10,\"k\":11,\"l\":12,\"m\":13,\"n\":14,\"o\":15,\"p\":16}"
WHENFIFTEENP sixteen
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/when.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"asserts_fail":0'

echo "1526_cli_iffifteenp PASS"
