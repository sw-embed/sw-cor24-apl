#!/usr/bin/env bash
set -euo pipefail

# Build wrapper for sw-cor24-apl
# Delegates to the top-level build.sh

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

exec "$PROJECT_DIR/build.sh" "$@"
