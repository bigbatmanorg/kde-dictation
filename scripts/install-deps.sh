#!/usr/bin/env bash
# Set KDICT_ZYPPER_INSTALL_ARGS=--force-resolution to let zypper resolve
# package conflicts (CI containers) instead of aborting.
# Install build and runtime dependencies on Debian/Ubuntu (Kubuntu), Fedora,
# Arch/Manjaro and openSUSE. Usage: install-deps.sh [--build-only]
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=lib/distro.sh
source "$ROOT/scripts/lib/distro.sh"

FAMILY="${KDICT_DISTRO_FAMILY:-$(detect_family)}"
echo "Detected distribution family: $FAMILY"

case "$FAMILY" in
  debian)
    as_root apt-get update
    as_root env DEBIAN_FRONTEND=noninteractive apt-get install -y \
      build-essential cmake ninja-build pkg-config \
      qt6-base-dev qt6-multimedia-dev qt6-websockets-dev \
      libkf6globalaccel-dev libkf6statusnotifieritem-dev libyaml-cpp-dev \
      python3
    ;;
  fedora)
    as_root dnf install -y \
      gcc-c++ cmake ninja-build pkgconf-pkg-config \
      qt6-qtbase-devel qt6-qtmultimedia-devel qt6-qtwebsockets-devel \
      kf6-kglobalaccel-devel kf6-kstatusnotifieritem-devel yaml-cpp-devel \
      python3
    ;;
  arch)
    # Fresh containers ship without a synced package database.
    [[ -e /var/lib/pacman/sync/core.db ]] || as_root pacman -Sy --noconfirm
    as_root pacman -S --needed --noconfirm \
      base-devel cmake ninja pkgconf \
      qt6-base qt6-multimedia qt6-websockets \
      kglobalaccel kstatusnotifieritem yaml-cpp \
      python
    ;;
  suse)
    as_root zypper --non-interactive install ${KDICT_ZYPPER_INSTALL_ARGS:-} \
      gcc-c++ cmake ninja pkgconf-pkg-config \
      qt6-base-devel qt6-multimedia-devel qt6-websockets-devel \
      kf6-kglobalaccel-devel kf6-kstatusnotifieritem-devel yaml-cpp-devel \
      python3
    ;;
  *)
    cat >&2 <<'MSG'
Unsupported distribution. Install these (names vary) and re-run:
  C++20 compiler, cmake >= 3.24, ninja, Qt 6 (base, multimedia, websockets),
  KDE Frameworks 6: KGlobalAccel + KStatusNotifierItem, yaml-cpp,
  ydotool (Wayland) and/or xdotool (X11), python3
MSG
    exit 1
    ;;
esac

# Key-injection helpers are optional at install time: not every distribution
# packages both (e.g. Debian 13 has no ydotool). Install each one separately so
# a missing package only produces a warning.
install_optional() {
  local pkg="$1"
  case "$FAMILY" in
    debian) as_root env DEBIAN_FRONTEND=noninteractive apt-get install -y "$pkg" ;;
    fedora) as_root dnf install -y "$pkg" ;;
    arch)   as_root pacman -S --needed --noconfirm "$pkg" ;;
    suse)   as_root zypper --non-interactive install ${KDICT_ZYPPER_INSTALL_ARGS:-} "$pkg" ;;
  esac
}
for pkg in ydotool xdotool; do
  install_optional "$pkg" || echo "warning: could not install optional package '$pkg' on this distribution" >&2
done
if ! command -v ydotool >/dev/null 2>&1 && ! command -v xdotool >/dev/null 2>&1; then
  cat >&2 <<'MSG'
warning: neither ydotool nor xdotool is available. KDE Dictation needs one of them
to press the paste shortcut. Install ydotool (Wayland/X11, needs /dev/uinput:
https://github.com/ReimuNotMoe/ydotool) or xdotool (X11 only).
MSG
fi

echo "Dependencies installed."
