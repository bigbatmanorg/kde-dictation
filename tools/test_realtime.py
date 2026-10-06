#!/usr/bin/env python3
"""Smoke-test vLLM/Voxtral realtime ASR through LiteLLM.

Usage:
  python3 tools/test_realtime.py \
    'wss://LITELLM/v1/realtime?model=asr-realtime' API_KEY audio.pcm

The PCM file must be little-endian mono PCM16 at 16 kHz.
"""
import asyncio
import base64
import json
import sys

CHUNK_BYTES = 3200  # 100 ms of PCM16 mono at 16 kHz


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
        # Realtime ASR commonly runs on a private LAN host.  Bypass automatic
        # proxy discovery so a corporate/system proxy cannot reject its WS
        # upgrade before the backend receives the request.
        proxy=None,
    ) as ws:
        created = json.loads(await ws.recv())
        print("<-", json.dumps(created, ensure_ascii=False))
        if created.get("type") != "session.created":
            raise RuntimeError(f"expected session.created, got {created}")

        await ws.send(json.dumps({"type": "session.update", "model": "asr-realtime"}))
        # vLLM starts a streaming utterance after a non-final commit.
        await ws.send(json.dumps({"type": "input_audio_buffer.commit"}))

        async def receive():
            full = []
            while True:
                msg = await ws.recv()
                if not isinstance(msg, str):
                    continue
                obj = json.loads(msg)
                event_type = obj.get("type", "")
                if event_type == "transcription.delta":
                    delta = obj.get("delta", obj.get("text", ""))
                    full.append(delta)
                    print(delta, end="", flush=True)
                elif event_type == "transcription.done":
                    print("\n\nFINAL:", obj.get("text", ""))
                    return
                elif event_type == "error":
                    raise RuntimeError(obj.get("error", obj))

        receiver = asyncio.create_task(receive())
        with open(pcm, "rb") as f:
            while chunk := f.read(CHUNK_BYTES):
                await ws.send(json.dumps({
                    "type": "input_audio_buffer.append",
                    "audio": base64.b64encode(chunk).decode("ascii"),
                }))
                await asyncio.sleep(0.1)

        await ws.send(json.dumps({"type": "input_audio_buffer.commit", "final": True}))
        await receiver


if __name__ == "__main__":
    asyncio.run(main())
