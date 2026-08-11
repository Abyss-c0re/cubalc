#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1436_method_guard_multi_native.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard ALL miss (NEEDMETHODS dotted)
cat > "$TMPDIR/hard_all.cubalc" << 'C'
HOLD_FLASH 1
CLASS Ticket
  METHOD ping
    LET LAST = 1
  END
END
LET NEED_METHODS = "Ticket.ping\nTicket.Missing"
LET NEED_METHOD_ANY = ""
LET METHOD_GUARD_SOFT = 0
INCLUDE method_guard
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard_all.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDMETHODS|METHOD_GUARD|missing|Missing'

# hard ANY miss (NEEDMETHODANY dotted)
cat > "$TMPDIR/hard_any.cubalc" << 'C'
HOLD_FLASH 1
CLASS Ticket
  METHOD ping
    LET LAST = 1
  END
END
LET NEED_METHODS = ""
LET NEED_METHOD_ANY = "Ghost.a\nGhost.b"
LET METHOD_GUARD_SOFT = 0
INCLUDE method_guard
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard_any.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDMETHODANY|need one of|METHOD_GUARD|Ghost'

# regressions
OUT=$("$CUBALC" run -q programs/proof/1402_method_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q programs/proof/1428_method_guard_native.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q programs/proof/1422_hasmethods.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q programs/proof/1423_hasmethodany.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" which method_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'method_guard'

echo "1436_cli_method_guard_multi_native: PASS"
