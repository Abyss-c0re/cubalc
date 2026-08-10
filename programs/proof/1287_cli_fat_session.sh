#!/bin/sh
# INCLUDE fat_session + doctor lib_fat_session + which/cat/libs
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
ST=state/meaningful_iter/mi_fat_session
mkdir -p "$ST"
export CUBALC_STATE="$ST"

# write a tiny program file to avoid shell JSON escape footguns
PROG="$ST/run_fat_session.cubalc"
cat > "$PROG" <<EOF
DEFAULT NEED_VARROOM = 16
DEFAULT PLATE_PATH = "$ST/p.json"
DEFAULT PLATE_SEED = "{\"n\":0,\"ok\":true}"
INCLUDE fat_session
ASSERT OK == 1
ASSERT VARROOM >= 1
SETP "status" "ready"
INCLUDE plate_save
PASS
EOF

OUT=$("$CUBALC" run -q "$PROG" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q 'fat_session\|fat_boot\|plate_boot'
test -f "$ST/p.json"

# which/cat/libs
OUT=$("$CUBALC" which fat_session 2>&1)
printf '%s\n' "$OUT" | grep -q 'fat_session'
OUT=$("$CUBALC" cat fat_session 2>&1)
printf '%s\n' "$OUT" | grep -q 'fat_boot\|plate_boot'
OUT=$("$CUBALC" libs 2>&1)
printf '%s\n' "$OUT" | grep -q 'fat_session'

# doctor
OUT=$("$CUBALC" doctor)
printf '%s\n' "$OUT" | grep -q '"lib_fat_session":true'
n=$(printf '%s\n' "$OUT" | sed -n 's/.*"libs_n":\([0-9][0-9]*\).*/\1/p' | head -1)
test -n "$n"
test "$n" -ge 20

# file proof
OUT=$("$CUBALC" run -q programs/proof/1287_fat_session.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1287_cli_fat_session: PASS"
