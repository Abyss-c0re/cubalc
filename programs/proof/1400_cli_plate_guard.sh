#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1400_plate_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/soft_miss.cubalc" << 'C'
HOLD_FLASH 1
LET PLATE = "{\"n\":1,\"ok\":true}"
DEFAULT NEED_KEYS = "nosuch_cubalc_pg_miss"
DEFAULT NEED_KEY_ANY = ""
DEFAULT PLATE_GUARD_SOFT = 1
INCLUDE plate_guard
ASSERT PLATE_GUARD_OK == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft_miss.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/hard_miss.cubalc" << 'C'
HOLD_FLASH 1
LET PLATE = "{\"n\":1}"
DEFAULT NEED_KEYS = "nosuch_cubalc_pg_hard"
DEFAULT PLATE_GUARD_SOFT = 0
INCLUDE plate_guard
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard_miss.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDP|PLATE_GUARD|missing|ok.:false|nosuch'

cat > "$TMPDIR/any_miss.cubalc" << 'C'
HOLD_FLASH 1
LET PLATE = "{\"n\":1}"
DEFAULT NEED_KEYS = ""
DEFAULT NEED_KEY_ANY = "nosuch_a\nnosuch_b"
DEFAULT PLATE_GUARD_SOFT = 0
INCLUDE plate_guard
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/any_miss.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'PLATE_GUARD ANY|need one of|ok.:false'

OUT=$("$CUBALC" which plate_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'plate_guard'

OUT=$("$CUBALC" recipe plate_guard 2>&1)
printf '%s\n' "$OUT" | grep -q 'plate_guard'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_plate_guard":true'

OUT=$("$CUBALC" libs plate_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'plate_guard'

echo "1400_cli_plate_guard: PASS"
