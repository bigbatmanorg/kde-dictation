# Project Status

## v0.3.1 multi-distribution support

- Builds and passes the unit tests in clean Fedora, Ubuntu 25.10/26.04, Debian 13, Arch and
  openSUSE Tumbleweed containers (`scripts/test-in-container.sh`, GitHub Actions matrix).
- End-to-end paste (clipboard + key injection, incl. non-ASCII text) verified on Fedora 45 KDE
  Plasma 6.8 Wayland with a self-managed `ydotoold`.
- Not verified on real desktops: Kubuntu, Arch, openSUSE, X11/xdotool path (argument mapping is unit tested).

See [`CHANGELOG.md`](CHANGELOG.md) for details and `TESTING-LIVE.md` for the manual live checklist.
