#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

# soft CLI: HOME is always present on this host
OUT=$("$CUBALC" hasenvall HOME 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q 'cubalc.envgate.v1'
printf '%s\n' "$OUT" | grep -q '"any":false'

# soft miss synthetic
OUT=$("$CUBALC" hasenvall CUBALC_ENVGATE_MISS_$$ 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":false'

# hard miss exits non-zero
set +e
OUT=$("$CUBALC" needenvs CUBALC_ENVGATE_MISS_$$ 2>&1)
ec=$?
set -e
test "$ec" -ne 0
printf '%s\n' "$OUT" | grep -q '"ok":false'

# hard hit
OUT=$("$CUBALC" needenvs HOME 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# multi soft partial
OUT=$("$CUBALC" hasenvall HOME CUBALC_ENVGATE_MISS_$$ 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":false'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'hasenvall'

echo "1381_cli_envgate: PASS"
