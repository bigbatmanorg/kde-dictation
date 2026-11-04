# Changelog

## 0.3.1

First release with multi-distribution support (a `v0.3.0` tag exists but its release pipeline failed and published nothing).
Verified on Fedora KDE, Kubuntu/Ubuntu, Debian, Arch and openSUSE.

- Fix Fedora (and other distros without a user `ydotool.service`): the app no longer
  assumes `$XDG_RUNTIME_DIR/.ydotool_socket`. ydotoold sockets are discovered
  (`output.ydotool_socket`, `$YDOTOOL_SOCKET`, runtime dir, `/tmp`), stale sockets are
  detected, and a private `ydotoold` is started on demand (`output.ydotool_autostart`).
- New pluggable paste backend (`output.backend: auto|ydotool|xdotool`); xdotool is used
  on X11 sessions without requiring `/dev/uinput` access.
- `audio.device` is now honoured (id or description fragment); new `--list-devices`.
- `--help` / `--version` work without a display.
- `--check` reports session type, backend, `/dev/uinput` and actionable hints.
- Distro-aware `scripts/setup.sh`, `install-deps.sh` (apt, dnf, pacman, zypper) and
  `setup-uinput.sh` (uses logind `uaccess`; `input` group only as a fallback).
  `setup-kubuntu.sh` remains as an alias.
- `scripts/test-in-container.sh` plus GitHub Actions matrix for Fedora, Ubuntu, Debian, Arch, openSUSE.
- Packaging: RPM spec, Arch PKGBUILD, CPack DEB/RPM, AppStream metadata, XDG autostart option.
- Minimum Qt lowered from 6.8 to 6.6 (KF6.0 floor).

## 0.2.0

- Insert vLLM/Voxtral `transcription.delta` text into the focused application while speaking.
- Coalesce small deltas and serialize clipboard/ydotool paste operations.
- Reconcile `transcription.done` with already-streamed text to prevent duplicate final insertion.
- Preserve realtime-to-batch fallback: append only a safe missing suffix when possible.
- On final mismatch, avoid destructive backspacing and copy the authoritative transcript to clipboard.
- Treat post-commit realtime disconnects as immediate failures so fallback starts promptly.
- Parse vLLM string-form realtime error events.
- Update realtime smoke test and documentation to `/v1/realtime`.
