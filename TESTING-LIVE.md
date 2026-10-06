# Realtime live insertion test

## 1. Build and install

```bash
./scripts/test.sh
./scripts/install-user.sh
```

Your existing config is kept. Make sure it contains or inherits:

```yaml
transcription:
  mode: realtime
  realtime:
    path: /v1/realtime
    model: asr-realtime
    fallback_to_batch: true
    live_insert: true
    live_flush_ms: 75

output:
  paste_shortcut: ctrl+v
  live_paste_delay_ms: 40
```

## 2. Restart the app

```bash
pkill -x kde-dictation || true
~/.local/bin/kde-dictation
```

Keep that terminal visible for the first test.

## 3. Live test

Focus a text field in Kate, Firefox/ChatGPT, VS Code, or another application.
Start dictation with your KDE global shortcut and speak continuously.

Expected behavior:

1. Words/phrases appear in the focused control while you are still speaking.
2. Stopping dictation does not paste the entire transcript a second time.
3. The complete final transcript is left in the clipboard.

## 4. Fallback test

While recording, deliberately stop or disconnect the realtime backend and keep
speaking. Stop dictation normally.

Expected behavior:

- already streamed text remains in the field;
- the retained recording is submitted to batch `whisper-1`;
- if the batch result begins with the already-streamed text, only its missing
  suffix is pasted;
- if it disagrees, the field is not backspaced or rewritten and the authoritative
  batch result is placed in the clipboard with a notification.

## 5. Latency tuning

If text feels too chunky, lower:

```yaml
live_flush_ms: 50
```

If clipboard/paste occasionally races on Plasma, increase:

```yaml
live_paste_delay_ms: 60
```

Start with the defaults (75/40 ms) before tuning.
