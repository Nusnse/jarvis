"""OpenAI-compatible audio transcription (the Whisper HTTP endpoint)."""

from __future__ import annotations

import httpx

from jarvis_brain.llm.openai_compatible import ModelError


class WhisperTranscriber:
    name = "openai"

    def __init__(self, base_url: str, api_key: str, model: str, timeout: float) -> None:
        self.base_url = base_url.rstrip("/")
        self.api_key = api_key
        self.model = model
        self.timeout = timeout

    def transcribe(self, wav_bytes: bytes) -> str:
        url = f"{self.base_url}/audio/transcriptions"
        headers = {"Authorization": f"Bearer {self.api_key}"}
        files = {"file": ("speech.wav", wav_bytes, "audio/wav")}
        data = {"model": self.model}
        try:
            response = httpx.post(url, headers=headers, files=files, data=data, timeout=self.timeout)
        except httpx.HTTPError as exc:
            raise ModelError("Could not reach the speech service.") from exc
        if response.status_code >= 400:
            detail = response.text.strip().replace("\n", " ")
            if len(detail) > 180:
                detail = detail[:180] + "..."
            raise ModelError(f"Speech service returned HTTP {response.status_code}. {detail}")
        try:
            text = response.json()["text"]
        except (KeyError, TypeError, ValueError) as exc:
            raise ModelError("Speech service answered in an unexpected shape.") from exc
        return (text or "").strip()
