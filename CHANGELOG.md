# Changelog

## 0.2.0

- Insert vLLM/Voxtral `transcription.delta` text into the focused application while speaking.
- Coalesce small deltas and serialize clipboard/ydotool paste operations.
- Reconcile `transcription.done` with already-streamed text to prevent duplicate final insertion.
- Preserve realtime-to-batch fallback: append only a safe missing suffix when possible.
- On final mismatch, avoid destructive backspacing and copy the authoritative transcript to clipboard.
- Treat post-commit realtime disconnects as immediate failures so fallback starts promptly.
- Parse vLLM string-form realtime error events.
- Update realtime smoke test and documentation to `/v1/realtime`.
