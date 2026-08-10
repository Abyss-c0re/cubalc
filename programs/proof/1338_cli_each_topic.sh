#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" run -q programs/proof/1338_each_topic.cubalc 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

OUT=$("$CUBALC" forms "EACH TOPIC" 2>&1)
printf '%s\n' "$OUT" | grep -qi 'TOPIC'

OUT=$("$CUBALC" run -q -e 'EACH TOPIC MATCH "cap"
ASSERT TOPIC == "cap"
ASSERT EACHTOPICS_N == 1
END
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1338_cli_each_topic: PASS"
