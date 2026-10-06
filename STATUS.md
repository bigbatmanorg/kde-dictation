# Project Status

## v0.2.0 realtime live insertion

Implemented in this source package:

- `transcription.delta` is connected to the desktop output path.
- Deltas are coalesced and pasted serially while recording.
- `transcription.done` is reconciled so the final transcript is never pasted twice.
- Realtime -> batch fallback appends only a missing suffix when that is provably safe.
- Mismatched authoritative finals are copied to clipboard instead of destructively editing the focused control.
- vLLM string-form realtime errors are parsed.
- A WebSocket disconnect after final commit now fails immediately and invokes batch fallback instead of waiting for timeout.
- The realtime smoke-test tool uses the current vLLM/LiteLLM `/v1/realtime` JSON protocol.

Validation performed in the packaging environment:

- source-tree architecture contract passes;
- Python realtime/mock tools compile;
- all shell scripts pass `bash -n`.

The packaging environment does not contain Qt6/KF6 development packages, so the
native CMake build cannot be executed here. The immediately preceding uploaded
version was already building on the target Plasma host; run the commands below
on that host to compile and execute all Qt tests for this patch.

```bash
./scripts/test.sh
./scripts/install-user.sh
```

`install-user.sh` keeps the existing `~/.config/kde-dictation/config.yaml`. New
live insertion settings default to:

```yaml
transcription:
  realtime:
    live_insert: true
    live_flush_ms: 75

output:
  live_paste_delay_ms: 40
```
