#!/bin/sh
# cubalc init --fat · catalog fat_boot · run scaffold green
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
ST=state/meaningful_iter/mi_init_fat
mkdir -p "$ST"
export CUBALC_STATE="$ST"

# --list includes fat_boot template
OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -q 'fat_boot'
printf '%s\n' "$OUT" | grep -q -- '--fat'
printf '%s\n' "$OUT" | grep -q '"n":4'

# write scaffold into private state
PROG="$ST/fat_starter.cubalc"
rm -f "$PROG"
OUT=$("$CUBALC" init "$PROG" --fat --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"fat_boot"'
test -f "$PROG"
grep -q 'INCLUDE fat_boot' "$PROG"
grep -q 'NEED_VARROOM' "$PROG"
grep -q 'plate_boot\|PLATE_PATH' "$PROG"

# run scaffold (private state plate path inside program uses state/ — override via edit not needed if relative ok)
# program defaults PLATE_PATH = state/fat_agent.json under CUBALC_STATE? plate_boot uses path as written.
# Use a copy with path under ST for isolation
sed "s|state/fat_agent.json|$ST/fat_agent.json|" "$PROG" > "$ST/fat_run.cubalc"
OUT=$("$CUBALC" run -q "$ST/fat_run.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q 'fat_boot\|agent_boot\|var_guard\|plate_boot'
test -f "$ST/fat_agent.json"

# libs catalog hint for fat_boot
OUT=$("$CUBALC" libs 2>&1)
printf '%s\n' "$OUT" | grep -q 'fat_boot'
printf '%s\n' "$OUT" | grep -qi 'var_guard\|nest\|agent_boot'

echo "1284_cli_init_fat: PASS"
