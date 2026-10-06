#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"

"$ROOT/scripts/build.sh"
ctest --test-dir "$BUILD_DIR" --output-on-failure
python3 "$ROOT/tests/repo_contract_test.py"
