#!/usr/bin/env bash
# Build and test the project inside a clean distribution container, using the
# same scripts a user runs. Example:
#   scripts/test-in-container.sh fedora:latest
#   scripts/test-in-container.sh docker.io/library/archlinux:latest
# Set CONTAINER_ENGINE=docker to use docker instead of podman.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ENGINE="${CONTAINER_ENGINE:-podman}"
IMAGE="${1:?usage: $0 <image>}"

"$ENGINE" run --rm -v "$ROOT:/src:ro,z" "$IMAGE" bash -euc '
  mkdir /work && tar -C /src --exclude=./build --exclude=./.git -cf - . | tar -C /work -xf -
  cd /work
  ./scripts/install-deps.sh
  ./scripts/test.sh
  ./build/kde-dictation --version
  # Exercise the user-install path in a throw-away HOME.
  HOME=/tmp/kdict-home ./scripts/install-user.sh >/dev/null
  test -x /tmp/kdict-home/.local/bin/kde-dictation
  test -f /tmp/kdict-home/.config/kde-dictation/config.yaml
  test -f /tmp/kdict-home/.config/autostart/org.amvc.kdedictation.desktop
  test -f /tmp/kdict-home/.local/share/metainfo/org.amvc.kdedictation.metainfo.xml
  echo "install-user: OK"
  QT_QPA_PLATFORM=offscreen ./build/kde-dictation --help >/dev/null
'
