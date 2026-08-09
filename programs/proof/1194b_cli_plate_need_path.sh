#!/bin/sh
# cubalc plate has|need dotted path contracts
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_need_path"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"host":"cubeA","freq":{"error":5,"warn":2},"n":1}' | grep -q '"ok":true'

# has all present
OUT=$("$CUBALC" plate has "$F" host freq.error n)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"has_all":true'
# exit 0
"$CUBALC" plate has "$F" host freq.error >/dev/null

# has soft miss
set +e
OUT=$("$CUBALC" plate has "$F" host freq.crit)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"has_all":false'
printf '%s\n' "$OUT" | grep -q 'freq.crit'
test "$RC" -ne 0

# need hard gate fail
set +e
OUT=$("$CUBALC" plate need "$F" host freq.crit)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
printf '%s\n' "$OUT" | grep -q 'freq.crit'
test "$RC" -ne 0

# need pass
OUT=$("$CUBALC" plate need "$F" host freq.error freq.warn)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1194b_cli_plate_need_path: PASS"
