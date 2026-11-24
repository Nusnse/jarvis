"""Check the brain without a network and without an API key.

    python -m jarvis_brain.selftest
"""

from __future__ import annotations

import os
import struct
import sys


def _silent_wav() -> bytes:
    """A tenth of a second of silence, 16 kHz, 16-bit, mono."""
    samples = 1600
    data = b"\x00\x00" * samples
    header = b"RIFF" + struct.pack("<I", 36 + len(data)) + b"WAVEfmt "
    header += struct.pack("<IHHIIHH", 16, 1, 1, 16000, 32000, 2, 16)
    header += b"data" + struct.pack("<I", len(data))
    return header + data


def main() -> None:
    os.environ["LLM_PROVIDER"] = "mock"
    os.environ["STT_PROVIDER"] = "mock"
    os.environ["LLM_API_KEY"] = ""
    os.environ["STT_API_KEY"] = ""

    from fastapi.testclient import TestClient

    from jarvis_brain.server import create_app

    client = TestClient(create_app())

    health = client.get("/health")
    assert health.status_code == 200, health.text
    body = health.json()
    assert body["ok"] is True
    assert body["using_mock_llm"] is True
    assert body["tts_ready"] is False
    print("Health OK. Practice mode:", body["mock_llm_reason"])

    page = client.get("/")
    assert page.status_code == 200
    assert b"Jarvis" in page.content
    print("Test page OK.")

    first = client.post(
        "/v1/chat",
        json={"text": "Good morning, Jarvis", "session_id": "selftest", "device_id": "selftest"},
    )
    assert first.status_code == 200, first.text
    reply = first.json()
    assert reply["ok"] is True
    assert reply["reply"]
    assert reply["session_id"] == "selftest"
    print("Chat OK:", reply["reply"])

    second = client.post(
        "/v1/chat",
        json={"text": "Tell me a short joke.", "session_id": "selftest", "device_id": "selftest"},
    )
    assert "Practice mode" in second.json()["reply"] or "practice" in second.json()["reply"].lower()
    print("Memory path OK:", second.json()["reply"])

    voice = client.post(
        "/v1/voice",
        content=_silent_wav(),
        headers={"content-type": "audio/wav", "x-session-id": "selftest-voice", "x-device-id": "selftest"},
    )
    assert voice.status_code == 200, voice.text
    heard = voice.json()
    assert heard["transcript"] == "Jarvis, are you there?"
    assert heard["reply"]
    print("Voice OK:", heard["transcript"], "->", heard["reply"])

    with client.websocket_connect("/ws") as socket:
        socket.send_json({"text": "hello", "device_id": "selftest"})
        message = socket.receive_json()
        assert message["reply"]
        print("WebSocket OK:", message["reply"])

    print()
    print("All brain checks passed.")


if __name__ == "__main__":
    try:
        main()
    except AssertionError as exc:
        print("A check failed:", exc)
        sys.exit(1)
