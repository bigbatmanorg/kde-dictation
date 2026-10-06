#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CONFIG="${KDICT_CONFIG:-$HOME/.config/kde-dictation/config.yaml}"

if [[ ! -x "$ROOT/build/kde-dictation" ]]; then
  "$ROOT/scripts/build.sh"
fi
exec "$ROOT/build/kde-dictation" --config "$CONFIG" "$@"
