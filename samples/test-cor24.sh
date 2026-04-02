#!/usr/bin/env bash
# Run .cor24 samples through COR24 emulator and compare with .expected
# Usage: ./samples/test-cor24.sh [pattern]

set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT="$(dirname "$DIR")"
PATTERN="${1:-}"

# Build first
bash "$PROJECT/build.sh" 2>&1 | tail -1

pass=0
fail=0
skip=0

for cor24 in "$DIR"/*.cor24; do
    base="$(basename "$cor24" .cor24)"
    expected="$DIR/${base}.expected"
    [ -n "$PATTERN" ] && [[ "$base" != *"$PATTERN"* ]] && continue

    if [ ! -f "$expected" ]; then
        printf "%-25s SKIP (no .expected)\n" "$base"
        skip=$((skip + 1))
        continue
    fi

    # Build UART input from .cor24 file (each line becomes a command)
    uart_input=""
    while IFS= read -r line || [ -n "$line" ]; do
        uart_input="${uart_input}${line}\\n"
    done < "$cor24"

    # Detect hardware test variants
    emu_flags=""
    [[ "$base" == *-pressed* ]] && emu_flags="--switch on"

    # Run on emulator, extract output lines (skip banner and prompts)
    raw=$(cor24-run --run "$PROJECT/build/apl.s" -n 20000000 \
        $emu_flags -u "$uart_input" 2>&1 | grep -A1000 "^UART output:" | tail -n +2) || true

    # Strip prompt lines, input echo, and emulator status lines
    actual=""
    while IFS= read -r line; do
        # Skip empty lines
        [ -z "$line" ] && continue
        # Skip prompt + echo lines (start with 6 spaces)
        [[ "$line" == "      "* ]] && continue
        # Skip emulator status lines
        [[ "$line" == "Executed "* ]] && continue
        [[ "$line" == "Assembled "* ]] && continue
        [[ "$line" == "Running "* ]] && continue
        [[ "$line" == "CPU halted"* ]] && continue
        actual="${actual}${line}
"
    done <<< "$raw"

    expected_text=$(cat "$expected")

    if [ "$actual" = "${expected_text}
" ] || [ "$actual" = "$expected_text" ]; then
        printf "%-25s PASS\n" "$base"
        pass=$((pass + 1))
    else
        printf "%-25s FAIL\n" "$base"
        echo "  expected:"
        echo "$expected_text" | sed 's/^/    |/'
        echo "  actual:"
        echo "$actual" | sed 's/^/    |/'
        fail=$((fail + 1))
    fi
done

# Batch mode tests: .a24 files with matching .expected
for apl in "$DIR"/batch-*.a24; do
    [ -f "$apl" ] || continue
    base="$(basename "$apl" .a24)"
    expected="$DIR/${base}.expected"
    [ -n "$PATTERN" ] && [[ "$base" != *"$PATTERN"* ]] && continue

    if [ ! -f "$expected" ]; then
        printf "%-25s SKIP (no .expected)\n" "$base"
        skip=$((skip + 1))
        continue
    fi

    # Run in batch mode
    raw=$(cor24-run --run "$PROJECT/build/apl.s" -n 20000000 \
        --load-binary "$apl@0x080000" --patch "0x09FF00=0x080000" \
        2>&1 | grep -A1000 "^UART output:" | tail -n +2) || true

    # Filter output lines
    actual=""
    while IFS= read -r line; do
        [ -z "$line" ] && continue
        [[ "$line" == "Executed "* ]] && continue
        [[ "$line" == "Assembled "* ]] && continue
        [[ "$line" == "Running "* ]] && continue
        [[ "$line" == "CPU halted"* ]] && continue
        [[ "$line" == "Loaded "* ]] && continue
        [[ "$line" == "Patched "* ]] && continue
        actual="${actual}${line}
"
    done <<< "$raw"

    expected_text=$(cat "$expected")

    if [ "$actual" = "${expected_text}
" ] || [ "$actual" = "$expected_text" ]; then
        printf "%-25s PASS\n" "$base"
        pass=$((pass + 1))
    else
        printf "%-25s FAIL\n" "$base"
        echo "  expected:"
        echo "$expected_text" | sed 's/^/    |/'
        echo "  actual:"
        echo "$actual" | sed 's/^/    |/'
        fail=$((fail + 1))
    fi
done

echo ""
echo "Results: $pass pass, $fail fail, $skip skip"
[ "$fail" -eq 0 ] || exit 1
