#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1496_haspany.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
LET PLATE = "{\"a\":1,\"b\":2}"
HASPANY a nosuch
ASSERT LAST_N == 1
ASSERT HASPANY_N == 1
HASPANY nosuch_a nosuch_b
ASSERT LAST_N == 0
ASSERT KEYMISS_N == 2
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# NEEDPANY fatal when none present
cat > "$TMPDIR/hard_miss.cubalc" << 'C'
HOLD_FLASH 1
LET PLATE = "{\"n\":1}"
NEEDPANY nosuch_a nosuch_b
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard_miss.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDPANY miss|need one of|ok.:false'

cat > "$TMPDIR/hard_hit.cubalc" << 'C'
HOLD_FLASH 1
LET PLATE = "{\"n\":1}"
NEEDPANY nosuch_z n
ASSERT LAST_N == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/hard_hit.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1496_cli_haspany: PASS"
