"""Pick the speech-to-text backend from settings."""

from __future__ import annotations

from jarvis_brain.config import Settings
from jarvis_brain.stt.base import Transcriber
from jarvis_brain.stt.mock import MockTranscriber
from jarvis_brain.stt.openai_whisper import WhisperTranscriber


def build_transcriber(settings: Settings) -> Transcriber:
    if settings.using_mock_stt or settings.stt_provider == "mock":
        return MockTranscriber()
    return WhisperTranscriber(
        base_url=settings.stt_base_url,
        api_key=settings.stt_api_key,
        model=settings.stt_model,
        timeout=settings.llm_timeout,
    )
