#!/usr/bin/env bash
# Push CubalC to GitHub via HTTPS (username + PAT when prompted)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
git remote set-url origin https://github.com/Abyss-c0re/cubalc.git
git fetch origin
echo "Local:  $(git rev-parse --short HEAD)  ($(git log -1 --oneline))"
echo "Origin: $(git rev-parse --short origin/main 2>/dev/null || echo '?')"
echo "Status: $(git status -sb | head -1)"
echo ""
echo "Pushing main + tags (GitHub will ask for username + Personal Access Token)..."
git push -u origin main
git push origin --tags
echo "Done."
git status -sb | head -5
