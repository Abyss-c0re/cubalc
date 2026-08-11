#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1407_hardfail.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# HARDFAIL alone must fail exit without -s
cat > "$TMPDIR/hf.cubalc" << 'C'
HOLD_FLASH 1
HARDFAIL "boom_agent"
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hf.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'HARDFAIL|boom_agent|ok.:false'
printf '%s\n' "$OUT" | grep -q '"halted":true\|"exit_code":1\|HARDFAIL'

# soft FAIL still ok without -s
cat > "$TMPDIR/sf.cubalc" << 'C'
HOLD_FLASH 1
FAIL "soft"
ASSERT OK == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/sf.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# class_guard hard miss without -s (HARDFAIL path)
cat > "$TMPDIR/cg.cubalc" << 'C'
HOLD_FLASH 1
DEFAULT NEED_CLASSES = "nosuch_hardfail_class"
DEFAULT CLASS_GUARD_SOFT = 0
INCLUDE class_guard
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/cg.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'HARDFAIL|CLASS_GUARD|nosuch|ok.:false'

OUT=$("$CUBALC" forms HARDFAIL 2>&1)
printf '%s\n' "$OUT" | grep -qi HARDFAIL

echo "1407_cli_hardfail: PASS"
