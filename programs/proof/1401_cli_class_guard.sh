#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1401_class_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
DEFAULT NEED_CLASSES = "nosuch_cubalc_cg_miss"
DEFAULT NEED_CLASS_ANY = ""
DEFAULT CLASS_GUARD_SOFT = 1
INCLUDE class_guard
ASSERT CLASS_GUARD_OK == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
DEFAULT NEED_CLASSES = "nosuch_cubalc_cg_hard"
DEFAULT CLASS_GUARD_SOFT = 0
INCLUDE class_guard
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'CLASS_GUARD|missing|ok.:false|nosuch'

cat > "$TMPDIR/any.cubalc" << 'C'
HOLD_FLASH 1
DEFAULT NEED_CLASSES = ""
DEFAULT NEED_CLASS_ANY = "nosuch_a\nnosuch_b"
DEFAULT CLASS_GUARD_SOFT = 0
INCLUDE class_guard
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/any.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'CLASS_GUARD ANY|need one of|ok.:false'

OUT=$("$CUBALC" which class_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'class_guard'

OUT=$("$CUBALC" recipe class_guard 2>&1)
printf '%s\n' "$OUT" | grep -q 'class_guard'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_class_guard":true'

echo "1401_cli_class_guard: PASS"
