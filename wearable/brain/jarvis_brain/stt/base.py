"""Speech-to-text contract. The watch sends a WAV file. This returns text."""

from __future__ import annotations

from typing import Protocol


class Transcriber(Protocol):
    name: str

    def transcribe(self, wav_bytes: bytes) -> str:
        """Return the words heard in the recording."""
