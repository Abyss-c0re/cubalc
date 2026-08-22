#!/bin/sh
# CLI selftest: IFSIXTYNINEP plate-69 duals
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
BIN="$ROOT/out/cubalc"
[ -x "$BIN" ] || BIN="$ROOT/cubalc"
[ -x "$BIN" ] || BIN="$ROOT/build/cubalc"
out=$("$BIN" run "$ROOT/programs/proof/1581_ifsixtyninep.cubalc")
echo "$out" | grep -q '"asserts_fail":0'
echo "$out" | grep -q '"ok":true'
echo OK_ifsixtyninep
