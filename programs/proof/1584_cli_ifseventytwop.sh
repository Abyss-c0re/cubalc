#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="${CUBALC_BIN:-$ROOT/out/cubalc}"
test -x "$BIN"
exec "$BIN" run "$ROOT/programs/proof/1584_ifseventytwop.cubalc"
