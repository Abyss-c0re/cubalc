#!/usr/bin/env bash
# Host adapter only — NOT the CubalC language.
# Decodes optional peer plates into plain env the language understands:
#   CUBALC_PEER0_DIGIT CUBALC_PEER1_DIGIT   (0..9 as text)
#   CUBALC_PEER0_BITS  CUBALC_PEER1_BITS    (file path or raw 01)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${CUBALC_BIN:-$ROOT/out/cubalc}"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state}"
mkdir -p "$CUBALC_STATE"

if [[ ! -x "$BIN" ]]; then
  make -C "$ROOT" all
fi

PEER_ROOT="${CUBALC_PEER_ROOT:-${CUBEBRAIN_ROOT:-}}"
DIGIT_KEY="${CUBALC_PEER_DIGIT_KEY:-digit}"

json_digit() {
  local f="$1" key="$2"
  [[ -f "$f" ]] || return 0
  sed -n "s/.*\"${key}\"[[:space:]]*:[[:space:]]*\\([0-9][0-9]*\\).*/\\1/p" "$f" | head -1
}

# Convenience layout only when PEER_ROOT is set and caller did not already export digits/bits
if [[ -n "$PEER_ROOT" ]]; then
  if [[ -z "${CUBALC_PEER0_DIGIT:-}" && -f "$PEER_ROOT/lab/prophecy/titan/LATEST.json" ]]; then
    CUBALC_PEER0_DIGIT="$(json_digit "$PEER_ROOT/lab/prophecy/titan/LATEST.json" "$DIGIT_KEY" || true)"
    export CUBALC_PEER0_DIGIT
  fi
  if [[ -z "${CUBALC_PEER1_DIGIT:-}" && -f "$PEER_ROOT/lab/prophecy/clanker/LATEST.json" ]]; then
    CUBALC_PEER1_DIGIT="$(json_digit "$PEER_ROOT/lab/prophecy/clanker/LATEST.json" "$DIGIT_KEY" || true)"
    export CUBALC_PEER1_DIGIT
  fi
  if [[ -z "${CUBALC_PEER0_BITS:-}" && -f "$PEER_ROOT/lab/prophecy/titan/io_bits.txt" ]]; then
    export CUBALC_PEER0_BITS="$PEER_ROOT/lab/prophecy/titan/io_bits.txt"
  fi
  if [[ -z "${CUBALC_PEER1_BITS:-}" && -f "$PEER_ROOT/lab/prophecy/clanker/io_bits.txt" ]]; then
    export CUBALC_PEER1_BITS="$PEER_ROOT/lab/prophecy/clanker/io_bits.txt"
  fi
fi

# Explicit CUBALC_PEER0_JSON still allowed (host decode → digit env)
if [[ -z "${CUBALC_PEER0_DIGIT:-}" && -n "${CUBALC_PEER0_JSON:-}" ]]; then
  export CUBALC_PEER0_DIGIT="$(json_digit "$CUBALC_PEER0_JSON" "$DIGIT_KEY" || true)"
fi
if [[ -z "${CUBALC_PEER1_DIGIT:-}" && -n "${CUBALC_PEER1_JSON:-}" ]]; then
  export CUBALC_PEER1_DIGIT="$(json_digit "$CUBALC_PEER1_JSON" "$DIGIT_KEY" || true)"
fi

echo "# peer_fold adapter digit0=${CUBALC_PEER0_DIGIT:--} digit1=${CUBALC_PEER1_DIGIT:--}"
"$BIN" peers "$@"

if [[ -n "${PROPHECY_CUBE_ROOT:-}" && -d "$PROPHECY_CUBE_ROOT/state" && -f "$CUBALC_STATE/cubalc_viz_frame.json" ]]; then
  cp -f "$CUBALC_STATE/cubalc_viz_frame.json" "$PROPHECY_CUBE_ROOT/state/cubalc_viz_frame.json" 2>/dev/null || true
  cp -f "$CUBALC_STATE/cubalc_viz_frame.json" "$PROPHECY_CUBE_ROOT/state/viz_frame.json" 2>/dev/null || true
  echo "# published viz → $PROPHECY_CUBE_ROOT/state/"
fi
