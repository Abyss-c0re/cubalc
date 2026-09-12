#!/bin/bash
ulimit -s 65536 2>/dev/null || true
exec "$(dirname "$0")/cubalc" "$@"
