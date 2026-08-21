#!/bin/sh
# CLI selftest: IFFIFTYFIVEP plate-55 duals
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
BIN="$ROOT/out/cubalc"
[ -x "$BIN" ] || BIN="$ROOT/cubalc"
exec "$BIN" run "$ROOT/programs/proof/1567_iffiftyfivep.cubalc"
