#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1515_ifpairp_iftriplep.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"asserts_fail":0'

cat > "$TMPDIR/when.cubalc" << 'C'
HOLD_FLASH 1
LET pair = "{\"a\":1,\"b\":2}"
LET three = "{\"x\":1,\"y\":2,\"z\":3}"
WHENTWOKEYP pair
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 1
WHENTHREEKEYP three
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 1
WHENPAIRP three
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 0
WHENTRIPLEP pair
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/when.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1515_cli_ifpairp_iftriplep PASS"
