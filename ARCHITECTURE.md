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
     |                      WS /v1/audio/transcriptions/realtime
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

The realtime adapter is intentionally backend-specific and isolated. It currently
implements the NeMo-Speech.cpp realtime transcription protocol exposed by the
user's authenticated LiteLLM relay.

The desktop app must not leak this protocol into unrelated components.

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

Primary path:

```text
final UTF-8 transcript
  -> QClipboard
  -> short delay
  -> ydotool sends paste shortcut only
  -> currently focused control
```

Do not type the transcript character-by-character with ydotool. Clipboard transport
avoids keyboard-layout and Unicode problems.

If ydotool or ydotoold is unavailable, leave the text in the clipboard and surface
a warning; the transcript must not be lost.

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
- partial realtime typing;
- model hosting.
