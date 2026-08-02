#!/usr/bin/env bash
# Fire the cool crimson lattice State Matrix on the same SoT as viz.
# SoT: state/cubalc_viz_frame.json (cube.viz_frame.v1)
set -euo pipefail
PC="${PROPHECY_CUBE_ROOT:-/home/voldemar/Dev/lab/prophecy_cube}"
CB="${CUBEBRAIN_ROOT:-/home/voldemar/Dev/cubebrain}"
CUBE_GL="${CUBE_GL:-$CB/viz/lattice}"
FRAME="${CUBALC_VIZ_FRAME:-}"
export DISPLAY="${DISPLAY:-:0}"

if [[ -z "$FRAME" ]]; then
  # Prefer matrix SoT (cubalc_viz_frame) over lean viz_frame without bits
  for c in \
    "$PC/state/cubalc_viz_frame.json" \
    "$PC/cubalc/state/cubalc_viz_frame.json" \
    "$PC/state/viz_frame.json" \
    "$PC/cubalc/state/viz_frame.json"
  do
    if [[ -f "$c" ]]; then FRAME=$c; break; fi
  done
fi

if [[ -z "${FRAME:-}" || ! -f "$FRAME" ]]; then
  echo "lattice_viz: no cubalc_viz_frame.json — boot CubalC flow first" >&2
  echo "  e.g. $PC/scripts/cubalc_cube_flow.sh" >&2
  exit 1
fi

# Rebuild if source newer than binary
if [[ ! -x "$CUBE_GL" || "$CB/viz/lattice.c" -nt "$CUBE_GL" ]]; then
  echo "lattice_viz: building lattice…"
  make -C "$CB/viz" -j"$(nproc)"
fi

# Also project cells.bin for any legacy --cells consumers
python3 "$PC/scripts/cubalc_to_cells.py" --frame "$FRAME" \
  --out "${CUBEBRAIN_VIZ_CELLS:-/tmp/cubebrain_viz/cells.bin}" || true

echo "lattice_viz: crimson lattice ← $FRAME"
echo "  viz + lattice share cube.viz_frame.v1  ·  All Hail Cube"
# Prefer X11/XWayland so the window sits on the same desk surface as viz.
# Native Wayland also works; set CUBALC_CUBE_GL_WAYLAND=1 to keep it.
if [[ "${CUBALC_CUBE_GL_WAYLAND:-0}" != "1" ]]; then
  export GLFW_PLATFORM="${GLFW_PLATFORM:-x11}"
fi
exec env CUBALC_VIZ_FRAME="$FRAME" \
  "$CUBE_GL" --cubalc "$FRAME" --levitate "$@"
