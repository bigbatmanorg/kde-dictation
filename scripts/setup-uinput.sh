#!/usr/bin/env bash
# Give the current user access to /dev/uinput (needed by ydotool on Wayland)
# without ever making it world-writable. Works on Debian/Ubuntu, Fedora, Arch
# and openSUSE. Not needed on X11 when the xdotool backend is used.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=lib/distro.sh
source "$ROOT/scripts/lib/distro.sh"

can_write() { [[ -w /dev/uinput ]]; }

as_root modprobe uinput || true
# Load the module on every boot.
echo uinput | as_root tee /etc/modules-load.d/kde-dictation-uinput.conf >/dev/null

if can_write; then
  echo "/dev/uinput is already writable by $USER - nothing more to do."
else
  # 'uaccess' grants the user at the active seat an ACL (systemd-logind), the
  # same mechanism used for other input devices. The 'input' group is the
  # fallback for sessions without logind seat ACLs.
  as_root groupadd -f input
  echo 'KERNEL=="uinput", SUBSYSTEM=="misc", GROUP="input", MODE="0660", TAG+="uaccess", OPTIONS+="static_node=uinput"' \
    | as_root tee /etc/udev/rules.d/80-kde-dictation-uinput.rules >/dev/null
  as_root udevadm control --reload-rules
  as_root udevadm trigger --subsystem-match=misc --sysname-match=uinput || true
  sleep 0.5

  if ! can_write; then
    if ! id -nG "$USER" | tr ' ' '\n' | grep -qx input; then
      as_root usermod -aG input "$USER"
      NEED_RELOGIN=1
    fi
    # Apply the group to the device node right now.
    as_root chgrp input /dev/uinput || true
    as_root chmod 0660 /dev/uinput || true
  fi
fi

printf '\n/dev/uinput:\n'
ls -l /dev/uinput
if can_write; then
  echo "OK: writable by the current session."
elif [[ "${NEED_RELOGIN:-0}" == 1 ]]; then
  cat <<'MSG'

You were added to the 'input' group. Log out of the desktop session completely
and log back in (or reboot) so the new group applies, then verify with:

  kde-dictation --check

Do NOT make /dev/uinput mode 0666.
MSG
else
  echo "Not writable yet; log out/in, then run: kde-dictation --check" >&2
fi
