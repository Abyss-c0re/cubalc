#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1501_jsoncoalesce.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft miss + default
cat > "$TMPDIR/soft_def.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "a" ""
LET plate = LAST
SYS JSONCOALESCE plate "a" "b" OR "def"
ASSERT LAST_N == 1
SYS EQS LAST "def"
ASSERT LAST_N == 1
ASSERT JSONCOALESCE_HIT == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft_def.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# first non-empty + key index
cat > "$TMPDIR/idx.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "role" "" "name" "bob" "n" 1
LET plate = LAST
SYS JSONCOALESCE plate "status" "role" "name" "n"
ASSERT LAST_N == 1
SYS EQS LAST "bob"
ASSERT LAST_N == 1
ASSERT JSONCOALESCE_N == 3
SYS EQS JSONCOALESCE_KEY "name"
ASSERT LAST_N == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/idx.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# JSONNVL alias
cat > "$TMPDIR/alias.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "ok" true
SYS JSONNVL LAST "missing" "ok"
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
SYS JSONOBJ "x" "" "y" "two"
LET plate = LAST
SYS JOIN "" "x\ny"
SYS JSONCOALESCE plate OF LAST
ASSERT LAST_N == 1
SYS EQS LAST "two"
ASSERT LAST_N == 1
SYS EQS JSONCOALESCE_KEY "y"
ASSERT LAST_N == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/ofbag.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# regression: JSONHITVAL still works (empty still present)
cat > "$TMPDIR/hitval_still.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "err" "" "msg" "hi"
LET plate = LAST
SYS JSONHITVAL plate "err" "msg"
ASSERT LAST_N == 1
SYS EMPTY LAST
ASSERT LAST_N == 1
SYS EQS JSONHITVAL_KEY "err"
ASSERT LAST_N == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/hitval_still.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1501_cli_jsoncoalesce PASS"
