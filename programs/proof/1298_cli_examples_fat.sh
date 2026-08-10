#!/bin/sh
# examples/search fat nest discovery polish
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

# examples fat → fat_session + fat_boot present
OUT=$("$CUBALC" examples fat 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.examples.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q 'fat_session'
printf '%s\n' "$OUT" | grep -q 'fat_boot'
printf '%s\n' "$OUT" | grep -q 'var_guard\|time_guard'
# n_present > 0
printf '%s\n' "$OUT" | grep -qE '"n":[1-9]'

# search fat_session
OUT=$("$CUBALC" search fat_session 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -qi 'fat_session'

# search recipe → CLI command
OUT=$("$CUBALC" search recipe 2>&1)
printf '%s\n' "$OUT" | grep -q 'recipe'
printf '%s\n' "$OUT" | grep -qi 'cubalc.recipe\|path+deps\|recipe.v1\|lib'

# search init mentions fat-session
OUT=$("$CUBALC" search init 2>&1)
printf '%s\n' "$OUT" | grep -qi 'fat-session\|fat_session\|--fat'

# search durable nest path via fat
OUT=$("$CUBALC" search fat_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'fat_boot'

echo "1298_cli_examples_fat: PASS"
