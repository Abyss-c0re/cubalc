#!/bin/sh
# RECIPE in-lang dual of cubalc recipe + forms + soft miss + CLI parity
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'RECIPE fat_session
ASSERT OK == 1
ASSERT LIB_DEPS_N == 2
ASSERT RECIPE_DEFAULTS_N == 1
SYS HAS LAST "cubalc.recipe.v1"
ASSERT LAST_N == 1
SYS HASLINE RECIPE_DEPS "fat_boot"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'LIBCARD plate_tick
ASSERT OK == 1
ASSERT RECIPE_DEFAULTS_N == 4
SYS HAS LAST "defaults_n"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q -e 'RECIPE missing_lib_qqq
ASSERT OK == 0
CLEAR_ERR
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# forms discovery
OUT=$("$CUBALC" forms recipe 2>&1) || OUT=$("$CUBALC" forms RECIPE 2>&1)
printf '%s\n' "$OUT" | grep -qi 'RECIPE'

OUT=$("$CUBALC" forms libcard 2>&1) || OUT=$("$CUBALC" forms LIBCARD 2>&1)
printf '%s\n' "$OUT" | grep -qi 'LIBCARD\|RECIPE'

# help surfaces RECIPE form
OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -q 'RECIPE'

# CLI recipe still green (parity)
OUT=$("$CUBALC" recipe fat_session 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.recipe.v1"'
printf '%s\n' "$OUT" | grep -q '"deps_n":2'

OUT=$("$CUBALC" run -q programs/proof/1300_recipe.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1300_cli_recipe: PASS"
