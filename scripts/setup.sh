#!/usr/bin/env bash
# One-command first-time setup for KDE Plasma 6 on Kubuntu/Ubuntu/Debian,
# Fedora KDE, Arch/Manjaro and openSUSE: dependencies, uinput access, build,
# install into ~/.local and autostart.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

"$ROOT/scripts/install-deps.sh"
if [[ "${XDG_SESSION_TYPE:-}" != "x11" ]]; then
  "$ROOT/scripts/setup-uinput.sh"
fi
"$ROOT/scripts/install-user.sh"

CONFIG="$HOME/.config/kde-dictation/config.yaml"
cat <<MSG

Setup complete.

Open this private file and fill in only these values:

  $CONFIG

  transcription:
    base_url: https://YOUR-LITELLM-HOST
    api_key: YOUR_LITELLM_KEY

Then launch KDE Dictation from the application menu (or log out/in: it autostarts)
and assign its global shortcut in System Settings -> Keyboard -> Shortcuts ->
KDE Dictation -> Dictate. Verify prerequisites any time with:

  kde-dictation --check
MSG
