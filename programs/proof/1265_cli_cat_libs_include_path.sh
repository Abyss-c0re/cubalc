#!/bin/sh
# cubalc cat + libs resolve/list CUBALC_INCLUDE_PATH project libs
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cat_libs_ip"
LIBDIR="$ST/mylibs"
rm -rf "$ST"
mkdir -p "$LIBDIR"

cat > "$LIBDIR/agent_extra.cubalc" <<'LIB'
# project lib for cat/libs discovery
LET AGENT_EXTRA = 99
LIB

# cat without path fails
OUT=$("$CUBALC" cat agent_extra 2>&1 || true)
printf '%s\n' "$OUT" | grep -q '"ok":false'

# cat with INCLUDE_PATH dumps source + plate
OUT=$(CUBALC_INCLUDE_PATH="$LIBDIR" "$CUBALC" cat agent_extra 2>&1)
printf '%s\n' "$OUT" | grep -q 'AGENT_EXTRA'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q 'agent_extra'
printf '%s\n' "$OUT" | grep -q 'cubalc.cat.v1'

# libs lists project entry with origin include_path
OUT=$(CUBALC_INCLUDE_PATH="$LIBDIR" "$CUBALC" libs 2>&1)
printf '%s\n' "$OUT" | grep -q 'agent_extra'
printf '%s\n' "$OUT" | grep -q 'include_path'
printf '%s\n' "$OUT" | grep -q '"include_path_set":true'
printf '%s\n' "$OUT" | grep -q 'agent_boot'
printf '%s\n' "$OUT" | grep -q 'stdlib\|programs/lib'

# stdlib cat still works
OUT=$("$CUBALC" cat hold_seed 2>&1)
printf '%s\n' "$OUT" | grep -q 'HOLD_FLASH'
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1265_cli_cat_libs_include_path: PASS"
