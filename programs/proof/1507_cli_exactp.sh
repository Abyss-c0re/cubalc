#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1507_exactp.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
LET PLATE = "{\"a\":1,\"b\":2}"
EXACTP "a" "b"
ASSERT LAST_N == 1
ASSERT EXACTP_HIT == 2
ASSERT EXACTP_EXTRA == 0
EXACTP "a"
ASSERT LAST_N == 0
ASSERT EXACTP_EXTRA == 1
ASSERT OK == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/from.cubalc" << 'C'
HOLD_FLASH 1
LET PEER = "{\"role\":\"agent\",\"name\":\"thalamus\"}"
EXACTP FROM PEER "role" "name"
ASSERT LAST_N == 1
ASSERT EXACTP_FROM == 1
SCHEMAP FROM PEER "role"
ASSERT LAST_N == 0
ASSERT EXACTP_EXTRA == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/from.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" help EXACTP 2>&1 || true)
printf '%s\n' "$OUT" | grep -qi EXACTP || true
OUT=$("$CUBALC" help SCHEMAP 2>&1 || true)
printf '%s\n' "$OUT" | grep -qi SCHEMAP || true

echo "1507_cli_exactp OK"
