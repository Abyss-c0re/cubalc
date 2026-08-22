#!/bin/sh
# CLI selftest: IFSIXTYEIGHTP plate-68 duals
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
BIN="$ROOT/out/cubalc"
[ -x "$BIN" ] || BIN="$ROOT/cubalc"
[ -x "$BIN" ] || BIN="$ROOT/build/cubalc"
out=$("$BIN" run "$ROOT/programs/proof/1580_ifsixtyeightp.cubalc")
echo "$out" | grep -q '"asserts_fail":0'
echo "$out" | grep -q '"ok":true'
echo OK_ifsixtyeightp
