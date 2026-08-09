#!/bin/sh
# cubalc which / SYS WHICH resolve CUBALC_INCLUDE_PATH project libs
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_which_ipath"
LIBDIR="$ST/mylibs"
rm -rf "$ST"
mkdir -p "$LIBDIR"

cat > "$LIBDIR/proj_extra.cubalc" <<'LIB'
LET PROJ_EXTRA = 1
LIB

# without path → no which match
OUT=$("$CUBALC" which proj_extra 2>&1 || true)
printf '%s\n' "$OUT" | grep -q '"ok":false\|"n":0'

# with CUBALC_INCLUDE_PATH → which finds lib
OUT=$(CUBALC_INCLUDE_PATH="$LIBDIR" "$CUBALC" which proj_extra 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q 'proj_extra'
printf '%s\n' "$OUT" | grep -q '"kind":"lib"\|INCLUDE_PATH\|lib'

# SYS WHICH same resolve
OUT=$(CUBALC_INCLUDE_PATH="$LIBDIR" "$CUBALC" run -q -e 'SYS WHICH proj_extra
ASSERT OK == 1
SYS HAS LAST "proj_extra"
ASSERT LAST_N == 1
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"asserts_fail":0'

# doctor surfaces include_path_set when env set
OUT=$(CUBALC_INCLUDE_PATH="$LIBDIR" CUBALC_PRELOAD=agent_boot "$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"include_path_set":true'
printf '%s\n' "$OUT" | grep -q '"preload_set":true'
printf '%s\n' "$OUT" | grep -q 'INCLUDE_PATH\|which'

# stdlib still resolves without env
OUT=$("$CUBALC" which hold_seed 2>&1)
printf '%s\n' "$OUT" | grep -q 'hold_seed'
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1264_cli_which_include_path: PASS"
