"""Practice transcription. Ignores the audio and returns a fixed sentence.

That lets the watch's Talk button be tested before a speech API key exists.
The chat reply that follows will say practice mode is on.
"""


class MockTranscriber:
    name = "mock"

    def transcribe(self, wav_bytes: bytes) -> str:
        del wav_bytes
        return "Jarvis, are you there?"
