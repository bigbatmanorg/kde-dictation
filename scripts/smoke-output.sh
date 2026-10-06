#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CONFIG="${KDICT_CONFIG:-$HOME/.config/kde-dictation/config.yaml}"
TEXT="${1:-KDE dictation test: Příliš žluťoučký kůň — ľščťžýáíé.}"

if [[ ! -x "$ROOT/build/kde-dictation" ]]; then
  "$ROOT/scripts/build.sh"
fi

echo "You have 3 seconds to focus a target text field..."
exec "$ROOT/build/kde-dictation" --config "$CONFIG" --delay-ms 3000 --test-output "$TEXT"
