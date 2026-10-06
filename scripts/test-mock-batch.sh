#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
PORT="${KDICT_MOCK_PORT:-18765}"
TMP="$(mktemp -d)"
SERVER_PID=""
cleanup() {
  [[ -n "$SERVER_PID" ]] && kill "$SERVER_PID" 2>/dev/null || true
  rm -rf "$TMP"
}
trap cleanup EXIT

if [[ ! -x "$BUILD_DIR/kde-dictation" ]]; then
  "$ROOT/scripts/build.sh"
fi

python3 "$ROOT/tests/integration/mock_batch_server.py" "$PORT" &
SERVER_PID=$!
sleep 0.3

cat > "$TMP/config.yaml" <<YAML
activation:
  mode: toggle
audio:
  sample_rate: 16000
  max_recording_seconds: 60
transcription:
  mode: batch
  base_url: http://127.0.0.1:$PORT
  api_key: test
  batch:
    model: whisper-1
    path: /v1/audio/transcriptions
    language: auto
  realtime:
    fallback_to_batch: true
    finalization_timeout_ms: 1000
output:
  method: clipboard_paste
  paste_shortcut: ctrl+v
YAML

python3 - "$TMP/test.wav" <<'PY'
import sys, wave
p=sys.argv[1]
with wave.open(p, 'wb') as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(16000)
    w.writeframes(b'\0\0' * 1600)
PY

result="$(QT_QPA_PLATFORM=offscreen "$BUILD_DIR/kde-dictation" --config "$TMP/config.yaml" --test-stt "$TMP/test.wav")"
[[ "$result" == "mock transcript" ]]
echo "mock batch integration: OK"
