#!/bin/sh
# cubalc run -R / CUBALC_REQUIRE_VERSION — agent host version floor
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"

# current runtime satisfies 1.15 floor
OUT=$("$CUBALC" run -q -R 1.15 -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"require_version":"1.15"'

# env dual
OUT=$(CUBALC_REQUIRE_VERSION=1.15 "$CUBALC" run -q -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"require_version":"1.15"'

# impossible future floor fails before program body
OUT=$("$CUBALC" run -q -R 99.0 -e 'PASS' 2>&1 || true)
printf '%s\n' "$OUT" | grep -q '"ok":false'
printf '%s\n' "$OUT" | grep -q 'REQUIRE VERSION'
printf '%s\n' "$OUT" | grep -q '99.0'
printf '%s\n' "$OUT" | grep -q 'why_hint'

# CLI overrides env (still fail)
OUT=$(CUBALC_REQUIRE_VERSION=1.0 "$CUBALC" run -q -R 99.0 -e 'PASS' 2>&1 || true)
printf '%s\n' "$OUT" | grep -q '"ok":false'

# env catalog
OUT=$("$CUBALC" env REQUIRE 2>&1)
printf '%s\n' "$OUT" | grep -q 'CUBALC_REQUIRE_VERSION'

# top-level -R
OUT=$("$CUBALC" -R 1.15 -q -e 'PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1266_cli_require_version: PASS"
