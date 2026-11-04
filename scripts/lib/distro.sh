#!/usr/bin/env bash
# Shared helpers: detect the distribution family and the package manager.
# Sourced by the setup scripts; never executed directly.

# Echoes: debian | fedora | arch | suse | unknown
detect_family() {
  local id="" like=""
  if [[ -r /etc/os-release ]]; then
    # shellcheck disable=SC1091
    id="$(. /etc/os-release; echo "${ID:-}")"
    like="$(. /etc/os-release; echo "${ID_LIKE:-}")"
  fi
  local all=" $id $like "
  case "$all" in
    *" debian "*|*" ubuntu "*) echo debian ;;
    *" fedora "*|*" rhel "*|*" centos "*) echo fedora ;;
    *" arch "*) echo arch ;;
    *" suse "*|*" opensuse "*|*" sles "*) echo suse ;;
    *)
      if   command -v apt-get >/dev/null 2>&1; then echo debian
      elif command -v dnf     >/dev/null 2>&1; then echo fedora
      elif command -v pacman  >/dev/null 2>&1; then echo arch
      elif command -v zypper  >/dev/null 2>&1; then echo suse
      else echo unknown; fi ;;
  esac
}

# Runs a command as root, using sudo only when needed and available.
as_root() {
  if [[ ${EUID:-$(id -u)} -eq 0 ]]; then
    "$@"
  elif command -v sudo >/dev/null 2>&1; then
    sudo "$@"
  else
    echo "error: root privileges required for: $*  (install sudo or run as root)" >&2
    return 1
  fi
}
