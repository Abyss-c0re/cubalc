#!/bin/sh
# INCLUDE LAST / string-var after PICKLIB
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q -e 'PICKLIB agent_boot
INCLUDE LAST
VERSION
ASSERT OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q 'agent_boot'

OUT=$("$CUBALC" run -q -e 'LET lib = "hold_seed"
INCLUDE ONCE lib
ASSERT OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# bare stem regression
OUT=$("$CUBALC" run -q -e 'INCLUDE ONCE agent_boot
ASSERT OK == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" run -q programs/proof/1308_include_var.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

# forms mention LAST/var
OUT=$("$CUBALC" forms include 2>&1) || OUT=$("$CUBALC" forms INCLUDE 2>&1)
printf '%s\n' "$OUT" | grep -qi 'INCLUDE'

echo "1308_cli_include_var: PASS"
