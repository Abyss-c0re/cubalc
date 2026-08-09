#!/bin/sh
# cubalc plate default|toggle — DEFAULTP/TOGGLEP CLI duals (paths ok)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_def_tog"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate ensure "$F" '{"host":"cubeA","flags":{"debug":0},"n":1}' | grep -q '"ok":true'

# default insert nest path
OUT=$("$CUBALC" plate default "$F" cfg.port 8080)
printf '%s\n' "$OUT" | grep -q '"op":"default"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"inserted":true'
printf '%s\n' "$OUT" | grep -q '"kept":false'
OUT=$("$CUBALC" plate get "$F" cfg.port)
printf '%s\n' "$OUT" | grep -q '"value":"8080"'

# default keep (no clobber)
OUT=$("$CUBALC" plate default "$F" cfg.port 9999)
printf '%s\n' "$OUT" | grep -q '"kept":true'
printf '%s\n' "$OUT" | grep -q '"inserted":false'
OUT=$("$CUBALC" plate get "$F" cfg.port)
printf '%s\n' "$OUT" | grep -q '"value":"8080"'

# default existing shallow keep
OUT=$("$CUBALC" plate default "$F" host other)
printf '%s\n' "$OUT" | grep -q '"kept":true'
OUT=$("$CUBALC" plate get "$F" host)
printf '%s\n' "$OUT" | grep -q 'cubeA'

# toggle existing path
OUT=$("$CUBALC" plate toggle "$F" flags.debug)
printf '%s\n' "$OUT" | grep -q '"op":"toggle"'
printf '%s\n' "$OUT" | grep -q '"value":1'
printf '%s\n' "$OUT" | grep -q '"prev":0'
OUT=$("$CUBALC" plate get "$F" flags.debug)
printf '%s\n' "$OUT" | grep -q '"value":"1"'

OUT=$("$CUBALC" plate toggle "$F" flags.debug)
printf '%s\n' "$OUT" | grep -q '"value":0'
printf '%s\n' "$OUT" | grep -q '"prev":1'

# toggle create path (miss→1)
OUT=$("$CUBALC" plate toggle "$F" flags.verbose)
printf '%s\n' "$OUT" | grep -q '"value":1'
OUT=$("$CUBALC" plate get "$F" flags.verbose)
printf '%s\n' "$OUT" | grep -q '"value":"1"'

# aliases
OUT=$("$CUBALC" plate ensurekey "$F" cfg.tls 0)
printf '%s\n' "$OUT" | grep -q '"op":"default"'
printf '%s\n' "$OUT" | grep -q '"inserted":true'

OUT=$("$CUBALC" plate flip "$F" flags.verbose)
printf '%s\n' "$OUT" | grep -q '"op":"toggle"'
printf '%s\n' "$OUT" | grep -q '"value":0'

# help
OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'default'
printf '%s\n' "$OUT" | grep -q 'toggle'

echo "1199_cli_plate_default_toggle: PASS"
