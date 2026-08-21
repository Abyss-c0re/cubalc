#!/usr/bin/env bash
# CLI smoke: IFFORTYNINEP plate-49 duals
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="$ROOT/out/cubalc"
test -x "$BIN"
out=$("$BIN" run -q "$ROOT/programs/proof/1561_iffortyninep.cubalc")
echo "$out" | grep -q '"asserts_fail":0'
echo "$out" | grep -q '"ok":true'
echo OK_iffortyninep
