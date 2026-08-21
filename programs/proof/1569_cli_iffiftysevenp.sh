#!/bin/sh
# CLI selftest: IFFIFTYSEVENP plate-57 duals
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
BIN="$ROOT/out/cubalc"
[ -x "$BIN" ] || BIN="$ROOT/cubalc"
exec "$BIN" run "$ROOT/programs/proof/1569_iffiftysevenp.cubalc"
