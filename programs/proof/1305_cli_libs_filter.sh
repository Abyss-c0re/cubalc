#!/bin/sh
# cubalc libs [filter] — stem/path/hint substring discovery for agents
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

# full catalog still green
OUT=$("$CUBALC" libs 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.libs.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n_total":'
# unfiltered n_match == n_total
printf '%s\n' "$OUT" | grep -qE '"n_match":2[0-9]|"n":2[0-9]'

# plate filter
OUT=$("$CUBALC" libs plate 2>&1)
printf '%s\n' "$OUT" | grep -q '"filter":"plate"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"stem":"plate_tick"'
printf '%s\n' "$OUT" | grep -q '"stem":"plate_boot"'
# agent_boot stem not in plate filter results (hint text may still mention it)
printf '%s\n' "$OUT" | grep -q '"stem":"agent_boot"' && exit 1 || true
printf '%s\n' "$OUT" | grep -qE '"n_match":1[0-9]|"n":1[0-9]'

# fat filter
OUT=$("$CUBALC" libs fat 2>&1)
printf '%s\n' "$OUT" | grep -q 'fat_session'
printf '%s\n' "$OUT" | grep -q 'fat_boot'
printf '%s\n' "$OUT" | grep -q '"filter":"fat"'

# --filter form
OUT=$("$CUBALC" libs --filter tick 2>&1)
printf '%s\n' "$OUT" | grep -q 'plate_tick'
printf '%s\n' "$OUT" | grep -q 'plate_peer_tick'
printf '%s\n' "$OUT" | grep -q '"filter":"tick"'

# case-insensitive
OUT=$("$CUBALC" libs FAT 2>&1)
printf '%s\n' "$OUT" | grep -q 'fat_session'

# miss filter → ok:false exit 1
set +e
OUT=$("$CUBALC" libs no_such_lib_xyz_zzz 2>&1)
RC=$?
set -e
printf '%s\n' "$OUT" | grep -q '"ok":false'
printf '%s\n' "$OUT" | grep -q '"n_match":0'
test "$RC" -ne 0

# help mentions filter
OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -q 'filter\|libs|lib|stdlib \[q\]'

# prior meta proof surface still present
OUT=$("$CUBALC" libs fat_session 2>&1)
printf '%s\n' "$OUT" | grep -q '"stem":"fat_session"'
printf '%s\n' "$OUT" | grep -q '"deps_n":2'

echo "1305_cli_libs_filter: PASS"
