#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1498_require_jsonhasany.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft SYS path still works
cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "a" 1 "b" 2
LET plate = LAST
SYS JSONHASANY plate "a" "nosuch"
ASSERT LAST_N == 1
SYS JSONHASANY plate "nosuch_a" "nosuch_b"
ASSERT LAST_N == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# REQUIRE JSONHASANY fatal when none present
cat > "$TMPDIR/hard_miss.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "n" 1
LET plate = LAST
REQUIRE JSONHASANY plate "nosuch_a" "nosuch_b"
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard_miss.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'JSONHASANY none|need one of|ok.:false'

# NEEDJSONANY alias fatal
cat > "$TMPDIR/need_miss.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "n" 1
REQUIRE NEEDJSONANY LAST "x" "y"
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/need_miss.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'JSONHASANY none|need one of|ok.:false'

# hard hit succeeds
cat > "$TMPDIR/hard_hit.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "n" 1
LET plate = LAST
REQUIRE JSONHASANY plate "nosuch_z" "n"
ASSERT LAST_N == 1
ASSERT JSONHASANY_HIT == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/hard_hit.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# two-token REQUIRE JSON ANY
cat > "$TMPDIR/two_tok.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "role" "worker"
LET plate = LAST
REQUIRE JSON ANY plate "status" "role"
ASSERT OK == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/two_tok.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# regression: REQUIRE JSONHASALL still all-of
cat > "$TMPDIR/all_still.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "a" 1 "b" 2
LET plate = LAST
REQUIRE JSONHASALL plate "a" "b"
ASSERT OK == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/all_still.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1498_cli_require_jsonhasany: PASS"
