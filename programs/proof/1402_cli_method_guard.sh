#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1402_method_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/soft.cubalc" << 'C'
HOLD_FLASH 1
CLASS Ticket
  METHOD ping
    LET LAST = 1
  END
END
DEFAULT NEED_METHODS = "nosuch_cubalc_mg_miss"
DEFAULT NEED_METHOD_ANY = ""
DEFAULT METHOD_GUARD_SOFT = 1
INCLUDE method_guard
ASSERT METHOD_GUARD_OK == 0
PASS
C
OUT=$("$CUBALC" run -q "$TMPDIR/soft.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

cat > "$TMPDIR/hard.cubalc" << 'C'
HOLD_FLASH 1
CLASS Ticket
  METHOD ping
    LET LAST = 1
  END
END
DEFAULT NEED_METHODS = "Ticket.nosuch_cubalc_mg_hard"
DEFAULT METHOD_GUARD_SOFT = 0
INCLUDE method_guard
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'METHOD_GUARD|missing|ok.:false|nosuch'

cat > "$TMPDIR/any.cubalc" << 'C'
HOLD_FLASH 1
CLASS Ticket
  METHOD ping
    LET LAST = 1
  END
END
DEFAULT NEED_METHODS = ""
DEFAULT NEED_METHOD_ANY = "nosuch_a\nnosuch_b"
DEFAULT METHOD_GUARD_SOFT = 0
INCLUDE method_guard
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/any.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'METHOD_GUARD ANY|need one of|ok.:false'

OUT=$("$CUBALC" which method_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'method_guard'

OUT=$("$CUBALC" recipe method_guard 2>&1)
printf '%s\n' "$OUT" | grep -q 'method_guard'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_method_guard":true'

echo "1402_cli_method_guard: PASS"
