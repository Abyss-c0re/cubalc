#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export CUBALC_STATE="${CUBALC_STATE:-$ROOT/state}"
make -C "$ROOT" all
echo "======== CUBALC SCHOOL → CREATOR ========"
bash "$ROOT/school/run_student.sh" "hive-student-001"
echo
echo "======== DIPLOMA ========"
cat "$ROOT/school/students/hive-student-001/DIPLOMA.json"
echo
echo "======== SHOWCASE ========"
cat "$ROOT/school/students/hive-student-001/CREATOR_SHOWCASE.md"
