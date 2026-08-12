#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1499_jsonhitkey.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft miss path
cat > "$TMPDIR/soft_miss.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "a" 1
LET plate = LAST
SYS JSONHITKEY plate "x" "y"
ASSERT LAST_N == 0
SYS EMPTY LAST
ASSERT LAST_N == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft_miss.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# first key index
cat > "$TMPDIR/idx.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "role" "worker" "n" 1
LET plate = LAST
SYS JSONHITKEY plate "status" "role" "n"
ASSERT LAST_N == 1
SYS EQS LAST "role"
ASSERT LAST_N == 1
ASSERT JSONHITKEY_N == 2
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/idx.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# JSONFIRST alias
cat > "$TMPDIR/alias.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "ok" true
SYS JSONFIRST LAST "missing" "ok"
ASSERT LAST_N == 1
SYS EQS LAST "ok"
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
SYS JSONHITKEY plate OF LAST
ASSERT LAST_N == 1
SYS EQS LAST "y"
ASSERT LAST_N == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/ofbag.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# regression: JSONHASANY still works
cat > "$TMPDIR/any_still.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "a" 1
LET plate = LAST
SYS JSONHASANY plate "z" "a"
ASSERT LAST_N == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/any_still.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1499_cli_jsonhitkey: PASS"
