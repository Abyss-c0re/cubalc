#!/bin/sh
# CLI smoke for HITVALP
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
BIN=${CUBALC_BIN:-"$ROOT/out/cubalc"}
export CUBALC_STATE=${CUBALC_STATE:-/tmp/cubalc_proof_1504_$$}
mkdir -p "$CUBALC_STATE"
cleanup() { rm -rf "$CUBALC_STATE"; }
trap cleanup EXIT
"$BIN" run -q "$ROOT/programs/proof/1504_hitvalp.cubalc"
echo "1504_cli_hitvalp PASS"
