#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -q 'path_boot'
printf '%s\n' "$OUT" | grep -q -- '--path'

OUT=$("$CUBALC" init "$TMPDIR/path_demo" --path --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"path_boot"'
test -f "$TMPDIR/path_demo.cubalc"
grep -q 'INCLUDE path_boot' "$TMPDIR/path_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/path_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_path_boot":true'
printf '%s\n' "$OUT" | grep -q '"lib_path_guard":true'

OUT=$("$CUBALC" recipe path_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'path_boot'

OUT=$("$CUBALC" which path_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'path_boot'

OUT=$("$CUBALC" libs path_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'path_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi -- '--path'

OUT=$("$CUBALC" run -q programs/proof/1394_path_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# alias --path-guard / --hostpath
OUT=$("$CUBALC" init --path-guard "$TMPDIR/pg" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"path_boot"'

OUT=$("$CUBALC" init --hostpath "$TMPDIR/hp" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"path_boot"'

echo "1394_cli_init_path: PASS"
