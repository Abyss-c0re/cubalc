#!/bin/sh
# CLI selftest: IFSIXTYTWOP plate-62 duals
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
BIN="$ROOT/out/cubalc"
[ -x "$BIN" ] || BIN="$ROOT/cubalc"
[ -x "$BIN" ] || BIN="$ROOT/build/cubalc"
exec "$BIN" run "$ROOT/programs/proof/1574_ifsixtytwop.cubalc"
