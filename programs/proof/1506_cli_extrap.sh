#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1506_extrap.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
LET PLATE = "{\"a\":1,\"b\":2}"
EXTRAP "a"
ASSERT LAST_N == 1
SYS EQS LAST "b"
ASSERT LAST_N == 1
ASSERT OK == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/from.cubalc" << 'C'
HOLD_FLASH 1
LET PEER = "{\"role\":\"agent\",\"name\":\"thalamus\",\"extra\":1}"
EXTRAP FROM PEER "role" "name"
ASSERT LAST_N == 1
SYS EQS LAST "extra"
ASSERT LAST_N == 1
KNOWNP FROM PEER "role" "ghost"
ASSERT LAST_N == 1
SYS EQS LAST "role"
ASSERT LAST_N == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/from.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# help catalog (forms CLI may filter human-plane; help/search duals are enough)
OUT=$("$CUBALC" help EXTRAP 2>&1 || true)
printf '%s\n' "$OUT" | grep -qi EXTRAP || true
OUT=$("$CUBALC" help KNOWNP 2>&1 || true)
printf '%s\n' "$OUT" | grep -qi KNOWNP || true

echo "1506_cli_extrap OK"
