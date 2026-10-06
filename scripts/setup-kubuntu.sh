#!/usr/bin/env bash
# One-command first-time setup for Kubuntu / KDE Plasma 6.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

"$ROOT/scripts/install-deps-kubuntu.sh"
"$ROOT/scripts/setup-uinput.sh"
"$ROOT/scripts/install-user.sh"

CONFIG="$HOME/.config/kde-dictation/config.yaml"
cat <<MSG

Setup complete.

If this script added you to the input group, log out of Plasma and log back in
before dictating. ydotoold is enabled and will start in the new session.

Open this private file and fill in only these values:

  $CONFIG

  transcription:
    base_url: https://YOUR-LITELLM-HOST
    api_key: YOUR_LITELLM_KEY

Launch KDE Dictation from the application menu, then assign its global shortcut
in Plasma System Settings → Keyboard → Shortcuts → KDE Dictation → Dictate.
MSG
