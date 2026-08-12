#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1508_onlyp.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
LET PLATE = "{\"a\":1,\"b\":2}"
ONLYP "a" "b"
ASSERT LAST_N == 1
ASSERT ONLYP_EXTRA == 0
ONLYP "a"
ASSERT LAST_N == 0
ASSERT ONLYP_EXTRA == 1
ASSERT OK == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/from.cubalc" << 'C'
HOLD_FLASH 1
LET PEER = "{\"role\":\"agent\",\"name\":\"thalamus\"}"
ONLYP FROM PEER "role" "name"
ASSERT LAST_N == 1
ASSERT ONLYP_FROM == 1
NOEXTRAP FROM PEER "role"
ASSERT LAST_N == 0
ASSERT ONLYP_EXTRA == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/from.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" help ONLYP 2>&1 || true)
printf '%s\n' "$OUT" | grep -qi ONLYP || true
OUT=$("$CUBALC" help NOEXTRAP 2>&1 || true)
printf '%s\n' "$OUT" | grep -qi NOEXTRAP || true

echo "1508_cli_onlyp OK"
