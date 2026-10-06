#!/usr/bin/env bash
set -euo pipefail

sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  ninja-build \
  qt6-base-dev \
  qt6-multimedia-dev \
  qt6-websockets-dev \
  libkf6globalaccel-dev \
  libkf6statusnotifieritem-dev \
  libyaml-cpp-dev \
  ydotool \
  python3 \
  python3-websockets

echo "Dependencies installed."
echo "Run ./scripts/setup-uinput.sh next if ydotoold cannot access /dev/uinput."
