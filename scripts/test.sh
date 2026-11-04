#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
# Tests must run headless (CI containers, SSH sessions).
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"

"$ROOT/scripts/build.sh"
ctest --test-dir "$BUILD_DIR" --output-on-failure
python3 "$ROOT/tests/repo_contract_test.py"
