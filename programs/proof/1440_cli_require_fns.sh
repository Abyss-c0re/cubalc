#!/bin/sh
# cubalc run -N / CUBALC_REQUIRE_FNS — host FN floor after -I preload
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

# tiny lib that defines FNs for the floor
cat > "$TMPDIR/greet_lib.cubalc" << 'C'
FN greet name
  RET name
END
FN hello
  RET 1
END
C

# success: preload defines FNs then -N passes (body uses them)
OUT=$("$CUBALC" run -q -L "$TMPDIR" -I greet_lib -N greet,hello -e 'HASFN greet
ASSERT LAST_N == 1
HASFN hello
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"require_fns":"greet,hello"'

# long flag
OUT=$("$CUBALC" run -q -L "$TMPDIR" -I greet_lib --require-fns "greet hello" -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# hard miss without preload FN
set +e
OUT=$("$CUBALC" run -q -N nosuch_fn_xyz_zzz -e 'PASS' 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
printf '%s\n' "$OUT" | grep -qiE 'REQUIRE FNS|NEEDFNS|missing'
test "$RC" -ne 0

# hard miss: preload exists but missing one FN
set +e
OUT=$("$CUBALC" run -q -L "$TMPDIR" -I greet_lib -N greet,missing_fn_xyz -e 'PASS' 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$RC" -ne 0

# env dual
OUT=$(CUBALC_REQUIRE_FNS=greet,hello "$CUBALC" run -q -L "$TMPDIR" -I greet_lib -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q 'greet'

set +e
OUT=$(CUBALC_REQUIRE_FNS=nosuch_env_fn_zzz "$CUBALC" run -q -e 'PASS' 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
test "$RC" -ne 0

OUT=$("$CUBALC" env CUBALC_REQUIRE_FNS 2>&1)
printf '%s\n' "$OUT" | grep -qi 'REQUIRE_FNS\|FN'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi -- '-N\|require-fns\|FN floor'

# in-lang dual still works
OUT=$("$CUBALC" run -q -e 'FN ping
  RET 1
END
NEEDFNS ping
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1440_cli_require_fns: PASS"
