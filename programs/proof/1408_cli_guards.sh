#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" guards 2>&1)
printf '%s\n' "$OUT" | grep -q 'class_guard'
printf '%s\n' "$OUT" | grep -q 'method_guard'
printf '%s\n' "$OUT" | grep -q 'obj_guard'
printf '%s\n' "$OUT" | grep -q 'HARDFAIL\|hardfail'
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.guards.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"hardfail":true\|"hardfail": true\|hardfail.:true'

OUT=$("$CUBALC" guards oop 2>&1)
printf '%s\n' "$OUT" | grep -q 'class_guard'
printf '%s\n' "$OUT" | grep -q 'method_guard'
# filter should drop non-oop when plane is oop - path_guard should be absent or fewer
printf '%s\n' "$OUT" | grep -q 'field_guard'

OUT=$("$CUBALC" contracts class 2>&1)
printf '%s\n' "$OUT" | grep -q 'class_guard'
printf '%s\n' "$OUT" | grep -q 'cubalc.guards.v1'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qiE 'guards|contracts'

OUT=$("$CUBALC" doctor 2>&1)
printf '%s\n' "$OUT" | grep -qi 'guards'

echo "1408_cli_guards: PASS"
