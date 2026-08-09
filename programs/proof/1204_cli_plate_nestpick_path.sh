#!/bin/sh
# cubalc plate nestpick|nestomit — PICKOBJ/OMITOBJ duals with nest paths
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_nestpick_path"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"host":"cubeA","cfg":{"port":8080,"tls":0,"role":"edge","tmp":1},"flags":{"debug":0,"trace":1,"noise":9},"n":1}' | grep -q '"ok":true'

# shallow nestpick still works
OUT=$("$CUBALC" plate nestpick "$F" cfg port role)
printf '%s\n' "$OUT" | grep -q '"op":"nestpick"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":2'
OUT=$("$CUBALC" plate get "$F" cfg.port)
printf '%s\n' "$OUT" | grep -q 8080
# has exits 1 when miss — capture without set -e abort
OUT=$("$CUBALC" plate has "$F" cfg.tls || true)
printf '%s\n' "$OUT" | grep -q '"has_all":false'

# reseed cfg via path set (scalar)
"$CUBALC" plate set "$F" cfg.tls 0 | grep -q '"ok":true'
"$CUBALC" plate set "$F" cfg.tmp 1 | grep -q '"ok":true'
"$CUBALC" plate set "$F" cfg.port 8080 | grep -q '"ok":true'
"$CUBALC" plate set "$F" cfg.role edge | grep -q '"ok":true'

# deep path nestomit
OUT=$("$CUBALC" plate nestomit "$F" flags noise)
printf '%s\n' "$OUT" | grep -q '"op":"nestomit"'
printf '%s\n' "$OUT" | grep -q '"n":1'
OUT=$("$CUBALC" plate has "$F" flags.noise || true)
printf '%s\n' "$OUT" | grep -q '"has_all":false'
OUT=$("$CUBALC" plate get "$F" flags.trace)
printf '%s\n' "$OUT" | grep -q '"value":"1"'

# deep path nestpick — reseed flags via path set
"$CUBALC" plate set "$F" flags.debug 0 | grep -q '"ok":true'
"$CUBALC" plate set "$F" flags.trace 1 | grep -q '"ok":true'
"$CUBALC" plate set "$F" flags.noise 9 | grep -q '"ok":true'
OUT=$("$CUBALC" plate nestpick "$F" flags debug)
printf '%s\n' "$OUT" | grep -q '"n":1'
OUT=$("$CUBALC" plate get "$F" flags.debug)
printf '%s\n' "$OUT" | grep -q '"value":"0"'
OUT=$("$CUBALC" plate has "$F" flags.trace || true)
printf '%s\n' "$OUT" | grep -q '"has_all":false'

# nestkeys at path still peels
"$CUBALC" plate set "$F" flags.debug 0 | grep -q '"ok":true'
"$CUBALC" plate set "$F" flags.trace 1 | grep -q '"ok":true'
OUT=$("$CUBALC" plate nestkeys "$F" flags)
printf '%s\n' "$OUT" | grep -q debug
printf '%s\n' "$OUT" | grep -q trace

echo "1204_cli_plate_nestpick_path: PASS"
