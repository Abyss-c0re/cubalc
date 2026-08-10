#!/bin/sh
set -e
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
CUBALC="${CUBALC:-$ROOT/out/cubalc}"
cd "$ROOT"

OUT=$("$CUBALC" topic 2>&1)
printf '%s\n' "$OUT" | grep -q '"schema":"cubalc.topic.v1"'
printf '%s\n' "$OUT" | grep -q '"ok":true'
printf '%s\n' "$OUT" | grep -q '"topic":"general"'
printf '%s\n' "$OUT" | grep -qE '"tips_n":[1-9]'
printf '%s\n' "$OUT" | grep -qE '"forms_n":[1-9]'
printf '%s\n' "$OUT" | grep -q 'VERSION\|STATUS'

OUT=$("$CUBALC" topic cap 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"cap"'
printf '%s\n' "$OUT" | grep -q 'HASFORM'
printf '%s\n' "$OUT" | grep -q 'cap_boot\|FORMHINT\|NEEDFORMS'
printf '%s\n' "$OUT" | grep -q 'FORMHINT\|RELATED'

OUT=$("$CUBALC" topiccard fat 2>&1)
printf '%s\n' "$OUT" | grep -q '"topic":"fat"'
printf '%s\n' "$OUT" | grep -q 'VARROOM\|REMAIN_MS'

OUT=$("$CUBALC" help 2>&1)
printf '%s\n' "$OUT" | grep -qi 'topic'

OUT=$("$CUBALC" forms TOPIC 2>&1)
printf '%s\n' "$OUT" | grep -qi 'TOPIC'

OUT=$("$CUBALC" run -q -e 'TOPIC cap
ASSERT TOPIC_FORMS_N >= 5
ASSERT TOPIC_TIPS_N >= 3
SYS HASI "cubalc.topic.v1"
ASSERT LAST_N == 1
PASS' 2>&1)
printf '%s\n' "$OUT" | grep -q '"ok":true'

echo "1333_cli_topic: PASS"
