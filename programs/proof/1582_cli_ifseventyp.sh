#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="${CUBALC_BIN:-$ROOT/cubalc}"
exec "$BIN" "$ROOT/programs/proof/1582_ifseventyp.cubalc"
