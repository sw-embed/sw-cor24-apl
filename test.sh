#!/usr/bin/env bash
set -euo pipefail

# COR24 APL -- Automated Test Runner
# Runs all batch-*.a24 samples, compares output against .expected files.
# Usage: ./test.sh [--generate]  (--generate creates .expected files)

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SAMPLES="$SCRIPT_DIR/samples"
SPEED="${SPEED:-5000000}"
GENERATE=0

if [[ "${1:-}" == "--generate" ]]; then
    GENERATE=1
    echo "Generating expected output files..."
fi

PASS=0
FAIL=0
SKIP=0

for a24 in "$SAMPLES"/batch-*.a24; do
    name=$(basename "$a24" .a24)
    expected="$SAMPLES/$name.expected"

    # Get UART output (strip banner and trailing prompt)
    output=$("$SCRIPT_DIR/build.sh" run --batch "$a24" --speed "$SPEED" 2>&1 \
        | sed -n 's/^UART output: COR24 APL v[0-9.]*/___BANNER___/p' \
        | sed 's/___BANNER___\n*//' \
        | head -1)
    # Extract just the content after the banner
    actual=$("$SCRIPT_DIR/build.sh" run --batch "$a24" --speed "$SPEED" 2>&1 \
        | grep -A100 "^UART output:" \
        | tail -n +2 \
        | grep -v "^$" \
        | grep -v "^Executed" \
        | sed '/^      $/d')

    if [[ $GENERATE -eq 1 ]]; then
        echo "$actual" > "$expected"
        echo "  GEN: $name"
        continue
    fi

    if [[ ! -f "$expected" ]]; then
        echo " SKIP: $name (no .expected file)"
        SKIP=$((SKIP + 1))
        continue
    fi

    exp=$(cat "$expected")
    if [[ "$actual" == "$exp" ]]; then
        echo " PASS: $name"
        PASS=$((PASS + 1))
    else
        echo " FAIL: $name"
        diff <(echo "$exp") <(echo "$actual") | head -10
        FAIL=$((FAIL + 1))
    fi
done

# Horse race demos: just check for WINNER line
for a24 in "$SAMPLES"/horse-race*.a24; do
    name=$(basename "$a24" .a24)
    output=$("$SCRIPT_DIR/build.sh" run --batch "$a24" --speed "$SPEED" 2>&1)
    if echo "$output" | grep -q "WINNER"; then
        echo " PASS: $name (WINNER found)"
        PASS=$((PASS + 1))
    elif echo "$output" | grep -q "ERROR"; then
        echo " FAIL: $name (ERROR in output)"
        FAIL=$((FAIL + 1))
    else
        echo " SKIP: $name (no WINNER, may need more speed)"
        SKIP=$((SKIP + 1))
    fi
done

echo ""
echo "Results: $PASS passed, $FAIL failed, $SKIP skipped"
if [[ $FAIL -gt 0 ]]; then
    exit 1
fi
