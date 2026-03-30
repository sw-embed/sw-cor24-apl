#!/usr/bin/env bash
# Run all .apl samples through GNU APL and show output
# Usage: ./samples/test-gnu-apl.sh [pattern]
#   e.g. ./samples/test-gnu-apl.sh 09  (run only 09-*.apl)

set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
PATTERN="${1:-}"

pass=0
fail=0

for apl in "$DIR"/*.apl; do
    base="$(basename "$apl" .apl)"
    [ -n "$PATTERN" ] && [[ "$base" != *"$PATTERN"* ]] && continue

    printf "%-25s " "$base"
    output=$(apl --script --noCIN --noSV < "$apl" 2>/dev/null) || true
    echo "$output"
done
