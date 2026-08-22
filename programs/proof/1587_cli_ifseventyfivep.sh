#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="${CUBALC_BIN:-$ROOT/cubalc}"
exec "$BIN" run "$ROOT/programs/proof/1587_ifseventyfivep.cubalc"
