#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

# soft has — missing flags → ok:false in plate, exit 0
OUT=$("$CUBALC" hasflagall verbose out 2>&1)
printf '%s\n' "$OUT" | grep -q 'cubalc.flaggate.v1'
printf '%s\n' "$OUT" | grep -q '"mode":"has"'
printf '%s\n' "$OUT" | grep -q '"ok":false'

# soft has — present via -- live args
OUT=$("$CUBALC" hasflagall verbose out -- --verbose --out=x 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"live_args":2'

# hard need — missing → exit 1
set +e
OUT=$("$CUBALC" needflags missing_xyz 2>&1)
RC=$?
set -e
test "$RC" -ne 0
printf '%s\n' "$OUT" | grep -q 'cubalc.flaggate.v1'
printf '%s\n' "$OUT" | grep -q '"mode":"need"'

# hard need — present
OUT=$("$CUBALC" needflags verbose -- --verbose 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# aliases
OUT=$("$CUBALC" requireflags verbose -- --verbose 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'needflags'

echo "1366_cli_flaggate: PASS"
