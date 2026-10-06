# Project Status

KDE Dictation is ready for Kubuntu / Plasma 6 use.

For a new clone, run:

```bash
./scripts/setup-kubuntu.sh
```

The script installs dependencies, configures `ydotoold`, builds and installs the
application, creates a private user config, and enables autostart. Fill in only
the LiteLLM URL and API key in `~/.config/kde-dictation/config.yaml`.

Validated on the target Plasma host:

- CMake build succeeds with Qt 6 / KF6.
- All deterministic unit and repository-contract tests pass.
- Local mock batch-STT integration passes.
- User installation creates a `0600` config and an absolute autostart command.
- 16 kHz mono PCM16 capture is supported by the detected microphone.
- Clipboard output works on Plasma Wayland, including a Klipper stale-offer fallback.
