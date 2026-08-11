#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1448_try_finally.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# EXIT inside TRY still runs FINALLY (plate may halt)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
MARK="$TMP/cleaned.flag"
OUT=$("$CUBALC" run -q -e "TRY
  SYS TOUCH \"$MARK.partial\"
  EXIT 0
FINALLY
  SYS TOUCH \"$MARK\"
END
PASS" 2>&1 || true)
# FINALLY should have created mark even after EXIT 0
test -f "$MARK"

# missing END
set +e
OUT=$("$CUBALC" run -q -e 'TRY
  PASS
PASS' 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -qiE 'TRY without END|without END'

# regression RETRY
OUT=$("$CUBALC" run -q programs/proof/1447_retry.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1448_cli_try_finally: PASS"
