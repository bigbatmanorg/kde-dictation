#!/usr/bin/env bash
set -euo pipefail

sudo modprobe uinput
sudo groupadd -f input
sudo usermod -aG input "$USER"

echo 'KERNEL=="uinput", GROUP="input", MODE="0660", TAG+="uaccess", OPTIONS+="static_node=uinput"' \
  | sudo tee /etc/udev/rules.d/80-uinput.rules >/dev/null

sudo udevadm control --reload-rules
sudo udevadm trigger
sudo chgrp input /dev/uinput || true
sudo chmod 0660 /dev/uinput || true

printf '\n/dev/uinput:\n'
ls -l /dev/uinput
printf '\nCurrent groups:\n'
id

systemctl --user enable ydotool.service

if id -nG | tr ' ' '\n' | grep -qx input; then
  systemctl --user start ydotool.service
  sleep 0.5
  systemctl --user --no-pager status ydotool.service || true
else
  cat <<'MSG'

The input group was added to your account, but the current Plasma session has not
picked it up yet. Log out of Plasma completely and log back in (or reboot), then:

  systemctl --user start ydotool.service
  kde-dictation --check

Do NOT leave /dev/uinput mode 0666.
MSG
fi
