#!/usr/bin/env bash
# Unite LOVR + crimson OpenGL under Cube Law (one matrix, free devices).
set -euo pipefail
PC="${PROPHECY_CUBE_ROOT:-/home/voldemar/Dev/lab/prophecy_cube}"
export PROPHECY_CUBE_ROOT="$PC"
export CUBALC_STATE="${CUBALC_STATE:-$PC/cubalc/state}"
export CUBEBRAIN_VIZ_CELLS="${CUBEBRAIN_VIZ_CELLS:-/tmp/cubebrain_viz/cells.bin}"
export HOLD_FLASH=1
export DISPLAY="${DISPLAY:-:0}"
BIN="$PC/cubalc/out/cubalc"
CB="${CUBEBRAIN_ROOT:-/home/voldemar/Dev/cubebrain}"

mkdir -p "$CUBALC_STATE" "$PC/state" /tmp/cubebrain_viz

echo "# unite_visual_faces · HOLD_FLASH · share=smx · All Hail Cube"
if [[ ! -x "$BIN" ]] || [[ "$PC/cubalc/src/cubalc_core.c" -nt "$BIN" ]]; then
  make -C "$PC/cubalc" -j"$(nproc)" all
fi

# Core decides I/O: run law unity program → publish_united
"$BIN" run "$PC/cubalc/programs/unite_visual.cubalc" || true
# flow pulse so energy moves on binary wires
"$BIN" flow 4 0 || true

echo "# faces published:"
for f in \
  "$PC/state/cubalc_viz_frame.json" \
  "$PC/state/viz_frame.json" \
  "$PC/state/VISUAL_UNITY.json" \
  "$CUBALC_STATE/cubalc_viz_frame.json" \
  "$CUBEBRAIN_VIZ_CELLS"
do
  if [[ -f "$f" ]]; then
    sz=$(wc -c < "$f" | tr -d ' ')
    echo "  ok  $f  ($sz B)"
  else
    echo "  miss $f"
  fi
done

# Optional: fire crimson desktop face (devices free — not required for unity plate)
if [[ "${UNITE_FIRE_GL:-1}" == "1" ]]; then
  if [[ -x "$PC/scripts/cube_gl_cubalc.sh" ]]; then
    pkill -x cube_gl 2>/dev/null || true
    sleep 0.2
    nohup "$PC/scripts/cube_gl_cubalc.sh" > /tmp/cube_gl_cubalc.log 2>&1 &
    echo "# fired cube_gl crimson face  pid=$!"
  elif [[ -x "$CB/viz/cube_gl" ]]; then
    pkill -x cube_gl 2>/dev/null || true
    nohup env GLFW_PLATFORM="${GLFW_PLATFORM:-x11}" \
      "$CB/viz/cube_gl" --cubalc "$PC/state/cubalc_viz_frame.json" --levitate \
      > /tmp/cube_gl_cubalc.log 2>&1 &
    echo "# fired cube_gl --cubalc  pid=$!"
  fi
fi

if [[ -f "$PC/state/VISUAL_UNITY.json" ]]; then
  echo "# VISUAL_UNITY plate:"
  cat "$PC/state/VISUAL_UNITY.json"
fi
echo "# LOVR already drinks cubalc_viz_frame when present — united under Cube Law."
echo "# All Hail the Cube · All Hail NexusCore"
