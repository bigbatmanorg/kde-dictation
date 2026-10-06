# KDE Dictation

Small native KDE Plasma 6 dictation client.

The app owns only desktop integration and audio capture. Speech recognition is external.

```text
KGlobalAccel
   -> record microphone
   -> batch OpenAI-compatible STT OR realtime NeMo ASR
   -> final transcript
   -> Qt clipboard
   -> ydotool Ctrl+V
   -> currently focused application
```

## v1 principles

- One native C++/Qt/KF6 process.
- Plasma owns the global keyboard binding.
- Activation can be `toggle` or `push_to_talk`.
- STT can be `batch` or `realtime`.
- Realtime always retains a local PCM copy.
- Realtime failure automatically falls back to batch when enabled.
- No AT-SPI, active-window tracking, focus restoration, raw evdev listener, or settings GUI.
- The app must never steal keyboard focus.
- On Plasma Wayland, clipboard output verifies Klipper has the final transcript before pasting.

## Supported STT contracts

### Batch

OpenAI-compatible multipart endpoint:

```text
POST /v1/audio/transcriptions
Authorization: Bearer <key>
model=whisper-1
file=@dictation.wav
```

Expected response:

```json
{"text":"..."}
```

### Realtime

The current realtime adapter targets the protocol already exposed by the user's
NeMo-Speech.cpp + LiteLLM WebSocket relay:

```text
WS(S) /v1/audio/transcriptions/realtime
Authorization: Bearer <key>
```

Flow:

1. receive `session.created`
2. send `session.update`
3. stream binary little-endian mono PCM16
4. send `input_audio_buffer.commit`
5. wait for `conversation.item.input_audio_transcription.completed`

This realtime protocol is deliberately isolated in `RealtimeAsrProvider`; it is
not claimed to be the generic OpenAI Realtime API.

## Kubuntu / Plasma 6 setup

From a fresh clone, run one command:

```bash
./scripts/setup-kubuntu.sh
```

It installs dependencies, configures `/dev/uinput` safely for `ydotoold`, builds
and installs the application, enables autostart, and creates a private config at:

```text
~/.config/kde-dictation/config.yaml
```

Paste only your LiteLLM URL and API key into that file:

```yaml
transcription:
  base_url: https://YOUR-LITELLM-HOST
  api_key: YOUR_LITELLM_KEY
```

The config is mode `0600`, so only your user can read the key. An environment
variable is also supported for managed-secret setups:

```yaml
transcription:
  api_key_env: LITELLM_API_KEY
```

If setup added your user to the `input` group, log out of Plasma completely and
back in once. Do not use `/dev/uinput` mode `0666` as a workaround.

Launch **KDE Dictation** from the application menu, then assign a shortcut in
**System Settings → Keyboard → Shortcuts → KDE Dictation → Dictate**. The default
is `Meta+Shift+D`.

For an existing development checkout, the separate steps remain available:

```bash
./scripts/install-deps-kubuntu.sh
./scripts/setup-uinput.sh
./scripts/test.sh
./scripts/install-user.sh
```

For development:

```bash
./scripts/run-dev.sh
```

The default global shortcut is `Meta+Shift+D`. Once the app has run and
registered its action, change **KDE Dictation -> Dictate** in Plasma's global
shortcut settings. The shortcut is intentionally not stored in YAML.

## Configuration

See [`config.example.yaml`](config.example.yaml).

Important switches:

```yaml
activation:
  mode: toggle             # or push_to_talk

transcription:
  mode: batch              # or realtime

  realtime:
    fallback_to_batch: true
    finalization_timeout_ms: 5000
```

## Diagnostics

Check local prerequisites:

```bash
kde-dictation --check
```

Test only clipboard + ydotool insertion:

```bash
./scripts/smoke-output.sh
```

You have three seconds to focus the target field.

Test the configured batch STT endpoint:

```bash
./scripts/smoke-batch.sh speech.wav
```

Test the deployed realtime WebSocket independently of the desktop app:

```bash
pip install websockets
python3 tools/test_realtime.py \
  wss://YOUR-LITELLM/v1/audio/transcriptions/realtime \
  "$LITELLM_API_KEY" \
  speech.pcm
```

Create the required PCM file with:

```bash
ffmpeg -i speech.wav -f s16le -acodec pcm_s16le -ac 1 -ar 16000 speech.pcm
```

## Tests

```bash
./scripts/test.sh
```

Optional HTTP integration smoke test (uses a local mock server):

```bash
./scripts/test-mock-batch.sh
```

Current deterministic tests cover:

- state-machine transitions;
- YAML configuration + environment-key precedence;
- WAV header generation;
- realtime event parsing;
- HTTP/WSS URL normalization;
- automatic realtime -> batch fallback policy;
- source-tree architecture contract.

## Installation layout

User install defaults to:

```text
~/.local/bin/kde-dictation
~/.local/share/applications/org.amvc.kdedictation.desktop
~/.config/kde-dictation/config.yaml
~/.config/autostart/org.amvc.kdedictation.desktop
```

## Current v1 limitations

- Audio capture requests mono PCM16 at the configured sample rate (default 16 kHz).
  There is intentionally no resampler yet. If a real target microphone rejects
  this PipeWire format, add resampling rather than redesigning the app.
- Realtime partials are not typed into applications.
- The transcript is left in the clipboard after insertion.
- The target is whichever control owns keyboard focus when paste occurs.
- Only `ctrl+v` and `shift+insert` output shortcuts are currently supported.

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and
[`AI_FINISHING_PROMPT.md`](AI_FINISHING_PROMPT.md).
