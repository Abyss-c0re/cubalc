#!/bin/sh
# cubalc libs — stem + deps_n + defaults_n recipe meta for agents
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" libs 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.libs.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q 'deps_n'
printf '%s\n' "$OUT" | grep -q 'defaults_n'
printf '%s\n' "$OUT" | grep -q '"stem":"fat_session"'
# fat_session: 2 INCLUDE, 1 DEFAULT
printf '%s\n' "$OUT" | grep -qE '"stem":"fat_session"[^}]*"deps_n":2|"deps_n":2[^}]*"stem":"fat_session"'
printf '%s\n' "$OUT" | grep -qE '"stem":"fat_session"[^}]*"defaults_n":1|"defaults_n":1[^}]*"stem":"fat_session"'
# plate_tick: 0 INCLUDE, 4 DEFAULT
printf '%s\n' "$OUT" | grep -qE '"stem":"plate_tick"[^}]*"defaults_n":4|"defaults_n":4[^}]*"stem":"plate_tick"'
# agent_boot leaf
printf '%s\n' "$OUT" | grep -qE '"stem":"agent_boot"[^}]*"deps_n":0|"deps_n":0[^}]*"stem":"agent_boot"'
# human TSV cols
printf '%s\n' "$OUT" | grep -q 'fat_session.cubalc'
printf '%s\n' "$OUT" | grep -q 'deps_n defaults_n\|stem deps_n'
# note / header mentions deps meta
printf '%s\n' "$OUT" | grep -q 'deps_n'

# help mentions deps_n
OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -q 'deps_n\|defaults_n'

# include_path still works (1265 dual)
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_libs_meta"
LIBDIR="$ST/mylibs"
rm -rf "$ST"
mkdir -p "$LIBDIR"
printf 'DEFAULT X = 1\nINCLUDE hold_seed\n' > "$LIBDIR/proj_lib.cubalc"
OUT=$(CUBALC_INCLUDE_PATH="$LIBDIR" "$CUBALC" libs 2>&1)
printf '%s\n' "$OUT" | grep -q '"stem":"proj_lib"'
printf '%s\n' "$OUT" | grep -qE '"stem":"proj_lib"[^}]*"deps_n":1|"deps_n":1[^}]*"stem":"proj_lib"'
printf '%s\n' "$OUT" | grep -qE '"stem":"proj_lib"[^}]*"defaults_n":1|"defaults_n":1[^}]*"stem":"proj_lib"'

echo "1304_cli_libs_recipe_meta: PASS"
