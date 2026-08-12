#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1500_jsonhitval.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft miss path
cat > "$TMPDIR/soft_miss.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "a" 1
LET plate = LAST
SYS JSONHITVAL plate "x" "y"
ASSERT LAST_N == 0
SYS EMPTY LAST
ASSERT LAST_N == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft_miss.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# first value + key index
cat > "$TMPDIR/idx.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "role" "worker" "n" 1
LET plate = LAST
SYS JSONHITVAL plate "status" "role" "n"
ASSERT LAST_N == 1
SYS EQS LAST "worker"
ASSERT LAST_N == 1
ASSERT JSONHITVAL_N == 2
SYS EQS JSONHITVAL_KEY "role"
ASSERT LAST_N == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/idx.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# JSONFIRSTVAL alias
cat > "$TMPDIR/alias.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "ok" true
SYS JSONFIRSTVAL LAST "missing" "ok"
ASSERT LAST_N == 1
SYS EQS LAST "1"
ASSERT LAST_N == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/alias.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# OF bag
cat > "$TMPDIR/ofbag.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "x" 1 "y" 2
LET plate = LAST
SYS JOIN "" "z\ny\nx"
SYS JSONHITVAL plate OF LAST
ASSERT LAST_N == 1
SYS EQS LAST "2"
ASSERT LAST_N == 1
SYS EQS JSONHITVAL_KEY "y"
ASSERT LAST_N == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/ofbag.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# regression: JSONHITKEY still works
cat > "$TMPDIR/key_still.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "a" 1
LET plate = LAST
SYS JSONHITKEY plate "z" "a"
ASSERT LAST_N == 1
SYS EQS LAST "a"
ASSERT LAST_N == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/key_still.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1500_cli_jsonhitval PASS"
