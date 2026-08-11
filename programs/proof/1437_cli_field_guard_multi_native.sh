#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" run -q programs/proof/1437_field_guard_multi_native.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard ALL miss (NEEDFIELDS dotted)
cat > "$TMPDIR/hard_all.cubalc" << 'C'
HOLD_FLASH 1
CLASS Ticket
  FIELD id
END
LET NEED_FIELDS = "Ticket.id\nTicket.Missing"
LET NEED_FIELD_ANY = ""
LET FIELD_GUARD_SOFT = 0
INCLUDE field_guard
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard_all.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDFIELDS|FIELD_GUARD|missing|Missing'

# hard ANY miss (NEEDFIELDANY dotted)
cat > "$TMPDIR/hard_any.cubalc" << 'C'
HOLD_FLASH 1
CLASS Ticket
  FIELD id
END
LET NEED_FIELDS = ""
LET NEED_FIELD_ANY = "Ghost.a\nGhost.b"
LET FIELD_GUARD_SOFT = 0
INCLUDE field_guard
PASS
C
set +e
OUT=$("$CUBALC" run -q "$TMPDIR/hard_any.cubalc" 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'NEEDFIELDANY|need one of|FIELD_GUARD|Ghost'

# regressions
OUT=$("$CUBALC" run -q programs/proof/1403_field_guard.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q programs/proof/1430_field_guard_native.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q programs/proof/1424_hasfields.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q programs/proof/1425_hasfieldany.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" which field_guard 2>&1)
printf '%s\n' "$OUT" | grep -qi 'field_guard'

echo "1437_cli_field_guard_multi_native: PASS"
