#!/bin/sh
# cubalc plate CLI — agent plate file get/set/inc/keys/show without .cubalc
# Usability: shell/agent one-shots; dual of plate_boot/SETP/SAVEPLATE language surface.
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_plate_proof"
F="$ST/agent.json"
rm -rf "$ST"
mkdir -p "$ST"

"$CUBALC" plate set "$F" n 0 | grep -q '"ok":true'
"$CUBALC" plate set "$F" role worker | grep -q '"ok":true'
"$CUBALC" plate inc "$F" n 2 | grep -q '"value":2'
"$CUBALC" plate get "$F" n | grep -q '"value":"2"'
"$CUBALC" plate get "$F" missing OR fb | grep -q '"or":true'
"$CUBALC" plate keys "$F" | grep -q '"n":2'
"$CUBALC" plate show "$F" | grep -q '"op":"show"'
"$CUBALC" plate del "$F" role | grep -q '"removed":1'
"$CUBALC" plate "$F" | grep -q '"keys_n":1'
"$CUBALC" help 2>&1 | grep -q 'plate|jsonplate'
echo "cli_plate ok"
