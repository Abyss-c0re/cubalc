#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -q 'host_boot'
printf '%s\n' "$OUT" | grep -qE -- '--host|--host-boot|--host-session'

OUT=$("$CUBALC" init "$TMPDIR/host_demo" --host --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"host_boot"'
test -f "$TMPDIR/host_demo.cubalc"
grep -q 'INCLUDE host_boot' "$TMPDIR/host_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/host_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_host_boot":true'
printf '%s\n' "$OUT" | grep -q '"lib_host_session":true'

OUT=$("$CUBALC" recipe host_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'host_boot'

OUT=$("$CUBALC" which host_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'host_boot'

OUT=$("$CUBALC" libs host_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'host_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qiE -- '--host|--host-boot|--host-session'

OUT=$("$CUBALC" run -q programs/proof/1398_host_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" init --host-session "$TMPDIR/hs" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"host_boot"'

OUT=$("$CUBALC" init --host-boot "$TMPDIR/hb" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"host_boot"'

echo "1398_cli_init_host: PASS"
