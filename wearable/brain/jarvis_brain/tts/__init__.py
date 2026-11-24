"""Text-to-speech is a later phase.

The watch already has a speaker codec (ES8311) and an amplifier pin (PA,
GPIO 46). When this grows up, synthesize() should return WAV or MP3 bytes
and the HTTP reply should carry them. The watch would then play them.

TTS_PROVIDER in the .env file is recorded so the switch has a home, but
this version always returns nothing.
"""

from __future__ import annotations

from typing import Protocol


class Speaker(Protocol):
    name: str

    def synthesize(self, text: str) -> bytes | None:
        """Return audio bytes, or None when speech-out is not available."""


class SilentSpeaker:
    name = "none"

    def synthesize(self, text: str) -> bytes | None:
        del text
        # TODO(phase-later): call a cloud or local text-to-speech API and
        # return a WAV the watch can play through the ES8311 codec.
        return None


def build_speaker(provider: str) -> Speaker:
    del provider
    return SilentSpeaker()
