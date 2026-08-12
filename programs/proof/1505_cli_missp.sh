#!/bin/sh
# CLI smoke for MISSP/PRESENTP
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
BIN=${CUBALC_BIN:-"$ROOT/out/cubalc"}
export CUBALC_STATE=${CUBALC_STATE:-/tmp/cubalc_proof_1505_$$}
mkdir -p "$CUBALC_STATE"
cleanup() { rm -rf "$CUBALC_STATE"; }
trap cleanup EXIT
"$BIN" run -q "$ROOT/programs/proof/1505_missp.cubalc"
echo "1505_cli_missp PASS"
