#!/usr/bin/env bash
set -euo pipefail

# COR24 APL Interpreter -- Build Script
#
# Usage:
#   ./build.sh              Build only (compile + assemble check)
#   ./build.sh run          Build and run on emulator
#   ./build.sh run --terminal   Build and run in interactive terminal mode
#   ./build.sh run --batch <file.apl>  Load APL image and run in batch mode
#   ./build.sh clean        Remove build artifacts

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
SRC_DIR="$SCRIPT_DIR/src"

# Toolchain
TC24R="tc24r"
COR24_RUN="cor24-run"
TC24R_INCLUDE="/Users/mike/github/sw-embed/sw-cor24-x-tinyc/include"

# Source and output
MAIN_C="$SRC_DIR/main.c"
MAIN_S="$BUILD_DIR/apl.s"

# ---- Functions ----

build() {
    mkdir -p "$BUILD_DIR"

    echo "=== Compiling ==="
    "$TC24R" "$MAIN_C" -o "$MAIN_S" -I "$TC24R_INCLUDE"
    echo "  $MAIN_C -> $MAIN_S"
    echo ""
}

# APL image format constants (must match src/main.c)
APL_IMAGE_PTR="0x09FF00"
APL_IMAGE_BASE="0x080000"

run() {
    build

    # Check for --batch <file.apl> argument
    local batch_file=""
    local extra_args=()
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --batch)
                shift
                batch_file="$1"
                shift
                ;;
            *)
                extra_args+=("$1")
                shift
                ;;
        esac
    done

    echo "=== Running on COR24 emulator ==="
    echo ""

    if [[ -n "$batch_file" ]]; then
        if [[ ! -f "$batch_file" ]]; then
            echo "Error: APL image file not found: $batch_file"
            exit 1
        fi
        echo "  Loading APL image: $batch_file"
        echo "  Image address: $APL_IMAGE_BASE"
        echo ""
        "$COR24_RUN" --run "$MAIN_S" \
            --load-binary "$batch_file@$APL_IMAGE_BASE" \
            --patch "$APL_IMAGE_PTR=$APL_IMAGE_BASE" \
            ${extra_args[@]+"${extra_args[@]}"}
    else
        "$COR24_RUN" --run "$MAIN_S" ${extra_args[@]+"${extra_args[@]}"}
    fi
}

clean() {
    rm -rf "$BUILD_DIR"
    echo "Cleaned build artifacts."
}

# ---- Main ----

CMD="${1:-build}"

case "$CMD" in
    build)
        build
        echo "Build OK."
        ;;
    run)
        shift
        run "$@"
        ;;
    clean)
        clean
        ;;
    *)
        echo "Usage: $0 {build|run|clean}"
        exit 1
        ;;
esac
