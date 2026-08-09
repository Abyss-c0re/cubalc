#!/bin/sh
# cubalc run -I / CUBALC_PRELOAD + -L include-path
# Usability: inject INCLUDE ONCE before program body without editing source.
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_run_preload"
LIBDIR="$ST/mylibs"
rm -rf "$ST"
mkdir -p "$LIBDIR" "$ST"

# private lib only via -L / INCLUDE_PATH
cat > "$LIBDIR/preload_extra.cubalc" <<'LIB'
LET PRELOAD_EXTRA = 42
LET PRELOAD_TAG = "from_L_path"
LIB

# 1) -I agent_boot injects REQUIRE+VERSION without body INCLUDE
OUT=$("$CUBALC" run -q -I agent_boot -e 'SYS HAS VERSION "1.15"
ASSERT LAST_N == 1
SYS HAS INCLUDE_PATH "agent_boot"
ASSERT LAST_N == 1
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"preload_n":1'
printf '%s\n' "$OUT" | grep -q '"asserts_fail":0'

# 2) multi -I + --include= (order: hold_seed then agent_boot)
OUT=$("$CUBALC" run -q -I hold_seed --include=agent_boot -e 'SYS HAS INCLUDE_PATH "agent_boot"
ASSERT LAST_N == 1
SYS HAS VERSION "1.15"
ASSERT LAST_N == 1
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"preload_n":2'

# 3) CUBALC_PRELOAD env (colon list)
OUT=$(CUBALC_PRELOAD="agent_boot" "$CUBALC" run -q -e 'SYS HAS VERSION "1.15"
ASSERT LAST_N == 1
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"preload_n":1'

# 4) -L private dir + -I short name (no programs/lib copy)
OUT=$("$CUBALC" run -q -L "$LIBDIR" -I preload_extra -e 'ASSERT PRELOAD_EXTRA == 42
SYS EQS PRELOAD_TAG "from_L_path"
ASSERT LAST_N == 1
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"include_path_n":1'
printf '%s\n' "$OUT" | grep -q '"preload_n":1'

# 5) top-level alias (no run subcommand): cubalc -I … -e …
OUT=$("$CUBALC" -I agent_boot -q -e 'SYS HAS VERSION "1.15"
ASSERT LAST_N == 1
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# 6) file body + preload (not only -e)
cat > "$ST/body.cubalc" <<'BODY'
ASSERT PRELOAD_EXTRA == 42
PASS
PRINT "body_ok"
BODY
OUT=$("$CUBALC" run -q -L "$LIBDIR" -I preload_extra "$ST/body.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"asserts_fail":0'

# 7) env + CLI combine (dedupe)
OUT=$(CUBALC_PRELOAD="agent_boot" "$CUBALC" run -q -I agent_boot -e 'SYS HAS VERSION "1.15"
ASSERT LAST_N == 1
' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"preload_n":1'

# 8) cubalc env lists CUBALC_PRELOAD contract
OUT=$("$CUBALC" env PRELOAD 2>&1)
printf '%s\n' "$OUT" | grep -q 'CUBALC_PRELOAD'

echo "1260_cli_run_preload: PASS"
