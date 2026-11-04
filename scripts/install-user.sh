#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
PREFIX="${PREFIX:-$HOME/.local}"
CONFIG_DIR="$HOME/.config/kde-dictation"
AUTOSTART_DIR="$HOME/.config/autostart"

"$ROOT/scripts/build.sh"
cmake --install "$BUILD_DIR" --prefix "$PREFIX"

mkdir -p "$CONFIG_DIR" "$AUTOSTART_DIR"
if [[ ! -e "$CONFIG_DIR/config.yaml" ]]; then
  cp "$ROOT/config.example.yaml" "$CONFIG_DIR/config.yaml"
  chmod 600 "$CONFIG_DIR/config.yaml"
  echo "Created private config: $CONFIG_DIR/config.yaml"
else
  chmod 600 "$CONFIG_DIR/config.yaml"
  echo "Keeping existing $CONFIG_DIR/config.yaml"
fi

cat > "$AUTOSTART_DIR/org.amvc.kdedictation.desktop" <<DESKTOP
[Desktop Entry]
Type=Application
Name=KDE Dictation
Comment=Global shortcut dictation for KDE Plasma
Exec=$PREFIX/bin/kde-dictation
Icon=audio-input-microphone
Terminal=false
X-KDE-autostart-after=panel
DESKTOP

case ":$PATH:" in
  *":$PREFIX/bin:"*) ;;
  *) echo "note: $PREFIX/bin is not on your PATH; add it to run 'kde-dictation --check' from a terminal." ;;
esac
echo "Installed $PREFIX/bin/kde-dictation"
echo "Autostart: $AUTOSTART_DIR/org.amvc.kdedictation.desktop"
echo "Configure the shortcut in Plasma after the app has registered its Dictate action."
echo
echo "Edit these two values in $CONFIG_DIR/config.yaml:"
echo "  transcription.base_url: https://YOUR-LITELLM-HOST"
echo "  transcription.api_key: YOUR_LITELLM_KEY"
