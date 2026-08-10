#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

# soft has — missing → ok:false
OUT=$("$CUBALC" hasargall 0 1 2>&1)
printf '%s\n' "$OUT" | grep -q 'cubalc.arggate.v1'
printf '%s\n' "$OUT" | grep -q '"mode":"has"'
printf '%s\n' "$OUT" | grep -q '"ok":false'

# soft has — live positionals
OUT=$("$CUBALC" hasargall 0 1 -- a.txt b.txt 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"live_args":2'

# hard need — missing exit 1
set +e
OUT=$("$CUBALC" needargs 0 1 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -q 'cubalc.arggate.v1'
printf '%s\n' "$OUT" | grep -q '"mode":"need"'

# hard need — present
OUT=$("$CUBALC" needargs 0 1 -- x y 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" requireargs 0 -- only 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'needargs'

echo "1366_cli_arggate: PASS"
