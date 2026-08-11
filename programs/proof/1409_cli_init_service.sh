#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

OUT=$("$CUBALC" init --list 2>&1)
printf '%s\n' "$OUT" | grep -q 'service_boot'
printf '%s\n' "$OUT" | grep -qE -- '--service-boot|--service-session|--service-session'

OUT=$("$CUBALC" init "$TMPDIR/svc_demo" --service-boot --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"template":"service_boot"'
test -f "$TMPDIR/svc_demo.cubalc"
grep -q 'INCLUDE service_boot' "$TMPDIR/svc_demo.cubalc"

OUT=$("$CUBALC" run -q "$TMPDIR/svc_demo.cubalc" 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -q '"lib_service_boot":true'
printf '%s\n' "$OUT" | grep -q '"lib_service_session":true'

OUT=$("$CUBALC" recipe service_boot 2>&1)
printf '%s\n' "$OUT" | grep -q 'service_boot'

OUT=$("$CUBALC" which service_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'service_boot'

OUT=$("$CUBALC" libs service_boot 2>&1)
printf '%s\n' "$OUT" | grep -qi 'service_boot'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qiE -- '--service|service_boot'

OUT=$("$CUBALC" run -q programs/proof/1409_service_boot.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" init --service-session "$TMPDIR/ss" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"service_boot"'

OUT=$("$CUBALC" init --need-service "$TMPDIR/ns" --force 2>&1)
printf '%s\n' "$OUT" | grep -q '"template":"service_boot"'

echo "1409_cli_init_service: PASS"
