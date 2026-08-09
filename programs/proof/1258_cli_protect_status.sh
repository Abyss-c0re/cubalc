#!/bin/sh
# cubalc protect status — agent readiness plate (no board/smx)
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" protect status)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.protect_status.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"mode":"status"'
printf '%s\n' "$OUT" | grep -q '"hold_flash":1'
printf '%s\n' "$OUT" | grep -q 'hold_flash_means'
printf '%s\n' "$OUT" | grep -q 'device_firmware'
printf '%s\n' "$OUT" | grep -q '"program_present":true'
printf '%s\n' "$OUT" | grep -q '"docs_core_protect":true'
printf '%s\n' "$OUT" | grep -q '"docs_hold_flash":true'
printf '%s\n' "$OUT" | grep -q '"modular_lang":true'
printf '%s\n' "$OUT" | grep -q '"smx_key_configured"'
printf '%s\n' "$OUT" | grep -q '"why_hint"'
printf '%s\n' "$OUT" | grep -q '"next"'
printf '%s\n' "$OUT" | grep -q 'protect all'
printf '%s\n' "$OUT" | grep -q 'doctor'
printf '%s\n' "$OUT" | grep -q '"http_required":false'
printf '%s\n' "$OUT" | grep -q 'CORE_PROTECT'

# aliases
OUT2=$("$CUBALC" protect summary)
printf '%s\n' "$OUT2" | grep -q '"mode":"status"'
OUT3=$("$CUBALC" protect info)
printf '%s\n' "$OUT3" | grep -q '"why_hint"'

echo "1258_cli_protect_status: PASS"
