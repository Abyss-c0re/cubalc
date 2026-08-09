#!/bin/sh
# cubalc plate unflat — UNFLATKV dual
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
ST="${CUBALC_STATE:-$ROOT/state/meaningful_iter}/mi_cli_unflat"
F="$ST/agent.json"
BAG="$ST/flat.txt"
rm -rf "$ST"
mkdir -p "$ST"

# create empty plate file
"$CUBALC" plate ensure "$F" '{}' | grep -q '"ok":true'

# path:val args
OUT=$("$CUBALC" plate unflat "$F" host:cubeA cfg.meta.role:worker cfg.port:8080 n:3 empty:)
printf '%s\n' "$OUT" | grep -q '"op":"unflat"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"n":5'
printf '%s\n' "$OUT" | grep -q '"host":"cubeA"'
printf '%s\n' "$OUT" | grep -q '"role":"worker"'

# verify with get
OUT=$("$CUBALC" plate get "$F" cfg.meta.role)
printf '%s\n' "$OUT" | grep -q worker

# @file bag
printf '%s\n' 'cfg.meta.zone:Z' 'ok:true' > "$BAG"
OUT=$("$CUBALC" plate unflat "$F" @"$BAG")
printf '%s\n' "$OUT" | grep -q '"n":2'
OUT=$("$CUBALC" plate get "$F" cfg.meta.zone)
printf '%s\n' "$OUT" | grep -q Z
OUT=$("$CUBALC" plate get "$F" ok)
printf '%s\n' "$OUT" | grep -q true

# UNDER relative
G="$ST/peer.json"
"$CUBALC" plate ensure "$G" '{}' | grep -q '"ok":true'
OUT=$("$CUBALC" plate unflat "$G" UNDER cfg role:edge port:9)
printf '%s\n' "$OUT" | grep -q '"nest":"cfg"'
printf '%s\n' "$OUT" | grep -q '"n":2'
OUT=$("$CUBALC" plate get "$G" cfg.role)
printf '%s\n' "$OUT" | grep -q edge

# help
OUT=$("$CUBALC" plate 2>&1 || true)
printf '%s\n' "$OUT" | grep -q 'unflat'

echo "1212_cli_plate_unflat: PASS"
