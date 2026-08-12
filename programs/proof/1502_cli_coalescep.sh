#!/bin/sh
# CLI smoke for COALESCEP
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
BIN=${CUBALC_BIN:-"$ROOT/out/cubalc"}
export CUBALC_STATE=${CUBALC_STATE:-/tmp/cubalc_proof_1502_$$}
mkdir -p "$CUBALC_STATE"
cleanup() { rm -rf "$CUBALC_STATE"; }
trap cleanup EXIT
"$BIN" run -q "$ROOT/programs/proof/1502_coalescep.cubalc"
echo "1502_cli_coalescep PASS"
