#!/bin/sh
# CLI smoke for HITKEYP
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
BIN=${CUBALC_BIN:-"$ROOT/out/cubalc"}
export CUBALC_STATE=${CUBALC_STATE:-/tmp/cubalc_proof_1503_$$}
mkdir -p "$CUBALC_STATE"
cleanup() { rm -rf "$CUBALC_STATE"; }
trap cleanup EXIT
"$BIN" run -q "$ROOT/programs/proof/1503_hitkeyp.cubalc"
echo "1503_cli_hitkeyp PASS"
