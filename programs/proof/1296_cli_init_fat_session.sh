#!/bin/sh
# cubalc init --fat-session · catalog fat_session · run scaffold green
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
ST=state/meaningful_iter/mi_init_fat_session
mkdir -p "$ST"
export CUBALC_STATE="$ST"

# --list includes fat_session template
OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -q 'fat_session'
printf '%s\n' "$OUT" | grep -q -- '--fat-session\|--durable'
printf '%s\n' "$OUT" | grep -q '"n":5'

# write scaffold into private state
PROG="$ST/session_starter.cubalc"
rm -f "$PROG"
OUT=$("$CUBALC" init "$PROG" --fat-session --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"fat_session"'
test -f "$PROG"
grep -q 'INCLUDE fat_session' "$PROG"
grep -q 'NEED_VARROOM' "$PROG"
grep -q 'PLATE_PATH' "$PROG"

# run with plate path under ST
sed "s|state/fat_session.json|$ST/fat_session.json|" "$PROG" > "$ST/session_run.cubalc"
OUT=$("$CUBALC" run -q "$ST/session_run.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
test -f "$ST/fat_session.json"

# alias --durable
PROG2="$ST/durable_starter.cubalc"
OUT=$("$CUBALC" init "$PROG2" --durable --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"fat_session"'
grep -q 'INCLUDE fat_session' "$PROG2"

# doctor mentions fat_session init
OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q 'fat_session\|fat-session'

echo "1296_cli_init_fat_session: PASS"
