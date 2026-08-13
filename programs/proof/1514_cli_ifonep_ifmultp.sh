#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1514_ifonep_ifmultp.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"asserts_fail":0'

# WHEN aliases dual
cat > "$TMPDIR/when.cubalc" << 'C'
HOLD_FLASH 1
LET one = "{\"k\":9}"
LET multi = "{\"a\":1,\"b\":2}"
WHENONEKEYP one
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 1
WHENMULTIKEYP multi
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 1
WHENONEP multi
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 0
WHENMULTP one
  LET ok = 1
ELSE
  LET ok = 0
END
ASSERT ok == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/when.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1514_cli_ifonep_ifmultp PASS"
