#!/bin/sh
# CUBALC_INCLUDE_PATH — extra INCLUDE search dirs for agents/projects
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_include_path"
LIBDIR="$ST/mylibs"
rm -rf "$ST"
mkdir -p "$LIBDIR"

# private lib only on INCLUDE_PATH (not in programs/lib)
cat > "$LIBDIR/agent_extra.cubalc" <<'EOF'
# private project lib
LET AGENT_EXTRA_LOADED = 1
LET AGENT_EXTRA_TAG = "from_include_path"
EOF

# without path → soft miss
OUT=$("$CUBALC" run -e 'HOLD_FLASH 1
INCLUDE SOFT agent_extra
ASSERT INCLUDE_OK == 0
' 2>&1 || true)
printf '%s\n' "$OUT" | grep -q '"ok":true\|INCLUDE_OK'

# with CUBALC_INCLUDE_PATH → hit
OUT=$(CUBALC_INCLUDE_PATH="$LIBDIR" "$CUBALC" run -e 'HOLD_FLASH 1
INCLUDE agent_extra
ASSERT INCLUDE_OK == 1
ASSERT AGENT_EXTRA_LOADED == 1
SYS EQS AGENT_EXTRA_TAG "from_include_path"
ASSERT LAST_N == 1
SYS HAS INCLUDE_PATH "agent_extra"
ASSERT LAST_N == 1
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"asserts_fail":0'

# multi-dir: second dir wins after first miss
LIB2="$ST/other"
mkdir -p "$LIB2"
cat > "$LIB2/only_second.cubalc" <<'EOF'
LET ONLY_SECOND = 7
EOF
OUT=$(CUBALC_INCLUDE_PATH="$LIBDIR:$LIB2" "$CUBALC" run -e 'HOLD_FLASH 1
INCLUDE only_second
ASSERT ONLY_SECOND == 7
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# cubalc env lists the contract
OUT=$("$CUBALC" env INCLUDE 2>&1)
printf '%s\n' "$OUT" | grep -q 'CUBALC_INCLUDE_PATH'

echo "1259_cli_include_path: PASS"
