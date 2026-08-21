#!/bin/sh
# CLI selftest: IFFIFTYNINEP plate-59 duals
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
BIN="$ROOT/out/cubalc"
[ -x "$BIN" ] || BIN="$ROOT/cubalc"
exec "$BIN" run "$ROOT/programs/proof/1571_iffiftyninep.cubalc"
