#!/usr/bin/env bash
# CLI smoke: IFFIFTYP plate-50 duals
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="$ROOT/out/cubalc"
test -x "$BIN"
out=$("$BIN" run -q "$ROOT/programs/proof/1562_iffiftyp.cubalc")
echo "$out" | grep -q '"asserts_fail":0'
echo "$out" | grep -q '"ok":true'
echo OK_iffiftyp
