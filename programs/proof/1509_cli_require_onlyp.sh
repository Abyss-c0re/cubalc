#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1509_require_onlyp.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# soft ONLYP still non-fatal with extras
cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
LET PLATE = "{\"a\":1,\"b\":2,\"noise\":9}"
ONLYP "a" "b"
ASSERT LAST_N == 0
ASSERT ONLYP_EXTRA == 1
ASSERT OK == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# REQUIRE ONLYP fatal when extras present
cat > "$TMPDIR/hard_extra.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "a" 1 "noise" 9
LET plate = LAST
REQUIRE ONLYP plate "a"
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard_extra.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'ONLYP/JSONONLY extra|JSONONLY extra|noise|ok.:false'

# REQUIRE NOEXTRAP alias fatal
cat > "$TMPDIR/hard_noextrap.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "x" 1 "y" 2
LET plate = LAST
REQUIRE NOEXTRAP plate "x"
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard_noextrap.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0

# REQUIRE EXACTP fatal on missing
cat > "$TMPDIR/hard_exact_miss.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "status" "ready"
LET plate = LAST
REQUIRE EXACTP plate "status" "code"
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard_exact_miss.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'EXACTP/JSONEXACT|JSONEXACT|missing|ok.:false'

# REQUIRE EXACTP fatal on extra
cat > "$TMPDIR/hard_exact_extra.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "status" "ready" "code" 0 "noise" 1
LET plate = LAST
REQUIRE EXACTP plate "status" "code"
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard_exact_extra.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'EXACTP/JSONEXACT|JSONEXACT|extra|noise|ok.:false'

# success path aliases
cat > "$TMPDIR/ok_aliases.cubalc" << 'C'
HOLD_FLASH 1
SYS JSONOBJ "role" "agent" "name" "thalamus"
LET peer = LAST
REQUIRE ONLYP peer "role" "name"
ASSERT OK == 1
REQUIRE EXACTP peer "role" "name"
ASSERT OK == 1
REQUIRE SCHEMAP peer "role" "name"
ASSERT OK == 1
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/ok_aliases.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" help "REQUIRE ONLYP" 2>&1 || true)
printf '%s\n' "$OUT" | grep -qi ONLYP || true
OUT=$("$CUBALC" help "REQUIRE EXACTP" 2>&1 || true)
printf '%s\n' "$OUT" | grep -qi EXACTP || true

echo "1509_cli_require_onlyp OK"
