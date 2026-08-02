#!/usr/bin/env bash
# Manifest CubeBrain on the CubalC board — fold peer digits, publish viz + plate.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${CUBALC_BIN:-$ROOT/out/cubalc}"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state}"
export CUBEBRAIN_ROOT="${CUBEBRAIN_ROOT:-${HOME}/Dev/cubebrain}"
export PROPHECY_CUBE_ROOT="${PROPHECY_CUBE_ROOT:-${HOME}/Dev/lab/prophecy_cube}"
mkdir -p "$CUBALC_STATE"

if [[ ! -x "$BIN" ]]; then
  make -C "$ROOT" all
fi

echo "# cubalc oversee cubebrain root=$CUBEBRAIN_ROOT"
"$BIN" oversee "$@"

# Machine plate for Commander / viz / agents
TS="$(date -Iseconds 2>/dev/null || date '+%Y-%m-%dT%H:%M:%S%z')"
TITAN_JSON="$CUBEBRAIN_ROOT/lab/prophecy/titan/LATEST.json"
CLANKER_JSON="$CUBEBRAIN_ROOT/lab/prophecy/clanker/LATEST.json"
d_titan=""; d_clanker=""
[[ -f "$TITAN_JSON" ]] && d_titan="$(sed -n 's/.*"digit"[[:space:]]*:[[:space:]]*\([0-9][0-9]*\).*/\1/p' "$TITAN_JSON" | head -1)"
[[ -f "$CLANKER_JSON" ]] && d_clanker="$(sed -n 's/.*"digit"[[:space:]]*:[[:space:]]*\([0-9][0-9]*\).*/\1/p' "$CLANKER_JSON" | head -1)"

UNITY="0"
CUBES="0"
if [[ -f "$CUBALC_STATE/cubalc_viz_frame.json" ]]; then
  UNITY="$(sed -n 's/.*"unity"[[:space:]]*:[[:space:]]*\([0-9.]*\).*/\1/p' "$CUBALC_STATE/cubalc_viz_frame.json" | head -1)"
  CUBES="$(sed -n 's/.*"n_cubes"[[:space:]]*:[[:space:]]*\([0-9]*\).*/\1/p' "$CUBALC_STATE/cubalc_viz_frame.json" | head -1)"
  [[ -z "$CUBES" ]] && CUBES="$(sed -n 's/.*"cubes"[[:space:]]*:[[:space:]]*\([0-9]*\).*/\1/p' "$CUBALC_STATE/cubalc_viz_frame.json" | head -1)"
fi

PLATE="$CUBALC_STATE/OVERSEE_CUBEBRAIN.json"
cat > "$PLATE" <<EOF
{
  "schema": "cubalc.oversee.cubebrain.v1",
  "ok": true,
  "ts": "$TS",
  "language": "CubalC",
  "version": "1.3.0-c3",
  "law": {
    "cube_is_sot": true,
    "cube_overrides_surfaces": true,
    "cubalc_role": "language + play board + CBLC machine (oversee)",
    "cubebrain_role": "prophecy pipeline · algocube · titan/clanker IO matrix (peer, not deleted)",
    "nexuscore_role": "station hive SMX SoT",
    "commander": "only BlackCube Commander overrides NexusCore",
    "hold_flash": 1,
    "visual": "cubes",
    "not": "lego"
  },
  "progress": {
    "cubalc_version": "1.3.0-c3",
    "cubalc_live": true,
    "digit_auto_fold": true,
    "setdigit": true,
    "foldbits": true,
    "sys_num_json": true,
    "spawn_cubebrain": true,
    "cubalc_n_cubes": ${CUBES:-0},
    "cubalc_unity": ${UNITY:-0},
    "cubebrain_titan_digit": ${d_titan:-null},
    "cubebrain_clanker_digit": ${d_clanker:-null},
    "override_status": "MANIFEST — CubalC folds CubeBrain digits into matrix; peer pipeline still independent"
  },
  "next": [
    "single viz writer (prefer cubalc_viz_frame when present)",
    "titan-commander NexusCore atom online"
  ]
}
EOF

# Publish to prophecy_cube state for viz when root set
if [[ -n "${PROPHECY_CUBE_ROOT:-}" && -d "$PROPHECY_CUBE_ROOT/state" ]]; then
  cp -f "$CUBALC_STATE/cubalc_viz_frame.json" "$PROPHECY_CUBE_ROOT/state/cubalc_viz_frame.json" 2>/dev/null || true
  cp -f "$CUBALC_STATE/cubalc_viz_frame.json" "$PROPHECY_CUBE_ROOT/state/viz_frame.json" 2>/dev/null || true
  cp -f "$PLATE" "$PROPHECY_CUBE_ROOT/state/OVERSEE_CUBEBRAIN.json" 2>/dev/null || true
  echo "# published viz+plate → $PROPHECY_CUBE_ROOT/state/"
fi

echo "# plate $PLATE"
cat "$PLATE"
