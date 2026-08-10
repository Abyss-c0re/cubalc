#!/bin/sh
# cubalc help surfaces recipe · fat-session · run -T · discovery forms
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi '1\.15\.'
printf '%s\n' "$OUT" | grep -qi 'recipe'
printf '%s\n' "$OUT" | grep -qi 'fat-session\|fat_session'
printf '%s\n' "$OUT" | grep -qi '\-T\|timeout\|wall'
printf '%s\n' "$OUT" | grep -qi '\-I\|preload\|include'
printf '%s\n' "$OUT" | grep -qi 'LIBTREE\|LIBDEFAULTS\|LIBINFO\|LISTLIBS\|CATLIB'
printf '%s\n' "$OUT" | grep -qi 'examples fat\|examples|starters'
printf '%s\n' "$OUT" | grep -qi 'HOLD_FLASH'
printf '%s\n' "$OUT" | grep -qi 'doctor'

# -h alias
OUT=$("$CUBALC" -h 2>&1)
printf '%s\n' "$OUT" | grep -qi 'recipe'

echo "1299_cli_help_surface: PASS"
