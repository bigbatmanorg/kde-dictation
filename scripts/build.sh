#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"

GENERATOR=()
if command -v ninja >/dev/null 2>&1; then GENERATOR=(-G Ninja); fi

cmake -S "$ROOT" -B "$BUILD_DIR" "${GENERATOR[@]}" \
  -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-RelWithDebInfo}" \
  -DBUILD_TESTING=ON
cmake --build "$BUILD_DIR" -j"$(nproc)"

echo "Built: $BUILD_DIR/kde-dictation"
