#!/usr/bin/env python3
"""Smoke-test the deployed NeMo/LiteLLM realtime ASR path.

Usage:
  python3 tools/test_realtime.py WSS_URL API_KEY audio.pcm

The PCM file must be little-endian mono PCM16 at 16 kHz unless you change
SAMPLE_RATE below. This intentionally matches the protocol in the user's
nemo-stt/LiteLLM relay bundle, not the generic OpenAI Realtime protocol.
"""
import asyncio
import json
import sys

SAMPLE_RATE = 16000

async def main():
    try:
        import websockets
    except ImportError as exc:
        raise SystemExit("pip install websockets") from exc

    if len(sys.argv) != 4:
        raise SystemExit("usage: test_realtime.py <ws-url> <api-key> <pcm16-file>")
    url, key, pcm = sys.argv[1:]

    async with websockets.connect(
        url,
        additional_headers={"Authorization": f"Bearer {key}"},
        max_size=None,
    ) as ws:
        print("<-", await ws.recv())
        await ws.send(json.dumps({
            "type": "session.update",
            "session": {
                "sample_rate": SAMPLE_RATE,
                "automatic_punctuation": True,
            },
        }))
        print("<-", await ws.recv())

        with open(pcm, "rb") as f:
            while chunk := f.read(3200):
                await ws.send(chunk)
                await asyncio.sleep(0.1)

        await ws.send(json.dumps({"type": "input_audio_buffer.commit"}))
        while True:
            msg = await ws.recv()
            print("<-", msg)
            if isinstance(msg, str):
                obj = json.loads(msg)
                if str(obj.get("type", "")).endswith(".completed"):
                    return

if __name__ == "__main__":
    asyncio.run(main())
