# KDE Dictation v1 Architecture

## Scope

KDE Dictation is a small Plasma-native frontend around an external speech-to-text service.
It intentionally does not host models or implement a desktop accessibility stack.

## Runtime components

```text
ShortcutController (KGlobalAccel)
           |
           v
DictationController -----> TrayController (KStatusNotifierItem)
     |       |       |
     |       |       +----> TextOutput -> clipboard -> ydotoold/uinput
     |       |
     |       +------------> BatchOpenAIProvider
     |                      POST /v1/audio/transcriptions
     |
     +--------------------> RealtimeAsrProvider
     |                      WS /v1/realtime (vLLM/Voxtral via LiteLLM)
     |
     +--------------------> AudioRecorder (QAudioSource)
                             mono PCM16
```

## State machine

```text
Idle -> Recording -> Finalizing -> Inserting -> Idle
                         |
                         +-- errors/fallback
```

Operational errors never intentionally terminate the long-running tray process.

## Activation modes

### Toggle

```text
key down #1 -> start
key down #2 -> stop
```

### Push-to-talk

```text
key down -> start
key up   -> stop
```

KGlobalAccel owns both press and release. Plasma owns persistence and reassignment
of the actual shortcut.

## Batch STT

The batch adapter is the compatibility baseline and should remain generic:

```text
POST {base_url}/v1/audio/transcriptions
Authorization: Bearer <api_key>
multipart:
  model
  file
  language (optional)
  response_format=json
```

Response contract:

```json
{"text":"..."}
```

## Realtime STT

The realtime adapter implements the vLLM speech-to-text Realtime API exposed
through LiteLLM `/v1/realtime?model=asr-realtime`. Audio is PCM16 mono 16 kHz,
base64 encoded inside `input_audio_buffer.append` JSON events.

`transcription.delta` is incremental text. When `live_insert=true`, deltas are
coalesced briefly and serialized through the clipboard + ydotool output path so
words appear in the focused application while the user speaks.

On `transcription.done`, the complete final transcript is reconciled against the
concatenated deltas:

```text
exact match          -> no duplicate final paste
final extends live   -> paste only the missing suffix
whitespace-only diff -> leave field unchanged, put final in clipboard
other mismatch       -> never backspace; put authoritative final in clipboard
```

This keeps live insertion safe even if a backend or fallback transcript differs.

## Automatic realtime -> batch fallback

This is a hard v1 requirement.

Audio is always retained locally even while realtime frames are sent.

Failure while recording:

```text
realtime error
  -> mark realtime failed
  -> keep recording locally
  -> user stops
  -> encode retained PCM as WAV
  -> batch transcription
```

Failure while finalizing:

```text
commit/error/timeout
  -> encode retained PCM as WAV immediately
  -> batch transcription
```

This turns realtime into a latency optimization rather than a reliability dependency.

## Text output

Batch primary path:

```text
final UTF-8 transcript
  -> QClipboard
  -> short delay
  -> ydotool sends paste shortcut only
  -> currently focused control
```

Realtime live path:

```text
transcription.delta
  -> short coalescing queue
  -> QClipboard
  -> ydotool Ctrl+V
  -> visible text while speaking
```

Only one paste operation is in flight at a time, so a newer clipboard update can
never overtake an older paste. At stream completion the complete transcript is
left in the clipboard. Do not type transcript characters with ydotool; ydotool
only synthesizes the paste shortcut, avoiding layout and Unicode problems.

If ydotool or ydotoold becomes unavailable, automatic live paste is disabled for
that stream and the assembled transcript remains available in the clipboard.

## Focus semantics

The app never opens a focus-taking window during dictation.

The transcript is inserted into whatever owns keyboard focus at insertion time.
No original-window restoration is part of v1.

## UI

Only a native StatusNotifierItem is required in v1:

- idle;
- recording + elapsed time;
- transcribing/finalizing;
- inserting;
- error.

A floating overlay and settings GUI are explicitly deferred.

## Configuration ownership

Application YAML owns behavior and provider configuration.

Plasma/KGlobalAccel owns the shortcut.

This split is intentional.

## Security

- Prefer `api_key_env` over plaintext `api_key`.
- Never log API keys or Authorization headers.
- Transcripts are not logged by default.
- `/dev/uinput` stays `0660 root:input`; do not use permanent world-write access.

## Explicit non-goals for v1

- AT-SPI text insertion;
- KWin active-window tracking;
- focus restoration;
- application-specific paste rules;
- raw `/dev/input` listeners;
- settings GUI;
- floating overlay;
- transcript history database;
- LLM cleanup;
- model hosting.
