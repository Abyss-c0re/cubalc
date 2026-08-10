#!/bin/sh
# cubalc recipe / card — path + deps + defaults + head one plate
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" recipe fat_session 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.recipe.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"stem":"fat_session"'
printf '%s\n' "$OUT" | grep -q 'fat_boot'
printf '%s\n' "$OUT" | grep -q 'plate_boot'
printf '%s\n' "$OUT" | grep -q 'NEED_VARROOM=48'
printf '%s\n' "$OUT" | grep -q '"deps_n":2'
printf '%s\n' "$OUT" | grep -q '"head":\['

OUT=$("$CUBALC" card plate_tick 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q 'PLATE_PATH='
printf '%s\n' "$OUT" | grep -q '"defaults_n":4'

OUT=$("$CUBALC" recipe no_such_lib_xyz_zzz 2>&1) || true
printf '%s\n' "$OUT" | grep -q '"ok":false'

OUT=$("$CUBALC" recipe 2>&1) || true
printf '%s\n' "$OUT" | grep -q 'need libname\|usage'

# doctor mentions recipe
OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -qi 'recipe'

echo "1297_cli_recipe: PASS"
