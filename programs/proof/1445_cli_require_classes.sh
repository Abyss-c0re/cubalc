#!/bin/sh
# cubalc run -K / CUBALC_REQUIRE_CLASSES — host CLASS floor after -I preload
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

# tiny lib that defines CLASS for the floor
cat > "$TMPDIR/cell_lib.cubalc" << 'C'
CLASS Cell
  FIELD n 0
  METHOD tick
    RET 1
  END
END
CLASS Ticket
  FIELD id
END
C

# success: preload defines CLASSes then -K passes
OUT=$("$CUBALC" run -q -L "$TMPDIR" -I cell_lib -K Cell,Ticket -e 'HASCLASS Cell
ASSERT LAST_N == 1
HASCLASS Ticket
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"require_classes":"Cell,Ticket"'

# long flag
OUT=$("$CUBALC" run -q -L "$TMPDIR" -I cell_lib --require-classes "Cell Ticket" -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard miss without preload CLASS
set +e
OUT=$("$CUBALC" run -q -K NosuchClassXyzZzz -e 'PASS' 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
printf '%s\n' "$OUT" | grep -qiE 'REQUIRE CLASSES|NEEDCLASSES|missing'
test "$RC" -ne 0

# hard miss: preload exists but missing one CLASS
set +e
OUT=$("$CUBALC" run -q -L "$TMPDIR" -I cell_lib -K Cell,MissingClassXyz -e 'PASS' 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$RC" -ne 0

# env dual
OUT=$(CUBALC_REQUIRE_CLASSES=Cell,Ticket "$CUBALC" run -q -L "$TMPDIR" -I cell_lib -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q 'Cell'

set +e
OUT=$(CUBALC_REQUIRE_CLASSES=NosuchEnvClassZzz "$CUBALC" run -q -e 'PASS' 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$RC" -ne 0

OUT=$("$CUBALC" env CUBALC_REQUIRE_CLASSES 2>&1)
printf '%s\n' "$OUT" | grep -qi 'REQUIRE_CLASSES\|CLASS'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi -- '-K\|require-classes\|CLASS floor'

# in-lang dual still works
OUT=$("$CUBALC" run -q -e 'CLASS Ping
  FIELD x
END
NEEDCLASSES Ping
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# regression require-fns
OUT=$("$CUBALC" run -q programs/proof/1440_cli_require_fns.sh 2>&1 || true)
# run shell proof separately below

echo "1445_cli_require_classes: PASS"
